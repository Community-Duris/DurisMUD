/* ***************************************************************************
 *  file: db.h , Database module.                            Part of Duris *
 *  Usage: Loading/Saving chars booting world.                               *
 *************************************************************************** */

#ifndef _SOJ_DB_H_
#define _SOJ_DB_H_

#include <stdio.h>
#include <string>
using namespace std;

#define STRING(var) char (var)[MAX_STRING_LENGTH];

/* data files used by the game system */

#define SHOP_FILE         "areas/world.shp"
#define WORLD_FILE        "areas/world.wld"     /* room definitions          */
#define MOB_FILE          "areas/world.mob"     /* monster prototypes        */
#define OBJ_FILE          "areas/world.obj"     /* object prototypes         */
#define ZONE_FILE         "areas/world.zon"     /* zone defs & command tables*/
#define JUSTICE_FILE      "areas/world.justice" /* justice definition file   */

#define MAIL_FILE         "Players/mail"

#define CREDITS_FILE      "lib/information/credits"     /* for the 'credits' command */
#define MAP1_FILE         "lib/information/map1"
#define MAP2_FILE         "lib/information/map2"
#define MAP3_FILE         "lib/information/map3"
#define MAP1A_FILE         "lib/information/map1a"
#define MAP2A_FILE         "lib/information/map2a"
#define MAP3A_FILE         "lib/information/map3a"
#define GREETING_FILE     "lib/information/greeting"    /* this for ascii greetings */
#define GREETING1_FILE    "lib/information/greeting.1"
#define GREETING2_FILE    "lib/information/greeting.2"
#define GREETING3_FILE    "lib/information/greeting.3"


#define GREETINGA_FILE    "lib/information/greetinga"   /* this for ANSI greetings */
#define GREETINGA1_FILE   "lib/information/greetinga.1"
#define GREETINGA2_FILE   "lib/information/greetinga.2"
#define GREETINGA3_FILE   "lib/information/greetinga.3"
#define GREETINGA4_FILE   "lib/information/greetinga.4"

// #define HELP_KWRD_FILE    "lib/information/help_index"  /* for HELP <keywrd> */			// commented by weebler
#define HELP_PAGE_FILE    "lib/information/help"        /* for HELP <CR>             */
#define MOTD_FILE         "lib/information/motd"        /* messages of today         */

#define EMAIL_FILE        "Players/emailreg"   /* EMAIL Registration file */

#define GUILD_FRAG_FILE    "lib/information/guild_frags"        /* for the 'guild frag command' command    */
#define NEWS_FILE         "lib/information/news"        /* for the 'news' command    */
#define MORTAL_ARTI_MAIN_FILE  "Players/Artifacts/mortal_main" /* for the 'arti ' command    */
#define MORTAL_ARTI_IOUN_FILE  "Players/Artifacts/mortal_ioun" /* for the 'arti ioun' command    */
#define MORTAL_ARTI_UNIQUE_FILE "Players/Artifacts/mortal_unique" /* for the 'arti unique' command    */
#define PROJECTS_FILE     "lib/information/projects"     /* for the 'projects' command     */
#define FAQ_FILE          "lib/information/faq"     /* for the 'faq' command     */
#define RULES_FILE        "lib/information/rules"
#define WIZLISTA_FILE     "lib/information/wizlista"
#define WIZLIST_FILE      "lib/information/wizlist"     /* for WIZLIST               */
#define WIZMOTD_FILE      "lib/information/wizmotd"     /* wizard motd */

#define IDEA_FILE         "lib/reports/ideas"   /* for the 'idea'-command    */
#define TYPO_FILE         "lib/reports/typos"   /*         'typo'            */
#define BUG_FILE          "lib/reports/bugs"    /*         'bug'             */
#define CHEATERS_FILE      "lib/reports/cheaters"        /*         'bug'        */
#define OK_FILE            "lib/reports/ok_file"        /*         'bug'        */
#define BUG_CHEAT         "lib/reports/cheats"  /*         'cheat report'             */

