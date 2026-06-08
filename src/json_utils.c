/*
 * json_utils.c - json helpers for websocket stuff
 *
 * builds and parses json messages for the web client.
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "json_utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assocs.h"
#include "defines.h"
#include "handler.h"
#include "ships/ships.h"
#include "specializations.h"
#include "spells.h"

/* externs */
extern const char             *class_abbrevs[];
extern const char             *race_abbrevs[];
extern const char             *specdata[][MAX_SPEC];
extern const char             *pc_class_types[];
extern const struct race_names race_names_table[];
extern struct room_data       *world;
extern struct zone_data       *zone_table;
extern struct index_data      *mob_index;
extern struct index_data      *obj_index;
extern struct time_info_data   time_info;
extern const char             *month_name[];
extern const char             *size_types[];
int                            get_vis_mode(P_char ch, int room);

/* sql function for quest remaining count */
int sql_world_quest_can_do_another(struct char_data *ch);

/* parse incoming json command */
cJSON *json_parse_command(const char *json_str)
{
	if (!json_str || !*json_str)
		return NULL;
	return cJSON_Parse(json_str);
}

/* get command type from parsed json */
const char *json_get_cmd_type(cJSON *json)
{
	cJSON *type = cJSON_GetObjectItem(json, "type");
	if (type && cJSON_IsString(type))
	{
		return type->valuestring;
	}
	return NULL;
}

/* get command name from parsed json */
const char *json_get_cmd_name(cJSON *json)
{
	cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
	if (cmd && cJSON_IsString(cmd))
	{
		return cmd->valuestring;
	}
	return NULL;
}

/* get data object from parsed json */
cJSON *json_get_data(cJSON *json) { return cJSON_GetObjectItem(json, "data"); }

/* get string from data object */
const char *json_get_string(cJSON *data, const char *key)
{
	cJSON *item;
	if (!data)
		return NULL;

	/* If data is a string itself (for simple commands like "game") */
	if (cJSON_IsString(data) && key == NULL)
	{
		return data->valuestring;
	}

	item = cJSON_GetObjectItem(data, key);
	if (item && cJSON_IsString(item))
	{
		return item->valuestring;
	}
	return NULL;
}

/* get integer from data object */
int json_get_int(cJSON *data, const char *key, int default_val)
{
	cJSON *item;
	if (!data)
		return default_val;

	item = cJSON_GetObjectItem(data, key);
	if (item && cJSON_IsNumber(item))
	{
		return item->valueint;
	}
	return default_val;
}

/* check if utf-8 sequence is valid, returns byte count (1-4) or 0 if invalid */
static int utf8_sequence_len(const char *str, size_t i)
{
	unsigned char c       = (unsigned char)str[i];
	int           seq_len = 0;

	if (c < 0x80)
	{
		/* ASCII (0x00-0x7F) */
		return 1;
	}
	else if ((c & 0xE0) == 0xC0)
	{
		/* 2-byte sequence (0xC0-0xDF) */
		seq_len = 2;
	}
	else if ((c & 0xF0) == 0xE0)
	{
		/* 3-byte sequence (0xE0-0xEF) */
		seq_len = 3;
	}
	else if ((c & 0xF8) == 0xF0)
	{
		/* 4-byte sequence (0xF0-0xF7) */
		seq_len = 4;
	}
	else
	{
		/* Invalid leading byte or continuation byte (0x80-0xBF, 0xF8-0xFF) */
		return 0;
	}

	/* Validate continuation bytes (must be 0x80-0xBF) */
	for (int j = 1; j < seq_len; j++)
	{
		unsigned char cont = (unsigned char)str[i + j];
		if (cont == 0 || (cont & 0xC0) != 0x80)
		{
			return 0; /* Missing or invalid continuation byte */
		}
	}

	return seq_len;
}

/* escape string for json with utf-8 support (invalid bytes are skipped) */
char *json_escape_string(const char *str)
{
	size_t        len, i, j;
	char         *escaped;
	unsigned char c;

	if (!str)
		return strdup("");

	/* Calculate required length */
	len = 0;
	for (i = 0; str[i];)
	{
		c = (unsigned char)str[i];

		if (c >= 0x80)
		{
			/* Check for valid UTF-8 sequence */
			int seq_len = utf8_sequence_len(str, i);
			if (seq_len > 1)
			{
				len += seq_len; /* Valid UTF-8 passes through */
				i += seq_len;
			}
			else
			{
				i++; /* Skip invalid byte */
			}
			continue;
		}

		switch (str[i])
		{
			case '"':
			case '\\':
			case '\n':
			case '\r':
			case '\t':
				len += 2;
				break;
			default:
				if (c < 32)
				{
					len += 6; /* \uXXXX */
				}
				else
				{
					len++;
				}
		}
		i++;
	}

	escaped = (char *)malloc(len + 1);
	if (!escaped)
		return NULL;

	j = 0;
	for (i = 0; str[i];)
	{
		c = (unsigned char)str[i];

		if (c >= 0x80)
		{
			/* Check for valid UTF-8 sequence */
			int seq_len = utf8_sequence_len(str, i);
			if (seq_len > 1)
			{
				/* Copy valid UTF-8 sequence */
				for (int k = 0; k < seq_len; k++)
				{
					escaped[j++] = str[i + k];
				}
				i += seq_len;
			}
			else
			{
				i++; /* Skip invalid byte */
			}
			continue;
		}

		switch (str[i])
		{
			case '"':
				escaped[j++] = '\\';
				escaped[j++] = '"';
				break;
			case '\\':
				escaped[j++] = '\\';
				escaped[j++] = '\\';
				break;
			case '\n':
				escaped[j++] = '\\';
				escaped[j++] = 'n';
				break;
			case '\r':
				escaped[j++] = '\\';
				escaped[j++] = 'r';
				break;
			case '\t':
				escaped[j++] = '\\';
				escaped[j++] = 't';
				break;
			default:
				if (c < 32)
				{
					j += sprintf(escaped + j, "\\u%04x", c);
				}
				else
				{
					escaped[j++] = str[i];
				}
		}
		i++;
	}
	escaped[j] = '\0';

	return escaped;
}

