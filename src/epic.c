
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <string>
#include <vector>
#include <list>
using namespace std;

#include "structs.h"
#include "spells.h"
#include "comm.h"
#include "interp.h"
#include "prototypes.h"
#include "utils.h"
#include "damage.h"
#include "db.h"
#include "sql.h"
#include "epic.h"
#include "objmisc.h"
#include "events.h"
#include "random.zone.h"
#include "timers.h"
#include "assocs.h"
#include "nexus_stones.h"
#include "auction_houses.h"
#include "boon.h"
#include "trophy.h"
#include "epic_bonus.h"

extern P_room world;
extern P_index obj_index;
extern P_index mob_index;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern P_obj object_list;
extern int top_of_objt;
extern P_desc descriptor_list;
extern Skill skills[];
extern int      new_exp_table[];
extern void event_reset_zone(P_char, P_char, P_obj, void *);

vector<epic_zone_completion> epic_zone_completions;

const char *prestige_names[EPIC_MAX_PRESTIGE] = {
  "Unknown",
  "Serf",
  "Peasant",
  "Commoner",
  "Citizen",
  "Squire",
  "Noble",
  "Knight",
  "Hero",
  "Lord",
  "Champion",
  "Living Legend"
};

struct epic_reward {
  unsigned char type;
  int value;
  int min_points;
  int points_cost;
  int coins;
  unsigned int classes;
} epic_rewards[] = {
  {EPIC_REWARD_SKILL, SKILL_ANATOMY, 100, 1, 500000,
   CLASS_WARRIOR | CLASS_MERCENARY | CLASS_RANGER |
   CLASS_REAVER | CLASS_BERSERKER | CLASS_MONK |
   CLASS_DREADLORD | CLASS_CLERIC | CLASS_ROGUE |
   CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_AVENGER},
  {EPIC_REWARD_SKILL, SKILL_CHANT_MASTERY, 100, 1, 500000,
   CLASS_SORCERER | CLASS_CONJURER | CLASS_ILLUSIONIST |
   CLASS_NECROMANCER | CLASS_THEURGIST | CLASS_BARD},
  {EPIC_REWARD_SKILL, SKILL_SUMMON_BLIZZARD, 500, 1, 1000000,
   CLASS_SHAMAN | CLASS_SORCERER | CLASS_ETHERMANCER |
   CLASS_DRUID},
  {EPIC_REWARD_SKILL, SKILL_SUMMON_FAMILIAR, 100, 1, 500000,
   CLASS_SORCERER | CLASS_CONJURER | CLASS_SHAMAN |
   CLASS_DRUID | CLASS_NECROMANCER | CLASS_THEURGIST | 
   CLASS_ALCHEMIST | CLASS_ILLUSIONIST},
  {EPIC_REWARD_SKILL, SKILL_ADVANCED_MEDITATION, 100, 1, 500000,
   CLASS_SORCERER | CLASS_CONJURER | CLASS_ILLUSIONIST |
   CLASS_NECROMANCER | CLASS_THEURGIST | CLASS_PSIONICIST | 
   CLASS_PALADIN | CLASS_ANTIPALADIN | CLASS_BARD | 
   CLASS_REAVER },
  {EPIC_REWARD_SKILL, SKILL_DEVOTION, 500, 1, 1000000,
   CLASS_CLERIC | CLASS_PALADIN | CLASS_ANTIPALADIN |
   CLASS_ETHERMANCER },
  {EPIC_REWARD_SKILL, SKILL_SCRIBE_MASTERY, 500, 1, 1000000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_NECROMANCER |
    CLASS_THEURGIST | CLASS_ILLUSIONIST },
  {EPIC_REWARD_SKILL, SKILL_SNEAKY_STRIKE, 100, 1, 1000000,
    CLASS_ROGUE | CLASS_BARD | CLASS_MERCENARY },
  {EPIC_REWARD_SKILL, SKILL_SILENT_SPELL, 500, 1, 1000000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_ILLUSIONIST |
    CLASS_NECROMANCER | CLASS_THEURGIST },
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_LISTEN, 100, 1, 500000,
    CLASS_ROGUE | CLASS_BARD | CLASS_MERCENARY},
  {EPIC_REWARD_SKILL, SKILL_SHIELD_COMBAT, 100, 1, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_SHIELD_COMBAT, 1000, 1, 1000000,
    CLASS_WARRIOR | CLASS_CLERIC },
  {EPIC_REWARD_SKILL, SKILL_TWOWEAPON, 100, 1, 1000000,
    CLASS_WARRIOR | CLASS_MERCENARY | CLASS_ROGUE | CLASS_RANGER |
	CLASS_REAVER | CLASS_BERSERKER},
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_TWOWEAPON, 1000, 1, 1000000,
    CLASS_WARRIOR | CLASS_MERCENARY | CLASS_ROGUE | CLASS_RANGER |
        CLASS_REAVER | CLASS_BERSERKER},
  {EPIC_REWARD_SKILL, SKILL_JIN_TOUCH, 100, 1, 1000000,
  CLASS_MONK},
  {EPIC_REWARD_SKILL, SKILL_KI_STRIKE, 100, 1, 1000000,
  CLASS_MONK},
  {EPIC_REWARD_SKILL, SKILL_INFUSE_LIFE, 100, 1, 1000000,
  CLASS_CONJURER | CLASS_NECROMANCER | CLASS_THEURGIST | 
    CLASS_SHAMAN},
  {EPIC_REWARD_SKILL, SKILL_SPELL_PENETRATION, 500, 2, 5000000,
    CLASS_SORCERER | CLASS_CONJURER },
  {EPIC_REWARD_SKILL, SKILL_DEVASTATING_CRITICAL, 200, 1, 1000000,
  CLASS_ANTIPALADIN | CLASS_PALADIN | CLASS_AVENGER | CLASS_DREADLORD },
  {EPIC_REWARD_SKILL, SKILL_TOUGHNESS, 100, 1, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_SPATIAL_FOCUS, 100, 1, 1000000, CLASS_PSIONICIST},
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_ENDURANCE, 100, 1, 500000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_IMPROVED_TRACK, 100, 1, 1000000,
    CLASS_ROGUE | CLASS_MERCENARY | CLASS_RANGER},
  {EPIC_REWARD_SKILL, SKILL_EMPOWER_SONG, 100, 1, 500000,
    CLASS_BARD},
  {EPIC_REWARD_SKILL, SKILL_FIX, 100, 1, 100000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_CRAFT, 100, 1, 10000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_ENCRUST, 100, 1, 100000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_ENCHANT, 500, 1, 100000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_SPELLBIND, 250, 1, 100000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_SMELT, 100, 1, 10000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_FORGE, 100, 1, 100000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_TOTEMIC_MASTERY, 250, 1, 1000000,
    CLASS_SHAMAN },
  {EPIC_REWARD_SKILL, SKILL_INFUSE_MAGICAL_DEVICE, 100, 1, 1000000,
    CLASS_SORCERER | CLASS_CONJURER | CLASS_NECROMANCER | 
    CLASS_THEURGIST | CLASS_ETHERMANCER |CLASS_BARD | 
    CLASS_DRUID | CLASS_CLERIC | CLASS_PSIONICIST |
    CLASS_ILLUSIONIST | CLASS_ALCHEMIST | CLASS_SHAMAN },
  {EPIC_REWARD_SKILL, SKILL_INDOMITABLE_RAGE, 100, 1, 500000,
  CLASS_WARRIOR | CLASS_BERSERKER },
  {EPIC_REWARD_SKILL, SKILL_NATURES_SANCTITY, 100, 1, 1000000,
  CLASS_DRUID },
  {EPIC_REWARD_SKILL, SKILL_EXPERT_PARRY, 1000, 1,  2000000,
   CLASS_WARRIOR | CLASS_PALADIN | CLASS_RANGER},
  {EPIC_REWARD_SKILL, SKILL_EXPERT_RIPOSTE, 1000, 1, 2000000,
   CLASS_WARRIOR | CLASS_ANTIPALADIN | CLASS_DREADLORD | CLASS_AVENGER},
  {EPIC_REWARD_SKILL, SKILL_EPIC_STRENGTH, 100, 1, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_POWER, 100, 1, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_AGILITY, 100, 1, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_INTELLIGENCE, 100, 1, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_DEXTERITY, 100, 1, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_WISDOM, 100, 1, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_CONSTITUTION, 100, 1, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_CHARISMA, 100, 1, 1000000,
    0 },
  {EPIC_REWARD_SKILL, SKILL_EPIC_LUCK, 100, 1, 1000000,
    0 },
//  {EPIC_REWARD_SKILL, SKILL_SHIP_DAMAGE_CONTROL, 1000, 2, 8000000,
//    0 },
  {0}
};

