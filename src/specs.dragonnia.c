
/*
   ***************************************************************************
   *  File: specs.dragonnia.c                                  Part of Duris *
   *  Usage: mobile special procedures for Dragonnia                           *
   *  Copyright  1992 - Edoardo Keyvan Perez Izadi - all rights reserved       *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   *************************************************************************** 
 */

/***************************************************************************
 *  By  :   Edoardo Keyvan Perez Izadi                                     *
 *        (ISE) (Electronic Systems Engineer)                              *
 *                                                                         *
 *  Dedicated to Ilia , Pooka, Khisanth     For being good friends. =)     *
 *        Alexander, Swiftest, Pi           (players, *MMT and chiefs      *
 *        Heather, Dbra and others.                   of copper dikuMUD)   *
 *      To all those people I know already and those I don't know yet      *
 *      and I hope to know them soon also those who I know by name through *
 *      little contact. Our friendship has been possible thanks to MUD.    *
 *                                                                         *
 *  Students of the Tech of Monterrey (ITESM)                              *
 *  Monterrey, Nuevo Leon. Mexico.                                         *
 *  Second edition                                                         *
 *   (*MMT=Mexican MUD Team)                                               *
 ***************************************************************************/

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "vnum.obj.h"
#include "vnum.mob.h"
#include "vnum.room.h"

/*
   external variables 
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern char *command[];
extern const struct stat_data stat_factor[];
extern struct agi_app_type agi_app[];
extern struct dex_app_type dex_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;

/*
   DISARM 
 */

