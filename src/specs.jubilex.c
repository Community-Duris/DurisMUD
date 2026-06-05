#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include "specs.jubilex.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "damage.h"
#include "range.h"
#include "spells.h"

/*
 * external variables
 */

extern P_char      character_list;
extern P_desc      descriptor_list;
extern P_index     mob_index;
extern P_index     obj_index;
extern P_room      world;
extern P_obj       justice_items_list;
extern char       *coin_names[];
extern const char *command[];
extern const char *dirs[];
// extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int                    planes_room_num[];
extern int                    racial_base[];
extern int                    top_of_zone_table;
extern struct command_info    cmd_info[MAX_CMD_LIST];
extern struct str_app_type    str_app[];
extern struct time_info_data  time_info;
extern struct zone_data      *zone;
extern struct zone_data      *zone_table;
extern const char            *specdata[][MAX_SPEC];
extern struct class_names     class_names_table[];
int                           range_scan_track(P_char ch, int distance, int type_scan);
extern P_obj                  object_list;
extern Skill                  skills[];
extern char                  *spells[];

int slime_lake(P_char ch, P_char pl, int cmd, char *arg)
{
	if (cmd == CMD_DEATH)
	{
		P_obj obj1 = read_object(CLEANSED_LAKE_VNUM, VIRTUAL);
		if (!obj1)
		{
			logit(LOG_DEBUG, "slime_lake: object failed to load.");
			debug("slime_lake: object failed to load.");
			return FALSE;
		}

		P_obj obj2 = read_object(SLIME_SHEEN_VNUM, VIRTUAL);
		if (!obj2)
		{
			logit(LOG_DEBUG, "slime_lake: object failed to load.");
			debug("slime_lake: object failed to load.");
			return FALSE;
		}

		act("&+gWith a huge torrent of &+rvile &+genergies, the putrid &+Levil "
		    "&+ginfesting the lake exits the room in a deafening explosion!&n",
		    FALSE,
		    ch,
		    0,
		    0,
		    TO_ROOM);

		obj_to_room(obj1, ch->in_room);
		obj_to_room(obj2, ch->in_room);
		return (FALSE);
	}

	return FALSE;
}

int jubilex_one(P_char ch, P_char pl, int cmd, char *arg)
{
	if (cmd == CMD_DEATH)
	{
		P_obj obj = read_object(JUBILEX_WORMHOLE_VNUM, VIRTUAL);
		if (!obj)
		{
			logit(LOG_DEBUG, "slime_lake: object failed to load.");
			debug("slime_lake: object failed to load.");
			return FALSE;
		}

		obj_to_room(obj, ch->in_room);

		act("&+gThe faceless lord seems to reel in &+Lterror&+g, and "
		    "makes a hasty retreat for reinforcements!&n",
		    FALSE,
		    ch,
		    obj,
		    0,
		    TO_ROOM);

		act("$p &nsuddenly glows brightly!\n $n &nslowly fades out of existence.", FALSE, ch, obj, 0, TO_ROOM);

		P_char tch, temp;
		for (tch = world[real_room0(JUBILEX_DEATH_FROM_ROOM)].people; tch; tch = temp)
		{
			temp = tch->next_in_room;

			if (IS_NPC(tch))
			{
				char_to_room(tch, real_room0(JUBILEX_DEATH_TO_ROOM), -1);
			}
		}

		return (FALSE);
	}

	return FALSE;
}

int mask_of_wildmagic(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC)
	{
		if (!OBJ_WORN(obj))
			return FALSE;

		ch = obj->loc.wearing;
		if (!ch)
		{
			return FALSE;
		}

		if (!number(0, 5))
		{
			int msg = number(0, 2);
			switch (msg)
			{
				case 0:
					act("&+LYou are momentarily blinded as powerful &+Bsparks &+Lof &+bma&+Wgi&+bc &+Lleap forth from your mask.&n", FALSE, ch, obj, 0, TO_CHAR);
					act("&+LPowerful &+Bsparks &+Lof &+bma&+Wgi&+bc &+Lseem to flow out from $n's mask.&n", FALSE, ch, obj, 0, TO_ROOM);
					break;
				case 1:
					act("&+LYou suddenly feel a rush of &+wsup&+Wern&+watural &+rheat &+Lsurround your body.&n", FALSE, ch, obj, 0, TO_CHAR);
					act("&+L$n &+Lis suddenly surrounded by a &+rred &+Geldritch &+Laura, which quickly dissipates.&n", FALSE, ch, obj, 0, TO_ROOM);
					break;
				case 2:
					act("&+LYour eyes seem to be playing tricks on you as you see strange &+Mripples &+Lin &+Wreality&+L.&n", FALSE, ch, obj, 0, TO_CHAR);
					act("&+L$n's &+Mmask &+Wsparkles &+Lfor a moment, and reality seems to twist and bend.&n", FALSE, ch, obj, 0, TO_ROOM);
					break;
			}
		}
		return TRUE;
	}
	return FALSE;
}

