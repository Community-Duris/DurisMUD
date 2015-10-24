
/*
 * ***************************************************************************
 * *  File: limits.c                                           Part of Duris *
 * *  Usage: Procedures controlling gain and limit.
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "justice.h"
#include "sql.h"
#include "mm.h"
#include "map.h"
#include "trophy.h"
#include "assocs.h"
#include "alliances.h"
#include "defines.h"
#include "nexus_stones.h"
#include "boon.h"
#include "ctf.h"
#include "epic_bonus.h"

/*
 * external variables
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern const struct stat_data stat_factor[];
extern const struct max_stat max_stats[];
extern const struct racial_data_type racial_data[];
extern struct con_app_type con_app[];
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern struct time_info_data time_info;
extern P_index obj_index;
extern int last_update;
extern int get_innate_regeneration(P_char);
extern P_index mob_index;
struct mm_ds *dead_trophy_pool = NULL;
extern struct race_names race_names_table[];
extern float racial_exp_mods[LAST_RACE + 1];
extern float racial_exp_mod_victims[LAST_RACE + 1];

long     new_exp_table[TOTALLVLS];
long     global_exp_limit;

void     checkPeriodOfFame(P_char ch, char killer[1024]);
void     advance_skillpoints( P_char ch );
void     demote_skillpoints( P_char ch );

#if 0
#   define READ_TITLE(ch) (GET_SEX(ch) == SEX_MALE ?   \
        titles[GET_CLASS(ch) - 1][GET_LEVEL(ch)].title_m :  \
        titles[GET_CLASS(ch) - 1][GET_LEVEL(ch)].title_f)
#endif

/* * When age < base_age, return the value p0 */
/* * When age < 2 * base calculate the line between p1 & p2 */
/* * When age < 3 * base calculate the line between p2 & p3 */
/* * When age < 4 * base calculate the line between p3 & p4 */
/* * When age < 5 * base calculate the line between p4 & p5 */
/* * When age >= 80 return the value p6 */


int graf(P_char ch, int t_age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{
  int      a = 17;

  if (!t_age)
    t_age = p0;                 /* Somehow, we are occasionally passed an age of 0,
                                   which crashes us. This _might_ fix. */

  if (IS_PC(ch))
    a = racial_data[(int) GET_RACE(ch)].base_age;

  if (t_age < a)
    return (p0);                /* * < base_age   */
  else if (t_age <= 2 * a)
    return (int) (p1 + (((t_age - a) * (p2 - p1)) / a));        /* * <2x */
  else if (t_age <= 3 * a)
    return (int) (p2 + (((t_age - 2 * a) * (p3 - p2)) / a));    /* * <3x */
  else if (t_age <= 4 * a)
    return (int) (p3 + (((t_age - 3 * a) * (p4 - p3)) / a));    /* * <4x */
  else if (t_age <= 5 * a)
    return (int) (p4 + (((t_age - 4 * a) * (p5 - p4)) / a));    /* * <5x */
  else
    return (p6);                /* * >= 5x */
}

int vitality_limit(P_char ch)
{
  int max;
  int endurance = GET_CHAR_SKILL(ch, SKILL_IMPROVED_ENDURANCE);

  // Giving mounts epic endurance, since that's all they do.. (and tank sometimes).
  if( IS_NPC(ch) && IS_SET(ch->specials.act, ACT_MOUNT) )
  {
    endurance = 100;
  }

  if( IS_PC(ch) && (GET_AGE(ch) <= racial_data[GET_RACE(ch)].max_age) )
  {
    max = racial_data[(int) GET_RACE(ch)].base_vitality + ch->points.base_vitality
      + graf(ch, age(ch).year, 10, 20, 30, 40, 50, 60, 70);
  }
  else
  {
    // this is a hack.  remort'd undead will hit this condition almost always... they should use the
    // racial mod, but many undead had their base vit setbit... also some mobs might be using
    // the base vit (and not have a valid racial entry).  this will hopefully be a happy medium.
    max = ch->points.base_vitality ? ch->points.base_vitality : racial_data[(int) GET_RACE(ch)].base_vitality;
  }

  /* This is another pretty hack to increase movement points
   * due to having the IMPROVED ENDURANCE epic skill. Once
   * again, remove this or comment it out if this skill
   * ceases to exist.
   * Zion 11/1/07
   */
  if( endurance > 0 )
  {
    max += (endurance / 2);
  }

  return (max);
}

/*
 * calculate ch's mana regeneration rate, return regen/minute for mobs,
 * regen/hour for players.  Convert mobs to same scale when they cast like
 * players do.  JAB
 */

int mana_regen(P_char ch)
{
  int      gain;

  if (ch->points.mana_reg >= 0 && GET_MANA(ch) == GET_MAX_MANA(ch))
    return 0;

  if (GET_MANA(ch) > GET_MAX_MANA(ch))
  {
    gain = (GET_MAX_MANA(ch) - GET_MANA(ch)) / 5;
    return MIN(-1, gain);
  }

  if (IS_NPC(ch))
  {
    gain = GET_LEVEL(ch) >> 1;
  }
  else if (!IS_PUNDEAD(ch))
  {
    gain = graf(ch, age(ch).year, 10, 12, 14, 14, 10, 10, 6);
  }
  else
    gain = 20;

  if (affected_by_spell(ch, SPELL_FEEBLEMIND))
    gain >>= 2;

  switch (GET_POS(ch))
  {
  case STAT_DEAD:
  case STAT_DYING:
  case STAT_INCAP:
    gain = 0;
    break;
  case STAT_SLEEPING:
    gain <<= 1;
    break;
  case STAT_RESTING:
    gain += (gain >> 1);
    break;
  }

  if (IS_AFFECTED(ch, AFF_MEDITATE))
    gain += 5 + GET_CHAR_SKILL(ch, SKILL_MEDITATE);
  
  /*
  if (USES_MANA(ch))
  {
    if (GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION) >= 90)
    {
      gain = (int) (gain * 2.5);
              notch_skill(ch, SKILL_ADVANCED_MEDITATION, 2);
    }
    else
    {
      gain *= 2;
             notch_skill(ch, SKILL_ADVANCED_MEDITATION, 2);
    }
  }
  */

  gain += ch->points.mana_reg;

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
    gain = 0;

  if (has_innate(ch, INNATE_VULN_SUN) && IS_SUNLIT(ch->in_room) &&
     !IS_TWILIGHT_ROOM(ch->in_room) && !IS_AFFECTED4(ch, AFF4_GLOBE_OF_DARKNESS))
    gain = 0;

  
  return gain * gain / 8;
}

/* * calculate ch's hit regeneration rate, return regen/minute */

int hit_regen(P_char ch)
{
  int      gain;
  struct affected_type *af;

  if( affected_by_spell(ch, TAG_BUILDING) )
  {
    return 0;
  }

  if( ch->points.hit_reg >= 0 && GET_HIT(ch) == GET_MAX_HIT(ch) )
  {
    return 0;
  }

  if( GET_HIT(ch) > GET_MAX_HIT(ch) )
  {
    gain = (int) (GET_MAX_HIT(ch) - GET_HIT(ch)) / 10;
    return MIN(-1, gain);
  }

  if( IS_NPC(ch) )
  {
    gain = 14;
  }
  else
  {
    gain = graf(ch, age(ch).year, 16, 15, 14, 13, 11, 9, 6);
  }

  /* * Position calculations    */
  switch( GET_STAT(ch) )
  {
  case STAT_DEAD:
    gain = 0;                   /* * overrides normal gains */
    break;
  case STAT_DYING:
    gain = -2;                  /* * overrides normal gains */
    break;
  case STAT_INCAP:
    gain = -1;                  /* * overrides normal gains */
    break;
  case STAT_SLEEPING:
    gain += (gain < 0) ? (-(gain >> 1)) : (gain >> 1);  /* * 150% */
    break;
  case STAT_RESTING:
    gain += (gain < 0) ? (-(gain >> 2)) : (gain >> 2);  /* * 125% */
    break;
  }

  switch( GET_POS(ch) )
  {
  case POS_PRONE:
    gain += (gain < 0) ? (-(gain >> 2)) : (gain >> 2);  /* * 125% */
    break;
  case POS_KNEELING:
    gain += (gain < 0) ? (-(gain >> 4)) : (gain >> 4);  /* * 106% */
    break;
  case POS_SITTING:
    gain += (gain < 0) ? (-(gain >> 3)) : (gain >> 3);  /* * 113% */
    break;
  }

  if( GET_COND(ch, FULL) == 0 )
  {
    gain >>= 1;
  }
  if( GET_COND(ch, THIRST) == 0 )
  {
    gain >>= 1;
  }

  if( CHAR_IN_HEAL_ROOM(ch) && (GET_STAT(ch) >= STAT_SLEEPING) )
  {
    gain += GET_LEVEL(ch) * 2;
  }

  gain += ch->points.hit_reg;

  gain += (int)((float)gain * get_epic_bonus(ch, EPIC_BONUS_HEALTH));

  if( IS_AFFECTED4(ch, AFF4_REGENERATION) || has_innate(ch, INNATE_REGENERATION)
    || (has_innate(ch, INNATE_WOODLAND_RENEWAL) && (world[ch->in_room].sector_type == SECT_FOREST))
    || has_innate(ch, INNATE_ELEMENTAL_BODY))
  {
    gain += get_innate_regeneration(ch);
  }

  if( IS_AFFECTED4(ch, AFF4_TUPOR) )
  {
    gain += (int) (GET_LEVEL(ch) * 3.5);
  }

  if( CHAR_IN_NO_HEAL_ROOM(ch) && gain > 0 )
  {
    gain = 0;
  }

  if( gain == 0 && GET_STAT(ch) < STAT_SLEEPING )
  {
    gain = -1;
  }

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    for (af = ch->affected; af; af = af->next)
    {
      if (af->bitvector4 & AFF4_REGENERATION)
      {
        break;
      }
    }
    if (af)
    {
      ;
    }
    else if (IS_AFFECTED4(ch, AFF4_REGENERATION))
    {
      gain >>= 1;
    }
    else if (has_innate(ch, INNATE_WOODLAND_RENEWAL) && (world[ch->in_room].sector_type == SECT_FOREST)) //can regen in battle in forest - Drannak
    {
      gain >>= 1;
    }
    else
    {
      gain = 0;
    }
  }


  if( has_innate(ch, INNATE_VULN_SUN) && IS_SUNLIT(ch->in_room)
    && !IS_TWILIGHT_ROOM(ch->in_room) && !IS_AFFECTED4(ch, AFF4_GLOBE_OF_DARKNESS)
    && !IS_MAGIC_DARK(ch->in_room) )
  {
    gain = 0;
  }

  if( IS_AFFECTED3(ch, AFF3_SWIMMING) || IS_AFFECTED2(ch, AFF2_HOLDING_BREATH)
    || IS_AFFECTED2(ch, AFF2_IS_DROWNING) )
  {
    gain = 0;
  }

  return (gain);
}

