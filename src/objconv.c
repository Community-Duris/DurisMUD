/*
 * ***************************************************************************
 *   File: objconv.c                                           Part of Duris
 *   Usage: balancing objects during initial loading
 *   Copyright 1994 - 2008 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "db.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "spells.h"
#include "objmisc.h"

extern Skill skills[];

float apply_cost[] = {
  0, // APPLY_NONE              0
  0, // APPLY_LOWEST            1
  // APPLY_STR               1
  // APPLY_DEX               2
  // APPLY_INT               3
  // APPLY_WIS               4
  // APPLY_CON               5
  0, // APPLY_SEX               6
  0, // APPLY_CLASS             7
  0, // APPLY_LEVEL             8
  0, // APPLY_AGE               9
  0, // APPLY_CHAR_WEIGHT      10
  0, // APPLY_CHAR_HEIGHT      11
  0.7, // APPLY_MANA             12
  1, // APPLY_HIT              13
  1, // APPLY_MOVE             14
  0, // APPLY_GOLD             15
  0, // APPLY_EXP              16
  0, // APPLY_AC               17
  0, // APPLY_ARMOR            17
  3, // APPLY_HITROLL          18
  5, // APPLY_DAMROLL          19
  4, // APPLY_SAVING_PARA      20
  0, // APPLY_SAVING_ROD       21
  3, // APPLY_SAVING_FEAR     22
  2, // APPLY_SAVING_BREATH    23
  4, // APPLY_SAVING_SPELL     24
  0, // APPLY_FIRE_PROT        25
  // APPLY_AGI              26  /* these 5 are the 'normal' applies for the new */
  // APPLY_POW              27  /* stats */
  // APPLY_CHA              28
  // APPLY_KARMA            29
  // APPLY_LUCK             30
  // APPLY_STR_MAX          31  /* these 10 can raise a stat above 100, I will */
  // APPLY_DEX_MAX          32  /* personally rip the lungs out of anyone using */
  // APPLY_INT_MAX          33  /* these on easy-to-get items.  JAB */
  // APPLY_WIS_MAX          34
  // APPLY_CON_MAX          35
  // APPLY_AGI_MAX          36
  // APPLY_POW_MAX          37
  // APPLY_CHA_MAX          38
  // APPLY_KARMA_MAX        39
  // APPLY_LUCK_MAX         40
  // APPLY_STR_RACE         41  /* these 10 override the racial stat_factor */
  // APPLY_DEX_RACE         42  /* so that setting APPLY_STR_RACE <ogre> will, */
  // APPLY_INT_RACE         43  /* for example, give you gauntlets of ogre strength. */
  // APPLY_WIS_RACE         44  /* these aren't imped yet, but I figured I'd add */
  // APPLY_CON_RACE         45  /* them so I don't forget about it. */
  // APPLY_AGI_RACE         46
  // APPLY_POW_RACE         47
  // APPLY_CHA_RACE         48
  // APPLY_KARMA_RACE       49
  // APPLY_LUCK_RACE        50
  // APPLY_CURSE            51
  // APPLY_SKILL_GRANT      52
  // APPLY_SKILL_ADD        53
  // APPLY_HIT_REG          54
  // APPLY_MOVE_REG         55
  // APPLY_MANA_REG         56
  // APPLY_SPELL_PULSE      57
  // APPLY_COMBAT_PULSE     58
};
/*
 * randomize obj stats for new randomize obj code
 *
 * level is level of mob carrying/using it - 0 if none
 */

void randomizeObj(P_obj obj, int level)
{
  int      lowmod, highmod, pct, i, j, oldval, neggood = FALSE, mod;
  float    lowmult, highmult;


  return;                       /* hmm */

  if (!obj)
    return;

  /* got a better idea? */

  if (level <= 0)
    level = 25;

  // first let's check affs

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (obj->affected[i].location == APPLY_NONE)
      continue;

    /* don't modify everything, leave wacky crap like APPLY_EXP and
       APPLY_CLASS alone */

    switch (obj->affected[i].location)
    {
    case APPLY_STR:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_CON:
    case APPLY_AGI:
    case APPLY_POW:
    case APPLY_CHA:
    case APPLY_KARMA:
    case APPLY_LUCK:
      pct = 20;
      break;

    case APPLY_STR_MAX:
    case APPLY_DEX_MAX:
    case APPLY_INT_MAX:
    case APPLY_WIS_MAX:
    case APPLY_CON_MAX:
    case APPLY_AGI_MAX:
    case APPLY_POW_MAX:
    case APPLY_CHA_MAX:
    case APPLY_KARMA_MAX:
    case APPLY_LUCK_MAX:
      pct = 20;
      break;

    case APPLY_STR_RACE:
    case APPLY_DEX_RACE:
    case APPLY_INT_RACE:
    case APPLY_WIS_RACE:
    case APPLY_CON_RACE:
    case APPLY_AGI_RACE:
    case APPLY_POW_RACE:
    case APPLY_CHA_RACE:
    case APPLY_KARMA_RACE:
    case APPLY_LUCK_RACE:
      pct = 20;
      break;

    case APPLY_HIT:
    case APPLY_MOVE:
      pct = 30;
      break;

    case APPLY_AC:
    case APPLY_HITROLL:
    case APPLY_DAMROLL:
      pct = 20;
      break;

    case APPLY_SAVING_PARA:
    case APPLY_SAVING_ROD:
    case APPLY_SAVING_FEAR:
    case APPLY_SAVING_BREATH:
    case APPLY_SAVING_SPELL:
      pct = 25;
      neggood = TRUE;
      break;

    default:
      continue;
    }

    oldval = obj->affected[i].modifier;
    if (!oldval)
      continue;

    lowmod = highmod = (int) ((float) oldval * ((float) pct / 100.0));

    // make lowmod negative and highmod positive

    if (lowmod > 0)
      lowmod = -lowmod;
    if (highmod < 0)
      highmod = -highmod;

    // if original val is too low, low/highmod will be 0..  in this case, mod slightly
    // if on high-level 

    if ((lowmod == 0) || (highmod == 0))
    {
      // give 50% chance, always mod if on level 40+

      if (number(0, 1) || (level > 40))
      {
        lowmod = -1;
        highmod = 1;
      }
      else
        return;
    }

    // low/highmod are > zero

    else
    {
      // roll a level-modified numb and do crap

      j = number(-10, 40) + (level / 2);


      // critical badness (can only happen to mobs < 30)

      if (j < 5)
        mod = number(lowmod, 0);

      // level 1-30 mobs will hit the 'generic' modder

      else if (j < 55)
        mod = number(lowmod, highmod);

      // level 30-56 mobs will hit this highmod now and then

      else if (j < 68)
        mod = number(0, highmod / 2);

      // got super-lucky..  only 56+ mobs are going to hit this

      else
        mod = number(0, highmod);
    }

    obj->affected[i].modifier += (neggood ? (0 - mod) : mod);
  }
}


