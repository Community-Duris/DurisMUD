
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

// DO NOT move crews, just add new ones!
const ShipCrewData ship_crew_data[MAXCREWS] = {
//        Name                                                          Crew  Base     Base    Base     Base     Sail    Gun  Repair     Hire   Hire     Hire rooms
//                                                                     Level  Sail      Gun   Repair  Stamina     Mod    Mod    Mod      Cost  Frags 
    { "&+wAmateur Crew",                                                   0,     0,       0,       0,    500,      0,     0,     0,         0,     0, {     0, 0, 0, 0, 77 }, CF_NONE },
    { "&+yM&+La&+yg&+Li&+yc&+La&+yl &+yAu&+Lto&+yma&+Lto&+yns",           3,  3000,    3000,    2000,   1500,      3,     3,     2,  40000000,  3000, {     0, 0, 0, 0, 77 }, CF_SCOUT_RANGE_2 | CF_MAXSPEED_1 },
    { "&+yNorth&+Wshore &+LPirates",                                       0,   100,     100,     100,    600,      1,     1,     1,         0,     0, {     0, 0, 0, 0, 77 }, CF_NONE },
    { "&+BT&+bw&+Bi&+bl&+Bi&+bg&+Bh&+bt &+LCove &+rBucaneers",             1,   300,     300,     300,    700,      1,     1,     1,         0,     0, {     0, 0, 0, 0, 77 }, CF_NONE },
    { "&+yMa&+Lra&+yud&+Ler&+ys &+Lof the &+cFo&+Cur &+wWi&+Wnds",         2,  1000,    1000,    1000,    800,      1,     1,     1,         0,     0, {     0, 0, 0, 0, 77 }, CF_NONE },
    { "&+LDark &+ySun &+LSyndicate",                                       3,  2500,    2500,    2500,   1000,      1,     2,     1,         0,     0, {     0, 0, 0, 0, 77 }, CF_NONE },
    { "&+rSci&+Rons &+Lof the &+WAb&+wy&+Lss",                             4,  5000,    5000,    5000,   1500,      3,     3,     3,         0,     0, {     0, 0, 0, 0, 77 }, CF_NONE },

    { "&+YSturdy &+BWhalers",                                              0,     0,       0,       0,    750,      0,     0,     0,   1000000,     0, { 43220, 0, 0, 0, 77 }, CF_NONE },
    { "&+GEv&+ge&+Grm&+gee&+Gt &+BCoasters",                               0,   100,     120,     120,    625,      0,     1,     1,   2000000,    90, { 43222, 0, 0, 0, 77 }, CF_NONE },
    { "&+WTh&+wa&+Wrn&+wa&+Wdi&+wa&+Wn &+WR&+wi&+Wgg&+we&+Wrs",            0,   150,      80,     100,    600,      1,     0,     0,   1600000,    80, {133075, 0, 0, 0, 77 }, CF_NONE },
    { "&+MT&+me&+Mk&+ma&+Mn &+MMad&+mcaps",                                1,   550,     450,     400,    700,      1,     1,     0,   5000000,   350, { 28197, 0, 0, 0, 77 }, CF_NONE },

    { "&+GStr&+go&+Gng&+ga&+Grms &+go&+Gf &+GGh&+go&+Gr&+ge ",             0,     0,       0,       0,    780,      0,     0,     0,   1000000,     0, { 43221, 0, 0, 0, 77 }, CF_NONE },
    { "&+GSarmiz'Duul &+BScallywags",                                      0,   150,     100,      80,    600,      1,     0,     0,   1600000,    80, {  9704, 0, 0, 0, 77 }, CF_NONE },
    { "&+WSto&+wrm&+bPort &+rBucaneers",                                   0,   100,     140,     100,    625,      0,     2,     0,   2000000,   100, { 22481, 0, 0, 0, 77 }, CF_NONE },
    { "&+yJu&+wgg&+yer&+Wna&+wug&+Wh&+wt &+LDeserters",                    1,   500,     500,     350,    725,      1,     1,     0,   5000000,   360, { 22648, 0, 0, 0, 77 }, CF_NONE },
    
    { "&+rQuietus &+WPo&+Lwd&+wer &+wMo&+Lnk&+wey&+Ls",                    1,   500,     800,     400,    750,      0,     2,     0,   6000000,   500, {  1734, 0, 0, 0, 77 }, CF_SCOUT_RANGE_1 },
    { "&+BTorrhan &+LBl&+wa&+Lck &+LG&+wa&+Lng",                           1,   550,     450,     750,    750,      0,     0,     2,   6000000,   450, { 66735, 0, 0, 0, 77 }, CF_HULL_REPAIR_2 },

    { "&+WWinterhaven &+YSea&+ymans",                                      2,  1350,    1250,    1350,    800,      1,     0,     1,   9000000,   900, { 55418, 0, 0, 0, 77 }, CF_NONE },
    { "&+MMirabolan &+YMer&+Wcha&+Ynt&+Ws",                                2,  1550,    1300,    1250,    850,      2,     0,     0,  11000000,  1000, { 82641, 0, 0, 0, 77 }, CF_MAXCARGO_10 },
    { "&+WCo&+wr&+Wwe&+wl&+Wl &+CS&+ce&+Ba &+cD&+Bog&+bs",                 2,  1500,    1550,    1250,    800,      2,     2,     0,  12500000,  1200, { 54240, 0, 0, 0, 77 }, CF_SCOUT_RANGE_1 },
    { "&+RBoyard &+BNaval &+WGu&+wa&+Wr&+wd",                              2,  1450,    1650,    1300,    900,      1,     3,     0,  11000000,  1100, { 38107, 0, 0, 0, 77 }, CF_WEAPONS_REPAIR_3 },
    { "&+YVenan'Trut &+RR&+roy&+Ral &+wS&+Whi&+wp&+wwr&+Wigh&+wts",        2,  1250,    1300,    1750,    900,      0,     0,     4,  10000000,  1000, { 49051, 0, 0, 0, 77 }, CF_MAXSPEED_1 | CF_HULL_REPAIR_2 },
    { "&+cCeothian &+BSea&+Cfa&+cr&+Ce&+cr&+Cs",                           2,  1600,    1350,    1250,    800,      3,     0,     0,  12500000,  1200, { 81021, 0, 0, 0, 77 }, CF_SCOUT_RANGE_2 },
    { "&+GJade &+MTei&+mko&+mku &+BN&+co&+Br&+ci&+Bk&+cu&+Bm&+cii&+Bn",    2,  1350,    1500,    1500,    900,      1,     2,     2,  10000000,  1000, { 76859, 0, 0, 0, 77 }, CF_SAIL_REPAIR_3 },
};


