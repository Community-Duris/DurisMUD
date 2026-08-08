// redis dirty saves and world state persistence

#include "prototypes.h"
#include "structs.h"
#include "db.h"
#include "utility.h"
#include "utils.h"
#include "frag_cap_config.h"
#include "redis.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "copyover.h"
#include "epic.h"
#include "files.h"
#include "spells.h"
#include "sql.h"
#include "sql_player.h"
#include "ships/ships.h"

#ifndef __NO_MYSQL__
#include <cjson/cJSON.h>
#include <hiredis/hiredis.h>
#endif

extern const int               top_of_world;
extern int                     top_of_zone_table;
extern struct zone_data       *zone_table;
extern struct room_data       *world;
extern P_char                  character_list;
extern P_obj                   object_list;
extern P_index                 obj_index;
extern const struct race_names race_names_table[];
extern P_desc                  descriptor_list;

// ship object vnums defined in ships/ships.h

extern int                  _pwipe;
#ifdef __NO_MYSQL__
bool redis_clear_ship_snapshots(void)
{
	return true;
}
#endif

static redisContext *redis_ctx                 = NULL;
bool                 redis_enabled             = false;
bool                 redis_world_state_enabled = false;
int                  crash_recovery_boot       = 0;

#define REDIS_FLUSH_INTERVAL               (30 * WAIT_SEC)
#define REDIS_WORLD_STATE_INTERVAL_DEFAULT 10
#define REDIS_WORLD_STATE_MAX_AGE_DEFAULT  300

static int            world_state_interval   = REDIS_WORLD_STATE_INTERVAL_DEFAULT;
static int            world_state_max_age    = REDIS_WORLD_STATE_MAX_AGE_DEFAULT;
static volatile pid_t world_state_save_pid   = 0;
static volatile pid_t dirty_flush_pid        = 0;
static redisContext  *donation_sub_ctx       = NULL;
static volatile bool  donation_sub_connected = false;

/* Scan-and-delete with MATCH pattern. Fail closed if SCAN/DEL misshape. */
static bool redis_clear_scan_match(const char *pattern)
{
#ifndef __NO_MYSQL__
	char cursor[64] = "0";

	if (!redis_enabled || !redis_ctx || !pattern)
		return true;

	do
	{
		redisReply *scan = (redisReply *)redisCommand(redis_ctx,
				"SCAN %s MATCH %s COUNT 256", cursor, pattern);
		if (!scan || scan->type != REDIS_REPLY_ARRAY || scan->elements != 2 ||
			!scan->element[0] || !scan->element[1] ||
			!scan->element[0]->str ||
			scan->element[0]->type != REDIS_REPLY_STRING ||
			scan->element[1]->type != REDIS_REPLY_ARRAY)
		{
			if (scan)
				freeReplyObject(scan);
			return false;
		}

		snprintf(cursor, sizeof(cursor), "%s", scan->element[0]->str);
		redisReply *keys = scan->element[1];
		for (size_t i = 0; i < keys->elements; i++)
		{
			redisReply *key = keys->element[i];
			if (!key || key->type != REDIS_REPLY_STRING || !key->str)
			{
				freeReplyObject(scan);
				return false;
			}
			redisReply *del = (redisReply *)redisCommand(redis_ctx, "DEL %b",
					key->str, key->len);
			if (!del || (del->type != REDIS_REPLY_INTEGER &&
					     del->type != REDIS_REPLY_NIL))
			{
				if (del)
					freeReplyObject(del);
				freeReplyObject(scan);
				return false;
			}
			freeReplyObject(del);
		}
		freeReplyObject(scan);
	}
	while (strcmp(cursor, "0") != 0);

	return true;
#else
	(void)pattern;
	return true;
#endif
}

// rnum to vnum
static int get_room_vnum(P_char ch)
{
	if (!ch || ch->in_room < 0 || ch->in_room > top_of_world)
		return NOWHERE;
	return world[ch->in_room].number;
}

static bool redis_reconnect(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (redis_ctx)
	{
		redisFree(redis_ctx);
		redis_ctx = NULL;
	}

	const char *redis_host = getenv("REDIS_HOST");
	if (!redis_host || !*redis_host)
		redis_host = "127.0.0.1";

	const char *redis_port_str = getenv("REDIS_PORT");
	int         redis_port     = 6379;
	if (redis_port_str && *redis_port_str)
	{
		redis_port = atoi(redis_port_str);
		if (redis_port <= 0 || redis_port > 65535)
			redis_port = 6379;
	}

	redis_ctx = redisConnect(redis_host, redis_port);
	if (!redis_ctx || redis_ctx->err)
	{
		if (redis_ctx)
		{
			redisFree(redis_ctx);
			redis_ctx = NULL;
		}
		return false;
	}
	logit(LOG_SYS, "redis reconnected to %s:%d", redis_host, redis_port);
	return true;
#endif
}

void event_flush_dirty_players(P_char ch, P_char victim, P_obj obj, void *data);

bool redis_init(void)
{
#ifdef __NO_MYSQL__
	redis_enabled = false;
	return true;
#else
	const char *redis_env = getenv("REDIS");
	if (!redis_env || strcasecmp(redis_env, "TRUE") != 0)
	{
		logit(LOG_SYS, "redis disabled (set REDIS=TRUE in .env to enable)");
		redis_enabled = false;
		return true;
	}

	const char *redis_host = getenv("REDIS_HOST");
	if (!redis_host || !*redis_host)
		redis_host = "127.0.0.1";

	const char *redis_port_str = getenv("REDIS_PORT");
	int         redis_port     = 6379;
	if (redis_port_str && *redis_port_str)
	{
		redis_port = atoi(redis_port_str);
		if (redis_port <= 0 || redis_port > 65535)
			redis_port = 6379;
	}

	redis_ctx = redisConnect(redis_host, redis_port);
	if (!redis_ctx)
	{
		logit(LOG_SYS, "redis: failed to allocate context");
		redis_enabled = false;
		return false;
	}

	if (redis_ctx->err)
	{
		logit(LOG_SYS, "redis connect failed: %s", redis_ctx->errstr);
		redisFree(redis_ctx);
		redis_ctx     = NULL;
		redis_enabled = false;
		return false;
	}

	redis_enabled = true;
	logit(LOG_SYS, "redis connected to %s:%d, dirty saves enabled", redis_host, redis_port);

	// check for world state persistence
	const char *world_state_env = getenv("REDIS_WORLD_STATE");
	if (world_state_env && strcasecmp(world_state_env, "TRUE") == 0)
	{
		redis_world_state_enabled = true;

		const char *interval_str = getenv("REDIS_WORLD_STATE_INTERVAL");
		if (interval_str && *interval_str)
		{
			int interval = atoi(interval_str);
			if (interval >= 5 && interval <= 300)
				world_state_interval = interval;
		}

		const char *max_age_str = getenv("REDIS_WORLD_STATE_MAX_AGE");
		if (max_age_str && *max_age_str)
		{
			int max_age = atoi(max_age_str);
			if (max_age >= 60 && max_age <= 3600)
				world_state_max_age = max_age;
		}

		logit(LOG_SYS, "redis world state enabled: interval=%ds, max_age=%ds", world_state_interval, world_state_max_age);
	}

	// note: flush event scheduled in ne_init_events() after event system is ready

	// load obj_uid counter from redis
	redis_load_obj_uid_counter();

	redis_donation_subscribe_init();

	return true;
#endif
}

bool redis_clear_pwipe_state(void)
{
	if (!redis_enabled)
		return true;

	redis_clear_world_state();
	redis_clear_floor_drops();
	redis_clear_floor_pickups();
	redis_clear_dirty_players();
	redis_clear_online_players();
	/* Explicit season caches: leave no online-scoreboard / list bleed into new season. */
	redis_invalidate_fraglist();
	redis_invalidate_epic_zones();
	redis_invalidate_artifact_cache();
	redis_cache_del("mud:cache:named");
	if (!redis_clear_scan_match("mud:cache:artifact:*"))
		return false;
	if (!redis_clear_scan_match("mud:cache:*"))
		return false;
	return redis_clear_ship_snapshots();
}

void redis_cleanup(void)
{
#ifndef __NO_MYSQL__
	if (redis_ctx)
	{
		// save obj_uid counter before disconnect
		redis_save_obj_uid_counter();

		if (donation_sub_ctx)
		{
			redisFree(donation_sub_ctx);
			donation_sub_ctx       = NULL;
			donation_sub_connected = false;
		}

		redisFree(redis_ctx);
		redis_ctx = NULL;
	}
	redis_enabled = false;
#endif
}

bool redis_ping(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return false;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "PING");
	if (!reply)
	{
		logit(LOG_DEBUG, "redis ping failed: no reply");
		return false;
	}

	bool success = (reply->type == REDIS_REPLY_STATUS && reply->str && strcasecmp(reply->str, "PONG") == 0);
	freeReplyObject(reply);
	return success;
#else
	return false;
#endif
}

void redis_save_obj_uid_counter(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SET mud:next_obj_uid %lu", next_obj_uid);
	if (reply)
		freeReplyObject(reply);
#endif
}

void redis_load_obj_uid_counter(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET mud:next_obj_uid");
	if (reply && reply->type == REDIS_REPLY_STRING && reply->str)
	{
		unsigned long loaded = strtoul(reply->str, NULL, 10);
		if (loaded > next_obj_uid)
		{
			next_obj_uid = loaded;
			logit(LOG_SYS, "redis: loaded obj_uid counter = %lu", next_obj_uid);
		}
	}
	if (reply)
		freeReplyObject(reply);
#endif
}

void redis_log_floor_pickup(unsigned long obj_uid)
{
	if (_pwipe)
		return;
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx || obj_uid == 0)
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SADD mud:floor_pickups %lu", obj_uid);
	if (reply)
		freeReplyObject(reply);
#endif
}

bool redis_check_floor_pickup(unsigned long obj_uid)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx || obj_uid == 0)
		return false;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SISMEMBER mud:floor_pickups %lu", obj_uid);
	bool        found = (reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
	if (reply)
		freeReplyObject(reply);
	return found;
#else
	return false;
#endif
}

void redis_clear_floor_pickups(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "DEL mud:floor_pickups");
	if (reply)
		freeReplyObject(reply);
#endif
}

#define MAX_FLOOR_DROP_BATCH 64

static struct
{
	unsigned long uid;
	int           vnum;
	int           room_vnum;
	int           type;
	int           values[8];
	time_t        timers[6];
	char         *name;
	char         *short_desc;
	char         *long_desc;
	int           content_vnums[16];
	int           content_count;
} floor_drop_batch[MAX_FLOOR_DROP_BATCH];

static int           floor_drop_batch_count = 0;
static unsigned long floor_drop_removes[MAX_FLOOR_DROP_BATCH];
static int           floor_drop_remove_count = 0;

void redis_log_floor_drop(P_obj obj, int room_vnum)
{
	if (_pwipe)
		return;
#ifndef __NO_MYSQL__
	if (!obj || obj->obj_uid == 0)
		return;

	// skip old corpses (vnum 2, value[6] is timestamp) - older than 24h
	if (OBJ_VNUM(obj) == 2 && obj->value[6] > 0)
	{
		time_t corpse_time = (time_t)obj->value[6];
		if (get_time() - corpse_time > 86400)
			return;
	}

	if (floor_drop_batch_count >= MAX_FLOOR_DROP_BATCH)
	{
		redis_flush_floor_drops();
	}

	int idx                         = floor_drop_batch_count++;
	floor_drop_batch[idx].uid       = obj->obj_uid;
	floor_drop_batch[idx].vnum      = OBJ_VNUM(obj);
	floor_drop_batch[idx].room_vnum = room_vnum;
	floor_drop_batch[idx].type      = obj->type;

	for (int i = 0; i < NUMB_OBJ_VALS && i < 8; i++)
		floor_drop_batch[idx].values[i] = obj->value[i];

	for (int i = 0; i < 6; i++)
		floor_drop_batch[idx].timers[i] = obj->timer[i];

	floor_drop_batch[idx].name       = (obj->name && obj->name[0]) ? str_dup(obj->name) : NULL;
	floor_drop_batch[idx].short_desc = (obj->short_description && obj->short_description[0]) ? str_dup(obj->short_description) : NULL;
	floor_drop_batch[idx].long_desc  = (obj->description && obj->description[0]) ? str_dup(obj->description) : NULL;

	floor_drop_batch[idx].content_count = 0;
	P_obj content;
	for (content = obj->contains; content && floor_drop_batch[idx].content_count < 16; content = content->next_content)
	{
		floor_drop_batch[idx].content_vnums[floor_drop_batch[idx].content_count++] = OBJ_VNUM(content);
	}
#endif
}

