#include "comm.h"
#include "new_combat.h"
#include "objmisc.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"

extern const struct str_app_type str_app[52];
extern P_room world;
extern bodyLocInfo *physBodyLocTables[];


/* 
 * healCondition: should be called when a player is healed X points
 * goes through and heals a percentage of points in each location 
 */

void healCondition(P_char ch, int pts)
{
  int      i, phys_type, max, healloc;

#ifndef NEW_COMBAT
  return;
#else

  phys_type = GET_PHYS_TYPE(ch);
  max = getNumbBodyLocsbyPhysType(phys_type);

  for (i = 0; i < max; i++)
  {
    if (ch->points.location_hit[i] == BODYPART_GONE_VAL)
      continue;

    if (ch->points.location_hit[i] > 0)
    {
      healloc = MIN(pts, ch->points.location_hit[i]);

      ch->points.location_hit[i] -= healloc;
      pts -= healloc;

      if (pts <= 0)
        return;
    }
  }
#endif
}

/*
void healCondition(P_char ch, int pts) {
  int i, phys_type, max;

  phys_type = GET_PHYS_TYPE(ch);
  max = getNumbBodyLocsbyPhysType(phys_type);

  for (i = 0; i < max; i++) {
    if (ch->points.location_hit[i] == BODYPART_GONE_VAL) continue;

    ch->points.location_hit[i] -= MAX(1,(pts / GET_MAX_HIT(ch))); 
    if (ch->points.location_hit[i] < 0)
      ch->points.location_hit[i] = 0;
  }
}
*/

void regenCondition(P_char ch, int pts)
{
  int      i, phys_type, max;
  char     buf[256];
  const char *locstrn;

#ifndef NEW_COMBAT
  return;
#else

  if (!IS_SET(ch->specials.affected_by4, AFF4_REGENERATION))
    return;

  phys_type = GET_PHYS_TYPE(ch);
  max = getNumbBodyLocsbyPhysType(phys_type);

  for (i = 0; i < max; i++)
  {
    if (ch->points.location_hit[i] == BODYPART_GONE_VAL)
      continue;

    if (ch->points.location_hit[i] > 0)
      ch->points.location_hit[i] -= pts;

    if (ch->points.location_hit[i] < 0)
      ch->points.location_hit[i] = 0;

/*
    if (ch->points.location_hit[i] == get) {
      locstrn = getBodyLocStrn(i, ch);
      sprintf(buf, "$n's %s begins to grow back.", locstrn);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      sprintf(buf, "Your %s begins to grow back.", locstrn);
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
*/
  }
#endif
}


#ifdef NEW_COMBAT

/*
 * racePhysHumanoid : two legs, two arms, a head, fingers
 */

