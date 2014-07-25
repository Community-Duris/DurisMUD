/*
 * ***************************************************************************
 * *  File: item_enchant.c                                   Part of Duris*
 * *  Usage: procedures to create spell affects on items
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * * Written by: Granor
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "graph.h"
#include "interp.h"
#include "new_combat_def.h"
#include "structs.h"
#include "prototypes.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "utils.h"
#include "weather.h"
#include "sound.h"
#include "kingdom.h"
#include "assocs.h"
#include "justice.h"

/*
 * external variables
 */

extern P_index obj_index;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_char combat_list;
extern P_event current_event;
extern P_event event_type_list[];
extern P_obj object_list;
extern P_room world;
extern P_index mob_index;
extern const char *affected_bits[];
extern const char *apply_types[];
extern const char *extra_bits[];
extern const char *anti_bits[];
extern const char *item_types[];
extern const struct stat_data stat_factor[];
extern const int exp_table[];
extern int rev_dir[];
extern int avail_hometowns[][TOTALRACE];
extern int current_spell_being_cast;
extern int guild_locations[][TOTALCLASS];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int hometown[];
extern int top_of_world;
extern struct str_app_type str_app[];
extern struct con_app_type con_app[];
extern struct time_info_data time_info;
extern struct weather_data weather_info;
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;


/* This spell enchants the item. Once this is done, it must be
permanenced. Until then, the item cannot be rented, and is transient.
*/

