/*****************************************************************************
 * file: defines.h,                                          part of Duris    *
 * Usage: This contains almost all of the flags used in the duris zone structs*
 *****************************************************************************/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

  WARNING to all!

  Changing ANYTHING in this file can change things in the entire mud.  If you
  change the structure of obj_data or char_data, it will most likely require a
  change to the player save files, and probably changes to many of the other
  source files.  If you change ANYTHING in here, make damn sure those changes
  are documented to ~sojourn/Changed.

                                           John Bashaw, Gond of Duris

 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#ifndef _DURIS_DEFINES_H_
#define _DURIS_DEFINES_H_

#ifdef _HPUX_SOURCE
#define srandom srand
#define random rand
#endif

#include <sys/types.h>
#include <limits.h>

#ifdef _LINUX_SOURCE
#include <linux/time.h>
#else
#include <time.h>
#endif

#if defined(_SUN4_SOURCE) || defined(__CYGWIN32__) ||\
    (defined(_POSIX_SOURCE) && !defined(_SVID_SOURCE)) ||\
    defined(_FREEBSD)
typedef unsigned long int ulong;
#endif
#if defined(_SUN4_SOURCE) ||\
    (defined(_POSIX_SOURCE) && !defined(_SVID_SOURCE))
typedef unsigned int uint;
#endif
/* rather than doing these over and over again, define once here and use the
   defines in the other defines.  JAB */

#define BIT_1            1U
#define BIT_2            2U
#define BIT_3            4U
#define BIT_4            8U
#define BIT_5           16U
#define BIT_6           32U
#define BIT_7           64U
#define BIT_8          128U
#define BIT_9          256U
#define BIT_10         512U
#define BIT_11        1024U
#define BIT_12        2048U
#define BIT_13        4096U
#define BIT_14        8192U
#define BIT_15       16384U
#define BIT_16       32768U
#define BIT_17       65536U
#define BIT_18      131072U
#define BIT_19      262144U
#define BIT_20      524288U
#define BIT_21     1048576U
#define BIT_22     2097152U
#define BIT_23     4194304U
#define BIT_24     8388608U
#define BIT_25    16777216U
#define BIT_26    33554432U
#define BIT_27    67108864U
#define BIT_28   134217728U
#define BIT_29   268435456U
#define BIT_30   536870912U
#define BIT_31  1073741824U
#define BIT_32  2147483648U
#define BIT_33  4294967296U
#define BIT_34  8589934592U
#define BIT_35 17179869184U

#define MAX_INT_SIGNED    2147483647
#define MIN_INT_SIGNED   -2147483647
#define MAX_INT_UNSIGNED  4294967295U

/* The following defs are for obj_data  */

/* note:  before reassigning any of these, especially the ones that you have no
   clue about, they are probably there for compatibility with something, if you
   get ambitious, find out which ones aren't used and kill them.  JAB */

/* object 'type' */
#define MAX_SKILLS       2000
#define MAX_AFFECT_TYPES MAX_SKILLS+200
#define MAX_SPEC         4

#define ITEM_LOWEST      1
#define ITEM_LIGHT       1      /* an illuminating object */
#define ITEM_SCROLL      2      /* spells to be 'recited' */
#define ITEM_WAND        3      /* contains spells to 'use' */
#define ITEM_STAFF       4      /* also contains spells to 'use' (area affect) */
#define ITEM_WEAPON      5      /* for hurting things */
#define ITEM_FIREWEAPON  6      /* weapon used to fire others (bows, slings...) */
#define ITEM_MISSILE     7      /* arrow, bolt, ballista missile... */
#define ITEM_TREASURE    8      /* is intrinsically valuable */
#define ITEM_ARMOR       9      /* a piece of equipment granting physical prot. */
#define ITEM_POTION     10      /* for the quaffing */
#define ITEM_WORN       11      /* non-armor clothing items */
#define ITEM_OTHER      12      /* miscellaneous things */
#define ITEM_TRASH      13      /* OTHER (less useful) miscellaneous things*/
#define ITEM_WALL       14      /* Wall of force, etc */
#define ITEM_CONTAINER  15      /* for containing other items */
#define ITEM_NOTE       16      /* for passing information */
#define ITEM_DRINKCON   17      /* for containing potables */
#define ITEM_KEY        18      /* for unlocking locks */
#define ITEM_FOOD       19      /* for eating */
#define ITEM_MONEY      20      /* a pile of cash */
#define ITEM_PEN        21      /* for scribbling NOTEs */
#define ITEM_BOAT       22      /* row, row, row me boat, gently down the stream */
#define ITEM_BOOK       23      /* well, maybe someday will be useful */
#define ITEM_CORPSE     24      /* internal use only, do NOT assign this type! */
#define ITEM_TELEPORT   25      /* item can be used to teleport */
#define ITEM_TIMER      26      /* used chiefly to load/activate traps */
#define ITEM_VEHICLE    27      /* Like ships, but can 'follow' mobs (carraige) */
#define ITEM_SHIP       28      /* item which represents a sea-going vessel */
#define ITEM_SWITCH     29      /* item is a trigger for a switch proc */
#define ITEM_QUIVER     30      /* container for MISSILEs only */
#define ITEM_PICK       31      /* lockpicks */
#define ITEM_INSTRUMENT 32      /* bard's instruments */
#define ITEM_SPELLBOOK  33      /* Genuine magetype spellbook */
#define ITEM_TOTEM      34      /* shaman totem */
#define ITEM_STORAGE    35      /* like a container, but saves itself */
#define ITEM_SCABBARD   36      /* weapon scabbard */
#define ITEM_SHIELD     37      /* dedicated type for shields */
#define ITEM_BANDAGE    38
#define ITEM_SPAWNER    39
#define ITEM_HERB       40
#define ITEM_PIPE       41
#define ITEM_LAST       41

 /* obj->material - moved to objmisc.h */

 /* obj->value[1] for armor type - moved to objmisc.h */

 /* obj->status - moved to objmisc.h */

/* Bitvector For 'wear_flags' */

#define ITEM_NONE               0
#define ITEM_TAKE           BIT_1
#define ITEM_WEAR_FINGER    BIT_2
#define ITEM_WEAR_NECK      BIT_3
#define ITEM_WEAR_BODY      BIT_4
#define ITEM_WEAR_HEAD      BIT_5
#define ITEM_WEAR_LEGS      BIT_6
#define ITEM_WEAR_FEET      BIT_7
#define ITEM_WEAR_HANDS     BIT_8
#define ITEM_WEAR_ARMS      BIT_9
#define ITEM_WEAR_SHIELD    BIT_10
#define ITEM_WEAR_ABOUT     BIT_11
#define ITEM_WEAR_WAIST    BIT_12
#define ITEM_WEAR_WRIST     BIT_13
#define ITEM_WIELD          BIT_14
#define ITEM_HOLD           BIT_15
#define ITEM_THROW          BIT_16
#define ITEM_LIGHT_SOURCE   BIT_17
#define ITEM_WEAR_EYES      BIT_18
#define ITEM_WEAR_FACE      BIT_19
#define ITEM_WEAR_EARRING   BIT_20
#define ITEM_WEAR_QUIVER    BIT_21
#define ITEM_GUILD_INSIGNIA BIT_22
#define ITEM_WEAR_BACK      BIT_23
#define ITEM_ATTACH_BELT    BIT_24
#define ITEM_HORSE_BODY     BIT_25
#define ITEM_WEAR_TAIL      BIT_26
#define ITEM_WEAR_NOSE      BIT_27
#define ITEM_WEAR_HORN      BIT_28
#define ITEM_WEAR_IOUN      BIT_29
#define ITEM_SPIDER_BODY    BIT_30

/* Bitvector for 'extra_flags' */

