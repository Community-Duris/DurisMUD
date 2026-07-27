#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "utils.h"
#include "randobj.h"
#include <strings.h>
#include "tradeskill.h"
#include "vnum.obj.h"

extern P_index obj_index;

void create_zone(int theme, int map_room1, int map_room2, int level_range, int rooms);

/*  1-10 =  5, 11-20 =  6, 21-30 =  7, 31-35 =  8, 36-40 =  9, 41-45 = 10,
   46-50 = 12, 51-55 = 14, 56-59 = 16, 60    = 20, randomized +/- 1 */

/* zone diff 1-5 */

/* naming convention - if something with one affect, use name, such as
   'ring of life-sensing', 'ring of strength', etc */

randObjAff randomObjFieldsArr[] = {

	/* possible affect1 bits */

	{roatAffBit1,        AFF_INVISIBLE, 60, 0, FALSE, 0,  TRUE,                     "invisibility",                 "the spirits"},
	{roatAffBit1,           AFF_FARSEE, 40, 0, FALSE, 0,  TRUE,                         "farsight",                   "the eagle"},
	{roatAffBit1, AFF_DETECT_INVISIBLE, 60, 0, FALSE, 0,  TRUE,           "invisibility detection",                            ""},
	{roatAffBit1,            AFF_HASTE, 60, 0, FALSE, 0,  TRUE,                            "haste",                   "the snake"},
	{roatAffBit1,       AFF_SENSE_LIFE, 35, 0, FALSE, 0,  TRUE,                     "life-sensing",                            ""},
	{roatAffBit1,      AFF_MINOR_GLOBE, 50, 0, FALSE, 0,  TRUE,      "minor protection from magic",                "the beholder"},
	{roatAffBit1,       AFF_STONE_SKIN, 60, 0, FALSE, 0,  TRUE,                    "skin of stone",                   "the golem"},
	{roatAffBit1,        AFF_UD_VISION, 40, 0, FALSE, 0,  TRUE,                 "Underdark vision",                     "the bat"},
	{roatAffBit1,      AFF_WATERBREATH, 35, 0, FALSE, 0, FALSE,                   "waterbreathing",                    "the toad"},
	{roatAffBit1,     AFF_PROTECT_EVIL, 25, 0, FALSE, 0, FALSE,                  "evil protection",                 "the paladin"},
	{roatAffBit1,      AFF_SLOW_POISON, 15, 0, FALSE, 0, FALSE,           "protection from poison",                "the basilisk"},
	{roatAffBit1,     AFF_PROTECT_GOOD, 25, 0, FALSE, 0, FALSE,                  "good protection",             "the deathknight"},
	{roatAffBit1,      AFF_SKILL_AWARE, 60, 0, FALSE, 0,  TRUE,                        "awareness",                            ""},
	{roatAffBit1,            AFF_SNEAK, 60, 0, FALSE, 0,  TRUE,                          "stealth",                   "the rogue"},
	{roatAffBit1,      AFF_INFRAVISION, 35, 0, FALSE, 0, FALSE,                      "infravision",              "darkness sight"},
	{roatAffBit1,         AFF_LEVITATE, 40, 0, FALSE, 0, FALSE,                       "levitation",              "the mindflayer"},
	{roatAffBit1,              AFF_FLY, 70, 0, FALSE, 0, FALSE,                           "flight",                   "the swift"},
	{roatAffBit1,            AFF_AWARE, 60, 0, FALSE, 0,  TRUE,                        "awareness",                            ""},
	{roatAffBit1,        AFF_PROT_FIRE, 35, 0, FALSE, 0, FALSE,             "protection from fire",              "the salamander"},
	{roatAffBit1,      AFF_BIOFEEDBACK, 60, 0, FALSE, 0,  TRUE,                      "biofeedback",                            ""},

	/* possible affect2 bits */

	{roatAffBit2,      AFF2_FIRESHIELD, 60, 0, FALSE, 0,  TRUE,                       "fireshield",               "the elemental"},
	{roatAffBit2,     AFF2_ULTRAVISION, 50, 0, FALSE, 0,  TRUE,                      "ultravision",                    "the drow"},
	{roatAffBit2,     AFF2_DETECT_EVIL,  3, 0, FALSE, 0, FALSE,                   "evil detection",      "the eye of the paladin"},
	{roatAffBit2,     AFF2_DETECT_GOOD,  3, 0, FALSE, 0, FALSE,                   "good detection", "the eye of the anti-paladin"},
	{roatAffBit2,    AFF2_DETECT_MAGIC,  3, 0, FALSE, 0, FALSE,                  "magic detection",         "the eye of the mage"},
	{roatAffBit2,       AFF2_PROT_COLD, 35, 0, FALSE, 0, FALSE,             "protection from cold",                    "the yeti"},
	{roatAffBit2,  AFF2_PROT_LIGHTNING, 35, 0, FALSE, 0, FALSE,        "protection from lightning",                   "the storm"},
	{roatAffBit2,           AFF2_GLOBE, 80, 0, FALSE, 0, FALSE,            "protection from magic",                "the beholder"},
	{roatAffBit2,        AFF2_PROT_GAS, 35, 0, FALSE, 0, FALSE,              "protection from gas",                  "the dragon"},
	{roatAffBit2,       AFF2_PROT_ACID, 35, 0, FALSE, 0, FALSE,             "protection from acid",                   "the viper"},
	{roatAffBit2,      AFF2_SOULSHIELD, 55, 0, FALSE, 0,  TRUE,                       "soulshield",                       "faith"},
	{roatAffBit2,  AFF2_VAMPIRIC_TOUCH, 45, 0, FALSE, 0, FALSE,                   "vampiric touch",                 "the vampire"},
	{roatAffBit2,        AFF2_PASSDOOR, 55, 0, FALSE, 0,  TRUE,                         "passdoor",                   "the ghost"},

	/* possible 'affect's (strength etc) */

	{    roatAff,            APPLY_STR,  3, 2, FALSE, 0, FALSE,                         "strength",                   "the giant"},
	{    roatAff,            APPLY_DEX,  3, 2, FALSE, 0, FALSE,                        "dexterity",                   "the thief"},
	{    roatAff,            APPLY_INT,  3, 2, FALSE, 0, FALSE,                     "intelligence",                   "the gnome"},
	{    roatAff,            APPLY_WIS,  3, 2, FALSE, 0, FALSE,                           "wisdom",                    "the sage"},
	{    roatAff,            APPLY_CON,  3, 2, FALSE, 0, FALSE,                     "constitution",                   "the troll"},
	{    roatAff,            APPLY_AGI,  3, 2, FALSE, 0, FALSE,                          "agility",              "the mudskipper"},
	{    roatAff,            APPLY_POW,  3, 2, FALSE, 0, FALSE,                            "power",                   "the gypsy"},
	{    roatAff,            APPLY_CHA,  3, 2, FALSE, 0, FALSE,                         "charisma",                "the merchant"},
	{    roatAff,           APPLY_LUCK,  6, 4, FALSE, 0, FALSE,                             "luck",                  "the jester"},
	{    roatAff,           APPLY_MANA,  3, 2, FALSE, 0, FALSE,                             "mana",                  "the mystic"},
	{    roatAff,            APPLY_HIT,  3, 2, FALSE, 0, FALSE,                           "health",                  "the badger"},
	{    roatAff,           APPLY_MOVE,  3, 2, FALSE, 0, FALSE,                         "movement",                    "the hare"},
	{    roatAff,             APPLY_AC,  3, 2,  TRUE, 0, FALSE,                       "protection",                   "the golem"},
	{    roatAff,        APPLY_HITROLL, 10, 5, FALSE, 0, FALSE,                         "accuracy",                  "the archer"},
	{    roatAff,        APPLY_DAMROLL, 10, 5, FALSE, 0, FALSE,                            "death",                      "damage"},
	{    roatAff,    APPLY_SAVING_PARA, 15, 7,  TRUE, 0, FALSE,     "protection against paralysis",                    "the weed"},
	{    roatAff,     APPLY_SAVING_ROD, 15, 7,  TRUE, 0, FALSE,          "protection against rods",                   "the steed"},
	{    roatAff,    APPLY_SAVING_FEAR, 15, 7,  TRUE, 0, FALSE, "protection against petrification",                   "the ocean"},
	{    roatAff,  APPLY_SAVING_BREATH, 15, 7,  TRUE, 0, FALSE,        "protection against breath",                    "the king"},
	{    roatAff,   APPLY_SAVING_SPELL, 15, 7,  TRUE, 0, FALSE,         "protection against magic",                "the mountain"},
	{    roatAff,        APPLY_STR_MAX, 60, 7, FALSE, 0, FALSE,                         "strength",                   "the giant"},
	{    roatAff,        APPLY_DEX_MAX, 60, 7, FALSE, 0, FALSE,                        "dexterity",                   "the thief"},
	{    roatAff,        APPLY_INT_MAX, 60, 7, FALSE, 0, FALSE,                     "intelligence",                   "the gnome"},
	{    roatAff,        APPLY_WIS_MAX, 60, 7, FALSE, 0, FALSE,                           "wisdom",                    "the sage"},
	{    roatAff,        APPLY_CON_MAX, 60, 7, FALSE, 0, FALSE,                     "constitution",                   "the troll"},
	{    roatAff,        APPLY_AGI_MAX, 60, 7, FALSE, 0, FALSE,                          "agility",              "the mudskipper"},
	{    roatAff,        APPLY_POW_MAX, 60, 7, FALSE, 0, FALSE,                            "power",                   "the gypsy"},
	{    roatAff,        APPLY_CHA_MAX, 60, 7, FALSE, 0, FALSE,                         "charisma",                "the merchant"},
	{    roatAff,       APPLY_LUCK_MAX, 60, 7, FALSE, 0, FALSE,                             "luck",                  "the jester"},
	{    roatAff,       APPLY_STR_RACE, 70, 8, FALSE, 0, FALSE,                         "strength",                   "the giant"},
	{    roatAff,       APPLY_DEX_RACE, 70, 8, FALSE, 0, FALSE,                        "dexterity",                   "the thief"},
	{    roatAff,       APPLY_INT_RACE, 70, 8, FALSE, 0, FALSE,                     "intelligence",                   "the gnome"},
	{    roatAff,       APPLY_WIS_RACE, 70, 8, FALSE, 0, FALSE,                           "wisdom",                    "the sage"},
	{    roatAff,       APPLY_CON_RACE, 70, 8, FALSE, 0, FALSE,                     "constitution",                   "the troll"},
	{    roatAff,       APPLY_AGI_RACE, 70, 8, FALSE, 0, FALSE,                          "agility",              "the mudskipper"},
	{    roatAff,       APPLY_POW_RACE, 70, 8, FALSE, 0, FALSE,                            "power",                   "the gypsy"},
	{    roatAff,       APPLY_CHA_RACE, 70, 8, FALSE, 0, FALSE,                         "charisma",                "the merchant"},
	{    roatAff,      APPLY_LUCK_RACE, 70, 8, FALSE, 0, FALSE,                             "luck",                  "the jester"}
};