/* similar to GetSpellCircle, except it doesnt check circle for a given
   char. Rather, it simply returns the lowest circle this spell could
   be for a proper user.
 */

int GetCircle(int spl)
{
  int      circle = 11, i;

  for (i = 0; i < CLASS_COUNT; i++)
  {
    if (skills[spl].m_class[i].rlevel[0] &&
        skills[spl].m_class[i].rlevel[0] < circle)
      circle = skills[spl].m_class[i].rlevel[0];
  }
  circle = BOUNDED(2, circle, 11);      /* 2 prevents them from being too cheap */
  return circle;
}

/*
 * returns a base spell cost (copper).  this is later modified by type of item, etc etc
 */

int getSpellCost(const int spell)
{
  if (spell < 1)
    return 0;
  if (spell > LAST_SPELL)
    return 80000;               /* let's assume it's a new spell */

  switch (spell)
  {
  case SPELL_ARMOR:
    return 2000;
  case SPELL_TELEPORT:
    return 100000;
  case SPELL_BLESS:
    return 3000;
  case SPELL_BLINDNESS:
    return 20000;
  case SPELL_BURNING_HANDS:
    return 35000;
  case SPELL_CALL_LIGHTNING:
    return 45000;
  case SPELL_CHARM_PERSON:
    return 100000;
  case SPELL_CHILL_TOUCH:
    return 30000;
  case SPELL_FULL_HEAL:
    return 400000;
  case SPELL_CONE_OF_COLD:
    return 75000;
  case SPELL_CONTROL_WEATHER:
    return 50000;
  case SPELL_CREATE_FOOD:
    return 1000;
  case SPELL_CREATE_WATER:
    return 1500;
  case SPELL_CURE_BLIND:
    return 5000;
  case SPELL_CURE_CRITIC:
    return 50000;
  case SPELL_CURE_LIGHT:
    return 10000;
  case SPELL_CURSE:
    return 35000;
  case SPELL_CONTINUAL_LIGHT:
    return 50000;
  case SPELL_DETECT_INVISIBLE:
    return 65000;
  case SPELL_MINOR_CREATION:
    return 35000;
  case SPELL_FLAMESTRIKE:
    return 45000;
  case SPELL_DISPEL_EVIL:
    return 35000;
  case SPELL_EARTHQUAKE:
    return 60000;
  case SPELL_ENCHANT_WEAPON:
    return 50000;
  case SPELL_ENERGY_DRAIN:
    return 100000;
  case SPELL_FIREBALL:
    return 100000;
  case SPELL_HARM:
    return 50000;
  case SPELL_HEAL:
    return 100000;
  case SPELL_INVISIBLE:
    return 200000;
  case SPELL_LIGHTNING_BOLT:
    return 55000;
  case SPELL_LOCATE_OBJECT:
    return 45000;
  case SPELL_MAGIC_MISSILE:
    return 7500;
  case SPELL_POISON:
    return 35000;
  case SPELL_PROTECT_FROM_EVIL:
    return 20000;
  case SPELL_REMOVE_CURSE:
    return 6000;
  case SPELL_STONE_SKIN:
    return 75000;
  case SPELL_SHOCKING_GRASP:
    return 45000;
  case SPELL_SLEEP:
    return 40000;
  case SPELL_STRENGTH:
    return 30000;
  case SPELL_SUMMON:
    return 100000;
  case SPELL_HASTE:
    return 80000;
  case SPELL_WORD_OF_RECALL:
    return 500000;
  case SPELL_REMOVE_POISON:
    return 6000;
  case SPELL_SENSE_LIFE:
    return 85000;
  case SPELL_IDENTIFY:
    return 3500;
  //case SPELL_VENTRILOQUATE:
    //return 2500;
  case SPELL_FIRESTORM:
    return 150000;
  case SPELL_FIRE_BREATH:
    return 150000;
  case SPELL_GAS_BREATH:
    return 150000;
  case SPELL_FROST_BREATH:
    return 150000;
  case SPELL_ACID_BREATH:
    return 150000;
  case SPELL_LIGHTNING_BREATH:
    return 150000;
  case SPELL_FARSEE:
    return 65000;
  case SPELL_FEAR:
    return 8000;
  case SPELL_RECHARGER:
    return 30000;
  case SPELL_VITALITY:
    return 40000;
  case SPELL_CURE_SERIOUS:
    return 37500;
  case SPELL_DESTROY_UNDEAD:
    return 60000;
  case SPELL_METEOR_SWARM:
    return 400000;
  case SPELL_CREEPING:
    return 450000;
  case SPELL_MINOR_GLOBE:
    return 200000;
  case SPELL_CHAIN_LIGHTNING:
    return 200000;
  case SPELL_DIMENSION_DOOR:
    return 200000;
  case SPELL_VIGORIZE_LIGHT:
    return 3000;
  case SPELL_VIGORIZE_SERIOUS:
    return 10000;
  case SPELL_VIGORIZE_CRITIC:
    return 25000;
  case SPELL_DISPEL_INVISIBLE:
    return 75000;
  case SPELL_WIZARD_EYE:
    return 50000;
  case SPELL_CLAIRVOYANCE:
    return 200000;
  case SPELL_REJUVENATE_MAJOR:
    return 75000;
  case SPELL_RAY_OF_ENFEEBLEMENT:
    return 10000;
  case SPELL_DISPEL_GOOD:
    return 35000;
  case SPELL_DEXTERITY:
    return 25000;
  case SPELL_REJUVENATE_MINOR:
    return 50000;
  case SPELL_AGE:
    return 100000;
  case SPELL_CYCLONE:
    return 160000;
  case SPELL_BIGBYS_CLENCHED_FIST:
    return 100000;
  case SPELL_CONJURE_ELEMENTAL:
    return 100000;
  case SPELL_VITALIZE_MANA:
    return 75000;
  case SPELL_RELOCATE:
    return 1000000;
  case SPELL_PROTECT_FROM_GOOD:
    return 20000;
  case SPELL_ANIMATE_DEAD:
    return 75000;
  case SPELL_LEVITATE:
    return 60000;
  case SPELL_FLY:
    return 100000;
  case SPELL_WATERBREATH:
    return 50000;
  case SPELL_PLANE_SHIFT:
    return 500000;
  case SPELL_GATE:
    return 750000;
  case SPELL_RESURRECT:
    return 5000000;
  case SPELL_MASS_CHARM:
    return 500000;
  case SPELL_DETECT_EVIL:
    return 15000;
  case SPELL_DETECT_GOOD:
    return 15000;
  case SPELL_DETECT_MAGIC:
    return 15000;
  case SPELL_DISPEL_MAGIC:
    return 35000;
  case SPELL_PRESERVE:
    return 5000;
  case SPELL_MASS_INVIS:
    return 750000;
  case SPELL_PROTECT_FROM_FIRE:
    return 30000;
  case SPELL_PROTECT_FROM_COLD:
    return 30000;
  case SPELL_PROTECT_FROM_LIGHTNING:
    return 30000;
  case SPELL_DARKNESS:
    return 60000;
  case SPELL_MINOR_PARALYSIS:
    return 75000;
  case SPELL_MAJOR_PARALYSIS:
    return 150000;
  case SPELL_SLOW:
    return 50000;
  case SPELL_WITHER:
    return 60000;
  case SPELL_PROTECT_FROM_GAS:
    return 30000;
  case SPELL_PROTECT_FROM_ACID:
    return 30000;
  case SPELL_INFRAVISION:
    return 30000;
  case SPELL_PRISMATIC_SPRAY:
    return 300000;
  case SPELL_FIRESHIELD:
    return 100000;
  case SPELL_COLOR_SPRAY:
    return 250000;
  case SPELL_INCENDIARY_CLOUD:
    return 350000;
  case SPELL_ICE_STORM:
    return 200000;
  case SPELL_DISINTEGRATE:
    return 150000;
  case SPELL_CAUSE_LIGHT:
    return 10000;
  case SPELL_CAUSE_SERIOUS:
    return 25000;
  case SPELL_CAUSE_CRITICAL:
    return 45000;
  case SPELL_ACID_BLAST:
    return 45000;
  case SPELL_FAERIE_FIRE:
    return 34392;
  case SPELL_FAERIE_FOG:
    return 62000;
  case SPELL_PWORD_KILL:
    return 250000;
  case SPELL_PWORD_BLIND:
    return 75000;
  case SPELL_PWORD_STUN:
    return 55000;
  case SPELL_UNHOLY_WORD:
    return 225000;
  case SPELL_HOLY_WORD:
    return 275000;
  case SPELL_SUNRAY:
    return 75000;
  case SPELL_FEEBLEMIND:
    return 75000;
  case SPELL_SILENCE:
    return 75000;
  case SPELL_TURN_UNDEAD:
    return 45000;
  case SPELL_COMMAND_UNDEAD:
    return 75000;
  case SPELL_SLOW_POISON:
    return 3000;
  case SPELL_COLDSHIELD:
    return 110000;
  case SPELL_COMPREHEND_LANGUAGES:
    return 17500;
  case SPELL_WRAITHFORM:
    return 500000;
  case SPELL_VAMPIRIC_TOUCH:
    return 80000;
  case SPELL_PROT_UNDEAD:
    return 30000;
  case SPELL_PROT_FROM_UNDEAD:
    return 30000;
  case SPELL_COMMAND_HORDE:
    return 150000;
  case SPELL_HEAL_UNDEAD:
    return 65000;
  case SPELL_ENTANGLE:
    return 45000;
  case SPELL_CREATE_SPRING:
    return 8500;
  case SPELL_BARKSKIN:
    return 8750;
  case SPELL_MOONWELL:
    return 2000000;
  case SPELL_CREATE_DRACOLICH:
    return 500000;
  case SPELL_GLOBE:
    return 750000;
  case SPELL_EMBALM:
    return 10000;
  case SPELL_SHADOW_BREATH_1:
    return 250000;
  case SPELL_SHADOW_BREATH_2:
    return 250000;
  case SPELL_WALL_OF_FLAMES:
    return 200000;
  case SPELL_WALL_OF_ICE:
    return 200000;
  case SPELL_WALL_OF_STONE:
    return 200000;
  case SPELL_WALL_OF_IRON:
    return 200000;
  case SPELL_WALL_OF_FORCE:
    return 200000;
  case SPELL_WALL_OF_BONES:
    return 200000;
  case SPELL_LIGHTNING_CURTAIN:
    return 200000;
  case SPELL_MOLECULAR_CONTROL:
    return 150000;
  case SPELL_MOLECULAR_AGITATION:
    return 75000;
  case SPELL_ADRENALINE_CONTROL:
    return 50000;
  case SPELL_AURA_SIGHT:
    return 80000;
  case SPELL_AWE:
    return 80000;
  case SPELL_BALLISTIC_ATTACK:
    return 35000;
  case SPELL_BIOFEEDBACK:
    return 100000;
  case SPELL_CELL_ADJUSTMENT:
    return 45000;
  case SPELL_COMBAT_MIND:
    return 55000;
//    case SPELL_CONTROL_FLAMES          : return 25000;
  case SPELL_EGO_BLAST:
    return 25000;
  case SPELL_CREATE_SOUND:
    return 85000;
  case SPELL_DEATH_FIELD:
    return 650000;
  case SPELL_DETONATE:
    return 65000;
  case SPELL_DISPLACEMENT:
    return 35000;
  case SPELL_DOMINATION:
    return 65000;
  case SPELL_ECTOPLASMIC_FORM:
    return 55000;
  case SPELL_EGO_WHIP:
    return 75320;
  case SPELL_ENERGY_CONTAINMENT:
    return 80000;
  case SPELL_ENHANCE_ARMOR:
    return 40000;
  case SPELL_ENHANCED_STR:
    return 30000;
  case SPELL_ENHANCED_DEX:
    return 30000;
  case SPELL_ENHANCED_AGI:
    return 45000;
  case SPELL_ENHANCED_CON:
    return 45000;
  case SPELL_FLESH_ARMOR:
    return 35000;
  case SPELL_INERTIAL_BARRIER:
    return 70000;
  case SPELL_INFLICT_PAIN:
    return 35000;
  case SPELL_INTELLECT_FORTRESS:
    return 45000;
  case SPELL_LEND_HEALTH:
    return 2000;
  case SPELL_POWERCAST_FLY:
    return 100000;
  case SPELL_CONFUSE:
    return 150000;
  case SPELL_DISEASE:
    return 85000;
  case SPELL_CHARM_ANIMAL:
    return 65000;
  case SPELL_SOULSHIELD:
    return 85000;
  case SPELL_INVIS_MAJOR:
    return 400000;
  case SPELL_MASS_HEAL:
    return 500000;
  case SPELL_ICE_MISSILE:
    return 7500;
  case SPELL_SPIRIT_ARMOR:
    return 2000;
  case SPELL_WOLFSPEED:
    return 18750;
  case SPELL_TRANSFER_WELLNESS:
    return 2500;
  case SPELL_FLAMEBURST:
    return 25000;
  case SPELL_SCALDING_BLAST:
    return 35000;
  case SPELL_PYTHONSTING:
    return 8500;
  case SPELL_SNAILSPEED:
    return 23450;
  case SPELL_MOLEVISION:
    return 28392;
  case SPELL_PURIFY_SPIRIT:
    return 35000;
  case SPELL_PANTHERSPEED:
    return 42500;
  case SPELL_MOUSESTRENGTH:
    return 39400;
  case SPELL_SUMMON_BEAST:
    return 67500;
  case SPELL_HAWKVISION:
    return 42500;
  case SPELL_SCORCHING_TOUCH:
    return 42000;
  case SPELL_MENDING:
    return 65000;
  case SPELL_SOUL_DISTURBANCE:
    return 62000;
  case SPELL_SPIRIT_SIGHT:
    return 45000;
  case SPELL_BEARSTRENGTH:
    return 32000;
  case SPELL_SHREWTAMENESS:
    return 35000;
  case SPELL_SENSE_SPIRIT:
    return 80000;
  case SPELL_MALISON:
    return 35000;
  case SPELL_WELLNESS:
    return 450000;
  case SPELL_GREATER_MENDING:
    return 200000;
  case SPELL_LIONRAGE:
    return 75000;
  case SPELL_SPIRIT_ANGUISH:
    return 68000;
  case SPELL_EARTHEN_GRASP:
    return 50000;
  case SPELL_SUMMON_SPIRIT:
    return 200000;
  case SPELL_GREATER_SOUL_DISTURB:
    return 100000;
  case SPELL_SPIRIT_WARD:
    return 80000;
  case SPELL_ELEPHANTSTRENGTH:
    return 75000;
  case SPELL_GREATER_PYTHONSTING:
    return 35000;
  case SPELL_SCATHING_WIND:
    return 150000;
  case SPELL_REVEAL_TRUE_FORM:
    return 52400;
  case SPELL_GREATER_SPIRIT_SIGHT:
    return 100000;
  case SPELL_EARTHEN_RAIN:
    return 250000;
  case SPELL_GREATER_SPIRIT_WARD:
    return 120000;
  case SPELL_SUSTENANCE:
    return 1500;
  case SPELL_CORROSIVE_BLAST:
    return 2300;
  case SPELL_RESTORATION:
    return 8000;
  case SPELL_GREATER_SUST:
    return 15000;
  case SPELL_COLD_WARD:
    return 30000;
  case SPELL_FIRE_WARD:
    return 30000;
  case SPELL_LESSER_MENDING:
    return 8500;
  case SPELL_MOLTEN_SPRAY:
    return 45000;
  case SPELL_TRUE_SEEING:
    return 100000;
  case SPELL_IRONWOOD:
    return 42;
  case SPELL_RAVENFLIGHT:
    return 90000;
  case SPELL_GREATER_RAVENFLIGHT:
    return 250000;
  case SPELL_LORE:
    return 2500;
  case SPELL_WORMHOLE:
    return 2000000;
  case SPELL_ETHERPORTAL:
    return 2000000;
  case SPELL_FULL_HARM:
    return 85000;
  case SPELL_CONJURE_GREATER_ELEMENTAL:
    return 500000;
  case SPELL_GROUP_RECALL:
    return 50000000;
  case SPELL_GREATER_EARTHEN_GRASP:
    return 150000;
  case SPELL_GROUP_STONE_SKIN:
    return 300000;
  case SPELL_ENLARGE:
    return 100000;
  case SPELL_REDUCE:
    return 100000;
  case SPELL_REVEAL_SPIRIT_ESSENCE:
    return 2500;
  case SPELL_SPIRIT_JUMP:
    return 200000;
  case SPELL_BEHOLDER_DISIN:
    return 100000;
  case SPELL_BEHOLDER_DAMAGE:
    return 100000;
  case SPELL_ARIEKS_SHATTERING_ICEBALL:
    return 100000;
  case SPELL_ENRAGE:
    return 65000;
  case SPELL_BLINDING_BREATH:
    return 125000;
  case SPELL_MIND_BLANK:
    return 75000;
  case SPELL_SIGHT_LINK:
    return 300000;
  case SPELL_CANNIBALIZE:
    return 75000;
  case SPELL_TOWER_IRON_WILL:
    return 125000;
  case SPELL_MIRROR_IMAGE:
    return 10000;
  case SPELL_REVEAL_TRUE_NAME:
    return 750;
  case SPELL_BLUR:
    return 80000;
  case SPELL_PRISMATIC_CUBE:
    return 250000;
  case SPELL_JUDGEMENT:
    return 50000;
  case SPELL_GREATER_WRAITHFORM:
    return 3000000;
  case SPELL_ELEMENTAL_FORM:
    return 50000;

  default:
    return 3000;
  }
}

