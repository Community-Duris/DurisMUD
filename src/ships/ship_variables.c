
/*
 * ***************************************************************************
 * *  File: ship_variables.c                                   Part of Duris *
 *    Code Modifications by: Dalreth (4/07)
 *    Warship added Nov08 -Lucrot
  * ***************************************************************************
 */

#include "ships.h"
#include "config.h"
#include "objmisc.h"
#include "utils.h"
#include "spells.h"
#include "structs.h"

/* new Ship Constants */
const PortData ports[NUM_PORTS] = {
  { 559633, "Flann", 559632 },
  { 635260, "Dalvik", 635660 },
  { 88846,  "Menden", 550725 },
  { 82670,  "Myrabolus", 512874 },
  { 66689,  "Torrhan", 580879 },
  { 9967,   "Sarmiz'Duul", 622675 },
  { 22441,  "Storm Port", 587128 },
  { 49090,  "Venan'Trut", 545021 },
  { 43158,  "Thur'Gurax", 616546 },  
};

/*const char *guncrewname[MAXGUNCREW] = {
  "Standard Crew",
  "Veteran Archers",
  "Elite Catapult Crew",
  "Stamina Crew",
  "Magical Automotons"
};

const int guncrewstats[MAXGUNCREW][MAXCREWSTATS] = {
  {0,       5,          0,      0,      70,         0},
  {5,       10,         0,      0,      100,        500},
  {10,      15,         0,      5,      130,        1000},
  {0,       0,          0,      0,      200,        0},
  {20,      20,         5,     10,      200,        3000}
};

const int guncrewcost[MAXGUNCREW] = {
  0,
  2000000,
  5000000,
  750000,
  20000000
};

const char *sailcrewname[MAXSAILCREW] = {
  "Standard Crew",
  "Veteran Crew",
  "Elite Crew",
  "Repair Crew",
  "Magical Automotons"
};

const int sailcrewstats[MAXSAILCREW][MAXCREWSTATS] = {
// Min gun,     max gun,    min sail,   max sail,   repair%,    min frags
  {0,           0,          0,          5,          20,         0},          
  {0,           0,          5,          10,         30,         500},
  {0,           5,          10,         15,         40,         1000},
  {0,           0,          0,          10,         70,         0},
  {5,          10,          15,         20,         50,         3000}
};

const int sailcrewcost[MAXSAILCREW] = {
  0,
  2000000,
  5000000,
  750000,
  20000000
};*/


// DO NOT move crews, just add new ones!
const ShipCrewData ship_crew_data[MAXCREWS] = {
//      Type          Name                    Start     Min  Skill   Base      Hire   Min
//                                            Skill    Skill  Gain  Stamina    Cost  Frags
    { SAIL_CREW,   "Standard Crew",              0,       0, 1000,     70,        0,     0},
    { SAIL_CREW,   "Veteran Crew",          450000,  450000, 1250,    100,  2000000,   500},
    { SAIL_CREW,   "Elite Crew",           1000000, 1000000, 1500,    140,  5000000,  1000},
    { SAIL_CREW,   "Magical Automatons",   3600000, 3600000, 2000,    200, 20000000,  3000},
                                                            
    { GUN_CREW,    "Standard Crew",              0,       0, 1000,     70,        0,     0},
    { GUN_CREW,    "Sturdy Catapult Crew",       0,       0,  600,    140,  1000000,     0},
    { GUN_CREW,    "Veteran Archers",       550000,  550000, 1300,    100,  2500000,   600},
    { GUN_CREW,    "Elite Ballistic Crew", 1250000, 1250000, 1600,    140,  6000000,  1200},
    { GUN_CREW,    "Magical Automatons",   3600000, 3600000, 2000,    200, 20000000,  3000},
                                                            
    { REPAIR_CREW, "Standard Crew",              0,       0, 1000,     70,        0,     0},
    { REPAIR_CREW, "Veteran Shipwrights",   700000,  700000, 1400,    100,  3000000,   750},
    { REPAIR_CREW, "Expert Shipwrights",   1650000, 1650000, 1800,    140,  8000000,  1500}, 
                                                            
    { ROWING_CREW, "Standard Crew",              0,       0,    0,     70,        0,     0},
};
const int sail_crew_list[MAXCREWS]   = { 0,   1,  2,  3 , -1, -1, -1, -1, -1, -1, -1, -1, -1};
const int gun_crew_list[MAXCREWS]    = { 4,   5,  6,  7 ,  8, -1, -1, -1, -1, -1, -1, -1, -1};
const int repair_crew_list[MAXCREWS] = { 9,  10, 11, -1 , -1, -1, -1, -1, -1, -1, -1, -1, -1};
const int rowing_crew_list[MAXCREWS] = { 12, -1, -1, -1 , -1, -1, -1, -1, -1, -1, -1, -1, -1};


