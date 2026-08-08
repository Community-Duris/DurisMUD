/*
 ***************************************************************************
 *  File: specs.vecna.c                                   Part of Duris    *
 *  Usage: special procedures for Vecna zone                               *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.    *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 ***************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "assocs.h"
#include "damage.h"
#include "disguise.h"
#include "graph.h"
#include "grapple.h"
#include "justice.h"
#include "map.h"
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
extern const char *race_types[];
// extern const char rev_dir[];
extern const struct stat_data   stat_factor[];
extern const struct class_names class_names_table[];
extern int                      innate_abilities[];
extern int                      planes_room_num[];
extern int                      top_of_zone_table;
extern struct command_info      cmd_info[MAX_CMD_LIST];
extern struct dex_app_type      dex_app[52];
extern struct time_info_data    time_info;
extern struct zone_data        *zone;
extern struct zone_data        *zone_table;
extern bool                     has_skin_spell(P_char);

int vecna_bubble_room(int room, P_char ch, int cmd, char *arg)
{
	P_char target, next_target;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if ((cmd == CMD_PERIODIC) && (50 <= number(0, 100)))
	{
		send_to_room("&+LThe bubble in space floats aimlessly through the void.\r\n", real_room(room));
		return FALSE;
	}

	if ((cmd == CMD_PERIODIC) && (50 <= number(0, 100)))
	{
		send_to_room("&+LSuddenly your bubble &+WPOPS&+L!!!\r\n\r\n", real_room(room));
		for (target = world[real_room(room)].people; target; target = next_target)
		{
			next_target = target->next_in_room;
			if (target)
			{
				char_from_room(target);
				char_to_room(target, real_room(130074), 0);
			}
		}
		return FALSE;
	}
	return FALSE;
}

int vecna_black_mass(P_char ch, P_char tch, int cmd, char *arg)
{
	P_char target, next_target;
	char   buf[500];

	if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC)
		return FALSE;

	if (!ch)
		return FALSE;

	SET_BIT(ch->specials.act, ACT_SPEC_DIE);

	if (cmd == CMD_DEATH)
	{
		send_to_room("&+LAs the black mass dis&+wsipates, you slowly be&+Wgin to see again...\r\n\r\n", ch->in_room);
		for (target = world[ch->in_room].people; target; target = next_target)
		{
			next_target = target->next_in_room;
			if (target)
			{
				char_from_room(target);
				char_to_room(target, real_room(130076), 0);
			}
		}
		return FALSE;
	}
	return FALSE;
}

int vecnas_fight_proc(P_char ch, P_char tch, int cmd, char *arg)
{
	P_char                 victim;
	char                   buf[500];
	int                    helpers[] = {130037, 130039, 0};
	struct damage_messages messages  = {"",
	                                    "&+LThe arch-lich &+rVecna &+Lbegins to chant and hum.\r\n&+LSuddenly his right hand glows &+Cbri&+Bght bl&+Cue&+L and he lurches forward to grab "
	                                     "you!\r\n&+WIce quickly condenses into you causing severe pain!&n",
	                                    "&+LThe arch-lich &+rVecna &+Lbegins to chant and hum.\r\n&+LSuddenly his right hand glows &+Cbri&+Bght bl&+Cue&+L and he lurches forward to grab $N!\r\n&+WIce "
	                                     "quickly condenses into $N causing severe pain!&n",
	                                    "",
	                                    "&+LThe arch-lich &+rVecna &+Lbegins to chant and hum.\r\n&+LSuddenly his right hand glows &+Cbri&+Bght bl&+Cue&+L and he lurches forward to grab "
	                                     "you!\r\n&+WIce quickly condenses into you causing your heart to stop, and con&+wciousnes&+Ls to fade...&n",
	                                    "&+LThe arch-lich &+rVecna &+Lbegins to chant and hum.\r\n&+LSuddenly his right hand glows &+Cbri&+Bght bl&+Cue&+L and he lurches forward to grab $N!\r\n&+WIce "
	                                     "quickly condenses into $N causing $S heart to stop, and con&+wciousnes&+Ls to fade...&n"};

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch)
		return FALSE;

	// So woodlands staff doesn't own vecna on idle time because she stands indoors.
	strcpy(buf, "woodlands");
	if (!IS_FIGHTING(ch) && ch->equipment[WIELD] && (obj_index[ch->equipment[WIELD]->R_num].virtual_number == 130027))
	{
		do_remove(ch, buf, 0);
		return FALSE;
	}

	if (cmd == CMD_PERIODIC && IS_FIGHTING(ch))
	{
		if ((GET_HIT(ch) <= (GET_MAX_HIT(ch) * 0.30)))
		{
			return shout_and_hunt(ch, 2, "&+RFoolish mortals, tremble as my &+Wskeletal &+Lknight&+R descends upon you!&n", NULL, helpers, 0, 0);
		}
	}
	if (cmd == CMD_GOTHIT && IS_FIGHTING(ch))
	{
		if (!number(0, 50))
		{
			victim = GET_OPPONENT(ch);
			if (!victim || IS_TRUSTED(victim))
			{
				return FALSE;
			}

			if (GET_HIT(victim) <= (GET_MAX_HIT(victim) * 0.5))
			{
				act("&+LThe arch-lich &+rVecna &+Lbegins to chant and hum.\r\n&+LSuddenly his right hand glows &+Cbri&+Bght bl&+Cue&+L and he lurches forward to grab $N!\r\n&+W$N suddenly looks well "
				    "preserved!&n\r\n&+LA passageway in the wall reveals itself and hands grab the preserved body of $N and drag it away!&n",
				    FALSE,
				    ch,
				    0,
				    victim,
				    TO_NOTVICT);
				act("&+LThe arch-lich &+rVecna &+Lbegins to chant and hum.\r\n&+LSuddenly his right hand glows &+Cbri&+Bght bl&+Cue&+L and he lurches forward to grab you!\r\n&+WYou suddenly feel "
				    "preserved!&n",
				    FALSE,
				    ch,
				    0,
				    victim,
				    TO_VICT);
				char_from_room(victim);
				char_to_room(victim, real_room(130069), 0);
			}
			else
			{
				spell_damage(ch, victim, 400, SPLDAM_COLD, 0, &messages);
			}
		}
	}
	return FALSE;
}

int chressan_shout(P_char ch, P_char tch, int cmd, char *arg)
{
	int helpers[] = {130017, 0};
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}
	if (GET_HIT(ch) <= (GET_MAX_HIT(ch) * 0.50) && !number(0, 8))
	{
		return shout_and_hunt(ch, 2, "&+rGuardians, step forth and destroy %s!&n", NULL, helpers, 0, 0);
	}
	return FALSE;
}

int vecna_deathaltar(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char hecate;

	if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC)
		return FALSE;

	if (cmd == CMD_TOUCH || arg)
	{
		if (isname(arg, "hecate") || isname(arg, "altar"))
		{
			if ((hecate = world[real_room(130044)].people) && hecate->only.npc->R_num == real_mobile(130015))
			{
				char_from_room(hecate);
				char_to_room(hecate, real_room(130043), -2);
				send_to_room("&+LThe avatar of Death suddenly appears with his hand stretched out ready to reap.\r\n", real_room(130043));
				return TRUE;
			}
		}
	}
	return FALSE;
}

int vecna_deathportal(P_obj obj, P_char ch, int cmd, char *arg)
{
	int rooms[] = {130072, 130073, 130075};

	if (cmd == CMD_SET_PERIODIC)
	{
		obj->value[5] = 0;
		return TRUE;
	}

	if (!obj || !ch)
		return FALSE;

	if (cmd == CMD_ENTER && arg)
	{
		if (isname(arg, "portal") || isname(arg, "death"))
		{
			/* round-robin portals
			if ((obj->value[5] >= 0) && (obj->value[5] < 3))
			{
				obj->value[0] = rooms[obj->value[5]];
				obj->value[5]++;
			}
			else
			{
				debug("Error with vecna_deathportal() using an invalid index number for the room[] array.  Resetting portal.");
				obj->value[5] = 0;
				return TRUE;
			}
			if (obj->value[5] >= 3 || obj->value[5] <= 0)
			{
				obj->value[5] = 0;
			}
			*/
			obj->value[0] = rooms[number(0, 2)];
			return FALSE;
		}
	}
	return FALSE;
}

