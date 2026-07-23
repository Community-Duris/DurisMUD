/*
 * gmcp.c - gmcp protocol for durismud
 *
 * sends game data to clients via telnet or websocket.
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "gmcp.h"
#include <cjson/cJSON.h>
#include <math.h>
#include <mysql.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "handler.h"
#include "json_utils.h"
#include "map.h"
#include "ships/ships.h"
#include "spells.h"
#include "sql.h"
#include "websocket.h"
#include "ws_auth.h"

extern const int top_of_world;

/* externs */
extern struct room_data        *world;
extern struct zone_data        *zone_table;
extern const char              *pc_class_types[];
extern struct descriptor_data  *descriptor_list;
extern const struct class_names class_names_table[];
extern const struct race_names  race_names_table[];

/* send gmcp negotiation request to client */
void gmcp_negotiate(struct descriptor_data *d)
{
	unsigned char negotiate[3];

	if (!d || d->websocket)
		return; /* WebSocket clients don't need telnet negotiation */

	negotiate[0] = TELNET_IAC;
	negotiate[1] = TELNET_WILL;
	negotiate[2] = TELOPT_GMCP;

	write_to_descriptor_binary(d, negotiate, 3);
}

/* handle gmcp negotiation response */
void gmcp_handle_negotiation(struct descriptor_data *d, int cmd)
{
	if (!d)
		return;

	if (cmd == TELNET_DO)
	{
		d->gmcp_enabled = 1;
		statuslog(56, "GMCP enabled for %s", d->host);
	}
	else if (cmd == TELNET_DONT)
	{
		d->gmcp_enabled = 0;
	}
}

static int gmcp_durisweb_auth_limited(struct descriptor_data *d)
{
	time_t now = time(NULL);

	if (!d->durisweb_auth_window_start ||
	    (now >= d->durisweb_auth_window_start && now - d->durisweb_auth_window_start >= WS_AUTH_FAILURE_WINDOW))
	{
		d->durisweb_auth_window_start = now;
		d->durisweb_auth_failures = 0;
	}
	return d->durisweb_auth_failures >= WS_AUTH_MAX_FAILURES;
}

static void gmcp_durisweb_auth_failure(struct descriptor_data *d)
{
	(void)gmcp_durisweb_auth_limited(d);
	if (d->durisweb_auth_failures < WS_AUTH_MAX_FAILURES)
		d->durisweb_auth_failures++;
	if (d->durisweb_auth_failures >= WS_AUTH_MAX_FAILURES)
		STATE(d) = CON_EXIT;
}

