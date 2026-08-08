#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utility.h"
#include "utils.h"
#include "tradeskill.h"
#include "mining.h"
#include "mining_config.h"
#include "achievements.h"
#include "map.h"
#include "objmisc.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "vnum.obj.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

extern P_desc descriptor_list;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;

void create_parchment(P_char ch);
int calc_ore_cost(P_char ch, P_obj ore);
int calc_gem_cost(P_char ch, P_obj gem, bool randommob);

void load_mines(bool set_event, bool load_all, int map);
bool load_one_mine(int map);

struct mine_range_data
{
	char *name;
	char *abbrev;
	int   start;
	int   end;
	int   type;
	int   mine_duration;
	bool  reload;
};

static struct mine_range_data mine_data[] = {
	// Keep entries stable; set the corresponding duris.properties capacity to 0 to disable placement.
	// Region start/end/duration values are overridden by lib/mining.cfg at boot.
	// New mine types also require MINES_* constants in mining.h and handling in mines_properties().
	// Zone display name, command argument matching, start range, end range, mine type, duration, reloading mines?
	// Note: mine_duration is in sets of 4 sec: 11 -> 9 * 4 = 36 sec, since event_mine_check occurs every 4 sec.
	{"Surface Map", "map", 500000, 659999, VOBJ_MINE, 9, TRUE},
	{"Underdark", "ud", 700000, 859999, VOBJ_MINE, 9, TRUE},
	{"Tharnadia Rift", "tharnrift", 110000, 119999, VOBJ_MINE, 9, FALSE},
	{"Surface Map - G", "mapg", 500000, 659999, VOBJ_GEMMINE, 15, TRUE},
	{"Underdark - G", "udg", 700000, 859999, VOBJ_GEMMINE, 15, TRUE},
	{0}
};

struct mines_event_data
{
	int map;
};

int mines_properties(int map)
{
	switch (map)
	{
		case MINES_MAP_SURFACE:
			return (int)get_property("mines.maxSurfaceMap", 50);
			break;
		case MINES_MAP_UD:
			return (int)get_property("mines.maxUD", 50);
			break;
		case MINES_MAP_THARNRIFT:
			return (int)get_property("mines.maxTharnRift", 50);
			break;
		case MINES_GEM_SURFACE:
			return (int)get_property("mines.maxGemSurface", 6);
			break;
		case MINES_GEM_UD:
			return (int)get_property("mines.maxGemUD", 2);
			break;
		default:
			logit(LOG_DEBUG, "mines_properties(): passing invalid map to function, using default 50 mines");
			return 50;
			break;
	}
}

// #define IS_MINING_PICK(obj) ( OBJ_VNUM(obj) == 253 || \
// OBJ_VNUM(obj) == 338 || \
// OBJ_VNUM(obj) == 10640 || \
// OBJ_VNUM(obj) == 95531 || \
// OBJ_VNUM(obj) == 49018 )

#define IS_MINING_PICK(obj) (isname("pick", obj->name) && obj->type == ITEM_WEAPON)

P_obj get_pick(P_char ch)
{
	if (!ch)
		return NULL;

	if (ch->equipment[WIELD] && IS_MINING_PICK(ch->equipment[WIELD]))
		return ch->equipment[WIELD];

	if (ch->equipment[WIELD2] && IS_MINING_PICK(ch->equipment[WIELD2]))
		return ch->equipment[WIELD2];

	return NULL;
}

