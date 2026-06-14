/****************************************************************************
 *  File: enhance.c                                           Part of Duris
 *  Usage: Item enhancement system
 *  Extracted from drannak.c 2026-07-14
 * ***************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "mm.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "enhance.h"
#include "tradeskill.h"
#include "objmisc.h"

/* Forward declarations for hash functions used in enhance() and do_enhance() */
static int enhance_hash(int key);
static int enhance_stat_hash(int key);
static struct enhance_essence_zone_rule *enhance_find_essence_zone_rule(int zone_number);
extern double enhance_stat_cap_multiplier;
extern int enhance_stat_platinum_base;
extern int enhance_stat_platinum_per_ival;
extern int enhance_level_gate_multiplier;
extern int enhance_mod_max_steps;
extern double enhance_stat_material_quantity_multiplier;

/* NPC death essence-drop tuning defaults; config reload restores these before parsing. */
static int enhance_essence_drop_enabled           = 1;
static int enhance_essence_primary_roll_max       = 3000;
static int enhance_essence_max_roll_max           = 4000;
static int enhance_essence_elite_level_multiplier = 1;
/* Level gates: drops only fire when mob level is within [min, max]. Defaults
 * 1-100 effectively accept all eligible NPCs, preserving existing behavior. */
static int enhance_essence_minimum_level            = 1;
static int enhance_essence_maximum_level            = 1000000;

/* Sparse per-zone essence drop override rules.  Any numeric field left at 0
 * (or omitted) inherits the global [essence_drop] default at apply time. */
#define ENHANCE_ESSENCE_MAX_ZONE_RULES 256
struct enhance_essence_zone_rule {
	int zone_number;
	int primary_roll_max;       /* 0 → use global default */
	int max_roll_max;            /* 0 → use global default */
	int elite_level_multiplier;  /* 0 → use global default */
};
static struct enhance_essence_zone_rule enhance_essence_zone_rules[ENHANCE_ESSENCE_MAX_ZONE_RULES];
static int enhance_essence_zone_rule_count = 0;

/* World tables needed to map an object template vnum back to its origin zone. */
extern P_room world;
extern struct zone_data *zone_table;
extern int top_of_zone_table;

void enhance(P_char ch, P_obj source, P_obj material)
{
	char  buf[MAX_STRING_LENGTH];
	P_obj robj;
	int   cost, searchcount, maxsearch, tries, sval, level;
	bool  validobj;
	int   newval, minval, chluck, wearflags;
	int   cascade_dir, cascade_step, cascade_ival;
	struct enhance_index_entry *entry;

		if (!ch || !source || !material)
			return;

		if (is_enhance_banned(source))
		{
			act("&+RYour $p&+R has too many conflicting enchantments to be enhanced.&n", FALSE, ch, source, 0, TO_CHAR);
			return;
		}
		if (is_enhance_banned(material))
		{
			act("&+RYour $p&+R cannot be used as an enhancement material&n.", FALSE, ch, material, 0, TO_CHAR);
			return;
		}

		chluck      = (GET_C_LUK(ch));
	sval        = itemvalue(source);
	minval      = itemvalue(source) - enhance_material_ival_delta;
	searchcount = 0;
	maxsearch   = enhance_search_max_attempts;
	// Only search matching wear flags unless none matching, then just search source wear flags.
	//  We skip ITEM_TAKE 'cause it's not really a wear flag.  We skip ITEM_HOLD, ITEM_ATTACH_BELT, and
	//  ITEM_WEAR_BACK because these are too common and override what people really want (i.e. a quiver).
	wearflags = (source->wear_flags & material->wear_flags) & ~(ITEM_TAKE | ITEM_HOLD | ITEM_ATTACH_BELT | ITEM_WEAR_BACK);
	if (!wearflags)
		wearflags = (source->wear_flags) & ~(ITEM_TAKE | ITEM_HOLD | ITEM_ATTACH_BELT | ITEM_WEAR_BACK);

	if (!wearflags)
	{
		send_to_char("This item can not be enhanced.\n", ch);
		return;
	}

	// Can enhance up to 3x level, same as forge/craft. --Eikel
	if (sval > GET_LEVEL(ch) * enhance_level_gate_multiplier)
	{
		snprintf(buf, MAX_STRING_LENGTH, "This item has ival %d; at your level you can enhance items up to ival %d.\r\n",
		         sval, GET_LEVEL(ch) * enhance_level_gate_multiplier);
		send_to_char(buf, ch);
		return;
	}

	if (IS_SET(source->wear_flags, ITEM_GUILD_INSIGNIA))
		minval += enhance_guild_insignia_ival_bonus;

	if (itemvalue(material) < minval)
	{
		char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
		snprintf(buf2, MAX_STRING_LENGTH, "%s", source->short_description);
		snprintf(buf, MAX_STRING_LENGTH, "&+REnhancing %s requires an item with at least an &+Witem value of: %d&n\r\n", buf2, minval);
		send_to_char(buf, ch);
		return;
	}

	if (sval <= enhance_cost_low_ival_threshold)
	{
		cost = enhance_cost_low_amount;
	}
	else
	{
		cost = enhance_cost_high_amount;
	}

	if (GET_MONEY(ch) < cost)
	{
		snprintf(buf, MAX_STRING_LENGTH, "It will require &+W%d platinum&n to &+Benhance&n this item.\r\n", cost / 1000);
		send_to_char(buf, ch);
		return;
	}

	if (number(1, enhance_luck_extreme_range) < chluck)
	{
		newval = sval + enhance_ival_gain_extreme;
		maxsearch *= 4;
		send_to_char("&+YYou feel &+MEXTREMELY Lucky&+Y!\r\n", ch);
	}
	else if (number(1, enhance_luck_very_range) < chluck)
	{
		newval = sval + enhance_ival_gain_very;
		maxsearch *= 3;
		send_to_char("&+YYou feel &+MVery Lucky&+Y!\r\n", ch);
	}
	else if (number(1, enhance_luck_lucky_range) < chluck)
	{
		newval = sval + enhance_ival_gain_lucky;
		maxsearch *= 2;
		send_to_char("&+YYou feel &+MLucky&+Y!\r\n", ch);
	}
	else
	{
		newval = sval + enhance_ival_gain_normal;
	}

	/* Cascade search through the ival hash table.
	 * Try exact match first, then cascade in the configured direction.
	 * cascade_down_first=1: try lower ival values first, then higher
	 * cascade_down_first=0: try higher ival values first, then lower
	 */
	robj = NULL;
	for (cascade_step = 0; cascade_step <= enhance_original_max_roll; cascade_step++)
	{
		for (cascade_dir = 0; cascade_dir < 2; cascade_dir++)
		{
			if (cascade_step == 0)
			{
				/* Exact match — only one try */
				if (cascade_dir > 0)
					continue;
				cascade_ival = newval;
			}
			else if (enhance_original_cascade_down_first)
			{
				/* Down first: try -step, then +step */
				cascade_ival = (cascade_dir == 0) ? (newval - cascade_step) : (newval + cascade_step);
			}
			else
			{
				/* Up first: try +step, then -step */
				cascade_ival = (cascade_dir == 0) ? (newval + cascade_step) : (newval - cascade_step);
			}

			if (cascade_ival < 1 || cascade_ival > enhance_ival_cap + enhance_original_max_roll)
				continue;

			/* Look up in hash table */
			for (entry = enhance_ival_table[enhance_hash(cascade_ival)]; entry; entry = entry->next)
			{
				if (entry->ival != cascade_ival)
					continue;

				/* Check wear flags match */
				if (!(wearflags & entry->wear_flags))
					continue;

				/* Check not same vnum */
				if (entry->vnum == OBJ_VNUM(source))
					continue;

				/* Found a match — read the object */
				robj = read_object(entry->vnum, VIRTUAL);
				if (robj)
				{
					validobj = TRUE;
					break;
				}
			}

			if (robj)
				break;
		}
		if (robj)
			break;
		searchcount++;
		if (searchcount > maxsearch)
			break;
	}

	if (!robj)
	{
		act("&+GThe &+ritem gods&+G could not find a better type of &+yitem &+Gthan your &n$p&+G this time. &+WTry again&+G. If your item's value is above &+W55&+G you may have the &+Wbest&+G "
		    "item of that type!\r\n",
		    FALSE,
		    ch,
		    source,
		    0,
		    TO_CHAR);
		return;
	}

	// Remove Curse, Secret, add Invis
	if (IS_SET(robj->extra_flags, ITEM_SECRET))
	{
		REMOVE_BIT(robj->extra_flags, ITEM_SECRET);
	}
	if (IS_SET(robj->extra_flags, ITEM_NODROP))
	{
		REMOVE_BIT(robj->extra_flags, ITEM_NODROP);
	}

	if (IS_SET(robj->extra_flags, ITEM_INVISIBLE))
	{
		REMOVE_BIT(robj->extra_flags, ITEM_INVISIBLE);
	}
	SET_BIT(robj->extra_flags, ITEM_NOREPAIR);
	SUB_MONEY(ch, cost, 0);
	send_to_char("Your pockets feel &+Wlighter&n.\r\n", ch);

	act("&+BYour enhancement is a success! You now have &n$p&+B!\r\n", FALSE, ch, robj, 0, TO_CHAR);
	snprintf(buf, MAX_STRING_LENGTH, "&+wFinal item value: %d.&n\r\n", itemvalue(robj));
	send_to_char(buf, ch);
	obj_to_char(robj, ch);
	obj_from_char(source);
	extract_obj(source);
	obj_from_char(material);
	extract_obj(material);
	statuslog(ch->player.level,
	          "&+BEnhancement&n:&n %s&n just got [%d] '%s&n' ival [%d] at [%d]!",
	          GET_NAME(ch),
	          obj_index[robj->R_num].virtual_number,
	          robj->short_description,
	          itemvalue(robj),
	          (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);
	return;
}

/* Stat name → APPLY location mapping for stat-enhance */
static const struct {
	const char *name;
	int         apply_loc;
	const char *color;
	const char *display_name;
} enhance_stat_names[] = {
	{"str",       APPLY_STR,     "&+r", "strength"},
	{"strength",  APPLY_STR,     "&+r", "strength"},
	{"dex",       APPLY_DEX,     "&+g", "dexterity"},
	{"dexterity", APPLY_DEX,     "&+g", "dexterity"},
	{"int",       APPLY_INT,     "&+M", "intelligence"},
	{"intelligence", APPLY_INT,  "&+M", "intelligence"},
	{"wis",       APPLY_WIS,     "&+c", "wisdom"},
	{"wisdom",    APPLY_WIS,     "&+c", "wisdom"},
	{"con",       APPLY_CON,     "&+c", "constitution"},
	{"constitution", APPLY_CON,  "&+c", "constitution"},
	{"agi",       APPLY_AGI,     "&+B", "agility"},
	{"agility",   APPLY_AGI,     "&+B", "agility"},
	{"pow",       APPLY_POW,     "&+L", "power"},
	{"power",     APPLY_POW,     "&+L", "power"},
	{"cha",       APPLY_CHA,     "&+C", "charisma"},
	{"charisma",  APPLY_CHA,     "&+C", "charisma"},
	{"hit",       APPLY_HIT,     "&+R", "health"},
	{"health",    APPLY_HIT,     "&+R", "health"},
	{"ac",        APPLY_AC,      "&+W", "armor class"},
	{"armor",     APPLY_AC,      "&+W", "armor class"},
	{"hitroll",   APPLY_HITROLL, "&+y", "precision"},
	{"damroll",   APPLY_DAMROLL, "&+y", "damage"},
	{"str_max",   APPLY_STR_MAX, "&+r", "greater strength"},
	{"dex_max",   APPLY_DEX_MAX, "&+g", "greater dexterity"},
	{"int_max",   APPLY_INT_MAX, "&+M", "greater intelligence"},
	{"wis_max",   APPLY_WIS_MAX, "&+c", "greater wisdom"},
	{"con_max",   APPLY_CON_MAX, "&+c", "greater constitution"},
	{"agi_max",   APPLY_AGI_MAX, "&+B", "greater agility"},
	{"pow_max",   APPLY_POW_MAX, "&+L", "greater power"},
	{"cha_max",   APPLY_CHA_MAX, "&+C", "greater charisma"},
	{NULL,        0,             NULL,  NULL}
};

/* Return an item's unmodified prototype value for one APPLY location. */
static int enhance_base_modifier(P_obj item, int apply_loc)
{
	P_obj base;
	int   i;
	int   modifier = 0;

	if (!item || apply_loc == APPLY_NONE)
		return 0;

	base = read_object(OBJ_VNUM(item), VIRTUAL);
	if (!base)
		return 0;

	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (base->affected[i].location == apply_loc)
		{
			modifier = base->affected[i].modifier;
			break;
		}
	}
	extract_obj(base);
	return modifier;
}

