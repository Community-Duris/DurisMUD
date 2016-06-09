/*
 *  guildhall_cmds.c
 *  Code for the 'construct', 'guildhall' command
 *
 *  Created by Torgal on 1/30/10.
 *
 */

#include <string.h>
#include <math.h>


#include "structs.h"
#include "guildhall.h"
#include "guildhall_db.h"
#include "prototypes.h"
#include "assocs.h"
#include "utils.h"
#include "utility.h"
#include "map.h"
#include "db.h"
#include "justice.h"

#define CAN_CONSTRUCT_CMD(ch) ( GET_ASSOC(ch) && (IS_LEADER(GET_M_BITS(GET_A_BITS(ch), A_RK_MASK)) || GT_LEADER(GET_M_BITS(GET_A_BITS(ch), A_RK_MASK))) )
#define CAN_GUILDHALL_CMD(ch) ( GET_ASSOC(ch) && (IS_OFFICER(GET_M_BITS(GET_A_BITS(ch), A_RK_MASK)) || GT_OFFICER(GET_M_BITS(GET_A_BITS(ch), A_RK_MASK))) )

// global variable declarations
extern P_room world;
extern const int rev_dir[];
extern struct zone_data *zone_table;

// function declarations
void do_construct_overmax(P_char ch, char *arg);
void do_construct_guildhall(P_char ch, char *arg);
void do_construct_destroy(P_char ch, char *arg);
void do_construct_reset(P_char ch, char *arg);
void do_construct_room(P_char ch, char *arg);
void do_construct_golem(P_char ch, char *arg);
void do_construct_upgrade(P_char ch, char *arg);
void do_construct_rename(P_char ch, char *arg);

void do_guildhall_list(P_char ch, char *arg);
void do_guildhall_destroy(P_char ch, char *arg);
void do_guildhall_reload(P_char ch, char *arg);
void do_guildhall_status(P_char ch, char *arg);
void do_guildhall_info(P_char ch, char *arg);
void do_guildhall_move(P_char ch, char *arg);

void guildhall_info(Guildhall *gh, P_char ch);
bool construct_main_guildhall(P_Guild guild, int outside_vnum, int racewar);
bool construct_new_guildhall_room(int id, int from_vnum, int dir);
bool construct_golem(Guildhall *gh, int slot, int type);
bool upgrade_room(GuildhallRoom *room, int type);
bool rename_room(GuildhallRoom *room, char *name);
bool move_guildhall(Guildhall *gh, int vnum);

bool destroy_guildhall(int id);
bool reload_guildhall(int id);

bool guildhall_map_check(P_char ch);

// checks ch's home/birthplace to make sure they are allowed to be homed there
int check_gh_home(P_char ch, int r_room)
{
  Guildhall *gh = NULL;

  //debug("entering check_gh_home(): home: %d birthplace: %d orig_birthplace: %d", GET_HOME(ch), GET_BIRTHPLACE(ch), GET_ORIG_BIRTHPLACE(ch));

  if( IN_GH_ZONE(real_room0(GET_BIRTHPLACE(ch))) )
  {
    gh = Guildhall::find_by_vnum(GET_BIRTHPLACE(ch));

    if( !gh || !IS_ASSOC_MEMBER(ch, gh->guild) )
    {
      debug("%s birthplace in GH but no GH or doesn't belong to guild, bumping to hometown", GET_NAME(ch));
      GET_BIRTHPLACE(ch) = GET_ORIG_BIRTHPLACE(ch);
    }
  }
  
  if( IN_GH_ZONE(real_room0(GET_HOME(ch))) )
  {
    gh = Guildhall::find_by_vnum(GET_HOME(ch));
    
    if( !gh || !IS_ASSOC_MEMBER(ch, gh->guild) )
    {
      debug("%s home in GH but no GH or doesn't belong to guild, bumping to hometown", GET_NAME(ch));
      GET_HOME(ch) = GET_BIRTHPLACE(ch);
    }    
  }

  // if r_room is already set, check to see if it's inside of the GH zone
  if( IN_GH_ZONE(r_room) )
  {
    gh = Guildhall::find_by_vnum(world[r_room].number);

    if( !gh || GET_ASSOC(ch) != gh->guild )
    {
      // if player comes back in the GH zone but not inside of a GH, bump to hometown
      debug("%s entering game in GH zone but no GH or not in guild, bumping to hometown", GET_NAME(ch));
      GET_HOME(ch) = GET_BIRTHPLACE(ch);
      r_room = real_room(GET_HOME(ch));
    }
  
    // TODO: add enemy in hall check
  }
  
  debug("leaving check_gh_home(): home: %d birthplace: %d orig_birthplace: %d", GET_HOME(ch), GET_BIRTHPLACE(ch), GET_ORIG_BIRTHPLACE(ch));

  return r_room;
}

//
// construct command
//
//

const char CONSTRUCT_SYNTAX[] =
"&+BConstruct syntax                                  \r\n"
"&+b----------------------------------------------------------------------------------\r\n"
"&+Wconstruct guildhall&n - construct a guildhall in the current room on map\r\n"
"&+Wconstruct golem    &n - construct a golem in the entrance room\r\n"
"&+Wconstruct room     &n - construct a new room\r\n"
"&+Wconstruct upgrade  &n - upgrade the current room\r\n"
"&+Wconstruct rename   &n - rename the current room\r\n"
"&+Wconstruct overmax  &n - increase max players\r\n";

void do_construct(P_char ch, char *arg, int cmd)
{
  if(!ch)
    return;
  
  if( !CAN_CONSTRUCT_CMD(ch) && !IS_TRUSTED(ch) )
  {
    send_to_char("You need to be the leader of a guild to use the 'construct' command.\r\n", ch);
    return;
  }

  if(!arg || !*arg)
  {
    send_to_char(CONSTRUCT_SYNTAX, ch);
    return;
  }
    
  char buff[MAX_STRING_LENGTH];
  arg = one_argument(arg, buff);
  
  if( is_abbrev(buff, "guildhall") )
  {
    do_construct_guildhall(ch, arg);
  }
  else if( is_abbrev(buff, "room") )
  {
    do_construct_room(ch, arg);
  }
  else if( is_abbrev(buff, "golem") )
  {
    do_construct_golem(ch, arg);
  }
  else if( is_abbrev(buff, "upgrade") )
  {
    do_construct_upgrade(ch, arg);
  }
  else if( is_abbrev(buff, "rename") )
  {
    do_construct_rename(ch, arg);
  }
  else if( is_abbrev(buff, "overmax") )
  {
    do_construct_overmax(ch, arg);
  }
  else
  {
    send_to_char(CONSTRUCT_SYNTAX, ch);
  }
  
}