/* handle incoming gmcp data from telnet client */
void gmcp_handle_input(struct descriptor_data *d, const char *data, size_t len)
{
	if (!d || !data || len == 0 || len > GMCP_MAX_INPUT_SIZE)
		return;

	if (len > 10 && strncmp(data, "Core.Hello", 10) == 0 && data[10] == ' ')
	{
		const char *end = data + len;
		const char *json_start = data + 10;
		while (json_start < end && *json_start == ' ')
			json_start++;

		if (json_start < end && *json_start == '{')
		{
			cJSON *root = cJSON_ParseWithLength(json_start, (size_t)(end - json_start));
			if (root)
			{
				cJSON *client  = cJSON_GetObjectItem(root, "client");
				cJSON *version = cJSON_GetObjectItem(root, "version");
				cJSON *sig     = cJSON_GetObjectItem(root, "sig");

				if (client && cJSON_IsString(client) && client->valuestring)
					strlcpy(d->client_name, client->valuestring, sizeof d->client_name);
				if (version && cJSON_IsString(version) && version->valuestring)
				{
					strncpy(d->client_version, version->valuestring, sizeof(d->client_version) - 1);
					d->client_version[sizeof(d->client_version) - 1] = '\0';
				}
				if (sig && cJSON_IsString(sig))
				{
					if (d->account || d->character || d->connected == CON_PLAYING || d->durisweb_verified || d->durisweb_backend || gmcp_durisweb_auth_limited(d))
					{
						/* Player and service identities cannot be mixed. */
					}
					else if (ws_verify_durisweb_signature(sig->valuestring))
					{
						d->durisweb_verified = 1;
						d->durisweb_backend  = 1;
						d->durisweb_auth_window_start = 0;
						d->durisweb_auth_failures = 0;
					}
					else
					{
						gmcp_durisweb_auth_failure(d);
					}
				}
				cJSON_Delete(root);
			}
		}
	}
	/* Client.Info is an alias some clients use */
	else if (len > 11 && strncmp(data, "Client.Info", 11) == 0 && data[11] == ' ')
	{
		const char *end = data + len;
		const char *json_start = data + 11;
		while (json_start < end && *json_start == ' ')
			json_start++;

		if (json_start < end && *json_start == '{')
		{
			cJSON *root = cJSON_ParseWithLength(json_start, (size_t)(end - json_start));
			if (root)
			{
				cJSON *client  = cJSON_GetObjectItem(root, "client");
				cJSON *version = cJSON_GetObjectItem(root, "version");

				if (client && cJSON_IsString(client) && client->valuestring)
					strlcpy(d->client_name, client->valuestring, sizeof d->client_name);
				if (version && cJSON_IsString(version) && version->valuestring)
					strlcpy(d->client_version, version->valuestring, sizeof d->client_version);
				cJSON_Delete(root);
			}
		}
	}
}

/* core gmcp send - handles telnet and websocket */
void gmcp_send(struct descriptor_data *d, const char *package, const char *json)
{
	if (!d || !package || !json)
		return;
	if (!d->gmcp_enabled && !d->websocket)
		return;

	/* Check descriptor is still valid (not a dangling pointer to closed socket) */
	if (!is_desc_valid(d))
		return;
	if (d->write_failed)
		return;

	if (d->websocket)
	{
		/* WebSocket: Send JSON message */
		websocket_send_json(d, "gmcp", package, json);
	}
	else
	{
		/* Telnet: Send IAC SB GMCP <package> <json> IAC SE */
		size_t         pkg_len   = strlen(package);
		size_t         json_len  = strlen(json);
		size_t         total_len = 3 + pkg_len + 1 + json_len + 2; /* IAC SB GMCP + package + space + json + IAC SE */
		unsigned char *buf       = (unsigned char *)malloc(total_len);

		if (!buf)
			return;

		buf[0] = TELNET_IAC;
		buf[1] = TELNET_SB;
		buf[2] = TELOPT_GMCP;
		memcpy(buf + 3, package, pkg_len);
		buf[3 + pkg_len] = ' ';
		memcpy(buf + 4 + pkg_len, json, json_len);
		buf[total_len - 2] = TELNET_IAC;
		buf[total_len - 1] = TELNET_SE;

		/* Use binary write to handle MCCP compression properly */
		write_to_descriptor_binary(d, buf, total_len);
		free(buf);
	}
}

/* send room.info when character enters a room */
void gmcp_room_info(struct char_data *ch)
{
	char             *json;
	struct room_data *room;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	room = &world[ch->in_room];
	json = json_build_room_info(room, ch);

	if (json)
	{
		gmcp_send(ch->desc, GMCP_PKG_ROOM_INFO, json);
		free(json);
	}
}

/*
 * send room.map for wilderness zones (surface, underdark, alatorin, newbie maps).
 * this stub is kept for backwards compatibility - the actual gmcp room.map is
 * sent from display_map_room() in map.c using gmcp_send_room_map().
 */
void gmcp_room_map(struct char_data *ch) { (void)ch; /* Suppress unused parameter warning */ }

