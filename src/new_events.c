
/*
 * ***************************************************************************
 * *  File: events.c                                           Part of Duris *
 * *  Usage: manipulate various event lists
 * * *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <string.h>
#include <stdio.h>
#ifndef _LINUX_SOURCE
#include <sys/types.h>
#endif

#include "comm.h"
#include "db.h"
#include "events.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "mm.h"
#include "objmisc.h"
#include "epic.h"
#include "profile.h"
#include "vnum.obj.h"

#define MAX_FUNCTIONS 5000
#define FUNCTION_NAMES_FILE "lib/misc/event_names"

extern Skill skills[];
struct event_type_data
{
  void    *func;
  char    *func_name;
} function_names[MAX_FUNCTIONS];

/* needed for StartRegen() */
extern int pulse;
/* if true, we have called Events() from the main loop this pulse already */
extern bool after_events_call;

/*
 * main array of pointers to lists, schedule is the 'master' controller,
 * has one element per pulse in a real minute.
 */

P_nevent ne_schedule[PULSES_IN_TICK];
P_nevent ne_schedule_tail[PULSES_IN_TICK];

/*
 * this code was majorly redone by Tharkun, look for original events description
 * in events.c file
 */

struct mm_ds *ne_dead_event_pool = NULL;

/*
 * external variables
 */
/*
 * external variables
 */

extern P_char character_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern int errno;
extern int pulse;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_world;
extern int top_of_zone_table;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern const struct racial_data_type racial_data[LAST_RACE + 1];
void interaction_to_new_wrapper(P_char, P_char, char *);
void event_reset_zone(P_char ch, P_char victim, P_obj obj, void *data);
void register_func_call(void* func, double time);


void disarm_single_event(P_nevent e)
{
  if (e->cld && e->ch)
    remove_link(e->ch, e->cld);
  e->cld = NULL;
  e->func = NULL;
  e->timer = 1;
}

void disarm_char_events(P_char ch, event_func_type func) {
  P_nevent e1, t_e;

  for (e1 = ch->nevents; e1; e1 = e1->next) {
    if ( !func || e1->func == func ) {
      if (e1->cld) {
        remove_link(ch, e1->cld);
      }
      e1->cld = NULL;
      e1->func = NULL;
      e1->timer = 1;
    }
  }
}

void disarm_obj_events(P_obj obj, event_func_type func) {
  P_nevent e1, t_e;

  for (e1 = obj->nevents; e1; e1 = e1->next) {
    if ( !func || e1->func == func ) {
      e1->func = NULL;
      e1->timer = 1;
    }
  }
}

void add_event(event_func func, int delay,
               P_char ch, P_char victim, P_obj obj,
               int flag, void *data, int data_size) 
{
  P_nevent event = NULL;
  struct char_link_data *cld;
  char *data_buf;
  int loc;

  if (!func)
    return;

  if (delay < 0)
    return;

  if (obj && (ch || victim))
    return;

  if (victim && !ch)
    return;

  if ((delay == 0) && after_events_call)
    delay++;

  event = (P_nevent)mm_get(ne_dead_event_pool);

  event->prev_sched = event->next_sched = NULL;
  event->ch = ch;
  event->victim = victim;
  event->obj = obj;
  event->func = func;
  if (ch && victim && ch != victim)
    event->cld = link_char(ch, victim, LNK_EVENT);
  else
    event->cld = NULL;

  if (data && data_size > 0) 
  {
    CREATE(data_buf, char, data_size, MEM_TAG_EVTBUF);
    //data_buf = (char*)malloc(data_size);
    event->data = memcpy(data_buf, data, data_size);
  }

  loc = (delay + pulse) % PULSES_IN_TICK;

  event->timer = (delay / PULSES_IN_TICK) + 1;
  event->element = loc;

  if (ch) 
  {
    event->next = ch->nevents;
    ch->nevents = event;
  }

  if (obj) 
  {
    event->next = obj->nevents;
    obj->nevents = event;
  }

  if (!ne_schedule[loc]) 
  {
    ne_schedule[loc] = event;
    event->prev_sched = NULL;
  } 
  else 
  {
    ne_schedule_tail[loc]->next_sched = event;
    event->prev_sched = ne_schedule_tail[loc];
  }
  ne_schedule_tail[loc] = event;

  //ne_event_counter++;

  return;
}

/*
 * return the time left in the e1 event
 */
int ne_event_time(P_nevent e1)
{
  int time_left;

  time_left = (e1->timer - 1) * PULSES_IN_TICK + e1->element - pulse;
  if (e1->element < pulse)
    time_left += PULSES_IN_TICK;

  return time_left;
}

