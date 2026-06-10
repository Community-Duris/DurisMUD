
/************************************************************************
 * sql.c - interface to MySQL database and functions for stats keeping  *
 *                                                                      *
 * Written by: Thima (Xenofon Papadopoulos)                             *
 *                                                                      *
 ************************************************************************/

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "utils.h"
#include "sql.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "account.h"
#include "assocs.h"
#include "boon.h"
#include "epic.h"
#include "graph.h"
#include "mm.h"
#include "objmisc.h"
#include "redis.h"
#include "specializations.h"
#include "spells.h"
#include "persistence_queue.h"
#include "sql_player.h"
#include "timers.h"

extern P_index                         mob_index;
extern const struct race_names         race_names_table[];
extern const struct class_names        class_names_table[];
extern const struct playable_race_info playable_races[];
extern const char                     *specdata[][MAX_SPEC];
extern P_room                          world;
extern int                             RUNNING_PORT;
void                                   get_assoc_name(int, char *);
bool                                   get_equipment_list(P_char ch, char *buf, int list_only);
extern P_index                         obj_index;
extern struct zone_data               *zone_table;
extern int                             top_of_zone_table;
extern P_index                         obj_index;
extern P_obj                           object_list;
extern P_room                          world;

void get_pkill_player_description(P_char ch, char *buffer);

#ifdef __NO_MYSQL__
int  initialize_mysql() { return 1; }
void do_sql(P_char ch, char *argument, int cmd) {}
int  sql_save_player_core(P_char ch) { return 1; }
void sql_modify_frags(P_char ch, int gain) {}
void sql_insert_item(P_char ch, P_obj obj, char *desc) {}

void sql_save_pkill(P_char ch, P_char victim) {}
void sql_insert_new_item(P_char ch, P_obj obj) {}

void sql_webinfo_toggle(P_char ch) {}
void sql_update_level(P_char ch) {}
void sql_update_money(P_char ch) {}
void sql_update_epics(P_char ch) {}
void sql_update_playtime(P_char ch) {}
void manual_log(P_char ch) {}
void perform_wiki_search(P_char ch, const char *buf) {}
int  sql_quest_finish(P_char ch, P_char giver, int type, int value) { return -1; }
int  sql_quest_trophy(P_char giver) { return -1; }
int  sql_shop_trophy(P_obj obj) { return -1; }
int  sql_shop_sell(P_char ch, P_obj obj, int value) { return -1; }
void sql_world_quest_finished(P_char ch, P_obj obj) {}
int  sql_world_quest_done_already(P_char ch, int quest_target) { return -1; }
int  sql_world_quest_can_do_another(P_char ch) { return -1; }
void sql_zone_touch_finished(const char *event_key, int boot_time, int touched_at, int zone_number, int toucher_pid, int group_size, int epic_value, int alignment_delta) {}

void        sql_connectIP(P_char ch) {}
void        sql_disconnectIP(P_char ch) {}
const char *sql_select_IP_info(P_char ch, char *buf, size_t bufSize, time_t *lastConnect, time_t *lastDisconnect)
{
	buf[0] = 0;
	return buf;
}
int  sql_find_racewar_for_ip(char *ip, int *racewar_side) { return -1; }
bool qry(const char *format, ...) { return TRUE; }
bool sql_persistence_write_item_event_line(const char *line) { return FALSE; }
bool sql_persistence_write_scalar_event_line(const char *line) { return FALSE; }
void send_to_pid_offline(const char *msg, int pid) {}
void send_offline_messages(P_char ch) {}
void log_epic_gain(int pid, int zone_id, int type, int epics) {}
void log_epic_gain_event(const char *event_key, int pid, int type, int type_id, int epics) {}
void update_zone_db() {}
void update_zone_epic_level(int zone_id, int level) {}
void show_frag_trophy(P_char ch, P_char who) { send_to_char("Disabled.", ch); }
void sql_log(P_char ch, char *kind, char *format, ...) {}

bool get_zone_info(int zone_number, struct zone_info *info) { return FALSE; }

string escape_str(const char *str) { return string(str); }

string get_mud_info(const char *name) { return string(); }

void send_mud_info(const char *name, P_char ch) {}

void sql_update_bind_data(int vnum, int *owner_pid, int *timer) {}

void sql_get_bind_data(int vnum, int *owner_pid, int *timer) {}

bool sql_pwipe(int code_verify)
{
	if (code_verify == 1723699)
	{
		logit(LOG_DEBUG, "sql_pwipe: &=GlCan't wipe the SQL stuff as SQL database is not loaded.");
	}
	else
	{
		logit(LOG_DEBUG, "sql_pwipe: &=GlSomeone called sql_pwipe with a bad verify code... hrm..");
	}
	return FALSE;
}
bool sql_clear_zone_trophy() { return FALSE; }
#else

static void sql_resetConnectTimes(void);

// The global database handler
MYSQL *DB;
static MYSQL *persistenceDB = NULL;
static pthread_mutex_t persistence_sql_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool persistence_tables_ready = FALSE;
static bool persistence_reward_tables_ready = FALSE;
static unsigned long persistence_reward_event_sequence = 0;
static MYSQL *sql_persistence_connection(void);
static bool sql_ensure_runtime_schema(MYSQL *db);
static bool sql_persistence_ensure_reward_tables(MYSQL *db);
static bool sql_persistence_write_item_event_line_locked(const char *line);
static bool sql_persistence_write_scalar_event_line_locked(const char *line);
static void sql_reward_event_key(char *buf, int buf_size, const char *type, int pid, int source_id);
static const char *sql_scalar_clean_field(const char *in, char *buf, int buf_size);

/* Escapes a string. */
char *mysql_str(const char *str, char *buf)
{
	mysql_real_escape_string(DB, buf, str, strlen(str));
	return buf;
}

string escape_str(const char *str)
{
	static char buff[MAX_STRING_LENGTH];
	mysql_real_escape_string(DB, buff, str, strlen(str));
	return string(buff);
}

/* populate races and classes lookup tables on boot */
void sql_populate_lookup_tables()
{
	char buf[MAX_STRING_LENGTH];
	char esc_name[256], esc_ansi[256], esc_short[64], esc_abbrev[16];
	int  i;

	logit(LOG_STATUS, "Populating lookup tables...");

	// clear existing data
	qry("DELETE FROM races");
	qry("DELETE FROM classes");

	// populate races table
	for (i = 0; i <= LAST_RACE; i++)
	{
		if (!race_names_table[i].normal || !race_names_table[i].normal[0])
			continue;

		mysql_real_escape_string(DB, esc_name, race_names_table[i].normal, strlen(race_names_table[i].normal));
		mysql_real_escape_string(DB, esc_ansi, race_names_table[i].ansi ? race_names_table[i].ansi : "", race_names_table[i].ansi ? strlen(race_names_table[i].ansi) : 0);
		mysql_real_escape_string(DB, esc_short, race_names_table[i].no_spaces ? race_names_table[i].no_spaces : "", race_names_table[i].no_spaces ? strlen(race_names_table[i].no_spaces) : 0);
		mysql_real_escape_string(DB, esc_abbrev, race_names_table[i].code ? race_names_table[i].code : "", race_names_table[i].code ? strlen(race_names_table[i].code) : 0);

		// check if this is a playable race and get racewar side
		int racewar  = 0;
		int playable = 0;
		for (int j = 0; playable_races[j].race_id >= 0; j++)
		{
			if (playable_races[j].race_id == i)
			{
				playable = 1;
				if (strcmp(playable_races[j].faction, "good") == 0)
					racewar = RACEWAR_GOOD;
				else if (strcmp(playable_races[j].faction, "evil") == 0)
					racewar = RACEWAR_EVIL;
				else if (strcmp(playable_races[j].faction, "undead") == 0)
					racewar = RACEWAR_UNDEAD;
				else if (strcmp(playable_races[j].faction, "neutral") == 0)
					racewar = RACEWAR_NEUTRAL;
				break;
			}
		}

		snprintf(buf,
		         sizeof(buf),
		         "INSERT INTO races (id, name, short_name, ansi_name, abbrev, racewar, playable) "
		         "VALUES (%d, '%s', '%s', '%s', '%s', %d, %d)",
		         i,
		         esc_name,
		         esc_short,
		         esc_ansi,
		         esc_abbrev,
		         racewar,
		         playable);
		qry("%s", buf);
	}

	// populate classes table
	for (i = 0; i <= CLASS_COUNT; i++)
	{
		if (!class_names_table[i].normal || !class_names_table[i].normal[0])
			continue;

		mysql_real_escape_string(DB, esc_name, class_names_table[i].normal, strlen(class_names_table[i].normal));
		mysql_real_escape_string(DB, esc_ansi, class_names_table[i].ansi ? class_names_table[i].ansi : "", class_names_table[i].ansi ? strlen(class_names_table[i].ansi) : 0);
		mysql_real_escape_string(DB, esc_short, class_names_table[i].code ? class_names_table[i].code : "", class_names_table[i].code ? strlen(class_names_table[i].code) : 0);

		char letter[2] = {class_names_table[i].letter, '\0'};

		snprintf(buf,
		         sizeof(buf),
		         "INSERT INTO classes (id, name, ansi_name, short_name, menu_char) "
		         "VALUES (%d, '%s', '%s', '%s', '%s')",
		         i,
		         esc_name,
		         esc_ansi,
		         esc_short,
		         letter);
		qry("%s", buf);
	}

	logit(LOG_STATUS, "Lookup tables populated.");
}

/* load .env file if present, setting environment variables */
int load_env_file(void)
{
	FILE *f = fopen(".env", "r");
	if (!f)
	{
		logit(LOG_STATUS, "No .env file found, using default database credentials.");
		return 0;
	}

	char line[256];
	int  count = 0;
	while (fgets(line, sizeof(line), f))
	{
		// skip comments and empty lines
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
			continue;

		// remove newline
		char *nl = strchr(line, '\n');
		if (nl)
			*nl = '\0';
		nl = strchr(line, '\r');
		if (nl)
			*nl = '\0';

		// skip empty lines after trimming
		if (line[0] == '\0')
			continue;

		// parse KEY=VALUE
		char *eq = strchr(line, '=');
		if (eq)
		{
			*eq = '\0';
			setenv(line, eq + 1, 1);
			count++;
		}
	}
	fclose(f);

	logit(LOG_STATUS, "Loaded %d environment variables from .env file.", count);
	return count;
}

/* Open a connection to the database. The connection will remain open
 * throughout the mud session. */
int initialize_mysql()
{
	/* use database from .env / environment variable */
	/* hack to ensure we're not using the live database when not running on default port */
	char db_name[50];
	snprintf(db_name, 50, "%s", DB_NAME);

	if (RUNNING_PORT != DFLT_PORT)
	{
		snprintf(db_name, 50, "duris_dev");
	}

	logit(LOG_STATUS, "Initializing MySQL persistent connection to %s (host=%s port=%d).", db_name, DB_HOST, DB_PORT);
	DB = mysql_init(NULL);
	if (DB == NULL)
	{
		logit(LOG_STATUS, "Error initializing handler.");
		return -1;
	}

	unsigned int timeout = 10;
  	mysql_options(DB, MYSQL_OPT_READ_TIMEOUT, &timeout);
  	mysql_options(DB, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

	DB = mysql_real_connect(DB, DB_HOST, DB_USER, DB_PASSWD, db_name, DB_PORT, NULL, CLIENT_MULTI_STATEMENTS);
	if (DB == NULL)
	{
		logit(LOG_STATUS, "Error connecting to database.");
		return -1;
	}

	logit(LOG_STATUS, "Connection established.");

	sql_resetConnectTimes();
	sql_populate_lookup_tables();

	if (!sql_ensure_runtime_schema(DB))
		logit(LOG_DEBUG, "Runtime schema drift repair did not complete; some SQL features may fail until migrations are applied.");

	(void)sql_persistence_connection();

	return 1;
}

/* Handle a query, log possible errors and return results (if available) */
MYSQL_RES *db_query(const char *format, ...)
{
	char    buf[MAX_LOG_LEN + MAX_STRING_LENGTH + 512];
	va_list args;
	int     ret;

	va_start(args, format);
	buf[0] = '\0';
	// SECURITY FIX: Replace vsprintf with vsnprintf to prevent buffer overflow
	ret = vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	// Check for overflow
	if (ret < 0 || ret >= (int)sizeof(buf))
	{
		logit(LOG_DEBUG, "MySQL: Query too long, truncated or error in formatting");
		return NULL;
	}

	if (!buf[0])
		return NULL;

	if (mysql_real_query(DB, buf, strlen(buf)) != 0)
	{
		logit(LOG_DEBUG, "MySQL: \"%s\" failed: %s", buf, mysql_error(DB));
		return NULL;
	}

	return mysql_store_result(DB);
}

/* Same as above, but won't log failed queries, ie when key restrictions suffice */
MYSQL_RES *db_query_nolog(const char *format, ...)
{
	char    buf[MAX_STRING_LENGTH];
	va_list args;
	int     ret;

	va_start(args, format);
	buf[0] = '\0';
	// SECURITY FIX: Replace vsprintf with vsnprintf to prevent buffer overflow
	ret = vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	// Check for overflow
	if (ret < 0 || ret >= (int)sizeof(buf))
	{
		return NULL;
	}

	if (mysql_real_query(DB, buf, strlen(buf)) != 0)
	{
		return NULL;
	}

	return mysql_store_result(DB);
}

/* Store core player data to the database. We assume that only association
 * names may contain special characters */
int sql_save_player_core(P_char ch)
{
	char                     query[MAX_STRING_LENGTH];
	char                     assoc_name[MAX_STRING_LENGTH];
	char                     assoc_name_sql[MAX_STRING_LENGTH];
	const char              *spec_name = "";
	struct char_player_data *p;
	int                      val;

	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);
	p   = &ch->player;
	val = flag2idx(p->m_class);

	if (GET_ASSOC(ch) == NULL)
	{
		assoc_name[0] = '\0';
	}
	else
	{
		snprintf(assoc_name, MAX_STRING_LENGTH, "%s", GET_ASSOC(ch)->get_name().c_str());
	}
	mysql_str(assoc_name, assoc_name_sql);

	if (IS_SPECIALIZED(ch))
	{
		spec_name = GET_SPEC_NAME(ch->player.m_class, ch->player.spec - 1);
	}

	// deactivate any other players with same name (handles renamed characters), async via scalar event queue
	{
		char line[PERSISTENCE_EVENT_MAX_LEN];
		int queued = 0;
		char name_clean[128];
		sql_scalar_clean_field(p->name, name_clean, sizeof(name_clean));
		snprintf(line,
		         sizeof(line),
		         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=player_active|pid=%d|name=%s",
		         (long)time(NULL),
		         GET_PID(ch),
		         name_clean);
		if (persistence_scalar_event_worker_running())
		{
			if (persistence_scalar_event_queue_enqueue(line))
				queued = 1;
			else if (persistence_write_fallback_event_line(line, "scalar_event", p->name, "queue_full_flat_fallback"))
				queued = 1;
		}
		if (!queued)
		{
			char name_sql[256];
			mysql_str(p->name, name_sql);
			snprintf(query, MAX_STRING_LENGTH, "UPDATE player_data SET active = 0 WHERE name = '%s' and pid != %d", name_sql, GET_PID(ch));
			db_query(query);
			db_query("UPDATE player_data SET active = 1 WHERE pid = %d", GET_PID(ch));
		}
	}

	// Update frag leaderboard tables for web statistics
	sql_update_account_character(ch);
	sql_update_frag_leaderboard(ch);

	return 1;
}

/* Save a variable delta. var_type: 1=FRAGS, 2=EXP */
#define PROGRESS_FRAGS       1
#define PROGRESS_EXP         2
void sql_save_progress(int pid, int delta, int var_type) { db_query("INSERT INTO progress VALUES( 0, %d, %d, NOW(), %d )", pid, var_type, delta); }