/*
 * calculate ch's move regeneration rate, return regen/minute
 */

int move_regen(P_char ch)
{
  float gain;
  int endurance;

  if( !IS_ALIVE(ch) )
  {
    return 0;
  }

  endurance = GET_CHAR_SKILL(ch, SKILL_IMPROVED_ENDURANCE);

  // Giving mounts epic endurance, since that's all they do.. (and tank sometimes).
  if( IS_NPC(ch) && IS_SET(ch->specials.act, ACT_MOUNT) )
  {
    endurance = 100;
  }

  if( ch->points.move_reg >= 0 && GET_VITALITY(ch) == GET_MAX_VITALITY(ch) )
  {
    return 0;
  }

  if( IS_AFFECTED3(ch, AFF3_SWIMMING) || IS_AFFECTED2(ch, AFF2_HOLDING_BREATH)
    || IS_AFFECTED2(ch, AFF2_IS_DROWNING) || IS_FIGHTING(ch) || IS_DESTROYING(ch) || IS_STUNNED(ch) )
  {
    return 0;
  }

  if(GET_VITALITY(ch) > GET_MAX_VITALITY(ch))
  {
    gain = (GET_MAX_VITALITY(ch) - GET_VITALITY(ch)) >> 2;
    return (int) MIN(-1, gain);
  }
  
  if(IS_NPC(ch) || IS_UNDEADRACE(ch) || IS_ANGEL(ch))
  {
    gain = 22;
  }
  else
  {
    gain = graf(ch, age(ch).year, 14, 20, 20, 16, 14, 12, 11);
  }

  if (GET_COND(ch, FULL) == 0)
    gain /= 1.250;
  if (GET_COND(ch, THIRST) == 0)
    gain /= 1.250;

  /*
   * Position calculations
   */
  switch (GET_STAT(ch))
  {
    case STAT_DEAD:
    case STAT_DYING:
    case STAT_INCAP:
      gain = 0;
      break;
    case STAT_SLEEPING:
      gain *= get_property("move.regen.sleeping", 1.75);
      break;
    case STAT_RESTING:
      gain *= get_property("move.regen.resting", 1.33);
      break;
    default:
      break;
  }

  switch (GET_POS(ch))
  {
    case POS_PRONE:
      gain *= get_property("move.regen.prone", 1.25);
      break;
    case POS_KNEELING:
    case POS_SITTING:
      gain *= get_property("move.regen.sitting", 1.13);
      break;
    default:
      break;
  }

  if (gain || ch->points.move_reg < 0)
    gain += ch->points.move_reg;

  if(gain > 0 &&
    IS_AFFECTED4(ch, AFF4_TUPOR))
      gain += 20;
 
  gain += (int)((float)gain * get_epic_bonus(ch, EPIC_BONUS_MOVES));

  /* This is another pretty hack to increase movement points
   * due to having the IMPROVED ENDURANCE epic skill. Once
   * again, remove this or comment it out if this skill
   * ceases to exist.
   * Zion 11/1/07
   */

  if (endurance > 0)
    gain += (endurance / 10);

  if (GET_RACE(ch) == RACE_QUADRUPED)
    gain += (number(0, 6));

  return (int) (gain * gain / 5);
}

