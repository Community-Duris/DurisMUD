
/*****************************************************************************
 * file: structs.h,                                          part of Duris *
 * Usage: This contains almost all of the structures for the mud             *
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

#ifndef _SOJ_STRUCTS_H_
#define _SOJ_STRUCTS_H_

#ifndef _SOJ_CONFIG_H_
#include "config.h"
#endif

#include "defines.h"
#include "player_log.h"
#include "map.h"

#ifdef _HPUX_SOURCE
#define srandom srand
#define random rand
#endif

#include <sys/types.h>
#include <zlib.h>

#ifdef _LINUX_SOURCE
#include <linux/time.h>
#else
#include <time.h>
#endif
#include "account.h"

#ifdef EFENCE
#include <stdlib.h>
#include "efence.h"
#endif

#ifdef _OSX_
typedef unsigned long int ulong;
#endif

#ifndef __cplusplus
typedef char                   bool;
#define false 0
#define true 1
#endif

typedef char                      byte;
typedef signed char               sbyte;
typedef signed short int          sh_int;
typedef struct AC_Memory          Memory;
typedef struct char_data         *P_char;
typedef struct Guild             *P_Guild;
typedef struct Alliance          *P_Alliance;
typedef struct descriptor_data   *P_desc;
typedef struct event_data        *P_event;
typedef struct nevent_data       *P_nevent;
typedef struct obj_data          *P_obj;
typedef struct room_data         *P_room;
typedef struct affected_by       *P_aff;
typedef struct mob_prog_data     *P_mprog;
typedef struct mob_prog_act_list *P_mprog_list;
typedef struct arti_data         *P_arti;
typedef unsigned char             ubyte;
typedef unsigned short int        ush_int;
typedef struct witness_data       wtns_rec;
typedef struct crime_info         crime_rec;
typedef struct crime_data         crm_rec;
// old guildhalls (deprecated)
//typedef struct house_control_rec *P_house;
//typedef struct house_upgrade_rec *P_house_upgrade;
typedef struct acct_entry *P_acct;
typedef void(*event_func_type)( P_char , P_char , P_obj , void * );

#ifdef REALTIME_COMBAT
typedef struct combat_data *P_combat;

#endif

#define MVFLG_DRAG_FOLLOWERS     BIT_1
#define MVFLG_NOMSG              BIT_2
#define MVFLG_FLEE               BIT_3

#define MESS_ATTACKER 1
#define MESS_VICTIM   2
#define MESS_ROOM     3

#define MAX_WEAR_OFF_MESSAGES  32

/* for 'generic_find' */

#define FIND_CHAR_ROOM     BIT_1
#define FIND_CHAR_WORLD    BIT_2
#define FIND_OBJ_INV       BIT_3
#define FIND_OBJ_ROOM      BIT_4
#define FIND_OBJ_WORLD     BIT_5
#define FIND_OBJ_EQUIP     BIT_6
#define FIND_IGNORE_ZCOORD BIT_7  // bit 7 and 8 not currently imped
#define FIND_MORT_ZCOORD   BIT_8  // mort zcoord == can get target if no more than 2 z diff, or any targ in room if hawkvision
#define FIND_NO_TRACKS     BIT_9  // Ignore object vnum VNUM_TRACKS (tracks left by players).

#define AFFTYPE_SHORT      BIT_1  // duration is in pulses instead of ticks
#define AFFTYPE_NOSHOW     BIT_2  // affect wont show up on score
#define AFFTYPE_NODISPEL   BIT_3  // affect cannot be dispelled
#define AFFTYPE_NOMSG      BIT_4  // affect should not produce a wear_off_message
#define AFFTYPE_CUSTOM1    BIT_5  // those flags are to be used for
#define AFFTYPE_CUSTOM2    BIT_6  // spec/skill specific distinguishing
#define AFFTYPE_LINKED_CH  BIT_7
#define AFFTYPE_NOSAVE     BIT_8
#define AFFTYPE_NOAPPLY    BIT_9  // do not apply anything from this affect on char, it's used to store data only
#define AFFTYPE_PERM       BIT_10 // affect will not disappear upon death
#define AFFTYPE_OFFLINE    BIT_11 // Continue to countdown timer while offline.
#define AFFTYPE_LINKED_OBJ BIT_12
#define AFFTYPE_STORE (AFFTYPE_NOAPPLY |\
                       AFFTYPE_NODISPEL |\
                       AFFTYPE_NOSHOW)
#define MAX_FORGE_ITEMS 1000
#define MEMTYPE_FULL    AFFTYPE_CUSTOM1

#define CST_SPELLWEAVE   BIT_1

#define MAX_NAME_LENGTH 12

/* structs for shops and utils related */
struct stack_data {
  int data[100];
  int len;
};
struct shop_buy_data {
  int type;
  char *keywords;
};

typedef int (*shop_proc_type) (P_char, P_char, int, char *);

struct shop_data {
  int close1, close2;           /* When does the shop close?               */
  int flag;                     /* Flag for open/close messages            */
  int in_room;                  /* * Where is the shop?                      */
  int keeper;                   /* * The mobil who owns the shop (virtual)   */
  int open1, open2;             /* * When does the shop open?                */
  int producing[MAX_PROD];      /* * Which item to produce (virtual)   */
  struct shop_buy_data *type;   /* * Which items to trade              */
  int shop_hometown;            /* * What hometown (if any) shop is in       */
  int social_action_types;      /* * Flag->For random social generator */
  int temptime;                 /* * Time storage flag.                      */
  int with_who;                 /* * Who does the shop trade with?           */
  float buy_percent;            /* * Factor to multiply cost with.           */
  float sell_percent;           /* * Factor to multiply cost with.           */
  char *close_message;          /* * Path from work to home in evening       */
  char *do_not_buy;             /* * If keeper dosn't buy such things.       */
  char *message_buy;            /* * Message when player buys item           */
  char *message_sell;           /* * Message when player sells item          */
  char *missing_cash1;          /* * Message if keeper hasn't got cash       */
  char *missing_cash2;          /* * Message if player hasn't got cash       */
  char *no_such_item1;          /* * Message if keeper hasn't got an item    */
  char *no_such_item2;          /* * Message if player hasn't got an item    */
  char *open_message;           /* * Path from home to work in morning       */
  char *racist_message;         /* * Message given to opposite races.        */
  byte magic_allowed;           /* * Flag->Can magic be cast in the shop     */
  byte number_items_produced;   /* * Number of items they make to sell */
  byte number_types_traded;     /* * Number of item types shop will buy  */
  byte racist;                  /* * Does this mob trade with other races?   */
  byte shop_is_roaming;         /* * Flag->Does shopkeeper roam the world?   */
  byte shop_killable;           /* * Does shopkeeper allow attacks?          */
  byte shop_new_options;        /* * Flag->Does shop use New or Old format?  */
  byte shopkeeper_race;         /* * Race of the shopkeeper.                 */
  byte temper1;                 /* * How does keeper react if no money       */
  byte temper2;                 /* * How does keeper react when attacked     */
  shop_proc_type func;          /* * Secondary spec_proc for shopkeeper      */
};

/* struct for the new editor code */

struct edit_data {
  char **lines;
  int max_lines;
  int cur_line;
  int is_god;
  P_desc desc;
  void (*callback)(P_desc desc, int callback_data, char *text);
  int callback_data;
};

/* The following defs are for obj_data  */

/* note:  before reassigning any of these, especially the ones that you have no
   clue about, they are probably there for compatibility with something, if you
   get ambitious, find out which ones aren't used and kill them.  JAB */

/* object 'type' */

#define PC_CORPSE       BIT_1
#define NPC_CORPSE      BIT_2
#define MISSING_SKULL   BIT_3
#define MISSING_SCALP   BIT_4
#define MISSING_FACE    BIT_5
#define MISSING_EYES    BIT_6
#define MISSING_EARS    BIT_7
#define MISSING_TONGUE  BIT_8
#define MISSING_BOWELS  BIT_9
#define MISSING_ARMS    BIT_10
#define MISSING_LEGS    BIT_11
#define HUMANOID_CORPSE BIT_16

/* kingdom stuff */

/* Troop constants */

#define TROOP_FOOT              0
#define TROOP_ARCHER            1
#define TROOP_CAVALRY           2
#define TROOP_SCOUT             3
#define TROOP_ASSASSIN          4
#define TROOP_CLERIC            5
#define TROOP_MAGE              6

#define NUM_TROOP_TYPES         7

#define TROOP_LEVEL_RAW         0
#define TROOP_LEVEL_SEASONED    1
#define TROOP_LEVEL_VETERAN     2
#define TROOP_LEVEL_ELITE       3

#define NUM_TROOP_LEVELS        4

#define TROOP_OFFENSE_BASIC     0
#define TROOP_OFFENSE_AVERAGE   1
#define TROOP_OFFENSE_GOOD      2

#define NUM_TROOP_OFFENSE       3

#define TROOP_DEFENSE_BASIC     0
#define TROOP_DEFENSE_AVERAGE   1
#define TROOP_DEFENSE_GOOD      2

#define NUM_TROOP_DEFENSE       3

#define LNK_CONSENT          0
#define LNK_RIDING           1
#define LNK_GUARDING         2
#define LNK_EVENT            3
#define LNK_SNOOPING         4
#define LNK_FLANKING         5
#define LNK_SONG             6
#define LNK_BATTLE_ORDERS    7
#define LNK_PET              8
#define LNK_ETHEREAL         9
#define LNK_ESSENCE_OF_WOLF 10
#define LNK_DEFEND          11
#define LNK_BLOOD_ALLIANCE  12
#define LNK_SUPPRESS_SOUND  13
#define LNK_CAST_WORLD      14
#define LNK_CAST_ROOM       15
#define LNK_PALADIN_AURA    16
#define LNK_GRAPPLED        17
#define LNK_CIRCLING        18
#define LNK_TETHER          19
#define LNK_SNG_HEALING     20
#define LNK_CEGILUNE        21
#define LNK_CHAR_OBJ_AFF    22
#define LNK_MAX             22

#define LNKFLG_NONE                 0
#define LNKFLG_ROOM             BIT_1
#define LNKFLG_AFFECT           BIT_2
#define LNKFLG_EXCLUSIVE        BIT_3
#define LNKFLG_OBJECT           BIT_4
#define LNKFLG_REMOVE_AFF       BIT_5 // Clear the affect associated when this link is broken.
#define LNKFLG_BREAK_REMOVE     BIT_6 // Break this link when obj is removed.
#define LNKFLG_SHOW_REMOVE_MSG  BIT_7 // Show message when removing affect.

#define PET_NOCASH       BIT_1
#define PET_NOORDER      BIT_2
#define PET_NOAGGRO      BIT_3

