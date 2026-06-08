/*
 * locker_async.c — main-thread walk+snapshot, worker SQL apply.
 *
 * Design:
 *  - Per-locker dirty slots coalesce many save triggers into one generation.
 *  - At most LOCKER_ASYNC_SNAPSHOTS_PER_PULSE new snapshots start per pulse.
 *  - At most LOCKER_ASYNC_MAX_INFLIGHT jobs are in the worker concurrently.
 *  - Snapshot is an immutable multi-statement SQL blob built on the main thread
 *    while that player is object-command locked.
 *  - Worker never touches P_char/P_obj; only applies the sealed SQL blob.
 *  - Completion runs on the next main pulse (extract terminal locker char, unlock).
 *
 * Note: private-chest rows are still written synchronously inside
 * StorageLocker::LockerToPFile() today. This path asyncs the bulk public inventory
 * carried by the locker character after LockerToPFile transitions.
 */

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "sql.h"
#include "sql_pool.h"
#include "sql_player.h"
#include "storage_lockers.h"
#include "locker_async.h"
#include "comm.h"
#include "db.h"
#include "events.h"

#include <mysql/mysql.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

extern P_index obj_index;
extern P_char  character_list;
extern P_room  world;
extern const int top_of_world;

#define LOCKER_ASYNC_SLOTS       128
#define LOCKER_ASYNC_NAME_LEN    128
#define LOCKER_ASYNC_JOBS        16
#define LOCKER_ASYNC_RESULTS     32
#define LOCKER_ASYNC_SCRIPT_INIT (64 * 1024)

enum locker_async_state
{
	LCHK_FREE = 0,
	LCHK_DIRTY,
	LCHK_INFLIGHT
};

struct locker_async_slot
{
	enum locker_async_state state;
	char   locker_name[LOCKER_ASYNC_NAME_LEN];
	int    locker_id;
	int    owner_pid;
	int    owner_assoc_id;
	int    terminal;
	int    rebuild_objects; /* another dirty landed while inflight */
	unsigned long gen;
	time_t dirty_at;
	int    user_pid;
	P_char chLocker;
	P_char chUser;
};

struct locker_async_job
{
	int    used;
	char   locker_name[LOCKER_ASYNC_NAME_LEN];
	unsigned long gen;
	int    terminal;
	int    user_pid;
	char  *sql;
};

struct locker_async_result
{
	int    used;
	char   locker_name[LOCKER_ASYNC_NAME_LEN];
	unsigned long gen;
	int    ok;
	int    terminal;
	int    user_pid;
};

static struct locker_async_slot g_slots[LOCKER_ASYNC_SLOTS];
static unsigned long g_gen_seq = 1;
static int g_snapshots_started_this_pulse = 0;
static int g_inflight = 0;

static pthread_mutex_t g_q_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_q_cv = PTHREAD_COND_INITIALIZER;
static struct locker_async_job g_jobs[LOCKER_ASYNC_JOBS];
static struct locker_async_result g_results[LOCKER_ASYNC_RESULTS];
static int g_worker_running = 0;
static int g_worker_created = 0;
static int g_worker_stop = 0;
static pthread_t g_worker_tid;
static int g_inited = 0;

static int locker_async_worker_available(void)
{
	int available;
	pthread_mutex_lock(&g_q_mu);
	available = g_worker_created && g_worker_running && !g_worker_stop;
	pthread_mutex_unlock(&g_q_mu);
	return available;
}

/* ---------------- slot helpers (main only) ---------------- */

static struct locker_async_slot *slot_find(const char *name)
{
	int i;
	if (!name || !*name)
		return NULL;
	for (i = 0; i < LOCKER_ASYNC_SLOTS; i++)
		if (g_slots[i].state != LCHK_FREE && !str_cmp(g_slots[i].locker_name, name))
			return &g_slots[i];
	return NULL;
}

static struct locker_async_slot *slot_alloc(const char *name)
{
	int i;
	struct locker_async_slot *s = slot_find(name);
	if (s)
		return s;
	for (i = 0; i < LOCKER_ASYNC_SLOTS; i++)
	{
		if (g_slots[i].state == LCHK_FREE)
		{
			memset(&g_slots[i], 0, sizeof(g_slots[i]));
			snprintf(g_slots[i].locker_name, sizeof(g_slots[i].locker_name), "%s", name);
			g_slots[i].state = LCHK_DIRTY;
			return &g_slots[i];
		}
	}
	return NULL;
}

static void slot_clear(struct locker_async_slot *s)
{
	if (!s)
		return;
	memset(s, 0, sizeof(*s));
	s->state = LCHK_FREE;
}

