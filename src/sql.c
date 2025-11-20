
/************************************************************************
 * sql.c - interface to MySQL database and functions for stats keeping  *
 *                                                                      *
 * Written by: Thima (Xenofon Papadopoulos)                             *
 *                                                                      *
 ************************************************************************/

#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "assocs.h"
#include "prototypes.h"
#include "sql.h"
#include "utils.h"
#include "account.h"
#include "comm.h"
#include "mm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "structs.h"
#include "spells.h"
#include "utils.h"
#include "specializations.h"
#include "epic.h"
#include "timers.h"

extern P_index mob_index;
extern const struct race_names race_names_table[];
extern const struct class_names class_names_table[];
extern const char *specdata[][MAX_SPEC];
extern P_room world;
extern int RUNNING_PORT;
void     get_assoc_name(int, char *);
bool     get_equipment_list(P_char ch, char *buf, int list_only);
extern P_index obj_index;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;

void get_pkill_player_description(P_char ch, char *buffer);

#ifdef __NO_MYSQL__
int initialize_mysql()
{
  return 1;
}
void do_sql(P_char ch, char *argument, int cmd)
{

}
int sql_save_player_core(P_char ch)
{
  return 1;
}
void sql_modify_frags(P_char ch, int gain)
{
}
void sql_insert_item(P_char ch, P_obj obj, char *desc)
{
}

void sql_save_pkill(P_char ch, P_char victim)
{
}
void sql_insert_new_item(P_char ch, P_obj obj)
{
}

void sql_webinfo_toggle(P_char ch)
{
}
void sql_update_level(P_char ch)
{
}
void sql_update_money(P_char ch)
{
}
void sql_update_epics(P_char ch)
{
}
void sql_update_playtime(P_char ch)
{
}
void manual_log(P_char ch)
{
}
void perform_wiki_search(P_char ch, const char *buf)
{
}
int sql_quest_finish(P_char ch, P_char giver, int type, int value)
{
return -1;
}
int sql_quest_trophy(P_char giver)
{
	return -1;
}
int sql_shop_trophy(P_obj obj){
	return -1;
}
int sql_shop_sell(P_char ch, P_obj obj, int value){
  return -1;
}
void sql_world_quest_finished(P_char ch, P_char giver, P_obj obj)
{
}
int sql_world_quest_done_already(P_char ch, int quest_target)
{
return -1;
}
int sql_world_quest_can_do_another(P_char ch)
{
return -1;
}

void sql_connectIP(P_char ch)
{
}
void sql_disconnectIP(P_char ch)
{
}
const char *sql_select_IP_info(P_char ch, char *buf, size_t bufSize, time_t *lastConnect, time_t*lastDisconnect)
{
  buf[0] = 0;
  return buf;
}
int sql_find_racewar_for_ip( char *ip, int *racewar_side )
{
  return -1;
}
bool qry(const char *format, ...) {
        return TRUE;
}
void send_to_char_offline(const char *msg, int pid) {
}
void send_offline_messages(P_char ch) {
}
void log_epic_gain(int pid, int zone_id, int type, int epics)
{
}
void update_zone_db()
{
}
void update_zone_epic_level(int zone_id, int level)
{
}
void show_frag_trophy(P_char ch, P_char who)
{
  send_to_char("Disabled.", ch);
}
void sql_log(P_char ch, char * kind, char * format, ...)
{
}

bool get_zone_info(int zone_number, struct zone_info *info)
{
  return FALSE;
}

string escape_str(const char *str)
{
  return string(str);
}

string get_mud_info(const char *name)
{
  return string();
}

void send_mud_info(const char* name, P_char ch)
{
}

void sql_update_bind_data(int vnum, int *owner_pid, int *timer)
{
}

void sql_get_bind_data(int vnum, int *owner_pid, int *timer)
{
}

bool sql_pwipe( int code_verify )
{
  if( code_verify == 1723699 )
  {
    logit(LOG_DEBUG, "sql_pwipe: &=GlCan't wipe the SQL stuff as SQL database is not loaded." );
  }
  else
  {
    logit(LOG_DEBUG, "sql_pwipe: &=GlSomeone called sql_pwipe with a bad verify code... hrm.." );
  }
  return FALSE;
}
bool sql_clear_zone_trophy()
{
  return FALSE;
}
#else

static void sql_resetConnectTimes(void);

// The global database handler
MYSQL   *DB;

/* Escapes a string. */
char    *mysql_str(const char *str, char *buf)
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

/* Open a connection to the database. The connection will remain open
 * throughout the mud session. */
int initialize_mysql()
{
  /* hack to ensure we're not using the live database when not running on default port */
  char db_name[50];
  snprintf(db_name, 50, DB_NAME);
  
  if( RUNNING_PORT != DFLT_PORT )
  {
    snprintf(db_name, 50, "duris_dev");    
  }
  
  logit(LOG_STATUS, "Initializing MySQL persistent connection to %s.", db_name);
  DB = mysql_init(NULL);
  if (DB == NULL)
  {
    logit(LOG_STATUS, "Error initializing handler.");
    return -1;
  }
    
  DB = mysql_real_connect(DB, DB_HOST, DB_USER, DB_PASSWD, db_name,
                          0, NULL, 0);
  if (DB == NULL)
  {
    logit(LOG_STATUS, "Error connecting to database.");
    return -1;
  }

  logit(LOG_STATUS, "Connection established.");

  sql_resetConnectTimes();

  return 1;
}

/* Handle a query, log possible errors and return results (if available) */
MYSQL_RES *db_query(const char *format, ...)
{
  char     buf[MAX_LOG_LEN + MAX_STRING_LENGTH + 512];
  va_list  args;
  int      ret;

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

  if(!buf[0])
        return NULL;

  if (mysql_real_query(DB, buf, strlen(buf)) != 0)
  {
    logit(LOG_DEBUG, "MySQL: \"%s\" failed: %s", buf, mysql_error(DB));
    return NULL;
  }

  return mysql_use_result(DB);
}


/* Same as above, but won't log failed queries, ie when key restrictions suffice */
MYSQL_RES *db_query_nolog(const char *format, ...)
{
  char     buf[MAX_STRING_LENGTH];
  va_list  args;
  int      ret;

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

  return mysql_use_result(DB);
}

/* Store core player data to the database. We assume that only association
 * names may contain special characters */
