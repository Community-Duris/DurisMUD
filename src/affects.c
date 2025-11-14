/*
 * ***************************************************************************
 *  file: affects.c                                          part of Duris
 *  usage: various routines for moving about objects/players.
 *  copyright  1990, 1991 - see 'license.doc' for complete information.
 *  copyright  1994, 1995 - sojourn systems ltd.
 *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "comm.h"
#include "account.h"
#include "db.h"
#include "events.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "arena.h"
#include "justice.h"
#include "mm.h"
#include "weather.h"
#include "interp.h"
#include "objmisc.h"
#include "paladins.h"
#include "guard.h"
#include "racewar_stat_mods.h"
#include "paladins.h"
#include "reavers.h"
#include "ships.h"
#include "epic_skills.h"
#include "disguise.h"
#include "sql.h"
#include "vnum.obj.h"
#include "ctf.h"
#include "files.h"

/*
 * external variables
 */

extern P_char character_list;
extern P_char combat_list;
extern P_char dead_guys;
extern P_desc descriptor_list;
extern P_event current_event;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern const struct race_names race_names_table[];
extern const struct class_names class_names_table[];
extern char *coin_names[];
extern char *coin_abbrev[];
extern const char *dirs[];
extern const struct stat_data stat_factor[];
extern const int rev_dir[];
extern int pulse;
extern struct con_app_type con_app[];
extern struct dex_app_type dex_app[];
extern struct max_stat max_stats[];
extern const struct racial_data_type racial_data[];
extern struct zone_data *zone_table;
extern struct time_info_data time_info;
extern struct arena_data arena;
extern P_event event_list;
static char buf[MAX_INPUT_LENGTH];
extern Skill skills[];
extern bool innate_two_daggers(P_char);
extern const char *event_names[];
int get_honing(P_obj);
extern float spell_pulse_data[LAST_RACE + 1];

struct mm_ds *dead_affect_pool = NULL;
struct mm_ds *dead_room_affect_pool = NULL;
float combat_by_class[CLASS_COUNT + 1][2];
float combat_by_race[LAST_RACE + 1][3];
float class_hitpoints[CLASS_COUNT + 1];
struct mm_ds *dead_link_pool = NULL;
struct mm_ds *dead_obj_link_pool = NULL;
struct mm_ds *dead_obj_affect_pool = NULL;

struct event_room_affect_data
{
  struct room_affect *af;
  int room;
};

extern void flanking_broken(struct char_link_data *);
extern void circle_broken(struct char_link_data *);
extern void song_broken(struct char_link_data *);
extern void essence_broken(struct char_link_data *);
extern void event_broken(struct char_link_data *);
extern void charm_broken(struct char_link_data *);
extern void casting_broken(struct char_link_data *);
extern void tether_broken(struct char_link_data *);

extern void cegilunes_broken(struct char_obj_link_data *);
extern void ileshs_broken(struct char_obj_link_data *);

extern char *get_function_name(void *func);
void unlink_char_affect(P_char, struct affected_type *);
void unlink_char_obj_affect(P_char, struct affected_type *);
struct link_description link_types[LNK_MAX+1];

int damroll_cap;
int hitroll_cap;
float pulse_all;
float shield_combat_mult;
float shield_combat_tank_mult;

/*
 * complete rewriting of the affect handler routines.  Reason?  Several
 * smallish problems with how affects are applied.  New method:  All
 * changes are made to dummy variables, then the summation of all changes
 * is applied to the actual character.  Thus if 10 items/spells change
 * strength up/down, only the net effect gets applied.  This also allows
 * the new STAT_MAX and RACIAL_STAT affects to work properly.  JAB
 */

struct hold_data TmpAffs;

//=================================================================================
//=== AFFECTS - MODIFY/APPLY
//=================================================================================

int apply_ac(P_char ch, int eq_pos)
{
  int value;

  if (!(ch && (eq_pos >= 0) && (eq_pos < MAX_WEAR) && ch->equipment[eq_pos]))
  {
    logit(LOG_EXIT, "assert: apply_ac called with bad args: ch: %s, eq_pos: %d, ch->eq[eq_pos]: %s.",
      ch ? J_NAME(ch) : "NULL", eq_pos, ch ? (ch->equipment[eq_pos] ? ch->equipment[eq_pos]->short_description : "No Eq in Slot" ): "NULL" );
    raise(SIGSEGV);
  }

  if (!GET_ITEM_TYPE(ch->equipment[eq_pos]) == ITEM_ARMOR &&
      !GET_ITEM_TYPE(ch->equipment[eq_pos]) == ITEM_SHIELD)
  {
    return 0;
  }

  switch (ch->equipment[eq_pos]->material)
  {
    case MAT_UNDEFINED:
    case MAT_NONSUBSTANTIAL:
      value = 0;
      break;
    case MAT_FLESH:
    case MAT_REEDS:
    case MAT_HEMP:
    case MAT_LIQUID:
    case MAT_CLOTH:
    case MAT_PAPER:
    case MAT_PARCHMENT:
    case MAT_LEAVES:
    case MAT_GENERICFOOD:
    case MAT_RUBBER:
    case MAT_FEATHER:
    case MAT_WAX:
      value = 1;
      break;
    case MAT_BARK:
    case MAT_SOFTWOOD:
    case MAT_SILICON:
    case MAT_CERAMIC:
    case MAT_PEARL:
    case MAT_EGGSHELL:
      value = 2;
      break;
    case MAT_HIDE:
    case MAT_LEATHER:
    case MAT_CURED_LEATHER:
    case MAT_LIMESTONE:
      value = 3;
      break;
    case MAT_IVORY:
    case MAT_BAMBOO:
    case MAT_HARDWOOD:
    case MAT_COPPER:
    case MAT_BONE:
    case MAT_MARBLE:
      value = 4;
      break;
    case MAT_STONE:
    case MAT_SILVER:
    case MAT_BRONZE:
    case MAT_IRON:
    case MAT_REPTILESCALE:
      value = 5;
      break;
    case MAT_GOLD:
    case MAT_CHITINOUS:
    case MAT_CRYSTAL:
    case MAT_STEEL:
    case MAT_BRASS:
    case MAT_OBSIDIAN:
    case MAT_GRANITE:
    case MAT_GEM:
      value = 6;
      break;
    case MAT_ELECTRUM:
    case MAT_PLATINUM:
    case MAT_RUBY:
    case MAT_EMERALD:
    case MAT_SAPPHIRE:
    case MAT_GLASSTEEL:
      value = 7;
      break;
    case MAT_DRAGONSCALE:
    case MAT_DIAMOND:
      value = 8;
      break;
    case MAT_MITHRIL:
    case MAT_ADAMANTIUM:
      value = 9;
      break;
    default:
      value = 0;
  }

  switch( eq_pos )
  {
    case WEAR_SHIELD:
      value *= 15;
      break;
    case WEAR_BODY:
      if (IS_SET(ch->equipment[eq_pos]->extra_flags, ITEM_WHOLE_BODY))
        value *= 8;
      else
        value *= 4;
      break;
    case WEAR_HORSE_BODY:
      value *= 2;
      break;
    case WEAR_HEAD:
      if (IS_SET(ch->equipment[eq_pos]->extra_flags, ITEM_WHOLE_HEAD))
        value *= 3;
      else
        value = (int) (value * 1.5);
      break;
    case WEAR_LEGS:
    case WEAR_ARMS:
    case WEAR_ARMS_2:
      value = (int) (value * 1.2);
      break;
    case WEAR_FEET:
    case WEAR_HANDS:
    case WEAR_HANDS_2:
    case WEAR_ABOUT:
      break;
    case WEAR_WAIST:
    case WEAR_WRIST_LL:
    case WEAR_WRIST_LR:
    case WEAR_WRIST_L:
    case WEAR_WRIST_R:
    case WEAR_NECK_1:
    case WEAR_NECK_2:
    case WEAR_FACE:
      value = (int) (value * 0.7);
      break;
    case WEAR_EYES:
    case WEAR_HORN:
      value = (int) (value * 0.4);
      break;
    default:
      break;;
  }
  // If values in zone files are better than values calculated..
  if( GET_ITEM_TYPE(ch->equipment[eq_pos]) == ITEM_SHIELD && eq_pos == WEAR_SHIELD )
  {
    value = MAX(value, ch->equipment[eq_pos]->value[3]);
    if( GET_CHAR_SKILL(ch, SKILL_SHIELD_COMBAT) )
    {
      // We add a percentage of the skill * value * the multiplier.
      // So, at 100 skill, we add value * multiplier (currently .5) -> value * 1.5.
      value += (int) ( (float)value * (float)GET_CHAR_SKILL(ch, SKILL_SHIELD_COMBAT) * shield_combat_mult ) / 100;
      // Only Swashbucklers and Guardians get tank bonus (and Pal/AP/Mercs).  Why Mercs?  Dunno...
      if( GET_CLASS(ch, CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_MERCENARY)
        || GET_SPEC(ch, CLASS_WARRIOR, SPEC_GUARDIAN ) || GET_SPEC(ch, CLASS_WARRIOR, SPEC_SWASHBUCKLER ) )
      {
      	value *= shield_combat_tank_mult;
      }
      /* Commented this out, it should be somewhere in calculate_ac or damage or such in fight.c.
       *   Somewhere where ac is used, not when shield is equipped.  It doesn't matter atm, since
       *   it's an epic skill right now.
      notch_skill(ch, SKILL_SHIELD_COMBAT, 33.33);
       */
      }
  }
  else if( GET_ITEM_TYPE(ch->equipment[eq_pos]) == ITEM_ARMOR )
  {
    value = MAX(value, ch->equipment[eq_pos]->value[0]);
  }

  return BOUNDED( -500, (value * MIN(100, ch->equipment[eq_pos]->condition)) / 100, 500);
}

int calculate_mana(P_char ch)
{
  int mana;

  mana = (int) (((float) GET_LEVEL(ch)) / 50 *
                GET_C_POW(ch) * GET_C_INT(ch) *
                get_property("mana.powMultiplier", 0.025));

  if (IS_PC(ch) && (GET_AGE(ch) <= racial_data[GET_RACE(ch)].max_age))
  {
    mana += (graf(ch, age(ch).year, 2, 4, 6, 8, 10, 12, 14));
  }

  return mana;
}

// This is now old code.  The current one in use is calculate_hitpoints2(ch).
int calculate_hitpoints(P_char ch)
{
  char buf[128];
  int hps, i, lvl, old_bonus, hitpoint_bonus, j, mod, toughness, racial_con;
  P_obj obj;
  bool apply_maxconbonus_hitpoints = FALSE;

  lvl = GET_LEVEL(ch);

  // Apply racial con modifiers.
  racial_con = stat_factor[GET_RACE(ch)].Con;
  for( i = 0; i < MAX_WEAR; i++ )
  {
    if( obj = ch->equipment[i] )
    {
      for( j = 0; j < MAX_OBJ_AFFECT; j++ )
      {
        if( obj->affected[j].location == APPLY_CON_RACE )
        {
          racial_con = MAX( racial_con, stat_factor[obj->affected[j].modifier].Con );
        }
      }
    }
  }

  i = MAX(-390, 390 - MIN(GET_C_CON(ch), racial_con));
  old_bonus = (int) (lvl * ((152100.0 - i * i) / 10864.285 - 4.0));
  hps = old_bonus;
  hps += (int) (((float) lvl) / 50 * racial_con * racial_con * get_property("hitpoints.conMultiplier", 0.035));

  if( IS_MULTICLASS_PC(ch) )
  {
    hps = (int) (hps * MAX(class_hitpoints[flag2idx(ch->player.m_class)],
              class_hitpoints[flag2idx(ch->player.secondary_class)]));
  }
  else
  {
    hps = (int) (hps * class_hitpoints[flag2idx(ch->player.m_class)]);
  }

  if( GET_AGE(ch) <= racial_data[GET_RACE(ch)].max_age )
  {
    hps += graf(ch, age(ch).year, 2, 4, 17, 14, 8, 4, 3);
  }

  // Small bonus for the first 10 levels heh.
  hps += MAX(0, 10 - lvl);

  // Should be made simpler some time, we add old con_bonus part from maxcon outside class multiplier
  i = MAX(-390, 390 - GET_C_CON(ch));
  hps += (int) (lvl * ((152100.0 - i * i) / 10864.285 - 4.0)) - old_bonus;

  if( IS_HARDCORE(ch) )
  {
    hps += (2 * lvl);
  }

  /* This calculates the HP bonus from the toughness
   * epic skill. Remove or comment it out if for some
   * reason in the future this skill ceases to exist.
   * -Zion 10/31/07 (happy halloween!)
   */
/*
  if(IS_AFFECTED3(ch, AFF3_PALADIN_AURA) && (GET_RACEWAR(ch) == 1))
   {
    if(ch->group)
	{
    	 if(ch->in_room == ch->group->ch->in_room)
    	 hps = (hps + BOUNDED(1, (GET_LEVEL(ch) * 2), 110));
	}
   }
*/

  if( IS_ILLITHID(ch) )
  {
    // 10 hps at level 1, 1 points in max con = 1 hp, gains 1 hp per level.
    hps = GET_LEVEL(ch) + 10 + MAX(GET_C_CON(ch), 100) - 100;
  }

  toughness = GET_CHAR_SKILL(ch, SKILL_TOUGHNESS);

  if( toughness > 0 && !GET_CLASS(ch, CLASS_MONK) )
  {
    hps += (int) (toughness * get_property("epic.skill.toughness", 0.500) * (GET_CLASS(ch, CLASS_WARRIOR | CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_MERCENARY) ? 2 : 1));
  }
  else
  {
    if( toughness < 50 )
    {
      hps += (int) (toughness * get_property("epic.skill.toughness.monk.low", 0.800));
    }
    else if( toughness >= 50 && toughness <= 90 )
    {
      hps += (int) (toughness * get_property("epic.skill.toughness.monk.medium", 1.000));
    }
    else
    {
      hps += (int) (toughness * get_property("epic.skill.toughness.monk.high", 1.250));
    }
  }
  if( hps < 0 )
  {
    logit(LOG_DEBUG, "%s has negative hitpoints bonus: %d (%d, %d)", GET_NAME(ch), hps, old_bonus, i);
    return 0;
  }
  // Casters get hitpoint bonus with con_max eq. Dec08 -Lucrot

  // Never liked this and it grew from making hitters far too powerful
  // downing this and finding a better solution - Jexni 2/6/11

  if( ch && GET_PRIME_CLASS(ch, MAX_CON_BONUS_CLASSES) && !IS_MULTICLASS_PC(ch) )
  {
    apply_maxconbonus_hitpoints = TRUE;
  }

  if(ch && IS_MULTICLASS_PC(ch)
    && GET_PRIME_CLASS(ch, CLASS_ETHERMANCER | CLASS_DRUID | CLASS_CLERIC | CLASS_SORCERER | CLASS_NECROMANCER | CLASS_SHAMAN | CLASS_PSIONICIST | CLASS_ILLUSIONIST | CLASS_CONJURER | CLASS_BARD | CLASS_SUMMONER | CLASS_BLIGHTER)
    && GET_SECONDARY_CLASS(ch, CLASS_ETHERMANCER | CLASS_DRUID | CLASS_CLERIC | CLASS_SORCERER | CLASS_NECROMANCER | CLASS_SHAMAN | CLASS_PSIONICIST | CLASS_ILLUSIONIST | CLASS_CONJURER | CLASS_BARD | CLASS_SUMMONER | CLASS_BLIGHTER))
  {
    apply_maxconbonus_hitpoints = TRUE;
  }

  if( ch && IS_PC(ch) && apply_maxconbonus_hitpoints )
  {
    hitpoint_bonus = 0;
    for (i = 0; i < MAX_WEAR; i++)
    {
      if(i == WEAR_ATTACH_BELT_1 || // Non max con bonuses for belted items.
        i == WEAR_ATTACH_BELT_2 ||
        i == WEAR_ATTACH_BELT_3)
      {
        continue;
      }
      if(ch->equipment[i])
      {
        obj = ch->equipment[i];

        for (j = 0; j < MAX_OBJ_AFFECT; j++)
        {
          if(obj->affected[j].location == APPLY_CON_MAX &&
            obj->affected[j].modifier > 0)
          {
            hitpoint_bonus += obj->affected[j].modifier;
          }
        }
      }
    }
    // Max con hitpoint bonus now uses a racial constitution ratio. May2010 -Lucrot
    if( IS_PC(ch) && hitpoint_bonus )
    {
      snprintf(buf, MAX_STRING_LENGTH, "stats.con.%s", race_names_table[GET_RACE(ch)].no_spaces);
      mod = (int) get_property(buf, 100.);
      hps += (int) (hitpoint_bonus * get_property("hitpoints.spellcaster.maxConBonus", 2.5) * mod / 100);
    }
  }

  return hps;
}

