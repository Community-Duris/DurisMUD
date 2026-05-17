/****************************************************************************
 *
 *  File: hardcore.c                                           Part of Duris
 *  Usage: PERIODcore chars related materia.
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark 			Date: 2002-04-18
 * ***************************************************************************
 */

#define TROPHY

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "mm.h"
#include "sound.h"
#include "spells.h"
#include "weather.h"

/*
 * external variables
 */

extern P_char      character_list;
extern P_desc      descriptor_list;
extern P_event     event_type_list[];
extern P_index     mob_index;
extern P_index     obj_index;
extern P_obj       object_list;
extern P_room      world;
extern char        debug_mode;
extern const char *race_types[];

// extern const int material_absorbtion[][];
extern const struct stat_data stat_factor[];
extern float                  fake_sqrt_table[];
extern int                    pulse;
extern int                    arena_hometown_location[];
extern struct arena_data      arena;
extern struct agi_app_type    agi_app[];
extern struct dex_app_type    dex_app[];
extern struct message_list    fight_messages[];
extern struct str_app_type    str_app[];
extern struct time_info_data  time_info;
extern struct zone_data      *zone_table;

extern struct minor_create_struct minor_create_name_list[];

int is_Raidable(P_char ch, char *argument, int cmd)
{
	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}
	else if (IS_TRUSTED(ch) || IS_NPC(ch))
	{
		return TRUE;
	}
	else if (hasRequiredSlots(ch))
	{
		return TRUE;
	}

	return FALSE;
}

void do_raid(P_char ch, char *argument, int cmd)
{
	P_desc i;
	P_char tch;
	int    found = 0;
	char   arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char   Gbuf[MAX_INPUT_LENGTH];
	if (!IS_TRUSTED(ch))
	{
		if (!ch)
			return;

		if (IS_NPC(ch))
		{
			send_to_char("Why you a npc care about raidable or not?!", ch);
			return;
		}
		if (hasRequiredSlots(ch))
			send_to_char("&+GRaidable you are, slay enemies you can!\n", ch);
		else
			send_to_char("&+RYou do not have enough equipment to be raidable.\n", ch);
	}

	if (IS_TRUSTED(ch))
	{
		argument_interpreter(argument, arg1, arg2);

		if (!str_cmp(arg1, ""))
		{
			send_to_char("raid [room|zone|good|evil|all|(character name)] [bad] <-Show only bad players option.\n", ch);
			return;
		}

		for (i = descriptor_list; i; i = i->next)
		{
			if (i->character && IS_PC(i->character) && (i->connected == CON_PLAYING))
			{
				tch = NULL;
				if ((!str_cmp(arg1, "room") && (i->character->in_room == ch->in_room)) || (!str_cmp(arg1, "zone") && (world[i->character->in_room].zone == world[ch->in_room].zone)) ||
				    (!str_cmp(arg1, "good") && GOOD_RACE(i->character)) || (!str_cmp(arg1, "evil") && EVIL_RACE(i->character)) || (!str_cmp(arg1, "all")))
				{
					tch = i->character;
				}
				else if (!str_cmp(arg1, i->character->player.name))
				{
					tch   = i->character;
					found = TRUE;
				}

				if (tch && IS_PC(tch) && !IS_TRUSTED(tch))
				{
					if (hasRequiredSlots(tch))
					{
						if (!str_cmp(arg2, "bad"))
							continue;
						snprintf(Gbuf, MAX_INPUT_LENGTH, "%s is &+Wgood&n.\n", GET_NAME(tch));
						send_to_char(Gbuf, ch);
					}
					else
					{
						snprintf(Gbuf, MAX_INPUT_LENGTH, "&+R%s&n is &+Rnot raidable!\n", GET_NAME(tch));
						send_to_char(Gbuf, ch);
					}
				}
			}
		}
		if (!found && !(!str_cmp(arg1, "room") || !str_cmp(arg1, "zone") || !str_cmp(arg1, "evil") || !str_cmp(arg1, "good") || !str_cmp(arg1, "all")))
		{
			send_to_char("You don't see anybody.\n", ch);
		}
		return;
	}
}

bool hasRequiredSlots(P_char ch)
{
	if (!IS_ALIVE(ch))
		return FALSE;

	int   wear_order[] = {41, 24, 40, 6, 19, 21, 22, 20, 39, 3, 4, 5, 35, 37, 12, 27, 23, 13, 10, 31, 11, 14, 15, 33, 34, 9, 32, 1, 2, 16, 17, 25, 26, 18, 7, 36, 8, 38, -1};
	int   found = 0, mod = 0, lvl;
	bool  minor_created;
	P_obj t_obj;

	for (int j = 0; wear_order[j] != -1; j++)
	{
		if ((t_obj = ch->equipment[wear_order[j]]) != NULL)
		{
			if (IS_SET(t_obj->extra_flags, ITEM_TRANSIENT))
			{
				continue;
			}

			minor_created = FALSE;
			for (int i = 0; minor_create_name_list[i].keyword[0]; i++)
			{
				if (obj_index[t_obj->R_num].virtual_number == minor_create_name_list[i].obj_number)
				{
					minor_created = TRUE;
					break;
				}
			}
			if (!minor_created)
			{
				found++;
			}
		}
	}

	lvl = GET_LEVEL(ch);
	//  Added some slightly more intelligent
	if (lvl >= 41)      //  rules to the equation figuring on how much
		mod = 0;        //  equipment a character must be wearing to
	else if (lvl >= 36) //  be raidable based upon level.  It is somewhat
		mod = 4;        //  foolish to believe a level 30 character is
	else if (lvl >= 31) //  going to have more than 1-1.5 sets of eq
		mod = 6;        //  on hand for CR's or such - Jexni  6/1/09
	else
		mod = 8;

	if (found + mod >= get_property("raid.min.slots", 12))
	{
		return TRUE;
	}

	return FALSE;
}
