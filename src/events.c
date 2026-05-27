/*
 * ***************************************************************************
 * *  File: events.c                                           Part of Duris *
 * *  Usage: manipulate various event lists
 * * *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#ifndef _LINUX_SOURCE
#include <sys/types.h>
#endif

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "utils.h"
#include "epic.h"
#include "gmcp.h"
#include "justice.h"
#include "mm.h"
#include "objmisc.h"
#include "profile.h"
#include "specs.prototypes.h"
#include "spells.h"

void     event_memorize(P_char, P_char, P_obj, void *);
void     disarm_char_nevents(P_char ch, event_func_type func);
void     disarm_obj_nevents(P_obj obj, event_func_type func);
P_nevent get_scheduled(P_char, event_func_type);
void     ne_init_events();
int      ne_event_time(P_nevent);

bool do_reporting = FALSE;

extern Skill skills[];

/* needed for StartRegen() */
extern int pulse;

/* if true, we have called Events() from the main loop this pulse already */
extern bool after_events_call;

/*
 * event_loading holds the number of pending events for each pulse, used
 * by ReSchedule to balance the loading of events so we don't get
 * everything happening at once.
 */

uint event_loading[PULSES_IN_TICK];

/*
 * event_counter is a diagnostic tool more than anything else, each
 * element of this array holds the total number of pending events of that
 * type. event_counter[LAST_EVENT] holds the number of elements in
 * event_list (ie. all pending events).  event_counter[LAST_EVENT + 1]
 * holds the number of elements in avail_events (ie. how many event
 * elements are sitting around taking up space and not doing anything.)
 */

uint event_counter[LAST_EVENT + 2];

/*
 * names of event types, used in log messages and by 'world events'
 */

const char *event_names[] = {"NULL",
                             "Deferred",
                             "Wait",
                             "Regen Hits",
                             "Regen Moves",
                             "Regen Mana",
                             "Mob Mundane",
                             "Mob Special",
                             "Object",
                             "Room",
                             "Falling Char", // 10
                             "Falling Obj",
                             "Zone Reset",
                             "Obj affect",
                             "Skill Reset",
                             "Special",
                             "Ship Move",
                             "Fire Plane",
                             "Auto Save",
                             "Track Decay",
                             "Spell Scribe", // 20
                             "Spell Mem",
                             "Spellcast",
                             "Melee combat",
                             "Bard singing",
                             "Bard fx-decay",
                             "Delayed func",
                             "Stunned",
                             "Knocked Out",
                             "Brain Drain",
                             "AggAttack", // 30
                             "Berserk",
                             "Affect Bal",
                             "Hunter",
                             "Underwater",
                             "Swimming",
                             "Brain Absorb",
                             "Gith neckbite",
                             "Reset zone complete",
                             "Disguise",
                             "Damage Char", // 40
                             "Immolate Char",
                             "Balance Effects NoDie",
                             "Dazzle",
                             "Throat Crush",
                             "Acid immolate",
                             "Short affect",
                             "Room affect",
                             "Order Troop",
                             "Combination",
                             "Undead Mem", // 50
                             "Flurry",
                             "Artifact",
                             "Sacking",
                             "Displacement",
                             "Lotus",
                             "Spec Timer",
                             "CDOOM",
                             "Reconstruction"
                             "Phantasmal form",
                             "Magma burst", // 60
                             "Dread wave",
                             "Piper singing",
                             "Piper fx-decay",
                             "Short affect",
                             "Room affect",
                             "Interaction",
                             "Interaction-peer",
                             "Barrage"};

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

extern P_char                        character_list;
extern P_index                       mob_index;
extern P_index                       obj_index;
extern P_obj                         object_list;
extern P_room                        world;
extern int                           errno;
extern int                           pulse;
extern int                           top_of_mobt;
extern int                           top_of_objt;
extern int                           top_of_zone_table;
extern struct time_info_data         time_info;
extern struct zone_data             *zone;
extern struct zone_data             *zone_table;
extern struct sector_data           *sector_table;
extern const struct racial_data_type racial_data[LAST_RACE + 1];
void                                 interaction_to_new_wrapper(P_char, P_char, char *);

