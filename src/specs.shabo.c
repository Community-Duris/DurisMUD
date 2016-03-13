/*
   ***************************************************************************
   *  File: specs.shabo.c                                     Part of Duris *
   *  Usage: special procedures zone Shaboath                               *
   *  Copyright  1990, 1991 - see 'license.doc' for complete information.    *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "justice.h"
#include "assocs.h"
#include "graph.h"
#include "disguise.h"

/*
   external variables
 */
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern char *command[];
extern const char *dirs[];
extern const char *race_types[];
extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int innate_abilities[];
extern int planes_room_num[];
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct dex_app_type dex_app[52];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

#define ZONE_GREAT_SHABOATH  129
#define GREAT_SHABOATH_START 32800
#define GREAT_SHABOATH_END   32929
#define GRAND_SAVANT 32881
#define NECROMANCER_BOSS_ROOM 32912

int pesky_imp_chest(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   peskyimp;
  int      count = 0;
  char     name[512];

  if (!ch || !obj || !arg)
    return FALSE;

  one_argument(arg, name);

  if (cmd == CMD_OPEN && obj->value[3] == 0 &&
      !IS_SET(obj->value[1], CONT_LOCKED) &&
      obj == get_obj_in_list_vis(ch, name, world[ch->in_room].contents))
  {
    obj->value[3] = 1;
    for (count = 0; count < 3; count++)
    {
      peskyimp = read_mobile(32861, VIRTUAL);
      if (!peskyimp)
      {
        logit(LOG_EXIT, "assert: error in pesky_imp_chest() proc");
        return FALSE;
      }
      char_to_room(peskyimp, ch->in_room, 0);
      act("$N &+Lleaps out of a chest and charges to attack!&N", FALSE,
          peskyimp, 0, peskyimp, TO_NOTVICT);
      MobStartFight(peskyimp, ch);
    }
  }
  return FALSE;
}

int monitor_trident(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   kala;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!dam)
    return (FALSE);
  if (!ch)
    return (FALSE);

  if (!OBJ_WORN(obj) || (obj->loc.wearing != ch))
    return (FALSE);
  kala = (P_char) arg;
  if (!kala)
    return (FALSE);
  if (number(0, 30))
    return (FALSE);
  act
    ("&+G$p &+Gwielded by $n &+Wflashes &+Gand a coating of slime engulfs you!&N",
     TRUE, ch, obj, kala, TO_VICT);
  act
    ("&+G$p &+Gwielded by $n &+Wflashes &+Gand a coating of slime engulfs $N!&N",
     TRUE, ch, obj, kala, TO_ROOM);
  act("&+GYour $p &+Wflashes &+Gand a coating of slime engulfs $N!&N", TRUE,
      ch, obj, kala, TO_CHAR);
  spell_acidimmolate(35, ch, NULL, 0, kala, 0);
  return (TRUE);
}