int locker_async_player_obj_locked(P_char ch)
{
	int i;
	int pid;

	if (!ch || IS_NPC(ch))
		return 0;
	pid = GET_PID(ch);
	if (pid <= 0)
		return 0;
	/* Only lock while DIRTY (waiting for / during the main-thread snapshot
	 * start). Once INFLIGHT the SQL payload is sealed; unlock the player. */
	for (i = 0; i < LOCKER_ASYNC_SLOTS; i++)
	{
		if (g_slots[i].state == LCHK_DIRTY && g_slots[i].user_pid == pid)
			return 1;
	}
	return 0;
}

int locker_async_name_busy(const char *locker_name)
{
	struct locker_async_slot *s = slot_find(locker_name);
	return (s && s->state != LCHK_FREE) ? 1 : 0;
}

/* ---------------- job/result queue (shared) ---------------- */

static int job_push(struct locker_async_job *src)
{
	int i;
	pthread_mutex_lock(&g_q_mu);
	for (i = 0; i < LOCKER_ASYNC_JOBS; i++)
	{
		if (!g_jobs[i].used)
		{
			g_jobs[i] = *src;
			g_jobs[i].used = 1;
			src->sql = NULL;
			pthread_cond_signal(&g_q_cv);
			pthread_mutex_unlock(&g_q_mu);
			return 1;
		}
	}
	pthread_mutex_unlock(&g_q_mu);
	return 0;
}

static int job_pop(struct locker_async_job *dst)
{
	int i;
	for (i = 0; i < LOCKER_ASYNC_JOBS; i++)
	{
		if (g_jobs[i].used)
		{
			*dst = g_jobs[i];
			memset(&g_jobs[i], 0, sizeof(g_jobs[i]));
			return 1;
		}
	}
	return 0;
}

static void result_push_locked(const struct locker_async_result *r)
{
	int i;
	for (i = 0; i < LOCKER_ASYNC_RESULTS; i++)
	{
		if (!g_results[i].used)
		{
			g_results[i] = *r;
			g_results[i].used = 1;
			return;
		}
	}
	logit(LOG_FILE, "locker_async: result queue full for %s gen=%lu", r->locker_name, r->gen);
}

/* ---------------- growable SQL script builders (main only) ---------------- */

struct la_buf
{
	char  *data;
	size_t len;
	size_t cap;
	int    failed;
};

static int la_buf_init(struct la_buf *b, size_t cap)
{
	memset(b, 0, sizeof(*b));
	b->data = (char *)malloc(cap);
	if (!b->data)
	{
		b->failed = 1;
		return 0;
	}
	b->cap = cap;
	b->data[0] = '\0';
	return 1;
}

static void la_buf_free(struct la_buf *b)
{
	if (b && b->data)
		free(b->data);
	if (b)
		memset(b, 0, sizeof(*b));
}

static int la_buf_reserve(struct la_buf *b, size_t need)
{
	char *n;
	size_t ncap;
	if (b->failed)
		return 0;
	if (b->len + need + 1 <= b->cap)
		return 1;
	ncap = b->cap ? b->cap : LOCKER_ASYNC_SCRIPT_INIT;
	while (b->len + need + 1 > ncap)
		ncap *= 2;
	n = (char *)realloc(b->data, ncap);
	if (!n)
	{
		b->failed = 1;
		return 0;
	}
	b->data = n;
	b->cap = ncap;
	return 1;
}

static int la_buf_puts(struct la_buf *b, const char *s)
{
	size_t n;
	if (!s)
		return 1;
	n = strlen(s);
	if (!la_buf_reserve(b, n))
		return 0;
	memcpy(b->data + b->len, s, n);
	b->len += n;
	b->data[b->len] = '\0';
	return !b->failed;
}

static int la_buf_printf(struct la_buf *b, const char *fmt, ...)
{
	va_list ap;
	int need;
	if (b->failed)
		return 0;
	va_start(ap, fmt);
	need = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (need < 0)
	{
		b->failed = 1;
		return 0;
	}
	if (!la_buf_reserve(b, (size_t)need + 1))
		return 0;
	va_start(ap, fmt);
	vsnprintf(b->data + b->len, b->cap - b->len, fmt, ap);
	va_end(ap);
	b->len += (size_t)need;
	return 1;
}

static char *la_esc(const char *s)
{
	return sql_escape_string(s ? s : "");
}

static int g_tmp_id_seq;

