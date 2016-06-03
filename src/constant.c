
/*
 * ***************************************************************************
 * *  File: constant.c                                         Part of Duris *
 * *  Usage: almost all of the tables and wordlists.
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include "config.h"
#include "objmisc.h"
#include "utils.h"
#include "spells.h"
#include "structs.h"
#include "ships.h"
#include "assocs.h"
#include "vnum.obj.h"

extern const char *god_list[];
const char *god_list[] = {
  "Clavados",
  "Railand",
  "Kvark",
  "Tharkun",
  "Fotenak",
  "\0"
};


/* Kingdom constants */

const char *troop_types[] = {
  "foot soldier",
  "archer",
  "cavalry",
  "scout",
  "assassin",
  "cleric",
  "mage",
  "\n"
};

const char *troop_levels[] = {
  "raw",
  "seasoned",
  "veteran",
  "elite",
  "\n"
};

const char *troop_offense[] = {
  "basic",
  "average",
  "well",
  "\n"
};

const char *troop_defense[] = {
  "basic",
  "average",
  "well",
  "\n"
};

const int troop_costs[NUM_TROOP_TYPES][4] = {
  {1, 5, 10},                   /* TROOP_FOOT */
  {2, 10, 20},                  /* TROOP_ARCHER */
  {4, 20, 40},                  /* TROOP_CAVALRY */
  {5, 25, 50},                  /* TROOP_SCOUT */
  {7, 35, 70},                  /* TROOP_ASSASSIN */
  {8, 40, 80},                  /* TROOP_CLERIC */
  {10, 50, 100}                 /* TROOP_MAGE */
};

extern const struct attr_names_struct attr_names[];
const attr_names_struct attr_names[] = {
  {0},
  {"str", "Strength"},
  {"dex", "Dexterity"},
  {"agi", "Agility"},
  {"con", "Constitution"},
  {"pow", "Power"},
  {"int", "Intelligence"},
  {"wis", "Wisdom"},
  {"pow", "Power"},
  {"kar", "Karma"},
  {"luc", "Luck"},
  {0}
};

// if ch is class, which other classes can they secondary to

extern const int allowed_secondary_classes[CLASS_COUNT + 1][5];
const int allowed_secondary_classes[CLASS_COUNT + 1][5] = {
  {-1},
  {CLASS_MERCENARY, CLASS_CLERIC, -1},            /* War */
  {CLASS_WARRIOR, CLASS_DRUID, CLASS_ROGUE, CLASS_PSIONICIST, -1}, /* Ran */
  {-1},                         		              /* Psi */
  {CLASS_WARRIOR, -1},          		              /* Pal */
  {CLASS_WARRIOR, -1},          		              /* APa */
  {CLASS_DRUID, CLASS_BARD, CLASS_WARRIOR, CLASS_SHAMAN, -1}, /* Cle */
  {-1},                        	 		              /* Mon */
  {CLASS_CLERIC, -1},                             /* Dru */
  {CLASS_CONJURER, CLASS_DRUID, CLASS_CLERIC, CLASS_BLIGHTER, -1},/* Sha */
  {CLASS_CONJURER, -1},         		              /* Sor */
  {-1},                         		              /* Nec */
  {CLASS_SORCERER, CLASS_SHAMAN, CLASS_BARD, -1}, /* Con */
  {CLASS_BARD, CLASS_MERCENARY, -1},              /* Rog */
  {-1},                             /* Assassin not currently in game      */
  {CLASS_ROGUE, CLASS_WARRIOR, -1},               /* Mer */
  {CLASS_SORCERER, CLASS_ROGUE, CLASS_ILLUSIONIST, CLASS_CONJURER, -1}, /* Bar */
  {-1},    	                        /* Thief not currently in game          */
  {-1},                             /* Warlock not currently in game        */
  {-1},                         		              /* MnF */
  {-1},                         		              /* Alc */
  {-1},                         		              /* Ber */
  {CLASS_SORCERER, CLASS_ROGUE, CLASS_PSIONICIST, -1}, /* Rea */
  {-1},                         		              /* Ilu */
  {CLASS_CLERIC, -1},                             /* Bli */
  {-1},                                  	        /* Dre */
  {CLASS_SORCERER, CLASS_SHAMAN, -1},             /* Eth */
  {-1},                                           /* Ave */
  {-1},                                           /* The */
  {CLASS_SORCERER, CLASS_SHAMAN, -1}              /* Sum */
};

