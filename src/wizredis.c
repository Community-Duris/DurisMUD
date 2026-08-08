// wiz command for redis status and cache management

#include "prototypes.h"
#include "structs.h"
#include "interp.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "redis.h"

// helper to format time ago
static void format_time_ago(time_t ts, char *buf, size_t len)
{
	if (ts == 0)
	{
		snprintf(buf, len, "never");
		return;
	}

	time_t now  = get_time();
	long   diff = now - ts;

	if (diff < 0)
		diff = 0;

	if (diff < 60)
		snprintf(buf, len, "%ld sec ago", diff);
	else if (diff < 3600)
		snprintf(buf, len, "%ld min ago", diff / 60);
	else if (diff < 86400)
		snprintf(buf, len, "%ld hr ago", diff / 3600);
	else
		snprintf(buf, len, "%ld day ago", diff / 86400);
}

// helper to format ttl remaining
static void format_ttl(long ttl, char *buf, size_t len)
{
	if (ttl <= 0)
	{
		snprintf(buf, len, "expired");
		return;
	}

	long mins = ttl / 60;
	long secs = ttl % 60;
	snprintf(buf, len, "%ld:%02ld", mins, secs);
}

static void redis_status_simple(P_char ch)
{
	char buf[MAX_STRING_LENGTH];
	char time_buf[64];
	int  pos = 0;

	pos += snprintf(buf + pos, sizeof(buf) - pos, "&+gRedis Status&n\r\n");

	// world state
	time_t ws_ts     = redis_world_state_timestamp();
	char  *valid_str = redis_cache_get("mud:world_state:valid");
	bool   is_valid  = (valid_str && strcmp(valid_str, "1") == 0);
	if (valid_str)
		free(valid_str);

	format_time_ago(ws_ts, time_buf, sizeof(time_buf));
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cworld_state&n      %s%-5s&n    %s\r\n", is_valid ? "&+G" : "&+R", is_valid ? "VALID" : "NONE", ws_ts > 0 ? time_buf : "");

	// dirty players
	int dirty = get_dirty_player_count();
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cdirty_players&n    &+Y%-5d&n    pending saves\r\n", dirty);

	// floor drops
	long floor_count = redis_hlen("mud:floor_drops");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cfloor_drops&n      &+Y%-5ld&n    objects\r\n", floor_count);

	pos += snprintf(buf + pos, sizeof(buf) - pos, "\r\n&+gCaches&n\r\n");

	// artifacts - just check if any exist
	bool arti_cached = redis_key_exists("mud:cache:artifact:1:0") || redis_key_exists("mud:cache:artifact:2:0") || redis_key_exists("mud:cache:artifact:3:0");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cartifacts&n        %s%s&n\r\n", arti_cached ? "&+G" : "&+R", arti_cached ? "CACHED" : "CLEAR");

	// fraglist
	bool frag_cached = redis_key_exists("mud:cache:fraglist");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cfraglist&n         %s%s&n\r\n", frag_cached ? "&+G" : "&+R", frag_cached ? "CACHED" : "CLEAR");

	// epic zones
	bool epic_cached = redis_key_exists("mud:cache:epic_zones");
	long epic_ttl    = redis_get_ttl("mud:cache:epic_zones");
	if (epic_cached && epic_ttl > 0)
	{
		format_ttl(epic_ttl, time_buf, sizeof(time_buf));
		pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cepic_zones&n       &+GCACHED&n   expires %s\r\n", time_buf);
	}
	else
	{
		pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cepic_zones&n       %s%s&n\r\n", epic_cached ? "&+G" : "&+R", epic_cached ? "CACHED" : "CLEAR");
	}

	// named
	bool named_cached = redis_key_exists("mud:cache:named");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cnamed&n            %s%s&n\r\n", named_cached ? "&+G" : "&+R", named_cached ? "CACHED" : "CLEAR");

	send_to_char(buf, ch);
}