void redis_flush_floor_drops(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		goto cleanup;

	if (floor_drop_batch_count == 0 && floor_drop_remove_count == 0)
		return;

	// process removes first
	for (int i = 0; i < floor_drop_remove_count; i++)
	{
		redisReply *reply = (redisReply *)redisCommand(redis_ctx, "HDEL mud:floor_drops %lu", floor_drop_removes[i]);
		if (reply)
			freeReplyObject(reply);
	}
	floor_drop_remove_count = 0;

	// process adds
	for (int i = 0; i < floor_drop_batch_count; i++)
	{
		cJSON *o = cJSON_CreateObject();
		if (!o)
			continue;

		cJSON_AddNumberToObject(o, "uid", (double)floor_drop_batch[i].uid);
		cJSON_AddNumberToObject(o, "v", floor_drop_batch[i].vnum);
		cJSON_AddNumberToObject(o, "rm", floor_drop_batch[i].room_vnum);
		cJSON_AddNumberToObject(o, "tp", floor_drop_batch[i].type);

		for (int j = 0; j < 8; j++)
		{
			if (floor_drop_batch[i].values[j] != 0)
			{
				char key[4];
				snprintf(key, sizeof(key), "v%d", j);
				cJSON_AddNumberToObject(o, key, floor_drop_batch[i].values[j]);
			}
		}

		bool has_timer = false;
		for (int j = 0; j < 6; j++)
		{
			if (floor_drop_batch[i].timers[j] != 0)
			{
				has_timer = true;
				break;
			}
		}
		if (has_timer)
		{
			cJSON *tmr = cJSON_CreateArray();
			for (int j = 0; j < 6; j++)
				cJSON_AddItemToArray(tmr, cJSON_CreateNumber((double)floor_drop_batch[i].timers[j]));
			cJSON_AddItemToObject(o, "tmr", tmr);
		}

		if (floor_drop_batch[i].name)
			cJSON_AddStringToObject(o, "nm", floor_drop_batch[i].name);
		if (floor_drop_batch[i].short_desc)
			cJSON_AddStringToObject(o, "sd", floor_drop_batch[i].short_desc);
		if (floor_drop_batch[i].long_desc)
			cJSON_AddStringToObject(o, "ld", floor_drop_batch[i].long_desc);

		if (floor_drop_batch[i].content_count > 0)
		{
			cJSON *contents = cJSON_CreateArray();
			for (int j = 0; j < floor_drop_batch[i].content_count; j++)
				cJSON_AddItemToArray(contents, cJSON_CreateNumber(floor_drop_batch[i].content_vnums[j]));
			cJSON_AddItemToObject(o, "con", contents);
		}

		char *json_str = cJSON_PrintUnformatted(o);
		cJSON_Delete(o);
		if (json_str)
		{
			redisReply *reply = (redisReply *)redisCommand(redis_ctx, "HSET mud:floor_drops %lu %s", floor_drop_batch[i].uid, json_str);
			free(json_str);
			if (reply)
				freeReplyObject(reply);
		}
	}

cleanup:
	for (int i = 0; i < floor_drop_batch_count; i++)
	{
		if (floor_drop_batch[i].name)
			str_free(floor_drop_batch[i].name);
		if (floor_drop_batch[i].short_desc)
			str_free(floor_drop_batch[i].short_desc);
		if (floor_drop_batch[i].long_desc)
			str_free(floor_drop_batch[i].long_desc);
		floor_drop_batch[i].name       = NULL;
		floor_drop_batch[i].short_desc = NULL;
		floor_drop_batch[i].long_desc  = NULL;
	}
	floor_drop_batch_count = 0;
#endif
}

void redis_remove_floor_drop(unsigned long obj_uid)
{
#ifndef __NO_MYSQL__
	if (obj_uid == 0)
		return;

	// check if it's in the pending batch - remove from there first
	for (int i = 0; i < floor_drop_batch_count; i++)
	{
		if (floor_drop_batch[i].uid == obj_uid)
		{
			if (floor_drop_batch[i].name)
				str_free(floor_drop_batch[i].name);
			if (floor_drop_batch[i].short_desc)
				str_free(floor_drop_batch[i].short_desc);
			if (floor_drop_batch[i].long_desc)
				str_free(floor_drop_batch[i].long_desc);

			// shift remaining entries
			for (int j = i; j < floor_drop_batch_count - 1; j++)
				floor_drop_batch[j] = floor_drop_batch[j + 1];
			floor_drop_batch_count--;
			return;
		}
	}

	// not in batch, queue for removal from redis
	if (floor_drop_remove_count < MAX_FLOOR_DROP_BATCH)
		floor_drop_removes[floor_drop_remove_count++] = obj_uid;
#endif
}

void redis_clear_floor_drops(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "DEL mud:floor_drops");
	if (reply)
		freeReplyObject(reply);
#endif
}

bool redis_check_floor_drop(unsigned long obj_uid)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx || obj_uid == 0)
		return false;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "HEXISTS mud:floor_drops %lu", obj_uid);
	bool        found = (reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
	if (reply)
		freeReplyObject(reply);
	return found;
#else
	return false;
#endif
}