const ShipChiefData ship_chief_data[MAXCHIEFS] = {
//      Type           Name                                            Min  Skill   Skill      Hire    Hire     Hire rooms
//                                                                    Skill  Gain    Mod       Cost   Frags
    { NO_CHIEF,    "None",                                               0,     0,      0,          0,      0, {     0,     0, 0, 0, 77 }, CCF_NONE  } ,

    { SAIL_CHIEF,  "&+wDe&+Lck &+wCa&+Lde&+wt",                        200,    10,      1,     800000,    120, { 43222,133075,     0, 0, 77 }, CCF_NONE  } , // TODO: tharn
    { SAIL_CHIEF,  "&+wR&+Lu&+wgg&+Le&+wd &+yHelms&+wman",             200,    10,      1,     800000,    120, {  9704, 22481,     0, 0, 77 }, CCF_NONE  } ,
    { GUNS_CHIEF,  "&+rGunner &+cCadet",                               250,    10,      1,    1200000,    150, { 43220,133075,     0, 0, 77 }, CCF_NONE  } , // TODO: tharn
    { GUNS_CHIEF,  "&+cExperienced &+rCanoneer",                       250,    10,      1,    1200000,    150, {  1734,     0,     0, 0, 77 }, CCF_NONE  } ,
    { RPAR_CHIEF,  "&+wShip&+yw&+wr&+yi&+wg&+yh&+wt &+yTyro",          220,    10,      1,    1000000,    130, { 43220,     0,     0, 0, 77 }, CCF_NONE  } ,
    { RPAR_CHIEF,  "&+wD&+yoc&+wk &+yCa&+wrp&+yen&+wter",              220,    10,      1,    1000000,    130, {  9704,     0,     0, 0, 77 }, CCF_NONE  } ,

    { SAIL_CHIEF,  "&+yOld &+gQ&+wu&+ga&+wr&+gt&+we&+gr&+wmaster",     800,    25,      2,    3000000,    540, {  1734, 82641, 55418, 0, 77 }, CCF_NONE  } ,
    { GUNS_CHIEF,  "&+BMaster &+rGu&+Rnn&+rer",                       1000,    35,      2,    4000000,    700, { 54240, 22481,     0, 0, 77 }, CCF_NONE  } ,
    { RPAR_CHIEF,  "&+yVet&+Ye&+Wr&+Ya&+yn &+yB&+Yo&+Wats&+Yw&+yain",  900,    30,      2,    3500000,    640, { 43221, 66735, 55418, 0, 77 }, CCF_NONE  } ,

    { SAIL_CHIEF,  "&+BChief &+WM&+wa&+Wt&+we",                       2000,    50,      3,    7500000,   1350, { 81021,133075,     0, 0, 77 }, CCF_NONE  } , // TODO: tharn
    { GUNS_CHIEF,  "&+WE&+wl&+Wi&+wte &+RGunner",                     2500,    70,      3,    9000000,   1640, { 76859, 38107,     0, 0, 77 }, CCF_NONE  } ,
    { RPAR_CHIEF,  "&+cE&+wxp&+ce&+wrt &+cE&+wng&+ci&+wn&+cee&+wr",   2200,    60,      3,    8000000,   1480, { 49051, 76859,     0, 0, 77 }, CCF_NONE  } ,
}; 

