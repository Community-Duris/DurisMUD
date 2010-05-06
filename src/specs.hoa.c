/*
   ***************************************************************************
   *  File: specs.hoa.c                                     Part of Duris    *
   *  Usage: special procedures for Hall of Ancients zone                    *
   *  Copyright  1990, 1991 - see 'license.doc' for complete information.    *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "justice.h"
#include "assocs.h"
#include "graph.h"
#include "disguise.h"
#include "damage.h"
#include "grapple.h"

/*
   external variables
 */
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern char *command[];
extern const char *dirs[];
extern const char *race_types[];
extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int innate_abilities[];
extern int planes_room_num[];
extern int top_of_world;
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct dex_app_type dex_app[52];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern const int exp_table[];
extern bool has_skin_spell(P_char);

int trap_razor_hooks(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return FALSE;

  if (IS_TRUSTED(ch) || IS_NPC(ch))
    return FALSE;

  if (cmd == CMD_DOWN)
  {
    if (IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM))
      return FALSE;
    else
    {
      act("&+LYou feel sharp and terrible pain as you traverse the walls.", TRUE, ch, 0, 0, TO_CHAR);
      spell_lightning_bolt(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
    }
    return FALSE;
  }
  return FALSE;
}

int trap_tower1_para(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return FALSE;

  if (IS_TRUSTED(ch) || IS_NPC(ch))
    return FALSE;

  if (cmd == CMD_UP)
  {
    if (IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM))
      return FALSE;
    else
    {
      act("&+wYou freeze in your tracks and gaze in awe at the sight of &+BRas&+bda&+Brn&+w.&n", TRUE, ch, 0, 0, TO_CHAR);
      if(!number(0, 2))
        CharWait(ch, PULSE_VIOLENCE * 5);
    }
    return FALSE;
  }
  return FALSE;
}

int trap_tower2_sleep(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return FALSE;

  if (IS_TRUSTED(ch) || IS_NPC(ch))
    return FALSE;

  if (cmd == CMD_UP)
  {
    if (IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM))
      return FALSE;
    else
    {
      act("&+LYou suddenly begin to feel very drowsy.&n", TRUE, ch, 0, 0, TO_CHAR);
      if (!number(0, 2))
        SET_POS(ch, GET_POS(ch) + STAT_SLEEPING);
    }
    return FALSE;
  }
  return FALSE;
}

#define LIMB_ARM 1
#define LIMB_LEG 2
#define LAST_LIMB 2