int sql_save_player_core(P_char ch)
{
  char     query[MAX_STRING_LENGTH];
  char     assoc_name[MAX_STRING_LENGTH];
  char     assoc_name_sql[MAX_STRING_LENGTH];
  const char *spec_name = "";
  struct char_player_data *p;
  int      val;

  if (IS_MORPH(ch))
    ch = MORPH_ORIG(ch);
  p = &ch->player;
  val = flag2idx(p->m_class);

  if( GET_ASSOC(ch) == NULL )
  {
    assoc_name[0] = '\0';
  }
  else
  {
    snprintf(assoc_name, MAX_STRING_LENGTH, "%s", GET_ASSOC(ch)->get_name().c_str() );
  }
  mysql_str(assoc_name, assoc_name_sql);

  if (IS_SPECIALIZED(ch))
  {
    spec_name = GET_SPEC_NAME(ch->player.m_class, ch->player.spec-1);
  }


  /* Some values might have changed, so we have to UPDATE. The INSERT will only
   * work the first time, since pid is a primary key. */
  db_query_nolog
    ("INSERT INTO players_core (pid, name, race, classname, spec, guild, webinfo_toggle, racewar, level, money, balance, playtime, epics) VALUES( %d, '', '', '', '', '', 0, 0 ,0 ,0,0,0,0)",
     GET_PID(ch));

  snprintf(query, MAX_STRING_LENGTH, "UPDATE players_core SET active = 0 WHERE name = '%s' and pid != %d", p->name, GET_PID(ch));
  db_query(query);

  snprintf(query, MAX_STRING_LENGTH,
          "UPDATE players_core SET name='%s', race = '%s', classname = '%s', "
          "spec = '%s', guild = '%s', webinfo_toggle = %d, "
          "level = %d, racewar=%d, active=1 WHERE pid = %d", p->name,
          race_names_table[p->race].ansi, get_class_name(ch, ch),
          spec_name, assoc_name_sql,
          (IS_SET(ch->specials.act2, PLR2_WEBINFO) ? 1 : 0),
          GET_LEVEL(ch), GET_RACEWAR(ch), GET_PID(ch));

  db_query(query);

  // Update frag leaderboard tables for web statistics
  sql_update_account_character(ch);
  sql_update_frag_leaderboard(ch);

  return 1;
}

/* Save a variable delta. Type can be one of FRAGS, EXP */
void sql_save_progress(int pid, int delta, const char *type)
{
  db_query("INSERT INTO progress VALUES( 0, %d, '%s', NOW(), %d )",
           pid, type, delta);
}

// Retrieves the current highest number of frags and which racewar side has it.
void get_level_cap_info( long *max_frags, int *racewar, int *level, time_t *next_update )
{
  MYSQL_RES *db = NULL;
  MYSQL_ROW row;
  db = db_query( "SELECT most_frags, racewar_leader, level, UNIX_TIMESTAMP(next_update) FROM level_cap" );

  if( (db == NULL) || (( row = mysql_fetch_row(db) ) == NULL) )
  {
    debug( "get_level_cap_info: Database read fail." );
    *max_frags = (long)-1;
    *racewar = RACEWAR_NONE;
    *level = 25;
    *next_update = 0;
    return;
  }
  *max_frags   = (long)(atof( row[0] ) * 100. + .01);
  *racewar     = atoi(row[1]);
  *level       = atoi(row[2]);
  *next_update = atol(row[3]);

  // cycle out until a NULL return
  while( row != NULL )
  {
    row = mysql_fetch_row(db);
  }
  mysql_free_result(db);
}

// Returns the highest level achievable by mortals, limited by racewar side.
int sql_level_cap( int racewar_side )
{
  int  leading_racewar, level_cap;
  MYSQL_RES *db = NULL;
  MYSQL_ROW row;

  db = db_query( "SELECT level, racewar_leader FROM level_cap" );

  if( (db == NULL) || (( row = mysql_fetch_row(db) ) == NULL) )
  {
    debug( "sql_level_cap: Database read fail." );
    return 25;
  }

  level_cap       = atoi(row[0]);
  leading_racewar = atoi(row[1]);

  // cycle out until a NULL return
  while( row != NULL )
  {
    row = mysql_fetch_row(db);
  }
  mysql_free_result(db);

  // Everyone can reach 56 when someone reaches the limit + 40.
  if( level_cap >= MAXLVLMORTAL )
    return MAXLVLMORTAL;
  // 25 is the lower limit.
  if( level_cap <= 25 )
    return 25;
  // Otherwise, we have a 1 level penalty for non-leading racewar sides (on non-circle levels).
  if( (racewar_side != leading_racewar) && (level_cap % 5 != 1) )
    return level_cap - 1;
  else
    return level_cap;
}

//#define CAP_DELAY(old_level) (time(NULL) + SECS_PER_REAL_DAY * (old_level / 10 - 1))
// 1 Day for under lvl 40, 2 days over + random number of hours up to a day..
#define CAP_DELAY(old_level) (time(NULL) + SECS_PER_REAL_DAY * (old_level / 39 + 1) + SECS_PER_REAL_HOUR * number(1,24))

// Checks the number of frags against the current highest and sets the new highest if applicable.
// Adjusted the time inbetween notches from a static 1 day to 1 day for levels 26-29, 2 days for 30-39,
//   3 days for 40-49, and 4 days for 50-56.
void sql_check_level_cap( long max_frags, int racewar )
{
  long old_max_frags;
  int  old_racewar, old_level;
  time_t next_update;
  char query[1024];

  get_level_cap_info( &old_max_frags, &old_racewar, &old_level, &next_update );
  // If we've capped out
  if( old_level >= MAXLVLMORTAL )
  {
    return;
  }
  // If enough time has passed, and level should change, update level if appropriate.
  if( next_update <= time(NULL) )
  {
    // Have enough frags to update level.
    if( old_level < FRAGS_TO_LEVEL(max_frags/100.) )
    {
      snprintf(query, 1024, "UPDATE level_cap SET most_frags = %f, racewar_leader = %d, level = %d, next_update = FROM_UNIXTIME(%ld)",
        max_frags/100., racewar, old_level + 1, CAP_DELAY(old_level) );
      db_query(query);
    }
    else if( max_frags > old_max_frags )
    {
      snprintf(query, 1024, "UPDATE level_cap SET most_frags = %f, racewar_leader = %d", max_frags / 100., racewar );
      db_query(query);
    }
  }
  // Just changing highest frag amount and, possibly, racewar leader.
  else if( max_frags > old_max_frags )
  {
    snprintf(query, 1024, "UPDATE level_cap SET most_frags = %f, racewar_leader = %d", max_frags/100., racewar );
    db_query(query);
  }
}

