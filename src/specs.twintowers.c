/*
   Specs for Sehanine's series of twin towers zones.
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "specs.prototypes.h"
#include "spells.h"
#include "vnum.obj.h"

/*
   external variables
 */

extern P_char      character_list;
extern P_desc      descriptor_list;
extern P_index     mob_index;
extern P_index     obj_index;
extern P_room      world;
extern char       *coin_names[];
extern char       *command[];
extern const char *dirs[];
// extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int                    planes_room_num[];
extern int                    top_of_zone_table;
extern struct command_info    cmd_info[MAX_CMD_LIST];
extern struct zone_data      *zone;
extern struct zone_data      *zone_table;

int forest_animals(P_char ch, P_char pl, int cmd, char *arg)
{
	if (cmd == CMD_DEATH)
	{
		P_obj obj;

		obj = read_object(GET_VNUM(ch), VIRTUAL);
		if (!(obj))
		{
			logit(LOG_EXIT, "forest_animals: death object for mob %d doesn't exist", GET_VNUM(ch));
			return FALSE;
		}
		obj_to_room(obj, ch->in_room);

		/*
		   finally, set the val0 to mark the time until decay.  I realize this is a
		   kludgy way to do it, but I don't feel like writing a new event type (ie:
		   event_char_execute type event for objects)
		 */

		/*
		   should end up so that val0 is the number of EVENT_OBJ_SPECIAL periodic
		   calls it will get in a mud day.
		 */

		obj->value[0] = SECS_PER_MUD_DAY / PULSE_MOBILE * WAIT_SEC;

		return FALSE;
	}
	return FALSE;
}

int forest_corpse(P_obj obj, P_char ch, int cmd, char *args)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (ch || cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	if (!obj->value[0]--)
	{
		// Make it into a real corpse
		P_obj corpse;

		corpse = read_object(VOBJ_TTFOREST_ROTTING_CORPSE, VIRTUAL);
		if (!corpse)
		{
			logit(LOG_EXIT, "forest_corpse: unable to load obj #%d", VOBJ_TTFOREST_ROTTING_CORPSE);
			return FALSE;
		}
		corpse->weight = obj->weight;
		set_obj_affected(corpse, get_property("timer.decay.corpse.npc", 120), TAG_OBJ_DECAY, 0);

		if (OBJ_CARRIED(obj))
		{
			P_char carrier;

			carrier = obj->loc.carrying;
			send_to_char("Something smells real bad...\r\n", carrier);
			obj_to_char(corpse, carrier);
		}
		else if (OBJ_ROOM(obj))
		{
			send_to_room("Something smells real bad...\r\n", obj->loc.room);
			obj_to_room(corpse, obj->loc.room);
		}
		else if (OBJ_INSIDE(obj))
		{
			obj_to_obj(corpse, obj->loc.inside);
		}
		else
		{
			// Just destroy both the obj and the corpse
			extract_obj(corpse);
		}
		extract_obj(obj, TRUE); // Not an arti, but 'in game.'
		return TRUE;
	}
	return FALSE;
}

int gardener_block(int room, P_char ch, int cmd, char *args)
{
	int r;
	int block = 0;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || !cmd)
		return FALSE;

	r = world[room].number;
	switch (cmd)
	{
		case CMD_NORTH:
			if ((r == 13553) || (r == 13558) || (r == 13568) || (r == 13569))
				block = 1;
			break;
		case CMD_SOUTH:
			if ((r == 13553) || (r == 13556) || (r == 13564) || (r == 13569))
				block = 1;
			break;
		case CMD_EAST:
			if ((r == 13570) || (r == 13558) || (r == 13557) || (r == 13555) || (r == 13556) || (r == 13571))
				block = 1;
			break;
		case CMD_WEST:
			if ((r == 13570) || (r == 13568) || (r == 13567) || (r == 13565) || (r == 13564) || (r == 13571))
				block = 1;
			break;
		default:
			return FALSE;
	}

	if (block)
		if (ch->equipment[WEAR_WAIST] && (obj_index[ch->equipment[WEAR_WAIST]->R_num].virtual_number == 13521))
		{
			return FALSE;
		}
		else
		{
			act("A magical force prevents you from entering the garden", TRUE, ch, NULL, NULL, TO_CHAR);
			act("$n seems to walk into an invisible wall.", TRUE, ch, NULL, NULL, TO_ROOM);
			return TRUE;
		}
	return FALSE;
}
