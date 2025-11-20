#include "comm.h"
#include "damage.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "justice.h"
#include "objmisc.h"
#include "prototypes.h"
#include "ships.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "siege.h"
#include <string.h>

P_siege  siege_objects;         /* List of siege objects to save   */

extern P_room world;
extern int top_of_objt;
extern P_town towns;
extern P_char destroying_list;
extern int top_of_zone_table;

// Dependent on ch's str and weight.  Pretty simple atm.
int siege_move_wait( P_char ch )
{
  // Base: 100 str, 100 weight, 2 secs
  int retval = 100*100*2*WAIT_SEC;

  retval /= GET_C_STR( ch );
  retval /= ch->player.weight;

  return retval;
}

// Based on chars str and agi right now.
int siege_load_wait( P_char ch, P_obj engine )
{
  // Base: 100 str, 100 agi, 2 secs
  int retval = 100*100*10*WAIT_SEC;

  retval /= GET_C_STR( ch );
  retval /= GET_C_AGI( ch );

  return retval;
}

void event_load_engine(P_char ch, P_char victim, P_obj obj, void *data)
{
  act( "You finish loading $p.", FALSE, ch, obj, NULL, TO_CHAR );
  act( "$n finishes loading $p.", TRUE, ch, obj, 0, TO_ROOM);
  obj->value[3]--;
  obj->value[2] = 1;
}

// Having to write this is a pain! Ok.. so not so bad.
// Char can move if exit is unblocked and ch not fighting.
bool can_move( P_char ch, int dir )
{
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
    return FALSE;

  if( !EXIT( ch, dir ) )
    return FALSE;
  if( !can_enter_room(ch, EXIT(ch, dir)->to_room, FALSE) )
    return FALSE;

  if( IS_CLOSED( ch->in_room, dir ) )
    return FALSE;
  if( IS_HIDDEN( ch->in_room, dir ) )
    return FALSE;
  if( IS_WALLED( ch->in_room, dir ) )
    return FALSE;

  return TRUE;
}

// This proc is for a ballista(OBJ # 461).
int ballista( P_obj obj, P_char ch, int cmd, char *arg )
{
  char   arg1[MAX_STRING_LENGTH];
  char   arg2[MAX_STRING_LENGTH];
  char   arg3[MAX_STRING_LENGTH];
  char   buf[MAX_STRING_LENGTH];
  int    num_rooms;
  int    in_room, ch_room;
  P_obj  ammo;
  int    dir;
  P_char vict;
  P_obj  target;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;
  if( cmd == CMD_PUSH )
  {
    // Parse argument.
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;
      // Figure out what direction.
      if( is_abbrev(arg2, "north") )
        dir = DIR_NORTH;
      else if( is_abbrev(arg2, "south") )
        dir = DIR_SOUTH;
      else if( is_abbrev(arg2, "east") )
        dir = DIR_EAST;
      else if( is_abbrev(arg2, "west") )
        dir = DIR_WEST;
      if( dir == -1 || *arg2 == '\0' )
      {
        send_to_char( "Move the ballista what direction?\n", ch );
        return TRUE;
      }
      else
      {
        // If there is no exit in that direction, or it's blocked.
        if( !can_move( ch, dir ) )
        {
          send_to_char( "You can't leave that way right now.\n", ch );
          return TRUE;
        }
        // Move it that direction.
        snprintf(buf, MAX_STRING_LENGTH, "You begin to push $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        snprintf(buf, MAX_STRING_LENGTH, "$n starts pushing $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);
        // Yes, this lags you to stop you from spamming.
        CharWait(ch, siege_move_wait(ch) );
        add_event(event_move_engine, siege_move_wait(ch), ch, NULL, 
            obj, 0, &dir, sizeof(int) );
      }
      return TRUE;
    }
  }
  else if( cmd == CMD_RELOAD )
  {
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) != obj ) 
      return FALSE;
    if( obj->value[2] > 0 )
      act( "$p is already loaded.", FALSE, ch, obj, NULL, TO_CHAR );
    else if( obj->value[3] <= 0 )
      act( "$p has no more ammo.", FALSE, ch, obj, NULL, TO_CHAR );
    else if( is_loading_siege( obj ) )
      act( "$p is already being reloaded.", FALSE, ch, obj, NULL, TO_CHAR );
    else
    {
      act( "You begin loading $p.", FALSE, ch, obj, NULL, TO_CHAR );
      act( "$n begins loading $p.", FALSE, ch, obj, NULL, TO_ROOM );
      add_event(event_load_engine, siege_load_wait(ch, obj), ch, NULL, 
          obj, 0, NULL, 0 );
      // Yes, this lags you while loading.
      CharWait(ch, siege_move_wait(ch) );
    }
    return TRUE;
  }
  else if( cmd == CMD_FIRE )
  {
    // Parse argument.
    half_chop(arg, arg1, arg3);
    half_chop(arg3, arg2, arg3);

    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;

      if( obj->value[2] == 0 )
      {
        act( "$p has no ammo.  Try to reload it first.", FALSE, ch, obj, NULL, TO_CHAR );
        return TRUE;
      }

      // Figure out what direction.
      if( is_abbrev(arg3, "north") )
        dir = DIR_NORTH;
      else if( is_abbrev(arg3, "south") )
        dir = DIR_SOUTH;
      else if( is_abbrev(arg3, "east") )
        dir = DIR_EAST;
      else if( is_abbrev(arg3, "west") )
        dir = DIR_WEST;
      if( dir == -1 || *arg3 == '\0' )
      {
        send_to_char( "Fire the ballista at who what direction?\n", ch );
        return TRUE;
      }
      else
      {
        snprintf(buf, MAX_STRING_LENGTH, "You fire $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        snprintf(buf, MAX_STRING_LENGTH, "$n fires $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);

        if( obj->loc_p != LOC_ROOM )
        {
          logit(LOG_DEBUG, "ballista: firing siege weapon not in a room.");
          return FALSE;
        }
        in_room = obj->loc.room;
        if( !in_room )
        {
          logit(LOG_DEBUG, "ballista: firing siege weapon in room 0.");
          return FALSE;
        }
        // Load exploded ammo into the room.
        ammo = read_object(real_object(obj->value[0]), REAL);
        if( !ammo )
        {
          logit(LOG_DEBUG, "ballista: couldn't load ammo.");
          return FALSE;
        }
        obj->value[2] = 0;
        obj_to_room( ammo, in_room );
        vict = NULL;
        target = NULL;
        // Fire the weapon 3x spaces to the dir. Hits walls.
        for( num_rooms = 0;num_rooms<3;num_rooms++)
        {
          ch_room = ch->in_room;
          ch->in_room = in_room;
          // Impale the target!
          if( num_rooms > 0 && !vict && (vict = get_char_room_vis(ch, arg2)) )
          {
            act( "$p impales $n!", TRUE, vict, ammo, 0, TO_ROOM );
            act( "$p impales YOU!", TRUE, vict, ammo, 0, TO_CHAR );
            char_from_room( vict );
          }
          // If we hit target siege object.
          if( num_rooms > 0 && !vict && (target = get_siege_room( ch, arg2)) )
          {
            snprintf(buf, MAX_STRING_LENGTH, "%s slams into $p", ammo->short_description );
            act( buf, TRUE, NULL, target, 0, TO_ROOM );
            act( buf, TRUE, ch, target, 0, TO_CHAR );
            damage_siege( target, ammo );
            ch->in_room = ch_room;
            return TRUE;
          }
          ch->in_room = ch_room;
          // If we hit a wall.
          if(  !VIRTUAL_EXIT(in_room, dir)
              || !VIRTUAL_EXIT(in_room, dir)->to_room )
          {
            snprintf(buf, MAX_STRING_LENGTH, "$p hits the %s wall.", dirs[dir] );
            act( buf, TRUE, NULL, ammo, 0, TO_ROOM );
            break;
          }
          else
          {
            in_room = VIRTUAL_EXIT(in_room, dir)->to_room;
            obj_from_room(ammo);
            obj_to_room(ammo, in_room);
            act( "$p flies through the room..", TRUE, NULL, ammo, 0, TO_ROOM );
          }
        }
        ch_room = ch->in_room;
        ch->in_room = ammo->loc.room;
        // Missile go SMACK!
        // If victim was impaled...
        if( vict )
        {
          char_to_room( vict, ammo->loc.room, -1 );
          act( "$p slams into the ground with $n impaled upon it.", TRUE, vict, ammo, NULL, TO_ROOM );
          act( "You hit the ground and bounce off of $p.", TRUE, vict, ammo, NULL, TO_CHAR );
        }
        else if ((target = get_siege_room( ch, arg2)))
        {
          snprintf(buf, MAX_STRING_LENGTH, "%s slams into $p", ammo->short_description );
          act( buf, TRUE, NULL, target, 0, TO_ROOM );
          act( buf, TRUE, ch, target, 0, TO_CHAR );
          damage_siege( target, ammo );
        }
        // If victim in final room...
        else if( (vict = get_char_room_vis(ch, arg2)) != NULL )
        {
          act( "$p slams into $n before hitting the dirt.", TRUE, vict, ammo, NULL, TO_ROOM );
          act( "$p slams into you before hitting the dirt.", TRUE, vict, ammo, NULL, TO_CHAR );
        }
        // Miss!
        else
          act( "$p slams into the ground.", TRUE, NULL, ammo, NULL, TO_ROOM );
        if( vict )
          damage(ch, vict, dice( ammo->value[0], ammo->value[1] * 3 ), TYPE_UNDEFINED );
        ch->in_room = ch_room;
        return TRUE;
      }
    }
  }
  return FALSE;
}