int racePhysHumanoid(const int race)
{
  switch (race)
  {
  case RACE_HUMAN:
  case RACE_BARBARIAN:
  case RACE_DROW:
  case RACE_GREY:
  case RACE_MOUNTAIN:
  case RACE_DUERGAR:
  case RACE_HALFLING:
  case RACE_GNOME:
  case RACE_OGRE:
  case RACE_TROLL:
  case RACE_HALFELF:
  case RACE_ILLITHID:
  case RACE_ORC:
  case RACE_GITHYANKI:
  case RACE_MINOTAUR:
  case RACE_AQUAELF:
  case RACE_SAHUAGIN:
  case RACE_GOBLIN:
  case RACE_PLICH:
  case RACE_PVAMPIRE:
  case RACE_PDKNIGHT:
  case RACE_F_ELEMENTAL:
  case RACE_A_ELEMENTAL:
  case RACE_W_ELEMENTAL:
  case RACE_E_ELEMENTAL:
  case RACE_UNDEAD:
  case RACE_VAMPIRE:
  case RACE_GHOST:
  case RACE_LYCANTH:
  case RACE_GIANT:
  case RACE_HALFORC:
  case RACE_GOLEM:
  case RACE_PRIMATE:
  case RACE_HUMANOID:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * racePhysFourArmedHumanoid : 2 legs, 4 arms - somewhat like insect, but stands upright,
 *                             and not necessarily an insect, at any rate
 */

int racePhysFourArmedHumanoid(const int race)
{
  switch (race)
  {
  case RACE_THRIKREEN:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * racePhysQuadraped : 4 legs, no arms
 */

int racePhysQuadraped(const int race)
{
  // have to guess with animal, herbivore, carnivore, and parasite..

  switch (race)
  {
  case RACE_REPTILE:
  case RACE_QUADRUPED:
  case RACE_ANIMAL:
  case RACE_HERBIVORE:
  case RACE_CARNIVORE:
  case RACE_PARASITE:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * racePhysCentaur : has four legs, two arms, humanoid upper body
 */

int racePhysCentaur(const int race)
{
  switch (race)
  {
  case RACE_CENTAUR:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * racePhysBird : two wings, two legs, small body and head
 */

int racePhysBird(const int race)
{
  switch (race)
  {
  case RACE_FLYING_ANIMAL:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * racePhysWingedHumanoid : two legs, two arms, wings
 */

int racePhysWingedHumanoid(const int race)
{
  switch (race)
  {
  case RACE_FAERIE:
  case RACE_DEMON:
  case RACE_DEVIL:
    return TRUE;

  default:
    return FALSE;
  }
}


/*
 * racePhysWingedQuadraped : four legs, wings - dragons, etc
 */

int racePhysWingedQuadraped(const int race)
{
  switch (race)
  {
  case RACE_DRAGON:
  case RACE_DRAGONKIN:
  case RACE_DRACOLICH:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * racePhysNoExtremities : no legs, no arms
 */

int racePhysNoExtremities(const int race)
{
  switch (race)
  {
  case RACE_SNAKE:
  case RACE_AQUATIC_ANIMAL:
  case RACE_PLANT:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * racePhysInsectoid : six legs, no arms
 */

int racePhysInsectoid(const int race)
{
  switch (race)
  {
  case RACE_INSECT:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * racePhysArachnid : eight legs, no arms
 */

int racePhysArachnid(const int race)
{
  switch (race)
  {
  case RACE_ARACHNID:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * racePhysBeholder : no arms or legs, big blobby thing with eyestalks
 */

int racePhysBeholder(const int race)
{
  switch (race)
  {
  case RACE_BEHOLDER:
    return TRUE;

  default:
    return FALSE;
  }
}

/*
 * getNumbBodyLocsbyRace : returns number of body locs based on race
 */

int getNumbBodyLocsbyRace(const int race)
{
  if (racePhysHumanoid(race))
    return NUMB_HUMANOID_BODY_LOCS;
  else if (racePhysFourArmedHumanoid(race))
    return NUMB_FOUR_ARMED_HUMANOID_BODY_LOCS;
  else if (racePhysQuadraped(race))
    return NUMB_QUADRUPED_BODY_LOCS;
  else if (racePhysCentaur(race))
    return NUMB_CENTAUR_BODY_LOCS;
  else if (racePhysBird(race))
    return NUMB_BIRD_BODY_LOCS;
  else if (racePhysWingedHumanoid(race))
    return NUMB_WINGED_HUMANOID_BODY_LOCS;
  else if (racePhysWingedQuadraped(race))
    return NUMB_WINGED_QUADRUPED_BODY_LOCS;
  else if (racePhysNoExtremities(race))
    return NUMB_NO_EXTREMITIES_BODY_LOCS;
  else if (racePhysInsectoid(race))
    return NUMB_INSECTOID_BODY_LOCS;
  else if (racePhysArachnid(race))
    return NUMB_ARACHNID_BODY_LOCS;
  else if (racePhysBeholder(race))
    return NUMB_BEHOLDER_BODY_LOCS;
  else
  {
    logit(LOG_DEBUG,
          "getNumbBodyLocsbyRace(): unrecognized physiology type (race %d)",
          race);
    return NUMB_HUMANOID_BODY_LOCS;     // we're doomed...
  }
}

/*
 * getNumbBodyLocsbyPhysType
 */

int getNumbBodyLocsbyPhysType(const int physType)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return NUMB_HUMANOID_BODY_LOCS;
  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return NUMB_FOUR_ARMED_HUMANOID_BODY_LOCS;
  case PHYS_TYPE_QUADRUPED:
    return NUMB_QUADRUPED_BODY_LOCS;
  case PHYS_TYPE_CENTAUR:
    return NUMB_CENTAUR_BODY_LOCS;
  case PHYS_TYPE_BIRD:
    return NUMB_BIRD_BODY_LOCS;
  case PHYS_TYPE_WINGED_HUMANOID:
    return NUMB_WINGED_HUMANOID_BODY_LOCS;
  case PHYS_TYPE_WINGED_QUADRUPED:
    return NUMB_WINGED_QUADRUPED_BODY_LOCS;
  case PHYS_TYPE_NO_EXTREMITIES:
    return NUMB_NO_EXTREMITIES_BODY_LOCS;
  case PHYS_TYPE_INSECTOID:
    return NUMB_INSECTOID_BODY_LOCS;
  case PHYS_TYPE_ARACHNID:
    return NUMB_ARACHNID_BODY_LOCS;
  case PHYS_TYPE_BEHOLDER:
    return NUMB_BEHOLDER_BODY_LOCS;

  default:
    logit(LOG_DEBUG,
          "getNumbBodyLocsbyPhysType(): unrecognized physiology type (phys %d)",
          physType);
    return NUMB_HUMANOID_BODY_LOCS;     // we're doomed...
  }
}

/*
 * getPhysTypebyRace
 */

int getPhysTypebyRace(const int race)
{
  if (racePhysHumanoid(race))
    return PHYS_TYPE_HUMANOID;
  else if (racePhysFourArmedHumanoid(race))
    return PHYS_TYPE_FOUR_ARMED_HUMANOID;
  else if (racePhysQuadraped(race))
    return PHYS_TYPE_QUADRUPED;
  else if (racePhysCentaur(race))
    return PHYS_TYPE_CENTAUR;
  else if (racePhysBird(race))
    return PHYS_TYPE_BIRD;
  else if (racePhysWingedHumanoid(race))
    return PHYS_TYPE_WINGED_HUMANOID;
  else if (racePhysWingedQuadraped(race))
    return PHYS_TYPE_WINGED_QUADRUPED;
  else if (racePhysNoExtremities(race))
    return PHYS_TYPE_NO_EXTREMITIES;
  else if (racePhysInsectoid(race))
    return PHYS_TYPE_INSECTOID;
  else if (racePhysArachnid(race))
    return PHYS_TYPE_ARACHNID;
  else if (racePhysBeholder(race))
    return PHYS_TYPE_BEHOLDER;
  else
  {
    logit(LOG_DEBUG, "getPhysTypebyRace(): unrecognized race (race %d)",
          race);
    return PHYS_TYPE_HUMANOID;  // we're doomed...
  }
}

/*
 * getBodyLocStrn : returns text string describing loc
 *
 *   loc : body location to get info on
 */

const char *getBodyLocStrn(const int loc, const P_char ch)
{
  return (physBodyLocTables[GET_PHYS_TYPE(ch)])[loc].bodyLocName;
}



/*
 * getBodyLocMaxHP : returns max hp location can have based on max hp value
 *
 *   max_hp : max HP (sole determinant of loc's max HP)
 *      loc : body location to get info on
 */

const int getBodyLocMaxHP(const P_char ch, const int loc)
{
  int      max_hp, val;

  max_hp = GET_MAX_HIT(ch);

  return (int) ((physBodyLocTables[GET_PHYS_TYPE(ch)])[loc].maxHPmult *
                (float) max_hp);
}


/*
 * getBodyLocCurrHP : returns curr hp body location has
 *
 *   ch : character with body loc
 *  loc : body location to get info on
 */

int getBodyLocCurrHP(const P_char ch, const int loc)
{
#   ifndef NEW_COMBAT
  return 0;
#   else
  if (!ch /*|| (loc < BODY_LOC_LOWEST) || (loc > BODY_LOC_HIGHEST) */ )
    return 0;

  return (getBodyLocMaxHP(ch, loc) - ch->points.location_hit[loc]);
#   endif
}


/*
 * getCharToHitValClassandLevel :
 *                   based on char's class and level, return their base to-hit
 *                   value
 */

int getCharToHitValClassandLevel(const P_char ch)
{
  int      val, lvl;

  if (!ch)
    return 0;

  lvl = GET_LEVEL(ch);
  if (!lvl)
    lvl = 1;                    // could happen

  switch (GET_CLASS(ch))
  {
    // 41 at level 50
  case CLASS_BERSERKER:
  case CLASS_WARRIOR:
    val = 10 + (lvl / 1.6);
    break;

    // 37 at level 50
  case CLASS_ANTIPALADIN:
  case CLASS_PALADIN:
    val = 9 + (lvl / 1.75);
    break;

    // 35 at level 50
  case CLASS_NONE:
  case CLASS_RANGER:
    val = 8 + (lvl / 1.8);
    break;

    // 32 at level 50
  case CLASS_MONK:
  case CLASS_MERCENARY:
    val = 7 + (lvl / 2);
    break;

    // 28 at level 50
  case CLASS_BARD:
    val = 5 + (lvl / 2.1);
    break;

    // 28 at level 50
  case CLASS_ASSASSIN:
    val = 6 + (lvl / 2.25);
    break;

    // 26 at level 50
  case CLASS_ROGUE:
  case CLASS_CLERIC:
  case CLASS_SHAMAN:
    val = 6 + (lvl / 2.5);
    break;

    // 23 at level 50
  case CLASS_DRUID:
    val = 5 + (lvl / 2.75);
    break;

    // 20 at level 50
  case CLASS_PSIONICIST:
  case CLASS_NECROMANCER:
  case CLASS_CONJURER:
  case CLASS_SUMMONER:
  case CLASS_SORCERER:
    val = 4 + (lvl / 3);
    break;

    // 14 at level 50 (woo!)
  default:
    val = 2 + (lvl / 4);
    break;
  }

  if (IS_RIDING(ch) &&
      ((GET_CLASS(ch) == CLASS_PALADIN) ||
       (GET_CLASS(ch) == CLASS_ANTIPALADIN)))
  {
    if (lvl < 26)
      val += 0;
    else if (lvl < 46)
      val += 1;
    else
      val += 2;
  }                             // no mod for 1-25, +1 for 26-45, +2 for 46+

  if (IS_RIDING(ch) &&
      ((GET_CLASS(ch) == CLASS_PALADIN) ||
       (GET_CLASS(ch) == CLASS_ANTIPALADIN)))
  {
    if (GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT) < number(1, 101))
    {
      if (lvl < 26)
        val += 0;
      else if (lvl < 46)
        val += 1;
      else
        val += 2;
    }
    if (IS_PC(ch) && !number(0, 1))
      notch_skill(ch, SKILL_MOUNTED_COMBAT, 20);
  }                             // no mod for 1-25, +1 for 26-45, +2 for 46+

  return BOUNDED(10, val, 60);
}


/*
 * getChartoHitSkillMod : based on weapon skill..
 */

int getChartoHitSkillMod(const int wpn_skl_lvl)
{
  return (wpn_skl_lvl >> 1);
}


/*
 * getVictimtoHitMod : victim's stuff may affect to-hit modifier
 */

int getVictimtoHitMod(const P_char ch, const P_char victim)
{
  struct affected_type *af;
  int      mod = 0;

  if (!ch || !victim)
    return 0;

  if ((IS_AFFECTED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch) &&
       !IS_EVIL(victim)) || (IS_AFFECTED(victim, AFF_PROTECT_GOOD) &&
                             IS_GOOD(ch) && !IS_GOOD(victim)))
    mod -= (GET_LEVEL(victim) / 10);

  /* prot from undead applies when undead attacks non-undead target. */
  if (IS_UNDEAD(ch) && !IS_UNDEAD(victim))
  {
    for (af = victim->affected; af; af = af->next)
    {
      if (af->type == SPELL_PROT_FROM_UNDEAD)
        break;
    }

    if (af)
      mod -= af->modifier;
  }

  if (affected_by_spell(victim, SPELL_FAERIE_FIRE))
    mod += 5;

  if (IS_NPC(victim))
    mod += (45 - GET_LEVEL(victim));
  else
    mod += (GET_LEVEL(ch) - GET_LEVEL(victim));

  return mod;
}


/*
 * getBodyLocTargettingtoHitMod
 */

int getBodyLocTargettingtoHitMod(const P_char ch, const P_char victim,
                                 const int body_loc_target,
                                 const int weaptype)
{
  int      chsize, victsize, mod, pos;

  if (!victim || !ch)
    return 0;

  chsize = GET_ALT_SIZE(ch);
  victsize = GET_ALT_SIZE(victim);

  mod =
    (physBodyLocTables[GET_PHYS_TYPE(ch)])[body_loc_target].
    bodyLocTargettingMod;
  pos =
    (physBodyLocTables[GET_PHYS_TYPE(ch)])[body_loc_target].bodyLocOverall;

  // now, compare sizes

  if ((pos == BODY_LOC_OVERALL_VERYLOW) || (pos == BODY_LOC_OVERALL_LOW))
  {
    if (chsize < victsize)
      mod += (victsize - chsize) * 15;
    else if (chsize > victsize)
      mod -= (chsize - victsize) * 25;
  }
  else if (pos == BODY_LOC_OVERALL_MIDDLE)
  {
    if (chsize < victsize)
      mod -= (victsize - chsize) * 5;
    else if /*( */ (chsize > victsize)
/*    &&
        (!IS_CENTAUR(victim) ||
         ((body_loc_target != BODY_LOC_FRONT_HORSE_BODY) &&
          (body_loc_target != BODY_LOC_REAR_HORSE_BODY))))*/
      mod -= (chsize - victsize) * 10;
  }
  else
    if ((pos == BODY_LOC_OVERALL_VERYHIGH) || (pos == BODY_LOC_OVERALL_HIGH))
  {
    if (chsize < victsize)
      mod -= (victsize - chsize) * 35;
    else if (chsize > victsize)
      mod += (chsize - victsize) * 15;
  }

  return mod;
}


/*
 * getWeaponUseString
 */

const char *getWeaponUseString(const int weaptype)
{
  switch (weaptype)
  {
  case WEAPON_NONE:
  case WEAPON_AXE:
  case WEAPON_HAMMER:
  case WEAPON_LONGSWORD:
  case WEAPON_MACE:
  case WEAPON_SPIKED_MACE:
  case WEAPON_SHORTSWORD:
  case WEAPON_CLUB:
  case WEAPON_SPIKED_CLUB:
  case WEAPON_STAFF:
  case WEAPON_2HANDSWORD:
  case WEAPON_SPEAR:
  case WEAPON_SICKLE:
  case WEAPON_NUMCHUCKS:
  case WEAPON_CLAW:
  default:
    return "swing";

  case WEAPON_DAGGER:
  case WEAPON_POLEARM:
  case WEAPON_LANCE:
  case WEAPON_TRIDENT:
  case WEAPON_HORN:
    return "jab";

  case WEAPON_BITE:
    return "teeth";

  case WEAPON_MAUL:
  case WEAPON_CRUSH:
    return "massive blow";

  case WEAPON_STING:
  case WEAPON_FLAIL:
  case WEAPON_WHIP:
    return "whip";

  }
}


/*
 * getWeaponHitVerb
 */

const char *getWeaponHitVerb(const int weaptype, const int tochar)
{
  switch (weaptype)
  {
  case WEAPON_NONE:
    if (tochar)
      return "hit";
    else
      return "hits";
  case WEAPON_HAMMER:
  case WEAPON_MACE:
  case WEAPON_SPIKED_MACE:
    if (tochar)
      return "smash";
    else
      return "smashes";
  case WEAPON_CLUB:
  case WEAPON_SPIKED_CLUB:
  case WEAPON_STAFF:
    if (tochar)
      return "pound";
    else
      return "pounds";
  case WEAPON_AXE:
  case WEAPON_SHORTSWORD:
  case WEAPON_LONGSWORD:
  case WEAPON_2HANDSWORD:
  case WEAPON_SPEAR:
  case WEAPON_SICKLE:
  case WEAPON_NUMCHUCKS:
  default:
    if (tochar)
      return "slash";
    else
      return "slashes";

  case WEAPON_DAGGER:
  case WEAPON_POLEARM:
  case WEAPON_LANCE:
  case WEAPON_TRIDENT:
  case WEAPON_HORN:
    if (tochar)
      return "stab";
    else
      return "stabs";

  case WEAPON_FLAIL:
  case WEAPON_WHIP:
    if (tochar)
      return "whip";
    else
      return "whips";

  case WEAPON_CLAW:
    if (tochar)
      return "claw";
    else
      return "claws";

  case WEAPON_BITE:
    if (tochar)
      return "bite";
    else
      return "bites";

  case WEAPON_STING:
    if (tochar)
      return "sting";
    else
      return "stings";

  case WEAPON_MAUL:
    if (tochar)
      return "maul";
    else
      return "mauls";

  case WEAPON_CRUSH:
    if (tochar)
      return "crush";
    else
      return "crushes";
  }

  if (tochar)
    return "buggy";
  else
    return "buggies";
}

#endif

/*
 * getWeaponSkillNumb : returns the weapon skill number required to use
 *                      this weapon
 */

int getWeaponSkillNumb(const P_obj weapon)
{
  if (!weapon)
    return SKILL_BAREHANDED_FIGHTING;

  if (GET_ITEM_TYPE(weapon) != ITEM_WEAPON)
    return SKILL_CLUB;

  /* simply return stuff based on weapon type in val0 */

  switch (weapon->value[0])
  {
  case WEAPON_AXE:
    return SKILL_AXE;
  case WEAPON_DAGGER:
    return SKILL_DAGGER;
  case WEAPON_FLAIL:
    return SKILL_FLAIL;
  case WEAPON_HAMMER:
    return SKILL_HAMMER;
  case WEAPON_LONGSWORD:
    return SKILL_LONGSWORD;
  case WEAPON_MACE:
  case WEAPON_SPIKED_MACE:
    return SKILL_MACE;
  case WEAPON_POLEARM:
    return SKILL_POLEARM;
  case WEAPON_SHORTSWORD:
    return SKILL_SHORTSWORD;
  case WEAPON_CLUB:
  case WEAPON_SPIKED_CLUB:
    return SKILL_CLUB;
  case WEAPON_STAFF:
    return SKILL_STAFF;
  case WEAPON_2HANDSWORD:
    return SKILL_TWOHANDED_SWORD;
  case WEAPON_WHIP:
    return SKILL_WHIP;
  case WEAPON_SPEAR:
    return SKILL_PICK;
  case WEAPON_LANCE:
    return SKILL_LANCE;
  case WEAPON_SICKLE:
    return SKILL_SICKLE;
  case WEAPON_TRIDENT:
    return SKILL_FORK;
  case WEAPON_HORN:
    return SKILL_HORN;
  case WEAPON_NUMCHUCKS:
    return SKILL_NUMCHUCKS;
  }

  return SKILL_CLUB;
}

#ifdef NEW_COMBAT
/*
 * getCharWeaponSkillLevel : works for both PCs and NPCs
 */

int getCharWeaponSkillLevel(const P_char ch, const P_obj weapon)
{
  int      skl;

  skl = getWeaponSkillNumb(weapon);

  if (IS_NPC(ch))
    return getNPCweaponSkillLevel(ch, skl);

  return (GET_CHAR_SKILL(ch, skl));
}


/*
 * getNPCweaponSkillLevel : as good as we can do for poor old NPCs
 */

int getNPCweaponSkillLevel(const P_char ch, const int wpn_skill)
{
  int      lvl, val, chcl;

  if (!ch)
    return 0;

  lvl = GET_LEVEL(ch);
  if (lvl <= 0)
    lvl = 1;

  chcl = GET_CLASS(ch);

  switch (chcl)
  {
  case CLASS_WARRIOR:
  case CLASS_BERSERKER:
  case CLASS_PALADIN:
  case CLASS_ANTIPALADIN:
    val = lvl * 2;
    break;

  case CLASS_NONE:
  case CLASS_RANGER:
  case CLASS_MONK:
  case CLASS_MERCENARY:
    val = lvl * 1.75;
    break;

  case CLASS_ASSASSIN:
  case CLASS_SHAMAN:
  case CLASS_CLERIC:
    val = lvl * 1.5;
    break;

  case CLASS_ROGUE:
  case CLASS_BARD:
    val = lvl * 1.3;
    break;

    /* mages, psis, anything else I forgot */

  default:
    val = lvl * 1.2;
    break;
  }

  /* give certain classes benefits with certain skills.. */

  if (((chcl == CLASS_ROGUE) || (chcl == CLASS_ASSASSIN)) &&
      (wpn_skill == SKILL_DAGGER))
    val += (lvl >> 1);

  if (((chcl == CLASS_SORCERER) || (chcl == CLASS_CONJURER) ||
     (chcl == CLASS_SUMMONER) ||  (chcl == CLASS_PSIONICIST) || (chcl == CLASS_NECROMANCER)) &&
      (wpn_skill == SKILL_STAFF))
    val += (lvl >> 2);

  if (val > 100)
    return 100;

  return val;
}


/*
 * canCharDodgeParry : vict is person trying to parry/dodge attack by
 *                     attacker - attacker pointer can be NULL, in
 *                     which case CAN_SEE check is skipped
 */

int canCharDodgeParry(const P_char vict, const P_char attacker)
{
  if (!vict || !AWAKE(vict) || IS_AFFECTED2(vict, AFF2_MINOR_PARALYSIS) ||
      IS_AFFECTED2(vict, AFF2_MAJOR_PARALYSIS) || IS_AFFECTED(vict, AFF_BOUND)
      || IS_AFFECTED(vict, AFF_KNOCKED_OUT) ||
      (GET_STAT(vict) <= STAT_SLEEPING) || IS_AFFECTED2(vict, AFF2_STUNNED) ||
      (attacker && !CAN_SEE(vict, attacker) &&
       GET_CHAR_SKILL(vict, SKILL_BLINDFIGHTING) > number(1, 300)))
    return FALSE;

  return TRUE;
}


/*
 * bodyLocisUpperArms : checks upper-most pair of arms
 */

int bodyLocisUpperArms(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return ((loc == BODY_LOC_HUMANOID_UPPER_LEFT_ARM) ||
            (loc == BODY_LOC_HUMANOID_UPPER_RIGHT_ARM) ||
            (loc == BODY_LOC_HUMANOID_LOWER_LEFT_ARM) ||
            (loc == BODY_LOC_HUMANOID_LOWER_RIGHT_ARM) ||
            (loc == BODY_LOC_HUMANOID_LEFT_ELBOW) ||
            (loc == BODY_LOC_HUMANOID_RIGHT_ELBOW));

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_UPPER_ARM) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_UPPER_ARM) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_UPPER_ARM) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_UPPER_ARM) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_ELBOW) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_ELBOW));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_UPPER_LEFT_ARM) ||
            (loc == BODY_LOC_CENTAUR_UPPER_RIGHT_ARM) ||
            (loc == BODY_LOC_CENTAUR_LOWER_LEFT_ARM) ||
            (loc == BODY_LOC_CENTAUR_LOWER_RIGHT_ARM) ||
            (loc == BODY_LOC_CENTAUR_LEFT_ELBOW) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_ELBOW));

  case PHYS_TYPE_WINGED_HUMANOID:
    return ((loc == BODY_LOC_WINGED_HUMANOID_UPPER_LEFT_ARM) ||
            (loc == BODY_LOC_WINGED_HUMANOID_UPPER_RIGHT_ARM) ||
            (loc == BODY_LOC_WINGED_HUMANOID_LOWER_LEFT_ARM) ||
            (loc == BODY_LOC_WINGED_HUMANOID_LOWER_RIGHT_ARM) ||
            (loc == BODY_LOC_WINGED_HUMANOID_LEFT_ELBOW) ||
            (loc == BODY_LOC_WINGED_HUMANOID_RIGHT_ELBOW));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisUpperWrists
 */

int bodyLocisUpperWrists(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return ((loc == BODY_LOC_HUMANOID_LEFT_WRIST) ||
            (loc == BODY_LOC_HUMANOID_RIGHT_WRIST));

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_WRIST) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_WRIST));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_LEFT_WRIST) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_WRIST));

  case PHYS_TYPE_WINGED_HUMANOID:
    return ((loc == BODY_LOC_WINGED_HUMANOID_LEFT_WRIST) ||
            (loc == BODY_LOC_WINGED_HUMANOID_RIGHT_WRIST));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisUpperHands
 */

