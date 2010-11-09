//
// Outposts and buildings are not attached to the mud, but is here in case someone wants to clean up and finish the code,
// which was started by Torgal in 2008 and then continued by Venthix in 2009.
// - Torgal 1/29/2010
//

#include "db.h"
#include "structs.h"
#include "prototypes.h"
#include "buildings.h"
#include "spells.h"
#include "utils.h"
#include "interp.h"
#include "comm.h"
#include "strings.h"
#include "defines.h"
#include "alliances.h"
#include "assocs.h"
#include "outposts.h"

extern P_room world;
extern P_index mob_index;

vector<Building*> buildings;

// basic information for building types:
// building type, building mob vnum, required wood, required stone, hitpoints, generator function, 
BuildingType building_types[] = {
  {BUILDING_OUTPOST, BUILDING_OUTPOST_MOB, 100000, 10000, 300000, outpost_generate},
  {0}
};

P_room get_unused_building_room()
{
  for( int i = BUILDING_START_ROOM; i <= BUILDING_END_ROOM; i++ )
  {
    int rroom = real_room0(i);
    if( rroom > 0 && world[rroom].funct == NULL )
      return &world[rroom];
  }

  return NULL;
}

BuildingType get_type(int type)
{
  int i;
  for( i = 0; building_types[i].type; i++ )
  {
    if( building_types[i].type == type )
      break;
  }

  return building_types[i];
}

Building* get_building_from_id(int id)
{
  for( int i = 0; i < buildings.size(); i++ )
  {
    if( !buildings[i] )
      continue;
    
    if( buildings[i]->id == id )
      return buildings[i];
    
  }
  
  return NULL;
}

Building* get_building_from_room(int rroom)
{
  int vnum = world[rroom].number;
  
  for( int i = 0; i < buildings.size(); i++ )
  {
    if( !buildings[i] )
      continue;
    
    for( int j = 0; j < buildings[i]->rooms.size(); j++ )
    {
      if( buildings[i]->rooms[j]->number == vnum )
        return buildings[i];
    }
    
  }
  
  return NULL;
}

Building* get_building_from_gateguard(P_char ch)
{
  if( !affected_by_spell(ch, TAG_GUILDHALL) )
    return NULL;

  struct affected_type *af = NULL, *hjp;
  for (hjp = ch->affected; hjp; hjp = hjp->next)
  {
    if (hjp->type == TAG_GUILDHALL)
    {
      af = hjp;
      break;
    }
  }
  if (!af)
    return NULL;

  for( int i = 0; i < buildings.size(); i++ )
  {
    if( buildings[i] && buildings[i]->id == af->modifier )
      return buildings[i];
  }
  
  return NULL;
}

Building* get_building_from_rubble(P_obj rubble)
{
  if (!rubble)
  {
    debug("called get_building_from_rubble with no obj");
    return NULL;
  }

  for( int i = 0; i < buildings.size(); i++ )
  {
    if( buildings[i] && buildings[i]->id == rubble->value[3] )
      return buildings[i];
  }

  return NULL;
}

Building* get_building_from_char(P_char ch)
{
  if( !affected_by_spell(ch, TAG_BUILDING) )
    return NULL;
  
  struct affected_type *af = NULL, *hjp;
  for (hjp = ch->affected; hjp; hjp = hjp->next)
  {
    if (hjp->type == TAG_BUILDING)
    {  
      af = hjp;
      break;
    }
  }
  if (!af)
    return NULL;

  for( int i = 0; i < buildings.size(); i++ )
  {
    if( buildings[i] && buildings[i]->id == af->modifier )
      return buildings[i];
  }
  
  return NULL;
}

int initialize_buildings()
{
  for( int i = BUILDING_START_ROOM; i <= BUILDING_END_ROOM; i++ )
  {
  if( real_room0(i) )
      world[real_room0(i)].funct = NULL;
  }
  
  // load buildings
  
  return TRUE;
}

Building* load_building(int guildid, int type, int location, int level)
{
  if (!type || !location)
  {
    debug("Failed to call load_building() with valid values passed.");
    return NULL;
  }

  Building* building = new Building(guildid, type, location, level);

  if (building->loaded)
    buildings.push_back(building);

  return building;
}

void do_build(P_char ch, char *argument, int cmd)
{
  if( !IS_TRUSTED(ch) )
  {
    send_to_char("Sorry?\r\n", ch);
    return;    
  }
  
  // TODO: implement buildings
  char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int level;

  one_argument(argument, arg);

  if (*arg && isdigit(*arg))
  {
    level = atoi(arg);
  }
  else
    level = 1;
  
  Building* building = new Building(0, 1, ROOM_VNUM(ch->in_room), level);

  if( building->loaded )
    buildings.push_back(building);
}