bool mine_friendly(int to_room)
{
	if (world[to_room].sector_type == SECT_HILLS)
		return true;

	if (world[to_room].dir_option[DIR_NORTH] &&
	    (world[world[to_room].dir_option[DIR_NORTH]->to_room].sector_type == SECT_HILLS || world[world[to_room].dir_option[DIR_NORTH]->to_room].sector_type == SECT_MOUNTAIN))
		return true;

	if (world[to_room].dir_option[DIR_EAST] &&
	    (world[world[to_room].dir_option[DIR_EAST]->to_room].sector_type == SECT_HILLS || world[world[to_room].dir_option[DIR_EAST]->to_room].sector_type == SECT_MOUNTAIN))
		return true;

	if (world[to_room].dir_option[DIR_SOUTH] &&
	    (world[world[to_room].dir_option[DIR_SOUTH]->to_room].sector_type == SECT_HILLS || world[world[to_room].dir_option[DIR_SOUTH]->to_room].sector_type == SECT_MOUNTAIN))
		return true;

	if (world[to_room].dir_option[DIR_WEST] &&
	    (world[world[to_room].dir_option[DIR_WEST]->to_room].sector_type == SECT_HILLS || world[world[to_room].dir_option[DIR_WEST]->to_room].sector_type == SECT_MOUNTAIN))
		return true;

	return false;
}

int get_mine_content(P_obj mine) { return mine->value[0]; }

int remove_mine_content(P_obj mine) { return (mine->value[0]--); }

// Takes a mine quality (regular mine, not gems) and returns a vnum for an ore.
//   This is just for raw ore, the char's luck check (increases value) is in event_mine_check.
//   Mine quality ranges from 0 to 3 (I hope someone edits here if it changes).
// Iron: 4/400 large, 15/400 medium, 7/400 small -> 26/400 total -> 6.5% chance.
// Adamantium: 1/400 large, 8/400 medium, 29/400 small -> 38/400 total -> 9.5% chance.
// Other 6 metals: 4/400 large, 20/400 medium, 32/400 small -> 56/400 total -> 14% chance.
// Note: This makes adamantium a little more likely than iron, but that's ok; it makes people happy.
//   You're most likely to find a small intermediate (non-iron, non-adamantium) ore, then small adamantium,
//   medium intermediate, medium iron, medium adamantium, small iron, large non-adamantium, lastly large adamantium.
int random_ore(int mine_quality)
{
	// Start with a random percentage.
	int quality = number(1, 100);
	// metal_type ranges from 0 to 7 (iron, steel, copper, silver, gold, platinum, mithril, adamantium)
	//   For mine_quality 0 -> 1-11 : 0, 12-25: 1, ... 82-95: 6, 96-100: 7
	//   For mine_quality 1 -> 1-08 : 0, 09-22: 1, ... 79-92: 6, 93-100: 7
	//   For mine_quality 2 -> 1-05 : 0, 06-19: 1, ... 76-89: 6, 90-100: 7
	//   For mine_quality 3 -> 1-02 : 0, 03-16: 1, ... 73-86: 6, 87-100: 7
	// It's important that there's a 14% chance to get adamantium with mine_quality 3, so we can get a large
	//   adamantium ore per the ore_size = ... % 14 must be 13 for a large ore.
	int ore_type = (quality + 3 * mine_quality + 2) / 14;
	// ore_quality ranges from 0 to 2 (small, medium, large)
	//   For each metal type, we have a domain of 0-13 to map to 0-2.
	// Below => (quality+3*mine_quality+2)%14 : ore_size -> 0-7: 0, 8-12: 1, 13: 2
	//   This correlates to 8/14 chance for small, 5/14 for med, 1/14 for large.
	//   Except adamantium only has a domain of 0-4 for mine_quality 0 so it will be small (5% chance) regardless.
	//     And adamantium only has a domain of 0-7 for mine_quality 1 so it will be small 8% regardless.
	//     And adamantium only has a domain of 0-10 for mine_quality 2 so it will be small 8% or medium 3%.
	//     And adamantium has a domain of 0-13 for mine_quality 3 so it will be 8% small, 5% medium, 1% large.
	//   Except iron has a domain of 3-13 for mine_quality 0 so it will be 5/11 small, 5/11 medium, 1/11 large.
	//     And iron has a domain of 6-13 for mine_quality 1 so it will be 2/8 small, 5/8 medium, 1/8 large.
	//     And iron has a domain of 9-13 for mine_quality 2 so it will be 4/5 medium, 1/5 large.
	//     And iron has a domain of 12-13 for mine_quality 3 so it will be 1/2 medium, 1/2 large.
	//     Note: The iron-size probabilities are assuming that the metal is iron (they are numerator% chance overall).
	int ore_size = (((quality + 3 * mine_quality + 2) % 14) - 3) / 5;

	// Range needs to be from 400260 to 400283 : 0 * 3 + 0 + 400260 to 7 * 3 + 2 + 400260.
	return (ore_type * 3) + ore_size + LOWEST_ORE_VNUM;

	/* Old code from when vnums were not in order.
	 * The "LARGE_ADAMANTIUM_ORE" constants and such are not updated.
	 * The above is so much nicer to read, and has decent comments and no if's.

	  // So, quality 0: 1 - 99, 1: 2 - 109, 2: 3 - 119, 3: 4 - 129
	  int x = number(1 + mine_quality, 99 + mine_quality * 10);

	  if(x >= 99 + mine_quality * 9) // 0: 99, 1: 108, 2: 117, 3: 126
	    return LARGE_ADAMANTIUM_ORE;
	  if(x >= 98 + mine_quality * 9) // 0: 98, 1: 107, 2: 116, 3: 125
	    return MEDIUM_ADAMANTIUM_ORE;
	  if(x >= 97 + mine_quality * 9)
	    return SMALL_ADAMANTIUM_ORE;

	  if(x >= 96 + mine_quality * 8)
	    return LARGE_MITHRIL_ORE;
	  if(x >= 95 + mine_quality * 7)
	    return MEDIUM_MITHRIL_ORE;
	  if(x >= 94 + mine_quality * 6)
	    return SMALL_MITHRIL_ORE;

	  if(x >= 91 + mine_quality * 5)
	    return LARGE_PLATINUM_ORE;
	  if(x >= 88 + mine_quality * 4)
	    return MEDIUM_PLATINUM_ORE;
	  if(x >= 85 + mine_quality * 3)
	    return SMALL_PLATINUM_ORE;

	  if(x >= 81 + mine_quality * 2)
	    return LARGE_GOLD_ORE;
	  if(x >= 77 + mine_quality * 1)
	    return MEDIUM_GOLD_ORE;
	  if(x >= 73 + mine_quality * 0)
	    return SMALL_GOLD_ORE;

	  if(x >= 66)
	    return LARGE_SILVER_ORE;
	  if(x >= 59)
	    return MEDIUM_SILVER_ORE;
	  if(x >= 52)
	    return SMALL_SILVER_ORE;

	  if(x >= 44)
	    return LARGE_COPPER_ORE;
	  if(x >= 36)
	    return MEDIUM_COPPER_ORE;
	  if(x >= 28)
	    return SMALL_COPPER_ORE;

	  if(x >= 19)
	    return LARGE_IRON_ORE;
	  if(x >= 10)
	    return MEDIUM_IRON_ORE;

	  return SMALL_IRON_ORE;
	*/
}

