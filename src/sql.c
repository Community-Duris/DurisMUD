
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
#include "sql_pool.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include "account.h"
#include "account_reward.h"
#include "assocs.h"
#include "boon.h"
#include "epic.h"
#include "graph.h"
#include "mm.h"
#include "objmisc.h"
#include "redis.h"
#include "specializations.h"
#include "spells.h"
#include "sql_player.h"
#include "frag_cap_config.h"
#include "timers.h"
#include "persistence_queue.h"
#include "utility.h"

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

static int  sql_trace_burst = 0;
static char sql_trace_last_site[64] = "";
static char sql_trace_last_sql[240] = "";
static bool sql_trace_enabled(void);
static bool sql_trace_active(void);
static const char *sql_trace_kind(const char *sql);
static void sql_trace_preview(const char *sql, char *out, size_t outsz);
static void sql_trace_log(const char *phase, MYSQL *conn, const char *sql);
static void sql_trace_log_drain(MYSQL *conn, const char *phase, bool drained);

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
void sql_world_quest_finished(P_char ch, P_char giver, P_obj obj) {}
int  sql_world_quest_done_already(P_char ch, int quest_target) { return -1; }
int  sql_world_quest_can_do_another(P_char ch) { return -1; }

void        sql_connectIP(P_char ch) {}
void        sql_disconnectIP(P_char ch) {}
const char *sql_select_IP_info(P_char ch, char *buf, size_t bufSize, time_t *lastConnect, time_t *lastDisconnect)
{
	buf[0] = 0;
	return buf;
}
int  sql_find_racewar_for_ip(char *ip, int *racewar_side) { return -1; }
bool qry(const char *format, ...) { return TRUE; }
void send_to_char_offline(const char *msg, int pid) {}
void send_offline_messages(P_char ch) {}
void log_epic_gain(int pid, int zone_id, int type, int epics) {}
void log_epic_gain_event(const char *event_key, int pid, int type, int type_id, int epics) {}
bool sql_persistence_item_owner_matches(unsigned long long item_uid, const char *owner_type, const char *owner_ref, const char *context) { return true; }
void update_zone_db() {}
void update_zone_epic_level(int zone_id, int level) {}
void show_frag_trophy(P_char ch, P_char who) { send_to_char("Disabled.", ch); }
void sql_log(P_char ch, char *kind, char *format, ...)
{
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
	int     raw_len;
	char   *raw_msg;
	char   *escaped_msg;
	string  esc_kind;
	string  esc_ip;
	string  esc_name;
	string  query;

	va_start(args, format);
	raw_len = vsnprintf(NULL, 0, format, args);
	va_end(args);
	if (raw_len < 0)
	{
		debug("sql_log: Message formatting error");
		return;
	}

	raw_msg = (char *)malloc((size_t)raw_len + 1);
	if (!raw_msg)
		return;

	va_start(args, format);
	vsnprintf(raw_msg, (size_t)raw_len + 1, format, args);
	va_end(args);

	escaped_msg = (char *)malloc(((size_t)raw_len * 2) + 1);
	if (!escaped_msg)
	{
		free(raw_msg);
		return;
	}
	mysql_real_escape_string(DB, escaped_msg, raw_msg, (unsigned long)raw_len);

	auto escape_sql = [](const char *src) -> string {
		if (!src)
			return string();
		size_t len = strlen(src);
		string out;
		out.resize((len * 2) + 1);
		unsigned long out_len = mysql_real_escape_string(DB, &out[0], src, (unsigned long)len);
		out.resize(out_len);
		return out;
	};

	esc_kind = escape_sql(kind);
	if (ch->desc && ch->desc->host)
		esc_ip = escape_sql(ch->desc->host);
	esc_name = escape_sql(GET_NAME(ch));

	query = "INSERT INTO log_entries (date, kind, ip_address, pid, player_name, zone_number, room_vnum, message) VALUES (NOW(), '";
	query += esc_kind;
	query += "', '";
	query += esc_ip;
	query += "', ";
	query += std::to_string(GET_PID(ch));
	query += ", '";
	query += esc_name;
	query += "', ";
	query += std::to_string(zone_table[world[ch->in_room].zone].number);
	query += ", ";
	query += std::to_string(world[ch->in_room].number);
	query += ", '";
	query += escaped_msg;
	query += "')";

	qry("%s", query.c_str());
	free(raw_msg);
	free(escaped_msg);
}

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
static bool sql_verify_boot_database(void);

// The global database handler
MYSQL *DB;

/* persistenceDB replaced by connection pool (sql_pool.c).
 * persistence_sql_mutex kept for backward compatibility — no longer
 * needed for connection serialisation but still referenced by
 * sql_persistence_raw.c for now. */
MYSQL *persistenceDB = NULL;
pthread_mutex_t persistence_sql_mutex = PTHREAD_MUTEX_INITIALIZER;

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

/* Resolve the requested database while retaining the non-default-port
 * production safety guard.  Explicit disposable/test database names must
 * remain usable on any port; only an implicit production target is redirected
 * to the development sandbox. */
const char *sql_persistence_db_name(void)
{
	const bool production_name = !strcmp(DB_NAME, "duris") ||
	                             !strcmp(DB_NAME, "duris_prod");

	if (RUNNING_PORT != DFLT_PORT && production_name)
		return "duris_dev";
	return DB_NAME;
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
			setenv(line, eq + 1, 0);
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
	/* Resolve the target database through the shared helper so the
	 * main DB connection, the connection pool slots, and the legacy
	 * persistenceDB fallback all connect to the same database. */
	const char *db_name = sql_persistence_db_name();

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

	if (!sql_verify_boot_database())
	{
		logit(LOG_STATUS, "FATAL: required database connection/schema check failed, aborting boot");
		if (DB)
		{
			mysql_close(DB);
			DB = NULL;
		}
		return -1;
	}

	/* Initialise the connection pool for async persistence
	 * workers (item, scalar, large-payload event queues). */
	if (sql_pool_init(SQL_POOL_DEFAULT_SIZE) != 0)
	{
		logit(LOG_STATUS, "Warning: connection pool init failed — persistence workers will use sync fallback.");
		/* Non-fatal: the main DB connection still works. */
	}

	return 1;
}

/* Handle a query, log possible errors and return results (if available) */
MYSQL_RES *db_query(const char *format, ...)
{
	va_list args;
	int     needed;
	char   *buf;
	MYSQL_RES *res;

	va_start(args, format);
	needed = vsnprintf(NULL, 0, format, args);
	va_end(args);
	if (needed < 0)
	{
		logit(LOG_DEBUG, "MySQL: Query formatting error");
		return NULL;
	}

	buf = (char *)malloc((size_t)needed + 1);
	if (!buf)
		return NULL;

	va_start(args, format);
	vsnprintf(buf, (size_t)needed + 1, format, args);
	va_end(args);

	if (!buf[0])
	{
		free(buf);
		return NULL;
	}

	if (!sql_trace_exec("db_query", buf, strlen(buf), true, false))
	{
		free(buf);
		return NULL;
	}

	res = mysql_store_result(DB);
	if (res)
		sql_trace_log("db_query/rows", DB, buf);
	free(buf);
	return res;
}