void do_construct_overmax(P_char ch, char *arg)
{
  char buff[MAX_STRING_LENGTH];
  int cp_cost = (int)get_property("guildhalls.construction.points.overmax", 100);
  int plat_cost = (int)get_property("guildhalls.construction.platinum.overmax", 100000);
  int maxovermax;
  
  if (!ch)
    return;

  if (!GET_ASSOC(ch))
  {
    send_to_char("You must be the leader of a guild to expand your max players.\r\n", ch);
    return;
  }
  
  if (IS_RACEWAR_GOOD(ch))
  {
    maxovermax = (int)get_property("guildhalls.construction.overmax.cap.good", 10);
    plat_cost = plat_cost/2;
  }
  else if (IS_RACEWAR_EVIL(ch))
    maxovermax = (int)get_property("guildhalls.construction.overmax.cap.evil", 5);
  else
    maxovermax = 0;

  if( GET_ASSOC(ch)->get_overmax() >= maxovermax )
  {
    send_to_char("You can't purchase anymore max players.\r\n", ch);
    return;
  }

  if (!IS_TRUSTED(ch))
  {
    debug("%d, %d", plat_cost, GET_MONEY(ch));
    if(GET_MONEY(ch) < plat_cost)
    {
      sprintf(buff, "You don't have enough money on you - it costs %s&n to extend your max players.\r\n", coin_stringv(plat_cost));
      send_to_char(buff, ch);
      return;
    }

    if( GET_ASSOC(ch)->get_construction() < cp_cost )
    {
      sprintf(buff, "Your guild doesn't yet have enough &+Wconstruction points&n - it costs %d to add a member over max.\r\n", cp_cost);
      send_to_char(buff, ch);
      return;
    }
  }

  if (!IS_TRUSTED(ch))
    SUB_MONEY(ch, plat_cost, 0);
  GET_ASSOC(ch)->add_construction( -cp_cost );
  GET_ASSOC(ch)->add_overmax( 1 );
  send_to_char("You have purchased an overmax player limit.\n\r", ch);

  return;
}

void do_construct_guildhall(P_char ch, char *arg)
{
  char buff[MAX_STRING_LENGTH];

  int plat_cost = get_property("guildhalls.construction.platinum.main", 0) * 1000;
  int cp_cost = get_property("guildhalls.construction.points.main", 0);
  
  if(!ch)
    return;
  
  if(!GET_ASSOC(ch))
  {
    send_to_char("You must be the leader of a guild to build a guildhall.\r\n", ch);
    return;
  }
  
  if( !IS_TRUSTED(ch) )
  {
    if( !guildhall_map_check(ch) )
    {
      return;
    }
    
    if( Guildhall::count_by_assoc_id(GET_ASSOC(ch)->get_id(), GH_TYPE_MAIN) > 0 )
    {
      send_to_char("You already have a guildhall!\r\n", ch);
      return;
    }

    if(GET_MONEY(ch) < plat_cost)
    {
      sprintf(buff, "You don't have enough money on you - it costs %s&n to build a guildhall.\r\n", coin_stringv(plat_cost));
      send_to_char(buff, ch);
      return;
    }
    
    if( GET_ASSOC(ch)->get_construction() < cp_cost )
    {
      sprintf(buff, "Your guild doesn't yet have enough &+Wconstruction points&n - it costs %d to build a guildhall.\r\n", cp_cost);
      send_to_char(buff, ch);
      return;
    }
  }

  if( construct_main_guildhall(GET_ASSOC(ch), world[ch->in_room].number, GET_RACEWAR(ch)) )
  {
    if(!IS_TRUSTED(ch))
    {
      SUB_MONEY(ch, plat_cost, 0);
      GET_ASSOC(ch)->add_construction( -cp_cost );
    }

    send_to_char("You stand back as a &+ggremlin construction team&n begins scrambling around the construction site. The building\r\n"
                 "begins to take shape immediately, and only a few minutes later you are left with a brand new guildhall and\r\n"
                 "a much lighter wallet.\r\n", ch);
    
    CharWait(ch, PULSE_VIOLENCE*2);
    logit(LOG_GUILDHALLS, "%s built a main guildhall for %s in %d", GET_NAME(ch), strip_ansi(GET_ASSOC(ch)->get_name().c_str()).c_str(), world[ch->in_room].number);
    return;
  }
  else
  {
    send_to_char("Something went wrong, please petition!\r\n", ch);
    debug("%s tried to construct a main guildhall, but it failed!", GET_NAME(ch));
  }  
}

