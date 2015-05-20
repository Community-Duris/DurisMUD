  /* ***************************************************************************
 *  File: config.h                                           Part of Duris *
 *  Usage: global configuration options and settings.                        *
 *  Copyright  1995 - Duris Systems Ltd.                                   *
 *************************************************************************** */

#ifndef _SOJ_CONFIG_H
#define _SOJ_CONFIG_H

/* couple of basic things */

#ifndef FALSE
#define FALSE (0)          /* pretty damn basic */
#endif
#ifndef TRUE
#define TRUE (1)           /* pretty damn basic */
#endif

/* default mother socket number */

#define DFLT_PORT 7777          /* default port */
// randomeq constants
#define MAX_SLOT 109
#define MAXMATERIAL 34
#define MAXPREFIX 56
#define RANDOM_EQ_VNUM 1252
/* configurable options */

#define ENABLE_TERSE 1
#define THARKUN_ARTIS 1

#define MEMCHK 1                /* Memory debugger, 1 for simple, 2 for long */
#define DB_NOTIFYoff            /* Logs when it can't find shit in area files */
#define SHIPS_BROKEoff          /* turns em on and off. Virtual ships that is */
#define CONFIG_JAIL             /* are the jails working? */
#undef ERANDOM                  /* enable BSD-sytle pseudo-random nuber generator */
#undef  FIGHT_DEBUG             /* for step by step debug info on fight routines */
#undef  GR                      /* some wierd thing dealing with high system loads */
#define LANGUAGE_CRYPT          /* Scramble unknown spoken languages */
#define MEM_DEBUG               /* detailed tracking of where all the RAM is going */
#define OOC                     /* allow use of global out of context channel */
#undef  OVL                     /* anti-spam code, but adds significant cpu overhead */
#undef  PET_STABLE              /* PET_STABLE allows stabling of pets -NOT YET FINISHED PORT! */
#undef  REALTIME_COMBAT         /* REALTIME_COMBAT is fake-realtime combat replacing old perform_violence */
#define SMART_PROMPT            /* SMART_PROMPT doesn't show prompt constantly (its annoying) */
#undef  SPELL_DEBUG             /* for step by step debug info on spell routines */
#define MISFIRE                 /* mistargeting spells when there are more allies in room than group cape and racewar is going on */
/* default filesystem variables */

#define DFLT_DIR "."                   /* default data directory     */
#define MUDNAME "Duris"                /* name whod shows for the mud */
#define SAVE_DIR "Players"             /* default directory for player save files */
#define BADNAME_DIR "Players/Declined" /* directory containing a list of declined names*/
#define STABLE_DIR "Stables"           /* default directory for pet stables save files */
#define NUM_ANSI_LOGINS 4              /* Current number of ANSI login sequences, max is 5 */

/* balance affecting variables */

/* #define STANDARD             1 */
#define DAMFACTOR               1
#define AREADAMAGEFACTOR                0.6
#define DRAG_COST               5 /* Additional move cost when dragging */
#define MANA_PER_CIRCLE         7 /* mobs still using mana, this much per circle for cost */
#define MAX_DRAG             1.75 /* N times max_carry is how much you can drag */
#define MAX_SKILL_USAGE        50 /* max number of skills with usage timers, PFILE! */
#define MAX_SHADOW_MOVES        5 /* number of 'free' moves before someone MIGHT notice a shadower */
#define MAX_TRACK_DIST         32 /* old limit for tracking */
#define SHADOW_AWARE_PENALTY   20 /* penalty to shadowers skill, if target is 'aware' */
#define MAX_TROPHY_SIZE        75 /* max number of mobs to keep in trophy array */
#define MIN_CHANCE_TO_FLEE      78  /* this is the chance to flee from room with 1 exit*/
#define MAX_CHANCE_TO_FLEE      86  /* this is the chance to flee from room with 4+ exits*/
#define MAX_CHANCE_TO_CONTROL_FLEE 95  /* this is the chance for controlled flee with skill at 100 */
#define MAX_PETS                5 /* number of pets we can save for reloading */

