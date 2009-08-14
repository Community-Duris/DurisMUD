
/*
 * ***************************************************************************
 * *  File: limits.c                                           Part of Duris *
 * *  Usage: Procedures controling gain and limit.
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

/*
 * external variables
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_obj object_list;
extern P_room world;
extern const int exp_table[TOTALLVLS];
extern const struct stat_data stat_factor[];
extern const struct max_stat max_stats[];
extern const struct racial_data_type racial_data[];
extern int top_of_world;
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

long      new_exp_table[TOTALLVLS];
long     global_exp_limit;

void     checkPeriodOfFame(P_char ch, char killer[1024]);

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


int graf(P_char ch, int t_age,
         int p0, int p1, int p2, int p3, int p4, int p5, int p6)
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
  int      max;
  int      endurance = GET_CHAR_SKILL(ch, SKILL_IMPROVED_ENDURANCE);

  if (IS_PC(ch) && (GET_AGE(ch) <= racial_data[GET_RACE(ch)].max_age))
    max =
      racial_data[(int) GET_RACE(ch)].base_vitality +
      ch->points.base_vitality + graf(ch, age(ch).year, 10, 20, 30, 40, 50,
                                      60, 70);
  else
    // this is a hack.  remort'd undead will hit this condition almost always... they should use the
    // racial mod, but many undead had their base vit setbit... also some mobs might be using
    // the base vit (and not have a valid racial entry).  this will hopefully be a happy medium.
    max = ch->points.base_vitality ? ch->points.base_vitality : racial_data[(int) GET_RACE(ch)].base_vitality;

  /* This is another pretty hack to increase movement points
   * due to having the IMPROVED ENDURANCE epic skill. Once
   * again, remove this or comment it out if this skill
   * ceases to exist.
   * Zion 11/1/07
   */
  if (endurance > 0)
  max += (endurance / 2);

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
    gain = graf(ch, age(ch).year, 12, 18, 22, 21, 14, 10, 6);
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

  if (USES_MANA(ch))
  {
    if (GET_CHAR_SKILL(ch, SKILL_ADVANCED_MEDITATION) >= 90)
    {
      gain = (int) (gain * 2.5);
    }
    else
    {
      gain *= 2;
    }
  }

  if (IS_AFFECTED(ch, AFF_MEDITATE))
    gain += 5 + GET_CHAR_SKILL(ch, SKILL_MEDITATE);

  gain += ch->points.mana_reg;

  if (IS_FIGHTING(ch))
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

  if (affected_by_spell(ch, TAG_BUILDING))
    return 0;
  
  if (ch->points.hit_reg >= 0 && GET_HIT(ch) == GET_MAX_HIT(ch))
    return 0;

  if (GET_HIT(ch) > GET_MAX_HIT(ch))
  {
    gain = (int) (GET_MAX_HIT(ch) - GET_HIT(ch)) / 10;
    return MIN(-1, gain);
  }
  if (IS_NPC(ch))
  {
    gain = 14;
  }
  else
  {
    gain = graf(ch, age(ch).year, 16, 15, 14, 13, 11, 9, 6);
  }

  /* * Position calculations    */
  switch (GET_STAT(ch))
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

  switch (GET_POS(ch))
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

  if (GET_COND(ch, FULL) == 0)
    gain >>= 1;
  if (GET_COND(ch, THIRST) == 0)
    gain >>= 1;

  if (CHAR_IN_HEAL_ROOM(ch) && (GET_STAT(ch) >= STAT_SLEEPING))
    gain += GET_LEVEL(ch) * 2;

  gain += ch->points.hit_reg;

  if (IS_AFFECTED4(ch, AFF4_REGENERATION) ||
      has_innate(ch, INNATE_REGENERATION) ||
      has_innate(ch, INNATE_ELEMENTAL_BODY))
    gain += get_innate_regeneration(ch);

  if (IS_AFFECTED4(ch, AFF4_TUPOR))
    gain += (int) (GET_LEVEL(ch) * 3.5);

  if (CHAR_IN_NO_HEAL_ROOM(ch) && gain > 0)
    gain = 0;

  if (gain == 0 && GET_STAT(ch) < STAT_SLEEPING)
    gain = -1;

  if (IS_FIGHTING(ch))
  {
    for (af = ch->affected; af; af = af->next)
      if (af->bitvector4 & AFF4_REGENERATION)
        break;
    if (af)
      ;
    else if (IS_AFFECTED4(ch, AFF4_REGENERATION))
      gain >>= 1;
    else
      gain = 0;
  }
  
  if (has_innate(ch, INNATE_VULN_SUN) && IS_SUNLIT(ch->in_room) &&
     !IS_TWILIGHT_ROOM(ch->in_room))
    gain = 0;

  if (IS_AFFECTED3(ch, AFF3_SWIMMING) || IS_AFFECTED2(ch, AFF2_HOLDING_BREATH)
      || IS_AFFECTED2(ch, AFF2_IS_DROWNING))
    gain = 0;

  return (gain);
}

