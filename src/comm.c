/*
 **************************************************************************
 *  File: comm.c                                             Part of Duris
 *  Usage: socket handling, main game loop
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 **************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utility.h"
#include "utils.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <gnutls/gnutls.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <zlib.h>
#include "assocs.h"
#include "auction_houses.h"
#include "boon.h"
#include "copyover.h"
#include "ctf.h"
#include "epic.h"
#include "ferry.h"
#include "gmcp.h"
#include "graph.h"
#include "guildhall.h"
#include "hardcore.h"
#include "json_utils.h"
#include "lookup_process.h"
#include "map.h"
#include "mccp.h"
#include "mm.h"
#include "nexus_stones.h"
#include "outposts.h"
#include "player_log.h"
#include "poll.h"
#include "profile.h"
#include "racewar_stat_mods.h"
#include "redis.h"
#include "ships.h"
#include "siege.h"
#include "spells.h"
#include "enhance.h"
#include "crafting.h"
#include "account_reward_config.h"
#include "frag_cap_config.h"
#include "hardcore_config.h"
#include "random_equipment_config.h"
#include "creation_availability_config.h"
#include "material_rarity.h"
#include "sql.h"
#include "sql_player.h"
#include "telnet.h"
#include "timers.h"
#include "tradeskill.h"
#include "ttype.h"
#include "unicode.h"
#include "websocket.h"
#include "world_quest.h"
#include "ws_handlers.h"
#include "latency_trace.h"
#include "persistence_queue.h"
#include "locker_async.h"
#if !defined(__NO_TESTS__) || defined(TEST_REAL_PERSISTENCE)
#include "test_async.h"
#endif

/* external variables */

extern P_event               schedule[];
extern P_index               mob_index;
extern P_room                world;
extern char                  debug_mode;
extern const int             top_of_world;
extern int                   top_of_zone_table;
extern struct ban_t         *ban_list;
extern struct wizban_t      *wizconnect;
extern struct time_info_data time_info;
extern struct zone_data     *zone;
extern struct zone_data     *zone_table;
extern char                 *shutdown_message;
extern const int             max_ingame_good;
extern const int             max_ingame_evil;
extern TimedShutdownData     shutdownData;
extern void                  timedShutdown(P_char ch, P_char, P_obj, void *data);
extern void                  checkpointing(void);

long sentbytes    = 0;
long recivedbytes = 0;
bool game_booted  = FALSE;

void request_shutdown(int shutdown_type, const char *issuer, const char *reason)
{
	shutdownData.reboot_time  = time(0);
	shutdownData.next_warning = -1;
	snprintf(shutdownData.IssuedBy, sizeof(shutdownData.IssuedBy), "%s", issuer ? issuer : "Launcher");
	snprintf(shutdownData.Reason, sizeof(shutdownData.Reason), "%s", reason ? reason : "signal from launcher");
	switch (shutdown_type)
	{
		case 1:
			shutdownData.eShutdownType = TimedShutdownData::OK;
			break;
		case 2:
			shutdownData.eShutdownType = TimedShutdownData::REBOOT;
			break;
		case 3:
			shutdownData.eShutdownType = TimedShutdownData::COPYOVER;
			break;
		default:
			shutdownData.eShutdownType = TimedShutdownData::OK;
			break;
	}
	timedShutdown(NULL, NULL, NULL, NULL);
}

extern void ne_events();

long unsigned int ip2ul(const char *ip);
void              load_alliances();
void              initialize_transport();
bool              newLeaderBoard(P_char ch, char *arg, int cmd);
bool              newHardcoreBoard(P_char ch, char *arg, int cmd);
void              format_to_snoopers(char *from_string, char *to_string);
extern void       update_breath_weapon_properties();
extern void       update_regen_properties();
static void       greet(P_desc newd);
static void       note_player_input_activity(P_desc t, const char *input);
static void       process_line(P_desc t, char *in);

/* local globals */

P_desc descriptor_list, next_to_process, next_save = 0;
fd_set input_set, output_set, exc_set; /* for socket handling */
int    bounce_null_sites = 0;
int    mini_mode         = 0;
int    lawful            = 0;
int    no_specials       = 0;
int    override          = 1;
int    pulse             = 0;
bool   after_events_call = FALSE;
const char *material_rarity_report_dir = NULL;
bool        material_rarity_report_mode = FALSE;
int    _reboot           = 0;
int    _copyover         = 0;
int    _autoboot         = 0;
int    _pwipe            = 0;
int    req_passwd        = 1;
int    shutdownflag      = 0;
// signal-initiated shutdown: 0=none, 1=shutdown, 2=reboot, 3=copyover
volatile sig_atomic_t signal_shutdown_pending = 0;
int                   slow_death              = 0;
volatile sig_atomic_t tics                    = 0;
long                  boot_time;
int                   ipc_id    = 0;
int                   was_upper = FALSE;
pid_t                 lookup_host_process;
pid_t                 lookup_ident_process;
int                   max_users_playing = 0;
int                   used_descs = 0, avail_descs = 0, max_descs = 0, max_descs_this_hour = 0;
struct mm_ds         *dead_desc_pool = NULL;
int                   RUNNING_PORT   = 0;
int                   no_random      = 0;
int                   no_ferries     = 0;

// copyover support
int        copyover_boot             = 0;
static int recovered_mother_desc     = -1;
static int recovered_mother_desc_ssl = -1;
static int recovered_ws_desc         = -1;

// listening sockets - stored here so copyover can access them
static int mother_desc     = -1;
static int mother_desc_ssl = -1;
static int ws_desc         = -1;
P_char     executing_ch;
#define MAX_COMMAND_OUTPUT (15 * MAX_STRING_LENGTH) // upped this to 3x vs original MWD26
#define PAD_COMMAND_OUTPUT (500)                    // some space for appending a warning
char   command_output[MAX_COMMAND_OUTPUT + PAD_COMMAND_OUTPUT + 1];
size_t output_length;

/* ============= Ansi color table and data structure  ============== */

struct ansi_color
{
	const char *symbol;  /* Symbol used in the game. eg &+symbol */
	const char *fg_code; /* ^[[fg_codem; = foreground color code */
	const char *bg_code; /* Same except it's the background color */
} color_table[] = {

	{ "L", "30", "40"}, /* * Black            */
	{ "R", "31", "41"}, /* * Red              */
	{ "G", "32", "42"}, /* * Green            */
	{ "Y", "33", "43"}, /* * Yellow           */
	{ "B", "34", "44"}, /* * Blue             */
	{ "M", "35", "45"}, /* * Magenta          */
	{ "C", "36", "46"}, /* * Cyan             */
	{ "W", "37", "47"}, /* * White            */
	{NULL, NULL, NULL}  /* * End of the table */
};

#define MIN_SOCKET_BUFFER_SIZE 20480

/*
 * ********************************************************************* *
 * main game loop and related stuff                                 *
 * *********************************************************************
 */

int main(int argc, char **argv)
{
	int         port, sslport;
	int         pos = 1;
	const char *dir;
	int         migrate_mode = 0;

	port    = DFLT_PORT;
	dir     = DFLT_DIR;
	sslport = SSL_PORT;

	randomize(0);

	// check for --migrate-all before regular arg parsing
	for (int i = 1; i < argc; i++)
	{
		if (!strncmp(argv[i], "--material-rarity-report", strlen("--material-rarity-report")))
		{
			const char *value = argv[i] + strlen("--material-rarity-report");
			if (*value == '=')
				value++;
			else if (!*value && i + 1 < argc)
				value = argv[++i];
			material_rarity_report_dir = *value ? value : "material-rarity-report";
			material_rarity_report_mode = TRUE;
			break;
		}
		if (!strcmp(argv[i], "--migrate-all"))
		{
			migrate_mode = 1;
			break;
		}
	}

	while ((pos < argc) && (*(argv[pos]) == '-'))
	{
		if (!strncmp(argv[pos], "--material-rarity-report", strlen("--material-rarity-report")))
		{
			if (argv[pos][strlen("--material-rarity-report")] == '\0' && pos + 1 < argc)
				pos++;
			pos++;
			continue;
		}
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
					fatal_boot_error("comm", "Directory arg expected after option -d.");
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
				mini_mode  = 1;
				no_ferries = 1;
				logit(LOG_STATUS, "Running in mini mode");
				break;
			case 'z':
				mini_mode = 2;
				logit(LOG_STATUS, "Running with area debugger on");
				break;
			case 'C':
				// copyover boot - sockets recovered from copyover.dat
				copyover_boot = 1;
				logit(LOG_STATUS, "Copyover boot mode");
				break;
			default:
				logit(LOG_STATUS, "Unknown option -% in argument string.", *(argv[pos] + 1));
				break;
		}
		pos++;
	}

	if (pos < argc)
		if (!isdigit(*argv[pos]))
		{
			fatal_boot_error("comm", "Usage: %s [-l] [-m] [-s] [-p] [-n] [-f] [-d pathname] [ port # ]", argv[0]);
		}
		else if ((port = atoi(argv[pos])) <= 1024)
		{
			fatal_boot_error("comm", "Illegal port #");
		}
		else
			sslport = port + 1;
	// Global variable so can check if mainmud or not!
	RUNNING_PORT = port;

	/* create an IPC msg queue to deal with hostname lookups.  */
	/*
	  ipc_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);
	  if (ipc_id < 0) {
	    fatal_boot_error("comm", "Unable to create message queue due to %d!", ipc_id);
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
		fatal_boot_error("comm", "chdir failed: %s", strerror(errno));
	}
	if (material_rarity_report_mode)
	{
		boot_material_rarity_objects(mini_mode);
		write_material_rarity_report(material_rarity_report_dir);
		return 0;
	}
	logit(LOG_STATUS, "Running game on port %d.", port);

	logit(LOG_STATUS, "Using %s as data directory.", dir);

	load_env_file();

	if (initialize_mysql() < 0)
	{
		fatal_boot_error("comm", "MySQL initialization failed!");
	}

	redis_init();

	// run migration and exit if requested
	if (migrate_mode)
	{
		printf("running pfile migration...\n");
		int count = sql_migrate_all_players();
		printf("migration complete: %d players migrated\n", count);
		return 0;
	}

	initialize_properties();

	load_event_names();

	init_cmdlog(); /* init cmd.debug file - DCL */

	run_the_game(port, sslport);

	return (0);
}

// all text meant to go to executing_ch - a player whos command
// is currently processed is saved into command_output
// buffer instead of being sent over the network
// the intercepting happens in send_to_char
void process_with_paging(P_char ch, char *comm)
{
	executing_ch    = ch;
	*command_output = '\0';
	output_length   = 0;
	command_interpreter(ch, comm);
	executing_ch = NULL;
	if (!ch->desc)
		return;
	else if (next_page(command_output, ch->desc))
		// page_string_real(ch->desc, command_output, 1);
		page_string_real(ch->desc, command_output);
	else
		SEND_TO_Q(command_output, ch->desc);
}

void game_up_message(int port)
{
	FILE *f;
	char  Gbuf1[200];

	f = fopen("foo_tmp", "w");
	snprintf(Gbuf1, 200, "Duris> The mud is up at port %d. Run! Panic! *FLEE*\n", port);
	fputs(Gbuf1, f);
	fclose(f);
	system("/usr/local/bin/stealth-wall < foo_tmp");
	unlink("foo_tmp");
	//  signal(SIGCHLD, (void *) reaper);
}

static void touch(const char *filename)
{
	// no need to check for failure, the next step will do
	close(open(filename, O_WRONLY | O_CREAT, 0666));
}

/* Init sockets, run game, and cleanup sockets */