// Retrieves the current highest number of frags and which racewar side has it.
void get_level_cap_info(long *max_frags, int *racewar, int *level, time_t *next_update)
{
	MYSQL_RES *db = NULL;
	MYSQL_ROW  row;
	db = db_query("SELECT most_frags, racewar_leader, level, UNIX_TIMESTAMP(next_update) FROM level_cap");

	if ((db == NULL) || ((row = mysql_fetch_row(db)) == NULL))
	{
		debug("get_level_cap_info: Database read fail.");
		*max_frags   = (long)-1;
		*racewar     = RACEWAR_NONE;
		*level       = 25;
		*next_update = 0;
		return;
	}
	*max_frags   = (long)(atof(row[0]) * 100. + .01);
	*racewar     = atoi(row[1]);
	*level       = atoi(row[2]);
	*next_update = atol(row[3]);

	// cycle out until a NULL return
	while (row != NULL)
	{
		row = mysql_fetch_row(db);
	}
	mysql_free_result(db);
}

// Returns the highest level achievable by mortals, limited by racewar side.
int sql_level_cap(int racewar_side)
{
	int        leading_racewar, level_cap;
	MYSQL_RES *db = NULL;
	MYSQL_ROW  row;

	db = db_query("SELECT level, racewar_leader FROM level_cap");

	if ((db == NULL) || ((row = mysql_fetch_row(db)) == NULL))
	{
		debug("sql_level_cap: Database read fail.");
		return 25;
	}

	level_cap       = atoi(row[0]);
	leading_racewar = atoi(row[1]);

	// cycle out until a NULL return
	while (row != NULL)
	{
		row = mysql_fetch_row(db);
	}
	mysql_free_result(db);

	// Everyone can reach 56 when someone reaches the limit + 40.
	if (level_cap >= MAXLVLMORTAL)
		return MAXLVLMORTAL;
	// 25 is the lower limit.
	if (level_cap <= 25)
		return 25;
	
	return level_cap;
}

#define CAP_DELAY(old_level) (time(NULL) + SECS_PER_REAL_DAY * 7)

// Checks the number of frags against the current highest and sets the new highest if applicable.
// Adjusted the time inbetween notches from a static 1 day to 1 day for levels 26-29, 2 days for 30-39,
//   3 days for 40-49, and 4 days for 50-56.
void sql_check_level_cap(long max_frags, int racewar)
{
	long   old_max_frags;
	int    old_racewar, old_level;
	time_t next_update;
	char   query[1024];

	get_level_cap_info(&old_max_frags, &old_racewar, &old_level, &next_update);
	// If we've capped out
	if (old_level >= MAXLVLMORTAL)
	{
		return;
	}
	// If enough time has passed, and level should change, update level if appropriate.
	if (next_update <= time(NULL))
	{
		// Have enough frags to update level.
		if (old_level < FRAGS_TO_LEVEL(max_frags / 100.))
		{
			// when level cap increases, give a boon to the side that caused it
			BoonData bdata;
			bdata.duration  = 2880;        // 48 hours
			bdata.racewar   = racewar;
			bdata.type      = BTYPE_EXPM;  // exp mod
			bdata.option    = BOPT_MOB;
			bdata.criteria  = 1;           // 1 kill
			bdata.criteria2 = -1;          // any mob
			bdata.bonus     = 2;           // 100% bonus 
			bdata.active    = 1;
			bdata.repeat    = 1;
			create_boon(&bdata);

			snprintf(
				query, 1024, "UPDATE level_cap SET most_frags = %f, racewar_leader = %d, level = %d, next_update = FROM_UNIXTIME(%ld)", max_frags / 100., racewar, old_level + 5, CAP_DELAY(old_level));
			db_query(query);
		}
		else if (max_frags > old_max_frags)
		{
			snprintf(query, 1024, "UPDATE level_cap SET most_frags = %f, racewar_leader = %d", max_frags / 100., racewar);
			db_query(query);
		}
	}
	// Just changing highest frag amount and, possibly, racewar leader.
	else if (max_frags > old_max_frags)
	{
		snprintf(query, 1024, "UPDATE level_cap SET most_frags = %f, racewar_leader = %d", max_frags / 100., racewar);
		db_query(query);
	}
}

// Sets the values of level (actual cap) and racewar (the side that is in the lead).
void get_level_cap(int *level, int *racewar)
{
	MYSQL_RES *db  = NULL;
	MYSQL_ROW  row = NULL;

	db = db_query("SELECT level, racewar_leader FROM level_cap");

	if ((db == NULL) || ((row = mysql_fetch_row(db)) == NULL))
	{
		debug("get_level_cap: Database read fail.");
		*level   = 25;
		*racewar = RACEWAR_NONE;
	}
	else
	{
		*level   = atoi(row[0]);
		*racewar = atoi(row[1]);
	}

	// cycle out until a NULL return
	while (row != NULL)
	{
		row = mysql_fetch_row(db);
	}
	mysql_free_result(db);
}

/* Save frags delta */
void sql_modify_frags(P_char ch, int gain)
{
	char line[PERSISTENCE_EVENT_MAX_LEN];
	int queued = 0;

	// We don't want IS_TRUSTED(ch) because that can be turned off with toggle fog.
	if (GET_LEVEL(ch) > MAXLVLMORTAL)
	{
		return;
	}
	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);

	// Enqueue frag progress and leaderboard update via async scalar event queue
	snprintf(line,
	         sizeof(line),
	         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=player_frags|pid=%d|delta=%d|total_frags=%d",
	         (long)time(NULL),
	         GET_PID(ch),
	         gain,
	         ch->only.pc->frags);
	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			queued = 1;
		else if (persistence_write_fallback_event_line(line, "scalar_event", GET_NAME(ch), "queue_full_flat_fallback"))
			queued = 1;
	}

	if (!queued)
	{
		// Fallback: synchronous write when worker not available
		sql_save_progress(GET_PID(ch), gain, PROGRESS_FRAGS);
		if (GET_PID(ch) > 0)
		{
			db_query("UPDATE frag_leaderboard SET total_frags = %d, last_updated = NOW() WHERE pid = %ld AND deleted_at IS NULL", ch->only.pc->frags, GET_PID(ch));
		}
	}

	if (gain > 0)
	{
		MYSQL_RES *res = db_query("SELECT SUM(total_frags) FROM frag_leaderboard WHERE racewar=%d", GET_RACEWAR(ch));
		if (res)
		{
			MYSQL_ROW row = mysql_fetch_row(res);
			if(row and row[0])
			{
				long total = atol(row[0]);
				sql_check_level_cap(total, GET_RACEWAR(ch));
			}
			mysql_free_result(res);
		}
	}
}

/*
 * Frag Leaderboard Hybrid System - for web statistics
 * These functions maintain the account_characters and frag_leaderboard tables
 * The MUD continues to use flat files, but web can query the database
 */

/* Update account_characters mapping table */
void sql_update_account_character(P_char ch)
{
	char        account_name_sql[MAX_STRING_LENGTH];
	char        char_name_sql[MAX_STRING_LENGTH];
	const char *account_name;

	if (!ch || IS_NPC(ch))
		return;

	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);

	account_name = get_account_name_safe(ch);

	// Escape strings for SQL safety
	mysql_str(account_name, account_name_sql);
	mysql_str(ch->player.name, char_name_sql);

	// Insert or update account_characters mapping, async via scalar event queue
	{
		char line[PERSISTENCE_EVENT_MAX_LEN];
		int queued = 0;
		char acct_clean[128], char_clean[128];
		sql_scalar_clean_field(account_name, acct_clean, sizeof(acct_clean));
		sql_scalar_clean_field(ch->player.name, char_clean, sizeof(char_clean));
		snprintf(line,
		         sizeof(line),
		         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=account_character_update|pid=%d|account_name=%s|char_name=%s",
		         (long)time(NULL),
		         GET_PID(ch),
		         acct_clean,
		         char_clean);
		if (persistence_scalar_event_worker_running())
		{
			if (persistence_scalar_event_queue_enqueue(line))
				queued = 1;
			else if (persistence_write_fallback_event_line(line, "scalar_event", GET_NAME(ch), "queue_full_flat_fallback"))
				queued = 1;
		}
		if (!queued)
		{
			db_query("INSERT INTO account_characters "
			         "(account_name, pid, char_name, created_at, deleted_at) "
			         "VALUES('%s', %ld, '%s', NOW(), NULL) "
			         "ON DUPLICATE KEY UPDATE "
			         "account_name = VALUES(account_name), "
			         "pid = VALUES(pid), "
			         "char_name = VALUES(char_name), "
			         "deleted_at = NULL",
			         account_name_sql,
			         GET_PID(ch),
			         char_name_sql);
		}
	}
}

double sql_get_total_donated(const char *account_name)
{
#ifdef __NO_MYSQL__
	return 0.0;
#else
	if (!account_name || !*account_name)
		return 0.0;

	MYSQL_RES *res = db_query("SELECT total_donated FROM accounts WHERE account_name='%s'", escape_str(account_name).c_str());
	if (!res)
		return 0.0;

	double    total = 0.0;
	MYSQL_ROW row   = mysql_fetch_row(res);
	if (row && row[0])
		total = atof(row[0]);

	mysql_free_result(res);
	return total;
#endif
}

/* Update frag_leaderboard table with current character data */
void sql_update_frag_leaderboard(P_char ch)
{
	char        account_name_sql[MAX_STRING_LENGTH];
	char        char_name_sql[MAX_STRING_LENGTH];
	char        race_sql[MAX_STRING_LENGTH];
	char        class_sql[MAX_STRING_LENGTH];
	const char *account_name;
	const char *race_name;
	const char *class_name;

	if (!ch || IS_NPC(ch))
		return;

	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);

	account_name = get_account_name_safe(ch);
	race_name    = race_names_table[ch->player.race].normal;
	class_name   = class_names_table[flag2idx(ch->player.m_class)].normal;

	// Escape strings for SQL safety
	mysql_str(account_name, account_name_sql);
	mysql_str(ch->player.name, char_name_sql);
	mysql_str(race_name, race_sql);
	mysql_str(class_name, class_sql);

	// Insert or update frag_leaderboard, async via scalar event queue
	{
		char line[PERSISTENCE_EVENT_MAX_LEN];
		int queued = 0;
		char acct_clean[128], char_clean[128], race_clean[64], class_clean[64];
		sql_scalar_clean_field(account_name, acct_clean, sizeof(acct_clean));
		sql_scalar_clean_field(ch->player.name, char_clean, sizeof(char_clean));
		sql_scalar_clean_field(race_name, race_clean, sizeof(race_clean));
		sql_scalar_clean_field(class_name, class_clean, sizeof(class_clean));
		snprintf(line,
		         sizeof(line),
		         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=frag_leaderboard_update|pid=%d|account_name=%s|char_name=%s|total_frags=%d|racewar=%d|race=%s|class_name=%s|level=%d",
		         (long)time(NULL),
		         GET_PID(ch),
		         acct_clean,
		         char_clean,
		         ch->only.pc->frags,
		         GET_RACEWAR(ch),
		         race_clean,
		         class_clean,
		         GET_LEVEL(ch));
		if (persistence_scalar_event_worker_running())
		{
			if (persistence_scalar_event_queue_enqueue(line))
				queued = 1;
			else if (persistence_write_fallback_event_line(line, "scalar_event", GET_NAME(ch), "queue_full_flat_fallback"))
				queued = 1;
		}
		if (!queued)
		{
			db_query("REPLACE INTO frag_leaderboard "
			         "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
			         "VALUES(%ld, '%s', '%s', %d, %d, '%s', '%s', %d, NULL)",
			         GET_PID(ch),
			         account_name_sql,
			         char_name_sql,
			         ch->only.pc->frags,
			         GET_RACEWAR(ch),
			         race_sql,
			         class_sql,
			         GET_LEVEL(ch));
		}
	}
}

/* Soft delete a character from the leaderboard tables */
void sql_soft_delete_character(long pid)
{
	if (pid <= 0)
		return;

	// Set deleted_at timestamp to NOW() for this character
	db_query("UPDATE account_characters SET deleted_at = NOW() WHERE pid = %ld AND deleted_at IS NULL", pid);

	db_query("UPDATE frag_leaderboard SET deleted_at = NOW() WHERE pid = %ld AND deleted_at IS NULL", pid);
}

/* Save frags delta */
void sql_insert_item(P_char ch, P_obj obj, char *desc)
{

	char query[MAX_STRING_LENGTH];
	char sql_desc[MAX_STRING_LENGTH];
	char sql_short[MAX_STRING_LENGTH];

	int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;
	mysql_str(desc, sql_desc);
	mysql_str(obj->short_description, sql_short);

	db_query_nolog("INSERT INTO items_stats VALUES( null, '%s', '', %d)", sql_short, m_virtual);
	snprintf(query,
	         MAX_STRING_LENGTH,
	         "UPDATE items_stats SET  obj_stat = '%s', vnum = %d "
	         " WHERE short_desc = '%s'",
	         sql_desc,
	         m_virtual,
	         sql_short);

	db_query(query);

	struct zone_data *zone = 0;
	zone                   = &zone_table[world[ch->in_room].zone];
}

void sql_insert_new_item(P_char ch, P_obj obj)
{
	char              item_id[MAX_STRING_LENGTH];
	int               m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;
	int               i         = ch->in_room;
	P_room            rm        = &world[i];
	struct zone_data *zone      = 0;
	zone                        = &zone_table[world[ch->in_room].zone];

	snprintf(item_id, MAX_STRING_LENGTH, "o %s", obj->name);
	do_stat(ch, item_id, 555);
}

unsigned long new_pkill_event(P_char ch)
{
	char room_name_sql[MAX_STRING_LENGTH];
	char query[MAX_STRING_LENGTH];

	mysql_str(world[ch->in_room].name, room_name_sql);
	snprintf(query, MAX_STRING_LENGTH, "INSERT INTO pkill_event (stamp, room_vnum, room_name) VALUES( NOW(), %d, '%s' )", world[ch->in_room].number, room_name_sql);

	if (mysql_real_query(DB, query, strlen(query)) != 0)
	{
		logit(LOG_DEBUG, "MYSQL: Failed to create pkill event");
		logit(LOG_DEBUG, "MYSQL: Query was: %s", query);
		return 0;
	}

	return mysql_insert_id(DB);
}

void get_pkill_player_description(P_char ch, char *buffer)
{
	char assoc_name[MAX_STRING_LENGTH];

	if (GET_ASSOC(ch) == NULL)
	{
		assoc_name[0] = '\0';
	}
	else
	{
		snprintf(assoc_name, MAX_STRING_LENGTH, "%s", GET_ASSOC(ch)->get_name().c_str());
	}

	snprintf(buffer, MAX_STRING_LENGTH, "[%2d %s&n] %s &n%s &n(%s&n)", GET_LEVEL(ch), get_class_name(ch, ch), GET_NAME(ch), assoc_name, race_names_table[GET_RACE(ch)].ansi);

	logit(LOG_DEBUG, "%s", buffer);
}

