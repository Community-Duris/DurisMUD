

/*
 * ***************************************************************************
 * *  File: interp.c                                           Part of Duris *
 * *  Usage: main command interpreter, and anciliary functions..
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <arpa/telnet.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>


#include "comm.h"
#include "db.h"
#include "events.h"
#include "graph.h"
#include "interp.h"
#include "new_combat_def.h"
#include "structs.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "utils.h"
#include "weather.h"
#include "sound.h"
#include "assocs.h"
#include "justice.h"
#include "mm.h"
#include "epic.h"
#include "sql.h"
#include "makeexit.h"
#include "nexus_stones.h"
#include "testcmd.h"
#include "rogues.h"
#include "disguise.h"
#include "grapple.h"
#include "map.h"
#include "alliances.h"
#include "avengers.h"
#include "multiplay_whitelist.h"
#include "guildhall.h"
#include "outposts.h"
#include "buildings.h"
#include "boon.h"
#include "ctf.h"
#include "tether.h"
#include "auction_houses.h"
#include "achievements.h"
#include "siege.h"
#include "vnum.room.h"
#include "tradeskill.h"

/*
 * external variables
 */
extern struct time_info_data time_info;
extern P_desc descriptor_list;

extern char debug_mode;
extern int hometown[];
extern int no_specials;
extern int pulse;               /*
                                 * TAM 2/94
                                 */
extern P_char character_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;

bool command_confirm;

void do_prestige(P_char ch, char *argument, int cmd);
void check_aggro_from_command( P_char exec_char );

/*=========================================================================*/
/*
 *    Global variables
 */
/*=========================================================================*/

struct command_info cmd_info[MAX_CMD_LIST];

/*
 * NOTE!  the order of this list is IMPORTANT! and must match up with the
 * list of #define's in interp.h.
 */

const char *command[MAX_CMD] = {
  "north",                      // 1
  "east",
  "south",
  "west",
  "up",
  "down",
  "enter",
  "exits",
  "kill",
  "get",
  "drink",                      /*
                                 * 11
                                 */
  "eat",
  "wear",
  "wield",
  "look",
  "score",
  "say",
  "gshout",
  "tell",
  "inventory",
  "qui",                        /*
                                 * 21
                                 */
  "bounce",
  "smile",
  "dance",
  "kiss",
  "cackle",
  "laugh",
  "giggle",
  "shake",
  "puke",
  "growl",                      /*
                                 * 31
                                 */
  "scream",
  "insult",
  "comfort",
  "nod",
  "sigh",
  "sulk",
  "help",
  "who",
  "emote",
  "echo",                       /*
                                 * 41
                                 */
  "stand",
  "sit",
  "rest",
  "sleep",
  "wake",
  "force",
  "transfer",
  "hug",
  "snuggle",
  "cuddle",                     /*
                                 * 51
                                 */
  "nuzzle",
  "cry",
  "news",
  "equipment",
  "buy",
  "sell",
  "value",
  "list",
  "drop",
  "goto",                       /*
                                 * 61
                                 */
  "weather",
  "read",
  "pour",
  "grab",
  "remove",
  "put",
  "shutdow",
  "save",
  "hit",
  "string",                     /*
                                 * 71
                                 */
  "give",
  "quit",
  "stat",
  "innate",
  "time",
  "load",
  "purge",
  "shutdown",
  "idea",
  "typo",                       /*
                                 * 81
                                 */
  "bug",
  "whisper",
  "cast",
  "at",
  "ask",
  "order",
  "sip",
  "taste",
  "snoop",
  "follow",                     /*
                                 * 91
                                 */
  "rent",
  "offer",
  "poke",
  "advance",
  "acc",
  "grin",
  "bow",
  "open",
  "close",
  "lock",                       /*
                                 * 101
                                 */
  "unlock",
  "mreport",
  "applaud",
  "blush",
  "burp",
  "chuckle",
  "clap",
  "cough",
  "curtsey",
  "fart",                       /*
                                 * 111
                                 */
  "flip",
  "fondle",
  "frown",
  "gasp",
  "glare",
  "groan",
  "grope",
  "hiccup",
  "lick",
  "love",                       /*
                                 * 121
                                 */
  "moan",
  "nibble",
  "pout",
  "purr",
  "ruffle",
  "shiver",
  "shrug",
  "sing",                       /*
                                 * 129 - special case for bards
                                 */
  "slap",
  "smirk",                      /*
                                 * 131
                                 */
  "snap",
  "sneeze",
  "snicker",
  "sniff",
  "snore",
  "spit",
  "squeeze",
  "stare",
  "strut",
  "thank",                      /*
                                 * 141
                                 */
  "twiddle",
  "wave",
  "whistle",
  "wiggle",
  "wink",
  "yawn",
  "snowball",
  "write",
  "hold",
  "flee",                       /*
                                 * 151
                                 */
  "sneak",
  "hide",
  "backstab",
  "pick",
  "steal",
  "bash",
  "rescue",
  "kick",
  "french",
  "comb",                       /*
                                 * 161
                                 */
  "massage",
  "tickle",
  "practice",
  "pat",
  "examine",
  "take",
  "info",
  "spells",
  "practise",
  "curse",                      /*
                                 * 171
                                 */
  "use",
  "where",
  "levels",
  "reroll",
  "pray",
  ":",
  "beg",
  "bleed",
  "cringe",
  "dream",                      /*
                                 * 181
                                 */
  "fume",
  "grovel",
  "hop",
  "nudge",
  "peer",
  "point",
  "ponder",
  "punch",
  "snarl",
  "spank",                      /*
                                 * 191
                                 */
  "steam",
  "ground",
  "taunt",
  "think",
  "whine",
  "worship",
  "yodel",
  "toggle",
  "wizmsg",
  "consider",                   /*
                                 * 201
                                 */
  "group",
  "restore",
  "return",
  "switch",                     /*
                                 * 205
                                 */
  "quaff",
  "recite",
  "users",
  "pose",
  "silence",
  "wizhelp",                    /*
                                 * 211
                                 */
  "credits",
  "disband",
  "vis",
  "lflags",                     /*
                                 * 215
                                 */
  "poofin",
  "wizlist",
  "display",
  "echoa",
  "demote",                     /*
                                 * 220
                                 */
  "poofout",
  "circle",
  "balance",
  "wizlock",
  "deposit",                    /*
                                 * 225
                                 */
  "withdraw",
  "ignore",
  "setattr",
  "title",
  "aggr",                       /*
                                 * 230
                                 */
  "gsay",
  "consent",
  "setbit",
  "hitall",
  "trap",                       /*
                                 * 235
                                 */
  "murder",
  "glance",
  "auction",
  "channel",
  "fill",                       /*
                                 * 240
                                 */
  "gcc",
  "track",
  "page",
  "commands",
  "attributes",                 /*
                                 * 245
                                 */
  "rules",
  "analyze",
  "listen",
  "disarm",
  "pet",                        /*
                                 * 250
                                 */
  "delete",
  "ban",
  "allow",
  "play",
  "move",                       /*
                                 * 255
                                 */
  "bribe",
  "bonk",
  "calm",
  "rub",
  "censor",                     /*
                                 * 260
                                 */
  "choke",
  "drool",
  "flex",
  "jump",
  "lean",                       /*
                                 * 265
                                 */
  "moon",
  "ogle",
  "pant",
  "pinch",
  "push",                       /*
                                 * 270
                                 */
  "scare",
  "scold",
  "seduce",
  "shove",
  "shudder",                    /*
                                 * 275
                                 */
  "shush",
  "slobber",
  "smell",
  "sneer",
  "spin",                       /*
                                 * 280
                                 */
  "squirm",
  "stomp",
  "strangle",
  "stretch",
  "tap",                        /*
                                 * 285
                                 */
  "tease",
  "tip",
  "tweak",
  "twirl",
  "undress",                    /*
                                 * 290
                                 */
  "whimper",
  "exchange",
  "release",
  "search",
  "join",                       /*
                                 * 295
                                 */
  "camp",
  "secret",
  "lookup",
  "report",
  "split",                      /*
                                 * 300
                                 */
  "world",
  "junk",
  "petition",
  "do",
  "'",                          /*
                                 * 305
                                 */
  "caress",
  "bury",
  "donate",
  "shout",
  "disembark",                  /*
                                 * 310
                                 */
  "panic",
  "nog",
  "twibble",
  "throw",
  "legsweep",                   /*
                                 * 315
                                 */
  "sweep",
  "apologize",
  "afk",
  "lag",
  "touch",                      /*
                                 * 320
                                 */
  "scratch",
  "wince",
  "toss",
  "flame",
  "arch",                       /*
                                 * 325
                                 */
  "amaze",
  "bathe",
  "embrace",
  "brb",
  "ack",                        /*
                                 * 330
                                 */
  "cheer",
  "snort",
  "eyebrow",
  "bang",
  "pillow",                     /*
                                 * 335
                                 */
  "nap",
  "nose",
  "raise",
  "hand",
  "pull",                       /*
                                 * 340
                                 */
  "tug",
  "wet",
  "mosh",
  "wait",
  "hi5",                        /*
                                 * 345
                                 */
  "envy",
  "flirt",
  "bark",
  "whap",
  "roll",                       /*
                                 * 350
                                 */
  "blink",
  "duh",
  "gag",
  "grumble",
  "dropkick",                   /*
                                 * 355
                                 */
  "whatever",
  "fool",
  "noogie",
  "melt",
  "smoke",                      /*
                                 * 360
                                 */
  "wheeze",
  "bird",
  "boggle",
  "hiss",
  "bite",                       /*
                                 * 365
                                 */
  "teleport",
  "bandage",
  "blow",
  "bored",
  "bye",                        /*
                                 * 370
                                 */
  "congratulate",
  "duck",
  "flutter",
  "goose",
  "gulp",                       /*
                                 * 375
                                 */
  "halo",
  "hello",
  "hickey",
  "hose",
  "hum",                        /*
                                 * 380
                                 */
  "impale",
  "jam",
  "kneel",
  "mourn",
  "protect",                    /*
                                 * 385
                                 */
  "puzzle",
  "roar",
  "rose",
  "salute",
  "skip",                       /*
                                 * 390
                                 */
  "swat",
  "tongue",
  "woops",
  "zone",
  "trip",                       /*
                                 * 395
                                 */
  "meditate",
  "shapechange",
  "assist",
  "doorbash",
  "exp",                        /*
                                 * 400
                                 */
  "rofl",
  "agree",
  "happy",
  "pucker",
  "spam",                       /*
                                 * 405
                                 */
  "beer",
  "bodyslam",
  "sacrifice",
  "terminate",
  "cd",                         /*
                                 * 410
                                 */
  "memorize",
  "forget",
  "headbutt",
  "shadow",
  "ride",                       /*
                                 * 415
                                 */
  "mount",
  "dismount",
  "debug",
  "freeze",
  "bbl",                        /*
                                 * 420
                                 */
  "gape",
  "veto",
  "jk",
  "tiptoe",
  "grunt",                      /*
                                 * 425
                                 */
  "holdon",
  "imitate",
  "tango",
  "tarzan",
  "pounce",                     /*
                                 * 430
                                 */
  "cheek",
  "layhand",
  "awareness",
  "firstaid",
  "springleap",                 /*
                                 * 435
                                 */
  "feigndeath",
  "chant",
  "drag",
  "speak",
  "reload",                     /*
                                 * 440
                                 */
  "dragonpunch",
  "revoke",
  "grant",
  "olc",
  "motd",                       /*
                                 * 445
                                 */
  "zreset",
  "full",
  "welcome",
  "introduce",
  "sweat",                      /*
                                 * 450
                                 */
  "mutter",
  "lucky",
  "ayt",
  "fidget",
  "fuzzy",                      /*
                                 * 455
                                 */
  "snoogie",
  "ready",
  "plonk",
  "hero",
  "lost",                       /*
                                 * 460
                                 */
  "drain",
  "flash",
  "curious",
  "hunger",
  "thirst",                     /*
                                 * 465
                                 */
  "echoz",
  "ptell",
  "scribe",
  "teach",
  "reinitphys",                 /*
                                 * 470
                                 */
  "finger",
  "approve",
  "decline",
  "summon",
  "clone",                      /*
                                 * 475
                                 */
  "trophy",
  "zap",
  "alert",
  "recline",
  "knock",                      /* 480 */

  "skills",
  "berserk",
  "faq",
  "disengage",
  "retreat",
  "inroom",
  "which",
  "proclib",
  "sethome",
  "scan",                       /* 490 */

  "apply",
  "hitch",
  "unhitch",
  "map",
  "ogreroar",
  "bearhug",
  "dig",
  "justice",
  "supervise",
  "society",                    /* 500 */

  "trapset",
  "trapremove",
  "carve",
  "depiss",
  "repiss",
  "breathe",
  "ingame",
  "fire",
  "repair",
  "mail",                       /* 510 */

  "rename",
  "project",
  "absorbe",
  "fly",
  "squidrage",
  "fraglist",
  "lwitness",
  "pay",
  "pardon",
  "reporting",                  /* 520 */
  "bind",
  "unbind",
  "turn_in",
  "capture",
  "appraise",
  "cover",
  "house",
  "guildhall",
  "doorkick",
  "buck",                       /* 530 */
  "stampede",
  "subterfuge",
  "dirttoss",
  "disguise",
  "claim",
  "charge",
  "lore",
  "poofinsound",
  "poofoutsound",
  "swim",                       /* 540 */
  "northwest",
  "southwest",
  "northeast",
  "southeast",
  "nw",
  "sw",
  "ne",
  "se",
  "will",
  "condition",                  /* 550 */
  "neckbite",
  "forage",
  "construct",
  "sack",
  "climb",
  "make",
  "throat",
  "ztestdesc",                  // would rather have 'test' for 'testcolor'
  "target",
  "suicide",                    /* 560 */
  "echog",
  "echoe",
  "asclist",
  "whois",
  "free",
  "echot",
  "roundkick",
  "pleasant",
  "hamstring",
  "empty",                      /* 570 */
  "arena",
  "artifacts",
  "invite",
  "uninvite",
  "combination",
  "wizconnect",
  "reply",
  "deathobj",
  "rwc",
  "affectpurge",                /* 580 */
  "tupor",
  "echou",
  "guard",
  "assimilate",
  "randobj",
  "commune",
  "terrain",
  "omg",
  "bslap",
  "hump",                       /* 590 */
  "bong",
  "spoon",
  "eek",
  "moving",
  "wit",
  "flap",
  "spew",
  "addict",
  "banzai",
  "bhug",                       /* 600 */
  "bkiss",
  "blindfold",
  "boogie",
  "brew",
  "bully",
  "bungee",
  "sob",
  "curl",
  "cherish",
  "clue",                       /* 610 */
  "chastise",
  "coffee",
  "cower",
  "carrot",
  "doh",
  "ekiss",
  "faint",
  "gawk",
  "ghug",
  "hangover",                   /* 620 */
  "hcuff",
  "hmm",
  "icecube",
  "maim",
  "mahvelous",
  "meow",
  "mmmm",
  "mooch",
  "cow",
  "muhaha",                     /* 630 */
  "mwalk",
  "nails",
  "nasty",
  "ni",
  "ohno",
  "oink",
  "ooo",
  "peck",
  "ping",
  "plop",                       /* 640 */
  "potato",
  "epardon",
  "reassure",
  "ohhappy",
  "glum",
  "smooch",
  "squeal",
  "squish",
  "stickup",
  "strip",                      /* 650 */
  "suffer",
  "stoke",
  "esweep",
  "swoon",
  "tender",
  "throttle",
  "timeout",
  "torture",
  "tummy",
  "type",                       /* 660 */
  "wedgie",
  "wish",
  "wrap",
  "yabba",
  "yahoo",
  "yeehaw",
  "sload",
  "castrate",
  "boot",
  "glee",                       /* 670 */
  "scowl",
  "mumble",
  "jeer",
  "tank",
  "praise",
  "beckon",
  "crack",
  "grenade",
  "chortle",
  "whip",                       /* 680 */
  "claw",
  "shades",
  "tan",
  "sunburn",
  "threaten",
  "twitch",
  "babble",
  "wrinkle",
  "guffaw",
  "jig",                        /* 690 */
  "blame",
  "shuffle",
  "froth",
  "howl",
  "monkey",
  "disgust",
  "vote",
  "hire",
  "reloadhelp",
  "testcolor",                  /* 700 */
  "multiclass",
  "resetarti",
  "specialize",
  "resetspec",
  "nchat",
  "warcry",
  "tackle",
  "disappear",
  "hardcore",
  "cheat",                      /* 710 */
  "cheater",
  "punish",
  "craft",
  "mix",
  "forge",
  "throwpotion",
  "projects",
  "rage",
  "maul",
  "rampage",                    /* 720 */
  "lotus",
  "true strike",
  "chi",
  "shieldpunch",
  "sweeping thrust",
  "infuriate",
  "encrust",
  "gmotd",
  "fix",
  "trample",                    /* 730 */
  "wail",
  "statistic",
  "home",
  "whirlwind",
  "epic",
  "thrust",
  "enchant",
  "unthrust",
  "rush",
  "properties",                /* 740 */
  "flank",
  "gaze",
  "quest",
  "relic",
  "scent",
  "call",
  "ok",
  "strike",
  "mug",
  "defend",                   /* 750 */
  "exhume",
  "more",
  "fade",
  "shriek",
  "remort",
  "recall",
  "raid",
  "mine",
  "fish",
  "spellbind",               /* 760 */
  "ascend",
  "smelt",
  "hone",
  "parlay",
  "newbie",
  "makeguide",
  "spellweave",
  "descend",
  "sql",
  "makeexit",               /* 770 */
  "heroescall",
  "nexus",
  "test",
  "tranquilize",
  "train",
  "slip",
  "headlock",
  "leglock",
  "groundslam",
  "infuse",                /* 780 */
  "build",
  "prestige",
  "alliance",
  "accuse",
  "destroy",
  "smite",
  "storage",
  "gather",
  "nafk",
  "grimace",               /* 790 */
  "newbsu",
  "givepet",
  "outpost",
  "offensive",
  "peruse",
  "harvest",
  "battlerager", // For the battlerager proc room
  "petition_block",
  "area",
  "whitelist",             /* 800 */
  "epicreset",
  "focus",
  "boon",
  "ctf",
  "tether",
  "questwhere",
  "newbsa",
  "salvage",
  "restrain",
  "barrage",               /* 810 */
  "blade",
  "consume",
  "riff",
  "leaderboard",
  "soulbind",
  "achievements",
  "salvation",
  "refine",
  "dreadnaught",
  "dice",                  /* 820 */
  "shadowstep",
  "garrote",
  "conjure",
  "dismiss",
  "enhance",
  "add",
  "deploy",
  "blood",
  "deforest",
  "beep",                 /* 830 */
  "deathsdoor",
  "offlinemsg",
  "instacast",
  "surname",
  "\n"                          /* MAX_CMD = 835, MAX_CMD_LIST = 1000 */
};

