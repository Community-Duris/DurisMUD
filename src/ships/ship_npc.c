#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ships.h"
#include "comm.h"
#include "db.h"
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
    "&+WSa&+wlt&+Ly &+yLeu&+Lcro&+ytta",
    "&+LAv&+wer&+Lnu&+Ws &+YR&+ri&+Rs&+rin&+Yg",
    "&+LWi&+wdow&+Le&+Wr's &+wSo&+Lrr&+wo&+Ww",
    "&+WTh&+we &+RPa&+Wi&+Rns&+ree&+Rk&+rer",
    "&+RHe&+ra&+yr&+Rt&+wl&+Le&+wss &+yB&+Yea&+yst",
    "&+WT&+whe &+GV&+Wi&+Gl&+ge H&+Gan&+Wg&+Gma&+gn",
    "&+WTh&+we &+CN&+Wor&+Ct&+ch &+CW&+Bi&+Wn&+cd",
    "&+RRev&+ren&+wg&+Le &+wof the &+LD&+wa&+Lrk &+RD&+rem&+Ron",
    "&+WT&+whe &+BC&+Ch&+Wi&+Cll&+Wi&+Cn&+Bg &+RB&+wa&+Wnn&+we&+Rr",
    "&+YF&+Wea&+yr&+ws&+Wom&+Ye &+YF&+yis&+Yt&+ys",
    "&+LBl&+wu&+Lnt &+WH&+war&+Wpo&+won",
    "&+WT&+whe &+YGo&+Wl&+Yd&+we&+Yn &+YN&+Wu&+Ygg&+Wet",
    "&+WT&+whe &+RW&+rr&+Rat&+rh &+wof the &+BT&+bi&+Bta&bn",
    "&+GS&+Yi&+Gre&+Yn&+w's &+GS&+Wo&+Gng",
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

