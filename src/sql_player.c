// sql_player.c
// player save/load functions for mysql storage
// part of pfile-to-db migration

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "sql_player.h"
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "account.h"
#include "assocs.h"
#include "files.h"
#include "mm.h"
#include "necromancy.h"
#include "ships/ships.h"
#include "redis.h"
#include "siege.h"
#include "spells.h"
#include "sql.h"
#include "player_name.h"

// external tables
extern P_index            obj_index;
extern struct index_data *mob_index;
extern int                top_of_world;
extern struct room_data  *world;
extern P_char             character_list;
extern struct mm_ds      *dead_mob_pool;
extern struct mm_ds      *dead_pconly_pool;
extern struct mm_ds      *dead_obj_pool;
extern P_obj              object_list;
extern unsigned long      next_obj_uid;
extern P_Guild            guild_list;
extern Skill               skills[];
void                      ensure_pconly_pool(void);

static void trace_append_file(const char *fmt, ...)
{
	FILE *fp = fopen("/tmp/garp-item-trace.log", "a");
	if (!fp)
		return;
	va_list ap;
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	fclose(fp);
}

#ifdef __NO_MYSQL__

// stubs when mysql is disabled

bool sql_begin_transaction(void) { return false; }
bool sql_commit(void) { return false; }
bool sql_rollback(void) { return false; }
bool sql_in_transaction(void) { return false; }

bool sql_save_player(P_char ch, int type, int room) { return false; }
bool sql_save_player_status(P_char ch, int type, int room) { return false; }
bool sql_save_player_skills(P_char ch) { return false; }
bool sql_save_player_affects(P_char ch) { return false; }
bool sql_save_player_items(P_char ch) { return false; }
bool sql_delete_player_items(int pid) { return false; }
bool sql_save_player_witnesses(P_char ch) { return false; }
bool sql_save_player_shapechanges(P_char ch) { return false; }
bool sql_save_player_recipes(P_char ch) { return false; }
bool sql_add_player_recipe(int pid, int recipe_vnum) { return false; }
bool sql_delete_player_recipes(int pid) { return false; }
bool sql_has_player_recipe(int pid, int recipe_vnum) { return false; }
int *sql_get_player_recipes(int pid, int *count)
{
	if (count)
		*count = 0;
	return NULL;
}

P_char sql_load_player(const char *name) { return NULL; }
bool   sql_player_exists(const char *name) { return false; }
int    sql_get_player_pid(const char *name) { return -1; }
bool   sql_load_player_status(P_char ch, int pid) { return false; }
bool   sql_load_player_skills(P_char ch) { return false; }
bool   sql_load_player_affects(P_char ch) { return false; }
bool   sql_load_player_items(P_char ch) { return false; }
bool   sql_load_player_witnesses(P_char ch) { return false; }
bool   sql_load_player_shapechanges(P_char ch) { return false; }
bool   sql_save_player_pets(P_char ch, int save_type) { return false; }
bool   sql_load_player_pets(P_char ch) { return false; }

bool sql_delete_player(int pid) { return false; }
bool sql_delete_player_by_name(const char *name) { return false; }

bool               sql_save_account(struct acct_entry *acc) { return false; }
struct acct_entry *sql_load_account(const char *name) { return NULL; }
bool               sql_account_exists(const char *name) { return false; }
bool               sql_link_player_to_account(const char *account_name, int pid) { return false; }

bool   sql_save_locker(P_char locker_ch, int owner_pid, int owner_assoc_id) { return false; }
P_char sql_load_locker(int owner_pid, int owner_assoc_id) { return NULL; }
P_char sql_load_locker_by_name(const char *locker_name) { return NULL; }
bool   sql_locker_exists(int owner_pid, int owner_assoc_id) { return false; }
bool   sql_locker_exists_by_name(const char *locker_name) { return false; }
bool   sql_delete_locker(int owner_pid, int owner_assoc_id) { return false; }
bool   sql_delete_locker_by_name(const char *locker_name) { return false; }

bool sql_migrate_player(const char *name) { return false; }
bool sql_verify_player(const char *name) { return false; }
int  sql_migrate_all_players(void) { return 0; }

char *sql_escape_string(const char *str) { return NULL; }
void  sql_player_error(const char *context, const char *query) {}

bool sql_save_corpse(P_obj corpse) { return false; }
bool sql_delete_corpse(const char *player_name, int save_id) { return false; }
bool sql_load_all_corpses(void) { return false; }

bool   sql_save_shopkeeper(P_char ch, int shop_nr) { return false; }
bool   sql_delete_shopkeeper(int shop_nr) { return false; }
P_char sql_restore_shopkeeper(int shop_nr) { return NULL; }
void   sql_restore_shopkeepers(void) {}

bool sql_save_saved_item(P_obj item, const char *item_key) { return false; }
bool sql_delete_saved_item(const char *item_key) { return false; }
void sql_restore_saved_items(void) {}

bool sql_save_siege_item(P_obj obj, int room_vnum) { return false; }
bool sql_save_siege_list(void) { return false; }
bool sql_delete_siege_items(int room_vnum) { return false; }
void sql_load_siege_list(void) {}

bool            sql_save_towns(void) { return false; }
bool            sql_load_towns(void) { return false; }
bool            sql_save_account_ips(const char *account_name, struct acct_ip *ips) { return false; }
struct acct_ip *sql_load_account_ips(const char *account_name) { return NULL; }
bool            sql_delete_account_ips(const char *account_name) { return false; }
bool            sql_save_kingdom_land(void) { return false; }

bool   sql_save_ship(P_ship ship) { return false; }
P_ship sql_load_ship(const char *owner_name) { return NULL; }
bool   sql_load_all_ships(void) { return false; }
bool   sql_delete_ship(const char *owner_name) { return false; }

bool   sql_save_guild(Guild *guild) { return false; }
Guild *sql_load_guild(unsigned int guild_id) { return NULL; }
bool   sql_load_all_guilds(void) { return false; }
bool   sql_delete_guild(unsigned int guild_id) { return false; }

bool      sql_load_account_bank(const char *account_name, int racewar, P_char ch) { return false; }
bool      sql_save_account_bank(const char *account_name, int racewar, P_char ch) { return false; }
long long sql_account_bank_deposit(const char *account_name, int racewar, int coin_type, int amount) { return -1; }
long long sql_account_bank_withdraw(const char *account_name, int racewar, int coin_type, int amount) { return -1; }
bool      sql_ensure_account_bank(const char *account_name, int racewar) { return false; }

#else

// globals

extern MYSQL *DB;

static int sql_count_obj_contents(P_obj obj);
static int sql_save_locker_item(int locker_id, int chest_id, P_obj obj, int container_id);
static bool sql_save_locker_item_children(int locker_id, int chest_id, P_obj obj, int item_id, bool own_txn);

// track transaction state
static bool in_transaction = false;
static P_char pending_account_cache_char = NULL;
static int    pending_account_cache_room = NOWHERE;
static bool   pending_account_cache_sync = false;

static void sql_sync_account_character_cache(P_char ch, int room);
static void sql_queue_account_character_cache_sync(P_char ch, int room);
static void sql_clear_account_character_cache_sync(void);

// transaction helpers

bool sql_begin_transaction(void)
{
	if (!DB)
	{
		logit(LOG_DEBUG, "sql_begin_transaction: db not initialized");
		return false;
	}

	if (in_transaction)
	{
		logit(LOG_DEBUG, "sql_begin_transaction: already in transaction");
		return false;
	}

	sql_clear_results();
	if (!sql_trace_exec("sql_begin_transaction", "START TRANSACTION", 17, false, false))
	{
		logit(LOG_DEBUG, "sql_begin_transaction: failed: %s", mysql_error(DB));
		return false;
	}

	in_transaction = true;
	return true;
}

bool sql_commit(void)
{
	if (!DB)
	{
		logit(LOG_DEBUG, "sql_commit: db not initialized");
		return false;
	}

	if (!in_transaction)
	{
		logit(LOG_DEBUG, "sql_commit: not in transaction");
		return false;
	}

	if (!sql_trace_exec("sql_commit", "COMMIT", 6, false, false))
	{
		logit(LOG_DEBUG, "sql_commit: failed: %s", mysql_error(DB));
		/* Keep transaction ownership and pending cache state intact so the
		 * caller can attempt an explicit rollback.  A failed COMMIT leaves
		 * server-side durability uncertain; claiming the transaction ended
		 * here would make that recovery path impossible. */
		return false;
	}

	in_transaction = false;
	if (pending_account_cache_sync)
	{
		P_char ch = pending_account_cache_char;
		int    room = pending_account_cache_room;
		sql_clear_account_character_cache_sync();
		sql_sync_account_character_cache(ch, room);
	}
	return true;
}

bool sql_rollback(void)
{
	if (!DB)
	{
		logit(LOG_DEBUG, "sql_rollback: db not initialized");
		return false;
	}

	if (!in_transaction)
	{
		logit(LOG_DEBUG, "sql_rollback: not in transaction");
		return false;
	}

	if (!sql_trace_exec("sql_rollback", "ROLLBACK", 8, false, false))
	{
		logit(LOG_DEBUG, "sql_rollback: failed: %s", mysql_error(DB));
		in_transaction = false;
		sql_clear_account_character_cache_sync();
		return false;
	}

	in_transaction = false;
	sql_clear_account_character_cache_sync();
	return true;
}

bool sql_in_transaction(void) { return in_transaction; }

static void sql_sync_account_character_cache(P_char ch, int room)
{
	if (!ch || !ch->desc || !ch->desc->account)
		return;

	struct acct_chars *c = ch->desc->account->acct_character_list;
	while (c)
	{
		if (c->charname && !strcasecmp(c->charname, GET_NAME(ch)))
		{
			c->last_room = room;
			c->level     = GET_LEVEL(ch);
			c->last_save = time(NULL);
			break;
		}
		c = c->next;
	}
}

static void sql_queue_account_character_cache_sync(P_char ch, int room)
{
	pending_account_cache_char = ch;
	pending_account_cache_room = room;
	pending_account_cache_sync = (ch != NULL);
}

static void sql_clear_account_character_cache_sync(void)
{
	pending_account_cache_char = NULL;
	pending_account_cache_room = NOWHERE;
	pending_account_cache_sync = false;
}

// Helper: safely append a formatted string to a batch buffer.
//
// Replaces the dangerous pattern:
//   if (pos > buf_size - 200) break;
//   pos += snprintf(buf + pos, buf_size - pos, ...);
// which is fragile because snprintf returns the number of chars it
// *would* have written when truncated, so pos can exceed buf_size
// after a truncated snprintf, and the next iteration's pre-check
// relies on a magic 200-byte margin to avoid an OOB write.
//
// This helper does the proper post-check: it inspects the snprintf
// return value and returns -1 (with the buffer null-terminated at
// buf_size-1) on truncation or encoding error, so callers can break
// the loop without ever leaving pos in an unsafe state.
//
// Returns the new position on success, or -1 on truncation/error.
static int batch_append(char *buf, int pos, size_t buf_size, const char *fmt, ...)
{
	if (!buf || pos < 0 || (size_t)pos >= buf_size)
	{
		return -1;
	}

	va_list args;
	va_start(args, fmt);
	int written = vsnprintf(buf + pos, buf_size - pos, fmt, args);
	va_end(args);

	if (written < 0)
	{
		// encoding error
		buf[buf_size - 1] = '\0';
		return -1;
	}

	if ((size_t)written >= buf_size - pos)
	{
		// truncated - null-terminate at end of buffer to keep it usable
		// for diagnostic logging without scribbling past buf_size.
		buf[buf_size - 1] = '\0';
		return -1;
	}

	return pos + written;
}

// Shared helper: for an item being saved, fill the
// wear_str, type_str, and bv1-5_str output buffers with the
// item's wear_flags, type, and bitvectors.  Each buffer must be
// at least 32 bytes (16 for type_str).  The bitvector buffers
// are set to the numeric value if it differs from the prototype,
// or to the literal "NULL" otherwise (load code uses NULL to
// mean "use prototype value").
//
// SIDE EFFECT: the prototype object loaded internally is freed
// (extract_obj) before returning.  The function name makes this
// explicit so callers can't accidentally skip the cleanup.
static int sql_validate_loaded_item_type(P_obj obj, int saved_type, const char *context)
{
	if (!obj)
		return saved_type;

	if (saved_type <= 0)
		return obj->type;

	if (saved_type > ITEM_LAST)
	{
		logit(LOG_DEBUG,
		      "sql item repair: ignoring out-of-range item_type for vnum=%d saved_type=%d proto_type=%d context=%s",
		      OBJ_VNUM(obj), saved_type, obj->type, context ? context : "(null)");
		return obj->type;
	}

	if (saved_type == ITEM_CORPSE && obj->type != ITEM_CORPSE)
	{
		logit(LOG_DEBUG,
		      "sql item repair: ignoring corpse item_type for vnum=%d saved_type=%d proto_type=%d context=%s",
		      OBJ_VNUM(obj), saved_type, obj->type, context ? context : "(null)");
		return obj->type;
	}

	return saved_type;
}

static void sql_format_item_diff_fields_and_free_proto(
	P_obj obj,
	char  *wear_str,
	char  *type_str,
	char  *material_str,
	char  *bv1_str,
	char  *bv2_str,
	char  *bv3_str,
	char  *bv4_str,
	char  *bv5_str)
{
	P_obj proto = read_object(obj->R_num, REAL);

	if (proto)
	{
		if (obj->wear_flags != proto->wear_flags)
			snprintf(wear_str, 32, "%d", obj->wear_flags);
		else
			strcpy(wear_str, "NULL");

		if (obj->type != proto->type)
			snprintf(type_str, 16, "%d", obj->type);
		else
			strcpy(type_str, "NULL");

		if (obj->material != proto->material)
			snprintf(material_str, 16, "%d", obj->material);
		else
			strcpy(material_str, "NULL");

		if (obj->bitvector != proto->bitvector)
			snprintf(bv1_str, 32, "%lu", obj->bitvector);
		else
			strcpy(bv1_str, "NULL");
		if (obj->bitvector2 != proto->bitvector2)
			snprintf(bv2_str, 32, "%lu", obj->bitvector2);
		else
			strcpy(bv2_str, "NULL");
		if (obj->bitvector3 != proto->bitvector3)
			snprintf(bv3_str, 32, "%lu", obj->bitvector3);
		else
			strcpy(bv3_str, "NULL");
		if (obj->bitvector4 != proto->bitvector4)
			snprintf(bv4_str, 32, "%lu", obj->bitvector4);
		else
			strcpy(bv4_str, "NULL");
		if (obj->bitvector5 != proto->bitvector5)
			snprintf(bv5_str, 32, "%lu", obj->bitvector5);
		else
			strcpy(bv5_str, "NULL");
	}
	else
	{
		snprintf(wear_str, 32, "%d", obj->wear_flags);
		snprintf(type_str, 16, "%d", obj->type);
		snprintf(material_str, 16, "%d", obj->material);
		snprintf(bv1_str, 32, "%lu", obj->bitvector);
		snprintf(bv2_str, 32, "%lu", obj->bitvector2);
		snprintf(bv3_str, 32, "%lu", obj->bitvector3);
		snprintf(bv4_str, 32, "%lu", obj->bitvector4);
		snprintf(bv5_str, 32, "%lu", obj->bitvector5);
	}

	if (proto)
		extract_obj(proto);
}


// utility functions

// escape string for sql, caller must free
char *sql_escape_string(const char *str)
{
	if (!str || !DB)
		return NULL;

	size_t len = strlen(str);
	// mysql_real_escape_string needs at most len*2+1 bytes
	char *escaped = (char *)malloc(len * 2 + 1);
	if (!escaped)
		return NULL;

	mysql_real_escape_string(DB, escaped, str, len);
	return escaped;
}

// log sql error with context
void sql_player_error(const char *context, const char *query)
{
	if (!DB)
	{
		logit(LOG_DEBUG, "sql_player: %s: db not initialized", context);
		return;
	}

	logit(LOG_DEBUG, "sql_player: %s: %s", context, mysql_error(DB));
	if (query)
	{
		// log first 200 chars of query for debugging
		//char truncated[201];
		//strncpy(truncated, query, 200);
		//truncated[200] = '\0';
		logit(LOG_DEBUG, "sql_player: query: %s...", query);
	}
}

// helper to run query and free result
static bool sql_run_query(const char *query)
{
	if (!DB || !query)
		return false;

	if (!sql_trace_exec("sql_run_query", query, strlen(query), false, false))
	{
		sql_player_error("sql_run_query", query);
		return false;
	}

	// consume any result set
	MYSQL_RES *result = mysql_store_result(DB);
	if (result)
		mysql_free_result(result);

	return true;
}

static bool sql_delete_player_subtable(int pid, const char *table_name)
{
	if (!DB || pid <= 0 || !table_name || !*table_name)
		return false;

	char query[128];
	snprintf(query, sizeof(query), "DELETE FROM %s WHERE pid=%d", table_name, pid);
	return sql_run_query(query);
}

// converts spellbook binary bits to json array string "[101,203,456]"
static char *spellbook_to_json(const char *bits)
{
	if (!bits)
		return NULL;

	char *buf = (char *)malloc(MAX_SKILLS * 6);
	if (!buf)
		return NULL;

	char *p = buf;
	*p++    = '[';

	int first = 1;
	for (int i = 0; i < MAX_SKILLS; i++)
	{
		if (bits[i / 8] & (1 << (i % 8)))
		{
			if (!first)
				*p++ = ',';
			p += snprintf(p, buf + MAX_SKILLS * 6 - p, "%d", i);
			first = 0;
		}
	}
	*p++ = ']';
	*p   = '\0';

	return buf;
}

// parses "[101,203,456]" and sets bits in output buffer
static void json_to_spellbook(const char *json, char *output)
{
	if (!json || !output)
		return;

	size_t buflen = (MAX_SKILLS + 1) / 8 + 1;
	memset(output, 0, buflen);

	const char *p = json;
	while (*p && *p != '[')
		p++;
	if (*p == '[')
		p++;

	while (*p)
	{
		while (*p && (*p == ' ' || *p == ','))
			p++;
		if (*p == ']' || !*p)
			break;

		int spell_id = atoi(p);
		if (spell_id >= 0 && spell_id < MAX_SKILLS)
			output[spell_id / 8] |= (1 << (spell_id % 8));

		while (*p && *p != ',' && *p != ']')
			p++;
	}
}

// for forked child process - needs its own db connection
MYSQL *sql_create_child_connection(void)
{
	MYSQL *conn = mysql_init(NULL);
	if (!conn)
		return NULL;

	conn = mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWD, sql_persistence_db_name(), DB_PORT, NULL, CLIENT_MULTI_STATEMENTS);
	if (!conn)
		return NULL;

	mysql_set_character_set(conn, "utf8mb4");
	return conn;
}

// child swaps in its own connection after fork
void sql_reset_for_child(MYSQL *child_conn)
{
	DB             = child_conn;
	in_transaction = false;
}

// player existence check

bool sql_player_exists(const char *name)
{
	if (!DB || !name)
		return false;

	char *escaped_name = sql_escape_string(name);
	if (!escaped_name)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "SELECT 1 FROM player_data WHERE LOWER(name)=LOWER('%s') LIMIT 1", escaped_name);
	free(escaped_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row    = mysql_fetch_row(result);
	bool      exists = (row != NULL);
	mysql_free_result(result);

	return exists;
}

bool sql_player_rename(P_char ch, const char *new_name)
{
	if (!DB || !new_name || !ch)
		return false;

	char normalized_name[MAX_STRING_LENGTH];
	strncpy(normalized_name, new_name, sizeof(normalized_name) - 1);
	normalized_name[sizeof(normalized_name) - 1] = '\0';
	normalize_player_name_case(normalized_name);

	char *escaped_name = sql_escape_string(normalized_name);
	if (!escaped_name)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "UPDATE player_data SET name='%s' WHERE pid='%d'", escaped_name, GET_PID(ch));
	free(escaped_name);

	return sql_run_query(query);
}

int sql_get_player_pid(const char *name)
{
	if (!DB || !name)
		return -1;

	char *escaped_name = sql_escape_string(name);
	if (!escaped_name)
		return -1;

	char query[256];
	snprintf(query, sizeof(query), "SELECT pid FROM player_data WHERE LOWER(name)=LOWER('%s') LIMIT 1", escaped_name);
	free(escaped_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return -1;

	MYSQL_ROW row = mysql_fetch_row(result);
	int       pid = -1;
	if (row && row[0])
		pid = atoi(row[0]);
	mysql_free_result(result);

	return pid;
}

static bool sql_try_get_player_pid(const char *name, int *pid_out)
{
	if (!pid_out)
		return false;

	*pid_out = -1;
	if (!DB || !name)
		return false;

	char *escaped_name = sql_escape_string(name);
	if (!escaped_name)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "SELECT pid FROM player_data WHERE LOWER(name)=LOWER('%s') LIMIT 1", escaped_name);
	free(escaped_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
	{
		sql_player_error("sql_try_get_player_pid", query);
		return false;
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	if (row && row[0])
		*pid_out = atoi(row[0]);
	mysql_free_result(result);

	return true;
}

// player delete

bool sql_delete_player(int pid)
{
	if (!DB || pid <= 0)
		return false;

	char query[128];
	snprintf(query, sizeof(query), "DELETE FROM player_data WHERE pid=%d", pid);

	return sql_run_query(query);
}

bool sql_delete_player_by_name(const char *name)
{
	int pid = sql_get_player_pid(name);
	if (pid <= 0)
		return false;
	return sql_delete_player(pid);
}

// master save function

bool sql_save_player(P_char ch, int type, int room)
{
	if (!ch || !IS_PC(ch))
	{
		logit(LOG_DEBUG, "sql_save_player: invalid char or npc");
		return false;
	}

	trace_append_file("sql_save_player begin name=%s pid=%d type=%d room=%d\n",
	                  GET_NAME(ch), GET_PID(ch), type, room);

	if (!DB)
	{
		logit(LOG_DEBUG, "sql_save_player: db not initialized");
		return false;
	}

	// Start own transaction if not already in one (allows parent to wrap)
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
		{
			logit(LOG_DEBUG, "sql_save_player: failed to start transaction");
			return false;
		}
		own_txn = true;
	}

	// save all components
	if (!sql_save_player_status(ch, type, room))
	{
		logit(LOG_DEBUG, "sql_save_player: failed to save status for %s", GET_NAME(ch));
		sql_rollback();
		return false;
	}

	if (!sql_save_player_skills(ch))
	{
		logit(LOG_DEBUG, "sql_save_player: failed to save skills for %s", GET_NAME(ch));
		sql_rollback();
		return false;
	}

	if (!sql_save_player_affects(ch))
	{
		logit(LOG_DEBUG, "sql_save_player: failed to save affects for %s", GET_NAME(ch));
		sql_rollback();
		return false;
	}

	if (!sql_save_player_items(ch))
	{
		logit(LOG_DEBUG, "sql_save_player: failed to save items for %s", GET_NAME(ch));
		sql_rollback();
		return false;
	}

	if (!sql_save_player_pets(ch, type))
	{
		logit(LOG_DEBUG, "sql_save_player: failed to save pets for %s", GET_NAME(ch));
		sql_rollback();
		return false;
	}

	if (!sql_save_player_witnesses(ch))
	{
		logit(LOG_DEBUG, "sql_save_player: failed to save witnesses for %s", GET_NAME(ch));
		sql_rollback();
		return false;
	}

	if (!sql_save_player_shapechanges(ch))
	{
		logit(LOG_DEBUG, "sql_save_player: failed to save shapechanges for %s", GET_NAME(ch));
		sql_rollback();
		return false;
	}

	if (own_txn)
	{
		if (!sql_commit())
		{
			logit(LOG_DEBUG, "sql_save_player: failed to commit for %s", GET_NAME(ch));
			sql_rollback();
			trace_append_file("sql_save_player commit failed name=%s pid=%d type=%d room=%d\n",
			                  GET_NAME(ch), GET_PID(ch), type, room);
			return false;
		}
	}

	clear_player_dirty_container_flags(ch);
	REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_EQUIPMENT);
	REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_INVENTORY);

	trace_append_file("sql_save_player done name=%s pid=%d type=%d room=%d\n",
	                  GET_NAME(ch), GET_PID(ch), type, room);

	return true;
}

// status save (main player data)

bool sql_save_player_status(P_char ch, int type, int room)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	int pid = GET_PID(ch);
	int db_pid = -1;

	if (!sql_try_get_player_pid(GET_NAME(ch), &db_pid))
		return false;

	// if pid is 0 but player exists by name, look up the pid
	if (pid == 0 && db_pid > 0)
	{
		pid = db_pid;
		ch->only.pc->pid = pid;
	}

	bool is_update = (db_pid > 0);

	// for crash saves, preserve the existing last_room (camp/rent location)
	// don't overwrite with crash location so player returns to safe spot.
	// Exception: players currently inside locker rooms rely on the locker pre-save
	// hook (-80) to rewrite the save room to the room outside the locker. If we
	// blindly restore the previous DB last_room here, we strand them back inside
	// the transient locker room on next login.
	if (is_update && (type == RENT_CRASH || type == RENT_CRASH2) && !(ch->in_room != NOWHERE && IS_ROOM(ch->in_room, ROOM_LOCKER)))
	{
		char room_query[256];
		snprintf(room_query, sizeof(room_query), "SELECT last_room FROM player_data WHERE pid=%d", pid);
		MYSQL_RES *room_result = db_query(room_query);
		if (room_result)
		{
			MYSQL_ROW row = mysql_fetch_row(room_result);
			if (row && row[0])
				room = atoi(row[0]);
			mysql_free_result(room_result);
		}
	}


	// build the query
	// this is a big query, we'll use a large buffer
	char  query[16384];
	char *q         = query;
	int   remaining = sizeof(query);
	int   written;

	// escape strings that might contain special chars
	char *esc_name       = sql_escape_string(GET_NAME(ch) ? GET_NAME(ch) : "");
	char *esc_short      = sql_escape_string(ch->player.short_descr ? ch->player.short_descr : "");
	char *esc_long       = sql_escape_string(ch->player.long_descr ? ch->player.long_descr : "");
	char *esc_desc       = sql_escape_string(ch->player.description ? ch->player.description : "");
	char *esc_title      = sql_escape_string(GET_TITLE(ch) ? GET_TITLE(ch) : "");
	char *esc_poofin     = sql_escape_string(ch->only.pc->poofIn ? ch->only.pc->poofIn : "");
	char *esc_poofout    = sql_escape_string(ch->only.pc->poofOut ? ch->only.pc->poofOut : "");
	char *esc_poofinsnd  = sql_escape_string("");
	char *esc_poofoutsnd = sql_escape_string("");

	// Start own transaction only after all preflight lookups and string escaping succeed.
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
		{
			free(esc_name);
			free(esc_short);
			free(esc_long);
			free(esc_desc);
			free(esc_title);
			free(esc_poofin);
			free(esc_poofout);
			free(esc_poofinsnd);
			free(esc_poofoutsnd);
			return false;
		}
		own_txn = true;
	}

	sql_queue_account_character_cache_sync(ch, room);

	if (is_update)
	{
		written = snprintf(q,
		                   remaining,
		                   "UPDATE player_data SET "
		                   "short_descr='%s', long_descr='%s', description='%s', title='%s', "
		                   "m_class=%u, secondary_class=%u, spec=%d, race=%d, racewar=%d, "
		                   "level=%d, sex=%d, weight=%d, height=%d, size=%d, "
		                   "hometown=%d, birthplace=%d, orig_birthplace=%d, last_room=%d, "
		                   "birth_time=FROM_UNIXTIME(NULLIF(%ld,0)), played_time=%d, last_save=FROM_UNIXTIME(NULLIF(%ld,0)), perm_aging=%d,"
		                   "base_str=%d, base_dex=%d, base_agi=%d, base_con=%d, base_pow=%d, "
		                   "base_int=%d, base_wis=%d, base_cha=%d, base_kar=%d, base_luk=%d, "
		                   "mana=%d, base_mana=%d, hit_diff=%d, base_hit=%d, "
		                   "vitality=%d, base_vitality=%d, spells_memmed_extra=%d, "
		                   "copper=%d, silver=%d, gold=%d, platinum=%d, "
		                   "bank_copper=0, bank_silver=0, bank_gold=0, bank_platinum=0,"
		                   "exp=%d, epics=%ld, epic_skill_points=%ld, skillpoints=%d, spell_bind_used=%ld, "
		                   "act=%u, act2=%u, act3=%u, vote=%lu, alignment=%d,"
		                   "prestige=%d, assoc_id=%d, guild_status=%u, "
		                   "time_left_guild=FROM_UNIXTIME(NULLIF(%ld,0)), nb_left_guild=%d, time_unspecced=FROM_UNIXTIME(NULLIF(%ld,0)),"
		                   "frags=%ld, oldfrags=%ld, numb_deaths=%lu, "
		                   "condition_0=%d, condition_1=%d, condition_2=%d, condition_3=%d, condition_4=%d, "
		                   "poof_in='%s', poof_out='%s', poof_in_sound='%s', poof_out_sound='%s', "
		                   "echo_toggle=%d, prompt=%d, wiz_invis=%d, law_flags=%lu, "
		                   "wimpy=%d, aggressive=%d, highest_level=%d, screen_length=%d, "
		                   "quest_active=%d, quest_mob_vnum=%d, quest_type=%d, quest_accomplished=%d, "
		                   "quest_started=%d, quest_zone_number=%d, quest_giver=%d, quest_level=%d, "
		                   "quest_receiver=%d, quest_shares_left=%d, quest_kill_how_many=%d, "
		                   "quest_kill_original=%d, quest_map_room=%d, quest_map_bought=%d, "
		                   "last_ip=%lu "
		                   "WHERE pid=%d",
		                   esc_short,
		                   esc_long,
		                   esc_desc,
		                   esc_title,
		                   ch->player.m_class,
		                   ch->player.secondary_class,
		                   ch->player.spec,
		                   GET_RACE(ch),
		                   GET_RACEWAR(ch),
		                   GET_LEVEL(ch),
		                   GET_SEX(ch),
		                   ch->player.weight,
		                   ch->player.height,
		                   GET_SIZE(ch),
		                   GET_HOME(ch),
		                   GET_BIRTHPLACE(ch),
		                   GET_ORIG_BIRTHPLACE(ch),
		                   room,
		                   ch->player.time.birth,
		                   ch->player.time.played,
		                   (long)time(0),
		                   ch->player.time.perm_aging,
		                   ch->base_stats.Str,
		                   ch->base_stats.Dex,
		                   ch->base_stats.Agi,
		                   ch->base_stats.Con,
		                   ch->base_stats.Pow,
		                   ch->base_stats.Int,
		                   ch->base_stats.Wis,
		                   ch->base_stats.Cha,
		                   ch->base_stats.Kar,
		                   ch->base_stats.Luk,
		                   GET_MANA(ch),
		                   ch->points.base_mana,
		                   MAX(0, GET_MAX_HIT(ch) - GET_HIT(ch)),
		                   ch->points.base_hit,
		                   GET_VITALITY(ch),
		                   ch->points.base_vitality,
		                   ch->only.pc->spells_memmed[MAX_CIRCLE],
		                   GET_COPPER(ch),
		                   GET_SILVER(ch),
		                   GET_GOLD(ch),
		                   GET_PLATINUM(ch),
		                   GET_EXP(ch),
		                   ch->only.pc->epics,
		                   ch->only.pc->epic_skill_points,
		                   ch->only.pc->skillpoints,
		                   ch->only.pc->spell_bind_used,
		                   ch->specials.act,
		                   ch->specials.act2,
		                   ch->specials.act3,
		                   ch->only.pc->vote,
		                   ch->specials.alignment,
		                   ch->only.pc->prestige,
		                   GET_ASSOC_ID(ch),
		                   ch->specials.guild_status,
		                   ch->only.pc->time_left_guild,
		                   ch->only.pc->nb_left_guild,
		                   ch->only.pc->time_unspecced,
		                   ch->only.pc->frags,
		                   ch->only.pc->oldfrags,
		                   ch->only.pc->numb_deaths,
		                   ch->specials.conditions[0],
		                   ch->specials.conditions[1],
		                   ch->specials.conditions[2],
		                   ch->specials.conditions[3],
		                   ch->specials.conditions[4],
		                   esc_poofin,
		                   esc_poofout,
		                   esc_poofinsnd,
		                   esc_poofoutsnd,
		                   ch->only.pc->echo_toggle,
		                   ch->only.pc->prompt,
		                   ch->only.pc->wiz_invis,
		                   0,
		                   ch->only.pc->wimpy,
		                   ch->only.pc->aggressive,
		                   ch->only.pc->highest_level,
		                   ch->only.pc->screen_length,
		                   ch->only.pc->quest_active,
		                   ch->only.pc->quest_mob_vnum,
		                   ch->only.pc->quest_type,
		                   ch->only.pc->quest_accomplished,
		                   ch->only.pc->quest_started,
		                   ch->only.pc->quest_zone_number,
		                   ch->only.pc->quest_giver,
		                   ch->only.pc->quest_level,
		                   ch->only.pc->quest_receiver,
		                   ch->only.pc->quest_shares_left,
		                   ch->only.pc->quest_kill_how_many,
		                   ch->only.pc->quest_kill_original,
		                   ch->only.pc->quest_map_room,
		                   ch->only.pc->quest_map_bought,
		                   ch->only.pc->last_ip,
		                   pid);
	}
	else
	{
		// insert new player
		written = snprintf(q,
		                   remaining,
		                   "INSERT INTO player_data ("
		                   "name, short_descr, long_descr, description, title, "
		                   "m_class, secondary_class, spec, race, racewar, level, sex, "
		                   "weight, height, size, hometown, birthplace, orig_birthplace, last_room, "
		                   "birth_time, played_time, last_save, perm_aging, "
		                   "base_str, base_dex, base_agi, base_con, base_pow, "
		                   "base_int, base_wis, base_cha, base_kar, base_luk, "
		                   "mana, base_mana, hit_diff, base_hit, vitality, base_vitality, spells_memmed_extra, "
		                   "copper, silver, gold, platinum, bank_copper, bank_silver, bank_gold, bank_platinum, "
		                   "exp, epics, epic_skill_points, skillpoints, spell_bind_used, "
		                   "act, act2, act3, vote, alignment,"
		                   "prestige, assoc_id, guild_status, time_left_guild, nb_left_guild, time_unspecced, "
		                   "frags, oldfrags, numb_deaths, "
		                   "condition_0, condition_1, condition_2, condition_3, condition_4, "
		                   "poof_in, poof_out, poof_in_sound, poof_out_sound, "
		                   "echo_toggle, prompt, wiz_invis, law_flags, wimpy, aggressive, highest_level, screen_length, "
		                   "quest_active, quest_mob_vnum, quest_type, quest_accomplished, "
		                   "quest_started, quest_zone_number, quest_giver, quest_level, "
		                   "quest_receiver, quest_shares_left, quest_kill_how_many, "
		                   "quest_kill_original, quest_map_room, quest_map_bought, last_ip"
		                   ") VALUES ("
		                   "'%s', '%s', '%s', '%s', '%s', "
		                   "%u, %u, %d, %d, %d, %d, %d, "
		                   "%d, %d, %d, %d, %d, %d, %d, "
		                   "FROM_UNIXTIME(NULLIF(%ld,0)), %d, FROM_UNIXTIME(NULLIF(%ld,0)), %d, "
		                   "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
		                   "%d, %d, %d, %d, %d, %d, %d, "
		                   "%d, %d, %d, %d, 0, 0, 0, 0, "
		                   "%d, %ld, %ld, %d, %ld, "
		                   "%u, %u, %u, %lu, %d, "
		                   "%d, %d, %u, FROM_UNIXTIME(NULLIF(%ld,0)), %d, FROM_UNIXTIME(NULLIF(%ld,0)), "
		                   "%ld, %ld, %lu, "
		                   "%d, %d, %d, %d, %d, "
		                   "'%s', '%s', '%s', '%s', "
		                   "%d, %d, %d, %lu, %d, %d, %d, %d, "
		                   "%d, %d, %d, %d, "
		                   "%d, %d, %d, %d, "
		                   "%d, %d, %d, "
		                   "%d, %d, %d, %lu"
		                   ")",
		                   esc_name,
		                   esc_short,
		                   esc_long,
		                   esc_desc,
		                   esc_title,
		                   ch->player.m_class,
		                   ch->player.secondary_class,
		                   ch->player.spec,
		                   GET_RACE(ch),
		                   GET_RACEWAR(ch),
		                   GET_LEVEL(ch),
		                   GET_SEX(ch),
		                   ch->player.weight,
		                   ch->player.height,
		                   GET_SIZE(ch),
		                   GET_HOME(ch),
		                   GET_BIRTHPLACE(ch),
		                   GET_ORIG_BIRTHPLACE(ch),
		                   room,
		                   ch->player.time.birth,
		                   ch->player.time.played,
		                   (long)time(0),
		                   ch->player.time.perm_aging,
		                   ch->base_stats.Str,
		                   ch->base_stats.Dex,
		                   ch->base_stats.Agi,
		                   ch->base_stats.Con,
		                   ch->base_stats.Pow,
		                   ch->base_stats.Int,
		                   ch->base_stats.Wis,
		                   ch->base_stats.Cha,
		                   ch->base_stats.Kar,
		                   ch->base_stats.Luk,
		                   GET_MANA(ch),
		                   ch->points.base_mana,
		                   MAX(0, GET_MAX_HIT(ch) - GET_HIT(ch)),
		                   ch->points.base_hit,
		                   GET_VITALITY(ch),
		                   ch->points.base_vitality,
		                   ch->only.pc->spells_memmed[MAX_CIRCLE],
		                   GET_COPPER(ch),
		                   GET_SILVER(ch),
		                   GET_GOLD(ch),
		                   GET_PLATINUM(ch),
		                   GET_EXP(ch),
		                   ch->only.pc->epics,
		                   ch->only.pc->epic_skill_points,
		                   ch->only.pc->skillpoints,
		                   ch->only.pc->spell_bind_used,
		                   ch->specials.act,
		                   ch->specials.act2,
		                   ch->specials.act3,
		                   ch->only.pc->vote,
		                   ch->specials.alignment,
		                   ch->only.pc->prestige,
		                   GET_ASSOC_ID(ch),
		                   ch->specials.guild_status,
		                   ch->only.pc->time_left_guild,
		                   ch->only.pc->nb_left_guild,
		                   ch->only.pc->time_unspecced,
		                   ch->only.pc->frags,
		                   ch->only.pc->oldfrags,
		                   ch->only.pc->numb_deaths,
		                   ch->specials.conditions[0],
		                   ch->specials.conditions[1],
		                   ch->specials.conditions[2],
		                   ch->specials.conditions[3],
		                   ch->specials.conditions[4],
		                   esc_poofin,
		                   esc_poofout,
		                   esc_poofinsnd,
		                   esc_poofoutsnd,
		                   ch->only.pc->echo_toggle,
		                   ch->only.pc->prompt,
		                   ch->only.pc->wiz_invis,
		                   0,
		                   ch->only.pc->wimpy,
		                   ch->only.pc->aggressive,
		                   ch->only.pc->highest_level,
		                   ch->only.pc->screen_length,
		                   ch->only.pc->quest_active,
		                   ch->only.pc->quest_mob_vnum,
		                   ch->only.pc->quest_type,
		                   ch->only.pc->quest_accomplished,
		                   ch->only.pc->quest_started,
		                   ch->only.pc->quest_zone_number,
		                   ch->only.pc->quest_giver,
		                   ch->only.pc->quest_level,
		                   ch->only.pc->quest_receiver,
		                   ch->only.pc->quest_shares_left,
		                   ch->only.pc->quest_kill_how_many,
		                   ch->only.pc->quest_kill_original,
		                   ch->only.pc->quest_map_room,
		                   ch->only.pc->quest_map_bought,
		                   ch->only.pc->last_ip);
	}

	// free escaped strings
	free(esc_name);
	free(esc_short);
	free(esc_long);
	free(esc_desc);
	free(esc_title);
	free(esc_poofin);
	free(esc_poofout);
	free(esc_poofinsnd);
	free(esc_poofoutsnd);

	// run the main query
	if (!sql_run_query(query))
	{
		sql_player_error("sql_save_player_status", query);
		if (own_txn) sql_rollback();
		return false;
	}

	// if insert, get the new pid
	if (!is_update)
	{
		ch->only.pc->pid = (int)mysql_insert_id(DB);
		pid              = ch->only.pc->pid;
	}
	else
	{
		// 0 affected rows is ok - means no values changed (e.g. multiple saves per second)
		// only a real mysql error means failure (which would have been caught by sql_run_query above)
	}

	// batched array saves for performance (was 1200+ individual queries, now ~12)

	// allocate buffer for batch inserts
	char *batch = (char *)malloc(65536);
	if (!batch)
	{
		if (own_txn) sql_rollback();
		return false;
	}

	int  pos;
	bool has_data;

	// languages - batch delete then batch insert
	if (!sql_delete_player_subtable(pid, "player_languages"))
	{
		free(batch);
		if (own_txn) sql_rollback();
		return false;
	}

	pos      = snprintf(batch, 65536, "REPLACE INTO player_languages (pid, tongue_id, proficiency) VALUES ");
	has_data = false;
	for (int i = 0; i < MAX_TONGUE; i++)
	{
		if (GET_LANGUAGE(ch, i) > 0)
		{
			int new_pos = batch_append(batch, pos, 65536, "%s(%d,%d,%d)", has_data ? "," : "", pid, i, GET_LANGUAGE(ch, i));
			if (new_pos < 0)
			{
				free(batch);
				if (own_txn) sql_rollback();
				return false;
			}
			pos = new_pos;
			has_data = true;
		}
	}
	if (has_data)
	{
		if (!sql_run_query(batch))
		{
			free(batch);
			if (own_txn) sql_rollback();
			return false;
		}
	}

	// intros - batch delete then batch insert
	if (!sql_delete_player_subtable(pid, "player_intros"))
	{
		free(batch);
		if (own_txn) sql_rollback();
		return false;
	}

	pos      = snprintf(batch, 65536, "REPLACE INTO player_intros (pid, intro_index, intro_pid, intro_time) VALUES ");
	has_data = false;
	for (int i = 0; i < MAX_INTRO; i++)
	{
		if (ch->only.pc->introd_list[i] != 0)
		{
			int new_pos = batch_append(batch, pos, 65536, "%s(%d,%d,%ld,FROM_UNIXTIME(NULLIF(%lu,0)))", has_data ? "," : "", pid, i, ch->only.pc->introd_list[i], ch->only.pc->introd_times[i]);
			if (new_pos < 0)
			{
				free(batch);
				if (own_txn) sql_rollback();
				return false;
			}
			pos = new_pos;
			has_data = true;
		}
	}
	if (has_data)
	{
		if (!sql_run_query(batch))
		{
			free(batch);
			if (own_txn) sql_rollback();
			return false;
		}
	}

	// timers - batch delete then batch insert

	pos      = snprintf(batch, 65536, "REPLACE INTO player_timers (pid, timer_id, timer_value) VALUES ");
	has_data = false;
	for (int i = 0; i < NUMB_PC_TIMERS; i++)
	{
		if (ch->only.pc->pc_timer[i] != 0)
		{
			int new_pos = batch_append(batch, pos, 65536, "%s(%d,%d,FROM_UNIXTIME(NULLIF(%ld,0)))", has_data ? "," : "", pid, i, (long)ch->only.pc->pc_timer[i]);
			if (new_pos < 0)
			{
				free(batch);
				if (own_txn) sql_rollback();
				return false;
			}
			pos = new_pos;
			has_data = true;
		}
	}
	if (has_data)
	{
		if (!sql_run_query(batch))
		{
			free(batch);
			if (own_txn) sql_rollback();
			return false;
		}
	}

	// undead spell slots - batch delete then batch insert

	pos      = snprintf(batch, 65536, "REPLACE INTO player_undead_slots (pid, circle, slots) VALUES ");
	has_data = false;
	for (int i = 0; i <= MAX_CIRCLE; i++)
	{
		if (ch->specials.undead_spell_slots[i] != 0)
		{
			int new_pos = batch_append(batch, pos, 65536, "%s(%d,%d,%d)", has_data ? "," : "", pid, i, ch->specials.undead_spell_slots[i]);
			if (new_pos < 0)
			{
				free(batch);
				if (own_txn) sql_rollback();
				return false;
			}
			pos = new_pos;
			has_data = true;
		}
	}
	if (has_data)
	{
		if (!sql_run_query(batch))
		{
			free(batch);
			if (own_txn) sql_rollback();
			return false;
		}
	}

	// forged items - batch delete then batch insert

	pos      = snprintf(batch, 65536, "REPLACE INTO player_forged_items (pid, forge_index, item_vnum) VALUES ");
	has_data = false;
	for (int i = 0; i < MAX_FORGE_ITEMS; i++)
	{
		if (ch->only.pc->learned_forged_list[i] != 0)
		{
			int new_pos = batch_append(batch, pos, 65536, "%s(%d,%d,%ld)", has_data ? "," : "", pid, i, ch->only.pc->learned_forged_list[i]);
			if (new_pos < 0)
			{
				free(batch);
				if (own_txn) sql_rollback();
				return false;
			}
			pos = new_pos;
			has_data = true;
		}
	}
	if (has_data)
	{
		if (!sql_run_query(batch))
		{
			free(batch);
			if (own_txn) sql_rollback();
			return false;
		}
	}

	// granted commands - batch delete then batch insert

	if (ch->only.pc->numb_gcmd > 0)
	{
		pos      = snprintf(batch, 65536, "REPLACE INTO player_granted_cmds (pid, cmd_num) VALUES ");
		has_data = false;
		for (int i = 0; i < ch->only.pc->numb_gcmd; i++)
		{
			int new_pos = batch_append(batch, pos, 65536, "%s(%d,%d)", has_data ? "," : "", pid, ch->only.pc->gcmd_arr[i]);
			if (new_pos < 0)
			{
				free(batch);
				if (own_txn) sql_rollback();
				return false;
			}
			pos = new_pos;
			has_data = true;
		}
		if (has_data)
		{
			if (!sql_run_query(batch))
			{
				free(batch);
				if (own_txn) sql_rollback();
				return false;
			}
		}
	}

	free(batch);

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return false; }
	}
	return true;
}

// skills save - batched for performance (2 queries instead of 2000)

bool sql_save_player_skills(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	// Start own transaction if not already in one
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	int pid = GET_PID(ch);
	if (pid <= 0)
	{
		if (own_txn) sql_rollback();
		return false;
	}

	char del_query[128];
	snprintf(del_query, sizeof(del_query), "DELETE FROM player_skills WHERE pid=%d", pid);
	if (!sql_run_query(del_query))
	{
		if (own_txn) sql_rollback();
		return false;
	}

	// build multi-row insert for skills that have values
	// max ~100 skills learned * ~40 bytes per value = ~4kb, use 64kb to be safe
	char *query = (char *)malloc(65536);
	if (!query)
	{
		if (own_txn) sql_rollback();
		return false;
	}

	int pos = snprintf(query, 65536, "REPLACE INTO player_skills (pid, skill_id, learned, taught) VALUES ");

	bool has_skills = false;
	for (int i = 0; i < MAX_SKILLS; i++)
	{
		if (ch->only.pc->skills[i].learned > 0 || ch->only.pc->skills[i].taught > 0)
		{
			int new_pos = batch_append(query, pos, 65536, "%s(%d,%d,%d,%d)", has_skills ? "," : "", pid, i, ch->only.pc->skills[i].learned, ch->only.pc->skills[i].taught);
			if (new_pos < 0)
			{
				free(query);
				if (own_txn) sql_rollback();
				return false;
			}
			pos = new_pos;
			has_skills = true;
		}
	}

	if (has_skills)
	{
		if (!sql_run_query(query))
		{
			free(query);
			if (own_txn) sql_rollback();
			return false;
		}
	}

	free(query);

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return false; }
	}
	return true;
}

// affects save - batched for performance

bool sql_save_player_affects(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	// Start own transaction if not already in one
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	int pid = GET_PID(ch);
	if (pid <= 0)
	{
		if (own_txn) sql_rollback();
		return false;
	}

	char del_query[128];
	snprintf(del_query, sizeof(del_query), "DELETE FROM player_affects WHERE pid=%d", pid);
	if (!sql_run_query(del_query))
	{
		if (own_txn) sql_rollback();
		return false;
	}

	// batch insert current affects
	// each affect ~150 bytes, max ~50 affects = ~8kb, use 32kb to be safe
	char *batch = (char *)malloc(32768);
	if (!batch)
	{
		if (own_txn) sql_rollback();
		return false;
	}

	int pos = snprintf(batch,
	                   32768,
	                   "REPLACE INTO player_affects (pid, type, duration, flags, modifier, location, level, "
	                   "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, custom_msg_char, custom_msg_room) VALUES ");

	bool has_affects = false;
	for (struct affected_type *af = ch->affected; af; af = af->next)
	{
		if (IS_SET(af->flags, AFFTYPE_NOSAVE))
			continue;

		const char *wear_off_char = NULL;
		const char *wear_off_room = NULL;
		if (af->wear_off_message_index > 0 && af->wear_off_message_index < MAX_WEAR_OFF_MESSAGES && af->type >= 0 && af->type < MAX_SKILLS)
		{
			wear_off_char = skills[af->type].wear_off_char[af->wear_off_message_index];
			wear_off_room = skills[af->type].wear_off_room[af->wear_off_message_index];
		}

		char *esc_wear_off_char = wear_off_char ? sql_escape_string(wear_off_char) : NULL;
		char *esc_wear_off_room = wear_off_room ? sql_escape_string(wear_off_room) : NULL;
		if ((wear_off_char && !esc_wear_off_char) || (wear_off_room && !esc_wear_off_room))
		{
			free(esc_wear_off_char);
			free(esc_wear_off_room);
			free(batch);
			if (own_txn) sql_rollback();
			return false;
		}

		char wear_off_char_sql[MAX_STRING_LENGTH * 2 + 3];
		char wear_off_room_sql[MAX_STRING_LENGTH * 2 + 3];
		if (esc_wear_off_char)
			snprintf(wear_off_char_sql, sizeof(wear_off_char_sql), "'%s'", esc_wear_off_char);
		else
			strcpy(wear_off_char_sql, "NULL");
		if (esc_wear_off_room)
			snprintf(wear_off_room_sql, sizeof(wear_off_room_sql), "'%s'", esc_wear_off_room);
		else
			strcpy(wear_off_room_sql, "NULL");

		int new_pos = batch_append(batch, pos, 32768, "%s(%d,%d,%d,%d,%d,%d,%d,%lu,%lu,%lu,%lu,%lu,%s,%s)",
		                has_affects ? "," : "",
		                pid,
		                af->type,
		                af->duration,
		                af->flags,
		                af->modifier,
		                af->location,
		                af->level,
		                af->bitvector,
		                af->bitvector2,
		                af->bitvector3,
		                af->bitvector4,
		                af->bitvector5,
		                wear_off_char_sql,
		                wear_off_room_sql);
		free(esc_wear_off_char);
		free(esc_wear_off_room);
		if (new_pos < 0)
		{
			free(batch);
			if (own_txn) sql_rollback();
			return false;
		}
		pos = new_pos;
		has_affects = true;
	}

	if (has_affects)
	{
		if (!sql_run_query(batch))
		{
			free(batch);
			if (own_txn) sql_rollback();
			return false;
		}
	}

	free(batch);

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return false; }
	}
	return true;
}

// items save

// save item affects (the obj->affected[] array)
static bool sql_save_item_affects(int item_id, P_obj obj)
{
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
		{
			// skip duplicates (same location+modifier already saved)
			bool is_dup = false;
			for (int j = 0; j < i; j++)
			{
				if (obj->affected[j].location == obj->affected[i].location && obj->affected[j].modifier == obj->affected[i].modifier)
				{
					is_dup = true;
					break;
				}
			}
			if (is_dup)
				continue;

			char ins_query[256];
			snprintf(ins_query, sizeof(ins_query), "INSERT INTO player_item_affects (item_id, location, modifier) VALUES (%d, %d, %d)", item_id, obj->affected[i].location, obj->affected[i].modifier);
			if (!sql_run_query(ins_query))
				return false;
		}
	}
	return true;
}

// check if object has any non-default data that needs individual handling
static bool obj_needs_individual_save(P_obj obj)
{
	if (!obj)
		return false;

	// has affects
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
			return true;
	}

	// has extra descriptions
	if (obj->ex_description)
		return true;

	// has nested containers
	if (obj->contains)
		return true;

	// has strung strings
	if (obj->str_mask & (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3))
		return true;

	return false;
}

// batch save simple container contents (items without affects/containers/strings)
// returns number of items saved, -1 on error
static int sql_batch_save_simple_items(int pid, int container_id, P_obj first_obj)
{
	if (!DB || !first_obj)
		return 0;

	// count simple items first
	int simple_count = 0;
	for (P_obj obj = first_obj; obj; obj = obj->next_content)
	{
		if (!IS_SET(obj->extra_flags, ITEM_NORENT) && !obj_needs_individual_save(obj))
			simple_count++;
	}

	if (simple_count == 0)
		return 0;

	// allocate batch buffer - each item needs ~300 bytes for values
	size_t buf_size = 1024 + (simple_count * 400);
	char  *batch    = (char *)malloc(buf_size);
	if (!batch)
		return -1;

	int pos = snprintf(batch,
	                   buf_size,
	                   "INSERT INTO player_items ("
	                   "pid, vnum, equip_slot, container_id, quantity, "
	                   "weight, cost, timer, extra_flags, "
	                   "value0, value1, value2, value3, value4, value5, value6, value7, "
	                   "wear_flags, item_type, item_material, obj_uid, item_condition"
	                   ") VALUES ");

	bool first       = true;
	int  batch_count = 0;
	char wear_str[32], type_str[16], material_str[16], bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];

	for (P_obj obj = first_obj; obj; obj = obj->next_content)
	{
		if (IS_SET(obj->extra_flags, ITEM_NORENT))
			continue;
		if (obj_needs_individual_save(obj))
			continue;

		int vnum = obj_index[obj->R_num].virtual_number;

			sql_format_item_diff_fields_and_free_proto(obj, wear_str, type_str, material_str, bv1_str, bv2_str, bv3_str, bv4_str, bv5_str);
			int new_pos = batch_append(batch, pos, buf_size,
			                           "%s(%d,%d,0,%d,1,%d,%d,%ld,%u,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%s,%lu,%d)",
			                           first ? "" : ",",
			                           pid,
			                           vnum,
			                           container_id,
			                           obj->weight,
			                           obj->cost,
			                           (long)obj->timer[0],
			                           obj->extra_flags,
			                           obj->value[0],
			                           obj->value[1],
			                           obj->value[2],
			                           obj->value[3],
			                           obj->value[4],
			                           obj->value[5],
			                           obj->value[6],
			                           obj->value[7],
			                           wear_str,
			                           type_str,
			                           material_str,
			                           obj->obj_uid,
			                           obj->condition);
		if (new_pos < 0)
		{
			free(batch);
			return -1;
		}
		pos = new_pos;

		first = false;
		batch_count++;

		// flush batch if getting large (stay under 1mb query limit)
		if (pos > (int)(buf_size - 500))
		{
			if (!sql_run_query(batch))
			{
				free(batch);
				return -1;
			}
			// reset for next batch
			pos   = snprintf(batch,
                           buf_size,
                           "INSERT INTO player_items ("
			                 "pid, vnum, equip_slot, container_id, quantity, "
			                 "weight, cost, timer, extra_flags, "
			                 "value0, value1, value2, value3, value4, value5, value6, value7, "
			                 "wear_flags, item_type, item_material, obj_uid, item_condition"
			                 ") VALUES ");
			first = true;
		}
	}

	// flush remaining
	if (!first)
	{
		if (!sql_run_query(batch))
		{
			free(batch);
			return -1;
		}
	}

	free(batch);
	return batch_count;
}

static bool sql_load_item_extra_descr_from_table(int item_id, P_obj obj, const char *table)
{
	char query[256];
	if (!obj || !DB)
		return true;

	// load extra descriptions (spellbooks etc)
	snprintf(query,
	         sizeof(query),
	         "SELECT keyword, description "
	         "FROM %s_extra_descr "
	         "WHERE item_id=%d",
	         table, item_id);

	MYSQL_RES* result = db_query("%s", query);
	if (result)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(result)))
		{
			struct extra_descr_data *ed;
			CREATE(ed, extra_descr_data, 1, MEM_TAG_EXDESCD);

			if (row[0] && strcmp(row[0], "SPELLBOOK") == 0)
			{
				CREATE(ed->keyword, char, 4, MEM_TAG_STRING);
				ed->keyword[0] = 3;
				ed->keyword[1] = 1;
				ed->keyword[2] = 3;
				ed->keyword[3] = '\0';

				size_t buflen = (MAX_SKILLS + 1) / 8 + 1;
				CREATE(ed->description, char, buflen, MEM_TAG_STRING);
				json_to_spellbook(row[1], ed->description);
			}
			else
			{
				ed->keyword     = row[0] ? str_dup(row[0]) : str_dup("");
				ed->description = row[1] ? str_dup(row[1]) : NULL;
			}

			ed->next            = obj->ex_description;
			obj->ex_description = ed;
			obj->str_mask |= STRUNG_EDESC;
		}
		mysql_free_result(result);
	}
	return true;
}

// load item affects from db into obj->affected[]
// clears prototype affects if db has any custom affects
static void sql_load_item_affects_from_table(int item_id, P_obj obj, const char *table)
{
	if (!obj || !DB || item_id <= 0 || !table)
		return;

	char query[256];
	snprintf(query, sizeof(query), "SELECT location, modifier FROM %s WHERE item_id=%d", table, item_id);
	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return;

	MYSQL_ROW row;
	int       aff_idx         = 0;
	bool      affects_cleared = false;

	while ((row = mysql_fetch_row(result)) && aff_idx < MAX_OBJ_AFFECT)
	{
		// clear prototype affects before loading first db affect
		if (!affects_cleared)
		{
			for (int a = 0; a < MAX_OBJ_AFFECT; a++)
			{
				obj->affected[a].location = 0;
				obj->affected[a].modifier = 0;
			}
			affects_cleared = true;
		}

		int loc = atoi(row[0]);
		int mod = atoi(row[1]);

		// skip duplicates from db
		bool is_dup = false;
		for (int d = 0; d < aff_idx; d++)
		{
			if (obj->affected[d].location == loc && obj->affected[d].modifier == mod)
			{
				is_dup = true;
				break;
			}
		}
		if (!is_dup)
		{
			obj->affected[aff_idx].location = loc;
			obj->affected[aff_idx].modifier = mod;
			aff_idx++;
		}
	}
	mysql_free_result(result);
}

static bool sql_save_item_extra_descr(int item_id, P_obj obj, const char *table)
{
	if (!obj || !obj->ex_description || !DB)
		return true;

	struct extra_descr_data *ed;
	for (ed = obj->ex_description; ed; ed = ed->next)
	{
		if (!ed->keyword)
			continue;

		size_t kw_len     = strlen(ed->keyword);
		char  *db_keyword = NULL;
		char  *db_desc    = NULL;

		// spellbook: magic marker \03\01\03
		if (kw_len == 3 && ed->keyword[0] == 3 && ed->keyword[1] == 1 && ed->keyword[2] == 3)
		{
			db_keyword = (char *)malloc(10);
			if (db_keyword)
				strcpy(db_keyword, "SPELLBOOK");
			db_desc = spellbook_to_json(ed->description);
		}
		else
		{
			db_keyword = sql_escape_string(ed->keyword);
			db_desc    = ed->description ? sql_escape_string(ed->description) : NULL;
		}

		if (!db_keyword)
			continue;

		char query[8192];
		if (db_desc)
		{
			snprintf(query, sizeof(query), "INSERT INTO %s (item_id, keyword, description) VALUES (%d, '%s', '%s')", table, item_id, db_keyword, db_desc);
		}
		else
		{
			snprintf(query, sizeof(query), "INSERT INTO %s (item_id, keyword, description) VALUES (%d, '%s', NULL)", table, item_id, db_keyword);
		}

		free(db_keyword);
		if (db_desc)
			free(db_desc);

		if (!sql_run_query(query))
			return false;
	}
	return true;
}

// save a single item and its contents recursively
// returns the item_id of the inserted item, or 0 on failure
static int sql_save_single_item_get_id(int pid, P_obj obj, int equip_slot, int container_id)
{
	if (!obj || !DB)
		return 0;

	// skip norent items
	if (IS_SET(obj->extra_flags, ITEM_NORENT))
		return 0;

	int vnum = obj_index[obj->R_num].virtual_number;

	// escape strings - only save if strung (different from prototype)
	// STRUNG_KEYS = name, STRUNG_DESC2 = short_description,
	// STRUNG_DESC1 = description, STRUNG_DESC3 = action_description
	char *esc_name   = NULL;
	char *esc_short  = NULL;
	char *esc_desc   = NULL;
	char *esc_action = NULL;

	if (obj->str_mask & STRUNG_KEYS)
		esc_name = sql_escape_string(obj->name ? obj->name : "");
	if (obj->str_mask & STRUNG_DESC2)
		esc_short = sql_escape_string(obj->short_description ? obj->short_description : "");
	if (obj->str_mask & STRUNG_DESC1)
		esc_desc = sql_escape_string(obj->description ? obj->description : "");
	if (obj->str_mask & STRUNG_DESC3)
		esc_action = sql_escape_string(obj->action_description ? obj->action_description : "");

	// build container_id string
	char container_str[32];
	if (container_id > 0)
		snprintf(container_str, sizeof(container_str), "%d", container_id);
	else
		strcpy(container_str, "NULL");

	// build name string with quotes or NULL
	char name_str[1024];
	if (esc_name)
		snprintf(name_str, sizeof(name_str), "'%s'", esc_name);
	else
		strcpy(name_str, "NULL");

	char short_str[1024];
	if (esc_short)
		snprintf(short_str, sizeof(short_str), "'%s'", esc_short);
	else
		strcpy(short_str, "NULL");

	char desc_str[2048];
	if (esc_desc)
		snprintf(desc_str, sizeof(desc_str), "'%s'", esc_desc);
	else
		strcpy(desc_str, "NULL");

	char action_str[2048];
	if (esc_action)
		snprintf(action_str, sizeof(action_str), "'%s'", esc_action);
	else
		strcpy(action_str, "NULL");

	char wear_str[32], type_str[16], material_str[16], bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];
	/* shared helper; also frees the loaded proto via extract_obj() */
	sql_format_item_diff_fields_and_free_proto(obj, wear_str, type_str, material_str, bv1_str, bv2_str, bv3_str, bv4_str, bv5_str);

	// build the query
	char query[8192];
	snprintf(query,
	         sizeof(query),
	         "INSERT INTO player_items ("
	         "pid, vnum, equip_slot, container_id, quantity, "
	         "weight, cost, timer, extra_flags, wear_flags, item_type, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, "
	         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
	         "item_material, obj_uid, item_condition"
	         ") VALUES ("
	         "%d, %d, %d, %s, 1, "
	         "%d, %d, %ld, %u, %s, %s, "
	         "%d, %d, %d, %d, %d, %d, %d, %d, "
	         "%s, %s, %s, %s, "
	         "%s, %s, %s, %s, %s, "
	         "%s, %lu, %d"
	         ")",
	         pid,
	         vnum,
	         equip_slot,
	         container_str,
	         obj->weight,
	         obj->cost,
	         (long)obj->timer[0],
	         obj->extra_flags,
	         wear_str,
	         type_str,
	         obj->value[0],
	         obj->value[1],
	         obj->value[2],
	         obj->value[3],
	         obj->value[4],
	         obj->value[5],
	         obj->value[6],
	         obj->value[7],
	         name_str,
	         short_str,
	         desc_str,
	         action_str,
	         bv1_str,
	         bv2_str,
	         bv3_str,
	         bv4_str,
	         bv5_str,
	         material_str,
	         obj->obj_uid,
	         obj->condition);

	// free escaped strings
	if (esc_name)
		free(esc_name);
	if (esc_short)
		free(esc_short);
	if (esc_desc)
		free(esc_desc);
	if (esc_action)
		free(esc_action);

	if (!sql_run_query(query))
	{
		sql_player_error("sql_save_single_item", query);
		return 0;
	}

	// get the inserted item_id
	int item_id     = (int)mysql_insert_id(DB);
	obj->db_item_id = item_id;

	// save item affects
	if (!sql_save_item_affects(item_id, obj))
		return 0;

	if (obj->ex_description && !sql_save_item_extra_descr(item_id, obj, "player_item_extra_descr"))
		return 0;

	// save container contents - batch simple items, individual for complex ones
	if (obj->contains)
	{
		// batch save simple items first (no affects, no strings, no nested containers)
		int batched = sql_batch_save_simple_items(pid, item_id, obj->contains);
		if (batched < 0)
			return 0;

		// individually save complex items (affects, strings, nested containers)
		for (P_obj content = obj->contains; content; content = content->next_content)
		{
			if (IS_SET(content->extra_flags, ITEM_NORENT))
				continue;
			if (!obj_needs_individual_save(content))
				continue; // already batch saved

			if (sql_save_single_item_get_id(pid, content, 0, item_id) == 0)
				return 0;
		}
	}

	return item_id;
}

// false if any item missing db_item_id (needs full save)
static bool all_items_have_db_ids(P_char ch)
{
	for (int i = 0; i < MAX_WEAR; i++)
	{
		P_obj eq = ch->equipment[i] ? ch->equipment[i] : save_equip[i];
		if (eq && eq->db_item_id <= 0)
			return false;
	}
	for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
	{
		if (obj->db_item_id <= 0)
			return false;
	}
	return true;
}

// helper: resave a single container's contents
static bool resave_container_contents(int pid, P_obj container)
{
	if (!container || container->db_item_id <= 0)
		return false;

	int container_db_id = container->db_item_id;

	// verify container still exists in database (may have been deleted by full save)
	char check_query[128];
	snprintf(check_query, sizeof(check_query), "SELECT 1 FROM player_items WHERE id=%d LIMIT 1", container_db_id);
	MYSQL_RES *check_result = db_query("%s", check_query);
	if (!check_result)
	{
		container->db_item_id = 0;
		return false;
	}
	MYSQL_ROW row    = mysql_fetch_row(check_result);
	bool      exists = (row != NULL);
	mysql_free_result(check_result);
	if (!exists)
	{
		container->db_item_id = 0;
		return false;
	}

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	// delete old contents
	char del_query[256];
	snprintf(del_query, sizeof(del_query), "DELETE FROM player_items WHERE container_id=%d", container_db_id);
	if (!sql_run_query(del_query))
	{
		if (own_txn) sql_rollback();
		return false;
	}

	// re-insert contents
	if (container->contains)
	{
		int batched = sql_batch_save_simple_items(pid, container_db_id, container->contains);
		if (batched < 0)
		{
			if (own_txn) sql_rollback();
			return false;
		}

		for (P_obj content = container->contains; content; content = content->next_content)
		{
			if (IS_SET(content->extra_flags, ITEM_NORENT))
				continue;
			if (!obj_needs_individual_save(content))
				continue;

			if (sql_save_single_item_get_id(pid, content, 0, container_db_id) == 0)
			{
				if (own_txn) sql_rollback();
				return false;
			}
		}
	}

	if (own_txn)
	{
		if (!sql_commit())
		{
			sql_rollback();
			return false;
		}
	}

	return true;
}

// helper: recursively find and resave dirty containers
static bool resave_dirty_containers(int pid, P_obj obj)
{
	if (!obj)
		return true;

	if (IS_SET(obj->runtime_flags, OBJ_RFLAG_DIRTY_CONTAINER))
	{
		if (!resave_container_contents(pid, obj))
			return false;
		REMOVE_BIT(obj->runtime_flags, OBJ_RFLAG_DIRTY_CONTAINER);
	}

	// check nested containers
	for (P_obj content = obj->contains; content; content = content->next_content)
	{
		if (content->contains)
		{
			if (!resave_dirty_containers(pid, content))
				return false;
		}
	}
	return true;
}

// Batched player item save — flattens entire item tree into one
// multi-row INSERT, then fixes up container_id relationships and saves
// affects/extra_descrs.  Replaces the per-item INSERT loop, reducing
// ~170 individual queries to ~3 per save.

// Structure for one item in the flattened tree
struct flat_item {
	P_obj obj;
	P_obj parent;         // NULL for top-level items
	int   equip_slot;     // 1..MAX_WEAR for equipment, 0 for inventory/container contents
	bool  single_saved;   // true if saved via per-item fallback (affects/descr already handled)
};

// Recursively flatten item tree: pre-order traversal (parent before children)
static bool flatten_item_tree(P_obj obj, P_obj parent, int equip_slot,
                              struct flat_item **list, int *count, int *capacity)
{
	if (!obj || IS_SET(obj->extra_flags, ITEM_NORENT))
		return true;

	if (*count >= *capacity)
	{
		int new_cap = *capacity * 2;
		struct flat_item *tmp = (struct flat_item *)realloc(*list, new_cap * sizeof(struct flat_item));
		if (!tmp)
			return false;  // old *list still valid — caller can inspect count
		*list     = tmp;
		*capacity = new_cap;
	}

	(*list)[*count].obj        = obj;
	(*list)[*count].parent     = parent;
	(*list)[*count].equip_slot = equip_slot;
	(*list)[*count].single_saved = false;
	(*count)++;

	// Recurse into container contents
	for (P_obj content = obj->contains; content; content = content->next_content)
	{
		if (!flatten_item_tree(content, obj, 0, list, count, capacity))
			return false;
	}

	return true;
}

static bool sql_save_player_items_batch_all(int pid, P_char ch,
                                            bool save_equipment, bool save_inventory)
{
	// ——— Step 1: flatten item tree ———————————————————————————————————————————————————————————————
	int cap = 128;
	struct flat_item *flat = (struct flat_item *)malloc(cap * sizeof(struct flat_item));
	if (!flat)
	{
		logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: malloc(flat) failed\n");
		return false;
	}

	int count = 0;

	// Equipment (equip_slot = 1..MAX_WEAR for the save query)
	if (save_equipment || save_inventory)
	{
		for (int i = 0; i < MAX_WEAR; i++)
		{
			P_obj eq = ch->equipment[i] ? ch->equipment[i] : save_equip[i];
			if (eq && !flatten_item_tree(eq, NULL, i + 1, &flat, &count, &cap))
			{
				logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: flatten equipment failed at slot=%d\n", i);
				free(flat);
				return false;
			}
		}
	}

	// Inventory (equip_slot = 0)
	if (save_inventory)
	{
		for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
			if (!flatten_item_tree(obj, NULL, 0, &flat, &count, &cap))
			{
				logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: flatten inventory failed\n");
				free(flat);
				return false;
			}
}

	if (count == 0)
	{
		free(flat);
		return true;   // nothing to save
	}

	// ——— Step 2 & 3: build multi-row INSERTs in sub-batches —————————————————————————————
	// Use 1MB buffer to respect MySQL max_allowed_packet (4MB default on 5.7).
	// Large inventories automatically split across multiple INSERT statements.
	const size_t BATCH_BUF_SIZE = 1048576;      // 1 MB
	const int    FLUSH_THRESHOLD = 1000000;      // flush when approaching 1 MB
	char  *batch = (char *)malloc(BATCH_BUF_SIZE);
	if (!batch)
	{
		logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: malloc(batch) failed\n");
		free(flat);
		return false;
	}

	const char *insert_header = "INSERT INTO player_items ("
	                   "pid, vnum, equip_slot, container_id, quantity, "
	                   "weight, cost, timer, extra_flags, wear_flags, item_type, "
	                   "value0, value1, value2, value3, value4, value5, value6, value7, "
	                   "name, short_descr, description, action_descr, "
	                   "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
	                   "item_material, obj_uid, item_condition"
	                   ") VALUES ";

	int pos             = snprintf(batch, BATCH_BUF_SIZE, "%s", insert_header);
	int batch_start_idx = 0;
	int items_in_batch  = 0;

	for (int i = 0; i < count; i++)
	{
		P_obj obj  = flat[i].obj;
		int   vnum = obj_index[obj->R_num].virtual_number;

		// Escape strung strings (same logic as sql_save_single_item_get_id)
		char *esc_name   = NULL;
		char *esc_short  = NULL;
		char *esc_desc   = NULL;
		char *esc_action = NULL;

		if (obj->str_mask & STRUNG_KEYS)
			esc_name = sql_escape_string(obj->name ? obj->name : "");
		if (obj->str_mask & STRUNG_DESC2)
			esc_short = sql_escape_string(obj->short_description ? obj->short_description : "");
		if (obj->str_mask & STRUNG_DESC1)
			esc_desc = sql_escape_string(obj->description ? obj->description : "");
		if (obj->str_mask & STRUNG_DESC3)
			esc_action = sql_escape_string(obj->action_description ? obj->action_description : "");

		// Build name/short/desc/action strings with quotes or NULL
		char name_str[1024], short_str[1024], desc_str[2048], action_str[2048];
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

		// Get diff-from-prototype fields (wear_flags, type, material, bitvectors)
		char wear_str[32], type_str[16], material_str[16];
		char bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];
		sql_format_item_diff_fields_and_free_proto(obj, wear_str, type_str, material_str,
		                                           bv1_str, bv2_str, bv3_str, bv4_str, bv5_str);

		// Pre-format this single row into a temp buffer.
		// If the row itself is too large (>16KB) for a single INSERT,
		// fall back to per-item sql_save_single_item_get_id().
		char row_buf[16384];
		int row_len = snprintf(row_buf, sizeof(row_buf),
		                       "%s(%d,%d,%d,NULL,1,%d,%d,%ld,%u,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%lu,%d)",
		                       (items_in_batch == 0) ? "" : ",",
		                       pid, vnum, flat[i].equip_slot,
		                       obj->weight, obj->cost, (long)obj->timer[0], obj->extra_flags,
		                       wear_str, type_str,
		                       obj->value[0], obj->value[1], obj->value[2], obj->value[3],
		                       obj->value[4], obj->value[5], obj->value[6], obj->value[7],
		                       name_str, short_str, desc_str, action_str,
		                       bv1_str, bv2_str, bv3_str, bv4_str, bv5_str,
		                       material_str, (unsigned long)obj->obj_uid, obj->condition);

		// Free escaped strings
		if (esc_name)
			free(esc_name);
		if (esc_short)
			free(esc_short);
		if (esc_desc)
			free(esc_desc);
		if (esc_action)
			free(esc_action);

		// Per-row overflow fallback: single item exceeds format buffer.
		if (row_len >= (int)sizeof(row_buf) - 1 || row_len < 0)
		{
			logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: row too large at item %d/%d vnum=%d\n", i, count, vnum);
			logit(LOG_DEBUG, "sql_save_player_items_batch_all: item vnum %d row too large, using single-insert fallback", vnum);

			// Temporarily detach contents so sql_save_single_item_get_id
			// doesn't recurse (tree is already flattened).  Restore after.
			P_obj saved_contains = obj->contains;
			obj->contains        = NULL;

			obj->db_item_id       = sql_save_single_item_get_id(pid, obj, flat[i].equip_slot, 0);
			flat[i].single_saved  = true;

			obj->contains = saved_contains;

			if (obj->db_item_id <= 0)
			{
				free(batch);
				free(flat);
				return false;
			}
			continue;
		}

		// Sub-batch flush: approaching 1 MB — execute current batch and restart.
		if (items_in_batch > 0 && pos + row_len > FLUSH_THRESHOLD)
		{
			if (!sql_run_query(batch))
			{
				logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: sub-batch query failed at item %d/%d\n", i, count);
				free(batch);
				free(flat);
				return false;
			}

			// Assign db_item_ids for this sub-batch (64-bit mysql_insert_id)
			unsigned long long first_id = mysql_insert_id(DB);
			int offset = 0;
			for (int k = batch_start_idx; k < i; k++)
			{
				if (!flat[k].single_saved)
				{
					flat[k].obj->db_item_id = (int)(first_id + offset);
					offset++;
				}
			}

			// Restart batch for remaining items
			pos             = snprintf(batch, BATCH_BUF_SIZE, "%s", insert_header);
			batch_start_idx = i;
			items_in_batch  = 0;

			// Strip leading comma from the first row of the new batch
			if (row_buf[0] == ',')
			{
				memmove(row_buf, row_buf + 1, (size_t)row_len);  // shifts null terminator too
				row_len--;
			}
		}

		int new_pos = batch_append(batch, pos, BATCH_BUF_SIZE, "%s", row_buf);
		if (new_pos < 0)
		{
			logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: row append failed at item %d/%d\n", i, count);
free(batch);
			free(flat);
			return false;
		}
		pos = new_pos;
		items_in_batch++;
	}

	// Flush the final sub-batch
	if (items_in_batch > 0)
	{
		if (!sql_run_query(batch))
		{
			logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: final batch query failed\nQUERY=%s\n", batch);
			free(batch);
			free(flat);
			return false;
		}

		unsigned long long first_id = mysql_insert_id(DB);
		int offset = 0;
		for (int k = batch_start_idx; k < count; k++)
		{
			if (!flat[k].single_saved)
			{
				flat[k].obj->db_item_id = (int)(first_id + offset);
				offset++;
			}
		}
	}

	// ——— Step 4: fix up container_id for items inside containers —————————————————————————
	int container_child_count = 0;
	for (int i = 0; i < count; i++)
	{
		if (flat[i].parent)
			container_child_count++;
	}

	if (container_child_count > 0)
	{
		pos = snprintf(batch, BATCH_BUF_SIZE, "UPDATE player_items SET container_id = CASE id ");

		for (int i = 0; i < count; i++)
		{
			if (flat[i].parent)
			{
				int new_pos = batch_append(batch, pos, BATCH_BUF_SIZE,
				                           "WHEN %d THEN %d ", flat[i].obj->db_item_id, flat[i].parent->db_item_id);
				if (new_pos < 0)
				{
					logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: container UPDATE build failed (case 2)\n");
					free(batch);
					free(flat);
					return false;
				}
				pos = new_pos;
			}
		}

		pos = batch_append(batch, pos, BATCH_BUF_SIZE, "END WHERE id IN (");
		bool first_in = true;
		for (int i = 0; i < count; i++)
		{
			if (flat[i].parent)
			{
				int new_pos = batch_append(batch, pos, BATCH_BUF_SIZE,
				                           "%s%d", first_in ? "" : ",", flat[i].obj->db_item_id);
				if (new_pos < 0)
				{
					logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items_batch_all: container UPDATE build failed (case 2)\n");
					free(batch);
					free(flat);
					return false;
				}
				pos = new_pos;
				first_in = false;
			}
		}
		pos = batch_append(batch, pos, BATCH_BUF_SIZE, ")");

		if (!sql_run_query(batch))
		{
			sql_player_error("sql_save_player_items_batch_all container UPDATE", batch);
			free(batch);
			free(flat);
			return false;
		}
	}

	free(batch);

	// ——— Step 5: save affects and extra descriptions per item ————————————————————————————
	for (int i = 0; i < count; i++)
	{
		// Items saved via per-item fallback already had affects/descr handled
		if (flat[i].single_saved)
			continue;

		P_obj obj     = flat[i].obj;
		int   item_id = obj->db_item_id;

		if (!sql_save_item_affects(item_id, obj))
		{
			free(flat);
			return false;
		}

		if (obj->ex_description)
		{
			if (!sql_save_item_extra_descr(item_id, obj, "player_item_extra_descr"))
			{
				free(flat);
				return false;
			}
		}
	}

	free(flat);
	return true;
}

bool sql_save_player_items(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	trace_append_file("sql_save_player_items begin name=%s pid=%d eq0=%p carrying=%p\n",
	                  GET_NAME(ch), GET_PID(ch), ch->equipment[0], ch->carrying);

	// Start own transaction if not already in one
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	int pid = GET_PID(ch);
	if (pid <= 0)
	{
		if (own_txn) sql_rollback();
		return false;
	}

	bool save_equipment  = IS_SET(ch->runtime_flags, CHAR_RFLAG_DIRTY_EQUIPMENT);
	bool save_inventory  = IS_SET(ch->runtime_flags, CHAR_RFLAG_DIRTY_INVENTORY);
	bool use_incremental = all_items_have_db_ids(ch) && !save_equipment && !save_inventory;

	logit(LOG_DEBUG,
	             "[real-persistence-test] sql_save_player_items debug: pid=%d save_equipment=%d save_inventory=%d use_incremental=%d carrying=%p eq0=%p\n",
	             pid,
	             save_equipment,
	             save_inventory,
	             use_incremental,
	             ch->carrying,
	             ch->equipment[0]);

	if (use_incremental)
	{
		// incremental save: only resave dirty containers
		for (int i = 0; i < MAX_WEAR; i++)
		{
			P_obj eq = ch->equipment[i] ? ch->equipment[i] : save_equip[i];
			if (eq)
			{
				if (!resave_dirty_containers(pid, eq))
				{
					if (own_txn) sql_rollback();
					return false;
				}
			}
		}
		for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
		{
			if (!resave_dirty_containers(pid, obj))
			{
				if (own_txn) sql_rollback();
				return false;
			}
		}
		if (own_txn)
		{
			if (!sql_commit()) { sql_rollback(); return false; }
		}
		return true;
	}

	char del_query[128] = {0};
	if (save_inventory)
	{
		// full save: delete all and re-insert
		snprintf(del_query, sizeof(del_query), "DELETE FROM player_items WHERE pid=%d", pid);
	}
	else if (save_equipment)
	{
		// only saving equipment, so only remove existing equipment
		snprintf(del_query, sizeof(del_query), "DELETE FROM player_items WHERE pid=%d AND equip_slot>0", pid);
	}
	if (del_query[0] && !sql_run_query(del_query))
	{
		if (own_txn) sql_rollback();
		trace_append_file("sql_save_player_items delete failed name=%s pid=%d query=%s\n",
		                  GET_NAME(ch), GET_PID(ch), del_query);
		return false;
	}

	bool success = sql_save_player_items_batch_all(pid, ch, save_equipment, save_inventory);
	if (!success)
		logit(LOG_DEBUG, "[real-persistence-test] sql_save_player_items debug: batch_all failed for pid=%d\n", pid);
	trace_append_file("sql_save_player_items batch result name=%s pid=%d success=%d save_equipment=%d save_inventory=%d\n",
	                  GET_NAME(ch), pid, success ? 1 : 0, save_equipment, save_inventory);

	if (own_txn)
	{
		if (success)
		{
			if (!sql_commit()) { sql_rollback(); return false; }
		}
		else
		{
			sql_rollback();
			return false;
		}
	}

	trace_append_file("sql_save_player_items done name=%s pid=%d\n", GET_NAME(ch), pid);
	return success;
}

bool sql_delete_player_items(int pid)
{
	if (!DB || pid <= 0)
		return false;

	char del_query[128];
	snprintf(del_query, sizeof(del_query), "DELETE FROM player_items WHERE pid=%d", pid);
	return sql_run_query(del_query);
}

// pet item affects save
static bool sql_save_pet_item_affects(int item_id, P_obj obj)
{
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
		{
			char ins_query[256];
			snprintf(
				ins_query, sizeof(ins_query), "INSERT INTO player_pet_item_affects (item_id, location, modifier) VALUES (%d, %d, %d)", item_id, obj->affected[i].location, obj->affected[i].modifier);
			if (!sql_run_query(ins_query))
				return false;
		}
	}
	return true;
}

// save a single pet item and its contents recursively
static int sql_save_single_pet_item(int pet_id, P_obj obj, int equip_slot, int container_id)
{
	if (!obj || !DB)
		return 0;

	if (IS_SET(obj->extra_flags, ITEM_NORENT))
		return 0;

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return 0;
		own_txn = true;
	}

	int vnum = obj_index[obj->R_num].virtual_number;

	char *esc_name   = NULL;
	char *esc_short  = NULL;
	char *esc_desc   = NULL;
	char *esc_action = NULL;

	if (obj->str_mask & STRUNG_KEYS)
		esc_name = sql_escape_string(obj->name ? obj->name : "");
	if (obj->str_mask & STRUNG_DESC2)
		esc_short = sql_escape_string(obj->short_description ? obj->short_description : "");
	if (obj->str_mask & STRUNG_DESC1)
		esc_desc = sql_escape_string(obj->description ? obj->description : "");
	if (obj->str_mask & STRUNG_DESC3)
		esc_action = sql_escape_string(obj->action_description ? obj->action_description : "");

	char container_str[32];
	if (container_id > 0)
		snprintf(container_str, sizeof(container_str), "%d", container_id);
	else
		strcpy(container_str, "NULL");

	char name_str[1024];
	if (esc_name)
		snprintf(name_str, sizeof(name_str), "'%s'", esc_name);
	else
		strcpy(name_str, "NULL");

	char short_str[1024];
	if (esc_short)
		snprintf(short_str, sizeof(short_str), "'%s'", esc_short);
	else
		strcpy(short_str, "NULL");

	char desc_str[2048];
	if (esc_desc)
		snprintf(desc_str, sizeof(desc_str), "'%s'", esc_desc);
	else
		strcpy(desc_str, "NULL");

	char action_str[2048];
	if (esc_action)
		snprintf(action_str, sizeof(action_str), "'%s'", esc_action);
	else
		strcpy(action_str, "NULL");

	// shared helper for diff-from-prototype fields (wear_flags, item_type, item_material, bitvectors)
	char wear_str[32], type_str[16], material_str[16], bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];
	sql_format_item_diff_fields_and_free_proto(obj, wear_str, type_str, material_str, bv1_str, bv2_str, bv3_str, bv4_str, bv5_str);

	char query[8192];
	snprintf(query,
	         sizeof(query),
	         "INSERT INTO player_pet_items ("
	         "pet_id, vnum, equip_slot, container_id, "
	         "weight, cost, timer, extra_flags, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, wear_flags, item_type, bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
	         "item_material"
	         ") VALUES ("
	         "%d, %d, %d, %s, "
	         "%d, %d, %ld, %lu, "
	         "%d, %d, %d, %d, %d, %d, %d, %d, "
	         "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, "
	         "%s"
	         ")",
	         pet_id,
	         vnum,
	         equip_slot,
	         container_str,
	         obj->weight,
	         obj->cost,
	         (long)obj->timer[0],
	         (unsigned long)obj->extra_flags,
	         obj->value[0],
	         obj->value[1],
	         obj->value[2],
	         obj->value[3],
	         obj->value[4],
	         obj->value[5],
	         obj->value[6],
	         obj->value[7],
	         name_str,
	         short_str,
	         desc_str,
	         action_str,
	         wear_str,
	         type_str,
	         bv1_str,
	         bv2_str,
	         bv3_str,
	         bv4_str,
	         bv5_str,
	         material_str);

	if (esc_name)
		free(esc_name);
	if (esc_short)
		free(esc_short);
	if (esc_desc)
		free(esc_desc);
	if (esc_action)
		free(esc_action);

	if (!sql_run_query(query))
	{
		logit(LOG_DEBUG, "sql_save_item: insert failed errno=%u sqlerr=%s", mysql_errno(DB), mysql_error(DB));
		if (own_txn) sql_rollback();
		return 0;
	}

	int item_id = (int)mysql_insert_id(DB);

	if (!sql_save_pet_item_affects(item_id, obj))
	{
		if (own_txn) sql_rollback();
		return 0;
	}

	if (obj->ex_description && !sql_save_item_extra_descr(item_id, obj, "player_pet_item_extra_descr"))
	{
		if (own_txn) sql_rollback();
		return 0;
	}

	if (obj->contains)
	{
		for (P_obj content = obj->contains; content; content = content->next_content)
		{
			if (!IS_SET(content->extra_flags, ITEM_NORENT))
			{
				if (sql_save_single_pet_item(pet_id, content, 0, item_id) <= 0)
				{
					if (own_txn) sql_rollback();
					return false;
				}
			}
		}
	}

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return 0; }
	}
	return item_id;
}

// pet save - save all player's pets with equipment
bool sql_save_player_pets(P_char ch, int save_type)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	// Start own transaction if not already in one
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	// only save pets on crash-type saves
	if (save_type != RENT_CRASH && save_type != RENT_CRASH2)
	{
		// clear any existing saved pets on normal logout
		int pid = GET_PID(ch);
		if (pid > 0)
		{
			char del_query[128];
			snprintf(del_query, sizeof(del_query), "DELETE FROM player_pets WHERE owner_pid=%d", pid);
			if (!sql_run_query(del_query))
			{
				if (own_txn) sql_rollback();
				return false;
			}
		}
		if (own_txn)
		{
			if (!sql_commit()) { sql_rollback(); return false; }
		}
		return true;
	}

	int pid = GET_PID(ch);
	if (pid <= 0)
	{
		if (own_txn) sql_rollback();
		return false;
	}

	// delete existing pets for this player (cascade deletes items/affects)
	char del_query[128];
	snprintf(del_query, sizeof(del_query), "DELETE FROM player_pets WHERE owner_pid=%d", pid);
	if (!sql_run_query(del_query))
	{
		if (own_txn) sql_rollback();
		return false;
	}

	// iterate through followers and save npc pets
	int pet_order = 0;
	for (struct follow_type *f = ch->followers; f; f = f->next)
	{
		P_char pet = f->follower;
		if (!pet || !IS_NPC(pet))
			continue;

		// only save pets in same room
		if (pet->in_room != ch->in_room)
			continue;

		int mob_vnum  = mob_index[GET_RNUM(pet)].virtual_number;
		int room_vnum = (pet->in_room >= 0) ? world[pet->in_room].number : 0;

		// get charm duration from affect if exists
		int charm_duration = -1;
		for (struct affected_type *af = pet->affected; af; af = af->next)
		{
			if (af->type == SPELL_CHARM_PERSON)
			{
				charm_duration = af->duration;
				break;
			}
		}

		char ins_query[512];
		snprintf(ins_query,
		         sizeof(ins_query),
		         "INSERT INTO player_pets (owner_pid, mob_vnum, pet_order, hit, max_hit, mana, max_mana, "
		         "vitality, max_vitality, charm_duration, room_vnum, saved_at) "
		         "VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, FROM_UNIXTIME(NULLIF(%ld,0)))",
		         pid,
		         mob_vnum,
		         pet_order,
		         GET_HIT(pet),
		         GET_MAX_HIT(pet),
		         GET_MANA(pet),
		         GET_MAX_MANA(pet),
		         GET_VITALITY(pet),
		         GET_MAX_VITALITY(pet),
		         charm_duration,
		         room_vnum,
		         (long)time(0));

		if (!sql_run_query(ins_query))
		{
			logit(LOG_DEBUG, "sql_save_player_pets: failed to save pet %s for %s", GET_NAME(pet), GET_NAME(ch));
			if (own_txn)
				sql_rollback();
			return false;
		}

		int pet_id = (int)mysql_insert_id(DB);

		// save pet equipment
		for (int i = 0; i < MAX_WEAR; i++)
		{
			if (pet->equipment[i] && !IS_SET(pet->equipment[i]->extra_flags, ITEM_NORENT))
			{
				if (sql_save_single_pet_item(pet_id, pet->equipment[i], i + 1, 0) <= 0)
				{
					if (own_txn) sql_rollback();
					return false;
				}
			}
		}

		// save pet inventory
		for (P_obj obj = pet->carrying; obj; obj = obj->next_content)
		{
			if (!IS_SET(obj->extra_flags, ITEM_NORENT))
			{
				if (sql_save_single_pet_item(pet_id, obj, 0, 0) <= 0)
				{
					if (own_txn) sql_rollback();
					return false;
				}
			}
		}

		pet_order++;
		logit(LOG_DEBUG, "sql_save_player_pets: saved pet %s (vnum %d) for %s with %d hp", GET_NAME(pet), mob_vnum, GET_NAME(ch), GET_HIT(pet));
	}

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return false; }
	}
	return true;
}

// pet load - restore all player's pets with equipment
bool sql_load_player_pets(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	int pid = GET_PID(ch);
	if (pid <= 0)
		return false;

	char query[256];
	snprintf(query,
	         sizeof(query),
	         "SELECT id, mob_vnum, hit, max_hit, mana, max_mana, vitality, max_vitality, charm_duration "
	         "FROM player_pets WHERE owner_pid=%d ORDER BY pet_order",
	         pid);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		int pet_db_id      = atoi(row[0]);
		int mob_vnum       = atoi(row[1]);
		int hit            = atoi(row[2]);
		int max_hit        = atoi(row[3]);
		int mana           = atoi(row[4]);
		int max_mana       = atoi(row[5]);
		int vitality       = atoi(row[6]);
		int max_vitality   = atoi(row[7]);
		int charm_duration = atoi(row[8]);

		int pet_rnum = real_mobile(mob_vnum);
		if (pet_rnum < 0)
		{
			logit(LOG_DEBUG, "sql_load_player_pets: bad vnum %d for %s", mob_vnum, GET_NAME(ch));
			continue;
		}

		P_char pet = read_mobile(pet_rnum, REAL);
		if (!pet)
		{
			logit(LOG_DEBUG, "sql_load_player_pets: failed to create mob %d for %s", mob_vnum, GET_NAME(ch));
			continue;
		}

		// place pet in player's room
		char_to_room(pet, ch->in_room, FALSE);

		// setup as pet with charm
		setup_pet(pet, ch, charm_duration, PET_NOAGGRO);
		add_follower(pet, ch);

		// restore stats
		GET_HIT(pet)          = hit;
		GET_MAX_HIT(pet)      = max_hit;
		GET_MANA(pet)         = mana;
		GET_MAX_MANA(pet)     = max_mana;
		GET_VITALITY(pet)     = vitality;
		GET_MAX_VITALITY(pet) = max_vitality;

		// load pet equipment and inventory
		char item_query[512];
		snprintf(item_query,
		         sizeof(item_query),
		         "SELECT id, vnum, equip_slot, container_id, weight, cost, timer, extra_flags, "
		         "value0, value1, value2, value3, value4, value5, value6, value7, "
		         "name, short_descr, description, action_descr, "
		         "wear_flags, item_type, item_material, "
		         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5 "
		         "FROM player_pet_items WHERE pet_id=%d ORDER BY id",
		         pet_db_id);

		MYSQL_RES *item_result = db_query("%s", item_query);
		if (item_result)
		{
			// two-pass: first create all items, then place them
			// need to handle containers properly
			struct
			{
				int   db_id;
				int   container_id;
				int   equip_slot;
				P_obj obj;
			} items[256];
			int item_count = 0;

			MYSQL_ROW item_row;
			while ((item_row = mysql_fetch_row(item_result)) && item_count < 256)
			{
				int item_db_id   = atoi(item_row[0]);
				int obj_vnum     = atoi(item_row[1]);
				int equip_slot   = atoi(item_row[2]);
				int container_id = item_row[3] ? atoi(item_row[3]) : 0;

				int obj_rnum = real_object(obj_vnum);
				if (obj_rnum < 0)
					continue;

				P_obj obj = read_object(obj_rnum, REAL);
				if (!obj)
					continue;

				// restore item properties
				obj->weight      = atoi(item_row[4]);
				obj->cost        = atoi(item_row[5]);
				obj->timer[0]    = atol(item_row[6]);
				obj->extra_flags = strtoul(item_row[7], NULL, 10);
				obj->value[0]    = atoi(item_row[8]);
				obj->value[1]    = atoi(item_row[9]);
				obj->value[2]    = atoi(item_row[10]);
				obj->value[3]    = atoi(item_row[11]);
				obj->value[4]    = atoi(item_row[12]);
				obj->value[5]    = atoi(item_row[13]);
				obj->value[6]    = atoi(item_row[14]);
				obj->value[7]    = atoi(item_row[15]);

				// restore strung strings if present
				if (item_row[16] && strlen(item_row[16]) > 0)
				{
					obj->name = str_dup(item_row[16]);
					obj->str_mask |= STRUNG_KEYS;
				}
				if (item_row[17] && strlen(item_row[17]) > 0)
				{
					obj->short_description = str_dup(item_row[17]);
					obj->str_mask |= STRUNG_DESC2;
				}
				if (item_row[18] && strlen(item_row[18]) > 0)
				{
					obj->description = str_dup(item_row[18]);
					obj->str_mask |= STRUNG_DESC1;
				}
				if (item_row[19] && strlen(item_row[19]) > 0)
				{
					obj->action_description = str_dup(item_row[19]);
					obj->str_mask |= STRUNG_DESC3;
				}

				if (item_row[20])
					obj->wear_flags = atoi(item_row[20]);
				if (item_row[21])
					obj->type = sql_validate_loaded_item_type(obj, atoi(item_row[21]), "sql_load_player_pet_items");
				if (item_row[22])
					obj->material = atoi(item_row[22]);
				if (item_row[23])
					obj->bitvector = strtoul(item_row[23], NULL, 10);
				if (item_row[24])
					obj->bitvector2 = strtoul(item_row[24], NULL, 10);
				if (item_row[25])
					obj->bitvector3 = strtoul(item_row[25], NULL, 10);
				if (item_row[26])
					obj->bitvector4 = strtoul(item_row[26], NULL, 10);
				if (item_row[27])
					obj->bitvector5 = strtoul(item_row[27], NULL, 10);

				// load item affects
				char affect_query[256];
				snprintf(affect_query, sizeof(affect_query), "SELECT location, modifier FROM player_pet_item_affects WHERE item_id=%d", item_db_id);
				MYSQL_RES *affect_result = db_query("%s", affect_query);
				if (affect_result)
				{
					int       aff_idx = 0;
					MYSQL_ROW affect_row;
					while ((affect_row = mysql_fetch_row(affect_result)) && aff_idx < MAX_OBJ_AFFECT)
					{
						obj->affected[aff_idx].location = atoi(affect_row[0]);
						obj->affected[aff_idx].modifier = atoi(affect_row[1]);
						aff_idx++;
					}
					mysql_free_result(affect_result);
				}

				// load extra descriptions
				snprintf(affect_query, sizeof(affect_query), "SELECT keyword, description FROM player_pet_item_extra_descr WHERE item_id=%d", item_db_id);
				MYSQL_RES *ed_result = db_query("%s", affect_query);
				if (ed_result)
				{
					MYSQL_ROW ed_row;
					while ((ed_row = mysql_fetch_row(ed_result)))
					{
						struct extra_descr_data *ed;
						CREATE(ed, extra_descr_data, 1, MEM_TAG_EXDESCD);

						if (ed_row[0] && strcmp(ed_row[0], "SPELLBOOK") == 0)
						{
							CREATE(ed->keyword, char, 4, MEM_TAG_STRING);
							ed->keyword[0] = 3;
							ed->keyword[1] = 1;
							ed->keyword[2] = 3;
							ed->keyword[3] = '\0';

							size_t buflen = (MAX_SKILLS + 1) / 8 + 1;
							CREATE(ed->description, char, buflen, MEM_TAG_STRING);
							json_to_spellbook(ed_row[1], ed->description);
						}
						else
						{
							ed->keyword     = ed_row[0] ? str_dup(ed_row[0]) : str_dup("");
							ed->description = ed_row[1] ? str_dup(ed_row[1]) : NULL;
						}

						ed->next            = obj->ex_description;
						obj->ex_description = ed;
						obj->str_mask |= STRUNG_EDESC;
					}
					mysql_free_result(ed_result);
				}

				items[item_count].db_id        = item_db_id;
				items[item_count].container_id = container_id;
				items[item_count].equip_slot   = equip_slot;
				items[item_count].obj          = obj;
				item_count++;
			}
			mysql_free_result(item_result);

			// place items - containers first, then equip/inventory
			for (int i = 0; i < item_count; i++)
			{
				if (items[i].container_id > 0)
				{
					// find container and put item in it
					for (int j = 0; j < item_count; j++)
					{
						if (items[j].db_id == items[i].container_id && items[j].obj)
						{
							obj_to_obj(items[i].obj, items[j].obj);
							break;
						}
					}
				}
				else if (items[i].equip_slot > 0 && items[i].equip_slot <= MAX_WEAR)
				{
					equip_char(pet, items[i].obj, items[i].equip_slot - 1, 9);
				}
				else
				{
					obj_to_char(items[i].obj, pet);
				}
			}

			for (int k = 0; k < item_count; k++)
			{
				if (items[k].obj)
				{
					recalc_container_weight(items[k].obj);
				}
			}
		}

		logit(LOG_DEBUG, "sql_load_player_pets: restored pet %s (vnum %d) for %s with %d/%d hp", GET_NAME(pet), mob_vnum, GET_NAME(ch), GET_HIT(pet), GET_MAX_HIT(pet));
	}

	mysql_free_result(result);

	// delete the saved pets after successful load
	char del_query[128];
	snprintf(del_query, sizeof(del_query), "DELETE FROM player_pets WHERE owner_pid=%d", pid);
	if (!sql_run_query(del_query))
	{
		return false;
	}

	return true;
}

// witnesses save

bool sql_save_player_witnesses(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	// Start own transaction if not already in one
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	int pid = GET_PID(ch);
	if (pid <= 0)
	{
		if (own_txn) sql_rollback();
		return false;
	}

	// DELETE + INSERT batch in one multi-statement round-trip.
	char batch[16384];
	int  bpos = snprintf(batch, sizeof(batch),
	                     "DELETE FROM player_witnesses WHERE pid=%d", pid);

	// insert current witnesses, flushing as needed
	for (wtns_rec *w = ch->specials.witnessed; w; w = w->next)
	{
		char *esc_attacker = sql_escape_string(w->attacker ? w->attacker : "");
		char *esc_victim   = sql_escape_string(w->victim ? w->victim : "");
		int new_pos = batch_append(batch, bpos, sizeof(batch),
		                           ";INSERT INTO player_witnesses (pid, crime, room_vnum, attacker_name, victim_name, witness_time) "
		                           "VALUES (%d, %d, %d, '%s', '%s', FROM_UNIXTIME(NULLIF(%ld,0)))",
		                           pid,
		                           w->crime,
		                           w->room,
		                           esc_attacker ? esc_attacker : "",
		                           esc_victim ? esc_victim : "",
		                           (long)w->time);
		free(esc_attacker);
		free(esc_victim);
		if (new_pos < 0)
		{
			if (bpos > 0 && !sql_run_multi_query(batch))
			{
				if (own_txn) sql_rollback();
				return false;
			}
			batch[0] = '\0';
			bpos = 0;
			new_pos = batch_append(batch, bpos, sizeof(batch),
			                       "INSERT INTO player_witnesses (pid, crime, room_vnum, attacker_name, victim_name, witness_time) "
			                       "VALUES (%d, %d, %d, '%s', '%s', FROM_UNIXTIME(NULLIF(%ld,0)))",
			                       pid,
			                       w->crime,
			                       w->room,
			                       esc_attacker ? esc_attacker : "",
			                       esc_victim ? esc_victim : "",
			                       (long)w->time);
			if (new_pos < 0)
			{
				if (own_txn) sql_rollback();
				return false;
			}
		}
		bpos = new_pos;
	}

	if (bpos > 0 && !sql_run_multi_query(batch))
	{
		if (own_txn)
			sql_rollback();
		return false;
	}

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return false; }
	}
	return true;
}

// shapechange save/load

bool sql_save_player_shapechanges(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	// Start own transaction if not already in one
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	int pid = GET_PID(ch);
	if (pid <= 0)
	{
		if (own_txn) sql_rollback();
		return false;
	}

	// DELETE + INSERT batch in one multi-statement round-trip.
	char batch[24576];
	int  bpos  = snprintf(batch, sizeof(batch),
	                       "DELETE FROM player_shapechanges WHERE pid=%d", pid);

	// insert current shapechanges, flushing as needed
	if (has_innate(ch, INNATE_SHAPECHANGE) && ch->only.pc->knownShapes)
	{
		for (struct char_shapechange_data *shape = ch->only.pc->knownShapes; shape; shape = shape->next)
		{
			int new_pos = batch_append(batch, bpos, sizeof(batch),
			                           ";INSERT INTO player_shapechanges (pid, mob_vnum, times_researched, last_researched, last_shapechanged) "
			                           "VALUES (%d, %d, %d, FROM_UNIXTIME(NULLIF(%ld,0)), FROM_UNIXTIME(NULLIF(%ld,0)))",
			                           pid,
			                           shape->mobVnum,
			                           shape->timesResearched,
			                           (long)shape->lastResearched,
			                           (long)shape->lastShapechanged);
			if (new_pos < 0)
			{
				if (bpos > 0 && !sql_run_multi_query(batch))
				{
					if (own_txn) sql_rollback();
					return false;
				}
				batch[0] = '\0';
				bpos = 0;
				new_pos = batch_append(batch, bpos, sizeof(batch),
				                       "INSERT INTO player_shapechanges (pid, mob_vnum, times_researched, last_researched, last_shapechanged) "
				                       "VALUES (%d, %d, %d, FROM_UNIXTIME(NULLIF(%ld,0)), FROM_UNIXTIME(NULLIF(%ld,0)))",
				                       pid,
				                       shape->mobVnum,
				                       shape->timesResearched,
				                       (long)shape->lastResearched,
				                       (long)shape->lastShapechanged);
				if (new_pos < 0)
				{
					if (own_txn) sql_rollback();
					return false;
				}
			}
			bpos = new_pos;
		}
	}

	if (bpos > 0 && !sql_run_multi_query(batch))
	{
		if (own_txn)
			sql_rollback();
		return false;
	}

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return false; }
	}
	return true;
}

bool sql_load_player_shapechanges(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	int pid = GET_PID(ch);
	if (pid <= 0)
		return false;

	// only load if character has shapechange innate
	if (!has_innate(ch, INNATE_SHAPECHANGE))
		return true;

	// clear existing shapes (defined in files.c)
	extern void delete_knownShapes(P_char ch);
	if (ch->only.pc->knownShapes)
		delete_knownShapes(ch);

	char query[256];
	snprintf(query,
	         sizeof(query),
	         "SELECT mob_vnum, times_researched, UNIX_TIMESTAMP(last_researched), UNIX_TIMESTAMP(last_shapechanged) "
	         "FROM player_shapechanges WHERE pid=%d ORDER BY id",
	         pid);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	struct char_shapechange_data **ppShape = &(ch->only.pc->knownShapes);
	MYSQL_ROW                      row;

	while ((row = mysql_fetch_row(result)))
	{
		int vnum = atoi(row[0]);

		// ensure vnum exists
		if (!real_mobile(vnum))
			continue;

		struct char_shapechange_data *shape;
		CREATE(shape, char_shapechange_data, 1, MEM_TAG_SHPCHNG);
		shape->mobVnum          = vnum;
		shape->timesResearched  = atoi(row[1]);
		shape->lastResearched   = atol(row[2]);
		shape->lastShapechanged = atol(row[3]);
		shape->next             = NULL;

		*ppShape = shape;
		ppShape  = &(shape->next);
	}

	mysql_free_result(result);
	return true;
}

// recipe save/load

bool sql_save_player_recipes(P_char ch)
{
	// No guard: this function is a no-op (recipes are saved individually via
	// sql_add_player_recipe() when learned, not in bulk). It is intentionally
	// safe to call outside a transaction - no SQL queries are issued.
	// Still called by sql_save_player() so the transaction chain is complete.
	(void)ch;
	return true;
}

bool sql_add_player_recipe(int pid, int recipe_vnum)
{
	if (!DB || pid <= 0)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "INSERT IGNORE INTO player_recipes (pid, recipe_vnum) VALUES (%d, %d)", pid, recipe_vnum);
	return sql_run_query(query);
}

bool sql_delete_player_recipes(int pid)
{
	if (!DB || pid <= 0)
		return false;

	char query[128];
	snprintf(query, sizeof(query), "DELETE FROM player_recipes WHERE pid=%d", pid);
	return sql_run_query(query);
}

bool sql_has_player_recipe(int pid, int recipe_vnum)
{
	if (!DB || pid <= 0)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "SELECT 1 FROM player_recipes WHERE pid=%d AND recipe_vnum=%d LIMIT 1", pid, recipe_vnum);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	bool has = (mysql_fetch_row(result) != NULL);
	mysql_free_result(result);
	return has;
}

// returns array of recipe vnums, sets count. caller must free array
int *sql_get_player_recipes(int pid, int *count)
{
	*count = 0;
	if (!DB || pid <= 0)
		return NULL;

	char query[256];
	snprintf(query, sizeof(query), "SELECT recipe_vnum FROM player_recipes WHERE pid=%d ORDER BY id", pid);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	int num_rows = mysql_num_rows(result);
	if (num_rows == 0)
	{
		mysql_free_result(result);
		return NULL;
	}

	int *recipes = (int *)malloc(num_rows * sizeof(int));
	if (!recipes)
	{
		mysql_free_result(result);
		return NULL;
	}

	MYSQL_ROW row;
	int       i = 0;
	while ((row = mysql_fetch_row(result)))
	{
		recipes[i++] = atoi(row[0]);
	}

	mysql_free_result(result);
	*count = i;
	return recipes;
}

// player load functions

// helper to safely get int from row, returns default if null
static int sql_row_int(MYSQL_ROW row, int idx, int def) { return (row && row[idx]) ? atoi(row[idx]) : def; }

// helper to safely get long from row
static long sql_row_long(MYSQL_ROW row, int idx, long def) { return (row && row[idx]) ? atol(row[idx]) : def; }

// helper to safely get ulong from row
static unsigned long sql_row_ulong(MYSQL_ROW row, int idx, unsigned long def) { return (row && row[idx]) ? strtoul(row[idx], NULL, 10) : def; }

// helper to duplicate string from row (uses tracked memory)
static char *sql_row_str(MYSQL_ROW row, int idx)
{
	if (!row || !row[idx])
		return NULL;
	return str_dup(row[idx]);
}

bool sql_load_player_status(P_char ch, int pid)
{
	if (!ch || !DB || pid <= 0)
		return false;

	char query[2048];
	snprintf(query,
	         sizeof(query),
	         "SELECT name, short_descr, long_descr, description, title, "
	         "m_class, secondary_class, spec, race, racewar, level, sex, "
	         "weight, height, size, hometown, birthplace, orig_birthplace, last_room, "
	         "UNIX_TIMESTAMP(birth_time), played_time, UNIX_TIMESTAMP(last_save), perm_aging, "
	         "base_str, base_dex, base_agi, base_con, base_pow, "
	         "base_int, base_wis, base_cha, base_kar, base_luk, "
	         "mana, base_mana, hit_diff, base_hit, vitality, base_vitality, spells_memmed_extra, "
	         "copper, silver, gold, platinum, bank_copper, bank_silver, bank_gold, bank_platinum, "
	         "exp, epics, epic_skill_points, skillpoints, spell_bind_used, "
	         "act, act2, act3, vote, alignment,prestige, assoc_id, guild_status, "
	         "UNIX_TIMESTAMP(time_left_guild), nb_left_guild, UNIX_TIMESTAMP(time_unspecced), frags, oldfrags, numb_deaths,"
	         "condition_0, condition_1, condition_2, condition_3, condition_4, "
	         "poof_in, poof_out, poof_in_sound, poof_out_sound, "
	         "echo_toggle, prompt, wiz_invis, law_flags, wimpy, aggressive, highest_level, screen_length, "
	         "quest_active, quest_mob_vnum, quest_type, quest_accomplished, "
	         "quest_started, quest_zone_number, quest_giver, quest_level, "
	         "quest_receiver, quest_shares_left, quest_kill_how_many, "
	         "quest_kill_original, quest_map_room, quest_map_bought, last_ip "
	         "FROM player_data WHERE pid=%d",
	         pid);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		return false;
	}

	int col = 0;

	// name and descriptions
	GET_NAME(ch)           = sql_row_str(row, col++);
	ch->player.short_descr = sql_row_str(row, col++);
	ch->player.long_descr  = sql_row_str(row, col++);
	ch->player.description = sql_row_str(row, col++);
	GET_TITLE(ch)          = sql_row_str(row, col++);

	// class/race/level
	ch->player.m_class         = sql_row_int(row, col++, 0);
	ch->player.secondary_class = sql_row_int(row, col++, 0);
	ch->player.spec            = sql_row_int(row, col++, 0);
	GET_RACE(ch)               = sql_row_int(row, col++, 0);
	GET_RACEWAR(ch)            = sql_row_int(row, col++, 0);
	ch->player.level           = sql_row_int(row, col++, 1);
	GET_SEX(ch)                = sql_row_int(row, col++, 0);

	// physical
	ch->player.weight = sql_row_int(row, col++, 0);
	ch->player.height = sql_row_int(row, col++, 0);
	GET_SIZE(ch)      = sql_row_int(row, col++, 0);

	// location
	GET_HOME(ch)             = sql_row_int(row, col++, 0);
	GET_BIRTHPLACE(ch)       = sql_row_int(row, col++, 0);
	GET_ORIG_BIRTHPLACE(ch)  = sql_row_int(row, col++, 0);
	int last_room_vnum       = sql_row_int(row, col++, 0);
	ch->specials.was_in_room = last_room_vnum;            // vnum for nanny.c placement
	ch->in_room              = real_room(last_room_vnum); // rnum as fallback
	if (ch->in_room != NOWHERE && IS_ROOM(ch->in_room, ROOM_LOCKER))
	{
		int locker_room = ch->in_room;
		int exit_room   = NOWHERE;

		if (world[locker_room].dir_option[0] && world[locker_room].dir_option[0]->to_room != NOWHERE)
			exit_room = world[locker_room].dir_option[0]->to_room;
		else if (GET_HOME(ch))
		{
			int home = real_room(GET_HOME(ch));
			if (home != NOWHERE)
				exit_room = home;
		}

		if (exit_room == NOWHERE && GET_BIRTHPLACE(ch))
		{
			int birth = real_room(GET_BIRTHPLACE(ch));
			if (birth != NOWHERE)
				exit_room = birth;
		}

		if (exit_room != NOWHERE)
		{
			logit(LOG_DEBUG,
			      "sql_load_player_status: redirecting %s out of locker room %d(%s) to %d(%s)",
			      GET_NAME(ch),
			      locker_room,
			      (world[locker_room].name) ? world[locker_room].name : "<unnamed>",
			      exit_room,
			      (world[exit_room].name) ? world[exit_room].name : "<unnamed>");
			ch->specials.was_in_room = world[exit_room].number;
			ch->in_room              = exit_room;
		}
	}

	// time
	ch->player.time.birth      = sql_row_long(row, col++, 0);
	ch->player.time.played     = sql_row_int(row, col++, 0);
	ch->player.time.saved      = sql_row_long(row, col++, 0);
	ch->player.time.logon      = time(0);
	ch->player.time.perm_aging = sql_row_int(row, col++, 0);

	// base stats
	ch->base_stats.Str = sql_row_int(row, col++, 0);
	ch->base_stats.Dex = sql_row_int(row, col++, 0);
	ch->base_stats.Agi = sql_row_int(row, col++, 0);
	ch->base_stats.Con = sql_row_int(row, col++, 0);
	ch->base_stats.Pow = sql_row_int(row, col++, 0);
	ch->base_stats.Int = sql_row_int(row, col++, 0);
	ch->base_stats.Wis = sql_row_int(row, col++, 0);
	ch->base_stats.Cha = sql_row_int(row, col++, 0);
	ch->base_stats.Kar = sql_row_int(row, col++, 0);
	ch->base_stats.Luk = sql_row_int(row, col++, 0);

	// points
	GET_MANA(ch)                           = sql_row_int(row, col++, 0);
	ch->points.base_mana                   = sql_row_int(row, col++, 0);
	int hit_diff                           = sql_row_int(row, col++, 0);
	ch->points.base_hit                    = sql_row_int(row, col++, 0);
	GET_VITALITY(ch)                       = sql_row_int(row, col++, 0);
	ch->points.base_vitality               = sql_row_int(row, col++, 0);
	ch->only.pc->spells_memmed[MAX_CIRCLE] = sql_row_int(row, col++, 0);

	// money
	GET_COPPER(ch)   = sql_row_int(row, col++, 0);
	GET_SILVER(ch)   = sql_row_int(row, col++, 0);
	GET_GOLD(ch)     = sql_row_int(row, col++, 0);
	GET_PLATINUM(ch) = sql_row_int(row, col++, 0);
	// skip old player bank columns (still in db for backup)
	// bank is loaded from account_banks after descriptor is set
	col += 4;
	GET_BALANCE_COPPER(ch)   = 0;
	GET_BALANCE_SILVER(ch)   = 0;
	GET_BALANCE_GOLD(ch)     = 0;
	GET_BALANCE_PLATINUM(ch) = 0;

	// experience
	GET_EXP(ch)                    = sql_row_int(row, col++, 0);
	ch->only.pc->epics             = sql_row_long(row, col++, 0);
	ch->only.pc->epic_skill_points = sql_row_long(row, col++, 0);
	ch->only.pc->skillpoints       = sql_row_int(row, col++, 0);
	ch->only.pc->spell_bind_used   = sql_row_long(row, col++, 0);

	// flags
	ch->specials.act       = sql_row_ulong(row, col++, 0);
	ch->specials.act2      = sql_row_ulong(row, col++, 0);
	ch->specials.act3      = sql_row_ulong(row, col++, 0);
	ch->only.pc->vote      = sql_row_ulong(row, col++, 0);
	ch->specials.alignment = sql_row_int(row, col++, 0);
	ch->only.pc->prestige  = sql_row_int(row, col++, 0);
	int assoc_id           = sql_row_int(row, col++, 0);
	if (assoc_id > 0)
		ch->specials.guild = get_guild_from_id(assoc_id);
	ch->specials.guild_status    = sql_row_int(row, col++, 0);
	ch->only.pc->time_left_guild = sql_row_long(row, col++, 0);
	ch->only.pc->nb_left_guild   = sql_row_int(row, col++, 0);
	ch->only.pc->time_unspecced  = sql_row_long(row, col++, 0);
	ch->only.pc->frags           = sql_row_long(row, col++, 0);
	ch->only.pc->oldfrags        = sql_row_long(row, col++, 0);
	ch->only.pc->numb_deaths     = sql_row_ulong(row, col++, 0);

	// conditions
	ch->specials.conditions[0] = sql_row_int(row, col++, 0);
	ch->specials.conditions[1] = sql_row_int(row, col++, 0);
	ch->specials.conditions[2] = sql_row_int(row, col++, 0);
	ch->specials.conditions[3] = sql_row_int(row, col++, 0);
	ch->specials.conditions[4] = sql_row_int(row, col++, 0);

	// immortal stuff
	ch->only.pc->poofIn        = sql_row_str(row, col++);
	ch->only.pc->poofOut       = sql_row_str(row, col++);
	col++;
	col++;
	ch->only.pc->echo_toggle   = sql_row_int(row, col++, 0);
	ch->only.pc->prompt        = sql_row_int(row, col++, 0);
	ch->only.pc->wiz_invis     = sql_row_long(row, col++, 0);
	col++;
	ch->only.pc->wimpy         = sql_row_int(row, col++, 0);
	ch->only.pc->aggressive    = sql_row_int(row, col++, -1);
	ch->only.pc->highest_level = sql_row_int(row, col++, 0);
	ch->only.pc->screen_length = sql_row_int(row, col++, 24);

	// quest data
	ch->only.pc->quest_active        = sql_row_int(row, col++, 0);
	ch->only.pc->quest_mob_vnum      = sql_row_int(row, col++, 0);
	ch->only.pc->quest_type          = sql_row_int(row, col++, 0);
	ch->only.pc->quest_accomplished  = sql_row_int(row, col++, 0);
	ch->only.pc->quest_started       = sql_row_int(row, col++, 0);
	ch->only.pc->quest_zone_number   = sql_row_int(row, col++, 0);
	ch->only.pc->quest_giver         = sql_row_int(row, col++, 0);
	ch->only.pc->quest_level         = sql_row_int(row, col++, 0);
	ch->only.pc->quest_receiver      = sql_row_int(row, col++, 0);
	ch->only.pc->quest_shares_left   = sql_row_int(row, col++, 0);
	ch->only.pc->quest_kill_how_many = sql_row_int(row, col++, 0);
	ch->only.pc->quest_kill_original = sql_row_int(row, col++, 0);
	ch->only.pc->quest_map_room      = sql_row_int(row, col++, 0);
	ch->only.pc->quest_map_bought    = sql_row_int(row, col++, 0);
	ch->only.pc->last_ip             = sql_row_ulong(row, col++, 0);

	mysql_free_result(result);

	// set pid
	ch->only.pc->pid = pid;

	// set position to standing/alive (will be properly set when entering game)
	SET_POS(ch, POS_STANDING + STAT_NORMAL);

	// calculate hit from hit_diff
	GET_HIT(ch) = GET_MAX_HIT(ch) - hit_diff;

	// load array data: languages, intros, timers, undead slots, forged items, granted cmds

	// languages
	snprintf(query, sizeof(query), "SELECT tongue_id, proficiency FROM player_languages WHERE pid=%d", pid);
	result = db_query("%s", query);
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int tongue = sql_row_int(row, 0, 0);
			if (tongue >= 0 && tongue < MAX_TONGUE)
				GET_LANGUAGE(ch, tongue) = sql_row_int(row, 1, 0);
		}
		mysql_free_result(result);
	}

	// intros
	snprintf(query, sizeof(query), "SELECT intro_index, intro_pid, UNIX_TIMESTAMP(intro_time) FROM player_intros WHERE pid=%d", pid);
	result = db_query("%s", query);
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int idx = sql_row_int(row, 0, 0);
			if (idx >= 0 && idx < MAX_INTRO)
			{
				ch->only.pc->introd_list[idx]  = sql_row_long(row, 1, 0);
				ch->only.pc->introd_times[idx] = sql_row_ulong(row, 2, 0);
			}
		}
		mysql_free_result(result);
	}

	// timers
	snprintf(query, sizeof(query), "SELECT timer_id, UNIX_TIMESTAMP(timer_value) FROM player_timers WHERE pid=%d", pid);
	result = db_query("%s", query);
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int idx = sql_row_int(row, 0, 0);
			if (idx >= 0 && idx < NUMB_PC_TIMERS)
				ch->only.pc->pc_timer[idx] = sql_row_long(row, 1, 0);
		}
		mysql_free_result(result);
	}

	// undead slots
	snprintf(query, sizeof(query), "SELECT circle, slots FROM player_undead_slots WHERE pid=%d", pid);
	result = db_query("%s", query);
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int circle = sql_row_int(row, 0, 0);
			if (circle >= 0 && circle <= MAX_CIRCLE)
				ch->specials.undead_spell_slots[circle] = sql_row_int(row, 1, 0);
		}
		mysql_free_result(result);
	}

	// forged items
	snprintf(query, sizeof(query), "SELECT forge_index, item_vnum FROM player_forged_items WHERE pid=%d", pid);
	result = db_query("%s", query);
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int idx = sql_row_int(row, 0, 0);
			if (idx >= 0 && idx < MAX_FORGE_ITEMS)
				ch->only.pc->learned_forged_list[idx] = sql_row_long(row, 1, 0);
		}
		mysql_free_result(result);
	}

	// granted commands - count first, then allocate and load
	snprintf(query, sizeof(query), "SELECT COUNT(*) FROM player_granted_cmds WHERE pid=%d", pid);
	result = db_query("%s", query);
	if (result)
	{
		row           = mysql_fetch_row(result);
		int cmd_count = sql_row_int(row, 0, 0);
		mysql_free_result(result);

		if (cmd_count > 0)
		{
			ch->only.pc->gcmd_arr = (int *)malloc(cmd_count * sizeof(int));
			if (ch->only.pc->gcmd_arr)
			{
				ch->only.pc->numb_gcmd = 0;
				snprintf(query, sizeof(query), "SELECT cmd_num FROM player_granted_cmds WHERE pid=%d ORDER BY id", pid);
				result = db_query("%s", query);
				if (result)
				{
					while ((row = mysql_fetch_row(result)) && ch->only.pc->numb_gcmd < cmd_count)
					{
						ch->only.pc->gcmd_arr[ch->only.pc->numb_gcmd++] = sql_row_int(row, 0, 0);
					}
					mysql_free_result(result);
				}
			}
		}
	}

	return true;
}

bool sql_load_player_skills(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	int pid = GET_PID(ch);
	if (pid <= 0)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "SELECT skill_id, learned, taught FROM player_skills WHERE pid=%d", pid);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		int skill_id = sql_row_int(row, 0, 0);
		if (skill_id >= 0 && skill_id < MAX_SKILLS)
		{
			ch->only.pc->skills[skill_id].learned = sql_row_int(row, 1, 0);
			ch->only.pc->skills[skill_id].taught  = sql_row_int(row, 2, 0);
		}
	}
	mysql_free_result(result);

	return true;
}

bool sql_load_player_affects(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	int pid = GET_PID(ch);
	if (pid <= 0)
		return false;

	char query[512];
	snprintf(query,
	         sizeof(query),
	         "SELECT type, duration, flags, modifier, location, level, "
	         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
	         "custom_msg_char, custom_msg_room "
	         "FROM player_affects WHERE pid=%d",
	         pid);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		struct affected_type af;
		memset(&af, 0, sizeof(af));

		af.type       = sql_row_int(row, 0, 0);
		af.duration   = sql_row_int(row, 1, 0);
		af.flags      = sql_row_int(row, 2, 0);
		af.modifier   = sql_row_int(row, 3, 0);
		af.location   = sql_row_int(row, 4, 0);
		af.level      = sql_row_int(row, 5, 0);
		af.bitvector  = sql_row_ulong(row, 6, 0);
		af.bitvector2 = sql_row_ulong(row, 7, 0);
		af.bitvector3 = sql_row_ulong(row, 8, 0);
		af.bitvector4 = sql_row_ulong(row, 9, 0);
		af.bitvector5 = sql_row_ulong(row, 10, 0);
		char *wear_off_char = sql_row_str(row, 11);
		char *wear_off_room = sql_row_str(row, 12);
		if (af.type == SKILL_DIAMOND_SOUL && af.location == APPLY_SAVING_PARA)
			af.wear_off_message_index = 1;

		if (wear_off_char || wear_off_room)
			affect_to_char_with_messages(ch, &af, wear_off_char, wear_off_room);
		else
			affect_to_char(ch, &af);
		free(wear_off_char);
		free(wear_off_room);
	}
	mysql_free_result(result);

	return true;
}

bool sql_load_player_items(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
	{
		return false;
	}

	int pid = GET_PID(ch);
	if (pid <= 0)
		return false;
	char owner_ref[32];
	snprintf(owner_ref, sizeof(owner_ref), "%d", pid);

	trace_append_file("sql_load_player_items begin name=%s pid=%d\n", GET_NAME(ch), pid);

	logit(LOG_FILE, "[sql_load_player_items] begin name=%s pid=%d", GET_NAME(ch), pid);

	// first, load all items into a temp array indexed by db id
	// then resolve container relationships

	char query[1024];
	snprintf(query,
         sizeof(query),
         "SELECT id, vnum, equip_slot, container_id, "
         "weight, cost, timer, extra_flags, wear_flags, item_type, "
         "value0, value1, value2, value3, value4, value5, value6, value7, "
         "name, short_descr, description, action_descr, "
         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
         "item_material, obj_uid, item_condition "
         "FROM player_items WHERE pid=%d ORDER BY id",
         pid);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
	{
		logit(LOG_FILE, "[sql_load_player_items] query failed name=%s pid=%d", GET_NAME(ch), pid);
		return false;
	}

	// count rows
	int num_rows = mysql_num_rows(result);
	if (num_rows == 0)
	{
		mysql_free_result(result);
		return true; // no items is valid
	}

	// allocate temp arrays
	P_obj *items         = (P_obj *)calloc(num_rows, sizeof(P_obj));
	int   *item_ids      = (int *)calloc(num_rows, sizeof(int));
	int   *container_ids = (int *)calloc(num_rows, sizeof(int));
	int   *equip_slots   = (int *)calloc(num_rows, sizeof(int));

	int       idx = 0;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) && idx < num_rows)
	{
		int col          = 0;
		int db_id        = sql_row_int(row, col++, 0);
		int vnum         = sql_row_int(row, col++, 0);
		int equip_slot   = sql_row_int(row, col++, 0);
		int container_id = sql_row_int(row, col++, 0);
		if (idx < 12 || equip_slot > 0)
		{
			char trace[MAX_STRING_LENGTH];
			snprintf(trace,
			         sizeof(trace),
			         "&+w[TRACE]&n row idx=%d db_id=%d vnum=%d equip=%d container=%d\r\n",
			         idx,
			         db_id,
			         vnum,
			         equip_slot,
			         container_id);
			logit(LOG_FILE, "%s", trace);
		}
		logit(LOG_FILE, "[sql_load_player_items] row pid=%d db_id=%d vnum=%d equip=%d container=%d", pid, db_id, vnum, equip_slot, container_id);

		// create object from prototype
		P_obj obj = read_object(vnum, VIRTUAL);
		if (!obj)
		{
			logit(LOG_DEBUG, "sql_load_player_items: failed to load vnum %d for %s", vnum, GET_NAME(ch));
			idx++;
			continue;
		}

		// override saved properties
		obj->weight      = sql_row_int(row, col++, obj->weight);
		obj->cost        = sql_row_int(row, col++, obj->cost);
		obj->timer[0]    = sql_row_long(row, col++, obj->timer[0]);
		obj->extra_flags = sql_row_ulong(row, col++, obj->extra_flags);
		obj->wear_flags  = sql_row_int(row, col++, obj->wear_flags);
		obj->type        = sql_validate_loaded_item_type(obj, sql_row_int(row, col++, obj->type), "sql_load_player_items");

		// NULL in db means use prototype value (passed as default)
		obj->value[0] = sql_row_int(row, col++, obj->value[0]);
		obj->value[1] = sql_row_int(row, col++, obj->value[1]);
		obj->value[2] = sql_row_int(row, col++, obj->value[2]);
		obj->value[3] = sql_row_int(row, col++, obj->value[3]);
		obj->value[4] = sql_row_int(row, col++, obj->value[4]);
		obj->value[5] = sql_row_int(row, col++, obj->value[5]);
		obj->value[6] = sql_row_int(row, col++, obj->value[6]);
		obj->value[7] = sql_row_int(row, col++, obj->value[7]);

		// strung strings (if not NULL, replace prototype)
		char *str_name   = sql_row_str(row, col++);
		char *str_short  = sql_row_str(row, col++);
		char *str_desc   = sql_row_str(row, col++);
		char *str_action = sql_row_str(row, col++);

		if (str_name)
		{
			obj->name = str_name;
			obj->str_mask |= STRUNG_KEYS;
		}
		if (str_short)
		{
			obj->short_description = str_short;
			obj->str_mask |= STRUNG_DESC2;
		}
		if (str_desc)
		{
			obj->description = str_desc;
			obj->str_mask |= STRUNG_DESC1;
		}
		if (str_action)
		{
			obj->action_description = str_action;
			obj->str_mask |= STRUNG_DESC3;
		}

		// restore bitvectors and item_material (NULL in db means use prototype value)
		obj->bitvector  = sql_row_ulong(row, col++, obj->bitvector);
		obj->bitvector2 = sql_row_ulong(row, col++, obj->bitvector2);
		obj->bitvector3 = sql_row_ulong(row, col++, obj->bitvector3);
		obj->bitvector4 = sql_row_ulong(row, col++, obj->bitvector4);
		obj->bitvector5 = sql_row_ulong(row, col++, obj->bitvector5);
		obj->material   = sql_row_int(row, col++, obj->material);

		// restore obj_uid and condition
		unsigned long saved_uid = sql_row_ulong(row, col++, 0);
		if (saved_uid > 0)
			obj->obj_uid = saved_uid;
		if (!sql_persistence_item_owner_matches(saved_uid, "player", owner_ref, "sql_load_player_items"))
		{
			{
				char trace[MAX_STRING_LENGTH];
				snprintf(trace, sizeof(trace), "&+w[TRACE]&n skip db_id=%d vnum=%d uid=%lu owner_mismatch\r\n", db_id, vnum, saved_uid);
				logit(LOG_FILE, "%s", trace);
			}
			logit(LOG_FILE, "[sql_load_player_items] skip db_id=%d vnum=%d uid=%lu owner_mismatch", db_id, vnum, saved_uid);
			extract_obj(obj, FALSE);
			continue;
		}
		obj->condition = sql_row_int(row, col++, obj->condition);

		// store db id for incremental saves
		obj->db_item_id = db_id;

		items[idx]         = obj;
		item_ids[idx]      = db_id;
		container_ids[idx] = container_id;
		equip_slots[idx]   = equip_slot;
		if (idx < 12 || equip_slot > 0)
		{
			char trace[MAX_STRING_LENGTH];
			snprintf(trace,
			         sizeof(trace),
			         "&+w[TRACE]&n loaded db_id=%d vnum=%d short=%s\r\n",
			         db_id,
			         vnum,
			         obj->short_description ? obj->short_description : "(null)");
			logit(LOG_FILE, "%s", trace);
		}
		logit(LOG_FILE, "[sql_load_player_items] loaded db_id=%d vnum=%d uid=%lu short=%s", db_id, vnum, saved_uid, obj->short_description ? obj->short_description : "(null)");
		idx++;
	}
	mysql_free_result(result);

	int loaded_count = idx;

	// load all item affects in one query (was N+1 queries, now 1)
	// track which items have had their prototype affects cleared
	bool *affects_cleared = (bool *)calloc(num_rows, sizeof(bool));

	snprintf(query,
	         sizeof(query),
	         "SELECT ia.item_id, ia.location, ia.modifier "
	         "FROM player_item_affects ia "
	         "JOIN player_items pi ON ia.item_id = pi.id "
	         "WHERE pi.pid=%d ORDER BY ia.item_id, ia.id",
	         pid);
	result = db_query("%s", query);
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int affect_item_id = sql_row_int(row, 0, 0);
			int location       = sql_row_int(row, 1, 0);
			int modifier       = sql_row_int(row, 2, 0);

			// find the item in our array and add the affect
			for (int i = 0; i < loaded_count; i++)
			{
				if (item_ids[i] == affect_item_id && items[i])
				{
					// clear prototype affects before adding first db affect
					if (!affects_cleared[i])
					{
						for (int a = 0; a < MAX_OBJ_AFFECT; a++)
						{
							items[i]->affected[a].location = 0;
							items[i]->affected[a].modifier = 0;
						}
						affects_cleared[i] = true;
					}

					// skip if this location+modifier already exists (db has duplicates)
					bool is_dup = false;
					for (int a = 0; a < MAX_OBJ_AFFECT; a++)
					{
						if (items[i]->affected[a].location == location && items[i]->affected[a].modifier == modifier)
						{
							is_dup = true;
							break;
						}
					}
					if (is_dup)
						break;

					// find next empty affect slot
					for (int a = 0; a < MAX_OBJ_AFFECT; a++)
					{
						if (items[i]->affected[a].location == 0 && items[i]->affected[a].modifier == 0)
						{
							items[i]->affected[a].location = location;
							items[i]->affected[a].modifier = modifier;
							break;
						}
					}
					break;
				}
			}
		}
		mysql_free_result(result);
	}
	free(affects_cleared);

	// load extra descriptions (spellbooks etc)
	snprintf(query,
	         sizeof(query),
	         "SELECT ed.item_id, ed.keyword, ed.description "
	         "FROM player_item_extra_descr ed "
	         "JOIN player_items pi ON ed.item_id = pi.id "
	         "WHERE pi.pid=%d ORDER BY ed.item_id",
	         pid);

	result = db_query("%s", query);
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int db_id = atoi(row[0]);

			P_obj obj = NULL;
			for (int i = 0; i < loaded_count; i++)
			{
				if (item_ids[i] == db_id && items[i])
				{
					obj = items[i];
					break;
				}
			}
			if (!obj)
				continue;

			struct extra_descr_data *ed;
			CREATE(ed, extra_descr_data, 1, MEM_TAG_EXDESCD);

			if (row[1] && strcmp(row[1], "SPELLBOOK") == 0)
			{
				CREATE(ed->keyword, char, 4, MEM_TAG_STRING);
				ed->keyword[0] = 3;
				ed->keyword[1] = 1;
				ed->keyword[2] = 3;
				ed->keyword[3] = '\0';

				size_t buflen = (MAX_SKILLS + 1) / 8 + 1;
				CREATE(ed->description, char, buflen, MEM_TAG_STRING);
				json_to_spellbook(row[2], ed->description);
			}
			else
			{
				ed->keyword     = row[1] ? str_dup(row[1]) : str_dup("");
				ed->description = row[2] ? str_dup(row[2]) : NULL;
			}

			ed->next            = obj->ex_description;
			obj->ex_description = ed;
			obj->str_mask |= STRUNG_EDESC;
		}
		mysql_free_result(result);
	}

	// place items in containers using linear search
	for (int i = 0; i < loaded_count; i++)
	{
		if (!items[i] || container_ids[i] == 0)
			continue;

		// find container by searching loaded items
		for (int j = 0; j < loaded_count; j++)
		{
			if (item_ids[j] == container_ids[i] && items[j])
			{
				obj_to_obj(items[i], items[j]);
				break;
			}
		}
	}

	for (int j = 0; j < loaded_count; j++)
	{
		if (items[j])
		{
			recalc_container_weight(items[j]);
		}
	}

	// second pass - put top-level items on character
	for (int i = 0; i < loaded_count; i++)
	{
		if (!items[i] || container_ids[i] != 0)
			continue;

		if (equip_slots[i] > 0 && equip_slots[i] <= MAX_WEAR)
		{
			// equipment slot (1-indexed in db, 0-indexed in array)
			int slot = equip_slots[i] - 1;
			if (!ch->equipment[slot])
			{
				{
					char trace[MAX_STRING_LENGTH];
					snprintf(trace, sizeof(trace), "&+w[TRACE]&n equip db_id=%d vnum=%d slot=%d\r\n", item_ids[i], OBJ_VNUM(items[i]), slot);
					logit(LOG_FILE, "%s", trace);
				}
				logit(LOG_FILE, "[sql_load_player_items] equip db_id=%d vnum=%d slot=%d", item_ids[i], OBJ_VNUM(items[i]), slot);
				equip_char(ch, items[i], slot, 0);
			}
			else
			{
				{
					char trace[MAX_STRING_LENGTH];
					snprintf(trace, sizeof(trace), "&+w[TRACE]&n carry-instead-of-equip db_id=%d vnum=%d slot=%d occupied\r\n", item_ids[i], OBJ_VNUM(items[i]), slot);
					logit(LOG_FILE, "%s", trace);
				}
				logit(LOG_FILE, "[sql_load_player_items] carry-instead-of-equip db_id=%d vnum=%d slot=%d occupied", item_ids[i], OBJ_VNUM(items[i]), slot);
				obj_to_char(items[i], ch);
			}
		}
		else
		{
			// inventory
			if (i < 12)
			{
				char trace[MAX_STRING_LENGTH];
				snprintf(trace, sizeof(trace), "&+w[TRACE]&n carry db_id=%d vnum=%d\r\n", item_ids[i], OBJ_VNUM(items[i]));
				logit(LOG_FILE, "%s", trace);
			}
			logit(LOG_FILE, "[sql_load_player_items] carry db_id=%d vnum=%d", item_ids[i], OBJ_VNUM(items[i]));
			obj_to_char(items[i], ch);
		}
	}
	{
		char trace[MAX_STRING_LENGTH];
		snprintf(trace, sizeof(trace), "&+w[TRACE]&n load items done num_rows=%d kept=%d\r\n", num_rows, loaded_count);
		logit(LOG_FILE, "%s", trace);
	}
	logit(LOG_FILE, "[sql_load_player_items] done name=%s loaded=%d kept=%d", GET_NAME(ch), num_rows, loaded_count);

	free(items);
	free(item_ids);
	free(container_ids);
	free(equip_slots);

	return true;
}

bool sql_load_player_witnesses(P_char ch)
{
	if (!ch || !IS_PC(ch) || !DB)
		return false;

	int pid = GET_PID(ch);
	if (pid <= 0)
		return false;

	char query[256];
	snprintf(query,
	         sizeof(query),
	         "SELECT crime, room_vnum, attacker_name, victim_name, UNIX_TIMESTAMP(witness_time) "
	         "FROM player_witnesses WHERE pid=%d",
	         pid);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		wtns_rec *w = (wtns_rec *)malloc(sizeof(wtns_rec));
		if (!w)
			continue;

		memset(w, 0, sizeof(wtns_rec));
		w->crime    = sql_row_int(row, 0, 0);
		w->room     = sql_row_int(row, 1, 0);
		w->attacker = sql_row_str(row, 2);
		w->victim   = sql_row_str(row, 3);
		w->time     = sql_row_long(row, 4, 0);

		// prepend to list
		w->next                = ch->specials.witnessed;
		ch->specials.witnessed = w;
	}
	mysql_free_result(result);

	return true;
}

P_char sql_load_player(const char *name)
{
	if (!name || !DB)
		return NULL;

	// get pid first
	int pid = sql_get_player_pid(name);
	if (pid <= 0)
	{
		logit(LOG_DEBUG, "sql_load_player: player %s not found in db", name);
		return NULL;
	}

	// allocate character structure
	P_char ch = (P_char)malloc(sizeof(struct char_data));
	if (!ch)
		return NULL;
	memset(ch, 0, sizeof(struct char_data));

	// allocate pc_only_data
	ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
	if (!ch->only.pc)
	{
		free(ch);
		return NULL;
	}
	memset(ch->only.pc, 0, sizeof(struct pc_only_data));

	// IS_PC is defined as !IS_NPC, and IS_NPC checks ACT_ISNPC flag
	// since we memset to 0, the flag is not set, so this is already a PC

	// load all components
	if (!sql_load_player_status(ch, pid))
	{
		logit(LOG_DEBUG, "sql_load_player: failed to load status for %s", name);
		free(ch->only.pc);
		free(ch);
		return NULL;
	}

	if (!sql_load_player_skills(ch))
	{
		logit(LOG_DEBUG, "sql_load_player: failed to load skills for %s", name);
		// continue anyway, skills aren't fatal
	}

	if (!sql_load_player_affects(ch))
	{
		logit(LOG_DEBUG, "sql_load_player: failed to load affects for %s", name);
		// continue anyway
	}

	if (!sql_load_player_items(ch))
	{
		logit(LOG_DEBUG, "sql_load_player: failed to load items for %s", name);
		// continue anyway
	}

	if (!sql_load_player_witnesses(ch))
	{
		logit(LOG_DEBUG, "sql_load_player: failed to load witnesses for %s", name);
		// continue anyway
	}

	return ch;
}

static bool               sql_save_account_characters(struct acct_entry *acc);
static bool               sql_account_ips_query_failed       = false;
static bool               sql_account_chars_query_failed     = false;
static void               free_acct_ip_list(struct acct_ip *ips);
static void               free_acct_char_list(struct acct_chars *chars);
static struct acct_chars *sql_load_account_characters(const char *account_name);

bool sql_save_account(struct acct_entry *acc)
{
	if (!DB || !acc || !acc->acct_name)
		return false;

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	char *esc_name  = sql_escape_string(acc->acct_name);
	char *esc_email = sql_escape_string(acc->acct_email ? acc->acct_email : "");
	char *esc_pass  = sql_escape_string(acc->acct_password ? acc->acct_password : "");
	char *esc_conf  = sql_escape_string(acc->acct_confirmation ? acc->acct_confirmation : "");

	if (!esc_name || !esc_email || !esc_pass || !esc_conf)
	{
		if (esc_name)
			free(esc_name);
		if (esc_email)
			free(esc_email);
		if (esc_pass)
			free(esc_pass);
		if (esc_conf)
			free(esc_conf);
		return false;
	}

	char query[2048];
	snprintf(query,
	         sizeof(query),
	         "insert into accounts (account_name, email, password, confirmation_code, "
	         "confirmed, confirmation_sent, blocked, last_login, last_good_char, last_evil_char, "
	         "flags1, flags2, flags3, flags4) values ('%s', '%s', '%s', '%s', %d, %d, %d, FROM_UNIXTIME(NULLIF(%ld,0)), FROM_UNIXTIME(NULLIF(%ld,0)), FROM_UNIXTIME(NULLIF(%ld,0)), %lu, %lu, %lu, %lu) "
	         "on duplicate key update email='%s', password='%s', confirmation_code='%s', "
	         "confirmed=%d, confirmation_sent=%d, blocked=%d, last_login=FROM_UNIXTIME(NULLIF(%ld,0)), last_good_char=FROM_UNIXTIME(NULLIF(%ld,0)), last_evil_char=FROM_UNIXTIME(NULLIF(%ld,0)), "
	         "flags1=%lu, flags2=%lu, flags3=%lu, flags4=%lu",
	         esc_name,
	         esc_email,
	         esc_pass,
	         esc_conf,
	         acc->acct_confirmed,
	         acc->acct_confirmation_sent,
	         acc->acct_blocked,
	         acc->acct_last,
	         acc->acct_good,
	         acc->acct_evil,
	         acc->acct_flags1,
	         acc->acct_flags2,
	         acc->acct_flags3,
	         acc->acct_flags4,
	         esc_email,
	         esc_pass,
	         esc_conf,
	         acc->acct_confirmed,
	         acc->acct_confirmation_sent,
	         acc->acct_blocked,
	         acc->acct_last,
	         acc->acct_good,
	         acc->acct_evil,
	         acc->acct_flags1,
	         acc->acct_flags2,
	         acc->acct_flags3,
	         acc->acct_flags4);

	free(esc_name);
	free(esc_email);
	free(esc_pass);
	free(esc_conf);

	if (!sql_run_query(query))
	{
		if (own_txn)
			sql_rollback();
		return false;
	}

	// save ips
	if (!sql_save_account_ips(acc->acct_name, acc->acct_unique_ips))
	{
		logit(LOG_DEBUG, "sql_save_account: failed to save ips for %s", acc->acct_name);
		if (own_txn)
			sql_rollback();
		return false;
	}

	// save characters
	if (!sql_save_account_characters(acc))
	{
		logit(LOG_DEBUG, "sql_save_account: failed to save characters for %s", acc->acct_name);
		if (own_txn)
			sql_rollback();
		return false;
	}

	if (own_txn && !sql_commit())
	{
		sql_rollback();
		return false;
	}

	return true;
}

static bool sql_save_account_characters(struct acct_entry *acc)
{
	if (!DB || !acc || !acc->acct_name)
		return false;

	char *esc_name = sql_escape_string(acc->acct_name);
	if (!esc_name)
		return false;

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
		{
			free(esc_name);
			return false;
		}
		own_txn = true;
	}

	for (struct acct_chars *ch = acc->acct_character_list; ch; ch = ch->next)
	{
		if (!ch->charname)
			continue;

		char *esc_char = sql_escape_string(ch->charname);
		if (!esc_char)
			continue;

		int  pid = sql_get_player_pid(ch->charname);
		char pid_buf[32];
		const char *pid_sql = "NULL";
		if (pid > 0)
		{
			snprintf(pid_buf, sizeof(pid_buf), "%d", pid);
			pid_sql = pid_buf;
		}

		char query[512];
		snprintf(query,
		         sizeof(query),
		         "insert into account_characters (account_name, char_name, pid, login_count, last_login, blocked, racewar) "
		         "values ('%s', '%s', %s, %lu, FROM_UNIXTIME(NULLIF(%ld,0)), %d, %d) "
		         "on duplicate key update login_count=%lu, last_login=FROM_UNIXTIME(NULLIF(%ld,0)), blocked=%d, racewar=%d, deleted_at=NULL, pid=VALUES(pid), account_name=VALUES(account_name), char_name=VALUES(char_name)",
		         esc_name,
		         esc_char,
		         pid_sql,
		         ch->count,
		         ch->last,
		         ch->blocked,
		         ch->racewar,
		         ch->count,
		         ch->last,
		         ch->blocked,
		         ch->racewar);

		bool ok = sql_run_query(query);
		free(esc_char);
		if (!ok)
		{
			free(esc_name);
			if (own_txn)
				sql_rollback();
			return false;
		}
	}

	free(esc_name);
	if (own_txn && !sql_commit())
	{
		sql_rollback();
		return false;
	}
	return true;
}

struct acct_entry *sql_load_account(const char *name)
{
	if (!DB || !name)
		return NULL;

	char *esc_name = sql_escape_string(name);
	if (!esc_name)
		return NULL;

	char query[512];
	snprintf(query,
	         sizeof(query),
	         "select account_name, email, password, confirmation_code, confirmed, confirmation_sent, "
	         "blocked, last_login, last_good_char, last_evil_char, flags1, flags2, flags3, flags4 "
	         "from accounts where account_name='%s'",
	         esc_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
	{
		free(esc_name);
		return NULL;
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		free(esc_name);
		return NULL;
	}

	struct acct_entry *acc = (struct acct_entry *)malloc(sizeof(struct acct_entry));
	if (!acc)
	{
		mysql_free_result(result);
		free(esc_name);
		return NULL;
	}
	memset(acc, 0, sizeof(struct acct_entry));

	acc->acct_name              = str_dup(row[0] ? row[0] : "");
	acc->acct_email             = str_dup(row[1] ? row[1] : "");
	acc->acct_password          = str_dup(row[2] ? row[2] : "");
	acc->acct_confirmation      = str_dup(row[3] ? row[3] : "");
	acc->acct_confirmed         = row[4] ? atoi(row[4]) : 0;
	acc->acct_confirmation_sent = row[5] ? atoi(row[5]) : 0;
	acc->acct_blocked           = row[6] ? atoi(row[6]) : 0;
	acc->acct_last              = row[7] ? atol(row[7]) : 0;
	acc->acct_good              = row[8] ? atol(row[8]) : 0;
	acc->acct_evil              = row[9] ? atol(row[9]) : 0;
	acc->acct_flags1            = row[10] ? strtoul(row[10], NULL, 10) : 0;
	acc->acct_flags2            = row[11] ? strtoul(row[11], NULL, 10) : 0;
	acc->acct_flags3            = row[12] ? strtoul(row[12], NULL, 10) : 0;
	acc->acct_flags4            = row[13] ? strtoul(row[13], NULL, 10) : 0;

	mysql_free_result(result);

	// load ips
	sql_account_ips_query_failed = false;
	acc->acct_unique_ips        = sql_load_account_ips(name);
	if (sql_account_ips_query_failed)
	{
		free_acct_ip_list(acc->acct_unique_ips);
		free(esc_name);
		free(acc);
		return NULL;
	}
	acc->num_ips = 0;
	for (struct acct_ip *ip = acc->acct_unique_ips; ip; ip = ip->next)
		acc->num_ips++;

	// load characters
	sql_account_chars_query_failed = false;
	acc->acct_character_list       = sql_load_account_characters(acc->acct_name ? acc->acct_name : name);
	if (sql_account_chars_query_failed)
	{
		free_acct_ip_list(acc->acct_unique_ips);
		free_acct_char_list(acc->acct_character_list);
		free(esc_name);
		free(acc);
		return NULL;
	}
	acc->num_chars = 0;
	for (struct acct_chars *ch = acc->acct_character_list; ch; ch = ch->next)
		acc->num_chars++;

	free(esc_name);
	return acc;
}

static void free_acct_ip_list(struct acct_ip *ips)
{
	while (ips)
	{
		struct acct_ip *next = ips->next;
		ips->hostname        = check_and_clear(ips->hostname);
		ips->ip_address      = check_and_clear(ips->ip_address);
		FREE(ips);
		ips = next;
	}
}

static void free_acct_char_list(struct acct_chars *chars)
{
	while (chars)
	{
		struct acct_chars *next = chars->next;
		chars->charname         = check_and_clear(chars->charname);
		FREE(chars);
		chars = next;
	}
}

static struct acct_chars *sql_load_account_characters(const char *account_name)
{
	if (!DB || !account_name)
		return NULL;

	char *esc_name = sql_escape_string(account_name);
	if (!esc_name)
		return NULL;

	char query[512];
	snprintf(query,
	         sizeof(query),
	         "select ac.char_name, ac.login_count, ac.last_login, ac.blocked, ac.racewar, "
	         "pd.level, pd.race, pd.m_class, pd.secondary_class, pd.last_room, pd.last_save "
	         "from account_characters ac "
	         "left join player_data pd on ac.pid = pd.pid "
	         "where LOWER(ac.account_name)=LOWER('%s') and ac.deleted_at is null",
	         esc_name);
	free(esc_name);

	MYSQL_RES *result = db_query("%s", query);

	if (!result)
	{
		sql_account_chars_query_failed = true;
		logit(LOG_DEBUG, "sql_load_account_characters: query failed for %s", account_name);
		return NULL;
	}

	struct acct_chars *head = NULL;
	struct acct_chars *tail = NULL;
	MYSQL_ROW          row;

	while ((row = mysql_fetch_row(result)))
	{
		struct acct_chars *ch;
		CREATE(ch, struct acct_chars, 1, MEM_TAG_OTHER);

		ch->charname        = str_dup(row[0] ? row[0] : "");
		ch->count           = row[1] ? strtoul(row[1], NULL, 10) : 0;
		ch->last            = row[2] ? atol(row[2]) : 0;
		ch->blocked         = row[3] ? atoi(row[3]) : 0;
		ch->racewar         = row[4] ? atoi(row[4]) : 0;
		ch->level           = row[5] ? atoi(row[5]) : 0;
		ch->race            = row[6] ? atoi(row[6]) : 0;
		ch->m_class         = row[7] ? (unsigned int)strtoul(row[7], NULL, 10) : 0;
		ch->secondary_class = row[8] ? (unsigned int)strtoul(row[8], NULL, 10) : 0;
		ch->last_room       = row[9] ? atoi(row[9]) : 0;
		ch->last_save       = row[10] ? atol(row[10]) : 0;
		ch->next            = NULL;

		if (!head)
			head = ch;
		else
			tail->next = ch;
		tail = ch;
	}

	mysql_free_result(result);
	return head;
}

bool sql_account_exists(const char *name)
{
	if (!DB || !name)
		return false;

	char *escaped_name = sql_escape_string(name);
	if (!escaped_name)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "SELECT 1 FROM accounts WHERE account_name='%s' LIMIT 1", escaped_name);
	free(escaped_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row    = mysql_fetch_row(result);
	bool      exists = (row != NULL);
	mysql_free_result(result);
	return exists;
}

bool sql_link_player_to_account(const char *account_name, int pid)
{
	// todo: implement
	return false;
}

// locker functions

static bool sql_save_locker_item_affects(int item_id, P_obj obj)
{
	if (!obj || !DB || item_id <= 0)
		return false;

	// Own_txn wrapper for standalone-call safety
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
		{
			// skip duplicates (same location+modifier already saved)
			bool is_dup = false;
			for (int j = 0; j < i; j++)
			{
				if (obj->affected[j].location == obj->affected[i].location && obj->affected[j].modifier == obj->affected[i].modifier)
				{
					is_dup = true;
					break;
				}
			}
			if (is_dup)
				continue;

			char query[256];
			snprintf(query, sizeof(query), "INSERT INTO locker_item_affects (item_id, location, modifier) VALUES (%d, %d, %d)", item_id, obj->affected[i].location, obj->affected[i].modifier);
			if (!sql_run_query(query))
			{
				if (own_txn) sql_rollback();
				return false;
			}
		}
	}
	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return false; }
	}
	return true;
}

static int sql_count_obj_contents(P_obj obj)
{
	int count = 0;
	for (P_obj cur = obj ? obj->contains : NULL; cur; cur = cur->next_content)
		++count;
	return count;
}

static bool sql_save_locker_item_children(int locker_id, int chest_id, P_obj obj, int item_id, bool own_txn)
{
	if (!obj || !obj->contains)
		return true;

	for (P_obj content = obj->contains; content; content = content->next_content)
	{
		logit(LOG_DEBUG,
		      "sql_save_locker_item: recurse child from item_id=%d parent_vnum=%d child_vnum=%d child_uid=%lu",
		      item_id,
		      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
		      (content->R_num >= 0) ? obj_index[content->R_num].virtual_number : -1,
		      content->obj_uid);
		if (sql_save_locker_item(locker_id, chest_id, content, item_id) <= 0)
		{
			logit(LOG_DEBUG,
			      "sql_save_locker_item: child save failed parent_item_id=%d parent_vnum=%d child_vnum=%d child_uid=%lu parent_contains=%d child_contains=%d",
			      item_id,
			      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
			      (content->R_num >= 0) ? obj_index[content->R_num].virtual_number : -1,
			      content->obj_uid,
			      sql_count_obj_contents(obj),
			      sql_count_obj_contents(content));
			if (own_txn)
				sql_rollback();
			return false;
		}
	}

	return true;
}

static int sql_save_locker_item(int locker_id, int chest_id, P_obj obj, int container_id)
{
	if (!obj || !DB || locker_id <= 0)
		return 0;

	logit(LOG_DEBUG,
	      "sql_save_locker_item: begin locker_id=%d chest_id=%d container_id=%d vnum=%d uid=%lu short=%s",
	      locker_id,
	      chest_id,
	      container_id,
	      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
	      obj->obj_uid,
	      obj->short_description ? obj->short_description : "(null)");

	// Own_txn wrapper for standalone-call safety
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return 0;
		own_txn = true;
	}

	int vnum = obj_index[obj->R_num].virtual_number;

	char *esc_name   = NULL;
	char *esc_short  = NULL;
	char *esc_desc   = NULL;
	char *esc_action = NULL;

	if (obj->str_mask & STRUNG_KEYS)
		esc_name = sql_escape_string(obj->name ? obj->name : "");
	if (obj->str_mask & STRUNG_DESC2)
		esc_short = sql_escape_string(obj->short_description ? obj->short_description : "");
	if (obj->str_mask & STRUNG_DESC1)
		esc_desc = sql_escape_string(obj->description ? obj->description : "");
	if (obj->str_mask & STRUNG_DESC3)
		esc_action = sql_escape_string(obj->action_description ? obj->action_description : "");

	char container_str[32];
	if (container_id > 0)
		snprintf(container_str, sizeof(container_str), "%d", container_id);
	else
		strcpy(container_str, "NULL");

	char chest_id_str[32];
	if (chest_id > 0)
		snprintf(chest_id_str, sizeof(chest_id_str), "%d", chest_id);
	else
		strcpy(chest_id_str, "NULL");

	char name_str[1024], short_str[1024], desc_str[2048], action_str[2048];
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

	char wear_str[32], type_str[16], material_str[16], bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];
	/* type_str is unused here (locker INSERT formats item_type as %d) but the shared helper writes to it for signature uniformity */
	sql_format_item_diff_fields_and_free_proto(obj, wear_str, type_str, material_str, bv1_str, bv2_str, bv3_str, bv4_str, bv5_str);
	char query[8192];
	snprintf(query,
	         sizeof(query),
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
	         ")",
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
	         obj->value[0],
	         obj->value[1],
	         obj->value[2],
	         obj->value[3],
	         obj->value[4],
	         obj->value[5],
	         obj->value[6],
	         obj->value[7],
	         name_str,
	         short_str,
	         desc_str,
	         action_str,
	         bv1_str,
	         bv2_str,
	         bv3_str,
	         bv4_str,
	         bv5_str,
	         material_str,
	         obj->obj_uid,
	         obj->condition);

	if (esc_name)
		free(esc_name);
	if (esc_short)
		free(esc_short);
	if (esc_desc)
		free(esc_desc);
	if (esc_action)
		free(esc_action);

	if (!sql_run_query(query))
	{
		logit(LOG_DEBUG, "sql_save_item: insert failed errno=%u sqlerr=%s", mysql_errno(DB), mysql_error(DB));
		if (own_txn) sql_rollback();
		return 0;
	}

	int item_id = (int)mysql_insert_id(DB);
	logit(LOG_DEBUG,
	      "sql_save_locker_item: saved item_id=%d locker_id=%d chest_id=%d container_id=%d vnum=%d uid=%lu contains=%s",
	      item_id,
	      locker_id,
	      chest_id,
	      container_id,
	      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
	      obj->obj_uid,
	      obj->contains ? "yes" : "no");

	if (!sql_save_locker_item_affects(item_id, obj))
	{
		logit(LOG_DEBUG,
		      "sql_save_locker_item: affects save failed item_id=%d locker_id=%d chest_id=%d container_id=%d vnum=%d uid=%lu",
		      item_id,
		      locker_id,
		      chest_id,
		      container_id,
		      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
		      obj->obj_uid);
		if (own_txn) sql_rollback();
		return 0;
	}

	if (obj->ex_description && !sql_save_item_extra_descr(item_id, obj, "locker_item_extra_descr"))
	{
		logit(LOG_DEBUG,
		      "sql_save_locker_item: extra descr save failed item_id=%d locker_id=%d chest_id=%d container_id=%d vnum=%d uid=%lu",
		      item_id,
		      locker_id,
		      chest_id,
		      container_id,
		      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
		      obj->obj_uid);
		if (own_txn) sql_rollback();
		return 0;
	}

	if (!sql_save_locker_item_children(locker_id, chest_id, obj, item_id, own_txn))
		return 0;

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return 0; }
	}
	return item_id;
}

static bool sql_save_locker_upsert(P_char locker_ch,
	                                 const char *locker_name,
	                                 char *esc_name,
	                                 int owner_pid,
	                                 int owner_assoc_id,
	                                 int *locker_id)
{
	int existing_locker_id = sql_get_locker_id_by_name(locker_name);

	if (existing_locker_id > 0)
	{
		int carrying_count = 0;
		for (P_obj cur = locker_ch->carrying; cur; cur = cur->next_content)
			carrying_count++;
		logit(LOG_DEBUG,
		      "sql_save_locker: locker_id=%d name=%s saving %d top-level items to public chest",
		      existing_locker_id,
		      locker_name,
		      carrying_count);

		// locker exists - delete only PUBLIC chest items, keep private chest items
		int public_id = sql_get_or_create_public_chest(existing_locker_id);
		if (public_id <= 0)
		{
			logit(LOG_DEBUG, "sql_save_locker: failed to get/create public chest for %s", locker_name);
			sql_rollback();
			return false;
		}
		char del_query[512];
		snprintf(del_query, sizeof(del_query), "DELETE FROM locker_items WHERE locker_id=%d AND (chest_id IS NULL OR chest_id=%d)", existing_locker_id, public_id);
		if (!sql_run_query(del_query))
		{
			logit(LOG_DEBUG, "sql_save_locker: failed to delete old items for %s", locker_name);
			sql_rollback();
			return false;
		}
		*locker_id = existing_locker_id;
		return true;
	}

	// new locker - insert locker record
	char owner_pid_str[32], owner_assoc_str[32];
	if (owner_pid > 0)
		snprintf(owner_pid_str, sizeof(owner_pid_str), "%d", owner_pid);
	else
		strcpy(owner_pid_str, "NULL");
	if (owner_assoc_id > 0)
		snprintf(owner_assoc_str, sizeof(owner_assoc_str), "%d", owner_assoc_id);
	else
		strcpy(owner_assoc_str, "NULL");

	char ins_query[512];
	snprintf(ins_query,
	         sizeof(ins_query),
	         "INSERT INTO lockers (locker_name, owner_pid, owner_assoc_id, racewar, race) "
	         "VALUES ('%s', %s, %s, %d, %d)",
	         esc_name,
	         owner_pid_str,
	         owner_assoc_str,
	         GET_RACEWAR(locker_ch),
	         GET_RACE(locker_ch));

	if (!sql_run_query(ins_query))
	{
		logit(LOG_DEBUG, "sql_save_locker: failed to insert new locker %s", locker_name);
		sql_rollback();
		return false;
	}

	*locker_id = (int)mysql_insert_id(DB);
	return true;
}

static bool sql_save_locker_items(P_char locker_ch, int locker_id, int public_chest_id, const char *locker_name, bool own_txn)
{
	// save all items the locker char is carrying to public chest - any failure rolls back the whole locker save
	for (P_obj obj = locker_ch->carrying; obj; obj = obj->next_content)
	{
		logit(LOG_DEBUG,
		      "sql_save_locker: saving top-level item vnum=%d uid=%lu to locker_id=%d chest_id=%d",
		      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
		      obj->obj_uid,
		      locker_id,
		      public_chest_id);
		if (sql_save_locker_item(locker_id, public_chest_id, obj, 0) == 0)
		{
			logit(LOG_DEBUG, "sql_save_locker: failed to save item, rolling back for %s", locker_name);
			if (own_txn)
				sql_rollback();
			return false;
		}
	}

	if (own_txn && !sql_commit())
	{
		logit(LOG_DEBUG, "sql_save_locker: failed to commit for %s", locker_name);
		sql_rollback();
		return false;
	}

	return true;
}

bool sql_save_locker(P_char locker_ch, int owner_pid, int owner_assoc_id)
{
	if (!locker_ch || !DB)
	{
		logit(LOG_DEBUG,
		      "sql_save_locker: cannot save locker (locker_ch=%p, db=%s)",
		      (void *)locker_ch,
		      DB ? "ready" : "null");
		return false;
	}

	const char *locker_name = GET_NAME(locker_ch);
	if (!locker_name)
	{
		logit(LOG_DEBUG, "sql_save_locker: null locker name");
		return false;
	}

	char *esc_name = sql_escape_string(locker_name);
	if (!esc_name)
	{
		logit(LOG_DEBUG, "sql_save_locker: failed to escape locker name for %s", locker_name);
		return false;
	}

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		// start transaction (must succeed before any writes)
		if (!sql_begin_transaction())
		{
			logit(LOG_DEBUG, "sql_save_locker: failed to start transaction for %s", locker_name);
			free(esc_name);
			return false;
		}
		own_txn = true;
	}

	int locker_id = 0;
	if (!sql_save_locker_upsert(locker_ch, locker_name, esc_name, owner_pid, owner_assoc_id, &locker_id))
	{
		free(esc_name);
		return false;
	}

	free(esc_name);

	// get or create public chest for this locker
	int public_chest_id = sql_get_or_create_public_chest(locker_id);
	if (public_chest_id <= 0)
	{
		logit(LOG_DEBUG, "sql_save_locker: failed to get/create public chest after insert for %s", locker_name);
		sql_rollback();
		return false;
	}

	return sql_save_locker_items(locker_ch, locker_id, public_chest_id, locker_name, own_txn);
}

static P_obj sql_load_locker_items(int locker_id, int container_id, const char *owner_ref);

#define MAX_CONTAINER_LOAD_DEPTH 64

static P_obj sql_load_locker_items_filtered(int locker_id, int container_id, int chest_id, const char *owner_ref, int depth)
{
	if (!DB || locker_id <= 0)
		return NULL;

	logit(LOG_DEBUG,
	      "sql_load_locker_items_filtered: begin locker_id=%d container_id=%d chest_id=%d owner_ref=%s depth=%d",
	      locker_id,
	      container_id,
	      chest_id,
	      owner_ref ? owner_ref : "(null)",
	      depth);

	if (depth > MAX_CONTAINER_LOAD_DEPTH)
	{
		logit(LOG_DEBUG, "sql_load_locker_items_filtered: container depth exceeded for locker %d container %d", locker_id, container_id);
		return NULL;
	}

	char query[1024];
	char chest_filter[256] = "";
	if (chest_id > 0)
		snprintf(chest_filter, sizeof(chest_filter), " AND chest_id=%d", chest_id);
	else
		snprintf(chest_filter,
		         sizeof(chest_filter),
		         " AND (chest_id IS NULL OR chest_id NOT IN "
		         "(SELECT id FROM private_chests WHERE locker_id=%d AND is_public=0))",
		         locker_id);

	if (container_id > 0)
		snprintf(query,
		         sizeof(query),
		         "SELECT id, vnum, weight, cost, timer, extra_flags, wear_flags, item_type, "
		         "value0, value1, value2, value3, value4, value5, value6, value7, "
		         "name, short_descr, description, action_descr, obj_uid, item_condition, "
		         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, item_material "
		         "FROM locker_items WHERE locker_id=%d AND container_id=%d%s",
		         locker_id,
		         container_id,
		         chest_filter);
	else
		snprintf(query,
		         sizeof(query),
		         "SELECT id, vnum, weight, cost, timer, extra_flags, wear_flags, item_type, "
		         "value0, value1, value2, value3, value4, value5, value6, value7, "
		         "name, short_descr, description, action_descr, obj_uid, item_condition, "
		         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, item_material "
		         "FROM locker_items WHERE locker_id=%d AND container_id IS NULL%s",
		         locker_id,
		         chest_filter);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	P_obj     first_obj = NULL;
	P_obj     last_obj  = NULL;
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		int item_id = atoi(row[0]);
		int vnum    = atoi(row[1]);
		int rnum    = real_object(vnum);
		logit(LOG_DEBUG,
		      "sql_load_locker_items_filtered: row item_id=%d locker_id=%d container_id=%d chest_id=%d vnum=%d rnum=%d depth=%d",
		      item_id,
		      locker_id,
		      container_id,
		      chest_id,
		      vnum,
		      rnum,
		      depth);
		if (rnum < 0)
		{
			logit(LOG_DEBUG,
			      "sql_load_locker_items_filtered: skip unknown vnum item_id=%d vnum=%d locker_id=%d chest_id=%d container_id=%d",
			      item_id,
			      vnum,
			      locker_id,
			      chest_id,
			      container_id);
			continue;
		}

		P_obj obj = read_object(rnum, REAL);
		if (!obj)
		{
			logit(LOG_DEBUG,
			      "sql_load_locker_items_filtered: skip failed read_object item_id=%d vnum=%d rnum=%d locker_id=%d chest_id=%d container_id=%d",
			      item_id,
			      vnum,
			      rnum,
			      locker_id,
			      chest_id,
			      container_id);
			continue;
		}

		if (row[2])
			obj->weight = atoi(row[2]);
		if (row[3])
			obj->cost = atoi(row[3]);
		if (row[4])
			obj->timer[0] = atol(row[4]);
		if (row[5])
			obj->extra_flags = strtoul(row[5], NULL, 10);
		if (row[6])
			obj->wear_flags = atoi(row[6]);
		if (row[7])
			obj->type = sql_validate_loaded_item_type(obj, atoi(row[7]), "sql_load_locker_items");

		obj->value[0] = row[8] ? atoi(row[8]) : obj->value[0];
		obj->value[1] = row[9] ? atoi(row[9]) : obj->value[1];
		obj->value[2] = row[10] ? atoi(row[10]) : obj->value[2];
		obj->value[3] = row[11] ? atoi(row[11]) : obj->value[3];
		obj->value[4] = row[12] ? atoi(row[12]) : obj->value[4];
		obj->value[5] = row[13] ? atoi(row[13]) : obj->value[5];
		obj->value[6] = row[14] ? atoi(row[14]) : obj->value[6];
		obj->value[7] = row[15] ? atoi(row[15]) : obj->value[7];

		if (row[16] && strlen(row[16]) > 0)
		{
			obj->name = str_dup(row[16]);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (row[17] && strlen(row[17]) > 0)
		{
			obj->short_description = str_dup(row[17]);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (row[18] && strlen(row[18]) > 0)
		{
			obj->description = str_dup(row[18]);
			obj->str_mask |= STRUNG_DESC1;
		}
		if (row[19] && strlen(row[19]) > 0)
		{
			obj->action_description = str_dup(row[19]);
			obj->str_mask |= STRUNG_DESC3;
		}
		if (row[22])
			obj->bitvector = strtoul(row[22], NULL, 10);
		if (row[23])
			obj->bitvector2 = strtoul(row[23], NULL, 10);
		if (row[24])
			obj->bitvector3 = strtoul(row[24], NULL, 10);
		if (row[25])
			obj->bitvector4 = strtoul(row[25], NULL, 10);
		if (row[26])
			obj->bitvector5 = strtoul(row[26], NULL, 10);
		if (row[27])
			obj->material = atoi(row[27]);

		if (row[20] && strlen(row[20]) > 0)
		{
			unsigned long saved_uid = strtoul(row[20], NULL, 10);
			if (saved_uid > 0)
				obj->obj_uid = saved_uid;
			if (!sql_persistence_item_owner_matches(obj->obj_uid, "locker", owner_ref, "sql_load_locker_items"))
			{
				logit(LOG_DEBUG,
				      "sql_load_locker_items_filtered: owner mismatch item_id=%d vnum=%d uid=%lu locker_id=%d chest_id=%d container_id=%d owner_ref=%s",
				      item_id,
				      vnum,
				      obj->obj_uid,
				      locker_id,
				      chest_id,
				      container_id,
				      owner_ref ? owner_ref : "(null)");
				extract_obj(obj, FALSE);
				continue;
			}
		}
		if (row[21] && strlen(row[21]) > 0)
			obj->condition = atoi(row[21]);

		sql_load_item_affects_from_table(item_id, obj, "locker_item_affects");
		sql_load_item_extra_descr_from_table(item_id, obj, "locker_item_extra_descr");

		obj->contains = sql_load_locker_items_filtered(locker_id, item_id, chest_id, owner_ref, depth + 1);
		{
			int child_count = 0;
			for (P_obj c = obj->contains; c; c = c->next_content)
				child_count++;
			logit(LOG_DEBUG,
			      "sql_load_locker_items_filtered: loaded %d children for item_id=%d vnum=%d uid=%lu locker_id=%d chest_id=%d container_id=%d",
			      child_count,
			      item_id,
			      vnum,
			      obj->obj_uid,
			      locker_id,
			      chest_id,
			      container_id);
		}
		for (P_obj c = obj->contains; c; c = c->next_content)
		{
			if (!obj_can_nest(c, obj))
			{
				logit(LOG_DEBUG,
				      "sql_load_locker_items_filtered: skipping malformed container link child_db_id=%d parent_db_id=%d child_vnum=%d parent_vnum=%d child_uid=%lu parent_uid=%lu",
				      c->db_item_id,
				      obj->db_item_id,
				      (c->R_num >= 0) ? obj_index[c->R_num].virtual_number : -1,
				      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
				      c->obj_uid,
				      obj->obj_uid);
				continue;
			}
			logit(LOG_DEBUG,
			      "sql_load_locker_items_filtered: linked child_db_id=%d to parent_db_id=%d child_vnum=%d parent_vnum=%d child_uid=%lu parent_uid=%lu",
			      c->db_item_id,
			      obj->db_item_id,
			      (c->R_num >= 0) ? obj_index[c->R_num].virtual_number : -1,
			      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
			      c->obj_uid,
			      obj->obj_uid);
			c->loc_p      = LOC_INSIDE;
			c->loc.inside = obj;
		}

		if (!first_obj)
			first_obj = obj;
		else
			last_obj->next_content = obj;
		last_obj          = obj;
		obj->next_content = NULL;
	}

	mysql_free_result(result);
	return first_obj;
}
static P_obj sql_load_locker_items(int locker_id, int container_id, const char *owner_ref) { return sql_load_locker_items_filtered(locker_id, container_id, 0, owner_ref, 0); }

P_char sql_load_locker(int owner_pid, int owner_assoc_id)
{
	if (!DB)
		return NULL;

	char query[256];
	if (owner_pid > 0)
		snprintf(query, sizeof(query), "SELECT id, locker_name, racewar, race FROM lockers WHERE owner_pid=%d", owner_pid);
	else if (owner_assoc_id > 0)
		snprintf(query, sizeof(query), "SELECT id, locker_name, racewar, race FROM lockers WHERE owner_assoc_id=%d", owner_assoc_id);
	else
		return NULL;

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		return NULL;
	}

	int         locker_id   = atoi(row[0]);
	const char *locker_name = row[1];
	char owner_ref[MAX_INPUT_LENGTH] = "";
	snprintf(owner_ref, sizeof(owner_ref), "%s", locker_name);
	int         racewar     = atoi(row[2]);
	int         race        = atoi(row[3]);

	// allocate locker character
	P_char ch = (P_char)mm_get(dead_mob_pool);
	if (!ch)
	{
		mysql_free_result(result);
		return NULL;
	}
	clear_char(ch);
	ensure_pconly_pool();
	ch->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
	if (!ch->only.pc)
	{
		mm_release(dead_mob_pool, ch);
		mysql_free_result(result);
		return NULL;
	}
	memset(ch->only.pc, 0, sizeof(struct pc_only_data));
	ch->only.pc->aggressive  = -1;
	ch->only.pc->zone_trophy = NULL;
	ch->desc                 = NULL;

	ch->player.name = str_dup(locker_name);
	GET_RACEWAR(ch) = racewar;
	GET_RACE(ch)    = race;

	mysql_free_result(result);

	// load items
	ch->carrying = sql_load_locker_items(locker_id, 0, owner_ref);
	{
		int carry_count = 0;
		for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
			carry_count++;
		logit(LOG_DEBUG,
		      "sql_load_locker: locker_id=%d name=%s loaded %d top-level items",
		      locker_id,
		      locker_name,
		      carry_count);
	}
	for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
	{
		obj->loc_p        = LOC_CARRIED;
		obj->loc.carrying = ch;
	}

	return ch;
}

// load locker by name (used by storage_lockers.c)
P_char sql_load_locker_by_name(const char *locker_name)
{
	if (!DB || !locker_name)
		return NULL;

	char *esc_name = sql_escape_string(locker_name);
	if (!esc_name)
		return NULL;

	char query[256];
	snprintf(query, sizeof(query), "SELECT id, racewar, race FROM lockers WHERE locker_name='%s'", esc_name);
	free(esc_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		return NULL;
	}

	int locker_id = atoi(row[0]);
	int racewar   = atoi(row[1]);
	int race      = atoi(row[2]);
	mysql_free_result(result);
	char owner_ref[MAX_INPUT_LENGTH] = "";
	snprintf(owner_ref, sizeof(owner_ref), "%s", locker_name);

	// allocate locker character
	P_char ch = (P_char)mm_get(dead_mob_pool);
	if (!ch)
		return NULL;
	clear_char(ch);
	ensure_pconly_pool();
	ch->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
	if (!ch->only.pc)
	{
		mm_release(dead_mob_pool, ch);
		return NULL;
	}
	memset(ch->only.pc, 0, sizeof(struct pc_only_data));
	ch->only.pc->aggressive  = -1;
	ch->only.pc->zone_trophy = NULL;
	ch->desc                 = NULL;

	ch->player.name = str_dup(locker_name);
	GET_RACEWAR(ch) = racewar;
	GET_RACE(ch)    = race;

	// load items
	ch->carrying = sql_load_locker_items(locker_id, 0, owner_ref);
	for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
	{
		obj->loc_p        = LOC_CARRIED;
		obj->loc.carrying = ch;
	}

	return ch;
}

bool sql_locker_exists(int owner_pid, int owner_assoc_id)
{
	if (!DB)
		return false;

	char query[128];
	if (owner_pid > 0)
		snprintf(query, sizeof(query), "SELECT 1 FROM lockers WHERE owner_pid=%d LIMIT 1", owner_pid);
	else if (owner_assoc_id > 0)
		snprintf(query, sizeof(query), "SELECT 1 FROM lockers WHERE owner_assoc_id=%d LIMIT 1", owner_assoc_id);
	else
		return false;

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row    = mysql_fetch_row(result);
	bool      exists = (row != NULL);
	mysql_free_result(result);
	return exists;
}

bool sql_locker_exists_by_name(const char *locker_name)
{
	if (!DB || !locker_name)
		return false;

	char *esc_name = sql_escape_string(locker_name);
	if (!esc_name)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "SELECT 1 FROM lockers WHERE locker_name='%s' LIMIT 1", esc_name);
	free(esc_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row    = mysql_fetch_row(result);
	bool      exists = (row != NULL);
	mysql_free_result(result);
	return exists;
}

bool sql_delete_locker(int owner_pid, int owner_assoc_id)
{
	if (!DB)
		return false;

	char query[128];
	if (owner_pid > 0)
		snprintf(query, sizeof(query), "DELETE FROM lockers WHERE owner_pid=%d", owner_pid);
	else if (owner_assoc_id > 0)
		snprintf(query, sizeof(query), "DELETE FROM lockers WHERE owner_assoc_id=%d", owner_assoc_id);
	else
		return false;

	return sql_run_query(query);
}

bool sql_delete_locker_by_name(const char *locker_name)
{
	if (!DB || !locker_name)
		return false;

	char *esc_name = sql_escape_string(locker_name);
	if (!esc_name)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "DELETE FROM lockers WHERE locker_name='%s'", esc_name);
	free(esc_name);

	return sql_run_query(query);
}

// ============================================================================
// private chest functions
// ============================================================================

int sql_get_locker_id_by_name(const char *locker_name)
{
	if (!DB || !locker_name)
		return 0;

	char *esc_name = sql_escape_string(locker_name);
	if (!esc_name)
		return 0;

	char query[256];
	snprintf(query, sizeof(query), "SELECT id FROM lockers WHERE locker_name='%s'", esc_name);
	free(esc_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return 0;

	int       locker_id = 0;
	MYSQL_ROW row       = mysql_fetch_row(result);
	if (row)
		locker_id = atoi(row[0]);
	mysql_free_result(result);
	return locker_id;
}

int sql_get_or_create_public_chest(int locker_id)
{
	if (!DB || locker_id <= 0)
		return 0;

	char query[512];
	snprintf(query, sizeof(query), "SELECT id FROM private_chests WHERE locker_id=%d AND is_public=1", locker_id);

	MYSQL_RES *result = db_query("%s", query);
	if (result)
	{
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row)
		{
			int id = atoi(row[0]);
			mysql_free_result(result);
			return id;
		}
		mysql_free_result(result);
	}

	snprintf(query, sizeof(query), "INSERT INTO private_chests (locker_id, chest_name, is_public) VALUES (%d, 'public', 1)", locker_id);

	if (!sql_run_query(query))
		return 0;

	return (int)mysql_insert_id(DB);
}

int sql_create_private_chest(int locker_id, const char *chest_name, const char *password)
{
	if (!DB || locker_id <= 0 || !chest_name)
		return 0;

	if (sql_count_private_chests(locker_id) >= 5)
		return -1;

	char *esc_name = sql_escape_string(chest_name);
	if (!esc_name)
		return 0;

	char query[512];
	if (password && password[0])
	{
		char *esc_pass = sql_escape_string(password);
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO private_chests (locker_id, chest_name, password_hash, is_public) "
		         "VALUES (%d, '%s', SHA2('%s', 256), 0)",
		         locker_id,
		         esc_name,
		         esc_pass);
		free(esc_pass);
	}
	else
	{
		snprintf(query, sizeof(query), "INSERT INTO private_chests (locker_id, chest_name, is_public) VALUES (%d, '%s', 0)", locker_id, esc_name);
	}
	free(esc_name);

	if (!sql_run_query(query))
		return 0;

	return (int)mysql_insert_id(DB);
}

bool sql_delete_private_chest(int chest_id)
{
	if (!DB || chest_id <= 0)
		return false;

	char query[256];
	MYSQL_RES *result = NULL;
	MYSQL_ROW row = NULL;
	bool is_private = false;
	bool has_items = false;
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	/* Lock the parent first. InnoDB foreign-key inserts must wait on this
	 * lock, so the emptiness check and delete cannot race a child insert. */
	snprintf(query, sizeof(query), "SELECT is_public FROM private_chests WHERE id=%d FOR UPDATE", chest_id);
	result = db_query("%s", query);
	if (!result)
		goto fail;
	row = mysql_fetch_row(result);
	is_private = row && atoi(row[0]) == 0;
	mysql_free_result(result);
	if (!is_private)
		goto fail;

	snprintf(query, sizeof(query), "SELECT id FROM locker_items WHERE chest_id=%d FOR UPDATE", chest_id);
	result = db_query("%s", query);
	if (!result)
		goto fail;
	has_items = mysql_fetch_row(result) != NULL;
	mysql_free_result(result);
	if (has_items)
		goto fail;

	snprintf(query, sizeof(query), "DELETE FROM private_chests WHERE id=%d AND is_public=0", chest_id);
	if (!sql_run_query(query) || mysql_affected_rows(DB) != 1)
		goto fail;

	if (own_txn && !sql_commit())
		goto fail;
	return true;

fail:
	if (own_txn)
		sql_rollback();
	return false;
}

int sql_get_chest_id(int locker_id, const char *chest_name)
{
	if (!DB || locker_id <= 0 || !chest_name)
		return 0;

	char *esc_name = sql_escape_string(chest_name);
	if (!esc_name)
		return 0;

	char query[512];
	snprintf(query, sizeof(query), "SELECT id FROM private_chests WHERE locker_id=%d AND chest_name='%s'", locker_id, esc_name);
	free(esc_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return 0;

	int       id  = 0;
	MYSQL_ROW row = mysql_fetch_row(result);
	if (row)
		id = atoi(row[0]);
	mysql_free_result(result);
	return id;
}

bool sql_verify_chest_password(int chest_id, const char *password)
{
	if (!DB || chest_id <= 0)
		return false;

	char query[512];

	if (!password || !password[0])
	{
		snprintf(query, sizeof(query), "SELECT id FROM private_chests WHERE id=%d AND password_hash IS NULL", chest_id);
	}
	else
	{
		char *esc_pass = sql_escape_string(password);
		if (!esc_pass)
			return false;
		snprintf(query, sizeof(query), "SELECT id FROM private_chests WHERE id=%d AND password_hash=SHA2('%s', 256)", chest_id, esc_pass);
		free(esc_pass);
	}

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	bool found = (mysql_fetch_row(result) != NULL);
	mysql_free_result(result);
	return found;
}

int sql_count_private_chests(int locker_id)
{
	if (!DB || locker_id <= 0)
		return 0;

	char query[256];
	snprintf(query, sizeof(query), "SELECT COUNT(*) FROM private_chests WHERE locker_id=%d AND is_public=0", locker_id);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return 0;

	int       count = 0;
	MYSQL_ROW row   = mysql_fetch_row(result);
	if (row)
		count = atoi(row[0]);
	mysql_free_result(result);
	return count;
}

bool sql_log_chest_activity(int locker_id, int chest_id, const char *char_name, int action_type, const char *item_short)
{
	if (!DB || locker_id <= 0 || !char_name || action_type < 1)
		return false;

	char *esc_char = sql_escape_string(char_name);
	char *esc_item = item_short ? sql_escape_string(item_short) : NULL;

	char chest_str[32];
	if (chest_id > 0)
		snprintf(chest_str, sizeof(chest_str), "%d", chest_id);
	else
		strcpy(chest_str, "NULL");

	char query[1024];
	snprintf(query,
	         sizeof(query),
	         "INSERT INTO private_chest_log (locker_id, chest_id, char_name, action_type, item_short) "
	         "VALUES (%d, %s, '%s', %d, %s%s%s)",
	         locker_id,
	         chest_str,
	         esc_char,
	         action_type,
	         esc_item ? "'" : "",
	         esc_item ? esc_item : "NULL",
	         esc_item ? "'" : "");

	free(esc_char);
	if (esc_item)
		free(esc_item);

	return sql_run_query(query);
}

bool sql_save_private_chest_items(int locker_id, int chest_id, P_obj chest_obj)
{
	if (!DB || locker_id <= 0 || chest_id <= 0 || !chest_obj)
		return false;

	logit(LOG_DEBUG,
	      "sql_save_private_chest_items: begin locker_id=%d chest_id=%d chest_vnum=%d chest_uid=%lu contains=%d in_txn=%d",
	      locker_id,
	      chest_id,
	      (chest_obj->R_num >= 0) ? obj_index[chest_obj->R_num].virtual_number : -1,
	      chest_obj->obj_uid,
	      sql_count_obj_contents(chest_obj),
	      sql_in_transaction() ? 1 : 0);

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		// start transaction (must succeed before the DELETE - otherwise the chest
		// could be left empty if the inserts fail, losing all stored items)
		if (!sql_begin_transaction())
		{
			logit(LOG_DEBUG, "sql_save_private_chest_items: failed to start transaction for chest %d", chest_id);
			return false;
		}
		own_txn = true;
	}
	else
	{
		logit(LOG_DEBUG, "sql_save_private_chest_items: joining existing transaction for chest %d", chest_id);
	}

	// delete existing items for this chest
	logit(LOG_DEBUG,
	      "sql_save_private_chest_items: deleting old rows locker_id=%d chest_id=%d chest_uid=%lu contains=%d",
	      locker_id,
	      chest_id,
	      chest_obj->obj_uid,
	      sql_count_obj_contents(chest_obj));
	char del_query[256];
	snprintf(del_query, sizeof(del_query), "DELETE FROM locker_items WHERE locker_id=%d AND chest_id=%d", locker_id, chest_id);
	if (!sql_run_query(del_query))
	{
		logit(LOG_DEBUG, "sql_save_private_chest_items: failed to delete old items for chest %d", chest_id);
		if (own_txn)
			sql_rollback();
		return false;
	}

	// save all items in the chest - any failure rolls back the DELETE above
	for (P_obj obj = chest_obj->contains; obj; obj = obj->next_content)
	{
		logit(LOG_DEBUG,
		      "sql_save_private_chest_items: saving child locker_id=%d chest_id=%d obj_vnum=%d uid=%lu contains=%d type=%d container_id=NULL",
		      locker_id,
		      chest_id,
		      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
		      obj->obj_uid,
		      sql_count_obj_contents(obj),
		      obj->type);
		if (sql_save_locker_item(locker_id, chest_id, obj, 0) == 0)
		{
			logit(LOG_DEBUG,
			      "sql_save_private_chest_items: failed to save item locker_id=%d chest_id=%d obj_vnum=%d uid=%lu errno=%u sqlerr=%s",
			      locker_id,
			      chest_id,
			      (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : -1,
			      obj->obj_uid,
			      mysql_errno(DB),
			      mysql_error(DB));
			if (own_txn)
				sql_rollback();
			return false;
		}
	}

	if (own_txn && !sql_commit())
	{
		logit(LOG_DEBUG, "sql_save_private_chest_items: failed to commit for chest %d", chest_id);
		sql_rollback();
		return false;
	}

	return true;
}

void sql_load_private_chest_items(int locker_id, int chest_id, P_obj chest_obj)
{
	if (!DB || locker_id <= 0 || chest_id <= 0 || !chest_obj)
		return;

	char query[1024];
	char owner_ref[MAX_INPUT_LENGTH] = "";
	snprintf(query, sizeof(query), "SELECT locker_name FROM lockers WHERE id=%d LIMIT 1", locker_id);
	MYSQL_RES *locker_result = db_query("%s", query);
	if (locker_result)
	{
		MYSQL_ROW locker_row = mysql_fetch_row(locker_result);
		if (locker_row && locker_row[0])
			snprintf(owner_ref, sizeof(owner_ref), "%s", locker_row[0]);
		mysql_free_result(locker_result);
	}

	snprintf(query,
	         sizeof(query),
	         "SELECT id, vnum, weight, cost, timer, extra_flags, wear_flags, item_type, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, obj_uid, item_condition, "
	         "item_material "
	         "FROM locker_items WHERE locker_id=%d AND container_id IS NULL AND chest_id=%d",
	         locker_id,
	         chest_id);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		int item_id = atoi(row[0]);
		int vnum    = atoi(row[1]);
		int rnum    = real_object(vnum);
		if (rnum < 0)
			continue;

		P_obj obj = read_object(rnum, REAL);
		if (!obj)
			continue;

		if (row[2])
			obj->weight = atoi(row[2]);
		if (row[3])
			obj->cost = atoi(row[3]);
		if (row[4])
			obj->timer[0] = atol(row[4]);
		if (row[5])
			obj->extra_flags = strtoul(row[5], NULL, 10);
		if (row[6])
			obj->wear_flags = atoi(row[6]);
		if (row[7])
			obj->type = sql_validate_loaded_item_type(obj, atoi(row[7]), "sql_load_private_chest_items");

		obj->value[0] = row[8] ? atoi(row[8]) : obj->value[0];
		obj->value[1] = row[9] ? atoi(row[9]) : obj->value[1];
		obj->value[2] = row[10] ? atoi(row[10]) : obj->value[2];
		obj->value[3] = row[11] ? atoi(row[11]) : obj->value[3];
		obj->value[4] = row[12] ? atoi(row[12]) : obj->value[4];
		obj->value[5] = row[13] ? atoi(row[13]) : obj->value[5];
		obj->value[6] = row[14] ? atoi(row[14]) : obj->value[6];
		obj->value[7] = row[15] ? atoi(row[15]) : obj->value[7];

		if (row[16] && strlen(row[16]) > 0)
		{
			obj->name = str_dup(row[16]);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (row[17] && strlen(row[17]) > 0)
		{
			obj->short_description = str_dup(row[17]);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (row[18] && strlen(row[18]) > 0)
		{
			obj->description = str_dup(row[18]);
			obj->str_mask |= STRUNG_DESC1;
		}
		if (row[19] && strlen(row[19]) > 0)
		{
			obj->action_description = str_dup(row[19]);
			obj->str_mask |= STRUNG_DESC3;
		}
		// restore obj_uid and condition
		if (row[20] && strlen(row[20]) > 0)
		{
			unsigned long saved_uid = strtoul(row[20], NULL, 10);
			if (saved_uid > 0)
				obj->obj_uid = saved_uid;
		}
		if (row[21] && strlen(row[21]) > 0)
			obj->condition = atoi(row[21]);

		if (!sql_persistence_item_owner_matches(obj->obj_uid, "locker", owner_ref, "sql_load_private_chest_items"))
		{
			extract_obj(obj, FALSE);
			continue;
		}

		sql_load_item_affects_from_table(item_id, obj, "locker_item_affects");

		// Put the chest item into the room before loading nested contents so
		// nested containers do not get rejected by a fit check while still full.
		obj_to_obj(obj, chest_obj);

		// load contained items (bags inside the chest)
		obj->contains = sql_load_locker_items_filtered(locker_id, item_id, chest_id, owner_ref, 1);
		for (P_obj c = obj->contains; c; c = c->next_content)
		{
			if (!obj_can_nest(c, obj))
			{
				logit(LOG_DEBUG, "sql_load_siege_item_contents: skipping malformed container link %d -> %d", c->db_item_id, obj->db_item_id);
				continue;
			}
			c->loc_p      = LOC_INSIDE;
			c->loc.inside = obj;
		}
	}
	mysql_free_result(result);
}

// migration helpers

// allocate a temp char for migration (uses malloc, not pools)
static P_char alloc_temp_char(void)
{
	P_char ch = (P_char)malloc(sizeof(struct char_data));
	if (!ch)
		return NULL;
	memset(ch, 0, sizeof(struct char_data));

	ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
	if (!ch->only.pc)
	{
		free(ch);
		return NULL;
	}
	memset(ch->only.pc, 0, sizeof(struct pc_only_data));
	return ch;
}

// free a temp char allocated by alloc_temp_char
// also frees any items the char is carrying
static void free_temp_char(P_char ch)
{
	if (!ch)
		return;

	// properly extract equipment (must unequip first to clear loc.wearing)
	for (int i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
		{
			P_obj obj = unequip_char(ch, i);
			extract_obj(obj, FALSE);
		}
	}

	// properly extract carried items
	P_obj obj, next;
	for (obj = ch->carrying; obj; obj = next)
	{
		next = obj->next_content;
		obj_from_char(obj);
		extract_obj(obj, FALSE);
	}

	// strings from pfile loader need the proper deallocator
	if (ch->player.name)
		FREE(ch->player.name);
	if (ch->player.short_descr)
		FREE(ch->player.short_descr);
	if (ch->player.long_descr)
		FREE(ch->player.long_descr);
	if (ch->player.description)
		FREE(ch->player.description);
	if (ch->player.title)
		FREE(ch->player.title);
	if (ch->only.pc && ch->only.pc->poofIn)
		FREE(ch->only.pc->poofIn);
	if (ch->only.pc && ch->only.pc->poofOut)
		FREE(ch->only.pc->poofOut);

	if (ch->only.pc)
		free(ch->only.pc);
	free(ch);
}

bool sql_migrate_player(const char *name)
{
	if (!name || !*name)
		return false;

	logit(LOG_DEBUG, "sql_migrate_player: migrating %s", name);

	// check if already in db
	if (sql_player_exists(name))
	{
		logit(LOG_DEBUG, "sql_migrate_player: %s already exists in db, skipping", name);
		return true;
	}

	// allocate temp char
	P_char ch = alloc_temp_char();
	if (!ch)
	{
		logit(LOG_FILE, "sql_migrate_player: failed to allocate char for %s", name);
		return false;
	}

	// load from pfile
	int status = restoreCharOnly(ch, (char *)name);
	if (status < 0)
	{
		logit(LOG_FILE, "sql_migrate_player: failed to load pfile for %s (status %d)", name, status);
		free_temp_char(ch);
		return false;
	}

	// load items
	ch->carrying = NULL;
	for (int i = 0; i < MAX_WEAR; i++)
		ch->equipment[i] = NULL;
	if (restoreItemsOnly(ch, 0) < 0)
	{
		logit(LOG_FILE, "sql_migrate_player: failed to load items for %s", name);
		free_temp_char(ch);
		return false;
	}

	// save to db
	// use status as rent type, room 0 (will be fixed on login)
	bool result = sql_save_player(ch, status, 0);
	if (!result)
	{
		logit(LOG_FILE, "sql_migrate_player: failed to save %s to db", name);
		free_temp_char(ch);
		return false;
	}

	logit(LOG_DEBUG, "sql_migrate_player: successfully migrated %s", name);
	free_temp_char(ch);
	return true;
}

bool sql_verify_player(const char *name)
{
	if (!name || !*name)
		return false;

	// load from pfile
	P_char pfile_ch = alloc_temp_char();
	if (!pfile_ch)
		return false;

	int status = restoreCharOnly(pfile_ch, (char *)name);
	if (status < 0)
	{
		free_temp_char(pfile_ch);
		return false;
	}

	// load from db
	P_char db_ch = sql_load_player(name);
	if (!db_ch)
	{
		logit(LOG_FILE, "sql_verify_player: %s not found in db", name);
		free_temp_char(pfile_ch);
		return false;
	}

	// compare key fields
	bool match = true;

	if (strcmp(GET_NAME(pfile_ch), GET_NAME(db_ch)) != 0)
	{
		logit(LOG_FILE, "sql_verify_player: %s name mismatch", name);
		match = false;
	}
	if (GET_LEVEL(pfile_ch) != GET_LEVEL(db_ch))
	{
		logit(LOG_FILE, "sql_verify_player: %s level mismatch (%d vs %d)", name, GET_LEVEL(pfile_ch), GET_LEVEL(db_ch));
		match = false;
	}
	if (GET_RACE(pfile_ch) != GET_RACE(db_ch))
	{
		logit(LOG_FILE, "sql_verify_player: %s race mismatch", name);
		match = false;
	}
	if (pfile_ch->player.m_class != db_ch->player.m_class)
	{
		logit(LOG_FILE, "sql_verify_player: %s class mismatch", name);
		match = false;
	}
	if (GET_EXP(pfile_ch) != GET_EXP(db_ch))
	{
		logit(LOG_FILE, "sql_verify_player: %s exp mismatch (%ld vs %ld)", name, GET_EXP(pfile_ch), GET_EXP(db_ch));
		match = false;
	}
	if (GET_GOLD(pfile_ch) != GET_GOLD(db_ch))
	{
		logit(LOG_FILE, "sql_verify_player: %s gold mismatch", name);
		match = false;
	}

	free_temp_char(pfile_ch);
	free_temp_char(db_ch);

	if (match)
		logit(LOG_DEBUG, "sql_verify_player: %s verified OK", name);

	return match;
}

// migrate all players from pfiles to db
// returns count of successfully migrated players
int sql_migrate_all_players(void)
{
	DIR           *pf_dir;
	struct dirent *pf_entry;
	char           dname[256];
	char           fname[256];
	char           letter;
	char          *dot_index;
	int            success_count = 0;
	int            fail_count    = 0;
	int            skip_count    = 0;

	logit(LOG_DEBUG, "sql_migrate_all_players: starting migration");

	for (letter = 'a'; letter <= 'z'; letter++)
	{
		snprintf(dname, 256, "%s/%c", SAVE_DIR, letter);
		pf_dir = opendir(dname);
		if (!pf_dir)
			continue;

		while ((pf_entry = readdir(pf_dir)) != NULL)
		{
			strncpy(fname, pf_entry->d_name, sizeof(fname) - 1);
			fname[sizeof(fname) - 1] = '\0';

			// skip . and ..
			if (fname[0] == '.')
				continue;

			// skip files with extensions (like .locker, .old, etc)
			dot_index = strrchr(fname, '.');
			if (dot_index)
				continue;

			// try to migrate
			if (sql_player_exists(fname))
			{
				skip_count++;
				continue;
			}

			if (sql_migrate_player(fname))
				success_count++;
			else
				fail_count++;
		}

		closedir(pf_dir);
	}

	logit(LOG_DEBUG, "sql_migrate_all_players: done - %d migrated, %d failed, %d skipped", success_count, fail_count, skip_count);

	return success_count;
}

// town save/load

extern int               top_of_zone_table;
extern struct zone_data *zone_table;
extern P_town            towns;

bool sql_save_towns(void)
{
	if (!DB)
		return false;

	// start transaction (must succeed before the DELETE - otherwise the towns
	// table could be left empty if the inserts fail)
	if (!sql_begin_transaction())
	{
		logit(LOG_DEBUG, "sql_save_towns: failed to start transaction");
		return false;
	}

	if (!sql_run_query("DELETE FROM towns"))
	{
		logit(LOG_DEBUG, "sql_save_towns: failed to delete old towns");
		sql_rollback();
		return false;
	}

	for (P_town town = towns; town; town = town->next_town)
	{
		if (!town->zone || !town->zone->filename)
			continue;

		char *escaped_filename = sql_escape_string(town->zone->filename);
		if (!escaped_filename)
			continue;

		char query[1024];
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO towns (zone_filename, resources, defense, offense, "
		         "deploy_guard, guard_vnum, guard_max, guard_load_room, "
		         "deploy_cavalry, cavalry_vnum, cavalry_max, cavalry_load_room, "
		         "deploy_portals, portal_vnum, portal_load_room) "
		         "VALUES ('%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
		         escaped_filename,
		         town->resources,
		         town->defense,
		         town->offense,
		         town->deploy_guard ? 1 : 0,
		         town->guard_vnum,
		         town->guard_max,
		         town->guard_load_room,
		         town->deploy_cavalry ? 1 : 0,
		         town->cavalry_vnum,
		         town->cavalry_max,
		         town->cavalry_load_room,
		         town->deploy_portals ? 1 : 0,
		         town->portal_vnum,
		         town->portal_load_room);

		free(escaped_filename);
		if (!sql_run_query(query))
		{
			logit(LOG_DEBUG, "sql_save_towns: failed to insert town, rolling back");
			sql_rollback();
			return false;
		}
	}

	if (!sql_commit())
	{
		logit(LOG_DEBUG, "sql_save_towns: failed to commit");
		sql_rollback();
		return false;
	}

	return true;
}

bool sql_load_towns(void)
{
	if (!DB)
		return false;

	while (towns)
	{
		P_town next = towns->next_town;
		delete towns;
		towns = next;
	}
	towns = NULL;

	MYSQL_RES *result = db_query("SELECT zone_filename, resources, defense, offense, "
	                             "deploy_guard, guard_vnum, guard_max, guard_load_room, "
	                             "deploy_cavalry, cavalry_vnum, cavalry_max, cavalry_load_room, "
	                             "deploy_portals, portal_vnum, portal_load_room FROM towns");

	if (!result)
		return false;

	P_town   *town_ptr = &towns;
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		const char *zone_filename = row[0];
		bool        found         = false;

		for (int i = 1; i <= top_of_zone_table; i++)
		{
			if (!strcmp(zone_filename, zone_table[i].filename))
			{
				found               = true;
				P_town new_town     = new struct town;
				new_town->next_town = NULL;
				new_town->zone      = &(zone_table[i]);

				new_town->resources = atoi(row[1]);
				new_town->defense   = atoi(row[2]);
				new_town->offense   = atoi(row[3]);

				new_town->deploy_guard    = atoi(row[4]) ? TRUE : FALSE;
				new_town->guard_vnum      = atoi(row[5]);
				new_town->guard_max       = atoi(row[6]);
				new_town->guard_load_room = atoi(row[7]);

				new_town->deploy_cavalry    = atoi(row[8]) ? TRUE : FALSE;
				new_town->cavalry_vnum      = atoi(row[9]);
				new_town->cavalry_max       = atoi(row[10]);
				new_town->cavalry_load_room = atoi(row[11]);

				new_town->deploy_portals   = atoi(row[12]) ? TRUE : FALSE;
				new_town->portal_vnum      = atoi(row[13]);
				new_town->portal_load_room = atoi(row[14]);

				*town_ptr = new_town;
				town_ptr  = &(new_town->next_town);
				break;
			}
		}

		if (!found)
			logit(LOG_DEBUG, "sql_load_towns: zone '%s' not found", zone_filename);
	}

	mysql_free_result(result);
	return true;
}

// account ips

bool sql_save_account_ips(const char *account_name, struct acct_ip *ips)
{
	if (!DB || !account_name)
		return false;

	char *escaped_name = sql_escape_string(account_name);
	if (!escaped_name)
		return false;

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	char del_query[256];
	snprintf(del_query, sizeof(del_query), "DELETE FROM account_ips WHERE account_name='%s'", escaped_name);
	if (!sql_run_query(del_query))
	{
		free(escaped_name);
		if (own_txn)
			sql_rollback();
		return false;
	}

	for (struct acct_ip *ip = ips; ip; ip = ip->next)
	{
		char *escaped_hostname = sql_escape_string(ip->hostname ? ip->hostname : "");
		char *escaped_ip       = sql_escape_string(ip->ip_address ? ip->ip_address : "");

		if (escaped_hostname && escaped_ip)
		{
			char query[512];
			snprintf(query,
			         sizeof(query),
			         "INSERT INTO account_ips (account_name, hostname, ip_address, count) "
			         "VALUES ('%s', '%s', '%s', %lu)",
			         escaped_name,
			         escaped_hostname,
			         escaped_ip,
			         ip->count);
			if (!sql_run_query(query))
			{
				if (escaped_hostname)
					free(escaped_hostname);
				if (escaped_ip)
					free(escaped_ip);
				free(escaped_name);
				if (own_txn)
					sql_rollback();
				return false;
			}
		}

		if (escaped_hostname)
			free(escaped_hostname);
		if (escaped_ip)
			free(escaped_ip);
	}

	free(escaped_name);
	if (own_txn && !sql_commit())
	{
		sql_rollback();
		return false;
	}
	return true;
}

struct acct_ip *sql_load_account_ips(const char *account_name)
{
	if (!DB || !account_name)
		return NULL;

	char *escaped_name = sql_escape_string(account_name);
	if (!escaped_name)
		return NULL;

	char query[256];
	snprintf(query, sizeof(query), "SELECT hostname, ip_address, count FROM account_ips WHERE account_name='%s'", escaped_name);
	free(escaped_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
	{
		sql_account_ips_query_failed = true;
		logit(LOG_DEBUG, "sql_load_account_ips: query failed for %s", account_name);
		return NULL;
	}

	struct acct_ip *head = NULL;
	struct acct_ip *tail = NULL;
	MYSQL_ROW       row;

	while ((row = mysql_fetch_row(result)))
	{
		struct acct_ip *ip = NULL;
		CREATE(ip, struct acct_ip, 1, MEM_TAG_OTHER);
		if (!ip)
			continue;

		ip->hostname   = str_dup(row[0] ? row[0] : "");
		ip->ip_address = str_dup(row[1] ? row[1] : "");
		ip->count      = row[2] ? strtoul(row[2], NULL, 10) : 0;

		if (!head)
			head = ip;
		else
			tail->next = ip;
		tail = ip;
	}

	mysql_free_result(result);
	return head;
}

bool sql_delete_account_ips(const char *account_name)
{
	if (!DB || !account_name)
		return false;

	char *escaped_name = sql_escape_string(account_name);
	if (!escaped_name)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "DELETE FROM account_ips WHERE account_name='%s'", escaped_name);
	free(escaped_name);

	return sql_run_query(query);
}

// kingdom land migration

bool sql_save_kingdom_land(void)
{
	if (!DB)
		return false;

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	if (!sql_run_query("DELETE FROM kingdom_land"))
	{
		if (own_txn)
			sql_rollback();
		return false;
	}

	FILE *f = fopen(SAVE_DIR "/../Kingdoms/kingdom.land", "r");
	if (!f)
	{
		if (own_txn && !sql_commit())
		{
			sql_rollback();
			return false;
		}
		return true;
	}

	char line[256];
	while (fgets(line, sizeof(line), f))
	{
		char type;
		int  kingdom_id, start_vnum, end_vnum;
		int  count = sscanf(line, "%c %d %d", &type, &start_vnum, &end_vnum);

		if (count >= 2)
		{
			if (type == 'H')
			{
				kingdom_id = start_vnum;
				start_vnum = end_vnum;
				end_vnum   = start_vnum;
			}
			else
			{
				kingdom_id = 0;
			}

			if (count == 2 && type != 'H')
				end_vnum = start_vnum;

			char query[256];
			snprintf(query,
			         sizeof(query),
			         "INSERT INTO kingdom_land (kingdom_id, start_vnum, end_vnum, type) "
			         "VALUES (%d, %d, %d, '%c')",
			         kingdom_id,
			         start_vnum,
			         end_vnum,
			         type);
			if (!sql_run_query(query))
			{
				fclose(f);
				if (own_txn)
					sql_rollback();
				return false;
			}
		}
	}

	fclose(f);
	if (own_txn && !sql_commit())
	{
		sql_rollback();
		return false;
	}
	return true;
}

static bool sql_save_corpse_item_affects(int item_id, P_obj obj)
{
	if (!obj || !DB || item_id <= 0)
		return false;

	// Own_txn wrapper for standalone-call safety
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
		{
			// skip duplicates
			bool is_dup = false;
			for (int j = 0; j < i; j++)
			{
				if (obj->affected[j].location == obj->affected[i].location && obj->affected[j].modifier == obj->affected[i].modifier)
				{
					is_dup = true;
					break;
				}
			}
			if (is_dup)
				continue;

			char query[256];
			snprintf(query, sizeof(query), "INSERT INTO corpse_item_affects (item_id, location, modifier) VALUES (%d, %d, %d)", item_id, obj->affected[i].location, obj->affected[i].modifier);
			if (!sql_run_query(query))
			{
				if (own_txn) sql_rollback();
				return false;
			}
		}
	}
	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return false; }
	}
	return true;
}

static int sql_save_corpse_item(int corpse_id, int save_id, P_obj obj, int container_id)
{
	if (!obj || !DB || corpse_id <= 0)
		return 0;

	// Own_txn wrapper for standalone-call safety
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return 0;
		own_txn = true;
	}

	int vnum = obj_index[obj->R_num].virtual_number;
	char corpse_owner[64];
	snprintf(corpse_owner, sizeof(corpse_owner), "corpse:%d", save_id);
	persistence_record_item_event("owner_corpse", obj, NULL, "nowhere",
	                              corpse_owner, "sql_save_corpse_item");


	char *esc_name   = NULL;
	char *esc_short  = NULL;
	char *esc_desc   = NULL;
	char *esc_action = NULL;

	if (obj->str_mask & STRUNG_KEYS)
		esc_name = sql_escape_string(obj->name ? obj->name : "");
	if (obj->str_mask & STRUNG_DESC2)
		esc_short = sql_escape_string(obj->short_description ? obj->short_description : "");
	if (obj->str_mask & STRUNG_DESC1)
		esc_desc = sql_escape_string(obj->description ? obj->description : "");
	if (obj->str_mask & STRUNG_DESC3)
		esc_action = sql_escape_string(obj->action_description ? obj->action_description : "");

	char container_str[32];
	if (container_id > 0)
		snprintf(container_str, sizeof(container_str), "%d", container_id);
	else
		strcpy(container_str, "NULL");

	char name_str[1024], short_str[1024], desc_str[2048], action_str[2048];
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

	char query[8192];
	// Shared helper formats wear_str, type_str, and bv1-5_str
	// (NULL when matching the prototype) and frees the loaded prototype.
	// See sql_format_item_diff_fields_and_free_proto().
	char wear_str[32];
	char type_str[16];
	char material_str[16];
	char bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];
	sql_format_item_diff_fields_and_free_proto(
		obj, wear_str, type_str, material_str,
		bv1_str, bv2_str, bv3_str, bv4_str, bv5_str);

	snprintf(query,
	         sizeof(query),
	         "INSERT INTO corpse_items ("
	         "corpse_id, vnum, container_id, quantity, "
	         "weight, cost, timer, extra_flags, wear_flags, item_type, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, "
	         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
	         "item_material, obj_uid, item_condition"
	         ") VALUES ("
	         "%d, %d, %s, 1, "
	         "%d, %d, %ld, %lu, %s, %s, "
	         "%d, %d, %d, %d, %d, %d, %d, %d, "
	         "%s, %s, %s, %s, "
	         "%s, %s, %s, %s, %s, "
	         "%s, %lu, %d"
	         ")",
	         corpse_id,
	         vnum,
	         container_str,
	         obj->weight,
	         obj->cost,
	         (long)obj->timer[0],
	         (unsigned long)obj->extra_flags,
	         wear_str,
	         type_str,
	         obj->value[0],
	         obj->value[1],
	         obj->value[2],
	         obj->value[3],
	         obj->value[4],
	         obj->value[5],
	         obj->value[6],
	         obj->value[7],
	         name_str,
	         short_str,
	         desc_str,
	         action_str,
	         bv1_str,
	         bv2_str,
	         bv3_str,
	         bv4_str,
	         bv5_str,
	         material_str,
	         obj->obj_uid,
	         obj->condition);

	if (esc_name)
		free(esc_name);
	if (esc_short)
		free(esc_short);
	if (esc_desc)
		free(esc_desc);
	if (esc_action)
		free(esc_action);

	if (!sql_run_query(query))
	{
		logit(LOG_DEBUG, "sql_save_item: insert failed errno=%u sqlerr=%s", mysql_errno(DB), mysql_error(DB));
		if (own_txn) sql_rollback();
		return 0;
	}

	int item_id = (int)mysql_insert_id(DB);

	if (!sql_save_corpse_item_affects(item_id, obj))
	{
		if (own_txn) sql_rollback();
		return 0;
	}

	if (obj->ex_description && !sql_save_item_extra_descr(item_id, obj, "corpse_item_extra_descr"))
	{
		if (own_txn) sql_rollback();
		return 0;
	}

	if (obj->contains)
	{
		for (P_obj content = obj->contains; content; content = content->next_content)
		{
			if (sql_save_corpse_item(corpse_id, save_id, content, item_id) <= 0)
			{
				if (own_txn) sql_rollback();
				return 0;
			}
		}
	}

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return 0; }
	}
	return item_id;
}

bool sql_save_corpse(P_obj corpse)
{
	if (!corpse || !DB)
		return false;

	if (corpse->type != ITEM_CORPSE || !IS_SET(corpse->value[1], PC_CORPSE))
	{
		logit(LOG_DEBUG, "sql_save_corpse: not a PC corpse");
		return false;
	}

	const char *player_name = corpse->action_description;
	if (!player_name || !*player_name)
	{
		logit(LOG_DEBUG, "sql_save_corpse: missing player name");
		return false;
	}

	int save_id = corpse->value[CORPSE_SAVEID];
	if (save_id == 0)
		save_id = time(NULL);

	int room_vnum = 0;
	if (OBJ_ROOM(corpse) && corpse->loc.room > NOWHERE && corpse->loc.room <= top_of_world)
		room_vnum = world[corpse->loc.room].number;
	else if (OBJ_CARRIED(corpse) && corpse->loc.carrying)
		room_vnum = world[corpse->loc.carrying->in_room].number;

	char *esc_name = sql_escape_string(player_name);
	if (!esc_name)
		return false;

	char *esc_sdesc = sql_escape_string(corpse->short_description);
	if (!esc_sdesc)
	{
		free(esc_name);
		return false;
	}

	char *esc_desc = sql_escape_string(corpse->description);
	if (!esc_desc)
	{
		free(esc_name);
		free(esc_sdesc);
		return false;
	}

	// start transaction (must succeed before any writes)
	if (!sql_begin_transaction())
	{
		logit(LOG_DEBUG, "sql_save_corpse: failed to start transaction");
		free(esc_name);
		free(esc_sdesc);
		free(esc_desc);
		return false;
	}

	char del_query[256];
	snprintf(del_query, sizeof(del_query), "DELETE FROM corpses WHERE player_name='%s' AND save_id=%d", esc_name, save_id);
	if (!sql_run_query(del_query))
	{
		logit(LOG_DEBUG, "sql_save_corpse: failed to delete old corpse for %s", esc_name);
		free(esc_name);
		free(esc_sdesc);
		free(esc_desc);
		sql_rollback();
		return false;
	}

	char ins_query[512];
	snprintf(ins_query, sizeof(ins_query), "INSERT INTO corpses (player_name, save_id, room_vnum, short_descr, description) VALUES ('%s', %d, %d, '%s', '%s')", esc_name, save_id, room_vnum, esc_sdesc, esc_desc);
	free(esc_name);
	free(esc_sdesc);
	free(esc_desc);

	if (!sql_run_query(ins_query))
	{
		logit(LOG_DEBUG, "sql_save_corpse: failed to insert corpse for %s", player_name);
		sql_rollback();
		return false;
	}

	int corpse_id = (int)mysql_insert_id(DB);

	// save contained items atomically - any failure rolls back the whole corpse save
	for (P_obj obj = corpse->contains; obj; obj = obj->next_content)
	{
		if (sql_save_corpse_item(corpse_id, save_id, obj, 0) == 0)
		{
			logit(LOG_DEBUG, "sql_save_corpse: failed to save contained item, rolling back");
			sql_rollback();
			return false;
		}
	}

	if (!sql_commit())
	{
		logit(LOG_DEBUG, "sql_save_corpse: failed to commit for %s", player_name);
		sql_rollback();
		return false;
	}

	return true;
}

bool sql_delete_corpse(const char *player_name, int save_id)
{
	if (!player_name || !DB)
		return false;

	char *esc_name = sql_escape_string(player_name);
	if (!esc_name)
		return false;

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
		{
			free(esc_name);
			return false;
		}
		own_txn = true;
	}

	// Delete item affects first (child of corpse_items)
	char cascade_query[512];
	snprintf(cascade_query, sizeof(cascade_query),
		"DELETE FROM corpse_item_affects WHERE item_id IN "
		"(SELECT ci.id FROM corpse_items ci JOIN corpses c ON ci.corpse_id = c.id "
		"WHERE c.player_name='%s' AND c.save_id=%d)", esc_name, save_id);
	if (!sql_run_query(cascade_query))
	{
		free(esc_name);
		if (own_txn) sql_rollback();
		return false;
	}

	// Delete corpse items next
	snprintf(cascade_query, sizeof(cascade_query),
		"DELETE FROM corpse_items WHERE corpse_id IN "
		"(SELECT id FROM corpses WHERE player_name='%s' AND save_id=%d)", esc_name, save_id);
	if (!sql_run_query(cascade_query))
	{
		free(esc_name);
		if (own_txn) sql_rollback();
		return false;
	}

	// Delete the corpse itself
	char query[256];
	snprintf(query, sizeof(query), "DELETE FROM corpses WHERE player_name='%s' AND save_id=%d", esc_name, save_id);
	free(esc_name);
	if (!sql_run_query(query))
	{
		if (own_txn) sql_rollback();
		return false;
	}

	if (own_txn)
	{
		if (!sql_commit()) { sql_rollback(); return false; }
	}

	return true;
}

// single query corpse loading - all data in one query
#define MAX_CORPSE_ITEMS 512

extern int skip_corpse_save;

bool sql_load_all_corpses(void)
{
	if (!DB)
		return false;

	skip_corpse_save = 1; // don't write corpses back during load

	bool ok = false;
	MYSQL_RES *result = NULL;
	int       total_rows = 0;
	int       cur_corpse_id = -1;
	P_obj     cur_corpse = NULL;
	int       cur_room = 0;
	P_obj     obj_map[MAX_CORPSE_ITEMS];
	int       id_map[MAX_CORPSE_ITEMS];
	int       container_map[MAX_CORPSE_ITEMS];
	int       num_objs = 0;
	int       last_item_id = -1;
	int       skipped_item_id = -1;
	int       loaded = 0;
	MYSQL_ROW row;

	// one query gets everything: corpses + items + affects
	result = db_query("SELECT c.id, c.player_name, c.save_id, c.room_vnum, "
	                  "ci.id, COALESCE(ci.container_id, 0), ci.vnum, COALESCE(ci.item_type, 0), "
	                  "ci.weight, ci.cost, ci.timer, "
	                  "ci.extra_flags, ci.value0, ci.value1, ci.value2, ci.value3, ci.value4, "
	                  "ci.value5, ci.value6, ci.value7, ci.name, ci.short_descr, ci.description, "
	                  "ci.action_descr, COALESCE(cia.location, -1), COALESCE(cia.modifier, 0), "
	                  "ci.obj_uid, ci.item_condition, "
	                  "c.short_descr, c.description, "
	                  "ci.wear_flags, ci.item_type, ci.item_material, "
	                  "ci.bitvector1, ci.bitvector2, ci.bitvector3, ci.bitvector4, ci.bitvector5 "
	                  "FROM corpses c "
	                  "LEFT JOIN corpse_items ci ON ci.corpse_id = c.id "
	                  "LEFT JOIN corpse_item_affects cia ON cia.item_id = ci.id "
	                  "ORDER BY c.id, ci.id, cia.id");
	if (!result)
		goto cleanup;

	total_rows = mysql_num_rows(result);

	// tracking for current corpse being built
	while ((row = mysql_fetch_row(result)))
	{

		int corpse_id = atoi(row[0]);
		int item_id   = row[4] ? atoi(row[4]) : 0;

		// new corpse - finalize previous one first
		if (corpse_id != cur_corpse_id)
		{
			// finalize previous corpse if exists
			if (cur_corpse && num_objs > 0)
			{
// link containers using hash
#define HASH_SIZE        1024
				int hash_id[HASH_SIZE];
				int hash_idx[HASH_SIZE];
				for (int i = 0; i < HASH_SIZE; i++)
					hash_id[i] = -1;

				for (int i = 0; i < num_objs; i++)
				{
					int h = id_map[i] % HASH_SIZE;
					while (hash_id[h] != -1)
						h = (h + 1) % HASH_SIZE;
					hash_id[h]  = id_map[i];
					hash_idx[h] = i;
				}

				for (int i = 0; i < num_objs; i++)
				{
					if (container_map[i] == 0)
						continue;
					int h = container_map[i] % HASH_SIZE;
					while (hash_id[h] != -1 && hash_id[h] != container_map[i])
						h = (h + 1) % HASH_SIZE;
					if (hash_id[h] == container_map[i])
					{
						int j = hash_idx[h];
						if (!obj_can_nest(obj_map[i], obj_map[j]))
						{
							logit(LOG_DEBUG, "sql_restore_saved_items: skipping malformed container link %d -> %d", obj_map[i]->db_item_id, obj_map[j]->db_item_id);
							continue;
						}
						obj_map[i]->next_content = obj_map[j]->contains;
						obj_map[j]->contains     = obj_map[i];
						obj_map[i]->loc_p        = LOC_INSIDE;
						obj_map[i]->loc.inside   = obj_map[j];
						container_map[i]         = -1;
					}
				}
#undef HASH_SIZE

				// build top-level list
				P_obj first    = NULL;
				P_obj last_obj = NULL;
				for (int i = 0; i < num_objs; i++)
				{
					if (container_map[i] == 0)
					{
						if (!first)
							first = obj_map[i];
						else
							last_obj->next_content = obj_map[i];
						last_obj               = obj_map[i];
						last_obj->next_content = NULL;
					}
				}
				cur_corpse->contains = first;
				for (P_obj o = cur_corpse->contains; o; o = o->next_content)
				{
					o->loc_p      = LOC_INSIDE;
					o->loc.inside = cur_corpse;
				}
				obj_to_room(cur_corpse, cur_room);
				persistence_refresh_restored_corpse(cur_corpse, "sql_load_all_corpses");
				loaded++;
			}
			else if (cur_corpse)
			{
				// corpse with no items
				obj_to_room(cur_corpse, cur_room);
				persistence_refresh_restored_corpse(cur_corpse, "sql_load_all_corpses");
				loaded++;
			}

			// start new corpse
			num_objs      = 0;
			last_item_id  = -1;
			skipped_item_id = -1;
			cur_corpse_id = corpse_id;

			const char *player_name = row[1] ? row[1] : "";
			int         save_id     = atoi(row[2]);
			int         room_vnum   = atoi(row[3]);

			cur_room = real_room(room_vnum);
			if (cur_room == NOWHERE)
				cur_room = 0;

			int corpse_rnum = real_object(2); // vnum 2 is the corpse prototype
			if (corpse_rnum < 0)
			{
				cur_corpse = NULL;
				continue;
			}

			cur_corpse = read_object(corpse_rnum, REAL);
			if (!cur_corpse)
				continue;

			cur_corpse->type = ITEM_CORPSE;
			SET_BIT(cur_corpse->value[1], PC_CORPSE);
			cur_corpse->value[CORPSE_SAVEID] = save_id;

			if (cur_corpse->action_description)
				FREE(cur_corpse->action_description);
			cur_corpse->action_description = str_dup(player_name);

			if (row[27])
			{
				//FREE(cur_corpse->short_description);
				cur_corpse->short_description = str_dup(row[27]);
			}
			if (row[28])
			{
				//FREE(cur_corpse->description);
				cur_corpse->description = str_dup(row[28]);
			}
		}

		// no item in this row (corpse with no items)
		if (!row[4] || !cur_corpse)
			continue;

		// same item, just another affect
		if (item_id == last_item_id && num_objs > 0)
		{
			int aff_loc = atoi(row[24]);
			if (aff_loc >= 0)
			{
				P_obj obj = obj_map[num_objs - 1];
				for (int i = 0; i < MAX_OBJ_AFFECT; i++)
				{
					if (obj->affected[i].location == 0 && obj->affected[i].modifier == 0)
					{
						obj->affected[i].location = aff_loc;
						obj->affected[i].modifier = atoi(row[25]);
						break;
					}
				}
			}
			continue;
		}

		// new item
		if (num_objs >= MAX_CORPSE_ITEMS)
			continue;

		int vnum = atoi(row[6]);
		int rnum = real_object(vnum);
		if (rnum < 0)
		{
			last_item_id = item_id;
			continue;
		}

		P_obj obj = read_object(rnum, REAL);
		if (!obj)
		{
			last_item_id = item_id;
			continue;
		}

		if (row[8])
			obj->weight = atoi(row[8]);
		if (row[9])
			obj->cost = atoi(row[9]);
		if (row[10])
			obj->timer[0] = atol(row[10]);
		if (row[11])
			obj->extra_flags = strtoul(row[11], NULL, 10);
		for (int v = 0; v < 8; v++)
			obj->value[v] = row[12 + v] ? atoi(row[12 + v]) : 0;

		if (row[20] && row[20][0])
		{
			obj->name = str_dup(row[20]);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (row[21] && row[21][0])
		{
			obj->short_description = str_dup(row[21]);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (row[22] && row[22][0])
		{
			obj->description = str_dup(row[22]);
			obj->str_mask |= STRUNG_DESC1;
		}
		if (row[23] && row[23][0])
		{
			obj->action_description = str_dup(row[23]);
			obj->str_mask |= STRUNG_DESC3;
		}

		unsigned long saved_uid = row[26] ? strtoul(row[26], NULL, 10) : 0;
		if (saved_uid > 0)
		{
			obj->obj_uid = saved_uid;
			if (obj->obj_uid >= next_obj_uid)
				next_obj_uid = obj->obj_uid + 1;
		}
		if (row[27])
			obj->condition = atoi(row[27]);

		// v19 diff columns - NULL means use prototype value from read_object()
		if (row[30])
			obj->wear_flags = atoi(row[30]);
		if (row[31])
			obj->type = sql_validate_loaded_item_type(obj, atoi(row[31]), "sql_load_all_corpses");
		if (row[32])
			obj->material = atoi(row[32]);
		if (row[33])
			obj->bitvector = strtoul(row[33], NULL, 10);
		if (row[34])
			obj->bitvector2 = strtoul(row[34], NULL, 10);
		if (row[35])
			obj->bitvector3 = strtoul(row[35], NULL, 10);
		if (row[36])
			obj->bitvector4 = strtoul(row[36], NULL, 10);
		if (row[37])
			obj->bitvector5 = strtoul(row[37], NULL, 10);

		char owner_ref[32];
		snprintf(owner_ref, sizeof(owner_ref), "%d", cur_corpse->value[CORPSE_SAVEID]);
		if (!sql_persistence_item_owner_matches(saved_uid, "corpse", owner_ref, "sql_load_all_corpses"))
		{
			extract_obj(obj, FALSE);
			skipped_item_id = item_id;
			continue;
		}

		int aff_loc = atoi(row[24]);
		if (aff_loc >= 0)
		{
			obj->affected[0].location = aff_loc;
			obj->affected[0].modifier = atoi(row[25]);
		}

		sql_load_item_extra_descr_from_table(item_id, obj, "corpse_item");

		obj_map[num_objs]       = obj;
		id_map[num_objs]        = item_id;
		container_map[num_objs] = atoi(row[5]);
		num_objs++;
		last_item_id = item_id;
	}

	// finalize last corpse
	if (cur_corpse && num_objs > 0)
	{
#define HASH_SIZE 1024
		int hash_id[HASH_SIZE];
		int hash_idx[HASH_SIZE];
		for (int i = 0; i < HASH_SIZE; i++)
			hash_id[i] = -1;

		for (int i = 0; i < num_objs; i++)
		{
			int h = id_map[i] % HASH_SIZE;
			while (hash_id[h] != -1)
				h = (h + 1) % HASH_SIZE;
			hash_id[h]  = id_map[i];
			hash_idx[h] = i;
		}

		for (int i = 0; i < num_objs; i++)
		{
			if (container_map[i] == 0)
				continue;
			int h = container_map[i] % HASH_SIZE;
			while (hash_id[h] != -1 && hash_id[h] != container_map[i])
				h = (h + 1) % HASH_SIZE;
			if (hash_id[h] == container_map[i])
			{
				int j = hash_idx[h];
				if (!obj_can_nest(obj_map[i], obj_map[j]))
				{
					logit(LOG_DEBUG, "sql_restore_saved_items: skipping malformed container link %d -> %d", obj_map[i]->db_item_id, obj_map[j]->db_item_id);
					continue;
				}
				obj_map[i]->next_content = obj_map[j]->contains;
				obj_map[j]->contains     = obj_map[i];
				obj_map[i]->loc_p        = LOC_INSIDE;
				obj_map[i]->loc.inside   = obj_map[j];
				container_map[i]         = -1;
			}
		}
#undef HASH_SIZE

		P_obj first    = NULL;
		P_obj last_obj = NULL;
		for (int i = 0; i < num_objs; i++)
		{
			if (container_map[i] == 0)
			{
				if (!first)
					first = obj_map[i];
				else
					last_obj->next_content = obj_map[i];
				last_obj               = obj_map[i];
				last_obj->next_content = NULL;
			}
		}
		cur_corpse->contains = first;
		for (P_obj o = cur_corpse->contains; o; o = o->next_content)
		{
			o->loc_p      = LOC_INSIDE;
			o->loc.inside = cur_corpse;
		}
		obj_to_room(cur_corpse, cur_room);
		persistence_refresh_restored_corpse(cur_corpse, "sql_load_all_corpses");
		loaded++;
	}
	else if (cur_corpse)
	{
		obj_to_room(cur_corpse, cur_room);
		persistence_refresh_restored_corpse(cur_corpse, "sql_load_all_corpses");
		loaded++;
	}

	ok = true;

cleanup:
	if (result)
		mysql_free_result(result);
	skip_corpse_save = 0; // re-enable corpse saves
	return ok;
}

extern struct shop_data *shop_index;
extern int               number_of_shops;

static bool sql_save_shopkeeper_item_affects(int item_id, P_obj obj)
{
	if (!obj || !DB || item_id <= 0)
		return false;

	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
		{
			// skip duplicates
			bool is_dup = false;
			for (int j = 0; j < i; j++)
			{
				if (obj->affected[j].location == obj->affected[i].location && obj->affected[j].modifier == obj->affected[i].modifier)
				{
					is_dup = true;
					break;
				}
			}
			if (is_dup)
				continue;

			char query[256];
			snprintf(query, sizeof(query), "INSERT INTO shopkeeper_item_affects (item_id, location, modifier) VALUES (%d, %d, %d)", item_id, obj->affected[i].location, obj->affected[i].modifier);
			if (!sql_run_query(query))
				return false;
		}
	}
	return true;
}

static int sql_save_shopkeeper_item(int shopkeeper_id, P_obj obj, int equip_slot, int container_id)
{
	if (!obj || !DB || shopkeeper_id <= 0)
		return 0;

	int vnum = obj_index[obj->R_num].virtual_number;

	char *esc_name   = NULL;
	char *esc_short  = NULL;
	char *esc_desc   = NULL;
	char *esc_action = NULL;

	if (obj->str_mask & STRUNG_KEYS)
		esc_name = sql_escape_string(obj->name ? obj->name : "");
	if (obj->str_mask & STRUNG_DESC2)
		esc_short = sql_escape_string(obj->short_description ? obj->short_description : "");
	if (obj->str_mask & STRUNG_DESC1)
		esc_desc = sql_escape_string(obj->description ? obj->description : "");
	if (obj->str_mask & STRUNG_DESC3)
		esc_action = sql_escape_string(obj->action_description ? obj->action_description : "");

	char container_str[32];
	if (container_id > 0)
		snprintf(container_str, sizeof(container_str), "%d", container_id);
	else
		strcpy(container_str, "NULL");

	char name_str[1024], short_str[1024], desc_str[2048], action_str[2048];
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

	char query[8192];
	// Shared helper formats wear_str, type_str, and bv1-5_str
	// (NULL when matching the prototype) and frees the loaded prototype.
	// See sql_format_item_diff_fields_and_free_proto().
	char wear_str[32];
	char type_str[16];
	char material_str[16];
	char bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];
	sql_format_item_diff_fields_and_free_proto(
		obj, wear_str, type_str, material_str,
		bv1_str, bv2_str, bv3_str, bv4_str, bv5_str);

	snprintf(query,
	         sizeof(query),
	         "INSERT INTO shopkeeper_items ("
	         "shopkeeper_id, vnum, equip_slot, container_id, quantity, "
	         "weight, cost, timer, extra_flags, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, "
	         "wear_flags, item_type, item_material, "
	         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5"
	         ") VALUES ("
	         "%d, %d, %d, %s, 1, "
	         "%d, %d, %ld, %lu, "
	         "%d, %d, %d, %d, %d, %d, %d, %d, "
	         "%s, %s, %s, %s, "
	         "%s, %s, %s, "
	         "%s, %s, %s, %s, %s"
	         ")",
	         shopkeeper_id,
	         vnum,
	         equip_slot,
	         container_str,
	         obj->weight,
	         obj->cost,
	         (long)obj->timer[0],
	         (unsigned long)obj->extra_flags,
	         obj->value[0],
	         obj->value[1],
	         obj->value[2],
	         obj->value[3],
	         obj->value[4],
	         obj->value[5],
	         obj->value[6],
	         obj->value[7],
	         name_str,
	         short_str,
	         desc_str,
	         action_str,
	         wear_str,
	         type_str,
	         material_str,
	         bv1_str,
	         bv2_str,
	         bv3_str,
	         bv4_str,
	         bv5_str);

	if (esc_name)
		free(esc_name);
	if (esc_short)
		free(esc_short);
	if (esc_desc)
		free(esc_desc);
	if (esc_action)
		free(esc_action);

	if (!sql_run_query(query))
		return 0;

	int item_id = (int)mysql_insert_id(DB);

	if (!sql_save_shopkeeper_item_affects(item_id, obj))
		return 0;

	if (obj->contains)
	{
		for (P_obj content = obj->contains; content; content = content->next_content)
		{
			if (!sql_save_shopkeeper_item(shopkeeper_id, content, 0, item_id))
				return 0;
		}
	}

	return item_id;
}

static bool sql_save_shopkeeper_affects(int shopkeeper_id, P_char ch)
{
	if (!ch || !DB || shopkeeper_id <= 0)
		return false;

	for (struct affected_type *af = ch->affected; af; af = af->next)
	{
		if (IS_SET(af->flags, AFFTYPE_NOSAVE))
			continue;

		char query[512];
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO shopkeeper_affects (shopkeeper_id, type, duration, modifier, location, "
		         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5) "
		         "VALUES (%d, %d, %d, %d, %d, %lu, %lu, %lu, %lu, %lu)",
		         shopkeeper_id,
		         af->type,
		         af->duration,
		         af->modifier,
		         af->location,
		         af->bitvector,
		         af->bitvector2,
		         af->bitvector3,
		         af->bitvector4,
		         af->bitvector5);
		if (!sql_run_query(query))
			return false;
	}

	return true;
}

bool sql_save_shopkeeper(P_char ch, int shop_nr)
{
	if (!ch || !DB || shop_nr < 0)
		return false;

	if (IS_PC(ch) || !IS_SHOPKEEPER(ch))
		return false;

	// start transaction
	if (!sql_begin_transaction())
	{
		logit(LOG_DEBUG, "sql_save_shopkeeper: failed to start transaction for shop %d", shop_nr);
		return false;
	}

	int  mob_vnum  = mob_index[GET_RNUM(ch)].virtual_number;
	int  room_vnum = world[ch->in_room].number;
	long save_time = time(0);

	char del_query[128];
	snprintf(del_query, sizeof(del_query), "DELETE FROM shopkeepers WHERE shop_id=%d", shop_nr);
	if (!sql_run_query(del_query))
	{
		logit(LOG_DEBUG, "sql_save_shopkeeper: failed to delete old shopkeeper %d", shop_nr);
		sql_rollback();
		return false;
	}

	char ins_query[256];
	snprintf(
		ins_query, sizeof(ins_query), "INSERT INTO shopkeepers (shop_id, mob_vnum, room_vnum, save_time) VALUES (%d, %d, %d, FROM_UNIXTIME(NULLIF(%ld,0)))", shop_nr, mob_vnum, room_vnum, save_time);

	if (!sql_run_query(ins_query))
	{
		logit(LOG_DEBUG, "sql_save_shopkeeper: failed to insert shopkeeper %d", shop_nr);
		sql_rollback();
		return false;
	}

	int shopkeeper_id = (int)mysql_insert_id(DB);

	if (!sql_save_shopkeeper_affects(shopkeeper_id, ch))
	{
		logit(LOG_DEBUG, "sql_save_shopkeeper: failed to save affects for shop %d", shop_nr);
		sql_rollback();
		return false;
	}

	for (int i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
		{
			if (!sql_save_shopkeeper_item(shopkeeper_id, ch->equipment[i], i + 1, 0))
			{
				logit(LOG_DEBUG, "sql_save_shopkeeper: failed to save equip slot %d for shop %d", i, shop_nr);
				sql_rollback();
				return false;
			}
		}
	}

	for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
	{
		// skip producing items - they're regenerated from zone definitions
		if (shop_producing(obj, shop_nr))
			continue;
		if (!sql_save_shopkeeper_item(shopkeeper_id, obj, 0, 0))
		{
			logit(LOG_DEBUG, "sql_save_shopkeeper: failed to save inventory item for shop %d", shop_nr);
			sql_rollback();
			return false;
		}
	}

	if (!sql_commit())
	{
		logit(LOG_DEBUG, "sql_save_shopkeeper: failed to commit for shop %d", shop_nr);
		sql_rollback();
		return false;
	}

	return true;
}

bool sql_delete_shopkeeper(int shop_nr)
{
	if (!DB || shop_nr < 0)
		return false;

	char query[128];
	snprintf(query, sizeof(query), "DELETE FROM shopkeepers WHERE shop_id=%d", shop_nr);
	return sql_run_query(query);
}

static bool sql_save_saved_item_affects(int item_id, P_obj obj)
{
	if (!obj || !DB || item_id <= 0)
		return false;

	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
		{
			char query[256];
			snprintf(query, sizeof(query), "INSERT INTO saved_item_affects (item_id, location, modifier) VALUES (%d, %d, %d)", item_id, obj->affected[i].location, obj->affected[i].modifier);
			if (!sql_run_query(query))
				return false;
		}
	}
	return true;
}

static int sql_save_saved_item_recursive(const char *item_key, int room_vnum, P_obj obj, int container_id)
{
	if (!obj || !DB)
		return 0;

	int vnum = obj_index[obj->R_num].virtual_number;

	char *esc_name   = NULL;
	char *esc_short  = NULL;
	char *esc_desc   = NULL;
	char *esc_action = NULL;

	if (obj->str_mask & STRUNG_KEYS)
		esc_name = sql_escape_string(obj->name ? obj->name : "");
	if (obj->str_mask & STRUNG_DESC2)
		esc_short = sql_escape_string(obj->short_description ? obj->short_description : "");
	if (obj->str_mask & STRUNG_DESC1)
		esc_desc = sql_escape_string(obj->description ? obj->description : "");
	if (obj->str_mask & STRUNG_DESC3)
		esc_action = sql_escape_string(obj->action_description ? obj->action_description : "");

	char container_str[32];
	if (container_id > 0)
		snprintf(container_str, sizeof(container_str), "%d", container_id);
	else
		strcpy(container_str, "NULL");

	char name_str[1024], short_str[1024], desc_str[2048], action_str[2048];
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

	char *esc_key = sql_escape_string(item_key);

	char query[8192];
	// Shared helper formats wear_str, type_str, and bv1-5_str
	// (NULL when matching the prototype) and frees the loaded prototype.
	// See sql_format_item_diff_fields_and_free_proto().
	char wear_str[32];
	char type_str[16];
	char material_str[16];
	char bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];
	sql_format_item_diff_fields_and_free_proto(
		obj, wear_str, type_str, material_str,
		bv1_str, bv2_str, bv3_str, bv4_str, bv5_str);

	snprintf(query,
	         sizeof(query),
	         "INSERT INTO saved_items ("
	         "item_key, room_vnum, vnum, container_id, quantity, "
	         "weight, cost, timer, extra_flags, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, wear_flags, item_type, bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
	         "item_material"
	         ") VALUES ("
	         "'%s', %d, %d, %s, 1, "
	         "%d, %d, %ld, %lu, "
	         "%d, %d, %d, %d, %d, %d, %d, %d, "
	         "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, "
	         "%s"
	         ")",
	         esc_key ? esc_key : "",
	         room_vnum,
	         vnum,
	         container_str,
	         obj->weight,
	         obj->cost,
	         (long)obj->timer[0],
	         (unsigned long)obj->extra_flags,
	         obj->value[0],
	         obj->value[1],
	         obj->value[2],
	         obj->value[3],
	         obj->value[4],
	         obj->value[5],
	         obj->value[6],
	         obj->value[7],
	         name_str,
	         short_str,
	         desc_str,
	         action_str,
	         wear_str,
	         type_str,
	         bv1_str,
	         bv2_str,
	         bv3_str,
	         bv4_str,
	         bv5_str,
	         material_str);

	if (esc_key)
		free(esc_key);
	if (esc_name)
		free(esc_name);
	if (esc_short)
		free(esc_short);
	if (esc_desc)
		free(esc_desc);
	if (esc_action)
		free(esc_action);

	if (!sql_run_query(query))
		return 0;

	int item_id = (int)mysql_insert_id(DB);

	if (!sql_save_saved_item_affects(item_id, obj))
		return 0;

	if (obj->contains)
	{
		for (P_obj content = obj->contains; content; content = content->next_content)
		{
			if (sql_save_saved_item_recursive(item_key, room_vnum, content, item_id) <= 0)
				return 0;
		}
	}

	return item_id;
}

bool sql_save_saved_item(P_obj item, const char *item_key)
{
	if (!item || !item_key || !DB)
		return false;

	if (!OBJ_ROOM(item) || item->loc.room <= NOWHERE || item->loc.room > top_of_world)
		return false;

	int room_vnum = world[item->loc.room].number;

	bool  own_txn = false;
	bool  ok      = false;
	char *esc_key = sql_escape_string(item_key);
	if (!esc_key)
		return false;

	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
		{
			free(esc_key);
			return false;
		}
		own_txn = true;
	}

	char del_query[256];
	snprintf(del_query, sizeof(del_query), "DELETE FROM saved_items WHERE item_key='%s'", esc_key);
	free(esc_key);
	if (!sql_run_query(del_query))
		goto done;

	ok = sql_save_saved_item_recursive(item_key, room_vnum, item, 0) > 0;

done:
	if (own_txn)
	{
		if (ok)
		{
			if (!sql_commit())
			{
				sql_rollback();
				return false;
			}
		}
		else
		{
			sql_rollback();
		}
	}
	return ok;
}

bool sql_delete_saved_item(const char *item_key)
{
	if (!item_key || !DB)
		return false;

	char *esc_key = sql_escape_string(item_key);
	if (!esc_key)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "DELETE FROM saved_items WHERE item_key='%s'", esc_key);
	free(esc_key);

	return sql_run_query(query);
}

static bool sql_save_siege_item_affects(int item_id, P_obj obj)
{
	if (!obj || !DB || item_id <= 0)
		return false;

	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
		{
			char query[256];
			snprintf(query, sizeof(query), "INSERT INTO siege_item_affects (item_id, location, modifier) VALUES (%d, %d, %d)", item_id, obj->affected[i].location, obj->affected[i].modifier);
			if (!sql_run_query(query))
				return false;
		}
	}
	return true;
}

static int sql_save_siege_item_one(int room_vnum, P_obj obj, int container_id)
{
	if (!obj || !DB)
		return 0;

	int vnum = obj_index[obj->R_num].virtual_number;

	char *esc_name   = NULL;
	char *esc_short  = NULL;
	char *esc_desc   = NULL;
	char *esc_action = NULL;

	if (obj->str_mask & STRUNG_KEYS)
		esc_name = sql_escape_string(obj->name ? obj->name : "");
	if (obj->str_mask & STRUNG_DESC2)
		esc_short = sql_escape_string(obj->short_description ? obj->short_description : "");
	if (obj->str_mask & STRUNG_DESC1)
		esc_desc = sql_escape_string(obj->description ? obj->description : "");
	if (obj->str_mask & STRUNG_DESC3)
		esc_action = sql_escape_string(obj->action_description ? obj->action_description : "");

	char container_str[32];
	if (container_id > 0)
		snprintf(container_str, sizeof(container_str), "%d", container_id);
	else
		strcpy(container_str, "NULL");

	char name_str[1024], short_str[1024], desc_str[2048], action_str[2048];
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

	char query[8192];
	// Shared helper formats wear_str, type_str, and bv1-5_str
	// (NULL when matching the prototype) and frees the loaded prototype.
	// See sql_format_item_diff_fields_and_free_proto().
	char wear_str[32];
	char type_str[16];
	char material_str[16];
	char bv1_str[32], bv2_str[32], bv3_str[32], bv4_str[32], bv5_str[32];
	sql_format_item_diff_fields_and_free_proto(
		obj, wear_str, type_str, material_str,
		bv1_str, bv2_str, bv3_str, bv4_str, bv5_str);

	snprintf(query,
	         sizeof(query),
	         "INSERT INTO siege_items ("
	         "room_vnum, vnum, container_id, quantity, "
	         "weight, cost, timer, extra_flags, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, wear_flags, item_type, bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
	         "item_material"
	         ") VALUES ("
	         "%d, %d, %s, 1, "
	         "%d, %d, %ld, %lu, "
	         "%d, %d, %d, %d, %d, %d, %d, %d, "
	         "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, "
	         "%s"
	         ")",
	         room_vnum,
	         vnum,
	         container_str,
	         obj->weight,
	         obj->cost,
	         (long)obj->timer[0],
	         (unsigned long)obj->extra_flags,
	         obj->value[0],
	         obj->value[1],
	         obj->value[2],
	         obj->value[3],
	         obj->value[4],
	         obj->value[5],
	         obj->value[6],
	         obj->value[7],
	         name_str,
	         short_str,
	         desc_str,
	         action_str,
	         wear_str,
	         type_str,
	         bv1_str,
	         bv2_str,
	         bv3_str,
	         bv4_str,
	         bv5_str,
	         material_str);

	if (esc_name)
		free(esc_name);
	if (esc_short)
		free(esc_short);
	if (esc_desc)
		free(esc_desc);
	if (esc_action)
		free(esc_action);

	if (!sql_run_query(query))
		return 0;

	int item_id = (int)mysql_insert_id(DB);

	if (!sql_save_siege_item_affects(item_id, obj))
		return 0;

	if (obj->contains)
	{
		for (P_obj content = obj->contains; content; content = content->next_content)
		{
			if (sql_save_siege_item_one(room_vnum, content, item_id) <= 0)
				return 0;
		}
	}

	return item_id;
}

bool sql_save_siege_item(P_obj obj, int room_vnum)
{
	if (!obj || !DB)
		return false;

	return sql_save_siege_item_one(room_vnum, obj, 0) > 0;
}

bool sql_save_siege_list(void)
{
	if (!DB)
		return false;

	if (!sql_run_query("DELETE FROM siege_items"))
		return false;
	return true;
}

bool sql_delete_siege_items(int room_vnum)
{
	if (!DB)
		return false;

	char query[128];
	snprintf(query, sizeof(query), "DELETE FROM siege_items WHERE room_vnum=%d", room_vnum);
	return sql_run_query(query);
}

// temp struct for batched item loading
struct shopkeeper_item_temp
{
	int                          item_id;
	int                          container_id;
	int                          equip_slot;
	P_obj                        obj;
	struct shopkeeper_item_temp *next;
};

static void sql_load_all_shopkeeper_items(int shopkeeper_id, P_obj equipment[], P_obj *inventory)
{
	if (!DB || shopkeeper_id <= 0)
		return;

	// load all items in one query
	char query[512];
	snprintf(query,
	         sizeof(query),
	         "SELECT id, vnum, equip_slot, weight, cost, timer, extra_flags, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, "
	         "wear_flags, item_type, item_material, "
	         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, "
	         "container_id "
	         "FROM shopkeeper_items WHERE shopkeeper_id=%d ORDER BY id",
	         shopkeeper_id);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return;

	// first pass: create all objects and store metadata
	struct shopkeeper_item_temp *items      = NULL;
	struct shopkeeper_item_temp *last_item  = NULL;
	int                          item_count = 0;
	MYSQL_ROW                    row;

	while ((row = mysql_fetch_row(result)))
	{
		int item_id = atoi(row[0]);
		int vnum    = atoi(row[1]);
		int rnum    = real_object(vnum);
		if (rnum < 0)
			continue;

		P_obj obj = read_object(rnum, REAL);
		if (!obj)
			continue;

		int equip_slot   = atoi(row[2]);
		int container_id = row[27] ? atoi(row[27]) : 0;

		if (row[3])
			obj->weight = atoi(row[3]);
		if (row[4])
			obj->cost = atoi(row[4]);
		if (row[5])
			obj->timer[0] = atol(row[5]);
		if (row[6])
			obj->extra_flags = strtoul(row[6], NULL, 10);

		obj->value[0] = row[7] ? atoi(row[7]) : 0;
		obj->value[1] = row[8] ? atoi(row[8]) : 0;
		obj->value[2] = row[9] ? atoi(row[9]) : 0;
		obj->value[3] = row[10] ? atoi(row[10]) : 0;
		obj->value[4] = row[11] ? atoi(row[11]) : 0;
		obj->value[5] = row[12] ? atoi(row[12]) : 0;
		obj->value[6] = row[13] ? atoi(row[13]) : 0;
		obj->value[7] = row[14] ? atoi(row[14]) : 0;

		if (row[15] && strlen(row[15]) > 0)
		{
			obj->name = str_dup(row[15]);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (row[16] && strlen(row[16]) > 0)
		{
			obj->short_description = str_dup(row[16]);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (row[17] && strlen(row[17]) > 0)
		{
			obj->description = str_dup(row[17]);
			obj->str_mask |= STRUNG_DESC1;
		}
		if (row[18] && strlen(row[18]) > 0)
		{
			obj->action_description = str_dup(row[18]);
			obj->str_mask |= STRUNG_DESC3;
		}
		// v19 diff columns - NULL means use prototype value from read_object()
		if (row[19])
			obj->wear_flags = atoi(row[19]);
		if (row[20])
			obj->type = sql_validate_loaded_item_type(obj, atoi(row[20]), "sql_load_shopkeeper_items");
		if (row[21])
			obj->material = atoi(row[21]);
		if (row[22])
			obj->bitvector = strtoul(row[22], NULL, 10);
		if (row[23])
			obj->bitvector2 = strtoul(row[23], NULL, 10);
		if (row[24])
			obj->bitvector3 = strtoul(row[24], NULL, 10);
		if (row[25])
			obj->bitvector4 = strtoul(row[25], NULL, 10);
		if (row[26])
			obj->bitvector5 = strtoul(row[26], NULL, 10);



		struct shopkeeper_item_temp *temp = (struct shopkeeper_item_temp *)malloc(sizeof(struct shopkeeper_item_temp));
		temp->item_id                     = item_id;
		temp->container_id                = container_id;
		temp->equip_slot                  = equip_slot;
		temp->obj                         = obj;
		temp->next                        = NULL;

		if (!items)
			items = temp;
		else
			last_item->next = temp;
		last_item = temp;
		item_count++;
	}
	mysql_free_result(result);

	if (item_count == 0)
		return;

	// load all item affects in one query
	snprintf(query,
	         sizeof(query),
	         "SELECT sia.item_id, sia.location, sia.modifier "
	         "FROM shopkeeper_item_affects sia "
	         "INNER JOIN shopkeeper_items si ON sia.item_id = si.id "
	         "WHERE si.shopkeeper_id=%d ORDER BY sia.item_id",
	         shopkeeper_id);

	result = db_query("%s", query);
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int aff_item_id = atoi(row[0]);
			int location    = atoi(row[1]);
			int modifier    = atoi(row[2]);

			// find the item
			for (struct shopkeeper_item_temp *t = items; t; t = t->next)
			{
				if (t->item_id == aff_item_id)
				{
					for (int i = 0; i < MAX_OBJ_AFFECT; i++)
					{
						if (t->obj->affected[i].location == 0 && t->obj->affected[i].modifier == 0)
						{
							t->obj->affected[i].location = location;
							t->obj->affected[i].modifier = modifier;
							break;
						}
					}
					break;
				}
			}
		}
		mysql_free_result(result);
	}

	// link container contents
	for (struct shopkeeper_item_temp *t = items; t; t = t->next)
	{
		if (t->container_id > 0)
		{
			// find parent container
			for (struct shopkeeper_item_temp *p = items; p; p = p->next)
			{
				if (p->item_id == t->container_id)
				{
					if (!obj_can_nest(t->obj, p->obj))
					{
						logit(LOG_DEBUG, "sql_load_all_shopkeeper_items: skipping malformed container link %d -> %d", t->item_id, p->item_id);
						break;
					}
					t->obj->next_content = p->obj->contains;
					p->obj->contains     = t->obj;
					t->obj->loc_p        = LOC_INSIDE;
					t->obj->loc.inside   = p->obj;
					break;
				}
			}
		}
	}

	// assign equipment and inventory
	P_obj inv_first = NULL;
	P_obj inv_last  = NULL;

	for (struct shopkeeper_item_temp *t = items; t; t = t->next)
	{
		if (t->container_id > 0)
			continue; // already placed in container

		if (t->equip_slot > 0 && t->equip_slot <= MAX_WEAR)
			equipment[t->equip_slot - 1] = t->obj;
		else
		{
			if (!inv_first)
				inv_first = t->obj;
			else
				inv_last->next_content = t->obj;
			inv_last             = t->obj;
			t->obj->next_content = NULL;
		}
	}

	*inventory = inv_first;

	// free temp structs
	struct shopkeeper_item_temp *t = items;
	while (t)
	{
		struct shopkeeper_item_temp *next = t->next;
		free(t);
		t = next;
	}
}

static bool sql_load_shopkeeper_affects(P_char ch, int shopkeeper_id)
{
	if (!ch || !DB || shopkeeper_id <= 0)
		return false;

	char query[128];
	snprintf(query,
	         sizeof(query),
	         "SELECT type, duration, modifier, location, bitvector1, bitvector2, bitvector3, bitvector4, bitvector5 "
	         "FROM shopkeeper_affects WHERE shopkeeper_id=%d",
	         shopkeeper_id);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		struct affected_type af;
		memset(&af, 0, sizeof(af));
		af.type       = atoi(row[0]);
		af.duration   = atoi(row[1]);
		af.modifier   = atoi(row[2]);
		af.location   = atoi(row[3]);
		af.bitvector  = strtoul(row[4], NULL, 10);
		af.bitvector2 = strtoul(row[5], NULL, 10);
		af.bitvector3 = strtoul(row[6], NULL, 10);
		af.bitvector4 = strtoul(row[7], NULL, 10);
		af.bitvector5 = strtoul(row[8], NULL, 10);
		affect_to_char(ch, &af);
	}

	mysql_free_result(result);
	return true;
}

P_char sql_restore_shopkeeper(int shop_nr)
{
	if (!DB || shop_nr < 0)
		return NULL;

	char query[256];
	snprintf(query, sizeof(query), "SELECT id, mob_vnum, room_vnum FROM shopkeepers WHERE shop_id=%d", shop_nr);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		return NULL;
	}

	int shopkeeper_id = atoi(row[0]);
	int mob_vnum      = atoi(row[1]);
	int room_vnum     = atoi(row[2]);
	mysql_free_result(result);

	P_char ch = read_mobile(mob_vnum, VIRTUAL);
	if (!ch)
	{
		logit(LOG_DEBUG, "sql_restore_shopkeeper: mob vnum %d not found", mob_vnum);
		return NULL;
	}

	GET_BIRTHPLACE(ch) = room_vnum;
	sql_load_shopkeeper_affects(ch, shopkeeper_id);

	// batched load of all equipment and inventory
	P_obj equipment[MAX_WEAR];
	memset(equipment, 0, sizeof(equipment));
	P_obj inventory = NULL;

	sql_load_all_shopkeeper_items(shopkeeper_id, equipment, &inventory);

	for (int slot = 0; slot < MAX_WEAR; slot++)
	{
		if (equipment[slot])
			equip_char(ch, equipment[slot], slot, 0);
	}

	ch->carrying = inventory;
	for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
	{
		obj->loc_p        = LOC_CARRIED;
		obj->loc.carrying = ch;
	}

	return ch;
}

// temp struct for batched shopkeeper loading
struct shopkeeper_temp
{
	int                     shop_nr;
	int                     shopkeeper_id;
	int                     mob_vnum;
	int                     room_vnum;
	P_char                  mob;
	P_obj                   equipment[MAX_WEAR];
	P_obj                   inventory;
	struct shopkeeper_temp *next;
};

// temp struct for batched item loading across all shopkeepers
struct all_items_temp
{
	int                    item_id;
	int                    shopkeeper_id;
	int                    container_id;
	int                    equip_slot;
	P_obj                  obj;
	struct all_items_temp *next;
};

void sql_restore_shopkeepers(void)
{
	if (!DB)
		return;

	// query 1: load all shopkeepers
	MYSQL_RES *result = db_query("SELECT shop_id, id, mob_vnum, room_vnum FROM shopkeepers");
	if (!result)
		return;

	struct shopkeeper_temp *keepers      = NULL;
	int                     keeper_count = 0;
	MYSQL_ROW               row;

	while ((row = mysql_fetch_row(result)))
	{
		int shop_nr       = atoi(row[0]);
		int shopkeeper_id = atoi(row[1]);
		int mob_vnum      = atoi(row[2]);
		int room_vnum     = atoi(row[3]);

		P_char mob = read_mobile(mob_vnum, VIRTUAL);
		if (!mob)
		{
			logit(LOG_DEBUG, "sql_restore_shopkeeper: mob vnum %d not found", mob_vnum);
			continue;
		}

		struct shopkeeper_temp *k = (struct shopkeeper_temp *)malloc(sizeof(struct shopkeeper_temp));
		k->shop_nr                = shop_nr;
		k->shopkeeper_id          = shopkeeper_id;
		k->mob_vnum               = mob_vnum;
		k->room_vnum              = room_vnum;
		k->mob                    = mob;
		memset(k->equipment, 0, sizeof(k->equipment));
		k->inventory = NULL;

		GET_BIRTHPLACE(mob) = room_vnum;

		k->next = keepers;
		keepers = k;
		keeper_count++;
	}
	mysql_free_result(result);

	if (keeper_count == 0)
		return;

	// query 2: load all shopkeeper affects
	result = db_query("SELECT sa.shopkeeper_id, sa.type, sa.duration, sa.modifier, sa.location, "
	                  "sa.bitvector1, sa.bitvector2, sa.bitvector3, sa.bitvector4, sa.bitvector5 "
	                  "FROM shopkeeper_affects sa "
	                  "INNER JOIN shopkeepers s ON sa.shopkeeper_id = s.id");
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int shopkeeper_id = atoi(row[0]);
			for (struct shopkeeper_temp *k = keepers; k; k = k->next)
			{
				if (k->shopkeeper_id == shopkeeper_id)
				{
					struct affected_type af;
					memset(&af, 0, sizeof(af));
					af.type       = atoi(row[1]);
					af.duration   = atoi(row[2]);
					af.modifier   = atoi(row[3]);
					af.location   = atoi(row[4]);
					af.bitvector  = strtoul(row[5], NULL, 10);
					af.bitvector2 = strtoul(row[6], NULL, 10);
					af.bitvector3 = strtoul(row[7], NULL, 10);
					af.bitvector4 = strtoul(row[8], NULL, 10);
					af.bitvector5 = strtoul(row[9], NULL, 10);
					affect_to_char(k->mob, &af);
					break;
				}
			}
		}
		mysql_free_result(result);
	}

	// query 3: load all items for all shopkeepers
	struct all_items_temp *all_items = NULL;
	struct all_items_temp *last_item = NULL;

	result = db_query("SELECT si.id, si.shopkeeper_id, si.vnum, si.equip_slot, si.weight, si.cost, si.timer, "
	                  "si.extra_flags, si.value0, si.value1, si.value2, si.value3, si.value4, si.value5, "
	                  "si.value6, si.value7, si.name, si.short_descr, si.description, si.action_descr, si.container_id "
	                  "FROM shopkeeper_items si "
	                  "INNER JOIN shopkeepers s ON si.shopkeeper_id = s.id "
	                  "ORDER BY si.shopkeeper_id, si.id");
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int item_id       = atoi(row[0]);
			int shopkeeper_id = atoi(row[1]);
			int vnum          = atoi(row[2]);
			int rnum          = real_object(vnum);
			if (rnum < 0)
				continue;

			P_obj obj = read_object(rnum, REAL);
			if (!obj)
				continue;

			int equip_slot   = atoi(row[3]);
			int container_id = row[20] ? atoi(row[20]) : 0;

			if (row[4])
				obj->weight = atoi(row[4]);
			if (row[5])
				obj->cost = atoi(row[5]);
			if (row[6])
				obj->timer[0] = atol(row[6]);
			if (row[7])
				obj->extra_flags = strtoul(row[7], NULL, 10);

			obj->value[0] = row[8] ? atoi(row[8]) : 0;
			obj->value[1] = row[9] ? atoi(row[9]) : 0;
			obj->value[2] = row[10] ? atoi(row[10]) : 0;
			obj->value[3] = row[11] ? atoi(row[11]) : 0;
			obj->value[4] = row[12] ? atoi(row[12]) : 0;
			obj->value[5] = row[13] ? atoi(row[13]) : 0;
			obj->value[6] = row[14] ? atoi(row[14]) : 0;
			obj->value[7] = row[15] ? atoi(row[15]) : 0;

			if (row[16] && strlen(row[16]) > 0)
			{
				obj->name = str_dup(row[16]);
				obj->str_mask |= STRUNG_KEYS;
			}
			if (row[17] && strlen(row[17]) > 0)
			{
				obj->short_description = str_dup(row[17]);
				obj->str_mask |= STRUNG_DESC2;
			}
			if (row[18] && strlen(row[18]) > 0)
			{
				obj->description = str_dup(row[18]);
				obj->str_mask |= STRUNG_DESC1;
			}
			if (row[19] && strlen(row[19]) > 0)
			{
				obj->action_description = str_dup(row[19]);
				obj->str_mask |= STRUNG_DESC3;
			}

			struct all_items_temp *t = (struct all_items_temp *)malloc(sizeof(struct all_items_temp));
			t->item_id               = item_id;
			t->shopkeeper_id         = shopkeeper_id;
			t->container_id          = container_id;
			t->equip_slot            = equip_slot;
			t->obj                   = obj;
			t->next                  = NULL;

			if (!all_items)
				all_items = t;
			else
				last_item->next = t;
			last_item = t;
		}
		mysql_free_result(result);
	}

	// query 4: load all item affects
	result = db_query("SELECT sia.item_id, sia.location, sia.modifier "
	                  "FROM shopkeeper_item_affects sia "
	                  "INNER JOIN shopkeeper_items si ON sia.item_id = si.id "
	                  "INNER JOIN shopkeepers s ON si.shopkeeper_id = s.id "
	                  "ORDER BY sia.item_id");
	if (result)
	{
		while ((row = mysql_fetch_row(result)))
		{
			int aff_item_id = atoi(row[0]);
			int location    = atoi(row[1]);
			int modifier    = atoi(row[2]);

			for (struct all_items_temp *t = all_items; t; t = t->next)
			{
				if (t->item_id == aff_item_id)
				{
					for (int i = 0; i < MAX_OBJ_AFFECT; i++)
					{
						if (t->obj->affected[i].location == 0 && t->obj->affected[i].modifier == 0)
						{
							t->obj->affected[i].location = location;
							t->obj->affected[i].modifier = modifier;
							break;
						}
					}
					break;
				}
			}
		}
		mysql_free_result(result);
	}

	// link container contents
	for (struct all_items_temp *t = all_items; t; t = t->next)
	{
		if (t->container_id > 0)
		{
			for (struct all_items_temp *p = all_items; p; p = p->next)
			{
				if (p->item_id == t->container_id)
				{
					if (!obj_can_nest(t->obj, p->obj))
					{
						logit(LOG_DEBUG, "sql_restore_shopkeepers: skipping malformed container link %d -> %d", t->item_id, p->item_id);
						break;
					}
					t->obj->next_content = p->obj->contains;
					p->obj->contains     = t->obj;
					t->obj->loc_p        = LOC_INSIDE;
					t->obj->loc.inside   = p->obj;
					break;
				}
			}
		}
	}

	// assign items to shopkeepers
	for (struct all_items_temp *t = all_items; t; t = t->next)
	{
		if (t->container_id > 0)
			continue;

		for (struct shopkeeper_temp *k = keepers; k; k = k->next)
		{
			if (k->shopkeeper_id == t->shopkeeper_id)
			{
				if (t->equip_slot > 0 && t->equip_slot <= MAX_WEAR)
					k->equipment[t->equip_slot - 1] = t->obj;
				else
				{
					t->obj->next_content = k->inventory;
					k->inventory         = t->obj;
				}
				break;
			}
		}
	}

	// free item temp structs
	struct all_items_temp *ti = all_items;
	while (ti)
	{
		struct all_items_temp *next = ti->next;
		free(ti);
		ti = next;
	}

	// process each shopkeeper
	int loaded = 0;
	for (struct shopkeeper_temp *k = keepers; k; k = k->next)
	{
		int load_room = real_room(k->room_vnum);
		if (load_room == NOWHERE)
		{
			logit(LOG_DEBUG, "sql_restore_shopkeepers: bad room %d for shop %d", k->room_vnum, k->shop_nr);
			extract_char(k->mob);
			continue;
		}

		// equip and set inventory
		for (int slot = 0; slot < MAX_WEAR; slot++)
		{
			if (k->equipment[slot])
				equip_char(k->mob, k->equipment[slot], slot, 0);
		}
		k->mob->carrying = k->inventory;
		for (P_obj obj = k->mob->carrying; obj; obj = obj->next_content)
		{
			obj->loc_p        = LOC_CARRIED;
			obj->loc.carrying = k->mob;
		}

		// find shop index
		int shop_idx;
		for (shop_idx = 0; shop_idx < number_of_shops; shop_idx++)
		{
			if (shop_index[shop_idx].keeper == GET_RNUM(k->mob))
				break;
		}

		// remove existing keepers with same vnum
		int extracted = 0;
		for (P_char keeper2 = character_list; keeper2;)
		{
			P_char next = keeper2->next;
			if (IS_NPC(keeper2) && keeper2 != k->mob && mob_index[GET_RNUM(keeper2)].virtual_number == k->mob_vnum)
			{
				extract_char(keeper2);
				extracted++;
			}
			keeper2 = next;
		}
		logit(LOG_DEBUG, "sql_restore_shopkeepers: shop %d vnum %d extracted %d existing", k->shop_nr, k->mob_vnum, extracted);

		char_to_room(k->mob, load_room, 0);

		// add produced items not in db
		if (shop_idx < number_of_shops)
		{
			for (int i = 0; i < shop_index[shop_idx].number_items_produced; i++)
			{
				int rnum = shop_index[shop_idx].producing[i];
				if (rnum >= 0)
				{
					int found = 0;
					for (P_obj o = k->mob->carrying; o; o = o->next_content)
					{
						if (o->R_num == rnum)
						{
							found = 1;
							break;
						}
					}
					if (!found)
					{
						P_obj obj = read_object(rnum, REAL);
						if (obj)
							obj_to_char(obj, k->mob);
					}
				}
			}
			shop_index[shop_idx].dirty = 1;
		}
		loaded++;
	}

	// query 5: delete all shopkeepers in one go
	if (!sql_run_query("DELETE FROM shopkeepers"))
		logit(LOG_DEBUG, "sql_restore_shopkeepers: failed to delete old shopkeepers");

	// free keeper temp structs
	struct shopkeeper_temp *tk = keepers;
	while (tk)
	{
		struct shopkeeper_temp *next = tk->next;
		free(tk);
		tk = next;
	}

	logit(LOG_DEBUG, "sql_restore_shopkeepers: loaded %d shopkeepers", loaded);
}

void sql_save_dirty_shopkeepers(void)
{
	if (!DB)
		return;

	int saved = 0;
	for (int i = 0; i < number_of_shops; i++)
	{
		if (!shop_index[i].dirty)
			continue;

		int keeper_rnum = shop_index[i].keeper;
		if (keeper_rnum < 0)
		{
			shop_index[i].dirty = 0;
			continue;
		}

		// find the shopkeeper mob in the shop's defined room
		int    shop_room = real_room(shop_index[i].in_room);
		P_char keeper    = NULL;

		if (shop_room >= 0 && shop_room <= top_of_world)
		{
			for (P_char ch = world[shop_room].people; ch; ch = ch->next_in_room)
			{
				if (IS_NPC(ch) && GET_RNUM(ch) == keeper_rnum)
				{
					keeper = ch;
					break;
				}
			}
		}

		if (keeper)
		{
			// sql_save_shopkeeper already does DELETE before INSERT
			if (sql_save_shopkeeper(keeper, i))
			{
				shop_index[i].dirty = 0;
				saved++;
			}
			else
			{
				logit(LOG_DEBUG, "sql_save_dirty_shopkeepers: failed to save shopkeeper for shop %d", i);
			}
		}
		else
		{
			// keeper not found; keep dirty so a later flush can retry when the
			// NPC is present again.
			logit(LOG_DEBUG, "sql_save_dirty_shopkeepers: keeper not found for shop %d; leaving dirty", i);
		}
	}

	if (saved > 0)
		logit(LOG_DEBUG, "sql_save_dirty_shopkeepers: saved %d shopkeepers", saved);
}

static P_obj sql_load_saved_item_contents(const char *item_key, int container_id, int depth)
{
	if (!DB || !item_key)
		return NULL;

	if (depth > MAX_CONTAINER_LOAD_DEPTH)
	{
		logit(LOG_DEBUG, "sql_load_saved_item_contents: container depth exceeded for %s container %d", item_key, container_id);
		return NULL;
	}

	char *esc_key = sql_escape_string(item_key);
	if (!esc_key)
		return NULL;

	char query[512];
	snprintf(query,
	         sizeof(query),
	         "SELECT id, vnum, weight, cost, timer, extra_flags, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, "
	         "wear_flags, item_type, item_material, "
	         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5 "
	         "FROM saved_items WHERE item_key='%s' AND container_id=%d",
	         esc_key,
	         container_id);
	free(esc_key);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	P_obj     first_obj = NULL;
	P_obj     last_obj  = NULL;
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		int item_id = atoi(row[0]);
		int vnum    = atoi(row[1]);
		int rnum    = real_object(vnum);
		if (rnum < 0)
			continue;

		P_obj obj = read_object(rnum, REAL);
		if (!obj)
			continue;

		if (row[2])
			obj->weight = atoi(row[2]);
		if (row[3])
			obj->cost = atoi(row[3]);
		if (row[4])
			obj->timer[0] = atol(row[4]);
		if (row[5])
			obj->extra_flags = strtoul(row[5], NULL, 10);

		obj->value[0] = row[6] ? atoi(row[6]) : obj->value[0];
		obj->value[1] = row[7] ? atoi(row[7]) : obj->value[1];
		obj->value[2] = row[8] ? atoi(row[8]) : obj->value[2];
		obj->value[3] = row[9] ? atoi(row[9]) : obj->value[3];
		obj->value[4] = row[10] ? atoi(row[10]) : obj->value[4];
		obj->value[5] = row[11] ? atoi(row[11]) : obj->value[5];
		obj->value[6] = row[12] ? atoi(row[12]) : obj->value[6];
		obj->value[7] = row[13] ? atoi(row[13]) : obj->value[7];

		if (row[14] && strlen(row[14]) > 0)
		{
			obj->name = str_dup(row[14]);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (row[15] && strlen(row[15]) > 0)
		{
			obj->short_description = str_dup(row[15]);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (row[16] && strlen(row[16]) > 0)
		{
			obj->description = str_dup(row[16]);
			obj->str_mask |= STRUNG_DESC1;
		}
		if (row[17] && strlen(row[17]) > 0)
		{
			obj->action_description = str_dup(row[17]);
			obj->str_mask |= STRUNG_DESC3;
		}
		// v19 diff columns - NULL means use prototype value from read_object()
		if (row[18])
			obj->wear_flags = atoi(row[18]);
		if (row[19])
			obj->type = sql_validate_loaded_item_type(obj, atoi(row[19]), "sql_load_saved_item_contents");
		if (row[20])
			obj->material = atoi(row[20]);
		if (row[21])
			obj->bitvector = strtoul(row[21], NULL, 10);
		if (row[22])
			obj->bitvector2 = strtoul(row[22], NULL, 10);
		if (row[23])
			obj->bitvector3 = strtoul(row[23], NULL, 10);
		if (row[24])
			obj->bitvector4 = strtoul(row[24], NULL, 10);
		if (row[25])
			obj->bitvector5 = strtoul(row[25], NULL, 10);



		char aff_query[128];
		snprintf(aff_query, sizeof(aff_query), "SELECT location, modifier FROM saved_item_affects WHERE item_id=%d", item_id);
		MYSQL_RES *aff_result = db_query("%s", aff_query);
		if (aff_result)
		{
			MYSQL_ROW aff_row;
			int       aff_idx = 0;
			while ((aff_row = mysql_fetch_row(aff_result)) && aff_idx < MAX_OBJ_AFFECT)
			{
				obj->affected[aff_idx].location = atoi(aff_row[0]);
				obj->affected[aff_idx].modifier = atoi(aff_row[1]);
				aff_idx++;
			}
			mysql_free_result(aff_result);
		}

		obj->contains = sql_load_saved_item_contents(item_key, item_id, 0);
		for (P_obj c = obj->contains; c; c = c->next_content)
		{
			if (!obj_can_nest(c, obj))
			{
				logit(LOG_DEBUG, "sql_load_saved_item_contents: skipping malformed container link %d -> %d", c->db_item_id, obj->db_item_id);
				continue;
			}
			c->loc_p      = LOC_INSIDE;
			c->loc.inside = obj;
		}

		if (!first_obj)
			first_obj = obj;
		else
			last_obj->next_content = obj;
		last_obj          = obj;
		obj->next_content = NULL;
	}

	mysql_free_result(result);
	return first_obj;
}

void sql_restore_saved_items(void)
{
	if (!DB)
		return;

	struct restored_saved_item
	{
		char *item_key;
		P_obj item;
		struct restored_saved_item *next;
	};

	struct restored_saved_item *restored_head = NULL;
	struct restored_saved_item *restored_tail = NULL;

	// get distinct item keys with root items only
	MYSQL_RES *result = db_query("SELECT DISTINCT item_key, room_vnum, id, vnum, weight, cost, timer, extra_flags, "
	                             "value0, value1, value2, value3, value4, value5, value6, value7, "
	                             "name, short_descr, description, action_descr "
	                             "FROM saved_items WHERE container_id IS NULL");
	if (!result)
		return;

	int       loaded = 0;
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		const char *item_key  = row[0];
		int         room_vnum = atoi(row[1]);
		int         item_id   = atoi(row[2]);
		int         vnum      = atoi(row[3]);

		int room = real_room(room_vnum);
		if (room == NOWHERE)
		{
			logit(LOG_DEBUG, "sql_restore_saved_items: bad room %d for %s", room_vnum, item_key);
			continue;
		}

		int rnum = real_object(vnum);
		if (rnum < 0)
			continue;

		P_obj obj = read_object(rnum, REAL);
		if (!obj)
			continue;

		if (row[4])
			obj->weight = atoi(row[4]);
		if (row[5])
			obj->cost = atoi(row[5]);
		if (row[6])
			obj->timer[0] = atol(row[6]);
		if (row[7])
			obj->extra_flags = strtoul(row[7], NULL, 10);

		obj->value[0] = row[8] ? atoi(row[8]) : 0;
		obj->value[1] = row[9] ? atoi(row[9]) : 0;
		obj->value[2] = row[10] ? atoi(row[10]) : 0;
		obj->value[3] = row[11] ? atoi(row[11]) : 0;
		obj->value[4] = row[12] ? atoi(row[12]) : 0;
		obj->value[5] = row[13] ? atoi(row[13]) : 0;
		obj->value[6] = row[14] ? atoi(row[14]) : 0;
		obj->value[7] = row[15] ? atoi(row[15]) : 0;

		if (row[16] && strlen(row[16]) > 0)
		{
			obj->name = str_dup(row[16]);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (row[17] && strlen(row[17]) > 0)
		{
			obj->short_description = str_dup(row[17]);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (row[18] && strlen(row[18]) > 0)
		{
			obj->description = str_dup(row[18]);
			obj->str_mask |= STRUNG_DESC1;
		}
		if (row[19] && strlen(row[19]) > 0)
		{
			obj->action_description = str_dup(row[19]);
			obj->str_mask |= STRUNG_DESC3;
		}

		char aff_query[128];
		snprintf(aff_query, sizeof(aff_query), "SELECT location, modifier FROM saved_item_affects WHERE item_id=%d", item_id);
		MYSQL_RES *aff_result = db_query("%s", aff_query);
		if (aff_result)
		{
			MYSQL_ROW aff_row;
			int       aff_idx = 0;
			while ((aff_row = mysql_fetch_row(aff_result)) && aff_idx < MAX_OBJ_AFFECT)
			{
				obj->affected[aff_idx].location = atoi(aff_row[0]);
				obj->affected[aff_idx].modifier = atoi(aff_row[1]);
				aff_idx++;
			}
			mysql_free_result(aff_result);
		}

		obj->contains = sql_load_saved_item_contents(item_key, item_id, 0);
		for (P_obj c = obj->contains; c; c = c->next_content)
		{
			if (!obj_can_nest(c, obj))
			{
				logit(LOG_DEBUG, "sql_load_saved_item_contents: skipping malformed container link %d -> %d", c->db_item_id, obj->db_item_id);
				continue;
			}
			c->loc_p      = LOC_INSIDE;
			c->loc.inside = obj;
		}

		obj_to_room(obj, room);

		struct restored_saved_item *entry;
		CREATE(entry, struct restored_saved_item, 1, MEM_TAG_OTHER);
		if (entry)
		{
			entry->item_key = str_dup(item_key ? item_key : "");
			entry->item     = obj;
			entry->next     = NULL;
			if (!restored_head)
				restored_head = entry;
			else
				restored_tail->next = entry;
			restored_tail = entry;
		}
		loaded++;
	}

	mysql_free_result(result);

	// delete all saved items after loading (they get re-saved on next tick)
	if (!sql_run_query("DELETE FROM saved_items"))
	{
		logit(LOG_DEBUG, "sql_restore_saved_items: failed to delete old saved items; attempting to rewrite loaded items");
		for (struct restored_saved_item *entry = restored_head; entry; entry = entry->next)
		{
			if (!sql_save_saved_item(entry->item, entry->item_key))
				logit(LOG_DEBUG, "sql_restore_saved_items: failed to rewrite %s after delete failure", entry->item_key ? entry->item_key : "<null>");
		}
	}

	for (struct restored_saved_item *entry = restored_head; entry; )
	{
		struct restored_saved_item *next = entry->next;
		if (entry->item_key)
			free(entry->item_key);
		free(entry);
		entry = next;
	}

	logit(LOG_DEBUG, "sql_restore_saved_items: loaded %d items", loaded);
}

static P_obj sql_load_siege_item_contents(int room_vnum, int container_id, int depth)
{
	if (!DB)
		return NULL;

	if (depth > MAX_CONTAINER_LOAD_DEPTH)
	{
		logit(LOG_DEBUG, "sql_load_siege_item_contents: container depth exceeded for room %d container %d", room_vnum, container_id);
		return NULL;
	}

	char query[512];
	snprintf(query,
	         sizeof(query),
	         "SELECT id, vnum, weight, cost, timer, extra_flags, "
	         "value0, value1, value2, value3, value4, value5, value6, value7, "
	         "name, short_descr, description, action_descr, "
	         "wear_flags, item_type, item_material, "
	         "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5 "
	         "FROM siege_items WHERE room_vnum=%d AND container_id=%d",
	         room_vnum,
	         container_id);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	P_obj     first_obj = NULL;
	P_obj     last_obj  = NULL;
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		int item_id = atoi(row[0]);
		int vnum    = atoi(row[1]);
		int rnum    = real_object(vnum);
		if (rnum < 0)
			continue;

		P_obj obj = read_object(rnum, REAL);
		if (!obj)
			continue;

		if (row[2])
			obj->weight = atoi(row[2]);
		if (row[3])
			obj->cost = atoi(row[3]);
		if (row[4])
			obj->timer[0] = atol(row[4]);
		if (row[5])
			obj->extra_flags = strtoul(row[5], NULL, 10);

		obj->value[0] = row[6] ? atoi(row[6]) : obj->value[0];
		obj->value[1] = row[7] ? atoi(row[7]) : obj->value[1];
		obj->value[2] = row[8] ? atoi(row[8]) : obj->value[2];
		obj->value[3] = row[9] ? atoi(row[9]) : obj->value[3];
		obj->value[4] = row[10] ? atoi(row[10]) : obj->value[4];
		obj->value[5] = row[11] ? atoi(row[11]) : obj->value[5];
		obj->value[6] = row[12] ? atoi(row[12]) : obj->value[6];
		obj->value[7] = row[13] ? atoi(row[13]) : obj->value[7];

		if (row[14] && strlen(row[14]) > 0)
		{
			obj->name = str_dup(row[14]);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (row[15] && strlen(row[15]) > 0)
		{
			obj->short_description = str_dup(row[15]);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (row[16] && strlen(row[16]) > 0)
		{
			obj->description = str_dup(row[16]);
			obj->str_mask |= STRUNG_DESC1;
		}
		if (row[17] && strlen(row[17]) > 0)
		{
			obj->action_description = str_dup(row[17]);
			obj->str_mask |= STRUNG_DESC3;
		}
		// v19 diff columns - NULL means use prototype value from read_object()
		if (row[18])
			obj->wear_flags = atoi(row[18]);
		if (row[19])
			obj->type = sql_validate_loaded_item_type(obj, atoi(row[19]), "sql_load_siege_items");
		if (row[20])
			obj->material = atoi(row[20]);
		if (row[21])
			obj->bitvector = strtoul(row[21], NULL, 10);
		if (row[22])
			obj->bitvector2 = strtoul(row[22], NULL, 10);
		if (row[23])
			obj->bitvector3 = strtoul(row[23], NULL, 10);
		if (row[24])
			obj->bitvector4 = strtoul(row[24], NULL, 10);
		if (row[25])
			obj->bitvector5 = strtoul(row[25], NULL, 10);



		char aff_query[128];
		snprintf(aff_query, sizeof(aff_query), "SELECT location, modifier FROM siege_item_affects WHERE item_id=%d", item_id);
		MYSQL_RES *aff_result = db_query("%s", aff_query);
		if (aff_result)
		{
			MYSQL_ROW aff_row;
			int       aff_idx = 0;
			while ((aff_row = mysql_fetch_row(aff_result)) && aff_idx < MAX_OBJ_AFFECT)
			{
				obj->affected[aff_idx].location = atoi(aff_row[0]);
				obj->affected[aff_idx].modifier = atoi(aff_row[1]);
				aff_idx++;
			}
			mysql_free_result(aff_result);
		}

		obj->contains = sql_load_siege_item_contents(room_vnum, item_id, 0);
		for (P_obj c = obj->contains; c; c = c->next_content)
		{
			if (!obj_can_nest(c, obj))
			{
				logit(LOG_DEBUG, "sql_load_siege_item_contents: skipping malformed container link %d -> %d", c->db_item_id, obj->db_item_id);
				continue;
			}
			c->loc_p      = LOC_INSIDE;
			c->loc.inside = obj;
		}

		if (!first_obj)
			first_obj = obj;
		else
			last_obj->next_content = obj;
		last_obj          = obj;
		obj->next_content = NULL;
	}

	mysql_free_result(result);
	return first_obj;
}

extern P_siege siege_objects;

void sql_load_siege_list(void)
{
	if (!DB)
		return;

	siege_objects = NULL;

	MYSQL_RES *result = db_query("SELECT DISTINCT room_vnum, id, vnum, weight, cost, timer, extra_flags, "
	                             "value0, value1, value2, value3, value4, value5, value6, value7, "
	                             "name, short_descr, description, action_descr "
	                             "FROM siege_items WHERE container_id IS NULL");
	if (!result)
		return;

	int       loaded = 0;
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		int room_vnum = atoi(row[0]);
		int item_id   = atoi(row[1]);
		int vnum      = atoi(row[2]);

		int room = real_room(room_vnum);
		if (room == NOWHERE)
		{
			logit(LOG_DEBUG, "sql_load_siege_list: bad room %d", room_vnum);
			continue;
		}

		int rnum = real_object(vnum);
		if (rnum < 0)
			continue;

		P_obj obj = read_object(rnum, REAL);
		if (!obj)
			continue;

		if (row[3])
			obj->weight = atoi(row[3]);
		if (row[4])
			obj->cost = atoi(row[4]);
		if (row[5])
			obj->timer[0] = atol(row[5]);
		if (row[6])
			obj->extra_flags = strtoul(row[6], NULL, 10);

		obj->value[0] = row[7] ? atoi(row[7]) : 0;
		obj->value[1] = row[8] ? atoi(row[8]) : 0;
		obj->value[2] = row[9] ? atoi(row[9]) : 0;
		obj->value[3] = row[10] ? atoi(row[10]) : 0;
		obj->value[4] = row[11] ? atoi(row[11]) : 0;
		obj->value[5] = row[12] ? atoi(row[12]) : 0;
		obj->value[6] = row[13] ? atoi(row[13]) : 0;
		obj->value[7] = row[14] ? atoi(row[14]) : 0;

		if (row[15] && strlen(row[15]) > 0)
		{
			obj->name = str_dup(row[15]);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (row[16] && strlen(row[16]) > 0)
		{
			obj->short_description = str_dup(row[16]);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (row[17] && strlen(row[17]) > 0)
		{
			obj->description = str_dup(row[17]);
			obj->str_mask |= STRUNG_DESC1;
		}
		if (row[18] && strlen(row[18]) > 0)
		{
			obj->action_description = str_dup(row[18]);
			obj->str_mask |= STRUNG_DESC3;
		}

		char aff_query[128];
		snprintf(aff_query, sizeof(aff_query), "SELECT location, modifier FROM siege_item_affects WHERE item_id=%d", item_id);
		MYSQL_RES *aff_result = db_query("%s", aff_query);
		if (aff_result)
		{
			MYSQL_ROW aff_row;
			int       aff_idx = 0;
			while ((aff_row = mysql_fetch_row(aff_result)) && aff_idx < MAX_OBJ_AFFECT)
			{
				obj->affected[aff_idx].location = atoi(aff_row[0]);
				obj->affected[aff_idx].modifier = atoi(aff_row[1]);
				aff_idx++;
			}
			mysql_free_result(aff_result);
		}

		obj->contains = sql_load_siege_item_contents(room_vnum, item_id, 0);
		for (P_obj c = obj->contains; c; c = c->next_content)
		{
			if (!obj_can_nest(c, obj))
			{
				logit(LOG_DEBUG, "sql_load_siege_item_contents: skipping malformed container link %d -> %d", c->db_item_id, obj->db_item_id);
				continue;
			}
			c->loc.inside = obj;
			c->loc_p      = LOC_INSIDE;
		}

		obj_to_room(obj, room);

		P_siege siege     = new struct siege;
		siege->obj        = obj;
		siege->next_siege = siege_objects;
		siege_objects     = siege;

		loaded++;
	}

	mysql_free_result(result);
	logit(LOG_DEBUG, "sql_load_siege_list: loaded %d siege objects", loaded);
}

#define SHIP_SQL_BATCH_SIZE  (10 * 1024)

static bool sql_save_ship_armor(P_ship ship, char *queryBuffer, int batchSize, int &bufferPosition)
{
	if (!DB || !ship || ship->db_id == -1)
	{
		logit(LOG_DEBUG, "sql_save_ship_armor: invalid parameters");
		return false;
	}

	for (int i = 0; i < 4; i++)
	{
		if (bufferPosition >= batchSize)
		{
			logit(LOG_DEBUG, "sql_save_ship_armor: buffer overflow");
			return false;
		}

		bufferPosition += snprintf(queryBuffer + bufferPosition,
		                           batchSize - bufferPosition,
		                           "insert into ship_armor (ship_id, side, armor, internal) "
		                           "values (%d, %d, %d, %d) "
		                           "on duplicate key update armor=%d, internal=%d;",
		                           ship->db_id,
		                           i,
		                           ship->armor[i],
		                           ship->internal[i],
		                           ship->armor[i],
		                           ship->internal[i]);
	}
	return true;
}

static bool sql_save_ship_crew(P_ship ship, char *queryBuffer, int batchSize, int &bufferPosition)
{
	if (!DB || !ship || ship->db_id == -1)
	{
		logit(LOG_DEBUG, "sql_save_ship_crew: invalid parameters");
		return false;
	}

	if (bufferPosition >= batchSize)
	{
		logit(LOG_DEBUG, "sql_save_ship_crew: buffer overflow");
		return false;
	}

	bufferPosition += snprintf(queryBuffer + bufferPosition,
	                           batchSize - bufferPosition,
	                           "insert into ship_crew (ship_id, crew_index, sail_skill, guns_skill, rpar_skill, "
	                           "sail_chief, guns_chief, rpar_chief) "
	                           "values (%d, %d, %d, %d, %d, %d, %d, %d) "
	                           "on duplicate key update crew_index=%d, sail_skill=%d, guns_skill=%d, rpar_skill=%d, "
	                           "sail_chief=%d, guns_chief=%d, rpar_chief=%d;",
	                           ship->db_id,
	                           ship->crew.index,
	                           (int)(ship->crew.sail_skill * 1000),
	                           (int)(ship->crew.guns_skill * 1000),
	                           (int)(ship->crew.rpar_skill * 1000),
	                           ship->crew.sail_chief,
	                           ship->crew.guns_chief,
	                           ship->crew.rpar_chief,
	                           ship->crew.index,
	                           (int)(ship->crew.sail_skill * 1000),
	                           (int)(ship->crew.guns_skill * 1000),
	                           (int)(ship->crew.rpar_skill * 1000),
	                           ship->crew.sail_chief,
	                           ship->crew.guns_chief,
	                           ship->crew.rpar_chief);

	return true;
}

static bool sql_save_ship_slots(P_ship ship, char *queryBuffer, int batchSize, int &bufferPosition)
{
	if (!DB || !ship || ship->db_id == -1)
	{
		logit(LOG_DEBUG, "sql_save_ship_slots: invalid parameters");
		return false;
	}

	for (int i = 0; i < MAXSLOTS; i++)
	{
		if (bufferPosition >= batchSize)
		{
			logit(LOG_DEBUG, "sql_save_ship_slots: buffer overflow");
			return false;
		}

		bufferPosition += snprintf(queryBuffer + bufferPosition,
		                           batchSize - bufferPosition,
		                           "insert into ship_slots (ship_id, slot_index, slot_type, item_index, position, "
		                           "timer, val0, val1, val2, val3, val4) "
		                           "values (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d) "
		                           "on duplicate key update slot_type=%d, item_index=%d, position=%d, "
		                           "timer=%d, val0=%d, val1=%d, val2=%d, val3=%d, val4=%d;",
		                           ship->db_id,
		                           i,
		                           ship->slot[i].type,
		                           ship->slot[i].index,
		                           ship->slot[i].position,
		                           ship->slot[i].timer,
		                           ship->slot[i].val0,
		                           ship->slot[i].val1,
		                           ship->slot[i].val2,
		                           ship->slot[i].val3,
		                           ship->slot[i].val4,
		                           ship->slot[i].type,
		                           ship->slot[i].index,
		                           ship->slot[i].position,
		                           ship->slot[i].timer,
		                           ship->slot[i].val0,
		                           ship->slot[i].val1,
		                           ship->slot[i].val2,
		                           ship->slot[i].val3,
		                           ship->slot[i].val4);
	}
	return true;
}

bool sql_save_ship(P_ship ship)
{
	if (!DB || !ship || !ship->ownername)
		return false;

	char *esc_owner = sql_escape_string(ship->ownername);
	if (!esc_owner)
		return false;

	char *esc_name = sql_escape_string(ship->name ? ship->name : "");

	int   pos          = 0;
	char *batch        = (char *)malloc(SHIP_SQL_BATCH_SIZE);
	int   batchSize    = SHIP_SQL_BATCH_SIZE;
	if (!batch)
	{		
		free(esc_owner);
		if (esc_name)
			free(esc_name);
		return false;
	}
	memset(batch, 0, batchSize);

	/* Join an existing aggregate transaction when the caller owns one
	 * (for example shutdown_ships()).  Starting a nested transaction would
	 * fail and make an otherwise valid ship save look like a persistence
	 * error.  Only the transaction owner may commit or roll back it. */
	bool own_transaction = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
		{
			logit(LOG_DEBUG, "sql_save_ship: failed to start transaction");
			free(batch);
			free(esc_owner);
			if (esc_name)
				free(esc_name);
			return false;
		}
		own_transaction = true;
	}

	if (ship->db_id == -1)
	{
		char initQuery[1024];
		snprintf(initQuery,
		         ARRAY_SIZE(initQuery),
		         "insert into ships (owner_name, ship_name, ship_class, frags, anchor_room, time_played, mainsail, race, money, flags) "
		         "values ('%s', '%s', %d, %d, %d, %d, %d, %d, %d, %lu) ",
		         esc_owner,
		         esc_name,
		         ship->m_class,
		         ship->frags,
		         ship->anchor,
		         ship->time,
		         ship->mainsail,
		         ship->race,
		         ship->money,
		         ship->flags);
		// new ship
		if (!sql_run_query(initQuery))
		{
			sql_player_error("sql_save_ship", initQuery);
			free(batch);
			free(esc_owner);
			if (esc_name)
				free(esc_name);
			if (own_transaction)
				sql_rollback();
			return false;
		}

		// get ship id
		char query[200];
		snprintf(query, ARRAY_SIZE(query), "select id from ships where owner_name='%s'", esc_owner);
		MYSQL_RES* result = db_query("%s", query);
		free(esc_owner);
		if (esc_name)
			free(esc_name);

		if (!result)
		{
			sql_player_error("sql_save_ship_2", query);
			free(batch);
			if (own_transaction)
				sql_rollback();
			ship->db_id = -1;
			return false;
		}

		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row)
		{
			free(batch);
			mysql_free_result(result);
			if (own_transaction)
				sql_rollback();
			ship->db_id = -1;
			return false;
		}

		ship->db_id = atoi(row[0]);

		mysql_free_result(result);
	}
	else
	{
		pos += snprintf(batch + pos,
		                batchSize - pos,
		                "update ships set owner_name='%s', ship_name='%s', ship_class=%d, frags=%d, anchor_room=%d, time_played=%d, mainsail=%d, race=%d, money=%d, flags=%lu "
		                "where id=%d;",
		                esc_owner,
		                esc_name,
		                ship->m_class,
		                ship->frags,
		                ship->anchor,
		                ship->time,
		                ship->mainsail,
		                ship->race,
		                ship->money,
		                ship->flags,
		                ship->db_id);

		free(esc_owner);
		if (esc_name)
			free(esc_name);
	}

	if (!sql_save_ship_armor(ship, batch, batchSize, pos) || !sql_save_ship_crew(ship, batch, batchSize, pos) || !sql_save_ship_slots(ship, batch, batchSize, pos))
	{
		sql_player_error("sql_save_ship_3", NULL);
		free(batch);
		if (own_transaction)
			sql_rollback();
		ship->db_id = -1;
		return false;
	}
	
	MYSQL_RES *result = NULL;
	if (!sql_trace_exec("sql_save_ship_batch", batch, strlen(batch), false, true))
	{
		sql_player_error("sql_save_ship_4", batch);
		free(batch);
		sql_clear_results();
		if (own_transaction)
			sql_rollback();
		ship->db_id = -1;
		logit(LOG_DEBUG, "sql_save_ship: mysql_real_query failed for ship %d", ship->db_id);
		return false;
	}
	result = mysql_store_result(DB);
	free(batch);
	if (result)
	{
		mysql_free_result(result);
	}
	sql_clear_results(); // need to clear all of the batch results

	if (own_transaction && !sql_commit())
	{
		logit(LOG_DEBUG, "sql_save_ship: failed to commit for ship %d", ship->db_id);
		if (sql_in_transaction())
			sql_rollback();
		return false;
	}

	logit(LOG_DEBUG, "sql_save_ship: finished saving ship %d", ship->db_id);
	redis_cache_ship_snapshot(ship);

	return true;
}

static bool sql_load_ship_armor(int ship_id, P_ship ship)
{
	if (!DB || !ship || ship_id <= 0)
		return false;

	char query[128];
	snprintf(query, sizeof(query), "select side, armor, internal from ship_armor where ship_id=%d", ship_id);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		int side = atoi(row[0]);
		if (side >= 0 && side < 4)
		{
			ship->armor[side]    = atoi(row[1]);
			ship->internal[side] = atoi(row[2]);
		}
	}

	mysql_free_result(result);
	return true;
}

static bool sql_load_ship_crew(int ship_id, P_ship ship)
{
	if (!DB || !ship || ship_id <= 0)
		return false;

	char query[256];
	snprintf(query,
	         sizeof(query),
	         "select crew_index, sail_skill, guns_skill, rpar_skill, sail_chief, guns_chief, rpar_chief "
	         "from ship_crew where ship_id=%d",
	         ship_id);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (row)
	{
		ship->crew.index      = atoi(row[0]);
		ship->crew.sail_skill = (float)atoi(row[1]) / 1000.0f;
		ship->crew.guns_skill = (float)atoi(row[2]) / 1000.0f;
		ship->crew.rpar_skill = (float)atoi(row[3]) / 1000.0f;
		ship->crew.sail_chief = atoi(row[4]);
		ship->crew.guns_chief = atoi(row[5]);
		ship->crew.rpar_chief = atoi(row[6]);
	}

	mysql_free_result(result);
	return true;
}

static bool sql_load_ship_slots(int ship_id, P_ship ship)
{
	if (!DB || !ship || ship_id <= 0)
		return false;

	char query[256];
	snprintf(query,
	         sizeof(query),
	         "select slot_index, slot_type, item_index, position, timer, val0, val1, val2, val3, val4 "
	         "from ship_slots where ship_id=%d",
	         ship_id);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		int idx = atoi(row[0]);
		if (idx >= 0 && idx < MAXSLOTS)
		{
			ship->slot[idx].type     = atoi(row[1]);
			ship->slot[idx].index    = atoi(row[2]);
			ship->slot[idx].position = atoi(row[3]);
			ship->slot[idx].timer    = atoi(row[4]);
			ship->slot[idx].val0     = atoi(row[5]);
			ship->slot[idx].val1     = atoi(row[6]);
			ship->slot[idx].val2     = atoi(row[7]);
			ship->slot[idx].val3     = atoi(row[8]);
			ship->slot[idx].val4     = atoi(row[9]);
		}
	}

	mysql_free_result(result);
	return true;
}

P_ship sql_load_ship(const char *owner_name)
{
	if (!owner_name)
		return NULL;

	P_ship cached_ship = redis_load_ship_snapshot(owner_name);
	if (cached_ship)
		return cached_ship;

	if (!DB)
		return NULL;

	char *esc_owner = sql_escape_string(owner_name);
	if (!esc_owner)
		return NULL;

	char query[320];
	snprintf(query,
	         sizeof(query),
	         "select id, ship_name, ship_class, frags, anchor_room, time_played, mainsail, race, money, flags "
	         "from ships where owner_name='%s'",
	         esc_owner);
	free(esc_owner);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		return NULL;
	}

	int ship_id    = atoi(row[0]);
	int ship_class = atoi(row[2]);

	P_ship ship = new_ship(ship_class);
	if (!ship)
	{
		mysql_free_result(result);
		return NULL;
	}

	ship->db_id     = ship_id;
	ship->ownername = str_dup(owner_name);
	ship->name      = str_dup(row[1] ? row[1] : "");
	ship->frags     = atoi(row[3]);
	ship->anchor    = atoi(row[4]);
	ship->time      = atoi(row[5]);
	ship->mainsail  = atoi(row[6]);
	ship->race      = atoi(row[7]);
	ship->money     = atoi(row[8]);
	ship->flags     = row[9] ? strtoul(row[9], NULL, 10) : 0;
	mysql_free_result(result);

	if (!sql_load_ship_armor(ship_id, ship) || !sql_load_ship_crew(ship_id, ship) || !sql_load_ship_slots(ship_id, ship))
	{
		logit(LOG_DEBUG, "sql_load_ship: failed to load dependent ship rows for %s", ship->ownername ? ship->ownername : "<unknown>");
		delete_ship(ship, true);
		return NULL;
	}
	ship->save_pending         = false;
	ship->save_retry_after     = 0;
	ship->save_saved_signature = ship_save_signature(ship);
	redis_cache_ship_snapshot(ship);

	return ship;
}


bool sql_load_all_ships()
{
	if (!DB)
		return false;

	MYSQL_RES *result = db_query("select owner_name from ships");
	if (!result)
		return false;

	// collect owner names first to avoid nested queries
	char owner_names[512][64];
	int  num_ships = 0;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) && num_ships < 512)
	{
		if (!row[0])
			continue;
		strncpy(owner_names[num_ships], row[0], 63);
		owner_names[num_ships][63] = '\0';
		num_ships++;
	}
	mysql_free_result(result);

	// now load each ship
	for (int i = 0; i < num_ships; i++)
	{
		P_ship ship = sql_load_ship(owner_names[i]);
		if (!ship)
		{
			logit(LOG_FILE, "sql_load_all_ships: failed to load ship rows for %s", owner_names[i]);
			continue;
		}

		name_ship(ship->name, ship);
		if (!load_ship(ship, real_room0(ship->anchor)))
		{
			logit(LOG_FILE, "sql_load_all_ships: failed to load ship for %s", owner_names[i]);
			continue;
		}

		ship->mainsail = BOUNDED(0, ship->mainsail, SHIP_MAX_SAIL(ship));
		update_crew(ship);
		reset_crew_stamina(ship);
		set_ship_armor(ship, false);
		update_ship_status(ship);
	}

	return true;
}

bool sql_delete_ship(const char *owner_name)
{
	if (!DB || !owner_name)
		return false;

	char *esc_owner = sql_escape_string(owner_name);
	if (!esc_owner)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "delete from ships where owner_name='%s'", esc_owner);
	free(esc_owner);

	if (!sql_run_query(query))
		return false;

	redis_invalidate_ship_snapshot(owner_name);
	return true;
}

bool sql_save_guild(Guild *guild)
{
	if (!DB || !guild)
		return false;

	unsigned int gid         = guild->get_id();
	char        *esc_name    = sql_escape_string(guild->name);
	char        *esc_fragger = sql_escape_string(guild->frags.topfragger);

	char query[1024];
	snprintf(query,
	         sizeof(query),
	         "insert into guilds (id, name, racewar, bits, prestige, construction, "
	         "platinum, gold, silver, copper, frags, top_frags, topfragger) "
	         "values (%u, '%s', %u, %u, %lu, %lu, %u, %u, %u, %u, %ld, %ld, '%s') "
	         "on duplicate key update name='%s', racewar=%u, bits=%u, prestige=%lu, "
	         "construction=%lu, platinum=%u, gold=%u, silver=%u, copper=%u, "
	         "frags=%ld, top_frags=%ld, topfragger='%s'",
	         gid,
	         esc_name ? esc_name : "",
	         guild->racewar,
	         guild->bits,
	         guild->prestige,
	         guild->construction,
	         guild->platinum,
	         guild->gold,
	         guild->silver,
	         guild->copper,
	         guild->frags.frags,
	         guild->frags.top_frags,
	         esc_fragger ? esc_fragger : "",
	         esc_name ? esc_name : "",
	         guild->racewar,
	         guild->bits,
	         guild->prestige,
	         guild->construction,
	         guild->platinum,
	         guild->gold,
	         guild->silver,
	         guild->copper,
	         guild->frags.frags,
	         guild->frags.top_frags,
	         esc_fragger ? esc_fragger : "");

	if (esc_name)
		free(esc_name);
	if (esc_fragger)
		free(esc_fragger);

	if (!sql_run_query(query))
		return false;

	// start transaction for ranks + members (DELETEs run before INSERTs, so a
	// failure mid-loop would otherwise leave the guild with stale or empty
	// ranks/members)
	if (!sql_begin_transaction())
	{
		logit(LOG_DEBUG, "sql_save_guild: failed to start transaction for guild %u", gid);
		return false;
	}

	// save ranks
	snprintf(query, sizeof(query), "delete from guild_ranks where guild_id=%u", gid);
	if (!sql_run_query(query))
	{
		logit(LOG_DEBUG, "sql_save_guild: failed to delete old ranks for guild %u", gid);
		sql_rollback();
		return false;
	}
	for (int i = 0; i < ASC_NUM_RANKS; i++)
	{
		char *esc_title = sql_escape_string(guild->titles[i]);
		if (!esc_title)
			continue;
		snprintf(query, sizeof(query), "insert into guild_ranks (guild_id, rank_index, title) values (%u, %d, '%s')", gid, i, esc_title);
		bool ok = sql_run_query(query);
		free(esc_title);
		if (!ok)
		{
			logit(LOG_DEBUG, "sql_save_guild: failed to insert rank %d for guild %u", i, gid);
			sql_rollback();
			return false;
		}
	}

	// save members
	snprintf(query, sizeof(query), "delete from guild_members where guild_id=%u", gid);
	if (!sql_run_query(query))
	{
		logit(LOG_DEBUG, "sql_save_guild: failed to delete old members for guild %u", gid);
		sql_rollback();
		return false;
	}
	for (P_member mem = guild->members; mem; mem = mem->next)
	{
		char *esc_mname = sql_escape_string(mem->name);
		if (!esc_mname)
			continue;
		int  pid = sql_get_player_pid(mem->name);
		char pid_buf[32];
		const char *pid_sql = "NULL";
		if (pid > 0)
		{
			snprintf(pid_buf, sizeof(pid_buf), "%d", pid);
			pid_sql = pid_buf;
		}
		snprintf(query,
		         sizeof(query),
		         "insert into guild_members (guild_id, player_name, player_pid, bits, debt) "
		         "values (%u, '%s', %s, %u, %u)",
		         gid,
		         esc_mname,
		         pid_sql,
		         mem->bits,
		         mem->debt);
		bool ok = sql_run_query(query);
		free(esc_mname);
		if (!ok)
		{
			logit(LOG_DEBUG, "sql_save_guild: failed to insert member for guild %u", gid);
			sql_rollback();
			return false;
		}
	}

	if (!sql_commit())
	{
		logit(LOG_DEBUG, "sql_save_guild: failed to commit for guild %u", gid);
		sql_rollback();
		return false;
	}

	return true;
}

Guild *sql_load_guild(unsigned int guild_id)
{
	if (!DB || guild_id == 0)
		return NULL;

	char query[256];
	snprintf(query,
	         sizeof(query),
	         "select id, name, racewar, bits, prestige, construction, "
	         "platinum, gold, silver, copper, frags, top_frags, topfragger "
	         "from guilds where id=%u",
	         guild_id);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		return NULL;
	}

	Guild *guild     = new Guild();
	guild->id_number = atoi(row[0]);
	strncpy(guild->name, row[1] ? row[1] : "", ASC_MAX_STR - 1);
	guild->racewar         = row[2] ? atoi(row[2]) : 0;
	guild->bits            = row[3] ? atoi(row[3]) : 0;
	guild->prestige        = row[4] ? strtoul(row[4], NULL, 10) : 0;
	guild->construction    = row[5] ? strtoul(row[5], NULL, 10) : 0;
	guild->platinum        = row[6] ? atoi(row[6]) : 0;
	guild->gold            = row[7] ? atoi(row[7]) : 0;
	guild->silver          = row[8] ? atoi(row[8]) : 0;
	guild->copper          = row[9] ? atoi(row[9]) : 0;
	guild->frags.frags     = row[10] ? atol(row[10]) : 0;
	guild->frags.top_frags = row[11] ? atol(row[11]) : 0;
	strncpy(guild->frags.topfragger, row[12] ? row[12] : "", MAX_NAME_LENGTH);
	mysql_free_result(result);

	// load ranks
	snprintf(query, sizeof(query), "select rank_index, title from guild_ranks where guild_id=%u order by rank_index", guild_id);
	result = db_query("%s", query);
	if (!result)
	{
		logit(LOG_DEBUG, "sql_load_guild: failed to load ranks for guild %u", guild_id);
		delete guild;
		return NULL;
	}
	while ((row = mysql_fetch_row(result)))
	{
		int idx = atoi(row[0]);
		if (idx >= 0 && idx < ASC_NUM_RANKS)
			strncpy(guild->titles[idx], row[1] ? row[1] : "", ASC_MAX_STR_RANK - 1);
	}
	mysql_free_result(result);

	// load members
	snprintf(query, sizeof(query), "select player_name, bits, debt from guild_members where guild_id=%u", guild_id);
	result = db_query("%s", query);
	if (!result)
	{
		logit(LOG_DEBUG, "sql_load_guild: failed to load members for guild %u", guild_id);
		delete guild;
		return NULL;
	}
	P_member tail = NULL;
	while ((row = mysql_fetch_row(result)))
	{
		P_member mem = new guild_member();
		strncpy(mem->name, row[0] ? row[0] : "", MAX_NAME_LENGTH);
		mem->bits          = row[1] ? atoi(row[1]) : 0;
		mem->debt          = row[2] ? atoi(row[2]) : 0;
		mem->online_status = GSTAT_OFFLINE;
		mem->next          = NULL;

		if (!guild->members)
			guild->members = mem;
		else
			tail->next = mem;
		tail = mem;
		guild->member_count++;
	}
	mysql_free_result(result);

	return guild;
}

bool sql_load_all_guilds()
{
	if (!DB)
		return false;

	MYSQL_RES *result = db_query("select id from guilds");
	if (!result)
		return false;

	// collect all guild IDs first (can't run queries while fetching unbuffered results)
	unsigned int guild_ids[256];
	int          num_guilds = 0;
	MYSQL_ROW    row;
	while ((row = mysql_fetch_row(result)) && num_guilds < 256)
	{
		guild_ids[num_guilds++] = atoi(row[0]);
	}
	mysql_free_result(result);

	// now load each guild
	for (int i = 0; i < num_guilds; i++)
	{
		Guild *guild = sql_load_guild(guild_ids[i]);
		if (!guild)
		{
			logit(LOG_FILE, "sql_load_all_guilds: failed to load guild rows for %u", guild_ids[i]);
			continue;
		}
		guild->next_guild = guild_list;
		guild_list        = guild;
	}

	return true;
}

bool sql_delete_guild(unsigned int guild_id)
{
	if (!DB || guild_id == 0)
		return false;

	char query[128];
	snprintf(query, sizeof(query), "delete from guilds where id=%u", guild_id);
	return sql_run_query(query);
}

// ============================================================================
// spellbook (conjurable mobs) functions
// ============================================================================

bool sql_add_spellbook_mob(int pid, int mob_vnum)
{
	if (!DB || pid <= 0)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "insert ignore into player_spellbooks (pid, mob_vnum) values (%d, %d)", pid, mob_vnum);
	return sql_run_query(query);
}

bool sql_has_spellbook_mob(int pid, int mob_vnum)
{
	if (!DB || pid <= 0)
		return false;

	char query[256];
	snprintf(query, sizeof(query), "select 1 from player_spellbooks where pid=%d and mob_vnum=%d", pid, mob_vnum);
	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	bool has = (mysql_num_rows(result) > 0);
	mysql_free_result(result);
	return has;
}

// returns array of mob vnums, sets count. caller must free array.
int *sql_get_spellbook_mobs(int pid, int *count)
{
	*count = 0;
	if (!DB || pid <= 0)
		return NULL;

	char query[256];
	snprintf(query, sizeof(query), "select mob_vnum from player_spellbooks where pid=%d order by mob_vnum", pid);
	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return NULL;

	int num = mysql_num_rows(result);
	if (num == 0)
	{
		mysql_free_result(result);
		return NULL;
	}

	int *mobs = (int *)malloc(sizeof(int) * num);
	if (!mobs)
	{
		mysql_free_result(result);
		return NULL;
	}

	MYSQL_ROW row;
	int       i = 0;
	while ((row = mysql_fetch_row(result)) && i < num)
	{
		mobs[i++] = atoi(row[0]);
	}
	mysql_free_result(result);

	*count = i;
	return mobs;
}

bool sql_delete_spellbook_mobs(int pid)
{
	if (!DB || pid <= 0)
		return false;

	char query[128];
	snprintf(query, sizeof(query), "delete from player_spellbooks where pid=%d", pid);
	return sql_run_query(query);
}

// account bank

bool sql_ensure_account_bank(const char *account_name, int racewar)
{
	if (!DB || !account_name || !*account_name)
		return false;

	char *esc_name = sql_escape_string(account_name);
	if (!esc_name)
		return false;

	char query[512];
	snprintf(query, sizeof(query), "insert ignore into account_banks (account_name, racewar) values ('%s', %d)", esc_name, racewar);

	free(esc_name);

	return sql_run_query(query);
}

bool sql_load_account_bank(const char *account_name, int racewar, P_char ch)
{
	if (!DB || !account_name || !*account_name || !ch)
		return false;

	char *esc_name = sql_escape_string(account_name);
	if (!esc_name)
		return false;

	char query[512];
	snprintf(query,
	         sizeof(query),
	         "select bank_copper, bank_silver, bank_gold, bank_platinum "
	         "from account_banks where account_name='%s' and racewar=%d",
	         esc_name,
	         racewar);

	free(esc_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (row)
	{
		GET_BALANCE_COPPER(ch)   = atoi(row[0] ? row[0] : "0");
		GET_BALANCE_SILVER(ch)   = atoi(row[1] ? row[1] : "0");
		GET_BALANCE_GOLD(ch)     = atoi(row[2] ? row[2] : "0");
		GET_BALANCE_PLATINUM(ch) = atoi(row[3] ? row[3] : "0");
		mysql_free_result(result);
		return true;
	}

	mysql_free_result(result);

	GET_BALANCE_COPPER(ch)   = 0;
	GET_BALANCE_SILVER(ch)   = 0;
	GET_BALANCE_GOLD(ch)     = 0;
	GET_BALANCE_PLATINUM(ch) = 0;
	return false;
}

bool sql_save_account_bank(const char *account_name, int racewar, P_char ch)
{
	if (!DB || !account_name || !*account_name || !ch)
		return false;

	sql_ensure_account_bank(account_name, racewar);

	char *esc_name = sql_escape_string(account_name);
	if (!esc_name)
		return false;

	char query[512];
	snprintf(query,
	         sizeof(query),
	         "update account_banks set bank_copper=%d, bank_silver=%d, bank_gold=%d, bank_platinum=%d "
	         "where account_name='%s' and racewar=%d",
	         GET_BALANCE_COPPER(ch),
	         GET_BALANCE_SILVER(ch),
	         GET_BALANCE_GOLD(ch),
	         GET_BALANCE_PLATINUM(ch),
	         esc_name,
	         racewar);

	free(esc_name);

	return sql_run_query(query);
}

long long sql_account_bank_deposit(const char *account_name, int racewar, int coin_type, int amount)
{
	if (!DB || !account_name || !*account_name || amount <= 0)
		return -1;

	sql_ensure_account_bank(account_name, racewar);

	const char *coin_col;
	switch (coin_type)
	{
		case 0:
			coin_col = "bank_copper";
			break;
		case 1:
			coin_col = "bank_silver";
			break;
		case 2:
			coin_col = "bank_gold";
			break;
		case 3:
			coin_col = "bank_platinum";
			break;
		default:
			return -1;
	}

	char *esc_name = sql_escape_string(account_name);
	if (!esc_name)
		return -1;

	char query[512];
	snprintf(query, sizeof(query), "update account_banks set %s = %s + %d where account_name='%s' and racewar=%d", coin_col, coin_col, amount, esc_name, racewar);

	if (!sql_run_query(query))
	{
		free(esc_name);
		return -1;
	}

	snprintf(query, sizeof(query), "select %s from account_banks where account_name='%s' and racewar=%d", coin_col, esc_name, racewar);

	free(esc_name);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return -1;

	MYSQL_ROW row         = mysql_fetch_row(result);
	long long new_balance = row ? atoll(row[0] ? row[0] : "0") : -1;
	mysql_free_result(result);

	return new_balance;
}

long long sql_account_bank_withdraw(const char *account_name, int racewar, int coin_type, int amount)
{
	if (!DB || !account_name || !*account_name || amount <= 0)
		return -1;

	const char *coin_col;
	switch (coin_type)
	{
		case 0:
			coin_col = "bank_copper";
			break;
		case 1:
			coin_col = "bank_silver";
			break;
		case 2:
			coin_col = "bank_gold";
			break;
		case 3:
			coin_col = "bank_platinum";
			break;
		default:
			return -1;
	}

	char *esc_name = sql_escape_string(account_name);
	if (!esc_name)
		return -1;

	char query[512];
	snprintf(query, sizeof(query), "select %s from account_banks where account_name='%s' and racewar=%d", coin_col, esc_name, racewar);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
	{
		free(esc_name);
		return -1;
	}

	MYSQL_ROW row     = mysql_fetch_row(result);
	long long current = row ? atoll(row[0] ? row[0] : "0") : 0;
	mysql_free_result(result);

	if (current < amount)
	{
		free(esc_name);
		return -2;
	}

	snprintf(query, sizeof(query), "update account_banks set %s = %s - %d where account_name='%s' and racewar=%d and %s >= %d", coin_col, coin_col, amount, esc_name, racewar, coin_col, amount);

	if (!sql_run_query(query))
	{
		free(esc_name);
		return -1;
	}

	free(esc_name);

	if (mysql_affected_rows(DB) == 0)
		return -2;

	return current - amount;
}

#endif // __NO_MYSQL__