#define INNATE_HORSE_BODY               0
#define INNATE_LEVITATE                 1
#define INNATE_DARKNESS                 2
#define INNATE_FAERIE_FIRE              3
#define INNATE_UD_INVISIBILITY          4
#define INNATE_STRENGTH                 5
#define INNATE_DOORBASH                 6
#define INNATE_INFRAVISION              7
#define INNATE_SUMMON_HORDE             8
#define INNATE_ULTRAVISION              9
#define INNATE_OUTDOOR_SNEAK           10
#define INNATE_BODYSLAM                11
#define INNATE_SUMMON_MOUNT            12
#define INNATE_ANTI_GOOD               13
#define INNATE_ANTI_EVIL               14
#define INNATE_OGREROAR                15
#define INNATE_BLAST                   16
#define INNATE_UD_SNEAK                17
#define INNATE_SHIFT_ASTRAL            18
#define INNATE_SHIFT_PRIME             19
#define INNATE_VAMPIRIC_TOUCH          20
#define INNATE_BITE                    21
#define INNATE_LEAP                    22
#define INNATE_DOORKICK                23
#define INNATE_STAMPEDE                24
#define INNATE_CHARGE                  25
#define INNATE_WATERBREATH             26
#define INNATE_ENLARGE                 27
#define INNATE_REGENERATION            28
#define INNATE_REDUCE                  29
#define INNATE_BARB_BREATH             30
#define INNATE_PROJECT_IMAGE           31
#define INNATE_FIREBALL                32
#define INNATE_FIRESHIELD              33
#define INNATE_FIRESTORM               34
#define INNATE_PROT_FIRE               35
#define INNATE_TUPOR                   36
#define INNATE_SNEAK                   37
#define INNATE_PROT_LIGHTNING          38
#define INNATE_PLANE_SHIFT             39
#define INNATE_CHARM_ANIMAL            40
#define INNATE_HIDE                    41
#define INNATE_DISPEL_MAGIC            42
#define INNATE_GLOBE_OF_DARKNESS       43
#define INNATE_MASS_DISPEL             44
#define INNATE_DISAPPEAR               45
#define INNATE_FLURRY                  46
#define INNATE_SHAPECHANGE             47
#define INNATE_BATTLE_FRENZY           48
#define INNATE_THROW_LIGHTNING         49
#define INNATE_FLY                     50
#define INNATE_STONE                   51
#define INNATE_PHANTASMAL_FORM         52
#define INNATE_FARSEE                  53
#define INNATE_SHADE_MOVEMENT          54
#define INNATE_SHADOW_DOOR             55
#define INNATE_GOD_CALL                56
#define INNATE_FOREST_SIGHT            57
#define INNATE_BATTLE_RAGE             58
#define INNATE_DAMAGE_SPREAD           59
#define INNATE_TROLL_SKIN              60
#define INNATE_DAYVISION               61
#define INNATE_SPELL_ABSORB            62
#define INNATE_VULN_FIRE               63
#define INNATE_VULN_COLD               64
#define INNATE_EYELESS                 65
#define INNATE_WILDMAGIC               66
#define INNATE_KNIGHT                  67
#define INNATE_SENSE_WEAKNESS          68
#define INNATE_ACID_BLOOD              69
#define INNATE_CONJURE_WATER           70
#define INNATE_BARTER                  71
#define INNATE_WEAPON_IMMUNITY         72
#define INNATE_MAGIC_RESISTANCE        73
#define INNATE_BATTLEAID               74
#define INNATE_PERCEPTION              75
#define INNATE_DAYBLIND                76
#define INNATE_SUMMON_BOOK             77
#define INNATE_QUICK_THINKING          78
#define INNATE_RESURRECTION            79
#define INNATE_IMPROVED_HEAL           80
#define INNATE_GAMBLERS_LUCK           81
#define INNATE_BLOOD_SCENT             82
#define INNATE_UNHOLY_ALLIANCE         83
#define INNATE_MUMMIFY                 84
#define INNATE_FPRESENCE               85
#define INNATE_BLINDSINGING            86
#define INNATE_IMPROVED_FLEE           87
#define INNATE_ECHO                    88
#define INNATE_BRANCH                  89
#define INNATE_WEBWRAP                 90
#define INNATE_SUMMON_IMP              91
#define INNATE_HAMMER_MASTER           92
#define INNATE_AXE_MASTER              93
#define INNATE_GAZE                    94
#define INNATE_EMBRACE_DEATH           95
#define INNATE_LIFEDRAIN               96
#define INNATE_IMMOLATE                97
#define INNATE_VULN_SUN                98
#define INNATE_DECREPIFY               99
#define INNATE_GROUNDFIGHTING         100
#define INNATE_BOW_MASTERY            101
#define INNATE_SUMMON_WARG            102
#define INNATE_HATRED                 103
#define INNATE_EVASION                104
#define INNATE_DRAGONMIND             105
#define INNATE_SHIFT_ETHEREAL         106
#define INNATE_ASTRAL_NATIVE          107
#define INNATE_TWO_DAGGERS            108
#define INNATE_HOLY_LIGHT             109
#define INNATE_COMMAND_AURA           110
#define INNATE_DECEPTIVE_FLEE         111
#define INNATE_MINER                  112
#define INNATE_FOUNDRY                113
#define INNATE_FADE                   114
#define INNATE_IMPROVED_WORMHOLE      115
#define INNATE_LAY_HANDS              116
#define INNATE_HOLY_CRUSADE           117
#define MAGICAL_REDUCTION             118
#define INNATE_AURA_PROTECTION        119
#define INNATE_AURA_PRECISION         120
#define INNATE_AURA_BATTLELUST        121
#define INNATE_AURA_ENDURANCE         122
#define INNATE_AURA_HEALING           123
#define INNATE_AURA_VIGOR             124
#define INNATE_HASTE                  125
#define INNATE_DAUNTLESS              126
#define INNATE_SUMMON_TOTEM           127
#define INNATE_ENTRAPMENT             128
#define INNATE_PROT_COLD              129
#define INNATE_PROT_ACID              130
#define INNATE_FIRE_AURA              131
#define INNATE_SPAWN                  132
#define INNATE_WARCALLERS_FURY        133
#define INNATE_RRAKKMA                134
#define INNATE_DISEASED_BITE          135
#define INNATE_DIVINE_FORCE           136
#define INNATE_UNDEAD_FEALTY          137
#define INNATE_CALL_GRAVE             138
#define INNATE_SACRILEGIOUS_POWER     139
#define INNATE_BLUR                   140
#define INNATE_RAPIER_DIRK            141
#define INNATE_ELEMENTAL_BODY         142
#define INNATE_AMORPHOUS_BODY         143
#define INNATE_ENGULF                 144
#define INNATE_SLIME                  145
#define INNATE_DUAL_WIELDING_MASTER   146
#define INNATE_SPEED                  147
#define INNATE_ICE_AURA               148
#define INNATE_REQUIEM                149
#define INNATE_ALLY                   150
#define INNATE_SUMMON_HOST            151
#define INNATE_SPIDER_BODY            152
#define INNATE_SWAMP_SNEAK            153
#define INNATE_CALMING                154
#define INNATE_LONGSWORD_MASTER       155
#define INNATE_MELEE_MASTER           156
#define INNATE_GUARDIANS_BULWARK      157
#define INNATE_WALL_CLIMBING          158
#define INNATE_WOODLAND_RENEWAL       159
#define INNATE_NATURAL_MOVEMENT       160
#define MAGIC_VULNERABILITY           161
#define TWO_HANDED_SWORD_MASTERY      162
#define HOLY_COMBAT                   163
#define INNATE_GIANT_AVOIDANCE        164
#define INNATE_SEADOG                 165
#define INNATE_AURA_SPELL_PROTECTION  166
#define INNATE_VISION_OF_THE_DEAD     167
#define INNATE_REMORT                 168
#define INNATE_ELEMENTAL_POWER        169
#define INNATE_INTERCEPT              170
#define INNATE_DET_SUBVERSION         171
#define INNATE_LIVING_STONE           172
#define INNATE_INVISIBILITY           173

#define LAST_INNATE                   173   // LAST means last, not last + 1 or whatever

struct extra_descr_data {
  char *keyword;                /* Keyword in look/examine          */
  char *description;            /* What to see                      */
  struct extra_descr_data *next;/* Next in list                     */
};

#define OBJ_NOTIMER    -7000000

struct obj_affected_type {
  byte location;                /* Which ability to change (APPLY_XXX) */
  sbyte modifier;               /* How much it changes by              */
};

struct obj_affect {
  sh_int type;
  sh_int data;
  ulong extra2;
  struct obj_affect *next;
};

struct arti_data {
  int    vnum;          // Vnum of the artifact
  bool   owned;         // Whether the artifact's timer is currently countinng down.
  char   locType;       // ARTIFACT_NOTINGAME, ARTIFACT_ON_NPC, ARTIFACT_ON_PC, ARTIFACT_ONGROUND, or ARTIFACT_ONCORPSE
  int    location;      // The number corresponding to the location of the arti (vnum of mob|room/pid of PC/etc).
  time_t timer;         // The current time when the artifact will poof.
  char   type;          // ARTIFACT_IOUN / ARTIFACT_UNIQUE / ARTIFACT_MAIN
  P_arti next;          // Used for lists of P_arti.  Usually NULL.
};

/* ======================== Structure for object ========================= */
struct obj_data {
  long g_key;                   /* Key generated upon creation     */
  int R_num;                    /* Where in data-base               */
  byte type;                    /* Type of item                     */
  byte str_mask;                /* for 'strung' char * fields       */
  char *name;                   /* Title of object :get etc.        */
  char *description;            /* When in room                     */
  char *short_description;      /* when worn/carry/in cont.         */
  char *action_description;     /* What to write when used          */

  struct extra_descr_data
   *ex_description;             /* extra descriptions               */

  int value[NUMB_OBJ_VALS];     /* Values of the item (see    list) */
  time_t timer[6];              /* For objects that have powers per day */
  unsigned int wear_flags;             /* Where you can wear it            */
  unsigned int extra_flags;            /* If it hums, glows etc            */
  unsigned int anti_flags;             /* Specific anti-class and race     */
  unsigned int anti2_flags;
  unsigned int extra2_flags;           /* Range weapon-related flags       */
  int weight;                   /* Weight, what else                */
  byte material;                /* size of object (for race)        */
  int cost;                     /* Value when sold (cp.)            */
  sh_int trap_eff;              /* trap effect type                 */
  sh_int trap_dam;              /* trap damage type                 */
  sh_int trap_charge;           /* trap charges                     */
  sh_int trap_level;
  sh_int condition;             /* items condition or level         */
  sh_int craftsmanship;         /* how well made item is            */
  sh_int z_cord;                /* where in the room (up/down)      */

