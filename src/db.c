
/*
 * ***************************************************************************
 *   File: db.c                                               Part of Duris *
 *   Usage: Loading/Saving chars, booting world, resetting etc.
 *   Copyright  1990, 1991 - see 'license.doc' for complete information.
 *   Copyright 1994 - 2008 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "epic.h"
#include "interp.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "justice.h"
#include "mm.h"
#include "account.h"
#include "sql.h"
#include "ships.h"
#include "assocs.h"
#include "objmisc.h"
#include "siege.h"

/*
 * external variables
 */

extern P_desc descriptor_list;
extern P_event current_event;
extern P_event event_list;
extern const char *equipment_types[];
extern const char *town_name_list[];
extern const int min_stats_for_class[][8];
extern const struct race_names race_names_table[];
extern struct stat_data stat_factor[];
extern int hometown[];
extern int no_specials;
extern int pulse;
extern int shutdownflag;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern long boot_time;
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern uint event_counter[];
extern struct mm_ds *dead_pconly_pool;
extern struct mm_ds *dead_trophy_pool;
extern int portal_id;
extern void obj_affect_remove(P_obj, struct obj_affect *);
void     delete_knownShapes(P_char ch);
P_nevent get_scheduled(P_obj obj, event_func func);
void     proclib_obj_event(P_char, P_char, P_obj obj, void*);
int proclibObj_add(P_obj obj, char *procName, char *args);
extern void event_mob_mundane(P_char, P_char, P_obj, void*);
extern void event_random_exit(P_char, P_char, P_obj, void*);
extern void event_artifact_poof(P_char, P_char, P_obj, void*);
extern int teacher(P_char ch, P_char pl, int cmd, char *arg);
extern void event_mob_skin_spell(P_char, P_char, P_obj, void*);
extern struct social_messg *soc_mess_list;
void recalc_zone_numbers();
void ne_init_events();
extern void event_reset_zone(P_char, P_char, P_obj, void*);

/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

struct reset_q_type reset_q;

P_room   world;                 /* dyn alloc'ed array of rooms     */
P_town   towns;                 /* List of towns for defenses      */
int      top_of_world = 0;      /* ref to the top element of world */
P_obj    object_list = 0;       /* the global linked list of obj's */
P_char   character_list = 0;    /* global l-list of chars */
struct ban_t *ban_list = 0;
struct wizban_t *wizconnect = 0;
struct zone_data *zone_table;   /* table of reset data             */
struct sector_data *sector_table;       /* mostly weather info           */
int      top_of_zone_table = 0;
struct message_list fight_messages[MAX_MESSAGES];       /* fighting messages  */

char    *guild_frags = NULL;
char    *credits = NULL;        /* * the Credits List                */
//char    *news = NULL;           /* * the news                        */
string  news;
char    *projects = NULL;       /* * Project information             */
char    *faq = NULL;            /* * frequently asked questions      */
//char    *motd = NULL;           /* * ansi motd                       */
string  motd;
//char    *wizmotd = NULL;        /* * ansi wizmotd * */
string  wizmotd;
char    *help = NULL;           /* * the main help page              */
char    *rules = NULL;
char    *wizlist = NULL;        /* * the wizlist                     */
char    *wizlista = NULL;       /* * wizlist for ansi listeners * */
char    *greetinga = NULL;      /* * greeting for our ansi viewers * */
char    *greetinga1 = NULL;
char    *greetinga2 = NULL;
char    *greetinga3 = NULL;
char    *greetinga4 = NULL;
char    *greetings = NULL;      /* * greeting for ascii viewers * */
char    *worldmap = NULL;
char    *worldmapa = NULL;
char    *disclaimer = NULL;     /* * disclaimer message * */
char    *bugfile = NULL;
char    *generaltable = NULL;   /* * race/class comparison charts * */
char    *racewars = NULL;       /* * good/evil race explanation * */
char    *classtable = NULL;     /* * class selection tables * */
char    *racetable = NULL;      /* * race selection tables * */
//char    *attribmod = NULL;      /* * attribute modification for wipe 2011 * */
char    *namechart = NULL;
char    *reroll = NULL;
char    *bonus = NULL;
char    *keepchar = NULL;
char    *hometown_table = NULL;
char    *alignment_table = NULL;
char    *shutdown_message = NULL;
char    *artilist_mortal_main = NULL;
char    *artilist_mortal_ioun = NULL;
char    *artilist_mortal_unique = NULL;

FILE    *mob_f,                 /* * file containing mob prototypes  */
        *obj_f;                 /* * obj prototypes                  */
//      *help_fl;               /* * file for help texts (HELP <kwd>) */  This commented out by weebler

P_index  mob_index;             /* * index table for mobile file     */
P_index  obj_index;             /* * index table for object file     */

P_table  obj_tables;            /* for random obj tables */
P_table  mob_tables;            /* for random mob tables */

P_ereg   email_reg_table;
int      num_mob_tables, num_obj_tables = 0;

// struct help_index_element *help_index = 0;			// commented by weebler
struct info_index_element *info_index = 0;

int      top_of_mobt = 0;       /* * top of mobile index table * */
int      top_of_objt = 0;       /* * top of object index table * */
int      top_of_helpt;          /* * top of help index table         */
int      top_of_infot;          /* * top of info index table         */

int      no_mail = 0;           /* Is mail system working this boot? */

struct time_info_data time_info;        /* * the information about the time * */

struct mm_ds *dead_mob_pool = NULL;
struct mm_ds *dead_obj_pool = NULL;

P_index  generate_indices(FILE *, int *);

void assign_continents();

void     init_rand_tables(void);
void     init_email_reg_db(void);
void     dump_email_reg_db(void);
void     check_registered_email(struct descriptor_data *);
int      email_in_use(char *, char *);

void     release_obj_mem(P_obj obj);
void     release_acct_mem(P_obj obj);

void apply_zone_modifier(P_char ch);

void release_mob_mem(P_char ch, P_char victim, P_obj obj, void *data)
{
  if (ch->in_room != NOWHERE && is_char_in_room(ch, ch->in_room))
  {
    debug("Freeing memory from char in room %d!", world[ch->in_room].number);
  }
  mm_release(dead_mob_pool, ch);
}

void release_obj_mem(P_obj obj)
{
  mm_release(dead_obj_pool, obj);
}

void release_acct_mem(P_obj obj)
{
  mm_release(dead_obj_pool, obj);
}

const char *MENU = "\
   &+W      Welcome to\r\n\
\r\n\
   &+RDuris: Land of Bloodlust\r\n\
\r\n\
&+L=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n\
 &+L[&+W0&+L]&n&+y Leave Duris for a Time.\r\n\
 &+L[&+W1&+L]&n&+y Enter the realms of Duris.\r\n\
 &+L[&+W2&+L]&n&+y Read the background story.\r\n\
 &+L[&+W3&+L]&n&+y Change your password.\r\n\
 &+L[&+W4&+L]&n&+y Enter your character description.\r\n\
 &+L[&+W5&+L]&n&+y Delete this character.\r\n\
&+L=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n\
\r\n\
&+WChoose an option:&n";



const char *BACKGR_STORY = "\r\nThe History of Time:\r\n\r\n \
&+R=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&N\r\n \
&+R=                                                                     =&N\r\n \
&+R=                    DURIS - The Land of Bloodlust                    =&N\r\n \
&+R=                                                                     =&N\r\n \
&+R=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&N\r\n \
\r\n\r\n\
Peering out into the hazy gloom of the early dawn, you gaze upon the\r\n \
land of Duris for what seems like the first time. From the east, the\r\n \
first glimmers of the Black Sun, Dakhira, cast their evil light upon\r\n \
the planet. Perpetual twilight hugs the land, even in the breaking\r\n \
hours of the sunrise. Moving your eyes from the glowing horizon, you\r\n \
gaze out upon the vast windswept plains, worn smooth by the tread of\r\n \
the marching armies. Thus comes the time of the Bloodlust, you think,\r\n \
as idle thought ravages your thoughts. Here is where I shall die.\r\n \
\r\n\r\n\
As Dakhira slowly rises higher in the sky, chills and shakes wreck your\r\n \
frame and dark, evil energy seeps into your body. For now the urges\r\n \
can be fought, but soon you know the desire for blood and death will\r\n \
become too strong. Soon the hordes of the Darklords will be driven into\r\n \
a frenzy by the light of the sun and the great battle will be fought.\r\n \
Many souls shall depart this land today, and many rivulets of blood\r\n \
will feed the earth.\r\n \
\r\n\r\n\
Finally the whole sphere of the Black Sun passes over the horizon's\r\n \
edge and the rays of corrupted light filter through the early morning\r\n \
mist with a renewed intensity. Suddenly waves of weariness and fear\r\n \
overcome you as dark shapes are outlined against the lighter sky. The\r\n \
mounts of the Darklords, the blackest of Durian dragons fly! Trembling\r\n \
in fear you manage to look away from the sky towards your earthly\r\n \
place and then the cries attract you. There in the distance across the\r\n \
plains, where the hills begin, the dark hordes have begun to emerge\r\n \
onto the level ground.\r\n \
\r\n\r\n\
You want to run but your legs do not respond, for the energy of Dakhira\r\n \
now compels you to battle, compels you to the Bloodlust and your death.\r\n \
With a quick glance to your few comrades, you advance slowly upon the\r\n \
battlefield. A sudden determination and resolve overcomes you as the\r\n \
dark mass aproaches. Closer grow the hordes..closer...then sudden blasts\r\n \
of acid straif your back and you scream in agony. With a rush of wind,\r\n \
the Durian dragon ascends higher into the shadowy sky for another run,\r\n \
and the massive dark army closes.\r\n \
\r\n\r\n\
Stumbling to your feet, you are met with the onslaught of the front\r\n \
rank of the orcish horde. 'Comrades die with honor!' you scream as the\r\n \
ugly brute stabs at your breast. The ringing of steel and the crunch\r\n \
of wood sound from all around as the melee begins in ernest. Finally\r\n \
felling the orc sergeant, you plow into the ranks of the enemy with\r\n \
a flurry of slashes and faints. PAIN strikes you hard as an orc out-\r\n \
flanks you, driving his spear deep into your ribcage. Cracking bone\r\n \
and hot spewing blood send you reeling into collapse. Hanging on too\r\n \
life, you feel your blood flow from the wound but are vaguely aware of\r\n \
the battle passing you by as the swarms of goblins and orcs sweep past.\r\n \
\r\n\r\n \
As you fade in and out of unconsciousness, a white light pierces the\r\n \
darkness and manages to arouse you. Shimmering and blurring, you feel\r\n \
a comfort and peace wave over you, unknown for many many years. With\r\n \
a sigh of pleasure, you fade from life..your spirit freed from its\r\n \
mortal trappings. Riding slowly on the ethereal winds, higher into the\r\n \
sky a pull unlike any ever described or felt by you appears. Stronger\r\n \
and stronger the pull becomes, your soul only vaguely aware as it is\r\n \
sucked into the heart of the rift in the sky..as Dakhira grows and\r\n \
strengthens.\r\n \
\r\n\r\n \
Credits: Cython and Zarland\r\n \
See Also: HELP THEME on the mud itself\r\n\r\n\r\n";

const char *GREETINGS = "\r\n\r\n\
                                              __----~~~~~~~~~~~------__\r\n\
      Welcome to Duris DikuMUD     _//~    ~//====......          __--~ ~~\r\n\
                   -_       _..--+~o`\\     ||_   ~~~~~~::::... /~\r\n\
                ___-==_     `--=;_`_  \\   ||  -_             _/~~-\r\n\
        __---~~~.==~|-_=_     ~-~ _/~ |-   _||   -_        _/~\r\n\
   __--~     .=~    |  -_-_       /  /-   / ||     -_     / \r\n\
  =        .~       |    -_-_    /  /-   /   ||      -_  / \r\n\
 /  ____  /         |      - ~-_/  /|- _/   .||        -/ \r\n\
 |~~    ~~|--~~~~--_|_     ~==-/   | \\--===~~        ./ \r\n\
          '         ~|       /|    |-~\\~       __--~~\r\n\
                     |~~~~-_/ |   |   ~\\   _-~              /\\ \r\n\
 The land of the Bloodlust /  -_    -__  <--~                \\ \r\n\
                       _--~ _/ | .-~~____--~-/                ~~~===. \r\n\
                      ((->/~   '.|||' -_|    ~~-/ ,              . _|| \r\n\
                                 -_     -_      ~~---l__i__i__i--~~_/ \r\n\
                                 _-~-__   ~)  _=--_____________--~~\r\n\
                               //.-~~~-~_--~- |-------~~~~~~~~\r\n\
                                      //.-~~~--\\ \r\n\
                    Original Code: Hans Henrik, Katja Nyboe, \r\n\
              Tom Madsen, Michael Seifert, and Sebastian Hammer.\r\n\
\r\n\
                   ** Modified for Duris dikuMUD by **\r\n\
                     Lots and Lots of people!!!!!!!!!!\r\n\
                                          \r\n\r\n";


int fread_string_to_buffer(FILE * fl, char *buf)
{
  char     tmp[MAX_STRING_LENGTH];
  register char *point = NULL; 
  int      length = 0, t_length = 0, done = FALSE;

  buf[0] = '\0';

  do
  {
    if (!fgets(tmp, MAX_STRING_LENGTH - 5, fl))
    {
      perror("fread_string:");
      logit(LOG_DEBUG, "%s", tmp);
      raise(SIGSEGV);
    }
    t_length = strlen(tmp);

    /* find the last non-whitespace char in tmp */
    for (point = tmp + t_length - 1; point > tmp && isspace(*point);
         point--) ;

    if (*point == '~')
    {
      *point = '\0';
      done = TRUE;
    }
    else
    {
      point = tmp + t_length - 1;
      *point++ = '\r';
      *point++ = '\n';
      *point = '\0';
    }
    t_length = point - tmp;

    if (length + t_length >= MAX_STRING_LENGTH)
    {
      logit(LOG_EXIT, "fread_string: string too large (db.c)");
      raise(SIGSEGV);
    }
    else
    {
      strcat(buf + length, tmp);
      length += t_length;
    }
  }
  while (!done);

  for (point = buf + length - 1; point > buf && *point != '&'; point--) ;
  if (*point == '&' && toupper(*(point + 1)) != 'N')
  {
    strcat(buf, "&n");
    length += 2;
  }

  return length;
}

/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */
/*
void loadGodProcs()
{
  FILE    *f;
  char     vnum_str[15];
  int      rn;

  f = fopen("Players/deathobjs", "r");
  if (!f)
  {
    logit(LOG_STATUS, "Error loading death procs.");
    return;
  }
  while (fgets(vnum_str, 15, f))
  {
    if (!(rn = real_object0(atoi(vnum_str))))
    {
      logit(LOG_STATUS, "Error loading death procs: no such item.");
    }
    else
    {
      obj_index[rn].god_func = death_proc;
    }
  }
  fclose(f);
}
*/

/*
 * body of the booting system
 */