void do_mobdisarm(P_char ch, char *argument, int cmd)
{
  P_char   victim;
  P_obj    obj;
  char     name[MAX_INPUT_LENGTH];
  byte     percent;

  if (!SanityCheck(ch, "do_mobdisarm"))
    return;

  one_argument(argument, name);

  if (!GET_CLASS(ch, CLASS_WARRIOR) && !GET_CLASS(ch, CLASS_ROGUE) &&
      (GET_LEVEL(ch) < 22) && (!IS_WARRIOR(ch)) && (!IS_THIEF(ch)))
  {
    send_to_char
      ("You better leave all the martial arts to professionals.\r\n", ch);
    return;
  }
  if (!(victim = get_char_room_vis(ch, name)))
  {
    if (ch->specials.fighting)
    {
      victim = ch->specials.fighting;
    }
    else
    {
      send_to_char("Disarm whom?\r\n", ch);
      return;
    }
  }
  if (victim == ch)
  {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  if (CHAR_IN_SAFE_ZONE(victim))
  {
    return;
  }
  if (!ch->equipment[WIELD] && IS_PC(ch))
  {
    send_to_char("You need to wield a weapon, to make it a success.\r\n", ch);
    return;
  }
  obj = victim->equipment[WIELD];
  if (!obj)
  {
    send_to_char
      ("Hey!.. the victim doesn't have any weapon wielded to disarm.\r\n",
       ch);
    return;
  }
  percent = number(1, 101);     /*
                                   101% is a complete failure 
                                 */
  /*
     bonus penalties 
   */
  percent -= dex_app[STAT_INDEX(GET_C_DEX(ch))].reaction * 5;
  percent -= io_agi_defense(victim) / 2;

  /*
     If this will be installed as skill the message need to be moved on the
     file messages, and be removed some comments 
   */

  /*
     if (IS_PC(ch) && (percent > ch->only.pc->skills[SKILL_DISARM].learned)) {
     damage(ch, victim, 0, SKILL_DISARM); SET_POS(ch, POS_SITTING +
     GET_STAT(ch)); } else 
   */
  if ((IS_NPC(ch)) && (percent > (3 * GET_LEVEL(ch))))
  {
    /*
       damage(ch, victim, 0, SKILL_DISARM); 
     */
    act("You fruitlessly try to disarm $N. $N sends you sprawling.", 0, ch, 0,
        victim, TO_CHAR);
    act
      ("You easily dodge an attempt by $n to disarm you, who loses $s balance and falls.",
       0, ch, 0, victim, TO_VICT);
    act
      ("An attempt by $n to disarm $N is in vain as $N twists in a funny fashion.",
       1, ch, 0, victim, TO_ROOM);
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
  }
  else
  {
    /*
       damage(ch, victim, 1, SKILL_DISARM); 
     */
    act
      ("You deftly knock $p from $N's hand, making $N fall sprawling on the ground.",
       0, ch, obj, victim, TO_CHAR);
    act
      ("$n knocks $p from your grasp!!..and you lose your balance and fall on the ground.",
       0, ch, obj, victim, TO_VICT);
    act
      ("$n twists $N's arm, making $N fall sprawling on the ground and dropping $S weapon.",
       1, ch, obj, victim, TO_ROOM);
    SET_POS(victim, POS_SITTING + GET_STAT(victim));
    unequip_char(victim, WIELD);
    obj_to_room(obj, ch->in_room);
    CharWait(victim, PULSE_VIOLENCE * 2);
  }
  CharWait(ch, PULSE_VIOLENCE);
}

void call_solve_sanctuary(P_char ch, P_char vict)
{
  struct affected_type af;

  if (!SanityCheck(ch, "call_solve_sanctuary") ||
      !SanityCheck(vict, "call_solve_sanctuary - vict"))
    return;

  if (!affected_by_spell(vict, SPELL_STONE_SKIN))
    return;

  if (!saves_spell(vict, SAVING_FEAR))
  {
    if (affected_by_spell(vict, SPELL_STONE_SKIN))
      affect_from_char(vict, SPELL_STONE_SKIN);
    if (number(0, 4))
    {
      act("Your skin starts to soften, then regains its hard form.",
          1, ch, 0, vict, TO_VICT);
      act("$N's skin starts to soften, then firms up again.",
          1, ch, 0, vict, TO_NOTVICT);
      bzero(&af, sizeof(af));
      af.type = SPELL_STONE_SKIN;
      af.duration = 1;
      affect_to_char(vict, &af);
    }
    else
    {
      act("You feel your flesh soften and return to normal.",
          1, ch, 0, vict, TO_VICT);
      act("$N's skin loses its stone-like apperance.", 1, ch, 0, vict,
          TO_NOTVICT);
    }
  }
  else
  {
    act("$n fails in the attempt to dispel $N's stone-skin spell!.",
        1, ch, 0, vict, TO_NOTVICT);
    act("$n fails in the attempt to dispel your stone-skin!",
        1, ch, 0, vict, TO_VICT);
  }
}

void call_b_fire(P_char ch, P_char vict, int showhead)
{
  char     buf[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

  if (GET_HIT(vict) < 19)       /*
                                   why? 
                                 */
    return;
  if (showhead)
    strcpy(buf1, "The red head of ");
  else
    strcpy(buf1, "");

  sprintf(buf, "%s$n breathes fire breath at $N.", buf1);
  act(buf, 1, ch, 0, vict, TO_NOTVICT);
  sprintf(buf, "%s$n singes you with a powerful fire breath !.", buf1);
  act(buf, 1, ch, 0, vict, TO_VICT);
  act("You feel your flesh start to melt off your body!.", 1, ch, 0, vict,
      TO_VICT);
  spell_fire_breath(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
}

void call_b_frost(P_char ch, P_char vict, int showhead)
{
  char     buf[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

  if (GET_HIT(vict) < 19)
    return;
  if (showhead)
    strcpy(buf1, "The white head of ");
  else
    strcpy(buf1, "");

  sprintf(buf, "%s$n sends a powerful frost breath to $N.", buf1);
  act(buf, 1, ch, 0, vict, TO_NOTVICT);
  sprintf(buf, "%s$n sends you a powerful frost breath !.", buf1);
  act(buf, 1, ch, 0, vict, TO_VICT);

  spell_frost_breath(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
}

void call_b_acid(P_char ch, P_char vict, int showhead)
{
  char     buf[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

  if (GET_HIT(vict) < 19)
    return;
  if (showhead)
    strcpy(buf1, "The black head of ");
  else
    strcpy(buf1, "");

  sprintf(buf, "%s$n spits acid at $N.", buf1);
  act(buf, 1, ch, 0, vict, TO_NOTVICT);
  sprintf(buf, "%s$n spits acid towards you!", buf1);
  act(buf, 1, ch, 0, vict, TO_VICT);

  spell_acid_breath(GET_LEVEL(ch), ch, NULL, 0, vict, 0);
}

void call_b_gas(P_char ch, P_char vict, int showhead)
{
  char     buf[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

  if (GET_HIT(vict) < 19)
    return;
  if (showhead)
    strcpy(buf1, "The green head of ");
  else
    strcpy(buf1, "");

  sprintf(buf, "%s$n rears back and billows poison gas.", buf1);
  act(buf, 1, ch, 0, 0, TO_ROOM);

  spell_gas_breath(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
}

void call_b_lig(P_char ch, P_char vict, int showhead)
{
  char     buf[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

  if (GET_HIT(vict) < 19)
    return;
  if (showhead)
    strcpy(buf1, "The blue head of ");
  else
    strcpy(buf1, "");

  sprintf(buf, "%s$n sends lightning bolts at $N.", buf1);
  act(buf, 1, ch, 0, vict, TO_NOTVICT);
  sprintf(buf, "%s$n breathes lightning at you!", buf1);
  act(buf, 1, ch, 0, vict, TO_VICT);

  spell_lightning_breath(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
}

int demodragon(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict;
  P_char   temp;
  bool     chance = 0;
  int      dam = cmd / 1000;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!IS_AWAKE(ch))
    return FALSE;

  if (dam != 0)
    cmd = -2;

  give_proper_stat(ch);

  if (cmd != -2)
  {
    if (cmd && pl)
    {
      if (pl == ch)
        return FALSE;

      if ((ch->in_room == real_room(6803)) && (cmd == CMD_DOWN))
      {
        act("The DemoDragon humiliates $n, and blocks $s way.",
            FALSE, ch, 0, pl, TO_NOTVICT);
        send_to_char
          ("The DemoDragon humiliates you, and blocks your way.\r\n", pl);
        return TRUE;
      }
      return FALSE;
    }
  }
  if (!ch->specials.fighting)
    return FALSE;

  for (vict = world[ch->in_room].people; vict; vict = temp)
  {
    temp = vict->next_in_room;
    if (vict && ch == vict->specials.fighting)
    {
      if ((vict != ch->specials.fighting) &&
          (GET_LEVEL(ch) > 13) && !number(0, 8))
      {
        call_b_fire(ch, vict, TRUE);
        chance = 1;
        if (!number(0, 9))
          return TRUE;
      }
      if ((GET_LEVEL(ch) > 13) && (!number(0, 8) || (GET_HIT(ch) < 100)))
      {
        call_b_frost(ch, vict, TRUE);
        chance = 1;
        if (!number(0, 3))
          return TRUE;
      }
      if ((vict != ch->specials.fighting) &&
          (GET_LEVEL(ch) > 13) && !number(0, 8))
      {
        call_b_frost(ch, vict, TRUE);
        chance = 1;
        if (!number(0, 9))
          return TRUE;
      }
      if ((GET_LEVEL(ch) > 13) && (!number(0, 8) || (GET_HIT(ch) < 100)))
      {
        call_b_fire(ch, vict, TRUE);
        chance = 1;
        if (!number(0, 3))
          return TRUE;
      }
      if ((GET_LEVEL(ch) > 13) && !number(0, 100))
      {
        call_solve_sanctuary(ch, vict);
        chance = 1;
      }
    }
  }
  if (chance)
    return TRUE;
  return FALSE;
}

int room_of_sanctum(int room, P_char ch, int cmd, char *arg)
{
  P_char archbishop, bishop;
  P_obj  key;

  if( cmd == CMD_SET_PERIODIC )
    return FALSE;

  if( (cmd != CMD_UNLOCK) || !ch || !IS_AWAKE(ch) )
    return FALSE;

  // What if these mobs have been lured from their rooms?
  for( archbishop = world[real_room0(VROOM_DRAGONNIA_ARCHBISHOP)].people; archbishop; archbishop = archbishop->next_in_room )
  {
    if( IS_NPC(archbishop) && GET_VNUM(archbishop) == VMOB_DRAGONNIA_ARCHBISHOP )
    {
      break;
    }
  }
  // We'll hunt the world, just to make me happy.
  if( !archbishop )
  {
    for( archbishop = character_list; archbishop; archbishop = archbishop->next )
    {
      if( IS_NPC(archbishop) && GET_VNUM(archbishop) == VMOB_DRAGONNIA_ARCHBISHOP )
      {
        break;
      }
    }
  }
  for( bishop = world[real_room0(VROOM_DRAGONNIA_BISHOP)].people; bishop; bishop = bishop->next_in_room )
  {
    if( IS_NPC(bishop) && GET_VNUM(bishop) == VMOB_DRAGONNIA_BISHOP )
    {
      break;
    }
  }
  // We'll hunt the world, just to make me happy. - I mean, how ofter are you going to attempt to unlock
  //   in this room?
  if( !bishop )
  {
    for( bishop = character_list; bishop; bishop = bishop->next )
    {
      if( IS_NPC(bishop) && GET_VNUM(bishop) == VMOB_DRAGONNIA_BISHOP )
      {
        break;
      }
    }
  }

  if( archbishop && archbishop->equipment[HOLD]
    && OBJ_VNUM(archbishop->equipment[HOLD]) == VOBJ_DRAGONNIA_ARCHBISHOP_KEY)
  {
    if( ch->equipment[HOLD] && OBJ_VNUM(ch->equipment[HOLD]) == VOBJ_DRAGONNIA_ARCHBISHOP_KEY)
    {
      obj_to_char( unequip_char(ch, HOLD), ch );
    }
    for( key = ch->carrying; key && OBJ_VNUM(key) != VOBJ_DRAGONNIA_ARCHBISHOP_KEY; key = key->next_content)
      ;
    if( key )
    {
      act("The $o melts as you put it in the lock and burns you.", 0, ch, key, 0, TO_CHAR);
      act("Maybe you should find the 'real' key??", 0, ch, key, 0, TO_CHAR);
      act("The $o of $n has melted and you hear $m scream in pain!", 0, ch, key, 0, TO_ROOM);
      obj_from_char(key);
      extract_obj(key, TRUE); // Key is not an arti, but ok.
      key = NULL;
      if( GET_HIT(ch) <= 90 )
        GET_HIT(ch) = 1;
      else
        GET_HIT(ch) -= 90;
      StartRegen(ch, EVENT_HIT_REGEN);
      GET_VITALITY(ch) = 0;
      StartRegen(ch, EVENT_MOVE_REGEN);
    }
    return TRUE;
  }
  if( bishop && bishop->equipment[HOLD]
    && OBJ_VNUM(bishop->equipment[HOLD]) == VOBJ_DRAGONNIA_BISHOP_KEY)
  {
    act("A strange force doesn't let you unlock the sanctum.", 0, ch, 0, 0, TO_CHAR);
    act("A strange force doesn't let $n unlocks the sanctum.", 1, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}

int dragon_guard(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict, next;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  give_proper_stat(ch);

  if (!IS_AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;

  if ((world[ch->in_room].zone == world[real_room(6809)].zone) ||
      (world[ch->in_room].zone == world[real_room(6858)].zone))
  {
    /*
       for catch fast people whose use alias or macros 
     */
    if (cmd)
    {
      if ((!((cmd > CMD_DOWN) || (cmd < CMD_NORTH)) || (cmd == CMD_CAST)) &&
          !number(0, 3) && IS_PC(pl) && !IS_TRUSTED(pl))
      {
        act("$n screams 'INVADERS! INVADERS! BANZAI!!!CHARGE!!!ARGGHH!'",
            FALSE, ch, 0, 0, TO_ROOM);
        MobStartFight(ch, pl);
        return TRUE;
      }
      else
        return FALSE;
    }
    for (vict = world[ch->in_room].people; vict; vict = next)
    {
      next = vict->next_in_room;

      if (CAN_SEE(ch, vict) &&
          (IS_PC(vict) || ( !IS_DRAGON(vict) && IS_PC_PET(vict) ) )
          )
      {
        act("$n screams 'INVADERS! INVADERS! BANZAI!!!CHARGE!!!ARGGHH!'",
            FALSE, ch, 0, 0, TO_ROOM);
        MobStartFight(ch, vict);
        return TRUE;
      }
      else if (IS_NPC(vict) && GET_VNUM(vict) == 6826)
      {
        act("$n screams 'DESERTERS! Fresh blood! Kill!'",
            FALSE, ch, 0, 0, TO_ROOM);
        MobStartFight(ch, vict);
        return TRUE;
      }
    }
  }
  return FALSE;
}

int dragons_of_dragonnia(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict;
  P_char   temp;
  bool     chance = 0;
  bool     head;
  int      dam = cmd / 1000;
  char     Gbuf2[MAX_STRING_LENGTH];

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (dam != 0)
    cmd = -2;

  give_proper_stat(ch);

  if (cmd != -2)
  {
    if (cmd && pl)
    {
      if (pl == ch)
        return FALSE;

      /*
         generic block exit type deal 
       */
      /*
         must be a harem-sort of thing, neutral sex chars can pass 
       */
      if (GET_SEX(pl))
      {

        switch (cmd)
        {
        case CMD_NORTH:
          if ((ch->in_room == real_room(6824)) ||       /*
                                                           blue dragon 
                                                         */
              (ch->in_room == real_room(6833)) ||       /*
                                                           mystic dragon 
                                                         */
              (ch->in_room == real_room(6848))) /*
                                                   sacristan dragon 
                                                 */
            chance = TRUE;
          break;
        case CMD_EAST:
          if ((ch->in_room == real_room(6828)) ||       /*
                                                           green dragon 
                                                         */
              (ch->in_room == real_room(6864))) /*
                                                   red dragon 
                                                 */
            chance = TRUE;
          break;
        case CMD_SOUTH:
          if ((ch->in_room == real_room(6848)) ||       /*
                                                           sacristan dragon 
                                                         */
              (ch->in_room == real_room(6810)) ||       /*
                                                           white dragons 
                                                         */
              (ch->in_room == real_room(6806)) ||       /*
                                                           black dragon 
                                                         */
              (ch->in_room == real_room(6821))) /*
                                                   golden dragon 
                                                 */
            chance = TRUE;
          break;
        case CMD_UP:
          if (ch->in_room == real_room(6852))   /*
                                                   monk dragon 
                                                 */
            chance = TRUE;
          break;
        }
        if (chance)
        {
          act("$N humiliates $n, and blocks $s way.",
              FALSE, pl, 0, ch, TO_NOTVICT);
          act("$n humiliates you, and blocks your way.",
              FALSE, pl, 0, ch, TO_VICT);
          return TRUE;
        }
      }
      return FALSE;
    }
  }
  /*
     it is on a hit 
   */
  if ((GET_RNUM(ch) == real_mobile(6815)) || (GET_RNUM(ch) == real_mobile(6816)))
    head = TRUE;
  else
    head = FALSE;

  if (!ch->specials.fighting)
    return FALSE;

  for (vict = world[ch->in_room].people; vict; vict = temp)
  {
    temp = vict->next_in_room;

    if (vict && vict->specials.fighting == ch)
    {
      if ((GET_HIT(vict) < 39) && (GET_RNUM(ch) == real_mobile(6813)) &&
          (IS_GOOD(vict)))
      {
        act("$n pardons $N !!...and sends $M back home. =).",
            1, ch, 0, vict, TO_NOTVICT);
        act("$n pardons you!!... and sends you back home!!! =)'.",
            1, ch, 0, vict, TO_VICT);
        spell_heal(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, vict, 0);
        spell_word_of_recall(GET_LEVEL(ch), vict, 0, SPELL_TYPE_SPELL, vict,
                             0);
        return TRUE;
      }
      if (((GET_LEVEL(ch) > 24) || !number(0, 14)) &&
          (((GET_HIT(ch) * 100 / GET_MAX_HIT(ch)) > 90) || !number(0, 19)))
      {
        call_solve_sanctuary(ch, vict);
        chance = 1;
      }
      if ((GET_LEVEL(ch) > 13) && !number(0, 8) &&
          ((GET_RNUM(ch) == real_mobile(6802)) ||
           (GET_RNUM(ch) == real_mobile(6806)) ||
           (GET_RNUM(ch) == real_mobile(6816)) || (GET_RNUM(ch) == real_mobile(6815))))
      {
        call_b_frost(ch, vict, head);
        if (!number(0, 19))
          return TRUE;
        chance = 1;
        if (ch->in_room != vict->in_room)
          continue;
      }
      if ((GET_LEVEL(ch) > 13) && !number(0, 13) &&
          ((GET_RNUM(ch) == real_mobile(6803)) ||
           (GET_RNUM(ch) == real_mobile(6807)) ||
           (GET_RNUM(ch) == real_mobile(6813)) ||
           (GET_RNUM(ch) == real_mobile(6816)) || (GET_RNUM(ch) == real_mobile(6815))))
      {
        call_b_lig(ch, vict, head);
        if (!number(0, 99))
          return TRUE;
        chance = 1;
        if (ch->in_room != vict->in_room)
          continue;
      }
      if ((GET_LEVEL(ch) > 13) && !number(0, 8) &&
          ((GET_RNUM(ch) == real_mobile(6804)) ||
           (GET_RNUM(ch) == real_mobile(6808)) ||
           (GET_RNUM(ch) == real_mobile(6816)) || (GET_RNUM(ch) == real_mobile(6815))))
      {
        call_b_gas(ch, vict, head);
        if (!number(0, 9))
          return TRUE;
        chance = 1;
        if (ch->in_room != vict->in_room)
          continue;
      }
      if ((GET_LEVEL(ch) > 13) && !number(0, 3) &&
          ((GET_RNUM(ch) == real_mobile(6805)) ||
           (GET_RNUM(ch) == real_mobile(6809)) || (GET_RNUM(ch) == real_mobile(6816))))
      {
        call_b_acid(ch, vict, head);
        if (!number(0, 9))
          return TRUE;
        chance = 1;
        if (ch->in_room != vict->in_room)
          continue;
      }
      if ((GET_LEVEL(ch) > 13) && !number(0, 3) &&
          ((GET_RNUM(ch) == real_mobile(6810)) ||
           (GET_RNUM(ch) == real_mobile(6825)) ||
           (GET_RNUM(ch) == real_mobile(6824)) ||
           (GET_RNUM(ch) == real_mobile(6831)) ||
           (GET_RNUM(ch) == real_mobile(6832)) ||
           (GET_RNUM(ch) == real_mobile(6811)) ||
           (GET_RNUM(ch) == real_mobile(6812)) ||
           (GET_RNUM(ch) == real_mobile(6816)) || (GET_RNUM(ch) == real_mobile(6813))))
      {
        call_b_fire(ch, vict, head);
        if (!number(0, 9))
          return TRUE;
        chance = 1;
        if (ch->in_room != vict->in_room)
          continue;
      }
      if (!number(0, 20) && ((GET_RNUM(ch) == real_mobile(6810)) ||
                             (GET_RNUM(ch) == real_mobile(6813)) ||
                             (GET_RNUM(ch) == real_mobile(6811))))
      {
        if (vict->player.name)
          strcpy(Gbuf2, vict->player.name);
        do_mobdisarm(ch, Gbuf2, 0);
        chance = 1;
        if (ch->in_room != vict->in_room)
          continue;
      }
      if (!number(0, 3) && ((GET_RNUM(ch) == real_mobile(6810)) ||
                            (GET_RNUM(ch) == real_mobile(6811))))
      {
        if (!damage(ch, vict, number((GET_LEVEL(ch) >> 1),
                                     (GET_LEVEL(ch) * 6)), TYPE_UNDEFINED))
          damage(ch, vict, number((GET_LEVEL(ch) >> 1),
                                  (GET_LEVEL(ch) * 7)), TYPE_UNDEFINED);
        chance = 1;
        if (ch->in_room != vict->in_room)
          continue;
      }
      if ((GET_LEVEL(ch) > 13) && !number(0, 5) &&
          ((GET_RNUM(ch) == real_mobile(6813)) ||
           (GET_RNUM(ch) == real_mobile(6812)) ||
           (GET_RNUM(ch) == real_mobile(6832)) || (GET_RNUM(ch) == real_mobile(6831))))
      {
        act("$n blasts $N with a powerful fireball.",
            1, ch, 0, vict, TO_NOTVICT);
        act("$n blasts you with a powerful fireball!.",
            1, ch, 0, vict, TO_VICT);
        spell_fireball(15, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        if (!number(0, 9))
          return TRUE;
        chance = 1;
        if (ch->in_room != vict->in_room)
          continue;
      }
      if ((!number(0, 19) || ((GET_HIT(ch) < 199) && !number(0, 3))) &&
          ((GET_RNUM(ch) == real_mobile(6815)) || (GET_RNUM(ch) == real_mobile(6816))))
      {
        act("The heads of $n breath at $N.", 1, ch, 0, vict, TO_NOTVICT);
        act("The heads of $n rear back and breath on you!",
            0, ch, 0, vict, TO_VICT);
        call_b_gas(ch, vict, head);
        if (ch->in_room != vict->in_room)
          return TRUE;
        call_b_lig(ch, vict, head);
        if (ch->in_room != vict->in_room)
          return TRUE;
        call_b_frost(ch, vict, head);
        if (ch->in_room != vict->in_room)
          return TRUE;
        if (GET_RNUM(ch) == real_mobile(6816))
        {
          call_b_acid(ch, vict, head);
          if (ch->in_room != vict->in_room)
            return TRUE;
          call_b_fire(ch, vict, head);
        }
        return TRUE;
      }
      if ((GET_LEVEL(ch) > 13) && !number(0, 99) &&
          (GET_RNUM(ch) == real_mobile(6813)))
      {
        act("$n send $N somewhere.", 1, ch, 0, vict, TO_NOTVICT);
        act("$n tells you 'Go home!!! <grin>'.", 1, ch, 0, vict, TO_VICT);
        spell_word_of_recall(GET_LEVEL(ch), vict, 0, SPELL_TYPE_SPELL, vict,
                             0);
        return TRUE;
      }
    }
  }
  if (chance)
    return TRUE;
  return FALSE;
}

#define H_MAMMA 0
#define H_PAPA 1
#define H_BROTHER 2
#define H_SISTER 3
#define H_FRIEND 4

void call_protector(P_char ch, P_char tmp_ch, int nprotector, int type)
{
  P_char   protector;
  char     buf[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];
  static const char *helpertype[] = {
    "mamma",
    "papa",
    "brother",
    "sister",
    "friend",
    "\n"
  };
  static const char *caretype[] = {
    "baby",
    "baby",
    "brother",
    "brother",
    "friend",
    "\n"
  };

  // Create the protector from the number especified.
  if( !(protector = read_mobile(nprotector, REAL)) )
    return;

  // Verify gender
  if( (GET_SEX(protector) == SEX_MALE) && ((type == H_MAMMA) || (type == H_SISTER)) )
  {
    if( type == H_MAMMA )
      type = H_PAPA;
    else
      type = H_BROTHER;
  }
  sprintf(buf, "$n tells you 'My %s is coming'.", helpertype[(int) type]);
  sprintf(buf1, "$n told $N 'My %s is coming'.", helpertype[(int) type]);

  act("$n is crying.", 0, ch, 0, 0, TO_ROOM);
  act(buf, 1, ch, 0, tmp_ch, TO_VICT);
  act(buf1, 1, ch, 0, tmp_ch, TO_NOTVICT);

//  if (IS_SET(protector->specials.act, ACT_AGGRESSIVE))
//    REMOVE_BIT(protector->specials.act, ACT_AGGRESSIVE);
  if (IS_AGGRESSIVE(protector))
  {
    protector->only.npc->aggro_flags = protector->only.npc->aggro2_flags = protector->only.npc->aggro3_flags = 0;
  }
  if (!IS_SET(protector->specials.act, ACT_MEMORY))
    SET_BIT(protector->specials.act, ACT_MEMORY);
  char_to_room(protector, ch->in_room, 0);
  stop_fighting(tmp_ch);
  if (ch->following)
    stop_follower(ch);
  add_follower(ch, protector);
  group_add_member(protector, ch);
  act(" $n blocks your attack! ", FALSE, ch->following, 0, tmp_ch, TO_VICT);
  act(" $n blocks $N's attack! ", FALSE, ch->following, 0, tmp_ch,
      TO_NOTVICT);
  act(" You block $N's attack! ", FALSE, ch->following, 0, tmp_ch, TO_CHAR);
  if( !IS_FIGHTING(tmp_ch) )
    set_fighting(tmp_ch, ch->following);
  if( (GET_SEX(ch) == SEX_FEMALE) &&
      !((type == H_MAMMA) || (type == H_PAPA) || (type == H_FRIEND)) )
    strcpy(buf, "sister");
  else
    strcpy(buf, caretype[(int) type]);
  sprintf(buf1, "$n says 'Why? why? why are you fighting with my %s'.", buf);
  act(buf1, 1, protector, 0, 0, TO_ROOM);
  if( protector->in_room != tmp_ch->in_room )
    return;

  // Removing these to stop a crash bug.
//  if( !damage(protector, tmp_ch, number((GET_LEVEL(protector) >> 1), (GET_LEVEL(protector) * 2)), TYPE_UNDEFINED) )
  {
//    if( !damage(protector, tmp_ch, number((GET_LEVEL(protector) >> 1), (GET_LEVEL(protector) * 2)), TYPE_UNDEFINED) )
    {
      damage(protector, tmp_ch, number((GET_LEVEL(protector) >> 1), (GET_LEVEL(protector) * 2)), TYPE_UNDEFINED);
    }
  }
}

void call_protection(P_char ch, P_char tmp_ch)
{
  char     Gbuf2[MAX_STRING_LENGTH];

  act("$n is crying.", 0, ch, 0, 0, TO_ROOM);

  if (ch->following->in_room == ch->in_room)
  {
    stop_fighting(tmp_ch);
    act(" $n blocks your attack! ", FALSE, ch->following, 0, tmp_ch, TO_VICT);
    act(" $n blocks $N's attack! ", FALSE, ch->following, 0, tmp_ch, TO_NOTVICT);
    act(" You block $N's attack! ", FALSE, ch->following, 0, tmp_ch, TO_CHAR);
    if ((ch->following != tmp_ch) && IS_NPC(ch->following))
    {
      act("$n says 'Why? why? why are you fighting with $N?'.",
          1, ch->following, 0, ch, TO_NOTVICT);
      if (!damage(ch->following, tmp_ch,
                  number((GET_LEVEL(ch->following) >> 1),
                         (GET_LEVEL(ch->following) * 2)), TYPE_UNDEFINED))
        if (!damage(ch->following, tmp_ch,
                    number((GET_LEVEL(ch->following) >> 1),
                           (GET_LEVEL(ch->following) * 2)), TYPE_UNDEFINED))
          damage(ch->following, tmp_ch,
                 number((GET_LEVEL(ch->following) >> 1),
                        (GET_LEVEL(ch->following) * 2)), TYPE_UNDEFINED);
    }
    else if (ch->following == tmp_ch)
    {
      stop_fighting(ch);
      act
        ("$n asks $mself 'Why? why? why was I fighting with $N?  Who is my friend!'.",
         1, ch->following, 0, ch, TO_ROOM);
      act
        ("You ask yourself, why, why was I about to attack my little friend?'.",
         1, ch->following, 0, ch, TO_CHAR);
    }
    else if (IS_PC(ch->following))
    {
      act("$n says 'Why? why? why are you fighting with $N?'.",
          1, ch->following, 0, ch, TO_NOTVICT);
      strcpy(Gbuf2, tmp_ch->player.name);
      do_action(ch->following, Gbuf2, CMD_SLAP);
      do_action(ch->following, Gbuf2, CMD_SLAP);
      do_action(ch->following, Gbuf2, CMD_SLAP);
      stop_fighting(ch);
    }
    return;
  }
  else
  {
    act("$n is crying.", 0, ch, 0, 0, TO_ROOM);
    act("$n is crying.", 0, ch, 0, 0, TO_ROOM);
    stop_fighting(tmp_ch);
    act("You are called by divine forces to test your heroism!",
        0, ch->following, 0, 0, TO_CHAR);
    act("$n is called by divine forces to test $s heroism!.",
        0, ch->following, 0, 0, TO_ROOM);
    char_from_room(ch->following);
    char_to_room(ch->following, ch->in_room, -1);
    act("$n arrives suddenly.", 0, ch->following, 0, 0, TO_ROOM);
    act("$n blocks your attack! ", FALSE, ch->following, 0, tmp_ch, TO_VICT);
    act("$n blocks $N attack! ", FALSE, ch->following, 0, tmp_ch, TO_NOTVICT);
    act("You block $N's attack! ", FALSE, ch->following, 0, tmp_ch, TO_CHAR);
    if ((ch->following != tmp_ch) && (IS_NPC(ch->following)))
    {
      act("$n says 'Why? why? why are you fighting with $N?'.",
          1, ch->following, 0, ch, TO_NOTVICT);
      if (!damage(ch->following, tmp_ch,
                  number((GET_LEVEL(ch->following) >> 1),
                         (GET_LEVEL(ch->following) * 2)), TYPE_UNDEFINED))
        if (!damage(ch->following, tmp_ch,
                    number((GET_LEVEL(ch->following) >> 1),
                           (GET_LEVEL(ch->following) * 2)), TYPE_UNDEFINED))
          damage(ch->following, tmp_ch,
                 number((GET_LEVEL(ch->following) >> 1),
                        (GET_LEVEL(ch->following) * 2)), TYPE_UNDEFINED);
    }
    else if (ch->following == tmp_ch)
    {
      stop_fighting(ch);
      act
        ("$n asks $mself 'Why? why? why was I fighting with $N?  Who is my friend!'.",
         1, ch->following, 0, ch, TO_ROOM);
      act
        ("You ask yourself, why, why was I about to attack my little friend?'.",
         1, ch->following, 0, ch, TO_CHAR);
    }
    else if (IS_PC(ch->following))
    {
      act("$n says 'Why? why? why are you fighting with $N?'.",
          1, ch->following, 0, ch, TO_NOTVICT);
      if (tmp_ch->player.name)
        strcpy(Gbuf2, tmp_ch->player.name);
      do_action(ch->following, Gbuf2, CMD_SLAP);
      do_action(ch->following, Gbuf2, CMD_SLAP);
      do_action(ch->following, Gbuf2, CMD_SLAP);
      stop_fighting(ch);
    }
  }
}

int baby_dragon(P_char ch, P_char pl, int cmd, char *arg)
{
  static int num_tears = 0;
  static int rotation = number(0,9);
  P_char   tmp_ch;
  P_char   attacker;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  // Check for periodic event calls
  if( cmd == CMD_SET_PERIODIC )
    return TRUE;

  if( ch->specials.fighting )
  {
    act("$n is crying.", 0, ch, 0, 0, TO_ROOM);
    for( tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room )
    {
      if( IS_ALIVE(tmp_ch) && (tmp_ch->specials.fighting == ch) )
      {
        /* Commented this out.. to make it harder to cheese the dragonslayer achievement.
        if( OUTSIDE(ch) && !ch->group )
         */
        {
          act("$n cries louder.", 0, ch, 0, 0, TO_ROOM);
          if( ++num_tears >= 10 )
          {
            act("You hear an ungodly amount of flapping wings!.", 0, ch, 0, 0, TO_ROOM);
            while( num_tears-- > 0 )
            {
              switch( number(0, 2) )
              {
                case 0:
                  call_protector(ch, tmp_ch, real_mobile(6830), H_FRIEND);
                  break;
                case 1:
                  call_protector(ch, tmp_ch, real_mobile(6817), H_FRIEND);
                  break;
                case 2:
                  call_protector(ch, tmp_ch, real_mobile(6803), H_FRIEND);
                  break;
              }
              if( !IS_ALIVE(tmp_ch) )
              {
                return TRUE;
              }
            }
          }
          act("You see a great dragon flying toward you!.", 0, ch, 0, 0, TO_ROOM);
          // This way we cycle through each as opposed to picking a random one.
          rotation = (rotation + 1) % 10;
          switch( rotation )
          {
          case 1:
            call_protector(ch, tmp_ch, real_mobile(6830), H_FRIEND);
            break;
          case 2:
            call_protector(ch, tmp_ch, real_mobile(6816), H_MAMMA);
            break;
          case 3:
            call_protector(ch, tmp_ch, real_mobile(6806), H_PAPA);
            break;
          case 4:
            call_protector(ch, tmp_ch, real_mobile(6807), H_PAPA);
            break;
          case 5:
            call_protector(ch, tmp_ch, real_mobile(6808), H_PAPA);
            break;
          case 6:
            call_protector(ch, tmp_ch, real_mobile(6809), H_PAPA);
            break;
          case 7:
            call_protector(ch, tmp_ch, real_mobile(6832), H_MAMMA);
            break;
          default:
            call_protector(ch, tmp_ch, real_mobile(6827), H_BROTHER);
            break;
          }
          return TRUE;
        }
        if( ch->group )
        {
          for( attacker = world[ch->in_room].people; attacker; attacker = attacker->next_in_room )
          {
            if( attacker && (attacker->specials.fighting == ch) )
            {
              call_protection(ch, attacker);
            }
          }
          return TRUE;
        }
        if (!ch->group && !number(0, 9))
        {
          act("$n is crying.", 0, ch, 0, 0, TO_ROOM);
          act("$n cries louder.", 0, ch, 0, 0, TO_ROOM);
          act("You see a dragon coming toward you!.", 0, ch, 0, 0, TO_ROOM);
          call_protector(ch, tmp_ch, real_mobile(6814), H_FRIEND);
          return TRUE;
        }
      }
    }
  }
  if( cmd && pl )
  {
    if( pl == ch )
      return FALSE;
    switch (cmd)
    {
      case CMD_PAT:
        do_action(pl, arg, CMD_PAT);
        if( isname(arg, ch->player.name) )
        {
          if( ch->following )
            stop_follower(ch);
          add_follower(ch, pl);
          group_add_member(pl, ch);
          act("$n tells you 'Can i be your friend? =)'.", 1, ch, 0, pl, TO_VICT);
        }
        return TRUE;
        break;
      case CMD_KILL:
      case CMD_HIT:
      case CMD_BACKSTAB:
      case CMD_BASH:
      case CMD_KICK:
      case CMD_MURDER:
        one_argument(arg, Gbuf1);
        if ((ch == get_char_room(Gbuf1, pl->in_room)) && (pl == ch->following))
        {
          strcpy(Gbuf2, pl->player.name);
          do_action(pl, Gbuf2, CMD_SLAP);
          act("$n ask $mself 'Why? why? why was I trying to harm $N?  Who is my friend!'.",
            1, ch->following, 0, ch, TO_ROOM);
          act("You ask yourself 'Why? why? why was I trying to harm $N?  Who is my friend!'.",
            1, ch->following, 0, ch, TO_CHAR);
          return TRUE;
        }
        break;
      default:
        return FALSE;
        break;
    }
  }
  return FALSE;
}

#define TIMER 9
void make_remains(P_char ch)
{
  P_obj    remains;
  char     buf[MAX_INPUT_LENGTH];

  remains = read_object(3, VIRTUAL);

  remains->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);

  remains->name = str_dup("gold remains");

  sprintf(buf, "The remains of %s are scattered here.",
          ch->player.short_descr);
  remains->description = str_dup(buf);

  sprintf(buf, "The scattered remains of %s", ch->player.short_descr);
  remains->short_description = str_dup(buf);

  remains->value[2] = 50;
  obj_to_room(remains, ch->in_room);
}

int statue(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   temp;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!IS_NPC(ch))
    return FALSE;

  if (cmd < 0)
  {
    if (!ch->only.npc->spec[0])
      ch->only.npc->spec[0] = number(3, 5);
    else if (!--ch->only.npc->spec[0])
    {
      make_remains(ch);
      return (TRUE);
    }
    temp = read_mobile(GET_RNUM(ch), REAL);
    if (temp)
    {
      char_to_room(temp, ch->in_room, 0);
      temp->only.npc->spec[0] = ch->only.npc->spec[0];
      set_fighting(temp, ch->specials.fighting);
    }
    temp = read_mobile(GET_RNUM(ch), REAL);
    if (temp)
    {
      char_to_room(temp, ch->in_room, 0);
      temp->only.npc->spec[0] = ch->only.npc->spec[0];
      set_fighting(temp, ch->specials.fighting);
    }
    act
      ("The stones of the statue split apart and reform into two new statues.",
       TRUE, ch, 0, 0, TO_ROOM);
  }
  if (pl)
  {
    switch (cmd)
    {
    case CMD_UNLOCK:
      act
        ("The great statue begins to move with a magical aura and blocks your action.\r\n ",
         TRUE, ch, 0, 0, TO_ROOM);
      act("$n says 'Don't ever try that again!", TRUE, ch, 0, 0, TO_ROOM);
      return (TRUE);
      break;
    case CMD_KICK:
      if ((GET_HIT(ch) < 90) &&
          (GET_CLASS(pl, CLASS_ROGUE) ||
           GET_CLASS(pl, CLASS_WARRIOR)) && (!number(0, 9) || !number(0, 9)))
      {
        /*
           act("$N couldn't absorb the energy of $n!!", TRUE, pl, 0, ch,
           TO_ROOM); 
         */
        act("  $N is destroyed by the powerful kick of $n.", TRUE, pl, 0, ch,
            TO_ROOM);
        act(" Ohhh GREAT!! You just destroyed $N.", FALSE, pl, 0, ch,
            TO_CHAR);
        if (!number(0, 19) || !number(0, 19))
        {
          GET_HIT(ch) = GET_MAX_HIT(ch);
          act("The stones of the statue split apart and reform again.",
              TRUE, ch, 0, 0, TO_ROOM);
        }
        else
        {
          make_remains(ch);
          death_cry(ch);
          extract_char(ch);
          ch = NULL;
        }
        return (TRUE);
      }
      break;
    default:
      break;
    }
  }
  return (FALSE);
}
