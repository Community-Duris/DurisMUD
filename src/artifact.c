/*
   ***************************************************************************
   *  File: artifact.c                                         Part of Duris *
   *  Usage: routines for artifact-tracking system                           *
   *  This is a new file for artifacts using the mySQL database              *
   *  For old code, see artifact_old.c                                       *
   *  Copyright ??? - Duris Systems Ltd.                                     *
   ***************************************************************************
 */


#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "comm.h"
#include "db.h"
#include "files.h"
#include "mm.h"
#include "necromancy.h"
#include "prototypes.h"
#include "spells.h"
#include "sql.h"
#include "structs.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "vnum.obj.h"

// Artifact types.
#define ARTIFACT_MAIN   1
#define ARTIFACT_UNIQUE 2
#define ARTIFACT_IOUN   3

// Artifact locations.  Note: These values MUST be in the same order as the DB enum type locType.
#define ARTIFACT_NOTINGAME 1
#define ARTIFACT_ON_NPC    2
#define ARTIFACT_ON_PC     3
#define ARTIFACT_ONGROUND  4
#define ARTIFACT_ONCORPSE  5    // PC corpse is implied here.

// Externals
extern P_room world;
extern const int top_of_world;
extern P_index obj_index;
extern P_index mob_index;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_obj object_list;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;

struct arti_list
{
  int        pid;
  arti_data *artis;
  arti_list *next;
};

struct bind_data
{
  int        vnum;
  int        owner_pid;
  long       timer;
  bind_data *next;
};

// Internal globals
bool updateArtis = TRUE;

// Function delcarations
void list_artifacts_sql( P_char ch, int type, bool Godlist, bool allArtis );
void arti_clear_sql( P_char ch, char *arg );
void arti_files_to_sql( P_char ch, char *arg );
void arti_hunt_sql( P_char ch, char *arg );
void arti_player_sql( P_char ch, char *arg );
void arti_poof_sql( P_char ch, char *arg );
void arti_remove_sql( int vnum, bool mortalToo );
void arti_reset_sql( P_char ch, char *arg );
void arti_swap_sql( P_char ch, char *arg );
void arti_timer_sql( P_char ch, char *arg );
P_char load_dummy_char( char *name );
void nuke_eq( P_char ch );

/* This is an example of what the current artifacts table looks like. - 2/23/2015
+------+-------+-----------+----------+---------------------+------+---------------------+
| vnum | owned | locType   | location | timer               | type | lastUpdate          |
+------+-------+-----------+----------+---------------------+------+---------------------+
|  900 | N     | OnNPC     |    81454 | 2015-02-28 02:03:05 |    3 | 2015-06-29 03:28:46 |
|  901 | N     | NotInGame |        0 | 0000-00-00 00:00:00 |    3 | 2015-06-29 03:28:46 |
|  902 | Y     | OnPC      |    26636 | 2015-02-28 02:03:05 |    3 | 2015-06-29 03:28:46 |
|  903 | Y     | OnGround  |     1200 | 2015-02-28 02:03:05 |    3 | 2015-06-29 03:28:46 |
|  904 | Y     | OnCorpse  |    26636 | 2015-02-28 02:03:05 |    3 | 2015-06-29 03:28:46 |
+------+-------+-----------+----------+---------------------+------+---------------------+
          vnum      = vnum of the artifact
          owned     = 'Y' -> artifat is owned, 'N' artifact has yet to be acquired since last poof.
                      If the artifact is owned, then it's timer is set and ticking (sorta).
          locType   = enum('NotInGame', 'OnNPC', 'OnPC', 'OnGround', 'OnCorpse')  (OnCorpse -> PC Corpse)
                    = ARTIFACT_ {NOTINGAME | ON_NPC | ON_PC | ONGROUND | ONCORPSE}
          location  = PID of PC / vnum of NPC / vnum of room.
          timer     = when the arti is due to poof.
          type      = full artifact -> ARTIFACT_MAIN, unique -> ARTIFACT_UNIQUE, ioun -> ARTIFACT_IOUN
          lastUpdate= last time this entry was updated.
*/

// This function handles the input and routes to the correct function.
//   Just sends a list of possible arguments to ch if arg is not valid input.
void do_artifact_sql( P_char ch, char *arg, int cmd )
{
  char  arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  char *rest;
  bool  allArtis, Godlist;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  arg = one_argument( arg, arg1 );
  rest = one_argument( arg, arg2 );
  rest = one_argument( rest, arg3 );
  arg = skip_spaces(arg);

  // all -> show even the artis not in game.
  if( IS_TRUSTED(ch) && (( *arg3 && is_abbrev(arg3, "all") ) || ( *arg2 && is_abbrev(arg2, "all") )) )
  {
    allArtis = TRUE;
  }
  else
  {
    allArtis = FALSE;
  }

  if( !IS_TRUSTED(ch) || (( *arg3 && is_abbrev(arg3, "mortal") ) || ( *arg2 && is_abbrev(arg2, "mortal") )) )
  {
    Godlist = FALSE;
  }
  else
  {
    Godlist = TRUE;
  }

  if( is_abbrev(arg1, "list") )
  {
    list_artifacts_sql( ch, ARTIFACT_MAIN, Godlist, allArtis );
    return;
  }

  if( is_abbrev(arg1, "unique") )
  {
    list_artifacts_sql( ch, ARTIFACT_UNIQUE, Godlist, allArtis );
    return;
  }

  if( is_abbrev(arg1, "ioun") )
  {
    list_artifacts_sql( ch, ARTIFACT_IOUN, Godlist, allArtis );
    return;
  }

  if( GET_LEVEL(ch) < FORGER || IS_NPC(ch) )
  {
    send_to_char( "Valid arguments are list, unique or ioun.\n\r", ch );
    return;
  }

  // At this point, arg holds all the arguments but arg1.
  if( is_abbrev(arg1, "player") )
  {
    arti_player_sql( ch, arg );
    return;
  }

  if( is_abbrev(arg1, "poof") )
  {
    arti_poof_sql( ch, arg );
    return;
  }

  if( is_abbrev(arg1, "swap") )
  {
    arti_swap_sql( ch, arg );
    return;
  }

  if( is_abbrev(arg1, "timer") )
  {
    arti_timer_sql( ch, arg );
    return;
  }

  if( is_abbrev(arg1, "hunt") )
  {
    arti_hunt_sql( ch, arg );
    return;
  }

  if( is_abbrev(arg1, "clear") )
  {
    arti_clear_sql( ch, arg );
    return;
  }

  if( is_abbrev(arg1, "files") )
  {
    arti_files_to_sql( ch, arg );
    return;
  }

  if( is_abbrev(arg1, "reset") )
  {
    arti_reset_sql( ch, arg );
    return;
  }

  send_to_char( "Valid arguments are list, unique, ioun, swap, poof, timer, hunt, clear, or files.\n\r", ch );
  send_to_char( "Valid sub-arguments for list, unique, ioun are [mortal] - shows mortal list and [all] shows un-owned artis.\n\r", ch );
}

// This function displays either the Godlist or mortal list of artifacts of type type.
//   The type is either ARTIFACT_MAIN, ARTIFACT_UNIQUE, or ARTIFACT_IOUN.
//   Possible edit: Change select ... lastUpdate -> UNIXTIME_STAMP(lastUpdate), then use ctime(row[5]).
//     This will make the last update time look the same as when you type 'time' in game.
void list_artifacts_sql( P_char ch, int type, bool Godlist, bool allArtis )
{
  char       buf[MAX_STRING_LENGTH];
  char      *locName, locNameBuf[MAX_STRING_LENGTH], locNameBuf2[MAX_STRING_LENGTH];
  char       timer[MAX_STRING_LENGTH];
  int        vnum, ownerID, locType, totalTime, days, hours, minutes, articount[MAX_RACEWAR+1];
  bool       negTime, owned, shownData;
  P_obj      obj;
  P_char     owner;
  MYSQL_RES *res;
  MYSQL_ROW  row;

  memset(articount, 0, sizeof(articount));

  if( type != ARTIFACT_MAIN && type != ARTIFACT_UNIQUE && type != ARTIFACT_IOUN )
  {
    send_to_char( "Invalid artifact type.\n\r", ch );
    debug( "list_artifacts_sql: Invalid artifact type: %d.", type );
    return;
  }

  if( Godlist )
  {
    sprintf(buf, "&+YOwner                  Time      Last Update           %s\r\n\r\n",
      type == ARTIFACT_MAIN ? "Artifact" : type == ARTIFACT_UNIQUE ? "Unique" :
      type == ARTIFACT_IOUN ? "Ioun" : "Unknown Type" );
    send_to_char( buf, ch );

    // locType is an enum type, so to get the values not the names, we want locType+0.
    qry("SELECT vnum, locType+0, location, owned, UNIX_TIMESTAMP(timer), lastUpdate FROM artifacts WHERE type=%d", type );
  }
  else
  {
    sprintf(buf, "&+YOwner               %s\r\n\r\n", type == ARTIFACT_MAIN ? "Artifact" :
      type == ARTIFACT_UNIQUE ? "Unique" : type == ARTIFACT_IOUN ? "Ioun" : "Unknown Type" );
    send_to_char( buf, ch );

    qry("SELECT vnum, locType+0, location, owned FROM artifacts_mortal WHERE type=%d", type );
  }

  res = mysql_store_result(DB);
  buf[0] = '\0';

  shownData = FALSE;
  if( res )
  {
    if( mysql_num_rows(res) < 1 )
    {
      mysql_free_result(res);
      send_to_char( "No artifacts found.\n\r", ch );
      return;
    }

    while( (row = mysql_fetch_row(res)) )
    {
      vnum    = atoi(row[0]);
      locType = atoi(row[1]);
      ownerID = atoi(row[2]);

      owned = FALSE;
      // Row[3] is owned
      if( !strcmp(row[3], "Y") )
      {
        owned = TRUE;
      }

      obj = read_object( vnum, VIRTUAL );
      if( !obj || !IS_ARTIFACT(obj) )
      {
        debug("list_artifacts_sql: Non artifact on arti list: '%s' %d.", (obj == NULL) ? "NULL" : obj->short_description, vnum );
        // Pull obj if it loaded.
        if( obj )
        {
          // FALSE because we have a MySQL result open.
          extract_obj( obj, FALSE );
        }
        continue;
      }

      if( !allArtis && !owned )
      {
        extract_obj( obj, FALSE );
        continue;
      }

      owner = NULL;

      // Just making sure...
      if( !Godlist && !(locType == ARTIFACT_ON_PC || locType == ARTIFACT_ONCORPSE) )
      {
        debug( "list_artifacts_sql: Bad location (%d) on artifacts_mortal arti %d.",
          locType, vnum );
        extract_obj( obj, FALSE );
        continue;
      }
      switch( locType )
      {
        case ARTIFACT_NOTINGAME:
          locName = "&+RNotInGame&n";
          break;
        case ARTIFACT_ON_NPC:
          owner = read_mobile( ownerID, VIRTUAL );
          if( owner )
          {
            sprintf( locNameBuf, "%s", J_NAME(owner) );
            locName = locNameBuf;
            extract_char( owner );
            owner = NULL;
          }
          else
          {
            locName = "&+RBadMobVnum&n";
          }
          break;
        case ARTIFACT_ON_PC:
          locName = get_player_name_from_pid(ownerID);
          owner = load_dummy_char( locName );
          break;
        case ARTIFACT_ONGROUND:
          locName = world[real_room0(ownerID)].name;
          // Put room title on a seperate line with vnum
          sprintf( locNameBuf, "%s (Room #%d)\n%-20s", locName, ownerID, " " );
          locName = locNameBuf;
          break;
        case ARTIFACT_ONCORPSE:
          if( Godlist )
          {
            sprintf( locNameBuf, "%s's corpse", get_player_name_from_pid(ownerID) );
            locName = locNameBuf;
          }
          else
          {
            locName = get_player_name_from_pid(ownerID);
          }
          owner = load_dummy_char( get_player_name_from_pid(ownerID) );
          break;
        default:
          debug( "list_artifacts_sql: Bad locType: %d.", locType );
          extract_obj( obj, FALSE );
          continue;
          break;
      }

      // Increment total artis.
      articount[RACEWAR_NONE]++;
      // Increment the appropriate racewarside.
      if( owner )
      {
        if( GET_RACEWAR(owner) != RACEWAR_NONE )
        {
          articount[GET_RACEWAR(owner)]++;
        }
        nuke_eq(owner);
        extract_char(owner);
        owner = NULL;
      }
      // Mortals only see name and artifact.
      if( !Godlist )
      {
        sprintf(buf, "%-20s%s\r\n", locName, obj->short_description);
        send_to_char( buf, ch );
        shownData = TRUE;
        extract_obj( obj, FALSE );
        continue;
      }

      negTime = FALSE;
      // totalTime (left to poof in sec) is the timer (time at which it poofs) - now.
      if( (totalTime = atol(row[4]) - time(NULL)) < 0 )
      {
        negTime = TRUE;
        totalTime *= -1;
      }
      // Convert to minutes.
      totalTime /= 60;
      minutes = totalTime % 60;
      // Convert to hours.
      totalTime /= 60;
      hours = totalTime % 24;
      // Extract days.
      days = totalTime / 24;
      if( atol(row[4]) == 0 )
      {
        days = hours = minutes = 0;
      }
      sprintf( timer, "%c%2d:%02d:%02d", negTime ? '-' : ' ', days, hours, minutes );

      // Trim locName: NAX_NAME_LENGTH + strlen("'s corpse") == 12 + 9 == 21.
      sprintf( locNameBuf2, "%s", pad_ansi(locName, MAX_NAME_LENGTH + 9, TRUE).c_str() );
      locName = locNameBuf2;
      sprintf(buf, "%-21s&n%-11s %-22s%s (#%d)\r\n", locName, timer, row[5], obj->short_description, vnum );
      send_to_char( buf, ch );
      shownData = TRUE;
      extract_obj( obj, FALSE );
    }
    mysql_free_result(res);
  }
  else
  {
    debug( "list_artifacts_sql: Could not access %s artifact table for listing.", Godlist ? "Immortal" : "mortal" );
    send_to_char( "Error with database. :(\n\r", ch );
  }
  if( !shownData )
  {
    send_to_char( "No artifacts found.\n\r", ch );
  }
  else
  {
    sprintf(buf, "\r\n       &+r------&+LSummary&+r------&n\r\n" );
    sprintf(buf + strlen(buf), "         &+WGoodies:      %d&n\r\n", articount[RACEWAR_GOOD]);
    sprintf(buf + strlen(buf), "         &+rEvils:        %d&n\r\n", articount[RACEWAR_EVIL]);
    if( articount[RACEWAR_UNDEAD] )
      sprintf(buf + strlen(buf), "         &+LUndead:       %d&n\r\n", articount[RACEWAR_UNDEAD]);
    if( articount[RACEWAR_NEUTRAL] )
      sprintf(buf + strlen(buf), "         &+MNeutral:      %d&n\r\n", articount[RACEWAR_NEUTRAL]);
    sprintf(buf + strlen(buf), "         &+WTotal:        %d\r\n",
      articount[RACEWAR_NONE]);
    send_to_char( buf, ch );
  }
}