void ne_events(void)
{
  P_nevent current_event, t_e, next_event;

  if ((pulse < 0) || (pulse > 299)) 
  {
    logit(LOG_EXIT, "pulse (%d) out of range in Events", pulse);
    raise(SIGSEGV);
  }

  PROFILE_START(event_loop);
  for (current_event = ne_schedule[pulse]; current_event; current_event = next_event) 
  {
    next_event = current_event->next_sched;

    if (--(current_event->timer) > 0)
      continue;

    if (current_event->func)
    {
     #ifdef DO_PROFILE
      event_func_type evf = current_event->func;
      PROFILE_START(event_func);
      (current_event->func)(current_event->ch, current_event->victim, current_event->obj, current_event->data);
      PROFILE_END(event_func);
      PROFILE_REGISTER_CALL(evf, event_func_profile_end - event_func_profile_beg)
     #else
      (current_event->func)(current_event->ch, current_event->victim, current_event->obj, current_event->data);
     #endif
    }

    if (current_event->data) 
    {
      FREE(current_event->data);
      current_event->data = NULL;
    }

    if (current_event->ch) 
    {
      if (current_event->cld) 
      {
        remove_link(current_event->ch, current_event->cld);
      }

      if (current_event->ch->nevents == current_event) 
      {
        current_event->ch->nevents = current_event->next;
      } 
      else 
      {
        for (t_e = current_event->ch->nevents; t_e && (t_e->next != current_event); t_e = t_e->next)
          ;
        // used to crash here but what if char event list gets cleared inside event code?
        if (t_e)
          t_e->next = current_event->next;
      }
    }

    if (current_event->obj) 
    {
      if ((current_event->obj->nevents == current_event)) 
      {
        current_event->obj->nevents = current_event->next;
      } 
      else 
      {
        for (t_e = current_event->obj->nevents; t_e && (t_e->next != current_event); t_e = t_e->next)
          ;
        if (t_e)
          t_e->next = current_event->next;
      }
    }

    /*
     * schedule[] list
     */
    if (ne_schedule[current_event->element] == current_event) 
    {
      ne_schedule[current_event->element] = current_event->next_sched;
      if (ne_schedule[current_event->element])
        ne_schedule[current_event->element]->prev_sched = NULL;
    } 
    else 
    {
      current_event->prev_sched->next_sched = current_event->next_sched;
      if (current_event->next_sched)
        current_event->next_sched->prev_sched = current_event->prev_sched;
    }
    if (ne_schedule_tail[current_event->element] == current_event)
      ne_schedule_tail[current_event->element] = current_event->prev_sched;

    //event_counter--;

    current_event->prev_sched = current_event->next_sched = NULL;

    mm_release(ne_dead_event_pool, current_event);
  }
  PROFILE_END(event_loop);
}

P_nevent get_scheduled(P_char ch, event_func func)
{
  P_nevent e;

  for (e = ch->nevents; e; e = e->next)
    if (e->func == func)
      return e;

  return NULL;
}

P_nevent get_scheduled(P_obj obj, event_func func)
{
  P_nevent e;

  for (e = obj->nevents; e; e = e->next)
    if (e->func == func)
      return e;

  return NULL;

}

P_nevent get_next_scheduled(P_nevent e, event_func func)
{
  if (!e)
    return NULL;
  for (e = e->next; e; e = e->next)
    if (e->func == func)
      return e;
  return NULL;
}


