#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ships.h"
#include "comm.h"
#include "db.h"
//#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include "map.h"
#include "ship_npc.h"
#include "ship_npc_ai.h"

void  act_to_all_in_ship(P_ship ship, const char *msg);
int   getmap(P_ship ship);
bool load_npc_ship_crew(P_ship ship, int crew_size, int crew_level);

#define LOAD_RANGE 36

extern int top_of_world;



//////////////////////
// NAMES
//////////////////////

const char* pirateShipNames[] = 
{
    "&+WB&+wu&+Lcc&+wan&+Weer&+L'&+ws H&+Wo&+ww&+Ll",
    "&+RD&+rev&+ri&+Rl&+L'&+Rs &+LD&+wea&+Lt&+wh",
    "&+BO&+bce&+Ba&+bn&+L'&+bs &+CM&+cer&+Bma&+ci&+Cd",
    "&+BT&+bhe &+BC&+bru&+Be&+bl &+ME&+me&+Ml",
    "&+wThe &+RB&+rl&+Roo&+rd&+Ry &+RS&+rku&+Rll &+wof &+BAt&+bl&+Ban&+bti&+Bs",
    "&+wThe &+YS&+yha&+Ym&+ye &+Lof &+wthe &+CSe&+cv&+Cen &+CS&+cea&+Cs",
    "&+wThe &+LH&+wo&+Lrr&+wi&+Wb&+wl&+Le &+wD&+Loo&+wm",
    "&+RT&+rhe &+RV&+ri&+Rle &+rH&+Roa&+rr&+Rd",
    "&+LT&+yhe &+LF&+yo&+Lul &+yC&+Lut&+yl&+La&+yss",
    "&+rT&+Lhe &+rA&+Lwf&+ru&+Ll &+rD&+Le&+rmo&+Ln",
    "&+BN&+be&+Bpt&+bun&+Be&+L'&+bs &+GS&+gerp&+Gen&+gt",
    "&+LT&+whe &+LBla&+wc&+Lk &+WTh&+wun&+Wd&+we&+Lr",
    "&+YC&+yal&+Yy&+yps&+Yo&+L'&+ys &+RAn&+rge&+Rr",
    "&+YT&+yhe &+YC&+Wu&+Yrs&+ye&+Yd &+yG&+Yo&+Wl&+Yd",
    "&+LT&+whe &+LDark &+wD&+Lagg&+we&+Lr",
    "&+WT&+whe &+WWa&+wn&+Wde&+wr&+Win&+wg &+bT&+Br&+Cid&+Be&+bnt",
    "&+WJ&+Ye&+Wwe&+Yl &+wof the &+YE&+yas&+Yt",
    "&+GD&+gra&+Ggo&+gns &+LBla&+wc&+Lk &+yPl&+Yund&+Wer",
    "&+WMa&+Cel&+Ws&+Ctr&+Wom &+wof the &+WC&+Ca&+Wrr&+Cibe&+Wan",
    "&+YB&+yu&+Ycc&+yan&+Yee&+yr&+L'&+ys &+wDeath&+Wwish",
    "&+LOl&+we' &+WSk&+wu&+Ll&+Wl N' &+wBo&+Ln&+Wes",
    "&+LThe &+YGo&+yld&+Yen &+LThief",
    "&+cIron &+CCu&+ctla&+Css",
    "&+GJ&+gad&+Ge Dr&+gag&+Gon",
    "&&+WSa&+wlt&+Ly &+yLeu&+Lcro&+ytta",
    "&+LAv&+wer&+Lnu&+Ws &+YR&+ri&+Rs&+rin&+Yg",
};

const char* dreadnoughtShipNames[] = 
{
    "&+RCy&+rri&+Lc's Rev&+ren&+Rge",
};

