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

  // why can't gh golems have some AI?
  if (ch->group && ch->group->ch && !IS_PC(ch->group->ch))
    group_remove_member(ch);
  
  //
  // standard checks
  //
  if (!GET_A_NUM(ch))
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
  
  if( blocked_dir < NORTH || blocked_dir >= NUM_EXITS )
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

    bool allowed = IS_ASSOC_MEMBER(t_ch, GET_A_NUM(ch));

    struct alliance_data *alliance = get_alliance(GET_A_NUM(ch));

    if( alliance )
    {
      allowed = allowed || IS_ASSOC_MEMBER(t_ch, alliance->forging_assoc_id) || IS_ASSOC_MEMBER(t_ch, alliance->joining_assoc_id);
    }
    if(!allowed && pl->group)
    {
      struct group_list *gl;
      gl = pl->group;
      while (gl)
      {
	if (GET_A_NUM(gl->ch) == GET_A_NUM(ch))
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
    }

    return TRUE;
  }
  
  if ( pl && (cmd == CMD_GOTHIT && !number(0, 15)) ||
      (cmd == CMD_HIT || cmd == CMD_KILL))
  {
    //can add check here to see if guild has magic mouth upgrade from db?
    sprintf(buff,
	"&+cA magic mouth tells your guild 'Alert! $N&n&+c has trespassed into %s&n&+c!'&n",
	world[ch->in_room].name);
    for (P_desc i = descriptor_list; i; i = i->next)
      if (!i->connected &&
	  !is_silent(i->character, TRUE) &&
	  IS_SET(i->character->specials.act, PLR_GCC) &&
	  IS_MEMBER(GET_A_BITS(i->character)) &&
	  (GET_A_NUM(i->character) == GET_A_NUM(ch)) &&
	  !IS_TRUSTED(i->character))
	act(buff, FALSE, i->character, 0, pl, TO_CHAR);
    return FALSE;
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
        //sprintf(buff, "You peer into %s&n and see...\r\n\r\n", obj->short_description);
		send_to_char("You look out the window...\r\n", ch);
        //send_to_char(buff, ch);
        new_look(ch, 0, -5, real_room0(obj->value[0]));    
        return TRUE;
    /*
	else if( *buff && is_abbrev(buff, "out") )
    {
      if( !real_room0(obj->value[0]) )
        return FALSE;
      
      sprintf(buff, "You peer into %s&n and see...\r\n\r\n", obj->short_description);
      send_to_char(buff, ch);
      new_look(ch, 0, -5, real_room0(obj->value[0]));    
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
    
    if( !gh || gh->assoc_id != GET_A_NUM(ch) )
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
      sprintf(buff, "You look at %s&n...\r\n", obj->short_description);
      send_to_char(buff, ch);
      show_cargo_prices(ch);
      return TRUE;
    }
  }
  
  return FALSE;
}