/* strip ansi color codes (&+X, &-X, &=XY, &n, &N) and escape for json */
char *json_escape_ansi_string(const char *str)
{
	char  *stripped, *escaped;
	size_t len, i, j;

	if (!str)
		return strdup("");

	/* First pass: calculate length without color codes */
	len = 0;
	for (i = 0; str[i]; i++)
	{
		if (str[i] == '&' && str[i + 1])
		{
			if (str[i + 1] == '+' || str[i + 1] == '-')
			{
				/* &+X or &-X - skip 3 chars total */
				i += 2; /* loop will add 1 more */
			}
			else if (str[i + 1] == '=')
			{
				/* &=XY - skip 4 chars total */
				i += 3; /* loop will add 1 more */
			}
			else if (str[i + 1] == 'n' || str[i + 1] == 'N')
			{
				/* &n or &N - skip 2 chars total */
				i += 1; /* loop will add 1 more */
			}
			else
			{
				len++; /* Not a color code, keep the '&' */
			}
		}
		else
		{
			len++;
		}
	}

	stripped = (char *)malloc(len + 1);
	if (!stripped)
		return NULL;

	/* Second pass: copy without color codes */
	j = 0;
	for (i = 0; str[i]; i++)
	{
		if (str[i] == '&' && str[i + 1])
		{
			if (str[i + 1] == '+' || str[i + 1] == '-')
			{
				i += 2;
			}
			else if (str[i + 1] == '=')
			{
				i += 3;
			}
			else if (str[i + 1] == 'n' || str[i + 1] == 'N')
			{
				i += 1;
			}
			else
			{
				stripped[j++] = str[i];
			}
		}
		else
		{
			stripped[j++] = str[i];
		}
	}
	stripped[j] = '\0';

	/* Now escape for JSON */
	escaped = json_escape_string(stripped);
	free(stripped);

	return escaped;
}

/* free json string */
void json_free_string(char *str)
{
	if (str)
		free(str);
}