/* Format a raw-material object name without retaining a temporary game object. */
static void enhance_material_name(int vnum, char *buf, size_t size)
{
	P_obj material = read_object(vnum, VIRTUAL);

	if (material)
	{
		snprintf(buf, size, "%s", material->short_description);
		extract_obj(material);
	}
	else
		snprintf(buf, size, "raw material [%d]", vnum);
}

static int enhance_entry_modifier(const struct enhance_index_entry *entry, int apply_loc)
{
	int i;
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (entry->apply_loc[i] == apply_loc)
			return entry->apply_mod[i];
	return 0;
}

/* A superior stat may reach floor(1.5 * its positive prototype modifier). */
static int enhance_stat_cap(int base_modifier)
{
	return base_modifier > 0 ? (int)(base_modifier * enhance_stat_cap_multiplier) : 0;
}

/* Find the deterministic next template: exact stat value, compatible wear slot, lowest vnum. */
static struct enhance_index_entry *find_stat_enhance_target(P_obj source, int apply_loc, int desired_mod)
{
	struct enhance_index_entry *entry;
	struct enhance_index_entry *best = NULL;
	unsigned int source_wear = source->wear_flags & ~enhance_wear_skip_mask;

	for (entry = enhance_stat_table[enhance_stat_hash(apply_loc)]; entry; entry = entry->next)
	{
		if (enhance_entry_modifier(entry, apply_loc) != desired_mod)
			continue;
		if (source_wear && !(entry->wear_flags & source_wear))
			continue;
		if (!best || entry->vnum < best->vnum)
			best = entry;
	}
	return best;
}

/* Apply the game's visible superior marker once, without touching craftsmanship. */
static void mark_item_superior(P_obj item)
{
	char short_desc[MAX_STRING_LENGTH];

	if (!item || !item->short_description ||
	    strstr(item->short_description, "&+w[&+Lsu&+wp&+Wer&+wi&+Lor&+w]&n"))
		return;

	snprintf(short_desc, sizeof(short_desc),
	         "%s&n &+w[&+Lsu&+wp&+Wer&+wi&+Lor&+w]&n",
	         item->short_description);
	set_short_description(item, short_desc);
}

#define MAX_SUPERIOR_MATERIALS (MAX_OBJ_AFFECT * 2)

struct superior_material_requirement {
	int vnum;
	int count;
};

struct superior_enhancement_plan {
	int slots[MAX_OBJ_AFFECT];
	int slot_count;
	int remaining_enhancements;
	struct superior_material_requirement materials[MAX_SUPERIOR_MATERIALS];
	int material_count;
};

static bool is_superior_stat_apply(int apply_loc)
{
	int i;
	for (i = 0; enhance_stat_names[i].name; i++)
		if (enhance_stat_names[i].apply_loc == apply_loc)
			return TRUE;
	return FALSE;
}

static bool superior_plan_add_material(struct superior_enhancement_plan *plan, int vnum, int count)
{
	int i;

	if (count <= 0)
		return TRUE;
	for (i = 0; i < plan->material_count; i++)
	{
		if (plan->materials[i].vnum == vnum)
		{
			plan->materials[i].count += count;
			return TRUE;
		}
	}
	if (plan->material_count >= MAX_SUPERIOR_MATERIALS)
		return FALSE;
	plan->materials[plan->material_count].vnum = vnum;
	plan->materials[plan->material_count].count = count;
	plan->material_count++;
	return TRUE;
}

/* Count only template-backed future steps, so the preview never promises an unavailable tier. */
static int superior_stat_remaining_steps(P_obj item, int apply_loc, int current, int cap)
{
	int steps = 0;
	int next;

	for (next = current + 1; next <= cap; next++)
	{
		if (!find_stat_enhance_target(item, apply_loc, next))
			break;
		steps++;
	}
	return steps;
}

/* Build the next atomic all-stat improvement and aggregate duplicate material vnums. */
static bool build_superior_enhancement_plan(P_obj item, struct superior_enhancement_plan *plan)
{
	int i;

	memset(plan, 0, sizeof(*plan));
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		int base;
		int cap;
		int remaining;
		int low_vnum;
		int high_vnum;
		int high_count;
		int low_count;
		struct enhance_index_entry *target;
		P_obj target_obj;

		if (item->affected[i].location == APPLY_NONE || item->affected[i].modifier <= 0 ||
		    !is_superior_stat_apply(item->affected[i].location))
			continue;
		base = enhance_base_modifier(item, item->affected[i].location);
		cap = enhance_stat_cap(base);
		if (base <= 0 || item->affected[i].modifier >= cap)
			continue;
		target = find_stat_enhance_target(item, item->affected[i].location,
		                                  item->affected[i].modifier + 1);
		if (!target || !(target_obj = read_object(target->vnum, VIRTUAL)))
			continue;

		low_vnum = get_matstart(target_obj);
		extract_obj(target_obj);
		high_vnum = low_vnum + 4;
		high_count = (target->ival + 4) / 5;
		low_count = (target->ival + 4) - high_count * 5;
		low_count = (int)(low_count * enhance_stat_material_quantity_multiplier + 0.999999);
		high_count = (int)(high_count * enhance_stat_material_quantity_multiplier + 0.999999);
		if (!superior_plan_add_material(plan, low_vnum, low_count) ||
		    !superior_plan_add_material(plan, high_vnum, high_count))
			return FALSE;

		plan->slots[plan->slot_count++] = i;
		remaining = superior_stat_remaining_steps(item, item->affected[i].location,
		                                           item->affected[i].modifier, cap);
		plan->remaining_enhancements = MAX(plan->remaining_enhancements, remaining);
	}
	return plan->slot_count > 0;
}

static bool superior_plan_has_materials(P_char ch, const struct superior_enhancement_plan *plan)
{
	int i;
	for (i = 0; i < plan->material_count; i++)
		if (vnum_in_inv(ch, plan->materials[i].vnum) < plan->materials[i].count)
			return FALSE;
	return TRUE;
}