P_obj get_ore_from_mine(P_char ch, int mine_quality)
{
	P_obj ore;
	int   ore_type = random_ore(mine_quality);
	ore            = read_object(ore_type, VIRTUAL);
	if (!ore)
	{
		return NULL;
	}
	ore->value[4] = get_time();
	return ore;
}

P_obj get_gem_from_mine(P_char ch, int mine_quality)
{
	P_obj gem = read_object(mining_config_gem_vnum(), VIRTUAL);
	if (!gem) return NULL;
	gem->value[4] = get_time();
	return gem;
}

int mine(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC)
	{
		if (obj->value[0] <= 0)
		{
			extract_obj(obj, TRUE); // Not an arti, but 'in game.'
			return TRUE;
		}
	}

	if (cmd == CMD_MINE)
	{
		if (!ch || !IS_PC(ch) || !IS_ALIVE(ch))
		{
			return FALSE;
		}

		if (GET_CHAR_SKILL(ch, SKILL_MINE) == 0)
		{
			send_to_char("You don't know how to mine.\n", ch);
			return TRUE;
		}

		if (get_scheduled(ch, event_mine_check))
		{
			send_to_char("You're already mining!\n", ch);
			return TRUE;
		}

		if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			send_to_char("You're too relaxed to mine.\n", ch);
			return TRUE;
		}

		P_obj pick = get_pick(ch);
		if (!pick)
		{
			send_to_char("You need to be wielding a suitable mining pick.\n", ch);
			return TRUE;
		}

		if (get_mine_content(obj) <= 0)
		{
			send_to_char("This area has been completely deplenished!\n", ch);
			return TRUE;
		}

		// start mining
		send_to_char("You begin to mine...\n", ch);

		struct mining_data data;
		data.room = ch->in_room;
		// Immortals get 1 bout of 'You continue mining..' to make sure it works right.
		if (IS_TRUSTED(ch))
		{
			data.counter = 2;
		}
		else
		{
			// At 1 skill, roughly twice as long as 100 skill.  At 100 skill, ticks represented by val2 = mine_duration.
			data.counter = (obj->value[2] * 200) / (100 + GET_CHAR_SKILL(ch, SKILL_MINE));
			// Anti-cheater code: less than 16 sec?
			if (data.counter < 4)
			{
				// Punish with a long counter... 3 mins sounds good.
				data.counter = 45;
			}
			// Add a little real life luck variance to create the myths!
			data.counter += number(-1, 1);
		}
		data.mine_quality = obj->value[1];
		data.mine_type    = obj_index[obj->R_num].virtual_number;

		remove_mine_content(obj);

		if (get_mine_content(obj) <= 0)
		{
			send_to_char("There is very little left, but you keep digging one more time!\n", ch);
			extract_obj(obj, TRUE); // Not an arti, but 'in game.'
		}

		add_event(event_mine_check, PULSE_VIOLENCE, ch, 0, 0, 0, &data, sizeof(struct mining_data));
		return TRUE;
	}

	return FALSE;
}