#define WPNFLAG01   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED,                        //small ball        
#define WPNFLAG02   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED,                        //med ball          
#define WPNFLAG03   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED,                        //lrg ball          
#define WPNFLAG04   FORE_ALLOWED | REAR_ALLOWED |                               SHOTGUN,              //small cat         
#define WPNFLAG05   FORE_ALLOWED | REAR_ALLOWED |                               SHOTGUN,              //med cat           
#define WPNFLAG06   FORE_ALLOWED | REAR_ALLOWED |                               SHOTGUN,              //large cat         
#define WPNFLAG07                                 PORT_ALLOWED | STAR_ALLOWED,                        //hvy ball          
#define WPNFLAG08   FORE_ALLOWED | REAR_ALLOWED |                               RANGEDAM | CAPITOL,   //darkstone         
#define WPNFLAG09   FORE_ALLOWED | REAR_ALLOWED |                               RANGEDAM | CAPITOL,   //heavy darkstone   
#define WPNFLAG10   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED | MINDBLAST| CAPITOL,   //mind blast        
#define WPNFLAG11   FORE_ALLOWED | REAR_ALLOWED |                               SAILSHOT | CAPITOL,   //frag cannon       
#define WPNFLAG12   FORE_ALLOWED | REAR_ALLOWED |                               SHOTGUN  | CAPITOL    //long tom          

const WeaponData weapon_data[MAXWEAPON] = {
// Name                          Cost Weight Ammo    Min     Max     Min     Max  Fragments Damage  Sail   Hull    Sail    Armor   Reload  Reload  Volley      Flags
//                                                  range   range  damage  damage   count     arc    hit  damage  damage  pierce     time stamina    time 
 { "Small Ballistae",          50000,    3,   90,      0,      8,      2,      4,      1,      10,   20,    100,     50,     10,      30,      3,      7,  WPNFLAG01 },
 { "Medium Ballistae",        100000,    6,   60,      0,     10,      4,      6,      1,      10,   20,    100,     50,     10,      30,      5,      8,  WPNFLAG02 },
 { "Large Ballistae",         500000,   10,   30,      0,     12,      6,      9,      1,      10,   20,    100,     50,     10,      30,      7,     10,  WPNFLAG03 },
 { "Small Catapult",          500000,   10,   30,      4,     15,      2,      3,      4,     160,   20,    100,    100,      2,      30,     10,     18,  WPNFLAG04 },
 { "Medium Catapult",         800000,   13,   20,      5,     20,      2,      4,      5,     260,   20,    100,    100,      2,      30,     15,     24,  WPNFLAG05 },
 { "Large Catapult",         1200000,   17,   12,      6,     25,      2,      5,      6,     360,   20,    100,    100,      2,      30,     20,     30,  WPNFLAG06 },
 { "Heavy Ballistae",        1000000,   15,    6,      0,      5,     15,     25,      1,      10,    0,    100,      0,     15,      30,     20,      3,  WPNFLAG07 },
 { "Quartz Beamcannon",      4000000,    7,   40,      0,     20,      4,     16,      1,      10,   20,    100,     50,     15,      45,     10,      0,  WPNFLAG08 },
 { "Darkstone Beamcannon",   5000000,    9,   40,      0,     23,      5,     22,      1,      10,   20,    100,     50,     15,      45,     12,      0,  WPNFLAG09 },
 { "Mind Blast Cannon",      4000000,    5,   50,      0,     20,      0,      0,      1,      10,    0,      0,      0,      0,      45,      2,      0,  WPNFLAG10 },
 { "Fragmentation Cannon",   5000000,    7,   20,      0,     15,      4,      6,      5,      90,   50,     50,    100,      0,      45,     10,     12,  WPNFLAG11 },
 { "Long Tom Catapult",      5000000,    9,    6,     20,     32,      3,      6,      8,     360,   20,    100,    100,      3,      45,     20,     36,  WPNFLAG12 },
};                                                                                             
                                                                                               
                                                                                               
const char *armor_condition_prefix[3] = {                                                                       
  "&+R",
  "&+Y",
  "&+G"
};

