/*
 * ws_handlers.c - websocket command handlers for durismud
 *
 * handles json commands from web clients: login, register, enter, game, etc.
 */

#include "prototypes.h"
#include "creation_availability_config.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "ws_handlers.h"
#include <ctype.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "account.h"
#include "defines.h"
#include "files.h"
#include "hardcore_config.h"
#include "json_utils.h"
#include "mm.h"
#include "poll.h"
#include "sql.h"
#include "sql_player.h"
#include "player_name.h"
#include "websocket.h"

extern struct descriptor_data  *descriptor_list;
extern struct mm_ds            *dead_mob_pool;
extern struct mm_ds            *dead_pconly_pool;
extern struct room_data        *world;
extern int                      top_of_world;
extern int                      is_bcrypt_hash(const char *hash);
extern int                      bcrypt_verify_password(const char *password, const char *hash);
extern char                    *bcrypt_hash_password(const char *password);
extern bool                     account_exists(const char *dir, char *name);
extern int                      read_account(P_acct acct);
extern int                      write_account(P_acct acct);
extern int                      is_valid_email(const char *email);
extern bool                     is_email_taken(const char *email);
extern int                      restoreCharOnly(P_char ch, char *name);
extern const struct class_names class_names_table[];
extern const struct race_names  race_names_table[];
extern int                      class_table[LAST_RACE + 1][CLASS_COUNT + 1];
extern void                     roll_basic_attributes(P_char ch, int type);
extern const struct stat_data   stat_factor[];
extern const char              *stat_to_string2(int val);
extern const char              *town_name_list[];
extern const int                avail_hometowns[][LAST_RACE + 1];

/* forward declarations for helpers used by broadcast functions */
static const char *ws_get_race_name(int race);
static const char *ws_get_class_name(unsigned int m_class);

/* durisweb secret for service authentication */
#define DURISWEB_SECRET_DEFAULT "Dur1sM4pK3y2025xYz!"

static const char *get_durisweb_secret(void)
{
	const char *secret = getenv("DURISWEB_SECRET");
	if (!secret || !*secret)
	{
		static int warned = 0;
		if (!warned)
		{
			logit(LOG_DEBUG, "WARNING: DURISWEB_SECRET not set, using default (honeypot)");
			warned = 1;
		}
		return DURISWEB_SECRET_DEFAULT;
	}
	return secret;
}

static int verify_durisweb_sig(const char *sig)
{
	if (!sig || !*sig)
		return 0;

	const char *secret = get_durisweb_secret();
	if (!secret)
		return 0;

	time_t now    = time(NULL);
	long   minute = now / 60;

	/* check +/- 1 minute for clock skew */
	for (int offset = -1; offset <= 1; offset++)
	{
		char ts[32];
		snprintf(ts, sizeof(ts), "%ld", minute + offset);

		unsigned char digest[EVP_MAX_MD_SIZE];
		unsigned int  digest_len = 0;

		HMAC(EVP_sha256(), secret, strlen(secret), (unsigned char *)ts, strlen(ts), digest, &digest_len);

		/* sha256 = 32 bytes */
		if (digest_len != 32)
			continue;

		char expected[65];
		for (unsigned int i = 0; i < digest_len && i < 32; i++)
		{
			snprintf(expected + (i * 2), 3, "%02x", digest[i]);
		}
		expected[64] = '\0';

		if (strcmp(sig, expected) == 0)
		{
			return 1;
		}
	}
	return 0;
}

/* send auth response helper */
static void send_auth_response(struct descriptor_data *d, int success, const char *error)
{
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", "durisweb_auth");
	cJSON_AddBoolToObject(root, "success", success);
	if (error)
		cJSON_AddStringToObject(root, "error", error);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}
	cJSON_Delete(root);
}

/* durisweb service authentication */
void ws_cmd_durisweb_auth(struct descriptor_data *d, cJSON *data)
{
	cJSON *sig = cJSON_GetObjectItem(data, "sig");

	if (!sig || !cJSON_IsString(sig))
	{
		send_auth_response(d, 0, "Missing signature");
		return;
	}

	if (verify_durisweb_sig(sig->valuestring))
	{
		d->durisweb_verified = 1;
		d->durisweb_backend  = 1;
		statuslog(56, "DurisWeb service authenticated");
		send_auth_response(d, 1, NULL);
	}
	else
	{
		send_auth_response(d, 0, "Invalid signature");
	}
}

/* broadcast auction new to durisweb service */
void ws_broadcast_auction_new(int auction_id, const char *seller_name, const char *obj_short, int cur_price, int buy_price, int end_time)
{
	struct descriptor_data *d;
	cJSON                  *root, *data;
	char                   *json;

	root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "auction_new");

	data = cJSON_CreateObject();
	cJSON_AddNumberToObject(data, "id", auction_id);
	cJSON_AddStringToObject(data, "seller", seller_name ? seller_name : "");
	cJSON_AddStringToObject(data, "item", obj_short ? obj_short : "");
	cJSON_AddNumberToObject(data, "price", cur_price);
	cJSON_AddNumberToObject(data, "buyPrice", buy_price);
	cJSON_AddNumberToObject(data, "endTime", end_time);
	cJSON_AddItemToObject(root, "data", data);

	json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->websocket && d->durisweb_verified)
		{
			websocket_send_text(d, json);
		}
	}

	free(json);
}

/* broadcast auction bid to durisweb service */
void ws_broadcast_auction_bid(int auction_id, const char *bidder_name, int bid_amount, int prev_bidder_pid, const char *prev_bidder_name)
{
	struct descriptor_data *d;
	cJSON                  *root, *data;
	char                   *json;

	root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "auction_bid");

	data = cJSON_CreateObject();
	cJSON_AddNumberToObject(data, "id", auction_id);
	cJSON_AddStringToObject(data, "bidder", bidder_name ? bidder_name : "");
	cJSON_AddNumberToObject(data, "amount", bid_amount);
	cJSON_AddNumberToObject(data, "prevBidderPid", prev_bidder_pid);
	cJSON_AddStringToObject(data, "prevBidder", prev_bidder_name ? prev_bidder_name : "");
	cJSON_AddItemToObject(root, "data", data);

	json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

    for (d = descriptor_list; d; d = d->next) {
        if (d->websocket && d->durisweb_backend) {
            websocket_send_text(d, json);
        }
    }

	free(json);
}

/* broadcast auction close to durisweb service */
void ws_broadcast_auction_close(int auction_id, const char *winner_name, int winner_pid, int final_price, const char *close_reason, int seller_pid, const char *seller_name)
{
	struct descriptor_data *d;
	cJSON                  *root, *data;
	char                   *json;

	root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "auction_close");

	data = cJSON_CreateObject();
	cJSON_AddNumberToObject(data, "id", auction_id);
	cJSON_AddStringToObject(data, "winner", winner_name ? winner_name : "");
	cJSON_AddNumberToObject(data, "winnerPid", winner_pid);
	cJSON_AddNumberToObject(data, "price", final_price);
	cJSON_AddStringToObject(data, "reason", close_reason ? close_reason : "sold");
	cJSON_AddNumberToObject(data, "sellerPid", seller_pid);
	cJSON_AddStringToObject(data, "seller", seller_name ? seller_name : "");
	cJSON_AddItemToObject(root, "data", data);

	json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->websocket && d->durisweb_backend)
		{
			websocket_send_text(d, json);
		}
	}

	free(json);
}

/* broadcast mud shutdown to durisweb service */
void ws_broadcast_mud_shutdown(const char *type)
{
	struct descriptor_data *d;
	cJSON                  *root, *data;
	char                   *json;

	root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "mud_shutdown");

	data = cJSON_CreateObject();
	cJSON_AddStringToObject(data, "shutdownType", type ? type : "unknown");
	cJSON_AddItemToObject(root, "data", data);

	json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->websocket && d->durisweb_verified)
		{
			websocket_send_text(d, json);
		}
	}

	free(json);
}

/* broadcast player login to durisweb service */
void ws_broadcast_player_login(struct descriptor_data *player_d) {
    struct descriptor_data *d;
    cJSON *root, *data;
    char *json;

    if (!player_d || !player_d->character) return;

    root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddStringToObject(root, "type", "player_login");

    data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "character", GET_NAME(player_d->character));
    cJSON_AddStringToObject(data, "account", player_d->account ? player_d->account->acct_name : "");
    cJSON_AddStringToObject(data, "ip", player_d->host);
    cJSON_AddNumberToObject(data, "level", GET_LEVEL(player_d->character));
    cJSON_AddStringToObject(data, "race", ws_get_race_name(GET_RACE(player_d->character)));
    cJSON_AddStringToObject(data, "class", ws_get_class_name(player_d->character->player.m_class));
    cJSON_AddNumberToObject(data, "faction", GET_RACEWAR(player_d->character));
    cJSON_AddStringToObject(data, "client", player_d->client_name);
    cJSON_AddStringToObject(data, "clientVersion", player_d->client_version);
    cJSON_AddItemToObject(root, "data", data);

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return;

    for (d = descriptor_list; d; d = d->next) {
        if (d->websocket && d->durisweb_backend) {
            websocket_send_text(d, json);
        }
    }
    free(json);
}

/* broadcast player logout to durisweb service */
void ws_broadcast_player_logout(const char *character, int faction) {
    struct descriptor_data *d;
    cJSON *root, *data;
    char *json;

    if (!character) return;

    root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddStringToObject(root, "type", "player_logout");

    data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "character", character);
    cJSON_AddNumberToObject(data, "faction", faction);
    cJSON_AddItemToObject(root, "data", data);

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return;

    for (d = descriptor_list; d; d = d->next) {
        if (d->websocket && d->durisweb_backend) {
            websocket_send_text(d, json);
        }
    }
    free(json);
}

/* send wholist to a specific client (for backend requests) */
static void ws_send_wholist_to_client(struct descriptor_data *d) {
    struct descriptor_data *target;
    cJSON *root, *data, *players, *player;
    char *json;

    root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddStringToObject(root, "type", "wholist");

    data = cJSON_CreateObject();
    players = cJSON_CreateArray();

    for (target = descriptor_list; target; target = target->next) {
        if (target->connected != CON_PLAYING || !target->character)
            continue;
        if (d->character && !who_visible_to(d->character, target->character))
            continue;

        player = cJSON_CreateObject();
        {
            char display_name[MAX_STRING_LENGTH];
            cJSON_AddStringToObject(player, "character", who_display_name(d->character, target->character, display_name, sizeof(display_name)));
        }
        cJSON_AddStringToObject(player, "account", target->account ? target->account->acct_name : "");
        cJSON_AddStringToObject(player, "ip", target->host);
        cJSON_AddNumberToObject(player, "level", GET_LEVEL(target->character));
        cJSON_AddStringToObject(player, "race", ws_get_race_name(GET_RACE(target->character)));
        cJSON_AddStringToObject(player, "class", ws_get_class_name(target->character->player.m_class));
        cJSON_AddNumberToObject(player, "faction", GET_RACEWAR(target->character));
        cJSON_AddStringToObject(player, "client", target->client_name);
        cJSON_AddStringToObject(player, "clientVersion", target->client_version);
        cJSON_AddNumberToObject(player, "uptime", time(0) - target->character->player.time.logon);
        cJSON_AddItemToArray(players, player);
    }

    cJSON_AddItemToObject(data, "players", players);
    cJSON_AddItemToObject(root, "data", data);

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return;

    websocket_send_text(d, json);
    free(json);
}

/* handle request_wholist from backend only */
static void ws_cmd_request_wholist(struct descriptor_data *d, cJSON *data) {
    (void)data;
    if (!d->durisweb_verified) {
        ws_send_system(d, "error", "not authorized");
        return;
    }
    ws_send_wholist_to_client(d);
}