void event_mine_check(P_char ch, P_char victim, P_obj, void *data)
{
	struct mining_data *mdata = (struct mining_data *)data;
	P_obj               ore, pick;
	char                buf[MAX_STRING_LENGTH], dbug[MAX_STRING_LENGTH];
	float               newcost;
	bool                randommob, gem;
	P_char              mob;

	pick = get_pick(ch);

	if (!IS_ALIVE(ch))
	{
		logit(LOG_DEBUG, "event_mine_check: bad ch (%s)", ch ? J_NAME(ch) : "NULL");
		return;
	}

	if (!ch->desc || IS_FIGHTING(ch) || IS_DESTROYING(ch) || (ch->in_room != mdata->room) || !MIN_POS(ch, POS_STANDING + STAT_NORMAL) || IS_SET(ch->specials.affected_by, AFF_HIDE) ||
	    IS_IMMOBILE(ch) || !IS_AWAKE(ch) || IS_STUNNED(ch) || IS_CASTING(ch) || IS_AFFECTED2(ch, AFF2_CASTING))
	{
		send_to_char("You stop mining.\n", ch);
		return;
	}

	// No more invis mining.
	appear(ch);

	if (IS_DISGUISE(ch))
	{
		send_to_char("Mining will ruin your disguise!\r\n", ch);
		return;
	}

	if (!pick)
	{
		send_to_char("How are you supposed to mine when you don't have anything ready to mine with?\n", ch);
		return;
	}

	if (--mdata->counter <= 0)
	{
		if (mdata->mine_type == VOBJ_MINE)
		{
			if (GET_C_LUK(ch) > number(1, 3000))
			{
				ore = get_gem_from_mine(ch, mdata->mine_quality);
				gem = TRUE;
			}
			else
			{
				ore = get_ore_from_mine(ch, mdata->mine_quality);
				if (notch_achievement(ch, AIP_ORE_MINED) == 1000)
					apply_achievement(ch, ACH_DO_YOU_MINE);
				gem = FALSE;
			}
			if (!ore)
			{
				wizlog(56, "event_mine_check: couldn't load ore, quality %d.", mdata->mine_quality);
				logit(LOG_DEBUG, "event_mine_check: couldn't load ore, quality %d.", mdata->mine_quality);
				send_to_char("Your efforts were thwarted by an unseen force.  Tell a God.\n\r", ch);
				return;
			}
			// Moved to a function to make it more readable.
			if (gem)
				newcost = calc_gem_cost(ch, ore, FALSE);
			else
				newcost = calc_ore_cost(ch, ore);
		}
		else if (mdata->mine_type == VOBJ_GEMMINE)
		{
			randommob = FALSE;
			if (!number(0, 20))
			{
				randommob = TRUE;
				send_to_char("You dug up something .. that moves!\n\r", ch);
			}
			ore = get_gem_from_mine(ch, mdata->mine_quality);
			if (!ore)
			{
				wizlog(56, "event_mine_check: couldn't load gem, quality %d.", mdata->mine_quality);
				logit(LOG_DEBUG, "event_mine_check: couldn't load gem, quality %d.", mdata->mine_quality);
				send_to_char("Your efforts were thwarted by a mysterious force.  Tell a God.\n\r", ch);
				return;
			}
			// Moved to a function to make it more readable.
			newcost = calc_gem_cost(ch, ore, randommob);
		}
		else
		{
			wizlog(56, "event_mine_check: unknown mine type %d.", mdata->mine_type);
			logit(LOG_DEBUG, "event_mine_check: unknown mine type %d.", mdata->mine_type);
			send_to_char("Your mine doesn't seem to be a mine anymore.  Tell a God.\n\r", ch);
			return;
		}

		if (number(80, 140) < GET_C_LUK(ch))
		{
			newcost *= 1.3;
			send_to_char("&+yYou &+Ygently&+y break the &+Lore &+yfree from the &+Lrock&+y, preserving its natural form.&n\r\n", ch);
		}

		act("Your mining efforts turn up $p&n!", FALSE, ch, ore, 0, TO_CHAR);
		act("$n finds $p&n!", FALSE, ch, ore, 0, TO_ROOM);

		gain_exp(ch, NULL, (GET_CHAR_SKILL(ch, SKILL_MINE) * 4), EXP_BOON);
		ore->cost = newcost;
		obj_to_room(ore, ch->in_room);
		return;
	}

	if (get_property("halloween", 0.000) && (number(0, 100) < get_property("halloween.zombie.chance", 5.000)))
	{
		halloween_mine_proc(ch);
	}

	if (GET_VITALITY(ch) < 10)
	{
		send_to_char("You are too exhausted to continue mining.\n", ch);
		return;
	}

	if (IS_RIDING(ch))
	{
		send_to_char("Mining while mounted?  Good luck!\n", ch);
		if (!number(0, GET_CHAR_SKILL(ch, SKILL_MINE) / 20))
		{
			act("You fumble your $p!", FALSE, ch, pick, 0, TO_CHAR);
			if (ch->equipment[WIELD] == pick)
			{
				unequip_char(ch, WIELD);
				obj_to_char(pick, ch);
			}
			if (ch->equipment[WIELD2] == pick)
			{
				unequip_char(ch, WIELD2);
				obj_to_char(pick, ch);
			}
			else
			{
				logit(LOG_DEBUG, "event_mine_check: %s has pick '%s' (%d) but not in slot WIELD/WIELD2.", J_NAME(ch), pick->short_description, OBJ_VNUM(pick));
			}
			return;
		}
	}

	if (!notch_skill(ch, SKILL_MINE, get_property("skill.notch.mining", 2.5)) && !number(0, (GET_CHAR_SKILL(ch, SKILL_MINE) * 2)))
	{
		send_to_char("You thought you found something, but it was just worthless rock.\n", ch);
		return;
	}

	if (!number(0, 999))
	{
		create_parchment(ch);
	}

	// If pick breaks, return.
	if (!number(0, 4) && (OBJ_VNUM(pick) != 83318) && DamageOneItem(ch, 1, pick, false))
	{
		return;
	}

	send_to_char("You continue mining...\n", ch);
	notch_skill(ch, SKILL_MINE, get_property("skill.notch.mining", 2.5));
	GET_VITALITY(ch) -= (number(0, 100) > GET_CHAR_SKILL(ch, SKILL_MINE)) ? 3 : 2;

	add_event(event_mine_check, PULSE_VIOLENCE, ch, 0, 0, 0, mdata, sizeof(struct mining_data));

	// Noise distance calc
	for (P_desc i = descriptor_list; i; i = i->next)
	{
		if (i->connected != CON_PLAYING || ch == i->character || i->character->following == ch || world[i->character->in_room].zone != world[ch->in_room].zone ||
		    ch->in_room == i->character->in_room || ch->in_room == real_room(i->character->specials.was_in_room) || real_room(ch->specials.was_in_room) == i->character->in_room)
		{
			continue;
		}

		int dist = calculate_map_distance(ch->in_room, i->character->in_room);

		if (dist <= 400 && (number(1, 12) < 3))
		{
			zone_spellmessage(ch->in_room,
			                  TRUE,
			                  "&+yThe sound of &+wmetal &+yhewing &+Lrock&+y can be heard in the distance...&n\r\n",
			                  "&+yThe sound of &+wmetal &+yhewing &+Lrock&+y can be heard in the distance to the %s...&n\r\n");
		}
	}
}

