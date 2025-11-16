#include "stdio.h"
#include "string.h"
#include "time.h"

#include "db.h"
#include "interp.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "events.h"
#include "objmisc.h"
#include "prototypes.h"
#include "structs.h"
#include "justice.h"
#include "damage.h"
#include "guard.h"
#include "disguise.h"
#include "mm.h"
#include "achievements.h"

/*
 * external variables
 */
extern struct shapechange_struct shapechange_name_list[];
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;

const char *disguise_list[] = {
  "halfling",
  "gnome",
  "dwarf",
  "elf",
  "half-elf",
  "human",
  "duergar",
  "drow",
  "orc",
  "goblin",
  "barbarian",
  "githyanki",
  "vampire",
  "lich",
  "rakshasa",
  "githzerai",
  "orog",
  "\n"
};

struct disguise_list_data_struct
{
  int      size;
  int      race;
  const char *name[3];
};

struct disguise_list_data_struct disguise_list_data[] = {
  {SIZE_SMALL, RACE_HALFLING,
   {"&+ca halfling traveler&N", "&+ga halfling scout&N", "&+ya halfling merchant&N"}},
  {SIZE_SMALL, RACE_GNOME,
   {"&+La gnome merchant&N", "a gnome inventor", "&+Ra gnome peasant&N"}},
  {SIZE_MEDIUM, RACE_MOUNTAIN,
   {"a dwarf miner", "&+ra dwarf guard&N", "&+Ya dwarf merchant&N"}},
  {SIZE_MEDIUM, RACE_GREY,
   {"an elf archer", "&+Gan elven merchant&N", "&+gan elven scout&N"}},
  {SIZE_MEDIUM, RACE_HALFELF,
   {"a half-elf bard", "&+Ga half-elf merchant&N", "&+La half-elf guard&N"}},
  {SIZE_MEDIUM, RACE_HUMAN,
   {"a human street sweeper", "&+Ca human merchant&N", "&+Ga human traveler&N"}},
  {SIZE_SMALL, RACE_DUERGAR,
   {"a duergar brigand", "&+ra clansdwarf&N", "&+La duergar merchant&N"}},
  {SIZE_MEDIUM, RACE_DROW,
   {"a drow bladesman", "&+ma drow guard&N", "&+La drow merchant&N"}},
  {SIZE_MEDIUM, RACE_ORC,
   {"an orc shaman", "&+man orc guard&N", "&+Lan orcish merchant&N"}},
  {SIZE_SMALL, RACE_GOBLIN,
   {"a goblin magician", "&+ya goblin merchant&N", "&+ga goblin guard&N"}},
  {SIZE_LARGE, RACE_BARBARIAN,
   {"&+ya barbarian tribesman&N", "&+ya barbarian shaman&N", "&+ga barbarian fisherman&N"}},
  {SIZE_MEDIUM, RACE_GITHYANKI,
   {"&+ga githyanki antipaladin&N", "&+ga githyanki traveler&N", "&+La githyanki psionicist&N"}},
  {SIZE_MEDIUM, RACE_PVAMPIRE,
   {"&+ra vampire antipaladin&N", "&+ra vampire necromancer&N", "&+ra vampire warlock&N"}},
  {SIZE_MEDIUM, RACE_LICH,
   {"&+La lich wizard&N", "&+La lich necromancer&N", "&+La lich warlock&N"}},
  {SIZE_MEDIUM, RACE_RAKSHASA,
   {"a rakshasa tribesman", "&+ya rakshasa warrior&N", "&+ya rakshasa guard&N"}},
  {SIZE_MEDIUM, RACE_GITHZERAI,
   {"a githzerai slave", "a githzerai warrior", "a githzerai psionicist"}},
  {SIZE_LARGE, RACE_OROG,
   {"&+Lan orog warrior&n", "&+ran orog berserker&n", "&+yan orog warpriest&n"}}
};

