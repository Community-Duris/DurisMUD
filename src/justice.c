/******************************************************/
/* Justice.c                                          */
/******************************************************/

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include "justice.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "graph.h"
#include "mm.h"
#include "sound.h"
#include "spells.h"

extern P_room                world;
extern struct time_info_data time_info;
extern struct zone_data     *zone_table;
extern int                   top_of_zone_table;
extern int                   mini_mode;
extern const char           *race_types[];
extern P_char                character_list;
extern P_event               current_event;
extern P_index               mob_index;
extern P_event               event_type_list[LAST_EVENT];
extern P_desc                descriptor_list;
extern const char           *town_name_list[];
// extern int rev_dir[];

void check_item(P_char);

/* number of seconds after a person is outcast that they can be
   pardoned for it */

struct hometown_data hometowns[LAST_HOME] = {
	/* flags, report_room, guard_room[5], guard_mob, jail_room,
     */

	/* 1. Tharnadia */
	{   JUSTICE_GOODHOME, 132618, {133050, 132628, 133057, 133065, 132504}, 132546, 132619},

	/* 2. iliithid  town */
	{JUSTICE_NEUTRALHOME,      0,                      {96421, 0, 0, 0, 0},  96421,      0},

	/* 3. drow town */
	{   JUSTICE_EVILHOME,      0,              {36572, 36563, 36544, 0, 0},  36436,      0},

	/* 4. elftown */
	{   JUSTICE_GOODHOME,   8096,                 {8096, 8087, 8214, 0, 0},   8025,   8097},

	/* 5. dwarf town */
	{   JUSTICE_GOODHOME,  95643,              {95540, 95522, 95650, 0, 0},  95532,  95644},

	/* 6. duergar */
	{   JUSTICE_EVILHOME,      0,                      {17173, 0, 0, 0, 0},  17173,      0},

	/* 7. halfling */
	{   JUSTICE_GOODHOME,  16512,      {16506, 16522, 16526, 16653, 16551},  16705,  16513},

	/* 8. gnome town */
	{   JUSTICE_GOODHOME,  66069,                      {66091, 0, 0, 0, 0},  66001,  66074},

	/* 9. faange (ogres) */
	{   JUSTICE_EVILHOME,      0,                      {15231, 0, 0, 0, 0},  15231,      0},

	/* 10. ghore (trolls) */
	{   JUSTICE_EVILHOME,      0,                      {11503, 0, 0, 0, 0},  11503,      0},

	/* 11. "ugta"  (barb) */
	{   JUSTICE_GOODHOME,  39310,          {39100, 39109, 39166, 39310, 0},  39105,  39341},

	/* 12. bloodstone... */
	{JUSTICE_LEVEL_CHAOS,      0,                          {0, 0, 0, 0, 0},      0,      0},

	/* 13. "shady"... orc town */
	{   JUSTICE_EVILHOME,      0,      {97682, 97569, 97682, 97607, 97612},  97562,      0},

	/* 14 "nax" - mino hometown */
	{JUSTICE_LEVEL_CHAOS,      0,                      {37701, 0, 0, 0, 0},  37701,      0},

	/* 15 "fort marigot" (centaur) */
	{   JUSTICE_GOODHOME,   5307,                 {5319, 5306, 5371, 0, 0},   5340,   5308},

	/* 16  new elftown */
	{   JUSTICE_GOODHOME,  45006,      {45189, 45205, 45152, 45153, 45036},  45030,  45036},

	//  17 "Ancient City Ruins" (good undead)
	{JUSTICE_LEVEL_CHAOS,      0,                      {66201, 0, 0, 0, 0},  66201,      0},

	//  18 "payang" (evil undead)
	{JUSTICE_LEVEL_CHAOS,      0,                      {90341, 0, 0, 0, 0},  90313,      0},

	//  19 "Githyanki Fortress",
	{   JUSTICE_EVILHOME,      0,                      {19400, 0, 0, 0, 0},  19400,      0},

	//  20 "Moregeeth",
	{   JUSTICE_EVILHOME,      0,                      {70187, 0, 0, 0, 0},  70044,      0},

	//  "Harpy",
	{JUSTICE_LEVEL_CHAOS,      0,                          {0, 0, 0, 0, 0},      0,      0},
};

