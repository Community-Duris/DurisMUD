/****************************************************************************
 *
 *  File: randomeq.c                                           Part of Duris
 *  Usage: randomboject.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark                   Date: 2002-04-18
 * ***************************************************************************
 */

#define TROPHY

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "defines.h"
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
#include "spells.h"
#include "damage.h"
#include "justice.h"
#include "weather.h"
#include "sound.h"
#include "objmisc.h"
#include "salchemist.h"
#include "spells.h"
#include "epic.h"
#include "specs.prototypes.h"
#include "sql.h"
#include "vnum.obj.h"

/*
 * external variables
 */
extern char *spells[];
extern Skill skills[];
extern struct material_data materials[];
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern char debug_mode;
extern const char *race_types[];

//extern const int material_absorbtion[][];
extern const struct stat_data stat_factor[];
extern float fake_sqrt_table[];
extern int pulse;
extern int arena_hometown_location[];
extern struct arena_data arena;
extern struct agi_app_type agi_app[];
extern struct dex_app_type dex_app[];
extern struct message_list fight_messages[];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;

P_obj    set_encrust_affect(P_obj obj, int proc);

struct potion potion_data[] = {
  {SPELL_GREATER_LIVING_STONE, 51,
   {LIVING_STONE, FAERIE_DUST, DRAGONS_BLOOD, BONE}, FIRST_POTION_VIRTUAL},
  {SPELL_STRONG_ACID, 41,
   {BONE, DRAGONS_BLOOD, NIGHTSHADE, GREEN_HERB, LIVING_STONE},
   FIRST_POTION_VIRTUAL + 3},
  {SPELL_ENTANGLE, 41, {BONE, GREEN_HERB, GARLIC, MANDRAKE_ROOT},
   FIRST_POTION_VIRTUAL + 4},
  {SPELL_GLASS_BOMB, 36,
   {MANDRAKE_ROOT, GARLIC, GREEN_HERB, DRAGONS_BLOOD, NIGHTSHADE},
   FIRST_POTION_VIRTUAL + 5},
  {SPELL_FLY, 36,
   {MANDRAKE_ROOT, GARLIC, GREEN_HERB, FAERIE_DUST, NIGHTSHADE},
   FIRST_POTION_VIRTUAL + 6},
  {SPELL_NAPALM, 31, {GARLIC, DRAGONS_BLOOD, MANDRAKE_ROOT, NIGHTSHADE},
   FIRST_POTION_VIRTUAL + 7},
  {SPELL_FEEBLEMIND, 31, {GARLIC, GREEN_HERB, FAERIE_DUST, NIGHTSHADE},
   FIRST_POTION_VIRTUAL + 8},
  {SPELL_GREASE, 26, {GARLIC, DRAGONS_BLOOD, GREEN_HERB, NIGHTSHADE},
   FIRST_POTION_VIRTUAL + 9},
  {SPELL_LIVING_STONE, 26,
   {LIVING_STONE, FAERIE_DUST, DRAGONS_BLOOD, MANDRAKE_ROOT},
   FIRST_POTION_VIRTUAL + 11},
  {SPELL_SLOW, 21, {DRAGONS_BLOOD, FAERIE_DUST, GREEN_HERB, NIGHTSHADE},
   FIRST_POTION_VIRTUAL + 13},
  {SPELL_WITHER, 16, {NIGHTSHADE, GREEN_HERB, MANDRAKE_ROOT},
   FIRST_POTION_VIRTUAL + 15},
  {SPELL_DISPEL_MAGIC, 11, {NIGHTSHADE, GARLIC, MANDRAKE_ROOT},
   FIRST_POTION_VIRTUAL + 16},
  {SPELL_FAERIE_FIRE, 11, {GREEN_HERB, GARLIC, FAERIE_DUST},
   FIRST_POTION_VIRTUAL + 17},
  {SPELL_NITROGEN, 6, {GREEN_HERB, GARLIC}, FIRST_POTION_VIRTUAL + 18},
  {0}
};


int basic_ingredients[] = { VOBJ_INGRED_NIGHTSHADE, VOBJ_INGRED_MANDRAKE, VOBJ_INGRED_GARLIC, VOBJ_INGRED_FAERIE_DUST,
      VOBJ_INGRED_DRAGON_BLOOD, VOBJ_INGRED_GREEN_HERB, VOBJ_INGRED_STRANGE_STONE, VOBJ_INGRED_HUMAN_BONE };

const char *encrust_color_list[] = {
  "",
  "&+G",
  "&+R",
  "&+Y",
  "&+B",
  "&+L",
  "&+M",
  "&+r",
  "&+b",
  "&+m",
  " "
};


#define MAX_COMPONENTS_FOR_WEAPON 5
#define MAX_MATERIAL_QUALITY 5
#define WEAPON_QUALITY_DESCRIPTIONS_NUMBER 7
#define MIN_WEAPON_HARDINESS 3
#define MAX_BASE_POWER 12
//this is the main parameter to tweak the dice power of forged weapons
//12 == 4d5, the base power is increased by weapon type modifiers and
//forging the best weapon for the given material type

#define NO_WEAPON 1
#define LONGSWORD 2
#define SHORTSWORD 4
#define MACE 8
#define DAGGER 16
#define STAFF 32
#define HAMMER 64
#define AXE 128

#define WHIP 512
#define MIN_ELASTIC_WEAPON WHIP

extern P_index obj_index;

struct weapon_type
{
  char    *name;
  int      type;
  int      number_of_components;
  int      preferred_dice;
  int      power_mod;
  int      magic_mod;
  int      damage_type;
};

struct weapon_type crafted_weapon_types[] = {
  {"longsword", LONGSWORD, 3, 6, 4, 0, WEAPON_LONGSWORD},
  {"shortsword", SHORTSWORD, 2, 5, 2, 1, WEAPON_SHORTSWORD},
  {"axe", AXE, 3, 10, 5, 0, WEAPON_AXE},
  {"mace", MACE, 2, 4, 1, 2, WEAPON_MACE},
  {"dagger", DAGGER, 2, 4, 2, 2, WEAPON_DAGGER},
  {"staff", STAFF, 2, 4, -4, 4, WEAPON_STAFF},
  {"hammer", HAMMER, 2, 10, 3, 1, WEAPON_HAMMER},
  {"whip", WHIP, 2, 6, 1, 3, WEAPON_WHIP},
  {0}
};

struct material
{
  int      hardiness;
  int      magic_power;
  int      preferred_weapon;
  char    *adjective;
};

char    *weapon_quality[WEAPON_QUALITY_DESCRIPTIONS_NUMBER] = {
  "a broken",
  "a cracked",
  "a primitive",
  "a plain",
  "a decent",
  "a fine",
  "an excellent"
};

