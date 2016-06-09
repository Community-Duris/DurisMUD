/****************************************************************************
 *
 *  File: randomeq.c                                           Part of Duris
 *  Usage: randomboject.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark 			Date: 2002-04-18
 * ***************************************************************************
 */

#define TROPHY

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "mm.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "weather.h"
#include "sound.h"
#include "objmisc.h"
#include "map.h"

/*
 * external variables
 */
extern struct zone_data *zone_table;
extern const struct race_names race_names_table[];
extern const struct class_names class_names_table[];
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern char debug_mode;
extern const char *class_names[];
extern const char *race_types[];
extern const int top_of_world;

//extern const int material_absorbtion[][];
extern const struct stat_data stat_factor[];
extern float fake_sqrt_table[];
extern int pulse;
extern int arena_hometown_location[];
extern struct arena_data arena;
extern struct agi_app_type agi_app[];
extern struct dex_app_type dex_app[];
extern struct message_list fight_messages[];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern const int class_table[LAST_RACE + 1][CLASS_COUNT + 1];

int      find_a_zone();
int      get_name(char return_name[256]);
int      find_mob_map_room(P_char random_mob);

const char *prefix_name_theme[8][2] = {
  {
   "&+Wan un&+Ldead&n",
   "&+La sk&+Wel&+Leton&+w&n"   //0
   },
  {
   "&+Wa&+c tr&+Wans&+cpar&+Cent&n",    //1
   "&+Wa &+Lfl&+mow&+Mi&+Lng&n"},
  {
   "&n&+ga put&+Lrid &n&+gzo&+Lmbif&n&+gied&n", //2
   "&n&+ya mum&+Ymifi&+Wed&n"},
  {
   "&n&+wa tu&+Lnnel wor&n&+wking&n",   //3
   "&n&+wa &+Wwat&n&+wch&+Lful&n"},
  {
   "&na &n&+Wrighte&n&+wous&n", //4 GOODIE RELIC
   "&na &n&+Wcourage&n&+wous&n"},
  {
   "&na &n&+mmalev&n&+wole&+Lnt&n",     //5 eVILS
   "&na &n&+Ldevi&n&+mant&n"},
  {
   "&na &nr&n&+ravaging&n",     //6 UNDEAD
   "&na &n&+Rblo&n&+rodthi&n&+wrsty&n",
   },
  {
   "&+La &+YRELIC &+Lguarding&n",       //7
   "&+Ya &+YRELIC &+Yguarding&n",
   }



};

const char *prefix_name[37] = {
  "&+wAn &+rangry&+w&n",
  "&+LA &+btraveling&n",
  "&N&+wA &+Ldaft &N&+wlooking&n",
  "&+LA &+Lstrange looking&N",
  "&+LA &N&+bmysterious &+Llooking&N",
  "&+LA hulking&N",
  "&+LA &N&+wforeboding&N",
  "&+LA &N&+cmassive&N",
  "&+LA &+Cmuscular&N",
  "&+LA &N&+wsomber &+Llooking&N",
  "&+LA battle &N&+rscarred&N",
  "&N&+gA wandering&N",
  "&+LA &N&+gs&+Gi&N&+gc&+Gk&N&+gl&+Gy&N",
  "&+LA &N&+ypeculiar&N",
  "&+LA &+Rm&N&+re&+Rn&N&+ra&+Rc&N&+ri&+Rn&N&+rg&N",
  "&+LA &N&+bsin&+Lister&N",
  "&N&+wA &+Wlonely&N",
  "&+LA &N&+ydirty&N",
  "&N&+wA &+Lshifty &N&+wlooking&N",
  "&+LA &N&+bso&+Lm&N&+bb&+Ler looking&N",
  "&+LA &N&+bweird &+Llooking&N",
  "&+LAn &+Mugly&N",
  "&+LA &N&+wforlorn&N",
  "&+LA diabolical&N",
  "&+LA &+Rdeformed&N",
  "&+LA &N&+mhideous &+Llooking&N",
  "&+LA &N&+yscrawny&N",
  "&+LA &+Cskinny&N",
  "&+LA &+Wone &+Leyed&N",
  "&+LA ghastly&N",
  "&+LA &+Rhungry&N",
  "&+LA &N&+clistless&N",
  "&+LA &+Rdisgusting &+Llooking&N",
  "&+LAn &N&+mabnormal &+Llooking&N",
  "&+LA &N&+rdisgruntled&N",
  "&+LA &+Bmorbid&N",
  "&+LA desolate&N"
};