// The type is an artifact of old event code.  It's ignored now.
void clear_char_nevents(P_char ch, int type, void *func)
{
	P_char                 tch;
	struct char_link_data *cld;

	if (!ch)
	{
		logit(LOG_DEBUG, "clear_char_nevents: NULL ch");
		return;
	}

	disarm_char_nevents(ch, (event_func_type)func);
	ch->nevents = NULL;
}

void clear_events_type(P_char ch, int type) { clear_char_nevents(ch, type, NULL); }

/*
 * remove all events associated with the target char.  Called from
 * extract_char so that we don't wind up with bogus pointers for events.
 */

void ClearCharEvents(P_char target) { clear_char_nevents(target, -1, NULL); }

/*
 * remove all events associated with the target object.  Called from
 * extract_obj so that we don't wind up with bogus pointers for events.
 */

void ClearObjEvents(P_obj target)
{
	if (!target)
	{
		logit(LOG_DEBUG, "ClearObjEvents called with NULL target");
		return;
	}
	/*
	 * code copied from ClearCharEvents
	 */

	disarm_obj_nevents(target, 0);
}

void calculate_regen_values(int reg, int *per_pulse, int *delay)
{
	int sign;

	*per_pulse = 1;

	if (reg < 0)
	{
		sign = -1;
		reg  = -reg;
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
		*delay     = 1;
	}
	else
	{
		*delay     = reg;
		*per_pulse = sign;
	}
}

#define MOB_MANA_REGEN_DELAY 5
// codemod
void event_mana_regen(P_char ch, P_char victim, P_obj obj, void *data)
{
	float regen_value     = *((float *)data);
	int   regen_value_int = (int)regen_value;
	if (regen_value_int >= 1 || regen_value_int <= -1)
	{
		GET_MANA(ch) += regen_value_int;

		if (GET_MANA(ch) > GET_MAX_MANA(ch))
			GET_MANA(ch) = GET_MAX_MANA(ch);

		if (IS_PC(ch) && IS_AFFECTED(ch, AFF_MEDITATE) && (GET_MANA(ch) == GET_MAX_MANA(ch)) && GET_CLASS(ch, CLASS_PSIONICIST))
		{
			send_to_char("&+LYour mana reserves are now full.&n\r\n", ch);
			stop_meditation(ch);
		}

		regen_value = regen_value - (float)regen_value_int;
		gmcp_char_vitals(ch);
	}
	int per_tick = 0;
	// int per_tick = mana_regen(ch);
	if (IS_AFFECTED(ch, AFF_MEDITATE))
	{
		per_tick = ((GET_C_POW(ch) + GET_C_INT(ch)) / .2);
	}
	else
	{
		per_tick = ((GET_C_POW(ch) + GET_C_INT(ch)) / 2.5);
	}

	if ((per_tick == 0) || (GET_MANA(ch) == GET_MAX_MANA(ch) && per_tick > 0) || (GET_MANA(ch) < 0 && per_tick < 0))
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

#define MOB_WARD_REGEN_DELAY 3
// codemod
void event_ward_regen(P_char ch, P_char victim, P_obj obj, void *data)
{
	float regen_value     = *((float *)data);
	int   regen_value_int = (int)regen_value;
	if (regen_value_int >= 1 || regen_value_int <= -1)
	{
		GET_WARD(ch) += regen_value_int;

		if (GET_WARD(ch) > GET_MAX_WARD(ch))
			GET_WARD(ch) = GET_MAX_WARD(ch);

		regen_value = regen_value - (float)regen_value_int;
		gmcp_char_vitals(ch);
	}

	int per_tick = ward_regen(ch, FALSE);

	if ((per_tick == 0) || (GET_WARD(ch) == GET_MAX_WARD(ch) && per_tick > 0))
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
		delay = MOB_WARD_REGEN_DELAY;
		regen_value += ((float)per_tick / ((float)(PULSES_IN_TICK / MOB_WARD_REGEN_DELAY)));
	}
	add_event(event_ward_regen, delay, ch, 0, 0, 0, &regen_value, sizeof(regen_value));
}

#define MOB_MOVE_REGEN_DELAY 10