bool  load_npc_ship_crew(P_ship ship, int crew_size, int crew_level);
void setup_npc_clipper_01(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, SIDE_FORE);
    //set_weapon(ship, 1, W_SMALL_BAL, SIDE_PORT);
    //set_weapon(ship, 2, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_SMALL_BAL, SIDE_REAR);
    ship->crew.sail_skill = 200;
    ship->crew.guns_skill = 200;
    ship->crew.rpar_skill = 200;
    ship->frags = number(100, 150);
    load_npc_ship_crew(ship, 8, 0);
}
void setup_npc_clipper_02(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_MEDIUM_BAL, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_SMALL_BAL, SIDE_STAR);
    ship->crew.sail_skill = 200;
    ship->crew.guns_skill = 200;
    ship->crew.rpar_skill = 200;
    ship->frags = number(100, 150);
    load_npc_ship_crew(ship, 8, 0);
}
void setup_npc_clipper_03(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_BAL, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 3, W_SMALL_BAL, SIDE_REAR);
    ship->crew.sail_skill = 200;
    ship->crew.guns_skill = 200;
    ship->crew.rpar_skill = 200;
    ship->frags = number(150, 200);
    load_npc_ship_crew(ship, 8, 0);
}
void setup_npc_ketch_01(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_STAR);
    //set_weapon(ship, 2, W_SMALL_BAL, SIDE_STAR);
    ship->crew.sail_skill = 200;
    ship->crew.guns_skill = 200;
    ship->crew.rpar_skill = 200;
    ship->frags = number(150, 200);
    load_npc_ship_crew(ship, 9, 0);
}
void setup_npc_ketch_02(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 2, W_SMALL_BAL, SIDE_PORT);
    //set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_STAR);
    //set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_PORT);
    ship->crew.sail_skill = 200;
    ship->crew.guns_skill = 200;
    ship->crew.rpar_skill = 200;
    ship->frags = number(200, 250);
    load_npc_ship_crew(ship, 9, 0);
}
void setup_npc_ketch_03(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_CAT, SIDE_REAR);
    ship->crew.sail_skill = 200;
    ship->crew.guns_skill = 200;
    ship->crew.rpar_skill = 200;
    ship->frags = number(200, 250);
    load_npc_ship_crew(ship, 9, 0);
}
void setup_npc_caravel_01(P_ship ship, NPC_AI_Type type) // level 0
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 2, W_SMALL_BAL, SIDE_STAR);
    //set_weapon(ship, 3, W_SMALL_BAL, SIDE_STAR);
    ship->crew.sail_skill = 200;
    ship->crew.guns_skill = 200;
    ship->crew.rpar_skill = 200;
    ship->frags = number(250, 300);
    load_npc_ship_crew(ship, 12, 0);
}
void setup_npc_caravel_02(P_ship ship, NPC_AI_Type type) // level 0
{
    //set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 0, W_SMALL_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_STAR);
    ship->crew.sail_skill = 200;
    ship->crew.guns_skill = 200;
    ship->crew.rpar_skill = 200;
    ship->frags = number(250, 300);
    load_npc_ship_crew(ship, 12, 0);
}
void setup_npc_caravel_03(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_PORT);
    ship->crew.sail_skill = 600;
    ship->crew.guns_skill = 600;
    ship->crew.rpar_skill = 600;
    ship->frags = number(400, 500);
    load_npc_ship_crew(ship, 12, 1);
}
void setup_npc_corvette_01(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_SMALL_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_STAR);
    ship->crew.sail_skill = 300;
    ship->crew.guns_skill = 300;
    ship->crew.rpar_skill = 300;
    ship->frags = number(400, 500);
    load_npc_ship_crew(ship, 12, 1);
}
void setup_npc_corvette_02(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_SMALL_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_STAR);
    ship->crew.sail_skill = 300;
    ship->crew.guns_skill = 300;
    ship->crew.rpar_skill = 300;
    ship->frags = number(400, 500);
    load_npc_ship_crew(ship, 12, 1);
}
void setup_npc_corvette_03(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_SMALL_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_SMALL_BAL, SIDE_STAR);
    ship->crew.sail_skill = 300;
    ship->crew.guns_skill = 300;
    ship->crew.rpar_skill = 300;
    ship->frags = number(400, 500);
    load_npc_ship_crew(ship, 12, 1);
}
void setup_npc_corvette_04(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_SMALL_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 4, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 6, W_SMALL_BAL, SIDE_STAR);
    ship->crew.sail_skill = 1200;
    ship->crew.guns_skill = 1200;
    ship->crew.rpar_skill = 1200;
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 12, 2);
}
void setup_npc_corvette_05(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_PORT);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 12, 2);
}    


void setup_npc_corvette_06(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_PORT);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_07(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 3, W_MEDIUM_BAL, SIDE_STAR);
    ship->crew.sail_skill = 1800;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(700, 800);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_08(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_CAT, SIDE_REAR);
    set_weapon(ship, 2, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 3, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 5, W_SMALL_BAL, SIDE_PORT);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(700, 800);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_09(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 3, W_MINDBLAST, SIDE_STAR);
    ship->crew.sail_skill = 2500;
    ship->crew.guns_skill = 2500;
    ship->crew.rpar_skill = 2500;
    ship->frags = number(2000, 2200);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_10(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_MINDBLAST, SIDE_PORT);
    ship->crew.sail_skill = 2500;
    ship->crew.guns_skill = 2500;
    ship->crew.rpar_skill = 2500;
    ship->frags = number(2000, 2200);
    load_npc_ship_crew(ship, 12, 2);
}

void setup_npc_corvette_11(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_LIGHT_BEAM, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_LARGE_BAL, SIDE_PORT);
    ship->crew.sail_skill = 2400;
    ship->crew.guns_skill = 2400;
    ship->crew.rpar_skill = 2200;
    ship->frags = number(2000, 2200);
    load_npc_ship_crew(ship, 12, 3);
}

void setup_npc_destroyer_01(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 2, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 3, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 5, W_SMALL_BAL, SIDE_PORT);
    set_weapon(ship, 6, W_SMALL_BAL, SIDE_PORT);
    ship->crew.sail_skill = 400;
    ship->crew.guns_skill = 400;
    ship->crew.rpar_skill = 400;
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 15, 1);
}

