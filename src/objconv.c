/*
 * ***************************************************************************
 *   File: objconv.c                                           Part of Duris
 *   Usage: balancing objects during initial loading
 *   Copyright 1994 - 2008 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "db.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "objmisc.h"
#include "spells.h"

extern Skill skills[];

/* similar to GetSpellCircle, except it doesnt check circle for a given
   char. Rather, it simply returns the lowest circle this spell could
   be for a proper user.
 */

int GetCircle(int spl)
{
	int circle = 11, i;

	for (i = 0; i < CLASS_COUNT; i++)
	{
		if (skills[spl].m_class[i].rlevel[0] && skills[spl].m_class[i].rlevel[0] < circle)
			circle = skills[spl].m_class[i].rlevel[0];
	}
	circle = BOUNDED(2, circle, 11); /* 2 prevents them from being too cheap */
	return circle;
}

#define ALL_MAGES (CLASS_SORCERER | CLASS_NECROMANCER | CLASS_SUMMONER | CLASS_CONJURER | CLASS_ILLUSIONIST | CLASS_PSIONICIST)

#define ALL_ROGUES (CLASS_THIEF | CLASS_ASSASSIN | CLASS_BARD | CLASS_ROGUE)

// Sets anti-class flags based on material type and object name.
void material_restrictions(P_obj obj)
{
	ulong anti, anti2;
	int   mat;
	// Since it's referenced so much.
	char *name = obj->name;

	if (!obj)
	{
		return;
	}

	anti = anti2 = 0;
	mat          = obj->material;

	if (isname("quiver", name) || isname("badge", name) || isname("robe", name) || isname("tunic", name) || isname("cloak", name) || isname("pants", name) || isname("belt", name) ||
	    isname("earring", name) || isname("moccasins", name) || isname("ring", name) || isname("band", name) || isname("signet", name) || isname("hat", name) || isname("cap", name) ||
	    isname("bracelet", name) || isname("stud", name) || isname("amulet", name) || isname("bodycloak", name))
	{
		return;
	}

	if (IS_RIGID(mat) && (obj->wear_flags & ITEM_WEAR_BODY))
	{
		anti = ALL_MAGES | ALL_ROGUES | CLASS_MONK;
	}

	if (IS_RIGID(mat) && (obj->wear_flags & (ITEM_WEAR_FACE | ITEM_WEAR_ARMS | ITEM_WEAR_HEAD | ITEM_WEAR_LEGS)))
	{
		anti |= CLASS_MONK;
	}

	if (IS_METAL(mat) && (mat != MAT_MITHRIL) && (obj->wear_flags & (ITEM_WEAR_FACE | ITEM_WEAR_ARMS | ITEM_WEAR_HEAD | ITEM_WEAR_LEGS)))
	{
		anti |= ALL_MAGES;
	}

	if (obj->extra_flags & ITEM_ALLOWED_CLASSES)
		obj->anti_flags &= ~anti;
	else
		obj->anti_flags |= anti;
}