/* time factors */
#define MAX_ARENA_CORPSE_TIME  1200     /*   5 RL minutes */
#define OPT_USEC             250000     /* time delay corresponding to 4 pulses/sec */
#define OVL_PULSE                18     /* anti-spam repeated input limit period */
#define PULSE_BARD_UPDATE        35     /* 15 sec */
#define PULSE_MOBILE             30     /* 10 seconds */
#define PULSE_VEHICLE             2     /* for ships, 1 second */
#define PULSE_VIOLENCE           20     /* combat round, 5 seconds - from 4 Lohrr 6/16/14- upped from 3 on 3/6/14 Drannak */
#define PULSE_SPELLCAST           9     /* casting crap about 2 seconds */
#define PULSE_MOB_HUNT            6     /* how often a HUNTER mob moves */
#define PULSE_AUCTION             8
#define PULSE_SHAPECHANGE        30     /* Time for each * while changing */

#define SECS_PER_MUD_HOUR        75
#define SECS_PER_MUD_DAY       1800    /*  24 * SECS_PER_MUD_HOUR  */
#define SECS_PER_MUD_MONTH    63000    /*  35 * SECS_PER_MUD_DAY   */
#define SECS_PER_MUD_YEAR   1071000    /*  17 * SECS_PER_MUD_MONTH */
#define SECS_PER_REAL_MIN        60
#define SECS_PER_REAL_HOUR     3600     /*  60 * SECS_PER_REAL_MIN  */
#define SECS_PER_REAL_DAY     86400     /*  24 * SECS_PER_REAL_HOUR */
#define MINS_PER_REAL_DAY      1440     /*  60 min/hr * 24 hrs/day */
#define SECS_PER_REAL_YEAR 31536000     /* 365 * SECS_PER_REAL_DAY  */
#define SHORT_AFFECT             70     /* 20 seconds */
#define SHORT_PC_AFFECT          210     /* 20 seconds */
#define WAIT_ROUND               12     /* pulses in a combat round */
#define WAIT_SEC                  4     /* pulses in a second */
#define WAIT_MIN                  60*WAIT_SEC     /* pulses in a minute */
#define PULSES_IN_TICK          300
#define WAIT_PVPDELAY             60*WAIT_SEC /* 60 seconds for pvp delay before rent/etc */

/* limiting factors, some are arbitrary, some are not, change only after thourough checking! */

#define OVERLORD              62  /* highest possible level */
#define FORGER                61    
#define GREATER_G             60
#define LESSER_G              59
#define IMMORTAL              58
#define AVATAR                57
#define MAXLVL                62    /* highest level do_advance can handle */
#define TOTALLVLS             63    /* range of levels (0-62) */
#define MAXLVLMORTAL          56    /* highest a mortal can go */
#define MINLVLIMMORTAL        57    /* bottom of immortal levels */
#define MAX_CIRCLE            12    /* Alter at your own peril! */
#define MAX_CMD_LIST        1000    /* maximum number of total commands */
#define MAX_CONNECTIONS      256    /* last descriptor allowed, really; needs fixed */
#define MAX_DUPES_IN_WELL      5    /* donation well won't accept more than this of same item */
#define MAX_HOSTNAME         256    /* max length of server's hostname */
#define MAX_INPUT_LENGTH    1024    /*    12+ 80 character lines */
#define MAX_MESSAGES         160    /* for pose command */
#define MAX_MSGS             199    /* Max number of messages on one board. */
#define MAX_NUM_ROOMS        100    /* maximum number of rooms in a single ship */
#define MAX_NUM_SHIPS        600    /* maximum number of ships in game */
#define MAX_OBJ_AFFECT         4    /* silly, but I haven't gotten around to fixing it */
#define MAX_PLAYERS_BEFORE_LOCK 100 /* for running with hard limit on number of players */
#define MAX_PROD              25    /* shops, max number of differnt items a shop can produce */
#define MAX_QUESTS          2000    /* max allowable quests */
#define MAX_QUEUE_LENGTH    4800    /* 60 80 character lines */
#define MAX_SEASONS            4    /* maximum number of seasons in a zone */
#define MAX_STRING_LENGTH  65536    /* 819+ 80 character lines */
#define MAX_LOG_LEN       100000    /* MAX LOGGING */
#define MAX_TONGUE            29    /* number of defined tongues */
#define MAX_TRADE             10    /* shops, max number of item types they trade in  */
#define MAX_WHO_PLAYERS      512    /* for do_who, max number of players it can handle */
#define MAX_ZONES            512    /* mob memory shortcut, 'remembers' what zone player is in */

/* room in heavens to store corpses if room is invalid */
#define CORPSE_STORAGE 40

