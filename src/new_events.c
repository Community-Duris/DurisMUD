
/*
 * ***************************************************************************
 * *  File: events.c                                           Part of Duris *
 * *  Usage: manipulate various event lists
 * * *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#ifndef _LINUX_SOURCE
#include <sys/types.h>
#endif

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include "copyover.h"
#include "epic.h"
#include "justice.h"
#include "mm.h"
#include "objmisc.h"
#include "outposts.h"
#include "profile.h"
#include "redis.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "vnum.obj.h"

#define MAX_FUNCTIONS       6000
#define FUNCTION_NAMES_FILE "lib/misc/event_names"
#define NEVENT_BUDGET_USEC_DEFAULT 25000L
#define NEVENT_MAX_CALLBACKS_DEFAULT 1000L

/*
 * internal variables
 */
bool     debug_event_list = FALSE;
P_nevent current_nevent   = NULL;
long     ne_event_counter = 0;

struct nevent_funcs_name_data
{
	void *func;
	char *func_name;
} function_names[MAX_FUNCTIONS];

/*
 * this code was majorly redone by Tharkun, look for original events description
 * in events.c file
 */
struct mm_ds *ne_dead_event_pool = NULL;

/*
 * main array of pointers to lists, schedule is the 'master' controller,
 * has one element per pulse in a real minute.
 */
P_nevent ne_schedule[PULSES_IN_TICK];
P_nevent ne_schedule_tail[PULSES_IN_TICK];

/*
 * external variables
 */
extern Skill skills[];
/* if true, we have called Events() from the main loop this pulse already */
extern bool                          after_events_call;
extern P_char                        character_list;
extern P_index                       mob_index;
extern P_index                       obj_index;
extern P_obj                         object_list;
extern P_room                        world;
extern int                           errno;
extern int                           pulse;
extern int                           top_of_mobt;
extern int                           top_of_objt;
extern const int                     top_of_world;
extern int                           top_of_zone_table;
extern struct time_info_data         time_info;
extern struct zone_data             *zone;
extern struct zone_data             *zone_table;
extern struct sector_data           *sector_table;
extern const struct racial_data_type racial_data[LAST_RACE + 1];
void                                 interaction_to_new_wrapper(P_char, P_char, char *);
void                                 event_reset_zone(P_char ch, P_char victim, P_obj obj, void *data);
void                                 register_func_call(void *func, double time);
const char                          *get_function_name(void *func);
void                                 release_mob_mem(P_char ch, P_char victim, P_obj obj, void *data);
extern void                          event_mob_mundane(P_char, P_char, P_obj, void *);
extern void                          event_spellcast(P_char, P_char, P_obj, void *);

// This function clears up everything in e and gets it ready for mm_release.
// This includes removing it from the obj's list, ch's list, and ne_schedule[] list.
void clear_nevent(P_nevent e)
{
	P_nevent e1;
	P_obj    obj;
	P_char   ch;

	if (!e)
	{
		debug("clear_nevent: bad event e: %ld", e);
	}

	if (e->cld && e->ch)
	{
		remove_link(e->ch, e->cld);
	}
	e->cld = NULL;

	if (e->data)
	{
		FREE(e->data);
		e->data = NULL;
	}

	// If event has an obj, remove e from obj's event list.
	if ((obj = e->obj) != NULL)
	{
		// If e is the first in the list, move to next.
		if (obj->nevents == e)
		{
			obj->nevents = obj->nevents->next_obj_nev;
		}
		// Otherwise, look for event and pull it.
		else
		{
			LOOP_EVENTS_OBJ(e1, obj->nevents)
			{
				if (e1->next_obj_nev == e)
				{
					e1->next_obj_nev = e->next_obj_nev;
					break;
				}
				else if (e1->obj != obj)
				{
					debug("clear_nevent: event '%s': event->obj '%s' != obj '%s' in obj's event list.",
					      (e->func != NULL) ? get_function_name((void *)e->func) : "NULL",
					      (e1->obj == NULL) ? "NULL" : OBJ_SHORT(e1->obj),
					      OBJ_SHORT(obj));
				}
			}
			if (!e1)
			{
				debug("clear_nevent: obj '%s' does not have event '%s' in its event list head(%s).",
				      OBJ_SHORT(obj),
				      (e->func != NULL) ? get_function_name((void *)e->func) : "NoFunc",
				      (obj->nevents != NULL) ? ((obj->nevents->func != NULL) ? get_function_name((void *)obj->nevents->func) : "NoFunc") : "NULL");
			}
		}
		e->obj          = NULL;
		e->next_obj_nev = NULL;
	}

	// If event has a ch,
	if ((ch = e->ch) != NULL)
	{
		// If event is first on ch's list of events, just move list to 2nd.
		if (ch->nevents == e)
		{
			ch->nevents = ch->nevents->next_char_nev;
		}
		else
		{
			// Otherwise, find the event before e in ch's event list.
			LOOP_EVENTS_CH(e1, ch->nevents)
			{
				// If we find the previous, sent previous->next to e->next.
				if (e1->next_char_nev == e)
				{
					e1->next_char_nev = e1->next_char_nev->next_char_nev;
					break;
				}
				// Otherwise, do some debugging.
				else if (e1->ch && ch != e1->ch)
				{
					debug("clear_nevent: event->ch '%s' %d != ch '%s' %d in char's event list. %ld",
					      J_NAME(e1->ch),
					      IS_ALIVE(e1->ch) ? GET_ID(e1->ch) : -1,
					      J_NAME(ch),
					      IS_ALIVE(ch) ? GET_ID(ch) : -1,
					      e1);
				}
			}
			// If we reached the end of the list, or our assignment failed (can that even happen?)
			if (!e1 || e1->next_char_nev != e->next_char_nev)
			{
				debug("clear_nevent: event '%s' not in char '%s' %d event list.", (e->func != NULL) ? get_function_name((void *)e->func) : "NoFunc", J_NAME(ch), IS_ALIVE(ch) ? GET_ID(ch) : -1);
			}
		}
		e->ch            = NULL;
		e->next_char_nev = NULL;
	}

	// If e is the last element, back the last element up one.
	if (ne_schedule_tail[e->element] == e)
	{
		ne_schedule_tail[e->element] = e->prev_sched;
	}

	e1 = ne_schedule[e->element];
	// If at head of list
	if (e1 == e)
	{
		ne_schedule[e->element] = e1->next_sched;
		if (e1->next_sched)
			e1->next_sched->prev_sched = NULL;
		e1->next_sched = NULL;
		// e1->prev_sched already NULL, since it was head of list.
	}
	else
	{
		// Otherwise find it in list
		while (e1 && e1->next_sched != e)
		{
			e1 = e1->next_sched;
		}
		// If not in list!?
		if (!e1)
		{
			debug("Event e '%s' not in ne_schedule[e->element] list.", (e->func != NULL) ? get_function_name((void *)e->func) : "NoFunc");
			e->next_sched = e->prev_sched = NULL;
		}
		// Remove from list.
		else
		{
			e1->next_sched = e->next_sched;
			if (e1->next_sched)
			{
				e1->next_sched->prev_sched = e1;
			}
			e->next_sched = e->prev_sched = NULL;
		}
	}

	e->func    = NULL;
	e->timer   = 1;
	e->element = 0;
	e->victim  = NULL;
}

