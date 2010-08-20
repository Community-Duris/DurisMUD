/****************************************************************************
 *
 *  File: randomeq.c                                           Part of Duris
 *  Usage: randomboject.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark                   Date: 2002-04-18
 * ***************************************************************************
 */

#define TROPHY

#include <stdio.h>
#include <string.h>
#include <math.h>

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
#include "defines.h"
/*
 * external variables
 */
extern Skill skills[];
extern struct zone_data *zone_table;
extern const char *material_names[];
extern P_char character_list;
extern P_desc descriptor_list;
extern P_event event_type_list[];
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern char debug_mode;
extern const char *race_types[];
extern const int exp_table[];

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
extern void material_restrictions(P_obj);

int      find_map_place();
void     create_zone(int theme, int map_room1, int map_room2, int level_range,
                     int rooms);
void     set_keywords(P_obj t_obj, const char *newKeys);
void     set_short_description(P_obj t_obj, const char *newShort);
void     set_long_description(P_obj t_obj, const char *newDescription);


//m_number is the material number used by game check, constant.c for the
//correct value! - Kvark

struct randomeq_prefix
{

  int      m_number;
  char    *m_name;
  float    m_stat;
  float    m_ac;
  int      weight;
};


const int highdrop_mobs[64] = {
  19700, // Tiamat
  25040, // Brass Sultan
  25700, // Bahamut
  81454, // Chronomancer
  35523, // Dark Knight
  38317, // Lady Death
  44172, // King Arkan'non
  26642, // the Dark
  3524,  // Tyrlos
  45565, // Malevolence
  36856, // Kyeril
  25110, // Brass Padashaw
  21672, // King of Aravne
  98959, // Bard Faust
  87741, // snogres pit fiend
  87700, // snogres berserker
  87709, // snogres champion
  87729, // snogres chieftain
  38737, // Ny'Neth
  70941, // Kithron
  88316, // Kossuth
  87561, // Zuggtmoy
  87544, // Jubilex
  87613, // Graz'zt
  87612, // Lolth
  32637, // Aramus
  16205, // Aceralde
  43576, // Eligoth
  32420, // Bel
  58835, // Chenovog
  71259, // Yeenoghu
  78439, // Kyzastaxkasis
  58379, // Obox-ob
  58383, // Strahd
  2435,  // Llamanby
  15113, // Dranum
  91031, // Doru
  80538, // Skrentherlog
  8746,  // Crymson
};

const char *stone_list[] = {
  "",
  "&+Ggreen&+L stone&n",
  "&+Rred&+L stone&n",
  "&+Yyellow&+L stone&n",
  "&+Bblue&+L stone&n",
  "&+Lblack&+L stone&n",
  "&+Mpink&+L stone&n",
  "&+Wsharp &+rrubin&n",
  "&+Yyellow &+Wand &+bblue&+w cross&n",
  "&+rdark bl&+Roo&+rds&+Rto&+rne&n",
  " "
};

int      stone_spell_list[] = {
  0,
  SPELL_ACID_BLAST,
  SPELL_FIREBALL,
  SPELL_MAGIC_MISSILE,
  SPELL_CHILL_TOUCH,
  SPELL_NEGATIVE_CONCUSSION_BLAST,
  SPELL_SHOCKING_GRASP,
  SPELL_FLAMESTRIKE,
  SPELL_BLINDNESS,
  SPELL_ENERGY_DRAIN,
  0
};



struct randomeq_material material_data[MAXMATERIAL + 1] = {
  {3, "cloth", 12, 1},          //Cloth
  {5, "&+ysoftwood&n", 12, 3},  //soft wood
  {6, "&+yhardwood&n", 14, 4},  //hard wood
  {4, "&+ybark&n", 15, 6},      //Bark
  {7, "&+Wglass&n", 16, 2},     //glass
  {9, "&+Lclay&n", 17, 4},      //clay
  {12, "&+yhide&n", 18, 5},     //hide
  {39, "&+glimestone&n", 19, 3},        //limestone
  {21, "&+ycopper&n", 20, 6},   //copper
  {13, "&+yleather", 21, 5},    //leather
  {20, "&+ybronze&n", 21, 6},   //bronze
  {10, "&+wbone&n", 22, 7},     //bone
  {17, "&+ybrass&n", 23, 7},    //brass
  {34, "&+Wivory&n", 24, 5},    //ivory
  {37, "&+wgranite&n", 25, 9},  //granite
  {38, "&+Wma&+wr&+Lb&+wl&+We&n", 25, 7},       //marble
  {14, "&+ycured leather&n", 25, 6},    //cured leather
  {16, "steel", 30, 12},        //steel
  {15, "&+ciron&n", 30, 8},    //iron
  {36, "&+Lobsidian&n", 30, 10},        //obsidian
  {32, "&+Gemerald&n", 34, 7},  //emerald
  {8, "&+Ccrystal&n", 36, 7},   //crystal
  {26, "&+Rgem&n", 36, 7},      //gem
  {11, "&+Lstone&n", 38, 12},   //stone
  {2, "&+rflesh&n", 38, 5},     //Flesh
  {24, "&+Ygold&n", 38, 9},     //gold
  {33, "&+bsapphire&n", 38, 7}, //sapphire
  {22, "&+Wsilver&n", 40, 10},  //silver
  {31, "&+rruby&n", 42, 9},     //ruby
  {27, "&+Wdiamond&n", 43, 14}, //diamond
  {23, "&+Welectrum&n", 44, 12},        //electrum
  {19, "&+madamantium&n", 45, 15},      //adamantium
  {18, "&+Cmithril&n", 46, 15}, //mithril
  {25, "&+Wplatinum&n", 46, 13},        //platinum
  {35, "&+rdragon&+Lscale&n", 50, 15},  //dragon scale

};

struct randomeq_prefix prefix_data[MAXPREFIX + 1] = {
  {1, "&+Wm&+Ya&+Wg&+Yic&+Wal&n", 1.5, 1.5},
  {2, "&+Lmaste&+brly-craf&+Lted&n", 1.47, 1.45},
  {3, "&+Lsuperior&n", 1.41, 1.25},
  {4, "&+Yun&+Liq&+Yue&n", 1.4, 1.1},
  {5, "&+Cbeautiful&n", 1.35, 1.0},
  {6, "&+Lwell-cr&+ga&+Gf&+gt&+Led", 1.31, 1.40},
  {7, "&+Cfro&+Wst-rim&+Ced&n", 1.26, 0.9},
  {8, "&+Rfl&+Ya&+Wm&+Yi&+Rng", 1.25, 0.85},
  {9, "&+Mhumming&n", 1.23, 1.5},
  {10, "&+Bicy&n", 1.18, 0.82},
  {11, "&+Cfrozen&n", 1.16, 0.94},
  {12, "&+Wfine&n", 1.15, 1.2},
  {13, "&+wgl&+Wowi&+wng&n", 1.14, 1.0},
  {14, "&+gde&+Gc&+gent&n", 1.0, 1.0},
  {15, "&+wtarnished&n", 1.09, 1.0},
  {16, "&+Yg&+Wl&+Yitt&+We&+Yri&+Wn&+Yg&n", 1.05, 1.4},
  {17, "&+ytwined&n", 0.75, 0.8},
  {18, "&+cdented&n", 0.68, 0.9},
  {19, "&+gpitted&n", 0.62, 0.62},
  {20, "&+wcrude&n", 0.55, 0.70},
  {21, "&+ycracked&n", 0.5, 0.5},
  {22, "&+Wexquisite&n", 1.39, 1.4},
  {23, "&+rrusty&n", 0.7, 0.7},
  {24, "&+Lspiked&n", 1.2, 1.0},
  {25, "&+Cfanged&n", 1.3, 1.0},
  {26, "&+cstylish&n", 1.15, 1.05},
  {27, "&+Ldiabolical&n", 1.4, 1.4},
  {28, "&+Rsparkling&n", 1.40, 1.35},
  {29, "&+Ytwisted&n", 1.3, 1.2},
  {30, "&+Yshi&+Wmme&+Yring&n", 1.3, 1.4},
  {31, "&+whardened&n", 1.24, 1.23},
  {32, "&+Lgha&n&+wstly&n", 0.95, 1.0},
  {33, "&+Wan&+Cgel&n&+cic&n", 1.45, 1.35},
  {34, "&+mdrow&+L-made&n", 1.25, 1.0},
  {35, "&+Chuman&+L-made&n", 0.9, 1.15},
  {36, "&+Ydwarven&+L-made&n", 0.7, 0.9},
  {37, "&+run&+Learthly&n", 1.33, 1.30},
  {38, "&+yskin&+L-wrapped", 0.6, 0.7},
  {39, "&+Wje&+Cw&+Weled&n", 1.4, 1.30},
  {40, "&+Lsinister&n", 1.2, 1.20},
  {41, "&+Wsta&n&+wtic&n", 1.1, 1.20},
  {42, "&+Cgle&n&+cam&+Wing&n", 1.22, 0.9},
  {43, "&+wwell-used&n", 0.6, 0.5},
  {44, "&+Wleg&n&+wend&+Lary&n", 1.5, 1.4},
  {45, "&+Wknight's&n", 1.0, 1.11},
  {46, "&+Lsold&n&+wier's&n", 0.8, 0.9},
  {47, "&+Rbrutal&n", 0.8, 1.40},
  {48, "&+Cbor&+Weal&n", 0.8, 0.8},
  {49, "&+rbl&+Roo&+rdy&n", 0.9, 1.2},
  {50, "&+rblood-stained&n", 0.5, 0.6},
  {51, "&+Wlight&n", 0.8, 0.9},
  {52, "&+Ldark&n", 0.8, 0.9},
  {53, "&+cfr&+Cagi&+cle&n", 1.0, 1.0},
  {54, "&+rb&+yurn&+rt&n", 1.2, 0.9},
  {55, "&+mpatched&n", 1.23, 1.20},
  {56, "&+Rmisshaped&n", 1.03, 1.00},
  {57, "&+La&+ws&+Lh&+we&+Ln&n", 1.11, 1.20}
};