#define ALL_MAGES (CLASS_SORCERER | CLASS_NECROMANCER | CLASS_SUMMONER | CLASS_CONJURER \
  | CLASS_ILLUSIONIST |  CLASS_PSIONICIST)

#define ALL_ROGUES (CLASS_THIEF | CLASS_ASSASSIN | CLASS_BARD | CLASS_ROGUE)

// Sets anti-class flags based on material type and object name.
void material_restrictions( P_obj obj )
{
  ulong anti, anti2;
  int   mat;
  // Since it's referenced so much.
  char *name = obj->name;

  if( !obj )
  {
    return;
  }

  anti = anti2 = 0;
  mat = obj->material;

  if( isname("quiver", name) || isname("badge", name)   || isname("robe", name)
    || isname("tunic", name) || isname("cloak", name)   || isname("pants", name)
    || isname("belt", name)  || isname("earring", name) || isname("moccasins", name)
    || isname("ring", name)  || isname("band", name)    || isname("signet", name)
    || isname("hat", name)   || isname("cap", name)     || isname("bracelet", name)
    || isname("stud", name)  || isname("amulet", name)  || isname("bodycloak", name) )
  {
    return;
  }

  if( IS_RIGID(mat) && (obj->wear_flags & ITEM_WEAR_BODY) )
  {
    anti = ALL_MAGES | ALL_ROGUES | CLASS_MONK;
  }

  if( IS_RIGID(mat) && (obj->wear_flags & (ITEM_WEAR_FACE | ITEM_WEAR_ARMS | ITEM_WEAR_HEAD | ITEM_WEAR_LEGS)) )
  {
    anti |= CLASS_MONK;
  }

  if( IS_METAL(mat) && (mat != MAT_MITHRIL)
    && (obj->wear_flags & (ITEM_WEAR_FACE |ITEM_WEAR_ARMS | ITEM_WEAR_HEAD | ITEM_WEAR_LEGS)) )
  {
    anti |= ALL_MAGES;
  }

  if( obj->extra_flags & ITEM_ALLOWED_CLASSES )
    obj->anti_flags &= ~anti;
  else
    obj->anti_flags |= anti;
}

