/*
   ***************************************************************************
   *  File: specials.c                                         Part of Duris *
   *  Usage: support functions for special procedures                          *
   *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   *************************************************************************** 
 */

#include <stdio.h>
#include <strings.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "damage.h"
#include "map.h"

/*
   external variables 
 */

extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct zone_data *zone_table;
extern struct str_app_type str_app[];

P_nevent  get_scheduled(P_char ch, event_func func);

/*
   handle EVENT_FIRE_PLANE events.
   called from various places to start things, and from Events to schedule
   the next one. 
 */
void event_firesector(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type *af, *next;

  if( IS_TRUSTED(ch) || ch->in_room == NOWHERE
    || ((world[ch->in_room].sector_type != SECT_FIREPLANE)
    && (world[ch->in_room].sector_type != SECT_LAVA)
    && (world[ch->in_room].sector_type != SECT_UNDRWLD_LIQMITH)) )
    return;

  if( ENJOYS_FIRE_DAM(ch) )
  {
    if( GET_HIT(ch) < GET_MAX_HIT(ch) )
    {
      GET_HIT(ch) += 3;
      add_event(event_firesector, 3, ch, 0, 0, 0, 0, 0);
    }
    return;
  }

  if (IS_AFFECTED(ch, AFF_PROT_FIRE))
  {
    for (af = ch->affected; af; af = next)
    {
      next = af->next;
      if (af->type == SPELL_PROTECT_FROM_FIRE || af->type == SPELL_FIRE_WARD)
      {
        send_to_char("Your feeble spell is no match for elemental fire!\r\n", ch);
        affect_remove(ch, af);
      }
    }
    return;
  }

  if( GET_HIT(ch) < -7 )
  {
    send_to_char("Failing to bear the &+Reternal heat&n of this place, your body is devoured by &+Yflames&n..\r\n", ch);
    act("Failing to bear the &+Reternal heat&n of this place, $n's body is devoured by &+Yflames&n..",
      FALSE, ch, 0, 0, TO_ROOM);
    die(ch, ch);
    return;
  }
  else
  {
    GET_HIT(ch) -= 3;
    StartRegen(ch, EVENT_HIT_REGEN);
    // Doesn't matter if PC, if they have a descriptor, show the burn!
    if( ch->desc )
      ch->desc->prompt_mode = TRUE;
  }

  add_event(event_firesector, 3, ch, 0, 0, 0, 0, 0);
}

void firesector(P_char ch)
{
  if (IS_TRUSTED(ch) || GET_RACE(ch) == RACE_F_ELEMENTAL ||
      ch->in_room == NOWHERE || (IS_NPC(ch) && !IS_PC_PET(ch)) ||
      ((world[ch->in_room].sector_type != SECT_FIREPLANE) &&
       (world[ch->in_room].sector_type != SECT_LAVA) &&
       (world[ch->in_room].sector_type != SECT_UNDRWLD_LIQMITH)))
    return;

  if (!get_scheduled(ch, event_firesector))
    add_event(event_firesector, 3, ch, 0, 0, 0, 0, 0);
}

