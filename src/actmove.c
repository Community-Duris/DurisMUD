/*
 * ***************************************************************************
 * File: actmove.c                                          Part of Duris *
 * Usage: Movement commands, close/open & lock/unlock doors.
 * Copyright  1990, 1991 - see 'license.doc' for complete information.
 * Copyright 1994 - 2008 - Duris Systems Ltd.
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "map.h"
#include "graph.h"
#include "disguise.h"
#include "weather.h"
#include "siege.h"
#include "tradeskill.h"
#include "vnum.obj.h"

/*
 * external variables
 */

extern P_desc descriptor_list;
extern P_index obj_index;
extern P_index mob_index;
extern P_room world;
extern struct time_info_data time_info;
extern const char *command[];
extern const char *dirs[];
extern const char *short_dirs[];
extern const char *dirs2[];
extern const struct stat_data stat_factor[];
extern const int innate_abilities[];
extern const int class_innates[][5];
extern const int movement_loss[];
extern const int rev_dir[];
extern struct dex_app_type dex_app[];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern void check_room_links(P_char, int, int);
extern bool grease_check(P_char);
extern int get_number_allies_in_room(P_char ch, int room_index);
extern int get_weight_allies_in_room(P_char ch, int room_index);
void send_movement_noise(P_char ch, int num);

int is_ice(P_char ch, int room)
{
  P_obj    obj, next_obj;

  if( world[room].contents)
    for (obj = world[room].contents; obj; obj = next_obj)
    {
      next_obj = obj->next_content;

      if( obj->R_num == real_object(110))
        return TRUE;
    }

  if( affected_by_spell(ch, SPELL_PATH_OF_FROST))
    return TRUE;

  return FALSE;
}

int load_modifier(P_char ch)
{
  int p;

  if( IS_TRUSTED(ch) )
    return 75;
  if( CAN_CARRY_W(ch) <= 0 )
    return 300;

  p = 100 - MAX(0, ((CAN_CARRY_W(ch) - IS_CARRYING_W(ch)) * 100) / CAN_CARRY_W(ch));

  if( p < 10)
    return 75;
  if( p < 20)
    return 85;
  if( p < 30)
    return 95;
  if( p < 40)
    return 105;
  if( p < 55)
    return 125;
  if( p < 65)
    return 145;
  if( p < 75)
    return 165;
  if( p < 85)
    return 185;
  if( p < 95)
    return 200;
  return 300;
}

/*
 * new routine to swap 2 characters in a list of characters, it just swaps
 * pointers around, but have to make sure that the list stays sane. JAB
 */

void SwapCharsInList(P_char ch1, P_char ch2)
{
  P_char   tmp, t_ch1 = NULL, t_ch2 = NULL, p1 = NULL, p2 = NULL;

  if( !ch1 || !ch2)
    return;
  if( (ch1->in_room != ch2->in_room) || (ch1->in_room == NOWHERE))
    return;

  /*
   * find preceding elements for ch1 and ch2, since this is only a
   * singly- linked list, we must do this
   */

  LOOP_THRU_PEOPLE(tmp, ch1)
  {
    if( (tmp->next_in_room == ch1) || (tmp->next_in_room == ch2))
    {
      if( !p1)
        p1 = tmp;
      else
        p2 = tmp;
    }
    if( (tmp == ch1) || (tmp == ch2))
    {
      if( !t_ch1)
        t_ch1 = tmp;
      else
        t_ch2 = tmp;
    }
  }

  if( !t_ch1 || !t_ch2 || !p1 ||
      (!p2 && (t_ch1 != world[ch1->in_room].people)))
  {
    logit(LOG_EXIT, "char_in_room list corrupt - SwapCharsInList");
    raise(SIGSEGV);
  }
  /*
   * cheating a little to simplify things: t_ch1 now holds 'first' of
   * ch1/ch2 in list. t_ch2 now holds 'second' of ch1/ch2 in list. p1 is
   * the predecessor to t_ch1 or to t_ch2 if t_ch1 is head of list. p2
   * is predecessor to t_ch2 or NULL if t_ch1 is head of list.
   */

  if( !p2)
  {
    /*
     * t_ch1 is old head of list
     */
    world[ch1->in_room].people = t_ch2;
  }
  else
  {
    /*
     * swap leading p1
     */
    p1->next_in_room = t_ch2;
  }

  /*
   * ok, at this point we have swapped the 'leading' pointers (and the
   * head of list if appropriate), now we swap ch1's and ch2's
   * ->next_in_room pointers (caveats again).
   */

  tmp = t_ch1->next_in_room;
  t_ch1->next_in_room = t_ch2->next_in_room;
  if( tmp != t_ch2)
    t_ch2->next_in_room = tmp;
  else
    t_ch2->next_in_room = t_ch1;

  /*
   * swap trailing pointer, only if it's not that same as t_ch1
   */

  if( !p2)
  {
    if( p1 != t_ch1)
      p1->next_in_room = t_ch1;
  }
  else
  {
    if( p2 != t_ch1)
      p2->next_in_room = t_ch1;
  }
}

/*
 * This is a routine to check for problems with particular exits which
 */
/*
 * don't apply room-wide    -Sman
 */
/*
 * Return true if no direction-related problems, false if there are
 */

int leave_by_exit(P_char ch, int exitnumb)
{
  P_char   k = NULL, block1 = NULL, block2 = NULL, t_ch = NULL;
  char     j, exit1 = -1, exit2 = -1, exit3 = -1;
  int      room_to;
  P_char   target_head = NULL;
  int      num_in_room = 0, room_limit = 0;
  P_char   rider = NULL;

  /*
   * to avoid problems and conflicts, NEVER return TRUE from any check
   * in this function.  Check for problems and return FALSE if you find
   * one, else just drop through to final return TRUE;
   */

  if( !SanityCheck(ch, "leave_by_exit") ||
      (exitnumb < 0) || (exitnumb > (NUM_EXITS - 1)))
    return FALSE;

  if( world[ch->in_room].dir_option[exitnumb] == NULL )
    return FALSE;

  room_to = world[ch->in_room].dir_option[exitnumb]->to_room;

  /*
   * This is for the wind direction, so the wind can be too strong to
   * walk against it...
   */
#if 0
  if( IS_PC(ch) && !CHAR_IN_TOWN(ch) &&
      OUTSIDE(ch) && (in_weather_sector(ch->in_room) >= 0))
  {
    cond = &sector_table[in_weather_sector(ch->in_room)].conditions;
    if( (cond->wind_dir == exitnumb) && (cond->windspeed > 50))
    {
      if( IS_TRUSTED(ch))
        send_to_char("The wind is incredibly strong, but you don't care...\n",
                     ch);
      else if( !IS_AFFECTED(ch, AFF_WRAITHFORM))
        send_to_char("The wind is incredibly strong!\n"
                     "It requires a great effort to move through it.\n", ch);

    }
  }
#endif
  /*
   * ok, the wonder of NO_GROUND rooms, moving FROM a NO_GROUND room
   * requires fly, levitate, (or falling). Moving into a NO_GROUND room
   * depends on direction, moving up requires fly or levitate, moving
   * laterally requires a death wish, moving down, basically means you
   * jump.  If you are mounted only the mount needs fly or levitate
   * (pegasus for example).  JAB
   */

  StartRegen(ch, EVENT_MOVE_REGEN);
  StartRegen(ch, EVENT_HIT_REGEN);

  if( IS_PC(ch) && IS_RIDING(ch))
  {
    t_ch = get_linked_char(ch, LNK_RIDING);
    if(IS_AFFECTED2(t_ch, AFF2_MINOR_PARALYSIS) || 
        IS_AFFECTED2(t_ch, AFF2_MAJOR_PARALYSIS) || 
        IS_AFFECTED(t_ch, AFF_KNOCKED_OUT) || 
        GET_STAT(t_ch) == STAT_SLEEPING || 
        GET_STAT(t_ch) == STAT_DYING ||  
        GET_STAT(t_ch) == STAT_INCAP)
    {
      send_to_char("Alas, your mount is not quite in the shape for that.\n", ch);
      return FALSE;
    }
/*
    if( IS_FIGHTING(t_ch))
    {
    	send_to_char("Your mount is too busy fighting for its life!\n", ch);
    	return FALSE;
    }
*/
    if( !MIN_POS(t_ch, POS_STANDING + STAT_NORMAL))
    {
    	send_to_char("Your mount is busy regaining its footing!\n", ch);
    	return FALSE;
    }
    
  }
  else
  {
    t_ch = ch;
  }

  /*
   * AIR_PLANE rooms are NO_GROUND without gravity, so you need fly or
   * levitate to move, but you won't fall.
   */

  if( !IS_TRUSTED(t_ch) &&
      (world[t_ch->in_room].sector_type == SECT_AIR_PLANE))
  {
    if( !(IS_AFFECTED(t_ch, AFF_FLY) ||
          (((exitnumb == UP) || (exitnumb == DOWN)) &&
           IS_AFFECTED(t_ch, AFF_LEVITATE))))
    {
      if( ch == t_ch)
      {
        send_to_char("Try flapping your arms!  (Couldn't hurt)\n", ch);
        return FALSE;
      }
      else
      {
        send_to_char("Your mount would need wings to go there!\n", ch);
        return FALSE;
      }
    }
  }
  /*
   * ok, that handled moving FROM an AIR_PLANE room, now for moving INTO
   * one
   */

  if( (world[room_to].sector_type == SECT_AIR_PLANE) && !IS_TRUSTED(t_ch))
  {
    if( !IS_AFFECTED(t_ch, AFF_FLY) && !IS_AFFECTED(t_ch, AFF_LEVITATE))
    {
      if( ch == t_ch)
      {
        send_to_char("Try flapping your arms!  (Couldn't hurt)\n", ch);
        return FALSE;
      }
      else
      {
        send_to_char("Your mount would need wings to go there!\n", ch);
        return FALSE;
      }
    }
  }
  if( !IS_TRUSTED(t_ch) &&
      (world[t_ch->in_room].sector_type == SECT_NO_GROUND))
  {
    if( !(IS_AFFECTED(t_ch, AFF_FLY) ||
          (((exitnumb == UP) || (exitnumb == DOWN)) &&
           IS_AFFECTED(t_ch, AFF_LEVITATE))))
    {
      if( ch == t_ch)
      {
        send_to_char("Try flapping your arms!  (Couldn't hurt)\n", ch);
        return FALSE;
      }
      else
      {
        send_to_char("Your mount would need wings to go there!\n", ch);
        return FALSE;
      }
    }
  }
  /*
   * ok, that handled moving FROM a NO_GROUND room, now for moving INTO
   * one
   */

  if( (world[room_to].sector_type == SECT_NO_GROUND) && !IS_TRUSTED(t_ch))
  {
    if( exitnumb == UP)
    {
      if( !IS_AFFECTED(t_ch, AFF_FLY) && !IS_AFFECTED(t_ch, AFF_LEVITATE))
      {
        if( ch == t_ch)
        {
          send_to_char("Oops. Forget something? Like wings?\n", ch);
          return TRUE;
        }
        else
        {
          send_to_char("Your mount would need wings to go there!\n", ch);
          return FALSE;
        }
      }
    }
    else
    {
      /*
       * trying to move into a NO_GROUND from any direction but up
       */

      if( !IS_TRUSTED(t_ch) && !IS_AFFECTED(t_ch, AFF_FLY) &&
          !IS_AFFECTED(t_ch, AFF_LEVITATE) && ((world[room_to].light > 0) ||
                                               ((world[room_to].light == 0) &&
                                                IS_AFFECTED2(ch,
                                                             AFF2_ULTRAVISION)))
          && !IS_SET(world[room_to].dir_option[DOWN]->exit_info, EX_SECRET))
      {

        /*
         * ok, can see it coming
         */

        if( t_ch != ch)
        {
          act("$N balks, and refuses to move in that direction.", FALSE, ch,
              0, t_ch, TO_CHAR);
          act("$N shakes $S head, and refuses to move.", TRUE, ch, 0, t_ch,
              TO_NOTVICT);
          return FALSE;
        }
        else
        {
          send_to_char("Oops. Forget something? Like wings?\n", ch);
          return TRUE;
        }
      }
    }
  }
  /*
   * to keep them from riding into buildings, while in caves, etc
   */
#if 0
  if( IS_RIDING(ch) && !IS_TRUSTED(ch) &&
      (world[room_to].room_flags & INDOORS))
  {
    send_to_char("While mounted? I don't think so...\n", ch);
    return (0);
  }
#endif
  t_ch = NULL;                  /*
                                 * reinit it to prevent later problems.
                                 */

  /*
   * single file rooms, it skips this check for immorts, and for the
   * most degenerate case (ch is alone in room)
   */

  if( IS_RIDING(ch) && (world[room_to].room_flags & SINGLE_FILE))
  {
    send_to_char("You can't fit into this narrow passage while mounted...\n", ch);
    return FALSE;
  }

  if( (( rider = get_linking_char(ch, LNK_RIDING) ) != NULL)
    && (world[room_to].room_flags & SINGLE_FILE) )
  {
    send_to_char("You are almost knocked off your mount trying to head into such a cramped space.\n", rider );
    send_to_char("You almost knock off your rider trying to head into such a cramped space.\n", ch );
    return FALSE;
  }

  if( (world[ch->in_room].room_flags & SINGLE_FILE) && !IS_TRUSTED(ch) &&
      !IS_AFFECTED(ch, AFF_WRAITHFORM) && ((world[ch->in_room].people != ch)
                                           || (ch->next_in_room)))
  {

    /*
     * SINGLE_FILE rooms are limited to 2 directions (ONLY, no more,
     * no less) Rather than keeping 2 lists of the people in the room,
     * it adds new arrivals/removes those leaving, either from
     * top_of_list or end_of_list based on directions.  If there is a
     * char (PC/NPC) between ch and the direction they want to move,
     * they are 'blocked', only the char at the head of the list, and
     * the one at the end, are free to move (and then only in 1
     * direction, unless they are alone in the room).  Immortals are
     * 'transparent' to this process, they don't block others and
     * aren't blocked by others.  JAB
     */

    /*
     * locates the 2 exits from room, exits are n/e/s/w/u/d, first in
     * this list is exit1, second is exit2, always.
     */

    for (j = 0; j < NUM_EXITS; j++)
      if( world[ch->in_room].dir_option[(int) j])       /*
                                                         * * it's an exit
                                                         */
        if( exit1 == -1)        /*
                                 * * found an exit yet?
                                 */
          exit1 = j;
        else if( exit2 == -1)   /*
                                 * * found second exit yet?
                                 */
          exit2 = j;
        else
          exit3 = j;            /*
                                 * * this is only here for error checking
                                 */

    if( (exit1 == -1) || (exit2 == -1))
    {
      REMOVE_BIT(world[ch->in_room].room_flags, SINGLE_FILE);
      logit(LOG_DEBUG, "Room %d set SINGLE_FILE with < 2 exits",
            world[ch->in_room].number);
      exit1 = -1;               /*
                                 * * will cause normal behavior
                                 */
    }
    if( exit3 != -1)
    {
      REMOVE_BIT(world[ch->in_room].room_flags, SINGLE_FILE);
      logit(LOG_DEBUG, "Room %d set SINGLE_FILE with > 2 exits",
            world[ch->in_room].number);
      exit1 = -1;               /*
                                 * * will cause normal behavior
                                 */
    }
    for (k = world[ch->in_room].people; k && (k != ch); k = k->next_in_room)
      if( !IS_TRUSTED(k))
        block1 = k;

    block2 = ch->next_in_room;
    while (block2 && IS_TRUSTED(block2))
      block2 = block2->next_in_room;

    /*
     * ok, at this point: block1  holds the ch that is 'in the way' of
     * ch if he's trying to move in the exit1 direction, or NULL if
     * way is clear. block2  holds the ch that is 'in the way' of ch
     * if he's trying to move in the exit2 direction, or NULL if way
     * is clear.
     */

    /*
     * if exit1 is not a valid direction, then there was a problem
     * with the room, and the SINGLE_FILE flag is now gone, so we
     * don't do anything.
     */

    if( exit1 != -1)
    {
      if( (exitnumb == exit1) && block1)        /*
                                                 * * ch trying to move in * exit1 dir
                                                 *
                                                 */
        t_ch = block1;

      if( (exitnumb == exit2) && block2)        /*
                                                 * * ch trying to move in * exit2 dir
                                                 *
                                                 */
        t_ch = block2;

      /*
       * if t_ch is NULL at this point, the way is clear, and we
       * don't have to do anything special, so skip the rest.  If
       * it's non-NULL then it holds the char that is directly in
       * ch's way.
       */

      if( t_ch)
      {

        /*
         * ok, here, we have a character (t_ch) who is in the way,
         * in a SINGLE FILE room, of another character (ch) who is
         * trying to move. Three cases: 1. t_ch is prone, and not
         * fighting, ch can climb over t_ch. 2. t_ch is not prone,
         * and not fighting, but ch is prone, ch can slither passed
         * t_ch. 3. all other cases, ch bumps into t_ch, and ch
         * can't move in that direction, move aborts. JAB
         */

        if( (GET_POS(t_ch) == POS_PRONE) && !IS_FIGHTING(t_ch) &&
            (GET_POS(ch) > POS_PRONE) &&
            (!IS_FIGHTING(ch) || (ch->specials.fighting != t_ch)))
        {

          /*
           * case 1:  t_ch is prone, we can clamber over them.
           */

          act("Grunt!  You clamber over $N's supine form.",
              FALSE, ch, 0, t_ch, TO_CHAR);
          act("$n clambers over $N's prone body.",
              TRUE, ch, 0, t_ch, TO_NOTVICT);
          act("Ooof!  $n just WALKED over you!", FALSE, ch, 0, t_ch, TO_VICT);

          CharWait(ch, 6);

          /*
           * if they are just normally sleeping, they are damn
           * well gonna wake up!  Incap, paralyzed, or slept
           * chars just have to take it.
           */

          if( (GET_STAT(t_ch) == STAT_SLEEPING) &&
              (!affected_by_spell(t_ch, SPELL_SLEEP) ||
               NewSaves(t_ch, SAVING_SPELL, 2)) &&
              !IS_AFFECTED2(t_ch, AFF2_MINOR_PARALYSIS) &&
              !IS_AFFECTED2(t_ch, AFF2_MAJOR_PARALYSIS))
          {
            send_to_char("Nobody could sleep through THAT!\n", t_ch);
            act("$n snorts and comes awake!", TRUE, t_ch, 0, 0, TO_ROOM);
            if( affected_by_spell(t_ch, SPELL_SLEEP))
              affect_from_char(t_ch, SPELL_SLEEP);
            SET_POS(t_ch, GET_POS(t_ch) + STAT_NORMAL);
          }
        }
        else if( (GET_POS(t_ch) > POS_PRONE) && !IS_FIGHTING(t_ch) &&
                 (GET_POS(ch) == POS_PRONE) &&
                 (!IS_FIGHTING(ch) || (ch->specials.fighting != t_ch)))
        {

          /*
           * case 2:  ch is prone, and can slither by t_ch.
           */

          act("Ummph! You slither passed $N.", FALSE, ch, 0, t_ch, TO_CHAR);
          act("$n slithers passed $N.", FALSE, ch, 0, t_ch, TO_NOTVICT);
          act("Damn! $n just slithered passed you!", FALSE, ch, 0, t_ch, TO_VICT);

          CharWait(ch, 8);

          /*
           * if they are just normally sleeping, they are damn
           * well gonna wake up!  Incap, paralyzed, or slept
           * chars just have to take it.
           */

          if( (GET_STAT(t_ch) == STAT_SLEEPING) && !number(0, 2) &&
              (!affected_by_spell(t_ch, SPELL_SLEEP) ||
               NewSaves(t_ch, SAVING_SPELL, 2)) &&
              !IS_AFFECTED2(t_ch, AFF2_MINOR_PARALYSIS) &&
              !IS_AFFECTED2(t_ch, AFF2_MAJOR_PARALYSIS))
          {
            act("$n snorts and comes awake!", TRUE, t_ch, 0, 0, TO_ROOM);
            if( affected_by_spell(t_ch, SPELL_SLEEP))
              affect_from_char(t_ch, SPELL_SLEEP);
            SET_POS(t_ch, GET_POS(t_ch) + STAT_NORMAL);
          }
        }
        else
        {

          /*
           * default case, either t_ch is fighting, or the
           * positions are not compatible.
           */

          act("Oof!  It seems that $N is in your way.",
              FALSE, ch, 0, t_ch, TO_CHAR);
          act("Oof!  $n bumps into you.", FALSE, ch, 0, t_ch, TO_VICT);
          act("$n bumps into $N.", TRUE, ch, 0, t_ch, TO_NOTVICT);

          /*
           * if they are hiding in a SINGLE_FILE room, they
           * aren't going to be after someone runs into them!
           * Still might be invis though.
           */

          if( IS_AFFECTED(t_ch, AFF_HIDE))
          {
            REMOVE_BIT(t_ch->specials.affected_by, AFF_HIDE);
            act("Hey!  $N was hiding here!", FALSE, ch, 0, t_ch, TO_CHAR);
            act("Damn!  $n almost tripped over you!  Lousy hiding place.",
                FALSE, ch, 0, t_ch, TO_VICT);
            act("$n almost tripped over $N's hiding place!",
                TRUE, ch, 0, t_ch, TO_NOTVICT);
          }
          return FALSE;
        }

        /*
         * final check, if ch is fighting (but not fighting t_ch),
         * then we have to stop them when they (effectively) flee,
         * we have to stop the fight.  JAB
         */

        if( IS_FIGHTING(ch))
        {
          if( IS_FIGHTING(ch->specials.fighting) &&
              (ch->specials.fighting->specials.fighting == ch))
            stop_fighting(ch->specials.fighting);
          stop_fighting(ch);
        }
        /*
         * ok, swap ch's and t_ch's positions in room list, so
         * t_ch is no longer in the way (unless ch tries to go
         * back the other way, of course)
         */

        /*
         * made this a function to avoid repetitious code
         */

        SwapCharsInList(ch, t_ch);

        /*
         * they stay in room, have to move again, not gonna add a
         * recursive call, this is more realistic
         */
        return FALSE;
      }
    }
  }
  return TRUE;
}