#define WPNFLAG01   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED                         //small ball        
#define WPNFLAG02   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED                         //med ball          
#define WPNFLAG03   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED                         //lrg ball          
#define WPNFLAG04   FORE_ALLOWED | REAR_ALLOWED                               | BALLISTIC             // small cat         
#define WPNFLAG05   FORE_ALLOWED | REAR_ALLOWED                               | BALLISTIC             //med cat           
#define WPNFLAG06   FORE_ALLOWED | REAR_ALLOWED                               | BALLISTIC             //large cat         
#define WPNFLAG07                                 PORT_ALLOWED | STAR_ALLOWED                         //hvy ball          
#define WPNFLAG08   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED | RANGEDAM  | CAPITAL   //light beam
#define WPNFLAG09   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED | RANGEDAM  | CAPITAL   //heavy beam
#define WPNFLAG10   FORE_ALLOWED | REAR_ALLOWED | PORT_ALLOWED | STAR_ALLOWED | MINDBLAST | CAPITAL   //mind blast        
#define WPNFLAG11   FORE_ALLOWED | REAR_ALLOWED                                           | CAPITAL   //frag cannon       
#define WPNFLAG12   FORE_ALLOWED | REAR_ALLOWED                               | BALLISTIC | CAPITAL   //long tom          

