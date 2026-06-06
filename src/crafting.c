/*
 * Modern Craft and Forge shared calculations.
 * Command ownership moves here in subsequent extraction slices; the plan is
 * already shared so preview and execution cannot drift on material costs.
 */
#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "config.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "objmisc.h"
#include "spells.h"
#include "vnum.obj.h"
#include "crafting.h"
#include "utils.h"
#include "sql_player.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int crafting_level_gate = 3;
static int crafting_recipe_max_player_level = MAXLVLMORTAL;
static int crafting_experience_per_item_value = 1000;
static double crafting_material_quantity_multiplier = 1.0;
static bool crafting_recipe_examine_materials = TRUE;
static bool crafting_recipe_display_vnums = TRUE;
static bool crafting_craft_enabled = TRUE;
static bool crafting_forge_enabled = TRUE;
static int crafting_craft_essence_vnum = VOBJ_CRAFTING_ESSENCE;
static int crafting_craft_tool_vnum = VOBJ_CRAFTING_TOOLS;
static int crafting_forge_essence_vnum = VOBJ_FORGING_ESSENCE;
static int crafting_forge_tool_vnum = VOBJ_FORGING_FLUX;
static int crafting_scientific_tools_vnum_value = VOBJ_EPIC_LANTAN_TOOLS;
static bool crafting_scientific_tools_prevent_breakage_value = TRUE;
static int crafting_scientific_tools_recipe_roll_divisor_value = 15;
static int crafting_scientific_tools_recipe_player_multiplier_value = 2;
static double crafting_salvage_essence_luck_multiplier_value = 1.0;
static double crafting_salvage_essence_chance_multiplier_value = 1.0;

static void crafting_validate_content_vnum(int *configured_vnum, int default_vnum, const char *key)
{
	if (real_object(*configured_vnum) >= 0)
		return;
	logit(LOG_STATUS, "WARNING: %s vnum %d does not exist; using default %d.", key, *configured_vnum, default_vnum);
	*configured_vnum = default_vnum;
}