#define MESS_FILE         "lib/misc/messages"   /* damage message            */
#define SOCMESS_FILE      "lib/misc/actions"    /* messgs for social acts    */
#define POSEMESS_FILE     "lib/misc/poses"      /* for 'pose'-command         */
#define MOB_LOOKUP        "lib/misc/lookup.mob"
#define OBJ_LOOKUP        "lib/misc/lookup.obj"
#define WLD_LOOKUP        "lib/misc/lookup.wld"
#define ZON_LOOKUP        "lib/misc/lookup.zon"
#define BAN_FILE          "lib/misc/ban_sites"
#define WIZCONNECT_FILE   "lib/misc/wizconnect_sites"

#define DISCLAIMER_FILE   "lib/creation/disclaimer"     /* Disclaimer files */
#define RACEWARS_FILE     "lib/creation/racewars"       /* for good/evil race explanation */
#define GENERALTABLE_FILE "lib/creation/generaltable"   /* for race/class comparisons */
#define CLASSTABLE_FILE   "lib/creation/classtable"     /* for class table selection */
#define RACETABLE_FILE    "lib/creation/racetable"      /* for race table selection */
#define ATTRIBMOD_FILE    "lib/creation/attribmod"      /* attrib mod message for wipe 2011 */
#define NAMECHART_FILE    "lib/creation/namechart"      /* Name selection page */
#define REROLL_FILE       "lib/creation/reroll"         /* for reroll explanation */
#define BONUS_FILE        "lib/creation/bonus"          /* for bonus explanation */
#define KEEPCHAR_FILE     "lib/creation/keepchar"       /* for accepting char explanation */
#define HOMETOWN_FILE     "lib/creation/hometown"
#define ALIGNMENT_FILE    "lib/creation/alignment"
#define SHUTDOWN_FILE     "lib/creation/boom"

#define TERM_ARRAY_CHAR '\n'
#define NULL_FILE "\0"

struct ship_reg_node {

    char *name;
    int vnum;
    struct ship_reg_node *next;
};

extern struct ship_reg_node *ship_reg_db;


struct reboot_data {
  char reboot_option[30];
  char reboot_file[255];
  FILE *whichfile;
};

#define RB_OP(data) (data).reboot_option
#define RB_FILE(data) (data).reboot_file
#define RB_FILE_PTR(data) (data).whichfile
#define REREAD_FILE(file) file_to_string_alloc(RB_FILE((file)), \
                                               &RB_FILE_PTR((file)))

#define DO_ALL(str) ((is_abbrev((str), "all")) || *(str) == '*')
#define DO_LIST(str) ((is_abbrev((str), "list")) || *(str) == '?')

#define OK(ch)  send_to_char("Okay.\r\n", (ch))

/* global variables */
extern char *credits;
extern char *worldmap;
extern char *worldmapa;
extern string news;
extern char *projects;
extern char *faq;
extern string motd;
extern string wizmotd;
extern char *help;
extern char *rules;
extern char *wizlist;
extern char *wizlista;
extern char *greetinga;
extern char *disclaimer;
extern char *bugfile;
extern char *generaltable;
extern char *racewars;
extern char *classtable;
extern char *racetable;
extern char *attribmod;
extern char *namechart;
extern char *reroll;
extern char *bonus;
extern char *keepchar;
extern char *hometown_table;
extern char *alignment_table;

#define REAL 0
#define VIRTUAL 1

extern const char* MENU;
extern const char* GREETINGS;
extern const char* BACKGR_STORY;
#define WELC_MESSG \
"\r\n\
        Welcome to Duris Dikumud\r\n\
\r\n\r\n"

#define ZONE_SILENT    BIT_1
#define ZONE_SAFE      BIT_2
#define ZONE_TOWN      BIT_3
#define ZONE_NEW_RESET BIT_4
#define ZONE_MAP       BIT_5
#define ZONE_CLOSED    BIT_6

void free_world();

#endif /* #ifndef _SOJ_DB_H_ */