void gain_practices(P_char ch)
{
#if 0
  if (IS_NPC(ch))
    return;

  switch (GET_CLASS(ch))
  {
  case CLASS_SORCERER:
  case CLASS_NECROMANCER:
  case CLASS_CONJURER:
  case CLASS_SUMMONER:
  case CLASS_SHAMAN:
  case CLASS_CLERIC:
  case CLASS_DRUID:
    ch->only.pc->spells_to_learn +=
      MAX(2, wis_app[STAT_INDEX(GET_C_WIS(ch))].bonus + 2) + number(0, 1);
    break;
  default:
    ch->only.pc->spells_to_learn +=
      MAX(1, wis_app[STAT_INDEX(GET_C_WIS(ch))].bonus + 1) + number(0, 1);
    break;
  }
#endif
}

void lose_practices(P_char ch)
{
#if 0
  if (IS_NPC(ch))
    return;

  switch (GET_CLASS(ch))
  {
  case CLASS_SORCERER:
  case CLASS_NECROMANCER:
  case CLASS_CONJURER:
  case CLASS_SUMMONER:
  case CLASS_SHAMAN:
  case CLASS_CLERIC:
  case CLASS_DRUID:
    ch->only.pc->spells_to_learn -=
      MAX(2, wis_app[STAT_INDEX(GET_C_WIS(ch))].bonus + 2) + 1;
    break;
  default:
    ch->only.pc->spells_to_learn +=
      MAX(1, wis_app[STAT_INDEX(GET_C_WIS(ch))].bonus + 1) + 1;
    break;
  }
#endif
}

 /* give a gith his sword/staff of death if he's hit 50th for the first
    time */
void githyanki_weapon(P_char ch)
{
  P_obj    sword;
  char     strn[256];

  if (GET_CLASS(ch, CLASS_NECROMANCER) || GET_CLASS(ch, CLASS_SORCERER) ||
      GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_CONJURER) ||
      GET_CLASS(ch, CLASS_ILLUSIONIST) || GET_CLASS(ch, CLASS_SUMMONER) )
  {
    sword = read_object(19, VIRTUAL);
    if (sword)
    {
      sprintf(strn, "staff silverish silver %sz", GET_NAME(ch));
      sword->str_mask = STRUNG_KEYS;
      sword->name = str_dup(strn);

      obj_to_char(sword, ch);
      send_to_char
        ("&+CWith a blinding flash of light, a staff suddenly appears in your hands!\r\n",
         ch);
      act
        ("&+CIn a blinding flash of light, a staff suddenly appears in $n's hands!",
         FALSE, ch, 0, 0, TO_ROOM);
    }
  }
  else if (GET_CLASS(ch, CLASS_WARRIOR) || GET_CLASS(ch, CLASS_REAVER))
  {
    sword = read_object(418, VIRTUAL);
    if (sword)
    {
      sprintf(strn, "warhammer hammer silverish silver %sz",
              GET_NAME(ch));
      sword->str_mask = STRUNG_KEYS;
      sword->name = str_dup(strn);

      obj_to_char(sword, ch);
      send_to_char
        ("&+CWith a blinding flash of light, a warhammer suddenly appears in your hands!\r\n",
         ch);
      act
        ("&+CIn a blinding flash of light, a warhammer suddenly appears in $n's hands!",
         FALSE, ch, 0, 0, TO_ROOM);
    }
  }
  else if (GET_CLASS(ch, CLASS_ANTIPALADIN))
  {
    sword = read_object(18, VIRTUAL);
    if (sword)
    {
      sprintf(strn, "longsword sword long silverish silver %sz",
              GET_NAME(ch));
      sword->str_mask = STRUNG_KEYS;
      sword->name = str_dup(strn);

      obj_to_char(sword, ch);
      send_to_char
        ("&+CWith a blinding flash of light, a sword suddenly appears in your hands!\r\n",
         ch);
      act
        ("&+CIn a blinding flash of light, a sword suddenly appears in $n's hands!",
         FALSE, ch, 0, 0, TO_ROOM);
    }
  }
        else
      send_to_char("&+CYou should have gotten a special item.  Let a god know.\r\n",
                   ch);
}

/*
 * Gain in various points
 */

void illithid_advance_level(P_char ch)
{
  int minlvl = get_property("exp.maxExpLevel", 46);
  int i;

  if (GET_LEVEL(ch) >= 56)
    return;

  for (i = GET_LEVEL(ch) + 1; i > minlvl && (new_exp_table[i] <= GET_EXP(ch)); i++)
  {
    GET_EXP(ch) -= new_exp_table[i];
    advance_level(ch);
  }
}

void advance_level(P_char ch)
{
/*  struct time_info_data playing_time;*/
  int      add_mana = 0, i;
  int      prestige = 100;
  /* level normally, please
   *
   *
 ///TODO CODE THIS PIECE OF MASTER    */

  ch->player.level++;
  sql_update_level(ch);

  if( GET_LEVEL(ch) > 1 )
  {
    sql_log(ch, PLAYERLOG, "Advanced to level %d", GET_LEVEL(ch));
  }

  // If we're at a circle level & use slots...
  if( GET_LEVEL(ch) % 5 == 1 && USES_SPELL_SLOTS(ch) )
  {
    // Zero out the new spell slots gained..
    ch->specials.undead_spell_slots[(GET_LEVEL(ch)+4)/5] = 0;
  }

  send_to_char("&+WYou raise a level!&N\r\n", IS_SET(ch->specials.act, PLR_MORPH) ? ch->only.pc->switched : ch);
  logit(LOG_LEVEL, "Level %2d: %s", GET_LEVEL(ch), GET_NAME(ch));
  ch->only.pc->prestige++;

  if( IS_PC(ch) && GET_A_NUM(ch) && ch->only.pc->highest_level < GET_LEVEL(ch) && ch->group )
  {
    int group_size = 1;
    for( struct group_list *gl = ch->group; gl; gl = gl->next )
    {
      if( IS_PC(gl->ch) && gl->ch != ch && (GET_A_NUM(gl->ch) == GET_A_NUM(ch)) && gl->ch->in_room == ch->in_room )
        group_size++;
    }

    if( group_size >= get_property("prestige.guildedInGroupMinimum", 0) )
    {
      int prestige = get_property("prestige.gain.leveling", 0);

      send_to_char("&+bYour guild gained prestige!\r\n", ch);
      prestige = check_nexus_bonus(ch, prestige, NEXUS_BONUS_PRESTIGE);
      add_assoc_prestige(GET_A_NUM(ch), prestige);
    }
  }

  /* level out skills */
//#ifdef SKILLPOINTS
//  advance_skillpoints( ch );
//#else
  update_skills(ch);
//#endif

/*
  if (GET_LEVEL(ch) == 21 && !IS_NEWBIE(ch) ) {
    REMOVE_BIT(ch->specials.act2, PLR2_NCHAT);
  }*/

  if (GET_LEVEL(ch) == 35) {
    REMOVE_BIT(ch->specials.act2, PLR2_NEWBIE);
 //   REMOVE_BIT(ch->specials.act2, PLR2_NCHAT);
  }

  if (IS_PC(ch) && IS_GITHYANKI(ch) && (GET_LEVEL(ch) == 50) &&
      (ch->only.pc->highest_level < 50))
    githyanki_weapon(ch);

  if (IS_PC(ch) && (ch->only.pc->highest_level < GET_LEVEL(ch)))
    ch->only.pc->highest_level = GET_LEVEL(ch);

  if ((GET_LEVEL(ch) == get_property("exp.maxExpLevel", 45)) && !IS_HARDCORE(ch) && (!GET_RACE(ch) == RACE_PLICH))
  {
    char buf[512];
    sprintf(buf,
        "You are now level %d and are considered among the high level adventurers\n"
        "of Duris!  The path now set before you is a difficult one, as you must now\n"
        "battle the higher forces of the realms to further your conquest!\n",
        get_property("exp.maxExpLevel", 45));
    send_to_char(buf, ch);
  }

  /*
   * hitpoint gain
   */

  if (GET_LEVEL(ch) < 26)
  {
    ch->points.base_hit += (IS_ILLITHID(ch) ? 0 :number(0, 3));
    ch->points.base_mana += number(0, 3);
  }

  if (GET_HIT(ch) > GET_MAX_HIT(ch))
    GET_HIT(ch) = GET_MAX_HIT(ch);

  if (GET_LEVEL(ch) > 56)
    for (i = 0; i < 3; i++)
      ch->specials.conditions[i] = -1;

  affect_total(ch, FALSE);
  update_pos(ch);

  check_boon_completion(ch, NULL, 0, BOPT_LEVEL);
}

