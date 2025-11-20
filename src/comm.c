/*
 **************************************************************************
 *  File: comm.c                                             Part of Duris
 *  Usage: socket handling, main game loop
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 **************************************************************************
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <zlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <gnutls/gnutls.h>

#include "helpfile.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "lookup_process.h"
#include "olc.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "mm.h"
#include "spells.h"
#include "ships.h"
#include "telnet.h"
#include "mccp.h"
#include "sql.h"
#include "ferry.h"
#include "world_quest.h"
#include "auction_houses.h"
#include "epic.h"
#include "nexus_stones.h"
#include "racewar_stat_mods.h"
#include "player_log.h"
#include "tradeskill.h"
#include "map.h"
#include "assocs.h"
#include "timers.h"
#include "graph.h"
#include "profile.h"
#include "guildhall.h"
#include "outposts.h"
#include "boon.h"
#include "ctf.h"
#include "hardcore.h"
#include "siege.h"
#include "utility.h"

/* external variables */

extern P_event schedule[];
extern P_index mob_index;
extern P_room world;
extern char debug_mode;
extern const int top_of_world;
extern int top_of_zone_table;
extern struct ban_t *ban_list;
extern struct wizban_t *wizconnect;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern const char *shutdown_message;
extern const int max_ingame_good;
extern const int max_ingame_evil;

long     sentbytes = 0;
long     recivedbytes = 0;
bool     game_booted = FALSE;

extern void ne_events();

long unsigned int ip2ul(const char *ip);
void load_alliances();
void initialize_transport();
bool newLeaderBoard(P_char ch, char *arg, int cmd);
bool newHardcoreBoard(P_char ch, char *arg, int cmd);
void format_to_snoopers( char *from_string, char *to_string );
extern void update_breath_weapon_properties();
extern void update_regen_properties();
static void greet(P_desc newd);

/* local globals */

P_desc   descriptor_list, next_to_process, next_save = 0;
fd_set   input_set, output_set, exc_set;        /* for socket handling */
int      bounce_null_sites = 0;
int      mini_mode = 0;
int      lawful = 0;
int      no_specials = 0;
int      override = 1;
int      pulse = 0;
bool     after_events_call = FALSE;
int      _reboot = 0;
int      _copyover = 0;
int      _autoboot = 0;
int      _pwipe = 0;
int      req_passwd = 1;
int      shutdownflag = 0;
int      slow_death = 0;
int      tics = 0;
long     boot_time;
int      ipc_id = 0;
int      was_upper = FALSE;
pid_t    lookup_host_process;
pid_t    lookup_ident_process;
int      max_users_playing = 0;
int      used_descs = 0, avail_descs = 0, max_descs = 0;
struct mm_ds *dead_desc_pool = NULL;
int      RUNNING_PORT = 0;
int      no_random = 0;
int      no_ferries = 0;
P_char   executing_ch;
#define  MAX_COMMAND_OUTPUT (5 * MAX_STRING_LENGTH)
#define  PAD_COMMAND_OUTPUT (500)  // some space for appending a warning
char     command_output[MAX_COMMAND_OUTPUT + PAD_COMMAND_OUTPUT + 1];
size_t   output_length;

/* ============= Ansi color table and data structure  ============== */

struct ansi_color
{
  const char *symbol;           /* Symbol used in the game. eg &+symbol */
  const char *fg_code;          /* ^[[fg_codem; = foreground color code */
  const char *bg_code;          /* Same except it's the background color */
} color_table[] =
{

  {
  "L", "30", "40"},             /* * Black            */
  {
  "R", "31", "41"},             /* * Red              */
  {
  "G", "32", "42"},             /* * Green            */
  {
  "Y", "33", "43"},             /* * Yellow           */
  {
  "B", "34", "44"},             /* * Blue             */
  {
  "M", "35", "45"},             /* * Magenta          */
  {
  "C", "36", "46"},             /* * Cyan             */
  {
  "W", "37", "47"},             /* * White            */
  {
  NULL, NULL, NULL}             /* * End of the table */
};

#define MIN_SOCKET_BUFFER_SIZE 20480

const char *COMA_SIGN = "\r\n\
DurisMUD is currently inactive due to excessive load on the host machine.\r\n\
Please try again later.\r\n\r\n\
Sadly, \r\n\r\n\
the DurisMUD system operators\r\n\r\n";

/*
 * ********************************************************************* *
 * main game loop and related stuff                                 *
 * *********************************************************************
 */

int main(int argc, char **argv)
{
  int      port, sslport;
  int      pos = 1;
  const char *dir;

  port = DFLT_PORT;
  dir = DFLT_DIR;
  sslport = SSL_PORT;

  init_genrand(time(NULL));

  while ((pos < argc) && (*(argv[pos]) == '-'))
  {
    switch (*(argv[pos] + 1))
    {
    case 'f':
      no_ferries = 1;
      logit(LOG_STATUS, "Without ferries.");
      break;
    case 'l':
      no_random = 1;
      // lawful = 1;
      logit(LOG_STATUS, "Without randoms.");
      break;
    case 'd':
      if (*(argv[pos] + 2))
        dir = argv[pos] + 2;
      else if (++pos < argc)
        dir = argv[pos];
      else
      {
        logit(LOG_EXIT, "Directory arg expected after option -d.");
        raise(SIGSEGV);
      }
      break;
    case 's':
      no_specials = 1;
      logit(LOG_STATUS, "Suppressing assignment of special routines.");
      break;
    case 'p':
      req_passwd = 0;
      logit(LOG_STATUS, "Allowing changing of password without old one.");
      break;
    case 'n':
      bounce_null_sites = 1;
      logit(LOG_STATUS, "Bouncing null sites");
      break;
    case 'm':
      mini_mode = 1;
      no_ferries = 1;
      logit(LOG_STATUS, "Running in mini mode");
      break;
    case 'z':
      mini_mode = 2;
      logit(LOG_STATUS, "Running with area debugger on");
      break;
    default:
      logit(LOG_STATUS, "Unknown option -% in argument string.",
            *(argv[pos] + 1));
      break;
    }
    pos++;
  }

  if (pos < argc)
    if (!isdigit(*argv[pos]))
    {
      fprintf(stderr,
              "Usage: %s [-l] [-m] [-s] [-p] [-n] [-f] [-d pathname] [ port # ]\n",
              argv[0]);
      raise(SIGSEGV);
    }
    else if ((port = atoi(argv[pos])) <= 1024)
    {
      printf("Illegal port #\n");
      raise(SIGSEGV);
    }
    else
      sslport = port + 1;
//Global variable so can check if mainmud or not!
  RUNNING_PORT = port;

  /* create an IPC msg queue to deal with hostname lookups.  */
/*
  ipc_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);
  if (ipc_id < 0) {
    fprintf(stderr, "Unable to create message queue due to %d!\r\n", ipc_id);
    raise(SIGSEGV);;
  }
*/
  /* fork() off a new process to deal with hostname lookups. */
  /* fork will return 0 to the newly created process */

/*
  if (!(lookup_host_process = fork()))
    exit(run_lookup_host_process(ipc_id));
*/
/*
  if (!(lookup_ident_process = fork()))
    exit(run_lookup_ident_process(ipc_id));
*/
  if (chdir(dir) < 0)
  {
    perror("chdir");
    raise(SIGSEGV);
  }
  logit(LOG_STATUS, "Running game on port %d.", port);

  logit(LOG_STATUS, "Using %s as data directory.", dir);

  if( initialize_mysql() < 0 )
  {
    fprintf(stderr, "MySQL initialization failed! Dying!");
    raise(SIGSEGV);
  }

  initialize_properties();

  load_event_names();

  setrandom();                  /* new random functions - SAM */

  init_cmdlog();                /* init cmd.debug file - DCL */

  run_the_game(port, sslport);

  return (0);
}

// all text meant to go to executing_ch - a player whos command
// is currently processed is saved into command_output
// buffer instead of being sent over the network
// the intercepting happens in send_to_char
void process_with_paging(P_char ch, char *comm)
{
  executing_ch = ch;
  *command_output = '\0';
  output_length = 0;
  command_interpreter(ch, comm);
  executing_ch = NULL;
  if (!ch->desc)
    return;
  else if (next_page(command_output, ch->desc))
    //page_string_real(ch->desc, command_output, 1);
    page_string_real(ch->desc, command_output);
  else
    SEND_TO_Q(command_output, ch->desc);
}

void game_up_message(int port)
{
  FILE    *f;
  char     Gbuf1[200];

  f = fopen("foo_tmp", "w");
  snprintf(Gbuf1, 200, "Duris> The mud is up at port %d. Run! Panic! *FLEE*\n",
          port);
  fputs(Gbuf1, f);
  fclose(f);
  system("/usr/local/bin/stealth-wall < foo_tmp");
  unlink("foo_tmp");
//  signal(SIGCHLD, (void *) reaper);

}

/* Init sockets, run game, and cleanup sockets */

void run_the_game(int port, int sslport)
{
  long     time_before = 0;
  long     time_after = 0;
  char     buf[MAX_STRING_LENGTH];

  descriptor_list = NULL;

  time_before = clock();

  logit(LOG_STATUS, "Signal trapping.");
  signal_setup();

  SetSpellCircles();            /* spells circlewise done with pure math */

  boot_db(mini_mode);

  //game_up_message(port);
  init_astral_clock();          // fix the map sight distances

  if (no_random == 0)
    create_randoms();
  else
    fprintf(stderr, "Starting without random zones!.\n\r");

  fprintf(stderr, "-- Updating zone database.\r\n");
  update_zone_db();

  calculate_map_coordinates();
  fprintf(stderr, "--  Done calculating maps coordinates.\r\n");

  fprintf(stderr, "-- Calculating avg mob level for each zone.\r\n");
  calc_zone_mob_level();
  fprintf(stderr, "--  Done calculating mob level.\r\n");

  initialize_tradeskills();
  fprintf(stderr, "--  Done loading tradeskills/mines.\r\n");

  load_cmd_attributes();
  fprintf(stderr, "--  Done loading command attributes.\r\n");

  if ( no_ferries == 0 )
    init_ferries();
  else
    fprintf(stderr, "Starting without ferries.\r\n");

  initialize_transport();

  update_breath_weapon_properties();
  update_regen_properties();

  //initialize_buildings();

  Guild::initialize();
  fprintf(stderr, "-- Done loading guilds\r\n");

  Guildhall::initialize();
  fprintf(stderr, "-- Done loading guildhalls\r\n");

  init_auction_houses();

  reset_racewar_stat_mods();
  init_nexus_stones();

  init_outposts();

  fprintf(stderr, "-- Loading alliances\r\n");
  load_alliances();

#ifdef SIEGE_ENABLED
  fprintf(stderr, "-- Loading town data\r\n");
  init_towns();

  fprintf(stderr, "-- Loading siege data\r\n");
  init_siege();
#endif

  // This guarentees that files exist for reading.
  fprintf(stderr, "-- Touching leaderboard\r\n");
  snprintf(buf, MAX_STRING_LENGTH, "touch %s", leaderboard_file );
  system( buf );
  newLeaderBoard( NULL, "boot", 0 );
  fprintf(stderr, "-- Touching hall of fame\r\n");
  snprintf(buf, MAX_STRING_LENGTH, "touch %s", halloffamelist_file );
  system( buf );
  newHardcoreBoard( NULL, "boot", 0 );
  init_ctf();

  loadHints();
  epic_initialization();
  ssl_read_cert();
  time_after = clock();
  bfs_reset_marks();
  fprintf(stderr, "Boot completed in: %d milliseconds\n",
          (int) ((time_after - time_before) * 1E3 / CLOCKS_PER_SEC));
  logit(LOG_STATUS, "Boot completed in:%d milliseconds\n",
        (int) ((time_after - time_before) * 1E3 / CLOCKS_PER_SEC));

  game_booted = TRUE;

  fprintf(stderr, "Entering game loop.\n\r");
  logit(LOG_STATUS, "Entering game loop.");
  game_loop(port, sslport);

  /* Don't need this anymore, as dropped artis are handled in real time on the DB.
  // Look for dropped artis and remove them from the next boot.
  dropped_arti_hunt();
  */

#ifdef MEMCHK
  free_world();
  dump_mem_log();
#endif

  if( _reboot )
  {
    logit(LOG_EXIT, "Rebooting.");
    logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
    exit(52);                   /* what's so great about HHGTTG, anyhow? */
  }
  if( _copyover )
  {
    if( _autoboot )
    {
      logit(LOG_EXIT, "Auto reboot with copyover.");
      logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
      exit(57);
    }
    else
    {
      logit(LOG_EXIT, "Copyover reboot.");
      logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
      exit(53);
    }
  }
  if( _autoboot )
  {
    logit(LOG_EXIT, "Auto reboot.");
    logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
    exit(54);
  }
  if( _pwipe )
  {
    logit(LOG_EXIT, "Pwipe Shutdown.");
    logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
    exit(55);
  }
  logit(LOG_EXIT, "Normal termination of game.");
  logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
  logit(LOG_STATUS, "Normal termination of game.");
}

