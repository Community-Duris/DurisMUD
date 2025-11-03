/*
 *  guildhall_procs.c
 *  Duris
 *
 *  Created by Torgal on 2/3/10.
 *
 */

#include "guildhall.h"
#include "utility.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interp.h"
#include "assocs.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "alliances.h"
#include "storage_lockers.h"
#include "ships.h"

extern P_room world;
extern P_desc descriptor_list;

// Added by Gellz 02052015
// Proc for GH golems to ONLY allow guild members to flee in their room
int golem_noflee(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)    //events have priority (dont they alwys
     return FALSE;                //IF its an event, just go back
  if (cmd != CMD_FLEE)
     return FALSE;
  if (cmd == CMD_FLEE)
        {
        GuildhallRoom *room = Guildhall::find_room_by_vnum(world[ch->in_room].number);
          if (!room)
          { }                           //NON GH Room, why golem here?
          else
          {                             //IS GH room - player FLED.
            Guildhall *gh = room->guildhall;
            if (GET_ASSOC(pl) != gh->guild)  // Is user in Guild?
             {act("$N&+y takes advantage of the &+Rpanicked&+y look on your face to &+Wquickly&+y move and block the exit. You are unable to &+Rflee!&n\n", FALSE, pl, 0, ch, TO_CHAR);
            act("$N&+y senses the &+Rpanic&+y on $n's face and &+Wquickly&+y intercepts their attempt at escape. The &+Yfight&+y is still on!&n\n", FALSE, pl, 0, ch, TO_ROOM);
          return TRUE;
             } else                     // User in Guild - ALLOW Flee
             {act("$N&+y nods, winks and moves to the side, to allow you to escape the fray!.\n", FALSE, pl, 0, ch, TO_CHAR);
             act("$N&+y merely smirks and doesnt try to stop them as $n attempts to &+rflee&+y from all the &+rh&+Ror&+rr&+Ro&+rrs&+y in this room!&n\n", FALSE, pl, 0, ch, TO_ROOM);
              return FALSE;}
           }                            //END ELSE in GH and user FLED
        }                               //END CMD = FLEE
  return FALSE;
}                                       // END GELLZ GH Golem noflee Proc.


int guildhall_door(P_obj obj, P_char ch, int cmd, char *arg)
{
  if ( !obj )
    return FALSE;
  
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
    
  //
  // if a player does "look in <objectname>", we want to show them inside the entrance room
  //
  if( ch && cmd == CMD_LOOK &&
      isname(arg, "in guildhall"))
  {
    send_to_char("You peer inside the guildhall...\r\n", ch);
    new_look(ch, "inside", CMD_LOOK, real_room0(obj->value[0]));    
    return TRUE;
  }    
  return FALSE;
}