#define ITEM_GLOW          BIT_1
#define ITEM_NOSHOW        BIT_2        /* item is _never_ shown        */
#define ITEM_BURIED        BIT_3
#define ITEM_NOSELL        BIT_4        /* shopkeepers won't buy it     */
#define ITEM_CAN_THROW2    BIT_5
#define ITEM_INVISIBLE     BIT_6
#define ITEM_NOREPAIR      BIT_7
#define ITEM_NODROP        BIT_8
#define ITEM_RETURNING     BIT_9
#define ITEM_ALLOWED_RACES BIT_10
#define ITEM_ALLOWED_CLASSES BIT_11
#define ITEM_PROCLIB       BIT_12
#define ITEM_SECRET        BIT_13
#define ITEM_FLOAT         BIT_14
#define ITEM_NORESET       BIT_15
#define ITEM_NOLOCATE      BIT_16       /* Item cannot be located       */
#define ITEM_NOIDENTIFY    BIT_17       /* Item cannot be identified    */
#define ITEM_NOSUMMON      BIT_18       /* if worn cannot be summoned */
#define ITEM_LIT           BIT_19       /* Item has a light spell cast on it */
#define ITEM_TRANSIENT     BIT_20       /* Item which dissolves when dropped */
#define ITEM_NOSLEEP       BIT_21       /* If in inventory, cannot be slept */
#define ITEM_NOCHARM       BIT_22       /* If in inventory, cannot be charmed */
#define ITEM_TWOHANDS      BIT_23       /* item requires two hands to hold/wield */
#define ITEM_NORENT        BIT_24       /* item cannot be rented */
#define ITEM_CAN_THROW1    BIT_25
#define ITEM_HUM           BIT_26
#define ITEM_LEVITATES     BIT_27
#define ITEM_IGNORE        BIT_28
#define ITEM_ARTIFACT      BIT_29
#define ITEM_WHOLE_BODY    BIT_30
#define ITEM_WHOLE_HEAD    BIT_31
#define ITEM_ENCRUSTED     BIT_32

/* Bitvector for 'extra2_flags'
 * those flags can be granted by spells cast on items */

#define ITEM2_SILVER       BIT_1    /* Item harm AFF_SILVER         */
#define ITEM2_BLESS        BIT_2
#define ITEM2_SLAY_GOOD    BIT_3
#define ITEM2_SLAY_EVIL    BIT_4
#define ITEM2_SLAY_UNDEAD  BIT_5
#define ITEM2_SLAY_LIVING  BIT_6
#define ITEM2_MAGIC        BIT_7
#define ITEM2_LINKABLE     BIT_8   /* Makes an item linkable by a player */
#define ITEM2_NOPROC       BIT_9
#define ITEM2_NOTIMER      BIT_10
#define ITEM2_NOLOOT       BIT_11
#define ITEM2_CRUMBLELOOT  BIT_12
#define ITEM2_STOREITEM    BIT_13  /* Item Bought From a Shop */
#define ITEM2_SOULBIND     BIT_14  /* Item is Soulbound */
#define ITEM2_CRAFTED      BIT_15
#define ITEM2_QUESTITEM    BIT_16
#define ITEM2_TRANSPARENT  BIT_17  /* Item shows contents when looked at */

/* Bitvector for 'anti_flags' */
/*
#define ITEM_ALLOW_ALL         BIT_1
#define ITEM_ALLOW_WARRIOR     BIT_2
#define ITEM_ALLOW_RANGER      BIT_3
#define ITEM_ALLOW_PALADIN     BIT_4
#define ITEM_ALLOW_ANTIPALADIN BIT_5
#define ITEM_ALLOW_CLERIC      BIT_6
#define ITEM_ALLOW_MONK        BIT_7
#define ITEM_ALLOW_DRUID       BIT_8
#define ITEM_ALLOW_SHAMAN      BIT_9
#define ITEM_ALLOW_SORCERER    BIT_10
#define ITEM_ALLOW_NECROMANCER BIT_11
#define ITEM_ALLOW_CONJURER    BIT_12
#define ITEM_ALLOW_PSIONICIST  BIT_13
#define ITEM_ALLOW_THIEF       BIT_14
#define ITEM_ALLOW_ASSASSIN    BIT_15
#define ITEM_ALLOW_MERCENARY   BIT_16
#define ITEM_ALLOW_BARD        BIT_17
#define ITEM_ANTI_HUMAN        BIT_18
#define ITEM_ANTI_GREYELF      BIT_19
#define ITEM_ANTI_HALFELF      BIT_20
#define ITEM_ANTI_DWARF        BIT_21
#define ITEM_ANTI_HALFLING     BIT_22
#define ITEM_ANTI_GNOME        BIT_23
#define ITEM_ANTI_BARBARIAN    BIT_24
#define ITEM_ANTI_DUERGAR      BIT_25
#define ITEM_ANTI_DROWELF      BIT_26
#define ITEM_ANTI_TROLL        BIT_27
#define ITEM_ANTI_OGRE         BIT_28
#define ITEM_ANTI_ILLITHID     BIT_29
#define ITEM_ANTI_ORC          BIT_30

#define ITEM_ANTI2_THRIKREEN   BIT_1
#define ITEM_ANTI2_CENTAUR     BIT_2
#define ITEM_ANTI2_GITHYANKI   BIT_3
#define ITEM_ANTI2_MINOTAUR    BIT_4
#define ITEM_ANTI2_MALE        BIT_5
#define ITEM_ANTI2_FEMALE      BIT_6
#define ITEM_ANTI2_NEUTER      BIT_7
#define ITEM_ANTI2_AQUAELF     BIT_8
#define ITEM_ANTI2_SAHUAGIN    BIT_9
#define ITEM_ANTI2_GOBLIN      BIT_10
#define ITEM_ANTI2_LICH      BIT_11
#define ITEM_ANTI2_PVAMPIRE      BIT_12
#define ITEM_ANTI2_PSBEAST      BIT_13
#define ITEM_ALLOW2_WARLOCK      BIT_14
#define ITEM_ALLOW2_MINDFLAYER      BIT_15
#define ITEM_ANTI2_PDKNIGHT      BIT_16
#define ITEM_ANTI2_SGIANT       BIT_17
#define ITEM_ANTI2_WIGHT       BIT_18
#define ITEM_ANTI2_PHANTOM       BIT_19
#define ITEM_ALLOW2_ILLUSIONIST  BIT_20
#define ITEM_ALLOW2_BERSERKER    BIT_21
#define ITEM_ALLOW2_REAVER       BIT_22
#define ITEM_ALLOW2_ALCHEMIST    BIT_23
#define ITEM_ALLOW2_UNUSED       BIT_24
#define ITEM_ALLOW2_DREADLORD    BIT_25
#define ITEM_ALLOW2_ETHERMANCER    BIT_26
*/


/* obj size */
#define ITEM_SIZE_NONE       0
#define ITEM_SIZE_TINY       1
#define ITEM_SIZE_SMALL      2
#define ITEM_SIZE_MEDIUM     3
#define ITEM_SIZE_LARGE      4
#define ITEM_SIZE_HUGE       5
#define ITEM_SIZE_GIANT      6
#define ITEM_SIZE_GARGANTUAN 7
#define ITEM_SIZE_SM         8
#define ITEM_SIZE_ML         9
#define ITEM_SIZE_MAGICAL    10

/* Some different kind of liquids */
#define LIQ_WATER       0
#define LIQ_BEER        1
#define LIQ_WINE        2
#define LIQ_ALE         3
#define LIQ_DARKALE     4
#define LIQ_WHISKY      5
#define LIQ_LEMONADE    6
#define LIQ_FIREBRT     7
#define LIQ_LOCALSPC    8
#define LIQ_SLIME       9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_COKE       15
#define LIQ_LOTSAWATER 16
#define LIQ_HOLYWATER  17
#define LIQ_MISO       18
#define LIQ_MINESTRONE 19
#define LIQ_DUTCH      20
#define LIQ_SHARK      21
#define LIQ_BIRD       22
#define LIQ_CHAMPAGNE  23
#define LIQ_PEPSI      24
#define LIQ_LITWATER   25
#define LIQ_SAKE       26
#define LIQ_POISON     27
#define LIQ_UNHOLYWAT  28
#define LIQ_SCHNAPPES  29
#define LIQ_LAST_ONE LIQ_SCHNAPPES

/* for containers  - value[1] */

#define CONT_CLOSEABLE     BIT_1
#define CONT_HARDPICK      BIT_2 /* not used, for compatibility only */
#define CONT_CLOSED        BIT_3
#define CONT_LOCKED        BIT_4
#define CONT_PICKPROOF     BIT_5

/* for loc_p */
#define LOC_NOWHERE        BIT_1
#define LOC_ROOM           BIT_2
#define LOC_CARRIED        BIT_3
#define LOC_WORN           BIT_4
#define LOC_INSIDE         BIT_5

/* for 'str_mask' */