/* Accept new connects, relay commands, and call 'heartbeat-functs' */

void game_loop(int port, int sslport)
{
  P_char   t_ch = NULL;
  P_desc   point, next_point;
  char     buf[MAX_STRING_LENGTH];
  char     comm[MAX_INPUT_LENGTH];
  char     Debug[MAX_INPUT_LENGTH];
  int      player_count;
  static struct timeval opt_time;
  struct timeval last_time, now, timespent, timeout, null_time;
  struct host_answer host_ans_buf;
  struct ident_answer ident_ans_buf;
  sigset_t mask, oldset;
  int s, S;
  
  sentbytes = 0;
  recivedbytes = 0;
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;

  opt_time.tv_usec = OPT_USEC;  /* Init time values */
  opt_time.tv_sec = 0;
#if 1
  gettimeofday(&last_time, (struct timezone *) 0);
#endif
  pulse = 0;

#ifdef        DO_SET_DTABLE_SIZE
  (void) setdtablesize(128);
#endif

#if 1
  avail_descs = MAX_CONNECTIONS;
#else
  avail_descs = getdtablesize() - 10;
#endif

  snprintf(buf, MAX_STRING_LENGTH, "avail_descs set to: %d", avail_descs);
  logit(LOG_STATUS, buf);

  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigaddset(&mask, SIGUSR2);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGPIPE);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGALRM);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGURG);
  sigaddset(&mask, SIGSEGV);

#ifdef USE_ASYNCHRONOUS_IO
  io_init();
#endif

  dead_desc_pool = mm_create("SOCKET",
                             sizeof(struct descriptor_data),
                             offsetof(struct descriptor_data, next),
                             mm_find_best_chunk(sizeof
                                                (struct descriptor_data), 25,
                                                110));
  PROFILES(RESET);
#ifdef DO_PROFILE
  init_func_call_info();
#endif
  
  logit(LOG_STATUS, "Opening mother connection.");
  s = init_socket(port);
  logit(LOG_STATUS, "Opening father connection.");
  S = init_socket(sslport);

  /* Main loop */
  while (!shutdownflag)
  {
/*
    struct host_answer host_ans_buf;
    struct ident_answer ident_ans_buf;
*/
    bzero(&host_ans_buf, sizeof(host_ans_buf));
    bzero(&ident_ans_buf, sizeof(ident_ans_buf));
    /* Check for answers to hostname queuries */
    /* just ignore errors (hope they are all "no message" errors) */
#if 0
    if (msgrcv(ipc_id, (struct msgbuf *) &host_ans_buf,
               sizeof(struct host_answer) - sizeof(long),
               MSG_HOST_ANS, IPC_NOWAIT) == -1)
      host_ans_buf.desc = s;    /* so nothing happens  */
#endif
/*
    if (msgrcv(ipc_id, (struct msgbuf *) &ident_ans_buf,
               sizeof(struct ident_answer) - sizeof(long),
               MSG_IDENT_ANS, IPC_NOWAIT) == -1)
           ident_ans_buf.desc = s;
*/
    /* Check what's happening out there */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);

    /* Get the file descriptors for asynchrnonous IO */

#ifdef USE_ASYNCHRONOUS_IO
    input_set = io_readfds;
    output_set = io_writefds;
    exc_set = io_exceptfds;
#endif

    /* Continue with original code */
    PROFILE_START(connections);
    FD_SET(s, &input_set);
    FD_SET(S, &input_set);
    for (point = descriptor_list; point; point = point->next)
    {
      /*
       * while we are looping through descriptors, it would be a
       * good time to see if the message answer we checked for
       * before matches
       */

      if ((point->descriptor == ident_ans_buf.desc) &&
          (*((time_t *) (point->login + 4)) == ident_ans_buf.stamp))
        strcpy(point->login, ident_ans_buf.name);

      if ((point->descriptor == host_ans_buf.desc) &&
          !strncmp(host_ans_buf.addr, point->host /*+ 3 */ ,
                   strlen(host_ans_buf.addr)))
      {
        /* we have a match! */
        strncpy(point->host, host_ans_buf.name, 49);
        point->host[49] = 0;

        /* site ban code, skip if address is junk */
        snprintf(buf, MAX_STRING_LENGTH, "%s\r\n", point->host);
        SEND_TO_Q(buf, point);
        if (bannedsite(point->host, 0) || bannedsite(host_ans_buf.addr, 0))
        {
          write_to_descriptor(point,
                              "Your site has been banned from being able to connect to Duris.\r\n"
                              "You were banned because someone at your site has flagrantly violated\r\n"
                              "the rules to a point where banning your site was necessary.  If you\r\n"
                              "feel this is in error, please e-mail multiplay@durismud.com\r\n");
          banlog(56, "Reject Connect from %s, banned site.", point->host);
          logit(LOG_STATUS, "Rejected Connect from %s, banned site.", point->host);
          close_socket(point);
          continue;
        }
        else
        {
          /* good connection, send them on their way :) */
          SEND_TO_Q("Please enter your term type (<CR> ansi, '3' MSP, '?' help): ", point);
          point->connected = CON_GET_TERM;
          point->wait = 1;
        }
      }
      FD_SET(point->descriptor, &input_set);
      FD_SET(point->descriptor, &exc_set);
      FD_SET(point->descriptor, &output_set);
    }

    sigprocmask(SIG_SETMASK, &mask, &oldset);

    if (select(FD_SETSIZE, &input_set, &output_set, &exc_set, &null_time) < 0)
    {
      perror("Select poll");
      continue;
    }
    sigprocmask(SIG_SETMASK, &oldset, 0);

    /*
     ** Handle the asynchronous IO first.
     **
     ** Note that it is IMPORTANT that asynchronous is done before
     ** anything else.  Reason:  if we process something else, it
     ** is conceivable for the user to type in another command,
     ** i.e. "rent", then "kill receptionist", which will mean
     ** that the player will start attacking receptionist, but
     ** then he would have RENTED!!!!!!
     */

#ifdef USE_ASYNCHRONOUS_IO
    (void) io_processFDS(&input_set, &output_set, &exc_set);
#endif

    /* Respond to whatever might be happening */

    /* New connection? */
    if (FD_ISSET(s, &input_set))
      if (new_descriptor(s, 0) < 0)
        perror("New connection");
    /* New secure connection? */
    if (FD_ISSET(S, &input_set))
      if (new_descriptor(S, 1) < 0)
        perror("New connection");

    /* kick out the freaky folks */
    for (point = descriptor_list; point; point = next_point)
    {
      next_point = point->next;
      if (FD_ISSET(point->descriptor, &exc_set))
      {
        logit(LOG_COMM, "Closing socket with exception.  FIXME!");
        close_socket(point);
      }
      else if (FD_ISSET(point->descriptor, &input_set))
        if (point->connected != CON_SSLNEGO && process_input(point) < 0)
        {
          close_socket(point);
        }
    }
    PROFILE_END(connections);

#if 0
    if (debug_mode)
      loop_debug();
#endif

    /* process_commands */
    PROFILE_START(commands);
    for( point = descriptor_list, player_count = 0; point; point = next_to_process )
    {
      next_to_process = point->next;
      t_ch = point->character;

      if (point->connected == CON_SSLNEGO)
      {
        switch (ssl_negotiate(point->sslses))
        {
        case 0:
          greet(point);
          break;
        default:
          close_socket(point);
        case 1:
          continue;
        }
      }

      /* update max_users_playing for "who" information */
      if ((point->connected) == CON_PLAYING)
      {
        player_count++;
        if (player_count > max_users_playing)
          max_users_playing = player_count;
      }
      /* new timeout for non-playing sockets */

      if (point->connected)
      {
        point->wait++;

        switch (point->connected)
        {
          /* these are either yes/no or <return> 60 second timeout */
        case CON_APPROPRIATE_NAME:
        case CON_FLUSH:
        case CON_NAME_CONF:
        case CON_GET_SEX:
        case CON_GET_TERM:
          if (point->wait > 240)
          {
            write_to_descriptor(point, "Idle Timeout\n");
            close_socket(point);
            continue;
          }
          break;

          /* slightly more involved, 10 minute timeout */
        case CON_ALIGN:
        case CON_BONUS1:
        case CON_BONUS2:
        case CON_BONUS3:
        case CON_HOMETOWN:
        case CON_NAME:
        case CON_PWD_CONF:
        case CON_PWD_D_CONF:
        case CON_PWD_GET:
        case CON_PWD_NO_CONF:
        case CON_PWD_NEW:
        case CON_PWD_GET_NEW:
        case CON_PWD_NORM:
        case CON_GET_CLASS:
        case CON_GET_RACE:
        case CON_GET_RETURN:
        case CON_REROLL:
          if (point->wait > 2400)
          {
            write_to_descriptor(point, "Idle Timeout\n");
            close_socket(point);
            continue;
          }
          break;
          /*
           * for remaining states, 15 minutes, same as idle
           * timeout in game
           */
        default:
          if (point->wait > 3600)
          {
            write_to_descriptor(point, "Idle Timeout\n");
            close_socket(point);
            continue;
          }
          break;
        }
      }
      else if (IS_AFFECTED2(t_ch, AFF2_SLOW) && !IS_TRUSTED(t_ch) &&
               (pulse % 2) && !GET_CLASS(t_ch, CLASS_MONK))
        continue;
      else if (affected_by_spell(t_ch, TAG_CTF) && !IS_TRUSTED(t_ch) &&
	       (pulse % (int)get_property("ctf.slowness", 3)))
	continue;

      /* check for hella long wait time here..  bandaid solution but it should (sort of) work */

      if( (!t_ch || (t_ch && (CAN_ACT(t_ch) && !IS_SET(t_ch->specials.affected_by, AFF_CHARM))))
        && get_from_q(&point->input, comm) )
      {

        if( t_ch )
        {
          t_ch->specials.timer = 0;
        }
        point->prompt_mode = TRUE;

/*        if (point->olc)
          olc_string_add(point->olc, comm);
                                                                else */ if (point->showstr_count)
                                                        /* pager for text */
          show_string(point, comm);
        else if (point->str)    /* mail, boards */
          string_add(point, comm);
        else if (point->connected == CON_PLAYING)
        {
          if (t_ch && t_ch->desc && IS_SET(t_ch->specials.act, PLR_PAGING_ON))
          {
            process_with_paging(t_ch, comm);
          }
          else
            command_interpreter(t_ch, comm);
        }
        else
        {
          point->wait = 0;
          nanny(point, comm);
        }
      }
    }
    PROFILE_END(commands);

    PROFILE_START(prompts);
    for (point = descriptor_list; point; point = next_point)
    {
      next_point = point->next;
      if (FD_ISSET(point->descriptor, &output_set) && point->output.head)
        if (process_output(point) < 0)
        {
          close_socket(point);
        }
        else
#ifdef SMART_PROMPT
        if (point->character && (IS_PC(point->character) || IS_MORPH(point->character)))
        {
          if( IS_SET(GET_PLYR(point->character)->specials.act, PLR_OLDSMARTP)
            && !point->showstr_count && !point->str && !point->olc && !IS_FIGHTING(GET_PLYR(point->character)))
          {
            point->prompt_mode = FALSE;
          }
          else if( !IS_SET(GET_PLYR(point->character)->specials.act, PLR_SMARTPROMPT) )
          {
            point->prompt_mode = TRUE;
          }
        }
/*
              !IS_SET(GET_PLYR(point->character)->specials.act, PLR_SMARTPROMPT)
           && !IS_SET(GET_PLYR(point->character)->specials.act, PLR_OLDSMARTP))
*/
#endif
//        point->prompt_mode = TRUE;
    }

    /* give the people some prompts */
    make_prompt();
    PROFILE_END(prompts);

    /* handle heartbeat stuff */
    /* Note: pulse now changes every 1/4 sec  */
    after_events_call = TRUE;
    /* Not using old events anymore.
    if (schedule[pulse])
    {
      Events();
    }
    */
    ne_events();

    PROFILE_START(activities);
    if (!(pulse % (WAIT_SEC * 60)))
      timers_activity();

    if (!(pulse % WAIT_SEC))
      ship_activity();

    if( !no_ferries && !(pulse % WAIT_SEC) )
      ferry_activity();
    
    if(!(pulse % (WAIT_SEC * 60)))
      auction_houses_activity();

    if (!(pulse % (WAIT_SEC * 120)))
        spawn_random_mapmob();
        