  unsigned int bitvector;              /* To set chars bits                */
  unsigned int bitvector2;             /* 2nd 32 bits                      */
  unsigned int bitvector3;
  unsigned int bitvector4;
  unsigned int bitvector5;

  struct obj_affected_type
    affected[MAX_OBJ_AFFECT];   /* Which abilities in PC to change  */
  struct obj_affect *affects;

  byte loc_p;                   /* bitfield for loc union           */
  union {
    P_char carrying;            /* character carrying object        */
    P_char wearing;             /* character wearing object         */
    P_obj inside;               /* object's container               */
    int room;                   /* R_num of room it's in            */
  }
  loc;

  P_char hitched_to;            /* Who are we hitched to?           */
  P_event events;               /* events attached to this obj      */
  P_nevent nevents;
  P_obj contains;               /* Contains objects                 */
  P_obj next_content;           /* For 'contains' lists             */
  P_obj prev, next;             /* For the object list              */
};

/* ======================================================================= */

typedef void (*spell_func)(int, P_char, char*, int, P_char, P_obj);

struct spellcast_datatype {
  P_obj object;
  char *arg;
  int timeleft;
  int spell;
  unsigned char flags;
};

struct scribing_data_type {
  P_char ch;
  P_obj book;
  union {
    P_obj obj;
    P_char teacher;
  }
  source;

  int spell;
  int pagetime;                 /* in 1/4 seconds, RL / 1 pulses - time to
                                   complete one page of the spell */
  int page;                     /* page number last written - for event-wise
                                   way we do these things nowadays. */
  int flag;                     /* 0, 1, or 2. 0=teach, 1=spellbook, 2=scroll */
  void (*done_func) (P_char);  // optional function to call when done scribing.  used by 'scribe all'
};

#define SBOOK_MODE_AT_HAND   BIT_1
#define SBOOK_MODE_IN_INV    BIT_2
#define SBOOK_MODE_NO_SCROLL BIT_3
#define SBOOK_MODE_NO_BOOK   BIT_4
#define SBOOK_MODE_ON_BELT   BIT_5
#define SBOOK_MODE_ON_GROUND BIT_6

struct climate {
  char season_wind[MAX_SEASONS];
  char season_wind_dir[MAX_SEASONS];
  char season_wind_variance[MAX_SEASONS];
  char season_precip[MAX_SEASONS];
  char season_temp[MAX_SEASONS];
  char flags;
  signed int energy_add;
};

struct weather_data {
  signed char temp;
  signed char humidity;
  signed char precip_rate;
  int windspeed;
  char wind_dir;
  int pressure;                 /* Kept from previous system */
  char ambient_light;           /* Interaction between sun, moon, */
  /* clouds, etc. Local lights ignored */
  int free_energy;
  char flags;
  char precip_depth;            /* Snowpack, flood level */
  char pressure_change, precip_change;
};

/* structure for map sectors specifics */
struct sector_data {
  struct climate climate;
  struct weather_data conditions;
};

/* structure for the reset commands */
struct reset_com {
  char command;                 /* current command                      */
  bool if_flag;                 /* if TRUE: exe only if preceding exe'd */
  int arg1;                     /*                                      */
  int arg2;                     /* Arguments to the command             */
  int arg3;                     /*                                      */
  int arg4;

  /*
   *  Commands:              *
   *  'M': Read a mobile     *
   *  'O': Read an object    *
   *  'G': Give obj to mob   *
   *  'P': Put obj in obj    *
   *  'G': Obj to char       *
   *  'E': Obj to char equip *
   *  'D': Set state of door *
   */
};

/* zone definition structure. for the 'zone-table'   */
struct zone_data {
  int number;                   /* virtual zone number. first room vnum / 100 */
  char *name;                   /* name of this zone                  */
  char *filename;               /* name of file that contains this zone ('base' name, no extension) */
  int lifespan;                 /* current (minutes) in this reset */
  int lifespan_min;             /* minimum age (minutes) before reset */
  int lifespan_max;             /* maximum age (minutes) before reset */
  int fullreset_lifespan;       /* current (hours) in this reset      */
  int fullreset_lifespan_min;   /* minimum age (hours) before full reset */
  int fullreset_lifespan_max;   /* maximum age (hours) before full reset */
  int age;                      /* current age of this zone (minutes) */
  int fullreset_age;            /* current full reset age of this zone (hours) */
  int top;                      /* upper limit for rooms in this zone */
  int mapx, mapy;               /* size of map (in rooms) */

  int difficulty;             /* for random obj gen, maybe other stuff */
  ubyte flags;

  int real_top, real_bottom;
  int reset_mode;               /* conditions for reset (see below)   */
  struct reset_com *cmd;        /* command table for reset              */
  int status;
  /*
   *  Reset mode:                              *
   *  0: Don't reset, and don't update age.    *
   *  1: Reset if no PC's are located in zone. *
   *  2: Just reset.                           *
   */
  int hometown;  /* should default to 0, else be a number from 0 to
                    LAST_HOME */
  char *owner;
  int avg_mob_level;

  u_short players[MAX_RACEWAR + 1];
  bool misfiring[MAX_RACEWAR + 1];
};

struct continent_misfire_data
{
  u_short players[NUM_CONTINENTS][MAX_RACEWAR + 1];
  bool misfiring[NUM_CONTINENTS][MAX_RACEWAR + 1];
};

struct misfire_properties_struct
{
  ushort zoning_maxGroup;
  ushort pvp_maxAllies[MAX_RACEWAR + 1];
  ushort pvp_recountDelay; // in seconds
  ushort pvp_minChance;
  ushort pvp_chanceStep;
  ushort pvp_maxChance;
};

struct town {
  int  resources;
  int  defense;
  int  offense;

  bool deploy_guard;
  bool deploy_cavalry;
  bool deploy_portals;

  int  guard_vnum;
  int  guard_max;
  int  guard_load_room;

  int  cavalry_vnum;
  int  cavalry_max;
  int  cavalry_load_room;

  int  portal_vnum;
  int  portal_load_room;

  struct zone_data *zone;

  town *next_town;
};

typedef struct town *P_town;

struct table_element {
   int weight;
   int virtual_number;
};
typedef struct table_element *P_table_element;

struct table_data {
  int virtual_number; /* number for table */
  int weight;  /* total weight for this table */
  int empty_weight;
  int entries;  /* number of entries */
  P_table_element table;
};

typedef struct table_data *P_table;

typedef int (*mob_proc_type) (P_char, P_char, int, char *);
typedef int (*obj_proc_type) (P_obj, P_char, int, char *);
typedef int (*qst_func_type) (P_char, P_char, int, char *);

/* element in monster and object index-tables   */
struct index_data {
  int virtual_number;                  /* virtual number of this mob/obj */
  int number;                   /* number of existing units of this mob/obj */
  int limit;                    /* maximum number of this this mob/obj allowed in zone file */
  long pos;                     /* file position of this field              */

  /* these strings are used to share RAM, all mobs/objs desc pointers will
     point here.  Reason being, by placing them here, they never have to be
     freed, simplifies things immensely. -JAB */
  char *keys;                   /* mob/obj keywords                       */
  char *desc1;                  /* mob long_descr/obj description         */
  char *desc2;                  /* mob short_descr/obj short_description  */
  char *desc3;                  /* mob description/obj action_description */

  union {
    mob_proc_type mob;
    obj_proc_type obj;
  } func;                       /* special procedure for this mob/obj       */

  /* quest procedure for this mob         */
  qst_func_type qst_func;
};

typedef struct index_data *P_index;

/* for queueing zones for update   */
struct reset_q_element {
  int zone_to_reset;            /* ref to zone_data */
  struct reset_q_element *next;
};

/* structure for the update queue     */
struct reset_q_type {
  struct reset_q_element *head;
  struct reset_q_element *tail;
};

/* commented out by Weebler
struct help_index_element {
  char *keyword;
  long pos;
};
*/

struct info_index_element {
  char *keyword;
  long pos;
};

/* EMAIL registration defs */
struct registration_node {
  char host[80];
  char login[20];
  char name[20];
  struct registration_node *next;
};

typedef struct registration_node *P_ereg;


struct trackrecordtype {
  sbyte dir,from;                    /* direction; 6 = death has happened
here */
  ubyte race, weight, height;   /* info about one who left the track */
  char *name;

  struct trackmainttype *maint;
  /* Next in room / whole linked list */
  struct trackrecordtype *next_track, *next, *prev_track, *prev;
};

struct trackmainttype {
  struct trackrecordtype *first, *last;
  short tracks_in_room;
  int in;
};

struct troop_info_rec {
  int kingdom_num;
  int troops[NUM_TROOP_TYPES][NUM_TROOP_LEVELS][NUM_TROOP_OFFENSE][NUM_TROOP_DEFENSE];
  int in_room;
  int time_of_occupation;
  int last_movement;
  short is_acquiring_land;
  struct troop_info_rec *is_fighting;
  struct troop_info_rec *next;
};

struct room_direction_data {
  int to_room;                  /* Where direction leeds (NOWHERE) */
  int key;                      /* Key's number (-1 for no key)    */
  char *general_description;    /* When look DIR.                  */
  char *keyword;                /* for open/close                  */
  sh_int exit_info;             /* Exit info                       */
};

typedef int (*room_proc_type) (int, P_char, int, char *);

/* ========================= Structure for room ========================== */
struct room_data {
  int number;                                         /* Room number                        */
  ush_int zone;                                       /* Room zone (for resetting)          */
  byte sector_type;                                   /* sector type (move/hide)            */
  int continent;
  char *name;                                         /* Rooms name 'You are ...'           */
  char *description;                                  /* Shown when entered                 */
  struct extra_descr_data *ex_description;            /* for examine/look                   */
  struct room_direction_data *dir_option[NUM_EXITS];  /* Directions                         */
  ulong room_flags;                                   /* DEATH, DARK ... etc                */
//  ulong resources;                                  /* what resources the room contains   */
//  ubyte kingdom_num;                                /* matches guild or town number       */
//  ubyte kingdom_type;                               /* town, pc, npc                      */
  byte light;                                         /* Number of lightsources in room     */
  byte justice_area;
  byte chance_fall;
  byte current_speed;
  byte current_direction;

  /* special procedure */
  room_proc_type funct;

  P_obj contents;               /* List of items in room              */
  P_char people;                /* List of NPC / PC in room           */

  struct room_affect *affected;

  struct trackmainttype *track;
//  struct troop_info_rec *troop_info;
  int bfs_mark;
  sh_int x_coord;
  sh_int y_coord;
  sh_int z_coord;
  sh_int map_section;
};