int redis_restore_floor_drops(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return 0;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "HGETALL mud:floor_drops");
	if (!reply || reply->type != REDIS_REPLY_ARRAY)
	{
		if (reply)
			freeReplyObject(reply);
		return 0;
	}

	int restored = 0, skipped = 0;

	// hgetall returns [key, val, key, val...]
	for (size_t i = 0; i + 1 < reply->elements; i += 2)
	{
		const char *uid_str  = reply->element[i]->str;
		const char *json_str = reply->element[i + 1]->str;
		if (!uid_str || !json_str)
			continue;

		unsigned long uid = strtoul(uid_str, NULL, 10);
		if (uid == 0)
			continue;

		// already picked up? skip
		if (redis_check_floor_pickup(uid))
		{
			skipped++;
			continue;
		}

		cJSON *obj_json = cJSON_Parse(json_str);
		if (!obj_json)
			continue;

		cJSON *v  = cJSON_GetObjectItem(obj_json, "v");
		cJSON *rm = cJSON_GetObjectItem(obj_json, "rm");
		if (!v || !rm)
		{
			cJSON_Delete(obj_json);
			continue;
		}

		int vnum      = v->valueint;
		int room_vnum = rm->valueint;
		int rnum      = real_room(room_vnum);
		if (rnum < 0 || rnum > top_of_world)
		{
			cJSON_Delete(obj_json);
			continue;
		}

		P_obj obj = read_object(vnum, VIRTUAL);
		if (!obj)
		{
			cJSON_Delete(obj_json);
			continue;
		}

		obj->obj_uid = uid;

		cJSON *tp = cJSON_GetObjectItem(obj_json, "tp");
		if (tp && cJSON_IsNumber(tp))
			obj->type = tp->valueint;

		for (int j = 0; j < NUMB_OBJ_VALS; j++)
		{
			char key[4];
			snprintf(key, sizeof(key), "v%d", j);
			cJSON *val = cJSON_GetObjectItem(obj_json, key);
			if (val && cJSON_IsNumber(val))
				obj->value[j] = val->valueint;
		}

		cJSON *tmr = cJSON_GetObjectItem(obj_json, "tmr");
		if (tmr && cJSON_IsArray(tmr))
		{
			int    idx = 0;
			cJSON *t;
			cJSON_ArrayForEach(t, tmr)
			{
				if (idx < 6 && cJSON_IsNumber(t))
					obj->timer[idx] = (time_t)t->valuedouble;
				idx++;
			}
		}

		cJSON *nm = cJSON_GetObjectItem(obj_json, "nm");
		cJSON *sd = cJSON_GetObjectItem(obj_json, "sd");
		cJSON *ld = cJSON_GetObjectItem(obj_json, "ld");
		if (nm && cJSON_IsString(nm) && nm->valuestring[0])
		{
			if ((obj->str_mask & STRUNG_KEYS) && obj->name)
				str_free(obj->name);
			obj->name = str_dup(nm->valuestring);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (sd && cJSON_IsString(sd) && sd->valuestring[0])
		{
			if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
				str_free(obj->short_description);
			obj->short_description = str_dup(sd->valuestring);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (ld && cJSON_IsString(ld) && ld->valuestring[0])
		{
			if ((obj->str_mask & STRUNG_DESC1) && obj->description)
				str_free(obj->description);
			obj->description = str_dup(ld->valuestring);
			obj->str_mask |= STRUNG_DESC1;
		}

		obj_to_room(obj, rnum);

		cJSON *con = cJSON_GetObjectItem(obj_json, "con");
		if (con && cJSON_IsArray(con))
		{
			cJSON *cont_vnum;
			cJSON_ArrayForEach(cont_vnum, con)
			{
				if (!cJSON_IsNumber(cont_vnum))
					continue;
				int content_vnum = cont_vnum->valueint;
				if (content_vnum > 0)
				{
					P_obj content = read_object(content_vnum, VIRTUAL);
					if (content)
						obj_to_obj(content, obj);
				}
			}
		}

		cJSON_Delete(obj_json);
		restored++;
	}

	freeReplyObject(reply);

	if (skipped > 0)
		logit(LOG_SYS, "redis: floor drops: skipped %d picked-up items", skipped);
	if (restored > 0)
		logit(LOG_SYS, "redis: floor drops: restored %d items", restored);

	// dont clear floor_drops here - world_state restore needs to check against it
	// cleared after world_state restore completes

	return restored;
#else
	return 0;
#endif
}

// debounce dirty marks - skip if already marked this pid recently
#define MAX_DIRTY_DEBOUNCE 64
static struct
{
	int    pid;
	time_t last_mark;
} dirty_debounce[MAX_DIRTY_DEBOUNCE];
static int dirty_debounce_count = 0;

// returns true if we should proceed, false if debounced
static bool check_dirty_debounce(int pid)
{
	time_t now = get_time();

	for (int i = 0; i < dirty_debounce_count; i++)
	{
		if (dirty_debounce[i].pid == pid)
		{
			if (now - dirty_debounce[i].last_mark < 1) // 1 second debounce
				return false;
			dirty_debounce[i].last_mark = now;
			return true;
		}
	}

	// not found, add it
	if (dirty_debounce_count < MAX_DIRTY_DEBOUNCE)
	{
		dirty_debounce[dirty_debounce_count].pid       = pid;
		dirty_debounce[dirty_debounce_count].last_mark = now;
		dirty_debounce_count++;
	}
	return true;
}

static void redis_restore_dirty_snapshot(const char *inflight_key)
{
	if (!redis_enabled || !redis_ctx || !inflight_key)
		return;

	redisReply *restore = (redisReply *)redisCommand(redis_ctx,
	                                               "SUNIONSTORE mud:dirty_players 2 mud:dirty_players %s",
	                                               inflight_key);
	if (restore)
	{
		bool restored = (restore->type != REDIS_REPLY_ERROR);
		freeReplyObject(restore);
		if (restored)
		{
			redisReply *del = (redisReply *)redisCommand(redis_ctx, "DEL %s", inflight_key);
			if (del)
				freeReplyObject(del);
		}
	}
}

void mark_player_dirty(int pid)
{
	if (_pwipe)
		return;
#ifndef __NO_MYSQL__
	if (!redis_enabled || pid <= 0)
		return;

	// debounce - skip if we marked this pid dirty within the last second
	if (!check_dirty_debounce(pid))
	{
		redisReply *queued = (redisReply *)redisCommand(redis_ctx, "SISMEMBER mud:dirty_players %d", pid);
		if (queued)
		{
			bool already_queued = (queued->type == REDIS_REPLY_INTEGER && queued->integer == 1);
			freeReplyObject(queued);
			if (already_queued)
				return;
		}
	}

	if (!redis_ctx || redis_ctx->err)
	{
		if (!redis_reconnect())
		{
			redis_enabled = false;
			P_char ch     = find_player_by_pid(pid);
			if (ch && IS_PC(ch))
			{
				// Wrap save in transaction
				if (sql_begin_transaction())
				{
					sql_save_player(ch, RENT_CRASH, 0);
					if (!sql_commit()) sql_rollback();
				}
				else
					sql_save_player(ch, RENT_CRASH, 0);
			}
			return;
		}
	}

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SADD mud:dirty_players %d", pid);
	if (!reply)
	{
		P_char ch = find_player_by_pid(pid);
		if (ch && IS_PC(ch))
		{
			// Wrap save in transaction
			if (sql_begin_transaction())
			{
				sql_save_player(ch, RENT_CRASH, 0);
				if (!sql_commit()) sql_rollback();
			}
			else
				sql_save_player(ch, RENT_CRASH, 0);
		}
		return;
	}
	freeReplyObject(reply);
#endif
}

void flush_dirty_players(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	const char *inflight_key = "mud:dirty_players:flushing";

	// skip if previous async save still running
	if (dirty_flush_pid > 0)
	{
		int   status;
		pid_t result = waitpid(dirty_flush_pid, &status, WNOHANG);
		if (result == 0)
			return;
		if (result > 0)
		{
			if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
			{
				redisReply *del = (redisReply *)redisCommand(redis_ctx, "DEL %s", inflight_key);
				if (del)
					freeReplyObject(del);
			}
			else
			{
				logit(LOG_SYS, "flush_dirty: previous async save failed, restoring dirty set for retry");
				redis_restore_dirty_snapshot(inflight_key);
			}
		}
		dirty_flush_pid = 0;
	}

	if (!redis_ctx || redis_ctx->err)
	{
		if (!redis_reconnect())
		{
			redis_enabled = false;
			return;
		}
	}

	redisReply *rename = (redisReply *)redisCommand(redis_ctx, "RENAME mud:dirty_players %s", inflight_key);
	if (!rename)
		return;
	if (rename->type == REDIS_REPLY_ERROR)
	{
		freeReplyObject(rename);
		return;
	}
	freeReplyObject(rename);

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SMEMBERS %s", inflight_key);
	if (!reply)
		return;

	if (reply->type != REDIS_REPLY_ARRAY || reply->elements == 0)
	{
		freeReplyObject(reply);
		return;
	}

	// copy pids to array
	int  count = (int)reply->elements;
	int *pids  = (int *)malloc(count * sizeof(int));
	if (!pids)
	{
		freeReplyObject(reply);
		redis_restore_dirty_snapshot(inflight_key);
		return;
	}

	int valid = 0;
	for (size_t i = 0; i < reply->elements; i++)
	{
		if (reply->element[i]->type == REDIS_REPLY_STRING)
		{
			int pid = atoi(reply->element[i]->str);
			// only keep pids of players actually online
			P_char ch = find_player_by_pid(pid);
			if (ch && IS_PC(ch))
				pids[valid++] = pid;
		}
	}
	freeReplyObject(reply);

	if (valid == 0)
	{
		free(pids);
		redisReply *del = (redisReply *)redisCommand(redis_ctx, "DEL %s", inflight_key);
		if (del)
			freeReplyObject(del);
		return;
	}


	// fork for async save
	logit(LOG_SYS, "flush_dirty: saving %d online players async", valid);
	pid_t pid = fork();
	if (pid < 0)
	{
		// fork failed, sync fallback
		logit(LOG_SYS, "flush_dirty: fork failed, saving sync");
		for (int i = 0; i < valid; i++)
		{
			P_char ch = find_player_by_pid(pids[i]);
			if (!ch || !IS_PC(ch))
				continue;
			// Wrap save in transaction
			if (sql_begin_transaction())
			{
				sql_save_player(ch, RENT_CRASH, get_room_vnum(ch));
				if (!sql_commit()) sql_rollback();
			}
			else
				sql_save_player(ch, RENT_CRASH, get_room_vnum(ch));
			// gremlin needs to run in main process
			if (ch->in_room != NOWHERE && IS_ROOM(ch->in_room, ROOM_LOCKER) && world[ch->in_room].funct)
				(*world[ch->in_room].funct)(ch->in_room, ch, (-81), NULL);
		}
		free(pids);
		redisReply *del = (redisReply *)redisCommand(redis_ctx, "DEL %s", inflight_key);
		if (del)
			freeReplyObject(del);
		return;
	}

	if (pid == 0)
	{
		// child
		MYSQL *child_conn = sql_create_child_connection();
		if (!child_conn)
		{
			free(pids);
			_exit(1);
		}
		sql_reset_for_child(child_conn);

		bool all_ok = true;
		for (int i = 0; i < valid; i++)
		{
			P_char ch = find_player_by_pid(pids[i]);
			if (ch && IS_PC(ch))
			{
				// Wrap save in transaction
				if (sql_begin_transaction())
				{
					if (!sql_save_player(ch, RENT_CRASH, get_room_vnum(ch)))
						all_ok = false;
					if (!sql_commit())
					{
						sql_rollback();
						all_ok = false;
					}
				}
				else if (!sql_save_player(ch, RENT_CRASH, get_room_vnum(ch)))
				{
					all_ok = false;
				}
			}
		}

		mysql_close(child_conn);
		free(pids);
		_exit(all_ok ? 0 : 1);
	}

	// parent - gremlin events wont work in forked child so do it here
	// also clear dirty container flags since child has snapshot of them
	for (int i = 0; i < valid; i++)
	{
		P_char ch = find_player_by_pid(pids[i]);
		if (ch && IS_PC(ch))
		{
			clear_player_dirty_container_flags(ch);
			if (ch->in_room != NOWHERE && IS_ROOM(ch->in_room, ROOM_LOCKER) && world[ch->in_room].funct)
				(*world[ch->in_room].funct)(ch->in_room, ch, (-81), NULL);
		}
	}

	dirty_flush_pid = pid;
	free(pids);
#endif
}

int get_dirty_player_count(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return 0;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SCARD mud:dirty_players");
	if (!reply)
		return 0;

	int count = 0;
	if (reply->type == REDIS_REPLY_INTEGER)
		count = (int)reply->integer;

	freeReplyObject(reply);
	return count;
#else
	return 0;
#endif
}

void event_flush_dirty_players(P_char ch, P_char victim, P_obj obj, void *data)
{
	flush_dirty_players();
	redis_flush_floor_drops();

	if (redis_enabled)
		add_event(event_flush_dirty_players, REDIS_FLUSH_INTERVAL, NULL, NULL, NULL, 0, NULL, 0);
}

#define REDIS_WORLD_STATE_VER 6

// json world state save - v2 format with short keys for compactness
static bool redis_save_world_state_json(redisContext *ctx)
{
#ifdef __NO_MYSQL__
	return false;
#else
	struct timeval start, end;
	gettimeofday(&start, NULL);

	cJSON *root = cJSON_CreateObject();
	if (!root)
		return false;

	cJSON_AddNumberToObject(root, "ver", REDIS_WORLD_STATE_VER);
	cJSON_AddNumberToObject(root, "ts", (double)get_time());

	// mobs array
	cJSON *mobs = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "mobs", mobs);

	for (P_char ch = character_list; ch; ch = ch->next)
	{
		if (!IS_NPC(ch) || ch->in_room < 0 || IS_PC_PET(ch))
			continue;

		cJSON *mob = cJSON_CreateObject();
		cJSON_AddNumberToObject(mob, "v", GET_VNUM(ch));
		cJSON_AddNumberToObject(mob, "id", GET_IDNUM(ch));
		cJSON_AddNumberToObject(mob, "rm", world[ch->in_room].number);
		cJSON_AddNumberToObject(mob, "bp", ch->player.birthplace);
		cJSON_AddNumberToObject(mob, "hp", GET_HIT(ch));
		cJSON_AddNumberToObject(mob, "mhp", GET_MAX_HIT(ch));
		cJSON_AddNumberToObject(mob, "mn", GET_MANA(ch));
		cJSON_AddNumberToObject(mob, "mmn", GET_MAX_MANA(ch));
		cJSON_AddNumberToObject(mob, "vt", GET_VITALITY(ch));
		cJSON_AddNumberToObject(mob, "mvt", GET_MAX_VITALITY(ch));
		cJSON_AddNumberToObject(mob, "act", (int)ch->specials.act);
		cJSON_AddNumberToObject(mob, "act2", (int)ch->specials.act2);
		cJSON_AddNumberToObject(mob, "act3", (int)ch->specials.act3);
		if (GET_GOLD(ch) > 0)
			cJSON_AddNumberToObject(mob, "gld", GET_GOLD(ch));

		if (IS_SET(ch->only.npc->str_mask, STRUNG_KEYS))
		{
			cJSON_AddStringToObject(mob, "nm", GET_NAME(ch));
		}

		if (IS_SET(ch->only.npc->str_mask, STRUNG_DESC1))
		{
			cJSON_AddStringToObject(mob, "ld", ch->player.long_descr);
		}

		if (IS_SET(ch->only.npc->str_mask, STRUNG_DESC2))
		{
			cJSON_AddStringToObject(mob, "sd", ch->player.short_descr);
		}

		if (ch->following)
		{
			cJSON_AddNumberToObject(mob, "fid", GET_IDNUM(ch->following));
		}

		if (get_linking_char(ch, LNK_RIDING) != NULL)
		{
			cJSON_AddNumberToObject(mob, "rid", GET_IDNUM(get_linking_char(ch, LNK_RIDING)));
		}

		// equipment as slot:vnum object (only non-empty slots)
		cJSON *eq     = cJSON_CreateObject();
		bool   has_eq = false;
		for (int w = 0; w < MAX_WEAR; w++)
		{
			if (ch->equipment[w])
			{
				char slot[8];
				snprintf(slot, sizeof(slot), "%d", w);
				cJSON_AddNumberToObject(eq, slot, OBJ_VNUM(ch->equipment[w]));
				has_eq = true;
			}
		}
		if (has_eq)
			cJSON_AddItemToObject(mob, "eq", eq);
		else
			cJSON_Delete(eq);

		// inventory as vnum array
		P_obj obj;
		int   inv_count = 0;
		for (obj = ch->carrying; obj; obj = obj->next_content)
			inv_count++;
		if (inv_count > 0)
		{
			cJSON *inv = cJSON_CreateArray();
			for (obj = ch->carrying; obj; obj = obj->next_content)
				cJSON_AddItemToArray(inv, cJSON_CreateNumber(OBJ_VNUM(obj)));
			cJSON_AddItemToObject(mob, "inv", inv);
		}

		// affects array
		struct affected_type *af;
		int                   aff_count = 0;
		for (af = ch->affected; af; af = af->next)
			aff_count++;
		if (aff_count > 0)
		{
			cJSON *affs = cJSON_CreateArray();
			for (af = ch->affected; af; af = af->next)
			{
				cJSON *a = cJSON_CreateObject();
				cJSON_AddNumberToObject(a, "t", af->type);
				cJSON_AddNumberToObject(a, "d", af->duration);
				if (af->modifier != 0)
					cJSON_AddNumberToObject(a, "m", af->modifier);
				if (af->location != 0)
					cJSON_AddNumberToObject(a, "l", af->location);
				if (af->level != 0)
					cJSON_AddNumberToObject(a, "lv", af->level);
				if (af->bitvector != 0)
					cJSON_AddNumberToObject(a, "b1", (double)af->bitvector);
				if (af->bitvector2 != 0)
					cJSON_AddNumberToObject(a, "b2", (double)af->bitvector2);
				if (af->bitvector3 != 0)
					cJSON_AddNumberToObject(a, "b3", (double)af->bitvector3);
				if (af->bitvector4 != 0)
					cJSON_AddNumberToObject(a, "b4", (double)af->bitvector4);
				if (af->bitvector5 != 0)
					cJSON_AddNumberToObject(a, "b5", (double)af->bitvector5);
				cJSON_AddItemToArray(affs, a);
			}
			cJSON_AddItemToObject(mob, "aff", affs);
		}

		cJSON_AddItemToArray(mobs, mob);
	}

	// floor objects array
	cJSON *objs = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "objs", objs);

	for (P_obj obj = object_list; obj; obj = obj->next)
	{
		if (!OBJ_ROOM(obj))
			continue;

		int vnum = OBJ_VNUM(obj);
		if (vnum == VOBJ_PANEL || vnum == VOBJ_ALL_SHIPS || vnum == VOBJ_CARGO_CRATE)
			continue;

		// skip old corpses (value[6] is CORPSE_SAVEID timestamp) - older than 24h
		if (vnum == 2 && obj->value[6] > 0)
		{
			time_t corpse_time = (time_t)obj->value[6];
			if (get_time() - corpse_time > 86400)
				continue;
		}

		cJSON *o = cJSON_CreateObject();
		cJSON_AddNumberToObject(o, "uid", (double)obj->obj_uid);
		cJSON_AddNumberToObject(o, "v", vnum);
		cJSON_AddNumberToObject(o, "rm", world[obj->loc.room].number);
		cJSON_AddNumberToObject(o, "tp", obj->type);

		// values - only non-zero
		for (int i = 0; i < NUMB_OBJ_VALS; i++)
		{
			if (obj->value[i] != 0)
			{
				char key[4];
				snprintf(key, sizeof(key), "v%d", i);
				cJSON_AddNumberToObject(o, key, obj->value[i]);
			}
		}

		// timer - only if any non-zero
		bool has_timer = false;
		for (int i = 0; i < 6; i++)
		{
			if (obj->timer[i] != 0)
			{
				has_timer = true;
				break;
			}
		}
		if (has_timer)
		{
			cJSON *tmr = cJSON_CreateArray();
			for (int i = 0; i < 6; i++)
				cJSON_AddItemToArray(tmr, cJSON_CreateNumber((double)obj->timer[i]));
			cJSON_AddItemToObject(o, "tmr", tmr);
		}

		// strung strings (corpses etc)
		if (IS_SET(obj->str_mask, STRUNG_KEYS) && obj->name && obj->name[0])
			cJSON_AddStringToObject(o, "nm", obj->name);
		if (IS_SET(obj->str_mask, STRUNG_DESC2) && obj->short_description && obj->short_description[0])
			cJSON_AddStringToObject(o, "sd", obj->short_description);
		if (IS_SET(obj->str_mask, STRUNG_DESC1) && obj->description && obj->description[0])
			cJSON_AddStringToObject(o, "ld", obj->description);

		// contents
		P_obj content;
		int   cont_count = 0;
		for (content = obj->contains; content; content = content->next_content)
			cont_count++;
		if (cont_count > 0)
		{
			cJSON *contents = cJSON_CreateArray();
			for (content = obj->contains; content; content = content->next_content)
				cJSON_AddItemToArray(contents, cJSON_CreateNumber(OBJ_VNUM(content)));
			cJSON_AddItemToObject(o, "con", contents);
		}

		cJSON_AddItemToArray(objs, o);
	}

	// doors array
	cJSON *doors = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "doors", doors);

	for (int room = 0; room <= top_of_world; room++)
	{
		for (int dir = 0; dir < NUM_EXITS; dir++)
		{
			if (world[room].dir_option[dir] && IS_SET(world[room].dir_option[dir]->exit_info, EX_ISDOOR))
			{
				cJSON *d = cJSON_CreateObject();
				cJSON_AddNumberToObject(d, "rm", world[room].number);
				cJSON_AddNumberToObject(d, "dr", dir);
				cJSON_AddNumberToObject(d, "st", world[room].dir_option[dir]->exit_info);
				cJSON_AddItemToArray(doors, d);
			}
		}
	}

	// zones array
	cJSON *zones = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "zones", zones);

	for (int z = 0; z <= top_of_zone_table; z++)
	{
		cJSON *zn = cJSON_CreateObject();
		cJSON_AddNumberToObject(zn, "rn", z);
		cJSON_AddNumberToObject(zn, "age", zone_table[z].age);
		cJSON_AddNumberToObject(zn, "ls", zone_table[z].lifespan);
		cJSON_AddItemToArray(zones, zn);
	}

	// output to redis
	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	if (!json)
		return false;

	size_t      json_len = strlen(json);
	redisReply *reply    = (redisReply *)redisCommand(ctx, "SET mud:world_state %s", json);
	free(json);

	if (!reply)
		return false;
	freeReplyObject(reply);

	char timestamp_str[32];
	snprintf(timestamp_str, sizeof(timestamp_str), "%ld", (long)get_time());
	reply = (redisReply *)redisCommand(ctx, "SET mud:world_state:timestamp %s", timestamp_str);
	if (reply)
		freeReplyObject(reply);

	reply = (redisReply *)redisCommand(ctx, "SET mud:world_state:valid 1");
	if (reply)
		freeReplyObject(reply);

	gettimeofday(&end, NULL);
	long ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
	logit(LOG_SYS, "redis: world state json saved in %ldms (%.1fMB)", ms, json_len / 1048576.0);

	return true;