static void redis_status_detailed(P_char ch)
{
	char buf[MAX_STRING_LENGTH * 2];
	char time_buf[64];
	int  pos = 0;

	pos += snprintf(buf + pos, sizeof(buf) - pos, "&+gRedis Status (detailed)&n\r\n\r\n");

	pos += snprintf(buf + pos, sizeof(buf) - pos, "&+g[World Recovery]&n\r\n");

	// world state with full timestamp
	time_t ws_ts     = redis_world_state_timestamp();
	char  *valid_str = redis_cache_get("mud:world_state:valid");
	bool   is_valid  = (valid_str && strcmp(valid_str, "1") == 0);
	if (valid_str)
		free(valid_str);

	if (ws_ts > 0)
	{
		struct tm *tm_info = localtime(&ws_ts);
		char       date_buf[64];
		strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", tm_info);
		format_time_ago(ws_ts, time_buf, sizeof(time_buf));
		pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cworld_state&n      %s%-5s&n    %s (%s)\r\n", is_valid ? "&+G" : "&+R", is_valid ? "VALID" : "NONE", date_buf, time_buf);
	}
	else
	{
		pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cworld_state&n      &+RNONE&n\r\n");
	}

	// floor drops
	long floor_drops = redis_hlen("mud:floor_drops");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cfloor_drops&n      &+Y%-5ld&n    objects tracked\r\n", floor_drops);

	// floor pickups
	long floor_pickups = redis_scard("mud:floor_pickups");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cfloor_pickups&n    &+Y%-5ld&n    uids in dedup set\r\n", floor_pickups);

	// dirty players
	int dirty = get_dirty_player_count();
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cdirty_players&n    &+Y%-5d&n    pending async saves\r\n", dirty);

	// obj uid counter
	char *uid_str = redis_cache_get("mud:next_obj_uid");
	if (uid_str)
	{
		pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cnext_obj_uid&n     &+Y%s&n\r\n", uid_str);
		free(uid_str);
	}

	pos += snprintf(buf + pos, sizeof(buf) - pos, "\r\n&+g[Content Caches]&n\r\n");

	// artifacts - show all 6 variants
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cartifacts&n\r\n");

	const char *arti_names[] = {"major", "unique", "ioun"};
	for (int t = 1; t <= 3; t++)
	{
		for (int g = 0; g <= 1; g++)
		{
			char key[64];
			snprintf(key, sizeof(key), "mud:cache:artifact:%d:%d", t, g);
			bool cached = redis_key_exists(key);
			pos += snprintf(buf + pos, sizeof(buf) - pos, "    %s (%s)    %s%s&n\r\n", arti_names[t - 1], g ? "god" : "player", cached ? "&+G" : "&+R", cached ? "CACHED" : "CLEAR");
		}
	}

	// fraglist
	bool frag_cached = redis_key_exists("mud:cache:fraglist");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cfraglist&n         %s%s&n\r\n", frag_cached ? "&+G" : "&+R", frag_cached ? "CACHED" : "CLEAR");

	// epic zones with ttl
	bool epic_cached = redis_key_exists("mud:cache:epic_zones");
	long epic_ttl    = redis_get_ttl("mud:cache:epic_zones");
	if (epic_cached && epic_ttl > 0)
	{
		format_ttl(epic_ttl, time_buf, sizeof(time_buf));
		pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cepic_zones&n       &+GCACHED&n   ttl %s\r\n", time_buf);
	}
	else
	{
		pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cepic_zones&n       %s%s&n\r\n", epic_cached ? "&+G" : "&+R", epic_cached ? "CACHED" : "CLEAR");
	}

	// named
	bool named_cached = redis_key_exists("mud:cache:named");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cnamed&n            %s%s&n\r\n", named_cached ? "&+G" : "&+R", named_cached ? "CACHED" : "CLEAR");

	send_to_char(buf, ch);
}