// class names for particular multiclass combos
extern const struct mcname multiclass_names[];
const mcname multiclass_names[] = {
  /* Start Defining Multiclass names here */
  /* starting with CLASS_WARRIOR */
  {CLASS_WARRIOR, CLASS_RANGER,          "&+gWoodsman&n"},
  {CLASS_WARRIOR, CLASS_PSIONICIST,      "&+YBody &+MTamer&n"},  
  {CLASS_WARRIOR, CLASS_PALADIN,         "&+WDefender&n"},
  {CLASS_WARRIOR, CLASS_ANTIPALADIN,     "&+LBlackguard&n"},  
  {CLASS_WARRIOR, CLASS_CLERIC,          "&+cWar Priest&n"},
  {CLASS_WARRIOR, CLASS_MONK,            "&+BBlue Ninja&n"},  
  {CLASS_WARRIOR, CLASS_DRUID,           "&+BWo&+Gad&n"},
  {CLASS_WARRIOR, CLASS_SHAMAN,          "&+BCh&+Cie&+Bft&+Cai&+Bn&n"},  
  {CLASS_WARRIOR, CLASS_SORCERER,        "&+MSpell&+Bsword&n"},
  {CLASS_WARRIOR, CLASS_NECROMANCER,     "&+mDeath&+Bwarden&n"},  
  {CLASS_WARRIOR, CLASS_CONJURER,        "&+LEng&+rineer&n"},
  {CLASS_WARRIOR, CLASS_ROGUE,           "&+rCon &+yArtist&n"},  
  {CLASS_WARRIOR, CLASS_MERCENARY,       "&+yGladiator&n"},
  {CLASS_WARRIOR, CLASS_MINDFLAYER,      "&+MBrain &+BDud&n"},  
  {CLASS_WARRIOR, CLASS_BERSERKER,       "&+BV&+ri&+Rk&+ri&+Rn&+Bg&n"},
  {CLASS_WARRIOR, CLASS_REAVER,          "&+LC&+wav&+Le &+BLord&n"},  
  {CLASS_WARRIOR, CLASS_ILLUSIONIST,     "&+wG&+Wh&+wo&+Ws&+wt &+BTank&n"},
  {CLASS_WARRIOR, CLASS_DREADLORD,       "&+BSc&+Lo&+wur&+Lge&n"},  
  {CLASS_WARRIOR, CLASS_ETHERMANCER,     "&+BAb&+wominati&+Bon&n"},
  {CLASS_WARRIOR, CLASS_AVENGER,         "&+BJu&+Yst&+Wic&+Ya&+Br&n"},  
  {CLASS_WARRIOR, CLASS_THEURGIST,       "&+BRi&+Ypp&+Ber&n"},
  {CLASS_RANGER, CLASS_PSIONICIST,       "&+LVis&+Wion&+Wary&n"},  
  {CLASS_RANGER, CLASS_PALADIN,          "&+GWar&+Wden&n"},  
  {CLASS_RANGER, CLASS_ANTIPALADIN,      "&+LBleak &+GHero&n"},
  {CLASS_RANGER, CLASS_CLERIC,           "&+GWild &+YZealot&n"},  
  {CLASS_RANGER, CLASS_MONK,             "&+GGreen Ninja&n"},  
  {CLASS_RANGER, CLASS_DRUID,            "&+gFor&+Gres&+gtal&n"},
  {CLASS_RANGER, CLASS_SHAMAN,           "&+GG&+Ca&+Gr&+Co&+Gu&n"},  
  {CLASS_RANGER, CLASS_SORCERER,         "&+GOutland &+MMagi&n"},  
  {CLASS_RANGER, CLASS_NECROMANCER,      "&+GPu&+mtr&+Gif&+mie&+Gr&n"},
  {CLASS_RANGER, CLASS_CONJURER,         "&+YElement &+GArc&n"},  
  {CLASS_RANGER, CLASS_ROGUE,            "&+LScout&n"},  
  {CLASS_RANGER, CLASS_MERCENARY,        "&+GB&+yr&+Gu&+yt&+Ga&+yl&+Gi&+yz&+Ge&+yr&n"},
  {CLASS_RANGER, CLASS_BARD,             "&+bT&+Len&+bd&+Ger&+bi&+Lze&+br&n"},  
  {CLASS_RANGER, CLASS_MINDFLAYER,       "&+MKinetic &+GGod&n"},  
  {CLASS_RANGER, CLASS_BERSERKER,        "&+GA&+Rb&+ru&+Rs&+Re&+Gr&n"},
  {CLASS_RANGER, CLASS_REAVER,           "&+GR&+Le&+wdu&+Ln&+Gdant&n"},  
  {CLASS_RANGER, CLASS_ILLUSIONIST,      "&+GR&+Wa&+Cv&+Wn&+Co&+Gs&n"},  
  {CLASS_RANGER, CLASS_DREADLORD,        "&+GH&+rereti&+Gc&n"},
  {CLASS_RANGER, CLASS_ETHERMANCER,      "&+wWind &+GHunter&n"},  
  {CLASS_RANGER, CLASS_AVENGER,          "&+WHoly &+GHunter&n"},  
  {CLASS_RANGER, CLASS_THEURGIST,        "&+WD&+Yivin&+We &+GScout&n"},
  {CLASS_PSIONICIST, CLASS_PALADIN,      "&+bPro&+Wphet&n"},  
  {CLASS_PSIONICIST, CLASS_ANTIPALADIN,  "&+LFake &+bPro&+Wphet&n"},  
  {CLASS_PSIONICIST, CLASS_CLERIC,       "&+bArdent &+cSeer&n"},
  {CLASS_PSIONICIST, CLASS_MONK,         "&+mMind &+LNinja&n"},  
  {CLASS_PSIONICIST, CLASS_DRUID,        "&+bOra&+Gcle&n"},  
  {CLASS_PSIONICIST, CLASS_SHAMAN,       "&+CTotemic &+bRage&n"},
  {CLASS_PSIONICIST, CLASS_SORCERER,     "Arcane Mentalist&n"},  
  {CLASS_PSIONICIST, CLASS_NECROMANCER,  "&+mDread &+bWave&n"},
  {CLASS_PSIONICIST, CLASS_CONJURER,     "&+mMe&+Mnt&+Wal Man&+Mipu&+mlator&n"},  
  {CLASS_PSIONICIST, CLASS_ROGUE,        "&+LBack&+bbender&n"},  
  {CLASS_PSIONICIST, CLASS_MERCENARY,    "&+yArm&+bbender&n"},
  {CLASS_PSIONICIST, CLASS_BARD,         "&+MSong&+bbender&n"},  
  {CLASS_PSIONICIST, CLASS_MINDFLAYER,   "&+MThe &+bBoss&n"},  
  {CLASS_PSIONICIST, CLASS_BERSERKER,    "&+rF&+Rac&+re&+bbender&n"},
  {CLASS_PSIONICIST, CLASS_REAVER,       "&+LTra&+Wnsc&+Wendent&n"}, 
  {CLASS_PSIONICIST, CLASS_RANGER,       "&+LVis&+Wion&+Wary&n"}, 
  {CLASS_PSIONICIST, CLASS_ILLUSIONIST,  "&+bMindbender&n"},
  {CLASS_PSIONICIST, CLASS_DREADLORD,    "&+rDe&+Lath&+bbender&n"},  
  {CLASS_PSIONICIST, CLASS_ETHERMANCER,  "&+wWind&+bbender&n"},
  {CLASS_PSIONICIST, CLASS_AVENGER,      "&+WLife&+bbender&n"},  
  {CLASS_PSIONICIST, CLASS_THEURGIST,    "&+CH&+Weave&+Cn&+bbender&n"},
  {CLASS_PALADIN, CLASS_ANTIPALADIN,     "&+wGray Knight&n"},  
  {CLASS_PALADIN, CLASS_CLERIC,          "Divine Champion&n"},  
  {CLASS_PALADIN, CLASS_MONK,            "&+WWhite Ninja&n"},
  {CLASS_PALADIN, CLASS_DRUID,           "&+gLeafy &+WWrath&n"},
  {CLASS_PALADIN, CLASS_SHAMAN,          "&+CTotem &+WWalker&n"},
  {CLASS_PALADIN, CLASS_SORCERER,        "&+WPur&+Mifier&n"},
  {CLASS_PALADIN, CLASS_NECROMANCER,     "&+LBlack &+WKnight&n"},
  {CLASS_PALADIN, CLASS_CONJURER,        "&+yEarth &+WKnight&n"},
  {CLASS_PALADIN, CLASS_ROGUE,           "&+rRed &+WKnight&n"},
  {CLASS_PALADIN, CLASS_MERCENARY,       "&+yDirty &+WKnight&n"},
  {CLASS_PALADIN, CLASS_BARD,            "&+MPansy &+WKnight&n"},
  {CLASS_PALADIN, CLASS_MINDFLAYER,      "&+MPink Knight&n"},
  {CLASS_PALADIN, CLASS_BERSERKER,       "&+WHoly Wraith&n"},
  {CLASS_PALADIN, CLASS_REAVER,          "&+LB&+Wlen&+Ld Knight&n"},
  {CLASS_PALADIN, CLASS_ILLUSIONIST,     "Invis Knight&n"},
  {CLASS_PALADIN, CLASS_DREADLORD,       "&+LEbony Knight&n"},
  {CLASS_PALADIN, CLASS_ETHERMANCER,     "&+wSky &+WKnight&n"},
  {CLASS_PALADIN, CLASS_AVENGER,         "&+LDread &+WKnight&n"},
  {CLASS_PALADIN, CLASS_THEURGIST,       "&+WCrucible    &n"},
  {CLASS_ANTIPALADIN, CLASS_CLERIC,      "Unholy Champion&n"},
  {CLASS_ANTIPALADIN, CLASS_MONK,        "&+rRed Ninja   &n"},
  {CLASS_ANTIPALADIN, CLASS_DRUID,       "&+LDe&+gvour&+Ler&n"},
  {CLASS_ANTIPALADIN, CLASS_SHAMAN,      "&+LMis&+Cleader&n"},
  {CLASS_ANTIPALADIN, CLASS_SORCERER,    "&+LW&+gi&+Gt&+Whe&+Gr&+ge&+Lr&n"},
  {CLASS_ANTIPALADIN, CLASS_NECROMANCER, "&+mDread &+LReaper&n"},
  {CLASS_ANTIPALADIN, CLASS_CONJURER,    "&+YPlanet&+Lbasher&n"},
  {CLASS_ANTIPALADIN, CLASS_ROGUE,       "&+LDarkness&n"},
  {CLASS_ANTIPALADIN, CLASS_MERCENARY,   "&+yDi&+Lrr&+yty&n"},
  {CLASS_ANTIPALADIN, CLASS_BARD,        "&+LEvil &+bP&+goe&+bt&n"},
  {CLASS_ANTIPALADIN, CLASS_MINDFLAYER,  "&+LDark &+MHaze&n"},
  {CLASS_ANTIPALADIN, CLASS_BERSERKER,   "&+LM&+ro&+Rn&+rg&+Rr&+re&+Ll&n"},
  {CLASS_ANTIPALADIN, CLASS_REAVER,      "&+LDreadsp&+Wee&+Ld&n"},
  {CLASS_ANTIPALADIN, CLASS_ILLUSIONIST, "&+LSudden Death&n"},
  {CLASS_ANTIPALADIN, CLASS_DREADLORD,   "&+LIns&+rtiga&+Ltor&n"},
  {CLASS_ANTIPALADIN, CLASS_ETHERMANCER, "&+wWind&+Lbreaker&n"},
  {CLASS_ANTIPALADIN, CLASS_AVENGER,     "&+LRap&+Yture&n"},
  {CLASS_ANTIPALADIN, CLASS_THEURGIST,   "&+LRup&+Wture&n"},
  {CLASS_CLERIC, CLASS_MONK,             "&+wIvory Monk&n"},
  {CLASS_CLERIC, CLASS_DRUID,            "&+YCenobite&n"},
  {CLASS_CLERIC, CLASS_SHAMAN,           "&+WSpiritmaster&n"},
  {CLASS_CLERIC, CLASS_SORCERER,         "&+cF&+Mria&+cr&n"},
  {CLASS_CLERIC, CLASS_NECROMANCER,      "&+mExor&+ccist&n"},
  {CLASS_CLERIC, CLASS_CONJURER,         "&+YPlanes&+cwalker&n"},
  {CLASS_CLERIC, CLASS_ROGUE,            "&+rW&+Rhi&+rp&n"},
  {CLASS_CLERIC, CLASS_MERCENARY,        "&+yDirty &+cPriest&n"},
  {CLASS_CLERIC, CLASS_BARD,             "&+rSkald&n"},
  {CLASS_CLERIC, CLASS_MINDFLAYER,       "&+LS&+Wku&+Lnk&n"},
  {CLASS_CLERIC, CLASS_BERSERKER,        "&+rB&+Rr&+ru&+Rt&+ra&+Rl &+cZeal&n"},
  {CLASS_CLERIC, CLASS_REAVER,           "&+LB&+Wlen&+Ld &+cPriest&n"},
  {CLASS_CLERIC, CLASS_ILLUSIONIST,      "&+WS&+wl&+Wy &+cPriest&n"},
  {CLASS_CLERIC, CLASS_DREADLORD,        "&+cPe&+Ld&+roph&+Li&+cle&n"},
  {CLASS_CLERIC, CLASS_ETHERMANCER,      "&+wWind &+cPriest&n"},
  {CLASS_CLERIC, CLASS_AVENGER,          "&+WWar &+YZealot&n"},
  {CLASS_CLERIC, CLASS_THEURGIST,        "&+WHoly &+cPriest&n"},
  {CLASS_CLERIC, CLASS_BLIGHTER,         "&+LDecay &+yPriest&n"},
  {CLASS_MONK, CLASS_DRUID,              "&+gLeafy Ninja&n"},
  {CLASS_MONK, CLASS_SHAMAN,             "&+CSky &+wNinja&n"},
  {CLASS_MONK, CLASS_SORCERER,           "Pink Ninja&n"},
  {CLASS_MONK, CLASS_NECROMANCER,        "Evil Ninja&n"},
  {CLASS_MONK, CLASS_CONJURER,           "Yellow Ninja&n"},
  {CLASS_MONK, CLASS_ROGUE,              "Black Ninja&n"},
  {CLASS_MONK, CLASS_MERCENARY,          "Dirty Ninja&n"},
  {CLASS_MONK, CLASS_BARD,               "Ohm Ninja&n"},
  {CLASS_MONK, CLASS_MINDFLAYER,         "Brain Ninja&n"},
  {CLASS_MONK, CLASS_BERSERKER,          "Brutal Ninja&n"},
  {CLASS_MONK, CLASS_REAVER,             "Quick Ninja&n"},
  {CLASS_MONK, CLASS_ILLUSIONIST,        "Dream Ninja&n"},
  {CLASS_MONK, CLASS_DREADLORD,          "&+LDr&+read Nin&+Lja&n"},
  {CLASS_MONK, CLASS_ETHERMANCER,        "&+wCloud Ninja&n"},
  {CLASS_MONK, CLASS_AVENGER,            "&+WHoly Ninja&n"},
  {CLASS_MONK, CLASS_THEURGIST,          "&+WPure Ninja&n"},
  {CLASS_DRUID, CLASS_SHAMAN,            "&+rBeastmaster&n"},
  {CLASS_DRUID, CLASS_SORCERER,          "Forest Magi"},
  {CLASS_DRUID, CLASS_NECROMANCER,       "Blighter"},
  {CLASS_DRUID, CLASS_CONJURER,          "Sojourner"},
  {CLASS_DRUID, CLASS_MERCENARY,         "Forest Boxer"},
  {CLASS_DRUID, CLASS_BARD,              "Woodsinger"},
  {CLASS_DRUID, CLASS_MINDFLAYER,        "Savant"},
  {CLASS_DRUID, CLASS_BERSERKER,         "Vendo"},
  {CLASS_DRUID, CLASS_REAVER,            "Rough Druid"},
  {CLASS_DRUID, CLASS_ILLUSIONIST,       "Sly Druid"},
  {CLASS_DRUID, CLASS_DREADLORD,         "Black Druid"},
  {CLASS_DRUID, CLASS_ETHERMANCER,       "Wind Druid"},
  {CLASS_DRUID, CLASS_AVENGER,           "Tree Hugger"},
  {CLASS_DRUID, CLASS_THEURGIST,         "Lenser"},
  {CLASS_SHAMAN, CLASS_SORCERER,         "&+CSpirit&+Mwalker&n"},
  {CLASS_SHAMAN, CLASS_NECROMANCER,      "&+CTotem &+mHorde&n"},
  {CLASS_SHAMAN, CLASS_CONJURER,         "&+bChanneler&n"},
  {CLASS_SHAMAN, CLASS_ROGUE,            "Backtalker"},
  {CLASS_SHAMAN, CLASS_MERCENARY,        "Totem Boxer"},
  {CLASS_SHAMAN, CLASS_BARD,             "Totem Singer"},
  {CLASS_SHAMAN, CLASS_MINDFLAYER,       "Totemic Wave"},
  {CLASS_SHAMAN, CLASS_BERSERKER,        "Brutal Totem&n"},
  {CLASS_SHAMAN, CLASS_REAVER,           "&+LS&+wo&+Wulrend&+we&+Lr&n"},
  {CLASS_SHAMAN, CLASS_ILLUSIONIST,      "Totem Hider&n"},
  {CLASS_SHAMAN, CLASS_DREADLORD,        "Dread Totem&n"},
  {CLASS_SHAMAN, CLASS_ETHERMANCER,      "&+yGeo&nmancer&n"},
  {CLASS_SHAMAN, CLASS_AVENGER,          "Vengeance&n"},
  {CLASS_SHAMAN, CLASS_THEURGIST,        "Holy Totem"},
  {CLASS_SHAMAN, CLASS_BLIGHTER,         "&+LDes&+we&+Cc&+wr&+Lator&n"},
  {CLASS_SORCERER, CLASS_NECROMANCER,    "&+LDe&+rat&+Rh Ma&+rgu&+Ls&n"},
  {CLASS_SORCERER, CLASS_CONJURER,       "&+RArchmagi&n"},
  {CLASS_SORCERER, CLASS_ROGUE,          "Trapbreaker"},
  {CLASS_SORCERER, CLASS_MERCENARY,      "Spelltackler"},
  {CLASS_SORCERER, CLASS_BARD,           "&+bEnchanter&n"},
  {CLASS_SORCERER, CLASS_MINDFLAYER,     "Arcane Wave"},
  {CLASS_SORCERER, CLASS_BERSERKER,      "Magic Maul"},
  {CLASS_SORCERER, CLASS_REAVER,         "&+LDe&+mfil&+Ler&n"},
  {CLASS_SORCERER, CLASS_ILLUSIONIST,    "Sleuth"},
  {CLASS_SORCERER, CLASS_DREADLORD,      "Dreadcaster"},  
  {CLASS_SORCERER, CLASS_ETHERMANCER,    "&+cSt&+Corm Ma&+cgi&n"},
  {CLASS_SORCERER, CLASS_AVENGER,        "Arcane Zeal"},
  {CLASS_SORCERER, CLASS_THEURGIST,      "Diviner"},
  {CLASS_NECROMANCER, CLASS_CONJURER,    "Plane Ruiner"},
  {CLASS_NECROMANCER, CLASS_ROGUE,       "Pale Dirk"},
  {CLASS_NECROMANCER, CLASS_MERCENARY,   "Pale Brawler"},
  {CLASS_NECROMANCER, CLASS_BARD,        "Pale Singer"},
  {CLASS_NECROMANCER, CLASS_MINDFLAYER,  "Pale Wave"},
  {CLASS_NECROMANCER, CLASS_BERSERKER,   "Pale Maul"},
  {CLASS_NECROMANCER, CLASS_REAVER,      "Pale Blend"},
  {CLASS_NECROMANCER, CLASS_ILLUSIONIST, "Pale Secret"},
  {CLASS_NECROMANCER, CLASS_DREADLORD,   "Pale Master"},
  {CLASS_NECROMANCER, CLASS_ETHERMANCER, "Pale Sky"},
  {CLASS_NECROMANCER, CLASS_AVENGER,     "Pale Zeal"},
  {CLASS_NECROMANCER, CLASS_THEURGIST,   "Petspammer"},
  {CLASS_CONJURER, CLASS_ROGUE,          "Plane Dirk"},
  {CLASS_CONJURER, CLASS_MERCENARY,      "Dirty Planes"},
  {CLASS_CONJURER, CLASS_BARD,           "&+mG&+My&+bp&+Bs&+Wy&n"},
  {CLASS_CONJURER, CLASS_MINDFLAYER,     "Plane Wave"},
  {CLASS_CONJURER, CLASS_BERSERKER,      "Plane Maul"},
  {CLASS_CONJURER, CLASS_REAVER,         "Plane Blend"},
  {CLASS_CONJURER, CLASS_ILLUSIONIST,    "Plane Secret"},
  {CLASS_CONJURER, CLASS_DREADLORD,      "Plane Dread"},
  {CLASS_CONJURER, CLASS_ETHERMANCER,    "Plane Sky"},
  {CLASS_CONJURER, CLASS_AVENGER,        "Plane Zeal"},
  {CLASS_CONJURER, CLASS_THEURGIST,      "Holy Plane"},
  {CLASS_ROGUE, CLASS_MERCENARY,         "&+yThug&n"},
  {CLASS_ROGUE, CLASS_BARD,              "&+bGr&+Lift&+ber"},
  {CLASS_ROGUE, CLASS_MINDFLAYER,        "Sly Wave"},
  {CLASS_ROGUE, CLASS_BERSERKER,         "Sly Maul"},
  {CLASS_ROGUE, CLASS_REAVER,            "&+LSh&+wado&+Lw R&+Wea&+Lv&+We&+Lr&n"},
  {CLASS_ROGUE, CLASS_ILLUSIONIST,       "Sly Secret"},
  {CLASS_ROGUE, CLASS_DREADLORD,         "Sly Dread"},
  {CLASS_ROGUE, CLASS_ETHERMANCER,       "Sly Sky"},
  {CLASS_ROGUE, CLASS_AVENGER,           "Sly Zeal"},
  {CLASS_ROGUE, CLASS_THEURGIST,         "Holy Sneak"},
  {CLASS_MERCENARY, CLASS_BARD,          "Dirty Singer"},
  {CLASS_MERCENARY, CLASS_MINDFLAYER,    "Dirty Wave"},
  {CLASS_MERCENARY, CLASS_BERSERKER,     "Dirty Maul"},
  {CLASS_MERCENARY, CLASS_REAVER,        "Dirty Blend"},
  {CLASS_MERCENARY, CLASS_ILLUSIONIST,   "Dirty Secret"},
  {CLASS_MERCENARY, CLASS_DREADLORD,     "Dirty Dread"},
  {CLASS_MERCENARY, CLASS_ETHERMANCER,   "Dirty Sky"},
  {CLASS_MERCENARY, CLASS_AVENGER,       "Dirty Zeal"},
  {CLASS_MERCENARY, CLASS_THEURGIST,     "Dirty Lenser"},
  {CLASS_BARD, CLASS_MINDFLAYER,         "Brainsong"},
  {CLASS_BARD, CLASS_BERSERKER,          "Maulsong"},
  {CLASS_BARD, CLASS_REAVER,             "Blendsong"},
  {CLASS_BARD, CLASS_ILLUSIONIST,        "&+LT&+br&+ci&+Cc&+Wk&+Cs&+ct&+be&+Lr&n"},
  {CLASS_BARD, CLASS_DREADLORD,          "Dreadsong"},
  {CLASS_BARD, CLASS_ETHERMANCER,        "Skysong"},
  {CLASS_BARD, CLASS_AVENGER,            "Zealsong"},
  {CLASS_BARD, CLASS_THEURGIST,          "Loud Lenser"},
  {CLASS_MINDFLAYER, CLASS_BERSERKER,    "Brutal Haze"},
  {CLASS_MINDFLAYER, CLASS_REAVER,       "Blended Haze"},
  {CLASS_MINDFLAYER, CLASS_ILLUSIONIST,  "Sly Haze"},
  {CLASS_MINDFLAYER, CLASS_DREADLORD,    "Dread Haze"},
  {CLASS_MINDFLAYER, CLASS_ETHERMANCER,  "Sky Haze"},
  {CLASS_MINDFLAYER, CLASS_AVENGER,      "Zealous Haze"},
  {CLASS_MINDFLAYER, CLASS_THEURGIST,    "Lensed Haze "},
  {CLASS_BERSERKER, CLASS_REAVER,        "Blended Maul"},
  {CLASS_BERSERKER, CLASS_ILLUSIONIST,   "Sly Maul"},
  {CLASS_BERSERKER, CLASS_DREADLORD,     "Dread Maul"},
  {CLASS_BERSERKER, CLASS_ETHERMANCER,   "Sky Maul"},
  {CLASS_BERSERKER, CLASS_AVENGER,       "Zealous Maul"},
  {CLASS_BERSERKER, CLASS_THEURGIST,     "Lensing Maul"},
  {CLASS_REAVER, CLASS_ILLUSIONIST,      "Sly Blender"},
  {CLASS_REAVER, CLASS_DREADLORD,        "Dread Blend"},
  {CLASS_REAVER, CLASS_PSIONICIST,       "&+LTra&+Wnsc&+Wendent&n"},
  {CLASS_REAVER, CLASS_ETHERMANCER,      "Sky Blend"},
  {CLASS_REAVER, CLASS_AVENGER,          "Blended Zeal"},
  {CLASS_REAVER, CLASS_THEURGIST,        "Lensed Blend"},
  {CLASS_ILLUSIONIST, CLASS_DREADLORD,   "Sly Dread"},
  {CLASS_ILLUSIONIST, CLASS_ETHERMANCER, "Sly Cloud"},
  {CLASS_ILLUSIONIST, CLASS_AVENGER,     "Sly Zeal"},
  {CLASS_ILLUSIONIST, CLASS_THEURGIST,   "Sly Lense"},
  {CLASS_DREADLORD, CLASS_ETHERMANCER,   "Sky Dread"},
  {CLASS_DREADLORD, CLASS_AVENGER,       "Dread Zeal"},
  {CLASS_DREADLORD, CLASS_THEURGIST,     "Lensed Dread"},
  {CLASS_ETHERMANCER, CLASS_AVENGER,     "Lensed Zeal"},
  {CLASS_ETHERMANCER, CLASS_THEURGIST,   "Lensed Cloud"},
  {CLASS_AVENGER, CLASS_THEURGIST,       "Apostle"},
  {CLASS_SUMMONER, CLASS_SORCERER,       "&+rArchmage&n"},
  {CLASS_SUMMONER, CLASS_SHAMAN,         "&+CHaruspex&n"},
  {-1, -1, ""}
};

