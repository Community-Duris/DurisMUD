
/*
   ***************************************************************************
   *  File: new_skills.c                                       Part of Duris *
   *  Usage: This needs a better name (and organization) -JAB                *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "justice.h"
#include "damage.h"
#include "guard.h"

/*
   external variables
 */

extern P_room world;
extern char *dirs[];
extern const struct stat_data stat_factor[];
extern int innate_abilities[];
extern int class_innates[][5];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern struct str_app_type str_app[];
extern const int numCarvables;
extern const char *carve_part_name[];
extern const int carve_part_flag[];
extern P_obj object_list;
extern P_index obj_index;
extern P_char character_list;


/*
   This func contains collection of those basic checks that all
   fighting-related commands should do. Collected here so I need
   not change all zillion places if I want to fix change something.
   -Torm
 */

int CanDoFightMove(P_char ch, P_char victim)
{
  if( !ch || !victim )
    return FALSE;

  if( victim == ch )
  {
    send_to_char("Aren't we funny today...\r\n", ch);
    return FALSE;
  }

  if( !IS_ALIVE(victim) )
  {
    debug("CanDoFightMove in new_skills.c called on dead victim '%s'.", victim ? J_NAME(victim) : "NULL" );
    send_to_char("What?  Dead isn't good enough?  Leave that corpse alone!\r\n", ch);
    return FALSE;
  }

  if( !IS_ALIVE(ch) )
  {
    debug("CanDoFightMove in new_skills.c called on by dead ch '%s'.", ch ? J_NAME(ch) : "NULL" );
    return FALSE;
  }

  if( !AWAKE(ch) )
  {
    send_to_char("You can't do that if you're not awake!\r\n", ch);
    return FALSE;
  }

  if( affected_by_spell(ch, SONG_PEACE) )
  {
    send_to_char("You feel way too peaceful to consider doing anything offensive!\r\n", ch);
    return FALSE;
  }

  if(CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("You feel ashamed trying to disrupt the tranquility of this place.\r\n", ch);
    return FALSE;
  }

  if( GET_MASTER(ch) == victim && GET_MASTER(ch) )
  {
    act("$N is just such a good friend, you simply can't harm $M.", FALSE, ch, 0, victim, TO_CHAR);
    return FALSE;
  }

  if( P_char mount = get_linked_char(ch, LNK_RIDING) )
  {
    if( !GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT ) /*&& !is_natural_mount(ch, mount)*/)
    {
      send_to_char("While mounted? I don't think so...\r\n", ch);
      return FALSE;
    }
  }

  if( get_linking_char(ch, LNK_RIDING) == victim )
  {
    send_to_char("You can't harm your rider.\r\n", ch);
    return FALSE;
  }

  if( (world[ch->in_room].room_flags & SINGLE_FILE) && !AdjacentInRoom(ch, victim) )
  {
    act("$N seems to be just a BIT out of reach.", FALSE, ch, 0, victim, TO_CHAR);
    return FALSE;
  }

  if( !CAN_SEE(ch, victim) )
  {
    send_to_char("Um.. you don't see any such target here?\r\n", ch);
    return FALSE;
  }

  if( affected_by_spell(ch, SKILL_GAZE) )
  {
    send_to_char("You are too petrified with fear to try that.\r\n", ch);
    return FALSE;
  }

  if( IS_AFFECTED2(ch, AFF2_STUNNED) )
  {
    send_to_char("You're too stunned to contemplate that!\r\n", ch);
    return FALSE;
  }

  if( IS_AFFECTED(ch, AFF_KNOCKED_OUT) )
  {
    send_to_char("You can't do much of anything while knocked out!\r\n", ch);
    return FALSE;
  }

  if( IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) || IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS) )
  {
    send_to_char("It's tough to do anything while paralyzed.\r\n", ch);
    return FALSE;
  }

  if( IS_AFFECTED(ch, AFF_BOUND) )
  {
    send_to_char("You're too bound to do that.\r\n", ch);
    return FALSE;
  }

  if( IS_AFFECTED5(ch, AFF5_NOT_OFFENSIVE) )
  {
    send_to_char("Since you are not being offensive... try something else.\r\n", ch);
    return false;
  }

  if( IS_DESTROYING(ch) )
  {
    send_to_char( "Not while you're destroying an object.\n", ch );
    return FALSE;
  }

  return TRUE;
}

/*
   Generic function, but initially created for Shadow skill.

   * This function will recursively count the number of followers
   * effectively following 'ch'.  This means that 'ch' may have 3 chars
   * following him/her, but also, each of those chars have some followers.
   * The total number in the "tree" is returned.
   *
   *   --TAM 7-6-94
 */

int CountNumFollowers(P_char ch)
{
  struct follow_type *f;
  int      count = 0;

  f = ch->followers;

  if (!f)
  {
    return 0;
  }
  else
  {
    while (f)
    {
      ++count;
      count += CountNumFollowers(f->follower);
      f = f->next;
    }
  }

  return count;
}

int CountNumGreaterElementalFollowersInSameRoom(P_char ch)
{
  struct char_link_data *cld;
  P_char follower;
  int i = 0;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return 0;
  }

  for (cld = ch->linked; cld; cld = cld->next_linked)
  {
    follower = cld->linking;
    
    if(cld->type != LNK_PET)
    {
      continue;
    }
    
    if(GET_LEVEL(follower) > 45 &&
       IS_NPC(follower) &&
       IS_PC_PET(follower) &&
       IS_ELEMENTAL(follower))
    {
      i++;
    }
  }
  return i;
}

/* Awareness skill for assassins and rangers.   --TAM 7-8-94 */

void do_awareness(P_char ch, char *argument, int cmd)
{
  struct affected_type af, *af_ptr;
  int      duration = 0, lev_aware;
  int      skl_lvl = 0;

  if (IS_PC(ch))
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_AWARENESS);

  if (!skl_lvl)
  {
    act("You wouldn't know where to begin.", TRUE, ch, 0, 0, TO_CHAR);
    return;
  }
  act("You feel aligned with your surroundings.", TRUE, ch, 0, 0, TO_CHAR);

  lev_aware =
    STAT_INDEX(GET_C_INT(ch)) + STAT_INDEX(GET_C_DEX(ch)) +
    (GET_LEVEL(ch) >> 3);
  lev_aware *= (skl_lvl / 75);
  if (lev_aware > 0)
  {
    lev_aware = number((lev_aware >> 1), lev_aware);
  }
  duration = (int) ((1 + (GET_LEVEL(ch) >> 3) * skl_lvl / 75));
  duration *= STAT_INDEX(GET_C_INT(ch)) / 9;    /*
                                                   lower than 13 and player
                                                   suffers 'lack of attn span'
                                                   penalty
                                                 */
  if (duration > 0)
  {
    duration = number(1, duration);
  }
  bzero(&af, sizeof(af));
  af.type = SKILL_AWARENESS;
  af.duration = duration;
  af.modifier = lev_aware;
  af.bitvector = AFF_SKILL_AWARE;
  af.flags = AFFTYPE_NODISPEL;

  if (IS_AFFECTED(ch, AFF_SKILL_AWARE))
  {
    for (af_ptr = ch->affected; af_ptr; af_ptr = af_ptr->next)
    {
      if (af_ptr->type == SKILL_AWARENESS)
      {
        break;
      }
    }
    if (af_ptr)
      affect_remove(ch, af_ptr);
    REMOVE_BIT(ch->specials.affected_by, AFF_SKILL_AWARE);
  }
  affect_to_char(ch, &af);
  notch_skill(ch, SKILL_AWARENESS, 20);
}

int wornweight(P_char ch)
{
  int      enc;
  int      i;

  enc = 0;
  for (i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      enc += (GET_OBJ_WEIGHT(ch->equipment[i]));
  return enc;
}

int MonkAcBonus(P_char ch)
{
  int      b;

  if(GET_CLASS(ch, CLASS_MONK))
  {
    /*
       base bonus level * 2
     */
    b = -(4 * GET_LEVEL(ch));
    // Bonus based on martial arts skill
    b -= GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS);
    /*
       4x penalty for encumbering worn items with a small allowance for low levels
     */
    b += MAX(0, (3 * (wornweight(ch) - ((60 - GET_LEVEL(ch)) / 6))));

    return BOUNDED(-400, b, 50);
  }
  else
    return 0;
}

void MonkSetSpecialDie(P_char ch)
{
  if(IS_PC(ch) &&
     (ch->equipment[WIELD] ||
      ch->equipment[WEAR_SHIELD] ||
      ch->equipment[HOLD] ||
      ch->equipment[SECONDARY_WEAPON]))
  {
    ch->points.damnodice = 1;
    ch->points.damsizedice = 1;
    return;
  }
  switch (GET_LEVEL(ch))
  {
    case 1:
    case 2:
    case 3:
      ch->points.damnodice = 2;
      ch->points.damsizedice = 5;
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      ch->points.damnodice = 2;
      ch->points.damsizedice = 6;
      break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      ch->points.damnodice = 2;
      ch->points.damsizedice = 8;
      break;
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
      ch->points.damnodice = 4;
      ch->points.damsizedice = 5;
      break;
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
      ch->points.damnodice = 5;
      ch->points.damsizedice = 5;
      break;
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
      ch->points.damnodice = 5;
      ch->points.damsizedice = 6;
      break;
    case 35:
    case 36:
    case 37:
    case 38:
      ch->points.damnodice = 5;
      ch->points.damsizedice = 7;
      break;
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
      ch->points.damnodice = 6;
      ch->points.damsizedice = 6;
      break;
    case 45:
    case 46:
    case 47:
    case 48:
    case 49:
      ch->points.damnodice = 6;
      ch->points.damsizedice = 7;
      break;
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
      ch->points.damnodice = 7;
      ch->points.damsizedice = 7;
      break;
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
    case 61:
    case 62:
      ch->points.damnodice = 8;
      ch->points.damsizedice = 8;
      break;
    default:
      ch->points.damnodice = 1;
      ch->points.damsizedice = 4;
      break;
  }
}

int MonkDamage(P_char ch)
{
  int      dam;
  int      skl_lvl = 0;


  if (IS_PC(ch))
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS);

  if (GET_CLASS(ch, CLASS_MONK))
  {
    MonkSetSpecialDie(ch);
  }
  dam = dice(ch->points.damnodice, ch->points.damsizedice);
  dam += skl_lvl / 11;

  if (GET_CLASS(ch, CLASS_MONK))
  {
    // Weight factor affected by lvl and str.
    int weight = wornweight(ch) - (GET_LEVEL(ch) * GET_C_STR(ch))/100;
    // Does at least 1 damage, and at most dam damage.
    dam = BOUNDED(1, dam - weight, dam);
  }
  return dam;
}

int MonkNumberOfAttacks(P_char ch)
{
  int      a, l;
  int      skl_lvl = 0;

  /*
     simplified combat routine, return 1 if not a monk
   */
/*  if (IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM))
      return 2 + number(0,1);*/// handled elsewhere
  if (!GET_CLASS(ch, CLASS_MONK))
    return 1;

  /*
     monks without both hands free get 1 attack, period
   */
  if (IS_PC(ch) && (ch->equipment[WIELD] || ch->equipment[SECONDARY_WEAPON] ||
                    ch->equipment[HOLD] || ch->equipment[WEAR_SHIELD] ||
                    ch->equipment[WEAR_LIGHT]))
    return 1;
  if (IS_PC(ch))
  {
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS);
    notch_skill(ch, SKILL_MARTIAL_ARTS, 1);
  }
  else
    skl_lvl = BOUNDED(5, GET_LEVEL(ch) * 2, 100);


  for (a = 0, l = ((4 * GET_LEVEL(ch)) / number(25, 38));
       (a < 6) && (l > 0); l--)
    if (number(1, 100) < skl_lvl)
      a++;
  if (GET_LEVEL(ch) >= 51)
    if (GET_LEVEL(ch) >= 56)
      return BOUNDED(1, a + 2, 7);
    else
      return BOUNDED(1, a + 1, 6);

  return BOUNDED(1, a, 5);      /* 1 to 4 */
}

void do_feign_death(P_char ch, char *arg, int cmd)
{
  P_char   t = NULL, t_next, tch;
  int      skl_lvl = 0;
  struct affected_type af;

  if( IS_PC(ch) )
  {
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_FEIGN_DEATH);
  }
  else
  {
    skl_lvl = MIN(100, GET_LEVEL(ch) * 3);
  }

  if( !ch->specials.fighting )
  {
    send_to_char("You are not fighting anything!\r\n", ch);
    return;
  }
  if( !skl_lvl )
  {
    send_to_char("You know not how!\r\n", ch);
    return;
  }
  if (IS_RIDING(ch))
  {
    send_to_char("While mounted? I don't think so...\r\n", ch);
    return;
  }

// Changed this so wear off message works.. might want to redo affect_timer as a whole.
//  if( !affect_timer(ch, WAIT_SEC * get_property("timer.secs.feignDeath", 60), SKILL_FEIGN_DEATH) )
  if( affected_by_spell(ch, SKILL_FEIGN_DEATH ))
  {
    send_to_char("You don't feel up to faking it right now.\r\n", ch);
    return;
  }

  memset(&af, 0, sizeof(af));
  af.type = SKILL_FEIGN_DEATH;
  af.flags = AFFTYPE_STORE | AFFTYPE_SHORT;
  af.duration = WAIT_SEC * get_property("timer.secs.feignDeath", 60);
  af.modifier = 0;
  affect_to_char(ch, &af);


  send_to_char("You try to fake your own demise..\r\n", ch);
  death_cry(ch);
  act("$n is dead! R.I.P.", FALSE, ch, 0, 0, TO_ROOM);
  if (number(1, 101) < (skl_lvl * number(2, 5) / 2))
  {
    stop_fighting(ch);
    if( IS_DESTROYING(ch) )
      stop_destroying(ch);
    for (t = world[ch->in_room].people; t; t = t_next)
    {
      t_next = t->next_in_room;
      if (t->specials.fighting == ch)
      {
        stop_fighting(t);
        if((GET_CLASS(ch, CLASS_NECROMANCER) || GET_CLASS(ch, CLASS_THEURGIST))
          && GET_LEVEL(ch) > 40)
        {
          LOOP_THRU_PEOPLE(tch, ch)
          {
            if(IS_NPC(tch) && HAS_MEMORY(tch))
            {
              debug("FEIGN: (%s) feigned death and mob (%s) removed.", GET_NAME(ch), J_NAME(tch));
              if(!ch || !tch)
                return;
              forget(tch, ch);
            }
          }
        }
        if (number(1, 101) < (skl_lvl * number(2, 5) / 2))
        {
          if(IS_CASTING(t))
            StopCasting(t);
          SET_BIT(ch->specials.affected_by, AFF_HIDE);
        }
        SET_POS(ch, POS_PRONE + STAT_RESTING);  // was SLEEPING..
      }
    }
    CharWait(ch, PULSE_VIOLENCE * 2);
    notch_skill(ch, SKILL_FEIGN_DEATH, 40);
    return;
  }
  else
  {
    SET_POS(ch, POS_PRONE + STAT_RESTING);      // ditto
    notch_skill(ch, SKILL_FEIGN_DEATH, 40);
    CharWait(ch, PULSE_VIOLENCE * 3);
  }
}