//    if (!(pulse % WAIT_SEC))
//      arena_activity();

    if (!(pulse % SHORT_AFFECT))
      short_affect_update();

    if (!(pulse % PULSES_IN_TICK) && !mini_mode)
      web_info();

    if( !(pulse % ( WAIT_SEC * 300 )) )
      wimps_in_approve_queue();

    if (!(pulse % (WAIT_SEC * 120)))
    {
      epic_zone_balance();
    }

    if (!(pulse % WAIT_SEC))
      boon_maintenance();

    PROFILE_END(activities);

    PROFILE_START(combat);
    perform_violence();

    /* for action_delays[] related to combat --TAM 04/19/94 */
    for (point = descriptor_list; point; point = point->next)
    {
      if (point->character && point->connected == CON_PLAYING)
      {

        t_ch = point->character;

        if (!pulse)
        {
          if (IS_SET(t_ch->specials.act2, PLR2_HINT_CHANNEL))
          {
            tossHint(t_ch);
          }
        }
        if (t_ch->desc->last_map_update)
        {
          map_look(t_ch, MAP_AUTOMAP);
          t_ch->desc->last_map_update = 0;
        }
        if (t_ch->desc->last_group_update)
        {
          do_group(t_ch, "", 0);
          t_ch->desc->last_group_update = 0;
        }
        if (t_ch->points.delay_move > 0)
          t_ch->points.delay_move -=
            BOUNDED(0, !IS_MAP_ROOM(t_ch->in_room) ?
                    move_regen(t_ch, FALSE) : move_regen(t_ch, FALSE) / 2,
                    t_ch->points.delay_move);
      }
    }
//      }
    PROFILE_END(combat);

    PROFILE_START(pulse_reset);
    // tics since last checkpoint signal
    if( ++tics > BIT_30 )
    {
      tics = 1;
      debug( "Huge value for tics, resetting to 1." );
      logit(LOG_SYS, "Huge value for tics, resetting to 1.");
    }
    pulse++;
    after_events_call = FALSE;
    if (pulse >= PULSES_IN_TICK)
    {
      pulse = 0;
      affect_update();
      point_update();
    }
    /* check out the time */
#if 1
    gettimeofday(&now, (struct timezone *) 0);
    timespent = timediff(&now, &last_time);     /* time used this pulse */
    timeout = timediff(&opt_time, &timespent);  /* time to sleep this pulse */

    if (timeout.tv_sec || timeout.tv_usec)
    {

      /*
       * This keeps game from being a total processor hog by putting
       * it to sleep for the part of each 1/4 second that is not
       * used for game processing.
       */

      sigprocmask(SIG_SETMASK, &mask, &oldset);

      if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0)
      {
        perror("Select sleep");
        continue;
      }
      sigprocmask(SIG_SETMASK, &oldset, 0);
    }
    gettimeofday(&last_time, (struct timezone *) 0);    /* end of pulse reset */
#endif
    PROFILE_END(pulse_reset);
  }

  PROFILES(SAVE);
#ifdef DO_PROFILE
  save_func_call_info();
#endif

  // Don't want to save stuff just after we wiped all the tables in SQL.
  if( !_pwipe )
  {
    if( no_ferries == 0 )
    {
      shutdown_ferries();
    }

    shutdown_ships();

    shutdown_auction_houses();

    Guildhall::shutdown();
  }

  for (point = descriptor_list; point; point = point->next)
  {
    if (point->character)
    {
      /* check for CON_PLAYING before extracting char. -DCL */
      if (point->connected == CON_PLAYING)
      {
        /* when you extract_char() a morph, it un_morph's first, which
           results in another save.  Unfortunatly, the save_silent(...3)
           has already nuked all the eq...  so.. just un_morph() them
           before the save_silent */
        if (IS_MORPH(point->character))
        {
          if (IS_FIGHTING(point->character))
            stop_fighting(point->character);
          if( IS_DESTROYING(point->character))
            stop_destroying(point->character);
          un_morph(point->character);
        }
        if( shutdown_message )
        {
          write_to_descriptor(point, shutdown_message);
        }
        if( !_pwipe )
        {
          write_to_descriptor(point, "\r\nSaving...\r\n");
          do_save_silent(point->character, 3);
        }
        // If it's not an immortal.
        if( GET_LEVEL(point->character) < MINLVLIMMORTAL )
        {
          update_ingame_racewar( -GET_RACEWAR(point->character) );
        }
        extract_char(point->character);
      }
    }
  }
}

/*
 * ****************************************************************** *
 * general utility stuff (for local use)
 *                                    *
 * ******************************************************************
 */

int get_from_q(struct txt_q *queue, char *dest)
{
  struct txt_block *tmp;

  /* hmm, could it be this simple? JAB */
  if (!queue)
  {
    logit(LOG_COMM, "call to get_from_q with NULL queue");
    return (0);
  }
  if (!dest)
  {
    logit(LOG_COMM, "call to get_from_q with bogus string");
    return (0);
  }
  /*
   * Q empty?
   */
  if (!queue->head)
    return (0);

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  queue->head = queue->head->next;

  FREE(tmp->text);
  FREE(tmp);

  return (1);
}

/*
 * flag: 0 - input queue, don't do any extra processing 1 - concat txt
 * onto preceding queue if possible
 * (trivial I know)
 */

void write_to_q(const char *txt, struct txt_q *queue, const int flag)
{
  struct txt_block *n_new;
  unsigned int txtlen, taillen;

  /* hmm, could it be this simple? JAB */
  if (!queue)
  {
    logit(LOG_COMM, "call to write_to_q with NULL queue");
    return;
  }
  if (!txt || ((txtlen = strlen(txt)) >= MAX_STRING_LENGTH))
  {
    logit(LOG_COMM, "call to write_to_q with bogus string");
    return;
  }
  /* Q empty? */
  if (!queue->head)
  {
    CREATE(n_new, txt_block, 1, MEM_TAG_TXTBLK);
    CREATE(n_new->text, char, txtlen + 1, MEM_TAG_BUFFER);

    strcpy(n_new->text, txt);

    n_new->next = NULL;
    queue->head = queue->tail = n_new;
    return;
  }

  taillen = strlen(queue->tail->text);

  /* something already in Q so try to combine if possible */
  if (flag && ((txtlen + taillen) < MAX_INPUT_LENGTH))
  {
    /* combine this text with preceding text */
    RECREATE(queue->tail->text, char, (txtlen + taillen + 1));

    strcat(queue->tail->text, txt);
    return;
  }
  /* nope, it needs to be sent, just add a queue entry */
  CREATE(n_new, txt_block, 1, MEM_TAG_TXTBLK);
  CREATE(n_new->text, char, txtlen + 1, MEM_TAG_BUFFER);

  strcpy(n_new->text, txt);

  queue->tail->next = n_new;
  queue->tail = n_new;
  n_new->next = NULL;
}

/*
 * if b > a returns 0 secs, 0 usecs
 */

struct timeval timediff(struct timeval *a, struct timeval *b)
{
  static struct timeval rslt;

  rslt.tv_sec = a->tv_sec - b->tv_sec;
  rslt.tv_usec = a->tv_usec - b->tv_usec;

  while (rslt.tv_usec < 0)
  {
    rslt.tv_usec += 1000000;
    rslt.tv_sec--;
  }

  while (rslt.tv_usec > 1000000)
  {
    rslt.tv_usec -= 1000000;
    rslt.tv_sec++;
  }

  if (rslt.tv_sec < 0)
  {
    rslt.tv_usec = 0;
    rslt.tv_sec = 0;
  }
  return (rslt);
}

/*
 * Empty the queues before closing connection
 */

void flush_queues(P_desc d)
{
  char     str[MAX_STRING_LENGTH];

  while (get_from_q(&d->output, str)) ;
  while (get_from_q(&d->input, str)) ;
}

int wizconnectsite(char *name, char *player, int flag)
{
  struct wizban_t *tmp;
  char     buf[MAX_INPUT_LENGTH];
  char     buff[MAX_INPUT_LENGTH];
  int      i;

  if (name == NULL)
    return FALSE;

  /*
   * lowercase the name string, since strstr is case sensitive
   */
  for (i = 0; *(name + i) != '\0'; i++)
    buf[i] = LOWER(*(name + i));
  buf[i] = 0;                   /*
                                 * to terminate buf
                                 */
  /*
   * lowercase the name string, since strstr is case sensitive
   */
  for (i = 0; *(player + i) != '\0'; i++)
    buff[i] = LOWER(*(player + i));
  buff[i] = 0;                  /*
                                 * to terminate buf
                                 */
  i = 1;
  for (tmp = wizconnect; tmp; tmp = tmp->next)
  {
    if (strstr(buff, tmp->name))
    {
      i = 0;
      switch (flag)
      {
      case 0:
        if (strstr(buf, tmp->ban_str))
          return TRUE;
        break;
      case 1:
        if (tmp->ban_str[0] == '*')
          if (strstr(buf, (tmp->ban_str + 1)))
            return TRUE;
        break;
      }
    }
  }
  return i;
}

int bannedsite(char *name, int flag)
{
  struct ban_t *tmp;
  char     buf[MAX_INPUT_LENGTH];
  int      i;

  if (name == NULL)
    return FALSE;

  /*
   * lowercase the name string, since strstr is case sensitive
   */
  for (i = 0; *(name + i) != '\0'; i++)
    buf[i] = LOWER(*(name + i));
  buf[i] = 0;                   /*
                                 * to terminate buf
                                 */

  for (tmp = ban_list; tmp; tmp = tmp->next)
  {
    switch (flag)
    {
    case 0:
      if (match_pattern(tmp->ban_str, buf))
      {
        return TRUE;
      }
      break;
    case 1:
      if (tmp->ban_str[0] == '*')
        if (match_pattern((tmp->ban_str + 1), buf))
        {
          return TRUE;
        }
      break;
    }
  }
  return FALSE;
}

/*
 * ****************************************************************** *
 * socket handling                                                    *
 * ******************************************************************
 */

#if 0                           /*
                                 * old socket routines.  JAB
                                 */

/*
 * old socket code used to be here... but I fucking yanked it forever.
 * (Neb/Io/Gary/Whatever)
 *
 * Leaving the old "#if 0" here for memory sake.  You know... people can
 * sit around and say to their grandchildren: "When I was your age, there
 * was code here... it didn't work worth a damn... but we kept it there
 * anyway.  Not sure why, though".
 */

#else /*
       * old/new socket code. JAB
       */

int init_socket(int port)
{
  int      s, bind_error;
  char    *opt;
  char     hostname[MAX_HOSTNAME + 1];
  struct sockaddr_in sa;
  struct hostent *hp;
  int      value = 1;
  struct linger linger_values;

/*
 * struct linger ld;
 */
  int      buffsize, buffer;

  linger_values.l_onoff = 0;
  linger_values.l_linger = 0;

  bzero(&sa, sizeof(struct sockaddr_in));
/*
  gethostname(hostname, MAX_HOSTNAME);
  hp = gethostbyname(hostname);
  if (hp == NULL) {
    logit(LOG_EXIT, "gethostbyname");
    exit(1);
  }
*/
/*  sa.sin_family = hp->h_addrtype; */
  sa.sin_family = AF_INET;
  sa.sin_port = htons((unsigned short int) port);
  s = socket(PF_INET, SOCK_STREAM, 0);
  if (s < 0)
  {
    logit(LOG_EXIT, "Init-socket");
    exit(1);
  }
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)
  {
    logit(LOG_EXIT, "setsockopt REUSEADDR");
    exit(1);
  }
  if (setsockopt
      (s, SOL_SOCKET, SO_LINGER, &linger_values, sizeof(linger_values)) < 0)
  {
    logit(LOG_EXIT, "setsockopt REUSEADDR");
    exit(1);
  }
  buffsize = sizeof(int);

  if (getsockopt
      (s, SOL_SOCKET, SO_SNDBUF, (char *) &buffer, (socklen_t *) & buffsize))
  {
    logit(LOG_EXIT, "getsockopt SNDBUF");
    exit(1);
  }
  if (buffer < MIN_SOCKET_BUFFER_SIZE)
  {
    buffer = MIN_SOCKET_BUFFER_SIZE;
    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &buffer, sizeof(buffer))
        < 0)
    {
      logit(LOG_EXIT, "setsockopt SNDBUF");
      exit(1);
    }
  }
  if ((bind_error = (bind(s, (struct sockaddr *) &sa, sizeof(sa))) < 0))
  {
    logit(LOG_EXIT, "bind error %d", bind_error);
    close(s);
    exit(1);
  }
  listen(s, 40);                /*
                                 * if new connects still lockup, raise the
                                 * 50.  JAB
                                 */
  return (s);
}

int new_connection(int s)
{
  struct sockaddr_in isa;
  int      i;
  int      t;

  i = sizeof(isa);
  getsockname(s, (struct sockaddr *) &isa, (socklen_t *) & i);

  if ((t = accept(s, (struct sockaddr *) &isa, (socklen_t *) & i)) < 0)
  {
    perror("Accept");
    return (-1);
  }
  nonblock(t);

  return (t);
}