void do_disguise(P_char ch, char *arg, int cmd)
{
  int      skl_lvl = 0;
  int      percent = 0;
  int      i, the_size;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     name[MAX_STRING_LENGTH];
  P_char   target = NULL;
  P_obj    temp;
  bool     equipped;

  bool /*disguise_pc = FALSE, */ disguise_npc = FALSE;

  if (!ch)
    return;

/*  if (!IS_TRUSTED(ch)) {
   send_to_char("Not ready yet!\r\n", ch);
   return;
   } */

  // No disguise while fighting
  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
  {
    send_to_char("Can't do that while fighting!\r\n", ch);
    return;
  }

  // No diguising for NPC's!
  if (IS_NPC(ch))
    return;

  skl_lvl = GET_CHAR_SKILL(ch, SKILL_DISGUISE) * 2;

  // Do we have the skill?
  if (!skl_lvl)
  {
    send_to_char("You don't know how.\r\n", ch);
    return;
  }

  if (!*arg)
  {
    // Removing the disguise
    if (IS_DISGUISE(ch))
    {
      remove_disguise(ch, FALSE);
      act("$n starts removing $s disguise.", FALSE, ch, 0, ch, TO_ROOM);
      send_to_char( "You start removing your disguise.\n\r", ch );
      CharWait(ch, PULSE_VIOLENCE * 3);
    }
    // Otherwise they are clueless
    else
    {
      send_to_char("Disguise as who or what?\r\n", ch);
    }
    return;
  }

  // Disguising when already disgusied? I think not!
  if( IS_DISGUISE(ch) )
  {
    send_to_char("You need to remove your disguise (just 'disguise') before trying to disguise again!\r\n", ch);
    return;
  }

  one_argument(arg, name);

  // Search the disguise_list to see if we're looking for an npc disguise
  if( (i = search_block(name, disguise_list, 0)) >= 0 )
  {
    disguise_npc = TRUE;
  }
  else
  {
    target = (struct char_data *) mm_get(dead_mob_pool);
    ensure_pconly_pool();
    target->only.pc = (struct pc_only_data *) mm_get(dead_pconly_pool);

    // Bad / no pfile. Note: target will never be NULL because of above.
    if( (restoreCharOnly(target, skip_spaces(arg)) < 0) )
    {
      free_char(target);
      target = NULL;
    }

    // Character dosn't exist
    if( !target )
    {
      send_to_char("Disguise as who?\r\n", ch);
      return;
    }

    // Disguise as himself?
    if( target == ch )
    {
      send_to_char("It is too hard to disguise into that loser!\r\n", ch);
      free_char(target);
      return;
    }

    // No disguising as immortals
    if( !IS_TRUSTED(ch) && GET_LEVEL(target) > 56 )
    {
      send_to_char("You really don't want to disguise yourself as a God!\r\n", ch);
      free_char(target);
      return;
    }

    // Dissalow imms disguising as higher levle imms
    if (IS_TRUSTED(ch) )
    {
      if( GET_LEVEL(ch) <= GET_LEVEL(target) )
      {
        send_to_char("No you don't!\r\n", ch);
        free_char(target);
        return;
      }
    }

    // If it's a mortal disguising
    if( !IS_TRUSTED(ch) )
    {
      if( GET_RACE(target) == RACE_ILLITHID || GET_RACE(target) == RACE_CENTAUR || GET_RACE(target) == RACE_THRIKREEN
        || GET_RACE(target) == RACE_MINOTAUR || GET_RACE(target) == RACE_TROLL )
      {
        send_to_char("Disguising into that is just short of impossible.\r\n", ch);
        free_char(target);
        return;
      }
    }
  }

  equipped = FALSE;

  // Check if we have a disguise kit
  if( !IS_TRUSTED(ch) || !affected_by_spell(ch, ACH_DECEPTICON) )
  {
    if( !(temp = get_obj_in_list_vis(ch, "_disguise_kit_", ch->carrying)) )
    {
      temp = ch->equipment[HOLD];
      equipped = TRUE;
      if( (temp == 0) || !isname("_disguise_kit_", temp->name) )
      {
        act("You need a disguise kit.", FALSE, ch, 0, 0, TO_CHAR);
        if( target )
          free_char(target);
        return;
      }
    }
  }
  if( IS_TRUSTED(ch) || affected_by_spell(ch, ACH_DECEPTICON) )
    skl_lvl = 200;

  percent = number(1, 101);

  if( target )
  {
    percent += 30;

    if( racewar(ch, target) )
      percent += 20;

    if( GET_RACE(ch) != GET_RACE(target) )
      percent += 30;
    if( GET_SEX(ch) != GET_SEX(target) )
      percent += 10;
  }
  else if( GET_RACE(ch) != disguise_list_data[i].race )
  {
    percent += 30;
  }

  if( percent > skl_lvl )
  {
    send_to_char("You do a horrid job", ch);
    notch_skill(ch, SKILL_DISGUISE, 25);
    CharWait(ch, PULSE_VIOLENCE * 3);
    if( !IS_TRUSTED(ch) && !affected_by_spell(ch, ACH_DECEPTICON) && number(0, 1) )
    {
      send_to_char(", and ruin your disguise kit in the process.\r\n", ch);
      if( equipped )
        unequip_char(ch, HOLD);
      extract_obj(temp, TRUE); // An artifact disguise kit?
    }
    else
      send_to_char(", but manage to salvage the rest of the kit's supplies.\r\n", ch);
    if( target )
      free_char(target);

    return;
  }

  // Set size.
  if( target )
    the_size = GET_SIZE(target);
  else
    the_size = disguise_list_data[i].size;

  if( (GET_ALT_SIZE(ch) < (the_size - 1)) && !IS_TRUSTED(ch) )
  {
    send_to_char("You're too small for that!\r\n", ch);
    CharWait(ch, PULSE_VIOLENCE * 2);
  }
  else if( (GET_ALT_SIZE(ch) > (the_size + 1)) && !IS_TRUSTED(ch) )
  {
    send_to_char("You're too big for that!\r\n", ch);
    CharWait(ch, PULSE_VIOLENCE * 2);
  }
  else
  {
    // char, novictim, 1 increment, 3 = decepticon achievement
    update_achievements(ch, 0, 1, 3);
    if( target )
    {
      IS_DISGUISE_PC(ch) = TRUE;
      IS_DISGUISE_NPC(ch) = FALSE;
      IS_DISGUISE_ILLUSION(ch) = FALSE;
      IS_DISGUISE_SHAPE(ch) = FALSE;
      ch->disguise.name = str_dup(GET_NAME(target));
      ch->disguise.m_class = target->player.m_class;
      ch->disguise.race = GET_RACE(target);
      ch->disguise.level = GET_LEVEL(target);
      ch->disguise.hit = GET_LEVEL(ch) * 4;
      ch->disguise.racewar = GET_RACEWAR(target);
      if (GET_TITLE(target))
        ch->disguise.title = str_dup(GET_TITLE(target));
      snprintf(Gbuf1, MAX_STRING_LENGTH, "You disguise yourself into %s.\r\n", GET_NAME(target));
      send_to_char(Gbuf1, ch);
      snprintf(Gbuf1, MAX_STRING_LENGTH, "%s starts disguising into %s.", GET_NAME(ch), GET_NAME(target));
    }
    else
    {

      int      k = number(0, 2);

      IS_DISGUISE_NPC(ch) = TRUE;
      IS_DISGUISE_PC(ch) = FALSE;
      IS_DISGUISE_ILLUSION(ch) = FALSE;
      IS_DISGUISE_SHAPE(ch) = FALSE;

      stripansi_2(disguise_list_data[i].name[k], Gbuf1);
      GET_DISGUISE_TITLE(ch) = str_dup(Gbuf1 + 2);

      ch->disguise.name = str_dup(disguise_list_data[i].name[k]);

      snprintf(Gbuf1, MAX_STRING_LENGTH, "%s is standing here, busy with his own matters.", disguise_list_data[i].name[k]);

      ch->disguise.longname = str_dup(Gbuf1);
      //GET_DISGUISE_TITLE(ch) = ch->disguise.name = ch->disguise.longname = str_dup(disguise_list_data[i].name[number(0, 2)]);
      ch->disguise.race = disguise_list_data[i].race;
      ch->disguise.hit = GET_LEVEL(ch) * 4;
      snprintf(Gbuf1, MAX_STRING_LENGTH, "You disguise yourself into a %s.\r\n", disguise_list[i]);
      send_to_char(Gbuf1, ch);
      snprintf(Gbuf1, MAX_STRING_LENGTH, "%s starts disguising into a %s.", GET_NAME(ch), disguise_list[i]);
    }
    SET_BIT(ch->specials.act, PLR_NOWHO);
    act(Gbuf1, TRUE, ch, NULL, NULL, TO_ROOM);
    if( !IS_TRUSTED(ch) && !affected_by_spell(ch, ACH_DECEPTICON) )
    {
      if( equipped )
        unequip_char(ch, HOLD);
      extract_obj(temp, TRUE); // An artifact disguise kit?
    }
    notch_skill(ch, SKILL_DISGUISE, 6.25);
    CharWait(ch, PULSE_VIOLENCE * 5);
  }
  if( target )
    free_char(target);
  return;
}