/* misc thingies */
#define DEFAULT_MODE (SHOW_NAME | SHOW_ON)      /* information that whod reveals */

#define NOWHERE           -1    /* nil reference for room-database    */
#define OVL_DWEEB_LIMIT    1    /* legal repetitions within OVL_PULSE for dweebs */
#define OVL_NORMAL_LIMIT   5    /* legal repetitions within OVL_PULSE */

#ifdef MEM_DEBUG
#define MEM_TAG_STRING  "MSTR" // strings
#define MEM_TAG_BUFFER  "MBUF" // non-type byte arrays
#define MEM_TAG_ARRAY   "MARR" // typed arrays
#define MEM_TAG_SOCMSG  "MSCM" // social messages
#define MEM_TAG_SNOOP   "MSNP" // snoop data
#define MEM_TAG_WIZBAN  "MWZB" // wizban data
#define MEM_TAG_BAN     "MBAN" // ban data
#define MEM_TAG_DIRDATA "MDIR" // room direction data
#define MEM_TAG_TXTBLK  "MTXB" // text block data
#define MEM_TAG_REGNODE "MRGN" // registration nodes
#define MEM_TAG_TBLDATA "MTBL" // table data
#define MEM_TAG_TBLELEM "MTEL" // table elements
#define MEM_TAG_IDXDATA "MIDX" // index data
#define MEM_TAG_ROOMDAT "MRMD" // room data
#define MEM_TAG_ZONEDAT "MZON" // zone data
#define MEM_TAG_SECTDAT "MSCD" // sector data
#define MEM_TAG_RESET   "MRES" // reset commands
#define MEM_TAG_NPCONLY "MNPC" // npc only data
#define MEM_TAG_EXDESCD "MEXD" // extra description data
#define MEM_TAG_EDITDAT "MEDT" // edit data
#define MEM_TAG_SHPCHNG "MSHG" // shape change data
#define MEM_TAG_SHIPREG "MSRN" // ship reg node
#define MEM_TAG_SACKREC "MHSR" // house sack record
#define MEM_TAG_HUNTDAT "MHNT" // hunt data - this one is up for pool
#define MEM_TAG_MEMMAN  "MMEM" // memory management descriptor
#define MEM_TAG_MMLIST  "MMLS" // mem man descriptor list item
#define MEM_TAG_REMEMBD "MREM" // remember data
#define MEM_TAG_MOBMEM  "MMBM" // mob memory
#define MEM_TAG_OLCDATA "MOLC" // online creation data
#define MEM_TAG_QSTMSG  "MQMS" // quest message data
#define MEM_TAG_QSTCOMP "MQCM" // quest complete data
#define MEM_TAG_QSTGOAL "MQGL" // quest goal data
#define MEM_TAG_SHOPDAT "MSHP" // shop data
#define MEM_TAG_SHOPBUY "MSHB" // shop buy data
#define MEM_TAG_FOLLOW  "MFOL" // follow data
#define MEM_TAG_SHIPAI  "MSAI" // ship ai data
#define MEM_TAG_SHIPGRP "MSGP" // ship group data
#define MEM_TAG_SHIPDAT "MSPD" // ship data
#define MEM_TAG_POSLIST "MPLT" // position list type data
#define MEM_TAG_ZSTREAM "MZST" // mccp z stream data
#define MEM_TAG_EVTBUF  "MEVB" // buffer for event data
#define MEM_TAG_NQACTOR "MNQA" // new quest actor data
#define MEM_TAG_NQSTR   "MNQS" // new quest string
#define MEM_TAG_NQINST  "MNQI" // new quest instance
#define MEM_TAG_NQITEM  "MNQT" // new quest item
#define MEM_TAG_NQSKILL "MNQK" // new quest skill
#define MEM_TAG_NQRWRD  "MNQR" // new quest reward
#define MEM_TAG_NQACTN  "MNQC" // new quest action
#define MEM_TAG_NQATMP  "MNQE" // new quest actor template
#define MEM_TAG_NQQST   "MNQQ" // new quest quest data

#define MEM_TAG_OTHER   "MOTH" // non classified (please do not use unless absolutely necessary)
#endif

#define MINIMUM_VOTE_LEVEL  35
#define VOTE_SERIAL                     1
#define VOTE_ENABLED            1
#define VOTE_FILE                       "lib/vote.results"

#endif  /* _SOJ_CONFIG_H */
