/*
 * ***************************************************************************
 * *  File: common.c                                         Part of Duris *
 * *  Usage: almost all of the tables and wordlists.
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include "defines.h"
#include "objmisc.h"
#include "spells.h"

#include <vector>
using namespace std;

const char *specdata[][MAX_SPEC] = {
  {"", "", "", ""},                                                                               //None
  {"&+BSwordsman", "&+yGuardian", "&+bSwa&+Bsh&+Wbuc&+Bkler&n", ""},                              //Warrior
  {"&+cBlademaster", "&+gHuntsman", "&+gMa&+yrsha&+gll&n", ""},                                   //Ranger
  {"&+rPyr&+Rokine&+rtic", "&+MEn&+mslav&+Mer", "&+bPsyche&+Lporter", ""},                        //Psionicist
  {"&+wCrusa&+Wder", "&+WCavalier", "", ""},                                                      //Paladin
  {"&+LDark Knight", "&+LDem&+ronic Ri&+Lder", "&+LVi&+ro&+Llat&+ror&n", ""},                     //Anti-Paladin
  {"&+YZealot&n", "&+WHealer&n", "&+cHoly&+Wman&n", ""},                                          //Cleric
  {"&+rRe&+Rd Dra&+rgon", "&+gElap&+Ghi&+gdist", "", ""},                                         //Monk
  {"&+gFo&+Gre&+gst Druid", "&+cStorm &+CDruid", "", ""},                                         // Druid
  {"&+rEl&+Rem&+Lenta&+Rli&n&+rst", "&+WSpir&+Citua&+Wlist", "&+yAni&+Ymal&n&+yist", ""},         // Shaman
  {"&+MWild&+mmage", "&+LWizard", "&+LShadow&+wmage", ""},                                        /* CLASS_SORCERER */
  {"&+mDia&+rbolis", "&+mNe&+Lcro&+mlyte", "&+LReap&+wer", ""},                                   /* CLASS_NECROMANCER */
  {"&+CAir Magus&n", "&+BWater Magus&n", "&+rFire Magus&n", "&+yEarth Magus&n"},                  // Conjurer
  {"&+rAssassin&n", "&+LThief&n","Not Used" , "&+LSh&+wa&+Ldow &+BArc&+bher&n"},                  /* CLASS_ROGUE */
  {"", "", "", ""},                                                                               /* CLASS_ASSASSIN was replaced by CLASS_ROGUE, SPEC_ASSASSIN */
  {"&+yBr&+Lig&+yand", "&+yBounty &+LHunter", "", ""},                                            /* Mercenary */
  {"&+rD&+mis&+gha&+crm&+yon&+bist", "&+RScoundrel", "&+YMin&n&+ystr&+Yel", ""},                  //Bard
  {"", "", "", ""},                                                                               /* Thief is an UNUSED CLASS */
  {"", "", "", ""},                                                                               //Warlock
  {"", "", "", ""},                                                                               //MindFlayer
  {"&+CBat&n&+ctle-For&+Cger&n", "&+LBla&+ccksm&+Lith&n", "", ""},                                //Alchemist
  {"&+rMa&+RUle&+rR", "&+RRa&+rGe&+Rlo&+rRd", "", ""},                                            //Berserker
  {"&+CI&+Wc&+Ce &+LR&+Le&+wa&+wv&+Le&+Lr", "&+rF&+Rl&+Ya&+Rm&+re &+LR&+Le&+wa&+wv&+Le&+Lr", "&+bSh&+Bo&+Wck &+LR&+Le&+wa&+wv&+Le&+Lr", "&+LEa&+yrt&+Lh R&+yea&+Lve&+yr"},      //Reaver
  {"&+BM&+Yag&+Bic&+Yia&+Bn&n", "&+LDark &+mDreamer&n", "", ""},                                  // Illusionist
  {"&+YSt&+yor&+Ym &+LBringer&n", "&+GSc&+gou&+Yrg&+Ge&n", "&+LRu&+win&+Ler&n", ""},              // Blighter
  {"&+LDeath&+rlord", "&+LShadow&+rlord", "", ""},                                                // Dreadlord
  {"&+CTem&+cpe&+Cst Ma&+cgu&+Cs&n", "&+WFro&+cst &+CMagus", "&+YSt&+ya&+Yr &+LMa&+wgu&+Ls&n", ""}, // Ethermancer
  {"&+YLight&+Wbringer", "&+WInq&+wuisi&+Wtor", "", ""},                                          // Avenger
  {"&+wMedium&n", "&+YT&+Re&+rmpl&+Ra&+Yr&n", "&+C&+WT&+ch&+Ca&+Wu&+Cm&+ca&+Ct&+Wu&+Cr&+cg&+Ce&n", ""}, // Theurgist
  {"&+cCon&+Ctrol&+Wler&n", "&+rM&+Re&+Yn&+Wtal&+Yi&+Rs&+rt&n", "&+gNat&+Gural&+yist&n", ""}       // Summoner
};
/*
 * mob race lookup table, used to assign a race to a mob when reading them
 * from the .mob file.  Need to update this table when adding new races.
 *
 * A note on the codes:  when I set them up, I tried to follow a pattern of
 * general/specific, like all 'player' races are "Px", all elementals are
 * "Ex", undead "Ux", humanoid "Hx", animals "Ax", etc.  Current this is not
 * used for anything specific, but it looked like a good idea, so try to
 * follow it if you add more races.  JAB
 */