/* send pre-generated map buffer via gmcp, called from display_map_room() */
void gmcp_send_room_map(struct char_data *ch, const char *map_buf)
{
	char  *json_str;
	cJSON *json;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;
	if (!map_buf || !*map_buf)
		return;

	json = cJSON_CreateObject();
	if (!json)
		return;

	cJSON_AddStringToObject(json, "map", map_buf);

	json_str = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);

	if (json_str)
	{
		gmcp_send(ch->desc, GMCP_PKG_ROOM_MAP, json_str);
		free(json_str);
	}
}

/* send pre-generated quest map buffer via gmcp */
void gmcp_send_quest_map(struct char_data *ch, const char *map_buf)
{
	char  *json_str;
	cJSON *json;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;
	if (!map_buf || !*map_buf)
		return;
	if (!ch->only.pc)
		return;

	json = cJSON_CreateObject();
	if (!json)
		return;

	cJSON_AddStringToObject(json, "map", map_buf);
	cJSON_AddNumberToObject(json, "zoneNumber", ch->only.pc->quest_zone_number);

	json_str = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);

	if (json_str)
	{
		gmcp_send(ch->desc, GMCP_PKG_QUEST_MAP, json_str);
		free(json_str);
	}
}

/*
 * room update throttling - track dirty rooms and flush periodically
 * to avoid flooding during high-activity scenarios like big battles
 */
#define MAX_DIRTY_ROOMS 500
static int dirty_rooms[MAX_DIRTY_ROOMS];
static int dirty_room_count = 0;

void gmcp_mark_room_dirty(int room_number)
{
	int i;

	if (room_number < 0)
		return;

	/* check if already marked */
	for (i = 0; i < dirty_room_count; i++)
	{
		if (dirty_rooms[i] == room_number)
			return;
	}

	/* add to dirty list if space available */
	if (dirty_room_count < MAX_DIRTY_ROOMS)
	{
		dirty_rooms[dirty_room_count++] = room_number;
	}
}

void gmcp_flush_dirty_rooms(void)
{
	int               i;
	struct char_data *tch;
	struct room_data *room;

	for (i = 0; i < dirty_room_count; i++)
	{
		room = &world[dirty_rooms[i]];
		if (!room)
			continue;

		/* send room.info to all players in this room */
		for (tch = room->people; tch; tch = tch->next_in_room)
		{
			if (!IS_NPC(tch) && tch->desc && GMCP_ENABLED(tch))
			{
				gmcp_room_info(tch);
			}
		}
	}

	dirty_room_count = 0;
}

/*
 * ship contacts auto-update - periodically send contact updates to
 * players on ships
 */

/* check if ship has any gmcp-enabled players on board */
static bool ship_has_gmcp_players(struct ShipData *ship)
{
	struct char_data *tch;
	int               bridge_rnum;

	if (!ship)
		return false;

	/* only check the bridge room (where the control panel is) */
	bridge_rnum = real_room(ship->bridge);
	if (bridge_rnum < 0)
		return false;

	for (tch = world[bridge_rnum].people; tch; tch = tch->next_in_room)
	{
		if (!IS_NPC(tch) && tch->desc && GMCP_ENABLED(tch))
		{
			return true;
		}
	}
	return false;
}