extern const char valid_term_list[];
const char valid_term_list[] = "01) Generic\r\n02) Ansi\r\n";

const struct shapechange_struct shapechange_name_list[] = {
  {"bird", 40, ANIMAL_TYPE_BIRD},
  {"fish", 41, ANIMAL_TYPE_REPTILE},
  {"bear", 42, ANIMAL_TYPE_MAMMAL},
  {"lizard", 43, ANIMAL_TYPE_REPTILE},
  {"fish", 44, ANIMAL_TYPE_FISH},
  {"\0", 0, -1}
};

const char *poison_types[] = {
  "\n"
};

const char *disease_types[] = {
  "\n"
};

const char *shot_types[] = {
  "a missile",
  "a bullet",
  "an arrow",
  "a bolt",
  "a ballista bolt",
  "a catapult boulder",
  "\n"
};

const int shot_damage[] = {
  25,
  35,
  25,
  35,
  50,
  75,
  0
};

/* multiplier, memtime will be multiplied by appropriate constant */
extern const float druid_memtime_terrain_mod[NUM_SECT_TYPES];
const float druid_memtime_terrain_mod[NUM_SECT_TYPES] = {
  1.5,                          /* inside */
  1.5,                          /* city */
  1.2,                          /* field */
  1.0,                          /* forest */
  1.3,                          /* hills */
  1.5,                          /* mountain */
  1.2,                          /* water swin */
  1.2,                          /* water noswim */
  1.5,                          /* noground */
  1.3,                          /* underwater */
  1.3,                          /* underwater */
  1.2,                          /* fire plane */
  1.2,                          /* ocean */
  1.3,                          /* ud wild */
  2.0,                          /* ud city */
  2.0,                          /* ud inside */
  1.9,                          /* ud water */
  1.9,                          /* ud noswin */
  2.0,                          /* ud no ground */
  1.2,                          /* air plane */
  1.2,                          /* water plane */
  1.2,                          /* earth plane */
  1.4,                          /* ethereal */
  1.4,                          /* astral */
  1.5,                          /* desert */
  1.5,                          /* arctic */
  1.2,                          /* swamp */
  1.4,                          /* ud mountain */
  1.5,                          /* ud slime */
  1.5,                          /* ud low ceiling */
  2.0,                          /* ud liqmith */
  1.4,                          /* ud mushroom */
  1.5,                          /* castle wall */
  1.5,                          /* castle gate */
  1.5,                          /* castle */
  1.8,                          /* neg plane */
  1.2,                          /* plane of avernus */
  1.5,                          /* sect_road */
  1.1,                          /* snowy forest */
};

extern const float blighter_memtime_terrain_mod[NUM_SECT_TYPES];
const float blighter_memtime_terrain_mod[NUM_SECT_TYPES] = {
  1.5,                          /* inside */
  1.5,                          /* city */
  1.2,                          /* field */
  1.0,                          /* forest */
  1.25,                         /* hills */
  1.4,                          /* mountain */
  1.25,                         /* water swin */
  1.2,                          /* water noswim */
  1.5,                          /* noground */
  1.25,                         /* underwater */
  1.25,                         /* underwater */
  1.4,                          /* fire plane */
  1.25,                         /* ocean */
  1.1,                          /* ud wild */
  1.3,                          /* ud city */
  1.5,                          /* ud inside */
  1.3,                          /* ud water */
  1.3,                          /* ud noswin */
  1.5,                          /* ud no ground */
  1.4,                          /* air plane */
  1.25,                         /* water plane */
  1.4,                          /* earth plane */
  1.6,                          /* ethereal */
  1.6,                          /* astral */
  1.6,                          /* desert */
  1.4,                          /* arctic */
  1.1,                          /* swamp */
  1.6,                          /* ud mountain */
  1.3,                          /* ud slime */
  1.5,                          /* ud low ceiling */
  2.0,                          /* ud liqmith */
  1.0,                          /* ud mushroom */
  1.5,                          /* castle wall */
  1.5,                          /* castle gate */
  1.5,                          /* castle */
  1.8,                          /* neg plane */
  1.2,                          /* plane of avernus */
  1.5,                          /* sect_road */
  1.1,                          /* snowy forest */
};

const char *spell_types[] = {
  "NONE",
  "FIRE",
  "COLD",
  "ELECTRIC",
  "ACID",
  "BLOW",
  "GENERIC",
  "HEALING",
  "PROTECTION",
  "ENCHANTMENT",
  "DIVINATION",
  "TELEPORTATION",
  "SUMMONING",
  "ANIMAL",
  "ELEMENTAL",
  "SPIRIT",
  "PSIONIC"
};

extern const struct minor_create_struct minor_create_name_list[];
const struct minor_create_struct minor_create_name_list[] = {
  {"skin waterskin", 347},
  {"iron rations food", 364},
  {"small barrel", 385},
  {"lantern light", 399},
  {"torch", 398},
  {"bottle", 357},
  {"bag sack", 390},
  {"spellbook book", 203},
  {"quill pen", 204},
  {"backpack bp", 377},
  {"rugged cap", 450},
  {"helm helmet", 209},
  {"soft cloak", 445},
  {"green cloak", 457},
  {"black silk cloak", 718},
  {"lizard hide", 727},
  {"studded leather armor", 245},
  {"sleeves", 214},
  {"leggings", 227},
  {"soft boots", 440},
  {"sturdy boots", 246},
  {"hard leather boots", 439},
  {"oaken quarterstaff", 276},
  {"wooden spear", 278},
  {"hammer warhammer", 676},
  {"bastard sword", 280},
  {"steel dagger", 256},
  {"wooden mace", 677},
  {"battle axe", 254},
  {"spiked club", 699},
  {"large bone", 705},
  {"two-handed sword", 286},
  {"amber rod", 735},
  {"ragged elf scalps", 707},
  {"alligator skin jerkin", 706},
  {"filthy fur cowl", 702},
  {"animal furs", 701},
  {"blue scarf", 731},
  {"flowing magenta robes robe", 730},
  {"silk sash", 732},
  {"purple slippers shoes", 734},
  {"bronze bracers", 729},
  {"quartz", 111},
  {"obsidian", 109},
  {"bear", 108},
  {"lyre", 1241},
  {"flute", 1246},
  {"drum", 1243},
  {"shield", 1109},
  {"harp", 1245},
  {"horn", 1242},
  {"mandolin", 1244},
  {"potion", VOBJ_POTION_BOTTLES},
  {"paper", 5},
  {"shovel", 191},
  {"raft", 431},
  {"forging hammer",252},
  {"fishing pole", 336},
  {"whetstone", 367},
  {"bandage", 393},
  {"lockpicks", 412},

  {"\0", 0}
};

#if 0
const int material_absorbtion[TOTALATTACK_TYPES][TOTALMATERIALS] = {
/* pierce, slash                                */
  {},                           /* MAT_UNDEFINED  0        */
  {},                           /* MAT_N_SUBSTANT 1        */
  {},                           /* MAT_MAG_METAL  2        */
  {},                           /* MAT_WOOD       3        */
  {},                           /* MAT_CLOTH      4        */
  {},                           /* MAT_HIDE       5        */
  {},                           /* MAT_SILICON    6        */
  {},                           /* MAT_CRYSTAL    7        */
  {},                           /* MAT_MAGICAL    8        */
  {},                           /* MAT_BONE       9        */
  {},                           /* MAT_STONE     10        */
  {},                           /* MAT_LEATHER   11        */
  {},                           /* MAT_S_LEATHER 12        */
  {},                           /* MAT_SCALE     13        */
  {},                           /* MAT_CHAIN     14        */
  {},                           /* MAT_PLATE     15        */
  {},                           /* MAT_MITHRIL   16        */
  {},                           /* MAT_ADAMANTIUM 17       */
  {},                           /* MAT_BRONZE    18        */
  {},                           /* MAT_COPPER    19        */
  {},                           /* MAT_SILVER    20        */
  {},                           /* MAT_ELECTRUM  21        */
  {},                           /* MAT_GOLD      22        */
  {},                           /* MAT_PLATINUM  23        */
  {}                            /* MAT_GEM       24        */


};
#endif


extern const int movement_loss[];
const int movement_loss[] = {
  1,                            /* * Inside     */
  1,                            /* * City       */
  3,                            /* * Field      */
  3,                            /* * Forest     */
  4,                            /* * Hills      */
  5,                            /* * Mountains  */
  6,                            /* * Swimming   */
  6,                            /* * Unswimable */
  6,                            /* * No ground  */
  7,                            /* * underwater */
  7,                            /* * underwater walking */
  3,                            /* * plane of fire */
  75,                           /* * ocean      */
  3,                            /* * UD Wild    */
  2,                            /* * UD City    */
  2,                            /* * UD Inside  */
  5,                            /* * UD Swimming */
  5,                            /* * UD Unswimable */
  5,                            /* * UD No ground */
  2,                            /* * plane of air */
  5,                            /* * plane of water */
  4,                            /* * plane of earth */
  2,                            /* * plane of etheral */
  2,                            /* * plane of astral */
  7,                            /* desert */
  7,                            /* tundra/ice */
  7,                            /* swamp */
  7,                            /* UD mountains */
  5,                            /* UD slime pool */
  3,                            /* UD low cavern */
  5,                            /* UD liquid mithril */
  4,                            /* UD mushroom forest */
  10,                           /* Castle Wall */
  2,                            /* Castle Gate */
  2,                            /* Castle Main */
  4,                            /* negative plane */
  4                             /* plane of avernus */
};

extern const int track_limit[];
const int track_limit[] = {
  1,                            /* * Inside     */
  2,                            /* * City       */
  4,                            /* * Field      */
  4,                            /* * Forest     */
  3,                            /* * Hills      */
  2,                            /* * Mountains  */
  0,                            /* * Swimming   */
  0,                            /* * Unswimable */
  0,                            /* * No ground  */
  0,                            /* * underwater */
  0,                            /* * underwater walking */
  0,                            /* * plane of fire */
  0,                            /* * ocean      */
  4,                            /* * UD Wild    */
  2,                            /* * UD City    */
  1,                            /* * UD Inside  */
  0,                            /* * UD Swimming */
  0,                            /* * UD Unswimable */
  0,                            /* * UD No ground */
  0,                            /* * plane of air */
  0,                            /* * plane of water */
  0,                            /* * plane of earth */
  0,                            /* * plane of etheral */
  0,                            /* * plane of astral */
  5,                            /* desert */
  5,                            /* tundra/ice */
  2,                            /* swamp */
  2,                            /* UD mountains */
  2,                            /* UD slime pool */
  3,                            /* UD low cavern */
  2,                            /* UD liquid mithril */
  4,                            /* UD mushroom forest */
  0,                            /* Castle Wall */
  0,                            /* Castle Gate */
  0,                            /* Castle Main */
  0,                            /* negative plane */
  0,                            /* plane of avernus */
  4                             /* road */
};


const char *weekdays[7] = {
  "&+Wthe Day of the Moon",
  "&+ythe Day of the Storm",
  "&+mthe Day of the Deception",
  "&=LBthe Day of Thunder",
  "&+rthe Day of BloodLust",
  "&+cthe Day of the Great Gods",
  "&+Ythe Day of the Sun"
};

const char *month_name[17] = {
  "&+bMonth of the Cooling&N",  /* * 0 */
  "&+cMonth of FirstFrost&N",
  "&+WMonth of DeathIce&N",
  "&+CMonth of EverFreeze&N",
  "&+BMonth of FirstMelting&N",
  "&+WMonth of GreatWind&N",
  "&=LBMonth of Storms&N",
  "&+GMonth of Renewal&N",
  "&+gMonth of FullBloom&N",
  "&+MMonth of EternalDay&N",
  "&+YMonth of The Burning Sun&N",
  "&+yMonth of The Fevor&N",
  "&+RMonth of HeatsEnd&N",
  "&+yMonth of The Harvest Moon&N",
  "&+LMonth of The Rotting&N",
  "&+bMonth of Decay&N",
  "&+rMonth of BloodLust&N"
};

const char *fullness[] = {
  "less than half ",
  "about half ",
  "more than half ",
  ""
};

/*
 * adding new classes, these entries are minimum exp need for the level
 * 0-56
 */
// TOTALLVLS is the range of levels (0-62) = 63
/* After all that work making columns, I find there's a "new_exp_table". Sigh.
extern const int exp_table[];
const int exp_table[TOTALLVLS] = {                    0,
          1,      2000,      4000,      7000,     10000,
      16000,     26000,     32000,     45000,     60000,
      80000,    105000,    140000,    180000,    230000,
     290000,    390000,    500000,    650000,    850000,
    1400000,   2000000,   3000000,   4000000,   5000000,
    6000000,   7000000,   8000000,   9000000,  10000000,
   12000000,  14000000,  16000000,  18000000,  20000000,
   23000000,  26000000,  29000000,  32000000,  35000000,
   39000000,  43000000,  47000000,  51000000,  55000000,
   60000000,  67000000,  72000000,  77000000,  82000000,
   88000000,  94000000, 100000000, 115000000, 135000000,
  150000000, 170000000, 190000000, 220000000, 300000000,
  600000000, 600000000
};
*/

const char *player_bits[] = {
  "BRIEF",
  "NOSHOUT",
  "COMPACT",
  "DONTSET",
  "NAMES",
  "PETITION",
  "GCC",
  "WZLOG",
  "STATUS",
  "MAP",
  "VICIOUS",
  "ECHO",
  "SAVE-NOTE",
  "NOTELL",
  "DEBUGGER",
  "ANONYMOUS",
  "AGGIMMUNE",
  "WIZMUFFED",
  "NO-WHO",
  "PAGING_ON",
  "VNUM",
  "OLDSMARTP",
  "AFK",
  "SMARTPROMPT",
  "SILENCE",
  "FROZEN",
  "PLRLOG",
  "MAILING",
  "WRITING",
  "MORTALIZED",
  "MORPH",
  "BANLOG",
  "\n"
};

const char *player2_bits[] = {
  "NOLOCATE",
  "NOTITLE",
  "B_ALERT",
  "KING_VIEW",
  "SHIPMAP",
  "NOTAKE",
  "TERSE",
  "QUICKCHANT",
  "RWC",
  "PROJECT",
  "NPC_HOG",
  "UNUSED1",
  "NCHAT",
  "HARDCORE",
  "DAMAGE_DISPLAY",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "SHOW_QUEST",
  "SPEC_TIMER",
  "HEAL_DISPLAY",
  "BACK_RANK",
  "LOOK_GROUP",
  "EXP_DISPLAY",
  "SHOWSPEC",
  "MELEE_EXP",
  "HINTS",
  "WEBINFO",
  "WAIT",
  "NEWBIE",
  "NEWBIE_GUIDE",
  "ACC",
  "\n"
};

const char *player3_bits[] = {
  "FRAGLEADER",
  "LOWEST_FRAGS",
  "GUILD_NAME",
  "ACHIEVEMENT_SURNAME",
  "SURNAME_SERF",
  "SURNAME_COMMONER",
  "SURNAME_KNIGHT",
  "SURNAME_NOBLE",
  "SURNAME_LORD",
  "SURNAME_KING",
  "SURNAME_LIGHT",
  "SURNAME_DRAGON",
  "SURNAME_HEALS",
  "SURNAME_SERIAL",
  "SURNAME_REAPER",
  "\n"
};
  

const char *player_prompt[] = {
  "NONE",
  "HITS",
  "MAX-HITS",
  "MANA",
  "MAX-MANA",
  "MOVES",
  "MAX-MOVES",
  "TANK-COND",
  "TANK-NAME",
  "ENEMY-NAME",
  "ENEMY-COND",
  "VIS",
  "TWOLINE",
  "STATUS",
  "\n"
};