void do_first_aid(P_char ch, char *arg, int cmd)
{
  struct affected_type af;
  int      skl_lvl = 0, bonus;

  if (IS_NPC(ch))
    return;

send_to_char
      ("Try using the bandage instead.",
       ch);
return;

  if (has_innate(ch, INNATE_BATTLEAID))
  {
    bonus = dice(10, 10);
    send_to_char
      ("Your increased knowledge helps you apply more efficent first aid.",
       ch);
  }
  else
  {
    bonus = 0;
  }

  skl_lvl = GET_CHAR_SKILL(ch, SKILL_FIRST_AID);

  send_to_char("You attempt to render first aid unto yourself..\r\n", ch);

  if (affected_by_spell(ch, SKILL_FIRST_AID))
  {
    send_to_char("You can only do this once per day..\r\n", ch);
    return;
  }
  if (GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    GET_HIT(ch) += (MAX(5, skl_lvl / 3) + bonus);
    if (GET_HIT(ch) > GET_MAX_HIT(ch))
      GET_HIT(ch) = GET_MAX_HIT(ch);
  }
  notch_skill(ch, SKILL_FIRST_AID, 17);

  bzero(&af, sizeof(af));
  af.duration = 24;
  af.type = SKILL_FIRST_AID;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_PERM | AFFTYPE_NODISPEL;
  affect_to_char(ch, &af);
}

void chant_calm(P_char ch, char *argument, int cmd)
{
  P_char   d = NULL;
  int      skl_lvl = 0;

  if (!GET_CLASS(ch, CLASS_MONK) && !IS_TRUSTED(ch))
  {
    send_to_char("Just breathe deeply, will calm you right down.\r\n", ch);
    return;
  }

  if (IS_PC(ch))
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_CALM);
  else
    skl_lvl = MAX(100, GET_LEVEL(ch) * 3);

  send_to_char("You chant for peace and happiness for all.\r\n", ch);

  for (d = world[ch->in_room].people; d; d = d->next_in_room)
  {
    if (d->specials.fighting)
    {
      if (notch_skill(ch, SKILL_CALM, 10) || number(1, 130) < skl_lvl )
      {
        if(!IS_GREATER_RACE(d) &&
           !IS_ELITE(d))
        {
          stop_fighting(d);
          clearMemory(d);
          send_to_char("A sense of calm comes upon you.\r\n", d);
        }
      }
    }
    else if( IS_DESTROYING(d) )
      stop_destroying(d);
  }
  CharWait(ch, PULSE_VIOLENCE);
}

void chant_diamond_soul(P_char ch, char *argument, int cmd)
{
  struct affected_type af;
  char     buf[100];
  int      skl_lvl = 0;
  int duration = MAX(2, (GET_LEVEL(ch) / 16)  + 2);

  if (!GET_CLASS(ch, CLASS_MONK) && !IS_TRUSTED(ch))
  {
    send_to_char("Try becoming a monk first.\r\n", ch);
    return;
  }

  if (!affect_timer(ch,
	WAIT_SEC * get_property("timer.secs.monkDiamond", 120),
	SKILL_DIAMOND_SOUL))
  {
    send_to_char("Your mind needs rest...\r\n", ch);
    return;
  }

  if(IS_PC(ch) ||
     IS_PC_PET(ch))
        skl_lvl = GET_CHAR_SKILL(ch, SKILL_DIAMOND_SOUL);
  else
    skl_lvl = GET_LEVEL(ch) * 2;

  if (affected_by_spell(ch, SKILL_DIAMOND_SOUL))
  {
    send_to_char("You are already under the affects of diamond soul.\r\n", ch);
    return;
  }
  
  if (affected_by_spell(ch, SKILL_HEROISM))
  {
    send_to_char("You cannot call upon the diamond soul technique while under the influence of heroism.\r\n", ch);
    return;
  }
    
  if( !notch_skill(ch, SKILL_DIAMOND_SOUL, 30)
    && number(1, 105) > skl_lvl ) // 5 percent chance to fail at max pc skill.
  {
    send_to_char("Your inner thoughts are in turmoil.\r\n", ch);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }
  
  sprintf(buf, "You feel your magical resistance harden like a diamond.\r\n");
  bzero(&af, sizeof(af));
  af.type = SKILL_DIAMOND_SOUL;
  af.flags = AFFTYPE_NODISPEL;
  af.duration = duration;
  
  af.modifier = -(MAX(2, (int) (GET_LEVEL(ch) / 8)));
  af.location = APPLY_SAVING_SPELL;
  affect_to_char(ch, &af);

  af.modifier = -(MAX(2, (int) (GET_LEVEL(ch) / 8)));
  af.location = APPLY_SAVING_PARA;
  affect_to_char(ch, &af);

  send_to_char(buf, ch);

  CharWait(ch, PULSE_VIOLENCE);
}

void chant_heroism(P_char ch, char *argument, int cmd)
{
  struct affected_type af, af1;
  char     buf[100];
  int      skl_lvl = 0;
  int duration = MAX(5, (GET_LEVEL(ch) / 4)  + 2);

  if( !GET_CLASS(ch, CLASS_MONK) && !IS_TRUSTED(ch) )
  {
    send_to_char("You're no hero - you're a jackass.\r\n", ch);
    return;
  }

  if( !affect_timer(ch, WAIT_SEC * get_property("timer.secs.monkHeroism", 120), SKILL_HEROISM) )
  {
    send_to_char("Your mind needs rest...\r\n", ch);
    return;
  }

  if( IS_PC(ch) || IS_PC_PET(ch) )
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_HEROISM);
  else
    skl_lvl = GET_LEVEL(ch) * 2;

  if( affected_by_spell(ch, SKILL_HEROISM) )
  {
    send_to_char("You are already under the affects of heroism.\r\n", ch);
    return;
  }

  if( affected_by_spell(ch, SKILL_DIAMOND_SOUL) )
  {
    send_to_char("You cannot call upon the heroism technique while under the influence of diamond soul.\r\n", ch);
    return;
  }

  // 5 percent chance to fail at max pc skill.
  if( !notch_skill(ch, SKILL_HEROISM, 30 ) && number(1, 105) > skl_lvl )
  {
    send_to_char("Your inner thoughts are in turmoil.\r\n", ch);
    CharWait(ch, PULSE_VIOLENCE);
    return;
  }

  sprintf(buf, "A sense of heroism grows in your heart.\r\n");

  bzero(&af, sizeof(af));
  af.type = SKILL_HEROISM;
  af.flags = AFFTYPE_NODISPEL;
  af.duration = duration;

  af.modifier = MAX(2, (int) (GET_LEVEL(ch) / 8));
  af.location = APPLY_HITROLL;
  affect_to_char(ch, &af);

  af.modifier = MAX(2, GET_LEVEL(ch) / 8);
  af.location = APPLY_DAMROLL;
  affect_to_char(ch, &af);

  af.modifier = 0;
  af.location = 0;

  if( GET_SPEC(ch, CLASS_MONK, SPEC_WAYOFDRAGON) && !affected_by_spell(ch, SPELL_INDOMITABILITY) )
  {
    send_to_char("Something wicked just happened didn't it? My god you feel weird. \r\n", ch);
    bzero(&af1, sizeof(af1));
    af1.type = SPELL_INDOMITABILITY;
    af1.flags = AFFTYPE_NODISPEL;
    af1.duration = duration;
    affect_to_char(ch, &af1);
  }

  send_to_char(buf, ch);

  if( GET_LEVEL(ch) >= 36 && GET_SPEC(ch, CLASS_MONK, SPEC_WAYOFSNAKE) && !IS_AFFECTED4(ch, AFF4_DAZZLER) )
  {
    af.bitvector4 += AFF4_DAZZLER;
    send_to_char("Your body begins to glow with disorienting colors... \r\n", ch);
  }

  if( GET_LEVEL(ch) >= 51 && !IS_AFFECTED(ch, AFF_HASTE) )
  {
    af.bitvector += AFF_HASTE;
    send_to_char("Your body begins to speed up!\r\n", ch);
  }

  if( GET_LEVEL(ch) >= 56 && !IS_AFFECTED3(ch, AFF3_BLUR) )
  {
    af.bitvector3 += AFF3_BLUR;
    send_to_char("Your body begins to blur!\r\n", ch);
  }

  affect_to_char(ch, &af);
  CharWait(ch, PULSE_VIOLENCE);
}

void chant_buddha_palm(P_char ch, char *argument, int cmd)
{
  P_char   vict = NULL, hold = NULL;
  int      dam, num_tar;
  int      skl_lvl = 0;
  struct damage_messages messages = {
    "$N is wracked by spasms of agony as you reveal Buddha's true greatness!",
    "You are blasted by holy light sent by $n!",
    "$N is burned by brilliant light!",
    "$N dies from revealed trust of Buddha's greatness!",
    "As $n's chant nears it's end, you realize what a jerk you are!",
    "$N dies from $n's chant!"
  };

  if (!GET_CLASS(ch, CLASS_MONK) && !IS_TRUSTED(ch))
  {
    send_to_char("You may not perform the buddha palm chant.\r\n", ch);
    return;
  }

  if (!affect_timer(ch,
        WAIT_SEC * get_property("timer.secs.monkBuddha", 30),
        SKILL_BUDDHA_PALM))
  {
    send_to_char("You are not in the proper mood for that right now!\r\n", ch);
    return;
  }

  if (IS_PC(ch))
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_BUDDHA_PALM);
  else
    skl_lvl = MAX(100, GET_LEVEL(ch) * 3);

  if( !notch_skill(ch, SKILL_BUDDHA_PALM, get_property("skill.notch.offensive", 10))
    && number(1, 101) > skl_lvl)
  {
    send_to_char("You forgot the words for the chant.\r\n", ch);
    CharWait(ch, 2 * PULSE_VIOLENCE);
    return;
  }
  if (CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char
      ("You feel ashamed to try to disrupt the tranquility of this place.\r\n",
       ch);
    return;
  }
  if (EVIL_RACE(ch) || IS_EVIL(ch))
  {
    send_to_char
      ("A glowing globe of red light springs from your outstretched palms.\r\n",
       ch);
    act("A glowing glove of red light springs from $n's outstretched palms.",
        TRUE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    send_to_char
      ("A glowing globe of light springs from your outstretched palms.\r\n",
       ch);
    act("A glowing glove of light springs from $n's outstretched palms.",
        TRUE, ch, 0, 0, TO_ROOM);
  }

  num_tar = GET_LEVEL(ch) / 10;
  dam = dice(GET_LEVEL(ch) / 2, 8);
  for (vict = world[ch->in_room].people; vict; vict = hold)
  {
    hold = vict->next_in_room;
    if (should_area_hit(ch, vict))
    {
      num_tar--;
      if (EVIL_RACE(ch) || IS_EVIL(ch))
      {
        act("You burn $N with your reddish light.",
            FALSE, ch, 0, vict, TO_CHAR);
        act("$n burns $N with $s reddish light.",
            FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n burns you with $s reddish light.",
            FALSE, ch, 0, vict, TO_VICT);
      }
      else
      {
        act("You burn $N with your heavenly starlight.",
            FALSE, ch, 0, vict, TO_CHAR);
        act("$n burns $N with $s heavenly starlight.",
            FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n burns you with $s heavenly starlight.",
            FALSE, ch, 0, vict, TO_VICT);
      }

      spell_damage(ch, vict, dam, SPLDAM_HOLY,
                   SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
      if (num_tar <= 0)
        break;
    }
  }
  CharWait(ch, PULSE_VIOLENCE);
}

void chant_quivering_palm(P_char ch, char *argument, int cmd)
{
  P_char   vict = NULL;
  char     name[256];
  int      dam;
  int      skl_lvl = 0;
  struct damage_messages messages = {
    "$N is wracked by pain after you briefly touch your vibrating palm to him!",
    "$n extends his vibrating hand, and touches you - sending spasms of agony through you.",
    "$n touches $N with his vibrating palm, sending $M into spasms of agony.",
    "$N is killed instantly by the dreaded quivering palm!",
    "As $n touches you, you feel your bones and organs shatter inside.",
    "$N dies as $n touches $M."
  };

  if (!GET_CLASS(ch, CLASS_MONK) && !IS_TRUSTED(ch))
  {
    send_to_char("Too bad you're not a monk.\r\n", ch);
    return;
  }

  if (IS_PC(ch))
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_QUIVERING_PALM);
  else
    skl_lvl = GET_LEVEL(ch) * 2; // Let's try this at a value that makes a bit more sense.
                                 // Level 1 monk mobs should not have 100 skill, duhr.  - Jexni 09/20/08

  if (argument)
    one_argument(argument, name);
  if (!argument || !*argument || !(vict = get_char_room_vis(ch, name)))
  {
    if (ch->specials.fighting &&
        (GET_STAT(ch->specials.fighting) != STAT_DEAD))
      vict = ch->specials.fighting;
  }
  if (!vict || (GET_STAT(vict) == STAT_DEAD))
  {
    send_to_char("Chant on whom?\r\n", ch);
    return;
  }
  if (vict == ch)
  {
    send_to_char("You hum to yourself.\r\n", ch);
    return;
  }
  if (!affect_timer(ch,
        WAIT_SEC * get_property("timer.secs.monkQuivering", 30),
        SKILL_QUIVERING_PALM))
  {
    send_to_char("You are not in the proper mood for that right now!\r\n", ch);
    return;
  }
  if (CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char
      ("You feel ashamed to try to disrupt the tranquility of this place.\r\n",
       ch);
    return;
  }
  if ((IS_SET(world[ch->in_room].room_flags, SINGLE_FILE)) &&
      (!AdjacentInRoom(ch, vict)))
  {
    send_to_char("Your target is too far for your palm to reach!\n", ch);
    return;
  }
  if (!notch_skill(ch, SKILL_QUIVERING_PALM, get_property("skill.notch.offensive", 10))
    && number(1, 100) > skl_lvl)
  {
    send_to_char("You forgot the words for the chant.\r\n", ch);
    CharWait(ch, 2 * PULSE_VIOLENCE);
    return;
  }
  dam = GET_C_DEX(ch) * 2 + GET_CHAR_SKILL(ch, SKILL_QUIVERING_PALM);

  if (GET_CHAR_SKILL(ch, SKILL_ANATOMY) &&
      5 + GET_CHAR_SKILL(ch, SKILL_ANATOMY)/10 > number(0,100)) {
    dam = (int) (dam * 1.5);
  }

  /*  can't for the life of me figure out why we need 2 messages for this skill...
  act
    ("You suddenly grab hold of $N's forehead and sends $M quivering with your chant.",
     FALSE, ch, 0, vict, TO_CHAR);
  act
    ("$n suddenly grabs hold of $N's forehead and sends $M quivering with $s chant.",
     FALSE, ch, 0, vict, TO_NOTVICT);
  act
    ("$n suddenly grabs hold of your forehead and sends you quivering with $s chant.",
     FALSE, ch, 0, vict, TO_VICT); */
  melee_damage(ch, vict, dam, PHSDAM_TOUCH, &messages);
  if (!char_in_list(ch))
    return;
  CharWait(ch, PULSE_VIOLENCE);
}