struct mm_ds              *dead_justice_guard_pool = NULL;
struct mm_ds              *dead_witness_pool       = NULL;
struct mm_ds              *dead_crime_pool         = NULL;
struct justice_guard_list *guard_list              = NULL;

const char *justice_flags[] = {"Evil",    /* JUSTICE_EVILHOME */
                               "Good",    /* JUSTICE_GOODHOME */
                               "Harsh",   /* JUSTICE_LEVEL_HARSH */
                               "Chaotic", /* JUSTICE_LEVEL_CHAOS */
                               "Undead",
                               "Neutral",
                               "\n"};

int justice_send_guards(int to_rroom, P_char victim, int how_many)
{
	struct justice_guard_list *gl;
	int                        best_dist = 9999;
	int                        best_room = NOWHERE;
	int                        i, town;
	int                        hunt_type;
	hunt_data                  data;
	int                        ht = CHAR_IN_TOWN(victim);

	// WIPE2013 - Drannak
	return FALSE; // disabling

	// Only good hts get guards atm.
	if (ht <= 0 || ht > LAST_HOME || !IS_SET(hometowns[ht - 1].flags, JUSTICE_GOODHOME))
	{
		wizlog(56, "Justice: %s not in good ht %d; no guards dispensed.", GET_NAME(victim), ht);
		return FALSE;
	}

	if ((to_rroom == NOWHERE) && !victim)
		return FALSE;

	/* figure out how many are REALLY needed (ie: aren't already doing
	   it) */
	for (gl = guard_list; gl; gl = gl->next)
	{

		/* justice guards that have the same victim as we'd set, and the
		   same type flag that we'd set are counted as part of how_many */
		if (/*gl->ch->only.npc && */ (GET_STAT(gl->ch) != STAT_DEAD) && IS_NPC(gl->ch) && IS_SET(gl->ch->only.npc->spec[2], MOB_SPEC_JUSTICE) && (gl->ch->specials.arrest_by == victim))
			how_many--;
	}

	/* if the original how_many request is already being done, then
	   don't do any more */
	if (how_many <= 0)
		return FALSE;

	/* figure out what room we are going to, and what town thats in */
	if (victim && (to_rroom == NOWHERE))
	{
		to_rroom = victim->in_room;
		if (to_rroom == NOWHERE)
			return FALSE;
	}
	town = zone_table[world[to_rroom].zone].hometown;

	/* now find the best place to send the guards FROM */
	for (i = 0; i < NUM_EXITS; i++)
	{
		int rr, dist, dir;

		rr = real_room(hometowns[town - 1].guard_room[i]);
		if (rr == NOWHERE)
			continue;
		dir = find_first_step(rr, to_rroom, BFS_CAN_FLY | BFS_CAN_DISPEL | BFS_AVOID_NOMOB, 0, 0, &dist);
		if (dir == BFS_ALREADY_THERE)
			dist = 0;
		else if (dir < 0)
			continue;
		if (dist < best_dist)
		{
			best_room = rr;
			best_dist = dist;
		}
	}

	if (best_room == NOWHERE)
	{
		wizlog(56, "Justice: No guards able to get to room %d. Tracking %s in room %d.", world[to_rroom].number, GET_NAME(victim), world[victim->in_room].number);
		return FALSE;
	}
	/* This sets up the type of hunter event, based on the 'type' mob
	   spec[2] passed. */

	hunt_type = HUNT_JUSTICE_INVADER;

	// Can comment this out later.
	wizlog(56, "Justice: Dispatching %d guard(s) for %s.", how_many, GET_NAME(victim));

	/* now send them out! */
	while (how_many)
	{
		P_char tch;

		tch = justice_make_guard(best_room);
		if (!tch)
		{
			wizlog(56, "Justice: Unable to load mob vnum %d. Tracking: %s in room %d.", hometowns[town - 1].guard_mob, GET_NAME(victim), world[victim->in_room].number);
			return FALSE;
		}

		tch->specials.arrest_by = victim;

		data.hunt_type = hunt_type;
		if (victim)
			data.targ.victim = victim;
		else
			data.targ.room = to_rroom;

		data.huntFlags = BFS_CAN_FLY | BFS_BREAK_WALLS;
		if (npc_has_spell_slot(tch, SPELL_DISPEL_MAGIC))
			data.huntFlags |= BFS_CAN_DISPEL;
		data.retry     = 0;
		data.retry_dir = 0;
		data.path_step = -1;

		add_event(event_mob_hunt, PULSE_MOB_HUNT, tch, NULL, NULL, 0, &data, sizeof(hunt_data));
		// AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, tch, data);
		how_many--;

	} /* while() */
	return TRUE;
}