void boot_db(int mini_mode)
{
  logit(LOG_STATUS, "Boot db -- BEGIN.");
  fprintf(stderr, "\nBoot db -- BEGIN.\r\n");
  boot_time = time(0);

  logit(LOG_STATUS, "Resetting the game time:");
  fprintf(stderr, "Resetting the game time:\r\n");
  reset_time();

  fprintf(stderr,
          "Reading files from lib directory. (motd, wizlist, etc)\r\n");
  logit(LOG_STATUS, "Reading newsfile.");
//  news = file_to_string(NEWS_FILE);
  news = get_mud_info("news");
  
  logit(LOG_STATUS, "Reading projectsfile.");
  projects = file_to_string(PROJECTS_FILE);

  logit(LOG_STATUS, "Reading faqfile.");
  faq = file_to_string(FAQ_FILE);
  logit(LOG_STATUS, "Reading credits.");
  credits = file_to_string(CREDITS_FILE);
  logit(LOG_STATUS, "Reading Ansi motd.");
//  motd = file_to_string(MOTD_FILE);
  motd = get_mud_info("motd");
  
  logit(LOG_STATUS, "Reading Ansi wizmotd.");
//  wizmotd = file_to_string(WIZMOTD_FILE);
  wizmotd = get_mud_info("wizmotd");
  
  logit(LOG_STATUS, "Reading help.");
  help = file_to_string(HELP_PAGE_FILE);
  logit(LOG_STATUS, "Reading wizlist.");
  wizlist = file_to_string(WIZLIST_FILE);
  logit(LOG_STATUS, "Reading rules.");
  rules = file_to_string(RULES_FILE);
  logit(LOG_STATUS, "Reading Ansi 1 login screen.");
  greetinga = file_to_string(GREETINGA_FILE);
  logit(LOG_STATUS, "Reading Ansi 2 login screen.");
  greetinga1 = file_to_string(GREETINGA1_FILE);
  logit(LOG_STATUS, "Reading ANSI 3 login screen.");
  greetinga2 = file_to_string(GREETINGA2_FILE);
  logit(LOG_STATUS, "Reading ANSI 4 login screen.");
  greetinga3 = file_to_string(GREETINGA3_FILE);
  logit(LOG_STATUS, "Reading ANSI 5 login screen.");
  greetinga4 = file_to_string(GREETINGA4_FILE);
  logit(LOG_STATUS, "Reading ASCII login screen.");
  greetings = file_to_string("lib/information/greeting");
  logit(LOG_STATUS, "Reading ASCII worldmap.");
  worldmap = file_to_string("lib/information/worldmap");
  logit(LOG_STATUS, "Reading ANSI worldmap.");
  worldmapa = file_to_string("lib/information/worldmapa");
  logit(LOG_STATUS, "Reading Ansi wizlist.");
  wizlista = file_to_string(WIZLISTA_FILE);
  logit(LOG_STATUS, "Reading disclaimer.");
  disclaimer = file_to_string(DISCLAIMER_FILE);
  logit(LOG_STATUS, "Reading bug file.");
  bugfile = file_to_string(BUG_FILE);
  logit(LOG_STATUS, "Reading race/class comparison table.");
  generaltable = file_to_string(GENERALTABLE_FILE);
  logit(LOG_STATUS, "Reading Race table.");
  racetable = file_to_string(RACETABLE_FILE);
//  logit(LOG_STATUS, "Reading Attribute Mod Message(wipe 2011)");
//  attribmod = file_to_string(ATTRIBMOD_FILE);
  logit(LOG_STATUS, "Reading Class table.");
  classtable = file_to_string(CLASSTABLE_FILE);
  logit(LOG_STATUS, "Reading Racewars explanation.");
  racewars = file_to_string(RACEWARS_FILE);
  logit(LOG_STATUS, "Reading Namechart message.");
  namechart = file_to_string(NAMECHART_FILE);
  logit(LOG_STATUS, "Reading Reroll message.");
  reroll = file_to_string(REROLL_FILE);
  logit(LOG_STATUS, "Reading Bonus message.");
  bonus = file_to_string(BONUS_FILE);
  logit(LOG_STATUS, "Reading Keepchar message.");
  keepchar = file_to_string(KEEPCHAR_FILE);
  logit(LOG_STATUS, "Reading Hometown_table message.");
  hometown_table = file_to_string(HOMETOWN_FILE);
  logit(LOG_STATUS, "Reading Alignment_table message.");
  alignment_table = file_to_string(ALIGNMENT_FILE);
  logit(LOG_STATUS, "Reading Shutdown Ansi.");
  shutdown_message = file_to_string(SHUTDOWN_FILE);
  logit(LOG_STATUS, "Getting PC id numb info.");
  setNewPCidNumbfromFile();
  portal_id = 0;                // if someone knows a better place to put this, feel free to move it
  logit(LOG_STATUS, "Reading in short desc tables.");
  boot_desc_data();
  fprintf(stderr, "Opening mobile, object, help and info files.\r\n");
  logit(LOG_STATUS, "Opening mobile, object, help and info files.");
  if (!mini_mode)
  {
    if (!(mob_f = fopen(MOB_FILE, "r")))
    {
      perror("boot");
      fprintf(stderr,
              "ERROR! Trouble opening mobile file world.mob! Exiting.\r\n");
      raise(SIGSEGV);
    }
    if (!(obj_f = fopen(OBJ_FILE, "r")))
    {
      perror("boot");
      fprintf(stderr,
              "ERROR! Trouble opening object file world.obj! Exiting.\r\n");
      raise(SIGSEGV);
    }
  }
  else
  {
    if (!(mob_f = fopen("areas/mini.mob", "r")))
    {
      perror("boot");
      raise(SIGSEGV);
    }
    if (!(obj_f = fopen("areas/mini.obj", "r")))
    {
      perror("boot");
      raise(SIGSEGV);
    }
  }
  /*
    // This part commented out by Weebler
  if (!(help_fl = fopen(HELP_KWRD_FILE, "r")))
  {
    logit(LOG_FILE, "   Could not open help file.");
    fprintf(stderr, "ERROR! Could not open help file! Exiting.");
    raise(SIGSEGV);
  }
  else
    help_index = build_help_index(help_fl, &top_of_helpt);*/

  fprintf(stderr, "Loading zone table.\r\n");
  logit(LOG_STATUS, "Loading zone table.");
  boot_zones(mini_mode);

  fprintf(stderr, "Loading rooms.\r\n");
  logit(LOG_STATUS, "Loading rooms.");
  boot_world(mini_mode);

  fprintf(stderr, "Renumbering rooms.\r\n");
  logit(LOG_STATUS, "Renumbering rooms.");
  renum_world();

  fprintf(stderr, "Generating index table for mobiles.\r\n");
  logit(LOG_STATUS, "Generating index table for mobiles.");
  mob_index = generate_indices(mob_f, &top_of_mobt);

  fprintf(stderr, "Generating index table for objects.\r\n");
  logit(LOG_STATUS, "Generating index table for objects.");
  obj_index = generate_indices(obj_f, &top_of_objt);

  /*
   * load_obj_limits();
   */

  fprintf(stderr, "Renumbering zone table.\r\n");
  logit(LOG_STATUS, "Renumbering zone table.");
  renum_zone_table();

  fprintf(stderr, "Initializing Random Load Tables.\r\n");
  logit(LOG_STATUS, "Initializing Random Load Tables.");
  init_rand_tables();

  if (0)
  {                             /* EMAIL registration  */
    fprintf(stderr, "Initializing EMAIL registration table.\n\r");
    init_email_reg_db();
  }

  fprintf(stderr, "Loading social messages.\r\n");
  logit(LOG_STATUS, "Loading social messages.");
  boot_social_messages();

  if (!mini_mode)
  {
    fprintf(stderr, "Initializing boards.\r\n");
    logit(LOG_STATUS, "Initializing boards..");
    initialize_boards();
  }

  fprintf(stderr, "Loading pose messages.\r\n");
  logit(LOG_STATUS, "Loading pose messages.");
  boot_pose_messages();

  /*
   * before loading any mobs, initialize the memory management for
   * structs that will be used for mobiles (and objects)
   */

  dead_mob_pool = mm_create("CHARS",
                            sizeof(struct char_data),
                            offsetof(struct char_data, next),
                            mm_find_best_chunk(sizeof(struct char_data),
                                               (top_of_mobt >> 3),
                                               (top_of_mobt >> 1)));

  dead_obj_pool = mm_create("OBJS",
                            sizeof(struct obj_data),
                            offsetof(struct obj_data, next),
                            mm_find_best_chunk(sizeof(struct obj_data),
                                               (top_of_objt / 3),
                                               (top_of_objt >> 1)));

  if (!no_specials)
  {
    fprintf(stderr, "Assigning function pointers (spec procs):\r\n");
    logit(LOG_STATUS, "Assigning function pointers:");

    logit(LOG_STATUS, "   Mobiles.");
    fprintf(stderr, "-- Mobile special procedures.\r\n");
    assign_mobiles();

    logit(LOG_STATUS, "   Objects.");
    fprintf(stderr, "-- Object special procedures.\r\n");
    assign_objects();

    logit(LOG_STATUS, "   Room.");
    fprintf(stderr, "-- Room special procedures.\r\n");
    assign_rooms();
  }
  
  fprintf(stderr, "Assigning command pointers from interpreter.\r\n");

  fprintf(stderr, "-- Commands.\n");
  logit(LOG_STATUS, "   Commands.");
  assign_command_pointers();

  fprintf(stderr, "-- Spells.\n");
  logit(LOG_STATUS, "   Spells.");
  assign_spell_pointers();


  fprintf(stderr, "Initializing...\n");

  fprintf(stderr, "-- Innates\n");
  assign_innates();

  fprintf(stderr, "-- Links\n");
  initialize_links();

/*
 * logit(LOG_STATUS, "Init Grants"); assign_grant_commands();
 */
  fprintf(stderr, "-- Weather\n");
  logit(LOG_STATUS, "Setting up weather.");
  weather_setup();

  fprintf(stderr, "-- Banned sites\n");
  logit(LOG_STATUS, "Reading ban sites.");
  read_ban_file();

  logit(LOG_STATUS, "Reading wizconnect sites.");
  read_wizconnect_file();


  fprintf(stderr, "-- Events\n");
  logit(LOG_STATUS, "Initializing event driver.");
  ne_init_events();
  //init_events();

  /*
   * can't do the dynamic proc lib loading until AFTER the event driver
   * is started.  Some of the proc libs might try to start events in the
   * _init() function.  (ie: bloodstone gate)
   */

#ifdef SHLIB
  if (!no_specials)
  {
    fprintf(stderr,
            "Loading dynamic proc libs (and assigning pointers):\r\n");
    logit(LOG_STATUS, "Loading dynamic proc libs");
    load_all_proc_libs();
  }
#endif

  /*
   * have to do ships after events, since zone resets (loading ships)
   * are done in init_events now.
   */

  fprintf(stderr, "-- Ships\n");
  logit(LOG_STATUS, "Initializing ships.");
  initialize_ships();

  logit(LOG_STATUS, "Initializing Arena.");
  initialize_arena();

  logit(LOG_STATUS, "Setting up Carriages and wagons.");
  init_wagons();

  fprintf(stderr, "-- Mail\n");
  logit(LOG_STATUS, "Booting mail system.");
  if (!scan_mail_file())
  {
    logit(LOG_DEBUG, "Mail system error -- mail system disabled!");
    no_mail = 1;
  }

  fprintf(stderr, "-- Player corpses\n");
  logit(LOG_STATUS, "Reloading Player corpses.");
  restoreCorpses();

  logit(LOG_STATUS, "Reloading SavedItems.");
  restoreSavedItems();

  fprintf(stderr, "-- Shopkeepers\n");
  logit(LOG_STATUS, "Reloading Shopkeepers.");
  restore_shopkeepers();

  fprintf(stderr, "-- Associations\n");
  logit(LOG_STATUS, "Reloading associations table.");
  reload_assoc_table();

  if (!mini_mode)
  {
    logit(LOG_STATUS, "Reloading Town Justice.");
    restore_town_justice();

    logit(LOG_STATUS, "Loading patrol Justice area.");
    load_justice_area();
  }

// old guildhalls (deprecated)
//  logit(LOG_STATUS, "Loading house.");
//  restore_houses();

  // old guildhalls (deprecated)
//  logit(LOG_STATUS, " ... loading house construction Q");
//  loadConstructionQ();

  logit(LOG_STATUS, "Setting up player-side artifact list.");
  setupMortArtiList();

  fprintf(stderr, "-- Continents\n");
  assign_continents();
      
//  logit(LOG_STATUS, "Setting up god object procedures.");
//  loadGodProcs();

  logit(LOG_STATUS, "Boot db -- DONE.");
}

void update_stat_data()
{
  char     buf[128];
  int      i;

  for (i = 1; i <= LAST_RACE; i++)
  {
    sprintf(buf, "stats.str.%s", race_names_table[i].no_spaces);
    stat_factor[i].Str = (sh_int) get_property(buf, 100.);
    sprintf(buf, "stats.dex.%s", race_names_table[i].no_spaces);
    stat_factor[i].Dex = (sh_int) get_property(buf, 100.);
    sprintf(buf, "stats.agi.%s", race_names_table[i].no_spaces);
    stat_factor[i].Agi = (sh_int) get_property(buf, 100.);
    sprintf(buf, "stats.con.%s", race_names_table[i].no_spaces);
    stat_factor[i].Con = (sh_int) get_property(buf, 100.);
    sprintf(buf, "stats.pow.%s", race_names_table[i].no_spaces);
    stat_factor[i].Pow = (sh_int) get_property(buf, 100.);
    sprintf(buf, "stats.int.%s", race_names_table[i].no_spaces);
    stat_factor[i].Int = (sh_int) get_property(buf, 100.);
    sprintf(buf, "stats.wis.%s", race_names_table[i].no_spaces);
    stat_factor[i].Wis = (sh_int) get_property(buf, 100.);
    sprintf(buf, "stats.cha.%s", race_names_table[i].no_spaces);
    stat_factor[i].Cha = (sh_int) get_property(buf, 100.);
    sprintf(buf, "stats.kar.%s", race_names_table[i].no_spaces);
    stat_factor[i].Karma = (sh_int) get_property(buf, 100.);
    sprintf(buf, "stats.luc.%s", race_names_table[i].no_spaces);
    stat_factor[i].Luck = (sh_int) get_property(buf, 100.);
  }
}

/* reset the time in the game from file */
void reset_time(void)
{
  long     beginning_of_time = 650336715;

  time_info = mud_time_passed(time(0), beginning_of_time);

  logit(LOG_STATUS, "   Current Gametime:  %d/%d/%d  %d%s",
        time_info.month, time_info.day, time_info.year,
        (time_info.hour % 12) ? time_info.hour % 12 : 12,
        (time_info.hour == 12) ? " noon." :
        (time_info.hour == 0) ? " midnight." :
        (time_info.hour > 11) ? "pm." : "am.");
}

void weather_setup(void)
{
  int      zon, s, i;
  int      tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9, tmp10, tmp11,
    tmp12;
  FILE    *fl;

  /* default conditions for season values */
  const char winds[6] = { 2, 12, 30, 40, 50, 80 };
  const char precip[9] = { 0, 1, 5, 10, 15, 25, 35, 45, 60 };
  const char humid[9] = { 4, 10, 20, 30, 40, 50, 60, 75, 100 };
  const char temps[11] = { -15, -8, 0, 10, 17, 27, 33, 40, 50, 75, 100 };

  if (!(fl = fopen("areas/world.weather", "r")))
  {
    logit(LOG_EXIT, "weather_setup");
    raise(SIGSEGV);
  }
  for (zon = 0; zon <= 99; zon++)
  {
    for (i = 0; i < 4; i++)
    {
      sector_table[zon].climate.season_wind_dir[i] = number(0, 3);
      sector_table[zon].climate.season_wind_variance[i] = number(0, 1);
    }
    sector_table[zon].climate.flags = 0;
    sector_table[zon].climate.energy_add = number(0, 1000);
    fscanf(fl, " %d %d %d %d %d %d %d %d %d %d %d %d \n", &tmp1, &tmp2, &tmp3,
           &tmp4, &tmp5, &tmp6, &tmp7, &tmp8, &tmp9, &tmp10, &tmp11, &tmp12);
    sector_table[zon].climate.season_wind[0] = tmp1;
    sector_table[zon].climate.season_precip[0] = tmp2;
    sector_table[zon].climate.season_temp[0] = tmp3;
    sector_table[zon].climate.season_wind[1] = tmp4;
    sector_table[zon].climate.season_precip[1] = tmp5;
    sector_table[zon].climate.season_temp[1] = tmp6;
    sector_table[zon].climate.season_wind[2] = tmp7;
    sector_table[zon].climate.season_precip[2] = tmp8;
    sector_table[zon].climate.season_temp[2] = tmp9;
    sector_table[zon].climate.season_wind[3] = tmp10;
    sector_table[zon].climate.season_precip[3] = tmp11;
    sector_table[zon].climate.season_temp[3] = tmp12;

    /* get the season */
    s = get_season(zon);

    /* These are pretty standard start values */
    sector_table[zon].conditions.pressure = 980;
    sector_table[zon].conditions.free_energy = 10000;
    sector_table[zon].conditions.precip_depth = 0;
    sector_table[zon].conditions.flags = 0;

    /* These use the default conditions above */
    sector_table[zon].conditions.windspeed =
      winds[(int) sector_table[zon].climate.season_wind[s]];

    sector_table[zon].conditions.wind_dir =
      sector_table[zon].climate.season_wind_dir[s];

    sector_table[zon].conditions.precip_rate =
      precip[(int) sector_table[zon].climate.season_precip[s]];

    sector_table[zon].conditions.temp =
      temps[(int) sector_table[zon].climate.season_temp[s]];

    sector_table[zon].conditions.humidity =
      humid[(int) sector_table[zon].climate.season_precip[s]];

    /* Set ambient light */
    calc_light_zone(zon);
  }
  fclose(fl);
}

/* EMAIL Registration setup */
int email_in_use(char *login, char *host)
{
  int      temp, flag;
  struct registration_node *x;

  flag = 0;
  for (temp = 0; temp < 26; temp++)
  {
    x = email_reg_table[temp].next;
    while (x)
    {
      if (strstr(login, x->login) && strstr(host, x->host))
        flag = 1;
      x = x->next;
    }
  }
  return flag;
}

void check_registered_email(struct descriptor_data *d)
{

  struct registration_node *x;
  int      temp;
  char    *tname;

  tname = str_dup(GET_NAME(d->character));
  tname[0] = tolower(tname[0]); /* lowercase first letter */
  for (temp = 0; temp < 26; temp++)
  {
    x = email_reg_table[temp].next;
    while (x)
    {
      if (strstr(tname, x->name) &&
          (!(strstr(d->host, x->host) && strstr(d->login, x->login))))
      {
        /* we have same name but not same registered host/login */
      }
      if (!strstr(tname, x->name) &&
          ((strstr(d->host, x->host) && strstr(d->login, x->login))))
      {
        /* we have same host/login as someone ELSES character */
      }
      x = x->next;
    }
  }
  FREE(tname);
}

void dump_email_reg_db()
{
  int      temp;
  struct registration_node *x;
  FILE    *rfile;


  if (!(rfile = fopen(EMAIL_FILE, "w")))
  {
    fprintf(stderr, "Cant open email file for writing.\n\r");
    return;
  }
  for (temp = 0; temp < 26; temp++)
  {
    x = email_reg_table[temp].next;
    while (x)
    {
      fprintf(rfile, "%s %s %s\n", x->name, x->login, x->host);
      x = x->next;
    }
  }
  fprintf(rfile, "$\n");
  fclose(rfile);
}

void init_email_reg_db()
{
  int      temp;
  struct registration_node *x;
  FILE    *rfile;
  char     buf[MAX_STRING_LENGTH];

  CREATE(email_reg_table, registration_node, 26, MEM_TAG_REGNODE); /* 26 alphabets */
//  email_reg_table = (struct registration_node *) calloc(26, sizeof(struct registration_node));  /* 26 alphabets */
  
  for (temp = 0; temp < 26; temp++)
  {
    email_reg_table[temp].next = 0;
  }
  if (!(rfile = fopen(EMAIL_FILE, "r")))
  {
    fprintf(stderr, "Cant open email file.\n\r");
    return;
  }
  for (;;)
  {                             /* read in file */
    fgets(buf, 100, rfile);
    if (buf[0] == '$')
      break;                    /* eof */
    CREATE(x, registration_node, 1, MEM_TAG_REGNODE);
    //x = (struct registration_node *) malloc(sizeof(struct registration_node));
    sscanf(buf, "%s %s %s\n", x->name, x->login, x->host);
    temp = (int) buf[0] - (int) 'a';    /* couldn't remember function */
    x->next = email_reg_table[temp].next;
    email_reg_table[temp].next = x;
  }
  fclose(rfile);
};

/* generate random mob and obj tables */
void init_rand_tables()
{

  uint     mtables, otables, mentries, oentries;        /* counts for tables */
  int      v, w;
  FILE    *tfile;               /* table file */
  char     buf[MAX_STRING_LENGTH];
  unsigned int tmp;
  int      pos;

  mob_tables = 0;
  obj_tables = 0;
  mtables = 0;
  otables = 0;
  mentries = 0;
  oentries = 0;

  if (!(tfile = fopen("areas/world.tab", "r")))
  {
    raise(SIGSEGV);
  }

  /* First, count the number of each table. */
  for (;;)
  {
    fgets(buf, 81, tfile);
    if (buf[0] == '$')
      break;                    /*eof */
    if (buf[0] == 'M')
      mtables++;
    if (buf[0] == 'O')
      otables++;
  }
  rewind(tfile);
   CREATE(mob_tables, table_data, (unsigned) mtables, MEM_TAG_TBLDATA);
   CREATE(obj_tables, table_data, (unsigned) otables, MEM_TAG_TBLDATA);
//  mob_tables =
//    (struct table_data *) calloc(mtables, sizeof(struct table_data));
//  obj_tables =
//    (struct table_data *) calloc(otables, sizeof(struct table_data));
  num_mob_tables = mtables;
  num_obj_tables = otables;     /* globals for db.c */

  /* now, go through and create each table */
  mtables = otables = 0;
  for (;;)
  {
    fgets(buf, 81, tfile);
    if (buf[0] == '$')
      break;                    /* EOF */
    switch (buf[0])
    {
    case 'M':                  /* new mob table */
      sscanf(buf, "M %d %d\n", &v, &w);
      mob_tables[mtables].virtual_number = v;
      mob_tables[mtables].empty_weight = w;
      pos = ftell(tfile);       /* remember loc */
      /* count # of entries */
      tmp = 0;
      for (;;)
      {
        fgets(buf, 81, tfile);
        if (buf[0] == 'S')
          break;                /*end of table */
        tmp++;
      }
      fseek(tfile, pos, 0);
      CREATE(mob_tables[mtables].table, table_element, tmp, MEM_TAG_TBLELEM);

      mob_tables[mtables].entries = tmp;
      mob_tables[mtables].weight = mob_tables[mtables].empty_weight;
      tmp = 0;
      for (;;)
      {
        fgets(buf, 81, tfile);
        if (buf[0] == 'S')
          break;
        sscanf(buf, "%d %d", &v, &w);
        mob_tables[mtables].table[tmp].virtual_number = v;
        mob_tables[mtables].table[tmp].weight = w;
        mob_tables[mtables].weight += w;        /* total weight */
        tmp++;
      }
      mtables++;
      break;
    case 'O':                  /* new obj table */
      sscanf(buf, "O %d %d", &v, &w);
      obj_tables[otables].virtual_number = v;
      obj_tables[otables].empty_weight = w;
      pos = ftell(tfile);
      tmp = 0;
      for (;;)
      {
        fgets(buf, 81, tfile);
        if (buf[0] == 'S')
          break;
        tmp++;
      }
      fseek(tfile, pos, 0);
      CREATE(obj_tables[otables].table, table_element, tmp, MEM_TAG_TBLELEM);

      obj_tables[otables].entries = tmp;
      obj_tables[otables].weight = obj_tables[otables].empty_weight;
      tmp = 0;
      for (;;)
      {
        fgets(buf, 81, tfile);
        if (buf[0] == 'S')
          break;
        sscanf(buf, "%d %d", &v, &w);
        obj_tables[otables].table[tmp].virtual_number = v;
        obj_tables[otables].table[tmp].weight = w;
        obj_tables[otables].weight += w;
        tmp++;
      }
      otables++;
      break;
    default:
      break;
    }
  }
  fclose(tfile);
};

