#include "db.h"
#include "interp.h"
#include "utility.h"
#include "testcmd.h"
#include <cstring>
#include <fstream>
#include <math.h>
#include "epic.h"
#include "graph.h"
#include "map.h"
#include "objmisc.h"
#include "ships.h"
#include "spells.h"
#include "sql.h"
using namespace std;

extern struct zone_data        *zone_table;
extern int                      top_of_zone_table;
extern struct room_data        *world;
extern const int                top_of_world;
extern P_index                  obj_index;
extern int                      top_of_objt;
extern const flagDef            room_bits[];
extern const char              *sector_types[];
extern const AnsiString		sector_symbol[];
extern P_index                  mob_index;
extern int                      top_of_mobt;
extern P_desc                   descriptor_list;
extern int                      count_classes(P_char mob);
extern long                     new_exp_table[];
extern const char              *spells[];
extern float                    exp_mods[EXPMOD_MAX + 1];
extern const struct class_names class_names_table[];
extern struct zone_random_data
{
	int zone;
	int races[10];
	int proc_spells[3][2];
} zones_random_data[];
extern const racewar_struct racewar_color[MAX_RACEWAR + 2];
extern P_char               character_list;

extern int get_mincircle(int spell);

void display_map(P_char ch, int n, int show_map_regardless);
void do_test_radiate(P_char ch, char *arg, int cmd);

void disproom(P_char ch, int x, int y)
{
	int               local_y, local_x;
	int               vroom           = world[ch->in_room].number;
	struct zone_data *zone            = &zone_table[world[ch->in_room].zone];
	int               zone_start_vnum = world[zone->real_bottom].number;

	if (zone->mapx == 0 || zone->mapy == 0)
	{
		send_to_char("We have a serious problem: This room has a 0 mapx or 0 mapy.", ch);
		debug("disproom: we have a serious problem with this room r:%d v:%d - zone->mapx = %d, zone->mapy = %d.", ch->in_room, vroom, zone->mapx, zone->mapy);
		return;
	}

	// how far are we from the northern local map edge
	local_y = ((vroom - zone_start_vnum) / zone->mapx) % zone->mapy;

	// how far are we from the western local map edge
	local_x = (vroom - zone_start_vnum) % zone->mapy;

	if (local_x + x < 0)
	{
		local_x += zone->mapx;
	}
	else if (local_x + x >= zone->mapx)
	{
		local_x -= zone->mapx;
	}

	if (local_y + y < 0)
	{
		local_y += zone->mapy;
	}
	else if (local_y + y >= zone->mapy)
	{
		local_y -= zone->mapy;
	}

	int newx = local_x + x;
	int newy = local_y + y;

	char buff[100];
	snprintf(buff, 100, "(%3d,%3d) : [%6d] <%3d,%3d>\n", x, y, (zone_start_vnum + newx + (newy * zone->mapx)), newx, newy);
	send_to_char(buff, ch);
}

void display_exp_table(P_char ch, char *arg, int cmd)
{
	for (int i = 0; i < TOTALLVLS; i++)
	{
		char buff[128];
		snprintf(buff, 128, "%d: %ld\n", i, new_exp_table[i]);
		send_to_char(buff, ch);
	}
}

// Checks for all lava rooms in world.
#define START_ROOM 0
void do_test_lava(P_char ch, char *arg, int cmd)
{
	int    realRoomNum = START_ROOM, count = 0;
	char   buf[MAX_STRING_LENGTH];
	char   buf2[MAX_STRING_LENGTH];
	P_room rm;

	while (realRoomNum < top_of_world)
	{
		if (world[realRoomNum].sector_type == SECT_LAVA)
		{
			count++;
			rm = &world[realRoomNum];

			sprintbitde(rm->room_flags, room_bits, buf2);

			snprintf(buf,
			         MAX_STRING_LENGTH,
			         "&+YRoom: [&N%d&+Y](&N%d&+Y)  Zone: &N%d&+Y  Sector type: &N%s\n"
			         "&+YName: &N%s\n&+YRoom flags:&N %s\n\n",
			         rm->number,
			         realRoomNum,
			         zone_table[rm->zone].number,
			         sector_types[rm->sector_type],
			         rm->name,
			         buf2);

			send_to_char(buf, ch);
		}
		realRoomNum++;
	}
	--realRoomNum;
	snprintf(buf, MAX_STRING_LENGTH, "From room %d (%d) to %d (%d), found %d Lava rooms.\n\r", START_ROOM, world[START_ROOM].number, realRoomNum, world[realRoomNum].number, count);
	send_to_char(buf, ch);
}