/*randObjAff roaaAffBit2[];
randObjAff roaaAffBit3[];
randObjAff roaaAffBit4[];
randObjAff roaaExtra1[];
randObjAff roaaExtra2[];  // currently nothing applicable in here*/

// user func for testing random object goodness
void Encrypt(char *text, int sizeOfText, const char *key, int sizeOfKey)
{
	int offSet = 0;

	int i = 0;

	for (; i < sizeOfText; ++i, ++offSet)
	{
		if (offSet >= sizeOfKey)
			offSet = 0;

		text[i] = text[i] + key[offSet];
	}
}

void do_randobj(P_char ch, char *strn, int val)
{
	P_obj                           o;
	int                             tmp = 0;
	char                            Gbuf5[MAX_STRING_LENGTH];
	extern const struct class_names class_names_table[];

	if (!IS_TRUSTED(ch))
	{
		send_to_char("Bad mortal or mob.. bad!.\r\n", ch);
		return;
	}

	if (!str_cmp("remove", strn))
	{

		reset_lab(0);
		reset_lab(1);
		reset_lab(2);

		wizlog(56, "Reset map random");
		return;
	}

	if (!str_cmp("map", strn))
	{
		create_lab(0);
		create_lab(1);
		create_lab(2);
		wizlog(56, "Created map random");
		return;
	}

	if (!str_cmp("piece", strn))
	{
		o = create_material(ch, ch);
		obj_to_char(o, ch);
		return;
	}
	if (!str_cmp("stone", strn))
	{
		o = create_stones(ch);
		obj_to_char(o, ch);
		return;
	}
	if (!str_cmp("eq", strn))
	{
		o = create_random_eq_new(ch, ch, -1, -1);
		obj_to_char(o, ch);
		snprintf(Gbuf5, MAX_STRING_LENGTH, "o %s", o->name);
		do_stat(ch, Gbuf5, CMD_STAT);
		return;
	}

	if (!str_cmp("recipe", strn))
	{
		o = random_zone_item(ch);
		if (!o)
		{
			send_to_char("Failed creating random zone item.\n", ch);
			return;
		}

		if (obj_index[o->R_num].virtual_number == VOBJ_RANDOM_ARMOR || obj_index[o->R_num].virtual_number == VOBJ_RANDOM_THRUSTED || obj_index[o->R_num].virtual_number == VOBJ_RANDOM_WEAPON)
		{
			act("Created random object: $q.  Deleting it.", FALSE, ch, o, NULL, TO_CHAR);
			extract_obj(o);
			return;
		}
		create_recipe(ch, o);
		extract_obj(o);

		return;
	}

	if (!str_cmp("mob", strn))
	{
		create_random_mob(-1, 0);
		return;
	}

	if (!str_cmp("zone", strn))
	{
		create_zone(number(0, 2), 999, 999, 999, 999);
		return;
	}

	if (!str_cmp("encrust", strn))
	{
		for (int i = 0; i<10; i++)
		{
			o = random_zone_item(ch);
			if (!o)
				return send_to_char("Failed to create item.\n", ch);
			o->affected[2].location = APPLY_MOVE_REG;
			describe_encrusted_enhanced(o);
			send_to_char_f(ch, "%s\n", o->short_description);
			extract_obj(o);
		}
		return;
	}

	send_to_char("Syntax: randobj <remove, map, stone, piece, eq, recipe, mob, zone, encrust> \r\n", ch);
}