void run_the_game(int port, int sslport)
{
	long time_before = 0;
	long time_after  = 0;
	char buf[MAX_STRING_LENGTH];

	descriptor_list = NULL;

	time_before = clock();

	logit(LOG_STATUS, "Signal trapping.");
	signal_setup();

	SetSpellCircles(); /* spells circlewise done with pure math */

	// check for redis crash recovery before boot_db (so ne_init_events skips zone resets)
	if (!copyover_boot && redis_enabled && redis_world_state_enabled && redis_has_world_state())
	{
		crash_recovery_boot = 1;
		logit(LOG_STATUS, "Crash recovery data found in redis, will restore world state after boot");
	}

	boot_db(mini_mode);

	// game_up_message(port);
	init_astral_clock(); // fix the map sight distances

	// cache named report, fraglist, and epic zones in redis
	redis_cache_named_report();
	redis_cache_fraglist();
	redis_cache_epic_zones();

	// clear stale online list from previous boot/crash
	redis_clear_online_players();

	if (no_random == 0)
		create_randoms();
	else
		fprintf(stderr, "Starting without random zones!.\n\r");

	fprintf(stderr, "-- Updating zone database.\r\n");
	update_zone_db();

	calculate_map_coordinates();
	fprintf(stderr, "--  Done calculating maps coordinates.\r\n");

	fprintf(stderr, "-- Calculating avg mob level for each zone.\r\n");
	if (!mini_mode)
	{
		calc_zone_mob_level();
	}
	else
	{
		fprintf(stderr, "--  Skipping mob-level tradeskill load in mini mode.\r\n");
	}
	fprintf(stderr, "--  Done calculating mob level.\r\n");

	if (!mini_mode)
		initialize_tradeskills();
	else
		fprintf(stderr, "--  Skipping tradeskills/mines in mini mode.\r\n");
	fprintf(stderr, "--  Done loading tradeskills/mines.\r\n");

	if (!mini_mode)
		load_cmd_attributes();
	else
		fprintf(stderr, "--  Skipping command attributes in mini mode.\r\n");
	fprintf(stderr, "--  Done loading command attributes.\r\n");

	if (!mini_mode)
	{
		if (no_ferries == 0)
			init_ferries();
		else
			fprintf(stderr, "Starting without ferries.\r\n");

		initialize_transport();

		update_breath_weapon_properties();
		update_regen_properties();

		// initialize_buildings();

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

		fprintf(stderr, "-- Booting enhancement system\r\n");
		boot_enhancement_system();

		fprintf(stderr, "-- Booting crafting system\r\n");
		boot_crafting_system();

		fprintf(stderr, "-- Loading random equipment configuration\r\n");
		boot_random_equipment_config();

		fprintf(stderr, "-- Loading frag-cap configuration\r\n");
		boot_frag_cap_config();

		fprintf(stderr, "-- Loading account reward configuration\r\n");
		boot_account_reward_config();

		fprintf(stderr, "-- Loading Hardcore configuration\r\n");
		boot_hardcore_config();

		fprintf(stderr, "-- Loading creation availability configuration\r\n");
		boot_creation_availability_config();

#ifdef SIEGE_ENABLED
		fprintf(stderr, "-- Loading town data\r\n");
		init_towns();

		fprintf(stderr, "-- Loading siege data\r\n");
		init_siege();
#endif

		// This guarentees that files exist for reading.
		fprintf(stderr, "-- Touching leaderboard\r\n");
		touch(leaderboard_file);
		newLeaderBoard(NULL, "boot", 0);
		fprintf(stderr, "-- Touching hall of fame\r\n");
		touch(halloffamelist_file);
		newHardcoreBoard(NULL, "boot", 0);
		init_ctf();

		loadHints();
		epic_initialization();
		ssl_read_cert();
	}
	else
	{
		fprintf(stderr, "--  Skipping optional subsystems in mini mode.\r\n");
	}
	 time_after = clock();
	bfs_reset_marks();
	fprintf(stderr, "Boot completed in: %d milliseconds\n", (int)((time_after - time_before) * 1E3 / CLOCKS_PER_SEC));
	logit(LOG_STATUS, "Boot completed in:%d milliseconds\n", (int)((time_after - time_before) * 1E3 / CLOCKS_PER_SEC));

	game_booted = TRUE;

	fprintf(stderr, "Entering game loop.\n\r");
	logit(LOG_STATUS, "Entering game loop.");
	if (mini_mode)
	{
		persistence_replay_fallback_events();
		logit(LOG_STATUS, "Skipping persistence worker startup in mini mode.");
	}
	else
	{
		persistence_replay_fallback_events();
		persistence_start_item_event_worker();
		persistence_start_scalar_event_worker();
		persistence_start_large_event_worker();
		locker_async_init();
	}

	/* Boot-time scalar queue flood test: overflows the queue so the
	 * latency_trace instrumentation can capture scalar_enq_ok/drop
	 * and fallback_file_write statistics in the next periodic dump.
	 * Reset all TU-level ring buffers before the test so data is clean. */
	latency_trace_reset();
	persistence_queue_latency_reset();
	utility_latency_reset();
#ifndef __NO_TESTS__
	test_persistence_run_one("queue_flood_scalar");
	test_persistence_run_one("queue_routes_oversize_scalar_to_large");
	test_persistence_run_one("queue_routes_oversize_item_to_large");
	test_persistence_run_one("worker_scalar_fallback");
	test_persistence_run_one("worker_scalar_fifo_after_retry");
	test_persistence_run_one("worker_item_fifo");
	test_persistence_run_one("worker_large_roundtrip");
#endif
#ifdef TEST_REAL_PERSISTENCE
	test_real_persistence_run_all();
	test_real_persistence_print_summary();
#endif

	game_loop(port, sslport);
	if (!_pwipe)
	{
		persistence_stop_scalar_event_worker();
		persistence_stop_large_event_worker();
		persistence_stop_item_event_worker();
		locker_async_shutdown();
	}

	/* Don't need this anymore, as dropped artis are handled in real time on the DB.
	// Look for dropped artis and remove them from the next boot.
	dropped_arti_hunt();
	*/

#ifdef MEMCHK
	if (!_copyover)
	{
		free_world();
		dump_mem_log();
	}
#endif

	if (_reboot)
	{
		logit(LOG_EXIT, "Rebooting.");
		logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
		ws_broadcast_mud_shutdown("reboot");
		exit(52); /* what's so great about HHGTTG, anyhow? */
	}
	if (_copyover)
	{
		if (_autoboot)
		{
			logit(LOG_EXIT, "Auto reboot with copyover.");
			logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
		}
		else
		{
			logit(LOG_EXIT, "Copyover reboot.");
			logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
		}
		ws_broadcast_mud_shutdown("copyover");
		// attempt true copyover - if it returns, something went wrong
		copyover_save(mother_desc, mother_desc_ssl, ws_desc);
		// fallback to old behavior if copyover_save fails
		logit(LOG_EXIT, "Copyover failed, falling back to normal restart.");
		exit(_autoboot ? 57 : 53);
	}
	if (_autoboot)
	{
		logit(LOG_EXIT, "Auto reboot.");
		logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
		ws_broadcast_mud_shutdown("autoreboot");
		exit(54);
	}
	if (_pwipe)
	{
		logit(LOG_EXIT, "Pwipe Shutdown.");
		logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
		ws_broadcast_mud_shutdown("pwipe");
		exit(55);
	}
	ws_broadcast_mud_shutdown("manual");
	logit(LOG_EXIT, "Normal termination of game.");
	logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
	logit(LOG_STATUS, "Normal termination of game.");
}

/* Accept new connects, relay commands, and call 'heartbeat-functs' */

static void check_section_time(struct timespec *start, struct timespec *end, const char *name, int threshold)
{
  long elapsed = (end->tv_sec - start->tv_sec) * 1000 + (end->tv_nsec - start->tv_nsec) / 1000000;
  if (elapsed > threshold * 1000)
    logit(LOG_EXIT, "SLOW SECTION: %s took %ld ms", name, elapsed);
}

#define MAX_ACCEPTS_PER_PULSE 32

static int drain_new_connections(int listener, int conn_type, const char *label)
{
	int accepted_count = 0;

	for (int attempt = 0; attempt < MAX_ACCEPTS_PER_PULSE; attempt++)
	{
		if (new_descriptor(listener, conn_type) == 0)
		{
			accepted_count++;
			continue;
		}
		if (errno == EINTR)
		{
			attempt--;
			continue;
		}
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			logit(LOG_COMM, "%s accept failed: %s", label, strerror(errno));
		break;
	}
	return accepted_count;
}

void game_loop(int port, int sslport)
{
	P_char                t_ch = NULL;
	P_desc                point, next_point;
	char                  buf[MAX_STRING_LENGTH];
	char                  comm[MAX_INPUT_LENGTH];
	char                  Debug[MAX_INPUT_LENGTH];
	int                   player_count;
	static struct timeval opt_time;
	struct timeval        last_time, now, timespent, timeout, null_time;
	struct host_answer    host_ans_buf;
	struct ident_answer   ident_ans_buf;
	sigset_t              mask, oldset;
	int                   s, S;
	int                   WS; /* WebSocket listener socket */
	int                   accept_debug = getenv("DURIS_ACCEPT_DEBUG") != NULL;
	unsigned long         accept_debug_pulse = 0;

	sentbytes         = 0;
	recivedbytes      = 0;
	null_time.tv_sec  = 0;
	null_time.tv_usec = 0;

	opt_time.tv_usec = OPT_USEC; /* Init time values */
	opt_time.tv_sec  = 0;
#if 1
	gettimeofday(&last_time, (struct timezone *)0);
#endif
	pulse = 0;

#ifdef DO_SET_DTABLE_SIZE
	(void)setdtablesize(128);
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

	dead_desc_pool = mm_create("SOCKET", sizeof(struct descriptor_data), offsetof(struct descriptor_data, next), mm_find_best_chunk(sizeof(struct descriptor_data), 25, 110));

	// copyover recovery - pool must exist first
	if (copyover_boot)
	{
		if (copyover_recover(&recovered_mother_desc, &recovered_mother_desc_ssl, &recovered_ws_desc))
		{
			copyover_restore_combat();
			// recalculate avg mob level now that mobs are restored
			calc_zone_mob_level();
		}
	}

	// redis crash recovery - restore world state from redis snapshot
	if (crash_recovery_boot)
	{
		logit(LOG_STATUS, "Performing redis crash recovery...");
		if (redis_load_world_state())
		{
			copyover_restore_combat(); // reuse combat restoration logic
			calc_zone_mob_level();
			logit(LOG_STATUS, "Crash recovery complete");
		}
		else
		{
			logit(LOG_STATUS, "Crash recovery failed, will use normal boot state");
		}
		redis_clear_world_state();
		crash_recovery_boot = 0;
		// schedule world state saves now that recovery is done
		add_event(event_save_world_state, 30 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
	}

	PROFILES(RESET);
#ifdef DO_PROFILE
	init_func_call_info();
#endif

	// use recovered sockets if copyover, otherwise create new ones
	if (copyover_boot && recovered_mother_desc >= 0)
	{
		logit(LOG_STATUS, "Using recovered sockets from copyover");
		s  = recovered_mother_desc;
		S  = recovered_mother_desc_ssl;
		WS = recovered_ws_desc;
	}
	else if (copyover_boot)
	{
		// copyover mode but recovery failed - can't bind new sockets because old ones still open
		logit(LOG_STATUS, "FATAL: copyover recovery failed, cannot continue");
		exit(1);
	}
	else
	{
		logit(LOG_STATUS, "Opening mother connection.");
		s = init_socket(port);
		logit(LOG_STATUS, "Opening father connection.");
		S = init_socket(sslport);
		logit(LOG_STATUS, "Opening WebSocket connection.");
		WS = websocket_init(WS_PORT);
		if (WS < 0)
		{
			logit(LOG_STATUS, "WARNING: WebSocket server failed to start on port %d", WS_PORT);
		}
	}

	// store in file-scope statics for copyover access
	mother_desc     = s;
	mother_desc_ssl = S;
	ws_desc         = WS;
	copyover_boot   = 0;
	copyover_clear_boot();

	long    last_desc_per_hour_reset = time(0);
	clock_t loop_time_end;
	/* Main loop */
	while (!shutdownflag)
	{
		clock_t loop_time_begin = clock();
		// check for signal-initiated shutdown (from launcher)
		//PROFILE_START(process_signal_shutdown_pending);
		if (signal_shutdown_pending)
		{
			int type                = signal_shutdown_pending;
			signal_shutdown_pending = 0;
			request_shutdown(type, "Launcher", "signal from launcher");
		}
		//PROFILE_END(process_signal_shutdown_pending);
		checkpointing();

		if ((last_desc_per_hour_reset + 3600) <= time(0))
		{
			max_descs_this_hour = used_descs;
			last_desc_per_hour_reset = time(0);
		}
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
		input_set  = io_readfds;
		output_set = io_writefds;
		exc_set    = io_exceptfds;
#endif

		/* Continue with original code */
		PROFILE_START(connections);
		FD_SET(s, &input_set);
		FD_SET(S, &input_set);
		if (WS >= 0)
			FD_SET(WS, &input_set); /* WebSocket listener */
		for (point = descriptor_list; point; point = point->next)
		{
			/*
			 * while we are looping through descriptors, it would be a
			 * good time to see if the message answer we checked for
			 * before matches
			 */

			if ((point->descriptor == ident_ans_buf.desc) && (*((time_t *)(point->login + 4)) == ident_ans_buf.stamp))
				strcpy(point->login, ident_ans_buf.name);

			if ((point->descriptor == host_ans_buf.desc) && !strncmp(host_ans_buf.addr, point->host /*+ 3 */, strlen(host_ans_buf.addr)))
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
					                    "feel this is in error, please e-mail multiplay@newduris.com\r\n");
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
					point->wait      = 1;
				}
			}
			FD_SET(point->descriptor, &input_set);
			FD_SET(point->descriptor, &exc_set);
			FD_SET(point->descriptor, &output_set);
		}

		sigprocmask(SIG_SETMASK, &mask, &oldset);

		int select_result = select(FD_SETSIZE, &input_set, &output_set, &exc_set, &null_time);
		if (accept_debug && ((++accept_debug_pulse % 20) == 0 || FD_ISSET(s, &input_set)))
		{
			logit(LOG_STATUS,
			      "ACCEPT DEBUG: pulse=%lu listener=%d select_result=%d listener_ready=%d descriptors=%d",
			      accept_debug_pulse,
			      s,
			      select_result,
			      FD_ISSET(s, &input_set) ? 1 : 0,
			      used_descs);
		}
		if (select_result < 0)
		{
			perror("Select poll");
			// bad file descriptor - find and nuke it so we dont loop forever
			if (errno == EBADF)
			{
				struct descriptor_data *d, *next_d;
				for (d = descriptor_list; d; d = next_d)
				{
					next_d = d->next;
					if (fcntl(d->descriptor, F_GETFD) == -1 && errno == EBADF)
					{
						logit(LOG_STATUS, "ebadf: closing bad descriptor %d, host=%s, ws=%d, state=%d", d->descriptor, d->host ? d->host : "null", d->websocket, d->connected);
						close_socket(d);
					}
				}
			}
      		sigprocmask(SIG_SETMASK, &oldset, 0);
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
		(void)io_processFDS(&input_set, &output_set, &exc_set);
#endif

		/* Respond to whatever might be happening */

		/* Nonblocking accept is the authoritative readiness check. */
		drain_new_connections(s, 0, "Telnet");
		drain_new_connections(S, 1, "SSL");
		if (WS >= 0)
			drain_new_connections(WS, 2, "WebSocket");

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
			{
				if (point->connected != CON_SSLNEGO && process_input(point) < 0)
				{
					close_socket(point);
				}
			}
		}
		PROFILE_END(connections);
		double connections_time = (double)(connections_profile_end - connections_profile_beg) / (double)CLOCKS_PER_SEC;
		latency_trace_record("connections", (long)(connections_time * 1000000.0), pulse);