// This proc is for a battering ram(OBJ # 462).
int battering_ram( P_obj obj, P_char ch, int cmd, char *arg )
{
  char  arg1[MAX_STRING_LENGTH];
  char  arg2[MAX_STRING_LENGTH];
  char  buf[MAX_STRING_LENGTH];
  int   dir;
  P_obj target;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;
  if( cmd == CMD_PUSH )
  {
    // Parse argument.
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;
      // Figure out what direction.
      if( is_abbrev(arg2, "north") )
        dir = DIR_NORTH;
      else if( is_abbrev(arg2, "south") )
        dir = DIR_SOUTH;
      else if( is_abbrev(arg2, "east") )
        dir = DIR_EAST;
      else if( is_abbrev(arg2, "west") )
        dir = DIR_WEST;
      if( dir == -1 || *arg2 == '\0' )
      {
        send_to_char( "Move the battering ram what direction?\n", ch );
        return TRUE;
      }
      else
      {
        // If there is no exit in that direction, or it's blocked.
        if( !can_move( ch, dir ) )
        {
          send_to_char( "You can't leave that way right now.\n", ch );
          return TRUE;
        }
        // Move it that direction.
        snprintf(buf, MAX_STRING_LENGTH, "You begin to push $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        snprintf(buf, MAX_STRING_LENGTH, "$n starts pushing $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);
        // Yes, this lags you to stop you from spamming.
        CharWait(ch, siege_move_wait(ch) );
        add_event(event_move_engine, siege_move_wait(ch), ch, NULL, 
            obj, 0, &dir, sizeof(int) );
      }
      return TRUE;
    }
  }
  else if( cmd == CMD_THRUST )
  {
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      if( (target = get_siege_room( ch, arg2)) != NULL )
      {
        if( obj == target )
        {
          act( "You push $p around in circles.", FALSE, ch, target, NULL, TO_CHAR );
          act( "$n pushes $p around in circles.", FALSE, ch, target, NULL, TO_ROOM );
          return TRUE;
        }

        // Yes, this lags you to stop you from spamming.
        CharWait(ch, siege_move_wait(ch) / 2 );

        snprintf(buf, MAX_STRING_LENGTH, "You thrust %s at $p!", obj->short_description );
        act( buf, FALSE, ch, target, NULL, TO_CHAR );
        snprintf(buf, MAX_STRING_LENGTH, "$n thrusts %s at $p!", obj->short_description );
        act( buf, FALSE, ch, target, NULL, TO_ROOM );

        damage_siege( target, obj );
      }
      else
        act( "Thrust $p at what?", FALSE, ch, obj, NULL, TO_CHAR );
      return TRUE;
    }
  }
  return FALSE;
}

// This proc is for a catapult(OBJ # 463).
int catapult( P_obj obj, P_char ch, int cmd, char *arg )
{
  char  arg1[MAX_STRING_LENGTH];
  char  arg2[MAX_STRING_LENGTH];
  char  buf[MAX_STRING_LENGTH];
  int   dir;
  int   num_rooms;
  int   in_room;
  P_obj ammo;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;
  if( cmd == CMD_PUSH )
  {
    // Parse argument.
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;
      // Figure out what direction.
      if( is_abbrev(arg2, "north") )
        dir = DIR_NORTH;
      else if( is_abbrev(arg2, "south") )
        dir = DIR_SOUTH;
      else if( is_abbrev(arg2, "east") )
        dir = DIR_EAST;
      else if( is_abbrev(arg2, "west") )
        dir = DIR_WEST;
      if( dir == -1 || *arg2 == '\0' )
      {
        send_to_char( "Move the catapult what direction?\n", ch );
        return TRUE;
      }
      else
      {
        // If there is no exit in that direction, or it's blocked.
        if( !can_move( ch, dir ) )
        {
          send_to_char( "You can't leave that way right now.\n", ch );
          return TRUE;
        }
        // Move it that direction.
        snprintf(buf, MAX_STRING_LENGTH, "You begin to push $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        snprintf(buf, MAX_STRING_LENGTH, "$n starts pushing $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);
        // Yes, this lags you to stop you from spamming.
        CharWait(ch, siege_move_wait(ch) );
        add_event(event_move_engine, siege_move_wait(ch), ch, NULL, 
            obj, 0, &dir, sizeof(int) );
      }
      return TRUE;
    }
  }
  else if( cmd == CMD_RELOAD )
  {
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) != obj )
      return FALSE;
    if( obj->value[2] > 0 )
      act( "$p is already loaded.", FALSE, ch, obj, NULL, TO_CHAR );
    else if( obj->value[3] <= 0 )
      act( "$p has no more ammo.", FALSE, ch, obj, NULL, TO_CHAR );
    else if( is_loading_siege( obj ) )
      act( "$p is already being reloaded.", FALSE, ch, obj, NULL, TO_CHAR );
    else
    {
      act( "You begin loading $p.", FALSE, ch, obj, NULL, TO_CHAR );
      act( "$n begins loading $p.", FALSE, ch, obj, NULL, TO_ROOM );
      add_event(event_load_engine, siege_load_wait(ch, obj), ch, NULL, 
          obj, 0, NULL, 0 );
      // Yes, this lags you while loading.
      CharWait(ch, siege_move_wait(ch) );
    }
    return TRUE;
  }
  else if( cmd == CMD_FIRE )
  {
    // Parse argument.
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;

      if( obj->value[2] == 0 )
      {
        act( "$p has no ammo.  Try to reload it first.", FALSE, ch, obj, NULL, TO_CHAR );
        return TRUE;
      }

      // Figure out what direction.
      if( is_abbrev(arg2, "north") )
        dir = DIR_NORTH;
      else if( is_abbrev(arg2, "south") )
        dir = DIR_SOUTH;
      else if( is_abbrev(arg2, "east") )
        dir = DIR_EAST;
      else if( is_abbrev(arg2, "west") )
        dir = DIR_WEST;
      if( dir == -1 || *arg2 == '\0' )
      {
        send_to_char( "Fire the catapult what direction?\n", ch );
        return TRUE;
      }
      else
      {
        snprintf(buf, MAX_STRING_LENGTH, "You fire $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        snprintf(buf, MAX_STRING_LENGTH, "$n fires $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);

        if( obj->loc_p != LOC_ROOM )
        {
          logit(LOG_DEBUG, "catapult: firing siege weapon not in a room.");
          return FALSE;
        }
        in_room = obj->loc.room;
        if( !in_room )
        {
          logit(LOG_DEBUG, "catapult: firing siege weapon in room 0.");
          return FALSE;
        }
        // Load exploded ammo into the room.
        ammo = read_object(real_object(obj->value[0]), REAL);
        obj->value[2] = 0;
        obj_to_room( ammo, in_room );
        // Fire the weapon 4x spaces to the dir. Hits walls.
        for( num_rooms = 0;num_rooms<4;num_rooms++)
        {
          // If we hit a wall.
          if(  !VIRTUAL_EXIT(in_room, dir)
              || !VIRTUAL_EXIT(in_room, dir)->to_room )
          {
            snprintf(buf, MAX_STRING_LENGTH, "$p slams into the %s wall.", dirs[dir] );
            act( buf, TRUE, NULL, ammo, 0, TO_ROOM );
            return TRUE;
          }
          else
          {
            in_room = VIRTUAL_EXIT(in_room, dir)->to_room;
            obj_from_room(ammo);
            obj_to_room(ammo, in_room);
            act( "$p flies overhead..", TRUE, NULL, ammo, 0, TO_ROOM );
          }
        }
        // Ammo lands in to_room.. BOOM, SPLAT!
        act( "$p explodes overhead!", TRUE, NULL, ammo, 0, TO_ROOM );
        explode_ammo( ch, ammo );
        return TRUE;
      }
    }
  }
  return FALSE;
}