struct material material_parameters[NUMB_MATERIALS] = {
  {0, 0},                       //"UNDEFINED",
  {0, 0, NO_WEAPON},            //"NONSUBSTANTIAL",
  {0, 1, WHIP},                 //"FLESH",
  {-2, 1, WHIP},                //"CLOTH",
  {-4, 1},                      //"BARK",
  {0, 1, 0, "&+ywooden&n"},     //"SOFTWOOD",
  {1, 1, STAFF, "&+ywooden&n"}, //"HARDWOOD",
  {-2, 1, NO_WEAPON},           //"SILICON",
  {2, 2, DAGGER},               //"CRYSTAL",
  {-4, 1, NO_WEAPON},           //"CERAMIC",
  {0, 1, MACE},                 //"BONE",
  {1, 1, HAMMER},               //"STONE",
  {0, 1, WHIP},                 //"HIDE",
  {1, 1, WHIP},                 //"LEATHER",
  {2, 1, WHIP},                 //"CURED_LEATHER",
  {1, 1, MACE},                 //"IRON",
  {2, 1, LONGSWORD},            //"STEEL",
  {1, 1, MACE},                 //"BRASS",
  {3, 3, LONGSWORD | SHORTSWORD},       //"MITHRIL",
  {3, 2, LONGSWORD | SHORTSWORD},       //"ADAMANTIUM",
  {1, 1, MACE},                 //"BRONZE",
  {0, 1, MACE},                 //"COPPER",
  {0, 2, SHORTSWORD},           //"SILVER",
  {0, 2, SHORTSWORD},           //"ELECTRUM",
  {0, 2, SHORTSWORD, "&+Ygolden&n"},    //"GOLD",
  {1, 2, SHORTSWORD},           //"PLATINUM",
  {1, 2, DAGGER},               //"GEM",
  {3, 2, LONGSWORD | SHORTSWORD},       //"DIAMOND",
  {-10, 1, WHIP},               //"PAPER",
  {-10, 1, NO_WEAPON},          //"PARCHMENT",
  {0, 1, NO_WEAPON},            //"LEAVES",
  {1, 2, DAGGER | SHORTSWORD},  //"RUBY",
  {1, 3, DAGGER | SHORTSWORD},  //"EMERALD",
  {1, 2, DAGGER | SHORTSWORD},  //"SAPPHIRE",
  {-4, 3},                      //"IVORY",
  {3, 4, NO_WEAPON},            //"DRAGONSCALE",
  {1, 2, DAGGER},               //"OBSIDIAN",
  {1, 2, HAMMER},               //"GRANITE",
  {-2, 2},                      //"MARBLE",
  {-2, 2},                      //"LIMESTONE",
  {0, 0, NO_WEAPON},            //"LIQUID",
  {-5, 1},                      //"BAMBOO",
  {-6, 2, NO_WEAPON},           //"REEDS",
  {-4, 2, WHIP},                //"HEMP",
  {0, 0, NO_WEAPON},            //"GLASSTEEL",
  {0, 1, NO_WEAPON},            //"EGGSHELL",
  {0, 1, NO_WEAPON},            //"CHITINOUS",
  {0, 0, NO_WEAPON},            //"REPTILESCALE",
  {0, 0, NO_WEAPON},            //"GENERICFOOD",
  {-4, 0},                      //"RUBBER",
  {0, 2, NO_WEAPON},            //"FEATHER",
  {0, 2, NO_WEAPON},            //"WAX",
  {-8, 4},                      //"PEARL",
  {2, 1, LONGSWORD}             //"TIN"
};


struct weapon_type *lookup_weapon(char *name)
{
  int      i;

  for (i = 0; crafted_weapon_types[i].name; i++)
  {
    if(strstr(name, crafted_weapon_types[i].name))
    {
      return &(crafted_weapon_types[i]);
    }
  }

  return NULL;
}

int get_components(P_char ch, P_obj buffer[], int vnum, int max_components)
{
  P_obj    t_obj, next_obj;
  int      found = 0;

  for (t_obj = ch->carrying; t_obj && found < max_components;
       t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    if(obj_index[t_obj->R_num].virtual_number == vnum &&
        isname("piece", t_obj->name))
    {
      buffer[found++] = t_obj;
    }
  }

  return found;
}

int get_piece_quality(P_obj t_obj)
{
  if(strstr(t_obj->name, "cracked"))
    return 1;
  if(strstr(t_obj->name, "crude"))
    return 2;
  if(strstr(t_obj->name, "decent"))
    return 3;
  if(strstr(t_obj->name, "Fine"))
    return 4;
  if(strstr(t_obj->name, "Superior"))
    return 5;

  return 0;
}

void set_keywords(P_obj t_obj, const char *newKeys)
{
  if((t_obj->str_mask & STRUNG_KEYS) && t_obj->name)
  {
    FREE(t_obj->name);
  }
  t_obj->name = NULL;
  t_obj->str_mask |= STRUNG_KEYS;
  t_obj->name = str_dup(newKeys);
}

void set_short_description(P_obj t_obj, const char *newShort)
{

  if((t_obj->str_mask & STRUNG_DESC2) && t_obj->short_description)
  {
    FREE(t_obj->short_description);
  }
  t_obj->short_description = NULL;
  t_obj->str_mask |= STRUNG_DESC2;
  t_obj->short_description = str_dup(newShort);
}

void set_long_description(P_obj t_obj, const char *newDescription)
{
  if((t_obj->str_mask & STRUNG_DESC1) && t_obj->description)
  {
    FREE(t_obj->description);
  }
  t_obj->description = NULL;
  t_obj->str_mask |= STRUNG_DESC1;
  t_obj->description = str_dup(newDescription);
}


int get_id_for(P_obj t_obj)
{
  int      i;

  for (i = 0; i < LAST_BASIC_INGREDIENT; i++)
  {
    if(basic_ingredients[i] == obj_index[t_obj->R_num].virtual_number)
    {
      return i + 1;
    }
  }
  if(obj_index[t_obj->R_num].virtual_number != 8)
  {
    return WRONG_INGREDIENT;
  }

  if(strstr(t_obj->name, "bowels"))
  {
    return BOWELS;
  }
  else if(strstr(t_obj->name, "face"))
  {
    return FACE;
  }
  else if(strstr(t_obj->name, "eyes"))
  {
    return EYES;
  }
  else if(strstr(t_obj->name, "legs"))
  {
    return LEGS;
  }
  else if(strstr(t_obj->name, "arms"))
  {
    return ARMS;
  }
  else if(strstr(t_obj->name, "tongue"))
  {
    return TONGUE;
  }
  else if(strstr(t_obj->name, "scalp"))
  {
    return SCALP;
  }
  else if(strstr(t_obj->name, "skull"))
  {
    return SKULL;
  }
  else if(strstr(t_obj->name, "ears"))
  {
    return EARS;
  }

  return WRONG_INGREDIENT;
}

int got_all_ingredients(P_char ch, int required[])
{
  int      found[MAX_INGREDIENTS + 1];
  int      i;
  int      temp;
  P_obj    t_obj, next_obj;
  int      object_id;

  for (i = 0; i < MAX_INGREDIENTS + 1; i++)
  {
    found[i] = required[i];
  }

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    object_id = get_id_for(t_obj);

    for (i = 0; i < MAX_INGREDIENTS + 1; i++)
    {
      if(found[i] == object_id)
      {
        found[i] = 0;
        break;
      }
    }
  }

  for (i = 0; i < MAX_INGREDIENTS + 1; i++)
  {
    if(found[i])
    {
      return 0;
    }
  }

  notch_skill(ch, SKILL_MIX, 6.25);

  return 1;
}