#endif
}

// sync version, called by forked child - needs own redis connection
static bool redis_save_world_state_sync(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	const char *redis_host = getenv("REDIS_HOST");
	if (!redis_host || !*redis_host)
		redis_host = "127.0.0.1";

	const char *redis_port_str = getenv("REDIS_PORT");
	int         redis_port     = 6379;
	if (redis_port_str && *redis_port_str)
	{
		redis_port = atoi(redis_port_str);
		if (redis_port <= 0 || redis_port > 65535)
			redis_port = 6379;
	}

	redisContext *ctx = redisConnect(redis_host, redis_port);
	if (!ctx || ctx->err)
	{
		if (ctx)
			redisFree(ctx);
		return false;
	}

	redisReply *valid_reply = (redisReply *)redisCommand(ctx, "SET mud:world_state:valid 0");
	if (valid_reply)
		freeReplyObject(valid_reply);

	// use json format instead of binary
	bool result = redis_save_world_state_json(ctx);
	redisFree(ctx);
	return result;
#endif
}

// forks child to avoid blocking main loop
bool redis_save_world_state(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_world_state_enabled)
		return false;

	if (world_state_save_pid > 0)
	{
		int   status;
		pid_t result = waitpid(world_state_save_pid, &status, WNOHANG);
		if (result == 0)
		{
			// still running
			logit(LOG_DEBUG, "redis: world state save still in progress, skipping");
			return true;
		}
		world_state_save_pid = 0;
	}

	int num_mobs, num_objs, num_rooms;
	copyover_count_items(&num_mobs, &num_objs, &num_rooms);
	int num_zones = top_of_zone_table + 1;

	logit(LOG_SYS, "redis: saving world state (async, %d mobs, %d objs, %d doors, %d zones)", num_mobs, num_objs, num_rooms, num_zones);

	pid_t pid = fork();
	if (pid < 0)
	{
		logit(LOG_SYS, "redis: fork failed for world state save");
		return false;
	}

	if (pid == 0)
	{
		bool success = redis_save_world_state_sync();
		_exit(success ? 0 : 1);
	}

	world_state_save_pid = pid;
	return true;
#endif
}

bool redis_has_world_state(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_world_state_enabled)
		return false;

	if (!redis_ctx || redis_ctx->err)
	{
		if (!redis_reconnect())
			return false;
	}

	// check valid flag
	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET mud:world_state:valid");
	if (!reply)
		return false;

	bool valid = false;
	if (reply->type == REDIS_REPLY_STRING && reply->str && strcmp(reply->str, "1") == 0)
		valid = true;

	freeReplyObject(reply);

	if (!valid)
		return false;

	// check timestamp
	reply = (redisReply *)redisCommand(redis_ctx, "GET mud:world_state:timestamp");
	if (!reply)
		return false;

	time_t saved_time = 0;
	if (reply->type == REDIS_REPLY_STRING && reply->str)
		saved_time = (time_t)atol(reply->str);

	freeReplyObject(reply);

	if (saved_time == 0)
		return false;

	time_t now = get_time();
	if (now - saved_time > world_state_max_age)
	{
		logit(LOG_SYS, "redis: world state too old (%ld seconds), ignoring", (long)(now - saved_time));
		redis_clear_world_state();
		return false;
	}

	// check data exists
	reply = (redisReply *)redisCommand(redis_ctx, "EXISTS mud:world_state");
	if (!reply)
		return false;

	bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
	freeReplyObject(reply);

	return exists;
#endif
}

time_t redis_world_state_timestamp(void)
{
#ifdef __NO_MYSQL__
	return 0;
#else
	if (!redis_enabled || !redis_ctx)
		return 0;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET mud:world_state:timestamp");
	if (!reply)
		return 0;

	time_t ts = 0;
	if (reply->type == REDIS_REPLY_STRING && reply->str)
		ts = (time_t)atol(reply->str);

	freeReplyObject(reply);
	return ts;
#endif
}

void redis_clear_world_state(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "DEL mud:world_state mud:world_state:timestamp mud:world_state:valid");
	if (reply)
		freeReplyObject(reply);

	logit(LOG_SYS, "redis: cleared world state snapshot");
#endif
}