// Attempt to move the siege engine a direction.
void event_move_engine(P_char ch, P_char victim, P_obj obj, void *data)
{
  char buf[MAX_STRING_LENGTH];
  int dir = *((int *)data);
  int to_room;

  // If there's an exit and it's unblocked (and ch isn't fighting).
  if( can_move( ch, dir ) )
  {
    snprintf(buf, MAX_STRING_LENGTH, "You push $p %sward.", dirs[dir] );
    act( buf, FALSE, ch, obj, NULL, TO_CHAR );
    snprintf(buf, MAX_STRING_LENGTH, "$n pushes $p %sward.", dirs[dir] );
    act( buf, TRUE, ch, obj, 0, TO_ROOM);

    to_room = EXIT(ch, dir)->to_room;
    obj_from_room(obj);
    obj_to_room(obj, to_room);

    char_from_room( ch );
    char_to_room( ch, to_room, dir );

    snprintf(buf, MAX_STRING_LENGTH, "$n pushes $p in from the %s.", dirs[rev_dir[dir]] );
    act( buf, TRUE, ch, obj, 0, TO_ROOM);
  }
  else
    send_to_char( "You can't leave that way right now.\n", ch );

}

// This will probably want an update with ammo types.
void explode_ammo( P_char ch, P_obj ammo )
{
  P_char vict, next_vict;
  P_obj obj, next_obj;

  // Splat ppl!
  for( vict = world[ammo->loc.room].people;vict;vict = next_vict )
  {
    next_vict = vict->next_in_room;
    damage(ch, vict, dice( ammo->value[0], ammo->value[1] * 3 ), TYPE_UNDEFINED );
  }

  // Splat other siege weapons.
  for( obj = world[ammo->loc.room].contents;obj;obj = next_obj )
  {
    next_obj = obj->next_content;
    if( is_siege( obj ) )
      damage_siege( obj, ammo );
  }
}

// Returns TRUE iff ch is blocked by gates.
bool check_gates( P_char ch, int room )
{
  P_obj wall;
  int dir;

  for( dir = 0; dir < NUM_EXITS; dir++ )
  {
    if( world[ch->in_room].dir_option[dir]
        && world[ch->in_room].dir_option[dir]->to_room == room )
      break;
  }
  // If no exit to room, no gate can block it.
  if( dir == NUM_EXITS )
    return FALSE;

  wall = world[ch->in_room].contents;
  while( wall )
  {
    if( obj_index[wall->R_num].func.obj == castlewall )
      break;
    wall = wall->next_content;
  }
  if( !wall )
    return FALSE;

  // The direction the wall blocks is the dir ch wants to go.
  if( wall->value[0] == dir )
    return TRUE;
  return FALSE;
}

// This is the proc for castle walls.  Just a placeholder atm.
int castlewall( P_obj obj, P_char ch, int cmd, char *arg )
{
  if( cmd == CMD_SET_PERIODIC )
    return FALSE;

  if( !ch || !ch->in_room )
    return FALSE;
  if( !obj )
  {
    logit(LOG_DEBUG, "castlewall: proc called with no wall!");
    return FALSE;
  }

  // Is ch an invader of the town?
  if( !IS_INVADER( ch ) )
    return FALSE;

  // Value0 of a castlewall is the direction it blocks.
  if( (obj->value[0] == DIR_EAST  && cmd == CMD_EAST) 
      || (obj->value[0] == DIR_WEST  && cmd == CMD_WEST) 
      || (obj->value[0] == DIR_NORTH && cmd == CMD_NORTH) 
      || (obj->value[0] == DIR_SOUTH && cmd == CMD_SOUTH) )
  {
    if( isname( "gates", obj->name ) )
      act( "$p block your path.", FALSE, ch, obj, NULL, TO_CHAR );
    else
      act( "$p blocks your path.", FALSE, ch, obj, NULL, TO_CHAR );
    return TRUE;
  }

  return FALSE;
}

// Returns TRUE iff weapon is a siege object.
bool is_siege( P_obj object )
{
  if( !object )
    return FALSE;

  // If the object proc is ballista/battering_ram/catapult..
  if(  obj_index[object->R_num].func.obj == ballista
      || obj_index[object->R_num].func.obj == battering_ram
      || obj_index[object->R_num].func.obj == catapult
      || obj_index[object->R_num].func.obj == castlewall )
    return TRUE;

  return FALSE;
}