int guildhall_golem(P_char ch, P_char pl, int cmd, char *arg)
{
  char buff[MAX_STRING_LENGTH];

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  // make sure golem stays in its home room
  if( ch->player.birthplace && ch->in_room != real_room(ch->player.birthplace) )
  {
    act("$n pops out of existence.&n", FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(ch->player.birthplace), -1);
    act("$n pops into existence.&n", FALSE, ch, 0, 0, TO_ROOM);
  }

  // If there's someone fleeing and it's not the golem.
  if( cmd == CMD_FLEE && pl && pl != ch )
  {
    GuildhallRoom *room = Guildhall::find_room_by_vnum(world[ch->in_room].number);

    // We allow people to scramble to their feet etc.
    if( !MIN_POS(pl, POS_STANDING + STAT_RESTING) )
    {
      return FALSE;
    }
    // NON GH Room, why golem here?
    if( !room )
    {
      debug( "We have a real problem: Golem isn't in a guildhall room and is at its birthplace!" );
    }
    else
    // IS GH room - player FLED.
    {
      Guildhall *gh = room->guildhall;
      if( !gh )
      {
        debug( "We have a real problem: Guildhall room vnum %d doesn't have a guildhall!", room->vnum );
      }
      // Is person fleeing in Guild?
      else if( GET_ASSOC(pl) != gh->guild )
      {
        act("$N&+y takes advantage of the &+Rpanicked&+y look on your face to &+Wquickly&+y move and block the exit. You are unable to &+Rflee!&n\n", FALSE, pl, 0, ch, TO_CHAR);
        act("$N&+y senses the &+Rpanic&+y on $n's face and &+Wquickly&+y intercepts their attempt at escape. The &+Yfight&+y is still on!&n\n", FALSE, pl, 0, ch, TO_ROOM);
        return TRUE;
      }
      // User in Guild - ALLOW Flee
      else
      {
        act("$N&+y nods, winks and moves to the side, to allow you to escape the fray!.\n", FALSE, pl, 0, ch, TO_CHAR);
        act("$N&+y merely smirks and doesnt try to stop them as $n attempts to &+rflee&+y from all the &+rh&+Ror&+rr&+Ro&+rrs&+y in this room!&n\n", FALSE, pl, 0, ch, TO_ROOM);
        return FALSE;
      }
    }
  }

  // why can't gh golems have some AI?
  if (ch->group && ch->group->ch && !IS_PC(ch->group->ch))
    group_remove_member(ch);
  
  //
  // standard checks
  //
  if (!GET_ASSOC(ch))
  {
    logit(LOG_GUILDHALLS, "guildhall_golem() assigned to %s in %d has no association number!", ch->player.short_descr, world[ch->in_room].number);
    REMOVE_BIT(ch->specials.act, ACT_SPEC);
    return FALSE;
  }
  else
  {
    SET_MEMBER(GET_A_BITS(ch));
    SET_NORMAL(GET_A_BITS(ch));
  }

  int blocked_dir = direction_tag(ch);

  if( blocked_dir < DIR_NORTH || blocked_dir >= NUM_EXITS )
  {
    logit(LOG_GUILDHALLS, "guildhall_golem() assigned to %s in %d has an invalid blocking direction (%d)!", GET_NAME(ch), world[ch->in_room].number, blocked_dir);
    REMOVE_BIT(ch->specials.act, ACT_SPEC);
    return FALSE;
  }

  //
  // cmds
  //

  if( cmd == CMD_PERIODIC )
  {
    // TODO: add buffs based on online guild member count, etc
    return FALSE;
  }

  if( cmd == CMD_DEATH )
  {
    // death proc: remove golem from guildhall
    if( Guildhall *gh = Guildhall::find_from_ch(ch) )
    {
      gh->golem_died(ch);
    }

    return TRUE;
  }

  if( pl && cmd == cmd_from_dir(blocked_dir) )
  {
    // char tried to go in the blocked direction

    P_char   t_ch = pl;

    if (IS_PC_PET(pl))
      t_ch = pl->following;

    if(!t_ch)
      return FALSE;

    bool allowed = IS_ASSOC_MEMBER(t_ch, GET_ASSOC(ch));

    P_Alliance alliance = GET_ASSOC(ch)->get_alliance();

    if( alliance )
    {
      allowed = allowed || IS_ASSOC_MEMBER(t_ch, alliance->get_forgers()) || IS_ASSOC_MEMBER(t_ch, alliance->get_joiners());
    }
    if(!allowed && pl->group)
    {
      struct group_list *gl;
      gl = pl->group;
      while (gl)
      {
	      if (GET_ASSOC(gl->ch) == GET_ASSOC(ch))
	        allowed = TRUE;
	      gl = gl->next;
      }
    }

    if( IS_TRUSTED(pl) )
    {
      // don't show anything when immortals enter the GH
      return FALSE;
    }
    else if( allowed )
    {
      act("$N stands impassively as you pass by.", FALSE, pl, 0, ch, TO_CHAR);
      act("$N stands impassively as $n passes by.", FALSE, pl, 0, ch, TO_NOTVICT);
      return FALSE;
    }
    else
    {
      if(IS_WARRIOR(ch))
      {
        act("$N glares at you and knocks you to the ground.", FALSE, pl, 0, ch, TO_CHAR);
        act("$N glares at $n and knocks $m to the ground.", FALSE, pl, 0, ch, TO_NOTVICT);
        SET_POS(pl, GET_STAT(ch) + POS_SITTING);
        CharWait(pl, WAIT_SEC * 2); 
        return TRUE;
      }
      else
      {
        act("$N glares at you and says, 'halt!'", FALSE, pl, 0, ch, TO_CHAR);
        act("$N glares at $n and says, 'halt!'", FALSE, pl, 0, ch, TO_NOTVICT);
        return TRUE;
      }
    }

    return TRUE;
  }

  if( pl && (cmd == CMD_GOTHIT && !number(0, 15))
    || (cmd == CMD_HIT || cmd == CMD_KILL))
  {
    //can add check here to see if guild has magic mouth upgrade from db?
    snprintf(buff, MAX_STRING_LENGTH, "&+cA magic mouth tells your guild 'Alert! $N&n&+c has trespassed into %s&n&+c!'&n", world[ch->in_room].name);
    for (P_desc i = descriptor_list; i; i = i->next)
    {
      if( !i->connected && !is_silent(i->character, TRUE)
        && IS_SET(i->character->specials.act, PLR_GCC)
        && IS_MEMBER(GET_A_BITS(i->character))
        && (GET_ASSOC(i->character) == GET_ASSOC(ch))
        && !IS_TRUSTED(i->character) )
      {
        act(buff, FALSE, i->character, 0, pl, TO_CHAR);
      }
      return FALSE;
    }
  }
  return FALSE;
}

