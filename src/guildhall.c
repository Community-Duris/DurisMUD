/*
 *  guildhalls.c
 *  Duris
 *
 *  Created by Torgal on 1/29/10.
 *
 */

#include "guildhall.h"
#include "guildhall_db.h"
#include "utility.h"
#include "prototypes.h"
#include "utils.h"
#include "db.h"
#include "assocs.h"
#include "specs.prototypes.h"
#include "spells.h"

// global variables

extern P_room world;
extern P_index obj_index;
extern P_index mob_index;

vector<Guildhall*> Guildhall::guildhalls;

void Guildhall::initialize()
{
  obj_index[real_object0(GH_DOOR_VNUM)].func.obj = guildhall_door;
  obj_index[real_object0(GH_WINDOW_VNUM)].func.obj = guildhall_window;
  obj_index[real_object0(GH_HEARTSTONE_VNUM)].func.obj = guildhall_heartstone;
  obj_index[real_object0(GH_CARGO_BOARD_VNUM)].func.obj = guildhall_cargo_board;
  mob_index[real_mobile0(GH_GOLEM_WARRIOR)].func.mob = guildhall_golem;
  mob_index[real_mobile0(GH_GOLEM_CLERIC)].func.mob = guildhall_golem;
  mob_index[real_mobile0(GH_GOLEM_SORCERER)].func.mob = guildhall_golem;
  
  // load guildhalls from DB
  load_guildhalls(guildhalls);
  
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    load_guildhall_rooms(guildhalls[i]);
    guildhalls[i]->init();
  }    
}

void Guildhall::shutdown()
{
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    guildhalls[i]->save();
  }  
  
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    delete(guildhalls[i]);
  }    
}

void Guildhall::add(Guildhall* gh)
{
  if(!gh)
    return;
  
  guildhalls.push_back(gh);
}

bool Guildhall::reload()
{
  if( !this->deinit() )
    return FALSE;
  
  this->clear_rooms();
  load_guildhall(this->id, this);  
  load_guildhall_rooms(this);
    
  return this->init();  
}

void Guildhall::remove(Guildhall* gh)
{
  if(!gh)
    return;
  
  gh->deinit();
  gh->destroy();
    
  for( vector<Guildhall*>::iterator it = guildhalls.begin(); it != guildhalls.end(); it++ )
  {
    if( (*it) == gh )
    {
      guildhalls.erase(it);
      break;
    }
  }
  
  delete(gh);
}  

//
// finds the Guildhall with id
//
Guildhall* Guildhall::find_by_id(int id)
{
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    if( guildhalls[i] && guildhalls[i]->id == id )
    {
      debug("returning %d from find_by_id", i);
      return guildhalls[i];
    }
  }
  debug("failed to return a guildhall in guildhall.c find_by_id");
  return NULL;
}

Guildhall* Guildhall::find_by_assoc_id(int id)
{
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    if( guildhalls[i] && guildhalls[i]->assoc_id == id )
    {
      return guildhalls[i];
    }
  }
  debug("failed to return a guildhall in guildhall.c find_by_assoc_id");
  return NULL;
}

//
//  finds the Guildhall that ch is currently inside of
//
Guildhall* Guildhall::find_by_vnum(int vnum)
{
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    if( !guildhalls[i] )
      continue;
    
    for( int j = 0; j < guildhalls[i]->rooms.size(); j++ ) 
    {
      if( !guildhalls[i]->rooms[j] )
        continue;
      
      if( guildhalls[i]->rooms[j]->vnum == vnum )
        return guildhalls[i];
    }
  }
  return NULL;
}

//
// finds the Guildhall by the outside vnum
//
Guildhall* Guildhall::find_by_outside_vnum(int vnum)
{
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    if( guildhalls[i] && guildhalls[i]->outside_vnum == vnum )
      return guildhalls[i];
  }
  return NULL;  
}

//
// finds Guildhall by the ch's guildhall tag
//
Guildhall* Guildhall::find_from_ch(P_char ch)
{
  for (struct affected_type *afp = ch->affected; afp; afp = afp->next)
  {
    if (afp->type == TAG_GUILDHALL)
    {
      return Guildhall::find_by_id(afp->modifier);
    }
  }
  return NULL;  
}

//
// finds GuildhallRoom with id
//
GuildhallRoom* Guildhall::find_room_by_id(int id)
{
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    if( !guildhalls[i] )
      continue;
    
    for( int j = 0; j < guildhalls[i]->rooms.size(); j++ )
    {
      if( guildhalls[i]->rooms[j] && guildhalls[i]->rooms[j]->id == id )
        return guildhalls[i]->rooms[j];
    }
    
  }
  return NULL;  
}