struct epic_teacher_skill {
  int vnum;
  int skill;
  int min;
  int max;
  int pre_requisite;
  int deny_skill;
  int pre_req_lvl;
} epic_teachers[] = {
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
  {9454,  SKILL_CRAFT, 0, 100, 0, 0, 0}, //Rjinal in Samirz
  {40760, SKILL_ENCRUST, 0, 100, SKILL_CRAFT, 0, 100}, //Snent in Divine Home
  {78006, SKILL_ENCHANT, 0, 100, 0, SKILL_SPELLBIND, 0}, //Bargor in Oasis
  {94017, SKILL_SPELLBIND, 0, 100, 0, SKILL_ENCHANT, 0}, //Kalroh in Maze of Undead Army
  {37145, SKILL_SMELT, 0, 100, 0, 0, 0}, //Carmotee in Dumaathe
  {21618, SKILL_FORGE, 0, 100, 0, 0, 0}, //Tenkuss in Aravne
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
//  {2733, SKILL_SHIP_DAMAGE_CONTROL, 0, 100, 0, 0, 0}, // Commodore in Headless
  {0}
};

int epic_points(P_char ch)
{
  if(IS_NPC(ch))
    return 0;
  else
    return ch->only.pc->epics;
}

const char *epic_prestige(P_char ch)
{
  return prestige_names[MIN(epic_points(ch)/get_property("epic.prestigeNotch", 400), EPIC_MAX_PRESTIGE-1)];
}

int epic_skillpoints(P_char ch)
{
  if(!ch || !IS_PC(ch))
    return 0;

  return ch->only.pc->epic_skill_points;
}

void epic_gain_skillpoints(P_char ch, int gain)
{
  if(!ch || !IS_PC(ch))
    return;

  ch->only.pc->epic_skill_points = MAX(0, ch->only.pc->epic_skill_points + gain);
}

bool epic_stored_in(unsigned int *vector, int code)
{
  unsigned int flag = *vector;

  if((flag >> 30) == 0)
    *vector = (unsigned int)code;
  else if((flag >> 30) == 1)
    *vector |= (unsigned int)code << 10;
  else if((flag >> 30) == 2)
    *vector |= (unsigned int)code << 20;
  else
    return false;

  *vector += 0x40000000;

  return true;
}

void epic_complete_errand(P_char ch, int zone)
{
  struct affected_type af, *afp;

  for (afp = ch->affected; afp; afp = afp->next) {
    if(afp->type == TAG_EPIC_COMPLETED &&
        afp->modifier < 12) {
      if(!epic_stored_in(&afp->bitvector, zone))
        if(!epic_stored_in(&afp->bitvector2, zone))
          if(!epic_stored_in(&afp->bitvector3, zone))
            epic_stored_in(&afp->bitvector4, zone);
      afp->modifier++;
      break;
    }
  }

  if(!afp) {
    memset(&af, 0, sizeof(af));
    af.type = TAG_EPIC_COMPLETED;
    af.modifier = 1;
    af.flags = AFFTYPE_STORE | AFFTYPE_PERM;
    af.duration = -1;
    af.bitvector = 0x40000000 | zone;
    affect_to_char(ch, &af);
  }
}

int epic_random_task_zone(P_char ch)
{
  int zone_number = -1;
#ifdef __NO_MYSQL__
  return zone_number;
#else
  if(!qry("select number, name from zones where task_zone = 1 and number not in " \
           "(select type_id from epic_gain where pid = '%d' and type = '%d') " \
           "order by rand() limit 1", GET_PID(ch), EPIC_ZONE))
    return -1;

  MYSQL_RES *res = mysql_store_result(DB);

  if(mysql_num_rows(res) > 0)
  {
    MYSQL_ROW row = mysql_fetch_row(res);
    zone_number = atoi(row[0]);
  }

  mysql_free_result(res);

  return zone_number;

#endif
}

void epic_choose_new_epic_task(P_char ch)
{
  char buffer[512];
  int zone_number = -1;
  P_obj nexus;

  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }
  
  struct affected_type af, *afp;
  memset(&af, 0, sizeof(af));
  af.type = TAG_EPIC_ERRAND;
  af.flags = AFFTYPE_STORE | AFFTYPE_PERM;
  af.duration = -1;

  if(number(0, 5))
  {
    zone_number = epic_random_task_zone(ch);
  }

  if(zone_number < 0)
  {
    nexus = get_random_enemy_nexus(ch);
    if((number(0, 100) < 50) && (GET_LEVEL(ch) >= 51) && nexus)
    {
      act("The Gods of &+rDuris&n demand that you seek out $p and convert it!", FALSE, ch, nexus, 0, TO_CHAR);
      af.modifier = -STONE_ID(nexus);
    }
    else
    {
      send_to_char("The Gods of &+rDuris&n demand that you &+rspill the &+Rblood&n of the &+Lenemies&n of your race!\n", ch);
      af.modifier = SPILL_BLOOD;
    }
  }
  else
  {
    sprintf(buffer, "The Gods of &+rDuris&n have sent you to seek out the &+Bmagical &+Lstone&n of %s!\n", zone_table[real_zone0(zone_number)].name);
    send_to_char(buffer, ch);
    af.modifier = zone_number;
  }

  affect_to_char(ch, &af);
}

vector<epic_trophy_data> get_epic_zone_trophy(P_char ch)
{
  vector<epic_trophy_data> trophy;

#ifdef __NO_MYSQL__
  debug("get_epic_zone_trophy(): __NO_MYSQL__, returning 0");
  return trophy;
#else
  if(!qry("select type_id from epic_gain where pid = '%d' and type = '%d' order by time asc", GET_PID(ch), EPIC_ZONE))
    return trophy;

  MYSQL_RES *res = mysql_store_result(DB);

  if(!res)
  {
    mysql_free_result(res);
    return trophy;
  }

  list<epic_trophy_data> tq;

  int trophy_size = (int) get_property("epic.zoneTrophy.size", 40);

  MYSQL_ROW row;
  while(row = mysql_fetch_row(res))
  {
    int zone_number = atoi(row[0]);

    bool in_trophy = false;
    for(list<epic_trophy_data>::iterator it = tq.begin(); it != tq.end(); it++)
    {
      if(it->zone_number == zone_number)
      {
        in_trophy = true;
        break;
      }
    }

    if(!in_trophy)
    {
      tq.push_front(epic_trophy_data(zone_number, 1));
      if(tq.size() > trophy_size) tq.pop_back();
    }
    else
    {
      for(list<epic_trophy_data>::iterator it = tq.begin(); it != tq.end(); it++)
      {
        if(it->zone_number == zone_number)
        {
          it->count++;
          break;
        }
      }
    }

  }

  while(!tq.empty())
  {
    trophy.push_back(tq.front());
    tq.pop_front();
  }

  mysql_free_result(res);

  return trophy;

#endif
}

int modify_by_epic_trophy(P_char ch, int amount, int zone_number)
{
  vector<epic_trophy_data> trophy = get_epic_zone_trophy(ch);

  for(vector<epic_trophy_data>::iterator it = trophy.begin(); it != trophy.end(); it++)
  {
    if(zone_number == it->zone_number  && it->count > 0)
    {
      float factor = pow(get_property("epic.zoneTrophy.mod", 0.8), MIN(it->count, get_property("epic.zoneTrophy.maxMods", 4)));
      amount = (int) (amount * factor);
      amount = MAX(1, amount);

      switch(it->count)
      {
        case 1:
          send_to_char("This seems familiar somehow...\n", ch);
          break;

        case 2:
        case 3:
          send_to_char("&+GHaven't you seen all of this before?\n", ch);
          break;

        case 4:
        default:
          send_to_char("&+RThis is getting extremely boring.\n", ch);
          break;
      }

      return amount;
    }

  }

  return amount;
}

void group_gain_epic(P_char ch, int type, int data, int amount)
{
  gain_epic(ch, type, data, amount);

  if(ch->group)
  {
    for(struct group_list *gl = ch->group; gl; gl = gl->next)
    {
      if(gl->ch == ch) continue;
      if(gl->ch->in_room == ch->in_room)
      {
        gain_epic(gl->ch, type, data, amount);
      }
    }
  }
}