P_char justice_make_guard(int rroom)
{
	/* assumes rroom is a valid real room number */
	int                        vnum, town;
	P_char                     ch;
	P_obj                      obj = NULL;
	struct justice_guard_list *gl;

	vnum = hometowns[zone_table[world[rroom].zone].hometown - 1].guard_mob;

	ch = read_mobile(vnum, VIRTUAL);

	if (!ch)
		return NULL;

	/* make DAMN sure the birthplace is set... I need this to get back
	   home when I'm done with my duty */
	GET_BIRTHPLACE(ch) = world[rroom].number;
	char_to_room(ch, rroom, 0);

	// clear money
	CLEAR_MONEY(ch);

	/* standard clause */
	if (GET_HIT(ch) > GET_MAX_HIT(ch))
		GET_HIT(ch) = GET_MAX_HIT(ch);

	/* justice guards that don't have the ability to see in the dark
	   need torches... this takes care of that */
	if (!IS_AFFECTED2(ch, AFF2_ULTRAVISION))
	{
		obj = read_object(398, VIRTUAL);
		if (obj)
			equip_char(ch, obj, HOLD, 0);
	}
	/* justice guards should always be aggro to outcasts! */
	/*  if (!IS_SET(ch->specials.act, ACT_AGG_OUTCAST))
	    SET_BIT(ch->specials.act, ACT_AGG_OUTCAST);*/
	if (!IS_AGGROFLAG(ch, AGGR_OUTCASTS))
		SET_BIT(ch->only.npc->aggro_flags, AGGR_OUTCASTS);

	/* however, don't rely on the "standard" protector code for these
	   special guards... if I want them to do something similar, I'll
	   code it in JusticeGuardAct() */
	if (IS_SET(ch->specials.act, ACT_PROTECTOR))
		REMOVE_BIT(ch->specials.act, ACT_PROTECTOR);

	/* also, I don't want guards that I control to be hunting on their
	   own, so nuke HUNTER flags */
	if (IS_SET(ch->specials.act, ACT_HUNTER))
		REMOVE_BIT(ch->specials.act, ACT_HUNTER);

	// set them to always be able to fly.
	SET_BIT(ch->specials.affected_by, AFF_FLY);

	town = CHAR_IN_TOWN(ch);

	/* justice guards in hometowns should be aggro to evil/good based on
	   the hometown flags */

	if (IS_SET(hometowns[town - 1].flags, JUSTICE_EVILHOME))
		//    SET_BIT(ch->specials.act, ACT_AGG_RACEGOOD);
		SET_BIT(ch->only.npc->aggro_flags, AGGR_GOOD_RACE);

	if (IS_SET(hometowns[town - 1].flags, JUSTICE_GOODHOME))
		//    SET_BIT(ch->specials.act, ACT_AGG_RACEEVIL);
		SET_BIT(ch->only.npc->aggro_flags, AGGR_EVIL_RACE);

	// 1 in 4 chance that warriors are changed to clerics.  This is used for
	// dispelling walls, and just neat that a group of 5 should have a cleric
	if (!npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) && !number(0, 3))
	{
		ch->player.level   = 61;
		ch->player.m_class = CLASS_CLERIC;
	}

	/* give them some basic eq */
	// load_obj_to_newbies(ch);

	/* finally, put the guard in the "master" list of guards that
	   justice controls.  This allows me to loop through just those mobs
	   faster when I'm looking for a particular justice mob.  It also
	   provides a nice monitoring system via the mm_* debugging code. */

	if (!dead_justice_guard_pool)
		dead_justice_guard_pool = mm_create("JUSTICE", sizeof(struct justice_guard_list), offsetof(struct justice_guard_list, next), 1);
	gl         = (struct justice_guard_list *)mm_get(dead_justice_guard_pool);
	gl->ch     = ch;
	gl->next   = guard_list;
	guard_list = gl;

	return ch;
}

/* this function needed for extract_char() in case a justice mob is
   purged, etc */