// Returns true iff all the events in ch->nevents belong to ch.
bool check_ch_nevents(P_char ch)
{
	P_nevent e;

	if (!IS_ALIVE(ch))
	{
		return TRUE;
	}

	LOOP_EVENTS_CH(e, ch->nevents)
	{
		if (e->ch != NULL && e->ch != ch)
		{
			return FALSE;
		}
	}
	return TRUE;
}

// Returns true iff all the events in obj->nevents belong to obj.
bool check_obj_nevents(P_obj obj)
{
	P_nevent e;

	if (obj == NULL)
	{
		return TRUE;
	}

	LOOP_EVENTS_OBJ(e, obj->nevents)
	{
		if (e->obj != obj)
		{
			return FALSE;
		}
	}
	return TRUE;
}

// Make this event not do anything when it executes and make it execute asap.
void disarm_single_event(P_nevent e)
{
	if (e->cld && e->ch)
	{
		remove_link(e->ch, e->cld);
	}
	e->cld   = NULL;
	e->func  = NULL;
	e->timer = 1;
}

// This function neuters ch's events. (They fire but do nothing).
// If func == NULL, all events neutered, otherwise just events of type func.
// If we're neutering all of ch's events, it's fine to set e->ch = NULL and
//   ch->nevents = NULL.  However, if we're not disarming all events, we need
//   to leave e->ch unless we pull the events from the ch->nevents list.
void disarm_char_nevents(P_char ch, event_func_type func)
{
	P_nevent e1;

	if (func == NULL)
	{
		LOOP_EVENTS_CH(e1, ch->nevents)
		{
			if (e1->cld)
			{
				remove_link(ch, e1->cld);
			}
			e1->cld   = NULL;
			e1->func  = NULL;
			e1->timer = 1;
		}
		// Now clear the 'next_char_nev' list.
		while (ch->nevents)
		{
			// Save the next event.
			e1 = ch->nevents->next_char_nev;
			// Erase the next_char_nev & ch.
			ch->nevents->next_char_nev = NULL;
			ch->nevents->ch            = NULL;
			// Move to the next event.
			ch->nevents = e1;
		}
	}
	else
	{
		LOOP_EVENTS_CH(e1, ch->nevents)
		{
			if (e1->func == func)
			{
				if (e1->cld)
				{
					remove_link(ch, e1->cld);
				}
				e1->cld   = NULL;
				e1->func  = NULL;
				e1->timer = 1;
			}
		}
	}
}

// Sets objects events to NULL, so they fire blanks.
// If func == NULL, then neuter all events.
// If func != NULL, then neuter events of type func.
void disarm_obj_nevents(P_obj obj, event_func_type func)
{
	P_nevent e1, t_e;

	// If NULL function, then remove all events.
	if (func == NULL)
	{
		LOOP_EVENTS_OBJ(e1, obj->nevents)
		{
			e1->func  = NULL;
			e1->timer = 1;
		}
		// Now clear the 'next_obj_nev' list.
		while (obj->nevents)
		{
			// Save the next event.
			e1 = obj->nevents->next_obj_nev;
			// Erase the next_obj_nev & obj.
			obj->nevents->next_obj_nev = NULL;
			obj->nevents->obj          = NULL;
			// Move to the next event.
			obj->nevents = e1;
		}
	}
	else
	{
		LOOP_EVENTS_OBJ(e1, obj->nevents)
		{
			if (e1->func == func)
			{
				e1->func  = NULL;
				e1->timer = 1;
			}
		}
	}
}