static bool sql_verify_boot_database(void)
{
	if (!DB)
	{
		logit(LOG_STATUS, "FATAL: database connection is not initialized at boot.");
		return false;
	}

	const char *probe = "SELECT 1 FROM accounts LIMIT 1";
	if (!sql_trace_exec("boot/accounts_probe", probe, strlen(probe), true, false))
	{
		logit(LOG_STATUS, "FATAL: required accounts table is missing or unreadable at boot: %s", mysql_error(DB));
		return false;
	}

	MYSQL_RES *result = mysql_store_result(DB);
	if (result)
		mysql_free_result(result);

	/* The asynchronous persistence workers require these tables and their
	 * idempotency/index contract.  Fail at boot rather than allowing a worker
	 * to silently divert every event to an unverified fallback path. */
	const char *event_schema_probe =
		"SELECT COUNT(*) FROM information_schema.columns "
		"WHERE table_schema=DATABASE() AND "
		"((table_name='persistence_item_events' AND column_name IN "
		"('id','ts_usec','event_type','item_uid','vnum','item','actor','actor_id','source','target','note','dedupe_key','created_at')) "
		"OR (table_name='persistence_scalar_events' AND column_name IN "
		"('id','event_type','event_key','boot_time','touched_at','zone_number','toucher_pid','group_size','epic_value','alignment_delta','dedupe_key','created_at')))";
	result = db_query("%s", event_schema_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: persistence event schema metadata query failed at boot: %s", mysql_error(DB));
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(result);
	unsigned long *lengths = row ? mysql_fetch_lengths(result) : NULL;
	bool event_columns_ok = row && lengths && row[0] && atoi(row[0]) == 25;
	mysql_free_result(result);
	if (!event_columns_ok)
	{
		logit(LOG_STATUS, "FATAL: persistence event schema is incomplete at boot (expected 25 required columns).");
		return false;
	}

	const char *event_index_probe =
		"SELECT COUNT(*) FROM (SELECT DISTINCT table_name, index_name "
		"FROM information_schema.statistics WHERE table_schema=DATABASE() AND "
		"((table_name='persistence_item_events' AND index_name IN "
		"('PRIMARY','idx_item_uid_ts','idx_event_type_created','uq_item_dedupe')) "
		"OR (table_name='persistence_scalar_events' AND index_name IN "
		"('PRIMARY','idx_scalar_event_key','idx_scalar_zone_time','uq_scalar_dedupe')))) "
		"AS required_indexes";
	result = db_query("%s", event_index_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: persistence event index metadata query failed at boot: %s", mysql_error(DB));
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	bool event_indexes_ok = row && lengths && row[0] && atoi(row[0]) == 8;
	mysql_free_result(result);
	if (!event_indexes_ok)
	{
		logit(LOG_STATUS, "FATAL: persistence event schema indexes are incomplete at boot (expected 8 entries).");
		return false;
	}

	/* Auction settlement uses explicit transactions and therefore requires
	 * transactional storage.  A legacy MyISAM table can make rollback appear
	 * to succeed while leaving pickup/refund state partially committed. */
	const char *auction_engine_probe =
		"SELECT COUNT(DISTINCT table_name) FROM information_schema.tables "
		"WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN "
		"('auction_bid_history','auction_item_pickups','auction_money_pickups','auctions')";
	result = db_query("%s", auction_engine_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: auction storage-engine metadata query failed at boot: %s", mysql_error(DB));
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	bool auction_engines_ok = row && lengths && row[0] && atoi(row[0]) == 4;
	mysql_free_result(result);
	if (!auction_engines_ok)
	{
		logit(LOG_STATUS, "FATAL: transactional auction tables are not all InnoDB at boot (expected 4).");
		return false;
	}
	return true;
}

/* Same as above, but won't log failed queries, ie when key restrictions suffice */
MYSQL_RES *db_query_nolog(const char *format, ...)
{
	va_list args;
	int     needed;
	char   *buf;

	va_start(args, format);
	needed = vsnprintf(NULL, 0, format, args);
	va_end(args);
	if (needed < 0)
		return NULL;

	buf = (char *)malloc((size_t)needed + 1);
	if (!buf)
		return NULL;

	va_start(args, format);
	vsnprintf(buf, (size_t)needed + 1, format, args);
	va_end(args);

	if (!sql_trace_exec("db_query_nolog", buf, strlen(buf), true, false))
	{
		free(buf);
		return NULL;
	}

	free(buf);
	return mysql_store_result(DB);
}

/* Store core player data to the database. We assume that only association
 * names may contain special characters */
int sql_save_player_core(P_char ch)
{
	char                     query[MAX_STRING_LENGTH];
	char                     assoc_name[MAX_STRING_LENGTH];
	char                     assoc_name_sql[MAX_STRING_LENGTH * 2 + 1];
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

	// deactivate any other players with same name (handles renamed characters)
	snprintf(query, MAX_STRING_LENGTH, "UPDATE player_data SET active = 0 WHERE name = '%s' and pid != %d", p->name, GET_PID(ch));
	db_query(query);

	// mark this player as active
	db_query("UPDATE player_data SET active = 1 WHERE pid = %d", GET_PID(ch));

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
		*level       = frag_cap_config_get()->cap_floor_level;
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
		return frag_cap_config_get()->cap_floor_level;
	}

	level_cap       = atoi(row[0]);
	leading_racewar = atoi(row[1]);

	// cycle out until a NULL return
	while (row != NULL)
	{
		row = mysql_fetch_row(db);
	}
	mysql_free_result(db);

	const struct frag_cap_config *config = frag_cap_config_get();

	// Clamp database values to the configured mortal-cap range.
	if (level_cap >= config->cap_maximum_level)
		return config->cap_maximum_level;
	if (level_cap <= config->cap_floor_level)
		return config->cap_floor_level;

	return level_cap;
}

// Checks the number of frags against the current highest and sets the new highest if applicable.
// Timer policy is selected from the configured old-level bands in frag_cap.cfg.
void sql_check_level_cap(long max_frags, int racewar)
{
	long   old_max_frags;
	int    old_racewar, old_level;
	const struct frag_cap_config *config = frag_cap_config_get();
	time_t next_update;
	char   query[1024];

	get_level_cap_info(&old_max_frags, &old_racewar, &old_level, &next_update);
	// If we've capped out
	if (old_level >= config->cap_maximum_level)
	{
		return;
	}
	// If enough time has passed, and level should change, update level if appropriate.
	if (next_update <= time(NULL))
	{
		// Have enough frags to update level.
		if (old_level < frag_cap_config_cap_level_from_frags(max_frags / 100.))
		{
			// when level cap increases, give a boon to the side that caused it
			BoonData bdata;
			bdata.duration  = frag_cap_config_boon_duration_minutes(); // configurable minutes
			bdata.racewar   = racewar;
			bdata.type      = BTYPE_EXPM;
			bdata.option    = BOPT_MOB;
			bdata.criteria  = 1;
			bdata.criteria2 = -1;
			bdata.bonus     = frag_cap_config_boon_bonus();
			bdata.active    = 1;
			bdata.repeat    = 1;
			create_boon(&bdata);

			int next_level = old_level + config->cap_level_step;
			if (next_level > config->cap_maximum_level)
				next_level = config->cap_maximum_level;
			snprintf(query,
			         sizeof(query),
			         "UPDATE level_cap SET most_frags = %f, racewar_leader = %d, level = %d, next_update = FROM_UNIXTIME(%ld)",
			         max_frags / 100.,
			         racewar,
			         next_level,
			         (long)(time(NULL) + SECS_PER_REAL_DAY * frag_cap_config_timer_days(old_level)));
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

// Re-check the current racewar total even when no new frag was recorded.
// This allows a qualified cap increase and its boon to become available as
// soon as the configured timer expires.
void sql_check_level_cap_periodic(void)
{
	long max_frags;
	int  old_racewar, old_level;
	time_t next_update;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	get_level_cap_info(&max_frags, &old_racewar, &old_level, &next_update);
	if (old_racewar == RACEWAR_NONE || old_level < 0)
		return;

	res = db_query("SELECT COALESCE(SUM(total_frags), 0) FROM frag_leaderboard WHERE racewar=%d", old_racewar);
	if (!res)
		return;

	row = mysql_fetch_row(res);
	if (row && row[0])
	{
		max_frags = atol(row[0]);
		sql_check_level_cap(max_frags, old_racewar);
	}
	mysql_free_result(res);
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
	// We don't want IS_TRUSTED(ch) because that can be turned off with toggle fog.
	if (GET_LEVEL(ch) > MAXLVLMORTAL)
	{
		return;
	}
	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);
	sql_save_progress(GET_PID(ch), gain, PROGRESS_FRAGS);
	// Update frag leaderboard with new frag count (incremental update for performance)
	// Only update if the character is in the database (pid > 0)
	if (GET_PID(ch) > 0)
	{
		db_query("UPDATE frag_leaderboard SET total_frags = %d, last_updated = NOW() WHERE pid = %ld AND deleted_at IS NULL", ch->only.pc->frags, GET_PID(ch));
	}

	if (gain >= 0)
	{
		MYSQL_RES *res = db_query("SELECT COALESCE(SUM(total_frags), 0) FROM frag_leaderboard WHERE racewar=%d", GET_RACEWAR(ch));
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
	char        account_name_sql[MAX_STRING_LENGTH * 2 + 1];
	char        char_name_sql[MAX_STRING_LENGTH * 2 + 1];
	const char *account_name;

	if (!ch || IS_NPC(ch))
		return;

	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);

	if (GET_PID(ch) <= 0)
	{
		logit(LOG_DEBUG, "sql_update_account_character: invalid pid for %s", GET_NAME(ch) ? GET_NAME(ch) : "<null>");
		return;
	}

	account_name = get_account_name_safe(ch);

	// Escape strings for SQL safety
	mysql_str(account_name, account_name_sql);
	mysql_str(ch->player.name, char_name_sql);

	// Insert or update account_characters mapping
	// Using INSERT...ON DUPLICATE KEY UPDATE to preserve created_at for existing records
	if (!qry("INSERT INTO account_characters "
	         "(account_name, pid, char_name, created_at, deleted_at) "
	         "VALUES('%s', %ld, '%s', NOW(), NULL) "
	         "ON DUPLICATE KEY UPDATE "
	         "account_name = VALUES(account_name), "
	         "pid = VALUES(pid), "
	         "char_name = VALUES(char_name), "
	         "deleted_at = NULL",
	         account_name_sql,
	         GET_PID(ch),
	         char_name_sql))
	{
		logit(LOG_DEBUG, "sql_update_account_character: failed for %s", GET_NAME(ch) ? GET_NAME(ch) : "<null>");
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
	char        account_name_sql[MAX_STRING_LENGTH * 2 + 1];
	char        char_name_sql[MAX_STRING_LENGTH * 2 + 1];
	char        race_sql[MAX_STRING_LENGTH * 2 + 1];
	char        class_sql[MAX_STRING_LENGTH * 2 + 1];
	const char *account_name;
	const char *race_name;
	const char *class_name;

	if (!ch || IS_NPC(ch))
		return;

	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);

	if (GET_PID(ch) <= 0)
	{
		logit(LOG_DEBUG, "sql_update_frag_leaderboard: invalid pid for %s", GET_NAME(ch) ? GET_NAME(ch) : "<null>");
		return;
	}

	account_name = get_account_name_safe(ch);
	race_name    = race_names_table[ch->player.race].normal;
	class_name   = class_names_table[flag2idx(ch->player.m_class)].normal;

	// Escape strings for SQL safety
	mysql_str(account_name, account_name_sql);
	mysql_str(ch->player.name, char_name_sql);
	mysql_str(race_name, race_sql);
	mysql_str(class_name, class_sql);

	// Insert or update frag_leaderboard
	// Using INSERT ... ON DUPLICATE KEY UPDATE to preserve the row id while
	// refreshing the current stats.
	if (!qry("INSERT INTO frag_leaderboard "
	         "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
	         "VALUES(%ld, '%s', '%s', %d, %d, '%s', '%s', %d, NULL) "
	         "ON DUPLICATE KEY UPDATE "
	         "account_name=VALUES(account_name), "
	         "char_name=VALUES(char_name), "
	         "total_frags=VALUES(total_frags), "
	         "racewar=VALUES(racewar), "
	         "race=VALUES(race), "
	         "class=VALUES(class), "
	         "level=VALUES(level), "
	         "deleted_at=NULL",
	         GET_PID(ch),
	         account_name_sql,
	         char_name_sql,
	         ch->only.pc->frags,
	         GET_RACEWAR(ch),
	         race_sql,
	         class_sql,
	         GET_LEVEL(ch)))
	{
		logit(LOG_DEBUG, "sql_update_frag_leaderboard: failed for %s", GET_NAME(ch) ? GET_NAME(ch) : "<null>");
	}

}

/* Soft delete a character from the leaderboard tables */
bool sql_soft_delete_character(long pid)
{
	if (!DB || pid <= 0)
		return false;

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	// Set deleted_at timestamp to NOW() for this character
	if (!db_query("UPDATE account_characters SET deleted_at = NOW() WHERE pid = %ld AND deleted_at IS NULL", pid))
	{
		if (own_txn)
			sql_rollback();
		return false;
	}

	if (!db_query("UPDATE frag_leaderboard SET deleted_at = NOW() WHERE pid = %ld AND deleted_at IS NULL", pid))
	{
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

/* Save frags delta */
void sql_insert_item(P_char ch, P_obj obj, char *desc)
{

	char query[MAX_STRING_LENGTH];
	char sql_desc[MAX_STRING_LENGTH * 2 + 1];
	char sql_short[MAX_STRING_LENGTH * 2 + 1];

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
	char  room_name_sql[MAX_STRING_LENGTH * 2 + 1];
	string query;

	mysql_str(world[ch->in_room].name, room_name_sql);
	query = "INSERT INTO pkill_event (stamp, room_vnum, room_name) VALUES( NOW(), ";
	query += std::to_string(world[ch->in_room].number);
	query += ", '";
	query += room_name_sql;
	query += "' )";

	sql_clear_results_on(DB);
	if (!sql_trace_exec("new_pkill_event", query.c_str(), query.size(), false, false))
	{
		logit(LOG_DEBUG, "MYSQL: Failed to create pkill event");
		logit(LOG_DEBUG, "MYSQL: Query was: %s", query.c_str());
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
	char equip_sql[MAX_STRING_LENGTH * 2 + 1];
	char player_description_sql[MAX_STRING_LENGTH * 2 + 1];
	char log_sql[MAX_LOG_LEN * 2 + 1];

	if (!ch || !IS_PC(ch))
		return;

	if (!GET_PLAYER_LOG(ch))
	{
		logit(LOG_DEBUG, "Tried to dump player log (%s) in store_pkill_info(), but player log was null!", GET_NAME(ch));
		return;
	}

	get_equipment_list(ch, buf, 1);
	mysql_str(buf, equip_sql);

	get_pkill_player_description(ch, buf);
	mysql_str(buf, player_description_sql);

	mysql_str(GET_PLAYER_LOG(ch)->read(LOG_PUBLIC, MAX_LOG_LEN), log_sql);

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
	if (!ch || !IS_PC(ch))
		return;
	// level already saved in player_data
}

/* Update money info */
void sql_update_money(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;
	// money stored as copper/silver/gold/platinum in player_data
}

/* Update playtime info */
void sql_update_playtime(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;
	// playtime is played_time in player_data
}

/* Update player's epics: We want to record their total epics gained not epics unused */
void sql_update_epics(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;
	// epics already in player_data
}

void manual_log(P_char ch)
{

	char a[256], b[256];
	char buf[MAX_STRING_LENGTH];
	char equip_sql[MAX_STRING_LENGTH * 2 + 1];
	char log_sql[MAX_LOG_LEN * 2 + 1];
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

	snprintf(a, 256, "%d%d", number(0, 32767), number(0, 2147483647));
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

	db_query_nolog("INSERT IGNORE INTO ip_info (pid) VALUES (%d)", GET_PID(ch));
	if (ch->desc)
	{
		// Set racewar side if not an immortal.
		db_query("UPDATE ip_info SET last_disconnect = NOW(), racewar_side=%d WHERE pid = %d", IS_TRUSTED(ch) ? RACEWAR_NONE : GET_RACEWAR(ch), GET_PID(ch));
	}
}

void sql_connectIP(P_char ch)
{
	// insert will silently fail if the PID is already in the table
	db_query_nolog("INSERT IGNORE INTO ip_info (pid) VALUES (%d)", GET_PID(ch));
	if (ch->desc)
	{
		db_query("UPDATE ip_info SET last_ip = '%s', last_connect = NOW(), racewar_side = %d WHERE pid = %d", ch->desc->host, IS_TRUSTED(ch) ? RACEWAR_NONE : GET_RACEWAR(ch), GET_PID(ch));
	}
}

void sql_world_quest_finished(P_char ch, P_obj reward)
{
	char buf[MAX_STRING_LENGTH * 2 + 1];

	int   reward_vnum = reward ? ((reward->R_num >= 0) ? obj_index[reward->R_num].virtual_number : 0) : 0;
	char *reward_desc = reward ? mysql_str(reward->short_description, buf) : mysql_str("", buf);

	db_query("INSERT INTO world_quest_accomplished (pid, timestamp, quest_giver, player_name, player_level, quest_target, reward_vnum, reward_desc) VALUES (%d, now(), %d, '%s', %d, %d, %d, '%s')",
	         GET_PID(ch),
	         ch->only.pc->quest_giver,
	         GET_NAME(ch),
	         GET_LEVEL(ch),
	         ch->only.pc->quest_mob_vnum,
	         reward_vnum,
	         reward_desc);

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

static bool sql_trace_enabled(void)
{
	static int cached = -1;
	if (cached < 0)
	{
		const char *env = getenv("SQL_TRACE");
		bool        on  = false;

		if (env && *env && strcmp(env, "0") != 0 && strcasecmp(env, "false") != 0 && strcasecmp(env, "off") != 0)
		{
			on = true;
		}
		else
		{
			on = true;
		}
		cached = on ? 1 : 0;
	}
	return cached != 0;
}

static bool sql_trace_active(void)
{
	return sql_trace_enabled() || sql_trace_burst > 0;
}

static const char *sql_trace_kind(const char *sql)
{
	while (sql && *sql && isspace((unsigned char)*sql))
		++sql;

	if (!sql || !*sql)
		return "EMPTY";
	if (!strncasecmp(sql, "SELECT", 6))
		return "SELECT";
	if (!strncasecmp(sql, "UPDATE", 6))
		return "UPDATE";
	if (!strncasecmp(sql, "INSERT", 6))
		return "INSERT";
	if (!strncasecmp(sql, "DELETE", 6))
		return "DELETE";
	if (!strncasecmp(sql, "REPLACE", 7))
		return "REPLACE";
	if (!strncasecmp(sql, "START TRANSACTION", 17))
		return "TXN_BEGIN";
	if (!strncasecmp(sql, "COMMIT", 6))
		return "COMMIT";
	if (!strncasecmp(sql, "ROLLBACK", 8))
		return "ROLLBACK";
	return "OTHER";
}

static void sql_trace_preview(const char *sql, char *out, size_t outsz)
{
	size_t j = 0;

	if (!out || outsz == 0)
		return;
	out[0] = '\0';
	if (!sql)
		return;

	for (size_t i = 0; sql[i] && j + 1 < outsz; ++i)
	{
		unsigned char c = (unsigned char)sql[i];
		if (c == '\n' || c == '\r' || c == '	')
			c = ' ';
		out[j++] = (char)c;
		if (j + 4 >= outsz)
			break;
	}
	out[j] = '\0';
	if (sql[j] != '\0' && outsz > 4)
		strcat(out, "...");
}

static void sql_trace_log(const char *phase, MYSQL *conn, const char *sql)
{
	if (!conn)
		return;
	const bool trace_on  = sql_trace_enabled();
	const bool burst_on  = (sql_trace_burst > 0 && !trace_on);
	if (!trace_on && !burst_on)
		return;
	if (burst_on)
		--sql_trace_burst;

	char preview[240];
	sql_trace_preview(sql, preview, sizeof(preview));
	logit(LOG_DEBUG,
	      "[SQLTRACE] phase=%s conn=%lu kind=%s burst=%d errno=%u field_count=%u more_results=%d sql=\"%s\"",
	      phase,
	      (unsigned long)mysql_thread_id(conn),
	      sql_trace_kind(sql),
	      sql_trace_burst,
	      (unsigned int)mysql_errno(conn),
	      (unsigned int)mysql_field_count(conn),
	      mysql_more_results(conn),
	      preview);
}

static void sql_trace_log_drain(MYSQL *conn, const char *phase, bool drained)
{
	char preview[240];

	if (!conn)
		return;
	if (drained)
		sql_trace_burst = 100;
	if (!sql_trace_active() && !drained)
		return;

	sql_trace_preview(sql_trace_last_sql, preview, sizeof(preview));
	logit(LOG_DEBUG,
	      "[SQLTRACE] phase=%s conn=%lu drained=%d burst=%d errno=%u field_count=%u more_results=%d site=%s sql=\"%s\"",
	      phase,
	      (unsigned long)mysql_thread_id(conn),
	      drained ? 1 : 0,
	      sql_trace_burst,
	      (unsigned int)mysql_errno(conn),
	      (unsigned int)mysql_field_count(conn),
	      mysql_more_results(conn),
	      *sql_trace_last_site ? sql_trace_last_site : "unknown",
	      preview);
}

void sql_trace_panic(void)
{
	sql_trace_burst = 100;
}

bool sql_trace_exec(const char *site, const char *sql, size_t len, bool drain_before, bool drain_after)
{
	if (!DB || !sql)
		return false;

	snprintf(sql_trace_last_site, sizeof(sql_trace_last_site), "%s", site ? site : "unknown");
	sql_trace_preview(sql, sql_trace_last_sql, sizeof(sql_trace_last_sql));

	if (drain_before)
		sql_clear_results_on(DB);
	sql_trace_log(site, DB, sql);
	if (mysql_real_query(DB, sql, len) != 0)
	{
		logit(LOG_STATUS, "MySQL error: %s", mysql_error(DB));
		logit(LOG_STATUS, "on MySQL query: %s", sql);
		sql_trace_panic();
		sql_trace_log("exec/fail", DB, sql);
		return false;
	}

	if (drain_after)
		sql_clear_results_on(DB);
	sql_trace_log("exec/ok", DB, sql);
	return true;
}

void sql_clear_results_on(MYSQL *conn)
{
	if (!conn)
		return;

	int  status = 0;
	bool drained_any = false;
	do
	{
		/* did current statement return data? */
		MYSQL_RES *result = mysql_store_result(conn);
		if (result)
		{
			my_ulonglong rows = mysql_num_rows(result);
			unsigned int fields = mysql_num_fields(result);
			drained_any = true;
			logit(LOG_DEBUG,
			      "[SQLTRACE] phase=%s conn=%lu drained_result rows=%llu fields=%u more_results=%d",
			      "clear/result",
			      (unsigned long)mysql_thread_id(conn),
			      (unsigned long long)rows,
			      fields,
			      mysql_more_results(conn));
			mysql_free_result(result);
		}
		else /* no result set or error */
		{
			if (mysql_field_count(conn) == 0)
			{
				// printf("%lld rows affected\n", mysql_affected_rows(conn));
			}
			else if (mysql_errno(conn) == 0)
			{
				// Benign pre-clear: no pending result and no MySQL error.
				// Keep this silent; the query/site trace already shows the caller.
			}
			else /* actual error occurred */
			{
				logit(LOG_DEBUG, "MySQL error: %s", mysql_error(conn));
				sql_trace_log_drain(conn, "clear/error", true);
				break;
			}
		}
		/* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
		if ((status = mysql_next_result(conn)) > 0)
		{
			logit(LOG_DEBUG, "MySQL error: %s", mysql_error(conn));
			sql_trace_log_drain(conn, "clear/next_result_error", true);
			break;
		}
	} while (status == 0);

	if (drained_any)
		sql_trace_log_drain(conn, "clear/drained", true);
}


void sql_clear_results()
{
	sql_clear_results_on(DB);
}

/* Execute a semicolon-separated multi-statement query.
 * Uses mysql_real_query (supports multi-statement with
 * CLIENT_MULTI_STATEMENTS) and drains all result sets. */
/* Execute a semicolon-separated multi-statement query.
 * Uses mysql_real_query (supports multi-statement with
 * CLIENT_MULTI_STATEMENTS) and drains all result sets. */
bool sql_run_multi_query(const char *query)
{
	if (!DB || !query || !*query)
		return false;

	if (!sql_trace_exec("sql_run_multi_query", query, strlen(query), true, false))
	{
		sql_player_error("sql_run_multi_query", query);
		// Drain any partial result sets from statements that succeeded
		// before the failing one (multi-statement with CLIENT_MULTI_STATEMENTS).
		sql_clear_results();
		return false;
	}

	sql_clear_results();
	return true;
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

	if (!sql_trace_exec("qry/direct", buf, strlen(buf), true, false))
	{
		logit(LOG_DEBUG, "MySQL error: %s", mysql_error(DB));
		logit(LOG_DEBUG, "on MySQL query: %s", buf);
		return FALSE;
	}

	return TRUE;
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

	std::vector<int> delete_ids;
	MYSQL_ROW      row;
	while ((row = mysql_fetch_row(res)))
	{
		send_to_char(row[1], ch);
		delete_ids.push_back(atoi(row[0]));
	}

	mysql_free_result(res);

	for (int id : delete_ids)
	{
		qry("DELETE FROM offline_messages WHERE id = '%d'", id);
	}
}

int sql_shop_sell(P_char ch, P_obj obj, int value)
{
	int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;

	int pid = (IS_PC(ch) ? GET_PID(ch) : 0);

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

	int m_virtual = GET_VNUM(giver);
	// GET_PID(ch), ch->only.pc->quest_giver, GET_NAME(ch), GET_LEVEL(ch), ch->only.pc->quest_mob_vnum, m_virtual ,reward->short_description );
	char buff[MAX_STRING_LENGTH];
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

void log_epic_gain(int pid, int type, int type_id, int epics) { qry("INSERT INTO epic_gain (pid, time, type, type_id, epics) values ('%d', now(), '%d', '%d', '%d')", pid, type, type_id, epics); }

void log_epic_gain_event(const char *event_key, int pid, int type, int type_id, int epics)
{
	char line[PERSISTENCE_EVENT_MAX_LEN];

	snprintf(line,
	         sizeof(line),
	         "INSERT INTO epic_gain (pid, time, type, type_id, epics) VALUES ('%d', NOW(), '%d', '%d', '%d')",
	         pid, type, type_id, epics);

	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return;
		if (persistence_write_fallback_event_line(line, "scalar_event", "epic_gain", "queue_full_flat_fallback"))
			return;
	}

	qry("INSERT INTO epic_gain (pid, time, type, type_id, epics) VALUES ('%d', NOW(), '%d', '%d', '%d')",
	    pid, type, type_id, epics);
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

	sql_clear_results_on(DB);
	if (!sql_trace_exec("do_sql", argument, strlen(argument), false, false))
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

	if (ch->desc && *ch->desc->host)
	{
		snprintf(ip_buff, sizeof ip_buff, "%s", ch->desc->host);
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
	if (!qry("select * from artifact_bind where vnum = %d", vnum))
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
			*owner_pid = atoi(row[1]);
			*timer     = atoi(row[2]);
		}
	}
	mysql_free_result(res);
}

void sql_update_bind_data(int vnum, int *owner_pid, int *timer)
{
	if (!qry("select * from artifact_bind where vnum = %d", vnum))
	{
		logit(LOG_DEBUG, "sql_update_bind_data(): failed to read from database");
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (mysql_num_rows(res) > 0)
	{
		qry("UPDATE artifact_bind SET owner_pid = %d, timer = %d WHERE vnum = %d", *owner_pid, *timer, vnum);
	}
	else
	{
		qry("INSERT INTO artifact_bind VALUES(%d, %d, %d)", vnum, *owner_pid, *timer);
	}
	mysql_free_result(res);
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

/* Verify every runtime table and column referenced by sql_pwipe() before the
 * first destructive statement.  This must describe final runtime schema, not
 * migration-only helper tables. */
bool sql_verify_pwipe_manifest(void)
{
	static const char *const tables[] = {
		"account_bound_rewards", "account_bound_reward_summons", "account_bound_reward_pwipe_state", "account_characters",
		"account_locker_access", "account_locker_item_affects",
		"account_locker_item_extra_descr", "account_locker_items", "account_lockers",
		"alliances", "artifact_bind", "artifacts", "artifacts_mortal", "associations",
		"auction_bid_history", "auction_item_pickups", "auction_money_pickups", "auctions",
		"boons", "boons_progress", "boons_shop", "corpse_item_affects",
		"corpse_item_extra_descr", "corpse_items", "corpses", "ctf_data", "epic_bonus",
		"epic_gain", "eq_drop", "frag_leaderboard", "guild_members", "guild_ranks",
		"guild_transactions", "guildhall_rooms", "guildhalls", "guilds", "ip_info",
		"level_cap", "locker_access", "locker_activity_log", "locker_chests",
		"locker_item_affects", "locker_item_extra_descr", "locker_items", "locker_kickouts",
		"locker_session_state", "lockers", "log_entries", "nexus_stones", "offline_messages",
		"outposts", "persistence_item_events", "persistence_scalar_events", "pkill_event",
		"pkill_info", "player_affects", "player_data", "player_forged_items",
		"player_granted_cmds", "player_intros", "player_item_affects", "player_item_extra_descr",
		"player_items", "player_languages", "player_pet_item_affects",
		"player_pet_item_extra_descr", "player_pet_items", "player_pets", "player_recipes",
		"player_shapechanges", "player_skills", "player_spellbooks", "player_timers",
		"player_undead_slots", "player_witnesses", "poll_options", "poll_votes", "polls",
		"private_chest_log", "private_chests", "progress", "racewar_stat_mods",
		"saved_item_affects", "saved_item_extra_descr", "saved_items", "ship_armor",
		"ship_cargo_market_mods", "ship_cargo_prices", "ship_crew", "ship_slots", "ships",
		"shop_trophy", "shopkeeper_affects", "shopkeeper_item_affects",
		"shopkeeper_item_extra_descr", "shopkeeper_items", "shopkeepers", "siege_item_affects",
		"siege_item_extra_descr", "siege_items", "statistics", "timers", "world_quest_accomplished",
		"zone_touches", "zone_trophy", NULL
	};
	static const char *const columns[][2] = {
		{"account_bound_rewards", "id"}, {"account_bound_rewards", "expires_at"},
		{"account_bound_rewards", "remaining_pwipes"},
		{"account_bound_reward_summons", "grant_id"}, {"account_bound_reward_summons", "pid"},
		{"account_bound_reward_summons", "last_summoned_at"},
		{"account_bound_reward_pwipe_state", "id"}, {"account_bound_reward_pwipe_state", "last_processed_at"},
		{"outposts", "owner_id"}, {"outposts", "level"}, {"outposts", "walls"},
		{"outposts", "archers"}, {"outposts", "hitpoints"}, {"outposts", "territory"},
		{"outposts", "portal_room"}, {"outposts", "resources"}, {"outposts", "applied_resources"},
		{"outposts", "golems"}, {"outposts", "meurtriere"}, {"outposts", "scouts"},
		{"nexus_stones", "align"}, {"nexus_stones", "last_touched_at"},
		{"timers", "date"}, {"account_characters", "deleted_at"},
		{"player_data", "active"}, {"level_cap", "most_frags"}, {"level_cap", "racewar_leader"},
		{"level_cap", "level"}, {"level_cap", "next_update"}, {NULL, NULL}
	};
	char query[8192];
	int pos = snprintf(query, sizeof(query),
		"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN (");
	if (pos < 0 || (size_t)pos >= sizeof(query))
		return FALSE;
	int expected_tables = 0;
	for (int i = 0; tables[i] != NULL; i++)
	{
		int written = snprintf(query + pos, sizeof(query) - (size_t)pos,
			"%s'%s'", i ? "," : "", tables[i]);
		if (written < 0 || (size_t)written >= sizeof(query) - (size_t)pos)
			return FALSE;
		pos += written;
		expected_tables++;
	}
	if (pos + 2 >= (int)sizeof(query))
		return FALSE;
	strcat(query, ")");
	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return FALSE;
	MYSQL_ROW row = mysql_fetch_row(result);
	bool tables_ok = row && row[0] && atoi(row[0]) == expected_tables;
	mysql_free_result(result);
	if (!tables_ok)
		return FALSE;

	pos = snprintf(query, sizeof(query),
		"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND (");
	if (pos < 0 || (size_t)pos >= sizeof(query))
		return FALSE;
	int expected_columns = 0;
	for (int i = 0; columns[i][0] != NULL; i++)
	{
		int written = snprintf(query + pos, sizeof(query) - (size_t)pos,
			"%s(table_name='%s' AND column_name='%s')", i ? " OR " : "",
			columns[i][0], columns[i][1]);
		if (written < 0 || (size_t)written >= sizeof(query) - (size_t)pos)
			return FALSE;
		pos += written;
		expected_columns++;
	}
	if (pos + 2 >= (int)sizeof(query))
		return FALSE;
	strcat(query, ")");
	result = db_query("%s", query);
	if (!result)
		return FALSE;
	row = mysql_fetch_row(result);
	bool columns_ok = row && row[0] && atoi(row[0]) == expected_columns;
	mysql_free_result(result);
	return columns_ok;
}

/* Season-reset preflight: verify persistence event schema and auction engines.
 * These are the same checks as sql_verify_boot_database() but are callable
 * from sql_pwipe() as a preflight gate without requiring a fresh boot. */
bool sql_verify_persistence_schema(void)
{
	if (!DB)
		return FALSE;

	/* Verify persistence event columns exist. */
	const char *event_schema_probe =
		"SELECT COUNT(*) FROM information_schema.columns "
		"WHERE table_schema=DATABASE() AND "
		"((table_name='persistence_item_events' AND column_name IN "
		"('id','ts_usec','event_type','item_uid','vnum','item','actor','actor_id','source','target','note','dedupe_key','created_at')) "
		"OR (table_name='persistence_scalar_events' AND column_name IN "
		"('id','event_type','event_key','boot_time','touched_at','zone_number','toucher_pid','group_size','epic_value','alignment_delta','dedupe_key','created_at')))";
	MYSQL_RES *result = db_query("%s", event_schema_probe);
	if (!result)
		return FALSE;
	MYSQL_ROW row = mysql_fetch_row(result);
	bool event_columns_ok = row && row[0] && atoi(row[0]) == 25;
	mysql_free_result(result);
	if (!event_columns_ok)
		return FALSE;

	/* Verify persistence event indexes exist. */
	const char *event_index_probe =
		"SELECT COUNT(*) FROM (SELECT DISTINCT table_name, index_name "
		"FROM information_schema.statistics WHERE table_schema=DATABASE() AND "
		"((table_name='persistence_item_events' AND index_name IN "
		"('PRIMARY','idx_item_uid_ts','idx_event_type_created','uq_item_dedupe')) "
		"OR (table_name='persistence_scalar_events' AND index_name IN "
		"('PRIMARY','idx_scalar_event_key','idx_scalar_zone_time','uq_scalar_dedupe')))) "
		"AS required_indexes";
	result = db_query("%s", event_index_probe);
	if (!result)
		return FALSE;
	row = mysql_fetch_row(result);
	bool event_indexes_ok = row && row[0] && atoi(row[0]) == 8;
	mysql_free_result(result);
	return event_indexes_ok;
}

bool sql_verify_auction_engines(void)
{
	if (!DB)
		return FALSE;

	const char *auction_engine_probe =
		"SELECT COUNT(DISTINCT table_name) FROM information_schema.tables "
		"WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN "
		"('auction_bid_history','auction_item_pickups','auction_money_pickups','auctions')";
	MYSQL_RES *result = db_query("%s", auction_engine_probe);
	if (!result)
		return FALSE;
	MYSQL_ROW row = mysql_fetch_row(result);
	bool auction_engines_ok = row && row[0] && atoi(row[0]) == 4;
	mysql_free_result(result);
	return auction_engines_ok;
}

bool sql_pwipe(int code_verify)
{
	logit(LOG_DEBUG, "sql_pwipe: STARTED!");
	if (code_verify == 1723699)
	{
		/* ── Preflight: verify critical tables exist and have correct engines ── */
		logit(LOG_DEBUG, "sql_pwipe: Preflight schema check... .. .");
		send_to_all("Preflight schema check... .. .");
		if (!sql_verify_persistence_schema())
		{
			logit(LOG_DEBUG, "sql_pwipe: Preflight failed: persistence schema incomplete.");
			send_to_all("Preflight FAILED: persistence schema incomplete!\n");
			return FALSE;
		}
		if (!sql_verify_pwipe_manifest())
		{
			logit(LOG_DEBUG, "sql_pwipe: Preflight failed: reset manifest schema incomplete.");
			send_to_all("Preflight FAILED: reset manifest schema incomplete!\n");
			return FALSE;
		}
		/* Verify auction tables are InnoDB (transactional) before reset. */
		if (!sql_verify_auction_engines())
		{
			logit(LOG_DEBUG, "sql_pwipe: Preflight failed: auction tables not InnoDB.");
			send_to_all("Preflight FAILED: auction tables must be InnoDB!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "  success!");
		send_to_all("  success!\n");
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
		/* ── Season-reset manifest: player-owned item graphs ── */
		/* Child tables (affects, extra_descr) must be cleared before parent item tables. */
		logit(LOG_DEBUG, "sql_pwipe: Clearing player item subtable data... .. .");
		send_to_all("Clearing player item subtable data... .. .");
		if (qry("DELETE FROM player_item_affects") && qry("DELETE FROM player_item_extra_descr"))
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
		logit(LOG_DEBUG, "sql_pwipe: Clearing player items... .. .");
		send_to_all("Clearing player items... .. .");
		if (qry("DELETE FROM player_items"))
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
		/* ── Season-reset manifest: player pet graphs ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing player pet subtable data... .. .");
		send_to_all("Clearing player pet subtable data... .. .");
		if (qry("DELETE FROM player_pet_item_affects") && qry("DELETE FROM player_pet_item_extra_descr") && qry("DELETE FROM player_pet_items"))
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
		logit(LOG_DEBUG, "sql_pwipe: Clearing player pets... .. .");
		send_to_all("Clearing player pets... .. .");
		if (qry("DELETE FROM player_pets"))
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
		/* ── Season-reset manifest: player character state ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing player character state data... .. .");
		send_to_all("Clearing player character state data... .. .");
		if (qry("DELETE FROM player_affects") && qry("DELETE FROM player_skills") && qry("DELETE FROM player_spellbooks") &&
		    qry("DELETE FROM player_languages") && qry("DELETE FROM player_timers") && qry("DELETE FROM player_recipes") &&
		    qry("DELETE FROM player_shapechanges") && qry("DELETE FROM player_undead_slots") &&
		    qry("DELETE FROM player_witnesses") && qry("DELETE FROM player_forged_items") &&
		    qry("DELETE FROM player_granted_cmds") && qry("DELETE FROM player_intros"))
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
		/* ── Season-reset manifest: locker/chest graphs ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing locker subtable data... .. .");
		send_to_all("Clearing locker subtable data... .. .");
		if (qry("DELETE FROM locker_item_affects") && qry("DELETE FROM locker_item_extra_descr") &&
		    qry("DELETE FROM locker_items") && qry("DELETE FROM locker_chests") &&
		    qry("DELETE FROM locker_activity_log") && qry("DELETE FROM locker_kickouts") &&
		    qry("DELETE FROM locker_session_state"))
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
		logit(LOG_DEBUG, "sql_pwipe: Clearing locker data... .. .");
		send_to_all("Clearing locker data... .. .");
		if (qry("DELETE FROM lockers"))
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
		/* ── Season-reset manifest: account locker graphs ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing account locker data... .. .");
		send_to_all("Clearing account locker data... .. .");
		if (qry("DELETE FROM account_locker_item_affects") && qry("DELETE FROM account_locker_item_extra_descr") &&
		    qry("DELETE FROM account_locker_items") && qry("DELETE FROM account_locker_access") &&
		    qry("DELETE FROM account_lockers"))
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
		/* ── Season-reset manifest: private chests ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing private chest data... .. .");
		send_to_all("Clearing private chest data... .. .");
		if (qry("DELETE FROM private_chest_log") && qry("DELETE FROM private_chests"))
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
		/* ── Season-reset manifest: corpse graphs ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing corpse subtable data... .. .");
		send_to_all("Clearing corpse subtable data... .. .");
		if (qry("DELETE FROM corpse_item_affects") && qry("DELETE FROM corpse_item_extra_descr") && qry("DELETE FROM corpse_items"))
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
		logit(LOG_DEBUG, "sql_pwipe: Clearing corpse data... .. .");
		send_to_all("Clearing corpse data... .. .");
		if (qry("DELETE FROM corpses"))
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
		/* ── Season-reset manifest: saved item graphs ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing saved item data... .. .");
		send_to_all("Clearing saved item data... .. .");
		if (qry("DELETE FROM saved_item_affects") && qry("DELETE FROM saved_item_extra_descr") && qry("DELETE FROM saved_items"))
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
		/* ── Season-reset manifest: ship graphs ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing ship subtable data... .. .");
		send_to_all("Clearing ship subtable data... .. .");
		if (qry("DELETE FROM ship_armor") && qry("DELETE FROM ship_crew") && qry("DELETE FROM ship_slots"))
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
		logit(LOG_DEBUG, "sql_pwipe: Clearing ship data... .. .");
		send_to_all("Clearing ship data... .. .");
		if (qry("DELETE FROM ships"))
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
		/* ── Season-reset manifest: guild membership ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing guild membership data... .. .");
		send_to_all("Clearing guild membership data... .. .");
		if (qry("DELETE FROM guild_members") && qry("DELETE FROM guild_ranks") && qry("DELETE FROM guilds"))
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
		/* ── Season-reset manifest: world/competitive state ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing artifact data... .. .");
		send_to_all("Clearing artifact data... .. .");
		if (qry("DELETE FROM artifacts") && qry("DELETE FROM artifacts_mortal"))
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
		logit(LOG_DEBUG, "sql_pwipe: Clearing pvp and statistics data... .. .");
		send_to_all("Clearing pvp and statistics data... .. .");
		if (qry("DELETE FROM frag_leaderboard") && qry("DELETE FROM pkill_event") && qry("DELETE FROM pkill_info") &&
		    qry("DELETE FROM statistics") && qry("DELETE FROM eq_drop") && qry("DELETE FROM racewar_stat_mods"))
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
		/* ── Season-reset manifest: siege and shopkeeper graphs ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing siege and shopkeeper data... .. .");
		send_to_all("Clearing siege and shopkeeper data... .. .");
		if (qry("DELETE FROM siege_item_affects") && qry("DELETE FROM siege_item_extra_descr") && qry("DELETE FROM siege_items") &&
		    qry("DELETE FROM shopkeeper_affects") && qry("DELETE FROM shopkeeper_item_affects") &&
		    qry("DELETE FROM shopkeeper_item_extra_descr") && qry("DELETE FROM shopkeeper_items") &&
		    qry("DELETE FROM shopkeepers"))
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
		/* ── Season-reset manifest: polls and boon shop ── */
		logit(LOG_DEBUG, "sql_pwipe: Clearing poll and boon shop data... .. .");
		send_to_all("Clearing poll and boon shop data... .. .");
		if (qry("DELETE FROM poll_votes") && qry("DELETE FROM poll_options") && qry("DELETE FROM polls") &&
		    qry("DELETE FROM boons_shop"))
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
		/* ── Season-reset manifest: soft-delete account characters ── */
		logit(LOG_DEBUG, "sql_pwipe: Soft-deleting account characters... .. .");
		send_to_all("Soft-deleting account characters... .. .");
		if (qry("UPDATE account_characters SET deleted_at = COALESCE(deleted_at, NOW()) WHERE deleted_at IS NULL"))
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
		logit(LOG_DEBUG, "sql_pwipe: Clearing persistence event data... .. .");
		send_to_all("Clearing persistence event data... .. .");
		if (qry("DELETE FROM persistence_item_events") && qry("DELETE FROM persistence_scalar_events"))
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
		if (qry("UPDATE level_cap SET most_frags=0, racewar_leader=0, level=%d, next_update=NOW() + INTERVAL %d DAY",
		         frag_cap_config_reset_level(),
		         frag_cap_config_reset_timer_days()))
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
		if (!redis_clear_pwipe_state())
		{
			logit(LOG_DEBUG, "sql_pwipe: Redis pwipe invalidation failed.");
			return FALSE;
		}
		/* ── Postflight: verify critical season-scoped tables are empty ── */
		logit(LOG_DEBUG, "sql_pwipe: Postflight invariant check... .. .");
		send_to_all("Postflight invariant check... .. .");
		{
			/* Verify that player_items, player_pets, lockers, ships, guilds, and
			 * persistence event tables are all empty after reset. */
			const char *postflight_tables[] = {
				"player_items", "player_pets", "player_affects", "player_skills",
				"player_spellbooks", "player_languages", "player_timers",
				"lockers", "locker_items", "private_chests",
				"corpses", "corpse_items", "saved_items",
				"ships", "ship_slots", "ship_crew", "ship_armor",
				"guilds", "guild_members", "guild_ranks",
				"artifacts", "frag_leaderboard", "statistics",
				"persistence_item_events", "persistence_scalar_events",
				"polls", "shopkeepers",
				NULL
			};
			bool postflight_ok = TRUE;
			for (int i = 0; postflight_tables[i] != NULL; i++)
			{
				MYSQL_RES *res = db_query("SELECT COUNT(*) FROM %s", postflight_tables[i]);
				if (res)
				{
					MYSQL_ROW row = mysql_fetch_row(res);
					if (row && row[0] && atoi(row[0]) > 0)
					{
						logit(LOG_DEBUG, "sql_pwipe: Postflight FAILED: %s has %s rows.",
						      postflight_tables[i], row[0]);
						postflight_ok = FALSE;
					}
					mysql_free_result(res);
				}
				else
				{
					logit(LOG_DEBUG, "sql_pwipe: Postflight FAILED: cannot query %s.",
					      postflight_tables[i]);
					postflight_ok = FALSE;
				}
			}
			/* Verify player_data is all inactive. */
			MYSQL_RES *res = db_query("SELECT COUNT(*) FROM player_data WHERE active = 1");
			if (res)
			{
				MYSQL_ROW row = mysql_fetch_row(res);
				if (row && row[0] && atoi(row[0]) > 0)
				{
					logit(LOG_DEBUG, "sql_pwipe: Postflight FAILED: %d active player_data rows.", atoi(row[0]));
					postflight_ok = FALSE;
				}
				mysql_free_result(res);
			}
			else
			{
				logit(LOG_DEBUG, "sql_pwipe: Postflight FAILED: cannot query player_data active count.");
				postflight_ok = FALSE;
			}
			/* Verify account_characters are all soft-deleted. */
			res = db_query("SELECT COUNT(*) FROM account_characters WHERE deleted_at IS NULL");
			if (res)
			{
				MYSQL_ROW row = mysql_fetch_row(res);
				if (row && row[0] && atoi(row[0]) > 0)
				{
					logit(LOG_DEBUG, "sql_pwipe: Postflight FAILED: %d active account_characters rows.", atoi(row[0]));
					postflight_ok = FALSE;
				}
				mysql_free_result(res);
			}
			else
			{
				logit(LOG_DEBUG, "sql_pwipe: Postflight FAILED: cannot query account_characters.");
				postflight_ok = FALSE;
			}
			if (!postflight_ok)
			{
				logit(LOG_DEBUG, "sql_pwipe: Postflight invariants failed.");
				send_to_all("Postflight FAILED: season-reset invariants not satisfied!\n");
				return FALSE;
			}
		}
		if (!account_bound_rewards_on_successful_pwipe())
		{
			logit(LOG_DEBUG, "sql_pwipe: account reward pwipe policy failed; preserving rewards for manual review.");
			send_to_all("Account reward pwipe policy FAILED; rewards are being preserved for manual review.\n");
		}
		logit(LOG_DEBUG, "  success!");
		send_to_all("  success!\n");
		logit(LOG_DEBUG, "sql_pwipe: COMPLETED!");
		send_to_all("WIPE COMPLETED!");
		sleep(1);
		return TRUE;
	}
	else
	{
		logit(LOG_DEBUG, "sql_pwipe: Someone called sql_pwipe with a bad verify code... hrm..");
		return FALSE;
	}
}

void sql_log_player_login(P_char ch, const char *status)
{
	if (!ch || IS_NPC(ch) || !ch->desc)
		return;

	// Async scalar-event queue replaces fork().
	// Escape all user-supplied strings then build the INSERT line.
	char line[PERSISTENCE_EVENT_MAX_LEN];
	char esc_name[128], esc_ip[128], esc_account[128], esc_client[256];
	char esc_status[64], esc_client_ver[128];

	const char *acct = get_account_name_safe(ch);

	persistence_sql_escape_field(GET_NAME(ch), esc_name, sizeof(esc_name));
	persistence_sql_escape_field(ch->desc->host, esc_ip, sizeof(esc_ip));
	persistence_sql_escape_field(acct ? acct : "", esc_account, sizeof(esc_account));
	persistence_sql_escape_field(ch->desc->client_name[0] ? ch->desc->client_name : "",
	                             esc_client, sizeof(esc_client));
	persistence_sql_escape_field(ch->desc->client_version[0] ? ch->desc->client_version : "",
	                             esc_client_ver, sizeof(esc_client_ver));
	persistence_sql_escape_field(status, esc_status, sizeof(esc_status));

	snprintf(line, sizeof(line),
	         "INSERT INTO log_entries (date, kind, ip_address, pid, player_name, zone_number, room_vnum, message) "
	         "VALUES (NOW(), '%s', '%s', %d, '%s', 0, 0, 'account=%s client=%s %s')",
	         esc_status,
	         esc_ip,
	         GET_PID(ch),
	         esc_name,
	         esc_account,
	         esc_client,
	         esc_client_ver);

	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return;
		if (persistence_write_fallback_event_line(line, "scalar_event",
		                                          "player_login",
		                                          "queue_full_fallback"))
			return;
	}

	// Fallback: execute synchronously
	qry("%s", line);
}

/* ---- Persistence DB connection ---- */
MYSQL *sql_persistence_connection(void)
{
	/* Prefer the connection pool. Only fall back to the legacy singleton
	 * when no active pool exists (bootstrap/early-start). If an active pool
	 * is exhausted, fail closed so callers can alert/retry without blocking
	 * the main loop or creating an unscheduled fifth connection. */
	int pool_was_active = 0;
	MYSQL *conn = sql_pool_acquire_with_status(&pool_was_active);
	if (conn)
		return conn;
	if (pool_was_active)
		return NULL;

	/* Legacy fallback: lazy-initialise the singleton.
	 * Kept for bootstrap / early-start paths that run before
	 * the pool is initialised (e.g. sql_populate_lookup_tables).
	 * Must use the shared db-name resolver so pool/fallback never
	 * disagrees with the main DB connection about which database
	 * is the production one. */
	if (!persistenceDB)
	{
		pthread_mutex_lock(&persistence_sql_mutex);
		if (!persistenceDB)
		{
			persistenceDB = mysql_init(NULL);
			if (!persistenceDB)
			{
				pthread_mutex_unlock(&persistence_sql_mutex);
				return NULL;
			}
			if (!mysql_real_connect(persistenceDB, DB_HOST, DB_USER, DB_PASSWD,
			                        sql_persistence_db_name(), DB_PORT, NULL, 0))
			{
				logit(LOG_DEBUG, "Persistence MySQL: sql_persistence_connection failed: %s", mysql_error(persistenceDB));
				mysql_close(persistenceDB);
				persistenceDB = NULL;
				pthread_mutex_unlock(&persistence_sql_mutex);
				return NULL;
			}
		}
		pthread_mutex_unlock(&persistence_sql_mutex);
	}
	return persistenceDB;
}

/* Return a connection previously acquired via
 * sql_persistence_connection().  Pool connections go back to the pool;
 * legacy singleton connections are a no-op (they're owned by the
 * caller indefinitely). */
void sql_persistence_release_connection(MYSQL *conn)
{
	if (!conn)
		return;

	/* If this connection came from the pool, release it.  The pool
	 * checks pointer equality against its slots, so passing a
	 * non-pool connection is harmless — it simply won't match. */
	sql_pool_release(conn);
}

static bool persistence_decimal_field(const char *value, bool allow_negative)
{
	char *end = NULL;
	if (!value || !*value)
		return false;
	errno = 0;
	if (allow_negative)
		(void)strtol(value, &end, 10);
	else
		(void)strtoull(value, &end, 10);
	return errno == 0 && end && *end == '\0';
}

bool sql_persistence_write_item_event_line(const char *line)
{
	const char *item_event_prefix = "PERSISTENCE_ITEM_EVENT|";
	char record[2048];
	char *saveptr = NULL;
	char *field;
	char *value;
	char event[128] = "";
	char ts_usec[32] = "0";
	char item_uid[32] = "0";
	char vnum[32] = "-1";
	char item[256] = "";
	char actor[128] = "";
	char actor_id[32] = "-1";
	char source[256] = "";
	char target[256] = "";
	char note[256] = "";
	char event_q[256], item_q[64], vnum_q[32], item_text_q[512];
	char actor_q[256], actor_id_q[32], source_q[512], target_q[512], note_q[512];
	int seen_ts = 0, seen_event = 0, seen_item_uid = 0;
	int seen_vnum = 0, seen_actor_id = 0;
	char query[4096];
	int query_len;

	if (!line || strncmp(line, item_event_prefix,
	                     strlen(item_event_prefix)))
		return false;
	if (strlen(line) >= sizeof(record))
		return false;
	strcpy(record, line);

	field = strtok_r(record, "|", &saveptr);
	while ((field = strtok_r(NULL, "|", &saveptr)) != NULL)
	{
		value = strchr(field, '=');
		if (!value)
			return false;
		*value++ = '\0';
		if (!strcmp(field, "ts")) { seen_ts = 1; snprintf(ts_usec, sizeof(ts_usec), "%s", value); }
		else if (!strcmp(field, "event")) { seen_event = 1; snprintf(event, sizeof(event), "%s", value); }
		else if (!strcmp(field, "item_uid")) { seen_item_uid = 1; snprintf(item_uid, sizeof(item_uid), "%s", value); }
		else if (!strcmp(field, "vnum")) { seen_vnum = 1; snprintf(vnum, sizeof(vnum), "%s", value); }
		else if (!strcmp(field, "item")) snprintf(item, sizeof(item), "%s", value);
		else if (!strcmp(field, "actor")) snprintf(actor, sizeof(actor), "%s", value);
		else if (!strcmp(field, "actor_id")) { seen_actor_id = 1; snprintf(actor_id, sizeof(actor_id), "%s", value); }
		else if (!strcmp(field, "source")) snprintf(source, sizeof(source), "%s", value);
		else if (!strcmp(field, "target")) snprintf(target, sizeof(target), "%s", value);
		else if (!strcmp(field, "note")) snprintf(note, sizeof(note), "%s", value);
	}

	if (!seen_ts || !seen_event || !seen_item_uid || !seen_vnum || !seen_actor_id ||
	    !persistence_decimal_field(ts_usec, false) ||
	    !persistence_decimal_field(item_uid, false) ||
	    !persistence_decimal_field(vnum, true) ||
	    !persistence_decimal_field(actor_id, true))
		return false;

	persistence_sql_escape_field(event, event_q, sizeof(event_q));
	persistence_sql_escape_field(item, item_text_q, sizeof(item_text_q));
	persistence_sql_escape_field(actor, actor_q, sizeof(actor_q));
	persistence_sql_escape_field(source, source_q, sizeof(source_q));
	persistence_sql_escape_field(target, target_q, sizeof(target_q));
	persistence_sql_escape_field(note, note_q, sizeof(note_q));
	snprintf(item_q, sizeof(item_q), "%s", item_uid);
	snprintf(vnum_q, sizeof(vnum_q), "%s", vnum);
	snprintf(actor_id_q, sizeof(actor_id_q), "%s", actor_id);
	query_len = snprintf(query, sizeof(query),
	         "INSERT INTO persistence_item_events "
	         "(ts_usec,event_type,item_uid,vnum,item,actor,actor_id,source,target,note,dedupe_key) "
	         "VALUES (%s,'%s',%s,%s,'%s','%s',%s,'%s','%s','%s',"
	         "SHA2(CONCAT_WS('|',%s,'%s',%s,%s,'%s','%s',%s,'%s','%s','%s'),256)) "
	         "ON DUPLICATE KEY UPDATE id=id",
	         ts_usec, event_q, item_q,
	         vnum_q, item_text_q, actor_q, actor_id_q, source_q, target_q,
	         note_q, ts_usec, event_q, item_q, vnum_q, item_text_q, actor_q,
	         actor_id_q, source_q, target_q, note_q);
	if (query_len < 0 || query_len >= (int)sizeof(query))
		return false;
	return sql_persistence_execute_raw(query);
}

bool sql_persistence_write_scalar_event_line(const char *line)
{
	return sql_persistence_execute_raw(line);
}

bool sql_persistence_write_large_event_line(const char *line)
{
	return sql_persistence_execute_raw(line);
}

/* Validates that an item's persistence_event log matches its expected
 * owner.  Queries persistence_item_events for the most recent event
 * involving item_uid and checks that the target field matches the
 * expected owner_type:owner_ref.
 *
 * Returns true if:
 *   - item_uid == 0 (no ownership data, keep item)
 *   - No recent event found (safe default, keep item)
 *   - Most recent event target matches expected owner
 * Returns false if the item was last seen at a different owner (stolen item)
 */
bool sql_persistence_item_owner_matches(unsigned long long item_uid,
                                        const char *owner_type,
                                        const char *owner_ref,
                                        const char *context)
{
	/* No ownership data to validate */
	if (item_uid == 0)
		return true;

	if (!owner_type || !owner_ref || !context)
		return true;

	if (!DB)
		return true;

	/* Build the expected target prefix, e.g. "player:", "locker:", "corpse:" */
	char expected_prefix[64];
	snprintf(expected_prefix, sizeof(expected_prefix), "%s:", owner_type);
	size_t prefix_len = strlen(expected_prefix);

	/* Query the most recent persistence event for this item_uid.
	 * ORDER BY ts_usec DESC, id DESC gives us the latest event. */
	char query[512];
	snprintf(query, sizeof(query),
	         "SELECT target FROM persistence_item_events "
	         "WHERE item_uid=%llu "
	         "ORDER BY ts_usec DESC, id DESC LIMIT 1",
	         item_uid);

	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return true;  /* query failed, keep item (conservative) */

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row || !row[0])
	{
		mysql_free_result(result);
		return true;  /* no events found, keep item (conservative) */
	}

	const char *target = row[0];

	/* Check that the target starts with the expected owner_type prefix */
	if (strncmp(target, expected_prefix, prefix_len) != 0)
	{
		logit(LOG_DEBUG,
		      "sql_persistence: OWNERSHIP MISMATCH item_uid=%llu "
		      "expected=%s%s actual_target=%s context=%s",
		      item_uid, expected_prefix, owner_ref, target, context);
		mysql_free_result(result);
		return false;
	}

	/* Extract the owner ref from target (after the prefix) and compare */
	const char *actual_ref = target + prefix_len;
	if (strcmp(actual_ref, owner_ref) != 0)
	{
		logit(LOG_DEBUG,
		      "sql_persistence: OWNERSHIP MISMATCH item_uid=%llu "
		      "expected=%s%s actual_target=%s context=%s",
		      item_uid, expected_prefix, owner_ref, target, context);
		mysql_free_result(result);
		return false;
	}

	mysql_free_result(result);
	return true;
}

/* Logs zone touch events to persistence_scalar_events for epic analysis.
 * Uses the async persistence queue with flat-file and direct SQL fallbacks. */
void sql_zone_touch_finished(const char *event_key, int boot_time,
                             int touched_at, int zone_number,
                             int toucher_pid, int group_size,
                             int epic_value, int alignment_delta)
{
	char line[PERSISTENCE_EVENT_MAX_LEN];
	const char *safe_key;

	safe_key = (event_key && *event_key) ? event_key : "none";

	snprintf(line, sizeof(line),
	         "INSERT INTO persistence_scalar_events "
	         "(event_type, event_key, boot_time, touched_at, zone_number, "
	         "toucher_pid, group_size, epic_value, alignment_delta, dedupe_key, created_at) "
	         "VALUES ('zone_touch', '%s', %d, %d, %d, %d, %d, %d, %d, "
	         "SHA2(CONCAT('zone_touch', '|', '%s', '|', %d, '|', %d, '|', %d, '|', "
	         "%d, '|', %d, '|', %d, '|', %d, '|', %d), 256), NOW()) "
	         "ON DUPLICATE KEY UPDATE id=id",
	         safe_key, boot_time, touched_at, zone_number,
	         toucher_pid, group_size, epic_value, alignment_delta,
	         safe_key, boot_time, touched_at, zone_number,
	         toucher_pid, group_size, epic_value, alignment_delta);

	if (persistence_scalar_event_worker_running())
	{
		if (persistence_scalar_event_queue_enqueue(line))
			return;
		if (persistence_write_fallback_event_line(line, "scalar_event",
		                                          "zone_touch",
		                                          "queue_full_fallback"))
			return;
	}

	qry("INSERT INTO persistence_scalar_events "
	    "(event_type, event_key, boot_time, touched_at, zone_number, "
	    "toucher_pid, group_size, epic_value, alignment_delta, dedupe_key, created_at) "
	    "VALUES ('zone_touch', '%s', %d, %d, %d, %d, %d, %d, %d, "
	    "SHA2(CONCAT('zone_touch', '|', '%s', '|', %d, '|', %d, '|', %d, '|', "
	    "%d, '|', %d, '|', %d, '|', %d, '|', %d), 256), NOW()) "
	    "ON DUPLICATE KEY UPDATE id=id",
	    safe_key, boot_time, touched_at, zone_number,
	    toucher_pid, group_size, epic_value, alignment_delta,
	    safe_key, boot_time, touched_at, zone_number,
	    toucher_pid, group_size, epic_value, alignment_delta);
}
#endif