/* generate index table for object or monster file */
P_index generate_indices(FILE * fl, int *top)
{
  int      i = 0, num;
  P_index  t_idx;
  char     buf[512];

  rewind(fl);

  /* first time just count */
  rewind(fl);
  num = 0;
  for (;;)
  {
    fgets(buf, 511, fl);
    if (*buf == '$')
      break;
    if (*buf == '#')
      num++;
  }

  logit(LOG_STATUS, "\t\t%d entries allocated", num);

  /* allocate array of index_data */
  CREATE(t_idx, index_data, (unsigned) num, MEM_TAG_IDXDATA);

  rewind(fl);

  for (;;)
  {
    if (fgets(buf, 511, fl))
    {
      if (*buf == '#')
      {
        sscanf(buf, "#%d", &t_idx[i].virtual_number);
        t_idx[i].pos = ftell(fl);
        t_idx[i].number = 0;
        t_idx[i].func.mob = NULL;
        t_idx[i].qst_func = NULL;
        t_idx[i].keys = NULL;
        t_idx[i].desc1 = NULL;
        t_idx[i].desc2 = NULL;
        t_idx[i].desc3 = NULL;
        if (i && (t_idx[i - 1].virtual_number >= t_idx[i].virtual_number))
          logit(LOG_DEBUG, "Warning: index (%d, %d) out of order.",
                t_idx[i - 1].virtual_number, t_idx[i].virtual_number);
        i++;
      }
      else if (*buf == '$')     /* EOF  */
        break;
    }
    else
    {
      perror("generate indices");
      raise(SIGSEGV);
    }
  }
  *top = i - 2;
  return (t_idx);
}

/* load the rooms */
void boot_world(int mini_mode)
{
  FILE    *fl;
  int      num_rooms, room_nr = 0, zone = 0, virtual_nr, flag;
  int      tmp = 0, tmp1 = 0, tmp2 = 0, tmp3 = 0, i, name_length, desc_length;
  char     chk[MAX_STRING_LENGTH], tmp_buf[MAX_STRING_LENGTH];
  char     buf[MAX_INPUT_LENGTH];
  char     name_buf[MAX_STRING_LENGTH], desc_buf[MAX_STRING_LENGTH];
  struct extra_descr_data *new_descr;
  bool     found_name, found_desc;

  world = 0;
  character_list = 0;
  object_list = 0;

  if (mini_mode != 1)
  {
    if (!(fl = fopen(WORLD_FILE, "r")))
    {
      perror("fopen");
      logit(LOG_FILE, "boot_world: could not open world file.");
      logit(LOG_SYS, "boot_world: could not open world file.");
      raise(SIGSEGV);
    }
  }
  else if (mini_mode == 1)
  {
    if (!(fl = fopen("areas/mini.wld", "r")))
    {
      perror("fopen");
      raise(SIGSEGV);
    }
  }
  /* Count the number of rooms, to make allocation more efficient!! */
  num_rooms = 0;
  for (;;)
  {
    fgets(tmp_buf, MAX_STRING_LENGTH, fl);
    if (tmp_buf[0] == '$')
      break;
    if (tmp_buf[0] == '#')
      num_rooms++;
  }

  logit(LOG_STATUS, "\t\t%d rooms allocated", num_rooms);
  rewind(fl);

  /* Allocate array of room structures */
  CREATE(world, room_data, (unsigned) num_rooms, MEM_TAG_ROOMDAT);

  logit(LOG_STATUS, "\t\tMemory allocation complete");

  /*
   * allocate array of pointers to pointers for list of unique room
   * descs, this list will speed searching.  It is freed after all rooms
   * are read into memory.  The point?  Duplicate room descs are only
   * allocated once. All rooms with identical descs will all use the
   * same desc (mainly oceans, but there are other duplications as
   * well). JAB
   */

  do
  {
    fscanf(fl, " #%d\n", &virtual_nr);
    if (mini_mode == 2)
      fprintf(stderr, "#%d  ", virtual_nr);

    name_length = fread_string_to_buffer(fl, name_buf);
    if ((flag = (*name_buf != '$')))
    {                           /* a new record to be read */
      world[room_nr].number = virtual_nr;
      desc_length = fread_string_to_buffer(fl, desc_buf);
      found_name = FALSE;
      found_desc = (desc_length == 0);
      // code looking up duplicate room names and descriptions to save memory
      for (i = room_nr - 1; i > (zone ? zone_table[zone - 1].real_top + 1 : 0)
           && (!found_name || !found_desc) && room_nr - i < 102; i--)
      {
        if (!found_name && !strcmp(name_buf, world[i].name))
        {
          world[room_nr].name = world[i].name;
          found_name = TRUE;
        }
        if (!found_desc && world[i].description &&
            !strcmp(desc_buf, world[i].description))
        {
          world[room_nr].description = world[i].description;
          found_desc = TRUE;
        }
      }
      // end of memory preserving code

      if (!found_name)
      {
        CREATE(world[room_nr].name, char, (unsigned) name_length + 1, MEM_TAG_STRING);
//        world[room_nr].name = (char *) calloc(name_length + 1, sizeof(char));
        strcpy(world[room_nr].name, name_buf);
      }
      if (!found_desc)
      {
        CREATE(world[room_nr].description, char, (unsigned) desc_length + 1, MEM_TAG_STRING);
//        world[room_nr].description =
//          (char *) calloc(desc_length + 1, sizeof(char));
        strcpy(world[room_nr].description, desc_buf);
      }

      /* A few presets, may get changed further down */

/*      world[room_nr].resources = 0;
      world[room_nr].kingdom_num = 0;
      world[room_nr].kingdom_type = 0;*/
      world[room_nr].continent = 0;
      world[room_nr].funct = 0;
      world[room_nr].contents = 0;
      world[room_nr].people = 0;
      world[room_nr].light = 0;
      world[room_nr].justice_area = 0;
      for (tmp = 0; tmp <= (NUM_EXITS - 1); tmp++)
        world[room_nr].dir_option[tmp] = 0;
      world[room_nr].ex_description = 0;
      world[room_nr].chance_fall = 0;
      world[room_nr].current_speed = 0;
      world[room_nr].current_direction = -1;
      if (top_of_zone_table >= 0)
      {
        if (world[room_nr].number <= (zone ? zone_table[zone - 1].top : -1))
        {
          logit(LOG_DEBUG, "Room nr %d (%d) is below zone %d.\n",
                room_nr, world[room_nr].number, zone);
          raise(SIGSEGV);
        }
        while (world[room_nr].number > zone_table[zone].top)
          if (++zone > top_of_zone_table)
          {
            logit(LOG_DEBUG, "Room %d is outside of any zone.\n", virtual_nr);
            raise(SIGSEGV);
          }
        world[room_nr].zone = zone;
        if (zone_table[zone].real_bottom == -1)
          zone_table[zone].real_bottom = room_nr;
        zone_table[zone].real_top = room_nr;
      }

      /* tmp is the zone. Never used, and don't ask why :P */

      fgets(buf, sizeof(buf) - 1, fl);
      if (sscanf(buf, " %d %d %d %d\n", &tmp, &tmp1, &tmp2, &tmp3) == 4)
      {
        world[room_nr].room_flags = tmp1;
        world[room_nr].sector_type = tmp2;
//        world[room_nr].resources = tmp3;
      }
      else if (sscanf(buf, " %d %d %d\n", &tmp, &tmp1, &tmp2) == 3)
      {
        world[room_nr].room_flags = tmp1;
        world[room_nr].sector_type = tmp2;
      }
      /* fix a few things */

      if (IS_SET(world[room_nr].room_flags, NO_MAGIC))
        if (!IS_SET(world[room_nr].room_flags, NO_SUMMON))
          SET_BIT(world[room_nr].room_flags, NO_SUMMON);
      if (IS_SET(world[room_nr].room_flags, JAIL))
        if (!IS_SET(world[room_nr].room_flags, SAFE_ZONE))
          SET_BIT(world[room_nr].room_flags, SAFE_ZONE);
      if ((zone_table[zone].flags & ZONE_MAP) && (SECT_CITY == world[room_nr].sector_type))
        world[room_nr].sector_type = SECT_ROAD;

      //Make roads no gate..
      if( world[room_nr].sector_type == SECT_ROAD){
        SET_BIT(world[room_nr].room_flags, NO_GATE);
        SET_BIT(world[room_nr].room_flags, NO_TELEPORT);
      }
        //ADD NO PORT

      for (;;)
      {
        fscanf(fl, " %s \n", chk);

        if (*chk == 'D')        /* direction field  */
          setup_dir(fl, room_nr, atoi(chk + 1));
        else if (*chk == 'E')
        {                       /* extra description field */
          CREATE(new_descr, struct extra_descr_data, 1, MEM_TAG_EXDESCD);
          new_descr->keyword = fread_string(fl);
          new_descr->description = fread_string(fl);
          new_descr->next = world[room_nr].ex_description;
          world[room_nr].ex_description = new_descr;
        }
        else if (*chk == 'F')
        {
          fscanf(fl, "%d ", &tmp);
          world[room_nr].chance_fall = tmp;
        }
        else if (*chk == 'C')
        {
          fscanf(fl, "%d %d ", &tmp, &tmp2);
          world[room_nr].current_speed = tmp;
          world[room_nr].current_direction = tmp2;
        }
        else if (*chk == 'S')
          break;
      }
      if (world[room_nr].sector_type == SECT_INSIDE)
      {
        SET_BIT(world[room_nr].room_flags, INDOORS);
        SET_BIT(world[room_nr].room_flags, NO_PRECIP);
      }
      if ((world[room_nr].sector_type == SECT_NO_GROUND) &&
          (!world[room_nr].dir_option[5] ||
           (world[room_nr].dir_option[5]->to_room == room_nr)))
      {
        world[room_nr].sector_type = SECT_INSIDE;
      }
      if ((world[room_nr].chance_fall > 0) &&
          (!world[room_nr].dir_option[5] ||
           (world[room_nr].dir_option[5]->to_room == room_nr)))
      {
        world[room_nr].chance_fall = 0;
      }
      if (world[room_nr].room_flags & ROOM_IS_INN)
        world[room_nr].funct = inn;

      room_nr++;
    }
  }
  while (flag);

  fclose(fl);
  top_of_world = --room_nr;

  recalc_zone_numbers();
}

void free_world()
{
  //XXXXX 
  for( int room = 0; room <= top_of_world; room++ )
  {
    if( world[room].ex_description )
    {
      if( world[room].ex_description->keyword )
        FREE(world[room].ex_description->keyword);
      if( world[room].ex_description->description )
        FREE(world[room].ex_description->description);
      
      FREE(world[room].ex_description);
    }

    for( int dir = 0; dir < NUM_EXITS; dir++ )
    {
      if( world[room].dir_option[dir] )
      {
        if( world[room].dir_option[dir]->general_description )
          FREE(world[room].dir_option[dir]->general_description);
        if( world[room].dir_option[dir]->keyword )
          FREE(world[room].dir_option[dir]->keyword);
        
        FREE(world[room].dir_option[dir]);        
      }
    }
  }
  
  FREE(world);
  
  for( int mob = 0; mob <= top_of_mobt; mob++ )
  {
    if( mob_index[mob].keys )
      FREE(mob_index[mob].keys);

    if( mob_index[mob].desc1 )
      FREE(mob_index[mob].desc1);

    if( mob_index[mob].desc2 )
      FREE(mob_index[mob].desc2);

    if( mob_index[mob].desc3 )
      FREE(mob_index[mob].desc3);

  }
  FREE(mob_index);

  for( int obj = 0; obj <= top_of_objt; obj++ )
  {
    if( obj_index[obj].keys )
      FREE(obj_index[obj].keys);
    
    if( obj_index[obj].desc1 )
      FREE(obj_index[obj].desc1);
    
    if( obj_index[obj].desc2 )
      FREE(obj_index[obj].desc2);
    
    if( obj_index[obj].desc3 )
      FREE(obj_index[obj].desc3);

  }
  FREE(obj_index);
  
  FREE(soc_mess_list);
  
  for( int zone = 0; zone <= top_of_zone_table; zone++)
  {
    if( zone_table[zone].name )
      FREE(zone_table[zone].name);
    
    if( zone_table[zone].filename )
      FREE(zone_table[zone].filename);
    
    FREE(zone_table[zone].cmd);
  }
  FREE(zone_table);
  
  FREE(sector_table);  
}


/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
  int      state, key, to_room;
  char    *general_description, *keyword;

  general_description = fread_string(fl);
  keyword = fread_string(fl);
  if (fscanf(fl, " %d %d %d ", &state, &key, &to_room) != 3 || to_room < 0)
    return;

  CREATE(world[room].dir_option[dir], room_direction_data, 1, MEM_TAG_DIRDATA);

  world[room].dir_option[dir]->general_description = general_description;
  world[room].dir_option[dir]->keyword = keyword;

  state &= 3;                   // only grab first two bits, state gets set by zone reset (closed, blocked, secret)
  if (state)
  {
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
    if (state == 2)
      world[room].dir_option[dir]->exit_info |= EX_PICKABLE;
    if (state == 3)
      world[room].dir_option[dir]->exit_info |= EX_PICKPROOF;
  }
  else
    world[room].dir_option[dir]->exit_info = 0;

  world[room].dir_option[dir]->key = key;
  world[room].dir_option[dir]->to_room = to_room;

  if (to_room == 0)
    logit(LOG_DEBUG, "Room %d has exit to the void [Room 0].",
          world[room].number);
}

void renum_world(void)
{
  register int room, door, to_room;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door <= (NUM_EXITS - 1); door++)
      if (world[room].dir_option[door])
      {
        to_room = real_room0(world[room].dir_option[door]->to_room);
        if (to_room)
          world[room].dir_option[door]->to_room = to_room;
        else
        {
          FREE(world[room].dir_option[door]);
          world[room].dir_option[door] = NULL;
        }
      }
}

void renum_zone_table(void)
{
  int      zone, comm;

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (comm = 0; zone_table[zone].cmd[comm].command != 'S'; comm++)
    {
      switch (zone_table[zone].cmd[comm].command)
      {
      case 'A':
        zone_table[zone].cmd[comm].arg3 =
          real_mobile(zone_table[zone].cmd[comm].arg1);
        break;
      case 'B':
        zone_table[zone].cmd[comm].arg3 =
          real_object(zone_table[zone].cmd[comm].arg3);
        break;
      case 'C':
      case 'Y':
        zone_table[zone].cmd[comm].arg3 =
          real_room(zone_table[zone].cmd[comm].arg3);
        break;
      case 'M':
      case 'R':
      case 'F':
        zone_table[zone].cmd[comm].arg1 =
          real_mobile(zone_table[zone].cmd[comm].arg1);
        zone_table[zone].cmd[comm].arg3 =
          real_room(zone_table[zone].cmd[comm].arg3);
        break;
      case 'O':
        zone_table[zone].cmd[comm].arg1 =
          real_object(zone_table[zone].cmd[comm].arg1);
        if (zone_table[zone].cmd[comm].arg3 != NOWHERE)
          zone_table[zone].cmd[comm].arg3 =
            real_room(zone_table[zone].cmd[comm].arg3);
        break;
      case 'G':
        zone_table[zone].cmd[comm].arg1 =
          real_object(zone_table[zone].cmd[comm].arg1);
        break;
      case 'E':
        zone_table[zone].cmd[comm].arg1 =
          real_object(zone_table[zone].cmd[comm].arg1);
        break;
      case 'P':
        zone_table[zone].cmd[comm].arg1 =
          real_object(zone_table[zone].cmd[comm].arg1);
        zone_table[zone].cmd[comm].arg3 =
          real_object(zone_table[zone].cmd[comm].arg3);
        break;
      case 'D':
        zone_table[zone].cmd[comm].arg1 =
          real_room(zone_table[zone].cmd[comm].arg1);
        break;
      }                         /* if the real_xxxx() function returned -1, disable this command */
      if (zone_table[zone].cmd[comm].arg1 == -1)
        zone_table[zone].cmd[comm].command = '!';
    }
}

void recalc_zone_numbers()
{
  fprintf(stderr, "Recalculating zone numbers...\n");
  for( int z = 1; z <= top_of_zone_table; z++ )
  {
    int zone_real_bottom = zone_table[z].real_bottom;
    if( zone_real_bottom < 0 ) continue;
    int bottom_vnum = world[zone_real_bottom].number;
    
    if( zone_table[z].number != (int) ( bottom_vnum / 100 ) )
    {
      fprintf(stderr, "  -- %s has invalid number: %d (should be %d)\n", strip_ansi(zone_table[z].name).c_str(), zone_table[z].number, (int) (bottom_vnum/100));
      zone_table[z].number = (int) ( bottom_vnum / 100 );
    }
    
  }
}

void update_zone_difficulties()
{
  for( int z = 0; z <= top_of_zone_table; z++ )
  {
    struct zone_info zinfo;
    
    if( get_zone_info(zone_table[z].number, &zinfo) && zinfo.difficulty )
    {
      zone_table[z].difficulty = zinfo.difficulty;
    }
  }  
}

#define IS_ZONE_COMMAND(ch) (ch == 'M' || ch == 'O' || ch == 'E' ||\
                             ch == 'P' || ch == 'D' || ch == 'G' ||\
                             ch == 'R' || ch == 'F' || ch == 'A' ||\
                             ch == 'B' || ch == 'C' || ch == 'Y' || ch == 'S')