void add_event(event_func func, int delay, P_char ch, P_char victim, P_obj obj, int flag, void *data, int data_size)
{
	P_nevent               event, e;
	struct char_link_data *cld;
	char                  *data_buf;
	int                    loc;

	if (!func)
	{
		debug("add_event: No function!");
		return;
	}

	if (delay < 0)
	{
		debug("add_event: Delay (%d) les than zero?!", delay);
		return;
	}

	if (ch && !IS_ALIVE(ch) && func != release_mob_mem)
	{
		logit(LOG_DEBUG, "add_event: dead ch '%s' in room r%d/v%d function %s", GET_NAME(ch), ch->in_room, ROOM_VNUM(ch->in_room), get_function_name((void *)func));
		debug("add_event: dead ch '%s' in room r%d/v%d function %s", GET_NAME(ch), ch->in_room, ROOM_VNUM(ch->in_room), get_function_name((void *)func));
		return;
	}

	// No reason an event can't have an object and a ch/victim. - Lohrr
	// Ok, here's the reason... we have an object-events list and a char-events list.
	// Well, now we have two lists: obj->nevents->next_obj_nev ... and ch->nevents->next_char_nev.
	/*
	  if( obj && ch )
	  {
	    // Logging them anyway, just to test..
	    debug( "add_event: func: '%s' has ch: %s %d and obj: %s %d.",
	      (func == NULL) ? "NULL" : get_function_name((void*)func),
	      ch ? J_NAME(ch) : "NULL", ch ? GET_ID(ch) : -1,
	      obj ? OBJ_SHORT(obj) : "NULL", obj ? OBJ_VNUM(obj) : -1 );
	    return;
	  }
	*/

	// Should it be possible for an object to have a victim w/out a ch?
	if (victim && !ch)
	{
		debug(
			"add_event: victim '%s' & !ch, func: %s, obj: %s %d.", J_NAME(victim), (func == NULL) ? "NULL" : get_function_name((void *)func), obj ? OBJ_SHORT(obj) : "NULL", obj ? OBJ_VNUM(obj) : -1);
		return;
	}

	if ((delay == 0) && after_events_call)
	{
		delay++;
	}

	event = (P_nevent)mm_get(ne_dead_event_pool);

	event->prev_sched = event->next_sched = NULL;
	event->next_char_nev = event->next_obj_nev = NULL;
	event->ch                                  = ch;
	event->victim                              = victim;
	event->obj                                 = obj;
	event->func                                = func;

	if (ch && victim && ch != victim)
		event->cld = link_char(ch, victim, LNK_EVENT);
	else
		event->cld = NULL;

	if (data && data_size > 0)
	{
		CREATE(data_buf, char, data_size, MEM_TAG_EVTBUF);
		// data_buf = (char*)malloc(data_size);
		event->data = memcpy(data_buf, data, data_size);
	}

	loc = (delay + pulse) % PULSES_IN_TICK;

	event->timer   = (delay / PULSES_IN_TICK) + 1;
	event->element = loc;

	if (ch)
	{
		if (ch->nevents != NULL)
		{
			// Put event at the end.
			// Loop through ch's events and find the last one.
			LOOP_EVENTS_CH(e, ch->nevents)
			{
				if (e->next_char_nev == NULL)
					break;
			}
			// Put last->next to event.
			e->next_char_nev = event;
		}
		else
			ch->nevents = event;
		// Event at end of ch->nevents, so terminate list.
		event->next_char_nev = NULL;
	}

	if (obj)
	{
		event->next_obj_nev = obj->nevents;
		obj->nevents        = event;
	}

	// If empty list, set event to head of list.
	if (!ne_schedule[loc])
	{
		ne_schedule[loc] = event;
	}
	// Otherwise, add event to tail of list.
	else
	{
		ne_schedule_tail[loc]->next_sched = event;
		event->prev_sched                 = ne_schedule_tail[loc];
	}
	// Set tail to event
	ne_schedule_tail[loc] = event;
	ne_event_counter++;

	if (debug_event_list)
	{
		check_nevents();
	}

	return;
}

// Returns the time left (in pulses) in the e1 event
int ne_event_time(P_nevent e1)
{
	int time_left;

	time_left = (e1->timer - 1) * PULSES_IN_TICK + e1->element - pulse;
	if (e1->element < pulse)
		time_left += PULSES_IN_TICK;

	return time_left;
}