void store_pkill_info(unsigned long pkill_event, P_char ch, const char *type, int leader, int in_room)
{
	char buf[MAX_STRING_LENGTH];
	char line[PERSISTENCE_EVENT_MAX_LEN];
	/* Async event line budget: PERSISTENCE_EVENT_MAX_LEN=1024, fixed overhead=180 chars.
	 * 843 bytes remain for 3 fields -> 280 chars each (3 bytes margin).
	 * Sync fallback uses full MAX_STRING_LENGTH/MAX_LOG_LEN data via mysql_str().
	 * Tradeoff: async path truncates to 280 chars; sync path stores full data. */
	char pd_clean[280], eq_clean[280], lg_clean[280];
	const char *raw_log;
	int queued = 0;

	if (!ch || !IS_PC(ch))
		return;

	if (!GET_PLAYER_LOG(ch))
	{
		logit(LOG_DEBUG, "Tried to dump player log (%s) in store_pkill_info(), but player log was null!", GET_NAME(ch));
		return;
	}

	/* Collect raw data once for both async and sync fallback */
	get_equipment_list(ch, buf, 1);
	sql_scalar_clean_field(buf, eq_clean, sizeof(eq_clean));

	get_pkill_player_description(ch, buf);
	sql_scalar_clean_field(buf, pd_clean, sizeof(pd_clean));

	raw_log = GET_PLAYER_LOG(ch)->read(LOG_PUBLIC, MAX_LOG_LEN);
	sql_scalar_clean_field(raw_log, lg_clean, sizeof(lg_clean));

	/* Try async scalar event queue first */
	snprintf(line,
	         sizeof(line),
	         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=pkill_info|pkill_event=%lu|pid=%d|level=%d|pk_type=%s|player_description=%s|equip=%s|log=%s|inroom=%d|leader=%d",
	         (long)time(NULL),
	         pkill_event,
	         GET_PID(ch),
	         GET_LEVEL(ch),
	         type,
	         pd_clean,
	         eq_clean,
	         lg_clean,
	         in_room,
	         leader);
	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			queued = 1;
		else if (persistence_write_fallback_event_line(line, "scalar_event", GET_NAME(ch), "queue_full_flat_fallback"))
			queued = 1;
	}

	if (!queued)
	{
		/* Sync fallback: SQL-escape the raw strings from buf and raw_log */
		char equip_sql[MAX_STRING_LENGTH];
		char player_description_sql[MAX_STRING_LENGTH];
		char log_sql[MAX_LOG_LEN];

		get_equipment_list(ch, buf, 1);
		mysql_str(buf, equip_sql);

		get_pkill_player_description(ch, buf);
		mysql_str(buf, player_description_sql);

		mysql_str(raw_log, log_sql);

		db_query("INSERT INTO pkill_info (event_id, pid, level, pk_type, player_description, equip, log, inroom, leader) "
		         "VALUES( %d, %d, %d, '%s', '%s', '%s', '%s', %d ,%d )",
		         pkill_event,
		         GET_PID(ch),
		         GET_LEVEL(ch),
		         type,
		         player_description_sql,
		         equip_sql,
		         log_sql,
		         in_room,
		         leader);
	}
}

/* Save racewr pkill information */
void sql_save_pkill(P_char ch, P_char victim)
{
	P_char        tch;
	unsigned long pkill_event;

	// NPCs can't be pkilled.
	if (IS_NPC(victim))
	{
		return;
	}

	/* If pet is the killer, we blame the owner, if he's around */
	if (IS_NPC(ch))
	{
		if (ch->following && IS_PC(ch->following) && ch->in_room == ch->following->in_room && grouped(ch, ch->following))
		{
			ch = ch->following;
		}
		else
		{
			return;
		}
	}

	/* Log a new pkill event, and get the handler for further logs */
	pkill_event = new_pkill_event(ch);
	if (!pkill_event)
		return;

	int in_room = 0;
	int leader  = 0;

	// always store killer first, then group
	if (IS_PC(ch))
	{
		leader = (ch->group && ch->group->ch == ch) ? 1 : 0;
		store_pkill_info(pkill_event, ch, "KILLER", leader, 1);
	}

	if (ch->group)
	{
		for (struct group_list *gl = ch->group; gl; gl = gl->next)
		{
			if (IS_PC(gl->ch) && gl->ch != ch)
			{
				in_room = (ch->in_room == gl->ch->in_room) ? 1 : 0;
				store_pkill_info(pkill_event, gl->ch, "KILLER", 0, in_room);
			}
		}
	}

	// always store victim first, then group
	if (IS_PC(victim))
	{
		leader = (victim->group && victim->group->ch == victim) ? 1 : 0;
		store_pkill_info(pkill_event, victim, "VICTIM", leader, 1);
	}

	if (victim->group)
	{
		for (struct group_list *gl = victim->group; gl; gl = gl->next)
		{
			if (IS_PC(gl->ch) && gl->ch != victim)
			{
				in_room = (victim->in_room == gl->ch->in_room) ? 1 : 0;
				store_pkill_info(pkill_event, gl->ch, "VICTIM-GROUP", 0, in_room);
			}
		}
	}
}

/* Save character's preferences about displaying extended info on
   webpage for all to see. */
void sql_webinfo_toggle(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;
	// webinfo is stored in act2 flag, saved with player_data
}

/* Update level info */
void sql_update_level(P_char ch)
{
	char line[PERSISTENCE_EVENT_MAX_LEN];

	if (!ch || !IS_PC(ch))
		return;

	snprintf(line, sizeof(line), "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=player_level|pid=%d|level=%d", (long)time(NULL), GET_PID(ch), GET_LEVEL(ch));
	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return;
		if (persistence_write_fallback_event_line(line, "scalar_event", GET_NAME(ch), "queue_full_flat_fallback"))
			return;
	}

	// Sync fallback: update database directly when worker not available
	db_query("UPDATE player_data SET level=%d WHERE pid=%d", GET_LEVEL(ch), GET_PID(ch));
}

/* Update money info */
void sql_update_money(P_char ch)
{
	char line[PERSISTENCE_EVENT_MAX_LEN];

	if (!ch || !IS_PC(ch))
		return;

	snprintf(line,
	         sizeof(line),
	         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=player_money|pid=%d|copper=%d|silver=%d|gold=%d|platinum=%d|bank_copper=%d|bank_silver=%d|bank_gold=%d|bank_platinum=%d",
	         (long)time(NULL),
	         GET_PID(ch),
	         GET_COPPER(ch),
	         GET_SILVER(ch),
	         GET_GOLD(ch),
	         GET_PLATINUM(ch),
	         GET_BALANCE_COPPER(ch),
	         GET_BALANCE_SILVER(ch),
	         GET_BALANCE_GOLD(ch),
	         GET_BALANCE_PLATINUM(ch));
	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return;
		if (persistence_write_fallback_event_line(line, "scalar_event", GET_NAME(ch), "queue_full_flat_fallback"))
			return;
	}

	// Sync fallback: update database directly when worker not available
	db_query("UPDATE player_data SET copper=%d, silver=%d, gold=%d, platinum=%d, "
	         "bank_copper=%d, bank_silver=%d, bank_gold=%d, bank_platinum=%d WHERE pid=%d",
	         GET_COPPER(ch),
	         GET_SILVER(ch),
	         GET_GOLD(ch),
	         GET_PLATINUM(ch),
	         GET_BALANCE_COPPER(ch),
	         GET_BALANCE_SILVER(ch),
	         GET_BALANCE_GOLD(ch),
	         GET_BALANCE_PLATINUM(ch),
	         GET_PID(ch));
}

/* Update playtime info */
void sql_update_playtime(P_char ch)
{
	char line[PERSISTENCE_EVENT_MAX_LEN];

	if (!ch || !IS_PC(ch))
		return;

	snprintf(line, sizeof(line), "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=player_playtime|pid=%d|played_time=%d", (long)time(NULL), GET_PID(ch), ch->player.time.played);
	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return;
		if (persistence_write_fallback_event_line(line, "scalar_event", GET_NAME(ch), "queue_full_flat_fallback"))
			return;
	}

	// Sync fallback: update database directly when worker not available
	db_query("UPDATE player_data SET played_time=%d WHERE pid=%d", ch->player.time.played, GET_PID(ch));
}

/* Update player's epics: We want to record their total epics gained not epics unused */
void sql_update_epics(P_char ch)
{
	char line[PERSISTENCE_EVENT_MAX_LEN];

	if (!ch || !IS_PC(ch))
		return;

	snprintf(line, sizeof(line), "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=player_epics|pid=%d|epics=%ld", (long)time(NULL), GET_PID(ch), ch->only.pc->epics);
	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return;
		if (persistence_write_fallback_event_line(line, "scalar_event", GET_NAME(ch), "queue_full_flat_fallback"))
			return;
	}

	// Sync fallback: update database directly when worker not available
	db_query("UPDATE player_data SET epics=%ld WHERE pid=%d", ch->only.pc->epics, GET_PID(ch));
}

void manual_log(P_char ch)
{

	char a[256], b[256];
	char buf[MAX_STRING_LENGTH];
	char equip_sql[MAX_STRING_LENGTH];
	char log_sql[MAX_LOG_LEN];
	char buf2[MAX_LOG_LEN];
	int  space = MAX_LOG_LEN;

	// paranoia check
	if (!ch || !IS_PC(ch))
		return;

	if (!GET_PLAYER_LOG(ch))
	{
		logit(LOG_DEBUG, "Tried to dump player log (%s) in manual_log(), but player log was null!", GET_NAME(ch));
		return;
	}

	*buf2 = '\0';

	ITERATE_LOG(ch, LOG_PUBLIC)
	{
		strncat(buf2, LOG_MSG(), space);
		space -= strlen(LOG_MSG());

		if (space <= 0)
			break;
	}

	mysql_str(buf2, log_sql);

	snprintf(a, 256, "%d%ld", rand(), time(NULL));
	snprintf(b, 256, "%s", CRYPT2(a, ch->player.name));

	db_query("INSERT INTO MANUAL_LOG VALUES( 0, '%s', '%s', %d, 0, NOW() )", log_sql, b, GET_PID(ch));

	snprintf(buf, MAX_STRING_LENGTH, "Your log is @ '&+Whttp://duris.game-host.org/duris/php/stats/mylog.php?password=%s&n' \n", b);

	send_to_char(buf, ch, LOG_PRIVATE);
}

void sql_resetConnectTimes(void)
{
	// this should ONLY be called on mud bootup.  to ensure that, call it when sql is initialized
	db_query("UPDATE ip_info SET last_disconnect = NOW() WHERE last_connect > last_disconnect");
}

void sql_disconnectIP(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;

	db_query_nolog("INSERT INTO ip_info (pid) VALUES (%d)", GET_PID(ch));
	if (ch->desc)
	{
		// Set racewar side if not an immortal.
		db_query("UPDATE ip_info SET last_disconnect = NOW(), racewar_side=%d WHERE pid = %d", IS_TRUSTED(ch) ? RACEWAR_NONE : GET_RACEWAR(ch), GET_PID(ch));
	}
}

void sql_connectIP(P_char ch)
{
	// insert will silently fail if the PID is already in the table
	db_query_nolog("INSERT INTO ip_info (pid) VALUES (%d)", GET_PID(ch));
	if (ch->desc)
	{
		db_query("UPDATE ip_info SET last_ip = '%s', last_connect = NOW(), racewar_side = %d WHERE pid = %d", ch->desc->host, IS_TRUSTED(ch) ? RACEWAR_NONE : GET_RACEWAR(ch), GET_PID(ch));
	}
}

void sql_world_quest_finished(P_char ch, P_obj reward)
{
	MYSQL *db;
	char buf[MAX_STRING_LENGTH];
	char event_key[128];
	char event_key_sql[256];
	char line[PERSISTENCE_EVENT_MAX_LEN];
	char player_name[128];
	char reward_field[256];
	int queued = 0;

	int   reward_vnum = reward ? ((reward->R_num >= 0) ? obj_index[reward->R_num].virtual_number : 0) : 0;

	sql_reward_event_key(event_key, sizeof(event_key), "quest_complete", GET_PID(ch), ch->only.pc->quest_mob_vnum);
	snprintf(line,
	         sizeof(line),
	         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=world_quest_finished|event_key=%s|pid=%d|quest_giver=%d|player_name=%s|player_level=%d|quest_target=%d|reward_vnum=%d|reward_desc=%s",
	         (long)time(NULL),
	         event_key,
	         GET_PID(ch),
	         ch->only.pc->quest_giver,
	         sql_scalar_clean_field(GET_NAME(ch), player_name, sizeof(player_name)),
	         GET_LEVEL(ch),
	         ch->only.pc->quest_mob_vnum,
	         reward_vnum,
	         sql_scalar_clean_field(reward ? reward->short_description : "", reward_field, sizeof(reward_field)));

	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			queued = 1;
		else if (persistence_write_fallback_event_line(line, "scalar_event", GET_NAME(ch), "queue_full_flat_fallback"))
			queued = 1;
	}

	if (!queued)
	{
		char *reward_desc = reward ? mysql_str(reward->short_description, buf) : mysql_str("", buf);

		db = persistence_reward_tables_ready ? sql_persistence_connection() : NULL;
		if (!db)
		{
			db_query("INSERT INTO world_quest_accomplished (pid, timestamp, quest_giver, player_name, player_level, quest_target, reward_vnum, reward_desc) VALUES (%d, now(), %d, '%s', %d, %d, %d, '%s')",
			         GET_PID(ch),
			         ch->only.pc->quest_giver,
			         GET_NAME(ch),
			         GET_LEVEL(ch),
			         ch->only.pc->quest_mob_vnum,
			         reward_vnum,
			         reward_desc);
		}
		else
		{
			mysql_real_escape_string(db, event_key_sql, event_key, strlen(event_key));

			db_query("INSERT INTO world_quest_accomplished "
			         "(event_key, pid, timestamp, quest_giver, player_name, player_level, quest_target, reward_vnum, reward_desc) "
			         "VALUES ('%s', %d, now(), %d, '%s', %d, %d, %d, '%s') "
			         "ON DUPLICATE KEY UPDATE event_key=event_key",
			         event_key_sql,
			         GET_PID(ch),
			         ch->only.pc->quest_giver,
			         GET_NAME(ch),
			         GET_LEVEL(ch),
			         ch->only.pc->quest_mob_vnum,
			         reward_vnum,
			         reward_desc);
		}
	}

	mark_player_dirty(GET_PID(ch));
}

int sql_world_quest_can_do_another(P_char ch)
{
	// This crashed us when paly's horse called this function.
	if (!IS_PC(ch))
		return 0;

	MYSQL_RES *db = 0;
	if (GET_LEVEL(ch) < 50)
		db = db_query("SELECT count(id) FROM world_quest_accomplished where pid = %d and player_level =%d and TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 0", GET_PID(ch), GET_LEVEL(ch));
	else
		db = db_query("SELECT count(id) FROM world_quest_accomplished where pid = %d and TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 0", GET_PID(ch));

	int returning_value = 0;
	if (GET_LEVEL(ch) <= 30)
		returning_value = get_property("world.quest.max.level.30.andUnder", 6.000);
	else if (GET_LEVEL(ch) <= 40)
		returning_value = get_property("world.quest.max.level.40.andUnder", 6.000);
	else if (GET_LEVEL(ch) <= 50)
		returning_value = get_property("world.quest.max.level.50.andUnder", 6.000);
	else if (GET_LEVEL(ch) <= 55)
		returning_value = get_property("world.quest.max.level.55.andUnder", 6.000);
	else
		returning_value = get_property("world.quest.max.level.other", 6.000);

	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);
		if (NULL != row)
		{
			returning_value = returning_value - atoi(row[0]);
		}
		else
			returning_value;

		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}
	return MAX(returning_value, 0);
}

int sql_world_quest_done_already(P_char ch, int quest_target)
{

	MYSQL_RES *db              = db_query("SELECT count(id) FROM world_quest_accomplished where quest_target = %d and pid = %d", quest_target, GET_PID(ch));
	int        returning_value = 0;
	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);
		if (NULL != row)
		{
			returning_value = atoi(row[0]);
		}
		else
			returning_value = 0;

		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}
	return returning_value;
}

void sql_zone_touch_finished(const char *event_key, int boot_time, int touched_at, int zone_number, int toucher_pid, int group_size, int epic_value, int alignment_delta)
{
	MYSQL *db;
	char event_key_buf[128];
	char event_key_sql[256];
	char line[PERSISTENCE_EVENT_MAX_LEN];

	if (!event_key || !*event_key)
	{
		sql_reward_event_key(event_key_buf, sizeof(event_key_buf), "zone_touch", toucher_pid, zone_number);
		event_key = event_key_buf;
	}

	snprintf(line,
	         sizeof(line),
	         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=zone_touch|event_key=%s|boot_time=%d|touched_at=%d|zone_number=%d|toucher_pid=%d|group_size=%d|epic_value=%d|alignment_delta=%d",
	         (long)time(NULL),
	         event_key,
	         boot_time,
	         touched_at,
	         zone_number,
	         toucher_pid,
	         group_size,
	         epic_value,
	         alignment_delta);

	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return;
		if (persistence_write_fallback_event_line(line, "scalar_event", "zone_touch", "queue_full_flat_fallback"))
			return;
	}

	db = persistence_reward_tables_ready ? sql_persistence_connection() : NULL;
	if (!db)
	{
		db_query("INSERT INTO zone_touches (boot_time, touched_at, zone_number, toucher_pid, group_size, epic_value, alignment_delta) "
		         "VALUES (FROM_UNIXTIME(%d), FROM_UNIXTIME(%d), %d, %d, %d, %d, %d)",
		         boot_time,
		         touched_at,
		         zone_number,
		         toucher_pid,
		         group_size,
		         epic_value,
		         alignment_delta);
		return;
	}

	mysql_real_escape_string(db, event_key_sql, event_key, strlen(event_key));

	db_query("INSERT INTO zone_touches (event_key, boot_time, touched_at, zone_number, toucher_pid, group_size, epic_value, alignment_delta) "
	         "VALUES ('%s', FROM_UNIXTIME(%d), FROM_UNIXTIME(%d), %d, %d, %d, %d, %d) "
	         "ON DUPLICATE KEY UPDATE event_key=event_key",
	         event_key_sql,
	         boot_time,
	         touched_at,
	         zone_number,
	         toucher_pid,
	         group_size,
	         epic_value,
	         alignment_delta);
}