const WeaponData weapon_data[MAXWEAPON] = {
// Name                         Cost  Frags Crew  Weight Ammo    Min     Max     Min     Max  Fragments Damage  Sail   Hull    Sail    Armor   Reload  Volley      Flags
//                                           exp         range   range  damage  damage   count     arc    hit  damage  damage  pierce     time    time 
 { "Small Ballistae",          50000,    0,    0,     3,   60,      0,      8,      2,      4,      1,      10,   12,    100,     50,     10,      30,      7,  WPNFLAG01, }, //small ball  
 { "Medium Ballistae",        100000,    0,    0,     6,   50,      0,     10,      4,      6,      1,      10,   14,    100,     50,     10,      30,      8,  WPNFLAG02, }, //med ball    
 { "Large Ballistae",         500000,    0,    0,    10,   30,      0,     12,      6,      9,      1,      10,   16,    100,     50,     10,      30,     10,  WPNFLAG03, }, //lrg ball    
 { "Small Catapult",          500000,    0,    0,    10,   30,      4,     15,      2,      3,      4,     160,   20,    100,    100,      2,      30,     18,  WPNFLAG04, }, //small cat   
 { "Medium Catapult",         800000,    0,    0,    13,   20,      5,     20,      2,      4,      5,     260,   20,    100,    100,      2,      30,     24,  WPNFLAG05, }, //med cat     
 { "Large Catapult",         1200000,    0,    0,    17,   12,      6,     25,      2,      5,      6,     360,   20,    100,    100,      2,      30,     30,  WPNFLAG06, }, //large cat   
 { "Heavy Ballistae",        1000000,    0,    0,    15,    6,      0,      4,     15,     22,      1,      10,    0,    100,      0,     15,      30,      3,  WPNFLAG07, }, //hvy ball    
 { "Light Beamcannon",       4000000, 1600, 1600,     7,   40,      0,     20,      4,     16,      1,      10,   10,    100,     30,     15,      45,      0,  WPNFLAG08, }, //light beam  
 { "Heavy Beamcannon",       5000000, 1800, 1800,     9,   40,      0,     23,      5,     22,      1,      10,   10,    100,     30,     15,      45,      0,  WPNFLAG09, }, //heavy beam  
 { "Mind Blast Cannon",      4000000, 1700, 1700,     5,   50,      0,     20,      0,      0,      1,     360,    0,      0,      0,      0,      45,      0,  WPNFLAG10, }, //mind blast  
 { "Fragmentation Cannon",   5000000, 1900, 1900,     7,   20,      0,     16,      4,      6,      5,      90,   50,     50,    100,      0,      45,     12,  WPNFLAG11, }, //frag cannon 
 { "Long Tom Catapult",      5000000, 2000, 2000,     9,    6,     12,     32,      3,      6,      8,     360,   20,    100,    100,      3,      45,     36,  WPNFLAG12, }, //long tom    
};

#define EQFLAGS01  0        // Ram
#define EQFLAGS02  CAPITAL  // Levistone
#define EQFLAGS03  DIPLOMAT	// Pirate Suppression Flag

const EquipmentData equipment_data[MAXEQUIPMENT] = {
 // Name                                       Cost  Frags Weight      Flags
 { "&+yBronze&+L Plated Ram",                     0,     0,     0, EQFLAGS01 },
 { "Zentharium Levistone",                  5000000,  1000,     0, EQFLAGS02 },
 { "&+bDi&+Bploma&+bt's &+WFlag&n",               0,     0,     0, EQFLAGS03 }
};