/* simple djb2 hash for strings */
static unsigned long hash_string(const char *str)
{
	unsigned long hash = 5381;
	int           c;

	if (!str)
		return 0;

	while ((c = *str++))
	{
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash;
}

/* build json for ship contacts */
static char *json_build_ship_contacts(struct ShipData *ship, struct descriptor_data *d)
{
	cJSON      *root, *contacts_arr, *contact_obj;
	char       *json_str;
	int         k, i;
	const char *race_str;
	const char *status_str;

	if (!ship)
		return NULL;

	root = cJSON_CreateObject();
	if (!root)
		return NULL;

	/* ship's own heading and speed */
	cJSON_AddNumberToObject(root, "heading", (int)ship->heading);
	cJSON_AddNumberToObject(root, "speed", ship->speed);

	/* world position - always calculate for hash, only send to verified clients */
	if (ship->location >= 0 && ship->location < top_of_world)
	{
		struct zone_data *zone = &zone_table[world[ship->location].zone];
		if (IS_SET(zone->flags, ZONE_MAP) && zone->mapx > 0)
		{
			int vroom      = world[ship->location].number;
			int zone_start = world[zone->real_bottom].number;
			int local_x    = (vroom - zone_start) % zone->mapx;
			int local_y    = ((vroom - zone_start) / zone->mapx) % zone->mapy;
			if (d && d->durisweb_verified)
			{
				cJSON_AddNumberToObject(root, "worldX", local_x);
				cJSON_AddNumberToObject(root, "worldY", local_y);
			}
			else if (!d)
			{
				/* include in hash calculation so position changes trigger updates */
				cJSON_AddNumberToObject(root, "worldX", local_x);
				cJSON_AddNumberToObject(root, "worldY", local_y);
			}
		}
	}

	contacts_arr = cJSON_CreateArray();
	if (!contacts_arr)
	{
		cJSON_Delete(root);
		return NULL;
	}

	/* only get contacts if on open sea and not docked */
	if (IS_MAP_ROOM(ship->location) && !SHIP_DOCKED(ship))
	{
		k = getcontacts(ship);
		if (k > MAXSHIPS)
			k = MAXSHIPS; /* bounds check */

		for (i = 0; i < k; i++)
		{
			/* skip docked ships beyond range 5 */
			if (SHIP_DOCKED(contacts[i].ship) && contacts[i].range > 5)
				continue;

			contact_obj = cJSON_CreateObject();
			if (!contact_obj)
				continue;

			/* ship identification */
			cJSON_AddStringToObject(contact_obj, "id", contacts[i].ship->id);
			if (contacts[i].ship->name)
			{
				cJSON_AddStringToObject(contact_obj, "name", strip_ansi(contacts[i].ship->name).c_str());
			}
			else
			{
				cJSON_AddStringToObject(contact_obj, "name", "Unknown");
			}

			/* position and navigation */
			cJSON_AddNumberToObject(contact_obj, "x", contacts[i].x);
			cJSON_AddNumberToObject(contact_obj, "y", contacts[i].y);
			cJSON_AddNumberToObject(contact_obj, "range", contacts[i].range);
			cJSON_AddNumberToObject(contact_obj, "bearing", (int)contacts[i].bearing);
			cJSON_AddNumberToObject(contact_obj, "heading", (int)contacts[i].ship->heading);
			cJSON_AddNumberToObject(contact_obj, "speed", contacts[i].ship->speed);
			cJSON_AddStringToObject(contact_obj, "arc", contacts[i].arc);

			/* race/alignment (only if in scan range and not docked) */
			race_str = "unknown";
			if (contacts[i].range < SCAN_RANGE && !SHIP_DOCKED(contacts[i].ship))
			{
				switch (contacts[i].ship->race)
				{
					case GOODIESHIP:
						race_str = "good";
						break;
					case EVILSHIP:
						race_str = "evil";
						break;
					case UNDEADSHIP:
						race_str = "undead";
						break;
					case SQUIDSHIP:
						race_str = "squid";
						break;
					default:
						race_str = "unknown";
						break;
				}
			}
			cJSON_AddStringToObject(contact_obj, "race", race_str);

			/* status flags */
			status_str = "";
			if (SHIP_FLYING(contacts[i].ship))
				status_str = "flying";
			else if (SHIP_SINKING(contacts[i].ship))
				status_str = "sinking";
			else if (SHIP_IMMOBILE(contacts[i].ship))
				status_str = "immobile";
			else if (SHIP_DOCKED(contacts[i].ship))
				status_str = "docked";
			else if (SHIP_ANCHORED(contacts[i].ship))
				status_str = "anchored";
			cJSON_AddStringToObject(contact_obj, "status", status_str);

			/* targeting indicators */
			cJSON_AddBoolToObject(contact_obj, "targeting_you", contacts[i].ship->target == ship);
			cJSON_AddBoolToObject(contact_obj, "you_targeting", contacts[i].ship == ship->target);

			cJSON_AddItemToArray(contacts_arr, contact_obj);
		}
	}

	cJSON_AddItemToObject(root, "contacts", contacts_arr);

	json_str = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return json_str;
}

void gmcp_flush_dirty_ship_contacts(void)
{
	ShipVisitor       svs;
	struct ShipData  *ship;
	struct char_data *tch;
	char             *json, *player_json;
	unsigned long     new_hash;
	int               bridge_rnum;

	/* iterate through all ships */
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		ship = svs;

		if (!ship || !SHIP_LOADED(ship))
			continue;

		/* skip ships not on map, docked, or without gmcp players on bridge */
		if (!IS_MAP_ROOM(ship->location))
			continue;
		if (SHIP_DOCKED(ship))
			continue;
		if (!ship_has_gmcp_players(ship))
			continue;

		/* build base json for hash check */
		json = json_build_ship_contacts(ship, NULL);
		if (!json)
			continue;

		/* calculate hash of current contacts */
		new_hash = hash_string(json);

		/* send if contacts changed OR location changed */
		if (new_hash != ship->contacts_hash || ship->location != ship->last_gmcp_location)
		{
			bridge_rnum = real_room(ship->bridge);
			if (bridge_rnum >= 0)
			{
				for (tch = world[bridge_rnum].people; tch; tch = tch->next_in_room)
				{
					if (!IS_NPC(tch) && tch->desc && GMCP_ENABLED(tch))
					{
						/* build per-player json */
						player_json = json_build_ship_contacts(ship, tch->desc);
						if (player_json)
						{
							gmcp_send(tch->desc, GMCP_PKG_SHIP_CONTACTS, player_json);
							free(player_json);
						}
					}
				}
			}
			ship->contacts_hash      = new_hash;
			ship->last_gmcp_location = ship->location;
		}

		free(json);
	}
}