/* load the zone table and command tables */
void boot_zones(int mini_mode)
{
  FILE    *fl;
  int      num_zones, num_commands, zon = 0, cmd_no = 0, tmp, i, t_idx;
  int      tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
  int      nu1, nu2;            /* not used variables */
  int     *command_array;
  char    *check, buf[MAX_STRING_LENGTH], tmp_buf[MAX_STRING_LENGTH], c;
  char     temp_buf[MAX_STRING_LENGTH];

  if (!mini_mode)
  {
    if (!(fl = fopen(ZONE_FILE, "r")))
    {
      perror("boot_zones");
      raise(SIGSEGV);
    }
  }
  else
  {
    if (!(fl = fopen("areas/mini.zon", "r")))
    {
      perror("boot_zones");
      raise(SIGSEGV);
    }
  }
  logit(LOG_STATUS, "Counting zones...");
  num_zones = 0;
  for (;;)
  {
    fgets(tmp_buf, MAX_STRING_LENGTH, fl);
    if (tmp_buf[0] == '$')
      break;
    if (tmp_buf[0] == '#')
    {
      num_zones++;
    }
  }
  logit(LOG_STATUS, "\t\t%d zones allocated", num_zones);
  rewind(fl);

  /* Count the number of commands in each zone */
  logit(LOG_STATUS, "Counting commands for each zone...");
  CREATE(command_array, int, (unsigned) num_zones, MEM_TAG_ARRAY);
//  command_array = (int *) calloc(num_zones, sizeof(int));

  t_idx = 0;
  num_commands = 0;
  for (;;)
  {
    fgets(tmp_buf, MAX_STRING_LENGTH, fl);
    if (tmp_buf[0] == '$')
      break;
    if (tmp_buf[0] == '#')
    {
      /* skip over zone name */
      fgets(tmp_buf, MAX_STRING_LENGTH, fl);
    }
    else if (tmp_buf[0] == 'S')
    {
      command_array[t_idx] = num_commands + 1;
      t_idx++;
      num_commands = 0;
    }
    else if (IS_ZONE_COMMAND(tmp_buf[0]))
    {
      /* Commands A B C and Y are new random talbe zone comands */
      num_commands++;
    }
  }
  rewind(fl);

  /* Allocate array of zone structures */

  CREATE(zone_table, zone_data, num_zones, MEM_TAG_ZONEDAT);
  CREATE(sector_table, sector_data, 100, MEM_TAG_SECTDAT);
//  zone_table =
//    (struct zone_data *) calloc(num_zones, sizeof(struct zone_data));
//  sector_table =
//    (struct sector_data *) calloc(100, sizeof(struct sector_data));

  for (;;)
  {
    fscanf(fl, " #%d\n", &tmp);
    check = fread_string(fl);   /* zone name as specified by builder */
    if (*check == '$')
      break;                    /* * end of file */

    zone_table[zon].number = tmp; /* virtual zone number */
    zone_table[zon].name = check;
    zone_table[zon].avg_mob_level = -2;


    zone_table[zon].hometown = 0;       /* * default hometown is none */

    check = fread_string(fl);   /* zone filename */

    zone_table[zon].filename = check;

    fscanf(fl, "%d %d %d %d %d %d\n",
           &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6);
    /* * new with variable length lifespan */
    zone_table[zon].top = tmp1;
    zone_table[zon].reset_mode = tmp2;
    zone_table[zon].flags = tmp3;
    zone_table[zon].lifespan_min = tmp4;
    zone_table[zon].lifespan_max = tmp5;
    zone_table[zon].difficulty = tmp6;

    zone_table[zon].fullreset_lifespan_min = 20 * 60;
    zone_table[zon].fullreset_lifespan_max = 28 * 60;

    /* gotta preset this here */

    zone_table[zon].fullreset_lifespan =
      number(zone_table[zon].fullreset_lifespan_min,
             zone_table[zon].fullreset_lifespan_max);

    if (zone_table[zon].flags & ZONE_TOWN)
      for (i = 0; town_name_list[i][0] != '\n'; i++)
      {
        stripansi_2(zone_table[zon].name, temp_buf);
        if (isname(town_name_list[i], temp_buf))
        {
          zone_table[zon].hometown = i;
          break;
        }
      }
    /* Set beginning values for zone real range */
    zone_table[zon].real_bottom = zone_table[zon].real_top = -1;

    /* if zone is flagged as map, read map info (x,y size) */
    if (zone_table[zon].flags & ZONE_MAP)
    {
      fscanf(fl, "%d %d\n", &tmp1, &tmp2);
      zone_table[zon].mapx = tmp1;
      zone_table[zon].mapy = tmp2;
    }

    /* allocate the command table */
    if (command_array[zon] != 0)
    {
      CREATE(zone_table[zon].cmd, reset_com, (unsigned) command_array[zon], MEM_TAG_RESET);
/*
      zone_table[zon].cmd =
        (struct reset_com *) calloc(command_array[zon],
                                    sizeof(struct reset_com));
 */
    }
    else
    {
      fprintf(stderr, "zone %d has NO commands!\n", zon);
      raise(SIGSEGV);
    }

    /* read the command table */
    cmd_no = 0;

    for (;;)
    {
      fscanf(fl, " ");          /* skip blanks */
      fscanf(fl, "%c", &c);

      if (c == '*')
      {
        fgets(buf, MAX_STRING_LENGTH, fl);      /* skip command */
        /* OLC KLUDGE! */
        if (!strn_cmp(buf, "owner:", 6) && !zone_table[zon].owner)
        {
          char    *o, *p;

          o = buf + 6;
          while (*o == ' ')     /* skip whitespace  */
            o++;
          p = str_dup(o);
          o = p;
          while (*o)
            if ((*o == ' ') || (*o == '\n'))
              *o = '\0';
            else
              o++;
          zone_table[zon].owner = p;
        }
        continue;
      }

      if (!IS_ZONE_COMMAND(c))
      {
        fgets(buf, MAX_STRING_LENGTH, fl);      /* skip command */
        continue;
      }

      zone_table[zon].cmd[cmd_no].command = c;

      if (c == 'S')
        break;

      fscanf(fl, " %d %d %d %d %d %d %d",
             &tmp,
             &zone_table[zon].cmd[cmd_no].arg1,
             &zone_table[zon].cmd[cmd_no].arg2,
             &zone_table[zon].cmd[cmd_no].arg3,
             &zone_table[zon].cmd[cmd_no].arg4, &nu1, &nu2);

      zone_table[zon].cmd[cmd_no].if_flag = tmp;

      fgets(buf, sizeof(buf) - 1, fl);  /* read comment */

      cmd_no++;
      if (mini_mode == 2)
        fprintf(stderr, "cmd_no == %c%d", c, cmd_no);
    }

    zon++;
    if (mini_mode == 2)
      fprintf(stderr, "\r\nzon == %d\r\n", zon);
  }
  top_of_zone_table = --zon;
//  str_free(check);  // i don't think so..  the last time check is used, it reads a string that is put directly into the zone data
  FREE(command_array);
  fclose(fl);

  update_zone_difficulties();
}

#undef IS_ZONE_COMMAND

/*************************************************************************
*  procedures for resetting, both play-time and boot-time         *
*********************************************************************** */

/* get a mobile NUM from random table */
int get_mob_table(int tnum)
{
  int      temp;
  int      temp2;
  int      w, w2;

  for (temp = 0; temp < num_mob_tables; temp++)
  {
    if (mob_tables[temp].virtual_number == tnum)
      break;
  }
  if (mob_tables[temp].virtual_number != tnum)
    return 0;                   /* no table */
  w = number(0, mob_tables[temp].weight);       /* generate # between 0  and total wt */
  if (w < mob_tables[temp].empty_weight)
    return 0;                   /* empty chance */
  w2 = mob_tables[temp].empty_weight;
  for (temp2 = 0; temp2 < mob_tables[temp].entries; temp2++)
  {
    w2 += mob_tables[temp].table[temp2].weight;
    if (w < w2)                 /* got it */
      return mob_tables[temp].table[temp2].virtual_number;
  }
  return 0;                     /* shouldn't get here */
}

/* get a object NUM from random table */
int get_obj_table(int tnum)
{
  int      temp, temp2;
  int      w, w2;

  for (temp = 0; temp < num_obj_tables; temp++)
  {
    if (obj_tables[temp].virtual_number == tnum)
      break;
  }
  if (obj_tables[temp].virtual_number != tnum)
    return 0;                   /* no table */
  w = number(0, obj_tables[temp].weight);
  if (w < obj_tables[temp].empty_weight)
    return 0;
  w2 = obj_tables[temp].empty_weight;
  for (temp2 = 0; temp2 < obj_tables[temp].entries; temp2++)
  {
    w2 += obj_tables[temp].table[temp2].weight;
    if (w < w2)
      return obj_tables[temp].table[temp2].virtual_number;
  }
  return 0;
}

/* read a mobile from MOB_FILE */
P_char read_mobile(int nr, int type)
{
  P_char   mob = NULL;
  char     Gbuf1[MAX_STRING_LENGTH], buf[MAX_INPUT_LENGTH], letter = 0;
  int      foo, bar, i, j;
  long     tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9;
  unsigned utmp1, utmp2, utmp3, utmp4, utmp5, utmp6, utmp7, utmp8, utmp9;
  int      stmp, stmp3, stmp4, level;
  static int idnum = 0;

  i = nr;
  if (type == VIRTUAL)
    if ((nr = real_mobile(nr)) < 0)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "read_mobile: Mob %d not in database", i);
#endif
      return 0;
    }
  if (nr < 0)
  {
    logit(LOG_DEBUG, "read_mobile: negative rnum (%d). args %d, %s", nr,
          i, type ? "VIRTUAL" : "REAL");
    return 0;
  }
  fseek(mob_f, mob_index[nr].pos, 0);

  mob = (P_char) mm_get(dead_mob_pool);

  clear_char(mob);
  CREATE(mob->only.npc, npc_only_data, 1, MEM_TAG_NPCONLY);

  if (!mob->only.npc)
  {
    wizlog(56, "mob has no only.npc struct!");
    logit(LOG_DEBUG, "mob %s has no only.npc struct!", GET_NAME(mob));
    return NULL;
  }

  bzero(mob->only.npc, sizeof(npc_only_data));

  /* insert in list */
  mob->next = character_list;
  character_list = mob;
  mob->only.npc->R_num = nr;
  mob->desc = 0;
  mob_index[nr].number++;
  idnum++;
  mob->only.npc->idnum = idnum;
  mob->only.npc->default_pos = POS_STANDING + STAT_NORMAL;

  for( int i = 0; i < NUMB_CHAR_VALS; i++ )
  {
    mob->only.npc->value[i] = 0;
  }
  