#define MAX_ZONES_RANDOM_DATA 100
struct zone_random_data {
  int zone;
  int races[10];
  int proc_spells[3][2];
} zones_random_data[MAX_ZONES_RANDOM_DATA] = {
// Please keep this in numerical order by zone #.  It's much easier to search that way.
// Zone 1 not found.. If this is supposed to be Heaven, Heaven is zone 0 and that's a no-go.
//  {1, {RACE_ANGEL, RACE_DEVIL, 0}, {{6, SPELL_STONE_SKIN}}},
  {14, {RACE_GOBLIN, RACE_HUMANOID, RACE_HALFORC, RACE_ORC, RACE_LYCANTH, 0}, {{6, SPELL_STONE_SKIN}}},
  {18, {RACE_TROLL, RACE_OGRE, RACE_MINOTAUR, 0}, {{6, SPELL_STONE_SKIN}}},
  {20, {RACE_FAERIE, RACE_HUMAN, RACE_GREY, 0}, {{6, SPELL_STONE_SKIN}}},
  {21, {RACE_GOBLIN, RACE_LYCANTH, RACE_HALFORC, RACE_ORC, RACE_GIANT, RACE_TROLL, 0 }, {{6, SPELL_STONE_SKIN}}},
  {23, {RACE_PARASITE, RACE_DUERGAR, RACE_GOBLIN, RACE_HERBIVORE, 0}, {{6, SPELL_STONE_SKIN}}},
  {26, {RACE_HUMAN, RACE_GREY, RACE_HALFELF, RACE_CENTAUR, 0}, {{6, SPELL_STONE_SKIN}}},
  {37, {RACE_HUMAN, RACE_HUMANOID, RACE_HALFELF, RACE_GNOME, 0}, {{6, SPELL_STONE_SKIN}}},
  {40, {RACE_ORC, RACE_HALFORC, RACE_GOBLIN, RACE_REPTILE, 0}, {{6, SPELL_STONE_SKIN}}},
  {50, {RACE_GOLEM, RACE_WIGHT, RACE_REVENANT, RACE_DUERGAR, RACE_PHANTOM, RACE_TROLL, 0}, {{6, SPELL_STONE_SKIN}}},
  {53, {RACE_CENTAUR, RACE_GREY, 0}, {{6, SPELL_STONE_SKIN}}},
  {60, {RACE_HUMAN, RACE_HALFLING, RACE_BARBARIAN, RACE_HALFELF, RACE_MOUNTAIN, RACE_GNOME, 0}, {{5, SPELL_BLESS}}},
  {68, {RACE_DRAGON, RACE_DRAGONKIN, 0}, {{6, SPELL_STONE_SKIN}}},
  {73, {RACE_MINOTAUR, RACE_DROW, RACE_THRIKREEN, RACE_GITHYANKI, RACE_GITHZERAI, RACE_BEHOLDER, 0}, {{6, SPELL_STONE_SKIN}}},
// Zone 80 not found.
//  {80, {RACE_GREY, RACE_CENTAUR, RACE_HALFELF, 0}, {{6, SPELL_STONE_SKIN}}},
  {91, {RACE_DEVIL, RACE_ANGEL, RACE_RAKSHASA, RACE_SGIANT, RACE_DEMON, RACE_SHADE, RACE_ILLITHID,0 }, {{6, SPELL_STONE_SKIN}}},
  {93, {RACE_FAERIE, RACE_DROW, RACE_GREY, RACE_HARPY, RACE_RAKSHASA, RACE_GITHYANKI, RACE_GITHZERAI, 0}, {{6, SPELL_STONE_SKIN}}},
  {94, {RACE_ORC, RACE_HALFORC, RACE_GIANT, RACE_HUMAN, RACE_MINOTAUR, 0}, {{6, SPELL_STONE_SKIN}}},
  {103, {RACE_ORC, RACE_HALFORC, 0}, {{6, SPELL_STONE_SKIN}}},
  {106, {RACE_ORC, RACE_HALFORC, RACE_GOBLIN, RACE_MINOTAUR, RACE_DUERGAR, 0}, {{6, SPELL_STONE_SKIN}}},
  {112, {RACE_ILLITHID, RACE_GITHYANKI, 0}, {{6, SPELL_STONE_SKIN}}},
  {113, {RACE_TROLL, RACE_OGRE, RACE_GIANT, RACE_RAKSHASA, RACE_HUMANOID, RACE_LYCANTH, 0}, {{6, SPELL_STONE_SKIN}}},
  {130, {RACE_UNDEAD, RACE_GHOST, RACE_OGRE, RACE_DROW, RACE_VAMPIRE, RACE_LICH, 0}, {{6, SPELL_STONE_SKIN}}},
  {140, {RACE_FAERIE, RACE_F_ELEMENTAL, RACE_A_ELEMENTAL, RACE_W_ELEMENTAL, RACE_E_ELEMENTAL, 0}, {{6, SPELL_STONE_SKIN}}},
  {143, {RACE_UNDEAD, RACE_GHOST, RACE_VAMPIRE, RACE_GARGOYLE, RACE_PHANTOM, RACE_REVENANT, RACE_SHADE, 0}, {{6, SPELL_STONE_SKIN}}},
  {151, {RACE_DRAGON, RACE_DRAGONKIN, RACE_UNDEAD, RACE_DRACOLICH, RACE_GHOST, RACE_WIGHT, RACE_REVENANT, 0}, {{6, SPELL_STONE_SKIN}}},
  {152, {RACE_OGRE, RACE_TROLL, RACE_ORC, 0}, {{6, SPELL_STONE_SKIN}}},
  {159, {RACE_DRAGONKIN, RACE_DRAGON, 0}, {{6, SPELL_STONE_SKIN}}},
  {160, {RACE_HUMAN, RACE_HALFLING, 0}, {{6, SPELL_STONE_SKIN}}},
  {162, {RACE_DEVIL, RACE_FAERIE, RACE_ORC, RACE_GIANT, RACE_ILLITHID, 0}, {{6, SPELL_STONE_SKIN}}},
  // Woodseer
  {165, {RACE_HALFLING, 0}, {{4, SPELL_BARKSKIN}, {8, SPELL_BLESS}}},
  // Khildarak
  {170, {RACE_DUERGAR, 0}, {{4, SPELL_ARMOR}, {8, SPELL_STRENGTH}}},
  {196, {RACE_DRAGON, RACE_BEHOLDER, RACE_DRAGONKIN, 0}, {{6, SPELL_STONE_SKIN}}},
  {197, {RACE_GITHYANKI, RACE_GITHZERAI, RACE_ILLITHID, RACE_DEMON, RACE_BEHOLDER, 0}, {{6, SPELL_STONE_SKIN}}},
  {200, {RACE_ORC, RACE_HALFORC, RACE_DRAGONKIN, 0}, {{6, SPELL_STONE_SKIN}}},
  {202, {RACE_HUMAN, 0}, {{6, SPELL_STONE_SKIN}}},
  {213, {RACE_DROW, 0}, {{6, SPELL_STONE_SKIN}}},
  {215, {RACE_HUMAN, RACE_GREY, RACE_HALFELF, RACE_HUMANOID, 0}, {{6, SPELL_STONE_SKIN}}},
  {220, {RACE_BARBARIAN, RACE_ANIMAL, 0}, {{6, SPELL_STONE_SKIN}}},
  {222, {RACE_HUMAN, RACE_HALFORC, RACE_ORC, RACE_GREY, RACE_HALFELF, RACE_HALFLING, 0}, {{6, SPELL_STONE_SKIN}}},
  {224, {RACE_ORC, RACE_HALFORC, 0}, {{6, SPELL_STONE_SKIN}}},
  {226, {RACE_ORC, RACE_HALFORC, 0}, {{6, SPELL_STONE_SKIN}}},
  {232, {RACE_W_ELEMENTAL, 0}, {{6, SPELL_STONE_SKIN}}},
  {238, {RACE_E_ELEMENTAL, 0}, {{6, SPELL_STONE_SKIN}}},
  {244, {RACE_A_ELEMENTAL, 0}, {{6, SPELL_STONE_SKIN}}},
  {250, {RACE_F_ELEMENTAL, 0}, {{6, SPELL_STONE_SKIN}}},
  {257, {RACE_DRAGON, RACE_DRAGONKIN, 0}, {{6, SPELL_STONE_SKIN}}},
  {260, {RACE_TROLL, RACE_OGRE, 0}, {{6, SPELL_STONE_SKIN}}},
  {262, {RACE_HUMAN, RACE_HALFELF, RACE_GREY, RACE_MOUNTAIN, RACE_GNOME, 0}, {{6, SPELL_STONE_SKIN}}},
  {266, {RACE_DEMON, RACE_GITHYANKI, RACE_DRAGONKIN, RACE_BEHOLDER, 0}, {{6, SPELL_STONE_SKIN}}},
  {270, {RACE_HARPY, RACE_GIANT, RACE_MOUNTAIN, RACE_TROLL, 0}, {{6, SPELL_STONE_SKIN}}},
  {289, {RACE_ORC, RACE_HALFORC, RACE_GIANT, RACE_GOLEM, RACE_DEMON, RACE_MOUNTAIN, RACE_DUERGAR, 0}, {{6, SPELL_STONE_SKIN}}},
  // Gnome Village - Harrow
  {294, {RACE_GNOME, 0}, {{4, SPELL_ARMOR}}},
  {306, {RACE_ANIMAL, RACE_HERBIVORE, RACE_HALFLING, RACE_GREY, 0}, {{6, SPELL_STONE_SKIN}}},
  {308, {RACE_SGIANT, RACE_GIANT, 0}, {{6, SPELL_STONE_SKIN}}},
  {311, {RACE_HARPY, RACE_GARGOYLE, 0}, {{6, SPELL_STONE_SKIN}}},
  {313, {RACE_FAERIE, RACE_ANGEL, RACE_DEVIL, RACE_A_ELEMENTAL, 0}, {{6, SPELL_STONE_SKIN}}},
  {315, {RACE_W_ELEMENTAL, RACE_FAERIE, 0}, {{6, SPELL_STONE_SKIN}}},
  {328, {RACE_ILLITHID, RACE_GITHYANKI, RACE_GIANT, RACE_DEMON, 0}, {{6, SPELL_STONE_SKIN}}},
  {333, {RACE_PSBEAST, RACE_SHADE, RACE_UNDEAD, RACE_LYCANTH, RACE_GHOST, 0}, {{6, SPELL_STONE_SKIN}}},
  {335, {RACE_VAMPIRE, RACE_UNDEAD, RACE_GHOST, RACE_REVENANT, RACE_PDKNIGHT, RACE_LICH, 0}, {{6, SPELL_STONE_SKIN}}},
  {342, {RACE_DUERGAR, RACE_GIANT, RACE_ARACHNID, 0}, {{6, SPELL_STONE_SKIN}}},
  {360, {RACE_DEMON, RACE_DROW, RACE_DUERGAR, RACE_GITHYANKI, RACE_PDKNIGHT, 0}, {{6, SPELL_STONE_SKIN}}},
  {368, {RACE_VAMPIRE, RACE_UNDEAD, RACE_DROW, RACE_DUERGAR, RACE_WIGHT, RACE_REVENANT, RACE_DRACOLICH, 0}, {{6, SPELL_STONE_SKIN}}},
  {371, {RACE_HUMAN, RACE_BARBARIAN, RACE_HALFELF, 0}, {{6, SPELL_STONE_SKIN}}},
  {377, {RACE_MINOTAUR, RACE_THRIKREEN, 0}, {{6, SPELL_STONE_SKIN}}},
  {391, {RACE_BARBARIAN, RACE_HUMAN, 0}, {{6, SPELL_STONE_SKIN}}},
  {402, {RACE_HUMAN, RACE_HUMANOID, RACE_GOBLIN, RACE_HALFELF, RACE_HALFORC, 0}, {{6, SPELL_STONE_SKIN}}},
  {419, {RACE_BARBARIAN, RACE_ANIMAL, RACE_ARACHNID, 0}, {{6, SPELL_STONE_SKIN}}},
  {441, {RACE_GITHZERAI, 0}, {{6, SPELL_STONE_SKIN}}},
  {448, {RACE_ANGEL, RACE_DEVIL, RACE_DEMON, RACE_SGIANT, 0}, {{6, SPELL_STONE_SKIN}}},
  {450, {RACE_GREY, RACE_CENTAUR, RACE_HALFELF, 0}, {{6, SPELL_STONE_SKIN}}},
  {480, {RACE_HUMAN, RACE_BARBARIAN, RACE_HALFORC, RACE_HUMANOID, RACE_HALFLING, 0}, {{6, SPELL_STONE_SKIN}}},
  {510, {RACE_DEMON, RACE_DEVIL, RACE_GITHYANKI, RACE_VAMPIRE, RACE_DRAGON, 0}, {{6, SPELL_STONE_SKIN}}},
  {662, {RACE_HUMAN, RACE_GNOME, RACE_HALFELF, 0}, {{6, SPELL_STONE_SKIN}}},
  {671, {RACE_HUMAN, RACE_MOUNTAIN, RACE_DUERGAR, 0}, {{6, SPELL_STONE_SKIN}}},
  {676, {RACE_THRIKREEN, 0}, {{6, SPELL_STONE_SKIN}}},
  // The Town of Moregeeth (goblin ht)
  {700, {RACE_GOBLIN, 0}, {{4, SPELL_ARMOR}}},
  {710, {RACE_HUMAN, RACE_ORC, RACE_HALFORC, RACE_MINOTAUR, RACE_HUMANOID, 0}, {{6, SPELL_STONE_SKIN}}},
  {756, {RACE_HUMAN, RACE_LICH, RACE_BARBARIAN, RACE_PHANTOM, RACE_DEMON, RACE_UNDEAD, 0}, {{6, SPELL_STONE_SKIN}}},
  {758, {RACE_HUMAN, RACE_HARPY, RACE_MOUNTAIN, RACE_HALFELF, 0}, {{6, SPELL_STONE_SKIN}}},
  // Rift Valley Jungle
  {800, {RACE_GREY, RACE_HALFLING, RACE_SNAKE, RACE_CARNIVORE, RACE_HERBIVORE, RACE_FLYING_ANIMAL, RACE_ARACHNID, 0}, {{5, SPELL_BLESS}, {8, SPELL_STONE_SKIN}}},
  {824, {RACE_HUMAN, RACE_GREY, RACE_HALFELF, RACE_RAKSHASA, RACE_CENTAUR, RACE_MOUNTAIN, 0}, {{6, SPELL_STONE_SKIN}}},
  {856, {RACE_UNDEAD, RACE_GHOST, RACE_ORC, RACE_REVENANT, RACE_SHADE, 0}, {{6, SPELL_STONE_SKIN}}},
  {857, {RACE_SGIANT, RACE_GIANT, RACE_DRAGONKIN, RACE_HUMAN, 0}, {{6, SPELL_STONE_SKIN}}},
  {936, {RACE_HUMAN, RACE_HALFORC, RACE_HALFELF, 0}, {{6, SPELL_STONE_SKIN}}},
  {955, {RACE_MOUNTAIN,0 }, {{6, SPELL_STONE_SKIN}}},
  {960, {RACE_GIANT, RACE_SGIANT, RACE_TROLL, 0}, {{6, SPELL_STONE_SKIN}}},
  {964, {RACE_ILLITHID, RACE_DROW, 0}, {{6, SPELL_STONE_SKIN}}},
  // Shady Grove (orc ht)
  {975, {RACE_ORC, RACE_HALFORC, 0}, {{5, SPELL_ARMOR}}},
  {987, {RACE_VAMPIRE, RACE_SHADE, RACE_PDKNIGHT, RACE_LICH, RACE_PHANTOM, RACE_WIGHT, 0}, {{6, SPELL_STONE_SKIN}}},
  // Tharnadia (human ht)
  {1325, {RACE_HUMAN, 0}, {{4, SPELL_ARMOR}}},
  {0, {0}, {{0, 0}} }
};