/*  Not sure what this does, but commenting out because it seems unnecessary.
void nevent_from_char( P_nevent old_nevent )
{
  P_char ch;
  P_nevent e;

  if( !old_nevent || !(ch = old_nevent->ch) )
    return;

  // Pull from list
  if( ch->nevents == old_nevent )
  {
    ch->nevents = ch->nevents->next_char_nev;
    if( ch->nevents && ch != ch->nevents->ch && ch->nevents->ch != NULL )
    {
      debug( "nevent_from_char: ch '%s' %d not ch in ch->nevents, func %s.",
        IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch), get_function_name((void *)ch->nevents->func) );
      ch->nevents = NULL;
    }
  }
  else
  {
    if( ch->nevents && ch->nevents->ch != ch && ch->nevents->ch != NULL )
    {
      debug( "nevent_from_char: ch '%s' %d not ch in ch->nevents, func %s.",
        IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch), get_function_name((void *)ch->nevents->func) );
      ch->nevents = NULL;
    }
    LOOP_EVENTS_CH( e, ch->nevents )
    {
      if( e->next_char_nev && e->next_char_nev->ch && e->next_char_nev->ch != ch )
      {
        debug( "nevent_from_char: ch '%s' %d is not ch in sub-event e->next_char_nev, func %s.",
          IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch), get_function_name((void *)e->next_char_nev->func) );
        e->next_char_nev = NULL;
        break;
      }
      if( e->next_char_nev == old_nevent )
      {
        e->next_char_nev = old_nevent->next_char_nev;
        old_nevent->next_char_nev = NULL;
        if( e->next_char_nev && ch != e->next_char_nev->ch && e->next_char_nev->ch != NULL )
        {
          debug( "nevent_from_char: ch '%s' %d not ch in ch->nevents, func %s.",
            IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch), get_function_name((void *)e->next_char_nev->func) );
          e->next_char_nev = NULL;
        }
        break;
      }
    }
  }
}
*/

static long nevent_config_limit(const char *name, long default_value)
{
	const char *raw = getenv(name);
	char *end = NULL;
	long value;

	if (!raw || !*raw)
		return default_value;
	value = strtol(raw, &end, 10);
	if (!end || *end != '\0' || value < 0)
	{
		logit(LOG_STATUS, "Invalid %s='%s'; using %ld", name, raw, default_value);
		return default_value;
	}
	return value;
}

static long nevent_budget_usec(void)
{
	static long configured = -1;
	if (configured < 0)
		configured = nevent_config_limit("DURIS_NEVENT_BUDGET_USEC", NEVENT_BUDGET_USEC_DEFAULT);
	return configured;
}

static long nevent_max_callbacks(void)
{
	static long configured = -1;
	if (configured < 0)
		configured = nevent_config_limit("DURIS_NEVENT_MAX_CALLBACKS", NEVENT_MAX_CALLBACKS_DEFAULT);
	return configured;
}

static long nevent_elapsed_us(const struct timespec *started, const struct timespec *finished)
{
	return (finished->tv_sec - started->tv_sec) * 1000000L + (finished->tv_nsec - started->tv_nsec) / 1000L;
}

/* Move only the unscanned suffix to the front of the next pulse. Leaving it
 * in the current ring bucket would delay already-due work for a full cycle. */
static long nevent_defer_suffix(P_nevent deferred_head)
{
	P_nevent event;
	P_nevent deferred_tail;
	P_nevent prior;
	int next_pulse;
	long deferred = 0;

	if (!deferred_head)
		return 0;

	next_pulse = (pulse + 1) % PULSES_IN_TICK;
	deferred_tail = ne_schedule_tail[pulse];
	prior = deferred_head->prev_sched;
	if (prior)
		prior->next_sched = NULL;
	else
		ne_schedule[pulse] = NULL;
	ne_schedule_tail[pulse] = prior;
	deferred_head->prev_sched = NULL;

	for (event = deferred_head; event; event = event->next_sched)
	{
		event->element = next_pulse;
		deferred++;
		if (event == deferred_tail)
			break;
	}

	if (ne_schedule[next_pulse])
	{
		deferred_tail->next_sched = ne_schedule[next_pulse];
		ne_schedule[next_pulse]->prev_sched = deferred_tail;
	}
	else
	{
		deferred_tail->next_sched = NULL;
		ne_schedule_tail[next_pulse] = deferred_tail;
	}
	ne_schedule[next_pulse] = deferred_head;
	return deferred;
}