// Remove artifact entry from the artifacts table.
//   mortalToo means that we remove the entry from the mortals' table too.
// This is used for removing an arti from the game (I guess).
void arti_remove_sql( int vnum, bool mortalToo )
{
  if( !updateArtis)
  {
    return;
  }

  // Remove from artifacts table:
  qry( "DELETE FROM artifacts WHERE vnum = '%d'", vnum );
  // Possibly remove from artifacts_mortal table:
  if( mortalToo )
  {
    qry( "DELETE FROM artifacts_mortal WHERE vnum = '%d'", vnum );
  }
}

// This function is called at boot to set the mortals' artifact list table.
void setupMortArtiList_sql()
{
  // Clear the mortals table.
  qry( "TRUNCATE TABLE artifacts_mortal" );
  // Repopulate it:
  qry( "INSERT INTO artifacts_mortal SELECT * FROM artifacts WHERE locType='OnPC' OR locType='OnCorpse'" );
}

// Loads the artis that were on the ground and owned back into the boot.
void addOnGroundArtis_sql()
{
  P_obj arti;
  int   room;
  MYSQL_RES *res;
  MYSQL_ROW row;

  logit( LOG_ARTIFACT, "addOnGroundArtis_sql: Beginning." );

  qry("SELECT vnum, location FROM artifacts WHERE owned='Y' AND locType='OnGround'" );

  if( (res = mysql_store_result(DB)) != NULL )
  {
    if( mysql_num_rows(res) < 1 )
    {
      logit( LOG_ARTIFACT, "addOnGroundArtis_sql: No owned artifacts found on ground." );
    }
    else
    {
      while( (row = mysql_fetch_row(res)) )
      {
        if( !(arti = read_object( atoi(row[0]), VIRTUAL )) )
        {
          logit( LOG_ARTIFACT, "addOnGroundArtis_sql: Could not load object vnum %d.", atoi(row[0]) );
          continue;
        }
        if( (room = real_room(atoi(row[1]))) < 0 || room > top_of_world )
        {
          logit( LOG_ARTIFACT, "addOnGroundArtis_sql: Could not find room %d.", atoi(row[1]) );
          extract_obj( arti, FALSE );
          continue;
        }
        obj_to_room( arti, room );
      }
    }
    mysql_free_result(res);
  }
  else
  {
    logit( LOG_ARTIFACT, "addOnGroundArtis_sql: Could not pull on ground arti list." );
  }

  logit( LOG_ARTIFACT, "addOnGroundArtis_sql: Ending." );
}

// This function either finds the row in the artifacts table corresponding to arti,
//   or makes one, if there isn't one already.  If there is one, it calculates
//   the new time to poof.
void artifact_feed_to_min_sql( P_obj arti, int min_minutes )
{
  int vnum = GET_OBJ_VNUM(arti), location;
  long unsigned oldtime, to_time;
  P_char owner;
  P_obj  cont;
  MYSQL_RES *res;
  MYSQL_ROW row;

  if( !updateArtis )
  {
    return;
  }

  // Don't be corruptin' my table with a non-artifact!
  if( !IS_ARTIFACT(arti) )
  {
    logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: Non arti vnum %d.", vnum );
    return;
  }

  // Calculate the minimum time to poof in seconds.
  to_time = time(NULL) + min_minutes * 60;

  if( !qry("select owned, UNIX_TIMESTAMP(timer) from artifacts where vnum = %d", vnum) )
  {
    logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: failed to read from database.");
    return;
  }
  res = mysql_store_result(DB);
  if( mysql_num_rows(res) > 0 )
  {
    if( !(row = mysql_fetch_row(res)) )
    {
      logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: failed to fetch row.");
      mysql_free_result(res);
      return;
    }
    if( strcmp(row[0], "Y") )
    {
      logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: WARNING: Updating time on non-owned (%s) artifact %d.", row[0], vnum);
    }
    oldtime = atoi(row[1]);
    // Keep the bigger one, since we're feeding to at least min_minutes.
    to_time = (oldtime >= to_time) ? oldtime : to_time;

    qry("UPDATE artifacts SET timer = FROM_UNIXTIME(%lu) WHERE vnum = %d", to_time, vnum);
  }
  else
  {
    cont = arti;
    if( OBJ_INSIDE(cont) )
    {
      logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: arti vnum %d is inside a container?!", vnum);
      while( OBJ_INSIDE(cont) && cont->loc.inside )
      {
        cont = cont->loc.inside;
      }
    }

    if( OBJ_ROOM(cont) )
    {
      // Take Rnum and convert to vnum.
      location = cont->loc.room;
      if( location < 0 || location > top_of_world )
      {
        // Converting room to 0 here 'cause we're putting it in The Void instead of out-of-bounds.
        location = 0;
      }
      else
      {
        world[location].number;
      }
      qry("INSERT INTO artifacts VALUES(%d, 'Y', 'OnGround', %d, FROM_UNIXTIME(%lu), %d, SYSDATE() )", vnum,
        location, to_time, IS_IOUN(arti) ? ARTIFACT_IOUN : IS_UNIQUE(arti) ? ARTIFACT_UNIQUE : ARTIFACT_MAIN );
    }
    else if( OBJ_WORN(cont) || OBJ_CARRIED(cont) )
    {
      owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
      // We don't care if they're alive.
      if( !owner )
      {
        logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: arti vnum %d worn or carried, but no owner?!", vnum);
      }
      else
      {
        // Adding a NPC owner to arti -> owned = 'N', location = mob vnum.
        if( IS_NPC(owner) )
        {
          location = GET_VNUM(owner);
          qry("INSERT INTO artifacts VALUES(%d, 'N', 'OnNPC', %d, FROM_UNIXTIME(%lu), %d, SYSDATE() )", vnum,
            location, to_time, IS_IOUN(arti) ? ARTIFACT_IOUN : IS_UNIQUE(arti) ? ARTIFACT_UNIQUE : ARTIFACT_MAIN );
        }
        // Adding a PC owner to arti -> owned = 'Y', location = PID.
        else
        {
          location = GET_PID(owner);
          qry("INSERT INTO artifacts VALUES(%d, 'Y', 'OnPC', %d, FROM_UNIXTIME(%lu), %d, SYSDATE() )", vnum,
            location, to_time, IS_IOUN(arti) ? ARTIFACT_IOUN : IS_UNIQUE(arti) ? ARTIFACT_UNIQUE : ARTIFACT_MAIN );
        }
      }
    }
    else if( OBJ_INSIDE(cont) )
    {
      logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: arti vnum %d is inside a non-existent container?!", vnum);
    }
    else if( OBJ_NOWHERE(cont) )
    {
      logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: arti vnum %d is in location NOWHERE?!", vnum);
    }
    else
    {
      logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: arti vnum %d is in an UNKNOWN location?!", vnum);
    }
  }
  mysql_free_result(res);
}

// This function handles the 'soul' of the artifact.
void artifact_switch_check( P_char ch, P_obj arti )
{
  int owner_pid, timer, vnum;
  bool update = FALSE;

  if( !updateArtis )
  {
    return;
  }

  // And make sure it's and artifact
  if( !IS_ARTIFACT(arti) )
  {
    return;
  }

  // Gods don't affect the soul of the arti.
  if( IS_TRUSTED(ch) )
  {
    return;
  }

  // Load up the variables.
  vnum = GET_OBJ_VNUM(arti);
  sql_get_bind_data(vnum, &owner_pid, &timer);

  // If a pvp loot happened, and timeframe has passed, set to 0 for binding
  if( (owner_pid == -1)
    && (timer+(60*(int)get_property("artifact.feeding.switch.lootallowance.min", 30)) < time(NULL)) )
  {
    owner_pid = 0;
    update = TRUE;
  }

  // If we are ready to bind (from above, or because we picked arti up from it's load spot)
  if( !owner_pid && IS_PC(ch) )
  {
    // Set the artifact to the player
    act("&+L$p &+Lmerges with your &+wsoul&+L.", FALSE, ch, arti, 0, TO_CHAR);
    owner_pid = GET_PID(ch);
  }

  // If object is bound, and not being held by the owner
  if( IS_PC(ch) && (owner_pid != -1) && (owner_pid != GET_PID(ch)) )
  {
    // If by some chance, the timer wasn't set set it
    if( !timer )
    {
      logit( LOG_ARTIFACT, "artifact_switch_check: Timer on arti vnum %d was not set.", vnum );
      timer = time(NULL);
      update = TRUE;
    }
    // Otherwise if the timer is due, set it to the new player
    else if( (timer+(60*(int)get_property("artifact.feeding.switch.timer.min", 30))) < time(NULL) )
    {
      act("&+L$p &+Lmerges with your &+wsoul&+L.", FALSE, ch, arti, 0, TO_CHAR);
      owner_pid = GET_PID(ch);
      timer = 0;
      update = TRUE;
    }
    // 1% chance to complain.
    else if( !number(0, 99) )
    {
      act("&+m$p&+m whimpers softly inside your head.&n", FALSE, ch, arti, 0, TO_CHAR);
    }
  }
  // Or if object is bound by the currently carried player, make sure the timer is reset
  else if ( IS_PC(ch) && (owner_pid == GET_PID(ch)) )
  {
    timer = 0;
    update = TRUE;
  }

  if( update )
  {
    sql_update_bind_data(vnum, &owner_pid, &timer);
  }
}

// Update the DB with new artifact data.
// arti    : The artifact passed.  Should always be a valid arti, in a valid location.
//             Some error handling is done for invalid locations (mostly move to Limbo).
// owned   : 'Y'/'y' -> set owned in DB, 'N'/'n' -> unset owned in DB, any other value -> leave owned in DB.
// timer   : the new time when the artifact poofs (not the amount of time until it poofs).
// The variable obj1 represents the outer-most object (not in a container) from which we can
//   ascertain the true location of the arti (on a char / in a room / etc).
void artifact_update_sql( P_obj arti, char owned, time_t timer )
{
  int type, locType, location, vnum = arti ? GET_OBJ_VNUM(arti) : -1;
  bool new_owned, update_existing = FALSE;
  P_char owner;
  P_obj obj1;
  MYSQL_RES *res;
  MYSQL_ROW row;

  if( !updateArtis )
  {
    return;
  }

  if( !arti || !IS_ARTIFACT(arti) )
  {
    logit( LOG_ARTIFACT, "arti_update_sql: Non arti vnum %d.", vnum );
    return;
  }

  // Figure out the new info.
  type = IS_IOUN(arti) ? ARTIFACT_IOUN : IS_UNIQUE(arti) ? ARTIFACT_UNIQUE : ARTIFACT_MAIN;

  // Set location and locType here.
  // If we have it in a container, need to get the outer-most one and go from there.
  if( OBJ_INSIDE(arti) )
  {
    // Bug handling
    if( !(obj1 = arti->loc.inside) )
    {
      logit( LOG_ARTIFACT, "arti_update_sql: OBJ_INSIDE but no container, sending to Limbo, arti vnum %d.",
        vnum );
      arti->loc_p = LOC_NOWHERE;
      obj_to_room( arti, real_room0(ROOM_LIMBO_VNUM) );
    }
    else
    {
      // Get the outer-most container.
      while( OBJ_INSIDE(obj1) && obj1->loc.inside )
      {
        obj1 = obj1->loc.inside;
      }
      // Bug handling.. inside but NULL container.
      if( OBJ_INSIDE(obj1) )
      {
        logit( LOG_ARTIFACT, "arti_update_sql: OBJ_INSIDE but container not anywhere, sending to Limbo, arti vnum %d.",
          vnum );
        obj1->loc_p = LOC_NOWHERE;
        obj_to_room( obj1, real_room0(ROOM_LIMBO_VNUM) );
      }
    }
  }
  // If not in a container, just look for where arti is.
  else
  {
    obj1 = arti;
  }

  if( OBJ_WORN(obj1) || OBJ_CARRIED(obj1) )
  {
    // More bug handling.
    if( !(owner = (OBJ_WORN(obj1) ? obj1->loc.wearing : obj1->loc.carrying)) )
    {
      if( OBJ_WORN(obj1) )
      {
        logit( LOG_ARTIFACT, "arti_update_sql: OBJ_WORN but no loc.wearing, arti vnum %d/container vnum %d.",
          vnum, GET_OBJ_VNUM(obj1) );
      }
      else
      {
        logit( LOG_ARTIFACT, "arti_update_sql: OBJ_CARRIED but no loc.carrying, arti vnum %d.",
          vnum, GET_OBJ_VNUM(obj1) );
      }
      return;
    }
    // Imms can hold unlimited non-tracked artis.
    if( IS_TRUSTED(owner) )
    {
      return;
    }
    locType = IS_NPC(owner) ? ARTIFACT_ON_NPC : ARTIFACT_ON_PC;
    // GET_ID returns -2 if owner is not alive.
    location = GET_ID(owner);
  }
  else if( OBJ_ROOM(obj1) )
  {
    locType = ARTIFACT_ONGROUND;
    location = obj1->loc.room;
    if( location < 0 || location > top_of_world )
    {
      if( location == NOWHERE )
      {
        logit( LOG_ARTIFACT, "arti_update_sql: OBJ_ROOM but room num NOWHERE, listing as not in game, arti vnum %d.",
          vnum );
        locType = ARTIFACT_NOTINGAME;
      }
      else
      {
        logit( LOG_ARTIFACT, "arti_update_sql: OBJ_ROOM but room out of bounds, sending to Limbo, arti vnum %d.",
          vnum );
        obj1->loc_p = LOC_NOWHERE;
        obj_to_room( obj1, real_room0(ROOM_LIMBO_VNUM) );
        location = world[obj1->loc.room].number;
      }
    }
    else
    {
      // Convert from valid Rnum to Vnum.
      location = world[location].number;
    }
  }
  // Well, it's sorta a valid location (usually means we're inbetween obj_from_* and obj_to_* fns.
  else if( OBJ_NOWHERE(obj1) )
  {
    // If it's on a PC corpse (when corpses are created, they have eq added before being sent to a room,
    //   so they should come up under OBJ_NOWHERE.
    locType = ((obj1->type == ITEM_CORPSE) && IS_SET(obj1->value[CORPSE_FLAGS], PC_CORPSE))
      ? ARTIFACT_ONCORPSE : ARTIFACT_NOTINGAME;
    location = NOWHERE;
  }
  else
  {
    logit( LOG_ARTIFACT, "arti_update_sql: Trying to salvage arti vnum %d (unknown loc_p) to Limbo.", vnum );
    obj1->loc_p = LOC_NOWHERE;
    obj_to_room( obj1, real_room0(ROOM_LIMBO_VNUM) );
    locType = ARTIFACT_ONGROUND;
    location = world[obj1->loc.room].number;
  }
  // At this point, type, locType, location, and vnum should be correct.

  // If we can't query the DB, we have a big issue (only values we care about are time difference and owned value).
  if( !qry("SELECT owned, location, UNIX_TIMESTAMP(timer), UNIX_TIMESTAMP(lastUpdate) FROM artifacts WHERE vnum = %d", vnum) )
  {
    logit(LOG_ARTIFACT, "arti_update_sql: failed to read from database.");
    return;
  }
  res = mysql_store_result(DB);
  // Since vnum is unique, num rows should be 0 or 1.
  if( mysql_num_rows(res) < 1 || (row = mysql_fetch_row(res)) == NULL )
  {
    // Only set it to owned if we know that it's owned.
    if( UPPER(owned) == 'Y' )
    {
      new_owned = TRUE;
    }
    else
    {
      new_owned = FALSE;
    }
  }
  else
  {
    if( UPPER(owned) == 'Y' )
    {
      new_owned = TRUE;
    }
    else if( UPPER(owned) == 'N' )
    {
      new_owned = FALSE;
    }
    else
    {
      new_owned = (!strcmp(row[0], "Y")) ? TRUE : FALSE;
    }

    // If it's on a corpse, it should be on the corpse of the last owner,
    //   so we don't want to move it to NOWHERE.
    if( locType == ARTIFACT_ONCORPSE )
    {
      location = atoi(row[1]);
    }

    update_existing = TRUE;
  }

  mysql_free_result(res);

  // If we have an entry already in the DB, update it.
  if( update_existing )
  {
    // lastUpdate should update automatically.
    qry("UPDATE artifacts SET owned='%c', locType=%d, location=%d, timer=FROM_UNIXTIME(%lu), type=%d WHERE vnum=%d",
      new_owned ? 'Y' : 'N', locType, location, timer, type, vnum );
  }
  // Otherwise, create one.
  else
  {
    logit( LOG_ARTIFACT, "arti_update_sql: Creating entry: vnum: %d, new_owned: %c, locType: %d, location; %d, timer: %lu, type: %d.",
      vnum, new_owned ? 'Y' : 'N', locType, location, timer, type );

    // lastUpdate should update automatically.
    qry("INSERT INTO artifacts VALUES( %d, '%c', %d, %d, FROM_UNIXTIME(%lu), %d, SYSDATE())",
      vnum, new_owned ? 'Y' : 'N', locType, location, timer, type );
  }
}