struct random_mob
{

  int      vnum;
  char    *name;
  char    *keywords;
  char    *desc;
  float    level;
  int      aligment;
  char    *race;
  char    *m_class;
  ulong    aggro_flags;

};

// I assume this is an event of some kind?
void spawn_random_mapmob()
{
  P_char random_mob;
  int    count = 0;

  for (random_mob = character_list; random_mob; random_mob = random_mob->next)
  {
    if( IS_NPC(random_mob) && GET_VNUM(random_mob) == 1255 )
    {
      count++;
    }
  }

  if (count < 45)
  {
    create_random_mob(-1, 0);
  }
}

// Really should list what themes are here.. but I don't know them.
P_char create_random_mob(int theme, int mob_level)
{
  P_char   random_mob;
  int      zone_number = 0;
  int      zone_idx = 0;
  int      race = 0;
  int      level = 0;
  int      prefix = 0;
  int      class_idx = 0;
  int      to_room = 0;
  int      map_room = 0;
  int      tries = 0;
  int      i;
  char     race_name[256], class_name[256], temp_name[256];
  char     t_buf[MAX_STRING_LENGTH];
  struct zone_data *the_zone = 0;
  P_obj    o;

  if( !mob_level )
  {
    mob_level = number(10, 61);
  }

  if( theme == -1 )
  {
    random_mob = read_mobile(1255, VIRTUAL);
  }
  else
  {
    random_mob = read_mobile(1256, VIRTUAL);
  }

  if( !random_mob )
  {
    return NULL;
  }

  int zones_random_data_size = 0;
  for( int i = 0; zones_random_data[i].zone; i++ )
  {
    zones_random_data_size++;
  }

  // If, for some reason, there's no data in the zones_random_data array.
  if( zones_random_data_size < 1 )
  {
    extract_char(random_mob);
    return NULL;
  }

  zone_idx = number(0, zones_random_data_size-1);
  zone_number = zones_random_data[zone_idx].zone;
  the_zone = &zone_table[real_zone0(zone_number)];
  to_room = the_zone->real_bottom;

  if( to_room <= 0 )
  {
    debug("Could not find starting room for zone %d for random mob", zone_number);
    extract_char(random_mob);
    return 0;
  }

  char_to_room(random_mob, to_room, 0);

  int races = 0;
  for( int i = 0; zones_random_data[zone_idx].races[i]; i++ )
  {
    races++;
  }
  if( races < 1 )
  {
    extract_char(random_mob);
    return NULL;
  }
  // Theme 3 is dwarf.
  if( theme == 3 )
  {
    if( number(0, 1) )
    {
      race = RACE_MOUNTAIN;
    }
    else
    {
      race = RACE_DUERGAR;
    }
  }
  else
  {
    race = zones_random_data[zone_idx].races[number(0, races-1)];
  }
  // Theme 4 is good race.
  if( theme == 4 )
  {
    while (TRUE)
    {
      // Pick one of the first 27 races, or up to LAST_RACE if there are less than 27 races.
      race = number( 1, MIN(27, LAST_RACE) );

      // Stop looking when we find a good race.
      if (IS_RACEWAR_GOOD(random_mob))
      {
        break;
      }
    }
  }
  // Theme 5 is evil race.
  else if (theme == 5)
  {
    while (TRUE)
    {
      race = number( 1, MIN(27, LAST_RACE) );

      // If racewar == EVIL, then racewar != UNDEAD.
      if( IS_RACEWAR_EVIL(random_mob) )
      {
        break;
      }
    }
  }
  // Theme 6 is undead race.
  else if( theme == 6 )
  {
    while (TRUE)
    {
      race = number( 1, MIN(27, LAST_RACE) );

      if( IS_RACEWAR_UNDEAD(random_mob) )
      {
        break;
      }
    }
  }
  else if (theme == 7)
  {
    if (number(0, 1))
    {
      race = RACE_DRAGONKIN;
    }
    else
    {
      race = RACE_DRAGON;
    }
  }
  random_mob->player.race = race;

  //find a class for it!
  // adding this counter to prevent an infinite loop if every column in class_table[race] == 5 | chage by Torgal
  // Doing this differently.. gonna start at random class, and walk around the class_table, and if we come back to it stop.
  i = class_idx = number(1, CLASS_COUNT);
  while( class_table[race][class_idx] == 5 )
  {
    // Increment index or go to 1 at CLASS_COUNT (skip CLASS_NONE == 0).
    class_idx = (class_idx == CLASS_COUNT) ? 1 : (class_idx + 1);

    // If we've gone around all the classes..
    if( i == class_idx )
    {
      extract_char(random_mob);
      return NULL;
    }
  }
  // Convert class from a number to a bit.
  random_mob->player.m_class = 1 << (class_idx - 1);

  random_mob->specials.alignment =
    BOUNDED(-1000, 1000 * class_table[race][class_idx], 1000);


  // find a prefix
  prefix = number(1, 35);
  //set long desc
  for( i = 0; race_names_table[race].ansi[i]; i++ )
  {
    if (i && race_names_table[race].ansi[i - 1] != '+')
    {
      race_name[i] = tolower(race_names_table[race].ansi[i]);
    }
    else
    {
      race_name[i] = race_names_table[race].ansi[i];
    }
  }
  race_name[i] = 0;

  for (i = 0; class_names_table[class_idx].ansi[i]; i++)
  {
    if (i && class_names_table[class_idx].ansi[i - 1] != '+')
    {
      class_name[i] = tolower(class_names_table[class_idx].ansi[i]);
    }
    else
    {
      class_name[i] = class_names_table[class_idx].ansi[i];
    }
  }
  class_name[i] = 0;

  if( theme == -1 )
  {
    if( number(0, 4) && IS_HUMANOID(random_mob) )
    {
      if( get_name(temp_name) )
      {
        random_mob->player.sex = SEX_MALE;
      }
      else
      {
        random_mob->player.sex = SEX_FEMALE;
      }
      random_mob->only.npc->str_mask |= STRUNG_DESC1;
      sprintf(t_buf, "&+W%s&+w the&n %s %s &+Wfrom %s&n.", temp_name,
              race_name, class_name, the_zone->name);
      random_mob->player.long_descr = str_dup(t_buf);

      //Short
      random_mob->only.npc->str_mask |= STRUNG_DESC2;

      sprintf(t_buf, "%s the %s %s", temp_name, race_name, class_name);
      random_mob->player.short_descr = str_dup(t_buf);
      //name!
      random_mob->only.npc->str_mask |= STRUNG_KEYS;

      sprintf(t_buf, "%s %s %s", temp_name, strip_ansi(race_name).c_str(), strip_ansi(class_name).c_str());
      random_mob->player.name = str_dup(t_buf);
    }
    else
    {
      random_mob->only.npc->str_mask |= STRUNG_DESC1;
      sprintf(t_buf, "%s %s %s &+Wfrom %s&n.", prefix_name[prefix], race_name,
              class_name, the_zone->name);
      random_mob->player.long_descr = str_dup(t_buf);

//Short
      random_mob->only.npc->str_mask |= STRUNG_DESC2;

      sprintf(t_buf, "%s %s %s", prefix_name[prefix], race_name, class_name);
      random_mob->player.short_descr = str_dup(t_buf);
//name!
      random_mob->only.npc->str_mask |= STRUNG_KEYS;

      sprintf(t_buf, "%s %s", strip_ansi(race_name).c_str(), strip_ansi(class_name).c_str());
      random_mob->player.name = str_dup(t_buf);
    }
  }
  else
  {
    prefix = number(0, 1);
    random_mob->only.npc->str_mask |= STRUNG_DESC1;
    sprintf(t_buf, "%s %s %s&n stands here.",
            prefix_name_theme[theme][prefix], race_name, class_name);
    random_mob->player.long_descr = str_dup(t_buf);

//Short
    random_mob->only.npc->str_mask |= STRUNG_DESC2;

    sprintf(t_buf, "%s %s %s", prefix_name_theme[theme][prefix], race_name,
            class_name);
    random_mob->player.short_descr = str_dup(t_buf);
//name!
    random_mob->only.npc->str_mask |= STRUNG_KEYS;

    sprintf(t_buf, "%s %s", strip_ansi(race_name).c_str(), strip_ansi(class_name).c_str());
    random_mob->player.name = str_dup(t_buf);
  }

  //ok let's set some lvls and other attribs of the mob!
  //level
  // Simplified this
  level = (mob_level>0) ? mob_level : number(30, 61);

  random_mob->player.level = MIN(level, 61);
  //hit, dam
  //2k - 30k
  random_mob->points.base_hitroll = random_mob->points.hitroll = level;
  random_mob->points.base_damroll = random_mob->points.damroll = level;
// dice
  random_mob->points.damnodice = (int) MAX(1, level / 5 + number(0, 3));
  random_mob->points.damsizedice = (int) MAX(1, level / 5);

  if (GET_CLASS(random_mob, CLASS_MONK))
  {
    MonkSetSpecialDie(random_mob);
  }
//hps
  GET_MAX_HIT(random_mob) = GET_HIT(random_mob) = random_mob->points.base_hit =
    (int) (((level * level * level) / 12 + number(0, 89)) / 4);

//let's set some cash on it!
  GET_PLATINUM(random_mob) = number((int) (level / 2), level * 2);
  GET_GOLD(random_mob) = number(0, level);
//  GET_SILVER(random_mob) = number(0, level);
//  GET_COPPER(random_mob) = number(0, 99);

  //set right size!
  // Why you do GET_RACE here instead of just race is beyond me, but ok.
  switch( GET_RACE(random_mob) )
  {
  case RACE_NONE:
  case RACE_HUMAN:
  case RACE_DROW:
  case RACE_GREY:
  case RACE_HALFELF:
  case RACE_ILLITHID:
  case RACE_ORC:
  case RACE_THRIKREEN:
  case RACE_MOUNTAIN:
  case RACE_GITHYANKI:
  case RACE_LICH:
  case RACE_PVAMPIRE:
  case RACE_DUERGAR:
  case RACE_PHANTOM:
  case RACE_PSBEAST:
    GET_SIZE(random_mob) = SIZE_MEDIUM;
    break;
  case RACE_HALFLING:
  case RACE_GNOME:
  case RACE_GOBLIN:
    GET_SIZE(random_mob) = SIZE_SMALL;
    break;
  case RACE_TROLL:
  case RACE_BARBARIAN:
  case RACE_PDKNIGHT:
  case RACE_CENTAUR:
    GET_SIZE(random_mob) = SIZE_LARGE;
    break;
  case RACE_OGRE:
  case RACE_MINOTAUR:
  case RACE_SGIANT:
  case RACE_WIGHT:
    GET_SIZE(random_mob) = SIZE_HUGE;
    break;
  case RACE_F_ELEMENTAL:
  case RACE_A_ELEMENTAL:
  case RACE_W_ELEMENTAL:
  case RACE_E_ELEMENTAL:
    GET_SIZE(random_mob) = SIZE_MEDIUM + number(0, 1);
    break;
  case RACE_DEMON:
  case RACE_DEVIL:
    GET_SIZE(random_mob) = SIZE_HUGE + number(-1, 1);
    SET_BIT(random_mob->only.npc->aggro_flags, AGGR_ALL);
    break;
  case RACE_UNDEAD:
  case RACE_VAMPIRE:
  case RACE_GHOST:
  case RACE_LYCANTH:
    GET_SIZE(random_mob) = SIZE_MEDIUM;
    break;
  case RACE_GIANT:
    GET_SIZE(random_mob) = SIZE_HUGE + number(0, 1);
    break;
  case RACE_HALFORC:
    GET_SIZE(random_mob) = SIZE_MEDIUM;
    break;
  case RACE_GOLEM:
    GET_SIZE(random_mob) = SIZE_HUGE + number(0, 1);
    break;
  case RACE_FAERIE:
    GET_SIZE(random_mob) = SIZE_MEDIUM + number(0, 1);
    break;
  case RACE_DRAGON:
  case RACE_DRAGONKIN:
    GET_SIZE(random_mob) = SIZE_HUGE + number(0, 1);
    SET_BIT(random_mob->only.npc->aggro_flags, AGGR_ALL);
    break;
  case RACE_REPTILE:
    GET_SIZE(random_mob) = SIZE_SMALL + number(0, 2);
    SET_BIT(random_mob->only.npc->aggro_flags, AGGR_ALL);
    break;
  case RACE_SNAKE:
  case RACE_INSECT:
  case RACE_ARACHNID:
  case RACE_AQUATIC_ANIMAL:
  case RACE_FLYING_ANIMAL:
    GET_SIZE(random_mob) = SIZE_TINY;
    break;
  case RACE_QUADRUPED:
    GET_SIZE(random_mob) = SIZE_SMALL + number(0, 2);
    break;
  case RACE_PRIMATE:
  case RACE_HUMANOID:
  case RACE_ANIMAL:
    GET_SIZE(random_mob) = SIZE_SMALL + number(0, 2);
    break;
  case RACE_PLANT:
  case RACE_HERBIVORE:
  case RACE_CARNIVORE:
    GET_SIZE(random_mob) = SIZE_SMALL + number(0, 2);
    break;
  case RACE_PARASITE:
    GET_SIZE(random_mob) = SIZE_TINY;
    SET_BIT(random_mob->only.npc->aggro_flags, AGGR_ALL);
    break;
  case RACE_BEHOLDER:
  case RACE_DRACOLICH:
    GET_SIZE(random_mob) = SIZE_HUGE + number(0, 1);
    SET_BIT(random_mob->only.npc->aggro_flags, AGGR_ALL);
    break;
  case RACE_SLIME:
    GET_SIZE(random_mob) = SIZE_SMALL + number(0, 1);
    break;
  case RACE_GITHZERAI:
  case RACE_ANGEL:
  case RACE_RAKSHASA:
    GET_SIZE(random_mob) = SIZE_MEDIUM;
    break;
  }

  if(GET_LEVEL(random_mob) == 61)
  {
    SET_BIT(random_mob->specials.act, ACT_ELITE);
    SET_BIT(random_mob->specials.affected_by, AFF_BIOFEEDBACK);
  }

  if(GET_LEVEL(random_mob) >= 56)
  {
    SET_BIT(random_mob->specials.affected_by4, AFF4_DETECT_ILLUSION);
  }

  if(GET_LEVEL(random_mob) >= 50)
  {
    SET_BIT(random_mob->specials.affected_by, AFF_AWARE);
    SET_BIT(random_mob->specials.affected_by, AFF_SENSE_LIFE);
    SET_BIT(random_mob->specials.affected_by, AFF_HASTE);
  }

  //give the mob 1-3 random items!
  if( theme == -1 || 0 == number(0, 1) )
  {
    o = create_random_eq_new(random_mob, random_mob, -1, -1);
    obj_to_char(o, random_mob);

    while( !number(0, 2) )
    {
      o = create_random_eq_new(random_mob, random_mob, -1, -1);
      obj_to_char(o, random_mob);
    }
    do_wear(random_mob, "all", 0);
  }

  if (theme == -1)
  {
    if (!find_mob_map_room(random_mob))
    {
      extract_char(random_mob);
      return 0;
    }
  }

  if( theme != -1)
  {
    SET_BIT(random_mob->only.npc->aggro_flags, AGGR_ALL);
  }
  else
  {
    debug("Created level(%d): %s in %s (%d)", GET_LEVEL(random_mob), random_mob->player.long_descr,
      world[random_mob->in_room].name, world[random_mob->in_room].number);
  }

  GET_HOME(random_mob) = GET_BIRTHPLACE(random_mob) =
    GET_ORIG_BIRTHPLACE(random_mob) = world[random_mob->in_room].number;

  return random_mob;

}

int find_mob_map_room(P_char random_mob)
{
  int      tries = 0;
  int      to_room = 0;

  do
  {
    to_room = number(0, top_of_world);
    tries++;
  } while ((IS_SET(world[to_room].room_flags, PRIVATE) ||
         IS_SET(world[to_room].room_flags, PRIV_ZONE) ||
         IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
         world[to_room].sector_type == SECT_OCEAN ||
          world[to_room].sector_type == SECT_MOUNTAIN ||
          world[to_room].sector_type == SECT_WATER_SWIM ||
          world[to_room].sector_type == SECT_WATER_NOSWIM ||
         !IS_MAP_ROOM(to_room)) && tries < 2000);

  if (tries < 2000) {
    char_from_room(random_mob);
    char_to_room(random_mob, to_room, 0);
    return to_room;
  } else
    return 0;
}

int find_a_zone()
{
  int      zone = 0;

  zone = number(1, top_of_zone_table);
  return zone;
}