void damage_siege( P_obj siege, P_obj ammo )
{
  char  buf[MAX_STRING_LENGTH];
  bool  destroy = FALSE;
  int   damage;
  P_obj scraps;

  damage = dice( ammo->value[0], ammo->value[1] );

  switch( siege->material )
  {
    default:
      break;
    case MAT_NONSUBSTANTIAL:
    case MAT_LIQUID:
    case MAT_WAX:
      damage *= 10;
      break;
    case MAT_PAPER:
    case MAT_PARCHMENT:
    case MAT_LEAVES:
    case MAT_FLESH:
    case MAT_RUBBER:
    case MAT_FEATHER:
      damage *= 5;
      break;
    case MAT_CLOTH:
      damage = ( damage * 10 ) / 3;
      break;
    case MAT_BAMBOO:
    case MAT_REEDS:
    case MAT_HEMP:
    case MAT_EGGSHELL:
    case MAT_BARK:
    case MAT_GENERICFOOD:
      damage = ( damage * 5 ) / 2;
      break;
    case MAT_SOFTWOOD:
    case MAT_HARDWOOD:
    case MAT_PEARL:
      damage = ( damage * 9 ) / 4;
      break;
    case MAT_SILICON:
    case MAT_CERAMIC:
      damage = ( damage * 5 ) / 3;
      break;
    case MAT_HIDE:
    case MAT_LEATHER:
    case MAT_CURED_LEATHER:
      damage = ( damage * 7 ) / 3;
      break;
    case MAT_CRYSTAL:
    case MAT_BONE:
      damage = ( damage * 7 ) / 3;
      break;
    case MAT_GEM:
    case MAT_STONE:
    case MAT_GRANITE:
    case MAT_MARBLE:
    case MAT_LIMESTONE:
      damage = ( damage * 85 ) / 100;
      break;
    case MAT_IRON:
    case MAT_STEEL:
      damage = ( damage * 75 ) / 100;
      break;
    case MAT_BRASS:
    case MAT_MITHRIL:
      damage = ( damage * 70 ) / 100;
      break;
    case MAT_GLASSTEEL:
    case MAT_ADAMANTIUM:
      damage = ( damage * 65 ) / 100;
      break;
    case MAT_BRONZE:
    case MAT_COPPER:
    case MAT_SILVER:
    case MAT_ELECTRUM:
    case MAT_GOLD:
    case MAT_PLATINUM:
    case MAT_RUBY:
    case MAT_EMERALD:
    case MAT_SAPPHIRE:
    case MAT_IVORY:
      damage = ( damage * 89 ) / 100;
      break;
    case MAT_DRAGONSCALE:
    case MAT_DIAMOND:
      damage = ( damage * 67 ) / 100;
      break;
    case MAT_OBSIDIAN:
      damage = ( damage * 63 ) / 100;
      break;
    case MAT_CHITINOUS:
    case MAT_REPTILESCALE:
      damage = ( damage * 95 ) / 100;
      break;
  }

  // This should never happen, but just to make sure we don't heal target...
  if( damage < 1 )
    damage = 1;

  siege->condition -= damage;
  if( siege->condition <= 0 )
    destroy = TRUE;

  snprintf(buf, MAX_STRING_LENGTH, "$q %s", destroy ? "is completely destroyed!" :
      "is damaged from the blow!" );
  act(buf, TRUE, NULL, siege, 0, TO_ROOM);

  if( destroy )
  {
    act("$p collapses into scraps.", TRUE, NULL, siege, 0, TO_ROOM);
    scraps = read_object(9, VIRTUAL);
    if( !scraps )
    {
      remove_siege( siege );
      extract_obj( siege, TRUE ); // Siege arti?
      return;
    }
    snprintf(buf, MAX_STRING_LENGTH, "Scraps from %s&n lie in a pile here.",
        siege->short_description);
    scraps->description = str_dup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "a pile of scraps from %s", siege->short_description);
    scraps->short_description = str_dup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s scraps pile", siege->name);
    scraps->name = str_dup(buf);
    scraps->str_mask = STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_KEYS;
    set_obj_affected(scraps, 400, TAG_OBJ_DECAY, 0);
    obj_to_room(scraps, siege->loc.room);

    remove_siege( siege );
    extract_obj( siege, TRUE ); // Siege arti?
  }
}

// Find a siege object in ch's room.
P_obj get_siege_room( P_char ch, char *arg )
{
  P_obj obj;
  char *temp = arg;
  int   howmany;
  int   count = 0;

  // Need to handle 2.siege.
  howmany = get_number( &temp );

  for( obj = world[ch->in_room].contents; obj; obj = obj->next_content )
  {
    // If it is a siege weapon, and matches name, and matches amount..
    if( is_siege( obj ) && isname( temp, obj->name ) && howmany == ++count )
      return obj;
  }

  return NULL;
}

// See if someone already started a reload event.
bool is_loading_siege( P_obj siege )
{
  P_nevent e;

  LOOP_EVENTS_OBJ( e, siege->nevents )
  {
    if( e->func == event_load_engine )
    {
      return TRUE;
    }
  }
  return FALSE;
}

// Returns the corresponding town to the zone (or null if none).
P_town gettown( P_char ch )
{
  P_town town = towns;
  while( town && town->zone != &(zone_table[world[ch->in_room].zone]) )
    town = town->next_town;
  return town;
}