void event_underwatersector(P_char ch, P_char victim, P_obj obj, void *data)
{
  if (IS_AFFECTED(ch, AFF_WATERBREATH) || (ch->in_room == -1) ||
      !IS_UNDERWATER(ch))
  {
    REMOVE_BIT(ch->specials.affected_by2, AFF2_HOLDING_BREATH);
    REMOVE_BIT(ch->specials.affected_by2, AFF2_IS_DROWNING);
    return;
  }

  if (IS_AFFECTED2(ch, AFF2_IS_DROWNING))
  {
    if (GET_HIT(ch) < 0)
    {
      send_to_char
        ("Failing to hold your breath any second longer, you draw an airless breath..\r\n",
         ch);
      act
        ("Failing to hold $s breath any second longer, $n draws an airless breath..",
         FALSE, ch, 0, 0, TO_ROOM);
      die(ch, ch);
      return;
    }
    else
    {
      GET_HIT(ch) -= 3;
      StartRegen(ch, EVENT_HIT_REGEN);
    }

    if( ch->desc )
      ch->desc->prompt_mode = TRUE;
  }
  else if (IS_AFFECTED2(ch, AFF2_HOLDING_BREATH))
  {
    send_to_char("&+BYour lungs begin to burn!&n\r\n", ch);
    REMOVE_BIT(ch->specials.affected_by2, AFF2_HOLDING_BREATH);
    SET_BIT(ch->specials.affected_by2, AFF2_IS_DROWNING);
  }
  else
  {
    SET_BIT(ch->specials.affected_by2, AFF2_HOLDING_BREATH);
    send_to_char("&+BYou begin to hold your breath...&n\r\n", ch);
    add_event(event_underwatersector, 3 * GET_C_CON(ch), ch, 0, 0, 0, 0, 0);
    return;
  }

  add_event(event_underwatersector, 3, ch, 0, 0, 0, 0, 0);
}

void underwatersector(P_char ch)
{
  if ((IS_NPC(ch) && !IS_PC_PET(ch)) || !IS_UNDERWATER(ch) ||
      IS_TRUSTED(ch) || IS_AFFECTED(ch, AFF_WATERBREATH))
    return;

  if (!get_scheduled(ch, event_underwatersector))
    add_event(event_underwatersector, 3, ch, 0, 0, 0, 0, 0);
}

void swimming_char(P_char ch)
{
  int      swim_timer = 0;

  if (!ch)
  {
    logit(LOG_DEBUG, "NULL ch in call to swimming_char");
    return;
  }

  return;
  if (IS_TRUSTED(ch) || IS_NPC(ch) || ch->specials.z_cord > 0 ||
      IS_AFFECTED(ch, AFF_LEVITATE) || IS_AFFECTED(ch, AFF_FLY)
      || !IS_MAP_ROOM(ch->in_room) || (ch->in_room == NOWHERE) ||
      !IS_WATER_ROOM(ch->in_room))
  {
    REMOVE_BIT(ch->specials.affected_by3, AFF3_SWIMMING);
    REMOVE_BIT(ch->specials.affected_by2, AFF2_IS_DROWNING);
    REMOVE_BIT(ch->specials.affected_by2, AFF2_HOLDING_BREATH);

    return;
  }

  if (GET_STAT(ch) == STAT_DEAD)
  {
    logit(LOG_EXIT, "assert: dead char in call to swimming_char");
    raise(SIGSEGV);
  }

/*  if (current_event && (current_event->type == EVENT_SWIMMING)) {
      if (GET_VITALITY(ch) < 1) {
	if (IS_WATER_ROOM(ch->in_room)) {
          ch->specials.z_cord = -1;
          underwatersector(ch);
          REMOVE_BIT(ch->specials.affected_by3, AFF3_SWIMMING);
        }
      } else {
	GET_VITALITY(ch) -= (4 - (BOUNDED(0, (GET_CHAR_SKILL(ch, SKILL_SWIM) / 25), 4)));
      }
      if( ch->desc )
	ch->desc->prompt_mode = TRUE;

      StartRegen(ch, EVENT_MOVE_REGEN);
  } else {
    FIND_EVENT_TYPE(e1, EVENT_SWIMMING)
	if (ch == (P_char) e1->actor.a_ch)
      return;
  }

    if (!IS_AFFECTED3(ch, AFF3_SWIMMING) && !IS_AFFECTED2(ch, AFF2_HOLDING_BREATH)
	&& !IS_AFFECTED2(ch, AFF2_IS_DROWNING)) {
      SET_BIT(ch->specials.affected_by3, AFF3_SWIMMING);
      send_to_char("&+BYou begin to swim...&n\r\n", ch);
    }

  if (ch->in_room == NOWHERE || !IS_WATER_ROOM(ch->in_room)) {
    if (IS_AFFECTED2(ch, AFF2_HOLDING_BREATH))
      REMOVE_BIT(ch->specials.affected_by2, AFF2_HOLDING_BREATH);
    if (IS_AFFECTED2(ch, AFF2_IS_DROWNING))
      REMOVE_BIT(ch->specials.affected_by2, AFF2_IS_DROWNING);
    if (IS_AFFECTED3(ch, AFF3_SWIMMING))
      REMOVE_BIT(ch->specials.affected_by3, AFF3_SWIMMING);
    return;

  }

  swim_timer = GET_C_CON(ch) / 5;
  AddEvent(EVENT_SWIMMING, swim_timer, TRUE, ch, 0);*/
}


