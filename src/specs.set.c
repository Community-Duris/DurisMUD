/*****************************************************************************
 *  File: specs.zalrix.c   (Part of Duris)                                   *
 *  Usage: special procedures by Zalrix                                      *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                               *
 *****************************************************************************/

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include "damage.h"
#include "justice.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "weather.h"

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
extern int                    innate_abilities[];
extern int                    planes_room_num[];
extern int                    top_of_zone_table;
extern struct command_info    cmd_info[MAX_CMD_LIST];
extern struct dex_app_type    dex_app[52];
extern struct time_info_data  time_info;
extern struct zone_data      *zone;
extern struct zone_data      *zone_table;

const int   set_master_vnum[] = {22063, 22237, 22621, 45530, 45531, 82545, 75857, 0

};
const char *set_master_text[] = {
	"",
	"",
	"\n&+GMaster set granting:\n&+W*Stronger armor\n",
	"\n&+GMaster set granting:\n&+W*Stronger armor\n&+W*Defense vs spells\n",
	"\n&+GMaster set granting:\n&+W*Stronger armor\n&+W*Defense vs spells\n&+W*Hitroll\n",
	"\n&+GMaster set granting:\n&+W*Stronger armor\n&+W*Defense vs spells\n&+W*Hitroll\n&+W*Detect Invisbility\n",
	"\n&+GMaster set granting:\n&+W*Stronger armor\n&+W*Defense vs spells\n&+W*Hitroll\n&+W*Detect Invisbility\n&+W*Haste",
	" ",
	" ",
};

int master_set(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct affected_type af;
	int                  numb_items     = 0;
	int                  old_numb_items = 0;
	int                  i              = 0;
	int                  j              = 0;
	int                  k              = 0;
	P_obj                t_obj;
	int                  wear_order[] = {41, 24, 40, 6, 19, 21, 22, 20, 39, 3, 4, 5, 35, 37, 12, 23, 13, 10, 31, 11, 14, 15, 33, 34, 9, 32, 1, 2, 16, 17, 25, 26, 7, 36, 8, 38, -1};

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj || !OBJ_WORN(obj) || !(ch = obj->loc.wearing) || cmd != CMD_PERIODIC)
	{
		return FALSE;
	}
	if (!IS_ALIVE(ch) || IS_NPC(ch))
	{
		return FALSE;
	}

	// For each slot,
	for (j = 0; wear_order[j] != -1; j++)
	{
		// If there's eq there..
		if (ch->equipment[wear_order[j]])
		{
			t_obj = ch->equipment[wear_order[j]];
			k     = 0;
			// for each vnum in masters set..
			while (set_master_vnum[k] > 0)
			{
				// If the vnum matches, increase number of master items.
				if (obj_index[t_obj->R_num].virtual_number == set_master_vnum[k])
				{
					numb_items++;
					break;
				}
				k++;
			}
		}
	}

	// Remove all the old affects.
	affect_from_char(ch, TAG_SET_MASTER);
	bzero(&af, sizeof(af));
	af.type = TAG_SET_MASTER;

	if (numb_items > 6)
		numb_items = 6;
	if (numb_items > 1)
	{
		af.modifier = -20;
		af.location = APPLY_AC;
		affect_to_char(ch, &af);
	}
	if (numb_items > 2)
	{
		af.location = APPLY_SAVING_SPELL;
		af.modifier = -3;
		affect_to_char(ch, &af);
	}
	if (numb_items > 3)
	{
		af.modifier = 5;
		af.location = APPLY_HITROLL;
		affect_to_char(ch, &af);
	}
	if (numb_items > 4)
	{
		af.modifier  = 0;
		af.location  = 0;
		af.bitvector = AFF_DETECT_INVISIBLE;
		affect_to_char(ch, &af);
	}
	if (numb_items > 5)
	{
		af.bitvector = AFF_HASTE;
		affect_to_char(ch, &af);
	}

	if (numb_items < ch->only.pc->master_set)
	{
		act("Your $q glow of power decrese.", FALSE, ch, obj, obj, TO_CHAR);
		act("$n's $q glow of power decrese.", TRUE, ch, obj, NULL, TO_ROOM);
	}

	if (numb_items > ch->only.pc->master_set && numb_items > 1)
	{
		act("Your $q glow of power increase.", FALSE, ch, obj, obj, TO_CHAR);
		act("$n's $q glow of power increase.", TRUE, ch, obj, NULL, TO_ROOM);
	}

	ch->only.pc->master_set = numb_items;
	return FALSE;
}