void initialize_mining()
{
	mining_config_boot();

	obj_index[real_object0(VOBJ_MINE)].func.obj = mine;
	obj_index[real_object0(VOBJ_GEMMINE)].func.obj = mine;

	for (int i = 0; mine_data[i].name; i++)
	{
		load_mines(mine_data[i].reload, TRUE, i);
	}
}

bool invalid_mine_room(int rroom_id)
{
	if (IS_ROOM(rroom_id, ROOM_PRIVATE) ||
	    PRIVATE_ZONE(rroom_id)
	    //|| IS_ROOM(rroom_id, ROOM_NO_TELEPORT)
	    || world[rroom_id].dir_option[DIR_DOWN] || IS_WATER_ROOM(rroom_id) || world[rroom_id].sector_type == SECT_MOUNTAIN || world[rroom_id].sector_type == SECT_ROAD ||
	    world[rroom_id].sector_type == SECT_UNDRWLD_MOUNTAIN || world[rroom_id].sector_type == SECT_UNDRWLD_NOGROUND || world[rroom_id].sector_type == SECT_UNDRWLD_NOSWIM ||
	    world[rroom_id].sector_type == SECT_UNDRWLD_WATER || world[rroom_id].sector_type == SECT_UNDRWLD_INSIDE || world[rroom_id].sector_type == SECT_UNDRWLD_CITY ||
	    world[rroom_id].sector_type == SECT_OCEAN || world[rroom_id].sector_type == SECT_INSIDE || world[rroom_id].sector_type == SECT_CASTLE || world[rroom_id].sector_type == SECT_CASTLE_WALL ||
	    world[rroom_id].sector_type == SECT_CASTLE_GATE)
		return TRUE;

	for (P_obj tobj = world[rroom_id].contents; tobj; tobj = tobj->next)
	{
		if (OBJ_VNUM(tobj) == VOBJ_MINE)
			return TRUE;
	}

	return FALSE;
}