int ebb_vambraces(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || !obj)
		return FALSE;

	if (!OBJ_WORN_BY(obj, ch))
		return FALSE;

	if (arg && (cmd == CMD_REMOVE))
	{
		if (isname(arg, obj->name) || isname(arg, "all"))
		{
			affect_from_char(ch, SPELL_INERTIAL_BARRIER);
			return FALSE;
		}
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "ebb"))
		{
			curr_time = time(NULL);

			if (obj->timer[0] + 600 <= curr_time)
			{
				act("$p hums loudly!", FALSE, ch, obj, 0, TO_CHAR);
				act("$n's $p hums loudly!", FALSE, ch, obj, 0, TO_ROOM);

				struct affected_type af;
				bzero(&af, sizeof(af));
				af.type       = SPELL_INERTIAL_BARRIER;
				af.bitvector3 = AFF3_INERTIAL_BARRIER;
				af.modifier   = 0;
				af.duration   = 5;
				affect_to_char(ch, &af);

				obj->timer[0] = curr_time;
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}

	return FALSE;
}

void event_flow_amulet_vibrate(P_char ch, P_char victim, P_obj obj, void *data)
{
	if (OBJ_WORN(obj) && obj->loc.wearing)
	{
		act("$p &+rvibrates&+b quietly.&n", FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
		SET_BIT(obj->extra_flags, ITEM_HUM);
	}
	else
	{
		act("&+bThe $q &+rvibrates &+bquietly.&n", FALSE, 0, obj, 0, TO_NOTVICT);
		SET_BIT(obj->extra_flags, ITEM_HUM);
	}
}

int flow_amulet(P_obj obj, P_char ch, int cmd, char *arg)
{
	int  circle;
	char buf[256];
	int  curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "flow"))
		{
			curr_time = time(NULL);
			// 5 min 50 sec timer ?!?
			if (obj->timer[0] + 350 <= curr_time)
			{
				act("You say 'flow'", FALSE, ch, 0, 0, TO_CHAR);
				act("$p&+B hums loudly!&n", FALSE, ch, obj, 0, TO_CHAR);

				act("$n says 'flow'", TRUE, ch, obj, NULL, TO_ROOM);
				act("&+B$n&+B's $p &+Bhums loudly!&n", FALSE, ch, obj, 0, TO_ROOM);

				if (USES_SPELL_SLOTS(ch))
				{
					for (circle = get_max_circle(ch); circle >= 1; circle--)
					{
						if (ch->specials.undead_spell_slots[circle] < max_spells_in_circle(ch, circle))
						{
							// debug("circle slot: %d", circle); some zion debug nonsense, can re-enable it if you wish.
							snprintf(buf, 256, "&+LPowerful magic flows into your %d%s circle of knowledge.\n", circle, circle == 1 ? "st" : (circle == 2 ? "nd" : (circle == 3 ? "rd" : "th")));
							send_to_char(buf, ch);
							ch->specials.undead_spell_slots[circle]++;
							break;
						}
					}
				}
				else
				{
					int spell = memorize_last_spell(ch);

					if (spell)
					{
						snprintf(buf, 256, "&+BPowerful magic flows forth from the amulet, and your power of %s &+Breturns to you!&n\n", skills[spell].name);
						send_to_char(buf, ch);
					}
				}
				obj->timer[0] = curr_time;
				REMOVE_BIT(obj->extra_flags, ITEM_HUM);
				disarm_obj_nevents(obj, event_flow_amulet_vibrate);
				add_event(event_flow_amulet_vibrate, 350 * WAIT_SEC, 0, 0, obj, 0, 0, 0);
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}
	return FALSE;
}

int jubilex_grid_mob_generator(P_obj obj, P_char ch, int cmd, char *arg)
{
	int mob_vnums[] = {87507, 87505, 87508, 87604, 87599, 87504, 87506, 87515, 87552, 87514, 0};

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!obj || !OBJ_ROOM(obj))
		return FALSE;

	if (cmd == CMD_PERIODIC)
	{
		int curr_time = time(NULL);

		if (curr_time < obj->timer[0] + 180)
			return FALSE;

		/* go through the list of mob vnums and spawn a new one if the limit hasn't yet been reached */
		for (int i = 0; mob_vnums && mob_vnums[i] > 0; i++)
		{
			int rnum = real_mobile(mob_vnums[i]);
			if (mob_index[rnum].number < mob_index[rnum].limit)
			{
				P_char mob = read_mobile(rnum, REAL);
				char_to_room(mob, obj->loc.room, -1);
				act("&+cA vortex in the space-time continuum appears and $n&+c stumbles out.", FALSE, mob, obj, 0, TO_ROOM);
				obj->timer[0] = curr_time;
				break;
			}
		}

		return FALSE;
	}

	return FALSE;
}