int flayed_mind_mask(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      curr_time;


  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
    return FALSE;

  if( argument && (cmd == CMD_SAY) )
  {
    arg = argument;

    while (*arg == ' ')
      arg++;

    if( !strcmp(arg, "vision") )
    {
      if (!say(ch, arg))
      {
        return TRUE;
      }

      curr_time = time(NULL);
      // 30 min timer.
      if (obj->timer[0] + 1800 <= curr_time)
      {
        act("&+LYour senses sharpen as $q &+Lbegins to &+Wglow&+w and you can see once again!", FALSE, ch, obj, obj, TO_CHAR);
        act("$n's $q &+Wglows&+w vibrantly for a moment before the aura &+Lsubsides...&N", TRUE, ch, obj, NULL, TO_ROOM);
        spell_cure_blind(35, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = curr_time;
      }
      return TRUE;
    }
  }
  return FALSE;
}


int stalker_cloak(P_obj obj, P_char ch, int cmd, char *argument)
{
  char *arg;
  int   curr_time;


  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( argument && (cmd == CMD_SAY) )
  {
    arg = argument;
    while (*arg == ' ')
      arg++;
    if( !strcmp(arg, "reduce") )
    {
      if( !say(ch, arg) )
      {
        return TRUE;
      }
      curr_time = time(NULL);
      // 3 min timer.
      if( obj->timer[0] + 180 <= curr_time )
      {
        act("You whisper 'reduce' to your $q...", FALSE, ch, obj, obj, TO_CHAR);
        act("$n whispers 'reduce' to $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
        spell_reduce(35, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = curr_time;
      }
      return TRUE;
    }
    else if( !strcmp(arg, "enlarge") )
    {
      if( !say(ch, arg) )
      {
        return TRUE;
      }
      curr_time = time(NULL);
      // 3 min timer.
      if( obj->timer[0] + 180 <= curr_time )
      {
        act("You whisper 'enlarge' to your $q...", FALSE, ch, obj, obj, TO_CHAR);
        act("$n whispers 'enlarge' to $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
        spell_enlarge(35, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = curr_time;
      }
      return TRUE;
    }
    else if (!strcmp(arg, "hide"))
    {
      if (!say(ch, arg))
      {
        return TRUE;
      }
      curr_time = time(NULL);
      // 1 min timer.
      if( obj->timer[0] + 60 <= curr_time )
      {
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        SET_BIT(ch->specials.affected_by, AFF_HIDE);
        obj->timer[0] = curr_time;
      }
      return TRUE;
    }
  }
  return FALSE;
}

int finslayer_air(P_obj obj, P_char ch, int cmd, char *argument)
{
  char *arg;
  int   curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if(!IS_ALIVE(ch) || !OBJ_WORN(obj))
  {
    return FALSE;
  }

  if( argument && (cmd == CMD_SAY) )
  {
    arg = argument;

    while( *arg == ' ' )
    {
      arg++;
    }

    if( !strcmp(arg, "air") )
    {
      if( !say(ch, arg) )
      {
        return TRUE;
      }

      curr_time = time(NULL);
      // 1 min timer.
      if( obj->timer[0] + 60 <= curr_time )
      {
        act("Your $q begins to spew forth breathable air...", FALSE, ch, obj, obj, TO_CHAR);
        act("$n's $q begins to spew forth breathable air...&N", TRUE, ch, obj, NULL, TO_ROOM);
        spell_airy_water(35, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = curr_time;
      }
      return TRUE;
    }
  }
  return FALSE;
}

int aboleth_pendant(P_obj obj, P_char ch, int cmd, char *argument)
{
  int      curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC || !obj )
  {
    return FALSE;
  }

  if( !OBJ_WORN_POS(obj, WEAR_NECK_1) && !OBJ_WORN_POS(obj, WEAR_NECK_2) )
  {
    return FALSE;
  }

  ch = obj->loc.wearing;
  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  curr_time = time(NULL);
  // 900 for 15 mins
  if( obj->timer[0] + 900 <= curr_time )
  {
    act("Your $q hums briefly.", FALSE, ch, obj, NULL, TO_CHAR);
    act("$n's $q hums briefly.", FALSE, ch, obj, NULL, TO_ROOM);
    SET_BIT(ch->specials.affected_by, AFF_HIDE);
    obj->timer[0] = curr_time;
  }
  return FALSE;
}

int tower_summoning(P_obj obj, P_char ch, int cmd, char *arg)
{
//  register P_char i;
  P_char   istalker;
  P_char   vict;
  P_char   smob, next_mob;
  int      scount = 0;
  int      count = 0;
  int      sunum = 0;

//  P_obj i;

  /*
     check for periodic event calls 
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || (GET_POS(ch) == POS_SITTING) ||
      (GET_STAT(ch) == !STAT_NORMAL))
    return FALSE;

  if (IS_NPC(ch) && !IS_PC_PET(ch))
    return FALSE;

  if IS_TRUSTED
    (ch) return FALSE;
  /*
     change all instances of 32898/32904 if v-nums of rooms are changed! 
   */
  if (ch->in_room != real_room(32898))
    return FALSE;

  if (cmd == CMD_NORTH)
  {
    if (!world[ch->in_room].dir_option[NORTH])
    {
      logit(LOG_EXIT,
            "Tower of Summonings entrance room [%d] does not have north exit! Fix tower_summoning()",
            world[ch->in_room].number);
      wizlog(MINLVLIMMORTAL, "error in proc tower_summoning ");
      return FALSE;
    }
    else
      if (IS_SET(world[ch->in_room].dir_option[NORTH]->exit_info, EX_BLOCKED))
      return FALSE;
    else
      if (IS_SET(world[ch->in_room].dir_option[NORTH]->exit_info, EX_CLOSED)
          && (!IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM)))
      return FALSE;
    else
    {

      /* summon proc */
      spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
      
      for (smob = world[real_room0(32904)].people; smob; smob = next_mob) {
        next_mob = smob->next_in_room;
        
        if (IS_NPC(smob) && GET_VNUM(smob) == 32871) {
          scount++;
        }
      }

      if (scount >= 10) {
        return FALSE;
      }

      sunum = number(1, 2);
      for (count = 0; count < sunum; count++)
      {
        istalker = read_mobile(32871, VIRTUAL);
        if (!istalker)
        {
          logit(LOG_EXIT, "assert: error in tower_summoning() proc");
          return FALSE;
        }

        // prevent huge spam listing of mobs when players screw up and have to CR
        add_event(event_pet_death, 4 * 60 * 4, istalker, NULL, NULL, 0, NULL, 0);

        char_to_room(istalker, real_room(32904), 0);
        act("&+cAn invisible stalker&N comes out of hiding!", TRUE, ch, 0, 0,
            TO_CHAR);
//     act("&+cAn invisible stalker&N comes out of hiding!", TRUE, ch, 0, 0, TO_ROOM);
//               MobStartFight(istalker, ch);
//               return FALSE;
      }
//            return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}


int shabo_trap_north(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || (GET_POS(ch) == POS_SITTING) ||
      (GET_STAT(ch) == !STAT_NORMAL))
    return FALSE;

  if (IS_PC_PET(ch))
    return FALSE;

  if IS_TRUSTED
    (ch) return FALSE;

  if (cmd == CMD_NORTH)
  {
    if (world[ch->in_room].dir_option[NORTH])
      if (IS_SET(world[ch->in_room].dir_option[NORTH]->exit_info, EX_BLOCKED))
        return FALSE;
      else
        if (IS_SET(world[ch->in_room].dir_option[NORTH]->exit_info, EX_CLOSED)
            && (!IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM)))
        return FALSE;
      else
      {
        act("&+LYou hear a soft click.", TRUE, ch, 0, 0, TO_CHAR);
        spell_wither(60, ch, NULL, 0, ch, 0);
      }
    return FALSE;
  }
  return FALSE;
}


int shabo_trap_south(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || (GET_POS(ch) == POS_SITTING) ||
      (GET_STAT(ch) == !STAT_NORMAL))
    return FALSE;

  if (IS_NPC(ch) && !IS_PC_PET(ch))
    return FALSE;

  if IS_TRUSTED
    (ch) return FALSE;

  if (cmd == CMD_SOUTH)
  {
    if (world[ch->in_room].dir_option[SOUTH])

      if (IS_SET(world[ch->in_room].dir_option[SOUTH]->exit_info, EX_BLOCKED))
        return FALSE;
      else
        if (IS_SET(world[ch->in_room].dir_option[SOUTH]->exit_info, EX_CLOSED)
            && (!IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM)))
        return FALSE;
      else
      {
        act("&+LYou hear a soft click.", TRUE, ch, 0, 0, TO_CHAR);
        if (number(0, 20) < 5)
        {
          act("&+LYou get hit with a stunning force!.", TRUE, ch, 0, 0,
              TO_CHAR);
          act("&+L$n gets hit with a stunning force!", TRUE, ch, 0, 0,
              TO_NOTVICT);
          Stun(ch, ch, (dice(1, 3) * PULSE_VIOLENCE), TRUE);
          SET_POS(ch, POS_KNEELING + GET_STAT(ch));
          CharWait(ch, PULSE_VIOLENCE * 1);
        }
      }
    return FALSE;
  }
  return FALSE;
}

int shabo_trap_south_two(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || (GET_POS(ch) == POS_SITTING) ||
      (GET_STAT(ch) == !STAT_NORMAL))
    return FALSE;

  if (IS_NPC(ch) && !IS_PC_PET(ch))
    return FALSE;

  if IS_TRUSTED
    (ch) return FALSE;

  if (cmd == CMD_SOUTH)
  {
    if (world[ch->in_room].dir_option[SOUTH])
      if (IS_SET(world[ch->in_room].dir_option[SOUTH]->exit_info, EX_BLOCKED))
        return FALSE;
      else
        if (IS_SET(world[ch->in_room].dir_option[SOUTH]->exit_info, EX_CLOSED)
            && (!IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM)))
        return FALSE;
      else
      {
        act("&+LYou hear a soft click.", TRUE, ch, 0, 0, TO_CHAR);
        spell_wither(60, ch, NULL, 0, ch, 0);
      }
    return FALSE;
  }
  return FALSE;
}

int shabo_trap_down(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || (GET_POS(ch) == POS_SITTING) ||
      (GET_STAT(ch) == !STAT_NORMAL))
    return FALSE;

  if (IS_NPC(ch) && !IS_PC_PET(ch))
    return FALSE;

  if IS_TRUSTED
    (ch) return FALSE;

  if (cmd == CMD_DOWN)
  {
    if (world[ch->in_room].dir_option[DOWN])
      if (IS_SET(world[ch->in_room].dir_option[DOWN]->exit_info, EX_BLOCKED))
        return FALSE;
      else
        if (IS_SET(world[ch->in_room].dir_option[DOWN]->exit_info, EX_CLOSED)
            && (!IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM)))
        return FALSE;
      else
      {
        act("&+LYou hear a soft click.", TRUE, ch, 0, 0, TO_CHAR);
        spell_major_paralysis(60, ch, 0, 0, ch, 0);
      }
    return FALSE;
  }
  return FALSE;
}

