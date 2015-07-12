/*
   ***************************************************************************
   *  File: specs.object.c                                     Part of Duris *
   *  Usage: special procedures for objects                                    *
   *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <list>
using namespace std;

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
#include "damage.h"
#include "reavers.h"
#include "map.h"
#include "handler.h"
#include "ctf.h"
#include "blispells.h"
#include "utility.h"
#include "vnum.obj.h"

/*
   external variables
 */
extern P_event event_type_list[];
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern P_event current_event;
extern char *coin_names[];
extern char *command[];
extern const char *dirs[];
extern const char *race_types[];
extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int innate_abilities[];
extern int planes_room_num[];
extern const int top_of_world;
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct dex_app_type dex_app[52];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern void event_bleedproc(P_char ch, P_char victim, P_obj obj, void *data);
extern const struct racial_data_type racial_data[];
int      do_simple_move_skipping_procs(P_char, int, unsigned int);
extern Skill skills[];

void     bard_healing(int, P_char, P_char, int);
void     bard_protection(int, P_char, P_char, int);
void     bard_storms(int, P_char, P_char, int);
void     bard_chaos(int, P_char, P_char, int);
void     bard_harming(int, P_char, P_char, int);
void     bard_cowardice(int, P_char, P_char, int);
void     bard_calm(int, P_char, P_char, int);
void     bard_dragons(int, P_char, P_char, int);

void event_balance_affects(P_char, P_char, P_obj, void *);
void event_object_proc(P_char, P_char, P_obj, void *);
extern bool has_skin_spell(P_char);
void unmulti(P_char ch, P_obj obj);
int unmulti_altar(P_obj obj, P_char ch, int cmd, char *arg);

// to call from portal obj hooks
int portal_general_internal( P_obj obj, P_char ch, int cmd, char *arg,
                             struct portal_action_messages *msg );

/*static void hummer(P_obj);*/

//  updates the extra description "symbols" on the obj
// to reflect how many charges are left
static void artifact_monolith_update(P_obj obj)
{
  char buf[500];
  if (!obj)
    return;
  if (!(obj->value[0]))
    strcpy(buf, "&+LThe once glowing symbols are dull and faded, making them impossible to read.&n\n");
  else if (1 == obj->value[0])
    strcpy(buf, "&+LThere is a single &n&+rsymbol&+L, written in the ancient language of dragons.&n\n");
  else
    sprintf(buf, "&+LThere appear to be %d different &n&+rsymbols&+L, written in the ancient language\n&+Lof the dragons.&n\n",
            obj->value[0]);

  // now, find the proper extra description, remove the old one (if any) and
  // replace with the new one
  for (struct extra_descr_data *ed = obj->ex_description; ed; ed = ed->next)
    if (!str_cmp(ed->keyword, "symbols"))
    {
      if (ed->description)
        FREE(ed->description);
        ed->description = str_dup(buf);
        obj->str_mask |= STRUNG_EDESC;
    }
}

void dispel_portal(P_char ch, P_obj obj)
{
  P_obj obj2;

  if (GET_LEVEL(ch) < 46)
  {
    act("$p easily resists your assault!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  else if ((GET_LEVEL(ch) < 50) && !number(0, 1))
  {
    act("$p resists your assault!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  // success, search for portal on other side

  obj2 = world[real_room(obj->value[0])].contents;

  while (obj2)
  {
    if (obj2->R_num == obj->R_num && (obj2->value[7] == obj->value[7]) &&
        (obj2 != obj))
      break;

    obj2 = obj2->next_content;
  }

  if (!obj2)
  { // means we scrolled whole content of another side and cannot find portal
    send_to_char("bug in dispelling portals, notify a god.\n", ch);
    return;
  }

  // decay them both
  Decay(obj);
  Decay(obj2);
}

int artifact_monolith(P_obj monolith, P_char ch, int cmd, char *arg)
{
  P_obj obj, next_obj;
  const int HOURS_PER_CHARGE = 2;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;

  // If its not in room, return
  if( !monolith || !OBJ_ROOM(monolith) )
    return FALSE;

  // periodics.. do something fancy
  if (cmd == CMD_PERIODIC)
  {
    // Check for the existence of other of this item in the room.  If they exist, absorb them.
    bool bUpdateDesc = FALSE;

    for( obj = world[monolith->loc.room].contents; obj; obj = next_obj )
    {
      next_obj = obj->next_content;

      if( obj == monolith )
        continue;
      if( obj->R_num == monolith->R_num )
      {
        // Okay, each 'obj' has a charge count stored in val0.  add
        // obj's charges to monolith's charges, and then destroy obj.
        monolith->value[0] += MAX(1, obj->value[0]);
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'
        bUpdateDesc = TRUE;
      }
    }
    if( bUpdateDesc || !(monolith->value[1]) )
    {
      artifact_monolith_update(monolith);
      monolith->value[1] = CMD_SCRATCH;
      return TRUE;
    }
    // Now hum, or crackle or something - only if there are charges left
    if (!number(0,12) && (monolith->value[0]))
    {
      if (number(0,1))
        act("&+YA bolt of energy&n &+Rcrackles &+Yalong the length of $p&n.", TRUE, NULL, monolith, NULL, TO_ROOM);
      else
        act("The &+rsymbols&n on $p &+Wglow&n with &+Rpower&n.", TRUE, NULL, monolith, NULL, TO_ROOM);
      // the symbols on xxx glow with power
    }
    return TRUE;
  }

  if (ch && (cmd == monolith->value[1]))
  {
    char buf[MAX_STRING_LENGTH];
    one_argument(arg, buf);

    if (monolith == get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))
    {
      // check if the person even has any arti's.
      int i, nArtiCount = 0;
      for (i = 0; i < MAX_WEAR; i++)
      {
        if (ch->equipment[i] && IS_ARTIFACT(ch->equipment[i]))
          nArtiCount++;
      }
      // if no arti's, or no charges in the monolith, do nothing
      if (!nArtiCount || !(monolith->value[0]))
      {
        send_to_char("Nothing seems to happen.\n", ch);
      }
      else
      {
        // okay, each "charge" is worth X hours (or X * 3600 seconds).  Divide
        // that between all artifacts EQUALLY (regardless of how full they may
        // already be)
        int nFeedSeconds = (HOURS_PER_CHARGE * 3600) / nArtiCount;
        // now feed up them arti's!
        for (i = 0; i < MAX_WEAR; i++)
        {
          if (ch->equipment[i] && IS_ARTIFACT(ch->equipment[i]))
          {
            obj = ch->equipment[i];
            artifact_feed_sql(ch, ch->equipment[i], nFeedSeconds);
          }
        }
        // update the feeder object to have one less charge..
        monolith->value[0]--;
        artifact_monolith_update(monolith);
      }
      return TRUE;
    }
  }
  return FALSE;

}

int guildwindow(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      old_room, new_room;


  if (cmd == CMD_PERIODIC)
    return (FALSE);

  if (cmd != CMD_LOOK)
    return (FALSE);

  if (!arg || !*arg || str_cmp(arg, " window"))
    return (FALSE);

  if (cmd && (cmd == CMD_LOOK))
  {
    old_room = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, real_room0(obj->value[0]), -1);
    char_from_room(ch);
    char_to_room(ch, old_room, -2);
  }
  return (TRUE);
}

int guildhome(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      curr_time;
  P_char   temp_ch;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!obj)
    return FALSE;

  temp_ch = ch;

  if (argument && (cmd == CMD_SAY))
  {
    arg = argument;

    while (*arg == ' ')
      arg++;

    if (!strcmp(arg, "home"))
    {
      if (!say(ch, arg))
        return TRUE;

      GET_BIRTHPLACE(ch) = world[ch->in_room].number;
      send_to_char("&+CYou feel a warmth which quickly fades away...\n", ch);
      return TRUE;
    }
  }
  return FALSE;
}

int illithid_sack(P_obj obj, P_char ch, int cmd, char *argument)
{
  P_obj    s_obj = NULL;
  char     GBuf1[MAX_STRING_LENGTH], GBuf2[MAX_STRING_LENGTH];

  *GBuf1 = '\0';
  *GBuf2 = '\0';

  if (cmd == CMD_SET_PERIODIC)               /*
                                   Events have priority
                                 */
    return FALSE;

  if (!ch || !obj)              /*
                                   If the player ain't here, why are we?
                                 */
    return FALSE;

  if (argument && cmd == CMD_PUT)
  {
    argument_interpreter(argument, GBuf1, GBuf2);
    if (!*GBuf2)
      return FALSE;
    s_obj = get_obj_in_list_vis(ch, GBuf2, ch->carrying);
    if (!s_obj)
      s_obj = get_obj_in_list_vis(ch, GBuf2, world[ch->in_room].contents);
    if (s_obj != obj)
      return FALSE;

    /* ok they are attempting to get something from this chest */
    if (!IS_ILLITHID(ch) && !IS_PILLITHID(ch)&& !IS_TRUSTED(ch))
    {
      act("&+L$n &+Lis &+Rzapped&+L as $e tries to put something into $p!",
          FALSE, ch, obj, 0, TO_ROOM);
      act("&+LYou are &+Rzapped&+L as you try to put something into $p!",
          FALSE, ch, obj, 0, TO_CHAR);
      return TRUE;
    }

  }
  return FALSE;
}

int artifact_biofeedback(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      curr_time;
  P_char   temp_ch;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !obj )
  {
    return FALSE;
  }

  temp_ch = ch;

  if( !ch )
  {
    if( OBJ_WORN(obj) && obj->loc.wearing )
    {
      temp_ch = obj->loc.wearing;
    }
    else
    {
      return FALSE;
    }
  }


  if( !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( cmd != CMD_PERIODIC && !(cmd / 1000) )
  {
    return FALSE;
  }

  curr_time = time(NULL);

  if( obj->timer[0] + get_property("timer.bioIoun", 80) <= curr_time &&
      !has_skin_spell(temp_ch) )
  {
    spell_biofeedback(25, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
    obj->timer[0] = curr_time;
  }
  return FALSE;
}

/* I have horrbly twisted this function to be called only from
   equip_char, if passed cmd == -1 it activates stone -Zod*/
// This works just fine but I don't see any reference to -1 in the code anywhere?
int artifact_stone(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      curr_time;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;


  if( !OBJ_WORN(obj) || cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  if( !ch )
  {
    if( obj->loc.wearing )
    {
      ch = obj->loc.wearing;
    }
    else
    {
      return FALSE;
    }
  }
  else
  {
    return FALSE;
  }

  curr_time = time(NULL);

  if( !has_skin_spell(ch)
    && obj->timer[0] + (int) get_property("timer.stoneskin.generic", 60) <= curr_time )
  {
    spell_stone_skin(30, ch, 0, SPELL_TYPE_POTION, ch, 0);
    obj->timer[0] = curr_time;
  }

  return FALSE;
}

int artifact_shadow_shield(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      curr_time, i;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC || !OBJ_WORN(obj) )
    return FALSE;

  if( !ch )
  {
    if( obj->loc.wearing )
    {
      ch = obj->loc.wearing;
    }
    else
    {
      return FALSE;
    }
  }

  for( i = 0;i < MAX_WEAR;i++ )
  {
    if( ch->equipment[i] == obj )
    {
      if( i == WEAR_ATTACH_BELT_1 || i == WEAR_ATTACH_BELT_2 || i == WEAR_ATTACH_BELT_3 )
      {
        return FALSE;
      }
    }
  }

  curr_time = time(NULL);

  if( !has_skin_spell(ch)
    && obj->timer[0] + (int) get_property("timer.stoneskin.generic", 60) <= curr_time )
  {
    spell_shadow_shield(30, ch, 0, SPELL_TYPE_POTION, ch, 0);
    obj->timer[0] = curr_time;
  }
  return FALSE;
}

int death_proc(P_obj obj, P_char ch, int cmd, char *argument)
{

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  if (!obj || !ch)
    return FALSE;
  if (IS_PC(ch) && GET_LEVEL(ch) < 57)
  {
    statuslog(AVATAR,
              "%s being destroyed for possessing a god object [%d] %s.",
              GET_NAME(ch), obj_index[obj->R_num].virtual_number,
              obj->short_description);
    act
      ("$p &n&+Lbegins to glow &n&+wbrighter&+L and &+Wbrighter&+L until you are disolved by its divine power!",
       FALSE, ch, obj, 0, TO_CHAR);
    act
      ("$p &n&+Lbegins to glow &n&+wbrighter&+L and &+Wbrighter&+L until $n is disolved by its divine power!",
       FALSE, ch, obj, 0, TO_ROOM);
    die(ch, ch);
    return TRUE;
  }
  return FALSE;
}

int charon_ship(P_obj obj, P_char ch, int cmd, char *argument)
{
  int      curr_time, boat_room = real_room0(1299);
  int      to_room, old_room, dir, dist, spill = 0, look_out = 0;
  int      galleon_route[] = { 213121, 213221, 213321, 213421, 213521,
    213621, 213721, 213821, 213921, 213879,
    213880, 213881, 213882, 213982, 214082,
    214182, 214282, -1
  };
  P_char   tch, next_tch;
  P_obj    tobj, next_tobj;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }
  if( cmd != CMD_PERIODIC || !obj )
  {
    return FALSE;
  }

  if( !(obj->timer[0]) )
  {
    obj->timer[0] = time(NULL);
  }

  if( OBJ_ROOM(obj) )
  {
    curr_time = time(NULL);
    switch( obj->timer[1] )
    {
    // Beginning state, docked.
    case 0:
      // If 1 minute has passed..
      if( curr_time > obj->timer[0] + (1 * 60) )
      {
        obj->timer[0] = time(NULL);
        obj->timer[1] = 1;
        send_to_room("&+LA spectral galleon hoists its anchor, preparing to depart.\n", obj->loc.room);
      }
      break;
    // Preparing to sail
    case 1:
      // If 30 sec have passed..
      if( curr_time > obj->timer[0] + (30) )
      {
        obj->timer[0] = time(NULL);
        if (obj->timer[2])
        {
          obj->timer[1] = 3;
        }
        else
        {
          obj->timer[1] = 2;
        }
      }
      break;
    // Embark on journey, status sailing forward
    case 2:
      if( galleon_route[obj->timer[2] + 1] == -1 )
      {
        obj->timer[1] = 4;
        obj->timer[0] = time(NULL);
        send_to_room("&+LA spectral galleon drops its anchor.\n", obj->loc.room);
        spill = 1;
      }
      else if( (to_room = real_room(galleon_route[++(obj->timer[2])])) )
      {
        send_to_room("&+LA spectral galleon sails onward.\n", obj->loc.room);
        obj_from_room(obj);
        obj_to_room(obj, to_room);
        send_to_room("&+LA spectral galleon arrives.\n", obj->loc.room);
        look_out = 1;
      }
      break;
    // sailing backward
    case 3:
      if( !(obj->timer[2]) )
      {
        obj->timer[1] = 4;
        obj->timer[0] = time(NULL);
        send_to_room("&+LA spectral galleon drops its anchor.\n", obj->loc.room);
        spill = 1;
      }
      else if( (to_room = real_room(galleon_route[--(obj->timer[2])])) )
      {
        send_to_room("&+LA spectral galleon sails onward.\n", obj->loc.room);
        obj_from_room(obj);
        obj_to_room(obj, to_room);
        send_to_room("&+LA spectral galleon arrives.\n", obj->loc.room);
        look_out = 1;
      }
      break;
    // docking
    case 4:
      if( curr_time > obj->timer[0] + (30) )
      {
        obj->timer[0] = time(NULL);
        obj->timer[1] = 0;
      }
      break;
    }

    if( spill && boat_room )
    {
      send_to_room("&+LA large globe of blackness engulfs the entire room...\n", boat_room );
      send_to_room("&+LA black mist pours out of the galleon!&n\n", obj->loc.room );
      for( tch = world[boat_room].people; tch; tch = next_tch )
      {
        next_tch = tch->next_in_room;
        char_from_room(tch);
        char_to_room(tch, obj->loc.room, -2);
        if( isname(GET_NAME(tch), "charon") )
        {
          do_action(ch, 0, CMD_GRIN);
        }
        send_to_char("&+w ...light slowly begins &+Wto form... and you are elsewhere!\n", tch);
      }
      for( tobj = world[boat_room].contents; tobj; tobj = next_tobj )
      {
        next_tobj = tobj->next_content;
        obj_from_room(tobj);
        obj_to_room(tobj, obj->loc.room);
      }
    }
    if( look_out )
    {
      for (tch = world[boat_room].people; tch; tch = next_tch)
      {
        next_tch = tch->next_in_room;

        if (IS_NPC(tch))
        {
          continue;
        }

        old_room = tch->in_room;
        char_from_room(tch);
        char_to_room(tch, obj->loc.room, -1);
        char_from_room(tch);
        char_to_room(tch, old_room, -2);
      }
    }
  }
}

// pathfinder from KT
int pathfinder(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      curr_time;


  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !obj || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  // Any powers activated by keywords? Right here, bud.
  if( argument && (cmd == CMD_SAY) )
  {
    arg = argument;

    while (*arg == ' ')
    {
      arg++;
    }

    if (!strcmp(arg, "path"))
    {
      if( !say(ch, arg) )
      {
        return TRUE;
      }
      curr_time = time(NULL);

      // Every 800 sec.
      if( obj->timer[0] + 800 <= curr_time )
      {
        act("Your $q &+ghums&n briefly and points you in a direction.", FALSE, ch, obj, obj, TO_CHAR);
        act("$n's $q &+ghums&n briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_pass_without_trace(30, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = curr_time;
      }
      return TRUE;
    }
  }
  return FALSE;
}


int artifact_hide(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int curr_time, room;


  if (cmd == CMD_SET_PERIODIC)               /*
                                   Events have priority
                                 */
    return FALSE;

  if (!ch || !obj)              /*
                                   If the player ain't here, why are we?
                                 */
    return FALSE;

  if (!OBJ_WORN(obj))           /*
                                   Most things don't work in a sack...
                                 */
    return FALSE;

/*
   Any powers activated by keywords? Right here, bud.
 */

  if (argument && (cmd == CMD_SAY))
  {
    arg = argument;

    while (*arg == ' ')
      arg++;

    if (!strcmp(arg, "hide"))
    {
      if (!say(ch, arg))
        return TRUE;

      curr_time = time(NULL);

      if (obj->timer[0] + 60 <= curr_time)
      {
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);

        if(!(room = ch->in_room))
          return true;
        
        if(IS_WATER_ROOM(room) ||
          world[room].sector_type == SECT_OCEAN)
        {
          send_to_char("It's very &+Bwet&n here; too &+Bwet&n to hide behind anything.\r\n", ch);
          return true;
        }
        else
        {
          SET_BIT(ch->specials.affected_by, AFF_HIDE);
        }
        
        obj->timer[0] = curr_time;
      }
      return TRUE;
    }
  }
  return FALSE;
}

int artifact_invisible(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      curr_time;


  if (cmd == CMD_SET_PERIODIC)               /*
                                   Events have priority
                                 */
    return FALSE;

  if (!ch || !obj)              /*
                                   If the player ain't here, why are we?
                                 */
    return FALSE;

  if (!OBJ_WORN(obj))           /*
                                   Most things don't work in a sack...
                                 */
    return FALSE;

/*
   Any powers activated by keywords? Right here, bud.
 */

  if (argument && (cmd == CMD_SAY))
  {
    arg = argument;

    while (*arg == ' ')
      arg++;

    if (!strcmp(arg, "invisible"))
    {
      if (!say(ch, arg))
        return TRUE;

      curr_time = time(NULL);

      if (obj->timer[0] + 60 <= curr_time)
      {
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_improved_invisibility(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;
      }

      return TRUE;
    }
  }

  return FALSE;
}

int transp_tow_misty_gloves(P_obj obj, P_char ch, int cmd, char *argument)
{
  char     e_pos;
  int      curr_time;


  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC )
  {
    if( OBJ_WORN(obj) && (ch = obj->loc.wearing) )
    {
      if( !IS_ALIVE(ch) )
      {
        return FALSE;
      }
      curr_time = time(NULL);
      // Every 30 sec.
      if( obj->timer[0] + 30 <= curr_time && !has_skin_spell(ch) )
      {
        spell_stone_skin(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = curr_time;
        return TRUE;
      }
    }

    hummer(obj);
    return TRUE;
  }

  // If ch not wearing obj on hands.
  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || obj->loc.wearing->equipment[WEAR_HANDS] != obj )
  {
    return FALSE;
  }

  if( argument && (cmd == CMD_RUB) )
  {
    curr_time = time(NULL);

    act("You rub your gloved hands together.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n rubs $s gloved hands together.", FALSE, ch, 0, 0, TO_ROOM);
    // 2 min timer.
    if (obj->timer[1] + 120 <= curr_time)
    {
      act("Your hands tingle for a brief moment.", FALSE, ch, obj, obj, TO_CHAR);

      spell_improved_invisibility(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      SET_BIT(ch->specials.affected_by, AFF_HIDE);

      obj->timer[1] = curr_time;
      return TRUE;
    }
  }

  return FALSE;
}


/*void hummer (P_obj obj)
{
P_char t_ch;
if (!obj || number(0,9))
   return;

//Kvark if (IS_AFFECTED(ch, AFF_HIDE)
 if(OBJ_WORN(obj)){
      t_ch = obj->loc.wearing;
      if(t_ch)
         if (IS_AFFECTED(t_ch, AFF_HIDE)){
        return;
         }
  }
  if (OBJ_WORN(obj) || OBJ_CARRIED(obj)) {
    act("&+LA faint &n&+rhum&N&+L can be heard from&N $p &+Lcarried by $n&N.",
       FALSE, obj->loc.wearing, obj, 0 ,TO_ROOM);
    act("&+LA faint &n&+rhum&N&+L can be heard from&N $p&+L you are carrying.",
       FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
  }
}*/
#define FLT_TOROOM(x, y) (world[(x)].dir_option[(y)]->to_room)

int magic_mouth(P_obj obj, P_char ch, int cmd, char *arg)
{

  P_desc   i;
  char     buff[MAX_STRING_LENGTH];

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return FALSE;

  if (IS_TRUSTED(ch))
    return FALSE;

  if ((obj->value[0] != GET_A_NUM(ch)) &&       /* not in guild */
      (!number(0, 4)))          /* do only occasionally */
  {
    sprintf(buff,
            "&+cA magic mouth tells your guild 'Alert!  $N&n&+c has trespassed into %s&n&+c!'&N",
            world[ch->in_room].name);
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected &&
          !is_silent(i->character, TRUE) &&
          IS_SET(i->character->specials.act, PLR_GCC) &&
          IS_MEMBER(GET_A_BITS(i->character)) &&
          (GET_A_NUM(i->character) == obj->value[0]) &&
          !IS_TRUSTED(i->character))
        act(buff, FALSE, i->character, 0, ch, TO_CHAR);
  }
  return FALSE;
}

int floating_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      num_choices = 0, i;
  int      pos_dirs[NUM_EXITS];
  int      my_room;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd)
    return magic_pool(obj, ch, cmd, arg);

  /*
     okay... I will move with a 75% chance.
   */
  if (number(1, 100) > 75)
    return FALSE;

  my_room = obj->loc.room;

  for (i = 0; i < NUM_EXITS; i++)
    if ((world[my_room].dir_option[i]) &&
        (FLT_TOROOM(my_room, i) != NOWHERE) &&
        (!IS_SET(world[(my_room)].dir_option[(i)]->exit_info,
                 EX_CLOSED)) &&
        (!IS_SET(world[(my_room)].dir_option[(i)]->exit_info,
                 EX_SECRET)) &&
        (!IS_SET(world[(my_room)].dir_option[(i)]->exit_info,
                 EX_BLOCKED)) &&
        (!IS_SET(world[FLT_TOROOM(my_room, i)].room_flags, NO_MOB)))
      pos_dirs[num_choices++] = i;

  if (!num_choices)
    return FALSE;

  /*
     okay.. I'm going to move... lets do it
   */

  act("$p floats away.", TRUE, NULL, obj, NULL, TO_ROOM);
  obj_from_room(obj);
  obj_to_room(obj, FLT_TOROOM(my_room, pos_dirs[number(0, num_choices - 1)]));
  act("$p floats in.", TRUE, NULL, obj, NULL, TO_ROOM);
  return TRUE;

}

#undef FLT_TOROOM

int illithid_teleport_veil(P_obj obj, P_char ch, int cmd, char *arg)
{

  int max_room = 24;      /* match the array below - 1 */
  int r_room = -1;
  int to_room[] =
  { 96563, 96563, 11545, 15273, 96900,
    36171, 96563,  4109,  4437,  4109,
    96803, 17589, 17607, 96909,  3404,
    23812, 23805, 12535, 12528, 25458,
    25459, 25458, 12535, 12536, 12540
  };

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_ENTER || !obj || !IS_ALIVE(ch) || !arg || (str_cmp(" veil", arg)) )
  {
    return FALSE;
  }

  act("$p &+Wsuddenly glows brightly!", FALSE, ch, obj, 0, TO_ROOM);
  act("$n steps through, and is gone!", FALSE, ch, obj, 0, TO_ROOM);
  char_from_room(ch);
  while( r_room == -1 )
  {
    r_room = real_room(to_room[number(0, max_room)]);
  }

  act("You enter $p and reappear elsewhere...", FALSE, ch, obj, 0, TO_CHAR);
  char_to_room(ch, r_room, -1);
  do_restore(ch, GET_NAME(ch), -4);
  act("A crackle of energy is felt, and $n appears.", FALSE, ch, 0, 0, TO_ROOM);
  CharWait(ch, 3);

  return TRUE;
}

/*
   This is for a random teleporting pool in a zone -DR
 */
int teleporting_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      to_room;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  // Check to see if someone is trying to enter the pool
  if( cmd == CMD_ENTER )
  {
    return magic_pool(obj, ch, cmd, arg);
  }

  if( !ch )
  {

    /*
       Random chance of 6%-first number that came to mind ;) of teleporting.
       This might be a bit much, change if needed.
     */
    if( number(1, 250) > 15 )
    {
      return FALSE;
    }

    // Randomly pick a room in the zone
    do
    {
      to_room = (number(zone_table[world[obj->loc.room].zone].real_bottom,
                        zone_table[world[obj->loc.room].zone].real_top));
    }
    while (IS_SET(world[to_room].room_flags, PRIVATE) ||
           IS_SET(world[to_room].room_flags, NO_MOB));

    act("$p &+Lslowly vanishes away into nothingness.&N", TRUE, NULL, obj, NULL, TO_ROOM);
    obj_from_room(obj);
    obj_to_room(obj, to_room);
    act("$p &+Lslowly forms in front of you.&N", TRUE, NULL, obj, NULL, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}


int teleporting_map_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
  int to_room;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_ENTER )
  {
    return magic_map_pool(obj, ch, cmd, arg);
  }

  if( !ch )
  {
    /*
       Random chance of 6%-first number that came to mind ;) of teleporting.
       This might be a bit much, change if needed.
     */
    if( number(1, 250) > 15 )
    {
      return FALSE;
    }

    // Randomly pick a room in the zone
    do
    {
      to_room = (number(zone_table[world[obj->loc.room].zone].real_bottom,
        zone_table[world[obj->loc.room].zone].real_top));
    }
    while( IS_SET(world[to_room].room_flags, PRIVATE)
      || IS_SET(world[to_room].room_flags, NO_MOB) );

    act("$p &+Lslowly vanishes away into nothingness.&N", TRUE, NULL, obj, NULL, TO_ROOM);
    obj_from_room(obj);
    obj_to_room(obj, to_room);
    act("$p &+Lslowly forms in front of you.&N", TRUE, NULL, obj, NULL, TO_ROOM);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

int ring_elemental_control(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim, next_per;
  int      pos;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_RUB || !arg || !OBJ_WORN(obj) || obj->R_num != real_object(25080) )
  {
    return FALSE;
  }

  if( !*arg )
  {
    send_to_char("Rub what?!\n", ch);
    return TRUE;
  }
  half_chop(arg, Gbuf1, Gbuf2);

  if( !strstr(obj->name, Gbuf1) )
  {
    return FALSE;
  }

  if (IS_SET(world[ch->in_room].room_flags, NO_MAGIC))
  {
    send_to_char("That doesn't seem to do anything!\n", ch);
    return FALSE;
  }

  if( *Gbuf2 )
  {
    victim = get_char_room_vis(ch, Gbuf2);
    if( !victim )
    {
      send_to_char("No one by that name here.\n", ch);
      return TRUE;
    }
    if( !strstr(victim->player.name, "elemental") )
    {
      send_to_char("You can't charm non-elementals!\n", ch);
      return TRUE;
    }
  }
  else
  {
    for( victim = world[ch->in_room].people; victim; victim = next_per )
    {
      next_per = victim->next_in_room;

      if( strstr(victim->player.name, "elemental") && !GET_MASTER(victim) && (victim->player.level <= 55) && IS_NPC(victim) )
      {
        break;
      }
    }
  }

  if( !victim )
  {
    send_to_char("There aren't any elementals to charm.\n", ch);
    return TRUE;
  }
  if( !GET_MASTER(victim) && !GET_MASTER(ch) )
  {
    if (circle_follow(victim, ch))
    {
      send_to_char("Sorry, following in circles can not be allowed.\n", ch);
      return TRUE;
    }
    act("$n rubs $p.", TRUE, ch, obj, 0, TO_ROOM);
    act("You rub $p.", TRUE, ch, obj, 0, TO_CHAR);

    if (obj->value[2] > 0)
    {
      if (!--obj->value[2])
      {
        act("$p in $n's hands shatters and the pieces disappear in smoke.", TRUE, ch, obj, 0, TO_ROOM);
        act("$p in your hands shatters and the pieces disappear in smoke.", TRUE, ch, obj, 0, TO_CHAR);

        if( OBJ_WORN(obj) )
        {
          for( pos = 0; pos < MAX_WEAR; pos++ )
          {
            if( obj->loc.wearing->equipment[pos] == obj )
            {
              unequip_char(obj->loc.wearing, pos);
              break;
            }
          }
        }
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'
        obj = NULL;
      }
    }
    if( IS_NPC(victim) && IS_SET(victim->specials.act, ACT_BREAK_CHARM) )
    {
      send_to_char("This creature's will is too strong to be charmed.\n", ch);
      return TRUE;
    }
    if (victim->following)
    {
      stop_follower(victim);
    }

    add_follower(victim, ch);
    setup_pet(victim, ch, 24 * 16, 0);
    act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
  }
  return TRUE;
}

int jailtally(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_obj    t_obj;
  P_char   k;
  int      room;
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return (FALSE);

  if (((cmd == CMD_READ) || (cmd == CMD_LOOK)) && ch &&
      MIN_POS(ch, POS_KNEELING + STAT_RESTING))
  {
    while (isspace(*arg))
      arg++;
    t_obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents);
    if (t_obj == obj)
    {
      room = real_room(hometowns[CHAR_IN_TOWN(ch) - 1].jail_room);
      if (!world[room].people)
        send_to_char("Nobody in jail for now!\n", ch);
      else
      {
        send_to_char("The tally sheet lists the following inmates:\n", ch);
        for (k = world[room].people; k; k = k->next_in_room)
        {
          sprintf(Gbuf1, "%s\n", J_NAME(k));
          send_to_char(Gbuf1, ch);
        }
      }
      return (TRUE);
    }
  }
  return (FALSE);
}

// Allows holder to use decline/accept/ptell.
int trustee_artifact(P_obj obj, P_char ch, int cmd, char *arg)
{
  int cmd_list[] = { CMD_DECLINE, CMD_ACCEPT, CMD_PTELL, 0 }, i;

  if( !IS_ALIVE(ch) || IS_NPC(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  // Ok, chars trustee.
  for( i = 0; cmd_list[i]; i++ )
  {
    if( cmd_list[i] == cmd )
    {
      break;
    }
  }
  if( !cmd_list[i] )
    return FALSE;

  // Ok, we be cooking with gas now. We have proper cmd..
  (*cmd_info[cmd].command_pointer) (ch, arg, cmd);
  return TRUE;
}

int check_trap_trigger(P_char ch, int when)
{
  return FALSE;
}

int trap_timer(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_obj    trap;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!current_event || !ch)
    return FALSE;
  else if ((trap = read_object(obj->value[2], VIRTUAL)))
  {
    obj_to_room(trap, ch->in_room);
    return TRUE;
  }
  return FALSE;                 /*
                                   hope we didnt get here, but oh well
                                 */
}
int creeping_doom(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   t;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  /*
     if argument is prefixed with spaces, skip over them
   */
  while (*arg == ' ')
    ++arg;

  switch (cmd)
  {
  case CMD_KILL:
  case CMD_HIT:
    if (!str_cmp("creeping doom", arg) || !str_cmp("creeping", arg) ||
        !str_cmp("doom", arg))
    {
      send_to_char
        ("Your swing seems to have no affect on the shapeless mass.\n", ch);
      act
        ("$n attacks the creeping doom, but it's everywhere and apparently is unaffected.",
         FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    break;
  case CMD_CAST:
    /*
       skip over "'<spell name>' "
     */
    if (rindex(arg, '\'') == NULL)
      return FALSE;

    /*
       skip over spaces between spell incantation and target name
     */
    for (++arg; *arg && (*arg == ' '); arg++) ;

    if (!str_cmp("creeping doom", arg) || !str_cmp("creeping", arg) ||
        !str_cmp("doom", arg))
    {
      /*
         use magic resistance of 100% when imp'd
       */
      send_to_char("You can't.\n", ch);
      return TRUE;
    }
    break;
  case CMD_BACKSTAB:
    if (!str_cmp("creeping doom", arg) || !str_cmp("creeping", arg) ||
        !str_cmp("doom", arg))
    {
      send_to_char
        ("You hopelessly attempt to backstab the shapeless creeping doom.\n",
         ch);
      act("$n hopelessly tries to backstab the creeping doom.", FALSE, ch, 0,
          0, TO_ROOM);
      return TRUE;
    }
    break;
  case CMD_KICK:
    if (!str_cmp("creeping doom", arg) || !str_cmp("creeping", arg) ||
        !str_cmp("doom", arg))
    {
      send_to_char
        ("You kick the creeping doom, but with no apparent affect.\n", ch);
      act("$n kicks the creeping doom, but it has no affect.", FALSE, ch, 0,
          0, TO_ROOM);
      return TRUE;
    }
    break;
  case CMD_HITALL:
    LOOP_THRU_PEOPLE(t, ch) if (IS_NPC(t) && (GET_RNUM(t) == 9))
    {                           /*
                                   creeping dooms virtual #
                                 */
    }
    break;
  default:
    return FALSE;
    break;
  }                             /*
                                   switch
                                 */

  return FALSE;
}

/*
   use object with ITEM_SWITCH type:
   value[0] = command to trigger
   value[1] = room containing the blocked exit
   value[2] = direction of the exit in the room
   value[3] = 0:wall moves, 1:switch item moves
 */

int item_switch(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      door, in_room, bits, back;
  P_char   dummy;
  P_obj    object;
  char     buf[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !arg || !obj )
  {
    return FALSE;
  }

  // Tracks are never a switch.
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &dummy, &object);
  if( obj != object )
  {
    return FALSE;
  }

  if( obj->type != ITEM_SWITCH || obj->value[0] != cmd )
  {
    return FALSE;
  }

  in_room = real_room(obj->value[1]);
  if( in_room < 0 )
  {
    send_to_char("This item is broken.  Talk to a god!\n", ch);
    wizlog( MINLVLIMMORTAL, "item_switch: The switch '%s' (%d) is broken!", obj->short_description, GET_OBJ_VNUM(obj) );
    return TRUE;
  }
  door = obj->value[2];
  if( door < 0 || door >= NUM_EXITS )
  {
    send_to_char("This item is broken (exit # out of range).  Talk to a god!\n", ch);
    wizlog( MINLVLIMMORTAL, "item_switch: The switch '%s' (%d) has out of range exit %d!", obj->short_description, GET_OBJ_VNUM(obj), door );
    return TRUE;
  }
  if (!world[in_room].dir_option[door])
  {
    send_to_char("This item is broken (exit doesn't exist).  Talk to a god!\n", ch);
    wizlog( MINLVLIMMORTAL, "item_switch: The switch '%s' (%d) has exit %d which doesn't exit!", obj->short_description, GET_OBJ_VNUM(obj), door );
    return TRUE;
  }
  if( !IS_SET(world[in_room].dir_option[door]->exit_info, EX_BLOCKED) )
  {
    send_to_char("Nothing happens.\n", ch);
    return TRUE;
  }
  if( OBJ_ROOM(obj) )
  {
    if( obj->loc.room != in_room )
    {
      send_to_char("You hear a rumbling sound in the distance.\n", ch);
      act("You hear a rumbling sound in the distance.", FALSE, ch, 0, 0, TO_ROOM);
    }
  }
  else if( OBJ_CARRIED_BY(obj, ch) || OBJ_WORN_BY(obj, ch) )
  {
    if( ch->in_room != in_room )
    {
      send_to_char("Nothing happens.\n", ch);
      return TRUE;
    }
  }
  else
  {
    return FALSE;
  }
  REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED);
  back = world[in_room].dir_option[door]->to_room;

  if( IS_SET(world[in_room].dir_option[door]->exit_info, EX_SECRET) )
  {
    if( obj->value[3] == 1 )
    {
      sprintf(buf, "%s moves aside, revealing a wall behind.\n", obj->short_description);
      CAP(buf);
      send_to_room(buf, in_room);
    }
    else
    {
      if( door == DOWN )
      {
        strcpy(buf, "Part of the floor seems to be moving.\n");
      }
      else if( door == UP )
      {
        strcpy(buf, "Part of the ceiling seems to be moving.\n");
      }
      else
      {
        sprintf(buf, "The %s wall seems to be moving.\n", dirs[door]);
      }
      send_to_room(buf, in_room);
    }
  }
  else
  {
    if( back == in_room )
    {
      REMOVE_BIT(world[back].dir_option[(int) rev_dir[door]]->exit_info, EX_BLOCKED);
    }

    if( obj->value[3] == 1 )
    {
      sprintf(buf, "%s moves aside, revealing a passageway.\n", obj->short_description);
      CAP(buf);
      send_to_room(buf, in_room);
    }
    else
    {
      if( door == DOWN )
      {
        strcpy(buf, "Part of the floor moves aside, revealing a passageway.\n");
      }
      else if (door == UP)
      {
        strcpy(buf, "Part of the ceiling moves aside, revealing a passageway.\n");
      }
      else
      {
        sprintf(buf, "The %s wall moves aside, revealing a passageway.\n", dirs[door]);
      }
      send_to_room(buf, in_room);
    }
  }
  return TRUE;
}


/*
   Use ITEM_OTHER:
   value[0] = payback - #/100
   value[1] = amount of money this machine has made from player
   value[2] = number of times machine is played
   value[3] = amount of money this machine has payed off
   value[4] = # Same Goodie Payoffs
   value[5] = # Same Evil Payoffs
   value[6] = # Same Undead Payoffs
   value[7] = # Tiamat Payoffs

   this is now based on real slots,  payoffs are as follows

   Odds = (#of things on wheel/# of occurances)^3 (for all same)
    i.e. 100 objects on wheel/1 tiamat on wheel = 100^3 or 1:1,000,000 odds of 3 tiamats
   Odds = (#of things on wheel1/# of occurances)*(same for wheel 2, and wheel3) (for all same)
   Chance = 100/odds (gives a %)
   Payback = (Payoff * Chance) / 100
   Payoff = (Payback * 100) / Chance

   9 Goodies (4 each)
   7 Evils (3 each)
   6 Undead (2 each)

                   %Chance        payoff    odds        payback
   Any Goodie     36.0%                 x        1:      2.7    0.
   Any Evil       21.25%                x        1:      4.76   0.
   Any Undead     12.0%                 x        1:      8.33   0.
   Any Illithid   10.0%           x    1:     10      0.

   Any 3 Goodies   4.6656%          x2   1:     21.433  0.093
   Any 3 Evils     0.9261%    x5     1:    107.979  0.092
   All Blank       0.8%                 x10      1:    125      0.120
   Any 3 Undead    0.1728%          x25    1:    578.705  0.086
   All Illithid    0.1%                 x50      1:   1000      0.10
   All Same Goodie 0.0064%        x250     1:  15625      0.016
   All Same Evil   0.0027%    x500     1:  37037.037  0.013
   All Same Undead 0.0008%            x2500    1: 125000      0.02
   All Tiamat      0.0001%      x25000   1:1000000      0.025
    Totals         6.6745%                                      0.528

   If you don't understand this table, what it boils down to is approx 7% of all pulls
   will win something.  Over time the machine eats 52.8% on average of what is put into it.

 */

int slot_machine(P_obj obj, P_char ch, int cmd, char *arg)
{
  const char *name[] = {
    "[&+L     TIAMAT    &N]",   //0
    "[&+M    ILLITHID   &N]",   //1
    "[&+M    ILLITHID   &N]",   //2
    "[&+M    ILLITHID   &N]",   //3
    "[&+M    ILLITHID   &N]",   //4
    "[&+M    ILLITHID   &N]",   //5
    "[&+M    ILLITHID   &N]",   //6
    "[&+M    ILLITHID   &N]",   //7
    "[&+M    ILLITHID   &N]",   //8
    "[&+M    ILLITHID   &N]",   //9
    "[&+M    ILLITHID   &N]",   //10
    "[&+C     HUMAN     &N]",   //11
    "[&+C     HUMAN     &N]",   //12
    "[&+C     HUMAN     &N]",   //13
    "[&+C     HUMAN     &N]",   //14
    "[&+R     GNOME     &N]",   //15
    "[&+R     GNOME     &N]",   //16
    "[&+R     GNOME     &N]",   //17
    "[&+R     GNOME     &N]",   //18
    "[&+B   BARBARIAN   &N]",   //19
    "[&+B   BARBARIAN   &N]",   //20
    "[&+B   BARBARIAN   &N]",   //21
    "[&+B   BARBARIAN   &N]",   //22
    "[&+Y     DWARF     &N]",   //23
    "[&+Y     DWARF     &N]",   //24
    "[&+Y     DWARF     &N]",   //25
    "[&+Y     DWARF     &N]",   //26
    "[&+y    HALFLING   &N]",   //27
    "[&+y    HALFLING   &N]",   //28
    "[&+y    HALFLING   &N]",   //29
    "[&+y    HALFLING   &N]",   //30
    "[&+W  S&+wT&+WO&+wR&+WM &+wG&+WI&+wA&+WNT  &N]",   //31
    "[&+W  S&+wT&+WO&+wR&+WM &+wG&+WI&+wA&+WNT  &N]",   //32
    "[&+W  S&+wT&+WO&+wR&+WM &+wG&+WI&+wA&+WNT  &N]",   //33
    "[&+W  S&+wT&+WO&+wR&+WM &+wG&+WI&+wA&+WNT  &N]",   //34
    "[&+g    CEN&+LTAUR    &N]",        //35
    "[&+g    CEN&+LTAUR    &N]",        //36
    "[&+g    CEN&+LTAUR    &N]",        //37
    "[&+g    CEN&+LTAUR    &N]",        //38
    "[&+C    HALF-&+cELF   &N]",        //39
    "[&+C    HALF-&+cELF   &N]",        //40
    "[&+C    HALF-&+cELF   &N]",        //41
    "[&+C    HALF-&+cELF   &N]",        //42
    "[&+c    GREY ELF   &N]",   //43
    "[&+c    GREY ELF   &N]",   //44
    "[&+c    GREY ELF   &N]",   //45
    "[&+c    GREY ELF   &N]",   //46
    "[&+m    DROW ELF   &N]",   //47
    "[&+m    DROW ELF   &N]",   //48
    "[&+m    DROW ELF   &N]",   //49
    "[&+G   GITH&+WYANKI   &N]",        //50
    "[&+G   GITH&+WYANKI   &N]",        //51
    "[&+G   GITH&+WYANKI   &N]",        //52
    "[&+g     TROLL     &N]",   //53
    "[&+g     TROLL     &N]",   //54
    "[&+g     TROLL     &N]",   //55
    "[&+L      ORC      &N]",   //56
    "[&+L      ORC      &N]",   //57
    "[&+L      ORC      &N]",   //58
    "[&+r    DUERGAR    &N]",   //59
    "[&+r    DUERGAR    &N]",   //60
    "[&+r    DUERGAR    &N]",   //51
    "[&+G     GOBLIN    &N]",   //52
    "[&+G     GOBLIN    &N]",   //63
    "[&+G     GOBLIN    &N]",   //64
    "[&+b      OGRE     &N]",   //65
    "[&+b      OGRE     &N]",   //66
    "[&+b      OGRE     &N]",   //67
    "[&+L      L&+mIC&+LH     &N]",     //68
    "[&+L      L&+mIC&+LH     &N]",     //69
    "[&+R    VAM&+rPI&+RRE    &N]",     //70
    "[&+R    VAM&+rPI&+RRE    &N]",     //71
    "[&+R     W&+rI&+RG&+rH&+RT     &N]",       //72
    "[&+R     W&+rI&+RG&+rH&+RT     &N]",       //73
    "[&+L  DEATH &+bKNIGHT &N]",        //74
    "[&+L  DEATH &+bKNIGHT &N]",        //75
    "[&+W    PHA&+LNTOM    &N]",        //76
    "[&+W    PHA&+LNTOM    &N]",        //77
    "[&+L SHADOW &+rBEAST  &N]",        //78
    "[&+L SHADOW &+rBEAST  &N]",        //79
    "[&+L     BLANK     &N]",   //80
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",   //85
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",   //90
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",   //95
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",
    "[&+L     BLANK     &N]",   //99
    "[&+L     BLANK     &N]",   //100
    "[&+L     BLANK     &N]",   //
    "[&+L     BLANK     &N]",   //
    "[&+L     BLANK     &N]",   //
    "[&+L     BLANK     &N]",   //
    "[&+L     BLANK     &N]",   //105
    "[&+L     BLANK     &N]"    //
    "[&+L     BLANK     &N]"  //
    "[&+L     BLANK     &N]"  //
    "[&+L     BLANK     &N]"  //109
  };

  int      bits, coins, type, wheela, wheelb, wheelc, greywheel, count;
  P_char   dummy;
  P_obj    object;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH],
    Gbuf3[MAX_STRING_LENGTH];
  int      gpayoff, epayoff, upayoff, tpayoff, ipayoff, gggpayoff, eeepayoff,
    uuupayoff, blankpayoff;
  int      coinamt;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;


  if (cmd != CMD_PUT)
    return FALSE;

  //disabled until winning millions of plats is removed
  //send_to_char("Slot machines machines have been disabled due to excessive taxes.\r\n", ch);
  //return FALSE;


  arg = one_argument(arg, Gbuf1);       //multicoin
  arg = one_argument(arg, Gbuf2);       //multicoin
  arg = one_argument(arg, Gbuf3);       //multicoin
//  argument_interpreter(arg, Gbuf1, Gbuf2);
  if (!Gbuf1 || !Gbuf2 || !Gbuf3)
    return FALSE;

  type = coin_type(Gbuf2);
  coinamt = atoi(Gbuf1);
  // Don't include tracks with slot machine.
  bits = generic_find(Gbuf3, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &dummy, &object);

//  wizlog(MINLVLIMMORTAL, "%s played %s %s on %s in [%d]", GET_NAME(ch),
//                              Gbuf1, Gbuf2, Gbuf3, world[ch->in_room].number);
// wizlog(MINLVLIMMORTAL, "%s played %d %s on %s in [%d]", GET_NAME(ch),
//                              coinamt, Gbuf2, Gbuf3,world[ch->in_room].number);

  if ((type < 0) || (type > 3))
    return FALSE;

  if (coinamt < 0)
  {
    wizlog(MINLVLIMMORTAL,
           "&=LR%s just tried to play %d %s on a slot machine in [%d]&N",
           GET_NAME(ch), coinamt, Gbuf2, world[ch->in_room].number);
    return FALSE;
  }
  if (coinamt == 0)
    return FALSE;

  if ((coinamt > 5) && !IS_TRUSTED(ch))
  {
    act("&NYou cannot put that much in!&N", FALSE, ch, 0, 0, TO_CHAR);
    return TRUE;
  }

//  bits = generic_find(Gbuf2, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &dummy, &object);

  if (obj != object)
    return FALSE;

//  if (((type == 0) && (GET_COPPER(ch) == 0)) ||
//      ((type == 1) && (GET_SILVER(ch) == 0)) ||
//      ((type == 2) && (GET_GOLD(ch) == 0)) ||
//      ((type == 3) && (GET_PLATINUM(ch) == 0)))
  if (((type == 0) && (GET_COPPER(ch) < coinamt)) ||
      ((type == 1) && (GET_SILVER(ch) < coinamt)) ||
      ((type == 2) && (GET_GOLD(ch) < coinamt)) ||
      ((type == 3) && (GET_PLATINUM(ch) < coinamt)))
  {
    send_to_char("You don't have enough of that coin type.\n", ch);
    return TRUE;
  }
  else
  {
    act("You insert your coin(s) into $p.", FALSE, ch, obj, 0, TO_CHAR);
    switch (type)
    {
    case 0:
      GET_COPPER(ch) -= coinamt;
      obj->value[1] += coinamt;
      break;
    case 1:
      GET_SILVER(ch) -= coinamt;
      obj->value[1] += (10 * coinamt);
      break;
    case 2:
      GET_GOLD(ch) -= coinamt;
      obj->value[1] += (100 * coinamt);
      break;
    case 3:
      GET_PLATINUM(ch) -= coinamt;
      obj->value[1] += (1000 * coinamt);
      break;
    }
    if ((obj->value[2]) == 0)
      (obj->value[0] = 0);
    obj->value[2]++;
  }
  wheela = number(0, 79);
  wheelb = number(0, 79);
  wheelc = number(0, 79);
//    wheela = 39;  //these force elftime :P
//    wheelb = 43;
//    wheelc = 48;

  sprintf(Gbuf1, "You pull the lever and see: %s %s %s\n", name[wheela],
          name[wheelb], name[wheelc]);
  send_to_char(Gbuf1, ch);
  coins = 0;
  gggpayoff = 2 * coinamt;
  eeepayoff = 5 * coinamt;
//  blankpayoff = 11*coinamt;
  uuupayoff = 10 * coinamt;
  ipayoff = 15 * coinamt;
  gpayoff = 100 * coinamt;
  epayoff = 250 * coinamt;
  upayoff = 1250 * coinamt;
  tpayoff = 25000 * coinamt;
  greywheel = 1313;

  if ((wheela == 0) && (wheelb == 0) && (wheelc == 0))
    coins = tpayoff;            /* tiamat tiamat tiamat */

  if (((wheela >= 15) && (wheela <= 18)) ||
      ((wheelb >= 15) && (wheelb <= 18)) ||
      ((wheelc >= 15) && (wheelc <= 18)))
    coins = coinamt;            /* any gnome */

  if (((wheela >= 62) && (wheela <= 64)) ||
      ((wheelb >= 62) && (wheelb <= 64)) ||
      ((wheelc >= 62) && (wheelc <= 64)))
    coins = coinamt;            /* any goblin */

  if (((wheela >= 11) && (wheela <= 46)) &&
      ((wheelb >= 11) && (wheelb <= 46)) &&
      ((wheelc >= 11) && (wheelc <= 46)))
    coins = gggpayoff;          /* 3 goodies */

  if (((wheela >= 47) && (wheela <= 67)) &&
      ((wheelb >= 47) && (wheelb <= 67)) &&
      ((wheelc >= 47) && (wheelc <= 67)))
    coins = eeepayoff;          /* 3 evils */

  if (((wheela >= 68) && (wheela <= 79)) &&
      ((wheelb >= 68) && (wheelb <= 79)) &&
      ((wheelc >= 68) && (wheelc <= 79)))
    coins = uuupayoff;          /* 3 undead */

  if (((wheela >= 1) && (wheela <= 10)) &&
      ((wheelb >= 1) && (wheelb <= 10)) && ((wheelc >= 1) && (wheelc <= 10)))
    coins = ipayoff;            /* any illithid */

  if (((wheela >= 11) && (wheela <= 14)) &&
      ((wheelb >= 11) && (wheelb <= 14)) &&
      ((wheelc >= 11) && (wheelc <= 14)))
    coins = gpayoff;            /* 3 same goodies */

  if (((wheela >= 15) && (wheela <= 18)) &&
      ((wheelb >= 15) && (wheelb <= 18)) &&
      ((wheelc >= 15) && (wheelc <= 18)))
    coins = gpayoff;            /* 3 same goodies */

  if (((wheela >= 19) && (wheela <= 22)) &&
      ((wheelb >= 19) && (wheelb <= 22)) &&
      ((wheelc >= 19) && (wheelc <= 22)))
    coins = gpayoff;            /* 3 same goodies */

  if (((wheela >= 23) && (wheela <= 26)) &&
      ((wheelb >= 23) && (wheelb <= 26)) &&
      ((wheelc >= 23) && (wheelc <= 26)))
    coins = gpayoff;            /* 3 same goodies */

  if (((wheela >= 27) && (wheela <= 30)) &&
      ((wheelb >= 27) && (wheelb <= 30)) &&
      ((wheelc >= 27) && (wheelc <= 30)))
    coins = gpayoff;            /* 3 same goodies */

  if (((wheela >= 31) && (wheela <= 34)) &&
      ((wheelb >= 31) && (wheelb <= 34)) &&
      ((wheelc >= 31) && (wheelc <= 34)))
    coins = gpayoff;            /* 3 same goodies */

  if (((wheela >= 35) && (wheela <= 38)) &&
      ((wheelb >= 35) && (wheelb <= 38)) &&
      ((wheelc >= 35) && (wheelc <= 38)))
    coins = gpayoff;            /* 3 same goodies */

  if (((wheela >= 39) && (wheela <= 42)) &&
      ((wheelb >= 39) && (wheelb <= 42)) &&
      ((wheelc >= 39) && (wheelc <= 42)))
    coins = gpayoff;            /* 3 same goodies */

  if (((wheela >= 43) && (wheela <= 46)) &&
      ((wheelb >= 43) && (wheelb <= 46)) &&
      ((wheelc >= 43) && (wheelc <= 46)))
    coins = gpayoff;            /* 3 same goodies */

  if (((wheela >= 47) && (wheela <= 49)) &&
      ((wheelb >= 47) && (wheelb <= 49)) &&
      ((wheelc >= 47) && (wheelc <= 49)))
    coins = epayoff;            /* 3 same evils */

  if (((wheela >= 50) && (wheela <= 52)) &&
      ((wheelb >= 50) && (wheelb <= 52)) &&
      ((wheelc >= 50) && (wheelc <= 52)))
    coins = epayoff;            /* 3 same evils */

  if (((wheela >= 53) && (wheela <= 55)) &&
      ((wheelb >= 53) && (wheelb <= 55)) &&
      ((wheelc >= 53) && (wheelc <= 55)))
    coins = epayoff;            /* 3 same evils */

  if (((wheela >= 56) && (wheela <= 58)) &&
      ((wheelb >= 56) && (wheelb <= 58)) &&
      ((wheelc >= 56) && (wheelc <= 58)))
    coins = epayoff;            /* 3 same evils */

  if (((wheela >= 59) && (wheela <= 61)) &&
      ((wheelb >= 59) && (wheelb <= 61)) &&
      ((wheelc >= 59) && (wheelc <= 61)))
    coins = epayoff;            /* 3 same evils */

  if (((wheela >= 62) && (wheela <= 64)) &&
      ((wheelb >= 62) && (wheelb <= 64)) &&
      ((wheelc >= 62) && (wheelc <= 64)))
    coins = epayoff;            /* 3 same evils */

  if (((wheela >= 65) && (wheela <= 67)) &&
      ((wheelb >= 65) && (wheelb <= 67)) &&
      ((wheelc >= 65) && (wheelc <= 67)))
    coins = epayoff;            /* 3 same evils */

  if (((wheela >= 68) && (wheela <= 69)) &&
      ((wheelb >= 68) && (wheelb <= 69)) &&
      ((wheelc >= 68) && (wheelc <= 69)))
    coins = upayoff;            /* 3 same undead */

  if (((wheela >= 70) && (wheela <= 71)) &&
      ((wheelb >= 70) && (wheelb <= 71)) &&
      ((wheelc >= 70) && (wheelc <= 71)))
    coins = upayoff;            /* 3 same undead */

  if (((wheela >= 72) && (wheela <= 73)) &&
      ((wheelb >= 72) && (wheelb <= 73)) &&
      ((wheelc >= 72) && (wheelc <= 73)))
    coins = upayoff;            /* 3 same undead */

  if (((wheela >= 74) && (wheela <= 75)) &&
      ((wheelb >= 74) && (wheelb <= 75)) &&
      ((wheelc >= 74) && (wheelc <= 75)))
    coins = upayoff;            /* 3 same undead */

  if (((wheela >= 76) && (wheela <= 77)) &&
      ((wheelb >= 76) && (wheelb <= 77)) &&
      ((wheelc >= 76) && (wheelc <= 77)))
    coins = upayoff;            /* 3 same undead */

  if (((wheela >= 78) && (wheela <= 79)) &&
      ((wheelb >= 78) && (wheelb <= 79)) &&
      ((wheelc >= 78) && (wheelc <= 79)))
    coins = upayoff;            /* 3 same undead */

  if (((wheela >= 80) && (wheela <= 109)) &&
      ((wheelb >= 80) && (wheelb <= 109)) &&
      ((wheelc >= 80) && (wheelc <= 109)))
    coins = blankpayoff;        /* 3 blanks */

  if (((wheela >= 39) && (wheela <= 42)) &&
      ((wheelb >= 43) && (wheelb <= 46)) &&
      ((wheelc >= 47) && (wheelc <= 49)))
    coins = greywheel;          /* GREY WHEEL! */


  if (coins)
  {
    if (coins > (5 * coinamt))
      act
        ("$n &+winserts a coin into $p&+w, pulls the lever, and &+Bwon&+w!&N",
         FALSE, ch, obj, 0, TO_ROOM);
    if (coins == gpayoff)
    {
      act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Goodies&N!",
          FALSE, ch, 0, 0, TO_ROOM);
      act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Goodies!&N",
          FALSE, ch, 0, 0, TO_CHAR);
      obj->value[4]++;
    }
    if (coins == ipayoff)
    {
      act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Illithids&N!", FALSE,
          ch, 0, 0, TO_ROOM);
      act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Illithids!&N", FALSE,
          ch, 0, 0, TO_CHAR);
//          obj->value[0]++;
    }

    if (coins == epayoff)
    {
      act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Evils&N!", FALSE,
          ch, 0, 0, TO_ROOM);
      act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Evils!&N", FALSE,
          ch, 0, 0, TO_CHAR);
      obj->value[5]++;
    }

    if (coins == upayoff)
    {
      act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Undeads&N!",
          FALSE, ch, 0, 0, TO_ROOM);
      act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Undeads!&N",
          FALSE, ch, 0, 0, TO_CHAR);
      obj->value[6]++;
    }

    if (coins == tpayoff)
    {
      act
        ("&+wThe &=LRsiren&N&+w goes off&+B!&+w  Looks like someone hits the &+Rjackpot&N!",
         FALSE, ch, 0, 0, TO_ROOM);
      act
        ("&+wThe &=LRsiren&N&+w goes off&+B!&+w  Looks like you hit the &+Rjackpot!&N",
         FALSE, ch, 0, 0, TO_CHAR);
      obj->value[7]++;
      wizlog(MINLVLIMMORTAL,
             "%s has just won the Tiamat jackpot at 1:512000 odds  on a slot machine in room [%d]",
             GET_NAME(ch), world[ch->in_room].number);
    }

    if (coins == greywheel)
    {
      // odds of even getting here are...        1:10,666
      // odds of getting all all 3 drow 3 times are    1:119,796
      // total odds of winning full jackpot of 100k are  1:1,277,825,638
      obj->value[0]++;
      coins = 100 * coinamt;
      act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Its ELF TIME!&N!", FALSE, ch,
          0, 0, TO_ROOM);
      act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Its ELF TIME!&N!", FALSE, ch,
          0, 0, TO_CHAR);

      for (count = 0; count < 3; count++)
      {
        wheela = number(39, 49);
        wheelb = number(39, 49);
        wheelc = number(39, 49);
        sprintf(Gbuf1, "The machine shakes and you see: %s %s %s\n",
                name[wheela], name[wheelb], name[wheelc]);
        send_to_char(Gbuf1, ch);

        if (((wheela >= 39) && (wheela <= 42)) &&       // half elf
            ((wheelb >= 39) && (wheelb <= 42)) &&       // odds 1 20.796875
            ((wheelc >= 39) && (wheelc <= 42)))
        {
          coins *= 5;
          act
            ("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+CHalf-&+cElf&+ws! Multiplier x5!&N!",
             FALSE, ch, 0, 0, TO_ROOM);
          act
            ("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+CHalf-&+cElf&+ws! Multiplier x5!&N",
             FALSE, ch, 0, 0, TO_CHAR);
        }

        if (((wheela >= 43) && (wheela <= 46)) &&       // grey elf
            ((wheelb >= 43) && (wheelb <= 46)) &&       // odds 1 20.796875
            ((wheelc >= 43) && (wheelc <= 46)))
        {
          coins *= 5;
          act
            ("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+cGrey-Elf&+ws! Multiplier x5!&N!",
             FALSE, ch, 0, 0, TO_ROOM);
          act
            ("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+cGrey-Elf&+ws! Multiplier x5!&N",
             FALSE, ch, 0, 0, TO_CHAR);
        }

        if (((wheela >= 47) && (wheela <= 49)) &&       // drow elf
            ((wheelb >= 47) && (wheelb <= 49)) &&       // odds 1 49.2962962962962962962962962962784
            ((wheelc >= 47) && (wheelc <= 49)))
        {
          coins *= 10;
          act
            ("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+mDrow Elf&+ws! Multiplier x10!&N!",
             FALSE, ch, 0, 0, TO_ROOM);
          act
            ("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+mDrow Elf&+ws! Multiplier x10!&N",
             FALSE, ch, 0, 0, TO_CHAR);
        }
      }
    }
    if (coins == 100000 * coinamt)
    {
      wizlog(MINLVLIMMORTAL,
             "%s got the ELF jackpot at 1 to 1,277,825,638 odds on a slot machine in room [%d]",
             GET_NAME(ch), world[ch->in_room].number);
      act
        ("&+wThe &=LBsiren&N&+w goes off&+B!&+c %s has just won &=LWTHE JACKPOT!&N!",
         FALSE, ch, 0, 0, TO_ROOM);
      act
        ("&+wThe &=LRsiren&N&+w goes off&+B!&+c %s has just won &=LWTHE JACKPOT!&N!",
         FALSE, ch, 0, 0, TO_ROOM);
      act
        ("&+wThe &=LRsiren&N&+w goes off&+B!&+c you have just won &=LWTHE JACKPOT!&N",
         FALSE, ch, 0, 0, TO_CHAR);
      act
        ("&+wThe &=LRsiren&N&+w goes off&+B!&+c you have just won &=LWTHE JACKPOT!&N",
         FALSE, ch, 0, 0, TO_CHAR);
      if (type == 3)
      {
        coins = 50000 * coinamt;
        act("&+wYou receive &+Ba restring coupon&N.&N", FALSE, ch, 0, 0,
            TO_CHAR);
        act("&+wYou receive &+Ra restring coupon&N.&N", FALSE, ch, 0, 0,
            TO_CHAR);
        obj_to_char(read_object(44, VIRTUAL), ch);
        obj_to_char(read_object(43, VIRTUAL), ch);
      }
    }

    sprintf(Gbuf1, "You win %d %s coin(s)!", coins, coin_names[type]);
    switch (type)
    {
    case 0:
      GET_COPPER(ch) += coins;
      break;
    case 1:
      GET_SILVER(ch) += coins;
      coins *= 10;
      break;
    case 2:
      GET_GOLD(ch) += coins;
      coins *= 100;
      break;
    case 3:
      GET_PLATINUM(ch) += coins;
      coins *= 1000;
      break;
    }
    act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
    obj->value[3] += coins;
  }
  return TRUE;
}

int xmas_cap(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  P_char victim;
  int    curr_time = time(NULL);

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // 1/20 chance.
  if( cmd != CMD_GOTHIT || number(0, 19) )
  {
    return FALSE;
  }

  // important! can do this cast (next line) ONLY if cmd was CMD_GOTHIT or CMD_GOTNUKED
  if( !(data = (struct proc_data *) arg) )
  {
    return FALSE;
  }
  victim = data->victim;
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }

/*  if(curr_time > ( 1135362289 + 60 * 60* 24 * 7))
   {
    act("&+L$p &+Lhums with a &+GCRA&+YC&+GKED &+Lsounds.&n", FALSE, ch, obj, 0, TO_CHAR);
    act("&+L$p &+Lcrumble to dust.&n", FALSE, ch, obj, 0, TO_CHAR);
    extract_obj(obj, TRUE); // Not an arti, but 'in game.'
    return FALSE;
  } */

  if( IS_PC(ch) && obj->value[6] != GET_PID(ch) )
  {
    send_to_char("Stealing Presents is a BAD idea!\r\n", ch);
    act("&+L$p &+Lhums with a &+GCRA&+YC&+GKED &+Lsounds.&n", FALSE, ch, obj, 0, TO_CHAR);
    act("&+L$p &+Lcrumble to dust.&n", FALSE, ch, obj, 0, TO_CHAR);
    spell_ice_missile(30, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
    extract_obj(obj, TRUE); // Not an arti, but 'in game.'
    return FALSE;
  }
  if( IS_PC(victim) )
  {
    return FALSE;
  }

  act("&+W$n's&N $q &+Wspins in a fury&n, bringing the &+Rpower&n of &+WSanta!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+WYour&N $q &+Wspins in a fury&n, bringing the &+Rpower&n of &+WSanta!&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+W$n's&N $q &+Wspins in a fury&n, bringing the &+Rpower&n of &+WSanta!&N", TRUE, ch, obj, victim, TO_VICT);

  if( GET_LEVEL(ch) < 10 )
  {
    spell_ice_missile(30, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
    return TRUE;
  }
  if( GET_LEVEL(ch) < 25 )
  {
    spell_chill_touch(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, victim, 0);
    return TRUE;
  }
  if( GET_LEVEL(ch) < 38 )
  {
    spell_ice_storm(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
    return TRUE;
  }
  if( GET_LEVEL(ch) < 45 )
  {
    spell_frostbite(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, victim, 0);;
    return TRUE;
  }
  if( number(0,1) )
  {
    spell_arieks_shattering_iceball(30, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }
  else
  {
    spell_cone_of_cold(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }
  return TRUE;
}

#ifdef THARKUN_ARTIS

int bloodfeast(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;
  struct damage_messages messages = {
    "&+WYou feel energy flowing from $N&+W into you!&N",
    "&+LYou feel the lifeforce being drained from your limbs!&N",
    "&+L$N&+L looks &n&+wpale &+Las $s lifeforce is drained by $n!&N",
    "", "", ""
  };

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !(victim = (P_char) arg) )
  {
    return FALSE;
  }
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  // 1/25 chance.
  if( CheckMultiProcTiming(ch) && !number(0, 24))
  {
    act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim, TO_CHAR);
    act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim, TO_NOTVICT);
    act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim, TO_VICT);
    spell_damage(ch, victim, 400, SPLDAM_NEGATIVE, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | RAWDAM_NOKILL, &messages);
    vamp(ch, 50, (int) (GET_MAX_HIT(ch) * 1.3));
    return TRUE;
  }
  return FALSE;
}

#else

/* Bahamut procs -Zod */

int bloodfeast(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time, dam;
  P_char   victim;

  /*
     check for periodic event calls
   */
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC )
  {
    if( OBJ_WORN(obj) && obj->loc.wearing == ch )
    {
      curr_time = time(NULL);
      // Every 30 sec.
      if( obj->timer[0] + 30 <= curr_time && !has_skin_spell(temp_ch) )
      {
        spell_stone_skin(45, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
    hummer(obj);
    return TRUE;
  }

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN(obj) || obj->loc.wearing != ch )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  if( !number(0, 24) && CheckMultiProcTiming(ch) && !IS_RACEUNDEAD(victim) )
  {

    dam = BOUNDED(0, (GET_HIT(victim) + 9), 100);
    act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim, TO_CHAR);
    act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim, TO_NOTVICT);
    act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim, TO_VICT);
    act("&+WYou feel energy flowing from $N&+W into you!&N", TRUE, ch, obj, victim, TO_CHAR);
    act("&+LYou feel the lifeforce being drained from your limbs!&N", TRUE, ch, obj, victim, TO_VICT);
    act("&+L$N&+L looks &n&+wpale &+Las $s lifeforce is drained by $n!&N", TRUE, ch, obj, victim, TO_NOTVICT);
    vamp(ch, dam / 2, (int) (GET_MAX_HIT(ch) * 1.3));

    melee_damage(ch, victim, dam, PHSDAM_NOSHIELDS | PHSDAM_NOREDUCE | PHSDAM_NOPOSITION, 0);
    return TRUE;
  }
  return FALSE;
}
#endif

int mist_claymore(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_MELEE_HIT)
    return (FALSE);
  if (!ch)
    return (FALSE);

  if (!OBJ_WORN(obj) || (obj->loc.wearing != ch))
    return (FALSE);
  victim = (P_char) arg;
  if (!victim)
    return (FALSE);
  if (number(0, 30))
    return (FALSE);
  act("&+W$n's&N $q &+Wglows with an &+Geerie flame&+W!!&N", TRUE, ch, obj,
      victim, TO_NOTVICT);
  act("&+WYour&N $q &+Wglows with an &+Geerie flame&+W!!&N", TRUE, ch, obj,
      victim, TO_CHAR);
  act("&+W$n's&N $q &+Wglows with an &+Geerie flame&+W!!&N", TRUE, ch, obj,
      victim, TO_VICT);
  spell_incendiary_cloud(50, ch, NULL, 0, victim, obj);
  return (TRUE);
}

void event_revenant_crown(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type *af;
  P_obj armor = ch->equipment[WEAR_HEAD];
  
  if(GET_RACE(ch) != RACE_REVENANT)
  { 
    act("Your skin blisters and boils start to form!&n",
      FALSE, ch, obj, 0, TO_CHAR);
    wizlog(57," Reverant crown worn by %s begins to melt due race check conflict!", GET_NAME(ch));
    GET_HIT(ch) >> 1;
    CharWait(ch, 2 * WAIT_SEC);
  }
  else if ((af = get_spell_from_char(ch, TAG_RACE_CHANGE)) == NULL)
  {
    send_to_char
      ("&+WPossible serious screwup in the revenant helm proc! Tell a coder as once!&n\r\n",
       ch);
    wizlog(57,
        "Char %s found with racechange event but without racechange affect! revenant proc",
        GET_NAME(ch));
    return;
  }
  else if ( armor != NULL && obj_index[armor->R_num].virtual_number == REVENANT_CROWN_VNUM)
  {
    add_event(event_revenant_crown, (int) (0.5 * PULSE_VIOLENCE), ch, 0, 0, 0, 0, 0);
    return;
  }
  else
  {
    ch->player.race = af->modifier;
    affect_remove(ch, af);
//    GET_AGE(ch) = racial_data[(int) GET_RACE(ch)].base_age*2;
    // Set birthdate + base_age + 5 years.
    ch->player.time.birth = time(NULL);
    // Add base_age to birthdate + base_age + 5 years.
    ch->player.time.birth -= (racial_data[GET_RACE(ch)].base_age) * SECS_PER_MUD_YEAR;
    send_to_char
      ("The curse of the dark powers fade and your soul restores the body.\r\n",
       ch);
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
      ("Brr, you suddenly feel very naked.\r\n",
       ch);

    return;
  }

}

int revenant_helm(P_obj obj, P_char ch, int cmd, char *arg)
{
  int k = 0;
  P_obj  temp_obj;
  struct affected_type af;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC || !OBJ_WORN(obj) )
  {
    return FALSE;
  }
  // Need to do this in 2 steps, 'cause CMD_PERIODIC doesn't assign ch. *sigh*
  //   We could put an !(ch = obj->loc.wearing) .. but that assignment might not come before
  //   the IS_ALIVE etc checks.
  ch = obj->loc.wearing;
  if( !IS_ALIVE(ch) || IS_NPC(ch) || affected_by_spell(ch, TAG_RACE_CHANGE) )
  {
    return FALSE;
  }

  // Remove all eq..
  for( k = 0; k < MAX_WEAR; k++ )
  {
    temp_obj = ch->equipment[k];
    if( temp_obj && (obj != temp_obj) )
	  {
      if( obj_index[temp_obj->R_num].func.obj != NULL )
      {
        // Call the objects remove proc if there might be one.
        (*obj_index[temp_obj->R_num].func.obj) (temp_obj, ch, CMD_REMOVE, (char *) "all");
	    }
      obj_to_char(unequip_char(ch, k), ch);
      if( !IS_ALIVE(ch) )
      {
        return FALSE;
      }
    }
  }

  CharWait(ch, 5 * WAIT_SEC);
  send_to_char("Brr, you suddenly feel _almost_ naked.\r\n\n", ch);

  memset(&af, 0, sizeof(af));
  af.type = TAG_RACE_CHANGE;
  af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL;
  af.duration = -1;
  af.modifier = GET_RACE(ch);
  affect_to_char(ch, &af);
  add_event(event_revenant_crown, (int)(0.5 * PULSE_VIOLENCE), ch, 0, 0, 0, 0, 0);

  act("&+LThe figure of $n &+Lgrows darker and darker as $e absorbs all surrounding\n"
    "&+Wlight&+L. A sphere of absolute darkness spreads out around $m expanding\n"
    "&+Lrapidly outwards, and engulfing $n&+L. Moments later a loud boom echoes\n"
    "&+Lloudly from the sphere and where $n once stood is now a creature\n"
    "&+Lof great &+Bpower &+Land &N&+revil&+L...", FALSE, ch, obj, 0, TO_ROOM);
  act("Your $q &+Lhums loudly as it draws &+Cenergy &+Lfrom\n"
    "&+Lthe dark powers. You scream in agony as it melts the flesh of your\n"
    "&+Lbody transforming you into a creature of &+Bcold &+Land &N&+wdeath&+L!", FALSE, ch, obj, 0, TO_CHAR);

  ch->player.race = RACE_REVENANT;
  // Set birthdate + base_age + 5 years.
  ch->player.time.birth = time(NULL);
  // Add base_age to birthdate + base_age + 5 years.
  ch->player.time.birth -= (racial_data[RACE_REVENANT].base_age) * SECS_PER_MUD_YEAR;
  return TRUE;
}

void event_dragonlord_check(P_char ch, P_char victim, P_obj obj, void *data)
{
  struct affected_type *af;
  P_obj armor = ch->equipment[WEAR_BODY];
  int temp, dragonlord_slot = MAX_WEAR;
  bool bHasOtherArti = false;

  if(GET_RACE(ch) != RACE_DRAGONKIN)
  {
    act("Your scales smoke and burn as they &+Rdisintegrate!&n",
      FALSE, ch, obj, 0, TO_CHAR);
    wizlog(57,"Dragonlord armor worn by %s begins to melt due race check conflict!", GET_NAME(ch));
    GET_HIT(ch) >> 1;
    CharWait(ch, 2 * WAIT_SEC);
  }
  if((af = get_spell_from_char(ch, TAG_RACE_CHANGE)) == NULL)
  {
    send_to_char
      ("&+WPossible serious screwup in the dragonlord proc! Tell a coder as once!&n\r\n", ch);
    wizlog(57, "Char %s found with racechange event but without racechange affect! Dragonlord proc", GET_NAME(ch));
    return;
  }
// Same code Necroplasm uses.
  for (int i = 0; i < MAX_WEAR; i++)
  {
    if(ch->equipment[i] == armor)
    {
      dragonlord_slot = i;
    }
    if(ch->equipment[i] &&
       IS_SET(ch->equipment[i]->extra_flags, ITEM_ARTIFACT) &&
       (ch->equipment[i] != armor))
    {
      bHasOtherArti = true;
    }
  }
  
  if(bHasOtherArti &&
    (dragonlord_slot != MAX_WEAR) &&
    ch->equipment[dragonlord_slot])
  {
    act("The &+Wplatemail&n of the &+YDragonLord&n erupts acid and detaches from $n's body!&n",
      FALSE, ch, obj, 0, TO_ROOM);
    act("The &+Wplatemail&n of the &+YDragonLord&n erupts acid as it detaches from your body!",
      FALSE, ch, obj, 0, TO_CHAR);
    obj_to_char(unequip_char(ch, dragonlord_slot), ch);
    add_event(event_dragonlord_check, (int)(0.5 * PULSE_VIOLENCE), ch, 0, 0, 0, 0, 0); 
  }
  else if(armor != NULL &&
          obj_index[armor->R_num].virtual_number == DRAGONLORD_PLATE_VNUM &&
          GET_STAT(ch) > STAT_DEAD)
  {
    add_event(event_dragonlord_check, (int)(0.5 * PULSE_VIOLENCE), ch, 0, 0, 0, 0, 0); 
    return;
  }
  else
  {
    ch->player.race = af->modifier;
    affect_remove(ch, af);
    ch->player.time.birth = time(NULL) - (racial_data[GET_RACE(ch)].base_age) * 2;
//    GET_AGE(ch) = racial_data[(int) GET_RACE(ch)].base_age*2;
    // Set birthdate + base_age + 5 years.
    ch->player.time.birth = time(NULL);
    // Add base_age to birthdate + base_age + 5 years.
    ch->player.time.birth -= (racial_data[GET_RACE(ch)].base_age) * SECS_PER_MUD_YEAR;
    send_to_char("The curse of the dark powers fade and your soul restores the body.\r\n", ch);

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
      ("Brr, you suddenly feel very naked.\r\n",
       ch);

    return;
  }
  // int k = 0;
  // P_obj temp_obj;
  // for (k = 0; k < MAX_WEAR; k++)
  // {
    // temp_obj = ch->equipment[k];
    // if(temp_obj)
    // {
      // obj_to_char(unequip_char(ch, k), ch);
    // }
  // }
}

int dragonlord_plate_old(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_obj temp_obj;
  P_char temp_ch;
  struct affected_type af;
  int k = 0, temp_age = 1, secs;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == CMD_PERIODIC)
  {
    if (OBJ_WORN(obj))
    {
      temp_ch = obj->loc.wearing;

      if(!temp_ch ||
         !IS_ALIVE(temp_ch) ||
         !IS_PC(temp_ch))
      {
        return TRUE;
      }
      
      if(affected_by_spell(temp_ch, TAG_RACE_CHANGE))
      {
        return TRUE;
      }

      for(k = 0; k < MAX_WEAR; k++)
      {
        temp_obj = temp_ch->equipment[k];
        
        if(temp_obj &&
          (obj != temp_obj))
        {
          if (obj_index[temp_obj->R_num].func.obj != NULL)
            (*obj_index[temp_obj->R_num].func.obj) (temp_obj, temp_ch, CMD_REMOVE, (char *) "all");
          obj_to_char(unequip_char(temp_ch, k), temp_ch);
        }
      }
      
      send_to_char("Brr, you suddenly feel _almost_ naked.\r\n\n", temp_ch);

      CharWait(temp_ch, 5 * WAIT_SEC);

      memset(&af, 0, sizeof(af));
      af.type = TAG_RACE_CHANGE;
      af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL;
      af.duration = -1;
      af.modifier = GET_RACE(temp_ch);
      affect_to_char(temp_ch, &af);
      
      add_event(event_dragonlord_check, (int)(0.5 * PULSE_VIOLENCE), temp_ch, 0, 0, 0, 0, 0);
    
      act("&+RPain &+Llike you have never felt before renders you momentarily dazed as your\n&+Lflesh is ripped apart.  Your &N&+rmuscles &+Lripple and flex as they grow in size and\n&+Lstrength and a new skin of &N&+whardened dragonscales begins to form upon your body.\n&+LYou emerge from the transformation, flex your mighty new wings and &+Rroar &+Lloudly!&n\r\n",
          FALSE, temp_ch, obj, 0, TO_CHAR);
      act("&+L$n shudders and drops to $s knees as the awesome transformation takes hold.\n&+L$n&+L's body grows more muscular, while thick scales replace the shedded skin\n&+Land two &+Rgreat wings &+Lsprout from $s back. $n's face twists and is replaced by the\n&+Lvisage of a dragon, jaws filled with razor sharp teeth as $e roars loudly!&n\r\n",
          FALSE, temp_ch, obj, 0, TO_ROOM);

      temp_ch->player.race = RACE_DRAGONKIN;

      temp_ch->player.time.birth = time(NULL) - (racial_data[RACE_DRAGONKIN].base_age) * 2;
//      GET_AGE(temp_ch) += racial_data[RACE_DRAGONKIN].base_age * 2;
      // Set birthdate + base_age + 5 years.
      temp_ch->player.time.birth = time(NULL);
      // Add base_age to birthdate + base_age + 5 years.
      temp_ch->player.time.birth -= (racial_data[RACE_DRAGONKIN].base_age) * SECS_PER_MUD_YEAR;

      return TRUE;
    }
  }
  return FALSE;
}

int dragonlord_plate(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && OBJ_WORN(obj) && (ch = obj->loc.wearing) && IS_ALIVE(obj->loc.wearing) )
  {
    curr_time = time(NULL);
    // Every 30 min.
    if( obj->timer[1] + 1800 <= curr_time && !IS_AFFECTED4(ch, AFF4_STORNOGS_SPHERES) )
    {
      spell_stornogs_spheres(53, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      obj->timer[1] = curr_time;
      return TRUE;
    }
    // Every 30 sec.
    if( obj->timer[0] + 30 <= curr_time && !affected_by_spell(ch, SPELL_STONE_SKIN) )
    {
      spell_stone_skin(45, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      obj->timer[0] = curr_time;
      return TRUE;
    }
  }
  return FALSE;
}

int dragonlord_plate_oldold(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time;
  P_char temp_ch;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd == CMD_PERIODIC)
  {
    if(!(ch))
    {
      return false;
    }
    
    if (OBJ_WORN(obj))
    {
      temp_ch = obj->loc.wearing;
      curr_time = time(NULL);
      
      if(obj->timer[1] + 1800 <= curr_time &&
        !IS_AFFECTED4(temp_ch, AFF4_STORNOGS_SPHERES))
      {
        spell_stornogs_spheres(53, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
        
        obj->timer[1] = curr_time;
        
        return TRUE;
      }
      
      if(obj->timer[0] + 30 <= curr_time &&
        !has_skin_spell(temp_ch))
      {
        spell_stone_skin(45, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
        
        obj->timer[0] = curr_time;
        
        return TRUE;
      }
    }
  }
  return (FALSE);
}

int olympus_portal(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      to_room, base, real_top, real_bottom, origin_portal, origin_room;
  P_obj    portal = NULL;
  char     Gbuf1[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  // Every 30 sec..
  if( !obj || cmd != CMD_PERIODIC || obj->timer[0] + 30 > time(NULL) )
  {
    return FALSE;
  }

  switch( obj_index[obj->R_num].virtual_number )
  {
    case 99801:
      base = 140000;
      origin_portal = real_object(99800);
      origin_room = real_room(99805);
      break;
    case 99803:
      base = 150000;
      origin_portal = real_object(99802);
      origin_room = real_room(99806);
      break;
    case 99805:
      base = 160000;
      origin_portal = real_object(99804);
      origin_room = real_room(99807);
      break;
    case 99807:
      base = 170000;
      origin_portal = real_object(99806);
      origin_room = real_room(99808);
      break;
    default:
      return FALSE;
  }
  real_bottom = zone_table[world[real_room0(base)].zone].real_bottom;
  real_top = zone_table[world[real_room0(base)].zone].real_top;
  do
  {
    to_room = number(real_bottom, real_top);
  }
  while( IS_SET(world[to_room].sector_type, SECT_OCEAN) );

  if( OBJ_ROOM(obj) )
  {
    sprintf(Gbuf1, "&+WThe air shifts slighty as %s&+W folds up and vanishes!\n", obj->short_description);
    send_to_room(Gbuf1, obj->loc.room);
    obj_from_room(obj);
    obj_to_room(obj, to_room);
    sprintf(Gbuf1, "&+WA slight breeze wafts by as %s&+W materializes in the room!\n", obj->short_description);
    send_to_room(Gbuf1, obj->loc.room);
    if( origin_room > 0 && origin_portal > 0 )
    {
      portal = get_obj_in_list_num(origin_portal, world[origin_room].contents);
      if( portal )
      {
        portal->value[0] = world[obj->loc.room].number;
      }
    }
    obj->timer[0] = time(NULL);
    return TRUE;
  }

  return FALSE;
}

int living_necroplasm(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   temp_ch, i;
  int      slot;
  struct affected_type af;
  bool bHasOtherArti;
  int plasm_slot;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !obj || cmd != CMD_PERIODIC )
    return FALSE;

  if (OBJ_WORN(obj))
    ch = obj->loc.wearing;
  else if (OBJ_CARRIED(obj))
    ch = obj->loc.carrying;

  // recurse self
  if (!IS_SET(obj->extra_flags, ITEM_NODROP))
  {
    SET_BIT(obj->extra_flags, ITEM_NODROP);
  }

  // It's on body
  if (OBJ_WORN_BY(obj, ch) && !(obj == ch->equipment[HOLD]))
  {
    // verify that they have no other arti's - if they do, necro will poof from them
    bHasOtherArti = FALSE;
    plasm_slot = MAX_WEAR;
    for( int i = 0; i < MAX_WEAR; i++ )
    {
      if( ch->equipment[i] == obj )
      {
        plasm_slot = i;
        continue;
      }
      if( ch->equipment[i] && IS_ARTIFACT(ch->equipment[i]) )
      {
        bHasOtherArti = TRUE;
      }
    }
    if( bHasOtherArti && (plasm_slot != MAX_WEAR) && ch->equipment[plasm_slot] )
    {
      act("$p &+Rburns &+ran angry red &+Las it retreats from $n's body&n", FALSE, ch, obj, 0, TO_ROOM);
      act("$p &+Rburns &+ran angry red &+Las it retreats from your body&n", FALSE, ch, obj, 0, TO_CHAR);
      obj_to_char(unequip_char(ch, plasm_slot), ch);
      // dispel any SPELL_VAMPIRE to prevent cheesing of removing arti, wearing plasm, wearing other arti.
      affect_from_char(ch, SPELL_VAMPIRE);
      return TRUE;
    }
    if( IS_PC(ch) && !number(0, 3) && !NewSaves(ch, SAVING_PARA, 6) )
    {
      act("&+MYou feel queasy as $p &+Msends its tendrils deep into your body, harvesting your lifeforce!", FALSE, ch, obj, 0, TO_CHAR);
      GET_HIT(ch) = MAX(1, GET_HIT(ch) - 20);
    }
    if( !affected_by_spell(ch, SPELL_CURSE) && !number(0, 4) && !NewSaves(ch, SAVING_SPELL, 0))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_CURSE;
      af.duration = 24;
      af.modifier = -2;
      af.location = APPLY_HITROLL;
      affect_to_char(ch, &af);
      af.modifier = 5;
      af.location = APPLY_CURSE;
      affect_to_char(ch, &af);
      act("&+r$n &+rhowls in pain as $s $q &+rglows &+Rred hot!", FALSE, ch, obj, 0, TO_ROOM);
      act("&+rYou howl in pain as your $q &+rglows &+Rred hot!", FALSE, ch, obj, 0, TO_CHAR);
    }
    if( !affected_by_spell(ch, SPELL_VAMPIRE) )
    {
      act("$p &+Lruns its &+Gtendrils&+L through $n's&+L body, transforming $m!", FALSE, ch, obj, 0, TO_ROOM);
      act("$p &+Lruns its &+Gtendrils&+L through your body, transforming you!", FALSE, ch, obj, 0, TO_CHAR);
      spell_vampire(55, ch, 0, 0, ch, 0);
    }
    return TRUE;
  }
  // obj on ground
  if( OBJ_ROOM(obj) )
  {
    for( i = world[obj->loc.room].people; i; i = i->next_in_room )
    {
      if( IS_NPC(i) || IS_TRUSTED(i) )
      {
        continue;
      }
      if( !number(0, 4) )
      {
        obj_from_room(obj);
        obj_to_char(obj, i);
        act("$p &+Lcrawls over to $n &+Land jumps at $m!", FALSE, i, obj, 0, TO_ROOM);
        act("$p &+Lcrawls up to you and jumps at you!", FALSE, i, obj, 0, TO_CHAR);
        ch = i;
        break;
      }
    }
  }

  // if it's in the HOLD slot, return to inventory
  if (OBJ_WORN_BY(obj, ch) && (obj == ch->equipment[HOLD]))
  {
    obj_to_char(unequip_char(ch, HOLD), ch);
  }
  // if not worn, but carried, equip self
  if( OBJ_CARRIED_BY(obj, ch) )
  {
    // Only folks who are primary allowed class can use this item
    if( can_prime_class_use_item(ch, obj) )
    {
      // verify that they have no other arti's before equip'ing
      for (int i = 0; i < MAX_WEAR; i++)
      {
        if( ch->equipment[i] && IS_ARTIFACT(ch->equipment[i]) )
        {
          return FALSE;
        }
      }

      if( GET_RACE(ch) == RACE_CENTAUR )
      {
        slot = WEAR_HORSE_BODY;
      }
      if( GET_RACE(ch) == RACE_DRIDER )
      {
        slot = WEAR_SPIDER_BODY;
      }
      else
      {
        slot = WEAR_BODY;
      }
      if( ch->equipment[slot] )
      {
        obj_to_char(unequip_char(ch, slot), ch);
      }
      obj_from_char(obj);
      equip_char(ch, obj, slot, FALSE);
      act("$p &+Mbegins to envelop your body!", FALSE, ch, obj, 0, TO_CHAR);
      act("$p &+Mwraps itself around $n!", FALSE, ch, obj, 0, TO_ROOM);
      act("&+LThe nausea is too much, and the world passes away...", FALSE, ch, obj, 0, TO_CHAR);
      act("&+cJust as abruptly, the nausea subsides, and you feel yourself strangely transformed!", FALSE, ch, obj, 0, TO_CHAR);
      act("&+C$n &+cgrows pale as $s body begins pulsing underneath $p&+c.", FALSE, ch, obj, 0, TO_ROOM);
      return TRUE;
    }
  }
  return FALSE;
}

// VAPOR
int vapor(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   temp_ch, i;
  P_char   vict;
  struct affected_type af;
  int      slot, curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !(OBJ_WORN(obj) || OBJ_CARRIED(obj)) || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( (OBJ_WORN(obj) && ch != obj->loc.wearing)
    || (OBJ_CARRIED(obj) && ch != obj->loc.carrying) )
  {
    return FALSE;
  }

  /*
    Damage proc on.
  */
  curr_time = time(NULL);
  if( obj->timer[1] + 30 < curr_time )
  {
    if( IS_FIGHTING(ch) && OBJ_WORN(obj) )
    {
      vict = ch->specials.fighting;
      if( !IS_ALIVE(vict) )
      {
        return FALSE;
      }
      // 1/10 chance.
      if( !number(0, 9) && GET_HIT(vict) > 40 && GET_MAX_HIT(ch) < (int)(GET_HIT(ch) * 1.250) )
      {
        act("&+LSuddenly the &+wgr&+gee&+Gn &N&+gha&+wze&n &+Laround you comes alive and a &+bchilling feeling &+Lcreeps down\r\n"
          "&+Lyour spine. Two &+wwri&+Wthi&+Lng te&+Wnta&N&+wcles of &+Gmist &+Lspring out wrapping&n &+wthemselves around\r\n"
          "&+Lthe chest of $N&+L.  Moments later $E lets out an agonized scream as the warmth\r\n"
          "&+Lis &+bdrained from $S &+Lbody.&n", FALSE, ch, obj, vict, TO_CHAR);
        act("&+LThe &+wgr&+gee&+Gn c&N&+glo&+wud &+Lencasing $n &+Lsuddenly turns pitch-black. &+LTwin\r\n"
          "&+wten&+Wtac&+Lles of &+wwri&+Wth&+Lin&+Wg &+Gmist &+Lleap out from the haze wrapping themselves around\r\n"
          "&+Lthe chest of $N &+Lwho &N&+bshudders &+Lfrom the &N&+bcold.  &+LMoments later $E\r\n"
          "&+Llets out an agonized scream as the warmth is &+bdrained from &+L$S body.&n", FALSE, ch, obj, vict, TO_NOTVICT);
        act("&+LThe &N&+wgr&+gee&+Gn &N&+gha&+wze &+Lencasing $n &+Lsuddenly turns pitch-black and unleashes &+Ltwin\r\n"
          "&+wten&+Wtac&+Lles &+Wof &+Gmist &+Ldirectly at you, which wrap themselves around your chest. &+bA chilling\r\n"
          "&+bcold &+Lspreads throughout your body, &N&+bnumbing you to the core.  &+LMoments later, you\r\n"
          "&+Lfeel your life's essence being tapped from your body.&n", FALSE, ch, obj, vict, TO_VICT);

        GET_HIT(ch) += 50;
        GET_HIT(vict) -= 50;
        if( GET_HIT(vict) < 0 )
        {
          GET_HIT(vict) = 0;
        }
        update_pos(vict);

        obj->timer[1] = curr_time;
        return FALSE;
      }
    }
  }
// End normal proc.

  if( cmd == CMD_SAY )
  {
    if( isname(arg, "ignite") && !affected_by_spell(ch, SPELL_FIRESHIELD) )
    {
      act("$n says 'ignite' to $p.", FALSE, ch, obj, 0, TO_ROOM);
      act("You say 'ignite'", FALSE, ch, 0, 0, TO_CHAR);
      if( affected_by_spell(ch, SPELL_COLDSHIELD) )
      {
        act("&+rYou let out a silence scream as the $p &+rfeeds on your life force.&n ", FALSE, ch, obj, 0, TO_CHAR);
        act("&+r$n lets out a silent scream as the $p &+rfeeds on $m!&n", FALSE, ch, obj, 0, TO_ROOM);
        GET_HIT(ch) = MAX(1, GET_HIT(ch) - 30);
        affect_from_char(ch, SPELL_COLDSHIELD);
      }
      act("$p &+Yignites &+Linto a &+rf&+Ri&+rr&+Re&+ry &+Lshield of protection!", FALSE, ch, obj, 0, TO_ROOM);
      act("$p &+Yignites &+Linto a &+rf&+Ri&+rr&+Re&+ry &+Lshield of protection!", FALSE, ch, obj, 0, TO_CHAR);
      spell_fireshield(55, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    }
    else if( isname(arg, "freeze") && !affected_by_spell(ch, SPELL_COLDSHIELD) )
    {
      act("$n says 'freeze' to $p.", FALSE, ch, obj, 0, TO_ROOM);
      act("You say 'freeze'", FALSE, ch, 0, 0, TO_CHAR);
      if( affected_by_spell(ch, SPELL_FIRESHIELD) )
      {
        act("&+rYou let out a silence scream as the $p &+rfeeds on your life force.&n ", FALSE, ch, obj, 0, TO_CHAR);
        act("&+r$n lets out a silent scream as the $p &+rfeeds on $m!&n", FALSE, ch, obj, 0, TO_ROOM);
        GET_HIT(ch) = MAX(1, GET_HIT(ch) - 30);
        affect_from_char(ch, SPELL_FIRESHIELD);
      }
      act("$p &+Cfreezes &+Linto a &+cc&+Ch&+ci&+Cl&+cl&+Ci&+cn&+Cg &+Lshield of protection!", FALSE, ch, obj, 0, TO_ROOM);
      act("$p &+Cfreezes &+Linto a &+cc&+Ch&+ci&+Cl&+cl&+Ci&+cn&+Cg &+Lshield of protection!", FALSE, ch, obj, 0, TO_CHAR);
      spell_coldshield(55, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    }
  }

  if( cmd == CMD_GOTHIT )
  {
    // recurse self
    if( !IS_SET(obj->extra_flags, ITEM_NODROP) )
    {
      SET_BIT(obj->extra_flags, ITEM_NODROP);
    }
    // It's on body
    if( OBJ_WORN_BY(obj, ch) && !affected_by_spell(ch, SPELL_GLOBE)
      && !IS_AFFECTED2(ch, AFF2_GLOBE) )
    {
      if( IS_PC(ch) )
      {
        act("&+rYou let out a silence scream as the $p &+rfeeds on your life force.&n ", FALSE, ch, obj, 0, TO_CHAR);
        act("&+r$n lets out a silent scream as the $p &+rfeeds on $m!&n", FALSE, ch, obj, 0, TO_ROOM);
        GET_HIT(ch) = MAX(1, GET_HIT(ch) - 30);
      }
      spell_globe(55, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
      return FALSE;
    }

    if( OBJ_CARRIED_BY(obj, ch) )
    {
      if( GET_RACE(ch) == RACE_CENTAUR )
      {
        slot = WEAR_HORSE_BODY;
      }
	    else if( GET_RACE(ch) == RACE_DRIDER )
      {
        slot = WEAR_SPIDER_BODY;
      }
      else
      {
        slot = WEAR_LEGS;
      }
      if( ch->equipment[slot] )
      {
        obj_to_char(unequip_char(ch, slot), ch);
      }
      obj_from_char(obj);
      equip_char(ch, obj, slot, FALSE);

      act("&+LSuddenly the &+ggreen vapor &+Lon the ground starts to sw&+wi&+Wrl &+Las if coming&n&L&+Lalive."
        "  You gasp in horror when you feel it caressing your legs as&n&L&+Lit coils itself around you."
        "  &+WWr&+wi&+Wthing tentacles &+Lstart to probe you&n&L&+Llike the arms of a hungry octopus."
        "  Within seconds your whole being&n&L&+Lis encased in a &+bchilling cloud&n &+Lof &+ggreen vapor&+L.&n", FALSE, ch, obj, 0, TO_CHAR);
      act("&+LSuddenly the &+ggreen vapor &+Lby&n $n's &+Lfeet starts to sw&+wi&+Wrl &+Las if&n&L&+Lcoming alive."
        "  Staring wide-eyed, as if trying to deny reality, he&n&L&+Lwatches as the &+wvapor &+Lslowly coils itself around his legs."
        "  &+WWr&+wi&+Wthing&n&L&+Wtentacles &+Lstart to probe $s body like the arms of a hungry octopus&n&L"
        "&+Land within seconds $e is encased in a &+bchilling &+Lcloud of vapor.&n", FALSE, ch, obj, 0, TO_ROOM);
      return FALSE;
    }
  }
  return FALSE;
}

int dragon_helm(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time;
  P_char   temp_ch;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  if (cmd == CMD_WEAR)
  {
    if (OBJ_WORN(obj))
    {
      temp_ch = obj->loc.wearing;
      if (obj->timer[0] == 0 && GET_RACE(ch) != RACE_DRAGON && IS_PC(ch))
      {
        obj->timer[0] = GET_RACE(ch);
        /* set them to dragon here */
        act
          ("&+RYour blood runs like fire as you feel yourself take on a new form!&N",
           FALSE, ch, 0, 0, TO_CHAR);
        return TRUE;
      }
    }
  }
  if (cmd == CMD_REMOVE)
  {
    if (obj && ch && IS_PC(ch) && GET_RACE(ch) == RACE_DRAGON &&
        obj->timer[0] > 0)
    {
      /* set them to orig race here */
      obj->timer[0] = 0;
      act("&+RYou return to your original form!&N", FALSE, ch, 0, 0, TO_CHAR);
      return (TRUE);
    }
  }
  return (FALSE);
}

int golem_chunk(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   tar_ch, next;
  int      curr_time;
  struct affected_type af;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if (cmd == CMD_PERIODIC)
  {
    if( !IS_SET(obj->extra_flags, ITEM_NODROP) )
    {
      SET_BIT(obj->extra_flags, ITEM_NODROP);
    }
    if( OBJ_WORN(obj) && (ch = obj->loc.wearing) )
    {
      // 1/5 chance.
      if( IS_ALIVE(ch) && !affected_by_spell(ch, SPELL_CURSE) && !number(0, 4) && !NewSaves(ch, SAVING_SPELL, 0))
      {
        bzero(&af, sizeof(af));
        af.type = SPELL_CURSE;
        af.duration = 24;
        af.modifier = -2;
        af.location = APPLY_HITROLL;
        affect_to_char(ch, &af);
        af.modifier = 5;
        af.location = APPLY_CURSE;
        affect_to_char(ch, &af);
        act("&+LThe Chu&Nnk &+Lon $n shakes vigoriously &+Wtossing&N them side to side then suddenly stops.&N", FALSE, ch, obj, 0, TO_ROOM);
        act("&+LThe Chu&Nnk&+L shakes vigoriously, &+Wtossing&N you side to side, then suddenly stope.&N", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
    return TRUE;
  }


  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "slow") )
    {
      curr_time = time(NULL);
      // Every 24 minutes?
      if (obj->timer[0] + (60 * 24) <= curr_time)
      {
        obj->timer[0] = curr_time;
        act("&+L$n's $p&+L billows forth an enormous cloud of smoke enblankening you.&N", FALSE, ch, obj, 0, TO_ROOM);
        act("&+LYour $p&+L billows forth an enormous cloud of smoke emblankening everything.&N", FALSE, ch, obj, 0, TO_CHAR);
        for( tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next )
        {
          next = tar_ch->next_in_room;
          if (tar_ch != ch && tar_ch->group != ch->group)
          {
            spell_slow(55, ch, 0, SPELL_TYPE_SPELL, tar_ch, 0);
          }
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

int good_evil_stoneOrSoulshield(P_obj obj)
{
  int      curr_time;
  P_char   temp_ch;

  temp_ch = obj->loc.wearing;

  if ((GET_ALIGNMENT(temp_ch) < 900) && (GET_ALIGNMENT(temp_ch) > -900))
  {
    if (!FIRESHIELDED(temp_ch))
      spell_fireshield(55, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
  }
  else
  {
    if (!IS_AFFECTED2(temp_ch, AFF2_SOULSHIELD))
      spell_soulshield(55, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
  }
  curr_time = time(NULL);
  if (obj->timer[0] + 10 <= curr_time && !has_skin_spell(temp_ch))
  {
    spell_stone_skin(55, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
    obj->timer[0] = curr_time;
    return TRUE;
  }

  return FALSE;

}

void good_evil_procDrain(P_char ch, P_obj obj, P_char opponent,
                         int *swordMana)
{
  act("Your $p &+Rglows&N as it &+rbites deeply&n into $N&n", FALSE, ch, obj,
      opponent, TO_CHAR);
  act("$n's $p &+rbites deeply&n into you, draining your &+Wlife force&n.",
      FALSE, ch, obj, opponent, TO_VICT);
  act("$N &+Lpales&n as $S &+Wlife force&n is drained significantly.", FALSE,
      ch, obj, opponent, TO_NOTVICTROOM);
  GET_HIT(opponent) -= 50;
  GET_HIT(ch) += 50;
  *swordMana += 100;
}

int good_evil_attemptFightProc(P_char ch, P_obj obj, P_char opponent,
                               int procMana, int *swordMana)
{

  act("$p &+Wglows brightly&N for a moment.", FALSE, ch, obj, 0, TO_CHAR);
  act("$p &+Wglows brightly&N for a moment.", FALSE, ch, obj, 0, TO_ROOM);

  if (procMana < *swordMana)
  {
    *swordMana -= procMana;
    obj->value[6] = (obj->value[6] + 1) % 15;
    return TRUE;
  }
  good_evil_procDrain(ch, obj, opponent, swordMana);
  return FALSE;
}

int good_evil_fightingProc(P_char ch, P_obj obj, int isGood, int mana)
{
  /* PUT IN USEFUL VALUES HERE */
  int      manaCosts[] = {
    50,                         /* 0  dazzle */
    50,                         /* 1  blind */
    50,                         /* 2  curse */
    100,                        /* 3  bigby's hand */
    0,                          /* 4  keep at zero for mana draining */
    100,                        /* 5  heal */
    100,                        /* 6  fist */
    50,                         /* 7  immolate */
    100,                        /* 8  earthquake */
    0,                          /* 9  keep at zero for mana draining */
    100,                        /* 10 gStornog */
    50,                         /* 11 poison */
    100,                        /* 12 (un)holy word */
    0,                          /* 13 keep at zero for mana draining */
    200                         /* 14 apocalypse / judgement */
  };

  int      rand, save;
  P_char   tar_ch, next;

  P_char   opponent;

  mana += 100;
  if (mana > GET_HIT(ch))
    mana = GET_HIT(ch);

  if (isGood)
    rand = obj->value[6];
  else
    rand = number(0, 14);

  opponent = ch->specials.fighting;
  if (opponent == NULL)
    return mana;

  if (good_evil_attemptFightProc(ch, obj, opponent, manaCosts[rand], &mana))
  {

    switch (rand)
    {
    case 0:
      break;
    case 1:
      /* blind */
      act("&+LA dark cloud shoots forth toward you!&N", FALSE, ch, 0,
          opponent, TO_VICT);
      act("&+LA dark cloud shoots forth toward $n!&N", TRUE, opponent, 0, 0,
          TO_ROOM);
      save = opponent->specials.apply_saving_throw[SAVING_SPELL];
      opponent->specials.apply_saving_throw[SAVING_SPELL] += 15;
      spell_blindness(60, ch, 0, SPELL_TYPE_SPELL, opponent, 0);
      opponent->specials.apply_saving_throw[SAVING_SPELL] = save;
      break;
    case 2:
      /* curse */
      act("&+rA red cloud shoots forth toward you!&N", FALSE, ch, 0, opponent,
          TO_VICT);
      act("&+rA red cloud shoots forth toward $n!&N", TRUE, opponent, 0, 0,
          TO_ROOM);
      save = opponent->specials.apply_saving_throw[SAVING_SPELL];
      opponent->specials.apply_saving_throw[SAVING_SPELL] += 15;
      spell_curse(60, ch, 0, SPELL_TYPE_SPELL, opponent, 0);
      opponent->specials.apply_saving_throw[SAVING_SPELL] = save;
      break;
    case 11:
      /* poison */
      act("&+GA green cloud shoots forth toward you!&N", FALSE, ch, 0,
          opponent, TO_VICT);
      act("&+GA green cloud shoots forth toward $n!&N", TRUE, opponent, 0, 0,
          TO_ROOM);
      save = opponent->specials.apply_saving_throw[SAVING_SPELL];
      opponent->specials.apply_saving_throw[SAVING_SPELL] += 15;
      spell_poison(30, ch, 0, SPELL_TYPE_SPELL, opponent, 0);
      opponent->specials.apply_saving_throw[SAVING_SPELL] = save;
      break;
    case 4:
    case 9:
    case 13:
      /* drain mana */
      good_evil_procDrain(ch, obj, opponent, &mana);
      break;
    case 5:
      /* group heal */
      for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
      {
        next = tar_ch->next_in_room;
        if (tar_ch->group == ch->group)
        {
          spell_heal(55, ch, 0, 0, tar_ch, 0);
        }
      }
      spell_heal(20, ch, 0, 0, ch, 0);
      break;
    case 6:
      /* fist */
      spell_bigbys_clenched_fist(60, ch, NULL, SPELL_TYPE_SPELL, opponent, 0);
      break;
    case 7:
      /* immolate */
      spell_immolate(60, ch, NULL, 0, opponent, 0);
      break;
    case 8:
      /* earthquake */
      spell_earthquake(60, ch, NULL, SPELL_TYPE_SPELL, NULL, 0);
      break;
    case 10:
      /* group stornog */
      spell_group_stornog(56, ch, 0, 0, ch, NULL);
      break;
    case 3:
      /* hand */
      spell_bigbys_crushing_hand(60, ch, NULL, SPELL_TYPE_SPELL, opponent, 0);
      break;
    case 12:
      /* holy word / unholy word */
      if (isGood)
      {
        if (GET_ALIGNMENT(ch) > -350)
          spell_holy_word(60, ch, NULL, 0, opponent, 0);
      }
      else
      {
        if (GET_ALIGNMENT(ch) < 350)
          spell_unholy_word(60, ch, NULL, 0, opponent, 0);
      }
      break;
    case 14:
      spell_nova(60, ch, 0, 0, NULL, 0);
      break;
    }
  }

  return mana;

}

int good_evil_attemptDefenseProc(P_char ch, P_obj obj, int procMana,
                                 int *swordMana)
{

  if (procMana < *swordMana)
  {
    act("$p &+Wglows brightly for a moment.&N", FALSE, ch, obj, 0, TO_CHAR);
    act("$p &+Wglows brightly for a moment.&N", FALSE, ch, obj, 0, TO_ROOM);
    *swordMana -= procMana;
    obj->value[6] = (obj->value[6] + 1) % 5;
    return TRUE;
  }

  /* Maybe we should punish the wielder here */

  return FALSE;
}

int good_evil_defenseProc(P_char ch, P_obj obj, int isGood, int mana)
{
  int      rand;

  /* PUT IN USEFUL VALUES HERE */
  int      manaCosts[] = {
    30,                         /* gStone / gStornog */
    10,                         /* gHeal */
    10,                         /* gVigCrit */
    10,                         /* gProt cold+fire+acid+gas+lightning */
    10                          /* group armor+bless */
  };

  P_char   tar_ch, next;

  if (number(0, 14))
    return mana;

  if (isGood)
    rand = obj->value[6];
  else
    rand = number(0, 4);

  if (good_evil_attemptDefenseProc(ch, obj, manaCosts[rand], &mana))
  {
    switch (rand)
    {
    case 0:
      /* gStone / gStornog */
      if (!IS_AFFECTED4(ch, AFF4_STORNOGS_SPHERES) || !number(0,3))
      {
        spell_group_stornog(60, ch, 0, 0, ch, NULL);
        break;
      }
      else
      {
        spell_group_stone_skin(45, ch, 0, 0, ch, NULL);
        break;
      }

    case 1:
      /* gHeal */

      // this only "counts" if someone in the group actually needs to be healed!
      for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
      {
        next = tar_ch->next_in_room;
        if ((tar_ch->group == ch->group) && (GET_HIT(tar_ch) < GET_MAX_HIT(tar_ch)))
          break;
      }
      if (tar_ch)
      {
        spell_group_heal(50, ch, 0, 0, ch, 0);
        break;
      }

    case 2:
      /* gVigCrit */
      for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
      {
        next = tar_ch->next_in_room;
        if (tar_ch->group == ch->group)
        {
          spell_vigorize_critic(50, ch, 0, 0, tar_ch, 0);
        }
      }
      break;
    case 3:
      /* gProt cold+fire+acid+gas+lightning */
      for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
      {
        next = tar_ch->next_in_room;
        if (tar_ch->group == ch->group)
        {
          spell_protection_from_cold(50, ch, 0, 0, tar_ch, 0);
          spell_protection_from_fire(50, ch, 0, 0, tar_ch, 0);
          spell_protection_from_acid(50, ch, 0, 0, tar_ch, 0);
          spell_protection_from_gas(50, ch, 0, 0, tar_ch, 0);
          spell_protection_from_lightning(50, ch, 0, 0, tar_ch, 0);
        }
      }
      break;
    case 4:
      /* group armor+bless */
      for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
      {
        next = tar_ch->next_in_room;
        if (tar_ch->group == ch->group)
        {
          spell_armor(50, ch, 0, 0, ch, 0);
          spell_bless(50, ch, 0, 0, ch, 0);
        }
      }
      break;
    }
  }

  return mana;

}

int good_evil_checkHunger(P_char ch, P_obj obj, int mana)
{
  if (mana < 0 && !IS_TRUSTED(ch))      //punishment
  {
    act("$p &+rbecomes hungry and saps some of your strength.&N", FALSE, ch,
        obj, 0, TO_CHAR);
    act("$p &+rbecomes hungry and saps some of $n's strength.&N", FALSE, ch,
        obj, 0, TO_ROOM);
    if (GET_CLASS(ch, CLASS_WARRIOR))
    {
      GET_HIT(ch) >>= 1;
    }
    else
    {
      spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, ch, NULL);
      GET_HIT(ch) -= GET_HIT(ch) / 4;
    }

    mana = GET_MAX_HIT(ch) / 2;
  }

  return mana;

}

int isWieldingVnum(P_char ch, int vnum)
{
  if (!ch)
    return FALSE;

  if (ch->equipment[WIELD] &&
      (obj_index[ch->equipment[WIELD]->R_num].virtual_number == vnum))
    return TRUE;
  if (ch->equipment[WIELD2] &&
      (obj_index[ch->equipment[WIELD2]->R_num].virtual_number == vnum))
    return TRUE;
  if (ch->equipment[WIELD3] &&
      (obj_index[ch->equipment[WIELD3]->R_num].virtual_number == vnum))
    return TRUE;
  if (ch->equipment[WIELD4] &&
      (obj_index[ch->equipment[WIELD4]->R_num].virtual_number == vnum))
    return TRUE;

  return FALSE;
}

void good_evil_spellUp(P_char ch)
{
  /* Put in some cool spells here... deflect and blur for example */
  spell_blur(60, ch, 0, 0, ch, 0);
  spell_deflect(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
}

void good_evil_startBigFight(P_char attacker, P_char defender,
                             int goodieStarted, P_obj obj)
{
  char     buf[MAX_STRING_LENGTH];

  statuslog(AVATAR, "Mayhem and Symmetry are facing off at [%d]!",
            world[attacker->in_room].number);

  /* USE act(...) HERE A FEW TIMES AND PUT IN SOME COOL MESSAGES */
  act
    ("&+WUnimaginable &+WP&+CO&+BW&+bER flows through you as $p &+Rcompels&+L you to strike down its nemesis!&N",
     FALSE, attacker, obj, 0, TO_CHAR);
  send_to_char("&+YYou are compelled to fight without any self control!&N\n",
               attacker);
  if (goodieStarted)
  {
    act("$p &+Wenvelops $n &+Win a magnificent aura as the battle begins!&N",
        FALSE, attacker, obj, 0, TO_ROOM);
    act("$n charges forth and strikes at $N!",
        FALSE, attacker, obj, defender, TO_ROOM);
  }
  else
  {
    act("$p &+renshrouds $n &+rin a menacing aura as the battle begins!&N",
        FALSE, attacker, obj, 0, TO_ROOM);
    act("$n charges forth and strikes at $N!",
        FALSE, attacker, obj, defender, TO_ROOM);
  }

  good_evil_spellUp(attacker);
  good_evil_spellUp(defender);
  obj->value[5] = TRUE;
  if (attacker->specials.fighting != defender)
    stop_fighting(attacker);
  if (defender->specials.fighting != attacker)
    stop_fighting(defender);

   // Original crashes - Clav
  //set_fighting(attacker, defender);
  //set_fighting(defender, attacker);

  attack(attacker, defender);
  attack(defender, attacker);

}

void good_evil_coolDown(P_obj obj, P_char ch)
{
  /* PUT IN SOME COOL DOWN MESSAGE HERE */
  act("&+bColdness seeps into you as $p &+btakes it's toll on you.&N",
      FALSE, ch, obj, 0, TO_CHAR);
  act("$p &+Ldarkens and the &+Waura&+L surrounding $n dies down.&N",
      FALSE, ch, obj, 0, TO_ROOM);
  spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, ch, NULL);
  obj->value[7] = -10000;
  obj->value[5] = FALSE;
}

int killOtherSword(P_obj obj, P_char ch, int isGood)
{

  P_char   opponent;
  int      enemySwordVNum;

  if (!OBJ_WORN(obj))
    return FALSE;

  enemySwordVNum = isGood ? 21 : 22;

  if (IS_FIGHTING(ch) && isWieldingVnum(ch->specials.fighting, enemySwordVNum))
  {  /* already fighting the other sword */
    obj->value[5] = TRUE;
    obj->value[7] = -10000;
    return FALSE;
  }

  LOOP_THRU_PEOPLE(opponent, ch)
    if ((opponent != ch) && (!IS_TRUSTED(opponent)))
    if (isWieldingVnum(opponent, enemySwordVNum))
    {
      good_evil_startBigFight(ch, opponent, isGood, obj);
      return TRUE;
    }

  if (obj->value[5] == TRUE)
    // I WAS in a big fight, but not now.. cool down!
    good_evil_coolDown(obj, ch);
  return FALSE;


}

int attemptToDisengage(P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_KILL ||
      cmd == CMD_HIT ||
      cmd == CMD_INNATE ||
      cmd == CMD_FLEE || cmd == CMD_RESCUE || cmd == CMD_RETREAT)
  {
    send_to_char
      ("&+LYour weapon compels you to continue fighting its nemesis!&N\n",
       ch);
    return TRUE;
  }
  return FALSE;
}

void good_evil_poofSword(P_char ch, P_obj obj)
{

  /* Zap the char and poof */
  act("$p &+Wflares up&n, burns your hands, and vaporizes!&N", FALSE, ch, obj, 0, TO_CHAR);
  act("$p &+Wflares up&n, burns $n's hands, and vaporizes!&N", FALSE, ch, obj, 0, TO_ROOM);
  GET_HIT(ch) -= 100;

  // Don't need to remove from ch anymore, extract_obj handles it.
  extract_obj(obj, TRUE); // Bye arti sword.

/* Old code for putting it in a new room. (tweaked)
  if( OBJ_WORN(obj) )
  {
    for( int i = 0; i < MAX_WEAR; i++ )
    {
      if( ch->equipment[i] == obj )
      {
        unequip_char(ch, i);
        break;
      }
    }
  }
  else if( OBJ_CARRIED(obj) )
  {
    obj_from_char(obj);
  }
  // OBJ_ROOM or OBJ_INSIDE
  else if( !OBJ_NOWHERE(obj) )
  {
    return;
  }
  // Go to any random room in the game not in heaven.
  // Should tweak this to not land on mountains or other !accessible areas.
  obj_to_room(obj, number(zone_table[0].real_top + 1, top_of_world));
*/
}


void good_evil_configSword(P_char ch, P_obj obj)
{
  // configure the sword as 1h or 2h depending on the wielder, and set the dice
  // as follows:
  //  1h 5d5 5/5
  //  2h 6d6 6/6

  // classes which use as a 2h:  paladin, anti-paladin,

  // if the object is already worn, or there's no ch, then don't do anything
  if (OBJ_WORN(obj) || !ch)
    return;

  obj->value[6] = 0;  //  // which "random" effect
  obj->value[5] = FALSE;  // currently in noflee fight

  obj->affected[0].location = APPLY_HITROLL;
  obj->affected[1].location = APPLY_DAMROLL;

  if (GET_CLASS(ch, CLASS_PALADIN) || GET_CLASS(ch, CLASS_ANTIPALADIN) || GET_CLASS(ch, CLASS_AVENGER) || GET_CLASS(ch, CLASS_DREADLORD) || (GET_RACE(ch) == RACE_OGRE) || (GET_RACE(ch) == RACE_MINOTAUR))
  {
    SET_BIT(obj->extra_flags, ITEM_TWOHANDS);
    obj->value[0] = 13;
    obj->value[1] = obj->value[2] = 6; // 6d6
    obj->affected[0].modifier = obj->affected[1].modifier = 6;
    obj->weight = 15;
  }
  else
  {
    REMOVE_BIT(obj->extra_flags, ITEM_TWOHANDS);
    obj->value[0] = 5;
    obj->value[1] = obj->value[2] = 5;  // 5d5
    obj->affected[0].modifier = obj->affected[1].modifier = 5;
    obj->weight = 7;
  }
}

/* 'Mayhem', chaotic sword of the night */
/* 'Symmetry', the uniform sword of light */
int good_evil_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  bool bIsEvil = FALSE, bIsGood = FALSE;
  bool bBumpedOthers = FALSE;
  int  slot, mana, i;
  char tmp_buf[300];
  char curWhisper[300];
  static int num_attacks = 0;
  char **whisperings;
#define SWORD_WHISPERINGS  7
  char *e_whispers[SWORD_WHISPERINGS] =  {
    "I crave blood!",
    "Serve me well, and you shall be given unimaginable power!",
    "This world has not yet seen our true power, show them!",
    "Satisfy my thirst for blood, my minion.",
    "Bring me to Symmetry so that I can absorb its power.",     //5
    "All your base are belong to us!",
    "Slice your enemies apart with me, so that I may absorb their souls."
  };
  char *g_whispers[SWORD_WHISPERINGS] = {
    "Persue the path of goodness always.",
    "Study the way of 'pleasantry' that you may better love the gods.",
    "You must destroy the non-believers.",
    "This world shall be cleansed of evil by our power.",
    "You must vanquish more evil for me to aid you more.",      //5
    "All your base are belong to us!",
    "Do not be swayed to the dark side."
  };

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !obj )
  {
    return FALSE;
  }

  if( 21 == obj_index[obj->R_num].virtual_number )
    bIsEvil = TRUE;
  else if( 22 == obj_index[obj->R_num].virtual_number )
    bIsGood = TRUE;
  else
    return FALSE;

  // wield - if we might be wielding the sword, configure it
  // as required and then return FALSE (acting like we didn't do anything)
  if( cmd == CMD_WIELD && *arg )
  {
    if( !OBJ_WORN(obj) )
    {
      good_evil_configSword(ch, obj);
    }
    // Let the normal wield code deal with it now.
    return FALSE;
  }

  if( OBJ_WORN(obj) && (ch = obj->loc.wearing) )
  {
    if(!IS_SET(obj->extra_flags, ITEM_NODROP))
    {
      SET_BIT(obj->extra_flags, ITEM_NODROP);
    }
    if( !IS_ALIVE(ch) )
    {
      return FALSE;
    }
    good_evil_configSword(ch, obj);
    // if worn, but not as primary weapon, unequip
    if( ch->equipment[PRIMARY_WEAPON] != obj )
    {
      // It ONLY wants to be worn as primary slot!
      bBumpedOthers = TRUE;
      // Figure out what slot they have it in, and remove it!
      for (slot = 0; slot < MAX_WEAR; slot++)
      {
        if (ch->equipment[slot] == obj)
        {
          break;
        }
      }
      if( slot < MAX_WEAR )
      {
        obj_to_char(unequip_char(ch, slot), ch);
      }
    }
  }
  if( OBJ_CARRIED(obj) && IS_ALIVE(obj->loc.carrying) && !IS_TRUSTED(obj->loc.carrying) )
  {
    ch = obj->loc.carrying;
    good_evil_configSword(ch, obj);
    if( ch->equipment[PRIMARY_WEAPON] )
    {
      obj_to_char(unequip_char(ch, PRIMARY_WEAPON), ch);
      bBumpedOthers = TRUE;
    }
    if( ch->equipment[HOLD] )
    {
      obj_to_char(unequip_char(ch, HOLD), ch);
      bBumpedOthers = TRUE;
    }
    if( ch->equipment[WEAR_SHIELD] )
    {
      obj_to_char(unequip_char(ch, WEAR_SHIELD), ch);
      bBumpedOthers = TRUE;
    }
    if( IS_SET(obj->extra_flags, ITEM_TWOHANDS) && ch->equipment[SECONDARY_WEAPON] )
    {
      obj_to_char(unequip_char(ch, SECONDARY_WEAPON), ch);
      bBumpedOthers = TRUE;
    }
    if( bBumpedOthers )
    {
      // send a message :)
      act("Forcing aside your other equipment, $p shoves itself into your hands, ready for battle!", TRUE, ch, obj, NULL, TO_CHAR);
      act("Forcing aside $s other equipment, $p shoves itself $n's hands, ready for battle!", TRUE, ch, obj, NULL, TO_ROOM);
    }
    else
    {
      act("$p shoves itself into your hands, ready for battle!", TRUE, ch, obj, NULL, TO_CHAR);
      act("$p shoves itself $n's hands, ready for battle!", TRUE, ch, obj, NULL, TO_ROOM);
    }
    obj_from_char(obj);
    equip_char(ch, obj, PRIMARY_WEAPON, 0);
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }
  if( cmd == CMD_LOOK )
  {
    if( isname(arg, obj->name) && OBJ_WORN(obj) )
    {
      sprintf(tmp_buf, "&+LHealth remaining:&+r %d&N\n", -(obj->value[7]));
      send_to_char(tmp_buf, ch);
      return TRUE;
    }
  }
  if( !IS_TRUSTED(ch) )
  {
    if( !IS_NPC(ch) && (bIsEvil && RACE_GOOD(ch)) || (bIsGood && RACE_EVIL(ch)) )
    {
      good_evil_poofSword(ch, obj);
    }

    if( killOtherSword(obj, ch, bIsGood) )
    {
      return TRUE;
    }

    // if in "the big fight", prevent attempts to disengage
    if( obj->value[5] && attemptToDisengage(ch, cmd, arg) )
    {
      return TRUE;
    }
  }
  if( cmd == CMD_PERIODIC )
  {
    whisperings = bIsEvil ? e_whispers : g_whispers;
    if( !OBJ_WORN_BY(obj, ch) )
    {
      return FALSE;
    }
    // 1/30 chance.
    if (!number(0, 29))
    {
      sprintf(curWhisper, "$p whispers into your mind '%s&n'", whisperings[number(0, SWORD_WHISPERINGS - 1)]);
      act(curWhisper, FALSE, ch, obj, 0, TO_CHAR);
    }
    good_evil_stoneOrSoulshield(obj);

    if( IS_TRUSTED(ch) )
    {
      mana = 10000;
    }
    else
    {
      mana = -(obj->value[7]);
    }

    if( IS_FIGHTING(ch) )
    {
      mana = good_evil_fightingProc(ch, obj, bIsGood, mana);
    }
    else
    {
      mana = good_evil_defenseProc(ch, obj, bIsGood, mana);
    }

    mana = good_evil_checkHunger(ch, obj, mana);
    if( !IS_TRUSTED(ch) )
    {
      mana -= 3;
    }
    if( mana <= 30 )
    {
      send_to_char("&+LYou feel a strong urge to kill something emanating from your weapon!&N\n", ch);
    }
    obj->value[7] = -mana;
    return TRUE;
  }
  else if( obj->value[5] && (cmd/1000) )
  {
    obj->value[7] = -10000;
    mana = 10000;
    if( !number(0, 3) && !IS_AFFECTED4(ch, AFF4_DEFLECT) )
    {
      spell_deflect(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
    }
    else if( !number(0, 2) )
    {
      obj->value[6] = (obj->value[6] + 1) % 15;
      good_evil_fightingProc(ch, obj, bIsGood, mana);
    }
    else if( !num_attacks && !number(0, 3) )
    {
      num_attacks = number(3, 5);
      act("$p &+Wflares up, slashing your opponent with incredible speed!&n",
          TRUE, ch, obj, ch->specials.fighting, TO_CHAR);
      act("&+W$n's&N $p&+W flares up, slashing $N with incredible speed!&n",
          TRUE, ch, obj, ch->specials.fighting, TO_NOTVICT);
      act("&+W$n's&N $p&+W flares up, slashing YOU with incredible speed!&n",
          TRUE, ch, obj, ch->specials.fighting, TO_VICT);
      for( i = 0; i < num_attacks; i++ )
      {
        if( IS_ALIVE(ch) && IS_ALIVE(ch->specials.fighting) )
        {
          hit(ch, ch->specials.fighting, obj);
        }
      }
    }
  }
  return FALSE;
}


int dranum_mask(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      cur_time;
  P_char   vict;
  struct affected_type *af;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_WORN_POS(obj, WEAR_FACE) || cmd != CMD_PERIODIC || ch )
  {
    return FALSE;
  }
  if( !(ch = obj->loc.wearing) )
  {
    return FALSE;
  }

  // 1/10 chance.
  if( IS_FIGHTING(ch) && !number(0, 9) )
  {
    vict = ch->specials.fighting;
    // no cheesing mobs/dead ppl.
    if( !IS_ALIVE(vict) || IS_NPC(vict) )
    {
      return FALSE;
    }
    act("$p &+Rscares &N&+rthe &+Rliving &+LSHIT &N&+rout of $N!&N", TRUE, ch, obj, vict, TO_CHAR);
    act("&+r$n's&N $p &+Rscares &N&+rthe &+Rliving &+LSHIT &N&+rout of $N!&N", TRUE, ch, obj, vict, TO_NOTVICT);
    act("&+r$n's&N $p &+Rscares &N&+rthe &+Rliving &+LSHIT &N&+rout of YOU!&N", TRUE, ch, obj, vict, TO_VICT);
    if( !fear_check(vict) )
    {
      do_flee(vict, 0, 2);
      return TRUE;
    }
  }
  return FALSE;
}

int sunblade(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time, rand;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC )
  {
    if( OBJ_WORN(obj) && (ch = obj->loc.wearing) )
    {
      curr_time = time(NULL);

      if( !has_skin_spell(ch)
        && obj->timer[0] + (int) get_property("timer.stoneskin.artifact.sunblade", 30) <= curr_time )
      {
        spell_stone_skin(45, ch, 0, SPELL_TYPE_POTION, ch, 0);
        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
    hummer(obj);
    return TRUE;
  }

  if( cmd != CMD_MELEE_HIT || !OBJ_WORN(obj) || !IS_ALIVE(ch) || obj->loc.wearing != ch )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }
  // 4% chance
  if( !number(0, 24) && CheckMultiProcTiming(ch) )
  {
    act("&+r$n's&n $q &+Rexplodes&n&+r in a torrent of pure &+Rflame!&n",
      TRUE, ch, obj, victim, TO_CHAR);
    act("&+r$n's&n $q &+Rexplodes&n&+r in a torrent of pure &+Rflame!&n",
      TRUE, ch, obj, victim, TO_NOTVICT);
    act("&+r$n's&n $q &+Rexplodes&n&+r in a torrent of pure &+Rflame!&n",
      TRUE, ch, obj, victim, TO_VICT);
    spell_sunray(GET_LEVEL(ch), ch, NULL, 0, victim, 0);

  /*
    if(OUTSIDE(ch))
      spell_sunray(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
    else if(number(0, 1))
      spell_solar_flare(GET_LEVEL(ch), ch, NULL, 0, victim, NULL);
    else if(number(0, 1))
      spell_firebrand(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
    else
      spell_immolate(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
   */
    //if(GET_C_LUK(ch) > number(1, 1000))
   // {
      act("$p &+memits a &+Msoft glow&n&+m.&n", TRUE, ch, obj, victim, TO_CHAR);
      act("$p &+memits a &+Msoft glow&n&+m.&n", TRUE, ch, obj, victim, TO_NOTVICT);
      act("$p &+memits a &+Msoft glow&n&+m.&n", TRUE, ch, obj, victim, TO_VICT);
      spell_heal(GET_LEVEL(ch), ch, 0, 0, ch, 0);
  //  }
    return TRUE;
  }
  return FALSE;
}

int vigor_mask(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      cur_time;
  P_char   vict;
  struct affected_type *af;

  if (cmd == CMD_SET_PERIODIC)
  {
    return (TRUE);
  }

  if (!OBJ_WORN_POS(obj, WEAR_FACE))
    return (FALSE);

  if (ch || cmd)
    return (FALSE);

  if (cmd == CMD_REMOVE)
  {
    if (isname(arg, "mask") || isname(arg, "bahamut") || isname(arg, "vigor")
        || isname(arg, "unique"))
    {
      for (af = ch->affected; af; af = af->next)
      {
        if (af->type == SPELL_VITALITY)
        {
          break;
        }
      }
      affect_remove(ch, af);

    }
  }
  ch = obj->loc.wearing;

  if (!number(0, 9) && cmd == CMD_PERIODIC)
  {
    switch (number(0, 9))
    {
    case 0:
    case 1:
    case 2:
      act
        ("&+LA faint silhouette of a dragon briefly surrounds $n, then quickly dissipates.&N",
         TRUE, ch, obj, ch, TO_ROOM);
      send_to_char("&+LYou feel a faint chill run down your spine.&N\n", ch);
      break;
    case 3:
    case 4:
    case 5:
      act
        ("&+LSmall tendrils of smoke seep out of $n's mouth behind the mask.&N",
         TRUE, ch, obj, ch, TO_ROOM);
      send_to_char("&+LSmall tendrils of smoke flick out of your mouth.\n",
                   ch);
      break;
    case 6:
    case 7:
    case 8:
    case 9:
      act
        ("&+L$n's eyes glow briefly for a few seconds, then return to normal.&N",
         TRUE, ch, obj, ch, TO_ROOM);
      send_to_char("&+LYour vision became hazed for a few seconds.\n", ch);
      break;
    default:
      break;
    }
    return TRUE;
  }


  if (IS_FIGHTING(ch) && !number(0, 9) && cmd == 0)
  {
    vict = ch->specials.fighting;
    send_to_char
      ("&+WThe essense of Bahamut streams of of your mask and attacks your victim!&N\n",
       ch);
    if (affected_by_spell(ch, SPELL_VITALITY))
    {
      send_to_char("&+Byou feel revitalized.\n", ch);
      for (af = ch->affected; af; af = af->next)
      {
        if (af->type == SPELL_VITALITY)
        {
          af->duration = 24;
          af->modifier = GET_LEVEL(vict) * 4;
        }
      }
    }
    else
    {
      spell_vitality(GET_LEVEL(vict), ch, NULL, 0, ch, 0);
    }
    if (affected_by_spell(vict, SPELL_VITALITY))
    {
      send_to_char("&+L   /\\             /\\\n", vict);
      send_to_char("&+L   \\ \\           / /\n", vict);
      send_to_char("&+L    | \\  /\\_/\\  / |\n", vict);
      send_to_char("&+L    \\  \\/  _  \\/  /\n", vict);
      send_to_char("&+L     |  \\_/^\\_/  |\n", vict);
      send_to_char("&+L    /   \\  ^  /   \\\n", vict);
      send_to_char("&+L    \\  &+R(\\  &+L^  &+R/)&+L  /\n", vict);
      send_to_char("&+L     \\  &+R\\) &+L^ &+R(/&+L  /\n", vict);
      send_to_char("&+L     /\\    ^    /\\\n", vict);
      send_to_char("&+L     ||\\/\\ ^ /\\/||\n", vict);
      send_to_char("&+L     ||&Nv&+L\\\\&N. .&+L//&Nv&+L||\n", vict);
      send_to_char("&+L     || &Nv&+L\\___/&Nv&+L ||\n", vict);
      send_to_char("&+L     ||  &NvVVVv&+L  ||\n", vict);
      send_to_char("&+L     \\\\&Nn &+L\\   / &Nn&+L//\n", vict);
      send_to_char("&+L      \\\\&Nn &+L||| &Nn&+L//\n", vict);
      send_to_char("&+L       \\\\&Nn&+L\\|/&Nn&+L// &+WT&Nh&+Le Spirit of bahamut leaps at y&No&+Wu\n",
        vict);
      send_to_char("&+L        \\\\&Nnnn&+L//  &+Wa&Nn&+Ld sucks your vitality awa&Ny&+W!\n",
        vict);
      send_to_char("&+L         \\___/&N\n", vict);

      for (af = vict->affected; af; af = af->next)
      {
        if (af->type == SPELL_VITALITY)
        {
          break;
        }
      }
      if (af)
      {
        // will removing vit kill the victim?
        if (af->modifier >= (GET_HIT(vict)+10))
        {
          die(vict, ch);
        }
        else
        {
          affect_remove(vict, af);
          update_pos(vict);
        }
      }
    }
    else
    {
      act
        ("&+W$n's $q glows briefly and uses your essence to &+Brevitalize&N $n!&N",
         TRUE, ch, obj, vict, TO_VICT);

    }
    if (is_char_in_room(vict, ch->in_room))
    {
    act
      ("&+W$q glows for a second and drains the essence of $N.&N\n&+B$n becomes revitalized!&N",
       TRUE, ch, obj, vict, TO_NOTVICT);
    }
  }

  return (FALSE);
}

int mace_of_sea(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }

  act("&+b$n's&N $q &n&+rglows blue...&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+bYour&N $q &n&+rglows blue...&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+b$n's&N $q &n&+rglows blue...&N", TRUE, ch, obj, victim, TO_VICT);
  spell_lightning_bolt(40, ch, NULL, 0, victim, obj);
  return TRUE;
}

int serpent_blade(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }

  act("$p&+L carried by $n &+WLASHES &+Rout as it bites $N!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("$p&+L carried by you &+WLASHES &+Rout as it bites $N!&N", TRUE, ch, obj, victim, TO_CHAR);
  act("$p&+L carried by $n &+WLASHES &+Rout as it bites you!&N", TRUE, ch, obj, victim, TO_VICT);
  spell_poison(40, ch, 0, 0, victim, obj);
  spell_minor_paralysis(40, ch, NULL, 0, victim, obj);
  return TRUE;
}

int kvasir_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  victim = (P_char) arg;
  // 1/30 chance.
  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }

  switch (number(0, 3))
  {
  case 0:
    act("&+WFrost runs down the blade of a&n $q&L&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_NOTVICT);
    act("&+WFrost runs down the blade of a&n $q&L&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_CHAR);
    act("&+WFrost runs down the blade of a&n $q&L&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_VICT);
    spell_cone_of_cold(60, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
    break;

  case 1:
    act("&+WFrost runs down the blade of a&n $q&L&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_NOTVICT);
    act("&+WFrost runs down the blade of a&n $q&L&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_CHAR);
    act("&+WFrost runs down the blade of a&n $q&L&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_VICT);
    spell_ice_storm(60, ch, NULL, 0, victim, obj);
    break;

  case 2:
    act("&+RFlames &+Wcrackle along the blade of a&n $q&L&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_NOTVICT);
    act("&+RFlames &+Wcrackle along the blade of a&n $q&L&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_CHAR);
    act("&+RFlames &+Wcrackle along the blade of a&n $q&L&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_VICT);
    spell_immolate(60, ch, NULL, 0, victim, obj);
    break;

  case 3:
    act("&+RFlames &+Wcrackle along the blade of a&n $q&L&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_NOTVICT);
    act("&+RFlames &+Wcrackle along the blade of a&n $q&L&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_CHAR);
    act("&+RFlames &+Wcrackle along the blade of a&n $q&L&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n", TRUE, ch, obj, victim, TO_VICT);
    spell_firestorm(60, ch, NULL, 0, victim, obj);
    break;

  }
  return TRUE;
}



int lich_spine(P_obj obj, P_char ch, int cmd, char *arg)
{
  int save;
  P_char   victim;

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }
  act("&+b$n's&N $q &n&+rglows with a devilish light...&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+bYour&N $q &n&+rglows with a devilish light...&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+b$n's&N $q &n&+rglows with a devilish light...&N", TRUE, ch, obj, victim, TO_VICT);
  save = victim->specials.apply_saving_throw[SAVING_SPELL];
  victim->specials.apply_saving_throw[SAVING_SPELL] = 20;
  spell_wither(60, ch, NULL, 0, victim, obj);
  victim->specials.apply_saving_throw[SAVING_SPELL] = save;

  return TRUE;
}

int neg_orb(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;
  struct damage_messages messages = {
    "&+WYour&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out toward $N!&n",
    "&+W$n's&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out toward you!&n",
    "&+W$n's&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out toward $N!&n",
    "&+WYour&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out and slays $N!&n",
    "&+W$n's&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out toward you claiming your life!&n",
    "&+W$n's&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out and slays $N!&n",
      0
  };

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }

  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }

  messages.obj = obj;
  spell_damage(ch, victim, 75 * 4, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
  return TRUE;
}

int sanguine(P_obj obj, P_char ch, int cmd, char *argument)
{
  char *arg;
  int   curr_time;


  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !obj || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  // Any powers activated by keywords? Right here, bud.
  if( argument && (cmd == CMD_SAY) )
  {
    arg = argument;

    while (*arg == ' ')
    {
      arg++;
    }

    if (!strcmp(arg, "sanguine"))
    {
      if( !say(ch, arg) )
      {
        return TRUE;
      }
      curr_time = time(NULL);

      // 5 min timer.
      if( obj->timer[0] + 300 <= curr_time )
      {
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_armor(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_bless(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;
      }
      return TRUE;
    }
  }
  return FALSE;
}

// for 51 potions

int treasure_chest(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      found, chance1, chance2;
  P_obj   potion;

  chance1 = 10;
  chance2 = 1;
  found = 0;

  if (cmd == CMD_SET_PERIODIC) {
    return FALSE;
  }

  if (IS_TRUSTED(ch) || !CAN_SEE_OBJ(ch, obj)) {
    return FALSE;
  }
  if (!ch || !obj) {
    return FALSE;
  }

  if (number(0, 100) < chance1) {
    found++;

    if (number(0, 100) < chance2) {
      found++;
    }
  }


/*
   Any powers activated by keywords? Right here, bud.
 */
  if (cmd == CMD_OPEN)
  {

     if (number(0, 100) < chance1) {
       found++;

       if (number(0, 100) < chance2) {
         found++;
       }
     }

     if (found) {


     act("$n opens the &+Ytreasure &+ychest&n and finds something!&n", FALSE, ch, obj,
        NULL, TO_NOTVICTROOM);
     act("Your open the &+Ytreasure &+ychest&n and find something...&n", FALSE, ch, obj, NULL,
        TO_CHAR);

       while (found > 0) {

         potion = read_object(51004, VIRTUAL);

         obj_to_char(potion, ch);
         act("... &+Wa glowing potion!&n", FALSE, ch, obj, NULL, TO_CHAR);
         found--;
       }

     }


     act("&+yA &+Ytreasure &+ychest crumbles into &ndust.", FALSE, ch, obj, NULL, TO_CHAR);
     act("&+yA &+Ytreasure &+ychest crumbles into &ndust.", FALSE, ch, obj, NULL, TO_NOTVICTROOM);
     obj_from_room(obj);
     return TRUE;
  }

  return FALSE;
}


int demo_scimitar(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  // 1/25 chance.
  if( !IS_ALIVE(victim) || number(0,24) )
  {
    return FALSE;
  }

  act("$p &+Lcarried by $n&+L slices into $N's&+L soul...&n", TRUE, ch, obj, victim, TO_NOTVICT);
  act("Your $q &+Lslices into $N's&+L soul...&n", TRUE, ch, obj, victim, TO_CHAR);
  act("$p &+Lcarried by $n&+L slices into your&+L soul...&n", TRUE, ch, obj, victim, TO_VICT);

  GET_VITALITY(victim) = MAX(0, GET_VITALITY(victim) - (number(10, 40)));

  act("&+L$N&+L goes limp for a moment.&n", FALSE, ch, 0, victim, TO_NOTVICT);
  act("&+LYou feel your body&+L go limp for a moment.&n", FALSE, ch, 0, victim, TO_VICT);
  act("&+L$N&+L goes limp for a moment.&n", FALSE, ch, 0, victim, TO_CHAR);

  return TRUE;
}

int orb_of_destruction(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }

  // 1/16 chance.
  if( !(victim = (P_char) arg) || number(0, 15) )
  {
    return FALSE;
  }

  act("&+L$n's&N $q &n&+bdraws &+Benergy&n&+b from the surroundings and &+LBLASTS $N!&N",
    TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+LYour&N $q &n&+bdraws &+Benergy&n&+b from the surroundings and &+LBLASTS $N!&N",
    TRUE, ch, obj, victim, TO_CHAR);
  act("&+L$n's&N $q &n&+bdraws &+Benergy&n&+b from the surroundings and &+LBLASTS YOU!&N",
    TRUE, ch, obj, victim, TO_VICT);
  spell_disintegrate(40, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  return TRUE;
}

int labelas(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;
  int      room;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }
  if( !IS_ALIVE(ch) || (!arg && (cmd != CMD_CD)) )
  {
    return FALSE;
  }
  if( (cmd != CMD_SACRIFICE) && (cmd != CMD_TERMINATE) && (cmd != CMD_CD) )
  {
    return FALSE;
  }
  one_argument(arg, Gbuf1);
  if( (!*Gbuf1) && (cmd != CMD_CD) )
  {
    send_to_char("Who?\n", ch);
    return TRUE;
  }
  if( !OBJ_WORN_BY(obj, ch) || obj->loc.wearing->equipment[HOLD] != obj )
  {
    return FALSE;
  }
  if( cmd != CMD_CD && !(victim = get_char_room_vis(ch, Gbuf1)) )
  {
    send_to_char("Who?\n", ch);
    return (TRUE);
  }
/*
   if (str_cmp(ch->player.name, "Labelas")) {
 */
  if( isname(ch->player.name, obj->name) )
  {
    if( IS_PC(ch) )
    {
      act("&+gYou feel a wave of holy justice wash over you as the staff &+Wglows.", FALSE, ch, 0, 0, TO_CHAR);
      act("&+gThe Staff flies out of your hand and drives its sharp end into your chest!", FALSE, ch, 0, 0, TO_CHAR);
      act("&+g$n attempts to use the Staff of the Implementors!", TRUE, ch, 0, 0, TO_ROOM);
      act("&+gThe Staff &+Wglows&+g with power as it spins in the air and drives its pointed end into $n's chest!", TRUE, ch, 0, 0, TO_ROOM);

      statuslog(ch->player.level, "%s killed while trying to use the someone elses Implementor staff.", GET_NAME(ch));
      logit( LOG_WIZ, "%s killed while trying to use the Staff of Labelas.", GET_NAME(ch) );
      die(ch, ch);
      return TRUE;
    }
    else
    {
      send_to_char("Monsters can't use the Staff of Labelas!\n", ch);
      return (TRUE);
    }
  }
  if( cmd == CMD_CD )
  {
    if( !str_cmp(ch->player.name, "Labelas") )
    {
      if( !*Gbuf1 )
      {
        room = real_room(1213);
      }
      else if( is_number(Gbuf1) )
      {
        room = real_room(atoi(Gbuf1));
        if( room == NOWHERE || room > top_of_world )
        {
          send_to_char("No room exists with that number.\n", ch);
          return TRUE;
        }
      }
      else if( (victim = get_char_vis(ch, Gbuf1)) )
      {
        room = victim->in_room;
      }
      else
      {
        send_to_char("Sorry, I know of no one by that name!\n", ch);
        return TRUE;
      }
    }
    else
    {
      return FALSE;
    }

    act("&+g$n holds up $s staff and utters an arcane magical phrase.", TRUE, ch, 0, 0, TO_ROOM);
    act("&+g$n steps into a sphere of time and is gone...", TRUE, ch, 0, 0, TO_ROOM);

    char_from_room(ch);
    char_to_room(ch, room, -1);

    act("&+WA cold wind suddenly blows through the area, chilling you to the bone.", TRUE, ch, 0, 0, TO_ROOM);
    act("&+gFOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOSSSHHHHH!", TRUE, ch, 0, 0, TO_ROOM);
    act("&+gA huge twenty-foot-high shadow forms out of the ground, looming above you.", TRUE, ch, 0, 0, TO_ROOM);
    act("&+gIts form twists and contorts into the shape of Labelas Enoreth!", TRUE, ch, 0, 0, TO_ROOM);
    act("&+gThe Elven god Labelas Enoreth stands before you in all his majesty!", TRUE, ch, 0, 0, TO_ROOM);
    act("&+gThe winds fade...", TRUE, ch, 0, 0, TO_ROOM);

    return TRUE;
  }
  if( !(victim = get_char_room_vis(ch, Gbuf1)) )
  {
    send_to_char("Use the Staff on whom?\n", ch);
    return TRUE;
  }
  if( ch == victim )
  {
    send_to_char("Don't use the Staff on yourself!\n", ch);
    return TRUE;
  }
  if( IS_NPC(victim) )
  {
    send_to_char("Don't use the Staff on NPC's, okay?\n", ch);
    return TRUE;
  }
  act("&+gYou raise the staff high into the air, and strike at $N.", FALSE, ch, 0, victim, TO_CHAR);
  act("&+gThe staff comes to life, moving with your hand at blinding speed", FALSE, ch, 0, victim, TO_CHAR);
  act("&+gas if it had a hunger of its own for $N's heart!", FALSE, ch, 0, victim, TO_CHAR);

  act("&+g$n raises $s staff higher into the air and strikes at you!", FALSE, ch, 0, victim, TO_VICT);
  act("&+gThe staff comes to life, freezing you where you stand in utter terror!", FALSE, ch, 0, victim, TO_VICT);
  act("&+gIt sings through the air towards your chest, craving your heart.", FALSE, ch, 0, victim, TO_VICT);
  act("&+gYou cannot move as the staff strikes your chest with a sickening thud.", FALSE, ch, 0, victim, TO_VICT);
  act("&+gYou can hear your ribs crunch and break under the weapon's awesome power,", FALSE, ch, 0, victim, TO_VICT);
  act("&+gYou feel a horrid ripping sensation and scream in uncontrollable agony!", FALSE, ch, 0, victim, TO_VICT);
  act("&+gThe staff wrenches free of you, your still-beating heart impaled on its sharp end.", FALSE, ch, 0, victim, TO_VICT);
  act("&+LBlackness falls over you, and you fall to the ground.  You can literally", FALSE, ch, 0, victim, TO_VICT);
  act("&+Lfeel your lifeforce flow into $n, sacrificed to the mighty Labelas.", FALSE, ch, 0, victim, TO_VICT);

  act("&+g$n raises $s staff into the air and strikes at $N!", TRUE, ch, 0, victim, TO_NOTVICT);
  act("&+gThe staff comes alive with a magical life of its own, and sings through", TRUE, ch, 0, victim, TO_NOTVICT);
  act("&+gthe air towards $N, who cowers in utter fear and is frozen in place.", TRUE, ch, 0, victim, TO_NOTVICT);
  act("&+gThe staff strikes $S chest with a sickening thud. The sound of crunching", TRUE, ch, 0, victim, TO_NOTVICT);
  act("&+gribs and a horrible scream fill the air. The staff wrenches itself", TRUE, ch, 0, victim, TO_NOTVICT);
  act("&+gfree of $S chest, with $S still-beating heart on its sharp end.", TRUE, ch, 0, victim, TO_NOTVICT);
  act("&+g$N's lifeless body falls to the ground, $S face locked in a", TRUE, ch, 0, victim, TO_NOTVICT);
  act("&+ghorrible expression as $S lifeforce is sacrificed to the mighty Labelas.", TRUE, ch, 0, victim, TO_NOTVICT);

  if( cmd == CMD_SACRIFICE )
  {
    if( IS_PC(victim) )
    {
      statuslog(ch->player.level, "%s was sacrificed to Labelas.", GET_NAME(victim));
      logit(LOG_WIZ, "%s was sacrificed to Labelas.", GET_NAME(victim));
    }
    die(victim, ch);
    return (TRUE);
  }
  else if( cmd == CMD_TERMINATE )
  {
    if( IS_NPC(victim) )
    {
      logit(LOG_WIZ, "%s was sacrificed to Labelas.", GET_NAME(victim));
      die(victim, ch);
      send_to_char("You can't terminate mobs!!  Sacrificed instead.\n", ch);
      return TRUE;
    }
    if( GET_LEVEL(victim) > GET_LEVEL(ch) )
    {
      send_to_char("Now, now, don't try to terminate your superiors!\n", ch);
      return TRUE;
    }
    act(".", FALSE, ch, 0, victim, TO_CHAR);
    act(".", FALSE, ch, 0, victim, TO_CHAR);
    act("&+gYou call the pure might of the Forgers down upon $N", FALSE, ch, 0, victim, TO_CHAR);
    act("&+gWith great magic, you devour $N's soul completely and utterly, ", FALSE, ch, 0, victim, TO_CHAR);
    act("&+gobliterating $M from this world of Duris, forever...", FALSE, ch, 0, victim, TO_CHAR);
    act("&+gThe world stands in awe of your awesome power... ;)", FALSE, ch, 0, victim, TO_CHAR);

    act(".", FALSE, ch, 0, victim, TO_NOTVICT);
    act(".", FALSE, ch, 0, victim, TO_NOTVICT);
    act("&+g$n raises $s hand, and calls down the might of the Forgers on $N.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("&+g$n slowly devours $N's soul, utterly obliterating $S from this world forever.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("&+gYou can hear $N's soul scream in agony one last time, then fade on the winds....", FALSE, ch, 0, victim, TO_NOTVICT);

    act(".", FALSE, ch, 0, victim, TO_VICT);
    act(".", FALSE, ch, 0, victim, TO_VICT);
    act("&+g$n raises $s hand, and calls down the might of the Forgers upon you!", FALSE, ch, 0, victim, TO_VICT);
    act("&+g$n slowly devours your soul as it rises out of your dead body.", FALSE, ch, 0, victim, TO_VICT);
    act("&+WAAAAAAAAAAHHHHHHH!!!! The pain is agonizing, though there is no escape.", FALSE, ch, 0, victim, TO_VICT);
    act("&+WYour soul is destroyed utterly, forever obliterated from this world...", FALSE, ch, 0, victim, TO_VICT);

    statuslog(ch->player.level, "%s's soul was just utterly destroyed by the power of Labelas!", GET_NAME(victim));
    logit(LOG_WIZ, "%s was terminated by the power of Labelas.", GET_NAME(victim));
    if( victim->desc )
    {
      victim->desc->connected = CON_DELETE;
    }
    extract_char(victim);

    return TRUE;
  }
  logit(LOG_DEBUG, "Somehow reached the end of labelas()");
  return FALSE;
}

// Chaos/Ambran/Death Rider
int holy_weapon(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   tch, vict = NULL;
  struct proc_data *data = NULL;
  struct affected_type aff, *af;
  bool     should_jump;
  int      pcnt = 100, curr_time, tmpper, alignment; // 0 == good, 1 == evil, -1 == neutral
  struct damage_messages goodmessages = {
    "&+wThe power of your &+WGod&+w rains down pain and suffering upon $N&+w!&n",
    "&+wPain unlike you have ever felt before permeates your body.&n",
    "&+w$N &+wscreams in utter terror as he is judged before $n&+w's &+WGod&+w.&n",
    "&+w$N &+wfalls to the ground, their soul a mere shell of what it once was.&n",
    "&+WJudgement &+wis rendered, as you feel your soul being shattered to pieces.&n",
    "&+w$N &+wfalls to the ground, their soul a mere shell of what it once was.&n.",
      0
  };
  struct damage_messages evilmessages = {
    "&+LThe power of your &+RGod&+L rains down pain and suffering upon $N&+L!&n",
    "&+LPain unlike you have ever felt before permeates your body.&n",
    "&+L$N &+Lscreams in utter terror as he is damned by $n&+L's &+RGod&+L.&n",
    "&+L$N &+Lfalls to the ground, their soul a mere shell of what it once was.&n",
    "&+RDamnation &+Lis rendered, as you feel your soul being shattered to pieces.&n",
    "&+L$N &+Lfalls to the ground, their soul a mere shell of what it once was.&n.",
      0
  };

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!obj)
    return FALSE;

  switch (obj_index[obj->R_num].virtual_number)
  {
    case 425:  // Ambran
      alignment = 0;
      break;
    case 51005:  // Death Rider
      alignment = 1;
      break;
    default:
      alignment = -1;
      break;
  }

  if((cmd == CMD_REMOVE) && arg )
  {
    if( isname(arg, obj->name) || isname(arg, "all") )
    {
      if (affected_by_spell(ch, TAG_HOLY_OFFENSE))
        affect_from_char(ch, TAG_HOLY_OFFENSE);
      if (affected_by_spell(ch, TAG_HOLY_DEFENSE))
        affect_from_char(ch, TAG_HOLY_DEFENSE);
    }
  }

  if (OBJ_WORN(obj))
  {
     if(cmd == CMD_SAY && arg)
     {
        if(isname(arg, "attack"))
        {
           af = get_spell_from_char(ch, TAG_HOLY_OFFENSE);
           if (!af)
           {
              memset(&aff, 0, sizeof(aff));
              aff.type = TAG_HOLY_OFFENSE;
              aff.flags = AFFTYPE_NODISPEL;
              aff.modifier = 12;
              aff.location = APPLY_STR_MAX;
              aff.duration = -1;
              affect_to_char(ch, &aff);
              aff.type = TAG_HOLY_OFFENSE;
              aff.flags = AFFTYPE_NODISPEL;
              aff.modifier = 12;
              aff.location = APPLY_DEX_MAX;
              aff.duration = -1;
              affect_to_char(ch, &aff);
              aff.type = TAG_HOLY_OFFENSE;
              aff.flags = AFFTYPE_NODISPEL;
              aff.modifier = -1;
              aff.location = APPLY_COMBAT_PULSE;
              aff.duration = -1;
              affect_to_char(ch, &aff);
              act("&+LAs you utter the word, a chill runs through your body as power takes hold...", FALSE, ch, obj, 0, TO_CHAR);
              act("$n &+wwhispers something to $s $q&+w, and &+Lshudders &+wwith new power...", FALSE, ch, obj, 0, TO_ROOM);
              if(af = get_spell_from_char(ch, TAG_HOLY_DEFENSE))
                 affect_from_char(ch, TAG_HOLY_DEFENSE);
              CharWait(ch, 2 * PULSE_VIOLENCE);
              return TRUE;
           }
        }
        else if(isname(arg, "defend"))
        {  
           af = get_spell_from_char(ch, TAG_HOLY_DEFENSE);
           if (!af)
           {
               memset(&aff, 0, sizeof(aff));
               aff.type = TAG_HOLY_DEFENSE;
               aff.flags = AFFTYPE_NODISPEL;
               aff.modifier = 15;
               aff.location = APPLY_AGI_MAX;
               aff.duration = -1;
               affect_to_char(ch, &aff);
               aff.type = TAG_HOLY_DEFENSE;
               aff.flags = AFFTYPE_NODISPEL;
               aff.modifier = -8;
               aff.location = APPLY_SAVING_PARA;
               aff.duration = -1;
               affect_to_char(ch, &aff);
               aff.type = TAG_HOLY_DEFENSE;
               aff.flags = AFFTYPE_NODISPEL;
               aff.modifier = -8;
               aff.location = APPLY_SAVING_SPELL;
               aff.duration = -1;
               affect_to_char(ch, &aff);
               aff.type = TAG_HOLY_DEFENSE;
               aff.flags = AFFTYPE_NODISPEL;
               aff.modifier = -8;
               aff.location = APPLY_SAVING_BREATH;
               aff.duration = -1;
               affect_to_char(ch, &aff);
               aff.type = TAG_HOLY_DEFENSE;
               aff.flags = AFFTYPE_NODISPEL;
               aff.modifier = 1;
               aff.location = APPLY_COMBAT_PULSE;
               aff.duration = -1;
               affect_to_char(ch, &aff);
               act("&+WAs you utter the word, a chill runs through your body as power takes hold...", FALSE, ch, obj, 0, TO_CHAR);
               act("$n &+wwhispers something to $s $q&+w, and &+Wshudders &+wwith new power...", FALSE, ch, obj, 0, TO_ROOM);
               if(af = get_spell_from_char(ch, TAG_HOLY_OFFENSE))
                 affect_from_char(ch, TAG_HOLY_OFFENSE);
               CharWait(ch, 2 * PULSE_VIOLENCE);
               return TRUE;
           }
        }
     }
  } 

  if(cmd == CMD_MELEE_HIT && !number(0, 25) && CheckMultiProcTiming(ch))
  {
    vict = (P_char) arg;
    
    if (!vict || !IS_ALIVE(vict))
      return (FALSE);

    if (GET_RACEWAR(ch) == RACEWAR_GOOD)
    {
       act("$n's $q &+Wflares with pure light, unleashing the virtue of the gods at $N!&n",
        TRUE, ch, obj, vict, TO_NOTVICT);
       act("Your $q &+Wflares with pure light, unleashing the virtue of the gods at $N!&n",
        TRUE, ch, obj, vict, TO_CHAR);
       act("$n's $q &+Wflares with pure light, unleashing the virtue of the gods at _YOU_!&n",
        TRUE, ch, obj, vict, TO_VICT);
       spell_damage(ch, vict, 360, SPLDAM_HOLY, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &goodmessages);
       if(GET_C_LUK(ch) > number(0, 500))
       {
        spell_mending(51, ch, 0, 0, ch, 0);
       }
    }
    else if (GET_RACEWAR(ch) == RACEWAR_EVIL)
    {
       act("$n's $q &+Lflares with darkness, unleashing the wrath of the underworld upon $N!&n",
        TRUE, ch, obj, vict, TO_NOTVICT);
       act("Your $q &+Lflares with darkness, unleashing the wrath of the underworld upon $N!&n",
        TRUE, ch, obj, vict, TO_CHAR);
       act("$n's $q &+Lflares with darkness, unleashing the wrath of the underworld upon _YOU_!&n",
        TRUE, ch, obj, vict, TO_VICT);
       spell_damage(ch, vict, 360, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &evilmessages);
       if(GET_C_LUK(ch) > number(0, 500))
       {
         spell_malison(56, ch, 0, 0, vict, 0);
       }
    }
    return TRUE;
  }  

  if (cmd == CMD_PERIODIC)
  { 
     if(OBJ_WORN(obj))                  //  This periodically checks to see if
       ch = obj->loc.wearing;              //  to see if item is in the proper racewar hands
     else if(OBJ_CARRIED(obj))
       ch = obj->loc.carrying;
     else
       return FALSE;

     switch(alignment)
     {
       case 0:
         should_jump = !RACE_GOOD(ch);
         break; 
       case 1:
         should_jump = !RACE_EVIL(ch);
         break;
       default:
         should_jump = false;
     }

     if(IS_TRUSTED(ch))
       should_jump = false;

     if(!IS_PC(ch))
       should_jump = false;

     if(should_jump)
     {
         if (OBJ_WORN(obj))
           obj_to_char(unequip_char(ch, (ch->equipment[WIELD] == obj) ? WIELD : 
             (ch->equipment[SECONDARY_WEAPON] == obj) ? SECONDARY_WEAPON : 
               (ch->equipment[HOLD] == obj) ? HOLD : 0), ch);
         obj_from_char(obj);

       for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
       {
         if (IS_PC(tch) && opposite_racewar(ch, tch))
         {
           act("$p screams in outrage at your touch!", FALSE, ch, obj, 0, TO_CHAR);
           act("$p screams in outrage at $n's touch!", FALSE, ch, obj, 0, TO_ROOM);
           act("$p &=LCshimmers&n, blasts you with power and leaps to $N!", FALSE, ch, obj, tch, TO_CHAR);
           act("$p &=LCshimmers&n and blasts $n as it leaps to $N!", FALSE, ch, obj, tch, TO_NOTVICT);
           act("$p &=LCshimmers&n and blasts $n as it leaps to you!", FALSE, ch, obj, tch, TO_VICT);
           spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, ch, 0);
           obj_to_char(obj, tch);
           break;
         }
       }
       if (!tch)
       {
         act("$p &=LCshimmers&n, blasts you with power and vanishes from your hand!", FALSE, ch, obj, 0, TO_CHAR);
         act("$p &=LCshimmers&n and blasts $n before vanishing!", FALSE, ch, obj, 0, TO_ROOM);
         spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, ch, 0);
         extract_obj(obj, TRUE); // Bye arti sword.
       }
       return TRUE;
     }

     if (OBJ_WORN(obj) && !number(0, 10))
     {
       hummer(obj);
       for( struct group_list *tgl = ch->group; tgl && tgl->ch; tgl = tgl->next )
       {
 	    if(tgl->ch->in_room != ch->in_room) 
           continue;
 	
         if(!affected_by_spell(tgl->ch, SPELL_ARMOR))
           spell_armor(60, ch, 0, 0, tgl->ch, 0);
 
         if(!affected_by_spell(tgl->ch, SPELL_BLESS))
           spell_bless(60, ch, 0, 0, tgl->ch, 0);
       }
       return TRUE;
     }
     
    // Following proc finds the most hurt casting ally that is tanking and rescues them - Jexni 12/2/10

     if(IS_FIGHTING(ch) && OBJ_WORN(obj))
     {
        for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
        {
           if(grouped(ch, tch) && IS_FIGHTING(tch) && (tch != ch) && IS_CASTER(tch))
           {
             tmpper = (GET_HIT(tch) / GET_MAX_HIT(tch)) * 100;
             if(tmpper < pcnt)
             {
               vict = tch;
               pcnt = tmpper;
             }
           }      
        }
        for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
        {
           if(tch->specials.fighting == vict)
             break;
        }
        if(vict)
        {      
           if(CanDoFightMove(ch, tch))
           {
              if(alignment = 0)
              {
                 act("Your $q &+Wglows &+was you slam the pommel into $N&+w, &+wknocking $M away from your ally!", FALSE, ch, obj, tch, TO_CHAR);
                 act("$p &+Wglows &+was its pommel is slammed into $N&+w, &+wknocking $M away from YOU!", FALSE, vict, obj, tch, TO_CHAR);
                 act("$n&+w's $q &+Wglows &+was its pommel smashes into you, &+wknocking you off-balance and back several steps!", FALSE, ch, obj, tch, TO_VICT);
              }
              else
              {
                 act("&+LYour $q &+Rglows &+Las you slam the pommel into $N&+L, &+Lknocking $M away from your ally!", FALSE, ch, obj, tch, TO_CHAR);
                 act("$p &+Rglows &+Las its pommel is slammed into $N&+L, &+Lknocking $M away from YOU!", FALSE, vict, obj, tch, TO_CHAR);
                 act("$n&+L's $q &+Rglows &+Las its pommel smashes into you, &+Lknocking you off-balance and back several steps!", FALSE, ch, obj, tch, TO_VICT);
              }
              if(vict->specials.fighting == tch)
                stop_fighting(vict);
              if(!IS_FIGHTING(ch))
                set_fighting(ch, tch);
              if(IS_FIGHTING(tch))
              {
                stop_fighting(tch);
                set_fighting(tch, ch);
              }
           }
        }
        return TRUE;
     }
  }
  return FALSE;

/* Rewrote holy_weapon proc to be defensive or offensive by choice of wielder - Jexni 11/30/10
  if(cmd == CMD_MELEE_HIT && !number(0, 24) && CheckMultiProcTiming(ch))
  {
    vict = (P_char) arg;

    if( !vict )
      return FALSE;

    if( alignment == 0 )
    {
      act("Your $p&n fills you with &+WHOLY&n power!", FALSE, ch, obj, 0, TO_CHAR);
      act("$n's $p&n fills $m with &+WHOLY&n power!", FALSE, ch, obj, 0, TO_ROOM);
    }
    else
    {
      act("Your $p&n fills you with &+LUNHOLY&n power!", FALSE, ch, obj, 0, TO_CHAR);
      act("$n's $p&n fills $m with &+LUNHOLY&n power!", FALSE, ch, obj, 0, TO_ROOM);
    }

    for( int i = 0; (i < 2) && (GET_STAT(vict) != STAT_DEAD); i++ )
      spell_full_harm(60, ch, 0, 0, vict, obj);

    return TRUE;
  }

  if(cmd == CMD_GOTNUKED &&
    !number(0, 9))
  {

    data = (struct proc_data *) arg;
    vict = data->victim;

    if( !vict )
      return FALSE;

    act("&+L$N&+L's $q &+Labsorbs $n&+L's spell!", FALSE, vict, obj, ch, TO_NOTVICT);
    act("&+LYour spell is absorbed by $N&+L's $q&+L!", FALSE, vict, obj, ch, TO_CHAR);
    act("&+L$n&+L's spell is absorbed by your $q&+L!", FALSE, vict, obj, ch, TO_VICT);

    if( alignment == 0 && ( IS_EVIL(vict) || RACE_EVIL(vict) ) )
    {
      act("You are filled with &+WHOLY&n power!", FALSE, ch, obj, 0, TO_CHAR);
      act("$n is filled with &+WHOLY&n power!", FALSE, ch, obj, 0, TO_ROOM);
      spell_holy_word(60, ch, NULL, 0, vict, 0);
    }
    else if( alignment == 1 && ( IS_GOOD(vict) || RACE_GOOD(vict) ) )
    {
      act("You are filled with &+LUNHOLY&n power!", FALSE, ch, obj, 0, TO_CHAR);
      act("$n is filled with &+LUNHOLY&n power!", FALSE, ch, obj, 0, TO_ROOM);
      spell_unholy_word(60, ch, NULL, 0, vict, 0);
    }

    return TRUE;
  }

*/
}

int menden_figurine(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   tempchar = NULL;
  int      i, pos = -1;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !OBJ_WORN(obj) || !IS_ALIVE(ch) || !AWAKE(ch) )
  {
    return FALSE;
  }
  if( obj->loc.wearing != ch )
  {
    logit(LOG_DEBUG, "Buggy equip in figurine(): obj->loc.wearing != ch.");
    return FALSE;
  }

  // Item must be held to work.
  if( !OBJ_WORN_POS(obj, HOLD) && !OBJ_WORN_POS(obj, WIELD) )
  {
    return FALSE;
  }

  if (cmd == CMD_FLEX)
  {
    argument_interpreter(arg, Gbuf1, Gbuf2);

    if( isname(Gbuf1, obj->name) )
    {
      // Load depicted mob
      tempchar = read_mobile(obj->value[0], VIRTUAL);

      if( !tempchar )
      {
        logit( LOG_DEBUG, "menden_figurine(): mob %d not loadable", obj->value[0] );
        logit( LOG_SYS, "read_mobile(): failed to load" );
        return FALSE;
      }
      char_to_room(tempchar, ch->in_room, -1);
      unequip_char(obj->loc.wearing, pos);

      act("You flex $p several times, until it finally snaps!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n flexes $p several times, until it finally snaps!", TRUE, ch, obj, 0, TO_NOTVICT);
      act("From the $o come swirling vapors, which solidify to form $n.", TRUE, tempchar, obj, 0, TO_ROOM);
      act("The $o rapidly disintegrates to powder, only to be born away by a sudden wind.", TRUE, tempchar, obj, 0, TO_ROOM);

      add_follower(tempchar, ch);
      setup_pet(tempchar, ch, 10, PET_NOCASH);

      extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      return TRUE;
    }
  }
  return FALSE;
}

int die_roller(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      i, pos = -1, numb;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || cmd == CMD_PERIODIC || !AWAKE(ch))
    return (FALSE);

  if (!OBJ_WORN(obj))
    return (FALSE);

  if (obj->loc.wearing != ch)
  {
    logit(LOG_DEBUG, "Buggy equip in die_roller(): obj->loc.wearing != ch.");
    return (FALSE);
  }
  /*
     item must be held to work -- can modify
   */
  for (i = 0; i < MAX_WEAR; i++)
  {
    if (obj->loc.wearing->equipment[i] == obj)
    {
      pos = i;
      break;
    }
  }

  /*
     temporary kludge until we get secondary-hold as we should
   */
  if ((pos != HOLD) && (pos != WIELD))
    return (FALSE);

  if (cmd == CMD_ROLL)
  {                             /*
                                   roll
                                 */
    argument_interpreter(arg, Gbuf1, Gbuf2);
    /*
       item must be held to work
     */
    if (isname(Gbuf1, obj->loc.wearing->equipment[pos]->name))
    {

      /*
         roll the damn die
       */
      if (obj->value[0] <= 0)
      {
        send_to_char("error in die..  tell somebody.\n", ch);
        return TRUE;
      }

      numb = number(1, obj->value[0]);

      obj_to_room(unequip_char(ch, pos), ch->in_room);

      sprintf(Gbuf1, "Tossing the $q&n onto the ground, you roll a %u.",
              numb);
      sprintf(Gbuf2, "Tossing $p&n onto the ground, $n rolls a %u.", numb);

      act(Gbuf1, FALSE, ch, obj, 0, TO_CHAR);
      act(Gbuf2, FALSE, ch, obj, 0, TO_ROOM);

      return (TRUE);
    }
  }

  return (FALSE);
}


#if 0

int skeleton_key(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      door, other_room, rand, i, pos = -1, break_percent;
  char     type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  P_obj    target_o;
  P_char   victim;

  // Check for periodic event calls
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  /*
     no object? no character? not "unlock"? not awake? -- do nothing
   */
  if ((!obj) || (!ch) || (cmd != CMD_UNLOCK) || !AWAKE(ch))
    return (FALSE);

  // Must be equipped
  if (!OBJ_WORN(obj))
    return (FALSE);

  /*
     must be equipped by ch
   */
  if (obj->loc.wearing != ch)
  {
    logit(LOG_DEBUG, "Buggy equip in skel_key(): obj->loc.wearing != ch.");
    return (FALSE);
  }
  /*
     must be held to work -- can modify
   */
  for (i = 0; i < MAX_WEAR; i++)
    if (obj->loc.wearing->equipment[i] == obj)
      pos = i;

  if (pos != HOLD)
    return (FALSE);

  argument_interpreter(arg, type, dir);
  if (!*type)
    return (FALSE);
  // Don't include tracks.
  else if (generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &victim, &target_o))
  {

    /*
       target is an object
     */

    if (((target_o->type != ITEM_CONTAINER) &&
         (target_o->type != ITEM_QUIVER)) ||
        (!IS_SET(target_o->value[1], CONT_CLOSED)) || (target_o->value[2] < 0)
        || (has_key(ch, target_o->value[2])) ||
        (!IS_SET(target_o->value[1], CONT_LOCKED)))
    {

      /*
         let do_unlock deal with these cases
       */
      return (FALSE);

    }
    else if (IS_SET(target_o->value[1], CONT_PICKPROOF))
    {

      /*
         broke key
       */
      act("Your clumsiness caused $p to break into several pieces!", TRUE, ch,
          obj, 0, TO_CHAR);
      act("$n's clumsiness caused $p to break into several pieces!", TRUE, ch,
          obj, 0, TO_ROOM);

      /*
         remove key
       */
      unequip_char(obj->loc.wearing, pos);
      extract_obj(obj, TRUE); // Not an arti, but 'in game.'

      return (TRUE);

    }
    else
    {

      break_percent = 50 - dex_app[STAT_INDEX(GET_C_DEX(ch))].p_locks;

      /*
         random chance of breaking key
       */
      if ((rand = number(1, 100)) <= break_percent)
      {

        /*
           broke key
         */
        act("Your clumsiness caused $p to break into several pieces!", TRUE,
            ch, obj, 0, TO_CHAR);
        act("$n's clumsiness caused $p to break into several pieces!", TRUE,
            ch, obj, 0, TO_ROOM);

        /*
           remove key
         */
        unequip_char(obj->loc.wearing, pos);
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'

        return (TRUE);

      }
      else
      {

        /*
           successful use
         */
        act("You successfully use $p to unlock $P!", TRUE, ch, obj, target_o,
            TO_CHAR);
        act("$n successfully use $p to unlock $P!", TRUE, ch, obj, target_o,
            TO_ROOM);

        /*
           unlock
         */
        REMOVE_BIT(target_o->value[1], CONT_LOCKED);

        /*
           remove key
         */
        act("$p breaks into several pieces as it unlocks the lock.",
            TRUE, ch, obj, 0, TO_CHAR);
        act("$p breaks into several pieces as it unlocks the lock.",
            TRUE, ch, obj, 0, TO_ROOM);

        unequip_char(obj->loc.wearing, pos);
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'

        return (TRUE);
      }
    }
  }
  else if ((door = find_door(ch, type, dir)) >= 0)
  {

    /*
       target is a door
     */

    if ((!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) ||
        (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) ||
        (EXIT(ch, door)->key < 0) ||
        (has_key(ch, EXIT(ch, door)->key)) ||
        (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)))
    {

      /*
         let do_unlock deal with these cases
       */
      return (FALSE);

    }
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF))
    {

      /*
         broke key
       */
      act("Your clumsiness causes $p to break into several pieces!", TRUE, ch,
          obj, 0, TO_CHAR);
      act("$n's clumsiness causes $p to break into several pieces!", TRUE, ch,
          obj, 0, TO_ROOM);

      /*
         remove key
       */
      unequip_char(obj->loc.wearing, pos);
      extract_obj(obj, TRUE); // Not an arti, but 'in game.'

      return (TRUE);

    }
    else
    {

      /*
         random chance of breaking key
       */
      if ((rand = number(1, 100)) <= break_percent)
      {

        /*
           broke key
         */
        act("Your clumsiness causes $p to break into several pieces!", TRUE,
            ch, obj, 0, TO_CHAR);
        act("$n's clumsiness causes $p to break into several pieces!", TRUE,
            ch, obj, 0, TO_ROOM);

        /*
           remove key
         */
        unequip_char(obj->loc.wearing, pos);
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'

        return (TRUE);

      }
      else
      {

        /*
           successful use
         */
        if (EXIT(ch, door)->keyword)
        {
          act("You successfully use $p to unlock the $F!", TRUE, ch, obj,
              EXIT(ch, door)->keyword, TO_CHAR);
          act("$n successfully uses $p to unlock the $F!", TRUE, ch, obj,
              EXIT(ch, door)->keyword, TO_ROOM);
        }
        else
        {
          act("You successfully use $p to unlock the door!", TRUE, ch, obj, 0,
              TO_CHAR);
          act("$n successfully uses $p to unlock the door!", TRUE, ch, obj, 0,
              TO_ROOM);
        }

        /*
           unlock ch's side
         */
        REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);

        /*
           unlock the other side
         */
        if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
          if ((back = world[other_room].dir_option[(int) rev_dir[door]]))
            if (back->to_room == ch->in_room)
              REMOVE_BIT(back->exit_info, EX_LOCKED);

        /*
           remove key
         */
        act("$p breaks into several pieces as it unlocks the lock.", TRUE, ch,
            obj, 0, TO_CHAR);
        act("$p breaks into several pieces as it unlocks the lock.", TRUE, ch,
            obj, 0, TO_ROOM);

        unequip_char(obj->loc.wearing, pos);
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'

        return (TRUE);
      }
    }
  }
  return FALSE;
}

#endif
/*
   Pooks/Erevan, added because Erevan is the God of Mischief. Sep 7 1994
 */
int banana(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      rand;

/*
   P_char t_ch;
 */
  P_obj    new_obj;
  char     Gbuf1[MAX_STRING_LENGTH];
  struct affected_type af;

  /*
   * peel:   v-num 1234
   * banana: v-num 1235
   */

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch || cmd == CMD_PERIODIC || !AWAKE(ch))
    return (FALSE);

  if ((cmd < CMD_NORTH) || ((cmd > CMD_DOWN) && (cmd != CMD_EAT)))
    return (FALSE);

  /*
     Eat for banana
   */
  if (cmd == CMD_EAT)
  {
    one_argument(arg, Gbuf1);
    if (str_cmp(Gbuf1, "banana"))
      return (FALSE);
    if (obj->R_num == real_object(1235))
    {
      if (GET_COND(ch, FULL) > 20)
      {
        act("No thanks, I'm absolutely stuffed, couldn't eat another bite.",
            FALSE, ch, 0, 0, TO_CHAR);
        return (TRUE);
      }
      else
      {
        act("$n eats $p. After $e finishes it, $e arrogantly tosses "
            "the peel on the ground to rot.", TRUE, ch, obj, 0, TO_ROOM);
        act("You eat the $o. After you're done, you arrogantly toss the "
            "peel on the ground to rot.", FALSE, ch, obj, 0, TO_CHAR);
        gain_condition(ch, FULL, obj->value[0]);
        CharWait(ch, PULSE_VIOLENCE);
        if (GET_COND(ch, FULL) > 20)
          act("You feel comfortably sated.", FALSE, ch, 0, 0, TO_CHAR);
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'
        obj = NULL;
        new_obj = read_object(1234, VIRTUAL);
        set_obj_affected(new_obj, 2400, TAG_OBJ_DECAY, 0);
        obj_to_room(new_obj, ch->in_room);
        return (TRUE);
      }
      return (TRUE);
    }
    return (FALSE);
  }
  /*
     It's a peel and the player is moving...
   */
  if (obj->R_num == real_object(1235))
    return (FALSE);

  if( IS_TRUSTED(ch) )
    return (FALSE);

  if (IS_RIDING(ch))            /*
                                   Mounts are sure-footed :)
                                 */
    return (FALSE);

  if (IS_AFFECTED(ch, AFF_FLY) || IS_AFFECTED(ch, AFF_LEVITATE))
    return (FALSE);

  rand = number(1, STAT_INDEX(GET_C_INT(ch)));
  if (rand > 4)
  {                             /*
                                   Int check to be smart and not step on it
                                 */
    return (FALSE);
  }
  else
  {                             /*
                                   They stepped on it... random dex roll
                                   determines how bad...
                                 */
    rand = number(1, STAT_INDEX(GET_C_DEX(ch)));
    switch (rand)
    {
    case 1:
      act
        ("You slip on a banana peel, fall, and pass out when your head smacks the ground!",
         FALSE, ch, 0, 0, TO_CHAR);
      act("$n slips on a banana peel, falls over with a surprised look on $s "
          "face, and passes out when $s head smacks the ground!", TRUE, ch, 0,
          0, TO_ROOM);
      if (ch->specials.fighting)
        stop_fighting(ch);
      if( IS_DESTROYING(ch) )
        stop_destroying(ch);
      KnockOut(ch, 6);
      SET_POS(ch, GET_STAT(ch) + POS_PRONE);
/*
      bzero(&af, sizeof(af));
      af.type = SPELL_SLEEP;
      af.duration = number(4, 6);
      af.bitvector = AFF_SLEEP;
      SET_POS(ch, GET_POS(ch) + STAT_SLEEPING);
      affect_join(ch, &af, FALSE, FALSE);
*/

      StopMercifulAttackers(ch);
      GET_HIT(ch) = (GET_HIT(ch) > 15) ? GET_HIT(ch) - 15 : 1;
      return (TRUE);
    case 2:
    case 3:
    case 4:
    case 5:
      /*
         Might as well use rand again... good as anything
       */
      GET_HIT(ch) = (GET_HIT(ch) > rand) ? GET_HIT(ch) - rand : 1;
      act
        ("You slip on a banana peel and fall over with a shriek and a thump!\n"
         "\nYou hurt yourself in the fall!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n slips on a banana peel, shrieks, and falls over!", TRUE, ch, 0,
          0, TO_ROOM);
      SET_POS(ch, POS_SITTING + GET_STAT(ch));
      CharWait(ch, PULSE_VIOLENCE);
      return (TRUE);
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
      act("You step on a banana peel!", TRUE, ch, 0, 0, TO_CHAR);
      act
        ("Arms flailing, you manage you maintain your balance and end up looking only like a small fool.",
         TRUE, ch, 0, 0, TO_CHAR);
      act
        ("$n steps on a banana peel and, arms flailing wildly, barely maintains $s balance.",
         TRUE, ch, 0, 0, TO_ROOM);
      CharWait(ch, PULSE_VIOLENCE);
      return (TRUE);
    default:
      act
        ("You step on a banana peel, but manage to dance your way out of the area.",
         TRUE, ch, 0, 0, TO_CHAR);
      act
        ("$n steps on a banana peel, but with a quick smirk resumes $s travel.",
         TRUE, ch, 0, 0, TO_ROOM);
      return (FALSE);
    }
  }
  return (FALSE);
}

int tyr_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim = NULL, tch1, tch2;
  int      room;
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }
  if ((!ch) || (!arg && (cmd != CMD_CD)))
  {
    return FALSE;
  }
  if ((cmd != CMD_CD) && (cmd != CMD_SACRIFICE) && (cmd != CMD_TERMINATE))
  {
    return FALSE;
  }
  one_argument(arg, Gbuf1);
  if ((!*Gbuf1) && (cmd != CMD_CD))
  {
    send_to_char("Who?\n", ch);
    return FALSE;
  }
  if ((!OBJ_WORN_BY(obj, ch)) ||
      ((obj->loc.wearing->equipment[WIELD] != obj) &&
       (obj->loc.wearing->equipment[SECONDARY_WEAPON] != obj)))
  {
    return FALSE;
  }
  if ((cmd != CMD_CD) && (!(victim = get_char_room_vis(ch, Gbuf1))))
  {
    send_to_char("Who?\n", ch);
    return FALSE;
  }
  if (str_cmp(ch->player.name, "Tyr") && str_cmp(ch->player.name, "Gruumsh"))
  {
    if (IS_PC(ch))
    {
      act
        ("Gruumsh's Flaming Bringer of Vengeance glows briefly with a bright light.",
         FALSE, ch, 0, victim, TO_CHAR);
      act("The sword wrenches free of your grip, diving into your chest!",
          FALSE, ch, 0, victim, TO_CHAR);
      act
        ("$n attempts to kill $N with Gruumsh's Flaming Bringer of Vengeance!",
         TRUE, ch, 0, victim, TO_NOTVICT);
      act
        ("The sword wrenches free from $n, and carves its way into $s chest!",
         TRUE, ch, 0, victim, TO_NOTVICT);
      act
        ("$n attempts to kill you with Gruumsh's Flaming Bringer of Vengeance!",
         FALSE, ch, 0, victim, TO_VICT);
      act("The sword glows brightly before carving into $n's chest!", FALSE,
          ch, 0, victim, TO_VICT);
      statuslog(ch->player.level,
                "%s killed while fooling with Gruumsh's sword.",
                GET_NAME(ch));
      logit(LOG_WIZ, "%s killed trying to use Gruumsh's sword.",
            GET_NAME(ch));
      die(ch, ch);
      return TRUE;
    }
    else
    {
      send_to_char("Monsters can't use this mighty Sword!\n", ch);
      return TRUE;
    }
  }
  if (cmd == CMD_CD)
  {
    if ((!str_cmp(ch->player.name, "Gruumsh")) ||
        (!str_cmp(ch->player.name, "Tyr")))
    {
      if (!*Gbuf1)
      {
        room = real_room(1206);
      }
      else if (is_number(Gbuf1))
      {
        room = real_room(atoi(Gbuf1));
        if ((room == NOWHERE) || (room > top_of_world))
        {
          send_to_char("No room exists with that number.\n", ch);
          return TRUE;
        }
      }
      else if ((victim = get_char_vis(ch, Gbuf1)))
      {
        room = victim->in_room;
      }
      else
      {
        send_to_char("Sorry, I know of no one by that name!\n", ch);
        return TRUE;
      }
    }
    else
    {
      return FALSE;
    }

    act("You step into a portal that opens abruptly in front of you.\n",
        FALSE, ch, 0, 0, TO_CHAR);
    act("$n steps into a portal that opens abruptly in front of $m.",
        TRUE, ch, 0, 0, TO_ROOM);

    char_from_room(ch);
    char_to_room(ch, room, -1);

    act("$n steps out of a portal that opens abruptly in front of you.\n"
        "$n &+RFARTS&n loudly!!", TRUE, ch, 0, 0, TO_ROOM);
    /*
       stolen from mobact.c -- DTS 8/12/95
     */
    for (tch1 = world[ch->in_room].people; tch1; tch1 = tch2)
    {
      tch2 = tch1->next_in_room;

      if (IS_TRUSTED(tch1))
        continue;
      if (CAN_SEE(tch1, ch))
      {
        if (GET_LEVEL(tch1) < (GET_LEVEL(ch) / 3))
        {
          do_flee(tch1, 0, 2);  /*
                                   panic flee, no save
                                 */
        }
        else if (!NewSaves(tch1, SAVING_PARA, 1))
        {
          do_flee(tch1, 0, 1);  /*
                                   fear, but not panic
                                 */
        }
      }
    }

    return TRUE;
  }
  if (ch == victim)
  {
    send_to_char("Don't use your Sword on yourself, idiot!\n", ch);
    return TRUE;
  }
  if (IS_NPC(victim))
  {
    send_to_char("Don't use the Sword on monsters!\n", ch);
    return TRUE;
  }
  act("Your sword, Bringer of Vengeance, flares blue as you call down\n"
      "the fist of vengeance upon $N.  $S body is ravaged by a searing heat,\n"
      "causing $M to scream in agony.  $N crumbles in a heap at your feet.\n",
      FALSE, ch, 0, victim, TO_CHAR);

  act("$n points $s Bringer of Vengeance at you, which flares blue\n"
      "as $e calls down vengeance upon you.  Your body is ravaged by a searing heat,\n"
      "causing you to scream in agony.  As you look down, you see your\n"
      "former body, crumpled in a heap at $n's feet.\n",
      FALSE, ch, 0, victim, TO_VICT);

  act("$n points $s sword, Bringer of Vengeance, at $N.\n"
      "The sword flares blue as $n enacts vengeance upon $N, whose body\n"
      "is ravaged by a searing heat, causing $M to scream in agony.\n"
      "$N crumbles at $n's feet.\n", TRUE, ch, 0, victim, TO_NOTVICT);

  if (cmd == CMD_SACRIFICE)
  {
    statuslog(ch->player.level, "%s was sacrificed by %s.",
              GET_NAME(victim), GET_NAME(ch));
    logit(LOG_WIZ, "%s was sacrificed by %s.", GET_NAME(victim),
          GET_NAME(ch));
    die(victim, ch);
    return TRUE;
  }
  else if (cmd == CMD_TERMINATE)
  {
    act(".", FALSE, ch, 0, victim, TO_CHAR);
    act(".", FALSE, ch, 0, victim, TO_CHAR);
    act("You call the pure might of the Forgers down upon $N.\n"
        "With great magic, you devour $N's soul, utterly obliterating\n"
        "$M from the world of Duris forever...\n"
        "The world stands in awe of your awesome power... ;)",
        FALSE, ch, 0, victim, TO_CHAR);

    act(".", FALSE, ch, 0, victim, TO_NOTVICT);
    act(".", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$n points at $N, and whispers \"Vengeance\" to $s sword,\n"
        "Bringer of Vengeance.  $n raises $s hand, and calls down the\n"
        "might of the Forgers on $N.  $n slowly devours $N's soul,\n"
        "utterly destroying $M from this world of Duris forever...\n"
        "You can hear $N's soul scream in agony one last time,\n"
        "then fade on the winds....", FALSE, ch, 0, victim, TO_NOTVICT);

    act(".", FALSE, ch, 0, victim, TO_VICT);
    act(".", FALSE, ch, 0, victim, TO_VICT);
    act("$n points at you, and whispers \"Vengeance\" to $s sword,\n"
        "Bringer of Vengeance.  $n raises $s hand, and calls down\n"
        "the might of the Forgers upon you!\n"
        "$n slowly devours your soul as it rises out of your dead body.\n"
        "AAAAAAAAAAAAAAAAAAAHHHHH!!!! The pain is agonizing, but there is no escape\n"
        "Your soul is destroyed utterly, forever obliterated from this world.",
        FALSE, ch, 0, victim, TO_VICT);
    statuslog(ch->player.level, "%s was just terminated by the power of %s.",
              GET_NAME(victim), GET_NAME(ch));
    logit(LOG_WIZ, "%s terminated by %s.", GET_NAME(victim), GET_NAME(ch));
    if (victim->desc)
    {
      victim->desc->connected = CON_DELETE;
    }
    extract_char(victim);
    return TRUE;
  }
  logit(LOG_DEBUG, "Somehow reached the end of tyr_sword().");
  return FALSE;
}

// Subtract 1 from values[0] each cast.  When reaching 0, poof item.
int crystal_spike(P_obj obj, P_char ch, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, HOLD) )
  {
    return FALSE;
  }

  if( obj->loc.wearing == ch )
  {
    if (cmd == CMD_CAST)
    {
      obj->value[0]--;
      if (obj->value[0] == 0)
      {
        act("Your $q fades slowly, and disappears into nothing.", FALSE,
            obj->loc.wearing, obj, 0, TO_CHAR);
        act("$n's $q fades slowly, and disappears into nothing.", FALSE,
            obj->loc.wearing, obj, 0, TO_ROOM);
        unequip_char(obj->loc.wearing, HOLD);
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'
        obj = NULL;
      }
    }
  }
  return FALSE;
}

/* Woundhealer is in specs.undermountain.c -> woundhealer_scimitar.
int woundhealer(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam;
  P_char   vict;

  // Check for periodic event calls
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_MELEE_HIT)
    return (FALSE);

  if (!ch)
    return (FALSE);

  if (!OBJ_WORN_POS(obj, WIELD) && !OBJ_WORN_POS(obj, HOLD))
    return (FALSE);

  vict = (P_char) arg;
  dam = BOUNDED(0, (GET_HIT(vict) + 9), number(1, 8));

  if ((obj->loc.wearing == ch) && vict)
  {
    GET_HIT(ch) += dam;
    GET_HIT(vict) -= dam;
  }
  return (TRUE);
}
*/

#if 0

/* func is buggy..  if you want to fix it you'll have to fix the part
   that accesses the dir array and maybe other stuff */

int cursed_mirror(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      is_worn, is_carried = FALSE;
  char     buf[20];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if ((cmd < CMD_NORTH || cmd > CMD_UP) && cmd != CMD_LOOK)
    return FALSE;

  if (OBJ_WORN_BY(obj, ch))
  {
    is_worn = TRUE;
  }
  else if (OBJ_CARRIED_BY(obj, ch))
  {
    is_carried = TRUE;
  }
  if (cmd == CMD_LOOK)
  {
    if (is_worn && (number(1, 101) < 15) && !IS_DARK(ch->in_room))
    {
      send_to_char
        ("The cursed mirror in your hands leaps up into your line of sight!\n"
         "You find yourself looking back behind you.\n\n", ch);
      switch (number(0, 6))
      {
      case 0:
      case 1:
      case 3:
        strcpy(buf, dirs[(int) rev_dir[cmd]]);
        do_look(ch, buf, -4);
        break;
      case 4:
        send_to_char("You think you see someone staring at you.\n", ch);
        break;
      case 5:
        send_to_char
          ("You see a blurr as a shadowed figure darts out of your vision.\n",
           ch);
        break;
      case 6:
        send_to_char("You see nothing suspicious.\n", ch);
        break;
      }
      return TRUE;
    }
    else
    {
      return FALSE;
    }
  }
  else
  {
    if( (!is_worn && !is_carried) || (number(1, 101) > 15)
      || IS_DARK(ch->in_room) || IS_TRUSTED(ch) )
    {
      return FALSE;
    }
    else
    {
/*
   old_cmd = cmd;
   while (cmd == old_cmd) {
   cmd = number(CMD_NORTH, CMD_DOWN);
   }
 */
      if (!world[ch->in_room].dir_option[(int) rev_dir[cmd]] ||
          IS_SET(EXIT(ch, (int) rev_dir[cmd])->exit_info, EX_SECRET) ||
          IS_SET(EXIT(ch, (int) rev_dir[cmd])->exit_info, EX_BLOCKED))
      {
        do_move(ch, 0, rev_dir[cmd]);
        send_to_char
          ("The light glints off your mirror, temporarily blinding\n"
           "and confusing you! You stumble off in the reverse direction!\n\n",
           ch);
        return TRUE;
      }
      else
        return FALSE;
    }
    return FALSE;
  }
  return FALSE;
}
#endif

#if 0                           /*
                                   commented out till I can fix it...DTS
                                 */
int cursed_mirror(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      is_worn, is_carried, match;
  char     buf[200];
  char     tmpbuf[200];

  is_worn = is_carried = match = FALSE;
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if ((cmd < CMD_NORTH || cmd > CMD_UP) && cmd != CMD_LOOK)
    return FALSE;

  if (OBJ_WORN_BY(obj, ch))
  {
    is_worn = TRUE;
  }
  else if (OBJ_CARRIED_BY(obj, ch))
  {
    is_carried = TRUE;
  }
  if (cmd == CMD_LOOK)
  {
    if (is_worn && arg && (number(1, 101) < 15) && !IS_DARK(ch->in_room))
    {
      sscanf(arg, " %s", tmpbuf);
      match = search_block(tmpbuf, dirs, FALSE);
      if (match == -1)
        return FALSE;
      send_to_char
        ("The cursed mirror in your hands leaps up into your line of sight!\n"
         "You find yourself looking back behind you.\n\n", ch);
      switch (number(0, 6))
      {
      case 0:
      case 1:
      case 2:
      case 3:
        switch (match)
        {
        case 0:
          strcpy(buf, dirs[2]);
          break;
        case 1:
          strcpy(buf, dirs[3]);
          break;
        case 2:
          strcpy(buf, dirs[0]);
          break;
        case 3:
          strcpy(buf, dirs[1]);
          break;
        case 4:
          strcpy(buf, dirs[5]);
          break;
        case 5:
          strcpy(buf, dirs[4]);
          break;
        default:
          send_to_char("Serious error with mirror. Notify a forger!\n", ch);
          return FALSE;
        }
        do_look(ch, buf, -4);
        break;
      case 4:
        send_to_char("You think you see someone staring at you.\n", ch);
        break;
      case 5:
        send_to_char
          ("You see a blurr as a shadowed figure darts out of your vision.\n",
           ch);
        break;
      case 6:
        send_to_char("You see nothing suspicious.\n", ch);
        break;
      }
      return TRUE;
    }
    else
    {
      return FALSE;
    }
    return FALSE;
  }
  else
  {
    if( (!is_worn && !is_carried) || (number(1, 101) > 15)
      || IS_DARK(ch->in_room) || IS_TRUSTED(ch) )
    {
      return FALSE;
    }
    else
    {
      if (!world[ch->in_room].dir_option[(int) rev_dir[cmd]] ||
          IS_SET(EXIT(ch, (int) rev_dir[cmd])->exit_info, EX_SECRET) ||
          IS_SET(EXIT(ch, (int) rev_dir[cmd])->exit_info, EX_BLOCKED))
      {
        do_move(ch, 0, rev_dir[cmd]);
        send_to_char
          ("The light glints off your mirror, temporarily blinding\n"
           "and confusing you! You stumble off in the reverse direction!\n\n",
           ch);
        return TRUE;
      }
      else
        return FALSE;
    }
    return FALSE;
  }
  return FALSE;
}
#endif

/*
   This is mostly ripped off from item_switch(), but with modifications
   * specific to this particular item...
   * -- DTS 2/21/95
 */
int automaton_lever(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      bits;
  P_char   tempch;
  P_obj    tempobj;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !obj || !IS_ALIVE(ch) || !arg )
  {
    return FALSE;
  }

  // No tracks are autmaton levers.
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &tempch, &tempobj);
  if( tempobj != obj )
  {
    return FALSE;
  }

  if( cmd == CMD_PULL )
  {
    if (IS_SET(world[ch->in_room].dir_option[UP]->exit_info, EX_BLOCKED))
    {
      act("You pull $p.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n pulls $p.", TRUE, ch, obj, 0, TO_ROOM);
      send_to_room("A loud metallic scraping sounds, followed by a clunk.\n",
                   ch->in_room);
      send_to_room("The trapdoor appears to hang ever so slightly lower.\n",
                   ch->in_room);
      REMOVE_BIT(world[ch->in_room].dir_option[UP]->exit_info, EX_BLOCKED);
      REMOVE_BIT(world[real_room(12158)].dir_option[DOWN]->exit_info, EX_BLOCKED);
      return TRUE;
    }
    else
    {
      send_to_char("Nothing seems to happen.\n", ch);
      return TRUE;
    }
  }
  else
  {
    return FALSE;
  }
}

/*
   This obj proc lets a person offer a held TREASURE and receive some sort
   ** of goodie, detailed below.  The treasures are usually (presumably) gems.
   ** Chars will always get either a bless spell or a bit of money
   ** (amount varies).  There is a 6 in 700 chance of getting something extra
   ** cool, such as either of two charmies, or a cool potion or scroll, or a
   ** vitality spell.
 */

int llyms_altar(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_obj    treasure = NULL, tempobj = NULL;
  char     treas_name[MAX_INPUT_LENGTH], altar_name[MAX_INPUT_LENGTH];
  P_char   tempchar = NULL;
  int      bits, money = 0;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_OFFER || !obj || !IS_ALIVE(ch) || !AWAKE(ch) )
  {
    return FALSE;
  }

  arg = one_argument(arg, treas_name);
  arg = one_argument(arg, altar_name);

  if( !*treas_name || !*altar_name )
  {
    return FALSE;
  }

  // Skip tracks when looking for altar.
  bits = generic_find(altar_name, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &tempchar, &tempobj);
  bits = generic_find(treas_name, FIND_OBJ_EQUIP, ch, &tempchar, &treasure);
  if( tempobj != obj || !treasure )
  {
    return FALSE;
  }

  // Must be a TREASURE to work
  if( !CAN_SEE_OBJ(ch, treasure) || !OBJ_WORN_BY(treasure, ch) || !OBJ_WORN_POS(treasure, HOLD) || GET_ITEM_TYPE(treasure) != ITEM_TREASURE )
  {
    return FALSE;
  }

  act("You offer up $p to $P.", TRUE, ch, treasure, obj, TO_CHAR);
  act("$n offers up $p to $P.", TRUE, ch, treasure, obj, TO_ROOM);

  if( IS_ARTIFACT(treasure) )
  {
    act("$P rumbles briefly, then is silent.\n\rYour offering is refused.", TRUE, ch, 0, obj, TO_CHAR);
    act("$P rumbles briefly, then is silent.", TRUE, ch, 0, obj, TO_ROOM);
    return TRUE;
  }

  // It better be worth something -- at some point I'm gonna make reward depend on the value of the treasure
  if( treasure->cost < 10000 )
  {
    act("$P rumbles briefly, then is silent.", TRUE, ch, 0, obj, TO_CHAR);
    act("$P rumbles briefly, then is silent.", TRUE, ch, 0, obj, TO_ROOM);
    act("Your offering of $p is no good.", TRUE, ch, treasure, 0, TO_CHAR);
    act("$n's offering is inadequate.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  act("A &+Bblue light&n streaks from $P and strikes your hand, enveloping $p.", TRUE, ch, treasure, obj, TO_CHAR);
  act("A &+Bblue light&n streaks from $P and strikes $n's hand, enveloping $p.", TRUE, ch, treasure, obj, TO_ROOM);
/* This message got reworked.
  act ("$p is wrenched from your hand and plunges into $P!", TRUE, ch, treasure, obj, TO_CHAR);
  act ("$p is wrenched from $n's hand and plunges into $P!", TRUE, ch, treasure, obj, TO_ROOM);
 */

  obj_to_char(unequip_char(ch, HOLD), ch);
  obj_from_char(treasure);
  extract_obj(treasure, TRUE); // Not an arti, but 'in game.'

  // They get blessed, unless they're already blessed, in which case, they  get money.
  //   Then there's a small chance of something extra cool happening.
  if( !affected_by_spell(ch, SPELL_BLESS) )
  {
    act("$p throbs for a minute, and you suddenly feel blessed.", TRUE, ch, obj, 0, TO_CHAR);
    act("$p throbs for a minute, and $n glows with a pure &+Bwhite&n light.", TRUE, ch, obj, 0, TO_ROOM);
    act("The light swiftly fades around $n.", TRUE, ch, 0, 0, TO_ROOM);
    spell_bless(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
  }
  else
  {
    act("Your purse suddenly feels heavier!", TRUE, ch, 0, 0, TO_CHAR);
    money = number(1, 4);
    switch (money)
    {
    case 1:
      GET_PLATINUM(ch) += number(1, 10);
      break;
    case 2:
      GET_GOLD(ch) += number(1, 10);
      break;
    case 3:
      GET_SILVER(ch) += number(1, 10);
      break;
    case 4:
      GET_COPPER(ch) += number(1, 10);
      break;
    default:
      break;
    }
  }
  // Low chance of something really cool happening.
  switch( number(1, 700) )
  {
  case 1:
    tempchar = read_mobile(88814, VIRTUAL);
    if( !tempchar )
    {
      logit(LOG_SYS, "error in read_mobile: llyms_altar()");
      send_to_char("Error in altar proc.  Inform a god.\n", ch);
      return FALSE;
    }
    char_to_room(tempchar, ch->in_room, -1);
    act("$n appears in a flash of light!", TRUE, tempchar, 0, 0, TO_ROOM);
    setup_pet(tempchar, ch, -1, PET_NOCASH);
    add_follower(tempchar, ch);
    break;
  case 2:
    tempchar = read_mobile(88815, VIRTUAL);
    if( !tempchar )
    {
      logit(LOG_SYS, "error in read_mobile: llyms_altar()");
      send_to_char("Error in altar proc.  Inform a god.\n", ch);
      return FALSE;
    }
    char_to_room(tempchar, ch->in_room, -1);
    act("$n appears in a flash of light!", TRUE, tempchar, 0, 0, TO_ROOM);
    setup_pet(tempchar, ch, -1, PET_NOCASH);
    add_follower(tempchar, ch);
    break;
  case 3:
    tempobj = read_object(88831, VIRTUAL);
    if( !tempobj )
    {
      logit(LOG_SYS, "error in read_object: llyms_altar()");
      send_to_char("Error in altar proc. Inform a god.\n", ch);
      return FALSE;
    }
    obj_to_room(tempobj, ch->in_room);
    act("$p appears before you in a flash of light!", TRUE, ch, tempobj, 0, TO_CHAR);
    act("$p appears in a flash of light!", TRUE, ch, tempobj, 0, TO_ROOM);
    break;
  case 4:
    tempobj = read_object(88832, VIRTUAL);
    if( !tempobj )
    {
      logit(LOG_SYS, "error in read_object: llyms_altar()");
      send_to_char("Error in altar proc. Inform a god.\n", ch);
      return FALSE;
    }
    obj_to_room(tempobj, ch->in_room);
    act("$p appears before you in a flash of light!", TRUE, ch, tempobj, 0, TO_CHAR);
    act("$p appears in a flash of light!", TRUE, ch, tempobj, 0, TO_ROOM);
    break;
  case 5:
    tempobj = read_object(88833, VIRTUAL);
    if( !tempobj )
    {
      logit(LOG_SYS, "error in read_object: llyms_altar()");
      send_to_char("Error in altar proc. Inform a god.\n", ch);
      return (FALSE);
    }
    obj_to_room(tempobj, ch->in_room);
    act("$p appears before you in a flash of light!", TRUE, ch, tempobj, 0, TO_CHAR);
    act("$p appears in a flash of light!", TRUE, ch, tempobj, 0, TO_ROOM);
    break;
  case 6:
    if( !affected_by_spell(ch, SPELL_VITALITY) )
    {
      act("$p throbs for a minute, and you suddenly feel vitalized!", TRUE, ch, obj, 0, TO_CHAR);
      act("$p throbs for a minute, and $n appears vitalized!", TRUE, ch, obj, 0, TO_ROOM);
      spell_vitality(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
    }
    break;
  default:
    break;
  }
  return TRUE;
}

/*
   This little routine will make the ruby monocle "load randomly" if it's
   * on the ground in one of the given rooms at zone reset.  The room range is
   * 90124-90142.
   * -- DTS 4/4/95
 */

int fw_ruby_monocle(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      randroom;

  /* check for periodic event calls */
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_ROOM(obj) || cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  // If obj is in a good room, and zone just reset.
  if( (world[obj->loc.room].number >= 90124) &&
      (world[obj->loc.room].number <= 90142) &&
      (zone_table[world[obj->loc.room].zone].age == 0) )
  {
    randroom = number(90124, 90142);
    obj_from_room(obj);
    obj_to_room(obj, real_room(randroom));
    return TRUE;
  }
  return FALSE;
}

/* This object randomly moves around the map world, and allows entrance to
   the flying citadel floating above */
int flying_citadel(P_obj obj, P_char ch, int cmd, char *arg)
{
  int door;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  // If not in a room.. (i.e. some God picked it up or something).
  if( !OBJ_ROOM(obj) )
  {
    return FALSE;
  }

  door = number(0, 11);

  if( (world[obj->loc.room].number >= 110000) &&
      (world[obj->loc.room].number <= 199999) && door < 4 )
  {
    debug("flying_citadel: Passed check one.");
// This crap crashes the MUD atm.
return FALSE;
    if( VIRTUAL_EXIT(OBJ_ROOM(obj), door)->to_room &&
        VIRTUAL_EXIT(OBJ_ROOM(obj), door)->to_room != NOWHERE )
    {
      debug("flying_citadel: Passed check two.");
      send_to_room("The dark clouds overhead move onward.\n", OBJ_ROOM(obj));
      obj_from_room(obj);
      obj_to_room(obj, real_room(door));
      send_to_room("An extremely large shadow floats overhead.\n", OBJ_ROOM(obj));
      return TRUE;
    }
  }
  // Dunno about this code either.. As we don't fly up these days..
  if( IS_ALIVE(ch) && ch->specials.z_cord == 4 )
  {
    char_from_room(ch);
    char_to_room(ch, real_room0(obj_index[obj->R_num].number), 0);
    ch->specials.z_cord = 0;
    return TRUE;
  }

  return FALSE;
}

int orcus_wand(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim, tmp_ch1, tmp_ch2;
  int      temproom;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !arg || (cmd != CMD_SACRIFICE && cmd != CMD_TERMINATE && cmd != CMD_CD)
    || !OBJ_WORN_BY(obj, ch) || obj->loc.wearing->equipment[HOLD] != obj )
  {
    return FALSE;
  }

  if( GET_LEVEL(ch) < 59 && cmd != CMD_CD )
  {
    return FALSE;
  }

  one_argument(arg, Gbuf1);

  if( !*Gbuf1 )
  {
    send_to_char("Who?\n", ch);
    return TRUE;
  }

  if( !(victim = get_char_room_vis(ch, Gbuf1)) )
  {
    if( !(victim = get_char_vis(ch, Gbuf1)) )
    {
      send_to_char("Who?\n", ch);
      return (FALSE);
    }
    cmd = CMD_CD;
  }
  if( GET_LEVEL(ch) < 56 )
  {
    act("&+LThe Wand of Orcus glows briefly with a sickly black light.", FALSE, ch, 0, victim, TO_CHAR);
    act("&+LThe Wand wrenches free of your grip, diving into your chest!", FALSE, ch, 0, victim, TO_CHAR);
    act("&+r$n&+L attempts to kill&N&+r $N&N&+L with the Wand of Orcus!", TRUE, ch, 0, victim, TO_NOTVICT);
    act("&+LThe Wand of Orcus wrenches free from&N&+r $n&N, and carves its way into $n's chest!", TRUE, ch, 0, victim, TO_NOTVICT);
    act("&+r$n&N&+L attempts to kill you with the Wand of Orcus!", FALSE, ch, 0, victim, TO_VICT);
    act("&+LThe Wand of Orcus glows black, and carves into&N&+r $n's&N&+L chest!", FALSE, ch, 0, victim, TO_VICT);
    die(ch, ch);
    return TRUE;
  }
  if( (cmd == CMD_CD) && isname("Orcus", ch->player.name) )
  {
    temproom = victim->in_room;
    if( (temproom < 0) || (temproom > 999999) )
    {
      return FALSE;
    }

    strcpy(Gbuf4, "&+LOrcus&N&+b whispers 'Shift' to his Wand, which starts to glow black.\n"
      "&+bThe Wand opens a portal to the Abyss, which &+LOrcus&N&+b steps through.\n"
      "&+bA beam of black light hits the portal, and it snaps shut.&N\n");
    send_to_room(Gbuf4, ch->in_room);
    char_from_room(ch);
    char_to_room(ch, temproom, -1);

    strcpy(Gbuf4, "&+bThe sky turns black, as a great shadow looms over all you see.\n"
      "&+bThe shadow slowly shrinks down in size until the ghastly form of\n"
      "&+LOrcus, Demon Prince of The Undead&N&+b, appears infront of you.\n");
    send_to_room(Gbuf4, ch->in_room);
    for( tmp_ch1 = world[ch->in_room].people; tmp_ch1; tmp_ch1 = tmp_ch2 )
    {
      tmp_ch2 = tmp_ch1->next_in_room;
      if ((tmp_ch1 != ch) && (GET_LEVEL(tmp_ch1) < 51))
      {
        do_flee(tmp_ch1, 0, 2);
      }
    }
    return (TRUE);
  }
  if( ch == victim )
  {
    send_to_char("You cannot kill yourself, DUH!! Go pick your nose!..\n", ch);
    return (FALSE);
  }
  if( IS_PC(victim) && !str_cmp(victim->player.name, "Orcus") )
  {
    act("The Wand glows with a sickly black light, and flies out of your hand\n"
      "and shoots out a beam of deadly black light into your chest!\n"
      "You fall victim to the very god you sought to destroy...", FALSE, ch, 0, victim, TO_CHAR);
    act("$n attempts to kill Orcus with his own Wand!\n"
        "The Wand glows black, and flies out in the air and dives into $n's chest!", TRUE, ch, 0, victim, TO_NOTVICT);
    act("$n attempts to kill you with your own wand! What a dolt!\n"
        "The Wand glows black, and flies into the air and dives into $n's chest!", FALSE, ch, 0, victim, TO_VICT);
    die(ch, ch);
    return TRUE;
  }
  if( ch != victim )
  {
    act("&+bYou point your Wand at $N, and call down your power at $N.\n"
      "&+bThe Wand comes to life, and starts to glow with the power of the Abyss, \n"
      "&+bas if it had a hunger of its own for $N's heart!\n"
      "&+bThe Wand rips out $N's heart, stealing $S life life away...", FALSE, ch, 0, victim, TO_CHAR);

    act("&+b$n points at you, and whispers 'Die' to $s Wand.\n"
      "&+b$n points $s Wand at you and calls down $s power on you!\n"
      "&+bThe Wand comes to life, and starts to glow with the power of the Abyss!  It\n"
      "&+bThe Wand shoots a black beam of light at your head. Searing pain shoots\n"
      "&+bthrough your head for a split second, and then all goes black.\n", FALSE, ch, 0, victim, TO_VICT);

    act("&+b$n points at $N, and whispers 'Die' to $s Wand.\n"
      "&+bThe Wand shoots forth a beam of black light towords $N!\n"
      "&+b$N shudders slightly, and then falls to the ground, a withered\n"
      "&+bpile of flesh. $N is sacrificed to the Demon Prince, Orcus.\n", TRUE, ch, 0, victim, TO_NOTVICT);

    if( cmd == CMD_SACRIFICE )
    {
      die(victim, ch);
      if( (GET_LEVEL(ch) >= 57) && (GET_LEVEL(ch) >= GET_LEVEL(victim)) )
      {
        statuslog(ch->player.level, "%s was destroyed by Orcus.", GET_NAME(victim));
      }
    }
    else if( (cmd == CMD_TERMINATE) && (IS_PC(victim)) )
    {
      if( (GET_LEVEL(ch) >= MINLVLIMMORTAL) && (GET_LEVEL(ch) >= GET_LEVEL(victim)) )
      {
        act(".", FALSE, ch, 0, victim, TO_CHAR);
        act(".", FALSE, ch, 0, victim, TO_CHAR);
        act("You call on the pure might of the Forgers down upon $N", FALSE, ch, 0, victim, TO_CHAR);
        act("With great magic, you devour $N's soul and utterly, ", FALSE, ch, 0, victim, TO_CHAR);
        act("obliterating $M from this world of Duris, forever...", FALSE, ch, 0, victim, TO_CHAR);
        act("The world stands in awe of your awesome power... ;)", FALSE, ch, 0, victim, TO_CHAR);
        act(".", FALSE, ch, 0, victim, TO_NOTVICT);
        act(".", FALSE, ch, 0, victim, TO_NOTVICT);
        act("$n points at $N, and whispers 'Destroy' to $s Wand.", FALSE, ch, 0, victim, TO_NOTVICT);
        act("$n raises $s hand, and calls down the might of the Forgers on $N", FALSE, ch, 0, victim, TO_NOTVICT);
        act("$n slowly devours $N's soul, utterly destroying $S from this world forever.", FALSE, ch, 0, victim, TO_NOTVICT);
        act("You can hear $N's soul scream in agony one last time, then fade on the winds....", FALSE, ch, 0, victim, TO_NOTVICT);
        act(".", FALSE, ch, 0, victim, TO_VICT);
        act(".", FALSE, ch, 0, victim, TO_VICT);
        act("$n points at you, and whispers 'Destroy' to $s Wand.", FALSE, ch, 0, victim, TO_VICT);
        act("$n raises his hand, and calls down the might of the Forgers upon you!", FALSE, ch, 0, victim, TO_VICT);
        act("$n slowly devours your soul as it rises out of your dead body.", FALSE, ch, 0, victim, TO_VICT);
        act("AAAAAAAAAAHHHHHHHHHHHHH!!!! The pain is agonizing, though there is no escape..", FALSE, ch, 0, victim, TO_VICT);
        act("Your soul is destroyed utterly, forever obliterated from this world...", FALSE, ch, 0, victim, TO_VICT);
        statuslog( ch->player.level, "%s's soul was just utterly devoured by the power of Orcus. Boo Hiss! What a PUD!", GET_NAME(victim) );
        if( victim->desc )
        {
          victim->desc->connected = CON_DELETE;
        }
        extract_char(victim);
      }
    }
    else if( (cmd == CMD_TERMINATE) && (IS_NPC(victim)) )
    {
      die(ch, ch);
      statuslog(ch->player.level, "%s's soul was just utterly devoured by the power of Orucs.. whee", GET_NAME(victim) );
    }
    return TRUE;
  }
  return FALSE;
}

void prepare_wall_messages(char *color_string, char *ch_buffer, char *room_buffer)
{
  sprintf(ch_buffer, "...and are enveloped by a %s&N field as you try to pass through it.", color_string);
  sprintf(room_buffer, "...and is enveloped by a %s&N field as $e tries to pass through it.", color_string);
}

/*
 * This is a generic wall procedure. Since all walls are to large extent similar in their
 * behavior, it is strongly recommended to extend this function for the new walls instead
 * of adding other functions. This would lead to code duplication and general mess.
 * All walls have the same virtual number, with custom descriptions. Values on the item
 * have the following meaning:
 *
 * 0 - room number, where the other side of the wall is
 * 1 - direction this wall is blocking
 * 2 - wall's power ie. damage it deals or its 'hitpoints'
 * 3 - wall type ie. stone, flame, cube, illusionary etc.
 * 4 - wall's level, used for dispelling
 * 5 - number of wall's owner
 */
int wall_generic(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam, type, dircmd, spl;
  int      was_in, to_room = 0;
  P_char   illusionist;
  char     buffer[MAX_STRING_LENGTH], Gbuf1[MAX_STRING_LENGTH];
  char     arg1[512], arg2[512];
  struct follow_type *k, *next_dude;
  P_obj    tempobj;
  struct affected_type af;
  bool     drag_followers, downexit = FALSE;
  char     char_message[512], room_message[512];
  struct damage_messages messages = {
    char_message, char_message, room_message,
    "Your head is filled with all the colours of the rainbow before it bursts.",
    "Your head is filled with all the colours of the rainbow before it bursts.",
    "$n charges against $p turning $sself into a colourful corpse.",
    0, obj
  };

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (obj->loc_p != LOC_ROOM)
    return FALSE;

  type = obj->value[3];

  if(obj->value[1] == DOWN)
    downexit = TRUE;

  if (cmd == CMD_DECAY)
  {
    if (world[obj->loc.room].people)
    {
      if (type == WALL_OF_FLAMES)
      {
        act("&+R$p&n&+R fades away into thin air...&n", FALSE,
            world[obj->loc.room].people, obj, 0, TO_ROOM);
        act("&+R$p&n&+R fades away into thin air...&n", FALSE,
            world[obj->loc.room].people, obj, 0, TO_CHAR);
      }
      else
      {
        act("$p crumbles to dust and blows away.",
            TRUE, world[obj->loc.room].people, obj, 0, TO_ROOM);
        act("$p crumbles to dust and blows away.",
            TRUE, world[obj->loc.room].people, obj, 0, TO_CHAR);
      }
    }
    if (!VIRTUAL_EXIT(obj->loc.room, obj->value[1]))
    {
      logit(LOG_DEBUG,
          "Decay(): error - wall is not on valid exit - room rnum #%d, value[1] %d (trying to remove EX_WALLED)\r\n",
          obj->loc.room, obj->value[1]);
    }
    else
    {
      REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info, EX_WALLED);
      REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info, EX_BREAKABLE);
      REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info, EX_ILLUSION);
    }
    if(downexit && (world[obj->loc.room].sector_type == SECT_NO_GROUND || world[obj->loc.room].sector_type == SECT_UNDRWLD_NOGROUND))
    {
      int speed = 1;
      was_in = obj->loc.room;
      for(tempobj = world[was_in].contents; tempobj; tempobj = tempobj->next_content)
      {
        if(tempobj == obj)
          continue;
        add_event(event_falling_obj, 4, NULL, NULL, tempobj, 0, &speed, sizeof(speed));
      }
    }
    return TRUE;
  }

  if (cmd == CMD_FOUND && type == ILLUSIONARY_WALL)
  {
    REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info,
               EX_ILLUSION);
    return FALSE;
  }

  if (!ch || (ch->specials.z_cord != 0))
    return FALSE;

  if (type == WATCHING_WALL && cmd >= 1 && cmd < 1000)
  {

    if (obj->loc_p == LOC_ROOM && ch && !IS_TRUSTED(ch) &&
        (time(NULL) - obj->value[6]) > 10)
    {

      obj->value[6] = time(NULL);

      for (illusionist = character_list; illusionist; illusionist = illusionist->next)
        if(IS_PC(illusionist) &&
           GET_PID(illusionist) == obj->value[5])
          break;

      if (illusionist != NULL &&
          ch->in_room != illusionist->in_room &&
          !number(0, 2))
      {
        send_to_char("&=LWYou receive a vision from elsewhere.&n&n\n", illusionist);
        new_look(illusionist, "", CMD_LOOK, obj->loc.room);
      }
    }
  }

  if ((cmd == CMD_HIT) &&
      IS_SET(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info, EX_BREAKABLE))
  {                             // destroy wall by hitting it
    int      dam;

    if (arg)
      argument_split_2(arg, arg1, arg2);
    else
      return FALSE;

    if (strcmp(arg1, "wall") || obj->value[1] != dir_from_keyword(arg2))
      return FALSE;

    if (type == WALL_OF_STONE ||
	      type == WALL_OF_BONES ||
             type == WATCHING_WALL ||
	      type == WALL_OF_ICE)
      dam = GET_DAMROLL(ch);
    else if (type == WATCHING_WALL &&
             illusionist &&
             IS_ALIVE(illusionist) &&
             GET_PID(illusionist) == obj->value[5])
    {
      dam = GET_DAMROLL(ch);
      if(ch->in_room != illusionist->in_room &&
         !number(0, 1))
      {
        send_to_char("&+WYou receive a vision from elsewhere.\r\n", illusionist);
        new_look(illusionist, "", CMD_LOOK, obj->loc.room);
      }
    }
    else if (type == WALL_OF_IRON)
      dam = GET_DAMROLL(ch) / 2;
    else if (type == WALL_OF_FORCE)
      dam = GET_DAMROLL(ch) / 3;
    else if (type == WALL_OUTPOST)
      dam = 1;
    else
      return FALSE;

    if (type != WALL_OUTPOST)
      dam += GET_LEVEL(ch) / 4;

    act("You try to destroy $p...", TRUE, ch, obj, 0, TO_CHAR);
    act("$n tries to destroy $p...", TRUE, ch, obj, 0, TO_NOTVICT);

    if (!ac_can_see_obj(ch, obj))
    {
      act("You barely feel the wall, but yet you try!", TRUE, ch, obj, 0,
          TO_CHAR);
      dam = dam / 3;
    }

    if (obj->value[2] < dam || IS_TRUSTED(ch) || (IS_PC(ch) && obj->value[5] == GET_PID(ch)) || (IS_NPC(ch) && obj->value[5] == GET_RNUM(ch))  )
    {
      act("Your mighty hit totally destroys $p...", TRUE, ch, obj, 0,
          TO_CHAR);
      act("$n's mighty hit totally destroys $p...", TRUE, ch, obj, 0,
          TO_NOTVICT);
      // level 70 ensures that its dispelled..
      spell_dispel_magic(70, ch, NULL, SPELL_TYPE_SPELL, 0, obj);
    }
    else
    {
      obj->value[2] -= dam;
      damage(ch, ch, MAX(GET_LEVEL(ch) / 2, GET_DAMROLL(ch) * 2 - dam),
             TYPE_UNDEFINED);
    }
    if (type == WALL_OUTPOST)
      CharWait(ch, PULSE_VIOLENCE);
    else
      CharWait(ch, PULSE_VIOLENCE * 4);
    return TRUE;
  }


  dircmd = cmd_to_exitnumb(cmd);

  /* we go further only if a direction command was executed */
  if (dircmd == -1)
    return FALSE;

  /* does this wall really block the attempted direction */
  if (obj->value[1] != dircmd)
    return FALSE;

  if (IS_TRUSTED(ch))
  {
    act("&+WYou ignore the physical limitations of the world.&n", TRUE, ch,
        obj, NULL, TO_CHAR);
    act("$n &+Wsteps through $p grinning.&n", TRUE, ch, obj, NULL, TO_ROOM);
    do_simple_move_skipping_procs(ch, dircmd, 0);
    return TRUE;
  }

  drag_followers = FALSE;
  was_in = ch->in_room;
  to_room = real_room0(obj->value[0]);
  dam = obj->value[2];

  if (to_room == NOWHERE)
  {
    send_to_char("Bug with a wall! Report this to a god.\n", ch);
    return FALSE;
  }

  if (IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("&+RYou enter what appears to be a magical wall...&n\n"
                 "&+RYou chuckle at the limitations of the material world.&n\n",
                 ch);
    do_simple_move_skipping_procs(ch, dircmd, 0);
    return TRUE;
  }

  if( !leave_by_exit(ch, dircmd) )
  {
    return TRUE;
  }

  switch (type)
  {
  case WALL_OF_FLAMES:
    sprintf(buffer, "$n &+Ris surrounded by flames as $e goes to the %s.",
            dirs[dircmd]);
    act(buffer, TRUE, ch, obj, NULL, TO_ROOM);
/* XXX */
    if (!ENJOYS_FIRE_DAM(ch))
	  {
      send_to_char("&+RYou enter into a wall of flames...OUCH!&n\n", ch);

      if (IS_AFFECTED(ch, AFF_PROT_FIRE))
        dam /= 3;

      if (IS_NPC(ch) && !IS_MORPH(ch) && !IS_PC_PET(ch))
        dam = 1;

      if (((GET_HIT(ch) - 8) < dam))
      {
        send_to_char
          ("&+RYou are overwhelmed by the heat and&n&+L fall into darkness...\n", ch);
        do_simple_move_skipping_procs(ch, dircmd, 0);
        act("$n &+Rfalls through the flames burnt to a crisp!&n", FALSE, ch,
          obj, NULL, TO_NOTVICT);
        die(ch, ch);
        return TRUE;
      }

      GET_HIT(ch) -= dam;
      spell_blindness(obj->value[4], ch, 0, SPELL_TYPE_SPELL, ch, NULL);
	    do_simple_move_skipping_procs(ch, dircmd, 0);
	    act("$n &+Rsteps through the flames!&n", TRUE, ch, NULL, NULL, TO_ROOM);
    }
  	else
    {
      send_to_char("&+RYou feel the healing power of the flames!&n\n", ch);
      GET_HIT(ch) = MIN(GET_HIT(ch) + dam, GET_MAX_HIT(ch));
      do_simple_move_skipping_procs(ch, dircmd, 0);
      act("$n &+Rsteps through the flames grinning!&n", TRUE, ch, NULL, NULL, TO_ROOM);
    }

    update_pos(ch);
    StartRegen(ch, EVENT_HIT_REGEN);
    drag_followers = TRUE;
    break;
/* XXX */

  case WALL_OF_ICE:
    if(IS_PC(ch) && has_innate( ch, INNATE_WALL_CLIMBING )
      && number(1,100) > 60 )
    {
      act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.", TRUE, ch, obj, 0, TO_ROOM);
      act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.", TRUE, ch, obj, 0, TO_CHAR);
      do_simple_move_skipping_procs(ch, dircmd, 0);
      if (IS_AFFECTED2(ch, AFF2_PROT_COLD))
        dam -= dam / 3;
      GET_HIT(ch) = MAX(GET_HIT(ch) - MAX(dam, 20), 1);
      update_pos(ch);
      return TRUE;
    }

    act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
    act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);

    if (IS_AFFECTED2(ch, AFF2_PROT_COLD))
      dam -= dam / 3;

    send_to_char("&+WYou shiver from the &+bfreezing cold &+Wof the &+Cice&+W!&n\n", ch);
    GET_HIT(ch) = MAX(GET_HIT(ch) - MAX(dam / 2, 20), 1);
    update_pos(ch);
    StartRegen(ch, EVENT_MOVE_REGEN);

    break;
  case LIGHTNING_CURTAIN:
    sprintf(buffer, "$n &+Bis surrounded by lightning as $e goes to the %s.&n", dirs[dircmd]);
    act(buffer, TRUE, ch, obj, NULL, TO_ROOM);

    if (IS_AFFECTED2(ch, AFF2_PROT_LIGHTNING))
      dam -= dam / 3;

    if (IS_NPC(ch) && !IS_MORPH(ch))
      dam = 1;

    if (((GET_HIT(ch) - 10) < dam))
    {
      send_to_char("&+BYou are shocked to death!&n\n", ch);
      do_simple_move_skipping_procs(ch, dircmd, 0);
      act("$n &+Bfalls through the curtain looking quite charred!&n", FALSE, ch, obj, NULL, TO_NOTVICT);
      die(ch, ch);
      return TRUE;
    }

    GET_HIT(ch) = MAX(GET_HIT(ch) - dam, 1);
    do_simple_move_skipping_procs(ch, dircmd, 0);
    act("$n &+Bsteps through the lightning curtain!&n", TRUE, ch, NULL, NULL, TO_ROOM);
    act("&+YOUCH!  &+BYou step through the lightning curtain!&n", TRUE, ch, NULL, NULL, TO_CHAR);
    update_pos(ch);
    StartRegen(ch, EVENT_HIT_REGEN);

    drag_followers = TRUE;
    break;

  case PRISMATIC_WALL:

    //if (GET_PID(ch) == obj->value[5])
     if((IS_PC(ch) && GET_PID(ch) == obj->value[5]) ||
        (IS_NPC(ch) && GET_RNUM(ch) == obj->value[5]))
    {
      act("You walk through your own wall.", TRUE, ch, obj, NULL, TO_CHAR);
      do_simple_move_skipping_procs(ch, dircmd, 0);
      act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
      return TRUE;
    }

    // let them walk through the wall 25% of the time
    if (!number(0, 3) && IS_PC(ch))
    {
      act("$p fades to shards of magic, and blows away...&n", TRUE, ch, obj, NULL, TO_ROOM);
      send_to_char("The prismatic creation fades into nothing.\n", ch);
      spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, 0, obj);
      act("You walk through the wall.", TRUE, ch, obj, NULL, TO_CHAR);
      act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
      do_simple_move_skipping_procs(ch, dircmd, 0);
      act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
      GET_HIT(ch) = MAX(1, GET_HIT(ch) - 100);
     return TRUE;
    }
    else if(IS_PC(ch) && has_innate( ch, INNATE_WALL_CLIMBING )
      && number(1,100) > 60 )
    {
      act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.", TRUE, ch, obj, 0, TO_ROOM);
      act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.", TRUE, ch, obj, 0, TO_CHAR);
      do_simple_move_skipping_procs(ch, dircmd, 0);
      return TRUE;
    }
    else
    {
      act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
      act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
    }

    if (!number(0, 3) && IS_PC_PET(ch))
    {
      act("You walk through the wall.", TRUE, ch, obj, NULL, TO_CHAR);
      act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
      do_simple_move_skipping_procs(ch, dircmd, 0);
      act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
      GET_HIT(ch) = MAX(1, GET_HIT(ch) - 100);

      return TRUE;
    }
    else
    {
      act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
      act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
    }

    dam = 0;
    spl = 0;

    switch (number(0, 6))
    {
    case 0:
      prepare_wall_messages("&+rred", char_message, room_message);
      dam = 100;
      break;
    case 1:
      prepare_wall_messages("&+Rorange", char_message, room_message);
      dam = 70;
      break;
    case 2:
      prepare_wall_messages("&+Yyellow", char_message, room_message);
      dam = 40;
      break;
    case 3:
      prepare_wall_messages("&+Bblue", char_message, room_message);
      spl = SPELL_MINOR_PARALYSIS;
      break;
    case 4:
      prepare_wall_messages("&+bindigo", char_message, room_message);
      spl = SPELL_FEEBLEMIND;
      break;
    case 5:
      prepare_wall_messages("&+ggreen", char_message, room_message);
      spl = SPELL_POISON;
      break;
    case 6:
      prepare_wall_messages("&+bazure", char_message, room_message);
      spl = SPELL_BLINDNESS;
      break;
    }

    if (spl)
    {
      act(char_message, TRUE, ch, obj, 0, TO_CHAR);
      act(room_message, TRUE, ch, obj, 0, TO_NOTVICT);
      skills[spl].spell_pointer(50, ch, 0, 0, ch, NULL);
    }
    else if (dam > 0)
    {
      if (NewSaves(ch, SAVING_SPELL, 0))
        dam >>= 1;
      if (IS_NPC(ch) && !IS_MORPH(ch))
        dam = 1;
      spell_damage(ch, ch, dam, SPLDAM_FIRE, 0, &messages);
    }

    return TRUE;

  case WEB:
    sprintf(buffer, "$n &+Wis enveloped in sticky webs as $e goes to the %s.",
            dirs[dircmd]);
    send_to_char("&+WYou enter into the web!&n\n", ch);
    act(buffer, TRUE, ch, obj, NULL, TO_ROOM);
    do_simple_move_skipping_procs(ch, dircmd, 0);
    act("$n &+Wsteps through the web&n", TRUE, ch, NULL, NULL, TO_ROOM);
    if (!NewSaves(ch, SAVING_PARA, 0))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_MINOR_PARALYSIS;
      af.flags = AFFTYPE_SHORT;
      af.duration = number(4, 15) * WAIT_SEC;
      af.bitvector2 = AFF2_MINOR_PARALYSIS;
      affect_to_char(ch, &af);
    }
    spell_dispel_magic(45 + GET_ALT_SIZE(ch), ch, NULL, SPELL_TYPE_SPELL, 0, obj);
    update_pos(ch);

    drag_followers = TRUE;
    break;

  case LIFE_WARD:
    if ( (IS_PC(ch) && GET_PID(ch) == obj->value[5]) || (IS_NPC(ch) && obj->value[5] == GET_RNUM(ch)) || !number(0, 3))
    {
      act("&+L$n&+L passes right through $p&+L unharmed.", TRUE, ch, obj, 0,
          TO_ROOM);
      act("&+LYou pass right through $p&+L unharmed.", TRUE, ch, obj, 0,
          TO_CHAR);
      do_simple_move_skipping_procs(ch, dircmd, 0);
      return TRUE;
    }
    else if(IS_PC(ch) && has_innate( ch, INNATE_WALL_CLIMBING )
      && number(1,100) > 60 )
    {
      act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.", TRUE, ch, obj, 0, TO_ROOM);
      act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.", TRUE, ch, obj, 0, TO_CHAR);
      do_simple_move_skipping_procs(ch, dircmd, 0);
      dam = number(10, 28);
      GET_VITALITY(ch) = MAX(GET_VITALITY(ch) - dam, 1);
      update_pos(ch);
      StartRegen(ch, EVENT_MOVE_REGEN);
      return TRUE;
    }
    act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
    act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);

    dam = number(8, 22);

    send_to_char
      ("&+LYour limbs go numb as you try to pass through the &n&+bnegative energy.&n\n",
       ch);
    GET_VITALITY(ch) = MAX(GET_VITALITY(ch) - dam, 1);
    update_pos(ch);
    StartRegen(ch, EVENT_MOVE_REGEN);
    drag_followers = TRUE;
    break;

  case ILLUSIONARY_WALL:
    if (!IS_SET(obj->extra_flags, ITEM_SECRET))
    {
      act("$p blows away&n", TRUE, ch, obj, NULL, TO_ROOM);
      send_to_char("The illusion dissipates.\n", ch);
      spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, 0, obj);
      return FALSE;
    }
    if (number(0, 5))
    {
      send_to_char("Alas, you cannot go that way. . . .\n", ch);
      return TRUE;
    }
    if (!IS_TRUSTED(ch))
    {
      send_to_char("It was just an illusion!!\n", ch);
      if (IS_SET(obj->extra_flags, ITEM_SECRET))
      {
        REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
      }
    }
    else
    {
      send_to_char("You see right through this petty illusion.\n", ch);
    }
    return FALSE;

  case WALL_OF_FORCE:
    /*if ((IS_PC(ch) && GET_PID(ch) == obj->value[5]) ||
        (IS_NPC(ch) && GET_RNUM(ch) == obj->value[5])) */
    if(IS_PC(ch))
    {
      act("&+L$n&+L passes right through $p&+L.", TRUE, ch, obj, 0, TO_ROOM);
      act("&+LYou pass right through $p&+L.", TRUE, ch, obj, 0, TO_CHAR);

      do_simple_move_skipping_procs(ch, dircmd, 0);
    }
    else if(IS_PC(ch) && has_innate( ch, INNATE_WALL_CLIMBING ) )
    {
      int rand1 = number(1, 100);
      if (rand1 > 60)
      {
        act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.", TRUE, ch, obj, 0, TO_ROOM);
        act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.", TRUE, ch, obj, 0, TO_CHAR);
        do_simple_move_skipping_procs(ch, dircmd, 0);
      }
    	else
    	{
        act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
        act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
    	}
    }
    else
    {
      act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
      act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
    }
    return TRUE;
  case WALL_OUTPOST:
  case WATCHING_WALL:
  case WALL_OF_IRON:
    if(IS_PC(ch) && has_innate( ch, INNATE_WALL_CLIMBING ) )
     {
      if( number(1, 100) > 60 )
      {
        act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.", TRUE, ch, obj, 0, TO_ROOM);
        act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.", TRUE, ch, obj, 0, TO_CHAR);
        do_simple_move_skipping_procs(ch, dircmd, 0);
      }
    	else
    	{
        act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
        act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
    	}
    }
    else
    {
      act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
      act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
    }
    return TRUE;
  case WALL_OF_STONE:
    if(IS_PC(ch) && has_innate( ch, INNATE_WALL_CLIMBING ) )
    {
      if( number(1, 100) > 60 )
      {
        act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.", TRUE, ch, obj, 0, TO_ROOM);
        act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.", TRUE, ch, obj, 0, TO_CHAR);
	      do_simple_move_skipping_procs(ch, dircmd, 0);
      }
    	else
    	{
      act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
      act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
    	}
    }
    else
    {
      act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
      act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
    }
    return TRUE;
  case WALL_OF_BONES:
    if (obj->value[2] < 10) /* Hackich assumption that if strength < 10 it's a thin dragonscale sheath */
	  {
	    if (obj->value[2] <= 1)
	    {
  	    act("You bump into $p, destroying it in the process!", TRUE, ch, obj, 0, TO_CHAR);
        act("$n bumps into $p, destroying it in the process!", TRUE, ch, obj, 0, TO_NOTVICT);
        // level 70 ensures that its dispelled..
        spell_dispel_magic(70, ch, NULL, SPELL_TYPE_SPELL, 0, obj);
	    }
      else if(IS_PC(ch) && has_innate( ch, INNATE_WALL_CLIMBING )
        && number(1,100) > 60 )
      {
        act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.", TRUE, ch, obj, 0, TO_ROOM);
        act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.", TRUE, ch, obj, 0, TO_CHAR);
        do_simple_move_skipping_procs(ch, dircmd, 0);
        return TRUE;
      }
	    else
	    {
  	    act("You bump into $p, visibly weakening it!", TRUE, ch, obj, 0, TO_CHAR);
        act("$n bumps into $p, visibly weakening it!", TRUE, ch, obj, 0, TO_NOTVICT);
        if (!number(0, 2))
          obj->value[2] -= 1;
	    }
    }
    else if(IS_PC(ch) && has_innate( ch, INNATE_WALL_CLIMBING )
      && number(1,100) > 60 )
    {
      act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.", TRUE, ch, obj, 0, TO_ROOM);
      act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.", TRUE, ch, obj, 0, TO_CHAR);
      do_simple_move_skipping_procs(ch, dircmd, 0);
    }
	  else  /* a "normal" wall of bones */
	  {
  	  act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
      act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
	  }
	  return TRUE;
  default:
    logit(LOG_DEBUG, "Wrong value[3] set in wall.");
    send_to_char("Serious screw-up on wall! Tell a god.\n", ch);
    return FALSE;
  }

  if (drag_followers && was_in != ch->in_room && ch->followers)
  {
    for (k = ch->followers; k; k = next_dude)
    {
      next_dude = k->next;

      if ((was_in == k->follower->in_room) && CAN_ACT(k->follower) &&
          MIN_POS(k->follower, POS_STANDING + STAT_RESTING) &&
          !IS_FIGHTING(k->follower) && !NumAttackers(k->follower) &&
          CAN_SEE(k->follower, ch))
      {
        act("You follow $N.", FALSE, k->follower, 0, ch, TO_CHAR);
        send_to_char("\n", k->follower);
        sprintf(Gbuf1, "%s %s", command[exitnumb_to_cmd(dircmd) - 1], arg);
        command_interpreter(k->follower, Gbuf1);
      }
    }
  }

  return TRUE;
}

int changelog(P_obj obj, P_char ch, int cmd, char *args)
{
  FILE    *f = NULL;
  char     o_buf[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  char     arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char    *ret;

  if( cmd != CMD_READ || GET_LEVEL(ch) < MINLVLIMMORTAL || !obj )
  {
    return FALSE;
  }

  if( !args )
  {
    send_to_char("The book of files contain: src, areas, bugs, typos, ideas, cheats, cheaters, donations.\n", ch);
    return TRUE;
  }

  o_buf[0] = '\0';

  half_chop(args, arg, arg2);
  if (!str_cmp(arg, "src"))
  {
    f = fopen("lib/information/changelog.src", "r");
  }
  else if (!str_cmp(arg, "bugs"))
  {
    f = fopen("lib/reports/bugs", "r");
  }
  else if (!str_cmp(arg, "typos"))
  {
    f = fopen("lib/reports/typos", "r");
  }
  else if (!str_cmp(arg, "ideas"))
  {
    f = fopen("lib/reports/ideas", "r");
  }
  else if (!str_cmp(arg, "cheats"))
  {
    f = fopen("lib/reports/cheats", "r");
  }
  else if (!str_cmp(arg, "cheaters"))
  {
    f = fopen("lib/reports/cheaters", "r");
  }
  else if (!str_cmp(arg, "areas"))
  {
    f = fopen("lib/information/changelog.areas", "r");
  }
  else if (!str_cmp(arg, "donations"))
  {
    f = fopen("logs/log/donation", "r");
  }
  else
  {
    return FALSE;
  }

  if( !f )
  {
    send_to_char("Could not open file.\nThe book of files contain: src, areas, bugs, typos, ideas, cheats, cheaters, donations.\n", ch);
    return TRUE;
  }

  do
  {
    ret = fgets(buf, MAX_STRING_LENGTH, f);
    if( ret )
    {
      if ((strlen(o_buf) + strlen(buf)) >= (MAX_STRING_LENGTH - 10))
      {
        ret = NULL;
      }
      else
      {
        if (isalpha(buf[0]))
          strcat(o_buf, "&+B");
        else
          strcat(o_buf, "&+c");
        strcat(o_buf, buf);
      }
    }
  }
  while (ret);
  strcat(o_buf, "\n");

  fclose(f);
  page_string(ch->desc, o_buf, 1);
  return TRUE;
}

int zarbon_shaper(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!OBJ_WORN_POS(obj, HOLD))
    return (FALSE);

	if (cmd == CMD_PERIODIC)
	{
		/* can either remove the next line or set the second number higher if it hums too often */
		if( !number(0,2) )
			hummer(obj);
		return TRUE;
    }


  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "blink"))
    {
      curr_time = time(NULL);

      if (obj->timer[1] + number(1, 5) <= curr_time)
      {
        act("You say 'blink'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly, and you feel your body begin to vibrate.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'blink'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly, and their body begins to vibrate violently!", TRUE, ch, obj, NULL, TO_ROOM);
        spell_blink(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[1] = curr_time;
        return TRUE;
      }
    }
	else if (isname(arg, "deflect"))
  {
	curr_time = time(NULL);

    if (obj->timer[2] + 500 <= curr_time)
    {
    act("You say 'deflect'", FALSE, ch, 0, 0, TO_CHAR);
    act("Your $q begins to send out ripples of pure magical energy!", FALSE, ch, obj,
        obj, TO_CHAR);
    act("$n says 'deflect'", FALSE, ch, obj, obj, TO_ROOM);
    act("$n's $q begins to send out ripples of pure magical energy!", TRUE, ch, obj, 0, TO_ROOM);
    if (ch->group)
      cast_as_area(ch, SPELL_DEFLECT, 50, 0);
    else
      spell_deflect(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
    obj->timer[2] = curr_time;
    return TRUE;
    }
  }
  	else
    return FALSE;
  }
  	curr_time = time(NULL);

   	if (obj->timer[0] + number(1, 30) <= curr_time)
   	{
    	int spell = memorize_last_spell(ch);

      if( spell )
      {
        char buf[256];
        sprintf( buf, "&+WYou feel your power of %s &+Wreturning to you.&n\n",
                 skills[spell].name );
        send_to_char(buf, ch);
		 		obj->timer[0] = curr_time;
		 		return FALSE;
	  }
	}

	return FALSE;
}

int rod_of_zarbon(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  P_obj    curse;
  char     e_pos;
  int      bad_owner;
  int      dam = (dice(8, 10) * 6);
  struct damage_messages messages = {
    "&+L$N &+Lturns pale as your $q &+Ldrains $S lifeforce, transferring it to you!&n",
    "&+LYour soul feels hollow, as the power of $n&+L's $q&+L saps your lifeforce!&n",
    "&+L$N &+Lscreams out in pain, as $S lifeforce is drained by $n&+L!&n",
    "", "", "", 0, obj};

  vict = (P_char) arg;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) || cmd != CMD_MELEE_HIT )
  {
    return FALSE;
  }

  /* Check class and sex for the extra special stuff */
  if( !GET_CLASS(ch, CLASS_CLERIC) || (GET_SEX(ch) != SEX_FEMALE) || GET_RACE(ch) != RACE_DROW )
  {
    bad_owner = TRUE;
  }
  else
  {
    bad_owner = FALSE;
  }

  e_pos = ((obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ?
           SECONDARY_WEAPON : 0);

  /* must be wielded */
  if( !e_pos )
  {
    return FALSE;
  }

  /* The extra-special fancy-dancy power */
  // 1/100 chance.
  if( !bad_owner && !number(0, 99) )
  {
    curse = read_object(36761, VIRTUAL);
    if( curse )
    {
      obj_to_char(curse, vict);
    }
  }
  /* If we scored a _really_ nice hit, let's do some spell shit */
  else if( CheckMultiProcTiming(ch) && !number(0, 32) ) // 3%
  {
    act("Your $q flares up upon hitting $N!", FALSE, ch, obj, vict, TO_CHAR);
    act("$n's $q flares up upon hitting $N!", FALSE, ch, obj, vict, TO_NOTVICT);
    act("$n's $q flares up upon hitting you!", FALSE, ch, obj, vict, TO_VICT);

    spell_damage(ch, vict, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT | RAWDAM_NOKILL, &messages);

    vamp(ch, dam / 4, (int) (GET_MAX_HIT(ch) * 1.1));

    if( GET_VITALITY(vict) >= 25 && !number(0, 2) )
    {
      act("&+rYour $q &+mFLARES&n&+r, tapping the vigor of $N&+r!&n", FALSE, ch, obj, vict, TO_CHAR);
      act("&+r$n&+r's $q &+mFLARES&n&+r, draining $N&+r's vigor!&n", FALSE, ch, obj, vict, TO_NOTVICT);
      act("&+r$n&+r's $q &+mFLARES&n&+r, draining your vigor!&n", FALSE, ch, obj, vict, TO_VICT);

      GET_VITALITY(vict) -= (dam / 9);
      GET_VITALITY(ch) += (dam / 9);

			StartRegen(ch, EVENT_MOVE_REGEN);
      StartRegen(vict, EVENT_MOVE_REGEN);
    }
  }
  return TRUE;
}

/* flaming mace of the Ruzdo #75561 */

int flaming_mace_ruzdo(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
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

  if (!OBJ_WORN_POS(obj, WIELD))
    return (FALSE);

  vict = (P_char) arg;

  if (!vict)
    return (FALSE);

  if (obj->loc.wearing == ch)
  {
    if (!number(0, 30))
    {
      act
        ("Your $q glows brightly as a &+Yblast of pure light&N streaks out of it.",
         FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
      act
        ("$n's $q glows brightly as a &+Yblast of pure light&N streaks out of it!",
         FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
      spell_sunray(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);

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

int sword_named_magik(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) )
  {
    return FALSE;
  }

  vict = (P_char) arg;

  if( !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  if( OBJ_WORN_BY(obj, ch) )
  {
    // 1/30 chance.
    if( !number(0, 29) )
    {
      act("&+BYour $q engulfs $N  in its bright &+bblue &+Baura!&N", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
      act("&+B$n's $q engulfs $N  in its bright &+bblue &+Baura!&N", FALSE, obj->loc.wearing, obj, vict, TO_ROOM);
      spell_dispel_magic(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    else
    {
      if( !ch->specials.fighting )
      {
        set_fighting(ch, vict);
      }
    }
  }
  if( ch->specials.fighting )
  {
    return (FALSE);
  }
  else
  {
    return (TRUE);
  }
}

int yuan_ti_stone(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict = NULL, vict_next = NULL;

  /* check for periodic event calls */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch || !obj || cmd)
    return FALSE;

  /* Doesnt work in the chest */
  if (OBJ_NOWHERE(obj) || OBJ_INSIDE(obj))
    return FALSE;

  /* annoying hum, constant */
  act("An intense humming sound can be heard from $p&n.",
      FALSE, ch, obj, 0, TO_ROOM);

  return FALSE;
}

int trans_tower_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000, curr_time;
  P_char   victim;

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!ch && cmd == CMD_PERIODIC)
  {
    hummer(obj);
    return TRUE;
  }

  if (!ch)
    return FALSE;

  if (!OBJ_WORN_BY(obj, ch))
    return (FALSE);

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "stone"))
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 60 <= curr_time)
      {
        act("You say 'stone'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'stone'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_stone_skin(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
  }

  if (obj->loc.wearing->equipment[WIELD] != obj)
    return (FALSE);

  if (!dam)
    return FALSE;

  victim = (P_char) arg;
  if (!victim)
    return (FALSE);
  if (number(0, 20))
    return (FALSE);
  act
    ("$n's $q &+Wglows white&n, unleashing a massive ball of ice towards $N!",
     TRUE, ch, obj, victim, TO_NOTVICT);
  act
    ("Your $q &+Wglows white&n, unleashing a massive ball of ice towards $N!",
     TRUE, ch, obj, victim, TO_CHAR);
  act
    ("$n's $q &+Wglows white&n, unleashing a massive ball of ice towards _YOU_!",
     TRUE, ch, obj, victim, TO_VICT);
  spell_harm(50, ch, NULL, 0, victim, obj);
  spell_arieks_shattering_iceball(30, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  return (TRUE);
}

#ifdef THARKUN_ARTIS
int trans_tower_shadow_globe(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int      curr_time = time(NULL);

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (ch || cmd || !OBJ_WORN(obj))
    return FALSE;

  ch = obj->loc.wearing;

  if (!has_skin_spell(ch) &&
      obj->timer[0] + get_property("timer.stoneskin.generic", 60) < curr_time)
  {
    hummer(obj);
    spell_stone_skin(45, ch, 0, SPELL_TYPE_POTION, ch, 0);
    obj->timer[0] = curr_time;
  }

  if (IS_FIGHTING(ch))
  {
    if (!number(0, 3))
    {
      act("&+LYour $p throbs as an inky black darkness flows from it!&N",
          FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
      act("&+L$n's $p pulses as an inky black darkness flows from it!&N",
          FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
        if (!grouped(vict, ch) && !IS_TRUSTED(vict) && ch != vict)
          spell_wither(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
  }

  return FALSE;
}

#else

int trans_tower_shadow_globe(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict, temp;
  int      curr_time;

  vict = (P_char) arg;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }
  if (ch || cmd)
    return FALSE;

  if (!OBJ_WORN(obj))           /* Most things don't work in a sack... */
    return FALSE;

  ch = obj->loc.wearing;

  if (!has_skin_spell(ch))
  {
    hummer(obj);
    spell_stone_skin(45, ch, 0, SPELL_TYPE_POTION, ch, 0);
  }

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "invisible"))
    {
      curr_time = time(NULL);

      if (obj->timer[0] + 60 <= curr_time)
      {
        act("You say 'invisible'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'invisible'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_improved_invisibility(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
  }

  if (IS_FIGHTING(ch))
  {                             /* Check again, for the halibut */
    if (!number(0, 3))
    {
      act("&+LYour $p throbs as an inky black darkness flows from it!&N",
          FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
      act("&+L$n's $p pulses as an inky black darkness flows from it!&N",
          FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
      for (vict = world[ch->in_room].people; vict; vict = temp)
      {
        temp = vict->next_in_room;
        if (((vict->group != ch->group) && !IS_TRUSTED(vict)) || (!ch->group))
          if (ch != vict)
            spell_wither(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
      }
    }
  }

  return FALSE;
}

#endif

/* object burns on all commands I can think of that involve touching it */

int burn_touch_obj(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch)
    return FALSE;

  if (IS_TRUSTED(ch) || !CAN_SEE_OBJ(ch, obj))
    return FALSE;

  switch (cmd)
  {
  case CMD_TOUCH:
  case CMD_PULL:
  case CMD_PUSH:
  case CMD_MOVE:
  case CMD_USE:
  case CMD_TUG:
  case CMD_OPEN:
  case CMD_CLOSE:
  case CMD_RUB:
  case CMD_HOLD:
    act("YOW!  $p burns the hell out of you as you touch it!", FALSE, ch, obj,
        0, TO_CHAR);
    act("$n recoils in pain as $e is burned by touching $p.", FALSE, ch, obj,
        0, TO_ROOM);

    dam = 20;
    if (IS_AFFECTED(ch, AFF_PROT_FIRE))
      dam >>= 1;
    if (GET_RACE(ch) == RACE_TROLL)
      dam <<= 2;

    if (damage(ch, ch, dam, TYPE_UNDEFINED))
      return TRUE;

    if (GET_ITEM_TYPE(obj) == ITEM_SWITCH)
      return item_switch(obj, ch, cmd, arg);
    else
      return FALSE;

  default:
    return FALSE;
  }

  return FALSE;
}

/* stat pools, level 51+, drink mods stats by -3 to +5 (up to 100)
   (once a week), sip mods stats by -1 to +2 (up to 100..), twice a
   week (RL) */

#define STAT_POOL_SIP_MIN   1
#define STAT_POOL_SIP_MAX   3

#define STAT_POOL_DRINK_MIN   1
#define STAT_POOL_DRINK_MAX   3

int stat_pool_common(P_obj obj, P_char ch, int cmd, sh_int * statPtr,
                     const char *minusMsgCh, const char *minusMsgRoom,
                     const char *plusMsgCh, const char *plusMsgRoom)
{
  int      numb, oldStat;
  struct affected_type *af2;

  if (!ch || !IS_PC(ch))
    return FALSE;

  switch (cmd)
  {
  /*case CMD_SIP:
    if (GET_LEVEL(ch) < 51 || affected_by_spell(ch, TAG_POOL) )
    {
      send_to_char("The liquid burns your throat!  Ouch!\n", ch);
      act("$n reels in pain as $e takes a sip from $p!", FALSE,
          ch, obj, 0, TO_ROOM);

      damage(ch, ch, TYPE_UNDEFINED, 25);

      return TRUE;
    }

    numb = number(STAT_POOL_SIP_MIN, STAT_POOL_SIP_MAX);
    break;
  */
  case CMD_DRINK:
    if (GET_LEVEL(ch) < 51 )
    {
      send_to_char("You are much to lowly to even dream of drinking from that!\n", ch);
      return TRUE;
    }

    if ((af2 = get_spell_from_char(ch, TAG_POOL)) != NULL)
    {
      if ((af2->modifier + (60 * 60 * 24 * 2)) > time(NULL))
      {
        send_to_char("The liquid burns your throat!  Ouch!\n", ch);
        act("$n reels in pain as $e takes a drink from $p!", FALSE,
            ch, obj, 0, TO_ROOM);

        damage(ch, ch, TYPE_UNDEFINED, 50);

        return TRUE;
      }
    }

    numb = number(STAT_POOL_DRINK_MIN, STAT_POOL_DRINK_MAX);
    break;

  default:
    return FALSE;
  }
  
  affect_from_char(ch, TAG_POOL);

  struct affected_type af;
  bzero(&af, sizeof(af));
  af.type = TAG_POOL;
  af.flags = AFFTYPE_STORE | AFFTYPE_PERM;
  //af.duration = (int) (get_property("timer.mins.statPool", SECS_PER_REAL_HOUR * 24));
  af.duration = -1;
  af.modifier = time(NULL);
  affect_to_char(ch, &af);

  act("$n takes a drink from $p.", FALSE, ch, obj, 0, TO_ROOM);
  send_to_char("You take a drink...\n", ch);

  // heal em!

  if (GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    send_to_char("The cool waters of the pool heal your wounds!\n", ch);
    act("$n's wounds appear to have been healed!", FALSE, ch, 0, 0, TO_ROOM);

    GET_HIT(ch) = GET_MAX_HIT(ch);
  }

  // modify stat!

  oldStat = *statPtr;
  *statPtr = BOUNDED(1, (*statPtr) + numb, 100);
  numb = (*statPtr) - oldStat;

  if (numb == 0)
  {
    send_to_char("Strange..  You don't feel any different..\n", ch);
  }
  else if (numb < 0)
  {
    send_to_char(minusMsgCh, ch);
    act(minusMsgRoom, FALSE, ch, 0, 0, TO_ROOM);

    affect_total(ch, TRUE);
  }
  else
  {
    send_to_char(plusMsgCh, ch);
    act(plusMsgRoom, FALSE, ch, 0, 0, TO_ROOM);

    affect_total(ch, TRUE);
  }

  return TRUE;
}

int spell_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time, rannum;
  typedef void (*spell_func_ptr) (int, P_char, char *, int, P_char, P_obj);
  spell_func_ptr spells[9] = {
    spell_lifelust,
    spell_fly,
    spell_lionrage,
    spell_hawkvision,
    spell_armor,
    spell_detect_invisibility,
    spell_biofeedback,
    spell_greater_spirit_ward,
    spell_baladors_protection
  };

  void     (*spell_func) (int, P_char, char *, int, P_char, P_obj);

  rannum = number(0, 8);

  if (cmd == CMD_SET_PERIODIC)
  {
    curr_time = time(NULL);

    obj->value[0] = rannum;
    obj->timer[0] = curr_time;

    return TRUE;
  }

  spell_func = spells[obj->value[0]];

  curr_time = time(NULL);

  if (obj->timer[0] + (60 * 40) <= curr_time)
  {
    spell_func = spells[rannum];

    obj->value[0] = rannum;
    obj->timer[0] = curr_time;
  }

  if (cmd != CMD_DRINK)
    return FALSE;

  spell_func(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);

  return TRUE;
}

int stat_pool_str(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Str),
                          "&+LYou feel weaker.\n",
                          "$n's body momentarily sags, as though overburdened.",
                          "&+WYou feel stronger!\n",
                          "$n's muscles seem to grow for a brief instant.");
}

int stat_pool_dex(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Dex),
                          "&+LYou feel less dextrous.\n",
                          "You somehow get the sense that $n won't be so good at piano-playing anymore.",
                          "&+WYou feel more dextrous!\n",
                          "$n nimbly moves $s fingers with newfound dexterity.");
}

int stat_pool_agi(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Agi),
                          "&+LYou feel less agile.\n",
                          "$n takes on a more clumsy appearance than before.",
                          "&+WYou feel more agile!\n",
                          "$n's body suddenly appears more flexible than ever - $e does 23 backflips in a row!");
}

int stat_pool_con(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Con),
                          "&+LYou suddenly feel less healthy.\n",
                          "$n's countenance takes on a more unhealthy appearance.",
                          "&+WYou feel ten years younger!\n",
                          "$n's countenance takes on a more healthy appearance.");
}

int stat_pool_pow(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Pow),
                          "&+LYou suddenly feel less able to use the power of your mind.\n",
                          "$n sags noticably under the weight of the world.",
                          "&+WYour mind suddenly feels ten times as powerful!\n",
                          "$n's eyes take on a glimmering sheen as $e looks up from the pool.");
}

int stat_pool_int(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Int),
                          "&+LYou suddenly feel stupider...  You think.\n",
                          "$n suddenly looks utterly lost and confused.",
                          "&+WYou feel smarter!  Man, you were a real dumbass before.\n",
                          "$n smiles and recites a poem in a language you do not even recognize!");
}

int stat_pool_wis(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Wis),
                          "&+LYou feel as though some of your hard-won knowledge has slipped away..\n",
                          "$n has a momentary look of utter confusion.",
                          "&+WYou feel wiser!\n", 
                          "$n begins to admonish younger people around $m.");
}

int stat_pool_cha(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Cha),
                          "&+LYou don't feel any different, really..\n",
                          "You thought $n looked ugly before, but now $e looks even uglier.",
                          "&+WYou don't feel any different, really..\n",
                          "Wow, you never noticed before, but $n is kinda sexy.");
}

int stat_pool_luc(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Luk),
                          "&+LYour outlook on life is somewaht grimmer..\n",
                          "$n doesn't look so confident in believing in his lucky stars.\n",
                          "&+WYou feel as if you could roll Triple Tiamat's at the slots..\n",
                          "$n looks to have the confidence that life is going his way.\n");
}

int druid_sabre(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
}

int druid_spring(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DRINK)
  {
    if (!arg || !*arg)
      return FALSE;
    arg = skip_spaces(arg);
    if (*arg && !strcmp(arg, "spring"))
    {
      act("You drink from $p.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n drinks from $p.", FALSE, ch, obj, 0, TO_ROOM);

      if (obj->value[0] >= 51)
        spell_regeneration(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
//      if (obj->value[0] >= 41)
//        spell_endurance(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
      if (obj->value[0] >= 36)
        spell_aid(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);

      if (obj->value[0] >= 31)
      {
        spell_natures_touch(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      else
      {
        spell_cure_serious(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      spell_invigorate(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
      CharWait(ch, PULSE_VIOLENCE);
      return TRUE;
    }
  }

  if (cmd == CMD_DECAY)
  {
    if (world[obj->loc.room].people)
    {
      act("$p rapidly shrinks in size until finally it disappears entirely.",
        0, world[obj->loc.room].people, obj, 0, TO_ROOM);
      act("$p rapidly shrinks in size until finally it disappears entirely.",
        0, world[obj->loc.room].people, obj, 0, TO_CHAR);
    }
    return TRUE;
  }

  return FALSE;
}

int blighter_pond(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DRINK)
  {
    if (!arg || !*arg)
      return FALSE;
    arg = skip_spaces(arg);
    if( *arg && !strcmp(arg, "pond") )
    {
      if( obj->value[0] < 31 )
      {
        CharWait( ch, WAIT_SEC );
        return FALSE;
      }

      act("You drink from $p.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n drinks from $p.", FALSE, ch, obj, 0, TO_ROOM);

      if( obj->value[0] >= 31 )
      {
        spell_drain_nature(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
//      if (obj->value[0] >= 41)
//        spell_sap_nature(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
      if( obj->value[0] >= 51 )
      {
        spell_regeneration(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      if( GET_VITALITY(ch) < GET_MAX_VITALITY(ch) )
      {
        spell_invigorate(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      CharWait(ch, PULSE_VIOLENCE);
      return TRUE;
    }
  }

  if (cmd == CMD_DECAY)
  {
    if (world[obj->loc.room].people)
    {
      act("$p rapidly shrinks in size until finally it disappears entirely.",
        0, world[obj->loc.room].people, obj, 0, TO_ROOM);
      act("$p rapidly shrinks in size until finally it disappears entirely.",
        0, world[obj->loc.room].people, obj, 0, TO_CHAR);
    }
    return TRUE;
  }

  return FALSE;
}

int splinter(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if ( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if (arg && (cmd == CMD_RUB))
  {
    if (isname(arg, "splinter"))
    {
      curr_time = time(NULL);
      // Every 15 minutes.
      if (obj->timer[0] + (60 * 15) <= curr_time)
      {
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_cure_critic(45, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_cure_critic(45, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }
  return FALSE;
}

int church_door(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_RUB) )
  {
    if (isname(arg, "glass"))
    {
      curr_time = time(NULL);

      if( obj->timer[0] + 15 <= curr_time && !affected_by_spell(ch, SPELL_ARMOR) )
      {
        act("$p &N&+rhums&N&+y briefly as you are &+Wenveloped&N&+y in a &+Bmagical&+C force&+R field.&N", FALSE, ch, obj, obj, TO_CHAR);
        act("$p &N&+rhums&+y briefly as $n is &+Wenveloped&N&+y in a &+Bmagical &+Cforce&+R field.&N ", TRUE, ch, obj, NULL, TO_ROOM);
        spell_armor(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_bless(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        act("&+yThe &+Cforce &+Rfield&N&+y subsides and you feel able to better withstand your foes.&N", TRUE, ch, obj, NULL, TO_CHAR);
        obj->timer[0] = curr_time;
      }
      else
      {
        act("$p &N&+rhums&N&+y briefly and is quiet.&N", FALSE, ch, obj, obj, TO_CHAR);
        act("$p &N&+rhums&+y briefly and is quiet.&N ", TRUE, ch, obj, NULL, TO_ROOM);
      }
      return TRUE;
    }
  }
  return FALSE;
}

int earthquake_gauntlet(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      cur_time;
  P_char   vict = NULL;
  struct affected_type *af;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_WORN_POS(obj, WEAR_HANDS) || cmd != CMD_PERIODIC || ch )
  {
    return FALSE;
  }
  ch = obj->loc.wearing;
  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  // 1/10 chance
  if( IS_FIGHTING(ch) && !number(0, 9) )
  {
    act("&+y$n's $q &+ydrive into the ground causing the earth to buckle and break!&N",
      TRUE, ch, obj, vict, TO_ROOM);
    act("&+yYour $q &+ydrive into the ground causing the earth to buckle and break!&N",
      TRUE, ch, obj, vict, TO_CHAR);
    spell_earthquake(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    return TRUE;
  }

  return FALSE;
}

int blind_boots(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      cur_time;
  P_char   vict = NULL;
  struct affected_type af;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_WORN_POS(obj, WEAR_FEET) || ch || cmd != CMD_PERIODIC )
  {
    return FALSE;
  }
  ch = obj->loc.wearing;
  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  // 1/25 chance == 4%
  if( IS_FIGHTING(ch) && !number(0, 24) )
  {
    vict = ch->specials.fighting;

    if( affected_by_spell(vict, SPELL_BLINDNESS) || (GET_RACE(vict) == RACE_DRAGON)
      || (GET_RACE(vict) == RACE_DEMON) || (GET_RACE(vict) == RACE_DEVIL)
      || IS_IMMATERIAL(vict) || IS_ELEMENTAL(vict) || EYELESS(vict) || IS_ELITE(vict) )
    {
      return TRUE;
    }

    act("&+y$n &+ykicks up a cloud of dust, obscuring your vision!&N",
      TRUE, ch, obj, vict, TO_VICT);
    act("&+y$n &+ykicks up a cloud of dust at $N.&N",
      TRUE, ch, obj, vict, TO_NOTVICT);
    act("&+yYou &+ykick up a cloud of dust at $N!&N",
      TRUE, ch, obj, vict, TO_CHAR);
    blind(ch, vict, number(1, 3) * PULSE_VIOLENCE);
    return TRUE;
  }

  return FALSE;
}

// 56 ZONE PROCS BELOW HERE
int transparent_blade(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;
  struct damage_messages messages = {
    "Your $q &Nbu&n&+Brsts in&+Wto a haz&n&+be of blue light, enveloping $N i&+Bn an ether&+Weal cloud.&n",
    "&+W$n's&N $q bu&n&+Brsts in&+Wto a haz&n&+be of blue light, enveloping you i&+Bn an ether&+Weal cloud.&n",
    "&+W$n's&N $q bu&n&+Brsts in&+Wto a haz&n&+be of blue light, enveloping $N i&+Bn an ether&+Weal cloud.&N",
    "Your $q &Nbu&n&+Brsts in&+Wto a haz&n&+be of blue light, killing $N wi&+Bth an ether&+Weal cloud.&n",
    "&+W$n's&N $q bu&n&+Brsts in&+Wto a haz&n&+be of blue light, your soul is torn directly from your dying body..",
    "&+W$n's&N $q bu&n&+Brsts in&+Wto a haz&n&+be of blue light, killing $N wi&+Bth an ether&+Weal cloud.&N",
      0, obj
  };

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }
  spell_damage(ch, victim, 75 * 4, SPLDAM_SPIRIT, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &messages);
  return TRUE;
}

int serpent_of_miracles(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      rand;
  int      curr_time;
  P_char   victim;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }

  curr_time = time(NULL);
  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "wish") )
    {
      // 2min 40sec timer.
      if( obj->timer[0] + 160 <= curr_time )
      {
        rand = number(0, 7);
        switch( rand )
        {
        case 0:
          act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and infuse&n&+rs you with magic.&n", TRUE, ch, obj, victim, TO_CHAR);
          spell_globe(20, ch, 0, 0, ch, 0);
          break;
        case 1:
          act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and infuse&n&+rs you with magic.&n", TRUE, ch, obj, victim, TO_CHAR);
          spell_detect_invisibility(20, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          break;
        case 2:
          act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and infuse&n&+rs you with magic.&n", TRUE, ch, obj, victim, TO_CHAR);
          spell_haste(20, ch, 0, 0, ch, 0);
          break;
        case 3:
          act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and infuse&n&+rs you with magic.&n", TRUE, ch, obj, victim, TO_CHAR);
          spell_fly(20, ch, NULL, 0, ch, 0);
          break;
        case 4:
          act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and ble&n&+rsses you with ho&+Wly power.&n", TRUE, ch, obj, victim, TO_CHAR);
          spell_regeneration(20, ch, NULL, 0, ch, 0);
          break;
        case 5:
          act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and ble&n&+rsses you with ho&+Wly power.&n", TRUE, ch, obj, victim, TO_CHAR);
          spell_armor(20, ch, 0, 0, ch, 0);
          break;
        case 6:
          act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and ble&n&+rsses you with ho&+Wly power.&n", TRUE, ch, obj, victim, TO_CHAR);
          spell_soulshield(20, ch, 0, 0, ch, 0);
          break;
        case 7:
          act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and ble&n&+rsses you with ho&+Wly power.&n", TRUE, ch, obj, victim, TO_CHAR);
          spell_pantherspeed(20, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
          break;
        }
        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }
  return FALSE;

}

/* THIS IS NEWBIE ZONE  STREAM OF LIFE*/
int life_of_stream(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      bits;
  P_char   tempch;
  P_obj    tempobj;
  int      r_room, rand;

  if( cmd != CMD_ENTER || !arg || !IS_ALIVE(ch) || !*arg )
    return FALSE;

  if( !strcmp(arg, "stream") )
  {
    if( affected_by_spell(ch, TAG_LIFESTREAMNEWBIE) )
    {
      send_to_char("Your not ready yet, ask the paladin about racewar.\n", ch);
      return TRUE;
    }

    send_to_char("You feel refreshed as you enter the realm of life.\n", ch);
    GET_HIT(ch) = GET_MAX_HIT(ch);
    r_room = real_room(GET_BIRTHPLACE(ch));
    char_from_room(ch);
    char_to_room(ch, r_room, -1);
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

int newbie_sign1(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      bits;
  P_char   tempch;
  P_obj    tempobj;
  int      r_room, rand;
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;



  if (!ch || cmd == CMD_PERIODIC || !arg)
    return FALSE;
  one_argument(arg, Gbuf1);

  if (arg && (cmd == CMD_LOOK || cmd == CMD_EXAMINE || cmd == CMD_LOOK))
  {
    if (!isname(arg, "sign"))
      return FALSE;
    do_look(ch, "sign", -4);
    spell_armor(50, ch, 0, 0, ch, 0);
    return TRUE;

  }
  else
  {
    return FALSE;
  }
}

// Second Sign
int newbie_sign2(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      bits;
  P_char   tempch;
  P_obj    tempobj;
  int      r_room, rand;
  char     Gbuf1[MAX_STRING_LENGTH];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;



  if (!ch || cmd == CMD_PERIODIC || !arg)
    return FALSE;
  one_argument(arg, Gbuf1);


  if (arg && (cmd == CMD_LOOK || cmd == CMD_EXAMINE || cmd == CMD_LOOK))
  {
    if (!isname(arg, "sign"))
      return FALSE;

    do_look(ch, "sign", -4);
    spell_bless(50, ch, 0, 0, ch, 0);
    return TRUE;

  }
  else
  {
    return FALSE;
  }
}

int madman_shield(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // 1/10 chance.
  if( cmd != CMD_GOTHIT || number(0, 9) )
  {
    return FALSE;
  }

  // important! can do this cast (next line) ONLY if cmd was CMD_GOTHIT or CMD_GOTNUKED
  if( !(data = (struct proc_data *) arg) )
  {
    return FALSE;
  }
  if( !(vict = data->victim) )
  {
    return FALSE;
  }

  act("$N is burned by magical fire as he strikes the $q!", FALSE, ch, obj, vict, TO_NOTVICT);
  act("Your $q pulsates with &+Wmagical fire&n burning $N!", FALSE, ch, obj, vict, TO_CHAR);
  act("$n's $q flares up and burns you with magical fire.", FALSE, ch, obj, vict, TO_VICT);
  damage(ch, vict, (40 + number(0, 40)), TYPE_UNDEFINED);
  return TRUE;
}

int madman_mangler(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    ripostes;
  P_char victim;
  struct proc_data *data;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_REMOVE && isname(arg, obj->name) )
  {
    affect_from_char(ch, SPELL_BLUR);
    REMOVE_BIT(ch->specials.affected_by3, AFF3_BLUR);
    return FALSE;
  }

  // 1/20 chance.
  if( cmd == CMD_GOTHIT && !number(0, 19) )
  {
    if( !(data = (struct proc_data *) arg) )
    {
      return FALSE;
    }
    victim = data->victim;
    if( !IS_ALIVE(victim) )
    {
      return FALSE;
    }

    if( number(0, 1) )
    {
      ripostes = 1;
    }
    else if( number(0, 4) >= 2 )
    {
      ripostes = 2;
    }
    else
    {
      ripostes = 3;
    }

    act("$n's $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim, TO_NOTVICT | ACT_NOTTERSE);
    act("$n's $q deflects your blow and strikes YOU!", TRUE, ch, obj, victim, TO_VICT | ACT_NOTTERSE);
    act("Your $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim, TO_CHAR | ACT_NOTTERSE);
    do
    {
      hit(ch, victim, obj);
    }
    while( --ripostes && char_in_list(victim) && char_in_list(ch) );

    return TRUE;
  }

  if( cmd != CMD_MELEE_HIT )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }
  act("&+L$n's&N $q &n&+Lgoes &+rmad...&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+LYour&N $q &n&+Lgoes &+rmad...&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+L$n's&N $q &n&+Lgoes &+rmad...&N", TRUE, ch, obj, victim, TO_VICT);
  spell_blur(60, ch, 0, 0, ch, 0);
  return TRUE;
}

//Thanks giving procs -Kvark
int tripboots(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    cur_time;
  P_char vict = NULL;
  int    rand;
  struct affected_type af;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WEAR_FEET) || cmd != CMD_PERIODIC || ch != obj->loc.wearing )
  {
    return FALSE;
  }

  // 1/10 chance.
  if( IS_FIGHTING(ch) && !number(0, 9) )
  {
    rand = number(1, 10);
    // 70% chance.
    if( rand < 8 )
    {
      vict = ch->specials.fighting;
      act("&N$n sweep sends you crashing to the ground!&N", TRUE, ch, obj, vict, TO_VICT);
      act("&N$n sweep sends $N crashing to the ground!!&N", TRUE, ch, obj, vict, TO_NOTVICT);
      act("&NYour sweep sends $N crashing to the ground!&N", TRUE, ch, obj, vict, TO_CHAR);
      SET_POS(vict, POS_SITTING + GET_STAT(vict));
      CharWait(vict, PULSE_VIOLENCE * 2);
      return TRUE;
    }
    else
    {
      vict = ch->specials.fighting;
      act("&N$n falls like a drunk turkey to the ground!&N", TRUE, ch, obj, vict, TO_VICT);
      act("&N$n falls like a drunk turkey to the ground!&N", TRUE, ch, obj, vict, TO_NOTVICT);
      act("&NIn your haste to sweep $N off the ground, you fall to the ground in embarrassment! &N", TRUE, ch, obj, vict, TO_CHAR);
      SET_POS(ch, POS_SITTING + GET_STAT(ch));
      CharWait(ch, PULSE_VIOLENCE * 2);
      return (TRUE);
    }
  }
  return (FALSE);
}

int blindbadge(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      cur_time;
  P_char   vict = NULL;
  struct affected_type af;
  int      rand;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_WORN_POS(obj, GUILD_INSIGNIA) || !IS_ALIVE(ch) || ch != obj->loc.wearing || cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  // 1/6 chance.
  if( IS_FIGHTING(ch) && !number(0, 5) )
  {
    vict = ch->specials.fighting;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }
    rand = number(1, 10);
    // 70% chance.
    if (rand < 8)
    {
      act("&N$n's $q &+ysends out a stream of &+Ylight&+y towards you!&N", TRUE, ch, obj, vict, TO_VICT);
      act("&N$n's $q &+ysends out a stream of &+Ylight&+y towards $N!", TRUE, ch, obj, vict, TO_NOTVICT);
      act("&NYour $q &+ysends out a stream of &+Ylight&+y towards $N!&N", TRUE, ch, obj, vict, TO_CHAR);
      send_to_char("&+LYou have been blinded!\n", vict);
      act("&+L$N seems to been blinded!.&N", TRUE, ch, obj, vict, TO_NOTVICT);
      act("&+L$N seems to been blinded!.&N", TRUE, ch, obj, vict, TO_CHAR);
      bzero(&af, sizeof(af));
      af.type = SPELL_BLINDNESS;
      af.bitvector = AFF_BLIND;
      af.duration = 0;
      affect_to_char(vict, &af);
      return TRUE;
    }
    else
    {
      vict = ch->specials.fighting;
      act("&+y$n &+m's $q &+yfalls down over his eyes!&n", TRUE, ch, obj, vict, TO_VICT);
      act("&+y$n &+m's $q &+yfalls down over his eyes!&n", TRUE, ch, obj, vict, TO_NOTVICT);
      act("&+yYour $q &+yfalls down over your eyes!&n", TRUE, ch, obj, vict, TO_CHAR);
      send_to_char("&+LYou have been blinded!\n", ch);
      act("&+L$N seems to been blinded!.&N", TRUE, vict, obj, ch, TO_NOTVICT);
      act("&+L$N seems to been blinded!.&N", TRUE, vict, obj, ch, TO_CHAR);
      bzero(&af, sizeof(af));
      af.type = SPELL_BLINDNESS;
      af.bitvector = AFF_BLIND;
      af.duration = 0;
      affect_to_char(ch, &af);
      return TRUE;
    }
  }

  return FALSE;
}

int confusionsword(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;
  int      rand;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }

  rand = number(1, 10);
  // 70% chance.
  if (rand < 8)
  {
    act("&n$n's&N $q &n&+Mcreates a strange sound...&N", TRUE, ch, obj, victim, TO_NOTVICT);
    act("&nYour&N $q &n&+Mcreates a strange sound...&N", TRUE, ch, obj, victim, TO_CHAR);
    act("&n$n's&N $q &n&+Mcreates a strange sound... &N", TRUE, ch, obj, victim, TO_VICT);
    spell_inflict_pain(40, ch, 0, 0, victim, obj);
  }
  else
  {
    act("&n$n's&N $q &n&+Mcreates a &+YCRACKED&+M sound...&N", TRUE, ch, obj, victim, TO_NOTVICT);
    act("&nYour&N $q &n&+Mcreates a &+YCRACKED&+M sound...&N", TRUE, ch, obj, victim, TO_CHAR);
    act("&n$n's&N $q &n&+Mcreates a &+YCRACKED&+M sound... &N", TRUE, ch, obj, victim, TO_VICT);
    spell_ego_blast(30, ch, 0, 0, ch, obj);
  }
  return TRUE;
}

int fumblegaunts(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      cur_time;
  P_char   vict = NULL;
  struct affected_type af;
  int      rand;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_WORN_POS(obj, WEAR_HANDS) || !IS_ALIVE(ch) || ch != obj->loc.wearing || cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  if( GET_CLASS(ch, CLASS_MONK) )
  {
    return FALSE;
  }

  // 1/15 chance.
  if( IS_FIGHTING(ch) && !number(0, 14) )
  {
    vict = ch->specials.fighting;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }
    rand = number(1, 10);
    if (rand < 8)
    {
      act("&n$n's&N $q &+Rflares red!&N", TRUE, ch, obj, vict, TO_NOTVICT);
      act("&nYour&N $q &+Rflares red!&N", TRUE, ch, obj, vict, TO_CHAR);
      act("&n$n's&N $q &+Rflares red!&N", TRUE, ch, obj, vict, TO_VICT);
#ifndef REALTIME_COMBAT
      act("&+LYour $q blurs as it strikes&N $N.", FALSE, ch, obj, vict, TO_CHAR);
      act("&+L$n's $q blurs as it strikes you.", FALSE, ch, obj, vict, TO_VICT);
      act("&+L$n's $q blurs as it strikes&N $N.", FALSE, ch, obj, vict, TO_NOTVICT);
#endif
#ifndef NEW_COMBAT
      if (ch->specials.fighting)
        hit(ch, ch->specials.fighting, ch->equipment[PRIMARY_WEAPON]);
      if (ch->specials.fighting)
        hit(ch, ch->specials.fighting, ch->equipment[PRIMARY_WEAPON]);
      if (ch->specials.fighting)
        hit(ch, ch->specials.fighting, ch->equipment[PRIMARY_WEAPON]);
      if (ch->specials.fighting)
        hit(ch, ch->specials.fighting, ch->equipment[PRIMARY_WEAPON]);
      if (ch->specials.fighting)
        hit(ch, ch->specials.fighting, ch->equipment[PRIMARY_WEAPON]);
#endif
    }
    else
    {
      act("&n$n's&N $q &+Yflares yellow!&N", TRUE, ch, obj, vict, TO_NOTVICT);
      act("&nYour&N $q &+Yflares yellow!&N", TRUE, ch, obj, vict, TO_CHAR);
      act("&n$n's&N $q &+Yflares yellow!&N", TRUE, ch, obj, vict, TO_VICT);

      if( ch->equipment[WIELD] && (GET_LEVEL(ch) > 1)
        && !IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_NODROP)
        && (ch->equipment[WIELD]->type == ITEM_WEAPON) )
      {
        send_to_char("&=LYYou swing at your foe _really_ badly, sending your weapon flying!\n", ch);
        act("$n stumbles with $s attack, sending $s weapon flying!", TRUE, ch, 0, 0, TO_ROOM);
        P_obj weap = unequip_char(ch, WIELD);
        if( weap )
        {
          obj_to_room(weap, ch->in_room);
        }
        char_light(ch);
        room_light(ch->in_room, REAL);
      }
      else
        send_to_char("You stumble, but recover in time!\n", ch);
    }

  }
  return FALSE;
}

int brainripper(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;
  int      rand;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }

  rand = number(1, 10);
  // 70% chance.
  if( rand < 8 )
  {
    act("&+W$n's&N $q &+Wglows!&N", TRUE, ch, obj, victim, TO_NOTVICT);
    act("&+WYour&N $q &+Wglows!!&N", TRUE, ch, obj, victim, TO_CHAR);
    act("&+W$n's&N $q &+Wglows!!&N", TRUE, ch, obj, victim, TO_VICT);
    spell_inflict_pain(40, ch, 0, 0, victim, obj);
    spell_feeblemind(60, ch, NULL, 0, victim, 0);
  }
  else
  {
    act("&+W$n's&N $q &+Lglows!&N", TRUE, ch, obj, victim, TO_NOTVICT);
    act("&+WYour&N $q &+Lglows!!&N", TRUE, ch, obj, victim, TO_CHAR);
    act("&+W$n's&N $q &+Lglows!!&N", TRUE, ch, obj, victim, TO_VICT);
    berserk(ch, 6 * PULSE_VIOLENCE);
  }
  return (TRUE);
}

int stormbringer(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;
  int      rand;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  victim = (P_char) arg;
  // 1/30 chance
  if( !dam || !IS_ALIVE(ch) || !IS_ALIVE(victim) || !OBJ_WORN_BY(obj, ch) || number(0, 29) )
  {
    return FALSE;
  }

  act("&+W$n's&N $q &+Bsummons up a storm!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+WYour&N $q &+Bsummons up a storm!!&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+W$n's&N $q &+Wsummons up a storm!!&N", TRUE, ch, obj, victim, TO_VICT);
  /*act("&+cA &+Wforce&+c of &+Cwinds&+c throws you backwards.&n", TRUE, ch,
      obj, victim, TO_VICT);
  act("&+cA &+Wforce&+c of &+Cwinds&+c throws $N backwards.&n", TRUE, ch, obj,
      victim, TO_CHAR);
  act("&+cA &+Wforce&+c of &+Cwinds&+c throws $n backwards.&n", TRUE, victim,
      obj, ch, TO_NOTVICT);
  Stun(victim, ch, (dice(1, 2) * PULSE_VIOLENCE), FALSE);*/

  spell_cyclone(30, ch, 0, SPELL_TYPE_SPELL, victim, 0);
  if( !IS_ALIVE(ch) || !IS_ALIVE(victim) )
  {
    return TRUE;
  }

  switch( world[ch->in_room].sector_type )
  {
    case SECT_AIR_PLANE:
      act("&+cThe energies of the &+CAir Plane&+c fill $q &+cwith power!&n", TRUE, ch, obj, victim, TO_VICT);
      act("&+cThe energies of the &+CAir Plane&+c fill $q &+cwith power!&n", TRUE, ch, obj, victim, TO_CHAR);
      act("&+cThe energies of the &+CAir Plane&+c fill $q &+cwith power!&n", TRUE, victim, obj, ch, TO_NOTVICT);
      spell_chain_lightning(50, ch, 0, SPELL_TYPE_SPELL, victim, 0);
      break;
    case SECT_FIELD:
    case SECT_HILLS:
    case SECT_FOREST:
      act("&+LA powerful thundercloud appears on the horizon.&n", TRUE, ch, obj, victim, TO_VICT);
      act("&+LA powerful thundercloud appears on the horizon.&n", TRUE, ch, obj, victim, TO_CHAR);
      act("&+LA powerful thundercloud appears on the horizon.&n", TRUE, victim, obj, ch, TO_NOTVICT);
      cast_call_lightning(50, ch, 0, SPELL_TYPE_SPELL, victim, 0);
      break;
    case SECT_DESERT:
      act("&+rA great &+Rhot &+rsandstorm blows in on the horizon...&n", TRUE, ch, obj, victim, TO_VICT);
      act("&+rA great &+Rhot &+rsandstorm blows in on the horizon...&n", TRUE, ch, obj, victim, TO_CHAR);
      act("&+rA great &+Rhot &+rsandstorm blows in on the horizon...&n", TRUE, victim, obj, ch, TO_NOTVICT);
      spell_firestorm(50, ch, 0, SPELL_TYPE_SPELL, victim, 0);
      break;
  }
  return TRUE;
}

int hammer_titans(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;
  int      rand;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }

  act("&+W$n's&N $q &+Bsurges with electricity!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+WYour&N $q &+Bsurges with electricity!&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+W$n's&N $q &+Bsurges with electricity!&N", TRUE, ch, obj, victim, TO_VICT);
  spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
  if( char_in_list(victim) )
  {
    spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
  }
  return TRUE;
}

/* Guild badges Kvark 2002-02-04
 * This item is a bonus for those with artifact's and those that have frags, item changes name tho soo
 *   it's also fun for lowbies and others. This item might need tweaking in long wipes since the frag
 *   amount might be pretty high and frags above 100 let ya get pretty nice stat's.
 */
int guild_badge(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      cur_time;
  P_obj    tempobj;
  struct affected_type af;
  int      affects_bonus = 0;
  int      i, count, weekday;
  char     buf1[MAX_STRING_LENGTH];
  P_obj    item, next_item;
  int      curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC || !IS_ALIVE(ch) || IS_NPC(ch) || !OBJ_WORN_BY(obj, ch) )
  {
     return FALSE;
  }

  // Let's only call this function once every 12 min.
  curr_time = time(NULL);
  if( obj->timer[0] + 12*SECS_PER_REAL_MIN <= curr_time )
  {
    // Things breaks sometimes...
    //   be scared all time - on regular proc this object have chance to break.
    if( !number(0, 90) || obj->timer[0] + 10*SECS_PER_REAL_DAY <= curr_time && !number(0, 7) )
    {
      act("&+L$p &+Lhums with a &+GCRA&+YC&+GKING &+Lsound.&n", FALSE, ch, obj, 0, TO_CHAR);
      act("&+L$p &+Lcrumble to dust.&n", FALSE, ch, obj, 0, TO_CHAR);
      extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      return TRUE;
    }

    // Didnt break it? ok.... lets give them some fun then.
    obj->timer[0] = curr_time;

    // Things that modifes the badge.
    // If you have alot of frags, that's good for you!
    if( ch->only.pc->frags > 0 )
    {
      affects_bonus = (int) (ch->only.pc->frags / 12);

      for( i = count = 0; i < MAX_WEAR; i++ )
      {
        if( ch->equipment[i] )
        {
          if( IS_ARTIFACT(ch->equipment[i]) )
          {
            count++;
          }
        }
      }

      // One arti is worth some also!
      affects_bonus = affects_bonus + (60 * count);

      // Level ! ofcourse lvl changes is!
      affects_bonus = BOUNDED(0, affects_bonus, 1000);
      affects_bonus = (affects_bonus * GET_LEVEL(ch) / 50);

      // Goodies have it a bit easier get good stat's
      if( RACE_GOOD(ch) )
      {
        affects_bonus = (int) (affects_bonus * 1.3);
      }
      // Add some little randomness
      affects_bonus = affects_bonus + number(0,10);

      // Ok lets switch affects depenings on weekday.
      weekday = ((35 * time_info.month) + time_info.day + 1) % 7;
      switch( weekday )
      {
      case 0:
        obj->affected[0].location = APPLY_HIT;
        obj->affected[0].modifier = (int) (affects_bonus / 25);
        if( obj->affected[0].modifier < 0 )
        {
          obj->affected[0].modifier = 0;
        }
        obj->affected[1].location = APPLY_STR_MAX;
        obj->affected[1].modifier = (int) (affects_bonus / 200);
        if (obj->affected[1].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        break;
      case 1:
        obj->affected[0].location = APPLY_HIT;
        obj->affected[0].modifier = (int) (affects_bonus / 25);
        if (obj->affected[0].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        obj->affected[1].location = APPLY_DAMROLL;
        obj->affected[1].modifier = (int) (affects_bonus / 200);
        if (obj->affected[1].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        break;
      case 2:
        obj->affected[0].location = APPLY_AC;
        obj->affected[0].modifier = -((int) (affects_bonus / 100));
        if (obj->affected[0].modifier > 0)
        {
          obj->affected[1].modifier = 0;
        }
        obj->affected[1].location = APPLY_CON_MAX;
        obj->affected[1].modifier = (int) (affects_bonus / 200);
        if (obj->affected[1].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        break;
      case 3:
        obj->affected[0].location = APPLY_AC;
        obj->affected[0].modifier = (int) (affects_bonus / 25);
        if (obj->affected[0].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        obj->affected[1].location = APPLY_SAVING_SPELL;
        obj->affected[1].modifier = 1 - ((int) (affects_bonus / 200));
        if (obj->affected[1].modifier > 0)
        {
          obj->affected[1].modifier = 0;
        }
        break;
      case 4:
        obj->affected[0].location = APPLY_AC;
        obj->affected[0].modifier = (int) (affects_bonus / 25);
        if (obj->affected[0].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        obj->affected[1].location = APPLY_DEX;
        obj->affected[1].modifier = (int) (affects_bonus / 200);
        if (obj->affected[1].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        break;
      case 5:
        obj->affected[0].location = APPLY_HIT;
        obj->affected[0].modifier = (int) (affects_bonus / 25);
        if (obj->affected[0].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        obj->affected[1].location = APPLY_SAVING_PARA;
        obj->affected[1].modifier = 1 - ((int) (affects_bonus / 200));
        if (obj->affected[1].modifier > 0)
        {
          obj->affected[1].modifier = 0;
        }
        break;
      case 6:
        obj->affected[0].location = APPLY_HIT;
        obj->affected[0].modifier = (int) (affects_bonus / 25);
        if (obj->affected[0].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        obj->affected[1].location = APPLY_CON;
        obj->affected[1].modifier = (int) (affects_bonus / 200);
        if (obj->affected[1].modifier < 0)
        {
          obj->affected[1].modifier = 0;
        }
        break;
      default:
        break;
      }

      balance_affects(ch);
      // ok calculations are made, lets HUMMMM
      act("&+L$p &+Lhums briefly.&n", FALSE, ch, obj, 0, TO_CHAR);
      if( GET_TITLE(ch) )
      {
        sprintf(buf1, "&+LThe symbol of %s&N", GET_TITLE(ch));
        if( (obj->str_mask & STRUNG_DESC2) && obj->short_description )
        {
           FREE(obj->short_description);
        }
        obj->short_description = NULL;
        obj->str_mask |= STRUNG_DESC2;
        obj->short_description = str_dup(buf1);
      }
      else
      {
        sprintf(buf1, "&+LThe symbol of %s&N", GET_NAME(ch));
        if( (obj->str_mask & STRUNG_DESC2) && obj->short_description )
        {
           FREE(obj->short_description);
        }
        obj->short_description = NULL;
        obj->str_mask |= STRUNG_DESC2;
        obj->short_description = str_dup(buf1);
      }
      return TRUE;
    }
  }
  return FALSE;
}

int fun_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    dam = cmd / 1000;
  P_char victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }

  if( !(victim = (P_char) arg) || number(0, 1) )
  {
    return FALSE;
  }

  act("&+w$n's&N $q &N strikes out in a &+rfre&+Rnzy&N of &+cs&+Cp&+We&+Ce&+cd&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+wYour&N $q &N strikes out in a &+rfre&+Rnzy&N of &+cs&+Cp&+We&+Ce&+cd&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+w$n's&N $q &N strikes out in a &+rfre&+Rnzy&N of &+cs&+Cp&+We&+Ce&+cd&N", TRUE, ch, obj, victim, TO_VICT);
  spell_magic_missile(30, ch, NULL, 0, victim, 0);
  if( IS_ALIVE(ch) && IS_ALIVE(victim) )
  {
    spell_ice_missile(30, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }
  return TRUE;
}

int mankiller(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
  {
    return FALSE;
  }

  victim = (P_char) arg;
  // 1/15 chance.
  if( !IS_ALIVE(victim) || GET_SEX(victim) != SEX_MALE || number(0, 14) )
  {
    return FALSE;
  }
/* Dunno why we skip this, but ok...
  act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone you filthy male!&+W'&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+wYour $q &Nscreams out with a female voice &+W'&+MBegone you filthy male!&+W'&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone you filthy male!&+W'&N", TRUE, ch, obj, victim, TO_VICT);
*/
  switch( number(0, 2) )
  {
  case 0:
    act("&+w$n's&N $q &Nlets out a fearsome &+MSCREAM&n, directed at $N!&N", TRUE, ch, obj, victim, TO_NOTVICT);
    act("You grin slightly, as $q&N &+wspirit's &+MSCREAM&n causes $N to writhe in agony!&N", TRUE, ch, obj, victim, TO_CHAR);
    act("&+w$n's&N $q pierces your mind with a &+wsupernatural&n &+MSCREAM&n, stunning you!&N", TRUE, ch, obj, victim, TO_VICT);
    Stun(victim, ch, (int) (PULSE_VIOLENCE * 0.5), FALSE);
    return TRUE;
    break;
  case 1:
    act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone, you filthy male!&+W'&N", TRUE, ch, obj, victim, TO_NOTVICT);
    act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone, you filthy male!&+W'&N", TRUE, ch, obj, victim, TO_CHAR);
    act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone, you filthy male!&+W'&N", TRUE, ch, obj, victim, TO_VICT);
    spell_nightmare(60, ch, NULL, 0, victim, 0);
    return TRUE;
    break;
  case 2:
    act("&+w$n's $q&n emits an ear piercing &+RSHRIEK&n!&N", TRUE, ch, obj, victim, TO_NOTVICT);
    act("$q&n emits an ear piercing &+RSHRIEK&n! You instinctively cover your ears.&N", TRUE, ch, obj, victim, TO_CHAR);
    act("&+RARGH!&n As $n's $q&n emits a piercing &+RSHRIEK&n, you are filled with excruciating pain!&N", TRUE, ch, obj, victim, TO_VICT);
    spell_shatter(56, ch, NULL, 0, victim, 0);
    return TRUE;
    break;
  }
  return FALSE;
}

int dragonslayer(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !OBJ_WORN(obj) && !OBJ_CARRIED(obj) )
  {
    return FALSE;
  }

  if( OBJ_WORN(obj) )
  {
    ch = obj->loc.wearing;
  }
  else if (OBJ_CARRIED(obj))
  {
    ch = obj->loc.carrying;
  }
  else
  {
    return FALSE;
  }
  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( GET_LEVEL(ch) > 36 && !IS_TRUSTED(ch) )
  {
    act("&+GYour $q zaaaps your experienced hands&n!", TRUE, ch, obj, 0, TO_CHAR);
    act("&+G$n's $q zaaaps $s experienced hands&n!", TRUE, ch, obj, 0, TO_NOTVICT);
    act("&+GYour $q drops to the ground with a soft thud.", TRUE, ch, obj, 0, TO_CHAR);
    act("&+G$n's $q drops to the ground with a soft thud.", TRUE, ch, obj, 0, TO_NOTVICT);
    obj_from_char(obj);
    obj_to_room(obj, ch->in_room);
    return FALSE;
  }

  if( !dam || !OBJ_WORN(obj) )
  {
    return FALSE;
  }
  victim = (P_char) arg;
  // 1/20 chance.
  if( !IS_ALIVE(victim) || !IS_DRAGON(victim) || number(0, 19) )
  {
    return FALSE;
  }

  act("&+GYour $q &+Wglows &+Rbright &+rred &+Gat the sight of a &+gdr&+Gag&+gon&N.", TRUE, ch, obj, victim, TO_CHAR);
  act("&+G$n's $q &+Wglows &+Rbright &+rred &+Gat the sight of a &+gdr&+Gag&+gon&N.&N", TRUE, ch, obj, victim, TO_NOTVICT);
  spell_burning_hands(40, ch, NULL, 0, victim, obj);
  spell_stone_skin(26, ch, 0, 0, ch, 0);
  spell_haste(26, ch, 0, 0, ch, 0);
  return TRUE;
}

void event_lifereaver(P_char ch, P_char victim, P_obj obj, void *data)
{
  int dam;
  int count = *((int*)data);

  dam = dice(3, 6);

  if ((GET_HIT(ch) - dam) > 0)
  {
    GET_HIT(ch) -= dam;
    send_to_char("&+RYour wound bleeds openly!&N\n", ch);
    act("$n&+R bleeds all over the place.&N", TRUE, ch, NULL, NULL,
        TO_NOTVICT);
    make_bloodstain(ch);
  }

  if (count >= 0)
  {
    count--;
    add_event(event_lifereaver, PULSE_VIOLENCE, ch, 0, 0, 0, &count, sizeof(count));
  }
  else
  {
    send_to_char("&+rThe bleeding appears to have stopped.&N\n", ch);
  }
}

int lifereaver(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  char     buf[MAX_STRING_LENGTH];
  int      numb;
  
  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if (!OBJ_WORN(obj))
  {
    return FALSE;
  }
  if (OBJ_WORN(obj))
    ch = obj->loc.wearing;
  else if (OBJ_CARRIED(obj))
    ch = obj->loc.carrying;
  else
    return FALSE;


  if (ch == NULL)
    return FALSE;

  if (cmd)
  {
    return FALSE;
  }
  else if(IS_FIGHTING(ch))
  {
    if(!number(0, 3) &&
       !IS_UNDEADRACE(ch->specials.fighting))
    {
      vict = ch->specials.fighting;
      act("&+LYour $q &+Lslices into $N&+L, making him &+rBLEED&+L!&N", TRUE,
          ch, obj, vict, TO_CHAR);
      act("&+L$ns $q &+Lslices into $N&+L, making him &+rBLEED&+L!&N", TRUE,
          ch, obj, vict, TO_NOTVICT);
      act("&+L&ns $q &+Lslices into you, making you &+rBLEED&+L!&N", TRUE, ch,
          obj, vict, TO_VICT);
      numb = number(4, 7);
      add_event(event_lifereaver, PULSE_VIOLENCE, vict, 0, 0, 0, &numb, sizeof(numb));
      return TRUE;
    }

  }
  return FALSE;
}

int rax_red_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    dam = cmd / 1000;
  P_char victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) || !(victim = (P_char) arg) )
  {
    return FALSE;
  }
  // 1/2 chance.
  if( !IS_ALIVE(victim) || number(0, 1) )
  {
    return FALSE;
  }

  act("&+L$n's&N $q &+r st&+Rrik&+res &+Lout in a &+RF&+rUR&+RY &+Lof &+rde&+Rst&+rruc&+Rt&+rio&+Rn&+L!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+LYour&N $q &+r st&+Rrik&+res &+Lout in a &+RF&+rUR&+RY &+Lof &+rde&+Rst&+rruc&+Rt&+rio&+Rn&+L!&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+L$n's&N $q &+r st&+Rrik&+res &+Lout in a &+RF&+rUR&+RY &+Lof &+rde&+Rst&+rruc&+Rt&+rio&+Rn&+L!&N", TRUE, ch, obj, victim, TO_VICT);
  spell_bigbys_crushing_hand(60, ch, NULL, SPELL_TYPE_SPELL, victim, 0);

  return TRUE;
}

int obj_imprison(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      victim_in_room;
  P_char   tch;

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  victim_in_room = FALSE;
  if( OBJ_ROOM(obj) )
  {
    for (tch = world[obj->loc.room].people; tch; tch = tch->next_in_room)
    {
      if( IS_PC(tch) && GET_PID(tch) == obj->value[0] )
      {
        victim_in_room = TRUE;
        break;
      }
    }
  }

  // If it's been picked up or victim escaped the room somehow, then purge object and remove bit.
  if( !victim_in_room )
  {
    for( tch = character_list; tch; tch = tch->next )
    {
      if( IS_PC(tch) && GET_PID(tch) == obj->value[0] )
      {
        if( IS_AFFECTED5( tch, AFF5_IMPRISON ) )
        {
          REMOVE_BIT( tch->specials.affected_by5, AFF5_IMPRISON );
          break;
        }
      }
    }
    //obj_from_room(obj);
    extract_obj(obj, TRUE); // Not an arti, but 'in game.'
    return FALSE;
  }

  // If ch isn't the right ch.
  if( !IS_ALIVE(ch) || IS_TRUSTED(ch) || IS_NPC(ch) || obj->value[0] != GET_PID(ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_LOOK || cmd == CMD_SAY || cmd == CMD_PETITION )
  {
    return FALSE;
  }

  obj->value[1] -= GET_LEVEL(ch) + 66;

  // Struggled enough or 5% 'lucky' chance.
  if( obj->value[1] > 0 && number(1, 100) <= 95 )
  {
    act("But you are totally encased by the $q!", FALSE, ch, obj, NULL, TO_CHAR);
    act("$N struggles to break out of the $q but fails.", TRUE, ch, obj, ch, TO_NOTVICT);
    CharWait(ch, PULSE_VIOLENCE / 2);
    return TRUE;
  }

  act("Aha! The $q was nothing but an illusion, you are free!", FALSE, ch, obj, ch, TO_CHAR);
  act("An indescribable relief emanates from $N as $E realises the $q was just an illusion!", TRUE, NULL, obj, ch, TO_NOTVICT);
  REMOVE_BIT( ch->specials.affected_by5, AFF5_IMPRISON );
  extract_obj(obj, TRUE); // Not an arti, but 'in game.'

  return FALSE;
}

/* this proc is the staff of blue flames proc modified, Raxxel did it, and it probably doesn't work! */
int totem_of_mastery(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  char     e_pos;
  int      working, curr_time;

  vict = (P_char) arg;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( obj && !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  // If it must be wielded, use this
  e_pos = ((obj->loc.wearing->equipment[HOLD] == obj) ? HOLD :
           (obj->loc.wearing->equipment[WIELD] == obj) ? WIELD :
           (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ? SECONDARY_WEAPON :
           (obj->loc.wearing->equipment[THIRD_WEAPON] == obj) ? THIRD_WEAPON :
           (obj->loc.wearing->equipment[FOURTH_WEAPON] == obj) ? FOURTH_WEAPON : 0);

  if( !e_pos )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "spirit") )
    {
      act("You whisper 'spirit' to your $q", FALSE, ch, 0, 0, TO_CHAR);
      act("&+mA spirit fog streams out of your $q &+mand surrounds you.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n whispers 'spirit' to $q", TRUE, ch, obj, NULL, TO_ROOM);
      act("&+mA spirit fog streams out of &+M$n&+m's $q&+m.&N", TRUE, ch, obj, NULL, TO_ROOM);

      if( ch->group )
      {
        cast_as_area(ch, SPELL_SPIRIT_ARMOR, 60, 0);
      }
      else
      {
        spell_spirit_armor(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      return TRUE;
    }
    else if( isname(arg, "hawk") )
    {
      act("You whisper 'hawk' to your $q", FALSE, ch, 0, vict, TO_CHAR);
      act("&+wA vaporous &+Whawk&+w appears briefly from your&N $q.&N", FALSE,
          ch, obj, obj, TO_CHAR);
      act("$n whispers 'hawk' to $q.", FALSE, ch, obj, obj, TO_ROOM);
      act("&+wA vaporous &+Whawk&+w appears briefly from &+W$n&+w's&N $q.",
          TRUE, ch, obj, vict, TO_ROOM);
      if( ch->group )
      {
        cast_as_area(ch, SPELL_HAWKVISION, 60, 0);
      }
      else
      {
        spell_hawkvision(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      return TRUE;
    }
    else if( isname(arg, "panther") )
    {

      act("You whisper 'panther' to your $q", FALSE, ch, 0, vict, TO_CHAR);
      act("&+LThe spirit of a jet black &+wpanther&+L flows from your &N$q&+w.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n whispers 'panther' to $q.", FALSE, ch, obj, obj, TO_ROOM);
      act("&+LThe spirit of a jet black &+wpanther&+L flows from &+W$n&+w's&N $q&+w.", TRUE, ch, obj, vict, TO_ROOM);
      if( ch->group )
      {
        cast_as_area(ch, SPELL_PANTHERSPEED, 60, 0);
      }
      else
      {
        spell_pantherspeed(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      return TRUE;
    }
    else if( isname(arg, "sense") )
    {

      act("You whisper 'sense' to your $q", FALSE, ch, 0, vict, TO_CHAR);
      act("&+cA rush of air from your &N $q&+w.&N&+c engulfs you.", FALSE, ch, obj, obj, TO_CHAR);
      act("$n whispers 'sense' to $q.", FALSE, ch, obj, obj, TO_ROOM);
      act("&+cA rush of air from &+W$n&+w's&N $q&+c.", TRUE, ch, obj, vict, TO_ROOM);
      if( ch->group )
      {
        cast_as_area(ch, SPELL_SENSE_SPIRIT, 60, 0);
      }
      else
      {
        spell_sense_spirit(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      return TRUE;
    }
    else if( isname(arg, "raven") )
    {

      act("You whisper 'raven' to your $q", FALSE, ch, 0, vict, TO_CHAR);
      act("&+ySmoke billows from your&N $q &+yin the form of a &+WRaven&+y.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n whispers 'raven' to $p.", FALSE, ch, obj, obj, TO_ROOM);
      act("&+ySmoke billows from &+Y$n&+y's&N $q &+yin the form of &+WRaven&+y.", TRUE, ch, obj, vict, TO_ROOM);
      if( ch->group )
      {
        cast_as_area(ch, SPELL_RAVENFLIGHT, 60, 0);
      }
      else
      {
        spell_ravenflight(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      return TRUE;
    }
    else if( isname(arg, "ward") )
    {
      act("You whisper 'ward' to your $q", FALSE, ch, 0, vict, TO_CHAR);
      act("&+wA &+Wspirit &+wappears from your &N$q &+wand engulfs you.&N",
          FALSE, ch, obj, obj, TO_CHAR);
      act("$n whispers 'ward' to $q.", FALSE, ch, obj, obj, TO_ROOM);
      act("&+wA &+Wspirit&+w appears from &+W$n&+w's&N $q&+w.&N", TRUE, ch,
          obj, vict, TO_ROOM);
      if (ch->group)
      {
        cast_as_area(ch, SPELL_GREATER_SPIRIT_WARD, 60, 0);
      }
      else
      {
        spell_greater_spirit_ward(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      return TRUE;
    }
    else if( isname(arg, "lion") )
    {
      act("&+yA ghostly &+YLion &+RROARS&+y from your $q.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n whispers 'lion' to $q.", FALSE, ch, obj, obj, TO_ROOM);
      act("&+yA ghostly &+YLion &+RROARS&+y from &+Y$n&+y's&N $q&+y.&N", TRUE, ch, obj, vict, TO_ROOM);
      if( ch->group )
      {
        cast_as_area(ch, SPELL_LIONRAGE, 60, 0);
      }
      else
      {
        spell_lionrage(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      return TRUE;
    }
    else if( isname(arg, "elephant") )
    {
      act("You whisper 'elephant' to your $q", FALSE, ch, 0, vict, TO_CHAR);
      act("&+LThe sounds of &+welephants &+Ltrumpeting erupt from your&N $q&+L.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n whispers 'elephant' to $q.", FALSE, ch, obj, obj, TO_ROOM);
      act("&+LThe sounds of &+Welephants &+Ltrumpeting erupt from &+w$n&+L's&N $q&+L.", TRUE, ch, obj, vict, TO_ROOM);
      if( ch->group )
      {
        cast_as_area(ch, SPELL_ELEPHANTSTRENGTH, 60, 0);
      }
      else
      {
        spell_elephantstrength(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      }
      return TRUE;
    }
/* EVERYTHING HERE IS JUST FUNNY SILLY STUFF */
    else if (isname(arg, "mystra"))
    {
      act("You whisper 'mystra' to your $q", FALSE, ch, 0, vict, TO_CHAR);
      act("&+LThe sky darkens and fills with clouds.  You hear a deep rumbling&N", FALSE, ch, obj, obj, TO_CHAR);
      act("&+Lin the distance.  The rumbling gets louder and louder.  God himself&N", FALSE, ch, obj, obj, TO_CHAR);
      act("&+Lappears infront of you. &+WHOW DARE YOU SPEAK THAT VILE NAME?!&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n whispers 'mystra' to $q.", FALSE, ch, obj, obj, TO_ROOM);
      act("&+LThe sky darkens and fills with clouds.  You hear a deep rumbling&N", TRUE, ch, obj, vict, TO_ROOM);
      act("&+Lin the distance.  The rumbling gets louder and louder.  God himself&N", TRUE, ch, obj, vict, TO_ROOM);
      act("&+Lappears infront of you. &+WHOW DARE YOU SPEAK THAT VILE NAME?!&N", TRUE, ch, obj, vict, TO_ROOM);
      spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    }
  }

  if( cmd != CMD_MELEE_HIT )
  {
    return FALSE;
  }

  // 1/16 chance
  if( vict && !number(0, 15))
  {
    act("&+BYour $q flashes with lightning as a bolt fires out at $N.", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
    act("$n's $q &+Bradiates a bolt of lightning out at you.", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
    act("$n's $q &+Bradiates a blot of lightning at $N.", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
    spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    // Stop attack if one dies.
    if( !IS_ALIVE(ch) || !IS_ALIVE(vict) )
    {
      return TRUE;
    }
  }
  return FALSE;
}

int circlet_of_light(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char vict;
  char   e_pos;
  int    in_battle, working, curr_time;

  vict = (P_char) arg;

  in_battle = cmd / 1000;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !ch && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) || !OBJ_WORN_POS(obj, WEAR_HEAD) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "beblessed") )
    {
      act("&+wYou raise your hands in the air...&N\n", FALSE, ch, 0, 0, TO_CHAR);
      act("&+wYou send out a st&+Wre&+Cam &+wof &+Chealing &+Wenergies &+wto aid all those who are injured.&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n&+w raises $s hands in the air...\n", TRUE, ch, obj, NULL, TO_ROOM);
      act("&+w$n sends out a st&+Wre&+Cam &+wof &+Chealing &+Wenergies &+wto aid all those who are injured.&N", TRUE, ch, obj, NULL, TO_ROOM);

      spell_mass_heal(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    }
    else if( isname(arg, "mystra") )
    {
      act("You whisper 'mystra' to your $q", FALSE, ch, 0, vict, TO_CHAR);
      act("&+LThe sky darkens and fills with clouds.  You hear a deep rumbling&N", FALSE, ch, obj, obj, TO_CHAR);
      act("&+Lin the distance.  The rumbling gets louder and louder.  God himself&N", FALSE, ch, obj, obj, TO_CHAR);
      act("&+Lappears infront of you. &+WHOW DARE YOU SPEAK THAT VILE NAME?!&N", FALSE, ch, obj, obj, TO_CHAR);
      act("$n whispers 'mystra' to $q.", FALSE, ch, obj, obj, TO_ROOM);
      act("&+LThe sky darkens and fills with clouds.  You hear a deep rumbling&N", TRUE, ch, obj, vict, TO_ROOM);
      act("&+Lin the distance.  The rumbling gets louder and louder.  God himself&N", TRUE, ch, obj, vict, TO_ROOM);
      act("&+Lappears infront of you. &+WHOW DARE YOU SPEAK THAT VILE NAME?!&N", TRUE, ch, obj, vict, TO_ROOM);
      spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
      return TRUE;
    }
  }

  // Past here, and you're fighting.. 1/15 chance.
  if( !in_battle || !number(0, 14) )
  {
    return FALSE;
  }

  act("&+BYour $q flashes with lightning as a bolt fires out at $N.", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
  act("$n's $q &+Bradiates a bolt of lightning out at you.", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
  act("$n's $q &+Bradiates a blot of lightning at $N.", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
  spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, vict, 0);
  if( !IS_ALIVE(ch) || !IS_ALIVE(vict) )
  {
    return TRUE;
  }
  return FALSE;
}



/* long john silver's sword */
int ljs_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    dam = cmd / 1000;
  P_char victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch)
    || !(victim = (P_char) arg) )
  {
    return FALSE;
  }

  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }
  act("&+W$n's&N $q &+Lfl&+rar&+Res &+Lup with darkness...&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+WYour&N $q &+Lfl&+rar&+Res &+Lup with darkness...&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+W$n's&N $q &+Lfl&+rar&+Res &+Lup with darkness...&N", TRUE, ch, obj, victim, TO_VICT);
  /* This is just a temporary spell, the spell will probably be made something else later, but just getting it to work for now! */
  spell_greater_spirit_anguish(25, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  return (TRUE);
}

/* WUSS SWORD hehe */

int wuss_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    dam = cmd / 1000;
  P_char victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // 1/30 chance.
  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch)
    || !(victim = (P_char) arg) || number(0, 29) )
  {
    return FALSE;
  }
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  act("&+W$n's&N $q &+Wglows &+Ybrightly&+w...&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+WYour&N $q &+Wglows &+Ybrightly&+w...&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+W$n's&N $q &+Wglows &+Ybrightly&+w...&N", TRUE, ch, obj, victim, TO_VICT);
  spell_magic_missile(1, ch, NULL, 0, victim, obj);
  return TRUE;
}

int head_guard_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    dam = cmd / 1000;
  P_char victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch)
    || !(victim = (P_char) arg) || number(0, 29) )
  {
    return FALSE;
  }
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  act("&+W$n's&N $q &+rB&+RU&+YRS&+RT&+rS &+Linto &+rfl&+Ram&+Yes&+L...&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+WYour&N $q &+rB&+RU&+YRS&+RT&+rS &+Linto &+rfl&+Ram&+Yes&+L...&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+W$n's&N $q &+rB&+RU&+YRS&+RT&+rS &+Linto &+rfl&+Ram&+Yes&+L...&N", TRUE, ch, obj, victim, TO_VICT);
  spell_magma_burst(30, ch, NULL, 0, victim, obj);
  return TRUE;
}

int alch_rod(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      rand;
  int      dam = cmd / 1000;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) || !(victim = (P_char) arg) )
  {
    return FALSE;
  }
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }

  act("&+W$n's&N $q &+Lcalls upon &+cmy&+Cst&+Wi&+Cca&+cl &+rpo&+Rwe&+rrs&+L...&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+WYour&N $q &+Lcalls upon &+cmy&+Cst&+Wi&+Cca&+cl &+rpo&+Rwe&+rrs&+L...&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+W$n's&N $q &+Lcalls upon &+cmy&+Cst&+Wi&+Cca&+cl &+rpo&+Rwe&+rrs&+L...&N", TRUE, ch, obj, victim, TO_VICT);
  rand = number(0, 1);
  switch( rand )
  {
  case 0:
    spell_living_stone(30, ch, 0, 0, victim, obj);
    break;
  case 1:
    spell_shadow_monster(30, ch, NULL, 0, victim, obj);
    break;
  }
  return TRUE;
}


int dragon_skull_helm(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      rand;
  int      curr_time;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (ch != obj->loc.wearing)
    || !OBJ_WORN_POS(obj, WEAR_HEAD) )
  {
    return FALSE;
  }

  curr_time = time(NULL);
  if( !IS_SET(world[ch->in_room].room_flags, NO_MAGIC) )
  {
    // 1 min timer.
    if (obj->timer[0] + 60 <= curr_time)
    {
      obj->timer[0] = curr_time;
      rand = number(0, 7);
      switch( rand )
      {
      case 0:
        act("&+LYour $q&+L causes your eyes to &+Rg&+rlo&+Rw&+L.&n", TRUE, ch, obj, victim, TO_CHAR);
        spell_infravision(60, ch, 0, 0, ch, 0);
        break;
      case 1:
        act("&+LYour $q&+L causes your eyes to &+Wglow&+L.&n", TRUE, ch, obj, victim, TO_CHAR);
        spell_detect_invisibility(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
        break;
      case 2:
        act("&+LYour $q&+L causes your eyes to &+Yglow&+L.&n", TRUE, ch, obj, victim, TO_CHAR);
        spell_detect_good(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
        break;
      case 3:
        act("&+LYour $q&+L causes your eyes to &+wglow&+L.&n", TRUE, ch, obj, victim, TO_CHAR);
        spell_farsee(60, ch, NULL, 0, ch, 0);
        break;
      case 4:
        act("&+LYour $q&+L causes your eyes to &+Lglow&+L.&n", TRUE, ch, obj, victim, TO_CHAR);
        spell_sense_life(60, ch, 0, 0, ch, 0);
        break;
      case 5:
        act("&+LYour $q&+L causes your eyes to &+Wg&+wlo&+Ww&+L.&n", TRUE, ch, obj, victim, TO_CHAR);
        spell_hawkvision(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
        break;
      case 6:
        act("&+LYour $q&+L causes your eyes to &+rglow&+L.&n", TRUE, ch, obj, victim, TO_CHAR);
        spell_detect_evil(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
        break;
      case 7:
        act("&+LYour $q&+L causes your eyes to &+Bglow&+L.&n", TRUE, ch, obj, victim, TO_CHAR);
        spell_detect_magic(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
        break;
      }
      return FALSE;
    }
    if( IS_FIGHTING(ch) && number(0, 1) )
    {
      victim = ch->specials.fighting;
      if( !IS_ALIVE(victim) )
      {
        return FALSE;
      }
      if( IS_UNDEAD(victim) )
      {
        act("&+L$n's $q &+L causes &+W$m &+Leyes to &+Rp&+ru&+Rl&+rs&+Re &+Lwith &+Bincredible &+Wenergy&+L.", TRUE, ch, obj, victim, TO_NOTVICT);
        act("&+LYour $q &+L causes your eyes to &+Rp&+ru&+Rl&+rs&+Re &+Lwith &+Bincredible &+Wenergy&+L.", TRUE, ch, obj, victim, TO_CHAR);
        act("&+L$n's $q &+L causes &+W$n &+Leyes to &+Rp&+ru&+Rl&+rs&+Re &+Lwith &+Bincredible &+Wenergy&+L.", TRUE, ch, obj, victim, TO_VICT);
        spell_destroy_undead(60, ch, NULL, 0, victim, 0);
      }
    }
  }
  return FALSE;
}

int priest_rudder(P_obj obj, P_char ch, int cmd, char *argument)
{
  char  *arg;
  int    curr_time;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC || !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  ch = obj->loc.wearing;

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  curr_time = time(NULL);
  // 2 min timer.
  if( obj->timer[0] + 120 <= curr_time )
  {
    act("&+L$n's $q &+chums &+wsoftly&+L.&N", FALSE, ch, obj, 0, TO_ROOM);
    act("&+LYour $q &+chums &+wsoftly&+L.&N", FALSE, ch, obj, 0, TO_CHAR);
    if( !IS_SET(world[ch->in_room].room_flags, NO_MAGIC) )
    {
      spell_mass_heal(35, ch, 0, 0, ch, 0);
    }
    obj->timer[0] = curr_time;
  }

  return FALSE;
}



int ljs_armor(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  int      cur_time;

  P_char   vict;
  struct affected_type *af;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  if( !OBJ_WORN(obj) || !OBJ_WORN_POS(obj, WEAR_BODY) )
  {
    return FALSE;
  }

  ch = obj->loc.wearing;
  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }
  vict = ch->specials.fighting;
  if( !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  // 1/10 chance.
  if( IS_FIGHTING(ch) && !number(0, 9) )
  {
    if( IS_UNDEAD(vict) && !IS_UNDEAD(ch) )
    {
      act("&+L$n's&N $q &+Lgrows a &+wtough &+Llayer of skin.&N", TRUE, ch, obj, vict, TO_NOTVICT);
      act("&+LYour&N $q &+Lgrows a &+wtough &+Llayer of skin.&N", TRUE, ch, obj, vict, TO_CHAR);
      act("&+L$n's&N $q &+Lgrows a &+wtough &+Llayer of skin.&N", TRUE, ch, obj, vict, TO_VICT);
      if( !IS_SET(world[ch->in_room].room_flags, NO_MAGIC) )
      {
        spell_prot_from_undead(60, ch, 0, 0, ch, 0);
      }
      return TRUE;
    }
    if( !IS_UNDEAD(vict) )
    {
      act("&+w$n's&N $q &+Lgrows a &+Wtough &+Llayer of skin.&N", TRUE, ch, obj, vict, TO_NOTVICT);
      act("&+wYour&N $q &+Lgrows a &+Wtough &+Llayer of skin.&N", TRUE, ch, obj, vict, TO_CHAR);
      act("&+w$n's&N $q &+Lgrows a &+Wtough &+Llayer of skin.&N", TRUE, ch, obj, vict, TO_VICT);
      if( !IS_SET(world[ch->in_room].room_flags, NO_MAGIC) )
      {
        spell_protection_from_living(60, ch, 0, 0, ch, 0);
      }
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;

}

int alch_bag(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      rand;
  int      curr_time;
  P_char   victim;
  P_obj    ingred;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) || ch != obj->loc.wearing )
  {
    return FALSE;
  }

  if( !OBJ_WORN_POS(obj, WEAR_ATTACH_BELT_1)
    && !OBJ_WORN_POS(obj, WEAR_ATTACH_BELT_2)
    && !OBJ_WORN_POS(obj, WEAR_ATTACH_BELT_3) )
  {
    return FALSE;
  }

  curr_time = time(NULL);
  if( obj->timer[0] + 60 + number(0, 30) <= curr_time )
  {
/* if this somehow works, it is a MIRACLE as I had nothing to copy from */
/* should add a full set of ingredients into the pouch! */
/* obj #'s to use....... 821, 822, 823, 824, 825, 826, 827, 835 -Raxxel*/

/* begin experimental code! */
    obj->timer[0] = curr_time;

    // nightshade
    ingred = read_object( VOBJ_INGRED_NIGHTSHADE, VIRTUAL);
    if( !ingred )
    {
      return FALSE;
    }
    obj_to_obj(ingred, obj);

    // mandrake
    ingred = read_object( VOBJ_INGRED_MANDRAKE, VIRTUAL);
    if( !ingred )
    {
      return FALSE;
    }
    obj_to_obj(ingred, obj);

    // garlic
    ingred = read_object( VOBJ_INGRED_GARLIC, VIRTUAL);
    if( !ingred )
    {
      return FALSE;
    }
    obj_to_obj(ingred, obj);

    // faerie dust
    ingred = read_object( VOBJ_INGRED_FAERIE_DUST, VIRTUAL);
    if( !ingred )
    {
      return FALSE;
    }
    obj_to_obj(ingred, obj);

    // dragons blood
    ingred = read_object( VOBJ_INGRED_DRAGON_BLOOD, VIRTUAL);
    if( !ingred )
    {
      return FALSE;
    }
    obj_to_obj(ingred, obj);

    // green herb
    ingred = read_object( VOBJ_INGRED_GREEN_HERB, VIRTUAL);
    if( !ingred )
    {
      return FALSE;
    }
    obj_to_obj(ingred, obj);


    // strange stone
    ingred = read_object( VOBJ_INGRED_STRANGE_STONE, VIRTUAL);
    if( !ingred )
    {
      return FALSE;
    }
    obj_to_obj(ingred, obj);

    // empty potion bottle
    ingred = read_object(VOBJ_POTION_BOTTLES, VIRTUAL);
    if( !ingred )
    {
      return FALSE;
    }
    obj_to_obj(ingred, obj);
/* end experimental code! */

    /* let those who need to know, know ya just finished doing something spiffy */
    act("&+L$n's&N $q &+rp&+Mu&+Rl&+Ms&+res &+Lslightly and &+wgrows &+La little larger.&N", FALSE, ch, obj, 0, TO_ROOM);
    act("&+LYour$N $q &+rp&+Mu&+Rl&+Ms&+res &+Lslightly and &+wgrows &+La little larger.&N", FALSE, ch, obj, 0, TO_CHAR);
  }
  return FALSE;
}

int sinister_tactics_staff(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }
  if( !(victim = (P_char) arg) )
  {
    return FALSE;
  }
  // 1/25 chance.
  if( !IS_ALIVE(victim) || number(0, 24) )
  {
    return FALSE;
  }

  act("&+WYour $p &+Br&+be&+Bs&+bo&+Bn&+ba&+Bt&+be&+Bs &+Wwith the scream of &+Lt&+ror&+Rtu&+rre&+Ld &+csouls&+W!&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+W$n's $p &+Br&+be&+Bs&+bo&+Bn&+ba&+Bt&+be&+Bs &+Was it strikes $N.", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+WYour head is filled with the scream of &+Lt&+ror&+Rtu&+rre&+Ld &+csouls&+W!&N", TRUE, ch, obj, victim, TO_VICT);
  if( !fear_check(victim) )
  {
    do_flee(victim, 0, 2);
  }
  return TRUE;
}

int shard_frozen_styx_water(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }
  if( !(victim = (P_char) arg) )
  {
    return FALSE;
  }
  // 1/25 chance.
  if( !IS_ALIVE(victim) || number(0, 24) )
  {
    return FALSE;
  }

  act("&+LYour $p &+Lgets &+Cice &+Wcold&+L causing $N&+L's head to &+Bfreeze&+L!&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+L$n's $p &+Lgets &+Cice &+Wcold&+L causing $N&+L's head to &+Bfreeze&+L!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+L$n's $p &+Lgets &+Cice &+Wcold&+L causing your head to &+Bfreeze&+L!&N", TRUE, ch, obj, victim, TO_VICT);
  spell_feeblemind(35, ch, NULL, 0, victim, 0);
  return (TRUE);
}

int generic_riposte_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // 1/20 chance.
  if (cmd != CMD_GOTHIT || number(0, 19))
  {
    return FALSE;
  }

  if( !(data = (struct proc_data *) arg) )
  {
    return FALSE;
  }
  victim = data->victim;
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  act("$n's $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim, TO_NOTVICT | ACT_NOTTERSE);
  act("$n's $q deflects your blow and strikes YOU!", TRUE, ch, obj, victim, TO_VICT | ACT_NOTTERSE);
  act("Your $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim, TO_CHAR | ACT_NOTTERSE);
  hit(ch, victim, obj);

  return TRUE;

}

int generic_parry_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // 1/20 chance.
  if( cmd != CMD_GOTHIT || number(0, 19) )
  {
    return FALSE;
  }
  // important! can do this cast (next line) ONLY if cmd was CMD_GOTHIT or CMD_GOTNUKED
  if( !(data = (struct proc_data *) arg) )
  {
    return FALSE;
  }
  vict = data->victim;
  if( !IS_ALIVE(vict) )
  {
    return FALSE;
  }
  act("Your $q parries $N's lunge at you.", FALSE, ch, obj, vict, TO_CHAR | ACT_NOTTERSE);
  act("$n's $q parries your futile lunge at $m.", FALSE, ch, obj, vict, TO_VICT | ACT_NOTTERSE);
  act("$n's $q parries $N's lunge at $n.", FALSE, ch, obj, vict, TO_NOTVICT | ACT_NOTTERSE);

  return TRUE;
}

int lightning_armor(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // 1/30 chance on a hit.
  if( cmd != CMD_GOTHIT || number(0, 29) || !(data = (struct proc_data *) arg) )
  {
    return FALSE;
  }

  victim = data->victim;
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }
  act("&+L$n's $q &+bfl&+Bar&+Wes &+Bup &+bat &+L$N's &+chit &+Land &=LBZAPPPPS &N&+W$M&+L!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+L$n's $q &+bfl&+Bar&+Wes &+Bup &+bat &+Lyour &+chit &+Land &=LBZAPPPPS &N&+WYOU&+L!&N", TRUE, ch, obj, victim, TO_VICT);
  act("&+LYour $q &+bfl&+Bar&+Wes &+Bup &+bat &+L$N's &+chit &+Land &=LBZAPPPPS &N&+W$M&+L!&N", TRUE, ch, obj, victim, TO_CHAR);
  spell_lightning_bolt(60, ch, NULL, 0, victim, 0);
  if( IS_ALIVE(victim) && IS_ALIVE(ch) )
  {
    spell_call_lightning(60, ch, victim, 0);
  }
  return TRUE;

}

int imprison_armor(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct proc_data *data;
  struct affected_type af;
  P_char victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_GOTHIT )        // || number(0,30) )
  {
    return FALSE;
  }
  if( !(data = (struct proc_data *) arg) )
  {
    return FALSE;
  }
  if( !(victim = data->victim) )
  {
    return FALSE;
  }

  act("&+L$n's $q &+bfl&+Bar&+Wes &+Bup &+bat &+L$N's &+chit &+Land &=LBZAPPPPS &N&+W$M&+L!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+L$n's $q &+bfl&+Bar&+Wes &+Bup &+bat &+Lyour &+chit &+Land &=LBZAPPPPS &N&+WYOU&+L!&N", TRUE, ch, obj, victim, TO_VICT);
  act("&+LYour $q &+bfl&+Bar&+Wes &+Bup &+bat &+L$N's &+chit &+Land &=LBZAPPPPS &N&+W$M&+L!&N", TRUE, ch, obj, victim, TO_CHAR);
  af.type = SPELL_MAJOR_PARALYSIS;
  af.duration = 5;
  af.bitvector2 = AFF2_MAJOR_PARALYSIS;
  affect_to_char(victim, &af);

  if( IS_FIGHTING(victim) )
  {
    stop_fighting(victim);
  }
  if( IS_DESTROYING(victim) )
  {
    stop_destroying(victim);
  }

  return TRUE;
}



int god_bp(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim, tch;
  P_obj    object;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH],
    Gbuf3[MAX_STRING_LENGTH];
  char     getname[MAX_STRING_LENGTH];
  int      rr;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_GET)
    return FALSE;

  arg = one_argument(arg, Gbuf1);       //multicoin
  if (!Gbuf1)
    return FALSE;


  if (!IS_TRUSTED(ch))
  {
    wizlog(MINLVLIMMORTAL, "%s has a god backpack in [%d]", GET_NAME(ch),
           world[ch->in_room].number);
    return FALSE;
  }


  // check if gbuf2 is a playername in the room
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    sprintf(getname, "%s", GET_NAME(tch));
    for (rr = 0; *(getname + rr) != '\0'; rr++)
      getname[rr] = LOWER(*(getname + rr));

//    if (!isname(Gbuf1, getname))
//          return FALSE;

    if (isname(Gbuf1, getname))
    {                           // we have a winner!
      if ((GET_RACE(tch) == RACE_GOBLIN) || (GET_RACE(tch) == RACE_GNOME))
      {
        act("$n picks up $N.", FALSE, ch, obj, tch, TO_NOTVICT);
        act("$n picks you up.", FALSE, ch, obj, tch, TO_VICT);
        act("You pick up $N.", FALSE, ch, obj, tch, TO_CHAR);
        act("$n opens $s &+ya large leather backpack&N.", FALSE, ch, obj, tch,
            TO_NOTVICT);
        act("$n opens $s &+ya large leather backpack&N.", FALSE, ch, obj, tch,
            TO_VICT);
        act("You open &+ya large leather backpack&N.", FALSE, ch, obj, tch,
            TO_CHAR);
        act("$n throws $N into $s &+ya large leather backpack&N.", FALSE, ch,
            obj, tch, TO_NOTVICT);
        act("$n throws YOU into $s &+ya large leather backpack&N.", FALSE, ch,
            obj, tch, TO_VICT);
        act("You throw $N into &+ya large leather backpack&N.", FALSE, ch,
            obj, tch, TO_CHAR);
        char_from_room(tch);
        char_to_room(tch, real_room(501), -1);
//         act("The flap is opened up.", FALSE, ch, obj, 0, TO_ROOM);
//         act("$N is thrown in by a huge hand.", FALSE, ch, obj, tch, TO_ROOM);
//         act("The flap closes quickly.", FALSE, ch, obj, 0, TO_ROOM);
        return TRUE;
      }
    }
  }
}


int out_of_god_bp(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim, tch;
  P_obj    object;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH],
    Gbuf3[MAX_STRING_LENGTH];
  char     whee[MAX_STRING_LENGTH];
  int      rr, target, bproom;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_TUG)
    return FALSE;

  arg = one_argument(arg, Gbuf1);
  if (!Gbuf1)
    return FALSE;

  // check if gbuf1 is flap
  sprintf(whee, "%s", Gbuf1);
  for (rr = 0; *(whee + rr) != '\0'; rr++)
    whee[rr] = LOWER(*(whee + rr));

  if (!isname(whee, "flap"))
    return FALSE;

  target = real_room(world[ch->specials.was_in_room].number);
  bproom = world[ch->in_room].number;

  if (isname(whee, "flap"))
  {                             // we have a winner!
    if (number(0, 4) == 4)
    {                           // let em out heh
      act("$n opens the leather flap with a heroic show of might.", FALSE, ch,
          obj, tch, TO_NOTVICT);
      act("$n climbs out and quickly closes the flap behind $m.", FALSE, ch,
          obj, tch, TO_NOTVICT);
      act("You summon up all the strength within you and open the &+yflap&N.",
          FALSE, ch, obj, tch, TO_CHAR);
      act("You quickly climb out and close the &+yflap&N behind you.", FALSE,
          ch, obj, tch, TO_CHAR);

      char_from_room(ch);
      char_to_room(ch, real_room(target), -1);

      act
        ("$n quickly climbs out of &+ya large leather backpack&N, closing the flap behind $m.",
         FALSE, ch, obj, 0, TO_ROOM);
      return TRUE;
    }
    else
    {                           // they failed haha
      act
        ("$n attempts to open &+ya large leather backpack&N's huge flap but fails miserably, falling on $m ass.",
         FALSE, ch, obj, tch, TO_NOTVICT);
      act
        ("You attempt open &+ya large leather backpack&N but fail miserably, falling on your ass.",
         FALSE, ch, obj, tch, TO_CHAR);
      Stun(ch, ch, (dice(1, 3) * PULSE_VIOLENCE), FALSE);
      SET_POS(ch, POS_PRONE + GET_STAT(ch));
      CharWait(ch, PULSE_VIOLENCE * 1);

    }
  }

}

int ring_of_regeneration(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time = time(NULL);
  char first_arg[256];

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_WORN_POS(obj, WEAR_FINGER_R) && !OBJ_WORN_POS(obj, WEAR_FINGER_L) )
  {
    return FALSE;
  }

  one_argument(arg, first_arg);

  if( cmd == CMD_RUB && !strcmp("ring", first_arg) )
  {
    if( !IS_SET(obj->extra_flags, ITEM_GLOW) )
    {
      return FALSE;
    }
    else
    {
      act("$n's $q &nhums softly.", TRUE, ch, obj, 0, TO_VICT);
      act("$n's $q &nhums softly.", TRUE, ch, obj, 0, TO_NOTVICT);
      act("Your $q &nhums softly.", TRUE, ch, obj, 0, TO_CHAR);
      obj->timer[0] = curr_time;
      obj->extra_flags &= ~ITEM_GLOW;
      spell_heal(60, ch, 0, 0, ch, 0);
      return TRUE;
    }
  }

  if( cmd == CMD_PERIODIC && !IS_SET(obj->extra_flags, ITEM_GLOW)
    && obj->timer[0] + get_property("timer.proc.ringOfRegeneration", 150) <= curr_time )
  {
    act("Your $q &+Wglows&n with a soft light.", TRUE, obj->loc.wearing, obj, 0, TO_CHAR);
    obj->extra_flags |= ITEM_GLOW;
  }

  return FALSE;
}

int proc_whirlwinds(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time = time(NULL);
  char first_arg[256];

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( (!OBJ_WORN_POS(obj, WEAR_WRIST_R)) && (!OBJ_WORN_POS(obj, WEAR_WRIST_L))
    && (!OBJ_WORN_POS(obj, WEAR_WRIST_LR)) && (!OBJ_WORN_POS(obj, WEAR_WRIST_LL)) )
  {
      obj->extra_flags &= ~ITEM_HUM;
      return FALSE;
  }

  one_argument(arg, first_arg);

  if( cmd == CMD_TAP && !strcmp("bracer", first_arg) )
  {
    if( !IS_SET(obj->extra_flags, ITEM_HUM) )
    {
      return FALSE;
    }
    else
    {
      act("As you tap on $q an immense rush of &+Cvibrating energy&n flows down your limbs!.", TRUE, ch, obj, 0, TO_CHAR);
      act("$n performs a slight gesture around $s forearm and suddenly gains in speed and accuracy!", TRUE, ch, obj, 0, TO_ROOM);
      obj->timer[0] = curr_time;
      obj->extra_flags &= ~ITEM_HUM;
      set_short_affected_by(ch, SKILL_WHIRLWIND, 3 * PULSE_VIOLENCE);
      return TRUE;
    }
  }

  if( cmd == CMD_PERIODIC && !IS_SET(obj->extra_flags, ITEM_HUM)
    && obj->timer[0] + get_property("timer.proc.bracerOfWhirlwinds", 300) <= curr_time )
  {
      act("Your $q starts vibrating and humming quietly.", TRUE, obj->loc.wearing, obj, 0, TO_CHAR);
      obj->extra_flags |= ITEM_HUM;
  }

  return FALSE;
}

int glowing_necklace(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time;
  P_char   watermental;
  int      i, j, sum, elesize;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !OBJ_WORN(obj) || !IS_ALIVE(ch) || IS_SET(world[ch->in_room].room_flags, LOCKER) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_RUB) )
  {
    if( isname(arg, "necklace") || isname(arg, "glowing") )
    {
      if( IS_FIGHTING(ch) )
      {
        send_to_char("You're too busy fighting for your life to reach up for your necklace.\n", ch);
        return FALSE;
      }
      curr_time = time(NULL);
      if( (((obj->timer[0] + (60 * 3)) + number(0, 120)) <= curr_time) || IS_TRUSTED(ch) )
      {
        // Raise to 100 if you want spec pets to occur
        elesize = number(1, 98);

        if( elesize == 100 )
        {
          if( !IS_TRUSTED(ch) && !can_conjure_greater_elem(ch, GET_LEVEL(ch)) )
          {
            elesize = 90;
          }
          else
          {
            act("&+bAs you rub your $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of your $q&+b.  As the\n"
              "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
              "&+BA powerful triton says &N'How may I serve you'", FALSE, ch, obj, obj, TO_CHAR);
            act("&+bAs $n rub $s $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of $s $q&+b.  As the\n"
              "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
              "&+BA powerful triton says &N'How may I serve you'", FALSE, ch, obj, obj, TO_ROOM);
            watermental = read_mobile(1142, VIRTUAL);
          }
        }
        else if( elesize == 99 )
        {
          if( !IS_TRUSTED(ch) && !can_conjure_greater_elem(ch, GET_LEVEL(ch)) )
          {
            elesize = 90;
          }
          else
          {
            act("&+bAs you rub your $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of your $q&+b.  As the\n"
              "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
              "&+bA &+Bb&+e&+Ba&+bu&+Bt&+bi&+Bf&+bu&+Bl undine&N says &N'How may I serve you'", FALSE, ch, obj, obj, TO_CHAR);
            act("&+bAs $n rub $s $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of $s $q&+b.  As the\n"
              "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
              "&+bA &+Bb&+e&+Ba&+bu&+Bt&+bi&+Bf&+bu&+Bl undine&N says &N'How may I serve you'", FALSE, ch, obj, obj, TO_ROOM);
            watermental = read_mobile(1141, VIRTUAL);
          }
        }
        else if (elesize > 90)
        {
          if( !IS_TRUSTED(ch) && !can_conjure_greater_elem(ch, GET_LEVEL(ch)) )
          {
            elesize = 90;
          }
          else
          {
            act("&+bAs you rub your $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of your $q&+b.  As the\n"
              "&+Bwaters &+bsubside, the form of &+Ba HUGE water elemental&+b is left before you.\n"
              "&+bA HUGE water elemental says &N'How may I serve you'", FALSE, ch, obj, obj, TO_CHAR);
            act("&+bAs $n rub $s $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of $s $q&+b.  As the\n"
              "&+Bwaters &+bsubside, the form of &+Ba HUGE water elemental&+b is left before you.\n"
              "&+bA HUGE water elemental says &N'How may I serve you'", FALSE, ch, obj, obj, TO_ROOM);
            watermental = read_mobile(1140, VIRTUAL);
          }
        }
        if (elesize <= 90)
        {
          if( !IS_TRUSTED(ch) && !can_conjure_lesser_elem(ch, GET_LEVEL(ch)) )
          {
            act("&+bYour $q&+L hums briefly...&N", FALSE, ch, obj, obj, TO_CHAR);
            act("&+b$n's $q&+L hums briefly...&N", FALSE, ch, obj, obj, TO_ROOM);
            return FALSE;
          }
          else
          {
            act("&+bAs you rub your $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of your $q&+b.  As the\n"
              "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
              "&+bA water elemental says &N'How may I serve you'", FALSE, ch, obj, obj, TO_CHAR);
            act("&+bAs $n rub $s $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of $s $q&+b.  As the\n"
              "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
              "&+bA water elemental says &N'How may I serve you'", FALSE, ch, obj, obj, TO_ROOM);
            watermental = read_mobile(1103, VIRTUAL);
          }
        }
        if (!watermental)
        {
          act("&=LBTHERE IS NO WATER ELEMENTAL, TELL A GOD!!&N", FALSE, ch, obj, obj, TO_CHAR);
          return FALSE;
        }
        char_to_room(watermental, ch->in_room, 0);

        GET_SIZE(watermental) = SIZE_MEDIUM;
        watermental->player.m_class = CLASS_WARRIOR;
        justice_witness(ch, NULL, CRIME_SUMMON);
        watermental->player.level = 45;
        sum = dice(GET_LEVEL(watermental) * 4, 8) + (GET_LEVEL(watermental) * 3);
        while( watermental->affected )
        {
          affect_remove(watermental, watermental->affected);
        }
        if( !IS_SET(watermental->specials.act, ACT_MEMORY) )
        {
          clearMemory(watermental);
        }
        SET_BIT(watermental->specials.affected_by, AFF_INFRAVISION);
        remove_plushit_bits(watermental);
        GET_MAX_HIT(watermental) = GET_HIT(watermental) = watermental->points.base_hit = sum;
        watermental->points.base_hitroll = watermental->points.hitroll = GET_LEVEL(watermental) / 3;
        watermental->points.base_damroll = watermental->points.damroll = GET_LEVEL(watermental) / 3;
/* Does nothing because watermental is not a monk.
        MonkSetSpecialDie(watermental);
*/
        GET_EXP(watermental) = 0;
        balance_affects(watermental);
        setup_pet(watermental, ch, 1500, PET_NOCASH);
        add_follower(watermental, ch);
        obj->timer[0] = curr_time;
        return TRUE;
      }
      else
      {
        act("&+bAs you rub your $q, &+ba single drop of water drips out...&N", FALSE, ch, obj, obj, TO_CHAR);
        act("&+bAs $n &+brubs $s $q, &+Ba single drop of water drips out...&N", FALSE, ch, obj, obj, TO_ROOM);
      }
    }
  }
  return FALSE;
}

int staff_shadow_summoning(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      curr_time;
  P_char   shadow, aggshadow;
  int      i, j, sum, elesize;
  bool     summoned = FALSE;
  bool     aggsummoned = FALSE;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) )
    return FALSE;

  if( arg && (cmd == CMD_TAP) )
  {
    if (isname(arg, "staff"))
    {
      if( IS_FIGHTING(ch) )
      {
        send_to_char("You're too busy fighting for your life to worry about that.\n", ch);
        return FALSE;
      }
      curr_time = time(NULL);
      if( !IS_TRUSTED(ch) )
      {
        if (obj->value[7] > 100)
        {
          obj->value[7] = 100;
        }

        if( obj->value[7] > 0 )
        {
          // Every 12 min.
          if (obj->timer[0] + (60 * 12) <= curr_time)
          {
            obj->value[7]--;
            shadow = read_mobile(92076, VIRTUAL);
            act("&+LYou tap your $q&+L on the ground... Shadows quickly engulf the area.&N", FALSE, ch, obj, obj, TO_CHAR);
            act("&+LAs $n taps $s $q&+L on the ground..... Shadows quickly engulf the area.&N", FALSE, ch, obj, obj, TO_ROOM);
            summoned = TRUE;
          }
          else
          {
            act("&+LAs you tap your $q&+L..... Nothing Happens!&N", FALSE, ch, obj, obj, TO_CHAR);
            act("&+LAs $n taps $s $q&+L..... Nothing Happens!&N", FALSE, ch, obj, obj, TO_ROOM);
            return TRUE;
          }
        }
        // Every 15 sec .. oh aggro.
        else if( obj->timer[0] + 15 <= curr_time )
        {
          obj->value[7]--;
          shadow = read_mobile(200, VIRTUAL);
          act("&+LYou tap your $q&+L on the ground... Shadows quickly engulf the area.&N", FALSE, ch, obj, obj, TO_CHAR);
          act("&+LAs $n taps $s $q&+L on the ground..... Shadows quickly engulf the area.&N", FALSE, ch, obj, obj, TO_ROOM);
          summoned = TRUE;
          aggsummoned = TRUE;
        }
        else
        {
          act("&+LAs you tap your $q&+L..... Nothing Happens!&N", FALSE, ch, obj, obj, TO_CHAR);
          act("&+LAs $n taps $s $q&+L..... Nothing Happens!&N", FALSE, ch, obj, obj, TO_ROOM);
          return TRUE;
        }
      }
      else
      {
        shadow = read_mobile(92076, VIRTUAL);
        act("&+LYou tap your $q &+Lon the ground... Shadows quickly engulf the area.&N", FALSE, ch, obj, obj, TO_CHAR);
        act("&+LAs $n taps $s $q &+Lon the ground..... Shadows quickly engulf the area.&N", FALSE, ch, obj, obj, TO_ROOM);
        summoned = TRUE;
      }


      if( summoned )
      {
        if( !shadow )
        {
          act("&+WTHERE IS NO SHADOW, TELL A GOD!!&N", FALSE, ch, obj, obj, TO_CHAR);
          return FALSE;
        }
        char_to_room(shadow, ch->in_room, 0);

        GET_SIZE(shadow) = SIZE_MEDIUM;
        shadow->player.m_class = CLASS_WARRIOR;
        justice_witness(ch, NULL, CRIME_SUMMON);
        shadow->player.level = 40;
        sum = dice(GET_LEVEL(shadow) * 4, 8) + (GET_LEVEL(shadow) * 3);
        while( shadow->affected )
        {
          affect_remove(shadow, shadow->affected);
        }
        if( !IS_SET(shadow->specials.act, ACT_MEMORY) )
        {
          clearMemory(shadow);
        }
        SET_BIT(shadow->specials.affected_by, AFF_INFRAVISION);
        remove_plushit_bits(shadow);
        GET_MAX_HIT(shadow) = GET_HIT(shadow) = shadow->points.base_hit = sum;
        shadow->points.base_hitroll = shadow->points.hitroll = GET_LEVEL(shadow) / 3;
        shadow->points.base_damroll = shadow->points.damroll = GET_LEVEL(shadow) / 3;
/* This does nothing because shadow is not a Monk.
        MonkSetSpecialDie(shadow);
*/
        GET_EXP(shadow) = 0;
        if( !aggsummoned )
        {
          REMOVE_BIT(shadow->only.npc->aggro_flags, AGGR_ALL);
          balance_affects(shadow);
          setup_pet(shadow, ch, 1500, PET_NOCASH);
          add_follower(shadow, ch);
        }
        obj->timer[0] = curr_time;
        if( obj->value[7] < 0 )
        {
          obj->value[7] = 0;
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}


int rod_of_magic(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;
  P_char   victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || obj->loc.wearing != ch )
  {
    return FALSE;
  }

  victim = (P_char) arg;
  // 1/16 chance.
  if( !IS_ALIVE(victim) || number(0, 15) )
  {
    return (FALSE);
  }

  act("&+L$n's&N $q &+Lspews &+wforth &+mmeta&+Bmagic&+W!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+LYour&N $q &+Lspews &+wforth &+mmeta&+Bmagic&+W!&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+L$n's&N $q &+Lspews &+wforth &+mmeta&+Bmagic&+W!&N&N", TRUE, ch, obj, victim, TO_VICT);
  if( victim )
  {
    spell_stornogs_lowered_magical_res(60, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  }
  return TRUE;
}

int nightcrawler_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    dam = cmd / 1000;
  P_char victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) )
  {
    return FALSE;
  }

  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || number(0, 29) )
  {
    return FALSE;
  }

  act("&+LA beam of d&+barkne&+Lss erupts from the tip of $n's&N $q!&N", TRUE, ch, obj, victim, TO_NOTVICT);
  act("&+LA beam of d&+barkne&+Lss erupts from the tip of your&N $q!&N", TRUE, ch, obj, victim, TO_CHAR);
  act("&+LA beam of d&+barkne&+Lss erupts from the tip of $n's&N $q!&N&N", TRUE, ch, obj, victim, TO_VICT);
  spell_devitalize(50, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
  return TRUE;
}


// Mossi Modification:   Moving DECAY Procs

int blood_stains(P_obj obj, P_char ch, int cmd, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  const char *long_desc_reg[] = {
    "&+rBlood splatters cover the area.&n",
    "&+rA few drops of blood are scattered around the area.&n",
    "&+rPuddles of blood cover the ground.&n",
    "&+rBlood covers everything in the area.&n"
  };
  const char *long_desc_dry[] = {
    "&+rDried blood splatters cover the area.&n",
    "&+rA few drops of dry blood are scattered around the area.&n",
    "&+rPuddles of crusty blood cover the ground.&n",
    "&+rCrusty blood covers everything in the area.&n"
  };

  // Set the timer when obj is loaded
  if( cmd == CMD_SET_PERIODIC )
  {
    obj->timer[0] = time(NULL);
    return TRUE;
  }

  // We don't show a decay message.
  if( cmd == CMD_DECAY )
  {
    return TRUE;
  }

  if( cmd == CMD_PERIODIC && obj->value[1] < BLOOD_DRY )
  {
    // Change it up after 90 seconds
    if( !IS_SET(obj->extra_flags, ITEM_NOSHOW) && (obj->timer[0] < (time(NULL) - 90)) )
    {
      SET_BIT(obj->extra_flags, ITEM_NOSHOW);
      return TRUE;
    }

    // Change it up after 3 minutes
    if( (obj->value[1] == BLOOD_FRESH) && (obj->timer[0] < (time(NULL) - 180)) )
    {
      sprintf(buf, "%s", long_desc_reg[obj->value[0]]);
      obj->description = str_dup(buf);
      obj->value[1] = BLOOD_REG;
      return TRUE;
    }

    // Change it up after 7 minutes
    if( (obj->value[1] == BLOOD_REG) && (obj->timer[0] < (time(NULL) - 420)) )
    {
      sprintf(buf, "%s", long_desc_dry[obj->value[0]]);
      obj->description = str_dup(buf);
      obj->value[1] = BLOOD_DRY;
      return TRUE;
    }
  }
  return FALSE;
}

int tracks(P_obj obj, P_char ch, int cmd, char *argument)
{
  if (cmd == CMD_DECAY)
  {
     return TRUE;
  }
  return FALSE;
}

int ice_shattered_bits(P_obj obj, P_char ch, int cmd, char *argument)
{
  if (cmd == CMD_DECAY)
  {
    if (world[obj->loc.room].people)
    {
      act("$p melt into nothingness.",
          TRUE, world[obj->loc.room].people, obj, 0, TO_ROOM);
      act("$p melt into nothingness.",
          TRUE, world[obj->loc.room].people, obj, 0, TO_CHAR);
    }
    return TRUE;
  }
  return FALSE;
}

int ice_block(P_obj obj, P_char ch, int cmd, char *argument)
{
  if (cmd == CMD_DECAY)
    return TRUE;

  return FALSE;
}

int frost_beacon(P_obj obj, P_char ch, int cmd, char *argument)
{
  P_char   tch;
  char     buf[1024];

  memset(buf, 0, sizeof(buf));

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DECAY)
  {
    // Give a message to the caster if they're not in the room.
    for (tch = character_list; tch; tch = tch->next)
      if (IS_PC(tch) && GET_PID(tch) == obj->value[0])
      {
        if( tch->in_room != obj->loc.room )
          act("$p melts into nothingness.", TRUE, tch, obj, 0, TO_CHAR);
        break;
      }

    // Give a message to all in the room.
    if (world[obj->loc.room].people)
    {
      act("$p melts into nothingness.", TRUE, world[obj->loc.room].people,
          obj, 0, TO_ROOM);
      act("$p melts into nothingness.", TRUE, world[obj->loc.room].people,
          obj, 0, TO_CHAR);
    }
    return TRUE;
  }

  if (!ch || (ch->specials.z_cord != 0))
    return FALSE;

  if (number(0, 2) && IS_SET(obj->extra_flags, ITEM_SECRET) &&
      cmd_to_exitnumb(cmd) != -1 && obj->value[0] && obj->loc_p == LOC_ROOM &&
      (IS_PC(ch) && obj->value[0] != GET_PID(ch)) && !IS_TRUSTED(ch))
  {

    for (tch = character_list; tch; tch = tch->next)
      if (IS_PC(tch) && GET_PID(tch) == obj->value[0])
        break;

    if (tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch))
    {
      sprintf(buf, "$N has set off your frost beacon at %s!",
              world[ch->in_room].name);
      act(buf, FALSE, tch, 0, ch, TO_CHAR);
    }
  }

  return FALSE;
}

int random_tomb(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      bits;
  int      to_room;
  P_char   dummy;
  P_obj    obj2 = NULL;
  P_obj    tmp_object = NULL, k;
  bool     found_something = FALSE, have_one = FALSE;


  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || ((cmd != CMD_DIG) && (cmd != CMD_SEARCH)))
    return FALSE;

 if(IS_NPC(ch))
     return TRUE;

  if (world[ch->in_room].sector_type == 0)
  {
    act("You already found a secret zone here, but sure!", FALSE, ch, 0, 0,
        TO_CHAR);
    return FALSE;

  }

  if (cmd == CMD_DIG)
  {

    if (ch->equipment[HOLD])
    {
      tmp_object = ch->equipment[HOLD];
      if (isname("shovel", tmp_object->name) ||
          isname("hoe", tmp_object->name) || isname("pick", tmp_object->name))
        have_one = TRUE;
    }

    if (!have_one)
    {
      send_to_char("Using what? Your fingers?\n", ch);
      return FALSE;
    }

    if (number(0, 7))
    {
      act("You dig up absolutely nothing!", FALSE, ch, 0, 0, TO_CHAR);
      act("$n digs up absolutely nothing!", FALSE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      act("You dig into a pieces of bone.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n digs some bones.", FALSE, ch, 0, 0, TO_ROOM);
      do_search(ch, NULL, 0);
      world[ch->in_room].sector_type = 0;
    }
  }
  else if (cmd == CMD_SEARCH)
  {
    send_to_char("You don't find anything you didn't see before.\n", ch);
  }

  CharWait(ch, 3);

  return TRUE;
}

int random_glass(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      bits;
  int      to_room;
  P_char   dummy;
  P_obj    obj2 = NULL;
  P_obj    tmp_object = NULL, k;
  bool     found_something = FALSE, have_one = FALSE;


  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || ((cmd != CMD_DIG) && (cmd != CMD_SEARCH)))
    return FALSE;

  if (IS_NPC(ch))
    return TRUE;

  if (world[ch->in_room].sector_type == 0)
  {
    act("You already found a secret zone here, but sure!", FALSE, ch, 0, 0,
        TO_CHAR);
    return FALSE;

  }

  if (cmd == CMD_DIG)
  {

    if (ch->equipment[HOLD])
    {
      tmp_object = ch->equipment[HOLD];
      if (isname("shovel", tmp_object->name) ||
          isname("hoe", tmp_object->name) || isname("pick", tmp_object->name))
        have_one = TRUE;
    }

    if (!have_one)
    {
      send_to_char("Using what? Your fingers?\n", ch);
      return FALSE;
    }

    if (number(0, 7))
    {
      act("You dig up absolutely nothing!", FALSE, ch, 0, 0, TO_CHAR);
      act("$n digs up absolutely nothing!", FALSE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      act("You dig into a pieces of glass.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n digs some bones.", FALSE, ch, 0, 0, TO_ROOM);
      do_search(ch, NULL, 0);
      world[ch->in_room].sector_type = 0;
    }
  }
  else if (cmd == CMD_SEARCH)
  {
    send_to_char("You don't find anything you didn't see before.\n", ch);
  }

  CharWait(ch, 3);

  return TRUE;
}

int random_slab(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      bits;
  int      to_room;
  P_char   dummy;
  P_obj    obj2 = NULL;
  P_obj    tmp_object = NULL, k;
  bool     found_something = FALSE, have_one = FALSE;


  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!ch || ((cmd != CMD_DIG) && (cmd != CMD_SEARCH)))
    return FALSE;

  if (IS_NPC(ch))
    return TRUE;

  if (world[ch->in_room].sector_type == 0)
  {
    act("You already found a secret zone here, but sure!", FALSE, ch, 0, 0,
        TO_CHAR);
    return FALSE;

  }

  if (cmd == CMD_DIG)
  {

    if (ch->equipment[HOLD])
    {
      tmp_object = ch->equipment[HOLD];
      if (isname("shovel", tmp_object->name) ||
          isname("hoe", tmp_object->name) || isname("pick", tmp_object->name))
        have_one = TRUE;
    }

    if (!have_one)
    {
      send_to_char("Using what? Your fingers?\n", ch);
      return FALSE;
    }

    if (number(0, 7))
    {
      act("You dig up absolutely nothing!", FALSE, ch, 0, 0, TO_CHAR);
      act("$n digs up absolutely nothing!", FALSE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      do_search(ch, NULL, 0);
      world[ch->in_room].sector_type = 0;
    }
  }
  else if (cmd == CMD_SEARCH)
  {
    send_to_char("You don't find anything you didn't see before.\n", ch);
  }

  CharWait(ch, 3);

  return TRUE;
}

int lyrical_instrument_of_time(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      rand;
  int      curr_time;
  P_char   temp_ch;
  P_char   vict;
  char     e_pos;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (cmd != 0)
    return FALSE;

  if (!obj)
    return FALSE;

  temp_ch = ch;

  if (!OBJ_WORN(obj))
    return FALSE;

  if (!ch)
  {
    if (OBJ_WORN(obj) && obj->loc.wearing)
      temp_ch = obj->loc.wearing;
    else
      return FALSE;
  }

  if (!temp_ch || !SINGING(temp_ch))
    return FALSE;


  if (!(obj == temp_ch->equipment[HOLD]))
    return FALSE;

  curr_time = time(NULL);

  if (!IS_SET(world[temp_ch->in_room].room_flags, NO_MAGIC))
  {
    if (obj->timer[0] + 30 <= curr_time)
    {
      obj->timer[0] = curr_time;
      if (GET_HIT(temp_ch) < GET_MAX_HIT(temp_ch))
      {
        act
          ("&+L$n&+L's $q &+Yglows&+L and $n &+Lis bathed in a healing aura.&N",
           FALSE, temp_ch, obj, 0, TO_ROOM);
        act("&+LYour $q &+Yglows&+L and bathes you in a healing aura.&N",
            FALSE, temp_ch, obj, 0, TO_CHAR);

        bard_healing(60, temp_ch, temp_ch, SONG_HEALING);

        return (FALSE);

      }
      else
      {

        bard_protection(60, temp_ch, temp_ch, SONG_PROTECTION);

        return (FALSE);
      }
    }
  }

  if (IS_FIGHTING(temp_ch) && !number(0, 2))
  {
    vict = temp_ch->specials.fighting;

    if (!vict)
      return FALSE;

    rand = number(0, 5);

    act("&+LYou point your $p &+Lat &+W$N&+L.&N", TRUE, temp_ch, obj, vict,
        TO_CHAR);
    act("&+L$n points $p &+Lat &+W$N&+L.&N", TRUE, temp_ch, obj, vict,
        TO_NOTVICT);
    act("&+L$n points $p &+Lat &+Wyou&+L!&N", TRUE, temp_ch, obj, vict,
        TO_VICT);

    switch (rand)
    {
    case 0:
      bard_storms(60, temp_ch, vict, SONG_STORMS);
      break;
    case 1:
      bard_chaos(60, temp_ch, vict, SONG_CHAOS);

      break;
    case 2:
      bard_harming(60, temp_ch, vict, SONG_HARMING);

      break;
    case 3:
      bard_harming(60, temp_ch, vict, SONG_HARMING);
      break;
    case 4:
      bard_cowardice(60, temp_ch, vict, SONG_COWARDICE);
      break;
    case 5:
      bard_calm(60, temp_ch, vict, SONG_CALMING);
      break;
    }
    return FALSE;
  }
  return FALSE;

}


int ogre_warlords_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !dam || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  // 1/30 chance
  if( !number(0, 29) )
  {
    act("$n &+bsuddenly fills with an ogrish fury!", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("&+rThe fury of ogre kin starts to grow within you...&n\n", ch);
    berserk(ch, 6 * PULSE_VIOLENCE);
  }

  return FALSE;
}

#define AZER 7359

int flaming_axe_of_azer(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   tmp_ch, vict;
  int      room, level;
  int      dam = cmd / 1000;


  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !dam || !IS_ALIVE(ch) || !(room = ch->in_room) || !(vict = (P_char) arg) )
  {
    return FALSE;
  }

  // 1/30 chance
  if( !number(0, 29) )
  {

    level = BOUNDED(10, GET_LEVEL(ch)+number(-5, 5), 60);

    if( !(tmp_ch = read_mobile(AZER, VIRTUAL)) )
    {
      send_to_char("&+WSerious screw-up in your weapon's proc. Tell a coder immidiately.&n\n", ch);
      return FALSE;
    }
    char_to_room(tmp_ch, room, -1);
    act("&+rIn a column of &+RROARING&+r fire, $N&+r makes $S entrance.&n",
        FALSE, ch, 0, tmp_ch, TO_CHAR);
    act("&+rIn a column of &+RROARING&+r fire, $N&+r makes $S entrance.&n",
        FALSE, ch, 0, tmp_ch, TO_ROOM);
    group_add_member(ch, tmp_ch);

    if( is_char_in_room(ch, room) && is_char_in_room(vict, room) )
    {
      switch (number(0, 3))
      {
      case 0:
        spell_fireball(level, tmp_ch, NULL, 0, vict, 0);
        break;
      case 1:
        spell_magma_burst(level, tmp_ch, 0, 0, vict, 0);
        break;
      case 2:
        spell_molten_spray(level, tmp_ch, 0, 0, vict, 0);
        break;
      case 3:
        spell_immolate(level, tmp_ch, NULL, 0, vict, 0);
        break;
      default:
        break;
      }
    }

    act("&+rAfter helping $S master, $n&+r departs hastily.", FALSE, tmp_ch, 0, ch, TO_NOTVICT);
    act("&+rAfter helping you, $n&+r departs hastily.", FALSE, tmp_ch, 0, ch, TO_VICT);
    char_from_room(tmp_ch);
    char_to_room(tmp_ch, real_room(1), -1);
    extract_char(tmp_ch);
    // If we killed'm then don't execute attack.
    if( !IS_ALIVE(vict) )
    {
      return TRUE;
    }
  }
  // for to execute normal hit
  return FALSE;
}


int mrinlor_whip(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int      dam = cmd / 1000;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !dam || !IS_ALIVE(ch) || !obj || !OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_BY(obj, ch) || !(vict = (P_char) arg) )
  {
    return FALSE;
  }

  // 1/14 chance
  if( IS_FIGHTING(ch) && !number(0, 13) )
  {
      act("&nYour $p cracks as it whips $N and launches a &+rfireball&n!",
        TRUE, ch, obj, vict, TO_CHAR);
      act("&n$n's $p violently whips $N and launches a &+rfireball!&n",
        TRUE, ch, obj, vict, TO_NOTVICT);
      act("&+COUCH!&n $n just whipped you with his $p, leaving a &+rfireball&n coming right at you!&n",
        TRUE, ch, obj, vict, TO_VICT);

      spell_fireball(30, ch, NULL, 0, vict, 0);
  }
  return FALSE;
}

int khildarak_warhammer(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   tmp_ch, vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !obj )
  {
    wizlog(56, "khildarak_warhammer: &+WNo obj in proc.&n");
    return FALSE;
  }

  if( !OBJ_WORN(obj) || !(ch = obj->loc.wearing) || cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  // 1/30 chance for spellup.
  if( !number(0, 29) )
  {
    act("&+LThe strength of $p&+L infuses you!", FALSE, ch, obj, 0, TO_CHAR);

    switch (number(0, 6))
    {
    case 0:
      spell_barkskin(40, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
      break;
    case 1:
      spell_hawkvision(40, ch, 0, 0, ch, 0);
      break;
    case 2:
      spell_bless(40, ch, 0, 0, ch, 0);
      break;
    case 3:
      spell_bearstrength(40, ch, 0, 0, ch, 0);
      break;
    case 4:
      spell_combat_mind(32, ch, 0, 0, ch, 0);
      break;
    case 5:
      spell_spirit_armor(40, ch, 0, 0, ch, 0);
      break;
    case 6:
      spell_vigorize_serious(40, ch, 0, 0, ch, 0);
      break;
    }
    return FALSE;
  }
  return FALSE;
}

int mace_dragondeath(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   temp_ch;
  P_char   vict;
  int      dam, curr_time;
  char     e_pos;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC && cmd != CMD_MELEE_HIT )
  {
    return FALSE;
  }

  if( !obj || !OBJ_WORN(obj) || !(temp_ch = obj->loc.wearing) )
  {
    return FALSE;
  }

  if( !(obj == temp_ch->equipment[WIELD]) )
  {
    return FALSE;
  }

  if( cmd == CMD_PERIODIC && !IS_SET(world[temp_ch->in_room].room_flags, NO_MAGIC))
  {
    curr_time = time(NULL);

    if( obj->timer[0] + 60 <= curr_time )
    {
      obj->timer[0] = curr_time;
      // 1/10 chance each minute
      if( !number(0, 9) )
      {
        act("&+wYour $q &+wbegins to vibrate softly.\n"
          "&+wSuddenly, it flares &+Rbright red&+w and a &+rcrimson&+w cloud of &+Lsmoke&+w pours forth.\n"
          "&+wThe spirits of the &+LDragons&+w trapped within infuse you with power to combat\n"
          "&+wtheir bretheren!", FALSE, temp_ch, obj, 0, TO_CHAR);

        act("&+w$n&+w's $q &+wbegins to vibrate softly.\n"
          "&+wSuddenly, it flares &+Rbright red&+w and a &+rcrimson&+w cloud of &+Lsmoke&+w pours forth.\n"
          "&+wThe spirits of the &+LDragons&+w trapped within infuse you with power to combat\n"
          "&+wtheir bretheren!", FALSE, temp_ch, obj, 0, TO_ROOM);

        bard_dragons(60, temp_ch, temp_ch, 0);
        return FALSE;
      }
    }
  }

  // 1/10 chance on a hit
  if( cmd == CMD_MELEE_HIT && !number(0, 9) )
  {

    if( !(vict = (P_char) arg) )
      return FALSE;

    if( IS_DRAGON(vict) || (GET_RACE(vict) == RACE_DRAGONKIN) )
    {
      act("&+LYour $q &+Lbites deeply into $N!", FALSE, temp_ch, obj, vict, TO_CHAR);
      act("&+L$n&+L grins as $p&+L bites deeply into $N!", FALSE, temp_ch, obj, vict, TO_NOTVICT);
      act("&+LYou double over in pain as $p&+L bites deeply into your flesh!", FALSE, temp_ch, obj, vict, TO_VICT);

      dam = number(400, 500);
      melee_damage(temp_ch, vict, dam, PHSDAM_NOSHIELDS | PHSDAM_TOUCH, 0);
      return FALSE;
    }
  }
  return FALSE;
}


int lucky_weapon(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int      i;
  int      room;
  int      dam = cmd / 1000;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !dam || !IS_ALIVE(ch) || !(vict = (P_char) arg) )
  {
    return FALSE;
  }
  room = ch->in_room;
  if( !IS_ALIVE(vict) || vict->in_room != room )
  {
    return FALSE;
  }

  // 1/30 chance
  if( !number(0, 29) )
  {
    send_to_char("&=LWYou score a CRITICAL HIT!!!&N\n", ch);
    make_bloodstain(ch);
    if( !number(0, 2) )
    {
      spell_serendipity(40, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
    }

    hit(ch, vict, obj);
  }
  else if( !number(0, 100) )
  {
    send_to_char("&=LWYou score a REALLY LUCKY round of hits!!!!!&N\n", ch);
    spell_serendipity(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
    make_bloodstain(ch);
    make_bloodstain(ch);
    make_bloodstain(ch);
    for( i = number(2, 6); i; i-- )
    {
      if( is_char_in_room(ch, room) && is_char_in_room(vict, room) )
      {
        hit(ch, vict, obj);
      }
    }
    return TRUE;
  }

  return FALSE;

}

int glades_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int      i;
  int      dam = cmd / 1000;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !obj || !IS_ALIVE(ch) )
  if(!IS_ALIVE(ch))
  {
    return FALSE;
  }

  if( (IS_UNDEAD(ch) || IS_UNDEADRACE(ch)) && OBJ_WORN_BY(obj, ch) && !number(0, 2) )
  {
    act("&+B$p briefly flashes with &N&+Yintense light&N&+B, burning your hand severely!&N",
      TRUE, ch, obj, vict, TO_CHAR);
    act("$N screams in pain, as $S $p burns into $S skin!", FALSE, ch, obj,
      vict, TO_ROOM);
    spell_damage(ch, ch, number(120, 240), SPLDAM_HOLY, SPLDAM_NOSHRUG, 0);
    return FALSE;
  }

  if( !dam || !(vict = (P_char) arg) )
  {
    return FALSE;
  }
  if( !IS_ALIVE(vict) )
  {
    return FALSE;
  }

  // 1/30 chance
  if( !number(0, 29) )
  {
    act("&+BYour $p briefly flashes as its blade touches $N and burns $S skin!&n",
      TRUE, ch, obj, vict, TO_CHAR);
    act("&+B$p briefly flashes as its blade touches $N and burns $S skin!&n",
      TRUE, ch, obj, vict, TO_NOTVICT);
    act("&+BOUCH! $n just burned you with his $p!&n", TRUE, ch, obj, vict, TO_VICT);

    if( IS_UNDEAD(vict) )
    {
      spell_destroy_undead(50, ch, NULL, 0, vict, 0);
    }
    else if( IS_EVIL(vict) )
    {
      spell_dispel_evil(50, ch, NULL, SPELL_TYPE_SPELL, vict, 0);
    }
    else
    {
      spell_cause_critical(50, ch, NULL, SPELL_TYPE_SPELL, vict, 0);
    }
    return FALSE;
  }
  return FALSE;
}

int cold_hammer(P_obj obj, P_char ch, int cmd, char *arg)
{
  int    dam;
  P_char vict;
  struct proc_data *data;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( OBJ_ROOM(obj) && cmd == CMD_PERIODIC )
  {
    hummer(obj);
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_MELEE_HIT )
  {
    if( !OBJ_WORN_POS(obj, WIELD) )
    {
      return FALSE;
    }
    vict = (P_char) arg;

    if( OBJ_WORN_BY(obj, ch) && IS_ALIVE(vict) )
    {
      // 1/15 chance.
      if (!number(0, 14))
      {
        act("&+yYour $q &+ysummons a wind full with &+Ysharp&+y stones down on $N!", FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
        act("$n's $q &+ysummons a wind full with &+Ysharp&+y stones down on you!", FALSE, obj->loc.wearing, obj, vict, TO_VICT);
        act("$n's $q &+ysummons a wind full with &+Ysharp&+y stones down on $N!", FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
        spell_greater_living_stone(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);

        if( char_in_list(vict) )
        {
           spell_living_stone(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
        }
      }
    }
    return FALSE;
  }
  else if( cmd == CMD_GOTNUKED && !number(0, 3) )
  {
    if( !(data = (struct proc_data *) arg) )
    {
      return FALSE;
    }
    vict = data->victim;
    dam = data->dam;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }
    act("$n's $p calls down the power of heavens into the room!", FALSE, ch, obj, vict, TO_NOTVICT);
    act("$n's $p calls down the power of heavens into the room!", FALSE, ch, obj, vict, TO_VICT);
    act("Your $p calls down the power of heavens into the room!", FALSE, ch, obj, vict, TO_CHAR);

    spell_napalm(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    if( !number(0, 2) && IS_ALIVE(vict) && IS_ALIVE(ch) )
    {
      spell_napalm(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
    }
    if( !number(0, 19) && IS_ALIVE(ch) )
    {
      spell_meteorswarm(40, ch, 0, SPELL_TYPE_SPELL, NULL, 0);
    }
    if( !number(0, 19) && IS_ALIVE(ch) )
    {
      spell_earthen_rain(40, ch, 0, SPELL_TYPE_SPELL, NULL, 0);
    }
    return FALSE;
  }
  else if( cmd == CMD_GOTHIT && !number(0, 5) )
  {
    if( !(data = (struct proc_data *) arg) )
    {
      return FALSE;
    }
    vict = data->victim;
    dam = data->dam;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }
    act("Your hammer parries $N's vicious attack.", FALSE, ch, 0, vict, TO_CHAR | ACT_NOTTERSE);
    act("$n's hammer parries your futile attack.", FALSE, ch, 0, vict, TO_VICT | ACT_NOTTERSE);
    act("$n's hammer parries $N's attack.", FALSE, ch, 0, vict, TO_NOTVICT | ACT_NOTTERSE);
    return TRUE;
  }
  return FALSE;
}

int refreshing_fountain(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct affected_type af;
  char     Gbuf4[MAX_STRING_LENGTH];


  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd == CMD_DRINK)
  {
    if (!arg || !*arg)
      return FALSE;
    arg = skip_spaces(arg);
    if (*arg && !strcmp(arg, "spring"))
    {
      act("You drink from $p.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n drinks from $p.", FALSE, ch, obj, 0, TO_ROOM);
      if (!affected_by_spell(ch, SPELL_REFRESHING_FOUNTAIN))
      {

        act("$p&+W's &+Cwater&+W touches your soul!&n", TRUE, ch, obj, 0,
            TO_CHAR);
        bzero(&af, sizeof(af));
        af.type = SPELL_REFRESHING_FOUNTAIN;
        af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW;
        af.duration = 1 * PULSE_VIOLENCE;
        af.location = APPLY_HIT_REG;
        af.modifier = 1000;
        affect_to_char(ch, &af);
        af.location = APPLY_MOVE_REG;
        af.modifier = 40;
        affect_to_char(ch, &af);
      }
      else
        act("$p&+W's &+Cwater&+W touches your soul!&n", TRUE, ch, obj, 0,
            TO_CHAR);

      CharWait(ch, PULSE_VIOLENCE);
      return TRUE;
    }
  }

  return FALSE;
}



int magical_fountain(P_obj obj, P_char ch, int cmd, char *arg)
{
  return FALSE;
}

int cutting_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
  struct damage_messages messages = {
    "&+wThe &+Lblade &+won &N$q &+rcu&+Rts &+wyour finger drawing a little &+rb&+Rl&+roo&+Rd&+w.&N",
      0,
    "&+wThe &+Lblade &+won &N$n's &N$q &+rcu&+Rts &+wyour finger drawing a little &+rb&+Rl&+roo&+Rd&+w.&N",
      0, 0, 0, 0, obj
  };

  if( IS_ALIVE(ch) && IS_PC(ch) && OBJ_ROOM(obj) && cmd == CMD_GET &&
      obj == get_obj_in_list_vis(ch, arg, world[ch->in_room].contents) )
  {
    spell_damage(ch, ch, 4, SPLDAM_GENERIC, 0, &messages);
  }

  return FALSE;
}

int huntsman_ward(P_obj obj, P_char ch, int cmd, char *argument)
{
  P_char   tch;
  int      dam, item;
  char     buf[256];

  item = GET_OBJ_VNUM(obj);

  if( cmd == CMD_HIDE )
  {
    one_argument(argument, buf);

    if( GET_CHAR_SKILL(ch, SKILL_TRAP) && OBJ_ROOM(obj)
      && obj == get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))
    {

      act("You arm $p and hide it from the eyes of any trespassers.", FALSE, ch, obj, NULL, TO_CHAR);
      act("$n arms $p and hides it from the eyes of any trespassers.", FALSE, ch, obj, NULL, TO_ROOM);
      SET_BIT(obj->extra_flags, ITEM_SECRET);
      set_obj_affected(obj, 1800 * WAIT_SEC, TAG_OBJ_DECAY, 0);
      CharWait(ch, PULSE_VIOLENCE);
      if( IS_PC(ch) )
      {
        obj->value[0] = GET_PID(ch);
      }
      else
      {
        obj->value[0] = GET_RNUM(ch);
      }
      return TRUE;
    }
  }

  if (cmd == CMD_FOUND)
  {
    act("You succesfully disarmed $p hidden here.", FALSE, ch, obj, 0, TO_CHAR);
    ClearObjEvents(obj);
    obj->value[0] = 0;
    return TRUE;
  }

  if( number(0, 2) && IS_SET(obj->extra_flags, ITEM_SECRET)
    && cmd_to_exitnumb(cmd) != -1 && obj->value[0] && OBJ_ROOM(obj)
    && (IS_PC(ch) && obj->value[0] != GET_PID(ch)) && !IS_TRUSTED(ch) )
  {

    for (tch = character_list; tch; tch = tch->next)
    {
      if (IS_PC(tch) && GET_PID(tch) == obj->value[0])
      {
        break;
      }
    }

    if (item == 54)
    {
      if( tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch) )
      {
        sprintf(buf, "$N has trespassed in %s!", world[ch->in_room].name);
        act(buf, FALSE, tch, 0, ch, TO_CHAR);
        REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
        obj->value[0] = 0;
        ClearObjEvents(obj);

        if( number(0, 120) < GET_C_INT(ch) )
        {
          act("You notice you just broke a &+Wthin string&n attached to $p!", FALSE, ch, obj, 0, TO_CHAR);
        }
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      }

      return FALSE;
    }

    if( item == 77 )
    {
      if( tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch) )
      {
        REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
        obj->value[0] = 0;
        ClearObjEvents(obj);

        // PHSDAM_NOREDUCE -> 5d5 damage total.
        dam = dice(5, 5);

        act("Without warning, a hidden trap sends a flurry of tiny &+Lblack&n darts piercing $n's &+rflesh&n.",
          FALSE, ch, obj, 0, TO_NOTVICT);
        act("Without warning, a hidden trap sends a flurry of tiny &+Lblack&n darts piercing your &+rflesh&n.",
          FALSE, ch, obj, 0, TO_CHAR);

        melee_damage(tch, ch, dam, PHSDAM_NOREDUCE | PHSDAM_NOSHIELDS | PHSDAM_NOPOSITION | PHSDAM_NOENGAGE, 0);

        extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      }
      return FALSE;
    }

    if( item == 400229 )
    {
      struct affected_type af;
      if( tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch) )
      {
        sprintf(buf, "$N &+yhas sprung your &+rcrippling &+ytrap at&n %s!", world[ch->in_room].name);
        act(buf, FALSE, tch, 0, ch, TO_CHAR);
        REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
        obj->value[0] = 0;
        ClearObjEvents(obj);

        if (number(0, 120) < GET_C_INT(ch))
        {
          act("You notice you just broke a &+Wthin string&n attached to $p!", FALSE, ch, obj, 0, TO_CHAR);
        }

        memset(&af, 0, sizeof(af));

        af.type = TAG_CRIPPLED;
        af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL ;
        af.duration = 40;
        affect_to_char(ch, &af);
        act("&+ROUCH!!&+y Without warning, a &+rrusty &+yclamp suddenly tears at your legs!&n", FALSE, ch, 0, 0, TO_CHAR);
        act("$n &+ywinces in &+ragony &+yas a &+rrusty &+yclamp suddenly tears at their legs!&n", FALSE, ch, 0, 0, TO_ROOM);

        // 3-7 * ~5 damage = 15-35 damage total.
        int	numb = number(3, 7);
        add_event(event_bleedproc, PULSE_VIOLENCE, tch, ch, 0, 0, &numb, sizeof(numb));
        extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      }
     return FALSE;
    }
    if( item == 73 )
    {
      if( tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch) )
      {
        REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
        obj->value[0] = 0;
        ClearObjEvents(obj);

        act("Without warning, a hidden trap injects a large dose of &+gpoison&n into $n's &+rflesh&n.",
          FALSE, ch, obj, 0, TO_NOTVICT);
        act("Without warning, a hidden trap injects a large dose of &+gpoison&n into your &+rflesh&n.",
          FALSE, ch, obj, 0, TO_CHAR);

        spell_poison(56, ch, 0, 0, ch, NULL);

        extract_obj(obj, TRUE); // Not an arti, but 'in game.'
      }
      return FALSE;
    }
  }
  return FALSE;
}

// Item for learning skills.  Not in game as of 7/4/2015
int skill_beacon(P_obj obj, P_char ch, int cmd, char *argument)
{
  int      skill = obj->value[0];
  int      requirement = obj->value[1];
  int      cap = obj->value[2];
  int      l, t, maxlearn, i;
  bool     active = IS_SET(obj->extra2_flags, ITEM2_MAGIC);
  char     buf[1024];

  static const struct
  {
    int      room;
    int      skills[5];
  } beacon_loads[] =
  {
    { 26860,                    // neg
      { SKILL_DODGE, SKILL_MEDITATE, SKILL_SPELL_KNOWLEDGE_MAGICAL, SKILL_2H_BLUDGEON, SKILL_SHIELD_BLOCK}
    },
    { 25087,                    // brass
      { SKILL_1H_SLASHING, SKILL_UNARMED_DAMAGE, SKILL_SPELL_KNOWLEDGE_CLERICAL, SKILL_PARRY, SKILL_RESCUE}
    },
    { 81094,                    // ceothia
      { SKILL_QUICK_CHANT, SKILL_BASH, SKILL_1H_PIERCING, SKILL_2H_SLASHING, SKILL_HIDE}
    },
    { 25922,                    // baha
      { SKILL_1H_FLAYING, SKILL_TACKLE, SKILL_GAZE, SKILL_DOUBLE_ATTACK, SKILL_SPRINGLEAP}
    },
    { 45718,                    // cel
      { SKILL_SWEEPING_THRUST, SKILL_1H_BLUDGEON, SKILL_BACKSTAB, SKILL_HEADBUTT, SKILL_TRIP}
    },
    { 34804,                    // 4horsemen
      { SKILL_RIPOSTE, SKILL_FLANK, SKILL_RAGE, SKILL_MARTIAL_ARTS, SKILL_ARCANE_RIPOSTE}
    },
    { 0}
  };

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;

  if (cmd == CMD_PERIODIC)
  {
    if (!number(0, 5) && active)
    {
      act("You hear a cracking noise as twisting &+Bthreads of &-Lelectric&-l&+B discharges&n crawl up $p.",
         FALSE, 0, obj, 0, TO_ROOM);
      for (i = 0; beacon_loads[i].room; i++)
      {
        if (world[obj->loc.room].number == beacon_loads[i].room)
        {
          obj->value[0] = beacon_loads[i].skills[number(0, 4)];
          break;
        }
      }
    }
    else if (!number(0, 10) && !active)
    {
      act("$p flashes brightly then blurs and with a loud rumble falls apart leaving nothing but a pile of debris.",
         FALSE, 0, obj, 0, TO_ROOM);
      extract_obj(obj, TRUE); // Not an arti, but 'in game.'
    }
    return FALSE;
  }

  if( !ch || IS_NPC(ch) || (cmd != CMD_TOUCH && cmd != CMD_EXAMINE) || !argument )
    return FALSE;

  one_argument(argument, buf);

  if( obj != get_obj_in_list_vis(ch, buf, world[ch->in_room].contents) )
    return FALSE;

  l = ch->only.pc->skills[skill].learned;
  t = ch->only.pc->skills[skill].taught;
  maxlearn = MAX(SKILL_DATA_ALL(ch, skill).maxlearn[0],
                 SKILL_DATA_ALL(ch, skill).maxlearn[ch->player.spec]);

  if (cmd == CMD_TOUCH)
  {
    if (l < t || (requirement && t < requirement))
    {
      act("As you touch $p you feel the power surge under its surface but you can sense you"
        " are not ready yet to extend your capabilities.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n touches $p but nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
      return TRUE;
    }

    if (t == maxlearn || (cap && cap <= t))
    {
      act("As you touch $p you feel the power surge under its surface but you can sense it is"
        " not enough to extend your capabilities any further.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n touches $p but nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
      return TRUE;
    }

    ch->only.pc->skills[skill].taught = MIN(ch->only.pc->skills[skill].taught + 2, maxlearn);
    sprintf(buf, "As you reach towards $p, suddenly a &+Bcracking bolt&n\n"
            "jumps from it binding you for a second in an immobilizing\n"
            "grip. In a sudden flash of understanding you feel you can\n"
            "now progress further in &+W%s&n!", skills[skill].name);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    act("Upon $n's touch a &+B&-Lcracking bolt&n shoots out from $p "
        "binding $m for a second in an immobilizing grip.", FALSE, ch, obj, 0, TO_ROOM);
    REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);
    // Maybe add a cooldown timer here instead of removing the magic flag?
    return TRUE;
  }
  else if (cmd == CMD_EXAMINE)
  {
    if (IS_TRUSTED(ch))
    {
      sprintf(buf, "This is a skill beacon object. The following values are used to configure it:\n"
              "  &+Wval0&n   skill number\n"
              "  &+Wval1&n   minimal skill level to use the beacon\n"
              "  &+Wval2&n   maximal skill level beacon will grant");
      if (skill)
        sprintf(buf + strlen(buf), "\n$p is %sactive and grants skill &+W%s&n.",
          active ? "" : "in", skills[skill].name);
      if (requirement)
        sprintf(buf + strlen(buf), "\nrequired skill level is &+W%d&n", requirement);
      if (cap)
        sprintf(buf + strlen(buf), "\nit will not raise skill above &+W%d&n", cap);
    }
    else if (GET_C_INT(ch) > number(50, 150))
      sprintf(buf, "$p is a monolithic block of an unidentified metal. There are some runes drawn on"
        " it which you decipher as referring to the art of &+W%s&n.", skills[skill].name);
    else
      sprintf(buf, "$p is a monolithic block of an unidentified metal. There are some runes drawn on"
        " it which you can not decipher at all.");
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return TRUE;
  }

  return FALSE;
}


int vareena_statue(P_obj obj, P_char ch, int cmd, char *argument)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (cmd && !(cmd / 1000))
    return FALSE;

  if (!obj || ch || !OBJ_ROOM(obj))
    return FALSE;

  // see if vareena is in the room
  for (ch = world[obj->loc.room].people; ch; ch = ch->next_in_room)
  {
    if (isname("vareena", GET_NAME(ch)))
      break;
  }
  if (!ch)
    return FALSE;

  // chance of statue acting is VERY slight

  switch (number(0, 250))
  {
    case 0:
      act("&+BA wing feath&+ber from $p &+bgently caress&+Bes your cheek.&n", TRUE, ch ,obj, NULL, TO_CHAR);
      act("&+BA soft wing fea&+bther from $p&n &+bgently caress&+Bes $n&+B's cheek.&n", TRUE, ch, obj, 0, TO_ROOM);
      return TRUE;
    case 100:
      act("&+L$a $q &+bsurrounds you with &+Ba loving embrace.&n", TRUE, ch ,obj, NULL, TO_CHAR);
      act("&+L$a $q &+bsorrounds $n with &+Ba loving embrace.&n", TRUE, ch, obj, 0, TO_ROOM);
      return TRUE;
  }
  return FALSE;
}

int disarm_pick_gloves( P_obj obj, P_char ch, int cmd, char *arg )
{
   struct proc_data *data;
   P_char vict;
   P_obj  weap;

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( !OBJ_WORN_POS(obj, WEAR_HANDS) )
  {
    return FALSE;
  }

  // 1/10 chance.
  if( cmd == CMD_GOTHIT && !number(0,9) )
  {
    if( !(data = (struct proc_data *) arg) )
    {
      return FALSE;
    }
    vict = data->victim;
    if( !IS_ALIVE(vict) )
    {
      return FALSE;
    }

    if( vict->equipment[WIELD] && !IS_SET(vict->equipment[WIELD]->extra_flags, ITEM_NODROP)
      && vict->equipment[WIELD]->type == ITEM_WEAPON )
    {
      weap = unequip_char(vict, WIELD);
    }

    if( weap )
    {
      obj_to_char(weap, vict);
      strip_holy_sword( vict );

      act("You hear a soft tingling laughter as your gloves lash onto", FALSE, ch, obj, vict, TO_CHAR);
      act("$N's arm and start to bite him !! $N yelps in pain and drops his $q", FALSE, ch, weap, vict, TO_CHAR);

      act("OUCH OUCH!! You fumble your weapon as $n's", FALSE, ch, obj, vict, TO_VICT);
      act("$q bite you!", FALSE, ch, obj, vict, TO_VICT);
      set_short_affected_by(vict, SKILL_DISARM, 3 * PULSE_VIOLENCE);
      return TRUE;
    }
  }

 return FALSE;
}


int doom_blade_Proc(P_obj obj, P_char ch, int cmd, char *arg)
{

  int rand, i, dam = cmd / 1000, lvl = 1;
  P_char   vict, tar_ch, next;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  vict = (P_char) arg;
  if( !(obj) || !IS_ALIVE(ch) || !dam || !IS_ALIVE(vict) || ch->in_room != vict->in_room )
  {
    return FALSE;
  }

  lvl = MIN(40,GET_LEVEL(ch));

  // Max 1/9 chance.
  if( number(0, MAX( 8, 56 - GET_LEVEL(ch) )) )
  {
    return FALSE;
  }

  act("$n's $p releases a &+yhazy&n cloud of &+LDeath&n that surrounds $N!&n", FALSE, ch, obj, vict, TO_NOTVICT);
  act("$n's $p releases a &+LDEATH CLOUD&n that surrounds you entirely!", FALSE, ch, obj, vict, TO_VICT);
  act("Your $p releases a &+LDEATH CLOUD&n that surrounds $N!", FALSE, ch, obj, vict, TO_CHAR);

  if( IS_UNDEADRACE(vict) && (GET_SPEC(ch, CLASS_NECROMANCER, SPEC_REAPER)
    || GET_SPEC(ch, CLASS_THEURGIST, SPEC_THAUMATURGE)) )
  {
    spell_undead_to_death(lvl, ch, NULL, 0, vict, obj);
  }
  else
  {
    rand = number(0, 4);
    switch(rand)
    {
      case 0:
        spell_wither(lvl, ch, NULL, 0, vict, obj);
        break;
      case 1:
        spell_blindness(lvl, ch, NULL, 0, vict, obj);
        break;
      case 2:
        spell_disease(lvl, ch, NULL, 0, vict, obj);
        break;
      case 3:
        spell_curse(lvl, ch, NULL, 0, vict, NULL);
        break;
      case 4:
        spell_poison(lvl, ch, NULL, 0, vict, obj);
        break;
      default:
        break;
    }
  }
  return TRUE;
}

int newbie_portal(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_ENTER) {
    find_starting_location(ch, 0);
    GET_BIRTHPLACE(ch) = GET_HOME(ch);
    GET_ORIG_BIRTHPLACE(ch) = GET_HOME(ch);
    obj->value[0] = GET_HOME(ch);
    return FALSE;
  }

  return FALSE;
}

typedef int (*set_func)(P_char ch, P_obj obj, int count, int cmd, char *arg);

struct random_set_wear_off {
  struct affected_type *af;
  char zone_name[256];
};

void event_random_set_proc(P_char ch, P_char victim, P_obj obj, void* data)
{
  struct random_set_wear_off *rdata = (struct random_set_wear_off*)data;
  struct affected_type *afp, *afpp = rdata->af;
  char buffer[256];

  for( afp = ch->affected; afp; afp = afp->next )
  {
    if( afp == afpp )
    {
      sprintf(buffer, "Spirits of %s no longer support you.\n", rdata->zone_name);
      send_to_char(buffer, ch);
      affect_remove(ch, afp);
    }
  }
}

extern struct zone_random_data {
  int zone;
  int races[10];
  int proc_spells[3][2];
} zones_random_data[];

#define SETMSG_NONE     0
#define SETMSG_PROTECT  1
#define SETMSG_STRENGTH 2

void apply_zone_spell(P_char ch, int count, const char *zone_name, P_obj obj, int spell)
{
  int message = SETMSG_NONE;
  char buffer[512];

  switch (spell)
  {
    case SPELL_STONE_SKIN:
      if( !has_skin_spell(ch)
        && obj->timer[0] + get_property("timer.stoneskin.generic", 60) < time(NULL) )
      {
        spell_stone_skin(count * 5, ch, 0, 0, ch, 0);
        obj->timer[0] = time(NULL);
        message = SETMSG_PROTECT;
      }
      break;
    case SPELL_BARKSKIN:
      if( !affected_by_spell(ch, SPELL_BARKSKIN) )
      {
        spell_barkskin(count * 5, ch, 0, 0, ch, 0);
        message = SETMSG_PROTECT;
      }
      break;
    case SPELL_ARMOR:
      if( !affected_by_spell(ch, SPELL_ARMOR) )
      {
        spell_armor(MIN(56, count * 10), ch, 0, 0, ch, 0);
        message = SETMSG_PROTECT;
      }
      break;
    case SPELL_STRENGTH:
    case SPELL_BLESS:
      if( !affected_by_spell(ch, spell) )
      {
        (skills[spell].spell_pointer)(MIN(56, count * 10), ch, 0, 0, ch, 0);
        message = SETMSG_STRENGTH;
      }
      break;
  }

  if( message == SETMSG_PROTECT )
  {
    sprintf(buffer, "The spirits of %s grant you their protection.\n", zone_name);
    send_to_char(buffer, ch);
  }
  else if( message == SETMSG_STRENGTH )
  {
    sprintf(buffer, "The spirits of %s grant you their strength.\n", zone_name);
    send_to_char(buffer, ch);
  }
}

#undef SETMSG_NONE
#undef SETMSG_PROTECT
#undef SETMSG_STRENGTH

// Random zone eq spellups (depends on # items worn == count).
void check_zone_spells(P_char ch, P_obj obj, int count, const char *zone_name)
{
  int zone_room, zone_idx;
  int i;

  // Find the matching zone for the random eq.
  for( i = 0; i <= top_of_zone_table; i++ )
  {
    if( strstr(zone_name, zone_table[i].name) )
      break;
  }
  // If zone not found, return.
  if (i == top_of_zone_table)
  {
    return;
  }

  // Find the appropriate random_data for the zone.
  // This calculates the starting # for the zone as in the DE.
  zone_room = world[zone_table[i].real_bottom].number/100;
  // Walk the list of random eq proc'ing zones.
  for( i = 0; zones_random_data[i].zone; i++ )
  {
    if( zones_random_data[i].zone == zone_room )
    {
      zone_idx = i;
      break;
    }
  }
  // If random_data not found, return.
  if( zones_random_data[i].zone == 0 )
  {
    return;
  }

  // For the three possible spellups,
  for( i = 0; i < 3; i++ )
  {
    // If the required num of eq is met for spell
    if( zones_random_data[zone_idx].proc_spells[i][0] &&
        zones_random_data[zone_idx].proc_spells[i][0] <= count)
    {
      // cast the spell on ch.
      apply_zone_spell(ch, count, zone_name, obj, zones_random_data[zone_idx].proc_spells[i][1]);
    }
  }
}

int random_set(P_char ch, P_obj obj, int count, int cmd, char *arg)
{
  struct affected_type af, *afp;
  char *zone_name, buffer[256];
  struct random_set_wear_off rdata;
  P_nevent e;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( cmd != CMD_PERIODIC || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  // Why do we return true here?
  if( count < 3 )
  {
    return TRUE;
  }

  // Look for a random item proc..
  afp = get_spell_from_char(ch, TAG_SETPROC);
  if( !afp )
  {
    memset(&af, 0, sizeof(af));
    af.type = TAG_SETPROC;
    af.flags = AFFTYPE_NOSAVE | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
    af.location = APPLY_HIT;
    // This should be indefinite: Only changes upon eq removal/wear new eq.
    af.duration = -1;
    afp = affect_to_char(ch, &af);
  }

  zone_name = strstr(obj->short_description, " &+rfrom") + 9;

  // This right here creates the argument between sets of two different zones being on one char.
  disarm_char_nevents(ch, event_random_set_proc);
  rdata.af = afp;
  strcpy(rdata.zone_name, zone_name);
  // Event to remove rdata.af from ch. PULSE_MOBILE + 5 = 35 pulses = 9 sec??
  add_event(event_random_set_proc, PULSE_MOBILE + 5, ch, 0, 0, 0, &rdata, sizeof(rdata));

  check_zone_spells(ch, obj, count, zone_name);

  if( afp->modifier > (count - 2) * 5 )
  {
    sprintf(buffer, "You feel some of the %s's spirits attention leave you.\n", zone_name);
    send_to_char(buffer, ch);
  }
  else if( afp->modifier < (count - 2) * 5 )
  {
    sprintf(buffer, "You feel invigorated as the spirits of %s bless you.\n", zone_name);
    send_to_char(buffer, ch);
  }
  else
  {
    return TRUE;
  }

  afp->modifier = (count - 2) * 5;
  add_event(event_balance_affects, 0, ch, 0, 0, 0, 0, 0);

  return TRUE;
}

struct set_data {
  set_func func;
  int items[MAX_WEAR];
} sets[] = {
  { (set_func)master_set, {22063, 22237, 22621, 45530, 45531, 75857, 82545, 82559}},
  { random_set, {1252, 1254}},
  { (set_func)0, {0} }
};

int set_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_obj tobj, included[MAX_WEAR], cobj = obj;
  int s, i, j, count = 0;
  unsigned int flag = (cmd != CMD_PERIODIC) ? ITEM2_NOPROC : ITEM2_NOTIMER;
  char *c, *d;

  // Look through the sets for the right vnum.
  for( s = 0; sets[s].func; s++ )
  {
    for( i = 0; sets[s].items[i]; i++ )
    {
      // We found a set that obj is in.
      if( sets[s].items[i] == obj_index[obj->R_num].virtual_number )
      {
        break;
      }
    }
    if( sets[s].items[i] )
    {
      break;
    }
  }

  // If we didn't find a set that obj was in
  if( sets[s].items[i] == 0 )
  {
    return FALSE;
  }

  // Set the periodic timer if appropriate.
  if( cmd == CMD_SET_PERIODIC )
  {
    return sets[s].func(ch, obj, count, cmd, arg);
  }

  // Check random_set objs for the "<item> &+rfrom <zone>".
  if( sets[s].func == random_set )
  {
    c = strstr(obj->short_description, " &+rfrom ");
    if( !c )
    {
      // If not, remove the periodic timer.
      disarm_obj_nevents(obj, event_object_proc);
      return FALSE;
    }
  }

  // No proc if not worn.
  if( !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( IS_SET(obj->extra2_flags, flag) )
  {
    REMOVE_BIT(obj->extra2_flags, flag);
    return FALSE;
  }

  if( cmd == CMD_PERIODIC )
  {
    ch = obj->loc.wearing;
  }

  memset(included, 0, sizeof(included));

  // Walk through worn equipment.
  for (i = 0; i < MAX_WEAR; i++)
  {
    tobj = ch->equipment[i];

    // Skip belted & back?  Shouldn't we allow prime belt slot? and skip empty slots.
    //   Allowing prime belt slot and on back.. 8/21/2014
    if( i == WEAR_ATTACH_BELT_3 || i == WEAR_ATTACH_BELT_2 || !tobj )
//      || i == WEAR_ATTACH_BELT_1 || i == WEAR_BACK || !tobj )
    {
      continue;
    }
    // Walk through the current set..
    for( j = 0; sets[s].items[j]; j++ )
    {
      // If the set vnum matches
      if( sets[s].items[j] == obj_index[tobj->R_num].virtual_number )
      {
        break;
      }
    }
    // If we didn't find a match, continue.
    if( !sets[s].items[j] )
    {
      continue;
    }
    // If we have a random item..
    if( sets[s].func == random_set )
    {
      // If !zone or zones don't match.
      d = strstr(tobj->short_description, " &+rfrom ");
      if( !d || strcmp(c, d) != 0 )
      {
        continue;
      }
    }
    else
    {
      // Walk the included list.
      for( j = 0; included[j]; j++ )
      {
        if( included[j]->R_num == tobj->R_num )
        {
          break;
        }
      }
      // Skip to next item if it's a duplicate vnum.
      if( included[j] )
      {
        continue;
      }
    }
    // Set the no proc flag.
    if( tobj != obj )
    {
      SET_BIT(tobj->extra2_flags, flag);
    }

    // Add tobj to the end of the list of included objects.
    included[j] = tobj;
    // Save the object with the newest timer.
    if( cobj->timer[0] < tobj->timer[0] )
    {
      cobj = tobj;
    }
    // Increment the counter of items in set.
    count++;
  }

  // If the set's function returns true..
  if( sets[s].func(ch, cobj, count, cmd, arg) )
  {
    // If we're not periodic..
    if( cmd != CMD_PERIODIC )
    {
      // Remove the no proc flag from all items in set.
      for( i = 0; included[i]; i++ )
      {
        REMOVE_BIT(included[i]->extra2_flags, ITEM2_NOPROC);
      }
    }
    return TRUE;
  }

  return FALSE;
}

int unspec_altar(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd != CMD_PRAY || !ch)
    return FALSE;

  if (!strstr(arg, "altar"))
    return FALSE;

 if(!ch)
 return FALSE;

		/*
        if(GET_SPEC(ch, CLASS_SORCERER, SPEC_WIZARD))
          {
		ch->only.pc->skills[SKILL_SPELL_PENETRATION].taught = 0;
		ch->only.pc->skills[SKILL_SPELL_PENETRATION].learned = 0;
		do_save_silent(ch, 1); // racial skills require a save.
           }*/


  unspecialize(ch, obj);
}

int unmulti_altar(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd != CMD_PRAY || !ch)
    return FALSE;

  if( !strstr(arg, "altar"))
    return FALSE;

  if( !IS_MULTICLASS_PC(ch) )
  {
    send_to_char("&+WThe Gods ignore your prayers!\n", ch);
    return TRUE;
  }

  unmulti(ch, obj);
}

int thought_beacon(P_obj obj, P_char ch, int cmd, char *arg)
{
  if (cmd != CMD_DISPEL || !OBJ_ROOM(obj))
    return FALSE;

  send_to_room("&+LThe fog quickly disperses, leaving only a trace of it ever existing.\n", obj->loc.room);
  extract_obj(obj, FALSE); // Not an arti, but 'in game.'
  return TRUE;
}

	/* Procs made by Sev 2006 */

int bel_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int      dam = cmd / 1000, curr_time;
  struct damage_messages messages = {
    "&+RV&+ra&+Rmp&+ri&+Rr&+ri&+Rc &+renergy &+Linfuses into your body and up your arms, seeping into your &+Cs&+co&+Cu&+cl&+L!&n",
    "&+LYou feel yourself becoming &+Rweaker &+Las your &+Wl&+wi&+Wf&+we&+Wf&+wo&+Wrc&+we &+Lis &+rdrained &+Lout of you!&n",
    "&+rS&+Ra&+rt&+Ra&+rn&+Ri&+rc &+Renergy &+Linfuses into $n's &+Lhand and up $s arm, greatly strengthening $m.&n",
    "", "", "", 0, obj};

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) )
  {
    if( OBJ_WORN(obj) )
    {
      ch = obj->loc.wearing;
    }
    else
    {
      return FALSE;
    }
  }
  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_REMOVE) )
  {
    if( isname(arg, obj->name) || isname(arg, "all") )
    {
      if( affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD) )
      {
        affect_from_char(ch, SPELL_ILIENZES_FLAME_SWORD);
      }
    }
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "eld") )
    {
      curr_time = time(NULL);
      // 4 min timer.
      if( obj->timer[0] + 240 <= curr_time )
      {
        act("You say 'eld'", FALSE, ch, 0, 0, TO_CHAR);
        act("&+LYour $q &+rg&+Rl&+Yo&+Rw&+rs &+Lbriefly.&n", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'eld'", TRUE, ch, obj, NULL, TO_ROOM);
        act("&+LThe $q &+Lcarried by $n &+rg&+Rl&+Yo&+Rw&+rs&n &+Lbriefly.&n", TRUE, ch, obj, NULL, TO_ROOM);
        act("&+rS&+Ra&+rt&+Ra&+rn&+Ri&+rc &+Rf&+rl&+Ra&+rm&+Re&+rs &+Lengulf $n's &+Lblade as it comes alive with the power of &+rDa&+Rrag&+ror&+L!&n", TRUE, ch, obj, NULL, TO_ROOM);
        spell_ilienzes_flame_sword(51, ch, 0, SPELL_TYPE_SPELL, ch, 0);

        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
  }

  if( !dam )
  {
    return FALSE;
  }

  vict = (P_char) arg;
  if( !IS_ALIVE(vict) || (GET_RACE(vict) != RACE_DEMON && GET_RACE(vict) != RACE_DEVIL) )
  {
    return FALSE;
  }
  // 1/20 chance.
  if( number(0, 19) )
  {
    return FALSE;
  }

  dam = MIN((GET_HIT(vict) + 9), 100);
  act("&+LYour $q &+binfuses &+Lwith &+rs&+Ra&+rt&+Ra&+rn&+Ri&+rc &+Renergy &+Las it slashes into $N!&n", FALSE, ch, obj, vict, TO_CHAR);
  act("$n's $q &+bglows &+rcrimson &+Las it slashes into $N!&n", FALSE, ch, obj, vict, TO_NOTVICT);
  act("$n's $q &+bglows &+rcrimson &+Las it slices into you!&n", FALSE, ch, obj, vict, TO_VICT);
  spell_damage(ch, vict, 400, SPLDAM_NEGATIVE, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | RAWDAM_NOKILL, &messages);

  vamp(ch, dam / 2, (int) (GET_MAX_HIT(ch) * 1.3));

  return TRUE;
}

int zarthos_vampire_slayer(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict;
  int      dam = cmd / 1000;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) )
  {
    if( OBJ_WORN(obj) && IS_ALIVE(obj->loc.wearing) )
    {
      ch = obj->loc.wearing;
    }
    else
    {
      return FALSE;
    }
  }

  if( !dam )
  {
    return FALSE;
  }
  vict = (P_char) arg;
  if( !IS_ALIVE(vict) || !(IS_UNDEAD(vict) || IS_AFFECTED(vict, AFF_WRAITHFORM)) )
  {
    return FALSE;
  }
  // 1/25 chance.
  if( number(0, 24) )
  {
    return FALSE;
  }

  act("&+LYour $q &+Lradiates &+Wdivine &+Lpower as it slashes into $N!&n", FALSE, ch, obj, vict, TO_CHAR);
  act("$n's $q &+Lemanates &+Ws&+wp&+We&+wc&+Wt&+wr&+Wa&+wl &+Llight as it slashes into $N!&n", FALSE, ch, obj, vict, TO_NOTVICT);
  act("$n's $q &+Lradiates with &+Wdivine &+Lpower as it slices into you!&n", FALSE, ch, obj, vict, TO_VICT);
	spell_destroy_undead(39, ch, 0, SPELL_TYPE_SPELL, vict, 0);
  act("&+LThe $q &+Lbathes you in &+Chealing &+Llight.&n", TRUE, ch, obj, vict, TO_CHAR);
  spell_cure_serious(39, ch, 0, SPELL_TYPE_SPELL, ch, 0);

	return TRUE;
}

int critical_attack_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   victim;
  int      dam = cmd / 1000;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) )
  {
    if( OBJ_WORN(obj) && IS_ALIVE(obj->loc.wearing) )
    {
      ch = obj->loc.wearing;
    }
    else
    {
      return FALSE;
    }
  }

  if( !dam )
  {
    return FALSE;
  }

  victim = (P_char) arg;
  // 1/30 chance.
  if( !IS_ALIVE(victim) || IS_DRAGON(victim) || number(0, 29) )
  {
    return FALSE;
  }

  switch (number(0, 2))
  {
  case 0:
    act("&nYou slam the &+yhandle&n of your $q in $N's face!&n", FALSE, ch, obj, victim, TO_CHAR);
    act("$n makes a quick move and slams the &+yhandle&n of $s $q in your face!&n", FALSE, ch, obj, victim, TO_VICT);
    act("$n makes a quick move and slams the &+yhandle&n of $s $q in $N's face!&n", FALSE, ch, obj, victim, TO_NOTVICT);
    blind(ch, victim, 2 * PULSE_VIOLENCE);
    break;
  case 1:
    act("&nYou point your&n $q at&n $N and utter a word of &+rc&+Ro&+rm&+Rm&+ra&+Rn&+rd&n!", FALSE, ch, obj, victim, TO_CHAR);
    act("$n points $s $q at &+L_YOU_&n and utters a word of &+rc&+Ro&+rm&+Rm&+ra&+Rn&+rd&n!", FALSE, ch, obj, victim, TO_VICT);
    act("$n points $s $q at $N and utters a word of &+rc&+Ro&+rm&+Rm&+ra&+Rn&+rd&n!", FALSE, ch, obj, victim, TO_NOTVICT);
    Stun(victim, ch, PULSE_VIOLENCE, FALSE);
    break;
  case 2:
    act("&nYou &+rhook&n your $q around $N's leg and pull $M to the ground!&n", FALSE, ch, obj, victim, TO_CHAR);
    act("$n suddenly &+rhooks&n $s $q around your leg and pulls you to the ground!&n", FALSE, ch, obj, victim, TO_VICT);
    act("$n suddenly &+rhooks&n $s $q around&n $N's leg and pulls $M to the ground!&n", FALSE, ch, obj, victim, TO_NOTVICT);
    SET_POS(victim, POS_SITTING + GET_STAT(victim));
    break;
	}
  return TRUE;
}

//-------------------------------------------------
// OK. various portals stuff here (HOOK ACTIONS)
//-------------------------------------------------

int portal_race(P_char ch)
{
  if (IS_NPC(ch))
  {
    if (!ch->following)
      return -2;
    if (IS_ILLITHID(ch->following))
      return 0;
    if (EVIL_RACE(ch->following))
      return -1;
    return 1;
  }
  if (IS_ILLITHID(ch))
    return 0;
  if (EVIL_RACE(ch))
    return -1;
  return 1;
}

// common portals hook used for some portals/gates
// when we lazy enough to write new messages and dont need special checks
int portal_door(P_obj obj, P_char ch, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
   if (cmd == CMD_SET_PERIODIC) return FALSE;

   // what commands invokes portal actions
   if((cmd == CMD_DECAY) ||
      (cmd == CMD_DISPEL) ||
      (((cmd == CMD_ENTER) ||
      (cmd == CMD_LOOK)) && ch))
   {
   /*
    if(ch && !is_Raidable(ch, 0, 0))
    {
      send_to_char("&=LWYou are not raidable! You shall not pass!\r\n", ch);
      return false;
    }
	*/



      struct portal_action_messages msg = {
/*in ch    */ "You enter $p and reappear elsewhere...",
/*in ch r  */ "&+W$p suddenly glows brightly!\n"
    	      "$n enters $p and disappears among the mist.",
/*out ch   */ 0,
/*out ch r */ "$n steps out of $p.",
/*wait init*/ "It hasn't solidified yet...\n",
/*wait stb */ "It hasn't solidified again yet...\n",
/*decay ch */ "$p dissolves in a swirl of colors and is gone.",
/*decay r  */ "$p dissolves in a swirl of colors and is gone.",
/*bug      */ "Bug with portal!!! Tell a god.\n"
      };

      return portal_general_internal(obj, ch, cmd, arg, &msg);
   }

   return FALSE;
}

int portal_wormhole(P_obj obj, P_char ch, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
   if (cmd == CMD_SET_PERIODIC) return FALSE;

   if((cmd == CMD_DECAY) ||
      (cmd == CMD_DISPEL) ||
      (((cmd == CMD_ENTER) ||
      (cmd == CMD_LOOK)) && ch))
   {
   
    if(ch && !is_Raidable(ch, 0, 0))
    {
      send_to_char("&=LWYou are not raidable! You shall not pass!\r\n", ch);
      return false;
    }
    
      struct portal_action_messages msg = {
/*in ch    */ "&+LAs you enter $p&+L, you feel yourself being torn into a thousand pieces,\n"
    		  "&+Lscattered over the entirety of reality.  Bits of your shattered\n"
    		  "&+Lconsciousness float randomly about the universe with no overall\n"
    		  "&+Ldirection or purpose.  Suddenly, you find yourself elsewhere..",
/*in ch r  */ "$n steps into $p and disappears among the darkness.",
/*out ch   */ 0,
/*out ch r */ "$n stumbles out of $p.",
/*wait init*/ "The rift isn't quite stable enough yet...\n",
/*wait stb */ "The rift isn't quite stable enough again yet...\n",
/*decay ch */ 0,
/*decay r  */ 0,
/*bug      */ "Bug with wormhole!!! Tell a god.\n"
      };

/*  if (GET_RACE(ch) != RACE_ILLITHID) {
     send_to_char("&+WYour mind is too puny to survive the trip.\n", ch);
     return TRUE;
    }
    if (GET_LEVEL(ch) < 41) {
     send_to_char("&+WYour mind is not strong enough to survive the trip.\n", ch);
     return TRUE;
    }
*/

      return portal_general_internal(obj, ch, cmd, arg, &msg);
   }

   return FALSE;
}

int portal_etherportal(P_obj obj, P_char ch, int cmd, char *arg)
{
  /*
     check for periodic event calls
   */
   if (cmd == CMD_SET_PERIODIC) return FALSE;

   // what commands invokes portal actions
   if((cmd == CMD_DECAY) ||
      (cmd == CMD_DISPEL) ||
      (((cmd == CMD_ENTER) ||
      (cmd == CMD_LOOK)) && ch))
   {
    if(ch && !is_Raidable(ch, 0, 0))
    {
      send_to_char("&=LWYou are not raidable! You shall not pass!\r\n", ch);
      return false;
    }
  
      struct portal_action_messages msg = {
/*in ch    */ "&+YAs you enter $p&+Y, you feel yourself being torn into a thousand\n"
    	      "&+Ypieces, scattered over the entirety of reality. Bits of your shattered\n"
    	      "&+Yconsciousness float randomly about the universe with no overall\n"
    	      "&+Ydirection or purpose.  Suddenly, you find yourself elsewhere..",
/*in ch r  */ "$n steps into $p and disappears among the light.",
/*out ch   */ 0,
/*out ch r */ "$n steps out of $p.",
/*wait init*/ "The portal hasn't stabilized yet...\n",
/*wait stb */ "The portal hasn't re-stabilized yet...\n",
/*decay ch */ 0,
/*decay r  */ 0,
/*bug      */ "Bug with etherprotal!! Tell a god.\n"
      };

      return portal_general_internal(obj, ch, cmd, arg, &msg);
   }

   return FALSE;
}

//---------------------------------------------------------
// general portal actions: dispel,look in, enter
// (msg comes from portal hooks)
//---------------------------------------------------------
int portal_general_internal( P_obj obj, P_char ch, int cmd, char *arg, struct portal_action_messages *msg )
{
   int      bits;
   int      to_room;
   P_char   dummy;
   P_obj    obj2 = NULL;

   if (cmd == CMD_DISPEL)
   {
      dispel_portal(ch, obj);
      return TRUE;
   }

   if (cmd == CMD_DECAY)
   {
     // if some decay message is not set, then we will use generic obj decay
     if( !msg->decay_to_room || !msg->decay_to_char )
       return FALSE;

     if (world[obj->loc.room].people)
     {
       act(msg->decay_to_room, FALSE, world[obj->loc.room].people, obj, 0, TO_ROOM);
       act(msg->decay_to_char, FALSE, world[obj->loc.room].people, obj, 0, TO_CHAR);
     }
     return TRUE;
   }

   // parse thru "in"
   if (cmd == CMD_LOOK)
   {
     while (*arg == ' ')
       arg++;
     if (!*arg)
       return FALSE;
     if (strn_cmp(arg, "in ", 3))
       return FALSE;
     arg += 3;
   }

  // Get the portal object, since there may be more than one (skipping tracks)
  bits = generic_find(arg, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &dummy, &obj2);

  // If the object is not the one we seek then return false
  if (obj2 != obj)
    return FALSE;


  to_room = real_room(obj->value[0]);
  if (to_room == NOWHERE)
  {
    send_to_char(msg->bug_to_char, ch);
    return (TRUE);
  }
  if (cmd == CMD_LOOK)
  {
    act("You peer into $p and see...", 0, ch, obj, 0, TO_CHAR);
    if (0)
      send_to_char("It is pitch black over there...\n", ch);
    else
    {
      send_to_char(world[to_room].name, ch);
      send_to_char("\n", ch);
    }
    return TRUE;
  }

  // somehow other command passed, NO WAY! squash it! (why? because only enter processing follows)
  if (cmd != CMD_ENTER) return FALSE;

  /* otherwise cmd == enter */

  //--------------------------------
  // check timers
  // 1. initial stabilization
  if( (obj->value[4] > 0) && (time(0) - obj->timer[0]) < obj->value[4])
  {
    send_to_char(msg->wait_init_to_char, ch);
    return TRUE;
  }

  // 2. post enter stabilization
  if( obj->timer[1] > 0 && obj->value[5] > 0 &&
      (time(0) - obj->timer[1]) < obj->value[5] )
  {
    send_to_char(msg->wait_to_char, ch);
    return TRUE;
  }
  //--------------------------------

  if (!can_enter_room(ch, to_room, FALSE) ||
      ((obj->value[1] == RACE_ILLITHID) && (!IS_ILLITHID(ch))) ||
      (IS_SET(world[ch->in_room].room_flags, ARENA) !=
       IS_SET(world[to_room].room_flags, ARENA)))
  {
    send_to_char("A strong force pushes you back!\n", ch);
    return TRUE;
  }

  /*
  act("&+W$p suddenly glows brightly!", FALSE, ch, obj, 0, TO_ROOM);
*/

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  act(msg->step_in_to_room, FALSE, ch, obj, 0, TO_ROOM);
  char_from_room(ch);
  act(msg->step_in_to_char, FALSE, ch, obj, 0, TO_CHAR);

  /* Probability of in transit instability - SKB 13 Feb 1998 */
/*  if (!number(0, 99))
        to_room = real_room(number(99900,99999));*/

  char_to_room(ch, to_room, -1);
  act(msg->step_out_to_room, FALSE, ch, obj, 0, TO_ROOM);

  //------------------------
  // reset enter stabilization as someone entered
  obj->timer[1] = time(0);
  //------------------------

  // add lag after step out from portal
  if( obj->value[6] > 0 )
	 // value is set in seconds, convert into pulses
     CharWait(ch, obj->value[6] * WAIT_SEC);

#if 0
  if (IS_SET(world[to_room].room_flags, DEATH) && !IS_TRUSTED(ch))
  {
    death_cry(ch);
    extract_char(ch);
  }
#endif

  /* if obj->value[2] > 0, don't drop below 1, or else portal won't disappear properly */
  // decay when limit enters
  if (obj->value[2] > 0)
  {
    obj->value[2] = BOUNDED(1, obj->value[2], 9999);
    obj->value[2]--;
    if (obj->value[2] == 0)
      Decay(obj); // ??? dont decay another portal side?
  }

  return TRUE;
}

void event_mentality_mace_vibrate(P_char ch, P_char victim, P_obj obj, void *data)
{
  if( OBJ_WORN(obj) && obj->loc.wearing )
  {
    act("$p vibrates softly.", FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
  }
}

int mentality_mace(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time;
  struct follow_type *k, *p;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( !IS_ALIVE(ch) || !OBJ_WORN(obj) || !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_REMOVE) )
  {
    if( isname(arg, obj->name) || isname(arg, "all") )
    {
      for( k = ch->followers; k; k = p )
      {
        P_char tch = k->follower;
        p = k->next;
        if( tch && IS_NPC(tch) && GET_VNUM(tch) == 250 )
        {
          stop_fighting(tch);
          if( IS_DESTROYING(tch) )
          {
            stop_destroying(tch);
          }
          StopAllAttackers(tch);
          extract_char(tch);
        }
      }
    }
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if( isname(arg, "mentality") )
    {
      curr_time = time(NULL);
      // 5 min timer.
      if( IS_TRUSTED(ch) || (obj->timer[0] + 300 <= curr_time) )
      {
        act("$p hums loudly, and shatters your psyche!", FALSE, ch, obj, 0, TO_CHAR);
        act("$p hums loudly, and shatters $n's psyche!", FALSE, ch, obj, 0, TO_ROOM);

        spell_reflection(50, ch, "", 0, ch, 0);

        obj->timer[0] = curr_time;

        disarm_obj_nevents(obj, event_mentality_mace_vibrate);
        add_event(event_mentality_mace_vibrate, 300 * WAIT_SEC, 0, 0, obj, 0, 0, 0);

        return TRUE;
      }
      else
      {
        act("$p hums loudly, giving you a splitting headache!", FALSE, ch, obj, 0, TO_CHAR);
        act("$p hums loudly, and $n looks pained for a second.", FALSE, ch, obj, 0, TO_ROOM);

        spell_damage(ch, ch, 100, SPLDAM_GENERIC, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | RAWDAM_NOKILL, 0);
        return TRUE;
      }
    }
  }

  return FALSE;
}

int khaziddea_blade(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char victim;
  bool   fired;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  // 1/50 chance.
  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !(victim = (P_char) arg) || number(0, 49) )
  {
    return FALSE;
  }
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  fired = FALSE;
  if( affected_by_spell(victim, SPELL_STONE_SKIN) )
  {
    affect_from_char(victim, SPELL_STONE_SKIN);
    fired = TRUE;
  }
  else if( affected_by_spell(victim, SPELL_BIOFEEDBACK) )
  {
    affect_from_char(victim, SPELL_BIOFEEDBACK);
    fired = TRUE;
  }
  else if( affected_by_spell(victim, SPELL_SHADOW_SHIELD) )
  {
    affect_from_char(victim, SPELL_SHADOW_SHIELD);
    fired = TRUE;
  }

  if( fired )
  {
    act("$q draws $n's hand down swiftly and strikes through $N's defenses!", FALSE, ch, obj, victim, TO_ROOM);
    act("$q draws $n's hand down swiftly and strikes through your defenses!", FALSE, ch, obj, victim, TO_VICT);
    act("$q draws your hand down swiftly and strikes through $N's defenses!", FALSE, ch, obj, victim, TO_CHAR);
    return TRUE;
  }

  return FALSE;
}

int resurrect_room(P_char ch)
{
  P_obj obj, t_obj;
  int i = 0;

  // store a list of corpses ressed, only res once per char
  list<int> already_ressed;

  for (obj = world[ch->in_room].contents; obj; obj = t_obj)
  {
    t_obj = obj->next_content;

    if( obj->type != ITEM_CORPSE ||
        !IS_SET(obj->value[1], PC_CORPSE) )
      continue;

    P_char t_ch = find_player_by_pid(obj->value[3]);

    if( !t_ch )
      continue;

    if( !IS_TRUSTED(ch) )
    {

      if( !is_linked_to(ch, t_ch, LNK_CONSENT) )
        continue;

      // if already ressed in this room, don't res again
      bool skip = false;
      for( list<int>::iterator it = already_ressed.begin(); it != already_ressed.end(); it++ )
      {
        if( obj->value[3] == *it )
        {
          skip = true;
          break;
        }
      }

      if( skip )
        continue;

    }

    spell_resurrect(56, ch, 0, 0, 0, obj);
    already_ressed.push_back( obj->value[3] );

    i++;
  }

  return (i > 0);
}

int resurrect_totem(P_obj obj, P_char ch, int cmd, char *arg)
{

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !OBJ_WORN(obj) )
  {
    return FALSE;
  }

  if( cmd == CMD_PERIODIC )
  {
    if( !number(0,16) )
    {
      ch = obj->loc.wearing;

      if( !IS_ALIVE(ch) )
      {
        return FALSE;
      }

      act("Your $p &+Wshimmers&n slightly.", FALSE, ch, obj, 0, TO_CHAR);
      act("$p &ncarried by $n &+Wshimmers&n slightly.", FALSE, ch, obj, 0, TO_ROOM);
      spell_group_heal(50, ch, 0, 0, ch, 0);

      return TRUE;
    }
  }

  if( !OBJ_WORN_BY(obj, ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_SAY && arg )
  {
    if( isname(arg, "ilienze") )
    {
      int curr_time = time(NULL);
      // 1 rl hour timer.
      if (IS_TRUSTED(ch) || (obj->timer[0] + 3600 <= curr_time))
      {
        act("res room proc", FALSE, ch, 0, 0, TO_CHAR);

        if( resurrect_room(ch) )
        {
          obj->timer[0] = curr_time;
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

int harpy_gate(P_obj obj, P_char ch, int cmd, char *arg)
{
  if( !obj || !ch )
    return FALSE;

  if( IS_NPC(ch) || IS_TRUSTED(ch) )
    return FALSE;

  if( cmd == CMD_SOUTH )
  {
    if (GET_RACEWAR(ch) == RACEWAR_NEUTRAL)
    {
      send_to_char("&+yYou must choose your life path before leaving the settlement!\n", ch);
      return TRUE;
    }
  }

  return FALSE;
}

int blue_sword_armor(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000, curr_time;
  P_char   vict;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( (cmd == CMD_REMOVE) && arg )
  {
    if( isname(arg, obj->name) || isname(arg, "all") )
    {
      if (affected_by_spell(ch, SPELL_ARMOR))
      {
        affect_from_char(ch, SPELL_ARMOR);
        send_to_char("You feel less &+Wprotected&n.\r\n", ch);
      }
    }
  }

  if( !OBJ_WORN_POS(obj, WIELD) )
  {
    return FALSE;
  }

  if( arg && (cmd == CMD_SAY) )
  {
    if (isname(arg, "armor"))
    {
      curr_time = time(NULL);

      if (curr_time >= obj->timer[0] + 60)
      {
        act("You say 'armor'", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'armor'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_armor(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
  }
  return FALSE;
}

int jet_black_maul(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam = cmd / 1000, curr_time;
  P_char   vict;

  /*
     check for periodic event calls
   */
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
    return FALSE;

  if( (cmd == CMD_REMOVE) && arg )
  {
    if( isname(arg, obj->name) || isname(arg, "all") )
    {
      if (affected_by_spell(ch, SPELL_STRENGTH))
      {
        affect_from_char(ch, SPELL_STRENGTH);
        send_to_char("You feel less &+Cstrong&n.\r\n", ch);
      }
      if (affected_by_spell(ch, SPELL_ENHANCED_STR))
      {
        affect_from_char(ch, SPELL_ENHANCED_STR);
        send_to_char("&+CYour muscles return to normal.&n\r\n", ch);
      }
    }
  }

  if( !OBJ_WORN_POS(obj, WIELD) )
  {
    return FALSE;
  }

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "titan"))
    {
      curr_time = time(NULL);

      if (curr_time >= obj->timer[0] + 60)
      {
        act("You say 'titan'", FALSE, ch, obj, 0, TO_CHAR);
        act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

        act("$n says 'titan'", TRUE, ch, obj, NULL, TO_ROOM);
        act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
        spell_enhanced_strength(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        spell_strength(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = curr_time;

        return TRUE;
      }
    }
  }
  return FALSE;
}

#define TOE_SWITCH_ROOM 43697

int toe_chamber_switch(P_obj obj, P_char ch, int cmd, char *arg)
{
  int correct_button, button_guess = 0;
  P_char summoned, target;
  P_obj s_obj, t_obj;
  int mobiles[] = {43540, 43539, 43538, 43550, 0};

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) || !obj )
  {
    return FALSE;
  }

  for( s_obj = world[real_room0(TOE_SWITCH_ROOM)].contents; s_obj; s_obj = t_obj )
  {
    t_obj = s_obj->next_content;

    if( isname(s_obj->name, "limestone") )
    {
      correct_button = 1;
    }
    if( isname(s_obj->name, "obsidian") )
    {
      correct_button = 2;
    }
    if( isname(s_obj->name, "granite") )
    {
      correct_button = 3;
    }
    if( isname(s_obj->name, "adamantite") )
    {
      correct_button = 4;
    }
    if( isname(s_obj->name, "jade") )
    {
      correct_button = 5;
    }
  }

  if( arg && (cmd == CMD_PUSH) )
  {
    if( isname(arg, "limestone") )
    {
      button_guess = 1;
    }
    if( isname(arg, "obsidian") )
    {
      button_guess = 2;
    }
    if( isname(arg, "granite") )
    {
      button_guess = 3;
    }
    if( isname(arg, "adamantite") )
    {
      button_guess = 4;
    }
    if( isname(arg, "jade") )
    {
      button_guess = 5;
    }

    if( correct_button == button_guess )
    {
      REMOVE_BIT(world[ch->in_room].dir_option[0]->exit_info, EX_BLOCKED);
      act("&+yA &+Lgrinding &+ysound of stone on stone reverberates about the chamber,\r\n"
        "&+ybefore ending abruptly in an audible &+Lclick&+y.&n\r\n", TRUE, ch, 0, 0, TO_CHAR);
      act("&+yA &+Lgrinding &+ysound of stone on stone reverberates about the chamber,\r\n" \
        "&+ybefore ending abruptly in an audible &+Lclick&+y.&n\r\n", TRUE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    else if( button_guess <= 5 && button_guess >= 1 )
    {
      int i = number(0, 3);
      summoned = read_mobile(mobiles[i], VIRTUAL);

      if( !summoned )
      {
        logit(LOG_EXIT, "assert: error in toe_chamber_switch() proc");
        raise(SIGSEGV);
      }
      act("&+yThere is a rumbling sound of movement from overhead.  A hidden trapdoor swings\r\n"
        "&+yopen suddenly, releasing $N &+yback into the temple!&n\n"
        "&+yJust as quickly, the trapdoor slams shut again, stirring up a &+Lcloud&+y of dust.&n\r\n", FALSE, ch, 0, summoned, TO_CHAR);
      act("&+yThere is a rumbling sound of movement from overhead.  A hidden trapdoor swings\r\n"
        "&+yopen suddenly, releasing $N &+yback into the temple!&n\n"
        "&+yJust as quickly, the trapdoor slams shut again, stirring up a &+Lcloud&+y of dust.&n\r\n", FALSE, ch, 0, summoned, TO_ROOM);
      char_to_room(summoned, ch->in_room, 0);
      return TRUE;
    }
  }
  return FALSE;
}

void soul_taking_check(P_char ch, P_char tch)
{
  P_obj stiletto;
  //read_object(SOUL_TAKING_STILETTO, VIRTUAL);

  if(GET_CLASS(ch, CLASS_ROGUE))
  {
    if ((stiletto = ch->equipment[WIELD]) && obj_index[stiletto->R_num].virtual_number == SOUL_TAKING_STILETTO)
    {
      if (vamp(ch, 1.5 * GET_LEVEL(tch), 1.2 * GET_MAX_HIT(ch))) 
      {
      act("&+LYour stiletto &+Wglows &+Lwith power as it &+wdevours &+Lanother &+Wsoul!&n", FALSE, ch, 0, 0, TO_CHAR);
      act("&+L$p &+Wglows brightly in $n&+L's hands as it &+wdevours &+Lanother &+Wsoul!&n", FALSE, ch, stiletto, 0, TO_ROOM);
      }
    }
  }
}

int righteous_blade(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char victim;

  if( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }

  if( cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !(victim = (P_char) arg) )
  {
    return FALSE;
  }
  if( !IS_ALIVE(victim) )
  {
    return FALSE;
  }

  // 1/30 chance.
  if( !number(0, 29) )
  {
    if(!affected_by_spell(ch, SPELL_VIRTUE))
    {
	    if( (IS_EVIL(ch) && !IS_EVIL(victim)) || (IS_GOOD(ch) && !IS_GOOD(victim)) )
	    {
        act("&+wA &+Wbright &+Wpu&+wl&+Ws&+wa&+Wt&+wi&+Wng &n&+Cg&+Wlo&+Cw&n&+w surrounds $q&+w, and after short while it spreads over whole $n's &+wbody!", FALSE, ch, obj, victim, TO_ROOM);
        act("&+wA &+Wbright &+Cg&+Wlo&+Ww&n&+w surrounds your $q&+w, and then it spreads over all of you!", FALSE, ch, obj, victim, TO_CHAR);
		    spell_virtue(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        return FALSE;
	    }
    }
    // 1/30 * 1/4 == 1/120 chance.
    if( number(0, 3) )
	  {
      act("$n&+W's weapon briefly glows and vibrates upon striking $N&+W...", FALSE, ch, obj, victim, TO_ROOM);
      act("$n&+W's weapon briefly glows and vibrates upon striking you...", FALSE, ch, obj, victim, TO_VICT);
      act("&+WYour $q&+W briefly glows and vibrates as it strikes $N&+W...", FALSE, ch, obj, victim, TO_CHAR);
	    spell_life_bolt(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
      return FALSE;
    }
	  else
	  {
      act("$n&+W's weapon glows &+Rred&n&+W and vibrates upon striking $N&+W...", FALSE, ch, obj, victim, TO_ROOM);
      act("$n&+W's weapon glows &+Rred&n&+W and vibrates upon striking you&+W...", FALSE, ch, obj, victim, TO_VICT);
      act("&+WYour $q&+W glows &+Rred&n&+W and vibrates as it strikes $N&+W...", FALSE, ch, obj, victim, TO_CHAR);

	  	if( IS_UNDEAD(victim) )
      {
	  	  spell_destroy_undead(50, ch, 0, SPELL_TYPE_SPELL, victim, 0);
      }
	  	else
      {
        // dispel_lifeforce does no damage.
	      for( int i = 1 + GET_LEVEL(ch)/17; i && !affected_by_spell(victim, SPELL_DISPEL_LIFEFORCE); i-- )
	      {
	        spell_dispel_lifeforce(50, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	      }
      }
	  }
  }

  return FALSE;
}

int flame_blade(P_obj obj, P_char ch, int cmd, char *argument)
{
  if (!ch || !obj)
    return FALSE;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  P_char tch;

  if (OBJ_WORN(obj))
    tch = obj->loc.wearing;
  else if (OBJ_CARRIED(obj))
    tch = obj->loc.carrying;
  else
    return FALSE;

  if (!tch || IS_NPC(tch))
    return FALSE;

  if (GET_PID(tch) != obj->timer[1])
  {
    //Lamify it!
    obj->value[5] = 0;
    obj->value[6] = 0;
    obj->value[7] = 0;
    obj->bitvector = 0;
    obj->bitvector2 = 0;
    
    //Lets go ahead and kill the timer
    obj->timer[0] = 1;
    return FALSE;
  }
  
  return FALSE;
}

int miners_helmet(P_obj obj, P_char ch, int cmd, char *argument)
{
   char *arg;
   int curr_time;

   if (cmd == CMD_SET_PERIODIC)
      return FALSE;

   if (!ch || !obj) 
      return FALSE;

   if (!OBJ_WORN(obj)) 
      return FALSE;

   if (argument && (cmd == CMD_SAY))
   {
      arg = argument;

      while (*arg == ' ')
         arg++;

      if (!strcmp(arg, "paydirt"))
      {

         curr_time = time(NULL);

         // 10 minute timer = 600 sec.
         if (obj->timer[0] + 600 <= curr_time)
         {
             obj->timer[0] = curr_time;
             spell_lodestone_vision( 60, ch, arg, SPELL_LODESTONE, ch, obj );
         }

         return TRUE;
      }
   }
   return FALSE;
}

int thanksgiving_wings(P_obj obj, P_char ch, int cmd, char *argument)
{
  char    *arg;
  int      curr_time;
  P_char   temp_ch;

  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !obj || !OBJ_WORN(obj) || cmd != CMD_PERIODIC )
  {
    return FALSE;
  }

  if( !(temp_ch = ch) )
  {
    if( obj->loc.wearing )
    {
      temp_ch = obj->loc.wearing;
    }
    else
    {
      return FALSE;
    }
  }

  curr_time = time(NULL);

  if( obj->timer[0] + (int) 200 <= curr_time )
  {
    act("&nYou suddenly feel the urge for &+yTURKEY! &nand take a nice &+rbite &nout of your $p!", FALSE, temp_ch, obj, 0, TO_CHAR);
    act("$n suddenly looks &+Rravenous!&n and takes a huge &+rbite &nout of their $p&n!", FALSE, temp_ch, obj, 0, TO_ROOM);
    spell_invigorate(30, temp_ch, 0, SPELL_TYPE_POTION, temp_ch, 0);
    obj->timer[0] = curr_time;
  }
  return FALSE;
}

int moonstone(P_obj obj, P_char ch, int cmd, char *argument)
{
  char *name;
  struct obj_affect *aff;
  P_nevent e;

  // If not moonstone or bloodstone.
  if( !obj || (obj_index[obj->R_num].virtual_number != 419
    && obj_index[obj->R_num].virtual_number != 433) )
  {
    logit(LOG_DEBUG, "moonstone: obj proc called with no obj or non-moonstone obj: '%s' %d.",
      !obj ? "Null" : obj->short_description, !obj ? -1 : GET_OBJ_VNUM(obj) );
  }

  if (cmd == CMD_SET_PERIODIC)
  {
    return FALSE;
  }

  if( cmd == CMD_DECAY )
  {
    // Notify the caster then let it decay.
    name = get_player_name_from_pid( obj->value[0] );
    if( name )
    {
      ch = get_char_online( name );
      if( ch )
      {
        if( obj_index[obj->R_num].virtual_number == 419 )
        {
          send_to_char( "&+WYour moonstone fades to nothingness.&n\n", ch );
          affect_from_char(ch, SPELL_MOONSTONE);
        }
        else
        {
          send_to_char( "&+YYour &+rblood&+ystone&+Y fades to nothingness.&n\n", ch );
          affect_from_char(ch, SPELL_BLOODSTONE);
        }
      }
    }
    return FALSE;
  }

  // Upon dispel, make object decay soon and let it be dispelled.
  if( cmd == CMD_DISPEL )
  {
    // Get obj affect.
    aff = get_obj_affect( obj, TAG_OBJ_DECAY );
    if( aff )
    {
      // Find decay event.
      LOOP_EVENTS_OBJ( e, obj->nevents )
      {
        if( e->func != event_obj_affect )
        {
          continue;
        }
        // If event found, set it's timer to 1 min.
        if( *((struct obj_affect **)e->data) == aff )
        {
           e->timer = 1;
        }
      }
    }
    else
    {
      // Set timer to 1 min
      logit(LOG_DEBUG, "moonstone: obj has no decay timer! (creating one)");
      set_obj_affected( obj, 1, TAG_OBJ_DECAY, 0);
    }
    return FALSE;
  }

  return FALSE;
}