int bodyLocisUpperHands(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return ((loc == BODY_LOC_HUMANOID_LEFT_HAND) ||
            (loc == BODY_LOC_HUMANOID_RIGHT_HAND));

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_HAND) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_HAND));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_LEFT_HAND) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_HAND));

  case PHYS_TYPE_WINGED_HUMANOID:
    return ((loc == BODY_LOC_WINGED_HUMANOID_LEFT_HAND) ||
            (loc == BODY_LOC_WINGED_HUMANOID_RIGHT_HAND));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisHead : targetting head, not eyes or ears, etc
 */

int bodyLocisHead(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return (loc == BODY_LOC_HUMANOID_HEAD);

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return (loc == BODY_LOC_FOUR_ARMED_HUMANOID_HEAD);

  case PHYS_TYPE_QUADRUPED:
    return (loc == BODY_LOC_QUADRUPED_HEAD);

  case PHYS_TYPE_CENTAUR:
    return (loc == BODY_LOC_CENTAUR_HEAD);

  case PHYS_TYPE_BIRD:
    return (loc == BODY_LOC_BIRD_HEAD);

  case PHYS_TYPE_WINGED_HUMANOID:
    return (loc == BODY_LOC_WINGED_HUMANOID_HEAD);

  case PHYS_TYPE_WINGED_QUADRUPED:
    return (loc == BODY_LOC_WINGED_QUADRUPED_HEAD);

  case PHYS_TYPE_NO_EXTREMITIES:
    return (loc == BODY_LOC_NO_EXTREMITIES_HEAD);

  case PHYS_TYPE_INSECTOID:
    return (loc == BODY_LOC_INSECTOID_HEAD);

  case PHYS_TYPE_ARACHNID:
    return (loc == BODY_LOC_ARACHNID_HEAD);

  default:
    return FALSE;
  }
}


