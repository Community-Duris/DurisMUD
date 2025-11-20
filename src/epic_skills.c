#include <string.h>

#include "prototypes.h"
#include "skills.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "structs.h"
#include "damage.h"
#include "spells.h"
#include "epic.h"
#include "epic_skills.h"

extern P_index mob_index;
extern P_index obj_index;
extern Skill skills[];
extern P_room world;
extern struct race_names race_names_table[];

epic_reward epic_rewards[] = {
  {EPIC_REWARD_SKILL, SKILL_ANATOMY, 25, 25, 250000,
    CLASS_WARRIOR | CLASS_MERCENARY | CLASS_RANGER |
    CLASS_REAVER | CLASS_BERSERKER | CLASS_MONK |
    CLASS_DREADLORD | CLASS_CLERIC | CLASS_ROGUE |
    CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_AVENGER},
  {EPIC_REWARD_SKILL, SKILL_CHANT_MASTERY, 100, 75, 750000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_ILLUSIONIST |
    CLASS_NECROMANCER | CLASS_THEURGIST | CLASS_BARD |
    CLASS_SUMMONER | CLASS_ETHERMANCER },
  {EPIC_REWARD_SKILL, SKILL_SUMMON_BLIZZARD, 500, 50, 500000,
    CLASS_SHAMAN | CLASS_SORCERER | CLASS_ETHERMANCER |
    CLASS_DRUID | CLASS_BLIGHTER},
  {EPIC_REWARD_SKILL, SKILL_SUMMON_FAMILIAR, 100, 50, 500000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_SHAMAN |
    CLASS_DRUID | CLASS_NECROMANCER | CLASS_THEURGIST |
    CLASS_ALCHEMIST | CLASS_ILLUSIONIST | CLASS_SUMMONER |
    CLASS_BLIGHTER},
  {EPIC_REWARD_SKILL, SKILL_ADVANCED_MEDITATION, 25, 25, 250000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_ILLUSIONIST |
    CLASS_NECROMANCER | CLASS_THEURGIST | CLASS_PSIONICIST |
    CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_BARD |
    CLASS_REAVER | CLASS_SUMMONER },
  {EPIC_REWARD_SKILL, SKILL_DEVOTION, 500, 100, 1000000,
    CLASS_CLERIC | CLASS_PALADIN | CLASS_ANTIPALADIN |
    CLASS_ETHERMANCER },
  {EPIC_REWARD_SKILL, SKILL_SCRIBE_MASTERY, 500, 15, 150000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_NECROMANCER |
    CLASS_THEURGIST | CLASS_ILLUSIONIST | CLASS_SUMMONER},
  {EPIC_REWARD_SKILL, SKILL_SNEAKY_STRIKE, 100, 100, 1000000,
    CLASS_ROGUE | CLASS_BARD | CLASS_MERCENARY },
  {EPIC_REWARD_SKILL, SKILL_SILENT_SPELL, 500, 50, 500000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_ILLUSIONIST |
    CLASS_NECROMANCER | CLASS_THEURGIST | CLASS_SUMMONER},
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_LISTEN, 100, 50, 500000,
    CLASS_ROGUE | CLASS_BARD | CLASS_MERCENARY},
  {EPIC_REWARD_SKILL, SKILL_SHIELD_COMBAT, 100, 10, 100000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_SHIELD_COMBAT, 1000, 50, 500000,
    CLASS_WARRIOR | CLASS_CLERIC },
  {EPIC_REWARD_SKILL, SKILL_TWOWEAPON, 100, 25, 250000,
    CLASS_WARRIOR | CLASS_MERCENARY | CLASS_ROGUE | CLASS_RANGER |
	CLASS_REAVER | CLASS_BERSERKER},
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_TWOWEAPON, 1000, 100, 1000000,
    CLASS_WARRIOR | CLASS_MERCENARY | CLASS_ROGUE | CLASS_RANGER |
    CLASS_REAVER | CLASS_BERSERKER},
  {EPIC_REWARD_SKILL, SKILL_JIN_TOUCH, 100, 75, 750000,
    CLASS_MONK},
  {EPIC_REWARD_SKILL, SKILL_KI_STRIKE, 100, 100, 1000000,
    CLASS_MONK},
  {EPIC_REWARD_SKILL, SKILL_INFUSE_LIFE, 100, 50, 500000,
    CLASS_CONJURER | CLASS_NECROMANCER | CLASS_THEURGIST |
    CLASS_SHAMAN | CLASS_SUMMONER},
  {EPIC_REWARD_SKILL, SKILL_SPELL_PENETRATION, 500, 200, 2000000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_SUMMONER | CLASS_ILLUSIONIST },
  {EPIC_REWARD_SKILL, SKILL_DEVASTATING_CRITICAL, 200, 75, 750000,
    CLASS_WARRIOR | CLASS_BERSERKER | CLASS_AVENGER | CLASS_DREADLORD },
  {EPIC_REWARD_SKILL, SKILL_TOUGHNESS, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_SPATIAL_FOCUS, 100, 100, 1000000,
    CLASS_PSIONICIST},
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_ENDURANCE, 100, 100, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_TRACK, 100, 50, 500000,
    CLASS_ROGUE | CLASS_MERCENARY | CLASS_RANGER},
  {EPIC_REWARD_SKILL, SKILL_EMPOWER_SONG, 100, 50, 500000,
    CLASS_BARD},
  {EPIC_REWARD_SKILL, SKILL_FIX, 100, 50, 500000,
    0 },
//  {EPIC_REWARD_SKILL, SKILL_CRAFT, 100, 10, 10000,
//    0 },
  {EPIC_REWARD_SKILL, SKILL_ENCRUST, 100, 100, 1000000,
    0 },
//  {EPIC_REWARD_SKILL, SKILL_ENCHANT, 500, 100, 100000,
//    0 },
//  {EPIC_REWARD_SKILL, SKILL_SPELLBIND, 250, 100, 100000,
//    0 },
  {EPIC_REWARD_SKILL, SKILL_SMELT, 100, 50, 50000,
    0 },