// This is a rewrite of calculate_hitpoints(ch).  It's a little faster for Illithid hps.
// It's a little faster overall I think.
// It also uses the racial bonus as figured with APPLY_CON_RACE eq instead of using ch's
//   normal race.  (As it should be imo).
int calculate_hitpoints2(P_char ch)
{
  char  buf[128];
  int   i, j;
  int   level, race, age_mod, curr_con, toughness, newbie, hardcore;
  float old_bonus, new_bonus, class_mod, racial_con, maxconbonus;
  P_obj obj;

  if( IS_ILLITHID(ch) )
  {
    // Could simplify as there are no Warrior/Pal/AP/Merc Illithids, but just in case.
    return GET_MAX_HIT(ch) + GET_LEVEL(ch) + 10 + MAX(GET_C_CON(ch), 100) - 100
      + GET_CHAR_SKILL(ch, SKILL_TOUGHNESS) * get_property("epic.skill.toughness", 0.500)
      * (GET_CLASS(ch, CLASS_WARRIOR | CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_MERCENARY) ? 2 : 1);
  }

  // Calculate level
  level = GET_LEVEL(ch);

  // Calculate racial con.
  race = GET_RACE(ch);
  racial_con = stat_factor[race].Con;
  for( i = 0; i < MAX_WEAR; i++ )
  {
    if( obj = ch->equipment[i] )
    {
      for( j = 0; j < MAX_OBJ_AFFECT; j++ )
      {
        if( obj->affected[j].location == APPLY_CON_RACE
          && stat_factor[obj->affected[j].modifier].Con > racial_con )
        {
          race = obj->affected[j].modifier;
          racial_con = stat_factor[race].Con;
        }
      }
    }
  }

  // Calculate curr con (max is 390 * 2 = 780, because things go awry above this in the calculations).
  curr_con = MIN(GET_C_CON(ch), 780);

  i = MIN(curr_con, racial_con);
  old_bonus = level * ((-14./152100.) * i * i + (28./390.) * i - 4.);
  // 250 hps at lvl 50 and 100 con.  1000 hps at lvl 50 and 200 con.
  new_bonus = (((float) level) / 50. * racial_con * racial_con * get_property("hitpoints.conMultiplier", 0.025));

  // Calculate class mod
  class_mod = class_hitpoints[flag2idx(ch->player.m_class)];
  if( IS_MULTICLASS_PC(ch) )
  {
    class_mod = MAX( class_mod, class_hitpoints[flag2idx(ch->player.secondary_class)]);
  }
  old_bonus *= class_mod - 1.0;
  new_bonus *= class_mod;

  i = curr_con;
  new_bonus += level * ((-14./152100.) * i * i + (28./390.) * i - 4.);

  // Calculate age mod
  if( GET_AGE(ch) <= racial_data[GET_RACE(ch)].max_age )
  {
    age_mod = graf(ch, age(ch).year, 2, 4, 17, 14, 8, 4, 3);
  }

  // Calculate maxcon bonus (rewrote this a little to make it easier to read).
  maxconbonus = 0;
  if( GET_PRIME_CLASS(ch, MAX_CON_BONUS_CLASSES) && (!IS_MULTICLASS_PC(ch) || GET_SECONDARY_CLASS(ch, MAX_CON_BONUS_CLASSES)) )
  {
    for( i = 0; i < MAX_WEAR; i++ )
    {
      // We skip on back (bps don't give stats), and 2nd and 3rd items on belt (1st == belt buckle == ok)
      if( i == WEAR_BACK || i == WEAR_ATTACH_BELT_2 || i == WEAR_ATTACH_BELT_3 )
      {
        continue;
      }
      if( obj = ch->equipment[i] )
      {
        for( j = 0; j < MAX_OBJ_AFFECT; j++ )
        {
          if( obj->affected[j].location == APPLY_CON_MAX )
          {
            maxconbonus += obj->affected[j].modifier;
          }
        }
      }
    }
    if( maxconbonus )
    {
      maxconbonus *= racial_con / 100.;
      maxconbonus *= get_property("hitpoints.spellcaster.maxConBonus", 2.5);
    }
  }

  // Calculate toughness bonus
  if( (toughness = GET_CHAR_SKILL(ch, SKILL_TOUGHNESS)) > 0 && !GET_CLASS(ch, CLASS_MONK) )
  {
    toughness = (float)toughness * get_property("epic.skill.toughness", 0.500) * (GET_CLASS(ch, CLASS_WARRIOR | CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_MERCENARY) ? 2. : 1.);
  }
  else
  {
    if( toughness < 50 )
    {
      toughness = ((float)toughness * get_property("epic.skill.toughness.monk.low", 0.800));
    }
    else if( toughness >= 50 && toughness <= 90 )
    {
      toughness = ((float)toughness * get_property("epic.skill.toughness.monk.medium", 1.000));
    }
    else
    {
      toughness = ((float)toughness * get_property("epic.skill.toughness.monk.high", 1.250));
    }
  }

  // Small bonus for the first 9 levels heh.
  newbie = MAX(0, 10 - level);

  // Calculate hardcore bonus.
  hardcore = IS_HARDCORE(ch) ? (2 * level) : 0;

/* If you want to redo hps in some way, this is a good way to test what's changing and how much.
 * The old_bnus and new_bonus are just the base hps relating to current and racial con.
 * class_mod is the modifier for class, age_mod for age, maxconbonus for the casters, tough for toughness skill, newb for lowbies and HC for hardcore chars.
  if( IS_PC(ch) ) debug( "old = %.2f, new = %.2f, CM = %.2f, age = %d, maxcon = %.2f, tough = %d, newb = %d, HC = %d.",
    old_bonus, new_bonus, class_mod, age_mod, maxconbonus, toughness, newbie, hardcore );
 */

  // we use base_hit for the racial mod, and the difference is the hps gear.
  return (ch->points.base_hit * racial_con) / 100 + GET_MAX_HIT(ch) - ch->points.base_hit
    + old_bonus + new_bonus + age_mod + maxconbonus + toughness + newbie + hardcore;
}

void event_balance_affects(P_char ch, P_char victim, P_obj obj, void *data)
{
  affect_total(ch, TRUE);
}

void balance_affects(P_char ch)
{
  if( !IS_ALIVE(ch) || get_scheduled(ch, event_balance_affects) )
    return;

  add_event(event_balance_affects, 0, ch, 0, 0, 0, 0, 0);
}

void add_racial_stat_bonus(P_char ch, struct hold_data *affs)
{
  char buf[256];
  int  bonus;

  if( !affs || !ch )
  {
    raise(SIGSEGV);
  }

  snprintf(buf, 256, "stats.bonus.%s", race_names_table[ch->player.race].no_spaces);

  bonus = get_property(buf, 0);

  affs->c_Str += bonus;
  affs->m_Str += bonus;
  affs->c_Dex += bonus;
  affs->m_Dex += bonus;
  affs->c_Agi += bonus;
  affs->m_Agi += bonus;
  affs->c_Con += bonus;
  affs->m_Con += bonus;
  affs->c_Pow += bonus;
  affs->m_Pow += bonus;
  affs->c_Int += bonus;
  affs->m_Int += bonus;
  affs->c_Wis += bonus;
  affs->m_Wis += bonus;
  affs->c_Cha += bonus;
  affs->m_Cha += bonus;
  affs->c_Luc += bonus;
  affs->m_Luc += bonus;

}

/*
 * this routine actually applies the summarized affects to the character.
 * All sanity checking is done here. By breaking it out we can exercise
 * very discrete control over what is an isn't legal.   mode:  TRUE  -
 * apply TmpAffs. FALSE - reset the values.
 *
 * JAB
 */

void apply_affs(P_char ch, int mode)
{
  int t1, t2, t3, temp;
  float max_con_bonus;
  char  buf1[256];

  if(!ch)
  {
    return;
  }
  if (mode)
  {
    SET_BIT(ch->specials.affected_by, TmpAffs.BV_1);
    SET_BIT(ch->specials.affected_by2, TmpAffs.BV_2);
    SET_BIT(ch->specials.affected_by3, TmpAffs.BV_3);
    SET_BIT(ch->specials.affected_by4, TmpAffs.BV_4);
    SET_BIT(ch->specials.affected_by5, TmpAffs.BV_5);
    SET_BIT(ch->specials.affected_by, TmpAffs.Fprot);

    if (IS_AFFECTED3(ch, AFF3_COLDSHIELD))
    {
      REMOVE_BIT(ch->specials.affected_by2, AFF2_FIRESHIELD);
      REMOVE_BIT(ch->specials.affected_by3, AFF3_LIGHTNINGSHIELD);
    }
    else if (IS_AFFECTED3(ch, AFF3_LIGHTNINGSHIELD))
    {
      REMOVE_BIT(ch->specials.affected_by2, AFF2_FIRESHIELD);
      REMOVE_BIT(ch->specials.affected_by3, AFF3_COLDSHIELD);
    }
    if (IS_AFFECTED2(ch, AFF2_FIRESHIELD))
    {
      REMOVE_BIT(ch->specials.affected_by3, AFF3_COLDSHIELD);
      REMOVE_BIT(ch->specials.affected_by3, AFF3_LIGHTNINGSHIELD);
    }

    if (IS_AFFECTED4(ch, AFF4_NEG_SHIELD))
    {
      REMOVE_BIT(ch->specials.affected_by2, AFF2_SOULSHIELD);
    }
    if (IS_AFFECTED2(ch, AFF2_SOULSHIELD))
    {
      REMOVE_BIT(ch->specials.affected_by4, AFF4_NEG_SHIELD);
    }
/*  Elemental aura has it's aff which is fixed below.
    if( affected_by_spell(ch, SPELL_ELEMENTAL_AURA) )
    {
      REMOVE_BIT(ch->specials.affected_by2, AFF2_FIRE_AURA);
      REMOVE_BIT(ch->specials.affected_by2, AFF2_WATER_AURA);
      REMOVE_BIT(ch->specials.affected_by2, AFF2_EARTH_AURA);
      REMOVE_BIT(ch->specials.affected_by2, AFF2_AIR_AURA);
      REMOVE_BIT(ch->specials.affected_by4, AFF4_ICE_AURA);
    }
*/
    if (IS_AFFECTED2(ch, AFF2_FIRE_AURA))
    {
      REMOVE_BIT(ch->specials.affected_by2, AFF2_WATER_AURA);
      REMOVE_BIT(ch->specials.affected_by2, AFF2_EARTH_AURA);
      REMOVE_BIT(ch->specials.affected_by2, AFF2_AIR_AURA);
      REMOVE_BIT(ch->specials.affected_by4, AFF4_ICE_AURA);
    }
    if (IS_AFFECTED2(ch, AFF2_EARTH_AURA))
    {
      REMOVE_BIT(ch->specials.affected_by2, AFF2_WATER_AURA);
      REMOVE_BIT(ch->specials.affected_by2, AFF2_AIR_AURA);
      REMOVE_BIT(ch->specials.affected_by4, AFF4_ICE_AURA);
    }
    if (IS_AFFECTED2(ch, AFF2_AIR_AURA))
    {
      REMOVE_BIT(ch->specials.affected_by2, AFF2_WATER_AURA);
      REMOVE_BIT(ch->specials.affected_by4, AFF4_ICE_AURA);
    }
    if (IS_AFFECTED4(ch, AFF4_ICE_AURA))
    {
      REMOVE_BIT(ch->specials.affected_by2, AFF2_WATER_AURA);
    }
  }
  else
  {
    REMOVE_BIT(ch->specials.affected_by, TmpAffs.Fprot);
    REMOVE_BIT(ch->specials.affected_by, TmpAffs.BV_1);
    REMOVE_BIT(ch->specials.affected_by2, TmpAffs.BV_2);
    REMOVE_BIT(ch->specials.affected_by3, TmpAffs.BV_3);
    REMOVE_BIT(ch->specials.affected_by4, TmpAffs.BV_4);
    REMOVE_BIT(ch->specials.affected_by5, TmpAffs.BV_5);
  }

  /*
   * ok, some innate powers just set bits, so we need to reset those
   */

  if (has_innate(ch, INNATE_INFERNAL_FURY))
    SET_BIT(ch->specials.affected_by, AFF_INFERNAL_FURY);
  if (has_innate(ch, INNATE_SNEAK))
    SET_BIT(ch->specials.affected_by, AFF_SNEAK);
  if (has_innate(ch, INNATE_FARSEE))
    SET_BIT(ch->specials.affected_by, AFF_FARSEE);
  if (has_innate(ch, INNATE_PROT_LIGHTNING))
    SET_BIT(ch->specials.affected_by2, AFF2_PROT_LIGHTNING);
  if (has_innate(ch, INNATE_PROT_FIRE))
    SET_BIT(ch->specials.affected_by, AFF_PROT_FIRE);
  if (has_innate(ch, INNATE_WATERBREATH))
    SET_BIT(ch->specials.affected_by, AFF_WATERBREATH);
  if (has_innate(ch, INNATE_INFRAVISION))
    SET_BIT(ch->specials.affected_by, AFF_INFRAVISION);
  if (has_innate(ch, INNATE_FLY))
    SET_BIT(ch->specials.affected_by, AFF_FLY);
  if (has_innate(ch, INNATE_NATURAL_MOVEMENT))
    SET_BIT(ch->specials.affected_by3, AFF3_PASS_WITHOUT_TRACE);
  if (has_innate(ch, INNATE_HASTE))
    SET_BIT(ch->specials.affected_by, AFF_HASTE);
  if (has_innate(ch, INNATE_REGENERATION))
    SET_BIT(ch->specials.affected_by4, AFF4_REGENERATION);
  if (has_innate(ch, INNATE_BLOOD_SCENT))
    SET_BIT(ch->specials.affected_by5, AFF5_BLOOD_SCENT);
  if (has_innate(ch, INNATE_ULTRAVISION))
    SET_BIT(ch->specials.affected_by2, AFF2_ULTRAVISION);
  if (has_innate(ch, INNATE_ANTI_GOOD))
  {
    SET_BIT(ch->specials.affected_by, AFF_PROTECT_GOOD);
    SET_BIT(ch->specials.affected_by2, AFF2_DETECT_GOOD);
    SET_BIT(ch->specials.affected_by2, AFF2_DETECT_EVIL);
  }
  if (has_innate(ch, INNATE_PROT_FIRE))
    SET_BIT(ch->specials.affected_by, AFF_PROT_FIRE);
  if (has_innate(ch, INNATE_PROT_ACID))
    SET_BIT(ch->specials.affected_by2, AFF2_PROT_ACID);
  if (has_innate(ch, INNATE_PROT_COLD))
    SET_BIT(ch->specials.affected_by2, AFF2_PROT_COLD);
  if (has_innate(ch, INNATE_FIRE_AURA))
    SET_BIT(ch->specials.affected_by2, AFF2_FIRE_AURA);
  if (has_innate(ch, INNATE_ICE_AURA))
    SET_BIT(ch->specials.affected_by4, AFF4_ICE_AURA);
  if (has_innate(ch, INNATE_ANTI_EVIL))
  {
    SET_BIT(ch->specials.affected_by, AFF_PROTECT_EVIL);
    SET_BIT(ch->specials.affected_by2, AFF2_DETECT_EVIL);
    SET_BIT(ch->specials.affected_by2, AFF2_DETECT_GOOD);
  }
  if (has_innate(ch, INNATE_VAMPIRIC_TOUCH))
    SET_BIT(ch->specials.affected_by2, AFF2_VAMPIRIC_TOUCH);
  if (has_innate(ch, INNATE_HOLY_LIGHT))
    SET_BIT(ch->specials.affected_by4, AFF4_MAGE_FLAME);
  // There is no bonus to hitroll for innate_perception. Thus, going to allow
  // players to cast hawkvision. innate_perception has a bonus in track.c
  // if (has_innate(ch, INNATE_PERCEPTION))
    // SET_BIT(ch->specials.affected_by4, AFF4_HAWKVISION);
  if ((IS_FIGHTING(ch) || IS_DESTROYING(ch)) && IS_AFFECTED(ch, AFF_HIDE))
    REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
  if (GET_CHAR_SKILL(ch, SKILL_LISTEN))
    SET_BIT(ch->specials.affected_by5, AFF5_LISTEN);
  if (has_innate(ch, INNATE_DAUNTLESS))
    SET_BIT(ch->specials.affected_by4, AFF4_NOFEAR);
  if (has_innate(ch, INNATE_EYELESS))
    SET_BIT(ch->specials.affected_by5, AFF5_NOBLIND);
  if( has_innate(ch, INNATE_INVISIBILITY) )
    SET_BIT(ch->specials.affected_by, AFF_INVISIBLE);

  if( IS_AFFECTED5(ch, AFF5_THORNSKIN) )
  {
    REMOVE_BIT(ch->specials.affected_by, AFF_BARKSKIN);
  }

  /*
   * for stats, it just flat out recalcs them, no +/- about it, safer
   * that way
   */

  if( mode )
  {
    add_racewar_stat_mods(ch, &TmpAffs);
    // Only human PCs have racial bonuses atm.  This saves time.
    if( IS_PC(ch) && GET_RACE(ch) == RACE_HUMAN )
    {
      add_racial_stat_bonus(ch, &TmpAffs);
    }
  }

  t1 = (!mode || !TmpAffs.r_Str) ? (int) GET_RACE(ch) : TmpAffs.r_Str;
  t3 = (mode) ? (100 + TmpAffs.m_Str) : 100;
  t2 = BOUNDED(1, (ch->base_stats.Str + ((mode) ? TmpAffs.c_Str : 0)), t3);
  GET_C_STR(ch) = BOUNDED(1, (int) (stat_factor[t1].Str * t2 / 100. + .55), 511);

  t1 = (!mode || !TmpAffs.r_Dex) ? (int) GET_RACE(ch) : TmpAffs.r_Dex;
  t3 = (mode) ? (100 + TmpAffs.m_Dex) : 100;
  t2 = BOUNDED(1, (ch->base_stats.Dex + ((mode) ? TmpAffs.c_Dex : 0)), t3);
  GET_C_DEX(ch) = BOUNDED(1, (int) (stat_factor[t1].Dex * t2 / 100. + .55), 511);

  t1 = (!mode || !TmpAffs.r_Agi) ? (int) GET_RACE(ch) : TmpAffs.r_Agi;
  t3 = (mode) ? (100 + TmpAffs.m_Agi) : 100;
  t2 = BOUNDED(1, (ch->base_stats.Agi + ((mode) ? TmpAffs.c_Agi : 0)), t3);
  GET_C_AGI(ch) = BOUNDED(1, (int) (stat_factor[t1].Agi * t2 / 100. + .55), 511);

  // t1 = which race to apply racial con with.
  t1 = (!mode || !TmpAffs.r_Con) ? (int) GET_RACE(ch) : TmpAffs.r_Con;
  // t3 = the amount of maxcon to apply.
  t3 = (mode) ? (100 + TmpAffs.m_Con) : 100;
  // t2 = amount of base_con + reg con eq, bounded between 1 and t3 (100 + maxcon).
  //        This is the final con before racial mods.
  t2 = BOUNDED(1, (ch->base_stats.Con + ((mode) ? TmpAffs.c_Con : 0)), t3);
  // Current con = racial con * actual con / 100 + .55
  //                 Makes more sense to do (actual con) * (racial modifier/100) + .55 (?)
  GET_C_CON(ch) = BOUNDED(1, (int) (stat_factor[t1].Con * t2 / 100. + .55), 511);

  t1 = (!mode || !TmpAffs.r_Pow) ? (int) GET_RACE(ch) : TmpAffs.r_Pow;
  t3 = (mode) ? (100 + TmpAffs.m_Pow) : 100;
  t2 = BOUNDED(1, (ch->base_stats.Pow + ((mode) ? TmpAffs.c_Pow : 0)), t3);
  GET_C_POW(ch) = BOUNDED(1, (int) (stat_factor[t1].Pow * t2 / 100. + .55), 511);

  t1 = (!mode || !TmpAffs.r_Int) ? (int) GET_RACE(ch) : TmpAffs.r_Int;
  t3 = (mode) ? (100 + TmpAffs.m_Int) : 100;
  t2 = BOUNDED(1, (ch->base_stats.Int + ((mode) ? TmpAffs.c_Int : 0)), t3);
  GET_C_INT(ch) = BOUNDED(1, (int) (stat_factor[t1].Int * t2 / 100. + .55), 511);

  t1 = (!mode || !TmpAffs.r_Wis) ? (int) GET_RACE(ch) : TmpAffs.r_Wis;
  t3 = (mode) ? (100 + TmpAffs.m_Wis) : 100;
  t2 = BOUNDED(1, (ch->base_stats.Wis + ((mode) ? TmpAffs.c_Wis : 0)), t3);
  GET_C_WIS(ch) = BOUNDED(1, (int) (stat_factor[t1].Wis * t2 / 100. + .55), 511);

  t1 = (!mode || !TmpAffs.r_Cha) ? (int) GET_RACE(ch) : TmpAffs.r_Cha;
  t3 = (mode) ? (100 + TmpAffs.m_Cha) : 100;
  t2 = BOUNDED(1, (ch->base_stats.Cha + ((mode) ? TmpAffs.c_Cha : 0)), t3);
  GET_C_CHA(ch) = BOUNDED(1, (int) (stat_factor[t1].Cha * t2 / 100. + .55), 511);

  t1 = (!mode || !TmpAffs.r_Kar) ? (int) GET_RACE(ch) : TmpAffs.r_Kar;
  t3 = (mode) ? (100 + TmpAffs.m_Kar) : 100;
  t2 = BOUNDED(1, (ch->base_stats.Kar + ((mode) ? TmpAffs.c_Kar : 0)), t3);
  GET_C_KAR(ch) = BOUNDED(1, (int) (stat_factor[t1].Kar * t2 / 100. + .55), 511);

  t1 = (!mode || !TmpAffs.r_Luc) ? (int) GET_RACE(ch) : TmpAffs.r_Luc;
  t3 = (mode) ? (100 + TmpAffs.m_Luc) : 100;
  t2 = BOUNDED(1, (ch->base_stats.Luk + ((mode) ? TmpAffs.c_Luc : 0)), t3);
  GET_C_LUK(ch) = BOUNDED(1, (int) (stat_factor[t1].Luk * t2 / 100. + .55), 511);

  /*
   * note that the current stats now show the ACTUAL stat, including
   * racial factors, this is much better than the old method, fewer
   * recalcs.  Only practical difference, the stat section of
   * do_attributes() needs to change.  JAB
   */

  ch->specials.apply_saving_throw[SAVING_PARA] =
    (mode) ? BOUNDED(-128, TmpAffs.S_para, 127) : 0;
  ch->specials.apply_saving_throw[SAVING_ROD] =
    (mode) ? BOUNDED(-128, TmpAffs.S_rod, 127) : 0;
  ch->specials.apply_saving_throw[SAVING_FEAR] =
    (mode) ? BOUNDED(-128, TmpAffs.S_petri, 127) : 0;
  ch->specials.apply_saving_throw[SAVING_SPELL] =
    (mode) ? BOUNDED(-128, TmpAffs.S_spell, 127) : 0;
  ch->specials.apply_saving_throw[SAVING_BREATH] =
    (mode) ? BOUNDED(-128, TmpAffs.S_breath, 127) : 0;

  GET_AC(ch) = ch->points.base_armor + ((mode) ? TmpAffs.AC : 0);

  if (GET_C_AGI(ch) > stat_factor[(int) GET_RACE(ch)].Agi)
  {
    GET_AC(ch) -= (int) (GET_C_AGI(ch) - stat_factor[(int) GET_RACE(ch)].Agi);
  }

  temp = ch->points.base_damroll + ((mode) ? TmpAffs.Dam : 0);
  // Can go over for Immortals and mobs (which is an undetected overflow error).
  ch->points.damroll = (temp > 255) ? 255 : temp;

  if( IS_PC(ch) && ch->points.damroll > (damroll_cap * combat_by_race[GET_RACE(ch)][2]) )
  {
//if( IS_PC(ch) ) debug( "damroll: %d, damroll_cap: %d, racial modifier: %.3f, new damroll: %d", ch->points.damroll,
//  damroll_cap, combat_by_race[GET_RACE(ch)][2], (int)(damroll_cap * combat_by_race[GET_RACE(ch)][2]) );
    ch->points.damroll = (int)(damroll_cap * combat_by_race[GET_RACE(ch)][2]);
  }

/* This shit right here has got to go..  Too complex.
  if( GET_C_STR(ch) > stat_factor[(int) GET_RACE(ch)].Str )
  {
    if( GET_C_STR(ch) - stat_factor[(int) GET_RACE(ch)].Str < 9 )
    {
      ch->points.damroll += (int) (2 * sqrt((GET_C_STR(ch) - stat_factor[(int) GET_RACE(ch)].Str)));
    }
    else
    {
      int diff = GET_C_STR(ch) - stat_factor[(int) GET_RACE(ch)].Str;
      diff = (int) sqrt(sqrt(diff * diff * diff));
      // 127 is the greatest value a byte can be in this system.
      if( diff + ch->points.damroll > 127 )
      {
        ch->points.damroll = 127;
      }
      else
      {
        ch->points.damroll += diff;
      }
    }
  }
*/

  if( mode )
  {
    temp = ch->points.base_hitroll + TmpAffs.Hit;
    ch->points.hitroll = (temp > 127) ? 127 : temp;
  }

  if( mode )
  {
    if( IS_AFFECTED4(ch, AFF4_HAWKVISION) )
    {
      ch->points.hitroll = (ch->points.hitroll > 121) ? 127 : ch->points.hitroll + 5;
    }
  }

  if( IS_PC(ch) && ch->points.hitroll > hitroll_cap )
  {
    ch->points.hitroll = hitroll_cap;
  }

  if (GET_C_DEX(ch) > stat_factor[(int) GET_RACE(ch)].Dex)
  {
    temp = ch->points.hitroll + (int) (GET_C_DEX(ch) - stat_factor[(int) GET_RACE(ch)].Dex) / 2;
    ch->points.hitroll = (temp > 127) ? 127 : temp;
  }

  if( ( GET_CLASS(ch, CLASS_PALADIN) || GET_CLASS(ch, CLASS_ANTIPALADIN) ) && is_wielding_paladin_sword(ch) )
  {
    temp = ch->points.hitroll + GET_LEVEL(ch) / 6;
    ch->points.hitroll = (temp > 127) ? 127 : temp;
    temp = ch->points.damroll + GET_LEVEL(ch) / 6;
    ch->points.damroll = (temp > 255) ? 255 : temp;
  }

  if( has_innate(ch, INNATE_DUAL_WIELDING_MASTER) && ch->equipment[PRIMARY_WEAPON]
    && ch->equipment[SECONDARY_WEAPON] )
  {
    temp = ch->points.hitroll + GET_LEVEL(ch) / 5;
    ch->points.hitroll = (temp > 127) ? 127 : temp;
  }

  if( has_innate(ch, INNATE_HAMMER_MASTER) && ch->equipment[PRIMARY_WEAPON]
    && ch->equipment[PRIMARY_WEAPON]->value[0] == WEAPON_HAMMER )
  {
    temp = ch->points.hitroll + GET_LEVEL(ch) / 8;
    ch->points.hitroll = (temp > 127) ? 127 : temp;
    temp = ch->points.damroll + GET_LEVEL(ch) / 10;
    ch->points.damroll = (temp > 255) ? 255 : temp;
  }

  if( has_innate(ch, INNATE_AXE_MASTER) && (ch->equipment[PRIMARY_WEAPON]
    && ch->equipment[PRIMARY_WEAPON]->value[0] == WEAPON_AXE) )
  {
    temp = ch->points.hitroll + GET_LEVEL(ch) / 8;
    ch->points.hitroll = (temp > 127) ? 127 : temp;
    temp = ch->points.damroll + GET_LEVEL(ch) / 12;
    ch->points.damroll = (temp > 255) ? 255 : temp;
  }

  if( has_innate(ch, INNATE_LONGSWORD_MASTER) && ( ch->equipment[PRIMARY_WEAPON]
    && ch->equipment[PRIMARY_WEAPON]->value[0] == WEAPON_LONGSWORD))
  {
    temp = ch->points.hitroll + GET_LEVEL(ch) / 8;
    ch->points.hitroll = (temp > 127) ? 127 : temp;
    temp = ch->points.damroll + GET_LEVEL(ch) / 12;
    ch->points.damroll = (temp > 255) ? 255 : temp;
  }

  if( has_innate(ch, INNATE_GAMBLERS_LUCK) )
  {
    GET_C_LUK(ch) = GET_C_LUK(ch) + 10;
  }

  // sure best if we store max_hp in pfile :(
  // now lets use diff approach - store in pfile not HP, but difference max-curr HP
  // can you believe? it works! (Lom)
  if( !mode && (GET_MAX_HIT(ch) == 0) )
     t1 = GET_HIT(ch); // diff is stored in pfile
  else
     t1 = GET_MAX_HIT(ch) - GET_HIT(ch);

  GET_MAX_HIT(ch) = ch->points.base_hit + ((mode) ? TmpAffs.Hits : 0);
  GET_HIT(ch) = GET_MAX_HIT(ch) - t1;

  t1 = GET_MAX_VITALITY(ch) - GET_VITALITY(ch);
  GET_MAX_VITALITY(ch) = vitality_limit(ch) + ((mode) ? TmpAffs.Move : 0);
  GET_VITALITY(ch) = GET_MAX_VITALITY(ch) - t1;

  t1 = GET_MAX_MANA(ch) - GET_MANA(ch);
  GET_MAX_MANA(ch) = ch->points.base_mana + ((mode) ? TmpAffs.Mana : 0);
  GET_MANA(ch) = GET_MAX_MANA(ch) - t1;

  ch->player.time.age_mod = (mode) ? TmpAffs.Age : 0;

  ch->points.hit_reg = TmpAffs.hit_reg;
  ch->points.move_reg = TmpAffs.move_reg;
  ch->points.mana_reg = TmpAffs.mana_reg;

/* Original:
  // Using value defined in SPELL_PULSE macro.
  switch((int)TmpAffs.spell_pulse)
  {
    case 5:
    case 4:
      TmpAffs.spell_pulse = 3;
      break;
    case 3:
    case 2:
      TmpAffs.spell_pulse = 2;
      break;
    case 1:
      TmpAffs.spell_pulse = 1;
      break;
    case 0:
      TmpAffs.spell_pulse = 0;
      break;
    case -1:
      TmpAffs.spell_pulse = -1;
      break;
    case -2:
    case -3:
      TmpAffs.spell_pulse = -2;
      break;
    case -4:
    case -5:
      TmpAffs.spell_pulse = -3;
      break;
    case -6:
    case -7:
    case -8:
      TmpAffs.spell_pulse = -4;
      break;
    default:
      debug( "Char '%s' has spell pulse out of range (5 - -8) %d.", J_NAME(ch), TmpAffs.spell_pulse);
      TmpAffs.spell_pulse = 0;
      break;
  }
*/
  ch->points.spell_pulse = TmpAffs.spell_pulse;

/* Original:
  switch((int)TmpAffs.combat_pulse)
  {
    case 5:
    case 4:
      TmpAffs.combat_pulse = 3;
      break;
    case 3:
    case 2:
      TmpAffs.combat_pulse = 2;
      break;
    case 1:
      TmpAffs.combat_pulse = 1;
      break;
    case 0:
      TmpAffs.combat_pulse = 0;
      break;
    case -1:
      TmpAffs.combat_pulse = -1;
      break;
    case -2:
    case -3:
      TmpAffs.combat_pulse = -2;
      break;
    case -4:
    case -5:
      TmpAffs.combat_pulse = -3;
      break;
    case -6:
    case -7:
      TmpAffs.combat_pulse = -4;
      break;
    case -8:
    case -9:
    case -10:
    case -11:
    case -12:
      TmpAffs.combat_pulse = -5;
      break;
    default:
      TmpAffs.combat_pulse = 0;
      break;
  }
*/
  ch->points.combat_pulse = TmpAffs.combat_pulse;

  if (mode)
  {
    if (IS_AFFECTED(ch, AFF_ARMOR))
      GET_AC(ch) -= 30;
    if (IS_AFFECTED(ch, AFF_BARKSKIN))
      GET_AC(ch) -= 100;
    if( IS_AFFECTED5(ch, AFF5_THORNSKIN) )
      GET_AC(ch) -= 50;
    if (IS_AFFECTED3(ch, AFF3_GR_SPIRIT_WARD))
      ch->specials.apply_saving_throw[SAVING_SPELL] -= 4;
    if (IS_AFFECTED3(ch, AFF3_SPIRIT_WARD))
      ch->specials.apply_saving_throw[SAVING_SPELL] -= 2;
  }
  two_weapon_check(ch);

  //if (mode) {
  //  if ( (IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE) ||
  //        affected_by_spell(ch, SPELL_PLAGUE) ) &&
  //       !get_scheduled(ch, event_plague) )
  //  {
  //    add_event(event_plague, WAIT_SEC, ch, 0, 0, 0, 0, 0);
  //  }
  //}
}

