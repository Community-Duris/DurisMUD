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
#include "vnum.obj.h"
#include "vnum.room.h"
#include "weather.h"
/*
   external variables
 */

extern P_char      character_list;
extern P_desc      descriptor_list;
extern P_index     mob_index;
extern P_index     obj_index;
extern P_room      world;
extern const int   top_of_world;
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

/***********************************************************************
 * Begin: Special Procedures for Yuan-Ti zone                          *
 * Maker of Zone: Xueqin                                               *
 ***********************************************************************/

int drowcrusher(P_obj obj, P_char ch, int cmd, char *arg)
{
	//  P_char   ch2, victim, next;
	//  struct group_list *gl = NULL;
	P_char ch2;
	P_obj  obj2;
	int    i, from_room, to_room;

	if (cmd != CMD_HIT)
		return FALSE;
	if (!OBJ_WORN(obj) || !IS_ALIVE(ch))
		return FALSE;
	if (obj->loc.wearing->equipment[WIELD] != obj)
		return FALSE;
	if (ROOM_VNUM(ch->in_room) != VROOM_YUANTI_TREASUREROOM)
		return FALSE;
	if (!arg || !*arg)
		return FALSE;
	// Not looking for tracks, so skipping them
	generic_find(arg, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &ch2, &obj2);
	if (!obj2)
		return FALSE;

	/* check vnum of stone we hit, to the stone that we WANT to hit */
	if (OBJ_VNUM(obj2) != VOBJ_YUANTI_CRUSHERSTONE)
	{
		act("You playfully hit $p&n, *tink* *tink*", TRUE, ch, obj2, 0, TO_CHAR);
		return TRUE;
	}
	act("Your $q&n hums loudly as you strike the stone!", TRUE, ch, obj, NULL, TO_CHAR);
	act("$n's $q&n hums loudly as it strikes the stone!", TRUE, ch, obj, NULL, TO_ROOM);
	act("$p&n explodes, covering the room in a fine dust.", TRUE, NULL, obj2, NULL, TO_NOTVICT);
	act("The $q in your hands crumbles into small, unusable pieces.", TRUE, ch, obj, NULL, TO_CHAR);

	act("  A deep rumbling can be heard from within the temple, and rocks and other\n"
	    "debris start to fall on you. All of a sudden a sphere of force surrounds\n"
	    "you and a large booming voice can be heard, \"You have succeeded in your\n"
	    "quest, however do not expect Xueqin to save your hides again\". You are\n"
	    "then magicly transported to a different location, or perhaps a different\r\n"
	    "time, it is hard to tell, but a thick layer of dust covers the room, and a\r\n"
	    "dank smell overpowers your senses.\r\n",
	    TRUE,
	    NULL,
	    obj2,
	    NULL,
	    TO_ROOM);

	/* remove stone from room, and hammer from user */
	unequip_char(ch, WIELD);
	extract_obj(obj, TRUE);  // Not an arti, but 'in game.'
	extract_obj(obj2, TRUE); // Not an arti, but 'in game.'

	/* No.. just moving group members that were in the room with the stone.
	// Bring all users in room 80500-80545 to room 80546
	for (i = 80500; i < 80546; i++)
	{
	  for (victim = world[real_room(i)].people; victim; victim = next)
	  {
	    next = victim->next_in_room;
	    if (!IS_NPC(victim))
	    {
	      char_from_room(victim);
	      char_to_room(victim, real_room(80546), -1);
	    }
	  }
	}*/
	/* Moving everyone that was in the room to make the message make sense.
	  from_room = ch->in_room;
	  if( ch->group )
	  {
	    // Teleport the group members in the character's room
	    for( gl = ch->group; gl; gl = gl->next )
	    {
	      if( gl->ch->in_room == from_room )
	      {

	        // if they're fighting, break it up
	        if(IS_FIGHTING(gl->ch))
	          stop_fighting(gl->ch);
	        if(IS_DESTROYING(gl->ch))
	          stop_destroying(gl->ch);

	        // move the char
	        char_from_room(gl->ch);
	        char_to_room(gl->ch, real_room(VROOM_YUANTI_RUBBLEFILLED), -1);
	      }
	    }
	  }
	  else
	  {
	    // If they're fighting, break it up
	    if(IS_FIGHTING(ch))
	      stop_fighting(ch);
	    if(IS_DESTROYING(ch))
	      stop_destroying(ch);

	    // Move the char
	    char_from_room(ch);
	    char_to_room(ch, real_room(VROOM_YUANTI_RUBBLEFILLED), -1);
	  }
	*/
	from_room = ch->in_room;
	to_room   = real_room(VROOM_YUANTI_RUBBLEFILLED);
	while ((ch2 = world[from_room].people) != NULL)
	{
		char_from_room(ch2);
		char_to_room(ch2, to_room, -1);
	}
	return TRUE;
}