int OutlawAggro(struct char_data *ch, const char *foo)
{
#if 0
  struct char_data *tch;
  char     Gbuf4[MAX_STRING_LENGTH];
  int      flag_lvl;

  /*
     Heh, just for fun added this to proc.. (have a fun, people. :) 
   */
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    if (CAN_SEE(ch, tch))
      if ((CHAR_IS_FLAGGED(tch) == 4) || (CHAR_IS_FLAGGED(tch) == 3))
      {
        flag_lvl = CHAR_IS_FLAGGED(tch);
        sprintf(Gbuf4, foo,
                (flag_lvl == 4) ? "Outcast" :
                (flag_lvl == 2) ? "Outlaw" :
                (flag_lvl == 3) ? "Killer" : "Thief");
        act(Gbuf4, FALSE, ch, 0, 0, TO_ROOM);
        MobStartFight(ch, tch);
        return TRUE;
      }
#endif
  return FALSE;
}

long pow10(long x)
{
  int      y = 1;

  while (x-- > 0)
    y *= 10;

  return y;
}

void npc_steal(P_char ch, P_char vict)
{
  P_obj    obj = NULL, next_obj = NULL;
  int      percent, roll, loc, gold, chance;
  bool     failed, caught;

  if( !IS_ALIVE(ch) || !IS_ALIVE(vict) || IS_NPC(vict) )
  {
    return;
  }

  // Teachers don't steal.
  if( IS_SET(ch->specials.act, ACT_TEACHER) || IS_SET(ch->specials.act, ACT_SPEC_TEACHER) )
  {
    return;
  }

  if( IS_TRUSTED(vict) || IS_SET(world[ch->in_room].room_flags, SAFE_ZONE) )
  {
    return;
  }

  if( !number(0, 1) )
  {                             /* 50/50, items or coins */
    if( (IS_RIDING(ch)) || ((IS_CARRYING_N(ch) + 1) > CAN_CARRY_N(ch))
      || (CHAR_IN_SAFE_ZONE(ch)) || ((world[ch->in_room].room_flags & SINGLE_FILE)
      && !AdjacentInRoom(ch, vict)) || IS_FIGHTING(ch) || IS_FIGHTING(vict))
    {
      return;
    }

    if( IS_DESTROYING(ch) || IS_DESTROYING(vict) )
    {
      return;
    }

    chance = ((2 * GET_LEVEL(vict) + GET_LEVEL(vict) / 4) - GET_LEVEL(ch));
    if( chance == 0 )
    {
      chance = 1;
    }
    percent = (2 * GET_LEVEL(ch) * 100) / chance;
    percent += number(1, 20);
    percent -= (STAT_INDEX(GET_C_WIS(vict)) + STAT_INDEX(GET_C_INT(vict))) - 19;

    if( IS_AFFECTED(vict, AFF_AWARE) || affected_by_spell(vict, SKILL_AWARENESS) )
    {
      percent -= 100;
    }
    if( !CAN_SEE(vict, ch) )
    {
      percent += 40;
    }
    if( IS_IMMOBILE(vict) )
    {
      percent += 200;           /* ALWAYS SUCCESS */
    }
    else if( IS_AFFECTED2(vict, AFF2_STUNNED) )
    {
      percent += 20;            /* nice bonus if target is stunned */
    }
    else if( GET_STAT(vict) == STAT_SLEEPING )
    {
      percent += 40;            /* hefty bonus if just normal sleeping */
    }
    if( IS_TRUSTED(vict) )      /* NO NO to gods  */
    {
      percent = 0;              /* Failure */
    }

    roll = number(1, 100);
    caught = FALSE;
    failed = FALSE;
    loc = number(0, WEAR_QUIVER);
    if( loc && !vict->equipment[loc] )
    {
      send_to_char("You cannot resist searching for items, yet you find nothing of interest!\r\n", ch);
      failed = TRUE;
      percent += 50;
    }
    else
    {
      if( !loc )
      {
        for (obj = vict->carrying; obj; obj = next_obj)
        {
          next_obj = obj->next_content;
          if (obj->type == ITEM_CONTAINER && ItemsIn(obj) >= 1)
            break;
          else if ((obj->weight <= 120) && (RateObject(ch, 0, obj) >= 0))
            break;
        }
        if (!obj)
        {
          send_to_char("Hmm, not much there. Try a different pocket.\r\n", ch);
          return;
        }
      }
      else
        obj = vict->equipment[loc];
      // No stealing artifacts.
      if( (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch) || IS_ARTIFACT(obj) )
        failed = TRUE;
      if( !failed && (percent > 175) )
      {
        act("You suddenly feel like relieving $N of $S spare equipment.. ",
            FALSE, ch, obj, vict, TO_CHAR);
        act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
        if( loc )
        {
          obj = unequip_char(vict, loc);
          if( IS_ARTIFACT( obj ) )
          {
            statuslog( MINLVLIMMORTAL, "npc_steal: %s &+Rjust stole&n '%s' %d from %s.",
              OBJ_SHORT(obj), OBJ_VNUM(obj), J_NAME(vict) );
            // Don't need this; it's handled in unequip_char & obj_to_char.
            //   artifact_update_location_sql( obj );
          }
          obj_to_char(obj, ch);
        }
        else
        {
          obj_from_char(obj);
          obj_to_char(obj, ch);
        }
        /* success, but heavy stuff increases chance of getting caught */
        percent -= GET_OBJ_WEIGHT(obj);
      }
      else
      {
        send_to_char
          ("Uh huh.. You think your instincts got better of you!\r\n", ch);
        failed = TRUE;
        caught = TRUE;
      }
    }
    CharWait(ch, 24);

    if ((percent < 0) || MIN(100, percent) < number(-60, 100))
      caught = TRUE;
    if (!caught)
    {
      if (!failed)
        send_to_char("Heh heh, got away clean too!\r\n", ch);
      else
        send_to_char("Well, at least nobody saw that!\r\n", ch);
      return;
    }
    if (GET_STAT(vict) == STAT_SLEEPING)
    {
      send_to_char("Groping fingers disturb your rest!\r\n", vict);
      send_to_char
        ("Uh oh, looks like you weren't quite as careful as you should have been!\r\n",
         ch);
      do_wake(vict, 0, 0);
    }
    else
    {
      /* they are awake, and just caught the felonious miscreant in the act! */
      send_to_char("Ooops, better be more careful next time!\r\n", ch);
    }
    if (!failed)
    {
      act("&+WHey! $n just stole your $q!&n", FALSE, ch, obj, vict, TO_VICT);
      act("$n just stole $p from $N!", TRUE, ch, obj, vict, TO_NOTVICT);
    }
    else if (obj)
    {
      act("&+WHey! $n just tried to steal your $q!&n", FALSE, ch, obj, vict,
          TO_VICT);
      act("$n just tried to steal something from $N!", TRUE, ch, obj, vict,
          TO_NOTVICT);
    }
  }
  if (IS_AWAKE(vict) &&
      StatSave(vict, APPLY_DEX, (GET_LEVEL(vict) - GET_LEVEL(ch)) / 6))
  {                             /*(number(0, GET_LEVEL(vict)) > GET_LEVEL(ch)) */
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0,
        vict, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    /* Steal some coins */
    gold = (int) ((GET_PLATINUM(vict) * number(1, 10)) / 100);
    if (gold > 0)
    {
      GET_PLATINUM(ch) += gold;
      GET_PLATINUM(vict) -= gold;
    }
    gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
    if (gold > 0)
    {
      GET_GOLD(ch) += gold;
      GET_GOLD(vict) -= gold;
    }
    gold = (int) ((GET_SILVER(vict) * number(1, 10)) / 100);
    if (gold > 0)
    {
      GET_SILVER(ch) += gold;
      GET_SILVER(vict) -= gold;
    }
    gold = (int) ((GET_COPPER(vict) * number(1, 10)) / 100);
    if (gold > 0)
    {
      GET_COPPER(ch) += gold;
      GET_COPPER(vict) -= gold;
    }
  }
}