int shabo_trap_up(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || (GET_POS(ch) == POS_SITTING) ||
      (GET_STAT(ch) == !STAT_NORMAL))
    return FALSE;

  if (IS_NPC(ch) && !IS_PC_PET(ch))
    return FALSE;

  if IS_TRUSTED
    (ch) return FALSE;

  if (cmd == CMD_UP)
  {
    if (world[ch->in_room].dir_option[UP])
      if (IS_SET(world[ch->in_room].dir_option[UP]->exit_info, EX_BLOCKED))
        return FALSE;
      else
        if (IS_SET(world[ch->in_room].dir_option[UP]->exit_info, EX_CLOSED)
            && (!IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM)))
        return FALSE;
      else
      {
        act("&+LYou hear a soft click.", TRUE, ch, 0, 0, TO_CHAR);
        if (number(0, 3) == 1)
        {
          act("&+LSomething&N's sweep sends you crashing to the ground!&N",
              TRUE, ch, obj, ch, TO_CHAR);
          act("&+LSomething&N's sweep sends $N crashing to the ground!!&N",
              TRUE, ch, obj, ch, TO_NOTVICT);
          SET_POS(ch, POS_SITTING + GET_STAT(ch));
          CharWait(ch, PULSE_VIOLENCE * 2);
        }
      }
    return FALSE;
  }
  return FALSE;
}