// Sets the values of level (actual cap) and racewar (the side that is in the lead).
void get_level_cap( int *level, int *racewar )
{
  MYSQL_RES *db = NULL;
  MYSQL_ROW row = NULL;

  db = db_query( "SELECT level, racewar_leader FROM level_cap" );

  if( (db == NULL) || (( row = mysql_fetch_row(db) ) == NULL) )
  {
    debug( "get_level_cap: Database read fail." );
    *level = 25;
    *racewar = RACEWAR_NONE;
  }
  else
  {
    *level   = atoi(row[0]);
    *racewar = atoi(row[1]);
  }

  // cycle out until a NULL return
  while( row != NULL )
  {
    row = mysql_fetch_row(db);
  }
  mysql_free_result(db);
}

/* Save frags delta */
void sql_modify_frags(P_char ch, int gain)
{
  // We don't want IS_TRUSTED(ch) because that can be turned off with toggle fog.
  if( GET_LEVEL(ch) > MAXLVLMORTAL )
  {
    return;
  }
  if (IS_MORPH(ch))
    ch = MORPH_ORIG(ch);
  sql_save_progress(GET_PID(ch), gain, "FRAGS");
  if( gain > 0 )
    sql_check_level_cap( ch->only.pc->frags, GET_RACEWAR(ch) );

  // Update frag leaderboard with new frag count (incremental update for performance)
  // Only update if the character is in the database (pid > 0)
  if( GET_PID(ch) > 0 )
  {
    db_query(
      "UPDATE frag_leaderboard SET total_frags = %d, last_updated = NOW() WHERE pid = %ld AND deleted_at IS NULL",
      ch->only.pc->frags, GET_PID(ch)
    );
  }
}

/*
 * Frag Leaderboard Hybrid System - for web statistics
 * These functions maintain the account_characters and frag_leaderboard tables
 * The MUD continues to use flat files, but web can query the database
 */

/* Helper function to get account name safely */
static const char* get_account_name_safe(P_char ch)
{
  // If character has descriptor with account, return account name
  if (ch->desc && ch->desc->account && ch->desc->account->acct_name)
    return ch->desc->account->acct_name;

  // Character not connected or no account - return "Unknown"
  // This can happen when updating offline characters or during loading
  return "Unknown";
}

/* Update account_characters mapping table */
void sql_update_account_character(P_char ch)
{
  char account_name_sql[MAX_STRING_LENGTH];
  char char_name_sql[MAX_STRING_LENGTH];
  const char *account_name;

  if (!ch || IS_NPC(ch))
    return;

  if (IS_MORPH(ch))
    ch = MORPH_ORIG(ch);

  account_name = get_account_name_safe(ch);

  // Escape strings for SQL safety
  mysql_str(account_name, account_name_sql);
  mysql_str(ch->player.name, char_name_sql);

  // Insert or update account_characters mapping
  // Using INSERT...ON DUPLICATE KEY UPDATE to preserve created_at for existing records
  db_query(
    "INSERT INTO account_characters "
    "(account_name, pid, char_name, created_at, deleted_at) "
    "VALUES('%s', %ld, '%s', NOW(), NULL) "
    "ON DUPLICATE KEY UPDATE "
    "account_name = VALUES(account_name), "
    "char_name = VALUES(char_name), "
    "deleted_at = NULL",
    account_name_sql, GET_PID(ch), char_name_sql
  );
}

/* Update frag_leaderboard table with current character data */
void sql_update_frag_leaderboard(P_char ch)
{
  char account_name_sql[MAX_STRING_LENGTH];
  char char_name_sql[MAX_STRING_LENGTH];
  char race_sql[MAX_STRING_LENGTH];
  char class_sql[MAX_STRING_LENGTH];
  const char *account_name;
  const char *race_name;
  const char *class_name;

  if (!ch || IS_NPC(ch))
    return;

  if (IS_MORPH(ch))
    ch = MORPH_ORIG(ch);

  account_name = get_account_name_safe(ch);
  race_name = race_names_table[ch->player.race].normal;
  class_name = class_names_table[flag2idx(ch->player.m_class)].normal;

  // Escape strings for SQL safety
  mysql_str(account_name, account_name_sql);
  mysql_str(ch->player.name, char_name_sql);
  mysql_str(race_name, race_sql);
  mysql_str(class_name, class_sql);

  // Insert or update frag_leaderboard
  // Using REPLACE to handle both insert and update cases
  db_query(
    "REPLACE INTO frag_leaderboard "
    "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
    "VALUES(%ld, '%s', '%s', %d, %d, '%s', '%s', %d, NULL)",
    GET_PID(ch), account_name_sql, char_name_sql, ch->only.pc->frags,
    GET_RACEWAR(ch), race_sql, class_sql, GET_LEVEL(ch)
  );
}

/* Soft delete a character from the leaderboard tables */
void sql_soft_delete_character(long pid)
{
  if (pid <= 0)
    return;

  // Set deleted_at timestamp to NOW() for this character
  db_query(
    "UPDATE account_characters SET deleted_at = NOW() WHERE pid = %ld AND deleted_at IS NULL",
    pid
  );

  db_query(
    "UPDATE frag_leaderboard SET deleted_at = NOW() WHERE pid = %ld AND deleted_at IS NULL",
    pid
  );
}

/* Save frags delta */
void sql_insert_item(P_char ch, P_obj obj, char *desc)
{

char     query[MAX_STRING_LENGTH];
char     sql_desc[MAX_STRING_LENGTH];
char     sql_short[MAX_STRING_LENGTH];

int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;
mysql_str(desc, sql_desc);
mysql_str( obj->short_description, sql_short);


  db_query_nolog
    ("INSERT INTO items_stats VALUES( null, '%s', '', %d)",
     sql_short, m_virtual);
  snprintf(query, MAX_STRING_LENGTH,
          "UPDATE items_stats SET  obj_stat = '%s', vnum = %d "
          " WHERE short_desc = '%s'",  sql_desc, m_virtual, sql_short);

  db_query(query);

struct zone_data *zone = 0;
zone = &zone_table[world[ch->in_room].zone];

}



void sql_insert_new_item(P_char ch, P_obj obj)
{
char     item_id[MAX_STRING_LENGTH];
int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;
int i = ch->in_room;
P_room   rm = &world[i];
struct zone_data *zone = 0;
zone = &zone_table[world[ch->in_room].zone];

snprintf(item_id, MAX_STRING_LENGTH, "o %s", obj->name);
do_stat(ch,item_id, 555);

}