// This function just updates/creates a new entry for the arti with vnum vnum.
void artifact_update_sql( int vnum, bool owned, int locType, int location, time_t timer, int type )
{
  bool update_existing;
  MYSQL_RES *res;
  MYSQL_ROW row;

  if( !updateArtis )
  {
    return;
  }

  // If we can't query the DB, we have a big issue (only values we care about are time difference and owned value).
  if( !qry("SELECT owned, location, UNIX_TIMESTAMP(timer), UNIX_TIMESTAMP(lastUpdate) FROM artifacts WHERE vnum = %d", vnum) )
  {
    logit(LOG_ARTIFACT, "arti_update_sql: failed to read from database.");
    return;
  }
  res = mysql_store_result(DB);

  // Since vnum is unique, num rows should be 0 or 1.
  if( mysql_num_rows(res) < 1 || (row = mysql_fetch_row(res)) == NULL )
  {
    update_existing = FALSE;
  }
  else
  {
    update_existing = TRUE;
  }
  mysql_free_result(res);

  if( update_existing )
  {
    qry("UPDATE artifacts SET owned='%c', locType=%d, location=%d, timer=FROM_UNIXTIME(%lu), type=%d WHERE vnum=%d",
      owned ? 'Y' : 'N', locType, location, timer, type, vnum );
  }
  else
  {
    qry("INSERT INTO artifacts VALUES(%d, '%c', %d, %d, FROM_UNIXTIME(%lu), %d, SYSDATE())",
      vnum, owned ? 'Y' : 'N', locType, location, timer, type );
  }
}

// Remove the artifact data from the DB.
//   arti : The artifact to remove.
//   pid  : Whether it goes to the corpse of char who's PID is pid, or <= 0 means arti is gone from game.
// Returns true if successfully removed.
bool remove_owned_artifact_sql( P_obj arti, int pid )
{
  int vnum = arti ? GET_OBJ_VNUM(arti) : -1;
  bool update_existing = FALSE;
  MYSQL_RES *res;
  MYSQL_ROW row = NULL;

  if( !updateArtis )
  {
    return FALSE;
  }

  // Bad arguments...
  if( !arti || !IS_ARTIFACT(arti) )
  {
    logit( LOG_ARTIFACT, "remove_owned_artifact_sql: called with non-artifact '%s' %d.",
      arti ? arti->short_description : "NULL", arti ? GET_OBJ_VNUM(arti) : -1 );
    return FALSE;
  }

  // If we can't query the DB, we have a big issue (only values we care about are time difference and owned value).
  if( !qry("SELECT owned, UNIX_TIMESTAMP(timer), UNIX_TIMESTAMP(lastUpdate) FROM artifacts WHERE vnum = %d", vnum) )
  {
    logit(LOG_ARTIFACT, "remove_owned_artifact_sql: failed to read from database.");
    return FALSE;
  }
  res = mysql_store_result(DB);
  if( mysql_num_rows(res) > 0 && !(row = mysql_fetch_row(res)) )
  {
    logit(LOG_ARTIFACT, "remove_owned_artifact_sql: failed to fetch row?!");
    mysql_free_result(res);
    return FALSE;
  }
  if( row )
  {
    update_existing = TRUE;
  }
  mysql_free_result(res);

  // If there's an existing entry in the DB.
  if( update_existing )
  {
    // Non-positive pid -> remove arti from game.
    if( pid <= 0 )
    {
      qry("UPDATE artifacts SET owned='N', locType=%d, location=%d WHERE vnum=%d", ARTIFACT_NOTINGAME, NOWHERE, vnum );
    }
    // Otherwise, we're moving to a corpse of char who's PID is pid.
    else
    {
      // On a PC corpse -> owned == Yes, and location == pid.
      qry("UPDATE artifacts SET owned='Y', locType=%d, location=%d WHERE vnum=%d", ARTIFACT_ONCORPSE, pid, vnum );
    }
  }
  // If the entry doesn't exist and we're moving arti to a corpse (Yes, this would be a buggy situation).
  else if( pid > 0 )
  {
      // On a PC corpse -> owned == 'Y', locType == 'OnCorpse', and location == pid.
      qry("INSERT INTO artifacts VALUES(%d, 'Y', %d, %d, 0, %d, SYSDATE())", vnum, ARTIFACT_ONCORPSE, pid,
        IS_IOUN(arti) ? ARTIFACT_IOUN : IS_UNIQUE(arti) ? ARTIFACT_UNIQUE : ARTIFACT_MAIN );
  }
  // If pid <= 0 && there's no existing entry, don't bother.
  return TRUE;
}

// This is used for when a character is deleted.
void remove_all_artifacts_sql( P_char ch )
{
  int pid;

  if( !updateArtis )
  {
    return;
  }

  // If no ch / ch isn't a PC / or ch doesn't have PC data.
  if( !ch || !IS_PC(ch) || !ch->only.pc )
  {
    return;
  }
  pid = GET_PID(ch);

  // Nullify arti timers on all ch's equipment.
  qry("UPDATE artifacts SET owned='N', timer=0 WHERE location=%d and locType=%d", pid, ARTIFACT_ON_PC );
}

// This is a wrapper function for artifact_update_sql.
// It merely calculates the timer and location, then calls the above with appropriate owned/timer info.
void artifact_update_location_sql( P_obj arti )
{
  bool      timerStarted;
  arti_data artidata;
  P_obj     cont = arti;
  P_char    owner;

  if( !arti || !IS_ARTIFACT(arti) || !updateArtis )
  {
    return;
  }

  // Get data from DB.
  timerStarted = get_artifact_data_sql( GET_OBJ_VNUM(arti), &artidata );

  // Get outer-most container.
  while( OBJ_INSIDE(cont) && cont->loc.inside )
  {
    cont = cont->loc.inside;
  }

  if( OBJ_WORN(cont) || OBJ_CARRIED(cont) )
  {
    owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
    if( IS_NPC(owner) )
    {
      // NPCs don't start the timer.
      artifact_update_sql( arti, '0', timerStarted ? artidata.timer : 0 );
    }
    // PCs get a new timer if it wasn't previously owned.
    else
    {
      artifact_switch_check( owner, arti );
      if( !timerStarted )
      {
        // Set the timer to the max for a newly acquired arti.
        artidata.timer = time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
      }
      // PC owner forces a start to timer.
      artifact_update_sql( arti, 'Y', artidata.timer );
    }
  }
  // It's in a room / container / corpse / nowhere: We don't change the timer.
  else
  {
    artifact_update_sql( arti, '0', timerStarted ? artidata.timer : 0 );
  }
}

// Returns TRUE iff arti vnum has timer ticking already.
bool get_artifact_data_sql( int vnum, P_arti adata )
{
  bool owned;
  MYSQL_RES *res;
  MYSQL_ROW row = NULL;

  if( !qry("SELECT owned, locType+0, location, UNIX_TIMESTAMP(timer), type FROM artifacts WHERE vnum = %d", vnum) )
  {
    logit(LOG_ARTIFACT, "get_artifact_data_sql: failed to read from database.");
    return FALSE;
  }
  res = mysql_store_result(DB);

  // Non-buggy no row for arti.
  if( mysql_num_rows(res) <= 0 )
  {
    mysql_free_result(res);
    return FALSE;
  }

  if( !(row = mysql_fetch_row(res)) )
  {
    logit(LOG_ARTIFACT, "get_artifact_data_sql: failed to fetch row?!");
    mysql_free_result(res);
    return FALSE;
  }
  owned = (!strcmp(row[0], "Y")) ? TRUE : FALSE;
  if( adata != NULL )
  {
    adata->vnum = vnum;
    adata->owned = owned;
    adata->locType = atoi(row[1]);
    adata->location = atoi(row[2]);
    adata->timer = atol(row[3]);
    adata->type = atoi(row[4]);
    adata->next = NULL;
  }
  mysql_free_result(res);
  return owned;
}

void artifact_feed_sql(P_char owner, P_obj arti, int feed_seconds, bool soulCheck)
{
  int       vnum, owner_pid, timer;
  arti_data artidata;
  time_t    poof_time;
  bool      negFeed = FALSE;

  if( !updateArtis )
  {
    return;
  }

  if( !owner || !arti || IS_NPC(owner) )
  {
    statuslog(MINLVLIMMORTAL, "artifact_feed_sql: called with null / NPC owner or NULL arti.");
    debug( "artifact_feed_sql: called with owner (%s) and arti (%s) %d.", owner ? J_NAME(owner) : "NULL",
      arti ? arti->short_description : "NULL", arti ? GET_OBJ_VNUM(arti) : -1 );
    return;
  }
  if( IS_TRUSTED(owner) )
  {
    return;
  }

  vnum = GET_OBJ_VNUM(arti);
  sql_get_bind_data(vnum, &owner_pid, &timer);

  // Anti artifact sharing for feeding check
  if( soulCheck && IS_PC(owner) && (owner_pid != -1) && (owner_pid != GET_PID(owner)) )
  {
    act("&+L$p &+Lhas yet to accept you as its owner.", FALSE, owner, arti, 0, TO_CHAR);
    return;
  }

  // Get data from DB.
  poof_time = time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
  if( !get_artifact_data_sql( vnum, &artidata ) )
  {
    statuslog(MINLVLIMMORTAL, "artifact_feed_sql: called without an entry in DB?!");
    send_to_char("&+RYou feel a deep sense of satisfaction from somewhere...\r\n", owner);
    qry("INSERT INTO artifacts VALUES(%d, 'Y', %d, %d, FROM_UNIXTIME(%lu), %d, SYSDATE())",
      vnum, ARTIFACT_ON_PC, GET_PID(owner), poof_time, IS_IOUN(arti) ? ARTIFACT_IOUN
      : (IS_UNIQUE(arti) ? ARTIFACT_UNIQUE : ARTIFACT_MAIN) );
    return;
  }

  // If we're tyring to feed over the limit.
  if( poof_time < artidata.timer + feed_seconds )
  {
    // Set it to feed to max
    feed_seconds = poof_time - artidata.timer;
  }

  if( feed_seconds > ( 12 * 3600 ) )
  {
    send_to_char("&+RYou feel a deep sense of satisfaction from somewhere...&n\r\n", owner);
  }
  else if( feed_seconds < 0 )
  {
    negFeed = TRUE;
    feed_seconds *= -1;
    send_to_char("&+BYou hear a distant cry in the back of your mind...&n\n\r", owner);
  }
  else
  {
    send_to_char("&+RYou feel a light sense of satisfaction from somewhere...&n\r\n", owner);
  }

  statuslog(MINLVLIMMORTAL, "Artifact: %s [%d] on %s fed [&+G%c%ld&+Lh &+G%ld&+Lm &+G%ld&+Ls&n]",
    arti->short_description, GET_OBJ_VNUM(arti), GET_NAME(owner),
    negFeed ? '-' : ' ', feed_seconds / 3600, (feed_seconds / 60) % 60, feed_seconds % 60 );

  if( artidata.owned && (artidata.locType == ARTIFACT_ON_PC || artidata.locType == ARTIFACT_ONCORPSE)
    && artidata.location != GET_PID(owner) )
  {
    statuslog(MINLVLIMMORTAL, "artifact_feed_sql: tried to track arti vnum %d on %s when already tracked on %s.",
      vnum, GET_NAME(owner), get_player_name_from_pid(artidata.location) );
    return;
  }
  artifact_update_sql( arti, 'Y', artidata.timer + feed_seconds );
}

// Loads up a dummy copy of char 'name'.
P_char load_dummy_char( char *name )
{
  P_char owner;

  // Get the memory
  owner = (P_char) mm_get(dead_mob_pool);
  clear_char(owner);
  owner->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);
  owner->desc = NULL;

  if( restoreCharOnly( owner, name ) < 0 )
  {
    logit( LOG_ARTIFACT, "load_dummy_char: %s has bad / missing pfile.\n\r", name );
    free_char( owner );
    return NULL;
  }

  updateArtis = FALSE;
  restoreItemsOnly( owner, -1 );
  owner->next = character_list;
  character_list = owner;
  setCharPhysTypeInfo( owner );
  updateArtis = TRUE;

  return owner;
}

// Return a pointer to the first obj of vnum vnum on owner.
//   Checks eq'd eq first.
//   Does not check inside containers.
// Does not remove the object (this would ruin poof_artifact)!
P_obj get_object_from_char( P_char owner, int vnum )
{
  int i;
  P_obj obj;

  // Check eq'd stuff first.
  for( i = 0; i < MAX_WEAR; i++ )
  {
    if( !owner->equipment[i] )
      continue;
    if( obj_index[owner->equipment[i]->R_num].virtual_number == vnum )
      return owner->equipment[i];
  }
  // Then check inventory.
  for( obj = owner->carrying; obj; obj = obj->next_content )
  {
    if( obj_index[obj->R_num].virtual_number == vnum )
      return obj;
  }
  // If they don't have it.
  return NULL;
}