// Execute events!
void ne_events(void)
{
	static long count = 0;
	P_nevent    temp_event, next_event;
	P_char      ch;
	P_obj       obj;
	struct timespec loop_started, callback_started, callback_finished, loop_finished;
	long scanned = 0, executed = 0, slowest_us = 0, deferred = 0;
	long budget_usec = nevent_budget_usec();
	long max_callbacks = nevent_max_callbacks();
	bool budget_exhausted = FALSE;
	const char *slowest_name = "none";

	if ((pulse < 0) || (pulse >= PULSES_IN_TICK))
	{
		panic_corruption("ne_events", "pulse (%d) out of range", pulse);
	}

	if (debug_event_list)
	{
		check_nevents();
	}

	clock_gettime(CLOCK_MONOTONIC, &loop_started);
	PROFILE_START(event_loop);
	for (current_nevent = ne_schedule[pulse]; current_nevent; current_nevent = next_event)
	{
		scanned++;
		next_event = current_nevent->next_sched;

		if (--(current_nevent->timer) > 0)
		{
			if (budget_usec > 0 && !(scanned % 64))
			{
				clock_gettime(CLOCK_MONOTONIC, &loop_finished);
				budget_exhausted = nevent_elapsed_us(&loop_started, &loop_finished) >= budget_usec;
			}
			if (budget_exhausted && next_event)
			{
				deferred = nevent_defer_suffix(next_event);
				break;
			}
			continue;
		}

		// If this event has a function to execute (hasn't been neutered)
		if (current_nevent->func)
		{
			event_func_type callback_func = current_nevent->func;
			const char *callback_name = get_function_name((void *)callback_func);
			clock_gettime(CLOCK_MONOTONIC, &callback_started);
#ifdef DO_PROFILE
			PROFILE_START(event_func);
			(callback_func)(current_nevent->ch, current_nevent->victim, current_nevent->obj, current_nevent->data);
			PROFILE_END(event_func);
			PROFILE_REGISTER_CALL(callback_func, event_func_profile_end - event_func_profile_beg)
#else
			(callback_func)(current_nevent->ch, current_nevent->victim, current_nevent->obj, current_nevent->data);
#endif
			clock_gettime(CLOCK_MONOTONIC, &callback_finished);
			executed++;
			long callback_us = nevent_elapsed_us(&callback_started, &callback_finished);
			if (callback_us > slowest_us)
			{
				slowest_us = callback_us;
				slowest_name = callback_name;
			}
		}

		clear_nevent(current_nevent);
		mm_release(ne_dead_event_pool, current_nevent);
		ne_event_counter--;

		if (max_callbacks > 0 && executed >= max_callbacks)
			budget_exhausted = TRUE;
		if (budget_usec > 0)
		{
			clock_gettime(CLOCK_MONOTONIC, &loop_finished);
			if (nevent_elapsed_us(&loop_started, &loop_finished) >= budget_usec)
				budget_exhausted = TRUE;
		}
		if (budget_exhausted && next_event)
		{
			deferred = nevent_defer_suffix(next_event);
			break;
		}
	}
	current_nevent = NULL;
	PROFILE_END(event_loop);
	clock_gettime(CLOCK_MONOTONIC, &loop_finished);
	long loop_us = nevent_elapsed_us(&loop_started, &loop_finished);
	if (deferred > 0)
	{
		logit(LOG_STATUS,
		      "NEVENT BUDGET: pulse=%d total_us=%ld scanned=%ld executed=%ld deferred=%ld slowest=%s slowest_us=%ld scheduled=%ld",
		      pulse,
		      loop_us,
		      scanned,
		      executed,
		      deferred,
		      slowest_name ? slowest_name : "unknown",
		      slowest_us,
		      ne_event_counter);
	}
	if (loop_us >= 50000)
	{
		logit(LOG_STATUS,
		      "NEVENT SLOW: pulse=%d total_us=%ld scanned=%ld executed=%ld slowest=%s slowest_us=%ld scheduled=%ld",
		      pulse,
		      loop_us,
		      scanned,
		      executed,
		      slowest_name ? slowest_name : "unknown",
		      slowest_us,
		      ne_event_counter);
	}
	count++;
}

// Returns the first instance of an event with func as the event function.
// Note: This is only useful if there is only one such event, since the first instance
//   may not be the next to occur.
P_nevent get_scheduled(event_func func)
{
	P_nevent pEvent;

	for (int i = 0; i < PULSES_IN_TICK; i++)
	{
		pEvent = ne_schedule[i];
		while (pEvent != NULL)
		{
			if (pEvent->func == func)
			{
				return pEvent;
			}
			pEvent = pEvent->next_sched;
		}
	}
	return NULL;
}

P_nevent get_scheduled(P_char ch, event_func func)
{
	P_nevent e;

	LOOP_EVENTS_CH(e, ch->nevents)
	{
		if (e->func == func)
		{
			return e;
		}
	}

	return NULL;
}

P_nevent get_scheduled(P_obj obj, event_func func)
{
	P_nevent e;

	LOOP_EVENTS_OBJ(e, obj->nevents)
	{
		if (e->func == func)
		{
			return e;
		}
	}

	return NULL;
}

P_nevent get_next_scheduled_char(P_nevent e, event_func func)
{
	if (!e)
	{
		return NULL;
	}
	// Start with the next event in ch's list and look for func.
	LOOP_EVENTS_CH(e, e->next_char_nev)
	{
		if (e->func == func)
		{
			return e;
		}
	}
	return NULL;
}

P_nevent get_next_scheduled_obj(P_nevent e, event_func func)
{
	if (!e)
	{
		return NULL;
	}
	LOOP_EVENTS_OBJ(e, e->next_obj_nev)
	{
		if (e->func == func)
		{
			return e;
		}
	}
	return NULL;
}

void ne_init_event_pool(void)
{
	pulse = 0;
	memset(ne_schedule, 0, sizeof(ne_schedule));
	memset(ne_schedule_tail, 0, sizeof(ne_schedule_tail));
	ne_dead_event_pool = mm_create("NEVENTS", sizeof(struct nevent_data), offsetof(struct nevent_data, next_sched), 11);
}