void close_sockets(int s)
{
  logit(LOG_STATUS, "Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);
  close(s);
}

void close_socket(struct descriptor_data *d)
{
  struct descriptor_data *tmp;
  snoop_by_data *snoop_by_ptr, *next;
  int      is_morphed = IS_MORPH(d->character);
  char     Gbuf1[MAX_STRING_LENGTH];
  time_t   ct;

  compress_end(d, TRUE);        /* does flushing out all output break anything ? */
  if (d->sslses)
    ssl_close(d->sslses);
  if (d->descriptor)
    close(d->descriptor);
  flush_queues(d);
  --used_descs;

  /* Forget snooping */
/*
  if (d->snoop.snoop_by) {
    send_to_char("Your victim is no longer among us.\r\n", d->snoop.snoop_by);
    d->snoop.snoop_by->desc->snoop.snooping = 0;
  }
*/
  snoop_by_ptr = d->snoop.snoop_by_list;
  while (snoop_by_ptr)
  {
    if (is_morphed && affected_by_spell(d->character, SPELL_CHANNEL))
      send_to_char
        ("Your host has lost link... you can no longer maintain the sight link.\r\n",
         snoop_by_ptr->snoop_by);
    else
      send_to_char("Your victim is no longer among us.\r\n", snoop_by_ptr->snoop_by);
    snoop_by_ptr->snoop_by->desc->snoop.snooping = 0;

    next = snoop_by_ptr->next;
    FREE(snoop_by_ptr);

    snoop_by_ptr = next;
  }

  d->snoop.snoop_by_list = 0;

  if (is_morphed && affected_by_spell(d->character, SPELL_CHANNEL))
    un_morph(d->character);


  if (d->snoop.snooping)
  {
    /*
     * if !d->character, or they aren't playing, I can't get their
     * level.. so I'll assume its better then 58 to be safe
     */
    is_morphed = IS_MORPH(d->snoop.snooping);

    if (d->character && (d->connected == CON_PLAYING) &&
        (GET_LEVEL(d->character) < 58))
      send_to_char("&+CYou are no longer being snooped.&N\r\n",
                   d->snoop.snooping);
/*    d->snoop.snooping->desc->snoop.snoop_by = 0;*/
    if (is_morphed)
    {
      act
        ("&+B$n has lost $s link and is unable to maintain $s part of the spell!&n",
         FALSE, d->character, 0, d->snoop.snooping, TO_VICT);
      un_morph(d->snoop.snooping);
    }
    if (d->snoop.snooping)
    {
      rem_char_from_snoopby_list(&d->snoop.snooping->desc->snoop.
                                 snoop_by_list, d->character);
      d->snoop.snooping = 0;
    }
  }
/*  if (d->olc) {
    olc_end(d->olc);
  }*/
  if (d->str && (*d->str))
  {
    FREE(*d->str);
    if ((d->character) && (d->character->player.description == *d->str))
      /*
       * okay... we have a fun situation here.  They just lost link
       * while entering their description.  Before, the code would
       * try to free() this piece of memory twice.  Once, when doing
       * the free_char() call, and secondly when free()ing *d->str.
       * The proper thing to do is to set their description to NULL
       * (its not entered in yet anyway), and let the memory be
       * freed with *d->str  (neb)
       */
      d->character->player.description = NULL;
  }
  /* Okay, above sounds fine and dandy, but this is the real world,
     and it's named duris. Shit happens. I want d->str cleared
     God Damn it! So, I'll set it to null as well. It should already
     be caught, but just in case, I'd rather leave a few bytes of
     wasted memory lying around, than a potential bomb.
   */
  if (d->str)
    *d->str = NULL;
  d->str = NULL;
  d->backstr = NULL;
  /* Gee, that wasn't so tough now, was it? */

  if (d->character)
  {
    if (d->connected == CON_PLAYING)
    {
      sql_disconnectIP(d->character);
      act("$n has lost $s link.", TRUE, GET_PLYR(d->character), 0, 0,
          TO_ROOM);
      if ((NumAttackers(d->character) > 0) && !IS_TRUSTED(d->character))
      {
        logit(LOG_COMM, "Combat DropLink: %s [%s].",
              GET_NAME(GET_PLYR(d->character)), d->host);
        statuslog(56, "Combat DropLink: %s [%s].",
                  GET_NAME(GET_PLYR(d->character)), d->host);
      }
      else
      {
        logit(LOG_COMM, "Closing link to: %s [%s].", GET_NAME(GET_PLYR(d->character)), d->host);
        // Subtract 5 hrs: GMT -> EST.
        ct = time(0) - 5*60*60;
        snprintf(Gbuf1, MAX_STRING_LENGTH, "%s", asctime( localtime(&ct) ));
        *(Gbuf1 + strlen(Gbuf1) - 1) = '\0';
        loginlog(d->character->player.level, "%s [%s] has lost link @ %s EST.",
                 GET_NAME(GET_PLYR(d->character)), d->host, Gbuf1 );
        sql_log(d->character, CONNECTLOG, "Lost Link");
      }
      writeCharacter(d->character, RENT_CRASH, d->character->in_room);
      d->character->desc = 0;
    }
    else
    {
      logit(LOG_COMM, "Losing player: %s [%s].", GET_NAME(d->character),
            d->host);
      free_char(d->character);
      d->character = NULL;
    }
  }
  else
    logit(LOG_COMM, "Losing descriptor without char.");

  if (next_to_process == d)
    next_to_process = next_to_process->next;
  if (d == descriptor_list)
    descriptor_list = descriptor_list->next;
  else
  {
    /*
     * Locate the previous element
     */
    for (tmp = descriptor_list; tmp && (tmp->next != d); tmp = tmp->next) ;
    tmp->next = d->next;
  }

  if (d->descriptor)
    shutdown(d->descriptor, 2);

  if (d->showstr_head)
  {
    FREE(d->showstr_head);
  }
#   ifdef I_REALLY_WANT_TO_CRASH_THE_GAME
  if (d->showstr_point)
  {
#      ifdef MEM_DEBUG
    mem_use[MEM_STRINGS] -= strlen(d->showstr_point);
#      endif
    FREE(d->showstr_point);
  }

  if (d->showstr_count)
#      ifdef MEM_DEBUG
    mem_use[MEM_STRINGS] -= strlen(d->showstr_vector);
#      endif
  FREE(d->showstr_vector);

  if (d->storage)
    FREE(d->storage);

#   endif
       /* I really don't wanna crash it  */
#   ifdef USE_ACCOUNT
  if (d->account)
    free_account(d->account);
#   endif
  if (d)
  {
#   if 0
#      ifdef MEM_DEBUG
    mem_use[MEM_DESC] -= sizeof(struct descriptor_data);
#      endif
    FREE((char *) d);
#   endif
    mm_release(dead_desc_pool, d);
  }
}

void nonblock(int s)
{
  int      flags;

  flags = fcntl(s, F_GETFL);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0)
  {
    logit(LOG_EXIT, "Nonblock");
    exit(1);
  }
}

#endif /*
        * old/new socket code. 9/18/95  JAB
        */

int new_descriptor(int s, bool ssl)
{
  P_desc   newd;
  bool     flag = FALSE, found = FALSE, looking_up = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];
  int      desc, size;
  struct sockaddr_in sock;
  FILE    *f;
  gnutls_session_t sslses = 0;

  if ((desc = new_connection(s)) < 0)
    return (-1);

  if (ssl && !(sslses = ssl_new(desc)))
    return 0; // can legitimately fail if client sends garbage

  used_descs++;

  if (used_descs >= avail_descs)
  {
    // shouldn't write anything before setup
    // write(desc, "Sorry, the game is full...\r\n");
    used_descs--;
    shutdown(desc, 2);
    close(desc);
    return (0);
  }
  else if (used_descs > max_descs)
    max_descs = used_descs;

#if 0
#   ifdef MEM_DEBUG
  mem_use[MEM_DESC] += sizeof(struct descriptor_data);
#   endif
  CREATE(newd, struct descriptor_data, 1);
#endif
  newd = (struct descriptor_data *) mm_get(dead_desc_pool);
  bzero(newd, sizeof(struct descriptor_data));

  /*
   * find info
   */
  size = sizeof(sock);

  if (getpeername(desc, (struct sockaddr *) &sock, (socklen_t *) & size) < 0)
  {
    perror("getpeername");
    strcpy(Gbuf1, "&+RUNTRACEABLE&n");
    flag = TRUE;
  }
  else
  {

    snprintf(Gbuf1, MAX_STRING_LENGTH, "%d.%d.%d.%d",
            ((unsigned char *) &(sock.sin_addr))[0],
            ((unsigned char *) &(sock.sin_addr))[1],
            ((unsigned char *) &(sock.sin_addr))[2],
            ((unsigned char *) &(sock.sin_addr))[3]);

#if 0
    if (strlen(Gbuf1) < 8)
    {
      /*
       * address is garbage, bounce em
       */
      if (bounce_null_sites)
      {
        // WTF?  This is a set of valid addresses!
        write(desc, "Your site name is unparseable!\r\n");
        logit(LOG_COMM, "Null site name bounced.");
        shutdown(desc, 2);
        used_descs--;
        close(desc);
#if 0
#   ifdef MEM_DEBUG
        mem_use[MEM_DESC] -= sizeof(struct descriptor_data);
#   endif
        FREE((char *) newd);
#endif
        mm_release(dead_desc_pool, newd);
        return (0);
      }
      else
      {
        strcpy(Gbuf1, "&+RUNTRACEABLE&n");
        flag = TRUE;
      }
    }
#endif
    /*
     * things got ugly, 20k+ sites, so, split it into 2 files, a
     * sorted historical one and an unsorted 'recent' one.  Rather
     * than code in sorting routines, from time to time we combine the
     * two, and resort (by hand, ie. 'sort').  Yes, this is an awful
     * kludge, but, until we imp an asynch gethostbyaddr, it will have
     * to do.  JAB
     */

    /*
     * first, we do a binary search of the sorted file.
     */
/*
    if (!flag) {
      char *t;
      t = dnsdb_find(Gbuf1);
      if (t) {
        found = TRUE;
        strcpy(Gbuf3, t);
      }
    }
*/
    if (!flag && !found)
    {
      /*
       * well didn't have it on file, so have to bite the bullet and
       * look it up.  Fortunately, only have to do this once per
       * site, ever.
       */

      /*
       * okay.. need to look this up...
       */
      struct host_request buf;

      buf.mtype = MSG_HOST_REQ;
      buf.desc = desc;
      snprintf(buf.addr, MAX_STRING_LENGTH, "%d.%d.%d.%d",
              ((unsigned char *) &(sock.sin_addr))[0],
              ((unsigned char *) &(sock.sin_addr))[1],
              ((unsigned char *) &(sock.sin_addr))[2],
              ((unsigned char *) &(sock.sin_addr))[3]);


#if 0
      if (msgsnd(ipc_id, (struct msgbuf *) &buf,
                 sizeof(struct host_request) - sizeof(long), 0) == -1)
      {
        /*
         * msgsnd() failed... DAMN!  for now, just segfault
         */
        raise(SIGSEGV);;
      }
      /*
       * I'll use yellow to indicate the address is being looked up
       */
#endif

      snprintf(Gbuf1, MAX_STRING_LENGTH, "%s", buf.addr);
      looking_up = TRUE;
    }
    if (found)
      strcpy(Gbuf1, Gbuf3);
  }

  /*
   * okay.. we simulate it looking up even if doesn't need to.  This
   * way, all the bansite code, etc, can be put in the same place.
   */

  if (!looking_up)
  {                             /* simulate it anyway */
    struct host_answer buf;

    buf.mtype = MSG_HOST_ANS;
    buf.desc = desc;
    /*
     * note that I put in a "." in the front of the addr field. That
     * will signal the "reciver" that this name already occurs in the
     * lookup list.
     */
    snprintf(buf.addr, MAX_STRING_LENGTH, ".%d.%d.%d.%d",
            ((unsigned char *) &(sock.sin_addr))[0],
            ((unsigned char *) &(sock.sin_addr))[1],
            ((unsigned char *) &(sock.sin_addr))[2],
            ((unsigned char *) &(sock.sin_addr))[3]);

    strcpy(buf.name, Gbuf1);
#if 0
    if (msgsnd(ipc_id, (struct msgbuf *) &buf,
               sizeof(struct host_answer) - sizeof(long), 0) == -1)
    {
      /*
       * msgsnd() failed... DAMN!  for now, just segfault
       */
      raise(SIGSEGV);;
    }
#endif
    /*
     * I'll use yellow to indicate the address is being looked up
     */

    snprintf(Gbuf1, MAX_STRING_LENGTH, "%s", buf.addr);
  }