void poof_artifact( P_obj arti )
{
  int    room;
  P_char owner;
  P_obj  cont;

  if( !updateArtis )
  {
    return;
  }

  if( !IS_ARTIFACT(arti) )
  {
    logit( LOG_ARTIFACT, "poof_artifact: Non-arti vnum %d!", GET_OBJ_VNUM(arti) );
    return;
  }

  cont = arti;
  while( OBJ_INSIDE(cont) && cont->loc.inside )
  {
    cont = cont->loc.inside;
  }

  owner = NULL;
  // Pull artifact from wherever it is.
  switch( cont->loc_p )
  {
    case LOC_ROOM:
      if( cont != arti )
      {
        obj_from_obj(arti);
        obj_to_room(arti, cont->loc.room);
      }
      act("$p suddenly vanishes in a bright flash of light!", FALSE, NULL, arti, 0, TO_ROOM);
      break;
    case LOC_CARRIED:
      owner = cont->loc.carrying;
      if( cont != arti )
      {
        obj_from_obj(arti);
        obj_to_char(arti, owner);
      }
      act("Your $p vanishes with a bright flash of light!", FALSE, owner, arti, 0, TO_CHAR);
      act("$n's $p suddenly vanishes in a bright flash of light!", FALSE, owner, arti, 0, TO_ROOM);
      do_shout(owner, "Ouch!", 0);
      break;
    case LOC_WORN:
      owner = cont->loc.wearing;
      if( cont != arti )
      {
        obj_from_obj(arti);
        obj_to_char(arti, owner);
      }
      act("Your $p vanishes with a bright flash of light!", FALSE, owner, arti, 0, TO_CHAR);
      act("$n's $p suddenly vanishes in a bright flash of light!", FALSE, owner, arti, 0, TO_ROOM);
      do_shout(owner, "Ouch!", 0);
      break;
    case LOC_INSIDE:
      logit( LOG_ARTIFACT, "poof_artifact: Bad loc arti(%d)-container(%d) inside nothing.",
        GET_OBJ_VNUM(arti), GET_OBJ_VNUM(cont) );
    case LOC_NOWHERE:
    default:
      break;
  }
  if( cont == arti )
  {
    cont = NULL;
  }
  // And get rid of it.
  extract_obj( arti, TRUE );
  // Save where appropriate.
  if( owner )
  {
    writeCharacter(owner, RENT_POOFARTI, owner->in_room);
  }
  if( cont != NULL && GET_OBJ_VNUM(cont) == VOBJ_CORPSE && IS_SET(cont->value[CORPSE_FLAGS], PC_CORPSE) )
  {
    writeCorpse(cont);
  }
}

// Removes an unsaved char's equipment from game without disturbing arti list.
void nuke_eq( P_char ch )
{
  int i;
  P_obj item;

  for( i = 0; i < MAX_WEAR; i++ )
  {
    if( ch->equipment[i] )
    {
      item = unequip_char(ch, i);
      // This must be FALSE to prevent nuking arti files.
      extract_obj( item, FALSE );
    }
  }
  while( ch->carrying )
  {
    item = ch->carrying;
    obj_from_char( item );
    // This must be FALSE to prevent nuking arti files.
    extract_obj( item, FALSE );
  }
}

// Takes the arti vnum, and turns it into a text string (in buffer) displaying
//   the time left on the corresponding artifact.
void artifact_timer_sql( int vnum, char *buffer )
{
  arti_data artidata;
  time_t timer;

  if( !updateArtis )
  {
    return;
  }

  if( get_artifact_data_sql( vnum, &artidata ) )
  {
    if( artidata.owned )
    {
      timer = artidata.timer - time(NULL);
    }
    else
    {
      sprintf(buffer, "[ &=LRUnknown&n ]" );
    }
    if( timer < 0 )
    {
      timer *= -1;
      sprintf(buffer,"[&+R-%ld&+Lh &+R%ld&+Lm &+R%ld&+Ls&n]", timer / 3600, (timer / 60) % 60, timer % 60);
    }
    else if( timer <= (ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY)/16 )
    {
      sprintf(buffer, "[&+R%ld&+Lh &+R%ld&+Lm &+R%ld&+Ls&n]", timer / 3600, (timer / 60) % 60, timer % 60);
    }
    else if( timer <= (ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY)/5 )
    {
      sprintf(buffer, "[&+Y%ld&+Lh &+Y%ld&+Lm &+Y%ld&+Ls&n]", timer / 3600, (timer / 60) % 60, timer % 60);
    }
    else
    {
      sprintf(buffer, "[&+G%ld&+Lh &+G%ld&+Lm &+G%ld&+Ls&n]", timer / 3600, (timer / 60) % 60, timer % 60);
    }
  }
  else
  {
    sprintf(buffer, "[ &=LCUnknown&n ]" );
  }
}

// Recursive function to search through bags for the first instance of obj vnum.
//   First does not necessarily mean the outermost instance (although there should only be one.)
P_obj artifact_in_bag_search( P_obj contents, int vnum )
{
  P_obj temp, inside;

  // Look through the contents.
  for( temp = contents; temp; temp = temp->next_content )
  {
    // Found it.
    if( GET_OBJ_VNUM(temp) == vnum )
    {
      return temp;
    }
    // If we have a container in a container, search those contents.
    if( temp->contains )
    {
      // If it's inside those contents, return it.
      if( inside = artifact_in_bag_search(temp->contains, vnum) )
      {
        return inside;
      }
    }
  }
  // Didn't find it. :(
  return NULL;
}

// Finds an artifact in game not on an Immortal or PC or PC corpse.
// The goal of this function is to find the artifact where it loads in game before it's acquired by a PC.
P_obj artifact_find( int vnum )
{
  P_char owner;
  P_obj obj, cont;

  for( obj = object_list; obj; obj = obj->next )
  {
    if( GET_OBJ_VNUM(obj) == vnum )
    {
      cont = obj;
      while( OBJ_INSIDE(cont) && cont->loc.inside )
      {
        cont = cont->loc.inside;
      }
      switch( cont->loc_p )
      {
        // Score!
        case LOC_ROOM:
          return obj;
          break;
        case LOC_CARRIED:
        case LOC_WORN:
          owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
          if( IS_NPC(owner) )
          {
            return obj;
          }
          break;
        case LOC_INSIDE:
          logit( LOG_ARTIFACT, "artifact_find: Bad loc arti(%d)-container(%d) inside nothing.",
            vnum, GET_OBJ_VNUM(cont) );
          break;
        // Lost artis. :(
        case LOC_NOWHERE:
        default:
          break;
      }
    }
  }

  return NULL;
}

// This function finds the artifact corresponding to artidata.
// It does not return an artifact that isn't on the proper PC corpse for example.
// It also just searches in game.  If you want to search pfiles.  Do it after.
P_obj artifact_find( arti_data artidata )
{
  int    rroom, pid;
  int    vnum = artidata.vnum;
  int    i;
  char  *name;
  P_obj  obj, inside;
  P_char ch;

  switch( artidata.locType )
  {
    case ARTIFACT_ON_NPC:
      pid = artidata.location;
      for( ch = character_list; ch; ch = ch->next )
      {
        if( IS_NPC(ch) && GET_VNUM(ch) == pid )
        {
          // Check eq worn first, since they will probably be wearing it.
          for( i = 0; i < MAX_WEAR; i++ )
          {
            if( obj = ch->equipment[i] )
            {
              if( GET_OBJ_VNUM(obj) == vnum )
              {
                return obj;
              }
              // If they're wearing a container.
              if( obj->contains && (inside = artifact_in_bag_search(obj->contains, vnum)) )
              {
                return inside;
              }
            }
          }
          // If they're not wearing it, then check inventory.  We do have to keep looking since
          //   there can be multiple NPCs with the same vnum.
          if( inside = artifact_in_bag_search(ch->carrying, vnum) )
          {
            return inside;
          }
        }
      }
      break;
    // Since they may be LD, we have to check all chars, not just descriptors. :(
    case ARTIFACT_ON_PC:
      pid = artidata.location;
      for( ch = character_list; ch; ch = ch->next )
      {
        // Yay, we found them in game!
        if( IS_PC(ch) && GET_PID(ch) == pid )
        {
          // Check eq worn first, since they will probably be wearing it.
          for( i = 0; i < MAX_WEAR; i++ )
          {
            if( obj = ch->equipment[i] )
            {
              if( GET_OBJ_VNUM(obj) == vnum )
              {
                return obj;
              }
              // If they're wearing a container.
              if( obj->contains && (inside = artifact_in_bag_search(obj->contains, vnum)) )
              {
                return inside;
              }
            }
          }
          // If they're not wearing it, then check inventory.  We don't have to keep looking since
          //   each PC is unique in game.
          return artifact_in_bag_search(ch->carrying, vnum);
        }
      }
      // Here it gets hard: do we check the pfile or not?  I say no, since we don't have a way
      //   to dispose of the loaded char after the arti is handled.  If you want to check a pfile,
      //   you'll just have to do it after this function returns NULL.
      return NULL;
      break;
    // Since we don't have the room the corpse is in, we have to go through the whole object list.
    case ARTIFACT_ONCORPSE:
      name = get_player_name_from_pid( artidata.location );
      for( obj = object_list; obj; obj = obj->next )
      {
        // If we found a PC corpse with the correct name.  I would so like to start using pids
        //   instead of names for corpses.
        if( obj->type == ITEM_CORPSE && IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE)
          && isname(name, obj->name) )
        {
          if( inside = artifact_in_bag_search( obj->contains, vnum) )
          {
            return inside;
          }
        }
      }
      return NULL;
      break;
    case ARTIFACT_ONGROUND:
      // Bad room vnum on ground data.
      if( (rroom = real_room(artidata.location)) < 0 || rroom > top_of_world )
      {
        return NULL;
      }
      // Search the room's contents.
      for( obj = world[rroom].contents; obj; obj = obj->next_content )
      {
        if( GET_OBJ_VNUM(obj) == vnum )
        {
          return obj;
        }
        // If we've found a container.
        if( obj->contains )
        {
          // Look for the object inside the container.
          if( inside = artifact_in_bag_search(obj->contains, vnum) )
          {
            return inside;
          }
        }
      }
      return NULL;
    case ARTIFACT_NOTINGAME:
    default:
      return NULL;
      break;
  }
  // Should never get here, but just in case.
  return NULL;
}

// This function transfers the data from the old file-based system into the DB.
void arti_files_to_sql( P_char ch, char *arg )
{
  char      buf[MAX_STRING_LENGTH];
  char      pname[256], fname[256];
  int       vnum, pid, temp, type;
  time_t    lastUpdate, timer;
  DIR      *dir;
  struct    dirent *dire;
  FILE     *f;
  P_obj     arti, obj, obj2;
  P_char    tmpch, owner;
  arti_data artidata;
  bool super;

  if( !*arg || !strcmp(arg, "?") || !strcmp(arg, "help") )
  {
    send_to_char("This command transfers the data from the old file-based system into the DB.\n\r", ch);
    send_to_char("Please note: This command may pull artifacts from mobs in game.\n\r", ch);
    send_to_char("&=LRThis command should only be used once!&n\n\r", ch);
    return;
  }
  if( GET_LEVEL(ch) < OVERLORD )
  {
    send_to_char("This command is reserved for use by &+rOverlords&n only.\n\r", ch);
    return;
  }
  if( strcmp(arg, "confirm") && strcmp(arg, "super") )
  {
    send_to_char("This command is requires &+wconfirmation&n.  Please do not use if you don't know"
      " what you're doing, as it can corrupt the current artifact data.\n\r", ch);
    return;
  }
  if( !strcmp(arg, "super") )
  {
    super = TRUE;
  }
  else
  {
    super = FALSE;
  }

  // At this point, it's a go.
  dir = opendir(ARTIFACT_DIR);

  if( !dir )
  {
    sprintf(buf, "Could not open arti dir (%s)\r\n", ARTIFACT_DIR);
    send_to_char(buf, ch);
    return;
  }

  while( dire = readdir(dir) )
  {

    vnum = atoi(dire->d_name);
    if( !vnum )
    {
      continue;
    }
    sprintf(fname, ARTIFACT_DIR "%d", vnum);
    sprintf(buf, "Loading artifact file '%s'.\n\r", fname );
    send_to_char( buf, ch );

    if( !(f = fopen(fname, "rt")) )
    {
      logit( LOG_ARTIFACT, "arti_files_to_sql: Could not open file '%s'.", fname );
      continue;
    }

    // Init name to empty string.
    pname[0] = '\0';
    if( fscanf(f, "%s %d %lu %d %lu\n", pname, &pid, &lastUpdate, &temp, &timer) != 5 )
    {
      logit( LOG_ARTIFACT, "arti_files_to_sql: Could not read file '%s'.", fname );
      fclose(f);
      continue;
    }
    fclose(f);

    timer += ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
    sprintf( buf, "  Char '%s' %d, has arti %d (%s) with timer %s", pname, pid, vnum,
      (temp == 0) ? "on char" : ((temp == 1) ? "on corpse" : "unknown location"), ctime(&timer) );
    send_to_char( buf, ch );

    if( get_artifact_data_sql( vnum, &artidata ) )
    {
      // If it's owned by some other PC.
      if( artidata.owned && (artidata.locType == ARTIFACT_ON_PC || artidata.locType == ARTIFACT_ONCORPSE) )
      {
        sprintf( buf, "  &+rConflicts with data already in DB: Char '%s' %d (%s).. skipping...&n\n\r",
          get_player_name_from_pid(artidata.location), artidata.location,
          (artidata.locType == ARTIFACT_ON_PC) ? "on char" : ((artidata.locType == ARTIFACT_ONCORPSE) ? "on corpse"
          : "unknown location") );
        send_to_char( buf, ch );
        continue;
      }
      // Otherwise, pull it from ground/NPC/etc.
      else if( arti = artifact_find(artidata) )
      {
        sprintf( buf, "Found another copy of arti %s (%d) in game, pulling it.\n\r",
          OBJ_SHORT(arti), GET_OBJ_VNUM(arti) );
        send_to_char( buf, ch );
        extract_obj(arti, FALSE);
      }
    }

    if( arti = read_object( vnum, VIRTUAL) )
    {
      type = IS_IOUN(arti) ? ARTIFACT_IOUN : IS_UNIQUE(arti) ? ARTIFACT_UNIQUE : ARTIFACT_MAIN;

      // If there's another in the game (maybe on a mob / treasure room / chest / etc).
      if( obj_index[arti->R_num].number > 1 )
      {
        // Look for all other copies (this may piss off Imms with artis in bags or such).
        for( obj = object_list; obj; obj = obj2 )
        {
          obj2 = obj->next;

          // Remove it if we find it not on a PC.
          if( obj->R_num == arti->R_num && obj != arti )
          {
            if( OBJ_WORN(obj) || OBJ_CARRIED(obj) )
            {
              tmpch = OBJ_WORN(obj) ? obj->loc.wearing : obj->loc.carrying;
              if( tmpch && IS_NPC(tmpch) )
              {
                sprintf( buf, "  &+YFound another copy of arti '&n%s&+Y' &+w%d&+Y on '&n%s&+Y' &+w%d&+Y, pulling it.&n\n\r",
                  OBJ_SHORT(arti), GET_OBJ_VNUM(arti), J_NAME(tmpch), GET_VNUM(tmpch) );
                send_to_char( buf, ch );
                extract_obj(obj, FALSE);
              }
            }
            else
            {
              sprintf( buf, "  &+YFound another copy of arti '&n%s&+Y' &+w%d&+Y in game not on a char, pulling it.&n\n\r",
                OBJ_SHORT(arti), GET_OBJ_VNUM(arti) );
              send_to_char( buf, ch );
              extract_obj(obj, TRUE); // Yes, we want to remove the arti data here.
            }
          }
        }
      }

      // If we can read everything, update the entry for it.
      artifact_update_sql( vnum, TRUE, (temp==0) ? ARTIFACT_ON_PC : ARTIFACT_ONCORPSE, pid, timer, type );

      if( super )
      {
        owner = load_dummy_char(pname);
        if( !owner )
        {
          extract_obj( arti, FALSE );
        }
        else
        {
          if( get_object_from_char( owner, vnum ) == NULL )
          {
            obj_to_char( arti, owner);
            writeCharacter(owner, RENT_CRASH, owner->in_room);
          }
          nuke_eq(owner);
          extract_char(owner);
        }
      }
      else
        extract_obj( arti, FALSE );
    }

  }

  closedir(dir);
}

