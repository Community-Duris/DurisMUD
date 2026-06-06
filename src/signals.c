/*
 ***************************************************************************
 *  File: signals.c                                          Part of Duris *
 *  Usage: Signal Trapping.                                                  *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 ***************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include <execinfo.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

extern void exit(int);

/*
   external variables
 */

extern volatile sig_atomic_t tics;
extern bool game_booted;
extern int  shutdownflag;
// signal-initiated shutdown: 0=none, 1=shutdown, 2=reboot, 3=copyover
extern volatile sig_atomic_t signal_shutdown_pending;

// extern pid_t lookup_host_process;
void         reap(int sig);

void shutdown_request(int);
void shutdown_notice(int);
void reboot_request(int);
void hupsig(int);
void logsig(int);
void reap(int);
void checkpointing(void);
static void checkpointing_signal(int);

static void install_signal_handler(int signo, void (*handler)(int), int flags)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags   = flags;
	sigemptyset(&sa.sa_mask);
	if (sigaction(signo, &sa, NULL) < 0)
	{
		fatal_boot_error("signals", "sigaction(%d) failed: %s", signo, strerror(errno));
	}
}

void signal_setup(void)
{
	struct itimerval itime;
	struct timeval   interval;

	install_signal_handler(SIGUSR2, shutdown_request, SA_RESTART); // shutdown (no restart)
	install_signal_handler(SIGUSR1, shutdown_notice, SA_RESTART);  // copyover
	install_signal_handler(SIGRTMIN, reboot_request, SA_RESTART);  // reboot

	/*
	   just to be on the safe side:
	 */

	install_signal_handler(SIGHUP, hupsig, SA_RESTART);
	signal(SIGPIPE, SIG_IGN);
	install_signal_handler(SIGINT, hupsig, SA_RESTART);
	install_signal_handler(SIGALRM, logsig, SA_RESTART);
	install_signal_handler(SIGTERM, hupsig, SA_RESTART);
	/* new by fafhrd 11/28/99 */
	install_signal_handler(SIGCHLD, reap, SA_RESTART | SA_NOCLDSTOP);

	/*
	   set up the deadlock-protection
	 */

	// Start timer 900 sec after boot starts (15 min).
	interval.tv_sec  = 900;
	interval.tv_usec = 0;
	itime.it_value   = interval;
	// And have timer check every 15 minutes.
	itime.it_interval = interval;
	// Changing this to 5 min since we don't need to hang for 15 min to know we're stuck.
	itime.it_interval.tv_sec = 300;
	if (setitimer(ITIMER_VIRTUAL, &itime, 0) < 0)
	{
		fatal_boot_error("signals", "setitimer(ITIMER_VIRTUAL) failed: %s", strerror(errno));
	}
	install_signal_handler(SIGVTALRM, checkpointing_signal, SA_RESTART);
}

static volatile sig_atomic_t checkpoint_strikes = 0;
static volatile sig_atomic_t checkpoint_pending = 0;

void checkpointing(void)
{
	if (!checkpoint_pending)
	{
		return;
	}

	if (checkpoint_strikes < 2)
	{
		logit(LOG_EXIT, "CHECKPOINT warning: tics not updated (strike %d)", (int)checkpoint_strikes);
		checkpoint_pending = 0;
		return;
	}

	logit(LOG_EXIT, "CHECKPOINT shutdown: tics not updated (%d strikes)", (int)checkpoint_strikes);

	void *bt[64];
	int   n  = backtrace(bt, 64);
	int   fd = open(LOG_EXIT, O_WRONLY | O_APPEND | O_CREAT, 0644);
	if (fd >= 0)
	{
		char msg[64];
		int  len = snprintf(msg, sizeof(msg), "\n--- hung backtrace #%d ---\n", (int)checkpoint_strikes);
		write(fd, msg, len);
		backtrace_symbols_fd(bt, n, fd);
		close(fd);
	}

	// The reason for this, is that we don't want to reboot into a hung-during-boot situation.
	// In other words, if the mud hangs during a boot, we just want to die completely until it's fixed.
	if (game_booted)
	{
		exit(56);
	}
	else
	{
		exit(-1);
	}
}

static void checkpointing_signal(int signum)
{
	(void)signum;

	if (!tics)
	{
		checkpoint_strikes = checkpoint_strikes + 1;
		checkpoint_pending = 1;
	}
	else
	{
		tics              = 0;
		checkpoint_strikes = 0;
		checkpoint_pending  = 0;
	}
}

// sigusr1 - copyover request from launcher
void shutdown_notice(int signum)
{
	(void)signum;
	signal_shutdown_pending = 3; // copyover
}

// sigusr2 - clean shutdown (no restart)
void shutdown_request(int signum)
{
	(void)signum;
	signal_shutdown_pending = 1; // shutdown
}

// sigrtmin - reboot request from launcher
void reboot_request(int signum)
{
	(void)signum;
	signal_shutdown_pending = 2; // reboot
}

/*
   kick out players etc
 */
void hupsig(int signum)
{
	(void)signum;
	signal_shutdown_pending = 1;
}

void logsig(int signum)
{
	(void)signum;
}

/* This should do the trick... fafhrd 11/28/99 */

/* clean up our zombie kids to avoid defunct processes */
void reap(int sig)
{
	(void)sig;
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}

void reaper(int signum) {}