#if 0
/*
   A special for each tower roof in Anapest (room-based)

   int tower(int room, P_char ch, int cmd, char *arg)
   {
   if (!cmd) {
   if....
   act("There is activity along the valley rim.\r\n", ....);
   } else if (cmd==...look...) {
   if (*arg) {
   number = search_block(arg, t_skills, FALSE);
   if (number == -1) {

   ..nasty...may need to change call with self reference like w/mobs 
 */

#endif

/* Pi's room proc */

P_obj find_key(P_char ch, int key)
{
  P_obj    o, tar_obj = NULL;

  if (!has_key(ch, key))
    return (0);

  for (o = ch->carrying; o; o = o->next_content)
    if (obj_index[o->R_num].virtual_number == key)
      tar_obj = o;
  if (ch->equipment[HOLD])
    if (obj_index[ch->equipment[HOLD]->R_num].virtual_number == key)
      tar_obj = ch->equipment[HOLD];
  return (tar_obj);
}

/**
 * For negative plane events, similar to event_firesector. -Keja
 */
void event_negsector(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type *af, *next;

  if (IS_TRUSTED(ch) || GET_RACE(ch) == RACE_UNDEAD ||
      ch->in_room == NOWHERE ||
      ((world[ch->in_room].sector_type != SECT_NEG_PLANE)))
    return;

  if (IS_AFFECTED5(ch, AFF5_PROT_UNDEAD))
  {
    for (af = ch->affected; af; af = next)
    {
      next = af->next;
      if (af->type == SPELL_PROT_FROM_UNDEAD)
      {
        send_to_char("Your feeble spell is no match for the negative energy!\r\n",
                     ch);
        affect_remove(ch, af);
      }
    }
    return;
  }

  if (GET_HIT(ch) < 0)
  {
    send_to_char
      ("&+LYou gasp as you realize your lifeforce has run out!&n.\r\n",
       ch);
    act
      ("$n&+L collapses in a crumpled heap, as their body shrivels into nothing.\r\n",
       FALSE, ch, 0, 0, TO_ROOM);
    die(ch, ch);
    return;
  }
  else
  {
    GET_HIT(ch) -= 3;
    StartRegen(ch, EVENT_HIT_REGEN);
    if( ch->desc )
      ch->desc->prompt_mode = TRUE;
  }

  add_event(event_negsector, 3, ch, 0, 0, 0, 0, 0);
}

void negsector(P_char ch)
{
  if (IS_TRUSTED(ch) || GET_RACE(ch) == RACE_UNDEAD ||
      ch->in_room == NOWHERE || (IS_NPC(ch) && !IS_PC_PET(ch)) ||
      ((world[ch->in_room].sector_type != SECT_NEG_PLANE)))
    return;

  if (!get_scheduled(ch, event_negsector))
    add_event(event_negsector, 3, ch, 0, 0, 0, 0, 0);
}