struct random_spells spells_data[61] = {
  {0, SPELL_ICE_MISSILE, 1, "&+Cice missiles"},
  {1, SPELL_CURE_LIGHT, 1},
  {2, SPELL_CURE_BLIND, 1},
  {3, SPELL_MAGIC_MISSILE, 0, "&+Ymagic missiles"},
  {4, SPELL_SHOCKING_GRASP, 0, "&+Bshocking"},
  {5, SPELL_SLOW, 0, "slowness"},
  {6, SPELL_ARMOR, 1},
  {7, SPELL_CONTINUAL_LIGHT, 1},
  {8, SPELL_DISPEL_EVIL, 0, "&+Wthe holy"},
  {9, SPELL_BLESS, 1},
  {10, SPELL_SLEEP, 0, "&+Lsleep"},
  {11, SPELL_MINOR_GLOBE, 1},
  {12, SPELL_DISPEL_GOOD, 0, "the unholy"},
  {13, SPELL_FAERIE_FOG, 0, "the faeries"},
  {14, SPELL_EGO_WHIP, 0, "mindwhipping"},
  {15, SPELL_FLESH_ARMOR, 1},
  {16, SPELL_WOLFSPEED, 1},
  {17, SPELL_HAWKVISION, 1},
  {18, SPELL_PYTHONSTING, 0, "&+gthe snake"},
  {19, SPELL_FLAMEBURST, 0, "&+Rflameburst"},
  {20, SPELL_SCORCHING_TOUCH, 0, "&+Rscorching"},
  {21, SPELL_CURE_SERIOUS, 1},
  {22, SPELL_MINOR_GLOBE, 1},
  {23, SPELL_RAY_OF_ENFEEBLEMENT, 0, "feebleness"},
  {24, SPELL_DARKNESS, 1},
  {25, SPELL_PWORD_STUN, 0, "&+Wstunning"},
  {26, SPELL_BARKSKIN, 1},
  {27, SPELL_ADRENALINE_CONTROL, 1},
  {28, SPELL_BALLISTIC_ATTACK, 0, "missiles"},
  {29, SPELL_SOUL_DISTURBANCE, 0, "&+Lsoul crushing"},
  {30, SPELL_MENDING, 1},
  {31, SPELL_BEARSTRENGTH, 1},
  {32, SPELL_SHADOW_MONSTER, 0, "&+Lshadows"},
  {33, SPELL_COLD_WARD, 1},
  {34, SPELL_MOLTEN_SPRAY, 0, "molten rocks"},
  {35, SPELL_FLAMEBURST, 0, "&+rflamebursts"},
  {36, SPELL_WITHER, 0, "&+Lwithering"},
  {37, SPELL_MASS_INVIS, 1},
  {38, SPELL_ICE_STORM, 0, "&+cblizzards"},
  {39, SPELL_COLDSHIELD, 1},
  {40, SPELL_FEEBLEMIND, 0, "&+Lmindbreaking"},
  {41, SPELL_COLOR_SPRAY, 0, "magespray"},
  {42, SPELL_FIRESHIELD, 1},
  {43, SPELL_CHILL_TOUCH, 0, "&+bchilling"},
  {44, SPELL_FIREBALL, 0, "&+Rfireballs"},
  {45, SPELL_HEAL, 1},
  {46, SPELL_POISON, 0, "&+gvenom"},
  {47, SPELL_FEAR, 0, "&+Lfear"},
  {48, SPELL_FLY, 1},
  {49, SPELL_COLOR_SPRAY, 0, "magespray"},
  {50, SPELL_GLOBE, 1},
  {51, SPELL_HARM, 0, "harming"},
  {52, SPELL_ENLARGE, 1},
  {53, SPELL_REDUCE, 1},
  {54, SPELL_ELEPHANTSTRENGTH, 1},
  {55, SPELL_DETONATE, 0, "detonating"},
  {56, SPELL_CURSE, 0, "&+ycurse"},
  {57, SPELL_STONE_SKIN, 1},
  {58, SPELL_BIOFEEDBACK, 1},
  {59, SPELL_IMMOLATE, 0, "&+Rimmolating"},
  {60, SPELL_BLUR, 1}
};

// MAX_SLOT found in config.h
 // number, name, magical_mod, ac_mod, wear_slot, pieces, weapon_type.