static int emit_item_sql(struct la_buf *b, P_obj obj, int locker_id, int chest_id, int parent_tmp)
{
	int tmp;
	int vnum;
	char *esc_name = NULL, *esc_short = NULL, *esc_desc = NULL, *esc_action = NULL;
	char name_str[1024], short_str[1024], desc_str[2048], action_str[2048];
	char wear_str[32], type_str[16], material_str[16];
	char bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];
	char container_str[64], chest_id_str[32];
	int i;
	P_obj child;
	struct extra_descr_data *ed;

	if (!obj)
		return 1;
	if (obj->R_num < 0)
		return 0;

	vnum = obj_index[obj->R_num].virtual_number;
	tmp = ++g_tmp_id_seq;

	if (obj->str_mask & STRUNG_KEYS)
		esc_name = la_esc(obj->name);
	if (obj->str_mask & STRUNG_DESC2)
		esc_short = la_esc(obj->short_description);
	if (obj->str_mask & STRUNG_DESC1)
		esc_desc = la_esc(obj->description);
	if (obj->str_mask & STRUNG_DESC3)
		esc_action = la_esc(obj->action_description);

	if (esc_name)
		snprintf(name_str, sizeof(name_str), "'%s'", esc_name);
	else
		strcpy(name_str, "NULL");
	if (esc_short)
		snprintf(short_str, sizeof(short_str), "'%s'", esc_short);
	else
		strcpy(short_str, "NULL");
	if (esc_desc)
		snprintf(desc_str, sizeof(desc_str), "'%s'", esc_desc);
	else
		strcpy(desc_str, "NULL");
	if (esc_action)
		snprintf(action_str, sizeof(action_str), "'%s'", esc_action);
	else
		strcpy(action_str, "NULL");

	snprintf(wear_str, sizeof(wear_str), "%u", obj->wear_flags);
	snprintf(type_str, sizeof(type_str), "%d", (int)obj->type);
	snprintf(material_str, sizeof(material_str), "%d", (int)obj->material);
	snprintf(bv1_str, sizeof(bv1_str), "%lu", (unsigned long)obj->bitvector);
	snprintf(bv2_str, sizeof(bv2_str), "%lu", (unsigned long)obj->bitvector2);
	snprintf(bv3_str, sizeof(bv3_str), "%lu", (unsigned long)obj->bitvector3);
	snprintf(bv4_str, sizeof(bv4_str), "%lu", (unsigned long)obj->bitvector4);
	snprintf(bv5_str, sizeof(bv5_str), "%lu", (unsigned long)obj->bitvector5);

	if (parent_tmp > 0)
		snprintf(container_str, sizeof(container_str), "@la_i%d", parent_tmp);
	else
		strcpy(container_str, "NULL");

	if (chest_id > 0)
		snprintf(chest_id_str, sizeof(chest_id_str), "%d", chest_id);
	else
		strcpy(chest_id_str, "NULL");

	if (!la_buf_printf(b,
	         "INSERT INTO locker_items ("
	         "locker_id, chest_id, vnum, container_id, quantity, "
	         "weight, cost, timer, extra_flags, wear_flags, item_type, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, "
	         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
	         "item_material, obj_uid, item_condition"
	         ") VALUES ("
	         "%d, %s, %d, %s, 1, "
	         "%d, %d, %ld, %lu, %s, %s, "
	         "%d, %d, %d, %d, %d, %d, %d, %d, "
	         "%s, %s, %s, %s, "
	         "%s, %s, %s, %s, %s, "
	         "%s, %lu, %d"
	         ");\n"
	         "SET @la_i%d = LAST_INSERT_ID();\n",
	         locker_id,
	         chest_id_str,
	         vnum,
	         container_str,
	         obj->weight,
	         obj->cost,
	         (long)obj->timer[0],
	         (unsigned long)obj->extra_flags,
	         wear_str,
	         type_str,
	         obj->value[0], obj->value[1], obj->value[2], obj->value[3],
	         obj->value[4], obj->value[5], obj->value[6], obj->value[7],
	         name_str, short_str, desc_str, action_str,
	         bv1_str, bv2_str, bv3_str, bv4_str, bv5_str,
	         material_str,
	         obj->obj_uid,
	         obj->condition,
	         tmp))
	{
		free(esc_name); free(esc_short); free(esc_desc); free(esc_action);
		return 0;
	}

	free(esc_name); free(esc_short); free(esc_desc); free(esc_action);

	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		int j;
		int is_dup = 0;
		if (obj->affected[i].location == 0 && obj->affected[i].modifier == 0)
			continue;
		for (j = 0; j < i; j++)
		{
			if (obj->affected[j].location == obj->affected[i].location &&
			    obj->affected[j].modifier == obj->affected[i].modifier)
			{
				is_dup = 1;
				break;
			}
		}
		if (is_dup)
			continue;
		if (!la_buf_printf(b,
		         "INSERT INTO locker_item_affects (item_id, location, modifier) "
		         "VALUES (@la_i%d, %d, %d);\n",
		         tmp, obj->affected[i].location, obj->affected[i].modifier))
			return 0;
	}

	for (ed = obj->ex_description; ed; ed = ed->next)
	{
		char *ek = la_esc(ed->keyword ? ed->keyword : "");
		char *edesc = la_esc(ed->description ? ed->description : "");
		if (!ek || !edesc)
		{
			free(ek); free(edesc);
			return 0;
		}
		if (!la_buf_printf(b,
		         "INSERT INTO locker_item_extra_descr (item_id, keyword, description) "
		         "VALUES (@la_i%d, '%s', '%s');\n",
		         tmp, ek, edesc))
		{
			free(ek); free(edesc);
			return 0;
		}
		free(ek); free(edesc);
	}

	for (child = obj->contains; child; child = child->next_content)
	{
		if (!emit_item_sql(b, child, locker_id, chest_id, tmp))
			return 0;
	}
	return 1;
}