void justice_guard_remove(P_char ch)
{
	struct justice_guard_list *gl;

	if (IS_PC(ch) || !ch->only.npc || (!IS_SET(ch->only.npc->spec[2], MOB_SPEC_JUSTICE)))
		return;

	/* pull the guard from the master list of justice controlled guards
	 */

	if (!guard_list)
		return;
	if (!ch)
		return;

	if (ch == guard_list->ch)
	{
		gl         = guard_list;
		guard_list = gl->next;
		mm_release(dead_justice_guard_pool, gl);
	}
	else
	{
		for (gl = guard_list; gl && gl->next; gl = gl->next)
			if (ch == gl->next->ch)
			{
				struct justice_guard_list *t = gl->next;

				gl->next = t->next;
				mm_release(dead_justice_guard_pool, t);
			}
	}
}

void justice_delete_guard(P_char ch)
{
	int   i;
	P_obj obj;

	if (IS_PC(ch) || (!IS_SET(ch->only.npc->spec[2], MOB_SPEC_JUSTICE)))
		return;

	/* Justice guards which are removed via this function haven't been
	   killed or purged, but are "done" with their mission.  As well,
	   anything they are wearing is stuff I put on them when they
	   loaded.  Instead of having that stuff just drop to the ground,
	   which could result in a mess, just nuke it */

	for (i = 0; i < MAX_WEAR; i++)
		if (ch->equipment[i])
		{
			obj = unequip_char(ch, i);
			extract_obj(obj, TRUE); // If they've managed to pick up an arti.
		}
	/* similar story for stuff they are carrying.  While stuff they are
	   carrying was probably given to them, just dropping it makes a
	   mess... so... I'll just nuke it :) */

	if (ch->carrying)
	{
		P_obj next_obj;

		for (obj = ch->carrying; obj; obj = next_obj)
		{
			next_obj = obj->next_content;
			extract_obj(obj, TRUE); // If they've managed to pick up an arti.
		}
	}
	/* delete the char.  This will have the side effect of calling
	   justice_guard_remove() which will deal with taking the guard off
	   the master guard list. */

	extract_char(ch);
}

void justice_action_invader(P_char ch)
{
	struct zone_data *zone_struct;
	int               room;

	if (IS_TRUSTED(ch))
		return;

	/*
	  if (mini_mode)
	   return;
	*/

	if (!CHAR_IN_TOWN(ch))
		return;

	if (!IS_INVADER(ch))
		return;

	/*Original Justice
	if (!justice_send_guards(NOWHERE, ch, MOB_SPEC_J_OUTCAST, (MAX(11, GET_LEVEL(ch)) / 11) + 1))
	 {
	  return;
	 }
	*/

	zone_struct = &zone_table[world[ch->in_room].zone];
	room        = ch->in_room;

	//  Re-doing justice here.  Major idea is to get rid of the overkill response
	//  which doesn't allow for any hometown raiding, but still not make it far
	//  too easy for a group to sit in a hometown and frag people all day long.
	//  I've removed the constant spamming of the justice_raiding event that was
	//  causing mobs to constantly be spawned to hunt invaders.  Instead, it will
	//  be applied once only and give a 3 minute opportunity before it is refreshed.
	//  When it gets refreshed, I'll add a different event as a placeholder/counter.
	//  If this counter reaches 3x(10 minutes approx), I'll have a reinforcement
	//  squad issued from another hometown that will 'encourage' the invaders to
	//  leave.  Hopefully this works out better than justice ever has...  - Jexni 3/4/11

	if (IS_INVADER(ch) && IS_PC(ch) && IS_TOWN_RAIDED(ch))
	{
		if (!number(0, 1))
			return;
	}
	else
	{
		zone_struct->last_raid = time(0);
	}

	if (IS_INVADER(ch))
	{
		if (!number(0, 2))
		{
			justice_send_guards(NOWHERE, ch, 1);
		}

		if (IS_SET(hometowns[CHAR_IN_TOWN(ch) - 1].flags, JUSTICE_GOODHOME) && !(number(0, 15)) && get_property("justice.alarms.good", 1))
		{
			int rnum = number(1, 4);
			if (rnum == 1)
				justice_hometown_echo(CHAR_IN_TOWN(ch), "&+RAlarm bells sound, &+rsignalling an invasion!&n");
			if (rnum == 2)
				justice_hometown_echo(CHAR_IN_TOWN(ch), "&+LMilitia forces muster to bolster the town's defenses against the &=Lrinvaders!!!&n");
			if (rnum == 3)
				justice_hometown_echo(CHAR_IN_TOWN(ch), "&+LMilitia forces muster to bolster the town's defenses against the &=Lrinvaders!!!&n");
			if (rnum == 4)
				justice_hometown_echo(CHAR_IN_TOWN(ch), "&+RAlarm bells sound, &+rsignalling an invasion!&n");
			return;
		}
		else if (IS_SET(hometowns[CHAR_IN_TOWN(ch) - 1].flags, JUSTICE_EVILHOME) && !number(0, 15) && get_property("justice.alarms.evil", 1))
		{
			justice_hometown_echo(CHAR_IN_TOWN(ch), "&+yHorns begin to &+Ybellow &+yand drums &+cthunder&n &+yto the &+Rcall to arms!&n");
			return;
		}
	}
}