const char *internal_condition_prefix[3] = {
  "&+r",
  "&+y",
  "&+g"
};

/* NEVER CHANGE SHIP IDs Just add new ones. */
extern const ShipTypeData ship_type_data[MAXSHIPCLASS];
const ShipTypeData ship_type_data[MAXSHIPCLASS] = {

//ID, Ship Type Name,      Cost,  ECost, Hull, Slots, MxWeight, Sail, MxCargo, MxContra, MxPeople,MxSpeed, HDDec, Accel, MinLvl, FreeWpn, FreeCrg, 
 { 1,        "Sloop",    100000,      0,   10,     1,     5,      20,    1,        0,       1,     100,    50,     30,       0,      0,       0   },
 { 2,        "Yacht",    500000,      0,   25,     4,    12,      40,    3,        0,       2,     100,    50,     25,       0,      2,       0   },
 { 3,      "Clipper",   3000000,      0,  110,     6,    55,      90,   10,        1,       5,      88,    40,     22,      20,      8,       0   },
 { 4,        "Ketch",   5000000,      0,  150,     8,    75,     100,   20,        3,       8,      76,    40,     16,      25,     10,       0   },
 { 5,      "Caravel",   8000000,      0,  200,     8,   100,     110,   35,        6,      10,      64,    30,     12,      30,     13,      10   },
 { 6,      "Carrack",  16000000,      0,  260,    10,   130,     120,   50,       10,      15,      54,    30,     10,      35,     16,      22   },
 { 7,      "Galleon",  24000000,      0,  330,    12,   165,     130,   70,       15,      20,      46,    20,      8,      40,     19,      40   },
                                                                                                                                                  
 { 8,     "Corvette",   7500000,      0,  165,     6,    82,     110,    0,        5,       6,      74,    40,     18,      31,     11,       0   },
 { 9,    "Destroyer",  15000000,      0,  220,     8,   110,     120,    0,        0,       8,      63,    35,     15,      36,     14,       0   },
 {10,      "Cruiser",  22000000,      0,  285,    10,   142,     130,    0,        0,      10,      53,    30,     12,      41,     18,       0   },
 {11,      "Frigate",  36000000,      0,  400,    12,   200,     150,    0,        5,      15,      42,    25,      8,      46,     23,       0   },
 {12,  "Dreadnought",         0,      0,  600,    14,   300,     200,    0,        0,      20,      40,    22,      6,      51,     32,       0   } 

};

extern const ShipArcProperties ship_arc_properties[MAXSHIPCLASS];
const ShipArcProperties ship_arc_properties[MAXSHIPCLASS] = {
//      Weapon slots            Weapon weight                  Armor                   Internal
//   Fwrd Port Rear Stbd      Fwrd Port Rear Stbd       Fwrd Port Rear Stbd       Fwrd Port Rear Stbd  
  { {  0,   0,   0,   0 },   {  0,   0,   0,   0 },   {   2,   3,   1,   3 },   {   1,   1,   1,   1 } }, // Sloop
  { {  1,   1,   1,   1 },   {  3,   3,   3,   3 },   {   6,   8,   4,   8 },   {   3,   4,   2,   4 } }, // Yacht
  { {  1,   2,   1,   2 },   { 10,  15,  10,  15 },   {  29,  36,  18,  36 },   {  14,  18,   9,  18 } }, // Clipper
  { {  1,   2,   1,   2 },   { 13,  20,  13,  20 },   {  40,  50,  25,  50 },   {  20,  25,  12,  25 } }, // Ketch
  { {  1,   3,   1,   3 },   { 17,  27,  17,  27 },   {  53,  66,  33,  66 },   {  26,  33,  16,  33 } }, // Caravel
  { {  1,   3,   1,   3 },   { 21,  35,  21,  35 },   {  69,  86,  43,  86 },   {  34,  43,  21,  43 } }, // Carrack
  { {  2,   3,   1,   3 },   { 26,  40,  26,  40 },   {  88, 110,  55, 110 },   {  44,  55,  27,  55 } }, // Galleon
                                                      
  { {  1,   3,   1,   3 },   { 13,  30,  13,  30 },   {  50,  63,  37,  63 },   {  22,  27,  13,  27 } }, // Corvette
  { {  2,   3,   1,   3 },   { 30,  40,  30,  40 },   {  67,  84,  50,  84 },   {  29,  36,  18,  36 } }, // Destroyer
  { {  2,   3,   2,   3 },   { 34,  45,  34,  45 },   {  87, 109,  65, 109 },   {  38,  47,  23,  47 } }, // Cruiser
  { {  2,   4,   2,   4 },   { 42,  60,  42,  60 },   { 122, 153,  91, 153 },   {  53,  66,  33,  66 } }, // Frigate
  { {  3,   5,   2,   5 },   { 52,  75,  52,  75 },   { 153, 191, 115, 191 },   {  66,  83,  41,  83 } }, // Dreadnought
};                                                                              

