#ifndef __NECROMANCY_H__
#define __NECROMANCY_H__

#include "config.h"
#include "structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

#define NECROPET      1201

#define NECROPET_START      0
#define NECROPET_SKELETON   0
#define NECROPET_ZOMBIE     1
#define NECROPET_SPECTRE    2
#define NECROPET_WRAITH     3
#define NECROPET_VAMPIRE    4
#define NECROPET_LICH       5
#define NECROPET_SHADOW     6
#define NECROPET_END        6

#define THEURPET_START      7
#define THEURPET_SKELETON   7
#define THEURPET_ZOMBIE     8
#define THEURPET_SPECTRE    9
#define THEURPET_WRAITH     10
#define THEURPET_VAMPIRE    11
#define THEURPET_LICH       12
#define THEURPET_SHADOW     13
#define THEURPET_END        13

#define NECROPET_LAST       13

#define NECROGOLEM_FLESH 0
#define NECROGOLEM_BLOOD 1
#define NECROGOLEM_BONE  2
#define THEURGOLEM_VALOR 3
#define NECROGOLEM_LAST  3

#define CORPSEFORM_INNATE 0
#define CORPSEFORM_REG    1

/* defines for corpse objects values[] */
#define CORPSE_WEIGHT 0
#define CORPSE_FLAGS 1
#define CORPSE_LEVEL 2
#define CORPSE_VNUM 3
#define CORPSE_PID 3
#define CORPSE_EXP_LOSS 4
#define CORPSE_RACEWAR 5
#define CORPSE_SAVEID 6
#define CORPSE_RACE 7

struct undead_description
{
  char    *name;
  char    *short_desc;
  int      corpse_level;
  int      act;
  int      aff1;
  int      aff2;
  float    hps;
  int      max_level;
  int      cost;
  uint     pet_class;
  int      race;
};

struct golem_description
{
  char    *name;
  int      vnum;
  int      corpse_lvl;
  float    hps;
  int      max_level;
  int      cost;
};

void spell_corpseform(int, P_char, char *, int, P_char, P_obj);
void event_corpseform_wearoff(P_char, P_char, P_obj, void *);
void check_saved_corpse(P_char);
void event_saved_corpse(P_char ch, P_char vict, P_obj obj, void *data);
void create_saved_corpse(P_obj obj, P_char mob);
void spell_compact_corpse(int, P_char, char *, int, P_char, P_obj);

#endif // __NECROMANCY_H__