bool is_npc_ship_name(const char *name)
{
    for (unsigned n = 0; n < sizeof(pirateShipNames)/sizeof(char*); n++)
    {
        if (!strcmp(strip_ansi(name).c_str(), strip_ansi(pirateShipNames[n]).c_str()))
            return true;
    }
    for (unsigned n = 0; n < sizeof(dreadnoughtShipNames)/sizeof(char*); n++)
    {
        if (!strcmp(strip_ansi(name).c_str(), strip_ansi(dreadnoughtShipNames[n]).c_str()))
            return true;
    }
    return false;
}





//////////////////////
// SETUP
//////////////////////

void setup_npc_clipper_01(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, PORT);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(100, 150);
    load_npc_ship_crew(ship, 8, 0);
}
void setup_npc_clipper_02(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_MEDIUM_BAL, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(100, 150);
    load_npc_ship_crew(ship, 8, 0);
}
void setup_npc_clipper_03(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_BAL, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 3, W_SMALL_BAL, REAR);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(150, 200);
    load_npc_ship_crew(ship, 8, 0);
}
void setup_npc_ketch_01(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(150, 200);
    load_npc_ship_crew(ship, 9, 0);
}
void setup_npc_ketch_02(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(200, 250);
    load_npc_ship_crew(ship, 9, 0);
}
void setup_npc_ketch_03(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, REAR);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(200, 250);
    load_npc_ship_crew(ship, 9, 0);
}
void setup_npc_caravel_01(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(250, 300);
    load_npc_ship_crew(ship, 12, 0);
}
void setup_npc_caravel_02(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 2, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(250, 300);
    load_npc_ship_crew(ship, 12, 0);
}
void setup_npc_caravel_03(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    setcrew(ship, sail_crew_list[0], 600000);
    setcrew(ship, gun_crew_list[0], 600000);
    setcrew(ship, repair_crew_list[0], 600000);
    ship->frags = number(400, 500);
    load_npc_ship_crew(ship, 12, 1);
}
void setup_npc_corvette_01(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 2, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 300000);
    setcrew(ship, gun_crew_list[0], 300000);
    setcrew(ship, repair_crew_list[0], 300000);
    ship->frags = number(400, 500);
    load_npc_ship_crew(ship, 12, 1);
}
void setup_npc_corvette_02(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 2, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 300000);
    setcrew(ship, gun_crew_list[0], 300000);
    setcrew(ship, repair_crew_list[0], 300000);
    ship->frags = number(400, 500);
    load_npc_ship_crew(ship, 12, 1);
}
void setup_npc_corvette_03(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, PORT);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 4, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 300000);
    setcrew(ship, gun_crew_list[0], 300000);
    setcrew(ship, repair_crew_list[0], 300000);
    ship->frags = number(400, 500);
    load_npc_ship_crew(ship, 12, 1);
}
void setup_npc_corvette_04(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, PORT);
    set_weapon(ship, 3, W_SMALL_BAL, PORT);
    set_weapon(ship, 4, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 5, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 6, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[1], 800000);
    setcrew(ship, gun_crew_list[2], 800000);
    setcrew(ship, repair_crew_list[1], 800000);
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 12, 2);
}
void setup_npc_corvette_05(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, PORT);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 12, 2);
}    


void setup_npc_corvette_06(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_07(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 2, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 3, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1800000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 800);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_08(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, REAR);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 4, W_SMALL_BAL, PORT);
    set_weapon(ship, 5, W_SMALL_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 800);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_09(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 2, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 3, W_MINDBLAST, STARBOARD);
    setcrew(ship, sail_crew_list[2], 2500000);
    setcrew(ship, gun_crew_list[3], 2500000);
    setcrew(ship, repair_crew_list[2], 2500000);
    ship->frags = number(2000, 2200);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_10(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, PORT);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    set_weapon(ship, 3, W_MINDBLAST, PORT);
    setcrew(ship, sail_crew_list[2], 2500000);
    setcrew(ship, gun_crew_list[3], 2500000);
    setcrew(ship, repair_crew_list[2], 2500000);
    ship->frags = number(2000, 2200);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_11(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_QUARTZ, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, PORT);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    set_weapon(ship, 3, W_LARGE_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 2400000);
    setcrew(ship, gun_crew_list[3], 2400000);
    setcrew(ship, repair_crew_list[2], 2200000);
    ship->frags = number(2000, 2200);
    load_npc_ship_crew(ship, 12, 3);
}

