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

void event_memorize(P_char, P_char, P_obj, void *);
void disarm_char_nevents(P_char ch, event_func_type func);
void disarm_obj_nevents(P_obj obj, event_func_type func);
P_nevent get_scheduled(P_char, event_func_type);
void ne_init_events();
int ne_event_time(P_nevent);

bool     do_reporting = FALSE;

extern Skill skills[];

/* needed for StartRegen() */
extern int pulse;

/* if true, we have called Events() from the main loop this pulse already */
extern bool after_events_call;

/*
 * global list of pointers to all pending event structures.
 */

P_event  event_list = 0;
P_event  event_list_tail = 0;

/*
 * event structs are never freed (currently), they are created as needed,
 * and added to <event_list>, <schedule>, and <event_type_list>.  Once
 * triggered, if they are not cyclical, they are removed from these 3
 * lists and added to the head of <avail_events>.  No processing is done
 * on <avail_events> it is merely a 'holding' list for unused event
 * elements, when a new event is needed, it's pulled off this list, rather
 * than having to create/free them all the time.
 */

P_event  avail_events = 0;

/*
 * main array of pointers to lists, schedule is the 'master' controller,
 * has one element per pulse in a real minute.
 */

P_event  schedule[PULSES_IN_TICK];
P_event  schedule_tail[PULSES_IN_TICK];

/*
 * event_loading holds the number of pending events for each pulse, used
 * by ReSchedule to balance the loading of events so we don't get
 * everything happening at once.
 */

uint     event_loading[PULSES_IN_TICK];

/*
 * event_type_list is another array of pointers, but has one element per
 * EVENT_*, useful for finding an existing event by type.
 */

P_event  event_type_list[LAST_EVENT];
P_event  event_type_list_tail[LAST_EVENT];

/*
 * event_counter is a diagnostic tool more than anything else, each
 * element of this array holds the total number of pending events of that
 * type. event_counter[LAST_EVENT] holds the number of elements in
 * event_list (ie. all pending events).  event_counter[LAST_EVENT + 1]
 * holds the number of elements in avail_events (ie. how many event
 * elements are sitting around taking up space and not doing anything.)
 */

uint     event_counter[LAST_EVENT + 2];

/*
 * global var that holds the event currently being triggered, or NULL.
 */

P_event  current_event = 0;

/*
 * names of event types, used in log messages and by 'world events'
 */

const char *event_names[] = {
  "NULL",
  "Deferred",
  "Wait",
  "Regen Hits",
  "Regen Moves",
  "Regen Mana",
  "Mob Mundane",
  "Mob Special",
  "Object",
  "Room",
  "Falling Char",               // 10
  "Falling Obj",
  "Zone Reset",
  "Obj affect",
  "Skill Reset",
  "Special",
  "Ship Move",
  "Fire Plane",
  "Auto Save",
  "Track Decay",
  "Spell Scribe",               // 20
  "Spell Mem",
  "Spellcast",
  "Melee combat",
  "Bard singing",
  "Bard fx-decay",
  "Delayed func",
  "Stunned",
  "Knocked Out",
  "Brain Drain",
  "AggAttack",                  // 30
  "Berserk",
  "Affect Bal",
  "Hunter",
  "Underwater",
  "Swimming",
  "Brain Absorb",
  "Gith neckbite",
  "Reset zone complete",
  "Disguise",
  "Damage Char",                // 40
  "Immolate Char",
  "Balance Effects NoDie",
  "Dazzle",
  "Throat Crush",
  "Acid immolate",
  "Short affect",
  "Room affect",
  "Order Troop",
  "Combination",
  "Undead Mem",                 // 50
  "Flurry",
  "Artifact",
  "Sacking",
  "Displacement",
  "Lotus",
  "Spec Timer",
  "CDOOM",
  "Reconstruction" "Phantasmal form",
  "Magma burst",                // 60
  "Dread wave",
  "Piper singing",
  "Piper fx-decay",
  "Short affect",
  "Room affect",
  "Interaction",
  "Interaction-peer",
  "Barrage"
};