// This is a proc for the warmaster mobs in towns that build
//   walls etc.
int warmaster( P_char ch, P_char pl, int cmd, char *arg )
{
  P_town town;
  char buf[MAX_STRING_LENGTH], arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char *rest;
  P_obj donation, siege;
  int rank;
  int ztop, count, i, j, numgate;

  if( cmd == CMD_SET_PERIODIC ) return FALSE;

  if( cmd == CMD_PERIODIC ) return FALSE;

  if( !ch )
  {
    logit(LOG_DEBUG, "warmaster: called with no warmaster.");
    return FALSE;
  }

  if( !pl )
  {
    logit(LOG_DEBUG, "warmaster: called with no player.");
    return FALSE;
  }
  // In case someone tries to have a pet do warmaster stuff.
  if( IS_NPC( pl ) )
  {
    return FALSE;
  }
  
  town = gettown( ch );
  
  if( !town )
  {
    logit(LOG_DEBUG, "warmaster: called outside of town.");
    return FALSE;
  }

  if(cmd == CMD_LIST)
  {
      // List town's name, offense, defense, and resrources..
    snprintf(buf, MAX_STRING_LENGTH, "'%s'\nOffense:     %7d\nDefense:     %7d\nResources:   %7d\n"
        "Town Guards:  %s\nTown Cavalry: %s\nTown Portals: %s\n\n",
        town->zone->name, town->offense, town->defense, town->resources, 
        town->deploy_guard ? "  Deployed" : "Not Deployed",
        town->deploy_cavalry ? "  Deployed" : "Not Deployed",
        town->deploy_portals ? "  Deployed" : "Not Deployed" );
    send_to_char( buf, pl );

    if( IS_TRUSTED( pl ) )
    {
      snprintf(buf, MAX_STRING_LENGTH, "Guards: %d deployed of %d max, vnum %d, load in room %d.\n", 
          town->guard_vnum ? mob_index[real_mobile(town->guard_vnum)].number : 0,
          town->guard_max, town->guard_vnum, town->guard_load_room );
      send_to_char( buf, pl );
      snprintf(buf, MAX_STRING_LENGTH, "Cavalry: %d deployed of %d max, vnum %d, load in room %d.\n\n", 
          town->cavalry_vnum ? mob_index[real_mobile(town->cavalry_vnum)].number : 0,
          town->cavalry_max, town->cavalry_vnum,  town->cavalry_load_room );
      send_to_char( buf, pl );
    }

    rank = GET_SURNAME(pl);
    if( rank == 0 )
    {
      do_say(ch, "Get out of here whelp! Earn a title before you come to me", CMD_SAY);
      return TRUE;
    }

    send_to_char( "You can donate an item to increase resources.\n", pl );
    if( rank <= SURNAME_COMMONER )
      return TRUE;
    send_to_char( "You can buy a catapult, ballista, or battering ram.\n", pl );
    if( town->resources >= 150000 )
      send_to_char( "You can buy town gates.\n", pl );
    if( rank <= SURNAME_NOBLE )
      return TRUE;
    if( town->resources >= 50000 )
      send_to_char( "You can deploy guards/stop deployment with the deploy command.\n", pl );
    if( town->resources >= 600000 )
      send_to_char( "You can deploy town portals/stop deployment with the deploy command.\n", pl );
    if( rank <= SURNAME_LORD )
      return TRUE;
    if( town->resources >= 450000 )
      send_to_char( "You can deploy cavalry/stop deployment with the deploy command.\n", pl );
    if( rank == SURNAME_KING )
      return TRUE;
    return TRUE;
  }
  if( cmd == CMD_DONATE )
  {
    donation = get_obj_in_list_vis(pl, arg, pl->carrying);
    if( !donation || IS_ARTIFACT(donation) )
    {
      send_to_char( "Donate what?!?\n", pl );
      return TRUE;
    }
    snprintf(buf, MAX_STRING_LENGTH, "You donate %s to %s.\n", donation->short_description, 
        town->zone->name );
    send_to_char( buf, pl );
    obj_from_char( donation );
    town->resources += itemvalue( donation );
    extract_obj( donation, TRUE ); // Will never bi arti, but ok.
    save_towns();
    return TRUE;
  }
  if( cmd == CMD_DEPLOY )
  {
    // These guys can't deploy guards..
    rank = GET_SURNAME(pl);
    if( rank <= SURNAME_NOBLE )
      return FALSE;
    one_argument( arg, arg1 );
    if( *arg1 == '\0' )
    {
      if( town->resources >= 600000 )
        send_to_char( "You can deploy: guards, calvary, or portals.\n", pl );
      else if( town->resources >= 450000 )
        send_to_char( "You can deploy: guards or calvary.\n", pl );
      else if( town->resources >= 50000 )
        send_to_char( "You can deploy: guards.\n", pl );
      return TRUE;
    }
    if( is_abbrev( arg1, "guards" ) )
    {
      if( town->resources < 50000 )
      {
        send_to_char( "This town lacks the resources to deploy guards.\n"
            "You must have 50,000 in resources to deploy them.\n", pl );
        return TRUE;
      }

      // Toggle deploy guard and save
      if( town->deploy_guard )
      {
        send_to_char( "Town guards will now not be deployed.\n", pl );
        town->deploy_guard = FALSE;
      }
      else
      {
        send_to_char( "Town guards will now be deployed.\n", pl );
        town->deploy_guard = TRUE;
      }
      save_towns();
      return TRUE;
    }
    else if( is_abbrev( arg1, "cavalry" ) )
    {
      if( GET_SURNAME(pl) < SURNAME_LORD )
        return FALSE;

      if( town->resources < 450000 )
      {
        send_to_char( "This town lacks the resources to deploy cavalry.\n"
            "You must have 450,000 in resources to deploy them.\n", pl );
        return TRUE;
      }

      // Toggle deploy guard and save
      if( town->deploy_cavalry )
      {
        send_to_char( "Town cavalry will now not be deployed.\n", pl );
        town->deploy_cavalry = FALSE;
      }
      else
      {
        send_to_char( "Town calvary will now be deployed.\n", pl );
        town->deploy_cavalry = TRUE;
      }
      save_towns();
      return TRUE;
    }
    else if( is_abbrev( arg1, "portals" ) )
    {
      if( town->resources < 600000 )
      {
        send_to_char( "This town lacks the resources to deploy portals.\n"
            "You must have 600,000 in resources to deploy them.\n", pl );
        return TRUE;
      }

      // Toggle deploy guard and save
      if( town->deploy_portals )
      {
        send_to_char( "Town portals will now not be deployed.\n", pl );
        town->deploy_portals = FALSE;
      }
      else
      {
        send_to_char( "Town portals will now be deployed.\n", pl );
        town->deploy_portals = TRUE;
      }
      save_towns();
      return TRUE;
    }
  }
  if( cmd == CMD_BUY )
  {
    // People that can't buy anything here.
    if( GET_SURNAME(pl) <= SURNAME_COMMONER )
      return FALSE;

    rest = one_argument( arg, arg1 );
    rest = one_argument( rest, arg2 );
    if( *arg1 == '\0' )
    {
      send_to_char( "You can buy a catapult, ballista, or battering ram.\n", pl );
      if( town->resources >= 150000 )
        send_to_char( "You can buy some town gates.\n", pl );
      return TRUE;
    }
    if( is_abbrev( arg1, "ballista" ) )
    {
      if( 5000 * 1000 > GET_MONEY(pl) )
      {
        send_to_char( "A ballista costs 5000 platinum.\n", pl );
        return TRUE;
      }
      else
      {
        // Load them a ballista and take the cash.
        siege = read_object(real_object(461), REAL);
        if( !siege )
        {
          logit(LOG_DEBUG, "warmaster: couldn't load ballista.");
          return FALSE;
        }
        SUB_MONEY( pl, 5000 * 1000, 0 );
        obj_to_room( siege, pl->in_room );
        add_siege( siege );
        act( "You buy $p.", FALSE, pl, siege, NULL, TO_CHAR );
        act( "$n buys $p.", FALSE, pl, siege, NULL, TO_ROOM );
        return TRUE;
      }
    }
    if( is_abbrev( arg1, "battering" ) 
        || is_abbrev( arg1, "ram" ) )
    {
      if( 5000 * 1000 > GET_MONEY(pl) )
      {
        send_to_char( "A battering ram costs 5000 platinum.\n", pl );
        return TRUE;
      }
      else
      {
        // Load them a battering ram and take the cash.
        siege = read_object(real_object(462), REAL);
        if( !siege )
        {
          logit(LOG_DEBUG, "warmaster: couldn't load battering ram.");
          return FALSE;
        }
        SUB_MONEY( pl, 5000 * 1000, 0 );
        obj_to_room( siege, pl->in_room );
        add_siege( siege );
        act( "You buy $p.", FALSE, pl, siege, NULL, TO_CHAR );
        act( "$n buys $p.", FALSE, pl, siege, NULL, TO_ROOM );
        return TRUE;
      }
    }
    if( is_abbrev( arg1, "catapult" ) )
    {
      if( 5000 * 1000 > GET_MONEY(pl) )
      {
        send_to_char( "A catapult costs 5000 platinum.\n", pl );
        return TRUE;
      }
      else
      {
        // Load them a catapult and take the cash.
        siege = read_object(real_object(463), REAL);
        if( !siege )
        {
          logit(LOG_DEBUG, "warmaster: couldn't load catapult.");
          return FALSE;
        }
        SUB_MONEY( pl, 5000 * 1000, 0 );
        obj_to_room( siege, pl->in_room );
        add_siege( siege );
        act( "You buy $p.", FALSE, pl, siege, NULL, TO_CHAR );
        act( "$n buys $p.", FALSE, pl, siege, NULL, TO_ROOM );
        return TRUE;
      }
    }
    if( is_abbrev( arg1, "town" ) 
        || is_abbrev( arg1, "gates" )
        || atoi(arg1) > 0 )
    {
      // For 'buy <#>' instead of 'buy gates <#>'
      if( atoi(arg1) > 0 )
        snprintf(arg2, MAX_STRING_LENGTH, "%s", arg1 );

      if( town->resources < 150000 )
      {
        send_to_char( "This town lacks the resources to have gates.\n"
            "You must have 150,000 in resources to buy them.\n", pl );
        return TRUE;
      }

      if( 5000 * 1000 > GET_MONEY(pl) )
      {
        send_to_char( "Town gates cost 5000 platinum.\n", pl );
        return TRUE;
      }

      ztop = town->zone->real_top;
      count = 0;
      if( *arg2 == '\0' )
      {
        send_to_char( "You can buy: \n", ch );
        for( i = town->zone->real_bottom; i <= ztop; i++ )
        {
          for( j = 0; j < NUM_EXITS; j++ )
          {
            // Skip non-existant exits and exits to nowhere.
            if( !world[i].dir_option[j] || world[i].dir_option[j]->to_room == NOWHERE )
              continue;
            if( world[world[i].dir_option[j]->to_room].zone != world[i].zone )
            {
              if( has_gates( i ) )
                snprintf(buf, MAX_STRING_LENGTH, "%2d)*%-5s %s\n", ++count, dirs[j],
                    world[world[i].dir_option[j]->to_room].name );
              else
                snprintf(buf, MAX_STRING_LENGTH, "%2d) %-5s %s\n", ++count, dirs[j],
                    world[world[i].dir_option[j]->to_room].name );
              send_to_char( buf, pl );
            }
          }
        }
        if( count == 0 )
          send_to_char( "No exits from town found!\n", pl );
        return TRUE;
      }
      else
      {
        if( (numgate = atoi( arg2 )) < 1 )
        {
          send_to_char( "Invalid gate number (too small).\n", pl );
          return TRUE;
        }
        for( i = town->zone->real_bottom; i <= ztop; i++ )
        {
          for( j = 0; j < NUM_EXITS; j++ )
          {
            // Skip non-existant exits and exits to nowhere.
            if( !world[i].dir_option[j] || world[i].dir_option[j]->to_room == NOWHERE )
              continue;
            if( world[world[i].dir_option[j]->to_room].zone != world[i].zone )
            {
              if( ++count == numgate )
              {
                if( has_gates( i ) )
                {
                  send_to_char( "There are already gates at that exit.\n", pl );
                  return TRUE;
                }
                // Put a gate there.
                siege = read_object(real_object(464), REAL);
                if( !siege )
                {
                  logit(LOG_DEBUG, "warmaster: couldn't load town gates.");
                  return FALSE;
                }
                // Block the correct direction.
                siege->value[0] = rev_dir[j];
                obj_to_room( siege, i );//world[i].dir_option[j]->to_room );
                add_siege( siege );
                act( "You buy $p.", FALSE, pl, siege, NULL, TO_CHAR );
                act( "$n buys $p.", FALSE, pl, siege, NULL, TO_ROOM );
                SUB_MONEY( pl, 5000 * 1000, 0 );
                return TRUE;
              }
            }
          }
        }
        send_to_char( "Invalid gate number (too big).\n", pl );
        return TRUE;
      }
    }
  }

  return FALSE;
}