//
//
//

unsigned int getMonsterDiffNumber(unsigned int level, unsigned int zoneDiff)
{
	unsigned int numb;

	if (level <= 10) // 1-10
		numb = 5;
	else if (level <= 20) // 11-20
		numb = 6;
	else if (level <= 30) // 21-30
		numb = 7;
	else if (level <= 35) // 31-35
		numb = 8;
	else if (level <= 40) // 36-40
		numb = 9;
	else if (level <= 45) // 41-45
		numb = 10;
	else if (level <= 50) // 46-50
		numb = 11;
	else if (level <= 55) // 51-55
		numb = 12;
	else if (level <= 59) // 56-59
		numb = 14;
	else if (level <= 60) // 60
		numb = 15;
	else // 61+
		numb = 20;

	numb += number(-1, 1); // slight randomization

	numb *= zoneDiff;

	return numb;
}

//
//
//

P_obj createMundaneItem(P_obj mundane, unsigned int monDiffNumb)
{
	return mundane; // later that same day..
}

//
//
//

randObjAff *getRandObjAff(void) { return &randomObjFieldsArr[number(0, sizeof(randomObjFieldsArr) / sizeof(randomObjFieldsArr[0]) - 1)]; }

//
//
//

randObjAff *getValidRandObjAff(P_obj obj, unsigned int mdn, int rareOnly)
{
	randObjAff *roa;

	if (mdn < 5)
		return NULL;

	while (TRUE)
	{
		roa = getRandObjAff();

		if ((rareOnly || !roa->rareOnly) && (roa->cost <= mdn) && !(obj->wear_flags & roa->restrictedLoc))
			return roa;
	}
}