//  if (!found)
//    write_to_descriptor(desc, "Looking up your hostname...\r\n");
  /*
   * init desc data
   */
  newd->descriptor = desc;
  //newd->connected = CON_HOST_LOOKUP;
  newd->wait = 1;
  strncpy(newd->host, Gbuf1, 50);
  snprintf(Gbuf1, MAX_STRING_LENGTH,
          "host %s | sed -e 's/.*pointer\\ \\(.*\\)\\./\\1/g;t;d' > lib/etc/hosts/%d &",
          strip_ansi(newd->host).c_str(), desc);
  system(Gbuf1);
  *newd->host2 = '\0';
  newd->prompt_mode = FALSE;
  *newd->buf = '\0';
  newd->str = 0;
  newd->showstr_head = 0;
  newd->showstr_vector = 0;
  newd->showstr_count = 0;
  *newd->last_input = '\0';
  newd->output.head = NULL;
  newd->input.head = NULL;
  newd->next = descriptor_list;
  newd->character = 0;
  newd->original = 0;
  newd->snoop.snooping = 0;
  newd->snoop.snoop_by_list = 0;
  newd->tmp_val = 0;            /*
                                 * SAM 7-94
                                 */
  newd->confirm_state = 0;      /*
                                 * SAM 7-94
                                 */
  strcpy(newd->login, " ? ");
  newd->editor = NULL;
  newd->olc = NULL;
  newd->out_compress = MCCP_NONE;
  newd->z_str = NULL;
  newd->sslses = sslses;
  *newd->client_str = '\0';
  newd->term_type = TERM_ANSI;
  descriptor_list = newd;

  if (ssl && ssl_negotiate(sslses)) // do first round immediately
    STATE(newd) = CON_SSLNEGO;
  else
    greet(newd);

  return 0;
}

static void greet(P_desc newd)
{
  if (bannedsite(newd->host, 0))
  {
    write_to_descriptor(newd,
                        "Your site has been banned from being able to connect to Duris.\r\n"
                        "You were banned because someone at your site has flagrantly violated\r\n"
                        "the rules to a point where banning your site was necessary.  If you\r\n"
                        "feel this is in error, please e-mail multiplay@durismud.com\r\n");
    banlog(56, "Reject Connect from %s, banned site.", newd->host);
    logit(LOG_STATUS, "Rejected Connect from %s, banned site.", newd->host);
    STATE(newd) = CON_EXIT;
    //flush_queues(newd);
    return;
  }

  select_terminal(newd, "");

  advertise_mccp(newd);
}


/*
 * Return index into color_table. (Ithor) **  Did not use toupper,
 * because it tends to screw up on some machines. (mine)
 */

int find_color_entry(int c)
{
  int      i = 0;
  char     s;

  s = UPPER(c);

  while ((color_table[i].symbol != NULL) && (*color_table[i].symbol != s))
    i++;

  return i;
}

void append_prompt(P_char ch ,char *promptbuf)
{
  char t_buf[512];
  P_char t_ch_f;
  P_obj t_obj_f;
  P_char tank;
  int percent = 0;


  if(!ch)
    return;

  if(!IS_TRUSTED(ch) && (ch->desc->connected == CON_PLAYING || ch->desc->connected == CON_MAIN_MENU))
  {
    ;
  }
  else
  {
    return;
  }

  if (ch)
  {
    t_ch_f = GET_OPPONENT(ch);
    t_obj_f = ch->specials.destroying_obj;
  }

  if(IS_NPC(ch))
    return;

  strcat(promptbuf, "\n&+g<");
  if (GET_MAX_HIT(ch) > 0)
    percent = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
  else
    percent = -1;

  if (percent >= 66)
  {
    snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g %dh",
        ch->points.hit);
  }
  else if (percent >= 33)
  {
    snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+y %dh",
        ch->points.hit);
  }
  else if (percent >= 15)
  {
    snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+r %dh",
        ch->points.hit);
  }
  else
  {
    snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+R %dh",
        ch->points.hit);
  }
  snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g/%dH",
      GET_MAX_HIT(ch));

  if (GET_MAX_VITALITY(ch) > 0)
  {
    percent = (100 * GET_VITALITY(ch)) / GET_MAX_VITALITY(ch);
  }
  else
  {
    percent = -1;
  }

  if (percent >= 66)
  {
    snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g %dv",
        ch->points.vitality);
  }
  else if (percent >= 33)
  {
    snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+y %dv",
        ch->points.vitality);
  }
  else
  {
    snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+r %dv",
        ch->points.vitality);
  }
  snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g/%dV",
      GET_MAX_VITALITY(ch));

  strcat(promptbuf, " &+CPos:&+g");
  if (GET_POS(ch) == POS_STANDING)
    strcat(promptbuf, " standing");
  else if (GET_POS(ch) == POS_SITTING)
    strcat(promptbuf, " sitting");
  else if (GET_POS(ch) == POS_KNEELING)
    strcat(promptbuf, " kneeling");
  else if (GET_POS(ch) == POS_PRONE)
    strcat(promptbuf, " on your ass");
  strcat(promptbuf, " &+g>&n\n");


  if (t_ch_f && (ch->in_room == t_ch_f->in_room))
  {
    strcat(promptbuf, "&+g<");

    /* TANK elements only active if... */
    if( (tank = GET_OPPONENT(t_ch_f)) && (ch->in_room == tank->in_room) )
    {
      snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), " &+BT: %s", (ch != tank && !CAN_SEE(ch, tank)) ? "someone"
        : (IS_PC(tank) ? PERS(tank, ch, 0) : ( FirstWord(GET_NAME( tank )) )) );
      strcat(promptbuf, " &+CTP:&+g");
      if (GET_POS(tank) == POS_STANDING)
        strcat(promptbuf, " sta");
      else if (GET_POS(tank) == POS_SITTING)
        strcat(promptbuf, " sit");
      else if (GET_POS(tank) == POS_KNEELING)
        strcat(promptbuf, " kne");
      else if (GET_POS(tank) == POS_PRONE)
        strcat(promptbuf, " ass");

      strcat(promptbuf, " &+cTC:");
      if (GET_MAX_HIT(tank) > 0)
      {
        percent = (100 * GET_HIT(tank)) / GET_MAX_HIT(tank);
      }
      else
      {
        percent = -1;
      }
      if (percent >= 100)
      {
        strcat(promptbuf, "&+gexcellent");
      }
      else if (percent >= 90)
      {
        strcat(promptbuf, "&+Yfew scratches");
      }
      else if (percent >= 75)
      {
        strcat(promptbuf, "&+Y small wounds");
      }
      else if (percent >= 50)
      {
        strcat(promptbuf, "&+M few wounds");
      }
      else if (percent >= 30)
      {
        strcat(promptbuf, "&+m nasty wounds");
      }
      else if (percent >= 15)
      {
        strcat(promptbuf, "&+Rpretty hurt");
      }
      else if (percent >= 0)
      {
        strcat(promptbuf, "&+r awful");
      }
      else
      {
        strcat(promptbuf, "&+r bleeding, close to death");
      }

      snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), " &+rE: %s&+g",
          (!CAN_SEE(ch, t_ch_f)) ? "someone" : (IS_PC(t_ch_f)
                                                ? PERS(t_ch_f, ch, 0)
                                                : (FirstWord
                                                  ((t_ch_f)->player.
                                                   name))));
      if (GET_POS(t_ch_f) == POS_STANDING)
        strcat(promptbuf, " sta");
      else if (GET_POS(t_ch_f) == POS_SITTING)
        strcat(promptbuf, " sit");
      else if (GET_POS(t_ch_f) == POS_KNEELING)
        strcat(promptbuf, " kne");
      else if (GET_POS(t_ch_f) == POS_PRONE)
        strcat(promptbuf, " ass");

      strcat(promptbuf, "&+C EP: ");
      if (GET_MAX_HIT(t_ch_f) > 0)
      {
        percent = (100 * GET_HIT(t_ch_f)) / GET_MAX_HIT(t_ch_f);
      }
      else
      {
        percent = -1;
      }
      if (percent >= 100)
      {
        strcat(promptbuf, "&+gexcellent");
      }
      else if (percent >= 90)
      {
        strcat(promptbuf, "&+Yfew scratches");
      }
      else if (percent >= 75)
      {
        strcat(promptbuf, "&+Y small wounds");
      }
      else if (percent >= 50)
      {
        strcat(promptbuf, "&+M few wounds");
      }
      else if (percent >= 30)
      {
        strcat(promptbuf, "&+m nasty wounds");
      }
      else if (percent >= 15)
      {
        strcat(promptbuf, "&+Rpretty hurt");
      }
      else if (percent >= 0)
      {
        strcat(promptbuf, "&+r awful");
      }
      else
      {
        strcat(promptbuf, "&+r bleeding, close to death");
      }
      strcat(promptbuf, " &+g>&n\n ");

    }
  }
  if ( t_obj_f )
  {
    strcat(promptbuf, "&+g< &+rE: " );
    strcat(promptbuf, t_obj_f->short_description );
    strcat(promptbuf, "&+g >&n\n" );
  }
}

void write_to_pc_log(P_char ch, const char *message, int log)
{
  if( !ch )
    return;

  ch = GET_PLYR(ch);

  if( !ch || !IS_PC(ch) || !IS_ALIVE(ch) )
  {
    return;
  }
  
  if( !GET_PLAYER_LOG(ch) )
  {
    initialize_logs(ch, false);
  }
  
  if( !GET_PLAYER_LOG(ch) )
  {
    logit(LOG_DEBUG, "Reloaded player log (%s) in write_to_pc_log(), but still not loaded.", GET_NAME(ch));
    debug("Reloaded player log (%s) in write_to_pc_log(), but still not loaded.", GET_NAME(ch));
    return;
  }
  
  if( log < 0 || log >= NUM_LOGS )
  {
    logit(LOG_DEBUG, "Invalid log (%d) in write_to_pc_log()", GET_NAME(ch));
    debug("Invalid log (%d) in write_to_pc_log()", GET_NAME(ch));
    return;
  }

  GET_PLAYER_LOG(ch)->write(log, message);  
}

void initialize_logs(P_char ch, bool reset_logs)
{
  if( !ch || !IS_PC(ch) )
    return;

  if( !reset_logs && GET_PLAYER_LOG(ch) )
  {
    logit(LOG_DEBUG, "Tried to initialize player log (%s) in initialize_logs(), but was not null!", GET_NAME(ch));
    return;
  }
  
  if( reset_logs )
  {
    clear_logs(ch);    
  }

  GET_PLAYER_LOG(ch) = new PlayerLog;  
}

void clear_logs(P_char ch)
{
  if( !ch || !IS_PC(ch) )
    return;

  if( !GET_PLAYER_LOG(ch) )
  {
//    logit(LOG_DEBUG, "Tried to clear player log (%s) in clear_logs(), but was null!", GET_NAME(ch));
    return;
  }
  
  GET_PLAYER_LOG(ch)->clear();
}

/*
 * **  Combine multiple entries in the output queue to go to the same file
 * **  descriptor. (Max of 2 * MAX_STRING_LENGTH (16K)) **  Also go through
 * the strings adding color codes when appropriate, and **  striping the
 * special symbols when needed.
 */