/*
 * bodyLocisChin : targetting chin
 */

int bodyLocisChin(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return (loc == BODY_LOC_HUMANOID_CHIN);

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return (loc == BODY_LOC_FOUR_ARMED_HUMANOID_CHIN);

  case PHYS_TYPE_QUADRUPED:
    return (loc == BODY_LOC_QUADRUPED_CHIN);

  case PHYS_TYPE_CENTAUR:
    return (loc == BODY_LOC_CENTAUR_CHIN);

  case PHYS_TYPE_WINGED_HUMANOID:
    return (loc == BODY_LOC_WINGED_HUMANOID_CHIN);

  case PHYS_TYPE_WINGED_QUADRUPED:
    return (loc == BODY_LOC_WINGED_QUADRUPED_CHIN);

  default:
    return FALSE;
  }
}


/*
 * bodyLocisLeftEye
 */

int bodyLocisLeftEye(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return (loc == BODY_LOC_HUMANOID_LEFT_EYE);

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LEFT_EYE);

  case PHYS_TYPE_QUADRUPED:
    return (loc == BODY_LOC_QUADRUPED_LEFT_EYE);

  case PHYS_TYPE_CENTAUR:
    return (loc == BODY_LOC_CENTAUR_LEFT_EYE);

  case PHYS_TYPE_BIRD:
    return (loc == BODY_LOC_BIRD_LEFT_EYE);

  case PHYS_TYPE_WINGED_HUMANOID:
    return (loc == BODY_LOC_WINGED_HUMANOID_LEFT_EYE);

  case PHYS_TYPE_WINGED_QUADRUPED:
    return (loc == BODY_LOC_WINGED_QUADRUPED_LEFT_EYE);

  case PHYS_TYPE_NO_EXTREMITIES:
    return (loc == BODY_LOC_NO_EXTREMITIES_LEFT_EYE);

  case PHYS_TYPE_INSECTOID:
    return (loc == BODY_LOC_INSECTOID_LEFT_EYE);

  case PHYS_TYPE_ARACHNID:
    return (loc == BODY_LOC_ARACHNID_LEFT_EYE);

  default:
    return FALSE;
  }
}