unsigned long new_pkill_event(P_char ch)
{
  char     room_name_sql[MAX_STRING_LENGTH];
  char     query[MAX_STRING_LENGTH];

  mysql_str(world[ch->in_room].name, room_name_sql);
  snprintf(query, MAX_STRING_LENGTH, "INSERT INTO pkill_event (stamp, room_vnum, room_name) VALUES( NOW(), %d, '%s' )",
           world[ch->in_room].number, room_name_sql);

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

  if( GET_ASSOC(ch) == NULL )
  {
    assoc_name[0] = '\0';
  }
  else
  {
    snprintf(assoc_name, MAX_STRING_LENGTH, "%s", GET_ASSOC(ch)->get_name().c_str() );
  }

  snprintf(buffer, MAX_STRING_LENGTH, "[%2d %s&n] %s &n%s &n(%s&n)",
               GET_LEVEL(ch), get_class_name(ch, ch), GET_NAME(ch), assoc_name, race_names_table[GET_RACE(ch)].ansi);
  
  logit(LOG_DEBUG, "%s", buffer);
}

void store_pkill_info(unsigned long pkill_event, P_char ch, const char *type, int leader, int in_room)
{
  char     buf[MAX_STRING_LENGTH];
  char     equip_sql[MAX_STRING_LENGTH];
  char     player_description_sql[MAX_STRING_LENGTH];
  char     log_sql[MAX_LOG_LEN];

  if( !ch || !IS_PC(ch) )
    return;
  
  if( !GET_PLAYER_LOG(ch) )
  {
    logit(LOG_DEBUG, "Tried to dump player log (%s) in store_pkill_info(), but player log was null!", GET_NAME(ch));
    return;
  }

  get_equipment_list(ch, buf, 1);
  mysql_str(buf, equip_sql);

  get_pkill_player_description(ch, buf);
  mysql_str(buf, player_description_sql);

  mysql_str( GET_PLAYER_LOG(ch)->read(LOG_PUBLIC, MAX_LOG_LEN), log_sql);

  db_query("INSERT INTO pkill_info (event_id, pid, level, pk_type, player_description, equip, log, inroom, leader) "
           "VALUES( %d, %d, %d, '%s', '%s', '%s', '%s', %d ,%d )",
      pkill_event, GET_PID(ch), GET_LEVEL(ch), type, player_description_sql, equip_sql, log_sql, in_room, leader);
}