void do_construct_room(P_char ch, char *arg)
{
  char buff[MAX_STRING_LENGTH];

  int plat_cost = get_property("guildhalls.construction.platinum.room", 0) * 1000;
  int cp_cost = get_property("guildhalls.construction.points.room", 0);

  if( !ch )
    return;
  
  int from_vnum = world[ch->in_room].number;
  
  Guildhall *gh = Guildhall::find_by_vnum(from_vnum);
  
  if( !gh )
  {
    send_to_char("You must be in a guildhall to construct a new room!\r\n", ch);
    return;
  }
  
  if( GET_ASSOC(ch) != gh->guild )
  {
    send_to_char("You can only construct inside of YOUR guildhall.\r\n", ch);
    return;
  }
  
  if(!*arg)
  {
    sprintf(buff, "Syntax: construct room <north|east|south|west|up|down|northwest|northeast|southwest|southeast>\r\n"
                  "Cost: %s&n and %d &+Wconstruction points&n\r\n", coin_stringv(plat_cost), cp_cost);
    send_to_char(buff, ch);
    return;
  }

  arg = one_argument(arg, buff);

  int dir = dir_from_keyword(buff);
  
  if( dir < 0 )
  {
    send_to_char("Please enter a valid direction!\r\n", ch);
    return;
  }
  
  if( world[ch->in_room].dir_option[dir] )
  {
    send_to_char("There is already a room in that direction!\r\n", ch);
    return;
  }

  if(!gh->can_add_room())
  {
    send_to_char("Your guildhall has reached its room limit.\r\n", ch);
    return;
  }
  
  if(!IS_TRUSTED(ch))
  {
    if(GET_MONEY(ch) < plat_cost)
    {
      sprintf(buff, "You don't have enough money on you - it costs %s&n to build a new room.\r\n", coin_stringv(plat_cost));
      send_to_char(buff, ch);
      return;
    }    
    
    if(GET_ASSOC(ch)->get_construction() < cp_cost)
    {
      sprintf(buff, "Your guild doesn't yet have enough &+Wconstruction points&n - it costs %d to build a new room.\r\n", cp_cost);
      send_to_char(buff, ch);
      return;
    }    
  }
  
  if( construct_new_guildhall_room(gh->id, from_vnum, dir) )
  {
    if(!IS_TRUSTED(ch))
    {
      SUB_MONEY(ch, plat_cost, 0);
      GET_ASSOC(ch)->add_construction( -cp_cost );
    }
    
    send_to_char("A &+ggremlin construction team&n appears immediately from out of nowhere and begins boring a hole\r\n"
                 "in the wall. Seconds later the dust settles and your hall has a new room!\r\n", ch);
    
    CharWait(ch, PULSE_VIOLENCE*2);
    logit(LOG_GUILDHALLS, "%s built a new room for %s in %d", GET_NAME(ch), strip_ansi(GET_ASSOC(ch)->get_name().c_str()).c_str(), world[ch->in_room].number);
    return;
  }
  else
  {
    send_to_char("Something went wrong, please petition!\r\n", ch);
    debug("%s tried to construct a new room, but it failed!", GET_NAME(ch));
  }

}

void do_construct_golem(P_char ch, char *arg)
{
  char buff[MAX_STRING_LENGTH];

  int plat_cost = 0, cp_cost = 0;

  if( !ch )
    return;

  Guildhall *gh = Guildhall::find_by_vnum(world[ch->in_room].number);

  if( !gh )
  {
    send_to_char("You must be inside your guildhall in order to construct a golem!\r\n", ch);
    return;
  }

  if( GET_ASSOC(ch) != gh->guild )
  {
    send_to_char("You can only construct inside of YOUR guildhall.\r\n", ch);
    return;
  }

  if( !gh->entrance_room || gh->entrance_room->vnum != world[ch->in_room].number)
  {
    send_to_char("You must be in your guildhall's entrance hall in order to construct a golem!\r\n", ch);
    return;
  }

  if(!*arg)
  {
    // just 'construct golem' with no other arguments, so show the current golem slots

    send_to_char(
                 "&+BGuildhall golem slots\r\n"
                 "&+b-----------------------------------------\r\n", ch);

    for(int i = 0; i < GH_GOLEM_NUM_SLOTS; i++)
    {
      sprintf(buff, "&+W%d&n) %s\r\n", (i+1), ( gh->entrance_room->golems[i] ? gh->entrance_room->golems[i]->player.short_descr : "none" ));
      send_to_char(buff, ch);
    }

    send_to_char("\r\n", ch);

    send_to_char("Syntax: construct golem <slot> <warrior|cleric|sorcerer>\r\n", ch);

    send_to_char("Cost:\r\n", ch);

    sprintf(buff, " %s: %s&n and %d &+Wconstruction points&n\r\n", "warrior", 
            coin_stringv(get_property("guildhalls.construction.platinum.golem.warrior", 0)*1000), 
            get_property("guildhalls.construction.points.golem.warrior", 0));
    send_to_char(buff, ch);

    sprintf(buff, " %s: %s&n and %d &+Wconstruction points&n\r\n", "cleric", 
            coin_stringv(get_property("guildhalls.construction.platinum.golem.cleric", 0)*1000), 
            get_property("guildhalls.construction.points.golem.cleric", 0));
    send_to_char(buff, ch);

    sprintf(buff, " %s: %s&n and %d &+Wconstruction points&n\r\n", "sorcerer", 
            coin_stringv(get_property("guildhalls.construction.platinum.golem.sorcerer", 0)*1000), 
            get_property("guildhalls.construction.points.golem.sorcerer", 0));
    send_to_char(buff, ch);

    return;
  }

  //
  // choose the golem slot
  //
  arg = one_argument(arg, buff);

  int slot = atoi(buff);

  if( slot < 1 || slot > GH_GOLEM_NUM_SLOTS )
  {
    send_to_char("Please enter a valid golem slot.\r\n", ch);
    return;
  }

  if( gh->entrance_room->golems[slot-1] ) // (slot-1) because player enters 1-4 but its stored as 0-3
  {
    send_to_char("There is already a golem in that slot.\r\n", ch);
    return;
  }

  //
  // choose the golem type
  //
  arg = one_argument(arg, buff);
  int type = 0;

  if( is_abbrev(buff, "warrior") )
  {
    type = GH_GOLEM_TYPE_WARRIOR;
    plat_cost = get_property("guildhalls.construction.platinum.golem.warrior", 0) * 1000;
    cp_cost = get_property("guildhalls.construction.points.golem.warrior", 0);
  }
  else if( is_abbrev(buff, "cleric") )
  {
    type = GH_GOLEM_TYPE_CLERIC;
    plat_cost = get_property("guildhalls.construction.platinum.golem.cleric", 0) * 1000;
    cp_cost = get_property("guildhalls.construction.points.golem.cleric", 0);
  }
  else if( is_abbrev(buff, "sorcerer") )
  {
    type = GH_GOLEM_TYPE_SORCERER;
    plat_cost = get_property("guildhalls.construction.platinum.golem.sorcerer", 0) * 1000;
    cp_cost = get_property("guildhalls.construction.points.golem.sorcerer", 0);
  }
  else
  {
    send_to_char("Please enter a valid type of golem.\r\n", ch);
    return;
  }

  if(!IS_TRUSTED(ch))
  {
    if(GET_MONEY(ch) < plat_cost)
    {
      sprintf(buff, "You don't have enough money on you - it costs %s&n to build that type of golem.\r\n", coin_stringv(plat_cost));
      send_to_char(buff, ch);
      return;
    }

    if(GET_ASSOC(ch)->get_construction() < cp_cost)
    {
      sprintf(buff, "Your guild doesn't yet have enough &+Wconstruction points&n - it costs %d to build that type of golem.\r\n", cp_cost);
      send_to_char(buff, ch);
      return;
    }
  }

  if( construct_golem(gh, (slot-1), type) ) // (slot-1) because player enters 1-4 but they are stored in 0-3
  {
    if(!IS_TRUSTED(ch))
    {
      SUB_MONEY(ch, plat_cost, 0);
      GET_ASSOC(ch)->add_construction( -cp_cost );
    }

    send_to_char("A &+ggremlin construction team&n appears carrying a massive load of iron, clay, pottery, and bits of string.\r\n"
                 "They assemble the materials into a humanoid shape and then link hands and begin dancing around the shape,\r\n"
                 "chanting a strange ritual song. A few minutes later the pile of junk somehow coallesces into a united being,\r\n"
                 "and the &+ggremlins&n disappear as quickly as they came.\r\n", ch);

    CharWait(ch, PULSE_VIOLENCE*2);
    logit(LOG_GUILDHALLS, "%s built a type %d golem for %s in %d", GET_NAME(ch), type, strip_ansi(GET_ASSOC(ch)->get_name().c_str()).c_str(), world[ch->in_room].number);
    return;
  }
  else
  {
    send_to_char("Something went wrong, please petition!\r\n", ch);
    debug("%s tried to construct a golem, but it failed!", GET_NAME(ch));
  }
}