#define STRUNG_KEYS   BIT_1     /* M: name         O: name               */
#define STRUNG_DESC1  BIT_2     /* M: long_descr   O: description        */
#define STRUNG_DESC2  BIT_3     /* M: short_descr  O: short_description  */
#define STRUNG_DESC3  BIT_4     /* M: description  O: action_description */
#define STRUNG_EDESC  BIT_5     /* M: (n/a)        O: extra_description  */

#define NUMB_OBJ_VALS    8
#define NUMB_CHAR_VALS   8

/* The following defs are for room_data  */

/* Bitvector For 'room_flags' */

#define ROOM_DARK          BIT_1       /* Need a light to look around here    */
#define ROOM_LOCKER        BIT_2       /* locker - flag set on storage lockers */
#define ROOM_NO_MOB        BIT_3       /* Mobiles are not permitted into here */
#define ROOM_INDOORS       BIT_4       /* Room is considered to be 'indoors'  */
#define ROOM_SILENT        BIT_5
#define ROOM_UNDERWATER    BIT_6
#define ROOM_NO_RECALL     BIT_7
#define ROOM_NO_MAGIC      BIT_8       /* Casting magic is not permitted.        */
#define ROOM_TUNNEL        BIT_9
#define ROOM_PRIVATE       BIT_10      /* No more than two ppl can move in here  */
#define ROOM_ARENA         BIT_11
#define ROOM_SAFE          BIT_12      /* No steal, attacks permitted in room    */
#define ROOM_NO_PRECIP     BIT_13
#define ROOM_SINGLE_FILE   BIT_14
#define ROOM_JAIL          BIT_15
#define ROOM_NO_TELEPORT   BIT_16
#define ROOM_UNUSED        BIT_17      /* Currently unused     */
#define ROOM_HEAL          BIT_18      /* You regain stats twice as fast here    */
#define ROOM_NO_HEAL       BIT_19      /* Cannot regain hp/mv/ma within room     */
#define ROOM_INN           BIT_20      /* Players can rent here                  */
#define ROOM_DOCKABLE      BIT_21      /* SHIP can dock in this room             */
#define ROOM_MAGIC_DARK    BIT_22
#define ROOM_MAGIC_LIGHT   BIT_23
#define ROOM_NO_SUMMON     BIT_24      /* Cannot summon or be summoned to or from */
#define ROOM_GUILD         BIT_25      /* for player guild rooms */
#define ROOM_TWILIGHT      BIT_26
#define ROOM_NO_PSI        BIT_27      /* can psis cast in here? */
#define ROOM_NO_GATE       BIT_28      /* disallow gate/planeshift? */
#define ROOM_NO_TRACK      BIT_29      // Mobs do not track into / through this room.
#define ROOM_ATRIUM        BIT_30      /* (R) The door to a house      */
#define ROOM_BLOCKS_SIGHT  BIT_31     /* can't scan/farsee through it, for fog, etc */
#define ROOM_BFS_MARK      BIT_32      /* used internally for find_the_path code */

/* For 'dir_option' */

#define DIR_NORTH          0
#define DIR_EAST           1
#define DIR_SOUTH          2
#define DIR_WEST           3
#define DIR_UP             4
#define DIR_DOWN           5
#define DIR_NORTHWEST      6
#define DIR_SOUTHWEST      7
#define DIR_NORTHEAST      8
#define DIR_SOUTHEAST      9

#define NUM_EXITS     10

#define EX_ISDOOR      BIT_1
#define EX_CLOSED      BIT_2
#define EX_LOCKED      BIT_3
#define EX_RSCLOSED    BIT_4
#define EX_RSLOCKED    BIT_5
#define EX_PICKABLE    BIT_6    /* ignored, for backwards compatibility only */
#define EX_SECRET      BIT_7
#define EX_BLOCKED     BIT_8
#define EX_PICKPROOF   BIT_9
#define EX_WALLED      BIT_10  // modified by EX_ILLUSION and EX_BREAKABLE
#define EX_SPIKED      BIT_11
#define EX_ILLUSION    BIT_12
#define EX_BREAKABLE   BIT_13

/* For 'Sector types' */

#define SECT_INSIDE            0
#define SECT_CITY              1
#define SECT_FIELD             2
#define SECT_FOREST            3
#define SECT_HILLS             4
#define SECT_MOUNTAIN          5
#define SECT_WATER_SWIM        6
#define SECT_WATER_NOSWIM      7
#define SECT_NO_GROUND         8
#define SECT_UNDERWATER        9
#define SECT_UNDERWATER_GR    10
#define SECT_FIREPLANE        11
#define SECT_OCEAN            12
#define SECT_UNDRWLD_WILD     13
#define SECT_UNDRWLD_CITY     14
#define SECT_UNDRWLD_INSIDE   15
#define SECT_UNDRWLD_WATER    16
#define SECT_UNDRWLD_NOSWIM   17
#define SECT_UNDRWLD_NOGROUND 18
#define SECT_AIR_PLANE        19
#define SECT_WATER_PLANE      20
#define SECT_EARTH_PLANE      21
#define SECT_ETHEREAL         22
#define SECT_ASTRAL           23
#define SECT_DESERT           24
#define SECT_ARCTIC           25
#define SECT_SWAMP            26
#define SECT_UNDRWLD_MOUNTAIN 27
#define SECT_UNDRWLD_SLIME    28
#define SECT_UNDRWLD_LOWCEIL  29
#define SECT_UNDRWLD_LIQMITH  30
#define SECT_UNDRWLD_MUSHROOM 31
#define SECT_CASTLE_WALL      32
#define SECT_CASTLE_GATE      33
#define SECT_CASTLE           34
#define SECT_NEG_PLANE        35
#define SECT_PLANE_OF_AVERNUS 36
#define SECT_ROAD             37
#define SECT_SNOWY_FOREST     38
#define SECT_LAVA             39

#define NUM_SECT_TYPES        40

/* What the land contains in resources */
#define RESOURCE_NONE           BIT_1
#define RESOURCE_ORE            BIT_2
#define RESOURCE_GOLD           BIT_3
#define RESOURCE_GEM            BIT_4
#define RESOURCE_HERB           BIT_5
#define RESOURCE_MINERAL        BIT_6
#define RESOURCE_TIMBER         BIT_7
#define RESOURCE_FERTILE        BIT_8
#define RESOURCE_PEOPLE         BIT_9

#define MANA_ALL_ALIGNS      0
#define MANA_GOOD            1
#define MANA_NEUTRAL         2
#define MANA_EVIL            3

/* How much light is in the land ? */

#define SUN_DARK        0
#define SUN_RISE        1
#define SUN_LIGHT       2
#define SUN_SET         3

/* And how is the sky ? */

#define SKY_CLOUDLESS   0
#define SKY_CLOUDY      1
#define SKY_RAINING     2
#define SKY_LIGHTNING   3

/* ======================================================================== */

/* The following defs and structures are related to char_data   */

	/* For 'equipment' */
#define WEAR_NONE              -1
#define WEAR_LIGHT              0       /* should not be used any longer! */
#define WEAR_FINGER_R           1
#define WEAR_FINGER_L           2
#define WEAR_NECK_1             3
#define WEAR_NECK_2             4
#define WEAR_BODY               5
#define WEAR_HEAD               6
#define WEAR_LEGS               7
#define WEAR_FEET               8
#define WEAR_HANDS              9
#define WEAR_ARMS              10
#define WEAR_SHIELD            11
#define WEAR_ABOUT             12
#define WEAR_WAIST             13
#define WEAR_WRIST_R           14
#define WEAR_WRIST_L           15
#define PRIMARY_WEAPON         16
#define WIELD PRIMARY_WEAPON
#define SECONDARY_WEAPON       17
#define WIELD2 SECONDARY_WEAPON
#define HOLD                   18
#define WEAR_EYES              19
#define WEAR_FACE              20
#define WEAR_EARRING_R         21
#define WEAR_EARRING_L         22
#define WEAR_QUIVER            23
#define GUILD_INSIGNIA         24
#define THIRD_WEAPON           25
#define WIELD3 THIRD_WEAPON
#define FOURTH_WEAPON          26
#define WIELD4 FOURTH_WEAPON
#define WEAR_BACK              27
#define WEAR_ATTACH_BELT_1     28
#define WEAR_ATTACH_BELT_2     29
#define WEAR_ATTACH_BELT_3     30
#define WEAR_ARMS_2            31
#define WEAR_HANDS_2           32
#define WEAR_WRIST_LR          33
#define WEAR_WRIST_LL          34
#define WEAR_HORSE_BODY        35
#define WEAR_LEGS_REAR         36
#define WEAR_TAIL              37
#define WEAR_FEET_REAR         38
#define WEAR_NOSE              39
#define WEAR_HORN              40
#define WEAR_IOUN              41
#define WEAR_SPIDER_BODY       42