/*
 * Needed this for teleports and such
 */
int can_enter_room(P_char ch, int room, int show_msg)
{
  int      i;
  bool     has_boat;
  P_obj    obj;
  P_char   pers;

  if( !ch)
  {
    logit(LOG_DEBUG, "Null ch to can_enter_room().");
    return FALSE;
  }
  
  if( IS_NPC(ch) && ((mob_index[GET_RNUM(ch)].number == 11002) ||
                     (mob_index[GET_RNUM(ch)].number == 11003) ||
                     (mob_index[GET_RNUM(ch)].number == 11004)))
    return FALSE;

  if( check_castle_walls(ch->in_room, room) && !IS_TRUSTED(ch) )
  {
    if( show_msg)
      send_to_char("Castle walls can't be simply walked through!\n", ch);
    return FALSE;
  }

#ifdef SIEGE_ENABLED
  if( IS_INVADER( ch ) )
  {
    if( check_gates( ch, room) )
    {
      if( show_msg)
      {
        send_to_char("You can not pass through the gates!\n", ch);
      }
      return FALSE;
    }
  }
#endif

  if(world[room].sector_type == SECT_UNDRWLD_MOUNTAIN &&
     IS_MAP_ROOM(ch->in_room) &&
     IS_MAP_ROOM(room))
  {
    if( !IS_TRUSTED(ch))
    {
      if( show_msg)
        send_to_char("The cavern wall is too steep to climb!\n", ch);
      
      return FALSE;
    }
  }

  if(world[room].sector_type == SECT_MOUNTAIN &&
     IS_MAP_ROOM(ch->in_room) &&
     IS_MAP_ROOM(room))
  {
    if(!IS_TRUSTED(ch))
    {
      if( show_msg)
        send_to_char("The mountains are too treacherous to be scaled.  Find another way to pass them.\n", ch);
      return FALSE;
    }
  }
  
  if( IS_SET(world[room].room_flags, PRIVATE))
  {
    for (i = 0, pers = world[room].people; pers;
         pers = pers->next_in_room, i++) ;
    if( i > 1 && GET_LEVEL(ch) < 59)
    {
      if( show_msg)
        send_to_char("There's a private conversation going on over there.\n",
                     ch);
      return (0);
    }
  }
  /*
   * tunnels are not single-file, but are still pretty small
   */
  if( (world[room].room_flags & (INDOORS | NO_PRECIP)) &&
      ch->specials.z_cord > 0)
  {
    if( !IS_TRUSTED(ch))
    {
      if( show_msg)
        send_to_char("You would bash your brains out, better land first.\n",
                     ch);
      return (0);
    }
    else
    {
      send_to_char("Oof! A tight fit, best land.\n", ch);
      ch->specials.z_cord = 0;
    }
  }
  if( ch->specials.z_cord > 0)
    if( (world[room].room_flags & TUNNEL))
    {
      if( !IS_TRUSTED(ch))
      {
        if( show_msg)
          send_to_char("The passage is too tight to fly.\n", ch);
        return (0);
      }
      else
      {
        send_to_char("Oof! A tight fit, best land.\n", ch);
        ch->specials.z_cord = 0;
      }
    }
  /* low ceilings require crawling */
  if( world[room].sector_type == SECT_UNDRWLD_LOWCEIL &&
      GET_POS(ch) > POS_KNEELING &&
      !IS_MAP_ROOM(ch->in_room) &&
      !IS_MAP_ROOM(room))
  {
    if( !IS_TRUSTED(ch))
    {
      if( show_msg)
        send_to_char("The ceiling is to low for standing. Try crawling in.\n",
                     ch);
      return (0);
    }
    else
    {
      send_to_char("Oof! A tight fit, best crawl.\n", ch);
      SET_POS(ch, POS_KNEELING + GET_STAT(ch));
    }
  }
  /*
   * can't ride into a narrow hallway (RIDING) -DCL
   */
  if( GET_RACE(ch) == RACE_AQUATIC_ANIMAL)        /* fish can only go into water */
    if( (world[room].sector_type != SECT_OCEAN) &&
        (world[room].sector_type != SECT_UNDERWATER) &&
        (world[room].sector_type != SECT_UNDERWATER_GR) &&
        (world[room].sector_type != SECT_WATER_SWIM) &&
        (world[room].sector_type != SECT_WATER_NOSWIM) &&
        (world[room].sector_type != SECT_UNDRWLD_WATER) &&
        (world[room].sector_type != SECT_UNDRWLD_NOSWIM) &&
        (world[room].sector_type != SECT_WATER_PLANE))
      return FALSE;

  /*
   * cant move around on ocean unless in a ship --TAM 04/16/94
   */
#if 1
  if( world[room].sector_type == SECT_OCEAN && !IS_TRUSTED(ch) &&       // !IS_SET(world[ch->in_room].room_flags, DOCKABLE) &&
      world[ch->in_room].sector_type != SECT_OCEAN &&
      !IS_SET(world[room].room_flags, UNDERWATER) )
  {
    if( !is_ice(ch, room))
    {
      if( show_msg && !IS_AFFECTED(ch, AFF_FLY))
        send_to_char("Find a dock to swim from!\n", ch);
      else
      {
        if( show_msg && IS_AFFECTED(ch, AFF_FLY))
          send_to_char
            ("The headwinds would prove much too great. Ye best find a dock to start from!\n",
             ch);
      }
      return (0);
    }
  }
#endif
  if( world[room].sector_type == SECT_WATER_NOSWIM)
  {
    has_boat = FALSE;
    /*
     * See if char is carrying a boat
     */
    for (obj = ch->carrying; obj; obj = obj->next_content)
      if( obj->type == ITEM_BOAT)
        has_boat = TRUE;
    /*
     * See if char is wearing a boat (water walking items actually)
     */
    for (i = 0; i < MAX_WEAR; i++)
      if( ch->equipment[i] && (ch->equipment[i]->type == ITEM_BOAT))
        has_boat = TRUE;

    if( IS_NPC(ch) && IS_SET(ch->specials.act, ACT_CANSWIM))
      has_boat = TRUE;

    /*
     * To aid in being swept away with rivers current
     */
    if( world[ch->in_room].current_speed)
      has_boat = TRUE;

    P_char mount;
    if( (mount = get_linked_char(ch, LNK_RIDING)))
    {
       if( IS_AFFECTED(mount, AFF_FLY) || IS_AFFECTED(mount, AFF_LEVITATE) || IS_SET(mount->specials.act, ACT_CANSWIM))
         has_boat = TRUE;
    }
       
    if( IS_AFFECTED(ch, AFF_FLY))
      has_boat = TRUE;

    if( IS_AFFECTED(ch, AFF_LEVITATE))
      has_boat = TRUE;
    
    if( has_innate(ch, INNATE_SWAMP_SNEAK))
      has_boat = TRUE;

    if( !has_boat && !IS_TRUSTED(ch) && !IS_AFFECTED(ch, AFF_WRAITHFORM) &&
        ch->specials.z_cord < 1)
    {
#if 1
      if( show_msg)
        send_to_char("You need a boat to go there.\n", ch);
      return (0);
#else
      if( show_msg)
        SET_BIT(ch->specials.affected_by3, AFF3_SWIMMING);
      return 1;
#endif
    }
  }
  if( world[room].sector_type == SECT_NO_GROUND)
  {
    if((!IS_TRUSTED(ch)) &&
      (!IS_AFFECTED(ch, AFF_FLY)) &&
      (!IS_AFFECTED(ch, AFF_LEVITATE)) &&
      !IS_AFFECTED(ch, AFF_WRAITHFORM) &&
      (IS_PC(ch)))
    {
      if( show_msg)
        send_to_char("Oops. Forget something?", ch);
      return (1);
    }
  }
  /* If we get this far, it's okay to go this way */
  return (1);
}

char    *enter_message(P_char ch, P_char people, int exitnumb, char *amsg,
                       int was_in, int foo)
{
  char     tmp[512], tmp2[512];
  int      rev;
  P_char   mount;

  if(!ch ||
    !people)
  {
    strcpy(amsg, "enter_message error #157\n");
    return amsg;
  }
  /* build name */
/* removed by Zod
  if( IS_TRUSTED(ch) && ch->player.short_descr)
    strcpy(amsg, ch->player.short_descr);
  else 
*/
  strcpy(amsg, PERS(ch, people, TRUE));

  strcat(amsg, " ");

  /* describe person if needed */


  /* add placeholder and direction */
  if(ch->specials.z_cord != people->specials.z_cord)
  {
    if(ch->specials.z_cord > people->specials.z_cord)
    {
      strcat(amsg, "%s above you");
    }
    else
    {
      strcat(amsg, "%s below you");
    }
  }
  else
  {
    if((exitnumb >= 0) &&
      (exitnumb < NUM_EXITS))
    {
      rev = rev_dir[exitnumb];

      if(world[foo].dir_option[rev] &&
        (world[foo].dir_option[rev]->to_room == was_in))
      {
        sprintf(amsg + strlen(amsg), "%%s from %s", dirs2[rev]);
      }
      else
      {
        strcat(amsg, "%s from elsewhere");
      }
    }
    else
    {
      strcat(amsg, "%s from elsewhere");
    }
  }

  // For Linked Objects  - Dalreth
  mount = IS_RIDING(ch);

  if(ch->lobj && ch->lobj->Visible_Type())
  {
    strcat(amsg, " ");
    strcat(amsg, ch->lobj->Visible_Message());
    strcat(amsg, " $p");
  }

  if(mount &&
     mount->lobj &&
     mount->lobj->Visible_Type())
  {
    strcat(amsg, " ");
    strcat(amsg, mount->lobj->Visible_Message());
    strcat(amsg, " $p");
  }
  
  strcat(amsg, ".");

  /* finally, fill in the placeholder with some type of verb */
  if(mount)
  {
    if(CAN_SEE(people, mount))
    {
      if((IS_SET(world[ch->in_room].room_flags, UNDERWATER)) ||
        (ch->specials.z_cord < 0))
      {
        sprintf(tmp2, "swims in on %s", J_NAME(mount));
      }
      else
      {
        sprintf(tmp2, "rides in on %s", J_NAME(mount));
      }
    }
    else if((IS_SET(world[ch->in_room].room_flags, UNDERWATER)) ||
           (ch->specials.z_cord < 0))
    {
      strcpy(tmp2, "swims in on something");
    }
    else
    {
      strcpy(tmp2, "rides in on something");
    }
    /* amsg's only %s is placeholder for verb, which is now in tmp2 */

    sprintf(tmp, amsg, tmp2);

    strcpy(amsg, tmp);
  }
  else
  { /* char is not riding/pulling anything */

    /* amsg's only %s is placeholder for verb .. */

    sprintf(tmp, amsg,
            IS_SET(world[ch->in_room].room_flags, UNDERWATER) ? "swims in" :
            ch->specials.z_cord < 0 ? "swims in" :
            ch->specials.z_cord > 0 ? "flies in" :
            LEVITATE(ch, exitnumb) ? "floats in" :
            IS_SLIME(ch) ? "oozes in" :
            GET_RACE(ch) == RACE_DRAGON ? "lumbers in" :
            load_modifier(ch) > 199 ? "staggers in" :
            (SNEAK(ch) && !mount) ? "sneaks in" :
            GET_POS(ch) == POS_PRONE ? "slithers in" :
            GET_POS(ch) == POS_KNEELING ? "crawls in" :
            has_innate(ch, INNATE_HORSE_BODY) ? "trots in" :
	    has_innate(ch, INNATE_SPIDER_BODY) ? "skitters in" : "enters");

    if(SNEAK(ch) &&
      (!ch->lobj ||
      (ch->lobj &&
      !ch->lobj->Visible_Type())))
    {
      if(IS_TRUSTED(people) ||
        ((IS_AFFECTED(people, AFF_SENSE_LIFE) ||
        IS_AFFECTED(people, AFF_SKILL_AWARE)) &&
        StatSave(people, APPLY_INT, -4)) ||
          (GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF) &&
          affected_by_spell(ch, SKILL_SNEAK) &&
          (number(0, 100) > GET_CHAR_SKILL(ch, SKILL_SNEAK))))
        strcpy(amsg, tmp);
      else
        amsg[0] = '\0';         /* they ain't seeing jack */
    }
    else
      strcpy(amsg, tmp);
  }

  return amsg;
}