const char *sql_select_IP_info(P_char ch, char *buf, size_t bufSize, time_t *lastConnect, time_t *lastDisconnect)
{
	time_t now = 0;
	buf[0]     = '\0';

	MYSQL_RES *db = db_query("SELECT last_ip, UNIX_TIMESTAMP(last_connect), UNIX_TIMESTAMP(last_disconnect), UNIX_TIMESTAMP() "
	                         "FROM ip_info WHERE pid = %d",
	                         GET_PID(ch));
	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);

		if (NULL != row)
		{
			strncpy(buf, row[0] ? row[0] : "", bufSize - 2);
			buf[bufSize - 1] = '\0';
			now              = strtoul(row[3], NULL, 10);
			if (lastConnect)
			{
				*lastConnect = strtoul(row[1], NULL, 10);
				if (0 != *lastConnect)
					*lastConnect = now - *lastConnect;
			}
			if (lastDisconnect)
			{
				*lastDisconnect = strtoul(row[2], NULL, 10);
				if (0 != *lastDisconnect)
					*lastDisconnect = now - *lastDisconnect;
			}

			// cycle out until a NULL return
			while ((row = mysql_fetch_row(db)))
				;
		}
		mysql_free_result(db);
	}
	return buf;
}

// Returns the time needed *in seconds) to timeout the racewar side associated with an ip.
// Or 0 if no character has been on within an hour.
int sql_find_racewar_for_ip(char *ip, int *racewar_side)
{
	MYSQL_RES *db;
	MYSQL_ROW  row;
	time_t     last_connect, last_disconnect, hour_ago;

	db = db_query("SELECT UNIX_TIMESTAMP(last_connect), UNIX_TIMESTAMP(last_disconnect), UNIX_TIMESTAMP(), racewar_side"
	              " from ip_info WHERE last_ip = \"%s\" ORDER BY last_connect DESC LIMIT 1",
	              ip);

	if (db && ((row = mysql_fetch_row(db)) != NULL))
	{
		// Arih: fix NULL pointer crash when last_disconnect is NULL in ip_info table - 20251103
		// UNIX_TIMESTAMP() returns NULL for NULL datetime values, causing strtoul to segfault
		last_connect    = row[0] ? strtoul(row[0], NULL, 10) : 0;
		last_disconnect = row[1] ? strtoul(row[1], NULL, 10) : 0;
		hour_ago        = row[2] ? strtoul(row[2], NULL, 10) - 60 * 60 : 0;
		*racewar_side   = row[3] ? atoi(row[3]) : 0;

		// If they've been offline for an hour or more, return a 0 timer.
		if (last_disconnect > last_connect && last_disconnect <= hour_ago)
		{
			racewar_side = RACEWAR_NONE;
			while (row != NULL)
				row = mysql_fetch_row(db);
			return 0;
		}

		while (row != NULL)
			row = mysql_fetch_row(db);

		mysql_free_result(db);

		// Return an hour if they're still online, or time delta to an hour offline.
		return (last_disconnect < last_connect) ? 60 * 60 : last_disconnect - hour_ago;
	}

	if (db)
		mysql_free_result(db);
	return RACEWAR_NONE;
}

void perform_wiki_search(P_char ch, const char *query)
{

	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	char buf3[MAX_STRING_LENGTH];
	char escaped_query[MAX_STRING_LENGTH * 2 + 1]; // SECURITY: Buffer for escaped query (MySQL needs 2x+1 size)
	buf[0]  = '\0';
	buf2[0] = '\0';
	buf3[0] = '\0';
	MYSQL_ROW row;
	MYSQL_ROW row2;

	// SECURITY FIX: Sanitize user input to prevent SQL injection
	// Escape the query string using MySQL's built-in escape function
	mysql_real_escape_string(DB, escaped_query, query, strlen(query));

	/*
	MYSQL_RES *db  = db_query("SELECT UPPER(si_title) , old_id, REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(old_text,'<pre>',''),'</pre>',''), ']]', ''),'[[' ,'' ), '::', ':'), '<br>', '') FROM
	wikki_searchindex, wikki_text where old_id =( SELECT max(rev_text_id) FROM wikki_revision w where rev_page =( select si_page from wikki_searchindex where LOWER(si_title)  like LOWER('%s') limit
	1)) and si_title like LOWER('%s') limit 1", query, query);
	*/

	MYSQL_RES *db =
		db_query("SELECT REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(old_text,'<pre>',''),'</pre>',''), ']]', ''),'[[' ,'' ), '::', ':'), '<br>', ''), '\'\'', '') , '==', '') "
	             "FROM `wikki_text`  WHERE old_id = (SELECT rev_text_id FROM `wikki_page`,`wikki_revision`  WHERE (page_id=rev_page) AND rev_id = (SELECT page_latest FROM `wikki_page`  WHERE page_id "
	             "= (SELECT page_id  FROM `wikki_page`  WHERE page_namespace = '0' AND LOWER(page_title) = REPLACE(LOWER('%s'), ' ', '_')  LIMIT 1)  LIMIT 1)  LIMIT 1)  LIMIT 1",
	             escaped_query);
	if (db)
	{
		row = mysql_fetch_row(db);
		if (NULL != row)
		{
			snprintf(buf, MAX_STRING_LENGTH, "\t&+W========| &+m %s &+W |========&n\n%s", escaped_query, row[0]);
		}
		else
			snprintf(buf, MAX_STRING_LENGTH, "&+WNothing matches, see &+mHelp wiki&+W how to add this help.&n");
		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}

	/*
	  MYSQL_RES *db2 = db_query("SELECT lower(si_title), MATCH (si_text) AGAINST REPLACE(LOWER('%s'), ' ', '_') as SCORE  FROM wikki_searchindex  order by SCORE desc limit 10", query);
	  if (db2)
	  {
	    row2 = mysql_fetch_row(db2);

	    if (NULL != row2)
	    {
	        if( atoi(row2[1]) > 0)
	        {
	  strcat(buf2, "\r\n\r\n");
	  strcat(buf2, "&+WOther related topics:&n\r\n");
	        snprintf(buf3, MAX_STRING_LENGTH, "&+m%s&n, " , row2[0]);
	  strcat(buf2, buf3);
	        }

	      // cycle out until a NULL return
	        int i = 0;
	  while ((row2 = mysql_fetch_row(db2)))
	        {
	        if( atoi(row2[1]) > 0){
	  i++;
	  snprintf(buf3, MAX_STRING_LENGTH, "&+m%s&n, " , row2[0]);
	  if(i == 5)
	  strcat(buf3, "\r\n");
	  strcat(buf2, buf3);
	        }

	    }

	   }
	  }
	  */
	strcat(buf2, "\r\n");
	strcat(buf, buf2);
	send_to_char(buf, ch);
}