#if 0
    if (debug_mode)
      loop_debug();
#endif

		/* process_commands */
		PROFILE_START(commands);
		for (point = descriptor_list, player_count = 0; point; point = next_to_process)
		{
			next_to_process = point->next;
			t_ch            = point->character;

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

			/* wait for ttype response or timeout */
			if (point->connected == CON_TTYPE_NEGO)
			{
				if (point->ttype_state == TTYPE_COMPLETE || time(0) >= point->ttype_timeout)
				{
					greet(point);
				}
				else
					continue;
			}

			/* update max_users_playing for "who" information */
			if ((point->connected) == CON_PLAYING)
			{
				player_count++;
				if (player_count > max_users_playing)
					max_users_playing = player_count;
			}

			/* WebSocket ping/pong dead connection detection */
			if (point->websocket && point->ws_state == WS_STATE_OPEN)
			{
				time_t now = time(0);

				/* Check for ping timeout (no pong received) */
				if (point->ws_last_ping > 0 && !point->ws_pong_received && (now - point->ws_last_ping) > WS_PING_TIMEOUT)
				{
					statuslog(56, "WebSocket: Closing dead connection from %s (ping timeout)", point->host);
					websocket_close(point, WS_CLOSE_GOING_AWAY, "Ping timeout");
					close_socket(point);
					continue;
				}

				/* Send periodic ping */
				if (point->ws_last_ping == 0 || (now - point->ws_last_ping) >= WS_PING_INTERVAL)
				{
					websocket_send_ping(point);
					point->ws_last_ping     = now;
					point->ws_pong_received = 0; /* Reset for next cycle */
				}
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
			else if (IS_AFFECTED2(t_ch, AFF2_SLOW) && !IS_TRUSTED(t_ch) && (pulse % 2) && !GET_CLASS(t_ch, CLASS_MONK))
				continue;
			else if (affected_by_spell(t_ch, TAG_CTF) && !IS_TRUSTED(t_ch) && (pulse % (int)get_property("ctf.slowness", 3)))
				continue;

			/* check for hella long wait time here..  bandaid solution but it should (sort of) work */

			if ((!t_ch || (t_ch && (CAN_ACT(t_ch) && (!IS_SET(t_ch->specials.affected_by, AFF_CHARM) || (point->original))))) && get_from_q(&point->input, comm))
			{

				if (t_ch)
				{
					t_ch->specials.timer = 0;
				}
				point->prompt_mode = TRUE;

				if (point->showstr_count) /* pager for text */
					show_string(point, comm);
				else if (point->str) /* mail, boards */
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
		double commands_time = (double)(commands_profile_end - commands_profile_beg) / (double)CLOCKS_PER_SEC;
		latency_trace_record("commands", (long)(commands_time * 1000000.0), pulse);

		PROFILE_START(prompts);
		for (point = descriptor_list; point; point = next_point)
		{
			next_point = point->next;

			// this code tries to skip players who have too much pending text.
			// But, we currently boot them anyway...
			if (!FD_ISSET(point->descriptor, &output_set))
				continue;

			// skip ssl connections still negotiating
			if (point->connected == CON_SSLNEGO)
				continue;

			/* Drain WebSocket bytes retained after a partial write/EAGAIN before
			 * framing additional application output for this descriptor. */
			if (point->websocket && point->ws_output_offset < point->ws_output_len)
			{
				if (websocket_flush_output(point) < 0)
				{
					point->write_failed = 1;
					close_socket(point);
					continue;
				}
				if (point->ws_output_offset < point->ws_output_len)
					continue;
			}

			if (process_output(point) < 0)
			{
				close_socket(point);
				continue;
			}
		}

		PROFILE_END(prompts);
		double prompts_time = (double)(prompts_profile_end - prompts_profile_beg) / (double)CLOCKS_PER_SEC;
		latency_trace_record("prompts", (long)(prompts_time * 1000000.0), pulse);

		/* handle heartbeat stuff */
		/* Note: pulse now changes every 1/4 sec  */
		after_events_call = TRUE;
		/* Not using old events anymore.
		if (schedule[pulse])
		{
		  Events();
		}
		*/
		clock_t ne_events_begin = clock();
		ne_events();
		clock_t ne_events_end = clock();
		double ne_events_time = (double)(ne_events_end - ne_events_begin) / (double)CLOCKS_PER_SEC;
		latency_trace_record("ne_events", (long)(ne_events_time * 1000000.0), pulse);

		/* Flush dirty room GMCP updates every 2 pulses (~500ms) */
		if (!(pulse % 2))
		{
			clock_t _gmcp = clock();
			gmcp_flush_dirty_rooms();
			gmcp_flush_dirty_ship_contacts();
			gmcp_flush_dirty_ship_info();
			flush_pending_ship_saves();
			locker_async_pulse();
			latency_trace_record("gmcp_flush", (long)((clock() - _gmcp) * 1000000.0 / CLOCKS_PER_SEC), pulse);
		}

		PROFILE_START(activities);
		if (!(pulse % (WAIT_SEC * 60)))
			timers_activity();

		if (!(pulse % WAIT_SEC))
			ship_activity();

		if (!no_ferries && !(pulse % WAIT_SEC))
			ferry_activity();

		if (!(pulse % (WAIT_SEC * 60)))
			auction_houses_activity();

		if (!(pulse % (WAIT_SEC * 120)))
			spawn_random_mapmob();

		//    if (!(pulse % WAIT_SEC))
		//      arena_activity();

		if (!(pulse % SHORT_AFFECT))
			short_affect_update();

		if (!(pulse % PULSES_IN_TICK) && !mini_mode)
			web_info();

		if (!(pulse % (WAIT_SEC * 300)))
			wimps_in_approve_queue();

		if (!(pulse % (WAIT_SEC * 300)))
			poll_check_expirations();

		if (!(pulse % (WAIT_SEC * 120)))
		{
			epic_zone_balance();
		}

		if (!(pulse % (WAIT_SEC * 60)))
			sql_check_level_cap_periodic();

		if (!(pulse % (WAIT_SEC * 60)))
			boon_maintenance();

		PROFILE_END(activities);
		double activities_time = (double)(activities_profile_end - activities_profile_beg) / (double)CLOCKS_PER_SEC;
		latency_trace_record("activities", (long)(activities_time * 1000000.0), pulse);

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
				if (t_ch->desc && t_ch->desc->last_map_update)
				{
					// For ship passengers: GMCP only (handler.c already filters to GMCP-enabled only)
					if (IS_SHIP_ROOM(t_ch->in_room))
					{
						if (GMCP_ENABLED(t_ch))
						{
							P_ship ship = get_ship_from_char(t_ch);
							if (ship && IS_MAP_ROOM(ship->location))
							{
								int n = map_view_distance(t_ch, ship->location);
								if (n > 1)
								{
									// Render map and send via GMCP only (skip text by using websocket flag temporarily)
									bool was_websocket    = t_ch->desc->websocket;
									t_ch->desc->websocket = 1; // Force skip_text_output in display_map_room
									display_map_room(t_ch, ship->location, n, MAP_AUTOMAP, 0);
									t_ch->desc->websocket = was_websocket;
								}
							}
						}
					}
					else
					{
						map_look(t_ch, MAP_AUTOMAP);
					}
					t_ch->desc->last_map_update = 0;
				}
				if (t_ch->desc && t_ch->desc->last_group_update)
				{
					/* For GMCP clients, send structured data to group panel */
					if (GMCP_ENABLED(t_ch))
					{
						gmcp_send_group_status(t_ch);
					}
					/* For MSP clients, display text group output */
					if (t_ch->desc->term_type == TERM_MSP)
					{
						do_group(t_ch, "", 0);
					}
					t_ch->desc->last_group_update = 0;
				}
				if (t_ch->points.delay_move > 0)
					t_ch->points.delay_move -= BOUNDED(0, !IS_MAP_ROOM(t_ch->in_room) ? move_regen(t_ch, FALSE) : move_regen(t_ch, FALSE) / 2, t_ch->points.delay_move);
			}
		}
		//      }
		PROFILE_END(combat);
		double combat_time = (double)(combat_profile_end - combat_profile_beg) / (double)CLOCKS_PER_SEC;
		latency_trace_record("combat", (long)(combat_time * 1000000.0), pulse);

		PROFILE_START(pulse_reset);
		// tics since last checkpoint signal
		tics = tics + 1;
		if (tics > BIT_30)
		{
			tics = 1;
			debug("Huge value for tics, resetting to 1.");
			logit(LOG_SYS, "Huge value for tics, resetting to 1.");
		}
		pulse++;
		after_events_call = FALSE;
		clock_t affect_and_points_begin = clock();
		if (pulse >= PULSES_IN_TICK)
		{
			pulse = 0;
			
			affect_update();
			point_update();
		}
		clock_t affect_and_points_end = clock();
		double affect_and_points_time = (double)(affect_and_points_end - affect_and_points_begin) / (double)CLOCKS_PER_SEC;
		latency_trace_record("affect_and_points", (long)(affect_and_points_time * 1000000.0), pulse);
		/* check out the time */
#if 1
		loop_time_end = clock();
		double loop_time = (double)(loop_time_end - loop_time_begin) / (double)CLOCKS_PER_SEC;
		if (loop_time >= 0.250) // 4 ticks a sec
		{
			statuslog(56, "MUD TICK TOOK TOO LONG - loop time - %f", loop_time);
			statuslog(56, "  - connections time - %f", connections_time);
			statuslog(56, "  - activities time - %f", activities_time);
			statuslog(56, "  - combat time - %f", combat_time);
			statuslog(56, "  - commands time - %f", commands_time);
			statuslog(56, "  - ne_events time - %f", ne_events_time);
			statuslog(56, "  - prompts time - %f", prompts_time);
			statuslog(56, "  - aff/pts time - %f", affect_and_points_time);
		}
		latency_trace_record("total_tick", (long)(loop_time * 1000000.0), pulse);
		if (!(tics % 300)) { FILE *_ltf = fopen("/durismud/logs/latency_trace.log", "a"); if(_ltf){ latency_trace_dump(_ltf); fclose(_ltf); } latency_trace_dump(stderr); fflush(stderr); persistence_queue_latency_dump(); utility_latency_dump(); }
		memcpy(&timeout, &opt_time, sizeof(timeout));
		suseconds_t usec_spent = (suseconds_t)(loop_time * 1000 * 1000);
		timeout.tv_usec = MAX(0, timeout.tv_usec - usec_spent);

		if (timeout.tv_sec || timeout.tv_usec)
		{

			/*
			 * This keeps game from being a total processor hog by putting
			 * it to sleep for the part of each 1/4 second that is not
			 * used for game processing.
			 */

			sigprocmask(SIG_SETMASK, &mask, &oldset);

			if (select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &timeout) < 0)
			{
				sigprocmask(SIG_SETMASK, &oldset, 0);
				if (errno == EINTR)
					continue; // interrupted by signal, just retry
				perror("Select sleep");
				continue;
			}
			sigprocmask(SIG_SETMASK, &oldset, 0);
		}
		gettimeofday(&last_time, (struct timezone *)0); /* end of pulse reset */