/* helper structure for character display */
struct ws_char_info
{
	char name[32];
	int  level;
	int  race;
	int  hometown;       /* Room index for last room name */
	char class_str[256]; /* full class string (e.g., "cleric / zealot") */
};

static cJSON *ws_build_character_list(struct descriptor_data *d);

/* get race faction from playable_races[] array */
static const char *ws_get_race_faction(int race)
{
	int i;
	for (i = 0; playable_races[i].race_id != -1; i++)
	{
		if (playable_races[i].race_id == race)
		{
			return playable_races[i].faction;
		}
	}
	return "unknown";
}

/* check if race is playable */
static int ws_is_playable_race(int race)
{
	int i;
	for (i = 0; playable_races[i].race_id != -1; i++)
	{
		if (playable_races[i].race_id == race)
			return creation_race_enabled(race) ? 1 : 0;
	}
	return 0;
}

/* get alignment string from class_table value */
static const char *ws_get_class_alignment(int value)
{
	switch (value)
	{
		case -1:
			return "evil";
		case 0:
			return "neutral";
		case 1:
			return "good";
		case 2:
			return "any";
		case 3:
			return "good_neutral";
		case 4:
			return "neutral_evil";
		default:
			return NULL; /* forbidden */
	}
}

/* load basic character info for json response */
static int ws_load_char_info(const char *charname, struct ws_char_info *info)
{
	extern char *get_class_string(P_char ch, char *strn);
	P_char       temp_ch;
	int          result;

	temp_ch = (struct char_data *)malloc(sizeof(struct char_data));
	if (!temp_ch)
		return 0;

	memset(temp_ch, 0, sizeof(struct char_data));

	temp_ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
	if (!temp_ch->only.pc)
	{
		free(temp_ch);
		return 0;
	}

	memset(temp_ch->only.pc, 0, sizeof(struct pc_only_data));

	result = restoreCharOnly(temp_ch, (char *)charname);
	if (result < 0)
	{
		if (temp_ch->only.pc)
			free(temp_ch->only.pc);
		free(temp_ch);
		return 0;
	}

	strncpy(info->name, GET_NAME(temp_ch), 31);
	info->name[31] = '\0';
	/* capitalize first letter */
	if (info->name[0])
		info->name[0] = toupper(info->name[0]);

	info->level    = GET_LEVEL(temp_ch);
	info->race     = GET_RACE(temp_ch);
	info->hometown = GET_HOME(temp_ch);

	/* use get_class_string - handles spec and multiclass */
	get_class_string(temp_ch, info->class_str);

	cleanup_temp_char(temp_ch);
	if (temp_ch->only.pc)
		free(temp_ch->only.pc);
	free(temp_ch);

	return 1;
}

/* get race name string with ansi colors */
static const char *ws_get_race_name(int race)
{
	extern const struct race_names race_names_table[];
	if (race >= 0)
	{
		return race_names_table[race].ansi;
	}
	return "Unknown";
}

/* get class name string with ansi colors */
static const char *ws_get_class_name(unsigned int m_class)
{
	int idx = flag2idx(m_class);
	if (idx >= 0)
	{
		return class_names_table[idx].ansi;
	}
	return "Unknown";
}

/* send auth success message with character list */
void ws_send_auth_success(struct descriptor_data *d, const char *account_name)
{
	cJSON *root = cJSON_CreateObject();
	cJSON *data = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "type", "auth");
	cJSON_AddStringToObject(root, "status", "success");

	cJSON_AddStringToObject(data, "account", account_name);

	/* build character list */
	cJSON_AddItemToObject(data, "characters", ws_build_character_list(d));
	cJSON_AddItemToObject(root, "data", data);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(root);
}

/* send reconnect success message for linkdead reconnection */
void ws_send_reconnect_success(struct descriptor_data *d, const char *account_name, const char *char_name)
{
	cJSON *root      = cJSON_CreateObject();
	cJSON *data      = cJSON_CreateObject();
	cJSON *character = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "type", "auth");
	cJSON_AddStringToObject(root, "status", "reconnected");

	cJSON_AddStringToObject(data, "account", account_name);

	/* add reconnected character info */
	if (d->character)
	{
		cJSON_AddStringToObject(character, "name", GET_NAME(d->character));
		cJSON_AddNumberToObject(character, "level", GET_LEVEL(d->character));
		cJSON_AddStringToObject(character, "race", ws_get_race_name(GET_RACE(d->character)));
		cJSON_AddStringToObject(character, "class", ws_get_class_name(d->character->player.m_class));
	}
	else
	{
		cJSON_AddStringToObject(character, "name", char_name);
	}

	cJSON_AddItemToObject(data, "character", character);
	cJSON_AddItemToObject(root, "data", data);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(root);
}

/* send full game state after reconnection to resync client */
void ws_send_full_game_state(struct descriptor_data *d)
{
	if (!d || !d->character)
		return;

	/* trigger gmcp updates to send current state */
	if (d->character->in_room >= 0 && d->ws_handshake_done)
	{
		extern void gmcp_room_info(struct char_data * ch);
		extern void gmcp_room_map(struct char_data * ch);
		gmcp_room_info(d->character);
		gmcp_room_map(d->character);
	}

	extern void gmcp_char_vitals(struct char_data * ch);
	extern void gmcp_char_status(struct char_data * ch);
	extern void gmcp_char_affects(struct char_data * ch);
	extern void gmcp_quest_status(struct char_data * ch);

	/* Guard GMCP sends behind WS handshake to prevent leaking
	 * frames into the login stream before handshake completes */
	if (d->ws_handshake_done)
	{
		gmcp_char_vitals(d->character);
		gmcp_char_status(d->character);
		gmcp_char_affects(d->character);
		gmcp_quest_status(d->character);
	}

	/* send a "look" to show the room */
	write_to_q("look", &d->input, 0);
}

/* send auth failed message */
void ws_send_auth_failed(struct descriptor_data *d, const char *error)
{
	cJSON *root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "type", "auth");
	cJSON_AddStringToObject(root, "status", "failed");
	cJSON_AddStringToObject(root, "error", error);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(root);
}

/* send text message */
void ws_send_text(struct descriptor_data *d, const char *category, const char *text)
{
	cJSON *root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "type", "text");
	cJSON_AddStringToObject(root, "category", category);
	cJSON_AddStringToObject(root, "data", text);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(root);
}

/* send system message */
void ws_send_system(struct descriptor_data *d, const char *status, const char *message)
{
	cJSON *root = cJSON_CreateObject();
	cJSON *data = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "type", "system");
	cJSON_AddStringToObject(data, "status", status);
	cJSON_AddStringToObject(data, "message", message);
	cJSON_AddItemToObject(root, "data", data);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(root);
}

/* handle login command */
void ws_cmd_login(struct descriptor_data *d, cJSON *data)
{
	cJSON      *account_item, *password_item;
	const char *account_name, *password;
	char        tmp_name[256];
	int         password_valid = 0;

	if (!data)
	{
		ws_send_auth_failed(d, "Missing login data");
		return;
	}

	account_item  = cJSON_GetObjectItem(data, "account");
	password_item = cJSON_GetObjectItem(data, "password");

	if (!account_item || !cJSON_IsString(account_item))
	{
		ws_send_auth_failed(d, "Missing account name");
		return;
	}

	if (!password_item || !cJSON_IsString(password_item))
	{
		ws_send_auth_failed(d, "Missing password");
		return;
	}

	account_name = account_item->valuestring;
	password     = password_item->valuestring;

	/* Validate account name */
	if (strlen(account_name) < 3 || strlen(account_name) > 20)
	{
		ws_send_auth_failed(d, "Invalid account name");
		return;
	}

	/* lowercase for filesystem lookup */
	strncpy(tmp_name, account_name, sizeof(tmp_name) - 1);
	tmp_name[sizeof(tmp_name) - 1] = '\0';
	for (int i = 0; tmp_name[i]; i++)
	{
		tmp_name[i] = tolower(tmp_name[i]);
	}

	/* check if account exists */
	if (!account_exists("Accounts", tmp_name))
	{
		ws_send_auth_failed(d, "Account not found");
		return;
	}

	/* allocate and load account - if one exists, free it first */
	if (d->account)
	{
		d->account = free_account(d->account);
	}
	d->account = allocate_account();
	if (!d->account)
	{
		ws_send_auth_failed(d, "Failed to allocate account");
		return;
	}

	d->account->acct_name = str_dup(tmp_name);

	if (read_account(d->account) == -1)
	{
		ws_send_auth_failed(d, "Error loading account");
		d->account = free_account(d->account);
		return;
	}

	/* verify password */
	if (is_bcrypt_hash(d->account->acct_password))
	{
		password_valid = bcrypt_verify_password(password, d->account->acct_password);
	}
	else
	{
		/* legacy md5 hash */
		password_valid = (strcmp(CRYPT2((char *)password, d->account->acct_password), d->account->acct_password) == 0);
	}

	if (!password_valid)
	{
		ws_send_auth_failed(d, "Invalid password");
		d->account = free_account(d->account);
		return;
	}

	/* reconnect check: look for in-game characters from this account */
	{
		extern struct char_data *get_char_online(char *name, bool include_linkdead);
		struct descriptor_data  *k, *next_k;
		struct acct_chars       *c;
		struct char_data        *online_char = NULL;

		/* search through account's characters to find one in-game */
		if (d->account && d->account->acct_character_list)
		{
			c = d->account->acct_character_list;
			while (c)
			{
				online_char = get_char_online(c->charname, 1); /* include linkdead */
				if (online_char)
				{
					statuslog(56, "WebSocket: Found in-game character %s for account %s (linkdead=%s)", GET_NAME(online_char), tmp_name, online_char->desc ? "no" : "yes");
					break;
				}
				c = c->next;
			}
		}

		/* kick any duplicate sessions in character selection */
		for (k = descriptor_list; k; k = next_k)
		{
			next_k = k->next;
			if (k == d)
				continue;

			if (k->websocket && k->account && k->account->acct_name && strcasecmp(k->account->acct_name, tmp_name) == 0 && k->connected != CON_PLAYING)
			{

				statuslog(56, "WebSocket: Kicking duplicate account session for %s from %s (new login from %s)", tmp_name, k->host, d->host);

				ws_send_system(k, "kicked", "Another session has logged in with this account.");
				websocket_close(k, WS_CLOSE_NORMAL, "Duplicate session");
				close_socket(k);
			}
		}

		/* if we found an in-game character, reconnect to it */
		if (online_char)
		{
			struct descriptor_data *old_desc = online_char->desc;

			/* close old descriptor if exists */
			if (old_desc && old_desc != d)
			{
				old_desc->character = NULL;

				if (old_desc->websocket)
				{
					ws_send_system(old_desc, "kicked", "Reconnected from another session.");
					websocket_close(old_desc, WS_CLOSE_NORMAL, "Reconnected");
				}
				close_socket(old_desc);
			}

			/* attach character to new descriptor */
			d->character      = online_char;
			online_char->desc = d;
			d->connected      = CON_PLAYING;

			statuslog(56, "WebSocket: Reconnected %s to character %s from %s", tmp_name, GET_NAME(d->character), d->host);

			ws_send_reconnect_success(d, tmp_name, GET_NAME(d->character));
			ws_send_full_game_state(d);

			return;
		}
	}

	/* success - show character selection */
	d->connected = CON_ACCT_SELECT_CHAR;
	statuslog(56, "WebSocket login success for account: %s from %s", tmp_name, d->host);

	ws_send_auth_success(d, tmp_name);
}