extern const struct race_names race_names_table[LAST_RACE + 2];
const struct race_names race_names_table[LAST_RACE + 2] = {
  {"None", "None", "Unknown Race", "NO"},                   // 0
  {"Human", "Human", "&+CHuman&n", "PH"},
  {"Barbarian", "Barbarian", "&+BBarbarian&n", "PB"},
  {"Drow Elf", "DrowElf", "&+mDrow Elf&n", "PL"},
  {"Grey Elf", "GreyElf", "&+cGrey Elf&n", "PE"},
  {"Mountain Dwarf", "MountainDwarf", "&+YDwarf&n", "PM"},  // 5
  {"Duergar Dwarf", "DuergarDwarf", "&+rDuergar&n", "PD"},
  {"Halfling", "Halfling", "&+yHalfling&n", "PF"},
  {"Gnome", "Gnome", "&+RGnome&n", "PG"},
  {"Ogre", "Ogre", "&+bOgre&n", "PO"},
  {"Troll", "Troll", "&+gTroll&n", "PT"},                   // 10
  {"Half-Elf", "Half-Elf", "&+CHalf&+c-Elf&n", "P2"},
  {"Illithid", "Illithid", "&+MIllithid&n", "MF"},
  {"Orc", "Orc", "&+LOrc&n", "HO"},
  {"Thri-Kreen", "Thri-Kreen", "&+GThri-&+YKreen&n", "TK"},
  {"Centaur", "Centaur", "&+gCen&+Ltaur&n", "CT"},          // 15
  {"Githyanki", "Githyanki", "&+GGith&+Wyanki&n", "GI"},
  {"Minotaur", "Minotaur", "&+LMino&+rtaur&n", "MT"},
  {"Shade", "Shade", "&+LShade&n", "SH"},
  {"Revenant", "Revenant", "&+cRevenant&n", "RE"},
  {"Goblin", "Goblin", "&+GGoblin&n", "HG"},                // 20
  {"Lich", "Lich", "&+LL&+mic&+Lh&n", "UL"},
  {"Vampire", "Vampire", "&+RVam&+rpi&+Rre&n", "UM"},
  {"Death Knight", "DeathKnight", "&+LDeath &+bKnight&n", "UK"},
  {"Shadow Beast", "ShadowBeast", "&+LShadow &+rBeast&n", "US"},
  {"Storm Giant", "StormGiant", "&+wSt&+Wor&+wm G&+Wia&+wnt&n", "SG"},// 25
  {"Wight", "Wight", "&+RW&+ri&+Rg&+rh&+Rt&n", "UW"},
  {"Phantom", "Phantom", "&+WPha&+Lntom&n", "UP"},
  {"Harpy", "Harpy", "&+yHarpy&n", "MH"},
  {"Orog", "Orog", "&+LOr&+yo&+Lg&n", "OR"},
  {"Githzerai", "Githzerai", "&+WGith&+Gzerai&n", "GZ"},    // 30
  {"Drider", "Drider", "&+mDri&+Lder&n", "DR"},
  {"Kobold", "Kobold", "&+LKobold&n", "KB"},
  {"Planetbound Illithid", "Pillithid", "&+MIllithid&n", "PI"},
  {"Kuo Toa", "KuoToa", "&+GKu&+Lo T&+Goa&n", "KT"},
  {"Wood Elf", "WoodElf", "&+gW&+Goo&+gd E&+Glf&n", "WE"},  // 35
  {"Firbolg", "Firbolg", "&+yFir&+cbolg&n", "FB"},
  {"Agathinon", "Agathinon", "&+WAga&+Yt&+Whin&+Yo&+Wn&n", "AG"},
  {"Eladrin", "Eladrin", "&+cE&+Cl&+Wadr&+Ci&+cn&n", "EL"},
  {"Gargoyle", "Gargoyle", "&+LGar&+wgo&+Lyle&n", "MG"},
  {"Fire Elemental", "FireElemental", "&+rFire Elemental&n", "EF"}, // 40
  {"Air Elemental", "AirElemental", "&+CAir Elemental&n", "EA"},
  {"Water Elemental", "WaterElemental", "&+BWater Elemental&n", "EW"},
  {"Earth Elemental", "EarthElemental", "&+yEarth Elemental&n", "EE"},
  {"Demon", "Demon", "&+LD&+we&+Lm&+wo&+Ln&n", "X"},
  {"Devil", "Devil", "&+RD&+re&+Rv&+ri&+Rl&n", "Y"},        // 45
  {"Undead ", "Undead", "&+LUn&+wdead&n", "U"},
  {"Vampire", "Vampire", "&+rV&+Ra&+rm&+Rp&+ri&+Rr&+re&n", "UV"},
  {"Ghost", "Ghost", "&+WG&+wh&+Wo&+ws&+Wt&n", "UG"},
  {"Lycanthrope", "Lycanthrope", "&+LL&+yy&+Lc&+ya&+Ln&+yt&+Lh&+yr&+Lo&+yp&+Le&n", "L"},
  {"Giant", "Giant", "&+yGiant&n", "G"},                    // 50
  {"Half Orc", "HalfOrc", "&+wHalf &+LOrc&n", "H2"},
  {"Golem", "Golem", "&+yGolem&n", "OG"},
  {"Faerie", "Faerie", "&+MFa&+mer&+Mie&n", "HF"},
  {"Dragon", "Dragon", "&+LDragon&n", "D"},
  {"Dragonkin", "Dragonkin", "&+LDra&+wgon&+Lkin&n", "DK"}, // 55
  {"Reptile", "Reptile", "&+GR&+ge&+Gp&+gt&+Gi&+gl&+Ge&n", "R"},
  {"Snake", "Snake", "&+gS&+Gn&+ga&+Gk&+ge&n", "RS"},
  {"Insect", "Insect", "&+wI&+Ln&+ws&+Le&+wc&+Lt&n", "I"},
  {"Arachnid", "Arachnid", "&+wA&+Lr&+wa&+Lc&+wh&+Ln&+wi&+Ld&n", "AS"},
  {"Aquatic Animal", "AquaticAnimal", "&+bAquatic Animal&n", "F"}, // 60
  {"Winged Animal", "WingedAnimal", "&+WWinged Animal&n", "B"},
  {"Quadruped", "Quadruped", "&+yQuadruped&n", "AE"},
  {"Primate", "Primate", "&+yP&+Lr&+yi&+Lm&+ya&+Lt&+ye&n", "AA"},
  {"Humanoid", "Humanoid", "&+LH&+yu&+Lm&+ya&+Ln&+yo&+Li&+yd&n", "H"},
  {"Animal", "Animal", "&+yAnimal&n", "A"},                 // 65
  {"Plant", "Plant", "&+gPlant&n", "VT"},
  {"Herbivore", "Herbivore", "&+GH&+ge&+Gr&+gb&+Gi&+gv&+Go&+gr&+Ge&n", "AH"},
  {"Carnivore", "Carnivore", "&+rC&+Ra&+rr&+Rn&+ri&+Rv&+ro&+Rr&+re&n", "AC"},
  {"Parasite", "Parasite", "&+LP&+wa&+Lr&+wa&+Ls&+wi&+Lt&+we&n", "AP"},
  {"Beholder", "Beholder", "&+mBe&+Lho&+mld&+Ler&n", "BH"}, // 70
  {"Dracolich", "Dracolich", "&+LDr&+wa&+Lc&+wo&+Ll&+wi&+Lch&n", "UD"},
  {"Slime", "Slime", "&+GSlime&n", "SL"},
  {"Angel", "Angel", "&+WA&+Cng&+Wel&n", "AN"},
  {"Rakshasa", "Rakshasa", "&+rRaks&+Yhasa&n", "RA"},
  {"Construct", "Construct", "&+LC&+won&+Lstr&+wuc&+Lt&n", "CN"},// 75
  {"Efreet", "Efreet", "&+rEf&+Rre&n&+ret&n", "E"},
  {"Snow Ogre", "SnowOgre", "&+WSnow &n&+bOgre&n", "SO"},
  {"Beholderkin", "Beholderkin", "&+mBeholderkin&n", "BK"},
  {"Zombie", "Zombie", "&+GZo&+gmbie&n", "ZO"},
  {"Spectre", "Spectre", "&+wSpe&+Wctre&n", "SP"},          // 80
  {"Skeleton", "Skeleton", "&+wSkeleton&n", "SK"},
  {"Wraith", "Wraith", "&+LWr&+wai&+Lth&n", "WR"},
  {"Shadow", "Shadow", "&+LShadow&n", "SW"},
  {"Purple Worm", "PurpleWorm", "&+mPurple &+LWorm&n", "PW"},
  {"Void Elemental", "VoidElemental", "&+LVoi&+wd Elemen&+Ltal&n", "VE"}, // 85
  {"Ice Elemental", "IceElemental", "&+CIc&+We Ele&+cme&+Wnt&+Cal&n", "IE"},
  {"Phoenix", "Phoenix", "&+rP&+Rh&+Yo&+We&+Yn&+Ri&+rx&n", "PX"},
  {"Archon", "Archon", "&+WA&+Rrch&+Wo&+Rn&n", "AR"},
  {"Asura", "Asura", "&+YA&+Rs&+Yu&+Rra&n", "AU"},
  {"Titan", "Titan", "&+CT&+Wi&+Ct&+Wa&+Cn&n", "TT"},       // 90
  {"Avatar", "Avatar", "&+cAv&+Ca&+Wt&+Ca&+cr&n", "AV"},
  {"Ghaele", "Ghaele", "&+cGh&+Cae&+cle&n", "GH"},
  {"Bralani", "Bralani", "&+cBr&+ra&+cl&+ra&+cn&+ri&n", "BR"},
  {"Whiner", "Whiner", "&+MW&+Wh&+Min&+We&+Mr&n", "WH"},
  {"Incubus", "Incubus", "&+BIn&+bcub&+Bus&n", "IN"},       // 95
  {"Succubus", "Succubus", "&+LS&+ru&+Rcubb&+ru&+Ls&n", "SU"},
  {"Fire Giant", "FireGiant", "&+rFi&+Rre Gia&+rnt&n", "FG"},
  {"Frost Giant", "FrostGiant", "&+cFro&+Cst Gi&+cant&n", "IG"},
  {"Deva", "Deva", "&+wD&+Wev&+wa&n", "DV"},
  {0} /* 99 races! */
};

int race_size(int race)
{
  switch (race)
  {
    case RACE_NONE:
    case RACE_PARASITE:
	case RACE_SHADOW:
        case RACE_DEVA:
      return SIZE_NONE;
      break;
      
    case RACE_INSECT:
    case RACE_ARACHNID:
    case RACE_AQUATIC_ANIMAL:
    case RACE_FLYING_ANIMAL:
    case RACE_REPTILE:
    case RACE_SNAKE:
    case RACE_FAERIE:
      return SIZE_TINY;
      break;
      
    case RACE_HALFLING:
    case RACE_PRIMATE:
    case RACE_GOBLIN:
    case RACE_ANIMAL:
    case RACE_SHADE:
    case RACE_GNOME:
    case RACE_KOBOLD:
      return SIZE_SMALL;
      break;
      
    case RACE_HUMAN:
    case RACE_MOUNTAIN:
    case RACE_DROW:
    case RACE_GREY:
    case RACE_HALFELF:
    case RACE_ILLITHID:
    case RACE_ORC:
    case RACE_THRIKREEN:
    case RACE_GITHYANKI:
    case RACE_GITHZERAI:
    case RACE_UNDEAD:
    case RACE_VAMPIRE:
    case RACE_GHOST:
    case RACE_LYCANTH:
    case RACE_HALFORC:
    case RACE_HUMANOID:
    case RACE_HERBIVORE:
    case RACE_CARNIVORE:
    case RACE_LICH:
    case RACE_PVAMPIRE:
    case RACE_PDKNIGHT:
    case RACE_BARBARIAN:
    case RACE_BEHOLDERKIN:
    case RACE_ZOMBIE:
    case RACE_SPECTRE:
    case RACE_SKELETON:
    case RACE_WRAITH:
    case RACE_OROG:
    case RACE_DUERGAR:
    case RACE_AGATHINON:
    case RACE_ELADRIN:
    case RACE_PILLITHID:
    case RACE_WOODELF:
    case RACE_INCUBUS:
    case RACE_SUCCUBUS:
    case RACE_ARCHON:
    case RACE_ASURA:
    case RACE_BRALANI:
    case RACE_GHAELE:
    case RACE_KUOTOA:
    default:
      return SIZE_MEDIUM;
      break;
      
    case RACE_REVENANT:
    case RACE_TROLL:
    case RACE_QUADRUPED:
    case RACE_BEHOLDER:
    case RACE_CENTAUR:
    case RACE_PSBEAST:
    case RACE_DRAGONKIN:
    case RACE_SNOW_OGRE:
    case RACE_DRIDER:
    case RACE_PWORM:
      return SIZE_LARGE;
      break;
      
    case RACE_GOLEM:
    case RACE_F_ELEMENTAL:
    case RACE_A_ELEMENTAL:
    case RACE_W_ELEMENTAL:
    case RACE_E_ELEMENTAL:
    case RACE_V_ELEMENTAL:
    case RACE_I_ELEMENTAL:
    case RACE_OGRE:
    case RACE_MINOTAUR:
    case RACE_EFREET:
    case RACE_FIRBOLG:
      return SIZE_HUGE;
      break;
      
    case RACE_GIANT:
    case RACE_PLANT:
    case RACE_DEMON:
    case RACE_DEVIL:
    return SIZE_GIANT;
      break;
      
    case RACE_DRAGON:
    case RACE_CONSTRUCT:
    case RACE_TITAN:
    case RACE_AVATAR:
      return SIZE_GARGANTUAN;
      break;
  }
}