#endif
		PROFILE_END(pulse_reset);
	}

	PROFILES(SAVE);
#ifdef DO_PROFILE
	save_func_call_info();
#endif

	// Don't want to save stuff just after we wiped all the tables in SQL.
	if (!_pwipe)
	{
		save_dirty_shopkeepers();
		flush_pending_ship_saves();
		locker_async_drain(2000);

		if (no_ferries == 0)
		{
			shutdown_ferries();
		}

		shutdown_ships();

		shutdown_auction_houses();

		Guildhall::shutdown();
	}

	// skip character extraction during copyover - we need them intact
	if (!_copyover && !_pwipe)
	{
		persistence_flush_all_character_saves();
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
						if (IS_DESTROYING(point->character))
							stop_destroying(point->character);
						un_morph(point->character);
					}
					if (shutdown_message)
					{
						write_to_descriptor(point, shutdown_message);
					}
					if (!_pwipe)
					{
						write_to_descriptor(point, "\r\nSaving...\r\n");
				if (!do_save_silent(point->character, 3))
				logit(LOG_STATUS, "Failed to save %s during shutdown.", GET_NAME(point->character));
					}
					// If it's not an immortal.
					if (GET_LEVEL(point->character) < MINLVLIMMORTAL)
					{
						update_ingame_racewar(-GET_RACEWAR(point->character));
					}
					extract_char(point->character);
				}
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
	unsigned int      txtlen, taillen;

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
	queue->tail       = n_new;
	n_new->next       = NULL;
}

/*
 * if b > a returns 0 secs, 0 usecs
 */

struct timeval timediff(struct timeval *a, struct timeval *b)
{
	static struct timeval rslt;

	rslt.tv_sec  = a->tv_sec - b->tv_sec;
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
		rslt.tv_sec  = 0;
	}
	return (rslt);
}

/*
 * Empty the queues before closing connection
 */

void flush_queues(P_desc d)
{
	char str[MAX_STRING_LENGTH];

	while (get_from_q(&d->output, str))
		;
	while (get_from_q(&d->input, str))
		;
}

int wizconnectsite(char *name, char *player, int flag)
{
	struct wizban_t *tmp;
	char             buf[MAX_INPUT_LENGTH];
	char             buff[MAX_INPUT_LENGTH];
	int              i;

	if (name == NULL)
		return FALSE;

	/*
	 * lowercase the name string, since strstr is case sensitive
	 */
	for (i = 0; *(name + i) != '\0'; i++)
		buf[i] = LOWER(*(name + i));
	buf[i] = 0; /*
	             * to terminate buf
	             */
	/*
	 * lowercase the name string, since strstr is case sensitive
	 */
	for (i = 0; *(player + i) != '\0'; i++)
		buff[i] = LOWER(*(player + i));
	buff[i] = 0; /*
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
	char          buf[MAX_INPUT_LENGTH];
	int           i;

	if (name == NULL)
		return FALSE;

	/*
	 * lowercase the name string, since strstr is case sensitive
	 */
	for (i = 0; *(name + i) != '\0'; i++)
		buf[i] = LOWER(*(name + i));
	buf[i] = 0; /*
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

#if 0 /*                                                                                                                                                                                               \
       * old socket routines.  JAB                                                                                                                                                                     \
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

#else /*                                                                                                                                                                                               \
       * old/new socket code. JAB                                                                                                                                                                      \
       */

int init_socket(int port)
{
	int                s, bind_error;
	char              *opt;
	char               hostname[MAX_HOSTNAME + 1];
	struct sockaddr_in sa;
	struct hostent    *hp;
	int                value = 1;
	struct linger      linger_values;

	/*
	 * struct linger ld;
	 */
	int buffsize, buffer;

	linger_values.l_onoff  = 0;
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
	sa.sin_port   = htons((unsigned short int)port);
#ifdef IPPROTO_MPTCP
	/*
	 * Multipath TCP: if there are multiple routes available and enabled, they
	 * will be used together.  In our case (hardly any bandwidth used), the
	 * worse route will be kept on standby, to be used when lag happens.
	 *
	 * The kernel silently falls back to non-MPTCP extremely fast, thus broken
	 * routers or middleware rejecting packets with a flag they don't know
	 * doesn't require a retry from us.  Thus, the only concerns are platforms
	 * that don't support MPTCP (Windows, old BSDs) or have CONFIG_MPTCP=n.
	 */
	s             = socket(PF_INET, SOCK_STREAM, IPPROTO_MPTCP);
	if (s < 0)
#endif
	s             = socket(PF_INET, SOCK_STREAM, 0);
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
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger_values, sizeof(linger_values)) < 0)
	{
		logit(LOG_EXIT, "setsockopt REUSEADDR");
		exit(1);
	}
	buffsize = sizeof(int);

	if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&buffer, (socklen_t *)&buffsize))
	{
		logit(LOG_EXIT, "getsockopt SNDBUF");
		exit(1);
	}
	if (buffer < MIN_SOCKET_BUFFER_SIZE)
	{
		buffer = MIN_SOCKET_BUFFER_SIZE;
		if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&buffer, sizeof(buffer)) < 0)
		{
			logit(LOG_EXIT, "setsockopt SNDBUF");
			exit(1);
		}
	}
	if ((bind_error = (bind(s, (struct sockaddr *)&sa, sizeof(sa))) < 0))
	{
		logit(LOG_EXIT, "bind error %d", bind_error);
		close(s);
		exit(1);
	}
	if (listen(s, SOMAXCONN) < 0)
	{
		logit(LOG_EXIT, "listen failed");
		close(s);
		exit(1);
	}
	nonblock(s);
	return (s);
}

int new_connection(int s)
{
	struct sockaddr_in isa;
	int                i;
	int                t;

	i = sizeof(isa);
	getsockname(s, (struct sockaddr *)&isa, (socklen_t *)&i);

	if ((t = accept(s, (struct sockaddr *)&isa, (socklen_t *)&i)) < 0)
		return (-1);
	nonblock(t);
	i = 1;
	setsockopt(t, SOL_TCP, TCP_NODELAY, &i, sizeof(i));

	// increase send buffer
	i = 65536;
	setsockopt(t, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i));

	return (t);
}

/*
 * Check if a descriptor is valid (exists in descriptor_list)
 * Used to detect dangling pointers to freed/closing descriptors
 */
int is_desc_valid(struct descriptor_data *desc)
{
	struct descriptor_data *d;

	if (!desc)
		return 0;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d == desc)
			return 1;
	}
	return 0;
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
	snoop_by_data          *snoop_by_ptr, *next;
	int                     is_morphed = d->character ? IS_MORPH(d->character) : 0;
	char                    Gbuf1[MAX_STRING_LENGTH];
	time_t                  ct;

	compress_end(d, TRUE); /* does flushing out all output break anything ? */

	/* clean up poll wizard session if active */
	if (d->character && poll_wizard_active(d->character))
		poll_wizard_cancel(d->character);

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
			send_to_char("Your host has lost link... you can no longer maintain the sight link.\r\n", snoop_by_ptr->snoop_by);
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

		if (d->character && (d->connected == CON_PLAYING) && (GET_LEVEL(d->character) < 58))
			send_to_char("&+CYou are no longer being snooped.&N\r\n", d->snoop.snooping);
		/*    d->snoop.snooping->desc->snoop.snoop_by = 0;*/
		if (is_morphed)
		{
			act("&+B$n has lost $s link and is unable to maintain $s part of the spell!&n", FALSE, d->character, 0, d->snoop.snooping, TO_VICT);
			un_morph(d->snoop.snooping);
		}
		if (d->snoop.snooping)
		{
			rem_char_from_snoopby_list(&d->snoop.snooping->desc->snoop.snoop_by_list, d->character);
			d->snoop.snooping = 0;
		}
	}
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
	d->str     = NULL;
	d->backstr = NULL;
	/* Gee, that wasn't so tough now, was it? */

	if (d->character)
	{
		if (d->connected == CON_PLAYING)
		{
			sql_disconnectIP(d->character);
			redis_player_offline(d->character);
			act("$n has lost $s link.", TRUE, GET_PLYR(d->character), 0, 0, TO_ROOM);
			if ((NumAttackers(d->character) > 0) && !IS_TRUSTED(d->character))
			{
				logit(LOG_COMM, "Combat DropLink: %s [%s].", GET_NAME(GET_PLYR(d->character)), d->host);
				statuslog(56, "Combat DropLink: %s [%s].", GET_NAME(GET_PLYR(d->character)), d->host);
			}
			else
			{
				logit(LOG_COMM, "Closing link to: %s [%s].", GET_NAME(GET_PLYR(d->character)), d->host);
				// Subtract 5 hrs: GMT -> EST.
				ct = time(0) - 5 * 60 * 60;
				snprintf(Gbuf1, MAX_STRING_LENGTH, "%s", asctime(localtime(&ct)));
				*(Gbuf1 + strlen(Gbuf1) - 1) = '\0';
				loginlog(d->character->player.level, "%s [%s] has lost link @ %s EST.", GET_NAME(GET_PLYR(d->character)), d->host, Gbuf1);
				sql_log(d->character, CONNECTLOG, "Lost Link");
			}
			persistence_flush_character_saves(d->character);

			// Wrap final save in transaction (flush already completed above)
			if (sql_begin_transaction())
			{
				writeCharacter(d->character, RENT_CRASH, d->character->in_room);
				if (!sql_commit())
				{
					logit(LOG_DEBUG, "close_socket: commit failed for %s", GET_NAME(d->character));
					sql_rollback();
				}
			}
			else
			{
				// still try to save even if transaction start fails
				writeCharacter(d->character, RENT_CRASH, d->character->in_room);
			}
			d->character->desc = 0;
		}
		else
		{
			logit(LOG_COMM, "Losing player: %s [%s].", GET_NAME(d->character), d->host);
			free_char(d->character);
			d->character = NULL;
		}
	}
	else
		logit(LOG_COMM,
		      "Losing descriptor without char [host=%s desc=%d connected=%d ssl=%s].",
		      d->host ? d->host : "unknown",
		      d->descriptor,
		      d->connected,
		      d->sslses ? "yes" : "no");

	if (next_to_process == d)
		next_to_process = next_to_process->next;
	if (d == descriptor_list)
		descriptor_list = descriptor_list->next;
	else
	{
		/*
		 * Locate the previous element
		 */
		for (tmp = descriptor_list; tmp && (tmp->next != d); tmp = tmp->next)
			;
		if (tmp)
			tmp->next = d->next;
	}

	if (d->descriptor)
		shutdown(d->descriptor, 2);

	if (d->showstr_head)
	{
		FREE(d->showstr_head);
	}
#ifdef I_REALLY_WANT_TO_CRASH_THE_GAME
	if (d->showstr_point)
	{
#ifdef MEM_DEBUG
		mem_use[MEM_STRINGS] -= strlen(d->showstr_point);
#endif
		FREE(d->showstr_point);
	}

	if (d->showstr_count)
#ifdef MEM_DEBUG
		mem_use[MEM_STRINGS] -= strlen(d->showstr_vector);
#endif
	FREE(d->showstr_vector);

	if (d->storage)
		FREE(d->storage);

#endif
	/* I really don't wanna crash it  */