/* For players : specials.act */
#define PLR_BRIEF        BIT_1  /* Do not show long desc on a do_look    */
#define PLR_NOSHOUT      BIT_2  /* Like auction, etc, but with a !       */
#define PLR_COMPACT      BIT_3  /* Do not put in extra carriage return   */
#define PLR_DONTSET      BIT_4  /* Dont EVER set (ACT_ISNPC for mobs)    */
#define PLR_NAMES        BIT_5  /* If on, player sees new player names   */
#define PLR_PETITION     BIT_6  /* If on, player can buzz god            */
#define PLR_GCC          BIT_7  /* If on, player hears gcc channel       */
#define PLR_WIZLOG       BIT_8  /* For wizards. To receive system msgs   */
#define PLR_STATUS       BIT_9  /* deaths, logon, logoff */
#define PLR_MAP          BIT_10
#define PLR_VICIOUS      BIT_11
#define PLR_ECHO         BIT_12
#define PLR_SNOTIFY      BIT_13
#define PLR_NOTELL       BIT_14
#define PLR_DEBUG        BIT_15
#define PLR_ANONYMOUS    BIT_16
#define PLR_AGGIMMUNE    BIT_17
#define PLR_WIZMUFFED    BIT_18
#define PLR_NOWHO        BIT_19 /* player doesn't show up on who */
#define PLR_PAGING_ON    BIT_20
#define PLR_VNUM         BIT_21
#define PLR_OLDSMARTP    BIT_22 /* old-style smartprompt */
#define PLR_AFK          BIT_23
#define PLR_SMARTPROMPT  BIT_24
#define PLR_SILENCE      BIT_25 /* Player has been silenced.               */
#define PLR_FROZEN       BIT_26 /* Player has been frozen. (no cmd input)  */
#define PLR_PLRLOG       BIT_27 /* toggle for displaying login/out spam to wizards */
#define PLR_MAIL         BIT_28
#define PLR_WRITE        BIT_29
#define PLR_MORTAL       BIT_30 /* removes is_trusted   */
#define PLR_MORPH        BIT_31
#define PLR_BAN          BIT_32

/* For players : specials.act2 */
#define PLR2_NOLOCATE      BIT_1  /* noping */
#define PLR2_NOTITLE       BIT_2  /* don't show player titles */
#define PLR2_BATTLEALERT   BIT_3  /* battle highlighting */
#define PLR2_KINGDOMVIEW   BIT_4  /* show kingdom land on map */
#define PLR2_SHIPMAP       BIT_5  /* show maps while ships move*/
#define PLR2_NOTAKE        BIT_6  /* do not accept items from players */
#define PLR2_TERSE         BIT_7  /* terse battle mode (anti-spam) */
#define PLR2_QUICKCHANT    BIT_8
#define PLR2_RWC           BIT_9  /* Race War Char */
#define PLR2_PROJECT       BIT_10 /* Race War Char */
#define PLR2_NPC_HOG       BIT_11 /* Race War Char */
#define PLR2_UNUSED_1      BIT_12 /* Specialized Class */
#define PLR2_NCHAT         BIT_13
#define PLR2_HARDCORE_CHAR BIT_14
#define PLR2_DAMAGE        BIT_15
#define PLR2_B_PETITION    BIT_16 // Block petition channel
#define PLR2_BOON          BIT_17
#define PLR2_NEWBIEEQ      BIT_18
#define PLR2_SHOW_QUEST    BIT_19
#define PLR2_SPEC_TIMER    BIT_20
#define PLR2_HEAL          BIT_21
#define PLR2_BACK_RANK     BIT_22
#define PLR2_LGROUP        BIT_23
#define PLR2_EXP           BIT_24
#define PLR2_SPEC          BIT_25
#define PLR2_MELEE_EXP     BIT_26
#define PLR2_HINT_CHANNEL  BIT_27
#define PLR2_WEBINFO       BIT_28
#define PLR2_WAIT          BIT_29
#define PLR2_NEWBIE        BIT_30
#define PLR2_NEWBIE_GUIDE  BIT_31
#define PLR2_ACC           BIT_32

/* For players : specials.act3 */
#define PLR3_FRAGLEAD      BIT_1  /* FragList Leader */
#define PLR3_FRAGLOW       BIT_2  /* Lowest Fragger */
#define PLR3_GUILDNAME     BIT_3  // Whether or not they see guild name when using gcc.
#define PLR3_SURNAMES      BIT_4  // Toggle for whether or not to display surnames to the player.

#define SET_SURNAME( ch, number ) (SET_BIT(ch->specials.act3, number) )
#define SURNAME_MASK (BIT_12 | BIT_11 | BIT_10 | BIT_9 | BIT_8 | BIT_7 | BIT_6 | BIT_5)

// Start by removing the first 3 bits, then lop off the bits after 8 bits.
#define GET_SURNAME( ch ) ( (ch->specials.act3) & SURNAME_MASK )
#define CLEAR_SURNAME( ch ) ( ch->specials.act3 = (ch->specials.act3 & ~SURNAME_MASK) )
// The leaderboard-point based surnames get one 3 bit set.
#define SURNAME_SERF      (BIT_5)                 // 001
#define SURNAME_COMMONER  (BIT_6)                 // 010
#define SURNAME_KNIGHT    (BIT_6 | BIT_5)         // 011
#define SURNAME_NOBLE     (BIT_7)                 // 100
#define SURNAME_LORD      (BIT_7 | BIT_5)         // 101
#define SURNAME_KING      (BIT_7 | BIT_6)         // 110
// The 'special' surnames get their own set.
#define SURNAME_LIGHTBRINGER      (                                   BIT_8)  // 00001
#define SURNAME_DRAGONSLAYER      (                           BIT_9        )  // 00010
#define SURNAME_DOCTOR            (                           BIT_9 | BIT_8)  // 00011
#define SURNAME_SERIALKILLER      (                  BIT_10                )  // 00100
#define SURNAME_GRIMREAPER        (                  BIT_10 |         BIT_8)  // 00101
#define SURNAME_DECEPTICON        (                  BIT_10 | BIT_9        )  // 00110
#define SURNAME_TOUGHGUY          (                  BIT_10 | BIT_9 | BIT_8)  // 00111
#define SURNAME_DEMONSLAYER       (         BIT_11                         )  // 01000
// MAX_SURNAME:                   (BIT_12 | BIT_11 | BIT_10 | BIT_9 | BIT_8)  // 11111
// Do not use bits 5 through 12 .. These are for the surnames.

#define PLR3_NOBEEP        BIT_17
#define PLR3_UNDERLINE     BIT_18
#define PLR3_NOLEVEL       BIT_19
#define PLR3_EPICWATCH     BIT_20 /* For Immortals: displays calls to epiclog */
#define PLR3_PET_DAMAGE    BIT_21

/* For players : Prompt flags (16 bits max) */
#define PROMPT_NONE        BIT_1
#define PROMPT_HIT         BIT_2
#define PROMPT_MAX_HIT     BIT_3
#define PROMPT_MANA        BIT_4
#define PROMPT_MAX_MANA    BIT_5
#define PROMPT_MOVE        BIT_6
#define PROMPT_MAX_MOVE    BIT_7
#define PROMPT_TANK_COND   BIT_8
#define PROMPT_TANK_NAME   BIT_9
#define PROMPT_ENEMY       BIT_10
#define PROMPT_ENEMY_COND  BIT_11
#define PROMPT_VIS         BIT_12
#define PROMPT_TWOLINE     BIT_13
#define PROMPT_STATUS      BIT_14

/* which parameters should be ignored in calculations of exp_gained */
#define EXP_DAMAGE       1
#define EXP_HEALING      2
#define EXP_KILL         3
#define EXP_DEATH        4
#define EXP_QUEST        5
#define EXP_RESURRECT    6
#define EXP_MELEE        7
#define EXP_WORLD_QUEST  8
#define EXP_TANKING      9
#define EXP_BOON        10

struct racial_data_type {
  sh_int base_age, base_vitality, base_mana, max_mana, hp_bonus, max_age;
};

/* for mob's memory system. */
struct AC_Memory {
//  char *names;
  unsigned int pcID;
  struct AC_Memory *next;
};

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
  sh_int second, minute, hour, day, month, year;
};

/* This is the track info*/
 struct track_info{
   char *name;
   int dir;
   bool found;
 };



/* These data contain information about a players time data */
struct time_data {
  time_t birth;                 /* This represents the characters age    */
  time_t logon;                 /* Time of the last logon                */
  time_t saved;                 /* Time of the last save                 */
  sh_int perm_aging;            /* permanent 'unnatural' aging */
  sh_int age_mod;               /* temporary 'unnatural' aging */
  uint played;                  /* accumulated time played in secs       */
};

struct char_player_data {
  char *name;                   /* PC / NPC s name (kill ...  )         */
  char *short_descr;            /* for 'actions'                        */
  char *long_descr;             /* for 'look'.. Only here for testing   */
  char *description;            /* Extra descriptions                   */
  char *title;                  /* title                                */
  ubyte sex;                    /* sex                                  */
  unsigned int m_class;           /* class                                */
  ubyte spec;
  ubyte race;                   /* race                                 */
  ubyte racewar;                   /* race                                 */
  ubyte phys_type;              /* physiology type                      */
  ubyte level;                  /* level                                */
  ubyte secondary_level;                  /* level                                */
  unsigned int secondary_class;                    /* class                                */
  int hometown;                 /* PCs Hometown (last saved room)       */
  int birthplace;               /* birth room                           */
  int orig_birthplace;          /* original birth room                  */
  struct time_data time;        /* Age                                  */
  ush_int weight;               /* weight                               */
  ush_int height;               /* height                               */
  byte size;                    /* size T,S,M,L,H,G                     */
};

struct player_disguise_data {
  bool active_pc;               /* is he disguise                       */
  bool active_npc;
  bool active_illusion;
  bool active_shapechange;
  uint m_class;
  char *name;                   /* PC name                              */
  ubyte race;                   /* race                                 */
  ubyte level;                  /* level                                */
  char *title;                  /* title                                */
  char *longname;               /*long name for NPC                     */
  int hit;                      /* disguise hit points                  */
  int racewar;                  /* racewar                              */
};

struct stat_data {
  sh_int Str;  /* 4 physical stats */
  sh_int Dex;
  sh_int Agi;
  sh_int Con;

  sh_int Pow;  /* 4 mental stats */
  sh_int Int;
  sh_int Wis;
  sh_int Cha;

  sh_int Kar;  /* 2 'special' stats */
  sh_int Luk;
};