int shabo_trap_up_two(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   trapmob;
  int      dam, kala, kala2, kala3;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || (GET_POS(ch) == POS_SITTING) ||
      (GET_STAT(ch) == !STAT_NORMAL))
    return FALSE;

  if (IS_NPC(ch) && !IS_PC_PET(ch))
    return FALSE;

  if IS_TRUSTED
    (ch) return FALSE;


  if (cmd == CMD_UP)
  {
    if (world[ch->in_room].dir_option[UP])
      if (IS_SET(world[ch->in_room].dir_option[UP]->exit_info, EX_BLOCKED))
        return FALSE;
      else
        if (IS_SET(world[ch->in_room].dir_option[UP]->exit_info, EX_CLOSED)
            && (!IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM)))
        return FALSE;
      else
      {
        trapmob = read_mobile(14, VIRTUAL);     // load a temporary mob
        if (!trapmob)
        {
          logit(LOG_EXIT, "assert: shabo trapmob");
          wizlog(MINLVLIMMORTAL, "error in proc shaob_trap_up_two");
          return FALSE;
        }
        char_to_room(trapmob, ch->in_room, -2); // do spell shit....... 
        act("&+LYou hear a soft click.", TRUE, ch, 0, 0, TO_CHAR);
        spell_color_spray(60, trapmob, NULL, SPELL_TYPE_SPELL, ch, 0);
        extract_char(trapmob);  // then get rid of the mob


      }
    return FALSE;
  }
  return FALSE;
}