/*
 * basically just a huge switch() to alter lots of variables in hold_data,
 * called only from all_affects(). JAB
 */

void affect_modify(int loc, int mod, unsigned long *bitv, int from_eq)
{
  if (bitv)
  {
    if (!from_eq)
      SET_BIT(TmpAffs.BV_1, bitv[0]);
    else
      SET_BIT(TmpAffs.BV_1, bitv[0] & ~(AFF_STONE_SKIN | AFF_BIOFEEDBACK | AFF_HIDE));

    SET_BIT(TmpAffs.BV_2, bitv[1]);
    SET_BIT(TmpAffs.BV_3, bitv[2]);
    SET_BIT(TmpAffs.BV_4, bitv[3]);
    SET_BIT(TmpAffs.BV_5, bitv[4]);
    SET_BIT(TmpAffs.BV_6, bitv[5]);
  }
  switch (loc)
  {

  case APPLY_NONE:
    break;

  case APPLY_AGI_MAX:
    TmpAffs.m_Agi += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_AGI:
    TmpAffs.c_Agi += mod;
    break;

  case APPLY_CHA_MAX:
    TmpAffs.m_Cha += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_CHA:
    TmpAffs.c_Cha += mod;
    break;

  case APPLY_CON_MAX:
    TmpAffs.m_Con += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_CON:
    TmpAffs.c_Con += mod;
    break;

  case APPLY_DEX_MAX:
    TmpAffs.m_Dex += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_DEX:
    TmpAffs.c_Dex += mod;
    break;

  case APPLY_INT_MAX:
    TmpAffs.m_Int += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_INT:
    TmpAffs.c_Int += mod;
    break;

  case APPLY_KARMA_MAX:
    TmpAffs.m_Kar += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_KARMA:
    TmpAffs.c_Kar += mod;
    break;

  case APPLY_LUCK_MAX:
    TmpAffs.m_Luc += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_LUCK:
    TmpAffs.c_Luc += mod;
    break;

  case APPLY_POW_MAX:
    TmpAffs.m_Pow += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_POW:
    TmpAffs.c_Pow += mod;
    break;

  case APPLY_STR_MAX:
    TmpAffs.m_Str += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_STR:
    TmpAffs.c_Str += mod;
    break;

  case APPLY_WIS_MAX:
    TmpAffs.m_Wis += mod;
    /*
     * fall through, _MAX also affects current
     */
  case APPLY_WIS:
    TmpAffs.c_Wis += mod;
    break;

    /*
     * tricky, only the WORST 'magic' modifier applies, so we have to
     * check.  JAB
     */

  case APPLY_STR_RACE:
    if ((mod <= RACE_NONE) || (mod > LAST_RACE))
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_STR_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Str || (stat_factor[mod].Str > stat_factor[TmpAffs.r_Str].Str) )
      TmpAffs.r_Str = mod;
    break;
  case APPLY_DEX_RACE:
    if ((mod <= RACE_NONE) || (mod > LAST_RACE))
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_DEX_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Dex || (stat_factor[mod].Dex > stat_factor[TmpAffs.r_Dex].Dex) )
      TmpAffs.r_Dex = mod;
    break;
  case APPLY_AGI_RACE:
    if( (mod <= RACE_NONE) || (mod > LAST_RACE) )
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_AGI_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Agi || (stat_factor[mod].Agi > stat_factor[TmpAffs.r_Agi].Agi) )
      TmpAffs.r_Agi = mod;
    break;
  case APPLY_CON_RACE:
    if( (mod <= RACE_NONE) || (mod > LAST_RACE) )
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_CON_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Con || (stat_factor[mod].Con > stat_factor[TmpAffs.r_Con].Con) )
      TmpAffs.r_Con = mod;
    break;
  case APPLY_POW_RACE:
    if ((mod <= RACE_NONE) || (mod > LAST_RACE))
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_POW_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Pow || (stat_factor[mod].Pow > stat_factor[TmpAffs.r_Pow].Pow) )
      TmpAffs.r_Pow = mod;
    break;
  case APPLY_INT_RACE:
    if ((mod <= RACE_NONE) || (mod > LAST_RACE))
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_INT_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Int || (stat_factor[mod].Int > stat_factor[TmpAffs.r_Int].Int) )
      TmpAffs.r_Int = mod;
    break;
  case APPLY_WIS_RACE:
    if ((mod <= RACE_NONE) || (mod > LAST_RACE))
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_WIS_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Wis || (stat_factor[mod].Wis > stat_factor[TmpAffs.r_Wis].Wis) )
      TmpAffs.r_Wis = mod;
    break;
  case APPLY_CHA_RACE:
    if( (mod <= RACE_NONE) || (mod > LAST_RACE) )
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_CHA_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Cha || (stat_factor[mod].Cha > stat_factor[TmpAffs.r_Cha].Cha) )
      TmpAffs.r_Cha = mod;
    break;
  case APPLY_KARMA_RACE:
    if ((mod <= RACE_NONE) || (mod > LAST_RACE))
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_KARMA_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Kar || (stat_factor[mod].Kar > stat_factor[TmpAffs.r_Kar].Kar) )
      TmpAffs.r_Kar = mod;
    break;
  case APPLY_LUCK_RACE:
    if ((mod <= RACE_NONE) || (mod > LAST_RACE))
    {
      logit(LOG_DEBUG, "affect_modify(): unknown race (%d) for APPLY_LUCK_RACE.", loc);
      break;
    }
    if( !TmpAffs.r_Luc || (stat_factor[mod].Luk > stat_factor[TmpAffs.r_Luc].Luk) )
      TmpAffs.r_Luc = mod;
    break;

  case APPLY_FIRE_PROT:
    SET_BIT(TmpAffs.BV_1, AFF_PROT_FIRE);
    break;

#if 1
  case APPLY_ARMOR:
    TmpAffs.AC += mod;
    break;
#endif
  case APPLY_AGE:
    TmpAffs.Age += mod;
    break;

  case APPLY_DAMROLL:
    TmpAffs.Dam += mod;
    break;
  case APPLY_HITROLL:
    TmpAffs.Hit += mod;
    break;

  case APPLY_HIT:
    TmpAffs.Hits += mod;
    break;
  case APPLY_MOVE:
    TmpAffs.Move += mod;
    break;
  case APPLY_MANA:
    TmpAffs.Mana += mod;
    break;

  case APPLY_HIT_REG:
    TmpAffs.hit_reg += mod;
    break;
  case APPLY_MOVE_REG:
    TmpAffs.move_reg += mod;
    break;
  case APPLY_MANA_REG:
    TmpAffs.mana_reg += mod;
    break;

  case APPLY_SAVING_BREATH:
    TmpAffs.S_breath += mod;
    break;
  case APPLY_SAVING_PARA:
    TmpAffs.S_para += mod;
    break;
  case APPLY_SAVING_FEAR:
    TmpAffs.S_petri += mod;
    break;
  case APPLY_SAVING_ROD:
    TmpAffs.S_rod += mod;
    break;
  case APPLY_SAVING_SPELL:
    TmpAffs.S_spell += mod;
    break;
  case APPLY_CURSE:
    if (mod < 0)
      mod = -mod;

    TmpAffs.S_breath += mod;
    TmpAffs.S_para += mod;
    TmpAffs.S_petri += mod;
    TmpAffs.S_rod += mod;
    TmpAffs.S_spell += mod;
    break;
    /*
     * these 5 are all horribly bad ideas, possible we can imp things
     * to do these, but they sure as hell won't be APPLYs!  JAB
     */

  case APPLY_CLASS:
  case APPLY_EXP:
  case APPLY_GOLD:
  case APPLY_LEVEL:
  case APPLY_SEX:
    break;
    /*
     * and these 2 are pretty silly, so I'm not imping them.
     */

  case APPLY_CHAR_HEIGHT:
  case APPLY_CHAR_WEIGHT:
    break;
  case APPLY_SPELL_PULSE:
    TmpAffs.spell_pulse += mod;
    break;
  case APPLY_COMBAT_PULSE:
    TmpAffs.combat_pulse += mod;
    break;
  default:
    if (loc != 17)
    {
      logit(LOG_DEBUG, "affect_modify(): unknown apply (%d) from_eq (%d) mod (%d).", loc, from_eq, mod);
    }
    break;
  }
}

