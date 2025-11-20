//
// Outposts and buildings are not attached to the mud, but is here in case someone wants to clean up and finish the code,
// which was started by Torgal in 2008 and then continued by Venthix in 2009.
// - Torgal 1/29/2010
//

#include <string.h>
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
#include "events.h"
#include "ctf.h"
#include "sql.h"
#include "guildhall.h"

extern P_room world;
extern const int top_of_world;
extern P_index mob_index;
extern P_event current_event;
extern const char *get_event_name(P_event);
extern const int rev_dir[];

vector<Building*> buildings;

// basic information for building types:
// building type, building mob vnum, required wood, required stone, hitpoints, generator function
BuildingType building_types[] = {
  {BUILDING_OUTPOST, OUTPOST_BUILDING_MOB, 100000, 10000, 300000, outpost_generate},
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

    if( buildings[i]->get_id() == id )
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

    for( int j = 0; j < buildings[i]->size(); j++ )
    {
      if( buildings[i]->get_room(j)->number == vnum )
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
    if( buildings[i] && buildings[i]->get_id() == af->modifier )
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
    if( buildings[i] && buildings[i]->get_id() == rubble->value[3] )
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
    if( buildings[i] && buildings[i]->get_id() == af->modifier )
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

Building* load_building(P_Guild guild, int type, int location, int level)
{
  if (!type || !location)
  {
    debug("Failed to call load_building() with valid values passed.");
    return NULL;
  }

  Building* building = new Building(guild, type, location, level);

  if( building->is_loaded() )
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

  if( building->is_loaded() )
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
  
  if (GET_OPPONENT(ch))
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

  /*  and this doesn't work...
  // hack to clear event damage spells killing outpost multiple times
  if (current_event && current_event->actor.a_ch == ch)
    current_event = NULL;
  while (get_linking_char(ch, LNK_EVENT))
  {
    if (P_char evch = get_linking_char(ch, LNK_EVENT))
    {
      P_nevent nevent = NULL;
      LOOP_EVENTS(nevent, evch->nevents)
	if (nevent->victim == ch)
	{
	  nevent->victim = NULL;
	  disarm_single_event(nevent);
	}
    }
  }
  */

  clear_all_links(ch);
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
      debug("Error, outpost %d contains no mob spec function", building->get_id());
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
    debug("Error, outpost %d contains invalid ACT_SPEC info", building->get_id());
    return TRUE;
  }

  return TRUE;
}

int building_mob_proc(P_char ch, P_char pl, int cmd, char *arg)
{
  int        allow;
  P_Alliance alliance;
  P_Guild    guild;
  P_char     tmob;
  Building  *building;
  struct group_list *gl;

  if( !ch )
    return FALSE;

  if( cmd == CMD_SET_PERIODIC )
  {
    SET_BIT(ch->specials.act, ACT_SPEC_DIE);
    return FALSE;
  }

  if( !affected_by_spell(ch, TAG_BUILDING) )
    return FALSE;

  if( (building = get_building_from_char(ch)) == NULL )
    return FALSE;

  if( building->proc(ch, pl, cmd, arg) )
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

  guild = building->get_guild( );

  if( cmd == CMD_ENTER )
  {
    allow = FALSE;

    // Guild and Alliance checks
    if( IS_TRUSTED(pl) || (guild == NULL) || (GET_ASSOC( pl ) == guild) )
    {
      allow = TRUE;
    }
    else if( (( alliance = guild->get_alliance() ) != NULL)
      && (( GET_ASSOC(pl) == alliance->get_forgers() ) || ( GET_ASSOC(pl) == alliance->get_joiners() )) )
    {
      allow = TRUE;
    }

    if( !allow && (( gl = pl->group ) != NULL) )
    {
      while( gl )
      {
        if( GET_ASSOC(gl->ch) == guild )
        {
          allow = TRUE;
          break;
        }
        gl = gl->next;
      }
    }

    if( !(tmob = get_char_room_vis(pl, arg)) || tmob != ch )
      return FALSE;

    if( allow && (IS_FIGHTING(pl) || IS_DESTROYING(pl)) )
    {
      act("&+WYou cannot enter a guildhall in combat!", FALSE, pl, 0, 0, TO_CHAR);
      return TRUE;
    }

    if( !allow )
    {
      send_to_char("You don't own this outpost!\r\n", pl);
      return TRUE;
    }

    if( !building->get_room(0)->number )
      return FALSE;

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(pl) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", pl);
      drop_ctf_flag(pl);
    }
#endif

    act("You enter $N.", TRUE, pl, 0, tmob, TO_CHAR);
    act("$n enters $N.", TRUE, pl, 0, tmob, TO_NOTVICT);

    char_from_room(pl);
    char_to_room(pl, building->gate_room(), -1);
    return TRUE;
  }

  // Because outpost is para, can't use CMD_MOB_MUNDANE, so have to
  // make the percentages based on it getting hit or nuked.
  if( cmd == CMD_GOTHIT || cmd == CMD_GOTNUKED )
  {
    // Instead of making it attack 25% of the time, we can add things like amount of archers,
    //   how many attacks per round based on level of outpost, or how long they've held it for.
    if( get_outpost_archers(building) )
    {
      if( ((cmd == CMD_GOTHIT) && !number(0, 4)) || ((cmd == CMD_GOTNUKED) && !number(0, 2)) )
      {
        if( outpost_archer_attack(ch, pl) && (cmd != CMD_GOTNUKED) )
        {
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

// Building
int Building::next_id = 1;

Building::Building() : guild(NULL), type(0), room_vnum(0), level(0), loaded(false), mob_proc(NULL) {}

Building::Building(P_Guild _guild, int _type, int _room_vnum, int _level) : 
  guild(_guild), type(_type), room_vnum(_room_vnum), level(_level), loaded(false), mob_proc(NULL)
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
  P_char mob;

  if( !building )
    return FALSE;

  mob = building->get_mob();

  // set mob stuff
  if( get_current_outpost_hitpoints(building) > 0 )
  {
    GET_MAX_HIT(mob) = mob->points.base_hit = building_types[BUILDING_OUTPOST-1].hitpoints;
    GET_HIT(mob) = get_current_outpost_hitpoints(building);
  }
  else // It SHOULD be a new outpost...
  {
    GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = (int)(building_types[BUILDING_OUTPOST-1].hitpoints);
  }
  set_current_outpost_hitpoints(building);  
  GET_RACE(mob) = RACE_CONSTRUCT;
  SET_BIT(mob->specials.affected_by2, AFF2_MAJOR_PARALYSIS);
  SET_BIT(mob->specials.act, ACT_SPEC);
  building->set_proc( );

  // generate rooms
  P_room room = get_unused_building_room();

  if( !room )
    return FALSE;

  building->add_room(room);
  room->funct = outpost_inside;

  room->name = str_dup("&+LInside an &+yOutpost Tower");

  CREATE(room->dir_option[DIR_DOWN], room_direction_data, 1, MEM_TAG_DIRDATA);

  room->dir_option[DIR_DOWN]->to_room = building->location();
  room->dir_option[DIR_DOWN]->exit_info = 0;
  
  room->funct = outpost_inside;

  return TRUE;
}

// mob proc for outpost
int outpost_mob(P_char ch, P_char pl, int cmd, char *arg)
{
  if( !ch )
    return FALSE;
  
  //if( cmd == CMD_MOB_MUNDANE )
  //{
    //mobsay(ch, "&+Cmundane");
    //return TRUE;
  //}
  
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
      if ((old_room = ch->in_room))
      {
        Building* building = get_building_from_room(ch->in_room);
        int z = building->get_level();
        char_from_room(ch);
        ch->specials.z_cord = z;
        char_to_room(ch, world[old_room].dir_option[DIR_DOWN]->to_room, -1);
        char_from_room(ch);
        ch->specials.z_cord = 0;
        char_to_room(ch, old_room, -2);
        return TRUE;
      }
    }
  }
  return FALSE;
}

bool Building::proc( P_char ch, P_char pl, int cmd, char *arg)
{
  return ( (mob_proc != NULL) && (( *mob_proc )( ch, pl, cmd, arg )) );
}

void Building::update_outpost_owner( P_Guild new_guild )
{
  P_char op;
  struct affected_type *afp;

  if( !id )
  {
    debug("error calling update_outpost_owner, no building ID available");
    return;
  }

  // Update the DB
  db_query("UPDATE outposts SET owner_id = '%d' WHERE id = '%d'", new_guild->get_id(), id - 1);
  guild = new_guild;
}

// Clears the outpost portals.
void Building::clear_portal_op()
{
  if( portal_op )
  {
    extract_obj( portal_op );
    portal_op = NULL;
  }
  if( portal_gh )
  {
    extract_obj( portal_gh );
    portal_gh = NULL;
  }
}

bool Building::load_gateguard(int location, int guard_vnum, int golemnum)
{
  golems[golemnum] = read_mobile(guard_vnum, VIRTUAL);

  if( golems[golemnum] == NULL )
  {
    debug("load_gateguards: error reading mobile vnum %d.", guard_vnum);
    return FALSE;
  }

  add_tag_to_char(golems[golemnum], TAG_DIRECTION, rev_dir[golem_dir], AFFTYPE_PERM);
  add_tag_to_char(golems[golemnum], TAG_GUILDHALL, id, AFFTYPE_PERM);
  GET_ASSOC(golems[golemnum]) = guild;
  golems[golemnum]->player.birthplace = golems[golemnum]->player.orig_birthplace
    = golems[golemnum]->player.hometown = world[location].number;
  char_to_room(golems[golemnum], location, -1);

  return TRUE;
}

bool Building::generate_portals()
{
  char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char buff[MAX_STRING_LENGTH], ghname[MAX_STRING_LENGTH];
  FILE *f;
  int guildhall_room = 0;

  if( portal_op || portal_gh )
  {
    debug("generate_portals: Portal already exists.");
    return FALSE;
  }

  if( guild == NULL )
  {
    debug("generate_portals: Building isn't owned by a guild.");
    return FALSE;
  }
  else
  {
    if( Guildhall *gh = Guildhall::find_by_assoc_id(guild->get_id( )) )
    {
      if (gh->heartstone_room)
        guildhall_room = gh->heartstone_room->vnum;
      else
      {
        debug("can't find guildhall hearthstone_room");
        return 0;
      }
    }
    else
    {
      debug("You don't own a guildhall.");
      return 0;
    }
    
    snprintf(Gbuf1, MAX_STRING_LENGTH, "%sasc.%u", ASC_DIR, guild->get_id());
    f = fopen(Gbuf1, "r");
    if (!f)
    {
      snprintf(ghname, MAX_STRING_LENGTH, "Unknown");
    }
    else
    {
      fgets(Gbuf2, MAX_STR_NORMAL, f);
      Gbuf2[strlen(Gbuf2)-1] = 0;
      Gbuf2[ASC_MAX_STR-1] = 0;
      strcpy(ghname, Gbuf2);
      fclose(f);
    }
  }

  if ((portal_op = read_object(BUILDING_PORTAL, VIRTUAL)))
  {
    snprintf(buff, MAX_STRING_LENGTH, portal_op->description, ghname);
    portal_op->value[0] = guildhall_room;
    portal_op->description = str_dup(buff);
    portal_op->str_mask = STRUNG_DESC1;
    obj_to_room(portal_op, gate_room());
  }

  if ((portal_gh = read_object(BUILDING_PORTAL, VIRTUAL)))
  {
    snprintf(buff, MAX_STRING_LENGTH, portal_gh->description, continent_name(world[location()].continent));
    portal_gh->value[0] = rooms[0]->number;
    portal_gh->description = str_dup(buff);
    portal_gh->str_mask = STRUNG_DESC1;
    obj_to_room(portal_gh, real_room0(guildhall_room));
  }

  return 1;
}

bool Building::sub_money( int platinum, int gold, int silver, int copper )
{
  if( guild != NULL )
  {
    return guild->sub_money( platinum, gold, silver, copper );
  }
  return FALSE;
}