const char *weapons[ /*TOTALATTACK_TYPES */ ] =
{
  "Hit",
  "Bludgeon",
  "Pierce",
  "Slash",
  "Whip",
  "Claw",
  "Bite",
  "Sting",
  "Crush",
  "Pound",
  "Maul",
  "Thrash",
  "Blast",
  "Punch",
  "Stab",
  "\n"
};

const char *missileweapons[] = {
  "Normal Flight Arrow",
  "Composite Flight Arrow",
  "Normal Sheaf Arrow",
  "Composite Sheaf Arrow",
  "Hand Crossbow Quarrel",
  "Light Crossbow Quarrel",
  "Heavy Crossbow Quarrel",
  "Throwing Dagger",
  "Dart",
  "Throwing Hammer",
  "Throwing Hand Axe",
  "Sling Stone",
  "Javelin",
  "Spear",
  "Ballista Missile",
  "Catapult Missile",
  "\n"
};

extern const int rev_dir[NUM_EXITS];
const int rev_dir[NUM_EXITS] = {
  2,
  3,
  0,
  1,
  5,
  4,
  9,
  8,
  7,
  6
};

const char *dirs[] = {
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest",
  "southwest",
  "northeast",
  "southeast",
  "\n"
};

const char *dirs2[NUM_EXITS + 1] = {
  "the north",
  "the east",
  "the south",
  "the west",
  "above",
  "below",
  "the northwest",
  "the southwest",
  "the northeast",
  "the southeast",
  "\n"
};

const char *short_dirs[NUM_EXITS + 1] = {
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "nw",
  "sw",
  "ne",
  "se",
  "\n"
};


const char *where[] = {
  "<used as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<held as shield>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<primary weapon>     ",
  "<secondary weapon>   ",
  "<held>               ",
  "<worn on eyes>       ",
  "<worn on face>       ",
  "<worn in ear>        ",
  "<worn in ear>        ",
  "<worn as quiver>     ",
  "<worn as a badge>    ",
  "<third weapon>       ",
  "<fourth weapon>      ",
  "<worn on back>       ",
  "<worn on belt buckle>",
  "<attached to belt>   ",
  "<attached to belt>   ",
  "<worn on lower arms> ",
  "<worn on lower hands>",
  "<worn on lower wrist>",
  "<worn on lower wrist>",
  "<worn on horse body> ",
  "<worn on rear legs>  ",
  "<worn on tail>       ",
  "<worn on rear feet>  ",
  "<worn in nose>       ",
  "<worn on horns>      ",
  "<floating about head>",
  "<worn on spider body>"
};

const char *drinks[] = {
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "Coke",
  "water",
  "holy water",
  "miso soup",
  "minestrone",
  "dutch potato soup",
  "shark's fin soup",
  "bird's nest soup",
  "champagne",
  "Pepsi",
  "water",
  "sake",
  "&+gpoison&n",
  "unholy water",
  "&+Wpeppermint schnappes&n",
  "\n"
};

extern const int drink_aff[][3];
const int drink_aff[][3] = {
  {0, 0, 10},                   /* * Water    */
  {3, 2, 5},                    /* * beer     */
  {5, 2, 5},                    /* * wine     */
  {2, 2, 5},                    /* * ale      */
  {1, 2, 5},                    /* * ale      */
  {6, 1, 4},                    /* * Whiskey  */
  {0, 1, 8},                    /* * lemonade */
  {10, 0, 0},                   /* * firebr   */
  {3, 3, 3},                    /* * local    */
  {0, 4, -8},                   /* * juice    */
  {0, 3, 6},                    /* * milk     */
  {0, 1, 6},                    /* * tea      */
  {0, 1, 6},                    /* * coffee   */
  {0, 2, -1},                   /* * blood    */
  {0, 1, -2},                   /* * saltwater */
  {0, 1, 5},                    /* * coke      */
  {0, 0, 10},                   /* * new water */
  {0, 1, 10},
  {0, 1, 1},
  {0, 1, 1},
  {0, 1, 1},
  {0, 1, 1},
  {0, 1, 1},
  {2, 1, 1},
  {0, 1, 1},
  {0, 1, 1},
  {5, 1, 1},
  {0, 0, 0},                    /* poison       */
  {0, 0, 10},
  {6, 0, 2}
};

const char *color_liquid[] = {
  "clear",
  "&+ybrown&N",
  "clear",
  "&+ybrown&N",
  "&+Ldark&N",
  "&+Ygolden&N",
  "&+rred&N",
  "&+ggreen&N",
  "clear",
  "&+Glight green&N",
  "&+Wwhite&N",
  "&+ybrown&N",
  "&+Lblack&N",
  "&+rred&N",
  "clear",
  "&+Lblack&N",
  "clear",
  "clear",
  "&+ybrown&N",
  "&+ybrown&N",
  "&+Wwhite&N",
  "grey",
  "grey",
  "clear",
  "&+Lblack&N",
  "clear",
  "clear",
  "&+ggreen&n",
  "clear",
  "clear"
};

const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FIRE WEAPON",
  "MISSILE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQUID CONT",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "BOOK",
  "CORPSE",
  "TELEPORT",
  "TIMER",
  "VEHICLE",
  "SHIP",
  "SWITCH",
  "QUIVER",
  "PICK",
  "INSTRUMENT",
  "SPELLBOOK",
  "TOTEM",
  "STORAGE",
  "SCABBARD",
  "SHIELD",
  "BANDAGE",
  "SPAWNER",
  "\n"
};

flagDef  wear_bits[] = {
  {"TAKE", "Takeable", 1, 0},
  {"WEAR_FINGER", "Worn on finger", 1, 0},
  {"WEAR_NECK", "Worn on neck", 1, 0},
  {"WEAR_BODY", "Worn on body", 1, 0},
  {"WEAR_HEAD", "Worn on head", 1, 0},
  {"WEAR_LEGS", "Worn on legs", 1, 0},
  {"WEAR_FEET", "Worn on feet", 1, 0},
  {"WEAR_HANDS", "Worn on hands", 1, 0},
  {"WEAR_ARMS", "Worn on arms", 1, 0},
  {"WEAR_SHIELD", "Worn as shield", 1, 0},
  {"WEAR_ABOUT", "Worn about body", 1, 0},
  {"WEAR_WAIST", "Worn about waist", 1, 0},
  {"WEAR_WRIST", "Worn on wrist", 1, 0},
  {"WIELD", "Wieldable (weapon)", 1, 0},
  {"HOLD", "Held", 1, 0},
  {"THROW", "Thrown (not used)", 0, 0},
  {"LIGHT_SOURCE", "Light (obsolete)", 0, 0},
  {"WEAR_EYES", "Worn on eyes", 1, 0},
  {"WEAR_FACE", "Worn on face", 1, 0},
  {"WEAR_EARRING", "Worn as earring", 1, 0},
  {"WEAR_QUIVER", "Worn as quiver", 1, 0},
  {"WEAR_INSIGNIA", "Worn as badge", 1, 0},
  {"WEAR_BACK", "Worn on back", 1, 0},
  {"ATTACH_BELT", "Attachable to belt", 1, 0},
  {"HORSE_BODY", "Worn on horse's body", 1, 0},
  {"WEAR_TAIL", "Worn on tail", 1, 0},
  {"WEAR_NOSE", "Worn in nose", 1, 0},
  {"WEAR_HORN", "Worn on horns", 1, 0},
  {"WEAR_IOUN", "Worn as ioun stone", 1, 0},
  {"SPIDER_BODY", "Worn on spider's body", 1, 0},
  {0}
};