struct char_point_data {
  /* base is unmodified, this is the value that is saved for players */
  int base_hit;
  sh_int base_mana;
  sh_int base_vitality;

  /* these 3 are the current values */
  int hit;
  sh_int mana;
  sh_int vitality;

  /* these 3 are base + modifiers, and are used as a limit */
  int max_hit;
  sh_int max_mana;
  sh_int max_vitality;

  /* values for storing damage done to each body part - maximums are
     determined by max hp of player - body location stuff is currently
     in new_combat.h */

#ifdef NEW_COMBAT
  sh_int *location_hit;  /* dynamically allocated based on race - no need to
                            store numb of elements since it's static for race type */
#endif

  sh_int delay_move;            /* for out of breath stuff */
#if 1
  sh_int base_armor;            /* Mainly for mobs, PC is always 100  */
  sh_int curr_armor;            /* current armor class  */
#endif
  int cash[4];                  /* Money carried  */
  int curr_exp;                 /* The current experience of the player       */

  byte damnodice;               /* The number of damage dice               */
  byte damsizedice;             /* The size of the damage dice             */
  int base_hitroll;             /* base hit roll, 0 for PCs    */
  int base_damroll;             /* base damage roll, 0 for PCs */
  int hitroll;                  /* Any bonus or penalty to the hit roll    */
  int damroll;                  /* Any bonus or penalty to the damage roll */
  int hit_reg;                  /* bonus added to basic regen rate */
  sh_int move_reg;              /* bonus added to basic regen rate */
  sh_int mana_reg;              /* bonus added to basic regen rate */
  sh_int spell_pulse;           /* modifiers to spellcasting speed */
  sh_int combat_pulse;          /* modifiers to combat speed */
};

struct char_skill_data {
  byte learned;                 /* % chance for success 0 = not learned   */
  byte taught;                  /* learned may not pass this    */
};

/* Skill usage limit --TAM 04/16/94 */

struct affected_type {
  sh_int type;                     /* The type of spell that caused this      */
  byte wear_off_message_index;
  int duration;                                 /* For how long its effects will last      */
  uint flags;                                   /* flags describing affect behavior, see AFFTYPE_* defines */
  int modifier;                 /* This is added to apropriate ability     */
  ubyte location;               /* Tells which ability to change(APPLY_XXX)*/
  unsigned int bitvector;       /* Tells which bits to set (AFF_XXX)       */
  unsigned int bitvector2;      /* Tells which bits to set (AFF2_XXX)      */
  unsigned int bitvector3;
  unsigned int bitvector4;
  unsigned int bitvector5;
  struct affected_type *next;
};

struct room_affect {
  int type;                                             /* The type of spell that caused this      */
  int duration;                 /* duration in pulses (~1/4 second) if set
                                                                        tick duration is ignored */
  ulong room_flags;             /* DEATH, DARK ... etc                */
  P_char ch;                                    /* whoever created the affect */
  struct room_affect *next;
};

struct follow_type {
  P_char follower;
  struct follow_type *next;
};

struct trophy_data {
  int kills;
  int vnum;
  struct trophy_data *next;
};

struct char_shapechange_data {
  int mobVnum, timesResearched, lastResearched, lastShapechanged;
  struct char_shapechange_data *next;
};


#define MAX_INTRO 150

#define PC_TIMER_STAT_POOL  0
#define PC_TIMER_FLEE       1
#define PC_TIMER_HEAVEN     2
#define PC_TIMER_AVATAR     3
#define PC_TIMER_SBEACON    4

#define NUMB_PC_TIMERS 10

#define GOOD_HEAVEN_ROOM      1199
#define EVIL_HEAVEN_ROOM      1198
#define UNDEAD_HEAVEN_ROOM    1281
#define NEUTRAL_HEAVEN_ROOM   1197

struct pc_only_data {           /* values only used by PCs        */
  int pid; // replacement for PC's ->nr

  char *poofIn;
  char *poofOut;
  char *poofInSound;
  char *poofOutSound;
/*  char *title;*/

  P_char switched;
  P_char ignored;

  byte wiz_invis;               /* Used by wizard command "vis" */
  ubyte screen_length;          /* adjust paging to fit terminal */
  ubyte echo_toggle;
  ush_int prompt;
  char pwd[40];                 /* 'CRYPT'ed password    */

/* coins in bank */
  int spare1;
  int spare2;
  int spare3;
  int spare4;

  long frags;                 /* Pkill counter                           */
  long oldfrags;                 /* Pkill counter                           */
  long epics;                 /* # of epic points                           */
  long epic_skill_points;
  long spell_bind_used;                //used for skill_spellbind
  sh_int prestige;              /* commoner or lord?                       */
  time_t time_left_guild;       /* time you left guild                     */
  byte nb_left_guild;           /* number of time you left a guild         */
        time_t time_unspecced;
  ulong vote;                                   /* Voting Status -Strav */
  char *last_tell;

  int *gcmd_arr;                /* granted arr, stores granted cmd numbs */
  int numb_gcmd;                /* number of granted cmds */

  ulong law_flags;              /* KNOWN, WANTED, OUTCAST in hometowns */
  int olc_zone;                 /* what are they editing? */
#ifdef OVL
  sh_int ovl_count;
  sh_int ovl_timer;
#endif
  short aggressive;             /* If not -1, PC will attack aggs */
  short wimpy;                  /* If wimpy set, max hp before autoflee */
  struct char_skill_data skills[MAX_SKILLS];    /* Skills                */
  byte spells_memmed[MAX_CIRCLE + 1];
  ubyte talks[MAX_TONGUE];      /* PC's Tongue's 0 for NPC         */
  ubyte highest_level;           /* highest level PC has ever gotten */
  long int introd_list[MAX_INTRO];
  unsigned long int introd_times[MAX_INTRO];
  time_t pc_timer[NUMB_PC_TIMERS];
//  struct trophy_data *trophy;

  vector<struct zone_trophy_data> *zone_trophy;
  
  ulong numb_deaths;            /* number of times player has died */
  struct char_shapechange_data *knownShapes;

  PlayerLog *log;

  //Traffic messure.
  long send_data;
  long recived_data;
  int master_set;
  long int learned_forged_list[MAX_FORGE_ITEMS];

  //WORLD QUEST STUFF
  int quest_active;
  int quest_mob_vnum;
  int quest_type;
  int quest_accomplished;
  int quest_started;
  int quest_zone_number;
  int quest_giver;
  int quest_level;
  int quest_receiver;
  int quest_shares_left;
  int quest_kill_how_many;
  int quest_kill_original;
  int quest_map_room;
  int quest_map_bought;

  long unsigned int last_ip;
  int skillpoints;

};

struct npc_only_data {          /* values only used by NPCs  */
  int R_num; // replacement for NPC's ->nr

        int idnum;                    /* Given only to pets, used for crashsave */
  ulong aggro_flags;            /* Err..  aggro flags */
  ulong aggro2_flags;           /* aggro2 flags, more aggro goodness */
  ulong aggro3_flags;
  ubyte default_pos;            /* Default position                       */
  byte last_direction;          /* The last direction the monster went    */
  ubyte str_mask;               /* flag field for 'strung' char* fields   */
  
  sh_int spec[4] ;            /* for use by various special procs       */
  int value[NUMB_CHAR_VALS];
  sh_int attack_type;           /* barehand attack                        */
  Memory *memory;                 /* Used for memory system */
  int home;
  ush_int law_flags;            /* extensions to specials.act rel. to pkill */
  P_char orig_char;             /* used instead of memory ptr to keep
                                   track of who is controlling the mob */
  int lowest_hit;               /* lowest hitpoints this mob ever reached */
//  P_mprog_list mpact;
//  int mpactnum;
};

#define SECS_BETWEEN_AFF_REFRESH  60    /* RL seconds between each refresh */

struct char_special_data {
  unsigned int affected_by;     /* Bitvector for spells/skills affected by */
  unsigned int affected_by2;    /* Bitvector for spells/skills affected by */
  unsigned int affected_by3;
  unsigned int affected_by4;
  unsigned int affected_by5;

  byte x_cord;                  /* Sub-coordinate of large room            */
  byte y_cord;
  byte z_cord;                  /* hieght for flyers                       */

  ubyte position;               /* posture and status                      */
  unsigned int act;             /* flags for NPC behavior                  */
  unsigned int act2;            /* extra toggles - Zod                     */
  unsigned int act3;            /* achievement flags - Drannak             */

  float         base_combat_round;
  int           combat_tics;
  float         damage_mod;

  P_Guild guild;                /* which guild?                            */
  unsigned int guild_status;    /* rank, how you enter, etc.               */

  int carry_weight;             /* Carried weight                          */
  ush_int carry_items;          /* Number of items carried                 */
  int was_in_room;              /* previous room char was in               */
  byte apply_saving_throw[5];   /* Saving throw (Bonuses)                  */
  byte conditions[5];           /* Drunk full etc.                         */
  sh_int alignment;             /* +-1000 for alignments                   */
  int tracking;   /* room you are tracking to*/
  P_char fighting;              /* Opponent                                */
  P_char was_fighting;          /* time to see who I was fighting, mainly for assassin track */
  /* fighting list's next-char pointer thing is good, but too memhoggy
     if we want more combat values.. so.. */
  P_char next_fighting;         /* For fighting list             */
  P_char next_destroying;       /* For destroying list                     */
  P_obj destroying_obj;         /* For destroying objects                  */

  sh_int timer;
  wtns_rec *witnessed;
  P_char arrest_by;
  sh_int time_judge;
  char undead_spell_slots[MAX_CIRCLE+1];
};

/* this is.. semi-simple system. Overall, when attsleft==0 || !ch->specials.combat,
   it inits stuff (does single attack, sets attsleft to atts, and queues
   next event). Every event, it nukes parried_since_last_attack flag.
   repeat 'till enemies dead, we fled, or ... -Torm 11/94 */

#ifdef REALTIME_COMBAT
struct combat_data {
  int atts;
  int attsleft;
  int begpulse;
  bool dodged_this_round, parried_since_last_attack, dW;

  P_char next;
};

#endif


// Types of Links Objects can have to char data.  - Dalreth

#define LINKED_VEHICLE_PULL   1
#define LINKED_VEHICLE_PUSH   2
#define LINKED_DRAGGING       3
#define LINKED_FOLLOWING      4

class LINKED_OBJECTS {
        public:
        LINKED_OBJECTS *next;
        int type;
        P_obj object;
        
        LINKED_OBJECTS(P_obj o, int t, LINKED_OBJECTS *append );
        ~LINKED_OBJECTS();
        
        int Visible_Type(); // Returns link type 
        char *Visible_Message(); // Returns link type message 
  P_obj Visible_Object(); // Returns pointer to visibly linked object
};      