char    *leave_message(P_char ch, P_char people, int exitnumb, char *amsg)
{
  char     tmp[512];
  P_char   mount;

  if( !ch || !people)
  {
    strcpy(amsg, "leave_message error #283\n");
    return amsg;
  }

  strcpy(amsg, PERS(ch, people, TRUE));
  strcat(amsg, " ");

  /* add verb and direction */
  sprintf(amsg + strlen(amsg), "%s %s",
          IS_SET(world[ch->in_room].room_flags, UNDERWATER) ? "swims" :
          ch->specials.z_cord < 0 ? "swims" :
          ch->specials.z_cord > 0 ? "flies" :
          LEVITATE(ch, exitnumb) ? "floats" :
          IS_SLIME(ch) ? "oozes" :
          load_modifier(ch) > 199 ? "staggers" :
          SNEAK(ch) ? "sneaks" :
          GET_POS(ch) == POS_PRONE ? "slithers" :
          GET_POS(ch) == POS_KNEELING ? "crawls" :
          IS_CENTAUR(ch) ? "trots" : "leaves", dirs[exitnumb]);

  /* add goodies */
  if( ch->specials.z_cord != people->specials.z_cord)
  {
    if( ch->specials.z_cord > people->specials.z_cord)
      strcat(amsg, " above you");
    else
      strcat(amsg, " below you");
  }

  mount = IS_RIDING(ch);
  if(ch->lobj && ch->lobj->Visible_Type()) {
    strcat(amsg, " ");
    strcat(amsg, ch->lobj->Visible_Message());
    strcat(amsg, " $p");
  }

  if( mount)
  {
    sprintf(tmp, "%s riding on %s", amsg, CAN_SEE(people, mount) ?
            mount->player.short_descr : "something");
    strcpy(amsg, tmp);
  }

  if(mount && mount->lobj && mount->lobj->Visible_Type()) {
    strcat(amsg, " ");
    strcat(amsg, mount->lobj->Visible_Message());
    strcat(amsg, " $p");
  }
  strcat(amsg, ".");

  return amsg;
}

#undef SNEAK
#undef LEVITATE

/* Used for high winds blowing flying chars */
void blow_char_somewhere_else(P_char ch, int dir)
{
  P_char   t_ch = NULL;
  int      to_room, distance, zone_num, rroom;
  struct weather_data *cond;

  if( !IS_MAP_ROOM(ch->in_room) || IS_TRUSTED(ch))
    return;

  zone_num = world[ch->in_room].zone;
  cond = &sector_table[in_weather_sector(ch->in_room)].conditions;
  distance = BOUNDED(0, (cond->windspeed - 45) / 2, 20);

  switch (dir)
  {
  case NORTH:
    to_room = world[ch->in_room].number - (100 * distance);
    break;
  case EAST:
    to_room = world[ch->in_room].number + (1 * distance);
    break;
  case SOUTH:
    to_room = world[ch->in_room].number + (100 * distance);
    break;
  case WEST:
    to_room = world[ch->in_room].number - (1 * distance);
    break;
  case NORTHWEST:
    to_room =
      world[ch->in_room].number - ((100 * distance) / 2) -
      ((1 * distance) / 2);
    break;
  case NORTHEAST:
    to_room =
      world[ch->in_room].number - ((100 * distance) / 2) +
      ((1 * distance) / 2);
    break;
  case SOUTHWEST:
    to_room =
      world[ch->in_room].number + ((100 * distance) / 2) -
      ((1 * distance) / 2);
    break;
  case SOUTHEAST:
    to_room =
      world[ch->in_room].number + ((100 * distance) / 2) +
      ((1 * distance) / 2);
    break;
  default:
    to_room = world[ch->in_room].number;
    break;
  }

  rroom = real_room0(to_room);

  if( !rroom || (world[rroom].sector_type == SECT_OCEAN))
    return;

  if( IS_FIGHTING(ch))
    stop_fighting(ch);
  if( IS_DESTROYING(ch) )
    stop_destroying(ch);
  for (t_ch = world[ch->in_room].people; t_ch; t_ch = t_ch->next)
    if( IS_FIGHTING(t_ch) && (t_ch->specials.fighting == ch))
      stop_fighting(t_ch);

  send_to_char("The mighty winds toss you around like an orc in the ocean!\n",
               ch);
  act("The mighty winds toss $n around like an orc in the ocean!", FALSE, ch,
      0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, rroom, -1);

  act("$n is suddenly blown in from elsewhere.", FALSE, ch, 0, 0, TO_ROOM);
}

/*
 * This function is meant to be called from procs and do_simple_move only
 */
int do_simple_move_skipping_procs(P_char ch, int exitnumb, unsigned int flags)
{
  P_char   tch, t_ch;
  char     amsg[MAX_STRING_LENGTH];
  int      need_movement, was_in, new_room, count, i, cmd, cmd2, current, following;
  int      deceptnum, noise_var, calming = 0;
  struct weather_data *cond;
  struct follow_type *k, *next_dude;
  P_char   mount, rider, moving;
  struct zone_data *zone;

  if( IS_IMMOBILE(ch) )
  {
    return FALSE;
  }

  if( ch->in_room == NOWHERE )
  {
    return 0;
  }

  if( (exitnumb < 0) || (exitnumb >= NUM_EXITS) )
  {
    return FALSE;
  }

  if( !EXIT(ch, exitnumb) || (EXIT(ch, exitnumb)->to_room == NOWHERE)
    || ((IS_NPC(ch) && IS_SET(ch->specials.act, ACT_STAY_ZONE)
    && (world[ch->in_room].zone != world[EXIT(ch, exitnumb)->to_room].zone) && !GET_MASTER(ch))) )
  {
    send_to_char("Alas, you cannot go that way. . . .\n", ch);
    return FALSE;
  }

  if( affected_by_spell(ch, SKILL_DREADNAUGHT) )
  {
    send_to_char("&+RYou are too busy concentrating on defensive manouvers to move!\r\n", ch);
    return FALSE;
  }

  if( !IS_TRUSTED(ch) && (IS_SET(EXIT(ch, exitnumb)->exit_info, EX_SECRET)
    || IS_SET(EXIT(ch, exitnumb)->exit_info, EX_BLOCKED)) )
  {
    send_to_char("Alas, you cannot go that way. . . .\n", ch);
    return FALSE;
  }

  if( !leave_by_exit(ch, exitnumb) )
  {
    send_to_char("You can't leave that way.\n", ch);
    return FALSE;
  }

  if( !can_enter_room(ch, EXIT(ch, exitnumb)->to_room, TRUE) )
  {
    return FALSE;
  }

  if( IS_SET(EXIT(ch, exitnumb)->exit_info, EX_CLOSED) )
  {
    if( IS_TRUSTED(ch) )
    {
      send_to_char("You go with your godly self, ignoring physical barriers and all...\n", ch);
    }
    else if( IS_AFFECTED2(ch, AFF2_PASSDOOR) && (!IS_SET(EXIT(ch, exitnumb)->exit_info, EX_LOCKED)
      || !IS_SET(EXIT(ch, exitnumb)->exit_info, EX_PICKPROOF)) )
    {
      send_to_char("Your body vibrates as you fan out your molecules.\n", ch);
    }
    else
    {
      if( EXIT(ch, exitnumb)->keyword )
      {
        sprintf(amsg, "The %s seems to be closed.\n", FirstWord(EXIT(ch, exitnumb)->keyword));
        send_to_char(amsg, ch);
      }
      else
      {
        send_to_char("It seems to be closed.\n", ch);
      }

      return FALSE;
    }
  }
  else if( world[ch->in_room].current_speed && !IS_TRUSTED(ch) )
  {
    current = world[ch->in_room].current_direction;

    if( exitnumb == rev_dir[current] )
    {
      if( number(1, 101) < (world[ch->in_room].current_speed - (GET_C_STR(ch) / 3)) )
      {
        if( IS_WATER_ROOM(ch->in_room) && (ch->specials.z_cord < 1)
          && !IS_AFFECTED(ch, AFF_LEVITATE) && !IS_AFFECTED(ch, AFF_FLY) )
        {
          send_to_char("The force of the current prevents your movements against them.\n", ch);
          return FALSE;
        }
      }
    }
  }

  if( GET_POS(ch) == POS_SITTING )
  {
    send_to_char("Perhaps you should get on your feet first?\n", ch);
    return FALSE;
  }
  if( GET_STAT(ch) < STAT_RESTING )
    return 0;

  if( IS_AFFECTED(ch, AFF_BOUND) )
  {
    send_to_char("Your bonds prevent that!\n", ch);
    return FALSE;
  }

  if( IS_AFFECTED2(ch, AFF2_MINOR_INVIS) )
  {
    send_to_char("You reappear, visible to all.\n", ch);
    affect_from_char(ch, SPELL_INVISIBLE);
  }

  if( IS_CARRYING_W(ch) > CAN_CARRY_W(ch) && IS_PC(ch) )
  {
    send_to_char("You collapse under your carried load!\n", ch);
    act("$n collapses under the weight of $s inventory!", TRUE, ch, 0, 0, TO_ROOM);
    return 0;
  }
  if( ch->in_room == NOWHERE )
    return 0;


  if( GET_POS(ch) < POS_STANDING && (ch->lobj && ch->lobj->Visible_Type()) )
  {
    send_to_char("You need to be on your feet to move with your load!\n", ch);
    return FALSE;
  }

  mount = IS_RIDING(ch);

  if( mount )
  {
    moving = mount;
  }
  else
  {
    moving = ch;
  }

  if( mount && mount->specials.fighting )
  {
    send_to_char("&+WYour mount is in combat, maybe you should stay and fight!&n\n", ch);
    return FALSE;
  }

  rider = get_linking_char(ch, LNK_RIDING);
  if( rider && GET_OPPONENT(rider) )
  {
    send_to_char("Your rider is busy fighting, you cannot move away!\n", ch);
    return FALSE;
  }

  if( (mount || rider) && world[world[ch->in_room].dir_option[exitnumb]->to_room].sector_type == SECT_OCEAN )
  {
    if( mount )
    {
      send_to_char("It is too difficult to move there with someone on your back!\n", mount);
    }
    send_to_char("It is too difficult to direct your mount in the ocean!\n", ch);

    return FALSE;
  }

  need_movement = move_cost(moving, exitnumb);

  if( GET_POS(moving) == POS_PRONE)
  {
    need_movement += 6;
  }
  else if( GET_POS(moving) == POS_KNEELING)
  {
    need_movement += 3;
  }

  if( IS_AFFECTED2(moving, AFF2_FLURRY))
  {
    need_movement += 2;
  }

  /* High winds?  */
#if 0
  if( IS_PC(ch) && OUTSIDE(ch))
  {
    cond = &sector_table[in_weather_sector(ch->in_room)].conditions;
    if( (cond->wind_dir == exitnumb) && (cond->windspeed > 50))
      if( ch->specials.z_cord > 0)
      {
        blow_char_somewhere_else(ch, exitnumb);
        return 1;
      }
      else
        need_movement += BOUNDED(0, (cond->windspeed - 45) / 2, 10);
  }
#endif

  // check for guild golems moving..
  if( IS_NPC(ch) && ((mob_index[GET_RNUM(ch)].number == 11002)
    || (mob_index[GET_RNUM(ch)].number == 11003) || (mob_index[GET_RNUM(ch)].number == 11004)) )
  {
    return 0;
  }

  if( affected_by_spell(moving, SPELL_BLOODTOSTONE) )
  {
    need_movement += number(1,4);
  }

  if( affected_by_spell(moving, TAG_CRIPPLED) )
  {
    need_movement += 5;
  }

  if( IS_PC(ch) && GET_CHAR_SKILL(ch, SKILL_SNEAK) > 0 && affected_by_spell(ch, SKILL_SNEAK)
    && !mount && GET_VITALITY(ch) > need_movement )
  {
    notch_skill(ch, SKILL_SNEAK, 5);
  }

/*
  if( affected_by_spell(ch, TAG_PVPDELAY) ){
    send_to_char
          ("The &+Radrenaline&n is pumping through you like mad, this sure is exhausting...&n\n", ch);

    if(IS_PC(ch) &&  IS_THIEF(ch) && ( ch->only.pc->pc_timer[1] + 3 > time(NULL) ) )
      send_to_char
                ("...but as the master of close combat, you take no notice!!&n\n", ch);
    else
      need_movement += number(1,2);
  }
*/

  /* pc_timer[1] gets set on successful flee */
 /*
  if( IS_PC(ch) &&
      (ch->only.pc->pc_timer[1] + (IS_THIEF(ch) ? 5 : 10) > time(NULL)))
  {
    if( need_movement < 4)
      need_movement += 4;
    else
      need_movement <<= 1;

    send_to_char
      ("Panicking, you don't exactly take the most efficient route..\n", ch);
  }
*/
  if( mount )
  {
    if( (GET_VITALITY(mount) < need_movement) && !IS_TRUSTED(ch) && !IS_TRUSTED(mount) )
    {
      send_to_char("Your mount is too exhausted.\n", ch);
      return 0;
    }
    if( GET_VITALITY(ch) < 1 )  // can happen with 'blood to stone'
    {
      send_to_char("You're too exhausted to control your mount.\n", ch);
      return 0;
    }
  }
  else
  {
    if( (GET_VITALITY(ch) < need_movement) && !IS_TRUSTED(ch) )
    {
      send_to_char("You're too exhausted.\n", ch);
      return 0;
    }
  }

  if( IS_AFFECTED5(ch, AFF5_VINES) )
  {
    send_to_char("The &+Gvines&n surrounding you crumble and fall to the ground.\n", ch);
    affect_from_char(ch, SPELL_VINES);
  }

 /* if( is_illusion_char(ch) && GET_CLASS(ch, CLASS_ILLUSIONIST) && !GET_SPEC(ch, CLASS_ILLUSIONIST, SPEC_DECEIVER))
  {
   send_to_char("&+LUnable to maintain your &+mspell's &+Lfocus, you f&+wa&+Wde &+Lback into your own image.&n\n", ch);
   remove_disguise(ch, FALSE);
  } drannak - reenabling*/

  /* Trap check */
  if( checkmovetrap(ch, exitnumb) )
  {
    return 0;
  }

  // It takes 33% more moves to carry someone now.
  if( mount && IS_NPC(mount) )
  {
    // 33% more and at least one more move.
    need_movement = (need_movement>2) ? (4*need_movement)/3 : need_movement+1;
  }

  if( !IS_TRUSTED(ch) )
  {
    if( mount )
    {
      if( !IS_TRUSTED(mount) )
      {
        GET_VITALITY(mount) -= need_movement;
      }
    }
    else
    {
      GET_VITALITY(ch) -= need_movement;
    }
  }
  if( !(flags & MVFLG_NOMSG) )
  {
    LOOP_THRU_PEOPLE(tch, ch)
    {
      if( (ch == tch) || !IS_AWAKE(tch) )
      {
        continue;
      }
      amsg[0] = '\0';

      for( flags & MVFLG_FLEE ? (has_innate(ch, INNATE_DECEPTIVE_FLEE) ? deceptnum = 9 : deceptnum = 0) : deceptnum = 0; deceptnum >= 0; deceptnum-- )
      {
        if( has_innate(ch, INNATE_DECEPTIVE_FLEE) )
        {
          if( !CAN_GO(ch, deceptnum))
          {
            continue;
          }
        }
        leave_message(ch, tch, flags & MVFLG_FLEE ? (has_innate(ch, INNATE_DECEPTIVE_FLEE) ? deceptnum : exitnumb) : exitnumb, amsg);

        if( mount )
        {
          act(amsg, TRUE, ch, mount->lobj?mount->lobj->Visible_Object():0, tch, TO_VICT | ACT_IGNORE_ZCOORD);
        }
        else if( !IS_AFFECTED(ch, AFF_SNEAK) && !UD_SNEAK(ch) && !OUTDOOR_SNEAK(ch) && !SWAMP_SNEAK(ch) )
        {
          act(amsg, TRUE, ch, ch->lobj?ch->lobj->Visible_Object():0, tch, TO_VICT | ACT_IGNORE_ZCOORD);
        }
        else
        {
          /* sneaking, gods see it, and certain others may detect it as well. */
          if( IS_TRUSTED(tch) || ((IS_AFFECTED(tch, AFF_SENSE_LIFE) || (ch->lobj && ch->lobj->Visible_Type())
            || IS_AFFECTED(tch, AFF_SKILL_AWARE)) && StatSave(tch, APPLY_INT, -4)) )
          {
            act(amsg, TRUE,ch,ch->lobj?ch->lobj->Visible_Object():0, tch, TO_VICT | ACT_IGNORE_ZCOORD);
          }
        }
      }
    }
  }

#if 0
  if( IS_SHADOWING(ch) && !IS_SHADOW_MOVE(ch))
  {
    act("You stop shadowing $N.",
      FALSE, ch, 0, GET_CHAR_SHADOWED(ch), TO_CHAR);
    FreeShadowedData(ch, GET_CHAR_SHADOWED(ch));
  }

  if( IS_BEING_SHADOWED(ch))
  {
    ch->specials.shadow.valid_last_move = TRUE;
  }
#endif

  was_in = ch->in_room;
  new_room = world[was_in].dir_option[exitnumb]->to_room;
  zone = &zone_table[world[new_room].zone];
  if( IS_SET(world[was_in].room_flags, LOCKER) )
  {
    if( zone->status > ZONE_NORMAL )
    {
      // this functionality has been disabled
      new_room = alt_hometown_check(ch, new_room, 0);
    }
  }

  if( new_room == NOWHERE )
  {
    return 0;
  }

  check_room_links(ch, was_in, new_room);

  following = FALSE;
  if( affected_by_spell(ch, SPELL_SHADOW_MERGE))
  {
    for (t_ch = world[new_room].people; t_ch; t_ch = t_ch->next_in_room)
    {
      if( ch->following == t_ch)
      {
        following = TRUE;
      }
    }
    if( !following )
    {
      send_to_char("You step out of the &+Lshadows&n.\r\n", ch);
      if( affected_by_spell(ch, SPELL_SHADOW_MERGE) )
      {
         affect_from_char(ch, SPELL_SHADOW_MERGE);
      }
      REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
    }
  }

/* hack for the exping outside the zone exploit */

  if( IS_NPC(moving) && (zone != &zone_table[world[was_in].zone])
    && (world[was_in].number != GET_BIRTHPLACE(moving))
    && !affected_by_spell(moving, TAG_REDUCED_EXP) && !IS_MORPH(moving) )
  {
    int duration = WAIT_SEC*SECS_PER_REAL_HOUR*GET_LEVEL(moving)/60;
    set_short_affected_by(moving, TAG_REDUCED_EXP, duration);
  }

  /*
   * to make everything work right, have to move them twice, once so
   * CAN_SEE will work right, and second time to actually move them. JAB
   */
  if( world[new_room].people )
  {
    char_from_room(ch);
    if( !char_to_room(ch, new_room, -2) )
    {
      return FALSE;
    }
    char_light(ch);
    room_light(ch->in_room, REAL);

    LOOP_THRU_PEOPLE(tch, ch)
    {
      if( (ch == tch) || !IS_AWAKE(tch) )
      {
        continue;
      }

      amsg[0] = '\0';
      enter_message(ch, tch, exitnumb, amsg, was_in, new_room);

      int nocalming = 0;
      if( (RACE_EVIL(ch) && IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_GOODHOME)) ||
          (RACE_GOOD(ch) && IS_SET(hometowns[VNUM2TOWN(world[ch->in_room].number)-1].flags, JUSTICE_EVILHOME)))
      nocalming = 1;

      if( !IS_ELITE(tch) && !nocalming && (((GET_LEVEL(tch) - GET_LEVEL(ch)) <= 5) || !number(0, 3))
        && has_innate(ch, INNATE_CALMING) )
      {
        calming = (int)get_property("innate.calming.delay", 10);
      }

      if( mount )
      {
        act(amsg, TRUE, ch, mount->lobj?mount->lobj->Visible_Object():0, tch, TO_VICT | ACT_IGNORE_ZCOORD);
        if( is_aggr_to(tch, ch) )
        {
          add_event(event_agg_attack, number(1, MAX(1, (19 - STAT_INDEX(GET_C_AGI(tch))) / 2)) + calming, tch, ch, 0, 0, 0, 0);
        }
        /* cackle   */
        else if( is_aggr_to(tch, mount) )
        {
          add_event(event_agg_attack, number(1, MAX(1, (19 - STAT_INDEX(GET_C_AGI(tch))) / 2)), tch, mount, 0, 0, 0, 0);
        }
      }
      else if( !IS_AFFECTED(ch, AFF_SNEAK) && !UD_SNEAK(ch) && !OUTDOOR_SNEAK(ch) && !SWAMP_SNEAK(ch))
      {
        if( !(flags & MVFLG_NOMSG) )
        {
          act(amsg, TRUE, ch, ch->lobj?ch->lobj->Visible_Object():0, tch, TO_VICT | ACT_IGNORE_ZCOORD);
        }

        if( is_aggr_to(tch, ch) )
        {
          add_event(event_agg_attack, number(0, MAX(1, (22 - STAT_INDEX(GET_C_AGI(tch))) / 2)) + calming, tch, ch, 0, 0, 0, 0);
        }
      }
      else
      {
        /*
         * sneaking of some sort, gods see it, and certain others
         * may detect it as well.  JAB
         */
        if( IS_NPC(tch) && isname("_nosneak_", GET_NAME(tch)) && is_aggr_to(tch, ch) )
        {
          add_event(event_agg_attack, 1 + (calming / 3), tch, ch, 0, 0, 0, 0);
        }
        else if( IS_TRUSTED(tch) || ((IS_AFFECTED(tch, AFF_SENSE_LIFE)
          || IS_AFFECTED(tch, AFF_SKILL_AWARE)) && StatSave(tch, APPLY_INT, -4)) )
        {
          if( !(flags & MVFLG_NOMSG) )
          {
            act(amsg, TRUE, ch, ch->lobj?ch->lobj->Visible_Object():0, tch, TO_VICT | ACT_IGNORE_ZCOORD);
          }
          if( is_aggr_to(tch, ch) )
          {
            add_event(event_agg_attack, number(1, MAX(1, (25 - STAT_INDEX(GET_C_AGI(tch))) / 2)) + calming, tch, ch, 0, 0, 0, 0);
          }
        }
        else
        {
          if( is_aggr_to(tch, ch) )
          {
            add_event(event_agg_attack, number(PULSE_VIOLENCE, MAX(PULSE_VIOLENCE, (PULSE_VIOLENCE + 10 -
              dex_app[STAT_INDEX(GET_C_DEX(tch))].reaction))) + calming, tch, ch, 0, 0, 0, 0);
          }
        }
      }
    }
  }

  if( !IS_TRUSTED(ch) )
  {
    add_track(ch, exitnumb);
  }

  //Minor lag to people who are many in a map room. It's slower to move in it's crowded
  if( (world[ch->in_room].sector_type != SECT_ROAD) )
  {
    if( get_number_allies_in_room(ch, ch->in_room) > 2 )
    {
         /* Disabled for now let's see how the msg code works
          *   Glad this is disabled, kinda makes it tough for groups to move when half the group lags.
          send_to_char("Moving in this crowd, and this terrain is slow...\r\n", ch);
          CharWait(ch, 1 );
          */
    }
  }

  // Shade movement... only go vis if we hit a room without shadows.
  if( IS_AFFECTED(ch, AFF_HIDE) && IS_AFFECTED5(ch, AFF5_SHADE_MOVEMENT) )
  {
    if( IS_SUNLIT(new_room) || IS_TWILIGHT_ROOM(new_room) )
    {
      send_to_char("&+LYou quickly step out of the shadow.&n\r\n", ch);
      REMOVE_BIT(ch->specials.affected_by5, AFF5_SHADE_MOVEMENT);
      REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
    }
    else
    {
      send_to_char("&+LYou walk with the shadows.&n\r\n", ch);
    }
  }

  char_from_room(ch);
  ch->specials.was_in_room = world[was_in].number;
  if( !char_to_room(ch, new_room, exitnumb) )
  {
    return FALSE;
  }