/* Save racewr pkill information */
void sql_save_pkill(P_char ch, P_char victim)
{
  P_char   tch;
  unsigned long pkill_event;

  // NPCs can't be pkilled.
  if( IS_NPC(victim) )
  {
    return;
  }

  /* If pet is the killer, we blame the owner, if he's around */
  if( IS_NPC(ch) )
  {
    if( ch->following && IS_PC(ch->following)
      && ch->in_room == ch->following->in_room && grouped(ch, ch->following) )
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
  if( !pkill_event )
    return;

  struct group_list *gl;
  int in_room = 0;
  // loop ch's group
  if (ch->group)
  {
    for (struct group_list *gl = ch->group; gl; gl = gl->next)
    {
      if(ch->in_room == gl->ch->in_room)
        in_room = 1;
      else
        in_room = 0;
      if (IS_PC(gl->ch))
      {
        if (ch->group->ch == gl->ch)
          store_pkill_info(pkill_event, gl->ch, "KILLER", 1, in_room);
        else
          store_pkill_info(pkill_event, gl->ch, "KILLER", 0, in_room);
      }
    }
  }
  else if (IS_PC(ch))
  {
    store_pkill_info(pkill_event, ch, "KILLER", 0 ,1);
  }

  if (victim->group)
  {
    // and loop victims group
    for (struct group_list *gl = victim->group; gl; gl = gl->next)
    {
      if (IS_PC(gl->ch))
      {
        bool bIsVict = (gl->ch == victim);
        if(victim->in_room == gl->ch->in_room)
             in_room = 1;
        else
            in_room = 0;


        if (victim->group->ch == gl->ch)
          store_pkill_info(pkill_event, gl->ch, bIsVict ? "VICTIM" :
                                                          "VICTIM-GROUP", 1, in_room);
        else
          store_pkill_info(pkill_event, gl->ch, bIsVict ? "VICTIM" :
                                                          "VICTIM-GROUP", 0, in_room);
      }
    }
  }
  else if (IS_PC(victim))
  {
    store_pkill_info(pkill_event, victim, "VICTIM", 0, 1);
  }
}

/* Save character's preferences about displaying extended info on
   webpage for all to see. */
void sql_webinfo_toggle(P_char ch)
{
  db_query("UPDATE players_core SET webinfo_toggle=%d WHERE pid=%d",
           (IS_SET(ch->specials.act2, PLR2_WEBINFO) ? 1 : 0), GET_PID(ch));
}

/* Update level info */
void sql_update_level(P_char ch)
{
  db_query("UPDATE players_core SET level=%d WHERE pid=%d",
           GET_LEVEL(ch), GET_PID(ch));
}

/* Update money info */
void sql_update_money(P_char ch)
{
  db_query("UPDATE players_core SET money='%d', balance='%d' WHERE pid='%d'",
						GET_MONEY(ch), GET_BALANCE(ch), GET_PID(ch));
}

/* Update playtime info */
void sql_update_playtime(P_char ch)
{
	db_query("UPDATE players_core SET playtime='%d' WHERE pid = '%d'",
					 ch->player.time.played, GET_PID(ch));
}

/* Update player's epics: We want to record their total epics gained not epics unused */
void sql_update_epics(P_char ch)
{
  struct affected_type *paf = get_spell_from_char(ch, TAG_EPICS_GAINED);

  db_query("UPDATE players_core SET epics='%d' WHERE pid='%d'",
	         paf ? paf->modifier : 0, GET_PID(ch));
}

void manual_log(P_char ch)
{

  char     a[256], b[256];
  char     buf[MAX_STRING_LENGTH];
  char     equip_sql[MAX_STRING_LENGTH];
  char     log_sql[MAX_LOG_LEN];
  char     buf2[MAX_LOG_LEN];
  int      space = MAX_LOG_LEN;

  // paranoia check
  if( !ch || !IS_PC(ch) )
    return;

  if( !GET_PLAYER_LOG(ch) )
  {
    logit(LOG_DEBUG, "Tried to dump player log (%s) in manual_log(), but player log was null!", GET_NAME(ch));
    return;
  }

  *buf2 = '\0';

  ITERATE_LOG(ch, LOG_PUBLIC)
  {
    strncat( buf2, LOG_MSG(), space);
    space -= strlen(LOG_MSG());

    if( space <= 0 )
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
  db_query_nolog("INSERT INTO ip_info (pid) VALUES (%d)", GET_PID(ch));
  if (ch->desc)
  {
    // Set racewar side if not an immortal.
    db_query( "UPDATE ip_info SET last_disconnect = NOW(), racewar_side=%d WHERE pid = %d",
      IS_TRUSTED(ch) ? RACEWAR_NONE : GET_RACEWAR(ch), GET_PID(ch) );
  }
}

void sql_connectIP(P_char ch)
{
  // insert will silently fail if the PID is already in the table
  db_query_nolog("INSERT INTO ip_info (pid) VALUES (%d)", GET_PID(ch));
  if (ch->desc)
  {
    db_query("UPDATE ip_info SET last_ip = '%s', last_connect = NOW(), racewar_side = %d WHERE pid = %d",
      ch->desc->host, IS_TRUSTED(ch) ? RACEWAR_NONE : GET_RACEWAR(ch), GET_PID(ch));
  }
}

void sql_world_quest_finished(P_char ch, P_obj reward)
{
  char buf[MAX_STRING_LENGTH];

  int reward_vnum = reward ? ((reward->R_num >= 0) ? obj_index[reward->R_num].virtual_number : 0) : 0;
  char* reward_desc = reward ? mysql_str(reward->short_description, buf) : mysql_str("", buf);

  db_query("INSERT INTO world_quest_accomplished (pid, timestamp, quest_giver, player_name, player_level, quest_target, reward_vnum, reward_desc) VALUES (%d, now(), %d, '%s', %d, %d, %d, '%s')", 
						     GET_PID(ch), ch->only.pc->quest_giver, GET_NAME(ch), GET_LEVEL(ch), ch->only.pc->quest_mob_vnum, reward_vnum, reward_desc );
}

int sql_world_quest_can_do_another(P_char ch)
{
  // This crashed us when paly's horse called this function.
  if (!IS_PC(ch))
    return 0;

  MYSQL_RES *db = 0;
  if(GET_LEVEL(ch) < 50)
	 db = db_query("SELECT count(id) FROM world_quest_accomplished where pid = %d and player_level =%d and TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 0", GET_PID(ch), GET_LEVEL(ch) );
  else
	 db = db_query("SELECT count(id) FROM world_quest_accomplished where pid = %d and TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 0", GET_PID(ch));


  int returning_value = 0;
  if(GET_LEVEL(ch) <= 30)
    returning_value = get_property("world.quest.max.level.30.andUnder", 6.000);
  else if(GET_LEVEL(ch) <= 40)
    returning_value = get_property("world.quest.max.level.40.andUnder", 6.000);
  else if(GET_LEVEL(ch) <= 50) 
    returning_value = get_property("world.quest.max.level.50.andUnder", 6.000);
  else if(GET_LEVEL(ch) <= 55) 
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

  MYSQL_RES *db = db_query("SELECT count(id) FROM world_quest_accomplished where quest_target = %d and pid = %d", quest_target, GET_PID(ch));
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

const char *sql_select_IP_info(P_char ch, char *buf, size_t bufSize, time_t *lastConnect, time_t*lastDisconnect)
{
  time_t now = 0;
  buf[0] = '\0';

  MYSQL_RES *db = db_query("SELECT last_ip, UNIX_TIMESTAMP(last_connect), UNIX_TIMESTAMP(last_disconnect), UNIX_TIMESTAMP() "
                           "FROM ip_info WHERE pid = %d", GET_PID(ch));
  if (db)
  {
    MYSQL_ROW row = mysql_fetch_row(db);

    if (NULL != row)
    {
      strncpy(buf, row[0] ? row[0] : "", bufSize - 2);
      buf[bufSize-1] = '\0';
      now = strtoul(row[3], NULL, 10);
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
      while ((row = mysql_fetch_row(db)));
    }
    mysql_free_result(db);
  }
  return buf;
}

// Returns the time needed *in seconds) to timeout the racewar side associated with an ip.
// Or 0 if no character has been on within an hour.
int sql_find_racewar_for_ip( char *ip, int *racewar_side )
{
  MYSQL_RES *db;
  MYSQL_ROW row;
  time_t last_connect, last_disconnect, hour_ago;

  db = db_query( "SELECT UNIX_TIMESTAMP(last_connect), UNIX_TIMESTAMP(last_disconnect), UNIX_TIMESTAMP(), racewar_side"
    " from ip_info WHERE last_ip = \"%s\" ORDER BY last_connect DESC LIMIT 1", ip );

  if( db && (( row = mysql_fetch_row(db) ) != NULL) )
  {
    // Arih: fix NULL pointer crash when last_disconnect is NULL in ip_info table - 20251103
    // UNIX_TIMESTAMP() returns NULL for NULL datetime values, causing strtoul to segfault
    last_connect = row[0] ? strtoul(row[0], NULL, 10) : 0;
    last_disconnect = row[1] ? strtoul(row[1], NULL, 10) : 0;
    hour_ago = row[2] ? strtoul(row[2], NULL, 10) - 60 * 60 : 0;
    *racewar_side = row[3] ? atoi(row[3]) : 0;

    // If they've been offline for an hour or more, return a 0 timer.
    if( last_disconnect > last_connect && last_disconnect <= hour_ago )
    {
      racewar_side = RACEWAR_NONE;
      while( row != NULL )
        row = mysql_fetch_row(db);
      return 0;
    }

    while( row != NULL )
      row = mysql_fetch_row(db);

		mysql_free_result(db);

    // Return an hour if they're still online, or time delta to an hour offline.
    return (last_disconnect < last_connect) ? 60 * 60 : last_disconnect - hour_ago;
  }

  if( db )
    mysql_free_result(db);
  return RACEWAR_NONE;
}

void perform_wiki_search(P_char ch, const char *query)
{

  char     buf[MAX_STRING_LENGTH];
  char     buf2[MAX_STRING_LENGTH];
  char     buf3[MAX_STRING_LENGTH];
  char     escaped_query[MAX_STRING_LENGTH * 2 + 1];  // SECURITY: Buffer for escaped query (MySQL needs 2x+1 size)
  buf[0] = '\0';
  buf2[0] = '\0';
  buf3[0] = '\0';
  MYSQL_ROW row;
  MYSQL_ROW row2;

  // SECURITY FIX: Sanitize user input to prevent SQL injection
  // Escape the query string using MySQL's built-in escape function
  mysql_real_escape_string(DB, escaped_query, query, strlen(query));

/*
MYSQL_RES *db  = db_query("SELECT UPPER(si_title) , old_id, REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(old_text,'<pre>',''),'</pre>',''), ']]', ''),'[[' ,'' ), '::', ':'), '<br>', '') FROM wikki_searchindex, wikki_text where old_id =( SELECT max(rev_text_id) FROM wikki_revision w where rev_page =( select si_page from wikki_searchindex where LOWER(si_title)  like LOWER('%s') limit 1)) and si_title like LOWER('%s') limit 1", query, query);
*/


  MYSQL_RES *db = db_query("SELECT REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(old_text,'<pre>',''),'</pre>',''), ']]', ''),'[[' ,'' ), '::', ':'), '<br>', ''), '\'\'', '') , '==', '') FROM `wikki_text`  WHERE old_id = (SELECT rev_text_id FROM `wikki_page`,`wikki_revision`  WHERE (page_id=rev_page) AND rev_id = (SELECT page_latest FROM `wikki_page`  WHERE page_id = (SELECT page_id  FROM `wikki_page`  WHERE page_namespace = '0' AND LOWER(page_title) = REPLACE(LOWER('%s'), ' ', '_')  LIMIT 1)  LIMIT 1)  LIMIT 1)  LIMIT 1", escaped_query);
  if (db)
  {
    row = mysql_fetch_row(db);
    if (NULL != row)
    {
     snprintf(buf, MAX_STRING_LENGTH, "\t&+W========| &+m %s &+W |========&n\n%s" ,escaped_query, row[0]);
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

bool qry(const char *format, ...)
{
  char     buf[MAX_STRING_LENGTH];
  va_list  args;
  int      ret;

  if( !DB )
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

void send_to_pid_offline(const char *msg, int pid) {
	char buff[MAX_STRING_LENGTH];
	mysql_real_escape_string(DB, buff, msg, strlen(msg));
	qry("INSERT INTO offline_messages (date, pid, message) VALUES (now(), '%d', '%s')", pid, buff);
}

void send_offline_messages(P_char ch)
{
	if( !ch ) return;

	if( !qry("SELECT id, message FROM offline_messages WHERE pid = '%d' ORDER BY date ASC", GET_PID(ch)) ) {
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if( mysql_num_rows(res) < 1 ) {
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
	int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;
  
  int pid = ( IS_PC(ch) ? GET_PID(ch) : 0 );
	
  qry("INSERT INTO shop_trophy (item, value, seller, timestamp) VALUES ('%d', '%d', %d, now())", m_virtual, value, pid);
	
  return 1;
}

int sql_shop_trophy(P_obj obj)
{

if(!obj)
 return 0;

//mined ore doesnt devaule
if(strstr(obj->name, "_ore_"))
return 0;

int objvir = OBJ_VNUM(obj);
if((objvir >= 400000) && (objvir < 400202))
return 0;


int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;


  MYSQL_RES *db = db_query("SELECT count(id) FROM shop_trophy where item = %d and  TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 7",
                            m_virtual);

  int returning_value = 0;
  if (db)
  {
    MYSQL_ROW row = mysql_fetch_row(db);
    if (NULL != row)
    {
	    returning_value = atoi(row[0]);
    }
    else
   	   returning_value =  0;
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
	qry("INSERT INTO quest_trophy (mob_vnum, pid, type, reward_value, timestamp) VALUES ('%d', '%d', %d, %d ,now())", 
		m_virtual, GET_PID(ch) ,type, value );
	return 1;
}

int sql_quest_trophy(P_char giver)
{
 int m_virtual = GET_VNUM(giver);

  MYSQL_RES *db = db_query("SELECT count(id) FROM quest_trophy where mob_vnum = %d and  TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 14", m_virtual);
  int returning_value = 0;
  if (db)
  {
    MYSQL_ROW row = mysql_fetch_row(db);
    if (NULL != row)
    {
	    returning_value = atoi(row[0]);
    }
    else
   	   returning_value =  0;
    while ((row = mysql_fetch_row(db)))
      ;
    mysql_free_result(db);
  }
return returning_value;
}

void log_epic_gain(int pid, int type, int type_id, int epics)
{
	qry("INSERT INTO epic_gain (pid, time, type, type_id, epics) values ('%d', now(), '%d', '%d', '%d')", pid, type, type_id, epics);
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

  char     first[MAX_INPUT_LENGTH];
  char     second[MAX_INPUT_LENGTH];
  char     third[MAX_INPUT_LENGTH];
  char     fourth[MAX_INPUT_LENGTH];
  char    *rest;
  char     buf[MAX_STRING_LENGTH];
  int      limited_result = 0;
  int      prep_statement;
  int      num_fields, num_rows, i;

  char     result[MAX_STRING_LENGTH* 10];
  char     tmp[MAX_STRING_LENGTH];

  MYSQL_RES *db = 0;
  MYSQL_ROW row;

  if( !IS_TRUSTED(ch) )
  {
    send_to_char("A mere mortal can't do this!\r\n", ch);
    return;
  }

  if( !*argument )
  {
	  send_to_char("Sql is a command to let us gods, access database easy, it suport all kind of queries.\n"
      "&=LY-=Make sure you understand what you do else this command is most likly not designed for you=-&n\n", ch);
	  send_to_char("&+WSyntax: 'sql < query | prep <list | #> >'&n\n", ch);
	  return;
  }

  wizlog(56, "SQL (%s): '%s'", GET_TRUE_NAME(ch), argument);
  logit(LOG_WIZ, "SQL (%s): '%s'" , GET_TRUE_NAME(ch), argument);
  sql_log(ch, WIZLOG, "SQL: '%s'" , argument);

  rest = one_argument( argument, first );
  rest = one_argument( rest, second );

  if( strstr(first, "prep") )
  {
    if( strstr(second, "list") )
    {
      do_sql(ch, "SELECT id, description FROM prepstatement_duris_sql", 0);
    }
    if( !is_number(second) )
    {
//      send_to_char("\n\r&+YTo add prep queries just check how the table 'prepstatement_duris_sql' (&+Wsql desc prepstatement_duris_sql&+Y) and add!&n\n\r", ch);
      send_to_char("&+YSyntax:&n sql prep < list | number > [ desc | sql | run | delete ] [ description | sql code ]\n\r", ch );
      return;
    }
    else
    {
      prep_statement = (int) atoi(second);
      rest = one_argument( rest, third );
      rest = skip_spaces( rest );
      if( !*third )
      {
        snprintf(third, MAX_INPUT_LENGTH, "SELECT * FROM prepstatement_duris_sql WHERE id=%d", prep_statement );
        do_sql( ch, third, cmd );
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
      if( strstr(third, "run" ) )
      {
        db = db_query("SELECT sql_code FROM prepstatement_duris_sql WHERE id=%d", prep_statement);
        if( db )
        {
          MYSQL_ROW row = mysql_fetch_row(db);

          if( row != NULL )
          {
            snprintf(tmp, MAX_STRING_LENGTH, "%s", row[0]);
          }
          else
          {
            send_to_char("That prepped statement does not exist.\n\r", ch );
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
          send_to_char( "Error no db created.\n\r", ch );
        }
        return;
      }
      if( strstr(third, "desc" ) )
      {
        // SECURITY FIX: Escape user input to prevent SQL injection
        char escaped_desc[MAX_STRING_LENGTH * 2 + 1];
        mysql_real_escape_string(DB, escaped_desc, rest, strlen(rest));
        snprintf(buf, MAX_STRING_LENGTH, "UPDATE prepstatement_duris_sql SET description = '%s' WHERE id='%d'", escaped_desc, prep_statement );
        do_sql( ch, buf, 0);
        return;
      }
      if( strstr(third, "sql" ) )
      {
        // SECURITY FIX: Escape user input to prevent SQL injection
        char escaped_sql[MAX_STRING_LENGTH * 2 + 1];
        mysql_real_escape_string(DB, escaped_sql, rest, strlen(rest));
        if( qry("UPDATE prepstatement_duris_sql SET sql_code = '%s' WHERE id='%d'", escaped_sql, prep_statement ) )
        {
          snprintf(buf, MAX_STRING_LENGTH, "Row %d sql_code set to '%s'.\n\r", prep_statement, rest );
          send_to_char(buf, ch );
        }
        return;
      }
      if( strstr(third, "delete" ) )
      {
        if( qry("DELETE FROM prepstatement_duris_sql WHERE id=%d", prep_statement ) )
        {
          snprintf(buf, MAX_STRING_LENGTH, "Row %d deleted.\n\r", prep_statement);
          send_to_char(buf, ch );
        }
        return;
      }
    }
  }

  MYSQL_FIELD *fields;
  result[0] = '\0';

  if( mysql_real_query(DB, argument, strlen(argument)) )
  {
    snprintf(result, MAX_STRING_LENGTH, "%s", mysql_error(DB));
    logit(LOG_DEBUG, "MySQL error(sql command): %s", mysql_error(DB));
    send_to_char(result, ch);
    return;
  }
  db = mysql_use_result(DB);
  if( db )
  {
    num_fields = mysql_num_fields(db);

    fields = mysql_fetch_fields(db);
    for( i = 0; i < num_fields; i++ )
    {
      snprintf(tmp, MAX_STRING_LENGTH, " | %-15s&n ", fields[i].name);
      strcat(result, tmp);
    }
    strcat(result, " |\n\n");

    int maxsize = 100;
    while ((row = mysql_fetch_row(db)))
    {
      maxsize--;
      if( maxsize == 0 )
      {
        while( (row = mysql_fetch_row(db)) );
        limited_result = 1;
        break;
      }

      for( i = 0; i < num_fields; i++ )
      {
        snprintf(tmp, MAX_STRING_LENGTH, " | %-15s&n ", row[i]);
        strcat(result, tmp);
      }
      strcat(result, " |\n\n");
    }
    send_to_char(result, ch);
    if( limited_result )
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

    if( !qry("SELECT id FROM zones WHERE number = '%d'", number) )
    {
      logit(LOG_DEBUG, "update_zone_db(): qry failed");
      return;
    }
  
    char name_buff[MAX_STRING_LENGTH];
    mysql_real_escape_string(DB, name_buff, zone_table[z].name, strlen(zone_table[z].name));
  
    MYSQL_RES *res = mysql_store_result(DB);
    if( mysql_num_rows(res) > 0 )
    {
      qry("UPDATE zones SET name = '%s' WHERE number = '%d'", name_buff, number);
    }
    else
    {
      qry("INSERT INTO zones (number, name) VALUES ('%d', '%s')", number, name_buff);
    }
    mysql_free_result(res);
  }

  for( P_obj o = object_list; o; o = o->next )
  {
    int epic_type = 0;
    
    switch( obj_index[o->R_num].virtual_number )
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

    if( !epic_type )
      continue;

    int zone_id = obj_zone_id(o); 

    if( zone_id >= 0 )
    {
      qry("UPDATE zones SET epic_type = '%d' WHERE number = '%d'", epic_type, zone_table[zone_id].number);
    }

  }

}

void update_zone_epic_level(int zone_number, int level)
{
  qry("UPDATE zones SET epic_level = '%d' WHERE number = '%d'", level, zone_number);
}

void show_frag_trophy(P_char ch, P_char who)
{

  if( !IS_PC(who) )
    return;

  if( !qry("select players_core.name, count(*) as cnt from epic_gain, players_core where epic_gain.type_id = players_core.pid and epic_gain.pid = %d and type = 1 group by type_id order by name asc", who->only.pc->pid) )
  {
    logit(LOG_DEBUG, "show_frag_trophy(): query failed.");
    return;
  }
  
  MYSQL_RES *res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
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
    send_to_char( buff, ch);
  }
  
  mysql_free_result(res);
}

void sql_log(P_char ch, char * kind, char * format, ...)
{
  static char buff[MAX_STRING_LENGTH];
	buff[0] = '\0';

  if( !ch )
  {
    debug("sql_log called for non-existent ch!");
    return;
  }

  if(!IS_PC(ch))
  {
    debug("sql_log called in sql.c for mobile ch - %s - Vnum %d", GET_NAME(ch), GET_VNUM(ch));
    debug("sql_log kind '%s', format '%s'", kind, format );
    return;
  }

  va_list  args;
  int      ret;

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

	if( ch->desc && ch->desc->host )
	{
		snprintf(ip_buff, 50, "%s", ch->desc->host);
	}

  snprintf(buff, MAX_STRING_LENGTH, "INSERT INTO log_entries (date, kind, ip_address, pid, player_name, zone_number, room_vnum, message) VALUES " \
      "(now(), '%s', '%s', %d, '%s', %d, %d, '%s')", kind, ip_buff, GET_PID(ch), GET_NAME(ch), zone_table[world[ch->in_room].zone].number, world[ch->in_room].number, message_buff);

	qry(buff);
}

bool get_zone_info(int zone_number, struct zone_info *info)
{
  if( !info )
  {
    return FALSE;
  }

  if( !qry("SELECT number, name, epic_type, frequency_mod, zone_freq_mod, epic_level, task_zone, quest_zone, trophy_zone, suggested_group_size, epic_payout, difficulty FROM zones WHERE number = %d", zone_number) )
  {
    return FALSE;
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return FALSE;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);
  
  info->number = atoi(row[0]);
  info->name = string(row[1]);
  info->epic_type = atoi(row[2]);
  info->frequency_mod = atof(row[3]);
  info->zone_freq_mod = atof(row[4]);
  info->epic_level = atoi(row[5]);
  info->task_zone = (bool) atoi(row[6]);
  info->quest_zone = (bool) atoi(row[7]);
  info->trophy_zone = (bool) atoi(row[8]);
  info->suggested_group_size = atoi(row[9]);
  info->epic_payout = atoi(row[10]);
  info->difficulty = atoi(row[11]);
  
  mysql_free_result(res);
  return TRUE;
}

string get_mud_info(const char *name)
{
  if( !qry("SELECT content FROM mud_info WHERE name = '%s'", name) )
  {
    logit(LOG_DEBUG, "get_mud_info(): failed to read mud_info '%s' from database", name);
    return string();
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) > 0 )
  {
    MYSQL_ROW row = mysql_fetch_row(res);
    string ret_str(row[0]);
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

void send_mud_info(const char *name, P_char ch)
{
  send_to_char(get_mud_info(name).c_str(), ch, LOG_NONE);    
}

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
    //logit(LOG_DEBUG, "sql_get_bind_data(): Cannot find artifact entry, using default values.");
    *owner_pid = 0;
    *timer = 0;
    mysql_free_result(res);
    return;
  }
  else
  {
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row != NULL)
    {
      *owner_pid = atoi(row[1]);
      *timer = atoi(row[2]);
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
  if( !qry("UPDATE zones SET alignment=0 WHERE epic_type > 0") )
  {
    debug( "sql_clear_zone_trophy(): Failed sql UPDATE.. :(" );
    return FALSE;
  }

  return TRUE;
}

bool sql_pwipe( int code_verify )
{
  logit(LOG_DEBUG, "sql_pwipe: STARTED!" );
  if( code_verify == 1723699 )
  {
    logit(LOG_DEBUG, "sql_pwipe: Clearing zone alignments, trophy and touches... .. .");
    send_to_all( "Clearing zone alignments, trophy and touches... .. ." );
    if( sql_clear_zone_trophy()
      && qry("DELETE FROM zone_trophy")
      && qry("DELETE FROM zone_touches") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n" );
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing tower ownership... .. ." );
    send_to_all( "Clearing tower ownership... .. ." );
    if( qry("UPDATE outposts SET owner_id='0', level='8', walls='1', archers='0', hitpoints='300000', territory='0',"
      " portal_room='0', resources='0', applied_resources='0', golems='0', meurtriere='0', scouts='0'") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n" );
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing nexus stone data... .. ." );
    send_to_all( "Clearing nexus stone data... .. ." );
    if( qry("UPDATE nexus_stones SET align='0', last_touched_at='0'") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing prestige lists... .. ." );
    send_to_all( "Clearing prestige lists... .. ." );
    if( qry("DELETE FROM associations") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing alliances... .. ." );
    send_to_all( "Clearing alliances... .. ." );
    if( qry("DELETE FROM alliances") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing artifact bind data... .. ." );
    send_to_all( "Clearing artifact bind data... .. ." );
    if( qry("DELETE FROM artifact_bind") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing auction data... .. ." );
    send_to_all( "Clearing auction data... .. ." );
    if( qry("DELETE FROM auction_bid_history")
      && qry("DELETE FROM auction_item_pickups")
      && qry("DELETE FROM auction_money_pickups")
      && qry("DELETE FROM auctions") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing boon data... .. ." );
    send_to_all( "Clearing boon data... .. ." );
    if( qry("DELETE FROM boons_progress")
      && qry("DELETE FROM boons") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing ctf data... .. ." );
    send_to_all( "Clearing ctf data... .. ." );
    if( qry("DELETE FROM ctf_data") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing frag data and epic bonus data... .. ." );
    send_to_all( "Clearing frag data and epic bonus data... .. ." );
    if( qry("DELETE FROM epic_bonus")
      && qry("DELETE FROM epic_gain")
      && qry("DELETE FROM progress") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing guild data... .. ." );
    send_to_all( "Clearing guild data... .. ." );
    if( qry("DELETE FROM guild_transactions")
      && qry("DELETE FROM guildhall_rooms")
      && qry("DELETE FROM guildhalls") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing ip info... .. ." );
    send_to_all( "Clearing ip info... .. ." );
    if( qry("DELETE FROM ip_info") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing log entries... .. ." );
    send_to_all( "Clearing log entries... .. ." );
    if( qry("DELETE FROM log_entries") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing offline messages... .. ." );
    send_to_all( "Clearing offline messages... .. ." );
    if( qry("DELETE FROM offline_messages") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing cargo data... .. ." );
    send_to_all( "Clearing cargo data... .. ." );
    if( qry("DELETE FROM ship_cargo_market_mods")
      && qry("DELETE FROM ship_cargo_prices") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing timers... .. ." );
    send_to_all( "Clearing timers... .. ." );
    if( qry("UPDATE timers SET date='0'") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing shop data... .. ." );
    send_to_all( "Clearing shop data... .. ." );
    if( qry("DELETE FROM shop_trophy") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing completed quest data... .. ." );
    send_to_all( "Clearing completed quest data... .. ." );
    if( qry("DELETE FROM world_quest_accomplished") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Clearing locker grant list data... .. ." );
    send_to_all( "Clearing locker grant list data... .. ." );
    if( qry("DELETE FROM locker_access") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Deactivating players_core data... .. ." );
    send_to_all( "Deactivating players_core data... .. ." );
    if( qry("UPDATE players_core SET active = 0") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    logit(LOG_DEBUG, "sql_pwipe: Resetting level_cap data... .. ." );
    send_to_all( "Resetting level_cap data... .. ." );
    // Level 25 is base level for no frags.
    if( qry("UPDATE level_cap SET most_frags=0, racewar_leader=0, level=25, next_update=NOW()") )
    {
      logit(LOG_DEBUG, "  success!" );
      send_to_all( "  success!\n" );
    }
    else
    {
      logit(LOG_DEBUG, "        failure!");
      send_to_all( "        failure!\n");
      return FALSE;
    }
    return TRUE;
  }
  else
  {
    logit(LOG_DEBUG, "sql_pwipe: Someone called sql_pwipe with a bad verify code... hrm.." );
    return FALSE;
  }
  logit(LOG_DEBUG, "sql_pwipe: COMPLETED!" );
  send_to_all( "WIPE COMPLETED!" );
  sleep(1);
}
#endif