int vecna_torturerroom(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_obj corpse, next_corpse;
	char  buf[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!world[obj->loc.room].dir_option[DIR_NORTH] || !world[obj->loc.room].dir_option[DIR_NORTH]->to_room)
		return FALSE;

	for (corpse = world[obj->loc.room].contents; corpse; corpse = next_corpse)
	{
		next_corpse = corpse->next_content;

		if (corpse->type == ITEM_CORPSE)
		{
			snprintf(buf, MAX_STRING_LENGTH, "&+WGhostly hands &+Lrise from the ground and drag &n%s &+Linto the frigid earth.&n\r\n", corpse->short_description);
			send_to_room(buf, obj->loc.room);
			obj_from_room(corpse);
			obj_to_room(corpse, world[obj->loc.room].dir_option[DIR_NORTH]->to_room);
			snprintf(buf, MAX_STRING_LENGTH, "&+WGhostly hands &+Lappear overhead and pull &n%s &+Linto the room.&n\r\n", corpse->short_description);
			send_to_room(buf, corpse->loc.room);
			return FALSE;
		}
	}
	return FALSE;
}

int vecna_ghosthands(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_obj corpse, next_corpse;
	char  buf[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	// If no down exit.
	if (!world[obj->loc.room].dir_option[DIR_DOWN] || !world[obj->loc.room].dir_option[DIR_DOWN]->to_room)
	{
		return FALSE;
	}

	for (corpse = world[obj->loc.room].contents; corpse; corpse = next_corpse)
	{
		next_corpse = corpse->next_content;

		if (corpse->type == ITEM_CORPSE)
		{
			snprintf(buf, MAX_STRING_LENGTH, "&+WGhostly hands &+Lrise from the ground and drag &n%s &+Linto the frigid earth.&n\r\n", corpse->short_description);
			send_to_room(buf, obj->loc.room);
			obj_from_room(corpse);
			obj_to_room(corpse, world[obj->loc.room].dir_option[DIR_DOWN]->to_room);
			snprintf(buf, MAX_STRING_LENGTH, "&+WGhostly hands &+Lappear overhead and pull &n%s &+Linto the room.&n\r\n", corpse->short_description);
			send_to_room(buf, corpse->loc.room);
			return FALSE;
		}
	}
	return FALSE;
}

int vecna_gorge(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char target, next_target;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC && !number(0, 10))
	{
		for (target = world[obj->loc.room].people; target; target = next_target)
		{
			next_target = target->next_in_room;

			if (target && IS_NPC(target))
			{
				char_from_room(target);
				char_to_room(target, real_room(number(130003, 130027)), -2);
			}
		}
		return FALSE;
	}
	return FALSE;
}

int vecna_stonemist(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char                target;
	struct affected_type *af;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!OBJ_ROOM(obj) || number(0, 5) || cmd != CMD_PERIODIC || !IS_ALIVE(ch))
	{
		return FALSE;
	}

	if (number(1, 100) > 50)
	{
		for (target = world[obj->loc.room].people; target; target = target->next_in_room)
		{
			if (IS_TRUSTED(target) || IS_NPC(target))
			{
				continue;
			}

			if (affected_by_spell(target, SPELL_STONE_SKIN))
			{
				send_to_char("&+WA gh&+wostly mi&+Lt flows aro&+wund your &+Wlimbs, a&+ws it dis&+Lsipates you f&+weel more vu&+Wlnerable.&n\r\n", target);
				af = get_spell_from_char(target, SPELL_STONE_SKIN);
				if (af)
				{
					wear_off_message(target, af);
					affect_remove(target, af);
				}
				else
				{
					send_to_char("&+WA gh&+wostly mis&+Lt flows int&+wo the are&+Wa and qui&+wckly dis&+Lsipates...&n\r\n", target);
				}
			}
		}
	}
	else
	{
		for (target = world[obj->loc.room].people; target; target = target->next_in_room)
		{
			if (affected_by_spell(target, SPELL_VITALITY))
			{
				send_to_char("&+WA gh&+wostly mi&+Lt flows aro&+wund your &+Wlimbs, a&+ws it dis&+Lsipates you f&+weel your sp&+Wirit weaken.&n\r\n", target);
				af = get_spell_from_char(target, SPELL_VITALITY);
				if (af)
				{
					wear_off_message(target, af);
					affect_remove(target, af);
				}
			}
			else
			{
				send_to_char("&+WA gh&+wostly mis&+Lt flows int&+wo the are&+Wa and qui&+wckly dis&+Lsipates...&n\r\n", target);
			}
		}
	}
	return FALSE;
}

int vecna_mob_rebirth(P_char ch, P_char tch, int cmd, char *arg)
{
	int    helpers[] = {77713};
	P_char mob;

	if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC || !ch)
		return FALSE;

	SET_BIT(ch->specials.act, ACT_SPEC_DIE);

	if (cmd == CMD_DEATH)
	{
		mob = read_mobile(mob_index[GET_RNUM(ch)].virtual_number, VIRTUAL);
		if (!mob)
			return FALSE;
		char_to_room(mob, real_room(130028), -2);
	}
	return FALSE;
}

int vecna_pestilence(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch))
	{
		return FALSE;
	}

	/* Making this procable in offhand since the proc situation can be rare.
	  if (!OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_POS(obj, WIELD2))
	  {
	    return FALSE;
	  }
	*/

	if (number(0, 100) <= 30 && GET_ALIGNMENT(ch) < -750)
	{
		victim = GET_OPPONENT(ch);
		if (!IS_ALIVE(victim))
		{
			return FALSE;
		}

		if (GET_ALIGNMENT(victim) > 250)
		{
			act("&+LYour &+Runholy&+L sword&+y pestilence &+Lcrackles with &+revil magic &+Las it attacks $N&n", FALSE, ch, NULL, victim, TO_CHAR);
			act("&+L$n&+L's &+runholy &+Lsword &+ypestilence &+Lcrackles with &+revil magic &+Las it attacks you!&n", FALSE, ch, NULL, victim, TO_VICT);
			act("&+L$n&+L's &+runholy &+Lsword &+ypestilence &+Lcrackles with &+revil magic &+Las it attacks $N!&n", FALSE, ch, NULL, victim, TO_NOTVICT);
			spell_dispel_good(51, ch, 0, SPELL_TYPE_SPELL, victim, 0);
			if (IS_ALIVE(victim) && IS_ALIVE(ch))
			{
				spell_dispel_magic(51, ch, 0, SPELL_TYPE_SPELL, victim, 0);
			}
			if (IS_ALIVE(victim) && IS_ALIVE(ch))
			{
				spell_disease(80, ch, 0, SPELL_TYPE_SPELL, victim, 0);
			}
			return TRUE;
		}
	}
	return FALSE;
}