/* Vehicle code commented out?  For horse 'n buggy 'n such.
  if( mount && mount->vehicle)
  {
    obj_from_room(mount->vehicle);
    obj_to_room(mount->vehicle, ch->in_room);
  }
  else if( ch->vehicle)
  {
    obj_from_room(ch->vehicle);
    obj_to_room(ch->vehicle, ch->in_room);
  }
*/

  char_light(ch);
  room_light(ch->in_room, REAL);

  /*
   * penalty for sneak skill, slower movement, perm sneak doesn't have
   * this affect. JAB
   */
 /* Removing sneak skill penalty. Apr09 -Lucrot
  if( affected_by_spell(ch, SKILL_SNEAK) && !GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF) )
  {
    CharWait(ch, 2);
  }
  */

  if( IS_SET(ch->specials.affected_by3, AFF3_COVER) )
  {
    REMOVE_BIT(ch->specials.affected_by3, AFF3_COVER);
  }

// old guildhalls (deprecated)
//  if(!IS_SET(world[ch->in_room].room_flags, GUILD_ROOM) &&
//     IS_SET(ch->specials.affected_by4, AFF4_SACKING))
//  {
//    /* they are not in a guild room, but are set as sacking */
//    REMOVE_BIT(ch->specials.affected_by4, AFF4_SACKING);
//    send_to_char("You stop your sacking.\n", ch);
//    //clear_sacks(ch);
//  }
  if( ch->in_room == NOWHERE )
  {
    return 0;
  }
  if( (flags & MVFLG_DRAG_FOLLOWERS) && ch->followers )
  {
    struct room_affect raf, *praf = NULL;

    if( IS_AFFECTED4(ch, AFF4_GLOBE_OF_DARKNESS) && IS_PC(ch) && !CAN_NIGHTPEOPLE_SEE(was_in) )
    {
      memset(&raf, 0, sizeof(struct room_affect));
      raf.type = SPELL_GLOBE_OF_DARKNESS;
      raf.duration = 1;
      raf.room_flags = MAGIC_DARK;
      raf.ch = ch;
      praf = affect_to_room(was_in, &raf);
    }
    else if( IS_AFFECTED4(ch, AFF4_MAGE_FLAME) && IS_PC(ch) && !CAN_DAYPEOPLE_SEE(was_in) )
    {
      memset(&raf, 0, sizeof(struct room_affect));
      raf.type = SPELL_MAGE_FLAME;
      raf.duration = 1;
      raf.room_flags = MAGIC_LIGHT;
      raf.ch = ch;
      praf = affect_to_room(was_in, &raf);
    }

    /*  this is a little lag when you have many tch following you
       if( !ch->following) {
       count = 0;
       for (tch = world[was_in].tch; tch; tch = tch->next_in_room)
       if( IS_PC(tch) && grouped(tch, ch))
       count++;
       if( count > 3 && IS_MAP_ROOM(ch->in_room) )
       CharWait(ch, count/4);
       } */
    cmd = exitnumb_to_cmd(exitnumb) - 1;

    int num_followed = 0;
    for( k = ch->followers; k; k = next_dude )
    {
      next_dude = k->next;
      if( (was_in == k->follower->in_room) && CAN_ACT(k->follower)
        && MIN_POS(k->follower, POS_STANDING + STAT_RESTING) && !IS_FIGHTING(k->follower)
        && !IS_DESTROYING(k->follower) && !NumAttackers(k->follower) && CAN_SEE(k->follower, ch) )
      {
       /* if((IS_NPC(k->follower) &&
          k->follower->group &&
          (k->follower->group->ch != ch)))
        {
          act("You can't follow $N!", FALSE, k->follower, 0, ch, TO_CHAR);
        }*/
        if( affected_by_spell(ch, SPELL_DELIRIUM) && !number(0, 2) )
        {
          cmd2 = number(1, 6);
          while( !CAN_GO(ch, cmd2) && i < 10 )
          {
            i++;
            cmd2 = number(0, 6);
          }

          if( i < 9 )
          {
            send_to_char("&+WYou are &+Gconfused&+W and unable to follow, watch out!&n\n", ch);
            sprintf(amsg, "%s %s", command[cmd2], dirs[cmd_to_exitnumb(cmd2)]);
            command_interpreter(k->follower, amsg);
          }
          else
          {
            send_to_char("&+WYou are &+Gconfused&+W and unable to follow, watch out!&n\n", ch);
          }
        }
        else if( world[ch->in_room].sector_type == SECT_FOREST && IS_MAP_ROOM(ch->in_room)
          && get_property("terrain.forest.lostChance", 5.) > number(0, 99) )
        {
          send_to_char("Oh no! You got lost in these woods!\n", k->follower);
        }
        else
        {
          act("You follow $N.", FALSE, k->follower, 0, ch, TO_CHAR);
          send_to_char("\n", k->follower);
          sprintf(amsg, "%s %s", command[cmd], dirs[exitnumb]);
          SET_BIT(k->follower->specials.affected_by5, AFF5_FOLLOWING);
          command_interpreter(k->follower, amsg);
          REMOVE_BIT(k->follower->specials.affected_by5, AFF5_FOLLOWING);
          num_followed++;
        }
      }
    }

    if( praf != NULL )
    {
      affect_room_remove( was_in, praf );
    }

    if( IS_MAP_ROOM(ch->in_room) )
    {
      // Probably too simple, just count the number of allies in a room.
      noise_var = get_number_allies_in_room(ch, ch->in_room);
	    //noise_var = get_weight_allies_in_room(ch, ch->in_room); //changing to use weight code - 10/5/2012 Drannak

      // Let us randomize the noise a tad.
      noise_var = noise_var + number(-1, 1);

      // Sounds suppression threshold is 13.
      if( affected_by_spell(ch, SPELL_SUPPRESSION) && noise_var <= 13 )
      { // Just a placeholder.
      }
      else
      {
        //send_movement_noise(ch, noise_var  +1);
        send_movement_noise(ch, noise_var);
      }
    }
   /* Commenting this out for now since the num_followed routine was
    *   being circumvented by the players. Jan08 -Lucrot
    // Big groups shouldn't get the suppress sound bonus anyway
    if( IS_MAP_ROOM(ch->in_room) && num_followed >= 10 && affected_by_spell(ch, SPELL_SUPPRESSION) )
    {
      send_to_char("You find that it is very difficult to suppress the sound of such a large force.\n", ch);
      send_movement_noise(ch, (num_followed-5));
    }
    */
  }

  return (1);
}

void send_movement_noise(P_char ch, int num)
{
  if(IS_MAP_ROOM(ch->in_room) &&
    IS_PC(ch))
  {
    for (P_desc i = descriptor_list; i; i = i->next)
    {
      if( i->connected != CON_PLYNG ||
          ch == i->character ||
          i->character->following == ch ||
          world[i->character->in_room].zone != world[ch->in_room].zone ||
          ch->in_room == i->character->in_room ||
          ch->in_room == real_room(i->character->specials.was_in_room) ||
          real_room(ch->specials.was_in_room) == i->character->in_room )
      {
        continue;
      }
      
      int dist = calculate_map_distance(ch->in_room, i->character->in_room);

      if(num >= 20 &&
         dist <= 400)
      {
        send_to_char("&+LA &+Rl&+re&+Rg&+ri&+Ro&+rn &+Lmaneuvers nearby creating a massive plume of &+Wd&+wu&+Ws&+wt.&n\r\n", i->character);
      }
      else if(num >= 13 &&
              num <= 19 &&
              dist <= 300)
      {
        send_to_char("&+rThe ground &+Rb&+ru&+Rc&+rk&+Rl&+re&+Rs&n &+rto the tune of a marching &+Lhorde.&n\r\n", i->character);
      }
      else if(num >= 9 &&
              num <= 12 &&
              dist <= 150 )
      {
        send_to_char("&+yThe earth &+Ytrembles &+yfrom the marching of a nearby army.&n\r\n", i->character);
      }
      else if(num >= 5 &&
              num <= 8 &&
              dist <= 64 &&
              number(0, 4))
      {
        send_to_char("&+cYou hear the sounds of movement in the distance.&n\r\n", i->character);
      }
/*
      if(num >= 30 &&
         dist <= 400)
      {
        send_to_char("&+rThe ground &+Rb&+ru&+Rc&+rk&+Rl&+re&+Rs&n &+rto the tune of a marching &+Lhorde.&n\r\n", i->character);
      }

      else if(num >= 23 &&
              num <= 29 &&
              dist <= 320 )
      {
        send_to_char("&+yThe earth &+Ytrembles &+yfrom the marching of a nearby army.&n\r\n", i->character);
      }
      else if(num >= 19 &&
              num <= 22 &&
              dist <= 260 )
      {
        send_to_char("&+yRocks shake and the &+Learth&+y rumbles from the footsteps of an approaching &+Yparty.&n\r\n", i->character);
      }
      else if(num >= 14 &&
              num <= 18 &&
              dist <= 140 )
      {
        send_to_char("&+ySmall &+Lrocks&n jitter on the &+Lground &+yas an unknown &+Yentity &+yapproaches.&n\r\n", i->character);
      }
      else if(num >= 11 &&
              num <= 13 &&
              dist <= 64)
      {
       send_to_char("&nYou can hear the sound of &+ymovement &nin the distance.&n\r\n", i->character);
      }
*/
    }
  }
}

int do_simple_move(P_char ch, int exitnumb, unsigned int flags)
{
  struct affected_type *af;

  if( ch->in_room == NOWHERE)
    return 0;

  if( (exitnumb < 0) || (exitnumb >= NUM_EXITS))
    return FALSE;

  // Check for special routines
  if( special(ch, exitnumb_to_cmd(exitnumb), 0) )
    return FALSE;

  if( grease_check(ch))
    return FALSE;

  return do_simple_move_skipping_procs(ch, exitnumb, flags);
}