//
//
//

void applyRandomApplyToObject(P_obj obj, randObjAff *objAff, unsigned int monDiff)
{
	unsigned int i;

	if (!obj || !objAff || (objAff->affType != roatAff))
		return;

	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (obj->affected[i].location == APPLY_NONE)
			break;

	if (i == MAX_OBJ_AFFECT) // no space left in object
		return;

	monDiff -= objAff->cost;

	obj->affected[i].location = objAff->where;
	if (objAff->incCost)
	{
		obj->affected[i].modifier = (monDiff / objAff->incCost) + 1;

		if (objAff->negGood)
			obj->affected[i].modifier = -obj->affected[i].modifier;
	}
}

//
//
//

void applyRandomAffToObject(P_obj obj, randObjAff *objAff, unsigned int monDiff)
{
	if (!obj || !objAff)
		return;

	switch (objAff->affType)
	{
		case roatAffBit1:
			obj->bitvector |= objAff->where;
			break;

		case roatAffBit2:
			obj->bitvector2 |= objAff->where;
			break;

		case roatAffBit3:
			obj->bitvector3 |= objAff->where;
			break;

		case roatAffBit4:
			obj->bitvector4 |= objAff->where;
			break;

		case roatExtra1:
			obj->extra_flags |= objAff->where;
			break;

		case roatExtra2:
			obj->extra2_flags |= objAff->where;
			break;

		case roatAff:
			applyRandomApplyToObject(obj, objAff, monDiff);
			break;
	}
}

//
//
//

P_obj createMagicItem(P_obj magicI, unsigned int monDiffNumb)
{
	randObjAff *objAff;

	objAff = getValidRandObjAff(magicI, monDiffNumb, FALSE);
	if (!objAff)
		return magicI;

	applyRandomAffToObject(magicI, objAff, monDiffNumb);

	// 25% chance of second affect that uses 75% of mon diff numb

	if (number(1, 4) == 1)
	{
		monDiffNumb = MAX(5, (monDiffNumb * 3) / 4);

		objAff = getValidRandObjAff(magicI, monDiffNumb, FALSE);
		if (!objAff)
			return magicI;

		applyRandomAffToObject(magicI, objAff, monDiffNumb);
	}

	return magicI;
}