//
//  finds GuildhallRoom from current vnum
//
GuildhallRoom* Guildhall::find_room_by_vnum(int vnum)
{
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    if( !guildhalls[i] )
      continue;
    
    for( int j = 0; j < guildhalls[i]->rooms.size(); j++ )
    {
      if( guildhalls[i]->rooms[j] && guildhalls[i]->rooms[j]->vnum == vnum )
        return guildhalls[i]->rooms[j];
    }
    
  }
  return NULL;  
}

//
// finds a library room from vnum, if any
//
LibraryRoom* Guildhall::find_library_by_vnum(int vnum)
{
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    if( !guildhalls[i] )
      continue;
    
    for( int j = 0; j < guildhalls[i]->rooms.size(); j++ )
    {
      if( guildhalls[i]->rooms[j] && guildhalls[i]->rooms[j]->type == GH_ROOM_TYPE_LIBRARY && guildhalls[i]->rooms[j]->vnum == vnum )
        return (LibraryRoom *) guildhalls[i]->rooms[j];
    }
    
  }
  return NULL;  
}

//
// returns the count by association id of a certain GH type
//
int Guildhall::count_by_assoc_id(int assoc_id, int type)
{
  int count = 0;
  
  for( int i = 0; i < guildhalls.size(); i++ )
  {
    if( !guildhalls[i] )
      continue;
    
    if( guildhalls[i]->assoc_id == assoc_id && guildhalls[i]->type == type )
      count++;
    
  }
  return count;
}


//
// Guildhall methods
//
//

bool Guildhall::save()
{
  if( !this->valid() )
  {
    logit(LOG_GUILDHALLS, "Guildhall::save(%d): invalid!", this->id);
    return FALSE;
  }
  
  if( !save_guildhall(this) )
  {
    logit(LOG_GUILDHALLS, "Guildhall::save(%d): save_guildhall failed!", this->id);
    return FALSE;
  }

  // also save rooms
  for( int i = 0; i < this->rooms.size(); i++ )
  {
    this->rooms[i]->save();
  }    
  
  return TRUE;
}

bool Guildhall::destroy()
{
  if( !delete_guildhall(this) )
  {
    logit(LOG_GUILDHALLS, "Guildhall::destroy(%d): delete_guildhall failed!", this->id);
    return FALSE;
  }
  
  for( int i = 0; i < this->rooms.size(); i++ )
  {
    this->rooms[i]->destroy();
  }
  
  return TRUE;
}

bool Guildhall::can_add_room()
{
  return (this->rooms.size()+1 <= this->max_rooms);
}

void Guildhall::add_room(GuildhallRoom* room)
{
  if( !room )
  {
    logit(LOG_GUILDHALLS, "Guildhall::add_room(): invalid room!");
    return;
  }
  
  room->guildhall_id = this->id;
  room->guildhall = this;
  this->rooms.push_back(room);
}

bool Guildhall::init()
{
  // guildhall initialization
  // TODO: set up upkeep events
  
  // initialize rooms
  for( int i = 0; i < this->rooms.size(); i++ )
  {
    if( !this->rooms[i]->init() )
    {
      logit(LOG_GUILDHALLS, "Guildhall::init(%d): room %d init failed!", this->id, this->rooms[i]->id);
      return FALSE;
    }
  }
  
  return TRUE;
}

bool Guildhall::deinit()
{
  // guildhall deinitialization
  // TODO: remove upkeep events
  
  // deinitialize rooms
  for( int i = 0; i < this->rooms.size(); i++ )
  {
    if( !this->rooms[i]->deinit() )
    {
      logit(LOG_GUILDHALLS, "Guildhall::deinit(%d): room %d deinit failed!", this->id, this->rooms[i]->id);
      return FALSE;
    }
  }

  return TRUE;
}

bool Guildhall::valid()
{
  // initialize *rooms array and run init on each room
  for( int i = 0; i < this->rooms.size(); i++ )
  {
    GuildhallRoom *room = this->rooms[i];
    
    if( !room )
    {
      logit(LOG_GUILDHALLS, "Guildhall::valid(%d): invalid room pointer!");
      return FALSE;
    }
    
    if( !room->valid() )
    {
      logit(LOG_GUILDHALLS, "Guildhall::valid(%d): invalid room! (%d)", room->id);
      return FALSE;      
    }    
  }  
  
  if( this->rooms.size() > this->max_rooms )
  {
    logit(LOG_GUILDHALLS, "Guildhall::init(%d): too many rooms! (%d, max %d)", this->id, this->rooms.size(), this->max_rooms);
    return FALSE;
  }
  
  return TRUE;
}

void Guildhall::golem_died(P_char golem)
{
  if( !this->entrance_room )
    return;
  
  if( this->entrance_room->golem_died(golem) )
  {
    this->save();
  }
}