/*
 * shout_and_hunt().  This function is the result of some of the justice
 * code which has been kludged to work with other, non-hometown, mobs.
 * Its intended to be called from within the special procedure of a mob.
 * For an example, check 'sister_knight()' in specs.mobile.c.
 *
 * ch - pointer to me.. the mob the proc was called for. max_distance -
 * mobs which are greater then this distance away won't come to help.
 * This MUST be specified as greater then 0. shout_str - this is a string
 * which the mob will shout when its being attacked.  It must include a
 * single %s which will be replaced by the name of the char attacking.
 * locator_proc - This parameter, if specified as non-NULL should be the
 * address of a function.  Only mobs which have this function as a special
 * will run to help. vnums - This is a pointer to a list of vnums.  I
 * think the best way to specify it would be "{num1, num2, num3, 0}".  In
 * this case, only mobs with vnums of num1, num2, and num3 will respond.
 * The last vnum in the list MUST be 0.  If you don't want to limit based
 * on vnum, pass NULL here.  As a special note:  specifying a long list of
 * vnums here will lag the mud.  Don't do it.  This method should ONLY be
 * used in the cases discussed below within the function. act_mask - If
 * non-0, mobs which respond must have these bits set in their ACT flags.
 * For example, passing (ACT_HAS_MU | ACT_HAS_WA) would tell the function
 * to only have mobs which have BOTH of those act flags to come running.
 * no_act_mask - if non-0, mobs which respond must NOT have ANY of these
 * bits set in their ACT flags.  It would be foolish to specify the same
 * bit in both the act_mask, and the no_act_mask.  Therefore, the function
 * will segfault and blame you if you do it.
 */