#define CUR_MAX_WEAR           42
#define MAX_WEAR               CUR_MAX_WEAR + 1    /* size of equipment[] array */

/* For 'char_player_data' */

#define TONGUE_USING           0

#define TONGUE_COMMON          1

#define TONGUE_DWARVEN         2 /* Good races */
#define TONGUE_ELVEN           3
#define TONGUE_HALFLING        4
#define TONGUE_GNOME           5
#define TONGUE_BARBARIAN       6

#define TONGUE_DUERGAR         7 /* Evil races */
#define TONGUE_OGRE            8
#define TONGUE_TROLL           9
#define TONGUE_DROW            10
#define TONGUE_ORC             11

#define TONGUE_THRIKREEN       12
#define TONGUE_CENTAUR         13
#define TONGUE_GITHYANKI       14
#define TONGUE_MINOTAUR        15
#define TONGUE_AQUATICELF      16
#define TONGUE_SAHUAGIN        17
#define TONGUE_GOBLIN          18
#define TONGUE_LICH            19
#define TONGUE_VAMPIRE         20
#define TONGUE_DEATHKNIGHT     21
#define TONGUE_SHADOWBEAST     22
#define TONGUE_STORMGIANT      23
#define TONGUE_WIGHT           24
#define TONGUE_PHANTOM         25
#define TONGUE_ANIMAL          26
#define TONGUE_MAGIC           27
#define TONGUE_GOD             28
#define TONGUE_LASTHEARD       28

/* predifined attributes */

struct attr_names_struct {
  const char *abrv;
  const char *name;
};

#define STR	1
#define DEX	2
#define AGI	3
#define CON	4
#define POW	5
#define INT	6
#define WIS	7
#define CHA	8
#define KARMA	9
#define LUCK	10
#define MAX_ATTRIBUTES LUCK

#define MIN_LEVEL_FOR_ATTRIBUTES 20

/* Predifined  conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2
#define POISON_TYPE  3
#define DISEASE_TYPE 4
#define ALL_CONDS    5    /* all that are updated each short update */
#define MAX_COND     5    /* used by the loops, don't count all_cond */

/* Bitvector for 'affected_by'.  Also, used for object bitvectors -DCL */
#define AFF_NONE                  0
#define AFF_BLIND             BIT_1
#define AFF_INVISIBLE         BIT_2
#define AFF_FARSEE            BIT_3
#define AFF_DETECT_INVISIBLE  BIT_4
#define AFF_HASTE             BIT_5
#define AFF_SENSE_LIFE        BIT_6
#define AFF_MINOR_GLOBE       BIT_7
#define AFF_STONE_SKIN        BIT_8
#define AFF_UD_VISION         BIT_9
#define AFF_ARMOR             BIT_10
#define AFF_WRAITHFORM        BIT_11
#define AFF_WATERBREATH       BIT_12
#define AFF_KNOCKED_OUT       BIT_13
#define AFF_PROTECT_EVIL      BIT_14
#define AFF_BOUND             BIT_15
#define AFF_SLOW_POISON       BIT_16
#define AFF_PROTECT_GOOD      BIT_17
#define AFF_SLEEP             BIT_18
#define AFF_SKILL_AWARE       BIT_19    /* for awareness skill --TAM 7-9-94 */
#define AFF_SNEAK             BIT_20
#define AFF_HIDE              BIT_21
#define AFF_FEAR              BIT_22
#define AFF_CHARM             BIT_23
#define AFF_MEDITATE          BIT_24
#define AFF_BARKSKIN          BIT_25
#define AFF_INFRAVISION       BIT_26
#define AFF_LEVITATE          BIT_27
#define AFF_FLY               BIT_28
#define AFF_AWARE             BIT_29
#define AFF_PROT_FIRE         BIT_30
#define AFF_CAMPING           BIT_31
#define AFF_BIOFEEDBACK       BIT_32
#define AFF_INFERNAL_FURY     BIT_33

/* affected_by2 */

#define AFF2_FIRESHIELD       BIT_1
#define AFF2_ULTRAVISION      BIT_2
#define AFF2_DETECT_EVIL      BIT_3
#define AFF2_DETECT_GOOD      BIT_4
#define AFF2_DETECT_MAGIC     BIT_5
#define AFF2_MAJOR_PHYSICAL   BIT_6
#define AFF2_PROT_COLD        BIT_7
#define AFF2_PROT_LIGHTNING   BIT_8
#define AFF2_MINOR_PARALYSIS  BIT_9
#define AFF2_MAJOR_PARALYSIS  BIT_10
#define AFF2_SLOW             BIT_11
#define AFF2_GLOBE            BIT_12
#define AFF2_PROT_GAS         BIT_13
#define AFF2_PROT_ACID        BIT_14
#define AFF2_POISONED         BIT_15
#define AFF2_SOULSHIELD       BIT_16
#define AFF2_SILENCED         BIT_17
#define AFF2_MINOR_INVIS      BIT_18
#define AFF2_VAMPIRIC_TOUCH   BIT_19
#define AFF2_STUNNED          BIT_20
#define AFF2_EARTH_AURA       BIT_21
#define AFF2_WATER_AURA       BIT_22
#define AFF2_FIRE_AURA        BIT_23
#define AFF2_AIR_AURA         BIT_24
#define AFF2_HOLDING_BREATH   BIT_25    /* Underwater */
#define AFF2_MEMORIZING       BIT_26
#define AFF2_IS_DROWNING      BIT_27    /* Underwater */
#define AFF2_PASSDOOR         BIT_28
#define AFF2_FLURRY           BIT_29
#define AFF2_CASTING          BIT_30
#define AFF2_SCRIBING         BIT_31
#define AFF2_HUNTER           BIT_32

/* affected_by 3 */

#define AFF3_TENSORS_DISC       BIT_1
#define AFF3_TRACKING           BIT_2
#define AFF3_SINGING            BIT_3
#define AFF3_ECTOPLASMIC_FORM   BIT_4
#define AFF3_ABSORBING          BIT_5
#define AFF3_PROT_ANIMAL        BIT_6
#define AFF3_SPIRIT_WARD        BIT_7
#define AFF3_GR_SPIRIT_WARD     BIT_8
#define AFF3_NON_DETECTION      BIT_9
#define AFF3_SILVER             BIT_10    /* Char needs silver to dam  */
#define AFF3_PLUSONE            BIT_11    /* Char needs +1 to damage   */
#define AFF3_PLUSTWO            BIT_12    /* Char needs +2 to damage   */
#define AFF3_PLUSTHREE          BIT_13    /* Char needs +3 to damage   */
#define AFF3_PLUSFOUR           BIT_14    /* Char needs +4 to damage   */
#define AFF3_PLUSFIVE           BIT_15    /* Char needs +5 to damage   */
#define AFF3_ENLARGE            BIT_16
#define AFF3_REDUCE             BIT_17
#define AFF3_COVER              BIT_18
#define AFF3_FOUR_ARMS          BIT_19
#define AFF3_INERTIAL_BARRIER   BIT_20
#define AFF3_LIGHTNINGSHIELD    BIT_21
#define AFF3_COLDSHIELD         BIT_22
#define AFF3_CANNIBALIZE        BIT_23
#define AFF3_SWIMMING           BIT_24
#define AFF3_TOWER_IRON_WILL    BIT_25
#define AFF3_UNDERWATER         BIT_26
#define AFF3_BLUR               BIT_27
#define AFF3_ENHANCE_HEALING    BIT_28
#define AFF3_ELEMENTAL_FORM     BIT_29
#define AFF3_PASS_WITHOUT_TRACE BIT_30
#define AFF3_PALADIN_AURA       BIT_31
#define AFF3_FAMINE             BIT_32