/* Xueq's Dragon Armor, makes PC's agro Dragon mobs, and keeps
   PC's from fleeing */
int dragonarmor(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char Dragon_Mob;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;
	if (!obj)
		return FALSE;
	if (!ch)
		return FALSE;
	if (!OBJ_WORN_BY(obj, ch))
		return FALSE;

	if (IS_FIGHTING(ch) && (cmd == CMD_REMOVE))
	{
		act("Argh! Too hard to remove $p&n when you're fighting!", TRUE, ch, obj, 0, TO_CHAR);
		return TRUE;
	}
	/*
	  if (IS_FIGHTING(ch) && (cmd == CMD_FLEE))
	  {
	    act("WHAT?! You're too brave to flee! Fight on!", FALSE, ch, 0, 0,
	        TO_CHAR);
	    act("$n starts to flee but something compels $s to stay!", FALSE, ch, 0,
	        0, TO_ROOM);
	    return TRUE;
	  }
	*/
	if (cmd == CMD_GOTNUKED)
	{
		struct proc_data *data  = (struct proc_data *)arg;
		P_char            nuker = data->victim;

		if ((GET_RACE(nuker) != RACE_DRAGON && GET_RACE(nuker) != RACE_DRAGONKIN) || number(0, 2))
		{
			return FALSE;
		}
		if (data->flags & SPLDAM_BREATH)
		{
			act("$n's $p flares brightly as it absorbs $N's breath.", FALSE, ch, obj, nuker, TO_NOTVICT);
			act("Your $p flares brightly as it absorbs $N's breath.", FALSE, ch, obj, nuker, TO_CHAR);
			act("$n's $p flares brightly as it absorbs your breath.", FALSE, ch, obj, nuker, TO_VICT);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	if (IS_FIGHTING(ch) || IS_TRUSTED(ch))
		return FALSE;

	/* attack the first dragon mob that's in the room...wheeee */
	for (Dragon_Mob = world[ch->in_room].people; Dragon_Mob; Dragon_Mob = Dragon_Mob->next_in_room)
	{
		if (((GET_RACE(Dragon_Mob) == RACE_DRAGON) || (GET_RACE(Dragon_Mob) == RACE_DRAGONKIN)) && CAN_SEE(ch, Dragon_Mob))
		{
			act("You notice $N&n, and charge to attack! Banzai!", FALSE, ch, 0, Dragon_Mob, TO_CHAR);
			act("$n gets an _angry_ look on his face when he sees $N&n and charges into battle!", FALSE, ch, 0, Dragon_Mob, TO_NOTVICT);
			set_fighting(ch, Dragon_Mob);
			return TRUE;
		}
	}
	return FALSE;
}

/* proc for a hammer named 'squelcher' */
int squelcher(P_obj obj, P_char ch, int cmd, char *arg)
{
	int    dam = cmd / 1000;
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;
	if (!ch)
		return FALSE;
	if (!OBJ_WORN_BY(obj, ch))
		return FALSE;

	if (IS_FIGHTING(ch) && (cmd == CMD_REMOVE))
	{
		act("You stop using $p&n.", TRUE, ch, obj, 0, TO_CHAR);
		act("$n stops using $p&n.", TRUE, 0, obj, ch, TO_NOTVICT);
		act("$p&n jumps back into your hands!", TRUE, ch, obj, 0, TO_CHAR);
		act("$p&n jumps back into $n's hands!", TRUE, 0, obj, ch, TO_NOTVICT);
		return TRUE;
	}
	if (IS_FIGHTING(ch) && (cmd == CMD_FLEE))
	{
		act("The $p&n in your hands beckons you to fight on!", FALSE, ch, obj, 0, TO_CHAR);
		return TRUE;
	}
	if (!dam)
		return FALSE;
	vict = (P_char)arg;
	if (!vict)
		return FALSE;
	if (obj->loc.wearing->equipment[WIELD] != obj)
		return FALSE;
	if ((IS_FIGHTING(ch) && (number(1, 100) < 6)))
	{
		act("$n's $q &+Cglows &+Wbrightly&+C, and sends out a wave of energy!", TRUE, ch, obj, 0, TO_ROOM);
		act("Your $q &+Cglows &+Wbrightly&+C, and sends out a wave of energy!", TRUE, ch, obj, 0, TO_CHAR);
		spell_silence(50, ch, NULL, 0, vict, obj);
		return TRUE;
	}
	return FALSE;
}