static void locker_owner_ids(P_char chLocker, int *owner_pid, int *owner_assoc_id)
{
	*owner_pid = 0;
	*owner_assoc_id = 0;
	if (!chLocker || !GET_NAME(chLocker))
		return;
	if (strncmp(GET_NAME(chLocker), "guild.", 6) == 0)
	{
		*owner_assoc_id = atoi(GET_NAME(chLocker) + 6);
	}
	else if (strncmp(GET_NAME(chLocker), "account.", 8) == 0)
	{
		*owner_pid = 0;
	}
	else
	{
		char pname[MAX_NAME_LENGTH + 1];
		char *dot;
		strlcpy(pname, GET_NAME(chLocker), sizeof(pname));
		dot = strstr(pname, ".locker");
		if (dot)
			*dot = '\0';
		*owner_pid = sql_get_player_pid(pname);
	}
}

/* Ensure the locker row + public chest exist; return locker_id and public_chest_id.
 * Only metadata — no item walk. Returns 0 on failure. */
static int ensure_locker_ids(P_char chLocker, int owner_pid, int owner_assoc_id,
                             int *out_locker_id, int *out_public_id)
{
	int locker_id;
	int public_id;
	char *esc_name;
	char query[768];
	char owner_pid_str[32], owner_assoc_str[32];

	if (!chLocker || !GET_NAME(chLocker) || !out_locker_id || !out_public_id)
		return 0;

	locker_id = sql_get_locker_id_by_name(GET_NAME(chLocker));
	if (locker_id <= 0)
	{
		esc_name = la_esc(GET_NAME(chLocker));
		if (!esc_name)
			return 0;
		if (owner_pid > 0)
			snprintf(owner_pid_str, sizeof(owner_pid_str), "%d", owner_pid);
		else
			strcpy(owner_pid_str, "NULL");
		if (owner_assoc_id > 0)
			snprintf(owner_assoc_str, sizeof(owner_assoc_str), "%d", owner_assoc_id);
		else
			strcpy(owner_assoc_str, "NULL");
		snprintf(query, sizeof(query),
		         "INSERT INTO lockers (locker_name, owner_pid, owner_assoc_id, racewar, race) "
		         "VALUES ('%s', %s, %s, %d, %d)",
		         esc_name, owner_pid_str, owner_assoc_str,
		         GET_RACEWAR(chLocker), GET_RACE(chLocker));
		free(esc_name);
		if (!qry("%s", query))
			return 0;
		locker_id = sql_get_locker_id_by_name(GET_NAME(chLocker));
		if (locker_id <= 0)
			return 0;
	}

	public_id = sql_get_or_create_public_chest(locker_id);
	if (public_id <= 0)
		return 0;

	*out_locker_id = locker_id;
	*out_public_id = public_id;
	return 1;
}

/* Build sealed multi-statement SQL for chLocker->carrying public inventory. */
static char *build_locker_snapshot_sql(struct locker_async_slot *s)
{
	struct la_buf b;
	P_char chLocker = s->chLocker;
	int owner_pid = s->owner_pid;
	int owner_assoc_id = s->owner_assoc_id;
	int locker_id = 0;
	int public_id = 0;
	P_obj obj;

	if (!chLocker || !GET_NAME(chLocker))
		return NULL;

	if (!owner_pid && !owner_assoc_id)
		locker_owner_ids(chLocker, &owner_pid, &owner_assoc_id);
	s->owner_pid = owner_pid;
	s->owner_assoc_id = owner_assoc_id;

	/* Resolve ids on main (cheap). */
	if (!ensure_locker_ids(chLocker, owner_pid, owner_assoc_id, &locker_id, &public_id))
		return NULL;
	s->locker_id = locker_id;

	if (!la_buf_init(&b, LOCKER_ASYNC_SCRIPT_INIT))
		return NULL;

	g_tmp_id_seq = 0;

	if (!la_buf_printf(&b,
	         "START TRANSACTION;\n"
	         "DELETE FROM locker_items WHERE locker_id=%d AND (chest_id IS NULL OR chest_id=%d);\n",
	         locker_id, public_id))
		goto fail;

	for (obj = chLocker->carrying; obj; obj = obj->next_content)
	{
		if (!emit_item_sql(&b, obj, locker_id, public_id, 0))
			goto fail;
	}

	if (!la_buf_puts(&b, "COMMIT;\n"))
		goto fail;

	if (b.failed)
		goto fail;

	{
		char *out = b.data;
		b.data = NULL;
		la_buf_free(&b);
		return out;
	}

fail:
	la_buf_free(&b);
	return NULL;
}