#define AFF4_LOOTER                   BIT_1 /* Just looted someone, prevent rent */
#define AFF4_CARRY_PLAGUE             BIT_2
#define AFF4_SACKING                  BIT_3 /* sacking a guildhall */
#define AFF4_SENSE_FOLLOWER           BIT_4
#define AFF4_STORNOGS_SPHERES         BIT_5
#define AFF4_STORNOGS_GREATER_SPHERES BIT_6
#define AFF4_VAMPIRE_FORM             BIT_7
#define AFF4_NO_UNMORPH               BIT_8 /* can't return .. */
#define AFF4_HOLY_SACRIFICE           BIT_9
#define AFF4_BATTLE_ECSTASY     BIT_10
#define AFF4_DAZZLER            BIT_11
#define AFF4_PHANTASMAL_FORM    BIT_12
#define AFF4_NOFEAR             BIT_13
#define AFF4_REGENERATION       BIT_14
#define AFF4_DEAF               BIT_15
#define AFF4_BATTLETIDE         BIT_16
#define AFF4_EPIC_INCREASE      BIT_17
#define AFF4_MAGE_FLAME         BIT_18 /* magic torch floating above char */
#define AFF4_GLOBE_OF_DARKNESS  BIT_19 /* like mage flame but reverse! */
#define AFF4_DEFLECT            BIT_20
#define AFF4_HAWKVISION         BIT_21
#define AFF4_MULTI_CLASS        BIT_22
#define AFF4_SANCTUARY          BIT_23
#define AFF4_HELLFIRE           BIT_24
#define AFF4_SENSE_HOLINESS     BIT_25
#define AFF4_PROT_LIVING        BIT_26
#define AFF4_DETECT_ILLUSION    BIT_27
#define AFF4_ICE_AURA           BIT_28
#define AFF4_REV_POLARITY       BIT_29
#define AFF4_NEG_SHIELD         BIT_30
#define AFF4_TUPOR              BIT_31
#define AFF4_WILDMAGIC          BIT_32

/* if you add a new affect, make sure it makes sense to have it on items
 * or zone mobs loading with it. otherwise do not define affect - use
 * affected_by_spell with skill or spell code instead. in case you want
 * to create affect for the performance reasons, add it as aff5 or higher
 */
#define AFF5_DAZZLEE            BIT_1
#define AFF5_MENTAL_ANGUISH     BIT_2
#define AFF5_MEMORY_BLOCK       BIT_3
#define AFF5_VINES              BIT_4
#define AFF5_ETHEREAL_ALLIANCE  BIT_5
#define AFF5_BLOOD_SCENT        BIT_6
#define AFF5_FLESH_ARMOR        BIT_7
#define AFF5_WET                BIT_8
#define AFF5_HOLY_DHARMA        BIT_9
#define AFF5_ENH_HIDE           BIT_10
#define AFF5_LISTEN             BIT_11
#define AFF5_PROT_UNDEAD        BIT_12
#define AFF5_IMPRISON           BIT_13
#define AFF5_TITAN_FORM         BIT_14
#define AFF5_DELIRIUM           BIT_15
#define AFF5_SHADE_MOVEMENT     BIT_16
#define AFF5_NOBLIND            BIT_17
#define AFF5_MAGICAL_GLOW       BIT_18
#define AFF5_REFRESHING_GLOW    BIT_19
#define AFF5_MINE               BIT_20
#define AFF5_STANCE_OFFENSIVE   BIT_21
#define AFF5_STANCE_DEFENSIVE   BIT_22
#define AFF5_OBSCURING_MIST     BIT_23
#define AFF5_NOT_OFFENSIVE      BIT_24
#define AFF5_DECAYING_FLESH     BIT_25
#define AFF5_DREADNAUGHT        BIT_26
#define AFF5_FOREST_SIGHT       BIT_27
#define AFF5_THORNSKIN          BIT_28
#define AFF5_FOLLOWING          BIT_29
#define AFF5_ORDERING           BIT_30
#define AFF5_STONED             BIT_31

/* modifiers to char's abilities */

#define APPLY_NONE              0
#define APPLY_LOWEST            1
#define APPLY_STR               1
#define APPLY_DEX               2
#define APPLY_INT               3
#define APPLY_WIS               4
#define APPLY_CON               5
#define APPLY_SEX               6
#define APPLY_CLASS             7
#define APPLY_LEVEL             8
#define APPLY_AGE               9
#define APPLY_CHAR_WEIGHT      10
#define APPLY_CHAR_HEIGHT      11
#define APPLY_MANA             12
#define APPLY_HIT              13
#define APPLY_MOVE             14
#define APPLY_GOLD             15
#define APPLY_EXP              16
#define APPLY_AC               17
#define APPLY_ARMOR            17
#define APPLY_HITROLL          18
#define APPLY_DAMROLL          19
#define APPLY_SAVING_PARA      20
#define APPLY_SAVING_ROD       21
#define APPLY_SAVING_FEAR      22
#define APPLY_SAVING_BREATH    23
#define APPLY_SAVING_SPELL     24
#define APPLY_FIRE_PROT        25
#define APPLY_AGI              26  /* these 5 are the 'normal' applies for the new */
#define APPLY_POW              27  /* stats */
#define APPLY_CHA              28
#define APPLY_KARMA            29
#define APPLY_LUCK             30
#define APPLY_STR_MAX          31  /* these 10 can raise a stat above 100, I will */
#define APPLY_DEX_MAX          32  /* personally rip the lungs out of anyone using */
#define APPLY_INT_MAX          33  /* these on easy-to-get items.  JAB */
#define APPLY_WIS_MAX          34
#define APPLY_CON_MAX          35
#define APPLY_AGI_MAX          36
#define APPLY_POW_MAX          37
#define APPLY_CHA_MAX          38
#define APPLY_KARMA_MAX        39
#define APPLY_LUCK_MAX         40
#define APPLY_STR_RACE         41  /* these 10 override the racial stat_factor */
#define APPLY_DEX_RACE         42  /* so that setting APPLY_STR_RACE <ogre> will, */
#define APPLY_INT_RACE         43  /* for example, give you gauntlets of ogre strength. */
#define APPLY_WIS_RACE         44  /* these aren't imped yet, but I figured I'd add */
#define APPLY_CON_RACE         45  /* them so I don't forget about it. */
#define APPLY_AGI_RACE         46
#define APPLY_POW_RACE         47
#define APPLY_CHA_RACE         48
#define APPLY_KARMA_RACE       49
#define APPLY_LUCK_RACE        50
#define APPLY_CURSE            51
#define APPLY_SKILL_GRANT      52
#define APPLY_SKILL_ADD        53
#define APPLY_HIT_REG          54
#define APPLY_MOVE_REG         55
#define APPLY_MANA_REG         56
#define APPLY_SPELL_PULSE      57
#define APPLY_COMBAT_PULSE     58
#define APPLY_LAST             58

/* The various 'races', player races first, NPC only races after, I'm leaving
   a gap between the two to allow 'neat' addition of more player races.  The
   mob race codes are for use in the .mob files, it's an expansion of the old
   minimal system.  There is a lookup table in constant.c that will need adding
   to if you add races.  JAB */