//
// command for upgrading room types
//
void do_construct_upgrade(P_char ch, char *arg)
{
  char buff[MAX_STRING_LENGTH];
  
  int plat_cost = 0, cp_cost = 0;
  
  GuildhallRoom *room = Guildhall::find_room_by_vnum(world[ch->in_room].number);

  if(!room)
    return;
       
  Guildhall *gh = room->guildhall;
  
  if(!gh)
    return;

  if( GET_ASSOC(ch) != gh->guild )
  {
    send_to_char("You can only construct inside of YOUR guildhall.\r\n", ch);
    return;
  }
  
  if(!*arg)
  {
    // just 'construct upgrade' without arguments, so show list of possible upgrades
    
    send_to_char(
                 "&+BAvailable room upgrades\r\n"
                 "&+b-----------------------------------------\r\n", ch);
    
    
    sprintf(buff, "&+W%s&n) %s (%s and %d &+Wconstruction points&n)\r\n", "heal", "a relaxing healing room with a fountain", 
            coin_stringv(get_property("guildhalls.construction.platinum.upgrade.heal", 0) * 1000), 
            get_property("guildhalls.construction.points.upgrade.heal", 0));
    send_to_char(buff, ch);

    sprintf(buff, "&+W%s&n) %s (%s and %d &+Wconstruction points&n)\r\n", "window", "a room with a magical window that gives you a glimpse outside the guildhall", 
            coin_stringv(get_property("guildhalls.construction.platinum.upgrade.window", 0) * 1000), 
            get_property("guildhalls.construction.points.upgrade.window", 0));
    send_to_char(buff, ch);

    sprintf(buff, "&+W%s&n) %s (%s and %d &+Wconstruction points&n)\r\n", "bank", "a guild bank where you can enter your guild locker and deposit/withdraw guild funds", 
            coin_stringv(get_property("guildhalls.construction.platinum.upgrade.bank", 0) * 1000), 
            get_property("guildhalls.construction.points.upgrade.bank", 0));
    send_to_char(buff, ch);

    sprintf(buff, "&+W%s&n) %s (%s and %d &+Wconstruction points&n)\r\n", "town", "a portal to your local hometown inn", 
            coin_stringv(get_property("guildhalls.construction.platinum.upgrade.town_portal", 0) * 1000), 
            get_property("guildhalls.construction.points.upgrade.town_portal", 0));
    send_to_char(buff, ch);

    sprintf(buff, "&+W%s&n) %s (%s and %d &+Wconstruction points&n)\r\n", "library", "a library with a tome from which you can memorize all spells", 
            coin_stringv(get_property("guildhalls.construction.platinum.upgrade.library", 0) * 1000), 
            get_property("guildhalls.construction.points.upgrade.library", 0));
    send_to_char(buff, ch);

    sprintf(buff, "&+W%s&n) %s (%s and %d &+Wconstruction points&n)\r\n", "cargo", "an information board with current cargo prices across the world", 
            coin_stringv(get_property("guildhalls.construction.platinum.upgrade.cargo", 0) * 1000), 
            get_property("guildhalls.construction.points.upgrade.cargo", 0));
    send_to_char(buff, ch);
    
    send_to_char("\r\n", ch);
    send_to_char("To upgrade this room, type 'construct upgrade <type>'\r\n", ch);
    
    return;
  }

  if(!room->can_upgrade())
  {
    send_to_char("This room cannot be upgraded.\r\n", ch);
    return;
  }
  
  // list types you can upgrade to
  arg = one_argument(arg, buff);
  int type = 0;
  
  if( isname(buff, "window") )
  {
    type = GH_ROOM_TYPE_WINDOW;    
    plat_cost = get_property("guildhalls.construction.platinum.upgrade.window", 0) * 1000;
    cp_cost = get_property("guildhalls.construction.points.upgrade.window", 0);
  }
  else if( isname(buff, "heal") )
  {
    type = GH_ROOM_TYPE_HEAL;    
    plat_cost = get_property("guildhalls.construction.platinum.upgrade.heal", 0) * 1000;
    cp_cost = get_property("guildhalls.construction.points.upgrade.heal", 0);
  }
  else if( isname(buff, "bank") )
  {
    type = GH_ROOM_TYPE_BANK;    
    plat_cost = get_property("guildhalls.construction.platinum.upgrade.bank", 0) * 1000;
    cp_cost = get_property("guildhalls.construction.points.upgrade.bank", 0);
  }  
  else if( isname(buff, "town") )
  {
    type = GH_ROOM_TYPE_TOWN_PORTAL;    
    plat_cost = get_property("guildhalls.construction.platinum.upgrade.town_portal", 0) * 1000;
    cp_cost = get_property("guildhalls.construction.points.upgrade.town_portal", 0);
  }
  else if( isname(buff, "library") )
  {
    type = GH_ROOM_TYPE_LIBRARY;    
    plat_cost = get_property("guildhalls.construction.platinum.upgrade.library", 0) * 1000;
    cp_cost = get_property("guildhalls.construction.points.upgrade.library", 0);
  }      
  else if( isname(buff, "cargo") )
  {
    type = GH_ROOM_TYPE_CARGO;    
    plat_cost = get_property("guildhalls.construction.platinum.upgrade.cargo", 0) * 1000;
    cp_cost = get_property("guildhalls.construction.points.upgrade.cargo", 0);
  }        
  else
  {
    send_to_char("Please enter a valid type of room to upgrade to.\r\n", ch);
    return;
  }
     
  if(!IS_TRUSTED(ch))
  {
    if(GET_MONEY(ch) < plat_cost)
    {
      sprintf(buff, "You don't have enough money on you - it costs %s&n for that room upgrade.\r\n", coin_stringv(plat_cost));
      send_to_char(buff, ch);
      return;
    }    
    
    if(GET_ASSOC(ch)->get_construction() < cp_cost)
    {
      sprintf(buff, "Your guild doesn't yet have enough &+Wconstruction points&n - it costs %d for that room upgrade.\r\n", cp_cost);
      send_to_char(buff, ch);
      return;
    }    
  }
  
  if( upgrade_room(room, type) )
  {
    if(!IS_TRUSTED(ch))
    {
      SUB_MONEY(ch, plat_cost, 0);
      GET_ASSOC(ch)->add_construction( -cp_cost );
    }
    
    send_to_char("A &+ggremlin construction team&n appears suddenly and begins to renovate the room. As quickly as they appeared,\r\n"
                 "they are suddenly gone and the room has had a complete makeover.\r\n", ch);
    
    CharWait(ch, PULSE_VIOLENCE*2);
    logit(LOG_GUILDHALLS, "%s upgraded room for %s in %d to type %d", GET_NAME(ch), strip_ansi(GET_ASSOC(ch)->get_name().c_str()).c_str(), world[ch->in_room].number, type);
    return;
  }
  else
  {
    send_to_char("Something went wrong, please petition!\r\n", ch);
    debug("%s tried to upgrade a room, but it failed!", GET_NAME(ch));
  }
}