#ifdef USE_ACCOUNT
	if (d->account)
		d->account = free_account(d->account);
#endif

	/* Free WebSocket fragment buffer if any */
	websocket_free(d);

	if (d)
	{
#if 0
#ifdef MEM_DEBUG
    mem_use[MEM_DESC] -= sizeof(struct descriptor_data);
#endif
    FREE((char *) d);
#endif
		mm_release(dead_desc_pool, d);
	}
}

void nonblock(int s)
{
	int flags;

	flags = fcntl(s, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(s, F_SETFL, flags) < 0)
	{
		logit(LOG_EXIT, "Nonblock");
		exit(1);
	}
}

#endif /*                                                                                                                                                                                              \
        * old/new socket code. 9/18/95  JAB                                                                                                                                                            \
        */

/* parse proxy protocol v1 header - returns 1 if found, stores real ip */
static int parse_proxy_protocol(int desc, char *real_ip, size_t ip_len)
{
	char    buf[108];
	char    proto[8], src_ip[46], dst_ip[46];
	int     src_port, dst_port;
	ssize_t n;
	int     i;

	/* peek first 6 bytes to check for "PROXY " */
	n = recv(desc, buf, 6, MSG_PEEK);
	if (n < 6 || strncmp(buf, "PROXY ", 6) != 0)
		return 0;

	/* read full line up to \r\n */
	for (i = 0; i < (int)sizeof(buf) - 1; i++)
	{
		n = recv(desc, &buf[i], 1, 0);
		if (n <= 0)
			return 0;
		if (buf[i] == '\n')
		{
			buf[i + 1] = '\0';
			break;
		}
	}
	buf[i] = '\0';

	if (sscanf(buf, "PROXY %7s %45s %45s %d %d", proto, src_ip, dst_ip, &src_port, &dst_port) == 5)
	{
		strncpy(real_ip, src_ip, ip_len - 1);
		real_ip[ip_len - 1] = '\0';
		return 1;
	}

	return 0;
}

struct hostname_lookup_request
{
	char address[INET6_ADDRSTRLEN];
	int  descriptor;
};

#define MAX_HOSTNAME_LOOKUP_WORKERS 8
static pthread_mutex_t hostname_lookup_mutex = PTHREAD_MUTEX_INITIALIZER;
static int hostname_lookup_workers = 0;

static void *hostname_lookup_worker(void *arg)
{
	struct hostname_lookup_request *request = (struct hostname_lookup_request *)arg;
	struct addrinfo hints, *result = NULL;
	char hostname[NI_MAXHOST];
	char temp_path[128], final_path[128];
	FILE *f;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST;

	if (getaddrinfo(request->address, NULL, &hints, &result) == 0)
	{
		if (getnameinfo(result->ai_addr, result->ai_addrlen, hostname, sizeof(hostname), NULL, 0, NI_NAMEREQD) == 0)
		{
			snprintf(temp_path,
			         sizeof(temp_path),
			         "lib/etc/hosts/.%d.%s.%lu.tmp",
			         request->descriptor,
			         request->address,
			         (unsigned long)pthread_self());
			snprintf(final_path, sizeof(final_path), "lib/etc/hosts/%d.%s", request->descriptor, request->address);
			f = fopen(temp_path, "w");
			if (f != NULL)
			{
				int write_ok = fprintf(f, "%s\n", hostname) >= 0;
				int close_ok = fclose(f) == 0;
				if (write_ok && close_ok)
					rename(temp_path, final_path);
			}
		}
		freeaddrinfo(result);
	}

	pthread_mutex_lock(&hostname_lookup_mutex);
	hostname_lookup_workers--;
	pthread_mutex_unlock(&hostname_lookup_mutex);
	free(request);
	return NULL;
}

void resolve_descriptor_hostname_async(const char *address, int descriptor)
{
	struct hostname_lookup_request *request;
	pthread_t thread;
	pthread_attr_t attr;

	request = (struct hostname_lookup_request *)calloc(1, sizeof(*request));
	if (request == NULL)
		return;

	pthread_mutex_lock(&hostname_lookup_mutex);
	if (hostname_lookup_workers >= MAX_HOSTNAME_LOOKUP_WORKERS)
	{
		pthread_mutex_unlock(&hostname_lookup_mutex);
		free(request);
		return;
	}
	hostname_lookup_workers++;
	pthread_mutex_unlock(&hostname_lookup_mutex);

	strncpy(request->address, address, sizeof(request->address) - 1);
	request->descriptor = descriptor;
	{
		char stale_path[128];
		snprintf(stale_path, sizeof(stale_path), "lib/etc/hosts/%d.%s", descriptor, request->address);
		unlink(stale_path);
	}
	if (pthread_attr_init(&attr) != 0)
	{
		pthread_mutex_lock(&hostname_lookup_mutex);
		hostname_lookup_workers--;
		pthread_mutex_unlock(&hostname_lookup_mutex);
		free(request);
		return;
	}
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&thread, &attr, hostname_lookup_worker, request) != 0)
	{
		pthread_mutex_lock(&hostname_lookup_mutex);
		hostname_lookup_workers--;
		pthread_mutex_unlock(&hostname_lookup_mutex);
		free(request);
	}
	pthread_attr_destroy(&attr);
}

int new_descriptor(int s, int conn_type)
{
	P_desc             newd;
	bool               flag = FALSE, found = FALSE, looking_up = FALSE;
	char               Gbuf1[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];
	int                desc, size;
	struct sockaddr_in sock;
	FILE              *f;
	gnutls_session_t   sslses = 0;

	if ((desc = new_connection(s)) < 0)
		return (-1);

	if (desc >= FD_SETSIZE)
	{
		logit(LOG_COMM, "Accepted descriptor %d exceeds FD_SETSIZE %d; closing connection.", desc, FD_SETSIZE);
		shutdown(desc, 2);
		close(desc);
		return (0);
	}

	/* SSL connection - initialize TLS */
	if (conn_type == 1 && !(sslses = ssl_new(desc)))
	{
		shutdown(desc, 2);
		close(desc);
		return 0; // can legitimately fail if client sends garbage
	}

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
	
	if (used_descs > max_descs_this_hour)
		max_descs_this_hour = used_descs;

#if 0
#ifdef MEM_DEBUG
  mem_use[MEM_DESC] += sizeof(struct descriptor_data);
#endif
  CREATE(newd, struct descriptor_data, 1);
#endif
	newd = (struct descriptor_data *)mm_get(dead_desc_pool);
	bzero(newd, sizeof(struct descriptor_data));

	/*
	 * find info
	 */
	size = sizeof(sock);

	if (getpeername(desc, (struct sockaddr *)&sock, (socklen_t *)&size) < 0)
	{
		perror("getpeername");
		strcpy(Gbuf1, "&+RUNTRACEABLE&n");
		flag = TRUE;
	}
	else
	{

		snprintf(Gbuf1,
		         MAX_STRING_LENGTH,
		         "%d.%d.%d.%d",
		         ((unsigned char *)&(sock.sin_addr))[0],
		         ((unsigned char *)&(sock.sin_addr))[1],
		         ((unsigned char *)&(sock.sin_addr))[2],
		         ((unsigned char *)&(sock.sin_addr))[3]);

		/* check for proxy protocol on websocket connections */
		if (conn_type == 2)
		{
			char proxy_ip[46];
			if (parse_proxy_protocol(desc, proxy_ip, sizeof(proxy_ip)))
				strncpy(Gbuf1, proxy_ip, MAX_STRING_LENGTH - 1);
		}

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
#ifdef MEM_DEBUG
        mem_use[MEM_DESC] -= sizeof(struct descriptor_data);
#endif
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
			buf.desc  = desc;
			snprintf(buf.addr,
			         MAX_STRING_LENGTH,
			         "%d.%d.%d.%d",
			         ((unsigned char *)&(sock.sin_addr))[0],
			         ((unsigned char *)&(sock.sin_addr))[1],
			         ((unsigned char *)&(sock.sin_addr))[2],
			         ((unsigned char *)&(sock.sin_addr))[3]);

#if 0
      if (msgsnd(ipc_id, (struct msgbuf *) &buf,
                 sizeof(struct host_request) - sizeof(long), 0) == -1)
      {
        /*
         * msgsnd() failed... DAMN!  for now, just segfault
         */
			panic_corruption("comm", "msgsnd failed");
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
	{ /* simulate it anyway */
		struct host_answer buf;

		buf.mtype = MSG_HOST_ANS;
		buf.desc  = desc;
		/*
		 * note that I put in a "." in the front of the addr field. That
		 * will signal the "reciver" that this name already occurs in the
		 * lookup list.
		 */
		snprintf(buf.addr,
		         MAX_STRING_LENGTH,
		         ".%d.%d.%d.%d",
		         ((unsigned char *)&(sock.sin_addr))[0],
		         ((unsigned char *)&(sock.sin_addr))[1],
		         ((unsigned char *)&(sock.sin_addr))[2],
		         ((unsigned char *)&(sock.sin_addr))[3]);

		strcpy(buf.name, Gbuf1);
#if 0
    if (msgsnd(ipc_id, (struct msgbuf *) &buf,
               sizeof(struct host_answer) - sizeof(long), 0) == -1)
    {
      /*
       * msgsnd() failed... DAMN!  for now, just segfault
       */
			panic_corruption("comm", "msgsnd failed");
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
	// newd->connected = CON_HOST_LOOKUP;
	newd->wait = 1;
	strncpy(newd->host, Gbuf1, 50);
	resolve_descriptor_hostname_async(strip_ansi(newd->host).c_str(), desc);
	*newd->host2              = '\0';
	newd->prompt_mode         = FALSE;
	*newd->buf                = '\0';
	newd->str                 = 0;
	newd->showstr_head        = 0;
	newd->showstr_vector      = 0;
	newd->showstr_count       = 0;
	*newd->last_input         = '\0';
	newd->output.head         = NULL;
	newd->input.head          = NULL;
	newd->next                = descriptor_list;
	newd->character           = 0;
	newd->original            = 0;
	newd->snoop.snooping      = 0;
	newd->snoop.snoop_by_list = 0;
	newd->tmp_val             = 0; /*
	                                * SAM 7-94
	                                */
	newd->confirm_state = 0;       /*
	                                * SAM 7-94
	                                */
	strcpy(newd->login, " ? ");
	newd->editor       = NULL;
	newd->out_compress = MCCP_NONE;
	newd->z_str        = NULL;
	newd->sslses       = sslses;
	*newd->client_str  = '\0';
	newd->term_type    = TERM_ANSI;

	/* WebSocket connection - set flags and wait for HTTP upgrade */
	if (conn_type == 2)
	{
		int ws_opt               = 1;
		newd->websocket          = 1;
		newd->ws_state           = 0; /* WS_STATE_CONNECTING */
		newd->ws_handshake_done  = 0;
		newd->ws_fragment_buffer = NULL;
		newd->ws_fragment_len    = 0;
		newd->gmcp_enabled       = 1; /* WebSocket clients always get GMCP */
		/* WebSocket needs non-blocking I/O and low latency */
		fcntl(desc, F_SETFL, O_NONBLOCK);
		setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, &ws_opt, sizeof(ws_opt));
	}

	descriptor_list = newd;

	if (conn_type == 1) // ssl - always use CON_SSLNEGO, let game loop handle greet
	{
		ssl_negotiate(sslses); // do first round immediately
		STATE(newd) = CON_SSLNEGO;
	}
	else if (conn_type == 2)
		STATE(newd) = CON_GET_TERM; /* WebSocket waits for HTTP handshake */
	else
	{
		/* start ttype negotiation before greeting */
		ttype_negotiate(newd);
		STATE(newd)         = CON_TTYPE_NEGO;
		newd->ttype_timeout = time(0) + 5; /* 5 second timeout */
	}

	return 0;
}

static void greet(P_desc newd)
{
	check_cp437(newd);
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
		// flush_queues(newd);
		return;
	}

	select_terminal(newd, "");

	advertise_mccp(newd);
	gmcp_negotiate(newd);
	/* sga disabled - causes ^? ^M on raw telnet in character mode */
	/* sga_negotiate(newd); */
}

/*
 * Return index into color_table. (Ithor) **  Did not use toupper,
 * because it tends to screw up on some machines. (mine)
 */

int find_color_entry(int c)
{
	int  i = 0;
	char s;

	s = UPPER(c);

	while ((color_table[i].symbol != NULL) && (*color_table[i].symbol != s))
		i++;

	return i;
}

// LATENT: assumes callers pass a buffer of at least MAX_STRING_LENGTH;
// no size parameter to enforce. Safe because all callers use prompt_buf
// which is now MAX_STRING_LENGTH. Would need a size_t param to harden.
void append_prompt(P_char ch, char *promptbuf)
{
	char   t_buf[512];
	P_char t_ch_f;
	P_obj  t_obj_f;
	P_char tank;
	int    percent = 0;

	if (!ch)
		return;

	if (!IS_TRUSTED(ch) && (ch->desc->connected == CON_PLAYING || ch->desc->connected == CON_MAIN_MENU))
	{
		;
	}
	else
	{
		return;
	}

	if (ch)
	{
		t_ch_f  = GET_OPPONENT(ch);
		t_obj_f = ch->specials.destroying_obj;
	}

	if (IS_NPC(ch))
		return;

	strcat(promptbuf, "\n&+g<");
	if (GET_MAX_HIT(ch) > 0)
		percent = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
	else
		percent = -1;

	if (percent >= 66)
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g %dh", ch->points.hit);
	}
	else if (percent >= 33)
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+y %dh", ch->points.hit);
	}
	else if (percent >= 15)
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+r %dh", ch->points.hit);
	}
	else
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+R %dh", ch->points.hit);
	}
	snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g/%dH", GET_MAX_HIT(ch));

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
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g %dv", ch->points.vitality);
	}
	else if (percent >= 33)
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+y %dv", ch->points.vitality);
	}
	else
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+r %dv", ch->points.vitality);
	}
	snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g/%dV", GET_MAX_VITALITY(ch));

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
		if ((tank = GET_OPPONENT(t_ch_f)) && (ch->in_room == tank->in_room))
		{
			snprintf(promptbuf + strlen(promptbuf),
			         MAX_STRING_LENGTH - strlen(promptbuf),
			         " &+BT: %s",
			         (ch != tank && !CAN_SEE(ch, tank)) ? "someone" : (IS_PC(tank) ? PERS(tank, ch, 0) : (FirstWord(GET_NAME(tank)))));
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

			snprintf(promptbuf + strlen(promptbuf),
			         MAX_STRING_LENGTH - strlen(promptbuf),
			         " &+rE: %s&+g",
			         (!CAN_SEE(ch, t_ch_f)) ? "someone" : (IS_PC(t_ch_f) ? PERS(t_ch_f, ch, 0) : (FirstWord((t_ch_f)->player.name))));
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
	if (t_obj_f)
	{
		strcat(promptbuf, "&+g< &+rE: ");
		strcat(promptbuf, t_obj_f->short_description);
		strcat(promptbuf, "&+g >&n\n");
	}
}