void gain_epic(P_char ch, int type, int data, int amount)
{
  char buffer[256];
  struct affected_type af, *afp;
  int notch = get_property("epic.skillPointStep", 100);
  int errand_notch = get_property("epic.errandStep", 200);
  int old;
  
  if(!(ch) ||
     !IS_ALIVE(ch))
  {
    return;
  }

  if(amount < 1 ||
    IS_NPC(ch))
  {
    return;
  }

  if(IS_AFFECTED4(ch, AFF4_EPIC_INCREASE))
  {
    send_to_char("You feel the &+cblessing&n of the &+WGods&n wash over you.\n", ch);
	amount = (int) (amount * get_property("epic.witch.multiplier", 1.5));
  }

  if(type != EPIC_PVP && type != EPIC_SHIP_PVP && has_epic_task(ch))
  {
    send_to_char("You have not completed the task given to you by the Gods, \n" \
                 "so you are not able to progress at usual pace.\n", ch);
    amount = MAX(1, (int) (amount * get_property("epic.errand.penaltyMod", 0.25)));
  }

  // For marduk nexus stone... to change rate use property nexusStones.bonus.epics
  amount = check_nexus_bonus(ch, amount, NEXUS_BONUS_EPICS);
  amount = amount + (int)((float)amount * get_epic_bonus(ch, EPIC_BONUS_EPIC_POINT));

  if(GET_RACEWAR(ch) == RACEWAR_GOOD)
    amount = amount * (float)get_property("epic.gain.modifier.good", 1.000);
  if(GET_RACEWAR(ch) == RACEWAR_EVIL)
    amount = amount * (float)get_property("epic.gain.modifier.evil", 1.000);
  
  // add guild prestige
  check_assoc_prestige_epics(ch, amount, type);

  old = ch->only.pc->epics;
  sprintf(buffer, "You have gained %d epic point%s.\n", amount, amount == 1 ? "" : "s");
  send_to_char(buffer, ch);
  ch->only.pc->epics += amount;
  log_epic_gain(GET_PID(ch), type, data, amount);

  char type_str[10];
 
  switch(type)
  {
    case EPIC_ZONE:
      strcpy(type_str, "ZONE");
      break;
    case EPIC_PVP:
      strcpy(type_str, "PVP");
      break;
    case EPIC_SHIP_PVP:
      strcpy(type_str, "PVP_SHIP");
      break;
    case EPIC_ELITE_MOB:
      strcpy(type_str, "ELITE_MOB");
      break;
    case EPIC_QUEST:
      strcpy(type_str, "QUEST");
      break;
    case EPIC_RANDOM_ZONE:
      strcpy(type_str, "RANDOM_ZONE");
      break;
    case EPIC_NEXUS_STONE:
      strcpy(type_str, "NEXUS_STONE");
      break;
    case EPIC_BOON:
      strcpy(type_str, "BOON");
      break;
    default:
      strcpy(type_str, "");
      break;
  }

  statuslog(GREATER_G, "%s received %d epic points (%s)", ch->player.name, amount, type_str);

  /*
    exp.maxExpLevel means the highest level you can reach with just experience (i.e., without epics)
    epic.maxFreeLevel means the highest level you can reach by touching any stone. higher levels you have
    to touch specific stones to level, except in wipe2011
  */

  if(GET_LEVEL(ch) >= get_property("exp.maxExpLevel", 46) &&
      GET_LEVEL(ch) < get_property("epic.maxFreeLevel", 50))
  {
     epic_free_level(ch);
     //advance_level(ch);//, FALSE); handles leveling for wipe2011
  }

  // feed artifacts
  epic_feed_artifacts(ch, amount, type);

  int skill_notches = MAX(0, (int) ((old+amount)/notch) - (old/notch));
  
  if(skill_notches)
  {
    send_to_char("&+WYou have gained an epic skill point!&n\n", ch);
    epic_gain_skillpoints(ch, skill_notches);
  }

  if((old / errand_notch < (old + amount) / errand_notch) && !has_epic_task(ch))
  {
    epic_choose_new_epic_task(ch);
  }

}

struct affected_type *get_epic_task(P_char ch)
{
  struct affected_type *hjp;
  
  if(!ch)
    return NULL;
  
  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if(hjp->type == TAG_EPIC_ERRAND)
      return hjp;
  
  return NULL;
}

bool has_epic_task(P_char ch)
{
  return (get_epic_task(ch) != NULL);
}

void epic_frag(P_char ch, int victim_pid, int amount)
{
  struct affected_type *afp;

  if(afp = get_epic_task(ch)) {
    if(afp->modifier == SPILL_BLOOD) {
      send_to_char("The &+rGods of Duris&n are very pleased with this &+rblood&n.\n", ch);
      send_to_char("You can now progress further in your quest for epic power!\n", ch);
      amount *= 2;
      affect_remove(ch, afp);
    }
  }
  gain_epic(ch, EPIC_PVP, victim_pid, amount);
}

void epic_feed_artifacts(P_char ch, int epics, int epic_type)
{
  if(IS_TRUSTED(ch) || !IS_PC(ch))
    return;

  int num_artis = 0;
  for (int i = 0; i < MAX_WEAR; i++)
  {
    if(ch->equipment[i] && (IS_ARTIFACT(ch->equipment[i]) || isname("powerunique", ch->equipment[i]->name)))
    {
      num_artis++;
    }
  }

  int feed_seconds = (int) (epics * get_property("artifact.feeding.epic.point.seconds", 3600));

  switch(epic_type)
  {
    case EPIC_ZONE:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.zone", 1.0));
      break;
    case EPIC_PVP:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.pvp", 2.0));
      break;
    case EPIC_SHIP_PVP:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.pvpShip", 2.0));
      break;
    case EPIC_ELITE_MOB:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.eliteMob", 1.0));
      break;
    case EPIC_QUEST:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.quest", 1.0));
      break;
    case EPIC_RANDOM_ZONE:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.randomZone", 1.0));
      break;
    case EPIC_NEXUS_STONE:
      feed_seconds = (int) (feed_seconds * get_property("artifact.feeding.epic.typeMod.nexusStone", 1.0));
      break;
    default:
      break;
  }

  if(num_artis > 0)
    feed_seconds = (int) (feed_seconds / num_artis);

  for (int i = 0; i < MAX_WEAR; i++)
  {
    P_obj obj = ch->equipment[i];
    if(obj && IS_ARTIFACT(obj))
    {
      feed_artifact(ch, ch->equipment[i], feed_seconds, ((epic_type == EPIC_PVP || epic_type == EPIC_SHIP_PVP) ? TRUE : FALSE));
    }
  }
}

/* epic stones absorb smaller stones and level potions into themselves */
void epic_stone_absorb(P_obj obj)
{
  P_obj obj_list = NULL;

  if(OBJ_ROOM(obj))
  {
    obj_list = world[obj->loc.room].contents;
  }
  else if(OBJ_CARRIED(obj))
  {
    obj_list = (obj->loc.carrying)->carrying;
  }

  if(!obj_list)
    return;

  for(P_obj tobj = obj_list; tobj; tobj = tobj->next_content)
  {
    if(tobj == obj)
      continue;

   /* if the other object is smaller epic stone, absorb it */
   if(GET_OBJ_VNUM(tobj) <= GET_OBJ_VNUM(obj) &&
       obj_index[tobj->R_num].func.obj == epic_stone)
   {
     extract_obj(tobj, TRUE);
   }

   /* if there is a level potion on the ground, absorb it */
   if(tobj->affected[0].location == APPLY_LEVEL)
   {
     extract_obj(tobj, TRUE);
   }
  }
}

/* calculate the epic point payout based on members in group */
int epic_stone_payout(P_obj obj, P_char ch)
{
  int num_players = 1;
  if(ch->group)
  {
    for(struct group_list *gl = ch->group; gl; gl = gl->next)
    {
      if(gl->ch == ch)
      {
        continue;
      }
      if(!IS_PC(gl->ch) ||
         IS_TRUSTED(gl->ch))
      {
        continue;
      }
      if(gl->ch->in_room == ch->in_room)
      {
        num_players++;
      }
    }
  }

  if(num_players < obj->value[1] && obj->value[1] != 0)
      num_players = obj->value[1];
  
  /* epic value is
    the old payout value * number of touches / total group players in room,
    max = (1.5) * the old payout value */

  int payout = (int) (obj->value[0] * obj->value[1] / num_players);
  int max_payout = (int) (obj->value[0] * (float) get_property("epic.touch.maxPayoutFactor", 1.5));

  int epic_value = BOUNDED(1, payout, max_payout);

//  DEPRECATED - Torgal 12/21/09
//  float freq_mod = get_epic_zone_frequency_mod(obj->value[2]);
//  int __old_epic_value = epic_value;
//
//  epic_value = MAX(1, (int) (epic_value * freq_mod));
//  debug("epic_stone_payout:freq_mod: old_epic_value: %d, epic_value: %d", __old_epic_value, epic_value);

  float alignment_mod = get_epic_zone_alignment_mod(obj->value[2], GET_RACEWAR(ch));
  int __old_epic_value = epic_value;

  epic_value = MAX(1, (int) (epic_value * alignment_mod));
  debug("epic_stone_payout:alignment_mod: old_epic_value: %d, epic_value: %d", __old_epic_value, epic_value);  

  epic_value = epic_value * get_property("epic.touch.PayoutFactor", 1.000);

  return epic_value;
}

void epic_stone_feed_artifacts(P_obj obj, P_char ch)
{
  int feed_amount = 0;
  switch(GET_OBJ_VNUM(obj))
  {
    case EPIC_MONOLITH:
      feed_amount = 3600 * get_property("artifact.feeding.epic.hours.monolith", 12);
      break;

    case EPIC_LARGE_STONE:
      feed_amount = 3600 * get_property("artifact.feeding.epic.hours.large", 6);
      break;

    case EPIC_SMALL_STONE:
      feed_amount = 3600 * get_property("artifact.feeding.epic.hours.small", 1);
      break;

    default:
      feed_amount = 0;
  }
}