void setup_npc_destroyer_01(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 4, W_SMALL_BAL, PORT);
    set_weapon(ship, 5, W_SMALL_BAL, PORT);
    set_weapon(ship, 6, W_SMALL_BAL, PORT);
    setcrew(ship, sail_crew_list[0], 400000);
    setcrew(ship, gun_crew_list[0], 400000);
    setcrew(ship, repair_crew_list[0], 400000);
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 15, 1);
}

void setup_npc_destroyer_02(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 2, W_LARGE_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[1], 500000);
    setcrew(ship, gun_crew_list[0], 500000);
    setcrew(ship, repair_crew_list[0], 500000);
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 15, 1);
}

void setup_npc_destroyer_03(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, FORE);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 4, W_MEDIUM_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 900);
    load_npc_ship_crew(ship, 15, 2);
}
void setup_npc_destroyer_04(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    set_weapon(ship, 3, W_LARGE_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 900);
    load_npc_ship_crew(ship, 15, 2);
}
void setup_npc_destroyer_05(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 4, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 5, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 6, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(800, 1000);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_06(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, REAR);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 4, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 5, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(800, 1000);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_07(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_LARGE_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 4, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(800, 1000);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_08(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_LARGE_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, PORT);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 900);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_09(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_LARGE_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, PORT);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 4, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 5, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(800, 1000);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_10(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_FRAG_CAN, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, REAR);
    set_weapon(ship, 2, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 3, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 4, W_LARGE_BAL, PORT);
    set_weapon(ship, 5, W_MEDIUM_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 2800000);
    setcrew(ship, gun_crew_list[3], 2500000);
    setcrew(ship, repair_crew_list[1], 2500000);
    ship->frags = number(2200, 2500);
    load_npc_ship_crew(ship, 15, 3);
}

void setup_npc_destroyer_11(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_DARKSTONE, FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 2, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 3, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 4, W_LARGE_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 2800000);
    setcrew(ship, gun_crew_list[3], 2500000);
    setcrew(ship, repair_crew_list[2], 2500000);
    ship->frags = number(2200, 2500);
    load_npc_ship_crew(ship, 15, 3);
}

void setup_npc_destroyer_12(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_DARKSTONE, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, REAR);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    set_weapon(ship, 3, W_LARGE_BAL, PORT);
    set_weapon(ship, 4, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 5, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 6, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 2800000);
    setcrew(ship, gun_crew_list[3], 2500000);
    setcrew(ship, repair_crew_list[2], 2500000);
    ship->frags = number(2200, 2500);
    load_npc_ship_crew(ship, 15, 3);
}

void setup_npc_destroyer_13(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, REAR);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 4, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 5, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 6, W_MINDBLAST, REAR);
    setcrew(ship, sail_crew_list[2], 2800000);
    setcrew(ship, gun_crew_list[3], 2500000);
    setcrew(ship, repair_crew_list[2], 2500000);
    ship->frags = number(2200, 2500);
    load_npc_ship_crew(ship, 15, 3);
}

void setup_npc_frigate_01(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_LARGE_CAT, FORE);
    set_weapon(ship, 1, W_FRAG_CAN, FORE);
    set_weapon(ship, 2, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 3, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 4, W_LARGE_BAL, PORT);
    set_weapon(ship, 5, W_LARGE_BAL, PORT);
    set_weapon(ship, 6, W_SMALL_CAT, REAR);
    setcrew(ship, sail_crew_list[2], 4000000);
    setcrew(ship, gun_crew_list[3], 3000000);
    setcrew(ship, repair_crew_list[2], 2500000);
    ship->frags = number(2500, 3000);
    load_npc_ship_crew(ship, 18, 3);
}

