/*
 * ***************************************************************************
 *   File: new_combat.c                                       Part of Duris
 *   Usage: Stuff related to new combat system
 *   Copyright  1990, 1991 - see 'license.doc' for complete information.
 *   Copyright  1994, 1995, 1997 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "justice.h"
#include "kingdom.h"
#include "new_combat.h"
#include "objmisc.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "siege.h"

extern const struct str_app_type str_app[52];
extern P_room world;
extern P_char combat_list;
extern P_char combat_next_ch;
extern const int top_of_world;
extern int body_dist_table[];
extern bodyLocInfo *physBodyLocTables[];
extern P_index obj_index;


#if 0
int pick_a_arm(P_char ch)
{
  if ((GET_RACE(ch) != RACE_THRIKREEN) || number(0, 1))
    return number(BODY_LOC_UPPER_LEFT_ARM, BODY_LOC_RIGHT_HAND);
  return number(BODY_LOC_UPPER_LEFT_ARM, BODY_LOC_LOWER_RIGHT_HAND);
}

int pick_a_head(P_char ch)
{
  if (number(0, 2))
    return number(BODY_LOC_HEAD, BODY_LOC_NECK);
  return number(BODY_LOC_CHIN, BODY_LOC_RIGHT_SHOULDER);
}

int pick_a_body(P_char ch)
{
  if (number(0, 3))
  {                             /* 25% we hit body area */
    return number(BODY_LOC_UPPER_TORSO, BODY_LOC_LOWER_TORSO);
  }
  if ((GET_RACE(ch) == RACE_CENTAUR) && number(0, 3))
    return number(BODY_LOC_FRONT_HORSE_BODY, BODY_LOC_REAR_HORSE_BODY);
  if (number(0, 1))
    return pick_a_head(ch);
  return pick_a_arm(ch);
}

int pick_a_leg(P_char ch)
{
  if (!number(0, 5))
    return pick_a_body(ch);
  if ((GET_RACE(ch) == RACE_CENTAUR) && !number(0, 1))
  {
    return number(BODY_LOC_LEFT_REAR_HOOF, BODY_LOC_UPPER_RIGHT_REAR_LEG);
  }
  return number(BODY_LOC_UPPER_LEFT_LEG, BODY_LOC_RIGHT_ANKLE);
}

int pick_a_limb(P_char ch)
{
  switch (number(0, 5))
  {
  case 0:
    return pick_a_body(ch);
    break;
  case 1:
    return pick_a_head(ch);
    break;
  case 2:
  case 3:
    return pick_a_arm(ch);
    break;
  case 4:
  case 5:
    return pick_a_leg(ch);
    break;
  }
  return BODY_LOC_UPPER_TORSO;
}

int pick_a_any(P_char ch)
{

  switch (number(0, 3))
  {
  case 0:
    return pick_a_head(ch);
    break;
  case 1:
    return pick_a_limb(ch);
    break;
  case 3:
  case 2:
    return pick_a_body(ch);
    break;
  }
  return BODY_LOC_UPPER_TORSO;
}
#endif

/*
 * calcChDamagetoVictwithInnateArmor : innate armor, baby.
 */

int calcChDamagetoVictwithInnateArmor(P_char ch, P_char victim, P_obj weap,
                                      const int dam, const int loc,
                                      const int specific_body_loc,
                                      int *damDefl, int *damAbsorb,
                                      int *innateArmorBlocks, int *weapDamage)
{
  int      new_dam = dam, mat = -1, special_armor = FALSE;
  char     strn[256];

  *innateArmorBlocks = FALSE;
  *damAbsorb = *damDefl = 0;

  if (!ch || !victim ||
      (dam <
       0)
      /*|| (body_loc < BODY_LOC_LOWEST) || (body_loc > BODY_LOC_HIGHEST) */ )
  {
    if (ch)
      send_to_char("no ch, victim, dam < 0, or loc out of range", ch);
    return 0;
  }
  if (dam == 0)
    return 0;

  if (GET_RACE(victim) == RACE_OGRE)
  {
    mat = MAT_CURED_LEATHER;
    special_armor = TRUE;
  }
  else if (IS_THRIKREEN(victim))
  {
    mat = MAT_CHITINOUS;
    special_armor = TRUE;
  }
  else if (GET_CLASS(victim) == CLASS_MONK)
  {
    special_armor = TRUE;
    if (GET_LEVEL(victim) > 50)
      mat = MAT_IRON;
    else if (GET_LEVEL(victim) > 40)
      mat = MAT_CURED_LEATHER;
    else if (GET_LEVEL(victim) > 30)
      mat = MAT_LEATHER;
    else if (GET_LEVEL(victim) > 20)
      mat = MAT_HIDE;
    else if (GET_LEVEL(victim) > 10)
      mat = MAT_BARK;
    else
      mat = MAT_FLESH;
  }
  else if (affected_by_spell(victim, SPELL_ARMOR))
    mat = MAT_HIDE;
  else if (affected_by_spell(victim, SPELL_BARKSKIN))
    mat = MAT_BARK;
  else if (affected_by_spell(victim, SPELL_SPIRIT_ARMOR))
    mat = MAT_HIDE;
  else if (affected_by_spell(victim, SPELL_ENHANCE_ARMOR))
    mat = MAT_SOFTWOOD;
  else if (affected_by_spell(victim, SPELL_FLESH_ARMOR))
    mat = MAT_FLESH;
  else if (affected_by_spell(victim, SONG_PROTECTION))
    mat = MAT_SOFTWOOD;

  if (mat == -1)
    return dam;

  // separate values for absorbtion/deflection do not matter, since no armor is
  // being damaged, but to emulate armor, use both values

  if (special_armor == TRUE)
  {
    *damDefl = (float) dam *getMaterialDeflection(mat, weap);
    *damAbsorb = (float) dam *getMaterialAbsorbtion(mat, weap);
  }

  if (affected_by_spell(victim, SPELL_ARMOR))
  {
    *damDefl += (float) dam *getMaterialDeflection(MAT_HIDE, weap);
    *damAbsorb += (float) dam *getMaterialAbsorbtion(MAT_HIDE, weap);
  }
  if (affected_by_spell(victim, SPELL_BARKSKIN))
  {
    *damDefl += (float) dam *getMaterialDeflection(MAT_BARK, weap);
    *damAbsorb += (float) dam *getMaterialAbsorbtion(MAT_BARK, weap);
  }
  if (affected_by_spell(victim, SPELL_SPIRIT_ARMOR))
  {
    *damDefl += (float) dam *getMaterialDeflection(MAT_HIDE, weap);
    *damAbsorb += (float) dam *getMaterialAbsorbtion(MAT_HIDE, weap);
  }
  if (affected_by_spell(victim, SPELL_ENHANCE_ARMOR))
  {
    *damDefl += (float) dam *getMaterialDeflection(MAT_SOFTWOOD, weap);
    *damAbsorb += (float) dam *getMaterialAbsorbtion(MAT_SOFTWOOD, weap);
  }
  if (affected_by_spell(victim, SPELL_FLESH_ARMOR))
  {
    *damDefl += (float) dam *getMaterialDeflection(MAT_FLESH, weap);
    *damAbsorb += (float) dam *getMaterialAbsorbtion(MAT_FLESH, weap);
  }
  if (affected_by_spell(victim, SONG_PROTECTION))
  {
    *damDefl += (float) dam *getMaterialDeflection(MAT_SOFTWOOD, weap);
    *damAbsorb += (float) dam *getMaterialAbsorbtion(MAT_SOFTWOOD, weap);
  }

  // higher level == more dam/defl - your skin gets harder or whatever kind
  // of bullshit you want

  if (GET_RACE(victim) == RACE_OGRE)
  {
    *damDefl += (float) dam *((float) GET_LEVEL(victim) / 400.0);       /* 25% extra at level 50 (total).. */
    *damAbsorb += (float) dam *((float) GET_LEVEL(victim) / 400.0);
  }
  else if (IS_THRIKREEN(victim))
  {
    *damDefl += (float) dam *((float) GET_LEVEL(victim) / 1000.0);      /* 10% extra at level 50 (total).. */
    *damAbsorb += (float) dam *((float) GET_LEVEL(victim) / 1000.0);
  }
  else if (GET_CLASS(victim) == CLASS_MONK)
  {
    *damDefl += (float) dam *((float) GET_LEVEL(victim) / 400.0);       /* 25% extra at level 50 (total).. */
    *damAbsorb += (float) dam *((float) GET_LEVEL(victim) / 400.0);
  }


  /* find a better way to calculate weapon damage */

  *weapDamage += *damAbsorb / 2;

  new_dam -= *damDefl + *damAbsorb;

  snprintf(strn, MAX_STRING_LENGTH, "(innate armor) defl: %d, absorb: %d, weapd: %d\r\n",
          *damDefl, *damAbsorb, *weapDamage);
  if (IS_TRUSTED(victim))
    send_to_char(strn, victim);
  if (IS_TRUSTED(ch))
    send_to_char(strn, ch);

  return new_dam;
}


/*
 * calcChDamagetoVictwithArmor : this function is called by calcChDamagetoVict
 *                               and calculates how much damage the victim
 *                               actually takes after taking the armor and
 *                               weapon into consideration
 */

