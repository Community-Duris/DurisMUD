#include <stdio.h>
#include <cstring>
#include <vector>
using namespace std;

#include "comm.h"
#include "interp.h"
#include "utils.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "sql.h"
#include "epic.h"
#include "specializations.h"

extern P_room world;
extern char *specdata[][MAX_SPEC];
extern const struct class_names class_names_table[];
extern const struct race_names race_names_table[];

#define SPEC_ALL 0

// this is the list of class specialziations blocked for certain races
struct allowed_race_spec_struct {
  int race;
  uint m_class;
  int spec;
} allowed_race_specs[] =
{
    /* War Ran Psi Pal APa Cle Mon Dru Sha Sor Nec Con Rog Mer Bar Ber Rea Illu Dre Eth Ave The */
	/* RACE_HUMAN,     RACE_BARBARIAN, RACE_DROW,      RACE_GREY,      RACE_MOUNTAIN, RACE_DUERGAR,
	 * RACE_HALFLING,  RACE_GNOME,     RACE_OGRE,      RACE_TROLL,     RACE_HALFELF,  RACE_ORC,
	 * RACE_THRIKREEN, RACE_CENTAUR,   RACE_GITHYANKI, RACE_MINOTAUR,  RACE_GOBLIN,   RACE_LICH,
	 * RACE_PVAMPIRE,  RACE_GITHZERAI, RACE_DRIDER,    RACE_AGATHINON, RACE_ELADRIN,  RACE_KOBOLD,
	 * RACE_PILLITHID, RACE_KUOTOA,    RACE_WOODELF,   RACE_FIRBOLG */
	/* Start Human Options */
  {RACE_HUMAN, CLASS_WARRIOR,     SPEC_GUARDIAN},
  {RACE_HUMAN, CLASS_WARRIOR,     SPEC_SWORDSMAN},
  {RACE_HUMAN, CLASS_WARRIOR,     SPEC_SWASHBUCKLER},
	{RACE_HUMAN, CLASS_MERCENARY,   SPEC_ALL},
  {RACE_HUMAN, CLASS_PALADIN,     SPEC_ALL},
  {RACE_HUMAN, CLASS_RANGER,      SPEC_ALL},
	{RACE_HUMAN, CLASS_DRUID,       SPEC_ALL},
	{RACE_HUMAN, CLASS_ROGUE,       SPEC_THIEF},
	{RACE_HUMAN, CLASS_ROGUE,       SPEC_ASSASSIN},
  {RACE_HUMAN, CLASS_CLERIC,      SPEC_ALL},
	{RACE_HUMAN, CLASS_SORCERER,    SPEC_ALL},
	{RACE_HUMAN, CLASS_CONJURER,    SPEC_ALL},
	{RACE_HUMAN, CLASS_SUMMONER,    SPEC_ALL},
	{RACE_HUMAN, CLASS_ETHERMANCER, SPEC_ALL},
 	{RACE_HUMAN, CLASS_MONK,        SPEC_ALL},
	{RACE_HUMAN, CLASS_NECROMANCER, SPEC_ALL},
	{RACE_HUMAN, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_HUMAN, CLASS_BERSERKER,   SPEC_ALL},
	{RACE_HUMAN, CLASS_BARD,        SPEC_ALL},
	{RACE_HUMAN, CLASS_SHAMAN,      SPEC_ALL},
	{RACE_HUMAN, CLASS_SHAMAN,      SPEC_ALL},
  {RACE_HUMAN, CLASS_PSIONICIST,  SPEC_ALL},
	/* End Human Options */

	/* Start Orc Options */
	{RACE_ORC, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_ORC, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_ORC, CLASS_WARRIOR,       SPEC_SWASHBUCKLER},
  {RACE_ORC, CLASS_PSIONICIST,    SPEC_ALL},
	{RACE_ORC, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_ORC, CLASS_ANTIPALADIN,   SPEC_ALL},
	{RACE_ORC, CLASS_REAVER,        SPEC_ALL},
	{RACE_ORC, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_ORC, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_ORC, CLASS_CLERIC,        SPEC_ALL},
  {RACE_ORC, CLASS_SORCERER,      SPEC_ALL},
	{RACE_ORC, CLASS_CONJURER,      SPEC_ALL},
	{RACE_ORC, CLASS_SUMMONER,      SPEC_ALL},
	{RACE_ORC, CLASS_ETHERMANCER,   SPEC_ALL},
  {RACE_ORC, CLASS_MONK, 	        SPEC_ALL},
	{RACE_ORC, CLASS_NECROMANCER,   SPEC_ALL},
  {RACE_ORC, CLASS_ILLUSIONIST,   SPEC_ALL},
	{RACE_ORC, CLASS_BERSERKER,     SPEC_ALL},
	{RACE_ORC, CLASS_BARD,          SPEC_ALL},
	{RACE_ORC, CLASS_SHAMAN,        SPEC_ALL},
	{RACE_ORC, CLASS_BLIGHTER,      SPEC_ALL},
	/* End Orc Options */

	/* Start Mountain Dwarf Options */
	{RACE_MOUNTAIN, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_MOUNTAIN, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_MOUNTAIN, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_MOUNTAIN, CLASS_PALADIN,      SPEC_ALL},
	{RACE_MOUNTAIN, CLASS_DRUID,        SPEC_ALL},
	{RACE_MOUNTAIN, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_MOUNTAIN, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_MOUNTAIN, CLASS_CLERIC,       SPEC_ALL},
	{RACE_MOUNTAIN, CLASS_BERSERKER,    SPEC_ALL},
	/* End Mountain Dwarf Options */

	/* Start Duergar Dwarf Options */
	{RACE_DUERGAR, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_DUERGAR, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_DUERGAR, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_DUERGAR, CLASS_ANTIPALADIN,   SPEC_ALL},
	{RACE_DUERGAR, CLASS_REAVER,        SPEC_EARTH_REAVER},
	{RACE_DUERGAR, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_DUERGAR, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_DUERGAR, CLASS_CLERIC,        SPEC_ALL},
	{RACE_DUERGAR, CLASS_BERSERKER,     SPEC_ALL},
	{RACE_DUERGAR, CLASS_BLIGHTER,      SPEC_ALL},
  /* End Duergar Dwarf Options */

	/* Start Centaur Options */
  {RACE_CENTAUR, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_CENTAUR, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_CENTAUR, CLASS_PALADIN,     SPEC_CRUSADER},
	{RACE_CENTAUR, CLASS_RANGER,      SPEC_ALL},
	{RACE_CENTAUR, CLASS_DRUID,       SPEC_ALL},
	{RACE_CENTAUR, CLASS_SHAMAN,      SPEC_ALL},
	/* End Centaur Options */

	/* Start Drider Options */
  {RACE_DRIDER, CLASS_WARRIOR,        SPEC_SWORDSMAN},
  {RACE_DRIDER, CLASS_WARRIOR,        SPEC_GUARDIAN},
  {RACE_DRIDER, CLASS_ANTIPALADIN,    SPEC_DARKKNIGHT},
  {RACE_DRIDER, CLASS_SORCERER,       SPEC_ALL},
  {RACE_DRIDER, CLASS_REAVER,         SPEC_ALL},
  {RACE_DRIDER, CLASS_NECROMANCER,    SPEC_ALL},
	{RACE_DRIDER, CLASS_BLIGHTER,       SPEC_ALL},
	/* End Drider Options */

	/* Start Barbarian Options */
  {RACE_BARBARIAN, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_BARBARIAN, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_BARBARIAN, CLASS_MERCENARY,   SPEC_ALL},
	{RACE_BARBARIAN, CLASS_ROGUE,       SPEC_THIEF},
	{RACE_BARBARIAN, CLASS_ROGUE,       SPEC_ASSASSIN},
	{RACE_BARBARIAN, CLASS_BERSERKER,   SPEC_ALL},
	{RACE_BARBARIAN, CLASS_SHAMAN,      SPEC_ALL},
	/* End Barbarian Options */

	/* Start Troll Options */
	{RACE_TROLL, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_TROLL, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_TROLL, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_TROLL, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_TROLL, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_TROLL, CLASS_BERSERKER,     SPEC_ALL},
	{RACE_TROLL, CLASS_SHAMAN,        SPEC_ALL},
	/* End Troll Options */

	/* Start Grey Elf Options */
	{RACE_GREY, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_GREY, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_GREY, CLASS_WARRIOR,      SPEC_SWASHBUCKLER},
	{RACE_GREY, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_GREY, CLASS_PALADIN,      SPEC_ALL},
	{RACE_GREY, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_GREY, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_GREY, CLASS_DRUID,        SPEC_ALL},
	{RACE_GREY, CLASS_RANGER,       SPEC_ALL},
	{RACE_GREY, CLASS_CLERIC,       SPEC_ALL},
	{RACE_GREY, CLASS_SORCERER,     SPEC_ALL},
	{RACE_GREY, CLASS_CONJURER,     SPEC_ALL},
	{RACE_GREY, CLASS_SUMMONER,     SPEC_ALL},
	{RACE_GREY, CLASS_ETHERMANCER,  SPEC_ALL},
	{RACE_GREY, CLASS_ILLUSIONIST,  SPEC_ALL},
	{RACE_GREY, CLASS_BARD,         SPEC_ALL},
	{RACE_GREY, CLASS_SHAMAN,       SPEC_ALL},
  {RACE_GREY, CLASS_PSIONICIST,   SPEC_ALL},
 	{RACE_GREY, CLASS_MONK,         SPEC_ALL},
	/* End Grey Elf Options */

	/* Start Tiefling Options */
	{RACE_TIEFLING, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_TIEFLING, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_TIEFLING, CLASS_WARRIOR,      SPEC_SWASHBUCKLER},
	{RACE_TIEFLING, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_TIEFLING, CLASS_PALADIN,      SPEC_ALL},
	{RACE_TIEFLING, CLASS_ANTIPALADIN,  SPEC_ALL},
	{RACE_TIEFLING, CLASS_REAVER,       SPEC_ALL},
	{RACE_TIEFLING, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_TIEFLING, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_TIEFLING, CLASS_DRUID,        SPEC_ALL},
	{RACE_TIEFLING, CLASS_RANGER,       SPEC_ALL},
	{RACE_TIEFLING, CLASS_CLERIC,       SPEC_ALL},
	{RACE_TIEFLING, CLASS_SORCERER,     SPEC_ALL},
	{RACE_TIEFLING, CLASS_CONJURER,     SPEC_ALL},
	{RACE_TIEFLING, CLASS_SUMMONER,     SPEC_ALL},
	{RACE_TIEFLING, CLASS_ETHERMANCER,  SPEC_ALL},
	{RACE_TIEFLING, CLASS_ILLUSIONIST,  SPEC_ALL},
	{RACE_TIEFLING, CLASS_BARD,         SPEC_ALL},
	{RACE_TIEFLING, CLASS_SHAMAN,       SPEC_ALL},
  {RACE_TIEFLING, CLASS_PSIONICIST,   SPEC_ALL},
 	{RACE_TIEFLING, CLASS_MONK,         SPEC_ALL},
	/* End Tiefling Options */

	/* Start Drow Elf Options */
	{RACE_DROW, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_DROW, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_DROW, CLASS_WARRIOR,      SPEC_SWASHBUCKLER},
	{RACE_DROW, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_DROW, CLASS_ANTIPALADIN,  SPEC_ALL},
	{RACE_DROW, CLASS_REAVER,       SPEC_ALL},
	{RACE_DROW, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_DROW, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_DROW, CLASS_CLERIC,       SPEC_ALL},
  {RACE_DROW, CLASS_SORCERER,     SPEC_ALL},
	{RACE_DROW, CLASS_CONJURER,     SPEC_ALL},
	{RACE_DROW, CLASS_SUMMONER,     SPEC_ALL},
	{RACE_DROW, CLASS_ETHERMANCER,  SPEC_ALL},
	{RACE_DROW, CLASS_NECROMANCER,  SPEC_ALL},
  {RACE_DROW, CLASS_ILLUSIONIST,  SPEC_ALL},
	{RACE_DROW, CLASS_BARD,         SPEC_ALL},
	{RACE_DROW, CLASS_SHAMAN,       SPEC_ALL},
	{RACE_DROW, CLASS_PSIONICIST,   SPEC_ALL},
 	{RACE_DROW, CLASS_MONK,         SPEC_ALL},
	{RACE_DROW, CLASS_BLIGHTER,     SPEC_ALL},
	/* End Drow Elf Options */

	/* Start Gnome Options */
  {RACE_GNOME, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_GNOME, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_GNOME, CLASS_MERCENARY,   SPEC_ALL},
	{RACE_GNOME, CLASS_CLERIC,      SPEC_ALL},
	{RACE_GNOME, CLASS_SORCERER,    SPEC_ALL},
	{RACE_GNOME, CLASS_CONJURER,    SPEC_ALL},
	{RACE_GNOME, CLASS_SUMMONER,    SPEC_ALL},
	{RACE_GNOME, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_GNOME, CLASS_BARD,        SPEC_ALL},
	{RACE_GNOME, CLASS_SHAMAN,      SPEC_ALL},
	{RACE_GNOME, CLASS_NECROMANCER, SPEC_ALL},
	/* End Gnome Options */

	/* Start Kobold Options */
  {RACE_KOBOLD, CLASS_WARRIOR,      SPEC_GUARDIAN},
  {RACE_KOBOLD, CLASS_WARRIOR,      SPEC_SWORDSMAN},
  {RACE_KOBOLD, CLASS_MERCENARY,    SPEC_ALL},
  {RACE_KOBOLD, CLASS_CLERIC,       SPEC_ALL},
  {RACE_KOBOLD, CLASS_SORCERER,     SPEC_ALL},
  {RACE_KOBOLD, CLASS_CONJURER,     SPEC_ALL},
  {RACE_KOBOLD, CLASS_SUMMONER,     SPEC_ALL},
  {RACE_KOBOLD, CLASS_ETHERMANCER,  SPEC_ALL},
  {RACE_KOBOLD, CLASS_NECROMANCER,  SPEC_ALL},
  {RACE_KOBOLD, CLASS_ILLUSIONIST,  SPEC_ALL},
  {RACE_KOBOLD, CLASS_BARD,         SPEC_ALL},
  {RACE_KOBOLD, CLASS_SHAMAN,       SPEC_ALL},
	/* End Kobold Options */

	/* Start Halfling Options */
  {RACE_HALFLING, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_HALFLING, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_HALFLING, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_HALFLING, CLASS_RANGER,       SPEC_ALL},
	{RACE_HALFLING, CLASS_DRUID,        SPEC_ALL},
	{RACE_HALFLING, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_HALFLING, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_HALFLING, CLASS_CLERIC,       SPEC_ALL},
	{RACE_HALFLING, CLASS_SORCERER,     SPEC_ALL},
	{RACE_HALFLING, CLASS_CONJURER,     SPEC_ALL},
	{RACE_HALFLING, CLASS_SUMMONER,     SPEC_ALL},
	{RACE_HALFLING, CLASS_ETHERMANCER,  SPEC_ALL},
	{RACE_HALFLING, CLASS_ILLUSIONIST,  SPEC_ALL},
	{RACE_HALFLING, CLASS_BARD,         SPEC_ALL},
	{RACE_HALFLING, CLASS_SHAMAN,       SPEC_ALL},
	/* End Halfling Options */

	/* Start Goblin Options */
	{RACE_GOBLIN, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_GOBLIN, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_GOBLIN, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_GOBLIN, CLASS_ANTIPALADIN,   SPEC_ALL},
	{RACE_GOBLIN, CLASS_REAVER,        SPEC_ALL},
	{RACE_GOBLIN, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_GOBLIN, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_GOBLIN, CLASS_CLERIC,        SPEC_ALL},
  {RACE_GOBLIN, CLASS_SORCERER,      SPEC_ALL},
	{RACE_GOBLIN, CLASS_CONJURER,      SPEC_ALL},
	{RACE_GOBLIN, CLASS_SUMMONER,      SPEC_ALL},
	{RACE_GOBLIN, CLASS_ETHERMANCER,   SPEC_ALL},
	{RACE_GOBLIN, CLASS_NECROMANCER,   SPEC_ALL},
  {RACE_GOBLIN, CLASS_ILLUSIONIST,   SPEC_ALL},
	{RACE_GOBLIN, CLASS_BARD,          SPEC_ALL},
	{RACE_GOBLIN, CLASS_SHAMAN,        SPEC_ALL},
	{RACE_GOBLIN, CLASS_BLIGHTER,      SPEC_ALL},
	/* End Goblin Options */

	/* Start Githzerai Options */
  {RACE_GITHZERAI, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_GITHZERAI, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_GITHZERAI, CLASS_PALADIN,     SPEC_ALL},
	{RACE_GITHZERAI, CLASS_RANGER,      SPEC_ALL},
	{RACE_GITHZERAI, CLASS_DRUID,       SPEC_ALL},
	{RACE_GITHZERAI, CLASS_CLERIC,      SPEC_ALL},
	{RACE_GITHZERAI, CLASS_SORCERER,    SPEC_ALL},
	{RACE_GITHZERAI, CLASS_CONJURER,    SPEC_ALL},
	{RACE_GITHZERAI, CLASS_SUMMONER,    SPEC_ALL},
	{RACE_GITHZERAI, CLASS_ETHERMANCER, SPEC_ALL},
 	{RACE_GITHZERAI, CLASS_MONK,        SPEC_ALL},
	{RACE_GITHZERAI, CLASS_NECROMANCER, SPEC_ALL},
	{RACE_GITHZERAI, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_GITHZERAI, CLASS_BARD,        SPEC_ALL},
  {RACE_GITHZERAI, CLASS_PSIONICIST,  SPEC_ALL},
	/* End Githzerai Options */

	/* Start Githyanki Options */
	{RACE_GITHYANKI, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_GITHYANKI, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_GITHYANKI, CLASS_ANTIPALADIN,   SPEC_ALL},
	{RACE_GITHYANKI, CLASS_REAVER,        SPEC_ALL},
	{RACE_GITHYANKI, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_GITHYANKI, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_GITHYANKI, CLASS_CLERIC,        SPEC_ALL},
  {RACE_GITHYANKI, CLASS_SORCERER,      SPEC_ALL},
	{RACE_GITHYANKI, CLASS_CONJURER,      SPEC_ALL},
	{RACE_GITHYANKI, CLASS_SUMMONER,      SPEC_ALL},
	{RACE_GITHYANKI, CLASS_ETHERMANCER,   SPEC_ALL},
  {RACE_GITHYANKI, CLASS_MONK, 	        SPEC_ALL},
	{RACE_GITHYANKI, CLASS_NECROMANCER,   SPEC_ALL},
  {RACE_GITHYANKI, CLASS_ILLUSIONIST,   SPEC_ALL},
	{RACE_GITHYANKI, CLASS_BARD,          SPEC_ALL},
	{RACE_GITHYANKI, CLASS_PSIONICIST,    SPEC_ALL},
	{RACE_GITHYANKI, CLASS_BLIGHTER,      SPEC_ALL},
  /* End Githyanki Options */

  /* Start Firbolg Options */
  {RACE_FIRBOLG, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_FIRBOLG, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_FIRBOLG, CLASS_SHAMAN,      SPEC_ALL},
	/* End Firbolg Options */

	/* Start Ogre Options */
	{RACE_OGRE, CLASS_WARRIOR,   SPEC_GUARDIAN},
	{RACE_OGRE, CLASS_WARRIOR,   SPEC_SWORDSMAN},
	{RACE_OGRE, CLASS_SHAMAN,        SPEC_ALL},
	/* End Ogre Options */

	/* Start Half-Elf Options */
/*	{RACE_HALFELF, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_HALFELF, CLASS_WARRIOR,     SPEC_SWASHBUCKLER},
	{RACE_HALFELF, CLASS_RANGER,      SPEC_BLADEMASTER},
	{RACE_HALFELF, CLASS_PALADIN,     SPEC_ALL},
	{RACE_HALFELF, CLASS_CLERIC,      SPEC_ZEALOT},
	{RACE_HALFELF, CLASS_DRUID,       SPEC_STORM},
	{RACE_HALFELF, CLASS_SORCERER,    SPEC_ALL},
	{RACE_HALFELF, CLASS_CONJURER,    SPEC_ALL},
	{RACE_HALFELF, CLASS_SUMMONER,    SPEC_ALL},
	{RACE_HALFELF, CLASS_ROGUE,       SPEC_ALL},
	{RACE_HALFELF, CLASS_MERCENARY,   SPEC_OPPORTUNIST},
	{RACE_HALFELF, CLASS_BARD,        SPEC_ALL},
	{RACE_HALFELF, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_HALFELF, CLASS_THEURGIST,   SPEC_TEMPLAR},
	{RACE_HALFELF, CLASS_THEURGIST,   SPEC_THAUMATURGE},
*/	/* End Half-Elf Options */

	/* Start Thri-Kreen Options */
	{RACE_THRIKREEN, CLASS_WARRIOR, SPEC_SWORDSMAN},
	/* End Thri-Kreen Options */

	/* Start Minotaur Options */
	{RACE_MINOTAUR, CLASS_WARRIOR,   SPEC_GUARDIAN},
	{RACE_MINOTAUR, CLASS_WARRIOR,   SPEC_SWORDSMAN},
	{RACE_MINOTAUR, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_MINOTAUR, CLASS_SHAMAN,        SPEC_ALL},
  {RACE_MINOTAUR, CLASS_SORCERER,      SPEC_ALL},
	{RACE_MINOTAUR, CLASS_BERSERKER,     SPEC_ALL},
 	/* End Minotaur Options */

	/* Start Lich Options */
	{RACE_LICH, CLASS_NECROMANCER, SPEC_ALL},
	/* End Lich Options */

	/* Start Vampire Options */
	//{RACE_PVAMPIRE, CLASS_DREADLORD, SPEC_ALL},
	/* End Vampire Options */

	/* Start Agathinon Options */
	//{RACE_AGATHINON, CLASS_AVENGER, SPEC_ALL},
	/* End Agathinon Options */

	/* Start Eladrin Options */
	//{RACE_ELADRIN, CLASS_THEURGIST, SPEC_ALL},
	/* End Eladrin Options */

	/* Start Illithid Options */
	//{RACE_PILLITHID, CLASS_WARRIOR,     SPEC_SWORDSMAN},
/*	{RACE_PILLITHID, CLASS_PSIONICIST,  SPEC_ALL},
	{RACE_PILLITHID, CLASS_SORCERER,    SPEC_SHADOW},
	{RACE_PILLITHID, CLASS_SORCERER,    SPEC_WIZARD},
	{RACE_PILLITHID, CLASS_CONJURER,    SPEC_ALL},
	{RACE_PILLITHID, CLASS_SUMMONER,    SPEC_ALL},
	//{RACE_PILLITHID, CLASS_ILLUSIONIST, SPEC_DARK_DREAMER},
	//{RACE_PILLITHID, CLASS_ILLUSIONIST, SPEC_DECEIVER},
	//{RACE_PILLITHID, CLASS_ETHERMANCER, SPEC_STARMAGUS},
*/	/* End Illithid Options */

	/* Start Kuo Toa Options */
/*	{RACE_KUOTOA, CLASS_WARRIOR, SPEC_GUARDIAN},
	{RACE_KUOTOA, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_KUOTOA, CLASS_CLERIC,  SPEC_ZEALOT},
	{RACE_KUOTOA, CLASS_SHAMAN,  SPEC_ELEMENTALIST},
	{RACE_KUOTOA, CLASS_ROGUE,   SPEC_ASSASSIN},
*/	/* End Kuo Toa Options */

	/* Start Wood Elf Options */
/*	{RACE_WOODELF, CLASS_WARRIOR, SPEC_GUARDIAN},
	{RACE_WOODELF, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_WOODELF, CLASS_RANGER,  SPEC_ALL},
	{RACE_WOODELF, CLASS_DRUID,   SPEC_WOODLAND},
	{RACE_WOODELF, CLASS_ROGUE,   SPEC_ASSASSIN},
	{RACE_WOODELF, CLASS_ROGUE,   SPEC_THIEF},
*/	/* End Wood Elf Options */
	
	
	//{0}
};