const char *fill_words[] = {
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

// Rude words that should not be found in names/ship names.
// Please keep in alpha order to make sure no duplicates.
const char *rude_ass[] = {
  "ass",
  "bitch",
  "cunt",
  "pussy",
  "dick",
  "fuck",
  "penis",
  "shit",
  "nigger",
  "nazi",
  "\n"
};

int search_block(char *arg, const char **list, int exact)
{
  register int i, l;

  if (!arg)
    return -1;

  /*
   * Make into lower case, and get length of string
   */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact)
  {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!str_cmp(arg, *(list + i)))
        return (i);
  }
  else
  {
    if (!l)
      l = 1;                    /*
                                 * Avoid "" to match the first available
                                 * string
                                 */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strn_cmp(arg, *(list + i), (unsigned) l))
        return (i);
  }

  return (-1);
}

int old_search_block(const char *argument, const uint begin, uint length,
                     const char **list, const int mode)
{
  int      guess, found, search;

  if (!argument)
    return -1;

  /*
   * If the word contain 0 letters, then a match is already found
   */
  found = (length < 1);
  guess = 0;
  /*
   * Search for a match
   */

  if (mode == 2)
  {
    /* in mode 2, first check for an exact match, then check for a mere
       "left side" match if nothing found */

    while (!found && (*(list[guess]) != '\n'))
    {
      found = (length == strlen(list[guess]));
      for (search = 0; (search < length) && found; search++)
        found = (*(argument + begin + search) == *(list[guess] + search));
      guess++;
    }

    if (!found)
      guess = 0;
    else
      return guess;

    while (!found && (*(list[guess]) != '\n'))
    {
      found = 1;
      for (search = 0; (search < length) && found; search++)
        found = (*(argument + begin + search) == *(list[guess] + search));
      guess++;
    }
  }
  else if (mode)
  {
    while (!found && (*(list[guess]) != '\n'))
    {
      found = (length == strlen(list[guess]));
      for (search = 0; (search < length) && found; search++)
        found = (*(argument + begin + search) == *(list[guess] + search));
      guess++;
    }
  }
  else
  {
    while (!found && (*(list[guess]) != '\n'))
    {
      found = 1;
      for (search = 0; (search < length) && found; search++)
        found = (*(argument + begin + search) == *(list[guess] + search));
      guess++;
    }
  }

  return (found ? guess : -1);
}

/*
 * SAM 7-94, command confirmation
 */
void do_confirm(P_char ch, bool yes)
{
  P_char founder;

  // Was a force, or npc doint it.
  if( IS_NPC(ch) || !ch->desc )
    return;

  // The putz just entered yes or no without a pending command
  if( ch->desc->confirm_state != CONFIRM_AWAIT )
  {
    send_to_char("What do you wish to confirm?\r\n", ch);
    return;
  }
  // They said no
  if( !yes )
  {
    ch->desc->confirm_state = CONFIRM_NONE;
    send_to_char("Did not think you wanted to do that... :P\r\n", ch);
    return;
  }
  // They said yes, so do it
  // Guild creation - Drannak
  if (strstr(ch->desc->client_str, "found_asc"))
  {
    char guildinfo[MAX_INPUT_LENGTH];
    if( GET_MONEY(ch) < GUILD_COST )
    {
      send_to_char("You dont have enough money!\r\n", ch);
      ch->desc->confirm_state = CONFIRM_DONE;
      strcpy(ch->desc->client_str, "");
      return;
    }
    founder = world[ch->in_room].people;
    while( founder )
    {
      if( IS_NPC(founder) && (mob_index[GET_RNUM(founder)].func.mob == assoc_founder) )
        break;
      founder = founder->next_in_room;
    }
    if( !founder )
    {
      send_to_char("The guild creating mob seems to have wandered off!\n", ch);
      ch->desc->confirm_state = CONFIRM_DONE;
      strcpy(ch->desc->client_str, "");
      return;
    }

    sprintf(guildinfo, "%s", ch->desc->last_command);
    bool x = found_asc(founder, ch, "n", guildinfo);
    if(x)
    SUB_MONEY(ch, GUILD_COST, 0);
    ch->desc->confirm_state = CONFIRM_DONE;
    strcpy(ch->desc->client_str, "");
    return;
   }

  ch->desc->confirm_state = CONFIRM_DONE;
  command_interpreter(ch, ch->desc->last_command);
}

/*
 * **    Improvements/Additions ** ** 1) disallow certain commands when
 * paralyzed. --TAM ** 2) disallow all but stand command when berserked
 * recently --TAM ** 3) disallow all commands for instant kill when used
 * recently --TAM ** 4) slow rate which commands are parsed if affected by
 * slow.  commands **    are ignored and thrown away. --TAM ** 5) add in
 * ability to force certain commands to require confirmation **    to be
 * executed. --SAM 7-94 **
 */