/* handle enter game command */
void ws_cmd_enter(struct descriptor_data *d, cJSON *data)
{
	cJSON                  *char_item;
	const char             *char_name;
	struct acct_chars      *c;
	struct descriptor_data *k, *next_k;

	/* prevent duplicate entry if already entering or playing */
	if (d->connected == CON_ACCT_CONFIRM_CHAR || d->connected == CON_PLAYING)
	{
		return;
	}

	if (!data)
	{
		ws_send_text(d, "system", "Missing character data");
		return;
	}

	char_item = cJSON_GetObjectItem(data, "character");
	if (!char_item || !cJSON_IsString(char_item))
	{
		ws_send_text(d, "system", "Missing character name");
		return;
	}

	char_name = char_item->valuestring;

	if (!d->account || !d->account->acct_character_list)
	{
		ws_send_text(d, "system", "No characters available");
		return;
	}

	/* find character in account list */
	c = d->account->acct_character_list;
	while (c)
	{
		if (strcasecmp(c->charname, char_name) == 0)
		{
			break;
		}
		c = c->next;
	}

	if (!c)
	{
		ws_send_text(d, "system", "Character not found");
		return;
	}

	/* duplicate session check: kick old session if character already logged in */
	for (k = descriptor_list; k; k = next_k)
	{
		next_k = k->next;

		/* skip self */
		if (k == d)
			continue;

		/* check if this descriptor has the same character */
		if (k->character && GET_NAME(k->character) && strcasecmp(GET_NAME(k->character), char_name) == 0)
		{

			statuslog(56, "WebSocket: Kicking duplicate session for %s from %s (new connection from %s)", char_name, k->host, d->host);

			/* notify old client */
			if (k->websocket)
			{
				ws_send_system(k, "kicked", "Another session has connected with this character.");
				websocket_close(k, WS_CLOSE_NORMAL, "Duplicate session");
			}

			close_socket(k);
		}
		/* also check pending character selection */
		else if (k->selected_char_name && strcasecmp(k->selected_char_name, char_name) == 0)
		{

			statuslog(56, "WebSocket: Kicking pending session for %s from %s (new connection from %s)", char_name, k->host, d->host);

			if (k->websocket)
			{
				ws_send_system(k, "kicked", "Another session has connected with this character.");
				websocket_close(k, WS_CLOSE_NORMAL, "Duplicate session");
			}

			close_socket(k);
		}
	}

	/* store selection and use nanny flow to enter game */
	if (d->selected_char_name)
	{
		str_free(d->selected_char_name);
	}
	d->selected_char_name = str_dup(c->charname);

	/* queue 'y' to confirm character selection */
	write_to_q("y", &d->input, 0);
	d->connected = CON_ACCT_CONFIRM_CHAR;
}

/* handle game command */
void ws_cmd_game(struct descriptor_data *d, cJSON *data)
{
	const char *cmd;

	if (!data)
		return;

	if (cJSON_IsString(data))
	{
		cmd = data->valuestring;
	}
	else
	{
		cJSON *cmd_item = cJSON_GetObjectItem(data, "command");
		if (cmd_item && cJSON_IsString(cmd_item))
		{
			cmd = cmd_item->valuestring;
		}
		else
		{
			return;
		}
	}

	if (cmd && *cmd)
	{
		write_to_q(cmd, &d->input, 0);
	}
}

/* handle register command - create a new account */
void ws_cmd_register(struct descriptor_data *d, cJSON *data)
{
	cJSON *account_json, *password_json, *email_json;
	char   tmp_name[MAX_INPUT_LENGTH];
	char  *hash;
	int    i;

	/* get required fields from json */
	account_json  = cJSON_GetObjectItemCaseSensitive(data, "account");
	password_json = cJSON_GetObjectItemCaseSensitive(data, "password");
	email_json    = cJSON_GetObjectItemCaseSensitive(data, "email");

	if (!cJSON_IsString(account_json) || !account_json->valuestring || !cJSON_IsString(password_json) || !password_json->valuestring || !cJSON_IsString(email_json) || !email_json->valuestring)
	{
		ws_send_auth_failed(d, "Missing required fields: account, password, email");
		return;
	}

	/* validate account name length */
	if (strlen(account_json->valuestring) < 3)
	{
		ws_send_auth_failed(d, "Account name must be at least 3 characters");
		return;
	}

	if (strlen(account_json->valuestring) > 14)
	{
		ws_send_auth_failed(d, "Account name must be 14 characters or less");
		return;
	}

	/* copy and normalize account name */
	strncpy(tmp_name, account_json->valuestring, MAX_INPUT_LENGTH - 1);
	tmp_name[MAX_INPUT_LENGTH - 1] = '\0';

	/* convert to lowercase except first char */
	tmp_name[0] = toupper(tmp_name[0]);
	for (i = 1; tmp_name[i]; i++)
	{
		tmp_name[i] = tolower(tmp_name[i]);
	}

	/* validate account name characters */
	for (i = 0; tmp_name[i]; i++)
	{
		if (!isalpha(tmp_name[i]) && tmp_name[i] != '_')
		{
			ws_send_auth_failed(d, "Account name can only contain letters and underscores");
			return;
		}
	}

	/* check if account already exists */
	if (account_exists("Accounts", tmp_name))
	{
		ws_send_auth_failed(d, "An account with that name already exists");
		return;
	}

	/* validate email format */
	if (!is_valid_email(email_json->valuestring))
	{
		ws_send_auth_failed(d, "Invalid email address format");
		return;
	}

	/* check if email is already in use */
	if (is_email_taken(email_json->valuestring))
	{
		ws_send_auth_failed(d, "Email address is already in use");
		return;
	}

	/* validate password length */
	if (strlen(password_json->valuestring) < 6)
	{
		ws_send_auth_failed(d, "Password must be at least 6 characters");
		return;
	}

	/* allocate new account - if one exists, free it first to avoid memory leaks */
	if (d->account)
	{
		d->account = free_account(d->account);
	}
	d->account = allocate_account();
	if (!d->account)
	{
		ws_send_auth_failed(d, "Failed to create account - server error");
		statuslog(56, "&+RALERT&n: WebSocket could not allocate account for %s", tmp_name);
		return;
	}

	/* set account name and email */
	d->account->acct_name  = str_dup(tmp_name);
	d->account->acct_email = str_dup(email_json->valuestring);

	/* hash password with bcrypt */
	hash = bcrypt_hash_password(password_json->valuestring);
	if (!hash)
	{
		ws_send_auth_failed(d, "Failed to hash password - server error");
		d->account = free_account(d->account);
		return;
	}
	d->account->acct_password = str_dup(hash);
	FREE(hash);

	/* mark account as confirmed (skip email verification for web clients) */
	d->account->acct_confirmed = 1;

	/* save account to disk */
	if (write_account(d->account) == -1)
	{
		ws_send_auth_failed(d, "Failed to save account - server error");
		statuslog(56, "&+RALERT&n: WebSocket failed to write account for %s", tmp_name);
		d->account = free_account(d->account);
		return;
	}

	statuslog(56, "WebSocket: New account created: %s", tmp_name);

	ws_send_auth_success(d, "registered");
}

/* handle chargen options request */
void ws_cmd_chargen_options(struct descriptor_data *d, cJSON *data)
{
	cJSON      *response, *races_array, *race_obj, *classes_array, *class_obj;
	int         i, j, race_id, align_val;
	const char *align_str;
	char       *json_str;

	response = cJSON_CreateObject();
	if (!response)
	{
		ws_send_system(d, "error", "Failed to create chargen options");
		return;
	}

	cJSON_AddStringToObject(response, "type", "chargen_options");
	races_array = cJSON_AddArrayToObject(response, "races");

	/* loop over playable_races[] array */
	for (i = 0; playable_races[i].race_id != -1; i++)
	{
		race_id = playable_races[i].race_id;
		if (!creation_race_enabled(race_id))
			continue;

		race_obj = cJSON_CreateObject();
		cJSON_AddNumberToObject(race_obj, "id", race_id);
		cJSON_AddStringToObject(race_obj, "name", race_names_table[race_id].normal);
		cJSON_AddStringToObject(race_obj, "ansi", race_names_table[race_id].ansi);
		cJSON_AddStringToObject(race_obj, "faction", playable_races[i].faction);

		/* build array of available classes for this race */
		classes_array = cJSON_AddArrayToObject(race_obj, "classes");

		for (j = 1; j <= CLASS_COUNT; j++)
		{
			align_val = class_table[race_id][j];

			/* skip forbidden classes */
			if (!creation_class_enabled(j) || align_val == 5)
				continue;

			align_str = ws_get_class_alignment(align_val);
			if (!align_str)
				continue;

			class_obj = cJSON_CreateObject();
			cJSON_AddNumberToObject(class_obj, "id", j);
			cJSON_AddStringToObject(class_obj, "name", class_names_table[j].normal);
			cJSON_AddStringToObject(class_obj, "ansi", class_names_table[j].ansi);
			cJSON_AddStringToObject(class_obj, "alignment", align_str);

			cJSON_AddItemToArray(classes_array, class_obj);
		}

		cJSON_AddItemToArray(races_array, race_obj);
	}

	json_str = cJSON_PrintUnformatted(response);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(response);
}

/* helper: build chargen stats as cJSON object */
static cJSON *build_chargen_stats_json(stat_data *stats)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddStringToObject(obj, "str", stat_to_string2(stats->Str));
	cJSON_AddStringToObject(obj, "dex", stat_to_string2(stats->Dex));
	cJSON_AddStringToObject(obj, "agi", stat_to_string2(stats->Agi));
	cJSON_AddStringToObject(obj, "con", stat_to_string2(stats->Con));
	cJSON_AddStringToObject(obj, "pow", stat_to_string2(stats->Pow));
	cJSON_AddStringToObject(obj, "int", stat_to_string2(stats->Int));
	cJSON_AddStringToObject(obj, "wis", stat_to_string2(stats->Wis));
	cJSON_AddStringToObject(obj, "cha", stat_to_string2(stats->Cha));
	cJSON_AddStringToObject(obj, "luk", stat_to_string2(stats->Luk));
	cJSON_AddStringToObject(obj, "kar", stat_to_string2(stats->Kar));
	return obj;
}

/* helper: map stat name to pointer in chargen_stats (kar not modifiable) */
static sh_int *get_chargen_stat_ptr(stat_data *stats, const char *name)
{
	if (strcmp(name, "str") == 0)
		return &stats->Str;
	if (strcmp(name, "dex") == 0)
		return &stats->Dex;
	if (strcmp(name, "agi") == 0)
		return &stats->Agi;
	if (strcmp(name, "con") == 0)
		return &stats->Con;
	if (strcmp(name, "pow") == 0)
		return &stats->Pow;
	if (strcmp(name, "int") == 0)
		return &stats->Int;
	if (strcmp(name, "wis") == 0)
		return &stats->Wis;
	if (strcmp(name, "cha") == 0)
		return &stats->Cha;
	if (strcmp(name, "luk") == 0)
		return &stats->Luk;
	return NULL;
}