/*
 * bodyLocisRightEye
 */

int bodyLocisRightEye(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return (loc == BODY_LOC_HUMANOID_RIGHT_EYE);

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return (loc == BODY_LOC_FOUR_ARMED_HUMANOID_RIGHT_EYE);

  case PHYS_TYPE_QUADRUPED:
    return (loc == BODY_LOC_QUADRUPED_RIGHT_EYE);

  case PHYS_TYPE_CENTAUR:
    return (loc == BODY_LOC_CENTAUR_RIGHT_EYE);

  case PHYS_TYPE_BIRD:
    return (loc == BODY_LOC_BIRD_RIGHT_EYE);

  case PHYS_TYPE_WINGED_HUMANOID:
    return (loc == BODY_LOC_WINGED_HUMANOID_RIGHT_EYE);

  case PHYS_TYPE_WINGED_QUADRUPED:
    return (loc == BODY_LOC_WINGED_QUADRUPED_RIGHT_EYE);

  case PHYS_TYPE_NO_EXTREMITIES:
    return (loc == BODY_LOC_NO_EXTREMITIES_RIGHT_EYE);

  case PHYS_TYPE_INSECTOID:
    return (loc == BODY_LOC_INSECTOID_RIGHT_EYE);

  case PHYS_TYPE_ARACHNID:
    return (loc == BODY_LOC_ARACHNID_RIGHT_EYE);

  default:
    return FALSE;
  }
}


/*
 * bodyLocisEar
 */

int bodyLocisEar(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return ((loc == BODY_LOC_HUMANOID_LEFT_EAR) ||
            (loc == BODY_LOC_HUMANOID_RIGHT_EAR));

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_LEFT_EAR) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_RIGHT_EAR));

  case PHYS_TYPE_QUADRUPED:
    return ((loc == BODY_LOC_QUADRUPED_LEFT_EAR) ||
            (loc == BODY_LOC_QUADRUPED_RIGHT_EAR));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_LEFT_EAR) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_EAR));

  case PHYS_TYPE_WINGED_HUMANOID:
    return ((loc == BODY_LOC_WINGED_HUMANOID_LEFT_EAR) ||
            (loc == BODY_LOC_WINGED_HUMANOID_RIGHT_EAR));

  case PHYS_TYPE_WINGED_QUADRUPED:
    return ((loc == BODY_LOC_WINGED_QUADRUPED_LEFT_EAR) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_RIGHT_EAR));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisNeck
 */

int bodyLocisNeck(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return (loc == BODY_LOC_HUMANOID_NECK);

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return (loc == BODY_LOC_FOUR_ARMED_HUMANOID_NECK);

  case PHYS_TYPE_QUADRUPED:
    return (loc == BODY_LOC_QUADRUPED_NECK);

  case PHYS_TYPE_CENTAUR:
    return (loc == BODY_LOC_CENTAUR_NECK);

  case PHYS_TYPE_BIRD:
    return (loc == BODY_LOC_BIRD_NECK);

  case PHYS_TYPE_WINGED_HUMANOID:
    return (loc == BODY_LOC_WINGED_HUMANOID_NECK);

  case PHYS_TYPE_WINGED_QUADRUPED:
    return (loc == BODY_LOC_WINGED_QUADRUPED_NECK);

  default:
    return FALSE;
  }
}


/*
 * bodyLocisUpperTorso
 */

int bodyLocisUpperTorso(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return (loc == BODY_LOC_HUMANOID_UPPER_TORSO);

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return (loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_TORSO);

  case PHYS_TYPE_CENTAUR:
    return (loc == BODY_LOC_CENTAUR_UPPER_TORSO);

  case PHYS_TYPE_WINGED_HUMANOID:
    return (loc == BODY_LOC_WINGED_HUMANOID_UPPER_TORSO);

  case PHYS_TYPE_INSECTOID:
    return (loc == BODY_LOC_INSECTOID_UPPER_BODY);

  case PHYS_TYPE_ARACHNID:
    return (loc == BODY_LOC_ARACHNID_UPPER_BODY);

  default:
    return FALSE;
  }
}


/*
 * bodyLocisLowerTorso
 */

int bodyLocisLowerTorso(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return (loc == BODY_LOC_HUMANOID_LOWER_TORSO);

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_TORSO);

  case PHYS_TYPE_CENTAUR:
    return (loc == BODY_LOC_CENTAUR_LOWER_TORSO);

  case PHYS_TYPE_WINGED_HUMANOID:
    return (loc == BODY_LOC_WINGED_HUMANOID_LOWER_TORSO);

  case PHYS_TYPE_INSECTOID:
    return (loc == BODY_LOC_INSECTOID_LOWER_BODY);

  case PHYS_TYPE_ARACHNID:
    return (loc == BODY_LOC_ARACHNID_LOWER_BODY);

  default:
    return FALSE;
  }
}


/*
 * bodyLocisUpperShoulders
 */

int bodyLocisUpperShoulders(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return ((loc == BODY_LOC_HUMANOID_LEFT_SHOULDER) ||
            (loc == BODY_LOC_HUMANOID_RIGHT_SHOULDER));

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_SHOULDER) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_SHOULDER));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_LEFT_SHOULDER) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_SHOULDER));

  case PHYS_TYPE_WINGED_HUMANOID:
    return ((loc == BODY_LOC_WINGED_HUMANOID_LEFT_SHOULDER) ||
            (loc == BODY_LOC_WINGED_HUMANOID_RIGHT_SHOULDER));

  case PHYS_TYPE_INSECTOID:
    return ((loc == BODY_LOC_INSECTOID_FRONT_LEFT_UPPER_JOINT) ||
            (loc == BODY_LOC_INSECTOID_FRONT_RIGHT_UPPER_JOINT));

  case PHYS_TYPE_ARACHNID:
    return ((loc == BODY_LOC_ARACHNID_FRONT_LEFT_UPPER_JOINT) ||
            (loc == BODY_LOC_ARACHNID_FRONT_RIGHT_UPPER_JOINT));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisLegs : birds/insects/arachnids can't wear leggings, or 
 *                 at least, they shouldn't be able to, so don't check
 *                 them - including centaurs/quads for completeness
 */

