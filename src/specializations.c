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
	 * RACE_THRIKREEN, RACE_CENTAUR,   RACE_GITHYANKI, RACE_MINOTAUR,  RACE_GOBLIN,   RACE_PLICH,
	 * RACE_PVAMPIRE,  RACE_GITHZERAI, RACE_DRIDER,    RACE_AGATHINON, RACE_ELADRIN,  RACE_KOBOLD,
	 * RACE_PILLITHID, RACE_KUOTOA,    RACE_WOODELF,   RACE_FIRBOLG */
	/* Start Human Options */
	{RACE_HUMAN, CLASS_WARRIOR,     SPEC_ALL},
	{RACE_HUMAN, CLASS_RANGER,      SPEC_ALL},
	{RACE_HUMAN, CLASS_PALADIN,     SPEC_ALL},
	{RACE_HUMAN, CLASS_CLERIC,      SPEC_ALL},
	{RACE_HUMAN, CLASS_DRUID,       SPEC_ALL},
	{RACE_HUMAN, CLASS_SHAMAN,      SPEC_ALL},
	{RACE_HUMAN, CLASS_SHAMAN,      SPEC_ALL},
	{RACE_HUMAN, CLASS_SORCERER,    SPEC_ALL},
	{RACE_HUMAN, CLASS_CONJURER,    SPEC_ALL},
	{RACE_HUMAN, CLASS_ROGUE,       SPEC_ALL},
	{RACE_HUMAN, CLASS_MERCENARY,   SPEC_ALL},
	{RACE_HUMAN, CLASS_BARD,        SPEC_ALL},
	{RACE_HUMAN, CLASS_BERSERKER,   SPEC_ALL},
	{RACE_HUMAN, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_HUMAN, CLASS_ETHERMANCER, SPEC_ALL},
	{RACE_HUMAN, CLASS_THEURGIST,   SPEC_ALL},
	/* End Human Options */

	/* Start Barbarian Options */
	{RACE_BARBARIAN, CLASS_WARRIOR,   SPEC_GUARDIAN},
	{RACE_BARBARIAN, CLASS_WARRIOR,   SPEC_SWORDSMAN},
	{RACE_BARBARIAN, CLASS_SHAMAN,    SPEC_SPIRITUALIST},
	{RACE_BARBARIAN, CLASS_MERCENARY, SPEC_OPPORTUNIST},
	/* End Barbarian Options */

	/* Start Drow Elf Options */
	{RACE_DROW, CLASS_WARRIOR,     SPEC_SWASHBUCKLER},
	{RACE_DROW, CLASS_CLERIC,      SPEC_HEALER},
	{RACE_DROW, CLASS_CLERIC,      SPEC_ZEALOT},
	{RACE_DROW, CLASS_SORCERER,    SPEC_WIZARD},
	{RACE_DROW, CLASS_NECROMANCER, SPEC_DIABOLIS},
	{RACE_DROW, CLASS_NECROMANCER, SPEC_REAPER},
	{RACE_DROW, CLASS_CONJURER,    SPEC_FIRE},
	{RACE_DROW, CLASS_ROGUE,       SPEC_SHARPSHOOTER},
	{RACE_DROW, CLASS_BARD,        SPEC_MINSTREL},
	{RACE_DROW, CLASS_BARD,        SPEC_SCOUNDREL},
	{RACE_DROW, CLASS_REAVER,      SPEC_FLAME_REAVER},
	{RACE_DROW, CLASS_REAVER,      SPEC_SHOCK_REAVER},
	/* End Drow Elf Options */

	/* Start Grey Elf Options */
	{RACE_GREY, CLASS_WARRIOR,  SPEC_SWASHBUCKLER},
	{RACE_GREY, CLASS_RANGER,   SPEC_WOODSMAN},
	{RACE_GREY, CLASS_CLERIC,   SPEC_HEALER},
	{RACE_GREY, CLASS_CLERIC,   SPEC_HOLYMAN},
	{RACE_GREY, CLASS_DRUID,    SPEC_WOODLAND},
	{RACE_GREY, CLASS_SORCERER, SPEC_WIZARD},
	{RACE_GREY, CLASS_CONJURER, SPEC_FIRE},
	{RACE_GREY, CLASS_ROGUE,    SPEC_SHARPSHOOTER},
	{RACE_GREY, CLASS_BARD,     SPEC_MINSTREL},
	/* End Grey Elf Options */

	/* Start Mountain Dwarf Options */
	{RACE_MOUNTAIN, CLASS_WARRIOR,   SPEC_GUARDIAN},
	{RACE_MOUNTAIN, CLASS_CLERIC,    SPEC_HOLYMAN},
	{RACE_MOUNTAIN, CLASS_CLERIC,    SPEC_ZEALOT},
	{RACE_MOUNTAIN, CLASS_ROGUE,     SPEC_ASSASSIN},
	{RACE_MOUNTAIN, CLASS_MERCENARY, SPEC_BOUNTY},
	{RACE_MOUNTAIN, CLASS_BERSERKER, SPEC_ALL},
	/* End Mountain Dwarf Options */

	/* Start Duergar Dwarf Options */
	{RACE_DUERGAR, CLASS_WARRIOR,   SPEC_SWORDSMAN},
	{RACE_DUERGAR, CLASS_CLERIC,    SPEC_HOLYMAN},
	{RACE_DUERGAR, CLASS_CLERIC,    SPEC_ZEALOT},
	{RACE_DUERGAR, CLASS_ROGUE,     SPEC_ASSASSIN},
	{RACE_DUERGAR, CLASS_ROGUE,     SPEC_THIEF},
	{RACE_DUERGAR, CLASS_MERCENARY, SPEC_BOUNTY},
	{RACE_DUERGAR, CLASS_BERSERKER, SPEC_ALL},
	/* End Duergar Dwarf Options */

	/* Start Halfling Options */
	{RACE_HALFLING, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_HALFLING, CLASS_CLERIC,      SPEC_HEALER},
	{RACE_HALFLING, CLASS_SORCERER,    SPEC_WILDMAGE},
	{RACE_HALFLING, CLASS_ROGUE,       SPEC_THIEF},
	{RACE_HALFLING, CLASS_BARD,        SPEC_SCOUNDREL},
	{RACE_HALFLING, CLASS_ILLUSIONIST, SPEC_DARK_DREAMER},
	/* End Halfling Options */

	/* Start Gnome Options */
	{RACE_GNOME, CLASS_WARRIOR,     SPEC_SWASHBUCKLER},
	{RACE_GNOME, CLASS_CLERIC,      SPEC_ZEALOT},
	{RACE_GNOME, CLASS_SHAMAN,      SPEC_ELEMENTALIST},
	{RACE_GNOME, CLASS_SORCERER,    SPEC_SHADOW},
	{RACE_GNOME, CLASS_CONJURER,    SPEC_EARTH},
	{RACE_GNOME, CLASS_ILLUSIONIST, SPEC_DECEIVER},
	{RACE_GNOME, CLASS_ETHERMANCER, SPEC_COSMOMANCER},
	{RACE_GNOME, CLASS_BARD,        SPEC_DISHARMONIST},
	{RACE_GNOME, CLASS_THEURGIST,   SPEC_MEDIUM},
	/* End Gnome Options */

	/* Start Ogre Options */
	{RACE_OGRE, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_OGRE, CLASS_MERCENARY, SPEC_OPPORTUNIST},
	{RACE_OGRE, CLASS_SHAMAN, SPEC_SPIRITUALIST},
	/* End Ogre Options */

	/* Start Troll Options */
	{RACE_TROLL, CLASS_WARRIOR, SPEC_GUARDIAN},
	{RACE_TROLL, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_TROLL, CLASS_MERCENARY, SPEC_OPPORTUNIST},
	{RACE_TROLL, CLASS_SHAMAN, SPEC_ELEMENTALIST},
	/* End Troll Options */

	/* Start Half-Elf Options */
	{RACE_HALFELF, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_HALFELF, CLASS_WARRIOR,     SPEC_SWASHBUCKLER},
	{RACE_HALFELF, CLASS_RANGER,      SPEC_BLADEMASTER},
	{RACE_HALFELF, CLASS_PALADIN,     SPEC_CAVALIER},
	{RACE_HALFELF, CLASS_CLERIC,      SPEC_ZEALOT},
	{RACE_HALFELF, CLASS_DRUID,       SPEC_STORM},
	{RACE_HALFELF, CLASS_SORCERER,    SPEC_WILDMAGE},
	{RACE_HALFELF, CLASS_SORCERER,    SPEC_WIZARD},
	{RACE_HALFELF, CLASS_CONJURER,    SPEC_AIR},
	{RACE_HALFELF, CLASS_CONJURER,    SPEC_WATER},
	{RACE_HALFELF, CLASS_ROGUE,       SPEC_ALL},
	{RACE_HALFELF, CLASS_MERCENARY,   SPEC_OPPORTUNIST},
	{RACE_HALFELF, CLASS_BARD,        SPEC_ALL},
	{RACE_HALFELF, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_HALFELF, CLASS_THEURGIST,   SPEC_TEMPLAR},
	{RACE_HALFELF, CLASS_THEURGIST,   SPEC_THAUMATURGE},
	/* End Half-Elf Options */

	/* Start Orc Options */
	{RACE_ORC, CLASS_WARRIOR,       SPEC_ALL},
	{RACE_ORC, CLASS_ANTIPALADIN,   SPEC_ALL},
	{RACE_ORC, CLASS_CLERIC,        SPEC_ALL},
	{RACE_ORC, CLASS_SHAMAN,        SPEC_ALL},
	{RACE_ORC, CLASS_SORCERER,      SPEC_ALL},
	{RACE_ORC, CLASS_NECROMANCER,   SPEC_ALL},
	{RACE_ORC, CLASS_CONJURER,      SPEC_ALL},
	{RACE_ORC, CLASS_ROGUE,         SPEC_ALL},
	{RACE_ORC, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_ORC, CLASS_BARD,          SPEC_ALL},
	{RACE_ORC, CLASS_BERSERKER,     SPEC_ALL},
	{RACE_ORC, CLASS_REAVER,        SPEC_ALL},
	{RACE_ORC, CLASS_ILLUSIONIST,   SPEC_ALL},
	{RACE_ORC, CLASS_ETHERMANCER,   SPEC_ALL},
	/* End Orc Options */

	/* Start Thri-Kreen Options */
	{RACE_THRIKREEN, CLASS_WARRIOR, SPEC_SWORDSMAN},
	/* End Thri-Kreen Options */

	/* Start Centaur Options */
	{RACE_CENTAUR, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_CENTAUR, CLASS_RANGER,      SPEC_MARSHALL},
	{RACE_CENTAUR, CLASS_DRUID,       SPEC_STORM},
	{RACE_CENTAUR, CLASS_SHAMAN,      SPEC_ANIMALIST},
	/*{RACE_CENTAUR, CLASS_ETHERMANCER, SPEC_WINDTALKER}, I think not -Kitsero */
	/* End Centaur Options */

	/* Start Githyanki Options */
	{RACE_GITHYANKI, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_GITHYANKI, CLASS_PSIONICIST,  SPEC_PSYCHEPORTER},
	{RACE_GITHYANKI, CLASS_ANTIPALADIN, SPEC_DEMONIC},
	{RACE_GITHYANKI, CLASS_CLERIC,      SPEC_ZEALOT},
	{RACE_GITHYANKI, CLASS_SORCERER,    SPEC_WILDMAGE},
	{RACE_GITHYANKI, CLASS_NECROMANCER, SPEC_NECROLYTE},
	{RACE_GITHYANKI, CLASS_NECROMANCER, SPEC_REAPER},
	{RACE_GITHYANKI, CLASS_REAVER,      SPEC_ICE_REAVER},
	/* End Githyanki Options */

	/* Start Minotaur Options */
	{RACE_MINOTAUR, CLASS_WARRIOR, SPEC_GUARDIAN},
	{RACE_MINOTAUR, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_MINOTAUR, CLASS_SHAMAN, SPEC_ELEMENTALIST},
	{RACE_MINOTAUR, CLASS_SHAMAN, SPEC_SPIRITUALIST},
	{RACE_MINOTAUR, CLASS_MERCENARY, SPEC_BOUNTY},
	{RACE_MINOTAUR, CLASS_BERSERKER, SPEC_ALL},
	/* End Minotaur Options */

	/* Start Goblin Options */
	{RACE_GOBLIN, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_GOBLIN, CLASS_CLERIC, SPEC_HEALER},
	{RACE_GOBLIN, CLASS_SHAMAN, SPEC_ANIMALIST},
	{RACE_GOBLIN, CLASS_ROGUE, SPEC_THIEF},
	{RACE_GOBLIN, CLASS_MERCENARY, SPEC_BOUNTY},
	{RACE_GOBLIN, CLASS_ETHERMANCER, SPEC_FROST_MAGUS},
	{RACE_GOBLIN, CLASS_ETHERMANCER, SPEC_COSMOMANCER},
	/* End Goblin Options */

	/* Start Lich Options */
	{RACE_PLICH, CLASS_NECROMANCER, SPEC_ALL},
	/* End Lich Options */

	/* Start Vampire Options */
	{RACE_PVAMPIRE, CLASS_DREADLORD, SPEC_ALL},
	/* End Vampire Options */

	/* Start Githzerai Options */
	{RACE_GITHZERAI, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_GITHZERAI, CLASS_CLERIC, SPEC_ZEALOT},
	//{RACE_GITHZERAI, CLASS_MONK, SPEC_ALL},
	{RACE_GITHZERAI, CLASS_SORCERER, SPEC_WILDMAGE},
	/* End Githzerai Options */

	/* Start Drider Options */
	{RACE_DRIDER, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_DRIDER, CLASS_SORCERER, SPEC_SHADOW},
	{RACE_DRIDER, CLASS_REAVER, SPEC_ALL},
	/* End Drider Options */

	/* Start Agathinon Options */
	{RACE_AGATHINON, CLASS_AVENGER, SPEC_ALL},
	/* End Agathinon Options */

	/* Start Eladrin Options */
	{RACE_ELADRIN, CLASS_THEURGIST, SPEC_ALL},
	/* End Eladrin Options */

	/* Start Kobold Options */
	{RACE_KOBOLD, CLASS_WARRIOR,     SPEC_SWASHBUCKLER},
	{RACE_KOBOLD, CLASS_CLERIC,      SPEC_ZEALOT},
	{RACE_KOBOLD, CLASS_SHAMAN,      SPEC_SPIRITUALIST},
	{RACE_KOBOLD, CLASS_SORCERER,    SPEC_SHADOW},
	{RACE_KOBOLD, CLASS_NECROMANCER, SPEC_NECROLYTE},
	{RACE_KOBOLD, CLASS_CONJURER,    SPEC_EARTH},
	{RACE_KOBOLD, CLASS_BARD,        SPEC_DISHARMONIST},
	{RACE_KOBOLD, CLASS_ETHERMANCER, SPEC_WINDTALKER},
	/* End Kobold Options */

	/* Start Illithid Options */
	{RACE_PILLITHID, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_PILLITHID, CLASS_PSIONICIST,  SPEC_ALL},
	{RACE_PILLITHID, CLASS_SORCERER,    SPEC_SHADOW},
	{RACE_PILLITHID, CLASS_SORCERER,    SPEC_WIZARD},
	{RACE_PILLITHID, CLASS_CONJURER,    SPEC_AIR},
	{RACE_PILLITHID, CLASS_CONJURER,    SPEC_WATER},
	{RACE_PILLITHID, CLASS_ILLUSIONIST, SPEC_DARK_DREAMER},
	{RACE_PILLITHID, CLASS_ILLUSIONIST, SPEC_DECEIVER},
	{RACE_PILLITHID, CLASS_ETHERMANCER, SPEC_COSMOMANCER},
	/* End Illithid Options */

	/* Start Kuo Toa Options */
	{RACE_KUOTOA, CLASS_WARRIOR, SPEC_GUARDIAN},
	{RACE_KUOTOA, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_KUOTOA, CLASS_CLERIC,  SPEC_ZEALOT},
	{RACE_KUOTOA, CLASS_SHAMAN,  SPEC_ELEMENTALIST},
	{RACE_KUOTOA, CLASS_ROGUE,   SPEC_ASSASSIN},
	/* End Kuo Toa Options */

	/* Start Wood Elf Options */
	{RACE_WOODELF, CLASS_WARRIOR, SPEC_GUARDIAN},
	{RACE_WOODELF, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_WOODELF, CLASS_RANGER,  SPEC_ALL},
	{RACE_WOODELF, CLASS_DRUID,   SPEC_WOODLAND},
	{RACE_WOODELF, CLASS_ROGUE,   SPEC_SHARPSHOOTER},
	/* End Wood Elf Options */
	
	/* Start Firbolg Options */
	{RACE_FIRBOLG, CLASS_WARRIOR,   SPEC_SWORDSMAN},
	{RACE_FIRBOLG, CLASS_DRUID,     SPEC_WOODLAND},
	{RACE_FIRBOLG, CLASS_MERCENARY, SPEC_BOUNTY},
	{0}
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