// json world state load - parses v2 format saved by redis_save_world_state_json
static bool redis_load_world_state_json(const char *json)
{
#ifdef __NO_MYSQL__
	return false;
#else
	cJSON *root = cJSON_Parse(json);
	if (!root)
	{
		logit(LOG_SYS, "redis: json parse failed");
		return false;
	}

	// check version
	cJSON *ver = cJSON_GetObjectItem(root, "ver");
	if (!ver || !cJSON_IsNumber(ver) || ver->valueint != REDIS_WORLD_STATE_VER)
	{
		logit(LOG_SYS, "redis: json version mismatch (expected %d, got %d)", REDIS_WORLD_STATE_VER, ver ? ver->valueint : 0);
		cJSON_Delete(root);
		return false;
	}

	int mobs_restored = 0, objs_restored = 0, objs_skipped = 0;
	int doors_restored = 0, zones_restored = 0;

	struct follow_riding_data {
		P_char                     mob;
		int                        followingID;
		int                        ridingID;
		struct follow_riding_data *next;
	} *frdHead = NULL;

	// restore mobs
	cJSON *mobs = cJSON_GetObjectItem(root, "mobs");
	if (mobs && cJSON_IsArray(mobs))
	{
		cJSON *mob_json;
		cJSON_ArrayForEach(mob_json, mobs)
		{
			cJSON *v  = cJSON_GetObjectItem(mob_json, "v");
			cJSON *rm = cJSON_GetObjectItem(mob_json, "rm");
			if (!v || !rm)
				continue;

			int vnum      = v->valueint;
			int room_vnum = rm->valueint;

			int mob_rnum = real_mobile(vnum);
			if (mob_rnum < 0)
				continue;

			int rnum = real_room(room_vnum);
			if (rnum < 0 || rnum > top_of_world)
				continue;

			P_char mob = read_mobile(mob_rnum, REAL);
			if (!mob)
				continue;

			// restore idnum
			cJSON *id = cJSON_GetObjectItem(mob_json, "id");
			if (id && cJSON_IsNumber(id))
				GET_IDNUM(mob) = id->valueint;

			char_to_room(mob, rnum, -2);

			// restore stats
			cJSON *hp   = cJSON_GetObjectItem(mob_json, "hp");
			cJSON *mhp  = cJSON_GetObjectItem(mob_json, "mhp");
			cJSON *mn   = cJSON_GetObjectItem(mob_json, "mn");
			cJSON *mmn  = cJSON_GetObjectItem(mob_json, "mmn");
			cJSON *vt   = cJSON_GetObjectItem(mob_json, "vt");
			cJSON *mvt  = cJSON_GetObjectItem(mob_json, "mvt");
			cJSON *gld  = cJSON_GetObjectItem(mob_json, "gld");
			cJSON *bp   = cJSON_GetObjectItem(mob_json, "bp");
			cJSON *act  = cJSON_GetObjectItem(mob_json, "act");
			cJSON *act2 = cJSON_GetObjectItem(mob_json, "act2");
			cJSON *act3 = cJSON_GetObjectItem(mob_json, "act3");

			if (hp && cJSON_IsNumber(hp))
				GET_HIT(mob) = hp->valueint;
			if (mhp && cJSON_IsNumber(mhp))
				GET_MAX_HIT(mob) = mhp->valueint;
			if (mn && cJSON_IsNumber(mn))
				GET_MANA(mob) = mn->valueint;
			if (mmn && cJSON_IsNumber(mmn))
				GET_MAX_MANA(mob) = mmn->valueint;
			if (vt && cJSON_IsNumber(vt))
				GET_VITALITY(mob) = vt->valueint;
			if (mvt && cJSON_IsNumber(mvt))
				GET_MAX_VITALITY(mob) = mvt->valueint;
			if (gld && cJSON_IsNumber(gld))
				GET_GOLD(mob) = gld->valueint;
			if (bp && cJSON_IsNumber(bp))
				GET_BIRTHPLACE(mob) = bp->valueint;
			if (act && cJSON_IsNumber(act))
				mob->specials.act = (uint)act->valueint;
			if (act2 && cJSON_IsNumber(act2))
				mob->specials.act2 = (uint)act2->valueint;
			if (act3 && cJSON_IsNumber(act3))
				mob->specials.act3 = (uint)act3->valueint;

			SET_POS(mob, POS_STANDING + STAT_NORMAL);

			// handle restrung mobs (like randoms)
			cJSON *name  = cJSON_GetObjectItem(mob_json, "nm");
			cJSON *shortD = cJSON_GetObjectItem(mob_json, "sd");
			cJSON *longD = cJSON_GetObjectItem(mob_json, "ld");
			if (name && cJSON_IsString(name))
			{
				mob->only.npc->str_mask |= STRUNG_KEYS;
				GET_NAME(mob) = str_dup(name->valuestring);
			}
			if (shortD && cJSON_IsString(shortD))
			{
				mob->only.npc->str_mask |= STRUNG_DESC2;
				mob->player.short_descr = str_dup(shortD->valuestring);
			}
			if (longD && cJSON_IsString(longD))
			{
				mob->only.npc->str_mask |= STRUNG_DESC1;
				mob->player.long_descr = str_dup(longD->valuestring);
			}

			// restore equipment
			cJSON *eq = cJSON_GetObjectItem(mob_json, "eq");
			if (eq && cJSON_IsObject(eq))
			{
				cJSON *slot_obj;
				cJSON_ArrayForEach(slot_obj, eq)
				{
					if (!cJSON_IsNumber(slot_obj))
						continue;
					int slot    = atoi(slot_obj->string);
					int eq_vnum = slot_obj->valueint;
					if (slot >= 0 && slot < MAX_WEAR && eq_vnum > 0)
					{
						P_obj obj = read_object(eq_vnum, VIRTUAL);
						if (obj)
							equip_char(mob, obj, slot, 0);
					}
				}
			}

			// restore inventory
			cJSON *inv = cJSON_GetObjectItem(mob_json, "inv");
			if (inv && cJSON_IsArray(inv))
			{
				cJSON *inv_vnum;
				cJSON_ArrayForEach(inv_vnum, inv)
				{
					if (!cJSON_IsNumber(inv_vnum))
						continue;
					int item_vnum = inv_vnum->valueint;
					if (item_vnum > 0)
					{
						P_obj obj = read_object(item_vnum, VIRTUAL);
						if (obj)
							obj_to_char(obj, mob);
					}
				}
			}

			// restore affects
			cJSON *affs = cJSON_GetObjectItem(mob_json, "aff");
			if (affs && cJSON_IsArray(affs))
			{
				cJSON *aff_json;
				cJSON_ArrayForEach(aff_json, affs)
				{
					struct affected_type af;
					memset(&af, 0, sizeof(af));

					cJSON *t  = cJSON_GetObjectItem(aff_json, "t");
					cJSON *d  = cJSON_GetObjectItem(aff_json, "d");
					cJSON *m  = cJSON_GetObjectItem(aff_json, "m");
					cJSON *l  = cJSON_GetObjectItem(aff_json, "l");
					cJSON *lv = cJSON_GetObjectItem(aff_json, "lv");
					cJSON *b1 = cJSON_GetObjectItem(aff_json, "b1");
					cJSON *b2 = cJSON_GetObjectItem(aff_json, "b2");
					cJSON *b3 = cJSON_GetObjectItem(aff_json, "b3");
					cJSON *b4 = cJSON_GetObjectItem(aff_json, "b4");
					cJSON *b5 = cJSON_GetObjectItem(aff_json, "b5");

					if (t && cJSON_IsNumber(t))
						af.type = t->valueint;
					if (d && cJSON_IsNumber(d))
						af.duration = d->valueint;
					if (m && cJSON_IsNumber(m))
						af.modifier = m->valueint;
					if (l && cJSON_IsNumber(l))
						af.location = l->valueint;
					if (lv && cJSON_IsNumber(lv))
						af.level = lv->valueint;
					if (b1 && cJSON_IsNumber(b1))
						af.bitvector = (unsigned long)b1->valuedouble;
					if (b2 && cJSON_IsNumber(b2))
						af.bitvector2 = (unsigned long)b2->valuedouble;
					if (b3 && cJSON_IsNumber(b3))
						af.bitvector3 = (unsigned long)b3->valuedouble;
					if (b4 && cJSON_IsNumber(b4))
						af.bitvector4 = (unsigned long)b4->valuedouble;
					if (b5 && cJSON_IsNumber(b5))
						af.bitvector5 = (unsigned long)b5->valuedouble;

					affect_to_char(mob, &af);
				}
			}

			cJSON *fid = cJSON_GetObjectItem(mob_json, "fid");
			cJSON *rid = cJSON_GetObjectItem(mob_json, "rid");

			struct follow_riding_data *frd = NULL;

			if (fid)
			{
				if (!frd)
				{
					CREATE(frd, struct follow_riding_data, 1, "MFRD");
					frd->next = frdHead;
					frdHead   = frd;
					frd->mob  = mob;
				}
				frd->followingID = fid->valueint;
			}

			if (rid)
			{
				if (!frd)
				{
					CREATE(frd, struct follow_riding_data, 1, "MFRD");
					frd->next = frdHead;
					frdHead   = frd;
					frd->mob  = mob;
				}
				frd->ridingID = rid->valueint;
			}

			mobs_restored++;
		}

		for (struct follow_riding_data *it = frdHead; it;)
		{
			if (it->followingID != 0)
			{
				for (P_char tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
				{
					if (GET_IDNUM(tmp_ch) == it->followingID)
					{
						add_follower(it->mob, tmp_ch);
						char buf[10];
						strcpy(buf, "group all");
						command_interpreter(tmp_ch, buf);
						break;
					}
				}
			}

			if (it->ridingID != 0)
			{
				for (P_char tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
				{
					if (GET_IDNUM(tmp_ch) == it->ridingID)
					{
						char buf[MAX_STRING_LENGTH];
						snprintf(buf, MAX_STRING_LENGTH, "%s", FirstWord(GET_NAME(it->mob)));
						do_mount(tmp_ch, buf, 0);
						add_follower(it->mob, tmp_ch);
						break;
					}
				}
			}

			struct follow_riding_data *toFree = it;
			it                                = it->next;
			FREE(toFree);
		}
	}

	// restore floor objects
	cJSON *objs = cJSON_GetObjectItem(root, "objs");
	if (objs && cJSON_IsArray(objs))
	{
		cJSON *obj_json;
		cJSON_ArrayForEach(obj_json, objs)
		{
			cJSON *uid_json = cJSON_GetObjectItem(obj_json, "uid");
			cJSON *v        = cJSON_GetObjectItem(obj_json, "v");
			cJSON *rm       = cJSON_GetObjectItem(obj_json, "rm");
			if (!v || !rm)
				continue;

			unsigned long uid = 0;
			if (uid_json && cJSON_IsNumber(uid_json))
				uid = (unsigned long)uid_json->valuedouble;

			// skip if picked up before crash
			if (uid > 0 && redis_check_floor_pickup(uid))
			{
				objs_skipped++;
				continue;
			}

			// skip if already restored from floor_drops (has fresher data)
			if (uid > 0 && redis_check_floor_drop(uid))
			{
				objs_skipped++;
				continue;
			}

			int vnum      = v->valueint;
			int room_vnum = rm->valueint;

			int rnum = real_room(room_vnum);
			if (rnum < 0 || rnum > top_of_world)
				continue;

			P_obj obj = read_object(vnum, VIRTUAL);
			if (!obj)
				continue;

			// restore uid
			if (uid > 0)
				obj->obj_uid = uid;

			// restore type
			cJSON *tp = cJSON_GetObjectItem(obj_json, "tp");
			if (tp && cJSON_IsNumber(tp))
				obj->type = tp->valueint;

			// restore values
			for (int i = 0; i < NUMB_OBJ_VALS; i++)
			{
				char key[4];
				snprintf(key, sizeof(key), "v%d", i);
				cJSON *val = cJSON_GetObjectItem(obj_json, key);
				if (val && cJSON_IsNumber(val))
					obj->value[i] = val->valueint;
			}

			// restore timers
			cJSON *tmr = cJSON_GetObjectItem(obj_json, "tmr");
			if (tmr && cJSON_IsArray(tmr))
			{
				int    idx = 0;
				cJSON *t;
				cJSON_ArrayForEach(t, tmr)
				{
					if (idx < 6 && cJSON_IsNumber(t))
						obj->timer[idx] = (time_t)t->valuedouble;
					idx++;
				}
			}

			// restore strings (for corpses etc) - must set str_mask to avoid leak
			cJSON *nm = cJSON_GetObjectItem(obj_json, "nm");
			cJSON *sd = cJSON_GetObjectItem(obj_json, "sd");
			cJSON *ld = cJSON_GetObjectItem(obj_json, "ld");
			if (nm && cJSON_IsString(nm) && nm->valuestring[0])
			{
				if ((obj->str_mask & STRUNG_KEYS) && obj->name)
					str_free(obj->name);
				obj->name = str_dup(nm->valuestring);
				obj->str_mask |= STRUNG_KEYS;
			}
			if (sd && cJSON_IsString(sd) && sd->valuestring[0])
			{
				if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
					str_free(obj->short_description);
				obj->short_description = str_dup(sd->valuestring);
				obj->str_mask |= STRUNG_DESC2;
			}
			if (ld && cJSON_IsString(ld) && ld->valuestring[0])
			{
				if ((obj->str_mask & STRUNG_DESC1) && obj->description)
					str_free(obj->description);
				obj->description = str_dup(ld->valuestring);
				obj->str_mask |= STRUNG_DESC1;
			}

			obj_to_room(obj, rnum);

			// restore contents (only if object is a container type)
			cJSON *con = cJSON_GetObjectItem(obj_json, "con");
			if (con && cJSON_IsArray(con) && (obj->type == ITEM_CONTAINER || obj->type == ITEM_QUIVER || obj->type == ITEM_STORAGE || obj->type == ITEM_CORPSE))
			{
				cJSON *cont_vnum;
				cJSON_ArrayForEach(cont_vnum, con)
				{
					if (!cJSON_IsNumber(cont_vnum))
						continue;
					int content_vnum = cont_vnum->valueint;
					if (content_vnum > 0)
					{
						P_obj content = read_object(content_vnum, VIRTUAL);
						if (content)
							obj_to_obj(content, obj);
					}
				}
			}

			objs_restored++;
		}
	}

	// clear pickup and drop logs after restore
	if (objs_skipped > 0)
		logit(LOG_SYS, "redis: skipped %d items (picked up or already restored)", objs_skipped);
	redis_clear_floor_pickups();
	redis_clear_floor_drops();

	// restore doors
	cJSON *doors = cJSON_GetObjectItem(root, "doors");
	if (doors && cJSON_IsArray(doors))
	{
		cJSON *door_json;
		cJSON_ArrayForEach(door_json, doors)
		{
			cJSON *rm = cJSON_GetObjectItem(door_json, "rm");
			cJSON *dr = cJSON_GetObjectItem(door_json, "dr");
			cJSON *st = cJSON_GetObjectItem(door_json, "st");
			if (!rm || !dr || !st)
				continue;

			int room_vnum = rm->valueint;
			int dir       = dr->valueint;
			int state     = st->valueint;

			int rnum = real_room(room_vnum);
			if (rnum >= 0 && rnum <= top_of_world && dir >= 0 && dir < NUM_EXITS && world[rnum].dir_option[dir])
			{
				world[rnum].dir_option[dir]->exit_info = state;
				doors_restored++;
			}
		}
	}

	// restore zones
	cJSON *zones = cJSON_GetObjectItem(root, "zones");
	if (zones && cJSON_IsArray(zones))
	{
		cJSON *zone_json;
		cJSON_ArrayForEach(zone_json, zones)
		{
			cJSON *rn  = cJSON_GetObjectItem(zone_json, "rn");
			cJSON *age = cJSON_GetObjectItem(zone_json, "age");
			cJSON *ls  = cJSON_GetObjectItem(zone_json, "ls");
			if (!rn || !age || !ls)
				continue;

			int zone_rnum = rn->valueint;
			if (zone_rnum >= 0 && zone_rnum <= top_of_zone_table)
			{
				zone_table[zone_rnum].age      = age->valueint;
				zone_table[zone_rnum].lifespan = ls->valueint;
				zones_restored++;
			}
		}
	}

	cJSON_Delete(root);

	logit(LOG_SYS, "redis: json world state restored (%d mobs, %d objs, %d doors, %d zones)", mobs_restored, objs_restored, doors_restored, zones_restored);

	return true;
#endif
}

bool redis_load_world_state(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_world_state_enabled)
		return false;

	if (!redis_ctx || redis_ctx->err)
	{
		if (!redis_reconnect())
			return false;
	}

	// restore floor drops first - has most recent data
	redis_restore_floor_drops();

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET mud:world_state");
	if (!reply || redis_ctx->err)
	{
		if (reply)
			freeReplyObject(reply);
		return false;
	}

	if (reply->type != REDIS_REPLY_STRING || !reply->str || reply->len == 0)
	{
		freeReplyObject(reply);
		return false;
	}

	const char *buffer = reply->str;
	size_t      len    = reply->len;

	// detect format: json starts with '{', binary starts with 'COPY'
	if (len > 0 && buffer[0] == '{')
	{
		// json format
		bool result = redis_load_world_state_json(buffer);
		freeReplyObject(reply);
		return result;
	}
	else if (len >= 4 && memcmp(buffer, COPYOVER_MAGIC, 4) == 0)
	{
		// old binary format - no longer supported
		logit(LOG_SYS, "redis: detected old binary world state format, clearing");
		freeReplyObject(reply);
		redis_clear_world_state();
		return false;
	}
	else
	{
		// unknown format
		logit(LOG_SYS, "redis: unknown world state format, clearing");
		freeReplyObject(reply);
		redis_clear_world_state();
		return false;
	}
#endif
}

void event_save_world_state(P_char ch, P_char victim, P_obj obj, void *data)
{
	if (redis_enabled && redis_world_state_enabled)
	{
		redis_flush_floor_drops();
		redis_save_world_state();
		// clear floor_drops - items now in world_state snapshot
		// this prevents purged/decayed items from coming back as zombies
		redis_clear_floor_drops();
		add_event(event_save_world_state, world_state_interval * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
	}
}

bool redis_cache_set(const char *key, const char *value)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_ctx || !key || !value)
		return false;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SET %s %b", key, value, strlen(value));
	if (!reply)
		return false;

	bool ok = (reply->type == REDIS_REPLY_STATUS);
	freeReplyObject(reply);
	return ok;
#endif
}