/* ship.info - sends static/slow-changing ship data */
void gmcp_flush_dirty_ship_info(void)
{
	ShipVisitor       svs;
	struct ShipData  *ship;
	struct char_data *tch;
	char             *json, *player_json;
	unsigned long     new_hash;
	int               bridge_rnum;

	/* iterate through all ships */
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		ship = svs;

		if (!ship || !SHIP_LOADED(ship))
			continue;

		/* skip ships without gmcp players on bridge */
		if (!ship_has_gmcp_players(ship))
			continue;

		/* build base json for hash check (no player-specific bonuses) */
		json = json_build_ship_info(ship, NULL);
		if (!json)
			continue;

		/* check if anything changed */
		new_hash = hash_string(json);

		if (new_hash != ship->ship_info_hash)
		{
			bridge_rnum = real_room(ship->bridge);
			if (bridge_rnum >= 0)
			{
				for (tch = world[bridge_rnum].people; tch; tch = tch->next_in_room)
				{
					if (!IS_NPC(tch) && tch->desc && GMCP_ENABLED(tch))
					{
						/* build per-player json with racial bonuses */
						player_json = json_build_ship_info(ship, tch);
						if (player_json)
						{
							gmcp_send(tch->desc, GMCP_PKG_SHIP_INFO, player_json);
							free(player_json);
						}
					}
				}
			}
			ship->ship_info_hash = new_hash;
		}

		free(json);
	}
}

/* send char.vitals when hp/mana/move changes */
void gmcp_char_vitals(struct char_data *ch)
{
	char *json;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	json = json_build_char_vitals(ch);

	if (json)
	{
		gmcp_send(ch->desc, GMCP_PKG_CHAR_VITALS, json);
		free(json);
	}
}