int bodyLocisLegs(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return ((loc == BODY_LOC_HUMANOID_UPPER_LEFT_LEG) ||
            (loc == BODY_LOC_HUMANOID_LEFT_KNEE) ||
            (loc == BODY_LOC_HUMANOID_LOWER_LEFT_LEG) ||
            (loc == BODY_LOC_HUMANOID_UPPER_RIGHT_LEG) ||
            (loc == BODY_LOC_HUMANOID_RIGHT_KNEE) ||
            (loc == BODY_LOC_HUMANOID_LOWER_RIGHT_LEG));

  case PHYS_TYPE_QUADRUPED:
    return ((loc == BODY_LOC_QUADRUPED_UPPER_LEFT_FRONT_LEG) ||
            (loc == BODY_LOC_QUADRUPED_LEFT_FRONT_KNEE) ||
            (loc == BODY_LOC_QUADRUPED_LOWER_LEFT_FRONT_LEG) ||
            (loc == BODY_LOC_QUADRUPED_UPPER_RIGHT_FRONT_LEG) ||
            (loc == BODY_LOC_QUADRUPED_RIGHT_FRONT_KNEE) ||
            (loc == BODY_LOC_QUADRUPED_LOWER_RIGHT_FRONT_LEG));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_UPPER_LEFT_FRONT_LEG) ||
            (loc == BODY_LOC_CENTAUR_LEFT_FRONT_KNEE) ||
            (loc == BODY_LOC_CENTAUR_LOWER_LEFT_FRONT_LEG) ||
            (loc == BODY_LOC_CENTAUR_UPPER_RIGHT_FRONT_LEG) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_FRONT_KNEE) ||
            (loc == BODY_LOC_CENTAUR_LOWER_RIGHT_FRONT_LEG));

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_LEG) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LEFT_KNEE) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_LEG) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_LEG) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_RIGHT_KNEE) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_LEG));

  case PHYS_TYPE_WINGED_HUMANOID:
    return ((loc == BODY_LOC_WINGED_HUMANOID_UPPER_LEFT_LEG) ||
            (loc == BODY_LOC_WINGED_HUMANOID_LEFT_KNEE) ||
            (loc == BODY_LOC_WINGED_HUMANOID_LOWER_LEFT_LEG) ||
            (loc == BODY_LOC_WINGED_HUMANOID_UPPER_RIGHT_LEG) ||
            (loc == BODY_LOC_WINGED_HUMANOID_RIGHT_KNEE) ||
            (loc == BODY_LOC_WINGED_HUMANOID_LOWER_RIGHT_LEG));

  case PHYS_TYPE_WINGED_QUADRUPED:
    return ((loc == BODY_LOC_WINGED_QUADRUPED_UPPER_LEFT_FRONT_LEG) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_LEFT_FRONT_KNEE) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_LOWER_LEFT_FRONT_LEG) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_UPPER_RIGHT_FRONT_LEG) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_RIGHT_FRONT_KNEE) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_LOWER_RIGHT_FRONT_LEG));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisFeet : birds/insects/arachnids shan't wear boots/shoes
 */

int bodyLocisFeet(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_HUMANOID:
    return ((loc == BODY_LOC_HUMANOID_LEFT_FOOT) ||
            (loc == BODY_LOC_HUMANOID_LEFT_ANKLE) ||
            (loc == BODY_LOC_HUMANOID_RIGHT_FOOT) ||
            (loc == BODY_LOC_HUMANOID_RIGHT_ANKLE));

  case PHYS_TYPE_QUADRUPED:
    return ((loc == BODY_LOC_QUADRUPED_LEFT_FRONT_HOOF) ||
            (loc == BODY_LOC_QUADRUPED_LEFT_FRONT_ANKLE) ||
            (loc == BODY_LOC_QUADRUPED_RIGHT_FRONT_HOOF) ||
            (loc == BODY_LOC_QUADRUPED_RIGHT_FRONT_ANKLE));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_LEFT_FRONT_HOOF) ||
            (loc == BODY_LOC_CENTAUR_LEFT_FRONT_ANKLE) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_FRONT_HOOF) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_FRONT_ANKLE));

  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_LEFT_FOOT) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LEFT_ANKLE) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_RIGHT_FOOT) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_RIGHT_ANKLE));

  case PHYS_TYPE_WINGED_HUMANOID:
    return ((loc == BODY_LOC_WINGED_HUMANOID_LEFT_FOOT) ||
            (loc == BODY_LOC_WINGED_HUMANOID_LEFT_ANKLE) ||
            (loc == BODY_LOC_WINGED_HUMANOID_RIGHT_FOOT) ||
            (loc == BODY_LOC_WINGED_HUMANOID_RIGHT_ANKLE));

  case PHYS_TYPE_WINGED_QUADRUPED:
    return ((loc == BODY_LOC_WINGED_QUADRUPED_LEFT_FRONT_HOOF) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_LEFT_FRONT_ANKLE) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_RIGHT_FRONT_HOOF) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_RIGHT_FRONT_ANKLE));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisLowerArms : checks lower-most pair of arms (if applicable)
 */

int bodyLocisLowerArms(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_LOWER_ARM) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_LOWER_ARM) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_LOWER_ARM) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_LOWER_ARM) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_ELBOW) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_ELBOW));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisLowerWrists
 */

int bodyLocisLowerWrists(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_WRIST) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_WRIST));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisLowerHands
 */

int bodyLocisLowerHands(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_FOUR_ARMED_HUMANOID:
    return ((loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_HAND) ||
            (loc == BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_HAND));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisHorseBody
 */

int bodyLocisHorseBody(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_QUADRUPED:
    return ((loc == BODY_LOC_QUADRUPED_FRONT_BODY) ||
            (loc == BODY_LOC_QUADRUPED_REAR_BODY));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_FRONT_HORSE_BODY) ||
            (loc == BODY_LOC_CENTAUR_REAR_HORSE_BODY));

  case PHYS_TYPE_WINGED_QUADRUPED:
    return ((loc == BODY_LOC_WINGED_QUADRUPED_FRONT_BODY) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_REAR_BODY));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisRearLegs : including just because there is an eq slot for it
 */

int bodyLocisRearLegs(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_QUADRUPED:
    return ((loc == BODY_LOC_QUADRUPED_UPPER_LEFT_REAR_LEG) ||
            (loc == BODY_LOC_QUADRUPED_LOWER_LEFT_REAR_LEG) ||
            (loc == BODY_LOC_QUADRUPED_UPPER_RIGHT_REAR_LEG) ||
            (loc == BODY_LOC_QUADRUPED_LOWER_RIGHT_REAR_LEG) ||
            (loc == BODY_LOC_QUADRUPED_LEFT_REAR_KNEE) ||
            (loc == BODY_LOC_QUADRUPED_RIGHT_REAR_KNEE));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_UPPER_LEFT_REAR_LEG) ||
            (loc == BODY_LOC_CENTAUR_LOWER_LEFT_REAR_LEG) ||
            (loc == BODY_LOC_CENTAUR_UPPER_RIGHT_REAR_LEG) ||
            (loc == BODY_LOC_CENTAUR_LOWER_RIGHT_REAR_LEG) ||
            (loc == BODY_LOC_CENTAUR_LEFT_REAR_KNEE) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_REAR_KNEE));

  case PHYS_TYPE_WINGED_QUADRUPED:
    return ((loc == BODY_LOC_WINGED_QUADRUPED_UPPER_LEFT_REAR_LEG) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_LOWER_LEFT_REAR_LEG) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_UPPER_RIGHT_REAR_LEG) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_LOWER_RIGHT_REAR_LEG) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_LEFT_REAR_KNEE) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_RIGHT_REAR_KNEE));

  default:
    return FALSE;
  }
}


/*
 * bodyLocisRearFeet
 */

int bodyLocisRearFeet(const int physType, const int loc)
{
  switch (physType)
  {
  case PHYS_TYPE_QUADRUPED:
    return ((loc == BODY_LOC_QUADRUPED_LEFT_REAR_HOOF) ||
            (loc == BODY_LOC_QUADRUPED_LEFT_REAR_ANKLE) ||
            (loc == BODY_LOC_QUADRUPED_RIGHT_REAR_HOOF) ||
            (loc == BODY_LOC_QUADRUPED_RIGHT_REAR_ANKLE));

  case PHYS_TYPE_CENTAUR:
    return ((loc == BODY_LOC_CENTAUR_LEFT_REAR_HOOF) ||
            (loc == BODY_LOC_CENTAUR_LEFT_REAR_ANKLE) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_REAR_HOOF) ||
            (loc == BODY_LOC_CENTAUR_RIGHT_REAR_ANKLE));

  case PHYS_TYPE_WINGED_QUADRUPED:
    return ((loc == BODY_LOC_WINGED_QUADRUPED_LEFT_REAR_HOOF) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_LEFT_REAR_ANKLE) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_RIGHT_REAR_HOOF) ||
            (loc == BODY_LOC_WINGED_QUADRUPED_RIGHT_REAR_ANKLE));

  default:
    return FALSE;
  }
}