//
//
//

P_obj createRareItem(P_obj rareI, unsigned int monDiffNumb)
{
	unsigned int numb;
	randObjAff  *objAff;

	// can have 3-4 affs, first two take 100% of diff numb, third takes
	// 75%, fourth (25% chance) takes 60%

	objAff = getValidRandObjAff(rareI, monDiffNumb, TRUE);
	if (!objAff)
		return rareI;

	applyRandomAffToObject(rareI, objAff, monDiffNumb);

	objAff = getValidRandObjAff(rareI, monDiffNumb, TRUE);
	if (!objAff)
		return rareI;

	applyRandomAffToObject(rareI, objAff, monDiffNumb);

	// third affect uses 75% of mon diff numb

	numb = MAX(5, (monDiffNumb * 3) / 4);

	objAff = getValidRandObjAff(rareI, numb, TRUE);
	if (!objAff)
		return rareI;

	applyRandomAffToObject(rareI, objAff, numb);

	// 25% chance of fourth affect that uses 60% of mon diff numb

	if (number(1, 4) == 4)
	{
		numb = MAX(5, (monDiffNumb * 3) / 5);

		objAff = getValidRandObjAff(rareI, numb, TRUE);
		if (!objAff)
			return rareI;

		applyRandomAffToObject(rareI, objAff, numb);
	}

	return rareI;
}

//
//
//

P_obj createSetItem(unsigned int mobDiffNumb)
{
	P_obj setItem;

	// choose from a table..

	return NULL;
}

//
//
//

P_obj createUniqueItem(unsigned int mobDiffNumb)
{
	// choose from a table..

	return NULL;
}

//
// create basic 'type' of random item, i.e. long sword, gold ring, etc
//

P_obj createRandomItemType(void)
{
	P_obj obj;

	obj = read_object(RANDOM_OBJ_VNUM, VIRTUAL);
	if (!obj)
		return NULL;

	// for now, don't do jack - use table of item 'templates', pick one
	// randomly, perhaps weight the table

	return obj;
}

//
// returns NULL if no item created, pointer to new item otherwise
//
//   pc - character killing mob
//   mob - mob killed (may be used for enhanced logging)
//   moblvl - level of monster being killed
//   zoneDiff - difficulty of zone (1-5), 0 and function will figure it out
//   createItem - if TRUE is passed in, always creates an item
//

P_obj createRandomItem(P_char pc, P_char mob, int moblvl, int zoneDiff, int createItem)
{
	int          rareUniqueCh = 100; // 100% = 1000, 10% = 100, 1% = 10, .5% = 5, .1% = 1
	unsigned int monDiffNumb;

	if (!pc || !moblvl)
		return NULL;

	if (GET_LEVEL(mob) > 30)
		return NULL;

	if (GET_LEVEL(pc) > 20) // no randoms from lvl 20+ characters
		return NULL;

	// 40% chance to get something

	if (!createItem && (number(1, 10) > 4))
		return NULL;

	if (!zoneDiff)
		zoneDiff = 3; // needs to be fixed, obviously..

	zoneDiff = BOUNDED(1, zoneDiff, 5);

	// figure out 'monster difficulty number'

	monDiffNumb = getMonsterDiffNumber(moblvl, zoneDiff);

	// see if we create a magic item (25% chance)

	if (number(1, 4) != 1)
		return createMundaneItem(createRandomItemType(), monDiffNumb);

	// okay, we have at least a magic item - see if we get a rare or unique
	// (base chance 10%, altered by PC vs. mob level)

	// alter chance by -.5% for each PC level higher than mob

	if (moblvl < GET_LEVEL(pc))
		rareUniqueCh -= 5 * (GET_LEVEL(pc) - moblvl);

	if (rareUniqueCh <= 1)
		rareUniqueCh = 1;

	// check for rare (base chance 10%, lowest chance .1%)

	if (rareUniqueCh >= number(1, 1000))
	{
		// check for unique/set (base chance 10%, lowest .1%)

		if (rareUniqueCh >= number(1, 1000))
		{
			// we have a unique or set, choose one (50% for either)

			if (number(0, 1))
				return createSetItem(monDiffNumb);
			else
				return createUniqueItem(monDiffNumb);
		}

		return createRareItem(createRandomItemType(), monDiffNumb);
	}

	return createMagicItem(createRandomItemType(), monDiffNumb);
}