#define RACE_NONE             0
#define RACE_HUMAN            1 /* mob race code: PH */
#define RACE_BARBARIAN        2 /* mob race code: PB */
#define RACE_DROW             3 /* mob race code: PL */
#define RACE_GREY             4 /* mob race code: PE */
#define RACE_MOUNTAIN         5 /* mob race code: PM */
#define RACE_DUERGAR          6 /* mob race code: PD */
#define RACE_HALFLING         7 /* mob race code: PF */
#define RACE_GNOME            8 /* mob race code: PG */
#define RACE_OGRE             9 /* mob race code: PO */
#define RACE_TROLL           10 /* mob race code: PT */
#define RACE_HALFELF         11 /* mob race code: P2 */
#define RACE_ILLITHID        12 /* mob race code: MF */
#define RACE_ORC             13 /* mob race code: HO */
#define RACE_THRIKREEN       14 /* mob race code: TK */
#define RACE_CENTAUR         15 /* mob race code: CT */
#define RACE_GITHYANKI       16 /* mob race code: GI */
#define RACE_MINOTAUR        17 /* mob race code: MT */
#define RACE_SHADE           18 /* mob race code: SA */
#define RACE_REVENANT        19 /* mob race code: AE */
#define RACE_GOBLIN          20 /* mob race code: HG */
#define RACE_LICH            21 /* mob race code: UL */
#define RACE_PVAMPIRE        22 /* mob race code: UM */
#define RACE_PDKNIGHT        23 /* mob race code: UK */
#define RACE_PSBEAST         24 /* mob race code: US */
#define RACE_SGIANT          25 /* mob race code: SG */
#define RACE_WIGHT           26 /* mob race code: UW */
#define RACE_PHANTOM         27 /* mob race code: UP */
#define RACE_HARPY           28 /* mob race code: MH */
#define RACE_OROG            29 /* mob race code: OG */
#define RACE_GITHZERAI       30 /* mob race code: GZ */
#define RACE_DRIDER          31 /* mob race code: DR */
#define RACE_KOBOLD          32 /* mob race code: KB */
#define RACE_PILLITHID       33 /* mob race code: PI */
#define RACE_KUOTOA          34 /* mob race code: KT */
#define RACE_WOODELF         35 /* mob race code: WE */
#define RACE_FIRBOLG         36 /* mob race code: FB */
#define RACE_TIEFLING        37 /* mob race code: TF */
#define RACE_PLAYER_MAX RACE_TIEFLING
#define RACE_AGATHINON       38 /* mob race code: EH */
#define RACE_ELADRIN         39 /* mob race code: EL */
#define RACE_GARGOYLE        40 /* mob race code: MG */
#define RACE_F_ELEMENTAL     41 /* mob race code: EF */
#define RACE_A_ELEMENTAL     42 /* mob race code: EA */
#define RACE_W_ELEMENTAL     43 /* mob race code: EW */
#define RACE_E_ELEMENTAL     44 /* mob race code: EE */
#define RACE_DEMON           45 /* mob race code: X  */
#define RACE_DEVIL           46 /* mob race code: Y  */
#define RACE_UNDEAD          47 /* mob race code: U  */
#define RACE_VAMPIRE         48 /* mob race code: UV */
#define RACE_GHOST           49 /* mob race code: UG */
#define RACE_LYCANTH         50 /* mob race code: L  */
#define RACE_GIANT           51 /* mob race code: G  */
#define RACE_HALFORC         52 /* mob race code: H2 */
#define RACE_GOLEM           53 /* mob race code: OG */
#define RACE_FAERIE          54 /* mob race code: HF */
#define RACE_DRAGON          55 /* mob race code: D  */
#define RACE_DRAGONKIN       56 /* mob race code: DK */
#define RACE_REPTILE         57 /* mob race code: R  */
#define RACE_SNAKE           58 /* mob race code: RS */
#define RACE_INSECT          59 /* mob race code: I  */
#define RACE_ARACHNID        60 /* mob race code: AS */
#define RACE_AQUATIC_ANIMAL  61 /* mob race code: F  */
#define RACE_FLYING_ANIMAL   62 /* mob race code: B  */
#define RACE_QUADRUPED       63 /* mob race code: AE */
#define RACE_PRIMATE         64 /* mob race code: AA */
#define RACE_HUMANOID        65 /* mob race code: H  */
#define RACE_ANIMAL          66 /* mob race code: A  */
#define RACE_PLANT           67 /* mob race code: VT */
#define RACE_HERBIVORE       68 /* mob race code: AH */
#define RACE_CARNIVORE       69 /* mob race code: AC */
#define RACE_PARASITE        70 /* mob race code: AP */
#define RACE_BEHOLDER        71 /* mob race code: BH */
#define RACE_DRACOLICH       72 /* mob race code: UD */
#define RACE_SLIME           73 /* mob race code: SL */
#define RACE_ANGEL           74 /* mob race code: AN */
#define RACE_RAKSHASA        75 /* mob race code: RA */
#define RACE_CONSTRUCT       76 /* mob race code: CN */
#define RACE_EFREET          77 /* mob race code: E  */
#define RACE_SNOW_OGRE       78 /* mob race code: SO */
#define RACE_BEHOLDERKIN     79 /* mob race code: BK */
#define RACE_ZOMBIE          80 /* mob race code: ZO */
#define RACE_SPECTRE         81 /* mob race code: SP */
#define RACE_SKELETON        82 /* mob race code: SK */
#define RACE_WRAITH          83 /* mob race code: WR */
#define RACE_SHADOW          84 /* mob race code: SW */
#define RACE_PWORM           85 /* mob race code: PW */
#define RACE_V_ELEMENTAL     86 /* mob race code: VE */
#define RACE_I_ELEMENTAL     87 /* mob race code: IE */
#define RACE_PHOENIX         88 /* mob race code: PX */
#define RACE_ARCHON          89 /* mob race code: AR */
#define RACE_ASURA           90 /* mob race code: AU */
#define RACE_TITAN           91 /* mob race code: TT */
#define RACE_AVATAR          92 /* mob race code: AV */
#define RACE_GHAELE          93 /* mob race code: GH */
#define RACE_BRALANI         94 /* mob race code: BR */
#define RACE_WHINER          95 /* mob race code: WH */
#define RACE_INCUBUS         96 /* mob race code: IN */
#define RACE_SUCCUBUS        97 /* mob race code: SU */
#define RACE_FIREGIANT       98 /* mob race code: FG */
#define RACE_FROSTGIANT      99 /* mob race code: IG */
#define RACE_DEVA           100 /* mob race code: DV */
#define LAST_RACE           100 /* 100 races on duris today, 100 races..*/

#define DEFINED_RACES       100 /* actual number of races defined */
#define MAX_HATRED	     5

#define RACEWAR_NONE         0
#define RACEWAR_GOOD         1
#define RACEWAR_EVIL         2
#define RACEWAR_UNDEAD       3
#define RACEWAR_NEUTRAL      4
#define MAX_RACEWAR          4

struct racewar_struct
{
  const char color;
  const char *name;
};

#define MAX_SURNAME                         9

struct surname_struct
{
  const char *color_name;
  const char *name;
  int         achievement_number;
};

// 1 is default (everyone has), 6 is frag based (2000 means 20.00 frags).  The rest are via achievements.
#define HAS_SURNAME( ch, i ) ( (i == 1) ? TRUE : (( i == 6 ) ? ( GET_FRAGS(ch) > 2000 ) \
  : ( affected_by_spell(ch, surnames[i].achievement_number) )) )

/* class defn's (PC) */
/* IF YOU ADD A CLASS, YOU NEED TO ADD IT HERE AND EXPMOD_CLS_...
 * IF YOU REMOVE A CLASS, YOU SHOULD TO UPDATE EXPMOD_CLS_... to EXPMOD_CLS_UNUSED or such."
 * What's important is that flag2idx(<class>) == EXPMOD_CLS_<class>.
 */
#define CLASS_NONE                   0
#define CLASS_WARRIOR            BIT_1
#define CLASS_RANGER             BIT_2
#define CLASS_PSIONICIST         BIT_3
#define CLASS_PALADIN            BIT_4
#define CLASS_ANTIPALADIN        BIT_5
#define CLASS_CLERIC             BIT_6
#define CLASS_MONK               BIT_7
#define CLASS_DRUID              BIT_8
#define CLASS_SHAMAN             BIT_9
#define CLASS_SORCERER          BIT_10
#define CLASS_NECROMANCER       BIT_11
#define CLASS_CONJURER          BIT_12
#define CLASS_ROGUE             BIT_13
#define CLASS_ASSASSIN          BIT_14
#define CLASS_MERCENARY         BIT_15
#define CLASS_BARD              BIT_16
#define CLASS_THIEF             BIT_17
#define CLASS_WARLOCK           BIT_18
#define CLASS_MINDFLAYER        BIT_19
#define CLASS_ALCHEMIST         BIT_20
#define CLASS_BERSERKER         BIT_21
#define CLASS_REAVER            BIT_22
#define CLASS_ILLUSIONIST       BIT_23
#define CLASS_BLIGHTER          BIT_24
#define CLASS_DREADLORD         BIT_25
#define CLASS_ETHERMANCER       BIT_26
#define CLASS_AVENGER           BIT_27
#define CLASS_THEURGIST         BIT_28
#define CLASS_SUMMONER          BIT_29
#define CLASS_COUNT                 29

#define CLASS_TYPE_WARRIOR   1
#define CLASS_TYPE_MAGE      2
#define CLASS_TYPE_CLERIC    3
#define CLASS_TYPE_THIEF     4

/* animals fall into three categorizations */
#define ANIMAL_TYPE_BIRD     0
#define ANIMAL_TYPE_REPTILE  1
#define ANIMAL_TYPE_MAMMAL   2
#define ANIMAL_TYPE_FISH     3

