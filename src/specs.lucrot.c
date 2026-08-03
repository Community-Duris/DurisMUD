// Lucrot's file of pain, sweat, and tears.
// 2009

#include <ctype.h>
#include <list>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <vector>
using namespace std;

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
#include "specs.prototypes.h"
#include "spells.h"

extern P_index obj_index;
extern char   *spells[];
extern int     char_is_on_plane(P_char);
extern P_room  world;

int lucrot_mindstone(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time = get_time();

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC)
	{
		if (!IS_OBJ_STAT2(obj, ITEM2_MAGIC) && (obj->timer[0] + 1800 <= curr_time))
		{
			SET_BIT(obj->extra2_flags, ITEM2_MAGIC);
			if (OBJ_WORN(obj) && (ch = obj->loc.wearing))
				act("&+cYour&n $q &+cvibrates softly for a second.&n\n", FALSE, ch, obj, obj, TO_CHAR);
		}
		return TRUE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if ((cmd == CMD_SAY) && arg)
	{
		if (isname(arg, "journey"))
		{
			curr_time = get_time();

			if (affected_by_spell(ch, TAG_PVPDELAY) || IS_FIGHTING(ch) || IS_IMMOBILE(ch))
			{
				send_to_char("&+WYour thoughts are too incohesive and disorganized.&n\r\n", ch);
				CharWait(ch, PULSE_VIOLENCE * 1);

				return TRUE;
			}
			// 30 min timer.
			if (obj->timer[0] + 1800 <= curr_time)
			{
				int to_room = real_room0(GET_HOME(ch));

				if (to_room <= 0)
				{
					to_room = real_room0(19721);
				}

				act("You say 'Journey'", FALSE, ch, 0, 0, TO_CHAR);
				act("\n&+cYour&n $q &+cpulses!&n\n", FALSE, ch, obj, obj, TO_CHAR);

				act("$n says 'Journey'", TRUE, ch, obj, NULL, TO_ROOM);
				act("\n$n's &n$q &+cpulses!&n\n", TRUE, ch, obj, NULL, TO_ROOM);

				if (IS_ROOM(ch->in_room, ROOM_NO_RECALL))
				{
					send_to_char("&+WThere is something about this area that prevents your journey home.&n\r\n", ch);
					CharWait(ch, PULSE_VIOLENCE * 1);

					return TRUE;
				}

				char_from_room(ch);
				char_to_room(ch, to_room, 0);

				act("$n materializes suddenly...", 0, ch, 0, 0, TO_ROOM);
				CharWait(ch, PULSE_VIOLENCE * 4);

				REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);
				obj->timer[0] = curr_time;
				return TRUE;
			}
		}
	}
	return FALSE;
}