/*
 * calculate ch's move regeneration rate, return regen/minute
 */

int move_regen(P_char ch)
{
  float gain;
  int endurance = GET_CHAR_SKILL(ch, SKILL_IMPROVED_ENDURANCE);

  if(!(ch) ||
     !IS_ALIVE(ch))
        return 0;
        
  if(ch->points.move_reg >= 0 &&
     GET_VITALITY(ch) == GET_MAX_VITALITY(ch))
        return 0;

  if(IS_AFFECTED3(ch, AFF3_SWIMMING) ||
     IS_AFFECTED2(ch, AFF2_HOLDING_BREATH) ||
     IS_AFFECTED2(ch, AFF2_IS_DROWNING) ||
     IS_FIGHTING(ch) ||
     IS_STUNNED(ch))
        return 0;
        

  if(GET_VITALITY(ch) > GET_MAX_VITALITY(ch))
  {
    gain = (GET_MAX_VITALITY(ch) - GET_VITALITY(ch)) >> 2;
    return (int) MIN(-1, gain);
  }
  
  if(IS_NPC(ch) || IS_UNDEADRACE(ch))
  {
    gain = 22;
  }
  else
  {
    gain = graf(ch, age(ch).year, 22, 25, 25, 22, 20, 16, 14);
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

  if (gain)
    gain += ch->points.move_reg;

  if(gain > 0 &&
    IS_AFFECTED4(ch, AFF4_TUPOR))
      gain += 20;
  
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
      GET_CLASS(ch, CLASS_ILLUSIONIST))
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
      send_to_char("&+Cyou should've gotten a special item..  let a god know.\r\n",
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
  
  send_to_char("&+WYou raise a level!&N\r\n",
               IS_SET(ch->specials.act, PLR_MORPH) ?
               ch->only.pc->switched : ch);
  logit(LOG_LEVEL, "Level %2d: %s", GET_LEVEL(ch), GET_NAME(ch));
  ch->only.pc->prestige++;

  if( IS_PC(ch) && 
     GET_A_NUM(ch) && 
     ch->only.pc->highest_level < GET_LEVEL(ch) &&
     ch->group )
  {
    int group_size = 1;
    for( struct group_list *gl = ch->group; gl; gl = gl->next )
    {
      if( IS_PC(gl->ch) && 
          gl->ch != ch &&
          ( (GET_A_NUM(gl->ch) == GET_A_NUM(ch)) || is_allied_with(GET_A_NUM(ch), GET_A_NUM(gl->ch)) ) &&
         gl->ch->in_room == ch->in_room )
	group_size++;
    }
    
    if( group_size >= (int) get_property("guild.prestige.groupSizeMinimum", 3) )
    {
      send_to_char("&+bYour guild gained some prestige!\r\n", ch);
      prestige = check_nexus_bonus(ch, prestige, NEXUS_BONUS_PRESTIGE);
      add_assoc_prestige(GET_A_NUM(ch), prestige);      
    }
  }
  
  /* level out skills */
  update_skills(ch);

  if (GET_LEVEL(ch) == 21 && !IS_NEWBIE(ch) ) {
    REMOVE_BIT(ch->specials.act2, PLR2_NCHAT);
  }

  if (GET_LEVEL(ch) == 35) {
    REMOVE_BIT(ch->specials.act2, PLR2_NEWBIE);
    REMOVE_BIT(ch->specials.act2, PLR2_NCHAT);
  }

  if (IS_PC(ch) && IS_GITHYANKI(ch) && (GET_LEVEL(ch) == 50) &&
      (ch->only.pc->highest_level < 50))
    githyanki_weapon(ch);

  if (IS_PC(ch) && (ch->only.pc->highest_level < GET_LEVEL(ch)))
    ch->only.pc->highest_level = GET_LEVEL(ch);

  if (GET_LEVEL(ch) == get_property("exp.maxExpLevel", 45)) {
    char buf[512];
    sprintf(buf, 
        "You are now level %d and are considered among "
        "the high level adventurers\n"
        "of Duris!  The path now set before you is a difficult one, "
        "as you must now\n"
        "battle the higher forces of the realms to further your conquest!\n",
        get_property("exp.maxExpLevel", 45));
    send_to_char(buf, ch);
  }

  /*
   * hitpoint gain
   */

  if (GET_LEVEL(ch) < 26) {
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

  update_skills(ch);

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
  if (IS_NPC(ch))
    return;

  sprintf(buffer, "Experience: %s by %d\n", GET_NAME(ch), gain);

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
    sprintf(buf, "exp.required.%d", ((i + 4) / 5) * 5);
    new_exp_table[i] = (int) get_property(buf, i * i * 1000);
    global_exp_limit += new_exp_table[i]; 
  }
}

int exp_mod(P_char k, P_char victim)
{
  int      diff, mod;

  diff = GET_LEVEL(k) - GET_LEVEL(victim);
  if (diff > 40)
    mod = 1;
  else if (diff > 30)           /* 31+    */
    mod = 3;
  else if (diff > 20)           /* 21-30    */
    mod = 10;
  else if (diff > 15)           /* 16-20    */
    mod = 20;
  else if (diff > 10)           /* 11-15    */
    mod = 30;
  else if (diff > 5)            /* 6-10    */
    mod = 55;
  else if (diff > 2)            /* 3-5    */
    mod = 90;
  else if (diff > -5)           /* -2 - 2    */
    mod = 100;
  else if (diff > -10)           /* -6 - -3    */
    mod = 107;
  else if (diff > -21)          /* -10 - -7    */
    mod = 115;
  else
    mod = 125;                  /* < -10 */

  return mod;
}

int gain_exp(P_char ch, P_char victim, const int value, int type)
{
  int      i, range;
  double   new_xp, XP;
  bool pvp = FALSE;

  if (GET_LEVEL(ch) >= MINLVLIMMORTAL || CHAR_IN_ARENA(ch))
    return 0;

  if (victim)
    victim = GET_PLYR(victim);

  if (type == EXP_DAMAGE)
  {
    if (!victim || IS_PC(victim) || ch == victim)
    {
      return 0;      
    }
    else
    {
      new_xp = (int) ( GET_LEVEL(ch) * GET_LEVEL(victim) * (float) get_property("exp.factor.damage", 2.0) );
    }
  }
  else if (type == EXP_HEALING)
  {
    if (!victim || IS_PC(victim))
    {
      return 0;      
    }
    else
    {
      new_xp = (int) ( GET_LEVEL(ch) * GET_LEVEL(victim) * (float) get_property("exp.factor.healing", 4.0) );      
    }
  }
  else if (type == EXP_MELEE)
  {
    new_xp = ( GET_LEVEL(ch) * GET_LEVEL(victim) * (float) get_property("exp.factor.melee", 0.1) );
  }
  else if (type == EXP_DEATH)
  {
    if( RACE_GOOD(ch) && GET_LEVEL(ch) < (int) get_property("exp.goodieDeathExpLossLevelThreshold", 30) )
    {      
      new_xp = 0;
    }
    else if (GET_LEVEL(ch) > 50)
    {
      new_xp = -(new_exp_table[GET_LEVEL(ch) + 1] >> 5);      
    }
    else
    {
      new_xp = -(new_exp_table[GET_LEVEL(ch) + 1] >> 3);          
    }
  }
  else if (type == EXP_KILL)
  {
    if( IS_PC(ch) && IS_PC(victim) )
    {
      pvp = TRUE;
      
      if ( (RACE_EVIL(ch) && RACE_EVIL(victim)) ||
           (RACE_GOOD(ch) && RACE_GOOD(victim)) )
        new_xp = 0;
      else if (GET_LEVEL(victim) < 20)    //check if victim is higher then 20
        new_xp = 0;
      else if ((GET_LEVEL(ch) - GET_LEVEL(victim)) > 15)  //check if diff is bigger then 15
        new_xp = 0;
      else                        //else give this..
        new_xp = value; //GET_LEVEL(victim) * (int) get_property("exp.racewar.perLevel", 100000);
      
      debug("pvp exp (%s [%d] killed %s [%d]): %d", GET_NAME(ch), GET_LEVEL(ch), GET_NAME(victim), GET_LEVEL(victim), (int) new_xp);
      logit(LOG_DEBUG, "pvp exp (%s [%d] killed %s [%d]): %d", GET_NAME(ch), GET_LEVEL(ch), GET_NAME(victim), GET_LEVEL(victim), (int) new_xp);
    }
    else
    {
      new_xp = value;
    }
    new_xp = new_xp * get_property("exp.factor.kill", 1.0);
  }
  else
  {
    new_xp = value;    
  }

  if (victim && IS_NPC(victim) && affected_by_spell(victim, TAG_REDUCED_EXP))
  {
    new_xp *= 0.5;
  }

  XP = (int) new_xp;
  ch = GET_PLYR(ch);
    
  if (IS_NPC(ch))
    return 0;
 
  if (XP > 0 && victim)
  {
 
// Non multi-class mobs exp modifiers are below.
    if(!IS_MULTICLASS_NPC(victim))
    {
      if(GET_LEVEL(victim) <= 20)
      { }
      else if(GET_CLASS(victim, CLASS_WARRIOR))
        XP *= 0.50;
      else if(GET_CLASS(victim, CLASS_MERCENARY))
        XP *= 0.50;
      else if(GET_CLASS(victim, CLASS_ROGUE))
        XP *= 0.50;
      else if(GET_CLASS(victim, CLASS_AVENGER))
        XP *= 0.75;
      else if(GET_CLASS(victim, CLASS_DREADLORD))
        XP *= 0.75;
      else if(GET_CLASS(victim, CLASS_SORCERER))
        XP *= 1.25;
      else if(GET_CLASS(victim, CLASS_CONJURER))
        XP *= 1.25;
      else if(GET_CLASS(victim, CLASS_NECROMANCER))
        XP *= 1.25;
      else if(GET_CLASS(victim, CLASS_PSIONICIST))
        XP *= 1.25;
      else if(GET_CLASS(victim, CLASS_PALADIN))
        XP *= 1.25;
      else if(GET_CLASS(victim, CLASS_ANTIPALADIN))
        XP *= 1.25;
      else if(GET_CLASS(victim, CLASS_ETHERMANCER))
        XP *= 1.15;
      else if(GET_CLASS(victim, CLASS_BERSERKER))
        XP *= 0.50;
      else if(GET_CLASS(victim, CLASS_MONK))
        XP *= 0.75;
      else if(GET_CLASS(victim, CLASS_CLERIC))
        XP *= 0.85;
      else if(GET_CLASS(victim, CLASS_SHAMAN))
        XP *= 1.25;
      else if(GET_CLASS(victim, CLASS_DRUID))
        XP *= 1.25;
      else
        XP *= 0.50;
    }
    
    if(GET_LEVEL(victim) > 20)
    {
      if(IS_ANIMAL(victim) ||
         IS_SLIME(victim))
        XP *= 0.50;
    }
    
    if(GET_RACE(victim) == RACE_DROW)
      XP *= 1.15;
    
    if(GET_RACE(victim) == RACE_GREY)
      XP *= 1.15;
      
    if(GET_RACE(victim) == RACE_PLICH)
      XP *= 1.15;
      
    if(IS_SET(victim->specials.act, ACT_HUNTER))
      XP *= 1.15;
      
    if(IS_GREATER_RACE(victim))
      XP *= 2.00;
    else if(IS_ELITE(victim))
      XP *= 2.00;
    else if(GET_RACE(victim) == RACE_GIANT)
      XP *= 1.50;
    else if(IS_ELEMENTAL(victim))
      XP *= 1.30;
    else if(IS_UNDEADRACE(victim))
      XP *= 1.30;
      
    if(!IS_PC(ch) &&
       !IS_SET(ch->specials.act, ACT_MEMORY))
      XP *= 0.25;
    
    if(IS_PC_PET(victim))
      return 0;

    if(IS_SHOPKEEPER(victim))
      return 0;
      
    if(IS_SET(world[victim->in_room].room_flags, GUILD_ROOM | ROOM_HOUSE | SAFE_ZONE))
      return 0;
      
    if(CHAR_IN_TOWN(ch) && (GET_LEVEL(victim) > 20) && !IS_PC(victim))
      XP = (int) (XP * 0.2);

    if(!GET_EXP(victim))
      return 0;

    XP = (XP * exp_mod(ch, victim)) / 100;

    if(GET_CLASS(ch, CLASS_PALADIN) && IS_GOOD(victim))
      XP = (int) (XP * get_property("exp.factor.paladin.vsGood", 0.2));

    if(GET_CLASS(ch, CLASS_PALADIN) && IS_EVIL(victim))
      XP = (int) (XP * get_property("exp.factor.paladin.vsEvil", 1.1));

    if(GET_CLASS(ch, CLASS_ANTIPALADIN) && IS_GOOD(victim))
      XP = (int) (XP * get_property("exp.factor.antipaladin.vsGood", 1.05));

    /* Racial experience */
    if(GET_RACE(ch) >= 1 && GET_RACE(ch) <= LAST_RACE )
    {
      char prop_buf[128];
      sprintf(prop_buf, "exp.factor.%s", race_names_table[GET_RACE(ch)].no_spaces);
      XP = (int) ( XP * (float) get_property(prop_buf, 1.0) );
    }
  }  

  if (XP > 0 && type != EXP_RESURRECT)
  {
    if( pvp )
    {
      XP = MIN(XP, (new_exp_table[GET_LEVEL(ch) + 1] / 4));
    }
    else
    {
      XP = MIN(XP, (new_exp_table[GET_LEVEL(ch) + 1] /
                    (1 + (GET_LEVEL(ch) / 3))));
    }
    
    if (RACE_GOOD(ch))
      XP = (int) (XP * (float) get_property("exp.factor.racewar.good", 1.0));
    else if (RACE_EVIL(ch))
      XP = (int) (XP * (float) get_property("exp.factor.racewar.evil", 0.7));
    else if (RACE_PUNDEAD(ch))
      XP = (int) (XP * (float) get_property("exp.factor.racewar.undead", 0.7));          
  }

  range = (int) (new_exp_table[GET_LEVEL(ch) + 1] / 3);
  XP = BOUNDED(-range, (int)(XP), range);

  if( pvp )
  {
    debug("pvp exp gain: %d", XP);
    logit(LOG_DEBUG, "pvp exp gain: %d", XP);    
  }
  
  if (XP < 0 && type != EXP_DEATH)
    return 0;

  if( !pvp )
  {
    XP = modify_exp_by_zone_trophy(ch, type, (int)(XP));    
  }

  XP = check_nexus_bonus(ch, (int)(XP), NEXUS_BONUS_EXP);

  // increase exp only to some limit (comulative exp till 61)
  if (XP < 0 || GET_EXP(ch) < global_exp_limit)
  {
    GET_EXP(ch) += (int)(XP);
  }

  display_gain(ch, (int)(XP));

  if (GET_LEVEL(ch) > MAXLVLMORTAL)
    return 0;

  if (XP > 0)
  {
    for (i = GET_LEVEL(ch) + 1;
         (i <= get_property("exp.maxExpLevel", 45)) && 
         (new_exp_table[i] <= GET_EXP(ch)); i++)
    {
      GET_EXP(ch) -= new_exp_table[i];
      advance_level(ch);
    }
  }
  else
  {
    while (GET_EXP(ch) < 0)
    {
      GET_EXP(ch) += new_exp_table[GET_LEVEL(ch)];
      lose_level(ch);
    }
  }

  return (int)(XP);
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
        SET_BIT(i->specials.act, PLR_AFK);

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
      strcat(Gbuf1, GET_NAME(i));
      strcat(Gbuf1, ", ");
      writeCharacter(i, 5, i->in_room);
      extract_char(i);
      if (i->desc)
        close_socket(i->desc);
      else
        free_char(i);

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