//
// command for renaming rooms
//
void do_construct_rename(P_char ch, char *arg)
{
  char buff[MAX_STRING_LENGTH];

  int plat_cost = get_property("guildhalls.construction.platinum.rename", 0) * 1000;
  int cp_cost = get_property("guildhalls.construction.points.rename", 0);

  GuildhallRoom *room = Guildhall::find_room_by_vnum(world[ch->in_room].number);
  
  if(!room)
    return;
  
  Guildhall *gh = room->guildhall;
  
  if(!gh)
  {
    send_to_char("You can only construct inside of your guildhall.\r\n", ch);
    return;
  }

  if( GET_ASSOC(ch) != gh->guild )
  {
    send_to_char("You can only construct inside of YOUR guildhall.\r\n", ch);
    return;
  }
  
  // skip space at the front of name
  while(*arg == ' ')
    arg++;
  
  if(!*arg || !arg)
  {
    sprintf(buff, "Syntax: construct rename <new room name>\r\nYou can use the 'testcolor' command to test your name first.\r\n"
                  "Cost: %s&n and %d &+Wconstruction points&n\r\n", coin_stringv(plat_cost), cp_cost);
    
    send_to_char(buff, ch);
    return;
  }

  if( !is_valid_ansi_with_msg(ch, arg, false) )
    return;
  
  if(!IS_TRUSTED(ch))
  {
    if(GET_MONEY(ch) < plat_cost)
    {
      sprintf(buff, "You don't have enough money on you - it costs %s&n to rename the room.\r\n", coin_stringv(plat_cost));
      send_to_char(buff, ch);
      return;
    }    
    
    if(GET_ASSOC(ch)->get_construction() < cp_cost)
    {
      sprintf(buff, "Your guild doesn't yet have enough &+Wconstruction points&n - it costs %d to rename the room.\r\n", cp_cost);
      send_to_char(buff, ch);
      return;
    }    
  }
  
  if( rename_room(room, arg) )
  {
    if(!IS_TRUSTED(ch))
    {
      SUB_MONEY(ch, plat_cost, 0);
      GET_ASSOC(ch)->add_construction( -cp_cost );
    }
    
    send_to_char("An annoyed, blurry-eyed &+ggremlin&n appears, gives you a dirty look, claps his hands two and a half times,\r\n"
                 "and then disappears. In the silence after his departure you notice that the room has ... changed.\r\n", ch);
    
    CharWait(ch, PULSE_VIOLENCE*2);
    logit(LOG_GUILDHALLS, "%s renamed for %s room %d to %s", GET_NAME(ch), strip_ansi(GET_ASSOC(ch)->get_name().c_str()).c_str(), world[ch->in_room].number, arg);
    return;
  }
  else
  {
    send_to_char("Something went wrong, please petition!\r\n", ch);
    debug("%s tried to rename a room, but it failed!", GET_NAME(ch));
  }  
}

//
//  guildhall command
//
//