void event_artifact_check_poof_sql( P_char ch, P_char vict, P_obj obj, void * arg )
{
  P_obj      arti, cont, content, corpse;
  P_char     owner;
  P_desc     desc;
  int        vnum, locType, location;
  bool       found;
  char      *name;
  MYSQL_RES *res;
  MYSQL_ROW  row = NULL;

  if( !updateArtis )
  {
    return;
  }

  // Pull all the artis we're gonna poof (one's that are owned, time's up and in game).
  qry("SELECT vnum, locType + 0, location FROM artifacts WHERE owned='Y' AND timer < now() AND locType<>'NotInGame'" );
  res = mysql_store_result(DB);

  // If there were any artis to pull
  if( mysql_num_rows(res) > 0 )
  {
    while( (row = mysql_fetch_row(res)) )
    {
      vnum     = atoi(row[0]);
      locType  = atoi(row[1]);
      location = atoi(row[2]);

      arti = NULL;
      if( locType == ARTIFACT_ONGROUND )
      {
        location = real_room(location);
        if( location >=0 && location <= top_of_world )
        {
          for( arti = world[location].contents; arti; arti = arti->next_content )
          {
            if( GET_OBJ_VNUM(arti) == vnum )
            {
              break;
            }
          }
        }
        // Here things get hairy.  We need to find a copy of the arti that's on ground or in a container
        //   that's not on an Immortal or in a bag possessed by an Immortal.  This case should never come
        //   up, but just in case.
        if( !arti || GET_OBJ_VNUM(arti) != vnum )
        {
          // Find the artifact in game, if it is.
          found = FALSE;
          for( arti = object_list; arti && !found; arti = arti->next )
          {
            if( GET_OBJ_VNUM(arti) == vnum )
            {
              cont = arti;
              // Find outermost container.
              while( OBJ_INSIDE(cont) && cont->loc.inside )
              {
                cont = cont->loc.inside;
              }
              switch( cont->loc_p )
              {
                // Score!
                case LOC_ROOM:
                  logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad location on 'OnGround' arti(%d) - actual room v%d.",
                    vnum, ROOM_VNUM(cont->loc.room) );
                  found = TRUE;
                  break;
                case LOC_CARRIED:
                case LOC_WORN:
                  owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
                  if( IS_NPC(owner) )
                  {
                    logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad location on 'OnGround' arti(%d) - on mob v%d.",
                      vnum, GET_VNUM(owner) );
                    found = TRUE;
                  }
                  break;
                case LOC_INSIDE:
                  logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad loc on 'OnGround' arti(%d)-container vnum %d inside nothing.",
                    cont->loc_p, vnum, GET_OBJ_VNUM(cont) );
                  break;
                // Lost artis. :(
                case LOC_NOWHERE:
                default:
                  logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad loc_p (%d) on 'OnGround' arti(%d)-container vnum %d.",
                    cont->loc_p, vnum, GET_OBJ_VNUM(cont) );
                  break;
              }
            }
          }
          // Just to make sure.
          if( !found )
          {
            arti = NULL;
          }
        }
        if( !arti || GET_OBJ_VNUM(arti) != vnum )
        {
          logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Could not find 'OnGround' artifact vnum %d anywhere.",
            vnum );
          continue;
        }
        // If we found arti in game.
        else
        {
          // Poof it!
          poof_artifact( arti );
        }
      } // End if locType == ARTIFACT_ONGROUND.
      else if( locType == ARTIFACT_ON_PC )
      {
        // Try to load the pfile if it's on a PC that isn't online.
        if( !is_pid_online(location, TRUE) )
        {
          owner = load_dummy_char(get_player_name_from_pid( location ));
          if( owner )
          {
            arti = get_object_from_char( owner, vnum );
          }
          if( arti )
          {
            poof_artifact( arti );
            writeCharacter( owner, RENT_POOFARTI, owner->in_room );
          }
          else
          {
            if( owner )
            {
              // Nuke the eq off dummy char so it doesn't fall to the ground and get duped.
              nuke_eq( owner );
            }
            logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Could not find artifact vnum %d on pfile of '%s' %d.",
              vnum, get_player_name_from_pid( location ), location );
          }
          extract_char( owner );
        }
        // PC online.
        else
        {
          arti = NULL;
          // First check descriptors (for speed).
          for( desc = descriptor_list; desc; desc = desc->next )
          {
            // Char must be in game and exist.
            if( desc->connected != CON_PLYNG || !(owner = GET_TRUE_CHAR_D(desc)) )
            {
              continue;
            }
            // Skip immortals.
            if( IS_TRUSTED(owner) )
            {
              continue;
            }
            if( arti = get_object_from_char( owner, vnum ) )
            {
              break;
            }
          }
          // If we didn't find it on descriptor list, gotta check character list for ld chars :(
          if( arti == NULL )
          {
            for( owner = character_list; owner; owner = owner->next )
            {
              // Found them ld! Hopefully, there's only one of them.
              if( IS_PC(owner) && GET_PID(owner) == location )
              {
                arti = get_object_from_char( owner, vnum );
                break;
              }
            }
          }
          if( arti != NULL )
          {
            poof_artifact( arti );
          }
          else
          {
            logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Could not find artifact vnum %d on char '%s' %d.",
              vnum, get_player_name_from_pid( location ), location );
          }
        }
      }
      else if( locType == ARTIFACT_ON_NPC )
      {
        for( owner = character_list; owner; owner = owner->next )
        {
          if( IS_NPC(owner) && GET_VNUM(owner) == location )
          {
            if( arti = get_object_from_char( owner, vnum ) )
            {
              break;
            }
          }
        }
        if( arti != NULL )
        {
          poof_artifact( arti );
        }
        else
        {
          logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Could not find artifact vnum %d on mob %d.",
            vnum, location );
        }
      }
      else if( locType == ARTIFACT_ONCORPSE )
      {
        name = get_player_name_from_pid(location);
        found = FALSE;
        corpse = NULL;
        for( arti = object_list; arti; arti = arti->next )
        {
          if( GET_OBJ_VNUM(arti) == vnum )
          {
            cont = arti;
            // Find outermost container.
            while( OBJ_INSIDE(cont) && cont->loc.inside )
            {
              cont = cont->loc.inside;

              if( GET_OBJ_VNUM(cont) == VOBJ_CORPSE && IS_SET(cont->value[CORPSE_FLAGS], PC_CORPSE)
                && isname(name, cont->name) )
              {
                corpse = cont;
                // It's on a corpse of the right char (don't stop moving outward though).
                found = TRUE;
              }
            }
            if( found )
            {
              obj_from_obj(arti);
              if( corpse != cont )
              {
                logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Poofing arti from corpse inside a container. ugh." );
              }
              else
              {
                writeCorpse(corpse);
              }
              switch( cont->loc_p )
              {
                case LOC_ROOM:
                  obj_to_room(arti, cont->loc.room);
                  break;
                case LOC_CARRIED:
                case LOC_WORN:
                  owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
                  obj_to_room(arti, owner->in_room);
                  break;
                case LOC_INSIDE:
                  logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad loc on 'OnCorpse' arti(%d)-container vnum %d inside nothing.",
                    cont->loc_p, vnum, GET_OBJ_VNUM(cont) );
                  break;
                // Lost artis. :(
                case LOC_NOWHERE:
                default:
                  logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad loc_p (%d) on 'OnCorpse' arti(%d)-container vnum %d.",
                    cont->loc_p, vnum, GET_OBJ_VNUM(cont) );
                  break;
              }
              break;
            }
          }
        }
        if( found )
        {
          poof_artifact(arti);
          break;
        }
        else
        {
          // Check for arti in anyplace other than on an Immortal.
          for( arti = object_list; arti && !found; arti = arti->next )
          {
            if( GET_OBJ_VNUM(arti) == vnum )
            {
              cont = arti;
              // Find outermost container.
              while( OBJ_INSIDE(cont) && cont->loc.inside )
              {
                cont = cont->loc.inside;
              }
              switch( cont->loc_p )
              {
                case LOC_ROOM:
                  logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad location on 'OnCorpse' arti(%d) - in room v%d.",
                    vnum, ROOM_VNUM(cont->loc.room) );
                  found = TRUE;
                  break;
                case LOC_CARRIED:
                case LOC_WORN:
                  owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
                  if( IS_NPC(owner) )
                  {
                    logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad location on 'OnCorpse' arti(%d) - on mob v%d.",
                      vnum, GET_VNUM(owner) );
                    found = TRUE;
                  }
                  break;
                case LOC_INSIDE:
                  logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad loc on 'OnCorpse' arti(%d)-container vnum %d inside nothing.",
                    cont->loc_p, vnum, GET_OBJ_VNUM(cont) );
                  break;
                // Lost artis. :(
                case LOC_NOWHERE:
                default:
                  logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Bad loc_p (%d) on 'OnCorpse' arti(%d)-container vnum %d.",
                    cont->loc_p, vnum, GET_OBJ_VNUM(cont) );
                  break;
              }
            }
          }
          if( found )
          {
            poof_artifact(arti);
          }
          else
          {
            logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Could not find artifact vnum %d on PC corpse %s (%d).",
              vnum, name, location );
          }
        }
      }
      else
      {
        logit( LOG_ARTIFACT, "event_artifact_check_poof_sql: Could not find artifact vnum %d: bad locType: %d.",
          vnum, locType );
      }
    }
  }

  mysql_free_result(res);

  // Clear the artis from the list.  Note: doing it after the loop intentionally.
  qry( "UPDATE artifacts SET owned='N', locType='NotInGame', location=-1, timer=0 WHERE owned='Y' AND timer < now()" );

  add_event( event_artifact_check_poof_sql, 6 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0 );
}

// Looks through list, and adds entry to the end of list.
// Assumes list is not NULL (has at least one entry).
void add_artidata_to_list( arti_data *list, int vnum, bool owned, char locType, int location, time_t timer, int type )
{

  if( !updateArtis )
  {
    return;
  }

  // This is bad.. vnum is already on list.
  if( list->vnum == vnum )
  {
    logit( LOG_ARTIFACT, "add_artidata_to_list: vnum %d already on list!? (at head location: %d).", list->location );
    return;
  }
  // Go to the last node.
  while( list->next )
  {
    list = list->next;
    // This is bad.. vnum is already on list.
    if( list->vnum == vnum )
    {
      logit( LOG_ARTIFACT, "add_artidata_to_list: vnum %d already on list!? (location: %d).", list->location );
      return;
    }
  }
  // And create a new one.
  list->next = new arti_data;
  list = list->next;
  list->vnum = vnum;
  list->owned = owned;
  list->locType = locType;
  list->location = location;
  list->timer = timer;
  list->type = type;
  list->next = NULL;
}

void event_artifact_wars_sql(P_char ch, P_char vict, P_obj obj, void *arg)
{
  arti_list *artilist, *nextlist;
  arti_data *node, *next_node;
  P_obj      arti;
  P_char     owner;
  int        pid, vnum, punish_level, punishment;
  int        count[4];
  MYSQL_RES *res;
  MYSQL_ROW  row;

  debug( "event_artifact_wars: beginning..." );

  // We only care about artis on a PC or corpse (Note: it's implied that owned='Y' on these).
  // The other options: not in game, on npc, on ground can not have fighting artis.
  qry("SELECT vnum, locType+0, location, UNIX_TIMESTAMP(timer), type FROM artifacts WHERE locType='OnPC' OR locType='OnCorpse'" );
  res = mysql_store_result(DB);

  if( !res || mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    debug( "event_artifact_wars_sql: No artifacts found." );
    return;
  }

  artilist = NULL;
  // First, build a list of those with multiple artifacts.
  // The list needs to be of a format: pid of char, then some sort of list of owned artifact vnum+type
  while( (row = mysql_fetch_row(res)) )
  {
    // Since locType is on PC or on PC corpse, location will always be the PID of the PC.
    pid = atoi(row[2]);
    vnum = atoi(row[0]);
    if( artilist == NULL )
    {
      artilist = new arti_list;
      artilist->pid = pid;
      artilist->next = NULL;
      artilist->artis = new arti_data;
      node = artilist->artis;
      node->vnum = vnum;
      node->owned = TRUE;
      node->locType = atoi(row[1]);
      node->location = pid;
      node->timer = atol(row[3]);
      node->type = atoi(row[4]);
      node->next = NULL;
    }
    else
    {
      nextlist = artilist;
      while( nextlist->pid != pid && nextlist->next != NULL )
      {
        nextlist = nextlist->next;
      }
      if( nextlist->pid == pid )
      {
        add_artidata_to_list( nextlist->artis, vnum, TRUE, atoi(row[1]), pid, atol(row[3]), atoi(row[4]) );
      }
      else
      {
        nextlist->next = new arti_list;
        nextlist = nextlist->next;
        nextlist->pid = pid;
        nextlist->next = NULL;
        nextlist->artis = new arti_data;
        node = nextlist->artis;
        node->vnum = vnum;
        node->owned = TRUE;
        node->locType = atoi(row[1]);
        node->location = pid;
        node->timer = atol(row[3]);
        node->type = atoi(row[4]);
        node->next = NULL;
      }
    }
  }
  mysql_free_result(res);

  // Second, decrement the timers on those with multiple of the same type.
  nextlist = artilist;
  while( nextlist )
  {
    // Count up the artis.
    count[0] = count[ARTIFACT_MAIN] = count[ARTIFACT_UNIQUE] = count[ARTIFACT_IOUN] = 0;
    node = nextlist->artis;
    while( node )
    {
      // Count[0] holds total number of artis.. to punish those who go 1 over with 4 artis.
      count[0]++;
      count[node->type]++;
      node = node->next;
    }
    // Count up how much over limit (1 of each is limit).
    punish_level = (count[ARTIFACT_MAIN] > 1) ? count[ARTIFACT_MAIN] - 1 : 0;
    punish_level += (count[ARTIFACT_UNIQUE] > 1) ? count[ARTIFACT_UNIQUE] - 1 : 0;
    punish_level += (count[ARTIFACT_IOUN] > 1) ? count[ARTIFACT_IOUN] - 1 : 0;
    // If they're in violation (more than one arti of the same type.
    if( punish_level > 0 )
    {
      // 1: 5min, 2: 15min, 3: 30min, 4: 64min, 5: 125min, 6: 216min, 7: 343min (happens every 30 min).
      // More than a punish_level of 3 is ridiculous though.
      switch( punish_level )
      {
        case 1:
            punishment = 300;
          break;
        case 2:
            punishment = 900;
          break;
        case 3:
            punishment = 1800;
          break;
        default:
          // x^3 minutes function.
          punishment = 60 * punish_level * punish_level * punish_level;
          break;
      }
      // 2 of the same type (and no others):  300 + 240 =  540 sec =  9min
      // 6 artis with two of each type     : 1800 + 720 = 2520 sec = 42min
      punishment += 120 * count[0];

      node = nextlist->artis;
      while( node )
      {
        arti = read_object( node->vnum, VIRTUAL );
        debug( "fight: '%s&n'%6d upset (%d/%d =%3d:%02d) with %s.",
          pad_ansi(arti->short_description, 35, TRUE).c_str(), node->vnum,
          punish_level, count[0], punishment/60, punishment%60, get_player_name_from_pid(node->location) );
        if( node->locType == ARTIFACT_ON_PC
          && (owner = get_player_from_name( get_player_name_from_pid(node->location) )) )
        {
          act("&+L$p &+Lseems very upset with you.&n", FALSE, owner, arti, 0, TO_CHAR);
        }
        extract_obj(arti, FALSE);
        qry("UPDATE artifacts SET timer = FROM_UNIXTIME(%lu) WHERE vnum = %d", node->timer - punishment, node->vnum );

        node = node->next;
      }
    }
    nextlist = nextlist->next;
  }

  // Third, free up the artilist object.
  while( artilist )
  {
    nextlist = artilist->next;
    node = artilist->artis;
    while( node )
    {
      next_node = node->next;
      delete node;
      node = next_node;
    }
    delete artilist;
    artilist = nextlist;
  }

  debug( "event_artifact_wars: ended." );

  add_event( event_artifact_wars_sql, 30 * 60 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0 );
}

