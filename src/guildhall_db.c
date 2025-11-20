/*
 *  guildhalls_db.c
 *  Duris
 *
 *  Created by Torgal on 1/30/10.
 *
 */

#include "guildhall_db.h"

#include <vector>
#include <string>
using namespace std;

#include "guildhall.h"
#include "sql.h"
#include "utility.h"
#include "utils.h"
#include "prototypes.h"
#include "assocs.h"

extern vector<Guildhall*> guildhalls;
extern P_room world;

int _next_guildhall_id = -1;
int _next_guildhall_room_id = -1;
int _next_guildhall_room_vnum = -1;

int next_guildhall_id()
{
#ifndef __NO_MYSQL__
  if( _next_guildhall_id == -1 )
  {
    if(!qry("select max(id) from guildhalls"))
    {
      logit(LOG_GUILDHALLS, "next_guildhall_id(): query failed");
      return -1;
    }
    
    MYSQL_RES *res = mysql_store_result(DB);
    MYSQL_ROW row = mysql_fetch_row(res);
    
    if( !row[0] )
    {
      _next_guildhall_id = 0;
    }
    else
    {
      _next_guildhall_id = atoi(row[0]);
    }
    mysql_free_result(res);
  }
#endif
  _next_guildhall_id++;
  
  return _next_guildhall_id;
}

int next_guildhall_room_id()
{
#ifndef __NO_MYSQL__
  if( _next_guildhall_room_id == -1 )
  {
    if(!qry("select max(id) from guildhall_rooms"))
    {
      logit(LOG_GUILDHALLS, "next_guildhall_room_id(): query failed");
      return -1;
    }
    
    MYSQL_RES *res = mysql_store_result(DB);
    MYSQL_ROW row = mysql_fetch_row(res);

    if( !row[0] )
    {
      _next_guildhall_room_id = 0;
    }
    else
    {
      _next_guildhall_room_id = atoi(row[0]);
    }
    mysql_free_result(res);
  }
#endif
  
  _next_guildhall_room_id++;
  
  return _next_guildhall_room_id;
}

// find the next available vnum in the GH rooms block.
// returns -1 if none are available
int next_guildhall_room_vnum()
{
  for( int vnum = GH_START_VNUM; vnum < GH_END_VNUM; vnum++ )
  {
    // skip if vnum isn't valid room
    if( real_room(vnum) < 0 )
      continue;
    
    // skip if vnum room is already set as guild room
    if( IS_ROOM(real_room0(vnum), ROOM_GUILD) )
      continue;
    
    SET_BIT(world[real_room0(vnum)].room_flags, ROOM_GUILD);
    
    return vnum;
  }

  return -1;
}

void load_guildhalls(vector<Guildhall*>& guildhalls)
{
#ifndef __NO_MYSQL__
  if(!qry("select id, assoc_id, type, outside_vnum, racewar from guildhalls order by id asc"))
  {
    logit(LOG_GUILDHALLS, "load_guildhalls(): query failed");
    return;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res)))
  {
    Guildhall *gh = new Guildhall();
    gh->id = atoi(row[0]);
    gh->guild = get_guild_from_id(atoi(row[1]));
    gh->type = atoi(row[2]);
    gh->outside_vnum = atoi(row[3]);
    gh->racewar = atoi(row[4]);

    guildhalls.push_back(gh);
  }

  mysql_free_result(res);
#endif
}

void load_guildhall(int id, Guildhall *gh)
{
  if(!gh)
  {
    logit(LOG_GUILDHALLS, "load_guildhall(): invalid gh");
    return;
  }
  
  if( gh->id <= 0 )
  {
    logit(LOG_GUILDHALLS, "load_guildhall(%d): invalid id", id);
    return;
  }  

#ifndef __NO_MYSQL__
  if(!qry("select id, assoc_id, type, outside_vnum, racewar from guildhalls where id = %d", id))
  {
    logit(LOG_GUILDHALLS, "load_guildhall(%d): query failed", id);
    return;
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  MYSQL_ROW row;
  
  if( !(row = mysql_fetch_row(res)) )
  {
    logit(LOG_GUILDHALLS, "load_guildhall(%d): guildhall with id not found!", id);
    mysql_free_result(res);
    return;    
  }

  gh->id = atoi(row[0]);
  gh->guild = get_guild_from_id(atoi(row[1]));
  gh->type = atoi(row[2]);
  gh->outside_vnum = atoi(row[3]);
  gh->racewar = atoi(row[4]);
    
  mysql_free_result(res);  
#endif  
}

void load_guildhall_rooms(Guildhall *guildhall)
{
  if( !guildhall )
  {
    logit(LOG_GUILDHALLS, "read_guildhall_rooms(): invalid guildhall!");
    return;
  }

  if( guildhall->id <= 0 )
  {
    logit(LOG_GUILDHALLS, "read_guildhall_rooms(): invalid id (%d)", guildhall->id);
    return;
  }

#ifndef __NO_MYSQL__
  if(!qry("select id, vnum, guildhall_id, name, type, value0, value1, value2, value3, value4, value5, value6, value7, exit0, exit1, exit2, exit3, exit4, exit5, exit6, exit7, exit8, exit9 from guildhall_rooms where guildhall_id = %d order by vnum", guildhall->id))
  {
    logit(LOG_GUILDHALLS, "read_guildhall_rooms(): query failed");
    return;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res)))
  {
    GuildhallRoom *room = NULL;

    switch(atoi(row[4]))
    {
      case GH_ROOM_TYPE_ENTRANCE:
        room = new EntranceRoom();
        break;
      case GH_ROOM_TYPE_INN:
        room = new InnRoom();
        break;
      case GH_ROOM_TYPE_HEARTSTONE:
        room = new HeartstoneRoom();
        break;
      case GH_ROOM_TYPE_PORTAL:
        room = new PortalRoom();
        break;
      case GH_ROOM_TYPE_WINDOW:
        room = new WindowRoom();
        break;
      case GH_ROOM_TYPE_HEAL:
        room = new HealRoom();
        break;
      case GH_ROOM_TYPE_BANK:
        room = new BankRoom();
        break;
      case GH_ROOM_TYPE_TOWN_PORTAL:
        room = new TownPortalRoom();
        break;
      case GH_ROOM_TYPE_LIBRARY:
        room = new LibraryRoom();
        break;
      case GH_ROOM_TYPE_CARGO:
        room = new CargoRoom();
        break;
      default:
        room = new GuildhallRoom();
    }

    if( !room )
    {
      logit(LOG_GUILDHALLS, "load_guildhall_rooms(): couldn't allocate new guildhallroom!");
      mysql_free_result(res);
      return;
    }

    room->id = atoi(row[0]);
    room->vnum = atoi(row[1]);
    room->guild = guildhall->get_assoc();
    room->name = string(row[3]);
    room->type = atoi(row[4]);
    
    for( int i = 0; i < GH_ROOM_NUM_VALUES; i++ )
    {
      room->value[i] = atoi(row[5+i]);
    }
    
    for( int i = 0; i < NUM_EXITS; i++ )
    {
      room->exits[i] = atoi(row[5+GH_ROOM_NUM_VALUES+i]);
    }
    
    guildhall->add_room(room);
  }
  
  mysql_free_result(res);  