void command_interpreter(P_char ch, char *argument)
{
  uint     look_at, begin, old_room;
  int      cmd, i, j, k, current;
  char    *ch_ptr;
  P_char   target, master, exec_char = ch;

  if( debug_mode )
  {
    cmdlog(ch, argument);
  }

  if( IS_PC(ch) && IS_SET(ch->specials.act, PLR_FROZEN) )
  {
    if( GET_LEVEL(ch) == MAXLVL )
    {
      send_to_char("Your &+Wpower&n ovverides the &+Cice&n that surrounds you.\n\r", ch );
    }
    else
    {
      send_to_char("You are frozen! You can't do anything! Nah Nah! :P\r\n", ch);
      return;
    }
  }
/*
  char testing = get_alias(ch, argument);
  char blah[MAX_STRING_LENGTH];

  if(!testing)
  {
  sprintf(blah, "%s", testing);
  command_interpreter(ch, testing);
  return;
  }
*/

  /* Find first non blank */
  for( begin = 0; (*(argument + begin) == ' '); begin++ ) ;

  /* Find length of first word */
  for( look_at = 0; *(argument + begin + look_at) > ' '; look_at++ )
  {
    /* Make all letters lower case AND find length */
    *(argument + begin + look_at) = LOWER(*(argument + begin + look_at));
  }

  if (IS_PC(ch) && ch->desc)
    if (ch->desc->confirm_state == CONFIRM_AWAIT)
    {
      if (*(argument + begin) == 'y')
      {
        do_confirm(ch, 1);
        return;
      }
      else if (*(argument + begin) == 'n')
      {
        do_confirm(ch, 0);
        return;
      }
    }
  cmd = old_search_block(argument, begin, look_at, command, 2);

  if (!cmd)
    return;
#if 0
  if ((cmd != CMD_PETITION) && IS_PC(ch) && IS_ILLITHID(ch) &&
      (GET_LEVEL(ch) < 52))
  {
    send_to_char("Don't you know, the Elder Brain wants you home!\r\n", ch);
    return;
  }
#endif

#if 0
  if( cmd != CMD_LOOK && cmd != CMD_GET && cmd != CMD_RENT &&
      cmd != CMD_TELL && cmd != CMD_PETITION && cmd != CMD_SAY &&
      cmd != CMD_GIVE && cmd != CMD_STAND && cmd != CMD_INVENTORY &&
      cmd != CMD_SCORE && cmd != CMD_WHO && cmd != CMD_EQUIPMENT &&
      cmd != CMD_REPLY && cmd != CMD_WEAR && cmd != CMD_REMOVE &&
      cmd != CMD_PUT && cmd != CMD_HELP && cmd != CMD_CAMP && 
      cmd != CMD_QUIT )
  {
    for( P_desc d = descriptor_list; d; d = d->next )
    {
      if( ch->desc && d->character && ch != d->character && !IS_TRUSTED(ch)
        && !IS_TRUSTED(d->character) && ch->only.pc->last_ip == d->character->only.pc->last_ip )
      {
        send_to_char("With your soul split into pieces, you can't do that!\n", ch);
        return;
      }
    }
  }
#endif

  /* happy little hack to make all 'say' procs work */
  if( cmd == CMD_SAY2 )
    cmd = CMD_SAY;

  if( IS_PC(ch) && IS_SET(ch->specials.act, PLR_AFK) )
  {
    REMOVE_BIT(ch->specials.act, PLR_AFK);
  }

  if( world[ch->in_room].chance_fall && number(1, 100) <= world[ch->in_room].chance_fall )
  {
    // Starting speed 0, and do not kill.
    if( falling_char(ch, 0, FALSE) )
    {
      return;
    }
  }

  // If there's a current in the water to pull chars around.
  if( world[ch->in_room].current_speed && !IS_TRUSTED(ch) )
  {
    // If there's water and they're in it and not calling on Gods for help.
    if( IS_WATER_ROOM(ch->in_room) && !IS_AFFECTED(ch, AFF_LEVITATE)
      && !IS_AFFECTED(ch, AFF_FLY) && cmd != CMD_PETITION )
    {
      if( number(1, 101) < world[ch->in_room].current_speed )
      {
        current = world[ch->in_room].current_direction;
        if( CAN_GO(ch, current) )
        {
          send_to_char("The current sweeps you away!\r\n", ch);
          do_move( ch, 0, exitnumb_to_cmd(current) );
          return;
        }
      }
    }
  }
  if (IS_AFFECTED2(ch, AFF2_CASTING))
  {
    /* This is all using the old event data stuff, ch->events is no longer used.
     *   It has been replaced by P_nevent, ch->nevents, LOOP_EVENTS_CH, ev->func == event_spellcast
     * Since we haven't seen it in a while, and ch->events is always NULL, no point in updating this.
    // check for spellcast loop bug!!!!!
    P_event  ev;

    // This is POSSIBLY a spellcast event in the wrong place.  Instead of
    //   just nuking it, see how long it has.  If more then 5 pulses, then nuke it.
    LOOP_EVENTS( ev, ch->events )
    {
      if( IS_PC(ch) && (ev->type == EVENT_SPELLCAST) && (event_time(ev, T_PULSES) > 5) )
      {
        statuslog(AVATAR, "Spellcast bug on %s: spell aborted.", J_NAME(ch));
        StopCasting(ch);
      }
    }
    */
    if( cmd != CMD_PETITION && cmd != CMD_RETURN )
    {
      send_to_char("You're busy spellcasting!\r\n", ch);
      if( IS_TRUSTED(ch) )
        send_to_char("&+YTry 'return' or you can petition other gods for help if you're stuck.&n\r\n", ch);
      else
        send_to_char("If you think you're stuck, you can still petition.\r\n", ch);
      return;
    }
  }
  if( IS_AFFECTED(ch, AFF_KNOCKED_OUT) )
  {
    if( (STAT_MASK & cmd_info[cmd].minimum_position) != STAT_DEAD )
    {
      // Channel spell is multi-cast spell to create an avatar mob.
      if( !(affected_by_spell(ch, SPELL_CHANNEL) && cmd == CMD_SAY) )
      {
        send_to_char("Being knocked unconscious strictly limits what you can do.\r\n", ch);
        return;
      }
    }
  }
 /* Check for ansi characters, mortals not allowed to use them to put
  * color in says, shouts, gossips, titles, etc. SAM 6-94
  */
  // mortals may never use the magical newline
  if( !IS_TRUSTED(ch) )
  {
    for (ch_ptr = argument; *ch_ptr != '\0'; ch_ptr++)
    {
      if( *ch_ptr == '&' && *(ch_ptr + 1) == 'L' )
      {
        send_to_char("No mortal may posess the newline!\r\n", ch);
        return;
      }
    }

    if( !((cmd == CMD_SOCIETY) || (cmd == CMD_CONSTRUCT) || (cmd == CMD_BUY)
      || (cmd == CMD_RENAME) || (cmd == CMD_TESTCOLOR)) )
    {
      for (ch_ptr = argument; *ch_ptr != '\0'; ch_ptr++)
      {
        if (*ch_ptr == '&')
        {
          switch (*(ch_ptr + 1))
          {
          case '+':
          case '-':
          case '=':
          case 'n':
          case 'N':
            send_to_char("Pardon? No ansi chars allowed as input.\r\n", ch);
            return;
            break;
          case '&':
              ch_ptr++;
            break;
          }
        }
      }
    }

    /* Check for sound stuff, ditto */
    for (ch_ptr = argument; *ch_ptr != '\0'; ch_ptr++)
    {
      if( *ch_ptr == '!' && *(ch_ptr + 1) == '!' && isupper(*(ch_ptr + 2)) )
      {
        send_to_char("Pardon? No sound sequences allowed as input.\r\n", ch);
        return;
      }
    }
  }
  if( (cmd > 0) && (cmd_info[cmd].command_pointer != 0) )
  {
    if( !MIN_POS(ch, cmd_info[cmd].minimum_position)
      || ((IS_FIGHTING(ch) || IS_DESTROYING(ch)) && !cmd_info[cmd].in_battle) )
    {
      if( GET_STAT(ch) < (cmd_info[cmd].minimum_position & STAT_MASK) )
      {
        switch (GET_STAT(ch))
        {
          case STAT_DEAD:
            send_to_char("Lie still; you are DEAD!!!\r\n", ch);
            break;
          case STAT_INCAP:
          case STAT_DYING:
            send_to_char("You are in pretty bad shape, unable to do anything!\r\n", ch);
            break;
          case STAT_SLEEPING:
            send_to_char("In your dreams, or what?\r\n", ch);
            break;
          case STAT_RESTING:
            send_to_char("Nah... You feel too relaxed to do that...\r\n", ch);
            break;
        }
      }
      if (GET_POS(ch) < (cmd_info[cmd].minimum_position & 3))
        switch (GET_POS(ch))
        {
        case POS_PRONE:
          send_to_char("Sorry, you can't do that while laying around.\r\n", ch);
          break;
        case POS_KNEELING:
          send_to_char("Maybe you should get up off your knees first?\r\n", ch);
          break;
        case POS_SITTING:
          send_to_char("Maybe you should get on your feet first?\r\n", ch);
          break;
        }
      if( (IS_FIGHTING(ch) || IS_DESTROYING(ch)) && !cmd_info[cmd].in_battle )
        send_to_char("Sorry, you aren't allowed to do that in combat.\r\n", ch);
      return;
    }
    else
    {
      // Always allow Imms to return to their own body.
      if( cmd == CMD_RETURN && ch->desc && ch->desc->original && IS_TRUSTED(ch->desc->original) )
      {
        do_return( ch, 0, CMD_RETURN );
      }
      if ((IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS) ||
           IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS)) &&
          !CAN_CMD_PARALYSIS(cmd) && !IS_TRUSTED(ch))
      {
        if ((IS_NPC(ch) || IS_NPC(ch)) && ch->following)
          send_to_char("It can't being paralyzed.\r\n", ch->following);
        else if (ch)
          send_to_char("You can't!  You're paralyzed to the bone.\r\n", ch);
        return;
      }
      if (affected_by_spell(ch, SKILL_GAZE))
      {
        send_to_char("You are too petrified with fear to try that.\r\n", ch);
        return;
      }
      if (affected_by_spell(ch, SKILL_RESTRAIN))
      {
        send_to_char("&+LYou are being restrained by a band of &+rminions&+L right now!\r\n", ch);
        return;
      }
      if ((target = get_linked_char(ch, LNK_ESSENCE_OF_WOLF)))
      {
        clear_links(ch, LNK_ESSENCE_OF_WOLF);
      }
      if( IS_AFFECTED2(ch, AFF2_SCRIBING) && (cmd != CMD_PETITION && cmd != CMD_TELL && cmd != CMD_MOVE
        && cmd != CMD_LOOK && cmd != CMD_WHISPER && cmd != CMD_FORAGE) )
      {
        send_to_char("You're busy scribing a spell into your spellbook!\r\n", ch);
        return;
      }
      if (affected_by_spell(ch, SKILL_WHIRLWIND))
      {
        send_to_char("You slow down unable to stay fully focused on melee.\n", ch);
        affect_from_char(ch, SKILL_WHIRLWIND);
      }
/*   charmees who are not naturally aggro, may not follow aggro order   */
      if(IS_AGG_CMD(cmd) &&
         IS_SET(ch->specials.affected_by, AFF_CHARM) &&
         (IS_PC(ch) || !IS_SET(ch->only.npc->aggro_flags, AGGR_ALL)))
      {
        i = number(1, 101);

        if(IS_UNDEADRACE(ch) &&
          !IS_PC(ch))
        {
          i = MAX(1, i - 50);
        }
	else if (IS_THEURPET_RACE(ch) &&
	    !IS_PC(ch))
	{
	  i = MAX(1, i - 50);
	}
	else if(IS_ELEMENTAL(ch) ||
                GET_RACE(ch) == RACE_DEMON)
        {
          i = MAX(1, i - 25);
        }
        
        if(ch->following &&
         ((IS_UNDEADRACE(ch) && GET_CLASS(ch->following, CLASS_NECROMANCER)) ||
	 (IS_THEURPET_RACE(ch) && GET_CLASS(ch->following, CLASS_THEURGIST))))
        {
          i = 0;
        }
        
        if(IS_ANIMAL(ch) &&
           GET_SPEC(ch->following, CLASS_SHAMAN, SPEC_ANIMALIST))
        {
          i >> 1;
        }
        
        if(ch->following &&
          GET_CLASS(ch->following, CLASS_MINDFLAYER))
        {
          i = i - GET_C_POW(ch->following);     /* illithids get power bonus */
        }

        if( ch->following && (i > (BOUNDED(0, GET_C_CHA(ch->following), 100))) && !number(0, 2) )
        {
          if( ((IS_UNDEADRACE(ch) || IS_THEURPET_RACE(ch)) && IS_NPC(ch)) || IS_ELEMENTAL(ch) )
          {
            REMOVE_BIT(ch->specials.affected_by, AFF_CHARM);
          }

          send_to_char("&+LUh oh. They don't seem to have agreed with that &+Wlast order.\r\n", ch->following);

          if( (IS_UNDEADRACE(ch) || IS_THEURPET_RACE(ch)) && IS_NPC(ch)
            && !IS_SET(ch->only.npc->aggro_flags, AGGR_ALL) )
          {
            SET_BIT(ch->only.npc->aggro_flags, AGGR_ALL);
          }
          return;
        }
      }
      /* can_exec checks level and granted settings */

      // This gets complicated with switched Immortals.
//      if (cmd_info[cmd].minimum_level > GET_LEVEL(ch))
      if( !can_exec_cmd(ch, cmd) )
      {
        // If we have a switched Imm
        if( ch->desc && ch->desc->original && can_exec_cmd(ch->desc->original, cmd) )
        {
          if( ch->desc->original->desc == ch->desc )
          {
            send_to_char("&+WYou feel holy as you execute a Imm command while switched.\n\r", ch );
            // Executing char gets the Imm's char.
            exec_char = ch->desc->original;
          }
          else
          {
            send_to_char("&+YYou can not use Imm commands while switched in silent mode.\r\n", ch);
            return;
          }
        }
        else
        {
          send_to_char("Pardon?\r\n", ch);
          return;
        }
      }
      if( ((cmd_info[cmd].minimum_position & STAT_MASK) > STAT_SLEEPING) ||
          (cmd_info[cmd].command_pointer == do_action) )
      {
        // Kill minor invis if applicable.
        if( (cmd != CMD_LOOK) && (cmd != CMD_MEMORIZE) &&
            (cmd != CMD_GCC) && (cmd != CMD_HELP) &&
            (cmd != CMD_PRAY) && (cmd != CMD_REST) &&
            (cmd != CMD_SLEEP) && (cmd != CMD_SIT) &&
            (cmd != CMD_RECLINE) && (cmd != CMD_EAT) &&
            (cmd != CMD_DRINK) && (cmd != CMD_MEDITATE) &&
            (cmd != CMD_RWC) && (cmd != CMD_ASSIMILATE) &&
            IS_AFFECTED2(exec_char, AFF2_MINOR_INVIS) )
        {
          send_to_char("You reappear, visible to all.\r\n", exec_char);
          affect_from_char(ch, SPELL_INVISIBLE);
        }
        // Kill hide if applicable
        if( ((cmd != CMD_LOOK) && (cmd != CMD_LISTEN) &&
             (cmd != CMD_SNEAK) && (cmd != CMD_GLANCE) &&
             (cmd != CMD_READ) && (cmd != CMD_STEAL) &&
             (cmd != CMD_SCAN) && IS_SET(exec_char->specials.affected_by, AFF_HIDE)) )
        {
          // Ambush.. allows one to stay hidden through tackle/backstab/headbutt?
          if( affected_by_spell(exec_char, SKILL_AMBUSH) )
          {
            if( cmd == CMD_TACKLE || cmd == CMD_BACKSTAB || cmd == CMD_HEADBUTT )
            {
              if( number(0, 101) < GET_CHAR_SKILL(exec_char, SKILL_AMBUSH) )
              {
                send_to_char("&+LYou remain well prepared.&n\r\n", ch);
              }
              else
              {
                notch_skill(exec_char, SKILL_AMBUSH, 1);
                affect_from_char(exec_char, SKILL_AMBUSH);
                REMOVE_BIT(exec_char->specials.affected_by5, AFF5_SHADE_MOVEMENT);
                REMOVE_BIT(exec_char->specials.affected_by, AFF_HIDE);
              }
            }
            else
            {
              notch_skill(exec_char, SKILL_AMBUSH, 2);
              affect_from_char(exec_char, SKILL_AMBUSH);
              REMOVE_BIT(exec_char->specials.affected_by5, AFF5_SHADE_MOVEMENT);
              REMOVE_BIT(exec_char->specials.affected_by, AFF_HIDE);
            }
          }
          // Shade movement allows one to stay hidden while walking in shadows (if there are any).
          else if( IS_SET(exec_char->specials.affected_by5, AFF5_SHADE_MOVEMENT) )
          {
            // 1..6 or 541 .. 548 .. movement commands.  Must be a dark room to stay hidden.
            if( !((cmd >= CMD_NORTH && cmd <= CMD_DOWN) || (cmd >= CMD_NORTHWEST && cmd <= CMD_SE))
              || IS_SUNLIT(exec_char->in_room) || IS_TWILIGHT_ROOM(exec_char->in_room) )
            {
              send_to_char("&+LYou quickly step out of the shadow.&n\r\n", exec_char);
              REMOVE_BIT(exec_char->specials.affected_by5, AFF5_SHADE_MOVEMENT);
              REMOVE_BIT(exec_char->specials.affected_by, AFF_HIDE);
            }
          }
          // Shadow merge allows one to stay hidden while following.
          else if( affected_by_spell(exec_char, SPELL_SHADOW_MERGE) )
          {
            // 1..6 or 541 .. 548 .. movement commands.  Allowing
            if( ((cmd >= CMD_NORTH && cmd <= CMD_DOWN) || (cmd >= CMD_NORTHWEST && cmd <= CMD_SE))
              && IS_AFFECTED5(exec_char, AFF5_FOLLOWING) )
            {
              send_to_char("You move in the &+Lshadows&n.\r\n", exec_char);
            }
            else
            {
              send_to_char("You step out of the &+Lshadows&n.\r\n", exec_char); 
              if( affected_by_spell(exec_char, SPELL_SHADOW_MERGE) )
              {
                affect_from_char(exec_char, SPELL_SHADOW_MERGE);
              }
              REMOVE_BIT(exec_char->specials.affected_by, AFF_HIDE);
            }
          }
          else if( affected_by_spell(exec_char, SPELL_SHADOW_PROJECTION) )
          {
            if ((IS_AGG_CMD(cmd) || (cmd == CMD_OPEN) || (cmd == CMD_GET) || (cmd == CMD_TAKE) || (cmd == CMD_DRAG)) && argument)
            {
              send_to_char("&+LYour interaction causes you to phase back into existence.&n\r\n", exec_char);
              affect_from_char(exec_char, SPELL_SHADOW_PROJECTION);
            }
            else
            {
              send_to_char("&+LYou effortlessly maneuver through the shadows, unseen by your foes.&n\r\n", exec_char);
            }
          }
          else
          {
            if( (cmd == CMD_FIRE) && (number(1, 100) <= GET_CHAR_SKILL(exec_char, SKILL_SHADOW_ARCHERY)) )
            {
              send_to_char("&+LYou fire silently from the shadows...&n\r\n", exec_char);
              notch_skill(exec_char, SKILL_SHADOW_ARCHERY, 1);
            }
            else
            {
              REMOVE_BIT(exec_char->specials.affected_by, AFF_HIDE);
            }
          }
        }

        if( IS_AFFECTED(exec_char, AFF_MEDITATE) )
        {
          if(cmd == CMD_MEDITATE)
          {
            send_to_char("You are already meditating.\n", exec_char);
          }
          else if( cmd != CMD_PRAY && cmd != CMD_MEMORIZE && cmd != CMD_ASSIMILATE
            && cmd != CMD_FOCUS && cmd != CMD_GCC && cmd != CMD_HELP && cmd != CMD_RWC
            && cmd != CMD_OUTPOST && cmd != CMD_NEXUS && cmd != CMD_FRAGLIST
            && cmd != CMD_DEFOREST && cmd != CMD_ARTIFACTS )
          {
            // Advanced med allows you to continue meditating while doing the below commands.
            // At 60 skill, you no longer have to worry about these commands.
            if( GET_CHAR_SKILL(exec_char, SKILL_ADVANCED_MEDITATION) < number(1, 60)
              || (cmd != CMD_GSAY && cmd != CMD_SAY && cmd != CMD_TELL && cmd != CMD_NCHAT && cmd != CMD_EMOTE) )
            {
              send_to_char("You stop meditating.\r\n", exec_char);
              stop_meditation(exec_char);
            }
            else
            {
              send_to_char("You continue your meditation uninterrupted.\n", exec_char);
              notch_skill(exec_char, SKILL_ADVANCED_MEDITATION, 2);
            }
          }
        }
      }

      if( !no_specials && special(exec_char, cmd, argument + begin + look_at) )
      {
        return;
      }

      // Hack for item_teleport objects
      if( exec_char && check_item_teleport(exec_char, argument + begin + look_at, cmd) )
      {
        return;
      }

      // Execute the bloody thing!!!
      if( (cmd_info[cmd].req_confirm == 1) && (IS_NPC(exec_char) || (exec_char->desc->confirm_state == CONFIRM_DONE)) )
      {
        if( exec_char->desc )
        {
          exec_char->desc->confirm_state = CONFIRM_NONE;
        }
        command_confirm = TRUE;
        ((*cmd_info[cmd].command_pointer) (exec_char, argument + begin + look_at, cmd));
      }
      else if( cmd_info[cmd].req_confirm == 1 )
      {
        if( exec_char->desc )
        {
          exec_char->desc->confirm_state = CONFIRM_AWAIT;
        }
        strcpy(exec_char->desc->last_command, argument);
        command_confirm = FALSE;
        ((*cmd_info[cmd].command_pointer)(exec_char, argument + begin + look_at, cmd));
      }
      else
      {
        if( exec_char->desc )
        {
          exec_char->desc->confirm_state = CONFIRM_NONE;
        }

        while( *(argument + begin + look_at) == ' ' )
        {
          look_at++;
        }
        old_room = exec_char->in_room;

        ((*cmd_info[cmd].command_pointer)(exec_char, argument + begin + look_at, cmd));

        // If mobs should and can aggro the executor of the above command.
        if( (cmd_info[cmd].check_aggro) && IS_ALIVE(exec_char)
          && (IS_PC( exec_char ) || ( (( master = get_linked_char(exec_char, LNK_PET) ) != NULL) && IS_PC(master) ))
          && (exec_char->in_room > RROOM_LIMBO) && !CHAR_IN_SAFE_ROOM(exec_char) )
        {
          // 5% chance to fail sneak.
          if( (exec_char->in_room == old_room) || !SNEAK(exec_char) || !number(0, 19) )
            // 5% chance for calming to work.
            if( !has_innate(ch, INNATE_CALMING) || !number(0, 19) )
              // And the charisma calm chance.
              if( number(1,100) > CALMCHANCE(exec_char) )
                check_aggro_from_command( exec_char );
        }
      }
    }
    return;
  }

  // Unknown or un-implemented command
  /*
   * sick multicommand kludge, not yet supported 'officially', but I
   * wasn't able to live without this. So sue me. -Torm
   */
  // If input starts with a number and was done by an Imm.
  if( IS_TRUSTED(exec_char) && sscanf(argument + begin, "%d ", &j) == 1 )
  {
    // If that number was between 1 and 20 && prevent recursing (3 3 ' hi -> 3 times do 3 times say hi)
    if( (j >= 1) && (j <= 20) && !sscanf(argument + begin + look_at, " %d ", &k))
    {
      k = exec_char->in_room;
      for( i = 0; (i < j) && CAN_ACT(exec_char) && (k == exec_char->in_room); i++ )
      {
        command_interpreter(exec_char, argument + begin + look_at);
      }
      return;
    }
  }
  if( (cmd > 0) && (cmd_info[cmd].command_pointer == 0) )
  {
    send_to_char("Sorry, but that command has yet to be implemented...\r\n", exec_char);
  }
  else
  {
    send_to_char("Pardon?\r\n", exec_char);
  }

  if( exec_char->desc )
  {
    exec_char->desc->confirm_state = CONFIRM_NONE;
  }
}