/* NEVER CHANGE SHIP IDs Just add new ones. */
extern const ShipTypeData ship_type_data[MAXSHIPCLASS];
const ShipTypeData ship_type_data[MAXSHIPCLASS] = {

//ID, Ship Type Name,      Cost,  ECost, Hull, Slots, MxWeight, Sail, MxCargo, MxContra, MxPeople,MxSpeed, HDDec, Accel, MinLvl, FreeWpn, FreeCrg,     Kind
 { 1,        "Sloop",    100000,      0,   10,     1,     5,      20,    1,        0,       1,     100,    50,     35,       0,      0,       0,   SHK_MERCHANT },
 { 2,        "Yacht",    300000,      0,   25,     4,    12,      40,    3,        0,       2,     100,    45,     28,       0,      2,       0,   SHK_MERCHANT },
 { 3,      "Clipper",   1500000,      0,  110,     6,    55,      90,   10,        1,       5,      88,    30,     22,      20,      8,       6,   SHK_MERCHANT },
 { 4,        "Ketch",   2500000,      0,  150,     8,    75,     100,   20,        3,       8,      78,    20,     17,      25,     10,       8,   SHK_MERCHANT },
 { 5,      "Caravel",   4000000,      0,  200,     8,   100,     110,   35,        6,      10,      68,    13,     13,      30,     13,      12,   SHK_MERCHANT },
 { 6,      "Carrack",   8000000,      0,  260,    10,   130,     120,   50,        9,      15,      58,     8,     10,      35,     16,      24,   SHK_MERCHANT },
 { 7,      "Galleon",  12000000,      0,  330,    12,   165,     130,   70,       12,      20,      50,     6,      8,      40,     19,      40,   SHK_MERCHANT },
 { 8,     "Corvette",   9000000,      0,  165,     6,    82,     120,    0,        0,       6,      74,    20,     20,      31,     13,       0,   SHK_WARSHIP  },
 { 9,    "Destroyer",  15000000,      0,  220,     8,   110,     130,    0,        0,       8,      64,    13,     14,      36,     16,       0,   SHK_WARSHIP  },
 {10,      "Frigate",  22000000,      0,  285,    10,   142,     140,    0,        0,      10,      55,     8,     10,      41,     20,       0,   SHK_WARSHIP  },
 {11,      "Cruiser",  36000000,      0,  400,    12,   200,     160,    0,        0,      15,      48,     5,      8,      46,     25,       0,   SHK_WARSHIP  },
 {12,  "Dreadnought",         0,      0,  600,    14,   300,     200,    0,        0,      20,      40,     4,      6,      51,     32,       0,   SHK_WARSHIP  },
 {13,   "Large Ship",         0,      0,  600,    14,   300,     200,    0,        0,      20,      40,     4,      6,      51,     32,       0,   SHK_WARSHIP  }

};

extern const ShipArcProperties ship_arc_properties[MAXSHIPCLASS];
const ShipArcProperties ship_arc_properties[MAXSHIPCLASS] = {
//      Weapon slots            Weapon weight                  Armor                   Internal
//   Fwrd Port Rear Stbd      Fwrd Port Rear Stbd       Fwrd Port Rear Stbd       Fwrd Port Rear Stbd  
  { {  0,   0,   0,   0 },   {  0,   0,   0,   0 },   {   2,   3,   1,   3 },   {   1,   1,   1,   1 } }, // Sloop
  { {  1,   1,   1,   1 },   {  3,   5,   3,   5 },   {   6,   8,   4,   8 },   {   3,   4,   2,   4 } }, // Yacht
  { {  1,   2,   1,   2 },   { 10,  13,  10,  13 },   {  29,  36,  18,  36 },   {  14,  18,   9,  18 } }, // Clipper
  { {  1,   2,   1,   2 },   { 13,  20,  13,  20 },   {  40,  50,  25,  50 },   {  20,  25,  12,  25 } }, // Ketch
  { {  1,   3,   1,   3 },   { 17,  26,  17,  26 },   {  53,  66,  33,  66 },   {  26,  33,  16,  33 } }, // Caravel
  { {  1,   3,   1,   3 },   { 21,  31,  21,  31 },   {  69,  86,  43,  86 },   {  34,  43,  21,  43 } }, // Carrack
  { {  2,   3,   1,   3 },   { 26,  35,  26,  35 },   {  88, 110,  55, 110 },   {  44,  55,  27,  55 } }, // Galleon
  { {  1,   3,   1,   3 },   { 13,  32,  13,  32 },   {  50,  63,  37,  63 },   {  22,  27,  13,  27 } }, // Corvette
  { {  2,   3,   1,   3 },   { 27,  38,  27,  38 },   {  67,  84,  50,  84 },   {  29,  36,  18,  36 } }, // Destroyer
  { {  2,   3,   2,   3 },   { 31,  44,  31,  44 },   {  87, 109,  65, 109 },   {  38,  47,  23,  47 } }, // Frigate
  { {  2,   4,   2,   4 },   { 35,  50,  35,  50 },   { 122, 153,  91, 153 },   {  53,  66,  33,  66 } }, // Cruiser
  { {  3,   5,   2,   5 },   { 51,  75,  51,  75 },   { 183, 229, 138, 229 },   {  79,  99,  49,  99 } }, // Dreadnought
  { {  3,   5,   2,   5 },   { 51,  75,  51,  75 },   { 183, 229, 138, 229 },   {  79,  99,  49,  99 } }  // Large Ship
};