// Checks to see if it's time to deploy guards/cavalry/portals.
void check_deploy( struct zone_data *zone )
{
  P_town town = towns;
  P_char mob;
  int i, vnum, rnum;
  char buf[MAX_STRING_LENGTH];

  // Search for town data corresponding to zone.
  while( town && town->zone != zone )
    town = town->next_town;
  // If no town data, or not set to deploy.
  if( !town )
    return;

  if( town->deploy_guard && town->guard_vnum )
  {
    rnum = real_mobile( town->guard_vnum );

    snprintf(buf, MAX_STRING_LENGTH, "Mob: %d '%s': Current load: %d, Max load: %d in room: %d.",
        town->guard_vnum, mob_index[rnum].desc2, mob_index[rnum].number,
        mob_index[rnum].limit, town->guard_load_room );
    wizlog( 60, buf );

    // Deploy guards if necessary..
    for( i = mob_index[rnum].number; i < mob_index[rnum].limit; i++ )
    {
      mob = read_mobile(rnum, REAL);
      apply_zone_modifier(mob);
      char_to_room(mob, real_room(town->guard_load_room), -1);
    }
  }
  if( town->deploy_cavalry && town->cavalry_vnum )
  {
    rnum = real_mobile( town->cavalry_vnum );

    snprintf(buf, MAX_STRING_LENGTH, "Mob: %d '%s': Current load: %d, Max load: %d in room: %d.",
        town->cavalry_vnum, mob_index[rnum].desc2, mob_index[rnum].number,
        mob_index[rnum].limit, town->cavalry_load_room );
    wizlog( 60, buf );

    // Deploy cavalry if necessary..
    for( i = mob_index[rnum].number; i < mob_index[rnum].limit; i++ )
    {
      mob = read_mobile(rnum, REAL);
      apply_zone_modifier(mob);
      char_to_room(mob, real_room(town->cavalry_load_room), -1);
    }
  }
  // Need to add portals

}

// Returns TRUE iff room has gates in it.
bool has_gates( int room )
{
  if( room <= 0 )
  {
    wizlog( 60, "has_gates: Bad room number!" );
    logit(LOG_DEBUG, "has_gates: Bad room number.");
    return FALSE;
  }

  P_obj objects = world[room].contents;

  while( objects )
  {
    // If the object proc is castlewall
    if( obj_index[objects->R_num].func.obj == castlewall )
      return TRUE;

    objects = objects->next_content;
  }
  return FALSE;
}

void kill_siege( P_char ch, P_obj obj )
{
  char buf[MAX_STRING_LENGTH];

  if( IS_FIGHTING( ch ) || IS_DESTROYING(ch) )
  {
    send_to_char( "You are already fighting.\n", ch );
    return;
  }
  if( !is_siege( obj ) )
  {
    send_to_char( "You can only attack siege objects.\n", ch );
    return;
  }

  act( "You begin to attack $p.", FALSE, ch, obj, NULL, TO_CHAR );

  set_destroying( ch, obj );

}

void multihit_siege( P_char ch )
{
  P_char other_attacker;
  P_obj  siege, scraps, weapon;
  bool   destroy = FALSE;
  char   buf[MAX_STRING_LENGTH];
  int    number_attacks, num_attacks;
  int    damage;
  int    attacks[256];

  if( !ch )
    return;

  siege = ch->specials.destroying_obj;

  if( !IS_DESTROYING(ch) )
    return;
  if( !siege )
    return;

  appear(ch);

  num_attacks = 0;
  number_attacks = calculate_attacks(ch, attacks);
  while( ++num_attacks <= number_attacks )
  {
    act( "You hit $p.", FALSE, ch, siege, NULL, TO_CHAR | ACT_NOTTERSE );
    act( "$n hits $p.", FALSE, ch, siege, NULL, TO_ROOM | ACT_NOTTERSE );

    weapon = ch->equipment[attacks[num_attacks-1]];
    if( !weapon )
    {
      if( GET_CLASS(ch, CLASS_MONK) )
        damage = MonkDamage(ch) + GET_DAMROLL(ch) / 2;
      else
        damage = 1 + GET_DAMROLL(ch) / 2;
    }
    else
      damage = dice( weapon->value[1], weapon->value[2] ) + GET_DAMROLL(ch) / 2;
    // Siege->value[7] holds the current hps left..
    siege->value[7] -= MAX( 1, damage );
    while( siege->value[7] < 0 )
    {
      // 800 hps / unit condition -> 80000 total hps.
      siege->value[7] += 800;
      siege->condition--;
    }
    // If siege is destroyed..
    if( siege->condition <= 0 )
      break;
  }
  if( num_attacks > number_attacks )
    num_attacks = number_attacks;
  snprintf(buf, MAX_STRING_LENGTH, "You hit $p &+R%d&n times.", num_attacks );
  act( buf, FALSE, ch, siege, NULL, TO_CHAR | ACT_TERSE );
  snprintf(buf, MAX_STRING_LENGTH, "$n hits $p &+R%d&n times.", num_attacks );
  act( buf, FALSE, ch, siege, NULL, TO_ROOM | ACT_TERSE );
  if( siege->condition <= 0 )
    destroy = TRUE;

  snprintf(buf, MAX_STRING_LENGTH, "$q %s", destroy ? "is completely destroyed!" :
      "is damaged from the blow!" );
  act(buf, TRUE, NULL, siege, 0, TO_ROOM);

  if( destroy )
  {
    stop_destroying( ch );
    for( other_attacker = destroying_list; other_attacker; 
        other_attacker = other_attacker->specials.next_destroying )
    {
      if( other_attacker->specials.destroying_obj == siege )
        stop_destroying( other_attacker );
    }

    act("$p collapses into scraps.", TRUE, NULL, siege, 0, TO_ROOM);
    scraps = read_object(9, VIRTUAL);
    if( !scraps )
    {
      remove_siege( siege );
      extract_obj( siege, TRUE ); // Siege arti?
      return;
    }
    snprintf(buf, MAX_STRING_LENGTH, "Scraps from %s&n lie in a pile here.",
        siege->short_description);
    scraps->description = str_dup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "a pile of scraps from %s", siege->short_description);
    scraps->short_description = str_dup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s scraps pile", siege->name);
    scraps->name = str_dup(buf);
    scraps->str_mask = STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_KEYS;
    set_obj_affected(scraps, 400, TAG_OBJ_DECAY, 0);
    obj_to_room(scraps, siege->loc.room);

    remove_siege( siege );
    extract_obj( siege, TRUE ); // Siege arti?
  }
}