void sql_clear_results()
{
	int status = 0;
	do
	{
		/* did current statement return data? */
		MYSQL_RES *result = mysql_store_result(DB);
		if (result)
		{
			mysql_free_result(result);
		}
		else /* no result set or error */
		{
			if (mysql_field_count(DB) == 0)
			{
				// printf("%lld rows affected\n", mysql_affected_rows(DB));
			}
			else /* some error occurred */
			{
				logit(LOG_DEBUG, "MySQL error: %s", mysql_error(DB));
				break;
			}
		}
		/* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
		if ((status = mysql_next_result(DB)) > 0)
		{
			logit(LOG_DEBUG, "MySQL error: %s", mysql_error(DB));
			break;
		}
	} while (status == 0);
}

bool qry(const char *format, ...)
{
	char    buf[MAX_STRING_LENGTH];
	va_list args;
	int     ret;

	if (!DB)
	{
		logit(LOG_DEBUG, "MySQL error: MySQL not initialized!");
		return FALSE;
	}

	va_start(args, format);
	buf[0] = '\0';
	// SECURITY FIX: Replace vsprintf with vsnprintf to prevent buffer overflow
	ret = vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	// Check for overflow
	if (ret < 0 || ret >= (int)sizeof(buf))
	{
		logit(LOG_DEBUG, "MySQL error: Query too long or formatting error");
		return FALSE;
	}

	if (mysql_real_query(DB, buf, strlen(buf)))
	{
		logit(LOG_DEBUG, "MySQL error: %s", mysql_error(DB));
		logit(LOG_DEBUG, "on MySQL query: %s", buf);
		return FALSE;
	}

	return TRUE;
}

static MYSQL *sql_persistence_connection(void)
{
	char db_name[50];
	MYSQL *db;
	unsigned int timeout = 10;

	if (persistenceDB && mysql_ping(persistenceDB) == 0)
		return persistenceDB;

	if (persistenceDB)
	{
		mysql_close(persistenceDB);
		persistenceDB = NULL;
		persistence_tables_ready = FALSE;
		persistence_reward_tables_ready = FALSE;
	}

	snprintf(db_name, sizeof(db_name), "%s", DB_NAME);
	if (RUNNING_PORT != DFLT_PORT)
		snprintf(db_name, sizeof(db_name), "duris_dev");

	db = mysql_init(NULL);
	if (!db)
		return NULL;

	mysql_options(db, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(db, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

	if (!mysql_real_connect(db, DB_HOST, DB_USER, DB_PASSWD, db_name, DB_PORT, NULL, CLIENT_MULTI_STATEMENTS))
	{
		mysql_close(db);
		return NULL;
	}

	persistenceDB = db;
	return persistenceDB;
}

static bool sql_persistence_query(MYSQL *db, const char *query)
{
	if (!db || !query)
		return FALSE;

	if (mysql_real_query(db, query, strlen(query)))
	{
		logit(LOG_DEBUG, "Persistence MySQL error: %s", mysql_error(db));
		return FALSE;
	}

	return TRUE;
}

static bool sql_persistence_ensure_tables(MYSQL *db)
{
	if (persistence_tables_ready)
		return TRUE;

	if (!sql_persistence_query(db,
	                          "CREATE TABLE IF NOT EXISTS persistence_item_event_audit ("
	                          "event_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
	                          "event_time BIGINT UNSIGNED NOT NULL,"
	                          "event_type VARCHAR(64) NOT NULL,"
	                          "item_uid BIGINT UNSIGNED NOT NULL,"
	                          "owner_type VARCHAR(32) NOT NULL,"
	                          "owner_ref VARCHAR(64) NOT NULL,"
	                          "actor_id INT NOT NULL DEFAULT -1,"
	                          "vnum INT NOT NULL DEFAULT -1,"
	                          "item_name VARCHAR(255) NOT NULL DEFAULT '',"
	                          "raw_event TEXT NOT NULL,"
	                          "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
	                          "PRIMARY KEY (event_id),"
	                          "KEY item_time (item_uid,event_time),"
	                          "KEY owner_lookup (owner_type,owner_ref)"
	                          ") ENGINE=InnoDB"))
		return FALSE;

	if (!sql_persistence_query(db,
	                          "CREATE TABLE IF NOT EXISTS persistence_items_current ("
	                          "item_uid BIGINT UNSIGNED NOT NULL,"
	                          "owner_type VARCHAR(32) NOT NULL,"
	                          "owner_ref VARCHAR(64) NOT NULL,"
	                          "event_time BIGINT UNSIGNED NOT NULL,"
	                          "event_type VARCHAR(64) NOT NULL,"
	                          "actor_id INT NOT NULL DEFAULT -1,"
	                          "vnum INT NOT NULL DEFAULT -1,"
	                          "item_name VARCHAR(255) NOT NULL DEFAULT '',"
	                          "updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP "
	                          "ON UPDATE CURRENT_TIMESTAMP,"
	                          "PRIMARY KEY (item_uid),"
	                          "KEY owner_lookup (owner_type,owner_ref)"
	                          ") ENGINE=InnoDB"))
		return FALSE;

	if (!sql_persistence_query(db,
	                          "CREATE TABLE IF NOT EXISTS persistence_item_conflicts ("
	                          "conflict_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
	                          "item_uid BIGINT UNSIGNED NOT NULL,"
	                          "existing_owner_type VARCHAR(32) NOT NULL,"
	                          "existing_owner_ref VARCHAR(64) NOT NULL,"
	                          "existing_event_time BIGINT UNSIGNED NOT NULL,"
	                          "incoming_owner_type VARCHAR(32) NOT NULL,"
	                          "incoming_owner_ref VARCHAR(64) NOT NULL,"
	                          "incoming_event_time BIGINT UNSIGNED NOT NULL,"
	                          "incoming_event_type VARCHAR(64) NOT NULL,"
	                          "resolution VARCHAR(64) NOT NULL,"
	                          "raw_event TEXT NOT NULL,"
	                          "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
	                          "PRIMARY KEY (conflict_id),"
	                          "KEY item_created (item_uid,created_at),"
	                          "KEY resolution_lookup (resolution)"
	                          ") ENGINE=InnoDB"))
		return FALSE;

	persistence_tables_ready = TRUE;
	return TRUE;
}

static bool sql_persistence_column_exists(MYSQL *db, const char *table_name, const char *column_name)
{
	char query[512];
	char table_sql[128];
	char column_sql[128];
	MYSQL_RES *res;
	MYSQL_ROW row;
	bool exists = FALSE;

	if (!db || !table_name || !column_name)
		return FALSE;

	mysql_real_escape_string(db, table_sql, table_name, strlen(table_name));
	mysql_real_escape_string(db, column_sql, column_name, strlen(column_name));
	snprintf(query,
	         sizeof(query),
	         "SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS "
	         "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '%s' AND COLUMN_NAME = '%s'",
	         table_sql,
	         column_sql);

	if (mysql_real_query(db, query, strlen(query)))
		return FALSE;

	res = mysql_store_result(db);
	if (!res)
		return FALSE;

	row = mysql_fetch_row(res);
	if (row && row[0] && atoi(row[0]) > 0)
		exists = TRUE;

	mysql_free_result(res);
	return exists;
}

static bool sql_persistence_index_exists(MYSQL *db, const char *table_name, const char *index_name)
{
	char query[512];
	char table_sql[128];
	char index_sql[128];
	MYSQL_RES *res;
	MYSQL_ROW row;
	bool exists = FALSE;

	if (!db || !table_name || !index_name)
		return FALSE;

	mysql_real_escape_string(db, table_sql, table_name, strlen(table_name));
	mysql_real_escape_string(db, index_sql, index_name, strlen(index_name));
	snprintf(query,
	         sizeof(query),
	         "SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS "
	         "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '%s' AND INDEX_NAME = '%s'",
	         table_sql,
	         index_sql);

	if (mysql_real_query(db, query, strlen(query)))
		return FALSE;

	res = mysql_store_result(db);
	if (!res)
		return FALSE;

	row = mysql_fetch_row(res);
	if (row && row[0] && atoi(row[0]) > 0)
		exists = TRUE;

	mysql_free_result(res);
	return exists;
}

static bool sql_persistence_ensure_event_key(MYSQL *db, const char *table_name)
{
	char query[512];

	if (!db || !table_name)
		return FALSE;

	if (!sql_persistence_column_exists(db, table_name, "event_key"))
	{
		snprintf(query, sizeof(query), "ALTER TABLE %s ADD COLUMN event_key VARCHAR(128) DEFAULT NULL", table_name);
		if (!sql_persistence_query(db, query))
			return FALSE;
	}

	if (!sql_persistence_index_exists(db, table_name, "reward_event_key"))
	{
		snprintf(query, sizeof(query), "ALTER TABLE %s ADD UNIQUE KEY reward_event_key (event_key)", table_name);
		if (!sql_persistence_query(db, query))
			return FALSE;
	}

	return TRUE;
}

static bool sql_persistence_ensure_index(MYSQL *db, const char *table_name, const char *index_name, const char *index_sql)
{
	char query[512];

	if (!db || !table_name || !index_name || !index_sql)
		return FALSE;

	if (sql_persistence_index_exists(db, table_name, index_name))
		return TRUE;

	snprintf(query, sizeof(query), "ALTER TABLE %s ADD KEY %s %s", table_name, index_name, index_sql);
	return sql_persistence_query(db, query);
}

static bool sql_persistence_ensure_reward_tables(MYSQL *db)
{
	if (persistence_reward_tables_ready)
		return TRUE;

	if (!db)
		return FALSE;

	if (!sql_persistence_ensure_event_key(db, "epic_gain"))
		return FALSE;

	if (!sql_persistence_ensure_event_key(db, "world_quest_accomplished"))
		return FALSE;

	if (!sql_persistence_ensure_event_key(db, "zone_touches"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "epic_gain", "epic_gain_pid_type_time", "(pid,type,time)"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "epic_gain", "epic_gain_pid_type_id", "(pid,type,type_id)"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "world_quest_accomplished", "world_quest_pid_level_time", "(pid,player_level,timestamp)"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "world_quest_accomplished", "world_quest_pid_target", "(pid,quest_target)"))
		return FALSE;

	persistence_reward_tables_ready = TRUE;
	return TRUE;
}

static bool sql_schema_ensure_column(MYSQL *db, const char *table_name, const char *column_name, const char *alter_sql)
{
	char query[512];

	if (!db || !table_name || !column_name || !alter_sql)
		return FALSE;

	if (sql_persistence_column_exists(db, table_name, column_name))
		return TRUE;

	snprintf(query, sizeof(query), "ALTER TABLE %s %s", table_name, alter_sql);
	return sql_persistence_query(db, query);
}

static bool sql_schema_ensure_obj_uid_columns(MYSQL *db, const char *table_name)
{
	char query[512];

	if (!db || !table_name)
		return FALSE;

	if (!sql_persistence_column_exists(db, table_name, "obj_uid"))
	{
		if (sql_persistence_column_exists(db, table_name, "unique_id"))
		{
			snprintf(query,
			         sizeof(query),
			         "ALTER TABLE %s CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL",
			         table_name);
			if (!sql_persistence_query(db, query))
				return FALSE;
		}
		else
		{
			snprintf(query,
			         sizeof(query),
			         "ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL");
			if (!sql_schema_ensure_column(db, table_name, "obj_uid", query))
				return FALSE;
		}
	}

	if (!sql_schema_ensure_column(db,
	                              table_name,
	                              "item_condition",
	                              "ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, table_name, "idx_obj_uid", "(obj_uid)"))
		return FALSE;

	return TRUE;
}

static bool sql_schema_ensure_persistent_item_columns(MYSQL *db, const char *table_name, bool has_chests)
{
	if (!db || !table_name)
		return FALSE;

	if (has_chests && !sql_schema_ensure_column(db,
	                                            table_name,
	                                            "chest_id",
	                                            "ADD COLUMN chest_id INT UNSIGNED DEFAULT NULL AFTER locker_id"))
		return FALSE;

	if (!sql_schema_ensure_column(db,
	                              table_name,
	                              "wear_flags",
	                              "ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags"))
		return FALSE;

	if (!sql_schema_ensure_column(db,
	                              table_name,
	                              "item_type",
	                              "ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags"))
		return FALSE;

	if (!sql_schema_ensure_column(db,
	                              table_name,
	                              "bitvector1",
	                              "ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER action_descr"))
		return FALSE;

	if (!sql_schema_ensure_column(db,
	                              table_name,
	                              "bitvector2",
	                              "ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1"))
		return FALSE;

	if (!sql_schema_ensure_column(db,
	                              table_name,
	                              "bitvector3",
	                              "ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2"))
		return FALSE;

	if (!sql_schema_ensure_column(db,
	                              table_name,
	                              "bitvector4",
	                              "ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3"))
		return FALSE;

	if (!sql_schema_ensure_column(db,
	                              table_name,
	                              "bitvector5",
	                              "ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4"))
		return FALSE;

	return sql_schema_ensure_obj_uid_columns(db, table_name);
}

static bool sql_schema_ensure_locker_tables(MYSQL *db)
{
	if (!db)
		return FALSE;

	if (!sql_schema_ensure_persistent_item_columns(db, "player_items", FALSE))
		return FALSE;

	if (!sql_schema_ensure_persistent_item_columns(db, "locker_items", TRUE))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "locker_items", "idx_locker_chest", "(locker_id,chest_id)"))
		return FALSE;

	if (!sql_persistence_query(db,
	                          "CREATE TABLE IF NOT EXISTS private_chests ("
	                          "id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
	                          "locker_id INT UNSIGNED NOT NULL,"
	                          "chest_name VARCHAR(32) NOT NULL,"
	                          "password_hash VARCHAR(64) DEFAULT NULL,"
	                          "is_public TINYINT(1) DEFAULT 0,"
	                          "sort_config TEXT DEFAULT NULL,"
	                          "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
	                          "FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,"
	                          "UNIQUE KEY uk_locker_chest (locker_id,chest_name),"
	                          "INDEX idx_locker_id (locker_id)"
	                          ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
		return FALSE;

	if (!sql_schema_ensure_column(db,
	                              "private_chests",
	                              "sort_config",
	                              "ADD COLUMN sort_config TEXT DEFAULT NULL AFTER is_public"))
		return FALSE;

	if (!sql_persistence_query(db,
	                          "CREATE TABLE IF NOT EXISTS private_chest_log ("
	                          "id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
	                          "locker_id INT UNSIGNED NOT NULL,"
	                          "chest_id INT UNSIGNED DEFAULT NULL,"
	                          "char_name VARCHAR(64) NOT NULL,"
	                          "action_type ENUM('open','close','put','get','fail') NOT NULL,"
	                          "item_short VARCHAR(256) DEFAULT NULL,"
	                          "logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
	                          "FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,"
	                          "INDEX idx_locker_id (locker_id),"
	                          "INDEX idx_logged_at (logged_at)"
	                          ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
		return FALSE;

	if (!sql_persistence_query(db,
	                          "INSERT IGNORE INTO private_chests (locker_id,chest_name,is_public) "
	                          "SELECT id,'public',1 FROM lockers WHERE locker_name LIKE 'account.%'"))
		return FALSE;

	return TRUE;
}

static bool sql_schema_ensure_corpse_tables(MYSQL *db)
{
	if (!db)
		return FALSE;

	if (!sql_persistence_query(db,
	                          "CREATE TABLE IF NOT EXISTS corpses ("
	                          "id INT AUTO_INCREMENT PRIMARY KEY,"
	                          "player_name VARCHAR(50) NOT NULL,"
	                          "save_id BIGINT NOT NULL,"
	                          "room_vnum INT DEFAULT 0,"
	                          "short_descr VARCHAR(512) DEFAULT NULL,"
	                          "description TEXT DEFAULT NULL,"
	                          "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
	                          "UNIQUE KEY uk_player_saveid (player_name, save_id),"
	                          "INDEX idx_player_name (player_name)"
	                          ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
		return FALSE;

	if (!sql_schema_ensure_column(db, "corpses", "short_descr", "ADD COLUMN short_descr VARCHAR(512) DEFAULT NULL"))
		return FALSE;

	if (!sql_schema_ensure_column(db, "corpses", "description", "ADD COLUMN description TEXT DEFAULT NULL"))
		return FALSE;

	if (!sql_persistence_query(db,
	                          "CREATE TABLE IF NOT EXISTS corpse_items ("
	                          "id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
	                          "corpse_id INT NOT NULL,"
	                          "vnum INT NOT NULL,"
	                          "item_type INT NOT NULL DEFAULT 0,"
	                          "container_id INT UNSIGNED DEFAULT NULL,"
	                          "quantity SMALLINT UNSIGNED DEFAULT 1,"
	                          "weight INT DEFAULT 0,"
	                          "cost INT DEFAULT 0,"
	                          "timer INT DEFAULT -1,"
	                          "extra_flags BIGINT UNSIGNED DEFAULT 0,"
	                          "value0 INT DEFAULT 0, value1 INT DEFAULT 0, value2 INT DEFAULT 0, value3 INT DEFAULT 0,"
	                          "value4 INT DEFAULT 0, value5 INT DEFAULT 0, value6 INT DEFAULT 0, value7 INT DEFAULT 0,"
	                          "name VARCHAR(512) DEFAULT NULL,"
	                          "short_descr VARCHAR(512) DEFAULT NULL,"
	                          "description TEXT DEFAULT NULL,"
	                          "action_descr TEXT DEFAULT NULL,"
	                          "obj_uid BIGINT UNSIGNED DEFAULT NULL,"
	                          "item_condition SMALLINT DEFAULT 100,"
	                          "INDEX idx_corpse_id (corpse_id),"
	                          "INDEX idx_vnum (vnum),"
	                          "INDEX idx_obj_uid (obj_uid)"
	                          ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
		return FALSE;

	if (!sql_schema_ensure_column(db, "corpse_items", "item_type", "ADD COLUMN item_type INT NOT NULL DEFAULT 0 AFTER vnum"))
		return FALSE;

	if (!sql_persistence_column_exists(db, "corpse_items", "obj_uid"))
	{
		if (sql_persistence_column_exists(db, "corpse_items", "unique_id"))
		{
			if (!sql_persistence_query(db,
			                           "ALTER TABLE corpse_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL"))
				return FALSE;
		}
		else if (!sql_schema_ensure_column(db, "corpse_items", "obj_uid", "ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL"))
		{
			return FALSE;
		}
	}

	if (!sql_schema_ensure_column(db, "corpse_items", "item_condition", "ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "corpse_items", "idx_obj_uid", "(obj_uid)"))
		return FALSE;

	if (!sql_persistence_query(db,
	                          "CREATE TABLE IF NOT EXISTS corpse_item_affects ("
	                          "id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
	                          "item_id INT UNSIGNED NOT NULL,"
	                          "location TINYINT UNSIGNED DEFAULT 0,"
	                          "modifier INT DEFAULT 0,"
	                          "INDEX idx_item_id (item_id)"
	                          ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
		return FALSE;

	if (!sql_persistence_query(db,
	                          "CREATE TABLE IF NOT EXISTS corpse_item_extra_descr ("
	                          "id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
	                          "item_id INT UNSIGNED NOT NULL,"
	                          "keyword VARCHAR(255) NOT NULL,"
	                          "description TEXT,"
	                          "INDEX idx_item_id (item_id)"
	                          ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
		return FALSE;

	return TRUE;
}

static bool sql_schema_ensure_auction_schema(MYSQL *db)
{
	if (!db)
		return FALSE;

	if (!sql_schema_ensure_column(db, "auctions", "obj_info_text", "ADD COLUMN obj_info_text TEXT DEFAULT NULL"))
		return FALSE;

	if (!sql_schema_ensure_column(db, "auction_item_pickups", "source_auction_id", "ADD COLUMN source_auction_id INT UNSIGNED DEFAULT NULL"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "auction_item_pickups", "idx_source_auction_id", "(source_auction_id)"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "auction_item_pickups", "idx_pid_retrieved", "(pid,retrieved,id)"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "auctions", "idx_status_end_id", "(status,end_time,id)"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "auctions", "idx_seller_status_end", "(seller_name,status,end_time)"))
		return FALSE;

	if (!sql_persistence_ensure_index(db, "auction_bid_history", "idx_auction_date", "(auction_id,date)"))
		return FALSE;

	return TRUE;
}

static bool sql_ensure_runtime_schema(MYSQL *db)
{
	if (!db)
		return FALSE;

	if (!sql_persistence_ensure_tables(db))
		return FALSE;

	if (!sql_persistence_ensure_reward_tables(db))
		return FALSE;

	if (!sql_schema_ensure_corpse_tables(db))
		return FALSE;

	if (!sql_schema_ensure_locker_tables(db))
		return FALSE;

	if (!sql_schema_ensure_auction_schema(db))
		return FALSE;

	return TRUE;
}

static void sql_reward_event_key(char *buf, int buf_size, const char *type, int pid, int source_id)
{
	if (!buf || buf_size <= 0)
		return;

	persistence_reward_event_sequence++;
	snprintf(buf, buf_size, "%s:%d:%d:%ld:%lu", type ? type : "reward", pid, source_id, (long)time(NULL), persistence_reward_event_sequence);
}

static const char *sql_scalar_clean_field(const char *in, char *buf, int buf_size)
{
	int i;

	if (!buf || buf_size <= 0)
		return "";

	if (!in)
	{
		buf[0] = '\0';
		return buf;
	}

	for (i = 0; in[i] && i < buf_size - 1; i++)
	{
		if (in[i] == '|' || in[i] == '\r' || in[i] == '\n')
			buf[i] = ' ';
		else
			buf[i] = in[i];
	}
	buf[i] = '\0';
	return buf;
}

static void persistence_line_field(const char *token, char *key, int key_size,
                                   char *value, int value_size)
{
	const char *eq;
	int key_len;

	if (key && key_size > 0)
		key[0] = '\0';
	if (value && value_size > 0)
		value[0] = '\0';

	if (!token)
		return;

	eq = strchr(token, '=');
	if (!eq)
		return;

	key_len = (int)(eq - token);
	if (key && key_size > 0)
	{
		key_len = MIN(key_len, key_size - 1);
		strncpy(key, token, key_len);
		key[key_len] = '\0';
	}

	if (value && value_size > 0)
		snprintf(value, value_size, "%s", eq + 1);
}

static void persistence_parse_owner(const char *target, char *owner_type,
                                    int owner_type_size, char *owner_ref,
                                    int owner_ref_size)
{
	const char *colon;
	int len;

	snprintf(owner_type, owner_type_size, "unknown");
	snprintf(owner_ref, owner_ref_size, "unknown");

	if (!target || !*target)
		return;

	if (!str_cmp(target, "destroyed"))
	{
		snprintf(owner_type, owner_type_size, "destroyed");
		snprintf(owner_ref, owner_ref_size, "0");
		return;
	}

	colon = strchr(target, ':');
	if (!colon)
	{
		snprintf(owner_ref, owner_ref_size, "%s", target);
		return;
	}

	len = (int)(colon - target);
	len = MIN(len, owner_type_size - 1);
	strncpy(owner_type, target, len);
	owner_type[len] = '\0';
	snprintf(owner_ref, owner_ref_size, "%s", colon + 1);
}

static bool sql_persistence_write_item_event_line_locked(const char *line)
{
	MYSQL *db;
	char copy[PERSISTENCE_EVENT_MAX_LEN];
	char *saveptr = NULL;
	char *token;
	char key[64], value[512];
	char event_type[64] = "unknown";
	char target[128] = "unknown";
	char owner_type[32], owner_ref[64];
	char item_name[MAX_INPUT_LENGTH] = "";
	char item_sql[MAX_STRING_LENGTH];
	char event_sql[256], owner_type_sql[128], owner_ref_sql[256];
	char raw_sql[PERSISTENCE_EVENT_MAX_LEN * 2 + 1];
	char query[MAX_STRING_LENGTH * 2];
	unsigned long long item_uid = 0;
	unsigned long long event_time = 0;
	int actor_id = -1;
	int vnum = -1;
	my_ulonglong conflicts_logged = 0;

	if (!line || !*line)
		return FALSE;

	db = sql_persistence_connection();
	if (!db || !sql_persistence_ensure_tables(db))
		return FALSE;

	snprintf(copy, sizeof(copy), "%s", line);
	token = strtok_r(copy, "|", &saveptr);
	while (token)
	{
		persistence_line_field(token, key, sizeof(key), value, sizeof(value));

		if (!str_cmp(key, "ts"))
			event_time = strtoull(value, NULL, 10);
		else if (!str_cmp(key, "event"))
			snprintf(event_type, sizeof(event_type), "%s", value);
		else if (!str_cmp(key, "item_uid"))
			item_uid = strtoull(value, NULL, 10);
		else if (!str_cmp(key, "vnum"))
			vnum = atoi(value);
		else if (!str_cmp(key, "item"))
			snprintf(item_name, sizeof(item_name), "%s", value);
		else if (!str_cmp(key, "actor_id"))
			actor_id = atoi(value);
		else if (!str_cmp(key, "target"))
			snprintf(target, sizeof(target), "%s", value);

		token = strtok_r(NULL, "|", &saveptr);
	}

	if (!item_uid)
		return FALSE;

	if (!event_time)
		event_time = time(NULL);

	persistence_parse_owner(target, owner_type, sizeof(owner_type),
	                        owner_ref, sizeof(owner_ref));

	mysql_real_escape_string(db, event_sql, event_type, strlen(event_type));
	mysql_real_escape_string(db, owner_type_sql, owner_type, strlen(owner_type));
	mysql_real_escape_string(db, owner_ref_sql, owner_ref, strlen(owner_ref));
	mysql_real_escape_string(db, item_sql, item_name, strlen(item_name));
	mysql_real_escape_string(db, raw_sql, line, strlen(line));

	snprintf(query, sizeof(query),
	         "INSERT INTO persistence_item_event_audit "
	         "(event_time,event_type,item_uid,owner_type,owner_ref,actor_id,vnum,item_name,raw_event) "
	         "VALUES (%llu,'%s',%llu,'%s','%s',%d,%d,'%s','%s')",
	         event_time, event_sql, item_uid, owner_type_sql, owner_ref_sql,
	         actor_id, vnum, item_sql, raw_sql);

	if (!sql_persistence_query(db, query))
		return FALSE;

	snprintf(query, sizeof(query),
	         "INSERT INTO persistence_item_conflicts "
	         "(item_uid,existing_owner_type,existing_owner_ref,existing_event_time,"
	         "incoming_owner_type,incoming_owner_ref,incoming_event_time,"
	         "incoming_event_type,resolution,raw_event) "
	         "SELECT item_uid,owner_type,owner_ref,event_time,'%s','%s',%llu,'%s',"
	         "IF(%llu >= event_time,'newest_incoming_wins','existing_newer_wins'),'%s' "
	         "FROM persistence_items_current "
	         "WHERE item_uid=%llu "
	         "AND (owner_type <> '%s' OR owner_ref <> '%s')",
	         owner_type_sql,
	         owner_ref_sql,
	         event_time,
	         event_sql,
	         event_time,
	         raw_sql,
	         item_uid,
	         owner_type_sql,
	         owner_ref_sql);

	if (!sql_persistence_query(db, query))
		return FALSE;

	conflicts_logged = mysql_affected_rows(db);
	if (conflicts_logged > 0)
	{
		logit(LOG_FILE,
		      "PERSISTENCE: domain=item_event owner=sql item_uid=%llu event_id=none action=owner_conflict detail=incoming %s:%s at %llu conflicted with current owner; newest owner will win",
		      item_uid, owner_type, owner_ref, event_time);
		logit(LOG_WIZ,
		      "PERSISTENCE: domain=item_event owner=sql item_uid=%llu event_id=none action=owner_conflict detail=incoming %s:%s at %llu conflicted with current owner; newest owner will win",
		      item_uid, owner_type, owner_ref, event_time);
	}

	snprintf(query, sizeof(query),
	         "INSERT INTO persistence_items_current "
	         "(item_uid,owner_type,owner_ref,event_time,event_type,actor_id,vnum,item_name) "
	         "VALUES (%llu,'%s','%s',%llu,'%s',%d,%d,'%s') "
	         "ON DUPLICATE KEY UPDATE "
	         "owner_type=IF(VALUES(event_time) >= event_time, VALUES(owner_type), owner_type),"
	         "owner_ref=IF(VALUES(event_time) >= event_time, VALUES(owner_ref), owner_ref),"
	         "event_type=IF(VALUES(event_time) >= event_time, VALUES(event_type), event_type),"
	         "actor_id=IF(VALUES(event_time) >= event_time, VALUES(actor_id), actor_id),"
	         "vnum=IF(VALUES(event_time) >= event_time, VALUES(vnum), vnum),"
	         "item_name=IF(VALUES(event_time) >= event_time, VALUES(item_name), item_name),"
	         "event_time=GREATEST(event_time, VALUES(event_time))",
	         item_uid, owner_type_sql, owner_ref_sql, event_time, event_sql,
	         actor_id, vnum, item_sql);

	return sql_persistence_query(db, query);
}

bool sql_persistence_write_item_event_line(const char *line)
{
	bool ok;

	pthread_mutex_lock(&persistence_sql_mutex);
	ok = sql_persistence_write_item_event_line_locked(line);
	pthread_mutex_unlock(&persistence_sql_mutex);

	return ok;
}

static bool sql_persistence_item_owner_matches_locked(unsigned long long item_uid,
                                                      const char *owner_type,
                                                      const char *owner_ref,
                                                      const char *context)
{
	MYSQL *db;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[512];
	bool matches = TRUE;

	if (!item_uid || !owner_type || !*owner_type || !owner_ref || !*owner_ref)
		return TRUE;

	db = sql_persistence_connection();
	if (!db || !sql_persistence_ensure_tables(db))
		return TRUE;

	snprintf(query,
	         sizeof(query),
	         "SELECT owner_type, owner_ref, event_time "
	         "FROM persistence_items_current WHERE item_uid=%llu LIMIT 1",
	         item_uid);

	if (!sql_persistence_query(db, query))
		return TRUE;

	res = mysql_store_result(db);
	if (!res)
		return TRUE;

	row = mysql_fetch_row(res);
	if (row && (str_cmp(row[0] ? row[0] : "", owner_type) ||
	            str_cmp(row[1] ? row[1] : "", owner_ref)))
	{
		matches = FALSE;
		logit(LOG_FILE,
		      "PERSISTENCE: domain=item_load owner=%s:%s item_uid=%llu event_id=none action=stale_owner_skip detail=context=%s current=%s:%s at %s",
		      owner_type,
		      owner_ref,
		      item_uid,
		      context ? context : "load",
		      row[0] ? row[0] : "unknown",
		      row[1] ? row[1] : "unknown",
		      row[2] ? row[2] : "unknown");
		logit(LOG_WIZ,
		      "PERSISTENCE: skipped stale item uid=%llu for %s:%s during %s; current owner is %s:%s.",
		      item_uid,
		      owner_type,
		      owner_ref,
		      context ? context : "load",
		      row[0] ? row[0] : "unknown",
		      row[1] ? row[1] : "unknown");
	}

	mysql_free_result(res);
	return matches;
}

bool sql_persistence_item_owner_matches(unsigned long long item_uid,
                                        const char *owner_type,
                                        const char *owner_ref,
                                        const char *context)
{
	bool ok;

	pthread_mutex_lock(&persistence_sql_mutex);
	ok = sql_persistence_item_owner_matches_locked(item_uid, owner_type, owner_ref, context);
	pthread_mutex_unlock(&persistence_sql_mutex);

	return ok;
}

static bool sql_persistence_write_scalar_event_line_locked(const char *line)
{
	MYSQL *db;
	char copy[PERSISTENCE_EVENT_MAX_LEN];
	char *saveptr = NULL;
	char *token;
	char key[64], value[512];
	char event_type[64] = "unknown";
	char event_key[128] = "";
	char player_name[128] = "";
	char reward_desc[256] = "";
	char event_key_sql[256];
	char player_name_sql[256];
	char reward_desc_sql[512];
	char bidder_name[128] = "";
	char bidder_name_sql[256];
	char query[MAX_STRING_LENGTH];
	int pid = 0;
	int level = 0;
	int copper = 0;
	int silver = 0;
	int gold = 0;
	int platinum = 0;
	int bank_copper = 0;
	int bank_silver = 0;
	int bank_gold = 0;
	int bank_platinum = 0;
	int played_time = 0;
	long epics = 0;
	int type = 0;
	int type_id = 0;
	int quest_giver = 0;
	int player_level = 0;
	int quest_target = 0;
	int reward_vnum = 0;
	int item_vnum = 0;
	int sale_value = 0;
	int seller_pid = 0;
	int mob_vnum = 0;
	int reward_type = 0;
	int reward_value = 0;
	int auction_id = 0;
	int bid_amount = 0;
	int delta = 0;
	int total_frags = 0;
	int boot_time = 0;
	int touched_at = 0;
	char player_description[280] = "";
	char equip[280] = "";
	char log[280] = "";
	char pk_type_sql[64] = "";
	char save_acct_name[128] = "";
	char save_race[64] = "";
	char save_class[64] = "";
	int racewar_val = 0;
	int zone_number = 0;
	int toucher_pid = 0;
	int group_size = 0;
	int epic_value = 0;
	int alignment_delta = 0;

	if (!line || !*line)
		return FALSE;

	db = sql_persistence_connection();
	if (!db)
		return FALSE;

	snprintf(copy, sizeof(copy), "%s", line);
	token = strtok_r(copy, "|", &saveptr);
	while (token)
	{
		persistence_line_field(token, key, sizeof(key), value, sizeof(value));

		if (!str_cmp(key, "event"))
			snprintf(event_type, sizeof(event_type), "%s", value);
		else if (!str_cmp(key, "event_key"))
			snprintf(event_key, sizeof(event_key), "%s", value);
		else if (!str_cmp(key, "pid"))
			pid = atoi(value);
		else if (!str_cmp(key, "level"))
			level = atoi(value);
		else if (!str_cmp(key, "copper"))
			copper = atoi(value);
		else if (!str_cmp(key, "silver"))
			silver = atoi(value);
		else if (!str_cmp(key, "gold"))
			gold = atoi(value);
		else if (!str_cmp(key, "platinum"))
			platinum = atoi(value);
		else if (!str_cmp(key, "bank_copper"))
			bank_copper = atoi(value);
		else if (!str_cmp(key, "bank_silver"))
			bank_silver = atoi(value);
		else if (!str_cmp(key, "bank_gold"))
			bank_gold = atoi(value);
		else if (!str_cmp(key, "bank_platinum"))
			bank_platinum = atoi(value);
		else if (!str_cmp(key, "played_time"))
			played_time = atoi(value);
		else if (!str_cmp(key, "epics"))
			epics = atol(value);
		else if (!str_cmp(key, "type"))
			type = atoi(value);
		else if (!str_cmp(key, "type_id"))
			type_id = atoi(value);
		else if (!str_cmp(key, "quest_giver"))
			quest_giver = atoi(value);
		else if (!str_cmp(key, "player_name"))
			snprintf(player_name, sizeof(player_name), "%s", value);
		else if (!str_cmp(key, "killer_name"))
			snprintf(player_name, sizeof(player_name), "%s", value);
		else if (!str_cmp(key, "player_level"))
			player_level = atoi(value);
		else if (!str_cmp(key, "quest_target"))
			quest_target = atoi(value);
		else if (!str_cmp(key, "reward_vnum"))
			reward_vnum = atoi(value);
		else if (!str_cmp(key, "reward_desc"))
			snprintf(reward_desc, sizeof(reward_desc), "%s", value);
		else if (!str_cmp(key, "item_vnum"))
			item_vnum = atoi(value);
		else if (!str_cmp(key, "sale_value"))
			sale_value = atoi(value);
		else if (!str_cmp(key, "seller_pid"))
			seller_pid = atoi(value);
		else if (!str_cmp(key, "mob_vnum"))
			mob_vnum = atoi(value);
		else if (!str_cmp(key, "reward_type"))
			reward_type = atoi(value);
		else if (!str_cmp(key, "reward_value"))
			reward_value = atoi(value);
		else if (!str_cmp(key, "auction_id"))
			auction_id = atoi(value);
		else if (!str_cmp(key, "bidder_name"))
			snprintf(bidder_name, sizeof(bidder_name), "%s", value);
		else if (!str_cmp(key, "bid_amount"))
			bid_amount = atoi(value);
		else if (!str_cmp(key, "boot_time"))
			boot_time = atoi(value);
		else if (!str_cmp(key, "touched_at"))
			touched_at = atoi(value);
		else if (!str_cmp(key, "zone_number"))
			zone_number = atoi(value);
		else if (!str_cmp(key, "toucher_pid"))
			toucher_pid = atoi(value);
		else if (!str_cmp(key, "group_size"))
			group_size = atoi(value);
		else if (!str_cmp(key, "epic_value"))
			epic_value = atoi(value);
		else if (!str_cmp(key, "alignment_delta"))
			alignment_delta = atoi(value);
		else if (!str_cmp(key, "last_touch"))
			;
		else if (!str_cmp(key, "reset_perc"))
			alignment_delta = atoi(value);
		else if (!str_cmp(key, "delta"))
			delta = atoi(value);
		else if (!str_cmp(key, "total_frags"))
			total_frags = atoi(value);
		else if (!str_cmp(key, "pkill_event"))
			type_id = atoi(value);
		else if (!str_cmp(key, "pk_type"))
			snprintf(pk_type_sql, sizeof(pk_type_sql), "%s", value);
		else if (!str_cmp(key, "player_description"))
			snprintf(player_description, sizeof(player_description), "%s", value);
		else if (!str_cmp(key, "equip"))
			snprintf(equip, sizeof(equip), "%s", value);
		else if (!str_cmp(key, "log"))
			snprintf(log, sizeof(log), "%s", value);
		else if (!str_cmp(key, "inroom"))
			type = atoi(value);
		else if (!str_cmp(key, "leader"))
			auction_id = atoi(value);
		else if (!str_cmp(key, "name"))
			snprintf(player_name, sizeof(player_name), "%s", value);
		else if (!str_cmp(key, "account_name"))
			snprintf(save_acct_name, sizeof(save_acct_name), "%s", value);
		else if (!str_cmp(key, "char_name"))
			snprintf(bidder_name, sizeof(bidder_name), "%s", value);
		else if (!str_cmp(key, "race"))
			snprintf(save_race, sizeof(save_race), "%s", value);
		else if (!str_cmp(key, "class_name"))
			snprintf(save_class, sizeof(save_class), "%s", value);
		else if (!str_cmp(key, "racewar"))
			racewar_val = atoi(value);

		token = strtok_r(NULL, "|", &saveptr);
	}

	if (pid <= 0 && str_cmp(event_type, "zone_touch")
	    && str_cmp(event_type, "zone_table_update")
	    && str_cmp(event_type, "zone_alignment_update"))
		return FALSE;

	if (!str_cmp(event_type, "player_level"))
	{
		snprintf(query, sizeof(query), "UPDATE player_data SET level=%d WHERE pid=%d", level, pid);
	}
	else if (!str_cmp(event_type, "player_money"))
	{
		snprintf(query,
		         sizeof(query),
		         "UPDATE player_data SET copper=%d, silver=%d, gold=%d, platinum=%d, "
		         "bank_copper=%d, bank_silver=%d, bank_gold=%d, bank_platinum=%d WHERE pid=%d",
		         copper,
		         silver,
		         gold,
		         platinum,
		         bank_copper,
		         bank_silver,
		         bank_gold,
		         bank_platinum,
		         pid);
	}
	else if (!str_cmp(event_type, "player_playtime"))
	{
		snprintf(query, sizeof(query), "UPDATE player_data SET played_time=%d WHERE pid=%d", played_time, pid);
	}
	else if (!str_cmp(event_type, "player_epics"))
	{
		snprintf(query, sizeof(query), "UPDATE player_data SET epics=%ld WHERE pid=%d", epics, pid);
	}
	else if (!str_cmp(event_type, "epic_gain"))
	{
		if (!sql_persistence_ensure_reward_tables(db))
			return FALSE;

		if (!event_key[0])
			sql_reward_event_key(event_key, sizeof(event_key), "epic_gain", pid, type_id);

		mysql_real_escape_string(db, event_key_sql, event_key, strlen(event_key));
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO epic_gain (event_key, pid, time, type, type_id, epics) "
		         "VALUES ('%s', '%d', now(), '%d', '%d', '%ld') "
		         "ON DUPLICATE KEY UPDATE event_key=event_key",
		         event_key_sql,
		         pid,
		         type,
		         type_id,
		         epics);
	}
	else if (!str_cmp(event_type, "world_quest_finished"))
	{
		if (!sql_persistence_ensure_reward_tables(db))
			return FALSE;

		if (!event_key[0])
			sql_reward_event_key(event_key, sizeof(event_key), "quest_complete", pid, quest_target);

		mysql_real_escape_string(db, event_key_sql, event_key, strlen(event_key));
		mysql_real_escape_string(db, player_name_sql, player_name, strlen(player_name));
		mysql_real_escape_string(db, reward_desc_sql, reward_desc, strlen(reward_desc));
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO world_quest_accomplished "
		         "(event_key, pid, timestamp, quest_giver, player_name, player_level, quest_target, reward_vnum, reward_desc) "
		         "VALUES ('%s', %d, now(), %d, '%s', %d, %d, %d, '%s') "
		         "ON DUPLICATE KEY UPDATE event_key=event_key",
		         event_key_sql,
		         pid,
		         quest_giver,
		         player_name_sql,
		         player_level,
		         quest_target,
		         reward_vnum,
		         reward_desc_sql);
	}
	else if (!str_cmp(event_type, "zone_touch"))
	{
		if (!sql_persistence_ensure_reward_tables(db))
			return FALSE;

		if (!event_key[0])
			sql_reward_event_key(event_key, sizeof(event_key), "zone_touch", toucher_pid, zone_number);

		mysql_real_escape_string(db, event_key_sql, event_key, strlen(event_key));
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO zone_touches "
		         "(event_key, boot_time, touched_at, zone_number, toucher_pid, group_size, epic_value, alignment_delta) "
		         "VALUES ('%s', FROM_UNIXTIME(%d), FROM_UNIXTIME(%d), %d, %d, %d, %d, %d) "
		         "ON DUPLICATE KEY UPDATE event_key=event_key",
		         event_key_sql,
		         boot_time,
		         touched_at,
		         zone_number,
		         toucher_pid,
		         group_size,
		         epic_value,
		         alignment_delta);
	}
	else if (!str_cmp(event_type, "shop_trophy_sell"))
	{
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO shop_trophy (item, value, seller, timestamp) "
		         "VALUES ('%d', '%d', %d, now())",
		         item_vnum,
		         sale_value,
		         seller_pid);
	}
	else if (!str_cmp(event_type, "quest_trophy_finish"))
	{
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO quest_trophy (mob_vnum, pid, type, reward_value, timestamp) "
		         "VALUES ('%d', '%d', %d, %d, now())",
		         mob_vnum,
		         pid,
		         reward_type,
		         reward_value);
	}
	else if (!str_cmp(event_type, "auction_bid_history"))
	{
		mysql_real_escape_string(db, bidder_name_sql, bidder_name, strlen(bidder_name));
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO auction_bid_history (date, auction_id, bidder_pid, bidder_name, bid_amount) "
		         "VALUES (unix_timestamp(), %d, %d, '%s', %d)",
		         auction_id,
		         pid,
		         bidder_name_sql,
		         bid_amount);
	}
	else if (!str_cmp(event_type, "player_frags"))
	{
		snprintf(query, sizeof(query), "INSERT INTO progress VALUES(0, %d, %d, NOW(), %d)", pid, PROGRESS_FRAGS, delta);
		if (!sql_persistence_query(db, query))
			return FALSE;

		if (pid > 0)
		{
			snprintf(query, sizeof(query), "UPDATE frag_leaderboard SET total_frags = %d, last_updated = NOW() WHERE pid = %d AND deleted_at IS NULL", total_frags, pid);
			if (!sql_persistence_query(db, query))
				return FALSE;
		}
	}
	else if (!str_cmp(event_type, "zone_table_update"))
	{
		snprintf(query, sizeof(query), "UPDATE zones SET last_touch = NOW() WHERE number = '%d'", zone_number);
		if (!sql_persistence_query(db, query))
			return FALSE;

		if (alignment_delta)
		{
			snprintf(query, sizeof(query), "UPDATE zones SET reset_perc = 1 WHERE number = '%d'", zone_number);
			if (!sql_persistence_query(db, query))
				return FALSE;
		}
		return TRUE;
	}
	else if (!str_cmp(event_type, "player_killed_by"))
	{
		mysql_real_escape_string(db, player_name_sql, player_name, strlen(player_name));
		snprintf(query, sizeof(query), "UPDATE player_data SET killed_by = '%s' WHERE pid = %d", player_name_sql, pid);
		if (!sql_persistence_query(db, query))
			return FALSE;
		return TRUE;
	}
	else if (!str_cmp(event_type, "pkill_info"))
	{
		/* SQL escape buffers: 2*280+1 = 561 (worst-case mysql_real_escape_string expansion).
		 * Fixes pre-existing overflow: old pd_sql[256] was too small even for 200-char inputs. */
		char pd_sql[561], eq_sql[561], lg_sql[561], pk_sql[64];
		mysql_real_escape_string(db, pk_sql, pk_type_sql, strlen(pk_type_sql));
		mysql_real_escape_string(db, pd_sql, player_description, strlen(player_description));
		mysql_real_escape_string(db, eq_sql, equip, strlen(equip));
		mysql_real_escape_string(db, lg_sql, log, strlen(log));
		snprintf(query,
		         sizeof(query),
		         "INSERT INTO pkill_info (event_id, pid, level, pk_type, player_description, equip, log, inroom, leader) "
		         "VALUES(%d, %d, %d, '%s', '%s', '%s', '%s', %d, %d)",
		         type_id,
		         pid,
		         player_level,
		         pk_sql,
		         pd_sql,
		         eq_sql,
		         lg_sql,
		         type,
		         auction_id);
		if (!sql_persistence_query(db, query))
			return FALSE;
		return TRUE;
	}
	else
	{
		return FALSE;
	}

	return sql_persistence_query(db, query);
}