bool redis_cache_set_ex(const char *key, int seconds, const char *value)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_ctx || !key || !value || seconds <= 0)
		return false;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SETEX %s %d %b", key, seconds, value, strlen(value));
	if (!reply)
		return false;

	bool ok = (reply->type == REDIS_REPLY_STATUS);
	freeReplyObject(reply);
	return ok;
#endif
}

char *redis_cache_get(const char *key)
{
#ifdef __NO_MYSQL__
	return NULL;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return NULL;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET %s", key);
	if (!reply)
		return NULL;

	char *result = NULL;
	if (reply->type == REDIS_REPLY_STRING && reply->str)
		result = strdup(reply->str);

	freeReplyObject(reply);
	return result;
#endif
}

void redis_cache_del(const char *key)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx || !key)
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "DEL %s", key);
	if (reply)
		freeReplyObject(reply);
#endif
}

#ifndef __NO_MYSQL__
static void redis_ship_cache_key(char *buf, size_t buf_size, const char *owner_name)
{
	snprintf(buf, buf_size, "ship:snapshot:%s", owner_name ? owner_name : "");
}

static int redis_json_int(cJSON *object, const char *key, int fallback)
{
	if (!cJSON_IsObject(object))
		return fallback;
	cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
	if (!cJSON_IsNumber(item))
		return fallback;
	return item->valueint;
}

static double redis_json_double(cJSON *object, const char *key, double fallback)
{
	if (!cJSON_IsObject(object))
		return fallback;
	cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
	if (!cJSON_IsNumber(item))
		return fallback;
	return item->valuedouble;
}

static const char *redis_json_string(cJSON *object, const char *key)
{
	if (!cJSON_IsObject(object))
		return NULL;
	cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
	if (!cJSON_IsString(item) || !item->valuestring)
		return NULL;
	return item->valuestring;
}

static int redis_json_array_int(cJSON *array, int index, int fallback)
{
	if (!cJSON_IsArray(array))
		return fallback;
	cJSON *item = cJSON_GetArrayItem(array, index);
	if (!cJSON_IsNumber(item))
		return fallback;
	return item->valueint;
}

static cJSON *redis_ship_snapshot_to_json(const struct ShipData *ship)
{
	if (!ship)
		return NULL;

	cJSON *root = cJSON_CreateObject();
	if (!root)
		return NULL;

	cJSON_AddNumberToObject(root, "db_id", ship->db_id);
	cJSON_AddStringToObject(root, "ownername", ship->ownername ? ship->ownername : "");
	cJSON_AddStringToObject(root, "name", ship->name ? ship->name : "");
	cJSON_AddNumberToObject(root, "m_class", ship->m_class);
	cJSON_AddNumberToObject(root, "frags", ship->frags);
	cJSON_AddNumberToObject(root, "anchor", ship->anchor);
	cJSON_AddNumberToObject(root, "time", ship->time);
	cJSON_AddNumberToObject(root, "mainsail", ship->mainsail);
	cJSON_AddNumberToObject(root, "race", ship->race);
	cJSON_AddNumberToObject(root, "money", ship->money);
	char flags_buf[32];
	snprintf(flags_buf, sizeof(flags_buf), "%lu", ship->flags);
	cJSON_AddStringToObject(root, "flags", flags_buf);

	cJSON *armor = cJSON_CreateArray();
	cJSON *internal = cJSON_CreateArray();
	if (!armor || !internal)
	{
		if (armor)
			cJSON_Delete(armor);
		if (internal)
			cJSON_Delete(internal);
		cJSON_Delete(root);
		return NULL;
	}
	for (int i = 0; i < 4; i++)
	{
		cJSON *arc = cJSON_CreateArray();
		cJSON *hin = cJSON_CreateArray();
		if (!arc || !hin)
		{
			if (arc)
				cJSON_Delete(arc);
			if (hin)
				cJSON_Delete(hin);
			cJSON_Delete(armor);
			cJSON_Delete(internal);
			cJSON_Delete(root);
			return NULL;
		}
		cJSON_AddItemToArray(arc, cJSON_CreateNumber(ship->armor[i]));
		cJSON_AddItemToArray(arc, cJSON_CreateNumber(ship->maxarmor[i]));
		cJSON_AddItemToArray(hin, cJSON_CreateNumber(ship->internal[i]));
		cJSON_AddItemToArray(hin, cJSON_CreateNumber(ship->maxinternal[i]));
		cJSON_AddItemToArray(armor, arc);
		cJSON_AddItemToArray(internal, hin);
	}
	cJSON_AddItemToObject(root, "armor", armor);
	cJSON_AddItemToObject(root, "internal", internal);

	cJSON *crew = cJSON_CreateObject();
	if (!crew)
	{
		cJSON_Delete(root);
		return NULL;
	}
	cJSON_AddNumberToObject(crew, "index", ship->crew.index);
	cJSON_AddNumberToObject(crew, "sail_skill", ship->crew.sail_skill);
	cJSON_AddNumberToObject(crew, "guns_skill", ship->crew.guns_skill);
	cJSON_AddNumberToObject(crew, "rpar_skill", ship->crew.rpar_skill);
	cJSON_AddNumberToObject(crew, "sail_chief", ship->crew.sail_chief);
	cJSON_AddNumberToObject(crew, "guns_chief", ship->crew.guns_chief);
	cJSON_AddNumberToObject(crew, "rpar_chief", ship->crew.rpar_chief);
	cJSON_AddItemToObject(root, "crew", crew);

	cJSON *slots = cJSON_CreateArray();
	if (!slots)
	{
		cJSON_Delete(root);
		return NULL;
	}
	for (int i = 0; i < MAXSLOTS; i++)
	{
		cJSON *slot = cJSON_CreateObject();
		if (!slot)
		{
			cJSON_Delete(slots);
			cJSON_Delete(root);
			return NULL;
		}
		cJSON_AddNumberToObject(slot, "type", ship->slot[i].type);
		cJSON_AddNumberToObject(slot, "index", ship->slot[i].index);
		cJSON_AddNumberToObject(slot, "position", ship->slot[i].position);
		cJSON_AddNumberToObject(slot, "timer", ship->slot[i].timer);
		cJSON_AddNumberToObject(slot, "val0", ship->slot[i].val0);
		cJSON_AddNumberToObject(slot, "val1", ship->slot[i].val1);
		cJSON_AddNumberToObject(slot, "val2", ship->slot[i].val2);
		cJSON_AddNumberToObject(slot, "val3", ship->slot[i].val3);
		cJSON_AddNumberToObject(slot, "val4", ship->slot[i].val4);
		cJSON_AddItemToArray(slots, slot);
	}
	cJSON_AddItemToObject(root, "slots", slots);
	return root;
}

static bool redis_ship_snapshot_from_json(cJSON *root, struct ShipData *ship)
{
	if (!cJSON_IsObject(root) || !ship)
		return false;

	int ship_class = redis_json_int(root, "m_class", -1);
	if (ship_class < 0)
		return false;

	ship->db_id    = redis_json_int(root, "db_id", -1);
	ship->ownername = str_dup(redis_json_string(root, "ownername") ? redis_json_string(root, "ownername") : "");
	ship->name      = str_dup(redis_json_string(root, "name") ? redis_json_string(root, "name") : "");
	ship->m_class   = ship_class;
	ship->frags     = redis_json_int(root, "frags", 0);
	ship->anchor    = redis_json_int(root, "anchor", 0);
	ship->time      = redis_json_int(root, "time", 0);
	ship->mainsail  = redis_json_int(root, "mainsail", 0);
	ship->race      = redis_json_int(root, "race", 0);
	ship->money     = redis_json_int(root, "money", 0);
	ship->flags     = strtoul(redis_json_string(root, "flags") ? redis_json_string(root, "flags") : "0", NULL, 10);

	cJSON *armor = cJSON_GetObjectItemCaseSensitive(root, "armor");
	cJSON *internal = cJSON_GetObjectItemCaseSensitive(root, "internal");
	if (!cJSON_IsArray(armor) || !cJSON_IsArray(internal))
		return false;
	for (int i = 0; i < 4; i++)
	{
		cJSON *arc = cJSON_GetArrayItem(armor, i);
		cJSON *hin = cJSON_GetArrayItem(internal, i);
		if (!cJSON_IsArray(arc) || !cJSON_IsArray(hin))
			return false;
		ship->armor[i] = redis_json_array_int(arc, 0, ship->armor[i]);
		ship->maxarmor[i] = redis_json_array_int(arc, 1, ship->maxarmor[i]);
		ship->internal[i] = redis_json_array_int(hin, 0, ship->internal[i]);
		ship->maxinternal[i] = redis_json_array_int(hin, 1, ship->maxinternal[i]);
	}

	cJSON *crew = cJSON_GetObjectItemCaseSensitive(root, "crew");
	if (!cJSON_IsObject(crew))
		return false;
	ship->crew.index      = redis_json_int(crew, "index", ship->crew.index);
	ship->crew.sail_skill = (float)redis_json_double(crew, "sail_skill", ship->crew.sail_skill);
	ship->crew.guns_skill = (float)redis_json_double(crew, "guns_skill", ship->crew.guns_skill);
	ship->crew.rpar_skill = (float)redis_json_double(crew, "rpar_skill", ship->crew.rpar_skill);
	ship->crew.sail_chief = redis_json_int(crew, "sail_chief", ship->crew.sail_chief);
	ship->crew.guns_chief = redis_json_int(crew, "guns_chief", ship->crew.guns_chief);
	ship->crew.rpar_chief = redis_json_int(crew, "rpar_chief", ship->crew.rpar_chief);

	cJSON *slots = cJSON_GetObjectItemCaseSensitive(root, "slots");
	if (!cJSON_IsArray(slots))
		return false;
	for (int i = 0; i < MAXSLOTS; i++)
	{
		cJSON *slot = cJSON_GetArrayItem(slots, i);
		if (!cJSON_IsObject(slot))
			return false;
		ship->slot[i].type     = redis_json_int(slot, "type", ship->slot[i].type);
		ship->slot[i].index    = redis_json_int(slot, "index", ship->slot[i].index);
		ship->slot[i].position = redis_json_int(slot, "position", ship->slot[i].position);
		ship->slot[i].timer    = redis_json_int(slot, "timer", ship->slot[i].timer);
		ship->slot[i].val0     = redis_json_int(slot, "val0", ship->slot[i].val0);
		ship->slot[i].val1     = redis_json_int(slot, "val1", ship->slot[i].val1);
		ship->slot[i].val2     = redis_json_int(slot, "val2", ship->slot[i].val2);
		ship->slot[i].val3     = redis_json_int(slot, "val3", ship->slot[i].val3);
		ship->slot[i].val4     = redis_json_int(slot, "val4", ship->slot[i].val4);
	}

	return true;
}