void init_towns()
{
  FILE *town_file;
  char  line[ MAX_STRING_LENGTH ];
  char *cp;
  P_town *town;
  bool found;
  int i;

  towns = NULL;
  town_file = fopen( SAVE_DIR "/towns", "r" );

  if(!town_file)
  {
    logit( LOG_DEBUG, "Could not open " SAVE_DIR "/towns" );
    return;
  }

  town = &towns;
  // While there's another town to read...
  while( fgets( line, sizeof line, town_file ) != NULL )
  {
    found = FALSE;
    // clear the carriage return at the end of the line.
    for (cp = line; !isspace(*cp); cp++) ;
    *cp = '\0';

    // Find the town in zone table list.
    for( i = 1; i <= top_of_zone_table; i++ )
    {
      // If found, load the info.
      if( !strcmp(line, zone_table[i].filename) )
      {
        found = TRUE;
        *town = new struct town;
        (*town)->next_town = NULL;

        (*town)->zone = &(zone_table[i]);

        // Offense/defense/resources
        fgets( line, sizeof line, town_file );
        sscanf(line, "%i %i %i\n", &((*town)->offense), &((*town)->defense), &((*town)->resources));
        // Guards
        fgets( line, sizeof line, town_file );
        (*town)->deploy_guard = !strcmp( line, "TRUE\n" ) ? TRUE : FALSE;
        fgets( line, sizeof line, town_file );
        sscanf(line, "%i %i %i\n", &((*town)->guard_vnum), &((*town)->guard_max), &((*town)->guard_load_room));
        // Cavalry
        fgets( line, sizeof line, town_file );
        (*town)->deploy_cavalry = !strcmp( line, "TRUE\n" ) ? TRUE : FALSE;
        fgets( line, sizeof line, town_file );
        sscanf(line, "%i %i %i\n", &((*town)->cavalry_vnum), &((*town)->cavalry_max), &((*town)->cavalry_load_room));
        // Portals
        fgets( line, sizeof line, town_file );
        (*town)->deploy_portals = !strcmp( line, "TRUE\n" ) ? TRUE : FALSE;
        fgets( line, sizeof line, town_file );
        sscanf(line, "%i %i\n", &((*town)->portal_vnum), &((*town)->portal_load_room));

//        logit(LOG_DEBUG, "Town loaded: '%s'", zone_table[i].filename);

        town = &((*town)->next_town);
        break;
      }
    }
    if( !found )
    {
       logit(LOG_DEBUG, "Town not found: '%s'", line );
       for( i = 1; i <= top_of_zone_table; i++ )
         logit(LOG_DEBUG, zone_table[i].filename );
       return;
    }

  }

  fclose(town_file);

  /* initialize warmaster specs */
  mob_index[real_mobile0(401000)].func.mob = warmaster;
  mob_index[real_mobile0(401010)].func.mob = warmaster;
  mob_index[real_mobile0(401020)].func.mob = warmaster;
  mob_index[real_mobile0(401030)].func.mob = warmaster;
  mob_index[real_mobile0(401040)].func.mob = warmaster;
  mob_index[real_mobile0(401050)].func.mob = warmaster;
  mob_index[real_mobile0(401060)].func.mob = warmaster;
  mob_index[real_mobile0(401070)].func.mob = warmaster;
  mob_index[real_mobile0(401080)].func.mob = warmaster;
  mob_index[real_mobile0(401090)].func.mob = warmaster;
}

void save_towns()
{
  FILE *town_file;
  char  line[ MAX_STRING_LENGTH ];
  P_town town;

  town_file = fopen( SAVE_DIR "/towns", "w" );

  if(!town_file)
  {
    logit(LOG_DEBUG, "Could not open " SAVE_DIR "/towns" );
    return;
  }

  // For each town..
  for( town = towns; town != NULL; town = town->next_town )
  {
    // Save the info.
    fprintf(town_file, "%s\n", town->zone->filename);
    fprintf(town_file, "%d %d %d\n", town->offense, town->defense, town->resources);
    fprintf(town_file, "%s\n", town->deploy_guard ? "TRUE" : "FALSE");
    fprintf(town_file, "%d %d %d\n", town->guard_vnum, town->guard_max, town->guard_load_room);
    fprintf(town_file, "%s\n", town->deploy_cavalry ? "TRUE" : "FALSE");
    fprintf(town_file, "%d %d %d\n", town->cavalry_vnum, town->cavalry_max, town->cavalry_load_room);
    fprintf(town_file, "%s\n", town->deploy_portals ? "TRUE" : "FALSE");
    fprintf(town_file, "%d %d\n", town->portal_vnum, town->portal_load_room);

  }
  fclose(town_file);
}

void list_town( P_char ch, P_town town )
{
  char buf[MAX_STRING_LENGTH];

  if( !town )
  {
    send_to_char( "Town doesn't exist!\n", ch );
    return;
  }

  // Show town name: level, off, def.
  snprintf(buf, MAX_STRING_LENGTH, "Town '%s': Resources %d, Offense %d, Defense %d\n"
                "  Guards:  %3s: Max %3d Vnum %6d Room %6d.\n"
                "  Cavalry: %3s: Max %3d Vnum %6d Room %6d.\n"
                "  Portals: %3s:         Vnum %6d Room %6d.\n",
    (town->zone != NULL) ? town->zone->name : "Unknown", town->resources, town->offense, town->defense,
    YESNO(town->deploy_guard), town->guard_max, town->guard_vnum, town->guard_load_room,
    YESNO(town->deploy_cavalry), town->cavalry_max, town->cavalry_vnum, town->cavalry_load_room, 
    YESNO(town->deploy_portals), town->portal_vnum, town->portal_load_room );
  send_to_char( buf, ch );
}

// Lists all towns with their level/defense/offense.
void list_towns( P_char ch )
{
  P_town town;

  if( !towns )
  {
    send_to_char( "Could not find list of towns!!\n", ch );
    return;
  }

  // For each town
  for( town = towns; town; town = town->next_town )
  {
    list_town( ch, town);
  }
}