void event_arti_hunt_sql(P_char ch, P_char victim, P_obj obj, void *data)
{
  if( !IS_ALIVE(ch) || !data )
  {
    statuslog( 56, "event_arti_hunt_sql: bad arg: ch '%s', data '%s'", ch ? J_NAME(ch) : "NULL", data != NULL ? (char *)data : "NULL" );
    debug( "event_arti_hunt_sql: bad arg: ch '%s', data '%s'", ch ? J_NAME(ch) : "NULL", data != NULL ? (char *)data : "NULL" );
    return;
  }

  arti_hunt_sql( ch, (char *)data );
}

// Searches through all pfiles with initial *arg for artis.
void arti_hunt_sql( P_char ch, char *arg )
{
  char           buf[MAX_STRING_LENGTH];
  char           dname[256];
  char           initial;
  int            count, wearloc;
  arti_data      artidata;
  struct dirent *dire;
  DIR           *dir;
  P_char         owner, mob;
  P_obj          arti, arti2;

  if( atoi(arg) == 1 )
  {
    // Search for a without delay.
    arti_hunt_sql( ch, "a" );
    // For the rest of the letters search for them with an incremented delay to prevent lag.
    for( initial = 'b', count = 1;initial <= 'z';initial++, count++ )
    {
      sprintf( buf, "%c", initial );
      add_event(event_arti_hunt_sql, count, ch, NULL, NULL, 0, &buf, sizeof(buf));
    }
    return;
  }
  if( !*arg || !isalpha(*arg) )
  {
    send_to_char( "Arti hunt needs a letter for which initial to hunt for, or 1 to search all pfiles.\n", ch );
    send_to_char( "Arti hunt will then search all pfiles with said initial for artifacts, and add them to the arti list if necessary.\n", ch );
    return;
  }
  if( *arg >= 'A' && *arg <= 'Z' )
  {
    *arg = *arg + 'a' - 'A';
  }

  // Read & loop through the directory..
  // Open the directory!
  sprintf( dname, "%s/%c", SAVE_DIR, *arg );
  dir = opendir( dname );
  if( !dir )
  {
    statuslog( 56, "hunt_for_artis: could not open arti dir (%s)\r\n", ARTIFACT_DIR );
    debug( "hunt_for_artis: could not open arti dir (%s)\r\n", ARTIFACT_DIR );
    return;
  }

  // Loop through the directory files.
  while (dire = readdir(dir))
  {
    // Skip backup/locker files/etc
    if( strstr( dire->d_name, "." ) )
      continue;

    if( (owner = load_dummy_char( dire->d_name )) == NULL )
    {
      sprintf( buf, "hunt_for_artis: %s has bad pfile.\n\r", dire->d_name );
      send_to_char( buf, ch );
      continue;
    }
    if( IS_TRUSTED( owner ) )
    {
      nuke_eq( owner );
      extract_char( owner );
      continue;
    }

    /* For debugging only.. gets spammy on live mud.
    sprintf( buf, "Hunting pfile of '%s'.\n", J_NAME(owner) );
    send_to_char( buf, ch );
    */

    // Search each pfile:
    // Search Worn equipment.
    for( wearloc = 0; wearloc < MAX_WEAR; wearloc++ )
    {
      arti = owner->equipment[wearloc];
      if( arti == NULL || !IS_ARTIFACT(arti) )
      {
        continue;
      }

      sprintf( buf, "%-12s has %s&n (%6d) : ", J_NAME(owner),
        pad_ansi(arti->short_description, 35, TRUE).c_str(), GET_OBJ_VNUM(arti) );
      send_to_char( buf, ch );

      if( !get_artifact_data_sql( GET_OBJ_VNUM(arti), &artidata ) )
      {
        send_to_char( "&+WNot yet tracked - adding.&n\n", ch );
        // If there's one in zone, pull it.
        if( (arti2 = artifact_find(GET_OBJ_VNUM(arti))) )
        {
          send_to_char( "&+WPulled artifact from zone.\n\r", ch );
          extract_obj(arti2);
        }
        // If they managed to get it on pfile and not in DB, give them full timer.
        artifact_update_sql( arti, 'Y', time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY );
      }
      else if( artidata.locType == ARTIFACT_ON_PC || artidata.locType == ARTIFACT_ONCORPSE )
      {
        if( artidata.location == GET_PID(owner) )
        {
          send_to_char( "Already tracked on char.\n", ch );
        }
        else
        {
          sprintf( buf, "&+ROn another char:&N %s\n", get_player_name_from_pid(artidata.location) );
          send_to_char( buf, ch );
        }
      }
      else if( artidata.locType == ARTIFACT_ON_NPC )
      {
        mob = read_mobile( artidata.location, VIRTUAL );
        sprintf( buf, "&+ROn a mob:&N '%s' %d.\n", J_NAME(mob), artidata.location );
        extract_char( mob );
        send_to_char( buf, ch );
        // If there's one in zone, pull it.
        if( (arti2 = artifact_find(GET_OBJ_VNUM(arti))) )
        {
          send_to_char( "&+WPulled artifact from zone.\n\r", ch );
          extract_obj(arti2);
        }
        artifact_update_location_sql( arti );
      }
      else if( artidata.locType == ARTIFACT_ONGROUND )
      {
        sprintf( buf, "&+ROn ground:&N '%s' %d.\n", world[real_room0(artidata.location)].name, artidata.location );
        send_to_char( buf, ch );
        // If there's one in zone, pull it.
        if( (arti2 = artifact_find(GET_OBJ_VNUM(arti))) )
        {
          send_to_char( "&+WPulled artifact from zone.\n\r", ch );
          extract_obj(arti2);
        }
        artifact_update_location_sql( arti );
      }
      else if( artidata.locType == ARTIFACT_NOTINGAME )
      {
        send_to_char( "&+WNot in game - updating.&n\n\r", ch );
        artifact_update_location_sql( arti );
      }
      else
      {
        send_to_char( "&+rUnknown location.&n\n\r", ch );
      }
    }
    // Search inventory.
    for( arti = owner->carrying; arti; arti = arti->next_content )
    {
      if( IS_ARTIFACT( arti ) )
      {
        sprintf( buf, "%-12s has %s&n (%6d) : ", J_NAME(owner),
          pad_ansi(arti->short_description, 35, TRUE).c_str(), obj_index[arti->R_num].virtual_number );
        send_to_char( buf, ch );
        if( !get_artifact_data_sql( GET_OBJ_VNUM(arti), &artidata ) )
        {
          send_to_char( "&+WNot yet tracked - adding.&n\n", ch );
          // If they managed to get it on pfile and not in DB, give them full timer.
          artifact_update_sql( arti, 'Y', time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY );
          // If there's one in zone, pull it.
          if( (arti2 = artifact_find(GET_OBJ_VNUM(arti))) )
          {
            send_to_char( "&+WPulled artifact from zone.\n\r", ch );
            extract_obj(arti2);
          }
        }
        else if( artidata.locType == ARTIFACT_ON_PC || artidata.locType == ARTIFACT_ONCORPSE )
        {
          if( artidata.location == GET_PID(owner) )
          {
            send_to_char( "Already tracked on char.\n", ch );
          }
          else
          {
            sprintf( buf, "&+ROn another char:&N %s\n", get_player_name_from_pid(artidata.location) );
            send_to_char( buf, ch );
          }
        }
        else if( artidata.locType == ARTIFACT_ON_NPC )
        {
          mob = read_mobile( artidata.location, VIRTUAL );
          sprintf( buf, "&+ROn a mob:&N '%s' %d.\n", J_NAME(mob), artidata.location );
          extract_char( mob );
          send_to_char( buf, ch );
        }
        else if( artidata.locType == ARTIFACT_ONGROUND )
        {
          sprintf( buf, "&+ROn ground:&N '%s' %d.\n", world[real_room0(artidata.location)].name, artidata.location );
          send_to_char( buf, ch );
        }
        else if( artidata.locType == ARTIFACT_NOTINGAME )
        {
          send_to_char( "&+rNot in game - updating.&n\n\r", ch );
          artifact_update_location_sql( arti );
        }
        else
        {
          send_to_char( "&+rUnknown location.&n\n\r", ch );
        }
      }
    }
    nuke_eq( owner );
    extract_char( owner );
  }
  // Close the directory!
  closedir( dir );
  sprintf( buf, "Arti hunted '%c' successfully!\n", *arg );
  send_to_char( buf, ch );
}

void arti_clear_sql( P_char ch, char *arg )
{
  char      buf[MAX_STRING_LENGTH];
  int       vnum;
  P_obj     arti;

  if( !*arg || !strcmp(arg, "?") || !strcmp(arg, "help") )
  {
    send_to_char( "This command clears the arti data for a specific arti vnum.\n\r", ch );
    send_to_char( "It should only be used by those who know what they're doing.\n\r", ch );
    send_to_char( "This will delete the entire entry from the Mortal and Immortal DB, be forewarned.\n\r", ch );
    return;
  }
  if( GET_LEVEL(ch) < FORGER )
  {
    send_to_char( "Maybe you could ask someone of higher level to do this.\n\r", ch );
    return;
  }

  if( (vnum = atoi(arg)) <= 0 )
  {
    send_to_char("The proper argument for this is the vnum of the arti for which you want the data cleared.\n\r", ch );
    return;
  }

  if( !(arti = read_object(vnum, VIRTUAL)) )
  {
    send_to_char("&+WThat's not a vnum for any object, wth?&n\n\r", ch );
    return;
  }
  if( IS_ARTIFACT(arti) )
  {
    act("$p &+Wis not an artifact.", FALSE, ch, arti, 0, TO_CHAR);
  }

  // Remove from artifacts table:
  if( qry( "DELETE FROM artifacts WHERE vnum = '%d'", vnum ) )
  {
    act("&+WThe artifact data for $p&+W has been cleared from the Immortal list.  You fool!", FALSE, ch, arti, 0, TO_CHAR);
    if( qry( "DELETE FROM artifacts_mortal WHERE vnum = '%d'", vnum ) )
    {
      act("&+WThe artifact data for $p&+W has been cleared from the Mortal list.  You fool!", FALSE, ch, arti, 0, TO_CHAR);
    }
    else
    {
      send_to_char("&+WFailed to remove entry from mortal DB.&n\n\r", ch );
    }
  }
  else
  {
    send_to_char("&+WFailed to remove entry from main DB.  wth?&n\n\r", ch );
  }
  extract_obj(arti);
}

// This function is used to poof an arti that's either in game or on a rented char.
// If the timer isn't ticking, this function won't do anything.
void arti_poof_sql( P_char ch, char *arg )
{
  char      buf[MAX_STRING_LENGTH], artishort[MAX_STRING_LENGTH];
  int       vnum;
  P_obj     arti;
  P_char    owner;
  arti_data artidata;

  if( !*arg || !strcmp(arg, "?") || !strcmp(arg, "help") )
  {
    send_to_char( "This command poofs the artifact specified by the vnum argument.\n\r", ch );
    send_to_char( "It should only be used by those who know what they're doing.\n\r", ch );
    send_to_char( "This will poof the artifact and reset the timer, be forewarned.\n\r", ch );
    return;
  }
  if( GET_LEVEL(ch) < FORGER )
  {
    send_to_char( "Maybe you could ask someone of higher level to do this.\n\r", ch );
    return;
  }

  if( (vnum = atoi(arg)) <= 0 )
  {
    send_to_char("The proper argument for this is the vnum of the arti for which you want the data cleared.\n\r", ch );
    return;
  }

  if( !(arti = read_object(vnum, VIRTUAL)) )
  {
    send_to_char("&+WThat's not a vnum for any object, wth?&n\n\r", ch );
    return;
  }
  if( !IS_ARTIFACT(arti) )
  {
    act("$p &+Wis not an artifact.", FALSE, ch, arti, 0, TO_CHAR);
    extract_obj(arti);
    return;
  }
  sprintf( artishort, "%s&n", OBJ_SHORT(arti) );
  extract_obj(arti);

  if( !get_artifact_data_sql(vnum, &artidata) )
  {
    sprintf( buf, "&+WThere is no data for %s atm; It shouldn't be in the game.&n\n\r", artishort );
    send_to_char( buf, ch );
    return;
  }
  owner = NULL;
  // If we can't find it in game.
  if( !(arti = artifact_find( artidata )) )
  {
    // If it isn't on a char, or the char is online.
    if( artidata.locType != ARTIFACT_ON_PC || is_pid_online(artidata.location, TRUE) )
    {
      sprintf( buf, "Could not find %s in game.\n\r", artishort );
      send_to_char( buf, ch );
      return;
    }
    if( !(owner = load_dummy_char( get_player_name_from_pid(artidata.location) )) )
    {
      sprintf(buf, "Could not load pfile of %s.\n\r", get_player_name_from_pid(artidata.location) );
      send_to_char( buf, ch );
      return;
    }
    if( (arti = get_object_from_char( owner, vnum )) == NULL )
    {
      sprintf(buf, "Strange, arti '%s' %d was not on %s's pfile!\n\r", artishort, vnum,
        get_player_name_from_pid(artidata.location));
      nuke_eq(owner);
      extract_char(owner);
      return;
    }
  }
  poof_artifact( arti );
  // If arti was on rented character.
  if( owner )
  {
    nuke_eq(owner);
    extract_char(owner);
  }
}