bool redis_cache_ship_snapshot(struct ShipData *ship)
{
	if (_pwipe)
		return false;
	if (!ship || !ship->ownername)
		return false;

	char key[256];
	redis_ship_cache_key(key, sizeof(key), ship->ownername);
	cJSON *root = redis_ship_snapshot_to_json(ship);
	if (!root)
		return false;

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return false;

	bool ok = redis_cache_set(key, json);
	free(json);
	return ok;
}

struct ShipData *redis_load_ship_snapshot(const char *owner_name)
{
	if (!owner_name)
		return NULL;

	char key[256];
	redis_ship_cache_key(key, sizeof(key), owner_name);
	char *json = redis_cache_get(key);
	if (!json)
		return NULL;

	cJSON *root = cJSON_Parse(json);
	free(json);
	if (!root)
		return NULL;

	int ship_class = redis_json_int(root, "m_class", -1);
	if (ship_class < 0)
	{
		cJSON_Delete(root);
		return NULL;
	}

	struct ShipData *ship = new_ship(ship_class);
	if (!ship)
	{
		cJSON_Delete(root);
		return NULL;
	}

	if (!redis_ship_snapshot_from_json(root, ship))
	{
		cJSON_Delete(root);
		if (ship->ownername)
			FREE(ship->ownername);
		if (ship->name)
			FREE(ship->name);
		FREE(ship);
		return NULL;
	}

	ship->save_pending         = false;
	ship->save_retry_after     = 0;
	ship->save_saved_signature = ship_save_signature(ship);

	cJSON_Delete(root);
	return ship;
}

void redis_invalidate_ship_snapshot(const char *owner_name)
{
	if (!owner_name)
		return;

	char key[256];
	redis_ship_cache_key(key, sizeof(key), owner_name);
	redis_cache_del(key);
}

bool redis_clear_ship_snapshots(void)
{
	if (!redis_enabled || !redis_ctx)
		return true;

	char cursor[64] = "0";
	do
	{
		redisReply *scan = (redisReply *)redisCommand(redis_ctx,
				"SCAN %s MATCH ship:snapshot:* COUNT 256", cursor);
		if (!scan || scan->type != REDIS_REPLY_ARRAY || scan->elements != 2 ||
			!scan->element[0] || !scan->element[1] ||
			!scan->element[0]->str ||
			scan->element[0]->type != REDIS_REPLY_STRING ||
			scan->element[1]->type != REDIS_REPLY_ARRAY)
		{
			if (scan)
				freeReplyObject(scan);
			return false;
		}

		snprintf(cursor, sizeof(cursor), "%s", scan->element[0]->str);
		redisReply *keys = scan->element[1];
		for (size_t i = 0; i < keys->elements; i++)
		{
			redisReply *key = keys->element[i];
			if (!key || key->type != REDIS_REPLY_STRING || !key->str)
			{
				freeReplyObject(scan);
				return false;
			}
			redisReply *del = (redisReply *)redisCommand(redis_ctx, "DEL %b",
					key->str, key->len);
			if (!del || (del->type != REDIS_REPLY_INTEGER &&
						del->type != REDIS_REPLY_NIL))
			{
				if (del)
					freeReplyObject(del);
				freeReplyObject(scan);
				return false;
			}
			freeReplyObject(del);
		}
		freeReplyObject(scan);
	}
	while (strcmp(cursor, "0") != 0);

	return true;
}
#endif

bool redis_publish(const char *channel, const char *message)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_ctx || !channel || !message)
		return false;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "PUBLISH %s %b", channel, message, strlen(message));
	if (!reply)
		return false;

	bool ok = (reply->type == REDIS_REPLY_INTEGER);
	freeReplyObject(reply);
	return ok;
#endif
}

void redis_donation_subscribe_init(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	const char *redis_host = getenv("REDIS_HOST");
	if (!redis_host || !*redis_host)
		redis_host = "127.0.0.1";

	const char *redis_port_str = getenv("REDIS_PORT");
	int         redis_port     = 6379;
	if (redis_port_str && *redis_port_str)
	{
		redis_port = atoi(redis_port_str);
		if (redis_port <= 0 || redis_port > 65535)
			redis_port = 6379;
	}

	donation_sub_ctx = redisConnect(redis_host, redis_port);
	if (!donation_sub_ctx || donation_sub_ctx->err)
	{
		if (donation_sub_ctx)
		{
			redisFree(donation_sub_ctx);
			donation_sub_ctx = NULL;
		}
		logit(LOG_SYS, "redis: donation subscriber failed to connect");
		return;
	}

	struct timeval tv = {0, 100000};
	redisSetTimeout(donation_sub_ctx, tv);

	redisReply *reply = (redisReply *)redisCommand(donation_sub_ctx, "SUBSCRIBE mud:nchat");
	if (reply)
		freeReplyObject(reply);

	donation_sub_connected = true;
	logit(LOG_SYS, "redis: donation subscriber connected to mud:nchat");
#endif
}

static void broadcast_donation_nchat(const char *char_name, double amount, const char *currency, const char *message, bool is_public)
{
	char   buf[MAX_STRING_LENGTH];
	P_desc i;
	P_char to;

	if (is_public && char_name && *char_name)
	{
		if (message && *message)
			snprintf(buf, sizeof(buf), "&+Y%s&n&+m donated &+W$%.2f %s&n&+m: &+w'%s'&n\n", char_name, amount, currency, message);
		else
			snprintf(buf, sizeof(buf), "&+Y%s&n&+m donated &+W$%.2f %s&n&+m!&n\n", char_name, amount, currency);
	}
	else
	{
		if (message && *message)
			snprintf(buf, sizeof(buf), "&+Yan anonymous donor&n&+m gave &+W$%.2f %s&n&+m: &+w'%s'&n\n", amount, currency, message);
		else
			snprintf(buf, sizeof(buf), "&+Yan anonymous donor&n&+m gave &+W$%.2f %s&n&+m!&n\n", amount, currency);
	}

	for (i = descriptor_list; i; i = i->next)
	{
		if (i->connected || !(to = i->character))
			continue;
		if (IS_NPC(to) || !PLR2_FLAGGED(to, PLR2_NCHAT))
			continue;
		send_to_char(buf, to);
	}

	logit(LOG_SYS, "donation: %s donated $%.2f %s", (is_public && char_name) ? char_name : "anonymous", amount, currency);
}

void redis_check_donation_messages(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	// attempt reconnect if disconnected
	if (!donation_sub_connected || !donation_sub_ctx)
	{
		redis_donation_subscribe_init();
		if (!donation_sub_connected)
			return;
	}

	redisReply *reply = NULL;

	if (redisGetReply(donation_sub_ctx, (void **)&reply) != REDIS_OK)
	{
		if (donation_sub_ctx->err)
		{
			// timeout errors are normal when no message available - just ignore
			if (strstr(donation_sub_ctx->errstr, "Resource temporarily unavailable") || strstr(donation_sub_ctx->errstr, "timed out"))
			{
				donation_sub_ctx->err       = 0;
				donation_sub_ctx->errstr[0] = '\0';
				return;
			}
			logit(LOG_SYS, "redis: donation subscriber error: %s, will reconnect", donation_sub_ctx->errstr);
			donation_sub_connected = false;
			redisFree(donation_sub_ctx);
			donation_sub_ctx = NULL;
		}
		return;
	}

	if (!reply)
		return;

	if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3)
	{
		if (reply->element[0]->type == REDIS_REPLY_STRING && strcmp(reply->element[0]->str, "message") == 0 && reply->element[2]->type == REDIS_REPLY_STRING)
		{
			const char *payload = reply->element[2]->str;

			cJSON *json = cJSON_Parse(payload);
			if (json)
			{
				cJSON *type_obj = cJSON_GetObjectItem(json, "type");
				if (type_obj && cJSON_IsString(type_obj) && strcmp(type_obj->valuestring, "donation") == 0)
				{
					cJSON *char_name_obj = cJSON_GetObjectItem(json, "character_name");
					cJSON *amount_obj    = cJSON_GetObjectItem(json, "amount");
					cJSON *currency_obj  = cJSON_GetObjectItem(json, "currency");
					cJSON *message_obj   = cJSON_GetObjectItem(json, "message");
					cJSON *is_public_obj = cJSON_GetObjectItem(json, "is_public");

					const char *char_name = (char_name_obj && cJSON_IsString(char_name_obj)) ? char_name_obj->valuestring : NULL;
					double      amount    = (amount_obj && cJSON_IsNumber(amount_obj)) ? amount_obj->valuedouble : 0.0;
					const char *currency  = (currency_obj && cJSON_IsString(currency_obj)) ? currency_obj->valuestring : "USD";
					const char *message   = (message_obj && cJSON_IsString(message_obj)) ? message_obj->valuestring : NULL;
					bool        is_public = (is_public_obj && cJSON_IsBool(is_public_obj)) ? cJSON_IsTrue(is_public_obj) : false;

					broadcast_donation_nchat(char_name, amount, currency, message, is_public);
				}
				cJSON_Delete(json);
			}
		}
	}

	freeReplyObject(reply);
#endif
}