/* send vitals update to all group members */
void gmcp_group_vitals(struct char_data *ch)
{
	struct group_list *gl;

	if (!ch || !ch->group)
		return;

	/* iterate through all group members */
	for (gl = ch->group; gl; gl = gl->next)
	{
		if (gl->ch && GMCP_ENABLED(gl->ch))
		{
			gmcp_char_vitals(gl->ch);
		}
	}
}

/* send group.status for group panel with member hp/move/position */
void gmcp_send_group_status(struct char_data *ch)
{
	cJSON             *root, *members_arr, *member_obj;
	struct group_list *gl;
	char              *json_str;
	int                count    = 0;
	int                max_size = 20; /* Default max group size */
	int                is_first = 1;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	/* if not in a group, send empty */
	if (!ch->group)
	{
		gmcp_send(ch->desc, GMCP_PKG_GROUP_STATUS, "{\"members\":[],\"size\":0,\"maxSize\":20}");
		return;
	}

	root        = cJSON_CreateObject();
	members_arr = cJSON_CreateArray();

	/* iterate through group members */
	for (gl = ch->group; gl; gl = gl->next)
	{
		struct char_data *member = gl->ch;
		if (!member)
			continue;

		member_obj = cJSON_CreateObject();

		cJSON_AddStringToObject(member_obj, "name", GET_NAME(member));
		cJSON_AddNumberToObject(member_obj, "level", GET_LEVEL(member));

		/* class - null for npcs */
		if (IS_NPC(member))
		{
			cJSON_AddNullToObject(member_obj, "class");
		}
		else
		{
			cJSON_AddStringToObject(member_obj, "class", class_names_table[flag2idx(member->player.m_class)].normal);
		}

		/* race - null for npcs */
		if (IS_NPC(member))
		{
			cJSON_AddNullToObject(member_obj, "race");
		}
		else
		{
			cJSON_AddStringToObject(member_obj, "race", race_names_table[(int)GET_RACE(member)].normal);
		}

		cJSON_AddNumberToObject(member_obj, "hp", GET_HIT(member));
		cJSON_AddNumberToObject(member_obj, "maxHp", GET_MAX_HIT(member));
		cJSON_AddNumberToObject(member_obj, "move", GET_VITALITY(member));
		cJSON_AddNumberToObject(member_obj, "maxMove", GET_MAX_VITALITY(member));

		/* position */
		const char *pos_str;
		switch (GET_POS(member))
		{
			case POS_PRONE:
				pos_str = "prone";
				break;
			case POS_SITTING:
				pos_str = "sitting";
				break;
			case POS_KNEELING:
				pos_str = "kneeling";
				break;
			case POS_STANDING:
				pos_str = "standing";
				break;
			default:
				pos_str = "standing";
				break;
		}
		cJSON_AddStringToObject(member_obj, "position", pos_str);

		/* rank - head for leader, front/back based on back_rank flag */
		const char *rank_str;
		if (is_first)
		{
			rank_str = "head";
			is_first = 0;
		}
		else if (!IS_NPC(member) && PLR2_FLAGGED(member, PLR2_BACK_RANK))
		{
			rank_str = "back";
		}
		else
		{
			rank_str = "front";
		}
		cJSON_AddStringToObject(member_obj, "rank", rank_str);

		cJSON_AddBoolToObject(member_obj, "isNpc", IS_NPC(member) ? 1 : 0);
		cJSON_AddBoolToObject(member_obj, "inRoom", (member->in_room == ch->in_room) ? 1 : 0);

		/* for npcs, calculate target number and keyword */
		if (IS_NPC(member) && member->in_room == ch->in_room)
		{
			/* count matching mobs using lifo order (last in = target #1) */
			struct char_data *tch;
			int               target_num      = 0;
			int               total_matching  = 0;
			int               member_position = 0;
			const char       *first_keyword   = NULL;

			/* get first keyword from npc's name list */
			if (member->player.name)
			{
				static char keyword_buf[64];
				strlcpy(keyword_buf, member->player.name, sizeof keyword_buf);
				/* get first word (keyword) */
				char *space = strchr(keyword_buf, ' ');
				if (space)
					*space = '\0';
				first_keyword = keyword_buf;
			}

			/* count matching mobs in room - lifo order means last in = #1 */
			if (first_keyword)
			{
				/* first pass: count total matching mobs and find member's position */
				for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
				{
					if (IS_NPC(tch) && tch->player.name)
					{
						/* check if this mob matches the keyword */
						if (isname(first_keyword, tch->player.name))
						{
							total_matching++;
							if (tch == member)
							{
								member_position = total_matching;
							}
						}
					}
				}
				/* lifo: last mob in list = #1, so reverse the position */
				if (member_position > 0)
				{
					target_num = total_matching - member_position + 1;
				}
			}

			if (target_num > 0 && first_keyword)
			{
				cJSON_AddNumberToObject(member_obj, "targetNum", target_num);
				cJSON_AddStringToObject(member_obj, "targetKeyword", first_keyword);
			}
			else
			{
				cJSON_AddNullToObject(member_obj, "targetNum");
				cJSON_AddNullToObject(member_obj, "targetKeyword");
			}
		}
		else
		{
			cJSON_AddNullToObject(member_obj, "targetNum");
			cJSON_AddNullToObject(member_obj, "targetKeyword");
		}

		cJSON_AddItemToArray(members_arr, member_obj);
		count++;
	}

	cJSON_AddItemToObject(root, "members", members_arr);
	cJSON_AddNumberToObject(root, "size", count);
	cJSON_AddNumberToObject(root, "maxSize", max_size);

	json_str = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	if (json_str)
	{
		gmcp_send(ch->desc, GMCP_PKG_GROUP_STATUS, json_str);
		free(json_str);
	}
}