bool sql_persistence_write_scalar_event_line(const char *line)
{
	bool ok;

	pthread_mutex_lock(&persistence_sql_mutex);
	ok = sql_persistence_write_scalar_event_line_locked(line);
	pthread_mutex_unlock(&persistence_sql_mutex);

	return ok;
}

void send_to_pid_offline(const char *msg, int pid)
{
	char buff[MAX_STRING_LENGTH];
	mysql_real_escape_string(DB, buff, msg, strlen(msg));
	qry("INSERT INTO offline_messages (date, pid, message) VALUES (now(), '%d', '%s')", pid, buff);
}

void send_offline_messages(P_char ch)
{
	if (!ch)
		return;

	if (!qry("SELECT id, message FROM offline_messages WHERE pid = '%d' ORDER BY date ASC", GET_PID(ch)))
	{
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		return;
	}

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		send_to_char(row[1], ch);
		qry("DELETE FROM offline_messages WHERE id = '%d'", atoi(row[0]));
	}

	mysql_free_result(res);
}

int sql_shop_sell(P_char ch, P_obj obj, int value)
{
	char line[PERSISTENCE_EVENT_MAX_LEN];
	int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;

	int pid = (IS_PC(ch) ? GET_PID(ch) : 0);

	snprintf(line,
	         sizeof(line),
	         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=shop_trophy_sell|item_vnum=%d|sale_value=%d|seller_pid=%d",
	         (long)time(NULL),
	         m_virtual,
	         value,
	         pid);

	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return 1;
		if (persistence_write_fallback_event_line(line, "scalar_event", "shop_trophy", "queue_full_flat_fallback"))
			return 1;
	}

	qry("INSERT INTO shop_trophy (item, value, seller, timestamp) VALUES ('%d', '%d', %d, now())", m_virtual, value, pid);

	return 1;
}

