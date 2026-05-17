
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
#include <time.h>
#include "damage.h"
#include "justice.h"
#include "objmisc.h"
#include "spells.h"
#include "weather.h"

extern P_char               character_list;
extern P_desc               descriptor_list;
extern P_index              mob_index;
extern P_index              obj_index;
extern P_obj                object_list;
extern P_room               world;
extern int                  proccing_slots[];
extern struct material_data materials[];

char *item_condition(P_obj obj)
{
	int value;
	// I can't imagine anything longer than "&N&=LR\0" which is only 7 chars.
	char bufCOLOR[8];
	// I can't imagine having a condition string longer than " [&N&=LR100%%&N]\0" which is only 16 chars.. 32 def safe.
	static char buf[32];

	if (obj->condition > 90)
		snprintf(bufCOLOR, 8, "&+G");
	else if (obj->condition > 70)
		snprintf(bufCOLOR, 8, "&+g");
	else if (obj->condition > 50)
		snprintf(bufCOLOR, 8, "&+y");
	else if (obj->condition > 20)
		snprintf(bufCOLOR, 8, "&+r");
	else if (obj->condition > 0)
		snprintf(bufCOLOR, 8, "&+R");
	else
		snprintf(bufCOLOR, 8, "&=LR");

	// Print either the [20%] or spaces of equivalent length.
	if (obj->condition < 90)
		snprintf(buf, 32, " [%s%2d&n%%]", bufCOLOR, obj->condition);
	else
		buf[0] = '\0';

	return buf;
}

const char *item_damage_messages[][2] = {
	{		"was damaged from the massive blow!",                   "was completely destroyed by the massive blow!"},
	{				 "is burned from the blast!",								   "melted from the intense heat!"},
	{		"is weakened from the intense cold!", "freezes and shatters into million pieces from the intense cold!"},
	{		 "is filled with electrical energy!",                "exploded spreading charges of electrical energy!"},
	{					  "was corroded by gas.",                   "crumbled as it was consumed by the toxic gas."},
	{					 "was corroded by acid.",									"melted corroded by the acid."},
	{		"is blasted by the negative energy!",                       "was disintegrated by the negative energy!"},
	{				   "is burned by the light!",             "shattered into a million pieces by the divine fury!"},
	{	  "cracks as the tortured body writhes!",      "shattered in bits as the tortured body spasms and quivers!"},
	{		  "is corrupted by spiritual anger!",                                     "was destroyed by the blast!"},
	{		"cracks attacked by the sound wave.",       "shattered into a million pieces as the sound wave hit it!"},
	{"cracks from the bombardment of &+yearth&n.",               "is smashed to bits by &+Lrocks&n and &+ydebris&n!"},
};

int DamageOneItem(P_char ch, int dam_type, P_obj obj, bool destroy)
{
	int  num;
	char buf[MAX_STRING_LENGTH];
	int  objtype       = GET_ITEM_TYPE(obj);
	bool force_destroy = destroy;

	if ((objtype == ITEM_TOTEM) || (objtype == ITEM_KEY) || (objtype == ITEM_SPELLBOOK) || IS_ARTIFACT(obj))
	{
		return 0;
	}

	num = number(1, 2);

	if (materials[obj->material].dam_res[dam_type])
	{
		num *= materials[obj->material].dam_res[dam_type];
	}

	// physical being the most common is less harsh
	if (dam_type == SPLDAM_GENERIC)
	{
		num >>= 1;
	}

	num = BOUNDED(1, num, 10);

	obj->condition -= num;

	if (obj->condition < 0)
	{
		destroy = TRUE;
	}

	// objtype = GET_ITEM_TYPE(obj);

	snprintf(buf, MAX_STRING_LENGTH, "Your $q %s", item_damage_messages[dam_type - 1][destroy ? 1 : 0]);
	act(buf, TRUE, ch, obj, 0, TO_CHAR);

	snprintf(buf, MAX_STRING_LENGTH, "$n's $q %s", item_damage_messages[dam_type - 1][destroy ? 1 : 0]);
	act(buf, TRUE, ch, obj, 0, TO_ROOM);

	if (destroy)
		if (!force_destroy)
			MakeScrap(ch, obj);
		else
			extract_obj(obj);

	return destroy;
}