void setup_npc_frigate_02(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 2, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 3, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 4, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 5, W_LARGE_BAL, PORT);
    set_weapon(ship, 6, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 7, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 8, W_MINDBLAST, REAR);
    setcrew(ship, sail_crew_list[2], 4000000);
    setcrew(ship, gun_crew_list[3], 3000000);
    setcrew(ship, repair_crew_list[2], 2500000);
    ship->frags = number(2500, 3000);
    load_npc_ship_crew(ship, 18, 3);
}


void setup_npc_dreadnought_01(P_ship ship, NPC_AI_Type type) // level 4
{
    set_weapon(ship, 0, W_LONGTOM, FORE);
    set_weapon(ship, 1, W_FRAG_CAN, FORE);
    set_weapon(ship, 2, W_LARGE_CAT, FORE);
    set_weapon(ship, 3, W_HEAVY_BAL, STARBOARD);
    set_weapon(ship, 4, W_HEAVY_BAL, STARBOARD);
    set_weapon(ship, 5, W_HEAVY_BAL, STARBOARD);
    set_weapon(ship, 6, W_HEAVY_BAL, STARBOARD);
    set_weapon(ship, 7, W_LARGE_BAL, PORT);
    set_weapon(ship, 8, W_LARGE_BAL, PORT);
    set_weapon(ship, 9, W_LARGE_BAL, PORT);
    set_weapon(ship,10, W_LARGE_BAL, PORT);
    set_weapon(ship,11, W_LARGE_BAL, PORT);
    set_weapon(ship,12, W_SMALL_CAT, REAR);
    set_weapon(ship,13, W_SMALL_CAT, REAR);
    setcrew(ship, sail_crew_list[3], 7500000);
    setcrew(ship, gun_crew_list[4], 7500000);
    setcrew(ship, repair_crew_list[2], 7500000);
    ship->frags = number(3000, 4000);
    load_npc_ship_crew(ship, 20, 4);
}

void setup_npc_dreadnought_02(P_ship ship, NPC_AI_Type type) // level 4
{
    set_weapon(ship, 0, W_LONGTOM, FORE);
    set_weapon(ship, 1, W_LARGE_CAT, FORE);
    set_weapon(ship, 2, W_LARGE_CAT, FORE);
    set_weapon(ship, 3, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 4, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 5, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 6, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 7, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 8, W_LARGE_BAL, PORT);
    set_weapon(ship, 9, W_LARGE_BAL, PORT);
    set_weapon(ship,10, W_LARGE_BAL, PORT);
    set_weapon(ship,11, W_LARGE_BAL, PORT);
    set_weapon(ship,12, W_LARGE_BAL, PORT);
    set_weapon(ship,13, W_MINDBLAST, REAR);
    setcrew(ship, sail_crew_list[3], 7500000);
    setcrew(ship, gun_crew_list[4], 7500000);
    setcrew(ship, repair_crew_list[2], 7500000);
    ship->frags = number(3000, 4000);
    load_npc_ship_crew(ship, 20, 4);
}