void remove_linked_object(P_obj o);
int get_object_link_type(P_obj o);
bool has_linked_object(P_char ch, P_obj o);
void add_linked_object(P_char c,P_obj o,int t);
void remove_all_linked_objects(P_char c);



/* ================== Structure for player/non-player ===================== */

/* only the most basic values go into this struct, plus pointers to anything
   else.  Most values should be organized in substructures, mainly just to
   establish a logical tree of values.  JAB */

struct char_data {
  int in_room;                  /* Location                  */
  byte light;                   /* amount of light emitted by char */

  /* new, removed the pc/npc specific items to their own structs -JAB */
  union {
    struct pc_only_data *pc;
    struct npc_only_data *npc;
  } only;

  P_desc desc;

  P_char next_in_room;          /* For room->people - list       */
  P_char next;                  /* For either mobile | p-list    */

  P_event events;               /* events attached to this char      */
  P_nevent nevents;

  struct char_player_data  player;      /* Normal data               */
  struct player_disguise_data disguise;

  struct stat_data base_stats;
  struct stat_data curr_stats;

  struct char_point_data   points;
  struct char_special_data specials;    /* Special playing constants   */

  struct affected_type *affected;       /* affected by what spells    */

  P_obj equipment[MAX_WEAR];      // Equipment array
  P_obj carrying;                 // Head of list

  LINKED_OBJECTS *lobj;           // carts, wagon, etc
  P_char following;               // Who is char following?
  struct follow_type *followers;  // List of chars followers
  struct group_list *group;       // Points to the head of the group list.
  struct char_link_data *linked;  // Slave links to other characters
  struct char_link_data *linking; // Master links to other characters
  struct char_obj_link_data *obj_linked;
};

/* ======================================================================== */

struct ban_t {
  char *name;
  int lvl;
  char *ban_str;
  struct ban_t *next;
};
struct wizban_t {
  char *name;
  char *ban_str;
  struct wizban_t *next;
};

/* ***********************************************************
*  The following structures are related to descriptor_data   *
*********************************************************** */

struct txt_block {
  char *text;
  struct txt_block *next;
};

struct txt_q {
  struct txt_block *head;
  struct txt_block *tail;
};

/* modes of connectedness */

#define CON_PLAYING                  0
#define CON_NAME                     1
#define CON_NAME_CONF                2
#define CON_PWD_NORM                 3
#define CON_PWD_GET                  4
#define CON_PWD_CONF                 5
#define CON_GET_SEX                  6
#define CON_RMOTD                    7
#define CON_MAIN_MENU                8
// #define CON_PUNTCNF                  9 <-- No longer used?
#define CON_GET_CLASS               10
// #define CON_LDEAD                   11 <-- No longer used?
#define CON_PWD_NEW                 12
#define CON_PWD_NO_CONF             13
#define CON_FLUSH                   14
#define CON_PWD_GET_NEW             15
#define CON_PWD_D_CONF              16
#define CON_GET_RACE                17
#define CON_GET_TERM                18
#define CON_GET_EXTRA_DESC          19
#define CON_GET_RETURN              20
#define CON_DSCLMR1                 21
#define CON_DSCLMR2                 22
#define CON_DSCLMR3                 23
#define CON_DSCLMR4                 24
#define CON_DSCLMR5                 25
#define CON_DSCLMR6                 26
#define CON_DISCLMR                 27
#define CON_SHOW_CLASS_RACE_TABLE   28
#define CON_ACCTINFO                29
#define CON_SHOW_RACE_TABLE         30
// #define CON_LINKVR                  31 <-- Unused.
// #define CON_LINKSET                 32 <-- Unused.
#define CON_APPROPRIATE_NAME        33
#define CON_DELETE                  34
#define CON_REROLL                  35
#define CON_BONUS1                  36
#define CON_BONUS2                  37
#define CON_BONUS3                  38
#define CON_KEEPCHAR                39
#define CON_ALIGN                   40
#define CON_HOMETOWN                41
#define CON_ACCEPTWAIT              42
#define CON_WELCOME                 43
#define CON_NEW_NAME                44
#define CON_HOST_LOOKUP             45 // looking up hostname...
/* These were for online zone editing.. no longer used.
#define CON_OEDIT                   46
#define CON_REDIT                   47
#define CON_ZEDIT                   48
#define CON_MEDIT                   49
#define CON_SEDIT                   50
#define CON_QEDIT                   51
*/
#define CON_BONUS4                  52 // Krov: new 5 bonus system
#define CON_BONUS5                  53
#define CON_TEXTED                  54
// #define CON_PICKSIDE                55 <-- Unused.
#define CON_ENTER_LOGIN             56
#define CON_ENTER_HOST              57
#define CON_CONFIRM_EMAIL           58
#define CON_EXIT                    59
#define CON_GET_ACCT_NAME           60
#define CON_GET_ACCT_PASSWD         61
#define CON_IS_ACCT_CONFIRMED       62
#define CON_DISPLAY_ACCT_MENU       63
#define CON_CONFIRM_ACCT            64
#define CON_VERIFY_NEW_ACCT_NAME    65
#define CON_GET_NEW_ACCT_EMAIL      66
#define CON_VERIFY_NEW_ACCT_EMAIL   67
#define CON_GET_NEW_ACCT_PASSWD     68
#define CON_VERIFY_NEW_ACCT_PASSWD  69
#define CON_VERIFY_NEW_ACCT_INFO    70
#define CON_ACCT_SELECT_CHAR        71
#define CON_ACCT_NEW_CHAR           72
#define CON_ACCT_DELETE_CHAR        73
#define CON_ACCT_DISPLAY_INFO       74
#define CON_ACCT_CHANGE_EMAIL       75
#define CON_ACCT_CHANGE_PASSWD      76
#define CON_ACCT_DELETE_ACCT        77
#define CON_ACCT_VERIFY_DELETE_ACCT 78
#define CON_VERIFY_ACCT_INFO        79
#define CON_ACCT_NEW_CHAR_NAME      80
#define CON_HARDCORE                81
#define CON_NEWBIE                  82
#define CON_SWAPSTATYN              83
#define CON_SWAPSTAT                84

#define TOTAL_CON 85

/* modes of confirmation- SAM 7-94 */
#define CONFIRM_NONE    0
#define CONFIRM_AWAIT   1
#define CONFIRM_DONE    2

typedef struct _snoop_by_data {
  P_char snoop_by;
  struct _snoop_by_data *next;
} snoop_by_data;

struct snoop_data {
  P_char snooping;              /* Who is this char snooping */
  snoop_by_data *snoop_by_list; /* And who is snooping on this char */
};

struct descriptor_data {
  sh_int descriptor;            /* file descriptor for socket */
  char host[50];                /* hostname                   */
  char host2[128];
  char login[9];                /* userid from host           */
  char registered_host[50];
  char registered_login[9];
  byte rtype;                   /* character restore status   */
  byte connected;               /* mode of 'connectedness'    */
  int wait;                     /* wait for how many loops    */
  char *showstr_head;           /* for paging through texts   */
  char **showstr_vector;        /*       -                    */
  int  showstr_count;           /* number of pages to page through      */
  int  showstr_page;            /* which page are we currently showing? */
  char **str;                   /* for the modify-str system  */
  char *backstr;                /* abort buffers              */
  char *storage;                /* file editor holding        */
  int max_str;                  /* -                          */
  char *name;                   /* name for mail system       */
  bool prompt_mode;             /* control of prompt-printing */
  char buf[MAX_QUEUE_LENGTH];   /* buffer for raw input       */
  char last_input[MAX_INPUT_LENGTH];    /* the last input         */
  struct txt_q output;          /* q of strings to send       */
  struct txt_q input;           /* q of unprocessed input     */
  P_char character;             /* linked to char             */
  P_char original;              /* original char              */
  struct snoop_data snoop;      /* to snoop people.           */
  struct olc_data *olc;         /*. OLC info - defined in olc.h   .*/
  P_desc next;                  /* link to next descriptor    */
  char tmp_val;                 /* temporary field used in char creation only */
  char confirm_state;           /* SAM 7-94, used to allow confirming commands */
  byte term_type;               /* terminal type, normal or ansi */
  char last_command[MAX_INPUT_LENGTH];
  P_acct account;
  /* SAM 7-94, used to allow confirming commands */
  char old_pwd[40];             /* old password held here when
                                   changing SAM 7-94 */
  struct edit_data *editor;     /* for new editor code */
  int out_compress;             /* are we compressing output ? */
  char *out_compress_buf;       /* MCCP output buffer */
  z_stream *z_str;           /* zlib internal state */
  int movement_noise            ;
  char client_str[MAX_INPUT_LENGTH];/* CLIENT SPECIFIC STRING */
  int last_map_update           ;/* CLIENT SPECIFIC INT */
  int last_group_update         ;/* CLIENT SPECIFIC INT */
};

struct damage_messages {
        char *attacker;
        char *victim;
        char *room;
        char *death_attacker;
        char *death_victim;
        char *death_room;
        int type;
  P_obj obj;
};

struct msg_type {
  char *attacker_msg;           /* message to attacker */
  char *victim_msg;             /* message to victim   */
  char *room_msg;               /* message to room     */
};

struct message_type {
  struct msg_type die_msg;      /* messages when death            */
  struct msg_type miss_msg;     /* messages when miss             */
  struct msg_type hit_msg;      /* messages when hit              */
  struct msg_type sanctuary_msg;/* messages when hit on sanctuary */
  struct msg_type god_msg;      /* messages when hit on god       */
  struct message_type *next;    /* to next messages of this kind.*/
};

struct message_list {
  int a_type;                   /* Attack type  */
  int number_of_attacks;        /* How many attack messages to chose from. */
  struct message_type *msg;     /* List of messages. */
};

struct str_app_type {
  byte tohit;                   /* To Hit (THAC0) Bonus/Penalty        */
  byte todam;                   /* Damage Bonus/Penalty                */
  sh_int carry_w;               /* Maximum weight that can be carrried */
  sh_int wield_w;               /* Maximum weight that can be wielded  */
};

struct dex_app_type {
  byte reaction;
  byte miss_att;
  byte p_pocket;
  byte p_locks;
  byte traps;
};

struct agi_app_type {
  byte defensive;
  byte sneak;
  byte hide;
};

struct con_app_type {
  byte hitp;
  byte shock;
};

struct int_app_type {
  byte learn;                   /* how many % a player learns a spell/skill */
};

struct wis_app_type {
  byte bonus;                   /* how many bonus skills a player can */
  /* practice pr. level                 */
};

struct cha_app_type {
  byte modifier;
};

/* For spec_procs */

