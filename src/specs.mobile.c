 /*
  * ***************************************************************************
  * *  File: specs.mobile.c                                     Part of Duris *
  * *  Usage: special procedures for mobiles                                    *
  * *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
  * *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
  * ***************************************************************************
  */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "comm.h"
#include "db.h"
#include "damage.h"
#include "events.h"
#include "interp.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "range.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "weather.h"
#include "assocs.h"
#include "world_quest.h"
#include "sql.h"
#include "map.h"
#include "alliances.h"
#include "nexus_stones.h"
#include "epic.h"
#include "necromancy.h"
#include "vnum.obj.h"
#include "tradeskill.h"

/*
 * external variables
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const int top_of_world;
extern P_obj justice_items_list;
extern char *coin_names[];
extern const char *command[];
extern const char *dirs[];
extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int planes_room_num[];
extern int racial_base[];
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern const char *crime_list[];
extern const char *crime_rep[];
extern const char *specdata[][MAX_SPEC];
extern struct class_names class_names_table[];
int      range_scan_track(P_char ch, int distance, int type_scan);
extern P_obj    object_list;
extern void give_proper_stat(P_char);
extern void insectbite(P_char, P_char);
extern P_char guard_check(P_char, P_char);
extern P_char pick_target(P_char, unsigned int);
extern int cast_as_damage_area(P_char, void (*func)
  (int, P_char, char *, int, P_char, P_obj), int, P_char, float, float);

struct social_type
{
  char    *cmd;
  int      next_line;
};

struct obj_cost
{
  int      total_cost;
  int      no_carried;
  bool     ok;
};

static int songcounter = 0;

int block_up(P_char ch, P_char pl, int cmd, char *arg)
{
  int      allowed = 0;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  allowed = 0;
  if (!ch)
    return 0;
  if (!pl)
    return 0;

  if( cmd != CMD_UP )
    return FALSE;

  if (IS_TRUSTED(pl) || IS_NPC(pl))
    allowed = 1;
  else
    allowed = 0;

  if (allowed)
  {
    act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
    act("$N nods and stands aside to let you pass.", FALSE, pl, 0, ch, TO_CHAR);
    return FALSE;
  }
  
  /* BLOCK! */
  act("$N &+yjumps in your path blocking the exit!&n.", FALSE, pl, 0, ch, TO_CHAR);
  act("$N &+yjumps in the way of $n blocking the exit!&n.", FALSE, pl, 0, ch, TO_NOTVICT);
  return TRUE;
}

int harpy_evil(P_char ch, P_char pl, int cmd, char *arg)
{
  char     obj_name[MAX_INPUT_LENGTH], *argument;
  P_obj    obj;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!IS_AWAKE(ch))
    return FALSE;

  if (cmd == CMD_GIVE)
  {
    argument = arg;
    argument = one_argument(argument, obj_name);


    if (GET_RACEWAR(pl) != RACEWAR_NEUTRAL)
    {
      mobsay(ch, "You have already picked a path.");
      return TRUE;
    }


    obj = get_obj_in_list_vis(pl, obj_name, pl->carrying);
    if (!obj)
    {
      send_to_char("You do not seem to have anything like that.\r\n", pl);
      return TRUE;
    }
    if( OBJ_VNUM(obj) == VOBJ_HARPY_CHOOSE_FEATHER )
    {
      act("$n gives $p to $N.", 1, pl, obj, ch, TO_NOTVICT);
      act("$n gives you $p.", 0, pl, obj, ch, TO_VICT);
      send_to_char("Ok.\r\n", pl);

      extract_obj(obj, TRUE); // Not an arti.

      GET_RACE(pl) = RACE_HARPY;
      GET_RACEWAR(pl) = RACEWAR_EVIL;
      GET_ALIGNMENT(pl) = -1000;

      mobsay(ch, "You have chosen the path of the darkness.  Now go slay some innocents.");

      return TRUE;
    }
  }

  return FALSE;

}
int harpy_good(P_char ch, P_char pl, int cmd, char *arg)
{
  char     obj_name[MAX_INPUT_LENGTH], *argument;
  P_obj    obj;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!IS_AWAKE(ch))
    return FALSE;

  if (cmd == CMD_GIVE)
  {
    argument = arg;
    argument = one_argument(argument, obj_name);

    if (GET_RACEWAR(pl) != RACEWAR_NEUTRAL)
    {
      mobsay(ch, "You have already picked a path.");
      return TRUE;
    }

    obj = get_obj_in_list_vis(pl, obj_name, pl->carrying);
    if (!obj)
    {
      send_to_char("You do not seem to have anything like that.\r\n", pl);
      return TRUE;
    }
    if( OBJ_VNUM(obj) == VOBJ_HARPY_CHOOSE_FEATHER )
    {
      act("$n gives $p to $N.", 1, pl, obj, ch, TO_NOTVICT);
      act("$n gives you $p.", 0, pl, obj, ch, TO_VICT);
      send_to_char("Ok.\r\n", pl);

      extract_obj(obj, TRUE); // Not an arti.

      GET_RACE(pl) = RACE_HARPY;
      GET_RACEWAR(pl) = RACEWAR_GOOD;
      GET_ALIGNMENT(pl) = 1000;

      mobsay(ch, "Always protect the innocent and be a beacon for justice and good.");
      return TRUE;
    }
  }

  return FALSE;

}

int gargoyle_master(P_char ch, P_char pl, int cmd, char *arg)
{
  int      corpse_num = 0;
  P_obj    obj, next_obj;
  char     buf[1024];

  memset(buf, 0, sizeof(buf));

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!IS_AWAKE(ch))
    return FALSE;

  if (cmd != -2)
  {
    if (cmd && pl)
    {
      if (pl == ch)
        return FALSE;

      if (cmd == CMD_LOOK)
      {
        if (!isname(arg, "gargoyle"))
          return FALSE;
        mobsay(ch, "Ask me if you want to be undead.");
      }
      if (arg && cmd == CMD_ASK)
      {
        if (!isname(arg, "gargoyle undead") &&
            !isname(arg, "Gargoyle undead"))
          return FALSE;

        if (GET_RACEWAR(pl) != RACEWAR_NEUTRAL)
        {
          mobsay(ch, "You have already picked a path.");
          return TRUE;
        }

        for (obj = world[ch->in_room].contents; obj; obj = next_obj)
        {
          next_obj = obj->next_content;
          if ((obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], NPC_CORPSE))
            if (strstr(obj->name, "harpy"))
              corpse_num++;
        }

        if (corpse_num < 2)
        {
          corpse_num = 2 - corpse_num;

          if (corpse_num == 1)
            snprintf(buf, sizeof(buf), "You need %d more corpse.",
                     corpse_num);
          else
            snprintf(buf, sizeof(buf), "You need %d more corpses.",
                     corpse_num);

          mobsay(ch, buf);
        }
        else
        {                       /*

                                   for (obj = world[ch->in_room].contents; obj; obj = next_obj) {
                                   next_obj = obj->next_content;
                                   if ((obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], NPC_CORPSE))
                                   if(strstr(obj->name, "harpy"))
                                   extract_obj(obj, TRUE); // Not an arti, but may contain one and is 'in game.'
                                   }

                                   send_to_char("&+LThe monsterous gargoyle grins evilly, then with lightning speed plunges his clawed hands into your chest.\n", pl);

                                   send_to_char("&+rYou feel yourself falling to the ground.\n", pl);
                                   send_to_char("&+rYour soul leaves your body in the cold sleep of death...\n", pl);

                                   send_to_char("&+LYou slowly open your eyes, gazing out at onto the world through the glaze of undeath.\n", pl);

                                   send_to_char("&+WYou have been reborn!\n", pl);

                                   act("&+rYou hear the death cry of $n.", TRUE, pl, 0, ch, TO_NOTVICT);
                                   act("&+rSuddenly, $n staggers to $s feet.", TRUE, pl, 0, ch, TO_NOTVICT);
                                   act("&+W$n has been reborn!", TRUE, pl, 0, ch, TO_NOTVICT);

                                   GET_RACE(pl) = RACE_GARGOYLE;
                                   GET_RACEWAR(pl) = RACEWAR_UNDEAD;
                                   GET_ALIGNMENT(pl) = -1000;

                                   if(GET_CLASS(pl, CLASS_BARD))
                                   pl->player.m_class = CLASS_SPIPER;
                                   //              GET_CLASS(pl) = CLASS_SPIPER; */
        }

        return TRUE;
      }

    }
  }
  return FALSE;
}

int temple_illyn(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 30))
  {
  case 0:
    mobsay(ch, "Fire is the key!");
    do_action(ch, 0, CMD_GRUMBLE);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GRUMBLE);
    return TRUE;
  }
  return FALSE;
}

int bs_boss(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Are you some kind of sick freak or what?");
    do_action(ch, 0, CMD_GLARE);
    return TRUE;
  case 1:
    mobsay(ch, "I am going to slaughter your hide when I am done here.");
    return TRUE;
  case 2:
    mobsay(ch, "Oh god, please have mercy!");
    do_action(ch, 0, CMD_GRUNT);
    return TRUE;
  case 3:
    act("$n squeezes out a huge chunk of shit.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "Damn! I forgot the toilet parchment!");
    do_action(ch, 0, CMD_MOAN);
    return TRUE;
  }
  return FALSE;
}

int braddistock(P_char ch, P_char pl, int cmd, char *arg)
{
  // Check for periodic event calls
  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // MobCombat() will call this function with pl, cmd, and arg set to null
  // Immortals aren't blocked.
  if( !pl || IS_TRUSTED(pl) )
  {
    return FALSE;
  }

  if( cmd == CMD_NORTH && pl != ch && (GET_LEVEL(pl) > 14) )
  {
    send_to_char("The spirit of Lord Braddistock says 'We don't want your kind around here!'", ch);
    act("$N says 'We don't want your kind around here!'", TRUE, pl, 0, ch, TO_NOTVICT);
    act("$N blocks your passage.", TRUE, pl, 0, ch, TO_CHAR);
    act("$N blocks $n.", TRUE, pl, 0, ch, TO_NOTVICT);
    return TRUE;
  }

  return FALSE;
}

int kimordril_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 95505, 95532, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+WHelp me elite guard, we are being attacked by %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int silver_lady_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 11302, 11303, 11304, 11305, 11307, 11308, 11310,
    11312, 11314, 0
  };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch)
    return shout_and_hunt(ch, 30,
                          "Help me mates!  We be under attack by %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int realms_master_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 11102, 11103, 11104, 11105, 11107, 11108, 11110,
    11112, 11114, 0
  };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch)
    return shout_and_hunt(ch, 30,
                          "Help me mates!  We be under attack by %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int imix_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 25450, 25430, 25410, 25415, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+WDenizens of fire!  Come and destroy %s!", NULL,
                          helpers, 0, 0);
  return FALSE;
}


int strychnesch_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 32830, 32829, 32831, 32832, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+WRaveners come to my aid, there are intruders within our domain! Kill %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int morgoor_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 32828, 32829, 32831, 32832, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+WRaveners come to my aid, there are intruders within our domain! Kill %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int jabulanth_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 32828, 32830, 32831, 32832, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+WRaveners come to my aid, there are intruders within our domain! Kill %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int redpal_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 32828, 32830, 32829, 32832, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+WRaveners come to my aid, there are intruders within our domain! Kill %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int cyvrand_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 32828, 32830, 32829, 32831, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+WRaveners come to my aid, there are intruders within our domain! Kill %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int overseer_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 32804, 32805, 32806, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+MInvaders!!!  Guards, come and help me destroy %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int caranthazal_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 32835, 32836, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+MBrethren, we have been invaded, come to me and dispose of %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int menzellon_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 12401, 12410, 12420, 12430, 12440, 12450, 12460, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+BYou shall DIE!  Denizens of the Ether, come absorb %s!", NULL, helpers, 0, 0);
  return FALSE;
}

int ogremoch_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int helpers[] = { 23801, 23802, 23803, 23804, 23807, 23927, 23862, 0 };
  if(cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }
  if(!tch && !number(0, 4))
  {
    return shout_and_hunt(ch, 50, "&+YCreatures of the earth!  Come to my aid and crush %s!", NULL, helpers, 0, 0);
  }
  return FALSE;
}

int olhydra_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 23200, 23210, 23215, 23220, 23230, 23250, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+bBeings of Water!  Assemble at once and destroy %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int yancbin_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 24400, 24410, 24415, 24420, 24430, 24450, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100, "&+CDenizens of air, destroy %s!",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int demogorgon_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 19830, 19850, 19860, 19880, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100, "&+GYou will pay for attacking me mortal worms!   Denizens of darkness, come and feast upon %s!", NULL, helpers, 0, 0);
  return FALSE;
}

int warden_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 7334, 7335, 7368, 7369, 7367, 7315, 7314, 7317, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 7))
    return shout_and_hunt(ch, 100, "&+MGuards!!!  Come at once! Destroy %s!", NULL, helpers, 0, 0);
  return FALSE;
}

int sister_knight(P_char ch, P_char tch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 8))
    return shout_and_hunt(ch, 100,
                          "Come, my sisters, we are under attack by %s!", sister_knight, NULL, 0, 0);
  return FALSE;
}

int good_city_guard(P_char ch, P_char tch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch)
    return shout_and_hunt(ch, 100, "%s has dared to attack me!",
                          good_city_guard, NULL, 0, 0);
  return FALSE;
}

void fetid_breath(P_char ch, P_char victim)
{
	int dam, level;
	bool knock = FALSE;
	bool feint = FALSE;

  struct damage_messages messages = {
    "$N &+gis hit by your fetid breath.",
    "$n &+gbreathes a stream of &+Gfetid gases&n&+g at you.",
    "$N &+gis covered with a stream of &+Gfetid gases&n&+g breathed by $n.",
    "$N &+gis dead, &+Gsuffocated&n&+g by your fetid breath.",
    "&+gYou are &+Gsuffocated&n&+g by the fetid gasses that $n&n&+g breathes on you.",
    "$n's &+gbreath kills $N &+gon the spot.", 0
  };
	
	if (resists_spell(ch, victim))
	  return;
	
	level = GET_LEVEL(ch);
	
	dam = dice(level, 8) + level;
	
	if (StatSave(victim, APPLY_CON, 0))
	  dam >> 1;
	
	if (!StatSave(victim, APPLY_AGI, 0))
	  knock = TRUE;
	
	if (!StatSave(victim, APPLY_WIS, 0))
	  feint = TRUE;

  spell_damage(ch, victim, dam, SPLDAM_GAS, SPLDAM_BREATH | SPLDAM_NOSHRUG, &messages);
  
  if (IS_ALIVE(victim) && knock)
  {
  	SET_POS(victim, POS_SITTING + GET_STAT(victim));
  	act ("$n &+Lis knocked down by the power of &+gthe &+Gfetid&n&+g breath!",
       FALSE, victim, 0, 0, TO_ROOM);
    send_to_char ("&+LYou are knocked down by the power of &+gthe &+Gfetid&n&+g breath!\n",
       victim);
  }
  
  if (IS_ALIVE(victim) && feint)
  {
    struct affected_type af;
    
    bzero(&af, sizeof(af));
    af.type = SPELL_MINOR_PARALYSIS;
    af.flags = AFFTYPE_SHORT;
    af.duration = WAIT_SEC;
    af.bitvector2 = AFF2_MINOR_PARALYSIS;

    affect_to_char(victim, &af);
    
    act ("$n &+Wturns pale as some magical force occupies $s body, causing all motion to halt.",
       FALSE, victim, 0, 0, TO_ROOM);
    send_to_char ("&+LYour body becomes like stone as the paralyzation takes effect.\n",
       victim);
    if (IS_FIGHTING(victim))
      stop_fighting(victim);
    if( IS_DESTROYING(victim) )
      stop_destroying(victim);
  }

}

int do_fetid_breath(P_char ch)
{
	P_char tch, tch_next;
	
  for (tch = world[ch->in_room].people; tch; tch = tch_next)
  {
    tch_next = tch->next_in_room;
    
    if (IS_TRUSTED(tch))
      continue;
    
    if ((ch->specials.fighting == tch) ||
        (tch->specials.fighting == ch))
    {
      fetid_breath(ch, tch);
      continue;
    }
    
    if ((get_linking_char(ch, LNK_RIDING) == tch) || (ch == tch) ||
        grouped(ch, tch))
      continue;
    
    if (IS_NPC(tch) && !IS_PC_PET(tch))
      continue;
    
    fetid_breath(ch, tch);
  }
}

int whirlwind_of_teetch(P_char ch, int targets)
{
	P_char victim;
	int hit = 0;
	
	if (!IS_FIGHTING(ch))
	return 0;
	
	act("$n&+W sends our a deep howl, and charges at $s&+W foes!",
	       TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("&+WYou send out a deep howl as you charge at your foes!", ch);
	
	for (int i = targets; i && IS_ALIVE(ch); i--) {
		if (victim = pick_target(ch, PT_TOLERANT))
		  insectbite(ch, victim);
	}
}

void hyena_bite(P_char ch, P_char victim)
{
  struct damage_messages messages = {
    "Your shape blurs as you lash towards $N and sink your fangs in $S flesh.",
    "$n's shape blurs as $e lashes towards you and sinks $s fangs in your flesh.",
    "$n's shape blurs as $e lashes towards $N and sinks $s fangs in $S flesh.",
    "Your shape blurs as you lash towards $N and sink your fangs in $S flesh.",
    "$n's shape blurs as $e lashes towards you and sinks $s fangs in your flesh.",
    "$n's shape blurs as $e lashes towards $N and sinks $s fangs in $S flesh.",
  };

	int level = GET_LEVEL(ch);
	
	int dam = dice(level, 10);
	
	if (raw_damage(ch, victim, dam, RAWDAM_DEFAULT, &messages) ==
      DAM_NONEDEAD)
  {
    int i = 1 + GET_LEVEL(ch)/12;
    while (i-- && !affected_by_spell(victim, SPELL_DISEASE))
      spell_disease(GET_LEVEL(ch), ch, 0, 0, victim, 0);
  }
}


int yeenoghu(P_char ch, P_char tch, int cmd, char *arg)
{
	P_char victim;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!IS_FIGHTING(ch))
    return FALSE;
    
  if (!number(0, 9) && GET_HIT(ch) < GET_MAX_HIT(ch)/2)
  {
  	act("$n&+L raises his clawed hands and emits a screeching scream!", 
	    TRUE, ch, 0, 0, TO_ROOM);
	  spell_full_heal(GET_LEVEL(ch), ch, 0, 0, ch, 0);
  }
  else if (!number(0, 5) && GET_HIT(ch) < GET_MAX_HIT(ch)/4)
  {
  	act("$n&+L raises his clawed hands and emits a screeching scream!", 
	    TRUE, ch, 0, 0, TO_ROOM);
	  spell_full_heal(GET_LEVEL(ch), ch, 0, 0, ch, 0);
  }
  else if (!number(0, 6))
    do_fetid_breath(ch);
  else if (!number(0, 5))
    whirlwind_of_teetch(ch, dice(2, 6));
  else if (!number(0, 3))
  {
  	P_char victim;
  	if (victim = pick_target(ch, PT_NUKETARGET | PT_WEAKEST))
      hyena_bite(ch, victim);
  }
}

void demogorgon_tail(P_char ch)
{
  int dam;
  P_char victim = pick_target(ch, PT_TOLERANT);
  
  if (!victim)
    return;

  act("$n &+Llashes out with $s &+Lpowerful &+rdemon spiked &+Ltail!&n",
    FALSE, ch, 0, 0, TO_ROOM);
 
  int level = GET_LEVEL(ch);  
  
  if (number(0, 2))
  {
    send_to_char("&+LYou are hit by &+GDemogorgon's &+Lmighty tail!\r\n",
      victim);
    act("$N is hit by &+GDemogorgon's &+Lmighty tail!",
      TRUE, ch, 0, victim, TO_NOTVICT);
    dam = dice(level, 15);
    spell_dispel_magic(60, ch, 0, 0, victim, 0);
    melee_damage(ch, victim, dam, 0, 0);
    spell_energy_drain(level, ch, 0, 0, victim, 0);
  }
  else
  {
    dam = dice(level, 11);
    
    P_char tch, next_tch;
    
    for (tch = world[ch->in_room].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;
      
      if (should_area_hit(ch, tch) && number(0, 1))
      {
        send_to_char("&+LYou are hit by &+GDemogorgon's &+Lthe sweeping tail!\r\n", tch);
        act("$N is hit by &+GDemogorgon's &+Lsweeping tail!", TRUE, ch, 0, tch, TO_NOTVICT);
        melee_damage(ch, victim, dam, 0, 0);
        dam *= 10;
        dam /= 9;
      }
    }
    
  }

}

void demogorgon_second_head(P_char ch)
{
  act("&+GThe second head of&n $n &+Yglares &+Garound...", FALSE, ch, 0, 0, TO_ROOM);
  
  P_char victim = pick_target(ch, PT_WEAKEST | PT_TOLERANT);

  if (!victim)
    return;

  if (!number(0, 3))
    spell_negative_concussion_blast(GET_LEVEL(ch), ch, 0, 0, victim, 0);
  else if (!number(0, 2))
    do_fetid_breath(ch);
  else if (!number(0, 6))
    cast_as_damage_area(ch, spell_cdoom, GET_LEVEL(ch), victim, 50, 20);
  else
    hyena_bite(ch, victim);

}

int demogorgon(P_char ch, P_char tch, int cmd, char *arg)
{
  
  P_char victim;
  int      helpers[] = { 19830, 19850, 19860, 19880, 19840, 19870, 19400, 19901, 19760, 0 };
  static bool stats_increased = FALSE;
  static bool demogorgon_shouted = FALSE;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || cmd || IS_IMMOBILE(ch))
    return FALSE;

/*
  He's a demon prince, he should have proper stats.
*/  

  if (!stats_increased)
  {
    stats_increased = TRUE;
    give_proper_stat(ch);
    act("$n &+Lgrows more powerful!", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  
/*
  Recharging the shout action, and preparing for the next battle
  (a little surprise for groups that massed and/or had to retreat)
*/
  
  if (demogorgon_shouted && !number(0, 10) && !IS_FIGHTING(ch))
  {
    demogorgon_shouted = FALSE; /* he's ready for shouting again */
    act("$n &+Lregains his posture, ready for any forthcoming battles.", FALSE, ch, 0, 0, TO_ROOM);
    wizlog(MINLVLIMMORTAL, "Demogorgon has just regained the ability to call for help.");
    return TRUE;
  }
  
  if (!IS_FIGHTING(ch))
    return FALSE;

/* 
  Note, the new "call friends in" radius is only TWO - this makes it
  possible for groups to take chances with tackling the boss before 
  having actually fully cleared the grid. Radius 3 is still a good
  chunk of the cubic grid. Most of it actually. To check if this shouldn't
  get lowered down to 2.
*/

  if (!demogorgon_shouted) 
  {
    demogorgon_shouted = TRUE;
    shout_and_hunt(ch, 3, "&+GYou will pay for attacking me mortal worms!   Denizens of darkness, come and feast upon %s!", NULL, helpers, 0, 0);
    return TRUE;
  }

/*
  And now it's time for his second head to act. 
  Alternatively, both heads concentrate on using the tail.
*/

  switch (number(0, 2))
  {
    case 0:
    case 1:
      demogorgon_second_head(ch);
      break;
    case 2:
      demogorgon_tail(ch);
      break;
    default:
      break;
  }
  return TRUE;
}

int astral_succubus(P_char ch, P_char tch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 10))
    return shout_and_hunt(ch, 10,
                          "&+RCome, my sisters, we are under attack by %s!", astral_succubus, NULL, 0, 0);
  return FALSE;
}

int earth_treant(P_char ch, P_char tch, int cmd, char *arg)
{
  P_char victim;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!IS_FIGHTING(ch))
    return FALSE;

  if (!number(0, 6))
  {
    if (!(victim = pick_target(ch, PT_CASTER | PT_STANDING | PT_SMALLER | PT_TOLERANT)))
      return FALSE;
    else
    {
    	if (ch->specials.fighting != victim)
    	{
    	  stop_fighting(ch);
        act("$n switches targets...", FALSE, ch, 0, 0, TO_ROOM);
        victim = guard_check(ch, victim);
        set_fighting(ch, victim);
        send_to_char("You switch opponents!\n", ch);
      } 
      if (GET_POS(victim) < POS_STANDING)
        hit(ch, victim, ch->equipment[PRIMARY_WEAPON]);
      else
        branch(ch, victim);
      return TRUE;
    }
  }

  return FALSE;	
}

int shadow_demon(P_char ch, P_char tch, int cmd, char *arg)
{
  P_char   vict;
  int      Mask;
  char     buf[MAX_INPUT_LENGTH], password[MAX_INPUT_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_WHISPER)       /*
                                 * Whisper
                                 */
    return (FALSE);

  if ((!CAN_SEE(ch, tch) || !CAN_SEE(tch, ch)) || IS_FIGHTING(ch) || !IS_AWAKE(ch))        /*
                                                                                         * if dragon can't see player
                                                                                         */
    return (FALSE);

  if (cmd == CMD_WHISPER)
  {
    half_chop(arg, buf, password);
    if (!*buf || !*password ||
        (!(vict = get_char_room_vis(tch, buf))) || (vict != ch))
      return (FALSE);

    if (str_cmp(password, "masque"))    /*
                                         * Password
                                         */
      return (FALSE);

    if (CAN_SEE(ch, tch) && IS_PC(tch) && !tch->specials.fighting)
    {
      if (tch->equipment[GUILD_INSIGNIA])
        Mask =
          obj_index[tch->equipment[GUILD_INSIGNIA]->R_num].virtual_number;
      else
        Mask = 0;
      if ((Mask == 8501) || (Mask == 8502) || (Mask == 8503))
      {
        act("$n whispers, 'Greetings follower of Mask!'\r\n"
            "The demon utters an arcane magical phrase, casting a powerful incantation!\r\n"
            "&+LThe room blackens with dark energy, shadows envelop the room........",
            FALSE, ch, 0, 0, TO_ROOM);
        act
          ("&+L$N is lost to the shadows of darkness and slowly slips from sight...",
           FALSE, ch, 0, tch, TO_NOTVICT);
        sprintf(buf,
                "&+LThe shadows lift for a moment as %s fades into exsistance..\r\n",
                GET_NAME(tch));
        send_to_room(buf, real_room(8450));
        send_to_char
          ("&+LA dark shadow envelops you as you fade from the room!\r\n"
           "The darkness vanishes and you stand inside the guild hall of the\r\n"
           "&+L<<=Shadowys of Dought=>>&N..\r\n", tch);
        char_from_room(tch);
        char_to_room(tch, real_room(8450), 0);
        act
          ("The shadows retreat as the mighty demon goes back into hiding......",
           FALSE, ch, 0, 0, TO_ROOM);
        return (TRUE);
      }
    }
  }
  return (FALSE);
}

int tiaka_ghoul(P_char ch, P_char tch, int cmd, char *arg)
{
  P_char   vict;
  int      Mask, GoodAlignment = 1;     /*
                                         * Define what tiaka consider a good align.
                                         */
  char     buf[MAX_INPUT_LENGTH], password[MAX_INPUT_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_WHISPER)
    return (FALSE);

  if ((!CAN_SEE(ch, tch) || !CAN_SEE(tch, ch)) || IS_FIGHTING(ch) || !IS_AWAKE(ch))        /*
                                                                                         * if dragon can't see player
                                                                                         */
    return (FALSE);

  if (cmd == CMD_WHISPER)
  {
    half_chop(arg, buf, password);
    if (!*buf || !*password ||
        (!(vict = get_char_room_vis(tch, buf))) || (vict != ch))
      return (FALSE);

    if (str_cmp(password, "blackend"))  /*
                                         * Password
                                         */
      return (FALSE);

    if (CAN_SEE(ch, tch) && !IS_NPC(tch) && !tch->specials.fighting)
    {
      if (tch->equipment[GUILD_INSIGNIA])
        Mask =
          obj_index[tch->equipment[GUILD_INSIGNIA]->R_num].virtual_number;
      else
        Mask = 0;
      if ((Mask == 1263) || (Mask == 1264))
      {
        if (GET_ALIGNMENT(tch) > GoodAlignment)
        {
          act("$n glances at $N, peering into $N's soul.", FALSE, ch, 0, tch,
              TO_NOTVICT);
          act("$n glances at you, scanning your soul.", FALSE, ch, 0, tch,
              TO_VICT);
          act
            ("$n grins at $N, and says 'Thou shall surly perish if thou stays here.'",
             FALSE, ch, 0, tch, TO_NOTVICT);
          act
            ("$n grins at you, and says 'Thou shall surly perish if thou stays here.'",
             FALSE, ch, 0, tch, TO_VICT);
          return (FALSE);
        }
        act("$n says, 'Thou are worth to enter.'", FALSE, ch, 0, 0, TO_ROOM);
        act("&+rTiaka&N bows to you, and opens the portal to the guildhall.",
            FALSE, ch, 0, 0, TO_ROOM);
        act("&+RThe portal swirls around the room, shifting colors slightly.",
            FALSE, ch, 0, 0, TO_ROOM);

        act
          ("&+r$N is enveloped into the portal, and slowly fades out of exsistance.",
           FALSE, ch, 0, tch, TO_NOTVICT);
        sprintf(buf,
                "&+rA portal opens up inside the room and %s steps through it..\r\n",
                GET_NAME(tch));
        send_to_room(buf, real_room(8556));
        send_to_char
          ("You are partially blinded as you step through the portal.\r\n",
           tch);
        send_to_char
          ("The light of the portal fades slowly from the room.\r\n", tch);
        char_from_room(tch);
        char_to_room(tch, real_room(8556), 0);
        act("Tiaka closes the portal, and stands back at attention.", FALSE,
            ch, 0, 0, TO_ROOM);
        return (TRUE);
      }
    }
  }
  return (FALSE);
}

int mystra_dragon(P_char ch, P_char tch, int cmd, char *arg)
{
  P_char   vict;
  int      Mask, EvilAlignment = -350;
  char     buf[MAX_INPUT_LENGTH], password[MAX_INPUT_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_WHISPER)       /*
                                 * Whisper
                                 */
    return (FALSE);

  if ((!CAN_SEE(ch, tch) || !CAN_SEE(tch, ch)) || IS_FIGHTING(ch) || !IS_AWAKE(ch))        /*
                                                                                         * if dragon can't see player
                                                                                         */
    return (FALSE);

  if (cmd == CMD_WHISPER)
  {
    half_chop(arg, buf, password);
    if (!*buf || !*password ||
        (!(vict = get_char_room_vis(tch, buf))) || (vict != ch))
      return (FALSE);

    if (str_cmp(password, "rovmelek"))  /*
                                         * Password
                                         */
      return (FALSE);

    if (CAN_SEE(ch, tch) && IS_PC(tch) && !tch->specials.fighting)
    {
      if (tch->equipment[GUILD_INSIGNIA])
        Mask =
          obj_index[tch->equipment[GUILD_INSIGNIA]->R_num].virtual_number;
      else
        Mask = 0;
      if ((Mask == 1240) || (Mask == 1241) || (Mask == 1242) ||
          (Mask == 1243))
      {
        if (GET_ALIGNMENT(tch) < EvilAlignment)
        {
          act("$n looks at $N with a penetrating stare, scanning $M.\r\n"
              "$n roars with rage at $N, \r\n"
              "'Evil wretch! You cannot enter Mystra's holy sanctum!'",
              FALSE, ch, 0, tch, TO_NOTVICT);
          act("$n looks at you with a penetrating stare, scanning you.\r\n"
              "$n roars with rage at you, \r\n"
              "'Evil wretch! You cannot enter Mystra's holy sanctum!'",
              FALSE, ch, 0, tch, TO_VICT);
          return (FALSE);
        }
        act("$n roars, 'Hail to the faithful of Mystra!'\r\n"
            "The dragon utters an arcane magical phrase, casting a powerful incantation!\r\n"
            "&+BThe room crackles with mystical energy, blue sparks shimmer and dance about.",
            FALSE, ch, 0, 0, TO_ROOM);
        act
          ("&+b$N is enveloped in a blue aura, and slowly fades out of exsistance.",
           FALSE, ch, 0, tch, TO_NOTVICT);
        sprintf(buf,
                "&+bA soft aura of light fills the room as %s fades into exsistance..\r\n",
                GET_NAME(tch));
        send_to_room(buf, real_room(8512));
        send_to_char
          ("&+bA soft aura of light surrounds you as you fade from the room!\r\n"
           "The aura vanishes and you stand inside the holy temple of Mystra..\r\n",
           tch);
        char_from_room(tch);
        char_to_room(tch, real_room(8512), 0);
        act
          ("The aura of magic ebbs, and the great dragon goes back to it's contemplation.",
           FALSE, ch, 0, 0, TO_ROOM);
        return (TRUE);
      }
    }
  }
  return (FALSE);
}

int hunt_cat(P_char ch, P_char tch, int cmd, char *arg)
{
  P_char   vict;
  int      Mask, EvilAlignment = -350;  /*
                                         * Define what cat consider an evil
                                         * align.
                                         */
  char     buf[MAX_INPUT_LENGTH], password[MAX_INPUT_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if ((cmd != CMD_WHISPER) && (cmd != CMD_MOUNT))
    return (FALSE);

  if ((!CAN_SEE(ch, tch) || !CAN_SEE(tch, ch)) || IS_FIGHTING(ch) || !IS_AWAKE(ch))        /*
                                                                                         * if cat can't see player
                                                                                         */
    return (FALSE);

  if (cmd == CMD_WHISPER)
  {
    half_chop(arg, buf, password);
    if (!*buf || !*password ||
        (!(vict = get_char_room_vis(tch, buf))) || (vict != ch))
      return (FALSE);

    if (str_cmp(password, "eternal"))   /*
                                         * Password
                                         */
      return (FALSE);

    if (CAN_SEE(ch, tch) && IS_PC(tch) && !tch->specials.fighting)
    {
      if (tch->equipment[GUILD_INSIGNIA])
        Mask =
          obj_index[tch->equipment[GUILD_INSIGNIA]->R_num].virtual_number;
      else
        Mask = 0;
      if ((Mask == 8427) || (Mask == 8428) || (Mask == 8429))
      {
        if (GET_ALIGNMENT(tch) < EvilAlignment)
        {
          act("$n looks at $N with a penetrating stare, scanning $M.", FALSE,
              ch, 0, tch, TO_NOTVICT);
          act("$n looks at you with a penetrating stare, scanning you.",
              FALSE, ch, 0, tch, TO_VICT);
          act
            ("$n roars with rage at $N, 'Evil wretch! You cannot enter &+RHouse Crimsonesti!!'",
             FALSE, ch, 0, tch, TO_NOTVICT);
          act
            ("$n roars at you, 'Evil wretch! You cannot enter &+RHouse Crimsonesti!!'",
             FALSE, ch, 0, tch, TO_VICT);
          return (FALSE);
        }
        act("$n roars, 'Long life to the faithful of Labelas!'", FALSE, ch, 0,
            0, TO_ROOM);
        act
          ("The cat utters and arcane magical phrase, casting a powerful incantation!",
           FALSE, ch, 0, 0, TO_ROOM);
        act
          ("&+RThe room crackles with mystical energy, crimson sparks shimmer and dance about.",
           FALSE, ch, 0, 0, TO_ROOM);
        act
          ("&+R$N is enveloped in a crimson aura, and slowly fades out of exsistance.",
           FALSE, ch, 0, tch, TO_NOTVICT);
        sprintf(buf,
                "&+RA soft aura of light fills the room as %s fades into exsistance..\r\n",
                GET_NAME(tch));
        send_to_room(buf, real_room(8426));
        send_to_char
          ("&+RA soft aura of light surrounds you as you fade from the room!\r\n",
           tch);
        send_to_char
          ("The aura vanishes and you stand inside the house of Crimsonesti!..\r\n",
           tch);
        char_from_room(tch);
        char_to_room(tch, real_room(8426), 0);
        act
          ("The aura of magic ebbs, and the great cat goes back to it's contemplation.",
           FALSE, ch, 0, 0, TO_ROOM);
        return (TRUE);
      }
    }
  }
  else if (cmd == CMD_MOUNT)
  {
    /*
     * okay, this is a stupid kludge, but it's basically so Tim can ride his **
     *
     * damned cat and mortals can't ride the one guarding house crimsonesti, **
     *
     * not that that's even used lately, but whatever...i'm too lazy to **
     * duplicate the cat with a different v-number and reassign the proc ** -
     * DTS 6/21/95
     */
    if (GET_LEVEL(tch) < MINLVLIMMORTAL)
    {
      act("$n growls as you try to mount $m.  You rethink the idea.",
          FALSE, ch, 0, tch, TO_VICT);
      act("$n growls as $N tries to mount $m.  $N rethinks the idea.",
          TRUE, ch, 0, tch, TO_NOTVICT);
      return (TRUE);
    }
    return (FALSE);
  }
  return (FALSE);
}

/*
 * spec death proc for hippogriff, mob 88815
 */

int hippogriff_die(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DEATH)
    act("As $n dies, it disintegrates in a flash of bright light!",
        TRUE, ch, 0, 0, TO_ROOM);
  return (FALSE);
}

/*
 * special death proc for crystal golem, mob 88814
 */

int crystal_golem_die(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DEATH)
    act("As $n dies, it shatters into crystal dust which quickly dissipates.",
        TRUE, ch, 0, 0, TO_ROOM);
  return (FALSE);
}

#define GUILD_ITEM_START 8508
#define GUILD_ITEM_END 8513
#define GUILD_ITEM_POS GUILD_INSIGNIA

int mailed_fist_guardian(P_char ch, P_char vict, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch->in_room != real_room(8574))
    return FALSE;

  if (cmd == CMD_NORTH)
    if (vict->equipment[GUILD_ITEM_POS] &&
        (obj_index[vict->equipment[GUILD_ITEM_POS]->R_num].virtual_number >=
         GUILD_ITEM_START) &&
        (obj_index[vict->equipment[GUILD_ITEM_POS]->R_num].virtual_number <=
         GUILD_ITEM_END))
    {
      act("The guard bows as $n enters the guild.", TRUE, ch, 0, 0, TO_ROOM);
      act("The guard bows as you enter the guild.", TRUE, ch, 0, 0, TO_CHAR);
      mobsay(ch, "Welcome, guildmember!");
      return FALSE;
    }
    else
    {
      mobsay(ch,
             "You look way too shifty to be member of guild dedicated to justice and duty!");
      act("$n blocks your entrance to the guild.", TRUE, ch, 0, vict,
          TO_VICT);
      act("$n blocks $N's entrance to the guild.", TRUE, ch, 0, vict,
          TO_NOTVICT);
      act("You block $N's entrance to the guild.", TRUE, ch, 0, vict,
          TO_CHAR);
      return TRUE;
    }
  if (vict)
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    switch (number(1, 8))
    {
    case 3:
      mobsay(ch,
             "If you're interested in upholding law and justice, and doing your duty..");
      mobsay(ch, "Contact nearest member of the guild.");
      break;
      /*
       * Waiting for _N_ other new messages.. :P
       */
    }
  }
  return FALSE;
}

#undef GUILD_ITEM_START
#undef GUILD_ITEM_END
#undef GUILD_ITEM_POS

int seas_coral_golem(P_char ch, P_char pl, int cmd, char *arg)
{
  int      earring = 0;
  char     Gbuf3[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if ((ch->in_room == real_room(20200)) && (cmd == CMD_SOUTH))
  {

    if (pl->equipment[GUILD_INSIGNIA])
      earring =
        obj_index[pl->equipment[GUILD_INSIGNIA]->R_num].virtual_number;

    if (earring == 20202)
    {                           /*
                                 * abalone earring
                                 */

      act("The coral golem bows before $n as $e enters the cave.", FALSE, pl,
          0, 0, TO_ROOM);
      send_to_char("The coral golem bows before you as you enter.\r\n", pl);
      act("$N leaves east, entering the cave.", FALSE, ch, 0, pl, TO_NOTVICT);
      sprintf(Gbuf3, "%s arrives from the west.\r\n", (GET_NAME(pl)));
      send_to_room(Gbuf3, real_room(20201));
      char_from_room(pl);
      char_to_room(pl, real_room(20201), 0);
      return TRUE;

    }
    else
    {

      act("The coral golem blocks $n's entry into the cave.", FALSE, pl, 0, 0,
          TO_ROOM);
      send_to_char("The coral golem blocks your entry into the cave.\r\n",
                   pl);
      send_to_char
        ("The coral golem says 'Only those of the Underground Seas may enter here.'\r\n",
         pl);
      return TRUE;

    }
  }
  if (pl)
  {
    return (0);
  }
  else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
  {
    switch (dice(2, 6))
    {
    case 2:
      act("$n seems to come to life as it smiles to you.", TRUE, ch, 0, 0,
          TO_ROOM);
      break;
    case 3:
      mobsay(ch, "Hail Valkur God of the Winds and the Oceans!");
      break;
    case 4:
      mobsay(ch, "Humans and barbarians!");
      mobsay(ch, "Speak to Prime about the Underground Seas!");
      break;
    case 5:
      act("$n keeps a close eye on you.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 6:
      mobsay(ch, "Humans and Barbarians!");
      mobsay(ch,
             "Speak to an Overseer about membership to the Underground Seas!");
      break;
    default:
      break;
    }
  }
  return FALSE;
}

#define RATE_TO_LOWER      10
#define RATE_TO_SILVER     15
#define RATE_TO_GOLD       25
#define RATE_TO_PLATINUM   45

int money_changer(P_char me, P_char ch, int cmd, char *arg)
{
  long     amount, from, to, n, ok, rate = 0;
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }
  if (!me || !ch || !IS_AWAKE(me) || IS_FIGHTING(me))
    return FALSE;

  if ((cmd != CMD_EXCHANGE) && (cmd != CMD_LIST))
    return FALSE;

  if (!CAN_SEE(me, ch))
  {
    mobsay(me, "How may I be of help if I cannot see you?");
    return TRUE;
  }
  if (cmd == CMD_LIST)
  {
    sprintf(Gbuf1,
            "Our surcharge is %d percent for exchange to a lower coin-type,",
            RATE_TO_LOWER);
    mobsay(me, Gbuf1);
    sprintf(Gbuf1, "%d percent for an exchange from copper to silver,",
            RATE_TO_SILVER);
    mobsay(me, Gbuf1);
    sprintf(Gbuf1,
            "%d percent for an exchange from copper or silver to gold,",
            RATE_TO_GOLD);
    mobsay(me, Gbuf1);
    sprintf(Gbuf1,
            "and %d percent for an exchange from a lesser coin to platinum.",
            RATE_TO_PLATINUM);
    mobsay(me, Gbuf1);
    return TRUE;
  }
  /*
   * else cmd == CMD_EXCHANGE
   */
  arg = one_argument(arg, Gbuf1);
  if (strlen(Gbuf1) > 6)
  {
    strcpy(Gbuf1,
           "Sorry, due to weight restrictions, we can't handle such large amounts.");
    mobsay(me, Gbuf1);
    return TRUE;
  }
  amount = strtol(Gbuf1, NULL, 0);

  if (amount < 0)
  {
    act("$n laughs at $N and says, 'Sorry, I don't deal in negative change.'",
        TRUE, ch, 0, me, TO_ROOM);
    act
      ("$N laughs at you and says, 'Sorry, I don't deal in negative change.'",
       TRUE, ch, 0, me, TO_CHAR);
    return FALSE;
  }
  arg = one_argument(arg, Gbuf1);
  from = coin_type(Gbuf1);
  one_argument(arg, Gbuf1);
  to = coin_type(Gbuf1);

  amount = amount * pow10(from);
  if ((amount == 0) || (from == -1) || (to == -1))
  {
    act("$n tries to exchange some coins with $N.", TRUE, ch, 0, me, TO_ROOM);
    act("$N stares blankly at $n.", TRUE, ch, 0, me, TO_ROOM);
    act("$N stares blankly at you.", TRUE, ch, 0, me, TO_CHAR);
    return FALSE;
  }
  if (from > to)
  {
    /*
     * not enough ?
     */
    if (ch->points.cash[from] < (amount / pow10(from)))
    {
      sprintf(Gbuf1,
              "You don't have enough %s coins to complete that exchange.\r\n",
              coin_names[from]);
      send_to_char(Gbuf1, ch);
      return TRUE;
    }
    n = (amount * (100 - RATE_TO_LOWER) / 100) / pow10(to);
    ok = (CAN_CARRY_COINS(ch) >= n);

    if (!ok)
    {
      send_to_char
        ("You can't carry the coins resulting from that exchange.\r\n", ch);
      return TRUE;
    }
    ch->points.cash[from] -= (amount / pow10(from));
    ch->points.cash[to] += n;
    act("$n exchanges some coins with $N.", TRUE, ch, 0, me, TO_ROOM);
    sprintf(Gbuf1, "You exchange %ld %s coins for %ld %s coins.\r\n",
            (amount / pow10(from)), coin_names[from], n, coin_names[to]);
    send_to_char(Gbuf1, ch);
    return TRUE;

  }
  if (from < to)
  {
    switch (to)
    {
    case 1:
      {
        rate = RATE_TO_SILVER;
      }
      break;
    case 2:
      {
        rate = RATE_TO_GOLD;
      }
      break;
    case 3:
      {
        rate = RATE_TO_PLATINUM;
      }
      break;
    default:
      {
        send_to_char("Unknown coin-type.\r\n", ch);
        return TRUE;
      }
      break;
    }

    amount =
      ((amount / ((pow10(to) * (10 + (rate / 10))) / 10)) *
       ((pow10(to) * (10 + (rate / 10))) / 10));

    if (amount == 0)
    {
      sprintf(Gbuf1,
              "You need to specify more %s coins to complete that exchange.\r\n",
              coin_names[from]);
      send_to_char(Gbuf1, ch);
      return TRUE;
    }
    if (ch->points.cash[from] < (amount / pow10(from)))
    {
      /*
       * not enough
       */
      sprintf(Gbuf1,
              "You don't have enough %s coins to complete that exchange.\r\n",
              coin_names[from]);
      send_to_char(Gbuf1, ch);
      return TRUE;
    }
    ch->points.cash[from] -= (amount / pow10(from));
    /*
     * weight check?
     */
    ch->points.cash[to] += amount / ((pow10(to) * (10 + (rate / 10))) / 10);
    act("$n exchanges some coins with $N.", TRUE, ch, 0, me, TO_ROOM);
    sprintf(Gbuf1, "You exchange %ld %s coins for %ld %s coins.\r\n",
            (amount / pow10(from)), coin_names[from],
            (amount / ((pow10(to) * (10 + (rate / 10))) / 10)),
            coin_names[to]);
    send_to_char(Gbuf1, ch);
    return TRUE;
  }
  if (from == to)
  {
    send_to_char("What a wiseguy.\r\n", ch);
    return TRUE;
  }
  return FALSE;
}

/*
 * social GENERAL PROCEDURES
 *
 * If first letter of the command is '!' this will mean that the following
 * command will be executed immediately.
 *
 * "G", n      : Sets next line to n
 * "g", n      : Sets next line relative to n, fx. line+=n
 * "m<dir>", n : move to <dir>, <dir> is 0, 1, 2, 3, 4 or 5
 * "w", n      : Wake up and set standing (if possible)
 * "c<txt>", n : Look for a person named <txt> in the room
 * "o<txt>", n : Look for an object named <txt> in the room
 * "r<int>", n : Test if the npc in room number <int>?
 * "s", n      : Go to sleep, return false if can't go sleep
 * "e<txt>", n : echo <txt> to the room, can use $o/$p/$N depending on
 * contents of the **thing
 * "E<txt>", n : Send <txt> to person pointed to by thing
 * "B<txt>", n : Send <txt> to room, except to thing
 * "?<num>", n : <num> in [1..99]. A random chance of <num>% success rate.
 * Will as usual advance one line upon sucess, and change
 * relative n lines upon failure.
 * "O<txt>", n : Open <txt> if in sight.
 * "C<txt>", n : Close <txt> if in sight.
 * "L<txt>", n : Lock <txt> if in sight.
 * "U<txt>", n : Unlock <txt> if in sight.
 */

/*
 * Execute a social command.
 */
void exec_social(P_char npc, char *cmd, int next_line, int *cur_line,
                 void **thing)
{
  bool     ok;

  if (IS_FIGHTING(npc))
    return;

  ok = TRUE;

  switch (*cmd)
  {

  case 'G':
    *cur_line = next_line;
    return;

  case 'g':
    *cur_line += next_line;
    return;

  case 'e':
    act(cmd + 1, FALSE, npc, (struct obj_data *) *thing, *thing, TO_ROOM);
    break;

  case 'E':
    act(cmd + 1, FALSE, npc, 0, *thing, TO_VICT);
    break;

  case 'B':
    act(cmd + 1, FALSE, npc, 0, *thing, TO_NOTVICT);
    break;

  case 'm':
    do_move(npc, 0, exitnumb_to_cmd(*(cmd + 1) - '0'));
    break;

  case 'w':
    if (IS_AWAKE(npc))
      ok = FALSE;
    else
      SET_POS(npc, POS_STANDING + STAT_NORMAL);
    break;

  case 's':
    if (!IS_AWAKE(npc))
      ok = FALSE;
    else
      SET_POS(npc, GET_POS(npc) + STAT_SLEEPING);
    break;

  case 'c':                    /*
                                 * Find char in room
                                 */
    *thing = get_char_room_vis(npc, cmd + 1);
    ok = (*thing != 0);
    break;

  case 'o':                    /*
                                 * Find object in room
                                 */
    *thing = get_obj_in_list_vis(npc, cmd + 1, world[npc->in_room].contents);
    ok = (*thing != 0);
    break;

  case 'r':                    /*
                                 * Test if in a certain room
                                 */
    ok = (npc->in_room == atoi(cmd + 1));
    break;

  case 'O':                    /*
                                 * Open something
                                 */
    do_open(npc, cmd + 1, 0);
    break;

  case 'C':                    /*
                                 * Close something
                                 */
    do_close(npc, cmd + 1, 0);
    break;

  case 'L':                    /*
                                 * Lock something
                                 */
    do_lock(npc, cmd + 1, 0);
    break;

  case 'U':                    /*
                                 * UnLock something
                                 */
    do_unlock(npc, cmd + 1, 0);
    break;

  case '?':                    /*
                                 * Test a random number
                                 */
    if (atoi(cmd + 1) <= number(1, 100))
      ok = FALSE;
    break;

  default:
    break;
  }                             /*
                                 * End Switch
                                 */

  if (ok)
    (*cur_line)++;
  else
    (*cur_line) += next_line;
}

int poison(P_char ch, P_char pl, int cmd, char *arg)
{
  int      type;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd)
    return FALSE;

  if (!IS_FIGHTING(ch))
    return FALSE;

  if (ch->specials.fighting &&
      (ch->specials.fighting->in_room == ch->in_room) &&
      !number(0, (MAXLVL - GET_LEVEL(ch))))
  {
    if (GET_LEVEL(ch) < 10)
      type = 2;
    else if (GET_LEVEL(ch) < 25)
      type = 1;
    else if (GET_LEVEL(ch) < 50)
      type = 3;
    else
      type = 10;
    act("$n bites $N!", 1, ch, 0, ch->specials.fighting, TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, ch->specials.fighting, TO_VICT);
    poison_neurotoxin(GET_LEVEL(ch), ch, 0, 0, ch->specials.fighting, 0);
    return TRUE;
  }
  return FALSE;
}

int thief(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   cons, next;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd)
    return FALSE;

  if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL) || IS_FIGHTING(ch))
    return FALSE;

  for (cons = world[ch->in_room].people; cons; cons = next)
  {
    next = cons->next_in_room;

    if (IS_PC(cons) && !IS_TRUSTED(cons) && !IS_FIGHTING(cons) && number(0, 1)
        && CAN_SEE(ch, cons))
    {
      npc_steal(ch, cons);
      return TRUE;
    }
  }

  return FALSE;
}

/*
 * ** used in the split shield area, copy of the guild guard changed
 * ** by Thomas Lowery 16 Jun 94
 */

int shady_man(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (IS_TRUSTED(ch))
  {
    return FALSE;
  }

  if ((ch->in_room == real_room(10302)) && (cmd == CMD_SOUTH))
  {
    if (GET_LEVEL(pl) > 20 && GET_LEVEL(pl) < 57)
    {
      act
        ("A shady old man whispers something to $n, stopping $m with his hand.",
         FALSE, pl, 0, 0, TO_ROOM);
      send_to_char
        ("A shady old man whispers 'This area is far below you, unless you wish to fight me.'\r\n",
         pl);
      return TRUE;
    }
  }
  return FALSE;
}

int gate_guard(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Gbuf3[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (IS_TRUSTED(ch))
  {
    return FALSE;
  }
  strcpy(Gbuf4,
         "a gate guard whispers 'We don't want your type here, get lost.'\r\n");
  strcpy(Gbuf3,
         "a gate guard whispers something to $n, stopping $m with his hand.");

  if ((ch->in_room == real_room(10320)) && (cmd == CMD_SOUTH))
  {
    if (GET_LEVEL(pl) < 51)
    {
      act(Gbuf3, FALSE, pl, 0, 0, TO_ROOM);
      send_to_char(Gbuf4, pl);
      return TRUE;
    }
  }
  return FALSE;
}

int guild_guard(P_char ch, P_char pl, int cmd, char *arg)
{
  bool     g_prot = FALSE, block = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH];
  int      Guild_Eq;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !pl)
    return FALSE;

  if ((ch->in_room != real_room(GET_BIRTHPLACE(ch))) ||
      (ch->in_room == NOWHERE))
    return FALSE;

  /*
   * ok, for ease of addition, add the virtual room number that the guard loads
   *
   * in as a case to the following switch.  After the case, set g_prot = TRUE
   * if this guard is supposed to use the guild_protector special (blast them,
   * outlaw them, teleport them away).  Check class restrictions and set block
   * = TRUE if the guard should block 'pl'. Note that guild guard's home is set
   *
   * to the room they load in, and this prevents them from doing ANYTHING
   * special, when they aren't in that room. Charmed, fled, transed, etc.  JAB
   */

  switch (world[ch->in_room].number)
  {
  case 17643:
    g_prot = FALSE;
    if (cmd == CMD_DOWN)
      block = TRUE;
    break;
  case 7528:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_ROGUE))
      block = TRUE;
    break;
  case 7565:
    g_prot = TRUE;
    if (((cmd == CMD_EAST) || (cmd == CMD_WEST)) &&
        !GET_CLASS(pl, CLASS_SHAMAN))
      block = TRUE;
    break;
  case 7578:
    g_prot = TRUE;
    if (((cmd == CMD_EAST) || (cmd == CMD_SOUTH)) &&
        !GET_CLASS(pl, CLASS_WARRIOR))
      block = TRUE;
    break;
  case 7584:
    g_prot = TRUE;
    if ((cmd == CMD_NORTH) &&
        !GET_CLASS(pl, CLASS_NECROMANCER) && !GET_CLASS(pl, CLASS_SUMMONER) &&
        !GET_CLASS(pl, CLASS_SORCERER) && !GET_CLASS(pl, CLASS_CONJURER))
      block = TRUE;
    break;
  case 7588:
    g_prot = TRUE;
    if ((cmd == CMD_EAST) && !GET_CLASS(pl, CLASS_CLERIC))
      block = TRUE;
    break;
  case 9300:
  case 9301:
    if (cmd == CMD_DOWN)
    {
      if (pl->equipment[GUILD_INSIGNIA])
        Guild_Eq =
          obj_index[pl->equipment[GUILD_INSIGNIA]->R_num].virtual_number;
      else
        Guild_Eq = 0;
      if ((Guild_Eq == 9301) || (Guild_Eq == 9302))
        break;
      else
        block = TRUE;
    }
    break;
  case 16501:
    if (cmd == CMD_NORTH)
    {
      if (pl->equipment[GUILD_INSIGNIA])
        Guild_Eq =
          obj_index[pl->equipment[GUILD_INSIGNIA]->R_num].virtual_number;
      else
        Guild_Eq = 0;
      if (Guild_Eq == 9316)
        break;
      else
        block = TRUE;
    }
    break;
  case 11603:
    g_prot = TRUE;
    if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_WARRIOR))
      block = TRUE;
    break;
  case 11633:
    g_prot = TRUE;
    if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_SHAMAN))
      block = TRUE;
    break;
  case 11619:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_MERCENARY))
      block = TRUE;
    break;
  case 8305:
    g_prot = TRUE;
    if ((cmd == CMD_EAST) && !GET_CLASS(pl, CLASS_RANGER))
      block = TRUE;
    break;
  case 8070:
  case 17550:
    g_prot = TRUE;
    if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_CLERIC))
      block = TRUE;
    break;
  case 8311:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_NECROMANCER))
      block = TRUE;
    break;
  case 8318:
    g_prot = TRUE;
    if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_BARD))
      block = TRUE;
    break;
  case 8200:
  case 17135:
    g_prot = TRUE;
    if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_ROGUE))
      block = TRUE;
    break;
  case 8137:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_DRUID))
      block = TRUE;
    break;
  case 8113:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_SORCERER))
      block = TRUE;
    break;
  case 8014:
  case 17221:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_WARRIOR))
      block = TRUE;
    break;
  case 16056:
    g_prot = TRUE;
    if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_CLERIC))
      block = TRUE;
    break;
  case 16392:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_ROGUE))
      block = TRUE;
    break;
  case 16192:
    g_prot = TRUE;
    if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_SORCERER))
      block = TRUE;
    break;
  case 16408:
    g_prot = TRUE;
    if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_ROGUE))
      block = TRUE;
    break;
  case 16283:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_SHAMAN))
      block = TRUE;
    break;
  case 16383:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_SHAMAN))
      block = TRUE;
    break;
  case 16007:
    g_prot = TRUE;
    if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_WARRIOR))
      block = TRUE;
    break;
  case 16145:
    g_prot = TRUE;
    if ((cmd == CMD_EAST) && !GET_CLASS(pl, CLASS_SORCERER))
      block = TRUE;
    break;
  case 17086:
    g_prot = TRUE;
    if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_MERCENARY))
      block = TRUE;
    break;
  case 17564:
    g_prot = TRUE;
    if ((cmd == CMD_EAST) && !GET_SPEC(pl, CLASS_ROGUE, SPEC_ASSASSIN))
      block = TRUE;
    break;
  case 25001:
  case 25086:
  case 25201:
  case 19859:
  case 4128:
    if (cmd == CMD_NORTH)
      block = TRUE;
    break;
  case 8044:
  case 8046:
  case 11685:
    if (cmd == CMD_EAST)
      block = TRUE;
    break;
  case 8087:
    if (((cmd == CMD_EAST) && (GET_RACE(pl) != RACE_GREY) &&
         (GET_RACE(pl) != RACE_HALFELF && (GET_RACE(pl) != RACE_CENTAUR))))
      block = TRUE;
    break;
  case 45017:
    if (((cmd == CMD_NORTH) && (GET_RACE(pl) != RACE_GREY) &&
         (GET_RACE(pl) != RACE_HALFELF && (GET_RACE(pl) != RACE_CENTAUR))))
      block = TRUE;
    break;
  case 11812:
    if (cmd == CMD_UP)
      block = TRUE;
    break;
  case 11008:
  case 11208:
  case 19950:
  case 25320:
  case 25326:
  case 19951:
  case 19954:
    if (cmd == CMD_SOUTH)
      block = TRUE;
    break;
  case 8053:
    if (cmd == CMD_WEST)
      block = TRUE;
    break;
  case 17343:
    if (cmd == CMD_DOWN)
      block = TRUE;
    break;
  case 17345:
    if (cmd == CMD_DOWN)
      block = TRUE;
    break;
  case 17347:
    if (cmd == CMD_DOWN)
      block = TRUE;
    break;

    /*
     * Ashrumite
     */
  case 66065:
    g_prot = TRUE;
    if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_SHAMAN))
      block = TRUE;
    break;
  case 66088:
    g_prot = TRUE;
    if ((cmd == CMD_EAST) && !GET_CLASS(pl, CLASS_CLERIC))
      block = TRUE;
    break;
  case 66028:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_ROGUE))
      block = TRUE;
    break;
  case 66084:
    g_prot = TRUE;
    if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_SORCERER) &&
        !GET_CLASS(pl, CLASS_SUMMONER) && !GET_CLASS(pl, CLASS_CONJURER))
      block = TRUE;
    break;
  case 66078:
    g_prot = TRUE;
    if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_WARRIOR))
      block = TRUE;
    break;
  }                             /*
                                 * end switch
                                 */

  /*
   * code added to allow mobs which are hunting their "home" to pass.
   */

  if (IS_NPC(pl))
  {
    P_nevent  ev;

    LOOP_EVENTS_CH(ev, pl->nevents)
    {
      if (ev->func == mob_hunt_event)
      {
        break;
      }
    }
    if ((ev) &&
        ((hunt_data*)(ev->data))->hunt_type == HUNT_ROOM &&
        ((hunt_data*)(ev->data))->targ.room == real_room(pl->player.birthplace))
      block = FALSE;
  }
  if (g_prot && IS_FIGHTING(ch) && (cmd == 0))
  {
    if (guild_protection(ch, ch->specials.fighting))
      return (TRUE);
  }
  if (block)
  {
    if (!IS_TRUSTED(pl))
    {
      act("$N humiliates you, and blocks your way.",
          FALSE, pl, 0, ch, TO_CHAR);
      act("$N humiliates $n, and blocks $s way.",
          FALSE, pl, 0, ch, TO_NOTVICT);
    }
    else
    {
      sprintf(Gbuf1, "$N bows before you, saying 'Right this way, My %s'",
              (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
      act(Gbuf1, FALSE, pl, 0, ch, TO_CHAR);
      sprintf(Gbuf1, "$N bows before $n, saying 'Right this way, My %s'",
              (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
      act(Gbuf1, FALSE, pl, 0, ch, TO_NOTVICT);
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

/*
 * this is a copy of guild_guard proc with a nasty twist.  Each PULSE_MOBILE
 * the guardian check the room it's supposed to be guarding, if there are
 * people in the room, it will attack them.  This is for use by mobs guarding
 * vaults and such.  And they will attack ANYONE in the room (including other
 * mobs) (excluding only gods).  They also block, like normal guild_guards.
 * -JAB
 */

int guardian(P_char ch, P_char pl, int cmd, char *arg)
{
  int      block_dir = 0, i;
  P_char   t_ch;
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return FALSE;

  if ((ch->in_room != real_room(GET_HOME(ch))) || (ch->in_room == NOWHERE))
  {
    /*
     * don't let them get out of position, unless charmed, which shouldn't
     * happen with these guys anyway.
     */
    if (!IS_AWAKE(ch) || IS_FIGHTING(ch))
      return FALSE;

    act("$N looks around frantically, then vanishes in a small puff of smoke",
        FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(GET_HOME(ch)), -1);

    return FALSE;
  }
  /*
   * ok, for ease of addition, add the virtual room number that the guard loads
   *
   * in as a case to the following switch.  After the case, set attack = TRUE
   * if this guard is supposed to use the guild_protector special (blast them,
   * outlaw them, teleport them away).  Check class restrictions and set block
   * = TRUE if the guard should block 'pl'. Note that guild guard's home is set
   *
   * to the room they load in, and this prevents them from doing ANYTHING
   * special, when they aren't in that room. Charmed, fled, transed, etc.  JAB
   */

  switch (world[ch->in_room].number)
  {
  case 8044:
    block_dir = 2;
    break;
  case 8046:
    block_dir = 2;
    break;
  case 8053:
    block_dir = 4;
    break;
  case 11008:
    block_dir = 3;
    break;
  case 11208:
    block_dir = 3;
    break;
  case 25001:
    block_dir = 1;
    break;
  case 25086:
    block_dir = 1;
    break;
  case 25201:
    block_dir = 1;
    break;
  case 97126:
  case 97242:
    block_dir = 3;
    break;
  }

  if (pl && cmd)
  {
    if (cmd == block_dir)
    {
      if (!IS_TRUSTED(pl))
      {
        act("$N humiliates you, and block your way.",
            FALSE, pl, 0, ch, TO_CHAR);
        act("$N humiliates $n, and blocks $s way.",
            FALSE, pl, 0, ch, TO_NOTVICT);
      }
      else
      {
        sprintf(Gbuf1, "$N bows before you, saying 'Right this way, My %s'",
                (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
        act(Gbuf1, FALSE, pl, 0, ch, TO_CHAR);
        sprintf(Gbuf1, "$N bows before $n, saying 'Right this way, My %s'",
                (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
        act(Gbuf1, FALSE, pl, 0, ch, TO_ROOM);
        return FALSE;
      }
      return TRUE;
    }
    return FALSE;
  }
  else if (pl && !cmd)
  {
    /*
     * the twist part
     */
    if (IS_FIGHTING(ch) || (block_dir < 1))
      return FALSE;

/*    block_dir--;*/
    block_dir = cmd_to_exitnumb(block_dir);

    if (!EXIT(ch, block_dir) || (EXIT(ch, block_dir)->to_room == NOWHERE))
    {
      logit(LOG_MOB, "bogus room to guard in guardian() for %s, in %d (%s)",
            ch->player.short_descr, world[ch->in_room].number,
            command[exitnumb_to_cmd(block_dir)]);
      REMOVE_BIT(ch->specials.act, ACT_SPEC);
      return FALSE;
    }
    i = EXIT(ch, block_dir)->to_room;
    t_ch = world[i].people;

    if (!t_ch)
      return FALSE;

    act("$n snarls angrily, and vanishes in a puff of smoke!",
        FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, i, -1);
    act("Snarling in rage, $n appears and attacks!", FALSE, ch, 0, 0,
        TO_ROOM);
    MobStartFight(ch, t_ch);
    return TRUE;

  }
  else
  {
    logit(LOG_MOB,
          "%s guardian special called in room %d with cmd and no target",
          ch->player.short_descr, world[ch->in_room].number);
  }
  return FALSE;
}

int devour(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    i, temp, next_obj;

  // Check for periodic event calls
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if( cmd != CMD_PERIODIC || !IS_AWAKE(ch) )
    return FALSE;

  for (i = world[ch->in_room].contents; i; i = i->next_content)
  {
    if ((GET_ITEM_TYPE(i) == ITEM_FOOD) || (GET_ITEM_TYPE(i) == ITEM_CORPSE))
    {
      if (GET_ITEM_TYPE(i) == ITEM_CORPSE)
        for (temp = i->contains; temp; temp = next_obj)
        {
          next_obj = temp->next_content;
          obj_from_obj(temp);
          obj_to_room(temp, ch->in_room);
        }
      if (IS_SET(i->value[1], PC_CORPSE))
      {
        logit(LOG_CORPSE, "%s devoured in room %d.", i->short_description, world[i->loc.room].number);
      }
      act("$n savagely devours $o.", FALSE, ch, i, 0, TO_ROOM);
      extract_obj(i, TRUE); // Just food/empty corpse, but 'in game.'
      return (TRUE);
    }
  }
  return (FALSE);
}

void event_tentacles(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_char   tch;

  for (tch = world[ch->in_room].people; tch; tch = tch->next)
    if ((tch->specials.fighting == ch) && (tch->points.damnodice == 4) &&
        (tch->points.damsizedice == 4) && (tch->specials.alignment == -200))
      break;

  if (!tch)
    REMOVE_BIT(ch->specials.affected_by2, AFF2_MAJOR_PARALYSIS);
}

int tentacle(P_char ch, P_char pl, int cmd, char *arg)
{
  int      dam = cmd / 1000;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  if (!dam || dam < 1)
    return FALSE;
  if (NewSaves(pl, SAVING_SPELL, 0) && (GET_POS(pl) == POS_STANDING))
  {
    REMOVE_BIT(ch->specials.affected_by2, AFF2_MAJOR_PARALYSIS);
    act("$N breaks free of $n's grip and quickly eliminates $m!", TRUE, ch, 0,
        pl, TO_NOTVICT);
    act("You finally manage to get free of $n's grip and kick $s ass!", TRUE,
        pl, 0, 0, TO_CHAR);
    die(ch, pl);
    return TRUE;
  }
  else
  {
    add_event(event_tentacles, PULSE_VIOLENCE, pl, 0, 0, 0, 0, 0);
  }
  return FALSE;
}

int charon(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, next_tch;
  int      to_room;
  P_obj    ship;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (cmd == CMD_ENTER)
  {
    arg = skip_spaces(arg);
    if (!strcmp(arg, "galleon") || !strcmp(arg, "spectral"))
    {
      act
        ("&+LA black haze surrounds you... when it clears, you are elsewhere!",
         FALSE, pl, 0, 0, TO_CHAR);
      act("&+LA black haze surrounds $n&+L... when it clears, $e is gone!",
          FALSE, pl, 0, 0, TO_ROOM);
      char_from_room(pl);
      char_to_room(pl, real_room0(1299), 0);
      return TRUE;
    }
    return FALSE;
  }
  if (IS_FIGHTING(ch))
  {
    /* Open a SERIOUS can o' whoopass! */
    act
      ("&+W$n&+W's jaw gapes as &+Lblackness&+W pours out of his eyes and mouth.&n",
       FALSE, ch, 0, 0, TO_ROOM);
    act
      ("&+W$n &n&+cheaves its mighty blade through the air and brings it's wrath unto the puny beings nearby...&n",
       FALSE, ch, 0, 0, TO_ROOM);
    for (tch = world[ch->in_room].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;
      if (ch->specials.fighting == tch ||
          (IS_PC(tch) && !number(0, 5) && !IS_TRUSTED(tch)))
      {
        act("$n&+w's mighty blade cuts $N clean in half!!", FALSE, ch, 0, tch,
            TO_NOTVICT);
        act("$n&+w's mighty blade cuts YOU clean in half!!", FALSE, ch, 0,
            tch, TO_VICT);
        die(tch, ch);
      }
    }
  }
  else
  {
    if (world[ch->in_room].number != 1299)
    {
      ship = get_obj_in_list_vis(ch, "galleon", world[ch->in_room].contents);
      if (!ship || !(to_room = real_room0(1299)))
        return FALSE;
      if (ship->timer[1] == 1)
      {
        act("$n boards $p.", FALSE, ch, ship, 0, TO_ROOM);
        char_from_room(ch);
        char_to_room(ch, to_room, 0);
        act("$n climbs aboard.", FALSE, ch, 0, 0, TO_ROOM);
      }
    }
  }
  return FALSE;
}

int shadow_demon_of_torm(P_char ch, P_char pl, int cmd, char *arg)
{
#if 0
  int      dam = cmd / 1000;

#endif

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || pl)
    return FALSE;

  if (cmd != -1)
  {
    if (affected_by_spell(ch, SPELL_SUMMON))
      return FALSE;
    act("Summoning magic dispersed, $n disappears into the shadows..",
        TRUE, ch, 0, 0, TO_ROOM);
    act("Suddenly shadows seem to cover a lot more of the room than before..",
        TRUE, ch, 0, 0, TO_ROOM);
    spell_darkness(20, ch, 0, 0, 0, 0);
    extract_char(ch);
    ch = NULL;
    return TRUE;
  }
  act("As $n dies, it melts into the shadows of the room.",
      TRUE, ch, 0, 0, TO_ROOM);
  act("Suddenly shadows seem to cover a lot more of the room than before..",
      TRUE, 0, 0, 0, TO_ROOM);
  spell_darkness(20, ch, 0, 0, 0, 0);
  return TRUE;
}

int dryad(P_char ch, P_char pl, int cmd, char *arg)
{
  struct affected_type af;
  P_char   vict, tmp_ch, next_vict_ch, next_tmp_ch;
  int      InRoom, HasCharmies;
  bool     princess = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  Gbuf4[0] = 0;
  /*
   * SAM 7-94 add check to see if pl exists
   */
  if ((pl) && GET_MASTER(pl) == ch)
    switch (cmd)
    {
    case CMD_SCORE:
    case CMD_TELL:
    case CMD_SHOUT:
    case CMD_LOOK:
    case CMD_HELP:
    case CMD_WHO:
    case CMD_WEATHER:
    case CMD_SAVE:
    case CMD_QUIT:
    case CMD_TIME:
    case CMD_TOGGLE:
    case CMD_CHANNEL:
    case CMD_GCC:
    case CMD_COMMANDS:
    case CMD_ATTRIBUTES:
    case CMD_PETITION:
      break;
    default:
      send_to_char
        ("Your thoughts are too hazy, soley focused on this lovely forest maiden.\r\n",
         pl);
      send_to_char
        ("You can do nothing but stand here and tend to her every whim..\r\n",
         pl);
      return (TRUE);
      break;
    }
  if (pl)
    return (0);
  else
  {
    if (GET_VNUM(ch) == 5702)
      princess = TRUE;

    if (IS_FIGHTING(ch))
    {
      for (tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next_tmp_ch)
      {
        next_tmp_ch = tmp_ch->next_in_room;

        if (GET_MASTER(tmp_ch) == ch &&
            MIN_POS(tmp_ch, POS_STANDING + STAT_SLEEPING))
        {
          for (vict = world[ch->in_room].people; vict; vict = next_vict_ch)
          {
            next_vict_ch = vict->next_in_room;

            if (IS_FIGHTING(vict) && (vict != ch) && GET_MASTER(vict) != ch)
            {
              sprintf(Gbuf4, "The dryad screams at %s, 'Protect me slave!'",
                      (IS_NPC(tmp_ch) ? tmp_ch->player.
                       short_descr : GET_NAME(tmp_ch)));
              act(Gbuf4, FALSE, ch, 0, tmp_ch, TO_NOTVICT);
              sprintf(Gbuf4,
                      "The dryad screams at you, 'Protect me slave!'\r\n");
              send_to_char(Gbuf4, tmp_ch);
              sprintf(Gbuf4,
                      "%s dives inbetween %s and the dryad, and takes up the fight!",
                      (IS_NPC(tmp_ch) ? tmp_ch->player.
                       short_descr : GET_NAME(tmp_ch)),
                      (IS_NPC(vict) ? vict->player.
                       short_descr : GET_NAME(vict)));
              act(Gbuf4, FALSE, ch, 0, 0, TO_NOTVICT);
              sprintf(Gbuf4,
                      "You dive inbetween your dryad and %s, taking up the fight!\r\n",
                      (IS_NPC(vict) ? vict->player.
                       short_descr : GET_NAME(vict)));
              send_to_char(Gbuf4, tmp_ch);
              stop_fighting(vict);
#ifndef NEW_COMBAT
              hit(tmp_ch, vict, tmp_ch->equipment[PRIMARY_WEAPON]);
#else
              hit(tmp_ch, vict, tmp_ch->equipment[WIELD], TYPE_UNDEFINED,
                  getBodyTarget(tmp_ch), TRUE, FALSE);
#endif
              return (TRUE);
            }
          }
        }
      }
/*
      LOOP_THRU_PEOPLE(tmp_ch, ch) {
*/
      for (tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next_tmp_ch)
      {
        next_tmp_ch = tmp_ch->next_in_room;

        if ((tmp_ch != ch) && IS_FIGHTING(tmp_ch) && GET_MASTER(tmp_ch) != ch)
        {
          if (GET_SEX(tmp_ch) == SEX_MALE)
          {
            if (!NewSaves(tmp_ch, SAVING_SPELL, princess ? 15 : 10))
            {
              act
                ("The dryad utters an arcane phrase and throws her hands outwards.\r\n"
                 "The dryad's powerful spell springs forth and strikes you in the chest!\r\n"
                 "You feel yourself falling deep into a powerful trance, focused on the dryad.\r\n"
                 "The charm sets upon you fully... You feel completely entranced by her, \r\n"
                 "totally in love with her. You feel is if you would do Anything for her..\r\n"
                 "For you, this lovely dryad has now become the center of the universe... ",
                 FALSE, tmp_ch, 0, ch, TO_CHAR);
              act
                ("The dryad utters a arcane phrase and throws her hands towards $N.\r\n"
                 "The dryad's powerful spell springs forth and strikes $N in the chest!\r\n"
                 "$N's eyes go blank as $E falls victim to the dryad's powerful charm... ",
                 FALSE, ch, 0, tmp_ch, TO_NOTVICT);
              stop_fighting(tmp_ch);
              stop_fighting(ch);
              if (tmp_ch->following)
                stop_follower(tmp_ch);
              add_follower(tmp_ch, ch);
              setup_pet(tmp_ch, ch, 24 * 18 * (princess ? 2 : 1), 0);
              if (princess)
                sprintf(Gbuf4,
                        "An exceptionally beautiful dryad princess is here, tending to her slaves..\r\n");
              else
                sprintf(Gbuf4,
                        "A beautiful dryad is standing here, tending to her slaves..\r\n");

              if ((ch->only.npc->str_mask & STRUNG_DESC1) &&
                  ch->player.long_descr)
              {
                FREE(ch->player.long_descr);
              }
              ch->only.npc->str_mask |= STRUNG_DESC1;
              ch->player.long_descr = (char *) str_dup(Gbuf4);
              if (IS_NPC(tmp_ch))
              {
                sprintf(Gbuf4,
                        "%s is standing here with a totally blank expression.\r\n",
                        tmp_ch->player.short_descr);
                if ((tmp_ch->only.npc->str_mask & STRUNG_DESC1) &&
                    tmp_ch->player.long_descr)
                {
                  FREE(tmp_ch->player.long_descr);
                }
                tmp_ch->only.npc->str_mask |= STRUNG_DESC1;
                tmp_ch->player.long_descr = (char *) str_dup(Gbuf4);
              }
              return (TRUE);
            }
            else
            {
              act
                ("The dryad utters an arcane phrase and throws her hands outwards.\r\n"
                 "The dryad's powerful spell springs forth and strikes you in the chest!\r\n"
                 "You go blank for an instant, but resist the dryad's powerful charm.",
                 FALSE, ch, 0, tmp_ch, TO_CHAR);
              act
                ("The dryad utters a arcane phrase and throws her hands towards $N.\r\n"
                 "The dryad's powerful spell springs forth and strikes $N in the chest!\r\n"
                 "$N goes black for an instant, but resists the dryad's powerful charm.",
                 FALSE, ch, 0, tmp_ch, TO_NOTVICT);
              return (FALSE);
            }
          }
        }
      }
    }
    else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      HasCharmies = 0;
/*      LOOP_THRU_PEOPLE(tmp_ch, ch) {*/
      for (tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next_tmp_ch)
      {
        next_tmp_ch = tmp_ch->next_in_room;

        if (GET_MASTER(tmp_ch) == ch)
        {
          HasCharmies = 1;
          if (number(0, 1) == 0)
          {
            strcpy(Gbuf4, "dryad");
            switch (dice(2, 17))
            {
            case 2:
              mobsay(ch, "Groom my hair, slave..");
              do_action(tmp_ch, Gbuf4, CMD_COMB);
              break;
            case 3:
              mobsay(ch, "Massage me my prince, I desire it...");
              do_action(tmp_ch, Gbuf4, CMD_MASSAGE);
              break;
            case 4:
              mobsay(ch, "Grovel to me my slave, show your subservience!");
              do_action(tmp_ch, Gbuf4, CMD_GROVEL);
              break;
            case 5:
              act("$n lets out a small provacative moan at you..", TRUE, ch,
                  0, 0, TO_ROOM);
              do_action(tmp_ch, Gbuf4, CMD_UNDRESS);
              break;
            case 6:
              mobsay(ch, "Mmmmmmmm my body feels so tense!");
              do_action(tmp_ch, Gbuf4, CMD_CARESS);
              break;
            case 7:
              do_action(ch, 0, CMD_PUCKER);
              do_action(tmp_ch, Gbuf4, CMD_FRENCH);
              break;
            case 8:
              mobsay(ch, "Do you love me, my slave?");
              do_action(tmp_ch, Gbuf4, CMD_LOVE);
              break;
            case 9:
              mobsay(ch, "Do you wish to stay with me forever my slave?");
              mobsay(tmp_ch,
                     "Yes! Forever! I love you my beautiful princess!");
              do_action(tmp_ch, Gbuf4, CMD_DREAM);
              strcpy(Gbuf4, tmp_ch->player.name);
              do_action(ch, Gbuf4, CMD_KISS);
              break;
            case 10:
              mobsay(ch, "I think that soon we will make love, my slave..");
              do_action(tmp_ch, Gbuf4, CMD_SEDUCE);
              break;
            case 11:
              mobsay(ch, "Bathe me, my slave, I wish to feel clean..");
              do_action(tmp_ch, Gbuf4, CMD_BATHE);
              break;
            case 12:
              mobsay(ch, "Do you find me attractive?");
              mobsay(tmp_ch,
                     "Yes! You are the most beautiful woman I have ever seen!");
              do_action(tmp_ch, Gbuf4, CMD_UNDRESS);
              break;
            case 13:
              act("The dryads beauty nearly overwhealms you with passion!",
                  TRUE, ch, 0, 0, TO_ROOM);
              do_action(tmp_ch, Gbuf4, CMD_MELT);
              break;
            case 14:
              mobsay(ch,
                     "You must please me slave, or I shall cast you out!");
              do_action(tmp_ch, 0, CMD_SULK);
              do_action(tmp_ch, 0, CMD_CRY);
              break;
            case 15:
              mobsay(ch, "Prove your love to me slave!");
              do_action(tmp_ch, Gbuf4, CMD_OGLE);
              do_action(tmp_ch, Gbuf4, CMD_EMBRACE);
              do_action(tmp_ch, 0, CMD_WHIMPER);
              strcpy(Gbuf4, tmp_ch->player.name);
              do_action(ch, Gbuf4, CMD_FLUTTER);
              break;
            case 16:
              do_action(tmp_ch, Gbuf4, CMD_ROSE);
              strcpy(Gbuf4, tmp_ch->player.name);
              do_action(ch, Gbuf4, CMD_PAT);
              break;
            case 17:
              mobsay(ch,
                     "Hmmmmm, I prefer my slaves to remain naked.. Disrobe for me.");
              do_action(ch, 0, CMD_GRIN);
              strcpy(Gbuf4, "all");
              do_remove(tmp_ch, Gbuf4, 0);
              strcpy(Gbuf4, "dryad");
              do_action(tmp_ch, Gbuf4, CMD_SEDUCE);
              break;
            default:
              break;
            }
          }
        }
      }
      if (HasCharmies == 1)
      {
        InRoom = world[ch->in_room].number;
        if ((princess && (InRoom != 5744)) ||
            (!princess && ((InRoom < 5733) || (InRoom > 5744))))
        {
          mobsay(ch, "Come my slave, let us go to my hidden abode.");
          act("The dryad utters an arcane magical phrase.", TRUE, ch, 0, 0,
              TO_ROOM);
          act("$n disappears in a blinding flash of light!", FALSE, ch, 0,
              tmp_ch, TO_NOTVICT);
          act
            ("FOOOOOOOOOOSH! With a flash of light, you are instantly teleported!",
             TRUE, ch, 0, 0, TO_ROOM);
          sprintf(Gbuf4,
                  "A beautiful dryad slowly fades into exsistance!\r\n");
          LOOP_THRU_PEOPLE(tmp_ch, ch) if (GET_MASTER(tmp_ch) == ch)
            sprintf(Gbuf1,
                    "%s slowly fades into existance, standing obediently behind the dryad.\r\n",
                    (IS_NPC(tmp_ch) ?
                     tmp_ch->player.short_descr : GET_NAME(tmp_ch)));
          strcat(Gbuf4, Gbuf1);
          send_to_room(Gbuf4, princess ? real_room(5744) : real_room(5739));
/*          LOOP_THRU_PEOPLE(tmp_ch, ch) {*/
          for (tmp_ch = world[ch->in_room].people; tmp_ch;
               tmp_ch = next_tmp_ch)
          {
            next_tmp_ch = tmp_ch->next_in_room;

            if (GET_MASTER(tmp_ch) == ch)
            {
              act("$N disappears in a blinding flash of light!", FALSE,
                  ch, 0, tmp_ch, TO_NOTVICT);
              char_from_room(tmp_ch);
              char_to_room(tmp_ch,
                           princess ? real_room(5744) : real_room(5739), -1);
            }
          }
          char_from_room(ch);
          char_to_room(ch, princess ? real_room(5744) : real_room(5739), -1);
        }
      }
      else
      {
        if (princess)
          sprintf(Gbuf4,
                  "An exceptionally beautiful dryad princess is here, observing you quietly.\r\n");
        else
          sprintf(Gbuf4,
                  "A beautiful dryad is standing here, observing you shyly.\r\n");
        if ((ch->only.npc->str_mask & STRUNG_DESC1) && ch->player.long_descr)
        {
          FREE(ch->player.long_descr);
        }
        ch->only.npc->str_mask |= STRUNG_DESC1;
        ch->player.long_descr = (char *) str_dup(Gbuf4);
        switch (dice(3, 8))
        {
        case 3:
          act("$n looks at you, both shy and nervous.", TRUE, ch, 0, 0,
              TO_ROOM);
          break;
        case 4:
          mobsay(ch, "May you go in peace throgh our forest...");
          break;
        case 5:
          act("$n sings a beautiful song that is filled with soft tones.",
              TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 6:
          act("$n smiles at you.", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 7:
          mobsay(ch, "Welcome traveller.");
          act("$n smiles.", TRUE, ch, 0, 0, TO_ROOM);
        case 8:
          act("$n keeps a wary eye on you.", TRUE, ch, 0, 0, TO_ROOM);
          break;
        default:
          break;
        }
      }
    }
  }
  return (FALSE);
}

struct ticket_info_data
{
  int      in_room;
  int      item_id;
  int      ship_id;
} ticket_info[] =
{

  {
    5313, 5341, 11100           /*
                                 * WD, to Caer Corwell, Realms Master
                                 */
  }
  ,
  {
    5399, 5341, 11300           /*
                                 * WD, to Caer Corwell, Silver Lady
                                 */
  }
  ,
  {
    26200, 26240, 11100         /*
                                 * Port of Caer Corwell, to WD, Realms Master
                                 */
  }
  ,
  {
    26200, 26240, 11300         /*
                                 * Port of Caer Corwell, to WD, Silver Lady
                                 */
  }
  ,
  {
  0, 0, 0}
};

int ticket_taker(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    obj;
  char     name[MAX_INPUT_LENGTH];
  bool     no_ticket, found, i;
  P_obj    obj_entered;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if ((cmd != CMD_ENTER) || !ch || !pl)
    return FALSE;
  if (!IS_AWAKE(ch))
    return FALSE;
  if (!CAN_SEE(ch, pl))
    return FALSE;

  /*
   * check for ticket on char.  If ticket not on char, then check for ticket
   * given to mob.
   */
  one_argument(arg, name);
  obj_entered = get_obj_in_list_vis(ch, name, world[ch->in_room].contents);
  if (!obj_entered)
    return FALSE;
  found = 0;
  no_ticket = 1;
  for (i = 0; ticket_info[(int) i].in_room != 0; i++)
  {
    if ((ch->in_room == real_room(ticket_info[(int) i].in_room)) &&
        (obj_entered->R_num == real_object(ticket_info[(int) i].ship_id)))
    {

      no_ticket = 0;

      for (obj = pl->carrying; obj; obj = obj->next_content)
      {
        if (obj_index[obj->R_num].virtual_number ==
            ticket_info[(int) i].item_id)
        {
          found = 1;
          act("$N tears up the ticket in your hand.", FALSE, pl, 0, ch, TO_CHAR);
          act("$N tears up the ticket in $n's hand.", FALSE, pl, 0, ch, TO_ROOM);
          obj_from_char(obj);
          extract_obj(obj, TRUE); // Not an arti, but 'in game.'
          obj = NULL;
          break;
        }
      }
      if (!found)
      {
        for (obj = ch->carrying; obj; obj = obj->next_content)
        {
          if (obj_index[obj->R_num].virtual_number ==
              ticket_info[(int) i].item_id)
          {
            found = 1;
            act("$n tears up the ticket.", FALSE, ch, 0, 0, TO_ROOM);
            obj_from_char(obj);
            extract_obj(obj, TRUE); // Not an arti, but 'in game.'
            break;
          }
        }
      }
      if (found)
      {
        act("Then $E says to you, 'You may proceed.'", FALSE, pl, 0, ch,
            TO_CHAR);
        act("Then $E says to $n, 'You may proceed.'", FALSE, pl, 0, ch,
            TO_ROOM);
        return FALSE;
      }
    }
  }
  if (!no_ticket)
  {
    act("$N says, 'you must have a ticket to proceed.'", FALSE, pl, 0, ch,
        TO_CHAR);
    return TRUE;
  }
  if (!found)                   /*
                                 * this ship is not restricted from being
                                 * boarded
                                 */
    return FALSE;
  return TRUE;
}

/*
 * code to allow mob to sail ship cannot be defined in spec proc because
 * mobile spec proc is called only once every 40 pulses.  Mobs need to
 * order ship to sailin 10 pulses interval. -DCL
 */

int navagator(P_char ch, P_char pl, int cmd, char *arg)
{
  int      realms_helpers[] = { 11102, 11103, 11104, 11105, 11107, 11108,
    11110, 11112, 11114, 0
  };
  int      silver_helpers[] = { 11102, 11103, 11104, 11105, 11107, 11108,
    11110, 11112, 11114, 0
  };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  /*
   * check for shout_and_hunt() stuff on ship navigators
   */

  if (!pl)
  {
    if (GET_VNUM(ch) == 11101)      /*
                                                         * realms master
                                                         */
      return shout_and_hunt(ch, 30,
                            "Help me mates!  We be under attack by %s!",
                            NULL, realms_helpers, 0, 0);
    if (GET_VNUM(ch) == 11301)      /*
                                                         * silver lady
                                                         */
      return shout_and_hunt(ch, 30,
                            "Help me mates!  We be under attack by %s!",
                            NULL, silver_helpers, 0, 0);
    return FALSE;
  }
  if ((cmd != CMD_ORDER) || (ch == pl))
    return FALSE;

  act("$n growls at $N, 'only I can order my ship to sail!'", FALSE, ch, 0,
      pl, TO_NOTVICT);
  act("$n growls at you, 'only I can order my ship to sail!'", FALSE, ch, 0,
      pl, TO_VICT);
  return TRUE;
}

/*
 *    Fun procs - SAM 6-94
 */

int billthecat(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd != 0)                 /*
                                 * mobact.c
                                 */
    return (FALSE);

  switch (number(0, 10))
  {
  case 0:
    mobsay(ch, "pffffpht!");
    return (TRUE);

  case 1:
    do_action(ch, 0, CMD_TRIP);
    return (TRUE);

  case 2:
    mobsay(ch, "ACK!");
    return (TRUE);

  case 3:
    do_action(ch, 0, CMD_BANG);
    return (TRUE);

  case 4:
    do_action(ch, 0, CMD_MOSH);
    return (TRUE);

  case 5:
    act("$n hocks up a furball.", TRUE, ch, 0, 0, TO_ROOM);
    return (TRUE);

  case 6:
    do_action(ch, 0, CMD_MOAN);
    return (TRUE);

  case 7:
    do_action(ch, 0, CMD_SLOBBER);
    return (TRUE);

  default:
    return (FALSE);
  }
}

int beavis(P_char ch, P_char pl, int cmd, char *arg)
{
  char     buf[255];

  /* check for periodic event calls  */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd != 0)                 /* mobact.c  */
    return (FALSE);

  strcpy(buf, "butthead");

  switch (number(0, 40))
  {
  case 0:
    mobsay(ch, "Heh hehe that sucks dude!");
    return (TRUE);
  case 1:
    mobsay(ch, "Dude, this is like cool!");
    return (TRUE);
  case 2:
    mobsay(ch, "Heh, whoa dude that was cool!");
    return (TRUE);
  case 3:
    mobsay(ch, "I think this is cool or something.");
    return (TRUE);
  case 4:
    mobsay(ch, "Look at those chicks, huh huh huh");
    return (TRUE);
  case 5:
    mobsay(ch, "huh huh huh huh huh huh huh huh huh");
    return (TRUE);
  case 6:
    mobsay(ch, "We're there dude!");
    return (TRUE);
  case 7:
    mobsay(ch, "This video SUCKS!");
    return (TRUE);
  case 8:
    mobsay(ch, "Shutup asswipe!");
    return (TRUE);
  case 9:
    mobsay(ch, "Metallica kicks ass!");
    return (TRUE);
  case 10:
    mobsay(ch, "White Zombie RULES!");
    return (TRUE);
  case 11:
    mobsay(ch, "Mmmmm tastes like chicken.");
    return (TRUE);
  case 12:
    mobsay(ch, "Fire fire fire fire fire!");
    return (TRUE);
  case 13:
    mobsay(ch, "Shutup ButtHead, I'll kick your ass!");
    return (TRUE);
  case 15:
    do_action(ch, 0, CMD_BANG);
    return (TRUE);
  case 16:
    do_action(ch, 0, CMD_MOSH);
    return (TRUE);
  case 17:
    do_action(ch, buf, CMD_BANG);
    return (TRUE);
  case 18:
    mobsay(ch, "Change it or kill me, Butthead!");
    return (TRUE);
  case 19:
    mobsay(ch, "Isn't this new band, Schlong?");
    return (TRUE);
  case 20:
    mobsay(ch, "Nachos rule!  They rule!");
    return (TRUE);
  default:
    return (FALSE);
  }
}

int butthead(P_char ch, P_char pl, int cmd, char *arg)
{
  char     buf[20];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd != 0)                 /*
                                 * mobact.c
                                 */
    return (FALSE);

  switch (number(0, 40))
  {
  case 0:
    mobsay(ch, "Heh hehe that sucks dude!");
    return (TRUE);
  case 1:
    mobsay(ch, "Dude, this is like cool!");
    return (TRUE);
  case 2:
    mobsay(ch, "Heh, whoa dude that was cool!");
    return (TRUE);
  case 3:
    mobsay(ch, "I think this is cool or something.");
    return (TRUE);
  case 4:
    mobsay(ch, "Look at those chicks, huh huh huh");
    return (TRUE);
  case 5:
    mobsay(ch, "huh huh huh huh huh huh huh huh huh");
    return (TRUE);
  case 6:
    mobsay(ch, "We're there dude!");
    return (TRUE);
  case 7:
    mobsay(ch, "This video SUCKS!");
    return (TRUE);
  case 8:
    mobsay(ch, "Shutup asswipe!");
    return (TRUE);
  case 9:
    mobsay(ch, "Metallica kicks ass!");
    return (TRUE);
  case 10:
    mobsay(ch, "White Zombie RULES!");
    return (TRUE);
  case 13:
    mobsay(ch, "Settle down Beavis.");
    return (TRUE);
  case 14:
    mobsay(ch, "I'll kick your ass!");
    return (TRUE);
  case 15:
    do_action(ch, 0, CMD_BANG);
    return (TRUE);
  case 16:
    do_action(ch, 0, CMD_MOSH);
    return (TRUE);
  case 17:
    strcpy(buf, "beavis");
    do_action(ch, buf, CMD_BANG);
    return (TRUE);
  case 18:
    mobsay(ch, "Shutup butt munch!");
    return (TRUE);
  case 19:
    mobsay(ch, "Shutup dillhole!");
    return (TRUE);
  case 20:
    mobsay(ch, "Don't bogart my log Beavis!");
    return (TRUE);
  case 21:
    mobsay(ch, "No, thats Prong");
    return (TRUE);
  case 22:
    mobsay(ch, "Hey Beavis, we're cool huh.");
    return (TRUE);
  case 23:
    mobsay(ch, "Nachos rule!  They rule!");
    return (TRUE);
  case 24:
    mobsay(ch, "Nudi... n u i d i s... heh nude people.");
    return (TRUE);

  default:
    return (FALSE);
  }
}

/*
 * The following pair of procs (xexos and agthrodos) are for the "same"
 * * monster, Xexos.  If in battle, Xexos uses the power of "alchemy" to
 * * transform himself into a monstrous agthrodos.  All inventory and equipment
 * * is transferred to its proper position.  When the agthrodos stops fighting,
 * * it will morph back, with all inventory and equipment in proper place.
 */

P_obj has_moonstone_fragment(P_char ch);
extern char arg1[MAX_STRING_LENGTH];
extern char arg2[MAX_STRING_LENGTH];
int xexos(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tempchar = NULL, was_fighting = NULL;
  P_obj    item, next_item;
  int      pos;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch))
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    was_fighting = ch->specials.fighting;

    /*
     * load agthrodos
     */
    tempchar = read_mobile(12026, VIRTUAL);

    if (!tempchar)
    {
      logit(LOG_EXIT, "assert: mob load failed in xexos()");
      raise(SIGSEGV);
    }
    /*
     * stick agthrodos in same room
     */
    char_to_room(tempchar, ch->in_room, -2);

    /*
     * transfer inventory to agthrodos
     */
    for (item = ch->carrying; item; item = next_item)
    {
      next_item = item->next_content;
      obj_from_char(item);
      obj_to_char(item, tempchar);
    }

    /*
     * transfer equipment to agthrodos
     */
    for (pos = 0; pos < MAX_WEAR; pos++)
    {
      if (ch->equipment[pos] != NULL)
      {
        item = unequip_char(ch, pos);
        equip_char(tempchar, item, pos, TRUE);
      }
    }

    /*
     * let the player know what's going on
     */
    mobsay(ch, "That was NOT a good idea!");
    act("$n pulls a vial from a hidden pocket and quickly quaffs it.",
        TRUE, ch, 0, 0, TO_ROOM);
    act("Flesh rends and tears, reshaping $n into something monstrous!",
        TRUE, ch, 0, 0, TO_ROOM);

    act("Whatever $n has become roars and charges to attack you!",
        FALSE, ch, 0, ch->specials.fighting, TO_VICT);

    act("Whatever $n has become roars and charges to attack $N!",
        FALSE, ch, 0, ch->specials.fighting, TO_NOTVICT);

    /*
     * remove xexos
     */
    extract_char(ch);
    ch = NULL;

    MobStartFight(tempchar, was_fighting);

    return (TRUE);

  }
  if (cmd == CMD_ASK)
  {
    if (has_moonstone_fragment(pl))
    {
      send_to_char ("Xexos says 'So, you've killed that... thing.\r\n", pl);
      send_to_char ("  Fine, i admit it. I stole the moonstone from that old fool!\r\n", pl);
      send_to_char ("  But as i was escaping Sarmiz bay on a ship, we've come under pirates attack.\r\n", pl);
      send_to_char ("  In the heat of battle, the moonstone has been broken into three pieces!\r\n", pl);
      send_to_char ("  One fragment fell into ocean and i managed to escape to the land with another.\r\n", pl);
      send_to_char ("  Pirates got the third one i guess...'\r\n\r\n", pl);
      send_to_char ("Xexos says 'I've tried to make an automaton with a single fragment, but its incomplete!\r\n", pl);
      send_to_char ("  Damn thing went berserk and i had to lock it down!'\r\n\r\n", pl);
      send_to_char ("Xexos signs 'And now i have to hide in this shithole from Erzul's revenge. So useless...'\r\n", pl);
      return TRUE;
    }
    else
    {
      half_chop(arg, arg1, arg2);
      if (*arg2)
      {
        if (isname(arg2, "automaton automatons erzul moonstone"))
        {
          send_to_char ("Xexos says 'I dont know what are you talking about, get lost!'\r\n", pl);
          return TRUE;
        }
      }
    }
  }

  return (FALSE);
}

int agthrodos(P_char ch, P_char pl, int cmd, char *arg)
{

  P_char   tempchar = NULL;
  P_obj    item, next_item;
  int      pos;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch))
    return FALSE;

  /*
   * if it's something besides a periodic event call, return
   */
  if (cmd != 0)
    return FALSE;

  /*
   * check if agthrodos has stopped fighting
   */
  if (!IS_FIGHTING(ch))
  {

    /*
     * load Xexos
     */
    tempchar = read_mobile(12025, VIRTUAL);

    if (!tempchar)
    {
      logit(LOG_EXIT, "assert: mob load failed in agthrodos()");
      raise(SIGSEGV);
    }
    /*
     * stick Xexos in same room
     */
    char_to_room(tempchar, ch->in_room, 0);

    /*
     * transfer inventory to Xexos
     */
    for (item = ch->carrying; item; item = next_item)
    {
      next_item = item->next_content;
      obj_from_char(item);
      obj_to_char(item, tempchar);
    }

    /*
     * transfer equipment to Xexos
     */
    for (pos = 0; pos < MAX_WEAR; pos++)
    {
      if (ch->equipment[pos] != NULL)
      {
        item = unequip_char(ch, pos);
        equip_char(tempchar, item, pos, TRUE);
      }
    }

    /*
     * let any watchers know what's going on
     */
    act("$n chuffs angrily, and looks around for further threats.",
        TRUE, ch, 0, 0, TO_ROOM);
    act("$n lets out a bellow, then reverts to its normal form, $N.",
        TRUE, ch, 0, tempchar, TO_NOTVICT);

    /*
     * remove agthrodos
     */
    extract_char(ch);
    ch = NULL;

    return (TRUE);

  }
  else
  {
    return FALSE;
  }
}

/*
 * If the automaton is alone in its room and the trapdoor is blocked, unblock
 * * the door, so that more people can come into their deaths...>8^)
 * * -- DTS 2/22/95
 */
int automaton_unblock(P_char ch, P_char pl, int cmd, char *arg)
{
  int      flag = FALSE;
  P_char   i;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || pl || cmd)
    return FALSE;

  if (world[ch->in_room].number != 12159)
    return FALSE;

  for (i = world[ch->in_room].people; i; i = i->next_in_room)
    if (i != ch)
      flag = TRUE;

  if (!flag)
  {
    if (IS_SET
        (world[real_room(12158)].dir_option[DOWN]->exit_info, EX_BLOCKED))
      REMOVE_BIT(world[real_room(12158)].dir_option[DOWN]->exit_info,
                 EX_BLOCKED);
    if (IS_SET(world[real_room(12159)].dir_option[UP]->exit_info, EX_BLOCKED))
      REMOVE_BIT(world[real_room(12159)].dir_option[UP]->exit_info,
                 EX_BLOCKED);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

int menden_figurine_die(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DEATH)
  {
    act
      ("$n dies, crumbling into powder which is quickly swept away by a sudden breeze.",
       TRUE, ch, 0, 0, TO_ROOM);
  }
  return (FALSE);
}

int menden_inv_serv_die(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DEATH)
  {
    act("$n, vanquished, dissolves into ethereal vapors and disappears.",
        FALSE, ch, 0, 0, TO_ROOM);
  }
  return (FALSE);
}

/*
 * This is for the drunken magus in Menden-on-the-Deep, but its style is
 * * lifted from jester(), with my own additions, of course. >8^)
 * *
 * * -- Damon Silver, aka Fleven 1994/07/02
 */

int menden_magus(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 50))
  {
  case 0:
    mobsay(ch, "What the hell wassh that shtupid word again?");
    return TRUE;
  case 1:
    mobsay(ch, "Shnake? No.");
    return TRUE;
  case 2:
    mobsay(ch, "Shlitherer? Nope nope nope.");
    return TRUE;
  case 3:
    mobsay(ch, "Definitely shomething to do with a shea monshter...");
    return TRUE;
  case 4:
    mobsay(ch, "Wench! Bring me another tankard of ale!");
    return TRUE;
  case 5:
    mobsay(ch, "Time to be getting home, maybe.");
    return TRUE;
  case 6:
    act
      ("$n hiccups, and a little bolt of lightning shoots from his fingers, scorching the floor.",
       TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "'Shcuse me.");
    act("$n grins sheepishly.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 7:
    act("$n teeters for a moment, but regains his balance.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 8:
    do_action(ch, 0, CMD_MUTTER);
    return TRUE;
  default:
    return FALSE;
  }
}

int menden_fisherman(P_char ch, P_char pl, int cmd, char *arg)
{
  char     buf[MAX_INPUT_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(1, 80))
  {
  case 1:
    {
      do_action(ch, 0, CMD_BURP);
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "'Course, Kilten's been sayin'...'scuse me.");
      do_action(ch, 0, CMD_COUGH);
      mobsay(ch, "Where was I? Oh.");
      strcpy(buf, "carafe");
      do_action(ch, buf, CMD_SIP);
      mobsay(ch,
             "His damned ship can make it to Verzanan in under two days.");
      do_action(ch, 0, CMD_ROLL);
      return TRUE;
    }
  case 3:
    {
      do_action(ch, 0, CMD_SMIRK);
      return TRUE;
    }
  case 4:
    {
      strcpy(buf, "wench");
      do_action(ch, buf, CMD_PINCH);
      return TRUE;
    }
  case 5:
    {
      mobsay(ch, "I once caught a dragonfish _this_ big!");
      act("$n stretches $s arms wide.", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  case 6:
    {
      mobsay(ch, "Those damned harpies must have eaten half me crew.");
      do_action(ch, 0, CMD_SHIVER);
      return TRUE;
    }
  case 7:
    {
      mobsay(ch, "I was stranded in the Moonshaes for put near a month.");
      return TRUE;
    }
  case 8:
    {
      mobsay(ch,
             "Yeah, me wife's been nagging at me to get her some pearls.");
      return TRUE;
    }
  case 9:
    {
      do_action(ch, 0, CMD_SCRATCH);
      return TRUE;
    }
  case 10:
    {
      mobsay(ch,
             "I've got a couple of brats down south, but I hardly ever see 'em.");
      do_action(ch, 0, CMD_CRY);
      return TRUE;
    }
  case 11:
    {
      mobsay(ch,
             "So I roll a five, another five, and suddenly I'm ahead forty fire-eyes!");
      return TRUE;
    }
  case 12:
    {
      mobsay(ch, "Aaaaah, feels good to be on dry land for a stretch.");
      do_action(ch, 0, CMD_STRETCH);
      do_action(ch, 0, CMD_WINK);
      return TRUE;
    }
  case 13:
    {
      do_action(ch, 0, CMD_HICCUP);
      return TRUE;
    }
  case 14:
    {
      mobsay(ch, "Murkas Magintii?  Yeah, I done heared of him.");
      mobsay(ch, "Wasn't he that guy that got swallowed by a whale?");
      return TRUE;
    }
  case 15:
    {
      strcpy(buf, "wench");
      do_action(ch, buf, CMD_OGLE);
      return TRUE;
    }
  case 16:
    {
      mobsay(ch,
             "Calim harem girls know tricks that'll blow yer jerkin off!");
      do_action(ch, 0, CMD_WINK);
      return TRUE;
    }
  case 17:
    {
      mobsay(ch, "I heard there be a spell lets ye ken any language.");
      do_action(ch, 0, CMD_SHRUG);
      do_action(ch, 0, CMD_PONDER);
      mobsay(ch, "Valkur knows that'd be helpful when I go tradin'!");
      do_action(ch, 0, CMD_CACKLE);
      return TRUE;
    }
  case 18:
    {
      mobsay(ch, "Y'know, me pa was a fisherman too.");
      return TRUE;
    }
  case 19:
    {
      mobsay(ch,
             "Ye'll never be as strong fighting wimpy dragons as if ye work the sea.");
      do_action(ch, 0, CMD_FLEX);
      return TRUE;
    }
  case 20:
    {
      mobsay(ch,
             "Wench, remind me to bring you back some turtle legs from Nhavan Island next trip.");
      strcpy(buf, "wench");
      do_action(ch, buf, CMD_SMILE);
      return TRUE;
    }
  case 21:
    {
      strcpy(buf, "me");
      do_action(ch, buf, CMD_SCRATCH);
      mobsay(ch, "What're ye after, anyway, ye crazy old coot?");
      strcpy(buf, "magus");
      do_action(ch, buf, CMD_POKE);
      return TRUE;
    }
  default:
    return FALSE;
  }
}

int brass_dragon(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if ((cmd > CMD_DOWN) || (cmd < CMD_NORTH))
    return FALSE;

  strcpy(Gbuf4, "The brass dragon humiliates you, and blocks your way.\r\n");
  strcpy(Gbuf2, "The brass dragon humiliates $n, and blocks $s way.");

  if ((ch->in_room == real_room(5065)) && (cmd == CMD_WEST))
  {
    act(Gbuf2, FALSE, pl, 0, 0, TO_ROOM);
    send_to_char(Gbuf4, pl);
    return TRUE;
  }
  return FALSE;
}

int janitor(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    i;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content)
  {
    if (CAN_GET_OBJ(ch, i) && (CAN_CARRY_W(ch) <= GET_OBJ_WEIGHT(i)) &&
        ((i->type == ITEM_DRINKCON) || (i->type == ITEM_TRASH) ||
         (i->type == ITEM_OTHER) || (i->type == ITEM_FOOD) || (i->cost < 20)))
    {

      act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);

      obj_from_room(i);
      obj_to_char(i, ch);
      return (TRUE);
    }
  }
  return (FALSE);
}

int cityguard(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, tar_ch = NULL;
  int      tar_align, a_flag, magnitude;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)))
    return (FALSE);

  a_flag = (IS_GOOD(ch) ? 1001 : IS_EVIL(ch) ? -1001 : 0);
  tar_align = a_flag;
  magnitude = 0;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch->specials.fighting && CAN_SEE(ch, tch) &&
        (IS_PC(tch) || (GET_VNUM(ch) !=
                        GET_VNUM(tch))))
    {
      switch (a_flag)
      {
      case -1001:              /*
                                 * guard is evil, nuke most good
                                 */
        if (GET_ALIGNMENT(tch) > tar_align)
        {
          tar_align = GET_ALIGNMENT(tch);
          tar_ch = tch;
        }
        break;
      case 0:                  /*
                                 * guard is neutral, get most divergent
                                 */
        if (((GET_ALIGNMENT(tch) > 0) &&
             (GET_ALIGNMENT(tch) > magnitude)) ||
            ((GET_ALIGNMENT(tch) < 0) && (GET_ALIGNMENT(tch) < -magnitude)))
        {
          tar_ch = tch;
          magnitude = GET_ALIGNMENT(tch);
          if (GET_ALIGNMENT(tch) < 0)
            magnitude = -magnitude;
        }
        break;
      case 1001:               /*
                                 * guard is good, nuke most evil
                                 */
        if (GET_ALIGNMENT(tch) < tar_align)
        {
          tar_align = GET_ALIGNMENT(tch);
          tar_ch = tch;
        }
        break;
      }
    }
  }

  if (tar_ch)
  {
    switch (a_flag)
    {
    case -1001:
      act("$n screams 'PURGE THE INNOCENT! BANZAI! CHARGE! ARARAGGGHH!'",
          FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 0:
      act("$n screams 'PRESERVE THE BALANCE! BANZAI! CHARGE! ARARAGGGHH!'",
          FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 1001:
      act("$n screams 'PROTECT THE INNOCENT! BANZAI! CHARGE! ARARAGGGHH!'",
          FALSE, ch, 0, 0, TO_ROOM);
      break;
    }
    MobStartFight(ch, tar_ch);
    return (TRUE);
  }
  return OutlawAggro(ch, "$n screams '%s! Fresh blood! Kill!'");
}

/*
 * The salesman, always after a tidy profit and the satisfaction of a sale, is
 * a social individual...Maybe overly so... <grin>
 */

int sales_spec(P_char ch, P_char pl, int cmd, char *arg)
{
  int      i_val = 0;
  P_obj    selling = 0, s_item = 0, obj = 0;
  P_char   c_obj = 0, k, dummy_char, old_follow = 0;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf4[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  /*
   * Find out if we have anything to sell, either in first equipment slot
   */
  /*
   * or something being carried
   */

  if ((selling = ch->equipment[WIELD]))
    i_val = ch->equipment[WIELD]->cost * 2 + 3;
  else if ((selling = ch->carrying))
    i_val = ch->carrying->cost * 2 + 3;
  /*
   * Handle cases where we might care what a player does: buy, steal, etc.
   */
  if (cmd != 0)
  {
    argument_interpreter(arg, Gbuf1, Gbuf2);
    if (*Gbuf1)
      c_obj = get_char_room(Gbuf1, ch->in_room);
    switch (cmd)
    {
    case CMD_KISS:
    case CMD_FONDLE:
    case CMD_GROPE:
    case CMD_LICK:
    case CMD_LOVE:
    case CMD_NIBBLE:
    case CMD_SQUEEZE:
    case CMD_FRENCH:
      /*
       * Actions of questionable intent
       */
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      if (pl->player.sex != 2)
      {
        act("$N slaps you before you can even begin.",
            FALSE, pl, 0, ch, TO_CHAR);
        act("$N slaps $n for his naughty intentions.",
            FALSE, pl, 0, ch, TO_ROOM);
      }
      return TRUE;
      break;
    case CMD_TELL:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      mobsay(ch, "I can talk all day, but only you can buy...");
      return TRUE;
      break;
    case CMD_SMILE:
      /*
       * The salesman suffers from acceptance anxiety :-)
       */
      if (!IS_AWAKE(ch))
        return FALSE;
      do_action(pl, Gbuf1, cmd);
      act("$n smiles too, trying to join in on the fun.",
          TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
      break;
    case CMD_INSULT:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_insult(pl, arg, 0);
      act("$N humbles you with a greater insult.", FALSE, pl, 0, ch, TO_CHAR);
      act("$N snaps back with a greater insult.", FALSE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_WAKE:
      if (c_obj != ch)
        return FALSE;
      if (GET_STAT(ch) != STAT_SLEEPING)
        act("$N is not sleeping.", FALSE, pl, 0, ch, TO_CHAR);
      else
      {
        act
          ("Your gentle nudging awakens $N, who yawns and clears his throat.",
           FALSE, pl, 0, ch, TO_CHAR);
        act("$n nudges $N awake.", FALSE, pl, 0, ch, TO_ROOM);
        SET_POS(ch, POS_STANDING + STAT_NORMAL);
      }
      return TRUE;
      break;
    case CMD_POKE:
      /*
       * An exchange of poking
       */
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      do_action(pl, arg, CMD_POKE);
      act("$N pokes you back.", FALSE, pl, 0, ch, TO_CHAR);
      act("$N pokes $n back.", TRUE, pl, 0, ch, TO_ROOM);
      return TRUE;
      break;
    case CMD_BACKSTAB:
      if ((c_obj != ch) || (!IS_AWAKE(ch)))
        return FALSE;
      act("Oof! $N knocks you down.", FALSE, pl, 0, ch, TO_CHAR);
      act("$n tries to backstab $N who, more alert than $E appears, slams\r\n"
          "$m down on the ground instead!", FALSE, pl, 0, ch, TO_ROOM);
      mobsay(ch, "I've dealt with your kind before!");
      SET_POS(pl, POS_SITTING + GET_STAT(pl));
      return TRUE;
      break;
    case CMD_BUY:
      /*
       * "To buy is to be" is the salesman's motto...
       */
      if ((strlen(Gbuf1) == 0) || (!IS_AWAKE(ch)) || !selling)
        return FALSE;
      if (c_obj == ch)
      {
        mobsay(ch, "I'm not for sale, idiot!");
        return TRUE;
      }
      if (!generic_find(Gbuf1, FIND_OBJ_EQUIP, ch, &dummy_char, &s_item))
        s_item = get_obj_in_list(Gbuf1, selling);
      if (strlen(Gbuf2) == 0)
      {
        if (!get_char_room_vis(pl, "2.salesman"))
        {
          if (s_item == selling)
          {
            if (transact(ch, s_item, pl, i_val))
            {
              if (ch->following)
                stop_follower(ch);
              REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
            }
          }
          else if (s_item)
          {
            sprintf(Gbuf4,
                    "$n grins and says, 'Buy the %s first, and then I'll\r\n"
                    "consider selling you the %s.",
                    selling->short_description, s_item->short_description);
            act(Gbuf4, FALSE, ch, 0, 0, TO_ROOM);
          }
          else
            mobsay(ch, "Sell you what?");
          return (TRUE);
        }
        else if (ch == get_char_room_vis(ch, "salesman"))
        {
          sprintf(Gbuf4,
                  "$n arches an eyebrow and says, 'Who 'ya talkin' to, %s?'\r\n"
                  "Use: BUY <OBJ> [FROM] <SELLER>.", pl->player.name);
          act(Gbuf4, FALSE, ch, 0, 0, TO_ROOM);
        }
        else
          mobsay(ch, "Yeah, we can't ALL oblige you, ya know?");
      }
      else
      {
        if (ch == get_char_room_vis(pl, Gbuf2))
        {
          if (s_item == selling)
          {
            if (transact(ch, s_item, pl, i_val))
            {
              if (ch->following)
                stop_follower(ch);
              REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
            }
          }
          else
            mobsay(ch, "I don't have that.");
          return (TRUE);
        }
      }
      break;
    default:
      return FALSE;
      break;
    }
  }
  else if (IS_LIGHT(ch->in_room))
  {
    if (GET_STAT(ch) == STAT_SLEEPING)
    {
      SET_POS(ch, POS_STANDING + STAT_NORMAL);
      act("$n becomes alert, ready to assault any unsuspecting "
          "persons with $s salesmanship.", TRUE, ch, 0, 0, TO_ROOM);
    }
    if (ch->following)
    {
      if (!selling)
      {
        /*
         * No items to sell, must have had item stolen
         */
        act("$n looks puzzled.", TRUE, ch, 0, 0, TO_ROOM);
        stop_follower(ch);
        REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
        return (TRUE);
      }
      i_val = selling->cost * 2 + 4;
      if (ch->following->in_room == ch->in_room)
      {
        if (ch->only.npc->spec[0] > 108)
        {                       /*
                                 * Number is not fixed in stone
                                 *
                                 */
          act("$n throws $s hands up in disgust.", TRUE, ch, 0, 0, TO_ROOM);
          old_follow = ch->following;
          stop_follower(ch);
          REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
          return TRUE;
        }
        else
        {
          if (!IS_AWAKE(ch->following))
            return (0);
          Gbuf4[0] = 0;
          switch (number(0, 14))
          {
          case 0:
            if (!(selling))
            {
              logit(LOG_EXIT, "assert: salesman proc");
              raise(SIGSEGV);
            }
            act("$n says 'I tell you this $o is of the finest quality.'",
                FALSE, ch, selling, 0, TO_ROOM);
            break;
          case 1:
            sprintf(Gbuf4, "Only %d coppers - a bargain!", i_val);
            do_say(ch, Gbuf4, 0);
            break;
          case 2:
            Gbuf4[0] = 'a';
            act("$N splutters 'You know, I have six hungry wives\r\n"
                "and a child to feed...'", FALSE, pl, 0, ch, TO_CHAR);
            break;
          case 3:
            act("$N waves the $p in your face.",
                FALSE, ch->following, selling, ch, TO_CHAR);
            act("$N waves the $p in $n's face.",
                FALSE, ch->following, selling, ch, TO_ROOM);
            break;
          case 4:
            if (!(selling))
            {
              logit(LOG_EXIT, "assert: salesman proc");
              raise(SIGSEGV);
            }
            act("The salesman demonstrates the unique usefulness of the $o.",
                TRUE, ch, selling, 0, TO_ROOM);
            break;
          case 5:
            do_action(ch, 0, CMD_CHUCKLE);
            break;
          default:
            break;
          }
          if (Gbuf4[0])
          {
            if (!(selling))
            {
              logit(LOG_EXIT, "assert: salesman proc");
              raise(SIGSEGV);
            }
            act("$n tries to sell the $o to $N.", FALSE, ch, selling,
                ch->following, TO_NOTVICT);
            ch->only.npc->spec[0] += 1;
          }
        }
        return TRUE;
      }
      else
      {
        stop_follower(ch);
        REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
        return TRUE;
      }
    }
    if (!ch->following && !selling)
    {
      SET_BIT(ch->specials.act, ACT_SCAVENGER);
      if (number(0, 5) == 0)
      {
        act("$n scans the floor.", TRUE, ch, 0, 0, TO_ROOM);
        return TRUE;
      }
    }
    else if (!ch->following)
    {
      REMOVE_BIT(ch->specials.act, ACT_SCAVENGER);
      for (k = world[ch->in_room].people; (pl = k); k = k->next_in_room)
      {
        if ((number(0, 4) < 3) && CAN_SEE(ch, pl) &&
            !circle_follow(ch, pl) && (old_follow != pl))
        {
          add_follower(ch, pl);
          SET_BIT(ch->specials.act, ACT_SENTINEL);
          sprintf(Gbuf4,
                  "The salesman saunters up to you and says, 'Hey %s!  Have I got a\r\n"
                  "deal for you! Take a look at this magnificent $o.\r\n"
                  "Isn't it just a dream?  And it can be yours for just %d coins!'",
                  pl->player.name, i_val);
          if (!(selling))
          {
            logit(LOG_EXIT, "assert: salesman proc");
            raise(SIGSEGV);
          }
          act(Gbuf4, FALSE, pl, selling, 0, TO_CHAR);
          act("$N makes a sales pitch to $n.", FALSE, pl, 0, ch, TO_ROOM);
          ch->only.npc->spec[0] = 100;
          return TRUE;
          break;
        }
      }
    }
  }
  else if (IS_AWAKE(ch))
  {
    if (!ch->light)
    {                           /*
                                 * See if we have a light source
                                 */
      for (obj = ch->carrying; obj; obj = obj->next_content)
      {
        if (obj->type == ITEM_LIGHT)
        {
          strcpy(Gbuf1, obj->name);
          do_grab(ch, Gbuf1, 0);
          return (TRUE);
        }
      }
    }
    else
    {
      obj = ch->equipment[HOLD];
      strcpy(Gbuf1, obj->name);
      do_remove(ch, Gbuf1, 0);
      strcpy(Gbuf1, obj->name);
      do_drop(ch, Gbuf1, 0);
      return (TRUE);
    }
    if (number(0, 5) == 0)
    {
      mobsay(ch, "Ah, must be time to go to bed!");
      act("The sounds of violent snoring filter through the area.", FALSE,
          ch, 0, 0, TO_ROOM);
      do_sleep(ch, 0, 0);
      if (ch->following)
      {
        stop_follower(ch);
        REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
      }
      return TRUE;
    }
  }
  return (FALSE);
}

int jester(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd)
    return (0);

  switch (number(0, 60))
  {
  case 0:
    mobsay(ch, "You are a real stinker!");
    return (1);
  case 1:
    mobsay(ch, "Have you considered getting a lobotomy?");
    return (1);
  case 2:
    mobsay(ch, "You're as stupid as you look!");
    return (1);
  case 3:
    mobsay(ch, "Get a real hair-cut!");
    return (1);
  case 4:
    act("$n does a backflip.", TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "Ha!");
    return (1);
  default:
    return (0);
  }
}

int spiny(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (!IS_FIGHTING(ch))
    {
      switch (dice(3, 2))
      {
      case 3:
        act("$n flips onto its back...or is that its front?", TRUE, ch, 0, 0,
            TO_ROOM);
        break;
      case 4:
        act("$n makes some clicking noises.", FALSE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return TRUE;
}

int snowvulture(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (!IS_FIGHTING(ch))
    {
      switch (dice(3, 3))
      {
      case 3:
        act("$n squeaks, \"Skaaa? reet.\"", 0, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n flaps about.", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case 5:
        devour(ch, pl, cmd, arg);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int snowbeast(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    return (0);
  }
  else
  {
    if (!IS_FIGHTING(ch))
    {
      switch (dice(3, 6))
      {
      case 3:
        mobsay(ch, "Yaargh, arrogha!!?!");
        break;
      case 4:
        mobsay(ch, "Hmmph.");
        break;
      case 5:
        act("The snowbeast scratches itself.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 6:
        act("The snowbeast stares inquisitively at a point in space.", TRUE,
            ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

/*
 * A special for the Knife Shop Proprieter (mob-based)
 */

int clyde(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!pl)
  {
    if (!MIN_POS(ch, POS_STANDING + STAT_RESTING))
      return (FALSE);
    switch (dice(2, 5))
    {
    case 1:
      act("An evil grin crosses $n's face.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      act("$n whistles a tune.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    };
  }
  else if ((ch->in_room == real_room(12595)) && (cmd == CMD_SOUTH) &&
           !GET_CLASS(pl, CLASS_ROGUE) &&
           (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
  {
    act
      ("With a gentle, but firm hand, Clyde guides you away from the curtain.",
       FALSE, pl, 0, 0, TO_CHAR);
    act("Clyde skillfully redirects $n from going behind the curtain.", FALSE,
        pl, 0, 0, TO_ROOM);
    return (TRUE);
  }
  else if ((cmd == CMD_BUY) || (cmd == CMD_SELL) || (cmd == CMD_LIST) ||
           (cmd == CMD_VALUE) || (cmd == CMD_PERUSE))
  {
    mobsay(ch, "I'm not open for business right now.");
    /*
     * in fact, he's never open...
     */
  }
  return (FALSE);
}

/*
 * Another mob-based special...
 */

int waiter(P_char ch, P_char pl, int cmd, char *arg)
{
  int      check1, check2, check3, check4, check5;
  P_obj    i;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if ((cmd == CMD_EAST) && (ch->in_room == real_room(12576)))
  {
    check1 = real_object(12530);
    check2 = real_object(12531);
    check3 = real_object(12532);
    check4 = -1;
    check5 = -1;                /*
                                 * For when I add other menu items
                                 */
    for (i = pl->carrying; i; i = i->next_content)
    {
      if ((i->R_num == check1) || (i->R_num == check2) ||
          (i->R_num == check3) || (i->R_num == check4) ||
          (i->R_num == check5))
      {
        act("The waiter prevents you from leaving.",
            FALSE, pl, 0, 0, TO_CHAR);
        act("The waiter prevents $n from leaving.", FALSE, pl, 0, 0, TO_ROOM);
        mobsay(ch, "You must finish your meal here!");
        return (TRUE);
      }
    }
  }
  return (shop_keeper(ch, pl, cmd, arg));
}

int barmaid(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * Do special stuff, then call regular shop routine
   */

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return (shop_keeper(ch, pl, cmd, arg));
}

int cookie(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!pl)
  {
    switch (number(1, 15))
    {
    case 2:
      do_action(ch, 0, CMD_BURP);
      switch (number(1, 3))
      {
      case 1:
        mobsay(ch, "Mmm! Mammoth!");
        break;
      case 2:
        mobsay(ch, "Mmm! Yak liver!");
        break;
      case 3:
        mobsay(ch, "Hmm. Can't quite place that one.");
        break;
      default:
        break;
      }
    default:
      break;
    }
  }
  return (FALSE);
}

int neophyte(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    if (ch->in_room == real_room(12587))
    {
      if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_CLERIC))
      {
        mobsay(ch, "Only the chosen get to see the master!");
        return (TRUE);
      }
    }
  }
  else
  {
    switch (number(1, 15))
    {
    case 1:
      do_action(ch, 0, CMD_STARE);
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int guru_anapest(P_char ch, P_char pl, int cmd, char *arg)
{                               /*
                                 * If in floating position, rotate, slow flips,
                                 *
                                 * etc...
                                 */
  P_char   who;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    if (!IS_AWAKE(ch))
      return (FALSE);
    argument_interpreter(arg, Gbuf1, Gbuf2);
    who = get_char_room(Gbuf1, ch->in_room);
    switch (cmd)
    {
    case CMD_WORSHIP:
      if (who == ch)
      {
        do_action(pl, arg, CMD_WORSHIP);
        mobsay(ch, "Don't worship me, for none are worthy of such respect.");
        return (TRUE);
      }
      break;
    case CMD_NUDGE:
      if (who == ch)
      {
        do_action(pl, arg, CMD_NUDGE);
        strcpy(Gbuf1, GET_NAME(pl));
        do_action(ch, Gbuf1, CMD_WINK);
        return (TRUE);
      };
      break;
    case CMD_WINK:
      if (who == ch)
      {
        do_action(pl, arg, CMD_WINK);
        strcpy(Gbuf1, GET_NAME(pl));
        do_action(ch, Gbuf1, CMD_NUDGE);
        return (TRUE);
      };
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 4))
    {
      spell_teleport(GET_LEVEL(ch), ch, 0, 0, ch, 0);
    }
  }
  else
  {
    switch (number(0, 25))
    {
    case 1:
      mobsay(ch, "Existence is suffering.");
      break;
    case 2:
      mobsay(ch, "Suffering is the end result of greed.");
      break;
    case 3:
      mobsay(ch, "Information complicates our lives.");
      break;
    case 4:
      mobsay(ch, "The pinnacle of existence is nothingness.");
      break;
    default:
      break;
    }
  }
  return FALSE;
}

int confess_figure(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    if (cmd == CMD_TELL)
    {
      return (TRUE);
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (number(1, 11))
    {
    case 1:
      mobsay(ch, "I hope your conscience bothers you!");
      break;
    default:
      break;
    }
    return (FALSE);
  }
  else
  {
    if (ch->only.npc->spec[0])
    {
    }
    else
      switch (number(1, 10))
      {
      case 1:
        do_action(ch, 0, CMD_COUGH);
        break;
      default:
        break;
      }
  }
  return (FALSE);
}

int taxman(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   who;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!IS_AWAKE(ch))
    return (FALSE);
  if (pl)
  {
    argument_interpreter(arg, Gbuf1, Gbuf2);
    who = get_char_room(Gbuf1, ch->in_room);
    switch (cmd)
    {
    case CMD_BACKSTAB:
      if (who == ch)
      {
        mobsay(ch, "Oh no you don't!");
        strcpy(Gbuf1, GET_NAME(pl));
        do_action(ch, Gbuf1, CMD_SPANK);
      }
      break;
    default:
      break;
    }
  }
  else
  {
    switch (number(1, 15))
    {
    case 1:
      do_action(ch, 0, CMD_CACKLE);
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int albert(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Gbuf4[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!pl)
  {
    if (IS_SET(ch->specials.act, ACT_SENTINEL) &&
        (GET_HIT(ch) == GET_MAX_HIT(ch)))
    {
      if (GET_STAT(ch) == STAT_SLEEPING)
        do_wake(ch, 0, 0);
      else if (!MIN_POS(ch, POS_STANDING + STAT_RESTING))
        do_stand(ch, 0, 0);
      REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
    }
    else if ((world[ch->in_room].number == 12613) ||
             (world[ch->in_room].number == 12614))
    {
      if (IS_FIGHTING(ch))
      {
        do_action(ch, 0, CMD_SCREAM);
        do_flee(ch, 0, 0);
      }
    }
    else if (GET_HIT(ch) != GET_MAX_HIT(ch))
    {
      strcpy(Gbuf4, "rub ring");
      command_interpreter(ch, Gbuf4);
      if (world[ch->in_room].number != 12613)
      {
        mobsay(ch, "What happened to my ring?");
      }
      else
      {
        SET_BIT(ch->specials.act, ACT_SENTINEL);
      }
    }
  }
  return (FALSE);
}

int mage_anapest(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   i, temp;
  char     Gbuf4[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!IS_AWAKE(ch))
    return (FALSE);
  if (pl)
  {
    switch (cmd)
    {
    case CMD_BACKSTAB:
      do_action(ch, 0, CMD_GROWL);
      spell_teleport(GET_LEVEL(ch), ch, 0, 0, pl, 0);
      act("She apparently doesn't like that.\r\n", FALSE, pl, 0, 0, TO_CHAR);
      return (TRUE);
      break;
    default:
      break;
    }
  }
  else
  {
    if (ch->in_room == real_room(12581))
    {
      for (i = world[ch->in_room].people; i; i = temp)
      {
        temp = i->next_in_room;
        if ((i != ch) && !GET_CLASS(i, CLASS_SORCERER))
        {
          mobsay(ch, "Be gone!");
          /*
           * Get rid of them
           */
          spell_teleport(GET_LEVEL(ch), ch, 0, 0, i, NULL);
        }
        else if ((i != ch) && (GET_ALIGNMENT(i) > 350))
        {
          sprintf(Gbuf4, "%s I don't think I like you!", i->player.name);
          do_tell(ch, Gbuf4, 0);
        }
      }
    }
  }

  return FALSE;
}

int farmer(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   who;
  const char *str = NULL;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl)
  {
    if (!IS_AWAKE(ch))
      return (FALSE);
    argument_interpreter(arg, Gbuf1, Gbuf2);
    who = get_char_room(Gbuf1, ch->in_room);
    switch (cmd)
    {
    case CMD_TELL:
      if (who == ch)
      {
        do_tell(pl, arg, 0);
        do_action(ch, 0, CMD_NOD);
        switch (number(1, 20))
        {
        case 1:
          str = " I ain't got no problem with that - it's your opinion.";
          break;
        case 2:
          str = " Hehehe.";
          break;
        case 3:
          str =
            " You're just SO much smarter than simple little farmers like us...NOT!";
          break;
        default:
          break;
        }
        if (str)
        {
          Gbuf2[0] = 0;
          strcat(Gbuf2, GET_NAME(pl));
          strcat(Gbuf2, str);
          do_tell(ch, Gbuf2, 0);
        }
      }
    }
  }
  else
  {
    switch (number(1, 13))
    {
    case 1:
      do_action(ch, 0, CMD_YODEL);
      break;
    case 2:
      if (world[ch->in_room].room_flags & INDOORS)
        mobsay(ch, "Ya know, I really like being outside.");
      else
      {
        act("$n examines the ground for its agricultural potential.", TRUE,
            ch, 0, 0, TO_ROOM);
        if (world[ch->in_room].sector_type <= SECT_CITY)
          mobsay(ch, "Pbbbbbt!");
      }
      break;
    default:
      break;
    }
  }
  return (FALSE);
}

int phalanx(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    obj;
  sh_int   temp = 0, temp2 = 0;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == CMD_DEATH)
  {
    obj = read_object(12000, VIRTUAL);
    if (!(obj))
    {
      logit(LOG_EXIT, "assert: phalanx proc");
      raise(SIGSEGV);
    }
    act
      ("BOOM! $n explodes, leaving a $p, which\r\nfalls toward the ground, sparkling along the way.\r\n",
       FALSE, ch, obj, 0, TO_ROOM);
    obj_to_room(obj, ch->in_room);
    return (FALSE);
  }
  if (pl)
  {
    if ((cmd == CMD_UP) && (pl != ch))
    {
      act("$n whirrs around the ceiling.", TRUE, ch, 0, 0, TO_ROOM);
      act("$N is preventing you.", FALSE, pl, 0, ch, TO_CHAR);
      return (TRUE);
    }
  }
  else
  {
    if (IS_FIGHTING(ch))
    {
      if ((GET_HIT(ch) < (GET_MAX_HIT(ch) / 4)) &&
          (ch->in_room != real_room(12144)))
      {
        temp = ch->in_room;
        do_move(ch, 0, CMD_UP);
        if (ch->in_room != temp)
        {
          temp2 = ch->in_room;
          char_from_room(ch);
          char_to_room(ch, temp, 0);
          act("$n retreats upward.", TRUE, ch, 0, 0, TO_ROOM);
          ch->points.hit += ch->points.hit;
          char_from_room(ch);
          char_to_room(ch, temp2, 0);
          act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
        }
      }
    }
#if 1
    else if (ch->points.base_armor == -50)
    {
      act("$n forms a new configuration.", TRUE, ch, 0, 0, TO_ROOM);
      ch->points.base_armor = -100;
    }
#endif
    else
    {
      switch (dice(3, 7))
      {
      case 20:
        act("$n splits apart to reorganize.", TRUE, ch, 0, 0, TO_ROOM);
        ch->points.base_armor = -50;
        break;
      case 19:
        act("$n makes some crackling noises.", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case 7:
        act("A spark emanates from the interior of $n.",
            FALSE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
      }
    }
  }
  return (FALSE);
}

int skeleton(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   temp;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch == pl)
    return FALSE;

  if (cmd == CMD_DEATH)
  {
    if (!ch->only.npc->spec[0])
      ch->only.npc->spec[0] = 3;
    else if (!--ch->only.npc->spec[0])
      return (FALSE);

    temp = read_mobile(GET_RNUM(ch), REAL);
    if (temp)
    {
      char_to_room(temp, ch->in_room, 0);
      temp->only.npc->spec[0] = ch->only.npc->spec[0];
    }
    temp = read_mobile(GET_RNUM(ch), REAL);
    if (temp)
    {
      char_to_room(temp, ch->in_room, 0);
      temp->only.npc->spec[0] = ch->only.npc->spec[0];
    }
    act
      ("The bones of the skeleton split apart and reform into two new skeletons.",
       TRUE, ch, 0, 0, TO_ROOM);
  }
  if (!pl || !CAN_SEE(ch, pl))
    return FALSE;

  if (cmd == CMD_FLEE)
  {
    if (!number(0, 19))
    {
      act("As you turn to flee, a skeleton trips you!", FALSE, pl, 0, 0,
          TO_CHAR);
      act("$N turns to run, but a skeleton trips $M!", TRUE, ch, 0, pl,
          TO_NOTVICT);
      SET_POS(pl, GET_STAT(pl) + POS_PRONE);
      CharWait(pl, 4);
      return TRUE;
    }
  }
  else if ((cmd == CMD_NORTH) || (cmd == CMD_SOUTH) || (cmd == CMD_EAST) ||
           (cmd == CMD_WEST) || (cmd == CMD_UP) || (cmd == CMD_DOWN))
  {
    if (!number(0, 19))
    {
      act("As you try to leave, a skeleton leaps in front of you!", FALSE, pl,
          0, 0, TO_CHAR);
      act("$N trys to leave, but a skeleton blocks $M!", TRUE, ch, 0, pl,
          TO_NOTVICT);
      CharWait(pl, 2);
      return TRUE;
    }
  }
  return (FALSE);
}

int animated_skeleton(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   undead, ch2;
  struct follow_type *followers;
  int      num;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (ch == pl)
    return FALSE;

  if (!number(0, 3))
    return FALSE;

  if (ch->following)
    ch2 = ch->following;
  else
    ch2 = ch;

  if (cmd == CMD_DEATH)
  {
    num = 0;
    for (followers = ch2->followers; followers; followers = followers->next)
      if (followers->follower && IS_NPC(followers->follower) && 
          (GET_VNUM(followers->follower) == 1201))
        num++;
    if (num > (GET_LEVEL(ch2) - 10) || number(0, 2))
    {
      act("$n shatters into a pile of useless bones!", TRUE, ch, 0, 0,
          TO_ROOM);
      return TRUE;
    }
    undead = read_mobile(GET_RNUM(ch), REAL);
    if (undead)
    {
      SET_BIT(undead->specials.act,
              ACT_SENTINEL | ACT_ISNPC | ACT_SPEC | ACT_SPEC_DIE);
      if (!IS_SET(undead->specials.act, ACT_MEMORY))
      {
        clearMemory(undead);
      }
      GET_RACE(undead) = RACE_UNDEAD;
      GET_SEX(undead) = SEX_NEUTRAL;
//      GET_CLASS(undead) = GET_CLASS(ch);
      undead->player.m_class = CLASS_WARRIOR;   // needs to be fixed..
      GET_ALIGNMENT(undead) = -500;
//      GET_LEVEL(undead) = BOUNDED(1, (GET_LEVEL(ch) - 1), 10);
      undead->player.level = BOUNDED(1, (GET_LEVEL(ch) - 1), 10);
      undead->only.npc->str_mask =
        (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);
      undead->player.name = str_dup(ch->player.name);
      undead->player.short_descr = str_dup(ch->player.short_descr);
      undead->player.long_descr = str_dup(ch->player.long_descr);
      undead->points.damnodice = ch->points.damnodice - 1;
      undead->points.base_hitroll = undead->points.hitroll =
        GET_LEVEL(undead) / 4;
      undead->points.base_damroll = undead->points.damroll =
        GET_LEVEL(undead) / 4;
      undead->points.mana = undead->points.base_mana = 0;
      GET_PLATINUM(undead) = 0;
      GET_GOLD(undead) = 0;
      GET_SILVER(undead) = 0;
      GET_COPPER(undead) = 0;
      while (undead->affected)
        affect_remove(undead, undead->affected);
      GET_EXP(undead) = 0;
      mob_index[GET_RNUM(undead)].func.mob = animated_skeleton;
      char_to_room(undead, ch->in_room, 0);
      balance_affects(undead);
    }
    undead = read_mobile(GET_RNUM(ch), REAL);
    if (undead)
    {
      SET_BIT(undead->specials.act,
              ACT_SENTINEL | ACT_ISNPC | ACT_SPEC | ACT_SPEC_DIE);
      if (!IS_SET(undead->specials.act, ACT_MEMORY))
        clearMemory(undead);
      GET_RACE(undead) = RACE_UNDEAD;
      GET_SEX(undead) = SEX_NEUTRAL;
      //    GET_CLASS(undead) = GET_CLASS(ch);
      undead->player.m_class = CLASS_WARRIOR;   // needs to be fixed
      GET_ALIGNMENT(undead) = -500;
//      GET_LEVEL(undead) = BOUNDED(1, (GET_LEVEL(ch) - 1), 10);
      undead->player.level = BOUNDED(1, (GET_LEVEL(ch) - 1), 10);
      undead->only.npc->str_mask =
        (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);
      undead->player.name = str_dup(ch->player.name);
      undead->player.short_descr = str_dup(ch->player.short_descr);
      undead->player.long_descr = str_dup(ch->player.long_descr);
      undead->points.damnodice = ch->points.damnodice - 1;
      undead->points.base_hitroll = undead->points.hitroll =
        GET_LEVEL(undead) / 4;
      undead->points.base_damroll = undead->points.damroll =
        GET_LEVEL(undead) / 4;
      undead->points.mana = undead->points.base_mana = 0;
      GET_PLATINUM(undead) = 0;
      GET_GOLD(undead) = 0;
      GET_SILVER(undead) = 0;
      GET_COPPER(undead) = 0;
      while (undead->affected)
        affect_remove(undead, undead->affected);
      GET_EXP(undead) = 0;
      mob_index[GET_RNUM(undead)].func.mob = animated_skeleton;
      char_to_room(undead, ch->in_room, 0);
      balance_affects(undead);
    }
    act
      ("The bones of the skeleton split apart and reform into two new skeletons.",
       TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}

int spore_ball(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   k, next;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DEATH)
  {
    act("$n crumples, a noxious gas escaping its interior.", FALSE, ch, 0, 0,
        TO_ROOM);
    if (CHAR_IN_SAFE_ZONE(ch))
      act("The gas dissipates harmlessly.", FALSE, ch, 0, 0, TO_ROOM);
    else
    {
      for (k = world[ch->in_room].people; k; k = next)
      {
        next = k->next_in_room;

        if (k != ch)
          spell_poison(24, ch, 0, 0, k, 0);
      }
    }

    return TRUE;
  }

  return (FALSE);
}

int bridge_troll(P_char ch, P_char pl, int cmd, char *arg)
{
  int      gold;
  P_char   k;
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl && (pl != ch))
  {
    if (cmd == CMD_GIVE)
    {
      gold = GET_MONEY(ch);
      do_give(pl, arg, 0);
      if ((gold = (GET_MONEY(ch) - gold)))
      {
        if (gold < 500)
          mobsay(ch, "You STILL need to pay me 500 copper coins, pal.");
        else
        {
          strcpy(Gbuf1, GET_NAME(pl));
          do_action(ch, Gbuf1, CMD_SMILE);      /*
                                                 * smile
                                                 */
          k = world[ch->in_room].people;
          while ((k != ch) && (k != pl) && k)
            k = k->next_in_room;
          if (!k)
          {                     /*
                                 * Should not happen!
                                 */
            logit(LOG_DEBUG, "Troll error 1!");
            return (TRUE);
          }
          act("$N picks you and tosses you to the other side of the bridge!",
              FALSE, pl, 0, ch, TO_CHAR);
          act("$N throws $n to the other side of the bridge!", TRUE, pl, 0,
              ch, TO_NOTVICT);
          char_from_room(pl);
          if (k == ch)
            if (ch->in_room == real_room(1863)) /*
                                                 * calimport troll
                                                 */
              char_to_room(pl, real_room(1862), 0);
            else
              char_to_room(pl, real_room(14236), 0);
          else
          {
            if (ch->in_room == real_room(1863)) /*
                                                 * calimport troll
                                                 */
              char_to_room(pl, real_room(1864), 0);
            else
              char_to_room(pl, real_room(14238), 0);
          }
          act("$n lands in a pile here from the direction of the bridge!",
              TRUE, pl, 0, 0, TO_ROOM);
          SET_POS(pl, POS_SITTING + GET_STAT(pl));
        }
      }
      return (TRUE);
    }
  }
  else
  {
    ch->only.npc->spec[0]++;
    if (ch->only.npc->spec[0] == 6)
    {
      ch->only.npc->spec[0] = 0;
      GET_HIT(ch) = MIN(GET_HIT(ch) + 6, ch->points.base_hit);
    }
  }
  return (FALSE);
}

int blob(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    i, temp, next_obj;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
  {
    ch->only.npc->spec[0] = 0;
    return TRUE;
  }
  if (cmd || !IS_AWAKE(ch) || IS_FIGHTING(ch))
    return (FALSE);

  i = world[ch->in_room].contents;
  /*
   * Check for takeable item to absorb.
   */
  if ((i != NULL) && (IS_SET(i->wear_flags, ITEM_TAKE)) && !IS_ARTIFACT(i) )
  {
    act("$n absorbs $p with a squish.", FALSE, ch, i, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);              /*
                                 * Absorption happened. Happy blob! =)
                                 */
  }
  else if (ch->only.npc->spec[0] != 0)
  {
    ch->only.npc->spec[0]--;
    return (FALSE);
  }
  else
  {
    ch->only.npc->spec[0] = 5;  /*
                                 * >5 turns before digestion tried again.
                                 */
    /*
     * No items or item not takeable. See if there's anything to digest.
     */
    i = ch->carrying;
    if (i != NULL)
    {
      if ((GET_ITEM_TYPE(i) == ITEM_CONTAINER) ||
          (GET_ITEM_TYPE(i) == ITEM_STORAGE) ||
          (GET_ITEM_TYPE(i) == ITEM_QUIVER) ||
          (GET_ITEM_TYPE(i) == ITEM_CORPSE))
        for (temp = i->contains; temp; temp = next_obj)
        {
          next_obj = temp->next_content;
          obj_from_obj(temp);
          obj_to_char(temp, ch);
        }                       /*
                                 * if a container is digested, leave contents
                                 * for later digestion
                                 */
      act("$n digests $p.", FALSE, ch, i, 0, TO_ROOM);
      obj_from_char(i);
      extract_obj(i, TRUE); // Shouldn't be an arti, but 'in game.'
      return (TRUE);
    }
    return (FALSE);
  }
}


/*
 * Boulder pushers will check to see if there are non-evil races in the rooms
 * * they're surveying, then possibly throw a "boulder" at them as a warning,
 * * inflicting minor damage.
 * *
 * * Designed for the ogre boulder pushers of Faang by Oghma
 * * -- DTS 2/8/95
 */

int boulder_pusher(P_char ch, P_char t_ch, int cmd, char *arg)
{
  int      to_room = NOWHERE, dam = 0;
  P_char   victim;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
  {
    return (TRUE);
  }
  if (!ch || t_ch || cmd)
  {
    return (FALSE);
  }
  if (IS_FIGHTING(ch) || !IS_AWAKE(ch))
    return (FALSE);

  switch (world[ch->in_room].number)
  {
  case 15354:
  case 15355:
  case 15356:
    to_room = 15302;
    break;
  case 15299:
  case 15300:
  case 15301:
  case 15302:
    to_room = 15253;
    break;
  default:
    return (FALSE);
  }

  if (world[real_room(to_room)].people)
  {
    for (victim = world[real_room(to_room)].people;
         victim && EVIL_RACE(victim); victim = victim->next_in_room) ;
    if (victim)
    {                           /*
                                 * not an evil race:  drow/duergar/ogre/troll
                                 */
      if ((CAN_SEE(ch, victim)) && (GET_LEVEL(victim) < MINLVLIMMORTAL))
      {
        if (number(1, 100) < 20)
        {
          act
            ("$n suddenly pushes a nearby boulder off the ledge with a roar!",
             FALSE, ch, 0, 0, TO_ROOM);
          act("$n peers downward to see the results of $s toss.", FALSE, ch,
              0, 0, TO_ROOM);
          if (!IS_SET
              (zone_table[world[victim->in_room].zone].flags, ZONE_SILENT) &&
              !IS_SET(world[victim->in_room].room_flags, ROOM_SILENT))
          {
            act
              ("You hear a mighty roar from overhead, and feel a sudden chill!",
               TRUE, victim, 0, 0, TO_CHAR);
            act("You suddenly hear a mighty roar from overhead!", TRUE,
                victim, 0, 0, TO_ROOM);
          }
          else
          {
            act("You feel a sudden chill, as of impending doom.",
                TRUE, victim, 0, 0, TO_CHAR);
          }

          /*
           * does it hit? partly based on chance, with slight bonus for dex
           */
          if ((number(1, 100) + STAT_INDEX(GET_C_DEX(victim))) < 50)
          {                     /*
                                 * hit
                                 *
                                 */
            act("A distant thud and a cry of pain can be heard from below.",
                FALSE, ch, 0, 0, TO_ROOM);

            act("A boulder hurtles down from overhead, striking you soundly!",
                FALSE, victim, 0, 0, TO_CHAR);
            act("A boulder hurtles down from overhead, striking $n soundly!",
                TRUE, victim, 0, 0, TO_ROOM);

            /*
             * damage is based partly on strength of ogre, partly on chance
             */
            dam = number(1, 50) + STAT_INDEX(GET_C_STR(ch));
            GET_HIT(victim) -= dam;
            update_pos(victim);
            send_to_char("&+ROWWW!!&n That really hurt!\r\n", victim);
            send_to_char
              ("It would probably be a good idea to GET OUT OF HERE!\r\n",
               victim);

            /*
             * does it kill victim?
             */
            if (GET_HIT(victim) < -10)
            {
              send_to_char("Alas, your wounds prove too much for you...\r\n",
                           victim);
              die(victim, ch);
              return (TRUE);
            }
            StartRegen(victim, EVENT_HIT_REGEN);

            /*
             * low possibility of stunnage or even KO
             */
            if (number(1, 100) < 10)
            {
              if (number(1, 100) < 40)
              {                 /*
                                 * KO
                                 */
                KnockOut(victim,
                         number(2, 25 - STAT_INDEX(GET_C_CON(victim))));
              }
              else
              {                 /*
                                 * stun
                                 */
                Stun(victim, ch, (number(2, 10) * PULSE_VIOLENCE), TRUE);
              }
            }
            return (TRUE);
          }
          else
          {                     /*
                                 * miss
                                 */
            act
              ("A distant thud can be heard.  $n curses and kicks at the ground.",
               TRUE, ch, 0, 0, TO_ROOM);
            act
              ("A boulder from overhead narrowly misses you and bounces off to one side.",
               FALSE, victim, 0, 0, TO_CHAR);
            act("Perhaps would be a good time to LEAVE this area!", FALSE,
                victim, 0, 0, TO_CHAR);
            act
              ("A boulder from overhead narrowly misses $n and bounces off to one side.",
               TRUE, victim, 0, 0, TO_ROOM);
            act("You count your lucky stars that it didn't strike you!", TRUE,
                victim, 0, 0, TO_ROOM);
            act
              ("It would probably be a good idea to leave before more rocks come!",
               TRUE, victim, 0, 0, TO_ROOM);
            return (TRUE);
          }
        }
      }
    }
  }
  /*
   * random actions if no target or unsatisfactory target
   */
  switch (number(1, 30))
  {
  case 1:
    do_action(ch, 0, CMD_MUTTER);
    return (TRUE);
  case 2:
    do_action(ch, 0, CMD_GRUMBLE);
    return (TRUE);
  case 3:
    do_action(ch, 0, CMD_TARZAN);
    return (TRUE);
  case 4:
    do_action(ch, 0, CMD_STOMP);
    return (TRUE);
  case 5:
    do_action(ch, 0, CMD_GRUNT);
    return (TRUE);
  case 6:
    do_action(ch, 0, CMD_ROAR);
    return (TRUE);
  case 7:
    do_action(ch, 0, CMD_FART);
    return (TRUE);
  case 8:
    do_action(ch, 0, CMD_STARE);
    return (TRUE);
  case 9:
    do_action(ch, 0, CMD_DROOL);
    return (TRUE);
  default:
    return (FALSE);
  }
}

/*
 * This is for Vaprak's kobold area.  When the mob dies, it becomes object
 * * 1438, a pile of stones, and its items and cash are placed within the
 * * container.
 * * Damon Silver, aka Oghma 8/3/94
 * * -- updated by DTS 2/9/95
 */

int stone_crumble(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    pile, money, obj, next_obj;
  int      pos, room;

  if( cmd == CMD_SET_PERIODIC )
    return FALSE;

  if( cmd != CMD_DEATH || !ch || IS_PC(ch) )
  {
    return FALSE;
  }

  // Where to put pile.
  room = (ch->in_room >= 0 && ch->in_room <= top_of_world) ? ch->in_room : NOWHERE;
  if( room == NOWHERE )
  {
    room = real_room(ch->specials.was_in_room);
  }
  // If we couldn't find a spot, just let the regular death code handle things.
  if( room == NOWHERE )
  {
    return FALSE;
  }

  if( !(pile = read_object(VOBJ_KOBOLD_DEATH_STONEPILE, VIRTUAL)) )
  {
    logit(LOG_OBJ, "stone_crumble: Could not load stone pile.");
    return FALSE;
  }

  if( pile->type != ITEM_CONTAINER )
  {
    logit(LOG_OBJ, "stone_crumble: Object %d not type ITEM_CONTAINER!! Aborted.", VOBJ_KOBOLD_DEATH_STONEPILE);
    return FALSE;
  }

  // Transfer inventory to pile
  for (obj = ch->carrying; obj; obj = next_obj)
  {
    next_obj = obj->next_content;
    obj_from_char(obj);
    obj_to_obj(obj, pile);
  }

  // Transfer equipment to pile
  for (pos = 0; pos < MAX_WEAR; pos++)
  {
    if (ch->equipment[pos])
    {
      obj_to_obj(unequip_char(ch, pos), pile);
    }
  }

  // Transfer money
  if (GET_MONEY(ch) > 0)
  {
    money = create_money(GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch), GET_PLATINUM(ch));
    obj_to_obj(money, pile);
  }

  obj_to_room(pile, room);

  // Clue players in
  act("$n stops fighting, and begins to shake.\r\n"
  "$n silently crumbles into $p... a cloud of dust rises.", TRUE, ch, pile, 0, TO_ROOM);
  return TRUE;

}

#define GUARDIAN_HELPER_LIMIT    30

int goodie_guardian(P_char ch, P_char pl, int cmd, char *arg)
{
  register P_char i;
  int num, count = 0;
  P_char   guardian;
  P_obj t_obj, next;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }
  if (!ch)
  {
    return FALSE;
  }
  
  if(cmd == CMD_DEATH)
  {
    debug("&+WGuardian death called.&n");
    act("&nThe guard moves to attack one final time, but instead chokes on his own &+rblood&n and falls to the ground, lifeless.\n&n", FALSE, ch, 0, 0, TO_ROOM);

    return true;
  }
  
  if (cmd != 0)
  {
    return FALSE;
  }
  
  if (IS_FIGHTING(ch))
  {
    /*
     * attempt to "summon" an elite tharn soldier...only possible if less than GUARDIAN_HELPER_LIMIT
     * * in world
     */
    for (i = character_list; i; i = i->next)
    {
      if ((IS_NPC(i)) && (GET_VNUM(i) == 446))
      {
        count++;
      }
    }
    if (count < GUARDIAN_HELPER_LIMIT)
    {
      if (number(1, 100) < 35)
      {
        guardian = read_mobile(446, VIRTUAL);
        if (!guardian)
        {
          logit(LOG_EXIT, "assert: error in highwayman() proc");
          raise(SIGSEGV);
        }
        act
          ("$n &nyells 'To arms brothers! Help me destroy this threat to our &+glands&n!&n\r\n"
           "&+WAn elite guard &ncharges into the fray, assisting his comrade...&n\r\n",
           FALSE, ch, 0, guardian, TO_ROOM);
        char_to_room(guardian, ch->in_room, 0);
        return TRUE;
      }
    }
  }

  return FALSE;

}

#undef GUARDIAN_HELPER_LIMIT


#define BAHAMUT_HELPER_LIMIT    3
int bahamut(P_char ch, P_char pl, int cmd, char *arg)
{
  register P_char i;
  int num, count = 0;
  P_char   dragon;
  P_obj t_obj, next, heart;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_DEATH )
  {
    act("&+WWith his very last breath, Bahamut closes his eyes.\n&n"
    "&+WBahamut's body begins to shimmer brilliantly, his flesh becoming more and more translucent!\n&n"
    "&+WAs the light subsides, only a few pieces of his once great body remain.&n", FALSE, ch, 0, 0, TO_ROOM);

    for( t_obj = ch->carrying; t_obj; t_obj = next )
    {
      next = t_obj->next_content;
      obj_from_char(t_obj);
      obj_to_room(t_obj, ch->in_room);
    }

    for (num = 0; num < MAX_WEAR; num++)
      if (ch->equipment[num])
        obj_to_room(unequip_char(ch, num), ch->in_room);

    heart = read_object(VOBJ_WH_DRAGONHEART_BAHAMUT, VIRTUAL);
    obj_to_room(heart, ch->in_room);
    // 3 mud days to complete (object ticks are the same as mob ticks.. *sigh*).
    // (Sec / MudDay) * (Pulse / Sec) * (ObjPulse / Pulse) = ObjPulse / MudWeek
    heart->value[0] = (3 * SECS_PER_MUD_DAY * WAIT_SEC) / PULSE_MOBILE;
    // Time in minutes = ObjPulse * (Pulse / ObjPulse) * (Sec / Pulse) = Secs
    heart->value[1] = ((heart->value[0] * PULSE_MOBILE) / WAIT_SEC);
    // Secs / 3600 = Hrs
    heart->value[2] =  heart->value[1] / 3600;
    // (Secs / 60) % 60 = Remainder of Mins
    heart->value[3] = (heart->value[1] / 60) % 60;
    // Secs % 60 = Remainder of Secs.
    heart->value[4] = (heart->value[1]) % 60;
    debug("&+WBahamut death: Heart decays in &+C%d&+W obj ticks = &+C%d&+W sec = &+C%d&+W:&+C%02d&+W:&+C%02d&+W.&n",
      heart->value[0], heart->value[1], heart->value[2], heart->value[3], heart->value[4]);
    logit(LOG_OBJ, "Bahamut death: Heart decays in %d obj ticks = %d sec = %d:%02d:%02d.",
      heart->value[0], heart->value[1], heart->value[2], heart->value[3], heart->value[4]);
    // Value1 is break chance haha.. need to 0 that out.
    heart->value[1] = 0;
    return TRUE;
  }

  if( cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  if( IS_FIGHTING(ch) )
  {
    /*
     * attempt to "summon" a silver dragon...only possible if less than BAHAMUT_HELP_LIMIT
     * * in world
     */
    for (i = character_list; i; i = i->next)
    {
      if ((IS_NPC(i)) && (GET_VNUM(i) == 25758))
      {
        count++;
      }
    }
    if (count < BAHAMUT_HELPER_LIMIT)
    {
      if (number(1, 100) < 50)
      {
        dragon = read_mobile(25758, VIRTUAL);
        if (!dragon)
        {
          logit(LOG_EXIT, "assert: error in bahamut() proc");
          raise(SIGSEGV);
        }
        act
          ("$n &+Wraises onto his hind legs and releases a tremendous &+RROAR!!!!&n\r\n"
           "&+BA magnificant &+Wsilver dragon&+B steps out of a &+Lportal&+B that closes instantly...&n\r\n",
           FALSE, ch, 0, dragon, TO_ROOM);
        char_to_room(dragon, ch->in_room, 0);
        return TRUE;
      }
    }
  }

  return FALSE;
}

#undef BAHAMUT_HELPER_LIMIT

#define IMP_LIMIT 5

int kobold_priest(P_char ch, P_char pl, int cmd, char *arg)
{
  register P_char i;
  P_char   imp;
  int      count = 0            /*
                                 * , flag = 0
                                 */ ;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
  {
    ch->only.npc->spec[0] = 0;
    return (TRUE);
  }
  if (!ch)
  {
    return (FALSE);
  }
  if (world[ch->in_room].number == 1482)
  {
    if ((cmd == CMD_NORTH) || (cmd == CMD_SOUTH) || (cmd == CMD_EAST))
    {
      if (pl)
      {
        act("$N makes a strange gesture!\r\n"
            "Suddenly, the way is blocked by an invisible wall of force!",
            FALSE, pl, 0, ch, TO_CHAR);
        act("$N makes a strange gesture!\r\n"
            "$n tries to leave the room but runs into an invisible barrier!",
            FALSE, pl, 0, ch, TO_NOTVICT);
        return (TRUE);
      }
      else
      {
        logit(LOG_DEBUG, "cmd in kobold_priest() but no pl!");
        raise(SIGSEGV);
      }
    }
    else if (cmd == CMD_WEST)
    {
      if (pl)
      {
        act("$N cackles with glee as $e sees you foolishly\r\n"
            "toss yourself off the dais and into the sacrificial pit!\r\n"
            "You find yourself tumbling down...\r\n"
            "                               ...down...\r\n"
            "                                      ...down...\r\n"
            "                                             ...into the pit.",
            FALSE, pl, 0, ch, TO_CHAR);
        act("$N cackles with glee as $e watches $n toss $mself into the\r\n"
            "sacrificial pit to the west!", FALSE, pl, 0, ch, TO_NOTVICT);
        char_from_room(pl);
        char_to_room(pl, real_room(1485), 0);
        return (TRUE);
      }
      else
      {
        logit(LOG_EXIT, "cmd to kobold_priest but no pl!");
        raise(SIGSEGV);
      }
    }
  }
  else
  {
    if (((cmd == CMD_NORTH) || (cmd == CMD_EAST) || (cmd == CMD_SOUTH) ||
         (cmd == CMD_UP) || (cmd == CMD_DOWN)) && (!IS_TRUSTED(pl)) &&
        (number(1, 100) > 20))
    {
      if (pl)
      {
        if (EXIT(pl, cmd_to_exitnumb(cmd)))
        {
          act("$N makes a strange gesture!\r\n"
              "Suddenly, the way is blocked by an invisible wall of force!",
              FALSE, pl, 0, ch, TO_CHAR);
          act("$N makes a strange gesture!\r\n"
              "$n tries to leave the room but runs into an invisible barrier!",
              FALSE, pl, 0, ch, TO_NOTVICT);
          return (TRUE);
        }
        else
        {
          return (FALSE);
        }
      }
      else
      {
        logit(LOG_EXIT, "cmd to kobold_priest but no pl!");
        raise(SIGSEGV);
      }
    }
    else
    {
      return (FALSE);
    }
  }

  /*
   * if it's some command besides a periodic event call, return
   */
  if (cmd != 0)
  {
    return (FALSE);
  }
  if (ch->only.npc->spec[0] > 0)
  {
    ch->only.npc->spec[0]--;
    return (FALSE);
  }
  else
  {
    ch->only.npc->spec[0] = 4;
    if (IS_FIGHTING(ch))
    {
      /*
       * attempt to "summon" purple imp...only possible if less than IMP_LIMIT
       * * in world
       */
      for (i = character_list; i; i = i->next)
      {
        if ((IS_NPC(i)) && (GET_VNUM(i) == 1440))
        {
          count++;
        }
      }
      if (count < IMP_LIMIT)
      {
        if (number(1, 100) < 90)
        {
          imp = read_mobile(1440, VIRTUAL);
          if (!imp)
          {
            logit(LOG_EXIT, "assert: error in kobold_priest() proc");
            raise(SIGSEGV);
          }
          act("&+M$n incants a powerful spell of summoning.\r\n"
              "&+M$N arrives from the depths of &+RHell&+M to aid its master!\r\n",
              FALSE, ch, 0, imp, TO_ROOM);
          char_to_room(imp, ch->in_room, 0);
          return (TRUE);
        }
        else
        {
          do_action(ch, 0, CMD_CURSE);
        }
      }
    }
  }

  return (FALSE);
}

#undef IMP_LIMIT

int stone_golem(P_char ch, P_char pl, int cmd, char *arg)
{

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
  {
    return (FALSE);
  }
  if (!ch || !pl)
  {
    return (FALSE);
  }
  if (world[ch->in_room].number == 1483)
  {
    if (cmd == CMD_WEST && !IS_TRUSTED(pl) && (number(1, 100) > 20))
    {
      act("You try to leave the room but are shoved back by $N!",
          FALSE, pl, 0, ch, TO_CHAR);
      act("$n tries to leave the room but is shoved back by $N!",
          FALSE, pl, 0, ch, TO_NOTVICT);
      return (TRUE);
    }
  }
  return (FALSE);
}

int tako_demon(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   k, victim = NULL;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
  {
    return (TRUE);
  }
  if (!ch)
  {
    return (FALSE);
  }
  if (world[ch->in_room].number == 1485)
  {
    if (pl && cmd == CMD_UP && !IS_TRUSTED(pl))
    {
      if (number(1, 100) > 50)
      {                         /*
                                 * caught!
                                 */
        act
          ("You try to leave but are grappled backwards by a snaky tentacle!",
           FALSE, pl, 0, ch, TO_CHAR);
        act
          ("$n tries to leave but is grappled backwards by a snaky tentacle!",
           FALSE, pl, 0, ch, TO_NOTVICT);
        return (TRUE);
      }
    }
    else if (!cmd && !IS_FIGHTING(ch))
    {
      for (k = world[real_room(1484)].people; k; k = k->next)
      {
        if (IS_PC(k) && CAN_SEE(ch, k) && !IS_TRUSTED(k) && !IS_FIGHTING(k))
        {
          victim = k;
          break;
        }
      }
      if (victim && (world[victim->in_room].number == 1484))
      {
        if (number(0, 100) < 30)
        {
          act("You are suddenly &+BYANKED&n downward by a snaky tentacle!",
              FALSE, victim, 0, 0, TO_CHAR);
          act("$n is suddenly &+BYANKED&n downwards by a snaky tentacle!",
              TRUE, victim, 0, 0, TO_ROOM);
          char_from_room(victim);
          char_to_room(victim, real_room(1485), 0);
          act("$N is suddenly yanked here from above by $n!",
              TRUE, ch, 0, victim, TO_NOTVICT);
          MobStartFight(ch, victim);
          return (TRUE);
        }
      }
    }
  }
  return (FALSE);
}

int chicken(P_char ch, P_char pl, int cmd, char *arg)
{

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
  {
    return (TRUE);
  }
  if (!ch || cmd)
  {
    return (FALSE);
  }
  if (IS_AWAKE(ch) || !IS_FIGHTING(ch))
  {
    switch (number(1, 25))
    {
    case 1:
      act("$n clucks contently on $s nest.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      act
        ("$n becomes frightened and looks all around, sensing danger nearby.",
         TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  else if (IS_FIGHTING(ch))
  {
    switch (number(1, 4))
    {
    case 1:
      act("'SQUAAAAAAWWWK' screams $n as $e tries to run away from you!",
          TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
    }
  }
  return (FALSE);

}

int cc_fisherffolk(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I hear the zoo keeper is looking for feathers again.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "I wish these darned fish would start biting.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Darn, I'm out of worms again.");
      return TRUE;
    }
  default:
    return FALSE;
  }
}

int cc_female_ffolk(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch,
             "Those sharkteeth trinkets some of the ffolk have sure are pretty.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "I wish I had a sharktooth necklace.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I wonder where to get a sharktooth necklace at?");
      return TRUE;
    }
  default:
    return FALSE;
  }
}

int cc_warehouse_man(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      act("$n flexes $s muscles as $e moves around some boxes.",
          TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  case 2:
    {
      act("$n mumbles something about $s foreman.", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I swear I am going to quit this damn job!");
      return TRUE;
    }
  default:
    return FALSE;
  }
}

int cc_warehouse_foreman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      act("$n yells, 'Get back to work!'", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  case 2:
    {
      act("$n yells, 'One more remark like that and you're fired!'",
          TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  case 3:
    {
      act("$n yells, 'Hurry up!  These crates gotta ship today!'",
          TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  default:
    return FALSE;
  }
}

int barbarian_spiritist(P_char ch, P_char pl, int cmd, char *arg)
{

  P_char   vict;
  struct affected_type af;

  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 5))
  {
  case 0:
    act
      ("$n gestures wildly, calling upon the spirits of his long dead ancestors to protect him in his time of need!  Luckily for you, they don't seem to have answered him this time.",
       TRUE, ch, 0, 0, TO_ROOM);
    return FALSE;
  case 1:
    act
      ("Tossing a handful of strange smelling herbs into the air, $n calls upon the spirit forces of nature to protect him from the heathen outsiders!",
       TRUE, ch, 0, 0, TO_ROOM);
    vict = char_in_room(ch->in_room);
    if (vict && CAN_SEE(ch, vict) && !IS_NPC(vict))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_CURSE;
      af.duration = 5;
      af.modifier = -3;
      af.location = APPLY_HITROLL;
      affect_to_char(vict, &af);
      af.modifier = -3;
      af.location = APPLY_DAMROLL;
      affect_to_char(vict, &af);
      send_to_char
        ("\r\n&+BThe summoned spirits&N interfere with your battle ability!.\r\n",
         vict);
    }
    return TRUE;
  case 2:
    act
      ("Rubbing his bone totem in his hands, $n chants an ancient mantra of  summoning!  &+BA shimmering aura of pale blue&N suddenly appears and surrounds you!",
       TRUE, ch, 0, 0, TO_ROOM);
    vict = char_in_room(ch->in_room);
    if (vict && CAN_SEE(ch, vict) && !IS_NPC(vict))
    {
      if (vict->equipment[PRIMARY_WEAPON] != NULL)
      {
        SET_BIT(vict->equipment[PRIMARY_WEAPON]->extra_flags, ITEM_SECRET);
        send_to_char
          ("\r\n&+BThe summoned spirits&N tug at your weapon, causing it to fly from your grip!\r\n",
           vict);
        obj_to_room(unequip_char(vict, PRIMARY_WEAPON), vict->in_room);
      }
      else if (vict->equipment[SECONDARY_WEAPON] != NULL)
      {
        SET_BIT(vict->equipment[SECONDARY_WEAPON]->extra_flags, ITEM_SECRET);
        send_to_char
          ("\r\n&+BThe summoned spirits&N tug at your weapon, causing it to fly from your grip!\r\n",
           vict);
        obj_to_room(unequip_char(vict, SECONDARY_WEAPON), vict->in_room);
      }
      act
        ("\r\n&+BThe spirits&N swarm $N, causing $M to flail about blindly!\r\n",
         FALSE, ch, 0, vict, TO_NOTVICT);
    }
    return TRUE;
  case 3:
    act
      ("Waving his arms wildly, $n calls down the might of the great spirits to harm his enemies, those who have dared attack him and his village!\r\n",
       TRUE, ch, 0, 0, TO_ROOM);
    vict = char_in_room(ch->in_room);
    if (vict && CAN_SEE(ch, vict) && !IS_NPC(vict))
    {
      spell_cyclone(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      do_action(ch, 0, CMD_CACKLE);
    }
    return TRUE;
  default:
    return FALSE;
  }
}

int plant_attacks_poison(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict, next_ch;
  int      flag = FALSE;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || pl || cmd)
    return FALSE;

/*  LOOP_THRU_PEOPLE(vict, ch)*/
  for (vict = world[ch->in_room].people; vict; vict = next_ch)
  {
    next_ch = vict->next_in_room;

    if ((vict != ch) && (IS_PC(vict) || IS_PC_PET(vict)) &&
        !number(0, 2) && !saves_spell(vict, SAVING_PARA) && CAN_SEE(ch, vict))
    {
      act("$n launches a volley of barbed red thorns!", TRUE, ch, 0, 0,
          TO_ROOM);
      act("You fling thorns towards your victims", FALSE, ch, 0, 0, TO_CHAR);
      act("One of $n's thorns pierces your skin!", FALSE, ch, 0, vict,
          TO_VICT);
      act("$N is hit by one of the thorns!", TRUE, ch, 0, vict, TO_ROOM);
      poison_neurotoxin(GET_LEVEL(ch), ch, 0, 0, vict, 0);
      flag = TRUE;
    }
  }

  return flag;
}

int plant_attacks_blindness(P_char ch, P_char pl, int cmd, char *arg)
{
  struct affected_type af;
  P_char   vict;
  int      duration_factor, eyewear_value, flag = FALSE;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || pl || cmd)
    return FALSE;

  LOOP_THRU_PEOPLE(vict, ch)
    if ((vict != ch) && (IS_PC(vict) || IS_PC_PET(vict))
        && !number(0, 2) &&
        !IS_AFFECTED(vict, AFF_BLIND) && !saves_spell(vict, SAVING_BREATH) &&
        CAN_SEE(ch, vict))
  {

    if (vict->equipment[WEAR_EYES])
      eyewear_value = (vict->equipment[WEAR_EYES]->value[0]);
    else
      eyewear_value = 0;

    duration_factor = (10 - eyewear_value + GET_LEVEL(ch));
    if (duration_factor < 1)
      duration_factor = 1;

    if (!flag)
    {
      act
        ("The bright red flowers along the vines of $n puff out a dense cloud of pollen!",
         TRUE, ch, 0, 0, TO_ROOM);
      act("You spray pollen towards the eyes of your victims", FALSE, ch, 0,
          0, TO_CHAR);
    }
    act("$n's pollen makes your eyes burn and water!  You can't see!",
        FALSE, ch, 0, vict, TO_VICT);
    act("$N staggers about blindly!", TRUE, ch, 0, vict, TO_NOTVICT);
    bzero(&af, sizeof(af));
    af.type = SPELL_BLINDNESS;
    af.duration = duration_factor;
    af.bitvector = AFF_BLIND;
    affect_join(vict, &af, FALSE, FALSE);
    flag = TRUE;
  }
  return flag;
}

int plant_attacks_paralysis(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict;
  int      duration_factor, flag = FALSE;
  struct affected_type af;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || pl || cmd)
    return FALSE;

  if (cmd == CMD_DEATH)                /*
                                 * unwrap vines upon death of vine
                                 */
    LOOP_THRU_PEOPLE(vict, ch)
      if ((vict != ch) && affected_by_spell(vict, SPELL_MAJOR_PARALYSIS))
      affect_from_char(vict, SPELL_MAJOR_PARALYSIS);

  duration_factor = 10;

  LOOP_THRU_PEOPLE(vict, ch)
    if ((vict != ch) && (IS_PC(vict) || IS_PC_PET(vict)) && !number(0, 2) &&
        !IS_AFFECTED2(vict, AFF2_MAJOR_PARALYSIS) &&
        !saves_spell(vict, SAVING_PARA))
  {
    act
      ("$n reach up and wrap themselves about you, making it difficult to move, or even breathe!",
       FALSE, ch, 0, vict, TO_VICT);
    act("$n reaches up and wraps about $N!", TRUE, ch, 0, vict, TO_NOTVICT);
    act("You reach up and wrap about $N!", TRUE, ch, 0, vict, TO_CHAR);
    bzero(&af, sizeof(af));
    af.type = SPELL_MAJOR_PARALYSIS;
    af.flags = AFFTYPE_SHORT;
    af.duration = duration_factor * WAIT_SEC;
    af.bitvector2 = AFF2_MAJOR_PARALYSIS;
    affect_join(vict, &af, FALSE, FALSE);
    flag = TRUE;
  }
  return flag;
}

/*
 * neverwinter mob procs
 */

int nw_woodelf(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I LOVE the forest!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "The trees are my home!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch,
             "The forest provides the perfect shelter for me and my forest friends!");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_elfhealer(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "The woods provide us with the natural healant of life!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "Do you not feel the natural healing vibes of the trees?");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch,
             "Sometimes, meditation amongst the trees provide the proper healant.");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_ammaster(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "The amethyst is the perfect stone of all life.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "The amethyst gives us our sustenance.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "The amethyst is the staff of life.");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_sapmaster(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "The sapphire represents power.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "The sapphire is that which would give us strength.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "The lion is the perfect symbol of the sapphire.");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_diamaster(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "The diamond gives us clarity of life.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "The diamond represents purity.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch,
             "One must channel their thoughts through the diamond for inspiration.");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_rubmaster(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "The ruby represents passion.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "Extreme emotions make life interesting.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "The ruby embodies all that we love and hate.");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_emmaster(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "The emerald sybolizes growth.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "The emerald is all that is nature.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch,
             "The trees, the rabbits, and even the ground you stand on is forcused through the emerald.");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_human(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 80))
  {
  case 1:
    {
      mobsay(ch, "Where the hell am I?");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch,
             "I take a lousy stroll from Verzanan, and this is where I end up?");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Shit. I need a tour guide or something.");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_hafbreed(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "At least the forest does not care who I am.");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_owl(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Hoo hoo!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "Hoo hoo!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Hoo hoo!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_golem(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Why don't these pesky mortals just leave me alone!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "I hate being bothered by flesh forms!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I must guard this tower with my life!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_agatha(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch,
             "Hah! I do not wish to speak with thee, unless thee hast news of Malchor!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch,
             "That Malchor OWES me, and I will soon be leaving for his tower for payment!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Soon, the golden horse shoe will be MINE!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_farmer(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I should not be here.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "There are fields to be plowed!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "There is corn to be grown!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_chicken(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Buk buk buk bugack!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "Buk buk buk bugack!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Bugack!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_pig(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Oink oink!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "Oink oink!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Oink oink!");
      return TRUE;
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_cow(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
  case 2:
  case 3:
    do_action(ch, 0, CMD_COW);
    return TRUE;

  default:
    return FALSE;
  }
}

int nw_chief(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "There is so much to do for a farming community.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "I must plan things around the seasons.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch,
             "Soon, the crops will be grown, and we will all eat like kings!");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_malchor(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "It is nice to have visitors at the museum!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "My museum is the best in all of the Neverwinter Woods!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "There is much to learn here.");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_builder(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I believe there needs to be a wall here.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "I must cement these corners correctly.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I wonder if that roof needs thatching.");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_carpen(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I must carve this furniture by the end of the day.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "Hmmmm...I do not think these walls will go in correctly.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I wonder whether these measurements are correct?");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_logger(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I love to walk on logs!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "Nobody can travel logs like I can!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I must get these logs to the stream!");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_cutter(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "TIMBER!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "TIMBER!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "TIMBER!");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_foreman(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Get those logs down to the stream!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "Get those trees cut, PRONTO!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "I want those planks cut by the end of the hour!");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_ansal(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I am glad we are getting along with the elves.");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch,
             "We produce the trees properly, and the elves leave us alone.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Looks like our production is going well.");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_vitnor(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "We serve the BEST and ONLY drinks in these woods!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "We have the tastiest drinks!");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Drinks are great to end a day on!");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_brock(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "I hate these rugs-- every day, rugs rugs rugs!!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch, "This store sure has plenty of dusty rugs.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "Let's see, what rugs will I kill today?");
    }
  default:
    {
      return FALSE;
    }
  }
}

int nw_merthol(P_char ch, P_char pl, int cmd, char *arg)
{
/*
 * check for periodic event calls
 */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 100))
  {
  case 1:
    {
      mobsay(ch, "Let's see if that dolt Brock can sell more than I can!");
      return TRUE;
    }
  case 2:
    {
      mobsay(ch,
             "My furs are of much better quality than those ugly dust - ridden rugs at Brock's.");
      return TRUE;
    }
  case 3:
    {
      mobsay(ch, "My furs will keep you warm in the winter.");
    }
  default:
    {
      return FALSE;
    }
  }
}

/*
 * support function for nw_mirroid, sends message to both rooms, and blocks/unblocks the
 * given exit.
 */
void nw_block_exit(int room, int dir, int flag)
{
  P_char   t_ch;
  int      i, room2;
  const char Gbuf1[] = "You see shifting reflections.\r\n";
  const char Gbuf2[] = "You hear a faint creaking, and see shifting reflections.\r\n";
  const char Gbuf3[] = "You hear a faint creaking.\r\n";

  if( (room == NOWHERE) || !VIRTUAL_EXIT(room, dir) )
    return;

  if( world[room].dir_option[dir]->to_room )
  {
    room2 = world[room].dir_option[dir]->to_room;
  }
  else
  {
    return;
  }

  if( room2 == NOWHERE )
  {
    return;
  }

  // TODO: Fix this such that is uses vis_mode = get_vis_mode(ch, room_no);
  //   'cause some people see in dark, some in light, some in both.
  // WTH does this for loop do?  Looks like crap to me.
  for( i = room; i != room2; i = room2 )
  {
    if( world[i].people && (!IS_ROOM(i, ROOM_SILENT) || IS_LIGHT(i)) )
    {
      if( IS_LIGHT(i) )
      {
        LOOP_THRU_PEOPLE(t_ch, world[i].people)
        {
          if( IS_AWAKE(t_ch) && !IS_AFFECTED(t_ch, AFF_BLIND) )
          {
            if( !IS_SET(world[i].room_flags, ROOM_SILENT) )
            {
              send_to_char(Gbuf2, t_ch);
            }
            else
            {
              send_to_char(Gbuf1, t_ch);
            }
          }
          else if( IS_AWAKE(t_ch) )
          {
            send_to_char(Gbuf3, t_ch);
          }
        }
      }
      else
      {
        LOOP_THRU_PEOPLE(t_ch, world[i].people)
        {
          if( IS_AWAKE(t_ch) )
          {
            send_to_char(Gbuf3, t_ch);
          }
        }
      }
    }
  }

  if( flag )
  {
    SET_BIT(world[room].dir_option[dir]->exit_info, EX_BLOCKED);
    SET_BIT(world[room2].dir_option[(int) rev_dir[dir]]->exit_info, EX_BLOCKED);
  }
  else
  {
    REMOVE_BIT(world[room].dir_option[dir]->exit_info, EX_BLOCKED);
    REMOVE_BIT(world[room2].dir_option[(int) rev_dir[dir]]->exit_info, EX_BLOCKED);
  }
}

/*
 * support function for nw_mirroid, reset the 'loops' in maze.  JAB
 */

void nw_reset_maze(int room)
{
  int      other_room;

  if (world[room].number == BOUNDED(99202, world[room].number, 99206))
  {
    other_room = real_room0(world[room].number + 29);
    world[room].dir_option[3]->to_room = other_room;
    world[other_room].dir_option[1]->to_room = room;
  }
  else if (world[room].number == BOUNDED(99231, world[room].number, 99235))
  {
    other_room = real_room0(world[room].number - 29);
    world[room].dir_option[1]->to_room = other_room;
    world[other_room].dir_option[3]->to_room = room;
  }
  else if (world[room].number == 99201)
  {
    other_room = real_room0(99236);
    world[room].dir_option[1]->to_room = other_room;
    world[other_room].dir_option[3]->to_room = room;
  }
  else if (world[room].number == 99236)
  {
    other_room = real_room0(99201);
    world[room].dir_option[1]->to_room = other_room;
    world[other_room].dir_option[3]->to_room = room;
  }
}

/*
 * ok, mirroids are very xenophobic, and move away from anything that's not
 * another mirroid, blocking the passageway behind them in the process.
 * They are also very claustrophobic, and mildy agoraphobic, so if they
 * become trapped in a room with no exits, they WILL open one, and if they
 * are in a room with exits in all directions, they are real likely to close
 * one of them off.
 *
 * Mirroids can move and/or block or unblock an exit at the same time.
 *
 * Maze rooms are 99201-99236, and form a 6x6 wrap-around helical grid, they
 * are identical except for exits.  Room 99237 is the 'exit' room, Room 99200
 * is the 'entrance' room.  Rooms 99201 thru 99206 are the 'western edge',
 * and at most 1 (one) of them will hold the 'exit' to 99237.  Rooms 99231
 * thru 99236 are the 'eastern edge', and at most 1 (one) of them will hold
 * the 'entrance' to 99200.
 *
 * It's quite possible that ALL exits/entrances will be blocked, in which
 * case, anyone wanting to use them will just have to wait, until a mirroid
 * decides to unblock one.
 *
 * Mirroids are stay-zone and sentinel, this routine limits their movement to
 * the rooms 99201-99236.
 *
 * What makes this so devilishly complex, is we have may have to change up to
 * 5 dir_option[] entries, in up to 5 seperate rooms (2 pairs of maze rooms,
 * plus possibly an entrance/exit room) each time a mirroid (un)blocks an exit.
 *
 * 99200 dir 3 will always point to one of the rooms in range 99231-99236,
 * and 99237 dir 1 will always point to one of the rooms 99201-99206, and the
 * corresponding room will point back, even if the exit happens to be blocked.
 *
 * When we shift the exit, we have to restore the old loop, then kill then new
 * loop, then add the entrance/exit connection.
 */

int nw_mirroid(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   t_ch = NULL;
  bool     run_away = FALSE;
  int      mode = 0, i, e_count = 0, e_flag = -1, c_dir = -1, c_room, e_dir,
    e_room = 0;

  /*
   * check for periodic event calls
   */

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (pl || cmd || !IS_AWAKE(ch) || !CAN_ACT(ch))
    return FALSE;

  c_room = ch->in_room;

  /*
   * mode numbers: 0 - do nothing 1 - open an exit and stay (only way to open
   * an exit/entrance) 2 - open an exit and use it 3 - close an exit and stay
   * (only way to close an exit/entrance) 4 - use an exit and close it behind 5
   *
   * - use an exit and leave it unchanged
   */

  /*
   * count unblocked exits
   */

  for (i = 0; i < 4; i++)
    if (EXIT(ch, i))
    {
      if (!IS_SET(EXIT(ch, i)->exit_info, EX_BLOCKED))
        e_count++;

      if ((EXIT(ch, i)->to_room == real_room(99200)) ||
          (EXIT(ch, i)->to_room == real_room(99237)))
      {
        if (IS_SET(EXIT(ch, i)->exit_info, EX_BLOCKED))
          e_flag = 0;
        else
          e_flag = 1;
      }
    }
  if (IS_FIGHTING(ch))
  {
    run_away = TRUE;

    /*
     * preferable mode is 4, but failing that, we use 2
     */
    if (e_count)
      mode = 4;
    else
      mode = 2;
  }
  else
  {
    /*
     * we aren't fighting, so we check our parameters and pick a mode
     */

    /*
     * check for non-mirroids in our room
     */
    LOOP_THRU_PEOPLE(t_ch, ch)
      if ((IS_PC(t_ch) || (GET_RNUM(t_ch) != GET_RNUM(ch))) && CAN_SEE(ch, t_ch))
    {
      run_away = TRUE;
      break;
    }
    if (run_away)
    {
      /*
       * if there is an exit, mode 4, if not, mode 2
       */
      if (e_count)
        mode = 4;
      else
        mode = 2;
    }
  }

  if (!mode)
  {
    /*
     * ok, at this point, we aren't fighting, nor are there non-mirroids in our
     *
     * room (that we can see), so we can be a little more flexible.
     */

    switch (e_count)
    {
    case 0:
      /*
       * no exits, do we sit and think, or open a path?
       */
      if (number(0, 9))
      {
        if (number(0, 1))
          mode = 1;
        else
          mode = 2;
      }
      break;
    case 1:
      /*
       * one exit, really likely to want to open another, but possibly we
       * close it.
       */
      switch (number(0, 20))
      {
      case 0:
      case 1:
      case 2:
        break;
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
        mode = 1;
        break;
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        mode = 2;
        break;
      case 16:
      case 17:
        mode = 3;
        break;
      case 18:
      case 19:
        mode = 4;
        break;
      case 20:
        mode = 5;
        break;
      }
      break;
    case 2:
      /*
       * two exits, about equal probabilities on all options.
       */
      switch (number(0, 20))
      {
      case 0:
      case 1:
        break;
      case 2:
      case 3:
      case 4:
      case 5:
        mode = 1;
        break;
      case 6:
      case 7:
      case 8:
      case 9:
        mode = 2;
        break;
      case 10:
      case 11:
      case 12:
      case 13:
        mode = 3;
        break;
      case 14:
      case 15:
      case 16:
      case 17:
        mode = 4;
        break;
      case 18:
      case 19:
      case 20:
        mode = 5;
        break;
      }
      break;
    case 3:
      /*
       * three exits, more likely to close one than open a new one.
       */
      switch (number(0, 20))
      {
      case 0:
        break;
      case 1:
      case 2:
        mode = 5;
        break;
      case 3:
      case 4:
        mode = 1;
        break;
      case 5:
      case 6:
      case 7:
        mode = 2;
        break;
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
        mode = 3;
        break;
      case 14:
      case 15:
      case 16:
      case 17:
      case 18:
      case 19:
      case 20:
        mode = 4;
        break;
      }
      break;
    case 4:
      /*
       * four exits, REAL likely to close one
       */
      switch (number(0, 20))
      {
      case 0:
      case 1:
        break;
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
        mode = 3;
        break;
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
      case 16:
      case 17:
      case 18:
      case 19:
        mode = 4;
        break;
      case 20:
        mode = 5;
        break;
      }
      break;
    }
  }
  if (!mode)
    return TRUE;

  /*
   * quickie check on 'entrance' status
   */

  if (world[ch->in_room].number ==
      BOUNDED(99231, world[ch->in_room].number, 99236))
  {

    /*
     * we are in one of the rooms that an exit is legal from
     */
    e_dir = 1;
    for (i = 99231; i < 99237; i++)
      if (!IS_SET(EXIT(ch, 1)->exit_info, EX_BLOCKED) &&
          (EXIT(ch, 1)->to_room == real_room(99200)))
      {
        e_room = i;
        break;
      }
    if (e_room)
    {
      if (real_room(e_room) == ch->in_room)
        e_room = -1;
    }
    else
      e_room = -2;
  }
  else if (world[ch->in_room].number ==
           BOUNDED(99201, world[ch->in_room].number, 99206))
  {
    /*
     * we are in one of the rooms that an exit is legal from
     */
    e_dir = 3;
    for (i = 99201; i < 99207; i++)
      if (!IS_SET(EXIT(ch, 3)->exit_info, EX_BLOCKED) &&
          (EXIT(ch, 3)->to_room == real_room(99237)))
      {
        e_room = i;
        break;
      }
    if (e_room)
    {
      if (real_room(e_room) == ch->in_room)
        e_room = -1;
    }
    else
      e_room = -2;
  }
  /*
   * ok, e_room: -2 there is no exit, but we are in a room we can MAKE an exit
   * from. -1 there is an exit, and it's in our room. 0 we aren't in a position
   *
   * to change the exit. + there is an exit, it's not in our room, but we can
   * change it.
   */

  /*
   * at this point, we have a mode between 1 and 5, and we are aware of the
   * exit status, now we have to make some final sanity checks, and actually DO
   *
   * something.
   */

  switch (mode)
  {
  case 1:                      /*
                                 * open up and sit still
                                 */
  case 2:                      /*
                                 * open up and use it
                                 */
    if (e_room == -2)
    {
      /*
       * no entrance/exit, but we can make one, do so, 50%
       */
      if (number(0, 1))
        c_dir = e_dir;
    }
    else if (e_room > 0)
    {
      /*
       * there IS an exit, but we might prefer having it here, change it 1 in
       *
       * 5
       */
      if (!number(0, 4))
      {
        nw_reset_maze(e_room);
        c_dir = e_dir;
      }
    }
    if (c_dir < 0)
    {
      /*
       * we haven't found an appropriate direction yet.
       */

      for (c_dir = number(0, 3), i = 0; i < 4; i++, c_dir++)
      {
        if (c_dir > 3)
          c_dir = 0;

        if (IS_SET(EXIT(ch, c_dir)->exit_info, EX_BLOCKED))
          break;
      }
    }
    break;

  case 3:                      /*
                                 * close up and sit still
                                 */
  case 4:                      /*
                                 * use an exit and close it behind us
                                 */
  case 5:                      /*
                                 * just wander
                                 */

    for (c_dir = number(0, 3), i = 0; i < 4; i++, c_dir++)
    {
      if (c_dir > 3)
        c_dir = 0;

      if ((EXIT(ch, c_dir) &&
           !IS_SET(EXIT(ch, c_dir)->exit_info, EX_BLOCKED)))
      {
        if ((mode == 3) || (world[EXIT(ch, c_dir)->to_room].number ==
                            BOUNDED(99201,
                                    world[EXIT(ch, c_dir)->to_room].number,
                                    99236)))
          break;
      }
    }
    break;

  }

  if (c_dir == -1)
    return FALSE;

  if (mode < 3)
  {
    nw_block_exit(ch->in_room, c_dir, 0);       /*
                                                 * unblock
                                                 */
    if (mode == 2)
      do_move(ch, 0, exitnumb_to_cmd(c_dir));
  }
  else if (mode == 3)
  {
    nw_block_exit(ch->in_room, c_dir, 1);       /*
                                                 * block
                                                 */
  }
  else
  {
    int      old_room = ch->in_room;

    do_move(ch, 0, exitnumb_to_cmd(c_dir));
    if (mode == 4)
      nw_block_exit(old_room, c_dir, 1);        /*
                                                 * block
                                                 */
  }

  return TRUE;
}

int ghore_paradise(P_char ch, P_char pl, int cmd, char *arg)
{
  int      beforecash, aftercash;

  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;

  if (ch->in_room == real_room(11666))
  {
    if (cmd == CMD_UP)
    {
      if (IS_PC(pl))
      {
        act("$n stops you.", FALSE, ch, 0, pl, TO_VICT);
        act("$n stops $N.", TRUE, ch, 0, pl, TO_NOTVICT);
        mobsay(ch, "Paradise is off limits to freeloaders!");
        mobsay(ch, "The price to enter is 10 Ghorean platinum!");
        return TRUE;
      }
      else
      {
        act("$N gives $n some money.", TRUE, ch, 0, pl, TO_NOTVICT);

        mobsay(ch, "Welcome to the Paradise.");
        act("$n steps aside to let $N up the passage", TRUE, ch, 0, pl,
            TO_NOTVICT);
        char_from_room(pl);
        char_to_room(pl, real_room(11667), 0);
        act("$n arrives from the passage below.", TRUE, pl, 0, 0, TO_ROOM);
        return TRUE;
      }
    }
    else if (cmd == CMD_GIVE)
    {
      beforecash = GET_MONEY(ch);
      do_give(pl, arg, cmd);
      aftercash = GET_MONEY(ch);
      if ((aftercash - beforecash) < 10000)
      {
        mobsay(ch, "The cost is 10 Ghorean platinum, worm!  Try again.");
        do_action(ch, 0, CMD_SPIT);
        return TRUE;
      }
      else
      {
        mobsay(ch, "Welcome to Paradise.");
        act("$n steps aside to let you continue up the passage.", FALSE, ch,
            0, pl, TO_VICT);
        act("$n steps aside to let $N further up the passage.", TRUE, ch, 0,
            pl, TO_NOTVICT);
        char_from_room(pl);
        char_to_room(pl, real_room(11667), 0);
        act("$n arrives from the passage below.", TRUE, pl, 0, 0, TO_ROOM);
        return TRUE;
      }
    }
    else if (cmd)
    {
      return FALSE;
    }
    else
    {
      switch (number(0, 80))
      {
      case 0:
        mobsay(ch, "Welcome to Paradise!");
        do_action(ch, 0, CMD_CACKLE);
        return TRUE;
      case 1:
        do_action(ch, 0, CMD_SING);
        mobsay(ch, "Gimme lots of money, or I'm gonna eat you....");
        return TRUE;
      case 2:
        mobsay(ch, "I will not let you pass unless you pay the tribute!");
        return TRUE;
      case 3:
        mobsay(ch, "Paradise lies ahead...");
        do_action(ch, 0, CMD_SMILE);
        return TRUE;
      case 4:
        act("$n holds out $s hand for some coins.", TRUE, ch, 0, 0, TO_ROOM);
        return TRUE;
      case 5:
        mobsay(ch, "Pay now, or die now; 'tis a simple choice, is it not?");
        do_action(ch, 0, CMD_CACKLE);
      default:
        return FALSE;
      }
    }
  }
  else
  {
    return FALSE;
  }
}

/*
 * combat special for warhorses
 */
int warhorse(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict, rider;
  int      i;
  bool     dropped_through = FALSE;

  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd || !IS_FIGHTING(ch) || !CAN_ACT(ch))
    return FALSE;

  /*
   * warhorses have 2 special attack forms, first is overbearing and trample,
   * this will work on everything smaller than dragons or demons.  This attack
   * works whether warhorse has a rider or not.  Second attack is a mule kick,
   * this attack work on everything, but doesn't work very well if the warhorse
   *
   * has a rider.
   */
  rider = get_linking_char(ch, LNK_RIDING);
  vict = ch->specials.fighting;

  switch (number(1, 3))
  {
  case 1:                      /*
                                 * overbear and trample
                                 */
    if (IS_DEMON(vict) || IS_DRAGON(vict))
      return FALSE;

    if ((world[ch->in_room].sector_type != SECT_FIREPLANE) &&
        (world[ch->in_room].sector_type > SECT_WATER_SWIM))
    {
      send_to_char("You have no footing here!\r\n", ch);
      return FALSE;
    }
    CharWait(ch, PULSE_VIOLENCE);
    if (!StatSave(vict, APPLY_AGI, (GET_LEVEL(vict) - GET_LEVEL(ch)) / 3))
    {
      /*
       * a hit! basically does about what a bodyslam would, but not quite as
       * bad
       */
      if (GET_POS(vict) == POS_STANDING)
      {
        act("$n charges $N and knocks $M flat!", FALSE, ch, 0, vict,
            TO_NOTVICT);
        act("$n charges you and knocks you flat!", FALSE, ch, 0, vict,
            TO_VICT);
        act("You charge $N and knock $M flat!", FALSE, ch, 0, vict, TO_CHAR);
        CharWait(vict, PULSE_VIOLENCE * 3);
        if (!damage
            (ch, vict, str_app[STAT_INDEX(GET_C_STR(ch))].todam + 5,
             SKILL_BASH))
        {
          SET_POS(vict, POS_PRONE + GET_STAT(vict));
          if (!number(0, 2))
            Stun(vict, ch, number(PULSE_VIOLENCE, PULSE_VIOLENCE * 5 / 2), TRUE);
        }
        else
        {
          return TRUE;
        }
      }
      else
      {
        act("$n charges $N!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n charges you!", FALSE, ch, 0, vict, TO_VICT);
        act("You charge $N!", FALSE, ch, 0, vict, TO_CHAR);
      }
      act("$n then starts a deadly little dance on $N's prone form!", FALSE,
          ch, 0, vict, TO_NOTVICT);
      act("$n then starts a deadly little dance on your tender body!", FALSE,
          ch, 0, vict, TO_VICT);
      act("Then you start a deadly little dance on $N's prone form!", FALSE,
          ch, 0, vict, TO_CHAR);

      /*
       * up to 3 'normal' attacks
       */
      for (i = 1; i < 4; i++)
      {
        if (!IS_AWAKE(vict) || !StatSave(vict, APPLY_AGI, -2))
        {
          if( damage(ch, vict, ( dice(ch->points.damnodice, ch->points.damsizedice) + TRUE_DAMROLL(ch) ),
            TYPE_UNDEFINED) )
          {
            break;
          }
        }
      }
      return TRUE;
      break;
    }
    else
    {
      if (rider || number(0, 4) || StatSave(ch, APPLY_AGI, -2))
      {
        act("$n charges toward $N, who quickly dodges aside!", FALSE, ch, 0,
            vict, TO_NOTVICT);
        act("$n charges you, but you manage to scramble out of the way!",
            FALSE, ch, 0, vict, TO_VICT);
        act("You charge $N, but $E dodges away!", FALSE, ch, 0, vict,
            TO_CHAR);
        return TRUE;
      }
      /*
       * Muhahaha, you thought you escaped!  Drops through to mulekick
       * section!
       */
      act
        ("As $N dodges $n's charge, $n whirls, and both of $s rear feet connect with crushing force!",
         FALSE, ch, 0, vict, TO_NOTVICT);
      act
        ("You dodge a charge from $n, only to be slammed with both of $s rear feet!",
         FALSE, ch, 0, vict, TO_VICT);
      act
        ("$N dodges your charge, you whirl around and connect with both rear feet!",
         FALSE, ch, 0, vict, TO_CHAR);
      dropped_through = TRUE;
    }

  case 2:                      /*
                                 * mulekick
                                 */
    if (!dropped_through)
    {
      if (rider && number(0, 4))
        return FALSE;
      if (StatSave(vict, APPLY_AGI, -2))
      {
        act("$n's rear feet whistle over $N's head, missing by scant inches!",
            FALSE, ch, 0, vict, TO_NOTVICT);
        act
          ("$n's rear feet whistle past your scalp, as you duck frantically!",
           FALSE, ch, 0, vict, TO_VICT);
        act("Your feet whistle over $N's head, missing by scant inches!",
            FALSE, ch, 0, vict, TO_CHAR);
        CharWait(ch, PULSE_VIOLENCE);
        return TRUE;
      }
      else
      {
        act
          ("$n lashes out with both rear feet, they connect with $N, generating meaty THUDS!",
           FALSE, ch, 0, vict, TO_NOTVICT);
        act
          ("$n kicks you with both rear feet, sending you back gasping in pain!",
           FALSE, ch, 0, vict, TO_VICT);
        act("Your kicks connect with $N, generating meaty THUDS!", FALSE, ch,
            0, vict, TO_CHAR);
      }
    }
    /*
     * ok, do the deed, VERY high chance of stunning victim in this case, in
     * addition to nasty damage!
     */
    if (damage(ch, vict, GET_LEVEL(ch), TYPE_UNDEFINED))
      return TRUE;
    if (!number(0, 10))
      Stun(vict, ch, PULSE_VIOLENCE * 2, FALSE);
    update_pos(vict);
    if (IS_AWAKE(vict))
      CharWait(vict, 2 * PULSE_VIOLENCE);
    CharWait(ch, PULSE_VIOLENCE);

    break;
  default:                     /*
                                 * do nothing special
                                 */
    return FALSE;
    break;
  }
  return FALSE;
}

int water_elemental(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict = NULL, next_ch;
  bool     found = FALSE;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd || pl || !CAN_ACT(ch))
    return FALSE;

  /*
   * ok, basically, this is a area bash attack, but not quite
   */

/*  LOOP_THRU_PEOPLE(vict, ch) {*/
  for (vict = world[ch->in_room].people; vict; vict = next_ch)
  {
    next_ch = vict->next_in_room;

    if (!should_area_hit(ch, vict))
      continue;

    if (!found)
    {
      found = TRUE;
      act("$n shimmers and the waters of the pool lift in a towering wall!",
          TRUE, ch, 0, 0, TO_ROOM);
    }
    CharWait(vict, PULSE_VIOLENCE * 2);
    SET_POS(vict, POS_PRONE + GET_STAT(vict));
    if (GET_RACE(vict) == RACE_F_ELEMENTAL)
    {
      act("$N vanishes in a cloud of steam!", FALSE, ch, 0, vict, TO_NOTVICT);
      act
        ("A wall of water crashes on top of you, you feel your lifefires being quenched!",
         FALSE, ch, 0, vict, TO_VICT);
      damage(ch, vict, dice(10, 6), TYPE_UNDEFINED);
    }
    else
    {
      act("A wave crashes over you, pounding you into the ground!", FALSE, ch,
          0, vict, TO_VICT);
      damage(ch, vict, dice(1, 5), TYPE_UNDEFINED);
    }
/*    update_pos(vict);*/
  }

  if (found)
    return TRUE;

  return FALSE;
}

int ice_snooty_wife(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    mobsay(ch, "Have you met my husband?");
    mobsay(ch, "He's the dashingly handsome one over there!");
    do_action(ch, 0, CMD_WINK);
    return TRUE;
  case 2:
    act("The garishly dressed woman spills her drink on the floor.",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_GIGGLE);
    mobsay(ch, "Oops!");
    mobsay(ch,
           "Mop boy!! My shoes are all wet! Clean them off this instant!");
    do_action(ch, 0, CMD_WHATEVER);
    mobsay(ch, "You just can't find good help these days.");
    do_action(ch, 0, CMD_SIGH);
    return TRUE;
  }
  return FALSE;
}

int ice_cleaning_crew(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    act("The humble member of the cleaning crew quietly sweeps the floors.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_SNEEZE);
    do_action(ch, 0, CMD_COUGH);
    mobsay(ch, "Does this winter never end?");
    do_action(ch, 0, CMD_SIGH);
    mobsay(ch, "These sniffles will be the end of me!");
    do_action(ch, 0, CMD_GRUMBLE);
    return TRUE;
  }
  return FALSE;
}

int ice_artist(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    mobsay(ch, "NO NO NO! This is all wrong!");
    mobsay(ch, "I specifically asked for SOLID ice, not a block of frost!");
    do_action(ch, 0, CMD_WHATEVER);
    return TRUE;
  case 2:
    mobsay(ch,
           "There must be a way to emphasize the solitude without overpowering sorrow.");
    do_action(ch, 0, CMD_ARCH);
    mobsay(ch,
           "Yes! That's it! I'll use the negative space to create a plane of emotion heretofore unknown in human art! I'll be the toast of Verzanan!");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  }
  return FALSE;
}

int ice_privates(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    mobsay(ch, "Guests must stay outside of restricted areas!");
    mobsay(ch, "The Icess will not tolerate infractions of the rules!");
    do_action(ch, 0, CMD_PEER);
    do_action(ch, 0, CMD_GLARE);
    return TRUE;
  case 2:
    act("The private looks you over, sizing up your capabilities.", TRUE,
        ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_CHUCKLE);
    mobsay(ch,
           "Keep your nose out of restricted areas and we won't have a problem.");
    do_action(ch, 0, CMD_PEER);
    return TRUE;
  }
  return FALSE;
}

int ice_masha(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;

  if ((cmd == CMD_GET) && pl && (pl != ch) &&
      (isname("onion", arg) || isname("all", arg)))
  {
    mobsay(ch, "Unhand my onions, fiend!");
    bash(ch, pl);
    return TRUE;
  }
  if (cmd || IS_FIGHTING(ch))
    return FALSE;

  switch (number(1, 10))
  {
  case 1:
    mobsay(ch, "You have to slice the onions just perfectly...");
    act("Masha skillfully dices his onions, and pieces fly everywhere.", TRUE,
        ch, 0, 0, TO_ROOM);
    mobsay(ch, "Theres nothing quite like the smell onions in the morning.");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  }

  return FALSE;
}

int ice_tubby_merchant(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    mobsay(ch, "Have you tried the cavier?  It's simply fabulous!");
    act
      ("The portly merchant dribbles wine down his shirt as he goes for another swig.",
       TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_SMIRK);
    return TRUE;
  case 2:
    mobsay(ch,
           "Where is my wife? She's the one in the dreadful Calimshan garb, looks like a piece of fruit with wrinkles.");
    do_action(ch, 0, CMD_ROFL);
    return TRUE;
  case 3:
    mobsay(ch, "So can I ask where your buying your supplies?");
    mobsay(ch,
           "I can offer you a sweet deal on dried goods and non-perishables.");
    do_action(ch, 0, CMD_WINK);
    mobsay(ch, "Pardon me while I refresh my drunk.");
    do_action(ch, 0, CMD_GIGGLE);
    return TRUE;
  }
  return FALSE;
}

int ice_priest(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    mobsay(ch, "Have you seen my speech notes?");
    mobsay(ch, "Aha! My notes! Now lets see..where was I...");
    mobsay(ch,
           " Yes, here we are, I am very grateful for this oppurtunity...");
    mobsay(ch, "I'd like to thank all my...no no no thats too cliche");
    do_action(ch, 0, CMD_PONDER);
    return TRUE;
  case 2:
    mobsay(ch, "How much time have I to prepare before the banquet begins?");
    mobsay(ch, "I better read over those notes I prepared.");
    do_action(ch, 0, CMD_THINK);
    do_action(ch, 0, CMD_FROWN);
    do_action(ch, 0, CMD_SCRATCH);
    return TRUE;
  }
  return FALSE;
}

int ice_garden_attendant(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 30))
  {
  case 1:
    act("The attendant meekly sweeps snow from the path.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_SPIT);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_SHIVER);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_COUGH);
    return TRUE;
  case 5:
    do_action(ch, 0, CMD_SNEEZE);
    return TRUE;
  }
  return FALSE;
}

int ice_raucous_guest(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 8))
  {
  case 1:
    mobsay(ch, "So I'm talking to this haughty Elf from Luethilspar...");
    mobsay(ch, "And all he can talk about is the Kobold situation!");
    act
      ("The guest of the castle throws his hands up in disgust with the whole situation.",
       TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch,
           "Let me tell you this joke I heard from Lord Piergeron on my last visit to Verzanan.  Did I tell you we're personal friends?");
    mobsay(ch,
           "Yes yes yes, me and his Lordship go way back!  We used to go on safari in our younger days, a mighty good shot with a bow that one is.");
    return TRUE;
  case 3:
    mobsay(ch, "At any rate, back to the joke.");
    act
      ("The raucous guest goes on with some rather uneventful tale about a dwarf in disguise as an elf in the city of Sylvandawn.",
       0, ch, 0, 0, TO_ROOM);
    mobsay(ch, "So the dwarf says to the elf, 'I don't drink!'");
    do_action(ch, 0, CMD_ROFL);
    return TRUE;
  }
  return FALSE;
}

int ice_tar(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 40))
  {
  case 1:
  case 2:
  case 3:
  case 4:
    act("Tar accidentally crushes another potatoe.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 5:
    do_action(ch, 0, CMD_CURSE);
    return TRUE;
  case 6:
    do_action(ch, 0, CMD_SCREAM);
    return TRUE;
  }
  return FALSE;
}

int ice_commander(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    mobsay(ch, "Where the bloody hell did that blasted book get to...");
    mobsay(ch, "That ragged old ancient lookin one...");
    mobsay(ch, "I know I left it around here somewhere...");
    do_action(ch, 0, CMD_SCRATCH);
    do_action(ch, 0, CMD_PONDER);
    return TRUE;
  case 2:
    act
      ("The commander begins to search the room, rifling through bookcases, cabinets, his desk drawers, and virtually every container in the room.",
       TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_BOGGLE);
    do_action(ch, 0, CMD_SHRUG);
    mobsay(ch, "Guess it'll show up eventually.");
    return TRUE;
  }
  return FALSE;
}

int ice_viscount(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    mobsay(ch,
           "Consumption of dry goods is up this month, going to have to cut back.");
    mobsay(ch, "Where are all these expenses coming from?");
    do_action(ch, 0, CMD_BOGGLE);
    return TRUE;
  case 2:
    mobsay(ch,
           "Im going to have to speak with Strife about these rising costs.");
    mobsay(ch, "Does she think she can take over Faerun for free?");
    do_action(ch, 0, CMD_BOGGLE);
    mobsay(ch,
           "We're going to have to schedule at least twice the number of current raids on surrounding villages and towns if we even hope to come close to our goal.");
    return TRUE;
  }
  return FALSE;
}

int ice_masonary_crew(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 3))
  {
  case 1:
    act("The mason arbitrarily slaps some grout on the wall.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act("The craftsman starts pressing tiles into the wet cement.", TRUE,
        ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}

int ice_impatient_guest(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(1, 15))
  {
  case 1:
    mobsay(ch, "Shouldn't the banquet have started by now?!");
    mobsay(ch, "Im really getting tired of waiting, it's been forever!");
    mobsay(ch, "What's the hold up? Can't you people move with purpose?");
    do_action(ch, 0, CMD_WHATEVER);
    do_action(ch, 0, CMD_TWIDDLE);
    return TRUE;
  case 2:
    mobsay(ch, "If I don't see some action in five minutes, Im leaving!");
    mobsay(ch,
           "I have better things to be doing than sit around in some frozen castle waiting for a banquet!");
    do_action(ch, 0, CMD_WHINE);
    return TRUE;
  }
  return FALSE;
}

int ice_privates2(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   leader;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd || ch->following)
    return FALSE;

  LOOP_THRU_PEOPLE(leader, ch)
  {
    if (IS_NPC(leader) && (GET_VNUM(leader) == (97020)) &&
        ((GET_VNUM(ch) == (97019)) || (GET_VNUM(ch) == (97018))))
    {
      add_follower(ch, leader); /*
                                 * Follow and assist leader
                                 */
      group_add_member(leader, ch);
    }
  }
  return FALSE;
}

int ice_bodyguards(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   blockee;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || cmd || pl || !IS_AWAKE(ch))
    return FALSE;

  LOOP_THRU_PEOPLE(blockee, ch)
    if (IS_NPC(blockee) &&
        (((GET_VNUM(blockee) == (97023)) &&
          (GET_VNUM(ch) == (97040))) ||
         ((GET_VNUM(blockee) == (97029)) &&
          (GET_VNUM(ch) == (97041))) ||
         ((GET_VNUM(blockee) == (97008)) &&
          (GET_VNUM(ch) == (97042)))))
  {
    if (NumAttackers(blockee))
    {
      rescue(ch, blockee, FALSE);
      return TRUE;
    }
  }
  return FALSE;
}

int ice_wolf(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   i, i_next, tempchar = NULL, tempchar2 = NULL, was_fighting = NULL;
  P_desc   d;
  P_obj    item, next_item;
  int      pos;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch))
    return FALSE;

  /*
   * if it's some command besides a periodic event call, return
   */
  if (cmd)
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    was_fighting = ch->specials.fighting;
    stop_fighting(ch);

    tempchar = read_mobile(97054, VIRTUAL);

    if (!tempchar)
    {
      logit(LOG_EXIT, "assert: mob load failed in ice_wolf()");
      raise(SIGSEGV);
    }
    char_to_room(tempchar, ch->in_room, -2);
    for (item = ch->carrying; item; item = next_item)
    {
      next_item = item->next_content;
      obj_from_char(item);
      obj_to_char(item, tempchar);      /*
                                         * transfer any eq and inv
                                         */
    }
    for (pos = 0; pos < MAX_WEAR; pos++)
    {
      if (ch->equipment[pos] != NULL)
      {
        item = unequip_char(ch, pos);
        equip_char(tempchar, item, pos, TRUE);
      }
    }

    act("The $n suddenly drops to the floor, howling in pain!", 0, ch, 0, 0,
        TO_ROOM);
    act("A moment later, $e trasforms into $N!", 1, ch, 0, tempchar, TO_ROOM);
    act("$n throws back $s head, and lets out a long howl.", 0, tempchar, 0,
        0, TO_ROOM);

    extract_char(ch);
    ch = NULL;

    /*
     * Howl for help, similar to echoz
     */
    for (d = descriptor_list; d; d = d->next)
    {
      if (d->connected == CON_PLYNG)
      {
        if (world[tempchar->in_room].zone ==
            world[d->character->in_room].zone)
        {
          send_to_char("A bloodcurdling howl is heard!", d->character);
          send_to_char("\r\n", d->character);
        }
      }
    }

    /*
     * Assistants in other room change now, and begin to come help
     */
    for (i = character_list; i; i = i_next)
    {
      i_next = i->next;
      if (IS_NPC(i) &&
          ((GET_VNUM(i) == 97031) ||
           (GET_VNUM(i) == 97032)))
      {
        tempchar2 = read_mobile(97055, VIRTUAL);
        if (!tempchar2)
        {
          logit(LOG_EXIT, "assert: second mob load failed in ice_wolf()");
          raise(SIGSEGV);
        }
        act("The $n suddenly drops to the floor, howling in pain!", 0, i, 0,
            0, TO_ROOM);
        act("A moment later, $e trasforms into $N!", 1, i, 0, tempchar2,
            TO_ROOM);
        act("$n throws back $s head, and lets out a long howl.", 0, tempchar2,
            0, 0, TO_ROOM);

        char_to_room(tempchar2, i->in_room, -2);
        if (!IS_SET(tempchar2->specials.act, ACT_HUNTER))
          SET_BIT(tempchar2->specials.act, ACT_HUNTER);

        for (item = i->carrying; item; item = next_item)
        {
          next_item = item->next_content;
          obj_from_char(item);
          obj_to_char(item, tempchar2);
        }
        for (pos = 0; pos < MAX_WEAR; pos++)
        {
          if (i->equipment[pos] != NULL)
          {
            item = unequip_char(i, pos);
            equip_char(tempchar2, item, pos, TRUE);
          }
        }

        /*
         * Code for memory (from set_fighting), this will make the converted
         * werewolves hunt.
         */

        if (tempchar2 && was_fighting)
        {
          if (HAS_MEMORY(tempchar2))
          {
            if (IS_PC(was_fighting))
            {
              if (!
                  (IS_TRUSTED(was_fighting) &&
                   IS_SET(was_fighting->specials.act, PLR_AGGIMMUNE)))
                if ((GET_STAT(tempchar2) > STAT_INCAP))
                  remember(tempchar2, was_fighting);
            }
            else if (IS_PC_PET(was_fighting) &&
                     (GET_MASTER(was_fighting)->in_room ==
                      was_fighting->in_room) &&
                     CAN_SEE(tempchar2, GET_MASTER(was_fighting)))
            {
              if (!(IS_TRUSTED(GET_MASTER(was_fighting)) &&
                    IS_SET(GET_MASTER(was_fighting)->specials.act,
                           PLR_AGGIMMUNE)))
                if ((GET_STAT(tempchar2) > STAT_INCAP))
                  remember(tempchar2, was_fighting->following);
            }
          }
        }
        extract_char(i);        /*
                                 * Set them hunting players, wherever they may
                                 * be
                                 */
      }
    }

    if (was_fighting)
      MobStartFight(tempchar, was_fighting);

    return TRUE;
  }
  return FALSE;
}

int ice_malice(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vapor, hated_one, next;
  P_obj    item, next_item;
  int      pos;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == CMD_DEATH)
  {                             /*
                                 * special die aspect
                                 */
    vapor = read_mobile(97056, VIRTUAL);
    if (!vapor)
    {
      logit(LOG_EXIT, "assert: mob load failed in ice_malice()");
      raise(SIGSEGV);
    }
    char_to_room(vapor, ch->in_room, 0);

    for (item = ch->carrying; item; item = next_item)
    {
      next_item = item->next_content;
      obj_from_char(item);
      obj_to_char(item, vapor); /*
                                 * transfer any eq and inv
                                 */
    }
    for (pos = 0; pos < MAX_WEAR; pos++)
    {
      if (ch->equipment[pos] != NULL)
      {
        item = unequip_char(ch, pos);
        equip_char(vapor, item, pos, TRUE);
      }
    }

    return TRUE;
  }
  if (!ch || !IS_AWAKE(ch) || !IS_FIGHTING(ch) || cmd || !CAN_ACT(ch))
    return FALSE;

  /*
   * Malice hates clerics, and will always target them
   */

  if (IS_FIGHTING(ch) && NumAttackers(ch) > 1)
    for (hated_one = world[ch->in_room].people; hated_one; hated_one = next)
    {
      next = hated_one->next_in_room;
      if ((ch == hated_one->specials.fighting) && IS_CLERIC(hated_one))
        /*
         * A cleric is fighting him
         */
        if ((hated_one != ch->specials.fighting) && CAN_SEE(ch, hated_one))
        {
          /*
           * But, he is not targeting them...
           */
          attack(ch, hated_one);
          return TRUE;
        }
    }
  return FALSE;
}

/* Negative Material */

int neg_pocket(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict, next;
  int      dam;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == CMD_DEATH)
  {                             /*
                                 * explode upon death
                                 */
    act("$n &N&+LEXPLODES, engulfing the area in a dark layer of death!", 0,
        ch, 0, 0, TO_ROOM);
    for (vict = world[ch->in_room].people; vict; vict = next)
    {
      next = vict->next_in_room;
      if ((ch == vict) || IS_TRUSTED(vict) || IS_NPC(vict))
        continue;

      dam = 250;

      if ((GET_HIT(vict) - dam) < -10)
      {
        act("&+LYou are engulfed into the darkness!&N", FALSE, ch, 0, 0,
            TO_CHAR);
        act("&+L$n&+L is engulfed completely by the darkness!&N", TRUE, ch, 0,
            0, TO_ROOM);
        logit(LOG_DEATH, "%s died from neg_pocket() explosion in room %d.",
              GET_NAME(vict), world[vict->in_room].number);
        die(vict, ch);
      }
      else
        GET_HIT(vict) -= dam;
    }
    return TRUE;
  }
  return FALSE;
}


int jotun_thrym(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict;
  struct affected_type af;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd)
    return FALSE;

  if (ch && IS_FIGHTING(ch))
    if (!number(0, 2))
    {
      vict = ch->specials.fighting;
      if (!vict)
        return FALSE;

      act
        ("&+BA blue bolt of energy streaks from&n $N's&n&+B hands, encasing&n $n &n&+Bin a solid block of ice!",
         0, vict, 0, ch, TO_NOTVICT);
      act
        ("&+BA blue bolt of energy streaks from&n $n's&n&+B hands, encasing you in a solid block of ice!",
         0, ch, 0, vict, TO_VICT);
      act
        ("&+BA blue bolt of energy streaks from your hands, encasing&n $N &+Bin a solid block of ice!",
         0, ch, 0, vict, TO_CHAR);

      /*
       * Shut em down!
       */

      StopCasting(vict);
      if (IS_FIGHTING(vict))
        stop_fighting(vict);
      if( IS_DESTROYING(vict) )
        stop_destroying(vict);
      bzero(&af, sizeof(af));
      af.type = SPELL_MAJOR_PARALYSIS;
      af.flags = AFFTYPE_SHORT;
      af.duration = 120 * WAIT_SEC;
      af.bitvector2 = AFF2_MAJOR_PARALYSIS;
      affect_to_char(vict, &af);
      CharWait(vict, af.duration);

      return TRUE;
    }
  return FALSE;
}

int jotun_utgard_loki(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict, next;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd)
    return FALSE;

  if (IS_FIGHTING(ch) && !number(0, 2))
  {
    act("$n &N&+Lcalls forth visions of immense horror!", 0, ch, 0, 0,
        TO_ROOM);
    for (vict = world[ch->in_room].people; vict; vict = next)
    {
      next = vict->next_in_room;
      if (!IS_GIANT(vict) && (vict != ch))
      {
        if (GET_LEVEL(vict) < 19)
        {                       /*
                                 * 20 and below, see ya...
                                 */
          do_flee(vict, 0, 2);
          act("&+LThe fear of it all overwhelms you!", 0, vict, 0, 0,
              TO_VICT);
        }
        if (GET_LEVEL(vict) < 31)       /*
                                         * 21-30, slight chance
                                         */
          if (!NewSaves(vict, SAVING_FEAR, -2) && !fear_check(vict))
          {
            do_flee(vict, 0, 2);
            act("&+LThe fear of it all overwhelms you!", 0, vict, 0, 0,
                TO_VICT);
          }
        if (GET_LEVEL(vict) <= MAXLVLMORTAL)     /*
                                                 * 31-56, good chance of staying
                                                 */
          if (!NewSaves(vict, SAVING_FEAR, 0) && !fear_check(vict))
          {
            do_flee(vict, 0, 1);
            act("&+LThe fear of it all overwhelms you!", 0, vict, 0, 0,
                TO_VICT);
          }
        if (ch->in_room != vict->in_room)
          if (IS_FIGHTING(vict))
            stop_fighting(vict);
      }
      return TRUE;
    }
  }
  return FALSE;
}

int jotun_balor(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict, next;
  int      dam;
  struct affected_type af;


  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == CMD_DEATH)
  {                             /*
                                 * explode upon death
                                 */
    act("$n &N&+rEXPLODES in a mass of fire and energy!", 0, ch, 0, 0,
        TO_ROOM);
    for (vict = world[ch->in_room].people; vict; vict = next)
    {
      next = vict->next_in_room;
      if ((ch == vict) || IS_TRUSTED(vict))
        continue;

      if (!IS_AFFECTED(vict, AFF_BLIND) && IS_AFFECTED(ch, AFF_INFRAVISION))
      {
        bzero(&af, sizeof(af));
        if (vict->in_room != NOWHERE)
          send_to_char("Aaarrrggghhh!!  The heat blinds you!!\n", ch);
        blind(ch, vict, 60 * WAIT_SEC);
      }

      if (IS_AFFECTED(vict, AFF_PROT_FIRE))
        dam = 150;              /*
                                 * Allow a slight help, but not just fire
                                 */
      else
        dam = 250;              /*
                                 * so skip all the elemental type checks
                                 */
      if ((GET_HIT(vict) - dam) < -10)
      {
        act("Your wounds prove too much for you!", FALSE, ch, 0, 0, TO_CHAR);
        act("$n's wounds prove too much for $m!", TRUE, ch, 0, 0, TO_ROOM);
        logit(LOG_DEATH, "%s died from jotun_balor() explosion in room %d.",
              GET_NAME(vict), world[vict->in_room].number);
        die(vict, ch);
      }
      else
        GET_HIT(vict) -= dam;
    }
    return TRUE;
  }
  return FALSE;
}

int jotun_mimer(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Gbuf1[MAX_STRING_LENGTH];

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;

  if ((ch->in_room != real_room(GET_BIRTHPLACE(ch))) ||
      (ch->in_room == NOWHERE))
  {
    if (!IS_AWAKE(ch) || IS_FIGHTING(ch))
      return FALSE;
    act("$N looks around frantically, then vanishes in a small puff of smoke",
        FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(GET_BIRTHPLACE(ch)), -1);
    return FALSE;
  }
  if (pl && cmd)
  {
    if (cmd == CMD_WEST)
    {
      if (GET_LEVEL(pl) < 51 && !IS_GIANT(pl))
        mobsay(ch, "None but giants may pass through to my well.");
      else
      {
        sprintf(Gbuf1, "$N bows before you, saying 'Right this way, My %s'",
                (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
        act(Gbuf1, FALSE, pl, 0, ch, TO_CHAR);
        sprintf(Gbuf1, "$N bows before $n, saying 'Right this way, My %s'",
                (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
        act(Gbuf1, FALSE, pl, 0, ch, TO_ROOM);
        return FALSE;
      }
      return TRUE;
    }
  }
  return FALSE;
}

#define NUM_ARCHERS      21     /*
                                 * # of rooms archers can shoot from
                                 */
#define NUM_TARGETS      3      /*
                                 * # of rooms an archer can shoot at
                                 */
#define HIT_CHANCE       30     /*
                                 * accuracy 30% chance to hit
                                 */
#define ARCHER_NUM_DICE  2      /*
                                 * archer damage dice
                                 */
#define ARCHER_SIZE_DICE 5      /*
                                 * archer does 2d5 each hit
                                 */

int archer(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   targ;
  int      i, j, k, gottem = FALSE;
  char     buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
  char     buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];

  int      to_from_rooms[NUM_ARCHERS][NUM_TARGETS + 1] = {
    /*
     * archer room     target room #1     #2       #3
     */
    {95518, 95509, 95508, 95507},
    {95518, 95512, 95509, 95510},
    {95522, 95509, 95508, 95507},
    {95522, 95512, 95509, 95510},
    {95516, 95512, 95510, 95511},
    {95516, 95512, 95510, 95511},
    {66005, 66095, 66130, 66129},
    {8087, 3651, 3650, 3649},
    {8213, 8214, 8215, 8216},
    {17367, 17024, 17023, 17022},
    {17366, 17024, 17023, 17022},
    {17052, 17021, 17022, 17032},
    {17053, 17021, 17022, 17032},
    {17054, 17021, 17022, 17032},
    {17272, 17030, 17031, 17032},
    {17273, 17030, 17031, 17032},
    {75081, 75076, 75075, -1},
    {75088, 75087, 75084, -1},
    {75094, 75093, 75095, -1},
    {77231, 77230, 77229, 77225},
	{77237, 77234, 77232, 77233}
  };

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_STANDING)
    return FALSE;

  sprintf(buf, "You feel a sharp pain in your side as an arrow finds its mark!");
  sprintf(buf1, "You hear a dull thud as an arrow pierces $n!");
  sprintf(buf2, "An arrow whistles by your ear, barely missing you!");
  sprintf(buf3, "An arrow narrowly misses $n!");

  for (i = 0; i < NUM_ARCHERS; i++)
  {
    if (real_room(to_from_rooms[i][0]) == ch->in_room)
    {
      for (j = 1; j <= NUM_TARGETS; j++)
      {
        if ((k = real_room(to_from_rooms[i][j])) >= 0)
        {
          for (targ = world[k].people; targ; targ = targ->next_in_room)
          {
            if (is_aggr_to(ch, targ))
            {
              if (number(1, 100) <= HIT_CHANCE)
              {
                act(buf, 1, ch, 0, targ, TO_VICT);
                act(buf1, 1, targ, 0, 0, TO_NOTVICT);
                if (!IS_TRUSTED(targ))
                  GET_HIT(targ) -= dice(ARCHER_NUM_DICE, ARCHER_SIZE_DICE);
                if (number(1, 100) < (HIT_CHANCE / 4))
                {
                  GET_HIT(targ) -= ((GET_HIT(targ) / 10) * 8);
                  send_to_char("The arrow pierces extremely deep!\r\n", targ);
                }
                if (GET_HIT(targ) < -10)
                {
                  send_to_char
                    ("Alas, your wounds prove too much for you...\r\n", targ);
                  die(targ, ch);
                  return TRUE;
                }
                StartRegen(targ, EVENT_HIT_REGEN);
                update_pos(targ);
                gottem = TRUE;
              }
              else
              {
                act(buf2, 1, ch, 0, targ, TO_VICT);
                act(buf3, 1, targ, 0, 0, TO_NOTVICT);
              }
            }
          }
        }
      }
    }
  }
  return gottem;
}

int citizenship(P_char ch, P_char pl, int cmd, char *arg)
{
  char     Gbuf1[MAX_STRING_LENGTH];
  int      i;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (pl)
  {
    i = CHAR_IN_TOWN(ch);
    if (IS_TOWN_INVADER(pl, i))
      return FALSE;
    if (cmd == CMD_BUY)
    {                           /*
                                 * Buy
                                 */
      one_argument(arg, Gbuf1);
      if (!*Gbuf1)
        return FALSE;
      if (!str_cmp(Gbuf1, "visa"))
      {
        if (i == zone_table[world[GET_BIRTHPLACE(pl)].zone].hometown)
        {
          mobsay(ch, "Now why would you want a visa to your own hometown?");
        }
        else if (PC_TOWN_JUSTICE_FLAGS(pl, i) != JUSTICE_IS_CITIZEN)
        {
          mobsay(ch, "Sorry, visas are not granted to non citizens.");
        }
        else if (transact(pl, NULL, ch, 10000))
        {
          find_starting_location(pl,
                                 zone_table[world[ch->in_room].zone].
                                 hometown);
          GET_BIRTHPLACE(pl) = GET_HOME(pl);
          mobsay(ch,
                 "Okay, temporary residency granted. Obey all our laws, or face our justice!");
        }
        else
          mobsay(ch, "Visas are 10 platinum. Nothings free around here!");
        return TRUE;
      }
      else if (!str_cmp(Gbuf1, "citizenship"))
      {
        if (PC_TOWN_JUSTICE_FLAGS(pl, i) == JUSTICE_IS_CITIZEN)
        {
          mobsay(ch,
                 "Now why would you want to purchase that when you're already a citizen?");
        }
        else if (PC_TOWN_JUSTICE_FLAGS(ch, i) == JUSTICE_IS_NORMAL)
        {
          if (transact(pl, NULL, ch, 200000))
          {
            mobsay(ch,
                   "Okay, your name has been added to our guards rosters. Obey all our laws, or face our justice!");
            PC_SET_TOWN_JUSTICE_FLAGS(pl, JUSTICE_IS_CITIZEN_BUY, i);
          }
          else
            mobsay(ch, "You'll need a 200 platinum tithe for that.");
        }
        else if (PC_TOWN_JUSTICE_FLAGS(pl, i) == JUSTICE_IS_OUTCAST)
        {
          if (transact(pl, NULL, ch, 1000000))
          {
            mobsay(ch,
                   "Okay, your previous crimes have been wiped from our records. Do not let us catch you in any further infractions.");
            PC_SET_TOWN_JUSTICE_FLAGS(pl, JUSTICE_IS_NORMAL, i);
          }
          else
            mobsay(ch,
                   "An outcast huh? For 1000 platinum, I'll clear your name.");
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

//NEWBIEGUARD
int newbie_guard_north(P_char ch, P_char pl, int cmd, char *arg)
{
  int  allowed = 0;
  P_char   rider;

  if(cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(!ch)
    return 0;

  if(!pl)
    return 0;

  if(!(cmd == CMD_NORTH))
    return 0;
    
  rider = get_linking_char(pl, LNK_RIDING);

  if(rider && 
    (GET_LEVEL(rider) >= 26 ||
    GET_LEVEL(ch) >= 26))
  { }  
  else if(GET_LEVEL(pl) < 26)
  {
    allowed = 1;
    
    if(IS_NPC(pl) &&
       IS_SET(pl->specials.act, ACT_MOUNT) &&
       rider &&
       GET_LEVEL(rider) > 25 &&
       !IS_TRUSTED(rider))
            allowed = 0;
  }
  else if (IS_TRUSTED(pl))
    allowed = 1;
  else
    allowed = 0;

  if (allowed)
  {
    act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
    act("$N nods and stands aside to let you pass.",
        FALSE, pl, 0, ch, TO_CHAR);
    return (FALSE);
  }
  /*
   * BLOCK!
   */
  act("$N says '&+ROver my dead body!&n.",
    FALSE, pl, 0, ch, TO_CHAR);
  act("$N says '&+ROver my dead body!&n.",
    FALSE, pl, 0, ch, TO_NOTVICT);
  return (TRUE);
}

int newbie_guard_east(P_char ch, P_char pl, int cmd, char *arg)
{
  int  allowed = 0;
  P_char   rider;

  if(cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(!ch)
    return 0;

  if(!pl)
    return 0;

  if(!(cmd == CMD_EAST))
    return 0;
    
  rider = get_linking_char(pl, LNK_RIDING);

  if(rider && 
    (GET_LEVEL(rider) >= 26 ||
    GET_LEVEL(ch) >= 26))
  { }  
  else if(GET_LEVEL(pl) < 26)
  {
    allowed = 1;
    
    if(IS_NPC(pl) &&
       IS_SET(pl->specials.act, ACT_MOUNT) &&
       rider &&
       GET_LEVEL(rider) > 25 &&
       !IS_TRUSTED(rider))
            allowed = 0;
  }
  else if (IS_TRUSTED(pl))
    allowed = 1;
  else
    allowed = 0;

  if (allowed)
  {
    act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
    act("$N nods and stands aside to let you pass.",
        FALSE, pl, 0, ch, TO_CHAR);
    return (FALSE);
  }
  /*
   * BLOCK!
   */
  act("$N says '&+ROver my dead body!&n.",
    FALSE, pl, 0, ch, TO_CHAR);
  act("$N says '&+ROver my dead body!&n.",
    FALSE, pl, 0, ch, TO_NOTVICT);
  return (TRUE);
}

int newbie_guard_south(P_char ch, P_char pl, int cmd, char *arg)
{
  int  allowed = 0;
  P_char   rider;

  if(cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(!ch)
    return 0;

  if(!pl)
    return 0;

  if(!(cmd == CMD_SOUTH))
    return 0;
    
  rider = get_linking_char(pl, LNK_RIDING);

  if(rider && 
    (GET_LEVEL(rider) >= 26 ||
    GET_LEVEL(ch) >= 26))
  { }  
  else if(GET_LEVEL(pl) < 26)
  {
    allowed = 1;
    
    if(IS_NPC(pl) &&
       IS_SET(pl->specials.act, ACT_MOUNT) &&
       rider &&
       GET_LEVEL(rider) > 25 &&
       !IS_TRUSTED(rider))
            allowed = 0;
  }
  else if (IS_TRUSTED(pl))
    allowed = 1;
  else
    allowed = 0;

  if (allowed)
  {
    act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
    act("$N nods and stands aside to let you pass.",
        FALSE, pl, 0, ch, TO_CHAR);
    return (FALSE);
  }
  /*
   * BLOCK!
   */
  act("$N says '&+ROver my dead body!&n.",
    FALSE, pl, 0, ch, TO_CHAR);
  act("$N says '&+ROver my dead body!&n.",
    FALSE, pl, 0, ch, TO_NOTVICT);
  return (TRUE);
}

int newbie_guard_west(P_char ch, P_char pl, int cmd, char *arg)
{
  int  allowed = 0;
  P_char   rider;

  if(cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(!ch)
    return 0;

  if(!pl)
    return 0;

  if(!(cmd == CMD_WEST))
    return 0;
    
  rider = get_linking_char(pl, LNK_RIDING);

  if(rider && 
    (GET_LEVEL(rider) >= 26 ||
    GET_LEVEL(ch) >= 26))
  { }  
  else if(GET_LEVEL(pl) < 26)
  {
    allowed = 1;
    
    if(IS_NPC(pl) &&
       IS_SET(pl->specials.act, ACT_MOUNT) &&
       rider &&
       GET_LEVEL(rider) > 25 &&
       !IS_TRUSTED(rider))
            allowed = 0;
  }
  else if (IS_TRUSTED(pl))
    allowed = 1;
  else
    allowed = 0;

  if (allowed)
  {
    act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
    act("$N nods and stands aside to let you pass.",
        FALSE, pl, 0, ch, TO_CHAR);
    return (FALSE);
  }
  /*
   * BLOCK!
   */
  act("$N says '&+ROver my dead body!&n.",
    FALSE, pl, 0, ch, TO_CHAR);
  act("$N says '&+ROver my dead body!&n.",
    FALSE, pl, 0, ch, TO_NOTVICT);
  return (TRUE);
}

//END NEWBIE GUARD
int rentacleric(P_char ch, P_char vict, int cmd, char *argument)
{
  int      i, diff, cost, spl;
  P_obj    obj, next_obj;
  char     buf[MAX_STRING_LENGTH];
  struct price_info
  {
    short int number;
    char     name[50];
    char     tobuy[50];
    int      price;
  } prices[] =
  {
    /* Spell Num (defined)      Name shown                               Name           Price  */
    {
    SPELL_CURE_CRITIC,   "&+WCure critical wounds&n     ",       "cure critical wounds", 250},
    {
    SPELL_FULL_HEAL,     "&+WFull heal&n                ",       "full heal",            500},
    {
    SPELL_ARMOR,         "&+wBenevolent armor&n         ",       "benevolent armor",     100},
    {
    SPELL_BLESS,         "&+WBlessing &+Lof the &+RGods&n     ", "blessing of the gods", 100},
    {
    SPELL_REMOVE_POISON, "&+GAntidote&n                 ",       "antidote",             600},
    {
    SPELL_CURE_DISEASE,  "&+yDisease &+wremoval&n          ",    "disease removal",      650},
    {
    SPELL_REMOVE_CURSE,  "&+rCurse &+wremoval&n            ",    "curse removal",        700},
    {
    SPELL_CURE_BLIND,    "&+WCure of &+Lblindness&n        ",    "cure of blindness",    500},
    {
    SPELL_ACCEL_HEALING, "&+YAccelerated &+Whealing&n      ",    "accelerated healing", 2500},
    {
    SPELL_RESURRECT,     "&+WResurrection&n             ",       "resurrection",        5000},
    {
     -1, "\r\n", -1},
  };

  if (cmd == CMD_SET_PERIODIC)
  {
    // So they can cast spells.
    if( GET_LEVEL(ch) < 56 )
      ch->player.level = 56;
    return TRUE;
  }

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch))
    return FALSE;

  if (cmd == CMD_BUY)
  {
    argument = one_argument(argument, buf);
    if (*buf)
    {
      for (i = 0; prices[i].number > SPELL_RESERVED_DBC; i++)
        if (is_abbrev(buf, prices[i].tobuy))
        {
          /* resur is special case. Just find any corpse and raise it :) */
          if (prices[i].number == SPELL_RESURRECT)
          {
            // In case 'order follower buy resurrect' etc.
            if( IS_NPC(vict) )
            {
              mobsay( ch, "I only raise PC corpses, maybe you should talk to Melmba" );
              return TRUE;
            }
            for (obj = world[ch->in_room].contents; obj; obj = next_obj)
            {
              next_obj = obj->next_content;
              if( (obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], PC_CORPSE)
                && isname(GET_NAME(vict), obj->action_description) )
                break;
            }
            if (!obj)
            {
              mobsay(ch, "Did you perhaps forget to bring your friend?");
              return TRUE;
            }
          }
          cost = prices[i].price * (GET_LEVEL(vict) < 36 ? GET_LEVEL(vict) / 4 : GET_LEVEL(vict));
          spl = prices[i].number;
          if (transact(vict, NULL, ch, cost))
          {
            /* make em broke, as clerics should be */
            GET_PLATINUM(ch) = GET_GOLD(ch) = GET_SILVER(ch) = GET_COPPER(ch) = 0;
            StopCasting(ch);
            if(!(spl == SPELL_RESURRECT))
            {
              MobCastSpell(ch, vict, NULL, spl, GET_LEVEL(vict) < 20 ? 60 : GET_LEVEL(vict));
              return TRUE;
            }
            else
            {
              MobCastSpell(ch, vict, obj, spl, 60);
              return TRUE;
            }
          }
          else
            return TRUE;
        }
      mobsay(ch, "Sorry, I don't know of that spell.");
      return TRUE;
    }
    else
    {
      act("$n tells you, 'Here is a listing of the prices for my services.'",
          FALSE, ch, 0, vict, TO_VICT);
      for (i = 0; prices[i].number > SPELL_RESERVED_DBC; i++)
      {
        cost = prices[i].price * (GET_LEVEL(vict) < 36 ? GET_LEVEL(vict) / 4 : GET_LEVEL(vict));
        sprintf(buf, "%s%s\r\n", prices[i].name, coin_stringv(cost));
        send_to_char(buf, vict);
      }
      return TRUE;
    }
  }
  else if (cmd == CMD_LIST)
  {
    act("$n tells you, 'Here is a listing of the prices for my services.'",
        FALSE, ch, 0, vict, TO_VICT);
    for (i = 0; prices[i].number > SPELL_RESERVED_DBC; i++)
    {
      cost = prices[i].price * (GET_LEVEL(vict) < 36 ? GET_LEVEL(vict) / 4 : GET_LEVEL(vict));
      sprintf(buf, "%s%s\r\n", prices[i].name, coin_stringv(cost));
      send_to_char(buf, vict);
    }
    return TRUE;
  }
  return FALSE;
}


/* Justice clerk (replace this fonction TASFALEN) */

int justice_clerk(P_char ch, P_char pl, int cmd, char *arg)
{
  char     buf[MAX_STRING_LENGTH], tempbuf[512];
  int      i, reward;
  int      crime_commited;
  crm_rec *crec = NULL;
  wtns_rec *rec = NULL;
  char     arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char     arg3[MAX_INPUT_LENGTH];
  time_t   ttime;
  P_char   criminal;
  bool     crime_ok = FALSE;
  bool     not_bind = FALSE;
  P_obj    next_obj = NULL, o_obj = NULL, prev_obj = NULL;
  int      found_item = FALSE;

  /*
   * check for periodic event calls
   */

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (pl)
  {
    i = CHAR_IN_TOWN(ch);

    if (i < 1 || i > LAST_HOME)
    {
      sprintf(tempbuf, "Justice_clerk is in town %d?!?\r\n", i);
      return FALSE;
    }

    ttime = time(NULL);

    if (IS_TOWN_INVADER(pl, i))
      return FALSE;
    if (cmd == CMD_LIST)
    {
      if (!hometowns[CHAR_IN_TOWN(ch) - 1].crime_list)
      {
        act("$n tells you, 'No wanted criminals right now, come back later.'",
            FALSE, ch, 0, pl, TO_VICT);
        return TRUE;
      }

      sprintf(buf, "&+RList of all wanted criminals&N.\r\n");
      while ((crec = crime_find(hometowns[CHAR_IN_TOWN(ch) - 1].crime_list,
                                NULL, NULL, 0, NOWHERE, J_STATUS_WANTED,
                                crec)))
      {
        sprintf(buf, "%s&+R%s&N wanted for %s, reward &+c%d&n platinums.\r\n",
                buf, crec->attacker, crime_list[crec->crime],
                (int) crec->money);
      }
      send_to_char(buf, pl);
      return TRUE;

    }
    else if (cmd == CMD_PAY)
    {
      if ((crec = crime_find(hometowns[CHAR_IN_TOWN(ch) - 1].crime_list,
                             J_NAME(pl), NULL, CRIME_NONE, NOWHERE,
                             J_STATUS_DEBT, NULL)))
      {

        if (GET_MONEY(pl) >= crec->money * 1000)
        {
          if (SUB_MONEY(pl, crec->money * 1000, 0) > -1)
            ADD_MONEY(ch, crec->money * 1000);
          else
            return TRUE;
          sprintf(buf, "You give %s %s.\r\n\r\n", ch->player.short_descr,
                  coin_stringv(crec->money * 1000));
          send_to_char(buf, pl);
          act("$n gives $N some money.", TRUE, pl, 0, ch, TO_NOTVICT);
          send_to_char("You are now in order, stay in the right way now.\r\n",
                       pl);
          crime_remove(CHAR_IN_TOWN(ch), crec);
          return TRUE;
        }
        else
        {
          sprintf(buf,
                  "You do not have enough money, your debt is %d platinum.\r\n\r\n",
                  crec->money);
          send_to_char(buf, pl);
          return TRUE;
        }
      }
      else
      {
        send_to_char("You owe no money to this city.\r\n", pl);
      }
      return TRUE;

    }
    else if (cmd == CMD_CLAIM)
    {
/*      if (justice_items_list) {
        prev_obj = NULL;
        for (o_obj = justice_items_list; o_obj; o_obj = next_obj) {
          next_obj = o_obj->next_content;
          if (strstr(o_obj->justice_name, J_NAME(pl))) {
            if (prev_obj == NULL) {
              justice_items_list = o_obj->next_content;
              prev_obj = NULL;
            } else {
              prev_obj->next_content = o_obj->next_content;
              o_obj->next_content = NULL;
            }
            o_obj->justice_status = 0;
            o_obj->justice_name = NULL;
            obj_to_char(o_obj, pl);
            found_item = TRUE;
          } else
            prev_obj = o_obj;
        }
        if (found_item)
          send_to_char("Found some items that belong to you, here you go.\r\n", pl);
        else
          send_to_char("No items belong to you.\r\n", pl);
        return TRUE;
      } else {
        send_to_char("No items belong to you.\r\n", pl);
        return TRUE;
      }*/
      return TRUE;

    }
    else if (cmd == CMD_PARDON)
    {
      arg = one_argument(arg, arg1);
      if (*arg1)
      {

        sprintf(buf, "%s %s\r\n", J_NAME(pl), arg1);
        send_to_char(buf, pl);

        if ((crec = crime_find(hometowns[CHAR_IN_TOWN(ch) - 1].crime_list,
                               arg1, J_NAME(pl), CRIME_NONE, NOWHERE,
                               J_STATUS_CRIME, NULL)))
        {
          crime_commited = crec->crime;
          send_to_char("I will log this in my book, thanks\r\n", pl);
          crime_add(CHAR_IN_TOWN(ch), arg1, J_NAME(pl), pl->in_room,
                    crime_commited, time(NULL), J_STATUS_PARDON, 1);
        }
        else
          send_to_char
            ("This person did not commit any crimes against you!\r\n", pl);
      }
      else
      {
        act("$n tells you, 'Pardon who?'", FALSE, ch, 0, pl, TO_VICT);
      }
      return TRUE;

    }
    else if (cmd == CMD_TURN_IN)
    {

      arg = one_argument(arg, arg1);
      if (*arg1)
      {
        if ((criminal = get_char(arg1)))
        {
          if (criminal->in_room == pl->in_room)
          {
            if ((crec = crime_find(hometowns[CHAR_IN_TOWN(ch) - 1].crime_list,
                                   arg1, NULL, CRIME_NONE, NOWHERE,
                                   J_STATUS_WANTED, NULL)))
            {

              if (!IS_SET(criminal->specials.affected_by, AFF_BOUND))
                not_bind = TRUE;

              mobsay(ch, "Thanks you for bringing this wanted criminal in.");
//            reward = crec->money;
              reward = 5;

              char_from_room(criminal);
              char_to_room(criminal,
                           real_room(hometowns[CHAR_IN_TOWN(ch) - 1].
                                     jail_room), -1);
              act("$n throw $N in jail.", FALSE, ch, 0, criminal, TO_NOTVICT);
              send_to_char("You've been thrown to jail.", criminal);
              REMOVE_BIT(criminal->specials.affected_by, AFF_BOUND);
              crime_add(CHAR_IN_TOWN(ch), J_NAME(criminal), J_NAME(ch),
                        real_room(hometowns[CHAR_IN_TOWN(ch) - 1].jail_room),
                        0, time(NULL), J_STATUS_IN_JAIL, 1);
              writeJailItems(criminal);
              writeCharacter(criminal, 7, criminal->in_room);

              if (GET_CLASS(pl, CLASS_MERCENARY))
              {
                if (GET_LEVEL(pl) >= 20)
                {
                  if (not_bind)
                  {
                    mobsay(ch,
                           "You just brought an unbound criminal, your probably working with him to get the reward.");
                    char_from_room(pl);
                    char_to_room(pl,
                                 real_room(hometowns[CHAR_IN_TOWN(ch) - 1].
                                           jail_room), -1);
                    act("$n throw $N in jail.", FALSE, ch, 0, pl, TO_NOTVICT);
                    send_to_char("All your equipment has been removed.\r\n",
                                 pl);
                    send_to_char("You've been thrown in jail.\r\n", pl);
                    crime_add(CHAR_IN_TOWN(ch), J_NAME(pl), J_NAME(ch),
                              real_room(hometowns[CHAR_IN_TOWN(ch) - 1].
                                        jail_room), 0, time(NULL),
                              J_STATUS_IN_JAIL, 1);
                    crime_add(CHAR_IN_TOWN(ch), J_NAME(pl), J_NAME(ch),
                              real_room(hometowns[CHAR_IN_TOWN(ch) - 1].
                                        jail_room), CRIME_AGAINST_TOWN,
                              time(NULL), J_STATUS_CRIME, 1);
//                  check_item(pl);
                    writeJailItems(pl);
                    writeCharacter(pl, 7, pl->in_room);
                  }
                  else
                  {
                    mobsay(ch, "Here is your reward, good work.");
//                  reward *= 2;
                    GET_PLATINUM(pl) += reward;
                    act("$n gives $N some platinum coins.",
                        FALSE, ch, 0, pl, TO_NOTVICT);
                    sprintf(buf, "%s gives you %d platinum coins.",
                            J_NAME(ch), reward);
                    send_to_char(buf, pl);

                    logit(LOG_CRIMES, "%s turn in %s and got %d plat.",
                          J_NAME(pl), J_NAME(criminal), reward);
                  }

                }
                else
                  send_to_char
                    ("Good work but your not experience enough to get the reward!\r\n",
                     pl);
              }
              else
                send_to_char("Only true bounty hunter get the reward!\r\n",
                             pl);
            }
            else
              send_to_char
                ("This person is not a wanted criminal, dont waste my time!\r\n",
                 pl);
          }
          else
            act("$n tells you, 'Dont see that person here!'",
                FALSE, ch, 0, pl, TO_VICT);
        }
        else
          act("$n tells you, 'Dont see that person here!'",
              FALSE, ch, 0, pl, TO_VICT);
      }
      else
      {
        if ((crec = crime_find(hometowns[CHAR_IN_TOWN(ch) - 1].crime_list,
                               J_NAME(pl), NULL, CRIME_NONE, NOWHERE,
                               J_STATUS_CRIME, NULL)))
        {
          mobsay(ch, "Thanks for turning yourself in.");
          char_from_room(pl);
          char_to_room(pl,
                       real_room(hometowns[CHAR_IN_TOWN(ch) - 1].jail_room),
                       -1);
          act("$n throw $N in jail.", FALSE, ch, 0, pl, TO_NOTVICT);
          send_to_char("All your equipment has been removed.\r\n", pl);
          send_to_char("You've been thrown in jail.", pl);
          crime_add(CHAR_IN_TOWN(ch), J_NAME(pl), J_NAME(ch),
                    real_room(hometowns[CHAR_IN_TOWN(ch) - 1].jail_room), 0,
                    time(NULL), J_STATUS_IN_JAIL, 1);
          crime_add(CHAR_IN_TOWN(ch), J_NAME(ch), J_NAME(pl), pl->in_room,
                    crec->crime, time(NULL), J_STATUS_PARDON, 1);
//          check_item(pl);
          writeJailItems(pl);
          writeCharacter(pl, 7, pl->in_room);

        }
        else
          act("$n tells you, 'Who do you want to turn in?'",
              FALSE, ch, 0, pl, TO_VICT);
      }

      return TRUE;

    }
    else if (cmd == CMD_REPORTING)
    {
      arg = one_argument(arg, arg2);

      if (*arg2)
      {

        for (i = 0; i < CRIME_NB; i++)
        {
          if ((!str_cmp(arg2, crime_rep[i])))
          {
            if (GET_CRIME_T(CHAR_IN_TOWN(pl), i))
              crime_commited = i;
            else
              send_to_char("That is not a crime here.\r\n", pl);
            crime_ok = TRUE;
          }
        }

        if (!crime_ok)
        {
          sprintf(buf, "Valid crimes are :");
          for (i = 0; i < CRIME_NB; i++)
          {
            if (GET_CRIME_T(CHAR_IN_TOWN(pl), i))
              sprintf(buf, "%s %s", buf, crime_rep[i]);
          }
          sprintf(buf, "%s\r\n", buf);
          send_to_char(buf, pl);
          return TRUE;
        }

        if (IS_TRUSTED(pl) && GET_LEVEL(pl) >= 59)
        {
          crime_add(CHAR_IN_TOWN(ch), arg1, arg3, pl->in_room, crime_commited,
                    time(NULL), J_STATUS_CRIME, 1);
          sprintf(buf, "You report that %s commited %s against %s\r\n", arg1,
                  arg2, arg3);
          send_to_char(buf, pl);
          justice_dispatch_guard(CHAR_IN_TOWN(ch), arg1, arg3,
                                 crime_commited);

          return TRUE;
        }

        if ((rec = witness_find(pl->specials.witnessed,
                                NULL, NULL, crime_commited, NOWHERE, NULL)))
        {
          if (!rec || !rec->attacker || !rec->victim)
          {
            wizlog(AVATAR, "DEBUG: Reporting error, missing rec->");
            return FALSE;
          }
          crime_add(CHAR_IN_TOWN(ch), rec->attacker, rec->victim, rec->room,
                    crime_commited, rec->time, J_STATUS_CRIME, 1);
          sprintf(buf, "You report that someone commited %s\r\n", arg2);
          send_to_char(buf, pl);

          justice_dispatch_guard(CHAR_IN_TOWN(ch), rec->attacker, rec->victim,
                                 crime_commited);

          while ((rec = witness_find(pl->specials.witnessed,
                                     rec->attacker, rec->victim,
                                     crime_commited, NOWHERE, NULL)))
          {
            witness_remove(pl, rec);
          }

        }
        else
        {
          send_to_char
            ("Liar, you report a crime you did not witness, get out of here!\r\n",
             pl);
        }

      }
      else
        send_to_char("Reporting <crime>\r\n", pl);

      return TRUE;
    }
  }
  return FALSE;
}


/*
 *Patrol leader Mob Proc
 */
int patrol_leader(P_char ch, P_char pl, int cmd, char *arg)
{
  int      door, direction;
  bool     CombatInRoom;
  P_char   tmp_ch;
  struct follow_type *k, *next_dude;
  char     buf[256];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (GET_VITALITY(ch) < 10)    /* ok dont get too tired */
    return TRUE;

  /* ok check to make sure followers are not out of move */

  if (ch->followers)
  {
    for (k = ch->followers; k; k = next_dude)
    {
      next_dude = k->next;
      if (IS_NPC(k->follower) && (GET_VITALITY(k->follower) < 10))
        return TRUE;
    }
  }

  CombatInRoom = FALSE;

  if (!ALONE(ch))
  {
    if (IS_FIGHTING(ch))
      CombatInRoom = TRUE;
    else
    {
      LOOP_THRU_PEOPLE(tmp_ch, ch) if (IS_FIGHTING(tmp_ch))
      {
        CombatInRoom = TRUE;
        break;
      }
    }
  }

  if (IS_FIGHTING(ch) && number(1, 3) == 1)
  {
    if (IS_PC(ch->specials.fighting) && RACE_EVIL(ch->specials.fighting))
    {
      strcpy(buf, "Die you evil scum!");
      do_yell(ch, buf, CMD_SHOUT);
    }
    else if (IS_PC(ch->specials.fighting) && RACE_GOOD(ch->specials.fighting))
    {
      strcpy(buf, "You moron I am here to protect you!");
      do_say(ch, buf, CMD_SAY);
    }
  }

  if (!CombatInRoom && (ch->in_room != NOWHERE) &&
      !IS_SET(world[ch->in_room].room_flags, ROOM_SILENT) &&
      !IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_SILENT) &&
      (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
  {

    /* ok we check if there is any evils near */

    if ((direction = range_scan(ch, NULL, 3, SCAN_EVILRACE)) >= 0)
    {
      if (EXIT(ch, direction))
      {
        if ((EXIT(ch, direction))->to_room &&
            world[EXIT(ch, direction)->to_room].justice_area ==
            world[ch->in_room].justice_area)
        {
          ch->only.npc->last_direction = direction;
          do_move(ch, 0, exitnumb_to_cmd(direction));
          return TRUE;
        }
      }
    }

    /* ok we check if there is combat near */

    if ((direction = range_scan(ch, NULL, 2, SCAN_COMBAT)) >= 0)
    {
      if (EXIT(ch, direction))
      {
        if ((EXIT(ch, direction))->to_room &&
            world[EXIT(ch, direction)->to_room].justice_area ==
            world[ch->in_room].justice_area)
        {
          ch->only.npc->last_direction = direction;
          do_move(ch, 0, exitnumb_to_cmd(direction));
          return TRUE;
        }
      }
    }
  }


  /* seem we can try to move, since nothing else to do */

  if (!CombatInRoom && !IS_AFFECTED(ch, AFF_CHARM))
  {
    if ((MIN_POS(ch, POS_STANDING + STAT_RESTING)) &&
        ((door = number(0, 4)) < 4))
    {
      if (EXIT(ch, door))
      {
        if (CAN_GO(ch, door) &&
            EXIT(ch, door)->to_room != NOWHERE &&
            !IS_SET(world[EXIT(ch, door)->to_room].room_flags, NO_MOB) &&
            world[EXIT(ch, door)->to_room].sector_type != SECT_NO_GROUND)
        {
          if (world[EXIT(ch, door)->to_room].justice_area ==
              world[ch->in_room].justice_area)
          {
            ch->only.npc->last_direction = door;
            do_move(ch, 0, exitnumb_to_cmd(door));
            return TRUE;
          }
        }
      }
    }
  }

  return FALSE;

}


/*
 * Patrol leader (road) Mob Proc
 * this mob should stay on road, if he get away from road he gonna try to get back asap
 */
int patrol_leader_road(P_char ch, P_char pl, int cmd, char *arg)
{
  int      door, i, direction;
  bool     CombatInRoom;
  P_char   tmp_ch;
  struct follow_type *k, *next_dude;
  char     pos_exit[NUM_EXITS];
  int      nb_exit = 0;
  char     buf[256];

  /* check for periodic event calls */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (GET_VITALITY(ch) < 10)    /* ok dont get too tired */
    return TRUE;

  /* ok check to make sure followers are not out of move */

  if (ch->followers)
  {
    for (k = ch->followers; k; k = next_dude)
    {
      next_dude = k->next;
      if (IS_NPC(k->follower) && (GET_VITALITY(k->follower) < 10))
        return TRUE;
    }
  }

  CombatInRoom = FALSE;

  if (!ALONE(ch))
  {
    if (IS_FIGHTING(ch))
      CombatInRoom = TRUE;
    else
    {
      LOOP_THRU_PEOPLE(tmp_ch, ch) if (IS_FIGHTING(tmp_ch))
      {
        CombatInRoom = TRUE;
        break;
      }
    }
  }

  if (IS_FIGHTING(ch) && number(1, 3) == 1)
  {
    if (IS_PC(ch->specials.fighting) && RACE_EVIL(ch->specials.fighting))
    {
      strcpy(buf, "Die you evil scum!");
      do_yell(ch, buf, CMD_SHOUT);
    }
    else if (IS_PC(ch->specials.fighting) && RACE_GOOD(ch->specials.fighting))
    {
      strcpy(buf, "You moron I am here to protect you!");
      do_say(ch, buf, CMD_SAY);
    }
  }

  if (!CombatInRoom && (ch->in_room != NOWHERE) &&
      !IS_SET(world[ch->in_room].room_flags, ROOM_SILENT) &&
      !IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_SILENT) &&
      (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
  {

    /* ok we check if there is any evils near */

    if ((direction = range_scan(ch, NULL, 1, SCAN_EVILRACE)) >= 0)
    {
      if (EXIT(ch, direction))
      {
        if (EXIT(ch, direction)->to_room &&
            world[EXIT(ch, direction)->to_room].justice_area ==
            world[ch->in_room].justice_area)
        {
          ch->only.npc->last_direction = direction;
          do_move(ch, 0, exitnumb_to_cmd(direction));
          return TRUE;
        }
      }
    }

    /* ok we check if there is combat near */

    if ((direction = range_scan(ch, NULL, 1, SCAN_COMBAT)) >= 0)
    {
      if (EXIT(ch, direction))
      {
        if (EXIT(ch, direction)->to_room &&
            world[EXIT(ch, direction)->to_room].justice_area ==
            world[ch->in_room].justice_area)
        {
          ch->only.npc->last_direction = direction;
          do_move(ch, 0, exitnumb_to_cmd(direction));
          return TRUE;
        }
      }
    }
  }

  /* seem we can try to move, since nothing else to do */

  if (world[ch->in_room].sector_type != SECT_ROAD)
  {                             /* gasp we left the road */

    if (!CombatInRoom && !IS_AFFECTED(ch, AFF_CHARM))
    {
      /* ok first lets see if we near a road? */
      for (door = 0; door < 4; door++)
      {
        if (EXIT(ch, door))
        {
          if ((MIN_POS(ch, POS_STANDING + STAT_RESTING)) &&
              (EXIT(ch, door)->to_room) &&
              CAN_GO(ch, door) &&
              !IS_SET(world[EXIT(ch, door)->to_room].room_flags, NO_MOB) &&
              world[EXIT(ch, door)->to_room].sector_type != SECT_NO_GROUND &&
              world[EXIT(ch, door)->to_room].justice_area ==
              world[ch->in_room].justice_area &&
              world[EXIT(ch, door)->to_room].sector_type == SECT_ROAD)
          {
            ch->only.npc->last_direction = door;
            do_move(ch, 0, exitnumb_to_cmd(door));
            return TRUE;
          }
        }
      }

      /* ok seem we are not near a road, PANIC! we are lost */
      /* lets move and hope we hit a road somewhere */

      if ((MIN_POS(ch, POS_STANDING + STAT_RESTING)) &&
          ((door = number(0, 4)) < 4))
      {
        if (EXIT(ch, door))
        {
          if (CAN_GO(ch, door) &&
              (EXIT(ch, door)->to_room) &&
              !IS_SET(world[EXIT(ch, door)->to_room].room_flags, NO_MOB) &&
              world[EXIT(ch, door)->to_room].sector_type != SECT_NO_GROUND)
          {
            if (EXIT(ch, door)->to_room &&
                world[EXIT(ch, door)->to_room].justice_area ==
                world[ch->in_room].justice_area)
            {
              ch->only.npc->last_direction = door;
              do_move(ch, 0, exitnumb_to_cmd(door));
              return TRUE;
            }
          }
        }
      }
    }

  }
  else
  {                             /* ok we are on the road so lets move */
    if (!CombatInRoom && !IS_AFFECTED(ch, AFF_CHARM))
    {

      /* first we check where we can go */
      for (door = 0; door < 4; door++)
      {
        if (EXIT(ch, door))
        {
          if ((MIN_POS(ch, POS_STANDING + STAT_RESTING)) &&
              CAN_GO(ch, door) &&
              (EXIT(ch, door)->to_room) &&
              !IS_SET(world[EXIT(ch, door)->to_room].room_flags, NO_MOB) &&
              world[EXIT(ch, door)->to_room].sector_type != SECT_NO_GROUND &&
              world[EXIT(ch, door)->to_room].justice_area ==
              world[ch->in_room].justice_area &&
              world[EXIT(ch, door)->to_room].sector_type == SECT_ROAD)
          {
            pos_exit[door] = TRUE;
            nb_exit++;
          }
          else
          {
            pos_exit[door] = FALSE;
          }
        }
        else
          pos_exit[door] = FALSE;
      }

      /* now lets make sure we dont go backward if there is another possibility */

      if (nb_exit == 1)
      {
        if ((i = number(0, nb_exit)) < nb_exit)
        {
          for (door = 0; door < 4; door++)
          {
            if (pos_exit[door])
            {
              ch->only.npc->last_direction = door;
              do_move(ch, 0, exitnumb_to_cmd(door));
              return TRUE;
            }
          }
        }
      }
      else
      {

        pos_exit[(int) rev_dir[(int) ch->only.npc->last_direction]] = FALSE;
        if ((i = number(1, nb_exit)) < nb_exit)
        {
          for (door = 0; door < 4; door++)
          {
            if (pos_exit[door])
              i--;
            if (i <= 0)
            {
              ch->only.npc->last_direction = door;
              do_move(ch, 0, exitnumb_to_cmd(door));
              return TRUE;
            }
          }
        }
      }
    }
  }
  return FALSE;

}


/*
 * summon_creature - summons one particular type of creature to master
 *      (leaving stats of NPC summoned alone) can control max numb and
 *      duration - returns pointer to P_char summoned or NULL if some error
 */

P_char summon_creature(int mobnumb, P_char master, int max_summon,
                       int dur, const char *appearsC, const char *appears)
{
  struct affected_type af;
  P_char   mob;
  struct follow_type *k;
  int      i;

  if (!master || (mobnumb < 0) || (real_mobile(mobnumb) == -1))
  {
    return NULL;
  }

  if (dur <= 0)
    dur = (GET_LEVEL(master) * 4) + 10;

  if (CHAR_IN_SAFE_ZONE(master))
  {
    send_to_char("A mysterious force blocks your summoning!\r\n", master);
    return NULL;
  }

  if (max_summon)
  {
    for (k = master->followers, i = 0; k; k = k->next)
    {
      if (k->follower && IS_NPC(k->follower) && (GET_VNUM(k->follower) == mobnumb))
        i++;
    }

    if (i >= max_summon)
    {
      send_to_char("You cannot bind any more creatures to your control.\r\n",
                   master);
      return NULL;
    }
  }

  mob = read_mobile(real_mobile(mobnumb), REAL);
  if (!mob)
  {
    logit(LOG_DEBUG, "summon_creature(): mob %d not loadable", mobnumb);
    return NULL;
  }

  char_to_room(mob, master->in_room, 0);

  while (mob->affected)
    affect_remove(mob, mob->affected);

  if (!IS_SET(mob->specials.act, ACT_MEMORY))
    clearMemory(mob);

  balance_affects(mob);

  if (appears)
    act(appears, FALSE, master, 0, master, TO_ROOM);
  if (appearsC)
    act(appearsC, FALSE, master, 0, 0, TO_CHAR);

  setup_pet(mob, master, dur, 0);

  add_follower(mob, master);
  group_add_member(master, mob);

  return mob;
}


/*
 *  if vict is charmed by someone other than master, make em charmed
 *  by master (group em, too) - if madatOldMaster is set, add old master's
 *  name to memory
 */

int recharm_ch(P_char master, P_char vict, bool madatOldMaster,
               char *charmMsg)
{
  P_char   oldmast = NULL, tmpch;

  if (IS_NPC(vict) && (IS_GREATER_DRACO(vict) || IS_GREATER_AVATAR(vict)))
    return FALSE;

  if (!master || !vict || (master == vict) || !IS_NPC(vict) ||
      !IS_PC_PET(vict) || (vict->in_room != master->in_room))
    return FALSE;

  if (GET_MASTER(vict) == master)
    return FALSE;               /* already charmed */

  /* we're successful, baby */

  if (charmMsg)
    act(charmMsg, TRUE, master, 0, vict, TO_ROOM);

  stop_fighting(vict);
  if( IS_DESTROYING(vict) )
    stop_destroying(vict);
  stop_fighting(master);
  if( IS_DESTROYING(master) )
    stop_destroying(master);

  /* stop all combat with new charmie */

  for (tmpch = world[vict->in_room].people; tmpch;
       tmpch = tmpch->next_in_room)
    if ((tmpch != vict) && (tmpch->specials.fighting == vict))
      stop_fighting(tmpch);

  if (vict->following)
  {
    oldmast = vict->following;
    stop_follower(vict);
  }

  add_follower(vict, master);

  setup_pet(vict, master, 24 * 18, 0);

  if (vict->group)
    group_remove_member(vict);

  group_add_member(master, vict);

//  SET_BIT(vict->specials.act, ACT_AGGRESSIVE);
  SET_BIT(vict->only.npc->aggro_flags, AGGR_ALL);
  SET_BIT(vict->specials.act, ACT_SENTINEL);
  SET_BIT(vict->specials.act, ACT_PROTECTOR);

  if (madatOldMaster && oldmast)
    remember(vict, oldmast);

  return TRUE;
}

/*
 * acerlade, the big meany in the transparent tower who turns PC charmees
 * into NPC charmees..  WATCH OUT KIDS!
 */

int transp_tow_acerlade(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, next;
  char     didit = FALSE;
  char     buf[256];

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!ch)
    return FALSE;

  /* hell, go for broke, try em all */

  if (ch->in_room == NOWHERE)
    return FALSE;

  sprintf(buf,
          "&+LWith a wicked grin, Aceralde brutally steals control of &n$N&+L!");
  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if (recharm_ch(ch, tch, (bool) TRUE, buf))
      didit = TRUE;
  }

  return didit;
}


/*
 * satar ghulan in obsidian citadel summons up to two flesh golems
 */

int obsid_cit_satar_ghulan(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!ch)
    return FALSE;               /* boggle */

  /* summon beastie #75648, max of 2, let the func set duration itself */

  if (summon_creature(75648, ch, 2, 0,
                      NULL, "&+MWith an arcane gesture, &n$N&+M suddenly "
                      "summons a flesh golem to do $S bidding!"))
    return TRUE;
  else
    return FALSE;
}

/*
 * death knight proc for obsidian citadel, make em cast fireball and
 * incend cloud now and then
 */

int obsid_cit_death_knight(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, vict = NULL;
  int      numbPCs = 0, luckyPC = 0, currPC = 0, numb;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!IS_FIGHTING(ch))
    return FALSE;

  if (!ch->specials.fighting)
    return FALSE;

  /* count number of PCs, pick someone */

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (IS_PC(tch) && !IS_TRUSTED(tch))
      numbPCs++;
  }

  if (!numbPCs)
    return FALSE;               /* doh */

  if (numbPCs == 1)
    luckyPC = 0;
  else
    luckyPC = number(0, numbPCs - 1);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (IS_PC(tch) && !IS_TRUSTED(tch))
    {
      if (currPC == luckyPC)
      {
        vict = tch;
        break;
      }
      else
        currPC++;
    }
  }

  if (!vict)                    /* hmm, error */
  {
    act("$n says 'bug in my proc!  tell a god!'", 1, ch, 0, 0, TO_ROOM);
    return FALSE;
  }

  numb = number(1, 100);

  /* 5% chance of incendiary cloud of death */

  if (numb <= 5)
  {
    act("$n&n's body suddenly &+Rglows brightly&n!", 1, ch, 0, 0, TO_ROOM);
    spell_incendiary_cloud((int) (GET_LEVEL(ch) * 1.5), ch, 0,
                           SPELL_TYPE_SPELL, vict, 0);
    return TRUE;
  }

  /* 25% chance of fireball */

  else if (numb >= 75)
  {
    act("$n&n's body suddenly &+Rglows brightly&n!", 1, ch, 0, 0, TO_ROOM);
    spell_fireball((int) (GET_LEVEL(ch) * 1.5), ch, 0, SPELL_TYPE_SPELL, vict,
                   0);
    return TRUE;
  }

  else
    return FALSE;
}

int claw_cavern_drow_mage(P_char ch, P_char pl, int cmd, char *arg)
{
  char     obj_name[MAX_INPUT_LENGTH], *argument;
  P_obj    obj;

  /*
   * Check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!pl || (IS_TRUSTED(pl) && (cmd != CMD_GIVE)))
    return FALSE;

  if (IS_AGG_CMD(cmd) && (cmd != CMD_WILL) && (cmd != CMD_CAST))
  {
    send_to_char("The walls hum as an invisible force pushes you away.\r\n",
                 pl);
    act("The walls hum as $n is pushed away by an invisible field.", FALSE,
        pl, 0, 0, TO_ROOM);
    mobsay(ch,
           "Fools!  Do you think I would even allow you to stand in my presence if I had a choice?");

    return TRUE;
  }

  if ((cmd == CMD_WILL) || (cmd == CMD_CAST))
  {
    send_to_char
      ("The walls glow as you feel your magic drained into the building around you.\r\n",
       pl);
    act
      ("$n looks bewildered as the walls glow and somehow prevent the casting of $s spell.",
       FALSE, pl, 0, 0, TO_ROOM);
    return TRUE;
  }

  if (cmd == CMD_GIVE)
  {
    argument = arg;
    argument = one_argument(argument, obj_name);

    obj = get_obj_in_list_vis(pl, obj_name, pl->carrying);
    if (!obj)
    {
      send_to_char("You do not seem to have anything like that.\r\n", pl);
      return TRUE;
    }

    if( OBJ_VNUM(obj) == VOBJ_CLWCVRN_RAINBOW_KEY )
    {
      act("$n gives $p to $N.", 1, pl, obj, ch, TO_NOTVICT);
      act("$n gives you $p.", 0, pl, obj, ch, TO_VICT);
      send_to_char("Ok.\r\n", pl);

      obj_from_char(obj);
      extract_obj(obj, TRUE); // Not an arti, but 'in game.'

      mobsay(ch, "Yes!  Now those fools shall bow to their true master!");
      do_action(ch, 0, CMD_CACKLE);
      act("The mage begins a complex incantation and the key begins to glow.",
          TRUE, ch, 0, 0, TO_ROOM);
      act("$n screams as the walls begin to hum at a high pitch and the key starts to shiver.",
         TRUE, ch, 0, 0, TO_ROOM);
      act("The crystal key shakes and glows, finally exploding in a burst of searing light.",
         TRUE, ch, 0, 0, TO_ROOM);

      obj = read_object(VOBJ_CLWCVRN_RAINBOW_SHARDS, VIRTUAL);
      if (!obj)
        mobsay(ch, "Oh damn, there is a bug in the proc!  Tell a god!");
      else
        obj_to_room(obj, ch->in_room);

      extract_char(ch);

      return TRUE;
    }

    return FALSE;               // let normal give handle it
  }

  return FALSE;
}
int undeadcont_track(P_char ch, P_char pl, int cmd, char *arg)
{
  int      door, direction;
  P_char   tmp_ch;
  char     buf[256];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (GET_VITALITY(ch) < 10)    /* ok dont get too tired */
    return TRUE;

  if (!IS_FIGHTING(ch) && (ch->in_room != NOWHERE) &&
      (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
  {

    /* ok we check if there is any PC near */

    if (range_scan_track(ch, 3, SCAN_ANY))
    {
      InitNewMobHunt(ch);
      return TRUE;
    }
  }
  return FALSE;

}


/*
 * Underdark Mob Proc
 */
int underdark_track(P_char ch, P_char pl, int cmd, char *arg)
{
  int      door, direction;
  P_char   tmp_ch;
  char     buf[256];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (GET_VITALITY(ch) < 10)    /* ok dont get too tired */
    return TRUE;

  if (!IS_FIGHTING(ch) && (ch->in_room != NOWHERE) &&
      (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
  {

    /* ok we check if there is any PC near */

    if (range_scan_track(ch, 3, SCAN_ANY))
    {
      InitNewMobHunt(ch);
      return TRUE;
    }
  }
  return FALSE;

}

int jindo_ticket_master(P_char ch, P_char pl, int cmd, char *arg)
{

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // MobCombat() will call this function with pl, cmd, and arg set to null
  if( !pl )
  {
    return FALSE;
  }

  if (cmd == CMD_WEST)
  {
    if( IS_TRUSTED(pl) )
    {
      act("$n bows to your &+WGodliness&n.", TRUE, pl, 0, ch, TO_VICT);
      return FALSE;
    }
    act("$n says 'If you would like to enter the carnival, please buy a ticket.'", TRUE, ch, 0, pl, TO_VICT);
    act("$n says 'Tickets are 5 gold pieces each.'", TRUE, ch, 0, pl, TO_VICT);
    act("$n blocks your passage.", TRUE, pl, 0, ch, TO_VICT);
    act("$N says to $n 'If you would like to enter the carnival, please buy a ticket.'", TRUE, pl, 0, ch, TO_NOTVICT);
    act("$N blocks $m passage.", TRUE, pl, 0, ch, TO_NOTVICT);
    return TRUE;
  }

  if( (cmd == CMD_BUY) && *arg )
  {
    if( isname(arg, "ticket") )
    {
      // Trying to cheese this huh?  Ticketmaster no like!
      if( IS_PC_PET(ch) )
      {
        act("$n says, 'I don't do &+Ycheese&n!'", TRUE, ch, 0, pl, TO_VICT);
        act("$n grabs your wrist and flips you to the ground pinning you!", TRUE, ch, 0, pl, TO_VICT);
        SET_POS(pl, POS_PRONE + GET_STAT(pl));
        act("$n hits your abdomen leaving you breathless!", TRUE, ch, 0, pl, TO_VICT);
        CharWait( pl, PULSE_VIOLENCE * 3 );
        return TRUE;
      }

      if (transact(pl, NULL, ch, 500))
      {
        act("$n says, 'Thank you for your purchase, enjoy the carnival.'", TRUE, ch, 0, pl, TO_VICT);
        act("$n hands $N a ticket and thanks $S for $M patronage.", TRUE, ch, 0, pl, TO_NOTVICT);
        char_from_room(pl);
        char_to_room(pl, real_room(82062), 0);
        act("$n is whisked away into the delights and the joys of the carnival.", TRUE, pl, 0, ch, TO_NOTVICT);

        return TRUE;
      }
    }
  }
  return FALSE;
}

int fooquest_boss(P_char ch, P_char pl, int cmd, char *arg)
{
  register P_char i;
  P_char   dragon;
  int      count = 0;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }
  if (!ch)
  {
    return FALSE;
  }
  if (IS_FIGHTING(ch))
  {
    /*
     * attempt to "summon" a silver dragon...only possible if less than BAHAMUT_HELP_LIMIT
     * * in world
     */
    for (i = character_list; i; i = i->next)
    {
      if ((IS_NPC(i)) && (GET_VNUM(i) == 65014))
      {
        count++;
      }
    }
    if (count < 5)
    {
      if (number(1, 100) < 50)
      {
        dragon = read_mobile(65014, VIRTUAL);
        if (!dragon)
        {
          logit(LOG_EXIT, "assert: error in bahamut() proc");
          raise(SIGSEGV);
        }
        act
          ("&+LThe air before you seems to rend and tear, revealing a black rift.&N\r\n"
           "An &+MIllithid&N stumbles out of &+Lthe wormhole&N.", FALSE, ch,
           0, dragon, TO_ROOM);
        char_to_room(dragon, ch->in_room, 0);
        return TRUE;
      }
    }
  }
  if (pl)
  {

    if (cmd == CMD_FLEE || cmd == CMD_RETREAT)
    {
      act
        ("$n &+Llooks at you and you feel your limbs numb, unable to carry you from the fight!&N",
         TRUE, ch, 0, pl, TO_VICT);
      act("$N &+Lprevents $n&+L from running with a mental blast!&N", TRUE,
          pl, 0, ch, TO_NOTVICT);
      return TRUE;
    }
  }

  return FALSE;

}
int fooquest_mob(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tempchar = NULL, was_fighting = NULL;
  P_obj    item, next_item;
  int      pos;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch))
    return FALSE;

  /*
   * if it's some command besides a periodic event call, return
   */
  if (!pl)
    return FALSE;

  if (IS_TRUSTED(pl))
  {
    if (cmd != CMD_USE)
      return FALSE;

    if (!isname(arg, "transform"))
      return FALSE;

    /*
     * load agthrodos
     */
    tempchar = read_mobile(65013, VIRTUAL);

    if (!tempchar)
    {
      logit(LOG_EXIT, "assert: mob load failed in xexos()");
      raise(SIGSEGV);
    }
    /*
     * stick agthrodos in same room
     */
    char_to_room(tempchar, ch->in_room, 0);

    /*
     * transfer inventory to agthrodos
     */
    if (RACE_EVIL(ch))
    {
      item = read_object(21, VIRTUAL);
    }
    else
    {
      item = read_object(22, VIRTUAL);
    }
    obj_to_char(item, tempchar);

    /*
     * let the player know what's going on
     */
    mobsay(ch, "You can DIE!");
    act("$n starts removing his disguise.", TRUE, ch, 0, 0, TO_ROOM);
    act
      ("An &+MIllithid&N invades your mind with 'Now I have the artifact, I don't need you anymore.'",
       TRUE, ch, 0, 0, TO_ROOM);

    /*
     * remove xexos
     */
    extract_char(ch);
    ch = NULL;
    return (TRUE);

  }
  else
  {
    return (FALSE);
  }
}

int get_map_room(int zone_id);

int world_quest(P_char ch, P_char pl, int cmd, char *arg)
{
  char     buf[MAX_INPUT_LENGTH];
  char     name[MAX_INPUT_LENGTH], what[MAX_STRING_LENGTH];
  char     money_string[MAX_INPUT_LENGTH];

  int temp = 0;
  float timediff, costmod;


  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !arg || !IS_ALIVE(pl) || IS_NPC(pl) )
  {
    return FALSE;
  }

  if( (cmd == CMD_ASK) && pl )
  {
    half_chop(arg, name, what);

    // ask <bartender> <quest|abandon|resign> ...
    if( ch != ParseTarget(ch, name) )
    {
      return FALSE;
    }

    if( isname( "abandon", what ) || isname( "resign", what ) )
    {

      if( pl->only.pc->quest_accomplished )
      {
        mobsay(ch, "Why would you like to resign when you done?!");
        return TRUE;
      }
      if( !pl->only.pc->quest_active )
      {
        mobsay(ch, "Abandon what quest, hmmmm!");
        return TRUE;
      }

      /* Can now abandon your quest at any bartender.
      if(pl->only.pc->quest_giver != GET_VNUM(ch))
      {
        mobsay(ch, "Abandon that quest at same place as you got it!");
        return TRUE;
      }
      */

      temp = (int)((get_property("world.quest.abandon.mod", 1.0) * GET_LEVEL(pl) * GET_LEVEL(pl) * GET_LEVEL(pl)));

      timediff = time(NULL) - pl->only.pc->quest_started;
      //debug("timediff: %f", timediff);
      costmod = 1.0 - (timediff / 60.0 / 60.0 / get_property("world.quest.cost.abandon.time", 24.000));
      costmod *= 100;
      //debug("costmod: %f", costmod);
      //debug("temp: %d", temp);
      temp = temp * BOUNDED(1, costmod, 100) / 100;
      //debug("temp: %d", temp);
      //debug("timediff: %f, hrsdiff: %f, costmod: %f, temp: %d, cost: %s", timediff, timediff / 60 / 60, costmod, temp, coin_stringv(temp));

      if( pl->only.pc->quest_type == FIND_AND_KILL && pl->only.pc->quest_kill_how_many > 0 )
      {
        // Allowing them to abandon at the cost of 1 quest (cost is done during the call to sql_world_quest_finished).
        if( !isname( "confirm", what ) )
        {
          send_to_char("You must be &+Wcrazy&N!  Try asking &+yabandon confirm&n if you really want to do that.\r\n", pl);
          return TRUE;
        }
      }

      sprintf(money_string, "OH NO, you've cost me alot of time and money, but toss me %s and I'll take care of your task!", coin_stringv(temp) );

      mobsay(ch, money_string);
      if (GET_MONEY(pl) < temp)
      {
        send_to_char("You dont have that much money...\r\n", pl);
        return (TRUE);
      }



      SUB_MONEY(pl, temp, 0);
      send_to_char("You hand over the money.\r\n", pl);

      send_to_char("You no longer have a task.\r\n", pl);
      // Make it so pl doesn't get the same quest again; once you fail, you fail.
      //   This also makes quest count as one of "today's quests."
      if( pl->only.pc->quest_type == FIND_AND_KILL && pl->only.pc->quest_kill_how_many > 0 )
      {
        sql_world_quest_finished(pl, 0);
      }
      resetQuest(pl);
      return TRUE;
    }

    if (isname(what, "map") || isname(arg, "m"))
    {

      if(pl->only.pc->quest_active != 1)
      {
        send_to_char("Maybe try getting a quest first?\r\n", pl);
        return TRUE;
      }

      int map_room = get_map_room(real_zone(pl->only.pc->quest_zone_number));
      //debug("do_quest(): quest_zone_number: %d, real_zone: %d, map_room: %d", pl->only.pc->quest_zone_number, real_zone(pl->only.pc->quest_zone_number), map_room);

      if( map_room <= 0 )
      {
        mobsay(ch, "Sorry, but I don't have any maps to that zone.");
        return TRUE;        
      }
      
      temp = 10 * GET_LEVEL(pl);

      sprintf(money_string, "Hmmmm, yeah, I might have a additional information for you, but I'm not giving it away for free! It'll cost you %s.", coin_stringv(temp) );

      mobsay(ch, money_string);
      if (GET_MONEY(pl) < temp)
      {
        send_to_char("You dont have the money, so you go can't get any additional information.\r\n", pl);
        return (TRUE);
      }

      SUB_MONEY(pl, temp, 0);
      send_to_char("You hand over the money.\r\n", pl);
      mobsay(ch, "Take a quick peek at this note:");
      quest_buy_map(pl);
      return TRUE;
    }


    if (!isname(what, "quest") && !isname(arg, "q"))      
      return FALSE;

    if(sql_world_quest_can_do_another(pl) < 1)
    {
      act
        ("$n says, 'Sorry, I don't have any more quests for right now.'",
         TRUE, ch, 0, pl, TO_VICT);

      return TRUE;
    }



    /*
       if(pl->only.pc->quest_accomplished != 1 &&
       pl->only.pc->quest_active == 1)
       {
       mobsay(ch, "Oh hmm unable to finish your quest? Why would you want another one then!");
       return TRUE;
       }	
     */
    if(pl->only.pc->quest_accomplished &&
        pl->only.pc->quest_giver == GET_VNUM(ch))
    {
      act
        ("$n says, 'Woah, nice work! Congratulations!'",
         TRUE, ch, 0, pl, TO_VICT);
      act
        ("$N says to $n, 'Well done!'",
         TRUE, pl, 0, ch, TO_NOTVICT);
      quest_full_reward(pl, ch, 1);
      return TRUE;
    }


    if(pl->only.pc->quest_accomplished)
    {
      act("$n says, 'Woah, nice work! But i didt give you this quest! Go find the real quest master'",
          TRUE, ch, 0, pl, TO_VICT);
      return TRUE;
    }

    if(pl->only.pc->quest_active == 1){
      mobsay(ch, "&+LBaaaaaah! Finish the quest that you're already on first, then come back!&n");
      send_to_char("&+LIf you unable to finish it, go to the quest master and ask him to take you of duty!\r\n", pl);
      return -1;
    }

    temp = 20 * GET_LEVEL(pl);

    sprintf(money_string, "Hmmmm, yeah, I might have a tip for you, but I'm not giving it away for free! It'll cost you %s.", coin_stringv(temp) );

    mobsay(ch, money_string);

    if (GET_MONEY(pl) < temp)
    {
      send_to_char("You dont have the money, so you go sulk in the corner.\r\n", pl);
      return (TRUE);
    }

    SUB_MONEY(pl, temp, 0);
    send_to_char("You hand over the money.\r\n", pl);

    if( createQuest(pl, ch) )
    {
      do_quest(pl, "", 0);
      mobsay(ch, "Remember, you can always type 'quest' to see your current quest.");
      return TRUE;
    }

    mobsay(ch, "Hmm, I'm unable to help you right now, try one of my colleagues around the world, or grab a few levels and come back.");
    send_to_char("\r\n&=LWYou get your money back.\r\n", pl);
    ADD_MONEY(pl, temp);
    return TRUE;
  }


  if (pl)
  {
    return (0);
  }

  switch (number(1, 15))
  {
    case 1:
      act("$n smells the fresh air.", TRUE, ch, 0, 0, TO_ROOM);
      break;

    case 2:
      strcpy(buf, "Come talk to me, I got some hot tips on quests for you!");
      do_yell(ch, buf, 0);
      break;
  }


  return FALSE;

}

int newbie_quest(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!ch || !cmd || !arg || !pl)
    return FALSE;

  if ((cmd == CMD_ASK && GET_LEVEL(pl) > 36) ||
      (IS_TRUSTED(pl) && cmd == CMD_ASK))
  {
    mobsay(ch,
           "&+wI am a questmob for the godquest for levels 1-35 only. Please let them be&+w for this is one of the rare occasions that a quest designed for low levels is&+w run and this quest will allow them to become more familiar with the mud to&+w become better players in the future.!");
    return TRUE;
  }
  return FALSE;

}

// From The Plains of Life - newbie zone.
int newbie_paladin(P_char ch, P_char pl, int cmd, char *arg)
{
  char     arg1[MAX_STRING_LENGTH];
  char     arg2[MAX_STRING_LENGTH];
  P_obj    sword;

  if( cmd != CMD_ASK || !IS_ALIVE(ch) || !IS_ALIVE(pl) || !arg )
  {
    return FALSE;
  }

  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);

  // If the first argument doesn't refer to the paladin, or the second arg isn't racewar and isn't racewars.
  if( !(get_char_room_vis(pl, arg1) == ch)
    || (strcmp(arg2, "racewar") && strcmp(arg2, "racewars")) )
  {
    return FALSE;
  }

  if( !affected_by_spell(pl, TAG_LIFESTREAMNEWBIE) )
  {
    mobsay(ch, "Go go in peace!");
    return TRUE;
  }
  affect_from_char(pl, TAG_LIFESTREAMNEWBIE);

  mobsay(ch, "&+bOgres&n, &+mDrow Elfs&n and &+gTrolls&n must die!&n");
  mobsay(ch, "Many evils died to this sword, it does me no good now, use it well.");
  mobsay(ch, "TYPE \"HELP RACEWAR\" for more information");

  sword = read_object(VOBJ_NEWBIE2_SWORD_BLESSED, VIRTUAL);
  act("$n gives $q to $N!", TRUE, ch, sword, pl, TO_NOTVICT);
  act("$n gives you $q!", TRUE, ch, sword, pl, TO_VICT);
  obj_to_char(sword, pl);

  mobsay(ch, "Here take this also, some items crafted by slaves in Bloodstone, maybe they will help you.");
  act("$n gives a lot of stuff to $N!", TRUE, ch, NULL, pl, TO_NOTVICT);
  act("$n gives you a lot of stuff.", TRUE, ch, NULL, pl, TO_VICT);
  load_obj_to_newbies(pl);
  return TRUE;
}


int Malevolence(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, vapor, vict = NULL;
  P_obj    item, next_item;
  int      numbPCs = 0, luckyPC = 0, currPC = 0, numb, pos, room;
  int      randroom;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if( cmd != CMD_PERIODIC )
    return FALSE;

  if( !IS_ALIVE(ch) )
  {
     return false;
  }

  if (!IS_FIGHTING(ch))
    return FALSE;

  if (!ch->specials.fighting)
    return FALSE;

  /* loop number of PC, pick someone */
  if (number(0, 49) == 0)
  {
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if (IS_PC(tch) && !IS_TRUSTED(tch))
      {
        if (number(0, 10) >= 5)
        {
          vict = tch;
          act("$n screams out in an unearthly howl '&+rI control reality! I control you! Begone from my domain and die within Celestia!' &n",
             FALSE, ch, 0, vict, TO_VICT);
          act("$n screams out in an unearthly howl '&+rI control reality! I control $N! Begone from my domain and die within Celestia!' &n",
             FALSE, ch, 0, vict, TO_NOTVICT);
          randroom = number(45500, 45520);
          char_from_room(vict);
          char_to_room(vict, real_room(randroom), -1);
        }
      }
    }
    return TRUE;
  }
  
  if (GET_HIT(ch) < (GET_MAX_HIT(ch) /3))
  {
    vapor = read_mobile(45571, VIRTUAL);
    GET_HIT(vapor) = GET_MAX_HIT(vapor) = vapor->points.base_hit = MAX(GET_HIT(ch)*4, 1500);
    if (!vapor)
    {
      logit(LOG_EXIT, "assert: mob load failed in Malevolence()");
      raise(SIGSEGV);
    }
    char_to_room(vapor, ch->in_room, 0);

    for (item = ch->carrying; item; item = next_item)
    {
      next_item = item->next_content;
      obj_from_char(item);
      obj_to_char(item, vapor); /*
                                 * transfer any eq and inv
                                 */
    }
    for (pos = 0; pos < MAX_WEAR; pos++)
    {
      if (ch->equipment[pos] != NULL)
      {
        item = unequip_char(ch, pos);
        equip_char(vapor, item, pos, TRUE);
      }
    }
    act("$n is dead! R.I.P.", TRUE, ch, 0, 0, TO_ROOM);
    act
      ("The corpse of &+wMa&+Llev&n&+rolen&n&+wce, the en&+Ltity of hav&n&+roc&n shudders in a spasm of death then glows with a blindling light.",
       FALSE, ch, 0, vapor, TO_NOTVICT);
    char_from_room(ch);
    char_to_room(ch, real_room(1), -1);
    act("$n screams out in an unearthly howl '&+rI LIVE!' &n", FALSE, vapor,
        0, vapor, TO_NOTVICT);
    die(ch, ch);
  }

  return FALSE;
}

int Malevolence_vapor(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, vict = NULL;
  int      numbPCs = 0, luckyPC = 0, currPC = 0, numb, pos;
  int      randroom;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!IS_FIGHTING(ch))
    return FALSE;

  if (!ch->specials.fighting)
    return FALSE;


  if (number(0, 24))
    return FALSE;

  /* loop number of PC, pick someone */

  switch (number(0, 9))
  {
  case 0:
  case 1:
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if (IS_PC(tch) && !IS_TRUSTED(tch))
      {
        if (number(0, 10) >= 5)
        {
          vict = tch;
          act("$n stares at $N as $e utters some uneartly incantations. &n",
              FALSE, ch, 0, vict, TO_NOTVICT);
          act
            ("$n stares at you while uttering some unearthly incantations. &n",
             FALSE, ch, 0, vict, TO_VICT);
          spell_chaotic_ripple(60, ch, 0, 0, vict, 0);
        }
      }
    }
    return TRUE;
  case 2:
  case 3:
    act("Your hands glow as you lay your hands on yourself.", FALSE, ch, 0, 0,
        TO_CHAR);
    act("$n's hands glow as $e lays $s hands on $mself.", FALSE, ch, 0, 0,
        TO_ROOM);
    GET_HIT(ch) += MIN(GET_LEVEL(ch) * 20, GET_MAX_HIT(ch) - GET_HIT(ch));
    return TRUE;
  case 4:
  case 5:
    for (vict = world[ch->in_room].people; vict; vict = tch)
    {
      tch = vict->next_in_room;

      if (ch == vict)
        continue;

      if (ch->group && vict->group && (ch->group == vict->group))
        continue;

      if (!CAN_SEE(ch, vict))
        continue;

      hit(ch, vict, ch->equipment[PRIMARY_WEAPON]);
    }
    return TRUE;
  case 6:
  case 7:
    if (!ch->specials.fighting)
      break;

    vict = ch->specials.fighting;

    act
      ("$n &+Ltouches $N&+L, draining his lifeforce and leaving $M&+L collapsed at $s feet.&n",
       FALSE, ch, 0, vict, TO_NOTVICT);
    act("$n &+Ltouches you. &+WOUCH!!!&n", FALSE, ch, 0, vict, TO_VICT);
    act("&+LYou feed upon $N&+L's blood.&n", FALSE, ch, 0, vict, TO_CHAR);

    GET_HIT(ch) += GET_HIT(vict);
    GET_HIT(vict) = -5;
    GET_VITALITY(vict) = 0;
    GET_MANA(vict) = 0;

    return TRUE;
  case 8:
  case 9:
    act("$n utters a word of power.&n", FALSE, ch, 0, 0, TO_ROOM);
    act("You utter a word of power.&n", FALSE, ch, 0, 0, TO_CHAR);
    spell_shadow_shield(60, ch, 0, 0, ch, 0);
    spell_stone_skin(60, ch, 0, 0, ch, 0);
    spell_biofeedback(60, ch, 0, 0, ch, 0);
    spell_inertial_barrier(60, ch, 0, 0, ch, 0);

    return TRUE;
  default:
    return FALSE;
  }

  return FALSE;
}

int celestia_pulsar(P_char ch, P_char pl, int cmd, char *arg)
{

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!IS_FIGHTING(ch))
    return FALSE;

  if (!number(0, 3))
  {
    spell_nova(60, ch, NULL, SPELL_TYPE_SPELL, 0, 0);
    return TRUE;
  }

  return FALSE;
}

int construct(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, next, vict;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!ch)
    return FALSE;

  //Do it 50 % of the time
  if (number(1, 100) < 50)
    return FALSE;

  if (ch->in_room == NOWHERE)
    return FALSE;
  if (!ch->specials.fighting)
    return FALSE;
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
  {

    if (ch->group && vict->group && (ch->group == vict->group))
      continue;
    if (IS_TRUSTED(ch) || (ch == vict))
      continue;
    else
    {
      if ((number(0, (SIZE_GARGANTUAN + 2)) - 2) > GET_ALT_SIZE(vict))
      {
        act("$n&+L picks up $N &+Land tosses $M &+Lagainst the wall!&n",
            FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n&+L picks you up and tosses you against the wall!&n",
            FALSE, ch, 0, vict, TO_VICT);
        SET_POS(vict, POS_PRONE + GET_STAT(vict));
        stop_fighting(vict);
        if( IS_DESTROYING(vict) )
          stop_destroying(vict);
        CharWait(vict, PULSE_VIOLENCE);
      }
    }
  }
  return TRUE;
}

int nyneth(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   fury;
  char     buf[256];

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!ch)
    return FALSE;


  if (ch->specials.fighting)
  {
    if (number(1, 100) < 10)
    {
      fury = read_mobile(38735, VIRTUAL);
      if (!fury)
      {
        logit(LOG_EXIT, "assert: error in nyneth proc");
        raise(SIGSEGV);
      }
      act("A &+rFuRy&n enters from somewhere.\r\n", FALSE, ch, 0, fury,
          TO_ROOM);
      char_to_room(fury, ch->in_room, 0);
      return TRUE;
    }
  }
  return FALSE;
}

int living_stone(P_char ch, P_char pl, int cmd, char *arg)
{
  if (!ch)
    return FALSE;
  
  return FALSE;
}

int elemental_swarm_fire(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, next, fury;
  P_char   vict;
  char     didit = FALSE;
  char     buf[256];

  if (cmd == CMD_SET_PERIODIC && ch->specials.fighting)
    return FALSE;

  if (!ch)
    return FALSE;
  //let's junk it on anything if not fighitng!
  if (cmd && ch->specials.fighting)
    return FALSE;


  if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
  {
    act("$n &+rlets forth a guttural &=LRROAR&+r!&n", TRUE, ch, 0, 0, TO_ROOM);
    spell_flamestrike(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
  }
  return FALSE;
}

int elemental_swarm_earth(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, next, fury;
  P_char   vict;
  char     didit = FALSE;
  char     buf[256];

  if (cmd == CMD_SET_PERIODIC && ch->specials.fighting)
    return FALSE;

  if (!ch)
    return FALSE;
  //let's junk it on anything if not fighitng!
  if (cmd && ch->specials.fighting)
    return FALSE;


  if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
  {
    act("$n &+rlets forth a guttural &=LRROAR&+r!&n", TRUE, ch, 0, 0, TO_ROOM);
    spell_earthen_maul(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
  }
  return FALSE;
}

int elemental_swarm_air(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, next, fury;
  P_char   vict;
  char     didit = FALSE;
  char     buf[256];

  if (cmd == CMD_SET_PERIODIC && ch->specials.fighting)
    return FALSE;

  if (!ch)
    return FALSE;
  //let's junk it on anything if not fighitng!
  if (cmd && ch->specials.fighting)
    return FALSE;


  if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
  {
    act("$n &+rlets forth a guttural &=LRROAR&+r!&n", TRUE, ch, 0, 0, TO_ROOM);
    spell_cyclone(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
  }
  return FALSE;
}

int elemental_swarm_water(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, next, fury;
  P_char   vict;
  char     didit = FALSE;
  char     buf[256];

  if (cmd == CMD_SET_PERIODIC && ch->specials.fighting)
    return FALSE;

  if (!ch)
    return FALSE;
  //let's junk it on anything if not fighitng!
  if (cmd && ch->specials.fighting)
    return FALSE;


  if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
  {
    act("$n &+rlets forth a guttural &=LRROAR&+r!&n", TRUE, ch, 0, 0, TO_ROOM);
    spell_dread_wave(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
  }
  return FALSE;
}
int shadow_monster(P_char ch, P_char pl, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_DEATH || (cmd == CMD_PERIODIC && (!number(0, GET_LEVEL(ch) / 3) || !ch->specials.fighting)) )
  {
    act("$n &+Lquickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
    act("$n &+rdisappears as &+Lquickly&+r as it came!", TRUE, ch, 0, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  return FALSE;
}

int insects(P_char ch, P_char pl, int cmd, char *arg)
{

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_DEATH || (cmd == CMD_PERIODIC && (!number(0, GET_LEVEL(ch) / 3) || !ch->specials.fighting)) )
  {
    act("$n &+Lquickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
    act("$n &+rdisappears as &+Lquickly&+r as it came!", TRUE, ch, 0, 0, TO_ROOM);
    {
      extract_char(ch);
    }
    return TRUE;
  }

  // Let's junk it on anything if not fighting!
  if( !ch->specials.fighting || cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  if( (ch->specials.fighting->in_room == ch->in_room) && !number(0, (MAXLVL - GET_LEVEL(ch))) )
  {
    /* This isn't used anywhere?
    int type;
    if (GET_LEVEL(ch) < 25)
      type = 1;
    else if (GET_LEVEL(ch) < 50)
      type = 3;
    else
      type = 10;
    */

    act("$n bites $N!", 1, ch, 0, ch->specials.fighting, TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, ch->specials.fighting, TO_VICT);
    poison_neurotoxin(10, ch, 0, 0, ch->specials.fighting, 0);
    return TRUE;
  }

  return FALSE;
}

int illus_dragon(P_char ch, P_char pl, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_DEATH || (cmd == CMD_PERIODIC && (!number(0, GET_LEVEL(ch) / 3) || !ch->specials.fighting)) )
  {
    act("$n &+Lquickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
    act("$n &+rdisappears as &+Lquickly&+r as it came!", TRUE, ch, 0, 0,
        TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  return FALSE;
}

int illus_titan(P_char ch, P_char pl, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_DEATH || (cmd == CMD_PERIODIC && (!number(0, GET_LEVEL(ch) / 3) || !ch->specials.fighting)) )
  {
    act("$n &+Lquickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
    act("$n &+rdisappears as &+Lquickly&+r as it came!", TRUE, ch, 0, 0,
        TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  return FALSE;
}

int greater_living_stone(P_char ch, P_char pl, int cmd, char *arg)
{
  if (!ch)
    return FALSE;
  
  return FALSE;
}



int imageproc(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == CMD_DEATH)
    return TRUE;

  if (!ch || IS_FIGHTING(ch) || cmd)
    return FALSE;

  if (!affected_by_spell(ch, SPELL_CHARM_PERSON))
  {
    act("&+L$n&+L quickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    extract_char(ch);
    return TRUE;
  }

  return FALSE;
}

int undead_dragon_east(P_char ch, P_char pl, int cmd, char *arg)
{
  ush_int  assoc;
  uint     bits;
  char    *tmp;
  int      dir = -1;
  int      badge = 0;
  int      allowed = 0;
  P_char   tch, next_ch;
  char     buf[MAX_STRING_LENGTH];
  char     temp[MAX_STRING_LENGTH];
  char     tmp2[MAX_STRING_LENGTH];
  int      is_avatar = FALSE;
  int      virt = 0;



  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  allowed = 0;

  if (!ch)
    return 0;

  if (!pl)
    return 0;


  if (!(cmd == CMD_EAST))
    return 0;

  if (IS_TRUSTED(pl))
    allowed = 1;
  else
    allowed = 0;

  if (allowed)
  {
    act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
    act("$N nods and stands aside to let you pass.",
        FALSE, pl, 0, ch, TO_CHAR);
    return (FALSE);
  }
  /*
   * BLOCK!
   */
  act("$N &+RROARS&+L at you while quickly blocking the exit!&n.", FALSE, pl,
      0, ch, TO_CHAR);
  act("$N &+RROARS&+L while quickly blocking the exit!&n.", FALSE, pl, 0, ch,
      TO_NOTVICT);
  return (TRUE);
}

/* this is hack n slashed from the pet baby dragon in dragonnia */
int undead_parrot(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tmp_ch;
  P_char   attacker;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;


  /*
   * check for periodic event call
   */

  if (cmd && pl)
  {
    if (pl == ch)
      return FALSE;
    switch (cmd)
    {
    case CMD_PET:
      do_action(pl, arg, CMD_PET);
      if (isname(arg, ch->player.name))
      {
        if (ch->following)
          stop_follower(ch);
        add_follower(ch, pl);
        group_add_member(pl, ch);
        act("$n says 'SQWAK!'.", 1, ch, 0, pl, TO_VICT);
      }
      return TRUE;
      break;
    case 25:                   /*
                                   kill
                                 */
    case 70:                   /*
                                   hit
                                 */
    case 154:                  /*
                                   back stab, bash, kick
                                 */
    case 157:
    case 159:
    case 236:                  /*
                                   murder
                                 */
      one_argument(arg, Gbuf1);
      if ((ch == get_char_room(Gbuf1, pl->in_room)) && (pl == ch->following))
      {
        strcpy(Gbuf2, pl->player.name);
/* add parrot ansi later */
        act
          ("&+LA friendly &+wsk&+Lele&+wtal &+Gp&+Ra&+Gr&+Rr&+Go&+Rt&N says 'SQWAK!' and flitters away for a moment.",
           1, ch->following, 0, ch, TO_ROOM);
        act
          ("&+LA friendly &+wsk&+Lele&+wtal &+Gp&+Ra&+Gr&+Rr&+Go&+Rt&N says 'SQWAK!' and flitters away for a moment.",
           1, ch->following, 0, ch, TO_CHAR);
        return TRUE;
      }
      break;
    default:
      return FALSE;
      break;
    }

    if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch))
      return FALSE;

    switch (number(0, 100))
    {
    case 0:
      act("$n says 'SQUAWK!", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    case 1:
      act("$n says 'Betcha Didn't Know I could talk.", TRUE, ch, 0, 0,
          TO_ROOM);
      return TRUE;
    case 2:
      act("$n says 'SQUAWK!", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    case 3:
      act("$n says 'SQUAWK!", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    case 4:
      act("$n says 'SQUAWK!", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}

#define LONGJOHNSILVER_HELPER_LIMIT     4


int long_john_silver_shout(P_char ch, P_char tch, int cmd, char *arg)
{
/* variables for summon proc */
  register P_char i;
  P_char   ljswraith;
  P_char   vict;
  int      count = 0;

/* variables for shout proc */
  int      helpers[] =
    { 70536, 70537, 70538, 70539, 70540, 70541, 70547, 70548, 0 };

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
  {
    return FALSE;
  }

  if (cmd != 0)
  {
    return FALSE;
  }

/* summon proc */
  if (IS_FIGHTING(ch))
  {
    /*
     * attempt to "summon" a wraith pirate...only possible if less than LONGJOHNSILVER_HELP_LIMIT
     * * in world
     */
    for (i = character_list; i; i = i->next)
    {
      if ((IS_NPC(i)) && (GET_VNUM(i) == 70536))
      {
        count++;
      }
    }
    if (count < LONGJOHNSILVER_HELPER_LIMIT)
    {
      if (number(1, 100) < 50)
      {
        ljswraith = read_mobile(70536, VIRTUAL);
        if (!ljswraith)
        {
          logit(LOG_EXIT, "assert: error in longjohnsilver() proc");
          wizlog(MINLVLIMMORTAL, "error in proc longjohnsilver");
          return FALSE;
        }
        act("$n &+Lraises his hands in the air..&n\r\n"
            "&+LThe wraith of a pirate appears out of thin air!&n\r\n",
            FALSE, ch, 0, ljswraith, TO_ROOM);
        char_to_room(ljswraith, ch->in_room, 0);
        vict = ch->specials.fighting;   /* lets make our pets fight something! */
        MobStartFight(ljswraith, vict);
        return TRUE;
      }
      else                      /* if can't summon, may as well yell for help */
        return shout_and_hunt(ch, 100,
                              "&+LArr! All hands come slay the intruder &+W%s &+LArr!&N",
                              NULL, helpers, 0, 0);
    }
  }

  return FALSE;

}

#undef LONGJOHNSILVERHELPER_LIMIT



int pirate_talk(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Arr! This be my vessel, begone!");
    do_action(ch, 0, CMD_STARE);
    return TRUE;
  case 1:
    mobsay(ch, "You shall meet the sharp edge of my blade Arr!");
    return TRUE;
  case 2:
    mobsay(ch, "Yo Ho Ho and a Bottle of Rum");
    do_action(ch, 0, CMD_HICCUP);
    return TRUE;
  case 3:
    act("$n stumbles around bumping into things.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_HICCUP);
    return TRUE;
  case 4:
    mobsay(ch, "A pirates life for me... A pirates life for me..");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  }
  return FALSE;
}

int pirate_female_talk(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Arr! Get outta my room!");
    do_action(ch, 0, CMD_STARE);
    return TRUE;
  case 1:
    mobsay(ch, "Arr! I'm goin' to kick your ass!");
    return TRUE;
  case 2:
    mobsay(ch, "You think you've had a bad day? Just look at my skin! Arr!");
    do_action(ch, 0, CMD_CRY);
    return TRUE;
  case 3:
    mobsay(ch, "I haven't had a REAL Jolly Roger for years...");
    do_action(ch, 0, CMD_MOAN);
    return TRUE;
  case 4:
    mobsay(ch, "A pirates life for me... A pirates life for me..");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 5:
    act("$n looks at you and giggles.", TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch,
           "I'm going to have to borrow the Lookout's telescope for this one.");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  }
  return FALSE;
}

int pirate_cabinboy_talk(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Is the poop deck really what I think it is?");
    do_action(ch, 0, CMD_PUZZLE);
    return TRUE;
  case 1:
    act("$n sings 'In the Navy!", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "Yo Ho Ho and a Bottle of Milk");
    return TRUE;
  case 3:
    act("$n air fences with his broom.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n pokes himself in the eye.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "A cabin boy's life for me... A cabin boy's life for me..");
    return TRUE;
  case 5:
    mobsay(ch,
           "Swab this.. Swab that.... If he tells me to swab one more thing I'll swab HIM!");
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  }
  return FALSE;
}


int shabo_butler(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   i, i_next, tempchar = NULL, tempchar2 = NULL, was_fighting = NULL;
  P_desc   d;
  P_obj    item, next_item;
  P_char   gunnadie;
  int      pos;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch))
    return FALSE;

  /*
   * if it's some command besides a periodic event call, return
   */
  if (cmd)
    return FALSE;

  if (IS_FIGHTING(ch))
  {
    was_fighting = ch->specials.fighting;
    gunnadie = ch->specials.fighting;
    stop_fighting(ch);

    tempchar = read_mobile(32844, VIRTUAL);

    if (!tempchar)
    {
      logit(LOG_EXIT, "assert: mob load failed in shabo_butler()");
      wizlog(MINLVLIMMORTAL, "error in proc shabo_butler");
      return FALSE;
    }
    char_to_room(tempchar, ch->in_room, -2);
    for (item = ch->carrying; item; item = next_item)
    {
      next_item = item->next_content;
      obj_from_char(item);
      obj_to_char(item, tempchar);      /*
                                         * transfer any eq and inv
                                         */
    }
    for (pos = 0; pos < MAX_WEAR; pos++)
    {
      if (ch->equipment[pos] != NULL)
      {
        item = unequip_char(ch, pos);
        equip_char(tempchar, item, pos, TRUE);
      }
    }

    act("The $n suddenly drops to the floor, howling in pain!", 0, ch, 0, 0,
        TO_ROOM);
    act("A moment later, $e trasforms into $N!", 1, ch, 0, tempchar, TO_ROOM);
    act("$n throws back $s head, and lets out a long howl.", 0, tempchar, 0,
        0, TO_ROOM);

    extract_char(ch);
    ch = NULL;

    /*
     * Howl for help, similar to echoz
     */
    for (d = descriptor_list; d; d = d->next)
    {
      if (d->connected == CON_PLYNG)
      {
        if (world[tempchar->in_room].zone ==
            world[d->character->in_room].zone)
        {
          send_to_char("A bloodcurdling howl is heard!", d->character);
          send_to_char("\r\n", d->character);
        }
      }
    }

    /*
     * Assistants in other room change now, and begin to come help
     */
    for (i = character_list; i; i = i_next)
    {
      i_next = i->next;
      if (IS_NPC(i) && (GET_VNUM(i) == 32841))
      {
        tempchar2 = read_mobile(32844, VIRTUAL);
        if (!tempchar2)
        {
          logit(LOG_EXIT, "assert: second mob load failed in shabo_butler()");
          wizlog(MINLVLIMMORTAL, "error in proc shabo_butler (second mobs)");
          return FALSE;
        }
        act("The $n suddenly drops to the floor, howling in pain!", 0, i, 0,
            0, TO_ROOM);
        act("A moment later, $e trasforms into $N!", 1, i, 0, tempchar2,
            TO_ROOM);
        act("$n throws back $s head, and lets out a long howl.", 0, tempchar2,
            0, 0, TO_ROOM);

        char_to_room(tempchar2, i->in_room, -2);
        if (!IS_SET(tempchar2->specials.act, ACT_HUNTER))
          SET_BIT(tempchar2->specials.act, ACT_HUNTER);

        for (item = i->carrying; item; item = next_item)
        {
          next_item = item->next_content;
          obj_from_char(item);
          obj_to_char(item, tempchar2);
        }
        for (pos = 0; pos < MAX_WEAR; pos++)
        {
          if (i->equipment[pos] != NULL)
          {
            item = unequip_char(i, pos);
            equip_char(tempchar2, item, pos, TRUE);
          }
        }

        /*
         * Code for memory (from set_fighting), this will make the converted
         * werewolves hunt.
         */

        if (tempchar2 && was_fighting)
        {
          if (HAS_MEMORY(tempchar2))
          {
            if (IS_PC(was_fighting))
            {
              if (!
                  (IS_TRUSTED(was_fighting) &&
                   IS_SET(was_fighting->specials.act, PLR_AGGIMMUNE)))
                if ((GET_STAT(tempchar2) > STAT_INCAP))
                  remember(tempchar2, was_fighting);
            }
            else if (IS_PC_PET(was_fighting) &&
                     (GET_MASTER(was_fighting)->in_room ==
                      was_fighting->in_room) &&
                     CAN_SEE(tempchar2, GET_MASTER(was_fighting)))
            {
              if (!(IS_TRUSTED(GET_MASTER(was_fighting)) &&
                    IS_SET(GET_MASTER(was_fighting)->specials.act,
                           PLR_AGGIMMUNE)))
                if ((GET_STAT(tempchar2) > STAT_INCAP))
                  remember(tempchar2, GET_MASTER(was_fighting));
            }
          }
        }
        extract_char(i);        /*
                                 * Set them hunting players, wherever they may
                                 * be
                                 */
      }
    }

//    if (was_fighting)
    MobStartFight(tempchar, gunnadie);

    return TRUE;
  }
  return FALSE;
}

int shabo_caran(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 32835, 32836, 0 };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100,
                          "&+MBrethren, we have been invaded, come to me and dispose of this filth!!&N",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int cow_talk(P_char ch, P_char tch, int cmd, char *arg)
{

  if (cmd == CMD_MOUNT)
  {
    if (GET_LEVEL(tch) < MINLVLIMMORTAL)
    {
      act("$n growls as you try to mount $m.  You rethink the idea.",
          FALSE, ch, 0, tch, TO_VICT);
      act("$n growls as $N tries to mount $m.  $N rethinks the idea.",
          TRUE, ch, 0, tch, TO_NOTVICT);
      return (TRUE);
    }
    return (FALSE);
  }

  /*
   * check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 40))
  {
  case 0:
    mobsay(ch, "Moooooooooooooooooo");
    return TRUE;
  case 1:
    mobsay(ch,
           "MooooooooooooooooooOOOOOOOOOoooooooooooooOOOOOOOOOooooooooO!");
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_MOON);
    mobsay(ch, "Hmm! That's not moo.");
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_COW);
    do_action(ch, 0, CMD_COW);
    do_action(ch, 0, CMD_COW);
    mobsay(ch, "Muhahahaha");
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 4:
    act("$n looks at you.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n sizes you up with a quick glance.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n whispers to you 'Moo'", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 5:
    mobsay(ch, "I wasn't always a cow you know.");
    do_action(ch, 0, CMD_HICCUP);
    mobsay(ch, "I was a champion boxer, now look at me.");
    do_action(ch, 0, CMD_HICCUP);
    return TRUE;
  }
  return FALSE;
}


int annoying_mob(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   temp;
  static int songcounter = 0;

  /*
   * check for periodic event calls
   */

  if (cmd == CMD_DEATH)
  {
    temp = read_mobile(GET_RNUM(ch), REAL);
    if (temp)
    {
      char_to_room(temp, ch->in_room, 0);
    }
    act
      ("An aura of intensely bright light surrounds &+Lan &+runk&+Rilla&+rble &+Lbastard&N for a moment.",
       TRUE, ch, 0, 0, TO_ROOM);
    act
      ("&+LAn &+runk&+Rilla&+rble &+Lbastard&N comes to life again! Taking a deep breath, &+Lan &+runk&+Rilla&+rble &+Lbastard&N opens its eyes!",
       TRUE, ch, 0, 0, TO_ROOM);
  }

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;
  if (cmd == 0)
  {
    switch (songcounter)
    {
    case 0:
      act("$n sings 'This is the song that never ends...'", FALSE, ch, 0, 0,
          TO_ROOM);
      songcounter++;
      break;
    case 1:
      act("$n sings 'Yes it goes on and on my friend...'", FALSE, ch, 0, 0,
          TO_ROOM);
      songcounter++;
      break;
    case 2:
      act
        ("$n sings 'Some people started singing it, not knowing what it was...'",
         FALSE, ch, 0, 0, TO_ROOM);
      songcounter++;
      break;
    case 3:
      act
        ("$n sings 'And they'll continue singing it forever just because...'",
         FALSE, ch, 0, 0, TO_ROOM);
      songcounter = 0;
      break;
    }
  }
  return (FALSE);
}

int shabo_petre(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   i, i_next, tempchar = NULL, tempchar2 = NULL, was_fighting = NULL;
  P_desc   d;
  P_obj    item, next_item;
  P_char   gunnadie;
  int      pos;
  int      helpers[] = { 32841, 32844, 32840, 0 };

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;               //TRUE;

  if(!ch ||
    !IS_AWAKE(ch) ||
    cmd)
  {
    return FALSE;
  }

  if(IS_FIGHTING(ch) &&
     ch->in_room != real_room(32885))
  {
    was_fighting = ch->specials.fighting;
    gunnadie = ch->specials.fighting;

    act("\n&+YHELP! Anyone please help! $N just hit me! HELP!\n&n", 0, ch, 0, gunnadie, TO_ROOM);
    stop_fighting(ch);
    act("$n fades from sight!!!", 0, ch, 0, 0, TO_ROOM);

    /* move the mob */
    char_from_room(ch);
    char_to_room(ch, real_room(32885), 0);
    GET_HOME(ch) = GET_BIRTHPLACE(ch) = GET_ORIG_BIRTHPLACE(ch) = real_room(32885);
    
    /* in target room make it look like a recall */
    act("$n &+WFades into the room with a nasty expression.&n",
        0, ch, 0, 0, TO_ROOM);
    
/*  I have no idea why this proc loads an earth elemental and starts fighting with it.
    I've commented this portion out. Apr09 -Lucrot
    tempchar2 = read_mobile(1101, VIRTUAL);
    char_to_room(tempchar2, real_room(32885), 0);

    MobStartFight(ch, tempchar2);
    shout_and_hunt(ch, 100,"&+YHELP! Anyone please help! %s just hit me! HELP!&n", NULL,
                   helpers, tempchar2, 0); */


    for (i = character_list; i; i = i_next)
    {
      i_next = i->next;
      
      if(IS_NPC(i) &&
        (GET_VNUM(i) == 32841))
      {
        tempchar2 = read_mobile(32844, VIRTUAL);

        if (!tempchar2)
        {
          logit(LOG_EXIT, "assert: second mob load failed in shabo_petre()");
          wizlog(MINLVLIMMORTAL, "error in proc shabo_petre");
          return FALSE;
        }
        
        act("The $n suddenly drops to the floor, howling in pain!",
          0, i, 0, 0, TO_ROOM);
        act("A moment later, $e trasforms into $N!",
          1, i, 0, tempchar2, TO_ROOM);
        act("$n throws back $s head, and lets out a long howl.",
          0, tempchar2, 0, 0, TO_ROOM);

        char_to_room(tempchar2, i->in_room, -2);

        if(!IS_SET(tempchar2->specials.act, ACT_HUNTER))
        {
          SET_BIT(tempchar2->specials.act, ACT_HUNTER);
        }

        for (item = i->carrying; item; item = next_item)
        {
          next_item = item->next_content;
          obj_from_char(item);
          obj_to_char(item, tempchar2);
        }

        for (pos = 0; pos < MAX_WEAR; pos++)
        {
          if (i->equipment[pos] != NULL)
          {
            item = unequip_char(i, pos);
            equip_char(tempchar2, item, pos, TRUE);
          }
        }
      }
    }
  }
  return FALSE;
}


int shabo_evilpetre(P_char ch, P_char pl, int cmd, char *arg)
{
  return FALSE;
}


/* ako procs */

/* mob 3715 */
int ako_hypersquirrel(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n runs around all over the place!", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act
      ("$n runs runs up your leg, up your back, around your neck, then leaps off and runs around some more.",
       TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act("$n runs really fast, slamming right into a tree!", TRUE, ch, 0, 0,
        TO_ROOM);
    return TRUE;
  case 3:
    act("$n makes some soft squirrel noises.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}

/* mob 3701 */
int ako_songbird(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;


  if (cmd && pl)
  {
    if (pl == ch)
      return FALSE;
    switch (cmd)
    {
    case CMD_PET:
      do_action(pl, arg, CMD_PET);
      act("$n makes a soft soothing sound then flies away.'.", 1, ch, 0, pl,
          TO_VICT);
    }
  }

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;



  switch (number(0, 100))
  {
  case 0:
    act("$n sings a beautiful song.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n pecks at the ground.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act
      ("$n flies really high up in the air, dive bombs at the ground and lands flawlessly!",
       TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_FLEX);
    return TRUE;
  case 3:
    act("$n sings a little more.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}

/* mob 3716 */
int ako_vulture(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n rips a bit of flesh out of the corpse and swallows it whole.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n pulls out a large maggot and eats it.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act("$n looks in your direction, you feel a little uneasy about that.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}

/* mob 3721 */
int ako_wildmare(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   mount;
  int      movescost;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n swats its tail at a fly.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n takes a mouthful of grass and starts chewing.", TRUE, ch, 0, 0,
        TO_ROOM);
    return TRUE;
  }
  return FALSE;
}



/* mob 3720 */
int ako_cow(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 5:
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 6:
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 7:
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 8:
    do_action(ch, 0, CMD_COW);
    return TRUE;
  case 9:
    act("$n moos at you!.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 10:
    act("$n swats its tail at a fly.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}

int raoul(P_char ch, P_char pl, int cmd, char *arg)
{
  char     buf[20];
  static int songcounter = 0;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;
  strcpy(buf, "christine");

  if (cmd == 0)
  {
    switch (songcounter)
    {
    case 0:
      do_action(ch, buf, CMD_CALM);
      break;
    case 1:
      do_action(ch, buf, CMD_STARE);
      break;
    case 2:
      act("$n sings 'No more talk of darkness, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      act("$n sings 'Forget these wide-eyed fears.'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 4:
      act("$n sings 'I'm here, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      act("$n sings 'nothing can harm you --'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 6:
      act("$n sings 'my words will warm and calm you.'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 7:
      act("$n sings 'Let me be your freedom, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 8:
      act("$n sings 'let daylight dry your tears.'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 9:
      act("$n sings 'I'm here, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 10:
      act("$n sings 'with you, beside you, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 11:
      act("$n sings 'to guard you and to guide you...'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 17:
      act("$n sings 'Let me be your shelter, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 18:
      act("$n sings 'let me be your light.'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 19:
      act("$n sings 'You're safe:'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 20:
      act("$n sings 'No-one will find you --'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 21:
      act("$n sings 'your fears are far behind you...'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 27:
      act
        ("$n sings 'Then say you'll share with me one love, one lifetime...'",
         FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 28:
      act("$n sings 'let me lead you from your solitude...'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 29:
      act("$n sings 'Say you need me with you here, beside you...'", FALSE,
          ch, 0, 0, TO_ROOM);
      break;
    case 30:
      act("$n sings 'anywhere you go, let me go too --'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 31:
      act("$n sings 'Christine, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 32:
      act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 35:
      act("$n sings 'Share each day with me, each night, each morning...'",
          FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 37:
      act("$n sings 'You know I do...'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 38:
      act("$n sings 'Love me --'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 39:
      act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 40:
      do_action(ch, buf, CMD_SMILE);
      break;
    case 41:
      act("$n sings 'Anywhere you go let me go too...'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 42:
      act("$n sings 'Love me --'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 43:
      act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 45:
      do_action(ch, 0, CMD_BOW);
      break;
    default:
      break;
    }
    songcounter++;
    if (songcounter >= 50)
      songcounter = 0;
  }
  return (FALSE);
}


int christine(P_char ch, P_char pl, int cmd, char *arg)
{
  char     buf[20];

  static int songcounter = 0;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;
  strcpy(buf, "raoul");
  if (cmd == 0)
  {
    switch (songcounter)
    {
    case 1:
      do_action(ch, buf, CMD_STARE);
      break;
    case 12:
      act("$n sings 'Say you love me every waking moment, '", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 13:
      act("$n sings 'turn my head with talk of summertime...'", FALSE, ch, 0,
          0, TO_ROOM);
      break;
    case 14:
      act("$n sings 'Say you need me with you, now and always...'", FALSE, ch,
          0, 0, TO_ROOM);
      break;
    case 15:
      act("$n sings 'promise me that all you say is true --'", FALSE, ch, 0,
          0, TO_ROOM);
      break;
    case 16:
      act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 22:
      act("$n sings 'All I want is freedom, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 23:
      act("$n sings 'a world with no more night...'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 24:
      act("$n sings 'and you, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 25:
      act("$n sings 'always beside me, '", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 26:
      act("$n sings 'to hold me and to hide me...'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 33:
      act("$n sings 'Say you'll share with me one love, one lifetime...'",
          FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 34:
      act("$n sings 'say the word and I will follow you...'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 35:
      act("$n sings 'Share each day with me, each night, each morning...'",
          FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 36:
      act("$n sings 'Say you love me...'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 38:
      act("$n sings 'Love me --'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 39:
      act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 40:
      do_action(ch, buf, CMD_SMILE);
      break;
    case 41:
      act("$n sings 'Anywhere you go let me go too...'", FALSE, ch, 0, 0,
          TO_ROOM);
      break;
    case 42:
      act("$n sings 'Love me --'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 43:
      act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 45:
      do_action(ch, 0, CMD_CURTSEY);
      break;
    default:
      break;
    }
    songcounter++;
    if (songcounter >= 50)
      songcounter = 0;
  }
  return (FALSE);
}

int cookie_monster(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   temp;
  static int songcounter = 0;

  /*
   * check for periodic event calls
   */

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;
  if (cmd == 0)
  {
    switch (songcounter)
    {
    case 0:
      act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch,
          0, 0, TO_ROOM);
      songcounter++;
      break;
    case 1:
      act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch,
          0, 0, TO_ROOM);
      songcounter++;
      break;
    case 2:
      act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch,
          0, 0, TO_ROOM);
      songcounter++;
      break;
    case 3:
      act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch,
          0, 0, TO_ROOM);
      songcounter++;
      break;
    case 4:
      act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch,
          0, 0, TO_ROOM);
      songcounter++;
      break;
    case 5:
      act("$n sings 'Oh, cookie, cookie, cookie starts with C", FALSE, ch, 0,
          0, TO_ROOM);
      songcounter = 0;
      break;
    }
  }
  return (FALSE);
}

int necro_specpet_bone(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;
  int      room;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
  {
    room = ch->in_room;
    vict = ch->specials.fighting;

    act("$n&+L cackles with delight!", FALSE, ch, 0, vict, TO_ROOM);
    spell_energy_drain(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
  }
  return FALSE;

}

int necro_specpet_blood(P_char ch, P_char pl, int cmd, char *arg)
{
}

int necro_specpet_flesh(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;
  int      curr_time;
  int      proctimer;

  struct damage_messages acid_blood = {
    "&+L$N &+Lwrithes in agony the black blood greedily eats into $S &+rflesh.&n",
    "&+LThe world &+rexp&+Rlo&+rdes in p&+Ra&+rin &+Las the black blood greedily eats into your &+rflesh.",
    "&+L$N &+Lwrithes in agony the black blood greedily eats into $S &+rflesh.&n",
    "What was once $N, but now only a mass of burnt flesh, crumbles in a heap on the ground.",
    "A pain beyond imagination overwhelms you as the black blood eats its ways into your heart.",
    "What was once $N, but now only a mass of burnt flesh, crumbles in a heap on the ground."
  };

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;


  if (IS_FIGHTING(ch) && (cmd == 0) && !number(0, 19))
  {
    vict = ch->specials.fighting;

    act
      ("&+LBlack blood &+wspurts from your wound as $N&+w's weapon &+wrips your &+rflesh.&n",
       FALSE, ch, 0, vict, TO_CHAR);
    act
      ("&+LBlack blood &+wspurts from $n &+was your weapon &+wrips $s &+rflesh.&n",
       FALSE, ch, 0, vict, TO_VICT);
    act
      ("&+LBlack blood &+wspurts from $n &+was $N&+w's weapon &+wrips $s &+rflesh.&n",
       FALSE, ch, 0, vict, TO_NOTVICT);


    if ((15 + GET_C_AGI(vict) / 6) > number(0, 100))
    {
      act
        ("&+w$N &+wjumps out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
         FALSE, vict, 0, ch, TO_CHAR);

      act
        ("&+wYou jump out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
         FALSE, vict, 0, ch, TO_VICT);

      act
        ("&+w$N &+wjumps out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
         FALSE, vict, 0, ch, TO_NOTVICT);

    }
    else
    {
      spell_damage(ch, vict, 40 + number(1, 40), SPLDAM_ACID,
                   SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &acid_blood);
    }
  }

  return FALSE;
}

int conj_specpet_xorn(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;
  int      curr_time;
  int      proctimer;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 10) == 1))
  {
    vict = ch->specials.fighting;
    if ((GET_SIZE(vict) >= SIZE_TINY) && (GET_SIZE(vict) <= SIZE_GIANT))
    {
      if ((GET_POS(vict) == POS_PRONE) || (GET_POS(vict) == POS_SITTING) ||
          (GET_POS(vict) == POS_KNEELING))
      {
        act
          ("$n&+y rears up and dives into the ground sending fragments flying.",
           FALSE, ch, 0, vict, TO_NOTVICT);
        act("&+yMoments later it bursts forth charging into $N.", FALSE, ch,
            0, vict, TO_NOTVICT);
        act("$N &+ycannot be knocked down any further!", FALSE, ch, 0, vict,
            TO_NOTVICT);

        act
          ("$n&+y rears up and dives into the ground sending fragments flying.",
           FALSE, ch, 0, vict, TO_VICT);
        act("&+yMoments later it bursts forth charging into you!", FALSE, ch,
            0, vict, TO_VICT);
        act("&+yYou cannot be knocked down any further!", FALSE, ch, 0, vict,
            TO_VICT);
      }
      else
      {
        act
          ("$n&+y rears up and dives into the ground sending fragments flying.",
           FALSE, ch, 0, vict, TO_NOTVICT);
        act("&+yMoments later it bursts forth charging into an $N.", FALSE,
            ch, 0, vict, TO_NOTVICT);
        act("$N &+yis flung to the ground by $n!", FALSE, ch, 0, vict,
            TO_NOTVICT);

        act
          ("$n&+y rears up and dives into the ground sending fragments flying.",
           FALSE, ch, 0, vict, TO_VICT);
        act("&+yMoments later it bursts forth charging into you!", FALSE, ch,
            0, vict, TO_VICT);
        act("&+yYou are flung to the ground by $n!", FALSE, ch, 0, vict,
            TO_VICT);
        SET_POS(vict, POS_SITTING + GET_STAT(vict));
        CharWait(vict, PULSE_VIOLENCE * 1);
      }
    }
    else
    {
      act
        ("$n&+y rears up and dives into the ground sending fragments flying.",
         FALSE, ch, 0, vict, TO_NOTVICT);
      act("&+yMoments later it bursts forth charging into an $N.", FALSE, ch,
          0, vict, TO_NOTVICT);
      act
        ("$n&+y rears up and dives into the ground sending fragments flying.",
         FALSE, ch, 0, vict, TO_VICT);
      act("&+yMoments later it bursts forth charging into you!.", FALSE, ch,
          0, vict, TO_VICT);

      if (GET_SIZE(vict) > SIZE_GIANT)
      {
        act("$N &+yis simply too large to be knocked down by $n!", FALSE, ch,
            0, vict, TO_NOTVICT);
        act("&+yYou are simply too large to be knocked down by $n!", FALSE,
            ch, 0, vict, TO_VICT);
      }
      if (GET_SIZE(vict) < SIZE_TINY)
      {
        act("$N &+yis simply too small to be knocked down by $n!", FALSE, ch,
            0, vict, TO_NOTVICT);
        act("&+yYou are simply too small to be knocked down by $n!", FALSE,
            ch, 0, vict, TO_VICT);
      }

    }
    return FALSE;
  }
  return FALSE;
}

int conj_specpet_golem(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;
  int      temp = 0;
  int      healpoints, door, target_room, in_room;


  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 10) == 1))
  {
    vict = ch->specials.fighting;
    // whee bearhug!
    
    if(vict->in_room != ch->in_room)
        return false;

    if (GET_SIZE(vict) > SIZE_GIANT)
      return FALSE;

    if (GET_SIZE(vict) <= SIZE_SMALL)
    {
      act("&+y$n &+ygrabs &+Y$N&+y by the &+Yhead &+yand &+Wthrows &+y$M&+y!",
          FALSE, ch, 0, vict, TO_NOTVICT);
      act("&+y$n &+ygrabs &+Yyou&+y by the &+Yhead &+yand &+Wthrows &+yyou!",
          FALSE, ch, 0, vict, TO_VICT);

      door = number(0, 9);

      if ((door == UP) || (door == DOWN))
        door = number(0, 3);

      if ((CAN_GO(vict, door)) && (!check_wall(vict->in_room, door)))
      {
        act("$n &+yflings &+Yyou &+yout of the room!", FALSE, ch, 0, vict,
            TO_VICT);
        act("$n &+yflings &+Y$N &+yout of the room!", FALSE, ch, 0, vict,
            TO_NOTVICT);
        target_room = world[vict->in_room].dir_option[door]->to_room;
        char_from_room(vict);
        if (char_to_room(vict, target_room, -1))
        {
          act("$n &+Yflies in &+Yface first&+y, crashing on the floor!", TRUE,
              vict, 0, 0, TO_ROOM);
          stop_fighting(vict);
          if( IS_DESTROYING(vict) )
            stop_destroying(vict);
          SET_POS(vict, POS_SITTING + GET_STAT(vict));
          CharWait(vict, PULSE_VIOLENCE * 1);
        }
      }
      else
      {
        act
          ("&+Y$N &+yflies into the &+Ywall &+ybreaking &+wbones &+yand landing with a stunning &+Yforce&+y.",
           FALSE, ch, 0, vict, TO_NOTVICT);
        act
          ("&+YYou &+yfly into the &+Ywall &+ybreaking &+wbones &+yand land with a stunning &+Yforce&+y.",
           FALSE, ch, 0, vict, TO_VICT);
        SET_POS(vict, POS_SITTING + GET_STAT(vict));
        healpoints = (number(1, 30));
        CharWait(vict, PULSE_VIOLENCE * 1);
        GET_HIT(vict) -= healpoints;
        healCondition(vict, healpoints);
        update_pos(vict);
      }

    }

    if (!CanDoFightMove(ch, vict))
      return FALSE;

    if (GET_ALT_SIZE(vict) > GET_ALT_SIZE(ch))
    {
      act("$n tries to squeeze the life out of $N, but can't get a grip.",
          FALSE, ch, 0, vict, TO_NOTVICT);
      act("You might as well try to hug a mountain!",
          FALSE, ch, 0, vict, TO_CHAR);
      act("$n tried to wrap his arms around you, but of course failed.",
          FALSE, ch, 0, vict, TO_VICT);
      CharWait(ch, PULSE_VIOLENCE * 2);
      return FALSE;
    }

    if (GET_ALT_SIZE(vict) < GET_ALT_SIZE(ch) - 2)
    {
      send_to_char("Don't hug that - simply swat it!\r\n", ch);
      return FALSE;
    }

    if (IS_SLIME(vict) || GET_RACE(vict) == RACE_AQUATIC_ANIMAL ||
        GET_RACE(vict) == RACE_SNAKE /*|| GET_RACE(vict) == RACE_FLYING_ANIMAL */ )
    {
      send_to_char("You just can't get a good grip on that.\r\n", ch);
      return FALSE;
    }

    if (GET_POS(vict) != POS_STANDING)
    {
      send_to_char
        ("Your enemy is not standing, you can't grab him right.\r\n", ch);
      return FALSE;
    }

    temp = (GET_LEVEL(vict) - GET_LEVEL(ch)) / 10 +
      (GET_C_AGI(vict) - GET_C_AGI(ch)) / 15 +
      (GET_C_STR(vict) - GET_C_STR(ch)) / 15;

    temp -= GET_AC(vict) / 20;

    if (IS_AFFECTED(vict, AFF_BLIND) || IS_AFFECTED(vict, AFF_KNOCKED_OUT) ||
        IS_AFFECTED(vict, AFF_SLEEP) || IS_AFFECTED(vict, AFF_MEDITATE) ||
        IS_AFFECTED2(vict, AFF2_SLOW) || IS_AFFECTED2(vict, AFF2_STUNNED))
      temp -= 5;
    if (IS_AFFECTED(ch, AFF_HASTE))
      temp -= 5;

    temp = BOUNDED(1, number(1, 101) + temp, 101);


    if (temp == 1)
    {
      act
        ("$n&+y squeezes $N &+yso hard, you can hear $N&+y's bones creaking!",
         FALSE, ch, 0, vict, TO_NOTVICT);
      act("You listen with satisfaction to the sound of $N's bones snapping.",
          FALSE, ch, 0, vict, TO_CHAR);
      act("$n &+yis breaking your bones with $s deadly hug!", FALSE, ch, 0,
          vict, TO_VICT);
      /* this really, really hurts... always better than usual bearhug */
      damage(ch, vict, 50 + GET_LEVEL(ch), SKILL_BEARHUG);
      return FALSE;
    }

    if (temp < 80)
    {
      act
        ("$n &+ywraps $s &+YHUGE &+yarms around $N&+y, squeezing $M powerfully.",
         FALSE, ch, 0, vict, TO_NOTVICT);
      act("You squeeze the living daylights out of $N.", FALSE, ch, 0, vict,
          TO_CHAR);
      act("$n&+y's &+YHUGE &+yarms lock around you in a painful grip.", FALSE,
          ch, 0, vict, TO_VICT);
      damage(ch, vict, 25 + GET_LEVEL(ch), SKILL_BEARHUG);
      return FALSE;
    }
    else
    {
      act("$n &+ytries to wrap $s &+YHUGE &+yarms around $N&+y, but fails.",
          FALSE, ch, 0, vict, TO_VICT);
      act("$N escaped right out of your arms. You must be getting slow.",
          FALSE, ch, 0, vict, TO_CHAR);
      act
        ("$n &+ytries to lock $s &+YHUGE &+yarms around you, but you escape easily.",
         FALSE, ch, 0, vict, TO_VICT);
      CharWait(ch, PULSE_VIOLENCE * 2);
      return FALSE;
    }
    return FALSE;
  }
  return FALSE;
}


int conj_specpet_djinni(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;
  int in_room;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if( cmd == CMD_PERIODIC && (tch = get_linked_char( ch, LNK_PET )) != NULL && IS_PC(tch)
    && !strcmp(tch->player.name, "Orthyn") && !number(0, 100) )
  {
    if( number(0, 3) )
      do_say( ch, "Ebola!", CMD_SAY );
    else
      do_say( ch, "AIDS!", CMD_SAY );
  }

  if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 10) == 1))
  {
    vict = ch->specials.fighting;
    if(vict->in_room != ch->in_room)
        return false;
    act("$n&+C suddenly picks up speed and &+Wtears &+Cthrough the room!",
        FALSE, ch, 0, vict, TO_ROOM);
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if ((!ch->group || ch->group != tch->group) && !number(0, 10))
      {
        if ((GET_SIZE(tch) > SIZE_GIANT))
        {
          act
            ("$n&+C tries to &+Wthrow &+C$N &+Cbut they are simply too large to lift into the &+Wair&+C!",
             FALSE, ch, 0, tch, TO_NOTVICT);
          act
            ("$n&+C tries to &+Wthrow &+Cyou &+Cbut you are simply too large to lift into the &+Wair&+C!",
             FALSE, ch, 0, tch, TO_VICT);
        }
        else
        {
          act
            ("$n&+C &+Wthrows &+C$N in mid &+Wair &+Cand sends them flying &+Wupwards&+C!",
             FALSE, ch, 0, tch, TO_NOTVICT);
          act
            ("$N &+ccr&+Cash&+ces &+Cto the ground with a bone crunching &+Wthud&+C!",
             FALSE, ch, 0, tch, TO_NOTVICT);

          act
            ("$n&+C &+Wthrows &+Cyou in mid &+Wair &+Csending you flying &+Wupwards&+C!",
             FALSE, ch, 0, tch, TO_VICT);
          act
            ("&+CYou &+ccr&+Cas&+ch &+Cto the ground with a bone crunching &+Wthud&+C!",
             FALSE, ch, 0, tch, TO_VICT);
          SET_POS(tch, POS_SITTING + GET_STAT(tch));
          CharWait(tch, PULSE_VIOLENCE * 1);
        }
      }
    }
  }
  return FALSE;
}

int conj_specpet_slyph(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;
  int      room, in_room;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( IS_ALIVE(ch) && IS_FIGHTING(ch) && cmd == CMD_MOB_MUNDANE && number(1, 10) == 1 )
  {
    room = ch->in_room;
    vict = ch->specials.fighting;
    if( vict->in_room != room )
    {
      return FALSE;
    }
    act("$n&+C gains a burst of &+Wenergy&+C!", FALSE, ch, 0, vict, TO_ROOM);
    spell_cyclone(45, ch, NULL, SPELL_TYPE_SPELL, vict, 0);
   // if (is_char_in_room(ch, room) && is_char_in_room(vict, room))
      //spell_cyclone(45, ch, NULL, SPELL_TYPE_SPELL, vict, 0);
  }
  return FALSE;
}

int conj_specpet_triton(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;
  int      healpoints = 432, in_room;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if( IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1)
    && world[ch->in_room].sector_type != SECT_FIREPLANE && world[ch->in_room].sector_type != SECT_LAVA )
  {
    if (GET_HIT(ch) < GET_MAX_HIT(ch))
    {
      if ((healpoints + GET_HIT(ch)) >= GET_MAX_HIT(ch))
        healpoints = MAX(0, GET_MAX_HIT(ch) - GET_HIT(ch) - dice(1, 4));
      GET_HIT(ch) += healpoints;
      healCondition(ch, healpoints);
      update_pos(ch);
      act("$n&+B stretches &+bout and lets loose a &+Btriumphant &+Whowl&+b!",
          FALSE, ch, 0, vict, TO_ROOM);
      act("&+bAbsorbing &+Bmoisture &+bfrom its surroundings $n &+brebuilds its &+Bbody.&+b!",
         FALSE, ch, 0, vict, TO_ROOM);
    }
  }
  return FALSE;
}

int conj_specpet_undine(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;
  int      healpoints = 50;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 10) == 1))
  {
    vict = ch->specials.fighting;
    
    if(vict->in_room != ch->in_room)
        return false;
        
    if ((GET_HIT(vict) - healpoints) <= 0)
      healpoints = (GET_HIT(vict) - dice(1, 4));
    GET_HIT(vict) -= healpoints;
    healCondition(vict, healpoints);
    update_pos(vict);
    act
      ("&+bWith &=LBlightning&N&+B speed $n&+b flows forward, choking &+B$N&+b!",
       FALSE, ch, 0, vict, TO_NOTVICT);
    act
      ("&+bWith &=LBlightning&N&+B speed $n&+b flows forward, choking &+BYOU&+b!",
       FALSE, ch, 0, vict, TO_VICT);
    if (number(1, 10) == 1)
    {
      spell_dread_wave(45, ch, NULL, SPELL_TYPE_SPELL, vict, NULL);
    }
  }
  return FALSE;
}

int conj_specpet_salamander(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;
  int in_room;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( IS_FIGHTING(ch) && cmd == CMD_MOB_MUNDANE && (number(1, 10) == 1)
    && world[ch->in_room].sector_type != SECT_WATER_PLANE )
  {
    vict = ch->specials.fighting;
    if( vict->in_room != ch->in_room )
    {
      return FALSE;
    }
    act("$n &+ropens its &+Rjaws &+rspewing a &+Rf&+rl&+Ra&+rm&+Ri&+rn&+Rg mass at $N!",
      FALSE, ch, 0, vict, TO_NOTVICT);
    act("$n &+ropens its &+Rjaws &+rspewing a &+Rf&+rl&+Ra&+rm&+Ri&+rn&+Rg mass at you!",
      FALSE, ch, 0, vict, TO_VICT);
    spell_immolate(50, ch, NULL, 0, vict, 0);
    if( number(1, 5) == 5 )
    {
      act("&+RSucking &+rin another &+Rbreath &+r$n &+rcovers &+R$N &+rwith &+Wwhite&+r-&+Rhot &+rmagma!",
         FALSE, ch, 0, vict, TO_NOTVICT);
      act("&+RSucking &+rin another &+Rbreath &+r$n &+rcovers &+Ryou &+rwith &+Wwhite&+r-&+Rhot &+rmagma!",
         FALSE, ch, 0, vict, TO_VICT);
      spell_magma_burst(60, ch, NULL, 0, vict, 0);
    }
  }
  return FALSE;
}

int conj_specpet_serpent(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch;
  P_char   vict;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if(IS_FIGHTING(ch) &&
    (cmd == 0) &&
    (number(1, 10) == 1) &&
    world[ch->in_room].sector_type != SECT_WATER_PLANE)
  {
    vict = ch->specials.fighting;
    if(vict->in_room != ch->in_room)
        return false;
        
    act("$n &+ropens its &+Rjaws &+rsucking in a deep &+Rbreath&+r!", FALSE,
        ch, 0, vict, TO_NOTVICT);
    act("$n &+ropens its &+Rjaws &+rsucking in a deep &+Rbreath&+r!", FALSE,
        ch, 0, vict, TO_VICT);
    act
      ("&+rA &+WGIGANTIC &+Rfireball &+rshoots forward totally &+Renveloping &+R$N&+r!",
       FALSE, ch, 0, vict, TO_NOTVICT);
    act
      ("&+rA &+WGIGANTIC &+Rfireball &+rshoots forward totally &+Renveloping &+Ryou&+r!",
       FALSE, ch, 0, vict, TO_VICT);
    spell_solar_flare(60, ch, NULL, 0, vict, NULL);
  }
  return FALSE;
}

int shabo_palle(P_char ch, P_char vict, int cmd, char *arg)
{
  int      found;
  P_char   tempch;
  P_obj    tempobj, next_obj;
  int      r_room, rand;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     Gbuf2[MAX_STRING_LENGTH];
  char     asked[MAX_STRING_LENGTH];
  P_char   i, i_next, tempchar = NULL, tempchar2 = NULL, was_fighting = NULL;
  P_desc   d;
  P_obj    item, next_item;
  P_char   gunnadie;
  static bool askedquestion = FALSE;
  static int timerr = 0;
  int      pos, rr;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch)
    return FALSE;

  if (arg && cmd == CMD_ASK)
  {
    sprintf(asked, "%s", arg);
    for (rr = 0; *(asked + rr) != '\0'; rr++)
      asked[rr] = LOWER(*(asked + rr));
  }

  timerr++;


  if (((timerr == 10) || isname(arg, "pallistren darkaland")) &&
      (askedquestion))
  {
    tempchar2 = read_mobile(32847, VIRTUAL);
    timerr = 0;

    if (!tempchar2)
      return FALSE;

    stop_fighting(ch);
    if( IS_DESTROYING(ch) )
      stop_destroying(ch);
    tempchar2 = read_mobile(32847, VIRTUAL);
    if (!tempchar2)
    {
      logit(LOG_EXIT, "assert: second mob load failed in shabo_palle()");
      wizlog(MINLVLIMMORTAL, "error in proc shabo_pelle");
      return FALSE;
    }
    char_to_room(tempchar2, ch->in_room, -2);
    for (item = ch->carrying; item; item = next_item)
    {
      next_item = item->next_content;
      obj_from_char(item);
      obj_to_char(item, tempchar2);
    }
    for (pos = 0; pos < MAX_WEAR; pos++)
    {
      if (ch->equipment[pos] != NULL)
      {
        item = unequip_char(ch, pos);
        equip_char(tempchar2, item, pos, TRUE);
      }
    }
    mobsay(ch,
           "You know, on second thought, I dont think you belong here.. DIE!");
    extract_char(ch);
    return TRUE;
  }
  if ((timerr == 10) && !(askedquestion))
    timerr = 0;


  if (!isname(arg, "pallistren darkaland"))
    return FALSE;

  if (isname(arg, "pallistren darkaland") && !askedquestion)
  {
    mobsay(ch,
           "Greetings.  I am Pallistren, representative to Shaboath.  It is obvious");
    mobsay(ch,
           "that you are no friends of the aboleth, and are here under less than welcome");
    mobsay(ch,
           "circumstances.  Oh, don't worry, I won't alert the aboleth, they are");
    mobsay(ch, "more than capable of taking care of themselves.");
    mobsay(ch,
           "I care little of what goes on on this plane, but the weaving of the magical");
    mobsay(ch,
           "arts that are taking place in the great towers of this city intigue me.  To");
    mobsay(ch,
           "date I have been unable to scry into them, however.  Unfortunately, I have");
    mobsay(ch,
           "not yet figured out how to enter them, to examine the magical undergoings");
    mobsay(ch, "more closely myself.");
    askedquestion = TRUE;
  }

}

int shabo_derro_savant(P_char ch, P_char pl, int cmd, char *arg)
{
  int      allowed = 0;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  allowed = 0;
  if (!ch)
    return 0;
  if (!pl)
    return 0;

  if (!(cmd == CMD_EAST) && !(cmd == CMD_WEST) && !(cmd == CMD_NORTH) &&
      !(cmd == CMD_NE) && !(cmd == CMD_NORTHEAST))
    return 0;

  if (IS_TRUSTED(pl) || IS_NPC(pl))
    allowed = 1;
  else
    allowed = 0;

  if (allowed)
  {
    act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
    act("$N nods and stands aside to let you pass.",
        FALSE, pl, 0, ch, TO_CHAR);
    return (FALSE);
  }
  /* BLOCK! */
  act("$N &+yjumps in your path blocking the exit!&n.", FALSE, pl, 0, ch,
      TO_CHAR);
  act("$N &+yjumps in the way of $n blocking the exit!&n.", FALSE, pl, 0, ch,
      TO_NOTVICT);
  return (TRUE);
}

/*
* Tharnadia patrols justice  - Kvark
*
* Basicly scan and track, and if enaged shout for assistence.
*
*
*/

/*
 *Patrol leader Mob Proc
 */

/*

*/



int      tower_data[5][6] = {
  {150115, 150122, 150129, 150130, 150131, 150132},     //Human
  {150118, 150124, 150133, 150134, 0, 0},       //Gnome
  {150119, 150126, 150139, 150140, 0, 0},       //dwarf
  {150116, 150123, 150135, 150136, 0, 0},       // barb
  {150117, 150125, 150137, 150138, 0, 0}        // halfling
};


int outpost_captain(P_char ch, P_char pl, int cmd, char *arg)
{
  int      i, door, direction;
  int      source_room, target_room;
  bool     CombatInRoom;
  int      distance = 10;
  int      helpers_1[6];        // 4 Elites lvl 50
  int      helpers_2[6];        // 4 elites lvl 50 + captain 55
  int      helpers_3[6];
  int      ii = 0;
  P_desc   d;


  //return 0;
  int      how_many;
  P_char   tmp_ch;
  P_char   t_ch, vict;
  struct follow_type *k, *next_dude;
  char     buf[256];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (GET_VITALITY(ch) < 10)    /* ok dont get too tired */
    return TRUE;



  if (time_info.hour == 5)
    distance = (int) (distance / 3) + number(1, 3);
  else if (time_info.hour == 6)
    distance = (int) (distance / 3) + number(1, 2);
  else if (time_info.hour == 15)
    distance = (int) (distance / 3) + number(1, 3);
  else if (time_info.hour == 16)
    distance = (int) (distance / 3) + number(1, 3);
  else if ((time_info.hour > 17) || (time_info.hour < 7))
  {
    distance = (int) (distance / 3) + number(0, 1);
    if (!number(0, 60) && !(GET_STAT(ch) == STAT_SLEEPING))
    {
      act("&+WThe sounds of violent snoring filter through the area.", FALSE,
          ch, 0, 0, TO_ROOM);
      do_sit(ch, 0, 0);
      do_sleep(ch, 0, 0);
      return FALSE;
    }
    if (!number(0, 10) && GET_STAT(ch) == STAT_SLEEPING)
    {
      act("&+WThe sounds of violent snoring filter through the area.", FALSE,
          ch, 0, 0, TO_ROOM);
      return FALSE;
    }
    if (GET_STAT(ch) == STAT_SLEEPING)
      return FALSE;
  }
  else
  {
    distance = distance + number(0, 2);
    if (GET_STAT(ch) == STAT_SLEEPING)
    {
      do_wake(ch, 0, 0);
      do_stand(ch, 0, 0);
      do_alert(ch, 0, 0);
      if (!number(0, 3))
        act("&+WThe sounds of snoring slowly dissipates.", FALSE, ch, 0, 0,
            TO_ROOM);
      return FALSE;
    }
  }

  /* ok check to make sure followers are not out of move */

  if (ch->followers)
  {
    for (k = ch->followers; k; k = next_dude)
    {
      next_dude = k->next;
      if (IS_NPC(k->follower) && (GET_VITALITY(k->follower) < 10))
        return TRUE;
    }
  }

  CombatInRoom = FALSE;

  if (!ALONE(ch))
  {
    if (IS_FIGHTING(ch))
      CombatInRoom = TRUE;
    else
    {
      LOOP_THRU_PEOPLE(tmp_ch, ch) if (IS_FIGHTING(tmp_ch))
      {
        CombatInRoom = TRUE;
        break;
      }
    }
  }
  how_many = 0;
  /* Fix this to check how many in room */
  if (IS_FIGHTING(ch) && number(1, 3) == 1)
  {
    if ((IS_PC(ch->specials.fighting) && RACE_EVIL(ch->specials.fighting)) ||
        (RACE_PUNDEAD(ch->specials.fighting) && IS_PC(ch->specials.fighting)))
    {
      LOOP_THRU_PEOPLE(t_ch, ch)
      {
        if ((IS_PC(t_ch) && RACE_EVIL(t_ch)) ||
            (IS_PC(t_ch) && RACE_PUNDEAD(t_ch)))
        {
          if (!IS_TRUSTED(ch))
            how_many++;
        }
      }
      ii = 0;
      if (GET_RACE(ch) == RACE_HUMAN)
        ii = 0;
      if (GET_RACE(ch) == RACE_GNOME)
        ii = 1;
      if (GET_RACE(ch) == RACE_MOUNTAIN)
        ii = 2;
      if (GET_RACE(ch) == RACE_BARBARIAN)
        ii = 3;
      if (GET_RACE(ch) == RACE_HALFLING)
        ii = 4;
      i = 6;

      helpers_1[0] = tower_data[ii][number(0, 5)];

      helpers_2[0] = tower_data[ii][number(0, 5)];
      helpers_2[1] = tower_data[ii][number(0, 5)];

      helpers_3[0] = tower_data[ii][number(0, 5)];
      helpers_3[1] = tower_data[ii][number(0, 5)];
      helpers_3[2] = tower_data[ii][number(0, 5)];

      if (how_many <= 4 && how_many > 1 && !number(0, 8))
        return shout_and_hunt(ch, 100,
                              "&+WGuardians of good come aid me in vanquishing the evil invaders!",
                              NULL, helpers_1, 0, 0);
      if (how_many > 4 && how_many <= 6 && !number(0, 5))
        return shout_and_hunt(ch, 100,
                              "&+WGuardians of good come help me destroy the evil invasion!",
                              NULL, helpers_2, 0, 0);
      if (how_many > 6 && !number(0, 1))
        return shout_and_hunt(ch, 100,
                              "&+Wguardians of good come help me defend our homeland!",
                              NULL, helpers_3, 0, 0);

    }
    else if (IS_PC(ch->specials.fighting) && RACE_GOOD(ch->specials.fighting))
    {
      strcpy(buf, "You moron I am here to protect you!");
      do_say(ch, buf, CMD_SAY);
    }
  }

  if (!CombatInRoom && (ch->in_room != NOWHERE) &&
      !IS_SET(world[ch->in_room].room_flags, ROOM_SILENT) &&
      !IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_SILENT) &&
      (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
  {

    /* ok we check if there is any evils near 50 rooms, if
     * soo, piss it off!
     */

// TRACK IF EVILS

    d = descriptor_list;
    ii = 0;
    if (number(0, 1))
      while (d)
      {
        if (!d->connected &&
            (world[ch->in_room].zone == world[d->character->in_room].zone) &&
            !(IS_SET(world[ch->in_room].room_flags, GUILD_ROOM)))
        {
          /*found char in same zone */
          if ((IS_PC(d->character) && RACE_EVIL(d->character) &&
               !IS_TRUSTED(d->character)) || (IS_PC(d->character) &&
                                              RACE_PUNDEAD(d->character) &&
                                              !IS_TRUSTED(d->character)))
          {
            if (how_close(ch->in_room, d->character->in_room, distance) > 0)
              if (number(0, 11) >
                  BOUNDED(0,
                          how_close(ch->in_room, d->character->in_room,
                                    distance), 10))
              {                 // the longer the less offen
                ii++;
                if (number(0, 10) < ii)
                {               // the more in group the easier
                  remember(ch, d->character);
                  wizlog(56, "Mob(%s), starts to hunt(%s) on tharn",
                         ch->player.short_descr, GET_NAME(d->character));
                  if (!number(0, 1))
                    act
                      ("&+WSomeone has discovered your presence and starts to track you!",
                       FALSE, d->character, 0, 0, TO_CHAR);

                }

              }

          }

        }
        d = d->next;
      }



// END TRACK IF EVILS!
/* ok we check if there is combat near */

    if ((direction = range_scan(ch, NULL, 2, SCAN_COMBAT)) >= 0)
    {
      if (EXIT(ch, direction))
      {
        if ((EXIT(ch, direction))->to_room &&
            world[EXIT(ch, direction)->to_room].justice_area ==
            world[ch->in_room].justice_area)
        {
          ch->only.npc->last_direction = direction;
          do_move(ch, 0, exitnumb_to_cmd(direction));
          return TRUE;
        }
      }
    }
  }


  /* seem we can try to move, since nothing else to do */


  return FALSE;

}

int necro_dracolich(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    t_obj, next;
  int      i;

  if (!cmd && !GET_MASTER(ch)) {
    if (!number(0,2)) {
      act("As the magic binding $n vanished, $e returns to unlife.",
          FALSE, ch, 0, 0, TO_ROOM);
      die(ch, ch);
    }
    return TRUE;
  }

  if (cmd == CMD_DEATH)
  {
    check_saved_corpse(ch);
    for (t_obj = ch->carrying; t_obj; t_obj = next)
    {
      next = t_obj->next_content;
      obj_from_char(t_obj);
      obj_to_room(t_obj, ch->in_room);
    }
    for (i = 0; i < MAX_WEAR; i++)
      if (ch->equipment[i])
        obj_to_room(unequip_char(ch, i), ch->in_room);

    if (GET_VNUM(ch) != 1201)
      act("$n collapses into a pile of &+Wbones&n which crumble to dust.",
          FALSE, ch, 0, 0, TO_ROOM);
    else
      act("$n crumbles to dust.", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

int witch_doctor(P_char witch, P_char customer, int cmd, char *arg)
{
  struct affected_type af;
  char     buf[256];
  int      i, room, tries, code;
  struct item
  {
    char    *keyword;
    char    *desc;
    int      price;
    byte     affect_vector;
    uint     affect_flag;
  } elixir_list[] =
  {
    { "accelerate haste",
      "a &+ymagical elixir&n labeled \"&+YAcce&+yler&+Yate&n\"", 3000, 1, AFF_HASTE},
    { "incendio fire",
      "a &+cwarm bottle&n labeled \"&+rProtectum &+RIncendio&n\"", 400, 1, AFF_PROT_FIRE},
    { "frigeo cold",
      "a &+ccold bottle&n labeled \"&+bProtectum &+BFrigeo&n\"", 400, 2, AFF2_PROT_COLD},
    { "aviate fly",
      "a &+clight flask&n labeled \"&+CAv&+Wia&+Cte&n\"", 1000, 1, AFF_FLY},
    { "indigetis epic epics",
      "a &+Wglowing elixir&n labeled \"&+WIn&+wdiget&+Wis&n\"", 10000, 4, AFF4_EPIC_INCREASE},
    { "gnowsis experience exp",
      "a &+mrapidly vibrating vial&n labeled \"&+MGnowsis Ektaktos&n\"", (100 * GET_LEVEL(customer)), 0, TAG_RESTED},
    { 0 }
  };

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;               // change to TRUE to enable wandering

  if( cmd == CMD_LIST )
  {
    act("$n whispers, '&+WI can offer you the following arcane elixirs:&n'\r\n",
      FALSE, witch, 0, customer, TO_VICT);
    for( i = 0; elixir_list[i].keyword; i++ )
    {
      sprintf(buf, "&+W%d)&n %s  - %d &+Wplatinum&n\r\n", i + 1, elixir_list[i].desc, elixir_list[i].price);
      send_to_char(buf, customer);
    }
    return TRUE;
  }

  if( cmd == CMD_BUY )
  {
    for( i = 0; elixir_list[i].keyword; i++ )
    {
      if( ((code = atoi(arg)) > 0 && code == i + 1)
        || isname(arg, elixir_list[i].keyword) )
      {
        memset(&af, 0, sizeof(struct affected_type));
        // Static for TAG_RESTED potion for exp
        if( i == 5 )
        {
  	      if (affected_by_spell(customer, TAG_RESTED) || affected_by_spell(customer, TAG_WELLRESTED)) 
	        {
            send_to_char("The &+Gwi&+gtc&+Gh &+gdoctor&n says to you 'You already have an &+mexperience bonus&n currently active, come find me again when that one has expired!'\n\n", customer);
            return TRUE;
          }
          else
          {
            // 150 ticks about 2 1/2 hrs.
            af.duration = 150;
            af.type = TAG_RESTED;
            // Note: This one is visible on score while the rest of the witch spells aren't.
            //   This is justifiable since it has a much shorter timer.
            af.flags = AFFTYPE_PERM | AFFTYPE_NODISPEL | AFFTYPE_OFFLINE;
            debug( "'%s' getting rested bonus from WITCH!", J_NAME(customer) );
          }
        }
        else
        {
          // 1 rl week = 7 * 24 * 60.
          af.duration = 10080;
          af.type = TAG_WITCHSPELL;
          af.flags = AFFTYPE_NOSHOW | AFFTYPE_PERM | AFFTYPE_NODISPEL | AFFTYPE_OFFLINE;
        }

        // Prices are in platinum -> 1000 * ...
        if( transact(customer, NULL, witch, 1000 * elixir_list[i].price) )
        {
          if (elixir_list[i].affect_vector == 1)
            af.bitvector = elixir_list[i].affect_flag;
          else if (elixir_list[i].affect_vector == 2)
            af.bitvector2 = elixir_list[i].affect_flag;
          else if (elixir_list[i].affect_vector == 3)
            af.bitvector3 = elixir_list[i].affect_flag;
          else if (elixir_list[i].affect_vector == 4)
            af.bitvector4 = elixir_list[i].affect_flag;
          else if (elixir_list[i].affect_vector == 5)
            af.bitvector5 = elixir_list[i].affect_flag;
          affect_to_char(customer, &af);
          sprintf(buf, "$n reaches down to $s sack and hands to you %s.\n"
            "As you quaff %s you feel the &+Ymagical powers&n surge through your body"
            " transforming you and making more &+Wpowerful&n than before.",
            elixir_list[i].desc, elixir_list[i].desc);
          act(buf, FALSE, witch, 0, customer, TO_VICT);
          GET_PLATINUM(witch) = 0;
        }
        return TRUE;
      }
    }
    act("$n says, 'I dont sell that.'", FALSE, witch, 0, customer, TO_VICT);
    return TRUE;
  }

  if (!cmd && !number(0, 150))
  {
    tries = 0;
    room = real_room0(500001);
    do
    {
      room = number(zone_table[world[room].zone].real_bottom,
                    zone_table[world[room].zone].real_top);
    }
    while( tries++ < 500 &&
           (IS_SET(world[room].room_flags, PRIV_ZONE) ||
            IS_SET(world[room].room_flags, NO_TELEPORT) ||
            world[room].sector_type == SECT_OCEAN ||
            world[room].sector_type == SECT_WATER_SWIM ||
            world[room].sector_type == SECT_MOUNTAIN ||
            world[room].sector_type == SECT_WATER_NOSWIM));
    if (tries >= 500)
    {
      statuslog(0, "Witch Doctor cannot find a spot to land, check his proc plz");
    }
    else
    {
      act("$n looks around and says, '&+WNice doing trade with you but I need to serve other customers as well.&n'",
         FALSE, witch, 0, 0, TO_ROOM);
      act("$n utters a few words, $s form blurs and shifts and $e's gone!", FALSE, witch, 0, 0, TO_ROOM);
      char_from_room(witch);
      char_to_room(witch, room, -1);
    }
  }

  return FALSE;
}
#define INVASION_ROOM_VNUM   29116
#define INVASION_LEADER_VNUM 28975
// When Timoro dies, a bunch of dwarven invaders led by mob vnum (above), come down
//   from room vnum (above) to avenge the death.
int timoro_die(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   i;
  char   dir[] = "down";

  if( cmd != CMD_DEATH )
    return FALSE;

  act("\nA banging sound can be heard as if someone is pounding the walls.\n"
      "All of a sudden large &+ycracks&n form along the ceiling and chunks of &+Lstone&n slam in the floor.\n"
      "A large band of dwarves pour out of the hole, faces grim.\n", TRUE, ch,
      0, 0, TO_ROOM);

  for( i = world[real_room(INVASION_ROOM_VNUM)].people; i; i = i->next_in_room )
  {
    if( (IS_NPC(i)) && (GET_VNUM(i) == INVASION_LEADER_VNUM) )
    {
      command_interpreter(i, dir);
      break;
    }
  }
  return FALSE;
}

void reload_io_assistant(P_char, P_char, P_obj, void *data)
{
  P_char ch;
  int realroom44 = real_room0(44);
  int realmob444 = real_mobile0(444);

  for (ch = world[realroom44].people; ch; ch = ch->next_in_room)
  {
    // mob is already there.. don't load new
    if (GET_RNUM(ch) == realmob444)
      return;
  }
  ch = read_mobile(444, VIRTUAL);
  char_to_room(ch, realroom44, -1);
  act("$n arrives in a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
}

int io_assistant(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }
  // anti-kill...
  GET_HIT(ch) = GET_MAX_HIT(ch);

  if (cmd == CMD_PURGE)
  {
    // set up an event to reload if needed..
    add_event(reload_io_assistant, 1, NULL, NULL, NULL, 0, NULL, 0);
    // act like we didn't handle it
    return FALSE;
  }

  if (!ch || !IS_AWAKE(ch) || cmd)
    return FALSE;

  int realroom44 = real_room0(44);
  // if NOT in the proper room, move to proper room
  if (ch->in_room != realroom44)
  {
    mobsay(ch, "I really need to get back to Io's room...");
    act("$n disappears in a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, realroom44, -1);
    act("$n arrives in a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
  }

  // if in the proper room, and others of me in the room, purge self
  for (pl = world[realroom44].people; pl; pl = pl->next_in_room)
  {
    if ((ch != pl) && IS_NPC(ch) && IS_NPC(pl) && (GET_RNUM(pl) == GET_RNUM(ch)))
    {
      mobsay(ch, "Oh no!  Too many assistants will drive Io crazy!  Bye!");
      act("$n disappears in a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
      extract_char(ch);
      return TRUE;
    }
  }

  P_obj obj;
  char buf[500];
  P_char chVar = get_char_room_vis(ch, "Vareena");

  static bool bVarHere = false;
  static time_t lastWhisper = 0;

  if (!bVarHere && chVar)
  { // vareena entered the room...
    bVarHere = true;
    strcpy(buf, "vareena");
    do_action(ch, buf, CMD_HAND);
    // load a rose...
    obj = read_object(1277, VIRTUAL);
    if (obj)
    {
      obj_to_char(obj, ch);
      do_give(ch, " rose vareena", CMD_GIVE);
    }
    return TRUE;
  }
  else if (!chVar)
  {
    bVarHere = false;
  }

  if (!number(0, 200))
  {
    do_action(ch, NULL, CMD_PONDER);;
    return TRUE;
  }

  // something is whacked with the random number generator, so adding some code to ensure a quiet
  // time...  don't spam poor vareena with whispers!  Max of 1 per 15 minutes
  if (chVar && (time(0) > (lastWhisper + (15 * 60))))
  {
    switch (number(1, 100))
    {
      case 50:
      case 51:
        act("$n whispers to you, 'Do you know that Io really loves you?  He's ALWAYS talking about you...'",
            FALSE, ch, 0, chVar, TO_VICT);
        break;

      case 52:
      case 53:
      case 54:
        act("$n whispers to you, 'Io asked me to remind you to email him info on what special procs you need in your glacier zone'",
            FALSE, ch, 0, chVar, TO_VICT);
        break;

      case 55:
      case 56:
        act("$n whispers to you, 'Smile, Vareena, someone loves you!'",
            FALSE, ch, 0, chVar, TO_VICT);
        break;

      case 57:
      case 58:
        act("$n whispers to you, 'What have you done to Io?  He's completely taken by you!'",
            FALSE, ch, 0, chVar, TO_VICT);
        break;

      default:
        return FALSE;
    }
    act("$n whispers something to $N.", FALSE, ch, 0, chVar, TO_NOTVICT);
    lastWhisper = time(0);
    return TRUE;;
  }

  return FALSE;
}

int bs_barons_mistress(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 74000, 74003, 74004, 74006, 74011, 74012, 74019, 74022,
     74037, 74038, 74039, 74040, 74041, 74042, 74043, 74044, 74045, 74055, 74056,
     74057, 74058, 74059, 74060, 74061, 74066, 74067, 74068, 74069, 74070, 74071,
     74074, 74075, 74081, 74082, 74083, 74084, 74085, 74086, 74089, 74090, 74092,
     74108, 74109, 74115, 74116, 74117, 74123, 74124, 74125, 74126, 74146, 74157,
     74158, 74160, 74175, 74176, 74179, 74180, 74181, 74186, 74187, 74188, 74190,
     74225, 74229, 74230, 74231, 74232, 74237, 74244, 74245, 0
  };
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!tch)
    return shout_and_hunt(ch, 4,
                          "Somebody HELP me! I am being robbed by %s!\r\n",
                          NULL, helpers, 0, 0);
  return FALSE;
}

int bs_citizen(P_char ch, P_char pl, int cmd, char *arg)
{

  /*
       check for periodic event calls
  */
      if (cmd == CMD_SET_PERIODIC)
         return TRUE;
                
      if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
          return FALSE;
                      
          switch (number(0, 100))
          {
            case 0:
              mobsay(ch, "Hail stranger!  Tis a fine day, is it not?");
              return TRUE;
            case 1:
              mobsay(ch, "Damn!  Taxes were raised again!");
              return TRUE;
            case 2:
              mobsay(ch, "Have you seen my wife around here anywhere?");
              return TRUE;
            case 3:
              mobsay(ch, "I hear Verzanan is a nice place to visit.");
              return TRUE;
            case 4:
              do_action(ch, 0, CMD_BURP);
              do_action(ch, 0, CMD_BLUSH);
              mobsay(ch, "Excuse me.");
              return TRUE;
            default:
            return FALSE;
          }
}


/*
   Bloodstone Zone 71 Mob proc
   *Mob#:74001
   *Name:Common woman
 */

int bs_comwoman(P_char ch, P_char pl, int cmd, char *arg)
{

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Have you been to the garden?");
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_PONDER);
    mobsay(ch, "Now where did that boy go?");
    return TRUE;
  case 2:
    act("$n pulls out a shopping list and studies it carefully.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
case 3:
    act("$n looks at you, and winks suggestively.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_BLUSH);
    return TRUE;
  case 4:
    mobsay(ch, "Have you traveled far?  Bloodstone does offer a nice inn.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7102
   *Name:Little brat
 */
int bs_brat(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNICKER);
    mobsay(ch, "I am hiding from my mom!");
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_SNORT);
    mobsay(ch, "Yummy, something big!");
    return TRUE;
  case 2:
    act("$n looks at you and bursts into laughter!", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    act("$n picks $s nose and casually places the treasure in $s mouth.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    mobsay(ch, "My pop could snap your neck instantly!");
    do_action(ch, 0, CMD_STRUT);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7103
   *Name:Holyman
 */
int bs_holyman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n raises $s unholy symbol, and cracks a wicked smile.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    mobsay(ch, "Praise be to &+yHoar&N, &+LThe Doombringer&N!");
    return TRUE;
  case 2:
    act("$n chants 'Fernum Acti Blas' and looks at you with contempt.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    mobsay(ch, "Leave me quickly, before I unleash my dark powers upon you!");
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_KNEEL);
    mobsay(ch, "Oh mighty &+yHoar&N, please grant me your dark powers!");
    act("A bolt of &+Bblue lightning&N strikes the holyman!",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch, "Praise be to &+yHoar&N!");
    do_action(ch, 0, CMD_STAND);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7104
   *Name:merchant
 */
int bs_merchant(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_PONDER);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_SMIRK);
    mobsay(ch, "Peasant trash!");
    return TRUE;
  case 2:
    mobsay(ch, "I must hurry, I have business with Waqar!");
    return TRUE;
  case 3:
    mobsay(ch, "A saber, a black silk sash...hmm what else was there?");
    do_action(ch, 0, CMD_PONDER);
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7105    7308
   *Name:wino    wino second
 */
int bs_wino(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodici event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_BURP);
    mobsay(ch, "Ahhh an excellent vintage!");
    return TRUE;
  case 1:
    act("$n kneels down close to the ground and covers $s mouth.",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_PUKE);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_HUM);
    mobsay(ch, "How dry I am, how dry I am..");
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_PONDER);
    mobsay(ch, "A roasted stirge would hit the spot!");
    do_action(ch, 0, CMD_DROOL);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_BOW);
    mobsay(ch, "At your service!");
    return TRUE;
  case 5:
    act("$n breaks out a bottle and takes a quick swig.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 6:
    mobsay(ch, "A tribute to the Baron!");
    do_action(ch, 0, CMD_FART);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7106
   *Name:watcher
 */
int bs_watcher(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 50))
  {
  case 0:
    mobsay(ch, "Move along, move along.");
    return TRUE;
  case 1:
    mobsay(ch, "Be careful you don't wander along the outside walls.");
    return TRUE;
  case 2:
    act("$n keeps a close eye on you.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    mobsay(ch, "I hear manticores make good mounts!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7107
   *Name:guard
 */
int bs_guard(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    mobsay(ch, "Beware the soultaker!");
    return TRUE;
  case 1:
    act("$n dazzles you with a sword technique.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "Now where did that apprentice go?  You're not him!");
    do_action(ch, 0, CMD_FROWN);
    return TRUE;
  case 3:
    mobsay(ch,
           "Like my equipment eh? Visit Waqar to get a fine saber like this!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7108
   *Name:squire
 */
int bs_squire(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    mobsay(ch, "I better get back to my master before I'm missed!");
    return TRUE;
  case 1:
    act("$n bungles a simple dagger technique.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_BLUSH);
    return TRUE;
  case 2:
    mobsay(ch, "With enough practice, I too can be a guard!");
    act("$n manages to execute a perfect dagger parry.",
        TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_STRUT);
    return TRUE;
  case 3:
    mobsay(ch, "I used to be a wandering soul such as yourself.");
    mobsay(ch, "Now my life has purpose!");
    do_action(ch, 0, CMD_SMIRK);
    return TRUE;
  default:
    return FALSE;
  }
}

int bs_peddler(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    mobsay(ch, "Can I sell you something?");
    return TRUE;
  case 1:
    act("$n displays to you his wares.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  case 2:
    mobsay(ch, "What?!  This is the finest merchandise in town!");
    do_action(ch, 0, CMD_FROWN);
    return TRUE;
  case 3:
    mobsay(ch, "Got something you don't want?  I'll buy most anything!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7110    7111     7112       7141
   *Name:vrock   hezrou   glabrezu   lurker
 */
int bs_critter(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  if (devour(ch, pl, cmd, arg))
    return TRUE;

  switch (number(0, 80))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7113
   *Name:timid prisoner
 */
int bs_timid(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "I swear I didn't do it!");
    do_action(ch, 0, CMD_CRY);
    return TRUE;
  case 1:
    mobsay(ch, "This place is too dangerous for me!");
    do_action(ch, 0, CMD_SNIFF);
    return TRUE;
  case 2:
    mobsay(ch, "Let me out! Let me out!");
    return TRUE;
  case 3:
    mobsay(ch, "I wouldn't go upstairs if I were you!");
    do_action(ch, 0, CMD_WINCE);
    return TRUE;
  case 4:
    mobsay(ch, "I wish that I was back home in Verzanan!");
    do_action(ch, 0, CMD_SIGH);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7114
   *Name:shady prisoner
 */
int bs_shady(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "The food is terrible here!");
    return TRUE;
  case 1:
    mobsay(ch, "Hey you're pretty cute!");
    do_action(ch, 0, CMD_DROOL);
    return TRUE;
  case 2:
    mobsay(ch, "Outta my way punk!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  case 3:
    act("$n looks you over very carefully.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n winks at you in a seductive way.", TRUE, ch, 0, 0, TO_ROOM);
    mobsay(ch, "What do you say you and me get together?");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7115
   *Name:sinister prisoner
 */
int bs_sinister(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Ya, I killed him.  What of it?");
    return TRUE;
  case 1:
    mobsay(ch, "Hey, you're pretty cute!");
    do_action(ch, 0, CMD_DROOL);
    return TRUE;
  case 2:
    mobsay(ch, "Go away before I rip your throat out!");
    do_action(ch, 0, CMD_GLARE);
    return TRUE;
  case 3:
    mobsay(ch, "Care to spar with me, chump?");
    do_action(ch, 0, CMD_FLEX);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7116
   *Name:menacing prisoner
 */
int bs_menacing(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    mobsay(ch, "I feel like eating somebody's heart!");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7117
   *Name:executioner
 */
int bs_executioner(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Nothing like a fine axe blade, eh?");
    do_action(ch, 0, CMD_CACKLE);
    return TRUE;
  case 1:
    mobsay(ch, "She said, 'Sir, please don't kill my husband!'");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  case 2:
    act("$n swings $s heavy axe over $s shoulder.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    mobsay(ch, "You'd have to be pretty strong to wield this axe!");
    do_action(ch, 0, CMD_FLEX);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_SMIRK);
    mobsay(ch, "Stay out of trouble or I'll add ya to my necklace!");
    do_action(ch, 0, CMD_CACKLE);
    mobsay(ch, "Understand me?!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7118
   *Name:baron
 */
int bs_baron(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_PONDER);
    mobsay(ch, "Methinks it is time to raise taxes!");
    do_action(ch, 0, CMD_CHUCKLE);
    return TRUE;
  case 1:
    mobsay(ch, "Where are my servants?!");
    do_action(ch, 0, CMD_POUT);
    return TRUE;
  case 2:
    act("$n removes a sparkling ring, and then replaces it!",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    mobsay(ch, "If my brother could only see me now!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_GRUMBLE);
    mobsay(ch, "I have business to tend to.  Go purchase something!");
    mobsay(ch, "We have some of the finest shops around!");
    return TRUE;
  case 5:
    do_action(ch, 0, CMD_YAWN);
    mobsay(ch, "Servants!  Time for my nap.");
    do_action(ch, 0, CMD_SNAP);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7120
   *Name:sparrow
 */
int bs_sparrow(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    do_action(ch, 0, CMD_HOP);
    return TRUE;
  case 1:
    act("$n turns $s eyes towards the ground searching for some food.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
  Bloodstone Zone 71 Mob proc
   *Mob#:7121        7122
   *Name:squirrel    huge squirrel
 */
int bs_squirrel(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    act("$n forages for some food.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n looks up to see what's around.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7123
   *Name:crow
 */
int bs_crow(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    do_action(ch, 0, CMD_HOP);
    act("$n eyes the area for some food.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    act("$n stares in your direction with malicious intent.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7124
   *Name:mountainman
 */
int bs_mountainman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_YODEL);
    return TRUE;
  case 1:
    act("$n strikes the ground with $s iron pick.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    mobsay(ch, "Hail stranger!  On your way to Bloodstone Keep?");
    return TRUE;
  case 3:
    mobsay(ch, "I'd keep clear of the manticore lair if I were you!");
    return TRUE;
  case 4:
    mobsay(ch, "To the far west is where the dead sleep.");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7125
   *Name:salesman
 */
int bs_salesman(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Pssst!  I've got some items you might want!");
    return TRUE;
  case 1:
    mobsay(ch, "Buy low, sell high...A sound method, is it not?");
    return TRUE;
  case 2:
    mobsay(ch, "I buy what you do not want!");
    do_action(ch, 0, CMD_SMILE);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7126
   *Name:nomad
 */
int bs_nomad(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Which way to the closest inn?  I am oh so very tired.");
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_WINCE);
    mobsay(ch, 
	   "I've seen many vile creatures in my days, but NONE as gruesome as you!");
    do_action(ch, 0, CMD_CRINGE);
    return TRUE;
  case 2:
    mobsay(ch, "This place is too dangerous for a wimp like you!");
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7127
   *Name:insane woman
 */
int bs_insane(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Who are you? What do you want? Leave me alone!");
    return TRUE;
  case 1:
    mobsay(ch, "Get them off of me!");
    do_action(ch, 0, CMD_HOP);
    return TRUE;
  case 2:
    mobsay(ch, "Kill them, kill them!!");
    do_action(ch, 0, CMD_STOMP);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_FIDGET);
    mobsay(ch, "Look!  A six foot stirge!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_CRY);
    mobsay(ch, "Damn elves!  Damn dwarves!  Damn them all!");
    do_action(ch, 0, CMD_LAUGH);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7128
   *Name:homeless man
 */
int bs_homeless(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Spare a few coins?");
    do_action(ch, 0, CMD_BEG);
    return TRUE;
  case 1:
    mobsay(ch, "I haven't always been this way...");
    do_action(ch, 0, CMD_CRY);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_SHIVER);
    mobsay(ch, "It is so cold, could you spare some coins for a room?");
    do_action(ch, 0, CMD_BEG);
    return TRUE;
  case 3:
    do_action(ch, 0, CMD_CRY);
    mobsay(ch, "Please help me, I haven't eaten in three moons.");
    do_action(ch, 0, CMD_SNIFF);
    return TRUE;
  case 4:
    mobsay(ch, "Will work for food!");
    return TRUE;
  case 5:
    mobsay(ch, "Spare some clothing for a homeless man?");
    do_action(ch, 0, CMD_SHIVER);
    mobsay(ch, "It's terribly cold here at night.");
    return TRUE;
  case 6:
    do_action(ch, 0, CMD_COUGH);
    return TRUE;
  default:
    return FALSE;
  }
}

 /*
   Bloodstone Zone 71 Mob proc
   *Mob#:7129
   *Name:baron's servant
 */

int bs_servant(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    mobsay(ch, "Time for the Baron's bath.");
    return TRUE;
  case 1:
    mobsay(ch, "The Baron is such a demanding master!");
    do_action(ch, 0, CMD_SNIFF);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_WINCE);
    mobsay(ch, "I hope that the Baron spares me from my daily beating.");
    mobsay(ch, "I have worked hard for him today!");
    do_action(ch, 0, CMD_PONDER);
    return TRUE;
  case 3:
    mobsay(ch, "Hmm..I wonder what the Baron will want to eat tonight?");
    do_action(ch, 0, CMD_PONDER);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7140
   *Name:wolf
 */
int bs_wolf(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  if (devour(ch, pl, cmd, arg))
    return TRUE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  case 2:
    do_action(ch, 0, CMD_BARK);
    return TRUE;
  case 3:
    act("$n lifts $s head up and lets out a piercing howl.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7143
   *Name:gnoll
 */
int bs_gnoll(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    act("$n looks at you with malicious intent.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

 /*
   Bloodstone Zone 71 Mob proc
   *Mob#:7144
   *Name:ettin
 */
int bs_ettin(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    act("$n scratches one of $s heads.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act("$n begins picking $s teeth again.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7147
   *Name:griffon
 */
int bs_griffon(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    act("$n fans $s enormous wings.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 2:
    act("$n begins eyeing you quite intently.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    act("$n snaps the bone of a carcus $e is devouring.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7152
   *Name:wereboar
 */
int bs_boar(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  case 2:
    act("$n lifts $s head up and lets out a piercing howl.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7153
   *Name:manticore cub
 */
int bs_cub(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 100))
  {
  case 0:
    act("$n rubs up against your leg.", TRUE, ch, 0, 0, TO_ROOM);
    do_action(ch, 0, CMD_PURR);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  case 2:
    act("$n swats at a leaf turning circles in the wind.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7154
   *Name:manticore fierce
 */
int bs_fierce(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  if (devour(ch, pl, cmd, arg))
    return TRUE;

  switch (number(0, 100))
  {
  case 0:
    act("$n eyes you very carefully.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 1:
    do_action(ch, 0, CMD_GROWL);
    return TRUE;
  case 2:
    act("$n lets out a tremendous roar.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 3:
    act("$n begins grooming itself.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  case 4:
    do_action(ch, 0, CMD_SNARL);
    return TRUE;
  default:
    return FALSE;
  }
}

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7160
   *Name:stirge
 */
int bs_stirge(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
     check for periodic event call
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
    return FALSE;

  switch (number(0, 80))
  {
  case 0:
    do_action(ch, 0, CMD_HOP);
    return TRUE;
  case 1:
    act("$n turns $s eyes towards you in search of some fresh blood.",
        TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  default:
    return FALSE;
  }
}

int llyren(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj t_obj;
  P_char owner = NULL;
  char buffer[256];

  if (cmd != CMD_LIST)
    return FALSE;
  
  if (!transact(pl, NULL, ch, 1000 * 50))
    return TRUE;

  for (t_obj = object_list; t_obj; t_obj = t_obj->next) {
    if (!IS_ARTIFACT(t_obj) || !strstr(t_obj->name, "unique") || 
        obj_index[t_obj->R_num].virtual_number == 22070) 
      // revenants crown won't be shown, it's a rareload
      continue;
    if (OBJ_WORN(t_obj))
      owner = t_obj->loc.wearing;
    else if (OBJ_CARRIED(t_obj))
      owner = t_obj->loc.carrying;
    else if (OBJ_ROOM(t_obj)) {
      sprintf(buffer, "I see %s in %s.\n", t_obj->short_description,
          world[t_obj->loc.room].name);
      send_to_char(buffer, pl);
      continue;
    } else
      continue;
    if (IS_PC(owner))
      continue;

    sprintf(buffer, "I see %s in posession of %s.\n",
        t_obj->short_description, owner->player.short_descr);
    send_to_char(buffer, pl);
  }

  return TRUE;
}

int teacher(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj t_obj;
  char buf[512];
  
  if (cmd != CMD_ASK || !arg || !strstr(arg, "level") ||
      !pl || !GET_CLASS(pl, ch->player.m_class))
    return FALSE;

  sprintf(buf, 
    "For your further path of development it is crucial that you visit\n"
    "%s of the magical runestones locates in the following lands:\n",
    GET_LEVEL(pl) >= get_property("exp.levelForAllRunestones", 51) - 1 ? 
      "all" : "one");

  for (t_obj = object_list; t_obj; t_obj = t_obj->next) {
    if (obj_index[t_obj->R_num].func.obj == epic_stone &&
        t_obj->value[3] == GET_LEVEL(pl) + 1) {
        strcat(buf, 
          zone_table[real_zone0(t_obj->value[2])].name);
        strcat(buf, "\n");
    }
  }

  send_to_char(buf, pl);
  return TRUE;
}

int clear_epic_task_spec(P_char npc, P_char ch, int cmd, char *arg)
{
  char askFor[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  P_obj nexus;

  if( cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC )
  {
    return FALSE;
  }

  if( !ch || !arg || !npc )
  {
    return FALSE;
  }

  arg = one_argument(arg, askFor);
  arg = one_argument(arg, askFor);

  if( cmd == CMD_ASK && !str_cmp(askFor,"task") )
  {
    if( !CAN_SPEAK(npc) )
    {
      return FALSE;
    }

    mobsay(npc, "Ah, another soul who feels unable to meet the harsh demands of the Gods.");
    mobsay(npc, "My child, I can help.");
    mobsay(npc, "I could prepare a &+Lprayer&n for you, and perhaps the Gods would listen.");
    return TRUE;
  }
  else if( cmd == CMD_ASK && !str_cmp(askFor,"prayer") )
  {
    if( !CAN_SPEAK(npc) )
    {
      return FALSE;
    }

    if( !CAN_SEE(npc, ch) )
    {
      mobsay(npc, "How can I help you if I cannot see you?");
      return TRUE;
    }
    struct affected_type *afp;
    afp = get_epic_task(ch);
    if( !afp )
    {
      mobsay(npc, "Whaat? The Gods haven't given you a task! Begone!");
      return TRUE;
    }

    /* count money */
    int price = get_property("mobspecs.epicTaskClear.price", 10000000);
    // i.e., spill blood task
    if( afp->modifier <= SPILL_BLOOD )
    {
      mobsay(npc, "The gods are upset with your prayer to clear your &+Rspilling blood&n task.");
      price *= 2;
    }
    // a nexus stone
    else if( afp->modifier < 0 && afp->modifier > -10 )
    {
      nexus = get_nexus_stone(-(afp->modifier));
      if( !nexus )
      {
        debug("clear_epic_task_spec(): error, can't find nexus");
        send_to_char("Can't clear a bugged task, please ask an imm.\r\n", ch);
        return TRUE;
      }
      if( (RACE_GOOD(ch) && STONE_ALIGN(nexus) < STONE_ALIGN_GOOD)
        || (RACE_EVIL(ch) && STONE_ALIGN(nexus) > STONE_ALIGN_EVIL) )
      {
        price = (price * 3) /2;
      }
    }

    if( GET_MONEY(ch) < price )
    {
      mobsay(npc, "I can't pray a proper prayer on an empty stomach!");
      sprintf(buffer, "You need at least %s&n more!", coin_stringv(price - GET_MONEY(ch)));
      mobsay(npc, buffer);
      return TRUE;
    }
    /* count money end */

    act("$n begins to chant in a deep voice, starting quietly and then raising $s voice \n" \
        "slowly until the entire room is shaking. Your conscience -- and your wallet -- suddenly \n" \
        "feel much lighter!", FALSE, npc, 0, ch, TO_VICT);
    act("$n begins to chant in a deep voice, starting quietly and then raising $s voice \n" \
        "slowly until the entire room is shaking. $N suddenly looks like a huge weight was taken \n" \
        "off $S shoulders.", FALSE, npc, 0, ch, TO_NOTVICT);
    affect_remove(ch, afp);
    SUB_MONEY(ch, price, 0);

    wizlog(AVATAR, "%s cleared his epic task at %s", GET_NAME(ch), GET_NAME(npc));
    logit(LOG_PLAYER, "%s cleared his epic task at %s", GET_NAME(ch), GET_NAME(npc));
    sql_log(ch, PLAYERLOG, "Cleared epic task at %s", GET_NAME(npc));
    return TRUE;
  }
  return FALSE;
}

int block_dir(P_char ch, P_char pl, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC || cmd == CMD_MOB_MUNDANE )
    return FALSE;
  
  if( !ch || !pl )
    return FALSE;
  
  bool allowed = TRUE;
  
  if( cmd == CMD_NORTH && isname("_block_north_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  else if( cmd == CMD_EAST && isname("_block_east_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  else if( cmd == CMD_SOUTH && isname("_block_south_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  else if( cmd == CMD_WEST && isname("_block_west_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  else if( cmd == CMD_UP && isname("_block_up_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  else if( cmd == CMD_DOWN && isname("_block_down_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  else if( cmd == CMD_NORTHWEST && isname("_block_northwest_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  else if( cmd == CMD_SOUTHWEST && isname("_block_southwest_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  else if( cmd == CMD_NORTHEAST && isname("_block_northeast_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  else if( cmd == CMD_SOUTHEAST && isname("_block_southeast_", GET_NAME(ch)) )
  {
    allowed = FALSE;
  }
  
  if( !allowed )
  {
    if( IS_TRUSTED(pl) )
    {
      act("$n bows in deference as you pass by.", FALSE, ch, 0, pl, TO_VICT);
      return FALSE;      
    }
    else
    {
      act("$n blocks you.", FALSE, ch, 0, pl, TO_VICT);
      act("$n blocks $N.", FALSE, ch, 0, pl, TO_NOTVICT);
      return TRUE;
    }
  }
  
  return FALSE;
}

int undead_howl(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   tch, next, vict;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!ch)
    return FALSE;

  //Do it 5 % of the time
  if (number(0, 99) > 10)
    return FALSE;

  if (ch->in_room == NOWHERE)
    return FALSE;
  if (!ch->specials.fighting)
    return FALSE;

  act("$n&+L unleashes a hellish, low &+whowl&+L; everything becomes a shade darker as it pierces your spirit.&n",
            FALSE, ch, 0, vict, TO_NOTVICT);

  for (vict = world[ch->in_room].people; vict; vict = tch)
  {

    tch = vict->next_in_room;
    
    if (ch->group && vict->group && (ch->group == vict->group))
      continue;
    if (IS_TRUSTED(ch) || (ch == vict))
      continue;
    else
    {
      if (!NewSaves(vict, SAVING_FEAR, (int) BOUNDED(0, (GET_LEVEL(ch) - GET_LEVEL(vict)) / 2, 10)))
    {
      act("$n&+L's soulless &+whowl&n strips your body of it's very soul...&n",
            FALSE, ch, 0, vict, TO_VICT);
      die(vict, ch);
      if (!number(0, 2))
        return TRUE;
    }
    }
  }
  return TRUE;
}

#define ELIGOTH_RIFT_VNUM 43580

int eligoth_rift_spawn(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_DEATH)
  {
    P_obj obj1 = read_object(ELIGOTH_RIFT_VNUM, VIRTUAL);
    if (!obj1)
    {
      logit(LOG_DEBUG, "eligoth_rift_spawn: object failed to load.");
      debug("eligoth_rift_spawn: object failed to load.");
      return FALSE;
    }

    act("&+YAs the &+rlifeblood &+Yflows from the defeated &+LLord&+Y, his remains twist\n"
        "&+Yand writhe, spasming violently.  A fell &+Lmixture &+Yof steam and smoke begins\n"
        "&+Yto seep from the corpse as it slowly &+Lcollapses &+Yinto itself, forming a &+Ldark\n"
        "&+Lrift.&n", FALSE, ch, 0, 0, TO_ROOM);

    obj_to_room(obj1, ch->in_room);

    return (FALSE);
  }

  return FALSE;
}

#undef ELIGOTH_RIFT_VNUM

int tentacler_death(P_char tentacler, P_char ch, int cmd, char *arg)
{
   int obj_load;

   if(!tentacler)
   {
     return FALSE;
   }

   if(cmd == CMD_SET_PERIODIC)
   {
     return TRUE;
   }

   if(cmd == CMD_DEATH)
   {
     P_obj tempobj;
     obj_load = number(0, 4);
     switch(obj_load)
     {
       case 0:
        if (!(tempobj = read_object(89145, VIRTUAL)))
        {
           logit(LOG_DEBUG, "tentacler_death: object failed to load.");
           debug("tentacler_death: object failed to load.");
           return FALSE;
        }
        break;
       case 1:
        if (!(tempobj = read_object(89146, VIRTUAL)))
        {
           logit(LOG_DEBUG, "tentacler_death: object failed to load.");
           debug("tentacler_death: object failed to load.");
           return FALSE;
        }
        break;
       case 2:
        if (!(tempobj = read_object(89147, VIRTUAL)))
        {
           logit(LOG_DEBUG, "tentacler_death: object failed to load.");
           debug("tentacler_death: object failed to load.");
           return FALSE;
        }
        break;
       case 3:
        if (!(tempobj = read_object(89148, VIRTUAL)))
        {
           logit(LOG_DEBUG, "tentacler_death: object failed to load.");
           debug("tentacler_death: object failed to load.");
           return FALSE;
        }
        break;
       case 4:
        if (!(tempobj = read_object(89149, VIRTUAL)))
        {
           logit(LOG_DEBUG, "tentacler_death: object failed to load.");
           debug("tentacler_death: object failed to load.");
           return FALSE;
        }
     }
     obj_to_room(tempobj, real_room(89227));
     return TRUE;
   }

   return FALSE;
} 

int monk_remort(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char tch;
  char name[MAX_STRING_LENGTH], msg[MAX_STRING_LENGTH];
  char Gbuf[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int epiccost, plat;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  argument_interpreter(arg, name, msg);

  if (!pl && (cmd == CMD_MOB_MUNDANE))
  {
    LOOP_THRU_PEOPLE(tch, ch)
    {
      if (!number(0, 3) && GET_CLASS(tch, CLASS_CLERIC) && !IS_MULTICLASS_PC(tch))
      {
	do_say(ch, "A cleric eh?  Have you heard the rumors of clerics becoming powerful monks?", -4);
	return FALSE;
      }
    }
  }

  if (pl && !IS_PC(pl))
    return FALSE;

  if (cmd != CMD_ASK)
    return FALSE;
 
  epiccost = (int)get_property("remort.monk.epic.cost", 1000.000);
  plat = (int)get_property("remort.monk.cost", 1000000.00);
  
  if (!strcmp(msg, "monk"))
  {
    do_say(ch, "Yes Monks are a powerful kind indeed.  If you seek to become one, I can teach you for a price.", -4);
    sprintf(Gbuf, "It will cost you %s, and you must posses %d epics.", coin_stringv(plat), epiccost);
    do_say(ch, Gbuf, -4);
    do_say(ch, "Ask me 'remort' to confirm.", -4);
    return TRUE;
  }
  if (!strcmp(msg, "remort"))
  {
    if (IS_TRUSTED(pl))
    {
      send_to_char("That would be very dumb.\n", pl);
      return TRUE;
    }

    if (!GET_CLASS(pl, CLASS_CLERIC))
    {
      send_to_char("You do not posses the correct class to obtain my teachings.\n", pl);
      return TRUE;
    }
  
    if ((GET_RACE(pl) != RACE_HUMAN) &&
        (GET_RACE(pl) != RACE_GNOME) &&
        (GET_RACE(pl) != RACE_GITHZERAI))
    {
      send_to_char("I do not teach your kind!  Be gone!\n", pl);
      return TRUE;
    }

    if (pl->only.pc->epics < (int)get_property("remort.monk.epic.cost", 1000.000))
    {
      send_to_char("You are not epic enough!\n", pl);
      return TRUE;
    }

    if (GET_MONEY(pl) < plat)
    {
      send_to_char("You can't afford it!\n", pl);
      return TRUE;
    }
    
    // PASSED!

    SUB_MONEY(pl, plat, 0);
    sprintf(Gbuf, "%s takes your money.\n", ch->player.short_descr);
    send_to_char(Gbuf, pl);
    
    forget_spells(pl, -1);
    pl->player.spec = 0;
    pl->player.secondary_class = 0;
    pl->player.m_class = CLASS_MONK;
    do_start(pl, 1);

    sprintf(Gbuf2, "You begin listening to %s as he begins\n", ch->player.short_descr);
    send_to_char(Gbuf2, pl);
    send_to_char("describing the ways of the &+LM&+won&+Lk&n to you.\n", pl);
    send_to_char("Before too long, you begin to forget your priesthood.\n", pl);
    CharWait(pl, WAIT_SEC * 30);
    return TRUE;
  }
}

void finish_smelt( P_char ch, P_char pl, int vnum );

// I changed Steelgrip's smelting quest into a proc for $ purposes.
int smelter(P_char ch, P_char pl, int cmd, char *argument)
{
  char  arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], *rest;
  char  buf[MAX_STRING_LENGTH];
  int   amount, type;
  P_obj obj;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd == CMD_ASK || cmd == CMD_TELL )
  {
    rest = one_argument( argument, arg1 );
    if( !(ch == get_char_room_vis(pl, arg1)) )
      return FALSE;
    one_argument( rest, arg2 );
    while( *rest == ' ' )
    {
      rest++;
    }
    if( isname(arg2, "smelting smelt") )
    {
      sprintf( buf, "You ask $N '%s'", rest );
      act( buf, FALSE, pl, 0, ch, TO_CHAR );
      sprintf( buf, "$n asks $N '%s'", rest );
      act( buf, TRUE, pl, 0, ch, TO_ROOM );
      act( "&n$n says to $N '&+YSmelting &Nhappens to be my specialty!'", TRUE, ch, 0, pl, TO_ROOM );
      act( "&n$n claps his hands, smiling.", FALSE, ch, 0, NULL, TO_ROOM );
      do_say( ch, "I can take two small pieces of the same ore and make them a medium.  I can also take"
        " two medium pieces of the same ore and make it a large.  Same price for either smelt!  &+W10"
        " platinum &Nfor &+ciron&n, &+Ctin&N, &+ycopper&N, or silver.  &+W50 platinum &Nfor &+Ygold&N,"
        " &+Wplatinum&n, or &+mmithril&n, and &+W100 platinum&N for &+Madamantium&n.", CMD_SAY );
      do_say( ch, "Remember, I only deal in &+Wplatinum&n now, so keep your other coins.", CMD_SAY );
      return TRUE;
    }
  }

  if( cmd == CMD_GIVE )
  {
    rest = one_argument(argument, arg1);
    rest = one_argument(rest, arg2);
    // If it's coins.
    if( is_number(arg1) && strlen(arg1) <= 7 )
    {
      // Trying to give a negative number of coins?
      if( (amount = atoi(arg1)) < 0 )
      {
        return FALSE;
      }
      type = coin_type(arg2);
      // Bad coin type or not enough coins.
      if( (type < 0) || (pl->points.cash[type] < amount) )
      {
        return FALSE;
      }
      // Is pl giving to ch?
      rest = one_argument(rest, arg1);
      if( !*arg1 || !(ch == get_char_room_vis( pl, arg1 )) )
      {
        return FALSE;
      }
      pl->points.cash[type] -= amount;
      ch->points.cash[type] += amount;
      sprintf( buf, "You give %d %s to $N.", amount, (type == 0) ? "copper" : (type == 1) ? "silver" :
        (type == 2) ? "gold" : "platinum" );
      act(buf, FALSE, pl, obj, ch, TO_CHAR);
      finish_smelt( ch, pl, 0 );
      return TRUE;
    }
    else if( (obj = get_obj_in_list_vis(pl, arg1, pl->carrying)) )
    {
      // If pl isn't giving to ch, or is trying to give a cursed/souldbound item.
      if( (ch != get_char_room_vis(pl, arg2)) || IS_SET(obj->extra_flags, ITEM_NODROP)
        || IS_OBJ_STAT2(obj, ITEM2_SOULBIND) )
      {
        return FALSE;
      }
      // If it's not a valid ore vnum.
      if( OBJ_VNUM(obj) < LOWEST_ORE_VNUM || OBJ_VNUM(obj) > LOWEST_ORE_VNUM + NUMBER_ORE_TYPES * 3 - 1 )
      {
        return FALSE;
      }

      if( (OBJ_VNUM(obj) - LOWEST_ORE_VNUM) % 3 == 2 )
      {
        act( "$n says '$p is too big to smelt into something bigger.'", FALSE, ch, obj, NULL, TO_ROOM );
        return TRUE;
      }

      if( vnum_in_inv( ch, OBJ_VNUM(obj) ) > 1 )
      {
        act( "$n says 'I already have enough of $p to smelt.'", FALSE, ch, obj, NULL, TO_ROOM );
        type = (OBJ_VNUM(obj) - LOWEST_ORE_VNUM) / 3;
        switch( type )
        {
          case 0:
          case 1:
          case 2:
          case 3:
            amount += 10;
            break;
          case 4:
          case 5:
          case 6:
            amount += 50;
            break;
          case 7:
            amount += 100;
            break;
        }
        if( amount - GET_PLATINUM(ch) > 0 )
        {
          sprintf( buf, "But I need %d more &+Wplatinum&n to complete this transaction.", amount - GET_PLATINUM(ch) );
          do_say( ch, buf, CMD_SAY );
          return TRUE;
        }
        // Otherwise we have the cash and the ore so finish.
        else
        {
          GET_PLATINUM(ch) -= amount;
          finish_smelt( ch, pl, OBJ_VNUM(obj) );
          return TRUE;
        }
      }
      else
      {
        obj_from_char(obj);
        act("$n gives $p to $N.", TRUE, pl, obj, ch, TO_ROOM);
        act("You give $p to $N.", FALSE, pl, obj, ch, TO_CHAR);
        obj_to_char(obj, ch);
        // If we have enough ores to smelt.
        if( vnum_in_inv( ch, OBJ_VNUM(obj) ) > 1 )
        {
          switch( (OBJ_VNUM(obj) - LOWEST_ORE_VNUM) / 3 )
          {
          case 0:
          case 1:
          case 2:
          case 3:
            amount = 10;
            break;
          case 4:
          case 5:
          case 6:
            amount = 50;
            break;
          case 7:
            amount = 100;
            break;
          }
          // If the smelter doesn't need more cash.
          if( amount - GET_PLATINUM(ch) <= 0 )
          {
            GET_PLATINUM(ch) -= amount;
            finish_smelt( ch, pl, OBJ_VNUM(obj) );
            return TRUE;
          }
          else
          {
            sprintf( buf, "I need %d more &+Wplatinum&n to complete this transaction.", amount - GET_PLATINUM(ch) );
            do_say( ch, buf, CMD_SAY );
          }
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

void finish_smelt( P_char ch, P_char pl, int vnum )
{
  char  buf[MAX_STRING_LENGTH], oreType[32];
  int   price;
  P_obj ore, obj;

  ore = obj = NULL;

  if( vnum == 0 )
  {
    for( obj = ch->carrying; obj; obj = obj->next_content )
    {
      // If it's a small/medium adamantium ore..
      if( OBJ_VNUM(obj) == LOWEST_ORE_VNUM + 7 * 3
        || OBJ_VNUM(obj) == LOWEST_ORE_VNUM + 7 * 3 + 1 )
      {
        // Look through the rest of the list for the same vnum.
        for( ore = obj->next_content; ore; ore = ore->next_content )
        {
          // If we've found a match.
          if( OBJ_VNUM(obj) == OBJ_VNUM(ore) )
          {
            vnum = OBJ_VNUM(obj);
            price = 100;
            break;
          }
        }
        if( vnum > 0 )
        {
          break;
        }
      }
    }
    if( vnum == 0 )
    {
      for( obj = ch->carrying; obj; obj = obj->next_content )
      {
        // If it's a small/medium gold/plat/mith ore..
        if( ((OBJ_VNUM(obj) - LOWEST_ORE_VNUM) % 3 == 0
          || (OBJ_VNUM(obj) - LOWEST_ORE_VNUM) % 3 == 1)
          && OBJ_VNUM(obj) >= LOWEST_ORE_VNUM + 4 * 3
          && OBJ_VNUM(obj) <= LOWEST_ORE_VNUM + 6 * 3 + 1 )
        {
          // Look through the rest of the list for the same vnum.
          for( ore = obj->next_content; ore; ore = ore->next_content )
          {
            // If we've found a match.
            if( OBJ_VNUM(obj) == OBJ_VNUM(ore) )
            {
              vnum = OBJ_VNUM(obj);
              price = 50;
              break;
            }
          }
          if( vnum > 0 )
          {
            break;
          }
        }
      }
    }
    if( vnum == 0 )
    {
      for( obj = ch->carrying; obj; obj = obj->next_content )
      {
        // If it's a small/medium iron/tin/copper/silver ore..
        if( (( (OBJ_VNUM(obj) - LOWEST_ORE_VNUM) % 3 == 0
          || (OBJ_VNUM(obj) - LOWEST_ORE_VNUM) ) % 3 == 1)
          && OBJ_VNUM(obj) >= LOWEST_ORE_VNUM
          && OBJ_VNUM(obj) <= LOWEST_ORE_VNUM + 3 * 3 + 1 )
        {
          // Look through the rest of the list for the same vnum.
          for( ore = obj->next_content; ore; ore = ore->next_content )
          {
            // If we've found a match.
            if( OBJ_VNUM(obj) == OBJ_VNUM(ore) )
            {
              vnum = OBJ_VNUM(obj);
              price = 10;
              break;
            }
          }
          if( vnum > 0 )
          {
            break;
          }
        }
      }
    }
    // If we didn't find one, return
    if( vnum == 0 )
    {
      return;
    }
    if( GET_PLATINUM(ch) < price )
    {
      sprintf( buf, "I need %d more &+Wplatinum&n to complete this transaction.", price - GET_PLATINUM(ch) );
      do_say( ch, buf, CMD_SAY );
      return;
    }
    GET_PLATINUM(ch) -= price;
  }
  else
  {
    for( obj = ch->carrying; obj; obj = obj->next_content )
    {
      if( OBJ_VNUM(obj) == vnum )
      {
        break;
      }
    }
    // If we didn't find even one, just return.
    if( !obj )
    {
      return;
    }
    for( ore = obj->next_content; ore; ore = ore->next_content )
    {
      if( OBJ_VNUM(ore) == vnum )
      {
        break;
      }
    }
    // If we didn't find a match, return.
    if( !ore )
    {
      return;
    }
  }

  price = ore->cost + obj->cost;
  obj_from_char(ore);
  obj_from_char(obj);

  ore = read_object( vnum+1, VIRTUAL );
  ore->cost = price;
  obj_to_char( ore, pl );

  switch( (OBJ_VNUM(ore) - LOWEST_ORE_VNUM) / 3 )
  {
    case 0:
      sprintf( oreType, "&+ciron&n" );
      break;
    case 1:
      sprintf( oreType, "&+Ctin&n" );
      break;
    case 2:
      sprintf( oreType, "&+ycopper&n" );
      break;
    case 3:
      sprintf( oreType, "&nsilver" );
      break;
    case 4:
      sprintf( oreType, "&+Ygold&n" );
      break;
    case 5:
      sprintf( oreType, "&+Wplatinum&n" );
      break;
    case 6:
      sprintf( oreType, "&+mmithril&n" );
      break;
    case 7:
      sprintf( oreType, "&+Madamantium&n" );
      break;
    default:
      sprintf( oreType, "&+Lmetal&n" );
      break;
  }
  sprintf( buf, "$n &+RSMELTS&n some %s!", oreType );
  act( buf, FALSE, ch, NULL, NULL, TO_ROOM );
  act( "$n gives $p to $N.", TRUE, ch, ore, pl, TO_ROOM );
}