void chant_jin_touch(P_char ch, char *argument, int cmd)
{
  P_char   vict = NULL;
  char name[256];
  int dam, damdice, percent, skl_lvl;

  if( !IS_ALIVE(ch) || IS_IMMOBILE(ch) )
  {
    return;
  }

  if( !GET_CLASS(ch, CLASS_MONK) )
  {
    send_to_char("Too bad you're not a monk.\r\n", ch);
    return;
  }
  else
  {
    if( IS_PC(ch) || IS_PC_PET(ch) )
      skl_lvl = GET_CHAR_SKILL(ch, SKILL_JIN_TOUCH);
    else
      skl_lvl = MAX(100, GET_LEVEL(ch) * 3);
  }

  //debug("(%s) jin skill is (%d).", GET_NAME(ch), skl_lvl);

  if(IS_FIGHTING(ch))
    vict = ch->specials.fighting;
  else if (argument)
  {
    one_argument(argument, name);
    vict = get_char_room_vis(ch, name);
  }
  
  if(!vict ||
     !IS_ALIVE(vict))
  {
    send_to_char("&+GChant on whom?\r\n", ch);
    return;
  }
  
  if(number(1, 50) > skl_lvl)
  {
    send_to_char("You forgot the words for the chant.\r\n", ch);
    
    CharWait(ch, PULSE_VIOLENCE);

    if(IS_NPC(vict) && CAN_SEE(vict, ch) && number(0, 1))
    {
      remember(vict, ch); 
      if(!IS_FIGHTING(vict) && !IS_DESTROYING(vict))
      {
        MobStartFight(vict, ch);
      }
    }

    return;
  }
  
  CharWait(ch, 2 * PULSE_VIOLENCE);
  
  if (!affect_timer(ch,
        WAIT_SEC * get_property("timer.secs.monkJintouch", 30),
        SKILL_JIN_TOUCH))
  {
    send_to_char("&+GYour inner force has not realigned...\r\n", ch);
    return;
  }

  if (vict == ch)
  {
    send_to_char("&+GYou hum to yourself.\r\n", ch);
    return;
  }
  
  if (CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("&+GYou feel ashamed to try to disrupt the tranquility of this place.\r\n", ch);
    return;
  }
  
  if ((IS_SET(world[ch->in_room].room_flags, SINGLE_FILE)) &&
      (!AdjacentInRoom(ch, vict)))
  {
    send_to_char("&+GYour target is too far for your touch to reach!\n", ch);
    return;
  }

  if (IS_NPC(vict) && CAN_SEE(vict, ch) && number(0, 1))
  {
    remember(vict, ch);
    if (!IS_FIGHTING(vict) && !IS_DESTROYING(vict))
      MobStartFight(vict, ch);
  }
  
  dam = (int)(dice(GET_C_DEX(ch), 3) +
              dice(GET_C_AGI(ch), 3) +
              dice(GET_C_WIS(ch), 3)) / 4;
  
  if(skl_lvl <= 80)
    dam = (int) (0.4 * dam);
  else if(skl_lvl <= 90)
    dam = (int) (0.6 * dam);

   dam /= 2;
    
  act("&+CYou harness your full Jin, and deliver a powerful strike to&n $N!&n",
    FALSE, ch, 0, vict, TO_CHAR);
  act("$n&+C chants a mantra, then touches&n $N&+C who reels in pain!&n",
    FALSE, ch, 0, vict, TO_NOTVICT);
  act("$n&+C's chants a mantra as $e touches you - pain courses throughout your body!&n",
    FALSE, ch, 0, vict, TO_VICT);

  debug("chant_jin_touch: (%s) hitting (%s) for (%d) dam.", GET_NAME(ch), GET_NAME(vict), dam);
  if(melee_damage(ch, vict, dam, PHSDAM_NOREDUCE | PHSDAM_NOPOSITION | PHSDAM_TOUCH, 0) != DAM_NONEDEAD)
        return;

  if (!char_in_list(ch))
    return;

  percent = (BOUNDED(0, (GET_LEVEL(ch) - GET_LEVEL(vict)), 30)) + number(-5, 5);

  if (GET_CHAR_SKILL(ch, SKILL_ANATOMY))
    percent += (int)(GET_CHAR_SKILL(ch, SKILL_ANATOMY)/10);

  if(!affected_by_spell(vict, SKILL_JIN_TOUCH) &&
    (percent > number(0, 19)) &&
    skl_lvl >= 100 &&
    !IS_SOULLESS(vict) &&
    !IS_GREATER_RACE(vict) &&
    !IS_ELITE(vict))
  {
    act("&+cYou harness the full, harmonius energy of your Jin, directing it at&n $N!",
      FALSE, ch, 0, vict, TO_CHAR);
    act("$n&+c attacks your very soul, and you feel less capable of defending yourself!&n",
      FALSE, ch, 0, vict, TO_VICT);
    act("$n's &+ctouch seems to have particularly weakened&n $N!&n",
      FALSE, ch, 0, vict, TO_NOTVICT);
    struct affected_type af;
    memset(&af, 0, sizeof(af));
    af.type = SKILL_JIN_TOUCH;
    af.duration =  1;

    af.modifier = GET_LEVEL(ch) * number(2, 3);
    af.location = APPLY_AC;
    affect_to_char(vict, &af);
  }
}

void chant_ki_strike(P_char ch, char *argument, int cmd)
{
  P_char   vict = NULL;
  char     name[256];
  int      dam, percent;
  int      skl_lvl = 0;
  int      level = GET_LEVEL(ch);

  if (!GET_CLASS(ch, CLASS_MONK))
  {
    send_to_char("Too bad you're not a monk, eh?\r\n", ch);
    return;
  }

  if (IS_PC(ch))
    skl_lvl = GET_CHAR_SKILL(ch, SKILL_KI_STRIKE);
  else
    skl_lvl = MAX(100, GET_LEVEL(ch) * 2);

  if (argument)
    one_argument(argument, name);
  if (!argument || !*argument || !(vict = get_char_room_vis(ch, name)))
  {
    if(ch->specials.fighting &&
      (GET_STAT(ch->specials.fighting) != STAT_DEAD))
       vict = ch->specials.fighting;
  }
  if (!vict || (GET_STAT(vict) == STAT_DEAD))
  {
    send_to_char("Chant on whom?\r\n", ch);
    return;
  }
  if (vict == ch)
  {
    send_to_char("You hum to yourself.\r\n", ch);
    return;
  }
  if (!affect_timer(ch,
        WAIT_SEC * get_property("timer.secs.monkKistrike", 45),
        SKILL_KI_STRIKE))
  {
    send_to_char("Yer not in proper mood for that right now!\r\n", ch);
    return;
  }
  if (CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char
      ("You feel ashamed to try to disrupt the tranquility of this place.\r\n",
       ch);
    return;
  }
  if ((IS_SET(world[ch->in_room].room_flags, SINGLE_FILE)) &&
      (!AdjacentInRoom(ch, vict)))
  {
    send_to_char("Your target is too far for your touch to reach!\n", ch);
    return;
  }
  if (number(1, 100) > skl_lvl)
  {
    appear(ch);
    send_to_char("You forgot the words for the chant.\r\n", ch);
    CharWait(ch, 2 * PULSE_VIOLENCE);

    if (IS_NPC(vict) && CAN_SEE(vict, ch) && number(0, 1))
    {
      remember(vict, ch);
      if (!IS_FIGHTING(vict) && !IS_DESTROYING(vict))
        MobStartFight(vict, ch);
    }

    return;
  }
  if (!IS_FIGHTING(ch) && !IS_DESTROYING(ch))
    set_fighting(ch, vict);
  appear(ch);
  act("&+BYou swiftly strike at $N&+B, delivering a quick, decisive blow to a pressure point!&n",
    FALSE, ch, 0, vict, TO_CHAR);
  act("&+B$n&+B lunges at $N&+B striking $S chest, leaving $M slightly dazed!&n",
    FALSE, ch, 0, vict, TO_NOTVICT);
  act("&+B$n&+B lunges at you, and before you can react, you feel somewhat dazed!&n",
     FALSE, ch, 0, vict, TO_VICT);
  CharWait(vict, (int) (1.5 * PULSE_VIOLENCE));
  if (!char_in_list(ch))
    return;

  CharWait(ch, (int) (2.5 * PULSE_VIOLENCE));

  percent = (BOUNDED(0, (GET_LEVEL(ch) - GET_LEVEL(vict)), 100));
  percent += number(-5, 20);

  if (GET_CHAR_SKILL(ch, SKILL_ANATOMY))
    percent += (int)(GET_CHAR_SKILL(ch, SKILL_ANATOMY)/10);

  if(!affected_by_spell(vict, SKILL_KI_STRIKE) &&
    (percent > number(1, 30)) &&
    !IS_GREATER_RACE(vict) &&
    !IS_ELITE(vict))
  {
      act("&+bYour attack on $N&+b's pressure point is particularly devastating!&n",
        FALSE, ch, 0, vict, TO_CHAR);
      act("&+b$n&+b's attack strikes hard, and you feel yourself slloooowwwww down!&n",
        FALSE, ch, 0, vict, TO_VICT);
      act("&+b$N&+b begins to move MUCH more sluggishly!&n",
        FALSE, ch, 0, vict, TO_NOTVICT);
      struct affected_type af;
      memset(&af, 0, sizeof(af));
      af.type = SKILL_KI_STRIKE;
      af.duration =  1;
      af.bitvector2 = AFF2_SLOW;
      affect_to_char(vict, &af);

  }

  CharWait(ch, (int) (0.2 * PULSE_VIOLENCE));
}

void chant_regenerate(P_char ch, char *argument, int cmd)
{
  struct affected_type af;

  if (!GET_CHAR_SKILL(ch, SKILL_REGENERATE))
  {
    send_to_char
      ("Your control over the body is not sufficient to attempt such sophisticated technique.\r\n",
       ch);
    return;
  }

  if (ch->specials.fighting)
  {
    send_to_char("You can't concentrate enough!\r\n", ch);
    return;
  }

  if (!affect_timer(ch,
        WAIT_SEC * get_property("timer.secs.monkRegenerate", 120),
        SKILL_REGENERATE))
  {
    send_to_char("Yer not in proper mood for that right now!\r\n", ch);
    return;
  }

  if (affected_by_spell(ch, SKILL_REGENERATE) ||
      affected_by_spell(ch, SPELL_REGENERATION) ||
      affected_by_spell(ch, SPELL_ACCEL_HEALING))
  {
    send_to_char("You are already regenerating.\r\n", ch);
    return;
  }

  if( !notch_skill(ch, SKILL_REGENERATE, 30)
    && number(1, 100) > GET_CHAR_SKILL(ch, SKILL_REGENERATE))
  {
    send_to_char("You forgot the words for the chant.\r\n", ch);
    CharWait(ch, 2 * PULSE_VIOLENCE);
    return;
  }

  send_to_char("You feel your body healing faster.\r\n", ch);
  bzero(&af, sizeof(af));
  af.type = SPELL_REGENERATION;
  af.duration = GET_LEVEL(ch) / 2;
  af.location = APPLY_HIT_REG;
  af.modifier = GET_CHAR_SKILL(ch, SKILL_REGENERATE);
  affect_to_char(ch, &af);

  notch_skill(ch, SKILL_REGENERATE, 35);
  CharWait(ch, PULSE_VIOLENCE);
}

void chant_tiger_palm(P_char ch, char *arg, int cmd)
{
  struct affected_type af;

  if (!GET_CHAR_SKILL(ch, SKILL_TIGER_PALM) && !IS_TRUSTED(ch))
  {
    send_to_char("You wouldnt know where to begin.\r\n", ch);
    return;
  }

 if (!affect_timer(ch,
       WAIT_SEC * get_property("timer.secs.monkTigerPalm", 30),
       SKILL_TIGER_PALM))
  {
    send_to_char("Yer not in proper mood for that right now!\r\n", ch);
    return;
  }

  if( !notch_skill(ch, SKILL_TIGER_PALM, get_property("skill.notch.chants", 20))
    && (number(1,101) > (IS_PC(ch) ? (1 + GET_CHAR_SKILL(ch, SKILL_TIGER_PALM)) : (MIN(100,GET_LEVEL(ch) * 2)))))
  {
    send_to_char("You fail to embrace the tiger palm concentration&n!\r\n", ch);
    return;
  }

  set_short_affected_by(ch, SKILL_TIGER_PALM, WAIT_SEC * (BOUNDED(4, (GET_CHAR_SKILL(ch, SKILL_TIGER_PALM) / 2), 55)));
  send_to_char
     ("&+rYou invoke the power of the tiger palm concentration.\r\n"
     , ch);
  act("&+r$n&+r's concentration grows as $e summons $s inner chi-powers.&n", FALSE, ch, 0, 0, TO_ROOM);
}

void chant_fist_of_dragon(P_char ch, char *arg, int cmd)
{
  struct affected_type af;
  int skl_lvl;

  skl_lvl = IS_TRUSTED(ch) ? 100 : GET_CHAR_SKILL(ch, SKILL_FIST_OF_DRAGON);

  if( !skl_lvl )
  {
    send_to_char("You wouldnt know where to begin.\r\n", ch);
    return;
  }

  if( !affect_timer(ch, WAIT_SEC * get_property("timer.secs.monkFistOfDragon", 30), SKILL_FIST_OF_DRAGON) )
  {
    send_to_char("Yer not in proper mood for that right now!\r\n", ch);
    return;
  }

  if( !notch_skill(ch, SKILL_FIST_OF_DRAGON, get_property("skill.notch.chants", 20))
    && (number(1,101) > skl_lvl) )
  {
    send_to_char("You fail to summon the power of the &+RRed Dragon&n!\r\n", ch);
    return;
  }

  set_short_affected_by(ch, SKILL_FIST_OF_DRAGON, WAIT_SEC * (MAX(4, (GET_LEVEL(ch) * skl_lvl / 100))));
  send_to_char("&+rThe strength of the &+Rred dragon &+rflows strong in your veins.\r\n", ch);
  act("&+r$n&+r's hands harden as $e summons $s inner chi-powers.&n", FALSE, ch, 0, 0, TO_ROOM);
}