void remove_disguise(P_char ch, bool show_messages)
{
  bool wasIllu = IS_DISGUISE_ILLUSION(ch);
  bool wasShape = IS_DISGUISE_SHAPE(ch);

  REMOVE_BIT(ch->specials.act, PLR_NOWHO);
  IS_DISGUISE_PC(ch) = FALSE;
  IS_DISGUISE_NPC(ch) = FALSE;
  IS_DISGUISE_ILLUSION(ch) = FALSE;
  IS_DISGUISE_SHAPE(ch) = FALSE;
  str_free(ch->disguise.name);
  ch->disguise.name = NULL;
  str_free(ch->disguise.longname);
  ch->disguise.longname = NULL;
  ch->disguise.m_class = 0;
  ch->disguise.race = 0;
  ch->disguise.level = 0;
  str_free(ch->disguise.title);
  ch->disguise.title = NULL;
  ch->disguise.hit = 0;
  if (show_messages)
  {
    if (wasIllu)
    {
      send_to_char("The illusion surrounding you fades away!\r\n", ch);
      act("The illusion surrounding $n fades apart!", FALSE, ch, 0, 0, TO_ROOM);
    } 
    else if (wasShape)
    {
      send_to_char("You shift back into your normal form!\r\n", ch);
      act("$n shifts back into their normal form!", FALSE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      send_to_char("Your disguise falls apart!\r\n", ch);
      act("$n's disguise falls apart!", FALSE, ch, 0, 0, TO_ROOM);
    }
  }
}