/***** String data *** */

  /*
   * added pointers to the index struct, so that all mobs of the same
   * type will now share all text.  This should save us a huge amount of
   * RAM. -JAB
   */

  if (!mob_index[nr].keys)
  {
    mob->player.name = fread_string(mob_f);
    if (!mob->player.name)
    {
      wizlog(56, "Error with mob:  No name");
      return NULL;
    }
    for (j = 0; *(mob->player.name + j); j++)   /* make sure all keywords
                                                   are lowercased  */
      *(mob->player.name + j) = LOWER(*(mob->player.name + j));
    mob_index[nr].keys = mob->player.name;
  }
  else
  {
    skip_fread(mob_f);
    mob->player.name = mob_index[nr].keys;
  }

  if (!mob_index[nr].desc2)
  {
    mob->player.short_descr = fread_string(mob_f);
    mob_index[nr].desc2 = mob->player.short_descr;
  }
  else
  {
    skip_fread(mob_f);
    mob->player.short_descr = mob_index[nr].desc2;
  }

  if (!mob_index[nr].desc1)
  {
    mob->player.long_descr = fread_string(mob_f);
    mob_index[nr].desc1 = mob->player.long_descr;
  }
  else
  {
    skip_fread(mob_f);
    mob->player.long_descr = mob_index[nr].desc1;
  }

  if (!mob_index[nr].desc3)
  {
    mob->player.description = fread_string(mob_f);
    mob_index[nr].desc3 = mob->player.description;
  }
  else
  {
    skip_fread(mob_f);
    mob->player.description = mob_index[nr].desc3;
  }

  /**** Numeric data ****/

  /*
   * get next line of info.  It can be one of three formats: %d%d%d%dS,
   * %d%d%dS, or %d%d%d - DCL
   */

  fgets(buf, sizeof(buf) - 1, mob_f);
  if (sscanf(buf, " %u %u %u %u %u %u %u %u %u %c \n", &utmp1, &utmp7, &utmp8, &utmp9, &utmp2, &utmp3, &utmp4, &utmp5, &utmp6, &letter) == 10)
  {
    mob->specials.act = utmp1;
    mob->specials.affected_by = utmp2;
    mob->specials.affected_by2 = utmp3;
    mob->specials.affected_by3 = utmp4;
    mob->specials.affected_by4 = utmp5;
    mob->specials.affected_by5 = 0;
    mob->specials.alignment = utmp6;
    mob->only.npc->aggro_flags = utmp7;
    mob->only.npc->aggro2_flags = utmp8;
    mob->only.npc->aggro3_flags = utmp9;
  }
  else if (sscanf(buf, " %lu %lu %lu %lu %lu %lu %lu %lu %c \n", &tmp1, &tmp7, &tmp8, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, &letter) == 9)
  {
    mob->specials.act = tmp1;
    mob->only.npc->aggro_flags = tmp7;
    mob->only.npc->aggro2_flags = tmp8;
    mob->only.npc->aggro3_flags = 0;
    mob->specials.affected_by = tmp2;
    mob->specials.affected_by2 = tmp3;
    mob->specials.affected_by3 = tmp4;
    mob->specials.affected_by4 = tmp5;
    mob->specials.affected_by5 = 0;
    mob->specials.alignment = tmp6;
  }
  else
    if (sscanf
        (buf, " %lu %lu %lu %lu %c \n", &tmp1, &tmp2, &tmp3, &tmp4,
         &letter) == 5)
  {
    mob->specials.act = tmp1;
    mob->specials.affected_by = tmp2;
    mob->specials.affected_by2 = tmp3;
    mob->specials.alignment = tmp4;
  }
  else
  {
    if (sscanf(buf, " %lu %lu %lu %c \n", &tmp1, &tmp2, &tmp3, &letter) < 3)
    {
      logit(LOG_DEBUG, "Mob %d has messed up format.",
            mob_index[nr].virtual_number);
      extract_char(mob);
      return 0;
    }
    mob->specials.act = tmp1;
    mob->specials.affected_by = tmp2;
    mob->specials.affected_by2 = 0;
    mob->specials.alignment = tmp3;
  }

  if (IS_SET(mob->specials.act, ACT_CANFLY))    /* hack hack  */
    SET_BIT(mob->specials.affected_by, AFF_FLY);
  if (IS_AFFECTED2(mob, AFF2_CASTING))
    REMOVE_BIT(mob->specials.affected_by2, AFF2_CASTING);
  if (IS_AFFECTED(mob, AFF_FEAR))
    REMOVE_BIT(mob->specials.affected_by, AFF_FEAR);
  if (IS_AFFECTED(mob, AFF_CAMPING))
    REMOVE_BIT(mob->specials.affected_by, AFF_CAMPING);
  if (IS_AFFECTED2(mob, AFF2_MAJOR_PARALYSIS))
    REMOVE_BIT(mob->specials.affected_by2, AFF2_MAJOR_PARALYSIS);
  if (IS_AFFECTED2(mob, AFF2_SCRIBING))
    REMOVE_BIT(mob->specials.affected_by2, AFF2_SCRIBING);

  SET_BIT(mob->specials.act, ACT_ISNPC);

  if (letter == 'S')
  {
    fgets(buf, sizeof(buf) - 1, mob_f);
    if (sscanf(buf, " %s %i %u %i %i \n", Gbuf1, &stmp, &utmp2, &stmp3, &stmp4) == 5)
    {
      mob->player.race = 0;

      /* defaults to RACE_NONE */
      for (i = 0; (i <= LAST_RACE) && !mob->player.race; i++)
      if (!str_cmp(race_names_table[i].code, Gbuf1))
        mob->player.race = i;

      GET_HOME(mob) = stmp;
      mob->player.m_class = utmp2;
      mob->player.spec = stmp3;
      mob->player.size = stmp4;
    }
    else
    {
      sscanf(buf, " %s %i %u %i \n", Gbuf1, &stmp, &utmp2, &stmp3);

      mob->player.race = 0;

      /* defaults to RACE_NONE */
      for (i = 0; (i <= LAST_RACE) && !mob->player.race; i++)
	if (!str_cmp(race_names_table[i].code, Gbuf1))
	  mob->player.race = i;
      
      GET_HOME(mob) = stmp;
      mob->player.m_class = utmp2;
      mob->player.size = stmp3;
    }
    /* The new easy monsters */
    fscanf(mob_f, " %ld ", &tmp);
    if( tmp > MAXLVL || tmp < 1 )
    {
      logit(LOG_DEBUG, "Bad level %d for mob '%s' %d", tmp, J_NAME(mob), GET_VNUM(mob) );
      debug( "Bad level %d for mob '%s' %d", tmp, J_NAME(mob), GET_VNUM(mob) );
      mob->player.level = level = (tmp > MAXLVL) ? MAXLVL : 1;
    }
    else
    {
      mob->player.level = level = tmp;
    }
#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (!IS_SET(mob->specials.act, ACT_ELITE))
      mob->player.level = MAX(1, (int)(mob->player.level/2));

    if (IS_SET(mob->specials.act, ACT_ELITE))
      mob->player.level -= number(10, 20);
    
    if (IS_SET(mob->specials.act, ACT_TEACHER) ||
	IS_SET(mob->specials.act, ACT_SPEC_TEACHER))
      mob->player.level = 56;
#endif

/*
 * The following initialises the # of spells useable for NPCs in a given
 * spell circle based on the spl_table[level][spell_circle] in memorize.c.
 * Element 0 of this tracking array serves as an accumulator used in
 * replenishing used slots. - SKB 31 Mar 1995
 */

    mob->specials.undead_spell_slots[0] = 0;
    for (j = 1; j <= MAX_CIRCLE; j++)
      mob->specials.undead_spell_slots[j] = spl_table[GET_LEVEL(mob)][j - 1];

    fscanf(mob_f, " %ld ", &tmp);
    /* was warping things.  Tempy fix til everything changes.  JAB */
    if(IS_WARRIOR(mob) ||
      IS_GREATER_RACE(mob) ||
      IS_ELITE(mob) ||
      IS_GIANT(mob))
    {
      mob->points.base_hitroll = BOUNDED(2, (GET_LEVEL(mob) >> 1), 25);
    }
    else
    {
      mob->points.base_hitroll = BOUNDED(0, (GET_LEVEL(mob) / 3), 25);
    }

    mob->points.hitroll = mob->points.base_hitroll;

    fscanf(mob_f, " %ld ", &tmp);
    mob->points.base_armor = BOUNDED(-250, tmp, 250);

    tmp = 0;
    tmp2 = 0;
    tmp3 = 0;
    fscanf(mob_f, " %ldd%ld+%ld ", &tmp, &tmp2, &tmp3);

    // Added some extra hps for mobs for the Sept 12th 2014 wipe.
    //  lvl 1:0, lvl 2:2, lvl 3:4, lvl4:8 .. lvl 50:1250, lvl 62:1922.
    if (tmp2 <= 0 || tmp <= 0)
    {
      // mob->points.base_hit = tmp3;
      mob->points.base_hit = tmp3 + level * level / 2;
    }
    else
    {
      // mob->points.base_hit = dice(tmp, tmp2) + tmp3;
      mob->points.base_hit = dice(tmp, tmp2) + tmp3 + level * level / 2;
    }
    mob->points.hit = mob->points.max_hit = mob->points.base_hit;
    if (mob->points.hit <= 0)
      logit(LOG_MOB, "Warning: MOB #%d has negative (%d) hp.\n",
            mob_index[nr].virtual_number, mob->points.hit);

    fscanf(mob_f, " %ldd%ld+%ld \n", &tmp, &tmp2, &tmp3);
    mob->points.base_damroll = mob->points.damroll = tmp3 + level/2;
    mob->points.damnodice = tmp;
    mob->points.damsizedice = tmp2;

    fgets(buf, sizeof(buf) - 1, mob_f);
    if( sscanf(buf, " %ld.%ld.%ld.%ld %ld", &tmp1, &tmp2, &tmp3, &tmp4, &tmp) == 5 )
    {
      GET_PLATINUM(mob) = tmp4; /* * (number(50, 200) / 100); */
      GET_GOLD(mob) = tmp3;     /* * (number(50, 200) / 100); */
      GET_SILVER(mob) = tmp2;   /* * (number(50, 200) / 100); */
      GET_COPPER(mob) = tmp1;   /* * (number(50, 200) / 100); */
      GET_EXP(mob) = tmp;
      if (GET_PLATINUM(mob) > 20)
      {
        tmp = ((GET_PLATINUM(mob) * 1000) + (GET_GOLD(mob) * 100) +
               (GET_SILVER(mob) * 10) + GET_COPPER(mob));
        ADD_MONEY(mob, tmp);
      }
    }
    else
    {
      tmp1 = 0;
      tmp = 0;
      if (sscanf(buf, " %ld %ld", &tmp1, &tmp) == 2)
      {
        ADD_MONEY(mob, tmp1);
        GET_EXP(mob) = tmp;
      }
      else
      {
        logit(LOG_DEBUG, "Bogus cash and/or exp for mob %d",
              mob_index[nr].virtual_number);
        raise(SIGSEGV);
      }
    }
  }
  else
  {
    fscanf(mob_f, " %ld ", &tmp);
    mob->base_stats.Str = (sh_int) (tmp * 4.5);

    fscanf(mob_f, " %ld ", &tmp);

    fscanf(mob_f, " %ld ", &tmp);
    mob->base_stats.Int = (sh_int) (tmp * 4.5);

    fscanf(mob_f, " %ld ", &tmp);
    mob->base_stats.Wis = (sh_int) (tmp * 4.5);

    fscanf(mob_f, " %ld ", &tmp);
    mob->base_stats.Dex = (sh_int) (tmp * 4.5);

    fscanf(mob_f, " %ld \n", &tmp);
    mob->base_stats.Con = (sh_int) (tmp * 4.5);

    mob->base_stats.Pow = mob->base_stats.Int;
    mob->base_stats.Agi = mob->base_stats.Dex;
    mob->base_stats.Cha = number(1, 100);
    mob->base_stats.Karma = number(1, 100);
    mob->base_stats.Luck = number(1, 100);

    fscanf(mob_f, " %ld ", &tmp);
    fscanf(mob_f, " %ld ", &tmp2);

    mob->points.base_hit = number(tmp, tmp2);
    mob->points.hit = mob->points.max_hit = mob->points.base_hit;
    if (mob->points.hit < 0)
      logit(LOG_DEBUG, "MOB #%d has negative (%d) hp.",
            mob_index[nr].virtual_number, mob->points.hit);

    fscanf(mob_f, " %ld ", &tmp);

    mob->points.base_armor = 250;

    fscanf(mob_f, " %ld ", &tmp);
    mob->points.mana = mob->points.base_mana = mob->points.max_mana = tmp;

    fscanf(mob_f, " %ld ", &tmp);
    mob->points.vitality = mob->points.base_vitality =
      mob->points.max_vitality = tmp;

    fscanf(mob_f, " %ld ", &tmp);
    GET_EXP(mob) = tmp;

    /* Get hometown */
    fscanf(mob_f, " %ld ", &tmp);
    GET_HOME(mob) = tmp;
    GET_BIRTHPLACE(mob) = tmp;

    /* Get alignment */
    fscanf(mob_f, " %ld \n", &tmp);
    GET_ALIGNMENT(mob) = tmp;
  }

  fscanf(mob_f, " %ld ", &tmp);
  /*
   * ok, this has to be changed, until all mob files are changed.  We
   * have to interpret old 'position' number into new one. JAB
   */
  switch (tmp)
  {
  case 0:                      /* * was POSITION_DEAD */
    logit(LOG_DEBUG, "Mob %d tried to load dead",
          mob_index[nr].virtual_number);
    SET_POS(mob, POS_PRONE + STAT_DYING);
    break;
  case 1:                      /* * was POSITION_MORTALLYW */
    SET_POS(mob, POS_PRONE + STAT_DYING);
    break;
  case 2:                      /* * was POSITION_INCAP */
    SET_POS(mob, POS_PRONE + STAT_INCAP);
    break;
  case 3:                      /* * was POSITION_STUNNED */
    SET_POS(mob, POS_SITTING + STAT_RESTING);
    break;
  case 4:                      /* * was POSITION_SLEEPING */
    SET_POS(mob, POS_PRONE + STAT_SLEEPING);
    break;
  case 5:                      /* * was POSITION_RESTING */
    SET_POS(mob, POS_SITTING + STAT_RESTING);
    break;
  case 6:                      /* * was POSITION_SITTING */
    SET_POS(mob, POS_SITTING + STAT_NORMAL);
    break;
  case 7:                      /* * was POSITION_FIGHTING */
    logit(LOG_DEBUG, "Mob %d loaded fighting.", mob_index[nr].virtual_number);
  case 8:                      /* * was POSITION_STANDING */
    SET_POS(mob, POS_STANDING + STAT_NORMAL);
    break;
  case 9:                      /* * was POSITION_SWIMMING? */
    SET_POS(mob, POS_PRONE + STAT_NORMAL);
    break;
  case 10:                     /* * was POSITION_FLYING */
    SET_POS(mob, POS_STANDING + STAT_NORMAL);
    SET_BIT(mob->specials.affected_by, AFF_FLY);
    break;
  case 11:                     /* * was POSITION_LEVITATING */
    SET_POS(mob, POS_STANDING + STAT_NORMAL);
    SET_BIT(mob->specials.affected_by, AFF_LEVITATE);
    break;
  }

  mob->only.npc->default_pos = mob->specials.position;

  fscanf(mob_f, " %ld ", &tmp);
  /*
   * ok, this has to be changed, until all mob files are changed.  We
   * have to interpret old 'position' number into new one. JAB
   */
  switch (tmp)
  {
  case 0:                      /* * was POSITION_DEAD */
    logit(LOG_DEBUG, "Mob %d tried to load dead",
          mob_index[nr].virtual_number);
    SET_POS(mob, POS_PRONE + STAT_DYING);
    break;
  case 1:                      /* * was POSITION_MORTALLYW */
    SET_POS(mob, POS_PRONE + STAT_DYING);
    break;
  case 2:                      /* * was POSITION_INCAP */
    SET_POS(mob, POS_PRONE + STAT_INCAP);
    break;
  case 3:                      /* * was POSITION_STUNNED */
    SET_POS(mob, POS_SITTING + STAT_RESTING);
    break;
  case 4:                      /* * was POSITION_SLEEPING */
    SET_POS(mob, POS_PRONE + STAT_SLEEPING);
    break;
  case 5:                      /* * was POSITION_RESTING */
    SET_POS(mob, POS_SITTING + STAT_RESTING);
    break;
  case 6:                      /* * was POSITION_SITTING */
    SET_POS(mob, POS_SITTING + STAT_NORMAL);
    break;
  case 7:                      /* * was POSITION_FIGHTING */
    logit(LOG_DEBUG, "Mob %d loaded fighting.", mob_index[nr].virtual_number);
  case 8:                      /* * was POSITION_STANDING */
    SET_POS(mob, POS_STANDING + STAT_NORMAL);
    break;
  case 9:                      /* * was POSITION_SWIMMING? */
    SET_POS(mob, POS_PRONE + STAT_NORMAL);
    break;
  case 10:                     /* * was POSITION_FLYING */
    SET_POS(mob, POS_STANDING + STAT_NORMAL);
    SET_BIT(mob->specials.affected_by, AFF_FLY);
    break;
  case 11:                     /* * was POSITION_LEVITATING */
    SET_POS(mob, POS_STANDING + STAT_NORMAL);
    SET_BIT(mob->specials.affected_by, AFF_LEVITATE);
    break;
  }

  tmp = mob->only.npc->default_pos;
  mob->only.npc->default_pos = mob->specials.position;
  mob->specials.position = tmp;

  fscanf(mob_f, " %ld \n", &tmp);
  mob->player.sex = tmp;

  if (letter == 'S')
  {
    mob->player.time.birth = time(0);
    mob->player.time.played = 0;
    mob->player.time.logon = time(0);

    for (i = 0; i < 3; i++)
      GET_COND(mob, i) = -1;

    for (i = 0; i < 5; i++)
      mob->specials.apply_saving_throw[i] = 0;

    if((GET_LEVEL(mob) > 50) ||
      IS_GREATER_RACE(mob) ||
      IS_ELITE(mob))
    {
      roll_basic_abilities(mob, 2);
    }
    else if (GET_LEVEL(mob) > 5)
      roll_basic_abilities(mob, 0);
    else
      roll_basic_abilities(mob, -1);

    if(IS_GREATER_RACE(mob) ||
      IS_ELITE(mob) ||
      strstr(mob->player.name, "guard") ||
      strstr(mob->player.name, "elite") ||
      strstr(mob->player.name, "militia") ||
      strstr(mob->player.name, "fighter") ||
      strstr(mob->player.name, "warrior"))
    {
      while (mob->base_stats.Str < min_stats_for_class[1][0])
        mob->base_stats.Str += number(10, 20);
      while (mob->base_stats.Dex < min_stats_for_class[1][1])
        mob->base_stats.Dex += number(10, 20);
      while (mob->base_stats.Agi < min_stats_for_class[1][2])
        mob->base_stats.Agi += number(10, 20);
      while (mob->base_stats.Con < min_stats_for_class[1][3])
        mob->base_stats.Con += number(10, 20);
    }
    if (strstr(mob->player.name, "thief") ||
        strstr(mob->player.name, "rogue") ||
        strstr(mob->player.name, "bandit") ||
        strstr(mob->player.name, "assassin"))
    {
      while (mob->base_stats.Dex < min_stats_for_class[13][1])
        mob->base_stats.Dex += number(10, 20);
      while (mob->base_stats.Agi < min_stats_for_class[13][2])
        mob->base_stats.Agi += number(10, 20);
      while (mob->base_stats.Int < min_stats_for_class[13][5])
        mob->base_stats.Int += number(10, 20);
      while (mob->base_stats.Cha < min_stats_for_class[13][7])
        mob->base_stats.Cha += number(10, 20);
      if (GET_CLASS(mob, CLASS_ROGUE) && (!mob_index[GET_RNUM(mob)].func.mob))
        mob_index[GET_RNUM(mob)].func.mob = thief;
    }
    if (strstr(mob->player.name, "bailiff"))
      if (!mob_index[GET_RNUM(mob)].func.mob)
        mob_index[GET_RNUM(mob)].func.mob = citizenship;

    mob->base_stats.Str = BOUNDED(25, mob->base_stats.Str, 200);
    mob->base_stats.Dex = BOUNDED(25, mob->base_stats.Dex, 200);
    mob->base_stats.Agi = BOUNDED(25, mob->base_stats.Dex, 200);
    mob->base_stats.Con = BOUNDED(25, mob->base_stats.Con, 200);
    mob->base_stats.Pow = BOUNDED(25, mob->base_stats.Con, 200);
    mob->base_stats.Int = BOUNDED(25, mob->base_stats.Int, 200);
    mob->base_stats.Wis = BOUNDED(25, mob->base_stats.Wis, 200);
    mob->base_stats.Cha = BOUNDED(25, mob->base_stats.Wis, 200);

    /* * variable mana */
    i = 80 + dice(MAX(1, GET_LEVEL(mob)), IS_ANIMAL(mob) ? 1 : 4) +
      GET_LEVEL(mob) * 2;

    /* a few special cases to up things a bit */
    if(IS_ELITE(mob) ||
      IS_GREATER_RACE(mob))
    { 
      i += GET_LEVEL(mob) * 5;
    }

    /* at this point, i ranges from 83 to 696, there are a few other cases */
    if (GET_LEVEL(mob) >= 56)
      i += 1000;
    else if (GET_LEVEL(mob) > 53)
      i += 500;
    else if (GET_LEVEL(mob) > 50)
      i += 150;

    mob->points.mana = mob->points.base_mana = mob->points.max_mana = i;

    mob->points.vitality = mob->points.base_vitality =
      mob->points.max_vitality =
      MAX(50,
          mob->base_stats.Agi) + (mob->base_stats.Str +
                                  mob->base_stats.Con) /
      ((IS_ANIMAL(mob)) ? 1 : 2);

  }
  else
  {                             /* The old monsters are down below here */
    fscanf(mob_f, " %s ", Gbuf1);
    mob->player.race = 0;

    /* defaults to RACE_NONE */
    for (i = 0; (i <= LAST_RACE) && !mob->player.race; i++)
      if (!str_cmp(race_names_table[i].code, Gbuf1))
        mob->player.race = i;

    logit(LOG_MOB, "Old style mob: %d Race: %s(%d)",
          mob_index[nr].virtual_number, Gbuf1, mob->player.race);

    fscanf(mob_f, " %ld ", &tmp);
//    GET_LEVEL(mob) = tmp;
    mob->player.level = tmp;

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (!IS_SET(mob->specials.act, ACT_ELITE))
      mob->player.level = (int)(mob->player.level/2);

    if (IS_SET(mob->specials.act, ACT_ELITE))
      mob->player.level -= number(5, 15);
    
    if (IS_SET(mob->specials.act, ACT_TEACHER) ||
	IS_SET(mob->specials.act, ACT_SPEC_TEACHER))
      mob->player.level = 56;
#endif

    fscanf(mob_f, " %ld ", &tmp);
    mob->player.time.birth = time(0);
    mob->player.time.played = 0;
    mob->player.time.logon = time(0);

    fscanf(mob_f, " %ld ", &tmp);       /* weight */

    fscanf(mob_f, " %ld \n", &tmp);     /* height */

    for (i = 0; i < 3; i++)
    {
      fscanf(mob_f, " %ld ", &tmp);
      GET_COND(mob, i) = tmp;
    }
    fscanf(mob_f, " \n ");

    for (i = 0; i < 5; i++)
    {
      fscanf(mob_f, " %ld ", &tmp);
      mob->specials.apply_saving_throw[i] = tmp;
    }

    fscanf(mob_f, " \n ");

    /* Set the damage as some standard 1d6 */
    fscanf(mob_f, " %ldd%ld+%ld %ld\n", &tmp, &tmp2, &tmp3, &tmp4);
    mob->points.base_damroll = mob->points.damroll = tmp3 + level/2;
    mob->points.damnodice = tmp;
    mob->points.damsizedice = tmp2;
    /* was warping things.  Tempy fix til everything changes.  JAB */
    if(IS_WARRIOR(mob) ||
      IS_GREATER_RACE(mob) ||
      IS_GIANT(mob) ||
      IS_ELITE(mob))
    {
      mob->points.base_hitroll = BOUNDED(2, (GET_LEVEL(mob) >> 1), 25);
    }
    else
    {
      mob->points.base_hitroll = BOUNDED(0, (GET_LEVEL(mob) / 3), 25);
    }
    mob->points.hitroll = mob->points.base_hitroll;

    /* read in amount of money the mob is carrying */
    fgets(buf, sizeof(buf) - 1, mob_f);
    if (sscanf(buf, " %ld.%ld.%ld.%ld %ld", &tmp1, &tmp2, &tmp3, &tmp4, &tmp)
        == 5)
    {
      GET_COPPER(mob) = tmp1;
      GET_SILVER(mob) = tmp2;
      GET_GOLD(mob) = tmp3;
      GET_PLATINUM(mob) = tmp4;
      GET_EXP(mob) = tmp;
    }
    else
    {
      tmp1 = 0;
      tmp = 0;
      if (sscanf(buf, " %ld %ld", &tmp1, &tmp) == 2)
      {
        ADD_MONEY(mob, tmp1);
        GET_EXP(mob) = tmp;
      }
      else
      {
        logit(LOG_DEBUG, "Bogus cash and/or exp for mob %d",
              mob_index[nr].virtual_number);
        raise(SIGSEGV);
      }
    }
  }

  foo = GET_DAMROLL(mob);
  foo += mob->points.damnodice * ((1 + mob->points.damsizedice) >> 1);
  if(IS_GREATER_RACE(mob) ||
    IS_ELITE(mob))
  {
    bar = MIN(foo, 100);
  }
  else
  {
    bar = MIN(foo, 60);
  }

  if (foo > bar)
  {
    foo = bar;
    logit(LOG_MOB, "FYI - no changes made to MOB: %d has _RIDICULOUS_ damage. %dd%d + %d (%d to %d) check mob code, stats and racial stats.",
          mob_index[nr].virtual_number, mob->points.damnodice,
          mob->points.damsizedice, GET_DAMROLL(mob),
          GET_DAMROLL(mob) + mob->points.damnodice,
          GET_DAMROLL(mob) +
          (mob->points.damnodice * mob->points.damsizedice));
  }

  mob->curr_stats = mob->base_stats;

  /* Set up an attack type */
  mob->only.npc->attack_type = GetFormType(mob);

  clearMemory(mob);

  if (IS_SET(mob->specials.act, ACT_SPEC) && (mob_index[nr].func.mob == 0))
  {
    REMOVE_BIT(mob->specials.act, ACT_SPEC);
    if (mob_index[nr].number == 1)      /*
                                         * only first, not every
                                         */
      logit(LOG_MOB, "ACT_SPEC, but no function: %d %s",
            mob_index[nr].virtual_number, GET_NAME(mob));
  }
  /* if they have a func but no spec bit, add one -- DTS 2/12/95 */
  if (mob_index[nr].func.mob && !IS_SET(mob->specials.act, ACT_SPEC))
    SET_BIT(mob->specials.act, ACT_SPEC);
  if (IS_SHOPKEEPER(mob))
  {
    SET_BIT(mob->specials.act, ACT_BREAK_CHARM);
    SET_BIT(mob->specials.act, ACT_SPEC_DIE);
  }

  if (IS_ACT(mob, ACT_TEACHER) && !mob_index[nr].func.mob)
  {
    mob_index[nr].func.mob = teacher;
    SET_BIT(mob->specials.act, ACT_SPEC);
  }

  setCharPhysTypeInfo(mob);

  /* init a periodic event for each mob */
  add_event(event_mob_mundane, PULSE_MOBILE + number(-4, 4), mob, 0, 0, 0, 0, 0);
  if (IS_ACT(mob, ACT_PATROL))
    add_event(event_patrol_move, WAIT_SEC, mob, 0,0,0,0,0);

  convertMob(mob);

  if (IS_AFFECTED(mob, AFF_STONE_SKIN | AFF_BIOFEEDBACK))
    add_event(event_mob_skin_spell, number(1,5), mob, 0, 0, 0, 0, 0);
  
  return (mob);
}