int calcChDamagetoVictwithArmor(P_char ch, P_char victim, P_obj weap,
                                const int dam, const int loc,
                                const int specific_body_loc,
                                P_obj * armor_damaged, int *damDefl,
                                int *damAbsorb, int *armorBlocks,
                                int *weapDamage)
{
  int      new_dam = dam, physType;
  P_obj    armor = NULL;
  char     strn[256];

  *armorBlocks = FALSE;
  *damAbsorb = *damDefl = /**weapDamage =*/ 0;
  *armor_damaged = NULL;

  if (!ch || !victim ||
      (dam <
       0)
      /*|| (body_loc < BODY_LOC_LOWEST) || (body_loc > BODY_LOC_HIGHEST) */ )
  {
    if (ch)
      send_to_char("no ch, victim, dam < 0, or loc out of range", ch);
    return 0;
  }
  if (dam == 0)
    return 0;

  physType = GET_PHYS_TYPE(victim);

  if (bodyLocisUpperArms(physType, loc))
  {
    armor = victim->equipment[WEAR_ARMS];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_ARMS) &&
      ((armor->value[2] & ARMOR_ARMS_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else if (bodyLocisUpperWrists(physType, loc))
  {
    /* needs to be updated somehow to check phys .. */

    armor = (loc == BODY_LOC_HUMANOID_LEFT_WRIST ?
             victim->equipment[WEAR_WRIST_L] : victim->
             equipment[WEAR_WRIST_R]);
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_WRIST) &&
      ((armor->value[2] & ARMOR_WRIST_ALL) ||
       (armor->value[2] & specific_body_loc));

    // gloves can also protect wrist

    if (!*armorBlocks)
    {
      armor = victim->equipment[WEAR_HANDS];
      *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HANDS) &&
        ((armor->value[2] & ARMOR_HANDS_ALL) ||
         (armor->value[2] & ARMOR_HANDS_WRIST));
    }
  }
  else if (bodyLocisUpperHands(physType, loc))
  {
    armor = victim->equipment[WEAR_HANDS];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HANDS) &&
      ((armor->value[2] & ARMOR_HANDS_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else
    /* check both because specific_body_loc will be different if chin */

  if (bodyLocisHead(physType, loc) || bodyLocisChin(physType, loc))
  {
    armor = victim->equipment[WEAR_HEAD];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HEAD) &&
      ((armor->value[2] & ARMOR_HEAD_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else if (bodyLocisLeftEye(physType, loc))
  {
    armor = victim->equipment[WEAR_EYES];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_EYES) &&
      ((armor->value[2] & ARMOR_EYE_LEFT) ||
       (armor->value[2] & ARMOR_EYE_LEFT_TRANSPARENT));
  }
  else if (bodyLocisRightEye(physType, loc))
  {
    armor = victim->equipment[WEAR_EYES];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_EYES) &&
      ((armor->value[2] & ARMOR_EYE_RIGHT) ||
       (armor->value[2] & ARMOR_EYE_RIGHT_TRANSPARENT));
  }
  else if (bodyLocisEar(physType, loc))
  {
    armor = victim->equipment[WEAR_HEAD];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HEAD) &&
      ((armor->value[2] & ARMOR_HEAD_ALL) ||
       (armor->value[2] & ARMOR_HEAD_SIDES_UPPER));

  }
  else if (bodyLocisNeck(physType, loc))
  {
    armor = victim->equipment[WEAR_NECK_1];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_NECK) &&
      ((armor->value[2] & ARMOR_NECK_ALL) ||
       (armor->value[2] & specific_body_loc));

    if (!*armorBlocks)
    {
      armor = victim->equipment[WEAR_NECK_2];
      *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_NECK) &&
        ((armor->value[2] & ARMOR_NECK_ALL) ||
         (armor->value[2] & specific_body_loc));
    }
  }
  else if (bodyLocisUpperTorso(physType, loc) ||
           bodyLocisLowerTorso(physType, loc) ||
           bodyLocisUpperShoulders(physType, loc))
  {
    armor = victim->equipment[WEAR_BODY];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_BODY) &&
      ((armor->value[2] & ARMOR_BODY_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else if (bodyLocisLegs(physType, loc))
  {
    armor = victim->equipment[WEAR_LEGS];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_LEGS) &&
      ((armor->value[2] & ARMOR_LEGS_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else if (bodyLocisFeet(physType, loc))
  {
    armor = victim->equipment[WEAR_FEET];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_FEET) &&
      ((armor->value[2] & ARMOR_FEET_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else if (bodyLocisLowerArms(physType, loc))
  {
    armor = victim->equipment[WEAR_ARMS_2];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_ARMS) &&
      ((armor->value[2] & ARMOR_ARMS_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else if (bodyLocisLowerWrists(physType, loc))
  {

    /* needs to be updated to check phys .. */

    armor = (loc == BODY_LOC_HUMANOID_LEFT_WRIST ?
             victim->equipment[WEAR_WRIST_LL] : victim->
             equipment[WEAR_WRIST_LR]);
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_WRIST) &&
      ((armor->value[2] & ARMOR_WRIST_ALL) ||
       (armor->value[2] & specific_body_loc));

    // gloves can also protect wrist

    if (!*armorBlocks)
    {
      armor = victim->equipment[WEAR_HANDS_2];
      *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HANDS) &&
        ((armor->value[2] & ARMOR_HANDS_ALL) ||
         (armor->value[2] & ARMOR_HANDS_WRIST));
    }
  }
  else if (bodyLocisLowerHands(physType, loc))
  {
    armor = victim->equipment[WEAR_HANDS];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HANDS) &&
      ((armor->value[2] & ARMOR_HANDS_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else if (bodyLocisHorseBody(physType, loc))
  {
    armor = victim->equipment[WEAR_HORSE_BODY];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HORSEBODY) &&
      ((armor->value[2] & ARMOR_HORSE_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else if (bodyLocisRearLegs(physType, loc))
  {
    armor = victim->equipment[WEAR_LEGS_REAR];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_LEGS) &&
      ((armor->value[2] & ARMOR_LEGS_ALL) ||
       (armor->value[2] & specific_body_loc));
  }
  else if (bodyLocisRearFeet(physType, loc))
  {
    armor = victim->equipment[WEAR_FEET_REAR];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_FEET) &&
      ((armor->value[2] & ARMOR_FEET_ALL) ||
       (armor->value[2] & specific_body_loc));
  }

#if 0
  switch (body_loc)
  {
  case BODY_LOC_UPPER_RIGHT_ARM:
  case BODY_LOC_UPPER_LEFT_ARM:
  case BODY_LOC_LOWER_RIGHT_ARM:
  case BODY_LOC_LOWER_LEFT_ARM:
  case BODY_LOC_RIGHT_ELBOW:
  case BODY_LOC_LEFT_ELBOW:
    armor = victim->equipment[WEAR_ARMS];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_ARMS) &&
      ((armor->value[2] & ARMOR_ARMS_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

  case BODY_LOC_LEFT_WRIST:
  case BODY_LOC_RIGHT_WRIST:
    armor = (body_loc == BODY_LOC_LEFT_WRIST ?
             victim->equipment[WEAR_WRIST_L] : victim->
             equipment[WEAR_WRIST_R]);
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_WRIST) &&
      ((armor->value[2] & ARMOR_WRIST_ALL) ||
       (armor->value[2] & specific_body_loc));

    // gloves can also protect wrist

    if (!*armorBlocks)
    {
      armor = victim->equipment[WEAR_HANDS];
      *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HANDS) &&
        ((armor->value[2] & ARMOR_HANDS_ALL) ||
         (armor->value[2] & ARMOR_HANDS_WRIST));
    }
    break;

  case BODY_LOC_RIGHT_HAND:
  case BODY_LOC_LEFT_HAND:
    armor = victim->equipment[WEAR_HANDS];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HANDS) &&
      ((armor->value[2] & ARMOR_HANDS_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

  case BODY_LOC_HEAD:
    armor = victim->equipment[WEAR_HEAD];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HEAD) &&
      ((armor->value[2] & ARMOR_HEAD_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

  case BODY_LOC_CHIN:
    armor = victim->equipment[WEAR_HEAD];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HEAD) &&
      ((armor->value[2] & ARMOR_HEAD_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

  case BODY_LOC_LEFT_EYE:
    armor = victim->equipment[WEAR_EYES];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_EYES) &&
      ((armor->value[2] & ARMOR_EYE_LEFT) ||
       (armor->value[2] & ARMOR_EYE_LEFT_TRANSPARENT));
    break;

  case BODY_LOC_RIGHT_EYE:
    armor = victim->equipment[WEAR_EYES];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_EYES) &&
      ((armor->value[2] & ARMOR_EYE_RIGHT) ||
       (armor->value[2] & ARMOR_EYE_RIGHT_TRANSPARENT));
    break;

    // head armor protects ears
  case BODY_LOC_LEFT_EAR:
  case BODY_LOC_RIGHT_EAR:
    armor = victim->equipment[WEAR_HEAD];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HEAD) &&
      ((armor->value[2] & ARMOR_HEAD_ALL) ||
       (armor->value[2] & ARMOR_HEAD_SIDES_UPPER));
    break;

  case BODY_LOC_NECK:
    armor = victim->equipment[WEAR_NECK_1];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_NECK) &&
      ((armor->value[2] & ARMOR_NECK_ALL) ||
       (armor->value[2] & specific_body_loc));

    if (!*armorBlocks)
    {
      armor = victim->equipment[WEAR_NECK_2];
      *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_NECK) &&
        ((armor->value[2] & ARMOR_NECK_ALL) ||
         (armor->value[2] & specific_body_loc));
    }
    break;

  case BODY_LOC_UPPER_TORSO:
  case BODY_LOC_LOWER_TORSO:
  case BODY_LOC_LEFT_SHOULDER:
  case BODY_LOC_RIGHT_SHOULDER:
    armor = victim->equipment[WEAR_BODY];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_BODY) &&
      ((armor->value[2] & ARMOR_BODY_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

  case BODY_LOC_UPPER_LEFT_LEG:
  case BODY_LOC_LOWER_LEFT_LEG:
  case BODY_LOC_LEFT_KNEE:
  case BODY_LOC_UPPER_RIGHT_LEG:
  case BODY_LOC_LOWER_RIGHT_LEG:
  case BODY_LOC_RIGHT_KNEE:
    armor = victim->equipment[WEAR_LEGS];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_LEGS) &&
      ((armor->value[2] & ARMOR_LEGS_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

  case BODY_LOC_LEFT_ANKLE:
  case BODY_LOC_LEFT_FOOT:
  case BODY_LOC_RIGHT_ANKLE:
  case BODY_LOC_RIGHT_FOOT:
    armor = victim->equipment[WEAR_FEET];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_FEET) &&
      ((armor->value[2] & ARMOR_FEET_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

    // thrikreen crap

  case BODY_LOC_UPPER_LEFT_LOWER_ARM:
  case BODY_LOC_LOWER_LEFT_LOWER_ARM:
  case BODY_LOC_LOWER_LEFT_ELBOW:
  case BODY_LOC_UPPER_RIGHT_LOWER_ARM:
  case BODY_LOC_LOWER_RIGHT_LOWER_ARM:
  case BODY_LOC_LOWER_RIGHT_ELBOW:
    armor = victim->equipment[WEAR_ARMS_2];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_ARMS) &&
      ((armor->value[2] & ARMOR_ARMS_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

  case BODY_LOC_LOWER_LEFT_WRIST:
  case BODY_LOC_LOWER_RIGHT_WRIST:
    armor = (body_loc == BODY_LOC_LEFT_WRIST ?
             victim->equipment[WEAR_WRIST_LL] : victim->
             equipment[WEAR_WRIST_LR]);
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_WRIST) &&
      ((armor->value[2] & ARMOR_WRIST_ALL) ||
       (armor->value[2] & specific_body_loc));

    // gloves can also protect wrist

    if (!*armorBlocks)
    {
      armor = victim->equipment[WEAR_HANDS_2];
      *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HANDS) &&
        ((armor->value[2] & ARMOR_HANDS_ALL) ||
         (armor->value[2] & ARMOR_HANDS_WRIST));
    }
    break;

  case BODY_LOC_LOWER_LEFT_HAND:
  case BODY_LOC_LOWER_RIGHT_HAND:
    armor = victim->equipment[WEAR_HANDS];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HANDS) &&
      ((armor->value[2] & ARMOR_HANDS_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;


    // centaur/horse crap

  case BODY_LOC_FRONT_HORSE_BODY:
  case BODY_LOC_REAR_HORSE_BODY:
    armor = victim->equipment[WEAR_HORSE_BODY];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_HORSEBODY) &&
      ((armor->value[2] & ARMOR_HORSE_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

  case BODY_LOC_UPPER_LEFT_REAR_LEG:
  case BODY_LOC_LOWER_LEFT_REAR_LEG:
  case BODY_LOC_UPPER_RIGHT_REAR_LEG:
  case BODY_LOC_LOWER_RIGHT_REAR_LEG:
  case BODY_LOC_LEFT_REAR_KNEE:
  case BODY_LOC_RIGHT_REAR_KNEE:
    armor = victim->equipment[WEAR_LEGS_REAR];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_LEGS) &&
      ((armor->value[2] & ARMOR_LEGS_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;


  case BODY_LOC_LEFT_REAR_HOOF:
  case BODY_LOC_RIGHT_REAR_HOOF:
  case BODY_LOC_LEFT_REAR_ANKLE:
  case BODY_LOC_RIGHT_REAR_ANKLE:
    armor = victim->equipment[WEAR_FEET_REAR];
    *armorBlocks = armor && (armor->value[1] == ARMOR_WEAR_FEET) &&
      ((armor->value[2] & ARMOR_FEET_ALL) ||
       (armor->value[2] & specific_body_loc));
    break;

  default:
    send_to_char("error in victim's armor damage\r\n", ch);
    send_to_char("error in your armor damage\r\n", victim);
    break;
  }
#endif

  if (*armorBlocks)
  {
    *damDefl = (float) dam *getArmorDeflection(armor, weap);
    *damAbsorb = (float) dam *getArmorAbsorbtion(armor, weap);

    *armor_damaged = armor;

    if (IS_RIDING(victim) && !IS_RIDING(ch) &&
        ((GET_CLASS(victim) == CLASS_PALADIN) ||
         (GET_CLASS(victim) == CLASS_ANTIPALADIN)))
    {
      if (GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT) < number(1, 101))
        *damDefl += 0.1;
      if (IS_PC(ch) && !number(0, 1))
        notch_skill(ch, SKILL_MOUNTED_COMBAT, 20);
    }

    /* find a better way to calculate weapon damage */

    *weapDamage += *damAbsorb / 2;

    new_dam -= *damDefl + *damAbsorb;

    snprintf(strn, MAX_STRING_LENGTH, "defl: %d, absorb: %d, weapd: %d\r\n", *damDefl, *damAbsorb,
            *weapDamage);
    if (IS_TRUSTED(victim))
      send_to_char(strn, victim);
    if (IS_TRUSTED(ch))
      send_to_char(strn, ch);
  }
  else if (IS_TRUSTED(victim))
    send_to_char("armor doesn't block\r\n", victim);

  return new_dam;
}


/*
 * displayWeaponDamage
 */

void displayWeaponDamage(const int weap_type, const P_char ch,
                         const P_obj object)
{
  char     damstrn[64], actstrn[256];

  strcpy(damstrn, " is buggy");

  switch (weap_type)
  {
  case WEAPON_SICKLE:
  case WEAPON_AXE:
    switch (number(0, 1))
    {
    case 0:
      strcpy(damstrn, " snaps at the handle");
      break;
    case 1:
      strcpy(damstrn, "'s blade shatters");
      break;
    }

    break;

  case WEAPON_DAGGER:
    switch (number(0, 1))
    {
    case 0:
      strcpy(damstrn, "'s blade snaps");
      break;
    case 1:
      strcpy(damstrn, "'s blade shatters");
      break;
    }

    break;

  case WEAPON_WHIP:
  case WEAPON_FLAIL:
    strcpy(damstrn, " snaps");
    break;

  case WEAPON_HAMMER:
    switch (number(0, 1))
    {
    case 0:
      strcpy(damstrn, " snaps at the handle");
      break;
    case 1:
      strcpy(damstrn, "'s head shatters");
      break;
    }

    break;

  case WEAPON_LONGSWORD:
  case WEAPON_SHORTSWORD:
  case WEAPON_2HANDSWORD:
    switch (number(0, 2))
    {
    case 0:
      strcpy(damstrn, " snaps at the hilt");
      break;
    case 1:
      strcpy(damstrn, " shatters to pieces");
      break;
    case 2:
      strcpy(damstrn, "'s blade snaps");
      break;
    }

    break;

  case WEAPON_MACE:
  case WEAPON_SPIKED_MACE:
    switch (number(0, 1))
    {
    case 0:
      strcpy(damstrn, "'s handle snaps in two");
      break;
    case 1:
      strcpy(damstrn, " head shatters");
      break;
    }

    break;

  case WEAPON_POLEARM:
    switch (number(0, 1))
    {
    case 0:
      strcpy(damstrn, " snaps at the handle");
      break;
    case 1:
      strcpy(damstrn, "'s tip shatters");
      break;
    }

    break;

  case WEAPON_CLUB:
  case WEAPON_SPIKED_CLUB:
  case WEAPON_LANCE:
  case WEAPON_STAFF:
    strcpy(damstrn, " breaks in two");
    break;

  case WEAPON_PICK:
    switch (number(0, 1))
    {
    case 0:
      strcpy(damstrn, " snaps at the handle");
      break;
    case 1:
      strcpy(damstrn, " snaps at the tip");
      break;
    }

    break;
  }

  snprintf(actstrn, MAX_STRING_LENGTH, "Your $q%s, rendering it useless.", damstrn);
  act(actstrn, TRUE, ch, object, 0, TO_CHAR);

  snprintf(actstrn, MAX_STRING_LENGTH, "$n's $q%s, rendering it useless.", damstrn);
  act(actstrn, TRUE, ch, object, 0, TO_ROOM);
}


/*
 * applyDamagetoObject : applies damage to an object, perhaps destroying it..
 *                       takes into account craftsmanship and whatever else makes
 *                       sense
 *
 *                       returns TRUE if the object is destroyed (this function
 *                       handles the extract_obj() call)
 */

int applyDamagetoObject(P_char ch, P_obj object, const unsigned int dam)
{
  int      craft, lowest_sp, below_lowest, percent_below, is_weapon,
    weap_type;
/*  char damstrn[64], actstrn[256]; */

  if( !object || !dam || IS_ARTIFACT(object) )
    return FALSE;

  craft = BOUNDED(OBJCRAFT_LOWEST, object->craftsmanship, OBJCRAFT_HIGHEST);
  is_weapon = (GET_ITEM_TYPE(object) == ITEM_WEAPON);
  if (is_weapon)
    weap_type = object->value[0];

  /* lowest sp before catastrophic failure [total destruction] of object is based
     entirely on craftsmanship */

  /* object of worst craftsmanship will start to fail at 45% of max sp.  objects of
     best craftsmanship won't fail until they're at 0 SP */

  lowest_sp = ((OBJCRAFT_HIGHEST - craft) * .03) * object->max_sp;

  object->curr_sp -= (MAX(1, (int) (dam / 10)));

  /* nothing is gonna survive 0 or less */

  /* different messages based on whether it's a weapon or armor/other */

  if (object->curr_sp <= 0)
  {
    if (is_weapon)
    {
      displayWeaponDamage(weap_type, ch, object);
    }
    else
    {
      act("$n's $q is utterly destroyed!", TRUE, ch, object, 0, TO_ROOM);
      act("Your $q is destroyed!", TRUE, ch, object, 0, TO_CHAR);
    }

    extract_obj(object);

    return TRUE;
  }
  if ( /*(lowest_sp <= 0) || */ (object->curr_sp >= lowest_sp))
    return FALSE;

  /* random chance based on how far below lowest_sp it is */

  below_lowest = lowest_sp - object->curr_sp;
  percent_below = (below_lowest / lowest_sp) * 100;

  /* hope ya don't get unlucky, pal */

  if (number(1, 100) < percent_below)
  {
    if (is_weapon)
    {
      displayWeaponDamage(weap_type, ch, object);
    }
    else
    {
      act("$n's $q falls apart, becoming a pile of useless scraps.", TRUE, ch,
          object, 0, TO_ROOM);
      act("Your $q falls apart, becoming a pile of useless scraps.", TRUE, ch,
          object, 0, TO_CHAR);
    }

    extract_obj(object);

    return TRUE;
  }
  return FALSE;
}


/*
 * victParry : echo stuff about parrying
 */

void victParry(const P_char ch, const P_char victim, P_obj weapon,
               const int body_loc_target, const int parryrand,
               const int chance)
{
  int      passedby = chance - parryrand, weaptype;
  char     notvictmsg[MAX_STRING_LENGTH], charmsg[MAX_STRING_LENGTH],
    victmsg[MAX_STRING_LENGTH];

  if (!ch || !victim || !weapon)
    return;

  weaptype = weapon->value[0];

  snprintf(notvictmsg, MAX_STRING_LENGTH, "$N %s a %s at $S %s from $n.",
          getParryEaseString(passedby, FALSE),
          getWeaponUseString(weaptype),
          getBodyLocStrn(body_loc_target, victim));

  snprintf(charmsg, MAX_STRING_LENGTH, "$N %s your %s at $S %s.",
          getParryEaseString(passedby, FALSE), getWeaponUseString(weaptype),
          getBodyLocStrn(body_loc_target, victim));

  snprintf(victmsg, MAX_STRING_LENGTH, "You %s a %s from $n at your %s.",
          getParryEaseString(passedby, TRUE), getWeaponUseString(weaptype),
          getBodyLocStrn(body_loc_target, victim));

  act(notvictmsg, FALSE, ch, 0, victim, TO_NOTVICT);
  act(charmsg, FALSE, ch, 0, victim, TO_CHAR);
  act(victmsg, FALSE, ch, 0, victim, TO_VICT);

  // start people fighting as they should ..

  damage(ch, victim, 0, TYPE_UNDEFINED);
}




/*
 * victDodge : echo stuff about dodging
 */

void victDodge(const P_char ch, const P_char victim, const int weaptype,
               const int body_loc_target, const int dodgerand,
               const int chance)
{
  int      passedby = chance - dodgerand;
  char     notvictmsg[MAX_STRING_LENGTH], charmsg[MAX_STRING_LENGTH],
    victmsg[MAX_STRING_LENGTH];

  if (!ch || !victim)
    return;


  snprintf(notvictmsg, MAX_STRING_LENGTH, "$N %s a %s at $S %s from $n.",
          getDodgeEaseString(passedby, FALSE),
          getWeaponUseString(weaptype),
          getBodyLocStrn(body_loc_target, victim));

  snprintf(charmsg, MAX_STRING_LENGTH, "$N %s your %s at $S %s.",
          getDodgeEaseString(passedby, FALSE), getWeaponUseString(weaptype),
          getBodyLocStrn(body_loc_target, victim));

  snprintf(victmsg, MAX_STRING_LENGTH, "You %s a %s from $n at your %s.",
          getDodgeEaseString(passedby, TRUE), getWeaponUseString(weaptype),
          getBodyLocStrn(body_loc_target, victim));

  act(notvictmsg, FALSE, ch, 0, victim, TO_NOTVICT);
  act(charmsg, FALSE, ch, 0, victim, TO_CHAR);
  act(victmsg, FALSE, ch, 0, victim, TO_VICT);

  damage(ch, victim, 0, TYPE_UNDEFINED);
}


/*
 * getBodypartWeight
 */

int getBodypartWeight(const P_char vict, const int loc)
{
  return 5;                     // temporary ......

}


/*
 * createBodypartinRoom
 */

void createBodypartinRoom(const int room, const int loc, const char *bodypart,
                          const P_char vict)
{
  P_obj    part;
  char     strn[1024];
  const char *charname;

  if (!vict || (room < 0) || (room > top_of_world) || !bodypart ||
      !bodypart[0])
  {
    wizlog(56, "some bug in createBodypartinRoom..");
    return;
  }
  charname = J_NAME(vict);

  part = read_object(OBJ_BODYPART_VNUM, VIRTUAL);
  if (!part)
  {
    wizlog(56, "couldn't load obj vnum #%d for bodypart", OBJ_BODYPART_VNUM);
    return;
  }
  part->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3);

  snprintf(strn, 1024, "%s %s", bodypart, GET_NAME(vict));
  part->name = str_dup(strn);

  snprintf(strn, 1024, "The %s of %s is lying here.", bodypart, charname);
  part->description = str_dup(strn);

  snprintf(strn, 1024, "the %s of %s", bodypart, charname);
  part->short_description = str_dup(strn);

  snprintf(strn, 1024, "%s", bodypart);
  part->action_description = str_dup(strn);

  part->weight = getBodypartWeight(vict, loc);

  // later will add stuff related to actual part

  SET_BIT(part->wear_flags, ITEM_TAKE);
  SET_BIT(part->wear_flags, ITEM_HOLD);

  obj_to_room(part, room);
}


/*
 * victLostLowerArm : wuh-oh!
 */

#if 0
int victLostLowerArm(P_char victim, const int loc)
{
  const char *locstrn;
  char     buf[1024];
  int      weappos, heldpos, remshield;


  locstrn = getBodyLocStrn(loc, victim);

  createBodypartinRoom(victim->in_room, loc, locstrn, victim);

  snprintf(buf, 1024, "$n's %s is destroyed and falls to the ground, useless.",
          locstrn);

  act(buf, FALSE, victim, 0, 0, TO_ROOM);

  snprintf(buf, 1024, "Your %s is destroyed and falls to the ground, useless.",
          locstrn);

  act(buf, FALSE, victim, 0, 0, TO_CHAR);

  victim->points.location_hit[loc] = BODYPART_GONE_VAL;

  switch (loc)
  {
  case BODY_LOC_LOWER_LEFT_ARM:
    victim->points.location_hit[BODY_LOC_LEFT_WRIST] =
      victim->points.location_hit[BODY_LOC_LEFT_HAND] = BODYPART_GONE_VAL;
    break;

  case BODY_LOC_LOWER_RIGHT_ARM:
    victim->points.location_hit[BODY_LOC_RIGHT_WRIST] =
      victim->points.location_hit[BODY_LOC_RIGHT_HAND] = BODYPART_GONE_VAL;
    break;

  case BODY_LOC_LOWER_LEFT_LOWER_ARM:
    victim->points.location_hit[BODY_LOC_LOWER_LEFT_WRIST] =
      victim->points.location_hit[BODY_LOC_LOWER_LEFT_HAND] =
      BODYPART_GONE_VAL;
    break;

  case BODY_LOC_LOWER_RIGHT_LOWER_ARM:
    victim->points.location_hit[BODY_LOC_LOWER_RIGHT_WRIST] =
      victim->points.location_hit[BODY_LOC_LOWER_RIGHT_HAND] =
      BODYPART_GONE_VAL;
    break;

  default:
    send_to_char("error in victLostArm()\r\n", victim);
    return FALSE;
  }

  // (upper) right arm is primary, (upper) left arm is secondary, lower right arm
  // is tertiary, lower left arm is quatiary (or whatever)

  // shield is always in lowest left arm - if we ever add handedness, we can check
  // for it more accurately, but as of right now everyone is right-handed

  heldpos = HOLD;

  switch (loc)
  {
  case BODY_LOC_LOWER_RIGHT_ARM:
    weappos = WIELD;
    remshield = FALSE;

    break;

  case BODY_LOC_LOWER_LEFT_ARM:
    weappos = WIELD2;

    if (!IS_THRIKREEN(victim))
      remshield = TRUE;
    else
      remshield = FALSE;

    break;

  case BODY_LOC_LOWER_RIGHT_LOWER_ARM:
    weappos = WIELD3;
    remshield = TRUE;

    break;

  case BODY_LOC_LOWER_LEFT_LOWER_ARM:
    weappos = WIELD4;
    remshield = FALSE;

    break;

    // check for invalid loc done above
  }

  if (victim->equipment[weappos])
    obj_to_room(unequip_char(victim, weappos), victim->in_room);

  if (victim->equipment[heldpos])
    obj_to_room(unequip_char(victim, heldpos), victim->in_room);

  if (remshield && victim->equipment[WEAR_SHIELD])
    obj_to_room(unequip_char(victim, WEAR_SHIELD), victim->in_room);

  return FALSE;
}
#endif


/*
 * checkEffectsofLocDamage
 */

int checkEffectsofLocDamage(P_char ch, P_char victim, const int loc,
                            const int dam)
{
  int      currloc_hp, predamloc_hp, maxloc_hp, phys_type, i;

  if (!victim)
    return TRUE;

  /*
     if ( (loc < BODY_LOC_LOWEST) || (loc > BODY_LOC_HIGHEST) ||  (dam <= 0)) {
     send_to_char("error in checkEffectsofLocDamage(): loc/dam is out of range", victim);

     return TRUE;
     } */
/*
  if (dam <=0)
    dam = 0;
*/
  // location is catastrophically fucked up once it drops to -10 or below, and is
  // not well off at below 0

  phys_type = GET_PHYS_TYPE(victim);

  maxloc_hp = getBodyLocMaxHP(victim, loc);
  currloc_hp = maxloc_hp - victim->points.location_hit[loc];
  predamloc_hp = currloc_hp + dam;

  if (predamloc_hp > maxloc_hp + 10)
    send_to_char("nuttiness in checkEffectsofLocDamage()..\r\n", victim);

  if (currloc_hp <= -10)
  {
    if ((physBodyLocTables[phys_type])[loc].
        bodyLocDestroyed(ch, victim, loc, dam))
      return TRUE;

    for (i = 0;
         (i < MAX_DEPENDENT_LOCS) &&
         (physBodyLocTables[phys_type])[loc].dependentBodyLocs[i]; i++)
    {
      victim->points.location_hit[(physBodyLocTables[phys_type])[loc].
                                  dependentBodyLocs[i]] = BODYPART_GONE_VAL;
    }

    return FALSE;
  }
  else if (currloc_hp < 0)
  {
    return (physBodyLocTables[phys_type])[loc].bodyLocHit(ch, victim, loc,
                                                          dam);
  }
  return FALSE;
}


/*
 * victDamage : this function applies the damage to the victim in the
 *              appropriate area, checking for effects of getting hit in
 *              that area at the same time
 *
 *              returns TRUE if victim dies
 */

int victDamage(P_char ch, P_char victim, const int barehanded,
               const int weaptype, const int dam, const int loc)
{
  char     notvictmsg[MAX_STRING_LENGTH], charmsg[MAX_STRING_LENGTH],
    victmsg[MAX_STRING_LENGTH];
  unsigned int maxhp;
  P_obj    wield;
  int      w_percent, h_percent, max_dam = 0, w_loop, h_loop;
  static int dam_ref[] = { 0, 2, 7, 10, 15, 25, 40, 55, 70, 85, 9999 };
  const char *weapon_damage[] = {
    "",
    " laughable",
    " feeble",
    " lame",
    " mediocre",
    " fine",
    " powerful",
    " mighty",
    " awesome",
    " devastating",
    " godly"
  };
  const char *victim_damage[] = {
    "grazes",
    "grazes",
    "wounds",
    "hits",
    "hits",
    "hits",
    "seriously wounds",
    "beats the crap out of",
    "nearly slaughters",
    "absolutely destroys",
    "hits"
  };

  if (!ch || !victim)
    return TRUE;
#if 0
  if ( /*(loc < BODY_LOC_LOWEST) || (loc > BODY_LOC_HIGHEST) || */ (dam <= 0))
  {
    send_to_char("error in victDamage(): loc/dam is out of range", ch);
    send_to_char("error in victDamage(): loc/dam is out of range", victim);

    return TRUE;
  }
#endif
/*
  if (IS_AFFECTED(victim, AFF_STONE_SKIN))
     dam = 1;
*/
  /* calc weapon damage message & victim damage message */
  if (ch->equipment[WIELD])
  {
    wield = ch->equipment[WIELD];
    max_dam = wield->value[1] * wield->value[2];
  }
  h_percent =
    BOUNDED(0, (int) ((dam * 100) / (GET_HIT(victim) + dam + 10)), 100);
  w_percent = BOUNDED(0, (int) ((dam * 100) / (max_dam + dam + 10)), 100);
  for (h_loop = 0; (h_percent > dam_ref[h_loop]); h_loop++) ;
  if (h_loop > 10)
    h_loop = 10;
  for (w_loop = 0; (w_percent > dam_ref[w_loop]); w_loop++) ;
  if (w_loop > 10)
    w_loop = 0;

  if (!barehanded)
  {
    snprintf(notvictmsg, MAX_STRING_LENGTH, "$n's%s %s %s $N in the %s.",
            weapon_damage[w_loop], getWeaponHitVerb(weaptype, TRUE),
            victim_damage[h_loop], getBodyLocStrn(loc, victim));
    snprintf(charmsg, MAX_STRING_LENGTH, "Your%s %s %s $N in the %s.", weapon_damage[w_loop],
            getWeaponHitVerb(weaptype, TRUE), victim_damage[h_loop],
            getBodyLocStrn(loc, victim));
    snprintf(victmsg, MAX_STRING_LENGTH, "$n's%s %s %s you in the %s.", weapon_damage[w_loop],
            getWeaponHitVerb(weaptype, TRUE), victim_damage[h_loop],
            getBodyLocStrn(loc, victim));
  }
  else
  {
    snprintf(notvictmsg, MAX_STRING_LENGTH, "$n's punch %s $N in the %s.",
            victim_damage[h_loop], getBodyLocStrn(loc, victim));
    snprintf(charmsg, MAX_STRING_LENGTH, "Your punch %s $N in the %s.",
            victim_damage[h_loop], getBodyLocStrn(loc, victim));
    snprintf(victmsg, MAX_STRING_LENGTH, "$n punches %s you in the %s.",
            victim_damage[h_loop], getBodyLocStrn(loc, victim));
  }

  act(notvictmsg, FALSE, ch, 0, victim, TO_NOTVICT);
  act(charmsg, FALSE, ch, 0, victim, TO_CHAR);
  act(victmsg, FALSE, ch, 0, victim, TO_VICT);

  // first, apply damage to overall hitpoints; then apply damage to
  // individual area and check for effects

  if (damage(ch, victim, dam, TYPE_UNDEFINED))
    return TRUE;

  // location_hit[] array stores damage done to that area - it won't ever
  // go over the max hp for that location

  if (victim->points.location_hit[loc] != BODYPART_GONE_VAL)
  {
    victim->points.location_hit[loc] += dam;

    maxhp = getBodyLocMaxHP(victim, loc);
    if (victim->points.location_hit[loc] > maxhp + 10)
      victim->points.location_hit[loc] = maxhp + 10;

    // victim can also die from location damage

    if (checkEffectsofLocDamage(ch, victim, loc, dam))
      return TRUE;
  }
  else
    send_to_char("curious, you hit a location that no longer exists..\r\n",
                 ch);

  return FALSE;
}


/*
 * victMiss
 */

void victMiss(const P_char ch, const P_char victim, const int weaptype,
              const int loc, const int barehanded)
{
  char     notvictmsg[MAX_STRING_LENGTH], charmsg[MAX_STRING_LENGTH],
    victmsg[MAX_STRING_LENGTH];

  if (!ch || !victim)
    return;
/*
   if ((loc < BODY_LOC_LOWEST) || (loc > BODY_LOC_HIGHEST))
   {
   send_to_char("error in victMiss(): loc is out of range", ch);
   send_to_char("error in victMiss(): loc is out of range", victim);

   return;
   }
 */

  if (!barehanded)
  {
    snprintf(notvictmsg, MAX_STRING_LENGTH, "$n misses $N with $s %s at $S %s.",
            getWeaponUseString(weaptype), getBodyLocStrn(loc, victim));
    snprintf(charmsg, MAX_STRING_LENGTH, "You miss $N with your %s at $S %s.",
            getWeaponUseString(weaptype), getBodyLocStrn(loc, victim));
    snprintf(victmsg, MAX_STRING_LENGTH, "$n misses you with $s %s at your %s.",
            getWeaponUseString(weaptype), getBodyLocStrn(loc, victim));
  }
  else
  {
    snprintf(notvictmsg, MAX_STRING_LENGTH, "$n misses $N with $s %s at $S %s.",
            "punch", getBodyLocStrn(loc, victim));
    snprintf(charmsg, MAX_STRING_LENGTH, "You miss $N with your %s at $S %s.",
            "punch", getBodyLocStrn(loc, victim));
    snprintf(victmsg, MAX_STRING_LENGTH, "$n misses you with $s %s at your %s.",
            "punch", getBodyLocStrn(loc, victim));
  }

  act(notvictmsg, FALSE, ch, 0, victim, TO_NOTVICT);
  act(charmsg, FALSE, ch, 0, victim, TO_CHAR);
  act(victmsg, FALSE, ch, 0, victim, TO_VICT);

  damage(ch, victim, 0, TYPE_UNDEFINED);        // start fighting, other crap

}


/*
 * getBodyTarget - search hook Tavril
 */

int getBodyTarget(const P_char ch)
{
  /* temporary */
/*
   return number(BODY_LOC_LOWEST, BODY_LOC_HIGHEST_HUMANOID);
 */
/*
   return BODY_LOC_LOWER_RIGHT_ARM;
   return BODY_LOC_UPPER_TORSO;
 */
#if 0
  if (IS_NPC(ch))
  {                             /* what will mobs target ? */
    return pick_a_any(ch);
  }
  else
    switch (ch->player.combat_target_loc)
    {
    case 0:
      return pick_a_any(ch);
      break;
    case 1:
      return pick_a_head(ch);
      break;
    case 2:
      return pick_a_body(ch);
      break;
    case 3:
      return pick_a_limb(ch);
      break;
    case 4:
      return pick_a_arm(ch);
      break;
    case 5:
      return pick_a_leg(ch);
      break;
    default:
      return pick_a_body(ch);
    }
#endif
  /* yet more temporary code */

  if (IS_NPC(ch))
    return BODY_LOC_HUMANOID_UPPER_TORSO;

  switch (ch->player.combat_target_loc)
  {
  case 0:
    return number(BODY_LOC_LOWEST_HUMANOID, BODY_LOC_HIGHEST_HUMANOID);
  case 1:
    return number(BODY_LOC_HUMANOID_HEAD, BODY_LOC_HUMANOID_RIGHT_EAR);
  default:
  case 2:
    return number(BODY_LOC_HUMANOID_UPPER_TORSO,
                  BODY_LOC_HUMANOID_LOWER_TORSO);
  case 3:
    if (number(0, 1))
      return number(BODY_LOC_HUMANOID_UPPER_LEFT_ARM,
                    BODY_LOC_HUMANOID_RIGHT_ELBOW);
    else
      return number(BODY_LOC_HUMANOID_UPPER_LEFT_LEG,
                    BODY_LOC_HUMANOID_RIGHT_KNEE);
  case 4:
    return number(BODY_LOC_HUMANOID_UPPER_LEFT_ARM,
                  BODY_LOC_HUMANOID_RIGHT_HAND);
  case 5:
    return number(BODY_LOC_HUMANOID_UPPER_LEFT_LEG,
                  BODY_LOC_HUMANOID_RIGHT_ANKLE);
  }
}


/*
 * displayArmorAbsorbedAllDamageMessage :
 *      might use all these function params someday
 *
 *    ch is person hitting victim, rest is pretty self-explanatory
 */

void displayArmorAbsorbedAllDamageMessage(const P_char ch,
                                          const P_char victim,
                                          const int barehanded,
                                          const int weaptype,
                                          const int body_loc_target,
                                          const P_obj armor_hit)
{
  char     charmsg[256], victmsg[256], notvictmsg[256];

  if (!ch || !victim || !armor_hit)
    return;                     // whoops

  snprintf(notvictmsg, 256, "$N's $q absorbs all damage from $n's %s.",
          getWeaponUseString(weaptype));
  snprintf(victmsg, 256, "Your $q absorbs all damage from $n's %s.",
          getWeaponUseString(weaptype));
  snprintf(charmsg, 256, "$N's $q absorbs all damage from your %s.",
          getWeaponUseString(weaptype));

  act(notvictmsg, FALSE, ch, armor_hit, victim, TO_NOTVICT);
  act(charmsg, FALSE, ch, armor_hit, victim, TO_CHAR);
  act(victmsg, FALSE, ch, armor_hit, victim, TO_VICT);
}


#ifdef NEW_COMBAT
/*
 * hit : revamped for new combat..  ch is hitter, victim is target, weapon
 *       is pointer to weapon being used, hit_type is be used so we
 *       can use the backstab/circle messages, body_loc_target is body location
 *       being targetted, try_dodge_parry is exactly what it says, auto_hit if
 *       TRUE always hits victim...  returns TRUE if victim is no longer hittable,
 *       FALSE otherwise
 */

int hit(P_char ch, P_char victim, P_obj weapon, const int hit_type,
        const int body_loc_target, const int try_dodge_parry,
        const int auto_hit)
{
#   if 0                        // old variables
  P_char   tch;
  P_obj    p_weapon = 0;
  int      w_type, victim_ac, to_hit, dam, diceroll, wpn_skill, sic, tmp;
  int      wpn_skill_num;
  struct affected_type *af;
  static int class_mod[17] =
    { 0, 12, 12, 12, 12, 12, 8, 10, 8, 8, 4, 4, 4, 6, 6, 9, 6 };
  /*  WA  RA  PS  PA  AP  CL MN  DR SH SO NE CO TH AS ME BA */
#   endif
  char     barehanded = FALSE, weaptype, crithit = FALSE, critfumb = FALSE;
  int      ch_tohit, wpn_skill_num, wpn_skill_lvl, ch_lvl, vict_dodge,
    hitrand, dam, pre_armor_dam, dodgerand, vict_parry, parryrand,
    armor_blocked, vamp_heal, inn_armor_blocked;
  unsigned int defl, absorb, weap_dam = 0, inn_defl, inn_abs;
  P_obj    armor_hit;
  char     strn[256];
  int      weapon_has_proc = FALSE;
  int      weap_has_poison = FALSE;

#   ifdef FIGHT_DEBUG
  char     buf[512];
#   endif

  if (!SanityCheck(ch, "hit") || !SanityCheck(victim, "hit"))
    return TRUE;

  if (IS_AFFECTED(ch, AFF_BOUND))
  {
    send_to_char("Your binds are too tight for that!\r\n", ch);
    return FALSE;
  }
  if (GET_STAT(victim) == STAT_DEAD)
  {
    send_to_char("Aww, leave them alone, they are dead already.\r\n", ch);
    statuslog(AVATAR, "%s hitting a dead %s", GET_NAME(ch), GET_NAME(victim));
    return TRUE;
  }
  if ((ch->in_room != victim->in_room) ||
      (ch->specials.z_cord != victim->specials.z_cord))
  {
    send_to_char("Who?\r\n", ch);
    return FALSE;
  }
#   if 0
  if (!can_hit_target(ch, victim))
  {
    send_to_char("Seems that it's too crowded!\r\n", ch);
    return FALSE;
  }
#   endif
  // set up some variables..

  ch_lvl = GET_LEVEL(ch);
  if (ch_lvl <= 0)
    ch_lvl = 1;

  if (!weapon)
  {
    if (IS_NPC(ch))
      weaptype = ch->only.npc->attack_type;
    else
      barehanded = TRUE;
  }
  else if (GET_ITEM_TYPE(weapon) != ITEM_WEAPON)
  {
    weaptype = WEAPON_CLUB;
  }
  else
  {
    weaptype = weapon->value[0];
  }
  if (weapon)
  {
    weapon_has_proc = (obj_index[weapon->R_num].func.obj != NULL);
    weap_has_poison = (weapon->value[4] != 0);
  }
  // get base to-hit val

  ch_tohit = 30;

  if (ch->specials.z_cord > 0)
  {
    if (IS_PC(ch) && GET_CHAR_SKILL(ch, SKILL_AERIAL_COMBAT) > number(1, 101))
    {
      ch_tohit -= 30;
      notch_skill(ch, SKILL_AERIAL_COMBAT, 10);
    }
    else
      notch_skill(ch, SKILL_AERIAL_COMBAT, 17);
  }

  if (IS_NPC(ch))
    ch_tohit += (int) ((float) GET_LEVEL(ch) / 1.25);
  ch_tohit += getCharToHitValClassandLevel(ch);

  // modify by weapon skill - func checks for NULL weapon, etc

  wpn_skill_num = getWeaponSkillNumb(weapon);

  /* func checks for NPC/PCness */

  wpn_skill_lvl = getCharWeaponSkillLevel(ch, weapon);

  ch_tohit += getChartoHitSkillMod(wpn_skill_lvl);

  // victim may modify to-hit by virtue of stuff

  ch_tohit += getVictimtoHitMod(ch, victim);

  // modify by location trying to hit

  ch_tohit +=
    getBodyLocTargettingtoHitMod(ch, victim, body_loc_target, weaptype);

  if (hit_type == SKILL_BACKSTAB)
    ch_tohit += GET_LEVEL(ch) / 4;

  ch_tohit = BOUNDED(1, ch_tohit, 100);

  // calculate dodge info

  if (try_dodge_parry)
    vict_dodge = getCharDodgeVal(victim, ch, body_loc_target, weapon);

  // generate the magical random number

  hitrand = number(1, 100);

  if ((hitrand <= 3) && !auto_hit)
    critfumb = TRUE;
  else if (hitrand >= 98)
    crithit = TRUE;

  // if random numb is lower than ch's to-hit val, victim is hit

  if ((auto_hit || !canCharDodgeParry(victim, ch)) ||
      (((hitrand <= ch_tohit) || crithit) && !critfumb))
  {
    // good to-hit = 100..  if they have a good to-hit we want the victim's
    // dodge score to be affected less

    if (try_dodge_parry)
    {
      dodgerand = number(1, 101);
      vict_dodge += BOUNDED(0, (hitrand - ch_tohit) / 2, 30);
      if ((GET_RACE(victim) == RACE_DROW) ||
          (GET_RACE(victim) == RACE_HALFLING))
      {
        vict_dodge *= 2;
      }
      vict_dodge = BOUNDED(0, vict_dodge, 60);

      if ((dodgerand <= vict_dodge) && canCharDodgeParry(victim, ch))
      {
        victDodge(ch, victim, weaptype, body_loc_target, dodgerand,
                  vict_dodge);

        // add check for NPC/PC attackback

        return FALSE;
      }
      if (!number(0, 1))
        notch_skill(victim, SKILL_DODGE, 17);
      // check parry

      parryrand = number(1, 101);
      vict_parry = getCharParryVal(victim, ch, body_loc_target, weapon);
      vict_parry = BOUNDED(0, vict_parry, 100);

      if ((parryrand <= vict_parry) && canCharDodgeParry(victim, ch))
      {
        if (TryRiposte(victim, ch))
          return TRUE;
        victParry(ch, victim, weapon, body_loc_target, parryrand, vict_parry);

        // add check for NPC/PC attackback

        return FALSE;
      }
      if (!number(0, 1))
        notch_skill(victim, SKILL_PARRY, 25);
      // check THRI leap
      if (IS_THRIKREEN(victim) && canCharDodgeParry(victim, ch) &&
          (GET_POS(victim) == POS_STANDING))
      {
        dodgerand = number(1, 101);
        vict_dodge = (GET_LEVEL(victim) / 2) + 10;
        vict_dodge -= load_modifier(victim) / 10;
        vict_dodge = BOUNDED(0, vict_dodge, 35);
        if (dodgerand < vict_dodge)
        {
          act("You leap into the air, avoiding $n's attack.", FALSE, ch, 0,
              victim, TO_VICT);
          act("$N leaps into the air, avoiding your attack.", FALSE, ch, 0,
              victim, TO_CHAR);
          act("$N leaps over $n's attack.", FALSE, ch, 0, victim, TO_NOTVICT);
          return FALSE;
        }
      }
    }
    if (crithit)
    {
      send_to_char("&=LWYou score a CRITICAL HIT!!!!!&N\r\n", ch);
      make_bloodstain(ch);
    }

    pre_armor_dam = dam =
      calcChDamagetoVict(ch, victim, weapon, body_loc_target, wpn_skill_num,
                         wpn_skill_lvl, hit_type, crithit);

    /* ok lets assume if backstab success he hit somewhere with no armor or found a nice spot to hit */
    if (hit_type != SKILL_BACKSTAB)
    {
      dam = calcChDamagetoVictwithArmor(ch, victim, weapon, dam, body_loc_target, 1,    /*spec_body_loc, */
                                        &armor_hit, &defl, &absorb,
                                        &armor_blocked, &weap_dam);
    }
    else
    {
      armor_hit = NULL;
      defl = 0;
      absorb = 0;
      armor_blocked = FALSE;
    }

    dam = calcChDamagetoVictwithInnateArmor(ch, victim, weapon, dam, body_loc_target, 1,        /*spec_body_loc, */
                                            &inn_defl, &inn_abs,
                                            &inn_armor_blocked, &weap_dam);

    if (dam && !weapon && IS_AFFECTED3(ch, AFF3_CANNIBALIZE))
    {
      GET_MANA(ch) += MIN(100, dam << 2);
      if (GET_MANA(ch) > GET_MAX_MANA(ch))
        GET_MANA(ch) = GET_MAX_MANA(ch);
    }
    if (dam && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH) && !weapon &&
        GET_CLASS(ch) != CLASS_MONK)
    {
      vamp_heal = MIN(((GET_MAX_HIT(ch) * 3) >> 1) - GET_HIT(ch), dam >> 1);

      GET_HIT(ch) += vamp_heal;
      healCondition(ch, vamp_heal);
    }

    if (dam && ((hit_type == SKILL_BACKSTAB) || (hit_type == SKILL_CIRCLE)))
    {
      act("$N makes a strange sound as you place $p in $S back.",
          FALSE, ch, weapon, victim, TO_CHAR);
      act("Out of nowhere, $n stabs you in the back.",
          FALSE, ch, weapon, victim, TO_VICT);
      act
        ("$n places $p in the back of $N, resulting in some strange noises and some blood.",
         FALSE, ch, weapon, victim, TO_NOTVICTROOM);
    }

    if (IS_TRUSTED(ch))
    {
      snprintf(strn, MAX_STRING_LENGTH, "defl: %d  abs: %d  weap_dam: %d  dam: %d\r\n", defl,
              absorb, weap_dam, dam);
      send_to_char(strn, ch);
    }
    // if there was damage before armor blocked but not after, the armor has absorbed
    // all damage (should only happen with very weak hits) - so, we should echo
    // some sort of message different than a 'miss' (which is what would show up)

    if (!dam)
    {
      // was damage, but it all got absorbed by armor

      if (pre_armor_dam)
      {
        displayArmorAbsorbedAllDamageMessage(ch, victim, barehanded, weaptype,
                                             body_loc_target, armor_hit);
        return FALSE;
      }
      // such a weak hit it didn't do any damage, even though it hit..  go figure

      else
      {
        char     notvictmsg[256], charmsg[256], victmsg[256];

        snprintf(notvictmsg, 256, "$n's weak %s does no appreciable damage to $N.",
                getWeaponUseString(weaptype));
        snprintf(charmsg, 256, "Your weak %s does no noticeable damage to $N.",
                getWeaponUseString(weaptype));
        snprintf(victmsg, 256, "$n's weak %s does no noticeable damage to you.",
                getWeaponUseString(weaptype));

        act(notvictmsg, FALSE, ch, 0, victim, TO_NOTVICT);
        act(charmsg, FALSE, ch, 0, victim, TO_CHAR);
        act(victmsg, FALSE, ch, 0, victim, TO_VICT);

        return FALSE;
      }
    }
    if (!victDamage(ch, victim, barehanded, weaptype, dam, body_loc_target))
    {
      if (absorb && armor_blocked)
        applyDamagetoObject(victim, armor_hit, absorb);
      if (weap_dam)
        applyDamagetoObject(ch, weapon, weap_dam);

      // weapon special proc
      if (weapon && weapon_has_proc && char_in_list(ch) && victim &&
          GET_OPPONENT(ch) && (ch->in_room == victim->in_room))
        (*obj_index[weapon->R_num].func.obj) (weapon, ch, 1000,
                                              (char *) victim);

      if (weapon && (dam > 0) && weap_has_poison && char_in_list(ch) &&
          GET_OPPONENT(ch))
      {
        resolve_poison(victim, weapon->value[4], TRUE);
        weapon->value[4] = 0;
      }
      return FALSE;
    }
    else
    {                           // vict is dead (sniff)
      return TRUE;
    }
  }
  else
  {
    victMiss(ch, victim, weaptype, body_loc_target, barehanded);

    /* justice check.. */

    return FALSE;
  }

  // old stuff

#   if 0
  play_sound("!!SOUND(battle* P=100)", NULL, ch->in_room, TO_ROOM);
  if (ch->equipment[PRIMARY_WEAPON] &&
      (ch->equipment[PRIMARY_WEAPON]->type == ITEM_WEAPON))
  {
    p_weapon = ch->equipment[PRIMARY_WEAPON];
#      ifdef NEW_COMBAT
    w_type = (p_weapon->value[3] + TYPE_HIT);
#      else
    switch (p_weapon->value[3])
    {
    case 0:
    case 1:
    case 2:
      w_type = TYPE_WHIP;
      break;
    case 3:
      w_type = TYPE_SLASH;
      break;
    case 4:
    case 5:
    case 6:
      w_type = TYPE_CRUSH;
      break;
    case 7:
      w_type = TYPE_BLUDGEON;
      break;
    case 8:
    case 9:
    case 10:
      w_type = TYPE_CLAW;
      break;
    case 11:
      w_type = TYPE_PIERCE;
      break;

    default:
      w_type = TYPE_HIT;
      break;
    }
#      endif
  }
  else
  {
    if (IS_NPC(ch) && ch->only.npc->attack_type)
      w_type = ch->only.npc->attack_type;
    else
      w_type = TYPE_HIT;
  }
#      ifdef FIGHT_DEBUG
  snprintf(buf, MAX_STRING_LENGTH, "&+Rweapon type: %d&n ", w_type);
  send_to_char(buf, ch);
#      endif

  if (IS_NPC(ch))
  {
    if (IS_DRAGON(ch))
      to_hit = 13;
    else if (IS_WARRIOR(ch) || IS_DEMON(ch) ||
             IS_SET(ch->specials.act, ACT_PROTECTOR))
      to_hit = 12;
    else if (IS_CLERIC(ch))
      to_hit = 8;
    else if (IS_THIEF(ch))
      to_hit = 6;
    else if (IS_MAGE(ch))
      to_hit = 4;
    else if (IS_ANIMAL(ch) && GET_LEVEL(ch) < 5)
      to_hit = 2;
    else
      to_hit = 10;

    to_hit = (int) (to_hit * GET_LEVEL(ch) / 6);
  }
  else
  {
    to_hit = (int) (GET_LEVEL(ch) * class_mod[(int) GET_CLASS(ch)] / 6);
  }

#      ifdef FIGHT_DEBUG
  snprintf(buf, MAX_STRING_LENGTH, "&+Rweapon skill num: %d&n ", wpn_skill_num);
  send_to_char(buf, ch);
#      endif

  if (!wpn_skill_num)
    to_hit += (IS_NPC(ch) ?
               BOUNDED(3, (GET_LEVEL(ch) * 2), 95) :
               GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS));
  else
    to_hit += (IS_NPC(ch) ?
               BOUNDED(3, (GET_LEVEL(ch) * 2), 95) :
               GET_CHAR_SKILL(ch, wpn_skill_num));

/*
 * weapon skill and level bonus are now equally important, so we average
 * them, this keeps me from having to fine tune things again. JAB
 */

  to_hit = (to_hit / 2) + 19;
  to_hit += GET_HITROLL(ch) * 2;
  to_hit += str_app[STAT_INDEX(GET_C_STR(ch))].tohit * 2;

  if (GET_VITALITY(ch) <= 0)
    to_hit -= 40;

  if (!CAN_SEE(ch, victim))
  {
    if (IS_NPC(ch))
    {
      to_hit -= 8 * MIN(100, (120 - 2 * GET_LEVEL(ch))) / 100;
    }
    else
    {
      to_hit -= 8 * (100 - GET_CHAR_SKILL(ch, SKILL_BLINDFIGHTING)) / 100;
      notch_skill(ch, SKILL_BLINDFIGHTING, 50);
    }
  }
  /* new way for protection from evil/good to work, if they apply, they
     reduce to_hit.  */

  if (((IS_EVIL(ch) && !IS_EVIL(victim) &&
        IS_AFFECTED(victim, AFF_PROTECT_EVIL)) || (IS_GOOD(ch) &&
                                                   !IS_GOOD(victim) &&
                                                   IS_AFFECTED(victim,
                                                               AFF_PROTECT_GOOD))))
    to_hit -= (GET_LEVEL(victim) / 10);

  /* prot from undead applies when undead attacks non-undead target. */
  if (IS_UNDEAD(ch) && !IS_UNDEAD(victim) &&
      affected_by_spell(victim, SPELL_PROT_FROM_UNDEAD))
  {
    for (af = victim->affected; af; af = af->next)
      if (af->type == SPELL_PROT_FROM_UNDEAD)
        break;
    if (af)
      to_hit -= af->modifier;
  }
  victim_ac = BOUNDED(-100, GET_AC(victim), 100);

  /*
   * changed BLIND and INVISIBLE, they are now just AFF_s with no
   * modifiers, the modifiers are applied dynamically here.  Note that
   * blinded chars get no further penalty when attacking invis targets!
   * JAB
   */

  if (IS_AFFECTED(victim, AFF_BLIND))
    victim_ac +=
      40 * (100 - GET_CHAR_SKILL_P(victim, SKILL_BLINDFIGHTING)) / 100;

  if (IS_AFFECTED(ch, AFF_BLIND))
    to_hit -= 8 * (100 - GET_CHAR_SKILL_P(ch, SKILL_BLINDFIGHTING)) / 100;
  else if (!CAN_SEE(ch, victim))
    victim_ac -= 40 * (100 - GET_CHAR_SKILL_P(ch, SKILL_BLINDFIGHTING)) / 100;

  if (load_modifier(victim) > 299)
    victim_ac += 40;
  else if (load_modifier(victim) > 199)
    victim_ac += 25;
  else if (load_modifier(victim) > 99)
    victim_ac += 10;

  if (GET_CLASS(victim) == CLASS_MONK)
    victim_ac += MonkAcBonus(victim);

  /* agil only applies when it's possible to dodge */
  if ((type != SKILL_BACKSTAB) && (type != SKILL_INSTANT_KILL) &&
      (type != SKILL_CIRCLE) &&
      AWAKE(victim) && !IS_AFFECTED2(victim, AFF2_MINOR_PARALYSIS) &&
      !IS_AFFECTED2(victim, AFF2_MAJOR_PARALYSIS) &&
      (CAN_SEE(victim, ch) ||
       (GET_CHAR_SKILL_P(victim, SKILL_BLINDFIGHTING) < number(1, 100))))
    victim_ac += io_agi_defense(victim);

  victim_ac = BOUNDED(-100, victim_ac, 100);
#      ifdef FIGHT_DEBUG
  snprintf(buf, MAX_STRING_LENGTH, "&+Rvictim ac: %d&n ", victim_ac);
  send_to_char(buf, ch);
#      endif

  to_hit = BOUNDED(1, (to_hit + (victim_ac * 79 / 100)), 100);

  /* change, if target is immobile, a hit is almost certain (damage is
   * not though).  JAB */

  if (!AWAKE(victim) ||
      (IS_AFFECTED2(victim, AFF2_MINOR_PARALYSIS)) ||
      (IS_AFFECTED2(victim, AFF2_MAJOR_PARALYSIS)))
    to_hit = 100;

  /*
   * new scale for tohit will range from 19 (level 1, no +tohit) up to
   * 176 (lvl 50 warrior, +30 tohit, 95% wpn skill), to which is added,
   * 79% of the AC to yield a tohit % roll in the range of -60 to 255,
   * which is bound to 1 - 100, the actual roll is 0 to 100 which must
   * be LESS than the calculated to_hit, so a '0' is always a hit and a
   * '100' is always a miss.  System has some flaws, but is the best
   * compromise I can come up with.  Its good points: 1.  AC is almost
   * always worth something. 2.  Newbies can hit lousy ACs fairly often.
   * 3.  ToHit items are ALWAYS useful. 4.  it works! -JAB
   *
   * Due to changes of adding wpn skill to to-hit, we do some more
   * things..: - Make min/max chance of hitting 5/95, with chance of
   * doing fumble at range 1-5 / 95-100 depending on char's wpn skill. -
   * Added (skill-30)*3/5 to it (making surefire hits fairly rare) - This
   * causes.. first, newbies hit easy ACs still fairly okay.. Hitting
   * armor-less opponents is still damn easy.. Yet hitting armored (VERY
   * HEAVILY ARMORED) ones requires great skill + some lvl + nice tohit.
   * -Tormie
   */

  diceroll = number(1, 100);
  dam = 0;

  if (diceroll < 5)
    sic = -1;
  else if (diceroll < 96)
    sic = 0;
  else
    sic = 1;

  if (IS_PC(ch))
  {
    if (wpn_skill_num)
      wpn_skill = GET_CHAR_SKILL(ch, wpn_skill_num);
    else
      wpn_skill = GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS);
  }
  else
    wpn_skill = GET_LEVEL(ch) * 2;

  wpn_skill = BOUNDED(GET_LEVEL(ch) / 2, wpn_skill, 95);

  if (sic == -1 && number(30, 101) > wpn_skill + (GET_C_LUCK(ch) / 2))
    sic = 0;
  if (sic == 1 && number(1, 101) <= wpn_skill + (GET_C_AGI(ch) / 2))
    sic = 0;
  if (IS_ANIMAL(ch) && GET_LEVEL(ch) < 5)
    sic = 0;                    /* keep cats/dogs from hurting too bad */
  /*
   * ok, now we got crit / normal / fumble rolls done.. lets do
   * something about them:
   */
  switch (sic)
  {
  case -1:
    /* what crit does.. is handled further on. */
    break;
  case 1:
    /* real code aint really proper for the _SICK_ fumble coding.. :-) */
    /* Now, lets be creative here..: */
    switch (number(1, 5))
    {
    case 1:
      if (ch->equipment[WIELD] && (GET_LEVEL(ch) > 1) &&
          !IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_NODROP) &&
          (ch->equipment[WIELD]->type == ITEM_WEAPON))
      {
        send_to_char
          ("&=LYYou swing at your foe _really_ badly, sending your weapon flying!\r\n",
           ch);
        act("$n stumbles with $s attack, sending $s weapon flying!", TRUE, ch,
            0, 0, TO_ROOM);
        P_obj weap = unequip_char(ch, WIELD);
        if (weap)
            obj_to_room(weap, ch->in_room);
        }
        char_light(ch);
        room_light(ch->in_room, REAL);
      }
      else
        send_to_char("You stumble, but recover in time!\r\n", ch);
      return;
    case 2:
      /*
       * this is the part I love best.. Attack at a random foe in
       * room, if none found, _SELF_! muhahahahahahaaaa..
       */
      stop_fighting(ch);
      if( IS_DESTROYING(ch) )
      {
        stop_destroying(ch);
      }
      sic = 1;
      for (tch = world[ch->in_room].people; (tch && sic);
           tch = tch->next_in_room)
        if (tch != ch && number(1, 101) <= 75)
          sic = 0;
      if (!tch)
        tch = ch;
      if (tch != ch)
      {
        act("$n stumbles, and jabs at $N!", TRUE, ch, 0, tch, TO_NOTVICT);
        act("You stumble in your attack, and jab at $N!", TRUE, ch, 0, tch,
            TO_CHAR);
        act("$n stumbles, and jabs at YOU!!", TRUE, ch, 0, tch, TO_VICT);
        return;
      }
      else
      {
        act("$n stumbles, hitting $mself!", TRUE, ch, 0, tch, TO_NOTVICT);
        act("You stumble in your attack, and hit yourself!", TRUE, ch, 0, tch,
            TO_CHAR);
        return;
      }
    };
  };

  if ((diceroll < to_hit) || (sic == -1))
  {
    if (sic == -1)
    {
      send_to_char("&=LWYou score a CRITICAL HIT!!!!!&N\r\n", ch);
      make_bloodstain(ch);
    }
    if (p_weapon)
    {
      dam += dice(p_weapon->value[1], MAX(1, p_weapon->value[2]));
    }
    dam += TRUE_DAMROLL(ch);

    if (p_weapon && (p_weapon->value[3] == 11) &&
        ((type == SKILL_BACKSTAB) || (type == SKILL_CIRCLE)))
    {
      dam *= BOUNDED(4, ((GET_LEVEL(ch) / 1) + 2), 10);
      act("$N makes a strange sound as $n places $s weapon in $N's back!",
          TRUE, ch, 0, tch, TO_NOTVICT);
      act("$N makes a strange sound as you place your weapon in $S back!",
          TRUE, ch, 0, tch, TO_CHAR);
      act("Out of nowhere, $n suddenly stabs you in the back with $s weapon!",
          TRUE, ch, 0, tch, TO_VICT);

      if (type == SKILL_CIRCLE)
        dam /= 3;
    }
    else
      /*
       * changed, mob barehand damage is added to weapon damage, so a
       * mob wielding almost ANY weapon does more damage than a barehand
       * one of the same type.  JAB
       */

    if (IS_NPC(ch))
      dam += dice(ch->points.damnodice, ch->points.damsizedice);
    else if (!p_weapon)
    {
      if (GET_CLASS(ch) == CLASS_MONK)
        dam += MonkDamage(ch);
      else
        dam += number(0, 2);    /* 1d3 - 1 dam with bare hands */
    }
    /* damage mods based on victim's posture.  JAB */

    if (!MIN_POS(victim, POS_STANDING + STAT_NORMAL))
    {
      if (MIN_POS(victim, POS_SITTING + STAT_RESTING))
        dam = dam * 1.5;
      else if (MIN_POS(victim, POS_KNEELING + STAT_DEAD))
        dam = dam * 1.75;
      else                      /* prone and/or not awake    */
        dam *= 2;
    }
    /* critical hits are real wicked, from 2 to 3.5 times damage! */
    if (sic == -1)
      dam = dam * number(4, 7) / 2;

    /* ok, lets now take attack skill into account */
    if (IS_PC(ch) && sic != -1)
    {
      tmp = (GET_CLASS(ch) == CLASS_MONK ? SKILL_MARTIAL_ARTS : SKILL_ATTACK);
      if (GET_CHAR_SKILL(ch, tmp) < number(1, 101))
        dam = dam * number(3, 9) / 10;
      else if (!number(0, 2))
        notch_skill(ch, tmp, 20);

      if (wpn_skill_num)
      {
        if (!number(0, 4))
          notch_skill(ch, wpn_skill_num, 33.33);
      }
      else if (!number(0, 3))
      {
        notch_skill(ch, SKILL_UNARMED_DAMAGE, 33.33);
      }
    }
    else if (IS_NPC(ch))
      if (number(1, 101) >= (20 + GET_LEVEL(ch)))
        dam = dam * number(3, 9) / 10;

    dam = BOUNDED(0, dam, 32766);

    /* exhausted people just can't hit as hard... so fix it */
    if (GET_VITALITY(ch) <= 0)
      dam >>= 1;

    if (!p_weapon && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH) &&
        GET_CLASS(ch) != CLASS_MONK)
    {
      if (!IS_TRUSTED(victim) && (GET_RACE(victim) != RACE_UNDEAD) &&
          (GET_RACE(victim) != RACE_VAMPIRE))
      {
        if (affected_by_spell(ch, SPELL_VAMPIRIC_TOUCH) &&
            !NewSaves(ch, SAVING_PARA, 0))
        {
          damage(ch, victim, to_hit, SPELL_VAMPIRIC_TOUCH);
          if (affected_by_spell(ch, SPELL_VAMPIRIC_TOUCH))
            affect_from_char(ch, SPELL_VAMPIRIC_TOUCH);
          GET_HIT(ch) += to_hit;
          return;
        }
        else
        {
          /* give tiny bonus since I'm such a nice guy */
          dam += number(1, 3);
        }
        /* there is save against natural vamp touch, in which case, does normal hit. */
      }
    }
    if (type == SKILL_BACKSTAB || type == SKILL_CIRCLE)
    {
      if (damage(ch, victim, dam, SKILL_BACKSTAB))
        return;
    }
    else
    {
      if (damage(ch, victim, dam, w_type))
        return;
    }
  }
  else
  {
/* oh damn, missed again */
    if (type == SKILL_BACKSTAB)
      damage(ch, victim, 0, type);
    else
      damage(ch, victim, 0, w_type);
  }

  /*
   * well somehow, I managed to screw up and remove the check for
   * weapons specials.  Ah well, here it is, if the primary weapon has a
   * special, it is checked everytime it does damage, misses don't
   * count.  Only check primary, since secondary IS primary when it
   * hits.  -JAB
   */

  if (p_weapon && obj_index[p_weapon->R_num].func.obj && (dam > 0)
      && GET_OPPONENT(ch) && (ch->in_room == victim->in_room))
    (*obj_index[p_weapon->R_num].func.obj) (p_weapon, ch, 1000,
                                            (char *) victim);

  /* poisoned blade check */
#      if 0
  if (p_weapon && (dam > 0) && p_weapon->value[0])
  {
    resolve_poison(victim, p_weapon->value[0], TRUE);
    p_weapon->value[0] = 0;     /* remove on success */
  }
#      endif

#   endif
       // #if 0 (old stuff in hit())
}

void perform_violence(void)
{
  P_char   ch, opponent;

#   if 0
  int      k, atts;
  bool     dA, dW, tA, Haste_att, Haste_dW, tried_dodge = FALSE, delay_flag,
    dodged;
  bool     tW, fW, Haste_tW, Haste_fW, stW, ttW;

  bool     Blur_att, Blur_dW, Blur_tW, Blur_fW = FALSE;
  bool     bstW, bttW;
#   endif

  struct affected_type *af;
  bool     delay_flag;
  int      Haste_att, Blur_att;

  /* new stuff */

  bool     no_dodge;

#ifdef SIEGE_ENABLED
  for( ch = destroying_list; ch; ch = ch->next_destroying )
  {
    multihit_siege( ch );
  }
#endif

  for (ch = combat_list; ch; ch = combat_next_ch)
  {
    combat_next_ch = ch->specials.next_fighting;
    delay_flag = FALSE;
    no_dodge = FALSE;

    /* even if they're bashed, they can still recover from headbutt */
    opponent = GET_OPPONENT(ch);

    if (!opponent)
    {
      logit(LOG_DEBUG, "%s fighting null opponent in perform_violence()!",
            GET_NAME(ch));
      send_to_char("Bug in fighting! Contact a coder ASAP!!\n", ch);
      return;
    }
    if (!FightingCheck(ch, opponent, "perform_violence"))
      continue;

/** handle paralysis and slowness --TAM 2/94 **/

    if (IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
    {
      act("You remain paralyzed and can't do a thing to defend yourself..",
          FALSE, ch, 0, 0, TO_CHAR);
      act
        ("$n strains to respond to $N's attack, but the paralysis is too overpowering.",
         FALSE, ch, 0, opponent, TO_ROOM);
      continue;
    }
    if (IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS))
    {
      act("You couldn't budge a feather in your present condition.",
          FALSE, ch, 0, 0, TO_CHAR);
      act("$n is too preoccupied with $s nervous system problem to fight.",
          FALSE, ch, 0, 0, TO_ROOM);
      continue;
    }
    /*
     * disarm check is done after paralysis check.  cant recover from
     * disarmament while paralyzed.  no attack if given for those
     * chars trying to recover their disarmed weapon.  --TAM
     */

    if (DisarmRecovery(ch) == FALSE)
      continue;
    if (IS_AFFECTED2(ch, AFF2_CASTING))
      continue;

/** TAM 2/94 -- remove minor paralysis affect if attacked **/
/**             since opponent paralyze, assume they get hit 100% time **/
    if (IS_AFFECTED2(opponent, AFF2_MINOR_PARALYSIS))
    {
      act
        ("$n's crushing blow frees $N from a magic which held $M motionless.",
         FALSE, ch, 0, opponent, TO_ROOM);
      act("$n's blow shatters the magic paralyzing you!", FALSE, ch, 0,
          opponent, TO_VICT);
      act("Your blow disrupts the magic keeping $N frozen.", FALSE, ch, 0,
          opponent, TO_CHAR);

      for (af = opponent->affected; af; af = af->next)
      {
        if (af->type == SPELL_MINOR_PARALYSIS)
        {
          affect_remove(opponent, af);
          break;
        }
      }

      REMOVE_BIT(opponent->specials.affected_by2, AFF2_MINOR_PARALYSIS);

//      tried_dodge = TRUE;       /* too disoriented to do for 1 rd */
      no_dodge = TRUE;
    }
    /*
     * Changed the routine so it allows multiple attacks, handles
     * multiple parry / single dodge okay too, monk multiple attacks
     * as well, unarmed. -Fingon
     */

    /*
     * haste adds 1 attack with primary and 50% chance of extra with
     * dual.
     */
    Haste_att = (IS_AFFECTED(ch, AFF_HASTE)) ? 1 : 0;
    Blur_att = (IS_AFFECTED3(ch, AFF3_BLUR)) ? 1 : 0;

    /* Monk are not affected by haste type spell */
    if (GET_CLASS(ch) == CLASS_MONK)
    {
      Haste_att = 0;
      Blur_att = 0;
    }

    if (IS_AFFECTED3(opponent, AFF3_INERTIAL_BARRIER))
    {
      Haste_att = 0;
      Blur_att = 0;
    }

    /*
     * double and dual checks
     */

    /* slow, cuts attacks in half, if they get an odd number, they get
     * the extra one only 1/2 the time */

    /* in new combat, slowness makes characters dodge worse and hit less
       often..  may make it reduce attacks, may not */

#   if 0
    if (IS_AFFECTED2(ch, AFF2_SLOW))
    {
      if ((atts % 2) && (pulse % 2))
        atts++;
      atts >>= 1;

      atts = MAX(number(1, 2), atts);   /* need this or 1 attackers, get none */
    }
    if (IS_AFFECTED3(opponent, AFF3_INERTIAL_BARRIER))
    {
      if ((atts % 2) && (pulse % 2))
        atts++;
      atts >>= 1;

      atts = MAX(1, atts);
    }
#   endif

#   if 0
    if (IS_TROOP(ch) || IS_TROOP(opponent))
    {
//      atts = MIN(1, atts);
      if (TroopCombat(ch, opponent, 1, 0))
        continue;
    }
#   endif

/* isn't this already covered above? */

/*
   if (IS_AFFECTED2(opponent, AFF2_MINOR_PARALYSIS)) {
   REMOVE_BIT(opponent->specials.affected_by2,
   AFF2_MINOR_PARALYSIS);
   no_dodge = TRUE;
   }
 */

    // first, their attack with their primary weapon

    if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
            getBodyTarget(ch), !no_dodge, FALSE))
      continue;
    if (!char_in_list(ch))
      continue;
    if (!char_in_list(opponent))
      continue;

    /* if person has a second weapon, check their skill */

/*    if (((IS_NPC(ch) ? GET_LEVEL(ch) * 2 : GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD)) > number(1, 101)) &&
   ch->equipment[WIELD2])
 */ if (PhasedAttack(ch, SKILL_DUAL_WIELD) && ch->equipment[WIELD2])
    {
      notch_skill(ch, SKILL_DUAL_WIELD, 17);
      if (hit(ch, opponent, ch->equipment[WIELD2], TYPE_UNDEFINED,
              getBodyTarget(ch), !no_dodge, FALSE))
        continue;
      if (!char_in_list(ch))
        continue;
      if (!char_in_list(opponent))
        continue;
    }
    /* check double attack with primary */

/*    if (((IS_NPC(ch) ? GET_LEVEL(ch) * 2 : GET_CHAR_SKILL(ch, SKILL_DOUBLE_ATTACK)) > number(1, 101))) {
 */
    if (PhasedAttack(ch, SKILL_DOUBLE_ATTACK))
    {
      notch_skill(ch, SKILL_DOUBLE_ATTACK, 17);
      if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
              getBodyTarget(ch), !no_dodge, FALSE))
        continue;
      if (!char_in_list(ch))
        continue;
      if (!char_in_list(opponent))
        continue;
    }
    /* check triple attack with primary */