int process_output(P_desc t)
{
  char     buf[MAX_STRING_LENGTH + 1], buffer[MAX_STRING_LENGTH + 1];
  char     buf2[MAX_STRING_LENGTH];
  bool     flg, bold = FALSE, blink = FALSE;
  int      ibuf = 0;
  int      i, j, k, bg = 0;
  snoop_by_data *snoop_by_ptr;
  P_char   realChar = t->original ? t->original : t->character;

  if( STATE(t) == CON_PLAYING && IS_PC(realChar)
    && ((t->prompt_mode == (PLR_FLAGGED(realChar, PLR_SMARTPROMPT))
    || (t->prompt_mode != PLR_FLAGGED(realChar, PLR_OLDSMARTP)))) )
  {
    if( !t->snoop.snooping || !t->snoop.snooping->desc || !t->snoop.snooping->desc->prompt_mode)
    if( write_to_descriptor(t, "\r\n") < 0 )
    {
      return (-1);
    }
  }
//  if( realChar && IS_ANSI_TERM(t) && GET_LEVEL(
 // t->character) >= 1 && (STATE(t) != CON_TEXTED) )   arih: why remove color? its ugly!

  if( IS_ANSI_TERM(t) && (STATE(t) != CON_TEXTED) &&
      (!realChar || (t->character && GET_LEVEL(t->character) >= 1)))
  {
    flg = TRUE;
  }
  else
  {
    flg = FALSE;
  }

  /* Cycle thru output queue */
  while( get_from_q(&t->output, buf) )
  {
#if 0
    if( PLR_FLAGGED(realChar, PLR_SMARTPROMPT) )
      format_text(buf, 1, t, MAX_STRING_LENGTH);
#endif

    if( (snoop_by_ptr = t->snoop.snoop_by_list) != NULL )
    {
      format_to_snoopers( buf, buf2 );
    }
    while( snoop_by_ptr )
/*    if (t->snoop.snoop_by) {*/
    {

      /* desc check makes snoop go wacky?  one never knows.. */
//      if (snoop_by_ptr->snoop_by->desc)
//      {
          write_to_q(buf2, &snoop_by_ptr->snoop_by->desc->output, 1);
//      }

      snoop_by_ptr = snoop_by_ptr->next;
    }

    ibuf = strlen(buf);
    /* Go through and convert/strip color symbols -Ithor */
    for( i = 0, j = 0; (i < ibuf) && (j < (sizeof(buffer))); i++ )
    {
      if (buf[i] == '&')
      {
        i++;
        if (i >= ibuf)
          continue;

        switch (buf[i])
        {
        case '&':
          buffer[j++] = '&';
          break;
        case 'N':
        case 'n':
          if (flg)
          {
            snprintf(&buffer[j], MAX_STRING_LENGTH, "\033[0m");
            j += 4;
          }
          was_upper = FALSE;
          break;
        case 'L':
          if (flg)
          {
            snprintf(&buffer[j], MAX_STRING_LENGTH, "\r\n");
            j += 2;
          }
          break;
        case '+':
        case '-':
          bg = (buf[i] == '-');
          i++;
          if (i >= ibuf)
            continue;

          bold = bg ? 0 : (isupper(buf[i])) ? 1 : 0;
          blink = !bg ? 0 : (isupper(buf[i])) ? 1 : 0;
          k = find_color_entry(buf[i]);
          if (color_table[k].symbol != NULL)
          {
            if (flg)
            {
              if (isupper(buf[i]))
                was_upper = TRUE;
              else if (was_upper)
              {
                snprintf(&buffer[j], MAX_STRING_LENGTH, "\033[0m");
                j += 4;
                was_upper = FALSE;
              }
              snprintf(&buffer[j], MAX_STRING_LENGTH, "\033[%s%s%sm", bold ? "1;" : "",
                      blink ? (t->character && (PLR3_FLAGGED(t->character, PLR3_UNDERLINE)) ? "4;" : "5;") : "",
                      (bg ? color_table[k].bg_code : color_table[k].fg_code));
              j += (5 + (bold ? 2 : 0) + (blink ? 2 : 0));
            }
          }
          else
          {
            snprintf(&buffer[j], MAX_STRING_LENGTH, "&%c%c", (bg ? '-' : '+'), buf[i]);
            j += 3;
          }
          break;

        case '=':
          i++;
          if (i >= ibuf)
            continue;

          blink = (isupper(buf[i]) ? 1 : 0);
          bg = find_color_entry(buf[i]);
          i++;
          if (i >= ibuf)
            continue;

          bold = (isupper(buf[i]) ? 1 : 0);
          k = find_color_entry(buf[i]);
          if ((color_table[k].symbol != NULL)
              && (color_table[bg].symbol != NULL))
          {
            if (flg)
            {
              if (isupper(buf[i]))
                was_upper = TRUE;
              else if (was_upper)
              {
                snprintf(&buffer[j], MAX_STRING_LENGTH, "\033[0m");
                j += 4;
                was_upper = FALSE;
              }
              snprintf(&buffer[j], MAX_STRING_LENGTH, "\033[%s%s%s;%sm", bold ? "1;" : "",
                      blink ? (t->character && (PLR3_FLAGGED(t->character, PLR3_UNDERLINE)) ? "4;" : "5;") : "",
                      color_table[bg].bg_code, color_table[k].fg_code);
              j += (8 + (bold ? 2 : 0) + (blink ? 2 : 0));
            }
          }
          else
          {
            snprintf(&buffer[j], MAX_STRING_LENGTH, "&=%c%c", buf[i - 1], buf[i]);
            j += 4;
          }
          break;

        default:
          snprintf(&buffer[j], MAX_STRING_LENGTH, "&%c", buf[i]);
          j += 2;
          break;
        }
      }
      else if (flg && (buf[i] == '\n'))
      {
        /* Want normal color at EoLN */
        snprintf(&buffer[j], MAX_STRING_LENGTH, "\033[0m\n");
        j += 5;
      }
      else
      {
        buffer[j++] = buf[i];
      }
    }

    buffer[j] = '\0';


    if (write_to_descriptor(t, buffer) < 0)
      return (-1);
  }

  if (!t->connected && t->character && !t->olc &&
      (IS_PC(t->character) || IS_MORPH(t->character)) &&
      !IS_SET(GET_PLYR(t->character)->specials.act, PLR_COMPACT))
  {
    if (write_to_descriptor(t, "\r\n") < 0)
    {
      return (-1);
    }
  }
  return (1);
}

/*
 * this routine takes raw input from a socket (t->buf) and breaks it up
 * and massages and filters it before writing it to the input queue
 * (t->input) for actual parsing by the mud.  ALL input from sockets must
 * pass through this routine.
 */

int process_input(P_desc t)
{
  int      sofar, thisround, begin, squelch, i, k, flag;
  char     tmp[MAX_INPUT_LENGTH + 3], buffer[MAX_INPUT_LENGTH + 60];
  snoop_by_data *snoop_by_ptr;

  sofar = 0;
  flag = 0;
  begin = strlen(t->buf);

  /*
   * Read in some stuff
   */
  if (t->sslses)
  {
    thisround = gnutls_record_recv(t->sslses, t->buf + begin, 
      (MAX_QUEUE_LENGTH - begin));
    if (!thisround)
    {
      logit(LOG_COMM, "EOF encountered on socket read for %s.",
        (t->character) ? GET_NAME(t->character) : "NOCHAR");
      return (-1);
    }
    else if (thisround > 0)
      sofar += thisround;
    else if (thisround != GNUTLS_E_AGAIN && thisround != GNUTLS_E_INTERRUPTED)
    {
      logit(LOG_COMM, "process_input() CON_%d %s Read: %d Error: %s",
            t->connected, (t->character) ? GET_NAME(t->character) : "",
            thisround, gnutls_strerror(thisround));
      return (-1);
    }
  }
  else do
  {
    if ((thisround = read(t->descriptor, (t->buf + begin + sofar),
                          (unsigned) (MAX_QUEUE_LENGTH - begin - sofar -
                                      1))) > 0)
      sofar += thisround;
    else if (thisround < 0)
      if (errno !=
#ifdef _HPUX_SOURCE
          EAGAIN
#else
          EWOULDBLOCK
#endif
        )
      {
        logit(LOG_COMM, "process_input() CON_%d %s Read: %d Error: %d",
              t->connected, (t->character) ? GET_NAME(t->character) : "",
              thisround, errno);
        return (-1);
      }
      else
        break;
    else
    {
      logit(LOG_COMM, "EOF encountered on socket read.");
      return (-1);
    }
  }

  while (!ISNEWL(*(t->buf + begin + sofar - 1)));

  *(t->buf + begin + sofar) = 0;

  /*
   * scan input stream for a newline, if one isn't found, command is not
   * yet ready for processing, so do not xfer it to input queue, and
   * return 0, so that this socket is skipped.
   */

  for (i = begin; !ISNEWL(*(t->buf + i)); i++)
    if (!*(t->buf + i))
      return (0);

#ifdef SMART_PROMPT
  if (t->character && IS_SET(t->character->specials.act, PLR_SMARTPROMPT))
    t->prompt_mode = TRUE;
#endif

  /*
   * input contains 1 or more newlines; process the stuff
   */

  for (i = 0, k = 0; *(t->buf + i);)
  {
    if (!ISNEWL(*(t->buf + i)) && !(flag = (k >= (MAX_INPUT_LENGTH - 2))))
    {
      /* telnet option ? */
      if (*(t->buf + i) == (signed char) IAC)
        i += parse_telnet_options(t, t->buf + i);
      /* backspace? */
      else if (*(t->buf + i) == '\b')
      {
        /* more than one char ? */
        if (k)
        {
          if (*(tmp + --k) == '$')
            k--;
          i++;
        }
        else
        {
          /* no chars to delete, so just skip the backspace */
          i++;
        }
      }
      else
      {
        if (*(t->buf + i) == '$')
        {
          /*
           * a '$'?  if so, have to double it, so that act()
           * won't choke on it later on.
           */
          *(tmp + k) = '$';
          k++;
        }
        /* printable character? */
        if (isascii(*(t->buf + i)) && isprint(*(t->buf + i)))
        {
          *(tmp + k) = *(t->buf + i);
          i++;
          k++;
        }
        else
        {
          /* garbage character, skip it */
          i++;
        }
      }
    }
    else
    {
      /* newline or input too long, have to actually DO something with it. */
      *(tmp + k) = 0;

      /*
       * bah! this was in wrong spot, wouldn't catch it until after
       * it had hosed memory by overwriting some huge ass string to
       * a little dinky char array.  JAB
       */

      if (k > (MAX_INPUT_LENGTH - 1))
      {
        k = MAX_INPUT_LENGTH - 1;
        *(tmp + k) = 0;
        snprintf(buffer, MAX_STRING_LENGTH, "Line too long. Truncated to:\r\n%s\r\n", tmp);
        if (write_to_descriptor(t, buffer) < 0)
          return (-1);

        /* skip the rest of the line */
        for( ; *(t->buf + i) && !ISNEWL(*(t->buf + i)); i++ )
          ;
      }
      /* handle '!' to repeat last command */
      if ((*tmp != '!') || !t->last_input || !t->character || t->connected)
        strcpy(t->last_input, tmp);
      else
        strcpy(tmp, t->last_input);

      if(t)
        if(t->character){
          t->character->only.pc->recived_data = t->character->only.pc->recived_data + strlen(tmp);
          recivedbytes = recivedbytes + strlen(tmp);
        }
      write_to_q(tmp, &t->input, 0);

      snoop_by_ptr = t->snoop.snoop_by_list;

/*
      if (t->snoop.snoop_by) {
*/
      while (snoop_by_ptr)
      {
        write_to_q("&+y%&n ", &snoop_by_ptr->snoop_by->desc->output, 1);
        write_to_q(tmp, &snoop_by_ptr->snoop_by->desc->output, 1);
        write_to_q("\r\n", &snoop_by_ptr->snoop_by->desc->output, 1);

        snoop_by_ptr = snoop_by_ptr->next;
      }

      /* find end of entry */
      for (; ISNEWL(*(t->buf + i)); i++) ;

      /* squelch the entry from the buffer */
      for (squelch = 0;; squelch++)
        if ((*(t->buf + squelch) = *(t->buf + i + squelch)) == '\0')
          break;
      k = 0;
      i = 0;
    }
  }
  return (1);
}

/*
 * **************************************************************** *
 * Public routines for system-to-player-communication        *
 * ****************************************************************
 */

static char send_to_char_f_buf[MAX_STRING_LENGTH];
void send_to_char_f(P_char ch, const char *fmt, ... )
{
  va_list args;

  va_start(args, fmt);
  vsnprintf(send_to_char_f_buf, sizeof(send_to_char_f_buf) - 1, fmt, args);
  va_end(args);

  send_to_char(send_to_char_f_buf, ch);
}

void send_to_char(const char *messg, P_char ch)
{
  send_to_char(messg, ch, LOG_PUBLIC);
}

void send_to_char(const char *messg, P_char ch, int log)
{
  static bool bSwitched = FALSE;

  if (ch && ch->desc && messg)
  {
    if( executing_ch != ch || !IS_SET(ch->specials.act, PLR_PAGING_ON) )
    {
      if( SWITCHED(ch) && !bSwitched )
      {
        char buf[30];
        snprintf(buf, 30, "&+M@&+W%s&n: ", J_NAME(ch) );
        bSwitched = TRUE;
        write_to_q(buf, &ch->desc->output, 1);
        bSwitched = FALSE;
      }
      write_to_q(messg, &ch->desc->output, 1);
    }
    else
    {
      static bool bWarningAdded = false;
      size_t      len = strlen(messg);

      if (!output_length)
        bWarningAdded = false;

      // once a 'warning' is appended, no more is added to the pager
      if (!bWarningAdded)
      {
        if (len < (MAX_COMMAND_OUTPUT - output_length))
        {
          strncat(command_output, messg, MAX_COMMAND_OUTPUT - output_length);
          output_length += len;
        }
        else
        {
          strncat(command_output, "\r\n\r\n&+W *** ...and the list goes on... ***&n\r\n", PAD_COMMAND_OUTPUT);
          bWarningAdded = true;
        }
      }
    }

    if((!IS_TRUSTED(ch) || log != LOG_PUBLIC) && log != LOG_NONE &&
        (ch->desc->connected == CON_PLAYING ||
         ch->desc->connected == CON_MAIN_MENU)) {
      write_to_pc_log(ch, messg, log);
    }
  }
}

bool send_to_pid(const char *str, int pid) {
        for (P_desc d = descriptor_list; d; d = d->next)
    {
      if (d->connected == CON_PLAYING && IS_PC(d->character) && GET_PID(d->character) == pid )
      {
        send_to_char(str, d->character);
                                return TRUE;
      }
    }
                return FALSE;
}

void send_to_all(const char *messg)
{
  P_desc   i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected)
        write_to_q(messg, &i->output, 2);
}

void send_to_outdoor(const char *messg)
{
  P_desc   i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && OUTSIDE(i->character))
        if (IS_TRUSTED(i->character) ||
            !IS_ROOM(i->character->in_room, ROOM_SILENT))
          write_to_q(messg, &i->output, 2);
}