// Called in place of die() for outposts mobs
int check_outpost_death(P_char ch, P_char killer)
{
  if (!ch || !killer)
    return FALSE;
  
  struct affected_type *af, *next_af;
  Building* building = get_building_from_char(ch);

  if (!building)
    return FALSE;
  
  if( !affected_by_spell(ch, TAG_BUILDING) )
    return FALSE;
  
  act("&+W$n has been destroyed!&n", TRUE, ch, 0, 0, TO_ROOM);
  
  if (ch->specials.fighting)
    stop_fighting(ch);
  
  StopAllAttackers(ch);

  REMOVE_BIT(ch->specials.act2, PLR2_WAIT);

  for (af = ch->affected; af; af = next_af)
  {
    next_af = af->next;
    if (!(af->flags & AFFTYPE_PERM) && (af->type != TAG_BUILDING))
    {
      affect_remove(ch, af);
    }
  }
  
  ClearCharEvents(ch);

  ch->specials.conditions[DISEASE_TYPE] = 0;
  ch->specials.conditions[POISON_TYPE] = 0;

  SET_POS(ch, POS_STANDING + STAT_NORMAL);
  
  if (IS_NPC(ch) &&
      (ch->specials.act & ACT_SPEC_DIE) &&
      (ch->specials.act & ACT_SPEC))
  {
    if (!mob_index[GET_RNUM(ch)].func.mob)
    {
      debug("Error, outpost %d contains no mob spec function", building->id);
      return TRUE;
    }
    else if (ch->in_room == NOWHERE)
    {
      debug("Outpost being attacked while in NOWHERE, not passing CMD_DEATH");
      GET_HIT(ch) = 0;
      set_current_outpost_hitpoints(building);
      return TRUE;
    }
    else
    {
      (*mob_index[GET_RNUM(ch)].func.mob) (ch, killer, CMD_DEATH, 0);
    }
  }
  else
  {
    debug("Error, outpost %d contains invalid ACT_SPEC info", building->id);
    return TRUE;
  }

  return TRUE;
}

int building_mob_proc(P_char ch, P_char pl, int cmd, char *arg)
{
  struct alliance_data *alliance;
  int allow = FALSE;

  if( !ch )
    return FALSE;

  if( cmd == CMD_SET_PERIODIC )
  {
    SET_BIT(ch->specials.act, ACT_SPEC_DIE);
    return TRUE;
  }

  if( !affected_by_spell(ch, TAG_BUILDING) )
    return FALSE;
  
  Building *building = get_building_from_char(ch);
  
  if( !building )
    return FALSE;

  if( building->mob_proc && (*building->mob_proc)(ch, pl, cmd, arg) )
    return TRUE;

  set_current_outpost_hitpoints(building);

  if( cmd == CMD_DEATH)
  {
    if (pl)
    {
      debug("Death blow to outpost delivered by: %s", GET_NAME(pl));
      outpost_death(ch, pl);
      return FALSE;
    }
    else
    {
      debug("Crap, someone's killed an outpost and their player_data struct has not been passed along!");
      return FALSE;
    }
  }

  if( cmd == CMD_ENTER )
  {
    // Guild and Alliance checks
    if (IS_TRUSTED(pl) ||
	(GET_A_NUM(pl) == building->guild_id) ||
        // Build command, or neutral outposts, no owner yet...
	(building->guild_id == 0))
      allow = TRUE;
    else if ((alliance = get_alliance(building->guild_id)) && 
            ((GET_A_NUM(pl) == alliance->forging_assoc_id) ||
             (GET_A_NUM(pl) == alliance->joining_assoc_id)))
      allow = TRUE;

    if (!allow && pl->group)
    {
      struct group_list *gl;
      gl = pl->group;
      while (gl)
      {
	if (GET_A_NUM(gl->ch) == building->guild_id)
	  allow = TRUE;
        gl = gl->next;
      }
    }
    
    if (allow && IS_FIGHTING(pl))
    {
      act("&+WYou cannot enter a guildhall in combat!", FALSE, pl, 0, 0, TO_CHAR);
      return TRUE;
    }

    if (!allow)
    {
      send_to_char("You don't own this outpost!\r\n", pl);
      return TRUE;
    }

    P_char tmob;
    if( !(tmob = get_char_room_vis(pl, arg)) || tmob != ch )
      return FALSE;
           
    if( !building->rooms[0]->number )
      return FALSE;
  
    act("You enter $N.", TRUE, pl, 0, tmob, TO_CHAR);
    act("$n enters $N.", TRUE, pl, 0, tmob, TO_NOTVICT);

    char_from_room(pl);
    char_to_room(pl, building->gate_room(), -1);
    return TRUE;
  }
  
  return FALSE;
}

// Building
int Building::next_id = 1;

Building::Building() : guild_id(0), type(0), room_vnum(0), level(0), loaded(false), mob_proc(NULL) {}

Building::Building(int _guild_id, int _type, int _room_vnum, int _level) : 
  guild_id(_guild_id), type(_type), room_vnum(_room_vnum), level(_level), loaded(false), mob_proc(NULL)
{
    load();
}

Building::~Building()
{
  unload();
}