int illesarus(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict;
  int dam, limb, curr_time;
  struct affected_type af;

  struct damage_messages messages = {
    "&+rBlood &+Lflies everywhere as &+MIl&+mlesar&+Mus &+Lslices without restraint.&n",
    "&+rBlood &+Lflies everywhere as &+MIl&+mlesar&+Mus &+Lslices without restraint.&n",
    "&+rBlood &+Lflies everywhere as &+MIl&+mlesar&+Mus &+Lslices without restraint.&n",
    "&+rBlood &+Lflies everywhere as &+MIl&+mlesar&+Mus &+Lslices without restraint.&n",
    "&+rBlood &+Lflies everywhere as &+MIl&+mlesar&+Mus &+Lslices without restraint.&n",
    "&+rBlood &+Lflies everywhere as &+MIl&+mlesar&+Mus &+Lslices without restraint.&n"
  };


  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  
  if (!ch && obj && cmd == CMD_PERIODIC && !number(0, 3))
  {
    hummer(obj);
    return TRUE;
  }
  
  if (!ch || !obj)
    return FALSE;

  if(!OBJ_WORN_POS(obj, WIELD))
    return false;
 
  curr_time = time(NULL);

  if (!has_skin_spell(ch) &&
      obj->timer[0] + (int) get_property("timer.stoneskin.generic", 60) <= curr_time)
  {
    spell_stone_skin(30, ch, 0, SPELL_TYPE_POTION, ch, 0);
    obj->timer[0] = curr_time;
    return FALSE;
  }

  if (cmd != CMD_MELEE_HIT || !ch)
    return FALSE;

  vict = (P_char) arg;
  dam = BOUNDED(0, (GET_HIT(vict) + 9), 200);

  if(vict &&
    !number(0, 24) &&
    CheckMultiProcTiming(ch))
  {
    // The damage
    act("&+LYou swing&n $q &+Lwith a broad two-handed arch at&n $N.&n", FALSE, ch, obj, vict, TO_CHAR);
    act("$n &+Lswings&n $q &+Lwith a broad two-handed arch at you.&n", FALSE, ch, obj, vict, TO_VICT);
    act("$n &+Lswings&n $q &+Lwith a broad two-handed arch at&n $N.&n", FALSE, ch, obj, vict, TO_NOTVICT);
    raw_damage(ch, vict, (BOUNDED(0, (GET_HIT(vict) + 9), 100) * 4), RAWDAM_DEFAULT, &messages);

    // The severing.. going to use grapple code here since it already exists
    // and does exactly what I'm looking for.
    limb = number(0, LAST_LIMB);
    memset(&af, 0, sizeof(af));
    if (limb)
    {
      switch (limb)
      {
        case LIMB_ARM:
          act("$N &+Lgasps as your vorpal sword nearly severs $S arm!&n", FALSE, ch, obj, vict, TO_CHAR);
          act("$q &+Lnearly severs your arm from your body!&n", FALSE, ch, obj, vict, TO_VICT);
          act("$N &+Lgasps as the vorpal sword nearly severs $S arm!&n", FALSE, ch, obj, vict, TO_NOTVICT);
          if (!IS_TARMLOCK(vict))
          {
            af.type = TAG_ARMLOCK;
            af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL | AFFTYPE_SHORT;
            af.duration = (4 * WAIT_SEC);
            affect_to_char(vict, &af);
          }
          break;
        case LIMB_LEG:
          act("$N g&+Lasps as your vorpal sword nearly severs $S leg!&n", FALSE, ch, obj, vict, TO_CHAR);
          act("$q &+Lnearly severs your leg from your body!&n", FALSE, ch, obj, vict, TO_VICT);
          act("$N &+Lgasps as the vorpal sword nearly severs $S leg!&n", FALSE, ch, obj, vict, TO_NOTVICT);
          if (!IS_TLEGLOCK(vict))
          {
            af.type = TAG_LEGLOCK;
            af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL | AFFTYPE_SHORT;
            af.duration = (4 * WAIT_SEC);
            affect_to_char(vict, &af);
          }
          break;
        default:
          break;
      }
    }
    return TRUE;
  }
  return FALSE;
}

int morkoth_mother(P_char ch, P_char tch, int cmd, char *arg)
{
  int helpers[] = { 77713 };

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!tch && !number(0, 4))
  {
    return shout_and_hunt(ch, 100, "&+WChildren, come defend your mother against %s!&n\r\n", NULL, helpers, 0, 0);
  }
  return FALSE;
}

int akckx(P_char ch, P_char vict, int cmd, char *arg)
{
  P_char soul;
  P_obj questobj;

  if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC)
    return FALSE;
 
  if (!ch)
    return FALSE;

  SET_BIT(ch->specials.act, ACT_SPEC_DIE);

  if (cmd == CMD_DEATH)
  {
    soul = read_mobile(77748, VIRTUAL);
    questobj = read_object(77750, VIRTUAL);

    if (!soul)
    {
      logit(LOG_DEBUG, "HoA(hall) quest mob for Akckx not available to load.");
      debug("HoA(hall) quest mob for Akckx not available to load.");
      return FALSE;
    }
    char_to_room(soul, real_room(77928), -1);
    
    if (!questobj)
    {
      logit(LOG_DEBUG, "HoA(hall) quest obj for Akckx not available to load.");
      debug("HoA(hall) quest obj for Akckx not available to load.");
      return FALSE;
    }
    obj_to_char(questobj, soul);
    
    return TRUE;
  }
  return FALSE;
}