bool is_allowed_race_spec(int race, uint m_class, int spec)
{
  for( int i = 0; allowed_race_specs[i].race; i++ )
  {
    if( allowed_race_specs[i].race == race &&
        allowed_race_specs[i].m_class == m_class &&
        (allowed_race_specs[i].spec == SPEC_ALL ||
         allowed_race_specs[i].spec == spec ) )
    {
      return TRUE;
    }
  }
  return FALSE;
}

bool append_valid_specs(char *buf, P_char ch)
{
  if( !ch )
    return false;
  
  bool found_one = false;
  for (int i = 0; i < MAX_SPEC; i++)
  {
    if ( *GET_SPEC_NAME(ch->player.m_class, i) && 
         is_allowed_race_spec(GET_RACE(ch), ch->player.m_class, i+1) )
    {
      strcat(buf, GET_SPEC_NAME(ch->player.m_class, i));
      strcat(buf, "\r\n");
      found_one = true;
    }
  }
  
  return found_one;  
}

string single_spec_list(int race, int cls)
{
  int spec, comma = 0;
  string return_str;

  for (spec = 0; spec < MAX_SPEC; spec++)
  {
    if (!strcmp(specdata[cls][spec], "") ||
        !strcmp(specdata[cls][spec], "Not Used"))
    {
      continue;
    }
    if (is_allowed_race_spec(race, 1 << (cls - 1), spec+1))
    {
      if (comma)
	return_str += "&n, ";
      else
	comma = 1;
	
      return_str += string(specdata[cls][spec]);
    }
  }

  return return_str;
}