void MakeScrap(P_char ch, P_obj obj)
{
	char  buf[MAX_STRING_LENGTH];
	P_obj t, x;
	int   pos;

	if (!ch || !obj || (ch->in_room == NOWHERE))
		return;

	act("$p falls to the ground in scraps.", TRUE, ch, obj, 0, TO_CHAR);
	act("$p falls to the ground in scraps.", TRUE, ch, obj, 0, TO_ROOM);

	t = read_object(9, VIRTUAL);

	if (!t)
		return;

	snprintf(buf, MAX_STRING_LENGTH, "Scraps from %s&n lie in a pile here.", obj->short_description);

	t->description = str_dup(buf);

	snprintf(buf, MAX_STRING_LENGTH, "a pile of scraps from %s", obj->short_description);

	t->short_description = str_dup(buf);

	snprintf(buf, MAX_STRING_LENGTH, "%s scraps pile", obj->name);

	t->name = str_dup(buf);

	t->str_mask = STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_KEYS;

	if (OBJ_CARRIED(obj))
	{
		obj_from_char(obj);
	}
	else if (OBJ_WORN(obj))
	{
		P_char wearer = obj->loc.wearing;

		if (wearer && !char_in_list(wearer))
		{
			logit(LOG_DEBUG, "MakeScrap(): stale wearer pointer, caller=%s obj=%s", J_NAME(ch), obj->short_description ? obj->short_description : "unknown");
			wearer = NULL;
		}

		if (!wearer)
		{
			for (wearer = character_list; wearer; wearer = wearer->next)
			{
				for (pos = 0; pos < MAX_WEAR; pos++)
					if (wearer->equipment[pos] == obj)
						break;
				if (pos < MAX_WEAR)
						break;
			}
		}

		if (wearer)
		{
			if (wearer != ch)
				logit(LOG_DEBUG, "MakeScrap(): caller/wearer mismatch, caller=%s wearer=%s obj=%s", J_NAME(ch), J_NAME(wearer), obj->short_description ? obj->short_description : "unknown");

			for (pos = 0; pos < MAX_WEAR; pos++)
				if (wearer->equipment[pos] == obj)
					break;
			if (pos < MAX_WEAR)
				unequip_char(wearer, pos);
			else
			{
				P_char found = NULL;
				int    found_pos = -1;

				logit(LOG_EXIT, "MakeScrap(): worn object not found in wearer equipment: caller=%s wearer=%s obj=%s", J_NAME(ch), J_NAME(wearer), obj->short_description ? obj->short_description : "unknown");
				for (found = character_list; found; found = found->next)
				{
					for (pos = 0; pos < MAX_WEAR; pos++)
						if (found->equipment[pos] == obj)
							break;
					if (pos < MAX_WEAR)
						{
							found_pos = pos;
							break;
						}
				}
				if (found_pos >= 0)
				{
					logit(LOG_DEBUG, "MakeScrap(): recovered worn object via global rescan, caller=%s wearer=%s obj=%s", J_NAME(ch), J_NAME(found), obj->short_description ? obj->short_description : "unknown");
					unequip_char(found, found_pos);
				}
				else
				{
					obj->loc_p       = LOC_NOWHERE;
					obj->loc.wearing = NULL;
				}
			}
		}
		else
		{
			logit(LOG_EXIT, "MakeScrap(): worn object missing live wearer: caller=%s obj=%s", J_NAME(ch), obj->short_description ? obj->short_description : "unknown");
			obj->loc_p       = LOC_NOWHERE;
			obj->loc.wearing = NULL;
		}
	}

	set_obj_affected(t, 400, TAG_OBJ_DECAY, 0);
	obj_to_room(t, ch->in_room);

	while (obj->contains)
	{
		x = obj->contains;
		obj_from_obj(x);
		obj_to_room(x, ch->in_room);
	}

	if (OBJ_FALLING(t))
		falling_obj(t, 1, false);

	extract_obj(obj);
}

void DamageAllStuff(P_char ch, int dam_type)
{
	int   j;
	P_obj obj, next;

	/* this procedure takes all of the items in equipment and inventory
	   and damages the ones that should be damaged */

	/* equipment */

	for (j = 0; j < MAX_WEAR; j++)
	{
		if (ch->equipment[j] && (j != WIELD) && (j != WIELD2))
		{
			obj = ch->equipment[j];
			DamageOneItem(ch, dam_type, obj, FALSE);
		}
	}

	/* inventory */
	for (obj = ch->carrying; obj; obj = next)
	{
		next = obj->next_content;
		DamageOneItem(ch, dam_type, obj, FALSE);
	}
}

void DamageStuff(P_char v, int type)
{
	int   slot;
	P_obj obj;

	slot = number(2, CUR_MAX_WEAR);
	if (v->equipment[slot] && slot != WIELD && slot != WIELD2)
	{
		DamageOneItem(v, type, v->equipment[slot], FALSE);
	}
}