void get_aura_affects(P_char ch)
{
  affected_type af;

  memset(&af, 0, sizeof(affected_type));
  af.location = APPLY_HITROLL;
  af.modifier = GET_LEVEL(ch)/12;
  affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);

  af.location = APPLY_DAMROLL;
  af.modifier = GET_LEVEL(ch)/12;
  affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);

  if (GET_CLASS(ch, CLASS_AVENGER))
        {
    af.location = APPLY_HIT_REG;
    af.modifier = GET_LEVEL(ch)/2;
    affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);

    if (GET_SPEC(ch, CLASS_AVENGER, SPEC_INQUISITOR))
    {
      af.location = APPLY_CON_MAX;
      af.modifier = GET_LEVEL(ch)/10;
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }
    else if (GET_SPEC(ch, CLASS_AVENGER, SPEC_LIGHTBRINGER))
    {
      af.location = APPLY_ARMOR;
      af.modifier = -(GET_LEVEL(ch));
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }
  }
  else if (GET_CLASS(ch, CLASS_DREADLORD))
  {
    af.location = APPLY_SAVING_FEAR;
    af.modifier = -(GET_LEVEL(ch)/10);
    affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);

    if (GET_SPEC(ch, CLASS_DREADLORD, SPEC_DEATHLORD))
    {

      af.location = APPLY_STR_MAX;
      af.modifier = GET_LEVEL(ch)/10;
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }
    else if (GET_SPEC(ch, CLASS_DREADLORD, SPEC_SHADOWLORD))
    {
      af.location = APPLY_AGI_MAX;
      af.modifier = GET_LEVEL(ch)/10;
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }
}
}

void get_epic_stat_affects(P_char ch)
{
  affected_type af;

  memset(&af, 0, sizeof(affected_type));

 /* This is for the epic stat adjustment skill thingie. This seemed like the best place to put it,
     but if anyone who is reading this and is having issues with it, or simply found a better way,
  I highly recommend moving it somewhere less obtrusive-maybe in it's own function? -Zion 9/28/2008
  */

  if (GET_CHAR_SKILL(ch, SKILL_EPIC_STRENGTH) > 0)
    {
      af.location = APPLY_STR_MAX;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_EPIC_STRENGTH) / 10);
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }

  if (GET_CHAR_SKILL(ch, SKILL_EPIC_POWER) > 0)
    {
      af.location = APPLY_POW_MAX;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_EPIC_POWER) / 10);
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }

  if (GET_CHAR_SKILL(ch, SKILL_EPIC_AGILITY) > 0)
    {
      af.location = APPLY_AGI_MAX;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_EPIC_AGILITY) / 10);
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }

  if (GET_CHAR_SKILL(ch, SKILL_EPIC_INTELLIGENCE) > 0)
    {
      af.location = APPLY_INT_MAX;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_EPIC_INTELLIGENCE) / 10);
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }

  if (GET_CHAR_SKILL(ch, SKILL_EPIC_DEXTERITY) > 0)
    {
      af.location = APPLY_DEX_MAX;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_EPIC_DEXTERITY) / 10);
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }

  if (GET_CHAR_SKILL(ch, SKILL_EPIC_WISDOM) > 0)
    {
      af.location = APPLY_WIS_MAX;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_EPIC_WISDOM) / 10);
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }

  if (GET_CHAR_SKILL(ch, SKILL_EPIC_CONSTITUTION) > 0)
    {
      af.location = APPLY_CON_MAX;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_EPIC_CONSTITUTION) / 10);
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }

  if (GET_CHAR_SKILL(ch, SKILL_EPIC_CHARISMA) > 0)
    {
      af.location = APPLY_CHA_MAX;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_EPIC_CHARISMA) / 10);
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }

  if (GET_CHAR_SKILL(ch, SKILL_EPIC_LUCK) > 0)
    {
      af.location = APPLY_LUCK_MAX;
      af.modifier = (GET_CHAR_SKILL(ch, SKILL_EPIC_LUCK) / 10);
      affect_modify(af.location, af.modifier, &(af.bitvector), FALSE);
    }
}


/*
 * Scan all equipment and affect structures attached to a character,
 * summing all the alterations, then either subtract them all, or add them
 * all.
 *
 * mode FALSE - remove all effects. mode TRUE  - apply all affects.
 *
 * JAB
 */

void all_affects(P_char ch, int mode)
{
  struct affected_type *af, *next;
  int      i, j;

  if (ch == NULL)               /* replaced call to SanityCheck with this */
    return;

  bzero(&TmpAffs, sizeof(struct hold_data));

  for (i = 0; i < MAX_WEAR; i++)
  {
    if (!ch->equipment[i])
    {
      continue;
    }
    /* held items only have effect, if they can only be held */
    if((i == HOLD || i == WIELD || i == WIELD2 || i == WIELD3 || i == WIELD4)
      && ch->equipment[i]->type != ITEM_WEAPON &&
      ch->equipment[i]->type != ITEM_FIREWEAPON &&
      (ch->equipment[i]->
        wear_flags & ~(ITEM_TAKE | ITEM_HOLD | ITEM_ATTACH_BELT)))
    {
      continue;
    }
    if(i == HOLD && (ch->equipment[i]->type == ITEM_WEAPON ||
      ch->equipment[i]->type == ITEM_FIREWEAPON))
    {
      continue;
    }
    //allowing first normal beltable item to grant stats. 10/8/12 Drannak
    if( (i == WEAR_ATTACH_BELT_2 ||
      i == WEAR_ATTACH_BELT_3 || i == WEAR_BACK) &&
      !IS_ARTIFACT(ch->equipment[i]) )
    {
        continue;
    }
// Below commented code is to hunt bad object affects.
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
    {
    affect_modify(ch->equipment[i]->affected[j].location,
                 ch->equipment[i]->affected[j].modifier,
                 &(ch->equipment[i]->bitvector), TRUE);
    }
    affect_modify(APPLY_AC, -(apply_ac(ch, i)), NULL, FALSE);
  }

  /* HERE is the place to go into TmpAffs and tone things down */
  TmpAffs.Hits = BOUNDED(0, TmpAffs.Hits, GET_LEVEL(ch) * 3);

  /* This is where we handle damroll cap via level. Now 240 at 56. */
  TmpAffs.Dam = MIN(GET_LEVEL(ch) * 4 + 16, TmpAffs.Dam);

  for (af = ch->affected; af; af = af->next)
  {
    if ( !IS_SET(af->flags, AFFTYPE_NOAPPLY) )
    {
      affect_modify(af->location, af->modifier, &(af->bitvector), FALSE);
    }
  }

  get_epic_stat_affects(ch);

  if (in_command_aura(ch))
  {
    get_aura_affects(ch->group->ch);
  }

  apply_affs(ch, mode);
  /*
   * now recalc con bonus, since we could have just changed something
   * dealing with Con
   */
  if (IS_PC(ch))
  {
    int missing_hps = GET_MAX_HIT(ch) - GET_HIT(ch);
    int missing_mana = GET_MAX_MANA(ch) - GET_MANA(ch);

    GET_MAX_HIT(ch) = calculate_hitpoints2(ch);
    GET_HIT(ch) = GET_MAX_HIT(ch) - missing_hps;
    GET_MAX_MANA(ch) += calculate_mana(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch) - missing_mana;
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
  if ((af = get_spell_from_char(ch, TAG_CTF_BONUS)) != NULL)
  {
    int      missing_hps = GET_MAX_HIT(ch) - GET_HIT(ch);
    int      missing_vitality = GET_MAX_VITALITY(ch) - GET_VITALITY(ch);
    int num = af->modifier;
    GET_MAX_HIT(ch) += (MIN(num, 20)*5);
    GET_HIT(ch) = GET_MAX_HIT(ch) - missing_hps;
    GET_MAX_VITALITY(ch) += (MIN(num, 20)*5);
    GET_VITALITY(ch) = GET_MAX_VITALITY(ch) - missing_vitality;
    ch->points.hitroll += BOUNDED(0, ch->points.hitroll + num, 20);
    ch->points.damroll += BOUNDED(0, ch->points.damroll + num, 20);
    if (num >= 50)
    {
      ch->points.combat_pulse = (int)((float)ch->points.combat_pulse / 1.5);
      ch->points.spell_pulse = (int)((float)ch->points.spell_pulse / 1.5);
    }
  }
#endif

}

/*
 * This updates a character by resetting affectable values to base states,
 * then reaffecting everything
 */

char affect_total(P_char ch, int kill_ch)
{
  P_char killer;

  if( !ch )
  {
    return FALSE;
  }

  all_affects(ch, FALSE);       /*
                                 * effectively resets character to a state
                                 * with NO affects
                                 */
  /*
   * be very very careful what you put in here, if, for example, someone
   * is close to death, and then you do something to force an update in
   * here, and they happen to be wearing some +hitpoints items, you just
   * killed them mysteriously.
   */

  all_affects(ch, TRUE);        /*
                                 * now add them all back
                                 */

  if( kill_ch && (GET_HIT(ch) < -10) && (GET_STAT(ch) != STAT_DEAD)
    && (IS_NPC(ch) || !ch->desc || (ch->desc && (ch->desc->connected == CON_PLAYING))) )
  {
    statuslog(ch->player.level, "%s killed in %d (%d hits) (affect_total)",
      GET_NAME(ch), ROOM_VNUM(ch->in_room), GET_HIT(ch));
    logit(LOG_DEATH, "%s killed in %d (%d hits) (affect_total)",
      GET_NAME(ch), ROOM_VNUM(ch->in_room), GET_HIT(ch));

    // No more vit death or zerk death to avoid frags
    for( killer = world[ch->in_room].people; killer; killer = killer->next_in_room )
    {
      if( GET_OPPONENT(killer) && GET_OPPONENT(killer) == ch
        && killer->in_room == ch->in_room && GET_RACEWAR(killer) != GET_RACEWAR(ch) )
      {
        die(ch, killer);
        return TRUE;
      }
    }

    die(ch, ch);
    return TRUE;
  }

  // Almost the same as original, just changed get_prop... to pulse_all for speed.
  ch->specials.base_combat_round = combat_by_race[GET_RACE(ch)][0] + pulse_all;

  /* Original:
  ch->specials.base_combat_round = (int)(combat_by_race[GET_RACE(ch)][0]);
  ch->specials.base_combat_round += (int)(get_property("damage.pulse.class.all", 2));
  */

/*
 * This is the new style combat calculation - Drannak
  ch->specials.base_combat_round = (200 - ch->base_stats.Agi);
  ch->specials.base_combat_round *= .13;
  if(GET_C_AGI(ch) > 100) //diminishing returns
   {
     int cmod = (GET_C_AGI(ch) - 100);
     if (cmod > 0 && cmod < 5)
	ch->specials.base_combat_round -= 1;
     else if(cmod >=5 && cmod < 15)
	ch->specials.base_combat_round -= 2;
     else if(cmod >=15 && cmod < 30)
       ch->specials.base_combat_round -= 3;
     else if(cmod >=30)
	ch->specials.base_combat_round -= 4;
   }
  else
   {
    int cmod = (100 - GET_C_AGI(ch));
	cmod *= .1;
	ch->specials.base_combat_round += (int)cmod;
    }
*/
  ch->specials.damage_mod = combat_by_race[GET_RACE(ch)][1];

  // Add class modifiers for pulse and damage.
  if (IS_PC(ch))
  {
    if (IS_MULTICLASS_PC(ch))
    {
      ch->specials.base_combat_round += MIN(combat_by_class[flag2idx(ch->player.m_class)][0],
        combat_by_class[flag2idx(ch->player.secondary_class)][0]);
      ch->specials.damage_mod *= MAX(combat_by_class[flag2idx(ch->player.m_class)][1],
        combat_by_class[flag2idx(ch->player.secondary_class)][1]);
    }
    else
    {
      ch->specials.base_combat_round += (combat_by_class[flag2idx(ch->player.m_class)][0]);
      ch->specials.damage_mod *= combat_by_class[flag2idx(ch->player.m_class)][1];
    }
  }
  else
  {
    // adjust damage_mod based on zone difficulty
    // we have to do this here because damage_mod is reset here every time it's called
    int zone_difficulty = BOUNDED(1, zone_table[world[real_room0(GET_BIRTHPLACE(ch))].zone].difficulty, 10);

    if( zone_difficulty > 1 )
    {
      float damage_mod_mod = 1.0+(get_property("damage.zoneDifficulty.mod.factor", 0.200)*zone_difficulty);
      ch->specials.damage_mod = (float) (ch->specials.damage_mod * damage_mod_mod);
    }
  }

/* For testing.. just displays the base/mod/new for combat pulse.
 * This is old.. We just now do Base and New:
  if( IS_PC(ch) ) debug( "Base: %.2f, Mod: %.2f, New: %.2f, Final: %d.",
    ch->specials.base_combat_round, COMBAT_PULSE(ch), ch->specials.base_combat_round * COMBAT_PULSE(ch),
    (int)(ch->specials.base_combat_round * COMBAT_PULSE(ch) + .5) );
  if( IS_PC(ch) ) debug( "Base: %.2f, New: %.2f", ch->specials.base_combat_round, COMBAT_PULSE(ch) );
*/
  // Multiply it by the pulse modifier (and add .5 for the rounding).
  ch->specials.base_combat_round = COMBAT_PULSE(ch);
  /* Original:
  ch->specials.base_combat_round += ch->points.combat_pulse;
  */

  if (innate_two_daggers(ch))
    ch->specials.base_combat_round += get_property("innate.dualDaggers.pulse", -3.0);

  if (IS_AFFECTED2(ch, AFF2_FLURRY))
    ch->specials.base_combat_round = (get_property("innate.flurry.pulse", .70) * ch->specials.base_combat_round);

  if( GET_CLASS(ch, CLASS_REAVER) )
    apply_reaver_mods(ch);

  ch->specials.base_combat_round = MAX(3.0, ch->specials.base_combat_round);

  if (IS_PC(ch) && GET_CHAR_SKILL(ch, SKILL_MINE) >= 1)
	  ch->specials.affected_by5 |= AFF5_MINE; /* high enough skill in forge grants miner's sight */

  /* only if actually in game. JAB */
  if (ch->desc && !ch->desc->connected && (GET_STAT(ch) != STAT_DEAD))
  {
    if (GET_HIT(ch) != GET_MAX_HIT(ch) || ch->points.hit_reg < 0)
      StartRegen(ch, EVENT_HIT_REGEN);
    if (GET_VITALITY(ch) != GET_MAX_VITALITY(ch) || ch->points.move_reg < 0)
      StartRegen(ch, EVENT_MOVE_REGEN);
    if (GET_MANA(ch) != GET_MAX_MANA(ch) || ch->points.mana_reg < 0)
      StartRegen(ch, EVENT_MANA_REGEN);
  }

  return FALSE;
}

//=================================================================================
//=== AFFECTS - CHAR
//=================================================================================
void event_short_affect(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct event_short_affect_data *event_data = (struct event_short_affect_data*)data;
  struct affected_type *af;

  // The affect was already removed.
  if( data == NULL )
  {
    return;
  }

  for (af = event_data->ch->affected; af; af = af->next)
    if (af == event_data->af)
      break;

  if (!af)
    return;

  wear_off_message(event_data->ch, af);
  affect_remove(event_data->ch, af);
}

/*
 * insert an affect_type in a char_data structure automatically sets
 * apropriate bits and apply's
 */

struct affected_type *affect_to_char(P_char ch, struct affected_type *af)
{
  struct affected_type *affected_alloc;

  if (!dead_affect_pool)
    dead_affect_pool = mm_create("AFFECTS", sizeof(struct affected_type),
                                 offsetof(struct affected_type, next), 10);

  affected_alloc = (struct affected_type *) mm_get(dead_affect_pool);
  *affected_alloc = *af;

  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;
  if( !IS_SET(af->flags, AFFTYPE_NOAPPLY) )
  {
    ch->specials.affected_by |= af->bitvector;
    ch->specials.affected_by2 |= af->bitvector2;
    ch->specials.affected_by3 |= af->bitvector3;
    ch->specials.affected_by4 |= af->bitvector4;
    ch->specials.affected_by5 |= af->bitvector5;
  }

  balance_affects(ch);

  if ( IS_SET(af->flags, AFFTYPE_SHORT) )
  {
    struct event_short_affect_data data;
    data.ch = ch;
    data.af = affected_alloc;
    add_event(event_short_affect, af->duration, ch, 0, 0, 0, &data, sizeof(data));
  }

  return affected_alloc;
}

void affect_to_char_with_messages(P_char ch, struct affected_type *af,
                                  char *wear_off_char, char *wear_off_room)
{
  struct affected_type *affected_alloc;
  Skill   *skill;
  int      i;

  if (!wear_off_char && !wear_off_room)
  {
    af->flags |= AFFTYPE_NOMSG;
    affect_to_char(ch, af);
    return;
  }

  affected_alloc = affect_to_char(ch, af);

  skill = &(skills[af->type]);
  /* since the code is a bit unclear I will explain:
   * gota find index under which we stored exactly
   * the same messages or first index with both
   * room and char messages empty ie, a free slot. */
  for (i = 0;
       i < MAX_WEAR_OFF_MESSAGES && (skill->wear_off_char[i] ||
                                     skill->wear_off_room[i]); i++)
  {
    if ((!wear_off_char && skill->wear_off_char[i]) ||
        (!wear_off_room && skill->wear_off_room[i]) ||
        (wear_off_char && !skill->wear_off_char[i]) ||
        (wear_off_room && !skill->wear_off_room[i]))
      continue;
    if ((!wear_off_char || !strcmp(skill->wear_off_char[i], wear_off_char)) &&
        (!wear_off_room || !strcmp(skill->wear_off_room[i], wear_off_room)))
      break;
  }

  /* do not store custom messages under 0 */
  if (i == 0 && !skill->wear_off_char[i] && !skill->wear_off_room[i])
    i = 1;

  if (i == MAX_WEAR_OFF_MESSAGES)
  {
    affected_alloc->wear_off_message_index = 0;
  }
  else if (skill->wear_off_char[i] == 0 && skill->wear_off_room[i] == 0)
  {
    if (wear_off_char)
      skill->wear_off_char[i] = str_dup(wear_off_char);
    if (wear_off_room)
      skill->wear_off_room[i] = str_dup(wear_off_room);
    affected_alloc->wear_off_message_index = i;
  }
  else
  {
    affected_alloc->wear_off_message_index = i;
  }
}

void set_short_affected_by(P_char ch, int spell, int duration)
{
  struct affected_type *affected_alloc;
  struct event_short_affect_data data;

  if (!dead_affect_pool)
    dead_affect_pool = mm_create("AFFECTS", sizeof(struct affected_type),
                                 offsetof(struct affected_type, next), 10);

  affected_alloc = (struct affected_type *) mm_get(dead_affect_pool);

  memset(affected_alloc, 0, sizeof(struct affected_type));
  affected_alloc->type = spell;
  affected_alloc->flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
  affected_alloc->duration = duration;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  balance_affects(ch);

  data.ch = ch;
  data.af = affected_alloc;
  add_event(event_short_affect, duration, ch, 0, 0, 0, &data, sizeof(data));
}

// used by memorization system because we care about the order there
// not the prettiest solution ever but better than having dedicated
// structures with special handling in files etc
void affect_to_end(P_char ch, struct affected_type *af)
{
  struct affected_type *afp, *prev = NULL;
  int      create = TRUE;

  for (afp = ch->affected; afp; afp = afp->next)
  {
    if (afp == af)
    {
      create = FALSE;
      if (prev)
        prev->next = afp->next;
      else
        ch->affected = afp->next;
    }
    else
      prev = afp;
  }
  if (create)
  {
    if (!dead_affect_pool)
      dead_affect_pool = mm_create("AFFECTS", sizeof(struct affected_type),
                                   offsetof(struct affected_type, next), 10);

    afp = (struct affected_type *) mm_get(dead_affect_pool);
    *afp = *af;
  }
  else
    afp = af;
  afp->next = NULL;
  if (prev)
    prev->next = afp;
  else
    ch->affected = afp;
}

void affect_remove(P_char ch, struct affected_type *af)
{
  struct affected_type *hjp;
  P_nevent pnev;

  if( !(ch && ch->affected) )
  {
    logit(LOG_EXIT, "affect_remove(): %s: %s", (ch ? "(NULL)" : J_NAME(ch)), (ch ? "no affects." : "no ch."));
    raise(SIGSEGV);
  }

  /*
   * remove structure *af from linked list
   */

  all_affects(ch, FALSE);
  // If af is at the head of list
  if (ch->affected == af)
  {
    ch->affected = af->next;
  }
  else
  {
    // Look for previous affect
    for( hjp = ch->affected; hjp->next != NULL; hjp = hjp->next )
    {
      if( hjp->next == af )
        break;
    }

    if( hjp->next != af )
    {
      logit(LOG_EXIT, "affect_remove(): could not locate affected_type in ch->affected for %s.", GET_NAME(ch));
      raise(SIGSEGV);
    }
    // Remove af from the list.
    hjp->next = af->next;
  }

  // Handle race changes.
  if( af->type == TAG_RACE_CHANGE )
  {
    ch->player.race = af->modifier;
  }

  // If it's a short affect.
  if( IS_SET(af->flags, AFFTYPE_SHORT) )
  {
    // Kill the timer on the corresponding event - let it die on its own.
    LOOP_EVENTS_CH( pnev, ch->nevents )
    {
      if( pnev->func == event_short_affect && pnev->data != NULL
        && ((struct event_short_affect_data*)(pnev->data))->af == af )
      {
        FREE( pnev->data );
        pnev->data = NULL;
        pnev->timer = 1;
        break;
      }
    }
  }

  if( IS_SET(af->flags, AFFTYPE_LINKED_CH) )
  {
    unlink_char_affect(ch, af);
  }
  if( IS_SET(af->flags, AFFTYPE_LINKED_OBJ) )
  {
    unlink_char_obj_affect(ch, af);
  }

  mm_release(dead_affect_pool, af);

  af = NULL;
  all_affects(ch, TRUE);

  char_light(ch);
  room_light(ch->in_room, REAL);

  balance_affects(ch);
}

/*
 * call affect_remove with every spell of spelltype "skill"
 */

void affect_from_char(P_char ch, int skill)
{
  struct affected_type *hjp, *tmp;

  for (hjp = ch->affected; hjp; hjp = tmp)
  {
    tmp = hjp->next;
    if( hjp->type == skill )
    {
      affect_remove(ch, hjp);
    }
  }
}

/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX)
 */