/* ---------------- worker ---------------- */

static int apply_sql_script(MYSQL *conn, const char *sql)
{
	const char *p;
	const char *q;
	char *stmt;
	size_t n;
	int status;

	if (!conn || !sql)
		return 0;

	p = sql;
	while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
		p++;
	if (!*p)
		return 1;
	if (p[0] == '/' && p[1] == '*')
		return 1;

	/* Prefer multi-statement when connection supports it. */
	if (mysql_real_query(conn, sql, (unsigned long)strlen(sql)) == 0)
	{
		do
		{
			MYSQL_RES *res = mysql_store_result(conn);
			if (res)
				mysql_free_result(res);
			status = mysql_next_result(conn);
			if (status > 0)
			{
				logit(LOG_FILE, "locker_async: multi result error: %s", mysql_error(conn));
				return 0;
			}
		} while (status == 0);
		return 1;
	}

	/* Fallback: split on ";\n". */
	p = sql;
	while (*p)
	{
		while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
			p++;
		if (!*p)
			break;
		q = strstr(p, ";\n");
		if (!q)
			q = p + strlen(p);
		n = (size_t)(q - p);
		while (n > 0 && (p[n - 1] == ' ' || p[n - 1] == '\n' || p[n - 1] == '\r' ||
		                 p[n - 1] == '\t' || p[n - 1] == ';'))
			n--;
		if (n > 0)
		{
			stmt = (char *)malloc(n + 1);
			if (!stmt)
				return 0;
			memcpy(stmt, p, n);
			stmt[n] = '\0';
			if (mysql_real_query(conn, stmt, (unsigned long)n) != 0)
			{
				logit(LOG_FILE, "locker_async: stmt failed: %s | sqlerr=%s", stmt, mysql_error(conn));
				free(stmt);
				return 0;
			}
			{
				MYSQL_RES *res = mysql_store_result(conn);
				if (res)
					mysql_free_result(res);
			}
			free(stmt);
		}
		if (!*q)
			break;
		p = q + 2;
	}
	return 1;
}

static void *locker_async_worker_main(void *arg)
{
	(void)arg;
#ifndef __NO_MYSQL__
	if (mysql_thread_init() != 0)
	{
		logit(LOG_FILE, "locker_async: mysql_thread_init failed");
		pthread_mutex_lock(&g_q_mu);
		g_worker_running = 0;
		pthread_mutex_unlock(&g_q_mu);
		return NULL;
	}
#endif

	for (;;)
	{
		struct locker_async_job job;
		struct locker_async_result res;
		MYSQL *conn = NULL;

		memset(&job, 0, sizeof(job));
		pthread_mutex_lock(&g_q_mu);
		while (!g_worker_stop && !job_pop(&job))
			pthread_cond_wait(&g_q_cv, &g_q_mu);
		if (g_worker_stop && !job.used)
		{
			pthread_mutex_unlock(&g_q_mu);
			break;
		}
		pthread_mutex_unlock(&g_q_mu);

		memset(&res, 0, sizeof(res));
		snprintf(res.locker_name, sizeof(res.locker_name), "%s", job.locker_name);
		res.gen = job.gen;
		res.terminal = job.terminal;
		res.user_pid = job.user_pid;
		res.ok = 0;

#ifndef __NO_MYSQL__
		conn = sql_persistence_connection();
		if (conn && job.sql)
			res.ok = apply_sql_script(conn, job.sql) ? 1 : 0;
		else if (job.sql && job.sql[0] == '/' && job.sql[1] == '*')
			res.ok = 1;
		if (conn)
			sql_persistence_release_connection(conn);
#else
		res.ok = 0;
#endif

		if (job.sql)
			free(job.sql);

		pthread_mutex_lock(&g_q_mu);
		result_push_locked(&res);
		pthread_mutex_unlock(&g_q_mu);
	}

#ifndef __NO_MYSQL__
	mysql_thread_end();
#endif
	pthread_mutex_lock(&g_q_mu);
	g_worker_running = 0;
	pthread_mutex_unlock(&g_q_mu);
	return NULL;
}

/* ---------------- mark dirty / pulse / completion ---------------- */