void event_check_donation_messages(P_char ch, P_char victim, P_obj obj, void *data)
{
	redis_check_donation_messages();

	if (redis_enabled)
		add_event(event_check_donation_messages, 1 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
}

// forward declare from random.mob.c
struct zone_random_data
{
	int zone;
	int races[10];
	int proc_spells[3][2];
};
extern struct zone_random_data zones_random_data[];
extern Skill                   skills[];

static char *generate_named_report(void)
{
	char *output = (char *)malloc(MAX_STRING_LENGTH * 4);
	if (!output)
		return NULL;

	output[0] = '\0';
	char buffer[MAX_STRING_LENGTH];

	strcat(output, "&+YCurrent listing of spells granted by named sets by zone.&n\n");
	strcat(output, "&+Y-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&n\n\n");
	strcat(output, "  &+MNotes&n: &+W*&n if a zone isn't listed, sets still grant hitpoints\n");
	strcat(output, "         &+W*&n caster level of the spell(s) is based on number of items\n");
	strcat(output, "           going over set requirements will increase caster level\n");
	strcat(output, "         &+W*&n &+Gthese&n spells have a cooldown of 1 minute\n");
	strcat(output, "           &+ythese&n spells have a cooldown of 5 minutes\n\n");
	strcat(output, "&+Y ZONE NAME                                        &+W|&+B SPELLS GRANTED &+W(&+Ypieces required&n&+W)&n\n");
	strcat(output, "&+W-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&n\n");

	for (int i = 0; zones_random_data[i].zone; i++)
	{
		int zone_id = real_zone(zones_random_data[i].zone);
		if (zone_id <= 0)
			continue;

		const char *zone_name = zone_table[zone_id].name;
		snprintf(buffer, sizeof(buffer), " %s    &+W|&n ", pad_ansi(zone_name, 45, FALSE).c_str());

		if (zones_random_data[i].proc_spells[0][0] == 0)
		{
			strcat(buffer, "&+LNONE&n");
			strcat(buffer, "\n");
			strcat(output, buffer);
			continue;
		}

		for (int x = 0; x < 3; x++)
		{
			if (zones_random_data[i].proc_spells[x][0] != 0 && zones_random_data[i].proc_spells[x][1] <= MAX_AFFECT_TYPES)
			{
				char        buf[256];
				const char *spellColor = "&+B";

				if (zones_random_data[i].proc_spells[x][1] == SPELL_STONE_SKIN || zones_random_data[i].proc_spells[x][1] == SPELL_INVIGORATE)
					spellColor = "&+G";
				else if (zones_random_data[i].proc_spells[x][1] == SPELL_CONJURE_ELEMENTAL)
					spellColor = "&+y";

				snprintf(buf, sizeof(buf), "%s%s%s &+W(&+Y%d&+W)&n", x != 0 ? "&+W,&n " : "", spellColor, skills[zones_random_data[i].proc_spells[x][1]].name, zones_random_data[i].proc_spells[x][0]);
				strcat(buffer, buf);
			}
		}
		strcat(buffer, "\n");
		strcat(output, buffer);
	}

	return output;
}

void redis_cache_named_report(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	char *report = generate_named_report();
	if (report)
	{
		redis_cache_set("mud:cache:named", report);
		free(report);
		logit(LOG_SYS, "redis: cached named report");
	}
#endif
}

char *redis_get_named_report(void) { return redis_cache_get("mud:cache:named"); }

// fraglist cache
extern void                 get_level_cap_info(long *max_frags, int *racewar, int *level, time_t *next_update);
extern int                  sql_level_cap(int racewar_side);
extern const racewar_struct racewar_color[];

#define MAX_FRAG_SIZE 10

static char *generate_fraglist_output(void)
{
#ifdef __NO_MYSQL__
	return NULL;
#else
	char *output = (char *)malloc(65536);
	if (!output)
		return NULL;

	output[0] = '\0';
	char       buf[2048], name[256];
	int        frags, count;
	float      fragnum;
	int        cap_level, cap_racewar, cap_others;
	long       cap_frags;
	time_t     cap_timer;
	int        days, hours, mins, secs;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	get_level_cap_info(&cap_frags, &cap_racewar, &cap_level, &cap_timer);
	cap_others = sql_level_cap((cap_racewar == RACEWAR_GOOD) ? RACEWAR_EVIL : RACEWAR_GOOD);
	cap_timer -= get_time();

	if (cap_timer <= 0)
	{
		secs = mins = hours = days = 0;
	}
	else
	{
		secs = cap_timer % 60;
		cap_timer /= 60;
		mins = cap_timer % 60;
		cap_timer /= 60;
		hours = cap_timer % 24;
		cap_timer /= 24;
		days = cap_timer;
	}

	snprintf(output,
	         65536,
	         "&+YFrag Level Cap:&+w %d - &+%c%s&n, &+w%d&N - Others, &+YTop Frag Amount: &+w%d.%02d\n"
	         "&+YTimer:&+w %02d:%02d:%02d:%02d &+YFrags needed:&+w %.2f&n\n\n&+WTop Fraggers\n\n",
	         cap_level,
	         racewar_color[cap_racewar].color,
	         racewar_color[cap_racewar].name,
	         cap_others,
	         (int)(cap_frags / 100),
	         (int)(cap_frags % 100),
	         days,
	         hours,
	         mins,
	         secs,
	         frag_cap_config_frags_for_level(cap_level + 1));

	// query top fraggers (no filter)
	res = db_query("SELECT char_name, total_frags FROM frag_leaderboard "
	               "WHERE deleted_at IS NULL ORDER BY total_frags DESC LIMIT %d",
	               MAX_FRAG_SIZE);
	if (res)
	{
		count = 0;
		while ((row = mysql_fetch_row(res)) && count < MAX_FRAG_SIZE)
		{
			if (row[0] && row[1])
			{
				strlcpy(name, row[0], sizeof name);
				name[0]                = toupper(name[0]);
				frags                  = atoi(row[1]);
				fragnum                = frags / 100.0;
				snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
				strcat(output, buf);
				count++;
			}
		}
		mysql_free_result(res);

		while (count < MAX_FRAG_SIZE)
		{
			snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n", "Nobody", 0.0);
			strcat(output, buf);
			count++;
		}
	}

	strcat(output, "\r\n\r\n&+LLowest Fraggers\r\n\r\n");

	// query lowest fraggers
	res = db_query("SELECT char_name, total_frags FROM frag_leaderboard "
	               "WHERE deleted_at IS NULL ORDER BY total_frags ASC LIMIT %d",
	               MAX_FRAG_SIZE);
	if (res)
	{
		count = 0;
		while ((row = mysql_fetch_row(res)) && count < MAX_FRAG_SIZE)
		{
			if (row[0] && row[1])
			{
				strlcpy(name, row[0], sizeof name);
				name[0]                = toupper(name[0]);
				frags                  = atoi(row[1]);
				fragnum                = frags / 100.0;
				snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
				strcat(output, buf);
				count++;
			}
		}
		mysql_free_result(res);

		while (count < MAX_FRAG_SIZE)
		{
			snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n", "Nobody", 0.0);
			strcat(output, buf);
			count++;
		}
	}

	strcat(output, "\r\n");
	return output;
#endif
}

void redis_cache_fraglist(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	char *output = generate_fraglist_output();
	if (output)
	{
		redis_cache_set("mud:cache:fraglist", output);
		free(output);
		logit(LOG_SYS, "redis: cached fraglist");
	}
#endif
}

char *redis_get_fraglist(void) { return redis_cache_get("mud:cache:fraglist"); }

void redis_invalidate_fraglist(void) { redis_cache_del("mud:cache:fraglist"); }

// epic zones cache - 15 min ttl for alignment display
#define EPIC_ZONES_CACHE_TTL 900

void redis_cache_epic_zones(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	char *output = generate_epic_zones_output();
	if (output)
	{
		redis_cache_set_ex("mud:cache:epic_zones", EPIC_ZONES_CACHE_TTL, output);
		free(output);
		logit(LOG_SYS, "redis: cached epic zones");
	}
#endif
}

char *redis_get_epic_zones(void) { return redis_cache_get("mud:cache:epic_zones"); }

void redis_invalidate_epic_zones(void) { redis_cache_del("mud:cache:epic_zones"); }

static void redis_publish_player_event(int pid, const char *event)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx || pid <= 0 || !event)
		return;

	char json[128];
	snprintf(json, sizeof(json), "{\"event\":\"%s\",\"pid\":%d}", event, pid);

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "PUBLISH mud:player %b", json, strlen(json));
	if (reply)
		freeReplyObject(reply);
#endif
}

// online players list for web
void redis_player_online(P_char ch)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx || !ch || IS_NPC(ch))
		return;

	const char *account    = get_account_name_safe(ch);
	const char *race_str   = race_names_table[GET_RACE(ch)].ansi;
	const char *class_str  = get_class_name(ch, NULL);
	const char *ip         = (ch->desc && ch->desc->host[0]) ? ch->desc->host : "";
	const char *client     = (ch->desc && ch->desc->client_name[0]) ? ch->desc->client_name : "";
	const char *client_ver = (ch->desc && ch->desc->client_version[0]) ? ch->desc->client_version : "";
	time_t      login_time = ch->player.time.logon;

	char json[1024];
	snprintf(json,
	         sizeof(json),
	         "{\"name\":\"%s\",\"account\":\"%s\",\"level\":%d,\"race\":\"%s\",\"class\":\"%s\","
	         "\"racewar\":%d,\"ip\":\"%s\",\"client\":\"%s\",\"client_version\":\"%s\",\"login_time\":%ld}",
	         GET_NAME(ch),
	         account ? account : "",
	         GET_LEVEL(ch),
	         race_str ? race_str : "",
	         class_str ? class_str : "",
	         GET_RACEWAR(ch),
	         ip,
	         client,
	         client_ver,
	         (long)login_time);

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "HSET mud:online %d %b", GET_PID(ch), json, strlen(json));
	if (reply)
		freeReplyObject(reply);

	redis_publish_player_event(GET_PID(ch), "login");
#endif
}

void redis_player_offline(P_char ch)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx || !ch || IS_NPC(ch))
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "HDEL mud:online %d", GET_PID(ch));
	if (reply)
		freeReplyObject(reply);

	redis_publish_player_event(GET_PID(ch), "logout");
#endif
}

void redis_clear_online_players(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "DEL mud:online");
	if (reply)
		freeReplyObject(reply);
#endif
}

// arti cache
static const char *get_artifact_cache_key(int type, bool godlist)
{
	static char key[64];
	snprintf(key, sizeof(key), "mud:cache:artifact:%d:%d", type, godlist ? 1 : 0);
	return key;
}

void redis_cache_artifact_list(int type, bool godlist, const char *json)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || type < 1 || type > 3 || !json)
		return;
	redis_cache_set(get_artifact_cache_key(type, godlist), json);
#endif
}

char *redis_get_artifact_list(int type, bool godlist) { return redis_cache_get(get_artifact_cache_key(type, godlist)); }

void redis_invalidate_artifact_cache(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	for (int t = 1; t <= 3; t++)
	{
		redis_cache_del(get_artifact_cache_key(t, false));
		redis_cache_del(get_artifact_cache_key(t, true));
	}
#endif
}

// generic helpers for wiz command

bool redis_key_exists(const char *key)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return false;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "EXISTS %s", key);
	if (!reply)
		return false;

	bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
	freeReplyObject(reply);
	return exists;
#endif
}

long redis_get_ttl(const char *key)
{
#ifdef __NO_MYSQL__
	return -1;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return -1;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "TTL %s", key);
	if (!reply)
		return -1;

	long ttl = -1;
	if (reply->type == REDIS_REPLY_INTEGER)
		ttl = (long)reply->integer;

	freeReplyObject(reply);
	return ttl;
#endif
}

long redis_hlen(const char *key)
{
#ifdef __NO_MYSQL__
	return 0;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return 0;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "HLEN %s", key);
	if (!reply)
		return 0;

	long len = 0;
	if (reply->type == REDIS_REPLY_INTEGER)
		len = (long)reply->integer;

	freeReplyObject(reply);
	return len;
#endif
}

long redis_scard(const char *key)
{
#ifdef __NO_MYSQL__
	return 0;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return 0;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SCARD %s", key);
	if (!reply)
		return 0;

	long card = 0;
	if (reply->type == REDIS_REPLY_INTEGER)
		card = (long)reply->integer;

	freeReplyObject(reply);
	return card;
#endif
}

char *redis_get_string(const char *key)
{
#ifdef __NO_MYSQL__
	return NULL;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return NULL;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET %s", key);
	if (!reply)
		return NULL;

	char *result = NULL;
	if (reply->type == REDIS_REPLY_STRING && reply->str)
		result = strdup(reply->str);

	freeReplyObject(reply);
	return result;
#endif
}

void redis_clear_dirty_players(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	redisReply *reply = (redisReply *)redisCommand(redis_ctx, "DEL mud:dirty_players");
	if (reply)
		freeReplyObject(reply);

	redisReply *inflight = (redisReply *)redisCommand(redis_ctx, "DEL mud:dirty_players:flushing");
	if (inflight)
		freeReplyObject(inflight);

#endif
}