int sql_shop_trophy(P_obj obj)
{

	if (!obj)
		return 0;

	// mined ore doesnt devaule
	if (strstr(obj->name, "_ore_"))
		return 0;

	int objvir = OBJ_VNUM(obj);
	if ((objvir >= 400000) && (objvir < 400202))
		return 0;

	int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;

	MYSQL_RES *db = db_query("SELECT count(id) FROM shop_trophy where item = %d and  TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 7", m_virtual);

	int returning_value = 0;
	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);
		if (NULL != row)
		{
			returning_value = atoi(row[0]);
		}
		else
			returning_value = 0;
		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}
	return returning_value;
}

///

int sql_quest_finish(P_char ch, P_char giver, int type, int value)
{

	char line[PERSISTENCE_EVENT_MAX_LEN];
	int m_virtual = GET_VNUM(giver);
	// GET_PID(ch), ch->only.pc->quest_giver, GET_NAME(ch), GET_LEVEL(ch), ch->only.pc->quest_mob_vnum, m_virtual ,reward->short_description );

	snprintf(line,
	         sizeof(line),
	         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=quest_trophy_finish|pid=%d|mob_vnum=%d|reward_type=%d|reward_value=%d",
	         (long)time(NULL),
	         GET_PID(ch),
	         m_virtual,
	         type,
	         value);

	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return 1;
		if (persistence_write_fallback_event_line(line, "scalar_event", "quest_trophy", "queue_full_flat_fallback"))
			return 1;
	}

	qry("INSERT INTO quest_trophy (mob_vnum, pid, type, reward_value, timestamp) VALUES ('%d', '%d', %d, %d ,now())", m_virtual, GET_PID(ch), type, value);
	return 1;
}

int sql_quest_trophy(P_char giver)
{
	int m_virtual = GET_VNUM(giver);

	MYSQL_RES *db              = db_query("SELECT count(id) FROM quest_trophy where mob_vnum = %d and  TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 14", m_virtual);
	int        returning_value = 0;
	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);
		if (NULL != row)
		{
			returning_value = atoi(row[0]);
		}
		else
			returning_value = 0;
		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}
	return returning_value;
}

void log_epic_gain(int pid, int type, int type_id, int epics)
{
	char event_key[128];

	sql_reward_event_key(event_key, sizeof(event_key), "epic_gain", pid, type_id);
	log_epic_gain_event(event_key, pid, type, type_id, epics);
}

void log_epic_gain_event(const char *event_key, int pid, int type, int type_id, int epics)
{
	MYSQL *db;
	char event_key_buf[128];
	char event_key_sql[256];
	char line[PERSISTENCE_EVENT_MAX_LEN];

	if (!event_key || !*event_key)
	{
		sql_reward_event_key(event_key_buf, sizeof(event_key_buf), "epic_gain", pid, type_id);
		event_key = event_key_buf;
	}

	snprintf(line,
	         sizeof(line),
	         "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=epic_gain|event_key=%s|pid=%d|type=%d|type_id=%d|epics=%d",
	         (long)time(NULL),
	         event_key,
	         pid,
	         type,
	         type_id,
	         epics);

	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return;
		if (persistence_write_fallback_event_line(line, "scalar_event", "epic_gain", "queue_full_flat_fallback"))
			return;
	}

	db = persistence_reward_tables_ready ? sql_persistence_connection() : NULL;
	if (!db)
	{
		qry("INSERT INTO epic_gain (pid, time, type, type_id, epics) values ('%d', now(), '%d', '%d', '%d')", pid, type, type_id, epics);
		return;
	}

	mysql_real_escape_string(db, event_key_sql, event_key, strlen(event_key));

	qry("INSERT INTO epic_gain (event_key, pid, time, type, type_id, epics) "
	    "values ('%s', '%d', now(), '%d', '%d', '%d') "
	    "ON DUPLICATE KEY UPDATE event_key=event_key",
	    event_key_sql,
	    pid,
	    type,
	    type_id,
	    epics);
}

/* The prepstatement_duris_sql table looks like:
+-------------+---------+------+-----+---------+----------------+
| Field       | Type    | Null | Key | Default | Extra          |
+-------------+---------+------+-----+---------+----------------+
| id          | int(11) | NO   | PRI | NULL    | auto_increment |
| description | text    | YES  |     | NULL    |                |
| sql_code    | text    | YES  |     | NULL    |                |
+-------------+---------+------+-----+---------+----------------+
*/
void do_sql(P_char ch, char *argument, int cmd)
{

	char  first[MAX_INPUT_LENGTH];
	char  second[MAX_INPUT_LENGTH];
	char  third[MAX_INPUT_LENGTH];
	char  fourth[MAX_INPUT_LENGTH];
	char *rest;
	char  buf[MAX_STRING_LENGTH];
	int   limited_result = 0;
	int   prep_statement;
	int   num_fields, num_rows, i;

	char result[MAX_STRING_LENGTH * 10];
	char tmp[MAX_STRING_LENGTH];

	MYSQL_RES *db = 0;
	MYSQL_ROW  row;

	if (!IS_TRUSTED(ch))
	{
		send_to_char("A mere mortal can't do this!\r\n", ch);
		return;
	}

	if (!*argument)
	{
		send_to_char("Sql is a command to let us gods, access database easy, it suport all kind of queries.\n"
		             "&=LY-=Make sure you understand what you do else this command is most likly not designed for you=-&n\n",
		             ch);
		send_to_char("&+WSyntax: 'sql < query | prep <list | #> >'&n\n", ch);
		return;
	}

	wizlog(56, "SQL (%s): '%s'", GET_TRUE_NAME(ch), argument);
	logit(LOG_WIZ, "SQL (%s): '%s'", GET_TRUE_NAME(ch), argument);
	sql_log(ch, WIZLOG, "SQL: '%s'", argument);

	rest = one_argument(argument, first);
	rest = one_argument(rest, second);

	if (strstr(first, "prep"))
	{
		if (strstr(second, "list"))
		{
			do_sql(ch, "SELECT id, description FROM prepstatement_duris_sql", 0);
		}
		if (!is_number(second))
		{
			//      send_to_char("\n\r&+YTo add prep queries just check how the table 'prepstatement_duris_sql' (&+Wsql desc prepstatement_duris_sql&+Y) and add!&n\n\r", ch);
			send_to_char("&+YSyntax:&n sql prep < list | number > [ desc | sql | run | delete ] [ description | sql code ]\n\r", ch);
			return;
		}
		else
		{
			prep_statement = (int)atoi(second);
			rest           = one_argument(rest, third);
			rest           = skip_spaces(rest);
			if (!*third)
			{
				snprintf(third, MAX_INPUT_LENGTH, "SELECT * FROM prepstatement_duris_sql WHERE id=%d", prep_statement);
				do_sql(ch, third, cmd);
				/* This won't work due to the fact that we're trying a second sql command?
				        if( !qry( third ) )
				        {
				          send_to_char( "Row does not exist: attempting to create..\n\r", ch );
				          snprintf(buf, MAX_STRING_LENGTH, "INSERT INTO prepstatement_duris_sql (id, description) VALUES (%d, 'new')", prep_statement );
				          do_sql( ch, buf, cmd );
				        }
				        else
				        {
				          do_sql( ch, third, cmd );
				        }
				*/
				return;
			}
			if (strstr(third, "run"))
			{
				db = db_query("SELECT sql_code FROM prepstatement_duris_sql WHERE id=%d", prep_statement);
				if (db)
				{
					MYSQL_ROW row = mysql_fetch_row(db);

					if (row != NULL)
					{
						snprintf(tmp, MAX_STRING_LENGTH, "%s", row[0]);
					}
					else
					{
						send_to_char("That prepped statement does not exist.\n\r", ch);
						tmp[0] = '\0';
					}
					while ((row = mysql_fetch_row(db)))
						;
					mysql_free_result(db);

					do_sql(ch, tmp, 0);
					return;
				}
				else
				{
					send_to_char("Error no db created.\n\r", ch);
				}
				return;
			}
			if (strstr(third, "desc"))
			{
				// SECURITY FIX: Escape user input to prevent SQL injection
				char escaped_desc[MAX_STRING_LENGTH * 2 + 1];
				mysql_real_escape_string(DB, escaped_desc, rest, strlen(rest));
				snprintf(buf, MAX_STRING_LENGTH, "UPDATE prepstatement_duris_sql SET description = '%s' WHERE id='%d'", escaped_desc, prep_statement);
				do_sql(ch, buf, 0);
				return;
			}
			if (strstr(third, "sql"))
			{
				// SECURITY FIX: Escape user input to prevent SQL injection
				char escaped_sql[MAX_STRING_LENGTH * 2 + 1];
				mysql_real_escape_string(DB, escaped_sql, rest, strlen(rest));
				if (qry("UPDATE prepstatement_duris_sql SET sql_code = '%s' WHERE id='%d'", escaped_sql, prep_statement))
				{
					snprintf(buf, MAX_STRING_LENGTH, "Row %d sql_code set to '%s'.\n\r", prep_statement, rest);
					send_to_char(buf, ch);
				}
				return;
			}
			if (strstr(third, "delete"))
			{
				if (qry("DELETE FROM prepstatement_duris_sql WHERE id=%d", prep_statement))
				{
					snprintf(buf, MAX_STRING_LENGTH, "Row %d deleted.\n\r", prep_statement);
					send_to_char(buf, ch);
				}
				return;
			}
		}
	}

	MYSQL_FIELD *fields;
	result[0] = '\0';

	if (mysql_real_query(DB, argument, strlen(argument)))
	{
		snprintf(result, MAX_STRING_LENGTH, "%s", mysql_error(DB));
		logit(LOG_DEBUG, "MySQL error(sql command): %s", mysql_error(DB));
		send_to_char(result, ch);
		return;
	}
	db = mysql_use_result(DB);
	if (db)
	{
		num_fields = mysql_num_fields(db);

		fields = mysql_fetch_fields(db);
		for (i = 0; i < num_fields; i++)
		{
			snprintf(tmp, MAX_STRING_LENGTH, " | %-15s&n ", fields[i].name);
			strcat(result, tmp);
		}
		strcat(result, " |\n\n");

		int maxsize = 100;
		while ((row = mysql_fetch_row(db)))
		{
			maxsize--;
			if (maxsize == 0)
			{
				while ((row = mysql_fetch_row(db)))
					;
				limited_result = 1;
				break;
			}

			for (i = 0; i < num_fields; i++)
			{
				snprintf(tmp, MAX_STRING_LENGTH, " | %-15s&n ", row[i]);
				strcat(result, tmp);
			}
			strcat(result, " |\n\n");
		}
		send_to_char(result, ch);
		if (limited_result)
		{
			send_to_char("Result to big, pls use limit. 'select * from blah &+Ylimit 10&n' will show 10 results.\n", ch);
		}
		mysql_free_result(db);
		return;
	}
}