void argument_interpreter(char *argument, char *first_arg, char *second_arg)
{
  int      look_at, found, begin;

  if (!argument)
  {
    *first_arg = '\0';
    *second_arg = '\0';
    return;
  }
  found = begin = 0;

  if (strlen(argument) >= MAX_INPUT_LENGTH)
  {
    logit(LOG_SYS, "too long arg in argument_interpreter.");
    *(first_arg) = '\0';
    *(second_arg) = '\0';
    return;
  }
  do
  {
    /*
     * Find first non blank
     */
    for (; *(argument + begin) == ' '; begin++) ;

    /*
     * Find length of first word
     */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /*
       * Make all letters lower case, AND copy them to first_arg
       */
      *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(first_arg + look_at) = '\0';
    begin += look_at;

  }
  while (fill_word(first_arg));

  do
  {
    /*
     * Find first non blank
     */
    for (; *(argument + begin) == ' '; begin++) ;

    /*
     * Find length of first word
     */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /*
       * Make all letters lower case, AND copy them to second_arg
       */
      *(second_arg + look_at) = LOWER(*(argument + begin + look_at));

    *(second_arg + look_at) = '\0';
    begin += look_at;

  }
  while (fill_word(second_arg));
}

/*
int is_number(char *str)
{
  int look_at;

  if (*str == '\0')
    return (0);

  for (look_at = 0; *(str + look_at) != '\0'; look_at++)
    if ((*(str + look_at) < '0') || (*(str + look_at) > '9'))
      return (0);
  return (1);
}
*/
bool is_number(char *str)
{
  if (!str || !*str)            /* Test for NULL pointer or string. */
    return FALSE;
  if (*str == '-')              /* -'s in front are valid. */
    str++;
  while (*str)
    if (!isdigit(*(str++)))
      return FALSE;

  return TRUE;
}

bool is_real_number( char *str )
{
  bool period = FALSE;

  // Empty strings not ok.
  if( !str || !*str )
    return FALSE;

  // Negative numbers are ok.
  if( *str == '-' )
    str++;

  // We need at least one number.
  if( !isdigit(*str) || (( *str == '.' ) && isdigit( str[1] )) )
  {
    return FALSE;
  }

  while( *str )
  {
    // Allow for 1 period.
    if( *str == '.' && !period )
    {
      period = TRUE;
      str++;
    }
    else if( !isdigit(*( str++ )) )
    {
      return FALSE;
    }
  }

  return TRUE;
}

/*
 * find the first sub-argument of a string, return pointer to first char
 * in primary argument, following the sub-arg
 */
char *one_argument(const char *argument, char *first_arg)
{
  int found, begin, look_at;

  found = begin = 0;

  if( !argument )
  {
    *first_arg = '\0';
    return NULL;
  }
  if( strlen(argument) >= MAX_INPUT_LENGTH )
  {
    logit(LOG_SYS, "one_argument: argument too long.");
    *(first_arg) = '\0';
    return NULL;
  }
  do
  {
    /*
     * Find first non blank
     */
    for( ; isspace(*(argument + begin)); begin++ )
      ;

    /*
     * Find length of first word
     */
    for( look_at = 0; *(argument + begin + look_at) > ' '; look_at++ )
    {
      // Make all letters lower case, AND copy them to first_arg
      *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    }

    *(first_arg + look_at) = '\0';
    begin += look_at;
  }
  while( fill_word(first_arg) );

  return ((char *) (argument + begin));
}

int fill_word(char *argument)
{
  return (search_block(argument, fill_words, TRUE) >= 0);
}

// Determine if a given string is an abbreviation of another
bool is_abbrev(const char *arg1, const char *arg2)
{
  int i;

  if( !arg1 || !arg2 )
    return FALSE;

  for( i = 0; *(arg1 + i); i++ )
  {
    if (LOWER(*(arg1 + i)) != LOWER(*(arg2 + i)))
    {
      return FALSE;
    }
  }

  return TRUE;
}

// Determine if arg2 is a suffix of arg1.
bool ends_with(const char *arg1, const char *arg2)
{
  int i1, i2;

  if( !arg1 || !arg2 )
  {
    return FALSE;
  }

  i1 = strlen(arg1);
  i2 = strlen(arg2);

  // Suffix must not be longer than the orignal string.
  if( i2 > i1 )
  {
    return FALSE;
  }

  // We check to see that the length is > 0, then decrement to check with the index.
  while( i2-- > 0 )
  {
    // must decrement i1 as well since we want the index not the length.
    --i1;
    if( LOWER(*(arg1 + i1)) != LOWER(*(arg2 + i2)) )
    {
      return FALSE;
    }
  }

  return TRUE;
}

/*
 * return first 'word' plus trailing substring of input string
 */
void half_chop(char *string, char *arg1, char *arg2)
{
  if (!string)
  {
    *arg1 = '\0';
    *arg2 = '\0';
    return;
  }
  if (strlen(string) >= MAX_INPUT_LENGTH)
  {
    logit(LOG_SYS, "too long arg in half_chop.");
    *(arg1) = '\0';
    *(arg2) = '\0';
    return;
  }
  for (; isspace(*string); string++) ;

  for (; !isspace(*arg1 = *string) && *string; string++, arg1++) ;

  *arg1 = '\0';

  for (; isspace(*string); string++) ;

  for (; (*arg2 = *string); string++, arg2++) ;
}

bool special(P_char ch, int cmd, char *arg)
{
  register P_obj i;
  register P_char k;
  int      j;

  if( !IS_ALIVE(ch) )
  {
    return false;
  }

  /*
   * special in room?
   */
  if (world[ch->in_room].funct)
    if ((*world[ch->in_room].funct) (ch->in_room, ch, cmd, arg))
      return (1);

  /*
   * special in equipment list?
   */
  for (j = 0; j <= (MAX_WEAR - 1); j++)
  {
    if (ch->equipment[j] && (ch->equipment[j]->R_num >= 0) &&
        obj_index[ch->equipment[j]->R_num].func.obj)
      if ((*obj_index[ch->equipment[j]->R_num].func.obj) (ch->equipment[j],
            ch, cmd, arg))
        return (1);
  }
  /*
   * special in inventory?
   */
  for (i = ch->carrying; i; i = i->next_content)
  {
    if ((i->R_num >= 0) && obj_index[i->R_num].func.obj)
      if ((*obj_index[i->R_num].func.obj) (i, ch, cmd, arg))
        return (1);
  }
  if (!ALONE(ch))
  {

    /*
     * special in mobile present?
     */
    for (k = world[ch->in_room].people; k; k = k->next_in_room)
    {
      if(!IS_ALIVE(k) ||
         k->in_room == NOWHERE)
      {
        continue;
      }
      if((k != ch) &&
         IS_NPC(k) &&
         IS_AWAKE(k) &&
         mob_index[GET_RNUM(k)].func.mob &&
         !affected_by_spell(k, TAG_CONJURED_PET) &&
         (!IS_IMMOBILE(k) ||
	  (IS_NPC(k) && (GET_VNUM(k) == OUTPOST_BUILDING_MOB))))
      {
        if ((*mob_index[GET_RNUM(k)].func.mob) (k, ch, cmd, arg))
        {
          return (1);
        }
      }
    }
    /*
     * quest mobile present?
     */
    for (k = world[ch->in_room].people; k; k = k->next_in_room)
      if ((k != ch) && IS_NPC(k) && IS_AWAKE(k) && mob_index[GET_RNUM(k)].qst_func)
        if ((*mob_index[GET_RNUM(k)].qst_func) (k, ch, cmd, arg))
          return (1);
  }
  /*
   * special in object present?
   */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if ((i->R_num >= 0) && obj_index[i->R_num].func.obj)
      if ((*obj_index[i->R_num].func.obj) (i, ch, cmd, arg))
        return (1);

  return (0);
}

/*
 * 6 macros for command assignment:
 *   CMD_Y()     - May be used while fighting
 *   CMD_N()     - May NOT be used while fighting
 *   CMD_CNF_Y() - REQUIRES confirmation, and may be used while fighting
 *   CMD_CNF_N() - REQUIRES confirmation, and may NOT be used while fighting
 *   CMD_TRIG()  - Used for reserved keywords that don't DO anything, but are used to trigger specials.
 *   CMD_SOC()   - Social, calls do_action
 *   CMD_GRT()   - Must be granted to be usable.
 *
 * first 4 have same format,  (command number (macro from interp.h),
 * minimum position/status, name of the routine to invoke for this
 * command, minimum level to use this command)
 *
 * CMD_TRIG() only requires (command number, min level) CMD_SOC() only
 * requires (command number, min position)
 *
 * for routines requiring confirmation, the burden of handling that
 * confirmation falls on the routine, do NOT just add one, things WILL
 * screw up (and probably crash).
 *
 * JAB
 */

#define CMD_N(number, min_pos, pointer, min_level, chk_aggro) { \
   cmd_info[(number)].command_pointer = (pointer);              \
   cmd_info[(number)].minimum_position = (min_pos);             \
   cmd_info[(number)].in_battle = FALSE;                        \
   cmd_info[(number)].minimum_level = (min_level);              \
   cmd_info[(number)].grantable = 0;                            \
   cmd_info[(number)].req_confirm = FALSE;                      \
   cmd_info[(number)].check_aggro = chk_aggro;}

#define CMD_Y(number, min_pos, pointer, min_level, chk_aggro) { \
   cmd_info[(number)].command_pointer = (pointer);              \
   cmd_info[(number)].minimum_position = (min_pos);             \
   cmd_info[(number)].in_battle = TRUE;                         \
   cmd_info[(number)].minimum_level = (min_level);              \
   cmd_info[(number)].grantable = 0;                            \
   cmd_info[(number)].req_confirm = FALSE;                      \
   cmd_info[(number)].check_aggro = chk_aggro;}

// Commands that need confirmation do not draw aggro, ever.
#define CMD_CNF_Y(number, min_pos, pointer, min_level) {        \
   cmd_info[(number)].command_pointer = (pointer);              \
   cmd_info[(number)].minimum_position = (min_pos);             \
   cmd_info[(number)].in_battle = TRUE;                         \
   cmd_info[(number)].minimum_level = (min_level);              \
   cmd_info[(number)].grantable = 0;                            \
   cmd_info[(number)].req_confirm = TRUE;                       \
   cmd_info[(number)].check_aggro = FALSE;}

#define CMD_CNF_N(number, min_pos, pointer, min_level) {        \
   cmd_info[(number)].command_pointer = (pointer);              \
   cmd_info[(number)].minimum_position = (min_pos);             \
   cmd_info[(number)].in_battle = FALSE;                        \
   cmd_info[(number)].minimum_level = (min_level);              \
   cmd_info[(number)].grantable = 0;                            \
   cmd_info[(number)].req_confirm = TRUE;                       \
   cmd_info[(number)].check_aggro = FALSE;}

#define CMD_TRIG(number, min_level, chk_aggro) {                \
   cmd_info[(number)].command_pointer = do_not_here;            \
   cmd_info[(number)].minimum_position = STAT_DEAD + POS_PRONE; \
   cmd_info[(number)].in_battle = TRUE;                         \
   cmd_info[(number)].minimum_level = (min_level);              \
   cmd_info[(number)].grantable = 0;                            \
   cmd_info[(number)].req_confirm = FALSE;                      \
   cmd_info[(number)].check_aggro = chk_aggro;}

// Socials always draw aggro.
#define CMD_SOC(number, min_position) {                         \
   cmd_info[(number)].command_pointer = do_action;              \
   cmd_info[(number)].minimum_position = (min_position);        \
   cmd_info[(number)].in_battle = TRUE;                         \
   cmd_info[(number)].minimum_level = 0;                        \
   cmd_info[(number)].grantable = 0;                            \
   cmd_info[(number)].req_confirm = FALSE;                      \
   cmd_info[(number)].check_aggro = TRUE;}

#define CMD_GRT(number, min_pos, pointer, min_level) {   \
   cmd_info[(number)].command_pointer = (pointer);     \
   cmd_info[(number)].minimum_position = (min_pos);    \
   cmd_info[(number)].in_battle = TRUE; \
   cmd_info[(number)].minimum_level = (min_level);      \
   cmd_info[(number)].grantable = 1;    \
   cmd_info[(number)].req_confirm = FALSE;}
/*
 * change the 0 back to -1 when replacing grant - or, 1!
 */