void ne_init_events(void)
{
  int j = 0, i = 0;

  pulse = 0;


  memset(ne_schedule, 0, sizeof(ne_schedule));
  memset(ne_schedule_tail, 0, sizeof(ne_schedule_tail));

  ne_dead_event_pool = mm_create("NEVENTS",
    sizeof(struct nevent_data),
    offsetof(struct nevent_data, next), 11);

  logit(LOG_STATUS, "assigning room specials events.");
  for (j = 0; j < top_of_world; j++)
    if (world[j].funct && (*world[j].funct) (j, 0, -10, 0))
    {
      add_event(room_event, PULSE_MOBILE + number(-4, 4), 0, 0, 0, 0, &j, sizeof(j));
    }

  /*
   * do boot_time reset of zones and fire up their reset events, it
   * staggers them over the entire pulse, because zone resets are
   * heavy-duty CPU hogs so we don't want them all at once.
   */

  logit(LOG_STATUS, "boot time reset of all zones.");

  for (i = 0, j = 0; j <= top_of_zone_table; i++, j++)
  {
    i = i % PULSES_IN_TICK;

    logit(LOG_STATUS, "zone %3d:(%5d-%5d) %s",
          j, j ? (zone_table[j - 1].top + 1) : 0, zone_table[j].top,
          zone_table[j].name);

    if (zone_table[j].reset_mode)
    {
      add_event(event_reset_zone, i, 0, 0, 0, 0, &j, sizeof(j));
    }

    reset_zone(j, TRUE);
  }

  /* special cases now */

  /* WD city noises */
  // no... fucking... way...
  //AddEvent(EVENT_SPECIAL, PULSE_MOBILE, TRUE, zone_noises, 0);

  /* game clock */
  add_event(event_another_hour, 500 - pulse, NULL, NULL, NULL, 0, NULL, 0);
  //AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, another_hour, 0);

  /* timed house control stuff */
  // old guildhalls (deprecated)
  //add_event(event_housekeeping, 500, NULL, NULL, NULL, 0, NULL, 0);
  //AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, do_housekeeping, 0);

  /* sunrise, sunset, etc informer */
  add_event(event_astral_clock, 500, NULL, NULL, NULL, 0, NULL, 0);
  //AddEvent(EVENT_SPECIAL, 500, TRUE, astral_clock, NULL);

  /* sector weather */
  for (j = 0; j < 100; j++)
  {
    add_event(event_weather_change, 500 + number(-9, 9), NULL, NULL, NULL, 0, &j, sizeof(j));
    //AddEvent(EVENT_SPECIAL, 500 + number(-9, 9), TRUE, weather_change, Gbuf1);  
  }  
  
  /* Statistic logging functionality */
  add_event(event_write_statistic, 60, NULL, NULL, NULL, 0, NULL, 0);
  //AddEvent(EVENT_SPECIAL, 60, TRUE, write_statistic, NULL);


  /* miscellaneous character looping */
  add_event(generic_char_event, 20 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
  //AddEvent(EVENT_SPECIAL, 20 * 4, FALSE, generic_char_event, 0);

  /* justice main engine */
  j = 1;
  add_event(event_justice_engine, PULSES_IN_TICK, NULL, NULL, NULL, 0, &j, sizeof(j));
  //AddEvent(EVENT_SPECIAL, PULSES_IN_TICK, TRUE, justice_engine1, NULL);

  /* rather than add a new function, set initial room light values here */
  for (j = 0; j < top_of_world; j++)
    room_light(j, REAL);

  logit(LOG_STATUS, "Done scheduling events.\n");
}

void zone_purge(int zone_number)
{
   P_char vict, next_v;
   P_obj obj, next_o;
   struct zone_data to_purge = zone_table[zone_number];
   int k;   

   for(k = to_purge.real_bottom;k != NOWHERE && k <= to_purge.real_top;k++)
   {
     for (vict = world[k].people; vict; vict = next_v)
     {
        next_v = vict->next_in_room;
        if(IS_NPC(vict) && !IS_MORPH(vict))
        {
          extract_char(vict);
          vict = NULL;
        }
      }

      for (obj = world[k].contents; obj; obj = next_o)
      {
         next_o = obj->next_content;

         if(obj->R_num == real_object(VOBJ_WALLS))
           continue;
  
         if(obj->type == ITEM_CORPSE && !obj->contains) // Don't purge corpses w/ contents
         {}
         else
         {
           extract_obj(obj, TRUE);
           obj = NULL;
         }
      }
   }
}

void event_broken(struct char_link_data *cld)
{
  P_char   ch = cld->linking;
  P_nevent  e;

  if (!ch)
    return;

  for (e = ch->nevents; e; e = e->next)
    if (e->cld == cld) {
      e->func = NULL;
      e->cld = NULL;
      break;
    }
}

char *get_function_name(void *func)
{
  int      i;

  for (i = 0; function_names[i].func; i++)
  {
    if (function_names[i].func == func)
      return function_names[i].func_name;
  }

  if( event_autosave == func )
    return "autosave";

  return "unknown function";
}


// file FUNCTION_NAMES_FILE should be created with
// nm --demangle dms | grep " T " before each boot,
// preferably with a mainboot script
void load_event_names()
{
  FILE    *f;
  void    *func;
  char     func_name[256];
  char     c;
  int      i = 0;

  f = fopen(FUNCTION_NAMES_FILE, "r");

  if (f)
  {
    while (fscanf(f, "%x %c %s", &func, &c, func_name) == 3 && i < MAX_FUNCTIONS)
    {
      function_names[i].func = func;
      function_names[i].func_name = str_dup(func_name);
      i++;
    }

    fclose(f);
  }
  function_names[i].func = 0;
}

void show_world_events(P_char ch, const char* arg)
{
    int count = 0;
    char     buf[MAX_STRING_LENGTH];
    if(!arg || arg[0] == '\0')
    {
      for (int i = 0; i < PULSES_IN_TICK; i++)
        if (ne_schedule[i])
        {
          for(P_nevent ev = ne_schedule[i]; ev; ev = ev->next_sched)
          {
            count++;
          }
        }
      sprintf(buf, "There are currently %d events scheduled on the system.\nSpecify a function name to see more information about that particular event.\n", count);
      send_to_char(buf, ch);
      return;
    }

    sprintf(buf, "Event function: %s\n\n", arg);
    strcat(buf, "    pulse | timer |  char name   |  vict name   | obj vnum |  data ptr  \n");
    strcat(buf, "   -------|-------|--------------|--------------|----------|------------\n");
    for (int i = 0; i < PULSES_IN_TICK; i++)
      if (ne_schedule[i])
      {
        for(P_nevent ev = ne_schedule[i]; ev; ev = ev->next_sched)
        {
          if(strcmp(get_function_name((void*)ev->func), arg))
            continue;
          count++;
          if((strlen(buf)+80) < sizeof(buf))
          {
            sprintf(buf + strlen(buf),
                    "    %-5d | %-5d | %-12.12s | %-12.12s | %-8d | 0x%08.8x\n",
                    ev->element,
                    ev->timer,
                    ev->ch ? GET_NAME(ev->ch) : "   none",
                    ev->victim ? GET_NAME(ev->victim) : "   none",
                    ev->obj ? GET_OBJ_VNUM(ev->obj) : 0,
                    ev->data);
          }
          else
          {
            i = PULSES_IN_TICK;
            break;
          }
        }
      }
    if(!count)
      strcat(buf, "\n   No events matched that function name.\n");

    page_string(ch->desc, buf, 1);
}


#ifdef DO_PROFILE

PROFILES(DEFINE);
bool do_profile = true;

void save_profile_data(const char* name, double total_inside, double total_outside, unsigned total)
{
    logit(LOG_FILE, "Profile info for \"%s\": inside = %.0f, outside = %.0f, total_calls = %d, average = %.0f, share = %6.3f%%",
      name,
      total_inside,
      total_outside,
      total,
      (total != 0) ? (total_inside / (double)total) : 0,
      (total_inside + total_outside != 0) ? (total_inside / (total_inside + total_outside) * 100.0) : 0);
}
      
struct FuncCallInfo
{
    const char* name;
    const void* addr;
    unsigned calls;
    double time;
    FuncCallInfo* next;
    FuncCallInfo* prev;
} funcCallInfo[MAX_FUNCTIONS + 1];
     

     
void reset_func_call_info()
{
    FuncCallInfo *curr = funcCallInfo;
    do
    {
       curr->calls = 0;
       curr->time = 0;
       curr = curr->next;
    }
    while(curr != funcCallInfo);
}

void init_func_call_info()
{
    funcCallInfo[0].addr = 0;
    funcCallInfo[0].name = "Zero or unknown";
    int i;
    for (i = 1; function_names[i - 1].func; i++)
    {
        funcCallInfo[i].name = function_names[i - 1].func_name;
        funcCallInfo[i].addr = function_names[i - 1].func;
        funcCallInfo[i - 1].next = &(funcCallInfo[i]);
        funcCallInfo[i].prev = &(funcCallInfo[i - 1]);
    }
    funcCallInfo[i - 1].next = &(funcCallInfo[0]);
    funcCallInfo[0].prev = &(funcCallInfo[i - 1]);
    reset_func_call_info();
}
    

                       
void save_func_call_info()
{
    unsigned total_calls = 0;
    double total_time = 0;

    FuncCallInfo *curr = funcCallInfo;
    do
    {
        total_calls += curr->calls;
        total_time += curr->time;
        curr = curr->next;
    }
    while(curr != funcCallInfo);

    curr = funcCallInfo;
    do
    {
        logit(LOG_FILE, "Profile info for function \"%-30s\": total calls = %9d (%7.3f%%)  total time = %9.0f (%7.3f%%)",
            curr->name,
            curr->calls,
            (total_calls != 0) ? ((double)curr->calls / (double)total_calls * 100.0) : 0,
            curr->time / 1000.,
            (total_time != 0) ? (curr->time / total_time * 100.0) : 0);
        curr = curr->next;
    }
    while(curr != funcCallInfo && curr->calls != 0);

    logit(LOG_FILE, "Profile info for function \"%-30s\": total calls = %9d (%7.3f%%)  total time = %9.0f (%7.3f%%)",
        "TOTAL",
        total_calls,
        100.0,
        total_time / 1000.,
        100.0);
}



void register_func_call(void* func, double time)
{
    for (FuncCallInfo *curr = funcCallInfo->next; curr != funcCallInfo; curr = curr->next)
    {
        if (curr->addr == func)
        {
            curr->calls++;
            curr->time += time;
            FuncCallInfo *prev = curr->prev;
            if (prev != funcCallInfo && curr->calls > prev->calls)
            {
                while (prev != funcCallInfo && prev->calls < curr->calls)
                    prev = prev->prev;
                curr->next->prev = curr->prev;
                curr->prev->next = curr->next;
                curr->next = prev->next;
                curr->prev = prev;
                prev->next = curr;
                curr->next->prev = curr;
            }
            return;
        }
    }
    funcCallInfo[0].calls++;
    funcCallInfo[0].time += time;
}

#endif