bool load_one_mine(int map)
{
	P_obj mine = read_object(mine_data[map].type, VIRTUAL);

	if (!mine)
	{
		wizlog(56, "Error loading mine obj [%d]", VOBJ_MINE);
		return FALSE;
	}

	int start = real_room(mining_config_region_value(map, "start", mine_data[map].start));
	int end   = real_room(mining_config_region_value(map, "end", mine_data[map].end));

	int tries   = 0;
	int to_room = -1;

	do
	{
		to_room = number(start, end);

		/* if it's valid and a mine friendly location, or just lucky, put a mine there */
		if (!invalid_mine_room(to_room) && (mine_friendly(to_room) || number(0, 100) < 15))
			break;

		tries++;
	} while (tries < 10000);

	if (tries >= 10000 || invalid_mine_room(to_room))
	{
		extract_obj(mine);
		return FALSE;
	}

	int random = number(0, 99);

	mine->value[2] = mining_config_region_value(map, "duration", mine_data[map].mine_duration);
	if (mine_data[map].type == VOBJ_GEMMINE)
	{
		mine->value[0] = number(12, 23);
		mine->value[1] = 100 + number(0, 3);
		// Description already set in heavens.obj file.
	}
	else if (random < 3)
	{
		mine->value[0]    = number(24, 32);
		mine->value[1]    = 3;
		mine->description = str_dup("&+LThe &+yearth &+Lhere is &+cbr&+Lim&+Cm&+Ling with &+Yore&+L - it's the &+GMother &+LLode!&n");
	}
	else if (random < 20)
	{
		mine->value[0]    = number(16, 24);
		mine->value[1]    = 2;
		mine->description = str_dup("&+LThe &+yearth&+L here is &+cst&+Lrea&+ck&+Led &+Lwith &+core&+L.&n");
	}
	else if (random < 75)
	{
		mine->value[0]    = number(12, 20);
		mine->value[1]    = 1;
		mine->description = str_dup("&+LA few chunks of &+Yore &+Lpoke out of the ground here.&n");
	}
	else
	{
		mine->value[0]    = number(8, 16);
		mine->value[1]    = 0;
		mine->description = str_dup("&+LA few glimmers &+Ws&+wpa&+Wrk&+wle&+L in the &+yearth &+Lhere.&n");
	}

	obj_to_room(mine, to_room);
	wizlog(56, "Mine (%d) loaded in %s [%d]", mine->value[1], world[to_room].name, ROOM_VNUM(to_room));

	return TRUE;
}