int locker_async_mark_dirty(P_char chLocker, P_char chUser, int terminal, const char *reason)
{
	struct locker_async_slot *s;
	const char *name;

	if (!g_inited || !locker_async_worker_available())
		return 0;
	if (!chLocker || !GET_NAME(chLocker))
		return 0;
	name = GET_NAME(chLocker);

	s = slot_alloc(name);
	if (!s)
	{
		persistence_alert(AVATAR, "locker_async", name, "none", "none",
		                  "slots_full",
		                  "dirty table full; reason=%s",
		                  reason ? reason : "unknown");
		return 0;
	}

	s->chLocker = chLocker;
	if (chUser)
		s->chUser = chUser;
	if (terminal)
		s->terminal = 1;
	if (chUser && !IS_NPC(chUser))
		s->user_pid = GET_PID(chUser);

	if (s->state == LCHK_INFLIGHT)
	{
		/* Coalesce: ask for another pass after current gen finishes. */
		s->rebuild_objects = 1;
		if (terminal)
			s->terminal = 1;
	}
	else
	{
		s->state = LCHK_DIRTY;
		s->gen = g_gen_seq++;
		s->dirty_at = time(NULL);
	}

	logit(LOG_DEBUG,
	      "locker_async: mark dirty name=%s terminal=%d gen=%lu reason=%s state=%d",
	      name, terminal ? 1 : 0, s->gen, reason ? reason : "?", s->state);
	return 1;
}

static P_char find_char_by_pid(int pid)
{
	P_char ch;
	if (pid <= 0)
		return NULL;
	for (ch = character_list; ch; ch = ch->next)
		if (IS_PC(ch) && GET_PID(ch) == pid)
			return ch;
	return NULL;
}

static P_char find_locker_char_by_name(const char *name)
{
	P_char ch;
	P_char found = NULL;
	int matches = 0;

	if (!name)
		return NULL;
	for (ch = character_list; ch; ch = ch->next)
	{
		if (!GET_NAME(ch) || str_cmp(GET_NAME(ch), name))
			continue;
		/* Prefer dedicated locker temporary chars (no real PID in play). */
		matches++;
		if (!found)
			found = ch;
		/* Prefer non-descriptor temporary locker avatars if present. */
		if (!ch->desc && (!found || found->desc))
			found = ch;
	}
	if (matches > 1)
		logit(LOG_DEBUG,
		      "locker_async: name lookup ambiguous name=%s matches=%d; using preferential temp/locker char",
		      name, matches);
	return found;
}

static void apply_result(struct locker_async_result *r)
{
	struct locker_async_slot *s = slot_find(r->locker_name);
	P_char chLocker;
	P_char chUser;
	int durable_ok;

	if (!s)
	{
		logit(LOG_DEBUG, "locker_async: result for unknown slot %s gen=%lu ok=%d",
		      r->locker_name, r->gen, r->ok);
		g_inflight = (g_inflight > 0) ? g_inflight - 1 : 0;
		return;
	}

	if (r->gen != s->gen)
	{
		logit(LOG_DEBUG, "locker_async: stale result name=%s res_gen=%lu slot_gen=%lu",
		      s->locker_name, r->gen, s->gen);
		g_inflight = (g_inflight > 0) ? g_inflight - 1 : 0;
		if (s->rebuild_objects)
			s->state = LCHK_DIRTY;
		return;
	}

	g_inflight = (g_inflight > 0) ? g_inflight - 1 : 0;
	chLocker = find_locker_char_by_name(s->locker_name);
	chUser = find_char_by_pid(s->user_pid);
	durable_ok = r->ok ? 1 : 0;

	if (!r->ok)
	{
		persistence_alert(AVATAR, "locker_async", s->locker_name, "none", "none",
		                  "worker_failed",
		                  "async locker save failed gen=%lu terminal=%d; sync fallback",
		                  s->gen, s->terminal ? 1 : 0);
		if (chLocker)
		{
			int owner_pid = s->owner_pid, owner_assoc = s->owner_assoc_id;
			if (!owner_pid && !owner_assoc)
				locker_owner_ids(chLocker, &owner_pid, &owner_assoc);
			if (sql_save_locker(chLocker, owner_pid, owner_assoc))
				durable_ok = 1;
			else if (writeCharacter(chLocker, s->terminal ? 3 : 0, NOWHERE))
				durable_ok = 1;
			else
				durable_ok = 0;
		}
		else
		{
			durable_ok = 0;
		}
	}

	if (s->terminal)
	{
		if (durable_ok && chLocker)
		{
			chLocker->specials.timer = 0;
			extract_char(chLocker);
			s->chLocker = NULL;
		}
		else if (!durable_ok)
		{
			/* Fail closed: keep locker char / re-entry fence until staff
			 * can recover; never extract after both async + sync failed. */
			persistence_alert(AVATAR, "locker_async", s->locker_name, "none", "none",
			                  "terminal_not_durable",
			                  "terminal save failed; refusing extract of locker char");
			s->rebuild_objects = 1;
		}
	}
	else if (!s->terminal && chLocker && chUser)
	{
		locker_async_request_resort(chLocker, chUser);
	}

	if (s->rebuild_objects)
	{
		s->rebuild_objects = 0;
		s->state = LCHK_DIRTY;
		s->gen = g_gen_seq++;
		s->dirty_at = time(NULL);
		if (chLocker)
			s->chLocker = chLocker;
		if (chUser)
			s->chUser = chUser;
	}
	else if (durable_ok || !s->terminal)
	{
		slot_clear(s);
	}
	else
	{
		/* Keep terminal-not-durable slot as DIRTY/busy fence. */
		s->state = LCHK_DIRTY;
		s->gen = g_gen_seq++;
		s->dirty_at = time(NULL);
	}
}