void epic_stone_set_affect(P_char ch)
{
  struct affected_type af;
  memset(&af, 0, sizeof(af));
  af.type = TAG_EPIC_MONOLITH;
  af.flags = AFFTYPE_STORE | AFFTYPE_PERM;
  af.duration = 1 + get_property("epic.monolith.activeTime", 60)/75;
  affect_to_char(ch, &af);
}

__attribute__ ((deprecated))
void epic_free_level(P_char ch)
{
  char buf[256];
  sprintf(buf, "epic.forLevel.%d", GET_LEVEL(ch)+1);

  int epics_for_level = get_property(buf, 1 << ((GET_LEVEL(ch) + 1) - 43));

  if(GET_EXP(ch) >= new_exp_table[GET_LEVEL(ch)+1] &&
         ch->only.pc->epics >= epics_for_level)
     {
         GET_EXP(ch) -= new_exp_table[GET_LEVEL(ch) + 1];
         advance_level(ch);//, FALSE); wipe2011
		 wizlog(56, "%s has attained epic level &+W%d&n!",
                GET_NAME(ch),
                GET_LEVEL(ch));
     }

}

void epic_stone_level_char(P_obj obj, P_char ch)
{
  if(IS_MULTICLASS_PC(ch) &&
      GET_LEVEL(ch) >= get_property("exp.maxMultiLevel", 56))
    return;

  char buf[256];
  sprintf(buf, "epic.forLevel.%d", GET_LEVEL(ch)+1);

  int epics_for_level = get_property(buf, 1 << (obj->value[3] - 43));

  if(IS_MULTICLASS_PC(ch) && GET_LEVEL(ch) >= 51)
  {
    epics_for_level *= (int) get_property("exp.multiEpicMultiplier", 3);
  }

#if defined(CTF_MUD) && (CTF_MUD == 1)
  epics_for_level = (int)(epics_for_level/3);
#endif

  if(GET_EXP(ch) >= new_exp_table[GET_LEVEL(ch)+1] &&
      ch->only.pc->epics >= epics_for_level)
  {
    GET_EXP(ch) -= new_exp_table[GET_LEVEL(ch) + 1];
    advance_level(ch);//, TRUE); wipe2011
	wizlog(56, "%s has attained epic level &+W%d&n!",
           GET_NAME(ch),
           GET_LEVEL(ch));
  }
}

void epic_stone_one_touch(P_obj obj, P_char ch, int epic_value)
{
  if(!obj || !ch || !epic_value)
    return;

  /*if(get_zone_exp(ch, world[ch->in_room].zone) < calc_min_zone_exp(ch))	
  {
    act("The burst of &+Bblue energy&n from $p flows around $n, leaving them unaffected!",
        FALSE, ch, obj, ch, TO_NOTVICT);
    act("The burst of &+Bblue energy&n from $p flows around you, leaving you unaffected!",
        FALSE, ch, obj, 0, TO_CHAR);
    send_to_char("You did not gain enough experience here before trying to gain more power!\n", ch);
    return;
  }*/

  act("The mystic &+Bblue energy&n from $p flows into $n!", FALSE, ch, obj, ch, TO_NOTVICT);
  act("The mystic &+Bblue energy&n from $p flows into you!", FALSE, ch, obj, 0, TO_CHAR);

  act("From deep inside, you realize that you have reached one of the key nodes of the\n"
      "magical energies flowing through the World of &+rDuris&n!\n"
      "Your body and mind align smoothly with the energy, embracing its powers, giving\n"
      "you strength and new knowledge!", FALSE, ch, obj, 0, TO_CHAR);

  epic_stone_set_affect(ch);

  /* if char is completing their epic errand, give them extra epic points! */
  struct affected_type *afp = get_epic_task(ch);
  if(afp && afp->modifier == obj->value[2])
  {
    send_to_char("The &+rGods of Duris&n are very pleased with your achievement!\n"
                 "You can now continue with your quest for &+Wpower!\n", ch);
    epic_complete_errand(ch, afp->modifier);
    affect_remove(ch, afp);
    gain_epic(ch, EPIC_ZONE, obj->value[2], (int) (epic_value * get_property("epic.errand.completeBonusMod", 1.5)));

  } else {
    /* not on epic errand, just give them the epic points */
    gain_epic(ch, EPIC_ZONE, obj->value[2], epic_value);
  }

  if(GET_LEVEL(ch) == (obj->value[3] - 1))
  {
    epic_stone_level_char(obj, ch);
  }
  check_boon_completion(ch, NULL, obj->value[2], BOPT_ZONE);
}

int epic_stone(P_obj obj, P_char ch, int cmd, char *arg)
{
  int zone_number = -1;
  char arg1[MAX_INPUT_LENGTH];
  P_obj stoneobj = NULL;

  if(cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(obj && cmd == CMD_PERIODIC)
  {
    /* periodic call */
    epic_stone_absorb(obj);

    /* set zone id */
    if(!obj->value[2])
    {
      zone_number = zone_table[obj_zone_id(obj)].number;
      obj->value[2] = zone_number;

      // set epic payout, suggested_group_size and epic_level from db
      struct zone_info zinfo;
      if(get_zone_info(zone_number, &zinfo))
      {
        obj->value[0] = zinfo.epic_payout;
        obj->value[1] = zinfo.suggested_group_size;
        obj->value[3] = zinfo.epic_level;
      }
    }

    if(OBJ_ROOM(obj))
    {
      REMOVE_BIT(obj->wear_flags, ITEM_TAKE);

      if(OBJ_MAGIC(obj) && !number(0,5))
      {
        act("A powerful humming sound can be heard from $p.", FALSE, 0, obj, 0, TO_ROOM);
      }
    }
  }

  if(cmd == CMD_TOUCH && IS_PC(ch))
  {
    one_argument(arg, arg1);
    stoneobj = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if(!stoneobj)
    {
      stoneobj = get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents);
      if(!stoneobj)
        return FALSE;
    }

    if(stoneobj != obj)
      return FALSE;

    zone_number = obj->value[2];

    /* the (magic) flag determines if the stone has been touched or not */
    if(!OBJ_MAGIC(obj) || epic_zone_done_now(zone_number))
    {
      act("$p seems to be powerless.", FALSE, ch, obj, 0, TO_CHAR);
      return TRUE;
    }

    for (P_char tmp_p = world[ch->in_room].people; tmp_p; tmp_p = tmp_p->next_in_room)
    {
      if(IS_FIGHTING(tmp_p))
      {
        send_to_char("It's not peaceful enough here!\r\n", ch);
        return TRUE;
      }
    }

    if(IS_TRUSTED(ch))
    {
      send_to_char("But you're already epic enough!\r\n", ch);
      return TRUE;
    }

    /* stones must be touched in the zone in which they were loaded */
    
    if(zone_number && world[ch->in_room].zone != real_zone(zone_number))
    {
      act("A sick noise emanates from $p, and a large crack runs down the side! Something was misplaced!",
           FALSE, ch, obj, 0, TO_CHAR);
      act("A sick noise emanates from $p, and a large crack runs down the side! Something was misplaced!",
          FALSE, ch, obj, 0, TO_ROOM);

      REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);
      return TRUE;
    }
    /*
    if(affected_by_spell(ch, TAG_EPIC_MONOLITH))
    {
      send_to_char("You need to wait before touching an epic stone again!\n", ch);
      return TRUE;
    } this mitigation is no longer necessary for wipe2011, as exp must be gained in zone before touch
      can reward player - Jexni */

    /* calculate epic value */
    int epic_value = epic_stone_payout(obj, ch);

    act("$n touches $p.", FALSE, ch, obj, ch, TO_NOTVICT);
    act("You touch $p.", FALSE, ch, obj, 0, TO_CHAR);

    act("$p begins to vibrate madly, shaking the entire room\n"
        "almost knocking you off your feet!\n"
        "Suddenly, a huge storm of &+Bblue energy&n erupts from it!", FALSE, ch, obj, 0, TO_ROOM);

    if(zone_number)
    {
      statuslog(GREATER_G, "%s touched the epic stone in %s", ch->player.name, zone_table[real_zone0(zone_number)].name);
      logit(LOG_EPIC, "%s touched the epic stone in %s", ch->player.name, strip_ansi(zone_table[real_zone0(zone_number)].name).c_str());
    }

    epic_stone_one_touch(obj, ch, epic_value);

    /* go through all members of group */
    if(ch->group)
    {
      for(struct group_list *gl = ch->group; gl; gl = gl->next)
      {
        if(gl->ch == ch) continue;
        if(!IS_PC(gl->ch) || IS_TRUSTED(gl->ch)) continue;
        if(gl->ch->in_room == ch->in_room)
        {
          epic_stone_one_touch(obj, gl->ch, epic_value);
        }
      }
    }

    if(zone_number > 0 && zone_number != RANDOM_ZONE_ID)
    {
      int delta = GET_RACEWAR(ch) == RACEWAR_EVIL ? -1 : 1;
      update_epic_zone_alignment(zone_number, delta);

      // set completed flag
      epic_zone_completions.push_back(epic_zone_completion(zone_number, time(NULL), delta));
      db_query("UPDATE zones SET last_touch = '%d' WHERE number = '%d'", time(NULL), zone_number);

      //  Allow !reset zones to possibly reset somewhere down the line...  - Jexni 11/7/11
      if(!zone_table[zone_number].reset_mode)
      {
        int x = real_zone(zone_number);
        add_event(event_reset_zone, 1, 0, 0, 0, 0, &x, sizeof(x));
        db_query("UPDATE zones SET reset_perc = 1 WHERE number = '%d'", zone_number);
      }
    }

    act("$p flashes brightly then blurs, and remains still and powerless.",
        FALSE, 0, obj, 0, TO_ROOM);
    REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);

    return TRUE;
  }

  return FALSE;
}