extern const int ship_allowed_weapons[MAXSHIPCLASS][MAXWEAPON];
const int ship_allowed_weapons [MAXSHIPCLASS][MAXWEAPON] = {

//  SmBal MdBal LgBal SmCat MdCat LgCat HvBal SmBmc LgBmc MBlst FrCan LtCat
  {   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   }, // Sloop
  {   1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   }, // Yacht
  {   1,    1,    0,    1,    0,    0,    0,    0,    0,    1,    0,    0,   }, // Clipper
  {   1,    1,    1,    1,    1,    0,    0,    0,    0,    1,    1,    0,   }, // Ketch
  {   1,    1,    1,    1,    1,    1,    1,    1,    0,    1,    1,    0,   }, // Caravel
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Carrack
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Galleon
                                                          
  {   1,    1,    1,    1,    1,    0,    0,    1,    0,    1,    1,    0,   }, // Corvette
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    0,   }, // Destroyer
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Cruiser
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Frigate
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Dreadnought
};


const char *arc_name[4] = {
  "forward",
  "port",
  "rear",
  "starboard"
};

const ulong slotmap[4] = {
  BIT_1,
  BIT_2,
  BIT_3,
  BIT_4
};
const char *ship_symbol[NUM_SECT_TYPES] = {
  "&=wl^^&N",                   /* * larger towns */
  "&+L++&N",                    /* * roads */
  "&+g..&N",                    /* * plains/fields */
  "&+G**&N",                    /* * forest */
  "&+y^^&N",                    /* * hills */
  "&+yMM&N",                    /* * mountains */
  "&=cCrr&N",                   /* * water shallow */
  "&+b~~&N",                    /* * water boat */
  "  ",                         /* * noground */
  "&+b~~&N",                    /* * underwater */
  "&+b~~&N",                    /* * underwater ground */
  "  ",                         /* * fire plane */
  "&+b~~&N",                    /* * water ship */
  "  ",                         /* * UD wild */
  "  ",                         /* * UD city */
  "  ",                         /* * UD inside */
  "  ",                         /* * UD water */
  "  ",                         /* * UD noswim */
  "  ",                         /* * UD noground */
  "  ",                         /* * air plane */
  "  ",                         /* * water plane */
  "  ",                         /* * earth plane */
  "  ",                         /* * etheral plane */
  "  ",                         /* * astral plane */
  "&+Y..&N",                    /* desert */
  "&+C..&N",                    /* arctic tundra */
  "&+M**&N",                    /* swamp */
  "  ",                         /* UD mountains */
  "  ",                         /* UD slime */
  "  ",                         /* UD low ceilings */
  "  ",                         /* UD liquid mithril */
  "  ",                         /* UD mushroom forest */
  "&-w  &N",                    /* Castle Wall */
  "&=wl##&N",                   /* Castle Gate */
  "&=wl..&N",                   /* Castle Itself */
  "  ",                         /* negative plane */
  "  ",                          /* plane of avernus */
  "&+L++&N",                      // roads
  "&=cW**&N",                    /* snowy forest */
};