static void drain_results(void)
{
	for (;;)
	{
		struct locker_async_result r;
		int found = 0;
		int i;

		memset(&r, 0, sizeof(r));
		pthread_mutex_lock(&g_q_mu);
		for (i = 0; i < LOCKER_ASYNC_RESULTS; i++)
		{
			if (g_results[i].used)
			{
				r = g_results[i];
				memset(&g_results[i], 0, sizeof(g_results[i]));
				found = 1;
				break;
			}
		}
		pthread_mutex_unlock(&g_q_mu);
		if (!found)
			break;
		apply_result(&r);
	}
}

static int start_one_snapshot(struct locker_async_slot *s)
{
	struct locker_async_job job;
	char *sql;
	P_char chLocker;
	P_char chUser;

	if (!s || s->state != LCHK_DIRTY)
		return 0;
	if (g_inflight >= LOCKER_ASYNC_MAX_INFLIGHT)
		return 0;
	if (g_snapshots_started_this_pulse >= LOCKER_ASYNC_SNAPSHOTS_PER_PULSE)
		return 0;

	chLocker = s->chLocker ? s->chLocker : find_locker_char_by_name(s->locker_name);
	chUser = s->chUser ? s->chUser : find_char_by_pid(s->user_pid);
	if (!chLocker)
	{
		logit(LOG_FILE, "locker_async: dirty locker char missing for %s — clearing", s->locker_name);
		slot_clear(s);
		return 0;
	}
	s->chLocker = chLocker;
	s->chUser = chUser;

	/* Always re-prepare live inventory onto the locker char before sealing.
	 * Terminal leave already did LockerToPFile; a second call is mostly no-op.
	 * Non-terminal rebuild gens MUST re-walk after a prior restore. */
	if (chUser && chUser->in_room != NOWHERE && IS_ROOM(chUser->in_room, ROOM_LOCKER))
	{
		if (!locker_async_prepare_snapshot(chUser))
		{
			persistence_alert(AVATAR, "locker_async", s->locker_name, "none", "none",
			                  "prepare_failed",
			                  "LockerToPFile failed before snapshot; aborting gen");
			slot_clear(s);
			return 0;
		}
	}

	/* Player is obj-locked via user_pid while DIRTY. Build snapshot now. */
	sql = build_locker_snapshot_sql(s);
	if (!sql)
	{
		persistence_alert(AVATAR, "locker_async", s->locker_name, "none", "none",
		                  "snapshot_failed",
		                  "could not build snapshot; falling back to sync sql_save_locker");
		{
			int owner_pid = s->owner_pid, owner_assoc = s->owner_assoc_id;
			if (!owner_pid && !owner_assoc)
				locker_owner_ids(chLocker, &owner_pid, &owner_assoc);
			if (!sql_save_locker(chLocker, owner_pid, owner_assoc))
				writeCharacter(chLocker, s->terminal ? 3 : 0, NOWHERE);
			if (s->terminal)
			{
				chLocker->specials.timer = 0;
				extract_char(chLocker);
			}
			slot_clear(s);
		}
		return 0;
	}

	memset(&job, 0, sizeof(job));
	snprintf(job.locker_name, sizeof(job.locker_name), "%s", s->locker_name);
	job.gen = s->gen;
	job.terminal = s->terminal;
	job.user_pid = s->user_pid;
	job.sql = sql;

	if (!job_push(&job))
	{
		free(sql);
		persistence_alert(AVATAR, "locker_async", s->locker_name, "none", "none",
		                  "job_queue_full",
		                  "falling back to sync save");
		{
			int owner_pid = s->owner_pid, owner_assoc = s->owner_assoc_id;
			if (!owner_pid && !owner_assoc)
				locker_owner_ids(chLocker, &owner_pid, &owner_assoc);
			sql_save_locker(chLocker, owner_pid, owner_assoc);
			if (s->terminal)
			{
				chLocker->specials.timer = 0;
				extract_char(chLocker);
			}
			slot_clear(s);
		}
		return 0;
	}

	s->state = LCHK_INFLIGHT;
	g_inflight++;
	g_snapshots_started_this_pulse++;

	/* Non-terminal mid-stay: restore chests/floor from the sealed snapshot
	 * copy (items still on chLocker). Main-thread walk is done; unlock
	 * happens because state is no longer DIRTY. */
	if (!s->terminal && chUser)
		locker_async_restore_snapshot_view(chUser);

	logit(LOG_DEBUG, "locker_async: enqueued name=%s gen=%lu terminal=%d inflight=%d",
	      s->locker_name, s->gen, s->terminal ? 1 : 0, g_inflight);
	return 1;
}