void ne_init_events(void)
{
	int j = 0, i = 0;

	ne_init_event_pool();

	logit(LOG_STATUS, "assigning room specials events.");
	for (j = 0; j < top_of_world; j++)
		if (world[j].funct && (*world[j].funct)(j, 0, CMD_SET_PERIODIC, 0))
		{
			add_event(room_event, PULSE_MOBILE + number(-4, 4), 0, 0, 0, 0, &j, sizeof(j));
		}

	/*
	 * do boot_time reset of zones and fire up their reset events, it
	 * staggers them over the entire pulse, because zone resets are
	 * heavy-duty CPU hogs so we don't want them all at once.
	 */

	logit(LOG_STATUS, "Boot time reset of all zones.");

	for (i = 0, j = 0; j <= top_of_zone_table; i++, j++)
	{
		i = i % PULSES_IN_TICK;

		logit(LOG_STATUS, "Zone %3d:(%5d-%5d) %s", j, j ? (zone_table[j - 1].top + 1) : 0, zone_table[j].top, zone_table[j].name);

		// schedule zone reset events (always needed)
		if (zone_table[j].reset_mode)
		{
			add_event(event_reset_zone, i, 0, 0, 0, 0, &j, sizeof(j));
		}

		// skip zone reset during copyover/crash_recovery - mobs preserved from before
		// but still initialize lifespan so zone timers work
		if (copyover_boot || crash_recovery_boot)
		{
			// just set lifespan without spawning mobs
			// for crash recovery, zone ages will be restored from redis later
			if (!crash_recovery_boot)
			{
				if (zone_table[j].lifespan_min != zone_table[j].lifespan_max)
					zone_table[j].lifespan = number(zone_table[j].lifespan_min, zone_table[j].lifespan_max);
				else
					zone_table[j].lifespan = zone_table[j].lifespan_min;
				zone_table[j].age = 0;
			}
		}
		else
		{
			reset_zone(j, 2);
		}
	}

	/* special cases now */

	/* game clock */
	// This is where we set the initial hour mud-tick.
	add_event(event_another_hour, 125 * WAIT_SEC - pulse, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, another_hour, 0);

	/* timed house control stuff */
	// old guildhalls (deprecated)
	// add_event(event_housekeeping, 500, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, do_housekeeping, 0);

	/* sunrise, sunset, etc informer */
	add_event(event_astral_clock, 125 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 500, TRUE, astral_clock, NULL);

	/* sector weather */
	// Why do we have 100 events where the weather changes instead of just one?
	for (j = 0; j < 100; j++)
	{
		// We take 2 ticks before we start changing the weather.
		add_event(event_weather_change, 125 * WAIT_SEC + number(-9, 9), NULL, NULL, NULL, 0, &j, sizeof(j));
		// AddEvent(EVENT_SPECIAL, 500 + number(-9, 9), TRUE, weather_change, Gbuf1);
	}

	/* Statistic logging functionality */
	add_event(event_write_statistic, 15 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 60, TRUE, write_statistic, NULL);

	/* miscellaneous character looping */
	add_event(generic_char_event, 20 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 20 * 4, FALSE, generic_char_event, 0);

	// Checks to see if artifact souls are ready to merge.
	add_event(event_artifact_check_bind_sql, 15 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// Makes artifacts fight and lose time on timers (penalty for multiple artis).
	add_event(event_artifact_wars_sql, 20 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// Checks ALL artis rented and non for negative timers..
	add_event(event_artifact_check_poof_sql, 35 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// Upkeep costs for outposts
	add_event(event_outposts_upkeep, SECS_PER_MUD_HOUR * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// Increases and notifies people if they've ranked up in feudal surname.
	add_event(event_update_surnames, 45 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// redis dirty player saves
	if (redis_enabled)
		add_event(event_flush_dirty_players, 5 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// redis donation message polling
	if (redis_enabled)
		add_event(event_check_donation_messages, 1 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// redis world state saves for crash recovery
	if (redis_enabled && redis_world_state_enabled && !crash_recovery_boot)
		add_event(event_save_world_state, 30 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	logit(LOG_STATUS, "Done scheduling events.\n");
}

void zone_purge(int zone_number)
{
	P_char           vict, next_v;
	P_obj            obj, next_o;
	struct zone_data to_purge = zone_table[zone_number];
	int              k;

	for (k = to_purge.real_bottom; k != NOWHERE && k <= to_purge.real_top; k++)
	{
		for (vict = world[k].people; vict; vict = next_v)
		{
			next_v = vict->next_in_room;
			if (IS_NPC(vict) && !IS_MORPH(vict))
			{
				extract_char(vict);
				vict = NULL;
			}
		}

		for (obj = world[k].contents; obj; obj = next_o)
		{
			next_o = obj->next_content;

			if (obj->R_num == real_object(VOBJ_WALLS))
				continue;

			if (obj->type == ITEM_CORPSE && !obj->contains) // Don't purge corpses w/ contents
			{
			}
			// Don't purge artis either.
			else if (!IS_ARTIFACT(obj))
			{
				extract_obj(obj, TRUE); // Does not include artis.
				obj = NULL;
			}
		}
	}
}

// This function is very CPU intensive.  Do _NOT_ leave it toggled on if you're not having issues.
void check_nevents()
{
	P_char            ch;
	bool              shown = FALSE;
	P_nevent e1, e2;

	// For each event in the game,
	for (int i = 0; i < PULSES_IN_TICK; i++)
	{
		for (e1 = ne_schedule[i]; e1; e1 = e1->next_sched)
		{
			// If the event has a ch,
			if ((ch = e1->ch))
			{
				// If the head of the list isn't equal to the second.
				if (ch->nevents && ch->nevents->ch && ch->nevents->ch != ch)
				{
					debug("check_nevents: ch '%s' %d not ch in ch->nevents, func %s.", IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch), get_function_name((void *)ch->nevents->func));
					shown       = TRUE;
					ch->nevents = NULL;
					continue;
				}
				// Make sure all of ch's events belong to ch.
				LOOP_EVENTS_CH(e2, ch->nevents)
				{
					if (e2->next_char_nev && e2->next_char_nev->ch && e2->next_char_nev->ch != ch)
					{
						debug("check_nevents: ch '%s' %d is not ch in sub-event e->next_char_nev, func %s.",
						      IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch),
						      GET_ID(ch),
						      get_function_name((void *)e2->next_char_nev->func));
						shown             = TRUE;
						e2->next_char_nev = NULL;
						break;
					}
				}
			}
		}
	}
	if (shown)
		debug("%ld is current time.", time(NULL));
}

void event_broken(struct char_link_data *cld)
{
	P_char   ch = cld->linking;
	P_nevent e;

	if (!ch)
	{
		return;
	}

	LOOP_EVENTS_CH(e, ch->nevents)
	{
		if (e->cld == cld)
		{
			e->func = NULL;
			e->cld  = NULL;
			return;
		}
	}
	if (debug_event_list)
	{
		debug("event_broken: couldn't find cld in cld->ch's event list, ch: '%s' %d", J_NAME(ch), GET_ID(ch));
	}
}

void *get_executable_base_address()
{
	FILE *f;
	char  line[256];
	void *base_address = NULL;
	// Open the maps file for the current process
	f = fopen("/proc/self/maps", "r");
	if (f == NULL)
	{
		perror("fopen");
		return NULL;
	}

	// Read the first line, which usually describes the main executable's text segment
	if (fgets(line, sizeof(line), f) != NULL)
	{
		// The address range is at the beginning of the line, e.g., "55b1f9f23000-55b1f9f24000 r-xp..."
		char *dash_pos = strchr(line, '-');
		if (dash_pos != NULL)
		{
			*dash_pos    = '\0'; // Temporarily terminate the string at the dash
			base_address = (void *)strtoul(line, NULL, 16);
		}
	}

	fclose(f);
	return base_address;
}

const char *get_function_name(void *func)
{
	int i;

	if (func == NULL)
	{
		return "NULL";
	}

	for (i = 0; function_names[i].func; i++)
	{
		if (function_names[i].func == func)
			return function_names[i].func_name;
	}

	// Shouldn't this event be in the function_names array?
	if (event_autosave == func)
		return "event_autosave";

	return "unknown function";
}

// file FUNCTION_NAMES_FILE should be created with
// nm --demangle dms | grep " T " before each boot,
// preferably with a mainboot script
void load_event_names()
{
	FILE *f;
	void *func;
	char  func_name[256];
	char  c;
	int   i = 0;

	void *base_address = get_executable_base_address();

	f = fopen(FUNCTION_NAMES_FILE, "r");

	if (f)
	{
		while (fscanf(f, "%p %c %s", &func, &c, func_name) == 3 && i < MAX_FUNCTIONS)
		{
			function_names[i].func      = (void*)((ulong)func + (ulong)base_address);
			function_names[i].func_name = str_dup(func_name);
			i++;
		}

		fclose(f);
	}
	function_names[i].func = 0;
}

void show_world_events(P_char ch, const char *arg)
{
	int  count = 0;
	char buf[MAX_STRING_LENGTH];
	if (!arg || arg[0] == '\0')
	{
		for (int i = 0; i < PULSES_IN_TICK; i++)
			if (ne_schedule[i])
			{
				for (P_nevent ev = ne_schedule[i]; ev; ev = ev->next_sched)
				{
					count++;
				}
			}
		snprintf(buf, MAX_STRING_LENGTH, "There are currently %d events scheduled on the system.\nSpecify a function name to see more information about that particular event.\n", count);
		send_to_char(buf, ch);
		return;
	}

	snprintf(buf, MAX_STRING_LENGTH, "Event function: %s\n\n", arg);
	strcat(buf, "    pulse | timer |  char name   |  vict name   | obj vnum |  data ptr  \n");
	strcat(buf, "   -------|-------|--------------|--------------|----------|------------\n");
	for (int i = 0; i < PULSES_IN_TICK; i++)
		if (ne_schedule[i])
		{
			for (P_nevent ev = ne_schedule[i]; ev; ev = ev->next_sched)
			{
				if (strcmp(get_function_name((void *)ev->func), arg))
					continue;
				count++;
				if ((strlen(buf) + 80) < sizeof(buf))
				{
					snprintf(buf + strlen(buf),
					         MAX_STRING_LENGTH - strlen(buf),
					         //                    "    %-5d | %-5d | %-12.12s | %-12.12s | %-8d | 0x%08.8x\n",
					         "    %-5d | %-5d | %-12.12s | %-12.12s | %-8d | %p\n",
					         ev->element,
					         ev->timer,
					         ev->ch ? GET_NAME(ev->ch) : "   none",
					         ev->victim ? GET_NAME(ev->victim) : "   none",
					         ev->obj ? OBJ_VNUM(ev->obj) : 0,
					         ev->data);
				}
				else
				{
					i = PULSES_IN_TICK;
					break;
				}
			}
		}
	if (!count)
		strcat(buf, "\n   No events matched that function name.\n");

	page_string(ch->desc, buf, 1);
}

#ifdef DO_PROFILE

PROFILES(DEFINE);
bool do_profile = TRUE;

void save_profile_data(const char *name, double total_inside, double total_outside, unsigned total)
{
	logit(LOG_FILE,
	      "Profile info for \"%s\": inside = %.0f, outside = %.0f, total_calls = %d, average = %.0f, share = %6.3f%%",
	      name,
	      total_inside,
	      total_outside,
	      total,
	      (total != 0) ? (total_inside / (double)total) : 0,
	      (total_inside + total_outside != 0) ? (total_inside / (total_inside + total_outside) * 100.0) : 0);
	statuslog(56,
	          "Profile info for \"%s\": inside = %.0f, outside = %.0f, total_calls = %d, average = %.0f, share = %6.3f%%",
	          name,
	          total_inside,
	          total_outside,
	          total,
	          (total != 0) ? (total_inside / (double)total) : 0,
	          (total_inside + total_outside != 0) ? (total_inside / (total_inside + total_outside) * 100.0) : 0);
}

struct FuncCallInfo
{
	const char   *name;
	const void   *addr;
	unsigned      calls;
	double        time;
	FuncCallInfo *next;
	FuncCallInfo *prev;
} funcCallInfo[MAX_FUNCTIONS + 1];

void reset_func_call_info()
{
	FuncCallInfo *curr = funcCallInfo;
	do
	{
		curr->calls = 0;
		curr->time  = 0;
		curr        = curr->next;
	} while (curr != funcCallInfo);
}

void init_func_call_info()
{
	funcCallInfo[0].addr = 0;
	funcCallInfo[0].name = "Zero or unknown";
	int i;

	for (i = 1; function_names[i - 1].func; i++)
	{
		funcCallInfo[i].name     = function_names[i - 1].func_name;
		funcCallInfo[i].addr     = function_names[i - 1].func;
		funcCallInfo[i - 1].next = &(funcCallInfo[i]);
		funcCallInfo[i].prev     = &(funcCallInfo[i - 1]);
	}
	funcCallInfo[i - 1].next = &(funcCallInfo[0]);
	funcCallInfo[0].prev     = &(funcCallInfo[i - 1]);
	reset_func_call_info();
}

void save_func_call_info()
{
	unsigned total_calls = 0;
	double   total_time  = 0;

	FuncCallInfo *curr = funcCallInfo;
	do
	{
		total_calls += curr->calls;
		total_time += curr->time;
		curr = curr->next;
	} while (curr != funcCallInfo);

	curr = funcCallInfo;
	do
	{
		logit(LOG_FILE,
		      "Profile info for function \"%-30s\": total calls = %9d (%7.3f%%)  total time = %9.0f (%7.3f%%)",
		      curr->name,
		      curr->calls,
		      (total_calls != 0) ? ((double)curr->calls / (double)total_calls * 100.0) : 0,
		      curr->time / 1000.,
		      (total_time != 0) ? (curr->time / total_time * 100.0) : 0);
		statuslog(56,
		          "Profile info for function \"%-30s\": total calls = %9d (%7.3f%%)  total time = %9.0f (%7.3f%%)",
		          curr->name,
		          curr->calls,
		          (total_calls != 0) ? ((double)curr->calls / (double)total_calls * 100.0) : 0,
		          curr->time / 1000.,
		          (total_time != 0) ? (curr->time / total_time * 100.0) : 0);
		curr = curr->next;
	} while (curr != funcCallInfo && curr->calls != 0);

	logit(LOG_FILE, "Profile info for function \"%-30s\": total calls = %9d (%7.3f%%)  total time = %9.0f (%7.3f%%)", "TOTAL", total_calls, 100.0, total_time / 1000., 100.0);
}

void register_func_call(void *func, double time)
{	
	for (FuncCallInfo *curr = funcCallInfo->next; curr != funcCallInfo; curr = curr->next)
	{
		if (curr->addr == func)
		{
			double wallClockInSec = time / (double)CLOCKS_PER_SEC;
			if(wallClockInSec > 0.05)
			{
				statuslog(56,
				  "LONG EVENT \"%-30s\": took %f seconds",
				  curr->name,
				  wallClockInSec);
			}
			curr->calls++;
			curr->time += time;
			FuncCallInfo *prev = curr->prev;
			if (prev != funcCallInfo && curr->calls > prev->calls)
			{
				while (prev != funcCallInfo && prev->calls < curr->calls)
					prev = prev->prev;
				curr->next->prev = curr->prev;
				curr->prev->next = curr->next;
				curr->next       = prev->next;
				curr->prev       = prev;
				prev->next       = curr;
				curr->next->prev = curr;
			}
			return;
		}
	}
	funcCallInfo[0].calls++;
	funcCallInfo[0].time += time;
}

#endif