void convertObj(P_obj obj)
{
  int      i, val0, val1, val2, val3, type;
  long     weight = 0, cost = 0;
  char     buf2[MAX_STRING_LENGTH];

  if( !obj || IS_SET(obj->extra_flags, ITEM_IGNORE) )
    return;

  obj->bitvector &= ~(AFF_SLEEP | AFF_FEAR | AFF_CHARM);

  val0 = obj->value[0];
  val1 = obj->value[1];
  val2 = obj->value[2];
  val3 = obj->value[3];
  type = GET_ITEM_TYPE(obj);

  /* set a few base values */

  switch (type)
  {
  case ITEM_TELEPORT:
    weight = 10000;
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    /* 5g per circle * (lvl / 10) * spells */
/*
    cost = 500 * (val0 / 10) * GetCircle(val1);
    if (val3)
      cost += (500 * (val0 / 10) * GetCircle(val2));
    else if (val2)
      cost += (500 * (val0 / 10) * GetCircle(val3));
*/
 /* 
   cost = (getSpellCost(val1) + getSpellCost(val2) + getSpellCost(val3));
    if (val0 >= 5)
      cost *= (val0 / 5);
*/
    /* scrolls are slightly better than potions, being targettable on anyone for
       some spells */
/*
    if (type == ITEM_SCROLL)
      cost = (int) (cost * 1.25);
*/
    weight = 2;
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    /* 5 g * circle * lvl * percentage of charges left */
/*
    if (val1 && val2)
      cost = 500 * (val0 / 10) * (val1 / val2) * GetCircle(val3);
    else
      cost = 500 * (val0 / 10) * .1 * GetCircle(val3);
*/
    /* rather than base it on percentage of charges left [a 100 charge staff with one
       charge of relocate would thus be rather cheap], let's just make it cost more
       for number of charges and a little more based on max */

/*
    cost = (int) (getSpellCost(val3) * val2 * (1.0 + (0.01 * val1)));
    if (val0 >= 5)
      cost *= (val0 / 5);
*/
    /* staves are decidedly better than wands */
/*
    if (type == ITEM_STAFF)
      cost *= 2;
    else
      cost = (int) (cost * 1.5);
*/
    weight = (type == ITEM_WAND) ? 2 : 5;
    break;
  case ITEM_WEAPON:
    /* 1 g * max damage */

    /* number of dice more important than size.. */
    cost = 100 * (val1 * 2) * val2;
    break;
  case ITEM_FIREWEAPON:
    /* 1 s * missle type * rate of fire */

    cost = (val0 ? 1 : 0) * (val1 ? 1 : 0) * 10;
    break;
  case ITEM_MISSILE:
    /* 1 s * missle type */
//    cost = val3 * (val1 * 2) * val2 * (val0 / 2) * 10;
    // Start with average damage * 10.
    cost = ( obj->value[1] * (obj->value[2] + 1) ) * 5;
    // cost = (10 * avgdam) squared * maxdamage cubed / 125
    cost = (cost * cost * val1 * val1 * val1 * val2 * val2 * val2) / 125;
    obj->cost = BOUNDED(1, cost, 5000000);
    weight = obj->weight;
    obj->weight = BOUNDED(0, weight, 10);
    return;
    break;
  case ITEM_TREASURE:
  case ITEM_TRASH:
  case ITEM_OTHER:
    /* hmm */
    break;
  case ITEM_ARMOR:
    /* 1 g * ac */
    if (val0 < 10)
      cost = 150 * val0;
    else if (val0 < 20)
      cost = 300 * val0;
    else if (val0 < 30)
      cost = 625 * val0;
    else if (val0 < 40)
      cost = 1250 * val0;
    else
      cost = 2500 * val0;
#if 0
    weight = 10;                /* modified later by material */
#endif
    break;
  case ITEM_WORN:
    cost = 100;
#if 0
    weight = 1;
#endif
    break;
  case ITEM_CONTAINER:
    /* 1 silver per pound */
    cost = 25 * val0;
    if (obj->weight <= 0)
    {
      weight = obj->weight;
      cost += (weight * -500);
    }
    if (weight >= 0)
      weight = (val0 > 50) ? 3 : 1;
    if (IS_SET(obj->wear_flags, ITEM_ATTACH_BELT) && weight > 9)
      REMOVE_BIT(obj->wear_flags, ITEM_ATTACH_BELT);
    if (obj->R_num == real_object(96443))
      cost = 1000;
    break;
  case ITEM_DRINKCON:
  case ITEM_QUIVER:
    /* 1 silver per drink/arrows held */
    cost = 20 * val0;
    weight = (val0 > 50) ? 3 : 1;
    if (IS_SET(obj->wear_flags, ITEM_ATTACH_BELT) && weight > 9)
      REMOVE_BIT(obj->wear_flags, ITEM_ATTACH_BELT);
    break;
  case ITEM_FOOD:
    if (isname("rations", obj->name))
      cost = 20;
    break;
  case ITEM_NOTE:
  case ITEM_PEN:
  case ITEM_BOOK:
  case ITEM_PICK:
    /* simple base values */
    cost = 5;
    if (type == ITEM_BOOK)
      weight = 3;
    else
      weight = 0;
    break;
  case ITEM_KEY:
    cost = 5;
    if (type == ITEM_BOOK)
      weight = 3;
    else
      weight = 0;
    SET_BIT(obj->extra_flags, ITEM_NORENT);
    break;
  case ITEM_SPELLBOOK:
    /* 15 c per page in book */
    break;
  case ITEM_INSTRUMENT:
    if (!strstr(obj->name, "instrument"))
    {
      sprintf(buf2, "%s %s", obj->name, "instrument");
      obj->name = str_dup(buf2);
    }
    break;
  case ITEM_TOTEM:
    /* ? */
    if (!strstr(obj->name, "totem"))
    {
      sprintf(buf2, "%s %s", obj->name, "totem");
      obj->name = str_dup(buf2);
    }
/*    weight = (GET_ITEM_TYPE(obj) == ITEM_TOTEM) ? 2 : 3;
    cost = 1;*/
    break;
  case ITEM_STORAGE:
    /* 5 p per pound held */
    cost = 5000 * val0;
/*    weight = (val0 > 100) ? 250 : 100; */
    break;
  }
  if (!cost)
    cost = obj->cost;
/*  if (!weight)
    weight = obj->weight;*/

  cost = BOUNDED(0, cost, 1000000);
  weight = BOUNDED(0, weight, 1000);

  /* at this point, we either have made cost/weight, or we're still using
     values from the files (for iffy items). From here on, we add/subtract */

  /* figure in the extras flags */

  if (IS_SET(obj->extra2_flags, ITEM2_MAGIC))
    cost *= 2;
  if (IS_SET(obj->extra_flags, ITEM_GLOW))
    cost += 100;
  if (IS_SET(obj->extra_flags, ITEM_HUM))
    cost += 100;
  if (IS_SET(obj->extra2_flags, ITEM2_BLESS))
    cost += 500;
  if (IS_SET(obj->extra_flags, ITEM_FLOAT))
    cost += 1000;
  if (IS_SET(obj->extra_flags, ITEM_NOSUMMON))
    cost += 15000;
  if (IS_SET(obj->extra_flags, ITEM_LIT))
    cost += 500;
  if (IS_SET(obj->extra_flags, ITEM_NOSLEEP))
    cost += 15000;
  if (IS_SET(obj->extra_flags, ITEM_NOCHARM))
    cost += 12500;
  if (IS_SET(obj->extra_flags, ITEM_TWOHANDS))
    weight += 5;
  if (IS_SET(obj->extra2_flags, ITEM2_SILVER))
  {
    cost += 5000;
    weight += 2;
  }
  if (IS_SET(obj->extra2_flags, ITEM2_SLAY_GOOD))
    cost += 6000;
  if (IS_SET(obj->extra2_flags, ITEM2_SLAY_EVIL))
    cost += 6000;
  if (IS_SET(obj->extra2_flags, ITEM2_SLAY_UNDEAD))
    cost += 6000;
  if (IS_SET(obj->extra2_flags, ITEM2_SLAY_LIVING))
    cost += 6000;
  if (IS_SET(obj->extra_flags, ITEM_RETURNING))
    cost += 2500;
  if (IS_SET(obj->extra_flags, ITEM_CAN_THROW1))
  {
    cost += 1200;
    weight -= 1;
  }
  if (IS_SET(obj->extra_flags, ITEM_CAN_THROW2))
  {
    cost += 5000;
    weight -= 2;
  }
  if (IS_SET(obj->extra_flags, ITEM_NORENT))
    cost /= 2;
  if (IS_SET(obj->extra_flags, ITEM_NODROP))
    cost = (int) (cost / 1.5);

  /* affects get added */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    switch (obj->affected[i].location)
    {
    case APPLY_NONE:
      break;
    case APPLY_STR:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_CON:
    case APPLY_AGI:
    case APPLY_POW:
    case APPLY_CHA:
    case APPLY_KARMA:
    case APPLY_LUCK:
      if (obj->affected[i].modifier > 0)
        cost += obj->affected[i].modifier * 150;
      else
        cost += obj->affected[i].modifier * 100;
      break;
    case APPLY_STR_MAX:
    case APPLY_DEX_MAX:
    case APPLY_INT_MAX:
    case APPLY_WIS_MAX:
    case APPLY_CON_MAX:
    case APPLY_AGI_MAX:
    case APPLY_POW_MAX:
    case APPLY_CHA_MAX:
    case APPLY_KARMA_MAX:
    case APPLY_LUCK_MAX:
    case APPLY_STR_RACE:
    case APPLY_DEX_RACE:
    case APPLY_INT_RACE:
    case APPLY_WIS_RACE:
    case APPLY_CON_RACE:
    case APPLY_AGI_RACE:
    case APPLY_POW_RACE:
    case APPLY_CHA_RACE:
    case APPLY_KARMA_RACE:
    case APPLY_LUCK_RACE:
      cost += obj->affected[i].modifier * 2500;
      break;
    case APPLY_SEX:
    case APPLY_CLASS:
    case APPLY_LEVEL:
    case APPLY_CHAR_WEIGHT:
    case APPLY_CHAR_HEIGHT:
    case APPLY_GOLD:
    case APPLY_EXP:
      cost += 5000;
      break;
    case APPLY_HIT:
      cost += obj->affected[i].modifier * 1000;
      break;
    case APPLY_MANA:
      cost += obj->affected[i].modifier * 500;
      break;
    case APPLY_MOVE:
      cost += obj->affected[i].modifier * 50;
      break;
    case APPLY_AGE:
      cost -= obj->affected[i].modifier * 1500; /* minus cause younger is better */
      break;
    case APPLY_ARMOR:
      if (obj->affected[i].modifier > -10)
        cost -= 500 * val0;
      else if (obj->affected[i].modifier > -20)
        cost -= 1000 * val0;
      else if (obj->affected[i].modifier > -30)
        cost -= 2000 * val0;
      else if (obj->affected[i].modifier > -40)
        cost -= 3500 * val0;
      else
        cost -= 6000 * val0;
      /* minus cause armor is a neg value */
      break;
    case APPLY_HITROLL:
    case APPLY_DAMROLL:
      cost += obj->affected[i].modifier * 1300;
      break;
    case APPLY_SAVING_PARA:
    case APPLY_SAVING_ROD:
    case APPLY_SAVING_FEAR:
    case APPLY_SAVING_BREATH:
    case APPLY_SAVING_SPELL:
      cost -= obj->affected[i].modifier * 2500; /* negative is better */
      break;
    }
  }

  /* bitvectors make big difference */
  if (IS_SET(obj->bitvector, AFF_BLIND))
    cost -= 5000;
  if (IS_SET(obj->bitvector, AFF_INVISIBLE))
    cost += 100000;
  if (IS_SET(obj->bitvector, AFF_FARSEE))
    cost += 10000;
  if (IS_SET(obj->bitvector, AFF_DETECT_INVISIBLE))
    cost += 4000;
  if (IS_SET(obj->bitvector, AFF_HASTE))
    cost += 25000;
  if (IS_SET(obj->bitvector, AFF_SENSE_LIFE))
    cost += 3500;
  if (IS_SET(obj->bitvector, AFF_MINOR_GLOBE))
    cost += 125000;
  if (IS_SET(obj->bitvector, AFF_STONE_SKIN))
    cost += 25000;
  if (IS_SET(obj->bitvector, AFF_UD_VISION))
    cost += 15000;
  if (IS_SET(obj->bitvector, AFF_WRAITHFORM))
    cost += 20000;
  if (IS_SET(obj->bitvector, AFF_WATERBREATH))
    cost += 10000;
  if (IS_SET(obj->bitvector, AFF_PROTECT_EVIL))
    cost += 15000;
  if (IS_SET(obj->bitvector, AFF_SLOW_POISON))
    cost += 7500;
  if (IS_SET(obj->bitvector, AFF_PROTECT_GOOD))
    cost += 15000;
  if (IS_SET(obj->bitvector, AFF_SLEEP))
    cost += 2500;
  if (IS_SET(obj->bitvector, AFF_SNEAK))
    cost += 50000;
  if (IS_SET(obj->bitvector, AFF_HIDE))
    cost += 250000;
  if (IS_SET(obj->bitvector, AFF_BARKSKIN))
    cost += 10420;
  if (IS_SET(obj->bitvector, AFF_INFRAVISION))
    cost += 60000;
  if (IS_SET(obj->bitvector, AFF_LEVITATE))
    cost += 40000;
  if (IS_SET(obj->bitvector, AFF_FLY))
    cost += 90000;
  if (IS_SET(obj->bitvector, AFF_AWARE))
    cost += 100000;
  if (IS_SET(obj->bitvector, AFF_PROT_FIRE))
    cost += 25000;
  if (IS_SET(obj->bitvector, AFF_BIOFEEDBACK))
    cost += 75000;
  if (IS_SET(obj->bitvector2, AFF2_FIRESHIELD))
    cost += 200000;
  if (IS_SET(obj->bitvector2, AFF2_ULTRAVISION))
    cost += 75000;
  if (IS_SET(obj->bitvector2, AFF2_DETECT_EVIL))
    cost += 10000;
  if (IS_SET(obj->bitvector2, AFF2_DETECT_GOOD))
    cost += 10000;
  if (IS_SET(obj->bitvector2, AFF2_DETECT_MAGIC))
    cost += 4000;
  if (IS_SET(obj->bitvector2, AFF2_PROT_COLD))
    cost += 25000;
  if (IS_SET(obj->bitvector2, AFF2_PROT_LIGHTNING))
    cost += 25000;
  if (IS_SET(obj->bitvector2, AFF2_GLOBE))
    cost += 500000;
  if (IS_SET(obj->bitvector2, AFF2_PROT_GAS))
    cost += 25000;
  if (IS_SET(obj->bitvector2, AFF2_PROT_ACID))
    cost += 25000;
  if (IS_SET(obj->bitvector2, AFF2_SOULSHIELD))
    cost += 75000;
  if (IS_SET(obj->bitvector2, AFF2_MINOR_INVIS))
    cost += 200000;
  if (IS_SET(obj->bitvector2, AFF2_VAMPIRIC_TOUCH))
    cost += 150000;
  if (IS_SET(obj->bitvector2, AFF2_PASSDOOR))
    cost += 75000;

  /* default condition hurts it */
  if (obj->condition < 50)
    cost /= 3;
  else if (obj->condition < 75)
    cost /= 2;
  else if (obj->condition < 90)
    cost = (int) (cost * 1.25);

  /* for some items, consider material type */