void send_to_nearby_rooms(int from_room, const char *messg)
{
/* lags mud to hell and back */

#if 0
  P_desc   i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && OUTSIDE(i->character))
        if (!IS_ROOM(i->character->in_room, ROOM_SILENT) &&
            (how_close(from_room, i->character->in_room, 10) >= 0))
          write_to_q(messg, &i->output, 2);
#endif
}

void send_to_zone_outdoor(int z_num, const char *messg)
{
  send_to_zone_func(z_num, (int) (-ROOM_INDOORS), messg);
}

void send_to_zone_indoor(int z_num, const char *messg)
{
  send_to_zone_func(z_num, (int) ROOM_INDOORS, messg);
}

void send_to_zone(int z_num, const char *msg)
{
  send_to_zone_func(z_num, 0, msg);
}

void send_to_except(const char *messg, P_char ch)
{
  P_desc   i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (ch->desc != i && !i->connected)
        if (IS_TRUSTED(i->character) ||
            !IS_ROOM(i->character->in_room, ROOM_SILENT))
          write_to_q(messg, &i->output, 2);
}

static char send_to_room_f_buf[MAX_STRING_LENGTH];
void send_to_room_f(int room, const char *fmt, ... )
{
    va_list args;

    va_start(args, fmt);
    vsnprintf(send_to_room_f_buf, sizeof(send_to_room_f_buf) - 1, fmt, args);
    va_end(args);

    send_to_room(send_to_room_f_buf, room);
}
void send_to_room(const char *messg, int room)
{
  P_char   i;

  if ((room < 0) || (room > top_of_world))
  {
    logit(LOG_DEBUG, "send_to_room(): room numb out of range (%d)", room);
    return;
  }

  if (messg)
    for (i = world[room].people; i; i = i->next_in_room)
      if (i->desc)
        if (IS_TRUSTED(i) ||
            !IS_ROOM(i->in_room, ROOM_SILENT) ||
            i->specials.z_cord == 0)
          write_to_q(messg, &i->desc->output, 2);
}

void send_to_room_except(const char *messg, int room, P_char ch)
{
  P_char   i;
  char     Gbuf4[MAX_STRING_LENGTH];

  if ((room < 0) || (room > top_of_world))
  {
    logit(LOG_DEBUG, "send_to_room_except(): room numb out of range (%d)",
          room);
    return;
  }

  if (messg)
    for (i = world[room].people; i; i = i->next_in_room)
      if ((i != ch) && i->desc)
      {
        if (GET_LEVEL(i) >= GET_LEVEL(ch))
        {
          snprintf(Gbuf4, MAX_STRING_LENGTH, "R[%s]", GET_NAME(ch));
          write_to_q(Gbuf4, &i->desc->output, 1);
        }
        write_to_q(messg, &i->desc->output, 2);
      }
}

void send_to_room_except_two(const char *messg, int room, P_char ch1,
                             P_char ch2)
{
  P_char   i;

  if ((room < 0) || (room > top_of_world))
  {
    logit(LOG_DEBUG, "send_to_room_except_two(): room numb out of range (%d)",
          room);
    return;
  }

  if (messg)
    for (i = world[room].people; i; i = i->next_in_room)
      if ((i != ch1) && (i != ch2) && i->desc)
        if (IS_TRUSTED(i) ||
            !IS_ROOM(i->in_room, ROOM_SILENT))
          write_to_q(messg, &i->desc->output, 2);
}


void act_convert(char *buf, const char *str, P_char ch, P_char to, P_obj obj, void *vict_obj, int type)
{
  P_char   vict;
  char     tbuf[MAX_STRING_LENGTH];
  bool     found;
  int      j, tbp, skip, which_z, sil = FALSE, ig_zc = FALSE;
  register char *point;
  register const char *strp, *i;
  int      terseonly = FALSE;
  int      notterse = FALSE;
  bool     no_eol = FALSE;

  for (strp = str, point = buf;;)
  {
    if (*strp == '$')
    {
      j = 0;

      switch (*(++strp))
      {
      case 'n':
        if (ch && to)
          i = PERS(ch, to, FALSE);
        else
          i = NULL;

        break;

      case 'N':
        if (vict_obj && to)
          i = PERS((P_char) vict_obj, to, FALSE);
        else
          i = NULL;

        break;

      case 'm':
        if (ch)
          i = HMHR(ch);
        else
          i = NULL;

        break;

      case 'M':
        if (vict_obj)
          i = HMHR((P_char) vict_obj);
        else
          i = NULL;

        break;

      case 's':
        if (ch)
          i = HSHR(ch);
        else
          i = NULL;

        break;

      case 'S':
        if (vict_obj)
        {
          if (type == TO_VICT)
            i = "your";
          else
            i = HSHR((P_char) vict_obj);
        }
        else
          i = NULL;

        break;

      case 'e':
        if (ch)
          i = HSSH(ch);
        else
          i = NULL;

        break;

      case 'E':
        if (vict_obj)
          i = HSSH((P_char) vict_obj);
        else
          i = NULL;

        break;

      case 'o':
        if (obj && to)
          i = OBJN(obj, to);
        else
          i = NULL;

        break;

      case 'O':
        if (vict_obj && to)
          i = OBJN((P_obj) vict_obj, to);
        else
          i = NULL;

        break;

      case 'p':
        if (obj && to)
          i = OBJS(obj, to);
        else
          i = NULL;

        break;

      case 'P':
        if (vict_obj && to)
          i = OBJS((P_obj) vict_obj, to);
        else
          i = NULL;

        break;

        /*
         * 'q's' are same as p's except it kills 'A |An
         * |The' from the start of the string, it's ugly,
         * cause we have to skip leading ansi stuff. JAB
         */
      case 'q':
      case 'Q':
        *tbuf = '\0';
        tbp = 0;
        skip = 0;
        found = FALSE;

        if (*strp == 'Q')
        {
          if (vict_obj && to)
            i = OBJS((P_obj) vict_obj, to);
          else
            i = NULL;
        }
        else
        {
          if (obj && to)
            i = OBJS(obj, to);
          else
            i = NULL;
        }

        if (i == NULL)
          break;

        for (; *i; i++)
        {
          if (skip)
          {
            skip--;
          }
          else
          {
/*
 * ANSI skipping
 */
            if (!found && (*i == '&'))
            {
              if ((*(i + 1) == 'N') || (*(i + 1) == 'N'))
                skip = 1;
              else if ((*(i + 1) == '-') || (*(i + 1) == '+'))
                skip = 2;
              else if (*(i + 1) == '=')
                skip = 3;
            }

// a and an

            if (!found && (LOWER(*i) == 'a') && (*(i + 1)))
            {
              if (*(i + 1) == ' ')
              {
                found = TRUE;
                i++;
                continue;
              }

              if ((LOWER(*(i + 1)) == 'n') && *(i + 2) && (*(i + 2) == ' '))
              {
                found = TRUE;
                i += 2;
                continue;
              }
            }

// the

            if (!found && (LOWER(*i) == 't'))
            {
              if ((LOWER(*(i + 1)) == 'h') && (LOWER(*(i + 2)) == 'e') &&
                  (*(i + 3) == ' '))
              {
                found = TRUE;
                i += 3;
                continue;
              }
            }

// some

            if (!found && (LOWER(*i) == 's') && (LOWER(*(i + 1)) == 'o') &&
                (LOWER(*(i + 2)) == 'm') && (LOWER(*(i + 3)) == 'e') &&
                (LOWER(*(i + 4)) == ' '))
            {
              found = TRUE;
              i += 4;
              continue;
            }
          }

          tbuf[tbp++] = *i;
        }

        tbuf[tbp++] = 0;
        i = tbuf;

        break;

      case 'a':
        if (obj)
          i = SANA(obj);
        else
          i = NULL;

        break;

      case 'A':
        if (vict_obj)
          i = SANA((P_obj) vict_obj);
        else
          i = NULL;

        break;

      case 'T':
        if (vict_obj)
          i = (char *) vict_obj;
        else
          i = NULL;

        break;

      case 'F':
        if (vict_obj)
          i = FirstWord((char *) vict_obj);
        else
          i = NULL;

        break;

      case 'w':                /* complicated crap, I use it for dam_messages() */
        if (type == TO_VICT)
        {
          if (ch && to)
            i = "you";
          else
            i = NULL;
        }
        else if (type == TO_CHAR)
        {
          if (vict_obj && to)
            i = PERS((P_char) vict_obj, to, FALSE);
          else
            i = NULL;
        }
        else if (type == TO_NOTVICT)
        {
          if (ch && to)
            i = PERS((P_char) vict_obj, to, FALSE);
          else
            i = NULL;
        }

        break;

      case 'W':
        if (type == TO_VICT)
          i = "r";              /* changes you to your */
        else
          i = "'s";             /* changes joe to joe's */

        break;

      case '$':
        i = "$";

        break;

      default:
        logit(LOG_DEBUG, "Invalid $-code, act(): $%c %s", *strp, str);
        i = NULL;

        break;
      }

      if (i)
        while (*(i + j))
          *(point++) = *(i + j++);

      ++strp;
    }
    else if (!(*(point++) = *(strp++)))
      break;
  }

  if (!no_eol)
  {
    *(--point) = '\n';
    *(++point) = '\r';
    *(++point) = '\0';
  }

  CAP(buf);
}

/*
 * higher-level communication

 n: ch name ("the boy")
 m: pronoun object ("him")
 s: possessive ("his")
 e: pronoun subject ("he")
 o: obj name ("totem")
 p: obj short description ("the lime-green totem")
 q: obj short description w/o article (a/an/the) ("lime-green totem")
 a: obj article (a/an/the) ("the")
 */