void add_troopresources(P_char ch, P_town town, int amount )
{
  char buf[MAX_STRING_LENGTH];

/* For debugging...
  snprintf(buf, MAX_STRING_LENGTH, "add_trooplevel: '%s' amount: '%d'.\n", town->zone->name, amount );
  send_to_char( buf, ch );
*/

  town->resources += amount;
  if( town->resources < 0 )
    town->resources = 0;
  list_town( ch, town );
  // Save the town info.
  save_towns();
}

void add_troopdefense(P_char ch, P_town town, int amount)
{
  town->defense += amount;
  if( town->defense < 0 )
    town->defense = 0;
  list_town( ch, town );
  // Save the town info.
  save_towns();
}

void add_troopoffense(P_char ch, P_town town, int amount)
{
  town->offense += amount;
  if( town->offense < 0 )
    town->offense = 0;
  list_town( ch, town );
  // Save the town info.
  save_towns();
}

P_town add_findtown( char *arg )
{
  P_town town = towns;
  char town_name[MAX_STRING_LENGTH];
  stripansi_2(town->zone->name, town_name);

  while( town )
  {
    if( is_abbrev(arg, town_name) )
      return town;

    town = town->next_town;
  }

  return NULL;
}

void do_add(P_char ch, char *arg, int cmd)
{
  char   arg1[MAX_STRING_LENGTH];
  char   arg2[MAX_STRING_LENGTH];
  char  *rest;
  int    amount;
  P_town town;

  // Parse argument.
  rest = one_argument(arg, arg1);
  if(!*arg1 )
  {
    send_to_char("This command is to add to troops in a town.\n", ch);
    send_to_char("Syntax: add [resources|defense|offense] <town> <amount>.\n", ch);
    send_to_char("     or add list.\n", ch);
    return;
  }

  // Add attribute to town.
  if( is_abbrev(arg1, "list") )
  {
    list_towns( ch );
    send_to_char( "\n", ch );
    list_siege( ch );
  }
  else if( is_abbrev(arg1, "resources") )
  {
    argument_interpreter(rest, arg1, arg2);
    amount = atoi( arg2 );
    town = add_findtown( arg1 );
    if( amount == 0)
    {
      send_to_char("Syntax: add resources <town> <amount>.\n", ch);
      return;
    }
    if( !town )
    {
      send_to_char( "Couldn't find town '", ch );
      send_to_char( arg1, ch );
      send_to_char(  "'.\n", ch );
      return;
    }
    add_troopresources( ch, town, amount );
  }
  else if( is_abbrev(arg1, "defense") )
  {
    argument_interpreter(rest, arg1, arg2);
    amount = atoi( arg2 );
    town = add_findtown( arg1 );
    if( amount == 0)
    {
      send_to_char("Syntax: add defense <town> <amount>.\n", ch);
      return;
    }
    if( !town )
    {
      send_to_char( "Couldn't find town '", ch );
      send_to_char( arg1, ch );
      send_to_char(  "'.\n", ch );
      return;
    }
    add_troopdefense( ch, town, amount );
  }
  else if( is_abbrev(arg1, "offense") )
  {
    argument_interpreter(rest, arg1, arg2);
    amount = atoi( arg2 );
    town = add_findtown( arg1 );
    if( amount == 0)
    {
      send_to_char("Syntax: add offense <town> <amount>.\n", ch);
      return;
    }
    if( !town )
    {
      send_to_char( "Couldn't find town '", ch );
      send_to_char( arg1, ch );
      send_to_char(  "'.\n", ch );
      return;
    }
    add_troopoffense( ch, town, amount );
  }
  else
  {
    send_to_char("Syntax: add [resources|defense|offense] <town> <amount>.\n", ch);
    send_to_char("     or add list.\n", ch);
    return;
  }
}

// Adds a new siege object to the list (for saves)
void add_siege( P_obj siege )
{
  P_siege newsiege = new struct siege;

  newsiege->obj = siege;
  newsiege->next_siege = siege_objects;  
  siege_objects = newsiege;

  save_siege_list();
}

// Removes a siege object from the list that's been destroyed.
void remove_siege( P_obj siege )
{
  P_siege sieges = siege_objects;
  P_siege siege2;

  if( sieges->obj == siege )
  {
    siege_objects = siege_objects->next_siege;
    sieges->next_siege = NULL;
    free( sieges );
    return;
  }
  while( sieges->next_siege )
  {
    if( sieges->next_siege->obj == siege )
    {
      siege2 = sieges->next_siege;
      sieges->next_siege = siege2->next_siege;
      siege2->next_siege = NULL;
      siege2->obj = NULL;
      free( siege2 );
      return;
    }
    sieges = sieges->next_siege;
  }
  logit(LOG_DEBUG, "remove_siege: siege not in list!" );
}

// Saves the siege objects.
void save_siege_list( )
{
  FILE   *siege_file;
  char    line[ MAX_STRING_LENGTH ];
  char    buff[ SAV_MAXSIZE ];
  char   *buf;
  P_siege siege;
  int     length;

  siege_file = fopen( SAVE_DIR "/siege", "w" );

  if(!siege_file)
  {
    logit(LOG_DEBUG, "Could not open " SAVE_DIR "/siege" );
    return;
  }

  // For each siege object..
  for( siege = siege_objects; siege != NULL; siege = siege->next_siege )
  {
    // Write the room number.
    fprintf(siege_file, "#%d\n", siege->obj->loc.room);
    // Write the object to file
    buf = buff;
    length = write_one_object( siege->obj, buf );
    fwrite( buff, length, 1, siege_file );
    fprintf( siege_file, "\n" );
  }

  fclose( siege_file );
}

// Lists all siege objects with room
void list_siege( P_char ch )
{
  P_siege siege;
  char    buf[MAX_STRING_LENGTH];

  if( !siege_objects )
  {
    send_to_char( "There are no siege objects!!\n", ch );
    return;
  }

  // For each siege object
  for( siege = siege_objects; siege; siege = siege->next_siege )
  {
    snprintf(buf, MAX_STRING_LENGTH, "%d) %s\n", siege->obj->loc.room, siege->obj->short_description );
    send_to_char( buf, ch );
  }
}

// Loads the siege objects.
void init_siege()
{

  FILE   *siege_file;
  char    line[ MAX_STRING_LENGTH ];
  char   *cp;
  P_siege siege;
  P_obj   obj;
  int     room;

  siege_objects = NULL;
  siege_file = fopen( SAVE_DIR "/siege", "r" );

  if(!siege_file)
  {
    logit( LOG_DEBUG, "Could not open " SAVE_DIR "/siege" );
    return;
  }

  while( fgets( line, sizeof line, siege_file ) != NULL )
  {
    if( line[0] != '#' )
    {
      logit( LOG_DEBUG, line );
      logit( LOG_DEBUG, "Siege file corrupted.." );
      fclose( siege_file );
      return;
    }
    cp = line+1;
    room = atoi( cp );
    if( !room )
    {
      logit( LOG_DEBUG, cp );
      logit( LOG_DEBUG, "Siege file room corrupted.." );
      fclose( siege_file );
    }
    fgets( line, sizeof line, siege_file );
    obj = read_one_object(line);
    obj_to_room( obj, room );
    siege = new struct siege;
    siege->obj = obj;
    siege->next_siege = siege_objects;
    siege_objects = siege;
  }

  // assign specs
  obj_index[real_object0(461)].func.obj = ballista;
  obj_index[real_object0(462)].func.obj = battering_ram;
  obj_index[real_object0(463)].func.obj = catapult;
  obj_index[real_object0(464)].func.obj = castlewall;
}