flagDef  extra_bits[] = {
  {"GLOW", "Glow", 1, 0},
  {"NOSHOW", "No show", 1, 0},
  {"BURIED", "Buried", 0, 0},
  {"NOSELL", "No sell", 1, 0},
  {"THROW2", "Thrown ranged", 1, 0},
  {"INVISIBLE", "Invisible", 1, 0},
  {"NOREPAIR", "Non repairable", 1, 0},
  {"NODROP", "No drop (cursed)", 1, 0},
  {"RETURNING", "Auto-returning", 1, 0},
  {"ALLOWED_RACES", "Listed races CAN use", 1, 0},
  {"ALLOWED_CLASSES", "Listed classes CAN use", 1, 0},
  {"PROCLIB", "Generic Special Proc", 0, 0},
  {"SECRET", "Secret", 1, 0},
  {"FLOAT", "Floats", 1, 0},
  {"NORESET", "Doesn't repop", 1, 0},
  {"NOLOCATE", "No locate", 1, 0},
  {"NOIDENTIFY", "No identify", 1, 0},
  {"NOSUMMON", "No summon", 1, 0},
  {"LIT", "Lit", 1, 0},
  {"TRANSIENT", "Transient", 1, 0},
  {"NOSLEEP", "No sleep", 1, 0},
  {"NOCHARM", "No charm", 1, 0},
  {"TWOHANDS", "Two-handed", 1, 0},
  {"NORENT", "No rent", 1, 0},
  {"THROW1", "Thrown close", 1, 0},
  {"HUM", "Hum", 1, 0},
  {"LEVITATES", "Levitates", 1, 0},
  {"IGNORE_ITEM", "Ignore item", 1, 0},
  {"ARTIFACT", "Artifact", 0, 0},
  {"WHOLEBODY", "Whole body", 1, 0},
  {"WHOLEHEAD", "Whole head", 1, 0},
  {"ENCRUSTED", "Was encrusted", 0, 0},
   {0}
};

flagDef  extra2_bits[] = {
  {"SILVER", "Made of silver", 1, 0},
  {"BLESS", "Blessed", 1, 0},
  {"SLAY_GOOD", "Slaying good", 1, 0},
  {"SLAY_EVIL", "Slaying evil", 1, 0},
  {"SLAY_UNDEAD", "Slaying undead", 1, 0},
  {"SLAY_LIVING", "Slaying living", 1, 0},
  {"MAGIC", "Magic", 1, 0},
  {"LINKABLE", "Linkable Object", 1, 0},
  {"NOPROC", "Ignore proc", 0, 0},
  {"NOTIMER", "Ignore timer proc", 0, 0},
  {"NOLOOT", "Not lootable", 1, 0},
  {"CRUMBLELOOT", "Crumble when looted", 1, 0},
  {"STOREITEM", "Bought from shop", 0, 0},
  {"SOULBIND", "Soul bound item", 0, 0},
  {"CRAFTED", "Crafted Item", 0, 0},
  {"QUESTITEM", "Quest Item", 0, 0},
  {"TRANSPARENT", "Transparent Item", 1, 0},
  {0}
};

flagDef  room_bits[] = {
  {"DARK", "Dark", 1, 0},
  {"LOCKER", "Locker", 0, 0},
  {"NO_MOB", "No mob", 1, 0},
  {"INDOORS", "Indoors", 1, 0},
  {"SILENT", "Silent", 1, 0},
  {"UNDERWATER", "Underwater", 1, 0},
  {"NO_RECALL", "No recall", 1, 0},
  {"NO_MAGIC", "No magic", 1, 0},
  {"TUNNEL", "Tunnel", 1, 0},
  {"PRIVATE", "Private", 0, 0},
  {"ARENA", "Arena", 1, 0},
  {"SAFE", "Safe", 0, 0},
  {"NO_PRECIP", "No precipitation", 1, 0},
  {"SINGLE_FILE", "Single file", 1, 0},
  {"JAIL", "Jail", 1, 0},
  {"NO_TELEPORT", "No teleport", 1, 0},
  {"PRIV_ZONE", "Private zone", 0, 0},
  {"HEAL", "Heal", 1, 0},
  {"NO_HEAL", "No heal", 1, 0},
  {"RENTABLE", "Rentable", 1, 0},
  {"DOCKABLE", "Dockable", 1, 0},
  {"MAGIC_DARK", "Magic darkness", 1, 0},
  {"MAGIC_LIGHT", "Magic light", 1, 0},
  {"NO_SUMMON", "No summon", 1, 0},
  {"GUILD_ROOM", "Guild room", 1, 0},
  {"TWILIGHT", "Twilight", 1, 0},
  {"NO_PSI", "No psionics", 1, 0},
  {"NO_GATE", "No gate/planeshift", 1, 0},
  {"HOUSE", "House", 0, 0},
  {"ATRIUM", "Atrium", 0, 0},
  {"BLOCKS_SIGHT", "Blocks farsee/scan", 1, 0},
  {"BFS_MARK", "BFS mark", 0, 0},
  {0}
};

const char *zone_bits[] = {
  "SILENT",
  "SAFE",
  "TOWN",
  "NEW_RESET",
  "MAP",
  "CLOSED",
  "\n"
};

const char *exit_bits[] = {
  "IS-DOOR",
  "CLOSED",
  "LOCKED",
  "RSCLOSED",
  "RSLOCKED",
  "PICKABLE",
  "SECRET",
  "BLOCKED",
  "PICKPROOF",
  "WALLED",
  "SPIKED",
  "ILLUSION",
  "WALL_BREAKABLE",

  "\n"
};

const char *sector_types[NUM_SECT_TYPES + 1] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water-Swim",
  "Water-NoSwim",
  "No-Ground",
  "Underwater",
  "Underwater-Ground",
  "Plane-of-Fire",
  "Ocean",
  "UD-Wild",
  "UD-City",
  "UD-Inside",
  "UD-Water-Swim",
  "UD-Water-NoSwim",
  "UD-No-Ground",
  "Plane-of-Air",
  "Plane-of-Water",
  "Plane-of-Earth",
  "Plane-of-Ethereal",
  "Plane-of-Astral",
  "Desert",
  "Tundra/Ice",
  "Swamp",
  "UD-Mountains",
  "UD-Slime",
  "UD-Low Ceilings",
  "UD-Liquid Mithril",
  "UD-Mushroom Forest",
  "Outer Castle Wall",
  "Castle Gate",
  "Castle",
  "Negative Plane",
  "Plane of Avernus",
  "Patrolled Road",
  "Snowy Forest",
  "Lava",
  "\n"
};

const char *equipment_types[] = {
  "Special",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waiste",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "Worn on eyes",
  "Worn on face",
  "Worn in right ear",
  "Worn in left ear",
  "Worn as quiver",
  "Worn on back",
  "Attach to belt",
  "Attach to belt",
  "Attach to belt",
  "Worn on lower arms",
  "Worn on lower hands",
  "Worn around lower right wrist",
  "Worn around lower left wrist",
  "Worn on horse body",
  "Worn on rear legs",
  "Worn on tail",
  "Worn on rear feet",
  "Worn in nose",
  "Worn on horns",
  "Worn above head",
  "Worn on spider body",
  "\n"
};

flagDef  affected1_bits[] = {
  {"BLIND", "Blind", 0, 0},
  {"INVIS", "Invisible", 1, 0},
  {"FARSEE", "Farsee", 1, 0},
  {"DET_INV", "Detect invis", 1, 0},
  {"HASTE", "Haste", 1, 0},
  {"SENSE_LIFE", "Sense life", 1, 0},
  {"MINOR_GLOBE", "Minor globe", 1, 0},
  {"STONE_SKIN", "Stoneskin", 1, 0},
  {"UD_VISION", "Underdark vis", 1, 0},
  {"ARMOR", "Armor", 1, 0},
  {"WRAITHFORM", "Wraithform", 0, 0},
  {"WATERBREATH", "Waterbreathing", 1, 0},
  {"KNOCKED_OUT", "Knocked out", 0, 0},
  {"PROT_EVIL", "Prot evil", 1, 0},
  {"BOUND", "Bound", 1, 0},
  {"SLOW_POISON", "Slow poison", 1, 0},
  {"PROT_GOOD", "Prot good", 1, 0},
  {"SLEEP", "Slept", 0, 0},
  {"SKILL_AWARE", "Skill aware", 0, 0},
  {"SNEAK", "Sneak", 1, 0},
  {"HIDE", "Hide", 1, 0},
  {"FEAR", "Fear", 0, 0},
  {"CHARM", "Charmed", 0, 0},
  {"MEDITATE", "Meditate", 0, 0},
  {"BARKSKIN", "Barkskin", 1, 0},
  {"INFRAVISION", "Infravision", 1, 0},
  {"LEVITATE", "Levitate", 1, 0},
  {"FLY", "Fly", 1, 0},
  {"AWARE", "Aware", 1, 0},
  {"PROT_FIRE", "Prot fire", 1, 0},
  {"CAMPING", "Camping", 0, 0},
  {"BIOFEEDBACK", "Biofeedback", 1, 0},
  {0}
};