/* hometowns SAM 7-94 */
#define HOME_CHOICE             -1      /* player has choice among several towns */
#define HOME_NONE                0
#define HOME_THARN               1      /* all goodies */
#define HOME_IXARKON             2       /* barbarian */
#define HOME_ARACHDRATHOS        3       /* drow elf */
#define HOME_SYLVANDAWN          4       /* grey-elf, half-elf */
#define HOME_MITHRIL_HALL        5      /* mountain dwarf */
#define HOME_GRANTOR             6      /* duergar */
#define HOME_WOODSEER            7      /* halfling */
#define HOME_ASHREMITE           8      /* gnome */
#define HOME_FAANGE              9      /* ogre */
#define HOME_GHORE              10      /* troll */
#define HOME_UGTA               11   /* human, half-elf */
#define HOME_BLOODSTONE         12      /* evil humans */
#define HOME_SHADY              13      /* orcs, goblin */
#define HOME_NAXVARAN           14      /* minotaur */
#define HOME_MARIGOT            15      /* centaur */
#define HOME_CHARIN             16      /* grey-elf, half-elf */
#define HOME_CITYRUINS          17      /* undead? */
#define HOME_KHHIYTIK           18      /* thrikreen */
#define HOME_GITHFORT           19      /* githyanki */
#define HOME_GOBLIN             20      /* goblin */
#define HOME_HARPY              21      /* harpy */
#define HOME_NEWBIE             22
#define HOME_PLANE_OF_LIFE	    23
#define HOME_OROGS				24      /* orog */

#define LAST_HOME               24      /* number of last hometown */

/* initial alignments SAM 7-94 */
#define ALIGN_EVIL              -1
#define ALIGN_NEUTRAL           0
#define ALIGN_GOOD              1
#define ALIGN_CHOICE            2       /* player has choice among E/N/G */

/* sex */
#define SEX_NEUTRAL     0
#define SEX_MALE        1
#define SEX_FEMALE      2

/* size */
#define SIZE_DEFAULT    (-1)
#define SIZE_NONE       0
#define SIZE_TINY       1
#define SIZE_SMALL      2
#define SIZE_MEDIUM     3
#define SIZE_LARGE      4
#define SIZE_HUGE       5
#define SIZE_GIANT      6
#define SIZE_GARGANTUAN 7
#define SIZE_MAXIMUM    7
#define SIZE_MINIMUM    1

/* these two work together to give us many possible positions. */

/* postures */
#define POS_PRONE       0
#define POS_SITTING     1
#define POS_KNEELING    2
#define POS_STANDING    3

/* status */
#define STAT_DEAD       BIT_3
#define STAT_DYING      BIT_4
#define STAT_INCAP      BIT_5
#define STAT_SLEEPING   BIT_6
#define STAT_RESTING    BIT_7
#define STAT_NORMAL     BIT_8

#define STAT_MASK       (BIT_9 - 4)

/* for mobile actions: specials.act */
#define ACT_SPEC               BIT_1    /* mob has a special routine           */
#define ACT_SENTINEL           BIT_2    /* this mobile not to be moved         */
#define ACT_SCAVENGER          BIT_3    /* pick up stuff lying around          */
#define ACT_ISNPC              BIT_4    /* This bit is set for use with IS_NPC */
#define ACT_NICE_THIEF         BIT_5    /* Set if a thief should NOT be killed */
#define ACT_BREATHES_FIRE      BIT_6
#define ACT_STAY_ZONE          BIT_7    /* MOB Must stay inside its own zone   */
#define ACT_WIMPY              BIT_8    /* MOB Will flee when injured          */
#define ACT_BREATHES_LIGHTNING BIT_9
#define ACT_BREATHES_FROST     BIT_10
#define ACT_BREATHES_ACID      BIT_11
#define ACT_MEMORY             BIT_12
#define ACT_IMMUNE_TO_PARA     BIT_13
#define ACT_NO_SUMMON          BIT_14   /* mob can't be summoned */
#define ACT_NO_BASH            BIT_15
#define ACT_TEACHER            BIT_16   /* guildmaster */
#define ACT_IGNORE             BIT_17
#define ACT_CANFLY             BIT_18
#define ACT_CANSWIM            BIT_19
#define ACT_BREATHES_GAS       BIT_20
#define ACT_BREATHES_SHADOW    BIT_21
#define ACT_BREATHES_BLIND_GAS BIT_22
#define ACT_GUILD_GOLEM        BIT_23   /* has 'guild golem' proc */
#define ACT_SPEC_DIE           BIT_24
#define ACT_ELITE              BIT_25
#define ACT_BREAK_CHARM        BIT_26
#define ACT_PROTECTOR          BIT_27
#define ACT_MOUNT              BIT_28   /* MOB can be mounted by player -DCL */
#define ACT_WILDMAGIC          BIT_29
#define ACT_PATROL             BIT_30
#define ACT_HUNTER             BIT_31   /* killeristic hunt mode, in which
                                         mob hunts regardless of sentinel */
#define ACT_SPEC_TEACHER       BIT_32   /* Mob aggroes on outcasts */

#define ACT2_COMBAT_NEARBY     BIT_1
#define ACT2_NO_LURE           BIT_2    // Will not hunt via MobHuntCheck
#define ACT2_REMEMBERS_GROUP   BIT_3    // experimental setting - will remember whole group
#define ACT2_BACK_RANK         BIT_22   // must be same as PLR2_BACK_RANK
#define ACT2_WAIT              BIT_29   // must be same as PLR2_WAIT

/* aggro flags for mobs */

#define AGGR_ALL             BIT_1
#define AGGR_DAY_ONLY        BIT_2
#define AGGR_NIGHT_ONLY      BIT_3
#define AGGR_GOOD_ALIGN      BIT_4
#define AGGR_NEUTRAL_ALIGN   BIT_5
#define AGGR_EVIL_ALIGN      BIT_6
#define AGGR_GOOD_RACE       BIT_7
#define AGGR_EVIL_RACE       BIT_8
#define AGGR_UNDEAD_RACE     BIT_9
#define AGGR_OUTCASTS        BIT_10
#define AGGR_FOLLOWERS       BIT_11
#define AGGR_UNDEAD_FOL      BIT_12
#define AGGR_ELEMENTALS      BIT_13
#define AGGR_DRACOLICH       BIT_14
#define AGGR_HUMAN           BIT_15
#define AGGR_BARBARIAN       BIT_16
#define AGGR_DROW_ELF        BIT_17
#define AGGR_GREY_ELF        BIT_18
#define AGGR_MOUNT_DWARF     BIT_19
#define AGGR_DUERGAR         BIT_20
#define AGGR_HALFLING        BIT_21
#define AGGR_GNOME           BIT_22
#define AGGR_OGRE            BIT_23
#define AGGR_TROLL           BIT_24
#define AGGR_HALF_ELF        BIT_25
#define AGGR_ILLITHID        BIT_26
#define AGGR_ORC             BIT_27
#define AGGR_THRIKREEN       BIT_28
#define AGGR_CENTAUR         BIT_29
#define AGGR_GITHYANKI       BIT_30
#define AGGR_MINOTAUR        BIT_31
#define AGGR_GOBLIN          BIT_32

#define AGGR2_LICH           BIT_1
#define AGGR2_PVAMPIRE       BIT_2
#define AGGR2_PDKNIGHT       BIT_3
#define AGGR2_PSBEAST        BIT_4
#define AGGR2_WARRIOR        BIT_5
#define AGGR2_RANGER         BIT_6
#define AGGR2_PSIONICIST     BIT_7
#define AGGR2_PALADIN        BIT_8
#define AGGR2_ANTIPALADIN    BIT_9
#define AGGR2_CLERIC         BIT_10
#define AGGR2_MONK           BIT_11
#define AGGR2_DRUID          BIT_12
#define AGGR2_SHAMAN         BIT_13
#define AGGR2_SORCERER       BIT_14
#define AGGR2_NECROMANCER    BIT_15
#define AGGR2_CONJURER       BIT_16
#define AGGR2_ROGUE          BIT_17
#define AGGR2_ASSASSIN       BIT_18
#define AGGR2_MERCENARY      BIT_19
#define AGGR2_BARD           BIT_20
#define AGGR2_THIEF          BIT_21
#define AGGR2_WARLOCK        BIT_22
#define AGGR2_MINDFLAYER     BIT_23
#define AGGR2_MALE           BIT_24
#define AGGR2_FEMALE         BIT_25
#define AGGR2_SGIANT         BIT_26
#define AGGR2_WIGHT          BIT_27
#define AGGR2_PHANTOM        BIT_28
#define AGGR2_SHADE          BIT_29
#define AGGR2_REVENANT       BIT_30
#define AGGR2_GITHZERAI      BIT_31
#define AGGR2_THEURGIST      BIT_32