/*
 * calcChDamagetoVict : this function calculates damage based on weapon type,
 *                      NPC/PCness, and a few other factors - doesn't take into
 *                      account armor, but does check for skill increase
 */

int calcChDamagetoVict(P_char ch, P_char victim, P_obj weap,
                       const int body_loc, const int wpn_skl,
                       const int wpn_skl_lvl, const int hit_type,
                       const int crit_hit)
{
  int      lvl = 0, dam = 0, skl, att_skill;

  if (!ch || !victim)
    return 0;

  if (weap && (GET_ITEM_TYPE(weap) == ITEM_WEAPON))
  {
    dam += dice(MAX(1, weap->value[1]), MAX(1, weap->value[2]));
  }
  else
    dam++;

  dam += str_app[STAT_INDEX(GET_C_STR(ch))].todam + GET_DAMROLL(ch);

  if (weap && (GET_ITEM_TYPE(weap) == ITEM_WEAPON) &&
      isBackstabber(weap) &&
      ((hit_type == SKILL_BACKSTAB) || (hit_type == SKILL_CIRCLE)))
  {
    dam *= BOUNDED(2, ((GET_LEVEL(ch) / 3) + 2), 8);
/*    dam *= MIN(number(2, 8), (SKILL_BACKSTAB / 12) + 1);*/

    if (hit_type == SKILL_CIRCLE)
    {
      if (IS_PC(ch))
        skl = GET_CHAR_SKILL(ch, SKILL_CIRCLE);
      else
        skl = BOUNDED(1, GET_LEVEL(ch) * 2, 100);

      dam = (int) ((float) dam * (float) (skl / 200.0));
    }
  }

  /*
   * changed, mob barehand damage is added to weapon damage, so a
   * mob wielding almost ANY weapon does more damage than a barehand
   * one of the same type.  JAB
   */

  if (IS_NPC(ch))
    dam += dice(ch->points.damnodice, ch->points.damsizedice);
  else if (!weap)
  {
    if (GET_CLASS(ch) == CLASS_MONK)
      dam += MonkDamage(ch);
    else
      dam += number(0, 2);      /* 1d3 - 1 dam with bare hands */
  }

  if (dam <= 0)
    dam = 1;

  /* damage mods based on victim's posture.  JAB */

  if (!MIN_POS(victim, POS_STANDING + STAT_NORMAL))
  {
    if (MIN_POS(victim, POS_SITTING + STAT_RESTING))
      dam *= 1.5;
    else if (MIN_POS(victim, POS_KNEELING + STAT_DEAD))
      dam *= 1.75;
    else                        /* prone and/or not awake    */
      dam *= 2;
  }

  /* critical hits are real wicked, from 2 to 3.5 times damage! */

  if (crit_hit)
    dam = ((dam * number(4, 7)) >> 1);

  /* ok, lets now take attack skill into account */

  att_skill = (weap ? SKILL_ATTACK : SKILL_BAREHANDED_FIGHTING);

  if (IS_PC(ch) && (GET_CHAR_SKILL(ch, att_skill) < number(1, 101)))
    dam = ((dam * number(6, 9)) / 10);
  else if (IS_NPC(ch) && (number(1, 101) >= (20 + GET_LEVEL(ch))))
    dam = ((dam * number(4, 9)) / 10);

  if (IS_PC(ch) && !number(0, 1))
    notch_skill(ch, att_skill, 20);

  if (IS_PC(ch))
  {
    if (wpn_skl && !number(0, 1))
      notch_skill(ch, wpn_skl, 33.33);
  }

  lvl = GET_LEVEL(ch);

  if (IS_RIDING(ch) && ((GET_CLASS(ch) == CLASS_PALADIN)
                        || (GET_CLASS(ch) == CLASS_ANTIPALADIN)))
  {
    if (GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT) < number(1, 101))
    {
      if (lvl < 26)
        dam += 0;
      else if (lvl < 46)
        dam += 1;
      else
        dam += 2;
    }
    if (IS_PC(ch) && !number(0, 1))
      notch_skill(ch, SKILL_MOUNTED_COMBAT, 20);
  }                             // no mod for 1-25, +1 for 26-45, +2 for 46+

  lvl = GET_LEVEL(ch);

  if (IS_RIDING(ch) && ((GET_CLASS(ch) == CLASS_PALADIN)
                        || (GET_CLASS(ch) == CLASS_ANTIPALADIN)))
  {
    if (lvl < 26)
      dam += 0;
    else if (lvl < 46)
      dam += 1;
    else
      dam += 2;
  }                             // no mod for 1-25, +1 for 26-45, +2 for 46+

  /* exhausted people just can't hit as hard... so fix it */

  if (GET_VITALITY(ch) <= 0)
    dam >>= 1;

  dam = BOUNDED(0, dam, 32766);

  return dam;
}


/*
 * getCharParryVal
 */

int getCharParryVal(const P_char vict, const P_char attacker,
                    const int body_loc_target, const P_obj weapon)
{
  int      parry_skill, chance, victsize, attsize, victagi, numb_att = 0;
  float    mod = 1.0;
  char     mod_sect = FALSE;
  P_char   tch;

  if (!vict)
    return 0;

  // victim isn't gonna parry with his arm..

  if (!canCharDodgeParry(vict, attacker) || !weapon)
    return 0;

  // parry skill for PCs is 1/2 weapon skill, 1/2 parry skill

  if (IS_PC(vict))
    parry_skill = ((GET_CHAR_SKILL(vict, SKILL_PARRY) / 2) +
                   (GET_CHAR_SKILL(vict, getWeaponSkillNumb(weapon)) / 2));
  else if (IS_WARRIOR(vict))
    parry_skill = BOUNDED(0, GET_LEVEL(vict) * 2, 100);
  else
    parry_skill = 0;

  chance = parry_skill / 5;

  // further modify chance by amount vict is weighed down, surroundings
  // (underwater, etc), affecting spells, and whatever else I can think
  // of

  // compare size of attacker vs victim

  // dunno how valid this is for parry ..  will probably have to depend on
  // type of swing _and_ size

/*
  if (attacker)
  {
    attsize = GET_ALT_SIZE(attacker);
    victsize = GET_ALT_SIZE(vict);

    chance += (attsize - victsize) * 15;
  }
*/

  // modify based on dexterity of defender

  mod += (GET_C_DEX(vict) - 50) / 200;
//  chance += (GET_C_DEX(vict) - 50) / 10;

  // modify based on terrain

  switch (world[vict->in_room].sector_type)
  {
  case SECT_UNDERWATER:
  case SECT_UNDERWATER_GR:
  case SECT_UNDRWLD_WATER:
  case SECT_UNDRWLD_NOSWIM:
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_OCEAN:
  case SECT_WATER_PLANE:
  case SECT_UNDRWLD_LIQMITH:
//      chance -= 50;
    mod -= 0.5;
    mod_sect = TRUE;
    break;

  case SECT_UNDRWLD_LOWCEIL:
//      chance -= 30;
    mod -= 0.3;
    mod_sect = TRUE;
    break;
  }

  if (IS_SET(world[vict->in_room].room_flags, SINGLE_FILE))
  {
    if (!mod_sect)
      mod -= 0.25;              //chance -= 25;
    else
      mod -= 0.15;              //chance -= 15;
  }

  // haste/blur helps a little, cuz I'm a nice guy

  if (IS_AFFECTED(vict, AFF_HASTE) || IS_AFFECTED3(vict, AFF3_BLUR))
  {
//    chance += 10;
    mod += 0.1;
  }

  // slowness hurts a tad

  if (IS_AFFECTED2(vict, AFF2_SLOW))
  {
//    chance -= 15;
    mod -= 0.15;
  }

  // the attacker's haste/slowness affects it too

  if (attacker &&
      (IS_AFFECTED(attacker, AFF_HASTE) || IS_AFFECTED3(attacker, AFF3_BLUR)))
  {
//    chance -= 5;
    mod -= 0.05;
  }

  // same deal with slowness

  if (attacker && IS_AFFECTED2(attacker, AFF2_SLOW))
  {
//    chance += 10;
    mod -= 0.10;
  }

  // take into account the number of people attacking the poor bastard

  for (tch = world[vict->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting == vict)
      numb_att++;
  }

  if (numb_att == 2)
//    chance -= 10;
    mod -= 0.1;
  else if (numb_att == 3)
//    chance -= 25;
    mod -= 0.25;
  else if (numb_att == 4)
//    chance -= 40;
    mod -= 0.40;
  else if (numb_att == 5)
//    chance -= 60;
    mod -= 0.60;
  else
//    chance -= (numb_att * 14);
    mod -= ((float) numb_att * 14.0) / 100.0;

  // add weight stuff and maybe weapon-based stuff here ...

  chance = (float) chance *mod;

  return BOUNDED(0, chance, 100);
}