void convertObj(P_obj obj)
{
	int  i, val0, val1, val2, val3, type;
	long weight = 0, cost = 0;
	char buf2[MAX_STRING_LENGTH];

	if (!obj || IS_SET(obj->extra_flags, ITEM_IGNORE))
		return;

	obj->bitvector &= ~(AFF_SLEEP | AFF_FEAR | AFF_CHARM);

	val0 = obj->value[0];
	val1 = obj->value[1];
	val2 = obj->value[2];
	val3 = obj->value[3];
	type = GET_ITEM_TYPE(obj);

	/* set a few base values */

	switch (type)
	{
		case ITEM_TELEPORT:
			weight = 10000;
			break;
		case ITEM_SCROLL:
		case ITEM_POTION:
			weight = 2;
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			weight = (type == ITEM_WAND) ? 2 : 5;
			break;
		case ITEM_WEAPON:
			/* 1 g * max damage */

			/* number of dice more important than size.. */
			if (val1 < 1)
				val1 = obj->value[1] = 1;
			if (val2 < 1)
				val2 = obj->value[2] = 1;
			cost = 100 * (val1 * 2) * val2;
			if (IS_BACKSTABBER(obj))
			{
				cost += 1000 * (val1 * val1 * val1 * 2);
				cost += 1000 * (val2 * val2 / 2);
			}
			break;
		case ITEM_FIREWEAPON:
			/* 1 s * missle type * rate of fire */

			cost = (val0 ? 1 : 0) * (val1 ? 1 : 0) * 10;
			break;
		case ITEM_MISSILE:
			/* 1 s * missle type */
			//    cost = val3 * (val1 * 2) * val2 * (val0 / 2) * 10;
			// Start with average damage * 10.
			cost = (obj->value[1] * (obj->value[2] + 1)) * 5;
			// cost = (10 * avgdam) squared * maxdamage cubed / 125
			cost        = (cost * cost * val1 * val1 * val1 * val2 * val2 * val2) / 125;
			obj->cost   = BOUNDED(1, cost, 5000000);
			weight      = obj->weight;
			obj->weight = BOUNDED(0, weight, 10);
			return;
			break;
		case ITEM_TREASURE:
		case ITEM_TRASH:
		case ITEM_OTHER:
			/* hmm */
			break;
		case ITEM_ARMOR:
			/* 1 g * ac */
			if (val0 < 10)
				cost = 150 * val0;
			else if (val0 < 20)
				cost = 300 * val0;
			else if (val0 < 30)
				cost = 625 * val0;
			else if (val0 < 40)
				cost = 1250 * val0;
			else
				cost = 2500 * val0;
			break;
		case ITEM_WORN:
			cost = 100;
			break;
		case ITEM_CONTAINER:
			/* 1 silver per pound */
			cost = 25 * val0;
			if (obj->weight <= 0)
			{
				weight = obj->weight;
				cost += (weight * -500);
			}
			if (weight >= 0)
				weight = (val0 > 50) ? 3 : 1;
			if (IS_SET(obj->wear_flags, ITEM_ATTACH_BELT) && weight > 9)
				REMOVE_BIT(obj->wear_flags, ITEM_ATTACH_BELT);
			if (obj->R_num == real_object(96443))
				cost = 1000;
			break;
		case ITEM_DRINKCON:
		case ITEM_QUIVER:
			/* 1 silver per drink/arrows held */
			cost   = 20 * val0;
			weight = (val0 > 50) ? 3 : 1;
			if (IS_SET(obj->wear_flags, ITEM_ATTACH_BELT) && weight > 9)
				REMOVE_BIT(obj->wear_flags, ITEM_ATTACH_BELT);
			break;
		case ITEM_FOOD:
			if (isname("rations", obj->name))
				cost = 20;
			break;
		case ITEM_NOTE:
		case ITEM_PEN:
		case ITEM_BOOK:
		case ITEM_PICK:
			/* simple base values */
			cost = 5;
			if (type == ITEM_BOOK)
				weight = 3;
			else
				weight = 0;
			break;
		case ITEM_KEY:
			cost = 5;
			if (type == ITEM_BOOK)
				weight = 3;
			else
				weight = 0;
			SET_BIT(obj->extra_flags, ITEM_NORENT);
			break;
		case ITEM_SPELLBOOK:
			/* 15 c per page in book */
			break;
		case ITEM_INSTRUMENT:
			if (!strstr(obj->name, "instrument"))
			{
				snprintf(buf2, MAX_STRING_LENGTH, "%s %s", obj->name, "instrument");
				obj->name = str_dup(buf2);
			}
			break;
		case ITEM_TOTEM:
			/* ? */
			if (!strstr(obj->name, "totem"))
			{
				snprintf(buf2, MAX_STRING_LENGTH, "%s %s", obj->name, "totem");
				obj->name = str_dup(buf2);
			}
			/*    weight = (GET_ITEM_TYPE(obj) == ITEM_TOTEM) ? 2 : 3;
			    cost = 1;*/
			break;
		case ITEM_STORAGE:
			/* 5 p per pound held */
			cost = 5000 * val0;
			/*    weight = (val0 > 100) ? 250 : 100; */
			break;
	}
	if (!cost)
		cost = obj->cost;
	/*  if (!weight)
	    weight = obj->weight;*/

	cost   = BOUNDED(0, cost, 1000000);
	weight = BOUNDED(0, weight, 1000);

	/* at this point, we either have made cost/weight, or we're still using
	   values from the files (for iffy items). From here on, we add/subtract */

	/* figure in the extras flags */

	if (IS_SET(obj->extra2_flags, ITEM2_MAGIC))
		cost *= 2;
	if (IS_SET(obj->extra_flags, ITEM_GLOW))
		cost += 100;
	if (IS_SET(obj->extra_flags, ITEM_HUM))
		cost += 100;
	if (IS_SET(obj->extra2_flags, ITEM2_BLESS))
		cost += 500;
	if (IS_SET(obj->extra_flags, ITEM_FLOAT))
		cost += 1000;
	if (IS_SET(obj->extra_flags, ITEM_NOSUMMON))
		cost += 15000;
	if (IS_SET(obj->extra_flags, ITEM_LIT))
		cost += 500;
	if (IS_SET(obj->extra_flags, ITEM_NOSLEEP))
		cost += 15000;
	if (IS_SET(obj->extra_flags, ITEM_NOCHARM))
		cost += 12500;
	if (IS_SET(obj->extra_flags, ITEM_TWOHANDS))
		weight += 5;
	if (IS_SET(obj->extra2_flags, ITEM2_SILVER))
	{
		cost += 5000;
		weight += 2;
	}
	if (IS_SET(obj->extra2_flags, ITEM2_SLAY_GOOD))
		cost += 6000;
	if (IS_SET(obj->extra2_flags, ITEM2_SLAY_EVIL))
		cost += 6000;
	if (IS_SET(obj->extra2_flags, ITEM2_SLAY_UNDEAD))
		cost += 6000;
	if (IS_SET(obj->extra2_flags, ITEM2_SLAY_LIVING))
		cost += 6000;
	if (IS_SET(obj->extra_flags, ITEM_RETURNING))
		cost += 2500;
	if (IS_SET(obj->extra_flags, ITEM_CAN_THROW1))
	{
		cost += 1200;
		weight -= 1;
	}
	if (IS_SET(obj->extra_flags, ITEM_CAN_THROW2))
	{
		cost += 5000;
		weight -= 2;
	}
	if (IS_SET(obj->extra_flags, ITEM_NORENT))
		cost /= 2;
	if (IS_SET(obj->extra_flags, ITEM_NODROP))
		cost = (int)(cost / 1.5);

	/* affects get added */
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		switch (obj->affected[i].location)
		{
			case APPLY_NONE:
				break;
			case APPLY_STR:
			case APPLY_DEX:
			case APPLY_INT:
			case APPLY_WIS:
			case APPLY_CON:
			case APPLY_AGI:
			case APPLY_POW:
			case APPLY_CHA:
			case APPLY_KARMA:
			case APPLY_LUCK:
				if (obj->affected[i].modifier > 0)
					cost += obj->affected[i].modifier * 150;
				else
					cost += obj->affected[i].modifier * 100;
				break;
			case APPLY_STR_MAX:
			case APPLY_DEX_MAX:
			case APPLY_INT_MAX:
			case APPLY_WIS_MAX:
			case APPLY_CON_MAX:
			case APPLY_AGI_MAX:
			case APPLY_POW_MAX:
			case APPLY_CHA_MAX:
			case APPLY_KARMA_MAX:
			case APPLY_LUCK_MAX:
			case APPLY_STR_RACE:
			case APPLY_DEX_RACE:
			case APPLY_INT_RACE:
			case APPLY_WIS_RACE:
			case APPLY_CON_RACE:
			case APPLY_AGI_RACE:
			case APPLY_POW_RACE:
			case APPLY_CHA_RACE:
			case APPLY_KARMA_RACE:
			case APPLY_LUCK_RACE:
				cost += obj->affected[i].modifier * 2500;
				break;
			case APPLY_SEX:
			case APPLY_CLASS:
			case APPLY_LEVEL:
			case APPLY_CHAR_WEIGHT:
			case APPLY_CHAR_HEIGHT:
			case APPLY_GOLD:
			case APPLY_EXP:
				cost += 5000;
				break;
			case APPLY_HIT:
				cost += obj->affected[i].modifier * 1000;
				break;
			case APPLY_MANA:
				cost += obj->affected[i].modifier * 500;
				break;
			case APPLY_MOVE:
				cost += obj->affected[i].modifier * 50;
				break;
			case APPLY_AGE:
				cost -= obj->affected[i].modifier * 1500; /* minus cause younger is better */
				break;
			case APPLY_ARMOR:
				if (obj->affected[i].modifier > -10)
					cost -= 500 * val0;
				else if (obj->affected[i].modifier > -20)
					cost -= 1000 * val0;
				else if (obj->affected[i].modifier > -30)
					cost -= 2000 * val0;
				else if (obj->affected[i].modifier > -40)
					cost -= 3500 * val0;
				else
					cost -= 6000 * val0;
				/* minus cause armor is a neg value */
				break;
			case APPLY_HITROLL:
			case APPLY_DAMROLL:
				cost += obj->affected[i].modifier * 1300;
				break;
			case APPLY_SAVING_PARA:
			case APPLY_SAVING_ROD:
			case APPLY_SAVING_FEAR:
			case APPLY_SAVING_BREATH:
			case APPLY_SAVING_SPELL:
				cost -= obj->affected[i].modifier * 2500; /* negative is better */
				break;
		}
	}

	/* bitvectors make big difference */
	if (IS_SET(obj->bitvector, AFF_BLIND))
		cost -= 5000;
	if (IS_SET(obj->bitvector, AFF_INVISIBLE))
		cost += 100000;
	if (IS_SET(obj->bitvector, AFF_FARSEE))
		cost += 10000;
	if (IS_SET(obj->bitvector, AFF_DETECT_INVISIBLE))
		cost += 4000;
	if (IS_SET(obj->bitvector, AFF_HASTE))
		cost += 25000;
	if (IS_SET(obj->bitvector, AFF_SENSE_LIFE))
		cost += 3500;
	if (IS_SET(obj->bitvector, AFF_MINOR_GLOBE))
		cost += 125000;
	if (IS_SET(obj->bitvector, AFF_STONE_SKIN))
		cost += 25000;
	if (IS_SET(obj->bitvector, AFF_UD_VISION))
		cost += 15000;
	if (IS_SET(obj->bitvector, AFF_WRAITHFORM))
		cost += 20000;
	if (IS_SET(obj->bitvector, AFF_WATERBREATH))
		cost += 10000;
	if (IS_SET(obj->bitvector, AFF_PROTECT_EVIL))
		cost += 15000;
	if (IS_SET(obj->bitvector, AFF_SLOW_POISON))
		cost += 7500;
	if (IS_SET(obj->bitvector, AFF_PROTECT_GOOD))
		cost += 15000;
	if (IS_SET(obj->bitvector, AFF_SLEEP))
		cost += 2500;
	if (IS_SET(obj->bitvector, AFF_SNEAK))
		cost += 50000;
	if (IS_SET(obj->bitvector, AFF_HIDE))
		cost += 250000;
	if (IS_SET(obj->bitvector, AFF_BARKSKIN))
		cost += 10420;
	if (IS_SET(obj->bitvector, AFF_INFRAVISION))
		cost += 60000;
	if (IS_SET(obj->bitvector, AFF_LEVITATE))
		cost += 40000;
	if (IS_SET(obj->bitvector, AFF_FLY))
		cost += 90000;
	if (IS_SET(obj->bitvector, AFF_AWARE))
		cost += 100000;
	if (IS_SET(obj->bitvector, AFF_PROT_FIRE))
		cost += 25000;
	if (IS_SET(obj->bitvector, AFF_BIOFEEDBACK))
		cost += 75000;
	if (IS_SET(obj->bitvector2, AFF2_FIRESHIELD))
		cost += 200000;
	if (IS_SET(obj->bitvector2, AFF2_ULTRAVISION))
		cost += 75000;
	if (IS_SET(obj->bitvector2, AFF2_DETECT_EVIL))
		cost += 10000;
	if (IS_SET(obj->bitvector2, AFF2_DETECT_GOOD))
		cost += 10000;
	if (IS_SET(obj->bitvector2, AFF2_DETECT_MAGIC))
		cost += 4000;
	if (IS_SET(obj->bitvector2, AFF2_PROT_COLD))
		cost += 25000;
	if (IS_SET(obj->bitvector2, AFF2_PROT_LIGHTNING))
		cost += 25000;
	if (IS_SET(obj->bitvector2, AFF2_GLOBE))
		cost += 500000;
	if (IS_SET(obj->bitvector2, AFF2_PROT_GAS))
		cost += 25000;
	if (IS_SET(obj->bitvector2, AFF2_PROT_ACID))
		cost += 25000;
	if (IS_SET(obj->bitvector2, AFF2_SOULSHIELD))
		cost += 75000;
	if (IS_SET(obj->bitvector2, AFF2_CONCEALMENT))
		cost += 200000;
	if (IS_SET(obj->bitvector2, AFF2_VAMPIRIC_TOUCH))
		cost += 150000;
	if (IS_SET(obj->bitvector2, AFF2_PASSDOOR))
		cost += 75000;

	/* default condition hurts it */
	if (obj->condition < 50)
		cost /= 3;
	else if (obj->condition < 75)
		cost /= 2;
	else if (obj->condition < 90)
		cost = (int)(cost * 1.25);

	/* for some items, consider material type */
	/* armor and worn items weights are affected by locale worn */
	if (GET_ITEM_TYPE(obj) == ITEM_ARMOR || GET_ITEM_TYPE(obj) == ITEM_WORN)
		if (CAN_WEAR(obj, ITEM_WEAR_FINGER) || CAN_WEAR(obj, ITEM_GUILD_INSIGNIA) || CAN_WEAR(obj, ITEM_WEAR_EYES) || CAN_WEAR(obj, ITEM_WEAR_EARRING))
			weight = 0;
		else if (CAN_WEAR(obj, ITEM_WEAR_HEAD) || CAN_WEAR(obj, ITEM_WEAR_WAIST) || CAN_WEAR(obj, ITEM_WEAR_WRIST) || CAN_WEAR(obj, ITEM_WEAR_TAIL) || CAN_WEAR(obj, ITEM_WEAR_QUIVER) ||
		         CAN_WEAR(obj, ITEM_WEAR_NOSE) || CAN_WEAR(obj, ITEM_WEAR_HORN))
			weight -= 10;
		else if (CAN_WEAR(obj, ITEM_WEAR_NECK) || CAN_WEAR(obj, ITEM_WEAR_FACE) || CAN_WEAR(obj, ITEM_WEAR_FEET) || CAN_WEAR(obj, ITEM_WEAR_HANDS))
			weight -= 5;
		else if (CAN_WEAR(obj, ITEM_WEAR_BODY) || CAN_WEAR(obj, ITEM_HORSE_BODY) || CAN_WEAR(obj, ITEM_SPIDER_BODY))
			weight += 10;
		else if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
		{
			if (isname("spiked", obj->name))
			{
				cost += 500;
				weight += 1;
			}
			if (isname("large", obj->name) || isname("huge", obj->name))
			{
				cost += 100;
				weight += 5;
			}
			if (isname("small", obj->name) || isname("tiny", obj->name))
				weight -= 5;
		}

	/* check some keywords */
	if (isname("ornate", obj->name) || isname("gem", obj->name) || isname("gold", obj->name) || isname("platinum", obj->name) || isname("jewel", obj->name))
		cost *= 2;
	if (isname("worn", obj->name) || isname("broken", obj->name) || isname("ruined", obj->name))
		cost /= 2;

	cost   = BOUNDED(1, cost, 5000000);
	weight = BOUNDED(0, weight, 10000);

	/* slight randomizing */
	cost += ((cost / 100) * number(-2, 2));

	/*  obj->weight = weight;*/
	if ((type != ITEM_INSTRUMENT) && (type != ITEM_TOTEM))
		obj->cost = cost;
}