void load_mines(bool set_event, bool load_all, int map)
{
	int                     max_mines, num_mines = 0, mine_type;
	struct mines_event_data mdata;

	mine_type = mine_data[map].type;
	for (P_obj tobj = object_list; tobj; tobj = tobj->next)
	{
		if ((OBJ_VNUM(tobj) == mine_type) && IS_SET(tobj->loc_p, LOC_ROOM) && (tobj->loc.room > 0) && (world[tobj->loc.room].number >= mining_config_region_value(map, "start", mine_data[map].start)) && (world[tobj->loc.room].number <= mining_config_region_value(map, "end", mine_data[map].end)))
		{
			num_mines++;
		}
	}
	max_mines = mining_config_region_value(map, "max", mines_properties(map));
	max_mines += number(-max_mines / 6, max_mines / 6);
	// debug("mines currently loaded: %d / %d", num_mines, max_mines );

	if (num_mines < max_mines)
	{
		if (load_all)
		{
			for (int i = 0; (i < (max_mines - num_mines)); i++)
			{
				load_one_mine(map);
			}
		}
		else
		{
			load_one_mine(map);
		}
	}

	mdata.map = map;

	if (set_event)
	{
		add_event(event_load_mines, (WAIT_SEC * 60) * ((int)get_property("mines.reloadMins", 10)), NULL, NULL, 0, 0, &mdata, sizeof(mdata));
	}
}

void event_load_mines(P_char ch, P_char victim, P_obj, void *data)
{
	struct mines_event_data *mdata = (struct mines_event_data *)data;

	if (!mdata)
	{
		debug("Passed null pointer to event_load_mines()");
		return;
	}

	load_mines(TRUE, FALSE, mdata->map);
}