void event_move_regen(P_char ch, P_char victim, P_obj obj, void *data)
{
	float regen_value     = *((float *)data);
	int   regen_value_int = (int)regen_value;
	if (regen_value_int >= 1 || regen_value_int <= -1)
	{
		GET_VITALITY(ch) += regen_value_int;

		if (GET_VITALITY(ch) > GET_MAX_VITALITY(ch))
			GET_VITALITY(ch) = GET_MAX_VITALITY(ch);

		regen_value = regen_value - (float)regen_value_int;
		gmcp_char_vitals(ch);
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

	if ((per_tick == 0) || (GET_VITALITY(ch) == GET_MAX_VITALITY(ch) && per_tick > 0) || (GET_VITALITY(ch) < 0 && per_tick < 0))
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
	float regen_value     = *((float *)data);
	int   regen_value_int = (int)regen_value;

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
		gmcp_char_vitals(ch);
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
	int             delay, per_tick;

	if (type == EVENT_MOVE_REGEN)
	{
		func = event_move_regen;
		if (get_scheduled(ch, func))
			return;
		per_tick = move_regen(ch, FALSE);
		delay    = IS_PC(ch) ? 1 : MOB_MOVE_REGEN_DELAY;
	}
	else if (type == EVENT_HIT_REGEN)
	{
		func = event_hit_regen;
		if (get_scheduled(ch, func))
			return;
		per_tick = hit_regen(ch, FALSE);
		delay    = 1;
	}
	else if (type == EVENT_MANA_REGEN)
	{
		func = event_mana_regen;
		if (get_scheduled(ch, func))
			return;
		// codemod remove multiplicate of mana_regen
		per_tick = mana_regen(ch, FALSE);

		delay = IS_PC(ch) ? 1 : MOB_MANA_REGEN_DELAY;
	}
	else if (type == EVENT_WARD_REGEN)
	{
		func = event_ward_regen;
		if (get_scheduled(ch, func))
			return;
		per_tick = ward_regen(ch, FALSE);
		delay    = IS_PC(ch) ? 1 : MOB_WARD_REGEN_DELAY;
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
	if (!ch)
	{
		logit(LOG_EXIT, "event_wait called in events.c with no ch");
		return;
	}
	if (ch) // Just making sure.
	{
		if (ch->specials.act2, PLR2_WAIT)
		{
			REMOVE_BIT(ch->specials.act2, PLR2_WAIT);
		}
		if (ch->in_room != NOWHERE)
		{
			update_pos(ch);
		}
	}
}

void DelayCommune(P_char ch, int delay)
{
	if (USES_SPELL_SLOTS(ch))
	{
		P_nevent e = get_scheduled(ch, event_memorize);
		if (e)
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

	if (!ch)
	{
		logit(LOG_EXIT, "CharWait called in events.c with no ch");
		return;
	}

	if (!IS_ALIVE(ch))
	{
		REMOVE_BIT(ch->specials.act2, PLR2_WAIT);
		debug("CharWait: Dead char: %s", J_NAME(ch));
		return;
	}

	if (!CAN_ACT(ch))
	{
		// The event event_wait just turns off the PLR2_WAIT bit (and updates position).
		e = get_scheduled(ch, event_wait);
		if (e)
		{
			// If the new delay is shorter than the current, ignore new.
			//   Note: If you want delays to stack, change this to to e->timer += delay (I think that'll work).
			if (ne_event_time(e) >= delay)
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
	if (!IS_TRUSTED(ch))
	{
		SET_BIT(ch->specials.act2, PLR2_WAIT);
	}
	add_event(event_wait, delay, ch, 0, 0, 0, 0, 0);
}

void event_reset_zone(P_char ch, P_char victim, P_obj obj, void *data)
{
	int zone = *((int *)data);
	zone_table[zone].age++;

	if (zone_table[zone].age >= zone_table[zone].lifespan && (zone_table[zone].reset_mode == 2 || ::is_empty(zone)))
	{
		reset_zone(zone, 0);
	}
	else if (!zone_table[zone].reset_mode)
	{
		no_reset_zone_reset(zone);
		return;
	}

	add_event(event_reset_zone, PULSES_IN_TICK, 0, 0, 0, 0, &zone, sizeof(zone));
}

void room_event(P_char ch, P_char victim, P_obj obj, void *data)
{
	P_room room = &world[*((int *)data)];

	if (room && room->funct)
	{
		(room->funct)(room->number, 0, 0, 0);
		add_event(room_event, PULSE_MOBILE + number(-4, 4), 0, 0, 0, 0, data, sizeof(int));
	}
}
