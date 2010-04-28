
/*
 * ***************************************************************************
 * *  File: debug.c                                            Part of Duris *
 * *  Usage: runtime debugging routines.
 * * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * *************************************************************************** 
 */

#include <ctype.h>
/*
 * #include <errno.h> 
 */
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "mm.h"
#include "profile.h"

/*
 * external variables 
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern int top_of_world;

extern mm_ds_list *mmds_list;
extern mem_usage mem_used[];
extern long allocation_list_node_count;

char     debug_mode = 1;
uint     logcount = 0;

/*
 * called once per game_loop 
 */

void loop_debug(void)
{
  FILE    *fl;
  P_desc   d;

  fl = fopen("logs/log/loop.debug", "w");

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->character)
    {
      if (d->character->in_room >= 0 && d->character->in_room < top_of_world)
        fprintf(fl, "%s m[%d] r[%d] v[%d]\n", GET_NAME(d->character),
                d->connected, d->character->in_room,
                world[d->character->in_room].number);
      else
        fprintf(fl, "%s m[%d] r[NOWHERE]\n", GET_NAME(d->character),
                d->connected);
    }
    else
      fprintf(fl, "[No name] m[%d]\n", d->connected);
  }
  fclose(fl);
}

void hour_debug(void)
{
}

static FILE *cmdfile;

void init_cmdlog(void)
{
  cmdfile = fopen("logs/log/cmd.debug", "w");
}

void cmdlog(P_char ch, char *str)
{
  if (!ch || !ch->player.name)
  {
    logit(LOG_EXIT, "bogus char in call to cmdlog");
    raise(SIGSEGV);
  }
  if (IS_NPC(ch))
    return;
  if (cmdfile && (*(str + 1) != '\0'))
  {
    logcount++;
    if (!(logcount % 500))
    {
      rewind(cmdfile);
    }
    fprintf(cmdfile, "[%d] %s in %d: %s\n", logcount, GET_NAME(ch),
            world[ch->in_room].number, str);
    fflush(cmdfile);
  }
}

void do_debug(P_char ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  if (*argument)
  {
    half_chop(argument, arg1, arg2);
    if (isname(arg1, "profile"))
    {
    #ifdef DO_PROFILE
      if (isname(arg2, "on"))
      {
        if (!do_profile)
        {
          do_profile = true;
          send_to_char("Profiling mode is now ON.\r\n", ch);
        }
        else
          send_to_char("Profiling mode is already ON.\r\n", ch);
      }
      else if (isname(arg2, "off"))
      {
        if (do_profile)
        {
          do_profile = false;
          send_to_char("Profiling mode is now OFF.\r\n", ch);
        }
        else
          send_to_char("Profiling mode is already OFF.\r\n", ch);
      }
      else if (isname(arg2, "reset"))
      {
        send_to_char("Resetting profiling results.\r\n", ch);
        PROFILES(RESET);
        reset_func_call_info();
      }
      else if (isname(arg2, "save"))
      {
        send_to_char("Saving profiling results.\r\n", ch);
        PROFILES(SAVE);
        save_func_call_info();
      }
      else
      {
        send_to_char("Syntex: debug profile <on|off|reset|save>.\r\n", ch);
      }
    #else
      send_to_char("Profiling is not defined.\r\n", ch);
    #endif
      return;
    }
  }
  if (debug_mode)
  {
    debug_mode = 0;
    send_to_char("Debug mode is now OFF.\r\n", ch);
    statuslog(ch->player.level, "debug mode OFF.");
  }
  else
  {
    debug_mode = 1;
    send_to_char("Debug mode is now ON.\r\n", ch);
    statuslog(ch->player.level, "debug mode ON.");
  }
}

#ifdef MEM_DEBUG


void do_mreport(P_char ch, char *argument, int cmd)
{
#ifdef MEMCHK
  char     buf[MAX_STRING_LENGTH] = "";
  struct mm_ds *mmds = NULL;
  struct mm_ds_list *mmlist;
  size_t   mm_active, mm_inactive, mm_allocated, mm_wasted, total_allocs = 0, total_size = 0;

  if (!ch || !ch->desc || !IS_TRUSTED(ch))
    return;

  sprintf(buf, "&+CDirectly allocated memory:&N\n");
  sprintf(buf + strlen(buf), "  &+WTag       &+BAllocations       &+YSize&n\n");
  sprintf(buf + strlen(buf), "------------------------------------\n"); 
  for(int i = 0; i < 52; i++)
  {
    if(!mem_used[i].allocs)
      continue;
    sprintf(buf + strlen(buf), " &+W%4s          &+B%6d       &+Y%8d&n\n", mem_used[i].tag, mem_used[i].allocs, mem_used[i].size);
    total_allocs += mem_used[i].allocs;
    total_size += mem_used[i].size;
  }
  sprintf(buf + strlen(buf), "------------------------------------\n"); 
  sprintf(buf + strlen(buf), "             &+B%8d     &+Y%10d&n\n", total_allocs, total_size);
  sprintf(buf + strlen(buf), "\n&+WAllocation header consumption: %d\n", total_allocs * sizeof(ALLOCATION_HEADER));
  total_size += total_allocs * sizeof(ALLOCATION_HEADER);

  sprintf(buf + strlen(buf), "\n&+CPooled memory resources:&N\n");

#   ifdef MM_STATS

  sprintf(buf + strlen(buf),
          " &+WType    &+C|&+W Active Objects    &+C|&+W Inactive Objects  &+C|&+W Pages Owned      &+C|&+W Waste&N\n");

  mm_active = mm_inactive = mm_allocated = mm_wasted = 0;

  for (mmlist = mmds_list; mmlist; mmlist = mmlist->next)
  {
    size_t   owned_memory, active_memory, inactive_memory;

    mmds = mmlist->mmds;

    owned_memory = (mmds->pages_owned * 4096);
    total_size += owned_memory;
    active_memory = (mmds->objs_used * mmds->size);
    inactive_memory = (owned_memory - mmds->bytes_wasted - active_memory);

    sprintf(buf + strlen(buf),
            " %7s &+C|&n %5d (%9d) &+C|&N %5d (%9d) &+C|&N %4d (%9d) &+C|&n %9d\n",
            mmds->name,
            mmds->objs_used, active_memory,
            (inactive_memory / mmds->size), inactive_memory,
            mmds->pages_owned, owned_memory, mmds->bytes_wasted);

    mm_active += active_memory;
    mm_inactive += inactive_memory;
    mm_allocated += owned_memory;
    mm_wasted += mmds->bytes_wasted;
  }
  sprintf(buf + strlen(buf),
          " &+WTOTALS  &+C|&+Y        %9d  &+C|&+Y        %9d  &+C|&+Y       %9d  &+C|&+Y %9d&N\n",
          mm_active, mm_inactive, mm_allocated, mm_wasted);

#   else
  sprintf(buf + strlen(buf), "&+CMM_STATS not compiled in!&N\r\n");
#   endif

  sprintf(buf + strlen(buf), "\n&+WTotal bytes used: &+C%d&n\n", total_size);

  send_to_char(buf, ch);
#else
  send_to_char("Memory checking not available.  Rebuild with MEMCHK defined.\n", ch);
#endif
}
#endif
