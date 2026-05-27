#ifndef __SQL_H_INCLUDED__
#define __SQL_H_INCLUDED__

#include "structs.h"
#include <stdlib.h>

/* default database credentials (fallback if env vars not set) */
#ifdef TEST_MUD
#define DB_HOST_DEFAULT   "127.0.0.1"
#define DB_USER_DEFAULT   "duris"
#define DB_PASSWD_DEFAULT "duris"
#define DB_NAME_DEFAULT   "duris_dev"
#else
#define DB_HOST_DEFAULT   "127.0.0.1"
#define DB_USER_DEFAULT   "duris"
#define DB_PASSWD_DEFAULT "duris"
#define DB_NAME_DEFAULT   "duris"
#endif

#ifdef __CYGWIN_BUILD__
#undef DB_HOST_DEFAULT
#define DB_HOST_DEFAULT "127.0.0.1"
#endif

/* get database credentials from env vars with fallback */
static inline const char *get_db_host(void)
{
	const char *val = getenv("DB_HOST");
	return (val && *val) ? val : DB_HOST_DEFAULT;
}

static inline const char *get_db_user(void)
{
	const char *val = getenv("DB_USER");
	return (val && *val) ? val : DB_USER_DEFAULT;
}

static inline const char *get_db_passwd(void)
{
	const char *val = getenv("DB_PASSWD");
	return (val && *val) ? val : DB_PASSWD_DEFAULT;
}

static inline const char *get_db_name(void)
{
	const char *val = getenv("DB_NAME");
	return (val && *val) ? val : DB_NAME_DEFAULT;
}

static inline int get_db_port(void)
{
	const char *val = getenv("DB_PORT");
	return (val && *val) ? atoi(val) : 0;
}

/* legacy macros for backward compatibility - use functions for new code */
#define DB_HOST   get_db_host()
#define DB_USER   get_db_user()
#define DB_PASSWD get_db_passwd()
#define DB_NAME   get_db_name()
#define DB_PORT   get_db_port()

#ifndef __NO_MYSQL__
#include <mysql.h>
extern MYSQL *DB;
MYSQL_RES    *db_query(const char *format, ...);
#endif

int  load_env_file(void);
int  initialize_mysql();
void sql_populate_lookup_tables();
int  sql_save_player_core(P_char ch);
bool sql_load_player_items(P_char ch);
int  sql_level_cap(int racewar_side);
// void sql_save_progress( int pid, int delta, const char *type );
void sql_modify_frags(P_char ch, int gain);
void sql_save_pkill(P_char ch, P_char victim);
void sql_webinfo_toggle(P_char ch);
void sql_update_level(P_char ch);
void sql_update_money(P_char ch);
void sql_update_playtime(P_char ch);
void sql_update_epics(P_char ch);
void get_log(P_char ch, char *temp);
void manual_log(P_char ch);
void sql_insert_item(P_char ch, P_obj obj, char *desc);
void sql_insert_new_item(P_char ch, P_obj obj);
void perform_wiki_search(P_char ch, const char *buf);
// use to insert a players IP address into the SQL database
void sql_connectIP(P_char ch);
// used to retrieve the last used IP for a player.
const char *sql_select_IP_info(P_char ch, char *buf, size_t bufSize, time_t *lastConnect = NULL, time_t *lastDisconnect = NULL);
int         sql_find_racewar_for_ip(char *ip, int *racewar_side);
// to log disconnect times...
void sql_disconnectIP(P_char ch);
bool qry(const char *format, ...);
bool sql_persistence_write_item_event_line(const char *line);
void sql_world_quest_finished(P_char ch, P_obj obj);
int  sql_world_quest_done_already(P_char ch, int number);
int  sql_world_quest_can_do_another(P_char ch);
void sql_clear_results();

void send_to_pid_offline(const char *msg, int pid);
void send_offline_messages(P_char ch);

int sql_shop_sell(P_char ch, P_obj obj, int value);
int sql_shop_trophy(P_obj obj);
int sql_quest_finish(P_char ch, P_char giver, int type, int value);
int sql_quest_trophy(P_char giver);

void log_epic_gain(int pid, int type, int type_id, int epics);
void do_sql(P_char ch, char *argument, int cmd);

void update_zone_db();
void update_zone_epic_level(int, int);

void show_frag_trophy(P_char ch, P_char who);

// Frag leaderboard hybrid system - for web statistics
void   sql_update_frag_leaderboard(P_char ch);
void   sql_update_account_character(P_char ch);
double sql_get_total_donated(const char *account_name);
void   sql_soft_delete_character(long pid);

string get_mud_info(const char *name);
void   send_mud_info(const char *name, P_char ch);

string escape_str(const char *str);

#include <vector>
using namespace std;

void zone_trophy_update();

#define PLAYERLOG  "player"
#define WIZLOG     "wiz"
#define QUESTLOG   "quest"
#define EXPLOG     "exp"
#define CONNECTLOG "connect"

#define FRAG_LEVEL_RATIO      0.429
#define FRAGS_TO_LEVEL(frags) ((int)(frags / FRAG_LEVEL_RATIO) + 25)
#define LEVEL_TO_FRAGS(level) ((level <= 25) ? 0 : ((float)(level - 25) * FRAG_LEVEL_RATIO))

void sql_log(P_char ch, char *kind, char *format, ...);
void sql_log_player_login(P_char ch, const char *status);

struct zone_info
{
	int    number;
	string name;
	int    epic_type;
	float  frequency_mod;
	float  zone_freq_mod;
	int    epic_level;
	bool   task_zone;
	bool   quest_zone;
	bool   trophy_zone;
	int    suggested_group_size;
	int    epic_payout;
	int    difficulty;
};

bool get_zone_info(int zone_number, struct zone_info *info);

void sql_get_bind_data(int vnum, int *owner_pid, int *timer);
void sql_update_bind_data(int vnum, int *owner_pid, int *timer);

void sql_ship_sunk(char owner);
void sql_get_sincesunk_frags(char owner, float *frags);
void sql_add_sincesunk_frags(char owner, float frags);

bool sql_pwipe(int code_verify);
bool sql_clear_zone_trophy();
#endif