int vecna_minifist(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	// 5% chance to proc.
	if (number(1, 100) <= 5)
	{
		victim = GET_OPPONENT(ch);
		if (!IS_ALIVE(victim))
		{
			return FALSE;
		}
		if (IS_OGRE(victim) || IS_CENTAUR(victim) || IS_SGIANT(victim) || IS_MINOTAUR(victim))
		{
			return FALSE;
		}

		act("&+LYour &+Gtwisted longsword &+Lshrieks in &+rrage &+Lat the sight of $N!&n", FALSE, ch, NULL, victim, TO_CHAR);
		act("&n$n's &+Gtwisted longsword &+Lshrieks in &+rrage &+Lat the sight of you!&n", FALSE, ch, NULL, victim, TO_VICT);
		act("$n's &+Gtwisted longsword &+Lshrieks in &+rrage &+Lat the sight of $N!&n", FALSE, ch, NULL, victim, TO_NOTVICT);
		spell_bigbys_clenched_fist(41, ch, 0, SPELL_TYPE_SPELL, victim, 0);
		return TRUE;
	}
	return FALSE;
}

int vecna_dispel(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	if (number(0, 100) <= 5)
	{
		victim = GET_OPPONENT(ch);
		act("&+LYour &+rrazor-edged dagger &+Lglows with motes of &+rcrimson &+Llight as it attacks $N's &+c m&+Cagi&+cc!", FALSE, ch, NULL, victim, TO_CHAR);
		act("$n's &+rrazor-edged dagger &+Lglows with motes of &+rcrimson &+Llight as it attacks your &+cm&+Cagi&+cc!", FALSE, ch, NULL, victim, TO_VICT);
		act("$n's &+rrazor-edged dagger &+Lglows with motes of &+rcrimson &+Llight as it attacks $N's &+cm&+Cagi&+cc!", FALSE, ch, NULL, victim, TO_NOTVICT);
		spell_dispel_magic(51, ch, 0, SPELL_TYPE_SPELL, victim, 0);
		return TRUE;
	}
	return FALSE;
}