/*
 * getParryEaseString
 */

const char *getParryEaseString(const int passedby, const int tochar)
{
  if (passedby > 85)
    if (tochar)
      return "have no trouble parrying";
    else
      return "has no trouble parrying";
  else if (passedby > 60)
    if (tochar)
      return "easily parry";
    else
      return "easily parry";
  else if (passedby > 40)
    if (tochar)
      return "parry";
    else
      return "parries";
  else if (passedby > 20)
    if (tochar)
      return "barely parry";
    else
      return "barely parries";
  else if (tochar)
    return "narrowly parry";
  else
    return "narrowly parries";
}


/*
 * getCharDodgeVal : returns dodge percentage from 0-100 - attacker can be NULL and
 *                   all attacker-specific bonuses/penalties aren't added in
 */

int getCharDodgeVal(const P_char vict, const P_char attacker,
                    const int body_loc_target, const P_obj weapon)
{
  int      dodge_skill, chance = 0, victsize, attsize, victagi, numb_att = 0;
  float    mod = 1.0;
  char     mod_sect = FALSE;
  P_char   tch;

  if (!vict)
    return 0;

  if (!canCharDodgeParry(vict, attacker))
    return 0;

  if (IS_PC(vict))
    dodge_skill = GET_CHAR_SKILL(vict, SKILL_DODGE);
  else
    dodge_skill = BOUNDED(0, GET_LEVEL(vict) * 2, 100);

  if (dodge_skill)
    chance = MAX(1, dodge_skill / 5);

  // further modify chance by amount vict is weighed down, surroundings
  // (underwater, etc), affecting spells, and whatever else I can think
  // of

  // compare size of attacker vs victim

  if (attacker)
  {
    attsize = GET_ALT_SIZE(attacker);
    victsize = GET_ALT_SIZE(vict);

    chance += (attsize - victsize) * 15;        // size is very important..
//    mod += (attsize - victsize) * 0.15;
  }

   // lucky or unlucky?

   if (GET_C_LUCK(vict) > number(0, 110)) {
     chance = (int) (chance * 1.05);
   } else {
     chance = (int) (chance * 0.95);
   }

  // modify based on agility of victim

  victagi = GET_C_AGI(vict);
//  chance += (victagi - 50) / 10;
  mod += (victagi - 50) / 200;

  // modify based on terrain

  switch (world[vict->in_room].sector_type)
  {
  case SECT_UNDERWATER:
  case SECT_UNDERWATER_GR:
  case SECT_UNDRWLD_WATER:
  case SECT_UNDRWLD_NOSWIM:
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_OCEAN:
  case SECT_WATER_PLANE:
  case SECT_UNDRWLD_LIQMITH:
//      chance -= 50;
    mod -= 0.50;
    mod_sect = TRUE;
    break;

  case SECT_UNDRWLD_LOWCEIL:
//      chance -= 30;
    mod -= 0.30;
    mod_sect = TRUE;
    break;

  case SECT_FOREST:
  case SECT_HILLS:
//      chance -= 15;
    mod -= 0.15;
    mod_sect = TRUE;
    break;

  case SECT_MOUNTAIN:
//      chance -= 20;
    mod -= 0.20;
    mod_sect = TRUE;
    break;
  }

  if (IS_SET(world[vict->in_room].room_flags, SINGLE_FILE))
  {
    if (!mod_sect)
      mod -= 0.25;              //chance -= 25;
    else
      mod -= 0.15;              //chance -= 15;
  }

  // haste/blur helps a little, cuz I'm a nice guy

  if (GET_CLASS(vict) != CLASS_MONK)
  {                             /* mon are not affected by haste or slow */
    if (IS_AFFECTED(vict, AFF_HASTE) || IS_AFFECTED3(vict, AFF3_BLUR))
    {
      //    chance += 10;
      mod += 0.10;
    }

    // slowness hurts a tad

    if (IS_AFFECTED2(vict, AFF2_SLOW))
    {
      //    chance -= 15;
      mod -= 0.15;
    }
  }

  // the character's haste/slowness affects it too
  if (GET_CLASS(attacker) != CLASS_MONK)
  {                             /* mon are not affected by haste or slow */

    if (attacker &&
        (IS_AFFECTED(attacker, AFF_HASTE) ||
         IS_AFFECTED3(attacker, AFF3_BLUR)))
    {
      //    chance -= 5;
      mod -= 0.05;
    }

    // same deal with slowness

    if (attacker && IS_AFFECTED2(attacker, AFF2_SLOW))
    {
      //    chance += 10;
      mod += 0.10;
    }
  }

  // a blow aimed at the arms/hands is gonna be easier to dodge

/*
  if (targetisArms(body_loc_target) || targetisHands(body_loc_target))
//    chance += 10;
    mod += 0.10;
*/

  // take into account the number of people attacking the poor bastard

  for (tch = world[vict->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting == vict)
      numb_att++;
  }

  if (numb_att == 2)
//    chance -= 10;
    mod -= 0.10;
  else if (numb_att == 3)
//    chance -= 25;
    mod -= 0.25;
  else if (numb_att == 4)
//    chance -= 40;
    mod -= 0.40;
  else if (numb_att == 5)
//    chance -= 60;
    mod -= 0.60;
  else
//    chance -= (numb_att * 14);
    mod -= ((float) numb_att * 14.0) / 100.0;

  // drow elves get a bonus, cuz..  they're drow..

  if (GET_RACE(vict) == RACE_DROW)
    chance += 10;               //chance += 15;

  // monk get a bonus
  if (GET_CLASS(vict) == CLASS_MONK)
    chance += 10;               //chance += 15;

  // add weight stuff and maybe weapon-based stuff here ...

  chance = (float) chance *mod;

  return BOUNDED(0, chance, 100);
}


/*
 * getDodgeEaseString
 */

const char *getDodgeEaseString(const int passedby, const int tochar)
{
  if (passedby > 85)
    if (tochar)
      return "have no trouble dodging";
    else
      return "has no trouble dodging";
  else if (passedby > 60)
    if (tochar)
      return "easily dodge";
    else
      return "easily dodges";
  else if (passedby > 40)
    if (tochar)
      return "dodge";
    else
      return "dodges";
  else if (passedby > 20)
    if (tochar)
      return "barely dodge";
    else
      return "barely dodges";
  else if (tochar)
    return "narrowly miss being hit by";
  else
    return "narrowly misses being hit by";
}
#endif