const char GUILDHALL_GOD_SYNTAX[] =
"&+BGuildhall syntax                                  \r\n"
"&+b------------------------------------------------------------------------\r\n"
"&+Wguildhall list&n                             - list all guildhall structures  \r\n"
"&+Wguildhall status [id]&n                      - show guildhall status  \r\n"
"&+Wguildhall reload <id>&n                      - reload guildhall  \r\n"
"&+Wguildhall destroy <id>&n                     - destroy guildhall  \r\n"
"&+Wguildhall move <id> <here|vnum>&n            - move a guildhall doorway \r\n"
"\r\n";

void do_guildhall(P_char ch, char *arg, int cmd)
{
  if(!ch || !IS_PC(ch))
    return;      
  
  if( !IS_TRUSTED(ch) )
  {
    if( !CAN_GUILDHALL_CMD(ch) && !IS_TRUSTED(ch) )
    {
      send_to_char("You need to be the leader of a guild to use the 'guildhall' command.\r\n", ch);
      return;
    }
       
    do_guildhall_info(ch, arg);
    return;
  }
  else
  {
    // god-only commands    
    char buff[MAX_STRING_LENGTH];
    arg = one_argument(arg, buff);

    if( !*buff )
    {
      send_to_char(GUILDHALL_GOD_SYNTAX, ch);
    }
    else if( is_abbrev(buff, "status") )
    {
      do_guildhall_status(ch, arg);
    }    
    else if( is_abbrev(buff, "list") )
    {
      do_guildhall_list(ch, arg);
    }
    else if( is_abbrev(buff, "move") )
    {
      do_guildhall_move(ch, arg);
    }
    else if( is_abbrev(buff, "reload") )
    {
      do_guildhall_reload(ch, arg);
    }
    else if( is_abbrev(buff, "destroy") )
    {
      do_guildhall_destroy(ch, arg);
    }    
    else
    {
      send_to_char(GUILDHALL_GOD_SYNTAX, ch);
    }
  }
  
}
  
void do_guildhall_list(P_char ch, char *arg)
{
  send_to_char(
"&+BListing guildhall structures\r\n"
"&+b-----------------------------------------------------------------------------------------------------\r\n"
"&+b  ID | Guild                         | Heartstone | Outside | Continent     \r\n"
"&+b-----------------------------------------------------------------------------------------------------\r\n", ch);

  for( int i = 0; i < Guildhall::guildhalls.size(); i++ )
  {
    Guildhall* gh = Guildhall::guildhalls[i];

    if(!gh)
      continue;

    char buff[MAX_STRING_LENGTH];

    sprintf(buff, "%4d &+b|&n %2d: %s&n &+b|&n &+C%10d&n &+b|&n &+C%7d&n &+b| %s&n\r\n", 
            gh->id, 
            gh->guild->get_id(),
            pad_ansi(gh->guild->get_name().c_str(), 25).c_str(), 
            (gh->heartstone_room ? gh->heartstone_room->vnum : -1),
            gh->outside_vnum,
            continent_name(world[real_room0(gh->outside_vnum)].continent));

    send_to_char(buff, ch);
  }  
}

void do_guildhall_destroy(P_char ch, char *arg)
{
  if(!ch)
    return;
  
  int id = atoi(arg);

  if( destroy_guildhall(id) )
  {
    logit(LOG_GUILDHALLS, "%s destroyed guildhall %d (%s)", GET_NAME(ch), id, strip_ansi(GET_ASSOC(ch)->get_name().c_str()).c_str());
    send_to_char("Guildhall destroyed.\r\n", ch);
  }
  else
  {
    send_to_char("Which guildhall?\r\n", ch);
  }
}

void do_guildhall_reload(P_char ch, char *arg)
{
  if(!ch)
    return;
  
  int id = atoi(arg);
  
  if( reload_guildhall(id) )
  {
    logit(LOG_GUILDHALLS, "%s reloaded guildhall %d (%s)", GET_NAME(ch), id, strip_ansi(GET_ASSOC(ch)->get_name().c_str()).c_str());
    send_to_char("Guildhall reloaded.\r\n", ch);
  }
  else 
  {
    send_to_char("Which guildhall?\r\n", ch);
  }
}

void do_guildhall_status(P_char ch, char *arg)
{
  if(!ch)
    return;
  
  Guildhall *gh = NULL;

  if(!arg || !*arg)
  {
    gh = Guildhall::find_by_vnum(world[ch->in_room].number);
  }
  else
  {
    gh = Guildhall::find_by_id(atoi(arg));
  }
  
  if(!gh)
  {
    send_to_char("Invalid guildhall id.\r\n", ch);
    return;
  }
  
  guildhall_info(gh, ch);
}
  
//
// guildhall info command for guild leader
//
void do_guildhall_info(P_char ch, char *arg)
{
  if(!ch)
    return;
    
  Guildhall *gh = Guildhall::find_by_vnum(world[ch->in_room].number);
  
  if(!gh)
  {
    send_to_char("You must be inside your guildhall to use this command.\r\n", ch);
    return;
  }
  
  guildhall_info(gh, ch);
}

//
// move guildhall to a new (map) room.
// syntax: guildhall move <id> <vnum|here>
//
void do_guildhall_move(P_char ch, char *arg)
{
  if(!ch)
    return;
  
  if(!arg || !*arg)
  {
    send_to_char("Syntax: guildhall move <id> <here|vnum>\r\n", ch);
    return;
  }
  
  char buff[MAX_STRING_LENGTH];
  arg = one_argument(arg, buff);

  Guildhall *gh = Guildhall::find_by_id(atoi(buff));
  
  if(!gh)
  {
    send_to_char("Invalid guildhall id.\r\n", ch);
    return;
  }

  arg = one_argument(arg, buff);
  int vnum = -1;
  
  if( !strcmp(buff, "here") )
  {
    vnum = world[ch->in_room].number;
  }
  else if( atoi(buff) )
  {
    vnum = atoi(buff);
  }
  else
  {
    send_to_char("Syntax: guildhall move <id> <here|vnum>\r\n", ch);
    return;
  }

  if(!real_room0(vnum))
  {
    send_to_char("Invalid room vnum.\r\n", ch);
    return;
  }

  int old_vnum = gh->outside_vnum;
  
  if( move_guildhall(gh, vnum) )
  {
    logit(LOG_GUILDHALLS, "%s moved guildhall %d (%s) from %d to %d", GET_NAME(ch), gh->id, strip_ansi(GET_ASSOC(ch)->get_name().c_str()).c_str(), old_vnum, vnum);
    send_to_char("Guildhall moved!\r\n", ch);
  }
  else
  {
    send_to_char("Something went wrong...\r\n", ch);
  }
}