void do_chant(P_char ch, char *argument, int cmd)
{
  char     buf[512];
  int      chant_index;

  const char *chant_skills[] = {
    "quivering palm",
    "buddha palm",
    "heroism",
    "calm",
    "regenerate",
    "jin touch",
    "fist of dragon",
    "chi purge",
    "ki strike",
    "tiger palm",
    "diamond soul",
    "\n"
  };

  const int chant_skillno[] = {
    SKILL_QUIVERING_PALM,
    SKILL_BUDDHA_PALM,
    SKILL_HEROISM,
    SKILL_CALM,
    SKILL_REGENERATE,
    SKILL_JIN_TOUCH,
    SKILL_FIST_OF_DRAGON,
    SKILL_CHI_PURGE,
    SKILL_KI_STRIKE,
    SKILL_TIGER_PALM,
    SKILL_DIAMOND_SOUL,
    0
  };
  int      skl_lvl = 0;

  if (!GET_CLASS(ch, CLASS_MONK) && !IS_TRUSTED(ch))
  {
    send_to_char("Learn some Yoga..  Then you can chant.\r\n", ch);
    return;
  }

  if( IS_PC(ch) )
    skl_lvl = IS_TRUSTED(ch) ? 100 : GET_CHAR_SKILL(ch, SKILL_CHANT);
  else
    skl_lvl = MAX(100, GET_LEVEL(ch) * 3);

  if (!argument || !*argument)
  {
    sprintf(buf, "You know the following chants:\r\n");
    sprintf(buf, "%s============================\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_QUIVERING_PALM) > 0)
      sprintf(buf, "%sQuivering Palm\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_BUDDHA_PALM) > 0)
      sprintf(buf, "%sBuddha palm\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_HEROISM) > 0)
      sprintf(buf, "%sHeroism\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_CHI_PURGE) > 0)
      sprintf(buf, "%sChi Purge\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_CALM) > 0)
      sprintf(buf, "%sCalm\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_REGENERATE) > 0)
      sprintf(buf, "%sRegenerate\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_JIN_TOUCH) > 0)
      sprintf(buf, "%sJin Touch\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_FIST_OF_DRAGON) > 0)
      sprintf(buf, "%sFist of dragon\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_KI_STRIKE) > 0)
      sprintf(buf, "%sKi Strike\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_TIGER_PALM) > 0)
      sprintf(buf, "%sTiger Palm\r\n", buf);
    if( IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_DIAMOND_SOUL) > 0)
      sprintf(buf, "%sDiamond Soul\r\n", buf);

    send_to_char(buf, ch);
    return;
  }
  argument = one_argument(argument, buf);

  chant_index = search_block(buf, chant_skills, FALSE);

  if ((chant_index == -1) || !skl_lvl)
  {
    send_to_char("You hum to yourself.\r\n", ch);
    return;
  }
  CharWait(ch, PULSE_SPELLCAST);
  /*
     let them succeeed all the time if practed well
   */
  if (number(1, (85 + GET_LEVEL(ch) / 2)) > (skl_lvl + GET_LEVEL(ch)))
  {
    send_to_char("Your throat suddenly becomes dry.\r\n", ch);
    act("$n tries to say something, but $s voice is garbled.", FALSE, ch, 0,
        0, TO_ROOM);

    notch_skill(ch, SKILL_CHANT, 5);
    return;
  }
  notch_skill(ch, SKILL_CHANT, get_property("skill.notch.chants", 10));
  act("You start to chant in a deep voice.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n starts to chant in a deep voice.", TRUE, ch, 0, 0, TO_ROOM);
  switch (chant_index)
  {
    case 0:
      chant_quivering_palm(ch, argument, cmd);
      break;
    case 1:
      chant_buddha_palm(ch, argument, cmd);
      break;
    case 2:
      chant_heroism(ch, argument, cmd);
      break;
    case 3:
      chant_calm(ch, argument, cmd);
      break;
    case 4:
      chant_regenerate(ch, argument, cmd);
      break;
    case 5:
      chant_jin_touch(ch, argument, cmd);
      break;
    case 6:
      chant_fist_of_dragon(ch, argument, cmd);
      break;
    case 7:
      chant_chi_purge(GET_LEVEL(ch), ch, NULL, 0, ch, NULL);
      break;
    case 8:
      chant_ki_strike(ch, argument, cmd);
      break;
    case 9:
      chant_tiger_palm(ch, argument, cmd);
      break;
    case 10:
      chant_diamond_soul(ch, argument, cmd);
      break;
    default:
      send_to_char("Error in chant, please report.\r\n", ch);
      break;
  }
}

/* Returns a % modifier based on condition of the victim. To use in offensive skills */

int GetConditionModifier(P_char victim)
{
  if (!victim || !IS_ALIVE(victim))
    return 0;

  if (GET_STAT(victim) == STAT_DYING || GET_STAT(victim) == STAT_INCAP)
    return 90;
    
  if (IS_AFFECTED2(victim, AFF2_MINOR_PARALYSIS) ||
    IS_AFFECTED2(victim, AFF2_MAJOR_PARALYSIS))
    return 75;

  if (GET_STAT(victim) == STAT_SLEEPING || IS_AFFECTED(victim, AFF_KNOCKED_OUT))
    return 50;

  if (IS_AFFECTED2(victim, AFF2_STUNNED))
    return 30;
    
  if (IS_AFFECTED(victim, AFF_BOUND))
    return 20;

  if (P_char mount = get_linked_char(victim, LNK_RIDING))
  {
      if (!GET_CHAR_SKILL(victim, SKILL_MOUNTED_COMBAT) /*&& !is_natural_mount(victim, mount)*/)
        return 15;
  }

  if (affected_by_spell(victim, SONG_PEACE))
    return 10;
  
  return 0;
}


void do_dragon_punch(P_char ch, char *argument, int cmd)
{
  P_char   vict = NULL;
  char     name[100];
  int      skl_lvl = 0;
  int      dam = 0;
  struct damage_messages messages = {
    "WHAP!  For a moment, you feel just like a dragon must as you punch $N.",
    "$n's fist flies toward you at frightening speeds, stopping only when it strikes your flesh.",
    "$n's fist shoots with sound barrier-breaking force towards $N, stopping only when it strikes $S flesh.",
    "You punch forcefully at $N, causing $S face to collapse under the blow, blood everywhere.  Hmm, doesn't look like $N will recover. Ever.",
    "$n's forceful punch causes you to see red, until you die shortly thereafter.",
    "$n punches $N right in the face!  There is blood everywhere, and $N seems to be dead!",
      0
  };

  if ((skl_lvl = GET_CHAR_SKILL(ch, SKILL_DRAGON_PUNCH)) == 0)
  {
    send_to_char("You don't know how.\r\n", ch);
    return;
  }

  vict = ParseTarget(ch, argument);
  if (!vict)
  {
      send_to_char
        ("A true martial artist would know his opponent for certain before striking.\r\n",
         ch);
      return;
  }
  if (!CanDoFightMove(ch, vict))
    return;

  if ((GET_ALT_SIZE(vict) < (GET_ALT_SIZE(ch) - 1)) ||
      (GET_ALT_SIZE(vict) > (GET_ALT_SIZE(ch) + 1)))
  {
    send_to_char
      ("Your punch would not be very effective on an opponent that size.\n\r",
       ch);
    return;
  }

  skl_lvl = BOUNDED(0, skl_lvl + GetConditionModifier(vict), 100);

  dam = dice(60, 8);
  dam = dam * MAX(30, skl_lvl) / 100;

  notch_skill(ch, SKILL_DRAGON_PUNCH, get_property("skill.notch.offensive", 15));
  // Gona try making them always land, but have damage based on skill.
  /*
  if (!notch_skill(ch, SKILL_DRAGON_PUNCH,
                   get_property("skill.notch.offensive", 15)) &&
      number(1, 101) > skl_lvl)
  {
    act("You miss $N with your haymaker!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n throws a haymaker at you which whistles by your ear!", FALSE, ch,
        0, vict, TO_VICT);
    act("$n just misses $N with a strong punch!", FALSE, ch, 0, vict,
        TO_NOTVICT);
  }
  else
  */
    melee_damage(ch, vict, dam, PHSDAM_TOUCH, &messages);

  CharWait(ch, 2 * PULSE_VIOLENCE);
}

void do_OLD_bandage(P_char ch, char *arg, int cmd)
{
  char     name[MAX_INPUT_LENGTH];

/*  char Gbuf1[MAX_STRING_LENGTH];*/
  P_char   t_char = NULL;

  if (!SanityCheck(ch, "do_bandage"))
    return;

  if (IS_NPC(ch))
  {
    send_to_char("You're too NPC-like to try.\r\n", ch);
    return;
  }
  if (IS_PC(ch) && GET_CHAR_SKILL(ch, SKILL_BANDAGE) == 0)
  {
    send_to_char
      ("Maybe you should leave that to someone skilled in battlefield first aid.\r\n",
       ch);
    return;
  }
  if (!*arg)
  {
    send_to_char("Yes, but whom would you like to bandage?\r\n", ch);
    return;
  }
  if (IS_FIGHTING(ch))
  {
    send_to_char("Try again when someone isn't trying to kill you.\r\n", ch);
    return;
  }
  if (IS_DESTROYING(ch))
  {
    send_to_char("Try again when you're not trying to break anything.\r\n", ch);
    return;
  }
  one_argument(arg, name);
  if (!(t_char = get_char_room_vis(ch, name)))
  {
    send_to_char("You don't see that person here.\r\n", ch);
    return;
  }
  if (NumAttackers(t_char))
  {
    send_to_char
      ("That person is still being attacked, maybe you should wait?\r\n", ch);
    return;
  }
  if (GET_HIT(t_char) >= 0 /* && GET_HIT(t_char) < GET_MAX_HIT(t_char) */ )
  {
/*
    sprintf(Gbuf1, "Your battlefield first-aid won't do much for %s right now.\r\n", GET_NAME(t_char));
    send_to_char(Gbuf1, ch);
*/
    act("Your battlefield first-aid won't do much for $N right now.", TRUE,
        ch, 0, t_char, TO_CHAR);
    return;
  }
  else if (GET_HIT(t_char) == GET_MAX_HIT(t_char))
  {
/*
    sprintf(Gbuf1, "%s is in perfect shape already!\r\n", HSSH(t_char));
    send_to_char(Gbuf1, ch);
*/
    act("$N is in perfect shape already!", TRUE, ch, 0, t_char, TO_CHAR);
    return;
  }
  if (number(1, 10) <= GET_CHAR_SKILL(ch, SKILL_BANDAGE))
  {                             /*
                                   Made their
                                   roll
                                 */
    act("You bandage $N, whose wounds appear not quite so life-threatening.",
        TRUE, ch, 0, t_char, TO_CHAR);
    act("$n bandages $N, whose wounds appear not quite so life-threatening.",
        FALSE, ch, 0, t_char, TO_ROOM);
    act
      ("You feel slightly better, but you still are in desperate need of healing.",
       TRUE, ch, 0, t_char, TO_VICT);
    if (IS_THRIKREEN(t_char))
      GET_HIT(t_char) += 1;
    else
      GET_HIT(t_char) += 2;

    if (has_innate(ch, INNATE_BATTLEAID) && GET_HIT(t_char) < 0)
      GET_HIT(t_char) += ((GET_HIT(t_char) * -1) + 1);

    notch_skill(ch, SKILL_BANDAGE, 3.33);
  }
  else if (number(1, 100) > 90)
  {                             /*
                                   Failed, and failed badly. Tricky
                                   business fixing someone out there on
                                   the battlefield.
                                 */
    act
      ("Your hasty attempts to help $N just cause more damage!  You kill $M!",
       TRUE, ch, 0, t_char, TO_CHAR);
    act
      ("$n's hasty attempts to help $N just cause more damage!  $m kills $M!",
       FALSE, ch, 0, t_char, TO_ROOM);
    act
      ("You suddenly feel much worse!  Someone's attempts to help you failed!",
       TRUE, ch, 0, t_char, TO_VICT);
    if (IS_PC(t_char))
    {
      statuslog(t_char->player.level,
                "%s died when %s attempted to bandage %s at %s [%d].",
                GET_NAME(t_char), GET_NAME(ch),
                HMHR(t_char), world[t_char->in_room].name,
                world[t_char->in_room].number);
      logit(LOG_DEATH, "%s killed by failed bandage from %s at %s [%d].",
            GET_NAME(t_char), GET_NAME(ch),
            world[t_char->in_room].name, world[t_char->in_room].number);
    }
    die(t_char, ch);
    t_char = NULL;
  }
  else
  {
    act("You clumsily cause more damage to $N!",
        TRUE, ch, 0, t_char, TO_CHAR);
    act("$n clumsily causes more damage to $N!",
        FALSE, ch, 0, t_char, TO_ROOM);
    act("You feel slightly worse as someone causes more damage to you!",
        FALSE, ch, 0, t_char, TO_VICT);
    GET_HIT(t_char)--;
    if (GET_HIT(t_char) <= -10)
    {
      if (IS_PC(t_char))
      {
        statuslog(t_char->player.level,
                  "%s: <-10 hps and died from %s's failed bandage in %d.",
                  GET_NAME(t_char), GET_NAME(ch),
                  ((t_char->in_room == NOWHERE) ?
                   -1 : world[t_char->in_room].number));

        logit(LOG_DEATH,
              "%s: <-10 hps and died from %s's failed bandage in %d.",
              GET_NAME(t_char), GET_NAME(ch),
              ((t_char->in_room ==
                NOWHERE) ? -1 : world[t_char->in_room].number));
      }
      die(t_char, ch);
      t_char = NULL;
    }
  }
  if (t_char)
    update_pos(t_char);
  CharWait(ch, 2 * PULSE_VIOLENCE);
  return;
}

void event_summon_book(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_obj    book;
  char     bookname[512];
  char     namebuf[512];
  char    *tmp;

  sprintf(bookname, "bookof%s", ch->player.name);

  for (book = object_list; book; book = book->next)
  {
    tmp = strstr(book->name, bookname);
    if (tmp && strcmp(tmp, bookname) == 0)
      break;
  }

  if (book)
    extract_obj(book);

  book = read_object(31, VIRTUAL);
  if (book == NULL)
    return;

  SET_BIT(book->str_mask, STRUNG_DESC2 || STRUNG_KEYS);
  sprintf(namebuf, "book spellbook %s", bookname);
  book->name = str_dup(namebuf);
  sprintf(namebuf, "a &+Wsp&+wel&+Wlb&+woo&+Wk&n &+Lbearing the insignia of&n %s", ch->player.name);
  book->short_description = str_dup(namebuf);
  book->extra_flags |= ITEM_NORENT;
  book->bitvector = 0;

  send_to_char("&+LA magical spellbook materializes slowly in your hands.&n\r\n", ch);

  obj_to_char(book, ch);
}

void do_summon_book(P_char ch, char *arg, int cmd)
{
  send_to_char("You utter a magical formula summoning your spellbook..\r\n", ch);
  add_event(event_summon_book, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
}

void event_summon_totem(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_obj    totem;
  char     totemname[512];
  char     namebuf[512];
  char    *tmp;

  sprintf(totemname, "totem spirit %s", ch->player.name);

  for (totem = object_list; totem; totem = totem->next)
  {
    tmp = strstr(totem->name, totemname);
    if (tmp && (strcmp(tmp, totemname) == 0) && (obj_index[totem->R_num].virtual_number == 417))
      break;
  }

  if (totem)
    extract_obj(totem);

  totem = read_object(417, VIRTUAL);
  if (totem == NULL)
    return;

  SET_BIT(totem->str_mask, STRUNG_DESC2 || STRUNG_KEYS);
  sprintf(namebuf, "totem spirit %s", totemname);
  totem->name = str_dup(namebuf);
  sprintf(namebuf, "a &+wspirit &+ytotem&n of&n &+G%s&n", ch->player.name);
  totem->short_description = str_dup(namebuf);
  totem->extra_flags |= ITEM_NORENT;
  totem->bitvector = 0;
  /* okay, lets give the totem stats, based on the level of the lil baby goblin */
  if (GET_LEVEL(ch) >= 56)
  {
   totem->affected[2].location = APPLY_SPELL_PULSE;
   totem->affected[2].modifier = -1;
  }
  if (GET_LEVEL(ch) >= 51)
  {
    totem->affected[1].location = APPLY_HIT;
    totem->affected[1].modifier = 10;
  }
  if (GET_LEVEL(ch) >= 41)
  {
    SET_BIT(totem->bitvector2, AFF2_SOULSHIELD);
  }
  if (GET_LEVEL(ch) >= 36)
  {
    SET_BIT(totem->bitvector, AFF_SENSE_LIFE);
  }
  if (GET_LEVEL(ch) >= 31)
  {
    SET_BIT(totem->bitvector, AFF_FARSEE);
  }

  send_to_char("&+GMaglubiyet&n answers your call, and a magic totem slowly materializes in your hands.\r\n", ch);

  obj_to_char(totem, ch);
}

void do_summon_totem(P_char ch, char *arg, int cmd)
{
  send_to_char("You thrust your arms skyward, uttering an incantation to &+GMaglubiyet&n.\r\n",
               ch);
  add_event(event_summon_totem, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
}

void mount_summoning_thing(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_char   mount = NULL;
  int      factor, align, in_room;
  struct follow_type *fol;

  if(!(ch) || (ch->in_room == NOWHERE) || !IS_ALIVE(ch))  /*
                                           They died in the meantime. Events
                                           should have been pulled for them,
                                           but why trust that
                                         */
    return;

  if(IS_SET(world[ch->in_room].room_flags, LOCKER))
  {
    send_to_char("Your mount couldn't find its way into the locker.\r\n", ch);
    return;
  }
  
  if(!is_prime_plane(ch->in_room) ||
    world[ch->in_room].sector_type == SECT_OCEAN ||
    IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
    send_to_char("&+LYour mount cannot find you here...\r\n", ch);
    return;
  }
  
  for (fol = ch->followers; fol; fol = fol->next)
  {
    if (IS_NPC(fol->follower) &&
        IS_SET(fol->follower->specials.act, ACT_MOUNT))
    {
      send_to_char("You already have a mount!\r\n", ch);
      return;
    }
  }
    
  if(IS_EVIL(ch) &&
          GET_CLASS(ch, CLASS_ANTIPALADIN))
  {
    mount = read_mobile(GET_SPEC(ch, CLASS_ANTIPALADIN, SPEC_DEMONIC) ? 1234 : 1231, VIRTUAL);
  }
  else if(IS_GOOD(ch) &&
          GET_CLASS(ch, CLASS_PALADIN))
  {
    mount = read_mobile(GET_SPEC(ch, CLASS_PALADIN, SPEC_CAVALIER) ? 1235 : 1232, VIRTUAL);
  }
  else if(GET_RACE(ch) == RACE_GOBLIN || GET_RACE(ch) == RACE_ORC)
  {
    mount = read_mobile( 39 , VIRTUAL);
  }
  else
  {
    send_to_char("A mount didn't load. Please report this with the bug command.", ch);
    return;
  }
  if(!mount)
  {
    logit(LOG_DEBUG, "mount_summoning_thing() did not load mount.");
    send_to_char("No mount could be found, please report this to a god.\r\n", ch);
    return;
  }
  if(ch &&
     mount) // Just making sure.
  {
    char_to_room(mount, ch->in_room, -2);

    act("$N answers your summons!", TRUE, ch, 0, mount, TO_CHAR);
    act("$N walks in, seemingly from nowhere, and nuzzles $n's face.", TRUE, ch,
        0, mount, TO_ROOM);
    setup_pet(mount, ch, -1, PET_NOCASH);
    add_follower(mount, ch);
    if(GET_LEVEL(ch) > 50 ||
      GET_SPEC(ch, CLASS_PALADIN, SPEC_CAVALIER) ||
      GET_SPEC(ch, CLASS_ANTIPALADIN, SPEC_DEMONIC))
    {
      SET_BIT(mount->specials.affected_by, AFF_FLY);
    }

    // Made all ap and paladin mounts more resistant. Nov08 -Lucrot
    if(GET_CLASS(ch, CLASS_ANTIPALADIN | CLASS_PALADIN))
    { // Tweaked AC from level * 3 to level * 6 Nov08 -Lucrot
      mount->points.base_armor = 0 - GET_LEVEL(ch) * 5; 
      if(GET_LEVEL(ch) > 45)
      {
        SET_BIT(mount->specials.affected_by, AFF_PROT_FIRE);
        SET_BIT(mount->specials.affected_by2, AFF2_PROT_COLD);
        SET_BIT(mount->specials.affected_by2, AFF2_PROT_LIGHTNING);
        SET_BIT(mount->specials.affected_by2, AFF2_PROT_GAS);
        SET_BIT(mount->specials.affected_by2, AFF2_PROT_ACID);
        if(GET_SPEC(ch, CLASS_PALADIN, SPEC_CAVALIER))
        {
          SET_BIT(mount->specials.affected_by, AFF_PROTECT_EVIL);
        }
        else if(GET_SPEC(ch, CLASS_ANTIPALADIN, SPEC_DEMONIC))
        {
          SET_BIT(mount->specials.affected_by, AFF_PROTECT_GOOD);
        }
      }
    }

    if(GET_LEVEL(ch) > 50 && 
      GET_SPEC(ch, CLASS_PALADIN, SPEC_CAVALIER))
    { // Holy sacrifice affect does not help much. Nov08 -Lucrot
      SET_BIT(mount->specials.affected_by4, AFF4_HOLY_SACRIFICE);
      // Added. Nov08 -Lucrot
      SET_BIT(mount->specials.affected_by4, AFF4_REGENERATION);
      
      if(GET_LEVEL(ch) > 53)
      { // Added. Nov08 -Lucrot
        SET_BIT(mount->specials.affected_by, AFF_HASTE);
      }
    }

    if(GET_LEVEL(ch) > 50 &&
      GET_SPEC(ch, CLASS_ANTIPALADIN, SPEC_DEMONIC))
    { // Battle X is excellent. Nov08 -Lucrot
      SET_BIT(mount->specials.affected_by4, AFF4_BATTLE_ECSTASY);
    }

    if(GET_LEVEL(ch) > 50 &&  // For all paladin and ap mounts. Nov08 -Lucrot
      GET_CLASS(ch, CLASS_ANTIPALADIN | CLASS_PALADIN))
    {// Added. Nov08 -Lucrot
      SET_BIT(mount->specials.affected_by4, AFF4_NOFEAR);
     
      if(GET_LEVEL(ch) > 51)
      {// Added. Nov08 -Lucrot
        SET_BIT(mount->specials.affected_by5, AFF5_NOBLIND);
      }
      
      if(GET_LEVEL(ch) > 52)
      {// Added. Nov08 -Lucrot
        SET_BIT(mount->specials.act, ACT_IMMUNE_TO_PARA);
      }
    }

    /*
       now we modify the base mount, based on paladin's level and alignment
     */

    /*
       factor ranges from 0 to 25 (unlucky 351 align level 15 to lucky 1000 align
       level 50)
     */
  // Alignment adjusted only affect paladins. Making these modifiers
  // standard for both classes. Nov08 -Lucrot
  // align = GET_ALIGNMENT(ch) / 200;
  // if (IS_EVIL(ch))
    // align = -align;
 
    // Level 30 mount factor average = 22.5 based on 0.750 mod.
    // Level 56 mount factor average = 42 based on 0.750 mod. Nov08 -Lucrot
    factor = (int) (GET_LEVEL(ch) * get_property("mount.summoned.FactorMod", 0.750) +
      number(-5, 5));
    mount->base_stats.Str = BOUNDED(75, mount->base_stats.Str, 75 + factor);
    mount->base_stats.Agi = BOUNDED(75, mount->base_stats.Agi, 75 + factor);
    mount->base_stats.Con = BOUNDED(75, mount->base_stats.Con, 75 + factor);
    mount->base_stats.Cha = 100;
    mount->player.level = 10 + factor;
    mount->points.base_hit = (factor * 50);
    GET_HIT(mount) = (factor * 50);
    GET_MAX_HIT(mount) = (factor * 50);
    mount->player.m_class = CLASS_NONE;
    MonkSetSpecialDie(mount);
    SET_BIT(mount->specials.act, ACT_MOUNT);
    
    if(IS_SET(mount->specials.act, ACT_MEMORY))
    {
      clearMemory(mount);
      REMOVE_BIT(mount->specials.act, ACT_MEMORY);
    }
    
    return;
  }
  send_to_char("A mount didn't load. Please report this with the bug command.\r\n", ch);
  return;

}

void do_summon_mount(P_char ch, char *arg, int cmd)
{
  int      sumtime;
  struct follow_type *fol;

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  for( fol = ch->followers; fol; fol = fol->next )
  {
    if( IS_NPC(fol->follower) && IS_SET(fol->follower->specials.act, ACT_MOUNT) )
    {
      send_to_char("You already have a mount!\r\n", ch);
      return;
    }
  }

  if( !is_prime_plane(ch->in_room) )
  {
    send_to_char("&+LYour mount cannot answer your call in this strange land/terrain...\r\n", ch);
    return;
  }

  if( IS_SET(world[ch->in_room].room_flags, LOCKER)
    || IS_SET(world[ch->in_room].room_flags, SINGLE_FILE) )
  {
    send_to_char("A horse couldn't fit in here!\r\n", ch);
    return;
  }

  if( world[ch->in_room].sector_type == SECT_OCEAN )
  {
    send_to_char("Nothing out here will answer your call!\r\n", ch);
    return;
  }

  if( !IS_GOOD(ch) && IS_PC(ch) && GET_CLASS(ch, CLASS_PALADIN) )
  {
    send_to_char("Not even horses can stand your offensive presence!\r\n", ch);
    return;
  }

  if( !IS_EVIL(ch) && IS_PC(ch) && GET_CLASS(ch, CLASS_ANTIPALADIN) )
  {
    send_to_char("Your innate skill seems to falter. You must be evil.\r\n", ch);
    return;
  }
  if( !OUTSIDE(ch) && !IS_UNDERWORLD(ch->in_room) )
  {
    send_to_char("Try again, OUTDOORS THIS TIME!\r\n", ch);
    return;
  }

  if( !check_innate_time(ch, INNATE_SUMMON_MOUNT) )
  {
    send_to_char("You can't summon another mount yet.\r\n", ch);
    return;
  }

  send_to_char("You begin calling for a mount..\r\n", ch);
  sumtime = number(70 - GET_LEVEL(ch), 100 + number(1, 200 - 2 * GET_LEVEL(ch)));
  add_event(mount_summoning_thing, sumtime, ch, 0, 0, 0, 0, 0);
}

void do_summon_warg(P_char ch, char *arg, int cmd)
{
  int      sumtime;
  struct follow_type *fol;

  for (fol = ch->followers; fol; fol = fol->next)
    if (IS_NPC(fol->follower) &&
        IS_SET(fol->follower->specials.act, ACT_MOUNT))
    {
      send_to_char("You already have a warg!\r\n", ch);
      return;
    }

	if (IS_SET(world[ch->in_room].room_flags, LOCKER)) {
		send_to_char("A warg couldn't fit in here!\r\n", ch);
		return;
	}

  if (!OUTSIDE(ch) && !IS_UNDERWORLD(ch->in_room))
  {
    send_to_char("Try again, OUTDOORS THIS TIME!\r\n", ch);
    return;
  }
    if (!check_innate_time(ch, INNATE_SUMMON_WARG))
    {
      send_to_char("You can summon just one warg a day!\r\n", ch);
      return;
    }
  send_to_char("You begin calling for a warg..\r\n", ch);
  sumtime =
    number(70 - GET_LEVEL(ch), 100 + number(1, 200 - 2 * GET_LEVEL(ch)));
  add_event(mount_summoning_thing, sumtime, ch, 0, 0, 0, 0, 0);
}



void orc_summoning_thing(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_char   orc = NULL;
  int /*shown = FALSE, */ called, i, count = 0, max_respond;
  struct char_link_data *cld;

  if (!ch || (ch->in_room == NOWHERE))  /*
                                           They died in the meantime. Events
                                           should have been pulled for them,
                                           but why trust that
                                         */
    return;

  if (IS_SET(world[ch->in_room].room_flags, LOCKER))
  {
		send_to_char("Your horde couldn't possibly fit in your locker!\r\n", ch);
	  return;
	}

  if (IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
		send_to_char("Your horde failed to reach you in this single file room. They went home.\r\n", ch);
	  return;
	}
  
  max_respond = BOUNDED(1, (int) (GET_LEVEL(ch) / 10), 20);
  
  for (cld = ch->linked; cld; cld = cld->next_linked)
    if (GET_RACE(cld->linking) == RACE_ORC && IS_NPC(cld->linking))
      count++;

  if (count >= max_respond)
  {
    send_to_char("It appears no more will answer your call for help.\r\n",
                 ch);
    return;
  }
  called = max_respond - count;

  for (i = 0; i < called; i++)
  {
    orc = read_mobile(4007, VIRTUAL);
    if (!orc /*&& !shown */ )
    {
      logit(LOG_DEBUG, "orc_summoning_thing(): mob 4010 not loadable");
      send_to_char
        ("No orc horde could be found, please report this to a god.\r\n",
         ch);
/*      shown = TRUE;*/
      return;
    }
    char_to_room(orc, ch->in_room, -2);

//      GET_LEVEL(orc) = 5;
    orc->player.level = 1;
    GET_MAX_HIT(orc) = GET_HIT(orc) = 1;

    orc->only.npc->aggro_flags = 0;
    setup_pet(orc, ch, -1, PET_NOCASH);
    add_follower(orc, ch);
  }

  send_to_char("Your brethren answer your call for help!\r\n", ch);
  act("A horde of orcs appear, charging in from every direction!", TRUE, ch,
      0, 0, TO_ROOM);
}

void do_summon_orc(P_char ch, char *arg, int cmd)
{
  int      sumtime;

  SanityCheck(ch, "do_summon_orc");

  if (IS_PC(ch) && (GET_RACE(ch) != RACE_ORC))
  {
    send_to_char("Only an orc may call for the orc horde!\r\n", ch);
    return;
  }
  if (!check_innate_time(ch, INNATE_SUMMON_HORDE))
  {
    send_to_char("You can call for the horde just thrice weekly!\r\n", ch);
    return;
  }

  act("$n throws $s head back, letting out a long, low-pitched howl...",
      FALSE, ch, 0, 0, TO_ROOM);
  send_to_char
    ("You throw your head back, letting out a long, low-pitched howl...\r\n",
     ch);

  // Cannot summon horde to plane type rooms or on oceans. 
  if(!is_prime_plane(ch->in_room) ||
    world[ch->in_room].sector_type == SECT_OCEAN)
  {
    send_to_char("&+LYou have a bad feeling that your horde will not arrive to this hostile environment...\r\n", ch);
    return;
  }
  
  if (IS_SET(world[ch->in_room].room_flags, SINGLE_FILE))
  {
		send_to_char("Your horde will not reach you here in this single file room.\r\n", ch);
	}
  
  sumtime = number(70 - GET_LEVEL(ch), 70);
  add_event(orc_summoning_thing, sumtime, ch, 0, 0, 0, 0, 0);
}

/* Ogre roar: Ogre scares the wits out of enemy. Assumed to be innate2 */
void do_ogre_roar(P_char ch, char *argument, int cmd)
{
  P_char   vict = NULL;
  char     name[MAX_INPUT_LENGTH];
  int      temp = 0;
  struct affected_type af;

  /* Why is the Proc called at all? */
  if (!ch)
    return;

  /* Test if it is an Ogre that hasn't used innate too often */
  if (GET_RACE(ch) != RACE_OGRE)
  {
    send_to_char("Your roar doesn't impress anybody that much!\r\n", ch);
    return;
  }
  if (!check_innate_time(ch, INNATE_OGREROAR))
  {
    send_to_char("Your throat is still too sore!\r\n", ch);
    return;
  }
  /* Are we ready to fight?  Impossible to roar loudly in reclined position */
  if (GET_POS(ch) == POS_PRONE)
  {
    send_to_char("They will just think you are snoring!\r\n", ch);
    return;
  }
  /* Do we have an enemy at all? */
  one_argument(argument, name);
  if (*name)
    vict = get_char_room_vis(ch, name);
  else if (!vict && ch->specials.fighting)
  {
    vict = ch->specials.fighting;
    if (vict->in_room != ch->in_room)
    {
      stop_fighting(ch);
      vict = NULL;
    }
  }
  if (!vict)
  {
    send_to_char("You roar and roar, but nobody cares.\r\n", ch);
    return;
  }
  /* do some basic fight checks */
  if (!CanDoFightMove(ch, vict))
    return;

  /* now checks on the race (size) and state of victim */
  /* big stuff isn't impressed at all, illithids have too much control
     over their minds to be scared */
  if ((GET_ALT_SIZE(vict) > GET_ALT_SIZE(ch)) || IS_ILLITHID(vict) || IS_PILLITHID(vict))
  {
    act("$n roars loudly at $N, but $N isn't fazed at all.",
        FALSE, ch, 0, vict, TO_ROOM);
    act("Obviously you are not as scary as you think you are.",
        FALSE, ch, 0, vict, TO_CHAR);
    act("$n roars at you, but you couldn't care less.",
        FALSE, ch, 0, vict, TO_VICT);
    return;
  }

  /* victim isn't hearing the roar */
  if (IS_AFFECTED(vict, AFF_KNOCKED_OUT) || IS_AFFECTED(vict, AFF_SLEEP) ||
      GET_STAT(vict) < STAT_RESTING ||
      IS_SET(world[(vict)->in_room].room_flags, ROOM_SILENT))
  {
    act("$N isn't able to hear your roar.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  /* victim is already affected, can't scare him twice... */
  if (affected_by_spell(vict, SKILL_OGRE_ROAR))
  {
    act("$N is scared beyond caring anyway.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  /* all tests passed, let's see if we can do it */
  /* people with good control of their minds are hard to impress */
  temp = (GET_LEVEL(vict) - GET_LEVEL(ch)) / 10 +
    (GET_C_POW(vict) - GET_C_POW(ch)) / 15 +
    (GET_C_WIS(vict) - GET_C_WIS(ch)) / 15;
  /* chance modifier bounded to min/max +-(6+10+10)=+-26 */
  /* do magic/affects check: is victim fearful already or meditating? */
  if (IS_AFFECTED(vict, AFF_FEAR) || IS_AFFECTED(vict, AFF_MEDITATE))
    temp -= 5;
  /* chance modifier now bounded by -31 to +26 */

  /* do random 1-101, add chance modifier */
  temp = BOUNDED(1, number(1, 101) + temp, 101);

  /* worst case for ogre */
  if (temp == 101)
  {
    act("$n tries to roar at $N, but just whimpers instead!",
        FALSE, ch, 0, vict, TO_ROOM);
    act("You are simply not up to it, $N frightens you too much!",
        FALSE, ch, 0, vict, TO_CHAR);
    act("$n snivels, being afraid of you.", FALSE, ch, 0, vict, TO_VICT);
    /* apply a downgraded AC/HITROLL hurt to char instead of victim */
    bzero(&af, sizeof(af));
    af.type = SKILL_OGRE_ROAR;
    af.duration = 10 + (temp / 7);
    af.modifier = 1 + GET_LEVEL(vict) / 2;
    af.location = APPLY_AC;
    affect_to_char(vict, &af);
    af.modifier = -(GET_HITROLL(ch) * GET_LEVEL(vict) / 100);
    af.location = APPLY_HITROLL;
    affect_to_char(ch, &af);

    if (IS_NPC(vict) && CAN_SEE(vict, ch))
    {
      remember(vict, ch);
      if (!IS_FIGHTING(vict) && !IS_DESTROYING(vict))
        MobStartFight(vict, ch);
    }

    return;
  }
  /* best case for ogre - his roar shakes the world */
  if (temp == 1)
  {
    act("$n roars incredibly loud at $N, $N is almost lifted off $S feet!",
        FALSE, ch, 0, vict, TO_ROOM);
    act("That roar would have stopped a charging elephant!",
        FALSE, ch, 0, vict, TO_CHAR);
    act("$n's roar freezes your brain and pops your eardrums!",
        FALSE, ch, 0, vict, TO_VICT);
    /* this really messes the victim up... worse than normal roar */
    bzero(&af, sizeof(af));
    af.type = SKILL_OGRE_ROAR;
    af.duration = 20 - (temp / 7);
    af.modifier = GET_LEVEL(ch) + 10;
    af.location = APPLY_AC;
    affect_to_char(vict, &af);
    af.modifier = -(GET_HITROLL(vict) * GET_LEVEL(ch) / 60);
    af.location = APPLY_HITROLL;
    affect_to_char(vict, &af);
    return;
  }
  else
    /* now the usual, ogres has a 2/3 chance to succeed against a
       enemy with modifiers=0 */
  if (temp < 68)
  {
    act("$n roars loudly at $N, who is visibly shaken.",
        FALSE, ch, 0, vict, TO_ROOM);
    act("Your roar at $N has definitely scared $m.",
        FALSE, ch, 0, vict, TO_CHAR);
    act("You feel a strong urge to run away after $n's roar.",
        FALSE, ch, 0, vict, TO_VICT);
    /* standard hurt to AC/HITROLL */
    bzero(&af, sizeof(af));
    af.type = SKILL_OGRE_ROAR;
    af.duration = 15 - (temp / 7);
    af.modifier = GET_LEVEL(ch);
    af.location = APPLY_AC;
    affect_to_char(vict, &af);
    af.modifier = -(GET_HITROLL(vict) * GET_LEVEL(ch) / 90);
    af.location = APPLY_HITROLL;
    affect_to_char(vict, &af);
    return;
  }
  else
  {
    act("$n's roar doesn't impress $N at all.", FALSE, ch, 0, vict, TO_ROOM);
    act("$N didn't seem to notice your roar.  Go for more volume!",
        FALSE, ch, 0, vict, TO_CHAR);
    act("$n's roar almost makes you laugh.", FALSE, ch, 0, vict, TO_VICT);

    if (IS_NPC(vict) && CAN_SEE(vict, ch))
    {
      remember(vict, ch);
      if (!IS_FIGHTING(vict) && !IS_DESTROYING(vict))
        MobStartFight(vict, ch);
    }

    return;
  }
}




/* Krov: carving procedure. checks in value[3] of a corpse which body
   parts are missing and then puts a (named) body part in corpse
   if the carve was successful */

void do_carve(struct char_data *ch, char *argument, int cmd)
{
  struct obj_data *corpse, *carve, *tool;
  char     cname[MAX_STRING_LENGTH];
  char     part[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  char    *dscp;
  byte     percent;
  int      i, which, piece, none;

#if 0
  /* how many parts, what names, what flags and what vnums of prototype */
  const int howmany = 9;
  const char *part_name[] = { "skull", "scalp", "face", "eyes", "ears",
    "tongue", "bowels", "arms", "legs"
  };
  const int part_flag[] = { MISSING_SKULL, MISSING_SCALP, MISSING_FACE,
    MISSING_EYES, MISSING_EARS, MISSING_TONGUE,
    MISSING_BOWELS, MISSING_ARMS, MISSING_LEGS
  };
#endif
  int      carve_weight[9];


  /* find out what corpse and what bodypart we want */
  half_chop(argument, cname, part);

  /* what corpse is player trying to carve? */
  if (*cname)
  {
    corpse = get_obj_in_list_vis(ch, cname, world[ch->in_room].contents);
    if (!corpse)
    {
      send_to_char("Open your eyes - that's not here.\r\n", ch);
      return;
    }
  }
  else
  {
    send_to_char("Maybe you should find something to carve first?\r\n", ch);
    return;
  }

  /* check if player is trying to carve a corpse */
  if (GET_ITEM_TYPE(corpse) != ITEM_CORPSE)
  {
    send_to_char("Geez... try carving a dead body.\r\n", ch);
    return;
  }
  /* for now only allow humanoid corpses */
  if (!(corpse->value[1] & HUMANOID_CORPSE))
  {
    send_to_char("Why would you want to do that?\r\n", ch);
    return;
  }
  /* check out what parts the body still has, if no part is given */
  none = 0;
  if (!*part)
  {
    sprintf(buf, "Parts of %s that are still intact:\r\n",
            corpse->short_description);
    for (i = 0; i < numCarvables; i++)
      if (!(corpse->value[1] & carve_part_flag[i]))
      {
        strcat(buf, carve_part_name[i]);
        strcat(buf, " ");
        none++;
      }
    if (!none)
      strcat(buf, "nothing");
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }
  tool = ch->equipment[WIELD];

  /* check if player wields a suitable weapon */
  if (!tool && !IS_TRUSTED(ch))
  {
    send_to_char("Trying to do it with your bare hands? Whew...\r\n", ch);
    return;
  }
  if (!IS_TRUSTED(ch) && !isname("blade", tool->name) &&
      !isname("dagger", tool->name) && !isname("knife", tool->name))
  {
    send_to_char("You need some kind of knife for carving!\r\n", ch);
    return;
  }
  /* what does player want to carve */
  piece = 0;
  for (which = 0; which < numCarvables; which++)
    if (!str_cmp(part, carve_part_name[which]))
    {
      piece = carve_part_flag[which];
      break;
    }
  if (!piece)
  {
    send_to_char("What piece of the body do you want to disect?\r\n", ch);
    return;
  }
  /* does the corpse still have that part? */
  if (corpse->value[1] & piece)
  {
    send_to_char("There is nothing left of that. Terrible.\r\n", ch);
    return;
  }
  /* throw a number */
  percent = number(1, 101);     /* 101 is a complete failure */

  /* if it is a complete failure, then then all parts are destroyed */
  if (percent == 101)
  {
    act("$n makes a bloody mess, horribly cutting up the $p. Gross!",
        TRUE, ch, corpse, 0, TO_ROOM);
    act("Urgh. Great, the $o is cut up beyond repair.",
        FALSE, ch, corpse, 0, TO_CHAR);
    /* set all bodyparts to missing */
    for (i = 0; i < numCarvables; i++)
      corpse->value[1] |= carve_part_flag[i];
    return;
  }
  /* no matter if fail or not, corpse hasn't got part anymore */
  corpse->value[1] |= piece;
  if (percent > GET_CHAR_SKILL(ch, SKILL_CARVE))
  {
    send_to_char("You butcher! It's all minced up now...\r\n", ch);
    notch_skill(ch, SKILL_CARVE, 5);
    return;
  }
  /* he's done it, let's load the prototype and finish it up */
  carve_weight[0] = corpse->weight / 10;
  carve_weight[1] = carve_weight[2] = carve_weight[3]
    = carve_weight[4] = carve_weight[5] = corpse->weight / 80;
  carve_weight[6] = carve_weight[8] = corpse->weight / 4;
  carve_weight[7] = corpse->weight / 7;

  notch_skill(ch, SKILL_CARVE, 2);
  carve = read_object(8, VIRTUAL);
  if (!carve)
  {
    send_to_char("Strange... please talk to a god about this.", ch);
    logit(LOG_OBJ, "Carve: Failed to load prototype 8.");
    return;
  }
  carve->str_mask =
    (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3);

  /* find out the player name and race from the copses' names */
  half_chop(corpse->name, cname, buf);

  /* as keywords the part name and the player */
  sprintf(buf, "%s %s", carve_part_name[which], cname);
  carve->name = str_dup(buf);

  /* Converts "The corpse of an orc is lying here." to e.g. "The skull..." */
  dscp = corpse->description + 11;
  sprintf(buf, "The %s %s", carve_part_name[which], dscp);
  if (piece == MISSING_EYES || piece == MISSING_EARS ||
      piece == MISSING_BOWELS || piece == MISSING_ARMS ||
      piece == MISSING_LEGS)
  {
    dscp = strstr(buf, " is ");
    if (dscp != NULL)
      sprintf(dscp, " are lying here.");
  }
  carve->description = str_dup(buf);

  /* same trick, this time for short_description */
  dscp = corpse->short_description + 11;
  sprintf(buf, "the %s %s", carve_part_name[which], dscp);
  carve->short_description = str_dup(buf);

  /* for action description just the part name */
  sprintf(buf, "%s", carve_part_name[which]);
  carve->action_description = str_dup(buf);

  /* now add the appropriate weigth */
  carve->weight = carve_weight[which];
  corpse->weight -= carve_weight[which];

  switch (piece)
  {
  case MISSING_SKULL:
    SET_BIT(carve->wear_flags, ITEM_ATTACH_BELT);
    carve->material = MAT_BONE;
    break;
  case MISSING_SCALP:
    SET_BIT(carve->wear_flags, ITEM_ATTACH_BELT);
    break;
  case MISSING_FACE:
    SET_BIT(carve->wear_flags, ITEM_ATTACH_BELT);
    break;
  case MISSING_EYES:
    break;
  case MISSING_EARS:
    break;
  case MISSING_TONGUE:
    break;
  case MISSING_BOWELS:
    SET_BIT(carve->wear_flags, ITEM_ATTACH_BELT);
    SET_BIT(carve->wear_flags, ITEM_WEAR_NECK);
    break;
  case MISSING_ARMS:
    carve->type = ITEM_WEAPON;
    carve->value[0] = 10;
    carve->value[1] = 1;
    carve->value[2] = 4;
    carve->value[3] = 7;
    carve->weight = 5;
    SET_BIT(carve->wear_flags, ITEM_WIELD);
    break;
  case MISSING_LEGS:
    carve->type = ITEM_WEAPON;
    carve->value[0] = 10;
    carve->value[1] = 1;
    carve->value[2] = 6;
    carve->value[3] = 7;
    carve->weight = 12;
    SET_BIT(carve->wear_flags, ITEM_WIELD);
    break;
  }

  /* player still has to get it, remains in corpse... */
  obj_to_obj(carve, corpse);

  act("$n successfully carved $p. Yuk!", TRUE, ch, carve, 0, TO_ROOM);
  send_to_char("There. Finally you pried it loose!\r\n", ch);

  return;
}

/* TASFALEN replace do_bind, do_unbind */

/*
   Can only bound player that are incapacitated or worst or major paralyse
*/

void do_bind(P_char ch, char *arg, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  P_char   t_char = NULL;

  if (!*arg)
  {
    send_to_char("Who do you want to bind?\r\n", ch);
    return;
  }

  if (IS_FIGHTING(ch))
  {
    send_to_char("Try again when someone isn't trying to kill you.\r\n", ch);
    return;
  }
  if (IS_DESTROYING(ch))
  {
    send_to_char("Try again when you're not trying to break something.\r\n", ch);
    return;
  }
  one_argument(arg, name);

  if (!(t_char = get_char_room_vis(ch, name)))
  {
    send_to_char("You don't see that person here.\r\n", ch);
    return;
  }

  if (ch == t_char)
  {
    send_to_char("Nice try!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(t_char, AFF_BOUND))
  {
    send_to_char
      ("They are already bound!\r\n", ch);
    return;
  }

  if (IS_NPC(t_char))
  {
    send_to_char("You cannot bind a mob.\r\n", ch);
    return;
  }

  if (IS_FIGHTING(t_char) || IS_DESTROYING(t_char))
  {
    send_to_char("Cannot bind someone fighting.\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_BOUND))
  {
    send_to_char("Try to unbind yourself first!\r\n", ch);
    return;
  }

  if (IS_AFFECTED(t_char, AFF_BOUND))
  {
    send_to_char("That person is already bound!\r\n", ch);
    return;
  }

  if ((GET_STAT(t_char) > STAT_INCAP) &&
      (!IS_AFFECTED2(t_char, AFF2_MAJOR_PARALYSIS)) &&
      !IS_AFFECTED(t_char, AFF_KNOCKED_OUT))
  {
    send_to_char
      ("The person you want to bind is not in a proper position.\r\n", ch);
    return;
  }

  /*
   * need both hands free for capture
   */
  if ((ch->equipment[WEAR_SHIELD]) ||
      (ch->equipment[PRIMARY_WEAPON]) || (ch->equipment[SECONDARY_WEAPON]))
  {
    send_to_char("You need both hands free to capture someone!\r\n", ch);
    return;
  }

  if (!ch->equipment[HOLD] || (!isname("rope", ch->equipment[HOLD]->name)))
  {
    send_to_char("You need to be holding rope to bind somebody.\r\n", ch);
    return;
  }

  SET_BIT(t_char->specials.affected_by, AFF_BOUND);
  act("$n ties $N up.", TRUE, ch, 0, t_char, TO_NOTVICT);
  act("You tie $N up.", TRUE, ch, 0, t_char, TO_CHAR);
  act("$n ties you up.", TRUE, ch, 0, t_char, TO_VICT);

  if (!IS_AFFECTED(t_char, AFF_KNOCKED_OUT))
    send_to_char("You're now bound and cannot move.\r\n", t_char);

  extract_obj(unequip_char(ch, HOLD), TRUE); // Arti rope?

  CharWait(ch, 2 * PULSE_VIOLENCE);

  return;
}


void do_unbind(P_char ch, char *arg, int cmd)
{
  char     name[MAX_INPUT_LENGTH];
  P_char   t_char = NULL;
  byte     percent;

  if (!*arg)
  {
    send_to_char("Who do you want to unbind?\r\n", ch);
    return;
  }

  one_argument(arg, name);

  if (!(t_char = get_char_room_vis(ch, name)))
  {
    send_to_char("You don't see that person here.\r\n", ch);
    return;
  }

  if (IS_NPC(t_char))
  {
    send_to_char("You cannot unbind a mob.\r\n", ch);
    return;
  }

  if (!IS_AFFECTED(t_char, AFF_BOUND))
  {
    if (ch == t_char)
      send_to_char("You're free as a bird!\r\n", ch);
    else
      send_to_char("That person is not bind!\r\n", ch);
    return;
  }

  percent = number(1, 101);

  if (ch == t_char)
  {
    if (IS_THIEF(ch))
    {
      if (percent <= (10 + (GET_LEVEL(ch) / 3)))
      {
        send_to_char("You use your skill to break free.\r\n", ch);
        act("$n twists $s body and breaks free from $s binding.", TRUE, ch, 0,
            0, TO_ROOM);
      }
      else
      {
        send_to_char("You try to use your skill to break free, but fail.\r\n",
                     ch);
        act
          ("$n tries to break free from $s bindings, but ends up tightening them more.",
           TRUE, ch, 0, 0, TO_ROOM);
        CharWait(ch, 3 * PULSE_VIOLENCE);
        return;
      }
    }
    else
    {
      if (percent <= (str_app[STAT_INDEX(GET_C_STR(ch))].todam + 10))
      {
        send_to_char("You use your brute strength to break free.\r\n", ch);
        act
          ("$n screws $s face in concentration, breaking free from $s bindings.",
           TRUE, ch, 0, 0, TO_ROOM);
      }
      else
      {
        send_to_char
          ("You try to use your strength to break free, but fail.\r\n", ch);
        act("$n's face becomes red as $e tries to free $mself.", TRUE, ch, 0,
            0, TO_ROOM);
        CharWait(ch, 2 * PULSE_VIOLENCE);
        return;
      }
    }
  }
  else
  {
    if (IS_AFFECTED(ch, AFF_BOUND))
    {
      send_to_char("You try, but fail.\r\n", ch);
      return;
    }

    act("$n removes $N's bindings.", FALSE, ch, 0, t_char, TO_NOTVICT);
    act("$n removes your bindings.", FALSE, ch, 0, t_char, TO_VICT);

    CharWait(t_char, 2 * PULSE_VIOLENCE);
  }

  REMOVE_BIT(t_char->specials.affected_by, AFF_BOUND);
  send_to_char("You're now free of your bindings.\r\n", t_char);
  CharWait(ch, 1 * PULSE_VIOLENCE);

  return;
}

void capture(P_char ch, P_char victim)
{
  int      percent, ch_chance, town;
  bool     is_wanted = FALSE;

  if (!SanityCheck(ch, "capture"))
    return;

  if ((IS_PC(ch) && (GET_CHAR_SKILL(ch, SKILL_CAPTURE) == 0)) ||
      (IS_NPC(ch) && !GET_CLASS(ch, CLASS_MERCENARY)))
  {
    send_to_char("You don't know how to capture.\r\n", ch);
    return;
  }
  if (!victim)
  {
    send_to_char("Capture who?\r\n", ch);
    return;
  }
  if (IS_NPC(victim))
  {
    send_to_char("You can't capture mobs.\r\n", ch);
    return;
  }

  if (!on_front_line(ch) || !on_front_line(victim))
  {
    send_to_char("You can't reach them!\r\n", ch);
    return;
  }

  if (IS_NPC(ch) && IS_PC(victim))
  {
    send_to_char("Nice try.\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_BOUND))
  {
    send_to_char("Try to unbind yourself first!\r\n", ch);
    return;
  }
  if (!HAS_FOOTING(ch))
  {
    send_to_char("You have no footing here!\r\n", ch);
    return;
  }
  if (!CanDoFightMove(ch, victim))
    return;

  /*
   * need both hands free for capture
   */
  if ((ch->equipment[WEAR_SHIELD]) ||
      (ch->equipment[PRIMARY_WEAPON]) || (ch->equipment[SECONDARY_WEAPON]))
  {
    send_to_char("You need both hands free to capture someone!\r\n", ch);
    return;
  }

  if (!ch->equipment[HOLD] || (!isname("rope", ch->equipment[HOLD]->name)))
  {
    send_to_char("You need to be holding rope to bind somebody.\r\n", ch);
    return;
  }

  if (IS_FIGHTING(ch) && (ch->specials.fighting != victim))
  {
    send_to_char
      ("You're WAY too busy to even attempt to capture someone else.\r\n",
       ch);
    return;
  }

  if (IS_AFFECTED(victim, AFF_BOUND))
  {
    send_to_char
      ("That person is bind enought already, c'mon now freak!!!\r\n", ch);
    return;
  }
  /*
   * for mobs too big and small
   */
  if (GET_RACE(victim) == RACE_GHOST)
  {
    act("Your attempt to grab $N fails as you simply pass through $M.",
        FALSE, ch, 0, victim, TO_CHAR);
    act("$n makes a valiant attempt to grab $N, but simply falls through $M.",
        FALSE, ch, 0, victim, TO_CHAR);
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
    return;
  }
  if (GET_RACE(victim) == RACE_A_ELEMENTAL)
  {
    act("&+WYour attempt to grab $N fails as it blows you on your ass!",
        FALSE, ch, 0, victim, TO_CHAR);
    act("&+W$n makes a valiant attempt to grab $N, but gets blown on $s ass!",
        FALSE, ch, 0, victim, TO_CHAR);
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
    return;
  }
  if (GET_RACE(victim) == RACE_E_ELEMENTAL)
  {
    act("&+WYour attempt to grab $N feels like you just hit a brick wall!",
        FALSE, ch, 0, victim, TO_CHAR);
    act("&+W$n just fell flat on $s ass when $e tried to grab $N!",
        FALSE, ch, 0, victim, TO_CHAR);
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
    return;
  }
  if (GET_RACE(victim) == RACE_F_ELEMENTAL)
  {
    act("&+WYour attempt to grab $N almost burns you to a crisp!",
        FALSE, ch, 0, victim, TO_CHAR);
    act("&+W$n tries to grab $N, but falls right through $M and onto $s ass!",
        FALSE, ch, 0, victim, TO_CHAR);
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
    return;
  }
  if (IS_WATERFORM(victim))
  {
    act("&+WYou almost drown in your attempt to grab $M!",
        FALSE, ch, 0, victim, TO_CHAR);
    act("$n almost drowns as $e passes right through $N!",
        FALSE, ch, 0, victim, TO_CHAR);
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
    return;
  }

  if (GET_ALT_SIZE(victim) > GET_ALT_SIZE(ch))
  {
    act("$n fails miserably in $s attempt to grab $N's massive form.", FALSE,
        ch, 0, victim, TO_ROOM);
    act("You simply cannot get a grab of $N's massive form.", FALSE, ch, 0,
        victim, TO_CHAR);
    return;
  }

  if (GET_ALT_SIZE(victim) < GET_ALT_SIZE(ch) - 1)
  {
    act("$n nearly topples over $mself as $e tries to grab $N.", FALSE, ch, 0,
        victim, TO_ROOM);
    send_to_char
      ("Capturing creatures of this minute size is hopeless at best.\r\n",
       ch);
    return;
  }

  /*
   * Get here iff. can capture victim
   */

  percent = number(1, 101);     /*
                                 * 101% is a complete failure
                                 */

  if (IS_NPC(ch))
    ch_chance = BOUNDED(5, GET_LEVEL(ch) << 1, 90);
  else
    ch_chance = BOUNDED(5, GET_CHAR_SKILL(ch, SKILL_CAPTURE), 90);

  /* capturing someone on the ground is a bit easier */

  ch_chance += ((GET_POS(victim) == POS_PRONE) ? 15 :
                (GET_POS(victim) != POS_STANDING) ? 10 : 0);

  /*
   * -20% for every level the guy is above you
   */

  if (GET_LEVEL(victim) > GET_LEVEL(ch))
    ch_chance -= (GET_LEVEL(victim) - GET_LEVEL(ch)) * 20;

  /*
   * let's take agility and dexterity into account, too
   */

  if (GET_C_AGI(ch) - GET_C_AGI(victim))
    ch_chance += ((GET_C_AGI(ch) - GET_C_AGI(victim)) / 3);

  if (GET_C_DEX(ch) - GET_C_DEX(victim))
    ch_chance += ((GET_C_DEX(ch) - GET_C_DEX(victim)) / 5);

  if (GET_C_LUK(ch) - GET_C_LUK(victim)) {
    ch_chance += ((GET_C_LUK(ch) - GET_C_LUK(victim)) / 5);
  }

  ch_chance = BOUNDED(1, ch_chance, 85);        /*
                                                 * always at least 15% chance to
                                                 * fail
                                                 */

  /*
   * final check to smarten mobs up a little, if odds are too low don't
   * try very often.  JAB
   */

  if (IS_NPC(ch) && (ch_chance < 20))
    if (number(1, 25) > ch_chance)
      return;

  for (town = 1; town <= LAST_HOME; town++)
  {
    if (!hometowns[town - 1].crime_list)
      continue;

    if (crime_find(hometowns[town - 1].crime_list, J_NAME(victim), NULL,
                   0, NOWHERE, J_STATUS_WANTED, NULL))
      is_wanted = TRUE;
  }

  if ((percent > ch_chance) || IS_TRUSTED(victim))
  {
    if (!IS_TRUSTED(victim) && !number(0, 3))
      notch_skill(ch, SKILL_CAPTURE, 3.33);

    SET_POS(ch, POS_PRONE + GET_STAT(ch));

    act("You try to grab $N, but find yourself face on the ground!", FALSE,
        ch, 0, victim, TO_CHAR);
    act("You avoid $n's grab as $e falls to the ground!", FALSE, ch, 0,
        victim, TO_VICT);
    act("$n falls face first on the ground as $N avoids $m.", FALSE, ch, 0,
        victim, TO_NOTVICT);

    notch_skill(ch, SKILL_CAPTURE, .67);

    CharWait(ch, PULSE_VIOLENCE * 5);

    if (!is_wanted)
    {
      justice_witness(ch, victim, CRIME_ATT_MURDER);
      if((GET_STAT(victim) > STAT_INCAP) && !IS_FIGHTING(ch) && !IS_DESTROYING(ch))
        set_fighting(ch, victim);
    }
  }
  else
  {
    /*
     * yup, two CharWaits(), this first one prevents all the silly
     * bash/flee things, can add more below if needed. JAB
     */
    CharWait(ch, PULSE_VIOLENCE * 5);
    CharWait(victim, PULSE_VIOLENCE * 1);

    stop_fighting(ch);
    stop_fighting(victim);
    stop_destroying(victim);

    SET_BIT(victim->specials.affected_by, AFF_BOUND);
    SET_POS(victim, POS_PRONE + GET_STAT(victim));

    act("You knock $N to the ground and bind $M up!", FALSE, ch, 0, victim,
        TO_CHAR);
    act("You are knocked to the ground and bound up by $n!", FALSE, ch, 0,
        victim, TO_VICT);
    act("$N is knocked to the ground and bound up by $n!", FALSE, ch, 0,
        victim, TO_NOTVICT);

    /* get rid of their rope.. */
    extract_obj(unequip_char(ch, HOLD), TRUE); // Arti rope?

    notch_skill(ch, SKILL_CAPTURE, 2.22);

    if (!is_wanted)
      justice_witness(ch, victim, CRIME_KIDNAPPING);

    return;
  }
#ifdef REALTIME_COMBAT
  if (ch->specials.fighting && !ch->specials.combat)
  {
    ch->specials.fighting = NULL;
    set_fighting(ch, victim);
  }
#endif
}

void do_capture(P_char ch, char *argument, int cmd)
{
  P_char   victim = NULL;

  SanityCheck(ch, "do_capture");

  victim = ParseTarget(ch, argument);

  capture(ch, victim);
}

void do_appraise(P_char ch, char *argument, int cmd)
{
  P_obj    temp = NULL;
  char     buf[MAX_STRING_LENGTH];
  int      percent;
  int      estimate_value;

  SanityCheck(ch, "do_appraise");

  one_argument(argument, buf);

  if (IS_AFFECTED(ch, AFF_BOUND))
  {
    send_to_char("Try to unbind yourself first!\r\n", ch);
    return;
  }

  if (!(temp = get_obj_in_list_vis(ch, buf, ch->carrying)))
  {
    act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  act("You examine $p closely!", FALSE, ch, temp, 0, TO_CHAR);
//    return;

  act("$n examines $p closely!", FALSE, ch, temp, 0, TO_ROOM);

  estimate_value = temp->cost;

  /* throw a number */
  percent = number(1, 101);     /* 101 is a complete failure */

  /* if it is a complete failure, then the appraise is way off */
  if (percent == 101)
  {
    if ((number(0, 1)))
      estimate_value += (estimate_value * (number(1, 3)));
    else
      estimate_value -= (estimate_value / (number(2, 4)));
  }
  else
  {
    if (percent > GET_CHAR_SKILL(ch, SKILL_APPRAISE))
    {
      notch_skill(ch, SKILL_APPRAISE, 5);
      if ((number(0, 2)))
        estimate_value += (estimate_value / (number(1, 15)));
      else
        estimate_value -= (estimate_value / (number(2, 15)));
    }
    else
    {
      if ((number(0, 1)))
        estimate_value += (estimate_value / (number(80, 120)));
      else
        estimate_value -= (estimate_value / (number(80, 120)));
    }
  }

  if (estimate_value <= 0)
    sprintf(buf, "This look like a piece of crap.\r\n");
  else
    sprintf(buf, "You estimate its value to: %s\r\n",
            coin_stringv(estimate_value));
  send_to_char(buf, ch);
  CharWait(ch, 2 * PULSE_VIOLENCE);

  return;
}

void do_chi(P_char ch, char *argument, int cmd)
{
  char     str[512], arg[512];
  int      skl_level;
  struct affected_type af;

  if (IS_NPC(ch))
    return;

  if (!GET_SPEC(ch, CLASS_MONK, SPEC_CHIMONK))
  {
    send_to_char("You lack the mental focus to perform this task\n\r", ch);
    return;
  }

  if (IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
      IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
  {
    send_to_char("Your nervous system is to pre-occupied at the moment.\n\r",
                 ch);
    return;
  }

  if (IS_RIDING(ch))
  {
    send_to_char("You cannot conventrate enough while mounted.\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (!*arg)
  {
    send_to_char
      ("You can focus your chi into the following enchantments:\n\rDisplacement\n\rReconstruction\n\rSight\n\r",
       ch);
    return;
  }

  skl_level = GET_CHAR_SKILL(ch, SKILL_CHI);

  if ((number(0, 101) - skl_level) > 0)
  {
    act("You falter as you try to summon your inner power...", FALSE, ch, 0,
        0, TO_CHAR);
    act
      ("$n's face looks frustrated as $e is unable to vanquish $s inner turmoil.",
       TRUE, ch, 0, 0, TO_ROOM);
    notch_skill(ch, SKILL_CHI, 10);
    CharWait(ch, PULSE_VIOLENCE * 3);
    return;
  }
  else
  {
    send_to_char("You focus your thoughts, harnessing your inner power.\n\r",
                 ch);
    if (is_abbrev(arg, "displacement"))
    {
      if (GET_POS(ch) < POS_STANDING)
      {
        send_to_char("You cannot concentrate while on the ground.\n\r", ch);
        return;
      }
      add_event(displacement_event, 2 * PULSE_VIOLENCE, ch, NULL, NULL, 0, NULL, 0);
      //AddEvent(EVENT_CHAR_EXECUTE, 2 * PULSE_VIOLENCE, TRUE, ch, displacement);
      CharWait(ch, PULSE_VIOLENCE * 2);
    }
    else if (is_abbrev(arg, "reconstruction"))
    {
      if (0 /*IS_AFFECTED5(ch, AFF5_LOTUS) */ )
      {
        skl_level = 0;
        skl_level = GET_CHAR_SKILL(ch, SKILL_RECONSTRUCTION);
        if ((number(0, 101) - skl_level) > 0)
        {
          send_to_char("You are unable to focus your thoughts.\n\r", ch);
          return;
        }
        send_to_char
          ("You turn your thoughts inward to your broken flesh.\n\rUsing your inner chi you cause your body to mend at an accelerated rate.\n\r",
           ch);
        act("&+cA strong sense of peace and calm emanates from $n&n.\n\r",
            FALSE, ch, 0, 0, TO_ROOM);
        memset(&af, 0, sizeof(af));
        af.type = SKILL_RECONSTRUCTION;
        af.flags = AFFTYPE_SHORT;
        af.duration = 20 * PULSE_VIOLENCE;
        af.location = APPLY_HIT_REG;
        af.modifier = 5 * GET_CHAR_SKILL(ch, SKILL_RECONSTRUCTION);
        affect_to_char(ch, &af);
        notch_skill(ch, SKILL_RECONSTRUCTION, 10);
        CharWait(ch, PULSE_VIOLENCE * 2);
      }
      else
      {
        send_to_char("You must clear your mind first.\n\r", ch);
        return;
      }
    }
    else if (is_abbrev(arg, "sight"))
    {
      if (0 /*IS_AFFECTED5(ch, AFF5_LOTUS) */ )
      {
        if (affected_by_spell(ch, SKILL_SIGHT))
          return;
        skl_level = 0;
        skl_level = GET_CHAR_SKILL(ch, SKILL_SIGHT);
        if ((number(0, 101) - skl_level) > 0)
        {
          send_to_char("You are unable to focus your thoughts.\n\r", ch);
          SET_BIT(ch->specials.act2, PLR2_SPEC_TIMER);
          return;
        }
        send_to_char
          ("You expand your mind uncloaking the mysteries of life.\n\r", ch);
        act("$n's eyes start to glow with a pale blue light.", FALSE, ch, 0,
            0, TO_ROOM);
        memset(&af, 0, sizeof(af));
        af.type = SKILL_SIGHT;
        af.duration = (GET_C_WIS(ch) / 10);
        af.bitvector = AFF_DETECT_INVISIBLE | AFF_SENSE_LIFE;
        af.bitvector2 = AFF2_DETECT_MAGIC;
        affect_to_char(ch, &af);
        notch_skill(ch, SKILL_SIGHT, 10);
        CharWait(ch, PULSE_VIOLENCE * 2);
      }
      else
      {
        send_to_char("You must clear your mind first.\n\r", ch);
        return;
      }
    }
    else
    {
      send_to_char("That is not an option.\n\r", ch);
      return;
    }
    notch_skill(ch, SKILL_CHI, 10);
  }
}

void displacement_event(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      skl_level;

  if (IS_NPC(ch))
    return;

  if (!GET_SPEC(ch, CLASS_MONK, SPEC_CHIMONK))
    return;

  if (GET_POS(ch) < POS_STANDING)
  {
    send_to_char("You cannot focus your mind while on the ground.\n\r", ch);
    return;
  }

  if (IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
      IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
  {
    send_to_char("Your nervous system is to pre-occupied at the moment.\n\r",
                 ch);
    return;
  }

  if (IS_RIDING(ch))
  {
    send_to_char("You cannot concentrate enough while mounted.\n\r", ch);
    return;
  }

  skl_level = GET_CHAR_SKILL(ch, SKILL_DISPLACEMENT);

  if ((number(1, 101) - skl_level) > 0)
  {
    send_to_char("You are unable to focus your thoughts.\n\r", ch);
    return;
  }
  spell_teleport(GET_LEVEL(ch), ch, 0, 0, ch, 0);
  CharWait(ch, PULSE_VIOLENCE * 3);
  notch_skill(ch, SKILL_DISPLACEMENT, 10);
}

void do_lotus(P_char ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return;

  if (!GET_SPEC(ch, CLASS_MONK, SPEC_CHIMONK))
  {
    send_to_char("You can not control your lotus chi!\n\r", ch);
    return;
  }

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("That would be un-wise!\n\r", ch);
    return;
  }

  if (IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
      IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
  {
    send_to_char("Your nervous system is to pre-occupied at the moment.\n\r",
                 ch);
    return;
  }

  if (0 /*IS_AFFECTED5(ch, AFF5_LOTUS) */ )
  {
    send_to_char("You are already in the lotus position.\n\r", ch);
    return;
  }

  if (GET_POS(ch) != POS_SITTING)
    do_sit(ch, "", CMD_LOTUS);

  send_to_char("You bend your legs with your hands on your knees.\n\r", ch);
  act("$n bends his legs and rests his hands on his knees.", FALSE, ch, 0, 0,
      TO_ROOM);
  
  add_event(lotus_event, 2 * PULSE_VIOLENCE, ch, NULL, NULL, 0, NULL, 0);
  //AddEvent(EVENT_CHAR_EXECUTE, PULSE_VIOLENCE * 2, TRUE, ch, lotus);
  CharWait(ch, PULSE_VIOLENCE);
}

void lotus_event(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      skl_level;

  if (IS_NPC(ch))
    return;

  if (!GET_SPEC(ch, CLASS_MONK, SPEC_CHIMONK))
    return;

  if (IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
      IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
  {
    send_to_char("Your nervous system is to pre-occupied at the moment.\n\r",
                 ch);
    return;
  }

  if(IS_FIGHTING(ch) || IS_DESTROYING(ch))
    return;

  if (IS_RIDING(ch))
  {
    send_to_char("You cannot concentrate while riding.\n\r", ch);
    return;
  }

  if (GET_POS(ch) != POS_SITTING)
  {
    send_to_char("You must be sitting to concetrate enough.\n\r", ch);
    return;
  }

  skl_level = GET_CHAR_SKILL(ch, SKILL_LOTUS);

  if ((number(0, 101) - skl_level) < 0)
  {
    act
      ("You slip beyond your conscious self and become one with the universe.",
       FALSE, ch, 0, 0, TO_CHAR);
    act("$n's face goes blank as $e enters a trance-like state.", FALSE, ch,
        0, 0, TO_ROOM);

/*      if(!IS_AFFECTED5(ch, AFF5_LOTUS))
         SET_BIT(ch->specials.affected_by5, AFF5_LOTUS);*/
    CharWait(ch, PULSE_VIOLENCE);
    notch_skill(ch, SKILL_LOTUS, 10);
    return;
  }

  send_to_char("You fail in your attempt to clear your mind of thoughts.\n\r",
               ch);
  CharWait(ch, PULSE_VIOLENCE * 2);

}

void do_true_strike(P_char ch, char *argument, int cmd)
{

  P_char   vict = NULL;
  char     name[100];
  int      skl_lvl = 0;
  int      x;
  int      victim_dead;

  if (IS_NPC(ch))
    return;

  if (!GET_SPEC(ch, CLASS_MONK, SPEC_CHIMONK))
    return;

  skl_lvl = GET_CHAR_SKILL(ch, SKILL_TRUE_STRIKE);

  vict = ParseTarget(ch, argument);
  if ( !vict )
  {
    send_to_char
      ("A true martial artist would know his opponent for certain before striking.\n\r",
       ch);
    return;
  }

  vict = guard_check(ch, vict);
  if (!vict)
  {
    send_to_char
      ("A true martial artist would know his opponent for certain before striking.\n\r",
       ch);
    return;
  }

  if (!on_front_line(ch) || !on_front_line(vict))
  {
    send_to_char("You can't reach them!\n\r", ch);
    return;
  }

  if (IS_SET(world[ch->in_room].room_flags, SINGLE_FILE) &&
      (!AdjacentInRoom(ch, vict)))
  {
    send_to_char("Your target is to far away for your punch to reach.\n\r",
                 ch);
    return;
  }

  if (GET_ALT_SIZE(vict) < (GET_ALT_SIZE(ch) - 1) ||
      (GET_ALT_SIZE(vict) > (GET_ALT_SIZE(ch) + 1)))
  {
    send_to_char
      ("Your punch would not be very effective on an opponent that size.\n\r",
       ch);
    return;
  }

  if (CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char
      ("You feel ashamed to try to disrupt the tranquility of this place.\n\r",
       ch);
    return;
  }

  if (!CanDoFightMove(ch, vict))
    return;

  if (number(1, 101) > skl_lvl)
  {
    damage(ch, vict, 0, SKILL_TRUE_STRIKE);
    send_to_char
      ("You feel like a fool as you swing and completely miss your target.\n\r",
       ch);
    notch_skill(ch, SKILL_TRUE_STRIKE, 10);
  }
  else
  {
    send_to_char
      ("You leap forward and deliver three strikes just below the ribs.\n\r",
       ch);
    act
      ("With lightning speed $n leaps forward and strikes $N three times below the ribs.",
       FALSE, ch, 0, vict, TO_NOTVICT);
    act
      ("With lightning speed $n leaps forward and strikes you three times just below the ribs.",
       FALSE, ch, 0, vict, TO_VICT);

    if (damage(ch, vict, (40 + dice(1, 12)) * 3, SKILL_TRUE_STRIKE))
      return;

    x = number(1, 100);

    if (x <= 20)
    {
      send_to_char("You fall to your knees screaming with agony!\n\r", vict);
      act("$n falls to his knees howling from the pain!", FALSE, vict, 0, 0,
          TO_ROOM);
      SET_POS(vict, POS_KNEELING + GET_STAT(ch));
      CharWait(vict, PULSE_VIOLENCE);
    }
    else if (x <= 40)
    {
      send_to_char
        ("The pain is to much to bear and totally numbs your senses.\n\r",
         vict);
      act("$n grimaces with pain.", FALSE, vict, 0, 0, TO_ROOM);
      Stun(vict, ch, PULSE_VIOLENCE * 2, FALSE);
    }
    else if (x <= 60)
    {
      send_to_char("You feel like your nervous system is on fire!\n\r", vict);
      send_to_char("You feel as tho you hit something vital!\n\r", ch);
      victim_dead =
        damage(ch, vict, (10 + dice(1, 12)) * 3, SKILL_TRUE_STRIKE);
    }
    notch_skill(ch, SKILL_TRUE_STRIKE, 10);
  }
  CharWait(ch, 2 * PULSE_VIOLENCE);
}

void chant_chi_purge(int level, P_char ch, char *arg, int type,
                      P_char victim, P_obj obj)
{
  int      poison, curse, wither, disease, blind;

  poison = FALSE;
  curse = FALSE;
  wither = FALSE;
  disease = FALSE;
  blind = FALSE;

  if (GET_LEVEL(ch) > 2)
    poison = TRUE;
  if (GET_LEVEL(ch) > 9)
    curse = TRUE;
  if (GET_LEVEL(ch) > 29)
    blind = TRUE;
  if (GET_LEVEL(ch) > 50)
    wither = TRUE;
  if (GET_LEVEL(ch) > 55)
    disease = TRUE;

  if( number(1, 100) > GET_CHAR_SKILL(ch, SKILL_CHI_PURGE)
    && !notch_skill(ch, SKILL_CHI_PURGE, 10) )
  {
    send_to_char("You forgot the words for the chant.\r\n", ch);
    notch_skill(ch, SKILL_CHI_PURGE, 15);
    CharWait(ch, 2 * PULSE_VIOLENCE);
    return;
  }

  if (!affect_timer(ch,
        WAIT_SEC * get_property("timer.secs.monkChipurge", 25),
        SKILL_CHI_PURGE))
  {
    send_to_char("Yer not in proper mood for that right now!\r\n", ch);
    return;
  }

  if (poison && (affected_by_spell(ch, SPELL_POISON) ||
                 IS_SET(victim->specials.affected_by2, AFF2_POISONED)))
  {
    act("&+GYou close your eyes, willing the poison in your bloodstream to dissipate.&n",
        FALSE, ch, 0, victim, TO_CHAR);
    poison_common_remove(victim);
  }

  if (curse && affected_by_spell(ch, SPELL_CURSE))
  {
    act("Harnessing your chi, you purify your mind and body of the curse.", FALSE, ch, 0,
        victim, TO_CHAR);
    affect_from_char(victim, SPELL_CURSE);
  }

  if (curse)
  {
    spell_remove_curse(level, ch, 0, 0, ch, NULL);
  }

  if (wither && affected_by_spell(ch, SPELL_WITHER))
  {
    act("You focus inward, causing your withered limbs to return to normal.",
        FALSE, ch, 0, victim, TO_CHAR);
    affect_from_char(victim, SPELL_WITHER);
  }

  if (disease && affected_by_spell(ch, SPELL_DISEASE))
  {
    act("&+rYou kneel, chanting, and begin to feel the effects of the disease fade away.",
        FALSE, ch, 0, victim, TO_CHAR);
    // affect_from_char(victim, SPELL_DISEASE);
  }

  if (disease)
  {
	  spell_cure_disease(level, ch, 0, 0, ch, NULL);
  }


  if (blind && IS_AFFECTED(ch, AFF_BLIND))
  {
    act("&+WYou close your eyes, chanting in a low tone, and when you open them once more, your vision has returned.", FALSE, ch, 0,
        victim, TO_CHAR);
    affect_from_char(victim, SPELL_BLINDNESS);
    if (IS_SET(victim->specials.affected_by, AFF_BLIND))
      REMOVE_BIT(victim->specials.affected_by, AFF_BLIND);
  }

  return;
}