void setup_npc_destroyer_02(P_ship ship, NPC_AI_Type type) // level 1
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_STAR);
    ship->crew.sail_skill = 500;
    ship->crew.guns_skill = 500;
    ship->crew.rpar_skill = 500;
    ship->frags = number(600, 700);
    load_npc_ship_crew(ship, 15, 1);
}

void setup_npc_destroyer_03(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_CAT, SIDE_FORE);
    set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 4, W_MEDIUM_BAL, SIDE_PORT);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(700, 900);
    load_npc_ship_crew(ship, 15, 2);
}
void setup_npc_destroyer_04(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_LARGE_BAL, SIDE_STAR);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(700, 900);
    load_npc_ship_crew(ship, 15, 2);
}
void setup_npc_destroyer_05(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 4, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 6, W_SMALL_BAL, SIDE_STAR);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(800, 1000);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_06(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_CAT, SIDE_REAR);
    set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 4, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_MEDIUM_BAL, SIDE_STAR);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(800, 1000);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_07(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_MEDIUM_BAL, SIDE_STAR);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(800, 1000);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_08(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, SIDE_PORT);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(700, 900);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_09(P_ship ship, NPC_AI_Type type) // level 2
{
    set_weapon(ship, 0, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_SMALL_BAL, SIDE_STAR);
    ship->crew.sail_skill = 1500;
    ship->crew.guns_skill = 1500;
    ship->crew.rpar_skill = 1500;
    ship->frags = number(800, 1000);
    load_npc_ship_crew(ship, 15, 2);
}

void setup_npc_destroyer_10(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_FRAG_CAN, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_CAT, SIDE_REAR);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 3, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 5, W_MEDIUM_BAL, SIDE_PORT);
    ship->crew.sail_skill = 2800;
    ship->crew.guns_skill = 2500;
    ship->crew.rpar_skill = 2500;
    ship->frags = number(2200, 2500);
    load_npc_ship_crew(ship, 15, 3);
}

void setup_npc_destroyer_11(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_HEAVY_BEAM, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 3, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_LARGE_BAL, SIDE_STAR);
    ship->crew.sail_skill = 2800;
    ship->crew.guns_skill = 2500;
    ship->crew.rpar_skill = 2500;
    ship->frags = number(2200, 2500);
    load_npc_ship_crew(ship, 15, 3);
}

void setup_npc_destroyer_12(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_HEAVY_BEAM, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_CAT, SIDE_REAR);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 4, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_SMALL_BAL, SIDE_STAR);
    set_weapon(ship, 6, W_SMALL_BAL, SIDE_STAR);
    ship->crew.sail_skill = 2800;
    ship->crew.guns_skill = 2500;
    ship->crew.rpar_skill = 2500;
    ship->frags = number(2200, 2500);
    load_npc_ship_crew(ship, 15, 3);
}

void setup_npc_destroyer_13(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_SMALL_CAT, SIDE_REAR);
    set_weapon(ship, 2, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 4, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 6, W_MINDBLAST, SIDE_REAR);
    ship->crew.sail_skill = 2800;
    ship->crew.guns_skill = 2500;
    ship->crew.rpar_skill = 2500;
    ship->frags = number(2200, 2500);
    load_npc_ship_crew(ship, 15, 3);
}

void setup_npc_frigate_01(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_FRAG_CAN, SIDE_FORE);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 3, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 5, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 6, W_SMALL_CAT, SIDE_REAR);
    ship->crew.sail_skill = 4000;
    ship->crew.guns_skill = 3000;
    ship->crew.rpar_skill = 2500;
    ship->frags = number(2500, 3000);
    load_npc_ship_crew(ship, 18, 3);
}