/*
 * lose in various points
 */

void lose_level(P_char ch)
{
  int      i;

  if (GET_LEVEL(ch) < 2)
    return;
  if (IS_HARDCORE(ch) && (GET_LEVEL(ch) > 49))
    return;

  send_to_char("&=LRYou lose a level!&N\r\n",
               IS_SET(ch->specials.act, PLR_MORPH) ?
               ch->only.pc->switched : ch);
  logit(LOG_LEVEL, "%s lost a level! (%d)", GET_NAME(ch), GET_LEVEL(ch));

  forget_spells(ch, -1);
  
  /*
   * hitpoint loss
   */
  if (GET_LEVEL(ch) < 26)
  {
    ch->points.base_hit = MAX(1, (ch->points.base_hit - 3));
    ch->points.base_mana = MAX(0, (ch->points.base_mana - 3));
  }

  ch->player.level = MAX(1, ch->player.level - 1);
  sql_update_level(ch);

//#ifdef SKILLPOINTS
//  demote_skillpoints(ch);
//#else
  update_skills(ch);
//#endif

  if (GET_LEVEL(ch) < MINLVLIMMORTAL)
    for (i = 0; i < 3; i++)
      ch->specials.conditions[i] = 0;

  balance_affects(ch);
}

void set_title(P_char ch)
{
  if (GET_TITLE(ch))
  {

    FREE(ch->player.title);
    ch->player.title = 0;
  }
}

void display_gain(P_char ch, int gain)
{
  char     buffer[MAX_STRING_LENGTH];
  P_char   tch;

  // only display PC's
  if( IS_NPC(ch) || ch->in_room < 0 || ch->in_room > top_of_world )
    return;

  if( GET_LEVEL(ch) >= MINLVLIMMORTAL )
    sprintf(buffer, "&+yExperience&n: %s would have gained %d.\n", GET_NAME(ch), gain);
  else
    sprintf(buffer, "&+yExperience&n: %s by %d.\n", GET_NAME(ch), gain);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (IS_TRUSTED(tch) && IS_SET(tch->specials.act2, PLR2_EXP))
    {
      send_to_char(buffer, tch);
    }
  }
}

void update_exp_table()
{
  char     buf[128];
  int      i;

  new_exp_table[0] = 0;
  global_exp_limit = 0;

  fprintf(stderr, "Generating exp table.\n");
  for (i = 1; i <= MAXLVL; i++)
  {
    // Changed this so we start at exp.required.1 and can set each value
    //   up to exp.required.62.  If no value is set, take the previous.
    // If you change this back, need to reset values in duris.properties.
//    sprintf(buf, "exp.required.%d", ((i + 4) / 5) * 5);
    sprintf(buf, "exp.required.%d", i);
    new_exp_table[i] = get_property(buf, -1);
    // If exp.required.i not found, set to i-1's value.
    if( new_exp_table[i] == -1 )
    {
      // Default lvl 1 exp is 2k.  But lvl 1 exp property should be set.
      new_exp_table[i] = (i==1) ? 2000 : new_exp_table[i-1];
    }
    global_exp_limit += new_exp_table[i];
  }
}

float gain_exp_modifiers_race_only(P_char ch, P_char victim, float XP)
{
  char prop_buf[128];

// debug("Gain exp race (%d) Start.", (int)XP);

  if(ch &&
     GET_RACE(ch) >= 1 &&
     GET_RACE(ch) <= LAST_RACE )
  {
    XP = XP * racial_exp_mods[GET_RACE(ch)];
  }
// debug("Gain exp ch (%d) Start.", (int)XP);  
  if(victim &&
     GET_RACE(victim) >= 1 &&
     GET_RACE(victim) <= LAST_RACE )
  {
    XP = XP * racial_exp_mod_victims[GET_RACE(victim)];
  }
// debug("Gain exp victim (%d) Start.", (int)XP);   
  prop_buf[0];
// debug("Gain exp race End (%d).", (int)XP);
  return XP;
}