#define AGGR3_OROG	     BIT_1
#define AGGR3_DRIDER         BIT_2
#define AGGR3_KOBOLD         BIT_3
#define AGGR3_KUOTOA         BIT_4
#define AGGR3_WOODELF        BIT_5
#define AGGR3_FIRBOLG        BIT_6
#define AGGR3_AGATHINON      BIT_7
#define AGGR3_ELADRIN        BIT_8
#define AGGR3_PILLITHID      BIT_9
#define AGGR3_ALCHEMIST      BIT_10
#define AGGR3_BERSERKER      BIT_11
#define AGGR3_REAVER         BIT_12
#define AGGR3_ILLUSIONIST    BIT_13
#define AGGR3_ETHERMANCER    BIT_14
#define AGGR3_DREADLORD      BIT_15
#define AGGR3_AVENGER        BIT_16
#define AGGR3_BLIGHTER       BIT_17
#define AGGR3_SUMMONER       BIT_18

#define SECS_BETWEEN_AFF_REFRESH  60    /* RL seconds between each refresh */

/* hunt types that use a P_char as a target should be below 127.  Ones
   that use a room as a target should be above 127 */

#define HUNT_HUNTER             1 /* ACT_HUNTER mobs */
#define HUNT_JUSTICE_ARREST     2 /* going to arrest someone */
#define HUNT_JUSTICE_OUTCAST    3 /* going to kill an outcast */
#define HUNT_JUSTICE_INVADER    4 /* going to kill an invader */
#define HUNT_JUSTICE_SPECVICT   5 /* special justice purpose */
/******************************************************************
  All types above use 'victim' as targ */


/* All types below use 'room' as targ
******************************************************************/
#define HUNT_JUSTICE_SPECROOM   128 /* special justice purpose */
#define HUNT_ROOM               129 /* just run to that room */
#define HUNT_JUSTICE_HELP       130 /* call for help from that room */

#define ZONE_NORMAL             1 /* zone not being raided or anything */
#define ZONE_REPAIR             2
#define ZONE_RAID               3
#define ZONE_SACK               4
#define ZONE_DESTROYED          5

#define KVARK_IMPROVED_FIGHTS 0

typedef struct _flagDef {
  const char *flagShort;//[FLAG_SHORT_LEN];
  const char *flagLong;//[FLAG_LONG_LEN];
  char editable;  // editable w/o special switch?  (in first element of
        // table, specifies number of flags in table)
  int defVal;     // default value (IS_NPC for mobs - 1, DEATH for rooms - 0,
                  // for example - hardly any flags will have def vals of 1)
                  // for 'enumerated' flagDef tables, specifies which value
                  // entry has - that's also why it's an int rather than a
} flagDef;

struct race_names {
  const char *normal;
  const char *no_spaces;
  const char *ansi;
  const char *code;
};

struct class_names {
  const char *normal;
  const char *ansi;
  const char *code;
  const char letter;
};

struct material_data {
  const char *name;
  const char dam_res[20];
};

#define _NEW_LOW_NECRO_ 0
#define PLAYERLESS_ZONE_SPEED_MODIFIER     3
#define WH_HIGH_PRIEST_VNUM            55184
#define IMAGE_REFLECTION_VNUM            250
#define DRAGONLORD_PLATE_VNUM          25723
#define REVENANT_CROWN_VNUM            22070
#define DWARVEN_ANCESTOR_VNUM             75

#define SNEAK(ch) (IS_AFFECTED(ch, AFF_SNEAK) || UD_SNEAK(ch) || OUTDOOR_SNEAK(ch) || SWAMP_SNEAK(ch))
#define LEVITATE(ch, dir) (IS_AFFECTED(ch, AFF_LEVITATE) && ((dir == DIR_UP) || (dir == DIR_DOWN)))

// world_quest_.c
#define FIND_AND_SOMETHING   0
#define FIND_AND_KILL   1
#define FIND_AND_ASK   2

#define RANDOM_ZONES 1 // Set to 1 to enable

#define MAX_ALTITUDE 3

#define ROLL_BAD       -1
#define ROLL_NORMAL     0
#define ROLL_MOB_NORMAL 1
#define ROLL_MOB_GOOD   2
#define ROLL_MOB_ELITE  3

// It is IMPERATIVE that EXP_MOD_CLS_* == flag2idx(CLASS_*)
// If you add a new class, you MUST add EXP_MOD_CLS_<newclass> and shift the rest of the values down one!!!
// Note: On 10/25/2015, I left 3 empty spaces for new classes, just change EXPMOD_CLS_NEWCLASS{1|2|3} with
//   EXPMOD_CLS_<new class name>; no need to shift (these slots are unused atm).
#define EXPMOD_NONE                         0
#define EXPMOD_CLS_WARRIOR                  1
#define EXPMOD_CLS_RANGER                   2
#define EXPMOD_CLS_PSIONICIST               3
#define EXPMOD_CLS_PALADIN                  4
#define EXPMOD_CLS_ANTIPALADIN              5
#define EXPMOD_CLS_CLERIC                   6
#define EXPMOD_CLS_MONK                     7
#define EXPMOD_CLS_DRUID                    8
#define EXPMOD_CLS_SHAMAN                   9
#define EXPMOD_CLS_SORCERER                10
#define EXPMOD_CLS_NECROMANCER             11
#define EXPMOD_CLS_CONJURER                12
#define EXPMOD_CLS_ROGUE                   13
#define EXPMOD_CLS_ASSASSIN                14
#define EXPMOD_CLS_MERCENARY               15
#define EXPMOD_CLS_BARD                    16
#define EXPMOD_CLS_THIEF                   17
#define EXPMOD_CLS_WARLOCK                 18
#define EXPMOD_CLS_MINDFLAYER              19
#define EXPMOD_CLS_ALCHEMIST               20
#define EXPMOD_CLS_BERSERKER               21
#define EXPMOD_CLS_REAVER                  22
#define EXPMOD_CLS_ILLUSIONIST             23
#define EXPMOD_CLS_BLIGHTER                24
#define EXPMOD_CLS_DREADLORD               25
#define EXPMOD_CLS_ETHERMANCER             26
#define EXPMOD_CLS_AVENGER                 27
#define EXPMOD_CLS_THEURGIST               28
#define EXPMOD_CLS_SUMMONER                29
#define EXPMOD_CLS_NEWCLASS1               30
#define EXPMOD_CLS_NEWCLASS2               31
#define EXPMOD_CLS_NEWCLASS3               32
#define EXPMOD_LVL_31_UP                   33
#define EXPMOD_LVL_41_UP                   34
#define EXPMOD_LVL_51_UP                   35
#define EXPMOD_LVL_55_UP                   36
#define EXPMOD_RES_EVIL                    37
#define EXPMOD_RES_NORMAL                  38
#define EXPMOD_VICT_BREATHES               39
#define EXPMOD_VICT_ACT_AGGRO              40
#define EXPMOD_VICT_ACT_HUNTER             41
#define EXPMOD_VICT_ELITE                  42
#define EXPMOD_VICT_HOMETOWN               43
#define EXPMOD_VICT_NOMEMORY               44
#define EXPMOD_PVP                         45
#define EXPMOD_GLOBAL                      46
#define EXPMOD_GOOD                        47
#define EXPMOD_EVIL                        48
#define EXPMOD_UNDEAD                      49
#define EXPMOD_NEUTRAL                     50
#define EXPMOD_DAMAGE                      51
#define EXPMOD_HEAL_NONHEALER              52
#define EXPMOD_HEAL_PETS                   53
#define EXPMOD_HEALING                     54
#define EXPMOD_MELEE                       55
#define EXPMOD_TANK                        56
#define EXPMOD_KILL                        57
#define EXPMOD_PALADIN_VS_GOOD             58
#define EXPMOD_PALADIN_VS_EVIL             59
#define EXPMOD_ANTIPALADIN_VS_GOOD         60
#define EXPMOD_OVER_LEVEL_CAP              61

#define EXPMOD_MAX                         61

#endif /* _DURIS_DEFINES_H_ */