#define COMMAND_ADD 1
#define COMMAND_SUB 2
#define COMMAND_SET 3
// Changes the timer on an artifact.
void arti_timer_sql( P_char ch, char *arg )
{
  char buf[MAX_STRING_LENGTH], artishort[256];
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  int  minutes, cmd, vnum;
  time_t new_time;
  P_obj arti;
  arti_data artidata;

  arg = one_argument( arg, arg1 );
  arg = one_argument( arg, arg2 );
  arg = one_argument( arg, arg3 );

  // If they don't have 3 args, then we can't do anything, so show the help.
  if( !*arg3 )
  {
    send_to_char( "&+WFormat: &+wartifact timer <set|add|subtract> <vnum> <time in minutes>&+W.&n\n\r", ch );
    send_to_char( "This command changes the timer on the artifact specified by the vnum argument.\n\r", ch );
    send_to_char( "&+WIt should only be used by those who know what they're doing.&n\n\r", ch );
    send_to_char( "This will only work on artifacts who's timers are already ticking.\n\r", ch );
    return;
  }

  if( GET_LEVEL(ch) < GREATER_G )
  {
    send_to_char( "Maybe you could ask someone of higher level to do this.\n\r", ch );
    return;
  }

  // Handle arg1: the command.
  if( is_abbrev(arg1, "set") )
  {
    cmd = COMMAND_SET;
  }
  else if( is_abbrev(arg1, "add") )
  {
    cmd = COMMAND_ADD;
  }
  else if( is_abbrev(arg1, "subtract") )
  {
    cmd = COMMAND_SUB;
  }
  else
  {
    send_to_char( "&+WFormat: &+wartifact timer <set|add|subtract> <vnum> <time in minutes>&+W.&n\n\r", ch );
    sprintf( buf, "&+W'&+w%s&+W' is not a valid subcommand. Please choose set, add or subtract.&n\n\r", arg1 );
    send_to_char( buf, ch );
    return;
  }

  // Handle arg2: the vnum.
  if( (vnum = atoi(arg2)) <= 0 )
  {
    send_to_char( "&+WFormat: &+wartifact timer <set|add|subtract> <vnum> <time in minutes>&+W.&n\n\r", ch );
    sprintf( buf, "&+W'&+w%s&+W' is not a valid vnum.  Please use a positive number for the vnum.&n\n\r", arg2 );
    send_to_char( buf, ch );
    return;
  }
  if( (arti = read_object( vnum, VIRTUAL )) == NULL )
  {
    send_to_char( "&+WFormat: &+wartifact timer <set|add|subtract> <vnum> <time in minutes>&+W.&n\n\r", ch );
    sprintf( buf, "&+W'&+w%s&+W' is not the vnum of any object in the game.&n\n\r", arg2 );
    send_to_char( buf, ch );
    return;
  }
  if( !IS_ARTIFACT(arti) )
  {
    sprintf( buf, "&+W'&+w%s&+W' &+w%d&+W is not an artifact.  This command only works with artifacts.&n\n\r",
      OBJ_SHORT(arti), vnum );
    send_to_char( buf, ch );
    return;
  }
  sprintf( artishort, "%s&n", OBJ_SHORT(arti) );
  extract_obj(arti);

  // Handle arg3: the time in minutes.
  if( (minutes = atoi(arg3)) == 0 )
  {
    sprintf( buf, "&+W'%s' is not a positive number.  Please supply a positive number of minutes.&n\n\r", arg3 );
    send_to_char( buf, ch );
    return;
  }
  if( minutes < 0 )
  {
    if( !strcmp(arg1, "add") )
    {
      sprintf( buf, "&+WMaybe you should try '&+wartifact timer subtract %d %d&+W'.&n\n\r", vnum, minutes * -1 );
      send_to_char( buf, ch );
    }
    else if( !strcmp(arg1, "subtract") )
    {
      sprintf( buf, "&+WMaybe you should try '&+wartifact timer add %d %d&+W'.&n\n\r", vnum, minutes * -1 );
      send_to_char( buf, ch );
    }
    else
    {
      send_to_char("&+WPlease supply a positive number of minutes.&n\n\r", ch );
    }
    return;
  }

  // Now we have a valid command, the vnum of an arti, and a postitive number of minutes to change.
  if( !get_artifact_data_sql( vnum, &artidata ) )
  {
    sprintf( buf, "&+WHmm.. There's no timer ticking on '&+w%s&+W' &+w%d&+W.&n\n\r", artishort, vnum );
    send_to_char( buf, ch );
    return;
  }

  // Calculate the new time at which to poof.
  if( cmd == COMMAND_ADD )
  {
    new_time = artidata.timer + 60 * minutes;
  }
  else if( cmd == COMMAND_SUB )
  {
    new_time = artidata.timer - 60 * minutes;
  }
  else if( cmd == COMMAND_SET )
  {
    new_time = time(NULL) + 60 * minutes;
  }
  else
  {
    send_to_char( "&+WSomeone messed with this and created a faulty command. sorry.\n\r", ch );
    return;
  }

  // Minimum of 1 minute.
  if( new_time < time(NULL) )
  {
    send_to_char( "&+WNew time is less than &+w0&+W minutes, setting to &+w1&+W minute.&n\n\r", ch );
    new_time = time(NULL) + 60;
  }
  // And max.
  if( new_time > time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY )
  {
    send_to_char( "&+WOops, went a little over max, didn't ya?&n\n\r", ch );
    new_time = time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
  }

  sprintf( buf, "&+WArtifact '&+w%s&+W' &+w%d&+W has had it's timer changed from &n", artishort, vnum );
  artifact_timer_sql( vnum, buf + strlen(buf) );
  strcat( buf, "&+W to &n" );

  // Use the uber-generic update.
  artifact_update_sql( vnum, artidata.owned, artidata.locType, artidata.location, new_time, artidata.type );

  artifact_timer_sql( vnum, buf + strlen(buf) );
  strcat( buf, "&+W.&n\n\r" );

  send_to_char( buf, ch );
}