void do_test_room(P_char ch, char *arg, int cmd)
{
	int               x = 0, y = 0;
	int               local_y, local_x;
	int               vroom = world[ch->in_room].number;
	struct zone_data *zone  = &zone_table[world[ch->in_room].zone];

	if (!IS_SET(zone->flags, ZONE_MAP))
	{
		send_to_char("This room not on a map.", ch);
		return;
	}

	int zone_start_vnum = world[zone->real_bottom].number;

	if (zone->mapx == 0 || zone->mapy == 0)
	{
		send_to_char("We have a serious problem: This map room has a 0 mapx or 0 mapy.", ch);
		debug("do_test_room: We have a serious problem with this map room r:%d v:%d - zone->mapx = %d, zone->mapy = %d.", ch->in_room, vroom, zone->mapx, zone->mapy);
		return;
	}

	// how far are we from the northern local map edge
	local_y = ((vroom - zone_start_vnum) / zone->mapx) % zone->mapy;

	// how far are we from the western local map edge
	local_x = (vroom - zone_start_vnum) % zone->mapy;

	char buff[100];
	snprintf(buff, 100, "&+CZone:&n (%dx%d) [%d,%d] '%s'&n %s\n", zone->mapx, zone->mapy, local_x, local_y, zone->name, zone->filename);
	send_to_char(buff, ch);

	send_to_char("&+CCalculated vnums:&n\n", ch);

	// Yeah, put in cyan and underline manually.  So sue me.
	send_to_char("\033[4;36mThis room: &n\033[4m", ch);
	disproom(ch, 0, 0);
	send_to_char("&+cOne north: &n", ch);
	disproom(ch, 0, -1);
	send_to_char("&+cOne east : &n", ch);
	disproom(ch, 1, 0);
	send_to_char("&+cOne south: &n", ch);
	disproom(ch, 0, 1);
	send_to_char("&+cOne west : &n", ch);
	disproom(ch, -1, 0);
}

void do_test_writemap(P_char ch, char *arg, int cmd)
{
	char buff[MAX_STRING_LENGTH];
	buff[0] = '\0';

	struct zone_data *zone = &zone_table[world[ch->in_room].zone];

	if (!IS_SET(zone->flags, ZONE_MAP))
		send_to_char("This room not on a map.", ch);

	string filename = zone->filename;
	filename += ".map";

	ofstream output_file(filename.c_str());

	int rroom = zone->real_bottom;

	int  what  = 0;
	bool hadbg = false;
	int  prev  = -1;
	int  where_rnum;

	int width  = zone->mapx;
	int height = zone->mapy;

	for (int y = 0; y < height; y++)
	{
		buff[0];
		AnsiString line;
		for (int x = 0; x < width; x++)
		{
			where_rnum = calculate_relative_room(rroom, x, y);

			if (!where_rnum)
				what = 21; // rock
			else
				what = world[where_rnum].sector_type;
			line.push_back(sector_symbol[what][0]);
		}
		line.push_back('\n');
		line.ansi(buff);
		output_file << buff;
	}
}

void do_test_zonepvp(P_char ch, char *arg)
{
	P_char  vict;
	u_short reset_players[MAX_RACEWAR + 1];
	int     zone;

	// Initialize.
	for (int i = 0; i < MAX_RACEWAR + 1; i++)
	{
		reset_players[i] = 0;
	}

	zone = world[ch->in_room].zone;

	for (vict = character_list; vict; vict = vict->next)
	{
		if (IS_NPC(vict))
			continue;
		if (vict->in_room == NOWHERE)
			continue;
		if (world[vict->in_room].zone != zone)
			continue;
		// Immortals do not affect misfire regardless of IS_TRUSTED toggle.
		if (GET_LEVEL(vict) < MINLVLIMMORTAL)
		{
			reset_players[GET_RACEWAR(vict)]++;
		}
	}

	for (int i = 0; i < MAX_RACEWAR + 1; i++)
	{
		if (zone_table[zone].players[i] != reset_players[i])
		{
			debug("do_test_zonepvp: Zone: %s (%d) - changing players[%s] from %d to %d.", zone_table[zone].name, zone, racewar_color[i].name, zone_table[zone].players[i], reset_players[i]);
			zone_table[zone].players[i] = reset_players[i];
		}
		zone_table[zone].misfiring[i] = FALSE;
	}
}

void do_test_suffix(P_char ch, char *arg)
{
	char orig[MAX_STRING_LENGTH], suffix[MAX_STRING_LENGTH];

	arg = one_argument(arg, orig);
	arg = one_argument(arg, suffix);

	send_to_char_f(ch, "string: '%s', suffix: '%s', suffix: %s.\n", orig, suffix, YESNO(ends_with(orig, suffix)));
}

void do_test_no_track(P_char ch, char *arg)
{
	int rroom = -1, count = 0;

	send_to_char("&=LWNo Track Rooms:&N\n", ch);
	while (++rroom < top_of_world)
	{
		if (IS_ROOM(rroom, ROOM_NO_TRACK))
		{
			send_to_char_f(ch, "Room: %3d) %6d '%s'.\n", ++count, world[rroom].number, world[rroom].name);
		}
	}
}