int shout_and_hunt(P_char ch, int max_distance, const char *shout_str, int (*locator_proc)(P_char, P_char, int, char *), int vnums[], ulong act_mask, ulong no_act_mask)
{
	P_char    target;
	P_nevent  ev;
	char      buffer[MAX_STRING_LENGTH], buffer2[MAX_STRING_LENGTH];
	hunt_data data;
	int       dummy;
	int       i;
	bool      has_help   = FALSE;
	bool      found_help = FALSE;

	if (!ch || !max_distance || (act_mask & no_act_mask))
	{
		logit(LOG_EXIT, "shout_and_hunt proc called with bogus params");
		return FALSE;
	}

	if (IS_PC(ch))
	{
		return FALSE;
	}

	if (!IS_FIGHTING(ch))
	{
		return FALSE;
	}

	/*
	 * if its quiet in here, can't shout for help
	 */
	if (IS_ROOM(ch->in_room, ROOM_SILENT))
	{
		return FALSE;
	}
	/*
	 * if paralyzed, casting, or sleeping, can't shout for help
	 */
	if (IS_IMMOBILE(ch) || IS_CASTING(ch) || GET_STAT(ch) == STAT_SLEEPING)
	{
		return FALSE;
	}
	/*
	 * how do I keep from shouting every round?
	 */

	// Attack the master and shout for the master not the pet.
	if (IS_NPC(GET_OPPONENT(ch)) && IS_PC_PET(GET_OPPONENT(ch)) && GET_OPPONENT(ch)->in_room == (GET_MASTER(GET_OPPONENT(ch)))->in_room)
	{
		GET_OPPONENT(ch) = GET_MASTER(GET_OPPONENT(ch));
	}

	/*
	 * party time
	 */
	/*
	 * first shout for help.  Note that I'm going to shout even if I'm
	 * bashed.  Just because I'm sitting on my butt doesn't mean I can
	 * scream for help
	 */
	snprintf(buffer, MAX_STRING_LENGTH, "%s shouts '", ch->player.short_descr);
	snprintf(buffer2, MAX_STRING_LENGTH, shout_str, CAN_SEE(ch, GET_OPPONENT(ch)) ? J_NAME(GET_OPPONENT(ch)) : "Someone");
	strcat(buffer, buffer2);
	strcat(buffer, "&n'\n");

	/*
	 * Replacing this with new generic function, ha.
	 * Also, i think it's the first place this new function is going to be used, go me!

	  do_sorta_yell(ch, buffer);
	*/
	int shout_distance = MIN(RMFR_MAX_RADIUS, max_distance);

	if (!ch->in_room)
	{
		// REMOVE_BIT on a non-existant room?!
		//    REMOVE_BIT(world[ch->in_room].room_flags, SINGLE_FILE);
		logit(LOG_DEBUG, "Problem in shout_and_run() for %s - bogus room", GET_NAME(ch));
		wizlog(MINLVLIMMORTAL, "Problem in shout_and_run() for %s - bogus room", GET_NAME(ch));
		return FALSE;
	}
	/*
	 * Decided to let it be heard only inside the same zone, as it is now.
	 */
	act(buffer, TRUE, ch, 0, 0, TO_ROOM);
	radiate_message_from_room(ch->in_room, buffer, shout_distance, (RMFR_FLAGS)(RMFR_RADIATE_ALL_DIRS | RMFR_PASS_WALL), 0);

	/*
	 * need to find all the mobs which 'locator proc' as their special
	 * procedure.  Unfortunatly, there is only 1 way to do this: going
	 * through the entire character_list.  For all mobs which have the
	 * proper proc, if they aren't already hunting, have them start :)
	 */

	for (target = character_list; target; target = target->next)
	{
		if (IS_NPC(target) && (!locator_proc || (mob_index[GET_RNUM(target)].func.mob == locator_proc)) && !((act_mask & target->specials.act) ^ act_mask) && !(no_act_mask & target->specials.act))
		{

			i = 0;

			/*
			 * if a vnum list was provided, check it.  Note that this is
			 * the LEAST preferred method for finding a target mob, as if
			 * more then 1 vnum is provided, its the slowest.  However, if
			 * alot of the mobs involved have their own specialized
			 * procedure, or if you wish some mobs to come running who
			 * don't shout for help, this is the only way of checking.  I
			 * don't like this. While its the best way to do it, its the
			 * most likely part of this function to lag the mud while it
			 * compares an entire list of vnums to every fucking mob in
			 * the game.  If you have 10,000 mobs in the game, and send
			 * down a list of only 5 vnums, it results in 50,000
			 * comparisms.  In contrast, using only the locator_proc
			 * and/or act_mask methods only check each mob ONCE.  Gond
			 * requested that this function be able to work with a list of
			 * vnums...  blame him!  (neb)
			 */

			if (vnums)
			{
				while ((vnums[i] != mob_index[GET_RNUM(target)].virtual_number) && vnums[++i])
					;
				if (!vnums[i])
					continue;
			}
			/*
			 * got one.  If they aren't already hunting someone, make
			 * things happen
			 */

			if (IS_FIGHTING(target))
				continue;

			// debug( "shout_and_hunt: NPC vnum %d.", mob_index[GET_RNUM(target)].virtual_number );

			LOOP_EVENTS_CH(ev, target->nevents)
			{
				if (ev->func == event_mob_hunt)
				{
					break;
				}
			}
			if (ev)
				continue;

			has_help = TRUE;

			/*
			 * check if its a possible hunt.. and if its within the max
			 * distance
			 */

			//       debug("Okay boss mob is shouting at this point and we calling for find_first_step in justice.c 2421");
			if (find_first_step(target->in_room,
			                    ch->in_room,
			                    (IS_MAGE(target) ? BFS_CAN_FLY : 0) | (npc_has_spell_slot(target, SPELL_DISPEL_MAGIC) ? BFS_CAN_DISPEL : 0) | BFS_BREAK_WALLS | BFS_AVOID_NOMOB,
			                    0,
			                    0,
			                    &dummy) < 0)
			{
				//        debug( "shout_and_hunt: mob (%s) no first step to room (%d).", J_NAME(target), ch->in_room);
				continue;
			}

			if (dummy > max_distance)
			{
				//        debug( "shout_and_hunt: mob (%s) too far to hunt to room (%d).", J_NAME(target), ch->in_room);
				continue;
			}

			if (CAN_SEE(ch, GET_OPPONENT(ch)))
			{
				data.hunt_type   = HUNT_JUSTICE_INVADER;
				data.targ.victim = GET_OPPONENT(ch);
			}
			else
			{
				data.hunt_type = HUNT_ROOM;
				data.targ.room = ch->in_room;
			}
			data.huntFlags = 0;
			data.retry     = 0;
			data.retry_dir = 0;
			data.path_step = -1;
			found_help     = TRUE;
			//    debug( "shout_and_hunt: adding hunt event mob (%s) to room (%d).", J_NAME(target), ch->in_room);
			add_event(event_mob_hunt, PULSE_MOB_HUNT, target, NULL, NULL, 0, &data, sizeof(hunt_data));
			// AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, target, data);
		}
	}

	// Flee if no help can get to ch.
	if (has_help && !found_help)
		do_flee(ch, "", CMD_FLEE);

	/*
	 * okay... last thing to do is add this "tank" to the mobs justice
	 * memory so we don't shout about him/her again
	 */

	return TRUE;
}