//
// guildhall 'direct' functions - 
//  these are where things are actually constructed/removed/etc, so we can keep
//  that logic separate from the cost calculation/etc

/*
 Constructs a new main guildhall with a standard template of rooms:
 
 I-H-P
   |
   E
   |
 
 I: inn H: heartstone P: portal room E: entrance
 
 South from entrance leads back to outside guildhall
 */
bool construct_main_guildhall(P_Guild guild, int outside_vnum, int racewar)
{
  Guildhall *gh = new Guildhall();
  gh->id = next_guildhall_id();
  gh->guild = guild;
  gh->type = GH_TYPE_MAIN;
  gh->outside_vnum = outside_vnum;
  gh->racewar = racewar;
  
  // add rooms
  GuildhallRoom *entrance = new GuildhallRoom();
  entrance->id = next_guildhall_room_id();
  entrance->type = GH_ROOM_TYPE_ENTRANCE;
  entrance->value[GH_VALUE_ENTRANCE_DIR] = SOUTH;
  entrance->vnum = next_guildhall_room_vnum();
  gh->add_room(entrance);
  
  GuildhallRoom *heartstone = new GuildhallRoom();
  heartstone->id = next_guildhall_room_id();
  heartstone->type = GH_ROOM_TYPE_HEARTSTONE;
  heartstone->vnum = next_guildhall_room_vnum();
  gh->add_room(heartstone);
  
  GuildhallRoom *portal = new GuildhallRoom();
  portal->id = next_guildhall_room_id();
  portal->type = GH_ROOM_TYPE_PORTAL;
  portal->vnum = next_guildhall_room_vnum();
  gh->add_room(portal);
  
  GuildhallRoom *inn = new GuildhallRoom();
  inn->id = next_guildhall_room_id();
  inn->type = GH_ROOM_TYPE_INN;
  inn->vnum = next_guildhall_room_vnum();
  gh->add_room(inn);
  
  entrance->exits[NORTH] = heartstone->vnum;
  heartstone->exits[SOUTH] = entrance->vnum;
  heartstone->exits[WEST] = inn->vnum;
  heartstone->exits[EAST] = portal->vnum;
  inn->exits[EAST] = heartstone->vnum;
  portal->exits[WEST] = heartstone->vnum;
  
  if( !gh->save() )
  {
    logit(LOG_GUILDHALLS, "construct_main_guildhall(): couldn't save new guildhall!");
    delete(gh);
    return FALSE;
  }
  
  Guildhall::add(gh);
  
  return gh->reload();
}

bool construct_new_guildhall_room(int id, int from_vnum, int dir)
{
  if( !from_vnum || dir < 0 || dir >= NUM_EXITS || !real_room0(from_vnum) )
  {
    return FALSE;
  }
  
  Guildhall *gh = Guildhall::find_by_id(id);
  
  if( !gh )
  {
    return FALSE;
  }
  
  GuildhallRoom *from_room = gh->find_room_by_vnum(from_vnum);
  
  if( !from_room )
  {
    return FALSE;
  }
  
  if( from_room->has_exit(dir) || world[real_room0(from_room->vnum)].dir_option[dir] )
  {
    return FALSE;
  }
  
  GuildhallRoom *room = new GuildhallRoom();
  room->id = next_guildhall_room_id();
  room->type = GH_ROOM_TYPE_GENERIC;
  room->vnum = next_guildhall_room_vnum();
  room->exits[rev_dir[dir]] = from_room->vnum;
  gh->add_room(room);
  
  from_room->exits[dir] = room->vnum;
  
  if( !gh->save() )
  {
    logit(LOG_GUILDHALLS, "construct_new_guildhall_room(): couldn't save guildhall!");
    return FALSE;
  }
  
  return gh->reload();
}

bool construct_golem(Guildhall *gh, int slot, int type)
{
  if(!gh || !gh->entrance_room)
    return FALSE;
  
  gh->entrance_room->value[GH_GOLEM_SLOT+slot] = type;

  if( !gh->save() )
  {
    logit(LOG_GUILDHALLS, "construct_golem(): couldn't save guildhall!");
    return FALSE;
  }
  
  return gh->reload();
}

bool destroy_guildhall(int id)
{
  if( Guildhall *gh = Guildhall::find_by_id(id) )
  {
    gh->deinit();
    gh->destroy();
    Guildhall::remove(gh);
    return TRUE;
  }
  return FALSE;  
}

bool reload_guildhall(int id)
{
  if( Guildhall *gh = Guildhall::find_by_id(id) )
  {
    return gh->reload();
  }
  return FALSE;  
}

bool upgrade_room(GuildhallRoom *room, int type)
{
  if( !room || !room->can_upgrade() )
    return FALSE;
  
  Guildhall *gh = room->guildhall;
  
  if(!gh)
    return FALSE;
  
  if( type < 0 || type >= GH_ROOM_NUM_TYPES )
    return FALSE;
  
  room->type = type;
  
  if( !gh->save() )
  {
    logit(LOG_GUILDHALLS, "upgrade_room(): couldn't save guildhall!");
    return FALSE;
  }
  
  return gh->reload();
}

bool rename_room(GuildhallRoom *room, char *name)
{
  room->name = string(name) + string("&n"); // add &n on the end to prevent ansi bleed
  
  if( !room->guildhall->save() )
  {
    logit(LOG_GUILDHALLS, "rename_room(): couldn't save guildhall!");
    return FALSE;
  }
  
  return room->guildhall->reload();  
}