void make_ice(P_char ch)
{
  P_obj    ice, obj, next_obj;

  if( world[ch->in_room].contents)
    for (obj = world[ch->in_room].contents; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      if( obj->R_num == real_object(110))
        return;
    }

  ice = read_object(VOBJ_PATH_OF_FROST_ICE, VIRTUAL);
  if( !ice )
    return;

  set_obj_affected(ice, 400, TAG_OBJ_DECAY, 0);

  if( ch->in_room == NOWHERE)
  {
    if( real_room(ch->specials.was_in_room) != NOWHERE)
      obj_to_room(ice, real_room(ch->specials.was_in_room));
    else
    {
      extract_obj(ice, FALSE);
      ice = NULL;
    }
  }
  else
    obj_to_room(ice, ch->in_room);
}

/*
 * Please oh please, do not put your stuff here, do_simple_move_skipping_procs
 * is the most likely place you want to put your code in.
 * do_move is NOT called during fleeing and other kinds of movement 
 * not triggered with 'walking' commands ie, typing e, w, s, n, u etc.
 * while you most likely want to have your code called also then.
 */
void do_move(P_char ch, char *argument, int cmd)
{
  int      cmd2, i;

  cmd = cmd_to_exitnumb(cmd);

  if( affected_by_spell(ch, SPELL_DELIRIUM) && !number(0, 2))
  {
    cmd2 = number(1, 6);

    while (!CAN_GO(ch, cmd2) && i < 10)
    {
      i++;
      cmd2 = number(1, 6);
    }
    if( i < 9)
    {
      send_to_char("&+WYou are &+Gconfused&+W, watch out!&n\n", ch);
      cmd = cmd2;
    }
  }

  if( !IS_TRUSTED(ch) && !(GET_CLASS(ch, CLASS_DRUID)
    || (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID)))
    && get_spell_from_room(&world[ch->in_room], SPELL_WANDERING_WOODS) )
  {
    if( number(1, (int) ((GET_C_INT(ch) - 100) / 20) + 100) < 61)
    {
      send_to_char("You try to leave, but just end up going in circles!\n", ch);
      act("$n leaves one direction and enters from another!", FALSE, ch, 0, 0,
          TO_ROOM);
      return;
    }
  }

  if( IS_TRUSTED(ch) && IS_MAP_ROOM(ch->in_room) )
  {
    char buff[10];
    one_argument(argument, buff);
    int distance = atoi(buff);

    if( distance > 1 )
    {
      // distance was specified, so warp forward
      int to_room = ch->in_room;

      bfs_clear_marks();

      for (i = 0; i < distance && VALID_RADIAL_EDGE(to_room, cmd); i++)
      {
        to_room = TOROOM(to_room, cmd);
      }

      if( to_room != NOWHERE && to_room != ch->in_room )
      {
        act("$n's outline blurs and then streaks into the distance.", TRUE, ch, 0, 0, TO_ROOM);
        char_from_room(ch);
        char_to_room(ch, to_room, -1);
        act("A blur streaks into the room, coalescing into $n.", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }

  if( do_simple_move(ch, cmd, MVFLG_DRAG_FOLLOWERS))
  {
    if( affected_by_spell(ch, SPELL_PATH_OF_FROST))
    {
      make_ice(ch);
    }
  }
  else
  {
    REMOVE_BIT(ch->specials.affected_by3, AFF3_TRACKING);
  }
}

int find_door(P_char ch, char *type, char *dir)
{
  int      door;
  char     Gbuf1[MAX_STRING_LENGTH];

  if( ch->specials.z_cord > 0)
  {
    send_to_char("Open what?\n", ch);
    return (-1);
  }
  if( *dir)
  {                             /* a direction was specified */
    door = search_block(dir, dirs, FALSE);
    if( door = -1)
      door = search_block(dir, short_dirs, FALSE);
    if( door == -1)
    {
      send_to_char("That's not a direction.\n", ch);
      return (-1);
    }
    if( EXIT(ch, door) &&
        !IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) &&
        !IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED))
      if( EXIT(ch, door)->keyword)
        if( isname(type, EXIT(ch, door)->keyword))
          return (door);

        else
        {
          sprintf(Gbuf1, "I see no %s there.\n", type);
          send_to_char(Gbuf1, ch);
          return (-1);
        }
      else
        return (door);
    else
    {
      send_to_char("I really don't see how you can close anything there.\n",
                   ch);
      return (-1);
    }
  }
  else
  {
    /* try to locate the keyword */
    for (door = 0; door <= (NUM_EXITS - 1); door++)
      if( EXIT(ch, door) &&
          !IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) &&
          !IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED))
        if( EXIT(ch, door)->keyword)
          if( isname(type, EXIT(ch, door)->keyword))
            return (door);

  }

  /* Making it possible to find doors based on direction specified alone.  */
  /* We get here after checking for the door keyword handle, possible also */
  /* with a direction specified.                                           */
  
  door = search_block(type, dirs, FALSE);
  if( door == -1)
    door = search_block(type, short_dirs, FALSE);

  if( door != -1)
  {
	if( EXIT(ch, door) &&
      !IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) &&
      !IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED))
      return (door);
    else
        {
          send_to_char("I see nothing suitable there.\n", ch);
          return (-1);
        }
  }
  
  /* No keyword matching, no direction matching, nothing happens. */
  
  sprintf(Gbuf1, "I see no %s here.\n", type);
  send_to_char(Gbuf1, ch);
  return (-1);
}