#define SPEC_TYPE_UNDEF  0
#define SPEC_TYPE_DAMAGE 1
#define SPEC_TYPE_DEATH  2

/* Attacktypes with grammar */

struct attack_hit_type {
  const char *singular;
  const char *plural;
  const char *damaged;
};

/** Support minor creation --TAM 2/94 **/
struct minor_create_struct {
  char keyword[64];
  sh_int obj_number;
};

/* ShapeChange: Valid mobs to become-type */
struct shapechange_struct {
  char name[64];                /* mob name */
  sh_int mob_number;
  int animal_type;
};

/* Types */



#define SKILL_CATEGORY_OFFENSIVE BIT_1
#define SKILL_CATEGORY_DEFENSIVE BIT_2

struct ClassSkillInfo {
        ClassSkillInfo() {
                for( int i = 0; i < MAX_SPEC+1; i++ ) {
                        rlevel[i] = 0;
                        maxlearn[i] = 0;
                }
        }
  byte rlevel[MAX_SPEC+1];                  /* level required, for spells, spell circle #*/
  byte maxlearn[MAX_SPEC+1];                /* max % that can be gained */
  byte costmult;
};

struct s_skill {
  const char *name;             /* name of skill */
  const char *wear_off_char[MAX_WEAR_OFF_MESSAGES];
  const char *wear_off_room[MAX_WEAR_OFF_MESSAGES];
  sh_int beats;                 /* cast time */
  unsigned int targets;     /* target types allowed */
  /* func pointer, 0 for skills */
  void (*spell_pointer) (int, P_char, char *, int, P_char, P_obj);
  struct ClassSkillInfo m_class[CLASS_COUNT];      /* info for each class */
//#ifdef SKILLPOINTS
//  int maxtrainwarr;
//  int maxtrainsorc;
//  int maxtrainprst;
//  int maxtrainrogu;
//  int dependency[7];
//  int mintotrain[7];
//  bool specskill;
//#endif
};

#ifndef _PFILE_
typedef struct s_skill Skill;
#endif

struct command_info {
  void (*command_pointer) (P_char, char *, int);
  byte minimum_position;
  bool in_battle;
  byte minimum_level;
  byte req_confirm;
  byte grantable;
  bool check_aggro;  // Check the room for mobs to aggro any ch that executes this command.
};


/* hunt types that use a P_char as a target should be below 127.  Ones
   that use a room as a target should be above 127 */

#define HUNT_HUNTER             1 /* ACT_HUNTER mobs */
#define HUNT_JUSTICE_ARREST     2 /* going to arrest someone */
#define HUNT_JUSTICE_OUTCAST    3 /* going to kill an outcast */
#define HUNT_JUSTICE_INVADER    4 /* going to kill an invader */
#define HUNT_JUSTICE_SPECVICT   5 /* special justice purpose */
/******************************************************************
  All types above use 'victim' as targ */

#define HUNT_LAST_VICTIM_TARGET 127

/* All types below use 'room' as targ
******************************************************************/
#define HUNT_JUSTICE_SPECROOM   128 /* special justice purpose */
#define HUNT_ROOM               129 /* just run to that room */
#define HUNT_JUSTICE_HELP       130 /* call for help from that room */

struct hunt_data {
  ubyte hunt_type;              /* see defines above.. */
  ubyte retry;                  /* counter for retrying moves that
                                   don't work */
  ubyte retry_dir;              /* last failed direction I tried moving */
  long  huntFlags;              // hunt flags passed to find_first_step
  union {
    P_char victim;              /* who am I hunting? */
    int room;                   /* what room am I hunting? */
  }
  targ;
};

struct event_data {
  ubyte type;                   /* type of event triggered:  EVENT_*        */
  short element;               /* element of events[] this is a member of  */
  bool one_shot;                /* if TRUE, event is deleted once triggered */
  ush_int timer;                /* number of cycles (minutes) before event,
                                   if we want to schedule an event longer
                                   than 45 (real) days in advance we'll
                                   have to change this to u_int which will
                                   let us schedule up to 4000 (real) years
                                   in advance.                              */
  union {
    P_char a_ch;                /* one of these will point to the initiator */
    P_obj a_obj;                /* (actor) of this event.  type determines  */
    P_room a_room;              /* which is valid.                          */
    struct trackrecordtype *a_track;
    void (*a_func) (void);
  }
  actor;

  union {
    P_char t_ch;                /* one of these will point at the target of */
    P_obj t_obj;                /* this event (or none, it's optional in    */
    P_room t_room;              /* some cases).  Or if this is a delayed    */
    struct zone_data *t_zone;   /* command of some sort, t_arg will get     */
    char *t_arg;                /* sent to command_interpreter.             */
    int t_num;
    struct scribing_data_type *t_scribe;
    struct spellcast_datatype *t_spell;
    void (*t_func) (P_char);
    struct hunt_data *t_hunt;
    struct generic_event_arguments *t_generic;
    P_event t_event;       /* just to confuse everyone (actually used by EVENT_PEER - Tharkun) */
  }
  target;

  P_event prev_sched;           /* pointer to prev event in schedule[]       */
//  P_event prev_type;            /* pointer to prev event in event_sub_list[] */
  P_event prev_event;           /* pointer to prev event in event_list or
                                   avail_events */
  P_event next_sched;           /* pointer to next event in schedule[]       */
//  P_event next_type;            /* pointer to next event in event_sub_list[] */
  P_event next_event;           /* pointer to next event in event_list or
                                   avail_events */
  P_event next;                 /* pointer to next event on obj or char      */
};

struct nevent_data {
  P_obj obj;
  P_char ch;
  P_char victim;
  event_func_type func;           // What function is called when event fires.
  void *data;                     // Data argument to func
  unsigned int timer;             // How much time in the row.
  unsigned int element;           // Which row of ne_schedule array
  struct char_link_data *cld;
  P_nevent next_char_nev;
  P_nevent next_obj_nev;
  P_nevent prev_sched;
  P_nevent next_sched;
};

/* data structure for justice witness record as held in memory.  This
   struct ends up being a linked list.. */

struct witness_data {
  time_t time;                  /* When did it happen? */
  char *attacker;               /* who did it? */
  char *victim;                 /* who did they do it to? */
  ubyte crime;                  /* what did they do? */
  int room;                     /* Where did they do it?  (VIRTUAL!) */
  wtns_rec *next;               /* next record (or NULL if none) */
};

struct crime_data {
  time_t time;
  char *attacker;
  char *victim;
  ubyte crime;
  int room;
  int money;
  ubyte status;
  crm_rec *next;
};

struct group_formations {
 int formation_id;
 char *formation_name;
 int total_slots;
 int offensive_slots;
 int defensive_slots;
 int free_slots;
 int protected_slots;
};
/* structure used for grouping.. */
struct group_list {
  P_char ch;
  int formation_id;
  int formation_valid;
  struct group_list *next;
};

struct auction_data {
  P_obj item;
  P_char seller;
  P_char buyer;
  int auctioneer; /* virtual number */
  int room;       /* virtual number */
  int bet;
  sh_int going;
  sh_int pulse;
};

extern struct auction_data auction[LAST_HOME];

struct  mob_prog_act_list
{
  P_mprog_list next;
  char *buf;
  P_char ch;
  P_obj obj;
  void *vo;
};

struct  mob_prog_data
{
  P_mprog next;
  int cmd;
  char *arg;
};

#define MP_GREET           1
#define MP_DIE             2
#define MP_SHOUT           3
#define MP_ACT             4
#define MP_SUMMON          5

#define ERROR_PROG        -1
#define IN_FILE_PROG       0
#define ACT_PROG           1
#define SPEECH_PROG        2
#define RAND_PROG          4
#define FIGHT_PROG         8
#define DEATH_PROG        16
#define HITPRCNT_PROG     32
#define ENTRY_PROG        64
#define GREET_PROG       128
#define ALL_GREET_PROG   256
#define GIVE_PROG        512
#define BRIBE_PROG      1024

/* all our log files */

/* general status type logs, not problems */
#define LOG_STATUS "logs/log/status"
#define LOG_COMM "logs/log/comm"
#define LOG_EXP "logs/log/exp"
#define LOG_EXIT "logs/log/exit"
#define LOG_DAEMON "logs/log/daemon"
#define LOG_PORTALS "logs/log/portals"
#define LOG_SHIP "logs/player-log/ship"
#define LOG_RECALL "logs/log/recall"
#define LOG_STEAL "logs/log/steal"
#define LOG_CHAT "logs/log/chat"

/* problem reporting */
#define LOG_ARTIFACT "logs/log/artifact"
#define LOG_DEBUG "logs/log/debug"
#define LOG_FILE "logs/log/file"
#define LOG_SAVED_OBJ "logs/log/saved_items"
#define LOG_SYS "logs/log/sys"
#define LOG_BOARD "logs/log/board"
#define LOG_VEHICLE "logs/log/vehicle"
#define LOG_HOUSE "logs/log/house"
#define LOG_MOB "logs/log/mob"
#define LOG_OBJ "logs/log/obj"
#define LOG_EVENT "logs/log/events"
#define LOG_DONATION "logs/log/donation"
#define LOG_PETITION "logs/log/petition"
#define LOG_EPIC "logs/log/epic"
#define LOG_HELP "lib/etc/help"
#define LOG_CARDGAMES "logs/log/cards"

/* these logs have data of lasting importance, so they are kept longer */
#define LOG_CORPSE "logs/player-log/corpse"
#define LOG_NEW "logs/player-log/new"
#define LOG_DEATH "logs/player-log/death"
#define LOG_LEVEL "logs/player-log/level"
#define LOG_PLAYER "logs/player-log/player"
#define LOG_NEWCHAR "logs/player-log/newchar"

/* what are those damn immorts up to now? */
#define LOG_WIZ "logs/player-log/wizcmds"
#define LOG_FLAG LOG_WIZ
#define LOG_FORCE LOG_WIZ
#define LOG_WIZLOAD LOG_WIZ

#define FRAGLIST_NORMAL         1
#define FRAGLIST_RACEWAR        2
#define FRAGLIST_CLASS          3
#define FRAGLIST_RACE           4

struct mcname {
  int cls1, 
  cls2;
  char *mc_name;
} ;

//#ifndef _NEWSHIP_H_
//  #include "ships.h"
//#endif

#define MIN_SPEC            0
#define SPEC_NONE           0
//Bard Specs
#define SPEC_LOREMASTER     1
#define SPEC_BATTLESEINGER  2
#define SPEC_STORMSINGER    3

//Monk Specs
#define SPEC_WAYOFDRAGON    1
#define SPEC_WAYOFSNAKE     2
#define SPEC_CHIMONK        3