int shabo_trap_north_two(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   trapmob;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || !IS_AWAKE(ch) || (GET_POS(ch) == POS_SITTING) ||
      (GET_STAT(ch) == !STAT_NORMAL))
    return FALSE;

  if (IS_NPC(ch) && !IS_PC_PET(ch))
    return FALSE;

  if IS_TRUSTED
    (ch) return FALSE;


  if (cmd == CMD_NORTH)
  {
    if (world[ch->in_room].dir_option[NORTH])

      if (IS_SET(world[ch->in_room].dir_option[NORTH]->exit_info, EX_BLOCKED))
        return FALSE;
      else
        if (IS_SET(world[ch->in_room].dir_option[NORTH]->exit_info, EX_CLOSED)
            && (!IS_AFFECTED4(ch, AFF4_PHANTASMAL_FORM)))
        return FALSE;
      else
      {
        act("&+LYou hear a soft click.", TRUE, ch, 0, 0, TO_CHAR);
        trapmob = read_mobile(14, VIRTUAL);     // load a temporary mob
        if (!trapmob)
        {
          logit(LOG_EXIT, "assert: shabo trapmob");
          wizlog(MINLVLIMMORTAL, "error in proc shabo_trap_north_two");
          return FALSE;
        }
        char_to_room(trapmob, ch->in_room, -2); // do spell shit.......
        act("&+LYou hear a soft click.", TRUE, ch, 0, 0, TO_CHAR);
        spell_wither(60, trapmob, NULL, 0, ch, 0);
        extract_char(trapmob);  // then get rid of the mob
      }
    return FALSE;
  }
  return FALSE;
}

/* MOX */