void spell_enchant(int level, P_char ch, P_char victim, P_obj obj)
{
  int      i, j;

  if (!((MAX_OBJ_AFFECT >= 2) && obj && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }

  if (((GET_ITEM_TYPE(obj) == ITEM_WEAPON) ||
       (GET_ITEM_TYPE(obj) == ITEM_DRINKCON) ||
       (GET_ITEM_TYPE(obj) == ITEM_NOTE)) &&
      !IS_SET(obj->extra_flags, ITEM_MAGIC) && !(obj->enchant.prepared))
  {

    if ( !(j = (GET_ITEM_TYPE(obj) == ITEM_WEAPON
      && (GET_CLASS(ch) == CLASS_CONJURER || GET_CLASS(ch) == CLASS_SUMMONER))
      || ( GET_ITEM_TYPE(obj) == ITEM_DRINKCON && GET_CLASS(ch) == CLASS_CLERIC)
      || ( GET_ITEM_TYPE(obj) == ITEM_NOTE
      && (GET_CLASS(ch) == CLASS_CONJURER || GET_CLASS(ch) == CLASS_SUMMONER
      || GET_CLASS(ch) == CLASS_SORCERER ||GET_CLASS(ch) == CLASS_NECROMANCER) )) && !IS_TRUSTED(ch))
    {
      send_to_char("You cannot prepare this kind of object.\r\n", ch);
      return;
    }

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
        return;

    obj->enchant.prepared = 1;

    SET_BIT(obj->extra_flags, ITEM_MAGIC);
    SET_BIT(obj->extra_flags, ITEM_NORENT);
    SET_BIT(obj->extra_flags, ITEM_NOSELL);
    SET_BIT(obj->extra_flags, ITEM_TRANSIENT);

    if (IS_GOOD(ch))
    {
//      SET_BIT(obj->extra_flags, ITEM_ANTI_EVIL);
      act("&+W$p glows white.", FALSE, ch, obj, 0, TO_CHAR);
    }
    else if (IS_EVIL(ch))
    {
//      SET_BIT(obj->extra_flags, ITEM_ANTI_GOOD);
      act("&+r$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
    }
    else
    {
      act("&+g$p glows green.", FALSE, ch, obj, 0, TO_CHAR);
      if (number(0, 9) < 3)
      {
        act("A flash of &+Wlight&N and &+G$p glows green&N.", FALSE, ch, obj,
            0, TO_CHAR);
//      SET_BIT(obj->extra_flags, ITEM_ANTI_GOOD);
//      SET_BIT(obj->extra_flags, ITEM_ANTI_EVIL);
      }
    }
  }
}

/*Add upto 5 plusses to the weapon (6 if its day and month of BL)*/
void spell_enchant_weapon(int level, P_char ch, P_char victim, P_obj obj)
{
  int      weekday, plus_six, six_possible;

  if (!((MAX_OBJ_AFFECT >= 2) && obj && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  if ((GET_ITEM_TYPE(obj) == ITEM_WEAPON) && (obj->enchant.prepared) &&
      IS_SET(obj->extra_flags, ITEM_MAGIC))
  {

    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier += 1;

    obj->affected[1].location = APPLY_DAMROLL;
    plus_six = ((obj->affected[1].modifier += 1) > 5);

    obj->enchant.magic += MAX(0, 10 * obj->affected[1].modifier
                              - GET_LEVEL(ch) / 5);

    weekday = ((35 * time_info.month) + time_info.day + 1) % 7;
    six_possible = ((weekday == 4) && (time_info.month == 34));

    if (IS_TRUSTED(ch))
      return;

    if ((number(1, 200) < obj->enchant.magic)
        || (plus_six && !(six_possible))
        || (max_magic(obj) < obj->enchant.magic))
    {
      act("&+R$p cannot hold this much magic, and explodes.", FALSE, ch,
          obj, 0, TO_CHAR);
      act("&+R$p, held by $n, &+Rcannot hold this much magic, and \
	explodes.", FALSE, ch, obj, 0, TO_ROOM);
      obj_from_char(obj);
      extract_obj(obj);
      die(ch, ch, 0);
    }
  }
}

/* to have spell effects on the object 
 weapons: procs
 drincon: potions effect
 note: like scroll
*/
void cast_spell_to_object(int level, P_char ch, P_obj obj, int spell)
{
  int      weekday, month, right_time;

  if (!((MAX_OBJ_AFFECT >= 2) && obj && ch))
  {
    logit(LOG_EXIT, "assert: bogus parms");
    raise(SIGSEGV);
  }
  if (((GET_ITEM_TYPE(obj) == ITEM_WEAPON) ||
       (GET_ITEM_TYPE(obj) == ITEM_NOTE) ||
       (GET_ITEM_TYPE(obj) == ITEM_DRINKCON))
      && (obj->enchant.prepared) &&
      IS_SET(obj->extra_flags, ITEM_MAGIC) && !obj->value[6])
  {

    obj->enchant.magic += 10 * GetCircle(spell) - GET_LEVEL(ch) / 5;

    if (spell == SPELL_DISPEL_MAGIC)
      obj->enchant.magic += 60;
    weekday = ((35 * time_info.month) + time_info.day + 1) % 7;
    month = time_info.month;

    right_time = cast_day(ch, spell, weekday, month);

    if (!right_time && (GET_ITEM_TYPE(obj) == ITEM_WEAPON))
    {
      send_to_char("The forces required to cast this spell have \
not reached sufficient levels. \r\n", ch);
      return;
    }

    if (can_cast_on_object(obj, spell))
    {
      if (GET_ITEM_TYPE(obj) == ITEM_WEAPON)
        obj_index[obj->R_num].func.obj = player_enchant_proc;
      if (GET_ITEM_TYPE(obj) == ITEM_DRINKCON)
      {
        if (obj->value[1])
        {
          send_to_char("Empty it first.\r\n", ch);
          return;
        }
        obj->value[1] = obj->value[0];
      }
      obj->value[6] = spell;
      obj->value[7] = level;
    }
    else
    {
      send_to_char("You cannot cast that spell on this item.\r\n", ch);
      return;
    }

    if (IS_TRUSTED(ch))
      return;

    if ((number(1, 200) < obj->enchant.magic)
        || (max_magic(obj) < obj->enchant.magic))
    {
      act("&+R$p cannot hold this much magic, and explodes.", FALSE, ch,
          obj, 0, TO_CHAR);
      act("&+R$p, held by $n, &+Rcannot hold this much magic, and \
	explodes.", FALSE, ch, obj, 0, TO_ROOM);
      obj_from_char(obj);
      extract_obj(obj);
      die(ch, ch, 0);
    }
  }
}

/* allow the weapon to be rented and no longer transient. String
it if its a weapon
*/
void spell_permanence(int level, P_char ch, P_char victim, P_obj obj)
{
  int      i = 0, j = 0;
  char     buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  if (!obj->enchant.prepared && !IS_SET(obj->extra_flags, ITEM_MAGIC))
  {
    send_to_char("Why bother permanencing this?\r\n", ch);
    return;
  }

  act("$p glows &+Bblue&N as all the magic becomes bonded with it.",
      FALSE, ch, obj, 0, TO_CHAR);
  act("$p , held by $n, glows &+Bblue&N as all the magic becomes bonded \
with it.", FALSE, ch, obj, 0, TO_ROOM);

  REMOVE_BIT(obj->extra_flags, ITEM_NORENT);
  REMOVE_BIT(obj->extra_flags, ITEM_TRANSIENT);
  obj->enchant.prepared = 0;

  buf[0] = '\0';

  if (GET_ITEM_TYPE(obj) == ITEM_WEAPON)
  {
    if ((obj->value[1] * obj->value[2]) <
        ((IS_SET(obj->extra_flags, ITEM_TWOHANDS)) ? 28 : 14))
    {
      (IS_SET(obj->extra_flags, ITEM_TWOHANDS)) ?
        (obj->value[1] = 4) : (obj->value[1] = 2);
      obj->value[2] = 7;
    }
    strcpy(buf2, obj->short_description);
    buf2[strlen(buf2) - 1] = '\0';
    buf2[strlen(buf2) - 1] = '\0';
    obj->affected[0].location = APPLY_HITROLL;
    switch (obj->affected[0].modifier)
    {
    case 0:
      break;
    case 1:
      strcat(buf, "A fine ");
      SET_BIT(obj->extra2_flags, ITEM2_PLUSONE);
      break;
    case 2:
      strcat(buf, "A powerful ");
      SET_BIT(obj->extra2_flags, ITEM2_PLUSTWO);
      break;
    case 3:
      strcat(buf, "A mighty ");
      SET_BIT(obj->extra2_flags, ITEM2_PLUSTHREE);
      break;
    case 4:
      strcat(buf, "An unearthly ");
      SET_BIT(obj->extra2_flags, ITEM2_PLUSFOUR);
      break;
    case 5:
      strcat(buf, "An ultimate ");
      SET_BIT(obj->extra2_flags, ITEM2_PLUSFIVE);
      break;
    default:
      strcat(buf, "A godly ");
      SET_BIT(obj->extra2_flags, ITEM2_PLUSFIVE);
      break;
    }

    strcat(buf, buf2);

    if (obj->value[6])
    {
      strcat(buf, " of ");
      switch (obj->value[6])
      {
      case SPELL_MAGIC_MISSILE:
        strcat(buf, "missiles");
        break;
      case SPELL_DISPEL_MAGIC:
        strcat(buf, "dispelling");
        break;
      case SPELL_LIGHTNING_BOLT:
        strcat(buf, "storms");
        break;
      case SPELL_FIREBALL:
        strcat(buf, "fire");
        break;
      case SPELL_EARTHEN_RAIN:
        strcat(buf, "earth");
        break;
      case SPELL_SOUL_DISTURBANCE:
        strcat(buf, "spirits");
        break;
      case SPELL_MOLEVISION:
        strcat(buf, "dimness");
        break;
      case SPELL_DARKNESS:
        strcat(buf, "darkness");
        break;
      case SPELL_CONTINUAL_LIGHT:
        strcat(buf, "light");
        break;
      case SPELL_CURE_LIGHT:
      case SPELL_CURE_SERIOUS:
      case SPELL_CURE_CRITIC:
        strcat(buf, "healing");
        break;
      case SPELL_ENERGY_DRAIN:
        strcat(buf, "draining");
        break;
      default:
        break;
      }
    }
    SET_BIT(obj->str_mask, STRUNG_DESC2);
    obj->short_description = str_dup(buf);
    SET_BIT(obj->str_mask, STRUNG_DESC3);
    obj->action_description = str_dup(buf);
  }
  if (GET_ITEM_TYPE(obj) == ITEM_NOTE)
  {
    strcpy(buf2, "A scroll of ");
    switch (obj->value[6])
    {
    case SPELL_MAGIC_MISSILE:
      strcat(buf, "magic missile");
      break;
    case SPELL_DISPEL_MAGIC:
      strcat(buf, "dispel magic");
      break;
    case SPELL_LIGHTNING_BOLT:
      strcat(buf, "lightning bolt");
      break;
    case SPELL_FIREBALL:
      strcat(buf, "fireball");
      break;
    case SPELL_EARTHEN_RAIN:
      strcat(buf, "earthen rain");
      break;
    case SPELL_SOUL_DISTURBANCE:
      strcat(buf, "soul disturbance");
      break;
    case SPELL_MOLEVISION:
      strcat(buf, "molevision");
      break;
    case SPELL_DARKNESS:
      strcat(buf, "darkness");
      break;
    case SPELL_CONTINUAL_LIGHT:
      strcat(buf, "continual light");
      break;
    case SPELL_CURE_LIGHT:
    case SPELL_CURE_SERIOUS:
    case SPELL_CURE_CRITIC:
      strcat(buf, "healing");
      break;
    case SPELL_ENERGY_DRAIN:
      strcat(buf, "energy drain");
      break;
    default:
      break;
    }
    strcat(buf2, buf);
    SET_BIT(obj->str_mask, STRUNG_KEYS);
    obj->name = str_dup(buf);
    SET_BIT(obj->str_mask, STRUNG_DESC2);
    obj->short_description = str_dup(buf2);
  }
}

/*determine proc by type of spell on weapon*/
int player_enchant_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  int      level;
  P_char   vict;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!dam)                     /*
                                   if dam is not 0, we have been called when
                                   weapon hits someone
                                 */
    return (FALSE);

  if (!ch)
    return (FALSE);

  if (!ch)
    return (FALSE);

  if (!OBJ_WORN_POS(obj, WIELD))
    return (FALSE);

  vict = (P_char) arg;

  if (!vict)
    return (FALSE);

  if (obj->loc.wearing == ch)
  {
    if (!number(0, 20))
    {
      level = obj->value[7];
      switch (obj->value[6])
      {
      case SPELL_MAGIC_MISSILE:
        act("Your $q glows brightly as magical arrows streak out of it.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as magical arrows streak out of it!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_magic_missile(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case SPELL_DISPEL_MAGIC:
        act("Your $q glows brightly as a beam of energy streaks out of it.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as a beam of energy streaks out of it!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_dispel_magic(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case SPELL_LIGHTNING_BOLT:
        act
          ("Your $q glows brightly as a bolt of lightning streaks out of it.",
           FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act
          ("$n's $q glows brightly as a bolt of lightning streaks out of it!",
           FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_lightning_bolt(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case SPELL_FIREBALL:
        act("Your $q glows brightly as a ball of fire streaks out of it.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as a ball of fire streaks out of it!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_fireball(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case SPELL_CURE_LIGHT:
        act("Your $q glows brightly as healing energy envelops you.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as healing energy envelops them!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_cure_light(level, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      case SPELL_CURE_SERIOUS:
        act("Your $q glows brightly as healing energy envelops you.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as healing energy envelops them!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_cure_serious(level, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      case SPELL_CURE_CRITIC:
        act("Your $q glows brightly as healing energy envelops you.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as healing energy envelops them!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_cure_critic(level, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        break;
      case SPELL_CONTINUAL_LIGHT:
        act("Your $q glows brightly and lights the room.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly and lights the room!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_continual_light(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case SPELL_DARKNESS:
        act("Your $q glows brightly as darkness cloaks the room.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as darkness cloaks the room!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_darkness(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case SPELL_MOLEVISION:
        act("Your $q glows brightly as a hazy outline seeps out of it.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as a hazy outline seeps out of it!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_molevision(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case SPELL_EARTHEN_RAIN:
        if (number(0, 1))
        {
          act("Your $q glows brightly as a beam of energy shoots out of it.",
              FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
          act("$n's $q glows brightly as a beam of energy shoots out of it.",
              FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
          cast_earthen_rain(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        }
        break;
      case SPELL_SOUL_DISTURBANCE:
        act("Your $q glows brightly as a beam of energy shoots out of it.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as a beam of energy shoots out of it!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_soul_disturbance(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      case SPELL_ENERGY_DRAIN:
        act("Your $q glows brightly as a dark beam shoots out of it.",
            FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q glows brightly as a dark beam shoots out of it!",
            FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
        cast_energy_drain(level, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        break;
      default:
        send_to_char("Somehow you have a proc that shouldnt be on the \
weapon, on the weapon. Contact a god immediately.\r\n", ch);
      }
    }
    else
    {
      if (!ch->specials.fighting)
        set_fighting(ch, vict);
    }
  }
  if (ch->specials.fighting)
    return (FALSE);             /*
                                   do the normal hit damage as well
                                 */
  else
    return (TRUE);
}

/*items have different amounts of magic they can have on them*/
int max_magic(P_obj obj)
{
  switch (GET_ITEM_TYPE(obj))
  {
  case ITEM_WEAPON:
    return ((IS_SET(obj->extra_flags, ITEM_TWOHANDS)) ? 200 : 100);
  case ITEM_NOTE:
    return 100;
  case ITEM_DRINKCON:
    return 100;
  default:
    return 0;
  }
}

/* which spells can be cast on which items */
int can_cast_on_object(P_obj obj, int spell)
{
  switch (GET_ITEM_TYPE(obj))
  {
  case ITEM_WEAPON:
    switch (spell)
    {
    case SPELL_MAGIC_MISSILE:
    case SPELL_DISPEL_MAGIC:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_FIREBALL:
    case SPELL_CURE_LIGHT:
    case SPELL_CURE_SERIOUS:
    case SPELL_CURE_CRITIC:
    case SPELL_CONTINUAL_LIGHT:
    case SPELL_DARKNESS:
    case SPELL_MOLEVISION:
    case SPELL_EARTHEN_RAIN:
    case SPELL_SOUL_DISTURBANCE:
    case SPELL_ENERGY_DRAIN:
      return 1;
    default:
      return 0;
    }
  case ITEM_NOTE:
    switch (spell)
    {
    case SPELL_MAGIC_MISSILE:
    case SPELL_DISPEL_MAGIC:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_FIREBALL:
    case SPELL_CURE_LIGHT:
    case SPELL_CURE_SERIOUS:
    case SPELL_CURE_CRITIC:
    case SPELL_CONTINUAL_LIGHT:
    case SPELL_DARKNESS:
    case SPELL_MOLEVISION:
    case SPELL_EARTHEN_RAIN:
    case SPELL_SOUL_DISTURBANCE:
    case SPELL_ENERGY_DRAIN:
      return 1;
    default:
      return 0;
    }
  case ITEM_DRINKCON:
    switch (spell)
    {
    case SPELL_CURE_LIGHT:
    case SPELL_CURE_SERIOUS:
    case SPELL_CURE_CRITIC:
    case SPELL_CURE_BLIND:
    case SPELL_REMOVE_POISON:
    case SPELL_REMOVE_CURSE:
      return 1;
    default:
      return 0;
    }
  default:
    return 0;
  }
}

/* spells must be cast on specific days*/
int cast_day(P_char ch, int spell, int day, int month)
{
  if (IS_TRUSTED(ch))
    return 1;
  switch (spell)
  {
  case SPELL_MAGIC_MISSILE:
  case SPELL_CURE_LIGHT:
    return 1;
  case SPELL_LIGHTNING_BOLT:
    return (((day == 3) || (day == 1)) && (month == 6));
  case SPELL_FIREBALL:
    return ((day == 6) && (month == 10));
  case SPELL_DISPEL_MAGIC:
    return ((day == 5) && (month == 7));
  case SPELL_REMOVE_POISON:
    return (((day == 5) || (day == 0)) && (month == 7));
  case SPELL_REMOVE_CURSE:
    return (((day == 5) || (day == 0)) && (month == 7));
  case SPELL_CURE_BLIND:
    return ((day == 5) && (month == 7));
  case SPELL_CURE_SERIOUS:
    return (((day == 5) || (day == 0)) && (month == 7));
  case SPELL_CURE_CRITIC:
    return ((day == 5) && (month == 8));
  case SPELL_MOLEVISION:
    return ((number(0, 1) == 1) && (day == 2));
  case SPELL_EARTHEN_RAIN:
    return ((number(0, 1) == 1) && (day == 3));
  case SPELL_SOUL_DISTURBANCE:
    return ((number(0, 1) == 1) && (day == 4));
  case SPELL_ENERGY_DRAIN:
    return ((day == 2) && (month == 15));
  case SPELL_CONTINUAL_LIGHT:
    return ((day == 6) && (month == 9));
  case SPELL_DARKNESS:
    return ((day == 0) && (month == 9));
  default:
    return 0;
  }
}