float gain_exp_modifiers(P_char ch, P_char victim, float XP)
{

// debug("Gain exp modifiers (%d).", (int)XP);

  if(victim)
  {
    if(CHAR_IN_TOWN(ch) &&
      (GET_LEVEL(victim) > 20) &&
      !IS_PC(victim))
    {
      XP = XP * get_property("gain.exp.mod.victim.location.hometown", 1.00);
      if(!number(0, 49)) // Limit the spam
        send_to_char("&+gThis being a hometown, you receive fewer exps...&n\r\n", ch);
    } 
      
    if(!IS_MULTICLASS_NPC(victim) &&
        GET_LEVEL(victim) > 20 &&
        !IS_PC(victim))
    {
      if(GET_CLASS(victim, CLASS_WARRIOR))
        XP = XP * get_property("gain.exp.mod.warrior", 1.00);
      else if(GET_CLASS(victim, CLASS_MERCENARY))
        XP = XP * get_property("gain.exp.mod.mercenary", 1.00);
      else if(GET_CLASS(victim, CLASS_ROGUE))
        XP = XP * get_property("gain.exp.mod.rogue", 1.00);
      else if(GET_CLASS(victim, CLASS_AVENGER))
        XP = XP * get_property("gain.exp.mod.avenger", 1.00);
      else if(GET_CLASS(victim, CLASS_DREADLORD))
        XP = XP * get_property("gain.exp.mod.dreadlord", 1.00);
      else if(GET_CLASS(victim, CLASS_SORCERER))
        XP = XP * get_property("gain.exp.mod.sorcerer", 1.00);
      else if(GET_CLASS(victim, CLASS_CONJURER))
        XP = XP * get_property("gain.exp.mod.conjurer", 1.00);
      else if(GET_CLASS(victim, CLASS_NECROMANCER))
        XP = XP * get_property("gain.exp.mod.necromancer", 1.00);
      else if(GET_CLASS(victim, CLASS_NECROMANCER))
        XP = XP * get_property("gain.exp.mod.theurgist", 1.00);
      else if(GET_CLASS(victim, CLASS_PSIONICIST))
        XP = XP * get_property("gain.exp.mod.psionicist", 1.00);
      else if(GET_CLASS(victim, CLASS_PALADIN))
        XP = XP * get_property("gain.exp.mod.paladin", 1.00);
      else if(GET_CLASS(victim, CLASS_ANTIPALADIN))
        XP = XP * get_property("gain.exp.mod.antipaladin", 1.00);
      else if(GET_CLASS(victim, CLASS_ETHERMANCER))
        XP = XP * get_property("gain.exp.mod.ethermancer", 1.00);
      else if(GET_CLASS(victim, CLASS_BERSERKER))
        XP = XP * get_property("gain.exp.mod.berserker", 1.00);
      else if(GET_CLASS(victim, CLASS_MONK))
        XP = XP * get_property("gain.exp.mod.monk", 1.00);
      else if(GET_CLASS(victim, CLASS_CLERIC))
        XP = XP * get_property("gain.exp.mod.cleric", 1.00);
      else if(GET_CLASS(victim, CLASS_SHAMAN))
        XP = XP * get_property("gain.exp.mod.shaman", 1.00);
      else if(GET_CLASS(victim, CLASS_DRUID))
        XP = XP * get_property("gain.exp.mod.druid", 1.00);
      else if(GET_CLASS(victim, CLASS_RANGER))
        XP = XP * get_property("gain.exp.mod.ranger", 1.00);
      else if(GET_CLASS(victim, CLASS_MINDFLAYER))
        XP = XP * get_property("gain.exp.mod.mindflayer", 1.00);
      else if(GET_CLASS(victim, CLASS_REAVER))
        XP = XP * get_property("gain.exp.mod.reaver", 1.00);
      else if(GET_CLASS(victim, CLASS_SUMMONER))
        XP = XP * get_property("gain.exp.mod.summoner", 1.00);
      else
        XP = XP * get_property("gain.exp.mod.other", 1.00);
    }
  
    // Aggro mobs yield more exp
    if (aggressive_to_basic(victim, ch))
    {
      XP = XP * get_property("gain.exp.mod.victim.act.aggro", 1.25);
    }
    
    // Careful with the breath modifier since many greater race mobs have a breathe weapon.
    if(CAN_BREATHE(victim))
    {
      XP = XP * get_property("gain.exp.mod.victim.ability.breath.weapon", 1.00);
    }
    
    if(IS_ELITE(victim))
    {
      XP = XP * get_property("gain.exp.mod.victim.elite", 1.00);
    }
    
    if(IS_SET(victim->specials.act, ACT_HUNTER))
    {
      XP = XP * get_property("gain.exp.mod.victim.act.hunter", 1.00);
    }
    
    if(!IS_PC(victim) &&
       !IS_SET(victim->specials.act, ACT_MEMORY))
    {
      XP = XP * get_property("gain.exp.mod.victim.act.nomemory", 1.00);
    }
    
    if(GET_CLASS(ch, CLASS_PALADIN) &&
       IS_GOOD(victim))
    {
      XP = XP * get_property("exp.factor.paladin.vsGood", 0.2);
    }
    
    if(GET_CLASS(ch, CLASS_PALADIN) &&
       IS_EVIL(victim))
    {
      XP = XP * get_property("exp.factor.paladin.vsEvil", 1.1);
    }
    
    if(GET_CLASS(ch, CLASS_ANTIPALADIN) &&
       IS_GOOD(victim))
    {
      XP = XP * get_property("exp.factor.antipaladin.vsGood", 1.05);
    }
  }
      
// Exp penalty for classes that advance too quickly.
  if(GET_CLASS(ch, CLASS_NECROMANCER) &&
     GET_LEVEL(ch) < 31)
        XP = XP * get_property("gain.exp.mod.player.necro", 1.00);
  else if(GET_CLASS(ch, CLASS_NECROMANCER))
    XP = XP * get_property("gain.exp.mod.player.necro.tier", 1.00);

  // Exp penalty for classes that advance too quickly.
  if(GET_CLASS(ch, CLASS_MERCENARY))
    XP = XP * get_property("gain.exp.mod.player.merc", 1.00);
    
  // Exp bonus for clerics, since we really need this class above all else.
  if(!IS_MULTICLASS_PC(ch) &&
     GET_CLASS(ch, CLASS_CLERIC))
      XP = XP * get_property("gain.exp.mod.player.cleric", 1.00);

  if(GET_LEVEL(ch) >= 31)
    XP = XP * get_property("gain.exp.mod.player.level.thirtyone", 1.000);
  if(GET_LEVEL(ch) >= 41)
    XP = XP * get_property("gain.exp.mod.player.level.fortyone", 1.000);
  if(GET_LEVEL(ch) >= 51)
    XP = XP * get_property("gain.exp.mod.player.level.fiftyone", 1.000);
  if(GET_LEVEL(ch) >= 55)
    XP = XP * get_property("gain.exp.mod.player.level.fiftyfive", 1.000);    

// debug("Gain exps mofidiers (%d).", (int)XP);

  return XP;
}

float gain_global_exp_modifiers(P_char ch, float XP)
{
  if (RACE_GOOD(ch))
    XP = XP * get_property("exp.factor.racewar.good", 1.0);
  else if (RACE_EVIL(ch))
    XP = XP * get_property("exp.factor.racewar.evil", 1.0);
  else if (RACE_PUNDEAD(ch))
    XP = XP * get_property("exp.factor.racewar.undead", 1.0);
  XP = XP * get_property("exp.factor.global", 1.0);

  return XP;
}

// Percentage of exp gained.
int exp_mod(P_char killer, P_char victim)
{
  int      diff, mod;

  // High difference -> high lvl killing lowbies
  diff = GET_LEVEL(killer) - GET_LEVEL(victim);
  if( diff > 40 )
  {
    mod = 0;
  }
  else if( diff > 30 )
  {
    mod = 1;
  }
  else if( diff > 20 )          /* 21 - 30  */
  {
    mod = 3;
  }
  else if( diff > 15 )          /* 16 - 20  */
  {
    mod = 10;
  }
  else if( diff > 10 )          /* 11 - 15  */
  {
    mod = 20;
  }
  else if( diff > 5 )           /*  6 - 10  */
  {
    mod = 55;
  }
  else if( diff > 2 )           /*  3 -  5  */
  {
    mod = 90;
  }
  else if( diff >= 0 )          /*  0 -  2  */
  {
    mod = 100;
  }
  else if (diff > -3)           /* -2 - 1    */
    mod = 120;
  else if (diff > -6)           /* -6 - -3    */
    mod = 125;
  else if (diff > -10)           /* -9 - -6    */
    mod = 130;
  else if (diff > -15)          /* -14 - -10    */
    mod = 135;
  else if (diff > -20)          /* -19 - -15    */
    mod = 140;
  else                          /* < -20 */
    mod = 150;

  return mod;
}

