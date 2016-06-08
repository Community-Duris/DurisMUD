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
#include "vnum.obj.h"
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
  float    weight_mod;
};

// Going with the terminator -1 for searches.  Just in case someone f's up the count.
#define NUM_HIGHDROP_MOBS 39
const int highdrop_mobs[NUM_HIGHDROP_MOBS+1] = {
  19700, //  0 Tiamat
  25040, //  1 Brass Sultan
  25700, //  2 Bahamut
  81454, //  3 Chronomancer
  35523, //  4 Dark Knight
  38317, //  5 Lady Death
  44172, //  6 King Arkan'non
  26642, //  7 the Dark
  3524,  //  8 Tyrlos
  45565, //  9 Malevolence
  36856, // 10 Kyeril
  25110, // 11 Brass Padashaw
  21672, // 12 King of Aravne
  98959, // 13 Bard Faust
  87741, // 14 snogres pit fiend
  87700, // 15 snogres berserker
  87709, // 16 snogres champion
  87729, // 17 snogres chieftain
  38737, // 18 Ny'Neth
  70941, // 19 Kithron
  88316, // 20 Kossuth
  87561, // 21 Zuggtmoy
  87544, // 22 Jubilex
  87613, // 23 Graz'zt
  87612, // 24 Lolth
  32637, // 25 Aramus
  16205, // 26 Aceralde
  43576, // 27 Eligoth
  32420, // 28 Bel
  58835, // 29 Chenovog
  71259, // 30 Yeenoghu
  78439, // 31 Kyzastaxkasis
  58379, // 32 Obox-ob
  58383, // 33 Strahd
  2435,  // 34 Llamanby
  15113, // 35 Dranum
  91031, // 36 Doru
  80538, // 37 Skrentherlog
  8746,  // 38 Crymson
  -1     // 39 == NUM_HIGHDROP_MOBS + 1
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

// Number, Name, Stat (material quality), AC - ac mod.
// Weight mods were pulled from http://www.aqua-calc.com/calculate/volume-to-weight .. using the kilogram.
struct randomeq_material material_data[MAXMATERIAL + 1] =
{
// m_number                                       m_stat  weight_mod
//                    m_name                           m_ac
  {MAT_CLOTH,         "cloth",                    12,  1, 0.15},  // 0
  {MAT_SOFTWOOD,      "&+ysoftwood&n",            12,  3, 0.30},
  {MAT_HARDWOOD,      "&+yhardwood&n",            14,  4, 0.35},
  {MAT_BAMBOO,        "&+ybamboo&n",              13,  3, 0.25},
  {MAT_REEDS,         "&+yreed&n",                13,  3, 0.27},
  {MAT_BARK,          "&+ybark&n",                15,  6, 0.29},  // 5
  {MAT_SILICON,       "&+Wglass&n",               16,  2, 0.38},
  {MAT_RUBBER,        "&+Lrubber&n",              10,  3, 0.22},
  {MAT_CERAMIC,       "&+Lclay&n",                17,  4, 0.57},
  {MAT_HIDE,          "&+yhide&n",                18,  5, 0.30},
  {MAT_LIMESTONE,     "&+glimestone&n",           19,  3, 0.56},  // 10
  {MAT_COPPER,        "&+ycopper&n",              20,  6, 2.12},
  {MAT_LEATHER,       "&+yleather",               21,  5, 0.32},
  {MAT_CURED_LEATHER, "&+ycured leather&n",       25,  6, 0.35},
  {MAT_HEMP,          "&+ghemp&n",                21,  5, 0.29},
  {MAT_BRONZE,        "&+ybronze&n",              21,  6, 2.06},  // 15
  {MAT_BONE,          "&+wbone&n",                22,  7, 0.35},
  {MAT_BRASS,         "&+ybrass&n",               23,  7, 1.99},
  {MAT_IVORY,         "&+Wivory&n",               24,  5, 0.60},
  {MAT_GRANITE,       "&+wgranite&n",             25,  9, 0.39},
  {MAT_MARBLE,        "&+Wma&+wr&+Lb&+wl&+We&n",  25,  7, 0.64},  // 20
  {MAT_STEEL,         "&nsteel",                  30, 12, 1.89},
  {MAT_IRON,          "&+ciron&n",                30,  8, 1.22},
  {MAT_OBSIDIAN,      "&+Lobsidian&n",            30, 10, 0.42},
  {MAT_EMERALD,       "&+Gemerald&n",             34,  7, 0.72},
  {MAT_CRYSTAL,       "&+Ccrystal&n",             36,  7, 0.70},  // 25
  {MAT_GEM,           "&+Rgem&n",                 36,  7, 0.70},
  {MAT_STONE,         "&+Lstone&n",               38, 12, 0.75},
  {MAT_FLESH,         "&+rflesh&n",               38,  5, 0.30},
  {MAT_GOLD,          "&+Ygold&n",                38,  9, 4.57},
  {MAT_SAPPHIRE,      "&+bsapphire&n",            38,  7, 1.07},  // 30
  {MAT_SILVER,        "&+Wsilver&n",              40, 10, 2.21},
  {MAT_TIN,           "&+Ctin&n",                 25, 15, 1.72},
  {MAT_RUBY,          "&+rruby&n",                42,  9, 1.08},
  {MAT_DIAMOND,       "&+Wdiamond&n",             43, 14, 0.95},
  {MAT_ELECTRUM,      "&+Welectrum&n",            44, 12, 2.23},  // 35
  {MAT_ADAMANTIUM,    "&+madamantium&n",          50, 32, 5.00},
  {MAT_MITHRIL,       "&+Cmithril&n",             46, 15, 0.65},
  {MAT_PLATINUM,      "&+Wplatinum&n",            46, 13, 1.26},
  {MAT_GLASSTEEL,     "&+cglassteel&n",           40, 18, 1.14},
  {MAT_REPTILESCALE,  "&+greptile scale&n",       30, 15, 0.55},  // 40
  {MAT_CHITINOUS,     "&+ychitin&n",              25, 20, 0.65},
  {MAT_DRAGONSCALE,   "&+rdragon&+Lscale&n",      65, 25, 0.85},  // 42
  {}
};

// m_number                                 m_stat      weight_mod
//    m_name                                      m_ac
struct randomeq_prefix prefix_data[MAXPREFIX + 1] = {
  {1, "&+Wm&+Ya&+Wg&+Yic&+Wal&n",           1.50, 1.50, 0.80},
  {2, "&+Lmaste&+brly-craf&+Lted&n",        1.47, 1.45, 0.85},
  {3, "&+Lsuperior&n",                      1.41, 1.25, 0.95},
  {4, "&+Yun&+Liq&+Yue&n",                  1.40, 1.10, 0.75},
  {5, "&+Cbeautiful&n",                     1.35, 1.00, 1.15},
  {6, "&+Lwell-cr&+ga&+Gf&+gt&+Led",        1.31, 1.40, 0.95},
  {7, "&+Cfro&+Wst-rim&+Ced&n",             1.26, 0.90, 1.15},
  {8, "&+Rfl&+Ya&+Wm&+Yi&+Rng",             1.25, 0.85, 0.95},
  {9, "&+Mhumming&n",                       1.23, 1.50, 1.05},
  {10, "&+Bicy&n",                          1.18, 0.82, 1.10},
  {11, "&+Cfrozen&n",                       1.16, 0.94, 1.12},
  {12, "&+Wfine&n",                         1.15, 1.20, 0.97},
  {13, "&+wgl&+Wowi&+wng&n",                1.14, 1.00, 1.02},
  {14, "&+gde&+Gc&+gent&n",                 1.00, 1.00, 0.99},
  {15, "&+wtarnished&n",                    1.09, 1.00, 1.02},
  {16, "&+Yg&+Wl&+Yitt&+We&+Yri&+Wn&+Yg&n", 1.05, 1.40, 1.05},
  {17, "&+ytwined&n",                       0.75, 0.80, 1.10},
  {18, "&+cdented&n",                       0.68, 0.90, 0.85},
  {19, "&+gpitted&n",                       0.62, 0.62, 0.80},
  {20, "&+wcrude&n",                        0.55, 0.70, 1.15},
  {21, "&+ycracked&n",                      0.50, 0.50, 1.00},
  {22, "&+Wexquisite&n",                    1.39, 1.40, 0.88},
  {23, "&+rrusty&n",                        0.70, 0.70, 1.05},
  {24, "&+Lspiked&n",                       1.20, 1.00, 1.15},
  {25, "&+Cfanged&n",                       1.30, 1.00, 1.10},
  {26, "&+cstylish&n",                      1.15, 1.05, 1.12},
  {27, "&+Ldiabolical&n",                   1.40, 1.40, 1.20},
  {28, "&+Rsparkling&n",                    1.40, 1.35, 1.15},
  {29, "&+Ytwisted&n",                      1.30, 1.20, 1.08},
  {30, "&+Yshi&+Wmme&+Yring&n",             1.30, 1.40, 0.95},
  {31, "&+whardened&n",                     1.24, 1.23, 1.03},
  {32, "&+Lgha&n&+wstly&n",                 0.95, 1.00, 0.95},
  {33, "&+Wan&+Cgel&n&+cic&n",              1.45, 1.35, 0.777},
  {34, "&+mdrow&+L-made&n",                 1.25, 1.00, 0.86},
  {35, "&+Chuman&+L-made&n",                0.90, 1.15, 1.00},
  {36, "&+Ydwarven&+L-made&n",              0.70, 0.90, 1.12},
  {37, "&+run&+Learthly&n",                 1.33, 1.30, 1.21},
  {38, "&+yskin&+L-wrapped",                0.60, 0.70, 1.02},
  {39, "&+Wje&+Cw&+Weled&n",                1.40, 1.30, 1.05},
  {40, "&+Lsinister&n",                     1.20, 1.20, 1.16},
  {41, "&+Wsta&n&+wtic&n",                  1.10, 1.20, 1.10},
  {42, "&+Cgle&n&+cam&+Wing&n",             1.22, 0.90, 1.02},
  {43, "&+wwell-used&n",                    0.60, 0.50, 0.98},
  {44, "&+Wleg&n&+wend&+Lary&n",            1.50, 1.40, 0.78},
  {45, "&+Wknight's&n",                     1.00, 1.11, 1.10},
  {46, "&+Lsold&n&+wier's&n",               0.80, 0.90, 1.12},
  {47, "&+Rbrutal&n",                       0.80, 1.40, 1.21},
  {48, "&+Cbor&+Weal&n",                    0.80, 0.80, 0.80},
  {49, "&+rbl&+Roo&+rdy&n",                 0.90, 1.20, 1.03},
  {50, "&+rblood-stained&n",                0.50, 0.60, 1.02},
  {51, "&+Wlight&n",                        0.80, 0.90, 0.90},
  {52, "&+Ldark&n",                         0.80, 0.90, 1.01},
  {53, "&+cfr&+Cagi&+cle&n",                1.00, 1.00, 0.89},
  {54, "&+rb&+yurn&+rt&n",                  1.20, 0.90, 0.98},
  {55, "&+mpatched&n",                      1.23, 1.20, 1.01},
  {56, "&+Rmisshaped&n",                    1.03, 1.00, 1.02},
  {57, "&+La&+ws&+Lh&+we&+Ln&n",            1.11, 1.20, 1.01}
};

struct random_spells spells_data[61] = {
//  s_number                      self_only
//     spell                         name
  { 0, SPELL_ICE_MISSILE,         FALSE, "&+Cice missiles"},
  { 1, SPELL_CURE_LIGHT,          TRUE},
  { 2, SPELL_CURE_BLIND,          TRUE},
  { 3, SPELL_MAGIC_MISSILE,       FALSE, "&+Ymagic missiles"},
  { 4, SPELL_SHOCKING_GRASP,      FALSE, "&+Bshocking"},
  { 5, SPELL_SLOW,                FALSE, "slowness"},
  { 6, SPELL_ARMOR,               TRUE},
  { 7, SPELL_CONTINUAL_LIGHT,     TRUE},
  { 8, SPELL_DISPEL_EVIL,         FALSE, "&+Wthe holy"},
  { 9, SPELL_BLESS,               TRUE},
  {10, SPELL_SLEEP,               FALSE, "&+Lsleep"},
  {11, SPELL_MINOR_GLOBE,         TRUE},
  {12, SPELL_DISPEL_GOOD,         FALSE, "the unholy"},
  {13, SPELL_FAERIE_FOG,          FALSE, "the faeries"},
  {14, SPELL_EGO_WHIP,            FALSE, "mindwhipping"},
  {15, SPELL_FLESH_ARMOR,         TRUE},
  {16, SPELL_WOLFSPEED,           TRUE},
  {17, SPELL_HAWKVISION,          TRUE},
  {18, SPELL_PYTHONSTING,         FALSE, "&+gthe snake"},
  {19, SPELL_FLAMEBURST,          FALSE, "&+Rflameburst"},
  {20, SPELL_SCORCHING_TOUCH,     FALSE, "&+Rscorching"},
  {21, SPELL_CURE_SERIOUS,        TRUE},
  {22, SPELL_MINOR_GLOBE,         TRUE},
  {23, SPELL_RAY_OF_ENFEEBLEMENT, FALSE, "feebleness"},
  {24, SPELL_DARKNESS,            TRUE},
  {25, SPELL_PWORD_STUN,          FALSE, "&+Wstunning"},
  {26, SPELL_BARKSKIN,            TRUE},
  {27, SPELL_ADRENALINE_CONTROL,  TRUE},
  {28, SPELL_BALLISTIC_ATTACK,    FALSE, "stones"},
  {29, SPELL_SOUL_DISTURBANCE,    FALSE, "&+Lsoul crushing"},
  {30, SPELL_MENDING,             TRUE},
  {31, SPELL_BEARSTRENGTH,        TRUE},
  {32, SPELL_SHADOW_MONSTER,      FALSE, "&+Lshadows"},
  {33, SPELL_COLD_WARD,           TRUE},
  {34, SPELL_MOLTEN_SPRAY,        FALSE, "molten rocks"},
  {35, SPELL_FLAMEBURST,          FALSE, "&+rflamebursts"},
  {36, SPELL_WITHER,              FALSE, "&+Lwithering"},
  {37, SPELL_MASS_INVIS,          TRUE},
  {38, SPELL_ICE_STORM,           FALSE, "&+cblizzards"},
  {39, SPELL_COLDSHIELD,          TRUE},
  {40, SPELL_FEEBLEMIND,          FALSE, "&+Lmindbreaking"},
  {41, SPELL_COLOR_SPRAY,         FALSE, "magespray"},
  {42, SPELL_FIRESHIELD,          TRUE},
  {43, SPELL_CHILL_TOUCH,         FALSE, "&+bchilling"},
  {44, SPELL_FIREBALL,            FALSE, "&+Rfireballs"},
  {45, SPELL_HEAL,                TRUE},
  {46, SPELL_POISON,              FALSE, "&+gvenom"},
  {47, SPELL_FEAR,                FALSE, "&+Lfear"},
  {48, SPELL_FLY,                 TRUE},
  {49, SPELL_COLOR_SPRAY,         FALSE, "magespray"},
  {50, SPELL_GLOBE,               TRUE},
  {51, SPELL_HARM,                FALSE, "harming"},
  {52, SPELL_ENLARGE,             TRUE},
  {53, SPELL_REDUCE,              TRUE},
  {54, SPELL_ELEPHANTSTRENGTH,    TRUE},
  {55, SPELL_DETONATE,            FALSE, "detonating"},
  {56, SPELL_CURSE,               FALSE, "&+ycurse"},
  {57, SPELL_STONE_SKIN,          TRUE},
  {58, SPELL_BIOFEEDBACK,         TRUE},
  {59, SPELL_IMMOLATE,            FALSE, "&+Rimmolating"},
  {60, SPELL_BLUR,                TRUE}
};

// MAX_SLOT found in config.h
extern const struct randomeq_slots slot_data[MAX_SLOT + 1];
const struct randomeq_slots slot_data[MAX_SLOT + 1] =
{
//   m_number                  m_stat      wear_bit                 damage_type - for weapons
//      m_name                       m_ac                        numb_material .. base_weight
  {  1, "&+Lring&N",           1.00, 0.00, ITEM_WEAR_FINGER,     1, 0,  1},
  {  2, "&+Lband&N",           1.10, 0.00, ITEM_WEAR_FINGER,     1, 0,  1},
  {  3, "&+Lsignet&n",         1.25, 0.00, ITEM_WEAR_FINGER,     1, 0,  1},
  {  4, "&+Lnecklace&N",       1.10, 0.00, ITEM_WEAR_NECK,       2, 0,  3},
  {  5, "&+Lcollar&N",         1.05, 1.00, ITEM_WEAR_NECK,       2, 0,  3},
  {  6, "&+Lchoker&N",         1.08, 0.50, ITEM_WEAR_NECK,       2, 0,  2},
  {  7, "&+Lchestplate&N",     1.30, 1.30, ITEM_WEAR_BODY,       3, 0, 25},
  {  8, "&+Lplatemail&N",      1.70, 1.70, ITEM_WEAR_BODY,       3, 0, 40},
  {  9, "&+Lringmail&N",       1.50, 1.45, ITEM_WEAR_BODY,       3, 0, 30},
  { 10, "&+Lrobe&N",           1.50, 0.70, ITEM_WEAR_BODY,       2, 0, 10},
  { 11, "&+Ltunic&n",          0.80, 0.50, ITEM_WEAR_BODY,       1, 0,  8},
  { 12, "&+Lhelmet&N",         1.05, 1.00, ITEM_WEAR_HEAD,       2, 0,  6},
  { 13, "&+Lhelm&N",           1.00, 1.00, ITEM_WEAR_HEAD,       2, 0,  5},
  { 14, "&+Lcrown&N",          1.06, 0.60, ITEM_WEAR_HEAD,       2, 0,  5},
  { 15, "&+Lhat&N",            1.00, 0.40, ITEM_WEAR_HEAD,       2, 0,  4},
  { 16, "&+Lskullcap&n",       1.20, 0.30, ITEM_WEAR_HEAD,       2, 0,  3},
  { 17, "&+Lleggings&N",       1.20, 1.00, ITEM_WEAR_LEGS,       2, 0,  8},
  { 18, "&+Lleg plates&N",     1.15, 1.10, ITEM_WEAR_LEGS,       2, 0, 10},
  { 19, "&+Lpants&N",          1.05, 0.80, ITEM_WEAR_LEGS,       2, 0,  7},
  { 20, "&+Lgreaves&n",        1.25, 1.50, ITEM_WEAR_LEGS,       3, 0,  6},
  { 21, "&+Lshoes&N",          1.00, 0.70, ITEM_WEAR_FEET,       2, 0,  3},
  { 22, "&+Lboots&N",          1.10, 1.00, ITEM_WEAR_FEET,       2, 0,  4},
  { 23, "&+Lmoccasins&n",      1.20, 0.70, ITEM_WEAR_FEET,       2, 0,  2},
  { 24, "&+Lgauntlets&N",      1.35, 1.00, ITEM_WEAR_HANDS,      2, 0,  6},
  { 25, "&+Ltalons&N",         1.20, 1.00, ITEM_WEAR_HANDS,      2, 0,  4},
  { 26, "&+Lgloves&N",         1.25, 1.10, ITEM_WEAR_HANDS,      2, 0,  3},
  { 27, "&+Lmitts&n",          0.90, 0.90, ITEM_WEAR_HANDS,      2, 0,  2},
  { 28, "&+Lsleeves&N",        1.30, 0.80, ITEM_WEAR_ARMS,       2, 0,  7},
  { 29, "&+Larm plates&N",     1.20, 1.20, ITEM_WEAR_ARMS,       2, 0,  9},
  { 30, "&+Lvambraces&n",      1.40, 1.40, ITEM_WEAR_ARMS,       3, 0,  9},
  { 31, "&+Lshield&N",         1.40, 1.30, ITEM_WEAR_SHIELD,     3, 0,  7},
  { 32, "&+Lheater shield&N",  1.30, 1.80, ITEM_WEAR_SHIELD,     3, 0,  6},
  { 33, "&+Lbodycloak&N",      1.42, 1.00, ITEM_WEAR_ABOUT,      3, 0,  8},
  { 34, "&+Lcloak&N",          1.37, 1.00, ITEM_WEAR_ABOUT,      3, 0,  6},
  { 35, "&+Lmantle&N",         1.40, 1.00, ITEM_WEAR_ABOUT,      3, 0,  7},
  { 36, "&+Lbelt&N",           1.23, 0.50, ITEM_WEAR_WAIST,      2, 0,  3},
  { 37, "&+Lgirth&N",          1.00, 1.00, ITEM_WEAR_WAIST,      2, 0,  4},
  { 38, "&+Lbracer&N",         1.00, 0.70, ITEM_WEAR_WRIST,      2, 0,  5},
  { 39, "&+Lbracelet&N",       1.10, 0.50, ITEM_WEAR_WRIST,      2, 0,  3},
  { 40, "&+Lwristguard&N",     1.00, 0.50, ITEM_WEAR_WRIST,      2, 0,  4},
  { 41, "&+Leyepatch&N",       1.10, 0.60, ITEM_WEAR_EYES,       2, 0,  1},
  { 42, "&+Lvisor&N",          1.10, 0.60, ITEM_WEAR_EYES,       2, 0,  3},
  { 43, "&+Lmask&N",           1.00, 1.00, ITEM_WEAR_FACE,       2, 0,  5},
  { 44, "&+Lveil&N",           1.05, 0.50, ITEM_WEAR_FACE,       2, 0,  2},
  { 45, "&+Learring&N",        1.00, 0.00, ITEM_WEAR_EARRING,    1, 0,  1},
  { 46, "&+Lstud&N",           0.70, 0.00, ITEM_WEAR_EARRING,    1, 0,  1},
  { 47, "&+Lquiver&N",         0.50, 0.00, ITEM_WEAR_QUIVER,     2, 0,  6},
  { 48, "&+Lbadge&N",          0.50, 0.00, ITEM_GUILD_INSIGNIA,  1, 0,  1},
  { 49, "&+Lsaddle&N",         0.70, 0.80, ITEM_HORSE_BODY,      1, 0,  8},
  { 50, "&+Ltail protector&N", 0.80, 0.20, ITEM_WEAR_TAIL,       1, 0,  3},
  { 51, "&+Lnose ring&N",      0.80, 0.00, ITEM_WEAR_NOSE,       1, 0,  1},
  { 52, "&+Lbelt buckle&N",    0.20, 0.10, ITEM_ATTACH_BELT,     1, 0,  2},
  { 53, "&+Lskull&N",          1.40, 0.50, ITEM_WEAR_HEAD,       2, 0,  3},
  { 54, "&+Lhorn&N",           1.00, 0.00, ITEM_WEAR_HORN,       1, 0,  2},
  { 55, "&+Lchainmail&N",      1.50, 1.30, ITEM_WEAR_BODY,       3, 0, 35},
  { 56, "&+Lcuirass&N",        1.30, 1.20, ITEM_WEAR_BODY,       2, 0, 20},
  { 57, "&+Lbreastplate&N",    1.40, 1.50, ITEM_WEAR_BODY,       2, 0, 25},
  { 58, "&+Lamulet&N",         1.10, 0.00, ITEM_WEAR_NECK,       2, 0,  2},
  { 59, "&+Lmedallion&N",      1.20, 0.00, ITEM_WEAR_NECK,       2, 0,  2},
  { 60, "&+Lcharm&N",          1.00, 0.00, ITEM_WEAR_NECK,       2, 0,  2},
  { 61, "&+Lpendant&N",        1.20, 0.00, ITEM_WEAR_NECK,       2, 0,  2},
  { 62, "&+Ltorque&N",         1.20, 1.00, ITEM_WEAR_NECK,       2, 0,  2},
  { 63, "&+Lgorget&n",         1.30, 1.50, ITEM_WEAR_NECK,       3, 0,  2},
  { 64, "&+Lcap&N",            1.10, 0.30, ITEM_WEAR_HEAD,       2, 0,  3},
  { 65, "&+Lcoif&N",           1.00, 1.10, ITEM_WEAR_HEAD,       2, 0,  5},
  { 66, "&+Lcirclet&N",        1.40, 0.60, ITEM_WEAR_HEAD,       2, 0,  4},
  { 67, "&+Ltiara&N",          1.30, 0.30, ITEM_WEAR_HEAD,       2, 0,  4},
  { 68, "&+Lhood&N",           1.20, 0.80, ITEM_WEAR_HEAD,       2, 0,  4},
  { 69, "&+Lclaws&N",          1.20, 1.00, ITEM_WEAR_HANDS,      2, 0,  4},
  { 70, "&+Lbuckler&N",        1.00, 1.00, ITEM_WEAR_SHIELD,     3, 0,  3},
  { 71, "&+Ltower shield&N",   1.10, 2.00, ITEM_WEAR_SHIELD,     3, 0, 11},
  { 72, "&+Lcord&n",           0.90, 0.10, ITEM_WEAR_TAIL,       2, 0,  2},
  { 73, "&+Lspider armor&N",   0.70, 1.20, ITEM_SPIDER_BODY,     1, 0,  6},

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
  { 74, "&+Llong sword&n",     1.00, 1.00, ITEM_WIELD, 2, WEAPON_LONGSWORD,    4},
  { 75, "&+Ybattle axe&n",     1.10, 1.20, ITEM_WIELD, 3, WEAPON_AXE,          6},
  { 76, "&+Ldagger&n",         0.80, 0.80, ITEM_WIELD, 2, WEAPON_DAGGER,       1},
  { 77, "&+rwarhammer&n",      1.00, 1.00, ITEM_WIELD, 2, WEAPON_HAMMER,       5},
  { 78, "&+Lsword&n",          1.00, 1.00, ITEM_WIELD, 2, WEAPON_LONGSWORD,    3},
  { 79, "&+Lshort sword&n",    0.80, 0.90, ITEM_WIELD, 2, WEAPON_SHORTSWORD,   2},
  { 80, "&+Glance&n",          1.20, 1.20, ITEM_WIELD, 2, WEAPON_LANCE,       10},
  { 81, "&+Ldrusus&n",         0.80, 0.80, ITEM_WIELD, 2, WEAPON_SHORTSWORD,   3},
  { 82, "&+Yghurka&n",         0.80, 0.90, ITEM_WIELD, 2, WEAPON_SHORTSWORD,   2},
  { 83, "&+Lflail&n",          1.20, 1.10, ITEM_WIELD, 3, WEAPON_FLAIL,        5},
  { 84, "&+rfalchion&n",       1.00, 1.00, ITEM_WIELD, 2, WEAPON_LONGSWORD,    8},
  { 85, "&+Rmaul&n",           0.90, 1.40, ITEM_WIELD, 3, WEAPON_HAMMER,       8},
  { 86, "&+Lbroad sword&n",    1.10, 1.20, ITEM_WIELD, 2, WEAPON_LONGSWORD,    7},
  { 87, "&+Lgreat sword&n",    1.10, 1.20, ITEM_WIELD, 3, WEAPON_2HANDSWORD,  10},
  { 88, "&+Lmorningstar&n",    1.00, 1.00, ITEM_WIELD, 2, WEAPON_MACE,         6},
  { 89, "&+ycudgel&n",         1.00, 1.00, ITEM_WIELD, 2, WEAPON_CLUB,         3},
  { 90, "&+Lscythe&n",         1.00, 1.00, ITEM_WIELD, 2, WEAPON_SICKLE,      10},
  { 91, "&+Wscimitar&n",       1.10, 0.90, ITEM_WIELD, 2, WEAPON_LONGSWORD,    4},
  { 92, "&+Btrident&n",        1.00, 1.00, ITEM_WIELD, 2, WEAPON_TRIDENT,      4},
  { 93, "&+Lmace&n",           1.00, 1.00, ITEM_WIELD, 2, WEAPON_MACE,         6},
  { 94, "&+Lpronged fork&n",   1.00, 1.00, ITEM_WIELD, 2, WEAPON_TRIDENT,      4},
  { 95, "&+Lwhip&n",           1.10, 0.90, ITEM_WIELD, 2, WEAPON_WHIP,         2},
  { 96, "&+Lknife&n",          0.80, 0.80, ITEM_WIELD, 2, WEAPON_DAGGER,       1},
  { 97, "&+Lshillelagh&n",     1.20, 1.20, ITEM_WIELD, 3, WEAPON_CLUB,         3},
  { 98, "&+Lrod&n",            1.00, 1.00, ITEM_WIELD, 2, WEAPON_CLUB,         2},
  { 99, "&+Llongbow&n",        1.00, 1.00, ITEM_WIELD, 3, 0,                   3},
  {100, "&+Lstiletto&n",       1.00, 0.60, ITEM_WIELD, 2, WEAPON_DAGGER,       2},
  {101, "&+Lbastard sword&n",  1.10, 1.20, ITEM_WIELD, 3, WEAPON_2HANDSWORD,   6},
  {102, "&+Whand axe&n",       0.90, 1.10, ITEM_WIELD, 2, WEAPON_AXE,          3},
  {103, "&+yquarterstaff&n",   1.40, 0.90, ITEM_WIELD, 2, WEAPON_CLUB,         4},
  {104, "&+Lleister&n",        1.10, 1.20, ITEM_WIELD, 3, WEAPON_TRIDENT,      3},
  {105, "&+Lsickle&n",         1.00, 1.00, ITEM_WIELD, 2, WEAPON_SICKLE,       2},
  {106, "&+Ldirk&n",           1.00, 0.70, ITEM_WIELD, 2, WEAPON_DAGGER,       2},
  {107, "&+Rswitchblade&n",    0.80, 0.80, ITEM_WIELD, 2, WEAPON_DAGGER,       1},
  {108, "&+rmallet&n",         1.00, 1.00, ITEM_WIELD, 2, WEAPON_HAMMER,       6},
  {109, "&+mtruncheon&n",      1.10, 1.20, ITEM_WIELD, 3, WEAPON_CLUB,         4},
  {110, "&+LBUGGED&n",         0.10, 0.10, ITEM_WIELD, 2, WEAPON_DAGGER,     999}
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
 /* char     buf1[MAX_STRING_LENGTH];
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

  convertObj(obj);*/

  int matnum = number(400000, 400209);

  P_obj obj = read_object(matnum, VIRTUAL);

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

// Returns TRUE iff a random item should be created from ch killing the mob.
bool check_random_drop(P_char ch, P_char mob, bool piece)
{
  int i;
  // Normally, we'd do char_lvl and mob_lvl as ints, but that would require a lot of casting.
  float char_lvl, mob_lvl, chance, char_mob_lvl_div, luck_divisor;

// int trophy_mod;

  if( !ch || !mob )
  {
    return FALSE;
  }

  if( !GET_EXP(mob) || IS_SHOPKEEPER(mob) )
  {
    return FALSE;
  }

  if( IS_SET(world[ch->in_room].room_flags, GUILD_ROOM) )
  {
    return FALSE;
  }

  char_lvl = (float)GET_LEVEL(ch);
  mob_lvl = (float)GET_LEVEL(mob);

  if( CHAR_IN_TOWN(ch) && (mob_lvl > 25) )
  {
    return FALSE;
  }

  if( (char_lvl - mob_lvl) > 10. )
  {
    return FALSE;
  }

/* Not worth the time to calculate.
  for( i = 0; highdrop_mobs[i] != -1; i++ )
  {
    if( highdrop_mobs[i] == GET_VNUM(mob) )
    {
      // 20% drop + reg chance.
      if( !number(0, 4) )
      {
        return TRUE;
      }
      break;
    }
    i++;
  }
*/

/* Modifier for trophy.  Since is no trophy atm, disabled.
  trophy_mod = 0;
  if (!IS_TRUSTED(ch) && !IS_NPC(ch))
  {
    for( tr = ch->only.pc->trophy; tr; tr = tr->next )
    {
      if( tr && GET_VNUM(mob) == tr->vnum )
      {
        trophy_mod = tr->kills > 0 ? tr->kills / 100 : 0;
        break;
      }
    }
  }
*/

  // Since extremely low lvl characters can easily get a high multiplier.
  // Note: 1x = base 50% chance, 2x = base 75% chance, 3x = base 100% chance.
  switch( (int)char_lvl )
  {
    case 1:
      // Requires lvl 5 mob for 1x multiplier, 8 for 2x, 11 for 3x
      char_mob_lvl_div = (mob_lvl - 2.) / 3.;
      break;
    case 2:
      // Requires lvl 7 mob for 1x multiplier, 11 for 2x, 15 for 3x
      char_mob_lvl_div = (mob_lvl - 3.) / 4.;
      break;
    case 3:
      // Requires lvl 9 mob for 1x multiplier, 14 for 2x, 19 for 3x
      char_mob_lvl_div = (mob_lvl - 4.) / 5.;
      break;
    case 4:
      // Requires lvl 10 mob for 1x multiplier, 16 for 2x and 22 for 3x
      char_mob_lvl_div = (mob_lvl - 4.) / 6.;
      break;
    case 5:
      // Requires lvl 11 mob for 1x multiplier, 17 for 2x and 23 for 3x
      char_mob_lvl_div = (mob_lvl - 5.) / 6.;
      break;
    case 6:
      // Requires lvl 12 mob for 1x multiplier, 19 for 2x, 26 for 3x
      char_mob_lvl_div = (mob_lvl - 5.) / 7.;
      break;
    case 7:
      // Requires lvl 12 mob for 1x multiplier, 20 for 2x, 28 for 3x
      char_mob_lvl_div = (mob_lvl - 4.) / 8.;
      break;
    case 8:
      // Requires lvl 13 mob for 1x multiplier, 22 for 2x, 31 for 3x
      char_mob_lvl_div = (mob_lvl - 4.) / 9.;
      break;
    case 9:
      // Requires lvl 13 mob for 1x multiplier, 23 for 2x, 33 for 3x
      char_mob_lvl_div = (mob_lvl - 3.) / 10.;
      break;
    case 10:
      // Requires lvl 13 mob for 1x multiplier, 24 for 2x, 35 for 3x
      char_mob_lvl_div = (mob_lvl - 2.) / 11.;
      break;
    case 11:
      // Requires lvl 14 mob for 1x multiplier, 26 for 2x, 38 for 3x
      char_mob_lvl_div = (mob_lvl - 2.) / 12.;
      break;
    case 12:
      // Requires lvl 14 mob for 1x multiplier, 27 for 2x, 40 for 3x
      char_mob_lvl_div = (mob_lvl - 1.) / 13.;
      break;
    case 13:
      // Requires lvl 14 mob for 1x multiplier, 28 for 2x, 42 for 3x
      char_mob_lvl_div = mob_lvl / 14.;
      break;
    case 14:
      // Requires lvl 14 mob for 1x multiplier, 29 for 2x, 44 for 3x
      char_mob_lvl_div = (mob_lvl + 1.) / 15.;
      break;
    default:
      // Note: Since MAXLVL = 62, and 3 ~~ 62 / 20, the highest level you can score 100% base via level
      //   is ch at level 20 vs a level 60 or higher mob.  Highest likely is level 16 ch vs level 48 mob.
      char_mob_lvl_div = mob_lvl / char_lvl;
      break;
  }
  // Minimum of 1/10th - equivalent of a lvl 50 fighting a lvl 5 mob.
  if( char_mob_lvl_div < .1 )
  {
    char_mob_lvl_div = .1;
  }
  luck_divisor = get_property("random.drop.luck.divisor", 4.);

  /* Start with a 50% chance for even levels and 100 luck.
   * All of below assumes luck_divisor == 4.000:
   * To start with a 100% chance with equal levels, you need 300 luck = (50 * luck_divisor + 100).
   *   So, to make it 400 luck for 100% chance with equal levels, set luck_divisor = 6 (+1. for every +50 luck).
   * To start with a 100% chance with 100 luck, you need a level multiplier of 3.0.
   *   Due to the multiplier not being a straight division at low lvls (for 100% base chance):
   *     @lvl 1 ch, mob lvl >= 12, @lvl 2 ch, mob lvl >= 14, @lvl 3 ch, mob lvl >= 21, @lvl 4 ch, mob lvl >= 24.
   *   At lvl 5 ch, mob lvl >= 15... hrm
   * Note: To change the 50% base chance, do not change the format of the function below,
   *   just change the 50 near the end to whatever base % you want.
   * You can change the random.drop.luck.divisor to whatever without changing the base chance
   *   with the equation in this format.
   */
  chance = (GET_C_LUK(ch) / luck_divisor) * char_mob_lvl_div + (50. - 100./luck_divisor);
  // Add up to or subtract up to 5 percentage points (rl luck).
  chance += (float)number(-5, 5);

  // Add up to 10% for low lvl chars.
  if( char_lvl < get_property("random.drop.increase.for.below.lvl", 26.) )
  {
     chance += (float)number(0, get_property("random.drop.increase.for.below.lvl.perc", 10));
  }

  // Boost for elite npcs
  if( IS_ELITE(mob) )
    chance += (float)number(5, 15);

  // Multiplier for hardcore
  if( IS_HARDCORE(ch) )
  {
    chance *= get_property("random.drop.modifier.hardcore", 1.500f);
  }

  // chance currently somewhere around 45-80% (% = min for 100 luck equal lvls above 25 non-elite,
  //   and 80% = max for 100 luck equal lvls below 26 elite mob).
  // 45-80% * {10|3} / 100 = {4.5-8% for piece|1.35-2.4% for equipment}
  if( piece )
  {
    chance *= get_property("random.drop.piece.percentage", 10.0f) / 100.0;
  }
  else
  {
    chance *= get_property("random.drop.equip.percentage", 3.0f) / 100.0;
  }

/* Disabled since no trophy atm.
  if (chance >= number(1, 100 + (trophy_mod * 2)))
*/
  // Add .5 for rounding fix to floor-function type-casting.
  if( (int)(chance + .5) >= number(1, 100) )
    return TRUE;

  return FALSE;
}

P_obj create_random_eq_new( P_char killer, P_char mob, int object_type, int material_type )
{
  P_obj    obj;
  int      ansi_n = 0;
  int      howgood, material, prefix, slot, value, bonus, count;
  int      splnum, tries;
  bool     should_have_weapon_proc = FALSE;
  struct zone_data *zone = NULL;
  char     o_name[MAX_STRING_LENGTH];
  char     o_short[MAX_STRING_LENGTH];
  char     o_long[MAX_STRING_LENGTH];
  char     owner[MAX_STRING_LENGTH];
  float    weight;

  // Load the random item blank
  if( object_type == -1 )
  {
    // This is hack to make equipment load more often than weapons. Aug09 -Lucrot
    if( number(0, 2) )
    {
      slot = number(0, 70); // Worn eq
    }
    else
    {
      slot = number(0, MAX_SLOT - 1); // Anything
    }
  }
  else
  {
    slot = BOUNDED(0, object_type, MAX_SLOT - 1);
  }

  if( slot_data[slot].wear_bit == ITEM_WIELD )
  {
    obj = read_object(VOBJ_RANDOM_WEAPON, VIRTUAL);
  }
  else
  {
    obj = read_object(VOBJ_RANDOM_ARMOR, VIRTUAL);
  }

  if( !obj )
  {
    send_to_char( "Failed to load a new item.\n\r", killer );
    return NULL;
  }

  prefix = number(0, MAXPREFIX);

  // Make sure you understand this value before ya change it.
  // howgood runs from 1 to 62 to start.
  howgood = (int) ((GET_LEVEL(killer) + GET_LEVEL(mob)) / 2.0);
  // howgood is then changed by the property %.
  howgood = (int) (howgood * (get_property("random.drop.modifier.quality", 80.000) / 100.0));

  if( material_type == -1 )
  {
    // howgood caps level check.
    if( howgood > GET_LEVEL(mob) / 3 )
    {
      material = number( GET_LEVEL( mob ) / 3, howgood );
    }
    else
    {
      material = howgood;
    }
    // Keep within bounds.
    material = BOUNDED(0, material, MAXMATERIAL - 1);
  }
  else
  {
    material = BOUNDED(0, material_type, MAXMATERIAL - 1);
  }

  zone = &zone_table[world[killer->in_room].zone];

  ansi_n = strcmp( strip_ansi(zone->name).c_str(), zone->name);

  if( IS_PC(killer) )
  {
    sprintf( owner, "%s", GET_NAME(killer) );
  }
  else
  {
    sprintf( owner, "%s", FirstWord(GET_NAME(killer)) );
  }

  // Set the items name / short / long descriptions!
  // For PCs and non-shop-keeper mobs.
  if( !number(0, (int) (400 / (GET_LEVEL(killer) + GET_C_LUK(killer) + GET_LEVEL(mob))))
    && (material_type) == -1 && ansi_n && (IS_PC(killer) || mob_index[GET_RNUM(killer)].func.mob != shop_keeper) )
  {
    if( IS_NEWBIE(killer) )
    {
      send_to_char( "You got a named random item!  Use 'help named equipment' for a list of what named eq can do for you!\n\r", killer );
    }
    sprintf(o_name, "random _noquest_ %s %s %s %s", strip_ansi(prefix_data[prefix].m_name).c_str(),
      strip_ansi(material_data[material].m_name).c_str(),
      strip_ansi(slot_data[slot].m_name).c_str(), strip_ansi(zone->name).c_str());

    // If it's a plural noun (ie shoes / boots / leggings / etc).
    // Note: Some plural nouns don't end in s, but ok.
    if( slot_data[slot].m_name[strlen(slot_data[slot].m_name) - 3] == 's' )
    {
      sprintf(o_short, "some %s %s %s &+rfrom %s&n", prefix_data[prefix].m_name,
        material_data[material].m_name, slot_data[slot].m_name, zone->name);
      sprintf(o_long, "Some %s %s %s &+rfrom %s&n lie here on the ground.",
        prefix_data[prefix].m_name, material_data[material].m_name,
        slot_data[slot].m_name, zone->name);
    }
    else
    {
      sprintf(o_short, "%s %s %s %s &+rfrom %s&n", VOWEL(prefix_data[prefix].m_name[3]) ? "an" : "a",
	      prefix_data[prefix].m_name, material_data[material].m_name, slot_data[slot].m_name, zone->name);
      sprintf(o_long, "%s %s %s %s &+rfrom %s&n lies here on the ground.",
        VOWEL(prefix_data[prefix].m_name[3]) ? "An" : "A",
        prefix_data[prefix].m_name, material_data[material].m_name,
        slot_data[slot].m_name, zone->name);
    }

    //  Zone named items get a higher chance to get multiple affects set.
    // 99.45% chance for one.
    obj = setsuffix_obj_new(obj);
    // 5% chance for 2, .025% chance for 3 ...
    while( !number(0, 19) )
    {
      obj = setsuffix_obj_new(obj);
    }
    // set values for a proc(should the item turn out to be a weapon)
    if( !number(0, 2) && (slot_data[slot].wear_bit == ITEM_WIELD) )
    {
      should_have_weapon_proc = TRUE;
    }
  }
  else if( !number(0, 9) && (material_type == -1) && (GET_LEVEL(mob) > 45) )
  {
    sprintf(o_name, "random _noquest_ %s %s %s %s", strip_ansi(prefix_data[prefix].m_name).c_str(),
      strip_ansi(material_data[material].m_name).c_str(), strip_ansi(slot_data[slot].m_name).c_str(), owner );
    if( slot_data[slot].m_name[strlen(slot_data[slot].m_name) - 3] == 's' )
    {
      sprintf(o_short, "some %s %s %s&n", prefix_data[prefix].m_name,
        material_data[material].m_name, slot_data[slot].m_name);
      sprintf(o_long, "Some %s %s %s&n lie here on the ground.",
        prefix_data[prefix].m_name, material_data[material].m_name, slot_data[slot].m_name);
    }
    else
    {
      sprintf(o_short, "%s %s %s %s&n", VOWEL(prefix_data[prefix].m_name[3]) ? "an" : "a",
        prefix_data[prefix].m_name, material_data[material].m_name, slot_data[slot].m_name);
      sprintf(o_long, "%s %s %s %s&n lies here on the ground.", VOWEL(prefix_data[prefix].m_name[3]) ? "An" : "A",
        prefix_data[prefix].m_name, material_data[material].m_name, slot_data[slot].m_name);
    }

    if( !number(0, 9) )
    {
      obj = setsuffix_obj_new(obj);
      // To ensure that we don't infinite loop if difficulty isn't set
      count = 0;
      // Zone difficulty generally 1, range 1 to 9 (as of 5/27/2016).
      // For difficutly 1: 1 in 7 chance (about 14%), 2: 2 in 8, 3: 3 in 9, 4: 4 in 10 .. 9: 9 in 15 = 60% chance.
      while( (number(1, zone->difficulty + 6) > 6) && count < 2 )
      {
        obj = setsuffix_obj_new(obj);
        count++;
      }
    }
  }
/* Commenting this out since we don't want our random eq to be crafted by someone who is just now receiving the item.
  else if( !number(0, BOUNDED(1, 150 - GET_CHAR_SKILL(killer, SKILL_CRAFT) - (GET_C_LUK(killer) - 90), 150))
    && material_type != -1 )
  {
    sprintf(o_name, "random _noquest_ %s %s %s %s", strip_ansi(prefix_data[prefix].m_name).c_str(),
            strip_ansi(material_data[material].m_name).c_str(),
            strip_ansi(slot_data[slot].m_name).c_str(), owner );
    if (slot_data[slot].m_name[strlen(slot_data[slot].m_name) - 3] == 's')
    {
      sprintf(o_short, "some %s %s %s crafted by &+r%s&n",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, owner);
      sprintf(o_long,
              "Some %s %s %s crafted by &+r%s&n lie here on the ground.",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, owner);
    }
    else
    {
      sprintf(o_short, "%s %s %s %s crafted by &+r%s&n",
              VOWEL(prefix_data[prefix].m_name[3]) ? "an" : "a",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, owner);
      sprintf(o_long, "%s %s %s %s crafted by &+r%s&n lies here on the ground.",
              VOWEL(prefix_data[prefix].m_name[3]) ? "An" : "A",
              prefix_data[prefix].m_name, material_data[material].m_name,
              slot_data[slot].m_name, owner);
    }
    obj = setsuffix_obj_new(obj);
    while (!number(0, 19))
    {
      obj = setsuffix_obj_new(obj);
    }
    should_have_weapon_proc = TRUE;
  }
*/
  else
  {
    sprintf(o_name, "random _noquest_ %s %s %s", strip_ansi(prefix_data[prefix].m_name).c_str(),
      strip_ansi(material_data[material].m_name).c_str(), strip_ansi(slot_data[slot].m_name).c_str());
    if (slot_data[slot].m_name[strlen(slot_data[slot].m_name) - 3] == 's')
    {
      sprintf(o_short, "some %s %s %s&n", prefix_data[prefix].m_name, material_data[material].m_name,
        slot_data[slot].m_name);
      sprintf(o_long, "Some %s %s %s&n lie here on the ground.", prefix_data[prefix].m_name, material_data[material].m_name,
        slot_data[slot].m_name);
    }
    else
    {
      sprintf(o_short, "%s %s %s %s&n", VOWEL(prefix_data[prefix].m_name[3]) ? "an" : "a", prefix_data[prefix].m_name,
        material_data[material].m_name, slot_data[slot].m_name);
      sprintf(o_long, "%s %s %s %s&n lies here on the ground.", VOWEL(prefix_data[prefix].m_name[3]) ? "An" : "A",
        prefix_data[prefix].m_name, material_data[material].m_name, slot_data[slot].m_name);
    }
  }

  set_keywords(obj, o_name);
  set_short_description(obj, o_short);
  set_long_description(obj, o_long);

  // Set wear flags
  SET_BIT(obj->wear_flags, ITEM_TAKE);
  SET_BIT(obj->wear_flags, slot_data[slot].wear_bit);

  // base_weight ( 1 - 40 ) * weight_mod ( 0.75 - 1.21 ) * weight_mod( 0.15 - 5.00 )
  // Weight range = 0 - 242 .. wow 242 for an (unearthly or brutal) adamantium platemail
  weight = (slot_data[slot].base_weight * prefix_data[prefix].weight_mod * material_data[material].weight_mod);
  // This brings the crazy platemail down to 107.
  weight = (weight + 2.0 * slot_data[slot].base_weight) / 3.0;
  // If it's between .1 and 1, round up.
  obj->weight = ((weight < 1.0) && (weight > 0.1)) ? 1 : weight;

  // Shields run from ( 5 - 20) * ( 0.75 - 1.21 ) * ( 0.15 - 5.00 )
  // Min: (( 5 * 0.75 * 0.15) + (2 *  5)) / 3 = 10.5625 / 3 = 3
  // Max: ((11 * 1.21 * 5.00) + (2 * 11)) / 3 = 88.55 / 3 = 29.

  // Item Attributes
  if( !number(0, 5) && (GET_LEVEL(mob) > 49) )
  {
    // (10 to 65) * (.5 to 1.5) * (.2 to 1.7) + (0 to 20)
    // (10 * .5 * .2 + 0) to (65 * 1.5 * 1.7 + 20) = 1 to 185
    value = (int) (material_data[material].m_stat * prefix_data[prefix].m_stat * slot_data[slot].m_stat + number(0, 20));
    // 1 to 185. When we divide by 30, we get 0 to 6.
    obj = setprefix_obj(obj, value / 30, 2);
  }
  if( !number(0, 2) && (GET_LEVEL(mob) > 35) )
  {
    value = (int) (material_data[material].m_stat * prefix_data[prefix].m_stat * slot_data[slot].m_stat + number(0, 20));
    // 1 to 185. When we divide by 36, we get 0 to 5.
    obj = setprefix_obj(obj, value / 36, 1);
  }
  value = (int) (material_data[material].m_stat * prefix_data[prefix].m_stat * slot_data[slot].m_stat + number(0, 20));
  // 1 to 185. When we divide by 46, we get 0 to 4.
  obj = setprefix_obj(obj, value / 46, 0);

  obj->material = material_data[material].m_number;

  // We don't need this: just sets cost and weight which we do in this function anyway.
  // convertObj(obj);
  // Sets anti-class flags based on material type (exemptions via object name)
  material_restrictions(obj);

  if( isname("unique", obj->name) )
  {
    obj->craftsmanship = OBJCRAFT_HIGHEST;
  }

  if( IS_SET(obj->wear_flags, ITEM_WEAR_QUIVER) )
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
    // m_stat ranges from 12 - 50,  12-50 + 75 = 87-125 = ac bonus
    obj->value[3] = (int) (material_data[material].m_stat + 75);
    if( isname("heater", obj->name) || isname("buckler", obj->name) )
    {
      // 37-75
      obj->value[3] -= 50;
    }
    else if( isname("tower", obj->name) )
    {
      // 137-175
      obj->value[3] += 50;
    }
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
  else if ( IS_SET(obj->wear_flags, ITEM_WIELD) && !isname("longbow", obj->name) )
  {
    obj->value[0] = slot_data[slot].damage_type;
    obj->type = ITEM_WEAPON;

    if( slot_data[slot].numb_material > 2 )
      obj->extra_flags |= ITEM_TWOHANDS;

    obj->value[1] = (int) (1 + (material_data[material].m_stat * prefix_data[prefix].m_stat *
      slot_data[slot].m_ac + number(0, 20)) / 20);

    obj->value[2] = MIN(
      (1 + (material_data[material].m_stat * prefix_data[prefix].m_stat * slot_data[slot].m_ac + number(0, 20)) / 20),
      ((obj->extra_flags & ITEM_TWOHANDS) ? 32 : 24) / obj->value[1] );
    obj->value[2] = BOUNDED( 4, obj->value[2], 10 );

    bonus = (int) ((material_data[material].m_stat * prefix_data[prefix].m_stat * slot_data[slot].m_stat) / 18 + 1);

    // Move 2 and 1 up to 3 and 2 so 1 doesn't get overwritten.
    obj->affected[3].location = obj->affected[2].location;
    obj->affected[3].modifier = obj->affected[2].modifier;
    obj->affected[2].location = obj->affected[1].location;
    obj->affected[2].modifier = obj->affected[1].modifier;

    obj->affected[1].location = APPLY_HITROLL;
    obj->affected[1].modifier = bonus + number(0, 1);
    obj->affected[0].location = APPLY_DAMROLL;
    obj->affected[0].modifier = bonus + number(0, 1);

    if( !(obj->extra_flags & ITEM_TWOHANDS) )
    {
      obj->affected[0].modifier = (sbyte) (obj->affected[0].modifier * 0.7);
      obj->affected[1].modifier = (sbyte) (obj->affected[1].modifier * 0.7);
    }

    if( IS_BACKSTABBER(obj) )
    {
      obj->value[1] = BOUNDED(1, (int) (obj->value[1] / 1.5), 3);
      obj->value[2] = BOUNDED(4, (int) (obj->value[2] * 1.6) + number(0,2) , obj->value[1] == 1 ? 8 : 6);
    }

    if( isname("lance", obj->name) )
    {
      obj->value[1] = 1;
      obj->value[2] = BOUNDED(6, (int) (obj->value[2] * 1.6) + number(2,4) , obj->value[1] == 1 ? 13 : 9);
    }

    if( should_have_weapon_proc )
    {
      tries = 0;
      // Why are we making ice missiles so common here?
      do
      {
        // Range from -14 to 60
        splnum = BOUNDED( 1, GET_LEVEL(mob), 60 ) - number( 0, 15 );
        // Range 0 to 60
        splnum = MAX( 0, splnum );
        tries++;
      }
      while( spells_data[splnum].self_only && tries < 100 );

      if( tries < 100 )
      {
        // Spell number
        obj->value[5] = spells_data[splnum].spell;
        // Spell level
        obj->value[6] = number(15, MAX(20, GET_LEVEL(mob) - 10));
        // Chance for spell proc (1 in val7).
        obj->value[7] = number(45, 60);
        SET_BIT(obj->extra2_flags, ITEM2_MAGIC);
      }
    }

    if( (obj->affected[3].location != APPLY_NONE) || (obj->affected[2].location != APPLY_NONE) )
    {
      SET_BIT(obj->extra2_flags, ITEM2_MAGIC);
    }
  }
  else if (isname("longbow", obj->name))
  {
    obj->type = ITEM_FIREWEAPON;
    obj->value[3] = 1;

    if( slot_data[slot].numb_material > 2 )
    {
      obj->extra_flags |= ITEM_TWOHANDS;
    }

    obj->value[0] = (int) (1 + (material_data[material].m_stat * prefix_data[prefix].m_stat *
                      slot_data[slot].m_ac + number(0, 40)));
    obj->value[1] = (int) (1 + (material_data[material].m_stat * prefix_data[prefix].m_stat *
                      slot_data[slot].m_ac + number(0, 40)));

    bonus = (int) ((material_data[material].m_stat * prefix_data[prefix].m_stat *
                      slot_data[slot].m_stat) / 20 + 1);

    // Move 2 and 1 up to 3 and 2 so 1 doesn't get overwritten.
    obj->affected[3].location = obj->affected[2].location;
    obj->affected[3].modifier = obj->affected[2].modifier;
    obj->affected[2].location = obj->affected[1].location;
    obj->affected[2].modifier = obj->affected[1].modifier;

    obj->affected[1].location = APPLY_HITROLL;
    obj->affected[1].modifier = bonus + number(0, 2);
    obj->affected[0].location = APPLY_DAMROLL;
    obj->affected[0].modifier = bonus + number(0, 2);
  }
  else
  {
    GET_ITEM_TYPE(obj) = ITEM_ARMOR;
    obj->value[0] = (int) (material_data[material].m_ac * prefix_data[prefix].m_ac * slot_data[slot].m_ac);
  }

  /*  if object has 2 identical locations for mod0 and mod1,
   *  we refigure a new mod location for mod1 and just pray mod2
   *  doesn't come out the same as mod0 and mod1 and makes some
   *  ridiculous +7dam belt :) - Jexni 11/01/08
  */
  /* Getting some extremely high and low random values (e.g. +200 damage belts) Nov08 -Lucrot
   * I removed the line that was causing the issue (I think), but leaving this commented out anyway.
   * The while loop looks buggy too.
  sbyte quickmod;
  if(obj->affected[0].location == obj->affected[1].location && obj->affected[0].location != APPLY_NONE)
  {
    obj->affected[0].modifier = (obj->affected[0].modifier + obj->affected[1].modifier) / 2;
    obj->affected[1].location = BOUNDED(APPLY_LOWEST, obj->affected[1].location + number(-3, 3), APPLY_LUCK_MAX);
    while(BAD_APPLYS(obj->affected[1].location) && (obj->affected[1].location > APPLY_NONE && obj->affected[1].location < APPLY_LUCK_MAX))
    {
      obj->affected[1].location += number(-2, 2);
    }
    quickmod = (sbyte) ((int) number(3, 5) * material_data[material].m_stat);
    if(obj->affected[1].location > 19 && obj->affected[1].location < 25)
      quickmod = -quickmod;
    obj->affected[1].modifier = quickmod;
  }
  */

  return obj;
}

// Good chance of adding a bitvector effect on item. 1 - ( 1 + 6 + 36 ) / 6^5 = about 99.45%
P_obj setsuffix_obj_new(P_obj obj)
{
  switch( dice(5, 6) )
  {
  case 5:
    SET_BIT(obj->bitvector, AFF_DETECT_INVISIBLE);
    break;
  case 6:
    SET_BIT(obj->bitvector2, AFF2_ULTRAVISION);
    break;
  case 7:
    SET_BIT(obj->bitvector2, AFF2_FIRESHIELD);
    break;
  case 8:
    SET_BIT(obj->bitvector, AFF_SLOW_POISON);
    break;
  case 9:
    SET_BIT(obj->bitvector, AFF_WATERBREATH);
    break;
  case 10:
    SET_BIT(obj->bitvector, AFF_INFRAVISION);
    break;
  case 11:
    SET_BIT(obj->bitvector, AFF_PROTECT_GOOD);
    break;
  case 12:
    SET_BIT(obj->bitvector, AFF_PROTECT_EVIL);
    break;
  case 13:
    SET_BIT(obj->bitvector2, AFF2_PROT_COLD);
    break;
  case 14:
    SET_BIT(obj->bitvector, AFF_LEVITATE);
    break;
  case 15:
    SET_BIT(obj->bitvector2, AFF2_DETECT_EVIL);
    break;
  case 16:
    SET_BIT(obj->bitvector2, AFF2_DETECT_GOOD);
    break;
  case 17:
    SET_BIT(obj->bitvector2, AFF2_DETECT_MAGIC);
    break;
  case 18:
    SET_BIT(obj->bitvector, AFF_PROT_FIRE);
    break;
  case 19:
    SET_BIT(obj->bitvector5, AFF5_PROT_UNDEAD);
    break;
  case 20:
    SET_BIT(obj->bitvector2, AFF2_PROT_ACID);
    break;
  case 21:
    SET_BIT(obj->bitvector, AFF_SENSE_LIFE);
    break;
  case 22:
    SET_BIT(obj->bitvector, AFF_MINOR_GLOBE);
    break;
  case 23:
    SET_BIT(obj->bitvector, AFF_FARSEE);
    break;
  case 24:
    SET_BIT(obj->bitvector3, AFF3_COLDSHIELD);
    break;
  case 25:
    SET_BIT(obj->bitvector2, AFF2_SOULSHIELD);
    break;
  case 26:
    SET_BIT(obj->bitvector, AFF_BARKSKIN);
    break;
  case 27:
    SET_BIT(obj->bitvector, AFF_HASTE);
    break;
  default:
    break;
  }
  return obj;
}


P_obj setprefix_obj(P_obj obj, float modifier, int affectnumber)
{

  switch( number(0, 30) )
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
    obj->affected[affectnumber].location = APPLY_MANA;
    modifier = (8 * modifier);
    break;
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
    obj->affected[affectnumber].location = APPLY_HIT;
    modifier = (7 * modifier);
    break;
  case 28:
    obj->affected[affectnumber].location = APPLY_HIT;
    modifier = (6 * modifier);
    break;
  case 29:
    obj->affected[affectnumber].location = APPLY_HIT;
    modifier = (5 * modifier);
    break;
  case 30:
    obj->affected[affectnumber].location = APPLY_HIT;
    modifier = (4 * modifier);
    break;
  default:
    obj->affected[affectnumber].location = APPLY_HIT;
    modifier = (3 * modifier);
    break;
  }
  if( modifier == 0 )
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

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || (cmd != CMD_MELEE_HIT && cmd != CMD_GOTHIT && cmd != CMD_GOTNUKED) )
  {
    return FALSE;
  }

  if( cmd == CMD_MELEE_HIT )
  {
    chance = 25;
  }

  if( cmd == CMD_GOTHIT )
  {
    chance = 100;
  }

  if( cmd == CMD_GOTNUKED )
  {
    chance = 50;
  }

  kala = (P_char) arg;

  if( !kala )
  {
    return FALSE;
  }

  curr_time = time(NULL);
//obj->value[6] = GET_VNUM(mob);
//obj->value[7] = named_spells[1];

//wizlog(56, "%d, %d, diff=%d" , obj->timer[0], curr_time,  (obj->timer[0] - curr_time)   );

  if( obj->timer[0] + 15 <= curr_time )
  {
    for( j = 0; wear_order[j] != -1; j++ )
    {
      if( ch->equipment[wear_order[j]] )
      {
        t_obj = ch->equipment[wear_order[j]];

        if( obj->value[6] == t_obj->value[6] && t_obj->value[7] != 0 )
        {
          numNamed++;
        }
      }

    }


    if( IS_FIGHTING(ch) && ((numNamed > 2 && !number(0, chance))
      || (cmd == CMD_MELEE_HIT && obj->value[6] == 999 && IS_SET(obj->wear_flags, ITEM_WIELD) && !number(0, 10))) )
    {
      kala = ch->specials.fighting;;
      act("&+B$n's $q &+rpu&+Rls&+rat&+Res &+Lwith &+be&+Bn&+Wer&+Bg&+by &+Lfor a &+rmoment&+L...&N",
         TRUE, ch, obj, kala, TO_NOTVICT);
      act("&+BYour $q &+rpu&+Rls&+rat&+Res &+Lwith &+be&+Bn&+Wer&+Bg&+by &+Lfor a &+rmoment&+L...&N",
         TRUE, ch, obj, kala, TO_CHAR);
      act("&+B$n's $q &+rpu&+Rls&+rat&+Res &+Lwith &+be&+Bn&+Wer&+Bg&+by &+Lfor a &+rmoment&+L...&N",
         TRUE, ch, obj, kala, TO_VICT);
      numNamed = obj->value[7];
      if (spells_data[numNamed].self_only)
      {
        kala = ch;
      }

      if (numNamed > 0)
      {
        ((*skills[spells_data[numNamed].spell].spell_pointer) ((int) 50, ch, 0, SPELL_TYPE_SPELL, kala, obj));
        //wizlog(56, "%d", numNamed);
        //wizlog(56, "Spell number:%d", spells_data[numNamed].spell);
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