const char *player_law_flags[] = {
  "KN_ASH",
  "KN_BS",
  "KN_CAL",
  "KN_EM",
  "KN_LUI",
  "KN_MH",
  "KN_TT",
  "KN_VE",
  "KN_UNUSED1",
  "KN_UNUSED2",
  "OC_ASH",
  "OC_BS",
  "OC_CAL",
  "OC_EM",
  "OC_LUI",
  "OC_MH",
  "OC_TT",
  "OC_VE",
  "OC_UNUSED1",
  "OC_UNUSED2",
  "WT_ASH",
  "WT_BS",
  "WT_CAL",
  "WT_EM",
  "WT_LUI",
  "WT_MH",
  "WT_TT",
  "WT_VE",
  "WT_UNUSED1",
  "WT_UNUSED2",
  "KN_EVIL",
  "OC_EVIL",
  "\n"
};

const char *position_types[] = {
  "Prone",
  "Kneeling",
  "Sitting",
  "Standing",
  "Dead",
  "Dying",
  "Incapacitated",
  "Sleeping",
  "Resting",
  "Normal",
  "\n"
};

const char *target_types[] = {
  "TAR_IGNORE",
  "TAR_CHAR_ROOM",
  "TAR_CHAR_WORLD",
  "TAR_FIGHT_SELF",
  "TAR_FIGHT_VICT",
  "TAR_SELF_ONLY",
  "TAR_SELF_NONO",
  "TAR_OBJ_INV",
  "TAR_OBJ_ROOM",
  "TAR_OBJ_WORLD",
  "TAR_OBJ_EQUIP",
  "TAR_CHAR_RANGE",
  "TAR_RANGE1",
  "TAR_RANGE2",
  "TAR_AREA",
  "\n"
};

const char *size_types[] = {
  "none",
  "tiny",
  "small",
  "medium",
  "large",
  "huge",
  "giant",
  "gargantuan",
  "\n"
};

const char *item_size_types[] = {
  "none",
  "tiny",
  "small",
  "medium",
  "large",
  "huge",
  "giant",
  "gargantuan",
  "small-medium",
  "medium-large",
  "magical",
  "\n"
};

// Total Connection types + "\n"
const char *connected_types[TOTAL_CON+1] = {
  "PLAYING",
  "GET_NAME",
  "CONF_NAME",
  "GET_PWD",
  "GET_N_PWD",
  "CONF_N_PWD",
  "PICK_SEX",
  "READ_MOTD",
  "MENU",
  "CONF_PUNT",
  "PICK_CLASS",
  "LINK_DEAD",
  "CONF_PWD",
  "CONF_C_PWD",
  "FLUSH",
  "GET_C_PWD",
  "CONF_D_PWD",
  "PICK_RACE",
  "PICK_TERM",
  "DESCRIBE",
  "DISCLAIMER",
  "DISCLAIMR1",
  "DISCLAIMR2",
  "DISCLAIMR3",
  "DISCLAIMR4",
  "DISCLAIMR5",
  "DISCLAIMR6",
  "AGREE_DSCL",
  "CLASS_INFO",
  "UNUSED",
  "RACEWARS",
  "LINKVR",
  "LINKSET",
  "APROP_NAME",
  "DELETE",
  "REROLLING",
  "BONUS1",
  "BONUS2",
  "BONUS3",
  "KEEPCHAR",
  "ALIGN",
  "HOMETOWN",
  "ACCEPTWAIT",
  "WELCOME",
  "NEWNAME",
  "HOST_LOOKUP",
  "OOLC",
  "ROLC",
  "ZOLC",
  "MOLC",
  "SOLC",
  "QOLC",
  "BONUS4",
  "BONUS5",
  "TEXT_EDITOR",
  "PICK_SIDE",
  "ENTER_LOGIN",
  "ENTER_HOST",
  "CONFIRM_EMAIL",
  "EXIT",
  "GET_ACCT_NAME",
  "GET_ACCT_PASSWD",
  "IS_ACCT_CONFIRMED",
  "DISPLAY_ACCT_MENU",
  "CONFIRM_ACCT",
  "VERIFY_NEW_ACCT_NAME",
  "GET_NEW_ACCT_EMAIL",
  "VERIFY_NEW_ACCT_EMAIL",
  "NEW_ACCT_PASSWD",
  "VERIFY_NEW_ACCT_PASSWD",
  "VERIFY_NEW_ACCT_INFO",
  "ACCT_SELECT_CHAR",
  "ACCT_NEW_CHAR",
  "ACCT_DELETE_CHAR",
  "ACCT_DISPLAY_INFO",
  "ACCT_CHANGE_EMAIL",
  "ACCT_CHANGE_PASSWD",
  "ACCT_DELETE_ACCT",
  "VERIFY_ACCT_DELETE_ACCT",
  "VERIFY_ACCT_INFO",
  "ACCT_NEW_CHAR_NAME",
  "HARDCORE",
  "NEWBIE",
  "SWAPSTATYN",
  "SWAPSTAT",
  "\n"
};

/*
 * This sort of data, is better handled by formulas, however: in this
 * visual form, cpu load is nil, and it's easy to see in order to tweak
 * it.  Once it's finalized, we can reduce it to macros.  JAB
 */
extern const struct str_app_type str_app[52];
const struct str_app_type str_app[52] = {
/*
 * +h,  +d,wgt c,wgt w
 */
  {-5, -12,    0,   0},          /* *  0 (      0) */
  {-5, -12,    5,   1},          /* *  1 (  1-  9) */
  {-3,  -9,   12,   1},          /* *  2 ( 10- 15) */
  {-3,  -7,   25,   2},          /* *  3 ( 16- 21) */
  {-2,  -5,   40,   3},          /* *  4 ( 22- 27) */
  {-2,  -3,   50,   5},          /* *  5 ( 28- 33) */
  {-1,   0,   60,   8},          /* *  6 ( 34- 39) */
  { 0,   1,   75,  10},          /* *  7 ( 40- 45) */
  { 0,   2,   85,  12},          /* *  8 ( 46- 50) */
  { 0,   3,   95,  13},          /* *  9 ( 51- 55) */
  { 0,   4,  105,  15},          /* * 10 ( 56- 61) */
  { 1,   5,  135,  17},          /* * 11 ( 62- 67) */
  { 1,   6,  175,  19},          /* * 12 ( 68- 73) */
  { 1,   7,  225,  23},          /* * 13 ( 74- 79) */
  { 2,   9,  285,  27},          /* * 14 ( 80- 85) */
  { 3,  10,  355,  30},          /* * 15 ( 86- 91) */
  { 3,  12,  445,  33},          /* * 16 ( 92-100) (human excellent) */
  { 3,  13,  535,  36},          /* * 17 (101-112) */
  { 4,  15,  625,  39},          /* * 18 (113-124) */
  { 4,  17,  715,  42},          /* * 19 (125-136) */
  { 4,  19,  805,  45},          /* * 20 (137-148) */
  { 5,  21,  895,  48},          /* * 21 (149-160) */
  { 5,  24,  985,  51},          /* * 22 (161-172) */
  { 5,  27, 1075,  54},          /* * 23 (173-184) */
  { 6,  30, 1165,  57},          /* * 24 (185-196) */
  { 6,  33, 1255,  60},          /* * 25 (197-208) */
  { 6,  36, 1345,  64},          /* * 26 (209-220) */
  { 7,  39, 1435,  68},          /* * 27 (221-232) */
  { 7,  42, 1525,  72},          /* * 28 (233-244) */
  { 7,  45, 1615,  76},          /* * 29 (245-256) */
  { 8,  48, 1705,  80},          /* * 30 (257-268) */
  { 8,  51, 1800,  85},          /* * 31 (269-280) */
  { 8,  54, 1900,  90},          /* * 32 (281-292) */
  { 9,  57, 2000,  95},          /* * 33 (293-304) */
  { 9,  60, 2100, 100},          /* * 34 (305-316) */
  { 9,  63, 2200, 105},          /* * 35 (317-328) */
  {10,  66, 2300, 110},          /* * 36 (329-340) */
  {10,  69, 2400, 115},          /* * 37 (341-352) */
  {10,  72, 2500, 120},          /* * 38 (353-364) */
  {11,  75, 2600, 125},          /* * 39 (365-376) */
  {11,  78, 2700, 130},          /* * 40 (377-388) */
  {11,  81, 2800, 135},          /* * 41 (389-400) */
  {12,  84, 2900, 140},          /* * 42 (401-412) */
  {12,  87, 3000, 145},          /* * 43 (413-424) */
  {12,  90, 3100, 150},          /* * 44 (425-436)  ( ogre berserk str max) */
  {13,  93, 3200, 155},          /* * 45 (437-448) */
  {13,  96, 3300, 160},          /* * 46 (449-460) */
  {13,  99, 3400, 165},          /* * 47 (461-472) */
  {14, 102, 3500, 170},          /* * 48 (473-484) */
  {14, 105, 3600, 175},          /* * 49 (485-496) */
  {14, 108, 3700, 180},          /* * 50 (497-508) */
  {15, 111, 3800, 185}           /* * 51 (509-512) */
};


extern const struct dex_app_type dex_app[52];
const struct dex_app_type dex_app[52] = {
/*
 * react, missile, steal, pick, traps
 */
  {-7, -7, -99, -99, -99},      /* * 0 (      0) */
  {-6, -6, -90, -90, -90},      /* * 1 (  1-  9) */
  {-4, -4, -80, -80, -80},      /* * 2 ( 10- 15) */
  {-3, -3, -70, -55, -70},      /* * 3 ( 16- 21) */
  {-2, -2, -45, -35, -45},      /* * 4 ( 22- 27) */
  {-1, -1, -25, -20, -25},      /* * 5 ( 28- 33) */
  {0, 0, -10, -10, -10},        /* * 6 ( 34- 39) */
  {0, 0, -5, -5, -5},           /* * 7 ( 40- 45) */
  {0, 0, 0, 0, 0},              /* * 8 ( 46- 50) */
  {0, 0, 0, 0, 0},              /* * 9 ( 51- 55) */
  {0, 0, 0, 0, 0},              /* * 10 ( 56- 61) */
  {0, 0, 0, 0, 0},              /* * 11 ( 62- 67) */
  {0, 0, 0, 0, 0},              /* * 12 ( 68- 73) */
  {1, 1, 10, 15, 0},            /* * 13 ( 74- 79) */
  {2, 2, 15, 20, 0},            /* * 14 ( 80- 85) */
  {3, 3, 20, 25, 5},            /* * 15 ( 86- 91) */
  {4, 4, 25, 25, 10},           /* * 16 ( 92-100) (human max) */
  {5, 5, 30, 30, 10},           /* * 17 (101-112) */
  {6, 6, 35, 30, 10},           /* * 18 (113-124) */
  {7, 7, 40, 30, 15},           /* * 19 (125-136) */
  {8, 8, 45, 35, 15},           /* * 20 (137-148) */
  {8, 8, 60, 35, 15},           /* * 21 (149-160) */
  {9, 9, 75, 35, 15},           /* * 22 (161-172) */
  {9, 9, 80, 35, 15},           /* * 23 (173-184) */
  {9, 9, 85, 40, 20},           /* * 24 (185-196) */
  {9, 9, 90, 40, 20},           /* * 25 (197-208) */
  {9, 9, 90, 40, 20},           /* * 26 (209-220) */
  {9, 9, 90, 40, 20},           /* * 27 (221-232) */
  {9, 9, 90, 40, 20},           /* * 28 (233-244) */
  {9, 9, 90, 40, 20},           /* * 29 (245-256) */
  {9, 9, 90, 40, 20},           /* * 30 (257-268) */
  {9, 9, 90, 40, 20},           /* * 31 (269-280) */
  {9, 9, 90, 40, 20},           /* * 32 (281-292) */
  {9, 9, 90, 40, 20},           /* * 33 (293-304) */
  {9, 9, 90, 40, 20},           /* * 34 (305-316) */
  {9, 9, 90, 40, 20},           /* * 35 (317-328) */
  {9, 9, 90, 40, 20},           /* * 36 (329-340) */
  {9, 9, 90, 40, 20},           /* * 37 (341-352) */
  {9, 9, 90, 40, 20},           /* * 38 (353-364) */
  {9, 9, 90, 40, 20},           /* * 39 (365-376) */
  {9, 9, 90, 40, 20},           /* * 40 (377-388) */
  {9, 9, 90, 40, 20},           /* * 41 (389-400) */
  {9, 9, 90, 40, 20},           /* * 42 (401-412) */
  {9, 9, 90, 40, 20},           /* * 43 (413-424) */
  {9, 9, 90, 40, 20},           /* * 44 (425-436) */
  {9, 9, 90, 40, 20},           /* * 45 (437-448) */
  {9, 9, 90, 40, 20},           /* * 46 (449-460) */
  {9, 9, 90, 40, 20},           /* * 47 (461-472) */
  {9, 9, 90, 40, 20},           /* * 48 (473-484) */
  {9, 9, 90, 40, 20},           /* * 49 (485-496) */
  {9, 9, 90, 40, 20},           /* * 50 (497-508) */
  {9, 9, 90, 40, 20}            /* * 51 (509-520)  */
};

extern const struct agi_app_type agi_app[52];
const struct agi_app_type agi_app[52] = {
/*
 * def, sneak, hide
 */
  {45, -99, -99},               /* * 0 (      0) */
  {30, -95, -90},               /* * 1 (  1-  9) */
  {20, -85, -80},               /* * 2 ( 10- 15) */
  {13, -80, -55},               /* * 3 ( 16- 21) */
  {8, -55, -35},                /* * 4 ( 22- 27) */
  {4, -35, -20},                /* * 5 ( 28- 33) */
  {0, -20, -10},                /* * 6 ( 34- 39) */
  {0, -10, -5},                 /* * 7 ( 40- 45) */
  {0, -5, 0},                   /* * 8 ( 46- 50) */
  {0, 0, 0},                    /* * 9 ( 51- 55) */
  {0, 0, 0},                    /* * 10 ( 56- 61) */
  {0, 0, 0},                    /* * 11 ( 62- 67) */
  {-4, 0, 0},                   /* * 12 ( 68- 73) */
  {-8, 0, 0},                   /* * 13 ( 74- 79) */
  {-13, 5, 5},                  /* * 14 ( 80- 85) */
  {-20, 10, 10},                /* * 15 ( 86- 91) */
  {-30, 15, 15},                /* * 16 ( 92-100) (human max) */
  {-38, 15, 15},                /* * 17 (101-112) */
  {-45, 15, 20},                /* * 18 (113-124) */
  {-51, 20, 20},                /* * 19 (125-136) */
  {-56, 20, 20},                /* * 20 (137-148) */
  {-60, 20, 25},                /* * 21 (149-160) */
  {-63, 25, 25},                /* * 22 (161-172) */
  {-65, 25, 25},                /* * 23 (173-184) */
  {-67, 25, 30},                /* * 24 (185-196) */
  {-68, 30, 30},                /* * 25 (197-208) */
  {-69, 30, 30},                /* * 26 (209-220) */
  {-70, 30, 30},                /* * 27 (221-232) */
  {-71, 30, 30},                /* * 28 (233-244) */
  {-72, 30, 30},                /* * 29 (245-256) */
  {-73, 30, 30},                /* * 30 (257-268) */
  {-74, 30, 30},                /* * 31 (269-280) */
  {-75, 30, 30},                /* * 32 (281-292) */
  {-76, 30, 30},                /* * 33 (293-304) */
  {-77, 30, 30},                /* * 34 (305-316) */
  {-78, 30, 30},                /* * 35 (317-328) */
  {-79, 30, 30},                /* * 36 (329-340) */
  {-80, 30, 30},                /* * 37 (341-352) */
  {-81, 30, 30},                /* * 38 (353-364) */
  {-82, 30, 30},                /* * 39 (365-376) */
  {-83, 30, 30},                /* * 40 (377-388) */
  {-84, 30, 30},                /* * 41 (389-400) */
  {-85, 30, 30},                /* * 42 (401-412) */
  {-86, 30, 30},                /* * 43 (413-424) */
  {-87, 30, 30},                /* * 44 (425-436) */
  {-88, 30, 30},                /* * 45 (437-448) */
  {-89, 30, 30},                /* * 46 (449-460) */
  {-90, 30, 30},                /* * 47 (461-472) */
  {-91, 30, 30},                /* * 48 (473-484) */
  {-92, 30, 30},                /* * 49 (485-496) */
  {-93, 30, 30},                /* * 50 (497-508) */
  {-94, 30, 30}                 /* * 51 (509-520)  */
};