flagDef  affected2_bits[] = {
  {"FIRESHIELD", "Fireshield", 1, 0},
  {"ULTRA", "Ultravision", 1, 0},
  {"DET_EVIL", "Detect evil", 1, 0},
  {"DET_GOOD", "Detect good", 1, 0},
  {"DET_MAGIC", "Detect magic", 1, 0},
  {"MAJOR_PHYS", "Major physical", 0, 0},
  {"PROT_COLD", "Prot cold", 1, 0},
  {"PROT_LIGHT", "Prot lightning", 1, 0},
  {"MINOR_PARA", "Minor paralyze", 0, 0},
  {"MAJOR_PARA", "Major paralyze", 0, 0},
  {"SLOWNESS", "Slowness", 1, 0},
  {"MAJOR_GLOBE", "Major globe", 1, 0},
  {"PROT_GAS", "Prot gas", 1, 0},
  {"PROT_ACID", "Prot acid", 1, 0},
  {"POISONED", "Poisoned", 0, 0},
  {"SOULSHIELD", "Soulshield", 1, 0},
  {"SILENCED", "Silenced", 0, 0},
  {"MINOR_INVIS", "Minor invis", 1, 0},
  {"VAMP_TOUCH", "Vampiric touch", 1, 0},
  {"STUNNED", "Stunned", 0, 0},
  {"EARTH_AURA", "Earth aura", 1, 0},
  {"WATER_AURA", "Water aura", 1, 0},
  {"FIRE_AURA", "Fire aura", 1, 0},
  {"AIR_AURA", "Air aura", 1, 0},
  {"HOLDING_BREATH", "Holding breath", 0, 0},
  {"MEMORIZING", "Memorizing", 0, 0},
  {"DROWNING", "Drowning", 0, 0},
  {"PASSDOOR", "Passdoor", 0, 0},
  {"FLURRY", "Flurry", 1, 0},
  {"CASTING", "Casting", 0, 0},
  {"SCRIBING", "Scribing", 0, 0},
  {"HUNTING", "Hunting", 0, 0},
  {0}
};


flagDef  affected3_bits[] = {
  {"TENSORS_DISC", "Tensor's disc", 1, 0},
  {"TRACKING", "Tracking", 0, 0},
  {"SINGING", "Singing", 0, 0},
  {"ECTO_FORM", "Ectoplasmic form", 1, 0},
  {"ABSORBING", "Absorbing", 0, 0},
  {"PROT_ANIMAL", "Protection from animals", 1, 0},
  {"SPIRIT_WARD", "Spirit ward", 1, 0},
  {"GR_SPIRIT_WARD", "Gr. spirit ward", 1, 0},
  {"NON_DETECTION", "Non-detectable", 1, 0},
  {"SILVER", "Made of silver", 0, 0},
  {"PLUSONE", "Plus one", 0, 0},
  {"PLUSTWO", "Plus two", 0, 0},
  {"PLUSTHREE", "Plus three", 0, 0},
  {"PLUSFOUR", "Plus four", 0, 0},
  {"PLUSFIVE", "Plus five", 0, 0},
  {"ENLARGED", "Enlarged", 1, 0},
  {"REDUCED", "Reduced", 1, 0},
  {"COVER", "Has cover", 1, 0},
  {"FOUR_ARMS", "Has four arms", 1, 0},
  {"INERT_BARRIER", "Inertial barrier", 1, 0},
  {"LIGHTNING_SHIELD", "Lightning Shield", 0, 0},
  {"COLDSHIELD", "Coldshield", 1, 0},
  {"CANNIBALIZE", "Cannibalize", 1, 0},
  {"SWIMMING", "Swimming", 0, 0},
  {"TOWER_IRON_WILL", "Tower of iron will", 1, 0},
  {"UNDERWATER", "Underwater", 0, 0},
  {"BLUR", "Blur", 1, 0},
  {"ENHANCED_HEALING", "Enhanced healing", 1, 0},
  {"ELEM_FORM", "Elemental form", 0, 0},
  {"PASS_WO_TRACE", "Pass w/o trace", 1, 0},
  {"PAL_AURA", "Paladin aura", 1, 0},
  {"FAMINE", "Famine", 0, 0},
  {0}
};


flagDef  affected4_bits[] = {
  {"LOOTER", "Looter", 0, 0},
  {"CARRY_PLAGUE", "Carries plague", 1, 0},
  {"SACKING", "Sacking", 0, 0},
  {"SENSE_FOLLOWER", "Sense followers", 1, 0},
  {"STOR_SPHERES", "Stornog's spheres", 1, 0},
  {"STOR_GR_SPHERES", "Stornog's gr. sph", 1, 0},
  {"VAMP_FORM", "Vampiric form", 0, 0},
  {"NO_UNMORPH", "Can't unmorph", 1, 0},
  {"HOLY_SACR", "Holy sacrifice", 1, 0},
  {"BATTLE_ECS", "Battle ecstasy", 1, 0},
  {"DAZZLER", "Dazzler", 1, 0},
  {"PHANTASMAL_FORM", "Phantasmal form", 1, 0},
  {"NOFEAR", "Immune to fear", 1, 0},
  {"REGENERATION", "Regenerates in combat", 1, 0},
  {"DEAF", "Deaf", 1, 0},
  {"BATTLETIDE", "Battletide", 0, 0},
  {"EPIC_INCREASE", "Enhanced epic payout", 1, 0},
  {"MAGE_FLAME", "Mage flame", 1, 0},
  {"GLOBE_DARK", "Globe of darkness", 1, 0},
  {"DEFLECT", "Deflection", 1, 0},
  {"HAWKVISION", "Hawkvision", 1, 0},
  {"MULTICLASS", "Multiclassed NPC", 0, 0},
  {"SANCTUARY", "Sanctuary", 1, 0},
  {"HELLFIRE", "Hellfire", 1, 0},
  {"SENSE_HOLINESS", "Sense holiness", 1, 0},
  {"PROT_LIVING", "Prot living", 1, 0},
  {"DETECT_ILLUSION", "Detect illusion", 1, 0},
  {"ICE_AURA", "Ice aura", 1, 0},
  {"REV_POLARITY", "Reverse polarity", 1, 0},
  {"NEG_SHIELD", "Negative shield", 1, 0},
  {"TUPOR", "Tupor", 0, 0},
  {"WILDMAGIC", "Wild magic", 0, 0},
  {0}
};

flagDef  affected5_bits[] = {
  {"DAZZLEE", "Dazzled", 1, 0},
  {"MENTAL_ANGUISH", "Mental anguish", 0, 0},
  {"MEMORY_BLOCK", "Memory block", 1, 0},
  {"VINES", "Vines", 1, 0},
  {"ETHEREAL_ALLIANCE", "Ethereal alliance", 0, 0},
  {"BLOOD_SCENT", "Blood scent", 0, 0},
  {"FLESH_ARMOR", "Flesh armor", 1, 0},
  {"WET", "Wet", 0, 0},
  {"DHARMA", "Dharma", 1, 0},
  {"ENH_HIDE", "Enhanced hide", 0, 0},
  {"LISTEN", "Listen", 0, 0},
  {"PROT_UNDEAD", "Prot undead", 1, 0},
  {"IMPRISON", "Imprisoned", 1, 0},
  {"TITAN FORM", "Titan form", 1, 0},
  {"DELIRIUM", "Delirium", 1, 0},
  {"SHADE_MOVEMENT", "Shade movement", 1, 0},
  {"NO BLIND", "No blind", 0, 0},
  {"MAGICAL GLOW", "Magical glow", 0, 0},
  {"REFRESHING GLOW", "Refreshing glow", 0, 0},
  {"MINERS SIGHT", "Miners sight", 1, 0},
  {"STANCE OFFENSIVE", "Stance offensive", 0, 0},
  {"STANCE DEFENSIVE", "Stance defensive", 0, 0},
  {"OBSCURING MIST", "Obscuring mist", 1, 0},
  {"NOT OFFENSIVE", "Non-offensive", 0, 0},
  {"DECAYING_FLESH", "Decaying Flesh", 1, 0},
  {"DREADNAUGHT", "Dreadnaught", 1, 0},
  {"FOREST_SIGHT", "Forest Sight", 1, 0},
  {"THORNSKIN", "Thornskin", 1, 0},
  {"FOLLOWING", "Following", 1, 0},
  {0}
};