/* send char.status on login, level up, etc. */
void gmcp_char_status(struct char_data *ch)
{
	char *json;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	json = json_build_char_status(ch);

	if (json)
	{
		gmcp_send(ch->desc, GMCP_PKG_CHAR_STATUS, json);
		free(json);
	}
}

/* send char.affects when buffs/debuffs change */
void gmcp_char_affects(struct char_data *ch)
{
	char *json;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	json = json_build_char_affects(ch);

	if (json)
	{
		gmcp_send(ch->desc, GMCP_PKG_CHAR_AFFECTS, json);
		free(json);
	}
}

/* send combat.update for a combat round */
void gmcp_combat_update(struct char_data *ch, struct char_data *victim, int damage, const char *damage_type, int critical)
{
	cJSON *root, *target, *round;
	char  *json;
	int    health_pct;

	if (!ch || !ch->desc || !victim)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	root = cJSON_CreateObject();

	/* target info */
	target = cJSON_CreateObject();
	cJSON_AddStringToObject(target, "name", GET_NAME(victim));

	/* calculate health percentage */
	if (GET_MAX_HIT(victim) > 0)
	{
		health_pct = (GET_HIT(victim) * 100) / GET_MAX_HIT(victim);
		if (health_pct < 0)
			health_pct = 0;
	}
	else
	{
		health_pct = 0;
	}

	/* health description */
	const char *health_desc;
	if (health_pct >= 100)
		health_desc = "excellent";
	else if (health_pct >= 90)
		health_desc = "few scratches";
	else if (health_pct >= 75)
		health_desc = "small wounds";
	else if (health_pct >= 50)
		health_desc = "quite a few wounds";
	else if (health_pct >= 30)
		health_desc = "big nasty wounds";
	else if (health_pct >= 15)
		health_desc = "pretty hurt";
	else if (health_pct >= 1)
		health_desc = "awful";
	else
		health_desc = "bleeding to death";

	cJSON_AddStringToObject(target, "health", health_desc);
	cJSON_AddNumberToObject(target, "healthPercent", health_pct);

	/* target position */
	const char *positions[] = {"on their ass", "sitting", "kneeling", "standing"};
	int         pos         = GET_POS(victim);
	const char *pos_desc    = (pos >= 0 && pos <= 3) ? positions[pos] : "standing";
	cJSON_AddStringToObject(target, "position", pos_desc);
	cJSON_AddItemToObject(root, "target", target);

	/* round info */
	if (damage > 0 || damage_type)
	{
		round = cJSON_CreateObject();
		cJSON_AddStringToObject(round, "attacker", GET_NAME(ch));
		cJSON_AddNumberToObject(round, "damage", damage);
		cJSON_AddStringToObject(round, "damageType", damage_type ? damage_type : "physical");
		cJSON_AddBoolToObject(round, "critical", critical);
		cJSON_AddItemToObject(root, "round", round);
	}

	json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	if (json)
	{
		gmcp_send(ch->desc, GMCP_PKG_COMBAT_UPDATE, json);
		free(json);
	}
}