//  {EPIC_REWARD_SKILL, SKILL_FORGE, 100, 1, 100000,
//    0 },
  {EPIC_REWARD_SKILL, SKILL_TOTEMIC_MASTERY, 250, 75, 750000,
    CLASS_SHAMAN },
  {EPIC_REWARD_SKILL, SKILL_INFUSE_MAGICAL_DEVICE, 100, 100, 1000000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_NECROMANCER |
    CLASS_THEURGIST | CLASS_ETHERMANCER |CLASS_BARD |
    CLASS_DRUID | CLASS_CLERIC | CLASS_PSIONICIST |
    CLASS_ILLUSIONIST | CLASS_ALCHEMIST | CLASS_SHAMAN |
    CLASS_SUMMONER | CLASS_BLIGHTER},
  {EPIC_REWARD_SKILL, SKILL_INDOMITABLE_RAGE, 100, 10, 100000,
    CLASS_WARRIOR | CLASS_BERSERKER },
  {EPIC_REWARD_SKILL, SKILL_NATURES_SANCTITY, 100, 75, 750000,
    CLASS_DRUID },
  {EPIC_REWARD_SKILL, SKILL_EXPERT_PARRY, 100, 200,  2000000,
    CLASS_WARRIOR | CLASS_PALADIN | CLASS_RANGER},
  {EPIC_REWARD_SKILL, SKILL_EXPERT_RIPOSTE, 100, 200, 2000000,
    CLASS_WARRIOR | CLASS_ANTIPALADIN | CLASS_DREADLORD | CLASS_AVENGER},
  {EPIC_REWARD_SKILL, SKILL_EPIC_STRENGTH, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_POWER, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_AGILITY, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_INTELLIGENCE, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_DEXTERITY, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_WISDOM, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_CONSTITUTION, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_CHARISMA, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_LUCK, 100, 50, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_SHIP_DAMAGE_CONTROL, 1000, 80, 8000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_NATURES_RUIN, 100, 75, 750000,
    CLASS_BLIGHTER },
  {0}
};

epic_teacher_skill epic_teachers[] = {
  {67103, SKILL_ADVANCED_MEDITATION, 0, 100, 0, 0, 0},
  {96013, SKILL_SUMMON_BLIZZARD, 0, 100, 0, 0, 0},
  {19141, SKILL_ANATOMY, 0, 100, 0, 0, 0},
  {75500, SKILL_CHANT_MASTERY, 0, 100, 0, 0, 0},
  {34446, SKILL_SUMMON_FAMILIAR, 0, 100, 0, 0, 0},
  {41327, SKILL_DEVOTION, 0, 100, 0, 0, 0},
  {36720, SKILL_SCRIBE_MASTERY, 0, 100, 0, 0, 0},
  {75523, SKILL_SNEAKY_STRIKE, 0, 100, 0, 0, 0},
  {18001, SKILL_SILENT_SPELL, 0, 100, 0, 0, 0},
  {27035, SKILL_IMPROVED_LISTEN, 0, 100, SKILL_LISTEN, 0, 80},
  {85724, SKILL_SHIELD_COMBAT, 0, 100, 0, 0, 0},
  {11306, SKILL_IMPROVED_SHIELD_COMBAT, 0, 100, SKILL_SHIELD_COMBAT, SKILL_IMPROVED_TWOWEAPON, 100},
  {36849, SKILL_TWOWEAPON, 0, 100, 0, 0, 0},
  {82500, SKILL_IMPROVED_TWOWEAPON, 0, 100, SKILL_TWOWEAPON, SKILL_IMPROVED_SHIELD_COMBAT, 100},
  {13219, SKILL_JIN_TOUCH, 0, 100, 0, 0, 0},
  {7357,  SKILL_KI_STRIKE, 0, 100, 0, 0, 0},
  {2428,  SKILL_INFUSE_LIFE, 0, 100, 0, 0, 0},
  {44824, SKILL_SPELL_PENETRATION, 0, 100, 0, 0, 0},
  {94364, SKILL_DEVASTATING_CRITICAL, 0, 100, 0, 0, 0},
  {80862, SKILL_TOUGHNESS, 0, 100, 0, 0, 0},
  {4208,  SKILL_SPATIAL_FOCUS, 0, 100, 0, 0, 0},
  {49161, SKILL_IMPROVED_ENDURANCE, 0, 100, 0, 0, 0},
  {20242, SKILL_IMPROVED_TRACK, 0, 100, SKILL_TRACK, 0, 95},
  {99548, SKILL_EMPOWER_SONG, 0, 100, 0, 0},
  {22436, SKILL_FIX, 0, 100, 0, 0, 0}, //smith in stormport
//  {9454,  SKILL_CRAFT, 0, 100, 0, 0, 0}, //Rjinal in Samirz
  {40760, SKILL_ENCRUST, 0, 100, SKILL_CRAFT, 0, 100}, //Snent in Divine Home
  {78006, SKILL_ENCHANT, 0, 100, 0, SKILL_SPELLBIND, 0}, //Bargor in Oasis
  {94017, SKILL_SPELLBIND, 0, 100, 0, SKILL_ENCHANT, 0}, //Kalroh in Maze of Undead Army
  {37145, SKILL_SMELT, 0, 100, 0, 0, 0}, //Carmotee in Dumaathe
//  {21618, SKILL_FORGE, 0, 100, 0, 0, 0}, //Tenkuss in Aravne
  {49162, SKILL_TOTEMIC_MASTERY, 0, 100, 0, 0, 0},
  {76008, SKILL_INFUSE_MAGICAL_DEVICE, 0, 100, 0, 0, 0},  //Deathium in Ultarium
  {28975, SKILL_INDOMITABLE_RAGE, 0, 100, 0, 0, 0},
  {70806, SKILL_NATURES_SANCTITY, 0, 100, 0, 0, 0},
  {6013,  SKILL_EXPERT_PARRY, 0, 100, 0, 0, 0}, //Bemon in Clavikord Swamp
  {75615, SKILL_EXPERT_RIPOSTE, 0, 100, 0, 0, 0}, //Rolart in Obsidian Citadel
  {98534, SKILL_EPIC_STRENGTH, 0, 100, 0, 0, 0}, //Olat in Bandit Canyon
  {4203,  SKILL_EPIC_POWER, 0, 100, 0, 0, 0}, //Ezallixxel
  {95304, SKILL_EPIC_AGILITY, 0, 100, 0, 0, 0}, //Grellinar in Darkfall
  {15120, SKILL_EPIC_INTELLIGENCE, 0, 100, 0, 0, 0}, //Undead Wizard in Cave City
  {80907, SKILL_EPIC_DEXTERITY, 0, 100, 0, 0, 0}, //Captain in Ceothia
  {53658, SKILL_EPIC_WISDOM, 0, 100, 0, 0, 0}, //Chauseis in Connector Zones (Sunwell area)
  {66671, SKILL_EPIC_CONSTITUTION, 0, 100, 0, 0, 0}, //Thurdorf in Torrhan
  {82408, SKILL_EPIC_CHARISMA, 0, 100, 0, 0, 0}, //Frolikk in Temple of Sun
  {21535, SKILL_EPIC_LUCK, 0, 100, 0, 0, 0}, //Babedo in Aravne
  {2733, SKILL_SHIP_DAMAGE_CONTROL, 0, 100, 0, 0, 0}, // Commodore in Headless
  {402029, SKILL_NATURES_RUIN, 0, 100, 0, 0, 0}, // Shezeera in Library zone
  {0}
};