NPCShipSetup npcShipSetup [] = {
    { SH_CLIPPER,     0, &setup_npc_clipper_01 },
    { SH_CLIPPER,     0, &setup_npc_clipper_02 },
    { SH_CLIPPER,     0, &setup_npc_clipper_03 },
    { SH_KETCH,       0, &setup_npc_ketch_01 },
    { SH_KETCH,       0, &setup_npc_ketch_02 },
    { SH_KETCH,       0, &setup_npc_ketch_03 },
    { SH_CARAVEL,     0, &setup_npc_caravel_01 },
    { SH_CARAVEL,     0, &setup_npc_caravel_02 },
    { SH_CARAVEL,     1, &setup_npc_caravel_03 },
    { SH_CORVETTE,    1, &setup_npc_corvette_01 },
    { SH_CORVETTE,    1, &setup_npc_corvette_02 },
    { SH_CORVETTE,    1, &setup_npc_corvette_03 },
    { SH_CORVETTE,    2, &setup_npc_corvette_04 },
    { SH_CORVETTE,    2, &setup_npc_corvette_05 },
    { SH_CORVETTE,    2, &setup_npc_corvette_06 },
    { SH_CORVETTE,    2, &setup_npc_corvette_07 },
    { SH_CORVETTE,    2, &setup_npc_corvette_08 },
    { SH_CORVETTE,    3, &setup_npc_corvette_09 },
    { SH_CORVETTE,    3, &setup_npc_corvette_10 },
    { SH_CORVETTE,    3, &setup_npc_corvette_11 },
    { SH_DESTROYER,   1, &setup_npc_destroyer_01 },
    { SH_DESTROYER,   1, &setup_npc_destroyer_02 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_03 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_04 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_05 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_06 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_07 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_08 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_09 },
    { SH_DESTROYER,   3, &setup_npc_destroyer_10 },
    { SH_DESTROYER,   3, &setup_npc_destroyer_11 },
    { SH_DESTROYER,   3, &setup_npc_destroyer_12 },
    { SH_FRIGATE,     3, &setup_npc_frigate_01 },
    { SH_FRIGATE,     3, &setup_npc_frigate_02 },
    { SH_DREADNOUGHT, 4, &setup_npc_dreadnought_01 },
    { SH_DREADNOUGHT, 4, &setup_npc_dreadnought_02 },
};




//////////////////////
// LOADING
//////////////////////

P_char dbg_char = 0;
P_ship load_npc_ship(int level, NPC_AI_Type type, int speed, int m_class, int room, P_char ch)
{
    int num = number(0, sizeof(npcShipSetup) / sizeof(NPCShipSetup) - 1);

    int i = 0, ii = 0;
    while (true)
    {
        if ((npcShipSetup[i].m_class == m_class || m_class == -1) && (SHIPTYPESPEED(npcShipSetup[i].m_class) >= speed || speed == -1) && npcShipSetup[i].level == level)
        {
            if (ii == num)
                break;
            ii++;
        }
        i++;
        if (i == sizeof(npcShipSetup) / sizeof(NPCShipSetup))
        {
            if (ii == 0)
                return 0; // there is no such ship setup in list
            i = 0;
        }
    }
    
    dbg_char = ch;


    P_ship ship = newship(npcShipSetup[i].m_class, true);
    if (!ship)
    {
        if (ch) send_to_char("Couldn't create npc ship!\r\n", ch);
        return false;
    }
    if (!ship->panel) 
    {
        if (ch) send_to_char("No panel!\r\n", ch);
        return false;
    }

    ship->race = NPCSHIP;
    ship->npc_ai = new NPCShipAI(ship, ch);
    ship->ownername = 0;

    int name_index = number(0, sizeof(pirateShipNames)/sizeof(char*) - 1);
    nameship(pirateShipNames[name_index], ship);
    assignid(ship, NULL, true);

    npcShipSetup[i].setup(ship, type);
    
    if (!loadship(ship, room))
    {
        if (ch) send_to_char("Couldnt load npc ship!\r\n", ch);
        return false;
    }
    
    ship->anchor = 0;
    REMOVE_BIT(ship->flags, DOCKED);
    return ship;
}