/*
void do_test_add_epic_skillpoint(P_char ch, const char *charname)
{
  P_char victim = get_char_room_vis(ch, charname);

  if( !victim )
  {
    send_to_char("No player by that name here.\n", ch);
    return;
  }

  epic_gain_skillpoints(victim, 1);
  send_to_char("&+WYou add an epic skill point.\n", ch);
  send_to_char("&+WYou have gained an epic skill point!\n", victim);

}
*/
extern Skill skills[];

void do_test(P_char ch, char *arg, int cmd)
{
	char buff[MAX_STRING_LENGTH];

	if (!IS_TRUSTED(ch))
	{
		return;
	}

	snprintf(buff, MAX_STRING_LENGTH, "%s in [%d]: test %s", GET_NAME(ch), world[ch->in_room].number, arg);

	wizlog(56, buff);
	logit(LOG_WIZ, buff);

	arg = one_argument(arg, buff);

	if (isname("oceanpvp", buff))
	{
		send_to_char_f(ch, "Ocean PvP state: %s.\n", YESNO(ocean_pvp_state()));
	}
	if (isname("weapondice", buff))
	{
		P_obj obj;
		int   count = 0;

		for (int r_num = 0; r_num <= top_of_objt; r_num++)
		{
			// Load a copy of object.
			obj = read_object(r_num, REAL);
			if ((obj->type == ITEM_WEAPON) && ((obj->value[1] < 1) || (obj->value[2] < 1)))
			{
				send_to_char_f(ch, "%d) %s %d - %dd%d.\n", ++count, OBJ_SHORT(obj), OBJ_VNUM(obj), obj->value[1], obj->value[2]);
			}
			extract_obj(obj);
		}
		if (count == 0)
		{
			send_to_char_f(ch, "All weapons have good dice.\n");
		}
		return;
	}
	if (isname("no_track", buff))
	{
		do_test_no_track(ch, arg);
		return;
	}
	if (isname("suffix", buff))
	{
		do_test_suffix(ch, arg);
		return;
	}
	if (isname("zonepvp", buff))
	{
		do_test_zonepvp(ch, arg);
		return;
	}
	if (isname("missile", buff))
	{
		int   count, rnum, type, avgdam, maxdam;
		P_obj obj;

		type = 0;
		arg  = skip_spaces(arg);
		if (!strcmp(arg, "arrow"))
		{
			type = MISSILE_ARROW;
		}
		else if (!strcmp(arg, "light"))
		{
			type = MISSILE_LIGHT_CBOW_QUARREL;
		}
		else if (!strcmp(arg, "heavy"))
		{
			type = MISSILE_HEAVY_CBOW_QUARREL;
		}
		else if (!strcmp(arg, "hand"))
		{
			type = MISSILE_HAND_CBOW_QUARREL;
		}
		else if (!strcmp(arg, "bullet"))
		{
			type = MISSILE_SLING_BULLET;
		}
		else if (!strcmp(arg, "dart"))
		{
			type = MISSILE_DART;
		}
		send_to_char_f(ch, "##)   VNUM 'object name                        '  dice  avg max    sugg actual\n");
		send_to_char_f(ch, "          *=ignore                                      dam dam   value value\n");
		for (rnum = count = 0; rnum <= top_of_objt; rnum++)
		{
			obj = read_object(rnum, REAL);
			if (obj->type == ITEM_MISSILE && (type == obj->value[3] || type == 0))
			{
				// avgdam is actually 10 times the real avg dam to keep the .5's.
				avgdam = (obj->value[1] * (obj->value[2] + 1)) * 5;
				maxdam = obj->value[1] * obj->value[2];

				send_to_char_f(ch,
				               "%2d) %6d%c'%s&n' %s%2dd%-2d&n %2d.%d  %2d %7d %d.\n",
				               ++count,
				               OBJ_VNUM(obj),
				               IS_SET(obj->extra_flags, ITEM_IGNORE) ? '*' : ' ',
				               pad_ansi(OBJ_SHORT(obj), 35, TRUE).c_str(),
				               (obj->value[1] > 3)    ? "&+R"
				               : (obj->value[1]) == 3 ? "&+Y"
				               : (obj->value[1]) == 2 ? "&+B"
				                                      : "",
				               obj->value[1],
				               obj->value[2],
				               avgdam / 10,
				               avgdam % 10,
				               maxdam,
				               avgdam * avgdam * maxdam * maxdam * maxdam / 125,
				               obj->cost);
			}
			extract_obj(obj);
		}
		return;
	}
	if (isname("stormdruid", buff))
	{
		P_char mob;
		int    R_num, count;

		send_to_char("&=LgList of Storm Druids:&N\n", ch);
		for (R_num = count = 0; R_num <= top_of_mobt; R_num++)
		{
			mob = read_mobile(R_num, REAL);
			if (GET_SPEC(mob, CLASS_DRUID, SPEC_STORM))
				send_to_char_f(ch, "%2d) %6d %s\n", ++count, GET_VNUM(mob), mob->player.short_descr);
			extract_char(mob);
		}
		return;
	}
	else if (isname("mapzones", buff))
	{
		send_to_char("&=LCNum) Zone Name                           (Zon#)&n\n", ch);
		for (int i = 0, count = 0; i <= top_of_zone_table; i++)
		{
			if (IS_SET(zone_table[i].flags, ZONE_MAP))
			{
				snprintf(buff, MAX_STRING_LENGTH, "%3d) %s&n (%d): IS a map zone.\n", ++count, pad_ansi(zone_table[i].name, 35, TRUE).c_str(), zone_table[i].number);
				send_to_char(buff, ch);
			}
		}
		return;
	}
	if (isname("exp_mods", buff))
	{
		// Last updated 10/31/2015
		char *exp_mod_names[EXPMOD_MAX + 1] = {"NONE",
		                                       "CLS_WARRIOR",
		                                       "CLS_RANGER",
		                                       "CLS_PSIONICIST",
		                                       "CLS_PALADIN",
		                                       "CLS_ANTIPALADIN",
		                                       "CLS_CLERIC",
		                                       "CLS_MONK",
		                                       "CLS_DRUID",
		                                       "CLS_SHAMAN",
		                                       "CLS_SORCERER",
		                                       "CLS_NECROMANCER",
		                                       "CLS_CONJURER",
		                                       "CLS_ROGUE",
		                                       "CLS_ASSASSIN",
		                                       "CLS_MERCENARY",
		                                       "CLS_BARD",
		                                       "CLS_THIEF",
		                                       "CLS_WARLOCK",
		                                       "CLS_MINDFLAYER",
		                                       "CLS_ALCHEMIST",
		                                       "CLS_BERSERKER",
		                                       "CLS_REAVER",
		                                       "CLS_ILLUSIONIST",
		                                       "CLS_BLIGHTER",
		                                       "CLS_DREADLORD",
		                                       "CLS_ETHERMANCER",
		                                       "CLS_AVENGER",
		                                       "CLS_THEURGIST",
		                                       "CLS_SUMMONER",
		                                       "CLS_NEWCLASS1",
		                                       "CLS_NEWCLASS2",
		                                       "CLS_NEWCLASS3",
		                                       "LVL_31_UP",
		                                       "LVL_41_UP",
		                                       "LVL_51_UP",
		                                       "LVL_55_UP",
		                                       "RES_EVIL",
		                                       "RES_NORMAL",
		                                       "VICT_BREATHES",
		                                       "VICT_ACT_AGGRO",
		                                       "VICT_ACT_HUNTER",
		                                       "VICT_ELITE",
		                                       "VICT_HOMETOWN",
		                                       "VICT_NOMEMORY",
		                                       "PVP",
		                                       "GLOBAL",
		                                       "GOOD",
		                                       "EVIL",
		                                       "UNDEAD",
		                                       "NEUTRAL",
		                                       "DAMAGE",
		                                       "HEAL_NONHEALER",
		                                       "HEAL_PETS",
		                                       "HEALING",
		                                       "MELEE",
		                                       "TANK",
		                                       "KILL",
		                                       "PALADIN_VS_GOOD",
		                                       "PALADIN_VS_EVIL",
		                                       "ANTIPALADIN_VS_GOOD"};

		for (int i = 0; i <= EXPMOD_MAX; i++)
		{
			snprintf(buff, MAX_STRING_LENGTH, "%3d) %20s %2.3f %s\n", i, exp_mod_names[i], exp_mods[i], (i <= CLASS_COUNT) ? class_names_table[i].ansi : "");
			send_to_char(buff, ch);
		}
		snprintf(buff, MAX_STRING_LENGTH, "WARRIOR: %d %d, CLERIC: %d %d.\n", flag2idx(CLASS_WARRIOR), EXPMOD_CLS_WARRIOR, flag2idx(CLASS_CLERIC), EXPMOD_CLS_CLERIC);
		send_to_char(buff, ch);
		return;
	}
	if (isname("shield", buff))
	{
		int   rnum, count, count2, afnum, currac, maxac, minac;
		long  sum;
		P_obj obj;

		for (sum = 0, rnum = count = 0, maxac = MIN_INT_SIGNED, minac = MAX_INT_SIGNED; rnum <= top_of_objt; rnum++)
		{
			obj = read_object(rnum, REAL);
			if (obj->type == ITEM_SHIELD)
			{
				count++;
				// This is pulled from affects.c - apply_ac
				switch (obj->material)
				{
					case MAT_UNDEFINED:
					case MAT_NONSUBSTANTIAL:
						currac = 0;
						break;
					case MAT_FLESH:
					case MAT_REEDS:
					case MAT_HEMP:
					case MAT_LIQUID:
					case MAT_CLOTH:
					case MAT_PAPER:
					case MAT_PARCHMENT:
					case MAT_LEAVES:
					case MAT_GENERICFOOD:
					case MAT_RUBBER:
					case MAT_FEATHER:
					case MAT_WAX:
						currac = 15;
						break;
					case MAT_BARK:
					case MAT_SOFTWOOD:
					case MAT_SILICON:
					case MAT_CERAMIC:
					case MAT_PEARL:
					case MAT_EGGSHELL:
						currac = 30;
						break;
					case MAT_HIDE:
					case MAT_LEATHER:
					case MAT_CURED_LEATHER:
					case MAT_LIMESTONE:
						currac = 45;
						break;
					case MAT_IVORY:
					case MAT_BAMBOO:
					case MAT_HARDWOOD:
					case MAT_COPPER:
					case MAT_BONE:
					case MAT_MARBLE:
						currac = 60;
						break;
					case MAT_STONE:
					case MAT_SILVER:
					case MAT_BRONZE:
					case MAT_IRON:
					case MAT_REPTILESCALE:
						currac = 75;
						break;
					case MAT_GOLD:
					case MAT_CHITINOUS:
					case MAT_CRYSTAL:
					case MAT_STEEL:
					case MAT_BRASS:
					case MAT_OBSIDIAN:
					case MAT_GRANITE:
					case MAT_GEM:
						currac = 90;
						break;
					case MAT_ELECTRUM:
					case MAT_PLATINUM:
					case MAT_RUBY:
					case MAT_EMERALD:
					case MAT_SAPPHIRE:
					case MAT_GLASSTEEL:
						currac = 105;
						break;
					case MAT_DRAGONSCALE:
					case MAT_DIAMOND:
						currac = 120;
						break;
					case MAT_MITHRIL:
					case MAT_ADAMANTIUM:
						currac = 135;
						break;
					default:
						currac = 0;
				}
				if (obj->value[3] < 0)
				{
					send_to_char_f(ch, "Shield '%s' %d needs a fix to make val3 positive.\n", OBJ_SHORT(obj), OBJ_VNUM(obj));
					currac = MAX(currac, -obj->value[3]);
				}
				else
				{
					currac = MAX(currac, obj->value[3]);
				}
				for (afnum = 0; afnum < MAX_OBJ_AFFECT; afnum++)
				{
					if (obj->affected[afnum].location == APPLY_AC)
					{
						if (obj->affected[afnum].modifier > 0)
						{
							send_to_char_f(ch, "Shield '%s' %d needs a fix to make a%dmod negative.\n", OBJ_SHORT(obj), OBJ_VNUM(obj), afnum);
							currac += obj->affected[afnum].modifier;
						}
						currac -= obj->affected[afnum].modifier;
					}
				}
				if (currac <= 0)
				{
					send_to_char_f(ch, "Shield '%s' %d needs a fix, since it has %d total ac.\n", OBJ_SHORT(obj), OBJ_VNUM(obj), currac);
				}
				sum += currac;
				if (currac > maxac)
				{
					maxac = currac;
				}
				if (currac < minac)
				{
					minac = currac;
				}
			}
			extract_obj(obj);
		}
		send_to_char_f(ch, "\nThe total number of shields: %d, sum of ac: %ld, average ac: %ld, max ac: %d, min ac: %d.\n", count, sum, sum / count, maxac, minac);
		send_to_char_f(ch, "Shields with max %d ac:\n", maxac);
		buff[0] = '\0';
		for (rnum = count = count2 = 0; rnum <= top_of_objt; rnum++)
		{
			obj = read_object(rnum, REAL);
			if (obj->type == ITEM_SHIELD)
			{
				switch (obj->material)
				{
					case MAT_UNDEFINED:
					case MAT_NONSUBSTANTIAL:
						currac = 0;
						break;
					case MAT_FLESH:
					case MAT_REEDS:
					case MAT_HEMP:
					case MAT_LIQUID:
					case MAT_CLOTH:
					case MAT_PAPER:
					case MAT_PARCHMENT:
					case MAT_LEAVES:
					case MAT_GENERICFOOD:
					case MAT_RUBBER:
					case MAT_FEATHER:
					case MAT_WAX:
						currac = 15;
						break;
					case MAT_BARK:
					case MAT_SOFTWOOD:
					case MAT_SILICON:
					case MAT_CERAMIC:
					case MAT_PEARL:
					case MAT_EGGSHELL:
						currac = 30;
						break;
					case MAT_HIDE:
					case MAT_LEATHER:
					case MAT_CURED_LEATHER:
					case MAT_LIMESTONE:
						currac = 45;
						break;
					case MAT_IVORY:
					case MAT_BAMBOO:
					case MAT_HARDWOOD:
					case MAT_COPPER:
					case MAT_BONE:
					case MAT_MARBLE:
						currac = 60;
						break;
					case MAT_STONE:
					case MAT_SILVER:
					case MAT_BRONZE:
					case MAT_IRON:
					case MAT_REPTILESCALE:
						currac = 75;
						break;
					case MAT_GOLD:
					case MAT_CHITINOUS:
					case MAT_CRYSTAL:
					case MAT_STEEL:
					case MAT_BRASS:
					case MAT_OBSIDIAN:
					case MAT_GRANITE:
					case MAT_GEM:
						currac = 90;
						break;
					case MAT_ELECTRUM:
					case MAT_PLATINUM:
					case MAT_RUBY:
					case MAT_EMERALD:
					case MAT_SAPPHIRE:
					case MAT_GLASSTEEL:
						currac = 105;
						break;
					case MAT_DRAGONSCALE:
					case MAT_DIAMOND:
						currac = 120;
						break;
					case MAT_MITHRIL:
					case MAT_ADAMANTIUM:
						currac = 135;
						break;
					default:
						currac = 0;
				}
				if (obj->value[3] < 0)
				{
					currac = MAX(currac, -obj->value[3]);
				}
				else
				{
					currac = MAX(currac, obj->value[3]);
				}
				for (afnum = 0; afnum < MAX_OBJ_AFFECT; afnum++)
				{
					if (obj->affected[afnum].location == APPLY_AC)
					{
						if (obj->affected[afnum].modifier > 0)
						{
							currac += obj->affected[afnum].modifier;
						}
						currac -= obj->affected[afnum].modifier;
					}
				}
				if (currac == maxac)
				{
					send_to_char_f(ch, "%2d) '%s' %6d.\n", ++count, pad_ansi(OBJ_SHORT(obj), 35, TRUE).c_str(), OBJ_VNUM(obj));
				}
				// Sneak in and keep a record of minac shields too.
				if (currac == minac)
				{
					snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff), "%2d) '%s' %6d.\n", ++count2, pad_ansi(OBJ_SHORT(obj), 35, TRUE).c_str(), OBJ_VNUM(obj));
				}
			}
			extract_obj(obj);
		}
		send_to_char_f(ch, "Shields with min %d ac:\n", minac);
		send_to_char(buff, ch);
		return;
	}
	if (isname("passwd", buff))
	{
		char buf1[MAX_STRING_LENGTH];
		char buf2[MAX_STRING_LENGTH];

		arg = skip_spaces(arg);
		snprintf(buf1, MAX_STRING_LENGTH, "%s", CRYPT(arg, GET_NAME(ch)));
		snprintf(buf2, MAX_STRING_LENGTH, "%s", CRYPT2(arg, GET_NAME(ch)));
		snprintf(buff, MAX_STRING_LENGTH, "%s\n\rcrypt1: %s, crypt1(crypt1): %s.\n\rcrypt2: %s, crypt2(crypt2): %s.\n\r", arg, buf1, CRYPT(arg, buf1), buf2, CRYPT2(arg, buf2));
		send_to_char(buff, ch);
		snprintf(buf1, MAX_STRING_LENGTH, "%s", CRYPT2(arg, buf2));
		if (!strcmp(buf1, buf2))
			send_to_char("They are the same.\n\r", ch);
		else
			send_to_char("They are not the same.\n\r", ch);
		send_to_char("Your password is '", ch);
		send_to_char(ch->only.pc->pwd, ch);
		send_to_char("'.\n\r", ch);
		return;
	}
	if (isname("room", buff))
	{
		do_test_room(ch, arg, cmd);
		return;
	}
	if (isname("radiate", buff))
	{
		do_test_radiate(ch, arg, cmd);
		return;
	}
	if (isname("lava", buff))
	{
		do_test_lava(ch, arg, cmd);
		return;
	}
	else if (isname("exp", buff))
	{
		display_exp_table(ch, arg, cmd);
		return;
	}
	else if (isname("map", buff))
	{
		one_argument(arg, buff);
		display_map(ch, atoi(buff), TRUE);
		return;
	}
	else if (isname("writemap", buff))
	{
		do_test_writemap(ch, arg, cmd);
		return;
	}
	else if (isname("arti", buff))
	{
		P_obj arti;
		int   feed_min;

		arg = one_argument(arg, buff);
		if ((arti = get_obj_in_list(buff, ch->carrying)) != NULL)
		{
			one_argument(arg, buff);
			if ((feed_min = atoi(buff)) > 0)
			{
				artifact_feed_to_min_sql(arti, feed_min);
			}
			else
			{
				send_to_char("Please enter a valid (positive) minimum feed minutes for arti.\n\r", ch);
			}
		}
		else
		{
			send_to_char("Couldn't find object to test.\n\r", ch);
		}
		return;
	}
	else if (isname("mem", buff))
	{
		int spell = memorize_last_spell(ch);

		if (spell)
		{
			char buf[256];
			snprintf(buf, 256, "&+Cpower of %s\n", skills[spell].name);
			send_to_char(buf, ch);
		}
		return;
	}
	else if (isname("skills", buff))
	{
		for (int i = 0; i < MAX_SKILLS; i++)
		{
			debug("[%d] %s", GET_CHAR_SKILL(ch, i), skills[i].name);
		}
	}
	else if (isname("loadallchars", buff))
	{
		test_load_all_chars(ch);
		return;
	}
	else if (isname("count", buff))
	{
		char   buf[MAX_STRING_LENGTH];
		int    classes;
		P_char mob;
		P_obj  obj;

		generic_find(arg, FIND_CHAR_WORLD, ch, &mob, &obj);
		if (mob)
		{
			classes = count_classes(mob);
			snprintf(buf, MAX_STRING_LENGTH, "%s (%d) has %d classes.\n", J_NAME(mob), IS_PC(mob) ? -1 : GET_VNUM(mob), classes);
			send_to_char(buf, ch);
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH, "Char '%s' not found.\n", skip_spaces(arg));
			send_to_char(buf, ch);
		}
		return;
	}
	else if (isname("sql_log", buff))
	{
		P_char mob;
		// Shadow
		mob = read_mobile(92076, VIRTUAL);
		sql_log(mob, WIZLOG, "SQL: '%s'", arg);
		extract_char(mob);
	}
	else if (isname("xlogx", buff))
	{
		int  num = atoi(arg);
		char buf[MAX_STRING_LENGTH];

		if (num > 0)
		{
			for (int i = 1; i <= num; i++)
			{
				snprintf(buf, MAX_STRING_LENGTH, "%d log %d: %f == %d.\n\r", i, i, i * log(i), (int)(i * log(i)));
				send_to_char(buf, ch);
			}
		}
		else
		{
			send_to_char("This is for testing x*log(x) function used in itemvalue.\n\rTakes a number > 0 as an argument.\n\r", ch);
		}
	}
	else if (isname("xlog2x", buff))
	{
		int  num = atoi(arg);
		char buf[MAX_STRING_LENGTH];

		if (num > 0)
		{
			for (int i = 1; i <= num; i++)
			{
				snprintf(buf, MAX_STRING_LENGTH, "%d log2 %d: %f == %d.\n\r", i, i, i * log2(i), (int)(i * log2(i)));
				send_to_char(buf, ch);
			}
		}
		else
		{
			send_to_char("This is for testing x*log2(x) function used in itemvalue.\n\rTakes a number > 0 as an argument.\n\r", ch);
		}
	}
	else if (isname("spell", buff))
	{
		int  num = atoi(arg);
		char buf[MAX_STRING_LENGTH];

		if (num < 1)
		{
			num = search_block(skip_spaces(arg), (const char **)spells, FALSE);
		}
		if (num < FIRST_SPELL || num > LAST_SPELL)
		{
			snprintf(buf, MAX_STRING_LENGTH, "Spell must be a number between %d and %d, or a valid spell name.\n\r", FIRST_SPELL, LAST_SPELL);
			send_to_char(buf, ch);
			return;
		}
		snprintf(buf, MAX_STRING_LENGTH, "The mincircle for spell '%s' (%d), is %d.\n\r", spells[num], num, get_mincircle(num));
		send_to_char(buf, ch);
	}
	else if (isname("randomize", buff))
	{
		P_obj obj;

		arg = one_argument(arg, buff);
		if ((obj = get_obj_in_list(buff, ch->carrying)) != NULL)
		{
			randomizeitem(ch, obj);
		}
		else
		{
			send_to_char("Could not find object '", ch);
			send_to_char(buff, ch);
			send_to_char("' in your inventory to randomize.\n\r", ch);
		}
	}
	else if (isname("randomzones", buff))
	{
		int   i, j;
		char  buf[MAX_STRING_LENGTH];
		FILE *pf;

		if (!(pf = fopen("logs/log/randomzones", "w")))
		{
			send_to_char("Couldn't open file. :(\n\r", ch);
			return;
		}
		fprintf(pf, "Printing list of zones with their spells:\n");
		debug("List of zones_random_data:");
		for (i = 0; i < 100; i++)
		{
			if (zones_random_data[i].zone == 0)
			{
				break;
			}
			debug("%2d) %4d = %4d rb: %7d zone: '%s'.",
			      i,
			      zones_random_data[i].zone,
			      zone_table[real_zone0(zones_random_data[i].zone)].number,
			      zone_table[real_zone0(zones_random_data[i].zone)].real_bottom,
			      zone_table[real_zone0(zones_random_data[i].zone)].name);

			fprintf(pf, "%2d) '%s'\n", i + 1, strip_ansi(zone_table[real_zone0(zones_random_data[i].zone)].name).c_str());
			for (j = 0; j < 3; j++)
			{
				if (zones_random_data[i].proc_spells[j][0] > 0)
				{
					fprintf(pf,
					        "  With %d pieces: Spell '%s'.\n",
					        zones_random_data[i].proc_spells[j][0],
					        zones_random_data[i].proc_spells[j][1] == SPELL_STONE_SKIN ? "stone skin"
					        : zones_random_data[i].proc_spells[j][1] == SPELL_BLESS    ? "bless"
					        : zones_random_data[i].proc_spells[j][1] == SPELL_STRENGTH ? "strength"
					        : zones_random_data[i].proc_spells[j][1] == SPELL_BARKSKIN ? "barkskin"
					        : zones_random_data[i].proc_spells[j][1] == SPELL_ARMOR    ? "armor"
					                                                                   : "unknown");
				}
			}
		}
		return;
	}
	else if (isname("clearepiczones", buff))
	{
		if (GET_LEVEL(ch) >= FORGER)
		{
			while (*arg == ' ')
			{
				arg++;
			}
			if (isname(arg, "confirm"))
			{
				sql_clear_zone_trophy();
			}
			else
			{
				send_to_char("You must '&=Blconfirm&n' this command in order to use it.\n\r", ch);
				send_to_char("This will &=Blpermenantly&n clear the zone alignment list; there is no going back!\n\r", ch);
			}
		}
		else
		{
			send_to_char("You must be a &+LFORGER&n or higher to execute this command.\n\r", ch);
		}
	}
	else if (isname("clearoutposts", buff))
	{
		if (GET_LEVEL(ch) >= FORGER)
		{
			while (*arg == ' ')
			{
				arg++;
			}
			if (isname(arg, "confirm"))
			{
				send_to_char("&=GLResetting oupost information... .. .", ch);
				if (qry("UPDATE outposts SET owner_id='0', level='8', walls='1', archers='0', hitpoints='300000', territory='0',"
				        " portal_room='0', resources='0', applied_resources='0', golems='0', meurtriere='0', scouts='0'"))
				{
					send_to_char("&=Bl success!&n\n\r", ch);
				}
				else
				{
					send_to_char("&=Rl failure!&n\n\r", ch);
				}
			}
			else
			{
				send_to_char("You must '&=Blconfirm&n' this command in order to use it.\n\r", ch);
				send_to_char("This will &=Blpermenantly&n reset the info on outposts, setting owners to none; there is no going back!\n\r", ch);
			}
		}
		else
		{
			send_to_char("You must be a &+LFORGER&n or higher to execute this command.\n\r", ch);
		}
	}
	else if (isname("angry", buff))
	{
		int maliciousPID;

		if (GET_LEVEL(ch) < OVERLORD)
		{
			send_to_char("Maybe when you're bigger.\n\r", ch);
			return;
		}

		while (*arg == ' ')
		{
			arg++;
		}

		maliciousPID = atoi(arg);
		debug("Searching for maliciousPID %d (%s)...", maliciousPID, arg);
		for (P_desc d = descriptor_list; d; d = d->next)
		{
			if (d->character && d->connected == CON_PLAYING && GET_PID(d->character) == maliciousPID)
			{
				debug("Found maliciousPID %d, calling very_angry_npc on '%s'...", GET_PID(d->character), J_NAME(d->character));
				very_angry_npc(ch, d->character, CMD_SHOUT, NULL);
				return;
			}
		}
		debug("Could not find maliciousPID %d in game. :(");
	}
	else
	{
		send_to_char("Invalid keyword.\r\n", ch);
		return;
	}
}

// Function to test the radiate message from room code.
void do_test_radiate(P_char ch, char *arg, int cmd)
{
	char  buff[MAX_STRING_LENGTH];
	char *message;
	int   num_rooms;

	arg = one_argument(arg, buff);

	if (!is_number(buff))
	{
		send_to_char("You dork.  You must specify a number of rooms to radiate as a first argument.\n\r"
		             "Then, supply the string you want to radiate.\n\r",
		             ch);
		return;
	}

	num_rooms = atoi(buff);
	message   = skip_spaces(arg);

	snprintf(buff, MAX_STRING_LENGTH, "%s in %s [%d] radiates \"%s\" %d rooms.", J_NAME(ch), world[ch->in_room].name, world[ch->in_room].number, message, num_rooms);
	wizlog(56, buff);
	logit(LOG_WIZ, buff);

	// Radiate from ch's room, the message, outward by num_rooms rooms, dracolich flags, and show 100% of the time.
	radiate_message_from_room(ch->in_room, message, num_rooms, (RMFR_FLAGS)(RMFR_RADIATE_ALL_DIRS | RMFR_PASS_WALL | RMFR_PASS_DOOR | RMFR_CROSS_ZONE_BARRIER), 100);
}