int human_girl(P_char ch, P_char vict, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;

  if (!number(0, 9))
  {
    switch (number(1, 2))
    {
      case 1:
        say(ch, "Be warned, Aan does not take kindly to those who seek to defame his cathedral.");
        break;
      case 2:
        say(ch, "Strife, Sin, and Death await those who do not bow to the power that is Aan.");
        break;
    }
    return FALSE;
  }
  return FALSE;
}

int hoa_plat(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC)
    return FALSE;

  if (!ch)
    return FALSE;

  if (cmd == CMD_GET || cmd == CMD_TAKE)
  {
    act("&+LA large rumble of stones moving can be heard from the other room.&n", FALSE, ch, 0, 0, TO_ROOM);
    act("&+LA large rumble of stones moving can be heard from the other room.&n", FALSE, ch, 0, 0, TO_CHAR);
    SET_BIT(world[real_room(77888)].dir_option[SOUTH]->exit_info, EX_BLOCKED);
    REMOVE_BIT(world[real_room(77893)].dir_option[DOWN]->exit_info, EX_BLOCKED);
    REMOVE_BIT(world[real_room(77893)].dir_option[DOWN]->exit_info, EX_CLOSED);
    REMOVE_BIT(world[real_room(77944)].dir_option[UP]->exit_info, EX_BLOCKED);
    REMOVE_BIT(world[real_room(77944)].dir_option[UP]->exit_info, EX_CLOSED);
    return FALSE;
  }
  return FALSE;
}

int hoa_death(P_char ch, P_char vict, int cmd, char *arg)
{
  P_char victim;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;

  SET_BIT(ch->specials.act, ACT_SPEC_DIE);

  if (cmd == CMD_DEATH)
  {
    act("&+LDeath &+WSHRIEKS&+L as his power dissipates around this cathedral.&n", FALSE, ch, 0, 0, TO_ROOM);
    REMOVE_BIT(world[real_room(77888)].dir_option[SOUTH]->exit_info, EX_BLOCKED);
    return FALSE;
  }

  if (IS_FIGHTING(ch))
  {
    victim = ch->specials.fighting;
    if (!number(0, 35) || IS_NPC(victim))
    {
      if (!victim || IS_TRUSTED(victim))
        return FALSE;

      act("&+LDeath smirks as he reaches out to touch $N, who suddenly collapses.&n", FALSE, ch, 0, victim, TO_NOTVICT);
      act("&+LDeath smirks as he reaches out to touch you.  You simply collapse.&n", FALSE, ch, 0, victim, TO_VICT);

      die(victim, ch);
      return TRUE;
    }
  }
  return FALSE;
}

int hoa_sin(P_char ch, P_char vict, int cmd, char *arg)
{
  P_char victim;
  struct affected_type af;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch && IS_FIGHTING(ch))
    if (!number(0, 4))
    {
      victim = ch->specials.fighting;
      if (!victim || GET_CLASS(victim, CLASS_PALADIN) || IS_TRUSTED(victim))
        return FALSE;

      act("&+rS&+Ri&+rn &+Lgets ahold of your sight and gazes deep within your soul.&L&+LSuddenly all your past sins seem to catch up to you!&n", FALSE, ch, 0, victim, TO_VICT);
      act("&+rS&+Ri&+rn &+Lgets ahold of &N's sight and gazes deep within $S soul.&L&+LSuddenly all $S past sins seem to catch up to $M!&n", FALSE, ch, 0, victim, TO_NOTVICT);

      StopCasting(victim);
      if (IS_FIGHTING(victim))
        stop_fighting(victim);

      bzero(&af, sizeof(af));
      af.type = SPELL_MAJOR_PARALYSIS;
      af.flags = AFFTYPE_SHORT;
      af.duration = WAIT_SEC * 10;
      af.bitvector2 = AFF2_MAJOR_PARALYSIS;
      affect_to_char(victim, &af);
      CharWait(victim, af.duration);

      return TRUE;
    }
    return FALSE;
}