P_ship try_load_npc_ship(P_ship target)
{
    if (target->m_class == SH_SLOOP || target->m_class == SH_YACHT)
        return 0;

    NPC_AI_Type type = NPC_AI_PIRATE;
    int level = 0;
    int n = number(0, SHIPHULLWEIGHT(target)) + target->frags;
    if (ISMERCHANT(target))
    {
        if (n < 200)
        {
            level = 0;
        }
        else if (n < 800)
        {
            level = 1;
        }
        else if (n < 1600 || number(1, 3) != 1)
        {
            level = 2;
            if (number(1, 3) == 1)
                type = NPC_AI_HUNTER;
        }
        else
        {
            level = 3;
            type = NPC_AI_HUNTER;
        }
    }
    else
    {
        if (n < number(1, 1000))
            return 0;
        type = NPC_AI_HUNTER;
        if (number(1, 3) == 1)
        {
            level = 3;
        }
        else
        {
            level = 2;
        }
    }

    P_ship ship = try_load_npc_ship(target, 0, type, level);

    if (ship)
    {
        statuslog(AVATAR, "%s's ship is attacked by a %s!", target->ownername, (type == NPC_AI_PIRATE) ? "pirate" : ((type == NPC_AI_HUNTER) ? "hunter" : "unknown"));
    }

    return ship;
}

P_ship try_load_npc_ship(P_ship target, P_char ch, NPC_AI_Type type, int level)
{
    getmap(target);

    int dir = target->heading + number(-45, 45);
    NPCShipAI::normalize_direction(dir);

    float rad = (float) ((float) (dir) * M_PI / 180.000);
    float ship_x = 50 + sin(rad) * LOAD_RANGE;
    float ship_y = 50 + cos(rad) * LOAD_RANGE;

    int location = tactical_map[(int) ship_x][100 - (int) ship_y].rroom;

    //char ttt[100];
    //sprintf(ttt, "Location: x=%d, y=%d, loc=%d, sect=%d\r\n", (int)ship_x, (int)ship_y, location, world[location].sector_type);
    //if (ch) send_to_char(ttt, ch);

    P_ship ship = try_load_npc_ship(location, target, ch, type, level);
    if (!ship) 
        return 0;

    ship->npc_ai->advanced = false;
    if (SHIPHULLWEIGHT(ship) < 200)
    {
        if (level == 1 && number(1, 3) == 1)
            ship->npc_ai->advanced = true;
        if (level == 2 && number(1, 3) < 3)
            ship->npc_ai->advanced = true;
        if (level == 3)
            ship->npc_ai->advanced = true;
    }
    else if (SHIPHULLWEIGHT(ship) < 250)
    {
        if (level == 1 && number(1, 5) == 1)
            ship->npc_ai->advanced = true;
        if (level == 2 && number(1, 3) == 1)
            ship->npc_ai->advanced = true;
        if (level == 3 && number(1, 2) == 1)
            ship->npc_ai->advanced = true;
    }

    ship->target = target;
    int ship_heading = dir + 180;
    NPCShipAI::normalize_direction(ship_heading);
    ship->setheading = ship_heading;
    ship->heading = ship_heading;
    int ship_speed = ship->get_maxspeed();
    ship->setspeed = ship_speed;
    ship->speed = ship_speed;

    return ship;
}

P_ship try_load_npc_ship(int location, P_ship target, P_char ch, NPC_AI_Type type, int level)
{
    if (world[location].sector_type != SECT_OCEAN)
    {
        if (ch) send_to_char("Wrong location type\r\n", ch);
        return 0;
    }

    int target_speed = target ? SHIPTYPESPEED(target->m_class) : -1;
    P_ship ship = load_npc_ship(level, type, target_speed, -1, location, ch);
    if (!ship)
        ship = load_npc_ship(level, type, 0, -1, location, ch);
    if (!ship)
        return 0;

    ship->setheading = ship->heading = 0;
    ship->setspeed = ship->speed = 0;
    ship->target = 0;
    ship->npc_ai->mode = NPC_AI_CRUISING;
    return ship;
}

bool try_unload_npc_ship(P_ship ship)
{
    if (ship->timer[T_MAINTENANCE] == 1)
    {
        everyone_get_out_newship(ship);
        shipObjHash.erase(ship);
        delete_ship(ship, true);
        return TRUE;
    }
    return FALSE;
}


P_ship npc_dreadnought = 0;