void guildhall_info(Guildhall *gh, P_char ch)
{
  if(!gh || !ch)
    return;
  
  // display guildhall information 
  
  char buff[MAX_STRING_LENGTH];
  
  sprintf(buff, "&+BGuildhall in &n%s\r\n", world[real_room0(gh->outside_vnum)].name);
  send_to_char(buff, ch);

  sprintf(buff, "&+BOwned by&n %s\r\n", gh->guild->get_name().c_str());
  send_to_char(buff, ch);

  sprintf(buff, "&+BRooms:&n %d\r\n", (int) gh->rooms.size());
  send_to_char(buff, ch);
  
  send_to_char("\r\n&+BGolems\r\n"
               "&+b----------------------------------------------\r\n", ch);
  
  for( int i = 0; i < GH_GOLEM_NUM_SLOTS; i++ )
  {
    if(gh->entrance_room->golems[i])
    {
      sprintf(buff, "&+W%d&n) %s &n(%s&n)\r\n", (i+1), pad_ansi(gh->entrance_room->golems[i]->player.short_descr, 30).c_str(), condition_str(gh->entrance_room->golems[i]));
      send_to_char(buff, ch);
    }
    else
    {
      sprintf(buff, "&+W%d&n) none\r\n", (i+1));
      send_to_char(buff, ch);
    }
  }
}

bool move_guildhall(Guildhall *gh, int vnum)
{
  if(!real_room0(vnum))
    return FALSE;
  
  gh->outside_vnum = vnum;
  
  if( !gh->save() )
  {
    logit(LOG_GUILDHALLS, "move_guildhall(): couldn't save guildhall!");
    return FALSE;
  }
  
  return gh->reload();
}

bool guildhall_map_check(P_char ch)
{
  int rroom = ch->in_room;
  
  if(!rroom)
    return FALSE;

  //making guildhalls only avail in towns - 8/22/13 Drannak
  if (IS_RACEWAR_GOOD(ch) && IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_EVILHOME))
  {
    send_to_char("Sure, call in the contractors!... Try to find a good hometown to build in.\n", ch);
    return FALSE;
  }
  else if (IS_RACEWAR_EVIL(ch) && IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_GOODHOME))
  {
    send_to_char("Sure, call in the contractors!... Try to find an evil hometown to build in.\n", ch);
    return FALSE;
  }

 //dranfat 
  if( Guildhall::find_by_vnum(world[rroom].number) )
  {
    send_to_char("There is already a guildhall here.\r\n", ch);
    return FALSE;
  }

  if((!IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_EVILHOME)) && (!IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_GOODHOME)))
  {
    send_to_char("Guildhalls can only be built within the confines of a city.\r\n", ch);
    return FALSE;
  }
  
 /* if( GET_RACEWAR(ch) == RACEWAR_GOOD )
  {
    if( !IS_CONTINENT(rroom, CONT_GC) )
    {
      send_to_char("You can only build a guildhall on the &+WGood Continent&n.\r\n", ch);
      return FALSE;
    }
    
    int dist = calculate_map_distance(ch->in_room, real_room(TH_MAP_VNUM));
    
    if( dist )
      dist = (int) sqrt(dist); // calculate_map_distance returns the square of the distance
    
    //if( dist < 0 || dist > MAX_GH_HOMETOWN_RADIUS )
    //{
      //send_to_char("You need to build your guildhall closer to Tharnadia.\r\n", ch);
      //return FALSE;
    //}
  }

  if( GET_RACEWAR(ch) == RACEWAR_EVIL )
  {
    if (IS_CONTINENT(rroom, CONT_EC))
    {
      int dist = calculate_map_distance(ch->in_room, real_room(SHADY_MAP_VNUM));

      if( dist )
        dist = (int) sqrt(dist); // calculate_map_distance returns the square of the distance

      if( dist < 0 || dist > MAX_GH_HOMETOWN_RADIUS )
      {
        send_to_char("You need to build your guildhall closer to Shady Grove.\r\n", ch);
        return FALSE;
      }
    }
    else if (IS_UD_MAP(ch->in_room))
    {
      int dist = calculate_map_distance(ch->in_room, real_room(KHILD_MAP_VNUM));

      if (dist)
	dist = (int) sqrt(dist);
      if (dist < 0 || dist > MAX_GH_HOMETOWN_RADIUS)
      {
	send_to_char("You need to build your guildhall closer to Khildarak.\r\n", ch);
	return FALSE;
      }
    }
    else
    {
      send_to_char("You can only build a guildhall on the &+LEvil Continent&n or Underdark.\r\n", ch);
      return FALSE;
    }
  }*/
  
  for( int i = 0; i < Guildhall::guildhalls.size(); i++ )
  {
    Guildhall* gh = Guildhall::guildhalls[i];
    
    int dist = calculate_map_distance(ch->in_room, real_room(gh->outside_vnum));
    
    if( dist )
      dist = (int) sqrt(dist); // calculate_map_distance returns the square of the distance
   
    if( dist >= 0 && dist < MAX_GH_PROXIMITY_RADIUS )
    {
      send_to_char("You can't build your hall so close to other guildhalls.\r\n", ch);
      return FALSE;
    }
  }  
  
 /* if( world[rroom].sector_type == SECT_FOREST ||
      world[rroom].sector_type == SECT_HILLS ||
      world[rroom].sector_type == SECT_FIELD ||
      IS_UD_MAP(rroom))*/
  if(((IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_EVILHOME)) || (IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_GOODHOME)))
	&& (world[rroom].sector_type == SECT_CITY || world[rroom].sector_type == SECT_INSIDE || world[rroom].sector_type == SECT_UNDRWLD_CITY))
  {
    return TRUE;
  }
  else
  {
   // send_to_char("You can't build your guildhall on this terrain.\r\n", ch);
    send_to_char("You can only construct a guildhall within the confines of a town.\r\n", ch);
    return FALSE;
  }
  
  return FALSE;
}

P_obj find_gh_library_book_obj(P_char ch)
{
  if(!ch)
    return NULL;
  
  LibraryRoom *room = Guildhall::find_library_by_vnum(world[ch->in_room].number);
  
  if(!room)
    return NULL;
  
  if(!room->guildhall)
    return NULL;
  
  if(GET_ASSOC(ch) != room->guild)
    return NULL;
  
  return room->tome;
}