static void redis_clear_cache(P_char ch, const char *cache, bool force)
{
	char buf[MAX_STRING_LENGTH];

	if (!*cache)
	{
		send_to_char("Usage: redis clear <cache> [force]\r\n", ch);
		send_to_char("&+cValid:&n world, floor, dirty, artifacts, fraglist, epic, named, all\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "world"))
	{
		redis_clear_world_state();
		send_to_char("&+GCleared:&n world_state\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "floor"))
	{
		redis_clear_floor_drops();
		redis_clear_floor_pickups();
		send_to_char("&+GCleared:&n floor_drops, floor_pickups\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "dirty"))
	{
		int count = get_dirty_player_count();
		if (count > 0 && !force)
		{
			snprintf(buf, sizeof(buf), "&+RCannot clear:&n %d players pending save. Use 'redis clear dirty force' to discard.\r\n", count);
			send_to_char(buf, ch);
			return;
		}
		redis_clear_dirty_players();
		if (count > 0)
			snprintf(buf, sizeof(buf), "&+YForce cleared:&n dirty_players (%d pending saves discarded)\r\n", count);
		else
			snprintf(buf, sizeof(buf), "&+GCleared:&n dirty_players\r\n");
		send_to_char(buf, ch);
		return;
	}

	if (is_abbrev(cache, "artifacts"))
	{
		redis_invalidate_artifact_cache();
		send_to_char("&+GCleared:&n artifacts (6 variants)\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "fraglist"))
	{
		redis_invalidate_fraglist();
		send_to_char("&+GCleared:&n fraglist\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "epic"))
	{
		redis_invalidate_epic_zones();
		send_to_char("&+GCleared:&n epic_zones\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "named"))
	{
		redis_cache_del("mud:cache:named");
		send_to_char("&+GCleared:&n named\r\n", ch);
		return;
	}

	snprintf(buf, sizeof(buf), "&+RUnknown cache:&n %s\r\n", cache);
	send_to_char(buf, ch);
	send_to_char("&+cValid:&n world, floor, dirty, artifacts, fraglist, epic, named, all\r\n", ch);
}

static void redis_clear_all(P_char ch, bool force, bool confirmed)
{
	char buf[MAX_STRING_LENGTH];

	if (!confirmed)
	{
		if (force)
			send_to_char("&+RUse 'redis clear all force confirm' to clear all caches including pending saves.&n\r\n", ch);
		else
			send_to_char("&+RUse 'redis clear all confirm' to clear all caches.&n\r\n", ch);
		return;
	}

	int dirty = get_dirty_player_count();

	// world recovery
	redis_clear_world_state();
	redis_clear_floor_drops();
	redis_clear_floor_pickups();

	// dirty players - only if force or empty
	if (force || dirty == 0)
	{
		redis_clear_dirty_players();
		send_to_char("&+GCleared:&n world_state, floor_drops, floor_pickups, dirty_players\r\n", ch);
	}
	else
	{
		snprintf(buf,
		         sizeof(buf),
		         "&+GCleared:&n world_state, floor_drops, floor_pickups\r\n"
		         "&+YSkipped:&n dirty_players (%d pending) - use 'force' to include\r\n",
		         dirty);
		send_to_char(buf, ch);
	}

	// content caches
	redis_invalidate_artifact_cache();
	redis_invalidate_fraglist();
	redis_invalidate_epic_zones();
	redis_cache_del("mud:cache:named");

	send_to_char("&+GCleared:&n artifacts (6), fraglist, epic_zones, named\r\n", ch);
}

void do_redis(P_char ch, char *argument, int cmd)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char arg4[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;

	if (!redis_enabled)
	{
		send_to_char("Redis is not enabled.\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);
	argument = one_argument(argument, arg4);

	// no args - simple status
	if (!*arg1)
	{
		redis_status_simple(ch);
		return;
	}

	// detailed
	if (is_abbrev(arg1, "detailed"))
	{
		redis_status_detailed(ch);
		return;
	}

	// clear
	if (is_abbrev(arg1, "clear"))
	{
		if (!*arg2)
		{
			send_to_char("Usage: redis clear <cache> [force]\r\n", ch);
			send_to_char("&+cValid:&n world, floor, dirty, artifacts, fraglist, epic, named, all\r\n", ch);
			return;
		}

		// all needs special handling
		if (is_abbrev(arg2, "all"))
		{
			bool force     = false;
			bool confirmed = false;

			// check for force and confirm in any order
			if (is_abbrev(arg3, "force"))
			{
				force = true;
				if (is_abbrev(arg4, "confirm"))
					confirmed = true;
			}
			else if (is_abbrev(arg3, "confirm"))
			{
				confirmed = true;
				if (is_abbrev(arg4, "force"))
					force = true;
			}

			redis_clear_all(ch, force, confirmed);
			return;
		}

		// single cache clear
		bool force = is_abbrev(arg3, "force");
		redis_clear_cache(ch, arg2, force);
		return;
	}

	// unknown
	send_to_char("Usage: redis [detailed | clear <cache>]\r\n", ch);
}