void event_object_proc(P_char ch, P_char victim, P_obj obj, void *data)
{
  if (obj_index[obj->R_num].func.obj)
    (*obj_index[obj->R_num].func.obj) (obj, 0, CMD_PERIODIC, 0);

  add_event(event_object_proc, PULSE_MOBILE + number(-4, 4), 0, 0, obj, 0, 0, 0);
}

/* read an object from OBJ_FILE */
P_obj read_object(int nr, int type)
{
  P_obj    obj;
  int      tmp, i, j, k;
  unsigned long int utmp;
  char     chk[MAX_STRING_LENGTH];
  struct extra_descr_data *new_descr;

  i = nr;
  if (type == VIRTUAL)
    if ((nr = real_object(nr)) < 0)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "read_object: Obj %d not in database", i);
#endif
      return (0);
    }
  if (nr < 0)
  {
    logit(LOG_DEBUG, "read_object: negative rnum (%d) args %d, %s", nr, i,
          type ? "VIRTUAL" : "REAL");
    return 0;
  }
  fseek(obj_f, obj_index[nr].pos, 0);

#if 0
#   ifdef MEM_DEBUG
  mem_use[MEM_OBJ] += sizeof(struct obj_data);
#   endif
  CREATE(obj, struct obj_data, 1);
#endif
  obj = (P_obj) mm_get(dead_obj_pool);

  memset(obj, 0, sizeof(struct obj_data));

  obj->R_num = -1;
  obj->loc_p = LOC_NOWHERE;
  obj->loc.room = NOWHERE;

  obj->R_num = nr;
  obj_index[nr].number++;

  if (object_list)
    object_list->prev = obj;
  obj->next = object_list;
  obj->prev = NULL;
  object_list = obj;

  /* *** string data *** */

  /*
   * added pointers to the index struct, so that all objs of the same
   * type will now share all text.  This should save us a huge amount of
   * RAM. -JAB
   */

  if (!obj_index[nr].keys)
  {
    obj->name = fread_string(obj_f);
    for (j = 0; *(obj->name + j); j++)  /* make sure all keywords are lowercased */
      *(obj->name + j) = LOWER(*(obj->name + j));
    obj_index[nr].keys = obj->name;
  }
  else
  {
    skip_fread(obj_f);
    obj->name = obj_index[nr].keys;
  }

  if (!obj_index[nr].desc2)
  {
    obj->short_description = fread_string(obj_f);
    obj_index[nr].desc2 = obj->short_description;
  }
  else
  {
    skip_fread(obj_f);
    obj->short_description = obj_index[nr].desc2;
  }

  if (!obj_index[nr].desc1)
  {
    obj->description = fread_string(obj_f);
    obj_index[nr].desc1 = obj->description;
  }
  else
  {
    skip_fread(obj_f);
    obj->description = obj_index[nr].desc1;
  }

  if (!obj_index[nr].desc3)
  {
    obj->action_description = fread_string(obj_f);
    obj_index[nr].desc3 = obj->action_description;
  }
  else
  {
    skip_fread(obj_f);
    obj->action_description = obj_index[nr].desc3;
  }
  obj->str_mask = 0;

  /* *** numeric data *** */

  fscanf(obj_f, " %d ", &tmp);
  obj->type = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->material = tmp;
  fscanf(obj_f, " %d ", &tmp);
//  obj->size = tmp;
  fscanf(obj_f, " %d ", &tmp);
//  obj->space = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->craftsmanship = tmp;
  fscanf(obj_f, " %d ", &tmp);
//  obj->damres_bonus = tmp;
  fscanf(obj_f, " %lu ", &utmp);
  obj->extra_flags = utmp;
  fscanf(obj_f, " %lu ", &utmp);
  obj->wear_flags = utmp;
  fscanf(obj_f, " %lu ", &utmp);
  obj->extra2_flags = utmp;
  fscanf(obj_f, " %lu ", &utmp);
  obj->anti_flags = utmp;
  fscanf(obj_f, " %lu ", &utmp);
  // Hack until we make as script to edit files directly.
  if (IS_SET(obj->anti_flags, CLASS_NECROMANCER))
    SET_BIT(obj->anti_flags, CLASS_THEURGIST);
  obj->anti2_flags = utmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->value[0] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->value[1] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->value[2] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->value[3] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->value[4] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->value[5] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->value[6] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->value[7] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->weight = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->cost = tmp;
  fscanf(obj_f, " %d \n", &tmp);
  obj->condition = tmp;
//  fscanf(obj_f, " %d \n", &tmp);
//  obj->max_condition = tmp;  wipe2011
//  if(obj->max_condition < 100)
//    obj->max_condition = 100;

  if (fscanf(obj_f, " %lu \n", &utmp) == 1)
  {
    obj->bitvector = utmp;
    if (fscanf(obj_f, " %lu \n", &utmp) == 1)
    {
      obj->bitvector2 = utmp;
      if (fscanf(obj_f, " %lu \n", &utmp) == 1)
      {
        obj->bitvector3 = utmp;
        if (fscanf(obj_f, " %lu \n", &utmp) == 1)
          obj->bitvector4 = utmp;
      }
    }
  }

//  if(obj->craftsmanship > ((OBJCRAFT_HIGHEST - 1) / 2))
//  {
//    obj->max_condition = (int) BOUNDED(100, (50 * 1.4285 * (obj->craftsmanship - 6)), 500);
    // 1.4285 = 10 / 7(max condition is 1000, there are 7 values after average craftsmanship)
//  } 
//  obj->condition = obj->max_condition;  wipe2011

  // nuke the proclib flag - it'll be put back if needed
  REMOVE_BIT(obj->extra_flags, ITEM_PROCLIB);
  /* *** extra descriptions *** */

  while (fscanf(obj_f, " %s \n", chk), *chk == 'E')
  {
    CREATE(new_descr, extra_descr_data, 1, MEM_TAG_EXDESCD);

    new_descr->keyword = fread_string(obj_f);
    new_descr->description = fread_string(obj_f);

    // hack for proc libs..
    if (!strn_cmp("_proclib_", new_descr->keyword, 9))
    {
      // send it to the proc lib stuff..
      if (!proclibObj_add(obj, new_descr->keyword + 9, new_descr->description))
      {
        // free what we just read - its useless now GLD
        FREE(new_descr->keyword);
        FREE(new_descr->description);
        FREE(new_descr);
        new_descr = NULL;
      }
    }
    if (new_descr)
    {
      new_descr->next = obj->ex_description;
      obj->ex_description = new_descr;
    }
  }

  for (i = 0; (i < MAX_OBJ_AFFECT) && (*chk == 'A'); i++)
  {
    fscanf(obj_f, " %d ", &tmp);
    obj->affected[i].location = tmp;
    fscanf(obj_f, " %d \n", &tmp);
    obj->affected[i].modifier = tmp;
    fscanf(obj_f, " %s \n", chk);
  }

  obj->z_cord = 0;

  /* Trapped item data */
  obj->trap_eff = obj->trap_dam = obj->trap_charge = 0;
  if (*chk == 'T')
  {
    fscanf(obj_f, " %d ", &tmp);
    obj->trap_eff = tmp;
    fscanf(obj_f, " %d ", &tmp);
    obj->trap_dam = tmp;
    fscanf(obj_f, " %d ", &tmp);
    obj->trap_charge = tmp;
    fscanf(obj_f, " %d \n", &tmp);
    obj->trap_level = tmp;
  }
  /* ensure builders dont mess things up */
  if (IS_SET(obj->wear_flags, ITEM_TAKE) &&
      !IS_SET(obj->wear_flags, ITEM_HOLD))
    SET_BIT(obj->wear_flags, ITEM_HOLD);
  if (IS_SET(obj->wear_flags, ITEM_HOLD) &&
      !IS_SET(obj->wear_flags, ITEM_TAKE))
    SET_BIT(obj->wear_flags, ITEM_TAKE);
  if (obj->type == ITEM_SWITCH && !obj_index[obj->R_num].func.obj)
    obj_index[obj->R_num].func.obj = item_switch;
  if (obj->type == ITEM_ARMOR && !obj->value[0])
    obj->type = ITEM_WORN;
#if 0
  if (obj->type == ITEM_ARMOR && !IS_SET(obj->wear_flags, ITEM_WEAR_BODY)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_LEGS)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_ARMS)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_HEAD)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_ABOUT)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_FEET)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_SHIELD)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_HANDS))
  {
    obj->type = ITEM_WORN;
    obj->affected[2].location = APPLY_ARMOR;
    obj->affected[2].modifier = -(obj->value[0]);
  }
#endif
  /* set up a few items that are belt attachable */
  if (((GET_ITEM_TYPE(obj) ==
        ITEM_DRINKCON) /*&& !isname("barrel", obj->name) */  &&
       (isname("canteen", obj->name) || isname("skin", obj->name) ||
        isname("horn", obj->name))) || ((GET_ITEM_TYPE(obj) == ITEM_CONTAINER)
                                        && (isname("bag", obj->name) ||
                                            isname("sack", obj->name) ||
                                            isname("tube", obj->name) ||
                                            isname("case", obj->name) ||
                                            isname("scabbard", obj->name) ||
                                            isname("pouch", obj->name)) &&
                                        (obj->value[0] < 25)) ||
      (GET_ITEM_TYPE(obj) == ITEM_QUIVER))
    SET_BIT(obj->wear_flags, ITEM_ATTACH_BELT);

  /* and some that are back */
  if ((GET_ITEM_TYPE(obj) == ITEM_CONTAINER && isname("backpack", obj->name))
      || GET_ITEM_TYPE(obj) == ITEM_QUIVER)
    SET_BIT(obj->wear_flags, ITEM_WEAR_BACK);

  /* set throw flag to obj */
  if (obj->type == ITEM_WEAPON)
  {
    if (strstr(obj->name, "axe") ||
        strstr(obj->name, "hammer") ||
        strstr(obj->name, "trident") ||
        strstr(obj->name, "club") || strstr(obj->name, "dart"))
      SET_BIT(obj->extra_flags, ITEM_CAN_THROW1);
    else if (strstr(obj->name, "dagger") ||
             strstr(obj->name, "spear") || strstr(obj->name, "javelin"))
      SET_BIT(obj->extra_flags, ITEM_CAN_THROW2);
    else if (strstr(obj->name, "boomerang"))
    {
      SET_BIT(obj->extra_flags, ITEM_CAN_THROW1);
      SET_BIT(obj->extra_flags, ITEM_CAN_THROW2);
      SET_BIT(obj->extra_flags, ITEM_RETURNING);
    }
    if( obj->value[0] == WEAPON_2HANDSWORD )
    {
      SET_BIT(obj->extra_flags, ITEM_TWOHANDS);
    }
  }

  if (obj_index[nr].func.obj)
  {
    if ((*obj_index[nr].func.obj) (obj, 0, CMD_SET_PERIODIC, 0))
      add_event(event_object_proc, PULSE_MOBILE + number(-4,4), 0, 0, obj, 0, 0, 0);    
  }

  if (isname("random_exit", obj->name))
    add_event(event_random_exit, 3, 0, 0, obj, 0, 0, 0);

  /* arti check every minute */

  if (IS_ARTIFACT(obj))
  {
    add_event(event_artifact_poof, 2 * WAIT_SEC, 0, 0, obj, 0, 0, 0);
    // Set current timer?  Not necessary as event_artifact_poof will fix it?
    //   Not really sure atm.. will test this and see what happens...
    obj->timer[3] = time(NULL);
  }

  /* init justice flag */
//  obj->justice_status = 0;
//  obj->justice_name = NULL;

  convertObj(obj);
  return (obj);
}


/*  Function to reset no_reset zones...
 *  This function will be called at a time during boot
 *     A) After the original stone has been touched
 *     B) Random event timer expires(event is applied after touch)
 *     C) Incremental chance of reset occurring above random roll
 *
 *  Why have this?  It moves us closer to a game-world that is persistent
 *  and auto-refreshes.  Incremental time will be stored in DB and will
 *  be modified by frequency of overall zone resets occurring through
 * this method.
 */