/* handle roll stats command */
void ws_cmd_roll_stats(struct descriptor_data *d, cJSON *data)
{
	cJSON *response;
	cJSON *race_item;
	int    race_id;
	char  *json_str;
	P_char temp_ch;

	/* get race from request */
	race_item = cJSON_GetObjectItem(data, "race");
	if (!race_item || !cJSON_IsNumber(race_item))
	{
		ws_send_system(d, "error", "Missing or invalid race");
		return;
	}
	race_id = race_item->valueint;

	/* validate race is playable */
	if (!ws_is_playable_race(race_id))
	{
		ws_send_system(d, "error", "Invalid race selection");
		return;
	}

	/* create temporary character for stat rolling */
	temp_ch = (struct char_data *)malloc(sizeof(struct char_data));
	if (!temp_ch)
	{
		ws_send_system(d, "error", "Server error: memory allocation failed");
		return;
	}
	memset(temp_ch, 0, sizeof(struct char_data));

	/* set race and roll stats */
	GET_RACE(temp_ch) = race_id;
	roll_basic_attributes(temp_ch, 0);

	/* store rolled stats in descriptor for later use */
	d->chargen_stats           = temp_ch->base_stats;
	d->chargen_race            = race_id;
	d->chargen_bonus_remaining = 5;

	/* build response - send only quality labels, not numbers */
	response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "type", "roll_stats");

	cJSON_AddItemToObject(response, "stats", build_chargen_stats_json(&temp_ch->base_stats));

	cJSON_AddNumberToObject(response, "bonusRemaining", d->chargen_bonus_remaining);

	free(temp_ch);

	json_str = cJSON_PrintUnformatted(response);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(response);
}

/* handle add bonus command - adds +5 to specified stat */
void ws_cmd_add_bonus(struct descriptor_data *d, cJSON *data)
{
	cJSON      *response, *stat_item;
	char       *json_str;
	const char *stat_name;
	sh_int     *stat_ptr = NULL;

	/* check if we have bonus points remaining */
	if (d->chargen_bonus_remaining <= 0)
	{
		ws_send_system(d, "error", "No bonus points remaining");
		return;
	}

	/* get stat to boost */
	stat_item = cJSON_GetObjectItem(data, "stat");
	if (!stat_item || !cJSON_IsString(stat_item))
	{
		ws_send_system(d, "error", "Missing or invalid stat");
		return;
	}
	stat_name = stat_item->valuestring;

	/* map stat name to pointer (9 stats can receive bonus - not kar) */
	stat_ptr = get_chargen_stat_ptr(&d->chargen_stats, stat_name);
	if (!stat_ptr)
	{
		ws_send_system(d, "error", "Invalid stat name");
		return;
	}

	/* check if stat is already at max */
	if (*stat_ptr >= 100)
	{
		ws_send_system(d, "error", "Stat is already at maximum");
		return;
	}

	/* add bonus (+5, capped at 100) */
	*stat_ptr = BOUNDED(1, *stat_ptr + 5, 100);
	d->chargen_bonus_remaining--;

	/* build response with updated stats */
	response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "type", "bonus_added");

	cJSON_AddItemToObject(response, "stats", build_chargen_stats_json(&d->chargen_stats));
	cJSON_AddNumberToObject(response, "bonusRemaining", d->chargen_bonus_remaining);
	cJSON_AddStringToObject(response, "boostedStat", stat_name);

	json_str = cJSON_PrintUnformatted(response);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(response);
}

/* handle swap stats command - swaps values of two stats */
void ws_cmd_swap_stats(struct descriptor_data *d, cJSON *data)
{
	cJSON      *response, *stat1_item, *stat2_item;
	char       *json_str;
	const char *stat1_name, *stat2_name;
	sh_int     *stat1_ptr = NULL, *stat2_ptr = NULL;
	sh_int      temp;

	/* get stat names */
	stat1_item = cJSON_GetObjectItem(data, "stat1");
	stat2_item = cJSON_GetObjectItem(data, "stat2");
	if (!stat1_item || !cJSON_IsString(stat1_item) || !stat2_item || !cJSON_IsString(stat2_item))
	{
		ws_send_system(d, "error", "Missing stat names for swap");
		return;
	}
	stat1_name = stat1_item->valuestring;
	stat2_name = stat2_item->valuestring;

	/* can't swap same stat */
	if (strcmp(stat1_name, stat2_name) == 0)
	{
		ws_send_system(d, "error", "Cannot swap a stat with itself");
		return;
	}

	/* map stat names to pointers (9 stats swappable - not kar) */
	stat1_ptr = get_chargen_stat_ptr(&d->chargen_stats, stat1_name);
	if (!stat1_ptr)
	{
		ws_send_system(d, "error", "Invalid first stat name");
		return;
	}

	stat2_ptr = get_chargen_stat_ptr(&d->chargen_stats, stat2_name);
	if (!stat2_ptr)
	{
		ws_send_system(d, "error", "Invalid second stat name");
		return;
	}

	/* perform the swap */
	temp       = *stat1_ptr;
	*stat1_ptr = *stat2_ptr;
	*stat2_ptr = temp;

	/* build response with updated stats */
	response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "type", "stats_swapped");

	cJSON_AddItemToObject(response, "stats", build_chargen_stats_json(&d->chargen_stats));
	cJSON_AddStringToObject(response, "swapped1", stat1_name);
	cJSON_AddStringToObject(response, "swapped2", stat2_name);

	json_str = cJSON_PrintUnformatted(response);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(response);
}

/* handle create character command */
void ws_cmd_create_character(struct descriptor_data *d, cJSON *data)
{
	cJSON      *name_item, *race_item, *class_item, *sex_item, *align_item;
	cJSON      *hometown_item, *hardcore_item, *newbie_item;
	cJSON      *response;
	char       *json_str;
	const char *name;
	int         race_id, class_id, sex, alignment;
	int         hometown_id, is_hardcore, is_newbie;
	int         class_align_req;
	const char *faction;

	/* validate required fields */
	name_item     = cJSON_GetObjectItem(data, "name");
	race_item     = cJSON_GetObjectItem(data, "race");
	class_item    = cJSON_GetObjectItem(data, "class");
	sex_item      = cJSON_GetObjectItem(data, "sex");
	hometown_item = cJSON_GetObjectItem(data, "hometown");
	hardcore_item = cJSON_GetObjectItem(data, "hardcore");
	newbie_item   = cJSON_GetObjectItem(data, "newbie");

	if (!name_item || !cJSON_IsString(name_item))
	{
		ws_send_system(d, "error", "Missing or invalid character name");
		return;
	}
	if (!race_item || !cJSON_IsNumber(race_item))
	{
		ws_send_system(d, "error", "Missing or invalid race");
		return;
	}
	if (!class_item || !cJSON_IsNumber(class_item))
	{
		ws_send_system(d, "error", "Missing or invalid class");
		return;
	}
	if (!sex_item || !cJSON_IsNumber(sex_item))
	{
		ws_send_system(d, "error", "Missing or invalid sex");
		return;
	}

	name     = name_item->valuestring;
	race_id  = race_item->valueint;
	class_id = class_item->valueint;
	sex      = sex_item->valueint;

	/* parse optional fields */
	hometown_id = hometown_item && cJSON_IsNumber(hometown_item) ? hometown_item->valueint : -1;
	is_hardcore = hardcore_item && cJSON_IsBool(hardcore_item) ? cJSON_IsTrue(hardcore_item) : 0;
	is_newbie   = newbie_item && cJSON_IsBool(newbie_item) ? cJSON_IsTrue(newbie_item) : 1;

	/* veterans only can be hardcore */
	if (!hardcore_config_get()->creation_enabled ||
	    (hardcore_config_get()->creation_veterans_only && is_newbie && is_hardcore))
	{
		is_hardcore = 0;
	}

	/* validate name length */
	if (strlen(name) < 2 || strlen(name) > 12)
	{
		ws_send_system(d, "error", "Name must be 2-12 characters");
		return;
	}

	/* validate race is playable */
	if (!ws_is_playable_race(race_id))
	{
		ws_send_system(d, "error", "Invalid race selection");
		return;
	}

	/* validate class is valid for race */
	if (class_id < 1 || class_id > CLASS_COUNT)
	{
		ws_send_system(d, "error", "Invalid class selection");
		return;
	}

	class_align_req = class_table[race_id][class_id];
	if (!creation_class_enabled(class_id) || class_align_req == 5)
	{
		ws_send_system(d, "error", "That class is not available for your race");
		return;
	}

	/* validate sex */
	if (sex < 1 || sex > 2)
	{
		ws_send_system(d, "error", "Invalid sex selection");
		return;
	}

	/* check alignment for neutral races */
	faction = ws_get_race_faction(race_id);
	if (strcmp(faction, "neutral") == 0)
	{
		align_item = cJSON_GetObjectItem(data, "alignment");
		if (!align_item || !cJSON_IsString(align_item))
		{
			ws_send_system(d, "error", "Neutral races must choose an alignment");
			return;
		}
		if (strcmp(align_item->valuestring, "good") == 0)
		{
			alignment = 1; /* good */
		}
		else if (strcmp(align_item->valuestring, "evil") == 0)
		{
			alignment = -1; /* evil */
		}
		else
		{
			ws_send_system(d, "error", "Invalid alignment selection");
			return;
		}
	}
	else if (strcmp(faction, "good") == 0)
	{
		alignment = 1;
	}
	else
	{
		alignment = -1;
	}

	/* validate class alignment requirement */
	if (class_align_req == 1 && alignment != 1)
	{
		ws_send_system(d, "error", "That class requires good alignment");
		return;
	}
	if (class_align_req == -1 && alignment != -1)
	{
		ws_send_system(d, "error", "That class requires evil alignment");
		return;
	}

	/* store chargen options in descriptor */
	d->chargen_hometown = hometown_id;
	d->chargen_hardcore = is_hardcore;
	d->chargen_newbie   = is_newbie;

	/* actual character creation */
	extern bool pfile_exists(const char *dir, char *name);
	extern void clear_char(P_char ch);
	extern void setCharPhysTypeInfo(P_char ch);
	extern void init_char(P_char ch);
	extern void add_char_to_account(P_desc d);
	extern int  writeCharacter(P_char ch, int type, int room);
	extern void enter_game(P_desc d);
	extern int  find_hometown(int race, bool force);

	char   capitalized_name[MAX_NAME_LENGTH + 1];
	int    i, actual_hometown;
	P_char ch;

	/* capitalize name */
	strncpy(capitalized_name, name, MAX_NAME_LENGTH);
	capitalized_name[MAX_NAME_LENGTH] = '\0';
	capitalized_name[0]               = toupper(capitalized_name[0]);
	for (i = 1; capitalized_name[i]; i++)
	{
		capitalized_name[i] = tolower(capitalized_name[i]);
	}

	/* check if name already exists */
	if (sql_player_exists(capitalized_name))
	{
		cJSON *err = cJSON_CreateObject();
		cJSON_AddStringToObject(err, "type", "create_character");
		cJSON_AddStringToObject(err, "status", "error");
		cJSON_AddStringToObject(err, "message", "That name is already in use");
		char *err_str = cJSON_PrintUnformatted(err);
		if (err_str)
		{
			websocket_send_text(d, err_str);
			free(err_str);
		}
		cJSON_Delete(err);
		return;
	}
	if (pfile_exists(BADNAME_DIR, capitalized_name))
	{
		cJSON *err = cJSON_CreateObject();
		cJSON_AddStringToObject(err, "type", "create_character");
		cJSON_AddStringToObject(err, "status", "error");
		cJSON_AddStringToObject(err, "message", "That name has been declined");
		char *err_str = cJSON_PrintUnformatted(err);
		if (err_str)
		{
			websocket_send_text(d, err_str);
			free(err_str);
		}
		cJSON_Delete(err);
		return;
	}

	/* allocate character structure if not exists */
	if (!d->character)
	{
		d->character = (struct char_data *)mm_get(dead_mob_pool);
		clear_char(d->character);
		ensure_pconly_pool();
		d->character->only.pc             = (struct pc_only_data *)mm_get(dead_pconly_pool);
		d->character->only.pc->aggressive = -1;
		d->character->desc                = d;
		setCharPhysTypeInfo(d->character);
	}

	ch = d->character;

	/* set character name */
	if (ch->player.name)
	{
		str_free(ch->player.name);
	}
	ch->player.name = str_dup(capitalized_name);
	normalize_player_name_case(ch->player.name);

	/* set race, sex, class */
	GET_RACE(ch)       = race_id;
	ch->player.sex     = sex;
	ch->player.m_class = 1 << (class_id - 1);

	/* set alignment (1000 for good, -1000 for evil) */
	GET_ALIGNMENT(ch) = (alignment == 1) ? 1000 : -1000;

	/* set racewar based on race and alignment */
	if (OLD_RACE_GOOD(race_id, GET_ALIGNMENT(ch)))
	{
		GET_RACEWAR(ch) = RACEWAR_GOOD;
	}
	else if (OLD_RACE_EVIL(race_id, GET_ALIGNMENT(ch)))
	{
		GET_RACEWAR(ch) = RACEWAR_EVIL;
	}
	else if (OLD_RACE_PUNDEAD(race_id))
	{
		GET_RACEWAR(ch) = RACEWAR_UNDEAD;
	}
	else if (IS_HARPY(ch))
	{
		GET_RACEWAR(ch) = RACEWAR_NEUTRAL;
	}

	/* set hometown */
	if (hometown_id < 0 || hometown_id > LAST_HOME)
	{
		actual_hometown = find_hometown(race_id, false);
		if (actual_hometown == HOME_CHOICE)
		{
			/* race has multiple choices - pick first available */
			for (i = 0; i <= LAST_HOME; i++)
			{
				if (avail_hometowns[i][race_id] == 1)
				{
					actual_hometown = i;
					break;
				}
			}
		}
	}
	else
	{
		actual_hometown = hometown_id;
	}
	GET_HOME(ch)            = actual_hometown;
	GET_BIRTHPLACE(ch)      = actual_hometown;
	GET_ORIG_BIRTHPLACE(ch) = actual_hometown;

	/* copy stats from descriptor (already rolled with bonuses/swaps applied) */
	ch->base_stats = d->chargen_stats;
	ch->curr_stats = d->chargen_stats;

	/* level stays at 0 - matching telnet behavior for new characters */

	/* set hardcore/newbie flags */
	if (is_newbie)
	{
		SET_BIT(ch->specials.act2, PLR2_NEWBIE);
	}
	if (is_hardcore)
	{
		SET_BIT(ch->specials.act2, PLR2_HARDCORE_CHAR);
	}

	/* initialize character (sets pid, skills, hp/mana/vitality, etc.) */
	init_char(ch);

	/* copy account password to character */
	strncpy(ch->only.pc->pwd, d->account->acct_password, sizeof(ch->only.pc->pwd) - 1);
	ch->only.pc->pwd[sizeof(ch->only.pc->pwd) - 1] = '\0';

#ifdef USE_ACCOUNT
	add_char_to_account(d);
#endif

	/* save character to disk */
	writeCharacter(ch, RENT_QUIT, NOWHERE);

	logit(LOG_NEW, "%s [%s] new WebSocket player.", GET_NAME(ch), d->host);
	statuslog(ch->player.level, "%s [%s] new WebSocket player.", GET_NAME(ch), d->host);

	/* set connection state before entering game */
	STATE(d) = CON_PLAYING;
	enter_game(d);
	d->prompt_mode = TRUE;

	/* send full game state via gmcp */
	ws_send_full_game_state(d);

	/* send success response */
	response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "type", "create_character");
	cJSON_AddStringToObject(response, "status", "created");
	cJSON_AddStringToObject(response, "message", "Character created successfully!");
	cJSON_AddStringToObject(response, "name", capitalized_name);
	cJSON_AddStringToObject(response, "race", race_names_table[race_id].normal);
	cJSON_AddStringToObject(response, "class", class_names_table[class_id].normal);
	cJSON_AddStringToObject(response, "faction", (alignment == 1) ? "good" : "evil");
	cJSON_AddBoolToObject(response, "hardcore", is_hardcore ? cJSON_True : cJSON_False);
	cJSON_AddBoolToObject(response, "newbie", is_newbie ? cJSON_True : cJSON_False);
	if (actual_hometown >= 0 && actual_hometown <= LAST_HOME)
	{
		cJSON_AddStringToObject(response, "hometown", town_name_list[actual_hometown]);
	}

	json_str = cJSON_PrintUnformatted(response);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(response);
}