// Dreadlord Specs
#define SPEC_DEATHLORD      1
#define SPEC_SHADOWLORD     2

//Reaver Specs
#define SPEC_ICE_REAVER     1
#define SPEC_FLAME_REAVER   2
#define SPEC_SHOCK_REAVER   3
#define SPEC_EARTH_REAVER   4

//Conj Specs
#define SPEC_AIR            1
#define SPEC_WATER          2
#define SPEC_FIRE           3
#define SPEC_EARTH          4

//Alchemist Specs
#define SPEC_BATTLE_FORGER  1
#define SPEC_BLACKSMITH     2

//Merc Specs
#define SPEC_BRIGAND 1
#define SPEC_BOUNTY 2

//Assassin Specs
#define SPEC_ASSMASTER 1
// #define SPEC_SHARPSHOOTER 2 No way pal, we'll jew this for something else!

// Zerker specs
#define SPEC_MAULER 1
#define SPEC_RAGELORD 2

// Necro specs
#define SPEC_DIABOLIS 1
#define SPEC_NECROLYTE 2
#define SPEC_REAPER 3

//Warrior SPecs
#define SPEC_SWORDSMAN 1
#define SPEC_GUARDIAN  2
#define SPEC_SWASHBUCKLER 3

/*AntiPaladin Specs */
#define SPEC_DARKKNIGHT 1
#define SPEC_DEMONIC 2
#define SPEC_VIOLATOR 3
#define SPEC_SPAWN 4

/* CLASS_PALADIN Specs */
#define SPEC_CRUSADER 1
#define SPEC_CAVALIER 2
/* CLASS_SORCERER Specs */
#define SPEC_WILDMAGE 1
#define SPEC_WIZARD 2
#define SPEC_SHADOW 3

#define SPEC_TRICKSTER 1
#define SPEC_CUTPURSE 2
#define SPEC_ROGUE 3

#define SPEC_ELEMENTALIST 1
#define SPEC_SPIRITUALIST 2
#define SPEC_ANIMALIST    3

#define SPEC_BLADEMASTER 1
#define SPEC_HUNTSMAN    2
#define SPEC_MARSHALL    3

#define SPEC_ZEALOT  1
#define SPEC_HEALER  2
#define SPEC_HOLYMAN 3

//Druid Specs
#define SPEC_WOODLAND 1
#define SPEC_STORM 2
#define LUNAR_DRUID 3

// Ethermancer Specs
#define SPEC_TEMPESTMAGUS 1
#define SPEC_FROSTMAGUS 2
#define SPEC_STARMAGUS 3

// Bard Specs
#define SPEC_DISHARMONIST 1
#define SPEC_SCOUNDREL 2
#define SPEC_MINSTREL 3

// Psi Specs
#define SPEC_PYROKINETIC 1
#define SPEC_ENSLAVER 2
#define SPEC_PSYCHEPORTER 3

//Illu Specs
#define SPEC_DECEIVER 1
#define SPEC_DARK_DREAMER 2

/* CLASS_BLIGHTER Specs */
#define SPEC_STORMBRINGER 1
#define SPEC_SCOURGE      2
#define SPEC_RUINER       3

//Avenger Specs
#define SPEC_LIGHTBRINGER 1
#define SPEC_INQUISITOR  2

//Rogue specs
#define SPEC_ASSASSIN 1
#define SPEC_THIEF    2
#define SPEC_OLD_SWASHBUCKLER  3 // Switched to warrior.
#define SPEC_SHARPSHOOTER 4

/* CLASS_THEURGIST Specs */
#define SPEC_MEDIUM 1
#define SPEC_TEMPLAR 2
#define SPEC_THAUMATURGE 3

//Summoner Specs
#define SPEC_CONTROLLER 1
#define SPEC_MENTALIST  2
#define SPEC_NATURALIST 3

#define MAX_SPEC 4

struct quest_msg_data
{
  char    *key_words;
  char    *message;
  bool     echoAll;
  struct quest_msg_data *next;
};

struct goal_data
{
  char     goal_type;
  int      number;
  struct goal_data *next;
};

struct quest_complete_data
{
  char    *message;
  struct goal_data *receive;
  struct goal_data *give;
  bool     disappear;
  char    *disappear_message;
  bool     echoAll;
  struct quest_complete_data *next;
};

struct quest_data
{
  int      quester;             /* the mob id who is seeking the quest */
  struct quest_msg_data *quest_message;
  struct quest_complete_data *quest_complete;
};

struct proc_data {
        P_char victim;
        int dam;
        int attacktype;
        uint flags;
        struct damage_messages *messages;
};

struct random_spells {
  int s_number;
  int spell;
  bool self_only;
  char name[32];
};

struct random_quest
{
  int z_number;
  char *z_mapdesc;
  int z_justan;
};


struct randomeq_slots
{
  int m_number;
  char *m_name;
  float m_stat;
  float m_ac;
  int wear_bit;
  int numb_material;
  int damage_type;
  int base_weight;
};

struct randomeq_material
{
  int m_number;
  char *m_name;
  float m_stat;
  float m_ac;
  float weight_mod;
};

struct spell_target_data {
  int ttype;
  P_char t_char;
  P_obj t_obj;
  char *arg;
};

typedef void (*link_breakage_func)(struct char_link_data*);

struct char_link_data {
  ush_int type;
  P_char linking;
  P_char linked;
  struct affected_type *affect; // optional, for links dependant affects
  struct char_link_data *next_linking;
  struct char_link_data *next_linked;
};

typedef void (*link_obj_breakage_func)(struct char_obj_link_data*);

struct char_obj_link_data {
  ush_int type;
  P_char  ch;
  P_obj   obj;
  struct affected_type *affect; // optional, for links dependant affects
  struct char_obj_link_data *next;
};

struct link_description {
  char *name;
  union {
    link_breakage_func ch;
    link_obj_breakage_func obj;
  }
  break_func;
  int flags;
};

struct event_short_affect_data {
  P_char ch;
  struct affected_type *af;
};

struct hold_data
{
  int      c_Str, c_Dex, c_Agi, c_Con, c_Pow, c_Int, c_Wis, c_Cha, c_Kar, c_Luc;
  int      m_Str, m_Dex, m_Agi, m_Con, m_Pow, m_Int, m_Wis, m_Cha, m_Kar, m_Luc;
  int      r_Str, r_Dex, r_Agi, r_Con, r_Pow, r_Int, r_Wis, r_Cha, r_Kar, r_Luc;
  int      AC, Age, Dam, Hit, Hits, Fprot, Move, Mana;
  int      S_spell, S_para, S_petri, S_rod, S_breath;
  int      hit_reg;
  sh_int   move_reg, mana_reg;
//  sh_int   spell_pulse, combat_pulse;
  float    spell_pulse, combat_pulse;
  ulong    BV_1, BV_2, BV_3, BV_4, BV_5, BV_6;
};

//-----------------------
struct portal_create_messages {
    char *fail_to_caster;
    char *fail_to_caster_room;
    char *fail_to_victim;
    char *fail_to_victim_room;
    char *open_to_caster;
    char *open_to_caster_room;
    char *open_to_victim;
    char *open_to_victim_room;
    char *npc_target_caster;
    char *bad_target_caster;
};

//-----------------------
struct portal_action_messages {
    char *step_in_to_char;
    char *step_in_to_room;
    char *step_out_to_char;
    char *step_out_to_room;
    char *wait_init_to_char;
    char *wait_to_char;
    char *decay_to_char;
    char *decay_to_room;
    char *bug_to_char;
};

struct portal_settings {
   int R_num;                    /* portal type  */
   int from_room;                /* from room (not used, for future magic maybe) */
   int to_room;                  /* to room (for gate type portals, with no target ) */
   int throughput;               /* How many can pass before closes (-1 means no limit) */
   int init_timeout;             /* Timeout before anyone can enter after open */
   int post_enter_timeout;       /* Timeout before next person can enter */
   int post_enter_lag;           /* Lag person gets when steps out portal */
   int decay_timer;              /* Portal decay timer */
};
//-----------------------

struct vehicle_data {
  int mob;                      /* mob vnum that moves the vehicle */
  int start1;                   /* start room */
  int destination1;             /* destination */
  int destination2;             /* destination */
  int destination3;             /* destination */
  int destination;              /* initially set to zero */
  int move_time;                /* time vehicle starts moving */
  int freq;                     /* after starting, how often repeat? */
};

struct ALLOCATION_HEADER
{
  char   *tag;
  size_t  size;
  char   *file;
  int     line;
  void   *body;
};

struct mem_usage
{
  char   *tag;
  size_t  size;
  size_t  allocs;
};


// this is stub for possible array-based event manager, not used yet  - Odorf
struct EventsFactory
{
    void init_event(nevent_data* event, event_func_type func, P_char ch, P_char victim, P_obj obj, void *data, int data_size);
    P_nevent add_event(nevent_data* &table, nevent_data* &size, nevent_data* &tail, event_func_type func, P_char ch, P_char victim, P_obj obj, void *data, int data_size);
    P_nevent add_global_event(event_func_type func, P_char ch, P_char victim, P_obj obj, void *data, int data_size);
    P_nevent add_local_event(event_func_type func, P_char ch, P_char victim, P_obj obj, void *data, int data_size);
    void expand(nevent_data* &table, nevent_data* &last, nevent_data* &tail);
    void init_table(P_nevent &table, P_nevent &last, P_nevent &tail, int size);
    void init_global_table(int size);
    void init_local_table(int size);

    int get_count() { return (global_tail - global_table) + (local_tail - local_table); }
    int get_size() { return (global_last - global_table) + (local_last - local_table); }
    int get_global_count() { return (global_tail - global_table); }
    int get_global_size() { return (global_last - global_table); }
    int get_local_count() { return (local_tail - local_table); }
    int get_local_size() { return (local_last - local_table); }

    P_nevent get_first_event();
    P_nevent get_next_event(P_nevent);

    nevent_data* global_table;
    nevent_data* global_last;
    nevent_data* global_tail;
    nevent_data* local_table;
    nevent_data* local_last;
    nevent_data* local_tail;
};

// For the shutdown command.
struct TimedShutdownData
{
  time_t  reboot_time;
  int  next_warning;
  enum
  {
    NONE,
    OK,
    REBOOT,
    COPYOVER,
    AUTOREBOOT,
    AUTOREBOOT_COPYOVER,
    PWIPE
  }
  eShutdownType;
  char IssuedBy[50];
};

#define REG_TROLL       0
#define REG_REVENANT    1
#define REG_HUNTSMAN    2
#define REG_WATERMAGUS  3
#define REG_MAX         3

#endif /* _SOJ_STRUCTS_H_ */