const char *apply_types[] = {
  "NONE",                       /* * 0 */
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",                        /* * 5 */
  "SEX",
  "CLASS",
  "LEVEL",
  "AGE",
  "WEIGHT",                     /* * 10 */
  "HEIGHT",
  "MANA",
  "HITPOINTS",
  "MOVE",
  "GOLD",                       /* * 15 */
  "EXP",
  "AC",
  "HITROLL",
  "DAMROLL",
  "SV_PARA",                    /* * 20 */
  "SV_ROD",
  "SV_FEAR",
  "SV_BREATH",
  "SV_SPELL",
  "FIRE_PROT",                  /* * 25 */
  "AGI",
  "POW",
  "CHA",
  "KARMA",
  "LUCK",                       /* * 30 */
  "STR_MAX",
  "DEX_MAX",
  "INT_MAX",
  "WIS_MAX",
  "CON_MAX",                    /* * 35 */
  "AGI_MAX",
  "POW_MAX",
  "CHA_MAX",
  "KARMA_MAX",
  "LUCK_MAX",                   /* * 40 */
  "STR_RACE",
  "DEX_RACE",
  "INT_RACE",
  "WIS_RACE",
  "CON_RACE",                   /* * 45 */
  "AGI_RACE",
  "POW_RACE",
  "CHA_RACE",
  "KARMA_RACE",
  "LUCK_RACE",                  /* * 50 */
  "CURSE",
  "SKILL_GRANT",
  "SKILL_ADD",
  "HIT_REG",
  "MOVE_REG",
  "MANA_REG",
  "SPELL_PULSE",
  "COMBAT_PULSE",
  "\n"
};

const char *apply_names[] = {
  "none",                       /* * 0 */
  "strength",
  "dexterity",
  "intelligence",
  "wisdom",
  "constitution",                        /* * 5 */
  "sex",
  "class",
  "level",
  "age",
  "weight",                     /* * 10 */
  "height",
  "mana",
  "hitpoints",
  "move",
  "gold",                       /* * 15 */
  "experience",
  "armor class",
  "hit roll",
  "damage roll",
  "save versus paralysis",                    /* * 20 */
  "save versus rods",
  "save versus fear",
  "save versus breath",
  "save versus spells",
  "fire protection",                  /* * 25 */
  "agility",
  "power",
  "charisma",
  "karma",
  "luck",                       /* * 30 */
  "maximum strength",
  "maximum dexterity",
  "maximum intelligence",
  "maximum wisdom",
  "maximum constitution",                    /* * 35 */
  "maximum agility",
  "maximum power",
  "maximum charisma",
  "maximum karma",
  "maximum luck",                   /* * 40 */
  "racial strength",
  "racial dexterity",
  "racial intelligence",
  "racial wisdom",
  "racial constitution",                   /* * 45 */
  "racial agility",
  "racial power",
  "racial charisma",
  "racial karma",
  "racial luck",                  /* * 50 */
  "cursed",
  "skill grant",
  "skill add",
  "hit regeneration",
  "move regeneration",
  "mana regeneration",
  "spell pulse",
  "combat pulse",
  "\n"
};

extern const struct class_names class_names_table[];
const struct class_names class_names_table[] = {
  {"None", "Unknown Class", "---", '-'},
  {"Warrior", "&+BWarrior&n", "War", 'w'},
  {"Ranger", "&+GRanger&n", "Ran", 'r'},
  {"Psionicist", "&+bPsionicist&n", "Psi", 'p'},
  {"Paladin", "&+WPaladin&n", "Pal", 'l'},
  {"Anti-Paladin", "&+LAnti-Paladin&n", "A-P", 'i'},
  {"Cleric", "&+cCleric&n", "Cle", 'c'},
  {"Monk", "&+LM&Non&+Lk&n", "Mon", 'k'},
  {"Druid", "&+gDruid&n", "Dru", 'd'},
  {"Shaman", "&+CShaman&n", "Sha", 'h'},
  {"Sorcerer", "&+MSorcerer&n", "Sor", 's'},
  {"Necromancer", "&+mNecromancer&n", "Nec", 'n'},
  {"Conjurer", "&+YConjurer&n", "Con", 'j'},
  {"Rogue", "&+rRogue&n", "Rog", 't'},
  {"Assassin", "&+rAssassin&n", "Ass", 'a'},
  {"Mercenary", "&+yMercenary&n", "Mer", 'm'},
  {"Bard", "&+LB&N&+bar&N&+Ld&n", "Bar", 'b'},
  {"Thief", "&+LThief&n", "Thf", 't'},
  {"Warlock", "&+BWar&+Llock&n", "Wlk", 'o'},
  {"MindFlayer", "&+MMindFlayer&n", "Mfl", 'f'},
  {"Alchemist", "&+CAlc&+chem&+Cist&n", "Alc", 'q'},
  {"Berserker", "&+rBeR&+RSeR&n&+rKeR&n", "Ber", 'u'},
  {"Reaver", "&+LRe&+Wav&+Ler&n", "Rev", 'v'},
  {"Illusionist", "&+WIl&+Clu&+csi&+Con&+Wist&n", "Ilu", 'y'},
  {"Blighter", "&+LBl&+yi&+ggh&+yt&+Ler&n", "Bli", 'x'},
  {"Dreadlord", "&+LDread&+rlord&n", "Dre", 'e'},
  {"Ethermancer", "&+wEthermancer&n", "Eth", 'g'},
  {"Avenger", "&+WAvenger&n", "Ave", '2'},
  {"Theurgist", "&+cTh&+Ceur&+Wgist&n", "The", '1'},
  {"Summoner", "&+RSummoner&n", "Sum", '3'},
  { NULL, NULL, NULL, '\0' }
};

flagDef  action2_bits[] = {
  { "COMBAT_NEARBY", "Listens for combat nearby", 1, 0 },
  { "NO_LURE", "Not lurable", 1, 0 },
  { "REMEMBERS_GROUP", "Remembers whole group", 1, 0 },
  { "UNUSED2_4", "Unused", 1, 0 },
  { "UNUSED2_5", "Unused", 1, 0 },
  { "UNUSED2_6", "Unused", 1, 0 },
  { "UNUSED2_7", "Unused", 1, 0 },
  { "UNUSED2_8", "Unused", 1, 0 },
  { "UNUSED2_9", "Unused", 1, 0 },
  { "UNUSED2_10", "Unused", 1, 0 },
  { "UNUSED2_11", "Unused", 1, 0 },
  { "UNUSED2_12", "Unused", 1, 0 },
  { "UNUSED2_13", "Unused", 1, 0 },
  { "UNUSED2_14", "Unused", 1, 0 },
  { "UNUSED2_15", "Unused", 1, 0 },
  { "UNUSED2_16", "Unused", 1, 0 },
  { "UNUSED2_17", "Unused", 1, 0 },
  { "UNUSED2_18", "Unused", 1, 0 },
  { "UNUSED2_19", "Unused", 1, 0 },
  { "UNUSED2_20", "Unused", 1, 0 },
  { "UNUSED2_21", "Unused", 1, 0 },
  { "BACK_RANK", "Back rank", 0, 0 },
  { "UNUSED2_23", "Unused", 1, 0 },
  { "UNUSED2_24", "Unused", 1, 0 },
  { "UNUSED2_25", "Unused", 1, 0 },
  { "UNUSED2_26", "Unused", 1, 0 },
  { "UNUSED2_27", "Unused", 1, 0 },
  { "UNUSED2_28", "Unused", 1, 0 },
  { "WAIT", "Wait lag", 0, 0 },
  { "UNUSED2_30", "Unused", 1, 0 },
  { "UNUSED2_31", "Unused", 1, 0 },
  { "UNUSED2_32", "Unused", 1, 0 },
  {0}
};