static void load_crafting_config(void)
{
	FILE *fp;
	char line[512];
	char *key, *value, *equals, *end;

	/* A restart/boot always starts from historical defaults. */
	crafting_level_gate = 3;
	crafting_recipe_max_player_level = MAXLVLMORTAL;
	crafting_experience_per_item_value = 1000;
	crafting_material_quantity_multiplier = 1.0;
	crafting_recipe_examine_materials = TRUE;
	crafting_recipe_display_vnums = TRUE;
	crafting_craft_enabled = TRUE;
	crafting_forge_enabled = TRUE;
	crafting_craft_essence_vnum = VOBJ_CRAFTING_ESSENCE;
	crafting_craft_tool_vnum = VOBJ_CRAFTING_TOOLS;
	crafting_forge_essence_vnum = VOBJ_FORGING_ESSENCE;
	crafting_forge_tool_vnum = VOBJ_FORGING_FLUX;
	crafting_scientific_tools_vnum_value = VOBJ_EPIC_LANTAN_TOOLS;
	crafting_scientific_tools_prevent_breakage_value = TRUE;
	crafting_scientific_tools_recipe_roll_divisor_value = 15;
	crafting_scientific_tools_recipe_player_multiplier_value = 2;
	crafting_salvage_essence_luck_multiplier_value = 1.0;
	crafting_salvage_essence_chance_multiplier_value = 1.0;

	fp = fopen("lib/crafting.cfg", "r");
	if (fp == NULL)
	{
		fprintf(stderr, "WARNING: Cannot open lib/crafting.cfg — using defaults.\r\n");
		logit(LOG_STATUS, "WARNING: Cannot open lib/crafting.cfg — using defaults.");
		return;
	}

	while (fgets(line, sizeof(line), fp) != NULL)
	{
		key = line;
		while (*key == ' ' || *key == '\t') key++;
		if (*key == '\0' || *key == '\n' || *key == '#') continue;
		equals = strchr(key, '=');
		if (equals == NULL) continue;
		*equals = '\0';
		end = equals - 1;
		while (end >= key && (*end == ' ' || *end == '\t')) *end-- = '\0';
		value = equals + 1;
		while (*value == ' ' || *value == '\t') value++;
		end = value + strlen(value);
		while (end > value && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';

		if (!strcmp(key, "crafting.level.gate.multiplier") && atoi(value) > 0)
			crafting_level_gate = atoi(value);
		else if (!strcmp(key, "crafting.recipe.max.player.level") && atoi(value) > 0 && atoi(value) <= MAXLVLMORTAL)
			crafting_recipe_max_player_level = atoi(value);
		else if (!strcmp(key, "crafting.recipe.examine.materials"))
			crafting_recipe_examine_materials = atoi(value) ? TRUE : FALSE;
		else if (!strcmp(key, "crafting.recipe.display.vnums"))
			crafting_recipe_display_vnums = atoi(value) ? TRUE : FALSE;
		else if (!strcmp(key, "crafting.experience.per.ival") && atoi(value) >= 0)
			crafting_experience_per_item_value = atoi(value);
		else if (!strcmp(key, "crafting.material.quantity.multiplier") && strtod(value, NULL) > 0.0)
			crafting_material_quantity_multiplier = strtod(value, NULL);
		else if (!strcmp(key, "crafting.craft.enabled"))
			crafting_craft_enabled = atoi(value) ? TRUE : FALSE;
		else if (!strcmp(key, "crafting.forge.enabled"))
			crafting_forge_enabled = atoi(value) ? TRUE : FALSE;
		else if (!strcmp(key, "crafting.craft.essence.vnum") && atoi(value) > 0)
			crafting_craft_essence_vnum = atoi(value);
		else if (!strcmp(key, "crafting.craft.tool.vnum") && atoi(value) > 0)
			crafting_craft_tool_vnum = atoi(value);
		else if (!strcmp(key, "crafting.forge.essence.vnum") && atoi(value) > 0)
			crafting_forge_essence_vnum = atoi(value);
		else if (!strcmp(key, "crafting.forge.tool.vnum") && atoi(value) > 0)
			crafting_forge_tool_vnum = atoi(value);
		else if (!strcmp(key, "crafting.salvage.scientific.tools.vnum") && atoi(value) > 0)
			crafting_scientific_tools_vnum_value = atoi(value);
		else if (!strcmp(key, "crafting.salvage.scientific.tools.prevent.breakage"))
			crafting_scientific_tools_prevent_breakage_value = atoi(value) ? TRUE : FALSE;
		else if (!strcmp(key, "crafting.salvage.scientific.tools.recipe.roll.divisor") && atoi(value) >= 1)
			crafting_scientific_tools_recipe_roll_divisor_value = atoi(value);
		else if (!strcmp(key, "crafting.salvage.scientific.tools.recipe.player.multiplier") && atoi(value) >= 1)
			crafting_scientific_tools_recipe_player_multiplier_value = atoi(value);
		else if (!strcmp(key, "crafting.salvage.essence.luck.multiplier") && strtod(value, NULL) > 0.0)
			crafting_salvage_essence_luck_multiplier_value = strtod(value, NULL);
		else if (!strcmp(key, "crafting.salvage.essence.chance.multiplier") && strtod(value, NULL) >= 0.0)
			crafting_salvage_essence_chance_multiplier_value = strtod(value, NULL);
	}
	fclose(fp);

	/* Content references are configurable, but invalid entries must not turn a
	 * recipe into an unfulfillable or silently free transaction. */
	crafting_validate_content_vnum(&crafting_craft_essence_vnum, VOBJ_CRAFTING_ESSENCE, "crafting.craft.essence.vnum");
	crafting_validate_content_vnum(&crafting_craft_tool_vnum, VOBJ_CRAFTING_TOOLS, "crafting.craft.tool.vnum");
	crafting_validate_content_vnum(&crafting_forge_essence_vnum, VOBJ_FORGING_ESSENCE, "crafting.forge.essence.vnum");
	crafting_validate_content_vnum(&crafting_forge_tool_vnum, VOBJ_FORGING_FLUX, "crafting.forge.tool.vnum");
	crafting_validate_content_vnum(&crafting_scientific_tools_vnum_value, VOBJ_EPIC_LANTAN_TOOLS, "crafting.salvage.scientific.tools.vnum");
}

int crafting_scientific_tools_vnum(void) { return crafting_scientific_tools_vnum_value; }
bool crafting_scientific_tools_prevent_breakage(void) { return crafting_scientific_tools_prevent_breakage_value; }
int crafting_scientific_tools_recipe_roll_divisor(void) { return crafting_scientific_tools_recipe_roll_divisor_value; }
int crafting_scientific_tools_recipe_player_multiplier(void) { return crafting_scientific_tools_recipe_player_multiplier_value; }
double crafting_salvage_essence_luck_multiplier(void) { return crafting_salvage_essence_luck_multiplier_value; }
double crafting_salvage_essence_chance_multiplier(void) { return crafting_salvage_essence_chance_multiplier_value; }

/* Any fifth-bitvector affect makes a recipe magical. */
bool has_affect(P_obj obj)
{

	if (IS_SET(obj->bitvector, AFF_STONE_SKIN) || IS_SET(obj->bitvector, AFF_HIDE) || IS_SET(obj->bitvector, AFF_SNEAK) || IS_SET(obj->bitvector, AFF_FLY) || IS_SET(obj->bitvector, AFF4_NOFEAR) ||
	    IS_SET(obj->bitvector2, AFF2_AIR_AURA) || IS_SET(obj->bitvector2, AFF2_EARTH_AURA) || IS_SET(obj->bitvector3, AFF3_INERTIAL_BARRIER) || IS_SET(obj->bitvector3, AFF3_REDUCE) ||
	    IS_SET(obj->bitvector2, AFF2_GLOBE) || IS_SET(obj->bitvector, AFF_HASTE) || IS_SET(obj->bitvector, AFF_DETECT_INVISIBLE) || IS_SET(obj->bitvector4, AFF4_DETECT_ILLUSION) ||
	    obj->bitvector5)
	{
		return TRUE;
	}
	return FALSE;
}

bool crafting_build_plan(P_obj item, struct crafting_plan *plan)
{
	int item_value;
	int low_material_vnum;
	int high_material_count;

	if (item == NULL || plan == NULL)
	{
		return FALSE;
	}

	item_value = itemvalue(item);
	low_material_vnum = get_matstart(item);
	if (item_value < 1 || low_material_vnum <= 0)
	{
		return FALSE;
	}

	high_material_count = (item_value + 4) / 5;
	plan->item_value = item_value;
	plan->low_material_vnum = low_material_vnum;
	plan->high_material_vnum = low_material_vnum + 4;
	plan->high_material_count = (int)ceil(high_material_count * crafting_material_quantity_multiplier);
	plan->low_material_count = (int)ceil(((item_value + 4) - high_material_count * 5) * crafting_material_quantity_multiplier);
	plan->magical = has_affect(item);
	return TRUE;
}

bool crafting_validate_recipe_target(P_obj item)
{
	struct crafting_plan plan;

	return item != NULL && !IS_OBJ_STAT2(item, ITEM2_QUESTITEM) && is_salvageable(item) && crafting_build_plan(item, &plan);
}

bool crafting_recipe_target_is_available(P_obj item)
{
	struct crafting_plan plan;

	if (!crafting_validate_recipe_target(item) ||
	    (!crafting_craft_enabled && !crafting_forge_enabled) ||
	    !crafting_build_plan(item, &plan))
		return FALSE;
	return plan.item_value <= crafting_level_gate * crafting_recipe_max_player_level;
}

void crafting_configure_recipe_scroll(P_obj recipe, P_obj target)
{
	struct crafting_plan plan;
	P_obj low, high;
	char text[MAX_STRING_LENGTH];

	if (!crafting_recipe_examine_materials || recipe == NULL || !crafting_build_plan(target, &plan))
		return;
	low = read_object(plan.low_material_vnum, VIRTUAL);
	high = read_object(plan.high_material_vnum, VIRTUAL);
	if (!low || !high)
	{
		if (low) extract_obj(low);
		if (high) extract_obj(high);
		return;
	}
	snprintf(text, sizeof(text), "This recipe teaches %s.\r\n\r\nRequired materials:\r\n  %d %s\r\n  %d %s\r\n",
	         target->short_description, plan.high_material_count, high->short_description,
	         plan.low_material_count, low->short_description);
	if (plan.magical)
	{
		char requirement[160];
		snprintf(requirement, sizeof(requirement), "  1 magical essence (required for magical items)\r\n");
		strncat(text, requirement, sizeof(text) - strlen(text) - 1);
	}
	strncat(text, "\r\nDiscipline consumables (in addition to the materials above):\r\n", sizeof(text) - strlen(text) - 1);
	{
		char requirement[256];
		snprintf(requirement, sizeof(requirement),
		         "  Craft: 1 gnomish crafting tool box (configured item vnum %d)\r\n"
		         "  Forge: 1 blacksmithing flux (configured item vnum %d)\r\n",
		         crafting_craft_tool_vnum, crafting_forge_tool_vnum);
		strncat(text, requirement, sizeof(text) - strlen(text) - 1);
	}
	strncat(text, "\r\nUse the recipe to learn it, then `craft` or `forge` to view and make it.\r\n", sizeof(text) - strlen(text) - 1);
	if ((recipe->str_mask & STRUNG_DESC3) && recipe->action_description)
		FREE(recipe->action_description);
	recipe->action_description = str_dup(text);
	recipe->str_mask |= STRUNG_DESC3;
	extract_obj(low);
	extract_obj(high);
}

void boot_crafting_system(void)
{
	load_crafting_config();
}

int crafting_level_gate_multiplier(void)
{
	return crafting_level_gate;
}

int crafting_experience_per_ival(void)
{
	return crafting_experience_per_item_value;
}

bool crafting_mode_enabled(enum crafting_mode mode)
{
	return mode == CRAFTING_MODE_CRAFT ? crafting_craft_enabled : crafting_forge_enabled;
}

int crafting_essence_vnum(enum crafting_mode mode)
{
	return mode == CRAFTING_MODE_CRAFT ? crafting_craft_essence_vnum : crafting_forge_essence_vnum;
}

int crafting_tool_vnum(enum crafting_mode mode)
{
	return mode == CRAFTING_MODE_CRAFT ? crafting_craft_tool_vnum : crafting_forge_tool_vnum;
}

void crafting_examine_support_item(P_char ch, P_obj item)
{
	if (!ch || !item)
		return;
	if (OBJ_VNUM(item) == crafting_tool_vnum(CRAFTING_MODE_FORGE) && GET_CHAR_SKILL(ch, SKILL_FORGE))
		send_to_char("&+yForge insight:&n This flux is consumed to bind the materials when you forge a recipe. Use `forge info <number>` to see what else you need.\r\n", ch);
	else if (OBJ_VNUM(item) == crafting_tool_vnum(CRAFTING_MODE_CRAFT) && GET_CHAR_SKILL(ch, SKILL_CRAFT))
		send_to_char("&+yCraft insight:&n This tool box is consumed while shaping a recipe. Use `craft info <number>` to see what else you need.\r\n", ch);
}

/* SQL is canonical. The legacy file is read only when a player has no SQL
 * recipes, and each valid entry is imported through the idempotent SQL API. */
int *crafting_get_player_recipes(P_char ch, int *count)
{
	int *recipes;
	int recipe_vnum;
	int capacity = 0;
	FILE *fp;
	char name[256];
	char path[512];
	char *p;

	if (count == NULL)
		return NULL;
	*count = 0;
	if (ch == NULL)
		return NULL;

	recipes = sql_get_player_recipes(GET_PID(ch), count);
	if (recipes != NULL || *count != 0)
	{
		int i, kept = 0;
		for (i = 0; i < *count; i++)
		{
			P_obj target = read_object(recipes[i], VIRTUAL);
			if (target && crafting_recipe_target_is_available(target))
				recipes[kept++] = recipes[i];
			if (target)
				extract_obj(target);
		}
		*count = kept;
		return recipes;
	}

	snprintf(name, sizeof(name), "%s", GET_NAME(ch));
	for (p = name; *p; p++)
		*p = LOWER(*p);
	snprintf(path, sizeof(path), "Players/Tradeskills/%c/%s.crafting", name[0], name);
	fp = fopen(path, "r");
	if (fp == NULL)
		return NULL;

	while (fscanf(fp, "%d", &recipe_vnum) == 1)
	{
		int *grown;
		int i;
		bool duplicate = FALSE;
		P_obj target;
		if (recipe_vnum <= 0 || real_object(recipe_vnum) < 0)
			continue;
		target = read_object(recipe_vnum, VIRTUAL);
		if (!target || !crafting_recipe_target_is_available(target))
		{
			if (target) extract_obj(target);
			continue;
		}
		extract_obj(target);
		for (i = 0; i < *count; i++)
			if (recipes[i] == recipe_vnum)
				duplicate = TRUE;
		if (duplicate)
			continue;
		if (*count == capacity)
		{
			capacity = capacity ? capacity * 2 : 16;
			grown = (int *)realloc(recipes, sizeof(*recipes) * capacity);
			if (grown == NULL)
			{
				free(recipes);
				fclose(fp);
				*count = 0;
				return NULL;
			}
			recipes = grown;
		}
		recipes[(*count)++] = recipe_vnum;
		sql_add_player_recipe(GET_PID(ch), recipe_vnum);
	}
	fclose(fp);
	return recipes;
}

static void crafting_handle_craft_command(P_char ch, char *argument, int cmd);
static void crafting_handle_forge_command(P_char ch, char *argument, int cmd);

void crafting_handle_command(P_char ch, enum crafting_mode mode, char *argument)
{
	if (!crafting_mode_enabled(mode))
	{
		send_to_char("That crafting discipline is currently unavailable.\r\n", ch);
		return;
	}
	switch (mode)
	{
		case CRAFTING_MODE_CRAFT:
			crafting_handle_craft_command(ch, argument, CMD_CRAFT);
			return;
		case CRAFTING_MODE_FORGE:
			crafting_handle_forge_command(ch, argument, CMD_FORGE);
			return;
	}
}


/* Modern Craft command implementation, extracted from actnew.c. */
static void crafting_handle_craft_command(P_char ch, char *argument, int cmd)
{
	char  buf1[MAX_STRING_LENGTH];
	char  first[MAX_INPUT_LENGTH];
	char  second[MAX_INPUT_LENGTH];
	char  rest[MAX_INPUT_LENGTH];
	int   i      = 0;
	int   choice = 0;
	P_obj hammer, foundry;

	/***DISPLAYRECIPES STUFF***/
	char          tempdesc[MAX_INPUT_LENGTH];
	char          short_desc[MAX_STRING_LENGTH];
	char          keywords[MAX_INPUT_LENGTH];
	char          buffer[256];
	long          choice2;
	int           selected = 0;
	int          *recipes;
	int           recipe_count;
	P_obj         tobj;

	/* Dunno the difference between this and above?
	   P_obj    craft_obj1, craft_obj2, craft_obj3, obj;
	   P_obj    t_obj, nextobj;
	   int      i, bits, j, in_room, material_type, item_type, howmany,
	   weapon_types, slot;
	   bool     equipped;
	   P_char   victim = NULL;
	   char     Gbuf1[MAX_STRING_LENGTH];
	   char     Gbuf2[MAX_STRING_LENGTH];
	   char    *r_str;

	   equipped = FALSE;
	   obj = 0;
	   */

	if (!GET_CHAR_SKILL(ch, SKILL_CRAFT))
	{
		act("You do not know how to &+rcraft&n items.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	recipes = crafting_get_player_recipes(ch, &recipe_count);
	if (recipes == NULL || recipe_count == 0)
	{
		send_to_char("You do not know any Craft recipes yet. Learn a recipe, then type `craft` to list it.\r\n", ch);
		free(recipes);
		return;
	}

	// If no arguments, display syntax and list of known recipes.
	if (!argument || !*argument)
	{
		send_to_char("&+wcraft Syntax:\n&n", ch);
		send_to_char("&+wcraft or craft list       - list your recipes\n&n", ch);
		send_to_char("&+wcraft info <number>      - show costs and requirements\n&n", ch);
		send_to_char("&+wcraft stat <number>      - inspect the finished item\n&n", ch);
		send_to_char("&+wcraft make <number>      - consume requirements and create it\n&n", ch);
		send_to_char("&+yYou know the following recipes:\n&n", ch);
		send_to_char("----------------------------------------------------------------------------\n", ch);
		send_to_char("&+B#     Recipe vnum       &+MItem&n\n\r", ch);
		// Walk through each recipe and display its number and short description.
		for (i = 0; i < recipe_count; i++)
		{
			tobj = read_object(recipes[i], VIRTUAL);
			if (tobj == NULL)
			{
				logit(LOG_DEBUG, "do_craft: '%s' has bad recipe vnum %d.", J_NAME(ch), recipes[i]);
				continue;
			}
			if (crafting_recipe_display_vnums)
				snprintf(buffer, sizeof buffer, "   &+W%-4d  %-18d&n%s&n\n", i + 1, recipes[i], tobj->short_description);
			else
				snprintf(buffer, sizeof buffer, "   &+W%-4d  &n%s&n\n", i + 1, tobj->short_description);
			page_string(ch->desc, buffer, 1);
			send_to_char("----------------------------------------------------------------------------\n", ch);
			extract_obj(tobj);
		}
		free(recipes);
		return;
	}

	// If no argument, no sense in chopping it up, so moved this past the !arg check.
	half_chop(argument, first, rest);
	half_chop(rest, second, rest);
	if (is_abbrev(first, "list"))
	{
		free(recipes);
		crafting_handle_craft_command(ch, "", cmd);
		return;
	}
	choice2 = atoi(second);

	// Accept either the stable recipe vnum or the displayed list number.
	if (choice2)
	{
		for (i = 0; i < recipe_count; i++)
			if (recipes[i] == choice2)
			{
				selected = recipes[i];
				break;
			}
		if (selected == 0 && choice2 <= recipe_count)
			selected = recipes[choice2 - 1];
	}
	free(recipes);

	// If they picked a recipe that wasn't in their book...
	if (choice2 != 0 && selected == 0)
	{
		send_to_char("You dont appear to have that &+Wrecipe&n in your list.&n\n", ch);
		return;
	}

	if (is_abbrev(first, "stat"))
	{
		if (choice2 == 0)
		{
			send_to_char("What &+Wrecipe&n would you like &+ystatistics&n about?\n", ch);
			return;
		}
		tobj = read_object(selected, VIRTUAL);
		send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
		spell_identify(GET_LEVEL(ch), ch, 0, 0, 0, tobj);
		extract_obj(tobj);
		return;
	}
	else if (is_abbrev(first, "info"))
	{
		if (choice2 == 0)
		{
			send_to_char("What &+Wrecipe&n would you like &+yinformation&n about?\n", ch);
			return;
		}

		tobj = read_object(selected, VIRTUAL);
		if (!tobj)
		{
			debug("Couldn't load object vnum %d.", selected);
			send_to_char("Couldn't create the object. :(\n\r", ch);
			return;
		}

		struct crafting_plan plan;
		if (!crafting_build_plan(tobj, &plan))
		{
			send_to_char("Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch);
			debug("Couldn't build crafting plan for object: '%s' %d.", tobj->short_description, selected);
			extract_obj(tobj);
			return;
		}
		int iVal                   = plan.item_value;
		int lowQualityMaterialVnum = plan.low_material_vnum;

		if (lowQualityMaterialVnum <= 0)
		{
			send_to_char("Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch);
			debug("Couldn't get start material for object: '%s' %d.", tobj->short_description, selected);
			extract_obj(tobj);
			return;
		}

		int numHighest = plan.high_material_count;
		int numLowest = plan.low_material_count;

		P_obj matLowest, matHighest;

		matLowest  = read_object(lowQualityMaterialVnum, VIRTUAL);
		matHighest = read_object(lowQualityMaterialVnum + 4, VIRTUAL);

		if (matLowest == NULL || matHighest == NULL)
		{
			send_to_char("Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch);
			debug("Couldn't load a material: matLowest(%s) or matHighest(%s) for object '%s' %d.",
			      (matLowest == NULL) ? "NULL" : matLowest->short_description,
			      (matHighest == NULL) ? "NULL" : matHighest->short_description,
			      tobj->short_description,
			      selected);
			extract_obj(tobj);
			return;
		}

		if (numLowest == 0)
		{
			send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
			snprintf(buf1, sizeof buf1, "To craft this item, you will need %d of %s.\r\n&n", numHighest, matHighest->short_description);
			page_string(ch->desc, buf1, 1);
		}
		else
		{
			send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
			snprintf(buf1, sizeof buf1, "To craft this item, you will need %d of %s and %d of %s.\r\n&n", numHighest, matHighest->short_description, numLowest, matLowest->short_description);
			page_string(ch->desc, buf1, 1);
		}

		/* It will never happen that we don't need at least one of matHighest since min itemvalue is 1,
		 *   then we add 4 (total 5) and divide by 5 (result minimum 1).
		 * But, in case we change that, I'm leaving the code here. 2/10/2015
		      send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
		      snprintf(buf1, sizeof buf1, "To craft this item, you will need %d of %s.\r\n&n", numLowest, matLowest->short_description);
		      page_string(ch->desc, buf1, 1);
		*/

		if (has_affect(tobj))
		{
			send_to_char("...as well as &+W1 &nof &+ma &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml &+messence&n due to the &+mmagical &nproperties this item possesses.\r\n", ch);
		}
		snprintf(buf1, sizeof buf1, "You will also need one gnomish crafting tool box; it is consumed as you work.\r\n");
		send_to_char(buf1, ch);

		// It's safe to assume tobj exists since we checked after the read_object call.
		extract_obj(tobj);
		// The same follows for matLowest and matHighest.
		extract_obj(matLowest);
		extract_obj(matHighest);
		return;
	}
	else if (is_abbrev(first, "make"))
	{
		if (choice2 == 0)
		{
			send_to_char("What &+Witem&n are you attempting to craft?\n", ch);
			return;
		}

		tobj = read_object(selected, VIRTUAL);
		if (!tobj)
		{
			debug("Couldn't load object vnum %d.", selected);
			send_to_char("Couldn't create the object. :(\n\r", ch);
			return;
		}

		struct crafting_plan plan;
		if (!crafting_build_plan(tobj, &plan))
		{
			send_to_char("Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch);
			debug("Couldn't build crafting plan for object: '%s' %d.", tobj->short_description, selected);
			extract_obj(tobj);
			return;
		}
		int iVal = plan.item_value;
		if (iVal > GET_LEVEL(ch) * crafting_level_gate_multiplier() || IS_OBJ_STAT2(tobj, ITEM2_QUESTITEM))
		{
			if (IS_OBJ_STAT2(tobj, ITEM2_QUESTITEM))
				send_to_char("This is a quest item and cannot be crafted from a player recipe.\r\n", ch);
			else
			{
				snprintf(buf1, sizeof buf1, "You need level %d to craft this recipe (you are level %d).\r\n", (iVal + crafting_level_gate_multiplier() - 1) / crafting_level_gate_multiplier(), GET_LEVEL(ch));
				send_to_char(buf1, ch);
			}
			extract_obj(tobj);
			return;
		}

		int lowQualityMaterialVnum  = plan.low_material_vnum;
		int highQualityMaterialVnum = plan.high_material_vnum;
		int numHighest = plan.high_material_count;
		int numLowest = plan.low_material_count;

		P_obj matLowest, matHighest;

		matLowest  = read_object(lowQualityMaterialVnum, VIRTUAL);
		matHighest = read_object(highQualityMaterialVnum, VIRTUAL);

		if (matLowest == NULL || matHighest == NULL)
		{
			send_to_char("Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch);
			debug("Couldn't load a material: matLowest(%s) or matHighest(%s) for object '%s' %d.",
			      (matLowest == NULL) ? "NULL" : matLowest->short_description,
			      (matHighest == NULL) ? "NULL" : matHighest->short_description,
			      tobj->short_description,
			      selected);
			extract_obj(tobj);
			return;
		}

		// If we're going to require multiple essences, we need to change this to int numAffects ...
		bool hasAffect = has_affect(tobj);

		int invLowMats  = 0; // Number of lowest quality materials in inventory.
		int invHighMats = 0; // Number of highest quality materials in inventory.
		int invEssences = 0; // Number of magical essences in inventory.
		int invTools    = 0; // Number of crafting tools in inventory
		int invVnum     = 0; // Vnum of the current inventory object.
		for (P_obj inventory = ch->carrying; inventory; inventory = inventory->next_content)
		{
			invVnum = OBJ_VNUM(inventory);

			if (invVnum == lowQualityMaterialVnum)
				invLowMats++;
			else if (invVnum == highQualityMaterialVnum)
				invHighMats++;
			else if (invVnum == crafting_essence_vnum(CRAFTING_MODE_CRAFT))
				invEssences++;
			else if (invVnum == crafting_tool_vnum(CRAFTING_MODE_CRAFT))
				invTools++;
		}

		// Check mats in inventory vs mats needed.
		if (invLowMats < numLowest || invHighMats < numHighest)
		{
			send_to_char("You do not have the required &+ysalvaged &+Ymaterials &nin your inventory.\r\n", ch);
			extract_obj(tobj);
			extract_obj(matLowest);
			extract_obj(matHighest);
			return;
		}
		// If for some reason we want more than 1 box of tools, change the 1 below.
		if (invTools < 1)
		{
			send_to_char("You must have &+ma &+ybox &+mof &+Rgnomish &+rcrafting &+mtools&n to create your item.\r\n", ch);
			extract_obj(tobj);
			extract_obj(matLowest);
			extract_obj(matHighest);
			return;
		}
		// If we're going to require multiple essences, need to edit this if statement.
		if (invEssences < 1 && hasAffect)
		{
			send_to_char("You must have &+W1 &nof &+ma &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml &+messence&n due to the &+mmagical &nproperties this item possesses.\r\n", ch);
			extract_obj(tobj);
			extract_obj(matLowest);
			extract_obj(matHighest);
			return;
		}

		// Remove the materials from inventory...  Since we are changing the inventory
		//   list, we need to use nextObj instead of just going to inventory->next_content.
		P_obj nextObj;
		// If for some reason we want more than 1 box of tools, change gotTools from boolean to int...
		//   For right now, gotTools -> TRUE when we extract a set of tools from inventory.
		bool gotTools = FALSE;
		for (P_obj inventory = ch->carrying; inventory; inventory = nextObj)
		{
			nextObj = inventory->next_content;
			invVnum = OBJ_VNUM(inventory);

			if ((numLowest > 0) && (invVnum == lowQualityMaterialVnum))
			{
				obj_from_char(inventory);
				extract_obj(inventory);
				numLowest--;
			}
			else if ((numHighest > 0) && (invVnum == highQualityMaterialVnum))
			{
				obj_from_char(inventory);
				extract_obj(inventory);
				numHighest--;
			}
			// If we're requiring multiple essences, need to change this if clause.
			else if (hasAffect && (invVnum == crafting_essence_vnum(CRAFTING_MODE_CRAFT)))
			{
				obj_from_char(inventory);
				extract_obj(inventory);
				hasAffect = FALSE;
			}
			// If we're requiring multiple tools, need to change this if clause.
			else if (!gotTools && (invVnum == crafting_tool_vnum(CRAFTING_MODE_CRAFT)))
			{
				obj_from_char(inventory);
				extract_obj(inventory);
				gotTools = TRUE;
			}
		}

		notch_skill(ch, SKILL_CRAFT, 50);

		SET_BIT(tobj->extra2_flags, ITEM2_CRAFTED);
		SET_BIT(tobj->extra_flags, ITEM_NOREPAIR);
		REMOVE_BIT(tobj->extra_flags, ITEM_SECRET);
		snprintf(keywords, sizeof keywords, "%s %s tradeskill", tobj->name, GET_NAME(ch));

		snprintf(tempdesc, sizeof tempdesc, "%s", tobj->short_description);
		snprintf(short_desc, sizeof short_desc, "%s &+ymade by&n &+r%s&n", tempdesc, GET_NAME(ch));
		set_keywords(tobj, keywords);
		set_short_description(tobj, short_desc);

		// Rewards here: tobj is the crafted item, so no need to load another.
		wizlog(56, "%s crafted '%s' (%d) ival %d.", GET_NAME(ch), tobj->short_description, selected, itemvalue(tobj));

		obj_to_char(tobj, ch);
		act("&+W$n &+Ldelicately opens their &+ybox &+mof &+Rgnomish &+rcrafting &+mtools&+L and starts their work...\r\n"
		    "&+W$n &+Lremoves the &+Wim&+wpur&+Lities &+Lfrom their &+ymaterials &+Land gently assembles a masterpiece...\r\n"
		    "&+L...hands shaking, &+W$n &+Lraises their head and &+Ysmiles&+L, admiring their new $p.&N",
		    TRUE,
		    ch,
		    tobj,
		    0,
		    TO_ROOM);
		act("You &+Ldelicately open your &+ybox &+mof &+Rgnomish &+rcrafting &+mtools&+L and get to work...\r\n"
		    "you &+Lremove the &+Wim&+wpur&+Lities &+Lfrom your &+ymaterials &+Land gently assemble a masterpiece...\r\n"
		    "&+L...hands shaking, &+Wyou &+Lraise your head and &+Ysmile&+L, admiring your new $p.&N",
		    FALSE,
		    ch,
		    tobj,
		    0,
		    TO_CHAR);

		gain_exp(ch, NULL, iVal * crafting_experience_per_ival(), EXP_BOON);
		extract_obj(matLowest);
		extract_obj(matHighest);
		// Save the character! 1 -> in game.
	if (!do_save_silent(ch, 1))
		logit(LOG_DEBUG, "Failed to save %s after heroics reward.", GET_NAME(ch));
	}
	else
	{
		send_to_char("Unknown Craft command. Use `craft` to list recipes and available commands.\r\n", ch);
	}

	/*
	   argument = one_argument(argument, Gbuf1);
	   howmany = 0;
	   i = 0;
	   craft_obj1 = 0;
	   craft_obj2 = 0;
	   craft_obj3 = 0;

	   for (t_obj = ch->carrying; t_obj; t_obj = nextobj)
	   {
	   nextobj = t_obj->next_content;


	   if (isname("piece", t_obj->name) && !isname("tail", t_obj->name) &&
	   obj_index[t_obj->R_num].virtual_number == RANDOM_EQ_VNUM)
	   {
	   i++;

	   if (i == 1)
	   {
	   craft_obj1 = t_obj;
	   }
	   if (i == 2)
	   {
	   craft_obj2 = t_obj;
	   }
	   if (i == 3)
	   {
	   craft_obj3 = t_obj;
	   }
	   }

	   }
	   slot = -1;

	   while (*argument == ' ')
	   argument++;


	   for (i = 0; i < MAX_SLOT; i++)
	   {
	   snprintf(Gbuf2, MAX_STRING_LENGTH, "%s\0", strip_ansi(slot_data[i].m_name).c_str());
	   if (!strcmp(argument, Gbuf2))
	   {
	   howmany = slot_data[i].numb_material;
	   slot = i;
	   break;
	   }
	   }

	   if (slot == -1 || howmany > 3)
	   {
	   act("You don't know how to create that...", FALSE, ch, 0, 0, TO_CHAR);
	   return;
	   }
	   if (craft_obj1 == 0)
	   {
	   act("You do not have enough material, need to have a piece in inventory",
	   FALSE, ch, 0, 0, TO_CHAR);
	   return;
	   }
	   if (craft_obj2 == 0 && howmany == 2)
	   {
	   act
	   ("You do not have enough material, need to have two pieces in inventory.",
	   FALSE, ch, 0, 0, TO_CHAR);
	   return;
	   }
	   if (craft_obj3 == 0 && howmany == 3)
	   {
	   act
	("You do not have enough material need to have three pieces in inventory.",
	 FALSE, ch, 0, 0, TO_CHAR);
	return;
  }


  if (howmany == 2)
  {
	if (craft_obj1->material != craft_obj2->material)
	{
	  act("Mixing materials like that might be dangerous.", FALSE, ch, 0, 0,
	      TO_CHAR);
	  return;
	}
  }

  if (howmany == 3)
  {
	if (craft_obj1->material != craft_obj2->material ||
	    craft_obj1->material != craft_obj3->material)
	{
	  act("Mixing materials like that might be dangerous.", FALSE, ch, 0, 0,
	      TO_CHAR);
	  return;
	}
  }

  for (i = 0; i <= MAXMATERIAL; i++)
  {
	if (material_data[i].m_number == craft_obj1->material)
	{
	  material_type = i;
	}
  }


  */
	/*        wizlog(56, "%s crafted.", GET_NAME(ch));

	          if (howmany > 0)
	          extract_obj(craft_obj1);
	          if (howmany > 1)
	          extract_obj(craft_obj2);
	          if (howmany > 2)
	          extract_obj(craft_obj3);
	          obj = create_random_eq_new(ch, ch, slot, material_type);
	          if (!obj)
	          {
	          act("&+yCrafting NOT COMPLETED CONTACT A GOD!&n", FALSE, ch, 0, 0,
	          TO_CHAR);
	          return;
	          }
	          obj_to_char(obj, ch);
	//notch_skill(ch, SKILL_CRAFT, 30);
	act("&+W$n crafted a $q&N.", TRUE, ch, obj, 0, TO_NOTVICT);
	act("&+WYou crafted a $q&N", TRUE, ch, obj, 0, TO_CHAR);

	return;
	*/
}

/* Modern Forge command implementation, extracted from tradeskill.c. */
static void crafting_handle_forge_command(P_char ch, char *argument, int cmd)
{
	int   skillLevel, objVnum, recipeNumber, commandType, iVal, invVnum;
	int   numHighQuality, numLowQuality, lowQualityMaterialVnum, highQualityMaterialVnum;
	int   invLowMats, invHighMats, invEssences;
	char  buf[256];
	char  recipe[256];
	char  Gbuf1[MAX_STRING_LENGTH];
	char  first[MAX_INPUT_LENGTH];
	char  second[MAX_INPUT_LENGTH];
	char  keywords[MAX_STRING_LENGTH];
	char  short_desc[MAX_STRING_LENGTH];
	char *rest;
	bool  hasAffect, hasFlux;
	P_obj obj, lowQualityMaterial, highQualityMaterial, inventory, invNextObj;

	if (!(skillLevel = GET_CHAR_SKILL(ch, SKILL_FORGE)))
	{
		act("You do not know how to &+Lforge&n.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	// load recipes from database
	int  recipe_count = 0;
	int *recipes      = sql_get_player_recipes(GET_PID(ch), &recipe_count);
	if (!recipes || recipe_count == 0)
	{
		send_to_char("You dont know any recipes yet.\r\n", ch);
		if (recipes)
			free(recipes);
		return;
	}
	rest = one_argument(argument, first);
	one_argument(rest, second);
	objVnum = atoi(second);

	// If we don't have a first argument.
	if (!*first)
	{
		commandType = 0;
	}
	else if (is_abbrev(first, "list"))
	{
		commandType = 0;
	}
	else if (is_abbrev(first, "info"))
	{
		commandType = 1;
	}
	else if (is_abbrev(first, "stat"))
	{
		commandType = 2;
	}
	else if (is_abbrev(first, "make"))
	{
		commandType = 3;
	}
	// The first argument is invalid.
	else
	{
		commandType = -1;
	}
	if (commandType == -1)
	{
		send_to_char("Unknown Forge command. Use `forge` to list recipes and available commands.\r\n", ch);
		free(recipes);
		return;
	}

	if (commandType == 0)
	{
		send_to_char("&+wForge Syntax:\n&n", ch);
		send_to_char("&+wforge or forge list       - list your recipes\n&n", ch);
		send_to_char("&+wforge info <number>      - show costs and requirements\n&n", ch);
		send_to_char("&+wforge stat <number>      - inspect the finished item\n&n", ch);
		send_to_char("&+wforge make <number>      - consume requirements and create it\n&n", ch);
		send_to_char("&+yYou know the following recipes:\n&n", ch);
		send_to_char("----------------------------------------------------------------------------\n", ch);
		send_to_char("&+B#     Recipe vnum       &+MItem&n\n\r", ch);

		for (int i = 0; i < recipe_count; i++)
		{
			if (!(obj = read_object(recipes[i], VIRTUAL)))
			{
				logit(LOG_DEBUG, "'%s' has bad recipe vnum %d.", ch ? J_NAME(ch) : "NULL", recipes[i]);
				continue;
			}
			if (crafting_recipe_display_vnums)
				snprintf(recipe, sizeof recipe, "   &+W%-4d  %-18d&n%s&n\n", i + 1, recipes[i], obj->short_description);
			else
				snprintf(recipe, sizeof recipe, "   &+W%-4d  &n%s&n\n", i + 1, obj->short_description);
			page_string(ch->desc, recipe, 1);
			send_to_char("----------------------------------------------------------------------------\n", ch);
			extract_obj(obj);
		}
		free(recipes);
		return;
	}

	// If the second argument doesn't exist or isn't a positive integer.
	if (objVnum <= 0)
	{
		if (commandType == 1)
		{
			send_to_char("What &+Wrecipe&n would you like &+yinformation&n about?\n", ch);
		}
		else if (commandType == 2)
		{
			send_to_char("What &+Wrecipe&n would you like &+ystatistics&n about?\n", ch);
		}
		else
		{
			send_to_char("What &+Witem &nare you attempting to forge?\n", ch);
		}
		free(recipes);
		return;
	}

	// Accept either the stable recipe vnum or the displayed list number.
	bool found = false;
	for (int i = 0; i < recipe_count; i++)
	{
		if (recipes[i] == objVnum)
		{
			found = true;
			break;
		}
	}
	if (!found && objVnum <= recipe_count)
	{
		objVnum = recipes[objVnum - 1];
		found = true;
	}
	free(recipes);

	// If we couldn't find the vnum in the recipe book (commandType is irrelevant).
	if (!found)
	{
		send_to_char("You dont appear to have that &+Wrecipe&n in your list.&n\n", ch);
		return;
	}

	// Attempt to load the object we're inspecting/making.
	if (!(obj = read_object(objVnum, VIRTUAL)))
	{
		snprintf(Gbuf1, sizeof Gbuf1, "Your recipe # %d seems to be &+rcorrupted&n. Please tell a &+WGod.\n\r", objVnum);
		logit(LOG_DEBUG, "do_forge: '%s' has bad recipe vnum (%d) - couldn't load object.", ch ? J_NAME(ch) : "NULL", objVnum);
		return;
	}

	// 1 -> "info"
	if (commandType == 1)
	{
		// Display required materials to make obj - formula below:
		struct crafting_plan plan;
		if (!crafting_build_plan(obj, &plan))
		{
			send_to_char("You couldn't figure out what materials to use.\n\r", ch);
			extract_obj(obj);
			return;
		}
		iVal                    = plan.item_value;
		numHighQuality          = plan.high_material_count;
		numLowQuality           = plan.low_material_count;
		lowQualityMaterialVnum  = plan.low_material_vnum;
		highQualityMaterialVnum = plan.high_material_vnum;
		lowQualityMaterial      = read_object(lowQualityMaterialVnum, VIRTUAL);
		highQualityMaterial     = read_object(highQualityMaterialVnum, VIRTUAL);

		if (lowQualityMaterial == NULL || highQualityMaterial == NULL)
		{
			send_to_char("You couldn't figure out what materials to use.\n\r", ch);
			if (lowQualityMaterial != NULL)
			{
				extract_obj(lowQualityMaterial);
			}
			if (highQualityMaterial != NULL)
			{
				extract_obj(highQualityMaterial);
			}
			extract_obj(obj);
			return;
		}

		send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
		if (numLowQuality == 0)
		{
			snprintf(recipe, sizeof recipe, "To forge this item, you will need %d of %s.\r\n&n", numHighQuality, highQualityMaterial->short_description);
		}
		else
		{
			// numHighQuality will always be >= 1 with the code the way it is on 2/11/2015.
			if (numHighQuality == 0)
			{
				snprintf(recipe, sizeof recipe, "To forge this item, you will need %d of %s.\r\n&n", numLowQuality, lowQualityMaterial->short_description);
			}
			else
			{
				snprintf(recipe,
				         sizeof recipe,
				         "To forge this item, you will need %d of %s and %d of %s.\r\n&n",
				         numHighQuality,
				         highQualityMaterial->short_description,
				         numLowQuality,
				         lowQualityMaterial->short_description);
			}
		}
		// If we're going to require multiple essences, need to edit this if statement.
		if (has_affect(obj))
		{
			strcat(recipe, "You must have &+W1 &nof &+ma &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml &+messence&n due to the &+mmagical &nproperties this item possesses.\r\n");
		}
		snprintf(Gbuf1, sizeof Gbuf1, "You will also need one blacksmithing flux; it is consumed to bind the work.\r\n");
		strncat(recipe, Gbuf1, sizeof(recipe) - strlen(recipe) - 1);

		page_string(ch->desc, recipe, 1);
		extract_obj(obj);
		extract_obj(highQualityMaterial);
		extract_obj(lowQualityMaterial);
		return;
	}
	// 2 -> "stat"
	else if (commandType == 2)
	{
		// Display info on obj:
		send_to_char("&+yYou open your &+Ltome &+yof &+Ycra&+yftsm&+Lanship &+yand examine the &+Litem&n.\n", ch);
		spell_identify(GET_LEVEL(ch), ch, 0, 0, 0, obj);
		extract_obj(obj);
		return;
	}
	// 3 -> "make"
	else if (commandType == 3)
	{
		// Attempt to make obj:
		struct crafting_plan plan;
		if (!crafting_build_plan(obj, &plan))
		{
			send_to_char("You couldn't figure out what materials to use.\n\r", ch);
			extract_obj(obj);
			return;
		}
		iVal = plan.item_value;
		if (iVal > GET_LEVEL(ch) * crafting_level_gate_multiplier() || IS_OBJ_STAT2(obj, ITEM2_QUESTITEM))
		{
			if (IS_OBJ_STAT2(obj, ITEM2_QUESTITEM))
				send_to_char("This is a quest item and cannot be forged from a player recipe.\r\n", ch);
			else
			{
				snprintf(buf, sizeof(buf), "You need level %d to forge this recipe (you are level %d).\r\n", (iVal + crafting_level_gate_multiplier() - 1) / crafting_level_gate_multiplier(), GET_LEVEL(ch));
				send_to_char(buf, ch);
			}
			extract_obj(obj);
			return;
		}
		numHighQuality          = plan.high_material_count;
		numLowQuality           = plan.low_material_count;
		lowQualityMaterialVnum  = plan.low_material_vnum;
		highQualityMaterialVnum = plan.high_material_vnum;
		hasAffect               = plan.magical;

		/* Foundry code here (not requiring one atm I guess):
		if( !check_foundry(ch) )
		{
		  act("&+LYou need to be by your foundry to forge...&n", FALSE, ch, 0, 0, TO_CHAR);
		  extract_obj( obj );
		  return;
		}
		*/

		hasFlux    = FALSE;
		invLowMats = invHighMats = invEssences = 0;
		// Count up the materials ch has on hand.
		for (inventory = ch->carrying; inventory; inventory = inventory->next_content)
		{
			invVnum = OBJ_VNUM(inventory);

			if (invVnum == lowQualityMaterialVnum)
			{
				invLowMats++;
			}
			else if (invVnum == highQualityMaterialVnum)
			{
				invHighMats++;
			}
			else if (invVnum == crafting_essence_vnum(CRAFTING_MODE_FORGE))
			{
				invEssences++;
			}
			else if (invVnum == crafting_tool_vnum(CRAFTING_MODE_FORGE))
			{
				hasFlux = TRUE;
			}
		}

		// Check to make sure ch has enough materials.
		if (hasFlux == FALSE)
		{
			send_to_char("You must have a &+Lblacksmithing &nflux to complete the smithing process!\r\n", ch);
			extract_obj(obj);
			return;
		}
		if ((hasAffect && invEssences < 1) || (invLowMats < numLowQuality) || (invHighMats < numHighQuality))
		{
			send_to_char("You do not have the required &+ysalvaged &+Ymaterials &nin your inventory.\r\n", ch);
			extract_obj(obj);
			return;
		}

		// Ok, ch has the materials needed to create obj in inventory.. Now take them away, muahahah!
		for (inventory = ch->carrying; inventory; inventory = invNextObj)
		{
			invNextObj = inventory->next_content;
			invVnum    = OBJ_VNUM(inventory);

			if ((invVnum == lowQualityMaterialVnum) && (numLowQuality > 0))
			{
				extract_obj(inventory, TRUE); // Not an arti, but 'in game.'
				numLowQuality--;
			}
			else if ((invVnum == highQualityMaterialVnum) && (numHighQuality > 0))
			{
				extract_obj(inventory, TRUE); // Not an arti, but 'in game.'
				numHighQuality--;
			}
			else if (hasAffect && (invVnum == crafting_essence_vnum(CRAFTING_MODE_FORGE)))
			{
				extract_obj(inventory, TRUE); // Not an arti, but 'in game.'
				hasAffect = FALSE;
			}
			else if (hasFlux && (invVnum == crafting_tool_vnum(CRAFTING_MODE_FORGE)))
			{
				extract_obj(inventory, TRUE); // Not an arti, but 'in game.'
				hasFlux = FALSE;
			}
		}

		notch_skill(ch, SKILL_FORGE, 50);
		SET_BIT(obj->extra2_flags, ITEM2_CRAFTED);
		SET_BIT(obj->extra_flags, ITEM_NOREPAIR);
		REMOVE_BIT(obj->extra_flags, ITEM_SECRET);

		snprintf(keywords, sizeof keywords, "%s %s tradeskill", obj->name, GET_NAME(ch));
		snprintf(short_desc, sizeof short_desc, "%s &+ymade by&n &+r%s&n", obj->short_description, GET_NAME(ch));
		set_keywords(obj, keywords);
		set_short_description(obj, short_desc);

		wizlog(56, "%s forged '%s' (%d) ival %d.", GET_NAME(ch), obj->short_description, objVnum, itemvalue(obj));
		obj_to_char(obj, ch);

		act("&+W$n &+Lgently takes their &+ymaterials&+L, their &nflux&+L, and places them into the &+rf&+Ro&+Yr&+Rg&+re&+L.\r\n"
		    "&+W$n &+Lremoves the &+yitems &+Lfrom the &+rheat &+Land starts to &nhammer &+Laway at the mixture..\r\n"
		    "&+L...after shedding plenty of &+Wsweat&+L, &+W$n &+Lsteps back, admiring their new $p.&N",
		    TRUE,
		    ch,
		    obj,
		    0,
		    TO_ROOM);
		act("You &+Lgently take your &+ymaterials&+L, the &nflux&+L, and place them into the &+rf&+Ro&+Yr&+Rg&+re&+L.\r\n"
		    "You &+Lremove the &+yitems &+Lfrom the &+rheat &+Land start to &nhammer &+Laway at the mixture..\r\n"
		    "&+L...after shedding plenty of &+Wsweat&+L, you &+Lstep back, admiring your new $p.&N",
		    FALSE,
		    ch,
		    obj,
		    0,
		    TO_CHAR);

		gain_exp(ch, NULL, iVal * crafting_experience_per_ival(), EXP_BOON);
	}
	else
	{
		send_to_char("&+RBad command type&n: Please report this to an Immortal.\n\r", ch);
		logit(LOG_DEBUG, "do_forge: bad command arguments '%s' by '%s'.", argument, ch ? J_NAME(ch) : "NULL");
		extract_obj(obj);
		return;
	}
}