/*    if (((IS_NPC(ch) ? GET_LEVEL(ch) * 2 : GET_CHAR_SKILL(ch, SKILL_TRIPLE_ATTACK)) > number(1, 101)))
 */ if (PhasedAttack(ch, SKILL_TRIPLE_ATTACK))
    {
      if (IS_NPC(ch) && (GET_LEVEL(ch) < 51))
      {
      }
      else
      {
        notch_skill(ch, SKILL_TRIPLE_ATTACK, 17);
        if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }
    }
    /* check normal attack with third/fourth weapons */

    if (HAS_FOUR_HANDS(ch) && ch->equipment[WIELD3])
    {
      if (hit(ch, opponent, ch->equipment[WIELD3], TYPE_UNDEFINED,
              getBodyTarget(ch), !no_dodge, FALSE))
        continue;
      if (!char_in_list(ch))
        continue;
      if (!char_in_list(opponent))
        continue;
    }
    if (HAS_FOUR_HANDS(ch) && ch->equipment[WIELD4])
    {
      if (hit(ch, opponent, ch->equipment[WIELD4], TYPE_UNDEFINED,
              getBodyTarget(ch), !no_dodge, FALSE))
        continue;
      if (!char_in_list(ch))
        continue;
      if (!char_in_list(opponent))
        continue;
    }
    /* if user has haste, give em a few additionals */

    if (Haste_att)
    {
      if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
              getBodyTarget(ch), !no_dodge, FALSE))
        continue;
      if (!char_in_list(ch))
        continue;
      if (!char_in_list(opponent))
        continue;

      if (HAS_FOUR_HANDS(ch) && ch->equipment[WIELD3])
      {
        if (hit(ch, opponent, ch->equipment[WIELD3], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }
      if (HAS_FOUR_HANDS(ch) && ch->equipment[WIELD4])
      {
        if (hit(ch, opponent, ch->equipment[WIELD4], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }
      if (((IS_NPC(ch) ? GET_LEVEL(ch) *
            2 : GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD)) > number(1, 101)) &&
          ch->equipment[WIELD2])
      {
        if (hit(ch, opponent, ch->equipment[WIELD2], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }
      /* check double attack with primary */

/*
   if (((IS_NPC(ch) ? GET_LEVEL(ch) * 2 : GET_CHAR_SKILL(ch, SKILL_DOUBLE_ATTACK)) > number(1, 101)) &&
   ch->equipment[WIELD])
   if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
   getBodyTarget(ch), !no_dodge, FALSE)) continue;
 */

      /* check triple attack with primary */
/*
   if (((IS_NPC(ch) ? GET_LEVEL(ch) * 2 : GET_CHAR_SKILL(ch, SKILL_TRIPLE_ATTACK)) > number(1, 101)) &&
   ch->equipment[WIELD]) {
   if (IS_NPC(ch) && (GET_LEVEL(ch) < 51)) {
   } else
   if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
   getBodyTarget(ch), !no_dodge, FALSE)) continue;
   }
 */
    }
    /* ditto for blur */

    if (Blur_att)
    {
      if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
              getBodyTarget(ch), !no_dodge, FALSE))
        continue;
      if (!char_in_list(ch))
        continue;
      if (!char_in_list(opponent))
        continue;

      if (HAS_FOUR_HANDS(ch) && ch->equipment[WIELD3])
      {
        if (hit(ch, opponent, ch->equipment[WIELD3], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }
      if (HAS_FOUR_HANDS(ch) && ch->equipment[WIELD4])
      {
        if (hit(ch, opponent, ch->equipment[WIELD4], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }
      if (((IS_NPC(ch) ? GET_LEVEL(ch) *
            2 : GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD)) > number(1, 101)) &&
          ch->equipment[WIELD2])
      {
        if (hit(ch, opponent, ch->equipment[WIELD2], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }
      /* check double attack with primary */

      if (((IS_NPC(ch) ? GET_LEVEL(ch) * 2 : GET_CHAR_SKILL(ch, SKILL_DOUBLE_ATTACK)) > number(1, 101)) /*&&
                                                                                                           ch->equipment[WIELD] */ )
      {
        if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }
      /* check triple attack with primary */

      if (((IS_NPC(ch) ? GET_LEVEL(ch) : GET_CHAR_SKILL(ch, SKILL_TRIPLE_ATTACK)) > number(1, 101))     /*&&
                                                                                                           ch->equipment[WIELD] */ )
      {
        if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }
    }
    /* Monk attack with barehands */
    if (GET_CLASS(ch) == CLASS_MONK)
    {

      if (ch->equipment[WIELD] || ch->equipment[SECONDARY_WEAPON] ||
          ch->equipment[HOLD] || ch->equipment[WEAR_SHIELD] ||
          ch->equipment[WEAR_LIGHT])
      {
        continue;
      }

      notch_skill(ch, SKILL_MARTIAL_ARTS, 33.33);
      notch_skill(ch, SKILL_BAREHANDED_FIGHTING, 33.33);

      if ((GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS) > number(1, 101)))
      {
        if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }

      if ((GET_LEVEL(ch) > 10) &&
          (GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS) > number(1, 101)))
      {
        if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }

      if ((GET_LEVEL(ch) > 20) &&
          (GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS) > number(1, 101)))
      {
        if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }

      if ((GET_LEVEL(ch) > 35) &&
          (GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS) > number(1, 101)))
      {
        if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }

      if ((GET_LEVEL(ch) >= 50) &&
          (GET_CHAR_SKILL(ch, SKILL_MARTIAL_ARTS) > number(1, 101)))
      {
        if (hit(ch, opponent, ch->equipment[WIELD], TYPE_UNDEFINED,
                getBodyTarget(ch), !no_dodge, FALSE))
          continue;
        if (!char_in_list(ch))
          continue;
        if (!char_in_list(opponent))
          continue;
      }


    }

    if (AWAKE(ch))
    {
      /*
       * in an effort to trim this function down just a little, I'm
       * moving lots of things out to their own functions.  -JAB
       */

      if (IS_NPC(ch) && !delay_flag && CAN_ACT(ch))
        MobCombat(ch);
    }
  }
}

#endif // #ifdef NEW_COMBAT