/*
 * [int] apply (all)
 */
extern const struct int_app_type int_app[52];
const struct int_app_type int_app[52] = {
/*
 * learn
 */
  {3},                          /* * 0 (      0) */
  {5},                          /* * 1 (  1-  9) */
  {7},                          /* * 2 ( 10- 15) */
  {8},                          /* * 3 ( 16- 21) */
  {9},                          /* * 4 ( 22- 27) */
  {10},                         /* * 5 ( 28- 33) */
  {11},                         /* * 6 ( 34- 39) */
  {12},                         /* * 7 ( 40- 45) */
  {13},                         /* * 8 ( 46- 50) */
  {15},                         /* * 9 ( 51- 55) */
  {17},                         /* * 10 ( 56- 61) */
  {19},                         /* * 11 ( 62- 67) */
  {22},                         /* * 12 ( 68- 73) */
  {25},                         /* * 13 ( 74- 79) */
  {30},                         /* * 14 ( 80- 85) */
  {35},                         /* * 15 ( 86- 91) */
  {40},                         /* * 16 ( 92-100) (human max) */
  {45},                         /* * 17 (101-112) */
  {50},                         /* * 18 (113-124) */
  {53},                         /* * 19 (125-136) */
  {55},                         /* * 20 (137-148) */
  {56},                         /* * 21 (149-160) */
  {60},                         /* * 22 (161-172) */
  {70},                         /* * 23 (173-184) */
  {80},                         /* * 24 (185-196) */
  {90},                         /* * 25 (197-208) */
  {90},                         /* * 26 (209-220) */
  {90},                         /* * 27 (221-232) */
  {90},                         /* * 28 (233-244) */
  {90},                         /* * 29 (245-256) */
  {90},                         /* * 30 (257-268) */
  {90},                         /* * 31 (269-280) */
  {90},                         /* * 32 (281-292) */
  {90},                         /* * 33 (293-304) */
  {90},                         /* * 34 (305-316) */
  {90},                         /* * 35 (317-328) */
  {90},                         /* * 36 (329-340) */
  {90},                         /* * 37 (341-352) */
  {90},                         /* * 38 (353-364) */
  {90},                         /* * 39 (365-376) */
  {90},                         /* * 40 (377-388) */
  {90},                         /* * 41 (389-400) */
  {90},                         /* * 42 (401-412) */
  {90},                         /* * 43 (413-424) */
  {90},                         /* * 44 (425-436) */
  {90},                         /* * 45 (437-448) */
  {90},                         /* * 46 (449-460) */
  {90},                         /* * 47 (461-472) */
  {90},                         /* * 48 (473-484) */
  {90},                         /* * 49 (485-496) */
  {90},                         /* * 50 (497-508) */
  {90}                          /* * 51 (509-520)  */
};

/*
 * [wis] apply (all)
 */
extern const struct wis_app_type wis_app[52];
const struct wis_app_type wis_app[52] = {
/*
 * bonus pracs
 */
  {0},                          /* * 0 (      0) */
  {0},                          /* * 1 (  1-  9) */
  {0},                          /* * 2 ( 10- 15) */
  {0},                          /* * 3 ( 16- 21) */
  {0},                          /* * 4 ( 22- 27) */
  {0},                          /* * 5 ( 28- 33) */
  {0},                          /* * 6 ( 34- 39) */
  {0},                          /* * 7 ( 40- 45) */
  {0},                          /* * 8 ( 46- 50) */
  {0},                          /* * 9 ( 51- 55) */
  {0},                          /* * 10 ( 56- 61) */
  {1},                          /* * 11 ( 62- 67) */
  {1},                          /* * 12 ( 68- 73) */
  {1},                          /* * 13 ( 74- 79) */
  {2},                          /* * 14 ( 80- 85) */
  {2},                          /* * 15 ( 86- 91) */
  {2},                          /* * 16 ( 92-100) (human max) */
  {3},                          /* * 17 (101-112) */
  {3},                          /* * 18 (113-124) */
  {3},                          /* * 19 (125-136) */
  {3},                          /* * 20 (137-148) */
  {3},                          /* * 21 (149-160) */
  {3},                          /* * 22 (161-172) */
  {3},                          /* * 23 (173-184) */
  {3},                          /* * 24 (185-196) */
  {3},                          /* * 25 (197-208) */
  {3},                          /* * 26 (209-220) */
  {3},                          /* * 27 (221-232) */
  {3},                          /* * 28 (233-244) */
  {3},                          /* * 29 (245-256) */
  {3},                          /* * 30 (257-268) */
  {3},                          /* * 31 (269-280) */
  {3},                          /* * 32 (281-292) */
  {3},                          /* * 33 (293-304) */
  {3},                          /* * 34 (305-316) */
  {3},                          /* * 35 (317-328) */
  {3},                          /* * 36 (329-340) */
  {3},                          /* * 37 (341-352) */
  {3},                          /* * 38 (353-364) */
  {3},                          /* * 39 (365-376) */
  {3},                          /* * 40 (377-388) */
  {3},                          /* * 41 (389-400) */
  {3},                          /* * 42 (401-412) */
  {3},                          /* * 43 (413-424) */
  {3},                          /* * 44 (425-436) */
  {3},                          /* * 45 (437-448) */
  {3},                          /* * 46 (449-460) */
  {3},                          /* * 47 (461-472) */
  {3},                          /* * 48 (473-484) */
  {3},                          /* * 49 (485-496) */
  {3},                          /* * 50 (497-508) */
  {3}                           /* * 51 (509-520)  */
};

/*
 * [Cha] apply (all)
 */
extern const struct cha_app_type cha_app[52];
const struct cha_app_type cha_app[52] = {
  {-50},                        /* * 0 (      0) */
  {-40},                        /* * 1 (  1-  9) */
  {-25},                        /* * 2 ( 10- 15) */
  {-10},                        /* * 3 ( 16- 21) */
  {-5},                         /* * 4 ( 22- 27) */
  {0},                          /* * 5 ( 28- 33) */
  {0},                          /* * 6 ( 34- 39) */
  {0},                          /* * 7 ( 40- 45) */
  {0},                          /* * 8 ( 46- 50) */
  {0},                          /* * 9 ( 51- 55) */
  {0},                          /* * 10 ( 56- 61) */
  {0},                          /* * 11 ( 62- 67) */
  {1},                          /* * 12 ( 68- 73) */
  {3},                          /* * 13 ( 74- 79) */
  {6},                          /* * 14 ( 80- 85) */
  {10},                         /* * 15 ( 86- 91) */
  {15},                         /* * 16 ( 92-100) (human max) */
  {20},                         /* * 17 (101-112) */
  {24},                         /* * 18 (113-124) */
  {28},                         /* * 19 (125-136) */
  {32},                         /* * 20 (137-148) */
  {35},                         /* * 21 (149-160) */
  {38},                         /* * 22 (161-172) */
  {41},                         /* * 23 (173-184) */
  {43},                         /* * 24 (185-196) */
  {45},                         /* * 25 (197-208) */
  {47},                         /* * 26 (209-220) */
  {49},                         /* * 27 (221-232) */
  {51},                         /* * 28 (233-244) */
  {52},                         /* * 29 (245-256) */
  {53},                         /* * 30 (257-268) */
  {54},                         /* * 31 (269-280) */
  {55},                         /* * 32 (281-292) */
  {56},                         /* * 33 (293-304) */
  {57},                         /* * 34 (305-316) */
  {58},                         /* * 35 (317-328) */
  {59},                         /* * 36 (329-340) */
  {60},                         /* * 37 (341-352) */
  {60},                         /* * 38 (353-364) */
  {60},                         /* * 39 (365-376) */
  {60},                         /* * 40 (377-388) */
  {60},                         /* * 41 (389-400) */
  {60},                         /* * 42 (401-412) */
  {60},                         /* * 43 (413-424) */
  {60},                         /* * 44 (425-436) */
  {60},                         /* * 45 (437-448) */
  {60},                         /* * 46 (449-460) */
  {60},                         /* * 47 (461-472) */
  {60},                         /* * 48 (473-484) */
  {60},                         /* * 49 (485-496) */
  {60},                         /* * 50 (497-508) */
  {60}                          /* * 51 (509-520)  */
};

const char *coin_names[] = {
  "&+ycopper&N",
  "silver",
  "&+Ygold&N",
  "&+Wplatinum&N"
};

const char *coin_abbrev[] = {
  "c",
  "s",
  "g",
  "p"
};

const char *resource_list[] = {
  "None",
  "Ore",
  "Precious ore",
  "Gemstones",
  "Rare herbs",
  "High mineral count",
  "Building-Grade timber",
  "Fertile land",
  "Peasant villages",
  "\n"
};

const char *kingdom_type_list[] = {
  "None",
  "Town Owned",
  "Guild",
  "Game",
  "\n"
};

const char *town_name_list[] = {
  "Nowhere",                    /* must remain 'nowhere'! */
  "Tharnadia",
  "Ixarkon",
  "Arachdrathos",
  "Sylvandawn",
  "Kimordril",
  "Khildarak",
  "Woodseer",
  "Ashrumite",
  "Faang",
  "Ghore",
  "Ugta",
  "Bloodstone",
  "Shady",
  "Nax",
  "Marigot",
  "Charing",
  "Ancient City Ruins",         /* last hometown that can have justice */
  "Payang",
  "Githyanki Hometown",
  "Moregeeth",
  "Harpy",
  "Outpost of Ailvio (In-depth Introduction)",
  "Plane of Life (Basic Introduction)",
  "Orog Encampment",
  "\n"
};


/*
 * before messing with this table, keep the following in mind: 1. If the
 * average of the 1st 8 stats is too high, the race will be too powerful,
 * unless there is a racial liability that balances this (lower than
 * average hits, etc). 2. If the average of the 1st 8 stats is too low,
 * the race will be too weak, again, unless there is a racial advantage
 * that balances this (troll regen, demigod-like ogre strength, amazing
 * halfing dex, etc). 3. These numbers are percents, ie. 100 is 100% or
 * 1.0x, 250 is 250% or 2.5x. 4. Power is strictly for magic, clerical
 * spells are completely different. 5. unsure how Karma and Luck will be
 * implimented, so they default to 100. 6. This table affects stats at
 * runtime, a change here, is immediately reflected in player/mob
 * abilities. 7. The values I set when writing the new system were
 * guesses, plain and simple, so they are most likely going to need
 * tweaking.  JAB
 */

struct stat_data stat_factor[LAST_RACE + 1];

/*
 * base age, base moves, base mana, max mana, hp_bonus, max age, speed, damage
 */
struct racial_data_type racial_data[LAST_RACE + 1] = {
// base_age,      max_mana,
//      base_vitality,  hp_bonus,
//           base_mana,    max_age
  {  17,  90,  50,  100, 0, 9999} , // none
  {  17,  90,  50,  100, 0,  100} , // human
  {  15, 130,  45,   80, 1,  100} , // barbarian
  { 100, 100,  60,  120,-1, 1200} , // drow
  { 150, 100,  60,  120,-1, 1200} , // grey
  {  50,  85,  45,   90, 1,  800} , // mountain
  {  40,  90,  45,   95, 1,  650} , // duergar
  {  33,  80,  50,   95, 0,  199} , // halfling
  {  90,  80,  55,  150,-1,  450} , // gnome
  {  20, 105,  35,   75, 1,   75} , // ogre - hp_bonus mostly in handler.c
  {  16, 110,  35,   75, 2,   75} , // troll
  {  40,  95,  55,  110, 0,  225} , // half-elf
  { 100, 100, 500,  600,-1,  325} , // Illithid
  {  15,  90,  50,  100, 0,   80} , // orc
  {  15, 140,  50,  100, 2,   65} , // thri-kreen
  {  18, 200,  50,  100, 1,  100} , // centaur
  {  20, 100, 200,  400, 0,  225} , // githyanki
  {  55, 150,  50,   50, 1,  300} , // minotaur
  {  90, 100, 200,  150,-1,  250} , // shade
  { 100, 110, 200,   75, 2,  250} , // Revenant
  {   8,  80,  60,  150,-1,   40} , // goblin
  { 550, 100, 200,  600, 0, 1500} , // lich
  { 300, 100, 200,  600, 1, 1000} , // vampire
  { 550, 100, 200,  600, 2, 1500} , // death knight
  { 150, 130, 200,  600, 0,  500} , // shadow beast
  {  25, 100,  35,   75, 1,  135} , // Storm Giant
  { 250, 105,  35,   75, 1, 1000} , // Wight
  { 250, 110, 200,  400,-1, 1000} , // Phantom
  {  14, 170, 200,  150,-1,   65} , // Harpy
  {  15, 100,  50,  100, 1,   60} , // Orog
  {  20, 100, 200,  400, 0,  225} , // Githzerai
  {  10, 200,  60,  120,-1,   90} , // Drider
  {  12,  90,  10,   50,-1,   55} , // Kobold
  {   6, 120, 250,  600,-3,   24} , // Pillithid
  {  12, 120,  10,   50, 1,   60} , // Kuo Toa
  { 250, 120,  10,   50, 1,  750} , // Wood Elf
  {  12, 140,  12,   55, 2,   45} , // Firbolg
  { 250, 130, 200,  150, 1, 1000} , // Agathinon
  { 150, 120,  10,   50, 1, 1000} , // Eladrin
  {  55, 170, 200,  150, 0,  250} , // Gargyole
  {   8, 170, 200,  150, 1,   20} , // Fire Elemental
  {   8, 170, 200,  150, 1,   20} , // Air Elemental
  {   8, 170, 200,  150, 1,   20} , // Water Elemental
  {   8, 170, 200,  150, 1,   20} , // Earth Elemental
  { 400, 170, 200,  150, 2, 1000} , // Demon
  { 400, 170, 200,  150, 2, 1000} , // Devil
  {  14, 170, 200,  150, 0,   20} , // Undead
  { 500, 170, 200,  150, 1, 1500} , // Vampire
  {1000, 170, 200,  150,-1, 1500} , // Ghost
  {  14, 170, 200,  150, 2,  150} , // Lycanthrope
  {  14, 170, 200,  150, 2,   55} , // Giant
  {  14, 170, 200,  150, 1,   65} , // Half Orc
  { 500, 170, 200,  150, 2, 1500} , // Golem
  {  65, 170, 200,  150,-2,  250} , // Faerie
  { 750, 100, 200,  120, 2, 1500} , // Dragon
  { 400, 100, 200,  120, 1, 1500} , // Dragonkin
  {   5,  60,  50,  100,-1,   20} , // Reptile
  {   5,  60,  50,  100,-1,   20} , // Snake
  {   1, 120,  50,  100,-2,    4} , // Insect
  {   2,  40,  50,  100,-2,    6} , // Arachnid
  {  15,  50,  50,  100, 0,   50} , // Aquatic
  {   5, 200,  50,  100, 0,   20} , // Flying Animal
  {  10, 350,  50,  100, 0,   25} , // Quadruped
  {  15,  90,  50,  100, 0,   45} , // Primate
  {  15, 100,  50,  100, 0,   75} , // Humanoid
  {  10,  75,  50,  100, 0,   60} , // Animal
  {  60,   0,  50,  100, 2,  200} , // Plant
  {   7,  50,  50,  100, 0,   25} , // Herbavore
  {   9,  75,  50,  100, 0,   40} , // Carnivore
  {   1,  50,  50,  100,-2,    2} , // Parasite
  { 200, 150,  50,  100, 1, 1000} , // Beholder
  { 500, 300,  50,  100, 2, 1500} , // Dracolich
  {  15,  30,  50,  100,-1,   50} , // Slime
  { 250, 150, 200,  300, 1, 1000} , // Angel
  { 150, 175, 100,  200, 1,  400} , // Rakshasa
  {1000,   0,   0,    0, 0, 1000} , // Construct
  { 500, 150, 300,  500, 2, 1000} , // Efreet
  {  35,  90,  50,  100, 2,  125} , // Snow Ogre
  { 100, 250, 500,  600, 1,  500} , // Beholderkin
  {  25, 125,   0,    0, 2,  125} , // Zombie
  { 150, 250, 300,  500, 1, 1000} , // Spectre
  { 200, 250,   0,    0, 2, 1000} , // Skeleton
  { 500, 300, 800, 1000, 1, 1500} , // Wraith
  { 500, 300, 500,  700, 1, 1500} , // Shadow
  {  14, 170, 200,  150, 2,  500} , // Purple Worm
  {   8, 170, 200,  150, 1,   20} , // Void Elemental
  {   8, 170, 200,  150, 1,   20} , // Ice Elemental
  { 500, 150, 300,  500, 2, 1500} , // Phoenix
  { 200, 250,   0,    0, 2, 1000} , // Archon
  { 500, 300, 800, 1000, 1, 1500} , // Asura
  { 500, 300,  50,  100, 2, 1000} , // Titan
  { 125, 300,  50,  100, 2,  500} , // Avatar
  { 150, 170, 200,  150, 1,  500} , // Ghaele
  { 100, 300, 800, 1000, 1,  300} , // Bralani
  { 100, 120,   0,    0, 1,  300} , // Whiner
  { 500, 300, 500,  700, 1, 1500} , // Incubus
  { 500, 300, 500,  700, 1, 1500} , // Succubus
  {  14, 170, 200,  150, 2,   55} , // Fire Giant
  {  14, 170, 200,  150, 2,   55} , // Frost Giant
  { 100, 300, 800, 1000, 1,  300}   // Deva

};