void do_mine(P_char ch, char *arg, int cmd)
{

	if (!ch || IS_NPC(ch))
		return;

	// Anyone wana take a crack at this below to make it work correctly?
	// If you don't get the idea, give me a hollar.
	// From hearing Torgal's responses to it as well as knowing nobody ever uses
	// this command, i'm going to go ahead and get the engine in game. -Venthix
	if (GET_CHAR_SKILL(ch, SKILL_MINE) <= 1)
	{
		send_to_char("&+LYou dont know how to mine.\r\n", ch);
		return;
	}

	int  i;
	char buff[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	half_chop(arg, arg1, arg2);
	// one_argument(arg, buff);

	// debug("(arg) %s, (arg1) %s, (arg2) %s", arg, arg1, arg2);

	if (!strcmp(arg1, "reset") && IS_TRUSTED(ch))
	{
		for (i = 0; mine_data[i].start; i++)
		{
			if (isname(arg2, mine_data[i].abbrev))
			{
				snprintf(buf2, MAX_STRING_LENGTH, "purge %s", mine_data[i].abbrev);
				do_mine(ch, buf2, CMD_MINE);
				wizlog(56, "%s loaded mines in %s", GET_NAME(ch), mine_data[i].name);
				logit(LOG_WIZ, "%s loaded mines in %s", GET_NAME(ch), mine_data[i].name);
				load_mines(FALSE, TRUE, i);
				return;
			}
		}
		snprintf(buf2, MAX_STRING_LENGTH, "Available options for mine reset: map | ud\n");
		/*
		  for (i = 0; mine_data[i].start; i++);
		  {
		  strcat(buf2, (mine_data[i].abbrev));
		  if (mine_data[i+1].abbrev)
		    strcat(buf2, " | ");
		}
		strcat(buf2, "\n");
		*/
		send_to_char(buf2, ch);
	}
	else if (!strcmp(arg1, "load") && IS_TRUSTED(ch))
	{
		for (i = 0; mine_data[i].start; i++)
		{
			if (!strcmp(arg, mine_data[i].abbrev))
			{
				wizlog(56, "%s loaded mine in %s", GET_NAME(ch), mine_data[i].name);
				logit(LOG_WIZ, "%s loaded mine in %s", GET_NAME(ch), mine_data[i].name);
				load_one_mine(i);
				return;
			}
		}
		snprintf(buf2, MAX_STRING_LENGTH, "Available options for mine load: map | ud\n");
		/*for (i = 0; mine_data[i].abbrev; i++);
		{
		  debug("%s", mine_data[i].abbrev);
		  strcat(buf2,  mine_data[i].abbrev);
		  if (mine_data[i+1].abbrev)
		    strcat(buf2, " | ");
		}
		strcat(buf2, "\n");
		*/
		send_to_char(buf2, ch);
	}
	else if (!strcmp(buff, "purge") && IS_TRUSTED(ch))
	{
		P_obj tobj = object_list;
		P_obj next = object_list->next;

		for (i = 0; mine_data[i].start; i++)
		{
			if (!strcmp(arg, mine_data[i].abbrev))
				break;
		}

		for (; tobj && next; tobj = next)
		{
			next = tobj->next;

			if ((OBJ_VNUM(tobj) == VOBJ_MINE) && (!strcmp(arg, mine_data[i].abbrev)) && (world[tobj->loc.room].number >= mine_data[i].start) && (world[tobj->loc.room].number <= mine_data[i].end))
			{
				extract_obj(tobj, TRUE); // Not an arti, but 'in game.'
			}
			// The all factor
			else if (OBJ_VNUM(tobj) == VOBJ_MINE && (!strcmp(arg, "all")))
			{
				extract_obj(tobj, TRUE);
			}
		}
		if (!strcmp(arg, "all"))
		{
			wizlog(56, "%s purged all mines.", GET_NAME(ch));
			logit(LOG_WIZ, "%s purged all mines.", GET_NAME(ch));
			return;
		}
		else if (!strcmp(arg, mine_data[i].abbrev))
		{
			wizlog(56, "%s purged %s mines.", GET_NAME(ch), mine_data[i].name);
			logit(LOG_WIZ, "%s purged %s mines.", GET_NAME(ch), mine_data[i].name);
			return;
		}
		else
		{
			snprintf(buf2, MAX_STRING_LENGTH, "Available options for mine purge: all | map | tharnrift\n");
			/*
			for (i = 0; mine_data[i].start; i++);
			{
			  strcat(buf2, mine_data[i].abbrev);
			  if (mine_data[i+1].abbrev)
			    strcat(buf2, " | ");
			}
			strcat(buf2, " | all\n");
			*/
			send_to_char(buf2, ch);
		}
	}
	send_to_char("You can't mine here!\n", ch);
}