void no_reset_zone_reset(int zone_number)
{
  if(!qry("SELECT reset_perc FROM zones WHERE id = '%d'", zone_number))
  {
    logit(LOG_DEBUG, "no_reset_zone_reset: could not find zone information for zone id %d", zone_number);
    return;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if(mysql_num_rows(res) < 1)
  {
    logit(LOG_DEBUG, "No data retrieved from DB for no_reset_zone_reset...");
    mysql_free_result(res);
    return;
  }

  MYSQL_ROW row = mysql_fetch_row(res);

  if(atoi(row[0]) > number(0, 99))
  {
    zone_purge(zone_number);
    reset_zone(zone_number, 1);
    db_query("UPDATE zones SET reset_perc = '%d' WHERE id = '%d'", 0, zone_number);
    epic_zone_erase_touch(zone_table[zone_number].number);
  }
  else
  {
    add_event(event_reset_zone, WAIT_MIN * 60, 0, 0, 0, 0, &zone_number, sizeof(zone_number));
    db_query("UPDATE zones SET reset_perc = '%d' WHERE id = '%d'", atoi(row[0]) + 1, zone_number);
  }
}

#define ZCMD zone_table[zone].cmd[cmd_no]

/* execute the reset command table of a given zone */
/* force_item_repop : 2 means this is a boot-time initial reset of zone. */
void reset_zone(int zone, int force_item_repop)
{
  int      cmd_no, last_cmd = 1, last_mob_load = 0;
  int      temp, room;
  time_t   timer;
  P_char   mob = NULL, last_mob = NULL, tmp_mob = NULL,
    last_mob_followable = NULL;
  P_obj    obj, obj_to;
  P_event  e1 = NULL;
  char     buf[MAX_STRING_LENGTH];

  for (cmd_no = 0;; cmd_no++)
  {
    room = 0;
    if (ZCMD.command == 'S')
      break;

    /* last_mob_load added in case an equipment load fails due to a random
       roll..  we want the rest of the stuff on the mob to happen (followers,
       other equip, items, riders), and if the original mob loaded, let's
       let it all happen */

    if (last_cmd || !ZCMD.if_flag ||
        (last_mob_load && ((ZCMD.command == 'G') || (ZCMD.command == 'E') ||
                           (ZCMD.command == 'R'))) ||
        (last_mob_followable && (ZCMD.command == 'F')))
      switch (ZCMD.command)
      {
      case 'Y':
        last_cmd = 0;
        temp = get_mob_table(ZCMD.arg1);
        if (!temp)
        {
          last_cmd = 0;
          break;
        }
        if (real_mobile(temp) == -1)
        {
          last_cmd = 0;
          break;
        }
        // set the mob limit from zone file
        mob_index[real_mobile(temp)].limit = ZCMD.arg2;

        if (mob_index[real_mobile(temp)].number < ZCMD.arg2)
        {
          mob = read_mobile(temp, VIRTUAL);
          if (!mob)
          {
            last_cmd = 0;
            break;
          }
          tmp_mob = NULL;
          last_mob = mob;
          GET_BIRTHPLACE(mob) = world[ZCMD.arg3].number;
          apply_zone_modifier(mob);
          char_to_room(mob, ZCMD.arg3, -2);
          last_cmd = 1;
        }
        else
          last_cmd = 0;

        break;

      case 'B':
        last_cmd = 0;
        temp = get_obj_table(ZCMD.arg1);
        if (!temp)
          break;
        temp = real_object(temp);
        if (temp == -1)
          break;

        obj_index[temp].limit = ZCMD.arg2; // set the mob limit from zone file

        if ((ZCMD.arg3 >= 0) && (obj_index[temp].number < ZCMD.arg2))
        {
          if( (timer = get_current_artifact_info(temp, 0, NULL, NULL, NULL, NULL, FALSE, NULL)) )
          {
            if( timer == -1 )
            {
              get_current_artifact_info(temp, 0, NULL, &room, NULL, NULL, FALSE, &timer);
            }
            else
            {
              //statuslog(56, "didn't load arti obj #%d because already tracked", obj_index[temp].virtual_number);
              break;
            }
          }

          if( !(obj = read_object(temp, REAL)) )
          {
             break;
          }
          // Remove the artifact unless artifact.respawn == 0
          //   or artifact.respawn == 1 and we are not booting.
          if( IS_ARTIFACT(obj) && ( get_property("artifact.respawn", 0) == 0
            || (get_property("artifact.respawn", 0) == 1 && force_item_repop != 2) ))
          {
            extract_obj(obj, TRUE);
            break;
          }
          if( IS_ARTIFACT(obj) && timer )
          {
            obj->timer[3] = timer;
            obj_to_room( obj, real_room(room) );
            break;
          }
          obj_to = get_obj_num(ZCMD.arg3);
          if( !obj_to )
          {
            extract_obj(obj, TRUE);
            break;
          }
          obj_to_obj(obj, obj_to);
          // Artifact poof timer to BLOOD_DAYS * secs in a day.
          obj->timer[3] = time(NULL);
 	        last_cmd = 1;
          break;
        }

        break;

      case 'C':
        last_cmd = 0;
        temp = get_obj_table(ZCMD.arg1);
        if (!temp)
          break;
        temp = real_object(temp);
        if (temp == -1)
          break;
          
        obj_index[temp].limit = ZCMD.arg2; // set the limit from zone file

        if (obj_index[temp].number > ZCMD.arg2)
          break;                /* enough in game */
        if (ZCMD.arg3 == -1)
        {
          /* bad command, disable */
          ZCMD.command = '!';
          break;
        }

        if( (timer = get_current_artifact_info(temp, 0, NULL, NULL, NULL, NULL, FALSE, NULL)) )
        {
          if( timer == -1 )
          {
            get_current_artifact_info(temp, 0, NULL, &room, NULL, NULL, FALSE, &timer);
          }
          else
          {
            //statuslog(56, "didn't load arti obj #%d because already tracked", obj_index[temp].virtual_number);
            break;
          }
        }

        if( !(obj = read_object(temp, REAL)) )
        {
           break;
        }
        // Remove the artifact unless artifact.respawn == 0
        //   or artifact.respawn == 1 and we are not booting.
        if( IS_ARTIFACT(obj) && ( get_property("artifact.respawn", 0) == 0
          || (get_property("artifact.respawn", 0) == 1 && force_item_repop != 2) ))
        {
          extract_obj(obj, TRUE);
          break;
        }
        if( IS_ARTIFACT(obj) && timer )
        {
          obj->timer[3] = timer;
          obj_to_room( obj, real_room(room) );
          break;
        }
        obj_to_room(obj, ZCMD.arg3);
        // Artifact poof timer to BLOOD_DAYS * secs in a day.
        obj->timer[3] = time(NULL);
        last_cmd = 1;

        break;

      case 'A':
        last_cmd = 0;
        temp = get_obj_table(ZCMD.arg1);
        if (!temp)
          break;
        temp = real_object(temp);
        if (temp == -1)
          break;

        obj_index[ZCMD.arg1].limit = ZCMD.arg2; // set the limit from zone file

        if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
        {
          if( (timer = get_current_artifact_info(temp, 0, NULL, NULL, NULL, NULL, FALSE, NULL)) )
          {
            if( timer == -1 )
            {
              get_current_artifact_info(temp, 0, NULL, &room, NULL, NULL, FALSE, &timer);
            }
            else
            {
              //statuslog(56, "didn't load arti obj #%d because already tracked", obj_index[temp].virtual_number);
              break;
            }
          }

          if( !(obj = read_object(temp, REAL)) )
          {
             break;
          }
          // Remove the artifact unless artifact.respawn == 0
          //   or artifact.respawn == 1 and we are not booting.
          if( IS_ARTIFACT(obj) && ( get_property("artifact.respawn", 0) == 0
            || (get_property("artifact.respawn", 0) == 1 && force_item_repop != 2) ))
          {
            extract_obj(obj, TRUE);
            break;
          }
          if( IS_ARTIFACT(obj) && timer )
          {
            obj->timer[3] = timer;
            obj_to_room( obj, real_room(room) );
            break;
          }
          if( mob )              /* last mob */
          {
            obj_to_char(obj, mob);
            // Artifact poof timer to BLOOD_DAYS * secs in a day.
            obj->timer[3] = time(NULL);
            last_cmd = 1;
            break;
          }
        }
        break;

      case 'M':                /* read a mobile */
        mob_index[ZCMD.arg1].limit = ZCMD.arg2; // set the limit from zone file

        if ((mob_index[ZCMD.arg1].number < ZCMD.arg2 &&
              ZCMD.arg4 == 100) || force_item_repop)
        {
          if (ZCMD.arg4 > number(0, 99))
          {
            if (!(mob = read_mobile(ZCMD.arg1, REAL)))
            {
              ZCMD.command = '!';
              logit(LOG_DEBUG,
                    "reset_zone(): (zone %d) mob %d [%d] not loadable", zone,
                    ZCMD.arg1, mob_index[ZCMD.arg1].virtual_number);
            }
          }
          else
          {
            mob = 0;
            last_mob = 0;
            logit(LOG_MOB, "M cmd not executed %d %d %d %d", ZCMD.arg1,
                  ZCMD.arg2, ZCMD.arg3, ZCMD.arg4);
          }
          if (!mob)
          {
            last_cmd = last_mob_load = 0;
            last_mob_followable = 0;
            break;
          }
          tmp_mob = NULL;
          last_mob = last_mob_followable = mob;
          GET_BIRTHPLACE(mob) = world[ZCMD.arg3].number;
          apply_zone_modifier(mob);
          char_to_room(mob, ZCMD.arg3, -2);
          last_cmd = last_mob_load = 1;
        }
        else
          last_cmd = last_mob_load = 0;
        break;

      case 'O':                /* load an object to room */
        obj_index[ZCMD.arg1].limit = ZCMD.arg2; // set the limit from zone file

        if ((ZCMD.arg1 >= 0) && (ZCMD.arg3 >= 0) &&
            ((obj_index[ZCMD.arg1].number < ZCMD.arg2 &&
              ZCMD.arg4 == 100) || force_item_repop))
        {
          if( !(obj = get_obj_in_list_num(ZCMD.arg1, world[ZCMD.arg3].contents)) || IS_SET(obj->wear_flags, ITEM_TAKE) )
          {
            obj = NULL;
            if (ZCMD.arg4 > number(0, 99))
            {
              if( (timer = get_current_artifact_info(ZCMD.arg1, 0, NULL, NULL, NULL, NULL, FALSE, NULL)) )
              {
                if( timer == -1 )
                {
                  get_current_artifact_info(ZCMD.arg1, 0, NULL, &room, NULL, NULL, FALSE, &timer);
                }
                else
                {
                  last_cmd = 0;
                  //statuslog(56, "didn't load arti obj #%d because already tracked", obj_index[temp].virtual_number);
                  break;
                }
              }

              if( !(obj = read_object(ZCMD.arg1, REAL)) )
              {
                ZCMD.command = '!';
                logit(LOG_DEBUG, "reset_zone(): (zone %d) obj %d [%d] not loadable",
                  zone, ZCMD.arg1, obj_index[ZCMD.arg1].virtual_number);
              }
              if( obj )
              {
                if( IS_ARTIFACT(obj)
                  && ( get_property("artifact.respawn", 0) == 0
                    || (get_property("artifact.respawn", 0) == 1 && force_item_repop != 2) ))
                {
                  extract_obj(obj, TRUE);
                  break;
                }
                if( IS_ARTIFACT(obj) && timer )
                {
                  obj->timer[3] = timer;
                  obj_to_room( obj, real_room(room) );
                  break;
                }
                obj_to_room(obj, ZCMD.arg3);
                // Artifact poof timer to BLOOD_DAYS * secs in a day.
                obj->timer[3] = time(NULL);
                last_cmd = 1;
                break;
              }
            }
          }
          else
            last_cmd = 0;
        }
        else if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
        {
          logit(LOG_OBJ, "O cmd: obj: %d to_room: %d, chance: %d, limit %d(%d)",
            obj_index[ZCMD.arg1].virtual_number, (ZCMD.arg3 >= 0) ? world[ZCMD.arg3].number : -2,
            ZCMD.arg4, ZCMD.arg2, obj_index[ZCMD.arg1].number);
          ZCMD.command = '!';   /* disable */
        }
        last_cmd = 0;
        break;

      case 'P':                /* object to object */
        last_cmd = 0;
        obj_index[ZCMD.arg1].limit = ZCMD.arg2; // set the limit from zone file

        if ((ZCMD.arg1 >= 0) && (ZCMD.arg3 >= 0) &&
            ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || force_item_repop))
        {

          if (ZCMD.arg4 > number(0, 99))
          {
            if( get_current_artifact_info(ZCMD.arg1, 0, NULL, NULL, NULL, NULL, FALSE, NULL) )
            {
              if( timer == -1 )
              {
                get_current_artifact_info(ZCMD.arg1, 0, NULL, &room, NULL, NULL, FALSE, &timer);
              }
              else
              {
                //statuslog(56, "didn't load arti obj #%d because already tracked", obj_index[temp].virtual_number);
                break;
              }
            }

            if (!(obj = read_object(ZCMD.arg1, REAL)))
            {
              ZCMD.command = '!';
              logit(LOG_DEBUG, "reset_zone(): (zone %d) obj %d [%d] not loadable", zone, ZCMD.arg1, obj_index[ZCMD.arg1].virtual_number);
            }
            if (obj)
            {
              obj_to = get_obj_num(ZCMD.arg3);
              if (obj_to)
              {
                if( IS_ARTIFACT(obj)
                  && ( get_property("artifact.respawn", 0) == 0
                  || (get_property("artifact.respawn", 0) == 1 && force_item_repop != 2) ) )
                {
                  extract_obj(obj, TRUE);
                  break;
                }
                if( IS_ARTIFACT(obj) && timer )
                {
                  obj->timer[3] = timer;
                  obj_to_room( obj, real_room(room) );
                  break;
                }
                obj->timer[3] = time(NULL);
            		obj_to_obj(obj, obj_to);
                last_cmd = 1;
                break;
              }
            }
          }
        }
        else if( obj_index[ZCMD.arg1].number < ZCMD.arg2 )
        {
          logit(LOG_OBJ, "P cmd: obj: %d to_obj: %d, chance: %d, limit %d(%d)",
            obj_index[ZCMD.arg1].virtual_number, (ZCMD.arg3 >= 0) ? obj_index[ZCMD.arg3].virtual_number : -2,
            ZCMD.arg4, ZCMD.arg2, obj_index[ZCMD.arg1].number);
          ZCMD.command = '!';   /* disable */
        }
        break;

      case 'G':                /* obj_to_char */
        last_cmd = 0;
        obj_index[ZCMD.arg1].limit = ZCMD.arg2; // set the limit from zone file

        if( (ZCMD.arg1 >= 0) && ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || force_item_repop) )
        {

          if (ZCMD.arg4 > number(0, 99))
          {
            if( (timer = get_current_artifact_info(ZCMD.arg1, 0, NULL, NULL, NULL, NULL, FALSE, NULL)) )
            {
              if( timer == -1 )
              {
                get_current_artifact_info(ZCMD.arg1, 0, NULL, &room, NULL, NULL, FALSE, &timer);
              }
              else
              {
                //statuslog(56, "didn't load arti obj #%d because already tracked", obj_index[temp].virtual_number);
                break;
              }
            }

            if( !(obj = read_object(ZCMD.arg1, REAL)) )
            {
              ZCMD.command = '!';
              logit(LOG_DEBUG, "reset_zone(): (zone %d) obj %d [%d] not loadable", zone,
                ZCMD.arg1, obj_index[ZCMD.arg1].virtual_number);
            }
            if( obj )
            {
              if( IS_ARTIFACT(obj)
                && ( get_property("artifact.respawn", 0) == 0
                || (get_property("artifact.respawn", 0) == 1 && force_item_repop != 2) ) )
              {
                extract_obj(obj, TRUE);
                break;
              }
              if( mob )
              {
                //Drannak trying out item stat randomization 3/28/14
                if(!IS_ARTIFACT(obj))
                  randomizeitem(mob, obj);
                // Artifact poof timer to BLOOD_DAYS * secs in a day.
                if( IS_ARTIFACT(obj) && timer )
                {
                  obj->timer[3] = timer;
                  obj_to_room( obj, real_room(room) );
                  break;
                }
                obj->timer[3] = time(NULL);
                obj_to_char(obj, mob);
                last_cmd = 1;
            		break;
              }
              else
              {
                logit(LOG_MOB, "G cmd: obj: %d  chance: %d, limit %d(%d) (no char)", obj_index[ZCMD.arg1].virtual_number, ZCMD.arg4,
                  ZCMD.arg2, obj_index[ZCMD.arg1].number);
                break;
              }
            }
          }
        }
        else if (ZCMD.arg1 < 0)
        {
          logit(LOG_OBJ, "G cmd, bad arg1.  disabling!");
          ZCMD.command = '!';

        }
        else if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
        {
          logit(LOG_OBJ, "G cmd: obj: %d to_char: %d, chance: %d, limit %d(%d)", 
            obj_index[ZCMD.arg1].virtual_number, (ZCMD.arg3 >= 0) ? mob_index[ZCMD.arg3].virtual_number : -2,
            ZCMD.arg4, ZCMD.arg2, obj_index[ZCMD.arg1].number);
          ZCMD.command = '!';   /* disable */
        }
        break;

      case 'E':                /* object to equipment list */
        last_cmd = 0;
        obj_index[ZCMD.arg1].limit = ZCMD.arg2; // set the limit from zone file

        if( (ZCMD.arg1 >= 0) && ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || force_item_repop) )
        {
          if (ZCMD.arg4 > number(0, 99))
          {
            if( (timer = get_current_artifact_info(ZCMD.arg1, 0, NULL, NULL, NULL, NULL, FALSE, NULL)) )
            {
              if( timer == -1 )
              {
                get_current_artifact_info(ZCMD.arg1, 0, NULL, &room, NULL, NULL, FALSE, &timer);
              }
              else
              {
                //statuslog(56, "didn't load arti obj #%d because already tracked", obj_index[temp].virtual_number);
                break;
              }
            }

            if( !(obj = read_object(ZCMD.arg1, REAL)) )
            {
              ZCMD.command = '!';
              logit(LOG_DEBUG, "reset_zone(): (zone %d) obj %d [%d] not loadable", zone, ZCMD.arg1, obj_index[ZCMD.arg1].virtual_number);
            }
            if( obj )
            {
              if( IS_ARTIFACT(obj)
                && ( get_property("artifact.respawn", 0) == 0
                || (get_property("artifact.respawn", 0) == 1 && force_item_repop != 2) ) )
              {
                extract_obj(obj, TRUE);
                break;
              }
              // Artifact poof timer to BLOOD_DAYS * secs in a day.
              if( IS_ARTIFACT(obj) && timer )
              {
                obj->timer[3] = timer;
                obj_to_room( obj, real_room(room) );
                break;
              }
              obj->timer[3] = time(NULL);
              if( mob && (ZCMD.arg3 > 0) && (ZCMD.arg3 <= CUR_MAX_WEAR) )
              {
                // Drannak trying out item stat randomization 3/28/14
                if(!IS_ARTIFACT(obj))
            	  	randomizeitem(mob, obj);
                if (mob->equipment[ZCMD.arg3])
                  unequip_char(mob, ZCMD.arg3);
                equip_char(mob, obj, ZCMD.arg3, 1);
                last_cmd = 1;
                break;
              }
              else
              {
                logit(LOG_OBJ, "E cmd: obj: %d pos: %d(%s) chance: %d, limit %d(%d)",
                  obj_index[ZCMD.arg1].virtual_number, ZCMD.arg3, ((ZCMD.arg3 > 0) && (ZCMD.arg3 <= CUR_MAX_WEAR)) ?
                  equipment_types[ZCMD.arg3] : "ERR", ZCMD.arg4, ZCMD.arg2, obj_index[ZCMD.arg1].number);
                break;
              }
            }
          }
        }
        else if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
        {
          logit(LOG_OBJ, "E cmd: obj: %d pos: %d(%s) chance: %d, limit %d(%d)",
            obj_index[ZCMD.arg1].virtual_number, ZCMD.arg3, ((ZCMD.arg3 > 0) && (ZCMD.arg3 <= CUR_MAX_WEAR)) ?
            equipment_types[ZCMD.arg3] : "ERR", ZCMD.arg4, ZCMD.arg2, obj_index[ZCMD.arg1].number);
          ZCMD.command = '!';   /* disable */
        }
        break;

      case 'F':                /* follow last mob M loaded */
        mob_index[ZCMD.arg1].limit = ZCMD.arg2; // set the limit from zone file

        if (mob_index[ZCMD.arg1].number < ZCMD.arg2)
        {
          if (ZCMD.arg4 > number(0, 99))
          {
            if (!(mob = read_mobile(ZCMD.arg1, REAL)))
            {
              ZCMD.command = '!';
              logit(LOG_DEBUG, "reset_zone(): (zone %d) mob %d [%d] not loadable", zone,
                ZCMD.arg1, mob_index[ZCMD.arg1].virtual_number);
            }
          }
          else
          {
            last_mob_load = 0;
            mob = last_mob = 0;
            logit(LOG_MOB, "F cmd not executed %d %d %d %d", ZCMD.arg1,
              ZCMD.arg2, ZCMD.arg3, ZCMD.arg4);
          }
          if( !last_mob )
          {
            last_cmd = last_mob_load = 0;
            last_mob_followable = 0;
            break;
          }
          tmp_mob = mob;
          GET_BIRTHPLACE(mob) = world[ZCMD.arg3].number;
          apply_zone_modifier(mob);
          char_to_room(mob, ZCMD.arg3, -2);
          add_follower(mob, last_mob_followable);
          strcpy(buf, "group all");
          command_interpreter(last_mob, buf);
          if( !IS_SET(mob->specials.act, ACT_SENTINEL) )
          {
            SET_BIT(mob->specials.act, ACT_SENTINEL);
          }
          last_cmd = last_mob_load = 1;
        }
        else
        {
          last_cmd = last_mob_load = 0;
        }
        break;

      case 'R':                /* last mob loaded with M/F command will mount this */
        mob_index[ZCMD.arg1].limit = ZCMD.arg2; // set the limit from zone file
        if (mob_index[ZCMD.arg1].number < ZCMD.arg2)
        {
          if (ZCMD.arg4 > number(0, 99))
          {
            if (!(mob = read_mobile(ZCMD.arg1, REAL)))
            {
              ZCMD.command = '!';
              logit(LOG_DEBUG, "reset_zone(): (zone %d) mob %d [%d] not loadable",
                zone, ZCMD.arg1, mob_index[ZCMD.arg1].virtual_number);
            }
          }
          else
          {
            mob = 0;
            last_mob_load = 0;
            logit(LOG_MOB, "R cmd not executed %d %d %d %d", ZCMD.arg1,
              ZCMD.arg2, ZCMD.arg3, ZCMD.arg4);
          }
          if( !last_mob )
          {
            last_cmd = last_mob_load = 0;
            break;
          }
          GET_BIRTHPLACE(mob) = world[ZCMD.arg3].number;
          apply_zone_modifier(mob);
          char_to_room(mob, ZCMD.arg3, -2);
          sprintf(buf, "%s", FirstWord(GET_NAME(mob)));
          if (!IS_SET(mob->specials.act, ACT_SENTINEL))
            SET_BIT(mob->specials.act, ACT_SENTINEL);
          if (!IS_SET(mob->specials.act, ACT_MOUNT))
            SET_BIT(mob->specials.act, ACT_MOUNT);
          if (!IS_SET(mob->specials.act, ACT_ISNPC))
            SET_BIT(mob->specials.act, ACT_ISNPC);
          if (tmp_mob)
          {
            do_mount(tmp_mob, buf, 0);
            add_follower(mob, tmp_mob);
          }
          else
          {
            do_mount(last_mob, buf, 0);
            add_follower(mob, last_mob);
          }
          last_cmd = last_mob_load = 1;
        }
        else
          last_cmd = last_mob_load = 0;
        break;

      case 'D':                /* set state of door */
        last_cmd = 0;
        if ((ZCMD.arg1 < 0) || !world[ZCMD.arg1].dir_option[ZCMD.arg2])
        {
          logit(LOG_DEBUG,
                "D cmd: room: %d dir: %d state: %d' has error.",
                (ZCMD.arg1 > 0) ? world[ZCMD.arg1].number : ZCMD.arg1,
                ZCMD.arg2, ZCMD.arg3);
          ZCMD.command = '!';   /* disable */
          break;
        }
        switch (ZCMD.arg3 & 0x03)
        {
        case 0:
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_CLOSED);
          break;
        case 1:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED);
          break;
        case 2:
        case 3:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          break;
        }
        if (ZCMD.arg3 & 0x04)
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_SECRET);
        if (ZCMD.arg3 & 0x08)
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_BLOCKED);
        last_cmd = 1;
        break;

      case '!':
        /* command previously disabled because of error */
        break;
      default:
        logit(LOG_FILE,
              "Undefd cmd in reset table; zone %d cmd #%d command %c.", zone,
              cmd_no, ZCMD.command);
        logit(LOG_DEBUG,
              "Undefd cmd in reset table; zone %d cmd #%d command %c.", zone,
              cmd_no, ZCMD.command);
        ZCMD.command = '!';
        last_cmd = 0;
        break;
      }
    else
      last_cmd = 0;
  }

  if (zone_table[zone].lifespan_min != zone_table[zone].lifespan_max)
    zone_table[zone].lifespan =
      number(zone_table[zone].lifespan_min, zone_table[zone].lifespan_max);
  else
    zone_table[zone].lifespan = zone_table[zone].lifespan_min;

  zone_table[zone].age = 0;