int guildhall_window_room(int room, P_char ch, int cmd, char *arg)
{
  return FALSE;
}

int guildhall_window(P_obj obj, P_char ch, int cmd, char *arg)
{
  if ( !obj )
    return FALSE;
  
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  
  //
  // if a player does "look in <objectname>", we want to show them outside the guildhall
  //
  if( ch && cmd == CMD_LOOK &&
	  isname(arg, "in glass"))
  {
    /*
	char buff[MAX_STRING_LENGTH];
    half_chop(arg, buff, arg);

    if( *buff && is_abbrev(buff, "in") )
    {
      arg = one_argument(arg, buff);

      if( isname(buff, obj->name) )
      {
        if( !real_room0(obj->value[0]) )
          return FALSE;
	*/
        //snprintf(buff, MAX_STRING_LENGTH, "You peer into %s&n and see...\r\n\r\n", obj->short_description);
		send_to_char("You look out the window...\r\n", ch);
        //send_to_char(buff, ch);
        new_look(ch, 0, CMD_LOOKOUT, real_room0(obj->value[0]));    
        return TRUE;
    /*
	else if( *buff && is_abbrev(buff, "out") )
    {
      if( !real_room0(obj->value[0]) )
        return FALSE;
      
      snprintf(buff, MAX_STRING_LENGTH, "You peer into %s&n and see...\r\n\r\n", obj->short_description);
      send_to_char(buff, ch);
      new_look(ch, 0, CMD_LOOKOUT, real_room0(obj->value[0]));    
      return TRUE;
    }
	*/
  }
  return FALSE;
}

int guildhall_heartstone(P_obj obj, P_char ch, int cmd, char *arg)
{
  if ( !obj )
    return FALSE;
  
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if( ch && cmd == CMD_TOUCH && isname(arg, obj->name) )
  {
    send_to_char("Nothing happens, but you get the distinct impression that the heartstone is just biding its time.\r\n", ch);
    return TRUE;
  }
  
  return FALSE;
}

int guildhall_bank_room(int room, P_char ch, int cmd, char *arg)
{
  if( !ch )
    return FALSE;
  
  if( cmd == CMD_ENTER )
  {
    Guildhall *gh = Guildhall::find_by_vnum(world[ch->in_room].number);
    
    if( !gh || gh->guild != GET_ASSOC(ch) )
    {
      send_to_char("This isn't your guildhall!\r\n", ch);
      return TRUE;
    }
  }
  
  return guild_locker_room_hook(room, ch, cmd, arg);
}

int guildhall_cargo_board(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  
  if (!obj||!ch)
    return (FALSE);
  
  if (arg && (cmd == CMD_LOOK))
  {
    char buff[MAX_STRING_LENGTH];
    one_argument(arg, buff);

    if (isname(buff, obj->name))
    {
      snprintf(buff, MAX_STRING_LENGTH, "You look at %s&n...\r\n", obj->short_description);
      send_to_char(buff, ch);
      show_cargo_prices(ch);
      return TRUE;
    }
  }
  
  return FALSE;
}