/* validate character name - check if already taken */
void ws_cmd_validate_name(struct descriptor_data *d, cJSON *data)
{
	extern bool pfile_exists(const char *dir, char *name);
	cJSON      *name_item, *response;
	char       *json_str;
	const char *name;
	char        capitalized_name[MAX_NAME_LENGTH + 1];
	int         i;

	name_item = cJSON_GetObjectItem(data, "name");
	if (!name_item || !cJSON_IsString(name_item))
	{
		ws_send_system(d, "error", "Missing character name");
		return;
	}

	name = name_item->valuestring;

	/* validate name length */
	if (strlen(name) < 2)
	{
		response = cJSON_CreateObject();
		cJSON_AddStringToObject(response, "type", "validate_name");
		cJSON_AddBoolToObject(response, "valid", 0);
		cJSON_AddStringToObject(response, "message", "Name must be at least 2 characters");
		goto send_response;
	}
	if (strlen(name) > 12)
	{
		response = cJSON_CreateObject();
		cJSON_AddStringToObject(response, "type", "validate_name");
		cJSON_AddBoolToObject(response, "valid", 0);
		cJSON_AddStringToObject(response, "message", "Name must be at most 12 characters");
		goto send_response;
	}

	/* validate name contains only letters */
	for (i = 0; name[i]; i++)
	{
		if (!isalpha(name[i]))
		{
			response = cJSON_CreateObject();
			cJSON_AddStringToObject(response, "type", "validate_name");
			cJSON_AddBoolToObject(response, "valid", 0);
			cJSON_AddStringToObject(response, "message", "Name can only contain letters");
			goto send_response;
		}
	}

	/* capitalize name for pfile_exists */
	strncpy(capitalized_name, name, MAX_NAME_LENGTH);
	capitalized_name[MAX_NAME_LENGTH] = '\0';
	capitalized_name[0]               = toupper(capitalized_name[0]);
	for (i = 1; capitalized_name[i]; i++)
	{
		capitalized_name[i] = tolower(capitalized_name[i]);
	}

	statuslog(56, "WS validate_name: checking '%s' in SAVE_DIR='%s'", capitalized_name, SAVE_DIR);

	/* check if player file exists */
	if (sql_player_exists(capitalized_name))
	{
		statuslog(56, "WS validate_name: '%s' EXISTS - returning invalid", capitalized_name);
		response = cJSON_CreateObject();
		cJSON_AddStringToObject(response, "type", "validate_name");
		cJSON_AddBoolToObject(response, "valid", 0);
		cJSON_AddStringToObject(response, "message", "Name is already in use");
		goto send_response;
	}

	/* check badname directory */
	if (pfile_exists(BADNAME_DIR, capitalized_name))
	{
		response = cJSON_CreateObject();
		cJSON_AddStringToObject(response, "type", "validate_name");
		cJSON_AddBoolToObject(response, "valid", 0);
		cJSON_AddStringToObject(response, "message", "That name is not allowed");
		goto send_response;
	}

	/* name is valid and available */
	statuslog(56, "WS validate_name: '%s' is available", capitalized_name);
	response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "type", "validate_name");
	cJSON_AddBoolToObject(response, "valid", 1);
	cJSON_AddStringToObject(response, "message", "Name is available");

send_response:
	json_str = cJSON_PrintUnformatted(response);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}
	cJSON_Delete(response);
}

/* get available hometowns for a race */
void ws_cmd_get_hometowns(struct descriptor_data *d, cJSON *data)
{
	cJSON *race_item, *response, *options, *option;
	char  *json_str;
	int    race_id, i, count = 0;

	race_item = cJSON_GetObjectItem(data, "race");
	if (!race_item || !cJSON_IsNumber(race_item))
	{
		ws_send_system(d, "error", "Missing race ID");
		return;
	}

	race_id = race_item->valueint;

	if (race_id < 1 || race_id > LAST_RACE)
	{
		ws_send_system(d, "error", "Invalid race ID");
		return;
	}

	response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "type", "hometowns");
	cJSON_AddNumberToObject(response, "race", race_id);

	options = cJSON_CreateArray();

	/* find available hometowns for this race */
	for (i = 0; i <= LAST_HOME; i++)
	{
		if (avail_hometowns[i][race_id] == 1)
		{
			option = cJSON_CreateObject();
			cJSON_AddNumberToObject(option, "id", i);
			cJSON_AddStringToObject(option, "name", town_name_list[i]);
			cJSON_AddItemToArray(options, option);
			count++;
		}
	}

	cJSON_AddItemToObject(response, "options", options);
	cJSON_AddNumberToObject(response, "count", count);
	/* if count == 1, frontend can skip selection */
	cJSON_AddBoolToObject(response, "hasChoice", count > 1 ? cJSON_True : cJSON_False);

	json_str = cJSON_PrintUnformatted(response);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}
	cJSON_Delete(response);
}

/* === account menu websocket handlers === */

/* send account message (generic helper) */
static void ws_send_account_message(struct descriptor_data *d, const char *action, cJSON *data_obj, const char *error)
{
	cJSON *root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "type", "account");
	cJSON_AddStringToObject(root, "action", action);

	if (data_obj)
	{
		cJSON_AddItemToObject(root, "data", data_obj);
	}
	if (error)
	{
		cJSON_AddStringToObject(root, "error", error);
	}

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(root);
}

/* build character list json array */
static cJSON *ws_build_character_list(struct descriptor_data *d)
{
	cJSON             *characters = cJSON_CreateArray();
	struct acct_chars *c;

	if (d->account && d->account->acct_character_list)
	{
		c = d->account->acct_character_list;
		while (c)
		{
			struct ws_char_info info;
			if (ws_load_char_info(c->charname, &info))
			{
				cJSON *char_obj = cJSON_CreateObject();
				cJSON_AddStringToObject(char_obj, "name", info.name);
				cJSON_AddNumberToObject(char_obj, "level", info.level);
				cJSON_AddStringToObject(char_obj, "race", ws_get_race_name(info.race));
				cJSON_AddStringToObject(char_obj, "class", info.class_str);

				/* last room name */
				if (info.hometown >= 0 && info.hometown < top_of_world && world[info.hometown].name)
				{
					cJSON_AddStringToObject(char_obj, "lastRoom", world[info.hometown].name);
				}
				else
				{
					cJSON_AddStringToObject(char_obj, "lastRoom", "Unknown");
				}

				cJSON_AddItemToArray(characters, char_obj);
			}
			c = c->next;
		}
	}

	return characters;
}