int vecna_boneaxe(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	if (number(0, 100) <= 6)
	{
		victim = GET_OPPONENT(ch);
		act("&+LYour large &+Wbone &+Laxe shrieks in &+rrage &+Lat the sight of $N!", FALSE, ch, NULL, victim, TO_CHAR);
		act("&n's &+Llarge &+Wbone &+Laxe shrieks in &+rrage &+Lat the sight of you!", FALSE, ch, NULL, victim, TO_VICT);
		act("$n's &+Llarge &+Wbone &+Laxe shrieks in &+rrage &+Lat the sight of $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
		spell_arieks_shattering_iceball(51, ch, 0, SPELL_TYPE_SPELL, victim, 0);
		return TRUE;
	}
	return FALSE;
}

int vecna_staffoaken(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;
	P_obj  obj_lose, obj_next;
	int    dam, pos, curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj)
	{
		return FALSE;
	}

	if (cmd == CMD_PERIODIC)
	{
		if (!number(0, 2))
		{
			hummer(obj);
		}
		if (OBJ_WORN_POS(obj, WIELD))
		{
			ch = obj->loc.wearing;
		}
		else
		{
			return FALSE;
		}

		// The periodic stuff...
		// 50% of the time, when hurt... heal...
		if ((number(0, 100) <= 50) && (GET_HIT(ch) < GET_MAX_HIT(ch)) && (world[ch->in_room].sector_type != SECT_INSIDE) && !IS_ROOM(ch->in_room, ROOM_INDOORS))
		{
			act("&+gYou raise $p&+g overhead and it covers you with its &+Ghealing magic.&n", TRUE, ch, obj, NULL, TO_CHAR);
			act("&+GA warm feeling fills your body.&n", TRUE, ch, NULL, NULL, TO_CHAR);
			act("&+g$n&+g raises $p&+g overhead as it covers $m with its &+Ghealing magic.&n", TRUE, ch, obj, NULL, TO_ROOM);
			act("&+G$n&+G looks much better.&n", TRUE, ch, NULL, NULL, TO_ROOM);
			vamp(ch, number(2, 12) + 50, GET_MAX_HIT(ch));
			return FALSE;
		}

		// Or remaining 50% of time, 70% chance to check wanting to be outside if inside.
		if (!IS_TRUSTED(ch) && number(0, 100) <= 70 && world[ch->in_room].sector_type == SECT_INSIDE || IS_ROOM(ch->in_room, ROOM_INDOORS))
		{
			dam = dice(25, 5) + 75;
			if (number(0, 100) <= 50)
			{
				act("&+gYou sense $p&+g's urgent longing to be outside.&n", TRUE, ch, obj, NULL, TO_CHAR);
				return FALSE;
			}

			act("&+gAs $p&+g's aura&+L dims&+g considerably, it imparts you with a longing to be back outdoors.&n", TRUE, ch, obj, NULL, TO_CHAR);
			act("&+gYou suddenly feel weaker as $p&+g drains your life force.&n", TRUE, ch, obj, NULL, TO_CHAR);
			act("$p&+g's magical aura dims considerably while &+yindoors.&n", TRUE, ch, obj, NULL, TO_ROOM);
			act("&+g$n&+g's eyes dull as $p&+g feeds on $s &+Cl&+cifeforc&+Ce.&n", TRUE, ch, obj, NULL, TO_ROOM);
			act("&+gThe &+Gmagical aura&+g around $p&+G flares&+g brightly once again.&n", TRUE, ch, obj, NULL, TO_ROOM);

			if ((GET_HIT(ch) + 11) >= dam)
			{
				damage(ch, ch, dam, TYPE_UNDEFINED);
				return FALSE;
			}
			else
			{
				act("&+gWhile trying to sustain itself $p&+g begins absorbing your mortal coil...&n", TRUE, ch, obj, NULL, TO_CHAR);
				act("&+g$n&+g's body becomes less substantial as $p&+g begins to absorb $m...&n", TRUE, ch, obj, NULL, TO_ROOM);
				unequip_all(ch);
				for (obj_lose = ch->carrying; obj_lose; obj_lose = obj_next)
				{
					obj_next = obj_lose->next_content;
					obj_from_char(obj_lose);
					act("$p falls to the ground.&n", TRUE, ch, obj_lose, NULL, TO_CHAR);
					act("$p falls to the ground.&n", TRUE, ch, obj_lose, NULL, TO_ROOM);
					obj_to_room(obj_lose, ch->in_room);
				}
				act("&+G$n&+G's body finally f&+gades completely aw&+Lay, as $e is completely absorbed.&n", TRUE, ch, NULL, NULL, TO_ROOM);
				char_from_room(ch);
				// Die in corpse storage room?
				char_to_room(ch, real_room(40), -2);
				act("&+GAs the last of your m&+gortal coil van&+Lishes your spirit is pulled to the underworld.&n\r\n", TRUE, ch, NULL, NULL, TO_CHAR);
				die(ch, ch);
				return FALSE;
			}
		}
	} // End periodics

	// Say procs
	if (cmd == CMD_SAY && arg && OBJ_WORN_POS(obj, WIELD) && ch == obj->loc.wearing)
	{
		if (isname(arg, "barkskin"))
		{
			act("$n says 'barkskin' to $p.", FALSE, ch, obj, 0, TO_ROOM);
			act("You say 'barkskin'", FALSE, ch, obj, 0, TO_CHAR);
			act("&+yBark grows from $q on to everything in sight.&n", FALSE, ch, obj, 0, TO_ROOM);
			act("&+yBark grows from $q on to everything in sight.&n", FALSE, ch, obj, 0, TO_CHAR);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_BARKSKIN, GET_LEVEL(ch), 0);
			}
			else
			{
				spell_barkskin(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "bless"))
		{
			act("$n says 'bless' to $p.", FALSE, ch, obj, 0, TO_ROOM);
			act("You say 'bless'", FALSE, ch, obj, 0, TO_CHAR);
			act("$p hums briefly...&n", FALSE, ch, obj, 0, TO_ROOM);
			act("$p hums briefly.&n", FALSE, ch, obj, 0, TO_CHAR);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_BLESS, GET_LEVEL(ch), 0);
			}
			else
			{
				spell_bless(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "undead"))
		{
			act("$n says 'undead' to $p.", FALSE, ch, obj, 0, TO_ROOM);
			act("You say 'undead'", FALSE, ch, obj, 0, TO_CHAR);
			act("$p hums briefly...&n", FALSE, ch, obj, 0, TO_ROOM);
			act("$p hums briefly.&n", FALSE, ch, obj, 0, TO_CHAR);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_PROT_UNDEAD, GET_LEVEL(ch), 0);
			}
			else
			{
				spell_prot_undead(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "consecrate"))
		{
			act("$n says 'consecrate' to $p.", FALSE, ch, obj, 0, TO_ROOM);
			act("You say 'consecrate'", FALSE, ch, obj, 0, TO_CHAR);
			act("$n taps $p on the ground three times...&n", FALSE, ch, obj, 0, TO_ROOM);
			act("You tap $p on the ground three times...&n", FALSE, ch, obj, 0, TO_CHAR);
			spell_consecrate_land(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0);
			return TRUE;
		}
		else if (isname(arg, "freedom"))
		{
			curr_time = get_time();
			if (obj->timer[1] + 900 <= curr_time)
			{
				act("$n says 'freedom' to $p.", FALSE, ch, obj, 0, TO_ROOM);
				act("You say 'freedom'", FALSE, ch, obj, 0, TO_CHAR);
				act("$n taps $p on the ground three times...&n", FALSE, ch, obj, 0, TO_ROOM);
				act("You tap $p on the ground three times...&n", FALSE, ch, obj, 0, TO_CHAR);
				spell_freedom_of_movement(46, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				for (struct group_list *gl = ch->group; gl; gl = gl->next)
					spell_freedom_of_movement(46, ch, 0, SPELL_TYPE_SPELL, gl->ch, 0);
				obj->timer[1] = curr_time;
				return TRUE;
			}
		}
	}

	// Ok, past all the periodic stuff, time for attack procs
	victim = GET_OPPONENT(ch);
	// 10% chance to proc.
	if (IS_ALIVE(victim) && cmd == CMD_MELEE_HIT && number(0, 100) < 10)
	{
		if (number(0, 2) && !affected_by_spell(victim, SPELL_ENTANGLE))
		{
			if (!OUTSIDE(ch))
			{
				return FALSE;
			}
			act("&+gYou strike the ground with $p&+g and call the plants to bind your foes!&n", TRUE, ch, obj, victim, TO_CHAR);
			act("&+g$n&+g strikes the ground with $p&+g and calls to the nearby plants...&n", TRUE, ch, obj, victim, TO_ROOM);
			spell_entangle(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, victim, NULL);
			return TRUE;
		}
		else if (number(0, 100) >= 50)
		{
			act("&+yYou draw&+Y energy&+y from &+gworld&+y around you into $p&+y and release them at $N&n.", TRUE, ch, obj, victim, TO_CHAR);
			act("$n&+y releases pent up&+Y energy&+y from $p&+y to blast&+y you.&n", TRUE, ch, obj, victim, TO_VICT);
			act("$n&+y releases pent up &+Yenergy&+y from $p&+y to blast $N&+y.&n", TRUE, ch, obj, victim, TO_NOTVICT);
			spell_sunray(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, victim, NULL);
			return TRUE;
		}
		else
		{
			act("&+yAs the &+Yenergy&+y diss&+Lipates, it is replaced by a hoard of biting &+ginsects&+L!&n", TRUE, ch, obj, victim, TO_CHAR);
			act("&+yAs the &+Yenergy&+y diss&+Lipates, it is replaced by a hoard of biting &+ginsects&+L!&n", TRUE, ch, obj, victim, TO_ROOM);
			spell_cdoom(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, victim, NULL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// timer[0] = class krindor is currently set to, 0 = original(reset) mode.
int vecna_krindor_main(P_obj obj, P_char ch, int cmd, char *arg)
{
	char   buf[MAX_STRING_LENGTH];
	P_char owner = NULL;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj)
		return FALSE;

	if (OBJ_CARRIED(obj))
	{
		owner = obj->loc.carrying;
	}
	else if (OBJ_WORN(obj))
	{
		owner = obj->loc.wearing;
	}

	if (owner)
	{
		// We need to reset when the device is not set to the player's main class,
		//   and it's not already at its default form.
		if (obj->timer[0] != owner->player.m_class && obj->timer[0] != 0)
		{
			reset_krindor(obj);
			return FALSE;
		}

		if (cmd == CMD_GOTHIT && GET_CLASS(ch, CLASS_ILLUSIONIST) && obj->timer[0] == CLASS_ILLUSIONIST)
		{
			krindor_illusionist(obj, owner, cmd, arg);
		}

		switch (owner->player.m_class)
		{
			case CLASS_ROGUE:
				if (obj->timer[0] != CLASS_ROGUE)
				{
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+gBoots of &+ythe Night&n.", TRUE, owner, obj, 0, TO_ROOM);
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+gBoots of &+ythe Night&n.", TRUE, owner, obj, 0, TO_CHAR);
					snprintf(buf, MAX_STRING_LENGTH, "&+cKrindor's &+gBoots of &+ythe Night&n");
					obj->short_description = NULL;
					obj->str_mask |= STRUNG_DESC2;
					obj->short_description = str_dup(buf);
					obj->type              = ITEM_ARMOR;
					SET_BIT(obj->wear_flags, ITEM_WEAR_FEET);
					REMOVE_BIT(obj->wear_flags, ITEM_HOLD);
					SET_BIT(obj->extra_flags, ITEM_ALLOWED_CLASSES);
					SET_BIT(obj->anti_flags, owner->player.m_class);

					obj->affected[0].location = APPLY_HIT;
					obj->affected[0].modifier = 10;
					obj->affected[1].location = APPLY_DAMROLL;
					obj->affected[1].modifier = 5;
					SET_BIT(obj->bitvector, AFF_DETECT_INVISIBLE);
					SET_BIT(obj->bitvector, AFF_SNEAK);
					SET_BIT(obj->bitvector, AFF_HIDE);

					obj->timer[0] = owner->player.m_class;
					return FALSE;
				}
				if (krindor_rogue(obj, owner, cmd, arg))
				{
					return TRUE;
				}
				break;
			case CLASS_BARD:
				if (obj->timer[0] != CLASS_BARD)
				{
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+gMusic &+yBox&n.", TRUE, owner, obj, 0, TO_ROOM);
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+gMusic &+yBox&n.", TRUE, owner, obj, 0, TO_CHAR);
					snprintf(buf, MAX_STRING_LENGTH, "&+cKrindor's &+gMusic &+yBox&n");
					obj->short_description = NULL;
					obj->str_mask |= STRUNG_DESC2;
					obj->short_description = str_dup(buf);
					obj->type              = ITEM_INSTRUMENT;
					SET_BIT(obj->wear_flags, ITEM_HOLD);
					SET_BIT(obj->wear_flags, ITEM_ATTACH_BELT);
					SET_BIT(obj->extra_flags, ITEM_ALLOWED_CLASSES);
					SET_BIT(obj->anti_flags, owner->player.m_class);
					obj->affected[0].location = APPLY_HIT_REG;
					obj->affected[0].modifier = 10;
					obj->affected[1].location = APPLY_INT_MAX;
					obj->affected[1].modifier = 5;
					obj->affected[2].location = APPLY_SAVING_BREATH;
					obj->affected[2].modifier = -10;
					SET_BIT(obj->bitvector, AFF_HASTE);
					SET_BIT(obj->bitvector4, AFF4_DETECT_ILLUSION);
					SET_BIT(obj->bitvector, AFF_SNEAK);
					obj->value[0] = 1000;
					obj->value[1] = MAXLVL;
					// Min level to use it.. rofl.. 1000 is not a good setting.
					obj->value[3] = 51;
					obj->timer[0] = owner->player.m_class;
					return FALSE;
				}
				if (krindor_bard(obj, owner, cmd, arg))
				{
					return TRUE;
				}
				break;
			case CLASS_PSIONICIST:
				if (obj->timer[0] != CLASS_PSIONICIST)
				{
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+gMask of &+ythe Illithid&n.", TRUE, owner, obj, 0, TO_ROOM);
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+gMask of &+ythe Illithid&n.", TRUE, owner, obj, 0, TO_CHAR);
					snprintf(buf, MAX_STRING_LENGTH, "&+cKrindor's &+gMask of &+ythe Illihthid&n");
					obj->short_description = NULL;
					obj->str_mask |= STRUNG_DESC2;
					obj->short_description = str_dup(buf);
					obj->type              = ITEM_WORN;
					SET_BIT(obj->wear_flags, ITEM_WEAR_FACE);
					REMOVE_BIT(obj->wear_flags, ITEM_HOLD);
					SET_BIT(obj->extra_flags, ITEM_ALLOWED_CLASSES);
					SET_BIT(obj->anti_flags, owner->player.m_class);
					obj->affected[0].location = APPLY_HIT;
					obj->affected[0].modifier = 20;
					obj->affected[1].location = APPLY_MANA;
					obj->affected[1].modifier = 100;
					obj->affected[2].location = APPLY_POW_MAX;
					obj->affected[2].modifier = 10;
					SET_BIT(obj->bitvector, AFF_DETECT_INVISIBLE);
					obj->timer[0] = owner->player.m_class;
					return FALSE;
				}
				if (krindor_psionicist(obj, owner, cmd, arg))
				{
					return TRUE;
				}
				break;
			case CLASS_MONK:
				if (obj->timer[0] != CLASS_MONK)
				{
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+gMu&+Clt&+Mi&+Cco&+glor &+yBelt&n.", TRUE, owner, obj, 0, TO_ROOM);
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+gMu&+Clt&+Mi&+Cco&+glor &+yBelt&n.", TRUE, owner, obj, 0, TO_CHAR);
					snprintf(buf, MAX_STRING_LENGTH, "&+cKrindor's &+gMu&+Clt&+Mi&+Cco&+glor &+yBelt&n");
					obj->short_description = NULL;
					obj->str_mask |= STRUNG_DESC2;
					obj->short_description = str_dup(buf);
					obj->type              = ITEM_WORN;
					SET_BIT(obj->wear_flags, ITEM_WEAR_WAIST);
					REMOVE_BIT(obj->wear_flags, ITEM_HOLD);
					SET_BIT(obj->extra_flags, ITEM_ALLOWED_CLASSES);
					SET_BIT(obj->anti_flags, owner->player.m_class);
					obj->affected[0].location = APPLY_HITROLL;
					obj->affected[0].modifier = 5;
					obj->affected[1].location = APPLY_DAMROLL;
					obj->affected[1].modifier = 5;
					SET_BIT(obj->bitvector, AFF_DETECT_INVISIBLE);
					SET_BIT(obj->bitvector, AFF_HASTE);
					SET_BIT(obj->bitvector2, AFF2_VAMPIRIC_TOUCH);
					obj->timer[0] = owner->player.m_class;
					return FALSE;
				}
				if (krindor_monk(obj, owner, cmd, arg))
				{
					return TRUE;
				}
				break;
			case CLASS_ILLUSIONIST:
				if (obj->timer[0] != CLASS_ILLUSIONIST)
				{
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+MM&+ma&+Ms&+mk of &+MI&+ml&+Ml&+mu&+Ms&+mi&+Mo&+mn&+Ms&n.", TRUE, owner, obj, 0, TO_ROOM);
					act("$p&n &+Lwrithes and warps as it reforms into &+cKrindor's &+MM&+ma&+Ms&+mk of &+MI&+ml&+Ml&+mu&+Ms&+mi&+Mo&+mn&+Ms&n.", TRUE, owner, obj, 0, TO_CHAR);
					snprintf(buf, MAX_STRING_LENGTH, "&+cKrindor's &+MM&+ma&+Ms&+mk of &+MI&+ml&+Ml&+mu&+Ms&+mi&+Mo&+mn&+Ms&n");
					obj->short_description = NULL;
					obj->str_mask |= STRUNG_DESC2;
					obj->short_description = str_dup(buf);
					obj->type              = ITEM_ARMOR;
					SET_BIT(obj->wear_flags, ITEM_WEAR_FACE);
					REMOVE_BIT(obj->wear_flags, ITEM_HOLD);
					SET_BIT(obj->extra_flags, ITEM_ALLOWED_CLASSES);
					SET_BIT(obj->anti_flags, owner->player.m_class);
					obj->affected[0].location = APPLY_INT;
					obj->affected[0].modifier = 10;
					obj->affected[1].location = APPLY_AC;
					obj->affected[1].modifier = -50;
					SET_BIT(obj->bitvector4, AFF4_DETECT_ILLUSION);
					SET_BIT(obj->bitvector, AFF_SNEAK);
					SET_BIT(obj->bitvector, AFF_INVISIBLE);
					SET_BIT(obj->bitvector2, AFF2_DETECT_MAGIC);
					SET_BIT(obj->bitvector3, AFF3_REDUCE);
					obj->timer[0] = owner->player.m_class;
					return FALSE;
				}
				break;
			default:
				if (cmd == CMD_PERIODIC && OBJ_WORN(obj))
				{
					int random = number(0, 100);
					int heal   = dice(2, 10) + 50;
					act("$p&n &+Lstrains against your control.&n", TRUE, owner, obj, 0, TO_ROOM);
					act("$p&n &+Lstrains against $n&+L's control.&n", TRUE, owner, obj, 0, TO_CHAR);

					if (random > 80)
					{
						spell_wither(60, owner, 0, SPELL_TYPE_SPELL, owner, 0);
						return FALSE;
					}
					if (random > 60)
					{
						GET_HIT(owner) = BOUNDED(1, GET_HIT(owner) - heal, GET_MAX_HIT(owner));
						return FALSE;
					}
					if (random > 40)
					{
						GET_VITALITY(owner) = BOUNDED(1, GET_VITALITY(owner) - heal, GET_MAX_VITALITY(owner));
						return FALSE;
					}
					if (random > 20)
					{
						spell_dispel_magic(60, owner, 0, SPELL_TYPE_SPELL, owner, 0);
						return FALSE;
					}
					act("$p&n &+Lgroans as it longs for someone else to wield it.&n", TRUE, owner, obj, 0, TO_ROOM);
					act("$p&n &+Lgroans as it longs for someone else to wield it.&n", TRUE, owner, obj, 0, TO_CHAR);
					return FALSE;
				}
				break;
		}
	}
	else if (!owner && obj->timer[0] != 0)
	{
		reset_krindor(obj);
	}
	return FALSE;
}

void reset_krindor(P_obj obj)
{
	P_obj tobj;
	int   i;

	if (!obj)
	{
		return;
	}

	act("$p&n &+Lwrithes and warps as it reforms to it's original form.&n", TRUE, 0, obj, 0, TO_ROOM);
	act("$p&n &+Lwrithes and warps as it reforms to it's original form.&n", TRUE, 0, obj, 0, TO_CHAR);

	if (obj->type == ITEM_CONTAINER)
	{
		for (tobj = obj->contains; tobj; tobj = tobj->next_content)
		{
			if (OBJ_INSIDE(tobj))
			{
				obj_from_obj(tobj);
				if (OBJ_ROOM(obj))
					obj_to_room(tobj, obj->loc.room);
				// Note to self, if its possible to hand this bag to someone else,
				// it's possible to abuse this and prevent people from leaving the
				// room due to weight restrictions.  Might consider options to
				// prevent reset_krindor from being called when the bag has been
				// given to someone else under certain circumstances.
				else if (OBJ_CARRIED(obj))
					obj_to_char(tobj, obj->loc.carrying);
				else if (OBJ_WORN(obj))
					obj_to_char(tobj, obj->loc.wearing);
				else
					debug("Error removing items from Krindor container");
			}
		}
	}

	obj->short_description = str_dup("&+cKrindor's &+gIncredible &+yDevice&n");
	obj->wear_flags        = ITEM_TAKE | ITEM_HOLD;
	obj->anti_flags        = 0;
	obj->type              = ITEM_TRASH;
	obj->timer[0]          = 0;
	REMOVE_BIT(obj->extra_flags, ITEM_ALLOWED_CLASSES);
	REMOVE_BIT(obj->bitvector, AFF_DETECT_INVISIBLE);
	REMOVE_BIT(obj->bitvector, AFF_SNEAK);
	REMOVE_BIT(obj->bitvector, AFF_HASTE);
	REMOVE_BIT(obj->bitvector, AFF_HIDE);
	REMOVE_BIT(obj->bitvector2, AFF2_VAMPIRIC_TOUCH);
	if (IS_SET(obj->bitvector4, AFF4_DETECT_ILLUSION))
		REMOVE_BIT(obj->bitvector4, AFF4_DETECT_ILLUSION);
	if (IS_SET(obj->bitvector, AFF_INVISIBLE))
		REMOVE_BIT(obj->bitvector, AFF_INVISIBLE);
	if (IS_SET(obj->bitvector2, AFF2_DETECT_MAGIC))
		REMOVE_BIT(obj->bitvector2, AFF2_DETECT_MAGIC);
	if (IS_SET(obj->bitvector3, AFF3_REDUCE))
		REMOVE_BIT(obj->bitvector3, AFF3_REDUCE);

	for (i = 0; i < 7; i++)
	{
		if (i < 4)
		{
			obj->affected[i].location = 0;
			obj->affected[i].modifier = 0;
		}
		obj->value[i] = 0;
	}
	return;
}

// 0 None
// 1 Warrior
// 2 Ranger
//* 3 Psionicist
int krindor_psionicist(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd != CMD_PERIODIC)
		return FALSE;

	if (!obj || (obj && !OBJ_WORN(obj)))
		return FALSE;

	if (!IS_AFFECTED3(ch, AFF3_CANNIBALIZE))
	{
		act("$n&+L's eyes flash with &+wdark power&+L beneath $p&+L...&n", TRUE, ch, 0, obj, TO_ROOM);
		act("&+LThe &+Mtentacles&+L of $p &+Lreadjust momentarily...&n", TRUE, ch, obj, GET_OPPONENT(ch), TO_CHAR);
		spell_cannibalize(60, ch, 0, 0, ch, 0);
		return FALSE;
	}

	if (IS_FIGHTING(ch) && !number(0, 3) && GET_POS(ch) == POS_STANDING && CAN_ACT(ch))
	{
		act("$n&+L's eyes flash with &+Wimmense power&+L beneath $p&+L...&n", TRUE, ch, 0, obj, TO_ROOM);
		act("&+LYou focus intently on $N&+L, while $p&+L silently empowers your will...&n", TRUE, ch, obj, GET_OPPONENT(ch), TO_CHAR);
		spell_detonate(60, ch, 0, 0, GET_OPPONENT(ch), 0);
		return FALSE;
	}

	return FALSE;
}
// 4 Paladin
// 5 Anti-Paladin
// 6 Cleric
// 7* Monk
int krindor_monk(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (!obj)
		return FALSE;

	if (ch && cmd == CMD_PERIODIC && OBJ_WORN(obj))
	{
		return FALSE;
	}
	return FALSE;
}
// 8 Druid
// 9 Shaman
// 10 Sorcerer
// 11 Necromancer
// 12 Conjurer
// 13 Rogue
int krindor_rogue(P_obj obj, P_char ch, int cmd, char *arg)
{
	int room = ch->in_room;

	// If they exist, are trying to hide, and not faerie fired.. then they can try to quick-hide.
	if (ch && cmd == CMD_HIDE && OBJ_WORN(obj) && !affected_by_spell(ch, SPELL_FAERIE_FIRE))
	{
		// Ok, if they're not water and room is water, they can't hide, so fail here instead of checking twice.
		if (!IS_WATERFORM(ch) && (IS_WATER(room) || IS_WATER_ROOM(room)))
		{
			send_to_char("&+LYou swim around madly, but find no hiding spot...&n\r\n", ch);
			return TRUE;
		}
		// Enhanced hide for assassins. 100%, no-lag.
		send_to_char("&+LYou slide into the shadows quickly...&n\r\n", ch);
		SET_BIT(ch->specials.affected_by, AFF_HIDE);
		// 1/6 the lag on regular hide.
		CharWait(ch, PULSE_VIOLENCE / 2);
		return TRUE;
	}
	return FALSE;
}
// 14 Assassin
// 15 Mercenary
//* 16 Bard
int krindor_bard(P_obj obj, P_char ch, int cmd, char *arg) { return FALSE; }
// 17 Thief
// 18 Warlock
// 19 MindFlayer
// 20 Alchemist
// 21 Berserker
//* 22 Reaver
//* 23 Illusionist (below)
// 24 unused
// 25 Dreadlord
//* 26 Ethermancer
// 27 Avenger

int krindor_illusionist(P_obj obj, P_char ch, int cmd, char *arg)
{
	int               random = dice(2, 3);
	struct proc_data *data;
	P_char            victim;

	if (!(number(0, 49))) // 2 percent
	{
		data   = (struct proc_data *)arg;
		victim = data->victim;

		act("Your $q &+ys&+Yh&+yi&+Ym&+ym&+Ye&+yr&+Ys!", FALSE, ch, obj, victim, TO_CHAR | ACT_NOTTERSE);
		act("$n's $q &+ys&+Yh&+yi&+Ym&+ym&+Ye&+yr&+Ys...", FALSE, ch, obj, victim, TO_VICT | ACT_NOTTERSE);
		act("$n's $q &+ys&+Yh&+yi&+Ym&+ym&+Ye&+yr&+Ys...", FALSE, ch, obj, victim, TO_NOTVICT | ACT_NOTTERSE);

		if (random == 2 || random == 6)
		{
			spell_reflection(50, ch, SPELL_TYPE_SPELL, 0, ch, 0);
			return true;
		}

		if (!(victim) || victim->in_room != ch->in_room)
			return false;

		if (random == 3 || random == 4)
		{
			spell_hammer(GET_LEVEL(ch), ch, SPELL_TYPE_SPELL, 0, victim, 0);
			return true;
		}

		if (random == 5)
		{
			spell_delirium(60, ch, SPELL_TYPE_SPELL, 0, victim, 0);
			return true;
		}
	}
	return false;
}

// Lucrot June09
int vecna_death_mask(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char            victim = NULL;
	int               num = 0, random = 0;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || cmd != CMD_GOTHIT)
	{
		return FALSE;
	}

	if (IS_NPC(ch) && !IS_PC_PET(ch) && strstr(ch->player.name, "vecna"))
	{
		num = number(0, 19); // 5% for vecna.
	}
	else
	{
		num = number(0, 99); // 1% for players
	}

	if (cmd == CMD_GOTHIT && !num)
	{
		if (!(data = (struct proc_data *)arg))
		{
			return FALSE;
		}
		victim = data->victim;
		if (!IS_ALIVE(victim) || victim->in_room != ch->in_room)
		{
			return FALSE;
		}

		if (IS_NPC(ch) && !IS_PC_PET(ch) && strstr(ch->player.name, "vecna"))
		{
			random = 65;
		}
		else
		{
			random = number(1, GET_LEVEL(ch));
		}

		if (random >= 56 && !number(0, 19) && !IS_CASTING(ch))
		{
			spell_acidimmolate(65, ch, 0, SPELL_TYPE_SPELL, victim, 0);
			return TRUE;
		}

		if (IS_UNDEADRACE(victim))
		{
			act("Your $q begins to &+Yg&+yl&+Yo&+yw&n and bolts of &+Gp&+gu&+Gt&+gr&+Gi&+Ld g&+gr&+Gee&+gn m&+Lag&+Gic &+glance&n into $N!", FALSE, ch, obj, victim, TO_CHAR | ACT_NOTTERSE);
			act("$n's $q begins to &+Yg&+yl&+Yo&+yw&n and bolts of &+Gp&+gu&+Gt&+gr&+Gi&+Ld g&+gr&+Gee&+gn m&+Lag&+Gic &+glance&n into you!", FALSE, ch, obj, victim, TO_VICT | ACT_NOTTERSE);
			act("$n's $q begins to &+Yg&+yl&+Yo&+yw&n and bolts of &+Gp&+gu&+Gt&+gr&+Gi&+Ld g&+gr&+Gee&+gn m&+Lag&+Gic &+glance&n into $N.", FALSE, ch, obj, victim, TO_NOTVICT | ACT_NOTTERSE);

			spell_undead_to_death(random, ch, 0, SPELL_TYPE_SPELL, victim, 0);
			return TRUE;
		}

		if (IS_HUMANOID(victim))
		{
			act("Your $q begins to &+Yg&+yl&+Yo&+yw&n and &+wt&+cw&+wi&+cn &+ybeams&n of &+Lblack magic lance&n into $N!", FALSE, ch, obj, victim, TO_CHAR | ACT_NOTTERSE);
			act("$n's $q begins to &+Yg&+yl&+Yo&+yw&n and &+wt&+cw&+wi&+cn &+ybeams&n of &+Lblack magic lance&n into you!", FALSE, ch, obj, victim, TO_VICT | ACT_NOTTERSE);
			act("$n's $q begins to &+Yg&+yl&+Yo&+yw&n and &+wt&+cw&+wi&+cn &+ybeams&n of &+Lblack magic lance&n into $N.", FALSE, ch, obj, victim, TO_NOTVICT | ACT_NOTTERSE);

			switch (number(1, 3))
			{
				case 1:
				{
					spell_wither(random, ch, 0, SPELL_TYPE_SPELL, victim, 0);
					break;
				}
				case 2:
				{
					spell_negative_concussion_blast(random, ch, 0, SPELL_TYPE_SPELL, victim, 0);
					break;
				}
				case 3:
				{
					spell_dispel_magic(random, ch, 0, SPELL_TYPE_SPELL, victim, 0);
					break;
				}
				default:
					break;
			}

			if (!number(0, 2))
			{
				act("&+wYour&n $q &+yoozes &+wcorrosive decaying matter upon your skin...", FALSE, ch, obj, victim, TO_CHAR | ACT_NOTTERSE);
				spell_damage(ch, ch, number(16, 80), SPLDAM_ACID, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
			}
		}
		return TRUE;
	}
	return FALSE;
}

int mob_vecna_procs(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data    *data;
	struct affected_type af;
	P_char               victim;
	int                  dam;

	if (cmd != CMD_GOTHIT)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if (!number(0, 40) && !IS_PC(ch) && !IS_PC_PET(ch))
	{
		if (!(data = (struct proc_data *)arg))
		{
			return FALSE;
		}
		victim = data->victim;
		if (!IS_ALIVE(victim) || victim->in_room != ch->in_room)
		{
			return FALSE;
		}

		if (!number(0, 25) || (strstr(ch->player.name, "lich") && !number(0, 15)))
		{
			act("&+LVecna makes his presence known by channeling his decrepit ancient power through&n $n!&n", TRUE, ch, 0, 0, TO_ROOM);
			switch (number(1, 5))
			{
				case 1:
					spell_summon_ghasts(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
					break;
				case 2:
					spell_negative_concussion_blast(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
					break;
				case 3:
					spell_dispel_magic(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
					break;
				case 4:
					spell_forked_lightning(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
					break;
				case 5:
					spell_incendiary_cloud(60, ch, 0, 0, victim, 0);
					break;
				default:
					break;
			}
			return TRUE;
		}

		// Insta-kill undead PC pets.
		if (IS_PC_PET(victim) && IS_UNDEADRACE(victim))
		{
			act("$N groans an unearthly sound and crumbles to dust!&n", TRUE, ch, 0, victim, TO_CHAR);
			act("$n &+Ltouch extinguishes your life force. Vecna's power is too mighty!!!&n", TRUE, ch, 0, victim, TO_VICT);
			act("$n grasps $N! $N moans an unearthly sound and crumbles to dust!&n", TRUE, ch, 0, victim, TO_NOTVICT);
			die(victim, ch);
			return TRUE;
		}

		// Undead PC loses 50% of their hitpoints.
		if (IS_PC(victim) && IS_UNDEADRACE(victim))
		{
			act("&+LVecna's maligned essence flows around you... You hear cackling in the distance!&n", TRUE, ch, 0, victim, TO_VICT);
			dam = (int)(GET_HIT(victim) * 0.50);
			spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
			return TRUE;
		}
		// A nasty curse that affects all stats, combat and spell pulse.
		if (strstr(ch->player.name, "ghast") && !affected_by_spell(victim, SPELL_DISEASE))
		{
			act("You find a narrow weakness in $N's defense and move in for a strike!&n", TRUE, ch, 0, victim, TO_CHAR);
			act("$n's &+ydiseased claws&n cut ever so slightly into your exposed skin...&n", TRUE, ch, 0, victim, TO_VICT);

			send_to_char("&+yYou feel extremely tired and fatigued. &+WCysts form on your skin!\r\n", victim);
			act("$n &+ysuddenly looks tired and fatigued. &+WCysts &+ystart to form on $m skin", FALSE, victim, 0, 0, TO_ROOM);

			bzero(&af, sizeof(af));
			af.type     = SPELL_DISEASE;
			af.duration = 3;
			af.modifier = (-1 * number(12, 20));
			af.flags    = AFFTYPE_NODISPEL;
			af.location = APPLY_STR;
			af.modifier = (-1 * number(12, 20));
			affect_to_char(victim, &af);

			af.location = APPLY_DEX;
			af.modifier = (-1 * number(12, 20));
			affect_to_char(victim, &af);

			af.location = APPLY_AGI;
			af.modifier = (-1 * number(12, 20));
			affect_to_char(victim, &af);

			af.location = APPLY_WIS;
			af.modifier = (-1 * number(12, 20));
			affect_to_char(victim, &af);

			af.location = APPLY_INT;
			af.modifier = (-1 * number(12, 20));
			affect_to_char(victim, &af);

			af.modifier = (-1 * number(10, 40));
			af.location = APPLY_CHA;
			affect_to_char(victim, &af);

			af.duration = 3;
			af.modifier = 5;
			af.location = APPLY_COMBAT_PULSE;
			affect_to_char(victim, &af);

			af.duration = 3;
			af.modifier = 2;
			af.location = APPLY_SPELL_PULSE;
			affect_to_char(victim, &af);

			af.location = APPLY_CON;
			af.modifier = (-1 * number(12, 20));
			affect_to_char(victim, &af);
		}

		if (strstr(ch->player.name, "knight"))
		{
			act("Your dark eyes find $N's soul, and you begin to extinguish it...&n", TRUE, ch, 0, victim, TO_CHAR);
			act("$n's &+Ldark eyes&n lock on to something within you. You feel your life rush from your body!&n", TRUE, ch, 0, victim, TO_VICT);
			act("$n's &+Ldark eyes &+yburn&n into $N's soul!&n", TRUE, ch, 0, victim, TO_NOTVICT);

			if (IS_AFFECTED2(victim, AFF2_SOULSHIELD))
			{
				send_to_char("Your soul is shielded!\r\n", victim);
				act("$N seems unaffected...&n", TRUE, ch, 0, victim, TO_NOTVICT);
			}
			else
			{
				act("$N staggers and shudders!!!&n", TRUE, ch, 0, victim, TO_NOTVICT);
				dam = number(120, 400);
				spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
			}
			return TRUE;
		}

		if (strstr(ch->player.name, "dracolich"))
		{
			act("Your massive claw slams into $N.&n", TRUE, ch, 0, victim, TO_CHAR);
			act("$n's massive claw shreds you!&n", TRUE, ch, 0, victim, TO_VICT);
			act("$n's massive claw shreds $N!&n", TRUE, ch, 0, victim, TO_NOTVICT);

			dam = number(400, 600);
			dam /= 4;
			melee_damage(ch, victim, dam, PHSDAM_TOUCH | PHSDAM_NOREDUCE | PHSDAM_NOPOSITION, 0);
			CharWait(victim, PULSE_VIOLENCE * 2);
			return TRUE;
		}

		if (strstr(ch->player.name, "wight"))
		{
			act("You trip up $N!&n", TRUE, ch, 0, victim, TO_CHAR);
			act("$n trips up $N!&n", TRUE, ch, 0, victim, TO_VICT);
			act("$n trips up $N!&n", TRUE, ch, 0, victim, TO_NOTVICT);

			if (IS_FIGHTING(victim))
			{
				stop_fighting(victim);
			}
			if (IS_DESTROYING(victim))
			{
				stop_destroying(victim);
			}
			SET_POS(victim, POS_KNEELING + GET_STAT(victim));
			CharWait(victim, PULSE_VIOLENCE * 2);
			return TRUE;
		}
		if (strstr(ch->player.name, "ghoul"))
		{
			act("You brush up against $N.&n", TRUE, ch, 0, victim, TO_CHAR);
			act("$n &+Lbrushes up against you!&n", TRUE, ch, 0, victim, TO_VICT);
			act("$n &+Lbrushes up against $N!&n", TRUE, ch, 0, victim, TO_NOTVICT);

			if (IS_FIGHTING(victim))
			{
				stop_fighting(victim);
			}
			if (IS_DESTROYING(victim))
			{
				stop_destroying(victim);
			}
			CharWait(victim, PULSE_VIOLENCE * 2);
			berserk(victim, 5);
			return TRUE;
		}

		if (strstr(ch->player.name, "shadow") || strstr(ch->player.name, "ghost") || strstr(ch->player.name, "wraith"))
		{
			act("Your negative essence flows out to engulf $N!&n", TRUE, ch, 0, victim, TO_CHAR);
			act("A &+Lpitch black tendril&n flows out from $n and into you!&n", TRUE, ch, 0, victim, TO_VICT);
			act("Something sinister, &+Lblack&n, and deathly flows from $n into $N!&n", TRUE, ch, 0, victim, TO_NOTVICT);

			dam = (int)(GET_HIT(victim) * 0.85);
			spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
			CharWait(victim, PULSE_VIOLENCE * 1);
			return TRUE;
		}

		if (strstr(ch->player.name, "vampire") && GET_POS(ch) == POS_STANDING && !IS_STUNNED(ch))
		{
			act("You leap at $N and sink your teeth into $S flesh!&n", TRUE, ch, 0, victim, TO_CHAR);
			act("$n leaps into you, and sinks $s teeth deep into your flesh!&n", TRUE, ch, 0, victim, TO_VICT);
			act("$n bears $s large fangs, growls, and leap at $N!&n", TRUE, ch, 0, victim, TO_NOTVICT);

			dam = number(200, 300);
			dam /= 4;
			melee_damage(ch, victim, dam, PHSDAM_TOUCH | PHSDAM_NOREDUCE | PHSDAM_NOPOSITION, 0);

			if (GET_POS(victim) == POS_STANDING && GET_SIZE(ch) >= GET_SIZE(victim))
			{
				SET_POS(victim, POS_KNEELING + GET_STAT(victim));
			}
			CharWait(victim, PULSE_VIOLENCE * 1);
			return TRUE;
		}
	}
	return FALSE;
}
