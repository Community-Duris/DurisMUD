/*
 * ***************************************************************************
 *   File: objmisc.c                                       Part of Duris
 *   Usage: Miscellaneous stuff related to objects
 *   Copyright  1990, 1991 - see 'license.doc' for complete information.
 *   Copyright  1994, 1995, 1997 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "objmisc.h"
#include <string.h>
#include "damage.h"

extern P_room world; /* dyn alloc'ed array of rooms     */
// extern int rev_dir[];
extern struct zone_data *zone_table;
/*
 * getWeaponDamType
 */

int getWeaponDamType(const int weaptype)
{
	switch (weaptype)
	{
		case WEAPON_SICKLE:
		case WEAPON_2HANDSWORD:
		case WEAPON_SHORTSWORD:
		case WEAPON_LONGSWORD:
		case WEAPON_AXE:
			return WEAPONTYPE_SLASH;

		case WEAPON_LANCE:
		case WEAPON_TRIDENT:
		case WEAPON_HORN:
		case WEAPON_SPEAR:
		case WEAPON_POLEARM:
		case WEAPON_DAGGER:
			return WEAPONTYPE_PIERCE;

		case WEAPON_HAMMER:
		case WEAPON_MACE:
		case WEAPON_SPIKED_MACE:
		case WEAPON_CLUB:
		case WEAPON_SPIKED_CLUB:
		case WEAPON_STAFF:
		case WEAPON_NUMCHUCKS:
			return WEAPONTYPE_BLUDGEON;

		case WEAPON_FLAIL:
		case WEAPON_WHIP:
			return WEAPONTYPE_WHIP;
	}

	return WEAPONTYPE_UNDEFINED;
}

int get_weapon_msg(P_obj weapon)
{
	switch (weapon->value[0])
	{
		case WEAPON_AXE:
		case WEAPON_SHORTSWORD:
		case WEAPON_2HANDSWORD:
		case WEAPON_SICKLE:
		case WEAPON_POLEARM:
		case WEAPON_LONGSWORD:
			return MSG_SLASH;
		case WEAPON_DAGGER:
		case WEAPON_SPEAR:
		case WEAPON_TRIDENT:
		case WEAPON_HORN:
			return MSG_PIERCE;
		case WEAPON_HAMMER:
		case WEAPON_FLAIL:
		case WEAPON_CLUB:
		case WEAPON_SPIKED_CLUB:
		case WEAPON_LANCE:
			return MSG_CRUSH;
		case WEAPON_MACE:
		case WEAPON_SPIKED_MACE:
		case WEAPON_STAFF:
		case WEAPON_NUMCHUCKS:
			return MSG_BLUDGEON;
		case WEAPON_WHIP:
			return MSG_WHIP;
		default:
			return MSG_HIT;
	}
}

void event_random_exit(P_char ch, P_char victim, P_obj obj, void *data)
{
	char buf[512];
	char exit_name[32];
	int  exit_dir, s_room, d_room;

	if (!obj)
		return;

	if (obj->value[0] > number(0, 99) && OBJ_ROOM(obj) && sscanf(obj->name, "%s exit_%s ", buf, exit_name) && (exit_dir = dir_from_keyword(exit_name)) != -1 &&
	    (d_room = real_room(obj->value[1])) != -1)
	{
		s_room = obj->loc.room;
		if (!world[s_room].dir_option[exit_dir])
		{
			CREATE(world[s_room].dir_option[exit_dir], room_direction_data, 1, MEM_TAG_DIRDATA);
			memset(world[s_room].dir_option[exit_dir], 0, sizeof(struct room_direction_data));
		}
		else
		{ // if an exit exists, we close off the zone the exit leads to
			// if we are using this as a random exit generator instead leading
			// to the same zone, it's ok, because we remove the closed flag of
			// the destination zone below.  Example result: Desolate is closed,
			// and Desolate Under Fire (default closed) becomes opened.  This
			// will help prevent people shifting into the zone when they shouldn't.
			if (!(zone_table[world[(world[s_room].dir_option[exit_dir])->to_room].zone].flags & ZONE_CLOSED))
			{ // close it...
				SET_BIT(zone_table[world[(world[s_room].dir_option[exit_dir])->to_room].zone].flags, ZONE_CLOSED);
			}
		}
		if (!world[d_room].dir_option[rev_dir[exit_dir]])
		{
			CREATE(world[d_room].dir_option[rev_dir[exit_dir]], room_direction_data, 1, MEM_TAG_DIRDATA);
			memset(world[d_room].dir_option[rev_dir[exit_dir]], 0, sizeof(struct room_direction_data));
		}
		world[s_room].dir_option[exit_dir]->to_room          = real_room(obj->value[1]);
		world[d_room].dir_option[rev_dir[exit_dir]]->to_room = s_room;
		if (zone_table[world[d_room].zone].flags & ZONE_CLOSED)
			REMOVE_BIT(zone_table[world[d_room].zone].flags, ZONE_CLOSED);
	}

	extract_obj(obj);
}

int obj_zone_id(P_obj o)
{
	P_obj tobj = o;

	while (tobj && OBJ_INSIDE(tobj))
		tobj = tobj->loc.inside;

	int zone_id = -1;

	if (!tobj)
	{
		return -1;
	}
	else if (OBJ_ROOM(tobj))
	{
		zone_id = world[tobj->loc.room].zone;
	}
	else if (OBJ_CARRIED(tobj) && tobj->loc.carrying->in_room != NOWHERE)
	{
		zone_id = world[tobj->loc.carrying->in_room].zone;
	}
	else if (OBJ_WORN(tobj) && tobj->loc.wearing->in_room != NOWHERE)
	{
		zone_id = world[tobj->loc.wearing->in_room].zone;
	}

	return zone_id;
}

int obj_room_id(P_obj o)
{
	P_obj tobj = o;

	while (tobj && OBJ_INSIDE(tobj))
		tobj = tobj->loc.inside;

	int room_id = -1;

	if (!tobj)
	{
		return -1;
	}
	else if (OBJ_ROOM(tobj))
	{
		room_id = tobj->loc.room;
	}
	else if (OBJ_CARRIED(tobj) && tobj->loc.carrying->in_room != NOWHERE)
	{
		room_id = tobj->loc.carrying->in_room;
	}
	else if (OBJ_WORN(tobj) && tobj->loc.wearing->in_room != NOWHERE)
	{
		room_id = tobj->loc.wearing->in_room;
	}

	return room_id;
}