#if 0
  if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
  {
    switch (obj->material)
    {
    case MAT_METAL:
      weight += 10;
      break;
    case MAT_MAG_METAL:
      weight += 5;
      cost += 1000;
      break;
    case MAT_WOOD:
      weight += 5;
      cost += 250;
      break;
    case MAT_CLOTH:
      cost += 50;
      break;
    case MAT_HIDE:
      weight += 1;
      cost += 125;
      break;
    case MAT_SILICON:
      weight += 5;
      cost += 750;
      break;
    case MAT_CRYSTAL:
      weight += 7;
      cost += 1000;
      break;
    case MAT_MAGICAL:
      weight += 5;
      cost += 1250;
      break;
    case MAT_BONE:
      weight += 10;
      cost += 50;
      break;
    case MAT_STONE:
      weight += 25;
      cost += 25;
    }
  }
#endif
  /* armor and worn items weights are affected by locale worn */
  if (GET_ITEM_TYPE(obj) == ITEM_ARMOR || GET_ITEM_TYPE(obj) == ITEM_WORN)
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER) || CAN_WEAR(obj, ITEM_GUILD_INSIGNIA)
        || CAN_WEAR(obj, ITEM_WEAR_EYES) || CAN_WEAR(obj, ITEM_WEAR_EARRING))
      weight = 0;
    else if (CAN_WEAR(obj, ITEM_WEAR_HEAD) || CAN_WEAR(obj, ITEM_WEAR_WAIST)
             || CAN_WEAR(obj, ITEM_WEAR_WRIST) ||
             CAN_WEAR(obj, ITEM_WEAR_TAIL) || CAN_WEAR(obj, ITEM_WEAR_QUIVER)
             || CAN_WEAR(obj, ITEM_WEAR_NOSE) ||
             CAN_WEAR(obj, ITEM_WEAR_HORN))
      weight -= 10;
    else if (CAN_WEAR(obj, ITEM_WEAR_NECK) || CAN_WEAR(obj, ITEM_WEAR_FACE) ||
             CAN_WEAR(obj, ITEM_WEAR_FEET) || CAN_WEAR(obj, ITEM_WEAR_HANDS))
      weight -= 5;
    else if (CAN_WEAR(obj, ITEM_WEAR_BODY) || CAN_WEAR(obj, ITEM_HORSE_BODY) || CAN_WEAR(obj, ITEM_SPIDER_BODY))
      weight += 10;
    else if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
    {
      if (isname("spiked", obj->name))
      {
        cost += 500;
        weight += 1;
      }
      if (isname("large", obj->name) || isname("huge", obj->name))
      {
        cost += 100;
        weight += 5;
      }
      if (isname("small", obj->name) || isname("tiny", obj->name))
        weight -= 5;
    }

  /* check some keywords */
  if (isname("ornate", obj->name) || isname("gem", obj->name) ||
      isname("gold", obj->name) || isname("platinum", obj->name) ||
      isname("jewel", obj->name))
    cost *= 2;
  if (isname("worn", obj->name) || isname("broken", obj->name) ||
      isname("ruined", obj->name))
    cost /= 2;

  cost = BOUNDED(1, cost, 5000000);
  weight = BOUNDED(0, weight, 10000);

  /* slight randomizing */
  cost += ((cost / 100) * number(-2, 2));

/*  obj->weight = weight;*/
  if ((type != ITEM_INSTRUMENT) && (type != ITEM_TOTEM))
    obj->cost = cost;

/*  setItemMaxSP(obj);*/
#if 0
  obj->curr_sp = obj->max_sp;   /* temporary */
#endif
}