void assign_command_pointers(void)
{
  int      position;

  for (position = 0; position < MAX_CMD_LIST; position++)
    cmd_info[position].command_pointer = NULL;

  /* wizcommands */

  CMD_GRT(CMD_APPROVE, STAT_DEAD + POS_PRONE, do_approve, AVATAR);
  CMD_GRT(CMD_ADVANCE, STAT_DEAD + POS_PRONE, do_advance, GREATER_G);
  CMD_GRT(CMD_ALLOW, STAT_DEAD + POS_PRONE, do_allow, GREATER_G);
  CMD_GRT(CMD_AT, STAT_DEAD + POS_PRONE, do_at, AVATAR);
  CMD_GRT(CMD_BAN, STAT_DEAD + POS_PRONE, do_ban, GREATER_G);
  CMD_GRT(CMD_CD, STAT_DEAD + POS_PRONE, do_not_here, GREATER_G);
  CMD_GRT(CMD_CLONE, STAT_DEAD + POS_PRONE, do_clone, LESSER_G);
  CMD_GRT(CMD_DEBUG, STAT_DEAD + POS_PRONE, do_debug, FORGER);
  CMD_GRT(CMD_DECLINE, STAT_DEAD + POS_PRONE, do_decline, AVATAR);
  CMD_GRT(CMD_DEMOTE, STAT_DEAD + POS_PRONE, do_demote, GREATER_G);
  CMD_GRT(CMD_ECHO, STAT_DEAD + POS_PRONE, do_echo, AVATAR);
  CMD_GRT(CMD_ECHOA, STAT_DEAD + POS_PRONE, do_echoa, LESSER_G);
  CMD_GRT(CMD_ECHOU, STAT_DEAD + POS_PRONE, do_echou, LESSER_G);
  CMD_GRT(CMD_ECHOE, STAT_DEAD + POS_PRONE, do_echoe, LESSER_G);
  CMD_GRT(CMD_ECHOT, STAT_DEAD + POS_PRONE, do_echot, LESSER_G);
  CMD_GRT(CMD_ECHOG, STAT_DEAD + POS_PRONE, do_echog, LESSER_G);
  CMD_GRT(CMD_ECHOZ, STAT_DEAD + POS_PRONE, do_echoz, LESSER_G);
  CMD_GRT(CMD_FINGER, STAT_DEAD + POS_PRONE, do_finger, AVATAR);
  CMD_GRT(CMD_FORCE, STAT_DEAD + POS_PRONE, do_force, GREATER_G);
  CMD_GRT(CMD_FREEZE, STAT_DEAD + POS_PRONE, do_freeze, LESSER_G);
  CMD_GRT(CMD_GRANT, STAT_DEAD + POS_PRONE, do_grant, 0);
  CMD_GRT(CMD_GOTO, STAT_DEAD + POS_PRONE, do_goto, AVATAR);
  CMD_GRT(CMD_GSHOUT, STAT_RESTING + POS_SITTING, do_shout, AVATAR);
  CMD_GRT(CMD_INGAME, STAT_DEAD + POS_PRONE, do_ingame, FORGER);
  CMD_GRT(CMD_INROOM, STAT_DEAD + POS_PRONE, do_inroom, GREATER_G);
  CMD_GRT(CMD_KNOCK, STAT_DEAD + POS_PRONE, do_knock, AVATAR);
  CMD_GRT(CMD_LEVELS, STAT_DEAD + POS_PRONE, do_levels, LESSER_G);
  CMD_GRT(CMD_LFLAGS, STAT_DEAD + POS_PRONE, do_law_flags, AVATAR);
  CMD_GRT(CMD_LOAD, STAT_DEAD + POS_PRONE, do_load, LESSER_G);
  CMD_GRT(CMD_LOOKUP, STAT_DEAD + POS_PRONE, do_lookup, IMMORTAL);
  CMD_GRT(CMD_LWITNESS, STAT_DEAD + POS_PRONE, do_list_witness, AVATAR);
  CMD_GRT(CMD_PLEASANT, STAT_DEAD + POS_PRONE, do_pleasantry, LESSER_G);
  CMD_GRT(CMD_POOFIN, STAT_DEAD + POS_PRONE, do_poofIn, AVATAR);
  CMD_GRT(CMD_POOFOUT, STAT_DEAD + POS_PRONE, do_poofOut, AVATAR);
  CMD_GRT(CMD_POOFINSND, STAT_DEAD + POS_PRONE, do_poofInSound, AVATAR);
  CMD_GRT(CMD_POOFOUTSND, STAT_DEAD + POS_PRONE, do_poofOutSound, AVATAR);
  CMD_GRT(CMD_PTELL, STAT_DEAD + POS_PRONE, do_ptell, AVATAR);
  CMD_GRT(CMD_PURGE, STAT_DEAD + POS_PRONE, do_purge, LESSER_G);
  CMD_GRT(CMD_RANDOBJ, STAT_DEAD + POS_PRONE, do_randobj, FORGER);
  CMD_GRT(CMD_REINITPHYS, STAT_DEAD + POS_PRONE, do_reinitphys, GREATER_G);
  CMD_GRT(CMD_RELEASE, STAT_DEAD + POS_PRONE, do_release, AVATAR);
  CMD_GRT(CMD_RELOADHELP, STAT_DEAD + POS_PRONE, do_reload_help, GREATER_G);
  CMD_GRT(CMD_RENAME, STAT_DEAD + POS_PRONE, do_rename, IMMORTAL);
  CMD_GRT(CMD_REROLL, STAT_DEAD + POS_PRONE, do_reroll, GREATER_G);
  CMD_GRT(CMD_RESTORE, STAT_DEAD + POS_PRONE, do_restore, GREATER_G);
 // CMD_GRT(CMD_EPICRESET, STAT_DEAD + POS_PRONE, do_epic_reset, GREATER_G);
 
  CMD_GRT(CMD_SQL, STAT_DEAD + POS_PRONE, do_sql, OVERLORD);
  CMD_GRT(CMD_MAKEEXIT, STAT_DEAD + POS_PRONE, do_makeexit, GREATER_G);

  CMD_GRT(CMD_REVOKE, STAT_DEAD + POS_PRONE, do_revoke, FORGER);
  CMD_GRT(CMD_SACRIFICE, STAT_DEAD + POS_PRONE, do_sacrifice, GREATER_G);
  CMD_GRT(CMD_SECRET, STAT_DEAD + POS_PRONE, do_secret, LESSER_G);
  CMD_GRT(CMD_SETATTR, STAT_DEAD + POS_PRONE, do_setattr, LESSER_G);
  CMD_GRT(CMD_SETBIT, STAT_DEAD + POS_PRONE, do_setbit, GREATER_G);
  CMD_GRT(CMD_SHUTDOW, STAT_DEAD + POS_PRONE, do_shutdow, FORGER);
  CMD_GRT(CMD_SHUTDOWN, STAT_DEAD + POS_PRONE, do_shutdown, FORGER);
  CMD_GRT(CMD_SILENCE, STAT_DEAD + POS_PRONE, do_silence, AVATAR);
  CMD_GRT(CMD_SNOOP, STAT_DEAD + POS_PRONE, do_snoop, GREATER_G);
  CMD_GRT(CMD_SNOWBALL, STAT_DEAD + POS_PRONE, do_action, AVATAR);
  CMD_GRT(CMD_STRING, STAT_DEAD + POS_PRONE, do_string, GREATER_G);
  CMD_GRT(CMD_SWITCH, STAT_DEAD + POS_PRONE, do_switch, FORGER);
//  CMD_GRT(CMD_TEDIT, STAT_DEAD + POS_PRONE, do_tedit, FORGER);
  CMD_GRT(CMD_TELEPORT, STAT_DEAD + POS_PRONE, do_teleport, LESSER_G);
  CMD_GRT(CMD_TERMINATE, STAT_DEAD + POS_PRONE, do_terminate, FORGER);
  CMD_GRT(CMD_PUNISH, STAT_DEAD + POS_PRONE, do_punish, GREATER_G);
  CMD_GRT(CMD_OK, STAT_DEAD + POS_PRONE, do_ok, FORGER);
  CMD_GRT(CMD_TITLE, STAT_DEAD + POS_PRONE, do_title, AVATAR);
  CMD_GRT(CMD_TRANSFER, STAT_DEAD + POS_PRONE, do_trans, IMMORTAL);
  CMD_GRT(CMD_NEWBIE, STAT_DEAD + POS_PRONE, do_newbie, AVATAR);
  CMD_GRT(CMD_MAKE_GUIDE, STAT_DEAD + POS_PRONE, do_make_guide, AVATAR);
  CMD_GRT(CMD_USERS, STAT_DEAD + POS_PRONE, do_users, AVATAR);
  CMD_GRT(CMD_WHERE, STAT_DEAD + POS_PRONE, do_where, IMMORTAL);
  CMD_GRT(CMD_WHICH, STAT_DEAD + POS_PRONE, do_which, IMMORTAL);
  CMD_GRT(CMD_OLC, STAT_DEAD + POS_PRONE, do_action, GREATER_G);
  CMD_GRT(CMD_WIZLOCK, STAT_DEAD + POS_PRONE, do_wizlock, FORGER);
  CMD_GRT(CMD_WIZCONNECT, STAT_DEAD + POS_PRONE, do_wizhost, GREATER_G);
  CMD_GRT(CMD_ZRESET, STAT_DEAD + POS_PRONE, do_zreset, GREATER_G);
  CMD_GRT(CMD_SETHOME, STAT_DEAD + POS_PRONE, do_sethome, LESSER_G);
  CMD_GRT(CMD_PROPERTIES, STAT_DEAD + POS_PRONE, do_properties, LESSER_G);
  CMD_Y(CMD_QUEST, STAT_DEAD + POS_PRONE, do_quest, 1, FALSE);
  CMD_GRT(CMD_PROCLIB, STAT_DEAD + POS_PRONE, do_proclib, FORGER);
  CMD_GRT(CMD_SUPERVISE, STAT_DEAD + POS_PRONE, do_supervise, FORGER);
  CMD_GRT(CMD_DEATHOBJ, STAT_DEAD + POS_PRONE, do_deathobj, FORGER);
  CMD_GRT(CMD_AFFECT_PURGE, STAT_DEAD + POS_PRONE, do_affectpurge, FORGER);
  CMD_GRT(CMD_DEPISS, STAT_DEAD + POS_PRONE, do_depiss, FORGER);
  CMD_GRT(CMD_REPISS, STAT_DEAD + POS_PRONE, do_repiss, FORGER);
  CMD_GRT(CMD_ASCLIST, STAT_DEAD + POS_PRONE, do_asclist, IMMORTAL);
  CMD_GRT(CMD_INVITE, STAT_DEAD + POS_PRONE, do_invite, FORGER);
  CMD_GRT(CMD_UNINVITE, STAT_DEAD + POS_PRONE, do_uninvite, FORGER);
  CMD_GRT(CMD_RESETARTI, STAT_DEAD + POS_PRONE, do_artireset, OVERLORD);
  CMD_GRT(CMD_RESETSPEC, STAT_DEAD + POS_PRONE, do_unspec, FORGER);
  CMD_GRT(CMD_STATISTIC, STAT_DEAD + POS_PRONE, do_statistic, FORGER);
#ifdef SIEGE_ENABLED
  CMD_GRT(CMD_ADD, STAT_DEAD + POS_PRONE, do_add, FORGER);
#endif
  CMD_GRT(CMD_STORAGE, STAT_DEAD + POS_PRONE, do_storage, GREATER_G);
  CMD_GRT(CMD_NEWBSU, STAT_DEAD + POS_PRONE, do_newb_spellup, LESSER_G);
  CMD_GRT(CMD_NEWBSA, STAT_DEAD + POS_PRONE, do_newb_spellup_all, LESSER_G);
  CMD_GRT(CMD_GIVEPET, STAT_DEAD + POS_PRONE, do_givepet, GREATER_G);
  CMD_GRT(CMD_PETITION_BLOCK, STAT_DEAD + POS_PRONE, do_petition_block, FORGER);
  CMD_GRT(CMD_WHITELIST, STAT_DEAD + POS_PRONE, do_whitelist, IMMORTAL);
  CMD_GRT(CMD_QUESTWHERE, STAT_NORMAL + POS_STANDING, do_questwhere, IMMORTAL);
  CMD_GRT(CMD_OFFLINEMSG, STAT_NORMAL + POS_STANDING, do_offlinemsg, GREATER_G);
  CMD_GRT(CMD_INSTACAST, STAT_NORMAL + POS_STANDING, do_cast, GREATER_G);
  CMD_GRT(CMD_WHOIS, STAT_DEAD + POS_PRONE, do_whois, LESSER_G);

  /*
   * commands requiring confirmation
   */

  CMD_CNF_N(CMD_JUNK, STAT_RESTING + POS_SITTING, do_junk, 56);
  CMD_N(CMD_QUIT, STAT_DEAD + POS_PRONE, do_camp, 0, TRUE);
  CMD_Y(CMD_BERSERK, STAT_NORMAL + POS_STANDING, do_berserk, 0, TRUE);
  CMD_CNF_N(CMD_SUICIDE, STAT_DEAD, do_suicide, 1);

  

  /*
   * level restricted commands
   */

  CMD_N(CMD_BURY, STAT_NORMAL + POS_STANDING, do_bury, 0, TRUE);
  CMD_Y(CMD_GCC, STAT_SLEEPING + POS_PRONE, do_gcc, 0, FALSE);
  CMD_N(CMD_SUMMON, STAT_NORMAL + POS_STANDING, do_innate, 10, TRUE);
#ifdef MEM_DEBUG
  CMD_Y(CMD_MREPORT, STAT_DEAD + POS_PRONE, do_mreport, AVATAR, FALSE);
#endif
  CMD_Y(CMD_NCHAT, STAT_SLEEPING + POS_PRONE, do_nchat, 1, FALSE);
  CMD_Y(CMD_PAGE, STAT_NORMAL + POS_STANDING, do_page, IMMORTAL, FALSE);
  CMD_Y(CMD_SKILLS, STAT_SLEEPING + POS_PRONE, do_skills, 1, FALSE);
  CMD_Y(CMD_SPELLS, STAT_SLEEPING + POS_PRONE, do_spells, 1, FALSE);
  CMD_Y(CMD_TELL, STAT_RESTING + POS_PRONE, do_tell, 1, FALSE);
  CMD_Y(CMD_BEEP, STAT_RESTING + POS_PRONE, do_beep, 1, FALSE);
  CMD_Y(CMD_RWC, STAT_SLEEPING + POS_PRONE, do_rwc, 62, FALSE);
  CMD_Y(CMD_REPLY, STAT_RESTING + POS_PRONE, do_reply, 1, FALSE);
  CMD_N(CMD_LICK, STAT_NORMAL + POS_STANDING, do_lick, 1, TRUE);

  /*
   * normal commands (not allowed while fighting)
   */
  CMD_N(CMD_DESCEND, STAT_NORMAL + POS_PRONE, do_descend, 0, TRUE);
  CMD_N(CMD_ENHANCE, STAT_NORMAL + POS_PRONE, do_enhance, 0, TRUE);
  CMD_N(CMD_REMORT, STAT_NORMAL + POS_PRONE, do_remort, 0, TRUE);
  CMD_N(CMD_SPECIALIZE, STAT_SLEEPING + POS_PRONE, do_specialize, 0, FALSE);
//  CMD_N(CMD_SPECIALIZE, STAT_NORMAL + POS_PRONE, do_spec, 0, FALSE);
  CMD_N(CMD_APPRAISE, STAT_NORMAL + POS_PRONE, do_appraise, 0, FALSE);
  CMD_N(CMD_APPLY, STAT_RESTING + POS_PRONE, do_apply_poison, 0, TRUE);
  CMD_N(CMD_ARTIFACTS, STAT_SLEEPING + POS_PRONE, do_artifact_sql, 0, FALSE);
  CMD_N(CMD_RAID, STAT_SLEEPING + POS_PRONE, do_raid, 0, FALSE); // Raidable?
  CMD_N(CMD_DISMISS, STAT_NORMAL + POS_STANDING, do_dismiss, 0, TRUE);
  CMD_N(CMD_DISAPPEAR, STAT_RESTING + POS_SITTING, do_disappear, 0, TRUE);
  CMD_N(CMD_ASSIST, STAT_NORMAL + POS_STANDING, do_assist, 0, TRUE);
  CMD_N(CMD_DICE, STAT_NORMAL + POS_STANDING, do_dice, 0, TRUE);
  CMD_N(CMD_AWARENESS, STAT_NORMAL + POS_STANDING, do_awareness, 0, FALSE);
  CMD_N(CMD_BACKSTAB, STAT_NORMAL + POS_STANDING, do_backstab, 0, TRUE);
  CMD_N(CMD_BALANCE, STAT_NORMAL + POS_STANDING, do_balance, 0, FALSE);
  CMD_N(CMD_BANDAGE, STAT_NORMAL + POS_STANDING, do_bandage, 0, TRUE);
  CMD_Y(CMD_BEARHUG, STAT_NORMAL + POS_STANDING, do_bearhug, 0, TRUE);
  CMD_N(CMD_BODYSLAM, STAT_NORMAL + POS_STANDING, do_bodyslam, 0, TRUE);
  CMD_N(CMD_CARVE, STAT_NORMAL + POS_STANDING, do_carve, 0, TRUE);
  CMD_N(CMD_CAMP, STAT_RESTING + POS_PRONE, do_camp, 0, TRUE);
  CMD_N(CMD_CLIMB, STAT_NORMAL + POS_STANDING, do_climb, 0, TRUE);
  CMD_N(CMD_CLOSE, STAT_RESTING + POS_SITTING, do_close, 0, TRUE);
  CMD_N(CMD_COMMANDS, STAT_SLEEPING + POS_PRONE, do_commands, 0, FALSE);
  CMD_N(CMD_CREDITS, STAT_DEAD + POS_PRONE, do_credits, 0, FALSE);
  CMD_N(CMD_DEPOSIT, STAT_NORMAL + POS_STANDING, do_deposit, 0, FALSE);
  CMD_N(CMD_DIG, STAT_NORMAL + POS_STANDING, do_dig, 0, TRUE);
  CMD_N(CMD_MINE, STAT_NORMAL + POS_STANDING, do_mine, 0, TRUE);
  CMD_N(CMD_FISH, STAT_NORMAL + POS_STANDING, do_fish, 0, TRUE);
  CMD_N(CMD_FORGE, STAT_RESTING + POS_PRONE, do_forge, 0, TRUE);
  CMD_N(CMD_ASCEND, STAT_RESTING + POS_PRONE, do_ascend, 0, TRUE);
  CMD_N(CMD_EXHUME, STAT_NORMAL + POS_STANDING, do_exhume, 0, TRUE);
  CMD_N(CMD_FADE, STAT_NORMAL + POS_STANDING, do_fade, 0, TRUE);
  CMD_N(CMD_DISGUISE, STAT_RESTING + POS_PRONE, do_disguise, 0, TRUE);
  CMD_N(CMD_DO, STAT_RESTING + POS_PRONE, do_do, 0, TRUE); // Shouldn't really be here, but ok.
  CMD_N(CMD_DONATE, STAT_NORMAL + POS_STANDING, do_donate, 0, TRUE);
  CMD_N(CMD_DOORBASH, STAT_NORMAL + POS_STANDING, do_doorbash, 0, TRUE);
  CMD_N(CMD_DOORKICK, STAT_NORMAL + POS_STANDING, do_doorkick, 0, TRUE);
  CMD_N(CMD_DOWN, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_MAKE, STAT_NORMAL + POS_SITTING, do_make, 0, FALSE);
  CMD_N(CMD_DRAG, STAT_NORMAL + POS_STANDING, do_drag, 0, TRUE);
  CMD_N(CMD_DRAIN, STAT_NORMAL + POS_SITTING, do_drain, 0, TRUE);
  CMD_N(CMD_DRINK, STAT_RESTING + POS_SITTING, do_drink, 0, TRUE);
  CMD_N(CMD_EAST, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_EAT, STAT_RESTING + POS_PRONE, do_eat, 0, TRUE);
  CMD_N(CMD_EMOTE, STAT_RESTING + POS_PRONE, do_emote, 0, TRUE);
  CMD_N(CMD_EMOTE2, STAT_RESTING + POS_PRONE, do_emote, 0, TRUE);
  CMD_N(CMD_ENTER, STAT_NORMAL + POS_STANDING, do_enter, 0, FALSE);
  CMD_N(CMD_EXAMINE, STAT_RESTING + POS_PRONE, do_examine, 0, FALSE);
  CMD_N(CMD_EXP, STAT_DEAD + POS_PRONE, do_explist, 0, FALSE);
  CMD_N(CMD_PROJECTS, STAT_SLEEPING + POS_PRONE, do_projects, 0, FALSE);
  CMD_N(CMD_HOME, STAT_SLEEPING + POS_PRONE, do_home, 0, FALSE);
  CMD_N(CMD_FAQ, STAT_SLEEPING + POS_PRONE, do_faq, 0, FALSE);
  CMD_N(CMD_FILL, STAT_NORMAL + POS_STANDING, do_fill, 0, TRUE);
  CMD_Y(CMD_FIRE, STAT_NORMAL + POS_STANDING, do_fire, 0, TRUE);
  CMD_N(CMD_THROW, STAT_NORMAL + POS_STANDING, do_throw, 0, TRUE);    /* TASFALEN */
  CMD_N(CMD_FIRSTAID, STAT_NORMAL + POS_STANDING, do_first_aid, 0, TRUE);
  CMD_N(CMD_FLY, STAT_NORMAL + POS_SITTING, do_fly, IMMORTAL, TRUE);
  CMD_N(CMD_FORAGE, STAT_RESTING + POS_SITTING, do_forage, 0, TRUE);
  CMD_N(CMD_CONSTRUCT, STAT_NORMAL + POS_STANDING, do_construct, 20, TRUE);
  CMD_N(CMD_GUILDHALL, STAT_DEAD + POS_PRONE, do_guildhall, 25, FALSE);
  // old guildhalls (deprecated)
//  CMD_N(CMD_SACK, STAT_NORMAL + POS_STANDING, do_sack, 25);
  CMD_N(CMD_FRAGLIST, STAT_DEAD + POS_PRONE, do_fraglist, 0, FALSE);
  CMD_N(CMD_HARDCORE, STAT_DEAD + POS_PRONE, displayHardCore, 0, FALSE);
  CMD_N(CMD_LEADERBOARD, STAT_DEAD + POS_PRONE, displayLeader, 0, FALSE);
  CMD_N(CMD_SOULBIND, STAT_DEAD + POS_PRONE, do_soulbind, 0, TRUE);
  CMD_N(CMD_REFINE, STAT_DEAD + POS_PRONE, do_refine, 0, TRUE);
  CMD_N(CMD_RELIC, STAT_DEAD + POS_PRONE, displayRelic, 0, FALSE);
  CMD_N(CMD_EPIC, STAT_DEAD + POS_PRONE, do_epic, 0, FALSE);
  CMD_N(CMD_NEXUS, STAT_DEAD + POS_PRONE, do_nexus, 0, FALSE);
  CMD_Y(CMD_TEST, STAT_DEAD + POS_PRONE, do_test, FORGER, FALSE);
  CMD_Y(CMD_TRANQUILIZE, STAT_DEAD + POS_PRONE, do_tranquilize, LESSER_G, FALSE);
  CMD_Y(CMD_HELP, STAT_DEAD + POS_PRONE, do_help, 0, FALSE);
  CMD_N(CMD_HIDE, STAT_RESTING + POS_SITTING, do_hide, 0, FALSE);
  CMD_N(CMD_HITCH, STAT_NORMAL + POS_STANDING, do_hitch_vehicle, 0, TRUE);
  CMD_N(CMD_INFO, STAT_SLEEPING + POS_PRONE, do_help, 0, FALSE);
  CMD_N(CMD_LISTEN, STAT_NORMAL + POS_STANDING, do_listen, 0, FALSE);
  CMD_N(CMD_LOCK, STAT_RESTING + POS_SITTING, do_lock, 0, TRUE);
  CMD_N(CMD_MAIL, STAT_RESTING + POS_SITTING, do_mail, 0, FALSE);
  CMD_N(CMD_MAP, STAT_SLEEPING + POS_PRONE, do_map, MINLVLIMMORTAL, FALSE);
  CMD_N(CMD_LOTUS, STAT_RESTING + POS_SITTING, do_lotus, 0, TRUE);
  CMD_N(CMD_MEDITATE, STAT_RESTING + POS_SITTING, do_meditate, 0, TRUE);
  CMD_N(CMD_MORE, STAT_DEAD + POS_PRONE, do_more, 0, FALSE);
  CMD_Y(CMD_RECALL, STAT_DEAD + POS_PRONE, do_recall, 0, FALSE);
  CMD_N(CMD_MOTD, STAT_SLEEPING + POS_PRONE, do_motd, 0, FALSE);
  CMD_N(CMD_GMOTD, STAT_SLEEPING + POS_PRONE, do_gmotd, 0, FALSE);
  CMD_Y(CMD_MOUNT, STAT_NORMAL + POS_STANDING, do_mount, 0, TRUE);
  CMD_N(CMD_NE, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_Y(CMD_NEWS, STAT_DEAD + POS_PRONE, do_news, 0, FALSE);
  CMD_Y(CMD_CHEATER, STAT_DEAD + POS_PRONE, do_cheaters, 0, FALSE);
  CMD_N(CMD_NORTH, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_NORTHWEST, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_NORTHEAST, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_NW, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_PICK, STAT_NORMAL + POS_STANDING, do_pick, 0, FALSE);
  CMD_Y(CMD_PLAY, STAT_RESTING + POS_SITTING, do_play, 0, TRUE);
  CMD_N(CMD_POSE, STAT_RESTING + POS_SITTING, do_pose, 0, TRUE);
  CMD_N(CMD_PRACTICE, STAT_RESTING + POS_SITTING, do_practice, 0, TRUE);
  CMD_N(CMD_PRACTISE, STAT_RESTING + POS_SITTING, do_practice, 0, TRUE);
  CMD_N(CMD_PUT, STAT_RESTING + POS_SITTING, do_put, 0, TRUE);
  CMD_N(CMD_EMPTY, STAT_RESTING + POS_SITTING, do_empty, 0, TRUE);
  CMD_N(CMD_QUI, STAT_DEAD + POS_PRONE, do_qui, 0, FALSE);
  CMD_N(CMD_READ, STAT_RESTING + POS_PRONE, do_read, 0, FALSE);
  CMD_N(CMD_RECITE, STAT_RESTING + POS_PRONE, do_recite, 0, TRUE);
  CMD_N(CMD_RELOAD, STAT_NORMAL + POS_STANDING, do_load_weapon, 0, TRUE);
  CMD_N(CMD_REST, STAT_RESTING + POS_PRONE, do_rest, 0, TRUE);
  CMD_N(CMD_RIDE, STAT_NORMAL + POS_STANDING, do_mount, 0, TRUE);
  CMD_N(CMD_RULES, STAT_RESTING + POS_PRONE, do_rules, 0, FALSE);
  CMD_N(CMD_SALVAGE, STAT_NORMAL + POS_STANDING, do_salvage, 0, TRUE);
  CMD_N(CMD_SCAN, STAT_RESTING + POS_STANDING, do_scan, 0, FALSE);
  CMD_N(CMD_SCRIBE, STAT_RESTING + POS_SITTING, do_scribe, 0, FALSE);
  CMD_N(CMD_SE, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_SEARCH, STAT_NORMAL + POS_STANDING, do_search, 0, TRUE);
  CMD_N(CMD_SHAPECHANGE, STAT_RESTING + POS_PRONE, do_shapechange, 0, TRUE);
  CMD_N(CMD_SLEEP, STAT_SLEEPING + POS_PRONE, do_sleep, 0, TRUE);
  CMD_N(CMD_SNEAK, STAT_NORMAL + POS_STANDING, do_sneak, 0, FALSE);
  CMD_N(CMD_SOUTH, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_SOUTHWEST, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_SOUTHEAST, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_SPLIT, STAT_RESTING + POS_SITTING, do_split, 0, FALSE);
  CMD_N(CMD_STEAL, STAT_NORMAL + POS_STANDING, do_steal, 0, TRUE);
  CMD_N(CMD_SW, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_STOMP, STAT_NORMAL + POS_SITTING, do_stomp, 0, TRUE);
  CMD_N(CMD_SWEEP, STAT_NORMAL + POS_SITTING, do_sweep, 0, TRUE);
  CMD_N(CMD_SWIM, STAT_NORMAL + POS_SITTING, do_swim, 60, TRUE);
  CMD_N(CMD_TASTE, STAT_RESTING + POS_PRONE, do_taste, 0, FALSE);
  CMD_N(CMD_TEACH, STAT_RESTING + POS_SITTING, do_teach, 0, TRUE);
  CMD_N(CMD_TRACK, STAT_NORMAL + POS_STANDING, do_track, 0, TRUE);
  CMD_N(CMD_TRAPSET, STAT_NORMAL + POS_STANDING, do_trapset, LESSER_G, TRUE);
  CMD_N(CMD_TRAPREMOVE, STAT_NORMAL + POS_STANDING, do_trapremove, LESSER_G, TRUE);
  CMD_N(CMD_UNHITCH, STAT_NORMAL + POS_STANDING, do_unhitch_vehicle, 0, TRUE);
  CMD_N(CMD_UNLOCK, STAT_RESTING + POS_SITTING, do_unlock, 0, TRUE);
  CMD_N(CMD_UP, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_WEAR, STAT_RESTING + POS_SITTING, do_wear, 0, TRUE);
  CMD_N(CMD_WEST, STAT_NORMAL + POS_PRONE, do_move, 0, TRUE);
  CMD_N(CMD_WHISPER, STAT_RESTING + POS_SITTING, do_whisper, 0, TRUE);
  CMD_N(CMD_WHO, STAT_DEAD + POS_PRONE, do_who, 0, FALSE);
  CMD_N(CMD_WITHDRAW, STAT_NORMAL + POS_STANDING, do_withdraw, 0, FALSE);
  CMD_N(CMD_WIZLIST, STAT_DEAD + POS_PRONE, do_wizlist, 0, FALSE);
  CMD_N(CMD_WRITE, STAT_NORMAL + POS_STANDING, do_write, 0, TRUE);
  CMD_N(CMD_JUSTICE, STAT_NORMAL + POS_SITTING, do_justice, 0, FALSE);
  CMD_N(CMD_SOCIETY, STAT_RESTING + POS_PRONE, do_society, 0, FALSE);
  CMD_N(CMD_BIND, STAT_NORMAL + POS_STANDING, do_bind, 0, TRUE);
  CMD_N(CMD_UNBIND, STAT_NORMAL + POS_PRONE, do_unbind, 0, TRUE);
  CMD_N(CMD_COVER, STAT_NORMAL + POS_STANDING, do_cover, 0, TRUE);
  CMD_N(CMD_STAMPEDE, STAT_NORMAL + POS_STANDING, do_stampede, 0, TRUE);
  CMD_N(CMD_CHARGE, STAT_NORMAL + POS_STANDING, do_charge, 0, TRUE);
  // old guildhalls (deprecated)
//  CMD_N(CMD_HOUSE, STAT_NORMAL + POS_PRONE, do_house, 0, TRUE);
  CMD_N(CMD_LORE, STAT_NORMAL + POS_STANDING, do_lore, 0, FALSE);
  CMD_N(CMD_CRAFT, STAT_NORMAL + POS_STANDING, do_craft, 0, TRUE);
  CMD_N(CMD_ENCRUST, STAT_NORMAL + POS_STANDING, do_encrust, 0, TRUE);
  CMD_N(CMD_SPELLBIND, STAT_NORMAL + POS_STANDING, do_spellbind, 0, TRUE);
  CMD_N(CMD_FIX, STAT_NORMAL + POS_STANDING, do_fix, 0, TRUE);
  CMD_N(CMD_MIX, STAT_NORMAL + POS_STANDING, do_mix, 0, TRUE);
  CMD_N(CMD_SMELT, STAT_NORMAL + POS_STANDING, do_smelt, 0, TRUE);
  CMD_N(CMD_TEST_DESC, STAT_NORMAL + POS_PRONE, do_testdesc, LESSER_G, FALSE);
  CMD_N(CMD_TESTCOLOR, STAT_NORMAL + POS_PRONE, do_testcolor, 0, FALSE);
  CMD_N(CMD_MULTICLASS, STAT_NORMAL + POS_PRONE, do_multiclass, 0, FALSE);
  CMD_Y(CMD_TARGET, STAT_DEAD + POS_PRONE, do_target, 0, TRUE);
  CMD_N(CMD_INTRODUCE, STAT_NORMAL + POS_PRONE, do_introduce, 0, TRUE);
  CMD_N(CMD_VOTE, STAT_NORMAL + POS_PRONE, do_vote, 25, FALSE);
  CMD_N(CMD_THRUST, STAT_NORMAL + POS_PRONE, do_thrust, 1, TRUE);
  CMD_N(CMD_UNTHRUST, STAT_NORMAL + POS_PRONE, do_unthrust, 1, TRUE);
  CMD_N(CMD_ENCHANT, STAT_NORMAL + POS_PRONE, do_enchant, 0, TRUE);
  CMD_N(CMD_INFUSE, STAT_NORMAL + POS_PRONE, do_infuse, 0, TRUE);
  CMD_N(CMD_GATHER, STAT_NORMAL + POS_PRONE, do_gather, 0, TRUE);
  CMD_N(CMD_AREA, STAT_RESTING + POS_PRONE, do_area, 0, FALSE);
  CMD_N(CMD_DEATHS_DOOR, STAT_NORMAL + POS_PRONE, do_deaths_door, 25, FALSE);
  CMD_N(CMD_SMOKE, STAT_RESTING + POS_PRONE, do_smoke, 1, TRUE );
  /*
   * normal commands (allowed while fighting)
   */

  CMD_Y(CMD_ABSORBE, STAT_NORMAL + POS_STANDING, do_absorbe, 0, TRUE);
  CMD_Y(CMD_ACHIEVEMENTS, STAT_DEAD + POS_PRONE, do_achievements, 0, FALSE);
  CMD_Y(CMD_ADDICTED_BLOOD, STAT_DEAD + POS_PRONE, do_addicted_blood, 0, FALSE);
  CMD_Y(CMD_CONJURE, STAT_NORMAL + POS_STANDING, do_conjure, 0, TRUE);
  CMD_Y(CMD_AGGR, STAT_RESTING + POS_PRONE, do_aggr, 0, FALSE);
  CMD_Y(CMD_SALVATION, STAT_NORMAL + POS_STANDING, do_salvation, 0, TRUE);
  CMD_Y(CMD_ALERT, STAT_RESTING + POS_PRONE, do_alert, 0, TRUE);
  CMD_Y(CMD_ARENA, STAT_RESTING + POS_PRONE, do_arena, 0, FALSE);
  CMD_Y(CMD_ASK, STAT_RESTING + POS_SITTING, do_ask, 0, TRUE);
  CMD_Y(CMD_ATTRIBUTES, STAT_SLEEPING + POS_PRONE, do_attributes, 0, FALSE);
  CMD_Y(CMD_BASH, STAT_NORMAL + POS_STANDING, do_bash, 0, TRUE);
  CMD_Y(CMD_DREADNAUGHT, STAT_NORMAL + POS_STANDING, do_dreadnaught, 0, TRUE);
  CMD_Y(CMD_SHADOWSTEP, STAT_NORMAL + POS_STANDING, do_shadowstep, 0, TRUE);
  CMD_Y(CMD_PARLAY, STAT_NORMAL + POS_STANDING, do_parlay, 0, FALSE);
  CMD_Y(CMD_THROWPOTION, STAT_NORMAL + POS_STANDING, do_throw_potion, 0, TRUE);
  CMD_Y(CMD_GUARD, STAT_NORMAL + POS_STANDING, do_guard, 0, TRUE);
  CMD_Y(CMD_BREATH, STAT_RESTING + POS_PRONE, do_breathe, 0, TRUE);
  CMD_Y(CMD_BUCK, STAT_NORMAL + POS_STANDING, do_buck, 0, TRUE);
  CMD_Y(CMD_BUG, STAT_DEAD + POS_PRONE, do_bug, 0, FALSE);
  CMD_Y(CMD_CHEAT, STAT_DEAD + POS_PRONE, do_cheat, 58, FALSE);
  CMD_Y(CMD_CAST, STAT_RESTING + POS_SITTING, do_cast, 0, TRUE);
  CMD_Y(CMD_SPELLWEAVE, STAT_RESTING + POS_SITTING, do_cast, 0, TRUE);
  CMD_Y(CMD_WILL, STAT_RESTING + POS_SITTING, do_will, 0, FALSE);
  CMD_Y(CMD_CHANNEL, STAT_RESTING + POS_PRONE, do_channel, 0, FALSE);
  CMD_Y(CMD_CHI, STAT_NORMAL + POS_SITTING, do_chi, 0, TRUE);
  CMD_Y(CMD_CHANT, STAT_NORMAL + POS_STANDING, do_chant, 0, TRUE);
  CMD_Y(CMD_CIRCLE, STAT_NORMAL + POS_STANDING, do_circle, 0, TRUE);
  CMD_Y(CMD_GARROTE, STAT_NORMAL + POS_STANDING, do_garrote, 0, TRUE);
  CMD_Y(CMD_COMBINATION, STAT_NORMAL + POS_STANDING, do_combination, 0, TRUE);
  CMD_Y(CMD_BLADE, STAT_NORMAL + POS_STANDING, do_barrage, 0, TRUE);
  CMD_Y(CMD_BARRAGE, STAT_NORMAL + POS_STANDING, do_barrage, 0, TRUE);
/*  CMD_Y(CMD_CONDITION, STAT_RESTING + POS_SITTING, do_condition, 0); */
  CMD_Y(CMD_CONSENT, STAT_DEAD + POS_PRONE, do_consent, 0, FALSE);
  CMD_Y(CMD_CONSIDER, STAT_RESTING + POS_SITTING, do_consider, 0, TRUE);
  CMD_Y(CMD_DISARM, STAT_NORMAL + POS_STANDING, do_disarm, 0, TRUE);
  CMD_Y(CMD_DISBAND, STAT_SLEEPING + POS_PRONE, do_disband, 0, TRUE);
  CMD_Y(CMD_DISENGAGE, STAT_NORMAL + POS_STANDING, do_disengage, 0, TRUE);
  CMD_Y(CMD_DISMOUNT, STAT_NORMAL + POS_STANDING, do_dismount, 0, TRUE);
  CMD_Y(CMD_DISPLAY, STAT_DEAD + POS_PRONE, do_display, 0, FALSE);
  CMD_Y(CMD_TRUE_STRIKE, STAT_NORMAL + POS_STANDING, do_true_strike, 0, TRUE);
  CMD_Y(CMD_DRAGONPUNCH, STAT_NORMAL + POS_STANDING, do_dragon_punch, 0, TRUE);
  CMD_Y(CMD_DIRTTOSS, STAT_NORMAL + POS_STANDING, do_dirttoss, 0, TRUE);
  CMD_Y(CMD_DROP, STAT_RESTING + POS_PRONE, do_drop, 0, TRUE);
  CMD_Y(CMD_EQUIPMENT, STAT_SLEEPING + POS_PRONE, do_equipment, 0, FALSE);
  CMD_Y(CMD_EXITS, STAT_RESTING + POS_SITTING, do_exits, 0, FALSE);
  CMD_Y(CMD_FEIGNDEATH, STAT_RESTING + POS_PRONE, do_feign_death, 0, TRUE);
  CMD_Y(CMD_FLANK, STAT_RESTING + POS_STANDING, do_flank, 0, TRUE);
  CMD_Y(CMD_FLEE, STAT_NORMAL + POS_PRONE, do_flee, 0, TRUE);
  CMD_Y(CMD_FOLLOW, STAT_RESTING + POS_SITTING, do_follow, 0, TRUE);
  CMD_Y(CMD_FORGET, STAT_RESTING + POS_PRONE, do_forget, 0, FALSE);
  CMD_Y(CMD_FREE, STAT_RESTING + POS_PRONE, do_nothing, 0, FALSE);
  CMD_Y(CMD_GAZE, STAT_RESTING + POS_STANDING, do_gaze, 0, TRUE);
  CMD_Y(CMD_GET, STAT_RESTING + POS_SITTING, do_get, 0, TRUE);
  CMD_Y(CMD_GIVE, STAT_RESTING + POS_SITTING, do_give, 0, TRUE);
  CMD_Y(CMD_GLANCE, STAT_RESTING + POS_PRONE, do_glance, 0, FALSE);
  CMD_Y(CMD_GRAB, STAT_RESTING + POS_PRONE, do_grab, 0, TRUE);
  CMD_Y(CMD_GROUP, STAT_RESTING + POS_PRONE, do_group, 0, FALSE);
  CMD_Y(CMD_GSAY, STAT_RESTING + POS_PRONE, do_gsay, 0, FALSE);
  CMD_Y(CMD_HEADBUTT, STAT_NORMAL + POS_STANDING, do_headbutt, 0, TRUE);
  CMD_Y(CMD_HIT, STAT_NORMAL + POS_STANDING, do_hit, 0, TRUE);
  CMD_Y(CMD_DESTROY, STAT_NORMAL + POS_STANDING, do_hit, 0, TRUE);
  CMD_Y(CMD_HITALL, STAT_NORMAL + POS_STANDING, do_hitall, 0, TRUE);
  CMD_Y(CMD_WARCRY, STAT_NORMAL + POS_STANDING, do_war_cry, 0, TRUE);
  CMD_Y(CMD_TACKLE, STAT_NORMAL + POS_STANDING, do_tackle, 0, TRUE);
  CMD_Y(CMD_LEGSWEEP, STAT_NORMAL + POS_STANDING, do_legsweep, 0, TRUE);
  CMD_Y(CMD_HOLD, STAT_RESTING + POS_PRONE, do_grab, 0, TRUE);
  CMD_Y(CMD_IDEA, STAT_DEAD + POS_PRONE, do_idea, 0, FALSE);
  CMD_Y(CMD_IGNORE, STAT_SLEEPING + POS_PRONE, do_ignore, 0, FALSE);
  CMD_Y(CMD_INNATE, STAT_NORMAL + POS_STANDING, do_innate, 0, TRUE);
  CMD_Y(CMD_INSULT, STAT_RESTING + POS_PRONE, do_insult, 0, TRUE);
  CMD_Y(CMD_INVENTORY, STAT_RESTING + POS_PRONE, do_inventory, 0, FALSE);
  CMD_Y(CMD_KICK, STAT_NORMAL + POS_STANDING, do_kick, 0, TRUE);
  CMD_Y(CMD_KILL, STAT_NORMAL + POS_STANDING, do_kill, 0, TRUE);
  CMD_Y(CMD_KNEEL, STAT_RESTING + POS_PRONE, do_kneel, 0, TRUE);
  CMD_Y(CMD_LAYHAND, STAT_NORMAL + POS_STANDING, do_layhand, 0, TRUE);
  CMD_Y(CMD_LOOK, STAT_RESTING + POS_PRONE, do_look, 0, FALSE);
  CMD_Y(CMD_MEMORIZE, STAT_RESTING + POS_SITTING, do_memorize, 0, FALSE);
  CMD_Y(CMD_ASSIMILATE, STAT_RESTING + POS_SITTING, do_assimilate, 0, FALSE);
  CMD_Y(CMD_COMMUNE, STAT_RESTING + POS_SITTING, do_assimilate, 0, FALSE);
  CMD_Y(CMD_DEFOREST, STAT_RESTING + POS_SITTING, do_assimilate, 0, FALSE);
  CMD_Y(CMD_PROJECT, STAT_RESTING + POS_PRONE, do_project, 0, FALSE);
  CMD_Y(CMD_MURDER, STAT_NORMAL + POS_STANDING, do_murder, 0, TRUE);
  CMD_Y(CMD_NECKBITE, STAT_NORMAL + POS_STANDING, do_gith_neckbite, 0, TRUE);
  CMD_Y(CMD_OGRE_ROAR, STAT_NORMAL + POS_STANDING, do_ogre_roar, 0, TRUE);
  CMD_Y(CMD_OPEN, STAT_RESTING + POS_SITTING, do_open, 0, TRUE);
  CMD_Y(CMD_ORDER, STAT_RESTING + POS_PRONE, do_order, 0, TRUE);
  CMD_Y(CMD_PETITION, STAT_DEAD + POS_PRONE, do_petition, 0, FALSE);
  CMD_Y(CMD_POUR, STAT_RESTING + POS_SITTING, do_pour, 0, TRUE);
  CMD_Y(CMD_PRAY, STAT_RESTING + POS_SITTING, do_memorize, 0, TRUE);
  CMD_Y(CMD_TERRAIN, STAT_RESTING + POS_PRONE, do_terrain, 0, FALSE);
  CMD_Y(CMD_QUAFF, STAT_RESTING + POS_SITTING, do_quaff, 0, TRUE);
  CMD_Y(CMD_RECLINE, STAT_RESTING + POS_PRONE, do_recline, 0, TRUE);
  CMD_Y(CMD_REMOVE, STAT_RESTING + POS_SITTING, do_remove, 0, TRUE);
  CMD_Y(CMD_REPORT, STAT_RESTING + POS_PRONE, do_report, 0, TRUE);
  CMD_Y(CMD_RESCUE, STAT_NORMAL + POS_STANDING, do_rescue, 0, TRUE);
  CMD_Y(CMD_RUSH, STAT_NORMAL + POS_STANDING, do_rush, 0, TRUE);
  CMD_Y(CMD_RETREAT, STAT_NORMAL + POS_STANDING, do_retreat, 0, TRUE);
  CMD_Y(CMD_RETURN, STAT_DEAD + POS_PRONE, do_return, 0, FALSE);
  CMD_Y(CMD_RUB, STAT_RESTING + POS_SITTING, do_rub, 0, TRUE);
  CMD_Y(CMD_SAVE, STAT_SLEEPING + POS_PRONE, do_save, 0, FALSE);
  CMD_Y(CMD_SAY, STAT_RESTING + POS_PRONE, do_say, 0, TRUE);
  CMD_Y(CMD_SAY2, STAT_RESTING + POS_PRONE, do_say, 0, TRUE);
  CMD_Y(CMD_SCENT, STAT_DEAD + POS_PRONE, do_blood_scent, 0, TRUE);
  CMD_Y(CMD_CALL, STAT_DEAD + POS_PRONE, do_call_grave, 0, TRUE);
  CMD_Y(CMD_SCORE, STAT_DEAD + POS_PRONE, do_score, 0, FALSE);
  CMD_Y(CMD_SHIELDPUNCH, STAT_NORMAL + POS_STANDING, do_shieldpunch, 0, TRUE);
  CMD_Y(CMD_SHOUT, STAT_RESTING + POS_SITTING, do_yell, 0, TRUE);
//  CMD_Y(CMD_SING, STAT_RESTING + POS_PRONE, do_bardcheck_action, 0, TRUE);
  CMD_Y(CMD_SIP, STAT_RESTING + POS_SITTING, do_sip, 0, TRUE);
  CMD_Y(CMD_SIT, STAT_RESTING + POS_PRONE, do_sit, 0, TRUE);
  CMD_Y(CMD_SPEAK, STAT_SLEEPING + POS_PRONE, do_speak, 0, FALSE);
  CMD_Y(CMD_SPRINGLEAP, STAT_RESTING + POS_PRONE, do_springleap, 0, TRUE);
  CMD_Y(CMD_STAND, STAT_RESTING + POS_PRONE, do_stand, 0, TRUE);
  CMD_Y(CMD_STAT, STAT_DEAD + POS_PRONE, do_stat, 0, FALSE);
  CMD_Y(CMD_SUBTERFUGE, STAT_NORMAL + POS_STANDING, do_subterfuge, 0, FALSE);
  CMD_Y(CMD_SWEEPING_THRUST, STAT_NORMAL + POS_STANDING, do_sweeping_thrust, 0, TRUE);
  CMD_Y(CMD_TAKE, STAT_RESTING + POS_SITTING, do_get, 0, TRUE);
  CMD_Y(CMD_THROAT_CRUSH, STAT_NORMAL + POS_STANDING, do_throat_crush, 0, TRUE);
  CMD_Y(CMD_TIME, STAT_DEAD + POS_PRONE, do_time, 0, FALSE);
  CMD_Y(CMD_TOGGLE, STAT_DEAD + POS_PRONE, do_toggle, 0, FALSE);
  CMD_Y(CMD_SURNAME, STAT_DEAD + POS_PRONE, do_surname, 0, FALSE);
  CMD_Y(CMD_TRAMPLE, STAT_NORMAL + POS_STANDING, do_trample, 0, TRUE);
  CMD_Y(CMD_TROPHY, STAT_RESTING + POS_PRONE, do_trophy, 0, FALSE);
  CMD_Y(CMD_TYPO, STAT_DEAD + POS_PRONE, do_typo, 0, FALSE);
  CMD_Y(CMD_USE, STAT_RESTING + POS_SITTING, do_use, 0, TRUE);
  CMD_Y(CMD_VIS, STAT_DEAD + POS_PRONE, do_vis, 0, TRUE);
  CMD_Y(CMD_WAKE, STAT_SLEEPING + POS_PRONE, do_wake, 0, TRUE);
//  CMD_Y(CMD_TUPOR, STAT_SLEEPING + POS_PRONE, do_tupor, 0, TRUE);
  CMD_Y(CMD_TUPOR, STAT_SLEEPING + POS_PRONE, do_assimilate, 0, TRUE);
  CMD_Y(CMD_WEATHER, STAT_RESTING + POS_PRONE, do_weather, 0, FALSE);
  CMD_Y(CMD_WHIRLWIND, STAT_NORMAL + POS_STANDING, do_whirlwind, 0, TRUE);
  CMD_Y(CMD_WIELD, STAT_RESTING + POS_PRONE, do_wield, 0, TRUE);
  CMD_Y(CMD_WIZHELP, STAT_DEAD + POS_PRONE, do_wizhelp, IMMORTAL, FALSE);
  CMD_Y(CMD_WIZMSG, STAT_DEAD + POS_PRONE, do_wizmsg, MINLVLIMMORTAL, FALSE);
  CMD_Y(CMD_WORLD, STAT_DEAD + POS_PRONE, do_world, 0, FALSE);
  CMD_Y(CMD_CAPTURE, STAT_NORMAL + POS_STANDING, do_capture, 0, TRUE);
  CMD_Y(CMD_ROUNDKICK, STAT_NORMAL + POS_STANDING, do_roundkick, 0, TRUE);
  CMD_Y(CMD_HAMSTRING, STAT_NORMAL + POS_STANDING, do_hamstring, 0, TRUE);
//  CMD_Y(CMD_DECREE, STAT_NORMAL + POS_STANDING, do_decree, 0, TRUE);
  CMD_Y(CMD_RAGE, STAT_NORMAL + POS_STANDING, do_rage, 0, TRUE);
  CMD_Y(CMD_MAUL, STAT_NORMAL + POS_STANDING, do_maul, 0, TRUE);
  CMD_Y(CMD_RESTRAIN, STAT_NORMAL + POS_STANDING, do_restrain, 0, TRUE);
  CMD_Y(CMD_RIFF, STAT_NORMAL + POS_STANDING, do_riff, 0, TRUE);
  CMD_Y(CMD_CONSUME, STAT_NORMAL + POS_STANDING, do_consume, 0, TRUE);
  CMD_Y(CMD_RAMPAGE, STAT_NORMAL + POS_STANDING, do_rampage, 0, TRUE);
  CMD_Y(CMD_INFURIATE, STAT_NORMAL + POS_STANDING, do_infuriate, 0, TRUE);
  CMD_Y(CMD_WAIL, STAT_RESTING + POS_SITTING, do_play, 0, TRUE);
  CMD_Y(CMD_SNEAKY_STRIKE, STAT_NORMAL + POS_STANDING, do_sneaky_strike, 0, TRUE);
  CMD_Y(CMD_MUG, STAT_NORMAL + POS_STANDING, do_mug, 0, TRUE);
  CMD_Y(CMD_SHRIEK, STAT_NORMAL + POS_STANDING, do_shriek, 0, TRUE);
  CMD_Y(CMD_SLIP, STAT_NORMAL + POS_STANDING, do_slip, 0, FALSE);
  CMD_Y(CMD_HEADLOCK, STAT_NORMAL + POS_STANDING, do_headlock, 0, TRUE);
  CMD_Y(CMD_GROUNDSLAM, STAT_NORMAL + POS_STANDING, do_groundslam, 0, TRUE);
  CMD_Y(CMD_LEGLOCK, STAT_NORMAL + POS_PRONE, do_leglock, 0, TRUE);

  CMD_N(CMD_BUILD, STAT_NORMAL + POS_STANDING, do_build, 0, TRUE);
  CMD_Y(CMD_PRESTIGE, STAT_NORMAL + POS_STANDING, do_prestige, 0, FALSE);
  CMD_N(CMD_ALLIANCE, STAT_RESTING + POS_PRONE, do_alliance, 0, FALSE);
  CMD_Y(CMD_ACC, STAT_SLEEPING + POS_PRONE, do_acc, 0, FALSE);
  CMD_Y(CMD_SMITE, STAT_NORMAL + POS_STANDING, do_holy_smite, 0, TRUE);
  CMD_N(CMD_OUTPOST, STAT_RESTING + POS_PRONE, do_outpost, 0, FALSE);
  CMD_Y(CMD_OFFENSIVE, STAT_RESTING + POS_PRONE, do_offensive, 0, FALSE);
  CMD_Y(CMD_FOCUS, STAT_RESTING + POS_SITTING, do_assimilate, 0, TRUE);
  CMD_Y(CMD_BOON, STAT_SLEEPING + POS_PRONE, do_boon, 0, FALSE);
  CMD_Y(CMD_CTF, STAT_NORMAL + POS_STANDING, do_ctf, 0, FALSE);
  CMD_Y(CMD_TETHER, STAT_NORMAL + POS_STANDING, do_tether, 0, FALSE);
  CMD_Y(CMD_AUCTION, STAT_NORMAL + POS_STANDING, new_ah_call, 0, FALSE);

  /*
   * 'commands' which exist only to trigger specials
   */

  CMD_TRIG(CMD_BRIBE, 0, TRUE);
  CMD_TRIG(CMD_BUY, 0, TRUE);
  CMD_TRIG(CMD_DELETE, 0, FALSE);
  CMD_TRIG(CMD_DISEMBARK, 0, TRUE);
  CMD_TRIG(CMD_EXCHANGE, 0, TRUE);
  CMD_TRIG(CMD_JOIN, 0, TRUE);
  CMD_TRIG(CMD_LIST, 0, FALSE);
  CMD_TRIG(CMD_PERUSE, 0, FALSE);
  CMD_TRIG(CMD_MOVE, 0, TRUE);
  CMD_TRIG(CMD_OFFER, 0, TRUE);
  CMD_TRIG(CMD_RENT, 0, FALSE);
  CMD_TRIG(CMD_REPAIR, 0, TRUE);
  CMD_TRIG(CMD_SELL, 0, TRUE);
  CMD_TRIG(CMD_VALUE, 0, FALSE);
  CMD_TRIG(CMD_ZAP, 0, TRUE);
  CMD_TRIG(CMD_PAY, 0, TRUE);
  CMD_TRIG(CMD_PARDON, 0, TRUE);
  CMD_TRIG(CMD_REPORTING, 0, TRUE);
  CMD_TRIG(CMD_TURN_IN, 0, TRUE);     /* TASFALEN3 */
  CMD_TRIG(CMD_CLAIM, 0, TRUE);
  CMD_TRIG(CMD_HIRE, 0, TRUE);
  //CMD_TRIG(CMD_AUCTION, 0, TRUE);
  CMD_TRIG(CMD_TRAIN, 0, TRUE);
  //CMD_TRIG(CMD_SPECIALIZE, 0);
  CMD_TRIG(CMD_HARVEST, 0, TRUE);
  CMD_TRIG(CMD_BATTLERAGER, 0, FALSE);
  CMD_TRIG(CMD_DEPLOY, 0, TRUE);

  /*
   * socials (all call do_action, rather than a specific func)
   */

  CMD_SOC(CMD_ACCUSE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_ACK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_AFK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_AGREE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_AMAZE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_APOLOGIZE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_APPLAUD, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_ARCH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_AYT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BANG, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BARK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BATHE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BBL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BEER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BEG, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BIRD, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BITE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BLEED, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BLINK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BLOW, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BLUSH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BOGGLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BONK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BORED, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BOUNCE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BOW, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BRB, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BURP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BYE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CACKLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CALM, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CARESS, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CENSOR, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CHEEK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CHEER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CHOKE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CHUCKLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CLAP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_COMB, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_COMFORT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CONGRATULATE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_COUGH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CRINGE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CRY, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CUDDLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CURIOUS, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CURSE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_CURTSEY, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_DANCE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_DREAM, STAT_SLEEPING + POS_PRONE);
  CMD_SOC(CMD_DROOL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_DROPKICK, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_DUCK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_DUH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_EMBRACE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_ENVY, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_EYEBROW, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FART, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FIDGET, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FLAME, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FLASH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_FLEX, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FLIP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_FLIRT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FLUTTER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FONDLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FOOL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FRENCH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FROWN, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FULL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FUME, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_FUZZY, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GAG, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GAPE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GASP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GIGGLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GLARE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GOOSE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_GRIN, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GROAN, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GROPE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GROVEL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GROWL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GRUMBLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GRUNT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GULP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HALO, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HAND, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HAPPY, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HELLO, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HERO, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HI5, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HICCUP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HICKEY, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HISS, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HOLDON, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HOP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_HOSE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HUG, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HUM, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_HUNGER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_IMITATE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_IMPALE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_JAM, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_JK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_JUMP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_KISS, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_LAG, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_LAUGH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_LEAN, STAT_NORMAL + POS_STANDING);
//  CMD_SOC(CMD_LICK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_LOST, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_LOVE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_LUCKY, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_MASSAGE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_MELT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_MOAN, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_MOON, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MOSH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MOURN, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_MUTTER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_NAP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_NIBBLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_NOD, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_NOG, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_NOOGIE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_NOSE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_NUDGE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_NUZZLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_OGLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PANIC, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_PANT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PAT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PEER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PET, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PILLOW, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PINCH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PLONK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_POINT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_POKE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PONDER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_POUNCE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_POUT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PROTECT, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_PUCKER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PUKE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PULL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PUNCH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PURR, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PUSH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_PUZZLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_RAISE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_READY, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_ROAR, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_ROFL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_ROLL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_ROSE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_RUFFLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SALUTE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SCARE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SCOLD, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SCRATCH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SCREAM, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SEDUCE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SHAKE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SHIVER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SHOVE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SHRUG, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SHUDDER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SHUSH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SIGH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SING, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SKIP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SLAP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SLOBBER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SMELL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SMILE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SMIRK, STAT_RESTING + POS_PRONE);
//  CMD_SOC(CMD_SMOKE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SNAP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SNARL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SNEER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SNEEZE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SNICKER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SNIFF, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SNOOGIE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SNORE, STAT_SLEEPING + POS_PRONE);
  CMD_SOC(CMD_SNORT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SNUGGLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SPAM, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SPANK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SPIN, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SPIT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SQUEEZE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SQUIRM, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_STARE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_STEAM, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_STRANGLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_STRETCH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_STRUT, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SULK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SWAT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_SWEAT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_GROUND, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TANGO, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TAP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TARZAN, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TAUNT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TEASE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_THANK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_THINK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_THIRST, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TICKLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TIP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TIPTOE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TONGUE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TOSS, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TOUCH, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TRIP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TUG, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TWEAK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TWIBBLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TWIDDLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_TWIRL, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_UNDRESS, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_VETO, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WAIT, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WAVE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WELCOME, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WET, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WHAP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WHATEVER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WHEEZE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WHIMPER, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WHINE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WHISTLE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WIGGLE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_WINCE, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WINK, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WOOPS, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_WORSHIP, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_YAWN, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_YODEL, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_ZONE, STAT_RESTING + POS_PRONE);

  CMD_SOC(CMD_OMG, STAT_RESTING + POS_PRONE);
  CMD_SOC(CMD_BSLAP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_HUMP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BONG, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SPOON, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_EEK, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MOVING, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_WIT, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_FLAP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SPEW, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_ADDICT, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BANZAI, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BHUG, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BKISS, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BLINDFOLD, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BOOGIE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BREW, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BULLY, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BUNGEE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SOB, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_CURL, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_CHERISH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_CLUE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_CHASTISE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_COFFEE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_COWER, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_CARROT, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_DOH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_EKISS, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_FAINT, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_GAWK, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_GHUG, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_HANGOVER, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_HCUFF, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_HMM, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_ICECUBE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MAIM, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MAHVELOUS, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MEOW, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MMMM, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MOOCH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_COW, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MUHAHA, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MWALK, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_NAILS, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_NASTY, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_NI, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_OHNO, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_OINK, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_OOO, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_PECK, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_PING, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_PLOP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_POTATO, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_EPARDON, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_REASSURE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_OHHAPPY, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_GLUM, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SMOOCH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SQUEAL, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SQUISH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_STICKUP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_STRIP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SUFFER, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_STOKE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_ESWEEP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SWOON, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TENDER, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_THROTTLE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TIMEOUT, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TORTURE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TUMMY, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TYPE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_WEDGIE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_WISH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_WRAP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_YABBA, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_YAHOO, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_YEEHAW, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SLOAD, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_CASTRATE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BOOT, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_GLEE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SCOWL, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MUMBLE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_JEER, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TANK, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_PRAISE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BECKON, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_CRACK, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_GRENADE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_CHORTLE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_WHIP, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_CLAW, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SHADES, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TAN, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SUNBURN, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_THREATEN, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_TWITCH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BABBLE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_WRINKLE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_GUFFAW, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_JIG, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_BLAME, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_SHUFFLE, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_FROTH, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_HOWL, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_MONKEY, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_DISGUST, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_NAFK, STAT_NORMAL + POS_STANDING);
  CMD_SOC(CMD_GRIMACE, STAT_NORMAL + POS_STANDING);

  CMD_N(CMD_SQUIDRAGE, STAT_NORMAL + POS_STANDING, do_squidrage, 41, TRUE);
}

/*
 * only used in this one routine, so nuke them now. JAB
 */

#undef CMD_CNF_N
#undef CMD_CNF_Y
#undef CMD_GRT
#undef CMD_N
#undef CMD_SOC
#undef CMD_TRIG
#undef CMD_Y

// This assumes exec_char is a valid PC that's in a valid room.
void check_aggro_from_command( P_char exec_char )
{
  P_char exec_master, mob, mob_master, next_mob;
  int room, calming_chance;

  room = exec_char->in_room;
  // Chance of aggro mobs not attacking.
  calming_chance = CALMCHANCE( exec_char );
  // Innate calming is +10% chance for this type of aggro check.
  if( has_innate(exec_char, INNATE_CALMING) )
  {
    calming_chance += 10;
  }

  // Check for a PC master (controller of exec_char) that's in the same room.
  exec_master = get_linked_char( exec_char, LNK_PET );
  if( !IS_ALIVE(exec_master) || !IS_PC(exec_master) || (exec_master->in_room != room) )
  {
    exec_master = NULL;
  }

  for( mob = world[exec_char->in_room].people; mob && IS_ALIVE(exec_char); mob = next_mob )
  {
    next_mob = mob->next_in_room;

    // Only consider aggro NPCs (not including self) that are not lagged nor fighting something else nor asleep/ko'd.
    if( !IS_ALIVE(mob) || !IS_NPC(mob) || !IS_AGGRESSIVE(mob) || (mob == exec_char) || !CAN_ACT(mob)
      || IS_FIGHTING(mob) || !IS_AWAKE(mob) )
    {
      continue;
    }

    // Pets only aggro chars when master is not in room, or when they are attacking master.
    if( (( mob_master = get_linked_char(mob, LNK_PET) ) != NULL) && (mob_master->in_room == room)
      && (GET_OPPONENT( exec_char ) != mob_master) )
    {
      continue;
    }

    // Check for calming & check to actually be aware of char.
    if( calming_chance >= number(1, 100) || number(25, 225) > GET_C_INT(mob) )
    {
      continue;
    }

    // Stand if need be.
    if( GET_POS( mob ) < POS_STANDING )
    {
      do_stand( mob, "", CMD_STAND );
    }

    // If the mob is too hurt to attack.
    if( (( 100 * GET_HIT(mob) ) / GET_MAX_HIT( mob )) <= number(1, 100) )
    {
      continue;
    }

    // If the mob can see the master of the executor, it may attack them (int check).
    if( exec_master && CAN_SEE(mob, exec_master) && aggressive_to(mob, exec_master)
      && (number( 1, 100 ) < GET_C_INT(mob) / 2) )
    {
      do_action(mob, "", CMD_SNEER);
      MobStartFight(mob, exec_master);
      // Just return if we kill the master, no need to pound on the charmie.
      // If we don't return, we need to set master to NULL.
      if( !IS_ALIVE(exec_master) )
      {
        return;
      }
      // Room kicked master or some such.
      if( exec_master->in_room != room )
      {
        exec_master = NULL;
      }
      continue;
    }

    // If the mob can see the executor of the command and doesn't like them or their boss.
    if( CAN_SEE(mob, exec_char) && (aggressive_to( mob, exec_char ) || ( exec_master && aggressive_to(mob, exec_master) )) )
    {
      MobStartFight(mob, exec_char);
      // If they killed the executor, or room kicked them etc.
      if( !IS_ALIVE(exec_char) || (exec_char->in_room != room) )
      {
        // Then start on the owner!
        if( IS_ALIVE(exec_master) && exec_master->in_room == room )
        {
          exec_char = exec_master;
          calming_chance = CALMCHANCE( exec_char ) + (has_innate( exec_char, INNATE_CALMING )) ? 10 : 0;
          exec_master = NULL;
        }
        else
        {
          return;
        }
      }
    }
  }
}

// Hybrid between one_argument and half_chop:
// Takes the string, skips initial spaces, pulls out arg1 (making all lowercase),
//   does _NOT_ skip any words, skips spaces and returns the remainder of the argument(s).
// Note: This _DOES_ treat a substring in quotes as a single argument.
char *lohrr_chop( char *string, char *arg1 )
{
  char *index;
  bool single_quoted;

  // First test for empty.
  if( (string == NULL) || (*string == '\0') )
  {
    *arg1 = '\0';
    return string;
  }

  // Skip spaces / tabs / etc
  for( ; isspace(*string); string++ )
  {
    ;
  }
  index = string;

  // If it starts with a single quote, then we must end with a closing single quote.
  single_quoted = (*index == '\'');
  if( single_quoted )
    index++;
  // Copy until we hit a space unless we have an opening quote and are looking for a closer.
  while( !isspace(*index) || single_quoted )
  {
    // If we've found the closing quote, go past and don't copy it.
    if( single_quoted && *index == '\'' )
    {
      index++;
      break;
    }
    // If we've reached the end of the string.
    if( *index == '\0' )
    {
      // Opening quote no closing quote. :/
      // So we start over not looking for single quotes.
      if( single_quoted )
      {
        index = string;
        single_quoted = FALSE;
      }
      else
      {
        break;
      }
    }
    *arg1 = LOWER(*index);
    arg1++;
    index++;
  }
  *arg1 = '\0';

  return index;
}