flagDef  action_bits[] = {
  {"SPEC", "Special proc", 1, 0},
  {"SENTINEL", "Sentinel", 1, 0},
  {"SCAVENGER", "Scavenger", 1, 0},
  {"ISNPC", "Is NPC", 0, 1},
  {"NICE_THIEF", "Nice thief", 1, 0},
  {"BREATHES_FIRE", "Breathes fire", 1, 0},
  {"STAY_ZONE", "Stays in zone", 1, 0},
  {"WIMPY", "Wimpy", 1, 0},
  {"BREATHES_LIGHTNING", "Breathes lightning", 1, 0},
  {"BREATHES_FROST", "Breathes frost", 1, 0},
  {"BREATHES_ACID", "Breathes acid", 1, 0},
  {"MEMORY", "Memory", 1, 0},
  {"NO_PARALYZE", "Can't be paralyzed", 1, 0},
  {"NO_SUMMON", "Can't be summoned", 1, 0},
  {"NO_BASH", "Can't be bashed", 1, 0},
  {"TEACHER", "Teacher", 1, 0},
  {"IGNORE_MOB", "Ignore mob", 1, 0},
  {"CANFLY", "Can fly", 1, 0},
  {"CANSWIM", "Can swim", 1, 0},
  {"BREATHES_GAS", "Breathes gas", 1, 0},
  {"BREATHES_SHADOW", "Breathes shadows", 1, 0},
  {"BREATHES_BLIND", "Breathes blinding gas", 1, 0},
  {"GUILD_GOLEM", "Guild golem", 1, 0},
  {"SPEC_DIE", "Special death proc", 1, 0},
  {"ELITE", "Elite type mob", 1, 0},
  {"BREAK_CHARM", "Breaks charm", 1, 0},
  {"PROTECTOR", "Protector", 1, 0},
  {"MOUNT", "Mount", 1, 0},
  {"WILDMAGE", "Wildmagic", 1, 0},
  {"PATROL", "Map Patrol", 1, 0},
  {"HUNTER", "Hunter", 1, 0},
  {"SPEC_TEACHER", "Specialization teacher", 1, 0},
  {0}
};

flagDef  aggro_bits[] = {
  {"AGGR_ALL", "Aggr to all", 1, 0},
  {"AGGR_DAY_ONLY", ".. by day only", 1, 0},
  {"AGGR_NIGHT_ONLY", ".. by night only", 1, 0},
  {"AGGR_GOOD_ALIGN", ".. to good align", 1, 0},
  {"AGGR_NEUTRAL_ALIGN", ".. to neut align", 1, 0},
  {"AGGR_EVIL_ALIGN", ".. to evil align", 1, 0},
  {"AGGR_GOOD_RACE", ".. to good races", 1, 0},
  {"AGGR_EVIL_RACE", ".. to evil races", 1, 0},
  {"AGGR_UNDEAD_RACE", ".. to undead races", 1, 0},
  {"AGGR_OUTCASTS", ".. to outcasts", 1, 0},
  {"AGGR_FOLLOWERS", ".. to all followers", 1, 0},
  {"AGGR_UNDEAD", ".. to undead fol", 1, 0},
  {"AGGR_ELEMENTALS", ".. to elemental fol", 1, 0},
  {"AGGR_DRACOLICH", ".. to dracolich fol", 1, 0},
  {"AGGR_HUMAN", ".. to humans", 1, 0},
  {"AGGR_BARBARIAN", ".. to barbarians", 1, 0},
  {"AGGR_DROW_ELF", ".. to drow elves", 1, 0},
  {"AGGR_GREY_ELF", ".. to grey elves", 1, 0},
  {"AGGR_MOUNT_DWARF", ".. to mount dwarves", 1, 0},
  {"AGGR_DEURGAR", ".. to duergar", 1, 0},
  {"AGGR_HALFLING", ".. to halflings", 1, 0},
  {"AGGR_GNOME", ".. to gnomes", 1, 0},
  {"AGGR_OGRE", ".. to ogres", 1, 0},
  {"AGGR_TROLL", ".. to trolls", 1, 0},
  {"AGGR_HALF_ELF", ".. to half-elves", 1, 0},
  {"AGGR_ILLITHID", ".. to illithids", 1, 0},
  {"AGGR_ORC", ".. to orcs", 1, 0},
  {"AGGR_THRIKREEN", ".. to thrikreen", 1, 0},
  {"AGGR_CENTAUR", ".. to centaurs", 1, 0},
  {"AGGR_GITHYANKI", ".. to githyanki", 1, 0},
  {"AGGR_MINOTAUR", ".. to minotaurs", 1, 0},
  {"AGGR_GOBLIN", ".. to goblins", 1, 0},
  {0}
};

flagDef  aggro2_bits[] = {
  {"AGGR2_LICH", "Aggr to liches", 1, 0},
  {"AGGR2_PVAMPIRE", ".. to vampires", 1, 0},
  {"AGGR2_PDKNIGHT", ".. to death knights", 1, 0},
  {"AGGR2_PSBEAST", ".. to shadow beasts", 1, 0},
  {"AGGR2_WARRIOR", ".. to warriors", 1, 0},
  {"AGGR2_RANGER", ".. to rangers", 1, 0},
  {"AGGR2_PSIONICIST", ".. to psionicists", 1, 0},
  {"AGGR2_PALADIN", ".. to paladins", 1, 0},
  {"AGGR2_ANTIPALADIN", ".. to anti-paladins", 1, 0},
  {"AGGR2_CLERIC", ".. to clerics", 1, 0},
  {"AGGR2_MONK", ".. to monks", 1, 0},
  {"AGGR2_DRUID", ".. to druids", 1, 0},
  {"AGGR2_SHAMAN", ".. to shamans", 1, 0},
  {"AGGR2_SORCERER", ".. to sorcerers", 1, 0},
  {"AGGR2_NECROMANCER", ".. to necromancers", 1, 0},
  {"AGGR2_CONJURER", ".. to conjurers", 1, 0},
  {"AGGR2_ROGUE", ".. to rogues", 1, 0},
  {"AGGR2_ASSASSIN", ".. to assassins", 1, 0},
  {"AGGR2_MERCENARY", ".. to mercenaries", 1, 0},
  {"AGGR2_BARD", ".. to bards", 1, 0},
  {"AGGR2_THIEF", ".. to thieves", 1, 0},
  {"AGGR2_WARLOCK", ".. to warlocks", 1, 0},
  {"AGGR2_MINDFLAYER", ".. to mindflayers", 1, 0},
  {"AGGR2_MALES", ".. to males", 1, 0},
  {"AGGR2_FEMALES", ".. to females", 1, 0},
  {"AGGR2_SGIANT", ".. to storm giants", 1, 0},
  {"AGGR2_WIGHT", ".. to wights", 1, 0},
  {"AGGR2_PHANTOM", ".. to phantoms", 1, 0},
  {"AGGR2_SHADE", ".. to shades", 1, 0},
  {"AGGR2_REVENANT", ".. to revenants", 1, 0},
  {"AGGR2_GITHZERAI", ".. to githzerais", 1, 0},
  {"AGGR2_THEURGIST", ".. to theurgists", 1, 0}
};

flagDef aggro3_bits[] = {
  {"AGGR3_OROG", ".. to orogs", 1, 0},
  {"AGGR3_DRIDER", ".. to driders", 1, 0},
  {"AGGR3_KOBOLD", ".. to kobolds", 1, 0},
  {"AGGR3_KUOTOA", ".. to kuo toas", 1, 0},
  {"AGGR3_WOODELF", ".. to wood elves", 1, 0},
  {"AGGR3_FIRBOLG", ".. to firbolgs", 1, 0},
  {"AGGR3_AGATHINON", ".. to agathinons", 1, 0},
  {"AGGR3_ELADRIN", ".. to eladrins", 1, 0},
  {"AGGR3_PILLITHID", ".. to planetbound illithids", 1, 0},
  {"AGGR3_ALCHEMIST", ".. to alchemists", 1, 0},
  {"AGGR3_BERSeRKER", ".. to berserkers", 1, 0},
  {"AGGR3_REAVER", ".. to reavers", 1, 0},
  {"AGGR3_ILLUSIONIST", ".. to illusionists", 1, 0},
  {"AGGR3_ETHERMANCER", ".. to ethermancers", 1, 0},
  {"AGGR3_DREADLORD", ".. to dreadlords", 1, 0},
  {"AGGR3_AVENGER", ".. to avengers", 1, 0}
};

const char *item_material[] = {
  "UNDEFINED",
  "NONSUBSTANTIAL",
  "FLESH",
  "CLOTH",
  "BARK",
  "SOFTWOOD",
  "HARDWOOD",
  "SILICON",
  "CRYSTAL",
  "CERAMIC",
  "BONE",
  "STONE",
  "HIDE",
  "LEATHER",
  "CURED_LEATHER",
  "IRON",
  "STEEL",
  "BRASS",
  "MITHRIL",
  "ADAMANTIUM",
  "BRONZE",
  "COPPER",
  "SILVER",
  "ELECTRUM",
  "GOLD",
  "PLATINUM",
  "GEM",
  "DIAMOND",
  "PAPER",
  "PARCHMENT",
  "LEAVES",
  "RUBY",
  "EMERALD",
  "SAPPHIRE",
  "IVORY",
  "DRAGONSCALE",
  "OBSIDIAN",
  "GRANITE",
  "MARBLE",
  "LIMESTONE",
  "LIQUID",
  "BAMBOO",
  "REEDS",
  "HEMP",
  "GLASSTEEL",
  "EGGSHELL",
  "CHITINOUS",
  "REPTILESCALE",
  "GENERICFOOD",
  "RUBBER",
  "FEATHER",
  "WAX",
  "PEARL",
  "TIN",
  "\n"
};