int gain_exp(P_char ch, P_char victim, const int value, int type)
{
  int goodcap = get_property("exp.level.cap.good", 15);
  int evilcap = get_property("exp.level.cap.evil", 15);
  bool pvp = FALSE;
  float XP = MAX(1, value);
  P_char master;

  if( ch && IS_PC(ch) )
  {
    ch = GET_PLYR(ch);
  }
  else
  {
    return 0;
  }

// debug("check 1 exp (%d:%d).", type, value);
  if( CHAR_IN_ARENA(ch) || IS_SET(world[ch->in_room].room_flags, GUILD_ROOM | SAFE_ZONE) )
  {
    return 0;
  }

  if( victim && type != EXP_RESURRECT )
  {
    if( IS_PC_PET(victim) || IS_SHOPKEEPER(victim)
      || IS_SET(world[victim->in_room].room_flags, GUILD_ROOM | SAFE_ZONE) )
    {
      return 0;
    }
  }

  if(ch && victim && IS_PC(ch) && IS_PC(victim))
  {
    if( opposite_racewar(ch, victim) )
    {
      pvp = TRUE;
    }

    if( (master = GET_MASTER(ch)) && opposite_racewar(master, victim) )
    {
      pvp = TRUE;
    }
  }

  if( type == EXP_RESURRECT )
  {
    ;
  }
  else if( affected_by_spell(ch, TAG_WELLRESTED) )
  {
    XP *= 2;
  }
  else if( affected_by_spell(ch, TAG_RESTED) )
  {
    XP *= 1.5;
  }

  if(type == EXP_RESURRECT)
  {
    ;
  }
  else if(type == EXP_DAMAGE)
  {
    if(ch == victim)
    {
// debug("Damage to self exp gain returning 0", XP);
      return 0;
    }

    if(IS_PC(ch) && IS_PC(victim))
    {
// debug("Pvp damage exp returning 0", XP);
      return 0;
    }

    // damaging mob to death summarily yields same exp as kill itself
    // +30 is + 10 to account for incap damage and + 20 for lowbie mobs
    // with very low hps
    XP = (XP / (GET_MAX_HIT(victim) + 30)) * GET_EXP(victim);

    // When someone else is tanking mob you damage, they get tanking exp
    P_char tank = GET_OPPONENT(victim);
    if (tank && tank != ch && IS_PC(tank) && grouped(tank, ch))
    {
      // powerleveling stopgap
      if (GET_LEVEL(tank) >= GET_LEVEL(ch) - (RACE_GOOD(ch) ? goodcap : evilcap))
      {
        gain_exp(tank, victim, XP, EXP_TANKING);
      }
    }

    XP = XP * get_property("exp.factor.damage", 1.00);
// debug("damage 1 exp gain (%d)", (int)XP);
    XP = gain_global_exp_modifiers(ch, XP);
// debug("damage 2 exp gain (%d)", (int)XP);
    XP = XP * exp_mod(ch, victim) / 100;
// debug("damage 3 exp gain (%d)", (int)XP);
    XP = modify_exp_by_zone_trophy(ch, type, XP);
// debug("damage 4 exp gain (%d)", (int)XP);
    XP = gain_exp_modifiers(ch, victim, XP);
// debug("damage 5 exp gain (%d)", (int)XP);
    XP = gain_exp_modifiers_race_only(ch, victim, XP);
// debug("damage 6 exp gain (%d)", (int)XP);
    XP = check_nexus_bonus(ch, (int)XP, NEXUS_BONUS_EXP);
// debug("damage 7 exp gain (%d)", (int)XP);
    XP = XP + (int)((float)XP * get_epic_bonus(ch, EPIC_BONUS_EXP));
  }
  else if(type == EXP_HEALING)
  {
    if (!victim)
        return 0;

    if (victim != ch && !grouped(victim, ch)) // only for healing self and groupies
        return 0;

    P_char attacker = GET_OPPONENT(victim);
    if (!attacker) // only for healing in fight
        return 0;

    if ((GET_LEVEL(victim) <= GET_LEVEL(ch) - (GOOD_RACE(ch) ? goodcap : evilcap)) || 
	    (GET_LEVEL(victim) >= GET_LEVEL(ch) + (GOOD_RACE(ch) ? goodcap : evilcap)))  // powerleveling stopgap
    {
      return 0;
    }

    XP = ((XP + 10) / 5) * ((GET_LEVEL(ch) + GET_LEVEL(victim)) / 2);
    XP = XP * get_property("exp.factor.healing", 1.00);

// debug("healing 1 (%d)", (int)XP);
    if(!GET_CLASS(ch, CLASS_CLERIC) &&
       !GET_SPEC(ch, CLASS_SHAMAN, SPEC_SPIRITUALIST))
    {
      XP = XP * get_property("exp.factor.healing.class.penalty", 0.50);
    }

// debug("healing 2 (%d)", (int)XP);
    if(IS_NPC(victim))
      XP /= 2;
    if(ch == victim)
      XP = XP / 2;
// debug("healing 3 (%d)", (int)XP);
    XP = gain_global_exp_modifiers(ch, XP);
// debug("healing 4 (%d)", (int)XP);
    XP = XP * exp_mod(ch, attacker) / 100;
// debug("healing 5 (%d)", (int)XP);
    XP = modify_exp_by_zone_trophy(ch, type, XP);
// debug("healing 6 (%d)", (int)XP);
    XP = gain_exp_modifiers(ch, attacker, XP);
// debug("healing 7 (%d)", (int)XP);
    XP = gain_exp_modifiers_race_only(ch, attacker, XP);
// debug("healing 8 (%d)", (int)XP);
    XP = check_nexus_bonus(ch, (int)XP, NEXUS_BONUS_EXP);
// debug("healing 9 (%d)", (int)XP);
  }
  else if(type == EXP_TANKING)
  {
    float group_size = 1;
    if (ch->group)
    {
      for (struct group_list *gl = ch->group; gl; gl = gl->next)
      {
        if(gl->ch != ch && IS_PC(gl->ch) && !IS_TRUSTED(gl->ch) 
           && ch->in_room == gl->ch->in_room)
        {
          group_size = group_size + 1;
        }
      }
    }
    if (group_size > 1)
        XP = XP / (group_size + 1);
    else
        return 0;

    XP = XP * get_property("exp.factor.tanking", 1.00);
// debug("tanking 1 (%d)", (int)XP);
    XP = gain_global_exp_modifiers(ch, XP);
// debug("tanking 2 (%d)", (int)XP);
    XP = XP * exp_mod(ch, victim) / 100;
// debug("tanking 3 (%d)", (int)XP);
    XP = modify_exp_by_zone_trophy(ch, type, XP);
// debug("tanking 4 (%d)", (int)XP);
    XP = gain_exp_modifiers(ch, victim, XP);
// debug("tanking 5 (%d)", (int)XP);
    XP = gain_exp_modifiers_race_only(ch, victim, XP);
// debug("tanking 6 (%d)", (int)XP);
    XP = check_nexus_bonus(ch, (int)XP, NEXUS_BONUS_EXP); 
// debug("tanking 7 (%d)", (int)XP);
  }
  else if(type == EXP_MELEE)
  {
    if(ch == victim)
    {
// debug("Self-melee returning 0");
      return 0;
    }
    if(IS_PC(ch) && IS_PC(victim) && !(pvp)) // Do not provide exps to same side combat melee exps.
    {
// debug("Same side melee returning 0");
      return 0;
    }
    XP = XP * GET_LEVEL(victim) * get_property("exp.factor.melee", 1.00);  // Just a small boost for lowbies, becomes insignificant at higher levels  -Odorf
// debug("melee 1 exp gain (%d)", (int)XP);
    if (GET_LEVEL(victim) < GET_LEVEL(ch) - 5)
        XP = XP / (1 + (GET_LEVEL(ch) - GET_LEVEL(victim) + 5) / 2); // no exp flow from fighting small mobs
// debug("melee 2 exp gain (%d)", (int)XP);
    XP = gain_global_exp_modifiers(ch, XP);
// debug("melee 3 exp gain (%d)", (int)XP);
    XP = XP * exp_mod(ch, victim) / 100;
// debug("melee 4 exp gain (%d)", (int)XP);
    XP = modify_exp_by_zone_trophy(ch, type, XP);
// debug("melee 5 exp gain (%d)", (int)XP);
    XP = gain_exp_modifiers(ch, victim, XP);
// debug("melee 6 exp gain (%d)", (int)XP);
    XP = gain_exp_modifiers_race_only(ch, victim, XP);
// debug("melee 7 exp gain (%d)", (int)XP);
    XP = check_nexus_bonus(ch, (int)XP, NEXUS_BONUS_EXP);
// debug("melee 8 exp gain (%d)", (int)XP);
  }
  else if( type == EXP_DEATH )
  {
    float mod = get_property("exp.factor.global", 1.0);

    // Goods don't lose exp on death untill over the threshold.
    if( RACE_GOOD(ch) && GET_LEVEL(ch) < (int) get_property("exp.goodieDeathExpLossLevelThreshold", 20) )
    {
      return 0;
    }

    XP = -1 * (new_exp_table[GET_LEVEL(ch) + 1] * get_property("exp.death.level.loss", 0.10));
    // We reduce the exp loss from death by 2x of the global modifier if the global modifier is less than 1/2.
    // So, .4 -> 80% modifier, .3 -> 60% modifier, .15 -> 30% modifier, etc.
    if( mod < .5 )
      XP = XP * 2 * mod;
// debug("death 1 exp gain (%d)", (int)XP);
  }
  else if(type == EXP_KILL)
  {
    if(ch == victim)
    {
// debug("Self-kill returning 0");
      return 0;
    }

// No exps for killing your friends.
    if(IS_PC(ch) && IS_PC(victim) && !(pvp) && !CHAR_IN_TOWN(ch))
    {
// debug("Same side kill returning 0");
      return 0;
    }

// Hard coding goodie anti-griefing code for hometowns. Oct09 -Lucrot
// This is a pure pvp mud.  Learn to intergrate into the pbase.
/*
       if(IS_PC(ch) &&
       IS_PC(victim) &&
       CHAR_IN_TOWN(ch) &&
       GOOD_RACE(ch) &&
       GOOD_RACE(victim) &&
       GET_LEVEL(victim) >= (int)(get_property("pvp.good.level.grief.victim", 20)) &&
       GET_LEVEL(ch) >= (int)(get_property("pvp.good.level.grief.ch", 20)) ||
       IS_PC_PET(ch))
    {
      XP = -1 * (new_exp_table[GET_LEVEL(ch) + 1] >> 4);
      send_to_char("&+WThe divine forces of &+RDuris &+Wfrowns upon you...\r\n", ch);
      send_to_char("&+WArcing bolts of energy drain away your life.\r\n", ch);
      send_to_char("&+RA blood red aura surrounds you.\r\n", ch);

      struct affected_type af;
      bzero(&af, sizeof(af));

      af.type = SPELL_CURSE;
      af.flags = AFFTYPE_NODISPEL | AFFTYPE_PERM;
      af.modifier = 20;
      af.duration = 25;

      af.location = APPLY_SAVING_SPELL;
      affect_to_char(ch, &af);

      af.location = APPLY_SAVING_BREATH;
      affect_to_char(ch, &af);

      af.location = APPLY_SAVING_PARA;
      affect_to_char(ch, &af);

      af.location = APPLY_SAVING_FEAR;
      affect_to_char(ch, &af);

      af.modifier = 2;
      af.type = SPELL_SLOW;
      affect_to_char(ch, &af);
    }
    else
    {
    */
      if (!IS_PC(victim))
      {
        XP = XP * get_property("exp.factor.kill", 1.00) ;
// debug("kill 1 exp gain (%d)", (int)XP);
        XP = gain_global_exp_modifiers(ch, XP);
// debug("kill 2 exp gain (%d)", (int)XP);
        XP = XP * exp_mod(ch, victim) / 100;
// debug("kill 3 exp gain (%d)", (int)XP);
        XP = modify_exp_by_zone_trophy(ch, type, XP);
// debug("kill 4 exp gain (%d)", (int)XP);
        XP = gain_exp_modifiers(ch, victim, XP);
// debug("kill 5 exp gain (%d)", (int)XP);
        XP = gain_exp_modifiers_race_only(ch, victim, XP);
// debug("kill 6 exp gain (%d)", (int)XP);
        XP = check_nexus_bonus(ch, (int)XP, NEXUS_BONUS_EXP); 
// debug("kill 7 exp gain (%d)", (int)XP);
      }
      logit(LOG_EXP, "KILL EXP: %s (%d) killed by %s (%d): old exp: %d, new exp: %d, +exp: %d",
        GET_NAME(victim), GET_LEVEL(victim), GET_NAME(ch), GET_LEVEL(ch), GET_EXP(ch), GET_EXP(ch) + (int)XP, (int)XP);
    //}

    if(pvp)
    {
      XP = XP * get_property("gain.exp.mod.pvp", 1.000);
// debug("kill 8 exp gain (%d)", (int)XP);
    }
    check_boon_completion(ch, victim, XP, BOPT_MOB);
    check_boon_completion(ch, victim, XP, BOPT_RACE);
  }
  else if(type == EXP_WORLD_QUEST)
  {
    XP = gain_exp_modifiers_race_only(ch, NULL, XP);
// debug("world quest 1 (%d)", (int)XP);
  }
  else if(type == EXP_QUEST)
  {
    XP = gain_exp_modifiers_race_only(ch, NULL, XP);
// debug("quest 1 (%d)", (int)XP);
  }

  int XP_final = (int)XP;
// debug("check 3 xp (%d)", XP_final);
  // Resses from Gods may restore more than 1/3 level (ie for liches).
  if( type != EXP_RESURRECT )
  {
    int range = new_exp_table[GET_LEVEL(ch) + 1] / 3;
// debug("check 4 xp (%d)", XP_final);
    XP_final = BOUNDED(-range, XP_final, range);
  }

  // if(XP_final > 0 &&
     // GET_EXP(ch) > (new_exp_table[GET_LEVEL(ch) + 1]));
  // {
    // XP_final = 1;
    // send_to_char("&+LYour exps are capped and you must gain a level to accumulate more exps.\r\n", ch);
  // }

  // increase exp only to some limit (cumulative exp for mortals)
  if( GET_LEVEL(ch) < MINLVLIMMORTAL && (XP_final < 0 || GET_EXP(ch) < global_exp_limit) )
  {
    GET_EXP(ch) += (int)XP_final;
  }
  display_gain(ch, (int)XP_final);
  if( GET_LEVEL(ch) >= MINLVLIMMORTAL )
  {
    return 0;
  }

  if (XP_final > 0)
  {
    // Hardcores should level via exp only. - Drannak 11/30/12
    // Liches can lvl exp only too since they are solo on 3rd racewar side (again). 7/7/2015
    if( (IS_HARDCORE(ch) || GET_RACE(ch) == RACE_PLICH) && (GET_LEVEL(ch) <= 55) )
    {
      for(int i = GET_LEVEL(ch) + 1;i <= 56 && (new_exp_table[i] <= GET_EXP(ch)); i++ )
  	  {
      	GET_EXP(ch) -= new_exp_table[i];
      	advance_level(ch);
  	  }
    }
    else
    {
      for( int i = GET_LEVEL(ch) + 1; (i <= get_property("exp.maxExpLevel", 46)) &&  (new_exp_table[i] <= GET_EXP(ch)); i++ )
      {
        GET_EXP(ch) -= new_exp_table[i];
        advance_level(ch);
      }
    }
  }
  else
  {
    while (GET_EXP(ch) < 0)
    {
      logit(LOG_EXP, "LOSING LEVEL: %s - old exp: %d, new exp %d, difference: %d",
        J_NAME(ch), GET_EXP(ch), GET_EXP(ch)+new_exp_table[GET_LEVEL(ch)], new_exp_table[GET_LEVEL(ch)] );
      GET_EXP(ch) += new_exp_table[GET_LEVEL(ch)];
      lose_level(ch);
    }
  }

  // Check boon exp modifier
  // This is a exp bonus for any exp gotten in zone?
  if( type != EXP_BOON && type != EXP_DEATH && type != EXP_RESURRECT )
  {
    check_boon_completion(ch, victim, (int)XP, BOPT_NONE);
  }
// debug("Gain exps final return (%d).", XP_final);
  return XP_final;
}