/* build auth success response */
char *json_build_auth_success(const char *account, struct char_data *char_list)
{
	cJSON            *root, *data, *characters, *char_obj;
	struct char_data *ch;
	char             *result;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", MSG_TYPE_AUTH);
	cJSON_AddStringToObject(root, "status", AUTH_SUCCESS);

	data = cJSON_CreateObject();
	cJSON_AddStringToObject(data, "account", account);

	characters = cJSON_CreateArray();

	/* Add each character */
	for (ch = char_list; ch; ch = ch->next)
	{
		char_obj = cJSON_CreateObject();
		cJSON_AddStringToObject(char_obj, "name", GET_NAME(ch));
		cJSON_AddStringToObject(char_obj, "class", get_class_name(ch, NULL));
		cJSON_AddNumberToObject(char_obj, "level", GET_LEVEL(ch));
		cJSON_AddStringToObject(char_obj, "race", race_names_table[(int)GET_RACE(ch)].normal);
		cJSON_AddItemToArray(characters, char_obj);
	}

	cJSON_AddItemToObject(data, "characters", characters);
	cJSON_AddItemToObject(root, "data", data);

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build auth failure response */
char *json_build_auth_failed(const char *error)
{
	cJSON *root;
	char  *result;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", MSG_TYPE_AUTH);
	cJSON_AddStringToObject(root, "status", AUTH_FAILED);
	cJSON_AddStringToObject(root, "error", error ? error : "Unknown error");

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build text message with category */
char *json_build_text(const char *category, const char *text)
{
	cJSON *root;
	char  *result;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", MSG_TYPE_TEXT);
	cJSON_AddStringToObject(root, "category", category ? category : TEXT_INFO);
	cJSON_AddStringToObject(root, "data", text ? text : "");

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build complete gmcp json wrapper */
char *json_build_gmcp_message(const char *package, const char *data_json)
{
	cJSON *root, *data;
	char  *result;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", MSG_TYPE_GMCP);
	cJSON_AddStringToObject(root, "package", package);

	/* Parse the data JSON and add it */
	data = cJSON_Parse(data_json);
	if (data)
	{
		cJSON_AddItemToObject(root, "data", data);
	}
	else
	{
		cJSON_AddNullToObject(root, "data");
	}

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build room.info gmcp message (ch is for visibility checks, can be null) */
char *json_build_room_info(struct room_data *room, struct char_data *ch)
{
	cJSON            *root, *exits, *coords, *doors, *players, *npcs, *items;
	char             *result;
	char             *clean_name, *clean_area;
	int               dir;
	struct char_data *tch;
	struct obj_data  *obj;
	/* standard gmcp uses short exit names */
	/* order matches defines.h: n,e,s,w,u,d,nw,sw,ne,se */
	const char *dir_abbrevs[] = {"n", "e", "s", "w", "u", "d", "nw", "sw", "ne", "se"};
	/* Terrain type to environment string mapping */
	const char *env_names[] = {
		"inside",
		"city",
		"field",
		"forest",
		"hills",
		"mountains",
		"water swim",
		"water noswim",
		"no ground",
		"underwater",
		"underwater ground",
		"plane of fire",
		"ocean",
		"ud wild",
		"ud city",
		"ud inside",
		"ud water swim",
		"ud water noswim",
		"ud no ground",
		"plane of air",
		"plane of water",
		"plane of earth",
		"plane of ethereal",
		"plane of astral",
		"desert",
		"tundra",
		"swamp",
		"ud mountains",
		"ud slime",
		"ud low ceilings",
		"ud liquid mithril",
		"ud mushroom forest",
		"outer castle wall",
		"castle gate",
		"castle",
		"negative plane",
		"plane of avernus",
		"patrolled road",
		"snowy forest",
		"lava",
	};

	if (!room)
		return strdup("{}");

	root = cJSON_CreateObject();

	/* Standard field: num (not vnum) - hide for wilderness zones */
	int  zone_num      = (room->zone >= 0) ? zone_table[room->zone].number : 0;
	bool is_wilderness = (zone_num == 600 ||                        // The Adventurers Shipyards
	                      (zone_num >= 1200 && zone_num <= 1238) || // Alatorin
	                      (zone_num >= 5000 && zone_num <= 6599) || // Surface
	                      (zone_num >= 6600 && zone_num <= 6999) || // Newbie Maps
	                      (zone_num >= 7000 && zone_num <= 8599));  // Underdark
	if (is_wilderness)
	{
		cJSON_AddNumberToObject(root, "num", 0);
	}
	else
	{
		cJSON_AddNumberToObject(root, "num", room->number);
	}

	/* Room name - clean for Mudlet, colored for web */
	clean_name = json_escape_ansi_string(room->name ? room->name : "Unknown");
	cJSON_AddStringToObject(root, "name", clean_name);
	free(clean_name);
	clean_name = json_escape_string(room->name ? room->name : "Unknown");
	cJSON_AddStringToObject(root, "colored_name", clean_name);
	free(clean_name);

	/* Area name - clean for Mudlet, colored for web */
	if (room->zone >= 0 && zone_table[room->zone].name)
	{
		clean_area = json_escape_ansi_string(zone_table[room->zone].name);
		cJSON_AddStringToObject(root, "area", clean_area);
		free(clean_area);
		clean_area = json_escape_string(zone_table[room->zone].name);
		cJSON_AddStringToObject(root, "colored_area", clean_area);
		free(clean_area);
	}
	else
	{
		cJSON_AddStringToObject(root, "area", "Unknown");
		cJSON_AddStringToObject(root, "colored_area", "Unknown");
	}

	/* Standard field: environment (string, not terrain number) */
	size_t env_count = sizeof(env_names) / sizeof(env_names[0]);

	if (room->sector_type >= 0 && (size_t)room->sector_type < env_count)
	{
		cJSON_AddStringToObject(root, "environment", env_names[room->sector_type]);
	}
	else
	{
		cJSON_AddStringToObject(root, "environment", "unknown");
	}

	/* Standard field: coords object - hide for wilderness zones */
	coords = cJSON_CreateObject();
	if (!is_wilderness)
	{
		cJSON_AddNumberToObject(coords, "x", room->x_coord);
		cJSON_AddNumberToObject(coords, "y", room->y_coord);
		cJSON_AddNumberToObject(coords, "z", room->z_coord);
	}
	cJSON_AddItemToObject(root, "coords", coords);

	/* Build exits object - standard format: "n": 12345 (just the room number) */
	/* Skip secret exits unless the door is open (discovered) */
	/* Hide vnums for wilderness zones (show true instead) */
	exits = cJSON_CreateObject();
	for (dir = 0; dir < 10; dir++)
	{
		if (room->dir_option[dir] && room->dir_option[dir]->to_room != NOWHERE)
		{
			/* Hide secret exits unless door is open */
			if (IS_SET(room->dir_option[dir]->exit_info, EX_SECRET) && IS_SET(room->dir_option[dir]->exit_info, EX_CLOSED))
			{
				continue;
			}
			if (is_wilderness)
			{
				cJSON_AddBoolToObject(exits, dir_abbrevs[dir], 1);
			}
			else
			{
				cJSON_AddNumberToObject(exits, dir_abbrevs[dir], world[room->dir_option[dir]->to_room].number);
			}
		}
	}
	cJSON_AddItemToObject(root, "exits", exits);

	/* Build doors object - only for exits that have doors */
	/* Skip secret doors that are still closed (undiscovered) */
	doors = cJSON_CreateObject();
	for (dir = 0; dir < 10; dir++)
	{
		if (room->dir_option[dir] && IS_SET(room->dir_option[dir]->exit_info, EX_ISDOOR))
		{
			/* Hide secret doors unless they are open (discovered) */
			if (IS_SET(room->dir_option[dir]->exit_info, EX_SECRET) && IS_SET(room->dir_option[dir]->exit_info, EX_CLOSED))
			{
				continue;
			}
			cJSON *door          = cJSON_CreateObject();
			char  *clean_keyword = json_escape_ansi_string(room->dir_option[dir]->keyword ? room->dir_option[dir]->keyword : "door");
			cJSON_AddStringToObject(door, "name", clean_keyword);
			free(clean_keyword);
			cJSON_AddBoolToObject(door, "closed", IS_SET(room->dir_option[dir]->exit_info, EX_CLOSED));
			cJSON_AddBoolToObject(door, "locked", IS_SET(room->dir_option[dir]->exit_info, EX_LOCKED));
			cJSON_AddItemToObject(doors, dir_abbrevs[dir], door);
		}
	}
	cJSON_AddItemToObject(root, "doors", doors);

	/* Build players array - visible PCs in room */
	players = cJSON_CreateArray();
	if (ch)
	{
		for (tch = room->people; tch; tch = tch->next_in_room)
		{
			if (tch == ch)
				continue; /* Skip self */
			if (IS_NPC(tch))
				continue; /* Skip NPCs */
			if (!CAN_SEE(ch, tch))
				continue; /* Visibility check */

			cJSON *player = cJSON_CreateObject();
			clean_name    = json_escape_ansi_string(GET_NAME(tch));
			cJSON_AddStringToObject(player, "name", clean_name);
			free(clean_name);
			cJSON_AddItemToArray(players, player);
		}
	}
	cJSON_AddItemToObject(root, "players", players);

	/* Build npcs array - visible NPCs in room */
	npcs = cJSON_CreateArray();
	if (ch)
	{
		int vis_mode = get_vis_mode(ch, ch->in_room);
		for (tch = room->people; tch; tch = tch->next_in_room)
		{
			if (!IS_NPC(tch))
				continue; /* Skip PCs */
			if (!CAN_SEE(ch, tch))
				continue; /* Visibility check */

			cJSON *npc = cJSON_CreateObject();

			/* check if player can only see red shapes (infravision mode) */
			if (vis_mode == 3)
			{
				cJSON_AddStringToObject(npc, "name", "a red shape");
				cJSON_AddStringToObject(npc, "colored_name", "&+ra red shape&n");
				cJSON_AddNumberToObject(npc, "vnum", 0);
				cJSON_AddStringToObject(npc, "keyword", "");
			}
			else
			{
				/* normal visibility - use short_descr */
				clean_name = json_escape_ansi_string(tch->player.short_descr ? tch->player.short_descr : GET_NAME(tch));
				cJSON_AddStringToObject(npc, "name", clean_name);
				free(clean_name);
				clean_name = json_escape_string(tch->player.short_descr ? tch->player.short_descr : GET_NAME(tch));
				cJSON_AddStringToObject(npc, "colored_name", clean_name);
				free(clean_name);
				cJSON_AddNumberToObject(npc, "vnum", GET_VNUM(tch));
				/* Extract first keyword from NPC's name list for targeting */
				if (tch->player.name)
				{
					char keyword_buf[64];
					strlcpy(keyword_buf, tch->player.name, sizeof(keyword_buf));
					char *space                          = strchr(keyword_buf, ' ');
					if (space)
						*space = '\0'; /* Get first word only */
					cJSON_AddStringToObject(npc, "keyword", keyword_buf);
				}
			}

			/* add fighting status if applicable */
			if (IS_FIGHTING(tch) && GET_OPPONENT(tch))
			{
				struct char_data *opponent = GET_OPPONENT(tch);
				if (opponent == ch)
				{
					cJSON_AddStringToObject(npc, "fighting", "you");
				}
				else if (opponent->in_room != tch->in_room)
				{
					cJSON_AddStringToObject(npc, "fighting", "someone who has already left");
				}
				else if (CAN_SEE(ch, opponent))
				{
					if (IS_NPC(opponent))
					{
						/* fighting another npc - use short_descr */
						clean_name = json_escape_ansi_string(opponent->player.short_descr ? opponent->player.short_descr : GET_NAME(opponent));
						cJSON_AddStringToObject(npc, "fighting", clean_name);
						free(clean_name);
					}
					else
					{
						/* fighting a player - use name */
						cJSON_AddStringToObject(npc, "fighting", GET_NAME(opponent));
					}
				}
				else
				{
					cJSON_AddStringToObject(npc, "fighting", "someone");
				}
			}

			cJSON_AddItemToArray(npcs, npc);
		}
	}
	cJSON_AddItemToObject(root, "npcs", npcs);

	/* Build items array - visible objects in room */
	items = cJSON_CreateArray();
	if (ch)
	{
		for (obj = room->contents; obj; obj = obj->next_content)
		{
			if (!CAN_SEE_OBJ(ch, obj))
				continue; /* Visibility check */

			cJSON *item = cJSON_CreateObject();
			/* Clean name for Mudlet compatibility */
			clean_name = json_escape_ansi_string(obj->short_description);
			cJSON_AddStringToObject(item, "name", clean_name);
			free(clean_name);
			/* Colored name for web client */
			clean_name = json_escape_string(obj->short_description);
			cJSON_AddStringToObject(item, "colored_name", clean_name);
			free(clean_name);
			cJSON_AddNumberToObject(item, "vnum", OBJ_VNUM(obj));
			cJSON_AddItemToArray(items, item);
		}
	}
	cJSON_AddItemToObject(root, "items", items);

	/* DurisMUD extensions (beyond standard GMCP) */
	if (room->zone >= 0)
	{
		cJSON_AddNumberToObject(root, "zone", zone_table[room->zone].number);
	}
	else
	{
		cJSON_AddNumberToObject(root, "zone", 0);
	}
	cJSON_AddNumberToObject(root, "terrain", room->sector_type);

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build char.vitals gmcp message */
char *json_build_char_vitals(struct char_data *ch)
{
	cJSON *root;
	char  *result;

	if (!ch)
		return strdup("{}");

	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "hp", GET_HIT(ch));
	cJSON_AddNumberToObject(root, "maxHp", GET_MAX_HIT(ch));
	cJSON_AddNumberToObject(root, "mana", GET_MANA(ch));
	cJSON_AddNumberToObject(root, "maxMana", GET_MAX_MANA(ch));
	cJSON_AddNumberToObject(root, "move", GET_VITALITY(ch));
	cJSON_AddNumberToObject(root, "maxMove", GET_MAX_VITALITY(ch));
	cJSON_AddNumberToObject(root, "exp", GET_EXP(ch));

	/* Experience to next level */
	extern long new_exp_table[];
	long        tnl = 0;
	if (GET_LEVEL(ch) < MAXLVLMORTAL)
	{
		tnl = new_exp_table[GET_LEVEL(ch) + 1] - GET_EXP(ch);
		if (tnl < 0)
			tnl = 0;
	}
	cJSON_AddNumberToObject(root, "tnl", tnl);

	/* All coin types */
	cJSON_AddNumberToObject(root, "platinum", GET_PLATINUM(ch));
	cJSON_AddNumberToObject(root, "gold", GET_GOLD(ch));
	cJSON_AddNumberToObject(root, "silver", GET_SILVER(ch));
	cJSON_AddNumberToObject(root, "copper", GET_COPPER(ch));

	/* Position - matches POS_ constants in defines.h */
	const char *positions[] = {"on your ass", "sitting", "kneeling", "standing"};
	int         pos         = GET_POS(ch);
	if (pos >= 0 && pos <= 3)
	{
		cJSON_AddStringToObject(root, "position", positions[pos]);
	}
	else
	{
		cJSON_AddStringToObject(root, "position", "standing");
	}

	/* Fighting target */
	if (IS_FIGHTING(ch) && GET_OPPONENT(ch) && GET_NAME(GET_OPPONENT(ch)))
	{
		cJSON_AddStringToObject(root, "fighting", GET_NAME(GET_OPPONENT(ch)));
	}
	else
	{
		cJSON_AddNullToObject(root, "fighting");
	}

	/* Whether character uses mana (Psionicist, Mindflayer, Illithid/Pillithid) */
	cJSON_AddBoolToObject(root, "usesMana", USES_MANA(ch) ? 1 : 0);

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build char.status gmcp message */
char *json_build_char_status(struct char_data *ch)
{
	cJSON      *root;
	char       *result;
	int         race;
	const char *class_name;

	if (!ch)
		return strdup("{}");

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", GET_NAME(ch) ? GET_NAME(ch) : "Unknown");
	cJSON_AddNumberToObject(root, "level", GET_LEVEL(ch));

	/* Use get_class_string which handles specialization and multiclass */
	char class_buffer[MAX_STRING_LENGTH];
	get_class_string(ch, class_buffer);
	cJSON_AddStringToObject(root, "class", class_buffer[0] ? class_buffer : "Unknown");

	race = (int)GET_RACE(ch);
	if (race >= 0 && race <= LAST_RACE)
	{
		cJSON_AddStringToObject(root, "race", race_names_table[race].normal);
	}
	else
	{
		cJSON_AddStringToObject(root, "race", "Unknown");
	}

	/* Alignment */
	if (IS_GOOD(ch))
	{
		cJSON_AddStringToObject(root, "alignment", "good");
	}
	else if (IS_EVIL(ch))
	{
		cJSON_AddStringToObject(root, "alignment", "evil");
	}
	else
	{
		cJSON_AddStringToObject(root, "alignment", "neutral");
	}

	/* Guild */
	if (GET_ASSOC(ch) && IS_MEMBER(GET_A_BITS(ch)))
	{
		cJSON_AddStringToObject(root, "guild", GET_ASSOC(ch)->get_name().c_str());
	}
	else
	{
		cJSON_AddStringToObject(root, "guild", "");
	}

	/* Title */
	if (GET_TITLE(ch))
	{
		cJSON_AddStringToObject(root, "title", GET_TITLE(ch));
	}
	else
	{
		cJSON_AddStringToObject(root, "title", "");
	}

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build char.affects gmcp message */
char *json_build_char_affects(struct char_data *ch)
{
	extern Skill          skills[];
	cJSON                *root, *affect_obj;
	struct affected_type *aff;
	char                 *result;
	int                   last = -1;

	root = cJSON_CreateArray();

	if (ch)
	{
		for (aff = ch->affected; aff; aff = aff->next)
		{
			/* Match score command logic for showing active spells */
			int is_herb = (aff->type >= HERB_OCULARIUS && aff->type <= HERB_GOOTWIET);
			if (aff->type <= 0 || !skills[aff->type].name || (aff->type > LAST_SKILL && !is_herb))
			{
				continue;
			}
			/* Skip affects marked as hidden */
			if (IS_SET(aff->flags, AFFTYPE_NOSHOW))
			{
				continue;
			}
			/* Skip duplicate affect types (spells can have multiple affect structs) */
			if (aff->type == last)
			{
				continue;
			}
			last = aff->type;

			affect_obj = cJSON_CreateObject();
			cJSON_AddStringToObject(affect_obj, "name", skills[aff->type].name);
			cJSON_AddNumberToObject(affect_obj, "id", aff->type);
			/* Send duration in seconds for frontend countdown timer */
			cJSON_AddNumberToObject(affect_obj, "duration", aff->duration * 60);
			cJSON_AddStringToObject(affect_obj, "icon", "shield"); /* Default icon */
			cJSON_AddItemToArray(root, affect_obj);
		}
	}

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build comm.channel gmcp message */
char *json_build_comm_channel(const char *channel, const char *sender, const char *text)
{
	cJSON *root;
	char  *result;
	time_t now;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "channel", channel ? channel : "info");
	cJSON_AddStringToObject(root, "sender", sender ? sender : "System");
	cJSON_AddStringToObject(root, "text", text ? text : "");

	time(&now);
	cJSON_AddNumberToObject(root, "timestamp", (double)now);

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build comm.channel gmcp message with alignment (for nchat) */
char *json_build_comm_channel_ex(const char *channel, const char *sender, const char *text, const char *alignment)
{
	cJSON *root;
	char  *result;
	time_t now;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "channel", channel ? channel : "info");
	cJSON_AddStringToObject(root, "sender", sender ? sender : "System");
	cJSON_AddStringToObject(root, "text", text ? text : "");

	if (alignment && *alignment)
	{
		cJSON_AddStringToObject(root, "alignment", alignment);
	}

	time(&now);
	cJSON_AddNumberToObject(root, "timestamp", (double)now);

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build stat roll response */
char *json_build_stat_roll(int *rolls, int *racial_mods, int *final, int rerolls)
{
	cJSON      *root, *data, *rolls_obj, *mods_obj, *final_obj;
	char       *result;
	const char *stats[] = {"str", "int", "wis", "dex", "con", "cha"};
	int         i;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", MSG_TYPE_CHARGEN);
	cJSON_AddStringToObject(root, "step", "stats");

	data = cJSON_CreateObject();

	rolls_obj = cJSON_CreateObject();
	mods_obj  = cJSON_CreateObject();
	final_obj = cJSON_CreateObject();

	for (i = 0; i < 6; i++)
	{
		cJSON_AddNumberToObject(rolls_obj, stats[i], rolls[i]);
		cJSON_AddNumberToObject(mods_obj, stats[i], racial_mods[i]);
		cJSON_AddNumberToObject(final_obj, stats[i], final[i]);
	}

	cJSON_AddItemToObject(data, "rolls", rolls_obj);
	cJSON_AddItemToObject(data, "racial_mods", mods_obj);
	cJSON_AddItemToObject(data, "final", final_obj);
	cJSON_AddNumberToObject(data, "rerolls_remaining", rerolls);

	cJSON_AddItemToObject(root, "data", data);

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build chargen complete response */
char *json_build_chargen_complete(const char *name, const char *message)
{
	cJSON *root, *data;
	char  *result;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", MSG_TYPE_CHARGEN);
	cJSON_AddStringToObject(root, "step", "complete");

	data = cJSON_CreateObject();
	cJSON_AddStringToObject(data, "name", name ? name : "");
	cJSON_AddStringToObject(data, "message", message ? message : "Character created!");

	cJSON_AddItemToObject(root, "data", data);

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build quest.status gmcp message for bartender quests */
char *json_build_quest_status(struct char_data *ch)
{
	cJSON            *root;
	char             *result;
	char              target_buf[MAX_STRING_LENGTH];
	struct char_data *q_mob = NULL;
	int               remaining;

	if (!ch || IS_NPC(ch))
		return NULL;

	root = cJSON_CreateObject();

	/* Get remaining quests for today */
	remaining = sql_world_quest_can_do_another(ch);
	cJSON_AddNumberToObject(root, "remaining", remaining);

	/* Check if quest is active */
	if (ch->only.pc->quest_active == 1)
	{
		cJSON_AddBoolToObject(root, "active", 1);

		/* Quest type */
		if (ch->only.pc->quest_type == FIND_AND_ASK)
		{
			cJSON_AddStringToObject(root, "type", "ask");
		}
		else if (ch->only.pc->quest_type == FIND_AND_KILL)
		{
			cJSON_AddStringToObject(root, "type", "kill");
			cJSON_AddNumberToObject(root, "killCount", ch->only.pc->quest_kill_how_many);
			cJSON_AddNumberToObject(root, "killRequired", ch->only.pc->quest_kill_original);
		}
		else
		{
			cJSON_AddNullToObject(root, "type");
		}

		/* Build target string - need to load mob to get name */
		q_mob = read_mobile(real_mobile(ch->only.pc->quest_mob_vnum), REAL);
		if (q_mob)
		{
			int zone_idx = real_zone0(ch->only.pc->quest_zone_number);

			if (ch->only.pc->quest_type == FIND_AND_ASK)
			{
				snprintf(target_buf, MAX_STRING_LENGTH, "Go ask %s in %s about the %s.", q_mob->player.short_descr, zone_table[zone_idx].name, month_name[time_info.month]);
			}
			else if (ch->only.pc->quest_type == FIND_AND_KILL)
			{
				int kills_left = ch->only.pc->quest_kill_original - ch->only.pc->quest_kill_how_many;
				snprintf(target_buf, MAX_STRING_LENGTH, "Go kill %d %s (%d left) in %s!", ch->only.pc->quest_kill_original, q_mob->player.short_descr, kills_left, zone_table[zone_idx].name);
			}
			else
			{
				snprintf(target_buf, MAX_STRING_LENGTH, "Unknown quest type");
			}

			extract_char(q_mob);
			q_mob = NULL;
		}
		else
		{
			snprintf(target_buf, MAX_STRING_LENGTH, "Quest target unavailable");
		}

		cJSON_AddStringToObject(root, "target", target_buf);
		cJSON_AddNumberToObject(root, "zoneNumber", ch->only.pc->quest_zone_number);
	}
	else
	{
		cJSON_AddBoolToObject(root, "active", 0);
		cJSON_AddNullToObject(root, "type");
		cJSON_AddStringToObject(root, "target", "");
	}

	/* Always include mapBought status */
	cJSON_AddBoolToObject(root, "mapBought", ch->only.pc->quest_map_bought == 1);

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

/* build ship.info gmcp message - static/slow-changing ship data */
char *json_build_ship_info(struct ShipData *ship, struct char_data *ch)
{
	cJSON      *root, *armor, *internal, *weapons, *cargo, *items;
	cJSON      *arc_arr;
	char       *result;
	int         i;
	const char *side_names[] = {"bow", "port", "stern", "starboard"};

	if (!ship)
		return strdup("{}");

	root = cJSON_CreateObject();

	/* basic info */
	cJSON_AddStringToObject(root, "name", ship->name ? ship->name : "Unknown");
	cJSON_AddStringToObject(root, "id", ship->id ? ship->id : "??");
	cJSON_AddStringToObject(root, "captain", ship->ownername ? ship->ownername : "");
	cJSON_AddNumberToObject(root, "class", ship->m_class);
	cJSON_AddNumberToObject(root, "frags", ship->frags);

	cJSON_AddStringToObject(root, "status", get_ship_status(ship));

	cJSON_AddNumberToObject(root, "maxSpeed", ship->get_maxspeed(ch));
	cJSON_AddNumberToObject(root, "contactRange", 35 + ship->crew.get_contact_range_mod());
	cJSON_AddNumberToObject(root, "sail", ship->mainsail);
	cJSON_AddNumberToObject(root, "maxSail", MAXSAIL);

	/* crew */
	cJSON_AddNumberToObject(root, "crewStamina", (int)ship->crew.stamina);
	cJSON_AddNumberToObject(root, "maxCrewStamina", (int)ship->crew.max_stamina);
	cJSON_AddNumberToObject(root, "repairStock", ship->repair);

	/* crew type and chiefs */
	cJSON_AddStringToObject(root, "crewType", ship_crew_data[ship->crew.index].name);

	cJSON *chiefs = cJSON_CreateObject();
	cJSON_AddStringToObject(chiefs, "sail", ship->crew.sail_chief > 0 ? ship_chief_data[ship->crew.sail_chief].name : "");
	cJSON_AddStringToObject(chiefs, "guns", ship->crew.guns_chief > 0 ? ship_chief_data[ship->crew.guns_chief].name : "");
	cJSON_AddStringToObject(chiefs, "repair", ship->crew.rpar_chief > 0 ? ship_chief_data[ship->crew.rpar_chief].name : "");
	cJSON_AddItemToObject(root, "chiefs", chiefs);

	/* crew skills */
	cJSON *skills = cJSON_CreateObject();
	cJSON_AddNumberToObject(skills, "sail", (int)ship->crew.sail_skill);
	cJSON_AddNumberToObject(skills, "guns", (int)ship->crew.guns_skill);
	cJSON_AddNumberToObject(skills, "repair", (int)ship->crew.rpar_skill);
	cJSON_AddItemToObject(root, "skills", skills);

	/* skill modifiers (from crew type + chiefs) */
	cJSON *skillMods = cJSON_CreateObject();
	cJSON_AddNumberToObject(skillMods, "sail", ship->crew.sail_mod());
	cJSON_AddNumberToObject(skillMods, "guns", ship->crew.guns_mod());
	cJSON_AddNumberToObject(skillMods, "repair", ship->crew.rpar_mod());
	cJSON_AddItemToObject(root, "skillMods", skillMods);

	/* people */
	cJSON_AddNumberToObject(root, "people", ship->people);
	cJSON_AddNumberToObject(root, "maxPeople", ship->get_capacity());

	/* armor - each arc is [current, max] */
	armor = cJSON_CreateObject();
	for (i = 0; i < 4; i++)
	{
		arc_arr = cJSON_CreateArray();
		cJSON_AddItemToArray(arc_arr, cJSON_CreateNumber(ship->armor[i]));
		cJSON_AddItemToArray(arc_arr, cJSON_CreateNumber(ship->maxarmor[i]));
		cJSON_AddItemToObject(armor, side_names[i], arc_arr);
	}
	cJSON_AddItemToObject(root, "armor", armor);

	/* internal hull - same pattern */
	internal = cJSON_CreateObject();
	for (i = 0; i < 4; i++)
	{
		arc_arr = cJSON_CreateArray();
		cJSON_AddItemToArray(arc_arr, cJSON_CreateNumber(ship->internal[i]));
		cJSON_AddItemToArray(arc_arr, cJSON_CreateNumber(ship->maxinternal[i]));
		cJSON_AddItemToObject(internal, side_names[i], arc_arr);
	}
	cJSON_AddItemToObject(root, "internal", internal);

	/* weapons array */
	weapons = cJSON_CreateArray();
	for (i = 0; i < MAXSLOTS; i++)
	{
		if (ship->slot[i].type == SLOT_WEAPON)
		{
			cJSON *w     = cJSON_CreateObject();
			int    w_idx = ship->slot[i].index;

			cJSON_AddNumberToObject(w, "slot", i);
			cJSON_AddStringToObject(w, "name", weapon_data[w_idx].name ? weapon_data[w_idx].name : "Unknown");

			/* position */
			const char *pos = "hold";
			if (ship->slot[i].position >= 0 && ship->slot[i].position < 4)
			{
				pos = side_names[ship->slot[i].position];
			}
			cJSON_AddStringToObject(w, "position", pos);

			/* ammo: val1 = current ammo */
			cJSON_AddNumberToObject(w, "ammo", ship->slot[i].val1);
			cJSON_AddNumberToObject(w, "maxAmmo", weapon_data[w_idx].ammo);

			/* damage: val2 = damage level (0 = ok, higher = worse) */
			cJSON_AddNumberToObject(w, "damage", ship->slot[i].val2);

			/* ready: timer == 0 means ready to fire */
			cJSON_AddBoolToObject(w, "ready", ship->slot[i].timer == 0 ? 1 : 0);

			cJSON_AddItemToArray(weapons, w);
		}
	}
	cJSON_AddItemToObject(root, "weapons", weapons);

	/* equipment array */
	cJSON *equipment = cJSON_CreateArray();
	for (i = 0; i < MAXSLOTS; i++)
	{
		if (ship->slot[i].type == SLOT_EQUIPMENT)
		{
			cJSON *e     = cJSON_CreateObject();
			int    e_idx = ship->slot[i].index;

			cJSON_AddNumberToObject(e, "slot", i);
			cJSON_AddStringToObject(e, "name", equipment_data[e_idx].name ? equipment_data[e_idx].name : "Unknown");
			cJSON_AddBoolToObject(e, "ready", ship->slot[i].timer == 0 ? 1 : 0);

			cJSON_AddItemToArray(equipment, e);
		}
	}
	cJSON_AddItemToObject(root, "equipment", equipment);

	/* cargo */
	cargo           = cJSON_CreateObject();
	int cargo_total = 0;
	items           = cJSON_CreateArray();

	for (i = 0; i < MAXSLOTS; i++)
	{
		if (ship->slot[i].type == SLOT_CARGO || ship->slot[i].type == SLOT_CONTRABAND)
		{
			cJSON *c = cJSON_CreateObject();
			cJSON_AddNumberToObject(c, "slot", i);
			cJSON_AddStringToObject(c, "name", cargo_type_name(ship->slot[i].index));
			cJSON_AddNumberToObject(c, "crates", ship->slot[i].val0);
			cJSON_AddNumberToObject(c, "invoicePrice", ship->slot[i].val1);
			cJSON_AddBoolToObject(c, "contraband", ship->slot[i].type == SLOT_CONTRABAND ? 1 : 0);
			cJSON_AddItemToArray(items, c);
			cargo_total += ship->slot[i].val0;
		}
	}

	cJSON_AddNumberToObject(cargo, "current", cargo_total);
	cJSON_AddNumberToObject(cargo, "max", SHIPTYPE_CARGO(ship->m_class));
	cJSON_AddItemToObject(cargo, "items", items);
	cJSON_AddItemToObject(root, "cargo", cargo);

	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}