void setup_npc_frigate_02(P_ship ship, NPC_AI_Type type) // level 3
{
    set_weapon(ship, 0, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, SIDE_FORE);
    set_weapon(ship, 2, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 3, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_MEDIUM_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 6, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 7, W_MEDIUM_BAL, SIDE_PORT);
    set_weapon(ship, 8, W_MINDBLAST, SIDE_REAR);
    ship->crew.sail_skill = 4000;
    ship->crew.guns_skill = 3000;
    ship->crew.rpar_skill = 2500;
    ship->frags = number(2500, 3000);
    load_npc_ship_crew(ship, 18, 3);
}

void setup_npc_dreadnought_01(P_ship ship, NPC_AI_Type type) // level 4
{
    set_weapon(ship, 0, W_LONGTOM, SIDE_FORE);
    set_weapon(ship, 1, W_FRAG_CAN, SIDE_FORE);
    set_weapon(ship, 2, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 3, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_HEAVY_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 6, W_HEAVY_BAL, SIDE_STAR);
    set_weapon(ship, 7, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 8, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 9, W_HEAVY_BAL, SIDE_PORT);
    set_weapon(ship,10, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship,11, W_HEAVY_BAL, SIDE_PORT);
    set_weapon(ship,12, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship,13, W_MINDBLAST, SIDE_REAR);
    set_weapon(ship,14, W_SMALL_CAT, SIDE_REAR);
    set_weapon(ship,15, W_SMALL_CAT, SIDE_REAR);
    ship->crew.sail_skill = 10000;
    ship->crew.guns_skill = 10000;
    ship->crew.rpar_skill = 10000;
    ship->frags = number(3000, 4000);
    load_npc_ship_crew(ship, 20, 4);
}

void setup_npc_dreadnought_02(P_ship ship, NPC_AI_Type type) // level 4
{
    set_weapon(ship, 0, W_LONGTOM, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 2, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 3, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 4, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_HEAVY_BEAM, SIDE_STAR);
    set_weapon(ship, 6, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 7, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 8, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship, 9, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship,10, W_HEAVY_BEAM, SIDE_PORT);
    set_weapon(ship,11, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship,12, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship,13, W_SMALL_CAT, SIDE_REAR);
    set_weapon(ship,14, W_SMALL_CAT, SIDE_REAR);
    set_weapon(ship,15, W_SMALL_CAT, SIDE_REAR);
    ship->crew.sail_skill = 10000;
    ship->crew.guns_skill = 10000;
    ship->crew.rpar_skill = 10000;
    ship->frags = number(3000, 4000);
    load_npc_ship_crew(ship, 20, 4);
}
void setup_npc_dreadnought_03(P_ship ship, NPC_AI_Type type) // level 4
{
    set_weapon(ship, 0, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 1, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 2, W_LARGE_CAT, SIDE_FORE);
    set_weapon(ship, 3, W_MINDBLAST, SIDE_STAR);
    set_weapon(ship, 4, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 5, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 6, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 7, W_LARGE_BAL, SIDE_STAR);
    set_weapon(ship, 8, W_MINDBLAST, SIDE_PORT);
    set_weapon(ship, 9, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship,10, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship,11, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship,12, W_LARGE_BAL, SIDE_PORT);
    set_weapon(ship,13, W_MINDBLAST, SIDE_REAR);
    set_weapon(ship,14, W_SMALL_CAT, SIDE_REAR);
    set_weapon(ship,15, W_SMALL_CAT, SIDE_REAR);
    ship->crew.sail_skill = 10000;
    ship->crew.guns_skill = 10000;
    ship->crew.rpar_skill = 10000;
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
    { SH_DREADNOUGHT, 4, &setup_npc_dreadnought_03 },
};




//////////////////////
// LOADING
//////////////////////