/* get extended account information */
void ws_cmd_account_info(struct descriptor_data *d, cJSON *data)
{
	extern char       *get_class_string(P_char ch, char *strn);
	cJSON             *info_data, *characters, *char_obj;
	char               time_buf[64];
	char               class_str[256];
	char               name_cap[32];
	struct acct_chars *c;
	P_char             temp_ch;
	long               total_playtime = 0;
	int                immortal_level = 0;

	if (!d->account)
	{
		ws_send_account_message(d, "error", NULL, "Not logged in");
		return;
	}

	info_data = cJSON_CreateObject();

	cJSON_AddStringToObject(info_data, "name", d->account->acct_name);
	cJSON_AddStringToObject(info_data, "email", d->account->acct_email ? d->account->acct_email : "");
	cJSON_AddStringToObject(info_data, "created", "unknown");

	/* last login */
	if (d->account->acct_last > 0)
	{
		strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&d->account->acct_last));
		cJSON_AddStringToObject(info_data, "lastLogin", time_buf);
	}
	else
	{
		cJSON_AddStringToObject(info_data, "lastLogin", "never");
	}

	/* single pass: collect playtime, immortal level, and character list */
	characters = cJSON_CreateArray();
	c          = d->account->acct_character_list;
	while (c)
	{
		temp_ch = (struct char_data *)malloc(sizeof(struct char_data));
		if (temp_ch)
		{
			memset(temp_ch, 0, sizeof(struct char_data));
			temp_ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
			if (temp_ch->only.pc)
			{
				memset(temp_ch->only.pc, 0, sizeof(struct pc_only_data));
				if (restoreCharOnly(temp_ch, c->charname) >= 0)
				{
					int level = GET_LEVEL(temp_ch);

					/* accumulate playtime */
					total_playtime += temp_ch->player.time.played;

					/* track highest immortal level */
					if (level >= 57 && level > immortal_level)
					{
						immortal_level = level;
					}

					/* build character json object */
					strncpy(name_cap, GET_NAME(temp_ch), 31);
					name_cap[31] = '\0';
					if (name_cap[0])
						name_cap[0] = toupper(name_cap[0]);

					get_class_string(temp_ch, class_str);

					char_obj = cJSON_CreateObject();
					cJSON_AddStringToObject(char_obj, "name", name_cap);
					cJSON_AddNumberToObject(char_obj, "level", level);
					cJSON_AddStringToObject(char_obj, "race", ws_get_race_name(GET_RACE(temp_ch)));
					cJSON_AddStringToObject(char_obj, "class", class_str);

					/* last room name */
					int hometown = GET_HOME(temp_ch);
					if (hometown >= 0 && hometown < top_of_world && world[hometown].name)
					{
						cJSON_AddStringToObject(char_obj, "lastRoom", world[hometown].name);
					}
					else
					{
						cJSON_AddStringToObject(char_obj, "lastRoom", "Unknown");
					}

					cJSON_AddItemToArray(characters, char_obj);
				}
				cleanup_temp_char(temp_ch);
				free(temp_ch->only.pc);
			}
			free(temp_ch);
		}
		c = c->next;
	}

	cJSON_AddNumberToObject(info_data, "totalPlaytime", total_playtime);
	cJSON_AddNumberToObject(info_data, "immortalLevel", immortal_level);
	cJSON_AddItemToObject(info_data, "characters", characters);

	ws_send_account_message(d, "info", info_data, NULL);
}

/* change account email */
void ws_cmd_change_email(struct descriptor_data *d, cJSON *data)
{
	cJSON      *new_email_json;
	const char *new_email;
	cJSON      *result_data;

	if (!d->account)
	{
		ws_send_account_message(d, "error", NULL, "Not logged in");
		return;
	}

	if (!data)
	{
		ws_send_account_message(d, "error", NULL, "Missing data");
		return;
	}

	new_email_json = cJSON_GetObjectItem(data, "newEmail");
	if (!new_email_json || !cJSON_IsString(new_email_json))
	{
		ws_send_account_message(d, "error", NULL, "Missing newEmail field");
		return;
	}

	new_email = new_email_json->valuestring;

	/* validate email format */
	if (!is_valid_email(new_email))
	{
		ws_send_account_message(d, "error", NULL, "Invalid email format");
		return;
	}

	/* check if email is already taken */
	if (is_email_taken(new_email))
	{
		/* allow keeping the same email */
		if (!d->account->acct_email || strcasecmp(new_email, d->account->acct_email) != 0)
		{
			ws_send_account_message(d, "error", NULL, "Email already in use");
			return;
		}
	}

	/* update email */
	if (d->account->acct_email)
	{
		FREE(d->account->acct_email);
	}
	d->account->acct_email = str_dup(new_email);
	if (-1 == write_account(d->account))
	{
		ws_send_account_message(d, "error", NULL, "Failed to save email change");
		statuslog(56, "&+RALERT&n: failed to save email change for %s", d->account->acct_name);
		persistence_alert(AVATAR, "account", d->account->acct_name, "none", "none", "write_failed", "email change save failed");
		return;
	}

	statuslog(56, "Account %s changed email to %s", d->account->acct_name, new_email);

	result_data = cJSON_CreateObject();
	cJSON_AddStringToObject(result_data, "email", new_email);
	ws_send_account_message(d, "email_changed", result_data, NULL);
}

/* change account password */
void ws_cmd_change_password(struct descriptor_data *d, cJSON *data)
{
	cJSON      *current_json, *new_json;
	const char *current_password, *new_password;
	char       *hash;

	if (!d->account)
	{
		ws_send_account_message(d, "error", NULL, "Not logged in");
		return;
	}

	if (!data)
	{
		ws_send_account_message(d, "error", NULL, "Missing data");
		return;
	}

	current_json = cJSON_GetObjectItem(data, "currentPassword");
	new_json     = cJSON_GetObjectItem(data, "newPassword");

	if (!current_json || !cJSON_IsString(current_json) || !new_json || !cJSON_IsString(new_json))
	{
		ws_send_account_message(d, "error", NULL, "Missing password fields");
		return;
	}

	current_password = current_json->valuestring;
	new_password     = new_json->valuestring;

	/* verify current password */
	if (!bcrypt_verify_password(current_password, d->account->acct_password))
	{
		ws_send_account_message(d, "error", NULL, "Current password incorrect");
		return;
	}

	/* validate new password */
	if (strlen(new_password) < 6)
	{
		ws_send_account_message(d, "error", NULL, "Password must be at least 6 characters");
		return;
	}

	/* hash new password */
	hash = bcrypt_hash_password(new_password);
	if (!hash)
	{
		ws_send_account_message(d, "error", NULL, "Failed to hash password");
		return;
	}

	/* update password */
	if (d->account->acct_password)
	{
		FREE(d->account->acct_password);
	}
	d->account->acct_password = str_dup(hash);
	FREE(hash);
	if (-1 == write_account(d->account))
	{
		ws_send_account_message(d, "error", NULL, "Failed to save password change");
		statuslog(56, "&+RALERT&n: failed to save password change for %s", d->account->acct_name);
		persistence_alert(AVATAR, "account", d->account->acct_name, "none", "none", "write_failed", "password change save failed");
		return;
	}

	statuslog(56, "Account %s changed password", d->account->acct_name);

	ws_send_account_message(d, "password_changed", NULL, NULL);
}

/* delete a character */
void ws_cmd_delete_character(struct descriptor_data *d, cJSON *data)
{
	cJSON             *name_json, *confirm_json;
	const char        *char_name;
	struct acct_chars *c, *prev;
	P_char             ch;
	cJSON             *result_data;

	if (!d->account)
	{
		ws_send_account_message(d, "error", NULL, "Not logged in");
		return;
	}

	if (!data)
	{
		ws_send_account_message(d, "error", NULL, "Missing data");
		return;
	}

	name_json    = cJSON_GetObjectItem(data, "name");
	confirm_json = cJSON_GetObjectItem(data, "confirm");

	if (!name_json || !cJSON_IsString(name_json))
	{
		ws_send_account_message(d, "error", NULL, "Missing character name");
		return;
	}

	if (!confirm_json || !cJSON_IsTrue(confirm_json))
	{
		ws_send_account_message(d, "error", NULL, "Deletion not confirmed");
		return;
	}

	char_name = name_json->valuestring;

	/* find character in account list */
	c    = d->account->acct_character_list;
	prev = NULL;
	while (c)
	{
		if (strcasecmp(c->charname, char_name) == 0)
		{
			break;
		}
		prev = c;
		c    = c->next;
	}

	if (!c)
	{
		ws_send_account_message(d, "error", NULL, "Character not found");
		return;
	}

	/* load character for deletion */
	ch = (struct char_data *)malloc(sizeof(struct char_data));
	if (!ch)
	{
		ws_send_account_message(d, "error", NULL, "Failed to load character");
		return;
	}

	memset(ch, 0, sizeof(struct char_data));
	ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
	if (!ch->only.pc)
	{
		free(ch);
		ws_send_account_message(d, "error", NULL, "Failed to load character");
		return;
	}

	memset(ch->only.pc, 0, sizeof(struct pc_only_data));

	if (restoreCharOnly(ch, (char *)char_name) < 0)
	{
		free(ch->only.pc);
		free(ch);
		ws_send_account_message(d, "error", NULL, "Failed to load character file");
		return;
	}

	/* log the deletion */
	statuslog(ch->player.level, "%s deleted %s via web client (%s@%s).", d->account->acct_name, char_name, d->login, d->host);
	logit(LOG_PLAYER, "%s deleted %s via web client (%s@%s).", d->account->acct_name, char_name, d->login, d->host);

	if (!deleteCharacter(ch))
	{
		ws_send_account_message(d, "error", NULL, "Failed to delete character database records");
		return;
	}

	/* free strings allocated by restoreCharOnly */
	if (ch->player.name)
		str_free(ch->player.name);
	if (ch->player.title)
		str_free(ch->player.title);
	if (ch->player.short_descr)
		str_free(ch->player.short_descr);
	if (ch->player.long_descr)
		str_free(ch->player.long_descr);
	if (ch->player.description)
		str_free(ch->player.description);
	if (ch->only.pc->poofIn)
		str_free(ch->only.pc->poofIn);
	if (ch->only.pc->poofOut)
		str_free(ch->only.pc->poofOut);
	if (ch->only.pc->poofInSound)
		str_free(ch->only.pc->poofInSound);
	if (ch->only.pc->poofOutSound)
		str_free(ch->only.pc->poofOutSound);
	if (ch->only.pc->gcmd_arr)
		FREE(ch->only.pc->gcmd_arr);

	free(ch->only.pc);
	free(ch);

	/* remove from account character list */
	if (prev)
	{
		prev->next = c->next;
	}
	else
	{
		d->account->acct_character_list = c->next;
	}
	FREE(c->charname);
	FREE(c);
	d->account->num_chars--;

	if (-1 == write_account(d->account))
	{
		statuslog(56, "&+RALERT&n: failed to save deleted-character account update for %s", d->account->acct_name);
		persistence_alert(AVATAR, "account", d->account->acct_name, "none", "none", "write_failed", "character deletion save failed");
	}

	/* send success with updated character list */
	result_data = cJSON_CreateObject();
	cJSON_AddStringToObject(result_data, "name", char_name);
	cJSON_AddItemToObject(result_data, "characters", ws_build_character_list(d));
	ws_send_account_message(d, "character_deleted", result_data, NULL);
}

/* helper to send admin_delete_character progress update */
static void ws_send_admin_delete_progress(struct descriptor_data *d, const char *request_id, const char *message, const char *status)
{
	cJSON *result = cJSON_CreateObject();
	cJSON_AddStringToObject(result, "type", "admin_delete_progress");
	if (request_id)
		cJSON_AddStringToObject(result, "requestId", request_id);
	cJSON_AddStringToObject(result, "message", message);
	cJSON_AddStringToObject(result, "status", status); /* "info", "success", "error" */

	char *json_str = cJSON_PrintUnformatted(result);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}
	cJSON_Delete(result);
}

/* helper to send admin_delete_character response with requestId */
static void ws_send_admin_delete_response(struct descriptor_data *d, int success, const char *account, const char *name, const char *request_id, const char *error)
{
	cJSON *result = cJSON_CreateObject();
	cJSON_AddStringToObject(result, "type", "admin_delete_character");
	cJSON_AddBoolToObject(result, "success", success);
	if (account)
		cJSON_AddStringToObject(result, "account", account);
	if (name)
		cJSON_AddStringToObject(result, "name", name);
	if (request_id)
		cJSON_AddStringToObject(result, "requestId", request_id);
	if (error)
		cJSON_AddStringToObject(result, "error", error);

	char *json_str = cJSON_PrintUnformatted(result);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}
	cJSON_Delete(result);
}