/*
 * The way this works:
 *
 * There are only 2 lists of struct event_data elements, event_list, and
 * avail_events, event_list holds the events that are pending,
 * avail_events holds the elements that are not currently in use.
 * event_list is doubly- linked, avail_events is a FIFO stack.
 *
 * schedule[] is 240 lists of pointers to events (one for each pulse in a
 * real minute), elements in these lists are also doubly-linked.
 *
 * event_type_list[] is (LAST_EVENT) lists of pointers to events, also
 * doubly- linked to facilitate finding an event of a given type.
 *
 * current_event is just a pointer to the 'active' event, or NULL, if
 * event processing is not being done.  Can be used as a flag to see if
 * current call is the result of an event or not.
 *
 * For almost ALL purposes, the only things you need to know is what the
 * various EVENT_ types are, and what arguments to give the AddEvent()
 * macro.
 *
 * schedule array is used to segregate event timing, as pulses are
 * advanced, they step down this array, each index represents a 1/4 second
 * timing pulse. If an event is supposed to take place 30 minutes in the
 * future, we add an event_data element to the list pointed to by
 * schedule[pulse], with a timer value of 30.  Then every 240th pulse, the
 * timer value of all events in that element's list are decremented, when
 * they hit 0, they are triggered (and deleted from the schedule list if
 * one_shot is TRUE).  Periodic events will just reset the timer value
 * (things like the clock tower in WD spring to mind) which is why
 * current_event exists (and will always point to the active event).  All
 * specials (mob/obj/room/global) will use this event queue.
 *
 * Advantages to doing this?  Speed.  Flexibility.  Efficiency.  It
 * eliminates LOTS of redundant and repetitious processing.  Why speed up
 * something that's already damn fast?  Because it frees up MANY processor
 * cycles we can use creatively (want to set a bunch of guards patrolling
 * an area with a fixed schedule?  Want a spell effect delayed by 10 real
 * minutes?  Want combat blow timing to be settable based on character's
 * speed and condition? Want to have regen rates set so that they get one
 * point at a time, at various times, rather than everyone getting a bunch
 * on the tick, every tick?  No problem.
 *
 * Key to this working, list of EVENT_TYPEs, there is room for (ubyte)255
 * different TYPES of events, that should be more than enough for any
 * foreseeable future.
 *
 * event_type_list has the list of pending events, (by event type)
 * avail_events holds the list of allocated, but not pending event
 * elements.  When a one_shot event is triggered and removed, it's added
 * to the head of this list.  When a new event needs to be added, it
 * checks this list and reuses the elements, only if no unused event
 * elements are available does it allocate a new one.  No mechanism for
 * freeing elements on these lists currently, if this becomes a problem,
 * I'll add an event to periodically free excess event elements.
 */

struct mm_ds *dead_event_pool = NULL;

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
extern int top_of_zone_table;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern const struct racial_data_type racial_data[LAST_RACE + 1];
void     interaction_to_new_wrapper(P_char, P_char, char *);

const char *get_event_name(P_event e)
{
  return event_names[(int) e->type];
}

// The type is an artifact of old event code.  It's ignored now.
void clear_char_nevents(P_char ch, int type, void *func)
{
  P_event  e1, next_event, prev_event;
  P_char   tch;
  struct char_link_data *cld;

  if( !ch )
  {
    logit(LOG_DEBUG, "clear_char_nevents: NULL ch");
    return;
  }

  disarm_char_nevents(ch, (event_func_type)func);
  ch->nevents = NULL;

// This whole section is old event stuff.  We don't do this anymore.
return;

  /*
   * ok, minor mod here, if a char dies, as a RESULT of event
   * processing, it is barely possible to whack up the event list.  So,
   * to prevent this, we detach all the events from the (soon to be
   * missing) char struct, change the type of all of them to EVENT_NONE,
   * and carry on, later event processing will take care of removing the
   * events themselves.  JAB
   */

  if( type == -1 )
  {
//    disarm_char_events(ch, (event_func_type)func);
    ch->nevents = NULL;
  }

  prev_event = NULL;

  for( e1 = ch->events; e1; e1 = next_event )
  {
    next_event = e1->next;

    if ((e1->type != type && type != -1) ||
        (type == EVENT_CHAR_EXECUTE &&
         (void *) (e1->target.t_func) != func) ||
        (type == EVENT_SPECIAL &&
         (void *) (e1->actor.a_func) != func))
    {
      prev_event = e1;
      continue;
    }

    switch (e1->type)
    {
    case EVENT_SPECIAL:
      if (e1->target.t_arg)
      {
        str_free(e1->target.t_arg);
        e1->target.t_arg = NULL;
      }
      break;
    case EVENT_MOB_HUNT:
      raise(SIGSEGV);
      break;
    }

    if (prev_event == NULL)
    {
      ch->events = next_event;
    }
    else
    {
      prev_event->next = next_event;
    }

    /* and set things for minimal impact */

    e1->type = EVENT_NONE;
    e1->one_shot = TRUE;
    e1->timer = 1;
    e1->next = NULL;
  }
}

void clear_events_type(P_char ch, int type)
{
  clear_char_nevents(ch, type, NULL);
}

/*
 * remove all events associated with the target char.  Called from
 * extract_char so that we don't wind up with bogus pointers for events.
 */