void do_open(P_char ch, char *argument, int cmd)
{
  int      door, other_room, retval;
  struct room_direction_data *back;
  P_obj    obj;
  P_char   victim;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( IS_IMMOBILE(ch) )
  {
    act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if( IS_BLIND(ch) )
  {
    act("You can't see a thing let alone a door or whatever.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  argument_interpreter(argument, Gbuf2, Gbuf3);

  // Lowered this from 5/6 chance to 3/4 chance.
  if( IS_FIGHTING(ch) && number(0, 3))
  {
    send_to_char("That's tough to do in battle, but still you try...\n", ch);
    return;
  }
  if( !*Gbuf2 )
  {
    send_to_char("Open what?\n", ch);
    return;
  }

  if( IS_TRUSTED(ch) )
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj);
  }
  else
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &obj);
  }

  // This is an object
  if( retval != 0 )
  {
    if( (obj->type != ITEM_CONTAINER) && (obj->type != ITEM_STORAGE) && (obj->type != ITEM_QUIVER) )
    {
      send_to_char("That's not a container.\n", ch);
      return;
    }
    else if( !IS_SET(obj->value[1], CONT_CLOSED))
    {
      send_to_char("But it's already open!\n", ch);
      return;
    }
    else if( !IS_SET(obj->value[1], CONT_CLOSEABLE))
    {
      send_to_char("You can't do that.\n", ch);
      return;
    }
    else if( IS_SET(obj->value[1], CONT_LOCKED))
    {
      send_to_char("It seems to be locked.\n", ch);
      return;
    }
    else
    {
      // Yeah, misusing the door variable.. so sue me.
      door = obj_index[obj->R_num].virtual_number;
      // Special proc for bag of random goodness - drannak 4/3/2013 400232= turkey gut
      if( door == REG_FAERIE_BAG_VNUM || door == RARE_FAERIE_BAG_VNUM
        || door == EXCEPT_FAERIE_BAG_VNUM || door == TURKEY_INNARDS_VNUM )
      {
        if( (IS_CARRYING_N(ch) + 1) > CAN_CARRY_N(ch)) //check their inventory
        {
          send_to_char("You currently have too many items in your inventory to open a bag,\r\nput some items in a container then try again!\r\n", ch);
          return;
        }

        if( !OBJ_CARRIED(obj) )
        {
          send_to_char("You must have the &+Witem&n in your &+Yinventory&n to open it!\r\n", ch);
          return;
        }
        if(door == REG_FAERIE_BAG_VNUM || door == RARE_FAERIE_BAG_VNUM)
        {
          send_to_char("&+mAs you open the &+Mbag&+m, a magical mist &+rex&+Rpl&+Mod&+Wes&+m covering everything!\r\n", ch);
          act("&+mAs $n opens the &+Mbag&+m, a magical mist &+rex&+Rpl&+Mod&+Wes&+m covering everything!", FALSE, ch, obj, 0, TO_ROOM);
        }
        else if(door == EXCEPT_FAERIE_BAG_VNUM)
        {
          send_to_char("&+mAs you open the &+Ychest&+m, a magical mist &+rex&+Rpl&+Mod&+Wes&+m covering everything!\r\n", ch);
          act("&+mAs $n opens the &+Ychest&+m, a magical mist &+rex&+Rpl&+Mod&+Wes&+m covering everything!", FALSE, ch, obj, 0, TO_ROOM);
        }
        else //turkey gut
        {
          send_to_char("&+rYou thrust your &+yhand &+rdeep into the slimy &+Lgut&+r, covering everything in &+Rblood&+r!\r\n", ch);
          act("&+r$n&+r thrusts their &+yhand &+rdeep into the slimy &+Lgut&+r, covering everything in &+Rblood&+r!", FALSE, ch, obj, 0, TO_ROOM);
        }

        char buf[MAX_STRING_LENGTH];
        P_obj robj;
        long robjint;
        bool validobj;
        validobj = FALSE;
/*
debug sprintf(buf, "validobj value: %d\n\r", validobj);
send_to_char(buf, ch);
*/
        if( door == TURKEY_INNARDS_VNUM )
        {
          int roll = number(0, 500);
          if( roll < 5) //big reward
          {
            robj = read_object(TURKEY_WING_CAPE_VNUM, VIRTUAL);
          }
          else if( roll < 20)
          {
            robj = read_object(EXCEPT_FAERIE_BAG_VNUM, VIRTUAL);
          }
          else
          {
            robj = read_object(TURKEY_FOOD_VNUM, VIRTUAL);
          }
        }
        else
        {
          while(validobj == FALSE)
          {
 	          robjint = number(1300, 134000);
 	          robj = read_object(robjint, VIRTUAL);
            validobj = TRUE;
            if( !robj )
            {
              validobj = FALSE;
            }
            else if( !IS_SET(robj->wear_flags, ITEM_TAKE) || robj->type == ITEM_KEY || IS_ARTIFACT(robj)
              || IS_SET(robj->extra_flags, ITEM_NORENT) || IS_SET(robj->extra_flags, ITEM_NOSHOW)
              || IS_SET(robj->extra_flags, ITEM_TRANSIENT) || isname("_noquest_", robj->name) )
            {
              validobj = FALSE;
              extract_obj(robj, FALSE);
            }
            else if(door == REG_FAERIE_BAG_VNUM && itemvalue(robj) > 125)
            {
              validobj = FALSE;
              extract_obj(robj, FALSE);
            }
            else if(door == RARE_FAERIE_BAG_VNUM && itemvalue(robj) < 25)
            {
              validobj = FALSE;
              extract_obj(robj, FALSE);
            }
            else if(door == EXCEPT_FAERIE_BAG_VNUM && itemvalue(robj) < 50)
            {
              validobj = FALSE;
              extract_obj(robj, FALSE);
            }
          }
  	    }
        //Remove Curse, Secret, add Invis
        REMOVE_BIT(robj->extra_flags, ITEM_SECRET);
        REMOVE_BIT(robj->extra_flags, ITEM_NODROP);
        REMOVE_BIT(robj->extra_flags, ITEM_INVISIBLE);

        if( door == TURKEY_INNARDS_VNUM )
        {
          act("&+rAfter some &+Rmessy &+rsearching, you finally find &n$p&+m!\r\n", FALSE, ch, robj, 0, TO_CHAR);
          act("&+rAfter some &+Rmessy &+rsearching, $n finally finds &n$p&+m!\r\n", FALSE, ch, robj, 0, TO_ROOM);
        }
        else
        {
          act("&+mWhen at last it clears the &+Mbag&+m is gone, and all that remains is &n$p&+m!\r\n", FALSE, ch, robj, 0, TO_CHAR);
          act("&+mWhen at last it clears the &+Mbag&+m is gone, and all that remains is &n$p&+m!\r\n", FALSE, ch, robj, 0, TO_ROOM);
        }

        obj_to_char(robj, ch);
        obj_from_char(obj);
        extract_obj(obj, FALSE);
        if( itemvalue(robj) > 100 )
        {
          statuslog(ch->player.level, "&+MFaerie Bag:&n (%s&n) just got [%d] (%s&n) ival %d at [%d]!", GET_NAME(ch),
            obj_index[robj->R_num].virtual_number, robj->short_description, itemvalue(robj),
           (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);
        }
        return;
      }
    }
    REMOVE_BIT(obj->value[1], CONT_CLOSED);
    send_to_char("Ok.\n", ch);
    act("$n opens $p.", FALSE, ch, obj, 0, TO_ROOM);
    if( obj_index[obj->R_num].virtual_number == 1270)
    {
       treasure_chest(obj, ch, CMD_OPEN, argument);
    }

    // Trap check
    if( checkopen(ch, obj) )
    {
      return;
    }
  }
  // Perhaps it is a door
  else if( (door = find_door(ch, Gbuf2, Gbuf3)) >= 0)
  {
    if( !IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's impossible, I'm afraid.\n", ch);

    else if( !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("It's already open!\n", ch);
    else if( IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("It seems to be locked.\n", ch);
    else if( IS_SET(EXIT(ch, door)->exit_info, EX_SPIKED))
      send_to_char("It seems to be spiked into its current position.\n", ch);
    else
    {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_SECRET);
      if( EXIT(ch, door)->keyword)
        act("$n opens the $F.", FALSE, ch, 0, EXIT(ch, door)->keyword,
            TO_ROOM);
      else
        act("$n opens the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("Ok.\n", ch);
      /*
       * now for opening the OTHER side of the door!
       */
      if( (other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if( (back = world[other_room].dir_option[rev_dir[door]]))
          if( back->to_room == ch->in_room)
          {
            REMOVE_BIT(back->exit_info, EX_CLOSED);
            REMOVE_BIT(back->exit_info, EX_SECRET);
            if( back->keyword)
            {
              sprintf(Gbuf1,
                      "The %s is opened from the other side.\n",
                      FirstWord(back->keyword));
              send_to_room(Gbuf1, EXIT(ch, door)->to_room);
            }
            else
              send_to_room("The door is opened from the other side.\n",
                           EXIT(ch, door)->to_room);
          }
    }
  }
}

void do_close(P_char ch, char *argument, int cmd)
{
  int      door, other_room, retval;
  struct room_direction_data *back;
  P_obj    obj;
  P_char   victim;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_BLIND(ch))
  {
    act("You can't see a thing let alone a door or whatever.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  argument_interpreter(argument, Gbuf2, Gbuf3);

  if( !*Gbuf2 )
  {
    send_to_char("Close what?\n", ch);
    return;
  }
  if( IS_TRUSTED(ch) )
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj);
  }
  else
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &obj);
  }

  // This is an object
  if( retval != 0 )
    if( (obj->type != ITEM_CONTAINER) &&
        (obj->type != ITEM_STORAGE) && (obj->type != ITEM_QUIVER))
      send_to_char("That's not a container.\n", ch);
    else if( IS_SET(obj->value[1], CONT_CLOSED))
      send_to_char("But it's already closed!\n", ch);
    else if( !IS_SET(obj->value[1], CONT_CLOSEABLE))
      send_to_char("That's impossible.\n", ch);
    else
    {
      SET_BIT(obj->value[1], CONT_CLOSED);
      send_to_char("Ok.\n", ch);
      act("$n closes $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  else if( (door = find_door(ch, Gbuf2, Gbuf3)) >= 0)
    /*
     * Or a door
     */

    if( !IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\n", ch);
    else if( IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("It's already closed!\n", ch);
    else if( IS_SET(EXIT(ch, door)->exit_info, EX_SPIKED))
      send_to_char("It's spiked into position!\n", ch);
    else
    {
      SET_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      if( EXIT(ch, door)->keyword)
        act("$n closes the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
      else
        act("$n closes the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("Ok.\n", ch);
      /*
       * now for closing the other side, too
       */
      if( (other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if( (back = world[other_room].dir_option[rev_dir[door]]))
          if( back->to_room == ch->in_room)
          {
            SET_BIT(back->exit_info, EX_CLOSED);
            if( back->keyword)
            {
              sprintf(Gbuf1, "The %s closes quietly.\n",
                      FirstWord(back->keyword));
              send_to_room(Gbuf1, EXIT(ch, door)->to_room);
            }
            else
              send_to_room("The door closes quietly.\n",
                           EXIT(ch, door)->to_room);
          }
    }
}

P_obj has_key(P_char ch, int key)
{
  P_obj    o;

  if( ch->equipment[HOLD])
    if( obj_index[ch->equipment[HOLD]->R_num].virtual_number == key)
      return (ch->equipment[HOLD]);

  for (o = ch->carrying; o; o = o->next_content)
    if( obj_index[o->R_num].virtual_number == key)
      return (o);

  return NULL;
}
void do_lock(P_char ch, char *argument, int cmd)
{
  int      door, other_room, retval;
  struct room_direction_data *back;
  P_obj    obj, key_obj;
  P_char   victim;
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_BLIND(ch))
  {
    act("You can't see a thing let alone a door or whatever.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  argument_interpreter(argument, Gbuf2, Gbuf3);

  if( !*Gbuf2 )
  {
    send_to_char("Lock what?\n", ch);
    return;
  }

  if( IS_TRUSTED(ch) )
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj);
  }
  else
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &obj);
  }

  // This is an object
  if( retval != 0 )
    if( (obj->type != ITEM_CONTAINER) &&
        (obj->type != ITEM_STORAGE) && (obj->type != ITEM_QUIVER))
      send_to_char("That's not a container.\n", ch);
    else if( !IS_SET(obj->value[1], CONT_CLOSED))
      send_to_char("Maybe you should close it first...\n", ch);
    else if( obj->value[2] < 1)
      send_to_char("That thing can't be locked.\n", ch);
    else if( !(key_obj = has_key(ch, obj->value[2]))
      && (!IS_TRUSTED(ch) || IS_NPC(ch)) )
      send_to_char("You don't seem to have the proper key.\n", ch);
    else if( IS_SET(obj->value[1], CONT_LOCKED))
      send_to_char("It is locked already.\n", ch);
    else
    {
      SET_BIT(obj->value[1], CONT_LOCKED);
      send_to_char("*Click*\n", ch);
      act("$n locks $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  else if( (door = find_door(ch, Gbuf2, Gbuf3)) >= 0)
    /*
     * a door, perhaps
     */

    if( !IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\n", ch);

    else if( !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("You have to close it first, I'm afraid.\n", ch);
    else if( EXIT(ch, door)->key < 1)
      send_to_char("There does not seem to be any keyholes.\n", ch);
    else if( !has_key(ch, EXIT(ch, door)->key)
      && (!IS_TRUSTED(ch) || IS_NPC(ch)) )
      send_to_char("You don't have the proper key.\n", ch);
    else if( IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("It's already locked!\n", ch);
    else
    {
      SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if( EXIT(ch, door)->keyword)
        act("$n locks the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
      else
        act("$n locks the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("*Click*\n", ch);
      /*
       * now for locking the other side, too
       */
      if( (other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if( (back = world[other_room].dir_option[rev_dir[door]]))
          if( back->to_room == ch->in_room)
            SET_BIT(back->exit_info, EX_LOCKED);
    }
}

void do_unlock(P_char ch, char *argument, int cmd)
{
  int      door, other_room, retval;
  struct room_direction_data *back;
  P_obj    obj, key_obj = NULL;
  P_char   victim;
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_BLIND(ch))
  {
    act("You can't see a thing let alone a door or whatever.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  argument_interpreter(argument, Gbuf2, Gbuf3);

  if( !*Gbuf2 )
  {
    send_to_char("Unlock what?\n", ch);
    return;
  }

  if( IS_TRUSTED(ch) )
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj);
  }
  else
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &obj);
  }

  // This is an object
  if( retval != 0 )
    if( (obj->type != ITEM_CONTAINER) &&
        (obj->type != ITEM_STORAGE) && (obj->type != ITEM_QUIVER))
      send_to_char("That's not a container.\n", ch);
    else if( !IS_SET(obj->value[1], CONT_CLOSED))
      send_to_char("Silly - it ain't even closed!\n", ch);
    else if( obj->value[2] < 0)
    {
      send_to_char("Odd, you can't seem to find a keyhole.\n", ch);
      if( GET_LEVEL(ch) < MINLVLIMMORTAL || IS_NPC(ch))
        return;
      REMOVE_BIT(obj->value[1], CONT_LOCKED);
      send_to_char("...but you unlock it anyway!\n", ch);
      act("$n unlocks $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
    else if( (key_obj = has_key(ch, VOBJ_TEMPLATE_KEY)) &&
             (key_obj->value[7] == obj_index[obj->R_num].virtual_number))
    {
      REMOVE_BIT(obj->value[1], CONT_LOCKED);
      send_to_char("*Click*\n", ch);
      act("$n unlocks $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
    else if( !(key_obj = has_key(ch, obj->value[2])))
    {
      send_to_char("You don't seem to have the proper key.\n", ch);
      if( GET_LEVEL(ch) < MINLVLIMMORTAL || IS_NPC(ch))
        return;
      REMOVE_BIT(obj->value[1], CONT_LOCKED);
      send_to_char("...but you unlock it anyway!\n", ch);
      act("$n unlocks $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
    else if( !IS_SET(obj->value[1], CONT_LOCKED))
      send_to_char("Oh.. it wasn't locked, after all.\n", ch);
    else
    {
      REMOVE_BIT(obj->value[1], CONT_LOCKED);
      send_to_char("*Click*\n", ch);
      act("$n unlocks $p.", FALSE, ch, obj, 0, TO_ROOM);

      /*
       * check for key breaking
       */
      if( key_obj && key_obj->value[1] > 0)
        if( number(0, 99) < key_obj->value[1])
        {
          act("Damn!  You broke your key!", FALSE, ch, 0, 0, TO_CHAR);
          act("$n's key breaks off in the lock!", FALSE, ch, 0, 0, TO_ROOM);
          if( ch->equipment[HOLD] && (ch->equipment[HOLD] == key_obj))
            unequip_char(ch, HOLD);
          extract_obj(key_obj, TRUE); // Not that there are any artifact keys but ok.
          key_obj = NULL;
        }
    }
  else if( (door = find_door(ch, Gbuf2, Gbuf3)) >= 0)
  {

    /*
     * it is a door
     */

    if( !IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
    {
      send_to_char("Unlock what?\n", ch);
      return;
    }
    else if( !IS_TRUSTED(ch) &&
             (IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) ||
              IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED)))
    {
      send_to_char("Unlock what?\n", ch);
      return;
    }
    else if( !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
    {
      send_to_char("Heck.. it ain't even closed!\n", ch);
      return;
    }
    else if( !IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
    {
      send_to_char("It's already unlocked, it seems.\n", ch);
      return;
    }
    else if( EXIT(ch, door)->key < 0)
    {
      send_to_char("You can't seem to spot any keyholes.\n", ch);
      if( GET_LEVEL(ch) < MINLVLIMMORTAL)
        return;
      send_to_char("...but you unlock it anyway!\n", ch);
    }
    else if( !(key_obj = has_key(ch, EXIT(ch, door)->key)))
    {

      send_to_char("You do not have the proper key for that.\n", ch);
      if( GET_LEVEL(ch) < MINLVLIMMORTAL || IS_NPC(ch))
        return;
      send_to_char("...but you unlock it anyway!\n", ch);

    }
    REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
    /*
     * only happens if a god does it
     */
    if( IS_SET(EXIT(ch, door)->exit_info, EX_SECRET))
    {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_SECRET);
      act("$n reveals a secret door!", 0, ch, 0, 0, TO_ROOM);
    }
    if( EXIT(ch, door)->keyword)
      act("$n unlocks the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
    else
      act("$n unlocks the door.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("*click*\n", ch);

    /*
     * check for key breaking
     */
    if( key_obj && key_obj->value[1] > 0)
      if( number(0, 99) < key_obj->value[1])
      {
        act("Damn!  You broke your key!", FALSE, ch, 0, 0, TO_CHAR);
        act("$n's key breaks off in the lock!", FALSE, ch, 0, 0, TO_ROOM);
        if( ch->equipment[HOLD] && (ch->equipment[HOLD] == key_obj))
          unequip_char(ch, HOLD);
        extract_obj(key_obj, TRUE); // Not that there are any artifact keys but ok.
        key_obj = NULL;
      }
    /*
     * now for unlocking the other side, too
     */
    if( (other_room = EXIT(ch, door)->to_room) != NOWHERE)
      if( (back = world[other_room].dir_option[rev_dir[door]]))
        if( back->to_room == ch->in_room)
        {
          REMOVE_BIT(back->exit_info, EX_LOCKED);
          if( IS_SET(back->exit_info, EX_SECRET))
            REMOVE_BIT(back->exit_info, EX_SECRET);
        }
  }
}

void do_pick(P_char ch, char *argument, int cmd)
{
  int      percent, door, other_room, chance, retval;
  struct room_direction_data *back;
  P_obj    obj, pick;
  P_char   victim;
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_BLIND(ch))
  {
    act("You can't see a thing let alone a door or whatever.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if( !GET_CHAR_SKILL(ch, SKILL_PICK_LOCK))
  {
    send_to_char("You don't know how to pick locks...\n", ch);
    return;
  }
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("Uh huh, while fighting eh?  That would be a REALLY nice trick!\n", ch);
    return;
  }
  pick = ch->equipment[HOLD];

  if( !pick || (pick->type != ITEM_PICK) || !CAN_SEE_OBJ(ch, pick))
  {
    send_to_char("Using what?  Your teeth?\n", ch);
    return;
  }
  if( !affect_timer(ch, get_property("timer.secs.pickLock", 5), SKILL_PICK_LOCK))
  {
    send_to_char
      ("Your hands are still shaking from that last attempt, rest a bit.\n",
       ch);
    return;
  }
  argument_interpreter(argument, Gbuf2, Gbuf3);

  chance = GET_CHAR_SKILL(ch, SKILL_PICK_LOCK) + pick->value[0];
  percent = number(1, 100);

  if( !*Gbuf2 )
  {
    send_to_char("Pick what?\n", ch);
    return;
  }

  if( IS_TRUSTED(ch) )
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj);
  }
  else
  {
    retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &obj);
  }

  // This is an object
  if( retval != 0 )
    if( (obj->type != ITEM_CONTAINER) &&
        (obj->type != ITEM_STORAGE) && (obj->type != ITEM_QUIVER))
      send_to_char("That's not a container.\n", ch);
    else if( !IS_SET(obj->value[1], CONT_CLOSED))
      send_to_char("Silly - it ain't even closed!\n", ch);
    else if( obj->value[2] < 0)
      send_to_char("Odd - you can't seem to find a keyhole.\n", ch);
    else if( !IS_SET(obj->value[1], CONT_LOCKED))
      send_to_char("Oho! This thing is NOT locked!\n", ch);
/*
 * else if( ((percent - IS_SET (obj->value[1], CONT_HARDPICK) ? 30 : 0) > BOUNDED (0, chance, 100)) ||
 * IS_SET (obj->value[1], CONT_PICKPROOF)) {
 */
    else if( (percent > chance) || IS_SET(obj->value[1], CONT_PICKPROOF))
    {
      send_to_char("You failed to pick the lock.\n", ch);
      notch_skill(ch, SKILL_PICK_LOCK, 25);
      CharWait(ch, 8);
      percent =
        percent - chance + pick->value[1] - IS_SET(obj->value[1],
                                                   CONT_HARDPICK) ? 15 : 0;
      if( IS_SET(obj->value[1], CONT_PICKPROOF))
        percent -= 20;          /*
                                 * higher chance to break picks
                                 */
      if( (percent > -1) && (number(-1, percent) > 0))
      {
        act("Damn!  You broke your $p too!", FALSE, ch, pick, 0, TO_CHAR);
        act("$n begins cursing under $s breath as $s $p snaps.",
            FALSE, ch, pick, 0, TO_ROOM);
        if( ch->equipment[HOLD] && (ch->equipment[HOLD] == pick))
          unequip_char(ch, HOLD);
        extract_obj(pick, TRUE); // Not that there are any artifact lockpicks, but ok.
      }
      return;
    }
    else
    {
      REMOVE_BIT(obj->value[1], CONT_LOCKED);
      send_to_char("*Click*\n", ch);
      act("$n fiddles with $p.", FALSE, ch, obj, 0, TO_ROOM);
      CharWait(ch, 8);
    }
  else if( (door = find_door(ch, Gbuf2, Gbuf3)) >= 0)
    if( IS_RIDING(ch))
    {
      send_to_char("While mounted? I don't think so...\n", ch);
      return;
    }
    else if( !IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
    {
      send_to_char("That's absurd.\n", ch);
    }
    else if( !IS_TRUSTED(ch) &&
             (IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) ||
              IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED)))
    {
      send_to_char("Unlock what?\n", ch);
      return;
    }
    else if( !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("You realize that the door is already open.\n", ch);
    else if( EXIT(ch, door)->key < 0)
      send_to_char("You can't seem to spot any lock to pick.\n", ch);
    else if( !IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("Oh.. it wasn't locked at all.\n", ch);
    else if( (percent > BOUNDED(0, chance, 100)) ||
             IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF))
    {
      send_to_char("You failed to pick the lock.\n", ch);
      notch_skill(ch, SKILL_PICK_LOCK, 25);
      CharWait(ch, 8);
      percent = percent - chance + pick->value[1];
      if( IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF))
        percent -= 20;          /*
                                 * higher chance to break picks
                                 */
      if( (percent > -1) && (number(-1, percent) > 0))
      {
        act("Damn!  You broke your $p too!", FALSE, ch, pick, 0, TO_CHAR);
        act("$n begins cursing under $s breath as $s $p snaps.",
            FALSE, ch, pick, 0, TO_ROOM);
        if( ch->equipment[HOLD] && (ch->equipment[HOLD] == pick))
          unequip_char(ch, HOLD);
        extract_obj(pick, TRUE); // Not that there are any artifact lockpicks, but ok.
      }
      return;
    }
    else
    {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      /*
       * only happens if a god does it
       */
      if( IS_SET(EXIT(ch, door)->exit_info, EX_SECRET))
      {
        REMOVE_BIT(EXIT(ch, door)->exit_info, EX_SECRET);
        act("$n reveals a secret door!", 0, ch, 0, 0, TO_ROOM);
      }
      if( EXIT(ch, door)->keyword)
        act("$n skillfully picks the lock of the $F.", 0, ch, 0,
            EXIT(ch, door)->keyword, TO_ROOM);
      else
        act("$n picks the lock of the door.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("The lock quickly yields to your skills.\n", ch);
      CharWait(ch, 8);

      if( IS_SET(EXIT(ch, door)->exit_info, EX_PICKABLE))
        pick->value[1] += 20;   /*
                                 * higher wear on picks
                                 */
      /*
       * now for unlocking the other side, too
       */
      if( (other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if( (back = world[other_room].dir_option[rev_dir[door]]))
          if( back->to_room == ch->in_room)
          {
            REMOVE_BIT(back->exit_info, EX_LOCKED);
            if( IS_SET(back->exit_info, EX_SECRET))
              REMOVE_BIT(back->exit_info, EX_SECRET);
          }
    }
  else
    return;

  if( pick->value[1] > number(0, 99))
  {
    act("Damn!  But you broke your $p!", FALSE, ch, pick, 0, TO_CHAR);
    act("$n begins cursing under $s breath as $s $p snaps.",
        FALSE, ch, pick, 0, TO_ROOM);
    if( ch->equipment[HOLD] && (ch->equipment[HOLD] == pick))
      unequip_char(ch, HOLD);
    extract_obj(pick, TRUE); // Not that there are any artifact lockpicks, but ok.
  }
}

void do_enter(P_char ch, char *argument, int cmd)
{
  int      door;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_BLIND(ch))
  {
    act("You can't see a thing let alone a gateway or whatever.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  one_argument(argument, Gbuf1);

  if( IS_RIDING(ch))
  {
    send_to_char("While mounted? I don't think so...\n", ch);
    return;
  }
  if( *Gbuf1)
  {                             /*
                                 * an argument was supplied, search for
                                 * door keyword
                                 */
    for (door = 0; door <= (NUM_EXITS - 1); door++)
      if( EXIT(ch, door) &&
          !IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) &&
          !IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED))
        if( EXIT(ch, door)->keyword)
          if( !str_cmp(EXIT(ch, door)->keyword, Gbuf1) && dirs[door])
          {
            strcpy(Gbuf1, dirs[door]);
            // old guildhalls (deprecated)
//            if( IS_SET(world[ch->in_room].room_flags, ROOM_ATRIUM))
//            {
//              if( !House_can_enter(ch, world[ch->in_room].number, door))
//              {
//                send_to_char("You cannot enter this private house!\n", ch);
//                return;
//              }
//            }
            command_interpreter(ch, Gbuf1);
            return;
          }
    sprintf(Gbuf4, "There is no %s here.\n", Gbuf1);
    send_to_char(Gbuf4, ch);
  }
  else if( IS_SET(world[ch->in_room].room_flags, INDOORS))
    send_to_char("You are already indoors.\n", ch);
  else
  {
    /*
     * try to locate an entrance
     */
    for (door = 0; door <= (NUM_EXITS - 1); door++)
      if( EXIT(ch, door) &&
          !IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) &&
          !IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED))
        if( EXIT(ch, door)->to_room != NOWHERE)
          if( !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
              IS_SET(world[EXIT(ch, door)->to_room].room_flags,
                     INDOORS) && dirs[door])
          {
            // old guildhalls (deprecated
//            if( IS_SET(world[ch->in_room].room_flags, ROOM_ATRIUM))
//            {
//              if( !House_can_enter(ch, world[ch->in_room].number, -1))
//              {
//                send_to_char("You cannot enter this private house!\n", ch);
//                return;
//              }
//            }
            strcpy(Gbuf1, dirs[door]);
            command_interpreter(ch, Gbuf1);
            return;
          }
    send_to_char("You can't seem to find anything to enter.\n", ch);
  }
}

#if 0                           /*
                                 * * boggle
                                 */
void do_leave(P_char ch, char *argument, int cmd)
{
  int      door;
  char     Gbuf1[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( !IS_SET(world[ch->in_room].room_flags, INDOORS))
    send_to_char("You are outside.. where do you want to go?\n", ch);
  else
  {
    for (door = 0; door <= (NUM_EXITS - 1); door++)
      if( EXIT(ch, door) &&
          !IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) &&
          !IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED))
        if( EXIT(ch, door)->to_room != NOWHERE)
          if( !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
              !IS_SET(world[EXIT(ch, door)->to_room].room_flags, INDOORS) &&
              dirs[door])
          {
            strcpy(Gbuf1, dirs[door]);
            command_interpreter(ch, Gbuf1);
            return;
          }
    send_to_char("I see no obvious exits to the outside.\n", ch);
  }
}
#endif

void do_follow(P_char ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH];
  P_char leader = NULL;
  struct follow_type *j, *k;

  one_argument(argument, name);

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_BLIND(ch))
  {
    act("You can't see!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
// There is a function called stop_all_followers, but it does not
// distinguish between players and mobs. Follow stop only allows
// someone to stop the following players in the same room.
  if(!strcmp("stop", name))
  {
    act("You look around and quickly decide nobody is worthy enough to follow your greatness.", FALSE, ch, 0, 0, TO_CHAR);
   
    for (k = ch->followers; k; k = j)
    {
      j = k->next;
      
      if(IS_PC(k->follower) &&
        !IS_TRUSTED(k->follower) &&
        ch->in_room == k->follower->in_room)
      {
        act("$n no longer wants you to follow.", TRUE, ch, 0, k->follower, TO_VICT);
        stop_follower(k->follower);
      }
    }
    CharWait(ch, (int) (0.5 * PULSE_VIOLENCE));
    return;
  }

  if(*name)
  {
    if( !(leader = get_char_room_vis(ch, name)))
    {
      send_to_char("I see no person by that name here!\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("Who do you wish to follow?\n", ch);
    return;
  }

  if(IS_NPC(leader) &&
    !IS_TRUSTED(ch) &&
    (IS_PC(ch) || IS_MORPH(ch) || (ch->following && IS_PC(ch->following))))
  {
    send_to_char("Why would you follow a stupid mob?\n", ch);
    return;
  }

  if(IS_PC(leader) &&
    !IS_TRUSTED(ch) &&
    IS_PC(ch) &&
    !IS_DISGUISE(ch) &&
    racewar(leader, ch))
  {
    send_to_char("You wish it was that easy, don't you?\n", ch);
    return;
  }
  
  if(GET_MASTER(ch))
  {
    act("But you only feel like following $N!",
        FALSE, ch, 0, GET_MASTER(ch), TO_CHAR);
  }
  else
  {                             /*
                                 * Not Charmed follow person
                                 */
    if( leader == ch)
    {
      if( !ch->following)
      {
        send_to_char("You are already following yourself.\n", ch);
        return;
      }
      stop_follower(ch);
    }
    else
    {
      if( circle_follow(ch, leader))
      {
        act("Sorry, but following in 'loops' is not allowed",
            FALSE, ch, 0, 0, TO_CHAR);
        return;
      }
      if( ch->following)
      {
        stop_follower(ch);
      }
      add_follower(ch, leader);
    }
  }
}

void do_drag(P_char ch, char *argument, int cmd)
{
  P_obj    obj;
  P_char   tch, owner = NULL;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf4[MAX_STRING_LENGTH];
  int      dragCommand;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_BLIND(ch))
  {
    act("You can't see a thing let alone a corpse or whatever.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

   if( IS_RIDING(ch))
  {
    send_to_char("While mounted? I don't think so...\n", ch);
    return;
  }


  half_chop(argument, Gbuf1, Gbuf2);

  if(!*Gbuf1 || !*Gbuf2)
  {                             /*
                                 * No arguments
                                 */
    send_to_char("Drag what/who which direction?\n", ch);
    return;
  }
  
  if(ch->specials.z_cord > 0)
  {
    send_to_char("You can't drag something when you're flying.\n", ch);
    return;
  }
  
  if(!on_front_line(ch))
  {
    send_to_char
      ("Sorry, you're not close enough .. try moving to the front battle line.\n",
       ch);
    return;
  }

  one_argument(Gbuf2, Gbuf4);
  dragCommand = old_search_block(Gbuf4, 0, strlen(Gbuf4), command, 2);
  
  if(!IS_TRUSTED(ch))
  {
    switch (dragCommand)
    {
    case CMD_DRAG:
      send_to_char("You can only grip one at a time!\n", ch);
      return;
    case CMD_NORTH:
    case CMD_EAST:
    case CMD_SOUTH:
    case CMD_WEST:
    case CMD_UP:
    case CMD_DOWN:
    case CMD_NORTHWEST:
    case CMD_SOUTHWEST:
    case CMD_NORTHEAST:
    case CMD_SOUTHEAST:
    case CMD_NW:
    case CMD_SW:
    case CMD_NE:
    case CMD_SE:
    case CMD_ENTER:
    case CMD_DISEMBARK:
      break;
    default:
      send_to_char("You cannot drag anything that way.\n", ch);
      return;
    }
  }

  /*
   * First, we'll check if its a mob/player...
   */

  if((tch = get_char_room_vis(ch, Gbuf1)))
  {

    if(ch == tch)
    {
      send_to_char("That would be quite a feat, my friend.\n", ch);
      return;
    }
    
    if((GET_STAT(tch) != STAT_INCAP) && (GET_STAT(tch) != STAT_DYING)
       /* && !(is_linked_to(ch, tch, LNK_CONSENT) &&
          (GET_STAT(tch) == STAT_SLEEPING)) && */
        /*!IS_AFFECTED(tch, AFF_KNOCKED_OUT) 
         &&  !IS_AFFECTED(tch, AFF_BOUND)) */ )
    {
      send_to_char("They may not appreciate that.\n", ch);
      return;
    }

    if((IS_NPC(tch) || (GET_WEIGHT(tch) + IS_CARRYING_W(tch)) >
      (MAX_DRAG * CAN_CARRY_W(ch) - IS_CARRYING_W(ch))) && !IS_TRUSTED(ch))
    {
      act("$E's too heavy for you to drag!", FALSE, ch, 0, tch, TO_CHAR);
      act("$n tries to drag $N out, but after a series of grunts, gives up.",
          TRUE, ch, 0, tch, TO_ROOM);
      return;
    }
/*    if( ch->points.delay_move + move_cost(ch, cmd) > 10) */
    if(((GET_VITALITY(ch) - DRAG_COST) < 0) &&
      !IS_TRUSTED(ch))
    {
      act("$n tries to drag $N out of the room, but is too tired!",
          TRUE, ch, 0, tch, TO_ROOM);
      act("You try to drag $N out of the room, but you are too tired!",
          FALSE, ch, 0, tch, TO_CHAR);
      return;
    }
    if((tch->specials.arrest_by != NULL) &&
      IS_AFFECTED(tch, AFF_BOUND) &&
        IS_NPC(ch))
    {
      if( CHAR_IN_TOWN(ch))
        if( GET_CRIME_T(CHAR_IN_TOWN(ch), CRIME_CORPSE_DRAG))
          return;
    }
    act("$n tries to drag $N out of the room.", TRUE, ch, 0, tch, TO_ROOM);
    if(!IS_TRUSTED(ch))
    {
      GET_VITALITY(ch) -= DRAG_COST;    /* moves will be the tired factor */
      CharWait(ch, PULSE_VIOLENCE);
    }
    command_interpreter(ch, Gbuf2);     /* move first player */
    if(ch->in_room != NOWHERE)
    {
      if(CHAR_IN_ARENA(ch) != CHAR_IN_ARENA(tch))
      {
        act("As you drag $N, the arena holds $S body back!", TRUE, ch, 0, tch,
            TO_CHAR);
        act("$n attempts to drag $N, but the arena holds $S body back!", TRUE,
            ch, 0, tch, TO_ROOM);
        act("$n attempts to drag you, but the arena holds your body back!",
            TRUE, ch, 0, tch, TO_VICT);

        return;
      }
      if(IS_SET(world[ch->in_room].room_flags, LOCKER))
      {
        act("The locker door slams shut before you can drag $N behind you.",
            TRUE, ch, 0, tch, TO_CHAR);
        act("A locker door slams shut before $N can be dragged in.", TRUE, ch,
            0, tch, TO_ROOM);
        act
          ("$n attempts to drag you, but the locker door slams shut too soon.",
           TRUE, ch, 0, tch, TO_VICT);
        act("&+MOUCH!&n  That really did &+MHURT!&n", TRUE, ch, 0, tch,
            TO_VICT);
        return;
      }


      if((GET_LEVEL(ch) > 55) &&
        IS_PC(ch))
        logit(LOG_WIZ, "%s dragged %s from [%d]", GET_NAME(ch), GET_NAME(tch),
              world[ch->in_room].number);

      char_from_room(tch);      /* move dragee */
      
      if( char_to_room(tch, ch->in_room, 0))
      {
        act("$n drags $N along behind $m.", TRUE, ch, 0, tch, TO_ROOM);
        act("You drag $N along behind you.", TRUE, ch, 0, tch, TO_CHAR);
        act("$n drags you along behind $m.", TRUE, ch, 0, tch, TO_VICT);

        if( (tch->specials.arrest_by != NULL) && IS_AFFECTED(tch, AFF_BOUND)
            && !IS_NPC(ch))
        {
          if( CHAR_IN_TOWN(ch))
            if( GET_CRIME_T(CHAR_IN_TOWN(ch), CRIME_CORPSE_DRAG))
              justice_witness(ch, NULL, CRIME_AGAINST_TOWN);
        }
      }
      return;
    }
    else
    {
      act("You can't drag $M out of the room!", TRUE, ch, 0, tch, TO_CHAR);
      return;
    }
    send_to_char("Error in do_drag(tch), please notify a forger.\n", ch);
    return;
  }
  else
    if( (obj = get_obj_in_list_vis(ch, Gbuf1, world[ch->in_room].contents)))
  {

    if( !str_cmp(Gbuf1, "all") || strstr(Gbuf1, "all."))
    {
      send_to_char("You can only grip one at a time!\n", ch);
      return;
    }
    /* Now, the code for objects... */

    /*
     * Player corpses are no-take to prevent corruption of player save
     * files when the game dies and a player has a corpse in their
     * inventory. Player corpses are v-num 2. The check below is to
     * allow players to drag any object that doesn't have the no-take
     * flag set and isn't a pcorpse.
     */

    if( !IS_SET(obj->wear_flags, ITEM_TAKE) && (obj->type != ITEM_CORPSE))
    {
      send_to_char("You can't drag that!\n", ch);
      return;
    }
    if( obj->type == ITEM_MONEY)
    {
      send_to_char
        ("Dragging a pile of coins is quite stupid. Try using a bag.\n", ch);
      return;
    }
    /* Let players drag an object up to 150 % of their max_carry */
    if( (GET_OBJ_WEIGHT(obj) >
         (MAX_DRAG * CAN_CARRY_W(ch) - IS_CARRYING_W(ch))) && !IS_TRUSTED(ch))
    {
      act("It's too heavy for you to drag!", FALSE, ch, 0, 0, TO_CHAR);
      act("$n tries to drag out $p, but after a series of grunts, gives up.",
          TRUE, ch, obj, 0, TO_ROOM);
      return;
    }
/*    if( ch->points.delay_move + move_cost(ch, cmd) > 10)  */
    if( ((GET_VITALITY(ch) - DRAG_COST) < 0) && !IS_TRUSTED(ch))
    {
      act("$n tries to drag $p out of the room, but is too tired!",
          TRUE, ch, obj, 0, TO_ROOM);
      act("You try to drag $p out of the room, but you are too tired!",
          FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    act("$n tries to drag $p out of the room.", TRUE, ch, obj, 0, TO_ROOM);

    if( !IS_TRUSTED(ch))
    {
      GET_VITALITY(ch) -= DRAG_COST;    /* moves will be the tired factor */
      CharWait(ch, PULSE_VIOLENCE);
    }

    /*
     * by using half_chop, they can type 'drag corpse enter portal',
     * and they will drag the corpse through the portal, much more
     * flexible. JAB
     */
    command_interpreter(ch, Gbuf2);

    if( ch->in_room != NOWHERE)
    {

      if(IS_SET(world[ch->in_room].room_flags, LOCKER))
      {
        act("The locker door slams shut on your hand before you can pull $p in.",
            TRUE, ch, obj, 0, TO_CHAR);
        act("A locker door slams shut on $N's hand.", TRUE, ch,
            obj, 0, TO_ROOM);
        return;
      }
    }

    /*
     * If player moved, drag obj
     */
    if( !OBJ_ROOM(obj))
    {
      send_to_char("That object is no longer in the room!\n", ch);
      return;
    }

    if( (ch->in_room != NOWHERE) && (ch->in_room != obj->loc.room))
    {

      /*
       * Assume that the player was able to move, and therefore,
       * object should be dragged.
       */

      obj_from_room(obj);
      obj_to_room(obj, ch->in_room);
      act("$n drags $p along behind $m.", TRUE, ch, obj, 0, TO_ROOM);
      act("You drag $p along behind you.", TRUE, ch, obj, 0, TO_CHAR);

      if( (GET_LEVEL(ch) > 55) && IS_PC(ch))
        logit(LOG_WIZ, "%s dragged by %s into [%d]", obj->short_description,
              GET_NAME(ch), world[ch->in_room].number);


      if( (obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], PC_CORPSE))
      {
        logit(LOG_CORPSE, "%s dragged by %s into [%d].",
              obj->short_description,
              (IS_PC(ch) ? GET_NAME(ch) : ch->player.short_descr),
              world[ch->in_room].number);

        strcpy(Gbuf4, obj->action_description);
        *Gbuf4 = tolower(*Gbuf4);
        owner = get_char(Gbuf4);
        if( obj->value[4] == 1)
        {
          if( owner)
          {
            if( !is_linked_to(ch, owner, LNK_CONSENT) && (owner != ch))
            {
              if( CHAR_IN_TOWN(ch))
              {
                if( GET_CRIME_T(CHAR_IN_TOWN(ch), CRIME_CORPSE_DRAG))
                {
                  justice_witness(ch, NULL, CRIME_CORPSE_DRAG);
                }
              }
            }
          }
          else
          {
            if( CHAR_IN_TOWN(ch))
            {
              if( GET_CRIME_T(CHAR_IN_TOWN(ch), CRIME_CORPSE_DRAG))
              {
                justice_witness(ch, NULL, CRIME_CORPSE_DRAG);
              }
            }
          }
        }
      }
      return;
    }
    else
    {
      act("You can't drag it out of the room!", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
    send_to_char("Error in do_drag(obj), please notify a forger.\n", ch);
    return;
  }
  send_to_char("What/who do you want to drag?\n", ch);
}

void do_stand(P_char ch, char *argument, int cmd)
{

  if(!ch)
  {
    logit(LOG_EXIT, "do_stand called in actmove.c with no ch");
    raise(SIGSEGV);
  }

  if( !IS_ALIVE(ch) )
  {
    send_to_char("&+rYou attempt to stand, but you are dead so your body doesn't respond.\n\r", ch);
    return;
  }

  if( GET_POS(ch) == POS_STANDING )
  {
    act("You are already standing.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  // Why is this here? - We might want to call do_stand from say, the monk auto-stand skill while they're bashed.
  if( !CAN_ACT(ch) )
  {
    return;
  }

  switch( GET_STAT(ch) )
  {
    case STAT_DEAD:
    case STAT_DYING:
    case STAT_INCAP:
    {
      send_to_char("Posture would seem to be far down on your list of problems right now!\n", ch);
      return;
    }
    case STAT_SLEEPING:
    {
      send_to_char("You dream of sleepwalking.\n", ch);
      return;
    }
    case STAT_RESTING:
    case STAT_NORMAL:
      if(IS_AFFECTED(ch, AFF_KNOCKED_OUT))
      {
        send_to_char("You should probably worry more about becoming conscious again.\n", ch);
        return;
      }
      if(IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) || IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less stand up!\n", ch);
        return;
      }
      if(IS_AFFECTED(ch, AFF_BOUND))
      {
        send_to_char("Try to unbind yourself first!\n", ch);
        return;
      }
      if( IS_AFFECTED2(ch, AFF2_STUNNED) )
      {
        // Maaaybeee
        if(number(1, (GET_C_AGI(ch) + GET_C_DEX(ch))) < number(70, 140))
        {
          send_to_char("You stagger about, then fall to your knees!\n", ch);
          SET_POS(ch, GET_STAT(ch) + POS_PRONE);
          return;
        }
        else if(number(1, (GET_C_AGI(ch) + GET_C_DEX(ch))) < number(70, 201))
        {
          send_to_char("You stagger about, then fall to your knees!\n", ch);
          SET_POS(ch, GET_STAT(ch) + POS_KNEELING);
          return;
        }
        else
        {
          send_to_char("You manage to unsteadily get to your feet.\n", ch);
          act("$n staggers about, but manages to get to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
        }
      }
      else
      {
        switch( GET_POS(ch) )
        {
          case POS_PRONE:
            act("You clamber to your feet.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case POS_KNEELING:
            act("You rise to your feet.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n rises to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case POS_SITTING:
            act("You clamber to your feet.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case POS_STANDING:
            act("You are already standing.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        break;
      }
      break;
  }

  // Returns TRUE if ch dies in the process.
  if( check_crippling_strike( ch ) )
    return;

  SET_POS(ch, POS_STANDING + STAT_NORMAL);
  stop_memorizing(ch);
}

// end do_stand

void do_sit(P_char ch, char *argument, int cmd)
{
  switch (GET_STAT(ch))
  {
  case STAT_DEAD:
  case STAT_DYING:
  case STAT_INCAP:
    send_to_char
      ("Posture would seem to be far down on your list of problems right now!\n",
       ch);
    return;
    break;
  case STAT_SLEEPING:
    send_to_char("You dream of sitting up.\n", ch);
    return;
    break;
  case STAT_RESTING:
  case STAT_NORMAL:
    if( IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    {
      send_to_char
        ("You should probably worry more about becoming conscious again.\n",
         ch);
      return;
    }
    switch (GET_POS(ch))
    {
    case POS_PRONE:
      if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
          IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less sit up!\n", ch);
        return;
      }
      act("You sit up.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sits up.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_KNEELING:
      if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
          IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less sit!\n", ch);
        return;
      }
      act("You get off your knees and sit.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n drops back off $s knees and sits.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_SITTING:
      send_to_char("You are sitting already.\n", ch);
      return;
      break;
    case POS_STANDING:
      if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
          IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less sit down!\n", ch);
        return;
      }
      act("You sit down.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sits down.", TRUE, ch, 0, 0, TO_ROOM);

      if( IS_FIGHTING(ch))
      {
        /*
         * they can, if they want, but...  JAB
         */
        if( ch->specials.fighting->specials.fighting &&
            (ch->specials.fighting->specials.fighting != ch) &&
            IS_NPC(ch->specials.fighting) && CAN_ACT(ch->specials.fighting) &&
            CAN_SEE(ch->specials.fighting, ch))
        {
          SET_POS(ch, GET_STAT(ch) + POS_SITTING);
          attack(ch->specials.fighting, ch);    /*
                                                 * ie: switch
                                                 */
          return;
        }
      }
      break;
    }
    break;
  }
  SET_POS(ch, GET_STAT(ch) + POS_SITTING);
}

void do_kneel(P_char ch, char *argument, int cmd)
{
  if(IS_RIDING(ch))
  {
    send_to_char("You can't kneel while riding.", ch);
    return;
  }
  switch (GET_STAT(ch))
  {
  case STAT_DEAD:
  case STAT_DYING:
  case STAT_INCAP:
    send_to_char
      ("Posture would seem to be far down on your list of problems right now!\n",
       ch);
    return;
    break;
  case STAT_SLEEPING:
    send_to_char("You seem to be having a dream of infancy.\n", ch);
    return;
    break;
  case STAT_RESTING:
  case STAT_NORMAL:
    if( IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    {
      send_to_char
        ("You should probably worry more about becoming conscious again.\n",
         ch);
      return;
    }
    switch (GET_POS(ch))
    {
    case POS_PRONE:
      if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
          IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less get to your knees!\n",
                     ch);
        return;
      }
      act("You swing up onto your knees.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n levers up onto $s knees.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_KNEELING:
      send_to_char("You are kneeling already.\n", ch);
      return;
      break;
    case POS_SITTING:
      if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
          IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less change position!\n",
                     ch);
        return;
      }
      act("You move from your butt to your knees.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops sitting around and gets to $s knees.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case POS_STANDING:
      if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
          IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less kneel!\n", ch);
        return;
      }
      act("You kneel.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n settles to $s knees.", TRUE, ch, 0, 0, TO_ROOM);

      if( IS_FIGHTING(ch))
      {
        /*
         * they can, if they want, but...  JAB
         */
        if( ch->specials.fighting->specials.fighting &&
            (ch->specials.fighting->specials.fighting != ch) &&
            IS_NPC(ch->specials.fighting) && CAN_ACT(ch->specials.fighting) &&
            CAN_SEE(ch->specials.fighting, ch))
        {
          SET_POS(ch, GET_STAT(ch) + POS_KNEELING);
          attack(ch->specials.fighting, ch);    /*
                                                 * ie: switch
                                                 */
          return;
        }
      }
      break;
    }
    break;
  }
  SET_POS(ch, GET_STAT(ch) + POS_KNEELING);
}

void do_recline(P_char ch, char *argument, int cmd)
{
  if(IS_RIDING(ch))
  {
    send_to_char("How exactly are you going to recline while riding?\n\r", ch);
    return;
  }
  switch (GET_STAT(ch))
  {
  case STAT_DEAD:
  case STAT_DYING:
  case STAT_INCAP:
    send_to_char
      ("Posture would seem to be far down on your list of problems right now!\n",
       ch);
    return;
    break;
  case STAT_SLEEPING:
    send_to_char("You dream of laying down.\n", ch);
    return;
    break;
  case STAT_RESTING:
  case STAT_NORMAL:
    if( IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    {
      send_to_char
        ("You should probably worry more about becoming conscious again.\n",
         ch);
      return;
    }
    switch (GET_POS(ch))
    {
    case POS_PRONE:
      send_to_char("You are already laying down.\n", ch);
      return;
      break;
    case POS_KNEELING:
      if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
          IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less lay down!\n", ch);
        return;
      }
      act("You recline.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n eases off $s knees and lays down.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_SITTING:
      if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
          IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less change position!\n",
                     ch);
        return;
      }
      act("You stop sitting around and lay down.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops sitting around and lays down.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_STANDING:
      if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
          IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
      {
        send_to_char("You can't even twitch, much less change position!\n",
                     ch);
        return;
      }
      act("You drop to your belly.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n drops flat to the ground.", TRUE, ch, 0, 0, TO_ROOM);

      if( IS_FIGHTING(ch))
      {
        /*
         * they can, if they want, but...  JAB
         */
        if( ch->specials.fighting->specials.fighting &&
            (ch->specials.fighting->specials.fighting != ch) &&
            IS_NPC(ch->specials.fighting) && CAN_ACT(ch->specials.fighting) &&
            CAN_SEE(ch->specials.fighting, ch))
        {
          SET_POS(ch, GET_STAT(ch) + POS_PRONE);
          stop_memorizing(ch);
          attack(ch->specials.fighting, ch);    /*
                                                 * ie: switch
                                                 */
          return;
        }
      }
      break;
    }
    break;
  }
  SET_POS(ch, GET_STAT(ch) + POS_PRONE);
  stop_memorizing(ch);
}

void do_rest(P_char ch, char *argument, int cmd)
{
/*
  if(IS_RIDING(ch))
  {
    send_to_char("It's too difficult to rest while riding someone.", ch);
    return;
  }
*/

  switch (GET_STAT(ch))
  {
  case STAT_RESTING:
    send_to_char("You are already resting.\n", ch);
    return;
    break;
  case STAT_SLEEPING:
    send_to_char("You dream of relaxing.\n", ch);
    return;
    break;
  case STAT_DEAD:
  case STAT_DYING:
  case STAT_INCAP:
    send_to_char("Just wait a bit, you'll soon be VERY relaxed.\n", ch);
    return;
    break;
  case STAT_NORMAL:
    if( IS_FIGHTING(ch) || NumAttackers(ch) )
    {
      send_to_char("Resting now will most likely lead to your final rest!\n",
                   ch);
      return;
    }
    if( IS_DESTROYING(ch) )
    {
      send_to_char("You try, but you can't focus while destroying.\n", ch);
      return;
    }
    if( IS_AFFECTED(ch, AFF_BOUND))
    {
      send_to_char
        ("Your bonds prevent you from really getting comfortable.\n", ch);
      return;
    }
    if( IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    {
      send_to_char
        ("You should probably worry more about becoming conscious again.\n",
         ch);
      return;
    }
    if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
        IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
    {
      send_to_char("You can't even twitch, much less relax!\n", ch);
      return;
    }
    switch (GET_POS(ch))
    {
    case POS_PRONE:
      send_to_char("You close your eyes and relax.\n", ch);
      act("You see some of the tension leave $n's body.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case POS_KNEELING:
      send_to_char("You slump and relax your posture.\n", ch);
      act("$n relaxes a bit.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_SITTING:
      send_to_char("You wiggle to find the most comfortable position.\n", ch);
      act("$n relaxes a bit.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_STANDING:
      send_to_char("You sit down and relax.\n", ch);
      act("$n sits down in a comfortable spot.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    }
    break;
  }
  SET_POS(ch, MIN(POS_SITTING, GET_POS(ch)) + STAT_RESTING);
  if( (GET_POS(ch) != POS_SITTING) && (GET_POS(ch) != POS_KNEELING))
    stop_memorizing(ch);
  StartRegen(ch, EVENT_HIT_REGEN);
  StartRegen(ch, EVENT_MOVE_REGEN);
  StartRegen(ch, EVENT_MANA_REGEN);
}

/*
 * new, does opposite of 'rest'
 */

void do_alert(P_char ch, char *argument, int cmd)
{
  switch (GET_STAT(ch))
  {
  case STAT_DEAD:
  case STAT_DYING:
  case STAT_INCAP:
    send_to_char("Just wait a bit, you'll soon be VERY relaxed.\n", ch);
    return;
    break;
  case STAT_SLEEPING:
    send_to_char("You dream of being alert.\n", ch);
    return;
    break;
  case STAT_NORMAL:
    send_to_char("You are already about as tense as you can get.\n", ch);
    return;
    break;
  case STAT_RESTING:
    if( IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    {
      send_to_char("Your brain is on strike, please try again later.\n", ch);
      return;
    }
    if( IS_STUNNED(ch))
    {
      send_to_char
        ("Your head is spinning, you can barely tell which way is up!\n", ch);
      return;
    }
    if( IS_FIGHTING(ch) || NumAttackers(ch))
      send_to_char("Excellent Idea!  You might actually survive!\n", ch);

    switch (GET_POS(ch))
    {
    case POS_PRONE:
      send_to_char
        ("You stop relaxing and try to become more aware of your surroundings.\n",
         ch);
      break;
    case POS_KNEELING:
      send_to_char("You straighten up a bit.\n", ch);
      act("$n straightens up a bit.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_SITTING:
      send_to_char("You sit up straight and start to pay attention.\n", ch);
      act("$n sits at attention.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_STANDING:
      send_to_char("You tense up and become more alert.\n", ch);
      break;
    }
    break;
  }
  SET_POS(ch, GET_POS(ch) + STAT_NORMAL);
  stop_memorizing(ch);
}

void do_sleep(P_char ch, char *argument, int cmd)
{
  if(IS_RIDING(ch))
  {
    send_to_char("Sleep while riding?  I don't think so.", ch);
    return;
  }
  if( IS_FIGHTING(ch) || NumAttackers(ch))
  {
    send_to_char("Sleep while fighting?  Are you MAD?\n", ch);
    return;
  }
  if( IS_DESTROYING( ch ) )
  {
    send_to_char("Sleep while destroying an object?\n", ch);
    return;
  }
  if( world[ch->in_room].sector_type >= SECT_WATER_SWIM &&
      world[ch->in_room].sector_type >= SECT_UNDRWLD_WATER &&
      world[ch->in_room].sector_type <= SECT_OCEAN)
  {
    act("Here? That's not too wise.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  switch (GET_STAT(ch))
  {
  case STAT_SLEEPING:
    send_to_char("You are already fast asleep.\n", ch);
    return;
    break;
  case STAT_DEAD:
  case STAT_DYING:
  case STAT_INCAP:
    send_to_char("Just wait a bit, you're about to start a LONG sleep.\n",
                 ch);
    return;
    break;
  case STAT_RESTING:
  case STAT_NORMAL:
    if( IS_AFFECTED(ch, AFF_BOUND))
    {
      send_to_char
        ("Your bonds prevent you from really getting comfortable.\n", ch);
      return;
    }
    if( IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    {
      send_to_char
        ("You should probably worry more about becoming conscious again.\n",
         ch);
      return;
    }
    if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
        IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
    {
      send_to_char("You can't even twitch, much less relax!\n", ch);
      return;
    }
    switch (GET_POS(ch))
    {
    case POS_PRONE:
      send_to_char("You drift off to sleep.\n", ch);
      break;
    case POS_KNEELING:
      send_to_char("You slump and relax your posture.\n", ch);
      act("$n seems to have fallen asleep.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_SITTING:
      send_to_char("You nod off.\n", ch);
      act("$n seems to have fallen asleep.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case POS_STANDING:
      send_to_char("You fall asleep.\n", ch);
      act("$n has fallen asleep on $s feet, this should be fun.", TRUE, ch, 0,
          0, TO_ROOM);
      break;
    }
    break;
  }

  SET_POS(ch, GET_POS(ch) + STAT_SLEEPING);
  stop_memorizing(ch);
}

void do_wake(P_char ch, char *argument, int cmd)
{
  P_char   tmp_char;
  char     Gbuf1[MAX_STRING_LENGTH];

  one_argument(argument, Gbuf1);

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if(!IS_ALIVE(ch))
  {
    return;
  }

  if(IS_IMMOBILE(ch))
  {
    act("Relax... sleep a while longer!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if( *Gbuf1)
  {
    if( GET_STAT(ch) == STAT_SLEEPING)
    {
      act("You can't wake people up if you are asleep yourself!",
          FALSE, ch, 0, 0, TO_CHAR);
    }
    else
    {
      tmp_char = get_char_room_vis(ch, Gbuf1);
      if( tmp_char)
      {
        if( tmp_char == ch)
        {
          act("If you want to wake yourself up, just type 'wake'",
              FALSE, ch, 0, 0, TO_CHAR);
        }
        else
        {
          if( GET_STAT(tmp_char) == STAT_SLEEPING)
          {
            if( !IS_TRUSTED(ch) && IS_AFFECTED4(tmp_char, AFF4_TUPOR))
            {
              act
                ("You try to wake $M up, but $E is too deep into the trance!",
                 FALSE, ch, 0, tmp_char, TO_CHAR);
              act
                ("$n tries to awaken $N, but $E is too deep into the trance.",
                 FALSE, ch, 0, tmp_char, TO_NOTVICT);
              return;
            }
            /*
               if( !IS_TRUSTED(ch) && IS_AFFECTED2(tmp_char, AFF2_MEMORIZING)) {
               act("You try to wake $M up, but $E is too deep into the trance!",
               FALSE, ch, 0, tmp_char, TO_CHAR);
               act("$n tries to awaken $N, but $E is too deep into the trance.",
               FALSE, ch, 0, tmp_char, TO_NOTVICT);
               return;
               }
             */
            if( !IS_TRUSTED(ch) && IS_AFFECTED(tmp_char, AFF_SLEEP) ||
                IS_AFFECTED(tmp_char, AFF_KNOCKED_OUT))
            {
              act("You try to wake $M up, but $E does not respond!",
                  FALSE, ch, 0, tmp_char, TO_CHAR);
              act("$n tries to awaken $N, but $E continues sawing logs.",
                  FALSE, ch, 0, tmp_char, TO_NOTVICT);
              return;
            }
            act("You wake $M up.", FALSE, ch, 0, tmp_char, TO_CHAR);
            SET_POS(tmp_char, GET_POS(tmp_char) + STAT_RESTING);
            act("You are awakened by $n.", FALSE, ch, 0, tmp_char, TO_VICT);
          }
          else
          {
            act("$N is already awake.", FALSE, ch, 0, tmp_char, TO_CHAR);
          }
        }
      }
      else
      {
        send_to_char("You do not see that person here.\n", ch);
      }
    }
  }
  else
  {
    if( IS_AFFECTED(ch, AFF_SLEEP) || IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    {
      send_to_char("You can't wake up!\n", ch);
    }
    else
    {
      if( GET_STAT(ch) != STAT_SLEEPING)
        send_to_char("You are already awake...\n", ch);
      else
      {
        send_to_char("You wake up.\n", ch);
        act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
        
        if(IS_SET(ch->specials.affected_by4, AFF4_TUPOR))
          REMOVE_BIT(ch->specials.affected_by4, AFF4_TUPOR);

        if( USES_TUPOR(ch) && IS_AFFECTED2(ch, AFF2_MEMORIZING) )
        {
          stop_memorizing(ch);
        }

        SET_POS(ch, GET_POS(ch) + STAT_RESTING);
      }
    }
  }
}