bool load_npc_dreadnought()
{
    if (npc_dreadnought != 0)
        return false;

    for (int i = 0; i < 10; i++)
    {
        int room = number (0, top_of_world);
        if (world[room].sector_type != SECT_OCEAN)
            continue;
        if ((npc_dreadnought = try_load_npc_ship(room, 0, 0, NPC_AI_HUNTER, 4)) != 0)
        {
            int name_index = number(0, sizeof(dreadnoughtShipNames)/sizeof(char*) - 1);
            nameship(dreadnoughtShipNames[name_index], npc_dreadnought);
            npc_dreadnought->npc_ai->permanent = true;
            return true;
        }
    }
    return false;
}


/////////////////////////////
//  CREWS
/////////////////////////////

NPCShipCrewData npcShipCrewData[] = 
{
  {
      0,
      40220,
      40221,
      { 40222, 40223, 40224, 40225, 40227, 0, 0, 0, 0, 0 },
      { 40226, 40230, 0, 0, 0, 0, 0, 0, 0, 0},
      { 40228, 40229, 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  {
      1,
      40231,
      40232,
      { 40233, 40234, 40235, 40236, 40238, 0, 0, 0, 0, 0 },
      { 40237, 40241, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 40239, 40240, 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  {
      2,
      40242,
      40243,
      { 40244, 40245, 40246, 40247, 40249, 0, 0, 0, 0, 0 },
      { 40248, 40252, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 40250, 40251, 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  {
      3,
      40253,
      40254,
      { 40255, 40256, 40257, 40258, 40260, 0, 0, 0, 0, 0 },
      { 40259, 40263, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 40261, 40262, 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  {
      4,
      40264,
      40265,
      { 40266, 40268, 40267, 40269, 40272, 0, 0, 0, 0, 0 },
      { 40273, 40274, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 40270, 40271, 0, 0, 0, 0, 0, 0, 0, 0 },
  }
};


int npc_ship_crew_member_func(P_char ch, P_char player, int cmd, char *arg)
{
    if (cmd != CMD_MOB_MUNDANE)
    {
        return FALSE;
    }
    if (zone_table[world[ch->in_room].zone].number != 600 && !IS_FIGHTING(ch))
    {
        extract_char(ch);
    }
    return FALSE;
}

int npc_ship_crew_captain_func(P_char ch, P_char player, int cmd, char *arg)
{
    return npc_ship_crew_member_func(ch, player, cmd, arg);
}

int npc_ship_crew_board_func(P_char ch, P_char player, int cmd, char *arg)
{
    return npc_ship_crew_member_func(ch, player, cmd, arg);
}




void assign_ship_crew_funcs()
{
    for (unsigned c = 0; c < sizeof(npcShipCrewData) / sizeof(NPCShipCrewData); c++)
    {
        NPCShipCrewData* crew_data = npcShipCrewData + c;

        if (crew_data->captain_mob)
            mob_index[real_mobile0(crew_data->captain_mob)].func.mob = npc_ship_crew_captain_func;
        if (crew_data->firstmate_mob)
            mob_index[real_mobile0(crew_data->firstmate_mob)].func.mob = npc_ship_crew_member_func;
        for (unsigned s = 0; s < sizeof(crew_data->spec_mobs) / sizeof(int); s++)
        {
            if (crew_data->spec_mobs[s])
                mob_index[real_mobile0(crew_data->spec_mobs[s])].func.mob = npc_ship_crew_member_func;
        }

        for (unsigned i = 0; i < sizeof(crew_data->inner_grunts) / sizeof(int); i++)
        {
            if (crew_data->inner_grunts[i])
                mob_index[real_mobile0(crew_data->inner_grunts[i])].func.mob = npc_ship_crew_member_func;
        }

        for (unsigned o = 0; o < sizeof(crew_data->outer_grunts) / sizeof(int); o++)
        {
            if (crew_data->outer_grunts[o])
                mob_index[real_mobile0(crew_data->outer_grunts[o])].func.mob = npc_ship_crew_board_func;
        }
    }
}

static int treasure_chests[5] = { 40215, 40216, 40217, 40218, 40219 };
static int treasure_chest_keys[5] = { 40220, 40221, 40222, 40223, 50224 };


bool load_treasure_chest(P_ship ship, P_char captain, int level)
{
    int r_num;
    if ((r_num = real_object(treasure_chests[level])) < 0)
        return false;
    P_obj chest = read_object(r_num, REAL);
    if (!chest)
        return false;
    obj_to_room(chest, real_room0(ship->bridge));

    if ((r_num = real_object(treasure_chest_keys[level])) < 0)
        return false;
    P_obj key = read_object(r_num, REAL);
    if (!key)
        return false;
    obj_to_char(key, captain);

    int money = 0;
    switch (level)
    {
    case 0: money = number(200, 400); break;
    case 1: money = number(500, 600); break;
    case 2: money = number(800, 1000); break;
    case 3: money = number(1200, 1500); break;
    case 4: money = number(3000, 5000); break;
    };

    P_obj money_obj = create_money(0, 0, 0, money);
    obj_to_obj(money_obj, chest);
    return true;
}


void apply_zone_modifier(P_char ch);
P_char load_npc_ship_crew_member(P_ship ship, int room_no, int vnum)
{
    int room = real_room0(room_no);
    int rnum;
    if ((rnum = real_mobile(vnum)) < 0)
    {
        return 0;
    }
    P_char mob = read_mobile(rnum, REAL);
    if (!mob)
    {
        return 0;
    }
    GET_BIRTHPLACE(mob) = world[room].number;
    apply_zone_modifier(mob);
    char_to_room(mob, room, 0);
    return mob;
}

bool load_npc_ship_crew(P_ship ship, int crew_size, int crew_level)
{
    int total_crews = sizeof(npcShipCrewData) / sizeof(NPCShipCrewData);
    int i = 0;
    for (; i < total_crews; i++)
    {
        if (npcShipCrewData[i].level == crew_level)
            break;
    }
    if (i == total_crews)
    {
        return false;
    }

    int spec_load = 1;
    if (crew_size <= 8)
        spec_load = 3;
    else if (crew_size <= 12)
        spec_load = 2;


    NPCShipCrewData* crew_data = npcShipCrewData + i;
    int loaded = 0;
    P_char captain = load_npc_ship_crew_member(ship, ship->bridge, crew_data->captain_mob);
    if (!captain) return false;
    loaded += 1;

    if (crew_size > 4)
    {
        if (!load_npc_ship_crew_member(ship, ship->bridge, crew_data->firstmate_mob)) return false;
        loaded += 1;
    }


    if (crew_size > 5)
    {
        int spec_load = MAX(crew_size - 5, 5);

        for (unsigned s = 0; s < sizeof(crew_data->spec_mobs) / sizeof(int) && crew_data->spec_mobs[s] != 0 && loaded < crew_size - 3; s++)
        {
            if (number(1, 6 - spec_load) > 1) continue;
            if (!load_npc_ship_crew_member(ship, ship->bridge, crew_data->spec_mobs[s])) return false;
            loaded++;
        }
    }

    unsigned grunt_count = 0;
    for (; grunt_count < sizeof(crew_data->inner_grunts) / sizeof(int); grunt_count++)
        if (crew_data->inner_grunts[grunt_count] == 0)
            break;

    if (grunt_count)
    {
        while (loaded < crew_size)
        {
            if (!load_npc_ship_crew_member(ship, ship->bridge, crew_data->inner_grunts[number(0, grunt_count - 1)])) return false;
            loaded++;
        }
    }

    ship->npc_ai->crew_data = crew_data;

    load_treasure_chest(ship, captain, crew_level);
    return true;
}



// Problems:
// if you cant hit target for too long in advanced mode, switch to basic
// Take damage_ready into account