void create_epic_skills()
{
  SKILL_CREATE("mine", SKILL_MINE, TAR_EPIC);
  SKILL_CREATE("craft", SKILL_CRAFT, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("forge", SKILL_FORGE, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("spell mastery", SKILL_SPELL_MASTERY, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("chant mastery", SKILL_CHANT_MASTERY, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("anatomy", SKILL_ANATOMY, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("summon blizzard", SKILL_SUMMON_BLIZZARD, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("summon familiar", SKILL_SUMMON_FAMILIAR, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("advanced meditation", SKILL_ADVANCED_MEDITATION, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("devotion", SKILL_DEVOTION, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("scribe mastery", SKILL_SCRIBE_MASTERY, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("sneaky strike", SKILL_SNEAKY_STRIKE, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("silent spell", SKILL_SILENT_SPELL, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("shield combat", SKILL_SHIELD_COMBAT, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("improved shield combat", SKILL_IMPROVED_SHIELD_COMBAT, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("improved listen", SKILL_IMPROVED_LISTEN, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("two weapon fighting", SKILL_TWOWEAPON, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("improved two weapon fighting", SKILL_IMPROVED_TWOWEAPON, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("jin touch", SKILL_JIN_TOUCH, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("ki strike", SKILL_KI_STRIKE, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("infuse life", SKILL_INFUSE_LIFE, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("spell penetration", SKILL_SPELL_PENETRATION, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("devastating critical", SKILL_DEVASTATING_CRITICAL, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("shieldless bash", SKILL_SHIELDLESS_BASH, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("toughness", SKILL_TOUGHNESS, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("spatial focus", SKILL_SPATIAL_FOCUS, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("improved endurance", SKILL_IMPROVED_ENDURANCE, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("improved track", SKILL_IMPROVED_TRACK, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("empower song", SKILL_EMPOWER_SONG, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("infuse magical device", SKILL_INFUSE_MAGICAL_DEVICE, TAR_PHYS |TAR_EPIC);
  SKILL_CREATE("indomitable rage", SKILL_INDOMITABLE_RAGE, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("fix", SKILL_FIX, TAR_EPIC);
  SKILL_CREATE("encrust", SKILL_ENCRUST, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("enchant", SKILL_ENCHANT, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("spellbind", SKILL_SPELLBIND, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("smelt", SKILL_SMELT, TAR_PHYS | TAR_EPIC);
  SKILL_CREATE("totemic mastery", SKILL_TOTEMIC_MASTERY, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("natures sanctity", SKILL_NATURES_SANCTITY, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("expert parry", SKILL_EXPERT_PARRY, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("expert riposte", SKILL_EXPERT_RIPOSTE, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("natures ruin", SKILL_NATURES_RUIN, TAR_MENTAL | TAR_EPIC);

  /* Stat adjustment epic skills */
  SKILL_CREATE("epic strength", SKILL_EPIC_STRENGTH, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("epic power", SKILL_EPIC_POWER, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("epic agility", SKILL_EPIC_AGILITY, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("epic intelligence", SKILL_EPIC_INTELLIGENCE, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("epic dexterity", SKILL_EPIC_DEXTERITY, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("epic wisdom", SKILL_EPIC_WISDOM, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("epic constitution", SKILL_EPIC_CONSTITUTION, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("epic charisma", SKILL_EPIC_CHARISMA, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("epic luck", SKILL_EPIC_LUCK, TAR_MENTAL | TAR_EPIC);
  SKILL_CREATE("ship damage control", SKILL_SHIP_DAMAGE_CONTROL, TAR_PHYS | TAR_EPIC);
}

void do_epic_skills(P_char ch, char *arg, int cmd)
{
  char buff[MAX_STRING_LENGTH];
  P_char teacher;

  if( IS_TRUSTED(ch) )
  {
    send_to_char("&+GSkills                    (Vnum) Teacher Name\n" \
                 "-------------------------------------------------\n", ch);
  } else {
    send_to_char("&+GThe following epic skills are available to you:\n" \
                 "-----------------------------------------------\n", ch);
  }

  int s, t;
  for(s = 0; epic_rewards[s].type; s++)
  {
    int skill = epic_rewards[s].value;

    if(skill <= 0 || skill >= (LAST_SKILL + 1))
      continue;

    // Handle anti-race - Thris dev crit.
    if( IS_THRIKREEN(ch) && skill == SKILL_DEVASTATING_CRITICAL )
    {
      continue;
    }

    for(t = 0; epic_teachers[t].vnum; t++)
    {
      if(epic_teachers[t].skill == skill)
        break;
    }

    if(!epic_teachers[t].vnum)
      continue;

    if(epic_rewards[s].classes &&
        !IS_SET(epic_rewards[s].classes, ch->player.m_class) &&
        !IS_SET(epic_rewards[s].classes, ch->player.secondary_class))
      continue;

    if(epic_teachers[t].deny_skill && GET_CHAR_SKILL(ch, epic_teachers[t].deny_skill))
      continue;

    if( epic_teachers[t].pre_requisite
      && (GET_CHAR_SKILL(ch, epic_teachers[t].pre_requisite) < epic_teachers[t].pre_req_lvl) )
    {
      continue;
    }

    if(IS_TRUSTED(ch))
    {
      if ((teacher = read_mobile(epic_teachers[t].vnum, VIRTUAL)))
      {
        snprintf(buff, MAX_STRING_LENGTH, "&+W%-25s &n(&+W%-5d&n) %s\n", skills[skill].name, epic_teachers[t].vnum, teacher->player.short_descr);
        extract_char(teacher);
      } else 
	{
        logit(LOG_DEBUG, "do_epic_skills(): epic_teachers[%d].vnum does not exist for epic skill %s", t, skills[skill].name);
        snprintf(buff, MAX_STRING_LENGTH, "&+W%-25s &n(&+W%-5d&n) Teacher does not exist.\n", skills[skill].name, epic_teachers[t].vnum);
      }
    } else 
	if ((teacher = read_mobile(epic_teachers[t].vnum, VIRTUAL)))
	{
      snprintf(buff, MAX_STRING_LENGTH, "&+W%-25s &n&+yTeacher&+Y: &n %s\n", skills[skill].name, teacher->player.short_descr);
	extract_char(teacher);
    }
    send_to_char(buff, ch);
  }

  send_to_char("\n", ch);
}


int epic_teacher(P_char ch, P_char pl, int cmd, char *arg)
{
  int skl, epics_cost, coins_cost;
  char buffer[256];
  float cost_mod;
  epic_teacher_skill *pTeacher;
  epic_reward *pReward;

  if( cmd != CMD_PRACTICE )
  {
    return FALSE;
  }

  pTeacher = NULL;
  for( int iTeacher = 0; epic_teachers[iTeacher].vnum; iTeacher++ )
  {
    if( GET_VNUM(ch) == epic_teachers[iTeacher].vnum )
    {
        pTeacher = &(epic_teachers[iTeacher]);
      break;
    }
  }
  if( pTeacher == NULL )
    return FALSE;

  // Find the skill
  pReward = NULL;
  for( skl = 0; epic_rewards[skl].type; skl++ )
  {
    if( epic_rewards[skl].type == EPIC_REWARD_SKILL
      && epic_rewards[skl].value == pTeacher->skill )
    {
      pReward = &(epic_rewards[skl]);
      break;
    }
  }
  if( pReward == NULL )
    return FALSE;

  skl = pReward->value;

  cost_mod = 1 + GET_CHAR_SKILL(pl, skl) / get_property("epic.progressFactor", 30);
  // For the 2015-6 wipe, doubling cash cost and tripling the epic point cost.
  epics_cost = 3 * (int) (cost_mod * pReward->points_cost);
  coins_cost = 2 * (int) (cost_mod * pReward->coins);

  if( IS_MULTICLASS_PC(pl)
    && !IS_SET(pReward->classes, pl->player.m_class)
    && IS_SET(pReward->classes, pl->player.secondary_class) )
  {
    epics_cost *= (int) (get_property("epic.multiclass.EpicSkillCost", 2));
    coins_cost *= (int) (get_property("epic.multiclass.EpicPlatCost", 3));
  }

  if( !arg || !*arg )
  {
    // Practice called with no arguments
    snprintf(buffer, 256, "Welcome, traveller!\n"
      "I am pleased that you have wandered so far in order to seek my assistance.\n"
      "There are few adventurers willing to seek out the knowledge of &+W%s&n.\n\n", skills[skl].name);
    send_to_char(buffer, pl);
    if( GET_CHAR_SKILL(pl, skl) < 100 )
    {
      // If they can learn the skill: Class gets it, not thri+dev crit, don't have mutually exclusive skill,
      //   missing pre-req skill, or skill maxxed.
      if(  !( pReward->classes && !IS_SET(pReward->classes, pl->player.m_class)
        && !IS_SET(pReward->classes, pl->player.secondary_class) )
        && !( IS_THRIKREEN(pl) && skl == SKILL_DEVASTATING_CRITICAL )
        && !( pTeacher->deny_skill && GET_CHAR_SKILL(pl, pTeacher->deny_skill) )
        && !( pTeacher->pre_requisite && GET_CHAR_SKILL(pl, pTeacher->pre_requisite) < pTeacher->pre_req_lvl )
        && !( GET_CHAR_SKILL(pl, skl) >= 100 || GET_CHAR_SKILL(pl, skl) >= pTeacher->max ) )
      {
        snprintf(buffer, 256, "It would cost you &+W%d&n epic points and &+W%s&n to learn &+W%s&n.\n",
          epics_cost, coin_stringv(coins_cost), skills[skl].name );
        send_to_char(buffer, pl);
      }
      else
      {
        snprintf(buffer, 256, "&+W%s&n is not currently available to you.\n", skills[skl].name );
        CAP(buffer);
        send_to_char(buffer, pl);
      }
    }
    else
    {
      snprintf(buffer, 256, "You have already maxxed &+W%s&n.\n", skills[skl].name );
      send_to_char(buffer, pl);
    }
    return TRUE;
  }

  // Trying to practice a different skill.
  if( !strstr(arg, skills[skl].name) )
  {
    if( is_abbrev(arg, skills[skl].name) )
      send_to_char( "To practice an epic skill, you must type the full epic skill name out.\n", pl );
    return FALSE;
  }

  // Handle anti-classes
  if( pReward->classes
    && !IS_SET(pReward->classes, pl->player.m_class)
    && !IS_SET(pReward->classes, pl->player.secondary_class) )
  {
    send_to_char("Unfortunately, I am not able to teach people of your class.\n", pl);
    return TRUE;
  }

  // Handle anti-race - Thris dev crit.
  if( IS_THRIKREEN(pl) && skl == SKILL_DEVASTATING_CRITICAL )
  {
    snprintf(buffer, MAX_STRING_LENGTH, "I cannot with good conscience teach this skill to a %s!\n", race_names_table[RACE_THRIKREEN].ansi );
    send_to_char(buffer, pl);
    return TRUE;
  }

  if( pTeacher->deny_skill && GET_CHAR_SKILL(pl, pTeacher->deny_skill) )
  {
    snprintf(buffer, MAX_STRING_LENGTH, "I cannot with good conscience teach this skill to someone who has already studied &+W%s&n!\n", skills[pTeacher->deny_skill].name);
    send_to_char(buffer, pl);
    return TRUE;
  }

  if( pTeacher->pre_requisite && GET_CHAR_SKILL(pl, pTeacher->pre_requisite) < pTeacher->pre_req_lvl )
  {
    snprintf(buffer, MAX_STRING_LENGTH, "You have not yet mastered the art of &+W%s&n!\r\n", skills[pTeacher->pre_requisite].name);
    send_to_char(buffer, pl);
    return TRUE;
  }

  if( GET_CHAR_SKILL(pl, skl) >= 100 || GET_CHAR_SKILL(pl, skl) >= pTeacher->max )
  {
    send_to_char("Unfortunately, I cannot teach you anything more, you have already mastered this skill!\n", pl);
    return TRUE;
  }

  // If the prereq skill is less than the epic skill (after teaching).
  if( pTeacher->pre_requisite
    && GET_CHAR_SKILL(pl, pTeacher->pre_requisite) < GET_CHAR_SKILL(pl, skl) + get_property("epic.skillGain", 10) )
  {
    snprintf(buffer, MAX_STRING_LENGTH, "You must study &+W%s&n more before you can progress in &+W%s&n.\n",
      skills[pTeacher->pre_requisite].name, skills[skl].name );
    send_to_char( buffer, pl );
    return TRUE;
  }

  if( pReward->classes
    && !IS_SET(pReward->classes, pl->player.m_class)
    && !IS_SET(pReward->classes, pl->player.secondary_class) )
  {
    send_to_char("Unfortunately, I am not able to teach people of your class.\n", pl);
    return TRUE;
  }

  // Handle anti-race - Thris dev crit.
  if( IS_THRIKREEN(pl) && skl == SKILL_DEVASTATING_CRITICAL )
  {
    snprintf(buffer, MAX_STRING_LENGTH, "I cannot with good conscience teach this skill to a %s!\n", race_names_table[RACE_THRIKREEN].ansi );
    send_to_char(buffer, pl);
    return TRUE;
  }

  if( pTeacher->deny_skill && GET_CHAR_SKILL(pl, pTeacher->deny_skill))
  {
    snprintf(buffer, MAX_STRING_LENGTH, "I cannot with good conscience teach that skill to someone who has already studied &+W%s&n!\n", skills[pTeacher->deny_skill].name);
    send_to_char(buffer, pl);
    return TRUE;
  }

  if( pTeacher->pre_requisite && GET_CHAR_SKILL(pl, pTeacher->pre_requisite) < pTeacher->pre_req_lvl )
  {
    send_to_char("You haven't progressed far enough to be able to master such skills!\n", pl);
    return TRUE;
  }

  if( GET_EPIC_POINTS(pl) < epics_cost )
  {
    send_to_char("You don't have enough epic points!\n", pl);
    return TRUE;
  }

  if( GET_MONEY(pl) < coins_cost )
  {
    send_to_char("You can't afford my teaching!", pl);
    return TRUE;
  }

  if( (GET_CHAR_SKILL(pl, skl) >= 100) || (GET_CHAR_SKILL(pl, skl) >= pTeacher->max) )
  {
    send_to_char("Unfortunately, I cannot teach you anything more, you have already mastered this skill!\n", pl);
    return TRUE;
  }

  snprintf(buffer, MAX_STRING_LENGTH, "$n takes you aside and teaches you the finer points of &+W%s&n.\n"
    "&+cYou feel your skill in %s improving.&n\n", skills[skl].name, skills[skl].name );
  act( buffer, FALSE, ch, 0, pl, TO_VICT );

  SUB_MONEY(pl, coins_cost, 0);

  // Ditching this, since we'll just use straight epic points now. Zion 4/8/2014
  // epic_gain_skillpoints(pl, -1 * points_cost);
	pl->only.pc->epics -= epics_cost;

  pl->only.pc->skills[skl].taught = pl->only.pc->skills[skl].learned =
    pl->only.pc->skills[skl].learned + get_property("epic.skillGain", 10);
  if( pl->only.pc->skills[skl].taught > 100 )
    pl->only.pc->skills[skl].taught = pl->only.pc->skills[skl].learned = 100;
  if( pl->only.pc->skills[skl].taught == 100 )
  {
    snprintf(buffer, MAX_STRING_LENGTH, "You have mastered &+W%s&N.\n", skills[skl].name );
    send_to_char( buffer, pl );
  }
  do_save_silent(pl, 1); // Epic stats require a save.
  CharWait(pl, PULSE_VIOLENCE);
  return TRUE;
}

void event_blizzard(P_char ch, P_char victim, P_obj obj, void *data)
{
  int count;
  P_room room = &world[ch->in_room];
  P_char next_ch;
  struct room_affect *raf = get_spell_from_room(room, SKILL_SUMMON_BLIZZARD);
  int step = *((int*)data);
  char buffer[256];
  struct damage_messages messages1 = {
    "&+CTiny shards of ice from the ravaging blizzard hurt $N badly.",
    "&+CYou are hurt badly by the tiny shards of ice from the ravaging blizzard.",
    "&+CTiny shards of ice from the ravaging blizzard hurt $N badly.",
    "&+CTiny shards of ice from the ravaging blizzard turned $N into a spiked statue!",
    "&+CTiny shards of ice from the ravaging blizzard slowly but surely bash out the last drops of heat from your freezing body..",
    "&+CTiny shards of ice from the ravaging blizzard turned $N into a spiked statue!", 0
  };
  struct damage_messages messages2 = {
    "&+CA cloud of &+Wsnow&+C surrounds $N &+Ccompletely, sucking away all heat.",
    "&+CYou are hurt badly by the tiny shards of ice from the ravaging blizzard.",
    "&+CA cloud of &+Wsnow&+C surrounds $N &+Ccompletely, sucking away all heat.",
    messages1.death_attacker, messages1.death_victim, messages1.death_room,
  };

  if(!raf) {
    send_to_room("&+CThe &+Ldark clouds&+C disperse and the blizzard comes to its end.&n\n", ch->in_room);
    return;
  }

  if(step == 1) {
    send_to_room(
        "&+CSuddenly &+Lheavy clouds&+C accumulate above your head, covering the entire sky!\n", ch->in_room);
    step++;
    add_event(event_blizzard, 3, ch, 0, 0, 0, &step, sizeof(step));
  } else if(step == 2) {
    send_to_room("&+CIt starts to &+Wsnow&+C!\n", ch->in_room);
    send_to_room(
        "Strong &+Wwinds &+Cbegin tossing the &+Wsnow &+Cand ice around with incredible force.&n\n", ch->in_room);
    step++;
    add_event(event_blizzard, 3, ch, 0, 0, 0, &step, sizeof(step));
  } else {
    struct room_affect *faf;
    struct affected_type af;

    count = 1;
    for (victim = room->people; victim; victim = next_ch) {
      next_ch = victim->next_in_room;
      if(victim != ch && !grouped(victim, ch))
        count++;
    }

    if((faf = get_spell_from_room(room, SPELL_FIRESTORM)) ||
        (faf = get_spell_from_room(room, SPELL_SCATHING_WIND)) ||
        (faf = get_spell_from_room(room, SPELL_INCENDIARY_CLOUD))) {
      if ((victim = get_random_char_in_room(ch->in_room, ch, 0)))
      {
        snprintf(buffer, 256, "&+CThe snow melts from the heat of &+R%s &+Cand you are only splashed by &+bwater&n.",
            skills[faf->type].name);
        send_to_char(buffer, victim);
        snprintf(buffer, 256, "&+CThe snow melts from the heat of &+R%s &+Cand $n is only splashed by &+bwater&n.",
            skills[faf->type].name);
        act(buffer, FALSE, victim, 0, 0, TO_ROOM);
        make_wet(victim, 2 * WAIT_MIN);
      }
    }
    else if ((victim = get_random_char_in_room(ch->in_room, ch, DISALLOW_SELF | DISALLOW_GROUPED)))
    {
      spell_damage(ch, victim, 70 + dice(4,6), SPLDAM_COLD,
          SPLDAM_NOSHRUG | SPLDAM_NODEFLECT,
          (number(0, 2) && GET_CHAR_SKILL(ch, SKILL_SUMMON_BLIZZARD) > 30) ? &messages1 : &messages2);
    }

    add_event(event_blizzard, number(20,30)/count, ch, 0, 0, 0, &step, sizeof(step));
  }
}

void do_summon_blizzard(P_char ch, char *argument, int cmd)
{
  int room = ch->in_room;
  struct room_affect raf;
  int step = 1;

  if(get_spell_from_room(&world[room], SKILL_SUMMON_BLIZZARD)) {
    send_to_char("There is already a blizzard raging here!", ch);
    return;
  }

  if(!affect_timer(ch, get_property("timer.mins.summonBlizzard", 3) * WAIT_MIN, SKILL_SUMMON_BLIZZARD)) {
    send_to_char("You are too tired to summon another blizzard.\n", ch);
    return;
  }

  send_to_char("You call upon forces of nature to bring a massive blizzard to this area.\n", ch);
  memset(&raf, 0, sizeof(raf));
  raf.type = SKILL_SUMMON_BLIZZARD;
  raf.duration = 3 * PULSE_VIOLENCE + (GET_CHAR_SKILL(ch, SKILL_SUMMON_BLIZZARD) * PULSE_VIOLENCE) / 30;

  affect_to_room(room, &raf);

  add_event(event_blizzard, 2, ch, 0, 0, 0, &step, sizeof(step));
}

void do_summon_familiar(P_char ch, char *argument, int cmd)
{
  P_char mob;
  char buffer[256];
  struct char_link_data *cld;

  typedef struct {
    int vnum;
    int skill;
    char *name;
  } familiar_data;

  familiar_data familiars[] = {
    { EPIC_CAT_VNUM, 5, "cat" },
    { EPIC_BAT_VNUM, 30, "bat" },
    { EPIC_IGUANA_VNUM, 50, "iguana" },
    { EPIC_RAVEN_VNUM, 70, "raven" },
    { EPIC_OWL_VNUM, 90, "owl" },
    { EPIC_IMP_VNUM, 100, "imp" },
    { 0 }
  };

  int i;

  int ch_skill_level = GET_CHAR_SKILL(ch, SKILL_SUMMON_FAMILIAR);

  if(IS_TRUSTED(ch))
  {
    ch_skill_level = 100;
  }

  if(strlen(argument) < 1) {
    send_to_char("You can summon the following familiars:\n", ch);
    for (i = 0; familiars[i].vnum && familiars[i].skill <= ch_skill_level; i++) {
      snprintf(buffer, 256, "  %s\n", familiars[i].name);
      send_to_char(buffer, ch);
    }
    return;
  }

  for (cld = ch->linked; cld; cld = cld->next_linked)
  {
    if(cld->type == LNK_PET)
    {
      for(i = 0; familiars[i].vnum; i++)
      {
        if(mob_index[GET_RNUM(cld->linking)].virtual_number == familiars[i].vnum)
        {
          send_to_char("But you already have a familiar!\n", ch);
          return;

        }
      }
    }
  }

  if(!affect_timer(ch,
        (get_property("timer.mins.summonFamiliar", 10) + 10 -
         2 * (ch_skill_level/20)) * WAIT_MIN,
        SKILL_SUMMON_FAMILIAR)) {
    send_to_char("You call for a familiar but no creature responds.\n", ch);
    return;
  }

  for (i = 0; familiars[i].vnum && familiars[i].skill <= ch_skill_level; i++) {
    if(!str_cmp(argument, familiars[i].name)) {
      mob = read_mobile(familiars[i].vnum, VIRTUAL);

      if(!mob)
      {
        send_to_char("Nothing answers.\n", ch);
        return;
      }

      int hits = GET_HIT(mob) + (2 * ch_skill_level);
      GET_HIT(mob) = GET_MAX_HIT(mob) = mob->points.base_hit = hits;

      char_to_room(mob, ch->in_room, 0);
      act("$n announces $s arrival with a quiet squeak.", FALSE,
          mob, 0, 0, TO_ROOM);
      setup_pet(mob, ch, 1000, PET_NOCASH);
      add_follower(mob, ch);
      return;
    }
  }

  send_to_char("You cannot summon this kind of familiar.\n", ch);
}

bool epic_summon(P_char ch, char *arg)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];

  if(!ch || IS_NPC(ch))
    return false;

  argument_interpreter(arg, buff2, buff3);

  if(!str_cmp("blizzard", buff2) && GET_CHAR_SKILL(ch, SKILL_SUMMON_BLIZZARD))
  {
    do_summon_blizzard(ch, 0, CMD_SUMMON);

  }
  else if(!str_cmp("familiar", buff2) && (IS_TRUSTED(ch) || GET_CHAR_SKILL(ch, SKILL_SUMMON_FAMILIAR)))
  {
    do_summon_familiar(ch, buff3, CMD_SUMMON);
  }
  else
  {
    return false;
  }

  return true;
}

int epic_familiar(P_char ch, P_char pl, int cmd, char *arg)
{
  if( !IS_ALIVE(ch) )
    return FALSE;

  if(cmd == CMD_SET_PERIODIC)
    return TRUE;

  P_char master = get_linked_char(ch, LNK_PET);


  // bat proc, has a chance to prevent incoming takedown
  if(GET_VNUM(ch) == EPIC_BAT_VNUM &&
      (cmd == CMD_BASH || cmd == CMD_TRIP || cmd == CMD_SPRINGLEAP ||
      cmd == CMD_TACKLE || cmd == CMD_BODYSLAM || cmd == CMD_MAUL) &&
      get_char_vis(pl, arg) == master && !number(0,2))
  {
    act("$n notices $N's maneuver and dives towards $S head to protect the master!",
        FALSE, ch, 0, pl, TO_NOTVICT);
    act("$n notices your maneuver and dives towards your head to protect $s master!",
        FALSE, ch, 0, pl, TO_VICT);

    if(GET_C_AGI(pl) < number(0,150) && !number(0,2))
    {
      act("$n's unexpected attack caused you to get lost in your tracks..",
          FALSE, ch, 0, pl, TO_VICT);
      act("$n's vicious assault disturbed $N's move.", FALSE, ch, 0, pl, TO_NOTVICT);
      CharWait(pl, PULSE_VIOLENCE);
      return TRUE;
    }
    else
    {
      act("You easily evade $n's attack.", FALSE, ch, 0, pl, TO_VICT);
      return FALSE;
    }
  }

  if(mob_index[GET_RNUM(ch)].virtual_number == EPIC_IGUANA_VNUM &&
      cmd == CMD_PAT && get_char_vis(pl, arg) == ch && pl == master)
  {
    if(IS_RIDING(ch))
    {
      act("$N pats $n softly on $s back.\n"
          "$n wiggles reluctantly and slowly begins to climb down $N's back.",
          FALSE, ch, 0, master, TO_NOTVICT);
      act("You pat $n softly on $s back.\n"
          "$n wiggles reluctantly and slowly begins to climb down your back.",
          FALSE, ch, 0, master, TO_VICT);
      do_dismount(ch, 0, CMD_DISMOUNT);
    }
    else if(!IS_RIDING(master))
    {
      act("$N pats $n softly on $s back.\n"
          "$n slowly begins to climb up $N's back.",
          FALSE, ch, 0, master, TO_NOTVICT);
      act("You pat $n softly on $s back.\n"
          "$n slowly begins to climb up your back.",
          FALSE, ch, 0, master, TO_VICT);
      link_char(ch, master, LNK_RIDING);
    }
    else
    {
      return FALSE;
    }

    return TRUE;
  }

  if(cmd == CMD_PERIODIC)
  {
    if(!master)
    {
      act("$n turns around looking for $s master then disappears.", FALSE, ch, 0, 0, TO_ROOM);
      extract_char(ch);
      return TRUE;
    }

    switch(ch->player.m_class)
    {
      case CLASS_WARRIOR:
      case CLASS_MERCENARY:
        break;

      case CLASS_SORCERER:
      case CLASS_CONJURER:
        if(!number(0,2) && master->in_room == ch->in_room)
        {
          CastMageSpell(ch, master, 1);
          return TRUE;
        }
        break;

      case CLASS_CLERIC:
        if(!number(0,2) && master->in_room == ch->in_room)
        {
          CastClericSpell(ch, master, 1);
          return TRUE;
        }
        break;

      default:
        break;
    }

    return TRUE;
  }

  return FALSE;
}

int spell_penetration_check(P_char caster, P_char victim)
{
   int skill = GET_CHAR_SKILL(caster, SKILL_SPELL_PENETRATION);

   if(skill)
   {
      skill /= 2;

      if(number(0, 110) < BOUNDED(10, skill, (int)get_property("skill.spellPenetration.highEndPercent", 60.000)) &&
         caster->in_room == victim->in_room)
      {
        struct affected_type af;

        memset(&af, 0, sizeof(af));
        af.type = SKILL_SPELL_PENETRATION;
        af.flags = AFFTYPE_NOAPPLY | AFFTYPE_SHORT;
        af.duration = 1;
        affect_to_char(victim, &af);
        act("&+CYour pure arcane focus causes your spell to partially burst through&n $N&+C's magical resistance!&n",
          TRUE, caster, 0, victim, TO_CHAR);
        act("$n&+C seems to focus for a moment, and $s spell partially bursts through your magical barrier!&n",
          TRUE, caster, 0, victim, TO_VICT);
        act("$n&+C seems to focus for a moment, and $s spell partially bursts through&n $N&+C's magical barrier!&n",
          TRUE, caster, 0, victim, TO_NOTVICT);
        return TRUE;
      }
    }
  return FALSE;
}

// bonuses
//     hit  dam
// 1    1    0
// 2    2    0
// 3    0    1
// 4    1    1
// 5    2    1
// 6    0    2
// 7    1    2
// 8    2    2


int devotion_skill_check(P_char ch)
{
  int dev_power = (GET_CHAR_SKILL(ch, SKILL_DEVOTION) - 40)/10 - number(0,50);

  if(dev_power <= 0) return 0;

  char buf[128];
  buf[0] = '\0';

    if(dev_power > 4)
      snprintf(buf, MAX_STRING_LENGTH,
        "You feel as if %s took over your body bringing death to your foes!\n",
        get_god_name(ch));
    else if(dev_power > 2)
      snprintf(buf, MAX_STRING_LENGTH,
        "%s fills you with holy power bringing death to your foes!\n",
        get_god_name(ch));
    else if(dev_power > 0)
      snprintf(buf, MAX_STRING_LENGTH,
        "%s fills you with holy power to destroy your foes!\n",
        get_god_name(ch));

  send_to_char(buf, ch);

  return 10 * dev_power;
}

int devotion_spell_check(int spell)
{
  switch(spell)
  {
    case SPELL_FLAMESTRIKE:
    case SPELL_APOCALYPSE:
    case SPELL_JUDGEMENT:
    case SPELL_FULL_HARM:
    case SPELL_HARM:
    case SPELL_CAUSE_LIGHT:
    case SPELL_CAUSE_SERIOUS:
    case SPELL_CAUSE_CRITICAL:
    case SPELL_DESTROY_UNDEAD:
    case SPELL_HOLY_WORD:
    case SPELL_UNHOLY_WORD:
    case SPELL_EARTHQUAKE:
    case SPELL_TURN_UNDEAD:
    case SPELL_BANISH:
      return TRUE;
    default:
      return FALSE;
  }

  return FALSE;
}

int chant_mastery_bonus(P_char ch, int dura)
{
  int chant_bonus;
  char buffer[256];

  if(5 + GET_CHAR_SKILL(ch, SKILL_CHANT_MASTERY) / 10 > number(0,100))
  {
     chant_bonus = MAX(0, GET_CHAR_SKILL(ch, SKILL_CHANT_MASTERY)/40 + number(-1,1));
     snprintf(buffer, 256, "%s magic surrounds you as you begin your chant.&n",
                     chant_bonus == 0 ? "&+WSparkling&n" :
                     chant_bonus == 1 ? "&+WSparkling" : "&+WSp&+Cark&+Wli&+Cn&+Wg");
     act(buffer, FALSE, ch, 0, 0, TO_CHAR);
     snprintf(buffer, 256, "%s magic surrounds $n &+Was $e begins $s chant.&n",
                     chant_bonus == 0 ? "&+WSparkling&n" :
                     chant_bonus == 1 ? "&+WSparkling" : "&+WSp&+Cark&+Wli&+Cn&+Wg");
     act(buffer, FALSE, ch, 0, 0, TO_ROOM);
  } 
  else 
  {
     CharWait(ch, dura);
     return dura;
  }

  if(chant_bonus == 3) 
  {
     CharWait(ch, 1);
     return 1;
  } 
  else if(chant_bonus == 2) 
  {
     CharWait(ch, dura >> 1);
     return  1;
  }
  else if(chant_bonus == 1) 
  {
     CharWait(ch, dura * 0.8);
     return (int) (dura * 0.6);
  }
  else
  {
     CharWait(ch, dura);
     return (int) (dura * 0.8);
  }
}



bool silent_spell_check(P_char ch)
{
  int skill = GET_CHAR_SKILL(ch, SKILL_SILENT_SPELL);

  if(skill <= 0)
    return FALSE;

  skill = BOUNDED(25, skill, 100);

  if(number(0, 100) < skill)
  {
    return TRUE;
  }
  else
  {
    act("You try to use your hands to cast a spell, but you just end up looking goofy.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n tries to use $s hands to cast a spell, but just ends up looking goofy.", FALSE, ch, 0, 0, TO_ROOM);
    return FALSE;
  }
}

void say_silent_spell(P_char ch, int spell)
{
  act("Using your expanded knowledge, you cast the spell with nothing but a gesture of the hand.",
      FALSE, ch, 0, 0, TO_CHAR);
  act("Using $s expanded knowledge, $n casts $s spell with nothing but a gesture of the hand.",
      FALSE, ch, 0, 0, TO_ROOM);
}


int two_weapon_check(P_char ch)
{
  int twoskl;

  twoskl = GET_CHAR_SKILL(ch, SKILL_TWOWEAPON);

  if(ch->equipment[PRIMARY_WEAPON] && (ch->equipment[SECONDARY_WEAPON] ||
      ch->equipment[THIRD_WEAPON] || ch->equipment[FOURTH_WEAPON]) && twoskl)
  {
    if(twoskl > 0 && twoskl < 20)
    {
      ch->points.hitroll += 1;
      ch->points.damroll += 1;
    }
    else if(twoskl >= 20 && twoskl < 40)
    {
      ch->points.hitroll += 2;
      ch->points.damroll += 2;
    }
    else if(twoskl >= 40 && twoskl < 60)
    {
      ch->points.hitroll += 3;
      ch->points.damroll += 3;
    }
    else if(twoskl >= 60 && twoskl < 80)
    {
      ch->points.hitroll += 4;
      ch->points.damroll += 4;
    }
    else if(twoskl >= 80 && twoskl < 100)
    {
      ch->points.hitroll += 5;
      ch->points.damroll += 5;
    }
    else if(twoskl == 100)
    {
      ch->points.hitroll += 6;
      ch->points.damroll += 6;
    }
  }
  return twoskl;
}


void do_infuse(P_char ch, char *arg, int cmd)
{
  P_obj device, t_obj, nextobj, stone = NULL;
  char Gbuf1[MAX_STRING_LENGTH], msg[MAX_STRING_LENGTH];
  int skill, c, i = 0;
  int charges, maxcharges;
  int check;
  struct affected_type af;

  if(!(skill = GET_CHAR_SKILL(ch, SKILL_INFUSE_MAGICAL_DEVICE)))
  {
    send_to_char("You don't know how.\r\n", ch);
    return;
  }

  arg = one_argument(arg, Gbuf1);

  if(!(device = get_object_in_equip_vis(ch, Gbuf1, &i)))
  {
    send_to_char("You need to hold something to infuse it.\r\n", ch);
    return;
  }

  if((device->type != ITEM_STAFF) &&
       (device->type != ITEM_WAND))
  {
    send_to_char("You can't infuse that!\r\n", ch);
    return;
  }

  if(get_spell_from_char(ch, SKILL_INFUSE_MAGICAL_DEVICE))
  {
    send_to_char("You need to wait before you can infuse a device again.\r\n", ch);
    return;
  }

  if((device->type == ITEM_STAFF) &&
       (skill < 40))
  {
    send_to_char("You're not proficient enough to infuse a staff yet.\r\n", ch);
    return;
  }

  if(isname("wicked", device->name))
  {
    send_to_char("You do not possess the extreme power to infuse this particular item.\r\n", ch);
    return;
  }

  if(device->value[7] >= 2)
  {
    send_to_char("This device is too worn out to be infused.\r\n", ch);
    return;
  }

  if(device->value[2] == device->value[1])
  {
    send_to_char("That device is already fully charged!\r\n", ch);
    return;
  }

  for (t_obj = ch->carrying; t_obj; t_obj = nextobj)
  {
    nextobj = t_obj->next_content;
    if(obj_index[t_obj->R_num].virtual_number == RANDOM_OBJ_VNUM)
    {
      if(isname("_strange_", t_obj->name))
      {
        stone = t_obj;
        break;
      }
    }
  }

  if(stone)
  {
    snprintf(msg, MAX_STRING_LENGTH, "&+WYou infuse the magic from %s &+Winto %s&+W.&n\r\n",
            stone->short_description, device->short_description);
    send_to_char(msg, ch);
    obj_from_char(stone);
    extract_obj(stone, FALSE);
  }
  else
  {
    send_to_char("You lack a magical stone to infuse with.\r\n", ch);
    return;
  }

  charges = device->value[2];
  maxcharges = device->value[1];

  for (check = 0, c = charges; c < maxcharges; c++)
  {
    snprintf(msg, MAX_STRING_LENGTH, "&+wYou infuse %s &+wwith a charge!&n\r\n", device->short_description);
    send_to_char(msg, ch);
    device->value[2]++;

    if(device->value[2] == device->value[1])
    {
      snprintf(msg, MAX_STRING_LENGTH, "&+W%s &+Whas been fully infused!\r\n", device->short_description);
      send_to_char(msg, ch);
      break;
    }

    check += 10;
    if((number(0, 100) < ((check) - (skill/2))) || (check == 100))
    {
      send_to_char("&+LYou can't infuse this anymore today.&n\r\n",  ch);
      break;
    }
  }

  memset(&af, 0, sizeof(af));
  af.type = SKILL_INFUSE_MAGICAL_DEVICE;
  af.duration = 24;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
  affect_to_char(ch, &af);

  if(skill >= 70)
  {
    device->value[7]++;
  }
  else
  {
    device->value[7] += 2;
  }

  CharWait(ch, (PULSE_VIOLENCE * 5));
}