/*
 *    SAM 7-94, Minimum stats required to choose a class, player must
 *      get a value greater or equal to the value in this table.
 *      7-31-94: lowered mins for fighter, cleric, rogue, conjurer,
 *      shaman by 1 temporarily.
 *      2-6-95: converted to new stats, note that these figures are for the
 *      stats unmodified by race.
 */

extern const int min_stats_for_class[CLASS_COUNT + 1][8];
const int min_stats_for_class[CLASS_COUNT + 1][8] = {
// Str, Dex, Agi, Con, Pow, Int, Wis, Cha
  {  0,   0,   0,   0,   0,   0,   0,   0}, // None
  { 55,  20,  25,  55,   0,   0,   0,   0}, // Warrior
  { 40,  40,  30,  40,   0,  40,  10,   0}, // Ranger
  {  0,   0,   0,   0,  90,  50,   0,   0}, // Psionicist
  { 55,  20,  25,  50,   0,   0,  65,  50}, // Paladin
  { 55,  20,  25,  50,   0,   0,  65,  50}, // Anti-Paladin
  { 40,   0,  20,  20,   0,   0,  55,  50}, // Cleric
  { 65,  60,  75,  50,   0,   0,  40,   0}, // Monk
  {  0,   0,   0,  20,   0,  50,  65,  40}, // Druid
  {  0,   0,   0,   0,   0,  50,  50,  40}, // Shaman
  {  0,   0,  30,   0,   0,  75,  10,   0}, // Sorcerer
  {  0,   0,  30,   0,   0,  75,  10,   0}, // Necromancer
  { 40,  30,   0,  40,   0,  60,  10,   0}, // Conjurer
  {  0,  75,  65,   0,   0,  30,   0,  30}, // Rogue
  {  0,  75,  60,   0,   0,  30,   0,   0}, // Assassin
  { 55,  50,  30,  40,   0,  30,  30,   0}, // Mercenary
  {  0,  55,  30,  30,   0,  60,  30,  70}, // Bard
  {  0,  75,  75,   0,   0,  30,   0,   0}, // Thief
  {  0,  75,  75,   0,   0,  30,   0,   0}, // Warlock
  {  0,  75,  75,   0,   0,  30,   0,   0}, // Mindflayer
  { 80,  75,  75,   0,   0,  30,   0,   0}, // Alchemist
  { 65,  30,  30,  65,   0,   0,   0,   0}, // Berserker
  { 40,  40,  30,  40,   0,  40,  10,   0}, // Reaver
  { 40,  30,   0,  40,   0,  60,  10,   0}, // Illusionist
  { 40,   0,  20,  40,   0,   0,  65,   0}, // Blighter
  { 60,  40,  30,  50,   0,  40,  10,  50}, // Dreadlord
  {  0,   0,   0,   0,   0,  50,  50,  40}, // Ethermancer
  { 40,  50,   0,  50,   0,  0,   60,   0}, // Avenger
  {  0,   0,  30,  30,   0,  75,  10,  20}, // Theurgist
  {  0,   0,  40,  40,   0,  60,   0,  60}  // Summoner
};

/*
 *      SAM 7-94, Initial alignments of players.  Use encoding to
 *      specify evil, neutral, good, or players choice for each race/class
 *
 *      Encoding:       -1 = evil (-1000)
 *                       0 = neutral (0)
 *                       1 = good (+1000)
 *                       2 = choice (any)
 *                       3 = choice (good/neutral)
 *                       4 = choice (neutral/evil)
 *                       5 = not a legal race/class combo
 */ 