void ClearCharEvents(P_char target)
{
  clear_char_nevents(target, -1, NULL);
}

/*
 * remove all events associated with the target object.  Called from
 * extract_obj so that we don't wind up with bogus pointers for events.
 */

void ClearObjEvents(P_obj target)
{
  P_event  e1 = NULL;

  if (!target)
  {
    logit(LOG_DEBUG, "ClearObjEvents called with NULL target");
    return;
  }
  /*
   * code copied from ClearCharEvents
   */

  disarm_obj_nevents(target, 0);

  while (target->events)
  {
    e1 = target->events;
    target->events = e1->next;

    if (e1->type == EVENT_NONE)
      continue;

    /* and set things for minimal impact */

    e1->one_shot = TRUE;
    e1->timer = 1;
    e1->type = EVENT_NONE;
    e1->next = NULL;
  }

  target->events = NULL;
}

void calculate_regen_values(int reg, int *per_pulse, int *delay)
{
  int      sign;

  *per_pulse = 1;

  if (reg < 0)
  {
    sign = -1;
    reg = -reg;
  }
  else
    sign = 1;

  while (reg > PULSES_IN_TICK)
  {
    (*per_pulse)++;
    reg -= PULSES_IN_TICK;
  }

  reg = MAX(1, ((PULSES_IN_TICK / reg) + number(0, 1)));

  if (*per_pulse > 1)
  {
    *per_pulse = sign * (*per_pulse - 1 + (pulse % reg ? 0 : 1));
    *delay = 1;
  }
  else
  {
    *delay = reg;
    *per_pulse = sign;
  }
}

#define MOB_MANA_REGEN_DELAY 5
//codemod
void event_mana_regen(P_char ch, P_char victim, P_obj obj, void *data)
{
  float regen_value = *((float*)data);
  int regen_value_int = (int)regen_value;
  if (regen_value_int >= 1 || regen_value_int <= -1)
  {
      GET_MANA(ch) += regen_value_int;

      if (GET_MANA(ch) > GET_MAX_MANA(ch))
        GET_MANA(ch) = GET_MAX_MANA(ch);

      if (IS_PC(ch) && IS_AFFECTED(ch, AFF_MEDITATE) &&
          (GET_MANA(ch) == GET_MAX_MANA(ch)) && GET_CLASS(ch, CLASS_PSIONICIST))
      {
        send_to_char("&+LYour mana reserves are now full.&n\r\n", ch);
        stop_meditation(ch);
      }


      regen_value = regen_value - (float)regen_value_int;
  }
int per_tick = 0;
  //int per_tick = mana_regen(ch);
  if (IS_AFFECTED(ch, AFF_MEDITATE))
  {
  per_tick = ((GET_C_POW(ch) + GET_C_INT(ch))/.2);

  }
  else
  {
  per_tick = ((GET_C_POW(ch) + GET_C_INT(ch))/2.5);
  }
 

 if ((per_tick == 0) ||
      (GET_MANA(ch) == GET_MAX_MANA(ch) && per_tick > 0) ||
      (GET_MANA(ch) < 0 && per_tick < 0))
  {
    return;
  }

  int delay;
  if (IS_PC(ch))
  {
      delay = 1;
      regen_value += ((float)per_tick / (float)PULSES_IN_TICK);
  }
  else
  {
      delay = MOB_MANA_REGEN_DELAY;
      regen_value += ((float)per_tick / ((float)(PULSES_IN_TICK / MOB_MANA_REGEN_DELAY)));
  }
  add_event(event_mana_regen, delay, ch, 0, 0, 0, &regen_value, sizeof(regen_value));
}

#define MOB_MOVE_REGEN_DELAY 10

void event_move_regen(P_char ch, P_char victim, P_obj obj, void *data)
{
  float regen_value = *((float*)data);
  int regen_value_int = (int)regen_value;
  if (regen_value_int >= 1 || regen_value_int <= -1)
  {
      GET_VITALITY(ch) += regen_value_int;

      if (GET_VITALITY(ch) > GET_MAX_VITALITY(ch))
        GET_VITALITY(ch) = GET_MAX_VITALITY(ch);

      regen_value = regen_value - (float)regen_value_int;
  }

  int per_tick = move_regen(ch, FALSE);

#if defined(CTF_MUD) && (CTF_MUD == 1)
  affected_type *af;
  if ((af = get_spell_from_char(ch, TAG_CTF_BONUS)) != NULL)
  {
    int num = af->modifier;
    if (num >= 10)
    {
      per_tick *= 2;
    }
  }
#endif
  
  if ((per_tick == 0) ||
      (GET_VITALITY(ch) == GET_MAX_VITALITY(ch) && per_tick > 0) ||
      (GET_VITALITY(ch) < 0 && per_tick < 0))
  {
    return;
  }

  int delay;
  if (IS_PC(ch))
  {
      delay = 1;
      regen_value += ((float)per_tick / (float)PULSES_IN_TICK);
  }
  else
  {
      delay = MOB_MOVE_REGEN_DELAY;
      regen_value += ((float)per_tick / ((float)(PULSES_IN_TICK / MOB_MOVE_REGEN_DELAY)));
  }
  add_event(event_move_regen, delay, ch, 0, 0, 0, &regen_value, sizeof(regen_value));
}