#endif  
}

bool save_guildhall(Guildhall *gh)
{
  if( !gh )
  {
    logit(LOG_GUILDHALLS, "save_guildhall(): invalid gh!");
    return FALSE; 
  }
  
  if(gh->outside_vnum < 0)
  {
    logit(LOG_GUILDHALLS, "save_guildhall(): invalid outside_vnum! (%d)", gh->outside_vnum);
    return FALSE;
  }  

  if(gh->id <= 0)
  {
    logit(LOG_GUILDHALLS, "save_guildhall(): invalid id! (%d)", gh->id);
    return FALSE; 
  }
  
#ifdef __NO_MYSQL__
  return TRUE;
#else  
  if(!qry("replace into guildhalls (id, assoc_id, type, outside_vnum, racewar) values (%d, %d, %d, %d, %d)",
          gh->id, gh->guild, gh->type, gh->outside_vnum, gh->racewar))
  {
    logit(LOG_GUILDHALLS, "save_guildhall(): replace query failed!");
    return FALSE;
  }
  
  return TRUE;
#endif
}

bool save_guildhall_room(GuildhallRoom *room)
{
  if( !room )
  {
    logit(LOG_GUILDHALLS, "save_guildhall_room(): invalid gh!");
    return FALSE; 
  }
  
  if(room->vnum < 0)
  {
    logit(LOG_GUILDHALLS, "save_guildhall_room(): invalid vnum! (%d)", room->vnum);
    return FALSE;
  }  
  
  if(room->id <= 0)
  {
    logit(LOG_GUILDHALLS, "save_guildhall_room(): invalid id! (%d)", room->id);
    return FALSE;    
  }
  
#ifdef __NO_MYSQL__
  return TRUE;
#else  
  if(!qry("replace into guildhall_rooms (id, vnum, guildhall_id, name, type, value0, value1, value2, value3, value4, value5, value6, value7, exit0, exit1, exit2, exit3, exit4, exit5, exit6, exit7, exit8, exit9) values (%d, %d, %d, '%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
          room->id,
          room->vnum, 
          room->guild->get_id(), 
          escape_str(room->name.c_str()).c_str(), 
          room->type, 
          room->value[0], 
          room->value[1], 
          room->value[2], 
          room->value[3], 
          room->value[4], 
          room->value[5], 
          room->value[6], 
          room->value[7], 
          room->exits[0], 
          room->exits[1], 
          room->exits[2], 
          room->exits[3], 
          room->exits[4], 
          room->exits[5], 
          room->exits[6], 
          room->exits[7], 
          room->exits[8], 
          room->exits[9]) )
  {
    logit(LOG_GUILDHALLS, "save_guildhall_room(): replace query failed!");
    return FALSE;
  }
  
  return TRUE;
#endif
}

bool delete_guildhall(Guildhall *gh)
{
  if( !gh )
  {
    logit(LOG_GUILDHALLS, "delete_guildhall(): invalid gh!");
    return FALSE; 
  }

  if(gh->id <= 0)
  {
    logit(LOG_GUILDHALLS, "delete_guildhall(): invalid id! (%d)", gh->id);
    return FALSE; 
  }
  
#ifdef __NO_MYSQL__
  return TRUE;
#else  
  if(!qry("delete from guildhalls where id = %d", gh->id))
  {
    logit(LOG_GUILDHALLS, "delete_guildhall(): delete query failed!");
    return FALSE;
  }
  
  return TRUE;
#endif
}

bool delete_guildhall_room(GuildhallRoom *room)
{
  if( !room )
  {
    logit(LOG_GUILDHALLS, "delete_guildhall_room(): invalid room!");
    return FALSE; 
  }
  
  if(room->id <= 0)
  {
    logit(LOG_GUILDHALLS, "delete_guildhall_room(): invalid id! (%d)", room->id);
    return FALSE;    
  }
  
#ifdef __NO_MYSQL__
  return TRUE;
#else  
  if(!qry("delete from guildhall_rooms where id = %d", room->id))
  {
    logit(LOG_GUILDHALLS, "delete_guildhall_room(): delete query failed!");
    return FALSE;
  }
  
  return TRUE;
#endif
}