void do_spec_list(P_char ch)
{
  char Gbuf[MAX_STRING_LENGTH], list[MAX_STRING_LENGTH];
  int race, cls, spec, show;
  bool comma;

  send_to_char("&+WCurrent list of specializations available:&n\n\n", ch);
  for( cls = 1; cls <= CLASS_COUNT; cls++ )
  {
    // Look for valid spec for the class.
    for( spec = 0; spec < MAX_SPEC; spec++ )
    {
      show = 0;
      if (strcmp(specdata[cls][spec], "")
        && strcmp(specdata[cls][spec], "Not Used"))
      {
        show = 1;
        break;
      }
    }
    if( show )
    {
      snprintf(Gbuf, MAX_STRING_LENGTH, "&+W*&n %s &+W*&n\n", class_names_table[cls].ansi);
      send_to_char(Gbuf, ch);
    }
    // Walk through each spec.
    for( spec = 0; spec < MAX_SPEC; spec++ )
    {
      if( strcmp(specdata[cls][spec], "") && strcmp(specdata[cls][spec], "Not Used") )
      {
        snprintf(list, MAX_STRING_LENGTH, " %s:", specdata[cls][spec]);
        comma = FALSE;
        for( race = 1; race <= RACE_PLAYER_MAX; race++ )
        {
          // If race has the spec, add it to the list.
          if( is_allowed_race_spec(race, 1 << (cls - 1), spec+1) )
          {
            if( comma )
            {
              snprintf(list + strlen(list), MAX_STRING_LENGTH - strlen(list), ", %s&n", race_names_table[race].ansi);
            }
            else
            {
              comma = TRUE;
              snprintf(list + strlen(list), MAX_STRING_LENGTH - strlen(list), " %s&n", race_names_table[race].ansi);
            }
          }
        }
        send_to_char(list, ch);
        send_to_char("\n", ch);
      }
    }
    if( show )
    {
      send_to_char("\n", ch);
    }
  }
}