// This function is designed to swap out one arti for another.
void arti_swap_sql( P_char ch, char *arg )
{
  char buf[MAX_STRING_LENGTH];
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char artishort1[256];
  int  vnum1, vnum2, wearloc;
  bool found;
  P_obj arti1, arti2, cont;
  P_char owner1, dummy;
  arti_data artidata;

  arg = one_argument( arg, arg1 );
  arg = one_argument( arg, arg2 );

  // If they don't have 2 args, then we can't do anything, so show the help.
  if( !*arg2 )
  {
    send_to_char( "&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch );
    send_to_char( "This command changes one artifact for another.\n\r", ch );
    send_to_char( "&+WIt should only be used by those who know what they're doing.&n\n\r", ch );
    send_to_char( "This will only work if arti1 has a ticking timer and arti2 doesn't.\n\r", ch );
    return;
  }

  // Handle arg1: the vnum of the first arti.
  if( (vnum1 = atoi(arg1)) <= 0 )
  {
    send_to_char( "&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch );
    sprintf( buf, "&+W'&+w%s&+W' is not a valid vnum.  Please use a positive number for the vnum.&n\n\r", arg1 );
    send_to_char( buf, ch );
    return;
  }
  if( (arti1 = read_object( vnum1, VIRTUAL )) == NULL )
  {
    send_to_char( "&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch );
    sprintf( buf, "&+W'&+w%s&+W' is not the vnum of any object in the game.&n\n\r", arg1 );
    send_to_char( buf, ch );
    return;
  }
  if( !IS_ARTIFACT(arti1) )
  {
    sprintf( buf, "&+W'&+w%s&+W' &+w%d&+W is not an artifact.  This command only works with artifacts.&n\n\r",
      OBJ_SHORT(arti1), vnum1 );
    send_to_char( buf, ch );
    return;
  }
  sprintf( artishort1, "%s&n", OBJ_SHORT(arti1) );
  extract_obj(arti1);

  // Handle arg2: the vnum of the first arti.
  if( (vnum2 = atoi(arg2)) <= 0 )
  {
    send_to_char( "&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch );
    sprintf( buf, "&+W'&+w%s&+W' is not a valid vnum.  Please use a positive number for the vnum.&n\n\r", arg2 );
    send_to_char( buf, ch );
    return;
  }
  if( (arti2 = read_object( vnum2, VIRTUAL )) == NULL )
  {
    send_to_char( "&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch );
    sprintf( buf, "&+W'&+w%s&+W' is not the vnum of any object in the game.&n\n\r", arg2 );
    send_to_char( buf, ch );
    return;
  }
  if( !IS_ARTIFACT(arti2) )
  {
    sprintf( buf, "&+W'&+w%s&+W' &+w%d&+W is not an artifact.  This command only works with artifacts.&n\n\r",
      OBJ_SHORT(arti2), vnum2 );
    send_to_char( buf, ch );
    return;
  }
  // Do not pull arti2.. will use it.

  // Now we have 2 valid vnums and their corresponding short descriptions.
  if( !get_artifact_data_sql( vnum1, &artidata ) )
  {
    sprintf( buf, "&+WThere's no timer ticking on '&+w%s&+W' &+w%d&+W. That won't work.&n\n\r", artishort1, vnum1 );
    send_to_char( buf, ch );
    return;
  }
  // We just want to make sure it's not ticking.
  if( get_artifact_data_sql( vnum2, NULL ) )
  {
    sprintf( buf, "&+WThere's a timer ticking on '&+w%s&+W' &+w%d&+W.  That won't work.&n\n\r",
      OBJ_SHORT(arti2), vnum2 );
    send_to_char( buf, ch );
    return;
  }
  // If there's a copy of arti2 in zone, pull it.
  if( (arti1 = artifact_find(vnum2)) != NULL )
  {
    extract_obj(arti1);
    sprintf( buf, "&+WPulled artifact '&+w%s&+W' &+w%d&+W from zone.&n\n\r", OBJ_SHORT(arti2), vnum2 );
    send_to_char( buf, ch );
  }

  cont = NULL;
  for( arti1 = object_list; arti1; arti1 = arti1->next )
  {
    // Found it in game!
    if( GET_OBJ_VNUM(arti1) == vnum1 )
    {
      cont = arti1;
      while( OBJ_INSIDE(cont) && cont->loc.inside )
      {
        cont = cont->loc.inside;
      }
      found = FALSE;
      // Just make sure it's in a valid location.
      switch( cont->loc_p )
      {
        case LOC_CARRIED:
        case LOC_WORN:
          owner1 = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
          // Skip artis on Immortals.
          if( !IS_TRUSTED(owner1) )
          {
            found = TRUE;
          }
          break;
        case LOC_ROOM:
          found = TRUE;
          break;
        // Not in a valid location, so skip it.
        case LOC_INSIDE:
        case LOC_NOWHERE:
          break;
      }
      if( found )
      {
        break;
      }
    }
  }
  dummy = NULL;
  // If it's on a PC, and the owner isn't online.
  if( !found && artidata.locType == ARTIFACT_ON_PC && !is_pid_online(artidata.location, TRUE) )
  {
    // Try to find it on their pfile.
    dummy = load_dummy_char(get_player_name_from_pid( artidata.location ));
    if( (arti1 = get_object_from_char( dummy, vnum1 )) == NULL )
    {
      sprintf( buf, "&+WCould not find '&+w%s&+W' &+w%d&+W on &+w%s&+W's pfile.&n\n\r", artishort1, vnum1,
        get_player_name_from_pid( artidata.location ) );
      nuke_eq( dummy );
      extract_char( dummy );
      return;
    }
  }
  if( !arti1 )
  {
    sprintf( buf, "&+WStrange, could not find artifact '&+w%s&+W' &+w%d&+W anywhere?!?&n\n\r", artishort1, vnum1 );
    send_to_char( buf, ch );
    if( dummy )
    {
      nuke_eq( dummy );
      extract_char( dummy );
    }
    return;
  }
  // At this point, we've found arti1 in a valid location (I hope), and have arti2 ready for transfer.
  // Put arti2 in the right spot.
  switch( arti1->loc_p )
  {
    case LOC_CARRIED:
      owner1 = arti1->loc.carrying;
      obj_to_char( arti2, arti1->loc.carrying );
      break;
    case LOC_WORN:
      owner1 = cont->loc.wearing;
      // Find it on their body.
      for( wearloc = 0; wearloc < MAX_WEAR; wearloc++ )
      {
        // And move it to their inventory (Also replace with arti2).
        if( owner1->equipment[wearloc] == arti1 )
        {
          obj_to_char(unequip_char( owner1, wearloc ), owner1);
          equip_char( owner1, arti2, wearloc, TRUE );
        }
      }
      break;
    case LOC_ROOM:
      obj_to_room( arti2, arti1->loc.room );
      break;
    case LOC_INSIDE:
      obj_to_obj( arti2, arti1->loc.inside );
      break;
    // Not in a valid location, so skip it.
    case LOC_NOWHERE:
      sprintf( buf, "&+WStrange, artifact '&+w%s&+W' &+w%d&+W has a bad location?!?&n\n\r", artishort1, vnum1 );
      send_to_char( buf, ch );
      if( dummy )
      {
        nuke_eq( dummy );
        extract_char( dummy );
      }
      return;
      break;
  }
  // Since arti2 is in position, can pull arti1.
  extract_obj( arti1, TRUE ); // Yes, we want to remove arti1 from owned artis.
  // Updata artidata type with arti2 stats.
  // The timer and owned don't change.  Nor does the locType / location since we put it in the same spot arti1 was in.
  artidata.type = IS_IOUN(arti2) ? ARTIFACT_IOUN : (IS_UNIQUE(arti2) ? ARTIFACT_UNIQUE : ARTIFACT_MAIN);
  // Use the uber-generic update.
  artifact_update_sql( vnum2, artidata.owned, artidata.locType, artidata.location, artidata.timer, artidata.type );
  if( owner1 == dummy )
  {
    owner1 = NULL;
  }
  // Save pfile if applies.
  if( dummy )
  {
    writeCharacter(dummy, RENT_SWAPARTI, dummy->in_room);
    nuke_eq( dummy );
    extract_char( dummy );
  }
  // Save in-game owner if applies.
  if( owner1 )
  {
    sprintf( buf, "&+WYour %s&+W suddenly changes into %s&+W!&n\n\r",
      artishort1, OBJ_SHORT(arti2) );
    send_to_char( buf, owner1 );
    sprintf( buf, "&+W$n's %s&+W suddenly changes into %s&+W!&n\n\r",
      artishort1, OBJ_SHORT(arti2) );
    act( buf, FALSE, owner1, NULL, 0, TO_ROOM);
    writeCharacter(owner1, RENT_CRASH, owner1->in_room);
  }
  // Save corpse if applies.
  if( artidata.locType == ARTIFACT_ONCORPSE )
  {
    cont = arti2;
    while( OBJ_INSIDE(cont) && cont->loc.inside )
    {
      cont = cont->loc.inside;
      if( cont->type == ITEM_CORPSE && IS_SET(cont->value[CORPSE_FLAGS], PC_CORPSE) )
      {
        writeCorpse(cont);
        break;
      }
    }
  }
  if( OBJ_ROOM(arti2) )
  {
    sprintf( buf, "&+W%s&+W suddenly changes into %s&+W!&n\n\r",
      artishort1, OBJ_SHORT(arti2) );
    act( buf, FALSE, NULL, arti2, 0, TO_ROOM );
  }

  sprintf( buf, "&+WArtifact '&+w%s&+W' &+w%d&+W swapped with artifact '&+w%s&+W' &+w%d&+W.&n\n\r",
    artishort1, vnum1, OBJ_SHORT(arti2), vnum2 );
  send_to_char( buf, ch );
}

// This function walks through the artifact_bind table, gathers its info, then compares
//   it against where the arti is currently.  If it's not currently on the proper char
//   the timer is set to switch owners.  If the timer is up, then the soul switches owners.
void event_artifact_check_bind_sql( P_char ch, P_char vict, P_obj obj, void * arg )
{
  bind_data *bindData, *list;
  arti_data  artidata;
  P_char     owner;
  P_obj      arti;
  int        timer_length, counter;
  long       curr_time;
  MYSQL_RES *res;
  MYSQL_ROW  row = NULL;

  debug( "event_artifact_check_bind_sql(): beginning..." );

  if( !qry("select vnum, owner_pid, timer from artifact_bind") )
  {
    debug( "event_artifact_check_bind_sql(): Failed initial query." );
    logit(LOG_ARTIFACT, "event_artifact_check_bind_sql(): failed to read from database.");
    return;
  }

  res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
  {
    debug( "event_artifact_check_bind_sql(): 0 rows in artifact_bind." );
    logit(LOG_ARTIFACT, "event_artifact_check_bind_sql(): 0 rows in artifact_bind.");
    mysql_free_result(res);
    return;
  }

  timer_length = 60 * get_property("artifact.feeding.switch.lootallowance.min", 15);
  curr_time = time(NULL);

  bindData = list = NULL;
  while( row = mysql_fetch_row(res) )
  {
    bindData = new bind_data;
    bindData->vnum      = atoi(row[0]);
    bindData->owner_pid = atoi(row[1]);
    bindData->timer     = atol(row[2]);

    bindData->next = list;
    list = bindData;
    bindData = NULL;
  }

  mysql_free_result(res);

  counter = 0;
  while( list != NULL )
  {
    bindData = list->next;

    arti = read_object( list->vnum, VIRTUAL );
    if( get_artifact_data_sql( list->vnum, &artidata ) )
    {
      if( artidata.locType == ARTIFACT_ON_PC )
      {
        // If we're on a new owner.
        if( list->owner_pid != artidata.location )
        {
          // If the timer has expired
          if( list->timer + timer_length < curr_time )
          {
            // If the owner isn't online, the soul can not merge.
            if( owner = get_char_online( get_player_name_from_pid(artidata.location) ) )
            {
              act("&+L$p &+Lmerges with your &+wsoul&+L.", FALSE, owner, arti, 0, TO_CHAR);
              qry("UPDATE artifact_bind SET owner_pid = %d, timer = %ld WHERE vnum = %d", artidata.location, curr_time, list->vnum);
              logit(LOG_ARTIFACT, "event_artifact_check_bind_sql(): artifact '%s' %d merged with '%s' %d's soul.",
                arti ? OBJ_SHORT(arti) : "NULL", list->vnum, J_NAME(owner), artidata.location );
              debug( "%3d: artifact '%s&n'%6d merged with '%s' %d's soul.",
                ++counter, pad_ansi( arti ? OBJ_SHORT(arti) : "NULL", 35, TRUE).c_str(), list->vnum, J_NAME(owner), artidata.location );
            }
            else
            {
              debug( "%3d: '%s&n'%6d is ready, but '%s' %d not online.",
                ++counter, pad_ansi( arti ? OBJ_SHORT(arti) : "NULL", 35, TRUE).c_str(), list->vnum, get_player_name_from_pid(artidata.location), artidata.location );
            }
          }
          else if( list->timer > curr_time )
          {
            debug( "%3d: artifact '%s&n'%6d's timer is later than curr_time.",
              ++counter, pad_ansi( arti ? OBJ_SHORT(arti) : "NULL", 35, TRUE).c_str(), list->vnum, J_NAME(owner), artidata.location );
            qry("UPDATE artifact_bind SET owner_pid = %d, timer = %ld WHERE vnum = %d", artidata.location, curr_time, list->vnum);
          }
        }
      }
      // Display artis that are on the corpse of new owner.
      else if( artidata.locType == ARTIFACT_ONCORPSE && artidata.location != list->owner_pid )
      {
        debug( "%3d: artifact '%s&n'%6d on corpse of '%s' %d.",
          ++counter, pad_ansi( arti ? OBJ_SHORT(arti) : "NULL", 35, TRUE).c_str(), list->vnum,
          get_player_name_from_pid(artidata.location), artidata.location );
      }
    }
    else if( list->owner_pid > 0 )
    {
      logit(LOG_ARTIFACT, "event_artifact_check_bind_sql(): artifact '%s' %d is unowned, but bound.",
        arti ? OBJ_SHORT(arti) : "NULL", list->vnum );
      debug( "%3d: artifact '%s&n' %d is unowned, but bound.  Setting owner_pid = -1 and timer = 0.",
        ++counter, pad_ansi( arti ? OBJ_SHORT(arti) : "NULL", 35, TRUE).c_str(), list->vnum );
      qry("UPDATE artifact_bind SET owner_pid = -1, timer = 0 WHERE vnum = %d", list->vnum);
    }
    if( arti )
    {
      extract_obj( arti );
    }

    // Delete and move to next.
    list->next = NULL;
    delete list;
    list = bindData;
  }

  debug( "event_artifact_check_bind_sql(): completed." );
  // Checks every 7 minutes.
  add_event( event_artifact_check_bind_sql, 7 * 60 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0 );
}

// Resets the timers on artifacts that weren't properly bound.
void arti_fixit_sql( P_char ch )
{
  int        pid, timer, curr_time;
  int        vnum, location, counter;
  time_t     new_time;
  P_obj      arti;
  MYSQL_RES *res;
  MYSQL_ROW  row = NULL;

  if( !qry("SELECT vnum, location FROM artifacts WHERE locType='OnPC'") )
  {
    send_to_char( "Failed SELECT command.\n\r", ch );
    return;
  }

  res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
  {
    send_to_char( "Empty set; no artifacts on PC in table artifacts.\n\r", ch );
    return;
  }

  new_time = time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
  curr_time = (int) time(NULL);

  counter = 0;
  // Walk through each arti that's on a PC.
  while( (row = mysql_fetch_row(res)) )
  {
    vnum = atoi(row[0]);
    location = atoi(row[1]);
    sql_get_bind_data( vnum, &pid, &timer);
    timer = curr_time;
    arti = read_object( vnum, VIRTUAL );
    // If the arti is on a different PC, we want to update artifact_bind AND increase the timer to max in artifacts.
    if( location != pid )
    {
      sql_update_bind_data( vnum, &location, &timer);
      qry("UPDATE artifacts SET timer = FROM_UNIXTIME(%lu) WHERE vnum = %d", new_time, vnum);
      send_to_char_f( ch, "%3d) '%s&n'%6d - timer reset and now owned by '%s' %d.\n\r",
        ++counter, pad_ansi( arti ? OBJ_SHORT(arti) : "NULL", 35, TRUE).c_str(), vnum, get_player_name_from_pid(location), location );
    }
    extract_obj(arti);
  }
  mysql_free_result(res);
  if( counter == 0 )
  {
    send_to_char( "All artifact bind_data are up to date.\n\r", ch );
  }
}

// Resets the 'soul' of the artifact of vnum == arg.
// It resets the timer to 0 also, so it will merge asap.
void arti_reset_sql( P_char ch, char *arg )
{
  int vnum;

  if( !*arg || !strcmp(arg, "?") || !strcmp(arg, "help") )
  {
    send_to_char( "This command resets artifact bind data (The &+Lsoul&n of the artifact).\n\r", ch );
    send_to_char( "It should only be used by those who know what they're doing.\n\r", ch );
    send_to_char( "This will unmerge the soul from the owner of the artifact, but set the timer to 0,\n\r", ch );
    send_to_char( "  so it will merge as soon as the owner triggers artifact_switch_check.\n\r", ch );
    return;
  }
  if( GET_LEVEL(ch) < FORGER )
  {
    send_to_char( "Maybe you could ask someone of higher level to do this.\n\r", ch );
    return;
  }

  if( !strcmp( "fixit", arg ) )
  {
    arti_fixit_sql( ch );
    return;
  }

  if( isname( "all", arg ) )
  {
    if( GET_LEVEL(ch) < OVERLORD )
    {
      send_to_char( "Maybe you could ask someone of higher level to do this.\n\r", ch );
      return;
    }
    if( !isname( "confirm", arg ) )
    {
      send_to_char( "&+RThis will reset all the artifacts' souls!  If you want to do this, it requires &+wconfirmation&+R.&n\n\r", ch );
      send_to_char( "&=LRThis is probably a really bad idea!&n\n\r", ch );
      return;
    }
    vnum = -1;
  }
  else if( (vnum = atoi(arg)) <= 0 )
  {
    send_to_char( "This command requires a vnum for an argument.\n\r", ch );
    return;
  }
  if( vnum > 0 )
  {
    if( qry("UPDATE artifact_bind SET owner_pid = -1, timer = 0 WHERE vnum = %d", vnum) )
    {
      send_to_char_f( ch, "Artifact vnum %d has a hungry soul.\n\r", vnum );
    }
    else
    {
      send_to_char( "Update operation failed.\n\r", ch );
    }
  }
  else
  {
    if( qry("UPDATE artifact_bind SET owner_pid = -1, timer = 0") )
    {
      send_to_char( "All artifacts' souls are hungry for an owner now.\n\r", ch );
    }
    else
    {
      send_to_char( "Update operation failed.\n\r", ch );
    }
  }
}

// Returns the first mob in the game with said vnum.
P_char find_mob_in_game( int vnum )
{
  P_char mob;

  for( mob = character_list; mob; mob = mob->next )
  {
    if( IS_NPC(mob) && GET_VNUM(mob) == vnum )
    {
      return mob;
    }
  }

  return NULL;
}

// Loads the artis that were on a random mob and owned back into the boot.
void addOnMobArtis_sql()
{
  P_obj      arti;
  P_char     mob;
  MYSQL_RES *res;
  MYSQL_ROW  row;

  logit( LOG_ARTIFACT, "addOnMobArtis_sql: Beginning." );

  qry("SELECT vnum, location FROM artifacts WHERE owned='Y' AND locType='OnNPC'" );

  if( (res = mysql_store_result(DB)) != NULL )
  {
    if( mysql_num_rows(res) < 1 )
    {
      logit( LOG_ARTIFACT, "addOnMobArtis_sql: No owned artifacts found on NPCs." );
    }
    else
    {
      while( (row = mysql_fetch_row(res)) )
      {
        if( !(arti = read_object( atoi(row[0]), VIRTUAL )) )
        {
          logit( LOG_ARTIFACT, "addOnMobArtis_sql: Could not load object vnum %d.", atoi(row[0]) );
          continue;
        }
        if( !(mob = find_mob_in_game( atoi(row[1]) )) )
        {
          logit( LOG_ARTIFACT, "addOnMobArtis_sql: Could not find mob vnum %d.", atoi(row[1]) );
          extract_obj( arti );
          continue;
        }
        obj_to_char( arti, mob );
      }
    }
    mysql_free_result(res);
  }
  else
  {
    logit( LOG_ARTIFACT, "addOnMobArtis_sql: Could not pull on mob arti list." );
  }

  logit( LOG_ARTIFACT, "addOnMobArtis_sql: Ending." );
}

void arti_player_sql( P_char ch, char *arg )
{
  char  buf[MAX_STRING_LENGTH], locationBuf[MAX_STRING_LENGTH], timeBuf[128], *name;
  int   pid, vnum, locType, minutes, hours;
  long  totalTime;
  bool  shownData, negTime;
  P_obj arti;
  MYSQL_RES *res;
  MYSQL_ROW  row;

  if( (pid = atoi(arg)) < 1 )
  {
    if( (pid = get_player_pid_from_name(arg)) < 1 )
    {
      send_to_char( "The '&+wartifact player&n' command requires a valid player name or pid.\n\r", ch );
      return;
    }
  }
  if( (name = get_player_name_from_pid(pid)) == NULL )
  {
    sprintf( buf, "'%s' was not found to be a valid player name or pid.\n", arg );
    send_to_char( buf, ch );
    return;
  }

  sprintf(buf, "&+YOwner                  Time      Last Update           Artifact\r\n\r\n" );
  send_to_char( buf, ch );

  // locType is an enum type, so to get the values not the names, we want locType+0.
  if( !qry("SELECT vnum, locType+0, location, owned, UNIX_TIMESTAMP(timer), lastUpdate FROM artifacts WHERE location=%d", pid) )
  {
    send_to_char( "&+RError with query attempt.  Aborting...\n", ch );
    return;
  }

  if( !(res = mysql_store_result(DB)) )
  {
    send_to_char( "&+RError storing query result.  Aborting...\n", ch );
    return;
  }

  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    send_to_char( "No artifacts found.\n\r", ch );
    return;
  }

  shownData = FALSE;
  while( (row = mysql_fetch_row(res)) )
  {
    vnum    = atoi(row[0]);
    locType = atoi(row[1]);

    // In case there's on in the room with vnum == pid or such.
    if( locType != ARTIFACT_ON_PC && locType != ARTIFACT_ONCORPSE )
    {
      continue;
    }

    // Tryin' load a copy of the arti for display purposes.
    arti = read_object( vnum, VIRTUAL );
    if( !arti || !IS_ARTIFACT(arti) )
    {
      debug("list_artifacts_sql: Non artifact on arti list: '%s' %d.", (arti == NULL) ? "NULL" : arti->short_description, vnum );
      // Pull arti if it loaded.
      if( arti )
      {
        extract_obj( arti );
      }
      continue;
    }

    if( locType == ARTIFACT_ON_PC )
    {
      sprintf(locationBuf, "%-21s", name);
    }
    else if( locType == ARTIFACT_ONCORPSE )
    {
      sprintf( buf, "%s's corpse", name );
      sprintf( locationBuf, "%-21s", buf );
    }
    else
    {
      sprintf(buf, "&+RError reading query result.  Skipping... '%s' %d.\n", OBJ_SHORT(arti), GET_OBJ_VNUM(arti) );
      send_to_char( buf, ch );
      extract_obj( arti );
      continue;
    }

    negTime = FALSE;
    // totalTime (left to poof in sec) is the timer (time at which it poofs) - now.
    if( atol(row[4]) == 0 )
    {
      totalTime = 0;
    }
    if( (totalTime = atol(row[4]) - time(NULL)) < 0 )
    {
      negTime = TRUE;
      totalTime *= -1;
    }
    // Convert to minutes.
    totalTime /= 60;
    minutes = totalTime % 60;
    // Convert to hours.
    totalTime /= 60;
    hours = totalTime % 24;

    sprintf( timeBuf, "%c%2ld:%02d:%02d", negTime ? '-' : ' ', totalTime / 24, hours, minutes );

    sprintf(buf, "%s&n%-11s %-22s%s (#%d)\r\n", locationBuf, timeBuf, row[5], OBJ_SHORT(arti), vnum );
    send_to_char( buf, ch );
    shownData = TRUE;
    extract_obj( arti, FALSE );
  }
  mysql_free_result(res);
}