void extract_used_ingredients(P_char ch, int ingredients[])
{
  int      found[MAX_INGREDIENTS + 1];
  P_obj    used_objs[MAX_INGREDIENTS + 1];
  P_obj    t_obj, next_obj;
  int      object_id;
  int      i;

  for (i = 0; i < MAX_INGREDIENTS + 1; i++)
  {
    found[i] = ingredients[i];
    used_objs[i] = NULL;
  }

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;

    object_id = get_id_for(t_obj);

    for (i = 0; i < MAX_INGREDIENTS + 1; i++)
    {
      if(found[i] == object_id)
      {
        found[i] = 0;
        used_objs[i] = t_obj;
        break;
      }
    }
  }

  for (i = 0; i < MAX_INGREDIENTS + 1; i++)
  {
    if(used_objs[i])
    {
      extract_obj(used_objs[i]);
    }
  }
}

P_obj get_bottle(P_char ch)
{
  P_obj    t_obj, next_obj;

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    if(obj_index[t_obj->R_num].virtual_number == BOTTLE_VIRTUAL && strstr(t_obj->name, "bottle"))
    {
      return t_obj;
    }
  }

  return NULL;
}

void do_mix(P_char ch, char *argument, int cmd)
{
  P_obj    bottle;
  char     arg[MAX_STRING_LENGTH];
  int      i;
  int      x = 1;
  char     Gbuf2[MAX_STRING_LENGTH];

  if(!GET_CHAR_SKILL(ch, SKILL_MIX))
  {
    act("Well trying might not hurt, but dont.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  CharWait(ch, PULSE_VIOLENCE * 1);
  one_argument(argument, arg);
  if(*arg)
  {
    act("Get a potion bottle, and have the garlic and stuff in inventory and type mix!",
       FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  bottle = get_bottle(ch);

  if(!bottle)
  {
    act("You need to have a potion bottle in your inventory.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  for (i = 0; potion_data[i].spell_type; i++)
  {
    if(GET_LEVEL(ch) >= potion_data[i].spell_level &&
        got_all_ingredients(ch, potion_data[i].ingredients))
    {
      while (TRUE)
      {
        P_obj    potion;
        char gbuf2[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];


      if(number(1, 160) < ((GET_C_WIS(ch) + GET_C_DEX(ch)) / 2))
      {
        potion = read_object(potion_data[i].vnum, VIRTUAL);
        potion->value[0] = GET_LEVEL(ch);
        act("You've &+Wcreated&n $p.", FALSE, ch, potion, 0, TO_CHAR);
        sprintf(gbuf2, "%s %s", GET_NAME(ch), potion->name);
        potion->name = str_dup(gbuf2);
        sprintf(buffer, "%s mixed by %s", potion->short_description, GET_NAME(ch));
        set_short_description(potion, buffer);
        obj_to_char(potion, ch);
       }
        else
       {
        act("&+RYou clumsily spill your ingredients everywhere, ruining your creation!", FALSE, ch, 0, 0, TO_CHAR);
       }
        extract_obj(bottle);

        if(number(0, 5))
        {
          bottle = read_object(835, VIRTUAL);
          obj_to_char(bottle, ch);
        }

        if(number(0, (GET_CHAR_SKILL(ch, SKILL_MIX) / 10 + number(1, 2))))
        {
          bottle = get_bottle(ch);
          if(!bottle)
          {
            act
              ("But cant make more since you dont have any more bottles on you!",
               FALSE, ch, 0, 0, TO_CHAR);
            break;
          }
        }
        else
        {
          act("You wasted all your ingredients\r\n", FALSE, ch, 0, 0,
              TO_CHAR);
          extract_used_ingredients(ch, potion_data[i].ingredients);
          break;
        }
      }
      notch_skill(ch, SKILL_MIX, 6.25);
      CharWait(ch, PULSE_VIOLENCE * 2);

      return;
    }
  }

  act("No potion created in the bottle!\r\n", FALSE, ch, 0, 0, TO_CHAR);
  return;

}


bool is_neg_good(sbyte location) {
  switch(location) {
    case APPLY_SAVING_PARA:
    case APPLY_SAVING_ROD:
    case APPLY_SAVING_FEAR:
    case APPLY_SAVING_BREATH:
    case APPLY_SAVING_SPELL:
    case APPLY_ARMOR:
      return TRUE;
    default:
      return FALSE;
  }
}

void do_spellbind (P_char ch, char *argument, int cmd)
{
  char     arg[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  char     tempbuf[MAX_STRING_LENGTH];
  struct affected_type *afp;
  P_obj item;
  bool neggood;
  int total_epic_points = GET_EPIC_POINTS(ch);
  int bonus;
  int skill = GET_CHAR_SKILL(ch, SKILL_SPELLBIND);
  int total = 0;

  if( !IS_ALIVE(ch) )
    return;

  if(!GET_CHAR_SKILL(ch, SKILL_SPELLBIND))
  {
    act("You do not have the epic skill to spellbind.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  sprintf(buf , "You have %d spellbinds left...\n" , (int)
    (total_epic_points));

  send_to_char(buf, ch);

  if(total_epic_points < 1)
  {
    act("You do not have enough power...", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  argument = one_argument(argument, arg);
  
  if(!*arg)
  {
    act("What item do you want to to spellbind?",
      FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(!(item = get_obj_in_list_vis(ch, arg, ch->carrying)))
  {
    act("What item do you do you want to spellbind?",
      FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
  if(IS_SET(item->extra_flags, ITEM_NOREPAIR))
  {
    act("You don't seem able to affect that item...", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(item->condition <= 0)
    return; // Bad! 
  
  if(item->condition <= 99)
  {
    send_to_char("This item is &=LRnot&n in perfect condition. Please fix it, first.\r\n", ch);
    return;
  }

  if(IS_ARTIFACT(item) &&
    !IS_TRUSTED(ch))
  {
    send_to_char("Enchanting this item is far beyond the abilities of any mere mortal!\r\n", ch);
    return;
  }

  if(skill < number(10, 140))
  {
    act("You failed to spellbind this item ...", FALSE, ch, 0, 0, TO_CHAR);
    if(!number(0, 4))
    {
      act("... and severely damage it.", FALSE, ch, 0, 0, TO_CHAR);
      SET_BIT(item->extra_flags, ITEM_NOREPAIR);
      item->condition = item->condition - number(1,20);
    }
    return;
  }

  act("You successfully bind some magic to the item...", FALSE, ch, 0, 0, TO_CHAR);

  //epic_gain_skillpoints(ch, -1);
  ch->only.pc->epics -= 1;

  if(!number(0, 2) && !IS_TRUSTED(ch))
  {
    item->condition = item->condition - number(1, 20);
    act("...however, you damaged it a bit...", FALSE, ch, 0, 0, TO_CHAR);
  }
  // Once in a while give a bonus to skill -- Unless you're an immortal, then you always get the bonus.
  else if(!number(0, 40) || IS_TRUSTED(ch))
  {
    setsuffix_obj_new(item);
    act("...you feel &+YSTRONG&n magic flowing this time...", FALSE, ch, 0, 0, TO_CHAR);
    total += 1;
  }

  SET_BIT(item->extra_flags, ITEM_NOREPAIR);

  // The nolocate flag allows imms to lookup. Not creating a new flag.
  if(!IS_SET(item->extra_flags, ITEM_NOLOCATE)) 
    SET_BIT(item->extra_flags, ITEM_NOLOCATE);

  for (int i = 0; i < 3; i++)
  {
    bonus = 0;
    total = 0;
    neggood = is_neg_good(item->affected[i].location);
    
// Don't want to mess with race max, pulse or bad applies etc...
    if(item->affected[i].location >= APPLY_LUCK_MAX)
      continue;

// Applying the bonus to a zero value is pointless.
    if(item->affected[i].modifier == 0) 
      continue;

// High and low values are hazardous since the values
// roll over into positive and negative.
    if(item->affected[i].modifier >= 120) 
      continue;
    
    if(item->affected[i].modifier <= -120) 
      continue;
    
    bonus = (int) (skill / 20);

// Limiting stat max modifiers otherwise it'll get out of
// control real quickly.
    if(is_stat_max(item->affected[i].location))
    {
      total += item->affected[i].modifier + ((int) (skill / 50));
    }
    else if(neggood)
    {
      if(item->affected[i].location >= APPLY_SAVING_PARA &&
         item->affected[i].location <= APPLY_SAVING_SPELL)
      {
        total = item->affected[i].modifier + ((total * -1) - (skill / 50));
      }
      else if(item->affected[i].location == APPLY_ARMOR)
      {
        total = (int) (item->affected[i].modifier * ((int) (skill / 50) + total));
      }
      else
      {
        total = (int) (item->affected[i].modifier - (-total - bonus));      
      }
    }
    else
    {
      total = (int) (item->affected[i].modifier + total + bonus);
    }
    item->affected[i].modifier = total;
  }

  sprintf(tempbuf , "%s %s", item->name, GET_NAME(ch));
  set_keywords(item, tempbuf);
  if(RACE_GOOD(ch))
  sprintf(tempbuf , "%s &+yenc&+Yha&+ynted by &+L%s&n", item->short_description, GET_NAME(ch));
  else if(RACE_EVIL(ch))
  sprintf(tempbuf , "%s &+renc&+Rha&+rnted by &+L%s&n", item->short_description, GET_NAME(ch));
  else
  sprintf(tempbuf , "%s &+wenc&+Wha&+wnted by &+L%s&n", item->short_description, GET_NAME(ch));
  set_short_description(item, tempbuf);
}

// Jewel list is located in randomeq.c under stone_list and stone_spell_list

void do_encrust(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_STRING_LENGTH];
  char     arg2[MAX_STRING_LENGTH];
  char     buf1[MAX_STRING_LENGTH];
  char     buf2[MAX_STRING_LENGTH];
  int      skill = 0;
  int craftsmanship;

  P_obj    item;
  P_obj    jewel;

  if( !GET_CHAR_SKILL(ch, SKILL_ENCRUST) )
  {
    act("Leave this to a real artisan.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  skill = GET_CHAR_SKILL(ch, SKILL_ENCRUST);

  argument = one_argument(argument, arg);
  if(!*arg)
  {
    act("What item do you wish to encrust?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  item = get_obj_in_list_vis(ch, arg, ch->carrying);

  if(!item)
  {
    act("What item do you wish to encrust?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_SET(item->extra_flags, ITEM_NOREPAIR))
  {
    send_to_char("You cannot encrust an item that cannot be repaired.", ch);
    return;
  }

  if(IS_ARTIFACT(item))
  {
    act("You are unable to encrust an artifact.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(isname("encrust", item->name))
  {
    act("You may not further encrust this item.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  one_argument(argument, arg2);

  if(!*arg2)
  {
    act("With what jewel?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  jewel = get_obj_in_list_vis(ch, arg2, ch->carrying);

   //Add a check if this is a encrustable item...
  if(!jewel)
  {
    act("What item do you wish to encrust?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if(jewel->value[6] == 0 && obj_index[jewel->R_num].virtual_number != RANDOM_OBJ_VNUM)
  {
    act("Is THAT a jewel?!?!?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (jewel == item)
  {
  	act("Try finding a suitable item to encrust with, bub.", FALSE, ch, 0, 0, TO_CHAR);
  	return;
  }

  sprintf(buf2, "%s attempts to encrust %s with %s...", GET_NAME(ch),
          item->short_description, jewel->short_description);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  wizlog(56, buf2 );

  craftsmanship = item->craftsmanship;
//  if(!item->value[5] || !item->value[6] || !item->value[7])
  {

    item->value[5] = jewel->value[6];
    item->value[6] = GET_LEVEL(ch);
    item->value[7] = 30;

  }

  if(number(1, 110) > skill)
  {
    act("You broke your item in the process.", FALSE, ch, 0, 0, TO_CHAR);
    act("...and breaks it in the process.", TRUE, ch, 0, 0, TO_ROOM);
    extract_obj(item);
    extract_obj(jewel);
    wizlog(56, "and ruined it in the process...");
    return;
  }
  else
  {
    //notch_skill(ch, SKILL_ENCRUST, 7.7);
    act("...creating a real masterpiece!", TRUE, ch, 0, 0, TO_ROOM);
    act("Hurrah! Hurrah!", FALSE, ch, 0, 0, TO_CHAR);
  }

  P_obj new_item = read_object(1251, VIRTUAL);

  if(!new_item)
  {
    return;
  }

  new_item->material = item->material;
  new_item->type = item->type;
  new_item->weight = item->weight+1;
  new_item->affected[0].location = item->affected[0].location;
  new_item->affected[0].modifier = item->affected[0].modifier;
  new_item->affected[1].location = item->affected[1].location;
  new_item->affected[1].modifier = item->affected[1].modifier;

  SET_BIT(new_item->wear_flags, ITEM_TAKE);
  SET_BIT(new_item->wear_flags, item->wear_flags);

  SET_BIT(new_item->bitvector, item->bitvector);
  SET_BIT(new_item->bitvector2, item->bitvector2);
  SET_BIT(new_item->bitvector3, item->bitvector3);
  SET_BIT(new_item->bitvector4, item->bitvector4);
  new_item->anti_flags |= item->anti_flags;
  new_item->anti2_flags |= item->anti2_flags;
  SET_BIT(new_item->extra_flags, item->extra_flags);
  SET_BIT(new_item->extra2_flags, item->extra2_flags);

  new_item->craftsmanship = MIN(craftsmanship+1, OBJCRAFT_HIGHEST);

  int i = 0;
  for(i;i < 7;i++)
  {
    new_item->value[i] = item->value[i];
  }

  SET_BIT(new_item->extra_flags, ITEM_ENCRUSTED | ITEM_NOREPAIR);
  SET_BIT(new_item->type, item->type);

  sprintf(buf1, "%s with %s", str_dup(item->short_description), str_dup(jewel->short_description));
  set_short_description(new_item, buf1);
  sprintf(buf1, "%s", str_dup(item->description));
  set_long_description(new_item, buf1);
  sprintf(buf1, "%s %s", item->name, "encrust");
  set_keywords(new_item, buf1);

  set_encrust_affect(new_item, jewel->value[6]);
  extract_obj(item);
  extract_obj(jewel);
  obj_to_char(new_item, ch);

  wizlog(56, "and created %s", new_item->short_description);

  return;
}

int encrusted_eq_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   kala;
  int      j, dam;

  if(cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( cmd != CMD_MELEE_HIT )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || IS_IMMOBILE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  kala = (P_char) arg;
  if( !IS_ALIVE(kala) )
  {
    return FALSE;
  }

  j = obj->value[5];

  // 1/30 chance.
  if( !number(0, 29) )
  {
    if( j != SPELL_ENERGY_DRAIN && j != 0 )
    {
     // validation
      if( j >= MAX_AFFECT_TYPES || *skills[j].spell_pointer == 0 )
      {
        return FALSE;
      }

      act("$n's&N $q &n&+Lmurmurs some strange incantations...&N", TRUE, ch, obj, kala, TO_NOTVICT);
      act("Your&N $q &n&+Lmurmurs some strange incantations...&N", TRUE, ch, obj, kala, TO_CHAR);
      act("$n's&N $q &n&+Lmurmurs some strange incantations...&N", TRUE, ch, obj, kala, TO_VICT);
      ((*skills[j].spell_pointer) ((int) obj->value[6], ch, 0, SPELL_TYPE_SPELL, kala, obj));
    }
    else if(j == SPELL_ENERGY_DRAIN && GET_HIT(ch) < GET_MAX_HIT(ch)
      && !IS_UNDEADRACE(kala) && !IS_AFFECTED4(kala, AFF4_NEG_SHIELD) && !resists_spell(ch, kala) )
    {
      act("&+LYour encrusted &+rbloodstone &+Rglows brightly&+L, and your vision is &+rbathed in &+Rred&+L.&n", FALSE, ch, 0, 0, TO_CHAR);
      act("$n&+L's encrusted &+rbloodstone &+Rglows&+L, and $s &+reyes &+Lare &+rbathed in &+Rred&+L.&n", FALSE, ch, 0, 0, TO_ROOM);
      dam = number(10, 30);
      vamp(ch, dam, GET_MAX_HIT(ch));
      melee_damage(ch, kala, dam, 0, 0);
    }
  }
  return TRUE;
}

void do_fix(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_STRING_LENGTH];
  char     arg2[MAX_STRING_LENGTH];
  char     buf1[MAX_STRING_LENGTH];
  int      skill = 0;

  P_obj    item;

  if(!GET_CHAR_SKILL(ch, SKILL_FIX))
  {
    act("Leave this to a real artisan.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  skill = GET_CHAR_SKILL(ch, SKILL_FIX);

  argument = one_argument(argument, arg);
  if(!*arg)
  {
    act("What item do you wish to fix?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  item = get_obj_in_list_vis(ch, arg, ch->carrying);

  if(!item)
  {
    act("What item do you wish to fix?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if(IS_SET(item->extra_flags, ITEM_NOREPAIR))
  {
    act("The power within this item is far beyond your ability", FALSE, ch, 0,
        0, TO_CHAR);
    return;
  }

  if(item->condition > 90)
  {
    act("That item seems good enough already!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if(item->condition < 20)
  {
    act("That item seems to be destroyed, not even you can fix that!", FALSE,
        ch, 0, 0, TO_CHAR);
    return;
  }

  //fix now requires a salvaged material piece from same material to fix
  P_obj t_obj, nextobj;
  char gbuf1[MAX_STRING_LENGTH];
  int i = 0;
  int imat = get_matstart(item);
  for (t_obj = ch->carrying; t_obj; t_obj = nextobj)
  {
    nextobj = t_obj->next_content;

    if(OBJ_VNUM(t_obj) == imat)
    {
      i++;
    }

  }
  P_obj needed = read_object(imat, VIRTUAL);
  if (i < 1)
  {
    sprintf(gbuf1, "You must have %s in your inventory to repair that item.\r\n", needed->short_description);
    send_to_char(gbuf1, ch);
    extract_obj(needed);
    return;
  }
  extract_obj(needed);
  //endmaterial check

  if(skill > number(0, 105))
  {
    act("$N fiddles with $p, fixing it quickly!", TRUE, ch, item, ch, TO_ROOM);
    act("You fiddle with $p, fixing it quickly!", TRUE, ch, item, 0, TO_CHAR);
    item->condition = 100 - number(0, 10);
  }
  else
  {
    item->condition = item->condition - number(2, 30);
    if(item->condition < 1)
    {
      act("$N fiddles with $p, but fails, destroying it!", TRUE, ch, item, ch, TO_ROOM);
      act("You fiddle with $p, but you fail, destroying it!", TRUE, ch, item, 0, TO_CHAR);
      item = NULL;
    }
    else
    {
      act("$N fiddles with $p, but fails, breaking it even more!", TRUE, ch,
          item, ch, TO_ROOM);
      act("You fiddle with $p, but you fail, breaking it even more!", TRUE, ch,
          item, 0, TO_CHAR);
    }
  }
  int done = 0;
  while(done < 1)
  {
    for (t_obj = ch->carrying; t_obj; t_obj = nextobj)
    {
      nextobj = t_obj->next_content;
      if(OBJ_VNUM(t_obj) == imat)
      {
        obj_from_char(t_obj);
        extract_obj(t_obj);
        done++;
      }
    }
  }
}

P_obj check_furnace(P_char ch)
{
  P_obj t_obj;

  for (t_obj = world[ch->in_room].contents; t_obj; t_obj = t_obj->next_content)
  {
    if(obj_index[t_obj->R_num].virtual_number == 361)
    {
      return t_obj;
    }
  }

  return NULL;
}

void do_smelt(P_char ch, char *arg, int cmd)
{
  P_obj furnace, first_obj, second_obj, ash, new_obj;

  if(!GET_CHAR_SKILL(ch, SKILL_SMELT)) {
         act("&+LWell trying might not hurt, but dont.&n", FALSE, ch, 0, 0, TO_CHAR);
         return;
      }
    furnace = check_furnace(ch);

    if(!furnace) { // No furnace in room
      act("&+LYou seem to be missing a vital part, wonder what it can be...&n", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }

    first_obj = furnace->contains;

    // Nothing inside!
    if(!first_obj || IS_ARTIFACT(first_obj) )
    {
      act("&+LNothing seems to happen!&n", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }

    second_obj = first_obj->next_content;

    // There is only 1 items inside, destroy it!
    if(!second_obj)
    {
      extract_obj(first_obj);
      act("&+LThe furnace roars with activity!&n", FALSE, ch, 0, 0, TO_CHAR);
      act("&+LYou destroy everything in the furnace!&n", FALSE, ch, 0, 0, TO_CHAR);
      act("&+L$p &+Lmakes a roaring sound!&n", FALSE, 0, furnace, 0, TO_ROOM);
      ash = read_object(2670, VIRTUAL);
      obj_to_obj(ash, furnace);
      return;
    }
    // There is more than 2 items inside
    else if(second_obj->next_content)
    {
      act("&+LThe furnace is too full to operate.&n", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    else if( IS_ARTIFACT(second_obj) )
    {
      act("&+LNothing seems to happen!&n", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    // 2 items inside
    else
    {

      new_obj = NULL;

      if(first_obj->R_num == second_obj->R_num) { // 2 identical items
        switch(obj_index[first_obj->R_num].virtual_number) {
        case 400260: //small iron
          new_obj = read_object(400261, VIRTUAL); //medium iron
          break;
        case 400261: //medium iron
          new_obj = read_object(400262, VIRTUAL); //large iron
          break;
        case 400263: //small TIN
          new_obj = read_object(400264, VIRTUAL); //medium steel
          break;
        case 400264: //medium TIN
          new_obj = read_object(400265, VIRTUAL); //large steel
          break;
        case 400266: //small copper
          new_obj = read_object(400267, VIRTUAL); //medium copper
          break;
        case 400267: //medium copper
          new_obj = read_object(400268, VIRTUAL); //large copper
          break;
        case 400269: //small silver
          new_obj = read_object(400270, VIRTUAL); //medium silver
          break;
        case 400270: //medium silver
          new_obj = read_object(400271, VIRTUAL); //large silver
          break;
        case 400272: //small gold
          new_obj = read_object(400273, VIRTUAL); //medium gold
          break;
        case 400273: //medium gold
          new_obj = read_object(400274, VIRTUAL); //large gold
          break;
        case 400275: //small platinum
          new_obj = read_object(400276, VIRTUAL); //medium platinum
          break;
        case 400276: //medium platinum
          new_obj = read_object(400277, VIRTUAL); //large platinum
          break;
        case 400278: //small mithril
          new_obj = read_object(400279, VIRTUAL); //medium mithril
          break;
        case 400279: //medium mithril
          new_obj = read_object(400280, VIRTUAL); //large mithril
          break;
        case 400281: //small adamantium
          new_obj = read_object(400282, VIRTUAL); //medium adamantium
          break;
        case 400282: //medium adamantium
          new_obj = read_object(400283, VIRTUAL); //large adamantium
          break;
        }
      }

      if(!new_obj) {
        if(strstr(first_obj->name, "_ore_") || strstr(second_obj->name, "_ore_")) { // at least one item is ore
        new_obj = read_object(362, VIRTUAL); //Alloy
        }
        else {
        new_obj = read_object(2670, VIRTUAL); //Ash
        }
      }

        if(!number(0,2)) {
            if(GET_CHAR_SKILL(ch, SKILL_SMELT) < number(0, 99))
            {
              send_to_char("&+LYou decide you shoulda been a carpenter, because you mess up while smelting!&n", ch);
              new_obj = read_object(362, VIRTUAL); //Alloy
            }
      }

      extract_obj(first_obj);
      extract_obj(second_obj);
      obj_to_obj(new_obj, furnace);

      act("&+LThe furnace hums with activity!&n", FALSE, ch, 0, 0, TO_CHAR);
      act("&+LYou have created $p!", FALSE, ch, new_obj, 0, TO_CHAR);
      act("&+L$p &+Lmakes a roaring sound!&n", FALSE, 0, furnace, 0, TO_ROOM);
      //notch_skill(ch, SKILL_SMELT, 50);
    }
}

bool MobAlchemistGetPotions(P_char ch, int type, int number)
{
  P_obj    bottle;
  int      i = 0;

  while (i++ < number)
  {
    bottle = read_object(potion_data[type].vnum, VIRTUAL);
    bottle->value[0] = MIN(50, GET_LEVEL(ch));
    obj_to_char(bottle, ch);
  }
  CharWait(ch, PULSE_VIOLENCE);

  return TRUE;
}

int spl2potion(int spl)
{
  int      i;

  for (i = 0; potion_data[i].spell_type; i++)
    if(potion_data[i].spell_type == spl)
      return i;

  return 0;
}

int count_potions(P_char ch)
{
  P_obj    t_obj;
  int      level;
  int      potions = 0;

  level = GET_LEVEL(ch);
  for (t_obj = ch->carrying; t_obj; t_obj = t_obj->next_content)
  {
    if(obj_index[t_obj->R_num].virtual_number >= FIRST_POTION_VIRTUAL &&
        obj_index[t_obj->R_num].virtual_number <= FIRST_POTION_VIRTUAL + 21 &&
        t_obj->value[0] <= level)
      potions++;
  }

  return potions;
}

P_obj get_potion(P_char ch)
{
  P_obj    t_obj, next_obj;
  int count = count_potions(ch);
  int pick = number(0, count - 1);

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    if(obj_index[t_obj->R_num].virtual_number >= FIRST_POTION_VIRTUAL &&
        obj_index[t_obj->R_num].virtual_number <= FIRST_POTION_VIRTUAL + 21 &&
        t_obj->value[0] <= GET_LEVEL(ch) && pick-- == 0)
    {
      return t_obj;
    }
  }

  return NULL;
}


#define LAST_HARMFUL_SPELL_TO_ADD 11

bool randomize_potion_non_damage(P_obj potion, int slot)
{

  int      random_spell_for_potion[LAST_HARMFUL_SPELL_TO_ADD + 1] = {
    SPELL_CURSE,
    SPELL_BLINDNESS,
    SPELL_WITHER,
    SPELL_STORNOGS_LOWERED_RES,
    SPELL_RAY_OF_ENFEEBLEMENT,
    SPELL_FEEBLEMIND,
    SPELL_DISPEL_LIFEFORCE,
    SPELL_DISEASE,
    SPELL_POISON,
    SPELL_MALISON,
    SPELL_MOUSESTRENGTH,
    SPELL_DISPEL_MAGIC
  };

  int      i;

  if(potion && (slot > 0) && (slot < 4))
  {
    i = number(0, LAST_HARMFUL_SPELL_TO_ADD);
    potion->value[slot] = random_spell_for_potion[i];
    return TRUE;
  }

  return FALSE;

}

// Random eq enchant proc (cure crit).
int thrusted_eq_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      rand;
  int      curr_time;
  P_char   kala;
  P_obj    ingred;
  P_char   i;
  int      j = 0;
  bool     found;

  if(cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  if( OBJ_ROOM(obj) )
  {
    found = FALSE;
    for (i = world[obj->loc.room].people; i; i = i->next_in_room)
    {
      if( IS_PC(i) && GET_PID(i) == obj->value[5] )
      {
        found = TRUE;
        break;
      }
    }

    if( !found )
    {
      act("$p's &+Gmagical &+Yaura&+G slowly dissipates away", FALSE, 0, obj, 0, TO_ROOM);
      extract_obj(obj, TRUE); // A random eq arti?
      return FALSE;
    }
    act("$p &+Wwhispers softly...", FALSE, i, obj, 0, TO_CHAR);

    for( i = world[obj->loc.room].people; i; i = i->next_in_room )
    {
      switch( obj->value[6] )
      {
      case 0:
        act("$p's &+Wsoft &+Gmagical &+Yaura&+G surrounds you!", FALSE, i, obj, 0, TO_CHAR);
        GET_HIT(i) = BOUNDED(0, (GET_HIT(i) + number(20, 35)), GET_MAX_HIT(i));
        break;
      case 1:
        act("$p's &+Wsoft &+Gmagical &+Yaura&+G surrounds you!", FALSE, i, obj, 0, TO_CHAR);
        GET_HIT(i) = BOUNDED(0, (GET_HIT(i) + number(20, 35)), GET_MAX_HIT(i));
        break;
      case 2:
        act("$p's &+Wsoft &+Gmagical &+Yaura&+G surrounds you!", FALSE, i, obj, 0, TO_CHAR);
        GET_HIT(i) = BOUNDED(0, (GET_HIT(i) + number(20, 35)), GET_MAX_HIT(i));
        break;
      case 3:
        act("$p's &+Wsoft &+Gmagical &+Yaura&+G surrounds you!", FALSE, i, obj, 0, TO_CHAR);
        GET_HIT(i) = BOUNDED(0, (GET_HIT(i) + number(20, 35)), GET_MAX_HIT(i));
        break;
      case 4:
        act("$p's &+Wsoft &+Gmagical &+Yaura&+G surrounds you!", FALSE, i, obj, 0, TO_CHAR);
        GET_HIT(i) = BOUNDED(0, (GET_HIT(i) + number(20, 35)), GET_MAX_HIT(i));
        break;
      case 5:
        act("$p's &+Wsoft &+Gmagical &+Yaura&+G surrounds you!", FALSE, i, obj, 0, TO_CHAR);
        GET_HIT(i) = BOUNDED(0, (GET_HIT(i) + number(20, 35)), GET_MAX_HIT(i));
        break;
      case 6:
        act("$p's &+Wsoft &+Gmagical &+Yaura&+G surrounds you!", FALSE, i, obj, 0, TO_CHAR);
        GET_HIT(i) = BOUNDED(0, (GET_HIT(i) + number(20, 35)), GET_MAX_HIT(i));
        break;
      case 7:
        act("$p's &+Wsoft &+Gmagical &+Yaura&+G surrounds you!", FALSE, i, obj, 0, TO_CHAR);
        GET_HIT(i) = BOUNDED(0, (GET_HIT(i) + number(20, 35)), GET_MAX_HIT(i));
        break;
      case 8:
        act("$p's &+Wsoft &+Gmagical &+Yaura&+G surrounds you!", FALSE, i, obj, 0, TO_CHAR);
        GET_HIT(i) = BOUNDED(0, (GET_HIT(i) + number(20, 35)), GET_MAX_HIT(i));
        break;
      default:
        break;
      }
    }
  }
  return FALSE;
}

struct spell_target_data common_target_data;

#define spl common_target_data.ttype
#define tar_obj common_target_data.t_obj
#define tar_char common_target_data.t_char
#define tar_arg common_target_data.arg
extern const struct class_names class_names_table[];
extern const char *class_names[];

void do_enchant(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_STRING_LENGTH];
  char     arg2[MAX_STRING_LENGTH];
  char     buf1[MAX_STRING_LENGTH];
  int      skill = 0, qend, circle = 0, t_circle = 0, i = 0;
  P_obj    item;
  P_obj    jewel;

  P_obj    temp_obj;

  common_target_data.ttype = 0;
  common_target_data.t_obj = 0;
  common_target_data.t_char = 0;

  if( !IS_ALIVE(ch) )
  {
    if( ch )
      act("The dead do not enchant things!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if(!GET_CHAR_SKILL(ch, SKILL_ENCHANT))
  {
    act("Leave this to a real artisan.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  skill = GET_CHAR_SKILL(ch, SKILL_ENCHANT);
  argument = one_argument(argument, arg);

  if(!*arg)
  {
    act("Which item do you want to enchant?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  item = get_obj_in_list_vis(ch, arg, ch->carrying);
  //Add a check if this is a encrustable item...
  if(!item)
  {
    act("Which item do you want to enchant?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(IS_ARTIFACT(item))
  {
    act("You are unable to enchant an artifact.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(!*argument)
  {
    act("Syntax is 'enchant sword 'bless'", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  argument = skip_spaces(argument);

  if(*argument != '\'')
  {
    send_to_char("Don't forget 'apostrophes'.\r\n", ch);
    return;
  }

  // Locate the last quote && lowercase the magic words (if any)
  for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
    *(argument + qend) = LOWER(*(argument + qend));

  if(*(argument + qend) != '\'')
  {
    send_to_char("Don't forget 'apostrophes'.\r\n", ch);
    return;
  }
  spl = old_search_block(argument, 1, (uint) (MAX(0, (qend - 1))), (const char **) spells, 0);

  if(spl != -1)
  {
    spl--;
  }
  if( (IS_AGG_SPELL(spl) != -1) && (skills[spl].spell_pointer == 0) )
  {
    send_to_char("Sorry, this magic has not yet been implemented.\r\n", ch);
    return;
  }
  if(IS_AGG_SPELL(spl))
  {
    send_to_char("A harmful spell is not a good idea.\r\n", ch);
    return;
  }
  if(!affected_by_spell(ch, spl))
  {
    send_to_char("You are not affected by that spell are you? \r\n", ch);
    return;
  }

  if( !IS_SET(skills[spl].targets, TAR_CHAR_ROOM) || IS_SET(skills[spl].targets, TAR_SELF_ONLY) )
  {
    send_to_char("Sorry, this item cannot be enchanted with a spell such as this. \r\n", ch);
    return;
  }

  while (i < CLASS_COUNT)
  {
    t_circle = skills[(spl)].m_class[i].rlevel[0];
    if(t_circle > circle)
    {
      circle = t_circle;
    }
    i++;
  }
  if(circle > (skill / 10) || circle == 0)
  {
    send_to_char("That spell is too complicated for you! \r\n", ch);
    return;
  }
  if(GET_PLATINUM(ch) < (circle * 10))
  {
    send_to_char("You do not have enough platinum on you.! \r\n", ch);
    return;
  }
  if(item->condition < get_property("skill.enchant.minItemCondition", 30))
  {
    send_to_char("No true alchemist would do something with a weapon in that condition. Repair it!! \r\n", ch);
    return;
  }

  GET_PLATINUM(ch) = GET_PLATINUM(ch) - (circle * 10);
  //notch_skill(ch, SKILL_ENCHANT, 7.7);

  act("&+L$n melts some &+Wplatinum &+Lcoins in a vial of &+gacid &+Land then&n &L&+Lproceeds to carefully pour it over $s $q.&n",
    TRUE, ch, item, 0, TO_ROOM);
  act("&+LYou melt some &+Wplatinum &+Lcoins in a vial of &+gacid &+Land then&n&L&+Lproceed to carefully pour it over your $q.&n",
    TRUE, ch, item, 0, TO_CHAR);

  // cant add_event with both ch and item so item/spell has to be passed in data
  // add_event(event_enchant, 1 * PULSE_VIOLENCE, ch, 0, item, 0, &spl, sizeof(spl));

  common_target_data.ttype = spl;
  common_target_data.t_obj = item;
  add_event(event_enchant, 1 * PULSE_VIOLENCE, ch, 0, 0, 0, &common_target_data, sizeof(common_target_data));
  CharWait(ch, PULSE_VIOLENCE * 1);
}

void event_enchant(P_char ch, P_char victim, P_obj item, void *data)
{
  int      spll = 0;;
  int      skill = 0;
  struct spell_target_data tmp;

  tmp = *((spell_target_data *) data);
  spll = tmp.ttype;
  item = tmp.t_obj;

  if(!IS_ALIVE(ch))
  {
    if( ch )
      act("The dead do not enchant things!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if(!item || ch != item->loc.carrying)
  {
    act("You lost the item! Where is it?", FALSE, ch, 0, 0, TO_CHAR);
    act("$n suddenly stops abruptly, looking around in anger.", TRUE, ch, 0, 0, TO_ROOM);
    CharWait(ch, PULSE_VIOLENCE * 1);
    return;
  }
  if(IS_ARTIFACT(item))
  {
    act("Your attempt to enchant an artifact fails!!!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sighs and mutters about something under $s breathe.", TRUE, ch, 0, 0, TO_ROOM);
    CharWait(ch, PULSE_VIOLENCE * 1);
    return;
  }
  skill = GET_CHAR_SKILL(ch, SKILL_ENCHANT);
  if(!number(0, skill - 10))
  {
    act("$n utters a foul curse as $e pours too much acid on $q.&L$n's $q was damaged as the acid eats into it!",
      FALSE, ch, item, 0, TO_ROOM);
    act("You utter a foul curse as you pour too much acid on the $q.&LYour $q was damaged as the acid eats into it!",
      FALSE, ch, item, 0, TO_CHAR);
    item->condition = item->condition - number(5, 10);

    if(item->condition < 1)
    {
      act("$n has destroyed $p!", TRUE, ch, item, 0, TO_ROOM);
      act("You have destroyed $p!", TRUE, ch, item, 0, TO_CHAR);
      extract_obj(item, TRUE); // Not an arti, but ok.
      item = NULL;
      return;
    }

    if(!number(0, 5))
    {
      act("$q &+yglows bright and eerie!&n", FALSE, ch, item, 0, TO_ROOM);
      act("$q &+yglows bright and eerie!&n", FALSE, ch, item, 0, TO_CHAR);
      SET_BIT(item->extra_flags, ITEM_NOREPAIR);
    }
    return;
  }
  act("$n's $q starts to heat up and then turns &+Wwhite hot before&n&Lslowly &+bcooling down&n and turning back to normal.&n",
    FALSE, ch, item, 0, TO_ROOM);
  act("Your $q starts to heat up and then turns &+Wwhite hot&n before&n&Lslowly &+bcooling down&n and turning back to normal.",
    TRUE, ch, item, 0, TO_CHAR);

  logit(LOG_DEBUG, "%s enchanted %s with %s.", GET_NAME(ch), item->short_description, skills[spll].name);

  if(get_obj_affect(item, SKILL_ENCHANT))
  {
    affect_from_obj(item, SKILL_ENCHANT);
  }

  if(CAN_WEAR(item, ITEM_ATTACH_BELT))
  {
    REMOVE_BIT(item->wear_flags, ITEM_ATTACH_BELT);
  }

  if(CAN_WEAR(item, ITEM_WEAR_BACK))
  {
    REMOVE_BIT(item->wear_flags, ITEM_WEAR_BACK);
  }

  if(ch && item)
  {
    set_obj_affected(item, (skill / 2) * WAIT_MIN, SKILL_ENCHANT, spll);
  }
}

void spell_napalm(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "&+RYou smirk as the sticky substance coating $N &+Rerupts in flames.&n",
    "&+WPain is all you know as the world around you turns into an inferno.&n",
    "&+RThe sticky substance coating $N &+Rerupts in flames as it comes into contact with air.&n",
    "&+L$N &+Lthrashes wildly as $E erupts in &+rflames &+Land soon all that remains is a pile of ash.&n",
    "&+LThe world becomes &+rfire &+Land &+ragony&+L, both following you into oblivion.&n",
    "&+L$N &+Lthrashes wildly as $E erupts in &+rflames &+Land soon all that remains is a pile of ash.&n", 0};

  dam = MIN(40, level) * 4 + number(0,level);

  spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_NODEFLECT, &messages);
}

void spell_strong_acid(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "$N screams in pain as the &+Gacid&n trickles down $S body.",
    "You scream in agony as the &+Gacid&n eats into your flesh.",
    "$N screams in pain as the &+Gacid&n trickles down $S body.",
    "You watch with disgust as $N is dissolved into a pile of smoking flesh.",
    "The sight of &+Gmelting flesh&n is the last you will ever see...",
    "The &+Gacid&n quickly dissolves $N into a pool of smoking flesh.", 0};

  dam = (int)(10.5 * MIN(level, 50)) + number(1, 25);

  spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_NODEFLECT, &messages);
}

void spell_glass_bomb(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "The flask &+Yexplodes&n showering $N with sharp pieces of glass.",
    "The flask &+Yexplodes&n showering you with sharp pieces of glass.",
    "The flask &+Yexplodes&n showering $N with sharp pieces of glass.",
    "The fury of the exploding glass reduces $N to a pile of &+rbleeding flesh&n.",
    "The fury of the exploding glass reduces you to a pile of &+rbleeding flesh&n.",
    "The fury of the exploding glass reduces $N to a pile of &+rbleeding flesh&n.", 0};

  dam = 6 * MIN(level, 45) + number(1, 25);

  spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages);
}

void spell_nitrogen(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  int dam;
  struct damage_messages messages = {
    "The &+Cnitrogen&n splashes $N freezing everything it touches.",
    "The &+Cintense cold&n of the liquid burns your body.",
    "The &+Cnitrogen&n splashes $N freezing everything it touches.",
    "$N &+Cis turned into a &+Wfrost-rimmed statue &+Cas $E is frozen solid by the liquid nitrogen.&n",
    "&+CFrost instantly covers you as the searing cold turns you into a &+Wfrozen statue.&n",
    "$N &+Cis turned into a &+Wfrost-rimmed statue &+Cas $E is frozen solid by the liquid nitrogen.&n", 0};

  dam = 4 * MIN(26, level) + number(0, 9);

  spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, &messages);
}

bool grease_check(P_char ch) {
  struct affected_type *af;

  if((af = get_spell_from_char(ch, SPELL_GREASE)) && !number(0,2) &&
      GET_C_AGI(ch) < number(0, 115)) {
    act("$n tries to run but slips on a greasy substance dripping off $s cloths!", FALSE,
        ch, 0, 0, TO_ROOM);
    send_to_char("You try to run but alas! You slip on the damned grease covering your feet!\n", ch);
    if(af->modifier-- == 0) {
      send_to_char("Fortunately you got rid of this thing this time!\n", ch);
      affect_from_char(ch, SPELL_GREASE);
    }
    SET_POS(ch, POS_SITTING + GET_STAT(ch));
    return TRUE;
  }

  return FALSE;
}

void spell_grease(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
{
  struct affected_type af, *afp;

  act("&+LA black oily substance&n spreads all over $N's clothes, yuck!", FALSE, victim, 0, ch, TO_ROOM);
  act("&+LA black oily substance&n spreads all over your clothes, yuck!", FALSE, victim, 0, 0, TO_CHAR);

  if(afp = get_spell_from_char(victim, SPELL_GREASE)) {
    afp->duration = 1;
    afp->modifier = number(1,4);
  } else {
    memset(&af, 0, sizeof(af));
    af.type = SPELL_GREASE;
    af.duration = 1;
    af.modifier = number(1,4);

    affect_to_char(victim, &af);
  }
}