extern const int ship_allowed_weapons[MAXSHIPCLASS][MAXWEAPON];
const int ship_allowed_weapons [MAXSHIPCLASS][MAXWEAPON] = {

//  SmBal MdBal LgBal SmCat MdCat LgCat HvBal LtBmc HvBmc MBlst FrCan LtCat
  {   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   }, // Sloop
  {   1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   }, // Yacht
  {   1,    1,    0,    1,    0,    0,    0,    0,    0,    1,    0,    0,   }, // Clipper
  {   1,    1,    1,    1,    1,    0,    0,    1,    0,    1,    1,    0,   }, // Ketch
  {   1,    1,    1,    1,    1,    1,    1,    1,    0,    1,    1,    0,   }, // Caravel
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Carrack
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Galleon
  {   1,    1,    1,    1,    1,    0,    0,    1,    0,    1,    1,    0,   }, // Corvette
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    0,   }, // Destroyer
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Cruiser
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Frigate
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Dreadnought
  {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   }, // Large Ship
};

extern const int ship_allowed_equipment [MAXSHIPCLASS][MAXEQUIPMENT];
const int ship_allowed_equipment [MAXSHIPCLASS][MAXEQUIPMENT] = {

//   Ram  LvStn  Diplomat
  {   0,    0,    1,  }, // Sloop
  {   0,    0,    1,  }, // Yacht
  {   1,    1,    1,  }, // Clipper
  {   1,    1,    1,  }, // Ketch
  {   1,    1,    1,  }, // Caravel
  {   1,    1,    1,  }, // Carrack
  {   1,    1,    1,  }, // Galleon
  {   1,    1,    1,  }, // Corvette
  {   1,    1,    1,  }, // Destroyer
  {   1,    1,    1,  }, // Cruiser
  {   1,    1,    1,  }, // Frigate
  {   1,    1,    1,  }, // Dreadnought
  {   1,    1,    1,  }  // Large Ship
};


const ulong arcbitmap[4] = {
  FORE_BIT,
  PORT_BIT,
  REAR_BIT,
  STAR_BIT
};
const char *ship_symbol[NUM_SECT_TYPES] = {
  "&=wl^^&N",                   // Zones
  "&+L++&N",                    // Roads
  "&+g..&N",                    // Plains/Fields
  "&+G**&N",                    // Forest
  "&+y^^&N",                    // Hills
  "&+yMM&N",                    // Mountains
  "&=cCrr&N",                   // Water-Shallow
  "&+b~~&N",                    // Water-Boat
  "  ",                         // Noground
  "&+b~~&N",                    // Underwater
  "&+b~~&N",                    // Underwater-Ground
  "&-r  &N",                    // Fire Plane
  "&+b~~&N",                    // Water Ship
  "  ",                         // UD Wild
  "  ",                         // UD City
  "  ",                         // UD Inside
  "  ",                         // UD Water
  "  ",                         // UD Noswim
  "  ",                         // UD Noground
  "&-C  &N",                    // Air Plane
  "&-B  &N",                    // Water Plane
  "&-y  &N",                    // Earth Plane
  "&-w  &N",                    // Etheral Plane
  "&-L  &n",                    // Astral Plane
  "&+Y..&N",                    // Desert
  "&+C..&N",                    // Arctic Tundra
  "&+M**&N",                    // Swamp
  "  ",                         // UD Mountains
  "  ",                         // UD Slime
  "  ",                         // UD Low Ceilings
  "  ",                         // UD Liquid Mithril
  "  ",                         // UD Mushroom Forest
  "&-w  &N",                    // Castle Wall
  "&=wl##&N",                   // Castle Gate
  "&=wl..&N",                   // Castle Itself
  "  ",                         // Negative Plane
  "  ",                         // Plane of Avernus
  "&+L++&N",                    // Roads
  "&=cW**&N",                   // Snowy Forest
  "&-r  &N"                     // Lava
};