void write_to_pc_log(P_char ch, const char *message, int log)
{
	if (!ch)
		return;

	ch = GET_PLYR(ch);

	if (!ch || !IS_PC(ch) || !IS_ALIVE(ch))
	{
		return;
	}

	if (!GET_PLAYER_LOG(ch))
	{
		initialize_logs(ch, false);
	}

	if (!GET_PLAYER_LOG(ch))
	{
		logit(LOG_DEBUG, "Reloaded player log (%s) in write_to_pc_log(), but still not loaded.", GET_NAME(ch));
		debug("Reloaded player log (%s) in write_to_pc_log(), but still not loaded.", GET_NAME(ch));
		return;
	}

	if (log < 0 || log >= NUM_LOGS)
	{
		logit(LOG_DEBUG, "Invalid log (%d) in write_to_pc_log()", GET_NAME(ch));
		debug("Invalid log (%d) in write_to_pc_log()", GET_NAME(ch));
		return;
	}

	GET_PLAYER_LOG(ch)->write(log, message);
}

void initialize_logs(P_char ch, bool reset_logs)
{
	if (!ch || !IS_PC(ch))
		return;

	if (!reset_logs && GET_PLAYER_LOG(ch))
	{
		logit(LOG_DEBUG, "Tried to initialize player log (%s) in initialize_logs(), but was not null!", GET_NAME(ch));
		return;
	}

	if (reset_logs)
	{
		clear_logs(ch);
	}

	GET_PLAYER_LOG(ch) = new PlayerLog;
}