#ifdef SIEGE_ENABLED
  // Check and deploy troops if applicable.
  check_deploy( &zone_table[zone] );
#endif
}

#undef ZCMD

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr)
{
  P_desc   i;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && (i->character->in_room != NOWHERE))
      if (world[i->character->in_room].zone == zone_nr)
        return (0);

  return (1);
}

/************************************************************************
*  procs of a (more or less) general utility nature              *
********************************************************************** */
/* read and allocate space for a '~'-terminated string from a given file.
   Added &n to end of strings with ansi, to prevent 'bleeding'  JAB
*/
char *fread_string(FILE * fl)
{
  char     buf[MAX_STRING_LENGTH], tmp[MAX_STRING_LENGTH], *rslt;
  register char *point;
  int      done = 0, length = 0, templength = 0;

  buf[0] = '\0';

  if( !fl )
  {
    fprintf(stderr, "fread_str: null file pointer!\n");
    return NULL;
  }
  do
  {
    if( !fgets(tmp, MAX_STRING_LENGTH - 5, fl) )
    {
      perror("fread_string");
      logit(LOG_DEBUG, "%s", tmp);
      raise(SIGSEGV);
      return NULL;
    }
    /* If there is a '~', END the string stop; else put an "\r\n" over
       the '\n'. */

    templength = strlen(tmp);

    /* find the last non-whitespace char in tmp */
    for (point = tmp + templength - 1; (point > tmp) &&
         isspace(*point); point--) ;

    /* if its a tilde, we're done :) */
    if (*point == '~')
    {
      *point = '\0';
      templength = strlen(tmp);
      done = 1;
    }
    else
    {
      point = tmp + templength - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    if (length + templength >= MAX_STRING_LENGTH)
    {
      logit(LOG_EXIT, "fread_string: string too large (db.c)");
      raise(SIGSEGV);
    }
    else
    {
      strcat(buf + length, tmp);
      length += strlen(tmp) /*templength */ ;
    }
  }
  while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0)
  {
    /* make sure there is a &n at the end if ANSI codes are imbedded! */
    if (strstr(buf, "&+"))
      if (!((buf[strlen(buf) - 2] == '&') &&
            (toupper(buf[strlen(buf) - 1]) == 'N')))
      {
        strcat(buf, "&n");
        length += 2;
      }
    CREATE(rslt, char, (unsigned) (length + 1), MEM_TAG_STRING);

    strcpy(rslt, buf);
  }
  else
    rslt = NULL;

  return rslt;
}



/*
 * advance file pointer past next '~' terminated string, this saves us an
 * alloc and free when we already HAVE that string in common storage.
 * read_object() and read_mobile() use this after the first call for each
 * obj/mob.  JAB
 */

void skip_fread(FILE * fl)
{
  char     tmp[MAX_STRING_LENGTH];
  register char *point;

  if (!fl)
  {
    fprintf(stderr, "skip_fread: null file pointer!\n");
    return;
  }
  for (;;)
  {
    if (!fgets(tmp, MAX_STRING_LENGTH - 1, fl))
    {
      perror("skip_fread");
      logit(LOG_DEBUG, "%s", tmp);
      raise(SIGSEGV);
    }
    for (point = tmp + strlen(tmp) - 1; (point >= tmp) && isspace(*point);
         point--) ;
    if (*point == '~')
      return;
  }
}

/* release memory allocated for a char struct */
void free_char(P_char ch)
{
  struct affected_type *af, *tmp;
//  struct trophy_data *tr1, *tr2;

  if (!ch)
  {
    logit(LOG_DEBUG, "free_char called with no char!");
    return;
  }
  if ((ch->specials.fighting))
  {
    logit(LOG_EXIT, "free_char: called with a non-extracted char");
    // tmp = (struct affected_type *) (0 / 0);
    tmp = NULL;
  }

  if (ch->player.title)
    str_free(ch->player.title);

  for (af = ch->affected; af; af = tmp)
  {
    tmp = af->next;
    affect_remove(ch, af);
  }

  clear_char_events(ch, -1, NULL);

  if (IS_PC(ch))
    delete_knownShapes(ch);

//  if (IS_PC(ch))                /* clear trophy */
//    for (tr1 = ch->only.pc->trophy; tr1; tr1 = tr2)
//    {
//      tr2 = tr1->next;
//      mm_release(dead_trophy_pool, tr1);
//    }
  /* remove all events associated with this ch  */
  //ClearCharEvents(ch);

#ifdef NEW_COMBAT
  if (ch->points.location_hit)
  {
    FREE((char *) ch->points.location_hit);
    ch->points.location_hit = NULL;
  }
  else
  {
    logit(LOG_DEBUG, "char (%s) had NULL location_hit", GET_NAME(ch));
  }
#endif

  if (IS_NPC(ch))
  {

    /* MOST mob strings should not be freed, as they are shared among
       all mobs with the same Vnum.  Only if a string has been altered
       inside the game, should it be freed here.  */

    if ((ch->only.npc->str_mask & STRUNG_KEYS) && ch->player.name)
      str_free(ch->player.name);

    if ((ch->only.npc->str_mask & STRUNG_DESC1) && ch->player.long_descr)
      str_free(ch->player.long_descr);

    if ((ch->only.npc->str_mask & STRUNG_DESC2) && ch->player.short_descr)
      str_free(ch->player.short_descr);

    if ((ch->only.npc->str_mask & STRUNG_DESC3) && ch->player.description)
      str_free(ch->player.description);
    if (ch->only.npc)
      FREE(ch->only.npc);
    ch->only.npc = NULL;
  }
  else
  {

    /* unlike for mobs, all player strings are unique, so they get
       freed always (if they exist of course) */


    if (ch->only.pc->poofIn)
    {
      str_free(ch->only.pc->poofIn);
    }
    if (ch->only.pc->poofOut)
    {
      str_free(ch->only.pc->poofOut);
    }

    /* title freed earlier */

    if (ch->player.description)
    {
      str_free(ch->player.description);
    }

    if (ch->player.short_descr)
    {
      str_free(ch->player.short_descr);
    }

    if (ch->player.name)
    {
      str_free(ch->player.name);
    }
    else
    {
      logit(LOG_DEBUG, "free_char called with no name. room: (%d)", ch->in_room);
    }

    if( ch->only.pc->log )
    {
      delete ch->only.pc->log;
      ch->only.pc->log = NULL;
    }
    
    mm_release(dead_pconly_pool, ch->only.pc);
    ch->only.pc = NULL;
  }
  SET_POS(ch, GET_POS(ch) + STAT_DEAD);
  add_event(release_mob_mem, 40, ch, 0, 0, 0, 0, 0);
  //release_mob_mem(ch);
  return;
}

/* release memory allocated for an obj struct */
void free_obj(P_obj obj)
{
  struct extra_descr_data *th, *next_one;
  struct obj_affect *af;

  if (!obj)
  {
    logit(LOG_DEBUG, "free_obj called with no obj!");
    return;
  }
  ClearObjEvents(obj);

  while (af = obj->affects)
    obj_affect_remove(obj, af);

  /* MOST obj strings should not be freed, as they are shared among all
     objects with the same Vnum.  Only if a string has been altered
     inside the game, should it be freed here.  */

  if ((obj->str_mask & STRUNG_KEYS) && obj->name)
    str_free(obj->name);

  if ((obj->str_mask & STRUNG_DESC1) && obj->description)
    str_free(obj->description);

  if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
    str_free(obj->short_description);

  if ((obj->str_mask & STRUNG_DESC3) && obj->action_description)
    str_free(obj->action_description);

  obj->str_mask = 0;

  for (th = obj->ex_description; th; th = next_one)
  {
    next_one = th->next;
    if (th->keyword)
    {
      str_free(th->keyword);
      th->keyword = NULL;
    }
    else
      debug("extra description with null keyword for %s",
            obj->short_description);

    if (th->description)
    {
      str_free(th->description);
      th->description = NULL;
    }
    FREE(th);
  }

  obj->ex_description = NULL;
  release_obj_mem(obj);

  obj = NULL;
}

/*
 * read contents of a text file, and place in buf
 * Removed global array, this function now mallocs what it needs SAM 7-94
 */

char    *file_to_string(const char *name)
{
  FILE    *fl;
  char     tmp[256], *ptr;
  char     Gbuf1[MAX_STRING_LENGTH * 20];

  bzero(tmp, 256);
  bzero(Gbuf1, MAX_STRING_LENGTH * 20);

  if (!(fl = fopen(name, "r")))
  {
    if(!(fl = fopen(name, "w")))
    {
      sprintf(tmp, "file-to-string (%s)", name);
      perror(tmp);
      return (NULL);
    }
    
    fclose(fl);
    
    if (!(fl = fopen(name, "r")))
    {
      sprintf(tmp, "file-to-string (%s)", name);
      perror(tmp);
      return (NULL);      
    }
  }
  
  do
  {
    fgets(tmp, 255, fl);

    if (!feof(fl))
    {
      if (strlen(Gbuf1) + strlen(tmp) + 2 > MAX_STRING_LENGTH * 20)
      {
        logit(LOG_FILE, "file_to_string(): file (%s) too long.", name);
        return (NULL);
      }
      strcat(Gbuf1, tmp);
      *(Gbuf1 + strlen(Gbuf1) + 1) = '\0';
      *(Gbuf1 + strlen(Gbuf1)) = '\r';
    }
  }
  while (!feof(fl));

  fclose(fl);
  CREATE(ptr, char, strlen(Gbuf1) + 1, MEM_TAG_STRING);

  strcpy(ptr, Gbuf1);

  return (ptr);
}

/* clear some of the the working variables of a char */

void reset_char(P_char ch)
{
  int      i;

  for (i = 0; i < MAX_WEAR; i++)        /* Initialisering  */
    ch->equipment[i] = 0;

  ch->followers = 0;
  ch->following = 0;
  ch->carrying = 0;
#ifdef REALTIME_COMBAT
  ch->specials.combat = 0;
#else
  ch->specials.next_fighting = 0;
#endif
  ch->specials.destroying_obj = 0;
  ch->next_in_room = 0;
  ch->specials.fighting = 0;
  ch->specials.carry_weight = 0;
  ch->specials.carry_items = 0;
  if (IS_PC(ch))
    ch->only.pc->wiz_invis = 0;
  REMOVE_BIT(ch->specials.act2, PLR2_WAIT);

  // we store diff now, so leave at 0 (Lom)
  if (GET_HIT(ch) < 0)
  {
    GET_HIT(ch) = 0;
  }
  if (GET_VITALITY(ch) <= 0)
  {
    GET_VITALITY(ch) = 1;
  }
  if (GET_MANA(ch) <= 0)
  {
    GET_MANA(ch) = 1;
  }
}

/* clear ALL the working variables of a char & NOT free any space alloc'ed */
void clear_char(P_char ch)
{
  int      i;

  bzero(ch, sizeof(struct char_data));

  ch->in_room = NOWHERE;
  ch->specials.was_in_room = NOWHERE;
  SET_POS(ch, POS_STANDING + STAT_NORMAL);
#if 1
  ch->points.base_armor = 0;  /* Basic Armor */
#endif

  /*
   * Zero out our other flags -- this should have been done when
   * we did 'bzero', but just in case
   */

  ch->affected = NULL;

	ch->lobj = NULL;
}

/*
 * the reason to have real_room0(), real_mobile0(), and real_object0()
 * that mirrors the original functions is very simple.  These new
 * functions returns 0 instead of -1 for items not found in database. So
 * in spec_ass.c when it calls these functions it won't be assigning to a
 * -1 array index entry.  This causes some problem when zones are removed
 * or ids are reassigned. -DCL
 */

/* returns the real number of the zone with given virtual number */
int real_zone0(const int virt)
{
  register int bot, top, mid;
  
  bot = 0;
  top = top_of_zone_table;
  
  if (virt == -1)
    return 0;
  
  /*
   * perform binary search on world-table
   */
  for (;;)
  {
    mid = (bot + top) >> 1;
    
    if ((zone_table + mid)->number == virt)
      return (mid);
    if (bot >= top)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "real_zone0: Zone %d not in database", virt);
#endif
      debug( "real_zone0: Zone %d not in database", virt);
      return (0);
    }
    if ((zone_table + mid)->number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * returns the real number of the zone with given virtual number
 */
int real_zone(const int virt)
{
  register int bot, top, mid;
  
  bot = 0;
  top = top_of_zone_table;
  if (virt == -1)
    return -1;
  
  /*
   * perform binary search on world-table
   */
  for (;;)
  {
    mid = (bot + top) >> 1;
    
    if ((zone_table + mid)->number == virt)
      return (mid);
    if (bot >= top)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "real_zone: Zone %d not in database", virt);
#endif
      return (-1);
    }
    if ((zone_table + mid)->number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/* returns the real number of the room with given virtual number */
int real_room0(const int virt)
{
  register int bot, top, mid;

  bot = 0;
  top = top_of_world;

  if (virt == -1)
    return 0;

  /*
   * perform binary search on world-table
   */
  for (;;)
  {
    mid = (bot + top) >> 1;

    if ((world + mid)->number == virt)
      return (mid);
    if (bot >= top)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "real_room0: Room %d not in database", virt);
#endif
      return (0);
    }
    if ((world + mid)->number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * returns the real number of the room with given virtual number
 */

int real_room(const int virt)
{
  register int bot, top, mid;

  bot = 0;
  top = top_of_world;
  if (virt == -1)
    return -1;

  /*
   * perform binary search on world-table
   */
  for (;;)
  {
    mid = (bot + top) >> 1;

    if ((world + mid)->number == virt)
      return (mid);
    if (bot >= top)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "real_room: Room %d not in database", virt);
#endif
      return (-1);
    }
    if ((world + mid)->number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * returns the real number of the monster with given virtual number
 */

int real_mobile0(const int virt)
{
  register int bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /*
   * perform binary search on mob-table
   */
  for (;;)
  {
    mid = (bot + top) >> 1;

    if ((mob_index + mid)->virtual_number == virt)
      return (mid);
    if (bot >= top)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "real_mobile0: Mob %d not in database", virt);
#endif
      return (0);
    }
    if ((mob_index + mid)->virtual_number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * returns the real number of the monster with given virtual number
 */

int real_mobile(const int virt)
{
  register int bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /*
   * perform binary search on mob-table
   */
  for (;;)
  {
    mid = (bot + top) >> 1;

    if ((mob_index + mid)->virtual_number == virt)
      return (mid);
    if (bot >= top)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "real_mobile: Mob %d not in database", virt);
#endif
      return (-1);
    }
    if ((mob_index + mid)->virtual_number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * returns the real number of the object with given virtual number
 */

int real_object0(const int virt)
{
  register int bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /*
   * perform binary search on obj-table
   */
  for (;;)
  {
    mid = (bot + top) >> 1;

    if ((obj_index + mid)->virtual_number == virt)
      return (mid);
    if (bot >= top)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "real_object0: Obj %d not in database", virt);
#endif
      return (0);
    }
    if ((obj_index + mid)->virtual_number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * returns the real number of the object with given virtual number
 */

int real_object(const int virt)
{
  register int bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /*
   * perform binary search on obj-table
   */
  for (;;)
  {
    mid = (bot + top) >> 1;

    if ((obj_index + mid)->virtual_number == virt)
      return (mid);
    if (bot >= top)
    {
#if DB_NOTIFY
      logit(LOG_DEBUG, "real_object: Obj %d not in database", virt);
#endif
      return (-1);
    }
    if ((obj_index + mid)->virtual_number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}
void worldcheck(P_char ch)
{
  int      i;
  char     tmp_buf[MAX_STRING_LENGTH];

  for (i = 1; i < top_of_world; i++)
  {
    if (world[i].number <= world[i - 1].number)
    {
      sprintf(tmp_buf,
              "Real: %d Virtual: %d is out of order with Real: %d Virtual: %d\r\n",
              i, world[i].number, i - 1, world[i - 1].number);
      send_to_char(tmp_buf, ch);
    }
  }
}

int InsertIntoFile(const char *filename, const char *text)
{
  FILE    *fin = 0;
  unsigned char *buffer = 0;
  long     sizeOfFile = 0;
  unsigned int sizeToRead = 0;
  FILE    *fout = 0;

  // open the existing bug file for read
  fin = fopen(filename, "rb");
  if (fin != NULL)
  {
    // create the buffer to hold the file's current contents
    CREATE(buffer, unsigned char, MAX_STRING_LENGTH, MEM_TAG_BUFFER);
    if (buffer == NULL)
    {
      fclose(fin);
      return 1;
    }

    // determine the size of the file
    fseek(fin, 0, SEEK_END);
    sizeOfFile = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    // determine if the file is larger than the max size
    sizeToRead = 0;
    if (sizeOfFile <= MAX_STRING_LENGTH)
      sizeToRead = (int) sizeOfFile;
    else
      sizeToRead = MAX_STRING_LENGTH;

    // read the file until the max size or end is found
    fread(buffer, sizeToRead, 1, fin);

    // close the input file
    fclose(fin);
  }

  // open the file again to be rewritten
  fout = fopen(filename, "wb");
  if (fout == NULL)
  {
    FREE(buffer);
    return 2;
  }

  // write the formatted text out to the file
  fputs(text, fout);

  // write out the existing file's contents
  if (buffer != NULL)
  {
    fwrite(buffer, sizeToRead, 1, fout);    
    FREE(buffer);
  }

  fclose(fout);

  return 0;
}

#if 0
void load_obj_limits()
{
  FILE    *f;
  int      vnum, max, rnum, rnum;

  f = fopen("obj_limits", "r");
  if (!f)
  {
    save_obj_limits();
    return;
  }
  while (!feof(f))
  {
    fgets(buf, sizeof(buf) - 1, f);
    if (sscanf(buf, "%d %d %d", &vnum, &max, &rented) == 3)
    {
      rnum = real_object(vnum);
      if (rnum != -1)
      {
        obj_index[rnum].max = max;
        obj_index[rnum].rnumber = rented;
      }
    }
  }
  fclose(f);
}

void save_obj_limits()
{
  FILE    *f;
  int      i;

  f = fopen("obj_limits", "w");
  for (i = 0; i <= top_of_objt; i++)
  {
    fprintf(f, "%d %d %d\n", obj_index[i].virtual_number,
            obj_index[i].max, obj_index[i].rnumber);
  }
  fclose(f);
}

#endif