extern const int class_table[LAST_RACE + 1][CLASS_COUNT + 1];
const int class_table[LAST_RACE + 1][CLASS_COUNT + 1] = {
/*
 *      Wa   Ra   Ps   Pa   Ap   Cl   Mo   Dr   Sh   So   Ne   Co   Ro   Ass  Me   Ba   Th   Wl   Mf   Al   Be   Rv   Il   Bli  DL   Eth  Av   Tg  Sum =Class
 *    -1) E, 0) N, 1) G,  2) Choice (ANY), 3) Choice (G/N), 4) Choice) N/E 5) NOGO */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* None */
  { 5 ,  2 ,  1 ,  1 ,  1 ,  5 ,  2 ,  1 ,  0 ,  2 ,  2 , -1 ,  2 ,  4 ,  5 ,  2 ,  1 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  2 ,  5 ,  5 ,  0 ,  5 ,  5 ,  2 },  /* Human */
  { 5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  2 ,  5 ,  5 ,  5 ,  0 ,  5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Barbarian */
  { 5 , -1 ,  5 , -1 ,  5 , -1 , -1 , -1 ,  5 , -1 , -1 , -1 , -1 , -1 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 ,  0 ,  5 ,  5 , -1 },  /* Drow Elf */
  { 5 ,  2 ,  1 ,  1 ,  1 ,  5 ,  2 ,  2 ,  0 ,  2 ,  2 ,  5 ,  2 ,  0 ,  5 ,  2 ,  1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  2 ,  5 ,  5 ,  0 ,  5 ,  5 ,  2 },  /* Grey Elf */
  { 5 ,  2 ,  5 ,  5 ,  1 ,  5 ,  2 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Mountain Dwarf */
  { 5 , -1 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Duergar Dwarf */
  { 5 ,  2 ,  1 ,  5 ,  5 ,  5 ,  2 ,  5 ,  0 ,  2 ,  2 ,  5 ,  2 ,  0 ,  5 ,  2 ,  1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  2 ,  5 ,  5 ,  0 ,  5 ,  5 ,  2 },  /* Halfling */
  { 5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  2 ,  5 ,  5 ,  2 ,  2 , -1 ,  2 ,  5 ,  5 ,  2 ,  1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  2 },  /* Gnome */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Ogre */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Troll */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Half-Elf */
  { 5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Illithid */
  { 5 , -1 ,  5 , -1 ,  5 , -1 , -1 , -1 ,  5 , -1 , -1 , -1 , -1 , -1 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 , -1 , -1 , -1 , -1 ,  5 ,  0 ,  5 ,  5 , -1 },  /* Orc */
  { 5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Thri-Kreen */
  { 5 ,  2 ,  1 ,  5 ,  1 ,  5 ,  5 ,  5 ,  0 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Centaur */
//      Wa   Ra   Ps   Pa   Ap   Cl   Mo   Dr   Sh   So   Ne   Co   Ro   Ass  Me   Ba   Th   Wl   Mf   Al   Be   Rv   Il   Bli  DL   Eth  Av   Tg   Sum =Class
  { 5 , -1 ,  5 , -1 ,  5 , -1 , -1 , -1 ,  5 ,  5 , -1 , -1 , -1 , -1 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 ,  0 ,  5 ,  5 , -1 },  /* Githyanki */
  { 5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  2 ,  2 ,  5 ,  5 ,  5 ,  5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Minotaur */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 , -1 ,  5 ,  5 , -1 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 , -1 },  /* Shade */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Revenant */
  { 5 , -1 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 , -1 , -1 , -1 , -1 , -1 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 ,  0 ,  5 ,  5 , -1 },  /* Goblin */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Lich */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 },  /* Vampire */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Death Knight */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Shadow Beast */
//      Wa   Ra   Ps   Pa   Ap   Cl   Mo   Dr   Sh   So   Ne   Co   Ro   Ass  Me   Ba   Th   Wl   Mf   Al   Be   Rv   Il   Bli  DL   Eth  Av   Tg   Sum =Class
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Storm Giant */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Wight */
  { 5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 },  /* Phantom */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Harpy */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Orog */
  { 5 ,  2 ,  1 ,  1 ,  1 ,  5 ,  2 ,  1 ,  0 ,  5 ,  2 , -1 ,  2 ,  5 ,  5 ,  5 ,  1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  2 ,  5 ,  5 ,  0 ,  5 ,  5 ,  2 },  /* Githzerai */
  { 5 , -1 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Drider */ 
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 , -1 , -1 , -1 , -1 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  0 ,  5 ,  5 , -1 },  /* Kobold */
  { 5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 },  /* Pillithid */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Kuo Toa */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Wood Elf */
  { 5 ,  2 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Firbolg */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  1 ,  5 ,  5 },  /* Agathinon */  
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  1 ,  5 },  /* Eladrin */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 },  /* Gargoyle */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  0 ,  5 ,  0 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 },  /* Fire Elemental */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  0 ,  5 ,  0 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Air Elemental */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  0 ,  5 ,  0 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Water Elemental */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  0 ,  5 ,  0 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Earth Elemental */
  { 5 , -1 ,  5 , -1 ,  5 , -1 , -1 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 },  /* Demon */
  { 5 , -1 ,  5 , -1 ,  5 , -1 , -1 ,  5 ,  5 ,  5 , -1 ,  5 , -1 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 },  /* Devil */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Undead  */
  { 5 , -1 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 , -1 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 },  /* Vampire */
  { 5 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 , -1 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Ghost */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Lycanthrope */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Giant */
  { 5 ,  0 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 , -1 , -1 , -1 , -1 ,  5 , -1 , -1 ,  0 , -1 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Half Orc */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Golem */
  { 5 ,  1 ,  1 ,  5 ,  5 ,  5 ,  1 ,  5 ,  1 ,  1 ,  1 ,  5 ,  1 ,  5 ,  5 ,  5 ,  1 ,  1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Faerie */
  { 5 ,  0 ,  5 , -1 ,  5 , -1 ,  0 ,  0 ,  5 ,  0 ,  0 , -1 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 },  /* Dragon */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Dragonkin */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  0 ,  0 ,  1 ,  5 ,  0 ,  5 ,  5 ,  5 ,  0 ,  0 ,  5 ,  0 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Reptile */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  1 ,  0 ,  0 ,  5 ,  5 ,  5 ,  0 ,  0 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  0 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Snake */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  1 ,  0 ,  0 ,  5 ,  5 ,  5 ,  0 ,  0 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Insect */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  1 ,  0 ,  0 , -1 ,  5 ,  5 ,  0 ,  0 ,  5 ,  0 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Arachnid */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  1 ,  5 ,  0 ,  5 ,  0 ,  5 ,  0 ,  0 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Fish */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  1 ,  5 ,  0 ,  5 ,  5 ,  5 ,  0 ,  0 ,  0 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Flying Animal */
  { 5 ,  0 ,  1 ,  5 ,  5 ,  5 ,  0 ,  5 ,  1 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Quadruped */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  0 ,  0 ,  1 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Primate */
  { 5 ,  0 ,  1 ,  5 ,  5 ,  5 ,  0 ,  0 ,  1 ,  0 ,  0 , -1 ,  0 ,  5 ,  0 ,  0 ,  0 ,  0 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Humanoid */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  1 ,  0 ,  0 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Animal */
  { 5 ,  0 ,  1 ,  5 ,  5 ,  5 ,  0 ,  5 ,  1 ,  1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Plant */
  { 5 ,  0 ,  1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  1 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Herbivore */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Carnivore */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  0 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Parasite */
  { 5 , -1 ,  5 , -1 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Beholder */
  { 5 , -1 ,  5 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 },  /* Dracolich */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Slime */
  { 5 ,  1 ,  1 ,  5 ,  1 ,  5 ,  1 ,  5 ,  1 ,  5 ,  1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  1 ,  5 ,  5 ,  5 ,  1 ,  1 ,  5 },  /* Angel */
  { 5 , -1 ,  5 ,  5 , -1 , -1 ,  0 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 , -1 , -1 , -1 , -1 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 },  /* Raksasha */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Construct */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  0 ,  5 ,  0 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Efreet */
  { 5 , -1 ,  5 ,  5 ,  5 , -1 , -1 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Snow Ogre */
  { 5 , -1 , -1 ,  5 , -1 , -1 ,  0 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 , -1 , -1 , -1 , -1 ,  5 ,  5 ,  5 , -1 , -1 , -1 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 },  /* Beholderkin */
  { 5 , -1 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Zombie */
  { 5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 , -1 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Spectre */
  { 5 , -1 ,  5 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Skeleton */
  { 5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Wraith */
  { 5 ,  5 ,  5 , -1 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Shadow */
  { 5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  1 ,  0 ,  0 ,  5 ,  5 ,  5 ,  0 ,  0 ,  5 ,  0 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Purple Worm */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Void Elemental */
  { 5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 ,  5 },  /* Ice Elemental */
};


/* arena 'pre-entry' room, start of arena, end of arena */
/* start to end should be nothing but arena (inclusive), pre-entry can be anywhere */

/* by the way, with the way the code is currently set up, arenas don't need to be
   'keyed' to their proper hometown - arenas can be anywhere */

extern const int hometown_arena[LAST_HOME + 1][3];
const int hometown_arena[LAST_HOME + 1][3] = {
  {0, 0, 0},                    /* godville */
  {132501, 1701, 1725},           /* tharn */
  {0, 0, 0},                    /* squidly */
  {0, 0, 0},                    /* drowton */
  {0, 0, 0},                    /* elfie */
  {0, 0, 0},                    /* dwarfie */
  {0, 0, 0},                    /* dark dwarfie */
  {0, 0, 0},                    /* halfy */
  {0, 0, 0},                    /* orangey */
  {0, 0, 0},                    /* fat-assy */
  {0, 0, 0},                    /* swampy */
  {0, 0, 0},                    /* kenny! */
  {0, 0, 0},                    /* genericsville */
  {0, 0, 0},                    /* orc center */
  {0, 0, 0},                    /* genericsville #2 */
  {0, 0, 0}                     /* dungy */
};

const int hometown[] = {
  1200,                         /* * Gods */
  132500,                       /* * Tharnadia */
  96400,                        /* * Ixarkon */
  36329,                        /* * Arachdrathos */
  8001,                         /* Sylvandawn */
  95556,                        /* Kimordril */
  17001,                        /* * Khildarak */
  16558,                        /* Woodseer */
  66001,                        /* Ashrumite */
  15296,                        /* * Faang */
  11703,                        /* * Ghore */
  39100,                        /* Ugta */
  55500,                        /* Bloodstone */
  97663,                        /* * Shady */
  37716,                        /* NaxVaran */
  5302,                         /* Fort Marigot */
  45006,                        /* Charing */
  66202,                        /* City Ruins (Undead) */
  67723,                        /* thri-kreen town */
  19428,                        /* gith fortress */
  70000,                        /* goblin town */
  31177,                        /* harpy town */
  29317,                        /* alivio */
  22806,                        /* plain of life */
  45906,                        /* orog encampment */
};

/*
 *      SAM 7-94, Insert a 1 in this table for each race that can
 *      select the particular city as its hometown.  For example,
 *      humans have the choice of Verzanan, Calimport, or Baulders Gate.
 *      Note though that only Verzanan is enable at this point for humans
 *      since the other hometowns are not finished.
 *
 *      As is standard practice here, the above comment is left for posterity's sake.
 *      It is a relic of a bygone era, but should be kept as a memorial. - Jexni 3/30/11
 *     
 *      Columns are ordered Human, Barbarian, Drow, Grey, Mountain, Duergar,
 *      Halfling, Gnome, Ogre, Troll, Half-elf, Illithid, Orc, Thrikreen,
 *      Centaur, Githyanki, Minotaur, Kobold, Drider, Wood Elf, Firbolg,
 *      Pillithid, Kuo Toa
 *
 * Human          (H) Barbarian      (B) Drow Elf       (D) Grey Elf       (E) Mountain Dwarf (M) Duergar Dwarf  (U)
 * Halfling       (L) Gnome          (G) Ogre           (R) Swamp Troll    (T) Half-Elf       (F) Orc            (O)
 * Thri-Kreen     (K) Centaur        (C) Githyanki      (J) Minotaur       (S) Kobold         (A) Drider         (2)
 * Wood Elf       (W) Firbolg        (P) Pillithid      (I) Kuo Toa        (Q)
 */

 /* first column is always all zeroes */

// Human, Barbarian, Drow, Grey, Mountain, Duergar, Halfling, Gnome, Ogre, Troll
// Half-elf, Illithid, Orc, Thri-kreen, Centaur, Githyanki, Minotaur, Shade,
// Revenant, Goblin, Plich, Vampire, Death Knight, Shadow Beast, Storm Giant,
// Wright, Phantom, Harpy, Orog, Githzerai

extern const int avail_hometowns[][LAST_RACE + 1];
const int avail_hometowns[][LAST_RACE + 1] = {
/* N  Hu Ba Dr Gr Mo Du Ha Gn Og Tr H2 Il Or Th Ce Gi Mi Ae Su Gb Li Va Dk Sb Sg Wg Ph Hr Oo Gt Dr Ko Pi Ku Wo Fi*/
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * None */
  {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},   /* * Tharnadia */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},   /* * Ixarkon */
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},   /* * Arachdrathos */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Sylvandawn */
  {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Kimordril */
  {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Khildarak */
  {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Woodseer */
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Ashrumite Village */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Faang */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Ghore  */
  {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},   /* * Ugta */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Bloodstone */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Shady */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* NaxVaran */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* Fort Marigot */
  {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* * Charing */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* City Ruins */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* thri town */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* gith town */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},   /* gob town */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* harpy town */
  {0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},   /* newbie */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   /* plane of life */
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0},  /* Orog Encampment */

};

/* N  Hu Ba Dr Gr Mo Du Ha Gn Og Tr H2 Il Or Th Ce Gi Mi Ae Su Gb Li Va DK SB*/

extern char guild_default_titles[ASC_NUM_RANKS][ASC_MAX_STR_RANK];
char guild_default_titles[ASC_NUM_RANKS][ASC_MAX_STR_RANK] =
{
  "Enemy of",
  "On parole,",
  "Member of",
  "Senior of",
  "Officer of",
  "Deputy of",
  "Leader of",
  "King of"
};

/*
 *      SAM 7-94
 *      Guilds in each city (virtual room number) for each class.
 *      The "--" entry is used as the default entering location
 *      in the case a character is not assigned a guild.  You should
 *      set that too the temple equivalent (rm 6074 of Tharanadia) in
 *      your city
 */
extern const int guild_locations[][CLASS_COUNT + 1];
const int guild_locations[][CLASS_COUNT + 1] = {
/* I hated the previous class labels with a passion, so lets do them better -Z */
/* CLASS_NONE,        CLASS_WARRIOR,     CLASS_RANGER,   CLASS_PSIONICIST,   CLASS_PALADIN
 * CLASS_ANTIPALADIN, CLASS_CLERIC,      CLASS_MONK,     CLASS_DRUID,        CLASS_SHAMAN
 * CLASS_SORCERER,    CLASS_NECROMANCER, CLASS_CONJURER, CLASS_ROGUE,       CLASS_ASSASSIN
 * CLASS_MERCENARY,   CLASS_BARD,        CLASS_THIEF,    CLASS_WARLOCK,     CLASS_MINDFLAYER
 * CLASS_ALCHEMIST,   CLASS_BERSERKER,   CLASS_REAVER,   CLASS_ILLUSIONIST, CLASS_UNUSED
 * CLASS_DREADLORD,   CLASS_ETHERMANCER, CLASS_AVENGER   CLASS_THEURGIST,    CLASS_SUMMONER
 */
  /* * None */
  {-1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1},

  /* * Tharn */
  {132575, 132575, 132978, 132656, 132544,
   132575, 132575, 132575, 132575, 132575,
   133019, 132656, 133018, 132575, 132575,
   132575, 132575, 132575, 132575, 132575,
   132575, 132575, 132575, 132501, 132575,
   132575, 132575, 132575, 132935, 133018},

/*
  // Winterhaven
  {55160, 55155, 55159,    -1, 55155,
   55160, 55154, 55159, 55154, 55152,
   55153,    -1, 55153, 55157, 55157,
   55157, 55156, 55157,    -1,    -1,
   55160,    -1,    -1, 55158,    -1,
      -1, 55152, 55155, 55158},
*/
  /* * Ixarkon */
  {96524, 96537, 96537, 96537, 96537,
   96537, 96537, 96537, 96537, 96537,
   96503, 96503, 96503, 96503, 96503,
   96537, 96537, 96537, 96537, 96524,
   96537, 96537, 96537, 96537, 96537,
   96503, 96503, 96503, 96503, 96503},

  /* * Arachdrathos */
  {36329, 36329, 36329, 36329, 36329,
   36329, 36329, 36329, 36329, 36329,
   36329, 36329, 36329, 36329, 36329,
   36329, 36329, 36329, 36329, 36329,
   36329, 36329, 36329, 36329, 36329,
   36329, 36329, 36329, 36329, 36329},

  /* * Sylvandawn */
  {8001, 8018, 8307, -1, -1,
     -1, 8072,   -1, 8139, -1,
   8115, 8314,   -1, 8201, -1,
   8320, 8320, 8201, -1, -1,
     -1,   -1,   -1, -1, -1,
    -1,    -1,   -1, -1, -1},

  /* * Kimordril */
  {95556, 95556, 95556, 95556, 95556,
   95556, 95556, 95556, 95556, 95556,
   95556, 95556, 95556, 95556, 95556,
   95556, 95556, 95556, 95556, 95556,
   95556, 95556, 95556, 95556, 95556,
   95556, 95556, 95556, 95556, 95556},

  /* * Khildarak */
  {17001, 17228, 17140, 17140, 17140,
   17140, 17555, 17140, 17140, 17140,
   17140, 17140, 17140, 17140, 17572,
   17081, 17140, 17140, 17140, 17140,
   17140, 17140, 17140, 17140, 17140,
   17140, 17140, 17140, 17140, 17140},

  /* * Woodseer */
  {16558, 16653, 16653, 16653, 16653,
   16653, 16653, 16653, 16653, 16676,
   16676, 16653, 16653, 16653, 16653,
   16653, 16653, 16653, 16653, 16653,
   16653, 16653, 16653, 16675, 16653,
   16653, 16653, 16653, 16653, 16653},

  /* * Ashrumite */
  {66001, 66107, 66103, 66103, 66103,
   66103, 66113, 66103, 66103, 66131,
   66103, 66103, 66103, 66106, 66103,
   66103, 66103, 66106, 66103, 66103,
   66103, 66103, 66103, 66084, 66103,
   66103, 66103, 66103, 66103, 66103},

  /* * Faang */
  {15263, 15295, 15295, 15295, 15295,
   15295, 15295, 15295, 15295, 15277,
   15295, 15295, 15295, 15295, 15295,
   15385, 15295, 15295, 15295, 15295,
   15295, 15295, 15295, 15295, 15295,
   15295, 15295, 15295, 15295, 15295},

  /* * Ghore */
  {11703, 11610, 11613, 11613, 11613,
   11613, 11613, 11613, 11613, 11638,
   11613, 11613, 11613, 11613, 11613,
   11626, 11613, 11613, 11613, 11613,
   11613, 11613, 11613, 11613, 11613,
   11613, 11613, 11613, 11613, 11613},

  /* * Ugta */
  {39100, 39283, 39318, 39318, 39318,
   39318, 39318, 39318, 39318, 39299,
   39318, 39318, 39318, 39318, 39318,
   39318, 39318, 39318, 39318, 39318,
   39318, 39318, 39318, 39318, 39318,
   39318, 39318, 39318, 39318, 39318},

  /* * Bloodstone */
  {74124, 74124, 74124, 74124, 74124,
   74124, 74124, 74124, 74124, 74124,
   74124, 74124, 74124, 74124, 74124,
   74124, 74124, 74124, 74124, 74124,
   74124, 74124, 74124, 74124, 74124,
   74124, 74124, 74124, 74124, 74124},

  /* * Shady */
  {97663, 97663, 97663, 97663, 97663,
   97663, 97663, 97663, 97663, 97663,
   97663, 97663, 97663, 97663, 97663,
   97663, 97663, 97663, 97663, 97663,
   97633, 97663, 97663, 97663, 97663,
   97663, 97633, 97663, 97663, 97663},

  /* * NaxVaran */
  {37716, 37776, 37776, 37776, 37776,
   37776, 37776, 37776, 37776, 37776,
   37944, 37776, 37776, 37776, 37776,
   37763, 37776, 37776, 37776, 37776,
   37776, 37776, 37776, 37776, 37776,
   37776, 37776, 37776, 37776, 37776},

  /* Fort Marigot */
  {5302, 5336, 5336, 5336, 5336,
   5336, 5336, 5336, 5374, 5343,
   5336, 5336, 5336, 5336, 5336,
   5336, 5385, 5336, 5336, 5336,
   5336, 5336, 5336, 5336, 5336,
   5336, 5336, 5336, 5336, 5336},

  /* * Charing */
  {45006, 45044, 45139, 45131, 45131,
   45131, 45030, 45131, 45024, 45131,
   45093, 45131, 45094, 45131, 45131,
   45131, 45148, 45131, 45131, 45131,
   45131, 45131, 45131, 45131, 45131,
   45131, 45131, 45131, 45131, 45094},

  /* City Ruins */
  {   -1, 66345,    -1, 66354,    -1,
   66345,    -1,    -1,    -1,    -1,
   66354, 66354, 66354, 66348, 66348,
   66345,    -1, 66348, 66354,    -1,
   66354,    -1, 66354, 66354, 66348,
   66345,    -1,    -1,   -1, 66354},

  /* thri town */
  {67745, 67745, 67745, 67745, 67745,
   67745, 67745, 67745, 67745, 67745,
   67745, 67745, 67745, 67745, 67745,
   67745, 67745, 67745, 67745, 67745,
   67745, 67745, 67745, 67745, 67745,
   67745, 67745, 67745, 67745, 67745},

  /* gith town */
  {19445, 19442, 19445, 19445, 19445,
   19439, 19450, 19445, 19445, 19445,
   19448, 19436, 19448, 19445, 19445,
   19445, 19478, 19445, 19445, 19445,
   19445, 19445, 19490, 19445, 19445,
   19445, 19445, 19445, 19445, 19448},

  /* goblin town */
  {70190, 70190, 70190, 70190, 70190,
   70190, 70177, 70190, 70190, 70277,
   70029, 70204, 70173, 70210, 70212,
   70194, 70186, 70210, 70190, 70190,
   70177, 70190, 70190, 70298, 70190,
   70190, 70190, 70190, 70190, 70173},

  /* harpy town */
  {31177, 31177, 31177, 31177, 31177,
   31177, 31177, 31177, 31177, 31177,
   31177, 31177, 31177, 31177, 31177,
   31177, 31177, 31177, 31177, 31177,
   31177, 31177, 31177, 31177, 31177,
   31177, 31177, 31177, 31177, 31177},

  /* newbie zone */
  {29201, 29201, 29201, 29201, 29201,
   29201, 29201, 29201, 29201, 29201,
   29201, 29201, 29201, 29201, 29201,
   29201, 29201, 29201, 29201, 29201,
   29201, 29201, 29201, 29201, 29201,
   29201, 29201, 29201, 29201, 29201},

  /* plane of life */
  {22800, 22800, 22800, 22800, 22800,
   22800, 22800, 22800, 22800, 22800,
   22800, 22800, 22800, 22800, 22800,
   22800, 22800, 22800, 22800, 22800,
   22800, 22800, 22800, 22800, 22800,
   22800, 22800, 22800, 22800, 22800},

   
  /* Orog Encampment */
  {45906, 45905,    -1,    -1,    -1,
      -1, 45906,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,
   45905,    -1,    -1,    -1,    -1,
      -1, 45905,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1},

};

/*
 * Note: If changing languages, do change those at structs.h as well.. If
 * adding any, change MAX_TONGUE or whatnotitwas too
 */

const char *language_names[] = {
  "common",                     /*1 */
  "dwarven",                    /*2 */
  "elven",                      /*3 */
  "halfling",                   /*4 */
  "gnome",                      /*5 */
  "barbarian",                  /*6 */
  "duergar",                    /*7 */
  "ogre",                       /*8 */
  "troll",                      /*9 */
  "drow",                       /*10 */
  "orc",                        /*11 */
  "thrikreen",                  /*12 */
  "centaur",                    /*13 */
  "githyanki",                  /*14 */
  "minotaur",                   /*15 */
  "shade",                      /*16 */
  "revenant",                   /*17 */
  "goblin",                     /*18 */
  "lich",                       /*19 */
  "vampire",                    /*20 */
  "deathknight",                /*21 */
  "shadowbeast",                /*22 */
  "stormgiant",                 /*23 */
  "wight",                      /*24 */
  "phantom",                    /*25 */
  "animal",                     /*26 */
  "magic",                      /*27 */
  "god",                        /*28 */
  "\n"
};

/*
 * over-100 values are the part below 100 + (int_of_char / the number of
 * hundreds .
 */

extern const int language_base[][TONGUE_GOD];
const int language_base[][TONGUE_GOD] = {

/* 0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16  17  18  19  20  21  22  23  24  252627           */
  {1, 100, 45, 65, 45, 45, 45, 1, 1, 1, 1, 25, 25, 35, 1, 25, 0, 0, 1, 1, 1, 1, 1, 35, 1, 1, 0, 0},     /* 1 HUMAN      */
  {6, 60, 45, 35, 45, 45, 100, 1, 35, 1, 1, 1, 25, 35, 1, 25, 0, 0, 1, 1, 1, 1, 1, 35, 1, 1, 0, 0},     /* 2 BARBARIAN  */
  {10, 1, 1, 65, 1, 1, 1, 35, 35, 35, 100, 60, 25, 1, 35, 25, 0, 0, 35, 1, 1, 1, 1, 1, 1, 1, 0, 0},     /* 3 DROW       */
  {3, 100, 45, 100, 35, 45, 45, 1, 1, 1, 65, 1, 25, 100, 1, 25, 0, 0, 1, 1, 1, 1, 1, 35, 1, 1, 0, 0},   /* 4 GREY       */
  {2, 60, 100, 45, 45, 45, 45, 65, 1, 1, 1, 1, 25, 35, 1, 25, 0, 0, 1, 1, 1, 1, 1, 35, 1, 1, 0, 0},     /* 5 MOUNTAIN   */
  {7, 1, 65, 1, 1, 1, 1, 100, 35, 35, 35, 60, 25, 1, 35, 25, 0, 0, 35, 1, 1, 1, 1, 1, 1, 1, 0, 0},      /* 6 DUERGAR    */
  {4, 60, 45, 45, 100, 65, 45, 1, 1, 1, 1, 1, 25, 35, 1, 25, 0, 0, 1, 1, 1, 1, 1, 35, 1, 1, 0, 0},      /* 7 HALFLING   */
  {5, 60, 45, 45, 65, 100, 45, 1, 1, 1, 1, 1, 25, 35, 1, 25, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},       /* 8 GNOME      */
  {8, 1, 1, 1, 1, 1, 35, 35, 100, 65, 35, 60, 25, 1, 35, 25, 0, 0, 35, 1, 1, 1, 1, 1, 1, 1, 0, 0},      /* 9 OGRE       */
  {9, 1, 1, 1, 1, 1, 1, 35, 65, 100, 35, 35, 25, 1, 35, 25, 0, 0, 35, 1, 1, 1, 1, 1, 1, 1, 0, 0},       /* 10 TROLL     */
  {3, 80, 45, 100, 45, 45, 45, 1, 1, 1, 1, 1, 25, 100, 1, 25, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},      /* 11 HALFELF   */
  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 0, 0},   /* 12 ILLITHID  */
  {11, 25, 1, 1, 1, 1, 1, 45, 45, 45, 45, 100, 25, 1, 25, 25, 0, 0, 65, 1, 1, 1, 1, 1, 1, 1, 0, 0},     /* 13 ORC       */
  {12, 50, 25, 25, 25, 25, 25, 25, 25, 25, 25, 50, 100, 1, 25, 100, 0, 0, 100, 1, 1, 1, 1, 1, 1, 1, 0, 0},      /* 14 THRIKREEN */
  {13, 55, 35, 100, 35, 35, 35, 1, 1, 1, 1, 1, 25, 100, 1, 25, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},     /* 15 CENTAUR   */
  {14, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 100, 1, 0, 0, 50, 1, 1, 1, 1, 1, 1, 1, 0, 0},     /* 16 GITHYANKI */
  {15, 50, 25, 25, 25, 25, 25, 25, 25, 25, 25, 50, 100, 25, 25, 100, 0, 0, 100, 1, 1, 1, 1, 1, 1, 1, 0, 0},     /* 17 MINOTAUR  */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 100, 0, 1, 35, 60, 35, 1, 35, 100, 35, 0, 0},        /* 18 SHADE   */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 100, 1, 35, 60, 35, 1, 35, 35, 100, 0, 0},        /* 19 REVENANT */
  {18, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 50, 1, 50, 50, 1, 0, 100, 1, 1, 1, 1, 1, 1, 1, 0, 0},   /* 20 GOBLIN  */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 100, 60, 35, 1, 35, 35, 35, 0, 0},  /* Lich  */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 60, 100, 35, 1, 35, 35, 35, 0, 0},  /* Vampire  */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 35, 60, 100, 1, 35, 35, 35, 0, 0},  /* Death Kight  */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 35, 60, 35, 100, 35, 35, 35, 0, 0}, /* Shadow Beast  */
  {1, 60, 45, 35, 45, 45, 45, 1, 1, 1, 1, 1, 25, 35, 1, 25, 0, 0, 1, 1, 1, 1, 1, 100, 1, 1, 0, 0},      /* storm giant  */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 35, 60, 35, 1, 35, 100, 35, 0, 0},  /* wight  */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 35, 60, 35, 1, 35, 35, 100, 0, 0},  /* phantom  */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 100, 100},     /* Harpy  */
  {11, 25, 1, 1, 1, 1, 1, 45, 100, 45, 45, 100, 25, 1, 25, 25, 0, 0, 65, 1, 1, 1, 1, 1, 1, 1, 0, 0},     /* 29 OROG       */
  {1, 100, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 75, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},     /* 30 GITHZERAI */
  {10, 1, 1, 65, 1, 1, 1, 35, 35, 35, 100, 60, 25, 1, 35, 25, 0, 0, 35, 1, 1, 1, 1, 1, 1, 1, 0, 0},     /* 31 Drider       */
  {18, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 50, 1, 50, 50, 1, 0, 100, 1, 1, 1, 1, 1, 1, 1, 0, 0},   /* 32 Kobold  */
  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 0, 0},   /* 33 P_ILLITHID  */
  {11, 25, 1, 1, 1, 1, 1, 45, 45, 45, 45, 100, 25, 1, 25, 25, 0, 0, 65, 1, 1, 1, 1, 1, 1, 1, 0, 0},     /* 34 Kuo Toa       */
  {3, 100, 45, 100, 35, 45, 45, 1, 1, 1, 65, 1, 25, 100, 1, 25, 0, 0, 1, 1, 1, 1, 1, 35, 1, 1, 0, 0},   /* 35 Wood elf       */
  {1, 60, 45, 35, 45, 45, 45, 1, 1, 1, 1, 1, 25, 35, 1, 25, 0, 0, 1, 1, 1, 1, 1, 100, 1, 1, 0, 0},      /* 36 Firbolg  */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 100, 0}      /* 31 Gargoyle  */

/* below is original languages, if you need to restore them, delete my additions above
and uncomment those below... */
// /*0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16 17 18           24*/
//  {1, 190, 201, 201, 201, 201, 201,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0, 0,0,0,0,0,0,0,0,0,0,0}, /* 1 HUMAN      */
//  {6, 201, 201, 201, 201, 201, 190,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0, 0,0,0,0,0,0,0,0,0,0,0}, /* 2 BARBARIAN  */
//  {10,  1,   1,   1,   1,   1,   1, 201, 201, 201, 190, 201,   1,   1,   1,   1,   1, 1,201,0,0,0,0,0,0,0,0}, /* 3 DROW       */
//  {3, 201, 201, 190, 201, 201, 201,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0, 0,0,0,0,0,0,0,0,0,0,0}, /* 4 GREY       */
//  {2, 201, 190, 201, 201, 201, 201,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0, 0,0,0,0,0,0,0,0,0,0,0}, /* 5 MOUNTAIN   */
//  {7,   1,   1,   1,   1,   1,   1, 190, 201, 201, 201, 201,   1,   1,   1,   1,   0, 0,0,201,0,0,0,0,0,0,0}, /* 6 DUERGAR    */
//  {4,  60,  60,  60, 190,  60,  60,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0, 0,0,0,0,0,0,0,0,0,0,0}, /* 7 HALFLING   */
//  {5, 201, 201, 201, 201, 190, 201,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0, 0,0,0,0,0,0,0,0,0,0,0}, /* 8 GNOME      */
//  {8,   1,   1,   1,   1,   1,   1, 201, 190, 201, 201, 201,   1,   1,   1,   1,   0, 0,50,0,0,0,0,0,0,0,0,0}, /* 9 OGRE       */
//  {9,   1,   1,   1,   1,   1,   1, 201, 201, 190, 201, 201,   1,   1,   1,   1,   0, 0,50,0,0,0,0,0,0,0,0,0}, /* 10 TROLL     */
//  {3,  60, 201, 190, 201, 201, 201,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0, 0,0,0,0,0,0,0,0,0,0,0}, /* 11 HALFELF   */
//  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,   0, 0,100,0,0,0,0,0,0,0,0,0}, /* 12 ILLITHID  */
//  {11,  1,   1,   1,   1,   1,   1,  50,  50,  50,  50, 190,   1,   1,   1,   1,   0, 0,100,0,0,0,0,0,0,0,0,0}, /* 13 ORC       */
//  {12,  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1, 190,   1,   1,   1,   0, 0,50,0,0,0,0,0,0,0,0,0}, /* 14 THRIKREEN */
//  {13, 60,   5,  80,   1,   1,   1,   1,   1,   1,   1,   1,   1, 190,   1,   1,   0, 0,0,0,0,0,0,0,0,0,0,0}, /* 15 CENTAUR   */
//  {14, 50,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1, 190,   1,   0, 0,50,0,0,0,0,0,0,0,0,0}, /* 16 GITHYANKI */
//  {15,201,   1,   1,   1,   1,  76,   1,   1,   1,   1,   1,   1,   1,   1, 190,   0, 0,50,0,0,0,0,0,0,0,0,0},  /* 17 MINOTAUR  */
//  {16,  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1, 100, 0,1,0,0,0,0,0,0,0,0,0}, /* 18 AQUAELF   */
//  {17,  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0, 0,50,0,0,0,0,0,0,0,0,0}, /* 19 SAHUAGIN  */
//  {18,  1,   1,   1,   1,   1,   1, 100, 100, 100, 100, 100,  50,   1,  50,  50,   1,50,100,0,0,0,0,0,0,0,0,0}, /* 20 GOBLIN  */
//  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,   0, 0,100,0,0,0,0,0,0,0,0,0}, /* Lich  */
//  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,   0, 0,100,0,0,0,0,0,0,0,0,0}, /* Vampire  */
//  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,   0, 0,100,0,0,0,0,0,0,0,0,0}, /* Death Kight  */
//  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,   0, 0,100,0,0,0,0,0,0,0,0,0}, /* Shadow Beast  */
//  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,   0, 0,100,0,0,0,0,100,0,0,0,0}, /* storm giant  */
//  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,   0, 0,100,0,0,0,0,0,100,0,0,0}, /* wight  */
//  {1, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,   0, 0,100,0,0,0,0,0,0,100,0,0} /* phantom  */
};

/*
 * min height, max height - weight is calced on the fly with real killer
 * formula
 */

/*
 * I hate ADND elves, let them be tolkienistic elves instead! (grey/drow)
 * (i.e. height taller than avg human, but weight at par with avg human -
 * modified in init_Height_Weight - elf weighs only 75% of what human of
 * same weight weighs + other mods) -Torm
 *
 * I dont care if you hate AD&D elves, I personally think Tolkienistic
 * elves suck...so NO they will be shorter..suffer -Cython
 *
 * Since you're both long gone, I'm going to go ahead and tell you both
 * to STFU, especially since we don't even use ht/wt for anything anymore!
 * - Jexni
 */

int racial_values[LAST_RACE + 1][2] = {
  {  0,   0},                   // none
  {150, 190},                   /* * human */
  {170, 210},                   /* * barbarian */
  {110, 120},                   /* * drow */
  {110, 120},                   /* * grey */
  { 90, 150},                   /* * mountain */
  { 90, 150},                   /* * duergar */
  { 70, 130},                   /* * halfling */
  { 75, 135},                   /* * gnome */
  {350, 450},                   /* * ogre */
  {170, 210},                   /* * troll */
  {140, 180},                   /* * half-elf */
  {140, 180},                   /* * illithid  */
  {156, 170},                   /* * orc */
  {100, 150},                   /* * thri-kreen */
  {225, 300},                   /* * centaur */
  {140, 180},                   /* * githyanki */
  {240, 280},                   /* * minotaur */
  { 75, 135},                    /* * shade */
  {220, 250},                   /* * Revenant */
  { 75, 135},                    /* * goblin */
  {150, 190},                   /* * lich */
  {150, 190},                   /* * vampire */
  {150, 190},                   /* * death knight */
  {150, 190},                   /* * shadow beast */
  {220, 270},                   /* * Storm Giant * */
  {220, 270},                   /* * Wight * */
  {110, 120},                   /* * Phantom */
  {110, 120},                   /* * Harpy */
  {205, 295},                   /* * Orog */
  {150, 190},                   /* * Githzerai */
  {110, 120},                   /* * Drider */
  { 75, 135},                    /* * Kobold */
  {140, 180},                   /* * PIllithid */
  { 50,  90},                     /* * Kuo Toa */
  {110, 120},                   /* * Wood Elf */
  {170, 210},                   /* * Firbolg */
  {150, 120},                   /* * Gargoyle */
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190},
  {150, 190}
};

const char *justice_obj_status[] = {
  "None",
  "Steal",
  "Corpse loot",
  "\n"
};

/* castle defines */

const char *castle_extras[] = {
  "None",
  "Arrow Slits",
  "Moat",
  "\n"
};

const long castle_extras_build_time[] = {
  -1,                           /* none */
  10,                           /* arrow slits */
  20,                           /* moat */
  -1
};

/* constants for generating short descs */
/* 100 max each so far */
extern char *appearance_descs[100];
extern char *shape_descs[100];
extern char *modifier_descs[100];
extern int num_appearances;
extern int num_shapes;
extern int num_modifiers;

const char *target_locs[] = {
  "Any",
  "Head",
  "Body",
  "Limbs",
  "Arms",
  "Legs",
  "\n"
};

/* Constants for carving up corpses */
extern const int numCarvables;
const int numCarvables = 9;
extern const char *carve_part_name[];
const char *carve_part_name[] = { "skull", "scalp", "face", "eyes", "ears",
  "tongue", "bowels", "arms", "legs"
};
extern const int carve_part_flag[];
const int carve_part_flag[] = { MISSING_SKULL, MISSING_SCALP, MISSING_FACE,
  MISSING_EYES, MISSING_EARS, MISSING_TONGUE,
  MISSING_BOWELS, MISSING_ARMS, MISSING_LEGS
};

/* first race is CH the other races are HATED races */

extern const int race_hatred_data[][MAX_HATRED];
const int race_hatred_data[][MAX_HATRED] = {
  {RACE_MOUNTAIN,RACE_ORC,RACE_DUERGAR,RACE_DROW,-1},
  {-1,-1,-1,-1,-1}                            /* * END */
};

extern const racewar_struct racewar_color[MAX_RACEWAR+2];
const racewar_struct racewar_color[MAX_RACEWAR+2] =
{
  {'W', "None"}, {'Y', "Good"}, {'R', "Evil"}, {'L', "Undead"}, {'M', "Neutral"}, {'C', "Unknown"}
};
