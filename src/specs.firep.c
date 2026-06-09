/* Made for Charcoal Palace */
#include <ctype.h>
#include <list>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
;

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include "assocs.h"
#include "damage.h"
#include "graph.h"
#include "justice.h"
#include "reavers.h"
#include "specs.firep.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "weather.h"

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
extern const char            *crime_list[];
extern const char            *crime_rep[];
extern const char            *specdata[][MAX_SPEC];
extern struct class_names     class_names_table[];
int                           range_scan_track(P_char ch, int distance, int type_scan);
extern P_obj                  object_list;

#define KOSSUTH_HELPER_LIMIT 6

int kossuth(P_char ch, P_char pl, int cmd, char *arg)
{
	P_char i;
	P_char          minion;
	int             count = 0;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}
	if (!ch)
	{
		return FALSE;
	}
	if (cmd != 0)
	{
		return FALSE;
	}
	if (IS_ROOM(ch->in_room, ROOM_SINGLE_FILE))
	{
		REMOVE_BIT(world[ch->in_room].room_flags, ROOM_SINGLE_FILE);
		act("&+RKossuth bellows &+Wmightily&+R, and great &+Lmasses &+Rof molten rock assemble\r\n"
		    "&+Rthemselves into a &+Lwider &+Rlandmass about his &+Wawesome &+Rfigure!&n",
		    FALSE,
		    ch,
		    0,
		    0,
		    TO_ROOM);
	}
	if (IS_FIGHTING(ch))
	{
		/*
		 * attempt to "summon" a fire minion...only possible if less than KOSSUTH_HELP_LIMIT
		 * in world
		 */
		for (i = character_list; i; i = i->next)
		{
			if ((IS_NPC(i)) && (GET_VNUM(i) == 88323))
			{
				count++;
			}
		}
		if (count < KOSSUTH_HELPER_LIMIT)
		{
			if (number(1, 100) < 50)
			{
				minion = read_mobile(88323, VIRTUAL);
				if (!minion)
				{
					logit(LOG_EXIT, "assert: error in kossuth() proc");
					raise(SIGSEGV);
				}
				act("&+r$n&+r makes a quaint gesture with his hand. &n\r\n"
				    "&+rSuddenly, reality begins to rend and tear, as &+WKossuth &+rspawns a fire minion from &+Lnothingness!&n\r\n",
				    FALSE,
				    ch,
				    0,
				    minion,
				    TO_ROOM);
				char_to_room(minion, ch->in_room, 0);
				return TRUE;
			}
		}
	}

	return FALSE;
}

int fruaack_shout(P_char ch, P_char tch, int cmd, char *arg)
{
	int helpers[] = {88300, 88301, 88304, 88326, 0};
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!tch && !number(0, 4))
		return shout_and_hunt(ch, 50, "&+RFinally a challenge!  &+rFire minions!  &+RTO ARMS!", NULL, helpers, 0, 0);

	return FALSE;
}

int charcoal_guard(P_char ch, P_char victim, int cmd, char *arg)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!IS_FIGHTING(ch) && range_scan_track(ch, 5, 5) && MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		InitNewMobHunt(ch);

	return FALSE;
}

#undef KOSSUTH_HELPER_LIMIT