/* send target health update for health bar */
void gmcp_combat_target(struct char_data *ch, struct char_data *victim) { gmcp_combat_update(ch, victim, 0, NULL, 0); }

/* send combat ended notification */
void gmcp_combat_end(struct char_data *ch)
{
	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	gmcp_send(ch->desc, GMCP_PKG_COMBAT_UPDATE, "{\"target\":null}");
}

/* send channel message via gmcp */
void gmcp_comm_channel(struct char_data *ch, const char *channel, const char *sender, const char *text)
{
	char *json;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	json = json_build_comm_channel(channel, sender, text);

	if (json)
	{
		gmcp_send(ch->desc, GMCP_PKG_COMM_CHANNEL, json);
		free(json);
	}
}

/* send channel message via gmcp with alignment (for nchat) */
void gmcp_comm_channel_ex(struct char_data *ch, const char *channel, const char *sender, const char *text, const char *alignment)
{
	char *json;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	json = json_build_comm_channel_ex(channel, sender, text, alignment);

	if (json)
	{
		gmcp_send(ch->desc, GMCP_PKG_COMM_CHANNEL, json);
		free(json);
	}
}

/* broadcast channel message to all gmcp-enabled players */
void gmcp_broadcast_channel(const char *channel, const char *sender, const char *text, struct char_data *exclude)
{
	struct descriptor_data *d;
	char                   *json;

	json = json_build_comm_channel(channel, sender, text);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->character && d->character != exclude && STATE(d) == CON_PLAYING && d->gmcp_enabled)
		{
			gmcp_send(d, GMCP_PKG_COMM_CHANNEL, json);
		}
	}

	free(json);
}

/* send quest.status for bartender quests */
void gmcp_quest_status(struct char_data *ch)
{
	char *json;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;

	json = json_build_quest_status(ch);

	if (json)
	{
		gmcp_send(ch->desc, GMCP_PKG_QUEST_STATUS, json);
		free(json);
	}

	/* send quest map once per session if player has one purchased */
	if (ch->only.pc && ch->only.pc->quest_map_bought == 1 && !ch->desc->gmcp_quest_map_sent)
	{
		gmcp_quest_map(ch);
		ch->desc->gmcp_quest_map_sent = 1;
	}
}

/* generate and send quest.map for bartender quests */
void gmcp_quest_map(struct char_data *ch)
{
	int old_room, map_room;

	if (!ch || !ch->desc)
		return;
	if (!GMCP_ENABLED(ch))
		return;
	if (!ch->only.pc)
		return;
	if (ch->only.pc->quest_map_bought != 1)
		return;

	map_room = real_room(ch->only.pc->quest_map_room);
	if (map_room < 0)
		return;

	/* temporarily move to map room */
	old_room = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, map_room, -2);

	/* generate and send quest map (20 tile radius) */
	display_map_room(ch, map_room, 20, 999, 1);

	/* return to original room */
	char_from_room(ch);
	char_to_room(ch, old_room, -2);
}