void event_hit_regen(P_char ch, P_char victim, P_obj obj, void *data)
{
  float regen_value = *((float*)data);
  int regen_value_int = (int)regen_value;

  if (regen_value_int >= 1 || regen_value_int <= -1)
  {
      GET_HIT(ch) += regen_value_int;

      if (GET_HIT(ch) < -10)
      {
        if (!char_in_list(ch))
          return;
        if (IS_PC(ch))
        {
          logit(LOG_DEATH, "%s killed in %d (< -10 hits)", GET_NAME(ch), (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);
          statuslog(ch->player.level, "%s died in %d ( < -10 hps).", GET_NAME(ch), ((ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number));
        }
        die(ch, ch);
        return;
      }
      regen_value = regen_value - (float)regen_value_int;
  }

  update_pos(ch);
  int per_tick = hit_regen(ch, FALSE);
#if defined(CTF_MUD) && (CTF_MUD == 1)
  affected_type *af;
  if ((af = get_spell_from_char(ch, TAG_CTF_BONUS)) != NULL)
  {
    int num = af->modifier;
    if (num >= 10)
    {
      per_tick *= 2;
    }
  }
#endif

  healCondition(ch, per_tick); // no idea if it really needed, disabled by NEW_COMBAT  -Odorf

  if (GET_HIT(ch) > GET_MAX_HIT(ch) && per_tick > 0)
  {
      GET_HIT(ch) = GET_MAX_HIT(ch);
      return;
  }
  if (per_tick == 0)
  {
    return;
  }

  regen_value += (float)per_tick / (float)PULSES_IN_TICK;
  add_event(event_hit_regen, 1, ch, 0, 0, 0, &regen_value, sizeof(regen_value));
}

void StartRegen(P_char ch, int type)
{
  event_func_type func;
  int delay, per_tick;

  if (type == EVENT_MOVE_REGEN)
  {
    func = event_move_regen;
    if (get_scheduled(ch, func)) return;
    per_tick = move_regen(ch, FALSE);
    delay = IS_PC(ch) ? 1 : MOB_MOVE_REGEN_DELAY; 
  }
  else if (type == EVENT_HIT_REGEN)
  {
    func = event_hit_regen;
    if (get_scheduled(ch, func)) return;
    per_tick = hit_regen(ch, FALSE);
    delay = 1;
  }
  else if (type == EVENT_MANA_REGEN)
  {
    func = event_mana_regen;
    if (get_scheduled(ch, func)) return;
    //codemod remove multiplicate of mana_regen
    per_tick = mana_regen(ch, FALSE);

    delay = IS_PC(ch) ? 1 : MOB_MANA_REGEN_DELAY; 
  }
  else
    return;

  if (per_tick == 0)
      return;

  float regen_value = (float)per_tick / (float)(PULSES_IN_TICK / delay);
  add_event(func, delay, ch, 0, 0, 0, &regen_value, sizeof(regen_value));
}

void event_wait(P_char ch, P_char victim, P_obj obj, void *data)
{
  if(!ch)
  {
    logit(LOG_EXIT, "event_wait called in events.c with no ch");
    raise(SIGSEGV);
  }
  if(ch) // Just making sure.
  {
    if(ch->specials.act2, PLR2_WAIT)
    {
      REMOVE_BIT(ch->specials.act2, PLR2_WAIT);
    }
    if(ch->in_room != NOWHERE)
    {
      update_pos(ch);
    }
  }
}

void DelayCommune(P_char ch, int delay)
{
  if(USES_SPELL_SLOTS(ch))
  {
    P_nevent e = get_scheduled(ch, event_memorize);
    if(e)
    {
      int old_time = ne_event_time(e);
      disarm_char_nevents(ch, event_memorize);
      add_event(event_memorize, (old_time + delay), ch, 0, 0, 0, 0, 0);
    }
  }
}

void CharWait(P_char ch, int delay)
{
  P_nevent e = NULL;
  int      i, old_time;

  if( !ch )
  {
    logit(LOG_EXIT, "CharWait called in events.c with no ch");
    raise(SIGSEGV);
  }

  if( !IS_ALIVE(ch) )
  {
    REMOVE_BIT(ch->specials.act2, PLR2_WAIT);
    debug( "CharWait: Dead char: %s", J_NAME(ch) );
    return;
  }

  if( !CAN_ACT(ch) )
  {
    // The event event_wait just turns off the PLR2_WAIT bit (and updates position).
    e = get_scheduled(ch, event_wait);
    if( e )
    {
      // If the new delay is shorter than the current, ignore new.
      //   Note: If you want delays to stack, change this to to e->timer += delay (I think that'll work).
      if( ne_event_time(e) >= delay )
      {
        return;
      }
      else
      {
        // Kill the shorter event and add a new one.. why not just update e->timer??
        // Because we bucket-sort events and this is faster than moving the event to a new bucket.
        disarm_char_nevents(ch, event_wait);
      }
    }
  }
  if( !IS_TRUSTED(ch) )
  {
    SET_BIT(ch->specials.act2, PLR2_WAIT);
  }
  add_event(event_wait, delay, ch, 0, 0, 0, 0, 0);
}

/*
 * return the time left in the e1 event, various units (see events.h)
 */

int event_time(P_event e1, int ttype)
{
  int      time_left;

  if (!e1 || (ttype < T_PULSES) || (ttype > T_DAYS))
    return 0;

  time_left = (e1->timer - 1) * PULSES_IN_TICK + e1->element - pulse;
  if (e1->element < pulse)
    time_left += PULSES_IN_TICK;

  switch (ttype)
  {
  case T_PULSES:
    return time_left;
    break;
  case T_SECS:
    return (time_left / WAIT_SEC);
    break;
  case T_ROUNDS:
    return (time_left / WAIT_ROUND);
    break;
  case T_MINS:
    return (time_left / WAIT_SEC / SECS_PER_REAL_MIN);
    break;
  case T_HOURS:
    return (time_left / WAIT_SEC / SECS_PER_REAL_HOUR);
    break;
  case T_DAYS:
    return (time_left / WAIT_SEC / SECS_PER_REAL_DAY);
    break;
  }
  return time_left;
}

/*
 * remove current_event from event arrays, returns TRUE on success
 */

bool RemoveEvent(void)
{
  P_event  e1 = current_event, e2;
  P_char   t_ch;

  if (!e1 || (e1->element == -1))
  {
    logit(LOG_EXIT, "RemoveEvent: no valid event to remove");
    raise(SIGSEGV);
  }

  /*
   * first, free t_arg if needed
   */

  switch (e1->type)
  {
  case EVENT_SPECIAL:
    if (e1->target.t_arg)
    {
      str_free(e1->target.t_arg);
      e1->target.t_arg = NULL;
    }
    break;
  case EVENT_SPELLCAST:
    e1->target.t_spell = NULL;
    break;
  case EVENT_MOB_HUNT:
    raise(SIGSEGV);
    break;
  }

  /*
   * remove from obj or char first
   */

  switch (e1->type)
  {
  case EVENT_MOB_HUNT:
    raise(SIGSEGV);
  case EVENT_CHAR_EXECUTE:
  case EVENT_FALLING_CHAR:
  case EVENT_SPELLCAST:
  case EVENT_SWIMMING:
    if (e1->actor.a_ch)
    {
      if (e1->actor.a_ch->events == e1)
      {
        /* head of list */
        e1->actor.a_ch->events = e1->next;
      }
      else
      {
        for (e2 = e1->actor.a_ch->events; e2 && (e2->next != e1);
             e2 = e2->next) ;
        if (!e2)
        {
          logit(LOG_EXIT, "event for %s not found in ch->events list",
                e1->actor.a_ch->player.name);
//  raise(SIGSEGV);
          break;
        }
        e2->next = e1->next;
      }
    }
    break;
  case EVENT_OBJ_AFFECT:
  case EVENT_FALLING_OBJ:
    if (!e1->actor.a_obj)
      break;

    if ((e1->actor.a_obj->events == e1))
    {
      /* head of list */
      e1->actor.a_obj->events = e1->next;
    }
    else
    {
      for (e2 = e1->actor.a_obj->events; e2 && (e2->next != e1);
           e2 = e2->next) ;
      if (!e2)
      {
        logit(LOG_EXIT, "event not found in obj->events list");
        raise(SIGSEGV);
      }
      e2->next = e1->next;
    }
    break;

  }

  /*
   * move from event_list to avail_events
   */

  if (event_list == e1)
  {
    event_list = e1->next_event;
    if (event_list)
      event_list->prev_event = NULL;
  }
  else
  {
    e1->prev_event->next_event = e1->next_event;
    if (e1->next_event)
      e1->next_event->prev_event = e1->prev_event;
  }
  if (event_list_tail == e1)
    event_list_tail = e1->prev_event;

  /*
   * schedule[] list
   */

  if (schedule[e1->element] == e1)
  {
    schedule[e1->element] = e1->next_sched;
    if (schedule[e1->element])
      schedule[e1->element]->prev_sched = NULL;
  }
  else
  {
    e1->prev_sched->next_sched = e1->next_sched;
    if (e1->next_sched)
      e1->next_sched->prev_sched = e1->prev_sched;
  }
  if (schedule_tail[e1->element] == e1)
    schedule_tail[e1->element] = e1->prev_sched;

  event_counter[LAST_EVENT]--;

  /*
   * clear out the (now) unused pointers
   */

  e1->prev_sched = e1->next_sched = NULL;
  e1->prev_event = current_event = NULL;
  e1->element = -1;             /* flag to show this struct is unused now  */

  mm_release(dead_event_pool, e1);

  return TRUE;
}

/* add an event element to schedule[], returns TRUE on success */

__attribute__((deprecated)) bool Schedule(int type, long pulses, int flag, void *actor, void *target)
{
  P_event  e1 = NULL, e2 = NULL;
  int      loc;
  long     delay;

  if (type >= LAST_EVENT)
    return FALSE;

  if (!actor)
    return FALSE;

  if (pulses < 0)
    return FALSE;

  /*
   * this next prevents overrun, if an event is added during event
   * processing, it gets added to head of event queues, so it doesn't
   * get processed until the next complete cycle (or later, thanks to
   * ReSchedule()), so the min delay is 1 while events are being
   * processed.
   */

  delay = pulses;
  if ((delay == 0) && after_events_call)
    delay++;

  e1 = (struct event_data *) mm_get(dead_event_pool);

  e1->prev_event = e1->next_event = NULL;
  e1->prev_sched = e1->next_sched = NULL;
  e1->element = -1;

  /*
   * ok, first we assign actor and target, we do it this way, because if
   * we abort out of here, it won't hose up the lists.
   */

  switch (type)
  {

    /*
     * one case has to be here for each EVENT_*, though some can be
     * combined
     */

  case EVENT_NONE:
    e1->actor.a_ch = NULL;
    e1->target.t_ch = NULL;
    break;
  case EVENT_SWIMMING:
    e1->actor.a_ch = (P_char) actor;
    if (IS_PC((P_char) actor))
      e1->target.t_ch = NULL;
    break;
  case EVENT_MOB_HUNT:
    raise(SIGSEGV);
    break;
  case EVENT_CHAR_EXECUTE:
    e1->actor.a_ch = (P_char) actor;
    e1->target.t_func = (void (*)(struct char_data *)) target;
    break;
  case EVENT_TRACK_DECAY:
    e1->actor.a_track = (struct trackrecordtype *) actor;
    e1->target.t_ch = NULL;
    break;
  case EVENT_SPELLCAST:
    e1->actor.a_ch = (P_char) actor;
    if (target)
      e1->target.t_spell = (struct spellcast_datatype *) target;
    else
      e1->target.t_spell = NULL;
    if (pulses > 4)
    {
      logit(LOG_EXIT, "Schedule: too many pulses for EVENT_SPELLCAST");
      raise(SIGSEGV);
    }
    break;
  case EVENT_FALLING_CHAR:
    e1->actor.a_ch = (P_char) actor;
    e1->target.t_num = (int)(long) target;
    break;
  case EVENT_FALLING_OBJ:
    e1->actor.a_obj = (P_obj) actor;
    e1->target.t_num = (int)(long) target;
    break;
  case EVENT_OBJ_AFFECT:
    e1->actor.a_obj = (P_obj) actor;
    e1->target.t_ch = (P_char) target;
    break;
  case EVENT_SPECIAL:
    e1->actor.a_func = (void (*)()) actor;
    if (target)
      e1->target.t_arg = str_dup((char *) target);
    else
      e1->target.t_arg = NULL;
    break;
  default:
    return FALSE;
    break;
  }

  /*
   * ok, valid data, now make sure all the pointers are pointing
   * correctly
   */

  /*
   * find right pulse to add this event
   */

  loc = delay % PULSES_IN_TICK;
  loc += pulse;
  if (loc >= PULSES_IN_TICK)
    loc -= PULSES_IN_TICK;
  if (loc < 0)
  {
    logit(LOG_EXIT, "Schedule: loc is screwy");
    raise(SIGSEGV);
  }

  /*
   * make sure everything is initialized
   */

  e1->type = type;
  e1->one_shot = flag;
  e1->timer = (delay / PULSES_IN_TICK) + 1;
  e1->element = loc;

  /*
   * hehe, nothing like maintaining 5 independant lists!  Gets
   * complicated, but it's lightning fast now
   */

  switch (e1->type)
  {
  case EVENT_MOB_HUNT:
    raise(SIGSEGV);
  case EVENT_CHAR_EXECUTE:
  case EVENT_FALLING_CHAR:
  case EVENT_SPELLCAST:
  case EVENT_SWIMMING:
    e1->next = e1->actor.a_ch->events;
    e1->actor.a_ch->events = e1;
    break;
  case EVENT_OBJ_AFFECT:
  case EVENT_FALLING_OBJ:
    e1->next = e1->actor.a_obj->events;
    e1->actor.a_obj->events = e1;
    break;
  }

  if (!schedule[loc])
  {
    schedule[loc] = e1;
    e1->prev_sched = NULL;
  }
  else
  {
    schedule_tail[loc]->next_sched = e1;
    e1->prev_sched = schedule_tail[loc];
  }
  schedule_tail[loc] = e1;
  event_loading[loc]++;

  if (!event_list)
  {
    event_list = e1;
    e1->prev_event = NULL;
  }
  else
  {
    event_list_tail->next_event = e1;
    e1->prev_event = event_list_tail;
  }
  event_list_tail = e1;
  e1->next_event = NULL;
  event_counter[LAST_EVENT]++;

  return TRUE;
}

void event_reset_zone(P_char ch, P_char victim, P_obj obj, void *data) 
{
  int zone = *((int*)data);
  zone_table[zone].age++;

  if (zone_table[zone].age >= zone_table[zone].lifespan &&
     (zone_table[zone].reset_mode == 2 || ::is_empty(zone)))
  {
    reset_zone(zone, 0);
  }
  else if(!zone_table[zone].reset_mode)
  {
    no_reset_zone_reset(zone);
    return;
  }

  add_event(event_reset_zone, PULSES_IN_TICK, 0, 0, 0, 0, &zone, sizeof(zone));
}

/*
 * called LAST in boot_world(), so that we have access to all of the
 * mobs/objs/ specials/etc.  Set things up and schedule the first set of
 * events.
 */

void init_events(void)
{
  int      i;
  int      j;
  char     Gbuf1[256];

  //ne_init_events();
  ///* make sure all the arrays are initialized */

  //pulse = 0;

  for (i = 0; i < LAST_EVENT; i++)
  {
    event_counter[i] = 0;
    event_type_list[i] = NULL;
    event_type_list_tail[i] = NULL;
  }

  event_counter[LAST_EVENT] = 0;

  for (i = 0; i < PULSES_IN_TICK; i++)
  {
    schedule[i] = NULL;
    schedule_tail[i] = NULL;
    event_loading[i] = 0;
  }

  /* create the initial set of event elements */

  j = top_of_zone_table * 2 + top_of_mobt * 3;  /* approximate */

  avail_events = NULL;

  dead_event_pool = mm_create("EVENTS", sizeof(struct event_data), offsetof(struct event_data, next), 11);

  //logit(LOG_STATUS, "%d initial event elements allocated.\n",
  //      event_counter[LAST_EVENT + 1]);

  //logit(LOG_STATUS, "assigning room specials events.");

  //for (j = 0; j < top_of_world; j++)
  //  if (world[j].funct && (*world[j].funct) (j, 0, -10, 0))
  //  {
  //    add_event(room_event, PULSE_MOBILE + number(-4, 4), 0, 0, 0, 0, &j,
  //              sizeof(j));
  //  }

  ///*
  // * do boot_time reset of zones and fire up their reset events, it
  // * staggers them over the entire pulse, because zone resets are
  // * heavy-duty CPU hogs so we don't want them all at once.
  // */

  //logit(LOG_STATUS, "boot time reset of all zones.");

  //for (i = 0, j = 0; j <= top_of_zone_table; i++, j++)
  //{
  //  i = i % PULSES_IN_TICK;

  //  logit(LOG_STATUS, "zone %3d:(%5d-%5d) %s",
  //        j, j ? (zone_table[j - 1].top + 1) : 0, zone_table[j].top,
  //        zone_table[j].name);

  //  if (zone_table[j].reset_mode)
  //  {
  //    add_event(event_reset_zone, i, 0, 0, 0, 0, &j, sizeof(j));
  //  }

  //  reset_zone(j, 2);
  //}


  ///* special cases now */

  ///* WD city noises */
  //AddEvent(EVENT_SPECIAL, PULSE_MOBILE, TRUE, zone_noises, 0);

  ///* game clock */
  //AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, another_hour, 0);

  ///* timed house control stuff */
  //AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, do_housekeeping, 0);

  ///* sunrise, sunset, etc informer */
  //AddEvent(EVENT_SPECIAL, 500, TRUE, astral_clock, NULL);

  ///* sector weather */
  //for (j = 0; j < 100; j++)
  //{
  //  snprintf(Gbuf1, 256, "%d", j);
  //  AddEvent(EVENT_SPECIAL, 500 + number(-9, 9), TRUE, weather_change, Gbuf1);
  //
  //}
  //
  //
  ///* Statistic logging functionality */
  //AddEvent(EVENT_SPECIAL, 60, TRUE, write_statistic, NULL);


  ///* miscellaneous character looping */
  //AddEvent(EVENT_SPECIAL, 20 * 4, FALSE, generic_char_event, 0);

  ///* justice main engine */
  //AddEvent(EVENT_SPECIAL, PULSES_IN_TICK, TRUE, justice_engine1, NULL);

  ///* rather than add a new function, set initial room light values here */
  //for (j = 0; j < top_of_world; j++)
  //  room_light(j, REAL);

  //logit(LOG_STATUS, "%d events scheduled.\n", event_counter[LAST_EVENT]);
}

/*
 * heart and soul of event driver system.  Called from game_loop only, at
 * most, once per pulse.  Decrements the timers and triggers the events
 * that should happen NOW.  No args, because everything it deals with is
 * global scope.
 */

void Events(void)
{
  P_event  e1, c_e;
  P_char   t_ch, t_pl;
  P_obj    t_obj;
  P_room   t_room;
  char    *t_arg;
  int      i, j;

  //if ((pulse < 0) || (pulse > 299))
  //{
  //  logit(LOG_EXIT, "pulse (%d) out of range in Events", pulse);
  //  raise(SIGSEGV);
  //}
  //for (c_e = schedule[pulse]; c_e; c_e = e1)
  //{
  //  current_event = c_e;
  //  e1 = c_e->next_sched;

  //  if (--(c_e->timer) > 0)
  //    continue;

  //  /* ok, trigger time */

  //  switch (c_e->type)
  //  {

  //    /* one case has to be here for each EVENT_* */

  //  case EVENT_NONE:
  //    break;
  //  case EVENT_CHAR_EXECUTE:
  //    t_ch = (P_char) c_e->actor.a_ch;
  //    if (c_e->target.t_func && t_ch)
  //      ((*c_e->target.t_func) (t_ch));
  //    break;
  //  case EVENT_FALLING_CHAR:
  //    t_ch = (P_char) c_e->actor.a_ch;
  //    if (t_ch)
  //      falling_char(t_ch, TRUE);
  //    break;
  //  case EVENT_OBJ_AFFECT:
  //    t_obj = (P_obj) c_e->actor.a_obj;
  //    if (t_obj)
  //    {
  //      struct obj_affect *af = (struct obj_affect *) c_e->target.t_ch;

  //      event_obj_affect(0,0,t_obj, af);
  //      t_obj = NULL;
  //      c_e->actor.a_obj = NULL;
  //    }
  //    break;

  //  case EVENT_SPECIAL:
  //    (c_e->actor.a_func) ();
  //    break;
  //  case EVENT_TRACK_DECAY:
  //    if (c_e->actor.a_track)
  //      nuke_track(c_e->actor.a_track);
  //    break;
  //  case EVENT_FALLING_OBJ:
  //    t_obj = (P_obj) c_e->actor.a_obj;
  //    if (t_obj)
  //      falling_obj(t_obj, 1, false);
  //    break;
  //  case EVENT_MOB_HUNT:
  //    raise(SIGSEGV);
  //    break;
  //  case EVENT_SWIMMING:
  //    t_ch = (P_char) c_e->actor.a_ch;
  //    if (t_ch)
  //      swimming_char(t_ch);
  //    break;
  //  default:
  //    logit(LOG_EXIT, "Bogus event type (%d) in Events.", c_e->type);
  //    raise(SIGSEGV);
  //    break;
  //  }

  //  if (c_e && c_e->one_shot && (c_e->element != -1))
  //  {
  //    current_event = c_e;
  //    RemoveEvent();
  //  }
  //}
}

void room_event(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_room   room = &world[*((int *) data)];

  if (room && room->funct)
  {
    (room->funct) (room->number, 0, 0, 0);
    add_event(room_event, PULSE_MOBILE + number(-4, 4), 0, 0, 0, 0, data, sizeof(int));
  }
}