void update_zone_db()
{
	/* update the zones in the database */
	for (int z = 1; z <= top_of_zone_table; z++)
	{
		int number = zone_table[z].number;

		if (!qry("SELECT id FROM zones WHERE number = '%d'", number))
		{
			logit(LOG_DEBUG, "update_zone_db(): qry failed");
			return;
		}

		char name_buff[MAX_STRING_LENGTH];
		mysql_real_escape_string(DB, name_buff, zone_table[z].name, strlen(zone_table[z].name));

		MYSQL_RES *res = mysql_store_result(DB);
		if (mysql_num_rows(res) > 0)
		{
			qry("UPDATE zones SET name = '%s' WHERE number = '%d'", name_buff, number);
		}
		else
		{
			qry("INSERT INTO zones (number, name) VALUES ('%d', '%s')", number, name_buff);
		}
		mysql_free_result(res);
	}

	for (P_obj o = object_list; o; o = o->next)
	{
		int epic_type = 0;

		switch (obj_index[o->R_num].virtual_number)
		{
			case EPIC_SMALL_STONE:
				epic_type = MAX(epic_type, EPIC_ZONE_TYPE_SMALL);
				break;

			case EPIC_LARGE_STONE:
				epic_type = MAX(epic_type, EPIC_ZONE_TYPE_LARGE);
				break;

			case EPIC_MONOLITH:
				epic_type = MAX(epic_type, EPIC_ZONE_TYPE_MONOLITH);
				break;
		}

		if (!epic_type)
			continue;

		int zone_id = obj_zone_id(o);

		if (zone_id >= 0)
		{
			qry("UPDATE zones SET epic_type = '%d' WHERE number = '%d'", epic_type, zone_table[zone_id].number);
		}
	}
}

void update_zone_epic_level(int zone_number, int level) { qry("UPDATE zones SET epic_level = '%d' WHERE number = '%d'", level, zone_number); }

void show_frag_trophy(P_char ch, P_char who)
{

	if (!IS_PC(who))
		return;

	if (!qry("select player_data.name, count(*) as cnt from epic_gain, player_data where epic_gain.type_id = player_data.pid and epic_gain.pid = %d and type = 1 group by type_id order by name asc",
	         who->only.pc->pid))
	{
		logit(LOG_DEBUG, "show_frag_trophy(): query failed.");
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		send_to_char("&+WYou haven't fragged anyone!\r\n", ch);
		return;
	}

	send_to_char("&+gFrag Trophy:\r\n", ch);

	char buff[MAX_STRING_LENGTH];

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		snprintf(buff, MAX_STRING_LENGTH, " &+g(&+G%2d&+g) &+W%s\r\n", atoi(row[1]), row[0]);
		send_to_char(buff, ch);
	}

	mysql_free_result(res);
}

void sql_log(P_char ch, char *kind, char *format, ...)
{
	static char buff[MAX_STRING_LENGTH];
	buff[0] = '\0';

	if (!ch)
	{
		debug("sql_log called for non-existent ch!");
		return;
	}

	if (!IS_PC(ch))
	{
		debug("sql_log called in sql.c for mobile ch - %s - Vnum %d", GET_NAME(ch), GET_VNUM(ch));
		debug("sql_log kind '%s', format '%s'", kind, format);
		return;
	}

	va_list args;
	int     ret;

	va_start(args, format);
	// SECURITY FIX: Replace vsprintf with vsnprintf to prevent buffer overflow
	ret = vsnprintf(buff, sizeof(buff), format, args);
	va_end(args);

	// Check for overflow
	if (ret < 0 || ret >= (int)sizeof(buff))
	{
		debug("sql_log: Message too long or formatting error");
		return;
	}

	static char message_buff[MAX_STRING_LENGTH];
	message_buff[0] = '\0';
	mysql_real_escape_string(DB, message_buff, buff, strlen(buff));

	static char ip_buff[15];
	ip_buff[0] = '\0';

	if (ch->desc && ch->desc->host)
	{
		snprintf(ip_buff, 50, "%s", ch->desc->host);
	}

	snprintf(buff,
	         MAX_STRING_LENGTH,
	         "INSERT INTO log_entries (date, kind, ip_address, pid, player_name, zone_number, room_vnum, message) VALUES "
	         "(now(), '%s', '%s', %d, '%s', %d, %d, '%s')",
	         kind,
	         ip_buff,
	         GET_PID(ch),
	         GET_NAME(ch),
	         zone_table[world[ch->in_room].zone].number,
	         world[ch->in_room].number,
	         message_buff);

	qry(buff);
}

bool get_zone_info(int zone_number, struct zone_info *info)
{
	if (!info)
	{
		return FALSE;
	}

	if (!qry("SELECT number, name, epic_type, frequency_mod, zone_freq_mod, epic_level, task_zone, quest_zone, trophy_zone, suggested_group_size, epic_payout, difficulty FROM zones WHERE number = %d",
	         zone_number))
	{
		return FALSE;
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		return FALSE;
	}

	MYSQL_ROW row = mysql_fetch_row(res);

	info->number               = atoi(row[0]);
	info->name                 = string(row[1]);
	info->epic_type            = atoi(row[2]);
	info->frequency_mod        = atof(row[3]);
	info->zone_freq_mod        = atof(row[4]);
	info->epic_level           = atoi(row[5]);
	info->task_zone            = (bool)atoi(row[6]);
	info->quest_zone           = (bool)atoi(row[7]);
	info->trophy_zone          = (bool)atoi(row[8]);
	info->suggested_group_size = atoi(row[9]);
	info->epic_payout          = atoi(row[10]);
	info->difficulty           = atoi(row[11]);

	mysql_free_result(res);
	return TRUE;
}

string get_mud_info(const char *name)
{
	if (!qry("SELECT content FROM mud_info WHERE name = '%s'", name))
	{
		logit(LOG_DEBUG, "get_mud_info(): failed to read mud_info '%s' from database", name);
		return string();
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (!res)
	{
		logit(LOG_DEBUG, "get_mud_info(): mysql_store_result failed for '%s'", name);
		return string();
	}

	if (mysql_num_rows(res) > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		string    ret_str(row[0]);
		mysql_free_result(res);
		return ret_str;
	}
	else
	{
		logit(LOG_DEBUG, "get_mud_info(): requested mud_info '%s', but doesn't exist!", name);
		mysql_free_result(res);
		return string();
	}
}

void send_mud_info(const char *name, P_char ch) { send_to_char(get_mud_info(name).c_str(), ch, LOG_NONE); }

void sql_get_bind_data(int vnum, int *owner_pid, int *timer)
{
	if (!qry("SELECT owner_pid, timer FROM artifact_bind WHERE vnum = %d", vnum))
	{
		logit(LOG_DEBUG, "sql_get_bind_data(): failed to read from database");
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (mysql_num_rows(res) < 1)
	{
		// logit(LOG_DEBUG, "sql_get_bind_data(): Cannot find artifact entry, using default values.");
		*owner_pid = 0;
		*timer     = 0;
		mysql_free_result(res);
		return;
	}
	else
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		if (row != NULL)
		{
			*owner_pid = atoi(row[0]);
			*timer     = atoi(row[1]);
		}
	}
	mysql_free_result(res);
}

void sql_update_bind_data(int vnum, int *owner_pid, int *timer)
{
	qry("INSERT INTO artifact_bind (vnum, owner_pid, timer) VALUES(%d, %d, %d) "
	    "ON DUPLICATE KEY UPDATE owner_pid=VALUES(owner_pid), timer=VALUES(timer)",
	    vnum,
	    *owner_pid,
	    *timer);
}

bool sql_clear_zone_trophy()
{
	// Update the table zones, set the alignment to 0, where there's an epic stone.
	if (!qry("UPDATE zones SET alignment=0 WHERE epic_type > 0"))
	{
		debug("sql_clear_zone_trophy(): Failed sql UPDATE.. :(");
		return FALSE;
	}

	return TRUE;
}

bool sql_pwipe(int code_verify)
{
	logit(LOG_DEBUG, "sql_pwipe: STARTED!");
	if (code_verify == 1723699)
	{
		logit(LOG_DEBUG, "sql_pwipe: Clearing zone alignments, trophy and touches... .. .");
		send_to_all("Clearing zone alignments, trophy and touches... .. .");
		if (sql_clear_zone_trophy() && qry("DELETE FROM zone_trophy") && qry("DELETE FROM zone_touches"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing tower ownership... .. .");
		send_to_all("Clearing tower ownership... .. .");
		if (qry("UPDATE outposts SET owner_id='0', level='8', walls='1', archers='0', hitpoints='300000', territory='0',"
		        " portal_room='0', resources='0', applied_resources='0', golems='0', meurtriere='0', scouts='0'"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing nexus stone data... .. .");
		send_to_all("Clearing nexus stone data... .. .");
		if (qry("UPDATE nexus_stones SET align='0', last_touched_at=NULL"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing prestige lists... .. .");
		send_to_all("Clearing prestige lists... .. .");
		if (qry("DELETE FROM associations"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing alliances... .. .");
		send_to_all("Clearing alliances... .. .");
		if (qry("DELETE FROM alliances"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing artifact bind data... .. .");
		send_to_all("Clearing artifact bind data... .. .");
		if (qry("DELETE FROM artifact_bind"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing auction data... .. .");
		send_to_all("Clearing auction data... .. .");
		if (qry("DELETE FROM auction_bid_history") && qry("DELETE FROM auction_item_pickups") && qry("DELETE FROM auction_money_pickups") && qry("DELETE FROM auctions"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing boon data... .. .");
		send_to_all("Clearing boon data... .. .");
		if (qry("DELETE FROM boons_progress") && qry("DELETE FROM boons"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing ctf data... .. .");
		send_to_all("Clearing ctf data... .. .");
		if (qry("DELETE FROM ctf_data"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing frag data and epic bonus data... .. .");
		send_to_all("Clearing frag data and epic bonus data... .. .");
		if (qry("DELETE FROM epic_bonus") && qry("DELETE FROM epic_gain") && qry("DELETE FROM progress"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing guild data... .. .");
		send_to_all("Clearing guild data... .. .");
		if (qry("DELETE FROM guild_transactions") && qry("DELETE FROM guildhall_rooms") && qry("DELETE FROM guildhalls"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing ip info... .. .");
		send_to_all("Clearing ip info... .. .");
		if (qry("DELETE FROM ip_info"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing log entries... .. .");
		send_to_all("Clearing log entries... .. .");
		if (qry("DELETE FROM log_entries"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing offline messages... .. .");
		send_to_all("Clearing offline messages... .. .");
		if (qry("DELETE FROM offline_messages"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing cargo data... .. .");
		send_to_all("Clearing cargo data... .. .");
		if (qry("DELETE FROM ship_cargo_market_mods") && qry("DELETE FROM ship_cargo_prices"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing timers... .. .");
		send_to_all("Clearing timers... .. .");
		if (qry("UPDATE timers SET date='0'"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing shop data... .. .");
		send_to_all("Clearing shop data... .. .");
		if (qry("DELETE FROM shop_trophy"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing completed quest data... .. .");
		send_to_all("Clearing completed quest data... .. .");
		if (qry("DELETE FROM world_quest_accomplished"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing locker grant list data... .. .");
		send_to_all("Clearing locker grant list data... .. .");
		if (qry("DELETE FROM locker_access"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Deactivating player_data... .. .");
		send_to_all("Deactivating player_data... .. .");
		if (qry("UPDATE player_data SET active = 0"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Resetting level_cap data... .. .");
		send_to_all("Resetting level_cap data... .. .");
		// needs to get this parameterized
		if (qry("UPDATE level_cap SET most_frags=0, racewar_leader=0, level=31, next_update=NOW() + INTERVAL 7 DAY"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		return TRUE;
	}
	else
	{
		logit(LOG_DEBUG, "sql_pwipe: Someone called sql_pwipe with a bad verify code... hrm..");
		return FALSE;
	}
	logit(LOG_DEBUG, "sql_pwipe: COMPLETED!");
	send_to_all("WIPE COMPLETED!");
	sleep(1);
}

void sql_log_player_login(P_char ch, const char *status)
{
	if (!ch || IS_NPC(ch) || !ch->desc)
		return;

	// copy data before fork since child can't access parent memory safely
	char name[32], ip[64], account[64], client[64], client_ver[32];
	int  pid_num;

	strncpy(name, GET_NAME(ch), sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';
	strncpy(ip, ch->desc->host, sizeof(ip) - 1);
	ip[sizeof(ip) - 1] = '\0';
	const char *acct   = get_account_name_safe(ch);
	strncpy(account, acct ? acct : "", sizeof(account) - 1);
	account[sizeof(account) - 1] = '\0';
	strncpy(client, ch->desc->client_name[0] ? ch->desc->client_name : "", sizeof(client) - 1);
	client[sizeof(client) - 1] = '\0';
	strncpy(client_ver, ch->desc->client_version[0] ? ch->desc->client_version : "", sizeof(client_ver) - 1);
	client_ver[sizeof(client_ver) - 1] = '\0';
	pid_num                            = GET_PID(ch);

	pid_t pid = fork();
	if (pid < 0)
		return; // fork failed, skip logging

	if (pid == 0)
	{
		// child process
		MYSQL *child_conn = sql_create_child_connection();
		if (!child_conn)
			_exit(1);

		sql_reset_for_child(child_conn);

		db_query("INSERT INTO log_entries (date, kind, ip_address, pid, player_name, zone_number, room_vnum, message) "
		         "VALUES (NOW(), '%s', '%s', %d, '%s', 0, 0, 'account=%s client=%s %s')",
		         status,
		         ip,
		         pid_num,
		         name,
		         account,
		         client,
		         client_ver);

		mysql_close(child_conn);
		_exit(0);
	}
	// parent continues immediately
}
#endif