int gain_condition(P_char ch, int condition, int value)
{
  int      intoxicated = 0, i, num;

  GET_COND(ch, FULL) = -1;
  GET_COND(ch, THIRST) = -1;

  for (i = 0; i < 3; i++)
  {
    if ((condition == ALL_CONDS) || (i == condition))
    {
      if (GET_COND(ch, i) == -1)
        continue;
      if (i == DRUNK)
        intoxicated = (GET_COND(ch, DRUNK) > 0);

      GET_COND(ch, i) = BOUNDED(0, (GET_COND(ch, i) + value), 24);

//        GET_COND(ch,i) = -1; /* prolly unnecessary but who knows */
      /* hunger/thirst removed as per tripods orders */


      if (IS_TRUSTED(ch))
        continue;

      if (CHAR_IN_TOWN(ch) &&
          ch->in_room == real_room(hometowns[CHAR_IN_TOWN(ch) - 1].jail_room))
        continue;

      if (GET_RACE(ch) == RACE_ILLITHID && i != FULL)
        continue;

      if (IS_PC(ch) && ch->only.pc->switched)
        continue;

      switch (i)
      {
      case FULL:
        if (GET_COND(ch, i) >= 0)
        {
          if (!GET_COND(ch, i))
//          send_to_char("&+RYou feel yourself weakening from intense hunger..\r\n", ch);
            send_to_char("&+RYou are hungry.\r\n", ch);
          else if (GET_COND(ch, i) < 3 &&
                   !IS_SET(ch->specials.act, PLR_WRITE))
            send_to_char("&+mYou are getting hungry.\r\n", ch);
          if (IS_AFFECTED3(ch, AFF3_FAMINE))
            GET_COND(ch, i) = 0;
/*        if (!GET_COND(ch,i))
          {
            num = (GET_MAX_HIT(ch) / 48) + 4;
            if ((GET_HIT(ch) - num) < 1) num = GET_HIT(ch) - 1;

            if (damage(ch,ch,num,TYPE_UNDEFINED)) return TRUE;
          }*/
        }
        break;
      case THIRST:
        if (GET_COND(ch, i) >= 0)
        {
          if (!GET_COND(ch, i))
          {
//          send_to_char("&+RYou feel yourself weakening from intense thirst..\r\n", ch);
            send_to_char("&+RYou are thirsty.\r\n", ch);

/*            num = (GET_MAX_HIT(ch) / 24) + 8;
            if ((GET_HIT(ch) - num) < 1) num = GET_HIT(ch) - 1;

            if (damage(ch, ch, num, TYPE_UNDEFINED)) return TRUE;*/
          }
          else if (GET_COND(ch, i) < 3 &&
                   !IS_SET(ch->specials.act, PLR_WRITE))
            send_to_char("&+mYou are getting thirsty.\r\n", ch);
        }
        break;
      case DRUNK:
        if (intoxicated && !GET_COND(ch, i))
          send_to_char("&+mYou are now sober.\r\n", ch);
        break;
      default:
        break;
      }
    }
  }

  return FALSE;
}