extern const struct randomeq_slots slot_data[MAX_SLOT + 1];
const struct randomeq_slots slot_data[MAX_SLOT + 1] = {
  {1, "&+Lring&N", 1.0, 0.0, ITEM_WEAR_FINGER, 1, 0},
  {2, "&+Lband&N", 1.1, 0.0, ITEM_WEAR_FINGER, 1, 0},
  {3, "&+Lsignet&n", 1.25, 0.0, ITEM_WEAR_FINGER, 1, 0},
  {4, "&+Lnecklace&N", 1.1, 0.0, ITEM_WEAR_NECK, 2, 0},
  {5, "&+Lcollar&N", 1.05, 1.0, ITEM_WEAR_NECK, 2, 0},
  {6, "&+Lchoker&N", 1.08, 0.5, ITEM_WEAR_NECK, 2, 0},
  {7, "&+Lchestplate&N", 1.3, 1.3, ITEM_WEAR_BODY, 3, 0},
  {8, "&+Lplatemail&N", 1.7, 1.7, ITEM_WEAR_BODY, 3, 0},
  {9, "&+Lringmail&N", 1.5, 1.45, ITEM_WEAR_BODY, 3, 0},
  {10, "&+Lrobe&N", 1.5, 0.7, ITEM_WEAR_BODY, 2, 0},
  {11, "&+Ltunic&n", 0.8, 0.5, ITEM_WEAR_BODY, 1, 0},
  {12, "&+Lhelmet&N", 1.05, 1.0, ITEM_WEAR_HEAD, 2, 0},
  {13, "&+Lhelm&N", 1.0, 1.0, ITEM_WEAR_HEAD, 2, 0},
  {14, "&+Lcrown&N", 1.06, 0.6, ITEM_WEAR_HEAD, 2, 0},
  {15, "&+Lhat&N", 1.0, 0.4, ITEM_WEAR_HEAD, 2, 0},
  {16, "&+Lskullcap&n", 1.2, 0.3, ITEM_WEAR_HEAD, 2, 0},
  {17, "&+Lleggings&N", 1.2, 1.0, ITEM_WEAR_LEGS, 2, 0},
  {18, "&+Lleg plates&N", 1.15, 1.1, ITEM_WEAR_LEGS, 2, 0},
  {19, "&+Lpants&N", 1.05, 0.8, ITEM_WEAR_LEGS, 2, 0},
  {20, "&+Lgreaves&n", 1.25, 1.5, ITEM_WEAR_LEGS, 3, 0},
  {21, "&+Lshoes&N", 1.0, 0.7, ITEM_WEAR_FEET, 2, 0},
  {22, "&+Lboots&N", 1.1, 1.0, ITEM_WEAR_FEET, 2, 0},
  {23, "&+Lmoccasins&n", 1.2, 0.7, ITEM_WEAR_FEET, 2, 0},
  {24, "&+Lgauntlets&N", 1.35, 1.0, ITEM_WEAR_HANDS, 2, 0},
  {25, "&+Ltalons&N", 1.2, 1.0, ITEM_WEAR_HANDS, 2, 0},
  {26, "&+Lgloves&N", 1.25, 1.1, ITEM_WEAR_HANDS, 2, 0},
  {27, "&+Lmitts&n", 0.9, 0.9, ITEM_WEAR_HANDS, 2, 0},
  {28, "&+Lsleeves&N", 1.3, 0.8, ITEM_WEAR_ARMS, 2, 0},
  {29, "&+Larm plates&N", 1.2, 1.2, ITEM_WEAR_ARMS, 2, 0},
  {30, "&+Lvambraces&n", 1.4, 1.4, ITEM_WEAR_ARMS, 3, 0},
  {31, "&+Lshield&N", 1.4, 1.3, ITEM_WEAR_SHIELD, 3, 0},
  {32, "&+Lheater shield&N", 1.3, 1.8, ITEM_WEAR_SHIELD, 3, 0},
  {33, "&+Lbodycloak&N", 1.42, 1.0, ITEM_WEAR_ABOUT, 3, 0},
  {34, "&+Lcloak&N", 1.37, 1.0, ITEM_WEAR_ABOUT, 3, 0},
  {35, "&+Lmantle&N", 1.4, 1.0, ITEM_WEAR_ABOUT, 3, 0},
  {36, "&+Lbelt&N", 1.23, 0.5, ITEM_WEAR_WAIST, 2, 0},
  {37, "&+Lgirth&N", 1.0, 1.0, ITEM_WEAR_WAIST, 2, 0},
  {38, "&+Lbracer&N", 1.0, 0.7, ITEM_WEAR_WRIST, 2, 0},
  {39, "&+Lbracelet&N", 1.1, 0.5, ITEM_WEAR_WRIST, 2, 0},
  {40, "&+Lwristguard&N", 1.0, 0.5, ITEM_WEAR_WRIST, 2, 0},
  {41, "&+Leyepatch&N", 1.1, 0.6, ITEM_WEAR_EYES, 2, 0},
  {42, "&+Lvisor&N", 1.1, 0.6, ITEM_WEAR_EYES, 2, 0},
  {43, "&+Lmask&N", 1, 1.0, ITEM_WEAR_FACE, 2, 0},
  {44, "&+Lveil&N", 1.05, 0.5, ITEM_WEAR_FACE, 2, 0},
  {45, "&+Learring&N", 1, 0.0, ITEM_WEAR_EARRING, 1, 0},
  {46, "&+Lstud&N", 0.7, 0.0, ITEM_WEAR_EARRING, 1, 0},
  {47, "&+Lquiver&N", 0.5, 0.0, ITEM_WEAR_QUIVER, 2, 0},
  {48, "&+Lbadge&N", 0.5, 0.0, ITEM_GUILD_INSIGNIA, 1, 0},
  {49, "&+Lsaddle&N", 0.7, 0.8, ITEM_HORSE_BODY, 1, 0},
  {50, "&+Ltail protector&N", 0.8, 0.2, ITEM_WEAR_TAIL, 1, 0},
  {51, "&+Lnose ring&N", 0.8, 0.0, ITEM_WEAR_NOSE, 1, 0},
  {52, "&+Lspider armor&N", 0.7, 0.8, ITEM_SPIDER_BODY, 1, 0},
  {53, "&+Lskull&N", 1.4, 0.5, ITEM_WEAR_HEAD, 2, 0},
  {54, "&+Lhorn&N", 1.0, 0.0, ITEM_WEAR_HORN, 1, 0},
  {55, "&+Lchainmail&N", 1.5, 1.3, ITEM_WEAR_BODY, 3, 0},
  {56, "&+Lcuirass&N", 1.3, 1.2, ITEM_WEAR_BODY, 2, 0},
  {57, "&+Lbreastplate&N", 1.4, 1.5, ITEM_WEAR_BODY, 2, 0},
  {58, "&+Lamulet&N", 1.1, 0.0, ITEM_WEAR_NECK, 2, 0},
  {59, "&+Lmedallion&N", 1.2, 0.0, ITEM_WEAR_NECK, 2, 0},
  {60, "&+Lcharm&N", 1.0, 0.0, ITEM_WEAR_NECK, 2, 0},
  {61, "&+Lpendant&N", 1.2, 0.0, ITEM_WEAR_NECK, 2, 0},
  {62, "&+Ltorque&N", 1.2, 1.0, ITEM_WEAR_NECK, 2, 0},
  {63, "&+Lgorget&n", 1.3, 1.5, ITEM_WEAR_NECK, 3, 0},
  {64, "&+Lcap&N", 1.1, 0.3, ITEM_WEAR_HEAD, 2, 0},
  {65, "&+Lcoif&N", 1.0, 1.1, ITEM_WEAR_HEAD, 2, 0},
  {66, "&+Lcirclet&N", 1.4, 0.6, ITEM_WEAR_HEAD, 2, 0},
  {67, "&+Ltiara&N", 1.3, 0.3, ITEM_WEAR_HEAD, 2, 0},
  {68, "&+Lhood&N", 1.2, 0.8, ITEM_WEAR_HEAD, 2, 0},
  {69, "&+Lclaws&N", 1.2, 1.0, ITEM_WEAR_HANDS, 2, 0},
  {70, "&+Lbuckler&N", 1.0, 1.0, ITEM_WEAR_SHIELD, 3, 0},
  {71, "&+Ltower shield&N", 1.1, 2.0, ITEM_WEAR_SHIELD, 3, 0},
  {72, "&+Lcord&n", 0.9, 0.1, ITEM_WEAR_TAIL, 2, 0},

/*
How to setting stats to random weapons works - Astansus's school for the inexperienced

#1, The first number is just the serial number - MUST update value in config.h if you add weapons
#2, Name of the weapon
#3, Modifier to the hit/dam of the weapon
       a value = 2.5 will give up to 9/9 if you get material = dragonscale
#4, Modifier to the dice of the weapon
       a value = 2.5 will give up to 9d9 if you get material = dragonscale
#5, ITEM_WIELD just makes this a weapon
#6, the number of pieces needed to craft a weapon of this type
       1-2 pieces 1handed weapon, 3+ pieces 2handed weapon
       the more pieces you add the heavier the weapon will become
#7, type of damage the weapon has - slash, bludgeon, pierce...

*/
  {73, "&+Llong sword&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_LONGSWORD},
  {74, "&+Ybattle axe&n", 1.1, 1.2, ITEM_WIELD, 3, WEAPON_AXE},
  {75, "&+Ldagger&n", 0.8, 0.8, ITEM_WIELD, 2, WEAPON_DAGGER},
  {76, "&+rwarhammer&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_HAMMER},
  {77, "&+Lsword&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_LONGSWORD},
  {78, "&+Lshort sword&n", 0.8, 0.9, ITEM_WIELD, 2, WEAPON_SHORTSWORD},
  {79, "&+Glance&n", 1.2, 1.2, ITEM_WIELD, 2, WEAPON_LANCE},
  {80, "&+Ldrusus&n", 0.8, 0.8, ITEM_WIELD, 2, WEAPON_SHORTSWORD},
  {81, "&+Yghurka&n", 0.8, 0.9, ITEM_WIELD, 2, WEAPON_SHORTSWORD},
  {82, "&+Lflail&n", 1.2, 1.1, ITEM_WIELD, 3, WEAPON_FLAIL},
  {83, "&+rfalchion&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_LONGSWORD},
  {84, "&+Rmaul&n", 0.9, 1.4, ITEM_WIELD, 3, WEAPON_HAMMER},
  {85, "&+Lbroad sword&n", 1.1, 1.2, ITEM_WIELD, 2, WEAPON_LONGSWORD},
  {86, "&+Lgreat sword&n", 1.1, 1.2, ITEM_WIELD, 3, WEAPON_2HANDSWORD},
  {87, "&+Lmorningstar&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_MACE},
  {88, "&+ycudgel&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_CLUB},
  {89, "&+Lscythe&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_SICKLE},
  {90, "&+Wscimitar&n", 1.1, 0.9, ITEM_WIELD, 2, WEAPON_LONGSWORD},
  {91, "&+Btrident&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_TRIDENT},
  {92, "&+Lmace&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_MACE},
  {93, "&+Lpronged fork&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_TRIDENT},
  {94, "&+Lwhip&n", 1.1, 0.9, ITEM_WIELD, 2, WEAPON_WHIP},
  {95, "&+Lknife&n", 0.8, 0.8, ITEM_WIELD, 2, WEAPON_DAGGER},
  {96, "&+Lshillelagh&n", 1.2, 1.2, ITEM_WIELD, 3, WEAPON_CLUB},
  {97, "&+Lrod&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_CLUB},
  {98, "&+Llongbow&n", 1.0, 1.0, ITEM_WIELD, 3, 0},
  {99, "&+Lstiletto&n", 1.0, 0.6, ITEM_WIELD, 2, WEAPON_DAGGER},
  {100, "&+Lbastard sword&n", 1.1, 1.2, ITEM_WIELD, 3, WEAPON_2HANDSWORD},
  {101, "&+Whand axe&n", 0.9, 1.1, ITEM_WIELD, 2, WEAPON_AXE},
  {102, "&+yquarterstaff&n", 1.4, 0.9, ITEM_WIELD, 2, WEAPON_CLUB},
  {103, "&+Lleister&n", 1.1, 1.2, ITEM_WIELD, 3, WEAPON_TRIDENT},
  {104, "&+Lsickle&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_SICKLE},
  {105, "&+Ldirk&n", 1.0, 0.7, ITEM_WIELD, 2, WEAPON_SHORTSWORD},
  {106, "&+Rswitchblade&n", 0.8, 0.8, ITEM_WIELD, 2, WEAPON_DAGGER},
  {107, "&+rmallet&n", 1.0, 1.0, ITEM_WIELD, 2, WEAPON_HAMMER},
  {108, "&+mtruncheon&n", 1.1, 1.2, ITEM_WIELD, 3, WEAPON_CLUB},
  {109, "&+LBUGGED&n", 0.1, 0.1, ITEM_WIELD, 2, WEAPON_DAGGER}


};

void create_randoms()
{
  int      theme = 0;
  int      map_room1 = 0;
  int      map_room2 = 0;
  int      level_range = 0;
  int      rooms = 0;
  int      map_room = 0;
  int      x = 0;
  int      tries = 0;
  long     time_before = 0;
  long     time_after = 0;
  int      i = 0;
  char     fname[256];
  char     buf2[MAX_STRING_LENGTH];
  FILE    *f;
  P_obj    fountain;

#ifndef RANDOM_ZONES
  fprintf(stderr, "Boot random zones -- BEGIN.\r\n");

  time_before = clock();


  while (x < 5)
  {
    fountain = read_object(real_object(70), REAL);
    if (fountain)
    {
      while (!(map_room = find_map_place()))
      {
        if( ++tries > 12 )
          break;
      }
      obj_to_room(fountain, map_room);

      wizlog(56, "Fountain loaded in room %d", map_room);
    }
    else
    {
      wizlog(56, "Could not load fountain item [70]");
    }
    
    x++;
  }

  fprintf(stderr, " -- Generating random zones\r\n");

  while (i < 45)
  {
    create_zone(number(0, 2), 999, 999, 999, 999);
    i++;
  }

  fprintf(stderr, " -- Generating random mobs\r\n");

  i = 0;
  while (i < 15)
  {
    create_random_mob(-1, 0);
    i++;
  }

  fprintf(stderr, " -- Generating random zone from areas/RANDOM_AREA file\r\n");

  sprintf(fname, "areas/RANDOM_AREA");
  f = fopen(fname, "r");
  if (!f)
  {
    wizlog(56,
           "Missing file areas/RANDOM_AREA unable to load specified random zones...");
    return;
  }
  while (!(feof(f)))
  {
    if (fgets(buf2, sizeof(buf2) - 1, f))
    {

      if (sscanf
          (buf2, "%d %d %d %d %d", &theme, &map_room1, &map_room2,
           &level_range, &rooms) == 5)
        create_zone(theme, map_room1, map_room2, level_range, rooms);

    }
  }
  fclose(f);

  time_after = clock();
  fprintf(stderr, "Boot random zones -- DONE in: %d milliseconds\n",
          (int) ((time_after - time_before) * 1E3 / CLOCKS_PER_SEC));
#endif 

}

P_obj create_material(int index)
{
  char     buf1[MAX_STRING_LENGTH];
  char     buf2[MAX_STRING_LENGTH];
  char     buf3[MAX_STRING_LENGTH];

  P_obj obj = read_object(RANDOM_EQ_VNUM, VIRTUAL);
  obj->material = material_data[index].m_number;

  sprintf(buf1, "random piece %s", strip_ansi(material_data[index].m_name).c_str());
  sprintf(buf2, "a piece of %s&n", material_data[index].m_name);
  sprintf(buf3, "&+La piece of %s&n lies here.", material_data[index].m_name);

  set_keywords(obj, buf1);
  set_short_description(obj, buf2);
  set_long_description(obj, buf3);

  SET_BIT(obj->wear_flags, BIT_15);
  SET_BIT(obj->wear_flags, BIT_1);

  convertObj(obj);

  return obj;
}


P_obj create_material(P_char killer, P_char mob)
{
  if(GET_CLASS(killer, CLASS_ALCHEMIST))
  {
    if(IS_PC(killer) && !number(0,6)){
      killer->only.pc->spell_bind_used = killer->only.pc->spell_bind_used - 10;
       send_to_char("&+YYou feel your power increase some...&n\n", killer);  
    }
  }
  int howgood = (int) ((GET_LEVEL(killer) + GET_LEVEL(mob)) / 2.8); /* make sure you understand this value before ya change it. */
  int material_index = number(0, BOUNDED(1, howgood, MAXMATERIAL));

  return create_material(material_index);
}

P_obj create_stones(P_char ch)
{
  P_obj    obj;
  char     buf1[MAX_STRING_LENGTH];
  int      i = number(1, 9);  // stones_list # of elements

  obj = read_object(RANDOM_OBJ_VNUM, VIRTUAL);
  sprintf(buf1, "random strange %s _strange_", strip_ansi(stone_list[i]).c_str());

  if ((obj->str_mask & STRUNG_KEYS) && obj->name)
    FREE(obj->short_description);
  obj->short_description = NULL;
  obj->str_mask |= STRUNG_KEYS;
  obj->name = str_dup(buf1);

  sprintf(buf1, "&+La strange %s", stone_list[i]);

  if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
    FREE(obj->short_description);
  obj->short_description = NULL;
  obj->str_mask |= STRUNG_DESC2;
  obj->short_description = str_dup(buf1);

  sprintf(buf1, "&+La strange %s lies here.", stone_list[i]);
  if ((obj->str_mask & STRUNG_DESC1) && obj->description)
    FREE(obj->description);
  obj->description = NULL;
  obj->str_mask |= STRUNG_DESC1;
  obj->description = str_dup(buf1);

  SET_BIT(obj->wear_flags, BIT_1);
  obj->value[6] = stone_spell_list[i];

  return obj;

}


int check_random_drop(P_char ch, P_char mob, int piece)
{
  int      dropprocent = 0;
  int      x, i = 0;
  int      trophy_mod, char_lvl, mob_lvl;
  struct trophy_data *tr;


  if (!ch || !mob)
    return 0;
  
  char_lvl = GET_LEVEL(ch);
  mob_lvl = GET_LEVEL(mob);

  if (!GET_EXP(mob))
    return 0;

  if (IS_SET(world[ch->in_room].room_flags, GUILD_ROOM))
    return 0;

  if (CHAR_IN_TOWN(ch) && (mob_lvl > 25))
    return 0;

  if ((char_lvl - mob_lvl) > 10)
    return 0;

  while (i < 15)
  {
    if (highdrop_mobs[i] == GET_VNUM(mob))
    {
      if (!number(0, 5))
      {
        return 1;
      }
    }
    i++;
  }

//  if (!IS_TRUSTED(ch) && !IS_NPC(ch))
//    for (tr = ch->only.pc->trophy; tr; tr = tr->next)
//    {
//      if (tr && GET_VNUM(mob) == tr->vnum)
//      {
//        trophy_mod = tr->kills > 0 ? tr->kills / 100 : 0;
//        break;
//      }
//      else
//        trophy_mod = 0;
//    }
//  else
    trophy_mod = 0;

  int luck = (int) GET_C_LUCK(ch);
  int chance;
  int charmobdiv = (mob_lvl / char_lvl);

 /* nix this for now for a more lenient formula - Jexni 07/12/08 // put the luck on a curve
  chance -= 50;
  chance *= 2;
  // but no more then 175!
  if (chance > 175.0)
    chance = 175.0;
   */
  
    chance = (int) ((luck / get_property("random.drop.luck.divisor", 10)) * charmobdiv);
    chance += number(0, 20);
    if(char_lvl < get_property("random.drop.increase.for.below.lvl", 20))
     chance += number(0, get_property("random.drop.increase.for.below.lvl.perc", 30));
  if(IS_ELITE(mob)) //another boost for elite npcs
    chance += number(5, 35);

  if (piece)
    chance *= (get_property("random.drop.piece.percentage", 20.0f) / 100.0);
  else
    chance *= (get_property("random.drop.equip.percentage", 2.0f) / 100.0);

  if (IS_HARDCORE(ch))
    chance = chance * (get_property("random.drop.modifier.hardcore", 150.0f) / 100.0);

  if (chance > number(0, 100 + (trophy_mod * 2)))
    return 1;

  return 0;
}

P_obj create_random_eq_new(P_char killer, P_char mob, int object_type,
                           int material_type)
{
  P_obj    obj;
  int      ansi_n = 0;
  int      ii;
  int      howgood, material, prefix, slot, value, bonus;
  int      should_have_weapon_proc = 0;
  struct zone_data *zone = 0;
  char     buf1[MAX_STRING_LENGTH];
  char     buf2[MAX_STRING_LENGTH];
  char     buf3[MAX_STRING_LENGTH];
  char     buf_temp[MAX_STRING_LENGTH];

  /*         Load the random item blank    */

  if (object_type == -1)
  {
// This is hack to make equipment load more often than weapons. Aug09 -Lucrot
    
    if(number(0, 2))
      slot = number(0, 70); // Worn eq
    else
      slot = number(0, MAX_SLOT - 1); // Anything
  }
  else
  {
    slot = BOUNDED(0, object_type, MAX_SLOT - 1);
  }

  if (slot_data[slot].wear_bit == ITEM_WIELD)
    obj = read_object(RANDOM_EQ_VNUM + 2, VIRTUAL);
  else
    obj = read_object(RANDOM_EQ_VNUM, VIRTUAL);

  if (!obj)
    return NULL;

  prefix = number(0, MAXPREFIX);

   /* make sure you understand this value before ya change it.  */
  howgood = (int) ((GET_LEVEL(killer) + GET_LEVEL(mob)) / 2.0);
  howgood = (int) (howgood * (get_property("random.drop.modifier.quality", 80.000) / 100.0));
  
  // if(IS_PC(killer))
  // {
      // howgood = howgood + (killer->only.pc->frags / 200);  // the rewards for pvp increase...
  // }
      
  if (material_type == -1)
    material = BOUNDED(1, number(MIN(GET_LEVEL(killer) / 3, howgood), howgood), MAXMATERIAL);
  else
    material = BOUNDED(1, material_type, MAXMATERIAL);

  zone = &zone_table[world[killer->in_room].zone];

  sprintf(buf_temp, "%s", strip_ansi(zone->name).c_str());

  ansi_n = strcmp(buf_temp, zone->name);


  if (IS_PC(killer))
    sprintf(buf_temp, "%s", GET_NAME(killer));
  else
    sprintf(buf_temp, "%s", mob->player.short_descr);

    /*      Set the items name!   */
  
  if (!number(0, (int) (400 / (GET_LEVEL(killer) + GET_C_LUCK(killer) + GET_LEVEL(mob)))) && \
       material_type == -1 && ansi_n && (IS_PC(killer) || mob_index[GET_RNUM(killer)].func.mob != shop_keeper))
  {     
    sprintf(buf1, "random _noquest_ %s %s %s %s", strip_ansi(prefix_data[prefix].m_name).c_str(),
            strip_ansi(material_data[material].m_name).c_str(),
            strip_ansi(slot_data[slot].m_name).c_str(), strip_ansi(zone->name).c_str());
    if (slot_data[slot].m_name[strlen(slot_data[slot].m_name) - 3] == 's')
    {
      sprintf(buf2, "some %s %s %s &+rfrom %s&n", prefix_data[prefix].m_name,
              material_data[material].m_name, slot_data[slot].m_name,
              zone->name);
      sprintf(buf3, "Some %s %s %s &+rfrom %s&n lie here on the ground.",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, zone->name);
    }
    else
    {
      sprintf(buf2, "%s %s %s %s &+rfrom %s&n",
              VOWEL(prefix_data[prefix].m_name[3]) ? "an" : "a",
	      prefix_data[prefix].m_name,
              material_data[material].m_name, slot_data[slot].m_name,
              zone->name);
      sprintf(buf3, "%s %s %s %s &+rfrom %s&n lies here on the ground.",
              VOWEL(prefix_data[prefix].m_name[3]) ? "An" : "a",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, zone->name);
    }
    /*  Zone named items get a higher chance to get multiple affects set */
    obj = setsuffix_obj_new(obj);
    while (!number(0, 19))
    {
      obj = setsuffix_obj_new(obj);
    }
    if(!number(0, 2)) //  set values for a proc(should the item turn out to be a weapon)
    {
       obj->value[6] = BOUNDED(1, (GET_LEVEL(mob) / number(1, 2)) + number(-5, 5), 60);
       obj->value[7] = spells_data[BOUNDED(0, GET_LEVEL(mob) - number(0, 6), 60)].s_number;
       should_have_weapon_proc = 1;
    }
  }
  else if (!number(0, 9) && material_type == -1 && GET_LEVEL(killer) > 45)
  {
/*     sprintf(buf1, "random _noquest_ %s %s %s", strip_ansi(prefix_data[prefix].m_name).c_str(),
            strip_ansi(material_data[material].m_name).c_str(),
            strip_ansi(slot_data[slot].m_name).c_str()); */
    if (slot_data[slot].m_name[strlen(slot_data[slot].m_name) - 3] == 's')
    {
      sprintf(buf2, "some %s %s %s&n", prefix_data[prefix].m_name,
              material_data[material].m_name, slot_data[slot].m_name);
      sprintf(buf3, "Some %s %s %s&n lie here on the ground.",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name);
    }
    else
    {
      sprintf(buf2, "%s %s %s %s&n",
              VOWEL(prefix_data[prefix].m_name[3]) ? "an" : "a",
              prefix_data[prefix].m_name,
              material_data[material].m_name, slot_data[slot].m_name);
      sprintf(buf3, "%s %s %s %s&n lies here on the ground.",
              VOWEL(prefix_data[prefix].m_name[3]) ? "An" : "A",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name);
    }
    sprintf(buf1, "random _noquest_ %s %s %s %s", strip_ansi(prefix_data[prefix].m_name).c_str(),
            strip_ansi(material_data[material].m_name).c_str(),
            strip_ansi(slot_data[slot].m_name).c_str(), strip_ansi(buf_temp).c_str());

    if(!number(0, 9))
    {
      obj = setsuffix_obj_new(obj);
      int sufcount = 0; // to ensure that we don't infinite loop if difficulty isn't set
      while (!number(0, zone->difficulty + 2) && sufcount < 2)  // zone difficulty generally 1, harder zones might be 2
      {
        obj = setsuffix_obj_new(obj);
        sufcount++; 
      }
    }
  }
  else if(!number(0, BOUNDED(1, 150 - GET_CHAR_SKILL(killer, SKILL_CRAFT) - (GET_C_LUCK(killer) - 90), 150)) &&
         material_type != -1)
  {
    sprintf(buf1, "random _noquest_ %s %s %s %s", strip_ansi(prefix_data[prefix].m_name).c_str(),
            strip_ansi(material_data[material].m_name).c_str(),
            strip_ansi(slot_data[slot].m_name).c_str(), strip_ansi(buf_temp).c_str());
    if (slot_data[slot].m_name[strlen(slot_data[slot].m_name) - 3] == 's')
    {
      sprintf(buf2, "some %s %s %s crafted by &+r%s&n",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, buf_temp);
      sprintf(buf3,
              "Some %s %s %s crafted by &+r%s&n lie here on the ground.",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, buf_temp);
    }
    else
    {
      sprintf(buf2, "%s %s %s %s crafted by &+r%s&n",
              VOWEL(prefix_data[prefix].m_name[3]) ? "an" : "a",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, buf_temp);
      sprintf(buf3, "%s %s %s %s crafted by &+r%s&n lies here on the ground.",
              VOWEL(prefix_data[prefix].m_name[3]) ? "An" : "A",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, buf_temp);
    }
    obj = setsuffix_obj_new(obj);
    while (!number(0, 19))
    {
      obj = setsuffix_obj_new(obj);
    }
    should_have_weapon_proc = 1;
  }
  else
  {
    sprintf(buf1, "random _noquest_ %s %s %s", strip_ansi(prefix_data[prefix].m_name).c_str(),
            strip_ansi(material_data[material].m_name).c_str(),
            strip_ansi(slot_data[slot].m_name).c_str());
    if (slot_data[slot].m_name[strlen(slot_data[slot].m_name) - 3] == 's')
    {
      sprintf(buf2, "some %s %s %s&n", prefix_data[prefix].m_name,
              material_data[material].m_name, slot_data[slot].m_name);
      sprintf(buf3, "Some %s %s %s&n lie here on the ground.",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name);
    }
    else
    {
      sprintf(buf2, "%s %s %s %s&n",
              VOWEL(prefix_data[prefix].m_name[3]) ? "an" : "a",
              prefix_data[prefix].m_name,
              material_data[material].m_name, slot_data[slot].m_name);
      sprintf(buf3, "%s %s %s %s&n lies here on the ground.",
              VOWEL(prefix_data[prefix].m_name[3]) ? "An" : "A",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name);
    }
  }

  set_keywords(obj, buf1);
  set_short_description(obj, buf2);
  set_long_description(obj, buf3);

  /*         Setbits        */

  SET_BIT(obj->wear_flags, ITEM_TAKE);
  SET_BIT(obj->wear_flags, slot_data[slot].wear_bit);

  obj->weight =
    (int) (slot_data[slot].numb_material * prefix_data[prefix].m_stat *
           slot_data[slot].m_ac + number(0, 1));

  if(IS_SET(obj->wear_flags, ITEM_WEAR_SHIELD))
    obj->weight = (obj->weight * number(2, 3));

  /*        Item Attributes    */
  if (1)
  {
    value =
      (int) (material_data[material].m_stat * prefix_data[prefix].m_stat *
             slot_data[slot].m_stat + number(0, 20));
    obj = setprefix_obj(obj, value / 18, 0);
  }
  if (!number(0, 2) && GET_LEVEL(killer) > 35)
  {
    value =
      (int) (material_data[material].m_stat * prefix_data[prefix].m_stat *
             slot_data[slot].m_stat + number(0, 20));
    obj = setprefix_obj(obj, value / 19, 1);
  }
  if (!number(0, 5) && GET_LEVEL(killer) > 49)
  {
    value =
      (int) (material_data[material].m_stat * prefix_data[prefix].m_stat *
             slot_data[slot].m_stat + number(0, 20));
    obj = setprefix_obj(obj, value / 19, 2);
  }

//obj->affected[4].location = APPLY_AC;
//obj->affected[4].modifier = 0 - (int) material_data[material].m_ac * prefix_data[prefix].m_ac * slot_data[slot].m_ac;

  obj->material = material_data[material].m_number;

  convertObj(obj);
  material_restrictions(obj);
  
  if(isname("unique", obj->name))
  {
    obj->craftsmanship = OBJCRAFT_HIGHEST;
  }

  if (isname("quiver", obj->name))
  {
    GET_ITEM_TYPE(obj) = ITEM_QUIVER;
    obj->value[0] = (int) (material_data[material].m_stat * number(80, 120));
    obj->value[1] = number(0, 1);
    obj->value[2] = 1;
    obj->value[3] = 0;
  }
  else if (IS_SET(obj->wear_flags, ITEM_WEAR_SHIELD))
  {
    GET_ITEM_TYPE(obj) = ITEM_SHIELD;
    obj->value[0] = number(1, 2);
    obj->value[1] = number(1, 8);
    obj->value[2] = number(1, 5);
    obj->value[3] = (int) (material_data[material].m_stat * number(90, 120));
    obj->value[4] = number(1, 5);
    obj->value[5] = number(0, 1);
  }
  else if(isname("robe", obj->name))
  {
    obj->value[0] = (int) (material_data[material].m_stat * number(90, 120));
    obj->value[1] = 0;
    obj->value[2] = 0;
    obj->value[3] = 1;
  }
  else if ( IS_SET(obj->wear_flags, ITEM_WIELD) && 
            !isname("longbow", obj->name) )
  {

    obj->value[0] = slot_data[slot].damage_type;
    obj->type = ITEM_WEAPON;
   
    if (slot_data[slot].numb_material > 2)
      obj->extra_flags |= ITEM_TWOHANDS;

    obj->value[1] =
      (int) (1 +
             (material_data[material].m_stat * prefix_data[prefix].m_stat *
              slot_data[slot].m_ac + number(0, 20)) / 20);
    
    obj->value[2] =
      (int) (BOUNDED (4, 
                     MIN((int)
                       (1 + (material_data[material].m_stat * prefix_data[prefix].m_stat * slot_data[slot].m_ac + number(0, 20)) / 20),
                       ((obj->extra_flags & ITEM_TWOHANDS) ? 32 : 24) / obj->value[1]),
                     10));
    bonus =
      (int) ((material_data[material].m_stat * prefix_data[prefix].m_stat *
              slot_data[slot].m_stat) / 18 + 1);
    obj->affected[0].location = APPLY_DAMROLL;
    obj->affected[0].modifier = bonus + number(0, 1);
    obj->affected[1].location = APPLY_HITROLL;
    obj->affected[1].modifier = bonus + number(0, 1);

    if (!(obj->extra_flags & ITEM_TWOHANDS))
    {
      obj->affected[0].modifier = (sbyte) (obj->affected[0].modifier * 0.7);
      obj->affected[1].modifier = (sbyte) (obj->affected[1].modifier * 0.7);
    }
 
    if (IS_BACKSTABBER(obj))
    {
      obj->value[1] = BOUNDED(1, (int) (obj->value[1] / 1.5), 3);
      obj->value[2] = BOUNDED(4, (int) (obj->value[2] * 1.6) + number(0,2) , obj->value[1] == 1 ? 12 : 8);
    }    

    if(isname("lance", obj->name))
    {
      set_keywords(obj, "lance");
      obj->value[1] = 1;
      obj->value[2] = BOUNDED(6, (int) (obj->value[2] * 1.6) + number(2,4) , obj->value[1] == 1 ? 13 : 9);
    }

    if (should_have_weapon_proc)
    {
      int      splnum;
      int tries = 0;
      do
      {
        splnum = BOUNDED(0, (BOUNDED(1, GET_LEVEL(mob), 60) - number(0, 15)), 60);
        tries++;
      }
      while (spells_data[splnum].self_only && tries < 100);
      
      if( tries < 100 )
      {
        obj->value[5] = spells_data[splnum].spell;
        obj->value[6] = number(15, MAX(20, GET_LEVEL(mob) - 10));
        obj->value[7] = number(45, 60);        
      }
    }

    if(number(0, 2))
      SET_BIT(obj->extra2_flags, ITEM2_MAGIC);

  }
  else if (isname("longbow", obj->name))
  {
    obj->type = ITEM_FIREWEAPON;
    obj->value[3] = 1;

    if (slot_data[slot].numb_material > 2)
      obj->extra_flags |= ITEM_TWOHANDS;

    obj->value[0] = (int) (1 + (material_data[material].m_stat * prefix_data[prefix].m_stat *
                      slot_data[slot].m_ac + number(0, 40)));
    obj->value[1] = (int) (1 + (material_data[material].m_stat * prefix_data[prefix].m_stat *
                      slot_data[slot].m_ac + number(0, 40)));

    bonus = (int) ((material_data[material].m_stat * prefix_data[prefix].m_stat *
                      slot_data[slot].m_stat) / 20 + 1);

    obj->affected[0].location = APPLY_DAMROLL;
    obj->affected[0].modifier = bonus + number(0, 2);
    obj->affected[1].location = APPLY_HITROLL;
    obj->affected[1].modifier = bonus + number(0, 2);   
  }
  else
  {
    GET_ITEM_TYPE(obj) = ITEM_ARMOR;
    obj->value[0] =
      (int) (material_data[material].m_ac * prefix_data[prefix].m_ac *
             slot_data[slot].m_ac);
  }

  /*  if object has 2 identical locations for mod0 and mod1, 
   *  we refigure a new mod location for mod1 and just pray mod2
   *  doesn't come out the same as mod0 and mod1 and makes some
   *  ridiculous +7dam belt :) - Jexni 11/01/08
  */  
// Getting some extremely high and low random values (e.g. +200 damage belts) Nov08 -Lucrot
  // sbyte quickmod;
  // if(obj->affected[0].location == obj->affected[1].location && obj->affected[0].location != APPLY_NONE)
  // {
    // obj->affected[0].modifier = (obj->affected[0].modifier + obj->affected[1].modifier) / 2;
    // obj->affected[0].modifier = (sbyte) ((int) obj->affected[0].modifier * material_data[material].m_stat);
    // obj->affected[1].location = BOUNDED(APPLY_LOWEST, obj->affected[1].location + number(-3, 3), APPLY_LUCK_MAX);
    // while(BAD_APPLYS(obj->affected[1].location) && (obj->affected[1].location > APPLY_NONE && obj->affected[1].location < APPLY_LUCK_MAX))
    // {
      // obj->affected[1].location += number(-2, 2);
    // } 
    // quickmod = (sbyte) ((int) number(3, 5) * material_data[material].m_stat);
    // if(obj->affected[1].location > 19 && obj->affected[1].location < 25)
      // quickmod = -quickmod;
    // obj->affected[1].modifier = quickmod;
  // }
  
  return obj;

}

P_obj setsuffix_obj_new(P_obj obj)
{
  switch (dice(5,5))
  {
  case 3:
    SET_BIT(obj->bitvector, AFF_DETECT_INVISIBLE);
    break;
  case 4:
    SET_BIT(obj->bitvector2, AFF2_ULTRAVISION);
    break;
  case 5:
    SET_BIT(obj->bitvector2, AFF2_FIRESHIELD);
    break;
  case 6:
    SET_BIT(obj->bitvector, AFF_SLOW_POISON);
    break;
  case 7:
    SET_BIT(obj->bitvector, AFF_WATERBREATH);
    break;
  case 8:
    SET_BIT(obj->bitvector, AFF_INFRAVISION);
    break;
  case 9:
    SET_BIT(obj->bitvector, AFF_PROTECT_GOOD);
    break;
  case 10:
    SET_BIT(obj->bitvector, AFF_PROTECT_EVIL);
    break;
  case 11:
    SET_BIT(obj->bitvector2, AFF2_PROT_COLD);
    break;
  case 12:
    SET_BIT(obj->bitvector, AFF_LEVITATE);
    break;
  case 13:
    SET_BIT(obj->bitvector2, AFF2_DETECT_EVIL);
    break;
  case 14:
    SET_BIT(obj->bitvector2, AFF2_DETECT_GOOD);
    break;
  case 15:
    SET_BIT(obj->bitvector2, AFF2_DETECT_MAGIC);
    break;
  case 16:
    SET_BIT(obj->bitvector, AFF_PROT_FIRE);
    break;
  case 17:
    SET_BIT(obj->bitvector5, AFF5_PROT_UNDEAD);
    break;
  case 18:
    SET_BIT(obj->bitvector2, AFF2_PROT_ACID);
    break;
  case 19:
    SET_BIT(obj->bitvector, AFF_SENSE_LIFE);
    break;
  case 20:
    SET_BIT(obj->bitvector, AFF_MINOR_GLOBE);
    break;
  case 21:
    SET_BIT(obj->bitvector, AFF_FARSEE);
    break;
  case 22:
    SET_BIT(obj->bitvector3, AFF3_COLDSHIELD);
    break;
  case 23:
    SET_BIT(obj->bitvector2, AFF2_SOULSHIELD);
    break;
  case 24:
    SET_BIT(obj->bitvector, AFF_BARKSKIN);
    break;
  case 25:
    SET_BIT(obj->bitvector, AFF_HASTE);
    break;
  default:
    break;
  }
  return obj;
}


P_obj setprefix_obj(P_obj obj, int modifier, int affectnumber)
{

  switch (number(0, 30))
  {
  case 0:
    obj->affected[affectnumber].location = APPLY_HITROLL;
    modifier = (int) (modifier * 1.2);
    break;
  case 1:
    obj->affected[affectnumber].location = APPLY_STR;
    modifier = (int) (modifier * 2);	 
    break;
  case 2:
    obj->affected[affectnumber].location = APPLY_DEX;
    modifier = (int) (modifier * 2);
    break;
  case 3:
    obj->affected[affectnumber].location = APPLY_INT;
    modifier = (int) (modifier * 2);
    break;
  case 4:
    obj->affected[affectnumber].location = APPLY_WIS;
    modifier = (int) (modifier * 2);
    break;
  case 5:
    obj->affected[affectnumber].location = APPLY_CON;
    modifier = (int) (modifier * 2);
    break;
  case 6:
  /*  obj->affected[affectnumber].location = APPLY_MANA;
    modifier = (8 * modifier);
    break;*/
  case 7:
    obj->affected[affectnumber].location = APPLY_HIT;
    modifier = (8 * modifier);
    break;
  case 8:
    obj->affected[affectnumber].location = APPLY_MOVE;
    modifier = (11 * modifier);
    break;
  case 9:
    obj->affected[affectnumber].location = APPLY_DAMROLL;
    break;
  case 10:
    obj->affected[affectnumber].location = APPLY_HITROLL;
    break;
  case 11:
    obj->affected[affectnumber].location = APPLY_DAMROLL;
    break;
  case 12:
    obj->affected[affectnumber].location = APPLY_SAVING_PARA;
    modifier = (0 - modifier);
    break;
/*  case 13:
    obj->affected[affectnumber].location = APPLY_SAVING_ROD;
    modifier = (0 - modifier);
    break;*/
  case 13:
    obj->affected[affectnumber].location = APPLY_SAVING_FEAR;
    modifier = (0 - modifier);
    break;
  case 14:
    obj->affected[affectnumber].location = APPLY_SAVING_BREATH;
    modifier = (0 - modifier);
    break;
  case 15:
    obj->affected[affectnumber].location = APPLY_SAVING_SPELL;
    modifier = (0 - modifier);
    break;
  case 16:
    obj->affected[affectnumber].location = APPLY_AGI;
    modifier = (int) (modifier * 2);
    break;
  case 17:
    obj->affected[affectnumber].location = APPLY_POW;
    modifier = (int) (modifier * 2);
    break;
  case 18:
    obj->affected[affectnumber].location = APPLY_LUCK;
    modifier = (int) (modifier * 2);
    break;
  case 19:
    obj->affected[affectnumber].location = APPLY_STR_MAX;
    modifier = (int) (modifier * 0.8);
    break;
  case 20:
    obj->affected[affectnumber].location = APPLY_DEX_MAX;
    break;
  case 21:
    obj->affected[affectnumber].location = APPLY_INT_MAX;
    modifier = (int) (modifier * 0.8);
    break;
  case 22:
    obj->affected[affectnumber].location = APPLY_WIS_MAX;
    modifier = (int) (modifier * 0.8);
    break;
  case 23:
    obj->affected[affectnumber].location = APPLY_CON_MAX;
    break;
  case 24:
    obj->affected[affectnumber].location = APPLY_AGI_MAX;
    break;
  case 25:
    obj->affected[affectnumber].location = APPLY_POW_MAX;
    break;
  case 26:
    obj->affected[affectnumber].location = APPLY_LUCK_MAX;
    break;
  case 27:
  case 28:
  case 29:
  case 30:
    obj->affected[affectnumber].location = APPLY_HIT;
    modifier = (8 * modifier);
    break;
  default:
    obj->affected[affectnumber].location = APPLY_HIT;
    modifier = (8 * modifier);
    break;
  }
  if(modifier == 0)
  obj->affected[affectnumber].location = APPLY_NONE;
    
  obj->affected[affectnumber].modifier = modifier;
  return obj;
}

int random_eq_proc(P_obj obj, P_char ch, int cmd, char *argument)
{

  P_char   kala;
  int      j;
  int      dam = cmd / 1000;
  P_obj    t_obj;
  char    *arg;
  long     curr_time;
  int      numNamed = 0;
  int      wear_order[] =
    { 41, 24, 40, 6, 19, 21, 22, 20, 39, 3, 4, 5, 35, 37, 12, 27, 23, 13, 28,
    29, 30, 10, 31, 11, 14, 15, 33, 34, 9, 32, 1, 2, 16, 17, 25, 26, 7, 36, 8,
      38, -1
  };
  int      chance = 0;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_MELEE_HIT || cmd == CMD_GOTHIT || cmd == CMD_GOTNUKED)
  {;
  }
  else
    return FALSE;

  if (cmd == CMD_MELEE_HIT)
    chance = 25;

  if (cmd == CMD_GOTHIT)
    chance = 100;

  if (cmd == CMD_GOTNUKED)
    chance = 50;

  if (!ch)
    return (FALSE);

  if (!OBJ_WORN(obj) || (obj->loc.wearing != ch))
    return (FALSE);


  kala = (P_char) arg;

  if (!kala)
    return FALSE;

  curr_time = time(NULL);
//obj->value[6] = GET_VNUM(mob);
//obj->value[7] = named_spells[1];


//wizlog(56, "%d, %d, diff=%d" , obj->timer[0], curr_time,  (obj->timer[0] - curr_time)   );

  if (obj->timer[0] + 15 <= curr_time)
  {
    for (j = 0; wear_order[j] != -1; j++)
    {
      if (ch->equipment[wear_order[j]])
      {
        t_obj = ch->equipment[wear_order[j]];

        if (obj->value[6] == t_obj->value[6] && t_obj->value[7] != 0)
          numNamed++;

      }

    }


    if ((numNamed > 2 &&
         IS_FIGHTING(ch) &&
         !number(0, chance)) ||
         (cmd == CMD_MELEE_HIT &&
         obj->value[6] == 999 &&
         IS_SET(obj->wear_flags, ITEM_WIELD) &&
         !number(0, 10) &&
         IS_FIGHTING(ch)))
    {
      kala = ch->specials.fighting;;
      act
        ("&+B$n's $q &+rpu&+Rls&+rat&+Res &+Lwith &+be&+Bn&+Wer&+Bg&+by &+Lfor a &+rmoment&+L...&N",
         TRUE, ch, obj, kala, TO_NOTVICT);
      act
        ("&+BYour $q &+rpu&+Rls&+rat&+Res &+Lwith &+be&+Bn&+Wer&+Bg&+by &+Lfor a &+rmoment&+L...&N",
         TRUE, ch, obj, kala, TO_CHAR);
      act
        ("&+B$n's $q &+rpu&+Rls&+rat&+Res &+Lwith &+be&+Bn&+Wer&+Bg&+by &+Lfor a &+rmoment&+L...&N",
         TRUE, ch, obj, kala, TO_VICT);
      numNamed = obj->value[7];
      if (spells_data[numNamed].self_only)
        kala = ch;

      if (numNamed > 0)
      {
        ((*skills[spells_data[numNamed].spell].spell_pointer) ((int) 50, ch,
                                                               0,
                                                               SPELL_TYPE_SPELL,
                                                               kala, obj));
        //wizlog(56, "%d", numNamed);
        //wizlog(56, "SPell number:%d", spells_data[numNamed].spell);
        //wizlog(56, "List number:%d", spells_data[numNamed].s_number);
        //wizlog(56, "self only:%d", spells_data[numNamed].self_only);
        obj->timer[0] = curr_time;
      }
      return FALSE;
    }


  }

  return FALSE;
}

P_obj set_enchant_affect(P_obj obj, int proc)
{
  switch (proc)
  {
  case SPELL_ACID_BLAST:
    SET_BIT(obj->bitvector2, AFF2_PROT_GAS);
    SET_BIT(obj->bitvector2, AFF2_PROT_ACID);
    obj->value[6] = 1;
    break;
  case SPELL_FIREBALL:
    obj->value[6] = 2;
    SET_BIT(obj->bitvector, AFF_PROT_FIRE);
    break;
  case SPELL_MAGIC_MISSILE:
    obj->value[6] = 3;
    SET_BIT(obj->bitvector, AFF_PROTECT_GOOD);
    break;
  case SPELL_CHILL_TOUCH:
    obj->value[6] = 4;
    SET_BIT(obj->bitvector2, AFF2_PROT_COLD);
    break;
  case SPELL_NEGATIVE_CONCUSSION_BLAST:
    obj->value[6] = 5;
    SET_BIT(obj->bitvector, AFF_FARSEE);
    break;
  case SPELL_SHOCKING_GRASP:
    obj->value[6] = 6;
    SET_BIT(obj->bitvector, AFF_FLY);
    break;
  case SPELL_FLAMESTRIKE:
    obj->value[6] = 7;
    SET_BIT(obj->bitvector, AFF_SENSE_LIFE);
    break;
  case SPELL_BLINDNESS:
    obj->value[6] = 8;
    SET_BIT(obj->bitvector, AFF_DETECT_INVISIBLE);
    break;
  case SPELL_ENERGY_DRAIN:
    obj->value[6] = 9;
    SET_BIT(obj->bitvector, AFF2_VAMPIRIC_TOUCH);
    break;
  }
}
P_obj set_encrust_affect(P_obj obj, int proc)
{

  switch (proc)
  {
  case SPELL_ACID_BLAST:
    SET_BIT(obj->bitvector2, AFF2_PROT_GAS);
    SET_BIT(obj->bitvector2, AFF2_PROT_ACID);
    break;
  case SPELL_FIREBALL:
    SET_BIT(obj->bitvector, AFF_PROT_FIRE);
    break;
  case SPELL_MAGIC_MISSILE:
    SET_BIT(obj->bitvector, AFF_PROTECT_GOOD);
    break;
  case SPELL_CHILL_TOUCH:
    SET_BIT(obj->bitvector2, AFF2_PROT_COLD);
    break;
  case SPELL_NEGATIVE_CONCUSSION_BLAST:
    SET_BIT(obj->bitvector, AFF_MINOR_GLOBE);
    break;
  case SPELL_SHOCKING_GRASP:
    SET_BIT(obj->bitvector, AFF_SLOW_POISON);
    break;
  case SPELL_FLAMESTRIKE:
    SET_BIT(obj->bitvector, AFF_SENSE_LIFE);
    break;
  case SPELL_BLINDNESS:
    SET_BIT(obj->bitvector2, AFF2_DETECT_MAGIC);
    break;
  case SPELL_ENERGY_DRAIN:
    SET_BIT(obj->bitvector, AFF_HASTE);
    break;
  }

}

bool identify_random(P_obj obj)
{
  char buffer[256];
  char old_name[256];
  char *c;
  int i;

  if (!obj->value[5])
    return false;

  for (i = 0; i < 62 && spells_data[i].spell != obj->value[5]; i++)
    ;

  if (i == 62 || !spells_data[i].name)
    return false;
  
  strcpy(old_name, obj->short_description);
  if (c = strstr(old_name, " of "))
    *c = '\0';
  if (c = strstr(old_name, " crafted by"))
    *c = '\0';

  sprintf(buffer, "%s of %s&n", old_name, spells_data[i].name);

  if (!strcmp(buffer, obj->short_description))
    return false;

  if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
    FREE(obj->short_description);

  obj->short_description = str_dup(buffer);

  obj->str_mask |= STRUNG_DESC2;

  return true;
}

/*
int stone_spell_list[] =
{
  0,
  SPELL_ACID_BLAST,
  SPELL_FIREBALL,
  SPELL_MAGIC_MISSILE,
  SPELL_CHILL_TOUCH,
  SPELL_NEGATIVE_CONCUSSION_BLAST,
  SPELL_SHOCKING_GRASP,
  SPELL_FLAMESTRIKE,
  SPELL_BLINDNESS,
  SPELL_ENERGY_DRAIN,
  0
};

*/