void do_specialize(P_char ch, char *argument, int cmd)
{
  P_char teacher;
  int      i;
  char     buf[MAX_STRING_LENGTH] = "";
  bool found_one;
  
  if (IS_MULTICLASS_PC(ch)) {
    send_to_char("You have already chosen another path!\n", ch);
    return;
  }
  
  if (!*argument)
  {
    strcpy(buf, "You can choose from the following specializations:\n\r");
    
    found_one = append_valid_specs(buf, ch);;
    
    if( !found_one )
    {
      send_to_char("There are no specializations available to you.\r\n", ch);
      return;      
    }

    send_to_char(buf, ch);
    return;
  }
  
  for (teacher = world[ch->in_room].people; teacher; teacher = teacher->next_in_room)
    if (IS_NPC(teacher) && GET_CLASS(teacher, ch->player.m_class) &&
        IS_SET(teacher->specials.act, ACT_SPEC_TEACHER))
      break;
  
  if (!teacher) {
    send_to_char("You need to find a teacher first.\n", ch);
    return;
  }
  
  if (IS_NPC(ch))
  {
    mobsay(teacher, "Please stop trying to crash the game by ordering pets to specialize.");
    return;
  }
  
  if (teacher->player.m_class != ch->player.m_class)
  {
    mobsay(teacher, "I know nothing of your kind. Be gone.");
    return;
  }
  
  if (IS_SPECIALIZED(ch))
  {
    mobsay(teacher, "You are already specialized.");
    return;
  }
  
  char     tmp[512];
  
  while (*argument == ' ')
    argument++;
  
  if (time(NULL) < ch->only.pc->time_unspecced)
  {
    sprintf(buf, "You cannot specialize until %s.\r\n",
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
    if( !*GET_SPEC_NAME(ch->player.m_class, i))
      continue;
    
    if( !is_allowed_race_spec(GET_RACE(ch), ch->player.m_class, i+1) )
      continue;
    
    if( is_abbrev( argument, strip_ansi(GET_SPEC_NAME(ch->player.m_class, i)).c_str()) )
    {
      sprintf(buf, "From this day onwards you will follow the path of the %s&n!", GET_SPEC_NAME(ch->player.m_class, i));
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
  } if (epic_skillpoints(ch) < 1) {
    send_to_char("You need one practice point to pay for this.\n", ch);
  } else {
    act("You kneel in front of $p and pray to the \n"
        "&+bWater Goddess&n. As you continue your meditation, you begin\n"
        "to feel your mind is breaking free from the old habits and you\n"
        "feel ready to learn new ways.\n", FALSE, ch, obj, 0, TO_CHAR);
    act("$n kneels before $p and sinks in prayers.\n"
        "After few moments of silence $e smiles and stands up looking reborn.\n", FALSE, ch, obj, 0, TO_ROOM);
    ch->player.spec = 0;
    update_skills(ch);
    epic_gain_skillpoints(ch, -1);
    forget_spells(ch, -1);
  }
}