void point_update(void)
{
  P_char   i, i_next;
  char     Gbuf1[MAX_STRING_LENGTH];

  *Gbuf1 = '\0';

  for (i = character_list; i; i = i_next)
  {
    i_next = i->next;
    if (IS_NPC(i))
      continue;

    if (i->desc && (i->desc->connected != CON_PLYNG))
      continue;

    if (!IS_AFFECTED(i, AFF_WRAITHFORM))
      if (gain_condition(i, ALL_CONDS, -1))
        continue;

    /* this is idle rent */

    /* DO NOT IDLE RENT PEOPLE WHO ARE MORPHED!!!!!!!!! */
    if (IS_SET(i->specials.act, PLR_MORPH))
      continue;
      
// No idle rent for imprisonment as it crashes the game. Mar09 -Lucrot
    if(IS_SET(i->specials.affected_by5, AFF5_IMPRISON))
    {
      continue;
    }
  
    /* it's an int, but >3 digits would mess up do_users */

    if (i->specials.timer < 999)
      i->specials.timer++;

    if (i->specials.timer > 3)
      if (!IS_SET(i->specials.act, PLR_AFK))
      {
        SET_BIT(i->specials.act, PLR_AFK);
#if defined (CTF_MUD) && (CTF_MUD == 1)
	if (affected_by_spell(i, TAG_CTF))
	{
	  send_to_char("You're idling has caused you to forget about the flag.\r\n", i);
	  while (affected_by_spell(i, TAG_CTF))
	    drop_ctf_flag(i);
	}
#endif
      }

    if ((i->specials.timer > 15) &&
        ((GET_LEVEL(i) <= 56) || ((!i->desc) && (GET_LEVEL(i) > 58))))
    {
      act("$n disappears into the void.", TRUE, i, 0, 0, TO_ROOM);
      send_to_char("You have been idle, and are pulled into a void.\r\n", i);

      logit(LOG_COMM, "idle rent for %s in %d.",
            GET_NAME(i), world[i->in_room].number);
      sql_log(i, CONNECTLOG, "Idle rent");
      
      if (!GET_NAME(i))
        continue;

      // Getting tired of this giving free recalls.
      /*
      int reloghere = GET_BIRTHPLACE(i);
      
      if (!reloghere)
        reloghere = GET_HOME(i);
      if (!reloghere)
        reloghere = GET_ORIG_BIRTHPLACE(i);
      if (!reloghere)
        reloghere = i->in_room;
      */
      
      strcat(Gbuf1, GET_NAME(i));
      strcat(Gbuf1, ", ");
      //writeCharacter(i, 5, reloghere);
      writeCharacter(i, 5, i->in_room);
      extract_char(i);
      if (i->desc)
        close_socket(i->desc);
      /* extract_char frees memory.
      else
        free_char(i);
      */
      continue;
    }
  }

  if (*Gbuf1)
  {
    Gbuf1[strlen(Gbuf1) - 2] = '.';
    Gbuf1[strlen(Gbuf1) - 1] = '\0';
    statuslog(57, "Idle rent for: %s", Gbuf1);
  }
}