void locker_async_pulse(void)
{
	int i;
	struct locker_async_slot *oldest_terminal = NULL;
	struct locker_async_slot *oldest_any = NULL;

	if (!g_inited)
		return;

	g_snapshots_started_this_pulse = 0;
	drain_results();

	for (i = 0; i < LOCKER_ASYNC_SLOTS; i++)
	{
		struct locker_async_slot *s = &g_slots[i];
		if (s->state != LCHK_DIRTY)
			continue;
		if (s->terminal)
		{
			if (!oldest_terminal || s->dirty_at < oldest_terminal->dirty_at)
				oldest_terminal = s;
		}
		if (!oldest_any || s->dirty_at < oldest_any->dirty_at)
			oldest_any = s;
	}

	if (oldest_terminal)
		start_one_snapshot(oldest_terminal);
	else if (oldest_any)
		start_one_snapshot(oldest_any);
}

int locker_async_drain(int wait_ms)
{
	int spins = 0;
	int max_spins = (wait_ms > 0) ? (wait_ms / 10) + 1 : 1;

	if (!g_inited)
		return 1;

	while (spins++ < max_spins)
	{
		int pending = 0;
		int i;

		/* While draining, allow one start per loop without breathing budget. */
		g_snapshots_started_this_pulse = 0;
		for (i = 0; i < LOCKER_ASYNC_SLOTS; i++)
		{
			if (g_slots[i].state == LCHK_DIRTY && g_inflight < LOCKER_ASYNC_MAX_INFLIGHT)
			{
				g_snapshots_started_this_pulse = 0;
				start_one_snapshot(&g_slots[i]);
			}
		}
		drain_results();
		for (i = 0; i < LOCKER_ASYNC_SLOTS; i++)
			if (g_slots[i].state != LCHK_FREE)
				pending = 1;
		if (!pending)
			return 1;
		usleep(10000);
	}
	return 0;
}

void locker_async_init(void)
{
	int err;
	if (g_inited)
		return;
	memset(g_slots, 0, sizeof(g_slots));
	memset(g_jobs, 0, sizeof(g_jobs));
	memset(g_results, 0, sizeof(g_results));
	pthread_mutex_lock(&g_q_mu);
	g_worker_stop = 0;
	g_worker_running = 1;
	g_worker_created = 0;
	pthread_mutex_unlock(&g_q_mu);
	err = pthread_create(&g_worker_tid, NULL, locker_async_worker_main, NULL);
	if (err != 0)
	{
		pthread_mutex_lock(&g_q_mu);
		g_worker_running = 0;
		pthread_mutex_unlock(&g_q_mu);
		logit(LOG_FILE, "locker_async: pthread_create failed: %d", err);
		persistence_alert(AVATAR, "locker_async", "worker", "none", "none",
		                  "start_failed", "locker async worker could not start");
		return;
	}
	pthread_mutex_lock(&g_q_mu);
	/* Creation succeeded, so join ownership persists even if the worker
	 * exits and clears its separate running/health flag. */
	g_worker_created = 1;
	pthread_mutex_unlock(&g_q_mu);
	g_inited = 1;
	logit(LOG_STATUS, "Started locker async persistence worker.");
}

void locker_async_shutdown(void)
{
	int join_created;

	pthread_mutex_lock(&g_q_mu);
	join_created = g_worker_created;
	pthread_mutex_unlock(&g_q_mu);
	if (!g_inited && !join_created)
		return;

	if (locker_async_worker_available())
		locker_async_drain(2000);

	pthread_mutex_lock(&g_q_mu);
	g_worker_stop = 1;
	pthread_cond_broadcast(&g_q_cv);
	pthread_mutex_unlock(&g_q_mu);

	if (join_created)
		pthread_join(g_worker_tid, NULL);

	pthread_mutex_lock(&g_q_mu);
	g_worker_created = 0;
	g_worker_running = 0;
	pthread_mutex_unlock(&g_q_mu);
	g_inited = 0;
	logit(LOG_STATUS, "Stopped locker async persistence worker.");
}