/* Aggregate-only preview: do not reveal affected stat names, values, or caps. */
static void show_superior_requirements(P_char ch, P_obj item, const struct superior_enhancement_plan *plan)
{
	char buf[MAX_STRING_LENGTH];
	char name[MAX_STRING_LENGTH];
	char line[MAX_STRING_LENGTH];
	int i;

	act("&+CThe &n$p&+C shimmers in your hands, its latent potential waiting to be unlocked.&n", FALSE, ch, item, 0, TO_CHAR);
	snprintf(buf, sizeof(buf), "&+YSuperior enhancements remaining:&n &+W%d&n\r\n"
	         "&+YMaterials required for the next enhancement:&n\r\n",
	         plan->remaining_enhancements);
	for (i = 0; i < plan->material_count; i++)
	{
		enhance_material_name(plan->materials[i].vnum, name, sizeof(name));
		snprintf(line, sizeof(line), "  &+W%d&n %s &+w(you have %d)&n\r\n",
		         plan->materials[i].count, name, vnum_in_inv(ch, plan->materials[i].vnum));
		strcat(buf, line);
	}
	strcat(buf, "&+ySyntax:&n enhance <item>\r\n");
	send_to_char(buf, ch);
}

/* Validate first, then consume the entire aggregate tribute and upgrade every planned slot. */
static bool perform_superior_enhancement(P_char ch, P_obj source,
	                                      const struct superior_enhancement_plan *plan)
{
	char buf[MAX_STRING_LENGTH];
	int i;
	int cost = enhance_stat_platinum_base + itemvalue(source) * enhance_stat_platinum_per_ival;

	if (!superior_plan_has_materials(ch, plan))
		return FALSE;
	if (GET_MONEY(ch) < cost)
	{
		snprintf(buf, sizeof(buf), "&+yIt will require &+W%d platinum&+y to enhance this item.\r\n", cost / 1000);
		send_to_char(buf, ch);
		return FALSE;
	}

	/* All availability checks precede every state mutation, preserving atomicity. */
	SUB_MONEY(ch, cost, 0);
	for (i = 0; i < plan->material_count; i++)
		vnum_from_inv(ch, plan->materials[i].vnum, plan->materials[i].count);
	for (i = 0; i < plan->slot_count; i++)
		source->affected[plan->slots[i]].modifier++;
	mark_item_superior(source);

	snprintf(buf, sizeof(buf), "&+BYour enhancement thrums with superior energy! &+w%d properties improved.&n\r\n", plan->slot_count);
	send_to_char(buf, ch);
	statuslog(ch->player.level,
	          "&+BStat-Enhance&n: %s&n enhanced '%s&n' across %d properties at [%d]!",
	          GET_NAME(ch), source->short_description, plan->slot_count,
	          (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);
	return TRUE;
}



void do_enhance(P_char ch, char *argument, int cmd)
{
	P_obj source, material;
	char  first[MAX_INPUT_LENGTH];
	char  second[MAX_INPUT_LENGTH];
	char  rest[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;

	if (!argument || !*argument)
	{
		if (!enhance_stat_enabled)
		{
			send_to_char("&+yWhich &+Witem &+ywould you like to &+men&+Mhan&+mce&+y? &n\r\n"
			             "Syntax: enhance <source item you want to upgrade> <upgrade material item>\r\n", ch);
			return;
		}
		send_to_char("&+yWhich &+Witem &+ywould you like to &+men&+Mhan&+mce&+y? &n\r\n"
		             "Syntax: enhance <source item>\r\n"
		             "        enhance <source item> <material item> &+w(legacy)\r\n", ch);
		return;
	}

	half_chop(argument, first, rest);
	half_chop(rest, second, rest);

	/* Enabled donor-free enhancement: one command upgrades every eligible stat atomically. */
	if (!*second && enhance_stat_enabled)
	{
		struct superior_enhancement_plan plan;

		if (!(source = get_obj_in_list_vis(ch, first, ch->carrying)))
		{
			act("&+yWhich &+Witem &+ywould you like to &+men&+Mhan&+mce&+y?", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		if (!is_salvageable(source))
		{
			act("&+yYour $p&+y cannot be used in this way... try something else&n.", FALSE, ch, source, 0, TO_CHAR);
			return;
		}
		if (is_enhance_banned(source))
		{
			act("&+yYour $p&+y has too many conflicting enchantments for further enhancement&n.", FALSE, ch, source, 0, TO_CHAR);
			return;
		}
		if (itemvalue(source) > GET_LEVEL(ch) * enhance_level_gate_multiplier)
		{
			snprintf(rest, sizeof(rest), "&+yThis item has ival %d; at your level you can enhance items up to ival %d.\r\n",
			         itemvalue(source), GET_LEVEL(ch) * enhance_level_gate_multiplier);
			send_to_char(rest, ch);
			return;
		}
		if (!build_superior_enhancement_plan(source, &plan))
		{
			send_to_char("&+yThis item has no further superior enhancement available.\r\n", ch);
			return;
		}
		if (!superior_plan_has_materials(ch, &plan))
		{
			show_superior_requirements(ch, source, &plan);
			return;
		}
		perform_superior_enhancement(ch, source, &plan);
		return;
	}

	/* Original 2-arg enhance */
	half_chop(argument, first, rest);
	half_chop(rest, second, rest);

	if (!(source = get_obj_in_list_vis(ch, first, ch->carrying)))
	{
		act("&+yWhich &+Witem &+ywould you like to &+men&+Mhan&+mce&+y?", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!(material = get_obj_in_list_vis(ch, second, ch->carrying)))
	{
		if (enhance_stat_enabled)
		{
			struct superior_enhancement_plan plan;
			if (build_superior_enhancement_plan(source, &plan))
				show_superior_requirements(ch, source, &plan);
			else
				send_to_char("&+yThis item has no further superior enhancement available.\r\n", ch);
		}
		else
			act("And which object is the enhancement object?", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	if (!strcmp(first, second))
	{
		send_to_char("&+yYou cannot enhance an item with itself!\r\n", ch);
		return;
	}
	if (!is_salvageable(source))
	{
		act("&+yYour $p&+y cannot be used in this way... try something else&n.", FALSE, ch, source, 0, TO_CHAR);
		return;
	}
	if (!is_salvageable(material))
	{
		act("&+yYour $p&+y cannot be used in this way... try something else&n.", FALSE, ch, material, 0, TO_CHAR);
		return;
	}
	else if (OBJ_VNUM(material) == OBJ_VNUM(source))
	{
		send_to_char("&+yYou cannot enhance an item with itself!\r\n", ch);
		return;
	}
	// If source is a weapon, material must be either a weapon or an essence.
	if (IS_SET(source->wear_flags, ITEM_WIELD) && !IS_SET(material->wear_flags, ITEM_WIELD) && (OBJ_VNUM(material) < 400238 || OBJ_VNUM(material) > 400258))
	{
		send_to_char("&+YWeapons&+y can only enhance other &+Yweapons&n!\r\n", ch);
		return;
	}

	if (OBJ_VNUM(material) > 400237 && OBJ_VNUM(material) < 400259)
		modenhance(ch, source, material);
	else
		enhance(ch, source, material);
}

const char *modenhance_names[APPLY_LAST + 1] =
{
	0,
	"&+wof &+rstrength&n",
	"&+wof &+gdexterity&n",
	"&+wof &+Mintelligence&n",
	"&+wof &+cwisdom&n",
	"&+wof &+cconstitution&n",
	0, 0, 0, 0, 0, 0,
	0, // mana
	"&+wof &+Rhealth&n",
	0, // moves
	0, 0, 0,
	"&+wof &+yprecision&n",
	"&+wof &+ydamage&n",
	0, 0, 0, 0, 0, 0,
	"&+wof &+Bagility&n",
	"&+wof &+Lpower&n",
	"&+wof &+Ccharisma&n",
	0, // karma
	0, // luck
	"&+wof &+rgreater strength&n",
	"&+wof &+ggreater dexterity&n",
	"&+wof &+Mgreater intelligence&n",
	"&+wof &+cgreater wisdom&n",
	"&+wof &+cgreater constitution&n",
	"&+wof &+Bgreater agility&n",
	"&+wof &+Lgreater power&n",
	"&+wof &+Cgreater charisma&n",
	0, // greater karma
	0, // greater luck
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0,
	"&+wof &+gregeneration&n",
	"&+wof &+Gendurance&n",
	0, // mana reg
	0, 0,
};

static int essence_loc(int vnum)
{
	switch (vnum)
	{
	case 400238: return APPLY_INT;
	case 400239: return APPLY_INT_MAX;
	case 400240: return APPLY_CON;
	case 400241: return APPLY_CON_MAX;
	case 400242: return APPLY_AGI;
	case 400243: return APPLY_AGI_MAX;
	case 400244: return APPLY_DEX;
	case 400245: return APPLY_DEX_MAX;
	case 400246: return APPLY_STR;
	case 400247: return APPLY_STR_MAX;
	case 400248: return APPLY_CHA;
	case 400249: return APPLY_CHA_MAX;
	case 400250: return APPLY_WIS;
	case 400251: return APPLY_WIS_MAX;
	case 400252: return APPLY_POW;
	case 400253: return APPLY_POW_MAX;
	case 400254: return APPLY_HIT;
	case 400255: return APPLY_HITROLL;
	case 400256: return APPLY_DAMROLL;
	case 400257: return APPLY_HIT_REG;
	case 400258: return APPLY_MOVE_REG;
	default: return 0;
	}
}

void modenhance(P_char ch, P_obj source, P_obj material)
{

	if (!ch || !source || !material)
		return;

	if (is_enhance_banned(source))
	{
		act("&+RYour $p&+R has too many conflicting enchantments to be enhanced.&n", FALSE, ch, source, 0, TO_CHAR);
		return;
	}
	if (is_enhance_banned(material))
	{
		act("&+RYour $p&+R cannot be used as an enhancement material&n.", FALSE, ch, material, 0, TO_CHAR);
		return;
	}

	char  buf[MAX_STRING_LENGTH];
	const char* modstring;
	P_obj robj;
	long  robjint;
	int   mod = 0, loctype = 0;
	int   validobj, cost = 0, searchcount = 0, tries;
	int   sval = itemvalue(source);
	validobj   = 0;
	int val    = itemvalue(material);
	int minval = itemvalue(source) - enhance_material_ival_delta;

	if (val <= 20)
	{
		cost = 1000;
		if (GET_MONEY(ch) < cost)
		{
			send_to_char("It will require &+W1 platinum&n to &+Benhance&n this item.\r\n", ch);
			return;
		}
	}
	else if (val <= 30)
	{
		cost = 20000;
		if (GET_MONEY(ch) < cost)
		{
			send_to_char("It will require &+W20 platinum&n to &+Benhance&n this item.\r\n", ch);
			return;
		}
	}
	else
	{
		cost = 100000;
		if (GET_MONEY(ch) < cost)
		{
			send_to_char("It will require &+W100 platinum&n to &+Benhance&n this item.\r\n", ch);
			return;
		}
	}

	int modtype = OBJ_VNUM(material);
	int loc = essence_loc(modtype);
	if (!loc)
		return send_to_char("&=rYBuggy essence material, please tell a god!&n\n", ch);

	if (source->affected[2].location == loc)
		loctype = 1;
	else
		source->affected[2].location = loc;
	modstring = modenhance_names[loc];

	switch (loc)
	{
	case APPLY_HIT:
	case APPLY_HIT_REG:
	case APPLY_MOVE_REG:
		mod = 3;
		break;
	default:
		mod = 1;
	}

	if (loctype == 1)
	{
		// IF they've been modified less than 3 times.
		if (source->affected[2].modifier / mod < enhance_mod_max_steps)
			source->affected[2].modifier += mod;
		else
		{
			send_to_char("Your enhancement was a failure.  Too much magic.\n", ch);
			return;
		}
	}
	else
		source->affected[2].modifier = mod;

	SET_BIT(source->extra2_flags, ITEM2_ENHANCED);

	SUB_MONEY(ch, cost, 0);
	send_to_char("Your pockets feel &+Wlighter&n.\r\n", ch);

	act("&+BYour enhancement is a success! Your &n$p&+B now feels slightly more powerful!\r\n", FALSE, ch, source, 0, TO_CHAR);
	snprintf(buf, MAX_STRING_LENGTH, "&+wApplied property: %s&+w. Final item value: %d.&n\r\n", modstring, itemvalue(source));
	send_to_char(buf, ch);

	obj_from_char(material);
	extract_obj(material);

	if (IS_ENCRUSTED(source))
		return describe_encrusted_enhanced(source);

	P_obj tempobj = read_object(OBJ_VNUM(source), VIRTUAL);
	char  tempdesc[MAX_STRING_LENGTH], short_desc[MAX_STRING_LENGTH], keywords[MAX_STRING_LENGTH];

	snprintf(keywords, MAX_STRING_LENGTH, "%s enhanced", tempobj->name);

	snprintf(tempdesc, MAX_STRING_LENGTH, "%s", tempobj->short_description);
	snprintf(short_desc, MAX_STRING_LENGTH, "%s %s&n", tempdesc, modstring);
	set_keywords(source, keywords);
	set_short_description(source, short_desc);
	extract_obj(tempobj);

	return;
}

int get_progress(P_char ch, int ach, uint required)
{
	int                   prog = 0, percentage = 0;
	struct affected_type *findaf, *next_af; // initialize affects

	for (findaf = ch->affected; findaf; findaf = next_af)
	{
		next_af = findaf->next;
		if (findaf && findaf->type == ach)
			prog = findaf->modifier;
	}

	if (required == 0)
		return 0;

	if (prog > 0)
	{
		prog *= 100;
		percentage = prog / required;
	}

	if (prog < 0)
		return 0;

	return percentage;
}

void thanksgiving_proc(P_char ch)
{
	P_char mob;
	if (!ch)
		return;
	char buff[MAX_STRING_LENGTH];
	snprintf(buff, MAX_STRING_LENGTH, " %s 86", GET_NAME(ch));
	act("&+YSuddenly and without warning, a &+rPlump &+yTurkey &+Yappears out of no where, seemly attracted to the freshly spilled &+Rblood&n!", TRUE, ch, 0, 0, TO_CHAR);
	act("&+YSuddenly and without warning, a &+rPlump &+yTurkey &+Yappears out of no where, seemly attracted to the freshly spilled &+Rblood&n!", TRUE, ch, 0, 0, TO_ROOM);
	// do_givepet(ch, buff, CMD_GIVEPET);
	mob = read_mobile(400005, VIRTUAL);
	if (!mob)
		return;
	obj_to_char(read_object(400232, VIRTUAL), mob);
	char_to_room(mob, ch->in_room, 0);
}

/* Generates at most one enhancement essence after the caller has determined that
 * this is an eligible NPC-death event. */
static void enhance_load_essence_drop(P_char ch, P_char killer)
{
	int reward;
	int moblvl = GET_LEVEL(ch);

	if (!enhance_essence_drop_enabled)
		return;

	/* Level gate: skip if mob level outside configured range. */
	if (moblvl < enhance_essence_minimum_level || moblvl > enhance_essence_maximum_level)
		return;

	/* Zone override: resolve effective roll parameters, falling back to globals. */
	{
		struct enhance_essence_zone_rule *rule = enhance_find_essence_zone_rule(ROOM_ZONE_NUMBER(ch->in_room));
		int primary_roll_max = enhance_essence_primary_roll_max;
		int max_roll_max     = enhance_essence_max_roll_max;
		int elite_mult       = enhance_essence_elite_level_multiplier;

		if (rule)
		{
			if (rule->primary_roll_max > 0)
				primary_roll_max = rule->primary_roll_max;
			if (rule->max_roll_max > 0)
				max_roll_max = rule->max_roll_max;
			if (rule->elite_level_multiplier > 0)
				elite_mult = rule->elite_level_multiplier;
		}

		if (IS_ELITE(ch))
		{
			moblvl *= elite_mult;
		}
		if (number(1, primary_roll_max) < moblvl)
	{
		debug("enhancematload: mob: '%s' (%d) moblvl %d%s", J_NAME(ch), GET_VNUM(ch), moblvl, IS_ELITE(ch) ? " ELITE." : ".");
		if (number(1, max_roll_max) < moblvl)
		{
			switch (number(1, 8))
			{
				case 1:
					reward = 400239;
					break;
				case 2:
					reward = 400241;
					break;
				case 3:
					reward = 400243;
					break;
				case 4:
					reward = 400245;
					break;
				case 5:
					reward = 400247;
					break;
				case 6:
					reward = 400249;
					break;
				case 7:
					reward = 400251;
					break;
				case 8:
					reward = 400253;
					break;
			}
		}
		else
		{
			reward = number(1, 13);
			switch (reward)
			{
				case 1:
					reward = 400238;
					break;
				case 2:
					reward = 400240;
					break;
				case 3:
					reward = 400242;
					break;
				case 4:
					reward = 400244;
					break;
				case 5:
					reward = 400246;
					break;
				case 6:
					reward = 400248;
					break;
				case 7:
					reward = 400250;
					break;
				case 8:
					reward = 400252;
					break;
				case 9:
					reward = 400254;
					break;
				case 10:
					reward = 400255;
					break;
				case 11:
					reward = 400256;
					break;
				case 12:
					reward = 400257;
					break;
				case 13:
					reward = 400258;
					break;
			}
		}
		P_obj gift = read_object(reward, VIRTUAL);
		if (gift)
		{
			// Show reward to master if killer is a pet.
			debug("enhancematload: '%s' (%d) rewarded to %s.", gift->short_description, OBJ_VNUM(gift), IS_PC_PET(killer) ? J_NAME(get_linked_char(killer, LNK_PET)) : J_NAME(killer));
					obj_to_char(gift, ch);
								}
							}
						}
					}

		/* =============================================================================
		 *  ENHANCE SYSTEM — GLOBALS, HASH TABLES, CONFIG PARSER, INDEX BUILDER
		 * =============================================================================
		 */

		/* ---- Config settings (defaults) ---- */
		int enhance_ival_cap                         = 95;
		int enhance_material_ival_delta              = 5;
		int enhance_guild_insignia_ival_bonus        = 5;
		int enhance_cost_low_ival_threshold          = 20;
		int enhance_cost_low_amount                  = 1000;
		int enhance_cost_high_amount                 = 10000;
		int enhance_search_vnum_min                  = 1300;
		int enhance_search_vnum_max                  = 134000;
		int enhance_search_max_attempts              = 20000;
		int enhance_wear_skip_mask                   = 15;
		int enhance_original_max_roll                = 4;
		int enhance_original_cascade_down_first      = 1;
		int enhance_level_gate_multiplier            = 3;
int enhance_mod_max_steps                     = 3;
		int enhance_luck_extreme_range               = 1200;
		int enhance_luck_very_range                  = 800;
		int enhance_luck_lucky_range                 = 400;
		int enhance_ival_gain_extreme                = 4;
		int enhance_ival_gain_very                   = 3;
		int enhance_ival_gain_lucky                  = 2;
		int enhance_ival_gain_normal                 = 1;
		/* Fail closed: stat enhancement requires an explicit config opt-in. */
		int enhance_stat_enabled                      = 0;
		/* Also fail closed: NPC reset material fallback is independently opt-in. */
		int enhance_stat_npc_material_fallback_enabled = 0;
double enhance_stat_cap_multiplier = 1.5;
int enhance_stat_platinum_base = 1000;
int enhance_stat_platinum_per_ival = 100;
double enhance_stat_material_quantity_multiplier = 1.0;

		/* Candidate-only exclusions for legacy random enhancement. */
		#define ENHANCE_MAX_POOL_EXCLUSIONS 256
		static int enhance_pool_excluded_zones[ENHANCE_MAX_POOL_EXCLUSIONS];
		static int enhance_pool_excluded_zone_count = 0;
		static int enhance_pool_excluded_vnums[ENHANCE_MAX_POOL_EXCLUSIONS];
		static int enhance_pool_excluded_vnum_count = 0;

		/* ---- Bitvector allow masks ---- */
		unsigned long enhance_allow_mask  = 0;
		unsigned long enhance_allow_mask2 = 0;
		unsigned long enhance_allow_mask3 = 0;
		unsigned long enhance_allow_mask4 = 0;
		unsigned long enhance_allow_mask5 = 0;

		/* ---- Hash tables ---- */
		struct enhance_index_entry *enhance_ival_table[ENHANCE_IVAL_TABLE_SIZE] = {0};
		struct enhance_index_entry *enhance_stat_table[ENHANCE_STAT_TABLE_SIZE] = {0};

		/* ---- Flag name → bitvalue lookup table ---- */
		static const struct {
			const char     *name;
			unsigned long   bit;
			int             section; /* 0=bitvector, 1=bitvector2, 2=bitvector3, 3=bitvector4, 4=bitvector5 */
		} enhance_flag_lookup[] = {
			/* bitvector (AFF_) */
			{"AFF_NONE",              AFF_NONE,             0},
			{"AFF_BLIND",             AFF_BLIND,            0},
			{"AFF_INVISIBLE",         AFF_INVISIBLE,        0},
			{"AFF_FARSEE",            AFF_FARSEE,           0},
			{"AFF_DETECT_INVISIBLE",  AFF_DETECT_INVISIBLE, 0},
			{"AFF_HASTE",             AFF_HASTE,            0},
			{"AFF_SENSE_LIFE",        AFF_SENSE_LIFE,       0},
			{"AFF_MINOR_GLOBE",       AFF_MINOR_GLOBE,      0},
			{"AFF_STONE_SKIN",        AFF_STONE_SKIN,       0},
			{"AFF_UD_VISION",         AFF_UD_VISION,        0},
			{"AFF_ARMOR",             AFF_ARMOR,            0},
			{"AFF_WRAITHFORM",        AFF_WRAITHFORM,       0},
			{"AFF_WATERBREATH",       AFF_WATERBREATH,      0},
			{"AFF_KNOCKED_OUT",       AFF_KNOCKED_OUT,      0},
			{"AFF_PROTECT_EVIL",      AFF_PROTECT_EVIL,     0},
			{"AFF_BOUND",             AFF_BOUND,            0},
			{"AFF_SLOW_POISON",       AFF_SLOW_POISON,      0},
			{"AFF_PROTECT_GOOD",      AFF_PROTECT_GOOD,     0},
			{"AFF_SLEEP",             AFF_SLEEP,            0},
			{"AFF_SKILL_AWARE",       AFF_SKILL_AWARE,      0},
			{"AFF_SNEAK",             AFF_SNEAK,            0},
			{"AFF_HIDE",              AFF_HIDE,             0},
			{"AFF_FEAR",              AFF_FEAR,             0},
			{"AFF_CHARM",             AFF_CHARM,            0},
			{"AFF_MEDITATE",          AFF_MEDITATE,         0},
			{"AFF_BARKSKIN",          AFF_BARKSKIN,         0},
			{"AFF_INFRAVISION",       AFF_INFRAVISION,      0},
			{"AFF_LEVITATE",          AFF_LEVITATE,         0},
			{"AFF_FLY",               AFF_FLY,              0},
			{"AFF_AWARE",             AFF_AWARE,            0},
			{"AFF_PROT_FIRE",         AFF_PROT_FIRE,        0},
			{"AFF_CAMPING",           AFF_CAMPING,          0},
			{"AFF_BIOFEEDBACK",       AFF_BIOFEEDBACK,      0},
			{"AFF_INFERNAL_FURY",     AFF_INFERNAL_FURY,    0},
			{"AFF_FREEDOM_OF_MVMNT",  AFF_FREEDOM_OF_MVMNT, 0},
			{"AFF_SANCTUM_DRACONIS",  AFF_SANCTUM_DRACONIS, 0},
			/* bitvector2 (AFF2_) */
			{"AFF2_FIRESHIELD",       AFF2_FIRESHIELD,      1},
			{"AFF2_ULTRAVISION",      AFF2_ULTRAVISION,     1},
			{"AFF2_DETECT_EVIL",      AFF2_DETECT_EVIL,     1},
			{"AFF2_DETECT_GOOD",      AFF2_DETECT_GOOD,     1},
			{"AFF2_DETECT_MAGIC",     AFF2_DETECT_MAGIC,    1},
			{"AFF2_MAJOR_PHYSICAL",   AFF2_MAJOR_PHYSICAL,  1},
			{"AFF2_PROT_COLD",        AFF2_PROT_COLD,       1},
			{"AFF2_PROT_LIGHTNING",   AFF2_PROT_LIGHTNING,  1},
			{"AFF2_MINOR_PARALYSIS",  AFF2_MINOR_PARALYSIS, 1},
			{"AFF2_MAJOR_PARALYSIS",  AFF2_MAJOR_PARALYSIS, 1},
			{"AFF2_SLOW",             AFF2_SLOW,            1},
			{"AFF2_GLOBE",            AFF2_GLOBE,           1},
			{"AFF2_PROT_GAS",         AFF2_PROT_GAS,        1},
			{"AFF2_PROT_ACID",        AFF2_PROT_ACID,       1},
			{"AFF2_POISONED",         AFF2_POISONED,        1},
			{"AFF2_SOULSHIELD",       AFF2_SOULSHIELD,      1},
			{"AFF2_SILENCED",         AFF2_SILENCED,        1},
			{"AFF2_CONCEALMENT",      AFF2_CONCEALMENT,     1},
			{"AFF2_VAMPIRIC_TOUCH",   AFF2_VAMPIRIC_TOUCH,  1},
			{"AFF2_STUNNED",          AFF2_STUNNED,         1},
			{"AFF2_EARTH_AURA",       AFF2_EARTH_AURA,      1},
			{"AFF2_WATER_AURA",       AFF2_WATER_AURA,      1},
			{"AFF2_FIRE_AURA",        AFF2_FIRE_AURA,       1},
			{"AFF2_AIR_AURA",         AFF2_AIR_AURA,        1},
			{"AFF2_HOLDING_BREATH",   AFF2_HOLDING_BREATH,  1},
			{"AFF2_MEMORIZING",       AFF2_MEMORIZING,      1},
			{"AFF2_IS_DROWNING",      AFF2_IS_DROWNING,     1},
			{"AFF2_PASSDOOR",         AFF2_PASSDOOR,        1},
			{"AFF2_FLURRY",           AFF2_FLURRY,          1},
			{"AFF2_CASTING",          AFF2_CASTING,         1},
			{"AFF2_SCRIBING",         AFF2_SCRIBING,        1},
			{"AFF2_HUNTER",           AFF2_HUNTER,          1},
			/* bitvector3 (AFF3_) */
			{"AFF3_TENSORS_DISC",       AFF3_TENSORS_DISC,       2},
			{"AFF3_TRACKING",           AFF3_TRACKING,           2},
			{"AFF3_SINGING",            AFF3_SINGING,            2},
			{"AFF3_ECTOPLASMIC_FORM",   AFF3_ECTOPLASMIC_FORM,   2},
			{"AFF3_ABSORBING",          AFF3_ABSORBING,          2},
			{"AFF3_PROT_ANIMAL",        AFF3_PROT_ANIMAL,        2},
			{"AFF3_SPIRIT_WARD",        AFF3_SPIRIT_WARD,        2},
			{"AFF3_GR_SPIRIT_WARD",     AFF3_GR_SPIRIT_WARD,     2},
			{"AFF3_NON_DETECTION",      AFF3_NON_DETECTION,      2},
			{"AFF3_SILVER",             AFF3_SILVER,             2},
			{"AFF3_PLUSONE",            AFF3_PLUSONE,            2},
			{"AFF3_PLUSTWO",            AFF3_PLUSTWO,            2},
			{"AFF3_PLUSTHREE",          AFF3_PLUSTHREE,          2},
			{"AFF3_PLUSFOUR",           AFF3_PLUSFOUR,           2},
			{"AFF3_PLUSFIVE",           AFF3_PLUSFIVE,           2},
			{"AFF3_ENLARGE",            AFF3_ENLARGE,            2},
			{"AFF3_REDUCE",             AFF3_REDUCE,             2},
			{"AFF3_COVER",              AFF3_COVER,              2},
			{"AFF3_FOUR_ARMS",          AFF3_FOUR_ARMS,          2},
			{"AFF3_INERTIAL_BARRIER",   AFF3_INERTIAL_BARRIER,   2},
			{"AFF3_LIGHTNINGSHIELD",    AFF3_LIGHTNINGSHIELD,    2},
			{"AFF3_COLDSHIELD",         AFF3_COLDSHIELD,         2},
			{"AFF3_CANNIBALIZE",        AFF3_CANNIBALIZE,        2},
			{"AFF3_SWIMMING",           AFF3_SWIMMING,           2},
			{"AFF3_TOWER_IRON_WILL",    AFF3_TOWER_IRON_WILL,    2},
			{"AFF3_UNDERWATER",         AFF3_UNDERWATER,         2},
			{"AFF3_BLUR",               AFF3_BLUR,               2},
			{"AFF3_ENHANCE_HEALING",    AFF3_ENHANCE_HEALING,    2},
			{"AFF3_ELEMENTAL_FORM",     AFF3_ELEMENTAL_FORM,     2},
			{"AFF3_PASS_WITHOUT_TRACE", AFF3_PASS_WITHOUT_TRACE, 2},
			{"AFF3_PALADIN_AURA",       AFF3_PALADIN_AURA,       2},
			{"AFF3_FAMINE",             AFF3_FAMINE,             2},
			{"AFF3_VIVERNAE_CONCORDIA", AFF3_VIVERNAE_CONCORDIA, 2},
			/* bitvector4 (AFF4_) */
			{"AFF4_LOOTER",                   AFF4_LOOTER,                   3},
			{"AFF4_CARRY_PLAGUE",             AFF4_CARRY_PLAGUE,             3},
			{"AFF4_SACKING",                  AFF4_SACKING,                  3},
			{"AFF4_SENSE_FOLLOWER",           AFF4_SENSE_FOLLOWER,           3},
			{"AFF4_STORNOGS_SPHERES",         AFF4_STORNOGS_SPHERES,         3},
			{"AFF4_STORNOGS_GREATER_SPHERES", AFF4_STORNOGS_GREATER_SPHERES, 3},
			{"AFF4_VAMPIRE_FORM",             AFF4_VAMPIRE_FORM,             3},
			{"AFF4_NO_UNMORPH",               AFF4_NO_UNMORPH,               3},
			{"AFF4_HOLY_SACRIFICE",           AFF4_HOLY_SACRIFICE,           3},
			{"AFF4_BATTLE_ECSTASY",           AFF4_BATTLE_ECSTASY,           3},
			{"AFF4_DAZZLER",                  AFF4_DAZZLER,                  3},
			{"AFF4_PHANTASMAL_FORM",          AFF4_PHANTASMAL_FORM,          3},
			{"AFF4_NOFEAR",                   AFF4_NOFEAR,                   3},
			{"AFF4_REGENERATION",             AFF4_REGENERATION,             3},
			{"AFF4_DEAF",                     AFF4_DEAF,                     3},
			{"AFF4_BATTLETIDE",               AFF4_BATTLETIDE,               3},
			{"AFF4_EPIC_INCREASE",            AFF4_EPIC_INCREASE,            3},
			{"AFF4_MAGE_FLAME",               AFF4_MAGE_FLAME,               3},
			{"AFF4_GLOBE_OF_DARKNESS",        AFF4_GLOBE_OF_DARKNESS,        3},
			{"AFF4_DEFLECT",                  AFF4_DEFLECT,                  3},
			{"AFF4_HAWKVISION",               AFF4_HAWKVISION,               3},
			{"AFF4_MULTI_CLASS",              AFF4_MULTI_CLASS,              3},
			{"AFF4_SANCTUARY",                AFF4_SANCTUARY,                3},
			{"AFF4_HELLFIRE",                 AFF4_HELLFIRE,                 3},
			{"AFF4_SENSE_HOLINESS",           AFF4_SENSE_HOLINESS,           3},
			{"AFF4_PROT_LIVING",              AFF4_PROT_LIVING,              3},
			{"AFF4_DETECT_ILLUSION",          AFF4_DETECT_ILLUSION,          3},
			{"AFF4_ICE_AURA",                 AFF4_ICE_AURA,                 3},
			{"AFF4_REV_POLARITY",             AFF4_REV_POLARITY,             3},
			{"AFF4_NEG_SHIELD",               AFF4_NEG_SHIELD,               3},
			{"AFF4_TUPOR",                    AFF4_TUPOR,                    3},
			{"AFF4_WILDMAGIC",                AFF4_WILDMAGIC,                3},
			/* bitvector5 (AFF5_) */
			{"AFF5_DAZZLEE",           AFF5_DAZZLEE,           4},
			{"AFF5_MENTAL_ANGUISH",    AFF5_MENTAL_ANGUISH,    4},
			{"AFF5_MEMORY_BLOCK",      AFF5_MEMORY_BLOCK,      4},
			{"AFF5_VINES",             AFF5_VINES,             4},
			{"AFF5_ETHEREAL_ALLIANCE", AFF5_ETHEREAL_ALLIANCE, 4},
			{"AFF5_BLOOD_SCENT",       AFF5_BLOOD_SCENT,       4},
			{"AFF5_FLESH_ARMOR",       AFF5_FLESH_ARMOR,       4},
			{"AFF5_WET",               AFF5_WET,               4},
			{"AFF5_HOLY_DHARMA",       AFF5_HOLY_DHARMA,       4},
			{"AFF5_ENH_HIDE",          AFF5_ENH_HIDE,          4},
			{"AFF5_LISTEN",            AFF5_LISTEN,            4},
			{"AFF5_PROT_UNDEAD",       AFF5_PROT_UNDEAD,       4},
			{"AFF5_IMPRISON",          AFF5_IMPRISON,          4},
			{"AFF5_TITAN_FORM",        AFF5_TITAN_FORM,        4},
			{"AFF5_DELIRIUM",          AFF5_DELIRIUM,          4},
			{"AFF5_SHADE_MOVEMENT",    AFF5_SHADE_MOVEMENT,    4},
			{"AFF5_NOBLIND",           AFF5_NOBLIND,           4},
			{"AFF5_MAGICAL_GLOW",      AFF5_MAGICAL_GLOW,      4},
			{"AFF5_REFRESHING_GLOW",   AFF5_REFRESHING_GLOW,   4},
			{"AFF5_MINE",              AFF5_MINE,              4},
			{"AFF5_STANCE_OFFENSIVE",  AFF5_STANCE_OFFENSIVE,  4},
			{"AFF5_STANCE_DEFENSIVE",  AFF5_STANCE_DEFENSIVE,  4},
			{"AFF5_OBSCURING_MIST",    AFF5_OBSCURING_MIST,    4},
			{"AFF5_NOT_OFFENSIVE",     AFF5_NOT_OFFENSIVE,     4},
			{"AFF5_DECAYING_FLESH",    AFF5_DECAYING_FLESH,    4},
			{"AFF5_DREADNAUGHT",       AFF5_DREADNAUGHT,       4},
			{"AFF5_FOREST_SIGHT",      AFF5_FOREST_SIGHT,      4},
			{"AFF5_THORNSKIN",         AFF5_THORNSKIN,         4},
			{"AFF5_FOLLOWING",         AFF5_FOLLOWING,         4},
			{"AFF5_ORDERING",          AFF5_ORDERING,          4},
			{"AFF5_STONED",            AFF5_STONED,            4},
			{"AFF5_JUDICIUM_FIDEI",    AFF5_JUDICIUM_FIDEI,    4},
			{NULL,                     0,                       0}
		};

		/* ---- Hash functions ---- */
		static int enhance_hash(int key)
		{
			int h = abs(key) % ENHANCE_IVAL_TABLE_SIZE;
			return h;
		}

		static int enhance_stat_hash(int key)
		{
			int h = abs(key) % ENHANCE_STAT_TABLE_SIZE;
			return h;
		}

		/* Deterministic lookup: linear scan over the bounded zone-rules array.
		 * Returns NULL when no rule exists for the given virtual zone number. */
		static struct enhance_essence_zone_rule *enhance_find_essence_zone_rule(int zone_number)
		{
			int i;
			for (i = 0; i < enhance_essence_zone_rule_count; i++)
				if (enhance_essence_zone_rules[i].zone_number == zone_number)
					return &enhance_essence_zone_rules[i];
			return NULL;
		}

		/* =============================================================================
		 *  load_enhance_config() — Parse lib/enhance.cfg for settings and bitvector masks
		 * =============================================================================
		 */
		void load_enhance_config(void)
		{
			FILE *fp;
			char  line[1024];
			char  section[64];
			int   section_idx;
			int   i, line_num;
			char  flag_name[128];
			char *p, *eq, *comma;

			fp = fopen("lib/enhance.cfg", "r");
			if (!fp)
			{
				fprintf(stderr, "WARNING: Cannot open lib/enhance.cfg — using defaults.\r\n");
				logit(LOG_STATUS, "WARNING: Cannot open lib/enhance.cfg — using defaults.");
				return;
			}

			section[0]   = '\0';
			section_idx  = -1;
			line_num     = 0;
			/* Reloads must not retain stale opt-ins or pool exclusions. */
			enhance_allow_mask = 0;
			enhance_allow_mask2 = 0;
			enhance_allow_mask3 = 0;
			enhance_allow_mask4 = 0;
			enhance_allow_mask5 = 0;
			enhance_stat_enabled = 0;
			enhance_stat_npc_material_fallback_enabled = 0;
			enhance_essence_drop_enabled = 1;
			enhance_essence_primary_roll_max = 3000;
			enhance_essence_max_roll_max = 4000;
			enhance_essence_elite_level_multiplier = 1;
			enhance_essence_minimum_level = 1;
			enhance_essence_maximum_level = 1000000;
			enhance_essence_zone_rule_count = 0;
			enhance_pool_excluded_zone_count = 0;
			enhance_pool_excluded_vnum_count = 0;

			while (fgets(line, sizeof(line), fp))
			{
				line_num++;

				/* Strip trailing whitespace / newline */
				p = line + strlen(line);
				while (p > line && (*(p - 1) == '\n' || *(p - 1) == '\r' || *(p - 1) == ' ' || *(p - 1) == '	'))
					*--p = '\0';

				/* Skip empty lines and comments */
				if (line[0] == '\0' || line[0] == '#')
					continue;

				/* Detect section header */
				if (line[0] == '[')
				{
					p = line + 1;
					i = 0;
					while (*p && *p != ']' && i < (int)sizeof(section) - 1)
						section[i++] = *p++;
					section[i] = '\0';

					if      (!strcmp(section, "settings"))          section_idx = 0;
					else if (!strcmp(section, "enhance_stat"))      section_idx = 1;
					else if (!strcmp(section, "pool_exclude_zone")) section_idx = 2;
					else if (!strcmp(section, "pool_exclude_vnum")) section_idx = 3;
					else if (!strcmp(section, "essence_drop"))      section_idx = 4;
					else if (!strcmp(section, "essence_drop_zone")) section_idx = 5;
					else if (!strcmp(section, "bitvector"))         section_idx = 10;
					else if (!strcmp(section, "bitvector2"))   section_idx = 11;
					else if (!strcmp(section, "bitvector3"))   section_idx = 12;
					else if (!strcmp(section, "bitvector4"))   section_idx = 13;
					else if (!strcmp(section, "bitvector5"))   section_idx = 14;
					else if (!strcmp(section, "spell"))        section_idx = 20;
					else                                       section_idx = -1;
					continue;
				}

				/* Parse key = value lines */
				eq = strchr(line, '=');
				if (!eq)
					continue;

				*eq = '\0';
				/* Trim key */
				p = line;
				while (*p == ' ' || *p == '	')
					p++;
				char *key = p;
				p = eq - 1;
				while (p > key && (*p == ' ' || *p == '	'))
					*p-- = '\0';

				/* Trim value */
				p = eq + 1;
				while (*p == ' ' || *p == '	')
					p++;
				char *val = p;

				if (section_idx == 0)
				{
					/* [settings] section */
					int ival = atoi(val);

					if      (!strcmp(key, "enhance.ival.cap"))                      enhance_ival_cap                    = ival;
					else if (!strcmp(key, "enhance.material.ival.delta"))           enhance_material_ival_delta         = ival;
					else if (!strcmp(key, "enhance.guild.insignia.ival.bonus"))    enhance_guild_insignia_ival_bonus   = ival;
					else if (!strcmp(key, "enhance.cost.low.ival.threshold"))       enhance_cost_low_ival_threshold     = ival;
					else if (!strcmp(key, "enhance.cost.low.amount"))               enhance_cost_low_amount             = ival;
					else if (!strcmp(key, "enhance.cost.high.amount"))              enhance_cost_high_amount            = ival;
					else if (!strcmp(key, "enhance.search.vnum.min"))               enhance_search_vnum_min             = ival;
					else if (!strcmp(key, "enhance.search.vnum.max"))               enhance_search_vnum_max             = ival;
					else if (!strcmp(key, "enhance.search.max.attempts"))           enhance_search_max_attempts         = ival;
					else if (!strcmp(key, "enhance.wear.skip.mask"))                enhance_wear_skip_mask              = ival;
					else if (!strcmp(key, "enhance.original.max.roll"))             enhance_original_max_roll           = ival;
					else if (!strcmp(key, "enhance.original.cascade.down.first"))  enhance_original_cascade_down_first = ival;
					else if (!strcmp(key, "enhance.level.gate.multiplier") && ival > 0) enhance_level_gate_multiplier = ival;
					else if (!strcmp(key, "enhance.mod.max.steps") && ival > 0) enhance_mod_max_steps = ival;
					else if (!strcmp(key, "enhance.luck.extreme.range"))            enhance_luck_extreme_range          = ival;
					else if (!strcmp(key, "enhance.luck.very.range"))               enhance_luck_very_range             = ival;
					else if (!strcmp(key, "enhance.luck.lucky.range"))              enhance_luck_lucky_range            = ival;
					else if (!strcmp(key, "enhance.ival.gain.extreme"))             enhance_ival_gain_extreme           = ival;
					else if (!strcmp(key, "enhance.ival.gain.very"))                enhance_ival_gain_very              = ival;
					else if (!strcmp(key, "enhance.ival.gain.lucky"))               enhance_ival_gain_lucky             = ival;
					else if (!strcmp(key, "enhance.ival.gain.normal"))              enhance_ival_gain_normal            = ival;
					/* skip unknown settings silently */
				}
				else if (section_idx == 1)
				{
					/* [enhance_stat] section: explicit on/off gate for the stat lane. */
					if (!strcmp(key, "enhance_stat.enabled"))
						enhance_stat_enabled = atoi(val) ? 1 : 0;
					else if (!strcmp(key, "enhance_stat.npc_material_fallback.enabled"))
						enhance_stat_npc_material_fallback_enabled = atoi(val) ? 1 : 0;
					else if (!strcmp(key, "enhance_stat.cap.multiplier") && strtod(val, NULL) > 0.0)
						enhance_stat_cap_multiplier = strtod(val, NULL);
					else if (!strcmp(key, "enhance_stat.platinum.base") && atoi(val) >= 0)
						enhance_stat_platinum_base = atoi(val);
					else if (!strcmp(key, "enhance_stat.platinum.per.ival") && atoi(val) >= 0)
						enhance_stat_platinum_per_ival = atoi(val);
					else if (!strcmp(key, "enhance_stat.material.quantity.mult") && strtod(val, NULL) > 0.0)
						enhance_stat_material_quantity_multiplier = strtod(val, NULL);
				}
				else if (section_idx == 4)
				{
					if (!strcmp(key, "enhance.essence_drop.enabled"))
						enhance_essence_drop_enabled = atoi(val) ? 1 : 0;
					else if (!strcmp(key, "enhance.essence_drop.primary_roll_max") && atoi(val) > 0)
						enhance_essence_primary_roll_max = atoi(val);
					else if (!strcmp(key, "enhance.essence_drop.max_roll_max") && atoi(val) > 0)
						enhance_essence_max_roll_max = atoi(val);
					else if (!strcmp(key, "enhance.essence_drop.elite_level_multiplier") && atoi(val) > 0)
						enhance_essence_elite_level_multiplier = atoi(val);
					else if (!strcmp(key, "enhance.essence_drop.minimum_level") && atoi(val) >= 1 && atoi(val) <= 100)
						enhance_essence_minimum_level = atoi(val);
					else if (!strcmp(key, "enhance.essence_drop.maximum_level") && atoi(val) >= 1)
						enhance_essence_maximum_level = atoi(val);
				}
				else if (section_idx == 5 && !strcmp(key, "zone"))
				{
					/* [essence_drop_zone] — sparse per-zone essence drop overrides.
					 * Format: zone=<vzone>[:<primary_roll_max>[:<max_roll_max>[:<elite_mult>]]]
					 * Any numeric field omitted or 0 inherits the global [essence_drop] default.
					 * Examples:
					 *   zone=50              (zone 50, all globals)
					 *   zone=50:2000         (zone 50, primary=2000, rest global)
					 *   zone=50:2000:3000    (zone 50, primary=2000, max=3000, rest global)
					 *   zone=50:2000:3000:2  (zone 50, primary=2000, max=3000, elite_mult=2)
					 */
					int vzone, pval, mval, eval;
					char *colon, *rest;
					vzone = atoi(val);
					if (vzone <= 0 || enhance_essence_zone_rule_count >= ENHANCE_ESSENCE_MAX_ZONE_RULES)
						continue;
					pval = mval = eval = 0;  /* 0 = inherit global default */
					rest = val;
					colon = strchr(rest, ':');
					if (colon)
					{
						pval = atoi(colon + 1);
						rest = colon + 1;
						colon = strchr(rest, ':');
						if (colon)
						{
							mval = atoi(colon + 1);
							rest = colon + 1;
							colon = strchr(rest, ':');
							if (colon)
								eval = atoi(colon + 1);
						}
					}
					enhance_essence_zone_rules[enhance_essence_zone_rule_count].zone_number           = vzone;
					enhance_essence_zone_rules[enhance_essence_zone_rule_count].primary_roll_max       = pval;
					enhance_essence_zone_rules[enhance_essence_zone_rule_count].max_roll_max            = mval;
					enhance_essence_zone_rules[enhance_essence_zone_rule_count].elite_level_multiplier  = eval;
					enhance_essence_zone_rule_count++;
				}
				else if (section_idx == 2 && !strcmp(key, "zone"))
				{
					if (enhance_pool_excluded_zone_count < ENHANCE_MAX_POOL_EXCLUSIONS)
						enhance_pool_excluded_zones[enhance_pool_excluded_zone_count++] = atoi(val);
				}
				else if (section_idx == 3 && !strcmp(key, "vnum"))
				{
					if (enhance_pool_excluded_vnum_count < ENHANCE_MAX_POOL_EXCLUSIONS)
						enhance_pool_excluded_vnums[enhance_pool_excluded_vnum_count++] = atoi(val);
				}
				else if (section_idx >= 10 && section_idx <= 14)
				{
					/* [bitvector] through [bitvector5] sections */
					/* Format: FLAG_NAME = ival, allowed */
					comma = strchr(val, ',');
					if (!comma)
						continue;

					*comma = '\0';
					/* Trim allowed value */
					p = comma + 1;
					while (*p == ' ' || *p == '	')
						p++;
					int allowed = atoi(p);

					/* Trim flag name from key */
					strncpy(flag_name, key, sizeof(flag_name) - 1);
					flag_name[sizeof(flag_name) - 1] = '\0';

					/* Look up flag name */
					int bit_section = section_idx - 10; /* 0=bitvector, 1=bitvector2, ... */
					for (i = 0; enhance_flag_lookup[i].name; i++)
					{
						if (enhance_flag_lookup[i].section == bit_section &&
						    !strcmp(flag_name, enhance_flag_lookup[i].name))
						{
							if (allowed)
							{
								switch (bit_section)
								{
									case 0: SET_BIT(enhance_allow_mask,  enhance_flag_lookup[i].bit); break;
									case 1: SET_BIT(enhance_allow_mask2, enhance_flag_lookup[i].bit); break;
									case 2: SET_BIT(enhance_allow_mask3, enhance_flag_lookup[i].bit); break;
									case 3: SET_BIT(enhance_allow_mask4, enhance_flag_lookup[i].bit); break;
									case 4: SET_BIT(enhance_allow_mask5, enhance_flag_lookup[i].bit); break;
								}
							}
							break;
						}
					}
				}
				/* ignore [enhance_stat] and [spell] sections for now */
			}

			fclose(fp);
			fprintf(stderr, "-- Loaded enhance config from lib/enhance.cfg\r\n");
			logit(LOG_STATUS, "Loaded enhance config from lib/enhance.cfg");
		}

		/* =============================================================================
		 *  is_enhance_banned() — Return TRUE if item has any AFF bit not in allow masks
		 * =============================================================================
		 */
		bool is_enhance_banned(P_obj item)
		{
			if (!item)
				return TRUE;

			/* If the item has any AFF bit set that is NOT in the allow mask, it's banned */
			if (item->bitvector  & ~enhance_allow_mask)  return TRUE;
			if (item->bitvector2 & ~enhance_allow_mask2) return TRUE;
			if (item->bitvector3 & ~enhance_allow_mask3) return TRUE;
			if (item->bitvector4 & ~enhance_allow_mask4) return TRUE;
			if (item->bitvector5 & ~enhance_allow_mask5) return TRUE;

			return FALSE;
		}

		/* Return the zone number owning an object's template vnum. */
		static int enhance_object_origin_zone(P_obj obj)
		{
			int zone;
			int vnum;

			if (!obj)
				return -1;
			vnum = OBJ_VNUM(obj);
			for (zone = top_of_zone_table; zone >= 0; zone--)
			{
				if (zone_table[zone].real_bottom >= 0 &&
				    world[zone_table[zone].real_bottom].number <= vnum)
					return zone_table[zone].number;
			}
			return -1;
		}

		/* Candidate-only exclusions; source and material validation stays affect-only. */
		static bool is_enhance_pool_banned(P_obj item)
		{
			int i;
			int vnum;
			int zone;

			if (is_enhance_banned(item))
				return TRUE;
			vnum = OBJ_VNUM(item);
			for (i = 0; i < enhance_pool_excluded_vnum_count; i++)
				if (enhance_pool_excluded_vnums[i] == vnum)
					return TRUE;
			zone = enhance_object_origin_zone(item);
			for (i = 0; i < enhance_pool_excluded_zone_count; i++)
				if (enhance_pool_excluded_zones[i] == zone)
					return TRUE;
			return FALSE;
		}

		/* =============================================================================
		 *  load_enhance_index() — Build ival and stat hash tables from vnum range
		 * =============================================================================
		 */
		void load_enhance_index(void)
		{
			int    vnum, ival, h, sh, j, count;
			P_obj  obj;
			struct enhance_index_entry *entry;

			fprintf(stderr, "-- Building enhance index (vnum %d to %d)...\r\n",
			        enhance_search_vnum_min, enhance_search_vnum_max);
			logit(LOG_STATUS, "Building enhance index (vnum %d to %d)...",
			      enhance_search_vnum_min, enhance_search_vnum_max);

			count = 0;

			for (vnum = enhance_search_vnum_min; vnum <= enhance_search_vnum_max; vnum++)
			{
				obj = read_object(vnum, VIRTUAL);
				if (!obj)
					continue;

				/* Validate with same criteria as enhance() */
				if (!IS_SET(obj->wear_flags, ITEM_TAKE) ||
				    IS_SET(obj->extra_flags, ITEM_ARTIFACT) ||
				    IS_SET(obj->extra_flags, ITEM_NOSELL) ||
				    IS_SET(obj->extra_flags, ITEM_NORENT) ||
				    IS_SET(obj->extra_flags, ITEM_NOSHOW) ||
				    IS_SET(obj->extra_flags, ITEM_TRANSIENT) ||
				    IS_OBJ_STAT2(obj, ITEM2_QUESTITEM))
				{
					extract_obj(obj);
					continue;
				}

				if (obj->type == ITEM_STAFF && obj->value[3] > 0)
				{
					extract_obj(obj);
					continue;
				}

				if (obj->type == ITEM_TREASURE || obj->type == ITEM_POTION ||
				    obj->type == ITEM_MONEY || obj->type == ITEM_KEY || obj->type == ITEM_WAND)
				{
					extract_obj(obj);
					continue;
				}

				if (is_enhance_pool_banned(obj))
				{
					extract_obj(obj);
					continue;
				}

				ival = itemvalue(obj);

				/* Allocate and fill index entry */
				entry = (struct enhance_index_entry *)malloc(sizeof(struct enhance_index_entry));
				if (!entry)
				{
					extract_obj(obj);
					continue;
				}

				entry->vnum       = vnum;
				entry->ival       = ival;
				entry->wear_flags = obj->wear_flags;
				entry->material   = obj->material;

				for (j = 0; j < MAX_OBJ_AFFECT; j++)
				{
					entry->apply_loc[j] = obj->affected[j].location;
					entry->apply_mod[j] = obj->affected[j].modifier;
				}

				/* Insert into ival hash table */
				h = enhance_hash(ival);
				entry->next = enhance_ival_table[h];
				enhance_ival_table[h] = entry;

				/* Insert into stat hash table (by each apply location that has a modifier) */
				for (j = 0; j < MAX_OBJ_AFFECT; j++)
				{
					if (obj->affected[j].location != APPLY_NONE && obj->affected[j].modifier != 0)
					{
						/* Allocate a separate entry for stat table */
						struct enhance_index_entry *stat_entry;
						stat_entry = (struct enhance_index_entry *)malloc(sizeof(struct enhance_index_entry));
						if (!stat_entry)
							continue;

						memcpy(stat_entry, entry, sizeof(struct enhance_index_entry));
						sh = enhance_stat_hash(obj->affected[j].location);
						stat_entry->next = enhance_stat_table[sh];
						enhance_stat_table[sh] = stat_entry;
					}
				}

				extract_obj(obj);
				count++;
			}

			fprintf(stderr, "-- Enhance index built: %d entries indexed\r\n", count);
			logit(LOG_STATUS, "Enhance index built: %d entries indexed", count);
			}

			/* Enhancement lifecycle/event boundary. Other gameplay systems call these
			* hooks, but enhancement configuration and reward-selection policy remains
			* owned by this module. */
			void boot_enhancement_system(void)
			{
			load_enhance_config();
			load_enhance_index();
			}

			void enhance_on_eligible_npc_death(P_char ch, P_char killer)
			{
			if (!ch || !IS_NPC(ch) || IS_PC_PET(ch) || GET_EXP(ch) <= 0)
			return;

			enhance_load_essence_drop(ch, killer);
			}

			void enhance_on_npc_item_reset_skipped(P_char mob, P_obj missing_item)
			{
			P_obj material;
			int   high_vnum;

			if (!enhance_stat_enabled || !enhance_stat_npc_material_fallback_enabled || !mob || !IS_NPC(mob) || !missing_item)
			return;

			high_vnum = get_matstart(missing_item) + 4;
			if (!(material = read_object(high_vnum, VIRTUAL)))
			{
			logit(LOG_DEBUG,
			      "enhance: missing high-quality material %d for skipped NPC item %d.",
			      high_vnum,
			      OBJ_VNUM(missing_item));
			return;
			}

			obj_to_char(material, mob);
			}