/* this function was needed so that (for justice purposes), mobs could
   shout and it would be "heard" even if the mob was indoors. */

void do_sorta_yell(P_char ch, char *str)
{
	P_desc i;
	char   Gbuf1[MAX_STRING_LENGTH];

	if (IS_PC(ch))
	{
		send_to_char("Sorry, only mobs can sort of yell.  You'll have to shout\r\n", ch);
		return;
	}
	if (!str)
		return;

	for (i = descriptor_list; i; i = i->next)
	{
		if (i->character && (i->character != ch) && !is_silent(i->character, FALSE) && !IS_SET(i->character->specials.act, PLR_NOSHOUT) && !i->connected)
			if (world[i->character->in_room].zone == world[ch->in_room].zone)
			{
				snprintf(Gbuf1, MAX_STRING_LENGTH, "$n shouts %s'%s'", language_known(ch, i->character), language_CRYPT(ch, i->character, str));
				act(Gbuf1, 0, ch, 0, i->character, TO_VICT);
			}
	}

	if (ch->desc)
		ch->desc->last_input[0] = '\0';
}

/* function to echo 'str' to anyone in 'town' hometown (who's awake)  */

void justice_hometown_echo(int town, const char *str)
{
	P_desc d;

	if (!str || !*str)
		return;

	for (d = descriptor_list; d; d = d->next)
		if ((d->connected == CON_PLAYING) && (CHAR_IN_TOWN(d->character) == town) && (IS_AWAKE(d->character)))
		{
			send_to_char(str, d->character);
			send_to_char("\r\n", d->character);
		}
}

/* fonction to load the outside room that are under justice patrol */
void load_justice_area(void)
{
	FILE *fl;
	int   the_end = TRUE, i;
	int   town_number, room_number, room_number1;
	char  type_rec;

	if (!(fl = fopen(JUSTICE_FILE, "r")))
	{
		perror("fopen");
		logit(LOG_FILE, "justice_area: could not open file.");
		logit(LOG_SYS, "justice_area: could not open file.");
		return;
	}

	do
	{

		fscanf(fl, " %c ", &type_rec);
		if (type_rec == 'H')
			fscanf(fl, " %d\n", &town_number);
		else if (type_rec == 'R')
		{
			fscanf(fl, " %d %d\n", &room_number, &room_number1);
			for (i = room_number; i <= room_number1; i++)
				world[real_room0(i)].justice_area = town_number;
		}
		else if (type_rec == 'U')
		{
			fscanf(fl, " %d\n", &room_number);
			world[real_room0(room_number)].justice_area = town_number;
		}
		else if (type_rec == '$')
			the_end = FALSE;

	} while (the_end);
	fclose(fl);

	return;
}