void epic_zone_balance()
{
  int i, alignment, delta, lt;
  vector<epic_zone_data> epic_zones = get_epic_zones();
  
  for (i = 0; i <= epic_zones.size(); i++)
  {
    if(!qry("SELECT alignment, last_touch FROM zones WHERE number = %d", epic_zones[i].number))
      return;

    MYSQL_RES *res = mysql_store_result(DB);

    if(mysql_num_rows(res) < 1)
    {
      mysql_free_result(res);
      return;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if(row)
    {
      alignment = atoi(row[0]);
      lt = atoi(row[1]);
    }

    mysql_free_result(res);
    
    if(lt == 0)
      db_query("UPDATE zones SET last_touch='%d' WHERE number='%d'", time(NULL), epic_zones[i].number);

    if((alignment == 0))
      continue;
    
    //debug("zone %d alignment %d", epic_zones[i].number, alignment);

    if(time(NULL) - lt > ((int)get_property("epic.alignment.reset.hour", 7*24)*60*60))
    {
      if(alignment > 0)
        delta = -1;
      else if(alignment < 0)
        delta = 1;

      //debug("calling update_epic_zone_alignment");
      db_query("UPDATE zones SET last_touch='%d' WHERE number='%d'", time(NULL), epic_zones[i].number);
      update_epic_zone_alignment(epic_zones[i].number, delta);
      continue;
    }
  }
}

int epic_teacher(P_char ch, P_char pl, int cmd, char *arg)
{
  int epic_penalty_cost = 1;
  int cash_penalty_cost = 1;
  char buffer[256];

  if(cmd == CMD_PRACTICE)
  {
    // find the teacher
    int t;
    for(t = 0; epic_teachers[t].vnum; t++)
    {
      if(GET_VNUM(ch) == epic_teachers[t].vnum)
        break;
    }

    if(!epic_teachers[t].vnum)
      return FALSE;

    // find the skill
    int s;
    for(s = 0; epic_rewards[s].type; s++)
    {
      if(epic_rewards[s].type == EPIC_REWARD_SKILL &&
          epic_rewards[s].value == epic_teachers[t].skill)
        break;
    }

    if(!epic_rewards[s].type)
      return FALSE;

    int skill = epic_rewards[s].value;
    float cost_f = 1 + GET_CHAR_SKILL(pl, skill) / get_property("epic.progressFactor", 30);
    int points_cost = (int) (cost_f * epic_rewards[s].points_cost);
    int coins_cost = (int) (cost_f * epic_rewards[s].coins);
    
    if(IS_MULTICLASS_PC(pl) &&
      !IS_SET(epic_rewards[s].classes, pl->player.m_class) &&
      IS_SET(epic_rewards[s].classes, pl->player.secondary_class))
    {
      points_cost *= (int) (get_property("epic.multiclass.EpicSkillCost", 2));
      coins_cost *= (int) (get_property("epic.multiclass.EpicPlatCost", 3));
    }

    if(!arg || !*arg)
    {
      // practice called with no arguments
      sprintf(buffer,
              "Welcome, traveller!\n"
              "I am pleased that you have wandered so far in order to seek my assistance.\n"
              "There are few adventurers willing to seek out the knowledge of &+W%s&n.\n\n", skills[skill].name);
      send_to_char(buffer, pl);

      if(epic_rewards[s].classes &&
         !IS_SET(epic_rewards[s].classes, pl->player.m_class) &&
         !IS_SET(epic_rewards[s].classes, pl->player.secondary_class))
      {
        send_to_char("Unfortunately, I am not able to teach people of your class.\n", pl);
        return TRUE;
      }

      if(epic_teachers[t].deny_skill && GET_CHAR_SKILL(pl, epic_teachers[t].deny_skill))
      {
        sprintf(buffer, "I cannot with good conscience teach that skill to someone who has already studied &+W%s&n!\n", skills[epic_teachers[s].deny_skill].name);
        send_to_char(buffer, pl);
        return TRUE;
      }

      if(epic_teachers[t].pre_requisite && GET_CHAR_SKILL(pl, epic_teachers[t].pre_requisite) < epic_teachers[t].pre_req_lvl)
      {
        sprintf(buffer, "You have not yet mastered the art of &+W%s&n!\r\n", skills[epic_teachers[t].pre_requisite].name);
        send_to_char(buffer, pl);
        return TRUE;
      }

      if(GET_CHAR_SKILL(pl, skill) >= 100 || GET_CHAR_SKILL(pl, skill) >= epic_teachers[t].max)
      {
        send_to_char("Unfortunately, I cannot teach you anything more, you have already mastered this skill!\n", pl);
        return TRUE;
      }
      
      sprintf(buffer, "It willcost you &+W%d&n epic skill points and &+W%s&n.\n", points_cost, coin_stringv(coins_cost));
      send_to_char(buffer, pl);
      return TRUE;
    }
    else if(strstr(arg, skills[skill].name))
    {
      // called with skill name
      if(epic_rewards[s].classes &&
          !IS_SET(epic_rewards[s].classes, pl->player.m_class) &&
          !IS_SET(epic_rewards[s].classes, pl->player.secondary_class))
      {
        send_to_char("Unfortunately, I am not able to teach people of your class.\n", pl);
        return TRUE;
      }

      if(epic_teachers[t].deny_skill && GET_CHAR_SKILL(pl, epic_teachers[t].deny_skill))
      {
        sprintf(buffer, "I cannot with good conscience teach that skill to someone who has already studied &+W%s&n!\n", skills[epic_teachers[s].deny_skill].name);
        send_to_char(buffer, pl);
        return TRUE;
      }

      if(epic_skillpoints(pl) < points_cost)
      {
        send_to_char("You don't have enough epic skill points!\n", pl);
        return TRUE;
      }

      if(epic_points(pl) < epic_rewards[s].min_points)
      {
        send_to_char("You haven't progressed far enough to be able to master such skills!\n", pl);
        return TRUE;
      }

      if(GET_MONEY(pl) < coins_cost)
      {
        send_to_char("You can't afford my teaching!", pl);
        return TRUE;
      }

      if(GET_CHAR_SKILL(pl, skill) >= 100 || GET_CHAR_SKILL(pl, skill) >= epic_teachers[t].max)
      {
        send_to_char("Unfortunately, I cannot teach you anything more, you have already mastered this skill!\n", pl);
        return TRUE;
      }

      sprintf(buffer, "$n takes you aside and teaches you the finer points of &+W%s&n.\n"
                      "&+cYou feel your skill in %s improving.&n\n",
              skills[skill].name, skills[skill].name);
      act(buffer, FALSE, ch, 0, pl, TO_VICT);

      SUB_MONEY(pl, coins_cost, 0);

      epic_gain_skillpoints(pl, -1 * points_cost);

      pl->only.pc->skills[skill].taught =
        pl->only.pc->skills[skill].learned =
        MIN(100, pl->only.pc->skills[skill].learned +
            get_property("epic.skillGain", 10));

      do_save_silent(pl, 1); // Epic stats require a save.
      CharWait(pl, PULSE_VIOLENCE);
      return TRUE;
    }

  }

  return FALSE;
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
      if(victim = get_random_char_in_room(ch->in_room, ch, 0)) {
        sprintf(buffer, "&+CThe snow melts from the heat of &+R%s &+Cand you are only splashed by &+bwater&n.",
            skills[faf->type].name);
        send_to_char(buffer, victim);
        sprintf(buffer, "&+CThe snow melts from the heat of &+R%s &+Cand $n is only splashed by &+bwater&n.",
            skills[faf->type].name);
        act(buffer, FALSE, victim, 0, 0, TO_ROOM);
        make_wet(victim, 2 * WAIT_MIN);
      }
    } else if(victim = get_random_char_in_room(ch->in_room, ch, DISALLOW_SELF | DISALLOW_GROUPED)) {
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
      sprintf(buffer, "  %s\n", familiars[i].name);
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
  if(!ch)
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

    if(GET_C_AGI(pl) < number(0,150) && !number(0,2)) {
      act("$n's unexpected attack caused you to get lost in your tracks..",
          FALSE, ch, 0, pl, TO_VICT);
      act("$n's vicious assault disturbed $N's move.", FALSE, ch, 0, pl, TO_NOTVICT);
      CharWait(pl, PULSE_VIOLENCE);
      return TRUE;
    } else {
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

void epic_initialization()
{
  for (int i = 0; epic_teachers[i].vnum; i++) {
    mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;
  }
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
      sprintf(buf,
        "You feel as if %s took over your body bringing death to your foes!\n",
        get_god_name(ch));
    else if(dev_power > 2)
      sprintf(buf,
        "%s fills you with holy power bringing death to your foes!\n",
        get_god_name(ch));
    else if(dev_power > 0)
      sprintf(buf,
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

int stat_shops(int room, P_char ch, int cmd, char *arg)
{
  char     buf[MAX_INPUT_LENGTH];
  int cost = 0;
  int cost_mod = 8;
  int MAX_SHOP_BUY = 95;

  /* check for periodic event calls */
  if(cmd == CMD_SET_PERIODIC)
    return FALSE;

  if(!ch)
    return (FALSE);

  if(cmd == CMD_LIST)
  {
     send_to_char("Available potions:\r\n\r\n", ch);

     cost = ch->base_stats.Str * ch->base_stats.Str * ch->base_stats.Str * cost_mod;
     if(ch->base_stats.Str < MAX_SHOP_BUY)
       sprintf(buf, "1. A &+Gmagical&n strength potion for %s\r\n", coin_stringv(cost));
     else
       sprintf(buf, "1. A &+Gmagical&n strength potion is not available for you.\r\n");
     send_to_char(buf, ch); 

     cost = ch->base_stats.Agi * ch->base_stats.Agi * ch->base_stats.Agi * cost_mod;
     if(ch->base_stats.Agi < MAX_SHOP_BUY)
       sprintf(buf, "2. A &+Gmagical&n agility potion for %s\r\n", coin_stringv(cost));
     else
       sprintf(buf, "2. A &+Gmagical&n agility potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Dex * ch->base_stats.Dex * ch->base_stats.Dex * cost_mod;
     if(ch->base_stats.Dex < MAX_SHOP_BUY)
       sprintf(buf, "3. A &+Gmagical&n dexterity potion for %s\r\n", coin_stringv(cost));
     else
       sprintf(buf, "3. A &+Gmagical&n dexterity potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Con * ch->base_stats.Con * ch->base_stats.Con * cost_mod;
     if(ch->base_stats.Con < MAX_SHOP_BUY)
       sprintf(buf, "4. A &+Gmagical&n constitution potion for %s\r\n", coin_stringv(cost));
     else
       sprintf(buf, "4. A &+Gmagical&n constitution potion is not available for you.\r\n");
     send_to_char(buf, ch); 

     cost = ch->base_stats.Luck * ch->base_stats.Luck * ch->base_stats.Luck * cost_mod;
     if(ch->base_stats.Luck < MAX_SHOP_BUY)
       sprintf(buf, "5. A &+Gmagical&n luck potion for %s\r\n", coin_stringv(cost));
     else
       sprintf(buf, "5. A &+Gmagical&n luck stat potion is not available for you.\r\n");
     send_to_char(buf, ch);

      cost = ch->base_stats.Pow   *ch->base_stats.Pow   *ch->base_stats.Pow  * cost_mod;
     if(ch->base_stats.Pow < MAX_SHOP_BUY)
       sprintf(buf, "6. A &+Gmagical&n power potion for %s\r\n", coin_stringv(cost));
     else
       sprintf(buf, "6. A &+Gmagical&n power stat potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Int * ch->base_stats.Int * ch->base_stats.Int * cost_mod;
     if(ch->base_stats.Int < MAX_SHOP_BUY)
       sprintf(buf, "7. A &+Gmagical&n intelligence potion for %s\r\n", coin_stringv(cost));
     else
       sprintf(buf, "7. A &+Gmagical&n intelligence stat potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Wis * ch->base_stats.Wis * ch->base_stats.Wis * cost_mod;
     if(ch->base_stats.Wis < MAX_SHOP_BUY)
       sprintf(buf, "8. A &+Gmagical&n wisdom potion for %s\r\n", coin_stringv(cost));
     else
       sprintf(buf, "8. A &+Gmagical&n wisdom stat potion is not available for you.\r\n");
     send_to_char(buf, ch);

     cost = ch->base_stats.Cha * ch->base_stats.Cha * ch->base_stats.Cha * cost_mod;
     if(ch->base_stats.Cha < MAX_SHOP_BUY)
       sprintf(buf, "9. A &+Gmagical&n charisma potion for %s\r\n", coin_stringv(cost));
     else
       sprintf(buf, "9. A &+Gmagical&n charisma stat potion  is not available for you.\r\n");
     send_to_char(buf, ch);

     return TRUE;
  }
  else if(cmd == CMD_BUY)
  {      
     arg = one_argument(arg, buf);
     if(!atoi(buf))
     {
        send_to_char("Exactly what are you trying to buy?\r\n", ch);
        return TRUE;
     }
     switch(atoi(buf))
     {
        case 1:
          cost = ch->base_stats.Str * ch->base_stats.Str * ch->base_stats.Str * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          } 
          if(ch->base_stats.Str >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }

          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_str(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 2:
          cost = ch->base_stats.Agi * ch->base_stats.Agi * ch->base_stats.Agi * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Agi >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_agi(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 3:
          cost = ch->base_stats.Dex * ch->base_stats.Dex * ch->base_stats.Dex * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Dex >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_dex(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 4:
          cost = ch->base_stats.Con * ch->base_stats.Con * ch->base_stats.Con * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Con >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_con(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 5:
          cost = ch->base_stats.Luck * ch->base_stats.Luck * ch->base_stats.Luck * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Luck >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_luck(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 6:
          cost = ch->base_stats.Pow * ch->base_stats.Pow * ch->base_stats.Pow * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Pow >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_pow(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 7:
          cost = ch->base_stats.Int * ch->base_stats.Int * ch->base_stats.Int * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Int >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_int(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 8:
          cost = ch->base_stats.Wis * ch->base_stats.Wis * ch->base_stats.Wis * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Wis >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_wis(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        case 9:
          cost = ch->base_stats.Cha * ch->base_stats.Cha * ch->base_stats.Cha * cost_mod;
          if(GET_MONEY(ch) < cost)
          {
             send_to_char("You dont have enough money!\r\n", ch);
             return TRUE;
          }
          if(ch->base_stats.Cha >= MAX_SHOP_BUY)
          {
             send_to_char("You cant buy that.\r\n", ch);
             return TRUE;
          }
          SUB_MONEY(ch, cost, 0);
          send_to_char("You quaffed a &+Gmagical&n potion.\r\n", ch);
          spell_perm_increase_cha(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          return TRUE;
          break;
        default:
          send_to_char("Exactly what are you trying to buy?\r\n", ch);
          return TRUE;
     }

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
     sprintf(buffer, "%s magic surrounds you as you begin your chant.&n",
                     chant_bonus == 0 ? "&+WSparkling&n" :
                     chant_bonus == 1 ? "&+WSparkling" : "&+WSp&+Cark&+Wli&+Cn&+Wg");
     act(buffer, FALSE, ch, 0, 0, TO_CHAR);
     sprintf(buffer, "%s magic surrounds $n &+Was $e begins $s chant.&n",
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

vector<string> get_epic_players(int racewar)
{
  vector<string> names;

#ifdef __NO_MYSQL__
  debug("get_epic_players(): __NO_MYSQL__, returning 0");
  return names;
#else
  if(!qry("SELECT name from players_core WHERE epics > 0 and racewar = '%d' and level < 57 order by epics desc limit %d", racewar, (int) get_property("epic.list.limit", 10)))
    return names;

  MYSQL_RES *res = mysql_store_result(DB);

  if(!res)
  {
    mysql_free_result(res);
    return names;
  }

  MYSQL_ROW row;
  while(row = mysql_fetch_row(res))
  {
    names.push_back(string(row[0]));
  }

  mysql_free_result(res);

  return names;
#endif
}

void do_epic(P_char ch, char *arg, int cmd)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];

  if(!ch || IS_NPC(ch))
    return;

  argument_interpreter(arg, buff2, buff3);

  if(!str_cmp("reset", buff2))
  {
    do_epic_reset(ch, arg, cmd);
    return;
  }
  
  if(!str_cmp("skills", buff2))
  {
    do_epic_skills(ch, arg, cmd);
    return;
  }

  if(!str_cmp("trophy", buff2))
  {
    do_epic_trophy(ch, arg, cmd);
    return;
  }

  if(!str_cmp("zones", buff2))
  {
    do_epic_zones(ch, arg, cmd);
    return;
  }

  if( !str_cmp("bonus", buff2) )
  {
    do_epic_bonus(ch, arg, cmd);
    return;
  }

  if( !str_cmp("share", buff2) )
  {
    do_epic_share(ch, arg, cmd);
    return;
  }

  // else show list of epic players
  vector<string> top_good_players = get_epic_players(RACEWAR_GOOD);
  vector<string> top_evil_players = get_epic_players(RACEWAR_EVIL);

  // list
  send_to_char("&+GEpic Players\n\n", ch);
  send_to_char(" &+WGoods\n\n", ch);

  for(int i = 0; i < top_good_players.size(); i++)
  {
    send_to_char("   ", ch);
    send_to_char(top_good_players[i].c_str(), ch);
    send_to_char("\n", ch);
  }

  send_to_char("\n\n &+LEvils\n\n", ch);

  for(int i = 0; i < top_evil_players.size(); i++)
  {
    send_to_char("   ", ch);
    send_to_char(top_evil_players[i].c_str(), ch);
    send_to_char("\n", ch);
  }
}

void epic_zone_erase_touch(int zone_number)
{
   for(vector<epic_zone_completion>::iterator it = epic_zone_completions.begin();
             it != epic_zone_completions.end();
             it++)
   {
      if((it->number == zone_number))
      {
         epic_zone_completions.erase(it);
         break;
      }
   }
}

bool epic_zone_done_now(int zone_number)
{
   for(vector<epic_zone_completion>::iterator it = epic_zone_completions.begin();
             it != epic_zone_completions.end();
             it++)
   {
      if((it->number == zone_number)) return true;
   }
   return false;
}

bool epic_zone_done(int zone_number)
{
   for(vector<epic_zone_completion>::iterator it = epic_zone_completions.begin();
             it != epic_zone_completions.end();
             it++)
   {
      if((it->number == zone_number) && 
         (time(NULL) - it->done_at) > (int) get_property("epic.showCompleted.delaySecs", (15*60))) 
      {
         return true;
      }
   }
   return false;
}

int epic_zone_data::displayed_alignment() const 
{
   int delta = 0;
   for(vector<epic_zone_completion>::iterator it = epic_zone_completions.begin();
            it != epic_zone_completions.end();
            it++)
   {
      if((it->number == this->number) && 
         (time(NULL) - it->done_at) < (int) get_property("epic.showCompleted.delaySecs", (15*60))) 
      {
         return this->alignment - it->delta;
      }
   }
   return this->alignment;
}


//void do_epic_zones(P_char ch, char *arg, int cmd)
//{
//  return;
//  
//  if(!ch || IS_NPC(ch))
//    return;
//
//#ifdef __NO_MYSQL__
//  send_to_char("This feature is disabled.\r\n", ch);
//  return;
//#endif
//
//  char buff[MAX_STRING_LENGTH];
//	char done_str[] = " ";
//
//  vector<epic_zone_data> epic_zones = get_epic_zones();
//
//  send_to_char("&+WEpic Zones &+G-----------------------------------------\n\n", ch);
//
//  const char *freq_strs[] = {
//    "&+W(&+Roverdone&+W)",
//    "&+W(&+yvery common&+W)",
//    "&+W(&ncommon&+W)",
//    "",
//    "&+W(&+cuncommon&+W)",
//    "&+W(&+brare&+W)",
//    "&+W(&+BVERY RARE&+W)"
//  };
//
//  for(int i = 0; i < epic_zones.size(); i++)
//  {
//    float freq = epic_zones[i].freq;
//
//		if(epic_zone_done(epic_zones[i].number)) done_str[0] = '*';
//		else done_str[0] = ' ';
//
//    int freq_str = 3;
//
//    if(freq < 0.50)
//      freq_str = 0;
//    else if(freq < 0.50)
//      freq_str = 1;
//    else if(freq < 0.80)
//      freq_str = 2;
//    else if(freq > 1.90)
//      freq_str = 6;
//    else if(freq > 1.50)
//      freq_str = 5;
//    else if(freq > 1.20)
//      freq_str = 4;
//    else
//      freq_str = 3;
//
//    sprintf(buff, "  %s%s %s\r\n", done_str, epic_zones[i].name.c_str(), freq_strs[freq_str]);
//    send_to_char(buff, ch);
//  }
//
//	send_to_char("\n* = already completed this boot.\n", ch);
//
//}

void do_epic_reset(P_char ch, char *arg, int cmd)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];
  
  argument_interpreter(arg, buff2, buff3);
  
  if(!ch || !IS_PC(ch))
    return;
  
  P_char t_ch = ch;
  
  if(IS_TRUSTED(ch) && strlen(buff3))
  {
    if(!(t_ch = get_char_vis(ch, buff3)) || !IS_PC(t_ch))
    {
      send_to_char("They don't appear to be in the game.\n", ch);
      return;
    }
  }
  
  // run through skills
  // for each skill that is epic:
  //    for each skill point:
  //      calculate epic pointcost / plat cost
  //      reimburse points / plat
  
  send_to_char("&+WResetting epic skills:\n", ch);
  
  int point_refund = 0;
  int coins_refund = 0;
  
  for (int skill_id = 0; skill_id <= MAX_AFFECT_TYPES; skill_id++)
  {
    int learned = t_ch->only.pc->skills[skill_id].learned;
    
    if(IS_EPIC_SKILL(skill_id) && learned)
    {
      // find in epic_rewards
      int s;
      
      bool found = false;
      for(s = 0; epic_rewards[s].type; s++)
      {
        if(epic_rewards[s].value == skill_id)
        {
          found = true;
          break;
        }
      }
      
      if(!found)
      {
        continue;
      }
      
      int points = 0;
      int coins = 0;
      
      for(int skill_level = 0; skill_level < learned; skill_level += (int) get_property("epic.skillGain", 10))
      {
        float cost_f = 1 + skill_level / get_property("epic.progressFactor", 30);
        int points_cost = (int) (cost_f * epic_rewards[s].points_cost);
        int coins_cost = (int) (cost_f * epic_rewards[s].coins);
        
        if(IS_MULTICLASS_PC(t_ch) &&
           !IS_SET(epic_rewards[s].classes, t_ch->player.m_class) &&
           IS_SET(epic_rewards[s].classes, t_ch->player.secondary_class))
        {
          points_cost *= (int) (get_property("epic.multiclass.EpicSkillCost", 2));
          coins_cost *= (int) (get_property("epic.multiclass.EpicPlatCost", 3));
        }
        
        points += points_cost;
        coins += coins_cost;
      }      
      
      sprintf(buff2, "&+W%s %d&n: &+W%d&n esp, %s\n", skills[skill_id].name, learned, points, coin_stringv(coins));
      send_to_char(buff2, ch);
      
      point_refund += points;
      coins_refund += coins;
      
      t_ch->only.pc->skills[skill_id].learned = t_ch->only.pc->skills[skill_id].taught = 0;
    }
  }
  
  sprintf(buff2, "Total: &+W%d&n esp, %s&n refunded\r\n", point_refund, coin_stringv(coins_refund));
  send_to_char(buff2, ch);
  
  insert_money_pickup(GET_PID(t_ch), coins_refund);
  t_ch->only.pc->epic_skill_points += point_refund;

  sprintf(buff2, "\r\n&+GYour epic skills have been reset: your skill points have been refunded, \r\n&+Gand %s&+G has been reimbursed and is waiting for you at the nearest auction house.\r\n\r\n", coin_stringv(coins_refund));
  
  if(!send_to_pid(buff2, GET_PID(t_ch)))
    send_to_pid_offline(buff2, GET_PID(t_ch));
  
  do_save_silent(t_ch, 1);  
}

void do_epic_zones(P_char ch, char *arg, int cmd)
{
  if(!ch || IS_NPC(ch))
    return;
  
#ifdef __NO_MYSQL__
  send_to_char("This feature is disabled.\r\n", ch);
  return;
#endif
  
  char buff[MAX_STRING_LENGTH];
	char done_str[] = " ";
  
  vector<epic_zone_data> epic_zones = get_epic_zones();
  
  send_to_char("&+WEpic Zones &+G-----------------------------------------\n\n", ch);
  
  // this array depends on the alignment max/min being +/-5
  const char *alignment_strs[] = {
    "&n(&+Lpure evil&n)",
    "&n(&+Lextremely evil&n)",
    "&n(&+Lvery evil&n)",
    "&n(&+Levil&n)",
    "&n(&+Lslightly evil&n)",
    "",
    "&n(&+Wslightly good&n)",
    "&n(&+Wgood&n)",
    "&n(&+Wvery good&n)",
    "&n(&+Wextremely good&n)",
    "&n(&+Wpure good&n)"
  };
  
  for(int i = 0; i < epic_zones.size(); i++)
  {    
		if(epic_zone_done(epic_zones[i].number)) done_str[0] = '*';
		else done_str[0] = ' ';
    
    int alignment_str = BOUNDED(0, EPIC_ZONE_ALIGNMENT_MAX + epic_zones[i].displayed_alignment(), 10);
    
    sprintf(buff, "  %s%s %s\r\n", done_str, pad_ansi(epic_zones[i].name.c_str(), 45).c_str(), alignment_strs[alignment_str]);
    send_to_char(buff, ch);
  }
  
	send_to_char("\n* = already completed this boot.\n", ch);  
}

void do_epic_share(P_char ch, char *arg, int cmd)
{

  struct affected_type *afp, *tafp;

  if(!has_epic_task(ch))
  {
    send_to_char("You don't have an epic task to share.\r\n", ch);
    return;
  }
  else
  {
    afp = get_epic_task(ch);
  }

  if( ch->group )
  {
    for( struct group_list *gl = ch->group; gl; gl = gl->next )
    {
      if( gl->ch == ch ) continue;
      if( gl->ch->in_room == ch->in_room )
      {
	if(is_linked_to(ch, gl->ch, LNK_CONSENT) && IS_PC(gl->ch))
	{
	  if(has_epic_task(gl->ch))
	  {
	    tafp = get_epic_task(gl->ch);
	    // Don't let nexus stones or pvp get replaced
	    if(tafp->modifier < 0)
	      continue;
	    tafp->type = afp->type;
	    tafp->flags = afp->flags;
	    tafp->duration = afp->duration;
	    tafp->modifier = afp->modifier;
	    act("&+C$n has just shared his epic task with you!&n", TRUE, ch, 0, gl->ch, TO_VICT);
	    act("&+CYou have just shared your epic task with $N.&n", TRUE, ch, 0, gl->ch, TO_CHAR);
	  }
	}
      }
    }
  }

}

void do_epic_trophy(P_char ch, char *arg, int cmd)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];

  argument_interpreter(arg, buff2, buff3);

  if(!ch || IS_NPC(ch))
    return;

  P_char t_ch = ch;

  if(IS_TRUSTED(ch) && strlen(buff3))
  {
    if(!(t_ch = get_char_vis(ch, buff3)))
    {
      send_to_char("They don't appear to be in the game.\n", ch);
      return;
    }
  }

  vector<epic_trophy_data> trophy = get_epic_zone_trophy(t_ch);

  send_to_char("&+WEpic Trophy\n", ch);

  for(int i = 0; i < trophy.size(); i++)
  {
    if(trophy[i].zone_number >= 0 && real_zone0(trophy[i].zone_number))
    {
      sprintf(buff2, "[&+W%3d&n] %s\n", trophy[i].count, zone_table[real_zone0(trophy[i].zone_number)].name);
      send_to_char(buff2, ch);
    }

  }

}

void update_epic_zone_alignment(int zone_number, int delta)
{
#ifdef __NO_MYSQL__
  return;
#else
  // add alignment
  qry("UPDATE zones SET alignment = alignment + (%d) WHERE number = %d AND epic_type > 0", delta, zone_number);

  // if alignment delta resulted in 0, add one more so that it doesn't stay on 0
  qry("UPDATE zones SET alignment = alignment + (%d) WHERE number = %d AND epic_type > 0 and alignment = 0", delta, zone_number);

  // min/max bounds on alignment
  qry("UPDATE zones SET alignment = %d WHERE alignment > %d", EPIC_ZONE_ALIGNMENT_MAX, EPIC_ZONE_ALIGNMENT_MAX);
  qry("UPDATE zones SET alignment = %d WHERE alignment < %d", EPIC_ZONE_ALIGNMENT_MIN, EPIC_ZONE_ALIGNMENT_MIN);
  
  //debug("update_epic_zone_alignment(zone_number=%d, delta=%d)", zone_number, delta);
#endif  
}

float get_epic_zone_alignment_mod(int zone_number, ubyte racewar)
{
#ifdef __NO_MYSQL__
  return 1.0;
#else
  
  float mod = 1.0;
  int alignment = 0;
  
  if(!qry("SELECT alignment FROM zones WHERE number = %d", zone_number))
    return mod;
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if(mysql_num_rows(res) < 1)
    return mod;
  
  MYSQL_ROW row = mysql_fetch_row(res);
  
  if(row)
    alignment = atoi(row[0]);
  
  mysql_free_result(res);

  if((alignment < 0 && racewar == RACEWAR_GOOD) || (alignment > 0 && racewar == RACEWAR_EVIL))
  {
    // good alignment, evil racewar or evil alignment, good racewar
    mod += ((float) abs(alignment)) * 0.3 * (float) get_property("epic.zone.alignmentMod", 0.10);
  }
  else if((alignment > 0 && racewar == RACEWAR_GOOD) || (alignment < 0 && racewar == RACEWAR_EVIL))
  {
    // good alignment, good racewar or evil alignment, evil racewar
    mod -= ((float) abs(alignment)) * (float) get_property("epic.zone.alignmentMod", 0.10);    
  }
  
  debug("get_epic_zone_alignment_mod(zone_number=%d, racewar=%d): %f", zone_number, (int) racewar, mod);
  
  return mod;
#endif
}

// called from timers.c
void update_epic_zone_mods()
{
#ifdef __NO_MYSQL__
  return;
#else
  int wait_secs = (int) get_property("epic.freqMod.tick.waitSecs", 3600);

  if(!has_elapsed("epic_zone_mod", wait_secs))
    return;

  float add = (float) get_property("epic.freqMod.tick.add", 0.002);
  float mod_max = (float) get_property("epic.freqMod.max", 2.00);
  float mod_min = (float) get_property("epic.freqMod.min", 0.40);

  qry("UPDATE zones SET frequency_mod = frequency_mod + (%f) WHERE epic_type > 0", add);
  qry("UPDATE zones SET frequency_mod = %f WHERE frequency_mod > %f", mod_max, mod_max);
  qry("UPDATE zones SET frequency_mod = %f WHERE frequency_mod < %f", mod_min, mod_min);

  set_timer("epic_zone_mod");
#endif
}

void update_epic_zone_frequency(int zone_number)
{
#ifdef __NO_MYSQL__
  return;
#else
  float sub = (float) get_property("epic.freqMod.touch.sub", 0.10);
  float mod_min = (float) get_property("epic.freqMod.min", 0.40);

  qry("UPDATE zones SET frequency_mod = frequency_mod - (%f * zone_freq_mod) WHERE number = %d AND epic_type > 0", sub, zone_number);
  qry("UPDATE zones SET frequency_mod = %f WHERE frequency_mod < %f", mod_min, mod_min);

  debug("update_epic_zone_frequency(zone_number=%d): -%f", zone_number, sub);
#endif
}

float get_epic_zone_frequency_mod(int zone_number)
{
#ifdef __NO_MYSQL__
  return 1.0;
#else

  float mod = 1.0;

  if(!qry("SELECT frequency_mod FROM zones WHERE number = %d", zone_number))
    return mod;

  MYSQL_RES *res = mysql_store_result(DB);

  if(mysql_num_rows(res) < 1)
    return mod;

  MYSQL_ROW row = mysql_fetch_row(res);

  if(row)
    mod = atof(row[0]);

  mysql_free_result(res);

  return mod;
#endif
}

vector<epic_zone_data> get_epic_zones()
{
  vector<epic_zone_data> zones;

#ifdef __NO_MYSQL__
  return zones;
#else

  if(!qry("SELECT number, name, frequency_mod, alignment FROM zones WHERE epic_type > 0 ORDER BY (suggested_group_size*epic_payout), id"))
  {
    return zones;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  MYSQL_ROW row;

  while(row = mysql_fetch_row(res))
  {
    zones.push_back(epic_zone_data(atoi(row[0]), string(row[1]), atof(row[2]), atoi(row[3])));
  }

  mysql_free_result(res);

  return zones;
#endif
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

void do_epic_skills(P_char ch, char *arg, int cmd)
{
  char buff[MAX_STRING_LENGTH];
  P_char teacher;

  if(IS_TRUSTED(ch))
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

    if(epic_teachers[t].pre_requisite && GET_CHAR_SKILL(ch, epic_teachers[t].pre_requisite) < 100)
      continue;

    if(IS_TRUSTED(ch))
    {
      if(teacher = read_mobile(epic_teachers[t].vnum, VIRTUAL))
      {
        sprintf(buff, "&+W%-25s &n(&+W%-5d&n) %s\n", skills[skill].name, epic_teachers[t].vnum, teacher->player.short_descr);
        extract_char(teacher);
      } else {
        logit(LOG_DEBUG, "do_epic_skills(): epic_teachers[%d].vnum does not exist for epic skill %s", t, skills[skill].name);
        sprintf(buff, "&+W%-25s &n(&+W%-5d&n) Teacher does not exist.\n", skills[skill].name, epic_teachers[t].vnum);
      }
    } else {
      sprintf(buff, "&+W%s\n", skills[skill].name);
    }
    send_to_char(buff, ch);
  }

  send_to_char("\n", ch);
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
    sprintf(msg, "&+WYou infuse the magic from %s &+Winto %s&+W.&n\r\n",
            stone->short_description, device->short_description);
    send_to_char(msg, ch);
    obj_from_char(stone, TRUE);
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
    sprintf(msg, "&+wYou infuse %s &+wwith a charge!&n\r\n", device->short_description);
    send_to_char(msg, ch);
    device->value[2]++;

    if(device->value[2] == device->value[1])
    {
      sprintf(msg, "&+W%s &+Whas been fully infused!\r\n", device->short_description);
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