// From 0 .. WEAPON_HIGHEST
flagDef weapon_types[WEAPON_HIGHEST+1] = {
  {"WEAPON_NONE",         "none",             1, WEAPON_NONE},
  {"WEAPON_AXE",          "axe",              1, WEAPON_AXE},
  {"WEAPON_DAGGER",       "dagger",           1, WEAPON_DAGGER},
  {"WEAPON_FLAIL",        "flail",            1, WEAPON_FLAIL},
  {"WEAPON_HAMMER",       "hammer",           1, WEAPON_HAMMER},
  {"WEAPON_LONGSWORD",    "longsword",        1, WEAPON_LONGSWORD},
  {"WEAPON_MACE",         "mace",             1, WEAPON_MACE},
  {"WEAPON_SPIKED_MACE",  "spiked mace",      1, WEAPON_SPIKED_MACE},
  {"WEAPON_POLEARM",      "polearm",          1, WEAPON_POLEARM},
  {"WEAPON_SHORTSWORD",   "shortsword",       1, WEAPON_SHORTSWORD},
  {"WEAPON_CLUB",         "club",             1, WEAPON_CLUB},
  {"WEAPON_SPIKED_CLUB",  "spiked club",      1, WEAPON_SPIKED_CLUB},
  {"WEAPON_STAFF",        "staff",            1, WEAPON_STAFF},
  {"WEAPON_2HANDSWORD",   "two-handed sword", 1, WEAPON_2HANDSWORD},
  {"WEAPON_WHIP",         "whip",             1, WEAPON_WHIP},
  {"WEAPON_SPEAR",        "spear",            1, WEAPON_SPEAR},
  {"WEAPON_LANCE",        "lance",            1, WEAPON_LANCE},
  {"WEAPON_SICKLE",       "sickle",           1, WEAPON_SICKLE},
  {"WEAPON_TRIDENT",      "trident",          1, WEAPON_TRIDENT},
  {"WEAPON_HORN",         "horn",             1, WEAPON_HORN},
  {"WEAPON_NUMCHUCKS",    "numchucks",        1, WEAPON_NUMCHUCKS}
};

flagDef  missile_types[] = {
  {"MISSILE_ARROW", "arrow", 1, MISSILE_ARROW},
  {"MISSILE_LIGHT_CBOW_QUARREL", "light cbow bolt", 1,
   MISSILE_LIGHT_CBOW_QUARREL},
  {"MISSILE_HEAVY_CBOW_QUARREL", "heavy cbow bolt", 1,
   MISSILE_HEAVY_CBOW_QUARREL},
  {"MISSILE_HAND_CBOW_QUARREL", "hand cbow bolt", 1,
   MISSILE_HAND_CBOW_QUARREL},
  {"MISSILE_SLING_BULLET", "sling bullet", 1, MISSILE_SLING_BULLET},
  {"MISSILE_DART", "dart", 1, MISSILE_DART},
  {0}
};


const char *craftsmanship_names[OBJCRAFT_HIGHEST + 1] = {
  "terribly made",
  "extremely poorly made",
  "very poorly made",
  "fairly poorly made",
  "of well below average quality",
  "of below average quality",
  "of slightly below average quality",
  "of average quality",
  "of slightly above average quality",
  "of above average quality",
  "of well above average quality",
  "excellently made",
  "made by a skilled artisian",
  "made by a very skilled artisian",
  "made by a master artisian",
  "of one-of-a-kind craftsmanship"
};

struct material_data materials[NUMB_MATERIALS] = {
// phys, fire, cold, light, gas, acid, neg, holy,  psi, spirit
 // P, F, C, L, G, A, N, H, P, S
  {"&=RLundefined&n",
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {"&+La non-substantial material&n",
   {1, 1, 1, 1, 1, 1, 1, 1, 2, 2}},
  {"&+rflesh&n",
   {3, 5, 1, 2, 0, 3, 0, 0, 0, 0}},
  {"cloth",
   {1, 5, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+ybark&n",
   {3, 5, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+ysoft wood&n",
   {3, 4, 0, 0, 0, 1, 0, 0, 0, 0}},
  {"&+yhard wood&n",
   {2, 3, 0, 0, 0, 1, 0, 0, 0, 0}},
  {"&+Wglass&n",
   {3, 0, 3, 0, 0, 0, 0, 0, 4, 0}},
  {"&+gcrystal&n",
   {0, 0, 2, 0, 0, 0, 0, 0, 0, 2}},
  {"&+yclay&n",
   {2, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {"&+Wbone&n",
   {2, 0, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+Lstone&n",
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {"&+yhide&n",
   {0, 0, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+yleather&n",
   {0, 0, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+ycured leather&n",
   {0, 0, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+ciron&n",
   {0, 0, 2, 0, 0, 2, 0, 0, 0, 0}},
//  P, F, C, L, G, A, N, H, P, S
  {"&+Csteel&n",
   {0, 0, 2, 0, 0, 1, 0, 0, 0, 0}},
  {"&+ybrass&n",
   {0, 2, 0, 0, 0, 3, 0, 0, 0, 0}},
  {"&+mmithril&n",
   {0, 0, 0, 2, 0, 0, 0, 0, 0, 0}},
  {"&+Madamantium&n",
   {0, 0, 0, 0, 0, 1, 1, 0, 0, 0}},
  {"&+ybronze&n",
   {0, 2, 0, 0, 0, 3, 0, 0, 0, 0}},
  {"&+ycopper&n",
   {0, 0, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+csilver&n",
   {2, 2, 0, 0, 0, 0, 0, 0, 0, 0}},
  {"&+Celectrum&n",
   {3, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {"&+Ygold&n",
   {2, 0, 0, 2, 0, 0, 0, 0, 0, 0}},
  {"&+Wplatinum&n",
   {3, 3, 0, 0, 0, 0, 0, 0, 0, 0}},
  {"&+ggem&n",
   {0, 0, 2, 0, 0, 0, 0, 0, 0, 4}},
  {"&+Wdiamond&n",
   {2, 0, 0, 0, 3, 0, 0, 0, 0, 0}},
  {"&+Wpaper&n",
   {3, 8, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+yparchment&n",
   {3, 8, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+gleaves&n",
   {3, 6, 0, 0, 0, 3, 0, 0, 0, 0}},
  {"&+rruby&n",
   {0, 0, 0, 1, 0, 0, 0, 0, 2, 0}},
  {"&+gemerald&n",
//  P, F, C, L, G, A, N, H, P, S
   {2, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {"&+bsapphire&n",
   {0, 0, 0, 0, 4, 0, 0, 0, 0, 0}},
  {"&+Wivory&n",
   {2, 0, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+gdragonscale&n",
   {0, 0, 0, 0, 0, 0, 4, 0, 0, 0}},
  {"&+Lobsidian&n",
   {0, 0, 3, 0, 0, 0, 0, 4, 0, 0}},
  {"&+Lgranite&n",
   {0, 0, 0, 0, 0, 1, 0, 0, 0, 0}},
  {"&+Wmarble&n",
   {2, 0, 0, 0, 0, 0, 0, 0, 0, 2}},
  {"&+glimestone&n",
   {3, 0, 0, 0, 0, 3, 0, 0, 0, 0}},
  {"&+cliquid&n",
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {"&+ybamboo&n",
   {2, 3, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+greeds&n",
   {3, 3, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+ghemp&n",
   {3, 4, 0, 0, 0, 3, 0, 0, 0, 0}},
  {"&+cglassteel&n",
   {0, 0, 0, 0, 3, 0, 3, 0, 3, 0}},
  {"&+Weggshell&n",
   {2, 0, 0, 0, 0, 4, 0, 0, 0, 0}},
  {"&+ychitin&n",
   {2, 2, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+greptile scale&n",
   {2, 0, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+Ygeneric food&n",
   {8, 8, 0, 0, 0, 8, 0, 0, 0, 0}},
//  P, F, C, L, G, A, N, H, P, S
   {"&+Lrubber&n",
   {0, 4, 0, 0, 0, 0, 6, 0, 0, 0}},
  {"&+Wfeather&n",
   {2, 3, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+Wwax&n",
   {9, 9, 0, 0, 0, 3, 0, 0, 0, 0}},
  {"&+Wpearl&n",
   {3, 5, 0, 0, 0, 2, 0, 0, 0, 0}},
  {"&+Ctin&n",
   {0, 0, 2, 0, 0, 2, 0, 0, 0, 0}}
};


/* constants for generating short descs */
/* 100 max each so far */
char    *appearance_descs[100];
char    *shape_descs[100];
char    *modifier_descs[100];
int      num_appearances;
int      num_shapes;
int      num_modifiers;