void clear_logs(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;

	if (!GET_PLAYER_LOG(ch))
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
	char           buf[MAX_STRING_LENGTH + 1], buffer[MAX_STRING_LENGTH + 1];
	char           buf2[MAX_STRING_LENGTH];
	bool           bold = FALSE, blink = FALSE;
	int            ibuf = 0;
	int            i, j, k, bg = 0;
	snoop_by_data *snoop_by_ptr;
	P_char         realChar = t->original ? t->original : t->character;
	string         descbuf;

	bool text = t->output.head;

	if (text && STATE(t) == CON_PLAYING && IS_PC(realChar) && ((t->prompt_mode == (PLR_FLAGGED(realChar, PLR_SMARTPROMPT)) || (t->prompt_mode != PLR_FLAGGED(realChar, PLR_OLDSMARTP)))))
	{
		if (!t->snoop.snooping || !t->snoop.snooping->desc || !t->snoop.snooping->desc->prompt_mode)
			descbuf += "\r\n";
	}

	if (text && !t->connected && t->character && (IS_PC(t->character) || IS_MORPH(t->character)) && !IS_SET(GET_PLYR(t->character)->specials.act, PLR_COMPACT))
	{
		write_to_q("\r\n", &t->output, 1);
	}

#ifdef SMART_PROMPT
	if (t->character && (IS_PC(t->character) || IS_MORPH(t->character)))
	{
		if (IS_SET(GET_PLYR(t->character)->specials.act, PLR_OLDSMARTP) && !t->showstr_count && !t->str && !IS_FIGHTING(GET_PLYR(t->character)))
		{
			t->prompt_mode = FALSE;
		}
		else if (!IS_SET(GET_PLYR(t->character)->specials.act, PLR_SMARTPROMPT) && text)
		{
			t->prompt_mode = TRUE;
		}
	}
#endif

	bool had_prompt = t->prompt_mode; // track if prompt will be sent
	make_prompt(t);

	/* Cycle thru output queue */
	while (get_from_q(&t->output, buf))
	{
#if 0
    if( PLR_FLAGGED(realChar, PLR_SMARTPROMPT) )
      format_text(buf, 1, t, MAX_STRING_LENGTH);
#endif

		if ((snoop_by_ptr = t->snoop.snoop_by_list) != NULL)
		{
			format_to_snoopers(buf, buf2);
		}
		while (snoop_by_ptr)
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
		for (i = 0, j = 0; (i < ibuf) && (j < (sizeof(buffer))); i++)
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
						snprintf(&buffer[j], MAX_STRING_LENGTH, "\033[0m");
						j += 4;
						was_upper = FALSE;
						break;
					case 'L':
						snprintf(&buffer[j], MAX_STRING_LENGTH, "\r\n");
						j += 2;
						break;
					case '+':
					case '-':
						bg = (buf[i] == '-');
						i++;
						if (i >= ibuf)
							continue;

						bold  = bg ? 0 : (isupper(buf[i])) ? 1 : 0;
						blink = !bg ? 0 : (isupper(buf[i])) ? 1 : 0;
						k     = find_color_entry(buf[i]);
						if (color_table[k].symbol != NULL)
						{
							if (isupper(buf[i]))
								was_upper = TRUE;
							else if (was_upper)
							{
								snprintf(&buffer[j], MAX_STRING_LENGTH, "\033[0m");
								j += 4;
								was_upper = FALSE;
							}
							snprintf(&buffer[j],
							         MAX_STRING_LENGTH,
							         "\033[%s%s%sm",
							         bold ? "1;" : "",
							         blink ? (t->character && (PLR3_FLAGGED(t->character, PLR3_UNDERLINE)) ? "4;" : "5;") : "",
							         (bg ? color_table[k].bg_code : color_table[k].fg_code));
							j += (5 + (bold ? 2 : 0) + (blink ? 2 : 0));
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
						bg    = find_color_entry(buf[i]);
						i++;
						if (i >= ibuf)
							continue;

						bold = (isupper(buf[i]) ? 1 : 0);
						k    = find_color_entry(buf[i]);
						if ((color_table[k].symbol != NULL) && (color_table[bg].symbol != NULL))
						{
							if (isupper(buf[i]))
								was_upper = TRUE;
							else if (was_upper)
							{
								snprintf(&buffer[j], MAX_STRING_LENGTH, "\033[0m");
								j += 4;
								was_upper = FALSE;
							}
							snprintf(&buffer[j],
							         MAX_STRING_LENGTH,
							         "\033[%s%s%s;%sm",
							         bold ? "1;" : "",
							         blink ? (t->character && (PLR3_FLAGGED(t->character, PLR3_UNDERLINE)) ? "4;" : "5;") : "",
							         color_table[bg].bg_code,
							         color_table[k].fg_code);
							j += (8 + (bold ? 2 : 0) + (blink ? 2 : 0));
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
			else if (buf[i] == '\n')
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

		descbuf += buffer;
	}

	if (write_to_descriptor(t, descbuf.c_str()) < 0)
		return (-1);

	if (had_prompt && !t->connected)
		if (send_ga(t) < 0)
			return (-1);

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
	int            thisround, begin;
	char           tmp[MAX_INPUT_LENGTH + 3], *buf, *bp;

	/* WebSocket connections use their own input processing */
	if (t->websocket)
	{
		return websocket_process_input(t);
	}

	begin = t->buflen;
	if (begin < 0 || begin >= MAX_QUEUE_LENGTH)
		panic_corruption("comm", "process_input: invalid buffer length %d", begin);
	buf = t->buf;

	/*
	 * Read in some stuff
	 */
	if (t->sslses)
	{
		thisround = gnutls_record_recv(t->sslses, buf + begin, MAX_QUEUE_LENGTH - begin - 1);
		if (!thisround)
		{
			logit(LOG_COMM,
			      "EOF encountered on socket read for %s [host=%s desc=%d connected=%d ssl=%s].",
			      (t->character) ? GET_NAME(t->character) : "NOCHAR",
			      t->host ? t->host : "unknown",
			      t->descriptor,
			      t->connected,
			      t->sslses ? "yes" : "no");
			return (-1);
		}
		else if (thisround < 0)
		{
			if (thisround != GNUTLS_E_AGAIN && thisround != GNUTLS_E_INTERRUPTED)
			{
				logit(LOG_COMM, "process_input() CON_%d %s Read: %d Error: %s", t->connected, (t->character) ? GET_NAME(t->character) : "", thisround, gnutls_strerror(thisround));
				return (-1);
			}
			return 0;
		}
	}
	else
	{
		thisround = read(t->descriptor, buf + begin, MAX_QUEUE_LENGTH - begin - 1);
		if (!thisround)
		{
			logit(LOG_COMM,
			      "EOF encountered on socket read for %s [host=%s desc=%d connected=%d ssl=%s].",
			      (t->character) ? GET_NAME(t->character) : "NOCHAR",
			      t->host ? t->host : "unknown",
			      t->descriptor,
			      t->connected,
			      t->sslses ? "yes" : "no");
			return (-1);
		}
		else if (thisround < 0)
		{
			if (errno != EAGAIN)
			{
				logit(LOG_COMM, "process_input() CON_%d %s Read: %d Error: %d", t->connected, (t->character) ? GET_NAME(t->character) : "", thisround, errno);
				return (-1);
			}
			return 0;
		}
	}

	int len = begin + thisround;
	buf[len] = 0; // safety vs broken code
	bp = buf;

	for (int i = 0; i < len; i++)
	{
		switch (buf[i])
		{
		case 0:     // illegal; ignore
		case '\r':
			break;

		case '\n':
			*bp = 0;
			process_line(t, buf);
			bp = buf;
			break;

		case (char)IAC:
		{
			int consumed = parse_telnet_options(t, buf + i, len - i);
			if (consumed <= 0)
			{
				/* Preserve a fragmented Telnet command for the next socket read. */
				memmove(bp, buf + i, len - i);
				bp += len - i;
				goto incomplete;
			}
			i += consumed - 1;
			break; /* prevent fall-through to backspace handler */
		}

		case '\b':
		case 127: // handle both ^H and DEL
			while (bp > buf)
			{
				// Eat whole Unicode characters, do no other
				// processing.  We don't support clusters thus
				// no need to consume multiple codepoints.
				if (!IS_UTF8_TAIL(*--bp) || t->cp437)
					break;
			}
			break;

		default:
			*bp++ = buf[i]; // possibly no-op if bp hasn't changed
		}
	}

incomplete:
	if (bp - buf > MAX_INPUT_LENGTH - 1)
	{
		// is it even a good idea to process it anyway?
		*bp = 0;
		process_line(t, buf);
		bp = buf;
	}
	t->buflen = bp - buf;
	return 0;
}

/*
 * Count accepted player input as activity when it is queued, not only when the
 * command loop eventually executes it.  Combat waits and map movement can
 * legitimately delay get_from_q() across several point_update() ticks.
 */
static void note_player_input_activity(P_desc t, const char *input)
{
	if (!t || t->connected != CON_PLAYING || !t->character || !IS_PC(t->character) || !*input)
		return;

	t->character->specials.timer = 0;
	REMOVE_BIT(t->character->specials.act, PLR_AFK);
}

static void process_line(P_desc t, char *in)
{
	char out[MAX_QUEUE_LENGTH * 3]; // max expansion
	char buffer[MAX_STRING_LENGTH];
#ifdef SMART_PROMPT
	if (t->character && IS_SET(t->character->specials.act, PLR_SMARTPROMPT))
		t->prompt_mode = TRUE;
#endif

	if (t->cp437)
		upgrade_cp437_and_dollars(out, in);
	else if (validate_utf8_and_dollars(out, in))
	{
		// During login (non-zero connected), bad bytes come from client negotiation;
		// silently discard and re-prompt instead of confusing the user.
		if (!t->connected)
			write_to_descriptor(t, "Bad characters in input, skipped.\r\n");
		out[0] = '\0';
	}

	note_player_input_activity(t, out);

	int k = strlen(out);
	if (k > (MAX_INPUT_LENGTH - 1))
	{
		k = MAX_INPUT_LENGTH - 1;
		while (IS_UTF8_TAIL(out[k])) // don't cut in the middle of an Unicode char
			k--; // max 3, we have validated
		out[k] = 0;

		char buffer[MAX_STRING_LENGTH];
		snprintf(buffer, sizeof buffer, "Line too long. Truncated to:\r\n%s\r\n", out);
		if (write_to_descriptor(t, buffer) < 0)
			return;
	}

	/* handle '!' to repeat last command */
	if ((*out != '!') || !*t->last_input || !t->character || t->connected)
		memcpy(t->last_input, out, k + 1);
	else
		strcpy(out, t->last_input);

	if (t && t->character && IS_PC(t->character))
	{
		t->character->only.pc->recived_data += k;
		recivedbytes                        += k;
	}
	write_to_q(out, &t->input, 0);

	snoop_by_data *snoop_by_ptr = t->snoop.snoop_by_list;

	while (snoop_by_ptr)
	{
		write_to_q("&+y%&n ", &snoop_by_ptr->snoop_by->desc->output, 1);
		write_to_q(out, &snoop_by_ptr->snoop_by->desc->output, 1);
		write_to_q("\r\n", &snoop_by_ptr->snoop_by->desc->output, 1);

		snoop_by_ptr = snoop_by_ptr->next;
	}
}

/*
 * **************************************************************** *
 * Public routines for system-to-player-communication        *
 * ****************************************************************
 */

static char send_to_char_f_buf[MAX_STRING_LENGTH];
void        send_to_char_f(P_char ch, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsnprintf(send_to_char_f_buf, sizeof(send_to_char_f_buf) - 1, fmt, args);
	va_end(args);

	send_to_char(send_to_char_f_buf, ch);
}

void send_to_char(const char *messg, P_char ch) { send_to_char(messg, ch, LOG_PUBLIC); }

void send_to_char(const char *messg, P_char ch, int log)
{
	static bool bSwitched = FALSE;

	if (ch && ch->desc && messg)
	{
		if (executing_ch != ch || !IS_SET(ch->specials.act, PLR_PAGING_ON))
		{
			if (SWITCHED(ch) && !bSwitched)
			{
				char buf[30];
				snprintf(buf, 30, "&+M@&+W%s&n: ", J_NAME(ch));
				bSwitched = TRUE;
				write_to_q(buf, &ch->desc->output, 1);
				bSwitched = FALSE;
			}
			write_to_q(messg, &ch->desc->output, 1);
		}
		else
		{
			static bool bWarningAdded = false;
			size_t      len           = strlen(messg);

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

		if ((!IS_TRUSTED(ch) || log != LOG_PUBLIC) && log != LOG_NONE && (ch->desc->connected == CON_PLAYING || ch->desc->connected == CON_MAIN_MENU))
		{
			write_to_pc_log(ch, messg, log);
		}
	}
}

bool send_to_pid(const char *str, int pid)
{
	for (P_desc d = descriptor_list; d; d = d->next)
	{
		if (d->connected == CON_PLAYING && IS_PC(d->character) && GET_PID(d->character) == pid)
		{
			send_to_char(str, d->character);
			return TRUE;
		}
	}
	return FALSE;
}

void send_to_all(const char *messg)
{
	P_desc i;

	if (messg)
		for (i = descriptor_list; i; i = i->next)
			if (!i->connected)
				write_to_q(messg, &i->output, 2);
}

void send_to_outdoor(const char *messg)
{
	P_desc i;

	if (messg)
		for (i = descriptor_list; i; i = i->next)
			if (!i->connected && OUTSIDE(i->character))
				if (IS_TRUSTED(i->character) || !IS_ROOM(i->character->in_room, ROOM_SILENT))
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

void send_to_zone_outdoor(int z_num, const char *messg) { send_to_zone_func(z_num, (int)(-ROOM_INDOORS), messg); }

void send_to_zone_indoor(int z_num, const char *messg) { send_to_zone_func(z_num, (int)ROOM_INDOORS, messg); }

void send_to_zone(int z_num, const char *msg) { send_to_zone_func(z_num, 0, msg); }

void send_to_except(const char *messg, P_char ch)
{
	P_desc i;

	if (messg)
		for (i = descriptor_list; i; i = i->next)
			if (ch->desc != i && !i->connected)
				if (IS_TRUSTED(i->character) || !IS_ROOM(i->character->in_room, ROOM_SILENT))
					write_to_q(messg, &i->output, 2);
}

static char send_to_room_f_buf[MAX_STRING_LENGTH];
void        send_to_room_f(int room, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsnprintf(send_to_room_f_buf, sizeof(send_to_room_f_buf) - 1, fmt, args);
	va_end(args);

	send_to_room(send_to_room_f_buf, room);
}
void send_to_room(const char *messg, int room)
{
	P_char i;

	if ((room < 0) || (room > top_of_world))
	{
		logit(LOG_DEBUG, "send_to_room(): room numb out of range (%d)", room);
		return;
	}

	if (messg)
		for (i = world[room].people; i; i = i->next_in_room)
			if (i->desc)
				if (IS_TRUSTED(i) || !IS_ROOM(i->in_room, ROOM_SILENT) || i->specials.z_cord == 0)
					write_to_q(messg, &i->desc->output, 2);
}

void send_to_room_except(const char *messg, int room, P_char ch)
{
	P_char i;
	char   Gbuf4[MAX_STRING_LENGTH];

	if ((room < 0) || (room > top_of_world))
	{
		logit(LOG_DEBUG, "send_to_room_except(): room numb out of range (%d)", room);
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

void send_to_room_except_two(const char *messg, int room, P_char ch1, P_char ch2)
{
	P_char i;

	if ((room < 0) || (room > top_of_world))
	{
		logit(LOG_DEBUG, "send_to_room_except_two(): room numb out of range (%d)", room);
		return;
	}

	if (messg)
		for (i = world[room].people; i; i = i->next_in_room)
			if ((i != ch1) && (i != ch2) && i->desc)
				if (IS_TRUSTED(i) || !IS_ROOM(i->in_room, ROOM_SILENT))
					write_to_q(messg, &i->desc->output, 2);
}

void act_convert(char *buf, const char *str, P_char ch, P_char to, P_obj obj, void *vict_obj, int type)
{
	P_char               vict;
	char                 tbuf[MAX_STRING_LENGTH];
	bool                 found;
	int                  j, tbp, skip, which_z, sil = FALSE, ig_zc = FALSE;
	char       *point;
	const char *strp, *i;
	int                  terseonly = FALSE;
	int                  notterse  = FALSE;
	bool                 no_eol    = FALSE;

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
						i = PERS((P_char)vict_obj, to, FALSE);
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
						i = HMHR((P_char)vict_obj);
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
							i = HSHR((P_char)vict_obj);
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
						i = HSSH((P_char)vict_obj);
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
						i = OBJN((P_obj)vict_obj, to);
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
						i = OBJS((P_obj)vict_obj, to);
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
					tbp   = 0;
					skip  = 0;
					found = FALSE;

					if (*strp == 'Q')
					{
						if (vict_obj && to)
							i = OBJS((P_obj)vict_obj, to);
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
								if ((LOWER(*(i + 1)) == 'h') && (LOWER(*(i + 2)) == 'e') && (*(i + 3) == ' '))
								{
									found = TRUE;
									i += 3;
									continue;
								}
							}

							// some

							if (!found && (LOWER(*i) == 's') && (LOWER(*(i + 1)) == 'o') && (LOWER(*(i + 2)) == 'm') && (LOWER(*(i + 3)) == 'e') && (LOWER(*(i + 4)) == ' '))
							{
								found = TRUE;
								i += 4;
								continue;
							}
						}

						tbuf[tbp++] = *i;
					}

					tbuf[tbp++] = 0;
					i           = tbuf;

					break;

				case 'a':
					if (obj)
						i = SANA(obj);
					else
						i = NULL;

					break;

				case 'A':
					if (vict_obj)
						i = SANA((P_obj)vict_obj);
					else
						i = NULL;

					break;

				case 'T':
					if (vict_obj)
						i = (char *)vict_obj;
					else
						i = NULL;

					break;

				case 'F':
					if (vict_obj)
						i = FirstWord((char *)vict_obj);
					else
						i = NULL;

					break;

				case 'w': /* complicated crap, I use it for dam_messages() */
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
							i = PERS((P_char)vict_obj, to, FALSE);
						else
							i = NULL;
					}
					else if (type == TO_NOTVICT)
					{
						if (ch && to)
							i = PERS((P_char)vict_obj, to, FALSE);
						else
							i = NULL;
					}

					break;

				case 'W':
					if (type == TO_VICT)
						i = "r"; /* changes you to your */
					else
						i = "'s"; /* changes joe to joe's */

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

void escape_act_dollars(char *dst, size_t dst_size, const char *src)
{
	if (!dst || dst_size == 0)
		return;
	if (!src)
	{
		dst[0] = '\0';
		return;
	}
	size_t di = 0;
	for (size_t si = 0; src[si] && di < dst_size - 2; si++)
	{
		if (src[si] == '$')
		{
			dst[di++] = '$';
			dst[di++] = '$';
		}
		else
			dst[di++] = src[si];
	}
	dst[di] = '\0';
}

// LATENT: no output buffer bounds checking on 'buf'/'tbuf' — safe only
// because format strings are code constants, not player-controlled.
// Would need snprintf-style length tracking to harden.
void act(const char *str, int hide_invisible, P_char ch, P_obj obj, void *vict_obj, int type)
{
	P_char to, vict;
	bool   found;
	// The array buf contains our primary string (final to be sent to target).
	char buf[MAX_STRING_LENGTH], tbuf[MAX_STRING_LENGTH], tbuf2[MAX_STRING_LENGTH];
	/* Debugging
	char     mybuf[MAX_STRING_LENGTH];
	int      mycheck;
	 */
	int                  j, tbp, skip, which_z, sil = type & ACT_SILENCEABLE;
	bool                 ignore_zcoord = type & ACT_IGNORE_ZCOORD;
	char       *point;
	const char *strp, *i;
	int                  terseonly = type & ACT_TERSE;
	int                  notterse  = type & ACT_NOTTERSE;
	bool                 no_eol    = type & ACT_NOEOL;
	unsigned int         flags     = type & ~7;

	type &= 7;

	if (!str || !*str)
		return;

	which_z = (ch ? ch->specials.z_cord : 0);

	if (type == TO_VICT)
	{
		to = (P_char)vict_obj;
		if (to == NULL || !to->desc)
			return;
		/*    which_z = (to ? to->specials.z_cord : 0); */
	}
	else if (type == TO_CHAR)
	{
		if (ch == NULL || !ch->desc)
		{
			return;
		}
		to = ch;
	}
	else if (type == TO_VICTROOM || type == TO_NOTVICTROOM)
	{
		vict = (P_char)vict_obj;
		if (vict)
		{
			if (vict->in_room == NOWHERE)
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
		if (!ch && obj)
		{
			if (!OBJ_ROOM(obj))
			{
				logit(LOG_DEBUG, "Comm.c act: no ch, has obj, but obj (%d) not in a room.", OBJ_VNUM(obj));
				return;
			}
			to      = world[obj->loc.room].people;
			which_z = obj->z_cord;
		}
		else
		{
			if (!ch || ch->in_room == NOWHERE)
			{
				/*        logit(LOG_DEBUG, "act TO_ROOM in NOWHERE %s (%s).", GET_NAME(ch), str);*/
				return;
			}
			to      = world[ch->in_room].people;
			which_z = ch->specials.z_cord;
		}
	}

	if (!to)
		return; /* if a tree falls in the forest... */

	for (; to; to = to->next_in_room)
	{
		// Viewing character needs a descriptor to send to, needs to be awake, and match z-requirements...
		if (to->desc && IS_AWAKE(to) &&
		    (ignore_zcoord || (to->specials.z_cord == which_z))
		    //   needs to not be ignoring target ch (also check only.pc not null - can be null during disconnect)
		    && (IS_NPC(to) || !to->only.pc || !to->only.pc->ignored || (to->only.pc->ignored != ch))
		    //   needs to match the target type: Only TO_CHAR is shown to ch, NOTVICT/NOTVICTROOM doesn't show to victim.
		    && ((type == TO_CHAR) || (to != ch)) &&
		    !((type == TO_NOTVICT || type == TO_NOTVICTROOM) && (to == (P_char)vict_obj))
		    //   needs to have terse toggled appropriately
		    && (IS_NPC(to) || (!terseonly && !notterse) || ((terseonly && IS_SET(to->specials.act2, PLR2_TERSE)) || (notterse && !IS_SET(to->specials.act2, PLR2_TERSE))))
		    //   and message shouldn't be hidden due to an invisible ch or obj.
		    && (!hide_invisible || (to == ch) || (ch ? CAN_SEE(to, ch) : CAN_SEE_OBJ(to, obj))))
		{
			// If it's silencable, and not to an Imm, and there is a flag flag to not show it.
			if (sil && !IS_TRUSTED(to) && (IS_ROOM(to->in_room, ROOM_SILENT) || IS_AFFECTED4(to, AFF4_DEAF)))
			{
				continue;
			}

			for (strp = str, point = buf;;)
			{
				if (*strp == '$')
				{
					j = 0;

					switch (*(++strp))
					{
						case 'n':
							if (ch)
							{
								if (ch == to)
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
							if (vict_obj)
							{
								if ((P_char)vict_obj == to)
								{
									i = "you";
								}
								else
								{
									i = PERS((P_char)vict_obj, to, FALSE);
								}
							}
							else
							{
								i = "(NULL)";
							}
							break;

						case 'm':
							if (ch)
								i = HMHR(ch);
							else
								i = "(NULL)";
							break;

						case 'M':
							if (vict_obj)
								i = HMHR((P_char)vict_obj);
							else
								i = "(NULL)";
							break;

						case 's':
							if (ch)
							{
								if (type == TO_CHAR)
									i = "your";
								else
									i = HSHR(ch);
							}
							else
								i = "(NULL)'s";
							break;

						case 'S':
							if (vict_obj)
							{
								if (type == TO_VICT)
									i = "your";
								else
									i = HSHR((P_char)vict_obj);
							}
							else
								i = "(NULL)'s";
							break;

						case 'e':
							if (ch)
								i = HSSH(ch);
							else
								i = "(NULL)";
							break;

						case 'E':
							if (vict_obj)
								i = HSSH((P_char)vict_obj);
							else
								i = "(NULL)";
							break;

						case 'o':
							if (obj)
								i = OBJN(obj, to);
							else
								i = "(NULL)";

							break;

						case 'O':
							if (vict_obj)
								i = OBJN((P_obj)vict_obj, to);
							else
								i = "(NULL)";
							break;

						case 'p':
							if (obj)
								i = OBJS(obj, to);
							else
								i = "(NULL)";
							break;

						case 'P':
							if (vict_obj)
								i = OBJS((P_obj)vict_obj, to);
							else
								i = "(NULL)";
							break;

							/*
							 * 'q's' are same as p's except it kills 'A | An | The | Some' from
							 * the start of the string, it's ugly, cause we have to skip leading ansi stuff. JAB
							 */
						case 'q':
						case 'Q':
						{
							if (*strp == 'Q')
							{
								if (vict_obj)
									i = OBJS((P_obj)vict_obj, to);
								else
									i = NULL;
							}
							else
							{
								if (obj)
									i = OBJS(obj, to);
								else
									i = NULL;
							}

							if (i == NULL)
							{
								i = "(NULL)";
								break;
							}

							tbuf[0] = '\0';
							tbp     = 0;
							// safety limit for tbuf to prevent buffer overflow
							const int tbuf_limit = MAX_STRING_LENGTH - 10;
							// First _copy_ ansi to tbuf.
							while (*i == '&' && tbp < tbuf_limit)
							{
								if (i[1] == 'n' || i[1] == 'N')
								{
									tbuf[tbp++] = '&';
									i++;
									tbuf[tbp++] = *(i++);
								}
								// Begins with && -> actual & to target.  What to do here is debatable.
								// Decided to just break and leave && as beginning of i.
								else if (i[1] == '&')
								{
									break;
								}
								else if ((i[1] == '+' || i[1] == '-') && is_ansi_char(i[2]))
								{
									tbuf[tbp++] = '&';
									i++;
									tbuf[tbp++] = *(i++);
									tbuf[tbp++] = *(i++);
								}
								else if (i[1] == '=' && is_ansi_char(i[2]) && is_ansi_char(i[3]))
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
							if ((LOWER(*i) == 'a'))
							{
								if (i[1] == ' ')
								{
									i += 2;
								}
								else if ((LOWER(i[1]) == 'n') && (i[2] == ' '))
								{
									i += 3;
								}
							}
							// Same for "The "
							else if ((LOWER(*i) == 't') && (LOWER(i[1]) == 'h') && (LOWER(i[2]) == 'e') && (LOWER(i[3]) == ' '))
							{
								i += 4;
							}
							// And same for "Some "
							else if ((LOWER(*i) == 's') && (LOWER(i[1]) == 'o') && (LOWER(i[2]) == 'm') && (LOWER(i[3]) == 'e') && (LOWER(i[3]) == ' '))
							{
								i += 5;
							}

							// If the whole string was just ansi chars with optional article (smh.. but zone writers).
							if (*i == '\0')
								i = "(NULL)";
							// Otherwise, add the rest of the string to the end of tbuf (contains ansi).
							else
							{
								snprintf(tbuf + tbp, MAX_STRING_LENGTH - tbp, "%s", i);
								i = tbuf;
							}
							break;
						}

						case 'a':
							if (obj)
								i = SANA(obj);
							else
								i = "(NULL)";
							break;

						case 'A':
							if (vict_obj)
								i = SANA((P_obj)vict_obj);
							else
								i = "(NULL)";
							break;

						case 'T':
							if (vict_obj)
								i = (char *)vict_obj;
							else
								i = "(NULL)";
							break;

						case 'F':
							if (vict_obj)
								i = FirstWord((char *)vict_obj);
							else
								i = "(NULL)";
							break;

						case 'w': /* complicated crap, I use it for dam_messages() */
							if (type == TO_VICT)
							{
								if (ch)
									i = "you";
								else
									i = "(NULL)";
							}
							else if (type == TO_CHAR)
							{
								if (vict_obj)
									i = PERS((P_char)vict_obj, to, FALSE);
								else
									i = "(NULL)";
							}
							else if (type == TO_NOTVICT)
							{
								if (ch)
									i = PERS((P_char)vict_obj, to, FALSE);
								else
									i = "(NULL)";
							}
							else if (type == TO_NOTVICTROOM)
							{
								if (ch)
									i = PERS((P_char)vict_obj, to, FALSE);
								else
									i = "(NULL)";
							}
							break;

						case 'W':
							if (type == TO_VICT)
								i = "r"; /* changes you to your */
							else
								i = "'s"; /* changes joe to joe's */
							break;

						case '$':
							i = "$";
							break;

						default:
							logit(LOG_DEBUG, "act(): Invalid $-code: '$%c' in '%s'.", *strp, str);
							i = "(NULL)";
							break;
					}

					if (i)
					{
						// Note: This doesn't handle ansi in the middle of the lower-cased words.
						// Making it so we don't get 'A', 'An', 'The', or 'Some' in the middle of a sentence (removing caps)!
						// For each word,
						// safety limit to prevent buffer overflow (leave room for null terminator)
						const int tbuf2_limit = MAX_STRING_LENGTH - 10;
						for (tbp = 0; *i;)
						{
							// bounds check before ansi processing
							if (tbp >= tbuf2_limit)
							{
								logit(LOG_DEBUG, "act(): tbuf2 overflow prevented in ansi processing");
								break;
							}

							// Copy beginning ansi.
							while (*i == '&' && tbp < tbuf2_limit)
							{
								if (i[1] == 'n' || i[1] == 'N')
								{
									tbuf2[tbp++] = '&';
									i++;
									tbuf2[tbp++] = *(i++);
								}
								// Begins with && -> actual & to target.  We just copy the && over in this case.
								else if (i[1] == '&')
								{
									tbuf2[tbp++] = '&';
									tbuf2[tbp++] = '&';
									i += 2;
								}
								else if ((i[1] == '+' || i[1] == '-') && is_ansi_char(i[2]))
								{
									tbuf2[tbp++] = '&';
									i++;
									tbuf2[tbp++] = *(i++);
									tbuf2[tbp++] = *(i++);
								}
								else if (i[1] == '=' && is_ansi_char(i[2]) && is_ansi_char(i[3]))
								{
									tbuf2[tbp++] = '&';
									i++;
									tbuf2[tbp++] = *(i++);
									tbuf2[tbp++] = *(i++);
									tbuf2[tbp++] = *(i++);
								}
								else
								{
									break; // not a recognized ansi code, exit loop
								}
							}

							// bounds check before article processing
							if (tbp >= tbuf2_limit)
								break;

							// "A " or "An "
							if (*i == 'A')
							{
								if (i[1] == ' ')
								{
									tbuf2[tbp++] = 'a';
									i++;
								}
								else if ((LOWER(i[1]) == 'n') && (i[2] == ' '))
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
							else if ((*i == 'T') && (LOWER(i[1]) == 'h') && (LOWER(i[2]) == 'e') && (i[3] == ' '))
							{
								tbuf2[tbp++] = 't';
								tbuf2[tbp++] = 'h';
								tbuf2[tbp++] = 'e';
								i += 3;
							}
							// "Some "
							else if ((*i == 'S') && (LOWER(i[1]) == 'o') && (LOWER(i[2]) == 'm') && (LOWER(i[3]) == 'e') && (LOWER(i[4]) == ' '))
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
								while (!isspace(*i) && *i != '\0' && tbp < tbuf2_limit)
								{
									tbuf2[tbp++] = *(i++);
								}
							}

							// Copy following white-space
							while (isspace(*i) && tbp < tbuf2_limit)
							{
								tbuf2[tbp++] = *(i++);
							}
						}
						tbuf2[tbp] = '\0';
						i          = tbuf2;

						for (j = 0; *(i + j) != '\0'; j++)
						{
							*(point++) = *(i + j);
						}
					}
					// Move past the character following the $.
					++strp;
				}
				// If it's not a $, just copy it, breaking at end of char *.
				else if ((*(point++) = *(strp++)) == '\0')
					break;
			}

			// Add \n\r to end of char *.
			if (!no_eol)
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
		if ((type == TO_VICT) || (type == TO_CHAR))
			return;
	}
}

// This function will have to be fixed if sound re-enabled. - Lohrr
// Right now, we're overwriting a const string.
const char *delete_doubledollar(const char *string)
{
	char *read1, *write1;

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
void format_to_snoopers(char *from_string, char *to_string)
{
	char *index, *index2;

	//  debug( "From: '%s'.", from_string );
	index2 = to_string;
	snprintf(index2, MAX_STRING_LENGTH, "&+C%%&N ");
	index2 += 7;
	index = from_string;
	while (*index != '\0')
	{
		if (*index == '\r')
		{
			index++;
			continue;
		}
		if (index[0] == '\n' && index[1] != '\0')
		{
			snprintf(index2, MAX_STRING_LENGTH, "\n&+C%%&N ");
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