struct affected_type *get_spell_from_char(P_char ch, int spell)
{
  struct affected_type *hjp;

  if( !IS_ALIVE(ch) || !(spell) )
  {
    return NULL;
  }

  for( hjp = ch->affected; hjp; hjp = hjp->next )
  {
    if( hjp->type == spell )
    {
      return hjp;
    }
  }
  return NULL;
}

bool affected_by_spell(P_char ch, int skill)
{
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->type == skill)
      return (TRUE);

  return (FALSE);
}

int affected_by_spell_count(P_char ch, int skill)
{
  int count = 0;
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->type == skill)
      count++;

  return count;
}

bool affected_by_skill(P_char ch, int skill)
{
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->type == TAG_SKILL_TIMER &&
        hjp->modifier == skill)
      return (TRUE);

  return (FALSE);
}

bool affected_by_spell_flagged(P_char ch, int skill, uint flags)
{
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->type == skill &&
        (hjp->flags & (AFFTYPE_CUSTOM1 | AFFTYPE_CUSTOM2)) == flags)
      return (TRUE);

  return (FALSE);
}

void affect_join(P_char ch, struct affected_type *af, int avg_dur,
                 int avg_mod)
{
  struct affected_type *hjp;
  bool     found = FALSE;

  for (hjp = ch->affected; !found && hjp; hjp = hjp->next)
  {
    if (hjp->type == af->type)
    {

      af->duration += hjp->duration;
      if (avg_dur)
        af->duration /= 2;

      af->modifier += hjp->modifier;
      if (avg_mod)
        af->modifier /= 2;

      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }

  if (!found)
    affect_to_char(ch, af);

  balance_affects(ch);
}

//---------------------------------------------------------------------------------
void wear_off_message(P_char ch, struct affected_type *af)
{
  if ((af->flags & AFFTYPE_NOMSG))      //|| (af->flags & AFFTYPE_SUBAFFECT))
    return;

  if( af->type == TAG_INNATE_TIMER && af->location == INNATE_LAY_HANDS )
  {
    af->type = TAG_LAYONHANDS;
  }

  if (af->wear_off_message_index)
  {
    if (skills[af->type].wear_off_char[af->wear_off_message_index])
    {
      send_to_char(skills[af->type].wear_off_char[af->wear_off_message_index], ch);
      send_to_char("\n", ch);
    }
    if (skills[af->type].wear_off_room[af->wear_off_message_index])
    {
      act(skills[af->type].wear_off_room[af->wear_off_message_index], FALSE,
          ch, 0, 0, TO_ROOM);
    }
  }
  else
  {
    if (skills[af->type].wear_off_char[0])
    {
      send_to_char(skills[af->type].wear_off_char[0], ch);
      send_to_char("\n", ch);
    }
    if (skills[af->type].wear_off_room[0])
    {
      act(skills[af->type].wear_off_room[0], FALSE, ch, 0, 0, TO_ROOM);
    }
  }

}

//=================================================================================
//=== AFFECTS - ROOM
//=================================================================================

//---------------------------------------------------------------------------------
void event_room_affect(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct event_room_affect_data *event_data = (struct event_room_affect_data *)data;
  struct room_affect *af;
  int room = event_data->room;

  for( af = world[room].affected; af; af = af->next )
  {
    if( af == event_data->af )
    {
      break;
    }
  }

  if( af == NULL )
  {
    return;
  }

  if( af->ch == NULL || !is_char_in_room(af->ch, room) )
  {
    if( skills[af->type].wear_off_room[0] )
    {
      send_to_room(skills[af->type].wear_off_room[0], room);
      send_to_room("\n", room);
    }
  }
  else if( skills[af->type].wear_off_room[0] )
  {
    act(skills[af->type].wear_off_room[0], FALSE, af->ch, 0, 0, TO_ROOM);
    if( skills[af->type].wear_off_char[0] )
    {
      act(skills[af->type].wear_off_char[0], FALSE, af->ch, 0, 0, TO_CHAR);
    }
  }
  affect_room_remove(room, af);
}

/*
 * insert an affect_type in a room_data structure automatically sets
 * apropriate bits and apply's
 * use with extreme care! it doesnt handle checking of original flags
 * so if room was say SILENT originally and you set it to silent with
 * affect, SILENT flag will be removed when affect wears off
 */
//---------------------------------------------------------------------------------
struct room_affect *affect_to_room(int room, struct room_affect *af)
{
  struct room_affect *affected_alloc;
  struct event_room_affect_data data;

  if( af->duration <= 0 )
  {
    return NULL;
  }

  if( !dead_room_affect_pool )
  {
    dead_room_affect_pool = mm_create("ROOM_AFFECTS", sizeof(struct room_affect),
      offsetof(struct room_affect, next), 10);
  }

  affected_alloc = (struct room_affect *) mm_get(dead_room_affect_pool);

  *affected_alloc = *af;
  affected_alloc->next = world[room].affected;
  world[room].affected = affected_alloc;

  // This line guarentees that af->room_flags don't include any flags already set.
  //   This is important for when the affect wears off.
  affected_alloc->room_flags = affected_alloc->room_flags & ~(world[room].room_flags);
  world[room].room_flags |= affected_alloc->room_flags;

  data.room = room;
  data.af = affected_alloc;
  add_event(event_room_affect, af->duration, 0, 0, 0, 0, &data, sizeof(data));
  affected_alloc->duration = 1 + af->duration / PULSES_IN_TICK;

  return affected_alloc;
}

//---------------------------------------------------------------------------------
void affect_room_remove(int room, struct room_affect *af)
{
  struct room_affect *hjp;

  // For this to work right, we need to be sure that af.room_flags doesn't conatin
  //   any flags that world[room] had originally.  This is done in affect_to_room
  world[room].room_flags -= af->room_flags;

  if( world[room].affected == af )
  {
    world[room].affected = af->next;
  }
  else
  {
    for( hjp = world[room].affected; (hjp->next) && (hjp->next != af); hjp = hjp->next )
    {
      ;
    }

    if( hjp->next != af )
    {
      return;
    }
    hjp->next = af->next;
  }

  mm_release(dead_room_affect_pool, af);

  af = NULL;
}

//---------------------------------------------------------------------------------
struct room_affect *get_spell_from_room(P_room room, int skill)
{
  struct room_affect *hjp;

  for (hjp = room->affected; hjp; hjp = hjp->next)
    if (hjp->type == skill)
      return hjp;

  return NULL;
}


//=================================================================================
//=== AFFECTS - OBJECT
//=================================================================================

struct obj_affect *get_obj_affect(P_obj obj, int spell)
{
  struct obj_affect *af;

  for (af = obj->affects; af; af = af->next)
    if (af->type == spell)
      return af;

  return NULL;
}

void recalculate_obj_extra(P_obj obj)
{
  struct obj_affect *t_af;

  t_af = get_obj_affect(obj, TAG_ALTERED_EXTRA2);
  if (!t_af)
    return;

  obj->extra2_flags = t_af->extra2;

  for (t_af = obj->affects; t_af; t_af = t_af->next)
    obj->extra2_flags |= t_af->extra2;
}

void obj_affect_remove(P_obj obj, struct obj_affect *af)
{
  struct obj_affect *t_af;
  P_nevent e;

  if( !obj )
  {
    return;
  }

  // This should fix a crash bug that occurs when an item's condition < 0.
  if( obj->condition < 0 )
  {
    obj->condition = 1;
  }

  // Find the assoicated event and disarm it
  LOOP_EVENTS_OBJ( e, obj->nevents )
  {
    if( e->func != event_obj_affect )
      continue;
    if( *((struct obj_affect **)e->data) == af )
    {
      disarm_single_event(e);
      break;
    }
  }

  if( obj->affects == af )
  {
    obj->affects = af->next;
  }
  else
  {
    for( t_af = obj->affects; t_af && t_af->next != af; t_af = t_af->next )
      ;

    if( t_af )
      t_af->next = af->next;
    else
    {
      // it is debatable on what should happen here.
      // basically this condition is met when you tell this function
      // to remove an affect on an object that it doesn't have.
      // this is happening regularly, actually - if you look at
      // event_obj_affect(), the Decay() call can end in a chain with
      // free_obj(), with will iterate over the object's affects and remove
      // them.  unfortunately, event_obj_affect() has no idea that
      // happened, so it calls this function.
      //
      // The BIG problem here is at the end of the function, where it
      // calls mm_release() to put this affect back in dead_obj_affect_pool.
      // That essentially puts the object at the tail of the dead pool
      // twice, meaning when the second instance of it is reused, there
      // is no telling what the base address + next_offset is going to
      // point to, but it sure as hell isn't going to be the next object
      // in the dead_obj_affect_pool (since the internals of the object
      // have been changed when the first instance of the object was reused).
      // That in turn should completely hose everything in that pool, and
      // possiblely cause memory corruption else where.
      //
      // Perhaps the game should die here, but it looks like just returning
      // at this point might fix the problem.
      return;
    }
  }

  if( af->extra2 && af->type != TAG_ALTERED_EXTRA2 )
  {
    recalculate_obj_extra(obj);
    for( t_af = obj->affects; t_af; t_af = t_af->next)
      if( t_af->extra2 && t_af->type != TAG_ALTERED_EXTRA2 )
        break;

    if( !t_af && (t_af = get_obj_affect(obj, TAG_ALTERED_EXTRA2)) )
      obj_affect_remove(obj, t_af);
  }

  mm_release(dead_obj_affect_pool, af);
}

void event_obj_affect(P_char, P_char, P_obj obj, void *data)
{
  struct obj_affect *af = *((struct obj_affect **)data);

  obj_affect_remove(obj, af);

  if( af->type == TAG_OBJ_DECAY )
    Decay(obj);
}

/*
void event_obj_affect(P_obj obj, struct obj_affect *af)
{
  if (af->type == TAG_OBJ_DECAY)
    Decay(obj);

  obj_affect_remove(obj, af);
}
*/

struct obj_affect *get_spell_from_obj(P_obj obj, int spell)
{
  struct obj_affect *af;

  for (af = obj->affects; af; af = af->next)
    if (af->type == spell)
      return af;

  return NULL;
}

void set_obj_affected_extra(P_obj obj, int time, sh_int spell, sh_int data, ulong extra2)
{
  struct obj_affect *af;

  if( !obj )
  {
    debug( "set_obj_affected_extra: NULL obj!?" );
    return;
  }

  if (!dead_obj_affect_pool)
    dead_obj_affect_pool = mm_create("OBJ_AFFECTS", sizeof(struct obj_affect),
                                     offsetof(struct obj_affect, next), 100);
  af = (struct obj_affect *) mm_get(dead_obj_affect_pool);

  af->type = spell;
  af->data = data;
  af->extra2 = extra2;
  af->next = obj->affects;
  obj->affects = af;

  if( extra2 && !get_obj_affect(obj, TAG_ALTERED_EXTRA2) && spell != TAG_ALTERED_EXTRA2 )
    set_obj_affected_extra(obj, -1, TAG_ALTERED_EXTRA2, 0, obj->extra2_flags);

  obj->extra2_flags |= extra2;

  if (time >= 0)
  {
    add_event(event_obj_affect, time, 0, 0, obj, 0, &af, sizeof(struct obj_affect*));
  }
}

void set_obj_affected(P_obj obj, int time, sh_int spell, sh_int data)
{
  set_obj_affected_extra(obj, time, spell, data, 0);
}

int obj_affect_time(P_obj obj, struct obj_affect *af)
{
  P_nevent e;

  LOOP_EVENTS_OBJ( e, obj->nevents )
  {
    if( e->func != event_obj_affect )
      continue;

    if( *((struct obj_affect **)e->data) == af )
      return ne_event_time(e);
  }
  return -1;
/*
  P_event  e;

  for (e = obj->events; e; e = e->next)
    if (e->type == EVENT_OBJ_AFFECT &&
        (struct obj_affect *) e->target.t_ch == af)
      return event_time(e, T_PULSES);

  return -1;
*/
}

int affect_from_obj(P_obj obj, sh_int spell)
{
  struct obj_affect *af, *next;

  struct affected_type *hjp, *tmp;

  for( af = obj->affects; af; af = next )
  {
    next = af->next;
    if (af->type == spell)
    {
      obj_affect_remove(obj, af);
    }
  }
  return 0;
}


//=================================================================================
//=== AFFECTS - FOR ADDING TIMER AFFECT
//=================================================================================

void add_tag_to_char(P_char ch, int tag, int modifier, int flags)
{
  struct affected_type af;
  memset(&af, 0, sizeof(af));
  af.type = tag;
  af.flags = flags;
  af.modifier = modifier;
  af.duration = -1;
  affect_to_char(ch, &af);  
}

//---------------------------------------------------------------------------------
bool affect_timer(P_char ch, int time, int spell)
{
  struct affected_type af, *afp;

  if (IS_TRUSTED(ch))
    return true;

  for (afp = ch->affected; afp; afp = afp->next)
    if (afp->type == TAG_SKILL_TIMER &&
        afp->modifier == spell)
      return false;

  memset(&af, 0, sizeof(af));
  af.type = TAG_SKILL_TIMER;
  af.flags = AFFTYPE_STORE | AFFTYPE_SHORT;
  af.duration = time;
  af.modifier = spell;
  affect_to_char(ch, &af);

  return true;
}

//---------------------------------------------------------------------------------
int counter(P_char ch, int tag)
{
  for (struct affected_type *afp = ch->affected; afp; afp = afp->next)
  {
    if (afp->type == tag)
    {
      return afp->modifier;
    }
  }

  return -1;
}

//---------------------------------------------------------------------------------
void add_counter(P_char ch, int tag)
{
  add_counter(ch, tag, 1, -1);
}

//---------------------------------------------------------------------------------
void add_counter(P_char ch, int tag, int value, int duration)
{
  for (struct affected_type *afp = ch->affected; afp; afp = afp->next)
  {
    if (afp->type == tag)
    {
      afp->modifier = MAX(0, afp->modifier + value);
      afp->duration = duration;
      return;
    }
  }

  struct affected_type af;
  memset(&af, 0, sizeof(af));
  af.type = tag;
  af.flags = AFFTYPE_PERM | AFFTYPE_NODISPEL;
  af.modifier = value;
  af.duration = duration;
  affect_to_char(ch, &af);
}

//---------------------------------------------------------------------------------
void remove_counter(P_char ch, int tag)
{
  affect_from_char(ch, tag);
}

//---------------------------------------------------------------------------------
void remove_counter(P_char ch, int tag, int modifier)
{
  for (struct affected_type *afp = ch->affected; afp; afp = afp->next)
  {
    if (afp->type == tag && afp->modifier == modifier)
    {
      affect_remove(ch, afp);
      return;
    }
  }
}

//=================================================================================
//=== AFFECTS - LINKING
//=================================================================================

/* links api allows to create symbolic links between characters.
 * it is guaranteed the character we are linked to is still in game.
 * examples of links are guard skill, events with interaction of
 * 2 characters.
 */

//---------------------------------------------------------------------------------
void define_link(int type, char *name, link_breakage_func break_func, int flags)
{
  link_types[type].name = name;
  link_types[type].break_func.ch = break_func;
  // Do not flag as object.
  link_types[type].flags = flags & ~LNKFLG_OBJECT;
}

void define_olink(int type, char *name, link_obj_breakage_func break_func, int flags)
{
  link_types[type].name = name;
  link_types[type].break_func.obj = break_func;
  // Flag as object
  link_types[type].flags = flags | LNKFLG_OBJECT;
}

//---------------------------------------------------------------------------------
void initialize_links()
{
  memset(link_types, 0, sizeof(link_types));

  define_link(LNK_CONSENT,          "CONSENT",            NULL,             LNKFLG_EXCLUSIVE);
  define_link(LNK_RIDING,           "RIDING",             NULL,             LNKFLG_EXCLUSIVE);
  define_link(LNK_GUARDING,         "GUARDING",           guard_broken,     LNKFLG_ROOM);
  define_link(LNK_EVENT,            "EVENT",              event_broken,     LNKFLG_NONE);
  define_link(LNK_SNOOPING,         "SNOOPING",           NULL,             LNKFLG_NONE);
  define_link(LNK_FLANKING,         "FLANKING",           flanking_broken,  LNKFLG_ROOM | LNKFLG_EXCLUSIVE);
  define_link(LNK_BATTLE_ORDERS,    "BATTLE_ORDERS",      NULL,             LNKFLG_ROOM | LNKFLG_EXCLUSIVE);
  define_link(LNK_SONG,             "SONG",               song_broken,      LNKFLG_AFFECT | LNKFLG_ROOM);
  define_link(LNK_PET,              "PET",                charm_broken,     LNKFLG_AFFECT | LNKFLG_EXCLUSIVE);
  define_link(LNK_ESSENCE_OF_WOLF,  "ESSENCE",            essence_broken,   LNKFLG_ROOM | LNKFLG_EXCLUSIVE);
  define_link(LNK_BLOOD_ALLIANCE,   "BLOOD_ALLIANCE",     NULL,             LNKFLG_EXCLUSIVE);
  define_link(LNK_ETHEREAL,         "ALLIED",             NULL,             LNKFLG_EXCLUSIVE);
  define_link(LNK_CAST_ROOM,        "CAST_ROOM",          casting_broken,   LNKFLG_ROOM);
  define_link(LNK_CAST_WORLD,       "CAST_WORLD",         casting_broken,   LNKFLG_NONE);
  define_link(LNK_PALADIN_AURA,     "PALADIN_AURA",       aura_broken,      LNKFLG_AFFECT | LNKFLG_ROOM);
  define_link(LNK_GRAPPLED,         "GRAPPLED",           NULL,             LNKFLG_ROOM);
  define_link(LNK_CIRCLING,         "CIRCLING",           NULL,             LNKFLG_ROOM | LNKFLG_EXCLUSIVE);
  define_link(LNK_TETHER,           "TETHERING",          tether_broken,    LNKFLG_ROOM);
  define_link(LNK_SNG_HEALING,      "SONG_HEALING",       song_broken,      LNKFLG_AFFECT | LNKFLG_ROOM);

  define_olink(LNK_CEGILUNE,        "CEGILUNES_SEARING",  cegilunes_broken, LNKFLG_EXCLUSIVE | LNKFLG_REMOVE_AFF | LNKFLG_BREAK_REMOVE );
  define_olink(LNK_ILESH,           "ILESHS_SMASHING",    ileshs_broken,    LNKFLG_EXCLUSIVE | LNKFLG_REMOVE_AFF | LNKFLG_BREAK_REMOVE );
  define_olink(LNK_CHAR_OBJ_AFF,    "CHAR_OBJ_AFFECT",    NULL,             LNKFLG_REMOVE_AFF | LNKFLG_BREAK_REMOVE | LNKFLG_SHOW_REMOVE_MSG );
}

//---------------------------------------------------------------------------------
struct char_link_data *link_char_with_affect(P_char ch, P_char target, ush_int type, struct affected_type *af)
{
  struct char_link_data *cld;

  // attempt to create an undefined link - have a look at initialize_links
  if( !link_types[type].name )
    raise(SIGSEGV);

  if (link_types[type].flags & LNKFLG_EXCLUSIVE)
    clear_links(ch, type);

  if( !dead_link_pool )
  {
    dead_link_pool =
      mm_create("LINKS", sizeof(struct char_link_data), offsetof(struct char_link_data, next_linking), 100);
  }

  cld = (struct char_link_data *) mm_get(dead_link_pool);
  cld->affect = af;
  cld->linking = ch;
  cld->linked = target;
  cld->type = type;
  cld->next_linking = ch->linking;
  cld->next_linked = target->linked;
  ch->linking = target->linked = cld;

  return cld;
}

struct char_obj_link_data *link_char_obj_with_affect(P_char ch, P_obj obj, ush_int type, struct affected_type *af)
{
  struct char_obj_link_data *cold;

  // attempt to create an undefined link - have a look at initialize_links
  if( (type > LNK_MAX) || (type < 0) )
    raise(SIGSEGV);

  if( link_types[type].flags & LNKFLG_EXCLUSIVE )
    clear_links(ch, type);

  if( !dead_obj_link_pool )
  {
    dead_obj_link_pool =
      mm_create("CHOBJLINKS", sizeof(struct char_obj_link_data), offsetof(struct char_obj_link_data, next), 100);
  }

  cold = (struct char_obj_link_data *) mm_get(dead_obj_link_pool);
  cold->affect = af;
  cold->ch = ch;
  cold->obj = obj;
  cold->type = type;
  cold->next = ch->obj_linked;
  ch->obj_linked = cold;

  return cold;
}

//---------------------------------------------------------------------------------
struct char_link_data *link_char(P_char ch, P_char target, ush_int type)
{
  return link_char_with_affect(ch, target, type, 0);
}

//---------------------------------------------------------------------------------
P_char get_linked_char(P_char ch, ush_int type)
{
  struct char_link_data *cld;

  for (cld = ch->linking; cld; cld = cld->next_linking)
    if (cld->type == type)
      return cld->linked;

  return NULL;
}

//---------------------------------------------------------------------------------
P_char get_linking_char(P_char ch, ush_int type)
{
  struct char_link_data *cld;

  for (cld = ch->linked; cld; cld = cld->next_linked)
    if (cld->type == type)
      return cld->linking;

  return NULL;
}

//---------------------------------------------------------------------------------
bool is_linked_to(P_char target, P_char ch, ush_int type)
{
  struct char_link_data *cld;

  for (cld = ch->linking; cld; cld = cld->next_linking)
  {
    if (cld->type == type && cld->linked == target)
      return TRUE;
  }

  return FALSE;
}

bool is_linked_to(P_char ch, P_obj obj, ush_int type)
{
  struct char_obj_link_data *cold;

  for( cold = ch->obj_linked; cold; cold = cold->next )
  {
    if( (cold->type == type) && (cold->obj == obj) )
      return TRUE;
  }

  return FALSE;
}

//---------------------------------------------------------------------------------
int is_linked_from(P_char target, P_char ch, ush_int type)
{
  struct char_link_data *cld;

  for (cld = target->linking; cld; cld = cld->next_linking)
  {
    if (cld->type == type && cld->linked == ch)
      return TRUE;
  }

  return FALSE;
}

//---------------------------------------------------------------------------------
void dispose_link(struct char_link_data *cld)
{
  struct affected_type *af;

  if (link_types[cld->type].flags & LNKFLG_AFFECT)
  {
    for( af = cld->linking->affected; af && af != cld->affect; af = af->next )
      ;
    if( af )
      affect_remove(cld->linking, af);
  }
  mm_release(dead_link_pool, cld);
}

/*
 * called when ch moves from old_room to new_room
 */
//---------------------------------------------------------------------------------
void check_room_links(P_char ch, int old_room, int new_room)
{
  struct char_link_data *cld, *next_cld;

  for (cld = ch->linking; cld; cld = next_cld)
  {
    next_cld = cld->next_linking;
    if (link_types[cld->type].flags & LNKFLG_ROOM)
      if (cld->linked->in_room != old_room &&
          cld->linked->in_room != new_room)
        unlink_char(ch, cld->linked, cld->type);
  }

  for (cld = ch->linked; cld; cld = next_cld)
  {
    next_cld = cld->next_linked;
    if (link_types[cld->type].flags & LNKFLG_ROOM)
      if (cld->linking->in_room != old_room &&
          cld->linking->in_room != new_room)
        unlink_char(cld->linking, ch, cld->type);
  }
}

//---------------------------------------------------------------------------------
void clear_all_links(P_char ch)
{
  struct char_link_data *cld;
  P_char rider;

  // If there's someone riding ch, they fall off/down.
  if( (rider = GET_RIDER(ch)) != NULL )
  {
    send_to_char( "You fall on your ass!\n\r", rider );
    SET_POS(rider, POS_SITTING + GET_STAT(rider));
  }

  while( cld = ch->linking )
    unlink_char(ch, cld->linked, cld->type);

  while( cld = ch->linked )
    unlink_char(cld->linking, ch, cld->type);
}

//---------------------------------------------------------------------------------
void clear_links(P_char ch, ush_int type)
{
  struct char_link_data *cld;
  P_char   tch;

  while( tch = get_linked_char(ch, type) )
    unlink_char(ch, tch, type);
}

// Clears all links with flag set.
void clear_links( P_char ch, P_obj obj, int flag )
{
  struct char_obj_link_data *cold, *cold_prev, *cold_next;
  P_obj tobj;

  // Remove all those at the head of the list.
  while( (( cold = ch->obj_linked ) != NULL) && (cold->obj == obj)
    && IS_SET(link_types[cold->type].flags, flag) )
  {
    if( IS_SET( link_types[cold->type].flags, LNKFLG_OBJECT) && link_types[cold->type].break_func.obj )
      link_types[cold->type].break_func.obj(cold);
    if( IS_SET(link_types[cold->type].flags, LNKFLG_REMOVE_AFF) && (cold->affect != NULL) )
    {
      // We remove this bit so we don't go in circles removing the link which removes the affect
      //   which removes the link...
      REMOVE_BIT( cold->affect->flags, AFFTYPE_LINKED_OBJ );
      if( IS_SET(link_types[cold->type].flags, LNKFLG_SHOW_REMOVE_MSG) )
        wear_off_message(ch, cold->affect);
      affect_remove( ch, cold->affect );
    }
    ch->obj_linked = cold->next;
    cold->next = NULL;
    mm_release(dead_obj_link_pool, cold);
  }

  // Remove all those after the head of the list.
  //   Set previous to the head (which we know from the above loop is not a hit).
  if( (cold_prev = ch->obj_linked) != NULL )
  {
    // First look at the second item in the list..
    // If it exists...
    for( cold = cold_prev->next; cold != NULL; cold = cold_next )
    {
      // Set cold_next to the second item after cold_prev in the list (possibly NULL).
      cold_next = cold->next;

      // If we have a hit,
      if( cold->obj == obj )
      {
        if( IS_SET(link_types[cold->type].flags, LNKFLG_REMOVE_AFF) )
        {
          REMOVE_BIT( cold->affect->flags, AFFTYPE_LINKED_OBJ );
          affect_remove( ch, cold->affect );
        }
        if( IS_SET( link_types[cold->type].flags, LNKFLG_OBJECT) && link_types[cold->type].break_func.obj )
          link_types[cold->type].break_func.obj(cold);
        if( IS_SET(link_types[cold->type].flags, LNKFLG_SHOW_REMOVE_MSG) )
          wear_off_message(ch, cold->affect);

        // Remove cold from the list.
        cold_prev->next = cold_next;
        // And free memory.
        cold->next = NULL;
        mm_release(dead_obj_link_pool, cold);
      }
      // If we don't have a hit, move cold_prev down the list one.
      else
      {
        cold_prev = cold;
      }
    }
  }
}

//---------------------------------------------------------------------------------
void linked_affect_to_char(P_char ch, struct affected_type *af, P_char source, int type)
{
  af->flags |= AFFTYPE_LINKED_CH | AFFTYPE_NOSAVE;
  if (af->duration == 0)
    af->duration = 1;
  link_char_with_affect(ch, source, type, affect_to_char(ch, af));
}

void linked_affect_to_char_obj(P_char ch, struct affected_type *af, P_obj obj, int type)
{
  af->flags |= AFFTYPE_LINKED_OBJ | AFFTYPE_NOSAVE;

  if( af->duration == 0 )
    af->duration = 1;
  link_char_obj_with_affect(ch, obj, type, affect_to_char(ch, af) );

}

//---------------------------------------------------------------------------------
void internal_unlink_char(P_char ch, struct char_link_data *cld, struct char_link_data *prev)
{
  struct char_link_data *cld2;

  if (prev)
    prev->next_linking = cld->next_linking;
  else
    ch->linking = cld->next_linking;
  for (cld2 = cld->linked->linked, prev = NULL; cld2;
       prev = cld2, cld2 = cld2->next_linked)
  {
    if (cld2 == cld)
      break;
  }
  if (!cld2)                    // link must be found in the linked list of the target as well
    raise(SIGSEGV);
  else if (prev)
    prev->next_linked = cld->next_linked;
  else
    cld->linked->linked = cld->next_linked;
  if (cld->affect)
    wear_off_message(cld->linking, cld->affect);
  if( !IS_SET( link_types[cld->type].flags, LNKFLG_OBJECT) && link_types[cld->type].break_func.ch )
    link_types[cld->type].break_func.ch(cld);
  dispose_link(cld);
}

//---------------------------------------------------------------------------------
void unlink_char(P_char ch, P_char target, ush_int type)
{
  struct char_link_data *cld, *prev = NULL;

  for (cld = ch->linking; cld; prev = cld, cld = cld->next_linking)
  {
    if (cld->type == type && cld->linked == target)
      break;
  }

  if (cld)
    internal_unlink_char(ch, cld, prev);
}

//---------------------------------------------------------------------------------
void unlink_char_affect(P_char ch, struct affected_type *af)
{
  struct char_link_data *cld, *prev = NULL;

  for (cld = ch->linking; cld; prev = cld, cld = cld->next_linking)
  {
    if (af == cld->affect)
      break;
  }

  if (cld)
    internal_unlink_char(ch, cld, prev);
}

void unlink_char_obj_affect(P_char ch, struct affected_type *af)
{
  struct char_obj_link_data *cold, *prev = NULL;

  for( cold = ch->obj_linked; cold; prev = cold, cold = cold->next )
  {
    if( af == cold->affect )
      break;
  }

  if( cold )
  {
    // If not at the head..
    if( prev )
    {
      // Remove from the middle.
      prev->next = cold->next;
    }
    else
    {
      // Remove from the head.
      ch->obj_linked = cold->next;
    }
    cold->next = NULL;
    mm_release(dead_obj_link_pool, cold);
  }
}

//---------------------------------------------------------------------------------
void remove_link(P_char ch, struct char_link_data *clda)
{
  struct char_link_data *cld, *prev = NULL;

  for (cld = ch->linking; cld; prev = cld, cld = cld->next_linking)
  {
    if (cld == clda)
      break;
  }

  if (cld)
    internal_unlink_char(ch, cld, prev);
}

//=================================================================================
//=== AFFECTS - SOME SPEC ACTIONS
//=== make_dry, make_wet, poo, camp, falling_char, falling_obj,
//=== blind, stun, knockout
//=== affect_update, short_affect_update
//=================================================================================

//---------------------------------------------------------------------------------
void update_damage_data()
{
  char     buf[128];
  float    melee_factor, multiplier, pulse;
  int      i;

  melee_factor = get_property("damage.meleeFactor", 1.04);

  for (i = 0; i <= LAST_RACE; i++)
  {
    snprintf(buf, 128, "damage.pulse.racial.%s", race_names_table[i].no_spaces);
    combat_by_race[i][0] = pulse = get_property(buf, (float) PULSE_VIOLENCE);
/* Disabling the totalOutput for the 2015-16 wipe.
 * This has the effect of setting everyone's totaloutput to 1.0, so less hidden variables.
 * It might have a strong mob buff (for weak) and nerf (for strong) for the various mob races.
 *   However, it'll be more like the zone writers intended etc.
 *   We can just up the damage where it should be; in the zone files / mob load.
    snprintf(buf, 128, "damage.totalOutput.racial.%s", race_names_table[i].no_spaces);
    multiplier = get_property(buf, 1.0);
    // Cancelling the rest of this crap. It doesn't work right anyway. - Lohrr
    // get_property(buf, 1.0) * melee_factor * pulse / PULSE_VIOLENCE;
    combat_by_race[i][1] = multiplier;
 */
    combat_by_race[i][1] = 1;
    snprintf(buf, 128, "damage.damrollModifier.racial.%s", race_names_table[i].no_spaces);
    multiplier = get_property(buf, 1.0);
    combat_by_race[i][2] = multiplier;
  }

  for (i = 0; i <= CLASS_COUNT; i++)
  {
    snprintf(buf, 128, "damage.totalOutput.class.%s", class_names_table[i].normal);
    combat_by_class[i][1] = get_property(buf, 0.0);
    snprintf(buf, 128, "damage.pulse.class.%s", class_names_table[i].normal);
    combat_by_class[i][0] = get_property(buf, 0.0);
    snprintf(buf, 128, "hitpoints.class.%s", class_names_table[i].normal);
    class_hitpoints[i] = get_property(buf, 1.0);
  }

  combat_by_class[0][1] = 0;
  combat_by_class[0][0] = 0;
  combat_by_race[0][1] = 0;
  combat_by_race[0][0] = 0;

  // This pulse-mod is added to the total pulses / round of each character in game.
  pulse_all = get_property("damage.pulse.class.all", 1.000);
  shield_combat_mult= get_property("skill.shieldCombat.ACBonusMultiplier", 0.500);
  shield_combat_tank_mult= get_property("skill.shieldCombat.ACTankMultiplier", 1.500);
}

//---------------------------------------------------------------------------------
// This function returns TRUE if the victim is blinded, and FALSE otherwise.
//   The victim is not blinded if it's !blind.
bool blind(P_char ch, P_char victim, int duration)
{
  struct affected_type af;

  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return FALSE;
  }
  if( IS_SET(victim->specials.affected_by5, AFF5_NOBLIND) )
  {
    return FALSE;
  }

  if( IS_AFFECTED(victim, AFF_BLIND) )
  {
    act("&+L$N &+Lis already blind as a bat!", TRUE, ch, 0, victim, TO_CHAR);
    send_to_char("Your eyes hurt briefly, but the feeling dissipates.\r\n", victim);
    return FALSE;
  }
  // Parasites and slime are immune to blindness. Nov08 -Lucrot
  if( GET_RACE(victim) == RACE_PARASITE || GET_RACE(victim) == RACE_SLIME )
  {
    return FALSE;
  }

  if( !has_innate(victim, INNATE_EYELESS) && !isname("_noblind_", GET_NAME(victim)) && !IS_TRUSTED(victim) )
  {
    act("&+L$n &+Lseems to be blinded!", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&+LYou have been blinded!\r\n", victim);
    memset(&af, 0, sizeof(struct affected_type));
    // if (!IS_AFFECTED(victim, AFF_BLIND) && duration > 5 * PULSE_VIOLENCE)
      // gain_exp(ch, victim, 0, EXP_DAMAGE);
    af.type = SPELL_BLINDNESS;
    af.flags = AFFTYPE_SHORT;
    af.bitvector = AFF_BLIND;
    af.duration = duration;
    affect_to_char(victim, &af);
    return TRUE;
  }

  return FALSE;
}

//---------------------------------------------------------------------------------
void Stun(P_char stunnee, P_char stunner, int duration, bool Fear_Check)
{
  struct affected_type af;
  int attlevel = GET_LEVEL(stunner), deflevel = GET_LEVEL(stunnee);

  if(!IS_ALIVE(stunnee))
  {
    return;
  }
  
  // Elite mobs are !stun. Oct08 -Lucrot
  if(IS_ELITE(stunnee))
  {
    return;
  }
  
  // Greater races are harder to stun based on their level. Level 60+ greater races
  // cannot be stunned when this function is called. Oct08 -Lucrot
  if(IS_GREATER_RACE(stunnee) || 
     GET_RACE(stunnee) == RACE_PLANT || 
     GET_RACE(stunnee) == RACE_GOLEM || 
     GET_RACE(stunnee) == RACE_CONSTRUCT)
  {
    if(!number(0, (int) BOUNDED(0, (60 - deflevel), 59)))
    {
      return;
    }
  }

  if(IS_AFFECTED2(stunnee, AFF2_STUNNED))
  {
    send_to_char("&+wIf you could get more stunned you would.\r\n", stunnee);
    return;
  }

  // SAVING_FEAR can now protect against stun, if there isn't a save already applied by calling function - Jexni 2/18/11
  // You have to save twice against stun effect, as stun is fairly nasty, 2nd is half duration
  // Fear_Check is passed by calling function to determine if stunnee gets to save w/ fear
  // Power word stun and similar spells have their own save breakdown and pass FALSE to this function

  int chance = BOUNDED(-25, attlevel - deflevel, 25);
  
  if(Fear_Check)
  {
    if(!NewSaves(stunnee, SAVING_FEAR, chance))
    {
      memset(&af, 0, sizeof(af));
      af.type = SPELL_PWORD_STUN;
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
      af.bitvector2 = AFF2_STUNNED;
      af.duration = duration;
      affect_to_char(stunnee, &af);  

      send_to_char("&+wThe world starts spinning, and your ears are ringing!\r\n", stunnee);
      act("$n&n is &+Wstunned!&n", TRUE, stunnee, 0, 0, TO_ROOM);
      if(IS_FIGHTING(stunnee))
        stop_fighting(stunnee);
      if(IS_DESTROYING(stunnee))
        stop_destroying(stunnee);
    }
    else if(!NewSaves(stunnee, SAVING_FEAR, chance + number(0, 3)))
    {
      memset(&af, 0, sizeof(af));
      af.type = SPELL_PWORD_STUN;
      af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
      af.bitvector2 = AFF2_STUNNED;
      af.duration = duration / 2;
      affect_to_char(stunnee, &af);

      send_to_char("&+wWow that &+Rsmarts... &+Wbut you manage to recover quickly!\r\n", stunnee);
      act("$n&n is momentarily &+Wdazed...&n", TRUE, stunnee, 0, 0, TO_ROOM);
      if(!number(0, 3) && IS_FIGHTING(stunnee))
      {
        stop_fighting(stunnee);
        if( IS_DESTROYING(stunnee) )
          stop_destroying(stunnee);
      }
    }
  }
  else
  {
    memset(&af, 0, sizeof(af));
    af.type = SPELL_PWORD_STUN;
    af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
    af.bitvector2 = AFF2_STUNNED;
    af.duration = duration;
    affect_to_char(stunnee, &af);

    send_to_char("&+wThe world starts spinning, and your ears are ringing!\r\n", stunnee);
    act("$n&n is &+Wstunned!&n", TRUE, stunnee, 0, 0, TO_ROOM);
    if(IS_FIGHTING(stunnee))
      stop_fighting(stunnee);
    if(IS_DESTROYING(stunnee))
      stop_destroying(stunnee);
  }
}

//---------------------------------------------------------------------------------
void KnockOut(P_char ch, int duration)
{
  struct affected_type af;

  if (IS_TRUSTED(ch))
  {
    send_to_char
      ("You briefly feel a slight pain in your head, but wait, it's gone!\r\n",
       ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_KNOCKED_OUT))
    return;

  send_to_char("That last blow was a bit too much, and you go down.\r\n", ch);
  act("$n slumps to the ground, unconscious.", TRUE, ch, 0, 0, TO_ROOM);
  SET_POS(ch, POS_PRONE + GET_STAT(ch));

  memset(&af, 0, sizeof(af));
  af.type = SKILL_HEADBUTT;
  af.bitvector = AFF_KNOCKED_OUT;
  af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOSHOW;
  af.duration = duration;

  affect_to_char_with_messages(ch, &af,
                               "Feeling begins to return... a major headache.",
                               "$n seems to have come back to his senses.");
}

//---------------------------------------------------------------------------------
void make_dry(P_char ch)
{
  struct affected_type *af, *naf;

  for (af = ch->affected; af; af = naf) {
    naf = af->next;
    if (IS_SET(af->bitvector5, AFF5_WET))
      affect_remove(ch, af);
  }

  REMOVE_BIT(ch->specials.affected_by5, AFF5_WET);
}

//---------------------------------------------------------------------------------
bool make_wet(P_char ch, int duration)
{
  struct affected_type af;

  if (IS_AFFECTED2(ch, AFF2_FIRESHIELD)) {
    send_to_char("Water hisses loudly as it evaporates on your fireshield.\n", ch);
    act("Water hisses loudly as it comes in contact with $n's fireshield.\n", FALSE, ch, 0, 0, TO_ROOM);
    return false;
  }

  if (!IS_AFFECTED5(ch, AFF5_WET)) {
    memset(&af, 0, sizeof(af));
    af.type = TAG_WET;
    af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
    af.bitvector5 = AFF5_WET;
    af.duration = duration;
    affect_to_char(ch, &af);
    return true;
  } else
    return false;
}

//---------------------------------------------------------------------------------
void poo(P_char ch)
{
  P_obj load;

  if( IS_PC(ch) && (IS_CENTAUR( ch ) || IS_MINOTAUR( ch ) || IS_GOBLIN( ch ))
    && (number( 0, 1000 ) == 42) && (load = read_object( 51, VIRTUAL )) )
  {
    if (IS_CENTAUR(ch))
    {
      load->str_mask = (STRUNG_DESC1 | STRUNG_DESC2);

      load->description =
        str_dup("&+yA steaming pile of horse droppings is here.&n");
      load->short_description =
        str_dup("&+ya steaming pile of horse droppings&n");

      if (ch->in_room == NOWHERE)
      {
        if (real_room(ch->specials.was_in_room) != NOWHERE)
          obj_to_room(load, real_room(ch->specials.was_in_room));
        else
        {
          extract_obj(load);
          load = NULL;
        }
      }
      else
      {
        obj_to_room(load, ch->in_room);
        send_to_char("&+yYou can't resist the urge anymore - a load drops from between your rear legs.\n", ch);
        act("&+yA load of dung suddenly drops from between $n&n&+y's rear legs, landing with a soft plop.",
          TRUE, ch, 0, 0, TO_ROOM);
      }
    }
    else if (IS_MINOTAUR(ch))
    {
      load->str_mask = (STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_KEYS);

      load->name = str_dup("dung cattle pile");
      load->description =
        str_dup("&+yA steaming pile of bullshit is here.&n");
      load->short_description = str_dup("&+ya steaming pile of bullshit&n");

      if (ch->in_room == NOWHERE)
      {
        if (real_room(ch->specials.was_in_room) != NOWHERE)
          obj_to_room(load, real_room(ch->specials.was_in_room));
        else
        {
          extract_obj(load);
          load = NULL;
        }
      }
      else
      {
        obj_to_room(load, ch->in_room);
        send_to_char("&+yYou can't resist the urge anymore - a load drops from between your legs.\n", ch);
        act("&+yA load of dung suddenly drops from between $n&n&+y's legs, landing with a soft plop.",
          TRUE, ch, 0, 0, TO_ROOM);
      }
    }
    else if (IS_GOBLIN(ch) && !number(0, 29))
    {
      load->str_mask = (STRUNG_DESC1 | STRUNG_DESC2);

      load->description = str_dup("&+ySome dry feces has been left here.&n");
      load->short_description = str_dup("&+ya dry goblin poo-poo&n");

      if (ch->in_room == NOWHERE)
      {
        if (real_room(ch->specials.was_in_room) != NOWHERE)
          obj_to_room(load, real_room(ch->specials.was_in_room));
        else
        {
          extract_obj(load);
          load = NULL;
        }
      }
      else
      {
        obj_to_room(load, ch->in_room);
        send_to_char("&+yMmmm, much better. Don't forget to clean it up now.\n", ch);
        act("&+y$n screws $s face in concentration, and some small tiny feces drop from between his legs.",
           TRUE, ch, 0, 0, TO_ROOM);
      }
    }
  }
}

//---------------------------------------------------------------------------------
int camp(P_char ch)
{
  struct affected_type *af;

  if (IS_PC(ch) && IS_AFFECTED(ch, AFF_CAMPING))
  {
    for (af = ch->affected; af && (af->type != TAG_CAMP); af = af->next) ;

    if (af)
    {
      if (!ch->desc ||
          IS_FIGHTING(ch) ||
          IS_DESTROYING(ch) ||
          (ch->in_room != af->modifier) ||
          (GET_STAT(ch) < STAT_SLEEPING) ||
          IS_SET(ch->specials.affected_by, AFF_HIDE) ||
          IS_IMMOBILE(ch))
      {
        affect_from_char(ch, TAG_CAMP);
        send_to_char("So much for that camping effort.\n", ch);
      }
      else
      {
        /*
         we don't want it wearing off during regular updates, so we add a
         little extra, and close it out before regular update code can kill

         it.
         */
        if (--(af->duration) < 2)
        {
          /* done the time, now we rent em out.  */
          if (IS_FIGHTING(ch))
            stop_fighting(ch);
          if( IS_DESTROYING(ch) )
            stop_destroying(ch);
          affect_from_char(ch, TAG_CAMP);
          if (!IS_RACEWAR_UNDEAD(ch))
          {
            act("$n rolls $mself up in $s bedroll and tunes out the world.",
                FALSE, ch, 0, 0, TO_ROOM);
            send_to_char("You hunker down for some serious 'roughing it'.\n",
                         ch);
          }
          else
          {
            act("$n climbs into $s shallow grave and covers $mself up.",
                FALSE, ch, 0, 0, TO_ROOM);
            send_to_char
              ("You climb into your little grave and bury yourself.\n", ch);
          }
          if( IS_SHIP_ROOM(ch->in_room) )
          {
            GET_HOME(ch) = GET_BIRTHPLACE(ch);
          }
          else
          {
            GET_HOME(ch) = world[ch->in_room].number;
          }
          writeCharacter(ch, RENT_CAMPED, ch->in_room);

          loginlog(ch->player.level, "%s has camped in [%d].",
                   GET_NAME(ch), world[ch->in_room].number);
          sql_log(ch, CONNECTLOG, "Camped");
          // If it's not an immortal.
          if( GET_LEVEL(ch) < MINLVLIMMORTAL )
          {
            update_ingame_racewar( -GET_RACEWAR(ch) );
          }
          extract_char(ch);

          /*
           ok, to make the new nanny work correctly, we need to assign
           their in_room to where they just camped, so if they come right
           back in from the menu, they come in where they are supposed to.
           -JAB
           */
          ch->in_room = real_room(GET_HOME(ch));
          return 1;
        }
      }
    }
    else
    {
      logit(LOG_DEBUG, "%s has AFF_CAMP, but no affect structure",
            GET_NAME(ch));
      send_to_char
        ("hmm, something strange has happened to your camp attempt, better petition for a god.\n",
         ch);
      REMOVE_BIT(ch->specials.affected_by, AFF_CAMPING);
    }
  }

  return 0;
}

void event_falling_char(P_char ch, P_char victim, P_obj obj, void *data)
{
  falling_char(ch, *((int*)data), TRUE);
}

/*
 ch CHAR_FALLING, this routine updates status, moves them, sends appropriate
 messages, kills them if neccessary, and handles event updates.  returns
 TRUE if they are actually falling, FALSE otherwise.

 assumption 15' per room fallen:
 first check: they fall 1 room in 1 second (4 pulses)       31mph
 second check: they fall 1 room in 1/2 second (2 pulses)    43mph
 extra checks: they fall one room per pulse (1/4 second)    +8mph

 speed piles up (to 250 mph ('terminal' velocity)) since this is no longer
 a loop, they can fall 'forever' if someone sets up a 'bottomless' pit.
 This isn't completely realistic but it will suffice, they'll hit 250mph
 in 32 pulses (~8 seconds) after falling 33 rooms (about 500'). If/when
 they hit bottom, they take:  (dice((speed - number(27, 30)), 9) - dex)
 damage so falling just one room will probably not do much damage
 (4d9-dex max), but falling a long ways can be decidedly fatal.

 Note that since this is event driven, if they are summoned out, they are
 still falling that fast, and they'll take that damage.  Since this isn't
 instantaneous they can (theoretically) save themselves by starting to fly
 or levitate (we can now add feather fall that kicks in if they exceed 20
 mph (they'd still be falling though *grin*)
*/

/* New formula; All damage taken is a percentage of their max, so a level
1 player has just as much chance of survival as a level 50.
max hit * (speed/2.5) + (90-120) - agil... So, one room fall, shitty char
stats, would be 12% of their max hit, plus about 30 points, depending on
agi. Should they reach terminal velocity, 33+ rooms, they will take 100%
damage, + the 30 or so points. Having safe_fall skill kick in, should
leave them living, but on the brink of death. */

bool falling_char(P_char ch, const int kill_char, bool caller_is_event)
{
  P_nevent ev;
  P_char   chr;
  int      dam = 0, speed = 0, i, new_room, save_act, had_zcord = FALSE;

  if (!ch)
  {
    logit(LOG_EXIT, "falling_char with NULL char.");
    raise(SIGSEGV);
  }

  if( ch->in_room == NOWHERE )
    return FALSE;

  if (!caller_is_event)
  {
    /* if not already falling  */
    LOOP_EVENTS_CH(ev, ch->nevents)
    {
      if( ev->func == event_falling_char )
      {
        return FALSE;
      }
    }

    /* If there is a ground, assume we are here due to fall_chance. Now, if
    they can't go, its prolly due to the down exit being closed/blocked.
    Even if it is really a bug, don't bother spamming messages.
    Using virtual can go, as it checks more door states */
    if ((world[ch->in_room].sector_type != SECT_NO_GROUND) &&
        (world[ch->in_room].sector_type != SECT_UNDRWLD_NOGROUND) &&
        !VIRTUAL_CAN_GO(ch->in_room, DIR_DOWN) && 
        (ch->specials.z_cord == 0))
      return FALSE;
    /* This isn't for sinking */
    if (ch->specials.z_cord < 0)
      return FALSE;

    /* Don't continue if a breakable wall is acting as a floor - Jexni 12/07/10 */    
    if (world[ch->in_room].dir_option[DIR_DOWN])
      if(IS_SET(world[ch->in_room].dir_option[DIR_DOWN]->exit_info, EX_BREAKABLE))
        return FALSE;

    /* ch has just stepped into space, initiate the plunge  */
    if (IS_TRUSTED(ch) || IS_AFFECTED(ch, AFF_LEVITATE) || IS_AFFECTED(ch, AFF_FLY))
    {
      send_to_char
      ("You grin as you realize you're floating, with no floor in the room.\n",
       ch);
      return FALSE;
    }
    else if(IS_RIDING(ch))
    {
      if(IS_AFFECTED(GET_MOUNT(ch), AFF_LEVITATE) || IS_AFFECTED(GET_MOUNT(ch), AFF_FLY))
      {
        send_to_char("You grin as you realize you're floating upon a flying mount.", ch);
        return FALSE;
      }
    }
    // If they have climb, they get a max 50% chance not to start falling.
    if( affected_by_spell(ch, SKILL_CLIMB) && number( 1, 100 ) > GET_CHAR_SKILL(ch, SKILL_CLIMB) / 2 )
    {
      send_to_char( "You start to slip, but catch yourself.\n", ch );
      return FALSE;
    }
    act("$n has just realized $e has no visible means of support!", TRUE, ch, 0, 0, TO_ROOM);
    if (GET_STAT(ch) > STAT_SLEEPING)
      send_to_char("You rediscover the law of gravity...\n...the hard way!\n", ch);
    else if (GET_STAT(ch) > STAT_INCAP)
      send_to_char("You get a sinking feeling.\n", ch);
    else
      send_to_char("Just when it seemed things couldn't get any worse...\n", ch);

    if (CAN_GO(ch, DIR_DOWN) || ch->specials.z_cord > 0)
    {
      /*  send them falling */
      speed = 1;
      add_event(event_falling_char, 0, ch, NULL, NULL, 0, &speed, sizeof(speed));
      return TRUE;
    }
    else
    {
      send_to_char("But wait!  Saved by a bug!\n", ch);
      act("$n is granted a reprieve, and breathes a prayer of thanks", FALSE,
          ch, 0, 0, TO_ROOM);
      logit(LOG_DEBUG, "Room (%d) Name: (%s) is NO_GROUND but has no valid 'down' exit",
            world[ch->in_room].number, GET_NAME(ch));
      world[ch->in_room].sector_type = SECT_INSIDE;
      return FALSE;
    }
  }

  /* get here, it's a normal event call  */
  speed = (int) kill_char;
  i = ch->in_room;

  if ((ch->specials.z_cord > 0) || 
      (world[i].dir_option[DIR_DOWN] &&
      !IS_SET(world[i].dir_option[DIR_DOWN]->exit_info, EX_CLOSED) &&
      !IS_SET(world[i].dir_option[DIR_DOWN]->exit_info, EX_BREAKABLE)))
  {
    if (ch->specials.z_cord > 0)
    {
      ch->specials.z_cord--;
      had_zcord = TRUE;
      new_room = ch->in_room;
    }
    else
      new_room = world[i].dir_option[DIR_DOWN]->to_room;

    if (speed < 45)
      act("$n drops from sight.", TRUE, ch, 0, 0, TO_ROOM);
    else if (speed < 90)
      act("Someone drops from sight.", TRUE, ch, 0, 0, TO_ROOM);
    else
      act("A large (screaming) object drops from sight!", TRUE, ch, 0, 0,
          TO_ROOM);

    char_from_room(ch);
    char_to_room(ch, new_room, -2);

    /* change falling speed  */
    if (IS_AFFECTED(ch, AFF_LEVITATE) || IS_AFFECTED(ch, AFF_FLY))
    {
      /* 1 gravity decel for lev, 1/2 grav for fly  */
      speed -= (IS_AFFECTED(ch, AFF_FLY)) ? 4 : 8;
      if (speed <= 0)
      {
        speed = 0;
        send_to_char("You slow to a stop.  WHEW!\n", ch);
        act("$n floats in from above.", TRUE, ch, 0, 0, TO_ROOM);
        do_look(ch, 0, -2);
        return FALSE;
      }
      else
        send_to_char("You slow down a little.\n", ch);
    }
    else
    {
      if (speed == 1)
        speed = 31;             /* first room  */
      else if (speed == 31)
        speed = 43;             /* second room  */
      else
        speed += 8;             /* additional rooms  */

      if (speed > 250)
        speed = 250;
      else if (speed < 31)
        speed = 31;
    }
  }
  else
    new_room = i;               /*
                                   this happens when faller is
                                   summoned/transed/teleported out
                                */

  if (!world[new_room].dir_option[DIR_DOWN] ||
      (world[new_room].dir_option[DIR_DOWN]->to_room == NOWHERE) ||
      IS_SET(world[new_room].dir_option[DIR_DOWN]->exit_info, EX_CLOSED) || 
      IS_SET(world[new_room].dir_option[DIR_DOWN]->exit_info, EX_BREAKABLE) ||
      ((ch->specials.z_cord == 0) && had_zcord))
  {

    /* oh dear, we seem to have run out of falling room!  Muhahaha  */

    dam = (int) (GET_MAX_HIT(ch) * ((speed / 2.5) / 100)) + (number(80, 120) - GET_C_AGI(ch));

    if (dam < 2)
      dam = 2;

    if (GET_CHAR_SKILL(ch, SKILL_SAFE_FALL))
      if (GET_CHAR_SKILL(ch, SKILL_SAFE_FALL) > number(1, 101))
        dam <<= 1;

    if (world[ch->in_room].dir_option[DIR_DOWN])
    { 
       if (IS_SET(world[ch->in_room].dir_option[DIR_DOWN]->exit_info, EX_BREAKABLE))
       {
          P_obj Wall;
          for(Wall = world[ch->in_room].contents; Wall; Wall = Wall->next_content)
          {
             if(Wall->R_num == real_object(VOBJ_WALLS))
               if(Wall->value[1] == 5)
                 break;
          }
          if(Wall && (speed > 43 || (Wall->value[2] / 2 < 10)))
          {
             act("You slam into $p, shattering it upon impact, and only marginally slowing your fall...", FALSE, ch, Wall, 0, TO_CHAR);
             act("$n falls from above, slamming into and shattering $p, before continuing to fall...", FALSE, ch, Wall, 0, TO_ROOM);
             damage(ch, ch, dam, TYPE_UNDEFINED);
             spell_dispel_magic(70, ch, NULL, SPELL_TYPE_SPELL, 0, Wall);
             speed /= 2;
             add_event(event_falling_char, 0, ch, NULL, NULL, 0, &speed, sizeof(speed));
             return FALSE;
          }
          else if(Wall)
          {
             Wall->value[2] /= 2;
          }
       }
    }

    if (dam <= 0)
    {
      send_to_char("You land deftly on your feet, nice jump!\n", ch);
      notch_skill(ch, SKILL_SAFE_FALL, 33.33);
      do_look(ch, 0, -2);
      act("$n drops in from above, landing neatly.", TRUE, ch, 0, 0, TO_ROOM);
      return FALSE;
    }
    else if (IS_WATER_ROOM(ch->in_room))
    {
      send_to_char("With a splash, you plunge into the waters!\n", ch);
      /*  ch->specials.z_cord = -1;*/
      do_look(ch, 0, -2);
      act("$n drops in from above with a loud splash.", TRUE, ch, 0, 0,
          TO_ROOM);
      return FALSE;
    }
    else
    {
      send_to_char("You land with stunning force!\n", ch);
      act("$n falls in from above, landing in a crumpled heap!", TRUE, ch, 0, 0, TO_ROOM);

      if (ch->specials.z_cord > 0)
        ch->specials.z_cord = 0;

      // if ch has rider, don't kill em..  a new law of physics

      chr = get_linking_char(ch, LNK_RIDING);

      if( kill_char && !chr )
      {
        if (damage(ch, ch, dam, DAMAGE_FALLING))
          return FALSE;
      }
      else
      {
        GET_HIT(ch) = MAX(GET_HIT(ch) - dam, -8);
        update_pos(ch);
      }

      SET_POS(ch, number(0, 2) + GET_STAT(ch));
      Stun(ch, ch, (100 * dam / GET_MAX_HIT(ch)), FALSE);  /* 1-100  */
      /* also can knock them out for a time.  */
      if (number(1, (100 * dam / GET_MAX_HIT(ch))) >
          number(STAT_INDEX(GET_C_CON(ch)) / 2,
                 STAT_INDEX(GET_C_CON(ch)) * 3))
        KnockOut(ch, number(2, MAX(2, (100 - GET_C_CON(ch)))));

      // if this is a mount, hurt rider (theoretically, only mounts will fall)
      // make sure char and rider are in the same room, lest only mount fell..

      if (chr && (ch->in_room == chr->in_room))
      {
        send_to_char("You land with stunning force!\n", chr);
        send_to_char("Your rider suffers a similar fate!\n", ch);

        act
          ("$n, riding $N, is thrown off $s mount and slams into the ground!",
           TRUE, chr, 0, ch, TO_NOTVICT);

        if (chr->specials.z_cord > 0)
          chr->specials.z_cord = 0;

        if( kill_char )
        {
          if (damage(chr, chr, dam, DAMAGE_FALLING))
            return FALSE;
        }
        else
        {
          GET_HIT(chr) = MAX(GET_HIT(chr) - dam, -8);
          update_pos(chr);
        }

        SET_POS(chr, number(0, 2) + GET_STAT(chr));
        Stun(chr, chr, (100 * dam / GET_MAX_HIT(chr)), FALSE);

        if (number(1, (100 * dam / GET_MAX_HIT(chr))) >
            number(STAT_INDEX(GET_C_CON(chr)) / 2,
                   STAT_INDEX(GET_C_CON(chr)) * 3))
          KnockOut(chr, number(2, MAX(2, (100 - GET_C_CON(chr)))));

        // as you might imagine, the rider is no longer riding

        unlink_char(chr, ch, LNK_RIDING);
      }

      return FALSE;
    }
  }
  /*
   just passing through
   */
  if (speed < 45)
  {
    act("$n falls in from above.", TRUE, ch, 0, 0, TO_ROOM);
    do_look(ch, 0, -2);
  }
  else if (speed < 90)
  {
    act("Someone hurtles in from above.", TRUE, ch, 0, 0, TO_ROOM);
    if (IS_PC(ch))
    {
      save_act = ch->specials.act;
      SET_BIT(ch->specials.act, PLR_BRIEF);
      do_look(ch, 0, -2);
      ch->specials.act = save_act;
    }
  }
  else
  {
    act("A large (screaming) object plummets in from above!", TRUE, ch, 0, 0,
        TO_ROOM);
    send_to_char("You fall, shapes and sounds shredding past you.\n", ch);
  }

  add_event(event_falling_char, (speed == 31) ? 4 : (speed == 43) ? 2 : 1, ch, NULL, NULL, 0, &speed, sizeof(speed));
  //AddEvent(EVENT_FALLING_CHAR, (speed == 31) ? 4 : (speed == 43) ? 2 : 1, TRUE, ch, speed);
  return TRUE;
}

void event_falling_obj(P_char ch, P_char victim, P_obj obj, void *data)
{
  falling_obj(obj, *((int*)data), TRUE);
}

/*
 obj OBJ_FALLING, this routine updates speed, moves it, sends appropriate
 messages, destroys/damages objects as neccessary, and handles event updates.
 returns TRUE if object is falling, FALSE otherwise.

 assumption 15' per room fallen:
 first check:  falls 1 room in 1 second (4 pulses)     31mph
 second check: falls 1 room in 1/2 second (2 pulses)    43mph
 extra checks: falls one room per pulse (1/4 second)    +8mph

 speed piles up (to 250 mph ('terminal' velocity)) since this is no longer
 a loop, it can fall 'forever' if someone sets up a 'bottomless' pit.
 This isn't completely realistic but it will suffice, it'll hit 250mph
 in 32 pulses (~8 seconds) after falling 33 rooms (about 500'). If/when
 it hits bottom, it'll take some damage (most things), or be destroyed.

 Note that since this is event driven, if object is grabbed it will xfer
 energy to the grabber, which will be in the form of increased speed (if
 grabber is also falling) and/or damage.  Same holds true if object hits
 a character (I'm not going to include falling objects hitting other objects).
*/
//---------------------------------------------------------------------------------
bool falling_obj(P_obj obj, int speed, bool caller_is_event)
{
  static bool          already_falling = FALSE;
  P_nevent             ev;
  room_direction_data *exit;
  int                  dam, new_room;
  bool                 sect_check;

  if( !obj )
  {
    logit(LOG_EXIT, "falling_obj: NULL obj.");
    raise(SIGSEGV);
  }

  if( !OBJ_ROOM(obj) )
  {
    logit(LOG_DEBUG, "falling_obj: obj '%s' %d is NOT in a room.", OBJ_SHORT(obj), OBJ_VNUM(obj) );
    // Somebody snagged it while it was falling
    // May have to do the damage here, but more likely in get()
    return FALSE;
  }

  /* Not for underwater use, or noshow objects. */
  if( obj->z_cord < 0 || already_falling || IS_NOSHOW(obj) )
    return FALSE;

  if (IS_SET(obj->extra_flags, ITEM_LEVITATES))
    return FALSE;

  // Make sure that, if it's not suppoesd to be already falling, that it isn't.
  if( !caller_is_event )
  {
    // If not already falling (making sure)
    LOOP_EVENTS_OBJ(ev, obj->nevents)
    {
      if( ev->func == event_falling_obj )
      {
        return FALSE;
      }
    }
    speed = 1;
    // Passes falling criteria for the room it's in to start falling.
    if(  ( world[obj->loc.room].sector_type == SECT_NO_GROUND )
      || ( world[obj->loc.room].sector_type == SECT_UNDRWLD_NOGROUND )
      || ( world[obj->loc.room].chance_fall >= number(1, 100) ) )
    {
      sect_check = TRUE;
    }
    else
    {
      sect_check = FALSE;
    }
  }
  // It's already falling
  else
  {
    sect_check = TRUE;
  }

  /* Don't continue if a breakable wall is acting as a floor - Jexni 12/07/10 */
  // We want to 'hit the floor' here, so use all !fall conditions:
  //   no height to fall, and no exit to fall through, it doesn't lead anywhere, or it's blocked.
  // Nowhere to fall: not in air (z_cord) and not without somewhere to fall and it can go through the exit.
  // And don't forget to check if it actually starts falling.
  if( obj->z_cord == 0 && ( !(exit = world[obj->loc.room].dir_option[DIR_DOWN]) || exit->to_room == NOWHERE
    || IS_SET(exit->exit_info, EX_CLOSED | EX_LOCKED | EX_BLOCKED | EX_WALLED | EX_BREAKABLE) || !sect_check ) )
  {
    // Hit the ground!
    // Debugging:
    if( exit && exit->to_room == NOWHERE )
    {
      logit(LOG_DEBUG, "Room (%d) has falling object vnum (%d) but has no valid 'down' exit.",
        world[obj->loc.room].number, obj_index[obj->R_num].virtual_number);
      FREE( exit );
      world[obj->loc.room].dir_option[DIR_DOWN] = exit = NULL;
    }
    // At this point, we know that, if exit exists, it doesn't lead to NOWHERE.
    if( ( ( world[obj->loc.room].sector_type == SECT_NO_GROUND )
      || ( world[obj->loc.room].sector_type == SECT_UNDRWLD_NOGROUND ) ) && !exit )
    {
      logit(LOG_DEBUG, "Room (%d) has sector type NOGROUND but has no valid 'down' exit.",
        world[obj->loc.room].number, obj_index[obj->R_num].virtual_number);
      world[obj->loc.room].sector_type = SECT_INSIDE;
    }
    // If we didn't pass the room-sector criteria to start falling.
    if( !sect_check )
    {
      return FALSE;
    }
    // If we've just started falling.
    if( speed <= 31 )
    {
      act("$p quivers in space for a second, then settles to the ground.", TRUE, 0, obj, 0, TO_ROOM);
      return FALSE;
    }
    else
    {
      // We seem to have run out of falling room!
      // Speed is at least 31 so dam at least 2d9.
      dam = dice(speed - 29, 9);

      if( dam/10 < 1 )
      {
        act("$p wafts to the ground.", TRUE, 0, obj, 0, TO_ROOM);
        return FALSE;
      }
      else
      {
        act("$p crashes into the ground from above!", TRUE, 0, obj, 0, TO_ROOM);
        if( !IS_ARTIFACT(obj) )
          obj->condition -= dam/10;
        // Add object damage/destroy, also hit people, here
        return FALSE;
      }
    }
  }

  // At this point, we know obj->z_cord > 0 or there's a valid exit down.
  if( speed < 45 )
    act("$p drops from sight.", TRUE, 0, obj, 0, TO_ROOM);
  else
    act("Something drops from sight.", TRUE, 0, obj, 0, TO_ROOM);

  if( obj->z_cord > 0 )
  {
    obj->z_cord--;
  }
  else
  {
    new_room = exit->to_room;
    obj_from_room(obj);
    already_falling = TRUE;
    obj_to_room(obj, new_room);
    already_falling = FALSE;
  }

  // 1 second and speed, 31 mph after that -- Jexfix
  if( speed < 31 )
  {
    speed = 31;
    dam = 4;
  }
  // Second room
  else if( speed == 31 )
  {
    speed = 43;
    dam = 2;
  }
  // Additional rooms
  else
  {
    speed += 8;
    // Terminal velocity.
    if( speed > 250 )
      speed = 250;
    dam = 1;
  }

  if( speed < 45 )
    act("$p falls in from above.", TRUE, 0, obj, 0, TO_ROOM);
  else
    act("Something falls in from above.", TRUE, 0, obj, 0, TO_ROOM);

  add_event(event_falling_obj, dam, NULL, NULL, obj, 0, &speed, sizeof(speed));
  //AddEvent(EVENT_FALLING_OBJ, (speed == 31) ? 4 : (speed == 43) ? 2 : 1, TRUE, obj, speed);
  return TRUE;
}

//---------------------------------------------------------------------------------
void affect_update(void)
{
  struct affected_type *af, *next_af_dude;
  P_char   i, i_next, orig;
  int      morphed = FALSE;

  for (i = character_list; i != NULL; i = i_next)
  {
    i_next = i->next;

    if (IS_PC(i) && i->desc && i->desc->connected)
      continue;

    if (IS_NPC(i) || (racial_data[GET_RACE(i)].max_age > GET_AGE(i))
        || IS_TRUSTED(i))
    {
      if (GET_HIT(i) < GET_MAX_HIT(i))
        StartRegen(i, EVENT_HIT_REGEN);

      if (GET_VITALITY(i) < GET_MAX_VITALITY(i))
        StartRegen(i, EVENT_MOVE_REGEN);

      if (GET_CLASS(i, CLASS_PSIONICIST) || GET_CLASS(i, CLASS_MINDFLAYER))
        if (GET_MANA(i) < GET_MAX_MANA(i))
          StartRegen(i, EVENT_MANA_REGEN);
    }

    if (IS_DISGUISE(i))
    {
      if (i->disguise.hit < number(0, 50))
        remove_disguise(i, TRUE);
      else
        i->disguise.hit -= 5;
    }
    /* safety checks here... */
    if (i && i->affected)
    {
      for (af = i->affected; af != NULL; af = next_af_dude)
      {
        next_af_dude = af->next;

        if (af->flags & AFFTYPE_SHORT)
          ;                     /* short affects are removed by the associated events */
        else if (af->duration >= 1)
          af->duration--;
        else if (af->duration == -1)
          /* No action  */
          af->duration = -1;    /* GODs only! unlimited  */
        else
        {
          wear_off_message(i, af);

          /* force an unmorph - only morphs should get the af, but you never know */

          if (af->type == SPELL_CALL_OF_THE_WILD)
          {
            if (IS_MORPH(i))
            {
              orig = i->only.npc->orig_char;

              act("$n suddenly changes shape, reforming into $N.", FALSE, i,
                  NULL, orig, TO_NOTVICT);
              send_to_char
                ("You suddenly feel yourself going back to normal.\n", i);

              un_morph(i);
              morphed = TRUE;
              break;
            }
            else
              send_to_char("error: call of wild improperly set up\n", i);
          }

          if (af->type == SPELL_CHANNEL)
          {
            if (af->bitvector == 0)
            {
              if (IS_MORPH(i))
              {
                orig = i->only.npc->orig_char;

                act("$n begins to fade away... losing it's power.", FALSE, i,
                    NULL, orig, TO_NOTVICT);
                send_to_char("You return to your own body.\n", i);

                un_morph(i);
                morphed = TRUE;
                break;
              }
              else
                send_to_char("error: channel improperly set up\n", i);
            }
            else
            {
              // keep them knocked out if leader still in avatar
              if (i->desc && i->desc->snoop.snooping)
                continue;
            }
          }

          affect_remove(i, af);
        }
      }

      if (morphed)
      {
        morphed = FALSE;
        continue;
      }
    }

    /* can only kill them here because it's the last check for this char .. */

    if (char_falling(i))
      falling_char(i, TRUE, false);
  }
}

/** TAM 2/94 -- this func. needed for affects which would upset balance of **/
/**   game if they endured the whole 75 seconds (game hr).  in   **/
/**   other words allow for affects in increments of 20 seconds  **/
/**   (SHORT_AFFECT)               **/
/**   Note: a separate list *shouldve* been made for these short **/
/**         affects, probably in char_data->specials.      **/

/*
 * this should be removed, do not put anything in here
 */
//---------------------------------------------------------------------------------
void short_affect_update(void)
{
  bool killed;
  P_char   i, i_next, killer;
  struct affected_type *af, *next_af_dude;

  for( i = character_list; i; i = i_next )
  {
    i_next = i->next;
    /* Gonna clear up an ancient carryover, due to items and bugs, it's
     *   possible for a player/mob to get to STAT_DEAD without dying.  This way
     *   they have 20 seconds (maybe) grace and then we kill em off.  JAB
     */
    if( (GET_HIT(i) < -10) || (GET_MAX_HIT(i) < -10) )
    {
      // Log deaths of PCs.
      if( IS_PC(i) )
      {
        statuslog(i->player.level, "%s killed in %d (< -10 hits)", GET_NAME(i),
          ((i->in_room == NOWHERE) ? -1 : world[i->in_room].number));
        logit(LOG_DEATH, "%s killed in %d (< -10 hits)", GET_NAME(i),
          (i->in_room == NOWHERE) ? -1 : world[i->in_room].number);
      }

      killed = FALSE;
      // No more vit death or zerk death to avoid frags
      for( killer = world[i->in_room].people; killer; killer = killer->next_in_room )
      {
        if( GET_OPPONENT(killer) && GET_OPPONENT(killer) == i &&
	        killer->in_room == i->in_room && GET_RACEWAR(killer) != GET_RACEWAR(i) )
        {
          die(i, killer);
          killed = TRUE;
          break;
        }
      }

      // We don't die a second time if there's a killer.  This prevents crashes.
      if( !killed )
      {
        die(i, i);
      }
      i = NULL;
      continue;
    }
    /*      if (!IS_SET(i->specials.affected_by3, AFF3_SWIMMING) && IS_WATER_ROOM(i->in_room)
      && i->specials.z_cord == 0 && !IS_AFFECTED(i, AFF_LEVITATE) && !IS_AFFECTED(i, AFF_FLY))
swimming_char(i);*/

    /* drop a load every now and then..  if there's a better place for
      this, lemme know */
    poo(i);//  yeah, a better place would be a game run by 12 year olds - Jexni

    if (camp(i))
    {
      continue;
    }

  }
}                               /* short_affect_update  */

void strip_holy_sword( P_char ch )
{
  if( affected_by_spell(ch, SPELL_HOLY_SWORD) )
  {
    affect_from_char( ch, SPELL_HOLY_SWORD );
    send_to_char( "&+wYour weapon abruptly ceases to &+Cglow&+w with holy power.\n&n", ch );
  }
}