void do_specialize(P_char ch, char *argument, int cmd)
{
  P_char teacher;
  int      i;
  char     buf[MAX_STRING_LENGTH];
  bool found_one;

  if( !strcmp(argument, "list") )
  {
    do_spec_list(ch);
    return;
  }

  if( IS_MULTICLASS_PC(ch) )
  {
    send_to_char("You have already chosen another path!\n", ch);
    return;
  }

  if( !*argument )
  {
    snprintf(buf, MAX_STRING_LENGTH, "You can choose from the following specializations:\n\r");

    found_one = append_valid_specs(buf, ch);;

    if( !found_one )
    {
      send_to_char("There are no specializations available to you.\r\n", ch);
      return;
    }

    send_to_char(buf, ch);
    return;
  }

  for( teacher = world[ch->in_room].people; teacher; teacher = teacher->next_in_room )
  {
    if( IS_NPC(teacher) && GET_CLASS(teacher, ch->player.m_class)
      && IS_SET(teacher->specials.act, ACT_SPEC_TEACHER) )
    {
      break;
    }
    // Allowing people to spec from a God if they have consent.
    if( IS_PC(teacher) && IS_TRUSTED(teacher) && is_linked_to( ch, teacher, LNK_CONSENT)
      && ch != teacher )
    {
      break;
    }
  }

  if( !teacher )
  {
    send_to_char("You need to find a teacher first.\n", ch);
    return;
  }

  if( IS_NPC(ch) )
  {
    mobsay(teacher, "Please stop trying to crash the game by ordering pets to specialize.");
    return;
  }

  if( teacher->player.m_class != ch->player.m_class
    && !(IS_PC(teacher) && IS_TRUSTED(teacher) && is_linked_to(ch, teacher, LNK_CONSENT)) )
  {
    mobsay(teacher, "I know nothing of your kind. Be gone.");
    return;
  }

  if( IS_SPECIALIZED(ch) )
  {
    mobsay(teacher, "You are already specialized.");
    return;
  }

  while (*argument == ' ')
    argument++;

  if( time(NULL) < ch->only.pc->time_unspecced )
  {
    snprintf(buf, MAX_STRING_LENGTH, "You cannot specialize until %s.\r\n",
            asctime(localtime(&ch->only.pc->time_unspecced)));
    send_to_char(buf, ch);
    return;
  }

  if (GET_LEVEL(ch) < 30)
  {
    mobsay(teacher, "You are not yet experienced enough to specialize.");
    return;
  }

  for (i = 0; i < MAX_SPEC; i++)
  {
    if( !*GET_SPEC_NAME(ch->player.m_class, i) )
      continue;

    if( !is_allowed_race_spec(GET_RACE(ch), ch->player.m_class, i+1) )
      continue;

    if( is_abbrev( argument, strip_ansi(GET_SPEC_NAME(ch->player.m_class, i)).c_str()) )
    {
      snprintf(buf, MAX_STRING_LENGTH, "From this day onwards you will follow the path of the %s&n!", GET_SPEC_NAME(ch->player.m_class, i));
      ch->player.spec = i+1;
      update_skills(ch);
      mobsay(teacher, buf);
      return;
    }
  }

  mobsay(teacher, "I'm sorry, but that specialization isn't available to you.");
}

void unspecialize(P_char ch, P_obj obj)
{
  if (!IS_SPECIALIZED(ch)) {
    send_to_char("You pray to the &+bWater Goddess&n but you get no response.",
                 ch);
  } if (GET_EPIC_POINTS(ch) < 10) {
    send_to_char("You need 10 epic points to pay for this.\n", ch);
  } else {
    act("You kneel in front of $p and pray to the \n"
        "&+bWater Goddess&n. As you continue your meditation, you begin\n"
        "to feel your mind is breaking free from the old habits and you\n"
        "feel ready to learn new ways.\n", FALSE, ch, obj, 0, TO_CHAR);
    act("$n kneels before $p and sinks in prayers.\n"
        "After few moments of silence $e smiles and stands up looking reborn.\n", FALSE, ch, obj, 0, TO_ROOM);
    ch->player.spec = 0;
    update_skills(ch);
    //epic_gain_skillpoints(ch, -1);
	ch->only.pc->epics -= 10;
    forget_spells(ch, -1);
  }
}