// actually load 
int Building::load()
{
  if( !type )
    return FALSE;
  
  if( !location() )
    return FALSE;

  if( !level )
    return FALSE;
  
  // instantiate mob, run generator function, place on map
  BuildingType building_type = get_type(type);
  
  if( !building_type.type )
    return FALSE;  
  
  mob = read_mobile(building_type.mob_vnum, VIRTUAL);
  //if (building_type.type == BUILDING_OUTPOST)
    //mob_index[real_mobile0(building_type.mob_vnum)].func.mob = building_mob_proc;

  if( !mob )
    return FALSE;
  
  // Setting building.id before we run the generator function since we need this value to
  // set appropriate hitpoints
  id = Building::next_id++;
  
  if( !(*building_type.generator)(this) )
    return FALSE;
  
  portal_op = NULL;
  portal_gh = NULL;
  for (int i = 0; i < MAX_OUTPOST_GATEGUARDS; i++)
    golems[i] = NULL;
  golem_room = 0;
  golem_dir = 0;

  struct affected_type af;
  bzero(&af, sizeof(af));
  
  af.type = TAG_BUILDING;
  af.modifier = id;
  af.duration = -1;
 
  affect_to_char(mob, &af);  
  char_to_room(mob, location(), -1);
  
  act("$n is built.", FALSE, mob, 0, 0, TO_ROOM);
  
  loaded = true;
  return TRUE;
}

int Building::unload()
{
  // deinstantiate object, rooms, etc, move all players, corpses, and non building objects to map
  
  while( rooms.size() )
  {
    P_room room = rooms.back();
    rooms.pop_back();
    
    if( !room )
      continue;
    
    // reset proc so we know the room is free
    world[real_room0(room->number)].funct = NULL;
    
    // reset room exits
    for( int j = 0; j < NUM_EXITS; j++ )
    {
      if( !room->dir_option[j] )
        continue;
      
      FREE(room->dir_option[j]);
      room->dir_option[j] = NULL;
    }
    
    // dump players outside
    P_char tch, tch_next;
    for( tch = room->people; tch; tch = tch_next )
    {
      tch_next = tch->next_in_room;
      char_from_room(tch);
      char_to_room(tch, location(), -1);      
    }
    
    // dump objects outside
    P_obj tobj, tobj_next;
    for( tobj = room->contents; tobj; tobj = tobj_next )
    {
      tobj_next = tobj->next_content;
      obj_from_room(tobj);
      obj_to_room(tobj, location());      
    }
  }
    
  extract_char(mob);  
  
  loaded = false;
  return TRUE;
}


//
// Individual building procs
//

// OUTPOST

// generate rooms, exits, set procs, etc
int outpost_generate(Building* building)
{
  if( !building )
    return FALSE;
    
  // set mob stuff
  if (get_current_outpost_hitpoints(building) > 0)
  {
    GET_MAX_HIT(building->mob) = building->mob->points.base_hit = building_types[BUILDING_OUTPOST-1].hitpoints;
    GET_HIT(building->mob) = get_current_outpost_hitpoints(building);
  }
  else // It SHOULD be a new outpost...
  {
    GET_MAX_HIT(building->mob) = GET_HIT(building->mob) = building->mob->points.base_hit = (int)(building_types[BUILDING_OUTPOST-1].hitpoints);
  }
  set_current_outpost_hitpoints(building);  
  GET_RACE(building->mob) = RACE_CONSTRUCT;
  SET_BIT(building->mob->specials.affected_by2, AFF2_MAJOR_PARALYSIS);
  SET_BIT(building->mob->specials.act, ACT_SPEC);
  building->mob_proc = outpost_mob;
  
  // generate rooms
  P_room room = get_unused_building_room();
  
  if( !room )
    return FALSE;
  
  building->rooms.push_back(room);
  room->funct = outpost_inside;
  
  room->name = str_dup("&+LInside an &+yOutpost Tower");
  
  CREATE(room->dir_option[DOWN], room_direction_data, 1, MEM_TAG_DIRDATA);
  
  room->dir_option[DOWN]->to_room = building->location();
  room->dir_option[DOWN]->exit_info = 0;
  
  room->funct = outpost_inside;

  return TRUE;
}

// mob proc for outpost
int outpost_mob(P_char ch, P_char pl, int cmd, char *arg)
{
  if( !ch )
    return FALSE;
  
  if( cmd == CMD_MOB_MUNDANE )
  {
    //mobsay(ch, "&+Cmundane");
    return TRUE;
  }
  
  return FALSE;
}

int outpost_inside(int room, P_char ch, int cmd, char *arg)
{
  char buff[MAX_STRING_LENGTH];
  int old_room = 0;

  if (!room || !ch || !cmd)
  {
    debug("outpost_inside(): called with invalid value passed to function");
    return FALSE;
  }

  if (cmd == CMD_LOOK)
  {
    one_argument(arg, buff);
    if (isname(" out", buff))
    {
      if (old_room = ch->in_room)
      {
        Building* building = get_building_from_room(ch->in_room);
	int z = building->level;
	char_from_room(ch);
	ch->specials.z_cord = z;
        char_to_room(ch, world[old_room].dir_option[DOWN]->to_room, -1);
        char_from_room(ch);
        ch->specials.z_cord = 0;
        char_to_room(ch, old_room, -2);
        return TRUE;
      }
    }
  }
  return FALSE;
}