/* admin delete a character (durisweb service only) */
void ws_cmd_admin_delete_character(struct descriptor_data *d, cJSON *data)
{
	cJSON             *account_json, *name_json, *deleted_by_json, *request_id_json, *pid_json;
	const char        *account_name, *char_name, *deleted_by, *request_id;
	int                char_pid;
	struct acct_chars *c, *prev;
	P_char             ch;
	P_acct             target_acct;

	/* only durisweb service can call this */
	if (!d->durisweb_verified)
	{
		ws_send_admin_delete_response(d, 0, NULL, NULL, NULL, "Not authorized");
		return;
	}

	if (!data)
	{
		ws_send_admin_delete_response(d, 0, NULL, NULL, NULL, "Missing data");
		return;
	}

	/* extract requestId first for all responses */
	request_id_json = cJSON_GetObjectItem(data, "requestId");
	request_id      = (request_id_json && cJSON_IsString(request_id_json)) ? request_id_json->valuestring : NULL;

	account_json    = cJSON_GetObjectItem(data, "account");
	name_json       = cJSON_GetObjectItem(data, "name");
	pid_json        = cJSON_GetObjectItem(data, "pid");
	deleted_by_json = cJSON_GetObjectItem(data, "deletedBy");

	if (!account_json || !cJSON_IsString(account_json))
	{
		ws_send_admin_delete_response(d, 0, NULL, NULL, request_id, "Missing account name");
		return;
	}

	if (!name_json || !cJSON_IsString(name_json))
	{
		ws_send_admin_delete_response(d, 0, NULL, NULL, request_id, "Missing character name");
		return;
	}

	if (!pid_json || !cJSON_IsNumber(pid_json))
	{
		ws_send_admin_delete_response(d, 0, NULL, NULL, request_id, "Missing character PID");
		return;
	}

	account_name = account_json->valuestring;
	char_name    = name_json->valuestring;
	char_pid     = pid_json->valueint;
	deleted_by   = deleted_by_json && cJSON_IsString(deleted_by_json) ? deleted_by_json->valuestring : "admin";

	/* send initial progress */
	{
		char msg[256];
		snprintf(msg, sizeof(msg), "Starting deletion of %s from account %s", char_name, account_name);
		ws_send_admin_delete_progress(d, request_id, msg, "info");
	}

	/* allocate and load target account */
	ws_send_admin_delete_progress(d, request_id, "Loading account data...", "info");
	target_acct = allocate_account();
	if (!target_acct)
	{
		ws_send_admin_delete_progress(d, request_id, "Failed to allocate account", "error");
		ws_send_admin_delete_response(d, 0, account_name, char_name, request_id, "Failed to allocate account");
		return;
	}

	target_acct->acct_name = str_dup(account_name);

	if (read_account(target_acct) == -1)
	{
		ws_send_admin_delete_progress(d, request_id, "Account not found", "error");
		ws_send_admin_delete_response(d, 0, account_name, char_name, request_id, "Account not found");
		free_account(target_acct);
		return;
	}

	ws_send_admin_delete_progress(d, request_id, "Account loaded successfully", "success");

	/* find character in account list */
	ws_send_admin_delete_progress(d, request_id, "Searching for character in account...", "info");
	c    = target_acct->acct_character_list;
	prev = NULL;
	while (c)
	{
		if (strcasecmp(c->charname, char_name) == 0)
		{
			break;
		}
		prev = c;
		c    = c->next;
	}

	if (!c)
	{
		ws_send_admin_delete_progress(d, request_id, "Character not found in account", "error");
		ws_send_admin_delete_response(d, 0, account_name, char_name, request_id, "Character not found in account");
		free_account(target_acct);
		return;
	}

	ws_send_admin_delete_progress(d, request_id, "Character found in account", "success");

	/* load character for deletion */
	ws_send_admin_delete_progress(d, request_id, "Loading character save file...", "info");
	ch = (struct char_data *)malloc(sizeof(struct char_data));
	if (!ch)
	{
		ws_send_admin_delete_progress(d, request_id, "Failed to allocate memory", "error");
		ws_send_admin_delete_response(d, 0, account_name, char_name, request_id, "Failed to allocate character");
		free_account(target_acct);
		return;
	}

	memset(ch, 0, sizeof(struct char_data));
	ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
	if (!ch->only.pc)
	{
		free(ch);
		ws_send_admin_delete_progress(d, request_id, "Failed to allocate memory", "error");
		ws_send_admin_delete_response(d, 0, account_name, char_name, request_id, "Failed to allocate character data");
		free_account(target_acct);
		return;
	}

	memset(ch->only.pc, 0, sizeof(struct pc_only_data));

	int restore_result = restoreCharOnly(ch, (char *)char_name);
	if (restore_result < 0)
	{
		/* pfile doesn't exist or is corrupted - still clean up account and database */
		if (restore_result == -1)
		{
			ws_send_admin_delete_progress(d, request_id, "Character save file not found (orphaned entry)", "info");
		}
		else
		{
			ws_send_admin_delete_progress(d, request_id, "Character save file corrupted", "info");
		}
		free(ch->only.pc);
		free(ch);

		ws_send_admin_delete_progress(d, request_id, "Cleaning up orphaned character data...", "info");

		/* log the deletion - audit trail */
		logit(LOG_PLAYER, "ADMIN: %s deleted character %s (pid=%d) from account %s via web admin (pfile missing/corrupted)", deleted_by, char_name, char_pid, account_name);

		/* soft delete from frag leaderboard tables using the provided PID */
		ws_send_admin_delete_progress(d, request_id, "Removing from frag leaderboard...", "info");
		if (!sql_soft_delete_character(char_pid))
		{
			ws_send_admin_delete_progress(d, request_id, "Failed to remove from frag leaderboard", "error");
			return;
		}
		ws_send_admin_delete_progress(d, request_id, "Removed from frag leaderboard", "success");

		/* remove from account character list */
		ws_send_admin_delete_progress(d, request_id, "Removing from account character list...", "info");
		if (prev)
		{
			prev->next = c->next;
		}
		else
		{
			target_acct->acct_character_list = c->next;
		}
		FREE(c->charname);
		FREE(c);
		target_acct->num_chars--;

		ws_send_admin_delete_progress(d, request_id, "Writing account file...", "info");
		if (-1 == write_account(target_acct))
		{
			ws_send_admin_delete_progress(d, request_id, "Failed to update account file", "error");
			statuslog(56, "&+RALERT&n: failed to update account file for %s after delete", account_name);
			persistence_alert(AVATAR, "account", account_name, "none", "none", "write_failed", "character delete account update failed");
			free_account(target_acct);
			return;
		}
		free_account(target_acct);
		ws_send_admin_delete_progress(d, request_id, "Account file updated", "success");

		/* send success - web will soft-delete from database */
		ws_send_admin_delete_progress(d, request_id, "Character deletion completed", "success");
		ws_send_admin_delete_response(d, 1, account_name, char_name, request_id, NULL);
		return;
	}

	ws_send_admin_delete_progress(d, request_id, "Character save file loaded", "success");

	/* log the deletion - audit trail */
	logit(LOG_PLAYER, "ADMIN: %s deleted character %s from account %s via web admin", deleted_by, char_name, account_name);

	/* delete character file and free temp character */
	ws_send_admin_delete_progress(d, request_id, "Deleting character save file...", "info");
	deleteCharacter(ch);
	ws_send_admin_delete_progress(d, request_id, "Character save file deleted", "success");

	/* free strings allocated by restoreCharOnly */
	if (ch->player.name)
		str_free(ch->player.name);
	if (ch->player.title)
		str_free(ch->player.title);
	if (ch->player.short_descr)
		str_free(ch->player.short_descr);
	if (ch->player.long_descr)
		str_free(ch->player.long_descr);
	if (ch->player.description)
		str_free(ch->player.description);
	if (ch->only.pc->poofIn)
		str_free(ch->only.pc->poofIn);
	if (ch->only.pc->poofOut)
		str_free(ch->only.pc->poofOut);
	if (ch->only.pc->poofInSound)
		str_free(ch->only.pc->poofInSound);
	if (ch->only.pc->poofOutSound)
		str_free(ch->only.pc->poofOutSound);
	if (ch->only.pc->gcmd_arr)
		FREE(ch->only.pc->gcmd_arr);

	free(ch->only.pc);
	free(ch);

	/* remove from account character list */
	ws_send_admin_delete_progress(d, request_id, "Removing from account character list...", "info");
	if (prev)
	{
		prev->next = c->next;
	}
	else
	{
		target_acct->acct_character_list = c->next;
	}
	FREE(c->charname);
	FREE(c);
	target_acct->num_chars--;
	ws_send_admin_delete_progress(d, request_id, "Removed from account", "success");

	ws_send_admin_delete_progress(d, request_id, "Writing account file...", "info");
	if (-1 == write_account(target_acct))
	{
		ws_send_admin_delete_progress(d, request_id, "Failed to update account file", "error");
		statuslog(56, "&+RALERT&n: failed to update account file for %s after delete", account_name);
		persistence_alert(AVATAR, "account", account_name, "none", "none", "write_failed", "character delete account update failed");
		free_account(target_acct);
		return;
	}
	ws_send_admin_delete_progress(d, request_id, "Account file updated", "success");
	free_account(target_acct);

	/* send success response */
	ws_send_admin_delete_progress(d, request_id, "Character deletion completed", "success");
	ws_send_admin_delete_response(d, 1, account_name, char_name, request_id, NULL);
}

/* get rested bonus status for all characters */
void ws_cmd_rested_bonus(struct descriptor_data *d, cJSON *data)
{
	cJSON             *result_data, *characters, *char_obj;
	struct acct_chars *c;
	P_char             temp_ch;
	time_t             current_time;

	if (!d->account)
	{
		ws_send_account_message(d, "error", NULL, "Not logged in");
		return;
	}

	result_data  = cJSON_CreateObject();
	characters   = cJSON_CreateArray();
	current_time = time(0);

	c = d->account->acct_character_list;
	while (c)
	{
		temp_ch = (struct char_data *)malloc(sizeof(struct char_data));
		if (temp_ch)
		{
			memset(temp_ch, 0, sizeof(struct char_data));
			temp_ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
			if (temp_ch->only.pc)
			{
				memset(temp_ch->only.pc, 0, sizeof(struct pc_only_data));
				if (restoreCharOnly(temp_ch, c->charname) >= 0)
				{
					time_t offline_seconds = current_time - temp_ch->player.time.saved;
					int    offline_hours   = offline_seconds / 3600;
					int    max_hours       = 20; /* well-rested threshold */
					int    percent         = (offline_hours * 100) / max_hours;
					if (percent > 100)
						percent = 100;

					/* capitalize name */
					char name_cap[32];
					strncpy(name_cap, GET_NAME(temp_ch), 31);
					name_cap[31] = '\0';
					if (name_cap[0])
						name_cap[0] = toupper(name_cap[0]);

					char_obj = cJSON_CreateObject();
					cJSON_AddStringToObject(char_obj, "name", name_cap);
					cJSON_AddNumberToObject(char_obj, "restedPercent", percent);
					cJSON_AddNumberToObject(char_obj, "restedHours", offline_hours > max_hours ? max_hours : offline_hours);
					cJSON_AddNumberToObject(char_obj, "maxHours", max_hours);
					cJSON_AddItemToArray(characters, char_obj);
				}
				cleanup_temp_char(temp_ch);
				free(temp_ch->only.pc);
			}
			free(temp_ch);
		}
		c = c->next;
	}

	cJSON_AddItemToObject(result_data, "characters", characters);
	ws_send_account_message(d, "rested_bonus", result_data, NULL);
}