P_char dbg_char = 0;
P_ship try_load_npc_ship(P_ship target)
{
    if (ISNPCSHIP(target))
        return 0;
    if (target->m_class == SH_SLOOP || target->m_class == SH_YACHT)
        return 0;

    NPC_AI_Type type = NPC_AI_PIRATE;
    int level = 0;
    int n = number(0, SHIPHULLWEIGHT(target)) + target->frags;
    if (ISMERCHANT(target))
    {
        if(IS_SET(target->flags, ATTACKBYNPC)) 
            return 0;
        if (n < 250)
        {
            level = 0;
        }
        else if (n < 800)
        {
            level = 1;
        }
        else if (n < 1600 || number(1, 4) != 1)
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

    P_ship ship = try_load_npc_ship(target, type, level, 0);

    if (ship)
    {
        statuslog(AVATAR, "%s's ship is attacked by a %s!", target->ownername, (type == NPC_AI_PIRATE) ? "pirate" : ((type == NPC_AI_HUNTER) ? "hunter" : "unknown"));
    }

    if(!IS_SET(target->flags, ATTACKBYNPC)) 
      SET_BIT(target->flags, ATTACKBYNPC); 

    return ship;
}

P_ship try_load_npc_ship(P_ship target, NPC_AI_Type type, int level, P_char ch)
{
    getmap(target);

    float dir = target->heading + number(-45, 45);
    normalize_direction(dir);

    int load_range = 0;
    if (type == NPC_AI_PIRATE || type == NPC_AI_HUNTER)
        load_range = 45;
    float rad = dir * M_PI / 180.000;
    float ship_x = 50 + sin(rad) * load_range;
    float ship_y = 50 + cos(rad) * load_range;

    int location = tactical_map[(int) ship_x][100 - (int) ship_y].rroom;

    //char ttt[100];
    //sprintf(ttt, "Location: x=%d, y=%d, loc=%d, sect=%d\r\n", (int)ship_x, (int)ship_y, location, world[location].sector_type);
    //if (ch) send_to_char(ttt, ch);

    P_ship ship = try_load_npc_ship(target, type, level, location, ch);
    if (!ship) 
        return 0;

    ship->npc_ai->advanced = 0;
    if (level == 1 && number(1, 4) == 1)
        ship->npc_ai->advanced = 1;
    if (level == 2 && number(1, 2) == 1)
        ship->npc_ai->advanced = 1;
    if (level >= 3)
        ship->npc_ai->advanced = 1;

    if (type == NPC_AI_PIRATE || type == NPC_AI_HUNTER)
    {
        ship->target = target;
        ship->npc_ai->mode = NPC_AI_ENGAGING;
    }
    if (type == NPC_AI_ESCORT)
    {
        ship->npc_ai->escort = target;
        ship->npc_ai->mode = NPC_AI_CRUISING;
    }
    float ship_heading = dir + 180;
    normalize_direction(ship_heading);
    ship->setheading = ship_heading;
    ship->heading = ship_heading;
    int ship_speed = ship->get_maxspeed();
    ship->setspeed = ship_speed;
    ship->speed = ship_speed;

    if (ch) send_to_char_f(ch, "Loaded level %d %s%s hull %d with %s AI at room %d.\r\n", level, type == NPC_AI_PIRATE ? "pirate" : "", type == NPC_AI_HUNTER ? "hunter" : "", ship->m_class, ship->npc_ai->advanced == 1 ? "advanced" : "basic", location);

    return ship;
}

P_ship try_load_npc_ship(P_ship target, NPC_AI_Type type, int level, int location, P_char ch)
{
    if (world[location].sector_type != SECT_OCEAN)
    {
        if (ch) send_to_char("Wrong location type\r\n", ch);
        return 0;
    }

    int min_speed = target ? (SHIPTYPESPEED(target->m_class) - 10) : -1;
    P_ship ship = load_npc_ship(level, type, min_speed, -1, location, ch);
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

    P_ship ship = new_ship(npcShipSetup[i].m_class, true);
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
    ship->npc_ai->type = type;
    ship->ownername = 0;
    ship->anchor = world[room].number;

    int name_index = number(0, sizeof(pirateShipNames)/sizeof(char*) - 1);
    name_ship(pirateShipNames[name_index], ship);
    assignid(ship, NULL, true);

    npcShipSetup[i].setup(ship, type);
    set_crew(ship, ship->npc_ai->crew_data->ship_crew_index, false);
    
    if (!load_ship(ship, room))
    {
        if (ch) send_to_char("Couldnt load npc ship!\r\n", ch);
        return false;
    }
    
    REMOVE_BIT(ship->flags, DOCKED);
    return ship;
}


bool try_unload_npc_ship(P_ship ship)
{
    everyone_get_out_newship(ship);
    shipObjHash.erase(ship);
    delete_ship(ship, true);
    return TRUE;
}


P_ship npc_dreadnought = 0;

bool load_npc_dreadnought()
{
    if (npc_dreadnought != 0)
        return false;

    for (int i = 0; i < 20; i++)
    {
        int room = number (0, top_of_world);
        if (!IS_MAP_ROOM(room) || world[room].sector_type != SECT_OCEAN)
            continue;
        if ((npc_dreadnought = try_load_npc_ship(0, NPC_AI_HUNTER, 4, room, 0)) != 0)
        {
            int name_index = number(0, sizeof(dreadnoughtShipNames)/sizeof(char*) - 1);
            name_ship(dreadnoughtShipNames[name_index], npc_dreadnought);
            npc_dreadnought->npc_ai->advanced = 1;
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
      40215,
      40220,
      2
  },
  {
      1,
      40231,
      40232,
      { 40233, 40234, 40235, 40236, 40238, 0, 0, 0, 0, 0 },
      { 40237, 40241, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 40239, 40240, 0, 0, 0, 0, 0, 0, 0, 0 },
      40216,
      40221,
      3
  },
  {
      2,
      40242,
      40243,
      { 40244, 40245, 40246, 40247, 40249, 0, 0, 0, 0, 0 },
      { 40248, 40252, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 40250, 40251, 0, 0, 0, 0, 0, 0, 0, 0 },
      40217,
      40222,
      4
  },
  {
      3,
      40253,
      40254,
      { 40255, 40256, 40257, 40258, 40260, 0, 0, 0, 0, 0 },
      { 40259, 40263, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 40261, 40262, 0, 0, 0, 0, 0, 0, 0, 0 },
      40218,
      40223,
      5
  },
  {
      4,
      40264,
      40265,
      { 40266, 40268, 40267, 40269, 40272, 0, 0, 0, 0, 0 },
      { 40273, 40274, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 40270, 40271, 0, 0, 0, 0, 0, 0, 0, 0 },
      40219,
      40224,
      6
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

static int materials[] = {7, 13, 20, 21, 22, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34 }; // quality materials
P_obj create_material(int index);

bool load_treasure_chest(P_ship ship, P_char captain, NPCShipCrewData* crew)
{
    int r_num;
    if ((r_num = real_object(crew->treasure_chest)) < 0)
        return false;
    P_obj chest = read_object(r_num, REAL);
    if (!chest)
        return false;
    obj_to_room(chest, real_room0(ship->bridge));

    if ((r_num = real_object(crew->treasure_chest_key)) < 0)
        return false;
    P_obj key = read_object(r_num, REAL);
    if (!key)
        return false;
    obj_to_char(key, captain);

    int money = 0;
    switch (crew->level)
    {
    case 0: money = number(200, 400); break;
    case 1: money = number(500, 600); break;
    case 2: money = number(800, 1000); break;
    case 3: money = number(1200, 1500); break;
    case 4: money = number(3000, 5000); break;
    };

    P_obj money_obj = create_money(0, 0, 0, money);
    obj_to_obj(money_obj, chest);


    int npieces = number(5, 10 + crew->level * 5);
    for (int i = 0; i < npieces; i++)
    {
        int material_index = materials[number(0, sizeof(materials)/sizeof(int) - 1)];
        P_obj piece = create_material(material_index);
        obj_to_obj(piece, chest);
    }

    int nstones = number(2, 4 + crew->level * 2);
    for (int i = 0; i < nstones; i++)
    {
        P_obj stone = create_stones(NULL);
        obj_to_obj(stone, chest);
    }

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
    if (crew_level < 3 && number(0,4) == 0)
        crew_level++;
    else if (crew_level > 0 && number(0,4) == 0)
        crew_level--;

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

    load_treasure_chest(ship, captain, crew_data);
    return true;
}