int mox_totem(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      rand;
  int      curr_time;
  P_char   temp_ch;
  P_char   kala;
  char     e_pos;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }
  if( cmd != CMD_PERIODIC || !(OBJ_WORN(obj) || OBJ_CARRIED(obj)) )
  {
    return FALSE;
  }

  temp_ch = ch;
  if( OBJ_WORN(obj) )
    ch = obj->loc.wearing;
  else
    ch = obj->loc.carrying;

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  e_pos = ((obj->loc.wearing->equipment[HOLD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);

  if( !e_pos )
  {
    return FALSE;
  }

  curr_time = time(NULL);

  if( !IS_SET(world[ch->in_room].room_flags, NO_MAGIC) )
  {
    if (obj->timer[0] + 30 <= curr_time)
    {
      obj->timer[0] = curr_time;
      if (GET_HIT(ch) < GET_MAX_HIT(ch))
      {
        act("&+w$n&+w's $q &+wglows and $n &+wis bathed in a healing aura.&N", FALSE, ch, obj, 0, TO_ROOM);
        act("&+wYour $q &+wglows and bathes you in a healing aura.&N", FALSE, ch, obj, 0, TO_CHAR);
        spell_mending(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
        return FALSE;
      }
    }
  }

  if (IS_FIGHTING(ch) && !number(0, 2))
  {
    kala = ch->specials.fighting;
    rand = number(0,3);
    switch (rand)
    {
      case 0:
        act("&+LYou point your $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_CHAR);
        act("&+L$n points $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_NOTVICT);
        act("&+L$n points $p &+Lat &+Wyou&+L!&N", TRUE, ch, obj, kala, TO_VICT);
        spell_snailspeed(60, ch, NULL, SPELL_TYPE_SPELL, kala, 0);
        break;
      case 1:
        act("&+LYou point your $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_CHAR);
        act("&+L$n points $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_NOTVICT);
        act("&+L$n points $p &+Lat &+Wyou&+L!&N", TRUE, ch, obj, kala, TO_VICT);
        spell_molevision(60, ch, NULL, SPELL_TYPE_SPELL, kala, 0);
        break;
      case 2:
        act("&+LYou point your $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_CHAR);
        act("&+L$n points $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_NOTVICT);
        act("&+L$n points $p &+Lat &+Wyou&+L!&N", TRUE, ch, obj, kala, TO_VICT);
        spell_malison(60, ch, NULL, SPELL_TYPE_SPELL, kala, 0);
        break;
    case 3:
        act("&+LYou point your $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_CHAR);
      act("&+L$n points $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_NOTVICT);
      act("&+L$n points $p &+Lat &+Wyou&+L!&N", TRUE, ch, obj, kala, TO_VICT);
      spell_soul_disturbance(60, ch, NULL, SPELL_TYPE_SPELL, kala, 0);
      break;
/* Call of the wild isn't in game right now..
    case 4:
      act("&+LYou point your $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_CHAR);
      act("&+L$n points $p &+Lat &+W$N&+L.&N", TRUE, ch, obj, kala, TO_NOTVICT);
      act("&+L$n points $p &+Lat &+Wyou&+L!&N", TRUE, ch, obj, kala, TO_VICT);
      spell_call_of_the_wild(60, ch, NULL, SPELL_TYPE_SPELL, kala, 0);
      break;
*/
    }
    return FALSE;
  }
  return FALSE;
}




void event_shabo_racechange(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type *af;

  if ((af = get_spell_from_char(ch, TAG_RACE_CHANGE)) == NULL)
  {
    send_to_char
      ("&+WPossible serious screwup in the racechange proc! Tell a coder as once!&n\r\n",
       ch);
    wizlog(57,
           "Char %s found with racechange event but without racechange affect!",
           GET_NAME(ch));
    return;
  }
  else if (((world[ch->in_room].number > GREAT_SHABOATH_START) &&
            (world[ch->in_room].number < GREAT_SHABOATH_END)) || number(0, 2))
  {
    add_event(event_shabo_racechange, PULSE_VIOLENCE, ch, 0, 0, 0, 0, 0);
    return;
  }
  else
  {
    ch->player.race = af->modifier;
    affect_remove(ch, af);
    send_to_char
      ("&+LYou feel greatly relieved as the magic twisting your body finally fades away...&n\r\n",
       ch);
    //Remove all equipment so no one can cheese thri-kreen arms - Drannak 12/12/12
    int k = 0;
    P_obj temp_obj;
    for (k = 0; k < MAX_WEAR; k++)
    {
      temp_obj = ch->equipment[k];
      if(temp_obj)
      {
        if (obj_index[temp_obj->R_num].func.obj != NULL)
          (*obj_index[temp_obj->R_num].func.obj) (temp_obj, ch, CMD_REMOVE, (char *) "all");
        obj_to_char(unequip_char(ch, k), ch);
      }
    }
    send_to_char
      ("...Brr, you suddenly feel very naked.\r\n",
       ch);
    return;
  }
}



#define NUMBER_RACES_FOR_GOOD 11
#define NUMBER_RACES_FOR_EVIL 9
#define NUMBER_RACES_FOR_UNDEAD 8

int shaboath_alternation_tower(int room, P_char ch, int cmd, char *argument)
{
  P_char   tch, next;
  struct affected_type *af;
  bool     did_something = FALSE;
  int      k, i = 0;


  int      goodie_races[NUMBER_RACES_FOR_GOOD] = { RACE_HUMAN, RACE_GREY,
    RACE_MOUNTAIN, RACE_BARBARIAN,
    RACE_GNOME, RACE_HALFLING,
    RACE_HALFELF, RACE_CENTAUR,
    RACE_SGIANT, RACE_MINOTAUR,
    RACE_THRIKREEN
  };

  int      evil_races[NUMBER_RACES_FOR_EVIL] = { RACE_DROW, RACE_DUERGAR,
    RACE_GITHYANKI, RACE_OGRE,
    RACE_GOBLIN, RACE_ORC,
    RACE_TROLL, RACE_MINOTAUR,
    RACE_THRIKREEN
  };

  int      undead_races[NUMBER_RACES_FOR_UNDEAD] =
    { RACE_PLICH, RACE_PVAMPIRE,
    RACE_PDKNIGHT, RACE_SHADE,
    RACE_REVENANT, RACE_PSBEAST,
    RACE_WIGHT, RACE_PHANTOM
  };


  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd)
    return FALSE;

  for (tch = world[real_room(room)].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if (IS_TRUSTED(tch))
      continue;

    if (IS_NPC(tch))
      continue;

    if (IS_DISGUISE(tch))
      remove_disguise(tch, TRUE);

    if ((af = get_spell_from_char(tch, TAG_RACE_CHANGE)) == NULL)
    {

      struct affected_type new_affect;

      af = &new_affect;
      memset(af, 0, sizeof(new_affect));
      af->type = TAG_RACE_CHANGE;
      af->duration = -1;
      af->flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
      af->modifier = GET_RACE(tch);
      affect_to_char(tch, af);
      add_event(event_shabo_racechange, PULSE_VIOLENCE, tch, 0, 0, 0, 0, 0);

      if (RACE_GOOD(tch))
      {

        for (k = number(0, NUMBER_RACES_FOR_GOOD - 1);
             GET_RACE(tch) == goodie_races[k];
             k = number(0, NUMBER_RACES_FOR_GOOD - 1)) ;

        tch->player.race = goodie_races[k];

      }
      else if (RACE_EVIL(tch))
      {

        for (k = number(0, NUMBER_RACES_FOR_EVIL - 1);
             GET_RACE(tch) == evil_races[k];
             k = number(0, NUMBER_RACES_FOR_EVIL - 1)) ;

        tch->player.race = evil_races[k];

      }
      else if (RACE_PUNDEAD(tch))
      {

        for (k = number(0, NUMBER_RACES_FOR_UNDEAD - 1);
             GET_RACE(tch) == undead_races[k];
             k = number(0, NUMBER_RACES_FOR_UNDEAD - 1)) ;

        tch->player.race = undead_races[k];

      }

      did_something = TRUE;
    }
  }

  return did_something;
}


int shaboath_necromancy_tower(int room, P_char ch, int cmd, char *argument)
{
  P_obj    obj, next_obj;
  P_char   necro_teacher;
  int      was_in_room, raised = 0;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd)
    return FALSE;

  for (obj = world[real_room(room)].contents; obj; obj = next_obj)
  {
    next_obj = obj->next_content;

    if ((GET_ITEM_TYPE(obj) == ITEM_CORPSE))
    {
/*          ((necro_teacher = get_char_num(NECROMANCER_BOSS)) != 0) &&
          can_raise_undead(necro_teacher, 56)) {
        wizlog(57,"necro teacher");
        was_in_room = necro_teacher->in_room;
        char_from_room(necro_teacher);
        char_to_room(necro_teacher, room, -1);
        spell_animate_dead(GET_LEVEL(necro_teacher), necro_teacher, 0, obj);
        char_from_room(necro_teacher);
        char_to_room(necro_teacher, was_in_room, -1);*/
      obj_from_room(obj);
      send_to_room
        ("&+LA black tentacle creeps in from above, picks the corpse, and departs dragging it behind...&n\r\n",
         real_room(room));
      obj_to_room(obj, real_room(NECROMANCER_BOSS_ROOM));
      raised++;

    }
  }
  if (raised)
    return TRUE;
  else
    return FALSE;
}

int shaboath_enchantment_tower(int room, P_char ch, int cmd, char *argument)
{
  P_char   tch, next;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd)
    return FALSE;

  for (tch = world[real_room(room)].people; tch; tch = next)
  {
    next = tch->next_in_room;

    if (IS_PC(tch) && !number(0, 3))
      switch (number(1, 16))
      {
      case 1:
        spell_curse(50, tch, NULL, SPELL_TYPE_SPELL, tch, 0);
        break;
      case 2:
        spell_blindness(50, tch, 0, 0, tch, 0);
        break;
      case 3:
        spell_silence(50, tch, NULL, 0, tch, 0);
        break;
      case 4:
        spell_mousestrength(50, tch, 0, 0, tch, 0);
        break;
      case 5:
        spell_molevision(50, tch, 0, 0, tch, 0);
        break;
      case 6:
        spell_shrewtameness(50, tch, 0, 0, tch, 0);
        break;
      case 7:
        spell_malison(50, tch, 0, 0, tch, 0);
        break;
      case 8:
        spell_feeblemind(50, tch, NULL, 0, tch, 0);
        break;
      case 9:
        spell_ray_of_enfeeblement(50, tch, NULL, 0, tch, 0);
        break;
      case 10:
        spell_sleep(50, tch, NULL, 0, tch, 0);
        break;
      case 11:
        spell_dispel_magic(50, tch, NULL, SPELL_TYPE_SPELL, tch, 0);
        break;
      case 12:
        spell_poison(50, tch, 0, 0, tch, 0);
        break;
      case 13:
        spell_disease(50, tch, NULL, SPELL_TYPE_SPELL, tch, 0);
        break;
      case 14:
        spell_minor_paralysis(50, tch, NULL, 0, tch, 0);
        break;
      case 15:
        spell_nether_touch(50, tch, NULL, SPELL_TYPE_SPELL, tch, 0);
        break;
      case 16:
        spell_dispel_lifeforce(50, tch, NULL, SPELL_TYPE_SPELL, tch, 0);
        break;
      }
    return TRUE;
  }

  return FALSE;

}