/* logout from account (disconnect) */
void ws_cmd_logout(struct descriptor_data *d, cJSON *data)
{
	if (!d->account)
	{
		ws_send_account_message(d, "error", NULL, "Not logged in");
		return;
	}

	statuslog(56, "Account %s logged out via web client", d->account->acct_name);

	ws_send_account_message(d, "logged_out", NULL, NULL);
	STATE(d) = CON_EXIT;
}

/* send return to account menu signal (on rent, death, quit, or suicide) */
void ws_send_return_to_menu(struct descriptor_data *d, const char *reason)
{
	cJSON *data_obj;

	if (!d || !d->account)
		return;
	if (!d->websocket)
		return;

	data_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(data_obj, "characters", ws_build_character_list(d));

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", "account");
	cJSON_AddStringToObject(root, "action", "return_to_menu");
	cJSON_AddStringToObject(root, "reason", reason);
	cJSON_AddItemToObject(root, "data", data_obj);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}

	cJSON_Delete(root);

	statuslog(56, "Account %s returned to menu: %s", d->account->acct_name, reason);
}

/* polls */
void ws_cmd_poll_list(struct descriptor_data *d, cJSON *data)
{
	if (!d || !d->account)
	{
		ws_send_system(d, "error", "Not authenticated");
		return;
	}

	cJSON *active_only_json = data ? cJSON_GetObjectItem(data, "active_only") : NULL;
	bool   active_only      = active_only_json ? cJSON_IsTrue(active_only_json) : true;

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", "poll_list");

	cJSON *data_obj  = cJSON_CreateObject();
	cJSON *polls_arr = cJSON_CreateArray();

	vector<poll_data> polls = poll_get_all(active_only);
	for (size_t i = 0; i < polls.size(); i++)
	{
		cJSON *poll_obj = cJSON_CreateObject();
		cJSON_AddNumberToObject(poll_obj, "id", polls[i].id);
		cJSON_AddStringToObject(poll_obj, "question", polls[i].question.c_str());
		cJSON_AddNumberToObject(poll_obj, "expires_at", (double)polls[i].expires_at);
		cJSON_AddNumberToObject(poll_obj, "total_votes", polls[i].total_votes);
		cJSON_AddBoolToObject(poll_obj, "multi_select", polls[i].multi_select);
		cJSON_AddBoolToObject(poll_obj, "is_active", polls[i].is_active);
		cJSON_AddItemToArray(polls_arr, poll_obj);
	}

	cJSON_AddItemToObject(data_obj, "polls", polls_arr);
	cJSON_AddItemToObject(root, "data", data_obj);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}
	cJSON_Delete(root);
}

void ws_cmd_poll_view(struct descriptor_data *d, cJSON *data)
{
	if (!d || !d->account)
	{
		ws_send_system(d, "error", "Not authenticated");
		return;
	}

	cJSON *poll_id_json = data ? cJSON_GetObjectItem(data, "poll_id") : NULL;
	if (!poll_id_json || !cJSON_IsNumber(poll_id_json))
	{
		ws_send_system(d, "error", "Missing poll_id");
		return;
	}

	int       poll_id = (int)cJSON_GetNumberValue(poll_id_json);
	poll_data poll    = poll_get_by_id(poll_id);

	if (poll.id == 0)
	{
		ws_send_system(d, "error", "Poll not found");
		return;
	}

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "type", "poll_view");

	cJSON *data_obj = cJSON_CreateObject();
	cJSON_AddNumberToObject(data_obj, "id", poll.id);
	cJSON_AddStringToObject(data_obj, "question", poll.question.c_str());
	cJSON_AddStringToObject(data_obj, "created_by", poll.created_by.c_str());
	cJSON_AddNumberToObject(data_obj, "expires_at", (double)poll.expires_at);
	cJSON_AddBoolToObject(data_obj, "multi_select", poll.multi_select);
	cJSON_AddNumberToObject(data_obj, "max_choices", poll.max_choices);
	cJSON_AddBoolToObject(data_obj, "is_active", poll.is_active);
	cJSON_AddNumberToObject(data_obj, "total_votes", poll.total_votes);

	/* voted? */
	bool has_voted = poll_has_voted(d->account->acct_name, poll_id);
	cJSON_AddBoolToObject(data_obj, "has_voted", has_voted);

	cJSON *options_arr = cJSON_CreateArray();
	for (size_t i = 0; i < poll.options.size(); i++)
	{
		cJSON *opt_obj = cJSON_CreateObject();
		cJSON_AddNumberToObject(opt_obj, "num", poll.options[i].option_num);
		cJSON_AddStringToObject(opt_obj, "text", poll.options[i].text.c_str());
		cJSON_AddNumberToObject(opt_obj, "votes", poll.options[i].vote_count);
		cJSON_AddItemToArray(options_arr, opt_obj);
	}
	cJSON_AddItemToObject(data_obj, "options", options_arr);

	cJSON_AddItemToObject(root, "data", data_obj);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str)
	{
		websocket_send_text(d, json_str);
		free(json_str);
	}
	cJSON_Delete(root);
}

void ws_cmd_poll_vote(struct descriptor_data *d, cJSON *data)
{
	if (!d || !d->account)
	{
		ws_send_system(d, "error", "Not authenticated");
		return;
	}

	cJSON *poll_id_json = data ? cJSON_GetObjectItem(data, "poll_id") : NULL;
	cJSON *choices_json = data ? cJSON_GetObjectItem(data, "choices") : NULL;

	if (!poll_id_json || !cJSON_IsNumber(poll_id_json))
	{
		ws_send_system(d, "error", "Missing poll_id");
		return;
	}

	if (!choices_json || !cJSON_IsArray(choices_json))
	{
		ws_send_system(d, "error", "Missing choices array");
		return;
	}

	int       poll_id = (int)cJSON_GetNumberValue(poll_id_json);
	poll_data poll    = poll_get_by_id(poll_id);

	if (poll.id == 0)
	{
		ws_send_system(d, "error", "Poll not found");
		return;
	}

	if (!poll.is_active)
	{
		ws_send_system(d, "error", "Poll is closed");
		return;
	}

	if (poll_has_voted(d->account->acct_name, poll_id))
	{
		ws_send_system(d, "error", "Already voted");
		return;
	}

	/* choices */
	vector<int> choices;
	int         arr_size = cJSON_GetArraySize(choices_json);
	for (int i = 0; i < arr_size; i++)
	{
		cJSON *item = cJSON_GetArrayItem(choices_json, i);
		if (cJSON_IsNumber(item))
		{
			choices.push_back((int)cJSON_GetNumberValue(item));
		}
	}

	if (choices.empty())
	{
		ws_send_system(d, "error", "No valid choices");
		return;
	}

	if (!poll.multi_select && choices.size() > 1)
	{
		ws_send_system(d, "error", "Only one choice allowed");
		return;
	}

	if ((int)choices.size() > poll.max_choices)
	{
		ws_send_system(d, "error", "Too many choices");
		return;
	}

	/* validate */
	for (size_t i = 0; i < choices.size(); i++)
	{
		bool found = false;
		for (size_t j = 0; j < poll.options.size(); j++)
		{
			if (poll.options[j].option_num == choices[i])
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			ws_send_system(d, "error", "Invalid choice");
			return;
		}
	}

	/* record vote */
#ifndef __NO_MYSQL__
	int votes_cast = poll_record_votes(d->account->acct_name, "web", poll_id, poll, choices);

	if (votes_cast > 0)
	{
		/* success */
		cJSON *root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, "type", "poll_vote");
		cJSON *data_obj = cJSON_CreateObject();
		cJSON_AddBoolToObject(data_obj, "success", true);
		cJSON_AddStringToObject(data_obj, "message", "Vote recorded");
		cJSON_AddItemToObject(root, "data", data_obj);

		char *json_str = cJSON_PrintUnformatted(root);
		if (json_str)
		{
			websocket_send_text(d, json_str);
			free(json_str);
		}
		cJSON_Delete(root);

		/* broadcast */
		poll = poll_get_by_id(poll_id);
		poll_broadcast_vote(poll_id, poll.total_votes);
	}
	else
	{
		ws_send_system(d, "error", "Failed to record vote");
	}
#else
	ws_send_system(d, "error", "Database not available");
#endif
}

/* dispatch */
void ws_handle_command(struct descriptor_data *d, const char *cmd, cJSON *data)
{
	if (!cmd)
		return;

	if (strcmp(cmd, "login") == 0)
	{
		ws_cmd_login(d, data);
	}
	else if (strcmp(cmd, "register") == 0)
	{
		ws_cmd_register(d, data);
	}
	else if (strcmp(cmd, "enter") == 0)
	{
		ws_cmd_enter(d, data);
	}
	else if (strcmp(cmd, "game") == 0)
	{
		ws_cmd_game(d, data);
	}
	else if (strcmp(cmd, "chargen_options") == 0)
	{
		ws_cmd_chargen_options(d, data);
	}
	else if (strcmp(cmd, "roll_stats") == 0)
	{
		ws_cmd_roll_stats(d, data);
	}
	else if (strcmp(cmd, "add_bonus") == 0)
	{
		ws_cmd_add_bonus(d, data);
	}
	else if (strcmp(cmd, "validate_name") == 0)
	{
		ws_cmd_validate_name(d, data);
	}
	else if (strcmp(cmd, "get_hometowns") == 0)
	{
		ws_cmd_get_hometowns(d, data);
	}
	else if (strcmp(cmd, "create_character") == 0)
	{
		ws_cmd_create_character(d, data);
	}
	else if (strcmp(cmd, "swap_stats") == 0)
	{
		ws_cmd_swap_stats(d, data);
	}
	else if (strcmp(cmd, "account_info") == 0)
	{
		ws_cmd_account_info(d, data);
	}
	else if (strcmp(cmd, "change_email") == 0)
	{
		ws_cmd_change_email(d, data);
	}
	else if (strcmp(cmd, "change_password") == 0)
	{
		ws_cmd_change_password(d, data);
	}
	else if (strcmp(cmd, "delete_character") == 0)
	{
		ws_cmd_delete_character(d, data);
	}
	else if (strcmp(cmd, "rested_bonus") == 0)
	{
		ws_cmd_rested_bonus(d, data);
	}
	else if (strcmp(cmd, "logout") == 0)
	{
		ws_cmd_logout(d, data);
	}
	else if (strcmp(cmd, "durisweb_auth") == 0)
	{
		ws_cmd_durisweb_auth(d, data);
	}
	else if (strcmp(cmd, "admin_delete_character") == 0)
	{
		statuslog(56, "Dispatching admin_delete_character command");
		ws_cmd_admin_delete_character(d, data);
	}
	else if (strcmp(cmd, "poll_list") == 0)
	{
		ws_cmd_poll_list(d, data);
	}
	else if (strcmp(cmd, "poll_view") == 0)
	{
		ws_cmd_poll_view(d, data);
	}
	else if (strcmp(cmd, "poll_vote") == 0)
	{
		ws_cmd_poll_vote(d, data);
	}
	else
	{
		/* unknown = game cmd */
		if (d->connected == CON_PLAYING)
		{
			write_to_q(cmd, &d->input, 0);
		}
	}
}

/* initialize websocket handlers */
void ws_handlers_init(void) { statuslog(56, "WebSocket command handlers initialized"); }