void act(const char *str, int hide_invisible, P_char ch, P_obj obj, void *vict_obj, int type)
{
  P_char   to, vict;
  bool     found;
  // The array buf contains our primary string (final to be sent to target).
  char     buf[MAX_STRING_LENGTH], tbuf[MAX_STRING_LENGTH], tbuf2[MAX_STRING_LENGTH];
  /* Debugging
  char     mybuf[MAX_STRING_LENGTH];
  int      mycheck;
   */
  int      j, tbp, skip, which_z, sil = type & ACT_SILENCEABLE;
  bool     ignore_zcoord = type & ACT_IGNORE_ZCOORD;
  register char *point;
  register const char *strp, *i;
  int      terseonly = type & ACT_TERSE;
  int      notterse = type & ACT_NOTTERSE;
  bool     no_eol = type & ACT_NOEOL;
  unsigned int flags = type & ~7;

  type &= 7;

  if( !str || !*str )
    return;

  which_z = (ch ? ch->specials.z_cord : 0);

  if( type == TO_VICT )
  {
    to = (P_char) vict_obj;
    if( to == NULL || !to->desc )
      return;
/*    which_z = (to ? to->specials.z_cord : 0); */
  }
  else if( type == TO_CHAR )
  {
    if( ch == NULL || !ch->desc )
    {
      return;
    }
    to = ch;
  }
  else if( type == TO_VICTROOM || type == TO_NOTVICTROOM )
  {
    vict = (P_char) vict_obj;
    if( vict )
    {
      if( vict->in_room == NOWHERE )
      {
        logit(LOG_DEBUG, "act TO_VICTROOM in NOWHERE %s (%s).", GET_NAME(vict), str);
        return;
      }
      to = world[vict->in_room].people;
    }
    else
    {
      return;
    }
    /*   which_z = (to ? to->specials.z_cord : 0); */
  }
  else
  {
    if( !ch && obj )
    {
      if( !OBJ_ROOM(obj) )
      {
        logit(LOG_DEBUG, "Comm.c act: no ch, has obj, but obj (%d) not in a room.", OBJ_VNUM(obj));
        return;
      }
      to = world[obj->loc.room].people;
      which_z = obj->z_cord;
    }
    else
    {
      if( !ch || ch->in_room == NOWHERE )
      {
/*        logit(LOG_DEBUG, "act TO_ROOM in NOWHERE %s (%s).", GET_NAME(ch), str);*/
        return;
      }
      to = world[ch->in_room].people;
      which_z = ch->specials.z_cord;
    }
  }

  if( !to )
    return;                     /* if a tree falls in the forest... */

  for( ; to; to = to->next_in_room )
  {
    // Viewing character needs a descriptor to send to, needs to be awake, and match z-requirements...
    if( to->desc && IS_AWAKE(to) && (ignore_zcoord || ( to->specials.z_cord == which_z ))
    //   needs to not be ignoring target ch
      && (IS_NPC( to ) || !to->only.pc->ignored || ( to->only.pc->ignored != ch ))
    //   needs to match the target type: Only TO_CHAR is shown to ch, NOTVICT/NOTVICTROOM doesn't show to victim.
      && (( type == TO_CHAR ) || ( to != ch ))
      && !(( type == TO_NOTVICT || type == TO_NOTVICTROOM ) && ( to == (P_char) vict_obj ))
    //   needs to have terse toggled appropriately
      && (IS_NPC( to ) || ( !terseonly && !notterse ) || ( (terseonly && IS_SET( to->specials.act2, PLR2_TERSE ))
      || (notterse && !IS_SET( to->specials.act2, PLR2_TERSE )) ))
    //   and message shouldn't be hidden due to an invisible ch or obj.
      && (!hide_invisible || ( to == ch ) || ( ch ? CAN_SEE(to, ch) : CAN_SEE_OBJ(to, obj) )) )
    {
      // If it's silencable, and not to an Imm, and there is a flag flag to not show it.
      if( sil && !IS_TRUSTED(to)
        && (IS_ROOM( to->in_room, ROOM_SILENT ) || IS_AFFECTED4( to, AFF4_DEAF )) )
      {
        continue;
      }

      for( strp = str, point = buf; ; )
      {
        if( *strp == '$' )
        {
          j = 0;

          switch( *(++strp) )
          {
          case 'n':
            if( ch )
            {
              if( ch == to )
              {
                i = "you";
              }
              else
              {
                i = PERS(ch, to, FALSE);
              }
            }
            else
            {
              i = "(NULL)";
            }
            break;

          case 'N':
            if( vict_obj )
            {
              if( (P_char)vict_obj == to )
              {
                i = "you";
              }
              else
              {
                i = PERS((P_char) vict_obj, to, FALSE);
              }
            }
            else
            {
              i = "(NULL)";
            }
            break;

          case 'm':
            if( ch )
              i = HMHR(ch);
            else
              i = "(NULL)";
            break;

          case 'M':
            if( vict_obj )
              i = HMHR((P_char) vict_obj);
            else
              i = "(NULL)";
            break;

          case 's':
            if( ch )
            {
              if( type == TO_CHAR )
                i = "your";
              else
                i = HSHR(ch);
            }
            else
              i = "(NULL)'s";
            break;

          case 'S':
            if( vict_obj )
            {
              if( type == TO_VICT )
                i = "your";
              else
                i = HSHR((P_char) vict_obj);
            }
            else
              i = "(NULL)'s";
            break;

          case 'e':
            if( ch )
              i = HSSH(ch);
            else
              i = "(NULL)";
            break;

          case 'E':
            if( vict_obj )
              i = HSSH((P_char) vict_obj);
            else
              i = "(NULL)";
            break;

          case 'o':
            if( obj )
              i = OBJN(obj, to);
            else
              i = "(NULL)";

            break;

          case 'O':
            if( vict_obj )
              i = OBJN((P_obj) vict_obj, to);
            else
              i = "(NULL)";
            break;

          case 'p':
            if( obj )
              i = OBJS(obj, to);
            else
              i = "(NULL)";
            break;

          case 'P':
            if( vict_obj )
              i = OBJS((P_obj) vict_obj, to);
            else
              i = "(NULL)";
            break;

            /*
             * 'q's' are same as p's except it kills 'A | An | The | Some' from
             * the start of the string, it's ugly, cause we have to skip leading ansi stuff. JAB
             */
          case 'q':
          case 'Q':
            if( *strp == 'Q' )
            {
              if( vict_obj )
                i = OBJS((P_obj) vict_obj, to);
              else
                i = NULL;
            }
            else
            {
              if( obj )
                i = OBJS(obj, to);
              else
                i = NULL;
            }

            if( i == NULL )
            {
              i = "(NULL)";
              break;
            }

            tbuf[0] = '\0';
            tbp = 0;
            // First _copy_ ansi to tbuf.
            while( *i == '&' )
            {
              if( i[1] == 'n' || i[1] == 'N' )
              {
                tbuf[tbp++] = '&';
                i++;
                tbuf[tbp++] = *(i++);
              }
              // Begins with && -> actual & to target.  What to do here is debatable.
              // Decided to just break and leave && as beginning of i.
              else if( i[1] == '&' )
              {
                break;
              }
              else if( (i[1] == '+' || i[1] == '-') && is_ansi_char(i[2]) )
              {
                tbuf[tbp++] = '&';
                i++;
                tbuf[tbp++] = *(i++);
                tbuf[tbp++] = *(i++);
              }
              else if( i[1] == '=' && is_ansi_char(i[2]) && is_ansi_char(i[3]) )
              {
                tbuf[tbp++] = '&';
                i++;
                tbuf[tbp++] = *(i++);
                tbuf[tbp++] = *(i++);
                tbuf[tbp++] = *(i++);
              }
              // Not an ansi code.
              else
                break;
            }

            // Now, if the rest starts with "A " or "An " skip those chars.
            if( (LOWER(*i) == 'a') )
            {
              if( i[1] == ' ' )
              {
                i += 2;
              }
              else if( (LOWER( i[1] ) == 'n') && ( i[2] == ' ') )
              {
                i += 3;
              }
            }
            // Same for "The "
            else if( (LOWER( *i ) == 't') && (LOWER( i[1] ) == 'h') && (LOWER( i[2] ) == 'e')
              && (LOWER( i[3] ) == ' ') )
            {
              i += 4;
            }
            // And same for "Some "
            else if( (LOWER( *i ) == 's') && (LOWER( i[1] ) == 'o') && (LOWER( i[2] ) == 'm')
              && (LOWER( i[3] ) == 'e') && (LOWER( i[3] ) == ' ') )
            {
              i += 5;
            }

            // If the whole string was just ansi chars with optional article (smh.. but zone writers).
            if( *i == '\0' )
              i = "(NULL)";
            // Otherwise, add the rest of the string to the end of tbuf (contains ansi).
            else
            {
              snprintf(tbuf + tbp, MAX_STRING_LENGTH, "%s", i );
              i = tbuf;
            }
            break;

          case 'a':
            if( obj )
              i = SANA(obj);
            else
              i = "(NULL)";
            break;

          case 'A':
            if( vict_obj )
              i = SANA((P_obj) vict_obj);
            else
              i = "(NULL)";
            break;

          case 'T':
            if( vict_obj )
              i = (char *) vict_obj;
            else
              i = "(NULL)";
            break;

          case 'F':
            if( vict_obj )
              i = FirstWord((char *) vict_obj);
            else
              i = "(NULL)";
            break;

          case 'w':            /* complicated crap, I use it for dam_messages() */
            if( type == TO_VICT )
            {
              if( ch )
                i = "you";
              else
                i = "(NULL)";
            }
            else if( type == TO_CHAR )
            {
              if( vict_obj )
                i = PERS((P_char) vict_obj, to, FALSE);
              else
                i = "(NULL)";
            }
            else if( type == TO_NOTVICT )
            {
              if( ch )
                i = PERS((P_char) vict_obj, to, FALSE);
              else
                i = "(NULL)";
            }
            else if( type == TO_NOTVICTROOM )
            {
              if( ch )
                i = PERS((P_char) vict_obj, to, FALSE);
              else
                i = "(NULL)";
            }
            break;

          case 'W':
            if( type == TO_VICT )
              i = "r";          /* changes you to your */
            else
              i = "'s";         /* changes joe to joe's */
            break;

          case '$':
            i = "$";
            break;

          default:
            logit(LOG_DEBUG, "act(): Invalid $-code: '$%c' in '%s'.", *strp, str);
            i = "(NULL)";
            break;
          }

          if( i )
          {
            // Note: This doesn't handle ansi in the middle of the lower-cased words.
            // Making it so we don't get 'A', 'An', 'The', or 'Some' in the middle of a sentence (removing caps)!
            // For each word,
            for( tbp = 0; *i; )
            {
              // Copy beginning ansi.
              while( *i == '&' )
              {
                if( i[1] == 'n' || i[1] == 'N' )
                {
                  tbuf2[tbp++] = '&';
                  i++;
                  tbuf2[tbp++] = *(i++);
                }
                // Begins with && -> actual & to target.  We just copy the && over in this case.
                else if( i[1] == '&' )
                {
                  tbuf2[tbp++] = '&';
                  tbuf2[tbp++] = '&';
                  i += 2;
                }
                else if( (i[1] == '+' || i[1] == '-') && is_ansi_char(i[2]) )
                {
                  tbuf2[tbp++] = '&';
                  i++;
                  tbuf2[tbp++] = *(i++);
                  tbuf2[tbp++] = *(i++);
                }
                else if( i[1] == '=' && is_ansi_char(i[2]) && is_ansi_char(i[3]) )
                {
                  tbuf2[tbp++] = '&';
                  i++;
                  tbuf2[tbp++] = *(i++);
                  tbuf2[tbp++] = *(i++);
                  tbuf2[tbp++] = *(i++);
                }
              }

              // "A " or "An "
              if( *i == 'A' )
              {
                if( i[1] == ' ' )
                {
                  tbuf2[tbp++] = 'a';
                  i++;
                }
                else if( (LOWER( i[1] ) == 'n') && (i[2] == ' ') )
                {
                  tbuf2[tbp++] = 'a';
                  tbuf2[tbp++] = 'n';
                  i += 2;
                }
                else
                {
                  tbuf2[tbp++] = 'A';
                  i++;
                }
              }
              // "The "
              else if( (*i == 'T') && (LOWER( i[1] ) == 'h') && (LOWER( i[2] ) == 'e') && (i[3] == ' ') )
              {
                tbuf2[tbp++] = 't';
                tbuf2[tbp++] = 'h';
                tbuf2[tbp++] = 'e';
                i += 3;
              }
              // "Some "
              else if( (*i == 'S') && (LOWER( i[1] ) == 'o') && (LOWER( i[2] ) == 'm')
                && (LOWER( i[3] ) == 'e') && (LOWER( i[4] ) == ' ') )
              {
                tbuf2[tbp++] = 's';
                tbuf2[tbp++] = 'o';
                tbuf2[tbp++] = 'm';
                tbuf2[tbp++] = 'e';
                i += 4;
              }
              // Any other word, just copy.
              else
              {
                while( !isspace(*i) && *i != '\0' )
                {
                  tbuf2[tbp++] = *(i++);
                }
              }

              // Copy following white-space
              while( isspace(*i) )
              {
                tbuf2[tbp++] = *(i++);
              }
            }
            tbuf2[tbp++] = '\0';
            i = tbuf2;

            for( j = 0; *(i + j) != '\0'; j++ )
            {
              *(point++) = *(i + j);
            }
          }
          // Move past the character following the $.
          ++strp;
        }
        // If it's not a $, just copy it, breaking at end of char *.
        else if( (*( point++ ) = *( strp++ )) == '\0' )
          break;
      }

      // Add \n\r to end of char *.
      if( !no_eol )
      {
        *(--point) = '\n';
        *(++point) = '\r';
        *(++point) = '\0';
      }

      /* CAP does this now...
      // Skip beginning ansi(s) and capitalize the first char.
      point = buf;
      while( *point == '&' && ( LOWER(*(point+1)) == 'n'
        || (*(point+1) == '+' && is_ansi_char( *(point+2) ))
        || (*(point+1) == '-' && is_ansi_char( *(point+2) ))
        || (*(point+1) == '=' && is_ansi_char( *(point+2) ) && is_ansi_char( *(point+3) )) ) )
      {
        if( *(point+1) == '+' || *(point+1) == '-' )
        {
          point += 3;
        }
        else if( *(point+1) == '=' )
        {
          point += 4;
        }
        else
        {
          point += 2;
        }
      }
      */

//      act_convert(mybuf, str, ch, to, obj, vict_obj, type);
//      mycheck = strcmp(mybuf, buf);

      CAP(buf);
      send_to_char(buf, to, (flags & ACT_PRIVATE) ? LOG_PRIVATE : LOG_PUBLIC);
    }

    // If there's only one recipient and we've sent the message to them, go ahead and return.
    if( (type == TO_VICT) || (type == TO_CHAR) )
      return;
  }
}

// This function will have to be fixed if sound re-enabled. - Lohrr
// Right now, we're overwriting a const string.
const char *delete_doubledollar(const char *string)
{
  char    *read1, *write1;

//  if ((write1 = strchr(string, '$')) == NULL)
    return string;

  read1 = write1;

  while (*read1)
    if ((*(write1++) = *(read1++)) == '$')
      if (*read1 == '$')
        read1++;

  *write1 = '\0';

  return string;
}

// Puts a Cyan % in front of each line.
void format_to_snoopers( char *from_string, char *to_string )
{
  char *index, *index2;

//  debug( "From: '%s'.", from_string );
  index2 = to_string;
  snprintf(index2, MAX_STRING_LENGTH, "&+C%%&N " );
  index2 += 7;
  index = from_string;
  while( *index != '\0' )
  {
    if( *index == '\r' )
    {
      index++;
      continue;
    }
    if( index[0] == '\n' && index[1] != '\0' )
    {
      snprintf(index2, MAX_STRING_LENGTH, "\n&+C%%&N " );
      index2 += 8;
      index++;
    }
    else
    {
      *(index2++) = *(index++);
    }
  }
  *index2 = '\0';
}
