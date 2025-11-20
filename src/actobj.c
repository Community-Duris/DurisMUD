/*
 * ***************************************************************************
 *  File: actobj.c                                           Part of Duris *
 *  Usage: Commands that mainly manipulate objects.
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "justice.h"
#include "objmisc.h"
#include "necromancy.h"
#include "sql.h"
#include "ctf.h"
#include "tradeskill.h"
#include "vnum.obj.h"

/*
 * external variables
 */

extern P_desc descriptor_list;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern P_event event_list;
extern bool command_confirm;
extern char *coin_names[];
extern char *drinks[];
extern const int drink_aff[][3];
extern const char *resource_list[];
extern const char *apply_types[];
extern const struct stat_data stat_factor[];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern P_index mob_index;
extern long new_exp_table[];  // Arih: Fixed type mismatch bug - was int, should be long

extern void obj_affect_remove(P_obj, struct obj_affect *);
extern bool has_eq_slot( P_char ch, int wear_slot );
extern void arti_clear_sql( P_char ch, char *arg );

#define USE_SPACE 0
#define IN_WELL_ROOM(x) ((world[(x)->in_room].number == 55126) || (world[(x)->in_room].number == 8003))

bool is_stat_max(sbyte location)
{
  if(location >= APPLY_STR_MAX &&
     location <= APPLY_LUCK_MAX)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

int wield_item_size(P_char ch, P_obj obj)
{
  if( !(IS_SET(obj->extra_flags, ITEM_TWOHANDS)
    || (obj->type == ITEM_WEAPON && obj->value[0] == WEAPON_2HANDSWORD))
    || (IS_GIANT(ch) && obj->type == ITEM_WEAPON) )
  {
    return 1;
  }
  else
  {
    return 2;
  }
}
/*
 * procedures related to get
 */
void get(P_char ch, P_obj o_obj, P_obj s_obj, int showit)
{
  int      got_p = 0, got_g = 0, got_s = 0, got_c = 0, notall = 0;
  char     Gbuf3[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  P_obj    corpse = NULL;
  P_event  e1 = NULL;
  bool     slip = FALSE;

  if( !o_obj || !ch )
  {
    logit(LOG_EXIT, "call to get with NULL obj or ch");
    raise(SIGSEGV);
  }

  if(o_obj->condition <= 0)
  {
    MakeScrap(ch, o_obj);
    return;
  }

  if (GET_CHAR_SKILL(ch, SKILL_SLIP))
  {
    if (number(0, 100) < BOUNDED(5, (GET_CHAR_SKILL(ch, SKILL_SLIP) +
        (GET_C_DEX(ch)/10)), 95))
    {
      slip = TRUE;
    }
  }

  if (IS_NPC(ch) && IN_WELL_ROOM(ch))
  {
    send_to_char("No mobs taking things from the well!\r\n", ch);
    return;
  }
  if (IS_NPC(ch) && (GET_RNUM(ch) == real_mobile(250)))
  {
    send_to_char("Too bad you're a mirror image and can't, eh?\r\n", ch);
    return;
  }

  /* Trap check */
  if (checkgetput(ch, o_obj))
    return;

  /* Don't screw up my pointers! */
  if (o_obj->hitched_to)
  {
    act("You can't, $p is hitched to $N.", FALSE, ch, o_obj,
        o_obj->hitched_to, TO_CHAR);
    return;
  }

  if (s_obj && (s_obj->type == ITEM_CORPSE) &&
      IS_SET(s_obj->value[1], PC_CORPSE))
    corpse = s_obj;

  if (s_obj && IS_OBJ_STAT(s_obj, ITEM_NOSHOW))
    showit = TRUE;

  if ((o_obj->type == ITEM_MONEY) &&
      ((o_obj->value[0] > 0) || (o_obj->value[1] > 0) ||
       (o_obj->value[2] > 0) || (o_obj->value[3] > 0)))
  {
    GET_PLATINUM(ch) += (got_p = o_obj->value[3]);    //(got_p = MAX(0, MIN(o_obj->value[3], CAN_CARRY_COINS(ch))));
//    notall += (got_p != o_obj->value[3]);
    o_obj->value[3] = 0;      //-= got_p;

    GET_GOLD(ch) += (got_g = o_obj->value[2]);        //(got_g = MAX(0, MIN(o_obj->value[2], CAN_CARRY_COINS(ch))));
//    notall += (got_g != o_obj->value[2]);
    o_obj->value[2] = 0;      //-= got_g;

    GET_SILVER(ch) += (got_s = o_obj->value[1]);      //(got_s = MAX(0, MIN(o_obj->value[1], CAN_CARRY_COINS(ch))));
//    notall += (got_s != o_obj->value[1]);
    o_obj->value[1] = 0;      //-= got_s;

    GET_COPPER(ch) += (got_c = o_obj->value[0]);      //(got_c = MAX(0, MIN(o_obj->value[0], CAN_CARRY_COINS(ch))));
//    notall += (got_c != o_obj->value[0]);
    o_obj->value[0] = 0;      //-= got_c;

    int total_value = (got_p * 1000 + got_g * 100 + got_s * 10 + got_c);

    if (total_value <= 0)
    {
      send_to_char("You can't carry any of the coins.\r\n", ch);
      return;
    }
    else if ( total_value > 999999)
    {
      logit(LOG_DEBUG, "%s (%d) got %s from %s.", J_NAME(ch),
        world[ch->in_room].number, coin_stringv(total_value),
        OBJ_NOWHERE(o_obj) ? "NOWHERE!!" : OBJ_ROOM(o_obj) ? "room" :
        OBJ_INSIDE(o_obj) ? o_obj->loc.inside->name :
        OBJ_CARRIED(o_obj) ? GET_NAME(o_obj->loc.carrying) :
        GET_NAME(o_obj->loc.wearing));
      if( IS_PC( ch ) )
      {
        sql_log(ch, PLAYERLOG, "Got %s from %s.",
          coin_stringv(total_value),
          OBJ_NOWHERE(o_obj) ? "NOWHERE!!" : OBJ_ROOM(o_obj) ? "room" :
          OBJ_INSIDE(o_obj) ? o_obj->loc.inside->name :
          OBJ_CARRIED(o_obj) ? GET_NAME(o_obj->loc.carrying) :
          GET_NAME(o_obj->loc.wearing));
      }

      wizlog(MINLVLIMMORTAL, "%s (%d) got %s from %s.",
        J_NAME(ch), world[ch->in_room].number, 
        coin_stringv(total_value),
        OBJ_NOWHERE(o_obj) ? "NOWHERE!!" : OBJ_ROOM(o_obj) ? "room" :
        OBJ_INSIDE(o_obj) ? o_obj->loc.inside->
        name : OBJ_CARRIED(o_obj) ? GET_NAME(o_obj->loc.
                                            carrying) : GET_NAME(o_obj->
                                                                 loc.
                                                                 wearing));
    }
    if (notall)
      snprintf(Gbuf3, MAX_STRING_LENGTH, "You got: ");
    else
      snprintf(Gbuf3, MAX_STRING_LENGTH, "There were: ");
    if (got_p)
      snprintf(Gbuf3 + strlen(Gbuf3), MAX_STRING_LENGTH - strlen(Gbuf3), "%d &+Wplatinum&N coin%s, ", got_p,
              ((got_p > 1) ? "s" : ""));
    if (got_g)
      snprintf(Gbuf3 + strlen(Gbuf3), MAX_STRING_LENGTH - strlen(Gbuf3), "%d &+Ygold&N coin%s, ", got_g,
              ((got_g > 1) ? "s" : ""));
    if (got_s)
      snprintf(Gbuf3 + strlen(Gbuf3), MAX_STRING_LENGTH - strlen(Gbuf3), "%d &+wsilver&n coin%s, ", got_s,
              ((got_s > 1) ? "s" : ""));
    if (got_c)
      snprintf(Gbuf3 + strlen(Gbuf3), MAX_STRING_LENGTH - strlen(Gbuf3), "%d &+ycopper&N coin%s, ", got_c,
              ((got_c > 1) ? "s" : ""));
    Gbuf3[strlen(Gbuf3) - 2] = '.';
    strcat(Gbuf3, "\r\n");

    if (notall)
    {
      if (s_obj)
      {
        if (OBJ_CARRIED_BY(s_obj, ch))
        {
          act("You get some coins from your $Q.", 1, ch, o_obj, s_obj,
              TO_CHAR);
          if (showit && !slip)
            act("$n gets some coins from $s $Q.", 1, ch, o_obj, s_obj,
                TO_ROOM);
        }
        else
        {
          act("You get some coins from $P.", 0, ch, o_obj, s_obj, TO_CHAR);
          if (showit && !slip)
            act("$n gets some coins from $P.", 1, ch, o_obj, s_obj, TO_ROOM);
        }
      }
      else
      {
        act("You get some coins.", 0, ch, o_obj, 0, TO_CHAR);
        if (showit && !slip)
          act("$n gets some coins.", 1, ch, o_obj, 0, TO_ROOM);
      }
      send_to_char("You couldn't carry all the coins.\r\n", ch);
      send_to_char(Gbuf3, ch);
      add_coins(o_obj, 0, 0, 0, 0);     /* change pile descs */
    }
    else
    {
      if (s_obj)
      {
        obj_from_obj(o_obj);
        if (OBJ_CARRIED_BY(s_obj, ch))
        {
          act("You get $p from your $Q.", 0, ch, o_obj, s_obj, TO_CHAR);
          if (showit && !slip)
            act("$n gets $p from $s $Q.", 1, ch, o_obj, s_obj, TO_ROOM);
        }
        else
        {
          act("You get $p from $P.", 0, ch, o_obj, s_obj, TO_CHAR);
          if (showit && !slip)
            act("$n gets $p from $P.", 1, ch, o_obj, s_obj, TO_ROOM);
        }
      }
      else
      {
        obj_from_room(o_obj);
        act("You get $p.", 0, ch, o_obj, 0, TO_CHAR);
        if (showit && !slip)
          act("$n gets $p.", 1, ch, o_obj, 0, TO_ROOM);
      }
      send_to_char(Gbuf3, ch);
      extract_obj(o_obj);
      o_obj = NULL;
    }

    // this call to writeCharacter is a Bad Thing.  Whatever is calling
    // get() should be writing the character.  Calling it here results in
    // screwed up pointers in do_get() if there's an event on the writeCharacter
    // writeCharacter(ch, 1, ch->in_room);

    /* update player corpse file  (if needed) */
    if (corpse)
    {
      writeCorpse(corpse);
    }
    return;
  }
  if (s_obj)
  {
    if (IS_OBJ_STAT2(o_obj, ITEM2_NOLOOT) && !IS_TRUSTED(ch))
    {
      send_to_char("&+LYou cannot take that.&n\n\r", ch);
      return;
    }

    obj_from_obj(o_obj);

#if USE_SPACE
    s_obj->space -= GET_OBJ_SPACE(o_obj);
#endif

    if (OBJ_CARRIED_BY(s_obj, ch))
    {
      act("You get $p from $P.", 0, ch, o_obj, s_obj, TO_CHAR);
      if (showit && !slip)
        act("$n gets $p from $s $Q.", 1, ch, o_obj, s_obj, TO_ROOM);
    }
    else
    {
      act("You get $p from $P.", 0, ch, o_obj, s_obj, TO_CHAR);
      if (showit && !slip)
        act("$n gets $p from $P.", 1, ch, o_obj, s_obj, TO_ROOM);
    }
    obj_to_char(o_obj, ch);
  }
  else
  {
    if (IS_OBJ_STAT2(o_obj, ITEM2_NOLOOT) && !IS_TRUSTED(ch))
    {
      send_to_char("&+LYou cannot take that.&n\n\r", ch);
      return;
    }

    obj_from_room(o_obj);
    act("You get $p.", 0, ch, o_obj, 0, TO_CHAR);
    if (showit && !slip)
      act("$n gets $p.", 1, ch, o_obj, 0, TO_ROOM);
    obj_to_char(o_obj, ch);
  }

  if (corpse)
    writeCorpse(corpse);

  char_light(ch);
  room_light(ch->in_room, REAL);
}

int fight_in_room(P_char ch)
{
  P_char   person = NULL;

  for (person = world[ch->in_room].people; person;
       person = person->next_in_room)
  {
    if (IS_FIGHTING(person))
    {
      return TRUE;
    }
  }
  return FALSE;
}

void do_get(P_char ch, char *argument, int cmd)
{
  P_char   hood = NULL, owner = NULL, rider;
  P_obj    s_obj = NULL, o_obj = NULL, next_obj;
  bool     found = FALSE, fail = FALSE, corpse_flag = FALSE, alldot = FALSE, carried;
  char     Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];
  char     Gbuf4[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  char     arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int      type = 0, total = 0;
  int      looting = FALSE;
  int      wfound = FALSE;
  int      i, j;
  P_obj    t_obj;
  int      wear_order[] =
    { 41, 24, 40, 6, 19, 21, 22, 20, 39, 3, 4, 5, 35, 37, 12, 27, 23, 13, 28,
    29, 30, 10, 31, 11, 14, 15, 33, 34, 9, 32, 1, 2, 16, 17, 25, 26, 18, 7,
      36, 8, 38, -1
  };

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if( IS_IMMOBILE(ch) )
  {
    send_to_char("No can do in your present state!\r\n", ch);
    return;
  }

  *Gbuf2 = '\0';
  *Gbuf3 = '\0';

  for (j = 0; wear_order[j] != -1; j++)
  {
    if (ch->equipment[wear_order[j]])
    {
      t_obj = ch->equipment[wear_order[j]];
      wfound = TRUE;
    }
  }

  if( IS_AFFECTED(ch, AFF_WRAITHFORM) )
  {
    send_to_char("You are too intangible!\r\n", ch);
    return;
  }

  if( IS_ANIMAL(ch) && IS_NPC(ch) )
  {
    send_to_char("You are a beast!\r\n", ch);
    return;
  }

  argument_interpreter(argument, arg1, arg2);

  if(IS_NPC(ch) && ch->following &&
    (ch->in_room == ch->following->in_room))
  {
    hood = ch->following;
  }
  else
  {
    hood = ch;
  }

  /* get type */
  if (!*arg1)                   /* no args, error  */
    type = 0;

  if (*arg1 && !*arg2)
  {                             /* only 1 arg, so assumes (from room) */
    alldot = FALSE;
    Gbuf2[0] = '\0';
    if (!strn_cmp(arg1, "all", 3) && (sscanf(arg1, "all.%s", Gbuf2) > 0))
    {
      strcpy(arg1, "all");
      alldot = TRUE;
    }
    if (!str_cmp(arg1, "all"))
    {
      type = 1;                 /* get all.(*) (from room) */
    }
    else
    {
      type = 2;                 /* get <object> (from room) */
    }
  }
  else if (*arg1 && *arg2)
  {                             /* 2 args, get something(s) from a container */
    alldot = FALSE;
    Gbuf2[0] = '\0';
    if (!strn_cmp(arg1, "all", 3) && (sscanf(arg1, "all.%s", Gbuf2) > 0))
    {
      strcpy(arg1, "all");
      alldot = TRUE;
    }
    if (!str_cmp(arg1, "all"))
    {
      if (!str_cmp(arg2, "all"))
        type = 3;
      else
        type = 4;
    }
    else
    {
      if (!str_cmp(arg2, "all"))
        type = 5;
      else
        type = 6;
    }
  }

// Removing buggy switch.

    /* get */
  if(type == 0)
  {
    send_to_char("Get what?\r\n", ch);
  }

    /* get all */
  if(type == 1)
  {
    s_obj = 0;
    found = FALSE;
    fail = FALSE;
    for (o_obj = world[ch->in_room].contents; o_obj; o_obj = next_obj)
    {
      next_obj = o_obj->next_content;

      if (alldot && !isname(Gbuf2, o_obj->name))
        continue;

      if (CAN_SEE_OBJ(ch, o_obj))
      {
        /* was object disarmed?  did PC still manage to get it?
           if (check_get_disarmed_obj(ch, o_obj->last_to_hold, o_obj))
           continue; */

        if( (IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch) ||
          ( (OBJ_VNUM(o_obj) > LOWEST_MAT_VNUM) && (OBJ_VNUM(o_obj) <= HIGHEST_MAT_VNUM)) )
        {
          if ((IS_CARRYING_W(ch, rider) + GET_OBJ_WEIGHT(o_obj)) <= CAN_CARRY_W(ch))
          {
            if( CAN_WEAR(o_obj, ITEM_TAKE)
              || ((GET_LEVEL(ch) >= 60) && !IS_NPC(ch)) )
            {
              get(ch, o_obj, 0, TRUE);
              if( IS_ARTIFACT( o_obj ) )
              {
                wizlog( 56, "%s getting artifact %s (%d) from room %d.",
                  J_NAME(ch), o_obj->short_description, obj_index[o_obj->R_num].virtual_number, world[ch->in_room].number );
                logit(LOG_OBJ, "%s getting artifact %s (%d) from room %d.",
                  J_NAME(ch), o_obj->short_description, obj_index[o_obj->R_num].virtual_number, world[ch->in_room].number );
              }
              total++;
            }
            else
            {
              strcpy(Gbuf4, o_obj->short_description);
              CAP(Gbuf4);
              snprintf(Gbuf3, MAX_STRING_LENGTH, "%s isn't takeable.\r\n", Gbuf4);
              send_to_char(Gbuf3, ch);
              fail = TRUE;
            }
          }
          else
          {
            strcpy(Gbuf4, o_obj->short_description);
            CAP(Gbuf4);
            snprintf(Gbuf3, MAX_STRING_LENGTH, "%s is too heavy to lift.\r\n", Gbuf4);
            send_to_char(Gbuf3, ch);
            fail = TRUE;
          }
        }
        else
        {
          send_to_char("You can't carry anything more.\r\n", ch);
          fail = TRUE;
          break;
        }
      }
    }

    if (total > 1)
    {
      snprintf(Gbuf3, MAX_STRING_LENGTH, "You got %d items.\r\n", total);
      send_to_char(Gbuf3, ch);
    }
    else if (!total)
    {
      if (!fail)
        send_to_char("You see nothing here.\r\n", ch);
    }
  }
    /* get ??? */
  if(type == 2)
  {
    found = FALSE;
    fail = FALSE;

    if( (o_obj = get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents, !IS_TRUSTED(ch))) )
    {
      /*
       * was object disarmed?  did PC still manage to get it? if so,
       * get object --TAM
       if (check_get_disarmed_obj(ch, o_obj->last_to_hold, o_obj))
       return;
       */

      if( IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
        || ((OBJ_VNUM(o_obj) > LOWEST_MAT_VNUM) && (OBJ_VNUM(o_obj) <= HIGHEST_MAT_VNUM)) )
      {
        if( (IS_CARRYING_W(ch, rider) + GET_OBJ_WEIGHT(o_obj)) <= CAN_CARRY_W(ch) )
        {
          if( CAN_WEAR(o_obj, ITEM_TAKE)
            || ((GET_LEVEL(ch) >= 60) && !IS_NPC(ch)) )
          {
            if( (GET_ITEM_TYPE(o_obj) == ITEM_CORPSE) && IS_SET(o_obj->value[1], PC_CORPSE) )
            {
              owner = get_char(o_obj->action_description);
              if ((ch == owner) ||
                  (owner && is_linked_to(ch, owner, LNK_CONSENT)) ||
                  (IS_TRUSTED(ch)))
              {
                logit(LOG_CORPSE, "%s%s: corpse of %s",
                      GET_NAME(ch), (hood == ch) ? "" : GET_NAME(hood),
                      o_obj->action_description);
                if (!wfound)
                  logit(LOG_CORPSE, "%s %s", GET_NAME(ch), argument);

              }
              else
              {
                send_to_char("Looting of player corpses requires consent.\r\n", ch);
                return;
              }
            }
            found = TRUE;
            get(ch, o_obj, s_obj, TRUE);
            if( IS_ARTIFACT( o_obj ) )
            {
              wizlog( 56, "%s getting artifact %s (%d) from room %d.",
                J_NAME(ch), o_obj->short_description, obj_index[o_obj->R_num].virtual_number, world[ch->in_room].number );
              logit(LOG_OBJ, "%s getting artifact %s (%d) from room %d.",
                J_NAME(ch), o_obj->short_description, obj_index[o_obj->R_num].virtual_number, world[ch->in_room].number );
            }

          }
          else
          {
            strcpy(Gbuf4, o_obj->short_description);
            CAP(Gbuf4);
            snprintf(Gbuf3, MAX_STRING_LENGTH, "%s isn't takeable.\r\n", Gbuf4);
            send_to_char(Gbuf3, ch);
            fail = TRUE;
          }
        }
        else
        {
          strcpy(Gbuf4, o_obj->short_description);
          CAP(Gbuf4);
          snprintf(Gbuf3, MAX_STRING_LENGTH, "%s is too heavy.\r\n", Gbuf4);
          send_to_char(Gbuf3, ch);
          fail = TRUE;
        }
      }
      else
      {
        send_to_char("You can't carry any more.\r\n", ch);
        fail = TRUE;
      }
    }
    else
    {
      if( IS_TRUSTED(ch) && !strcmp(arg1, "nowhere") )
      {
        send_to_char("Collecting Items from Nowhere:\n", ch);
        next_obj = object_list;
        Gbuf2[(i=0)] = '\0';
        while( next_obj )
        {
          s_obj = next_obj;
          next_obj = next_obj->next;
          // If the object is in the void, or a bad room, or worn on or carried by a bad person, or inside a bad object
          if( OBJ_NOWHERE(s_obj) || (OBJ_ROOM(s_obj) && ( ROOM_VNUM(s_obj->loc.room) == NOWHERE ))
            || (OBJ_WORN( s_obj ) && ( s_obj->loc.wearing == NULL ))
            || (OBJ_CARRIED( s_obj ) && ( s_obj->loc.carrying == NULL ))
            || (OBJ_INSIDE( s_obj ) && ( s_obj->loc.inside == NULL )) )
          {
            // Set object to LOC_NOWHERE so we can give it to the char without errors: Applies to NULL location objects.
            s_obj->loc_p = LOC_NOWHERE;
            obj_to_char( s_obj, ch );
            i += snprintf(Gbuf2 + i, MAX_STRING_LENGTH, "%s, ", OBJ_SHORT(s_obj) );
          }
        }
        if( i > 0 )
        {
          snprintf(Gbuf2 + i - 2, MAX_STRING_LENGTH, ".\n" );
        }
        else
        {
          snprintf(Gbuf2, MAX_STRING_LENGTH, "No items found in nowhere.\n" );
        }
        send_to_char( Gbuf2, ch );
        return;
      }
      snprintf(Gbuf3, MAX_STRING_LENGTH, "You do not see a %s here.\r\n", arg1);
      send_to_char(Gbuf3, ch);
      fail = TRUE;
    }
  }

    /* get all all */
  if(type == 3)
  {
    send_to_char("You must be joking?!\r\n", ch);
  }

    /* get all ??? */
  if(type == 4)
  {
    found = FALSE;
    fail = FALSE;

    s_obj = get_obj_in_list_vis(ch, arg2, ch->carrying);
    if (!s_obj)
      s_obj = get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents);

    if (s_obj)
    {
      if ((GET_ITEM_TYPE(s_obj) == ITEM_CONTAINER) ||
          (GET_ITEM_TYPE(s_obj) == ITEM_STORAGE) ||
          (GET_ITEM_TYPE(s_obj) == ITEM_QUIVER) ||
          (GET_ITEM_TYPE(s_obj) == ITEM_CORPSE))
      {

        if ((GET_ITEM_TYPE(s_obj) != ITEM_CORPSE) &&
            IS_SET(s_obj->value[1], CONT_CLOSED))
        {
          send_to_char("It seems to be closed.\r\n", ch);
          return;
        }
        if( (IS_FIGHTING(ch) || IS_DESTROYING(ch)) && (GET_ITEM_TYPE(s_obj) == ITEM_CORPSE))
        {
          send_to_char("You're too busy fighting to be pulling things out of bags!\r\n", ch);
          return;
        }
        if ((GET_ITEM_TYPE(s_obj) == ITEM_CORPSE) &&
            IS_SET(s_obj->value[CORPSE_FLAGS], PC_CORPSE))
          corpse_flag = 1;
        else
          corpse_flag = 0;
        if (corpse_flag && fight_in_room(ch) && !on_front_line(ch))
        {
          send_to_char("There's too much &+Rb&+rl&+Ro&+ro&+Rd&n flying around for you to do that!\r\n", ch);
          return;
        }

        for (o_obj = s_obj->contains; o_obj; o_obj = next_obj)
        {
          next_obj = o_obj->next_content;

          if (alldot && !isname(Gbuf2, o_obj->name))
            continue;

          /*
           * fixed an annoying bug, you can now always get
           * something from a container in your inv, in spite of
           * weight -JAB
           */
          if( CAN_SEE_OBJ(ch, o_obj) )
          {
            if( IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
              || ((OBJ_VNUM(o_obj) >= LOWEST_MAT_VNUM) && (OBJ_VNUM(o_obj) <= HIGHEST_MAT_VNUM) ))
            {
              if( ((IS_CARRYING_W(ch, rider) + GET_OBJ_WEIGHT(o_obj)) < CAN_CARRY_W(ch)) || OBJ_CARRIED(s_obj) )
              {
                if( CAN_WEAR(o_obj, ITEM_TAKE) || ((GET_LEVEL(ch) >= 60) && !IS_NPC(ch)) )
                {
                  if( (GET_ITEM_TYPE(o_obj) == ITEM_CORPSE) && IS_SET(o_obj->value[1], PC_CORPSE) )
                  {
                    logit(LOG_CORPSE, "%s%s: corpse of %s from %s", GET_NAME(ch), (hood == ch) ? "" : GET_NAME(hood),
                      o_obj->action_description, s_obj->name);
                  }
                  else if( corpse_flag && o_obj )
                  {
                    if( o_obj->type == ITEM_MONEY )
                    {
                      logit(LOG_CORPSE, "%s%s: %dp, %dg, %ds, %dc from %s",
                        GET_NAME(ch), (hood == ch) ? "" : GET_NAME(hood), o_obj->value[3], o_obj->value[2],
                        o_obj->value[1], o_obj->value[0], s_obj->action_description);
                    }
                    else
                    {
                      if( CAN_WEAR(o_obj, ITEM_WEAR_IOUN) || IS_ARTIFACT(o_obj) )
                      {
                        logit(LOG_CORPSE, "%s%s: %s [%d] (ARTIFACT) from %s", GET_NAME(ch),
                          (hood == ch) ? "" : GET_NAME(hood),
                          o_obj->name, obj_index[o_obj->R_num].virtual_number, s_obj->action_description);

                        act("$n gets $P from $p.", 0, ch, s_obj, o_obj, TO_ROOM);

                        // If the artifact was picked up across racewar lines.
                        if( (s_obj->value[5] != RACEWAR_NONE) && (GET_RACEWAR(ch) != s_obj->value[5]) )
                        {
                          int vnum = OBJ_VNUM(o_obj);
                          int owner_pid = -1;
                          int timer = time(NULL);
                          // This sets the 'soul' of the artifact to the new owner.
                          sql_update_bind_data(vnum, &owner_pid, &timer);
                          // Feed artifact to at least the minimum for across racewar sides.
                          artifact_feed_to_min_sql( o_obj, 5 * MINS_PER_REAL_DAY );
                        }
                      }
                      else
                      {
                        logit(LOG_CORPSE, "%s%s: %s [%d] from %s", GET_NAME(ch),
                          (hood == ch) ? "" : GET_NAME(hood), o_obj->name, obj_index[o_obj->R_num].virtual_number,
                          s_obj->action_description);

                        act("$n gets $P from $p.", 0, ch, s_obj, o_obj, TO_ROOM);

                      }
                    }
                  }
                  get(ch, o_obj, s_obj, FALSE);

                  total++;
                  if( IS_ARTIFACT( s_obj ) )
                  {
                    wizlog( 56, "%s getting artifact %s (%d) from room %d.",
                      J_NAME(ch), s_obj->short_description, obj_index[s_obj->R_num].virtual_number, world[ch->in_room].number );
                    logit(LOG_OBJ, "%s getting artifact %s (%d) from room %d.",
                      J_NAME(ch), s_obj->short_description, obj_index[s_obj->R_num].virtual_number, world[ch->in_room].number );
                  }
                  if (GET_ITEM_TYPE(s_obj) == ITEM_QUIVER)
                    if (s_obj->value[3] > 0)
                      s_obj->value[3]--;
                }
                else
                {
                  snprintf(Gbuf3, MAX_STRING_LENGTH, "%s isn't takeable.\r\n", o_obj->short_description);
                  send_to_char(Gbuf3, ch);
                  fail = TRUE;
                }
              }
              else
              {
                snprintf(Gbuf3, MAX_STRING_LENGTH, "%s is too heavy.\r\n", o_obj->short_description);
                send_to_char(Gbuf3, ch);
                fail = TRUE;
              }
            }
            else
            {
              send_to_char("You can't carry any more.\r\n", ch);
              fail = TRUE;
              break;
            }
          }
          if (corpse_flag && !IS_TRUSTED(ch))
          {
            CharWait(ch, PULSE_VIOLENCE);
          }
        }
        if (!total && !fail)
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "%s appears to be empty.\r\n", s_obj->short_description);
          send_to_char(Gbuf3, ch);
          fail = TRUE;
        }
        else if (total == 1)
          act("$n gets something from $p.", TRUE, ch, s_obj, 0, TO_ROOM);
        else if ((total > 1) && (total < 6))
          act("$n gets some stuff from $p.", TRUE, ch, s_obj, 0, TO_ROOM);
        else if (total > 5)
          act("$n gets a bunch of stuff from $p.", TRUE, ch, s_obj, 0, TO_ROOM);
      }
      else
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "%s is not a container.\r\n", s_obj->short_description);
        send_to_char(Gbuf3, ch);
        fail = TRUE;
      }
    }
    else
    {
      snprintf(Gbuf3, MAX_STRING_LENGTH, "You do not see or have the %s.\r\n", arg2);
      send_to_char(Gbuf3, ch);
      fail = TRUE;
    }
  }

    /* get ??? all */
  if(type == 5)
  {
    send_to_char("You can't take things from two or more containers.\r\n", ch);
  }

    /* get ??? ??? */
  if(type == 6)
  {
    found = FALSE;
    fail = FALSE;
    carried = TRUE;
    s_obj = get_obj_in_list_vis(ch, arg2, ch->carrying);
    if (!s_obj)
    {
      s_obj = get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents);
      carried = FALSE;
    }
    if (s_obj)
    {
      if ((GET_ITEM_TYPE(s_obj) == ITEM_CONTAINER) ||
          (GET_ITEM_TYPE(s_obj) == ITEM_CORPSE) ||
          (GET_ITEM_TYPE(s_obj) == ITEM_STORAGE) ||
          (GET_ITEM_TYPE(s_obj) == ITEM_QUIVER))
      {

        if ((GET_ITEM_TYPE(s_obj) != ITEM_CORPSE) &&
            IS_SET(s_obj->value[1], CONT_CLOSED))
        {
          send_to_char("It seems to be closed.\r\n", ch);
          return;
        }

        if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
        {
          send_to_char("You're too busy fighting to be pulling things out of bags!\r\n", ch);
          return;
        }

        if ((GET_ITEM_TYPE(s_obj) == ITEM_CORPSE) &&
            IS_SET(s_obj->value[CORPSE_FLAGS], PC_CORPSE))
          corpse_flag = 1;
        else
          corpse_flag = 0;

        if( (IS_FIGHTING(ch) || IS_DESTROYING(ch)) && (GET_ITEM_TYPE(s_obj) == ITEM_CORPSE))
        {
          send_to_char("You're too busy fighting to be pulling things out of bags!\r\n", ch);
          return;
        }

        /*
         * fixed an annoying bug, you can now always get
         * something from a container in your inv, in spite of
         * weight  -JAB
         */
        if (carried)
          o_obj = get_obj_in_list(arg1, s_obj->contains);
        else
          o_obj = get_obj_in_list_vis(ch, arg1, s_obj->contains);
        if (o_obj)
        {
          if (IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
            || ((OBJ_VNUM(o_obj) >= LOWEST_MAT_VNUM) && (OBJ_VNUM(o_obj) <= HIGHEST_MAT_VNUM) ))
          {
            if( ((IS_CARRYING_W(ch, rider) + GET_OBJ_WEIGHT(o_obj)) < CAN_CARRY_W(ch)) || OBJ_CARRIED(s_obj) )
            {
              if( CAN_WEAR(o_obj, ITEM_TAKE) || ((GET_LEVEL(ch) >= 60) && !IS_NPC(ch)) )
              {
                // SAM 7-94 log corpse looting of player
                if( (GET_ITEM_TYPE(o_obj) == ITEM_CORPSE) && IS_SET(o_obj->value[1], PC_CORPSE) )
                {
                  logit(LOG_CORPSE, "%s%s: corpse of %s from %s", GET_NAME(ch), (hood == ch) ? "" : GET_NAME(hood),
                    o_obj->action_description, s_obj->name);
                }
                else if( corpse_flag && o_obj )
                {
                  if( GET_ITEM_TYPE(o_obj) == ITEM_MONEY )
                  {
                    logit(LOG_CORPSE, "%s%s: %dp, %dg, %ds, %dc from %s", GET_NAME(ch),
                      (hood == ch) ? "" : GET_NAME(hood), o_obj->value[3], o_obj->value[2],
                      o_obj->value[1], o_obj->value[0], s_obj->action_description);
                  }
                  else
                  {
                    if( CAN_WEAR(o_obj, ITEM_WEAR_IOUN) || IS_ARTIFACT(o_obj) )
                    {
                      logit(LOG_CORPSE, "%s %s: %s [%d] (ARTIFACT) from %s", GET_NAME(ch),
                        (hood == ch) ? "" : GET_NAME(hood), o_obj->name,
                        obj_index[o_obj->R_num].virtual_number, s_obj->action_description);
                      // If the artifact was picked up across racewar lines.
                      if( (s_obj->value[5] != RACEWAR_NONE) && (GET_RACEWAR(ch) != s_obj->value[5]) )
                      {
                        int vnum = OBJ_VNUM(o_obj);
                        int owner_pid = -1;
                        int timer = time(NULL);
                        sql_update_bind_data(vnum, &owner_pid, &timer);
                        artifact_feed_to_min_sql( o_obj, (ARTIFACT_BLOOD_DAYS / 2) * MINS_PER_REAL_DAY );
                      }
                    }
                    else
                    {
                      logit(LOG_CORPSE, "%s%s: %s [%d] from %s", GET_NAME(ch), (hood == ch) ? "" : GET_NAME(hood),
                        o_obj->name, obj_index[o_obj->R_num].virtual_number, s_obj->action_description);
                    }
                  }
                  if( !IS_TRUSTED(ch) )
                  {
                    CharWait(ch, PULSE_VIOLENCE);
                  }
                }
                get(ch, o_obj, s_obj, TRUE);

                found = TRUE;
                if (GET_ITEM_TYPE(s_obj) == ITEM_QUIVER)
                  if (s_obj->value[3] > 0)
                    s_obj->value[3]--;
              }
              else
              {
                snprintf(Gbuf3, MAX_STRING_LENGTH, "%s isn't takable.\r\n",
                        o_obj->short_description);
                send_to_char(Gbuf3, ch);
                fail = TRUE;
              }
            }
            else
            {
              snprintf(Gbuf3, MAX_STRING_LENGTH, "%s is too heavy.\r\n",
                      o_obj->short_description);
              send_to_char(Gbuf3, ch);
              fail = TRUE;
            }
          }
          else
          {
            send_to_char("You can't carry any more.\r\n", ch);
            fail = TRUE;
          }
        }
        else
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "%s does not contain the %s.\r\n",
                  s_obj->short_description, arg1);
          send_to_char(Gbuf3, ch);
          fail = TRUE;
        }
      }
      else
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "%s isn't a container.\r\n",
                s_obj->short_description);
        send_to_char(Gbuf3, ch);
        fail = TRUE;
      }
    }
    else
    {
      snprintf(Gbuf3, MAX_STRING_LENGTH, "You do not see or have the %s.\r\n", arg2);
      send_to_char(Gbuf3, ch);
      fail = TRUE;
    }
  }

  writeCharacter(ch, 1, ch->in_room);
  char_light(ch);
  room_light(ch->in_room, REAL);

  if(looting)
    CharWait(ch, 10);
}

void do_junk(P_char ch, char *argument, int cmd)
{
  P_obj    tmp_object, next_obj;
  P_char   t_ch;
  bool     test = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];

  argument = one_argument(argument, Gbuf1);

  if( !IS_ALIVE(ch) || !IS_TRUSTED(ch) )
  {
    return;
  }

  /*
   * SAM 7-94, make char confirm a junk command
   */
  if (!command_confirm)
  {

    /*
     * check if its a reasonable argument to junk
     */
    if (is_number(Gbuf1))
    {
      send_to_char("Recycling coins is STUPID! :)", ch);
      if (ch->desc)
        ch->desc->confirm_state = CONFIRM_NONE;
      return;
    }
    else if ((*Gbuf1 == '\0') || (ch->carrying == NULL))
    {
      send_to_char("Junk what?\r\n", ch);
      if (ch->desc)
        ch->desc->confirm_state = CONFIRM_NONE;
      return;
    }
    else if ((str_cmp(Gbuf1, "all")) &&
             (!get_obj_in_list_vis(ch, Gbuf1, ch->carrying)))
    {
      send_to_char("Junk what?\r\n", ch);
      if (ch->desc)
        ch->desc->confirm_state = CONFIRM_NONE;
      return;
    }
    /*
     * seems somewhat legal, so ask them to confirm it
     */
    else if (ch->desc && !IS_TRUSTED(ch))
    {
      snprintf(Gbuf3, MAX_STRING_LENGTH,
              "WARNING: JUNK permanently destroys the specified object(s).\r\n"
              "Please confirm that you wish to JUNK %s (Yes/No) [No]:\r\n",
              Gbuf1);
      send_to_char(Gbuf3, ch);
      return;
    }
    else if (ch->desc)
      ch->desc->confirm_state = CONFIRM_NONE;
  }
  /*
   * end SAM
   */

  /*
   * confirmed junk!!!
   */

  if (*Gbuf1)
  {
    if (!str_cmp(Gbuf1, "all"))
    {
      for (tmp_object = ch->carrying; tmp_object; tmp_object = next_obj)
      {
        next_obj = tmp_object->next_content;

        if( IS_ARTIFACT(tmp_object) )
        {
          act("But $p is not junk!", 1, ch, tmp_object, 0, TO_CHAR);
          continue;
        }

        if (!IS_SET(tmp_object->extra_flags, ITEM_NODROP) || IS_TRUSTED(ch))
        {
          if (!IS_SET(tmp_object->extra_flags, ITEM_TRANSIENT))
          {
            if (CAN_SEE_OBJ(ch, tmp_object))
            {
              snprintf(Gbuf3, MAX_STRING_LENGTH, "You junk %s&n.\r\n", OBJ_SHORT(tmp_object));
              send_to_char(Gbuf3, ch);
              act("You are awarded for outstanding performance in recycling.",
                  FALSE, ch, 0, 0, TO_CHAR);
              act("$n has been awarded for being a good citizen.",
                  TRUE, ch, 0, 0, TO_ROOM);
              GET_SILVER(ch) += 1;
            }
            else
            {
              send_to_char("You junk something.\r\n", ch);
            }
          }
          else
          {
            snprintf(Gbuf3, MAX_STRING_LENGTH, "%s dissolves with a blinding light.\r\n", OBJ_SHORT(tmp_object));
            // Capitalize the first non-ansi char.
            CAP( Gbuf3 );

            for (t_ch = world[ch->in_room].people; t_ch; t_ch = t_ch->next_in_room)
            {
              if( CAN_SEE_OBJ(t_ch, tmp_object) )
                send_to_char(Gbuf3, t_ch);
            }
            extract_obj(tmp_object, TRUE); // Just in case someone enables junking artis.
            tmp_object = NULL;
            test = TRUE;
            continue;
          }
          act("$n junks $p.", 1, ch, tmp_object, 0, TO_ROOM);
          obj_from_char(tmp_object);
          extract_obj(tmp_object, TRUE); // Just in case someone enables junking artis.
          tmp_object = NULL;
          GET_SILVER(ch) += 1;
          test = TRUE;
        }
        else
        {
          if (CAN_SEE_OBJ(ch, tmp_object))
          {
            snprintf(Gbuf3, MAX_STRING_LENGTH, "You can't junk the %s, it must be CURSED!\r\n",
                    FirstWord(tmp_object->name));
            send_to_char(Gbuf3, ch);
            test = TRUE;
          }
        }
      }
      // (!str_cmp(Gbuf1, "all"))
      if (!test)
      {
        send_to_char("You do not seem to have anything.\r\n", ch);
      }
    }
    else
    {
      tmp_object = get_obj_in_list_vis(ch, Gbuf1, ch->carrying);
      if (tmp_object)
      {
        if( IS_ARTIFACT(tmp_object) )
        {
          act("But $p is not junk!", 1, ch, tmp_object, 0, TO_CHAR);
          return;
        }
        if (!IS_SET(tmp_object->extra_flags, ITEM_NODROP) || IS_TRUSTED(ch))
        {
          if (!IS_SET(tmp_object->extra_flags, ITEM_TRANSIENT))
          {
            snprintf(Gbuf3, MAX_STRING_LENGTH, "You junk %s.\r\n", OBJ_SHORT(tmp_object) );
            send_to_char(Gbuf3, ch);
            act("$n junks $p.", 1, ch, tmp_object, 0, TO_ROOM);
            extract_obj(tmp_object, TRUE); // Just in case someone enables junking artis.
            tmp_object = NULL;
            GET_SILVER(ch) += 1;
          }
          else
          {
            snprintf(Gbuf3, MAX_STRING_LENGTH, "%s dissolves with a blinding light.\r\n", OBJ_SHORT(tmp_object) );
            CAP(Gbuf3);
            for( t_ch = world[ch->in_room].people; t_ch; t_ch = t_ch->next_in_room )
            {
              if (CAN_SEE_OBJ(t_ch, tmp_object))
                send_to_char(Gbuf3, t_ch);
            }
            extract_obj(tmp_object, TRUE); // Just in case someone enables junking artis.
            /*
             * added by DTS 5/18/95 to solve light bug
             */
            char_light(ch);
            room_light(ch->in_room, REAL);
            return;
          }
        }
        else
          send_to_char("You can't junk it, it must be CURSED!\r\n", ch);
      }
      else
      {
        send_to_char("You do not have that item.\r\n", ch);
      }
    }
  }
  else
  {
    send_to_char("Junk what?\r\n", ch);
  }
}

void do_dropalldot(P_char ch, char *name, int cmd)
{
  P_obj    tmp_object, next_object, remember = NULL;
  P_char   tmp_ch;
  int      total = 0;
  int      plat, silv, gold, copp;
  char     Gbuf1[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];


  if (!strcmp(name, "coins"))
  {
    plat = ch->points.cash[0];
    gold = ch->points.cash[1];
    silv = ch->points.cash[2];
    copp = ch->points.cash[3];

    if ((plat + gold + silv + copp) == 0)
    {
      act("But you do not have any coins.", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }

    snprintf(Gbuf3, MAX_STRING_LENGTH,
            "You drop %d &+Wplatinum&n, %d &+Ygold&n, %d silver, and %d &+ycopper&n coin%s.\n\r",
            copp, silv, gold, plat,
            ((plat + gold + silv + copp) > 1) ? "s" : "");
    act(Gbuf3, TRUE, ch, 0, 0, TO_CHAR);
    act("$n drops some coins.", TRUE, ch, 0, 0, TO_ROOM);

    tmp_object = create_money(plat, gold, silv, copp);


    ch->points.cash[0] -= plat;
    ch->points.cash[1] -= gold;
    ch->points.cash[2] -= silv;
    ch->points.cash[3] -= copp;

    if ((plat * 1000 + gold * 100 + silv * 10 + copp) > 99000)
    {
      wizlog(MINLVLIMMORTAL, "%s drops %d p %d g %d s %d c in [%d]",
             J_NAME(ch), plat, gold, silv, copp, world[ch->in_room].number);
      logit(LOG_DEBUG, "%s drops %d p %d g %d s %d c in [%d]", J_NAME(ch),
            plat, gold, silv, copp, world[ch->in_room].number);
    }

    if (tmp_object && (ch->in_room != NOWHERE))
      obj_to_room(tmp_object, ch->in_room);
    else
    {
      logit(LOG_EXIT, "do_dropalldot: no tmp_object or ch in NOWHERE");
      raise(SIGSEGV);
    }

    if (IS_PC(ch))
      writeCharacter(ch, 1, ch->in_room);

    return;
  }

  /*
   * If "put all.object bag", get all carried items * named "object",
   * and put each into the bag.
   */

  for (tmp_object = ch->carrying; tmp_object; tmp_object = next_object)
  {
    next_object = tmp_object->next_content;
    if (isname(name, tmp_object->name))
      if (!IS_SET(tmp_object->extra_flags, ITEM_NODROP) || IS_TRUSTED(ch))
      {
        obj_from_char(tmp_object);
        obj_to_room(tmp_object, ch->in_room);
        total++;
        if (IS_TRUSTED(ch))
        {
          wizlog(GET_LEVEL(ch), "%s drops %s [%d].",
                 J_NAME(ch), tmp_object->short_description,
                 world[ch->in_room].number);
          logit(LOG_WIZ, "%s drops %s [%d].",
                J_NAME(ch), tmp_object->short_description,
                world[ch->in_room].number);
          sql_log(ch, WIZLOG, "Drops %s &n[%d]", tmp_object->short_description, obj_index[tmp_object->R_num].virtual_number);
        }
        else if( IS_ARTIFACT( tmp_object ) )
        {
          wizlog( 56, "%s dropping artifact %s (%d) in room %d.",
            J_NAME(ch), tmp_object->short_description, obj_index[tmp_object->R_num].virtual_number, world[ch->in_room].number );
          logit(LOG_OBJ, "%s dropping artifact %s (%d) in room %d.",
            J_NAME(ch), tmp_object->short_description, obj_index[tmp_object->R_num].virtual_number, world[ch->in_room].number );
        }
      }
  }

  if (total)
  {
    if (total == 1)
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "You drop one %s.", name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
      snprintf(Gbuf1, MAX_STRING_LENGTH, "$n drops one %s.", name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
    }
    else if (total < 6)
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "You drop %d %s(s).", total, name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
      snprintf(Gbuf1, MAX_STRING_LENGTH, "$n drops some %s(s).", name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "You drop %d %s(s).", total, name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
      snprintf(Gbuf1, MAX_STRING_LENGTH, "$n drops a bunch of %s(s).", name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
    }


    if (IS_PC(ch))
      writeCharacter(ch, 1, ch->in_room);
  }
  else
  {
    send_to_char("You don't have any of those to drop.\r\n", ch);
  }

}

void do_drop(P_char ch, char *argument, int cmd)
{
  int      amount, ctype;
  P_obj    tmp_object = NULL, next_obj;
  bool     test = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH],
    Gbuf3[MAX_STRING_LENGTH];

  if( IS_ANIMAL(ch) && IS_NPC(ch) )
    return;

  argument = one_argument(argument, Gbuf1);

#if defined (CTF_MUD) && (CTF_MUD == 1)
  if (!is_number(Gbuf1) &&
      *Gbuf1 && !strcmp(Gbuf1, "flag"))
  {
    // look for flag, if they have it, drop it, otherwise continue.
    if (drop_ctf_flag(ch))
      return;
  }
#endif

  if( is_number(Gbuf1) )
  {
    if (strlen(Gbuf1) > 7)
    {
      send_to_char("Number field too big.\r\n", ch);
      return;
    }
    if( (amount = atoi( Gbuf1 )) <= 0 )
    {
      send_to_char_f(ch, "Eh? What kind of magic is this.. dropping %d coins.\r\n", amount);
      return;
    }

    argument = one_argument(argument, Gbuf1);
    if( (ctype = coin_type( Gbuf1 )) == -1 )
    {
      send_to_char_f( ch, "Drop %d what?  What kind of coins did you want to drop?!\n'%s' is not a valid coin type.\n",
        amount, Gbuf1 );
      return;
    }

    switch (ctype)
    {
    case 0:
      if (GET_COPPER(ch) < amount)
      {
        send_to_char("You do not have that many &+ycopper&N coins!\r\n", ch);
        return;
      }
      break;
    case 1:
      if (GET_SILVER(ch) < amount)
      {
        send_to_char("You do not have that many &+wsilver&n coins!\r\n", ch);
        return;
      }
      break;
    case 2:
      if (GET_GOLD(ch) < amount)
      {
        send_to_char("You do not have that many &+Ygold&N coins!\r\n", ch);
        return;
      }
      break;
    case 3:
      if (GET_PLATINUM(ch) < amount)
      {
        send_to_char("You do not have that many &+Wplatinum&N coins!\r\n", ch);
        return;
      }
      break;
    }

    if (cmd == 1 && IS_PC(ch))
      send_to_char("Oops, trying to juggle too many loose coins, you drop a few.\r\n", ch);
    else
      send_to_char("OK.\r\n", ch);

  if(IS_PC(ch))
  {
    if (((ctype == 3) && (amount > 999)) || ((ctype == 2) && (amount > 99)))
    {
      wizlog(MINLVLIMMORTAL, "%s drops %d %s in [%d]", J_NAME(ch), amount,
             (ctype == 3) ? "plat" : "gold", world[ch->in_room].number);
      logit(LOG_DEBUG, "%s drops %d %s in [%d]", J_NAME(ch), amount,
            (ctype == 3) ? "plat" : "gold", world[ch->in_room].number);
      sql_log(ch, WIZLOG, "Dropped %d %s", amount,
            (ctype == 3) ? "plat" : "gold");
    }
    switch (ctype)
    {
    case 0:
      if (IS_TRUSTED(ch))
      {
        logit(LOG_WIZ, "%s drops %d copper coins [%d]",
              GET_NAME(ch), amount, world[ch->in_room].number);
        sql_log(ch, WIZLOG, "Dropped %d copper coins", amount);
      }
      act("$n drops some &+ycopper&N coins.", FALSE, ch, 0, 0, TO_ROOM);
      tmp_object = create_money(amount, 0, 0, 0);
      GET_COPPER(ch) -= amount;
      break;
    case 1:
      if (IS_TRUSTED(ch))
      {
        logit(LOG_WIZ, "%s drops %d silver coins [%d]",
              GET_NAME(ch), amount, world[ch->in_room].number);
        sql_log(ch, WIZLOG, "Dropped %d silver coins", amount);
      }
      act("$n drops some &+wsilver&n coins.", FALSE, ch, 0, 0, TO_ROOM);
      tmp_object = create_money(0, amount, 0, 0);
      GET_SILVER(ch) -= amount;
      break;
    case 2:
      if (IS_TRUSTED(ch))
      {
        logit(LOG_WIZ, "%s drops %d gold coins [%d]",
              GET_NAME(ch), amount, world[ch->in_room].number);
        sql_log(ch, WIZLOG, "Dropped %d gold coins", amount);
      }
      act("$n drops some &+Ygold&N coins.", FALSE, ch, 0, 0, TO_ROOM);
      tmp_object = create_money(0, 0, amount, 0);
      GET_GOLD(ch) -= amount;
      break;
    case 3:
      if (IS_TRUSTED(ch))
      {
        logit(LOG_WIZ, "%s drops %d platinum coins [%d]",
              GET_NAME(ch), amount, world[ch->in_room].number);
        sql_log(ch, WIZLOG, "Dropped %d platinum coins", amount);
      }
      act("$n drops some &+Wplatinum&N coins.", FALSE, ch, 0, 0, TO_ROOM);
      tmp_object = create_money(0, 0, 0, amount);
      GET_PLATINUM(ch) -= amount;
      break;
    }

    if (tmp_object && (ch->in_room != NOWHERE))
      obj_to_room(tmp_object, ch->in_room);
    else
    {
      logit(LOG_EXIT, "do_drop: no tmp_object or ch in NOWHERE");
      raise(SIGSEGV);
    }
}
    if (IS_PC(ch))
      writeCharacter(ch, 1, ch->in_room);

    return;
  }
  if (*Gbuf1)
  {

    if (sscanf(Gbuf1, "all.%s", Gbuf2) == 1)
    {
      do_dropalldot(ch, Gbuf2, cmd);
      return;
    }
    else if (!str_cmp(Gbuf1, "all"))
    {
      for (tmp_object = ch->carrying; tmp_object; tmp_object = next_obj)
      {
        next_obj = tmp_object->next_content;

        if (!IS_SET(tmp_object->extra_flags, ITEM_NODROP) || IS_TRUSTED(ch))
        {
          if (CAN_SEE_OBJ(ch, tmp_object))
          {
            snprintf(Gbuf3, MAX_STRING_LENGTH, "You drop %s.\r\n", tmp_object->short_description);
            send_to_char(Gbuf3, ch);
          }
          else
          {
            send_to_char("You drop something.\r\n", ch);
          }
          act("$n drops $p.", 1, ch, tmp_object, 0, TO_ROOM);
          obj_from_char(tmp_object);
          if (IS_TRUSTED(ch))
          {
            wizlog(GET_LEVEL(ch), "%s drops %s [%d].",
                   J_NAME(ch), tmp_object->short_description,
                   world[ch->in_room].number);
            logit(LOG_WIZ, "%s drops %s [%d].",
                  J_NAME(ch), tmp_object->short_description,
                  world[ch->in_room].number);
            sql_log(ch, WIZLOG, "Dropped %s", tmp_object->short_description);
          }
          else if( IS_ARTIFACT( tmp_object ) )
          {
            wizlog( 56, "%s dropping artifact %s (%d) in room %d.",
              J_NAME(ch), tmp_object->short_description, obj_index[tmp_object->R_num].virtual_number, world[ch->in_room].number );
            logit(LOG_OBJ, "%s dropping artifact %s (%d) in room %d.",
              J_NAME(ch), tmp_object->short_description, obj_index[tmp_object->R_num].virtual_number, world[ch->in_room].number );
          }
          obj_to_room(tmp_object, ch->in_room);

          if (IS_PC(ch))
            writeCharacter(ch, 1, ch->in_room);

          test = TRUE;
        }
        else
        {
          if (CAN_SEE_OBJ(ch, tmp_object))
          {
            snprintf(Gbuf3, MAX_STRING_LENGTH, "You can't drop %s, it must be CURSED!\r\n",
                    tmp_object->short_description);
            send_to_char(Gbuf3, ch);
            test = TRUE;
          }
        }
        /*
         * update player corpse file (if needed)
         */
        if (tmp_object && (tmp_object->type == ITEM_CORPSE) &&
            IS_SET(tmp_object->value[1], PC_CORPSE))
          writeCorpse(tmp_object);
      }

      if (!test)
      {
        send_to_char("You do not seem to have anything.\r\n", ch);
      }
    }
    else
    {
      tmp_object = get_obj_in_list_vis(ch, Gbuf1, ch->carrying);
      if (tmp_object)
      {
        if (!IS_SET(tmp_object->extra_flags, ITEM_NODROP) || IS_TRUSTED(ch))
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "You drop %s.\r\n", tmp_object->short_description);
          send_to_char(Gbuf3, ch);
          act("$n drops $p.", 0, ch, tmp_object, 0, TO_ROOM);
          obj_from_char(tmp_object);
          if (IS_TRUSTED(ch))
          {
            wizlog(GET_LEVEL(ch), "%s drops %s [%d]",
                   J_NAME(ch), tmp_object->short_description,
                   world[ch->in_room].number);
            logit(LOG_WIZ, "%s drops %s [%d]",
                  J_NAME(ch), tmp_object->short_description,
                  world[ch->in_room].number);
            sql_log(ch, WIZLOG, "Dropped %s", tmp_object->short_description);
          }
          else if( IS_ARTIFACT( tmp_object ) )
          {
            wizlog( 56, "%s dropping artifact %s (%d) in room %d.",
              J_NAME(ch), tmp_object->short_description, obj_index[tmp_object->R_num].virtual_number, world[ch->in_room].number );
            logit(LOG_OBJ, "%s dropping artifact %s (%d) in room %d.",
              J_NAME(ch), tmp_object->short_description, obj_index[tmp_object->R_num].virtual_number, world[ch->in_room].number );
          }
          obj_to_room(tmp_object, ch->in_room);

          if (IS_PC(ch))
            writeCharacter(ch, 1, ch->in_room);

          /*
           * update player corpse file  (if needed)
           */
          if (tmp_object && (tmp_object->type == ITEM_CORPSE) &&
              IS_SET(tmp_object->value[1], PC_CORPSE))
            writeCorpse(tmp_object);

        }
        else
        {
          send_to_char("You can't drop it, it must be CURSED!\r\n", ch);
        }
      }
      else
      {
        send_to_char("You do not have that item.\r\n", ch);
      }
    }
  }
  else
  {
    send_to_char("Drop what?\r\n", ch);
  }
}

#define PUT_COINS  1
#define PUT_ALL    2
#define PUT_ALLDOT 3
#define PUT_ITEM   4

void do_put(P_char ch, char *argument, int cmd)
{
  P_obj    o_obj = NULL, s_obj = NULL, tmp_obj, next_obj;
  P_char   t_ch;
  int      bits, amount, ctype, count = 0, attempted = 0;
  int      plat = 0, gold = 0, silv = 0, copp = 0;
  int      p, g, s, c, type = 0;
  char     buf[MAX_STRING_LENGTH];
  char     obj_name[MAX_STRING_LENGTH];
  char     cont_name[MAX_STRING_LENGTH];

  if( IS_ANIMAL(ch) && IS_NPC(ch) )
  {
    return;
  }

  argument = one_argument(argument, obj_name);

  if( !*obj_name )
  {
    send_to_char("Put what in what?\r\n", ch);
    return;
  }

  if( is_number(obj_name) )
  {
    type = PUT_COINS;
    if (strlen(obj_name) > 7)
    {
      send_to_char("Number field too large.\r\n", ch);
      return;
    }
    amount = atoi(obj_name);
    argument = one_argument(argument, obj_name);
    ctype = coin_type(obj_name);

    if (ctype >= 0 && ch->points.cash[ctype] < amount)
    {
      snprintf(buf, MAX_STRING_LENGTH, "You do not have that many %s coins!\r\n",
              coin_names[ctype]);
      send_to_char(buf, ch);
      return;
    }
    if (ctype == 3)
      plat = amount;
    else if (ctype == 2)
      gold = amount;
    else if (ctype == 1)
      silv = amount;
    else if (ctype == 0)
      copp = amount;
  }
  else if( !str_cmp(obj_name, "all") )
  {
    type = PUT_ALL;
  }
  else if (!strcmp(obj_name, "all.coins"))
  {
    type = PUT_COINS;
    plat = ch->points.cash[3];
    gold = ch->points.cash[2];
    silv = ch->points.cash[1];
    copp = ch->points.cash[0];
  }
  else if( sscanf(obj_name, "all.%s", buf) == 1 )
  {
    strcpy(obj_name, buf);
    type = PUT_ALLDOT;
  }
  else
    type = PUT_ITEM;

  argument = one_argument(argument, cont_name);

  if( !*cont_name )
  {
    snprintf(buf, MAX_STRING_LENGTH, "Put %s in what?\r\n", obj_name);
    send_to_char(buf, ch);
    return;
  }

  if( IS_TRUSTED(ch) )
  {
    bits = generic_find(cont_name, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &t_ch, &s_obj);
  }
  else
  {
    bits = generic_find(cont_name, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &t_ch, &s_obj);
  }

  if (!s_obj)
  {
    send_to_char("Into what?\r\n", ch);
    return;
  }

  if( type == PUT_COINS && (attempted = plat + gold + silv + copp > 0) )
  {
    p = plat;
    g = gold;
    s = silv;
    c = copp;
    for (tmp_obj = s_obj->contains; tmp_obj; tmp_obj = tmp_obj->next_content)
    {
      if (tmp_obj->type == ITEM_MONEY)
      {
        p += tmp_obj->value[3];
        g += tmp_obj->value[2];
        s += tmp_obj->value[1];
        c += tmp_obj->value[0];
        break;
      }
    }
    o_obj = create_money(c, s, g, p);
    obj_to_char(o_obj, ch);
    if ((count = put(ch, o_obj, s_obj, TRUE)))
    {
      snprintf(buf, MAX_STRING_LENGTH, "You put %d &+Wplatinum&n, %d &+Ygold&n, %d silver, and %d &+ycopper&n coins into $P.",
        plat, gold, silv, copp);
      act(buf, TRUE, ch, 0, s_obj, TO_CHAR);
      ch->points.cash[3] -= plat;
      ch->points.cash[2] -= gold;
      ch->points.cash[1] -= silv;
      ch->points.cash[0] -= copp;
      if (tmp_obj)
      {
        extract_obj(tmp_obj);
      }
    }
    else
    {
      extract_obj(o_obj);
    }
  }

  if (type == PUT_ITEM)
  {
    attempted = generic_find(obj_name, FIND_OBJ_INV, ch, &t_ch, &o_obj);
    if( attempted != 0 )
    {
      count = put(ch, o_obj, s_obj, TRUE);
    }
  }
  else if (type == PUT_ALL || type == PUT_ALLDOT)
  {
    for (o_obj = ch->carrying; o_obj; o_obj = next_obj)
    {
      next_obj = o_obj->next_content;
      if (o_obj == s_obj)
        continue;
      if (!CAN_SEE_OBJ(ch, o_obj) && type != PUT_ALL)
        continue;
      if (type == PUT_ALLDOT && !isname(obj_name, o_obj->name))
        continue;
      attempted = 1;
      if( put(ch, o_obj, s_obj, FALSE) )
        count++;
    }
  }
  if( attempted == 0 )
  {
    if (type != PUT_ITEM)
    {
      send_to_char("You don't have anything to put in it.\r\n", ch);
    }
    else
    {
      snprintf(buf, MAX_STRING_LENGTH, "You don't have the %s.\r\n", obj_name);
      send_to_char(buf, ch);
    }
  }
  else if (count)
  {
    if (type == PUT_ALL)
    {
      snprintf(buf, MAX_STRING_LENGTH, "You put %d items into $p.", count);
      act(buf, FALSE, ch, s_obj, 0, TO_CHAR);
      if (count < 6)
        act("$n puts some stuff into $p.", TRUE, ch, s_obj, 0, TO_ROOM);
      else
        act("$n puts a bunch of stuff into $p.", TRUE, ch, s_obj, 0, TO_ROOM);
    }
    else if (type == PUT_ALLDOT)
    {
      snprintf(buf, MAX_STRING_LENGTH, "You put %d %s(s) into $p.", count, obj_name);
      act(buf, FALSE, ch, s_obj, 0, TO_CHAR);
      if (count < 6)
        snprintf(buf, MAX_STRING_LENGTH, "$n puts some %s(s) into $p.", obj_name);
      else
        snprintf(buf, MAX_STRING_LENGTH, "$n puts a bunch of %s(s) into $p.", obj_name);
      act(buf, TRUE, ch, s_obj, 0, TO_ROOM);
    }
    char_light(ch);
    room_light(ch->in_room, REAL);
    if (IS_PC(ch))
      writeCharacter(ch, 1, ch->in_room);
    if (GET_ITEM_TYPE(s_obj) == ITEM_STORAGE)
      writeSavedItem(s_obj);
  }
}

#undef PUT_COINS
#undef PUT_ALL
#undef PUT_ALLDOT
#undef PUT_ITEM

bool put(P_char ch, P_obj o_obj, P_obj s_obj, int showit)
{
  char     Gbuf3[MAX_STRING_LENGTH];


  if (IS_ARTIFACT(o_obj) && !IS_TRUSTED(ch))
  {
    if (showit)
      act("$p does not wish to be confined in such a manner!", TRUE, ch,
          o_obj, 0, TO_CHAR);

    return FALSE;
  }

  if (o_obj)
    /* Trap check */
    /*
       if (checkgetput(ch, o_obj))
       return FALSE;
     */

    if (s_obj->type == ITEM_QUIVER)
    {
      if (!IS_SET(s_obj->value[1], CONT_CLOSED))
      {
        if (o_obj == s_obj)
        {
          if (showit)
            send_to_char("You can't put one quiver inside another.\r\n", ch);
          return (FALSE);
        }
        if (IS_SET(o_obj->extra_flags, ITEM_NODROP) && !IS_TRUSTED(ch))
        {
          if (showit)
            send_to_char
              ("You can't do that. Perhaps that item is cursed?\r\n", ch);
          return (FALSE);
        }
        if ((o_obj->type != ITEM_MISSILE) ||
            ((o_obj->type == ITEM_MISSILE) &&
             (s_obj->value[2] != o_obj->value[3])))
        {
          if (showit)
            send_to_char
              ("You cannot put that in a quiver, only arrows and quarrels.\r\n",
               ch);
          return (FALSE);
        }
        if (s_obj->value[0] > s_obj->value[3])
        {
          if (showit)
            send_to_char("Ok.\r\n", ch);
          if (OBJ_CARRIED(o_obj))
          {
            /*
             * ok, obj_from_char subtracts the objs weight from
             * what is being carried, if container is in room this
             * is fine, if container is in the player's inv, then
             * we have to add the obj's weight back.  Old method
             * didn't account for putting objects into containers
             * that WERE NOT in inven, thus placing an item in a
             * container you weren't holding, left you still
             * carrying the objects weight, this is most likely a
             * really ancient bug -JAB
             */
            obj_from_char(o_obj);
            obj_to_obj(o_obj, s_obj);
            s_obj->value[3]++;
          }
          else
          {
            obj_from_room(o_obj);
            /*
             * Do we need obj_from_room???(s_obj, ....);
             */
            obj_to_obj(o_obj, s_obj);
            /*
             * Do we need obj_to_room???(s_obj, ch);
             */
          }
          if (IS_TRUSTED(ch))
          {
            wizlog(GET_LEVEL(ch), "%s puts %s in %s [%d]",
                   J_NAME(ch), o_obj->short_description,
                   s_obj->short_description, world[ch->in_room].number);
            logit(LOG_WIZ, "%s puts %s in %s [%d]",
                   J_NAME(ch), o_obj->short_description,
                   s_obj->short_description, world[ch->in_room].number);
            sql_log(ch, WIZLOG, "Put %s in %s", o_obj->short_description, s_obj->short_description);            
          }

          if (IS_PC(ch))
            writeCharacter(ch, 1, ch->in_room);

          if (GET_ITEM_TYPE(o_obj) == ITEM_STORAGE)
            writeSavedItem(o_obj);
          if (showit)
            act("$n puts $p into $P.", TRUE, ch, o_obj, s_obj, TO_ROOM);
          char_light(ch);
          room_light(ch->in_room, REAL);
          return (TRUE);
        }
        else
        {
          if (showit)
            send_to_char("The quiver is full.\r\n", ch);
        }
      }
      else if (showit)
        send_to_char("It seems to be closed.\r\n", ch);
    }
    else if (GET_ITEM_TYPE(s_obj) == ITEM_CONTAINER ||
             GET_ITEM_TYPE(s_obj) == ITEM_STORAGE ||
             GET_ITEM_TYPE(s_obj) == ITEM_CORPSE)
    {
      if (!IS_SET(s_obj->value[1], CONT_CLOSED))
      {
        if (o_obj == s_obj)
        {
          if (showit)
            send_to_char("You try to fold it up, but fail.\r\n", ch);
          return (FALSE);
        }
        if (IS_SET(o_obj->extra_flags, ITEM_NODROP) && !IS_TRUSTED(ch))
        {
          if (showit)
            send_to_char
              ("You can't do that. Perhaps that item is cursed?\r\n", ch);
          return (FALSE);
        }

        if (((GET_OBJ_WEIGHT(s_obj) + GET_OBJ_WEIGHT(o_obj)) <=
             (s_obj->value[0])) || ((s_obj->value[0] == -1) &&
                                    (GET_ITEM_TYPE(s_obj) == ITEM_STORAGE ||
                                     GET_ITEM_TYPE(s_obj) == ITEM_CONTAINER)))
        {

#if USE_SPACE
          if (((GET_OBJ_SPACE(s_obj) + GET_OBJ_SPACE(o_obj)) <=
               (s_obj->value[3])) || ((s_obj->space == -1) &&
                                      (GET_ITEM_TYPE(s_obj) == ITEM_STORAGE ||
                                       GET_ITEM_TYPE(s_obj) ==
                                       ITEM_CONTAINER)))
          {
#endif
            if (showit)
              send_to_char("Ok.\r\n", ch);
            if (OBJ_CARRIED(o_obj))
            {
              /*
               * ok, obj_from_char subtracts the objs weight from
               * what is being carried, if container is in room this
               * is fine, if container is in the player's inv, then
               * we have to add the obj's weight back. Old method
               * didn't account for putting objects into containers
               * that WERE NOT in inven, thus placing an item in a
               * container you weren't holding, left you still
               * carrying the objects weight, this is most likely a
               * really ancient bug -JAB
               */
              obj_from_char(o_obj);
              obj_to_obj(o_obj, s_obj);
#if USE_SPACE
              s_obj->space += GET_OBJ_SPACE(o_obj);
#endif
            }
            else
            {
              /*
               * this is never used I don't think, can't put from
               * room to a container.  JAB
               */
              obj_from_room(o_obj);
              obj_to_obj(o_obj, s_obj);
            }

            if (IS_TRUSTED(ch))
            {
              wizlog(GET_LEVEL(ch), "%s puts %s in %s [%d]",
                     J_NAME(ch), o_obj->short_description,
                     s_obj->short_description, world[ch->in_room].number);
              logit(LOG_WIZ, "%s puts %s in %s [%d]",
                     J_NAME(ch), o_obj->short_description,
                     s_obj->short_description, world[ch->in_room].number);
              sql_log(ch, WIZLOG, "Put %s in %s", o_obj->short_description, s_obj->short_description);
            }
            if (showit)
              act("$n puts $p into $P.", TRUE, ch, o_obj, s_obj, TO_ROOM);
            char_light(ch);
            room_light(ch->in_room, REAL);

            if (GET_ITEM_TYPE(o_obj) == ITEM_STORAGE)
              if (IS_PC(ch))
                writeCharacter(ch, 1, ch->in_room);

            return (TRUE);
#if USE_SPACE
          }
          else
          {
            if (showit)
              send_to_char("Not enough place left to fit in.\r\n", ch);
          }
#endif
        }
        else
        {
          if (showit)
            send_to_char("It won't fit.\r\n", ch);
        }
      }
      else if (showit)
        send_to_char("It seems to be closed.\r\n", ch);
    }
    else
    {
      if (showit)
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "The %s is not a container.\r\n",
                FirstWord(s_obj->name));
        send_to_char(Gbuf3, ch);
      }
    }
  /*
   * added by DTS 5/18/95 to solve light bug
   */
  char_light(ch);
  room_light(ch->in_room, REAL);
  return (FALSE);
}

void do_give(P_char ch, char *argument, int cmd)
{
  char     obj_name[MAX_INPUT_LENGTH], vict_name[MAX_INPUT_LENGTH];
  char     arg[MAX_INPUT_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  int      amount, ctype;
  P_char   vict, rider;
  P_obj    obj;

/*  struct affected_type af;*/

  argument = one_argument(argument, obj_name);

  if (is_number(obj_name))
  {
    if (strlen(obj_name) > 7)
    {
      send_to_char("Number field too large.\r\n", ch);
      return;
    }
    amount = atoi(obj_name);
    argument = one_argument(argument, arg);

    ctype = coin_type(arg);

    if ((ctype == -1) || (amount <= 0))
    {
      send_to_char("Sorry, you can't do that!\r\n", ch);
      if( amount <= 0 )
        wizlog(57, "&-L&+R%s just tried to give %d %s in room %d!", GET_NAME(ch), amount,
          (ctype == 3) ? "plat" : (ctype == 2) ? "gold" : (ctype == 1) ? "silver" : "copper",
          world[ch->in_room].number);
      return;
    }
    if ((ch->points.cash[ctype] < amount) &&
        (IS_NPC(ch) || (GET_LEVEL(ch) < MAXLVL)))
    {
      snprintf(Gbuf1, MAX_STRING_LENGTH, "You do not have that many %s coins!\r\n", coin_names[ctype]);
      send_to_char(Gbuf1, ch);
      return;
    }
    argument = one_argument(argument, vict_name);
    if (!*vict_name)
    {
      send_to_char("To who?\r\n", ch);
      return;
    }
    if (!(vict = get_char_room_vis(ch, vict_name)))
    {
      send_to_char("To who?\r\n", ch);
      return;
    }

    if( racewar(ch, vict) )
    {
      send_to_char("Hey now, why would you want to do that?\r\n", ch);
      return;
    }

    if ((IS_NPC(vict) && ((GET_RNUM(vict) == real_mobile(250)) ||
      (GET_RNUM(vict) == real_mobile(650)))) || IS_AFFECTED(vict, AFF_WRAITHFORM))
    {
      send_to_char("They couldn't carry that if they tried.\r\n", ch);
      return;
    }


    send_to_char("Ok.\r\n", ch);

    if (((CAN_CARRY_COINS(vict, rider) < amount) || (amount > 1000)) &&
        !is_linked_to(ch, vict, LNK_CONSENT) && (!IS_TRUSTED(vict)) &&
        (cmd != -4) && !IS_TRUSTED(ch))
    {
      act("$E must consent to you before you can overload $M.", 0, ch, 0, vict, TO_CHAR);
      return;
    }
    if (((ctype == 3) && (amount > 999)) || ((ctype == 2) && (amount > 99)))
    {
      wizlog(56, "%s gives %s %d %s in [%d]", J_NAME(ch), J_NAME(vict),
             amount, (ctype == 3) ? "plat" : "gold",
             world[ch->in_room].number);
      logit(LOG_DEBUG, "%s gives %s %d %s in [%d]", J_NAME(ch), J_NAME(vict),
            amount, (ctype == 3) ? "plat" : "gold",
            world[ch->in_room].number);
    }
    if (IS_TRUSTED(ch))
    {
      wizlog(GET_LEVEL(ch), "%s gives %s %d %s coins.",
             J_NAME(ch), J_NAME(vict), amount, coin_names[ctype]);
      logit(LOG_WIZ, "%s gives %s %d %s coins.",
            J_NAME(ch), J_NAME(vict), amount, coin_names[ctype]);
      sql_log(ch, WIZLOG, "Gave %s %d %s coins.", J_NAME(vict), amount, coin_names[ctype]);
    }
    snprintf(Gbuf1, MAX_STRING_LENGTH, "%s gives you %d %s coins.\r\n",
            PERS(ch, vict, FALSE), amount, coin_names[ctype]);
    send_to_char(Gbuf1, vict);
    snprintf(Gbuf1, MAX_STRING_LENGTH, "$n gives some %s to $N", coin_names[ctype]);
    act(Gbuf1, TRUE, ch, 0, vict, TO_NOTVICT);

    if (IS_NPC(ch) || (GET_LEVEL(ch) < MAXLVL))
      ch->points.cash[ctype] -= amount;
    vict->points.cash[ctype] += amount;

    if (ch != vict)
    {
      writeCharacter(ch, 1, ch->in_room);
      writeCharacter(vict, 1, vict->in_room);
    }
    /*
     * added by DTS 5/18/95 to solve light bug
     */
    char_light(ch);
    room_light(ch->in_room, REAL);
    return;
  }
  argument = one_argument(argument, vict_name);

  if (!*obj_name || !*vict_name)
  {
    send_to_char("Give what to who?\r\n", ch);
    return;
  }
  if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying)))
  {
    send_to_char("You do not seem to have anything like that.\r\n", ch);
    return;
  }

  if (IS_SET(obj->extra_flags, ITEM_NODROP) && !IS_TRUSTED(ch))
  {
    send_to_char("You can't let go of it! Yeech!!\r\n", ch);
    return;
  }
  //prevent soulbind quest items - Drannak
  if( IS_OBJ_STAT2(obj, ITEM2_SOULBIND))
  {
    send_to_char("You may not relinquish posession of a &+Wsoulbound &nitem!\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, vict_name)))
  {
    send_to_char("No one by that name around here.\r\n", ch);
    return;
  }
  if(IS_NPC(ch) && !IS_SET(obj->wear_flags, ITEM_TAKE))
  {
    /* prevent pcs from getting !take items from raised corpses */
    return;
  }
  if (IS_PC(vict) && IS_PC(ch) && !IS_TRUSTED(ch) &&
      IS_SET(vict->specials.act2, PLR2_NOTAKE))
  {
    act("$N rejects your offering.", TRUE, ch, 0, vict, TO_CHAR);
    return;
  }
  if(IS_SET(obj->extra2_flags, ITEM2_CRAFTED) && IS_NPC(vict))
  {
    send_to_char("You may not give crafted/forged items to mobs.\r\n", ch);
    return;
  }
  if ((IS_NPC(vict) && (GET_RNUM(vict) == real_mobile(250))) ||
      IS_AFFECTED(vict, AFF_WRAITHFORM))
  {
    send_to_char("They couldn't carry that if they tried.\r\n", ch);
    return;
  }
  if (!IS_TRUSTED(ch) && IS_CARRYING_N(vict) >= CAN_CARRY_N(vict) &&
      !(IS_NPC(vict) && mob_index[GET_RNUM(vict)].qst_func))
  {
    act("$N seems to have $S hands full.", 0, ch, 0, vict, TO_CHAR);
    return;
  }
  if (((((GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict, rider)) > CAN_CARRY_W(vict)) ||
        (GET_OBJ_WEIGHT(obj) > 25)) && !is_linked_to(ch, vict, LNK_CONSENT))
      && (cmd != -4) && (!IS_TRUSTED(ch)))
  {
    act("$E must consent to you before you can overload $M.", 0, ch, 0, vict,
        TO_CHAR);
    return;
  }
  if (IS_ARTIFACT(obj) && racewar(ch, vict) )
  {
    send_to_char("That would just be unethical now wouldn't it?\r\n", ch);
    wizlog( 56, "%s tried to give %s to %s.", ch->player.name, obj->short_description, vict->player.name );
    return;
  }
  obj_from_char(obj);
  act("$n gives $p to $N.", 1, ch, obj, vict, TO_NOTVICT);
  act("$n gives you $p.", 0, ch, obj, vict, TO_VICT);
  send_to_char("Ok.\r\n", ch);
  obj_to_char(obj, vict);
  if (IS_TRUSTED(ch))
  {
    if (IS_ARTIFACT(obj) )
      logit(LOG_OBJ, "%s gives artifact %s (%d) to %s.",
        J_NAME(ch), obj->short_description, obj_index[obj->R_num].virtual_number, J_NAME(vict) );
    wizlog(GET_LEVEL(ch), "%s gives %s to %s.",
           J_NAME(ch), obj->short_description, J_NAME(vict));
    logit(LOG_WIZ, "%s gives %s to %s.",
          J_NAME(ch), obj->short_description, J_NAME(vict));
    sql_log(ch, WIZLOG, "Gave %s to %s.", obj->short_description, J_NAME(vict));
  }
  if (ch != vict)
  {
    writeCharacter(ch, 1, ch->in_room);
    writeCharacter(vict, 1, vict->in_room);
#ifndef __NO_MYSQL__
    artifact_switch_check(ch, obj);
#endif
  }
  /*
   * added by DTS 5/18/95 to solve light bug
   */
  char_light(ch);
  room_light(ch->in_room, REAL);
  nq_action_check(ch, vict, NULL);
}

void weight_change_object(P_obj obj, int weight)
{
  P_obj    tmp_obj;
  P_char   tmp_ch;
  int      pos;

  if (OBJ_ROOM(obj))
  {
    obj->weight += weight;
  }
  else if (OBJ_CARRIED(obj))
  {
    tmp_ch = obj->loc.carrying;
    obj_from_char(obj);
    obj->weight += weight;
    obj_to_char(obj, tmp_ch);
  }
  else if (OBJ_WORN(obj))
  {
    tmp_ch = obj->loc.wearing;
    for (pos = 0; pos < MAX_WEAR; pos++)
      if (tmp_ch->equipment[pos] == obj)
        break;
    if (pos >= MAX_WEAR)
    {
      logit(LOG_EXIT,
            "weight_change_object, can't find worn object in equip");
      raise(SIGSEGV);
    }
    unequip_char(tmp_ch, pos);
    obj->weight += weight;
    equip_char(tmp_ch, obj, pos, TRUE);
  }
  else if (OBJ_INSIDE(obj))
  {
    tmp_obj = obj->loc.inside;
    obj_from_obj(obj);
    obj->weight += weight;
    obj_to_obj(obj, tmp_obj);
  }
  else
  {
    logit(LOG_DEBUG, "Unknown attempt to subtract weight from an object.");
  }
}

void name_from_drinkcon(P_obj obj)
{
  int      i;
  char    *new_name;

  /*
   * FIRST, check to see if the obj even has the name of drink... if
   * not, don't remove it!
   */

  for (i = 0; i <= LIQ_LAST_ONE; i++)
    if (isname(drinks[i], obj->name))
      break;

  if (i > LIQ_LAST_ONE)         /* doesn't have the name of a drink.. just return */
    return;

  /*
   * okay.. the new name will be all the chars past the drinkname
   */

  new_name = str_dup((obj->name) + strlen(drinks[i]) + 1);

  /*
   * free any old name...
   */
  if ((obj->str_mask & STRUNG_KEYS) && obj->name)
    str_free(obj->name);

  /*
   * ... and assign the new one
   */

  obj->str_mask |= STRUNG_KEYS;
  obj->name = new_name;
}

void name_to_drinkcon(P_obj obj, int type)
{
  char    *new_name;

  /* don't add a new one if builder has done so */
  if (isname(drinks[type], obj->name))
    return;

  /* clear any old drink name attached to the object.  this will
     prevent object names like "water water flagon", which waste
     space, and look ugly as shit  */

  name_from_drinkcon(obj);

  CREATE(new_name, char, strlen(obj->name) + strlen(drinks[type]) + 2, MEM_TAG_STRING);

  snprintf(new_name, MAX_STRING_LENGTH, "%s %s", drinks[type], obj->name);

  if ((obj->str_mask & STRUNG_KEYS) && obj->name)
    str_free(obj->name);

  obj->str_mask |= STRUNG_KEYS;
  obj->name = new_name;
}

/*
 * Improved version of "drink".  Empty transient object should now
 * vanish.  You can drink from fountain and other such places.
 */
void do_drink(P_char ch, char *argument, int cmd)
{
  P_obj    temp;
  int      amount, healamt;
  char     Gbuf4[MAX_STRING_LENGTH];
  int      own_object;          /*
                                 * Boolean flag used to determine
                                 * whether to drop transient obj
                                 */
  if (GET_RACE(ch) == RACE_ILLITHID && GET_LEVEL(ch) < AVATAR)
  {
    send_to_char
      ("Ugh. Even if you had the means to drink, the thought revolts you.\r\n",
       ch);
    return;
  }

  /* procs will still work regardless of this.. */
  //send_to_char("You fill your mouth, but are unable to swallow!\r\n", ch);
  //return;

  /*
   * added by DTS 5/26/95 to prevent pets healing by drinking holy water
   */
  if (IS_NPC(ch))
  {
    send_to_char("Monsters don't need to drink!\n", ch);
    return;
  }
  one_argument(argument, Gbuf4);

  if (!(temp = get_obj_in_list_vis(ch, Gbuf4, ch->carrying)))
  {
    if (!(temp = get_obj_in_list_vis(ch, Gbuf4, world[ch->in_room].contents)))
    {
      act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    else
    {

      /* Need to set boolean value own_object to 0 so that we will not   */
      /* attempt to drop TRANSIENT object when it is not in the player's */
      /* inventory.     */

      own_object = 0;
    }
  }
  else
  {
    own_object = 1;
  }

  if (temp->type != ITEM_DRINKCON)
  {
    act("You can't drink from that!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0))
  {
    /* The pig is drunk */
    act("You simply fail to reach your mouth!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n tried to drink but missed $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  if (GET_COND(ch, THIRST) > 23)
  {                             /*
                                 * Stomach full
                                 */
    if (GET_COND(ch, THIRST) == -1)     // -Foo Disable thirst
    {
      act("You feel like your bladder will burst soon!",
          FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if ((GET_COND(ch, FULL) > 23) && (GET_COND(ch, THIRST) > 0))
  {
    /*
     * Stomach full
     */
    act("Your stomach can't contain anymore!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

/*
  send_to_char("Why bother?  The idea of 'drinking' repulses you.\r\n", ch);
  return;
*/
//  CharWait(ch, PULSE_VIOLENCE);

  if (temp->type == ITEM_DRINKCON)
  {
    if (temp->value[1])
    {                           /* Not empty */
      snprintf(Gbuf4, MAX_STRING_LENGTH, "$n drinks %s from $p.", drinks[temp->value[2]]);
      act(Gbuf4, TRUE, ch, temp, 0, TO_ROOM);
      snprintf(Gbuf4, MAX_STRING_LENGTH, "You drink the %s from $p.", drinks[temp->value[2]]);
      act(Gbuf4, TRUE, ch, temp, 0, TO_CHAR);

      amount = 1;
      if (GET_COND(ch, THIRST) < 10)
        amount++;

      if (drink_aff[temp->value[2]][DRUNK] > 0)
        amount += number(1, 2);

      if (temp->value[1] > 0)
        amount = MIN(amount, temp->value[1]);

      if (temp->value[1] > 0)
        weight_change_object(temp, -amount);    /* Subtract amount */

      if (gain_condition(ch, DRUNK,
                         (int) (drink_aff[temp->value[2]][DRUNK] * amount)))
        return;
      if (gain_condition(ch, FULL,
                         (int) (drink_aff[temp->value[2]][FULL] * amount)))
        return;
      if (gain_condition(ch, THIRST,
                         (int) (drink_aff[temp->value[2]][THIRST] * amount)))
        return;

      if (GET_COND(ch, DRUNK) > 10)
        act("You feel drunk.", FALSE, ch, 0, 0, TO_CHAR);

      if (GET_COND(ch, THIRST) > 20)
        act("You do not feel &+cth&+Ci&+cr&+Cst&+cy&n.", FALSE, ch, 0, 0, TO_CHAR);

      if (GET_COND(ch, FULL) > 20)
        act("You are full.", FALSE, ch, 0, 0, TO_CHAR);

      /* Condensed from 30 Lines to 12 by refactoring the logic. - Sniktiorg (Nov.9.12) */
      if ((temp->value[2] == LIQ_HOLYWATER && IS_GOOD(ch)) || (temp->value[2] == LIQ_UNHOLYWAT && IS_EVIL(ch)))
      {
        healamt = MIN(GET_MAX_HIT(ch) - GET_HIT(ch), dice(3, 3));

        send_to_char("You feel &+Wt&+wou&+Wc&+wh&+Wed&n by a higher power!\r\n", ch);
        GET_HIT(ch) += healamt;
        healCondition(ch, healamt);
        CharWait( ch, WAIT_SEC );
      }
      else if ((temp->value[2] == LIQ_UNHOLYWAT && IS_GOOD(ch)) || (temp->value[2] == LIQ_HOLYWATER && IS_EVIL(ch)))
      {
        send_to_char("You are &+rbl&+Ra&+rs&+Rte&+rd&n by a higher power!\r\n", ch);
        GET_HIT(ch) = MAX(0, GET_HIT(ch) - dice(3, 3));
      }
      else if (temp->value[3])
      {                         /*
                                 * The shit was poisoned !
                                 */
        act("Oops, it tasted rather strange?!!?", FALSE, ch, 0, 0, TO_CHAR);
        act("$n chokes and utters some strange sounds.",
            TRUE, ch, 0, 0, TO_ROOM);
        poison_lifeleak(10, ch, 0, 0, ch, 0);
      }
      if (temp->value[1] < 0)
        return;

      temp->value[1] -= amount;

      /* empty the container, and no longer poison. */
      if (!temp->value[1])
      {                         /* The last bit */
        temp->value[2] = 0;
        temp->value[3] = 0;
        name_from_drinkcon(temp);
      }
      /* Check to see if object is of type TRANSIENT and empty */
      /* If it is .. it needs to vanish.  The way to do that is */
      /* to force player to drop object */

      if (temp->value[1] <= 0 && IS_SET(temp->extra_flags, ITEM_TRANSIENT) && own_object)
      {

        act("The empty $q vanishes into thin air.\r\n", TRUE, ch, temp, 0, TO_CHAR);
        obj_from_char(temp);
        extract_obj(temp, TRUE); // Hmm, a transient drink container artifact?
      }
      return;
    }
  }
  /*
   * Only reach here if object is already empty
   */

  act("It's empty already.", FALSE, ch, 0, 0, TO_CHAR);
}

void do_eat(P_char ch, char *argument, int cmd)
{
  P_obj    temp;
  char     Gbuf1[MAX_STRING_LENGTH];
  bool     updateArtiList;

  argument = one_argument(argument, Gbuf1);

  if (GET_RACE(ch) == RACE_ILLITHID && !IS_TRUSTED(ch) )
  {
    send_to_char("Ugh. Even if you had the means to eat, the thought revolts you.\r\n", ch);
    return;
  }

  // NPCs don't eat for now.
  if( IS_NPC(ch) )
  {
    return;
  }

  if (!(temp = get_obj_in_list_vis(ch, Gbuf1, ch->carrying)))
  {
    act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  updateArtiList = FALSE;
  // We need some sort of confirmation as to what to do here.
  if( IS_ARTIFACT(temp) && IS_TRUSTED(ch) )
  {
    argument = skip_spaces(argument);
    if( !strcmp(argument, "update") )
    {
      updateArtiList = TRUE;
    }
    else
    {
      send_to_char_f( ch, "&+YWhen eating an artifact, you must specify '&+weat <arti> update&+Y' if you"
        " want to update the artifact list.  If you don't know whether to update, you're better off"
        " handing it to another Immortal who knows or can figure it out.  The reason for this is to"
        " figure out if the artifact is duplicated or if it's being pulled from a char for some"
        " reason, etc.  However, since your hungry, we just won't update it this time.  You can use"
        " '&+wartifact clear %d&+Y' to clear the DB entry if need be.&n\n\r", OBJ_VNUM(temp) );
    }
  }

  if ((temp->type != ITEM_FOOD) && (GET_LEVEL(ch) < AVATAR))
  {
    /*
     * if (GET_ITEM_TYPE(temp) == ITEM_CONTAINER && temp->value[3]) {
     * send_to_char("Mmm, tastes just like very rare steak!\r\n", ch);
     * act("$n savagely devours the corpse.", FALSE, ch, 0, 0,
     * TO_ROOM); return; } else {
     */
    act("That's not very edible, I'm afraid.", FALSE, ch, 0, 0, TO_CHAR);
    return;
    /*
     * }
     */
  }

  /*
     if ((GET_COND(ch, FULL) > 20) && !IS_AFFECTED3(ch, AFF3_FAMINE))
     {
     act("No thanks, I'm absolutely stuffed, couldn't eat another bite.",
     FALSE, ch, 0, 0, TO_CHAR);
     return;
     }
   */
  if (affected_by_spell(ch, TAG_EATEN) && !IS_TRUSTED(ch))
  {
    act("You feel sated already.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if( (temp->value[1] < 0 || (temp->timer[0] && (time(NULL) - temp->timer[0] > 1 * 60 * 10)) ) && !IS_TRUSTED(ch) )
  {
    act("That stinks, find some fresh food instead.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  /* special handling: value[5] specifies special functions for epic food */
  int oaffect;
  oaffect = (temp->value[5]);
  if(oaffect > 0)
  {
    // What slacker did this instead of writing a real object proc that captures CMD_EAT?
    if(oaffect == 1337) //+1 level mushroom
    {
      if( (GET_LEVEL(ch) > 45) || (GET_RACE(ch) == RACE_LICH) )
      {
        send_to_char("&+GYou are much too powerful for the magic of this item&n.\r\n", ch);
        return;
      }
      send_to_char("&+gAs you eat the &+GMushroom&+g, a &+Mmagical&+g essence surrounds you and you suddenly feel more &+Gexperienced!&n\r\n", ch);
      // GET_EXP(ch) = new_exp_table[GET_LEVEL(ch)];
      statuslog(ch->player.level, "&+CLevel:&n (%s&n) just ate level mushroom at [%d]!",
        GET_NAME(ch), (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);
      advance_level(ch);
      do_save_silent(ch, 1);
      extract_obj(temp);
      return;
    }
  }

  act("$n eats $p.", TRUE, ch, temp, 0, TO_ROOM);
  act("You eat the $q.", FALSE, ch, temp, 0, TO_CHAR);

  if (temp->type == ITEM_FOOD)
  {
    /* New code to grant reg from food */
    struct affected_type af;
    if (!affected_by_spell(ch, TAG_EATEN))
    {
      bzero(&af, sizeof(af));
      af.type = TAG_EATEN;
      af.flags = AFFTYPE_NOSHOW;
      af.duration = 1 + (temp->value[0] > 0) ? temp->value[0] : 1;

      int hit_reg;
      int mov_reg;
      if (temp->value[3] > 0) // TODO: apply poison
      {
          act("You feel &+gs&+Gi&+gc&+Gk&n.", FALSE, ch, 0, 0, TO_CHAR);
          hit_reg = -temp->value[3] - hit_regen(ch, TRUE);
          mov_reg = 0;
      }
      else
      {
        hit_reg = 15;
        if (temp->value[1] != 0)
          hit_reg = temp->value[1] * 15;
        if (temp->value[2] != 0)
          mov_reg = temp->value[2];
        else
          mov_reg = hit_reg;
      }

      af.location = APPLY_HIT_REG;
      af.modifier = hit_reg;
      affect_to_char(ch, &af);

      if( mov_reg != 0 )
      {
        af.location = APPLY_MOVE_REG;
        af.modifier = mov_reg;
        affect_to_char(ch, &af);
      }

      if( (af.modifier = temp->value[4]) != 0 )
      {
        af.location = APPLY_STR;
        affect_to_char(ch, &af);
        af.location = APPLY_CON;
        affect_to_char(ch, &af);
      }

      if( (af.modifier = temp->value[5]) != 0 )
      {
        af.location = APPLY_AGI;
        affect_to_char(ch, &af);
        af.location = APPLY_DEX;
        affect_to_char(ch, &af);
      }

      if( (af.modifier = temp->value[6]) != 0 )
      {
        af.location = APPLY_INT;
        affect_to_char(ch, &af);
        af.location = APPLY_WIS;
        affect_to_char(ch, &af);
      }

      if( (af.modifier = temp->value[7]) != 0 )
      {
        af.location = APPLY_DAMROLL;
        affect_to_char(ch, &af);
        af.location = APPLY_HITROLL;
        affect_to_char(ch, &af);
      }
    }
    else
    {
      act("You feel sated already.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }


    /* End new code to grant reg from eating */
    /*
       if (gain_condition(ch, FULL, temp->value[0]))
       return;

       if (GET_COND(ch, FULL) > 20)
       act("You feel comfortably sated.", FALSE, ch, 0, 0, TO_CHAR);

       if (temp->value[3] && (GET_LEVEL(ch) < MINLVLIMMORTAL))
       {
         act("Oops, it tasted rather strange?!!?", FALSE, ch, 0, 0, TO_CHAR);
         act("$n coughs and utters some strange sounds.",
         FALSE, ch, 0, 0, TO_ROOM);
         poison_lifeleak(10, ch, 0, 0, ch, 0);
       }
     */
  }

  if( updateArtiList )
  {
    snprintf(Gbuf1, MAX_STRING_LENGTH, "%d", OBJ_VNUM(temp) );
    arti_clear_sql( ch, Gbuf1 );
  }
  extract_obj(temp);
  // Added by DTS 5/18/95 to solve light bug
  char_light(ch);
  room_light(ch->in_room, REAL);
}

void do_pour(P_char ch, char *argument, int cmd)
{
  P_obj    from_obj;
  P_obj    to_obj;
  P_char   to_char;
  int      amount;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf4[MAX_STRING_LENGTH];

  argument_interpreter(argument, Gbuf1, Gbuf2);

  if (!*Gbuf1)
  {                             /* No arguments */
    act("What do you want to pour from?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!(from_obj = get_obj_in_list_vis(ch, Gbuf1, ch->carrying)))
  {
    act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (from_obj->type != ITEM_DRINKCON)
  {
    act("You can't pour from that!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (from_obj->value[1] == 0)
  {
    act("The $q is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (from_obj->value[1] < 0)
  {
    act
      ("You can't seem to pour $p out completely!  There's still more there!",
       FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (!*Gbuf2)
  {
    act("Where do you want it? Out or in what?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!str_cmp(Gbuf2, "out"))
  {
    act("$n empties $s $q.", TRUE, ch, from_obj, 0, TO_ROOM);
    act("You empty the $q.", FALSE, ch, from_obj, 0, TO_CHAR);
    weight_change_object(from_obj, -from_obj->value[1]);        /* Empty */
    from_obj->value[1] = 0;
    from_obj->value[2] = 0;
    from_obj->value[3] = 0;
    name_from_drinkcon(from_obj);
    return;
  }
  else if (!str_cmp(Gbuf2, "half"))
  {
    act("$n pours some liquid out of $s $q.", TRUE, ch, from_obj, 0, TO_ROOM);
    act("You partially empty the $q.", FALSE, ch, from_obj, 0, TO_CHAR);
    weight_change_object(from_obj, from_obj->value[1] / 2);
    from_obj->value[1] = from_obj->value[1] / 2;
    return;
  }
  else if ((to_char = get_char_vis(ch, Gbuf2)))
  {
    act("$n splashes $N with contents of $p.", TRUE, ch, from_obj, to_char, TO_NOTVICT);
    act("$n splashes you with contents of $p.", TRUE, ch, from_obj, to_char, TO_NOTVICT);
    act("You splash $N with contents of $p.", FALSE, ch, from_obj, to_char, TO_CHAR);
    weight_change_object(from_obj, -from_obj->value[1]);        /* Empty */
    from_obj->value[1] = 0;
    from_obj->value[2] = 0;
    from_obj->value[3] = 0;
    name_from_drinkcon(from_obj);
    return;
  }
  if (!(to_obj = get_obj_in_list_vis(ch, Gbuf2, ch->carrying)))
  {
    act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (to_obj->type != ITEM_DRINKCON)
  {
    act("You can't pour anything into that.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (to_obj == from_obj)
  {
    act("A most unproductive effort.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if ((to_obj->value[1] != 0) && (to_obj->value[2] != from_obj->value[2]))
  {
    act("There is already another liquid in it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!(to_obj->value[1] < to_obj->value[0]))
  {
    act("There is no room for more.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  snprintf(Gbuf4, MAX_STRING_LENGTH, "You pour the %s into the %s.",
          drinks[from_obj->value[2]], Gbuf2);
  send_to_char(Gbuf4, ch);

  /*
   * New alias
   */
  if (to_obj->value[1] == 0)
    name_to_drinkcon(to_obj, from_obj->value[2]);

  /*
   * First same type liq.
   */
  to_obj->value[2] = from_obj->value[2];

  /*
   * Then how much to pour
   */
  from_obj->value[1] -= (amount = (to_obj->value[0] - to_obj->value[1]));

  to_obj->value[1] = to_obj->value[0];

  if (from_obj->value[1] < 0)
  {                             /*
                                 * There was to little
                                 */
    to_obj->value[1] += from_obj->value[1];
    amount += from_obj->value[1];
    from_obj->value[1] = 0;
    from_obj->value[2] = 0;
    from_obj->value[3] = 0;
    name_from_drinkcon(from_obj);
  }
  /*
   * Then the poison boogie
   */
  to_obj->value[3] = (to_obj->value[3] || from_obj->value[3]);

  /*
   * And the weight boogie
   */
  weight_change_object(from_obj, -amount);
  weight_change_object(to_obj, amount); /*
                                         * Add weight
                                         */

  return;
}

void do_fill(P_char ch, char *argument, int cmd)
{
  P_obj    from_obj;
  P_obj    to_obj;
  int      amount;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf4[MAX_STRING_LENGTH];

  argument_interpreter(argument, Gbuf1, Gbuf2);

  if (!*Gbuf1)
  {
    act("Fill what from where?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!(to_obj = get_obj_in_list_vis(ch, Gbuf1, ch->carrying)))
  {
    act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (to_obj->type != ITEM_DRINKCON)
  {
    act("Fill only works with drink containers.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (to_obj->value[1] == to_obj->value[0])
  {
    act("Your $q is already full.", FALSE, ch, to_obj, 0, TO_CHAR);
    return;
  }
  if (!*Gbuf2)
  {
    /*
     * This will be for obvious in-room containers
     */
    act("For the moment, you need to type the source.",
        FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!(from_obj = get_obj_in_list_vis(ch, Gbuf2, ch->carrying)))
  {
    if (!
        (from_obj =
         get_obj_in_list_vis(ch, Gbuf2, world[ch->in_room].contents)))
    {
      act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    else if (from_obj->wear_flags & ITEM_TAKE)
    {
      act("You must get it first!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (from_obj->type != ITEM_DRINKCON)
  {
    act("You can't get anything out of that.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (to_obj == from_obj)
  {
    act("A most unproductive effort.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if ((to_obj->value[1] != 0) && (to_obj->value[2] != from_obj->value[2]))
  {
    act("There is already another liquid in it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!(to_obj->value[1] < to_obj->value[0]) || to_obj->value[1] < 0)
  {
    act("There is no room for more.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  snprintf(Gbuf4, MAX_STRING_LENGTH, "You fill the %s with the %s.", Gbuf1,
          drinks[from_obj->value[2]]);
  act(Gbuf4, FALSE, ch, 0, 0, TO_CHAR);

  /*
   * New alias
   */
  if (to_obj->value[1] == 0)
    name_to_drinkcon(to_obj, from_obj->value[2]);

  /*
   * First same type liq.
   */
  to_obj->value[2] = from_obj->value[2];

  /*
   * Then how much to pour
   */
  if (from_obj->value[1] > 0)
  {
    from_obj->value[1] -= (amount = (to_obj->value[0] - to_obj->value[1]));

    to_obj->value[1] = to_obj->value[0];

    if (from_obj->value[1] < 0)
    {                           /*
                                 * There was too little
                                 */
      to_obj->value[1] += from_obj->value[1];
      amount += from_obj->value[1];
      from_obj->value[1] = 0;
      from_obj->value[2] = 0;
      from_obj->value[3] = 0;
      name_from_drinkcon(from_obj);
    }
  }
  else
  {
    amount = to_obj->value[0] - to_obj->value[1];
    to_obj->value[1] += amount;
  }

  /*
   * Then the poison boogie
   */
  to_obj->value[3] = (to_obj->value[3] || from_obj->value[3]);

  /*
   * And the weight boogie
   */

  if (from_obj->value[1] >= 0)
    weight_change_object(from_obj, -amount);
  weight_change_object(to_obj, amount); /*
                                         * Add weight
                                         */

  return;
}

void do_sip(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_INPUT_LENGTH];
  char     Gbuf4[MAX_STRING_LENGTH];
  P_obj    temp;

  if (GET_RACE(ch) == RACE_ILLITHID && GET_LEVEL(ch) < AVATAR)
  {
    send_to_char
      ("Ugh. Even if you had the means to drink, the thought revolts you.\r\n",
       ch);
    return;
  }
  /*
   * added by DTS 5/26/95 to prevent pets healing by drinking holy water
   */
  if (IS_NPC(ch))
  {
    send_to_char("Monsters don't need to sip liquids!\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying)))
  {
    act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (temp->type != ITEM_DRINKCON)
  {
    act("You can't sip from that!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (GET_COND(ch, DRUNK) > 10)
  {                             /*
                                 * The pig is drunk !
                                 */
    act("You simply fail to reach your mouth!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n tries to sip, but fails!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  if (!temp->value[1])
  {                             /*
                                 * Empty
                                 */
    act("But there is nothing in it?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  act("$n sips from the $q.", TRUE, ch, temp, 0, TO_ROOM);
  snprintf(Gbuf4, MAX_STRING_LENGTH, "It tastes like %s.\r\n", drinks[temp->value[2]]);
  send_to_char(Gbuf4, ch);

  if (temp->value[3])
  {
    act("But it also has a strange taint!", FALSE, ch, 0, 0, TO_CHAR);
    poison_lifeleak(10, ch, 0, 0, ch, 0);
  }
  return;
}

void do_taste(P_char ch, char *argument, int cmd)
{
  char     arg[MAX_INPUT_LENGTH];
  P_obj    temp;

  if (GET_RACE(ch) == RACE_ILLITHID && GET_LEVEL(ch) < AVATAR)
  {
    send_to_char("Ugh. Even if you had the means to eat, the thought revolts you.\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying)))
  {
    act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (temp->type == ITEM_DRINKCON)
  {
    do_sip(ch, argument, -4);
    return;
  }
  if( !(temp->type == ITEM_FOOD) )
  {
    act("It tastes inedible, aren't you glad it wasn't coated with poison?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  act("$n tastes the $q.", TRUE, ch, temp, 0, TO_ROOM);
  act("You taste the $q.", FALSE, ch, temp, 0, TO_CHAR);

  if( temp->value[3] > 0 )
  {
    act("Oops, it did not taste good at all!", FALSE, ch, 0, 0, TO_CHAR);
    GET_HIT(ch) = MAX(1, GET_HIT(ch) - 10);
  }
  return;
}


// Functions Related to Do_Wear()

/*
 * The View function to Do_Wear() displays most wear messages, only 
 * leaving complex code in WEAR().  This is where you add the messages
 * for new equipment slots and what players see when they are worn.
 * It is ugly, but at least now all the messages are in one spot 
 * instead of spread over two different functions.  I opted to keep
 * this spaghetti code for the variability it allows over a simpler 
 * four case statement which handles the different wears generically.
 * -Sniktiorg (Nov.15.12)
 */ 
void perform_wear(P_char ch, P_obj obj_object, int keyword)
{
  struct affected_type af;
  switch (keyword)
  {
  case 0:
    // Technically, this shouldn't be called.
    act("$n lights $p and holds it.", FALSE, ch, obj_object, 0, TO_ROOM);
    break;
  case 1:
    // Place on proper finger. -Sniktiorg (Nov.14.12)
    if (ch->equipment[WEAR_FINGER_L]) {
      act("You place $p on your right ring finger.", 0, ch, obj_object, 0, TO_CHAR);
    } else {
      act("You place $p on your left ring finger.", 0, ch, obj_object, 0, TO_CHAR);
    }
    act("$n slips $s finger into $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 2:
    act("You duck your head and place $p around your neck.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n places $p around $s neck.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 3:
    act("You shrug into $p.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n shrugs into $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 4:
    act("You don $p on your head.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n dons $p on $s head.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 5:
    act("You slide your legs into $p.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n slides $s legs into $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 6:
    act("You place $p on your feet.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n places $p on $s feet.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 7:
    act("You pull $p onto your hands.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n pulls $p onto $s hands.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 8:
    act("You cover your arms with $p.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n covers $s arms with $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 9:
    act("You wear $p about your body.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n wears $p about $s body.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 10:
    act("You clasp $p around your waist.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n clasps $p around $s waist.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 11:
    // To_CHAR is in WEAR() because of complexity and laziness. -Sniktiorg (Nov.12.12)
    act("$n places $p around $s wrist.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 12:
    act("You wield $p.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n wields $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 13:
    if ((GET_ITEM_TYPE(obj_object) == ITEM_LIGHT) && obj_object->value[2]) {
      act("You light $p and hold it.", 0, ch, obj_object, 0, TO_CHAR);
      act("$n lights $p and holds it.", TRUE, ch, obj_object, 0, TO_ROOM);
      if (obj_object->value[2] > 0)
        CharWait(ch, 2);
    } else {
      act("You hold $p.", 0, ch, obj_object, 0, TO_CHAR);
      act("$n grabs $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    }
    break;
  case 14:
    act("You strap $p to your arm.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n straps $p to $s arm.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 15:
    act("You slide $p over your eyes.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n slides $p over $s eyes.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 16:
    act("You cover your face with $p.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n covers $s face with $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 17:
    // Place in proper ear. -Sniktiorg (Nov.14.12)
    if (ch->equipment[WEAR_EARRING_L]) {
      act("You wear $p on your right ear.", 0, ch, obj_object, 0, TO_CHAR);
    } else {
      act("You wear $p on your left ear.", 0, ch, obj_object, 0, TO_CHAR);
    }
    act("$n wears $p on $s ear.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 18:
    act("You strap $p onto your back.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n straps $p to $s back.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 19:
  case 28:
    act("You don the guild insignia of $p.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n dons $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 20:
    act("You strap $p on your back.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n straps $p to $s back.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 21:
    act("You attach $p to your belt.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n attaches $p to $s belt.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 22:
    act("You throw $p about your &+yhindquarters&n.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n wears $p about $s &+yhindquarters&n.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 23:
    act("You wear $p on your &+Ltail&n.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n wears $p on $s &+Ltail&n.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 24:
    act("You wear $p on your nose.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n wears $p on $s nose.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 25:
    act("You wear $p on your &+Lhorns&n.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n wears $p on $s &+Lhorns&n.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 26:
    act("You toss $p in the air and it begins orbiting your head.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n throws $p in the air and it begins circling $s head.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 27:
    act("You shrug your &+Lspider's&n abdomen into $p.", 0, ch, obj_object, 0, TO_CHAR);
    act("$n shrugs $s &+Lspider's&n abdomen into $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  }

// Set-Show Affects 
  if (IS_SET(obj_object->extra_flags, ITEM_LIT))
  {
    act("&+WIt glows brightly.&n", TRUE, ch, 0, 0, TO_ROOM);
    act("&+WIt glows brightly.&n", TRUE, ch, 0, 0, TO_CHAR);
  }

  //Battlemage Coat
    if (obj_index[obj_object->R_num].virtual_number == 400218 && !IS_MULTICLASS_PC(ch) && !affected_by_spell(ch, SPELL_BATTLEMAGE))
    {
	send_to_char("&+rAs you cover yourself with your &+Ymaje&+rst&+Yic &+Yrobe&+r,\r\n&+ryou suddenly feel an enhanced &+mpower&+r rise up within your &+Ybody&+r!&n\r\n", ch);
	act("&+L$n's &+Yeyes&+r suddenly glow &+yg&+Yo&+yl&+Yd&+ye&+Yn&+r with po&+Rwe&+rr!&n", TRUE, ch, 0, 0, TO_ROOM);
       bzero(&af, sizeof(af));
    	af.type = SPELL_BATTLEMAGE;
    	af.duration = -1;
    	affect_to_char(ch, &af);
    }
	

  if (IS_SET(obj_object->bitvector, AFF_INVISIBLE) &&
      !affected_by_spell(ch, TAG_PERMINVIS))
  {
    bzero(&af, sizeof(af));
    af.type = TAG_PERMINVIS;
    af.duration = -1;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char(ch, &af);
    /*
     * Only show vanish if ch was visible first.  Should actually check per person in room to see if 
     * they could see ch and then show message if they aren't affected by detect invis. - Sniktiorg (Nov.9.12)
     */	  
    if (!IS_AFFECTED(ch, AFF_INVISIBLE) &&
        !IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
    {
      act("&+L$n slowly fades out of existence.&n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("&+LYou vanish.&n\r\n", ch);
    }
  }
// End Set Affects
  
/*
  if (IS_SET(obj_object->bitvector, AFF_INVISIBLE) && !IS_AFFECTED(ch, AFF_INVISIBLE)
      && !IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
    if ((keyword != 13) || (obj_object->wear_flags == (ITEM_TAKE + ITEM_HOLD))) {
      act("$n slowly fades out of existence.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You vanish.\r\n", ch);
    }
*/
}

/*
 * Tallies the number of artifacts CH is using.
 */
int numb_artis_using(P_char ch)
{
  int      i, n = 0;

  for (i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i] &&
        IS_ARTIFACT(ch->equipment[i]) &&
        !CAN_WEAR(ch->equipment[i], WEAR_IOUN) &&
        (obj_index[ch->equipment[i]->R_num].virtual_number < 67200 ||
         obj_index[ch->equipment[i]->R_num].virtual_number > 67299))
      n++;

  return n;
}

/*
 * Calculates the number of free hands CH has.
 */
int get_numb_free_hands(P_char ch)
{
  int free_hands = 2;

  if (HAS_FOUR_HANDS(ch))
    free_hands += 2;

  if (ch->equipment[HOLD])
    free_hands -= wield_item_size(ch, ch->equipment[HOLD]);

  if (ch->equipment[WEAR_SHIELD])
    free_hands--;

  if (ch->equipment[WIELD])
    free_hands -= wield_item_size(ch, ch->equipment[WIELD]);

  if (ch->equipment[WIELD2])
    free_hands -= wield_item_size(ch, ch->equipment[WIELD2]);

  if (ch->equipment[WIELD3])
    free_hands -= wield_item_size(ch, ch->equipment[WIELD3]);

  if (ch->equipment[WIELD4])
    free_hands -= wield_item_size(ch, ch->equipment[WIELD4]);

  if (free_hands < 0)
    free_hands = 0;

  if (IS_GIANT(ch) && free_hands > 0)
    if (GET_RACE(ch) == RACE_MINOTAUR && ch->equipment[WIELD])
      return free_hands;
    else
      return free_hands + 1;

  return free_hands;
}

/* 
 * Checks if an object is an artifact.
 */
bool check_single_artifact(P_char ch, P_obj obj)
{
  if (!ch || !obj)
    return true;

  if (!(int)get_property("artifact.major.limit.one", 1))
  {
    return false;
  }

  if (IS_ARTIFACT(obj))
  {
    if (isname("unique", obj->name) &&
       !isname("powerunique", obj->name))
    {
      return false;
    }

    for (int i = 0; i < MAX_WEAR; i++)
    {
      if (ch->equipment[i] &&
	  IS_ARTIFACT(ch->equipment[i]) &&
	  !isname("unique", ch->equipment[i]->name))
      {
	return true;
      }
    }
  }

  return false;
}

/*
 * Helper function to cut down on massive repetition.  It executes the wear
 * call once the Controller [Wear()] has determined to do so. -Sniktiorg (Nov.16.12)
 */
void execute_wear(P_char ch, P_obj obj_object, int position, int keyword, bool showit)
{
  if (showit) // Show the Object Wear?
    perform_wear(ch, obj_object, keyword);
  obj_from_char(obj_object);
  equip_char(ch, obj_object, position, !showit);
}

/*
 * Helper function which wraps about Execute_Wear() for those items with standard
 * "You are already wearing (X)" messages. -Sniktiorg (Nov.17.12)
 * Currently, this function is only used by hands and arms as they are all subject
 * to being multi-armed.  However, they still could be replaced with 
 * remove_and_wear().  I have left this function incase it is decided that certain
 * equipment positions should not auto-replace.  Weapons and held items use their 
 * own convoluted logic as I was again too lazy to figure out how to make them more
 * concise. -Sniktiorg (Dec.12.12)
 */
int stop_or_wear(const char denied[], P_char ch, P_obj obj_object, int position, int keyword, bool showit)
{
  // Already Wearing the Item
  if (ch->equipment[position]) {
    if (showit)
      act(denied, 0, ch, ch->equipment[position], 0, TO_CHAR);
    } else {
    // Wear Item
      execute_wear(ch, obj_object, position, keyword, showit);
      return true;
    }
  return false;
}

/*
 * Returns TRUE if ch is wearing perm invis eq.
 */
int wearing_invis(P_char ch)
{
  int      found = 0, k;

  for (k = 0; k < MAX_WEAR; k++)
    if (ch->equipment[k] &&
        IS_SET(ch->equipment[k]->bitvector, AFF_INVISIBLE))
      found = 1;
  return found;
}

/*
 * The receiving code should handle displaying of messages to the user.
 * - Sniktiorg 25.1.13
 * New Remove code which handles only the removing of the item.  This
 * allows for use in loops as well as in stand-alone capacities.  It 
 * also facilitates removing an item by position which is used in the
 * auto-replace wear code.  The procedure returns an int representing
 * the following:
 */
#define REMOVE_SUCCESS        0
#define REMOVE_CURSED         1
#define REMOVE_BREAK_ENCHANT  2
#define REMOVE_CANT_CARRY     3
#define REMOVE_NOT_USING      4
int remove_item(P_char ch, P_obj obj, int position)
{
  struct   obj_affect *o_af;

  // Tests if Object Exists
  if( obj )
  {
    if( IS_SET(obj->extra_flags, ITEM_NODROP) && !IS_TRUSTED(ch) )
    {
      return REMOVE_CURSED;
    }
    else if( CAN_CARRY_N(ch) > IS_CARRYING_N(ch) )
    {
      if( ch->equipment[WEAR_WAIST] && ch->equipment[WEAR_WAIST] == obj )
      {
        if( ch->equipment[WEAR_ATTACH_BELT_1] )
        {
          obj_to_char(unequip_char(ch, WEAR_ATTACH_BELT_1), ch);
        }
        if( ch->equipment[WEAR_ATTACH_BELT_2] )
        {
          obj_to_char(unequip_char(ch, WEAR_ATTACH_BELT_2), ch);
        }
        if( ch->equipment[WEAR_ATTACH_BELT_3] )
        {
          obj_to_char(unequip_char(ch, WEAR_ATTACH_BELT_3), ch);
        }
      }
      // Remove holy sword spell effects if sword is removed.
      if( ch->equipment[WIELD] && ch->equipment[WIELD] == obj )
      {
        strip_holy_sword( ch );
      }

      obj_to_char(unequip_char(ch, position), ch);

      // Remove Affects
      if (IS_SET(obj->bitvector, AFF_INVISIBLE) && affected_by_spell(ch, TAG_PERMINVIS) && !wearing_invis(ch))
        affect_from_char(ch, TAG_PERMINVIS);

      if (obj && (o_af = get_obj_affect(obj, SKILL_ENCHANT)))
      {
        affect_from_char(ch, o_af->data);
        obj_affect_remove(obj, o_af);
        return REMOVE_BREAK_ENCHANT;
      }
    }
    else
    {
      return REMOVE_CANT_CARRY;
    }
  }
  else // Object Doesn't Exist
  {
    return REMOVE_NOT_USING;
  }

  return REMOVE_SUCCESS;
}

/*
 * Helper function which wraps about Execute_Wear() and allows for the remove
 * and replace behavior used on single location items (ie. head, arms, body, etc). -Sniktiorg (Dec.1.12)
 */
int remove_and_wear(P_char ch, P_obj obj_object, int position, int keyword, int comnd, bool showit)
{
  P_obj temp = ch->equipment[position];
  int removed;
  // Remove Item Already in Place
   //send_to_char(snprintf("%1", MAX_STRING_LENGTH, ch->equipment[position]), ch);
  if( temp )
  {
    removed = remove_item(ch, temp, position);
    if( removed == REMOVE_SUCCESS || removed == REMOVE_BREAK_ENCHANT )
    {
      if( showit )
      {
        act("You stop using $p.", FALSE, ch, temp, 0, TO_CHAR);
        if( removed == REMOVE_BREAK_ENCHANT )
        {
          act("&+cSome of your &+Cmagic&+c dissipates...&n", FALSE, ch, 0, 0, TO_CHAR);
        }
      }
      // Wear Item
      execute_wear(ch, obj_object, position, keyword, showit);
      return TRUE;
    }
    else if( removed == REMOVE_CURSED )
    {
      if( showit )
      {
        act("$p won't budge!  Perhaps it's cursed?!?", TRUE, ch, temp, 0, TO_CHAR);
      }
    }
    else if( removed == REMOVE_CANT_CARRY )
    {
      if( showit )
      {
        send_to_char("You can't carry that many items.\r\n", ch);
      }
    }
  }
  else
  {
    execute_wear(ch, obj_object, position, keyword, showit);
    return TRUE;
  }

  return FALSE;
}

/*
 * The Controller to the Do_Wear() function.  I have refactored it down
 * in size, squished some needless repetitive code, and moved most of the
 * messages into the View [Perform_Wear()].  There are still areas that
 * can be refactored, both minor portions and the function as a whole
 * (which still retains a highly repetitive nature and begs for some more
 * thought into how to rid the function of this. -Sniktiorg (Nov.16.12)
 * Improvements:
 * 1) Let player to wield 2 weapons (Dual Wield).
 * 2) Returns INT now because else it would piss off mobs.
 *    (1=Successful, 0=Failure)
 * 3) Shows the item you are currently wearing if the function
 *    doesn't replace the item automatically.
 * 4) Replaces certain items with new item on wear.
 *
 * When adding new item types, execute_wear() should be followed by a
 * RETURN TRUE, stop_or_wear() should be be couched in an if statement
 * which returns true [ie. if (stop_or_wear()) return true], and
 * remove_and_wear() should be called following a RETURN statement
 * [ie. return remove_and_wear()]. -Sniktiorg (Dec.12.12)
 */
int wear(P_char ch, P_obj obj_object, int keyword, bool showit )
{
  char     Gbuf3[MAX_STRING_LENGTH];
  int      free_hands, wield_to_where, o_size, hands_needed, comnd;

  // Kill on !Object or !Character or dead char.
  if( !obj_object || !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  // Scrap it. Might cause crash. Dec08 -Lucrot
  if(obj_object->condition <= 0)
  {
    wizlog(56, "%s wore %s that's condition 0 or less : attempting to scrap.",
      GET_NAME(ch), obj_object->short_description);
    MakeScrap(ch, obj_object);
    return FALSE;
  }

  // Quick and dirty periodic check for buggy items. Dec08 -Lucrot
  // Can write to player log if these checks are sufficient.
  for (int i = 0; i < 3; i++ )
  {  // Hunting a bad apply ...
    if(obj_object->affected[i].location > APPLY_LAST &&
       obj_object->affected[i].modifier == 2)
    {
      wizlog(56, "%s has buggy item with a bad apply : %s",
        GET_NAME(ch), obj_object->short_description);
    }
    // Hunting Max_Race Equipment
    if( obj_object->affected[i].location >= APPLY_STR_RACE
      && obj_object->affected[i].location <= APPLY_LUCK_RACE && !IS_ARTIFACT(obj_object) )
    {
      wizlog(56, "%s has MAX_RACE item : %s", GET_NAME(ch), obj_object->short_description);
    }
    // Hunting APPLY_DAMROLL >= 20
    if(obj_object->affected[i].location == APPLY_DAMROLL
      && obj_object->affected[i].modifier >= 20)
    {
      wizlog(56, "%s has item with >= 20 damroll : %s",
        GET_NAME(ch), obj_object->short_description);
    }
    // Hunting APPLY_HITROLL >= 20
    if(obj_object->affected[i].location == APPLY_HITROLL &&
       obj_object->affected[i].modifier >= 20)
    {
      wizlog(56, "%s has item with >= 20 hitroll : %s",
        GET_NAME(ch), obj_object->short_description);
    }

    if(is_stat_max(obj_object->affected[i].location) && !IS_ARTIFACT(obj_object))
    {
      // CHA and LUCK are less important.
      if( (obj_object->affected[i].location == APPLY_LUCK_MAX && obj_object->affected[i].modifier > 15)
        || (obj_object->affected[i].location == APPLY_CHA_MAX && obj_object->affected[i].modifier > 15)
        || (obj_object->affected[i].location != APPLY_LUCK_MAX && obj_object->affected[i].location != APPLY_CHA_MAX
        && obj_object->affected[i].modifier > 10) )
      {
        char buf[128];
        sprinttype(obj_object->affected[i].location, apply_types, buf);
        wizlog(56, "%s has %s with %d %s.",
          GET_NAME(ch), obj_object->short_description, obj_object->affected[i].modifier, buf);
      }
    }
  }

  // Cannot use the item.  Return FALSE.
  if( !can_char_use_item(ch, obj_object) )
  {
    if( showit )
      act("You can't use $p.", FALSE, ch, obj_object, 0, TO_CHAR);
    if( IS_TRUSTED(ch) && GET_LEVEL(ch) == OVERLORD )
      send_to_char( "But you don't care...\n", ch );
    else
      return FALSE;
  }
#if 1
  /*
   * monk weight restriction
   */
  if (IS_PC(ch) && GET_CLASS(ch, CLASS_MONK) && keyword != 20)
  {
    /*
    if (GET_LEVEL(ch) < 40 &&
       (GET_OBJ_WEIGHT(obj_object) > (27 - (GET_LEVEL(ch) / 2))))
    {
      if (showit)
        act("$p is far too heavy and cumbersome, your skills would be useless!", FALSE, ch, obj_object, 0, TO_CHAR);
      return FALSE;
    }
    */
    int monkweight = get_property("monk.weight.str.modifier.denominator", 10);

    if(GET_OBJ_WEIGHT(obj_object) > (int)(GET_C_STR(ch) / monkweight))
    {
      if(showit)
        act("$p is far too heavy and cumbersome, your skills would be useless!", FALSE, ch, obj_object, 0, TO_CHAR);
      return FALSE;
    }
  }
#endif
/* let's check for artis here */
/*
#if 0
  if (IS_ARTIFACT(obj_object) &&
      !CAN_WEAR(obj_object, WEAR_IOUN) &&
      (obj_index[obj_object->R_num].virtual_number < 67200 ||
       obj_index[obj_object->R_num].virtual_number > 67299) &&
      (numb_artis_using(ch) >= 1))
  {
    send_to_char
      ("You are already equipping one artifact - to equip another would fatally disrupt their magical auras!\r\n",
       ch);
    return FALSE;
  }
#endif // Wield as many artis as you want

  if (check_single_artifact(ch, obj_object))
  {
    send_to_char("You cannot wear any more items of such power!\r\n", ch);
    return FALSE;
  }
*/
  free_hands = get_numb_free_hands(ch);

  switch (keyword)
  {
  case 0: /* None */
    logit(LOG_OBJ, "wear(): object worn in invalid location (%s, '%s' %d)",
      J_NAME(ch), obj_object->short_description, OBJ_VNUM(obj_object) );
    break;

  case 1: /* Finger */
    if( CAN_WEAR(obj_object, ITEM_WEAR_FINGER) && !IS_THRIKREEN(ch) )
    {
      // Already Wearing the Item
      if( (ch->equipment[WEAR_FINGER_L]) && (ch->equipment[WEAR_FINGER_R]) )
      {
        if( showit )
        {
          send_to_char("Your fingers are already well adorned.\r\n", ch);
        }
      }
      else
      {
        // Wear Item
        execute_wear(ch, obj_object, ((ch->equipment[WEAR_FINGER_L]) ? WEAR_FINGER_R : WEAR_FINGER_L), keyword, showit);
        return TRUE;
      }
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that on your fingers.\r\n", ch);
      }
    }
    break;

  case 2: /* Neck */
    if( CAN_WEAR(obj_object, ITEM_WEAR_NECK) )
    {
      // Already Wearing the Item
      if( (ch->equipment[WEAR_NECK_1]) && (ch->equipment[WEAR_NECK_2]) )
      {
        if( showit )
        {
          send_to_char("You can't wear any more around your neck.\r\n", ch);
        }
      }
      else
      {
        // Wear Item
        execute_wear(ch, obj_object, ((ch->equipment[WEAR_NECK_1]) ? WEAR_NECK_2 : WEAR_NECK_1), keyword, showit);
        return TRUE;
      }
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that around your neck.\r\n", ch);
      }
    }
    break;

  case 3: /* Body */
    if( CAN_WEAR(obj_object, ITEM_WEAR_BODY) && has_eq_slot( ch, WEAR_BODY ) )
    {
      if( IS_SET(obj_object->extra_flags, ITEM_WHOLE_BODY) )
      {
        if( IS_CENTAUR(ch) || IS_MINOTAUR(ch) || IS_OGRE(ch) || IS_SGIANT(ch)
          || GET_RACE(ch) == RACE_WIGHT || GET_RACE(ch) == RACE_SNOW_OGRE )
        {
          if( showit )
          {
            send_to_char("You can't wear full body armor.\r\n", ch);
          }
          break;
        }
        else if( (ch->equipment[WEAR_ARMS]) && (ch->equipment[WEAR_LEGS]) )
        {
          if( showit )
          {
            send_to_char("You can't wear something on your arms and legs and wear that.\r\n", ch);
          }
          break;
        }
        else if( ch->equipment[WEAR_ARMS] )
        {
          if( showit )
          {
            send_to_char("You can't wear something on your arms and wear that.\r\n", ch);
          }
          break;
        }
        else if( ch->equipment[WEAR_LEGS] )
        {
          if( showit )
          {
            send_to_char("You can't wear something on your legs and wear that.\r\n", ch);
          }
          break;
        }
      }
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_BODY, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that on your body.\r\n", ch);
      }
    }
    break;

  case 4: /* Head */
    if( CAN_WEAR(obj_object, ITEM_WEAR_HEAD)
        && !IS_MINOTAUR(ch) && !IS_ILLITHID(ch) && !IS_PILLITHID(ch) ) /* Should be a Macro */
    {
      if( IS_SET(obj_object->extra_flags, ITEM_WHOLE_HEAD) )
      {
        if( (ch->equipment[WEAR_EYES]) && (ch->equipment[WEAR_FACE]) )
        {
          if( showit )
          {
            send_to_char("You can't wear something on your eyes and face and wear that.\r\n", ch);
          }
          break;
        }
        else if( ch->equipment[WEAR_EYES] )
        {
          if( showit )
          {
            send_to_char("You can't wear something on your eyes and wear that.\r\n", ch);
          }
          break;
        }
        else if( ch->equipment[WEAR_FACE] )
        {
          if( showit )
          {
            send_to_char("You can't wear something on your face and wear that.\r\n", ch);
          }
          break;
        }
      }
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_HEAD, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        if( IS_ILLITHID(ch) || IS_PILLITHID(ch) )
        {
          send_to_char("Sorry, you can't wear anything on your head.\r\n", ch);
        }
        else
        {
          send_to_char("You can't wear that on your head.\r\n", ch);
        }
      }
    }
    break;

  case 5: /* Legs */
    if( CAN_WEAR(obj_object, ITEM_WEAR_LEGS) && !IS_DRIDER(ch) && !IS_CENTAUR(ch)
      && !IS_HARPY(ch) && !IS_OGRE(ch) && !IS_FIRBOLG(ch) )
    {
      if( ch->equipment[WEAR_BODY] &&
          IS_SET(ch->equipment[WEAR_BODY]->extra_flags, ITEM_WHOLE_BODY) )
      {
        if( showit )
          send_to_char("You can't wear something on your legs and wear that on your body.\r\n", ch);
        break;
      }
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_LEGS, keyword, comnd, showit);
    }
    else
    {
      if (showit)
      {
        if(IS_EFREET(ch))
        {
          send_to_char("What legs?!  You have none!\r\n", ch);
        }
        else
        {
          send_to_char("You can't wear that on your legs.\r\n", ch);
        }
      }
    }
    break;

  case 6: /* Feet */
    if( CAN_WEAR(obj_object, ITEM_WEAR_FEET) )
    {
      if( isname("horseshoes", obj_object->name) )
      {
        if( IS_CENTAUR(ch) || IS_MINOTAUR(ch) )
        {
          // Replace if Wearing Something or Wear New Item
          return remove_and_wear(ch, obj_object, WEAR_FEET, keyword, comnd, showit);
        }
      }
      else if( !IS_DRIDER(ch) && !IS_THRIKREEN(ch) && !IS_HARPY(ch) && !IS_MINOTAUR(ch) && !IS_CENTAUR(ch) )
      {
        // Replace if Wearing Something or Wear New Item
        return remove_and_wear(ch, obj_object, WEAR_FEET, keyword, comnd, showit);
      }
    }
    if( showit )
    {
      send_to_char("You can't wear that on your feet.\r\n", ch);
    }
    break;

  case 7: /* Hands */
    if( CAN_WEAR(obj_object, ITEM_WEAR_HANDS) )
    {
      /* Didn't condense the following because it differentiates enough and a compound
       * ternary expression is too much to read. - Sniktiorg (Nov.12.12)
       */
      if( HAS_FOUR_HANDS(ch) )
      {
        if( (ch->equipment[WEAR_HANDS]) && (ch->equipment[WEAR_HANDS_2]) )
        {
          if( showit )
          {
            send_to_char("You can't wear any more on your hands.\r\n", ch);
          }
        }
        else
        {
          // Wear Item
          execute_wear(ch, obj_object, ((ch->equipment[WEAR_HANDS]) ? WEAR_HANDS_2 : WEAR_HANDS), keyword, showit);
          return TRUE;
        }
      } // End Four_Hands
      else
      {
        // Check if Wearing Something or Wear New Item (Technically, Could Auto-replace)
        if( stop_or_wear("You already wear $p on your hands.", ch, obj_object, WEAR_HANDS, keyword, showit) )
        {
          return TRUE;
        }
      } // End Two_Hands
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that on your hands.\r\n", ch);
      }
    }
    break;

  case 8: /* Arms */
    if( CAN_WEAR(obj_object, ITEM_WEAR_ARMS) && !IS_OGRE(ch) && !IS_FIRBOLG(ch) && !IS_SGIANT(ch)
      && !(GET_RACE(ch) == RACE_SNOW_OGRE) )
    {
     /* Didn't condense the following because it differentiates enough and a compound 
      * ternary expression is too much to read. - Sniktiorg (Nov.12.12)
      */
      if( HAS_FOUR_HANDS(ch) )
      {
        // Already Wearing Items on Both Arms
        if( (ch->equipment[WEAR_ARMS]) && (ch->equipment[WEAR_ARMS_2]) )
        {
          if( showit )
          {
            send_to_char("You can't wear any more on your arms.\r\n", ch);
          }
        }
        else
        {
          // Wear Item
          execute_wear(ch, obj_object, ((ch->equipment[WEAR_ARMS]) ? WEAR_ARMS_2 : WEAR_ARMS), keyword, showit);
          return TRUE;
        }
      }
      else
      {
        if( ch->equipment[WEAR_BODY] && IS_SET(ch->equipment[WEAR_BODY]->extra_flags, ITEM_WHOLE_BODY) )
        {
          if( showit )
          {
            send_to_char("You can't wear something on your arms and wear that on your body.\r\n", ch);
          }
          break;
        }
        // Check if Wearing Something or Wear New Item (Technically, Could Auto-replace)
        if( stop_or_wear("You already wear $p on your arms.", ch, obj_object, WEAR_ARMS, keyword, showit) )
        {
          return TRUE;
        }
      }
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that on your arms.\r\n", ch);
      }
    }
    break;

  case 9: /* About */
    if( CAN_WEAR(obj_object, ITEM_WEAR_ABOUT) )
    {
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_ABOUT, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that about your body.\r\n", ch);
      }
    }
    break;

  case 10: /* Waist */
    if( (CAN_WEAR(obj_object, ITEM_WEAR_WAIST)) )
    {
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_WAIST, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that about your waist.\r\n", ch);
      }
    }
    break;

  case 11: /* Wrist */
    // Can be Refactored a Bit, but I am too lazy at the moment.  -Sniktiorg (Nov.12.12)
    if( CAN_WEAR(obj_object, ITEM_WEAR_WRIST) )
    {
      if( (!HAS_FOUR_HANDS(ch) && ch->equipment[WEAR_WRIST_L] && ch->equipment[WEAR_WRIST_R])
        || (HAS_FOUR_HANDS(ch) && ch->equipment[WEAR_WRIST_L] && ch->equipment[WEAR_WRIST_R]
        && ch->equipment[WEAR_WRIST_LL] && ch->equipment[WEAR_WRIST_LR]) )
      {
        if( showit )
        {
          send_to_char("You already wear something around all your wrists.\r\n", ch);
        }
      }
      else
      {
        if( showit )
        {
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object);
        if( !ch->equipment[WEAR_WRIST_L] )
        {
          if( showit )
          {
            act("You place $p around your left wrist.", 0, ch, obj_object, 0, TO_CHAR);
          }
          equip_char(ch, obj_object, WEAR_WRIST_L, !showit);
          return TRUE;
        }
        else if( !ch->equipment[WEAR_WRIST_R] )
        {
          if( showit )
          {
            act("You place $p around your right wrist.", 0, ch, obj_object, 0, TO_CHAR);
          }
          equip_char(ch, obj_object, WEAR_WRIST_R, !showit);
          return TRUE;
        }
        else if( !ch->equipment[WEAR_WRIST_LL] )
        {
          if( showit )
          {
            act("You place $p around your lower left wrist.", 0, ch, obj_object, 0, TO_CHAR);
          }
          equip_char(ch, obj_object, WEAR_WRIST_LL, !showit);
          return TRUE;
        }
        else
        {
          if( showit )
          {
            act("You place $p around your lower right wrist.", 0, ch, obj_object, 0, TO_CHAR);
          }
          equip_char(ch, obj_object, WEAR_WRIST_LR, !showit);
          return TRUE;
        }
      }
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that around your wrist.\r\n", ch);
      }
    }
    break;

  case 12: /* Wield */
    if( !CAN_WEAR(obj_object, ITEM_WIELD) )
    {
      if( showit )
      {
        send_to_char("You can't wield that.\r\n", ch);
      }
      break;
    }
    if( !free_hands )
    {
      if( showit )
      {
        send_to_char("You need at least one free hand to wield anything.\r\n", ch);
      }
      break;
    }

    hands_needed = (IS_SET(obj_object->extra_flags, ITEM_TWOHANDS) || obj_object->value[0] == WEAPON_2HANDSWORD) ? 2 : 1;

    if( hands_needed == 2 && free_hands < 2 )
    {
      if( showit )
      {
        send_to_char("You need two free hands to wield that.\r\n", ch);
      }
      break;
    }

    if( GET_OBJ_WEIGHT(obj_object) > (str_app[STAT_INDEX(GET_C_STR(ch))].wield_w) )
    {
      if( showit )
      {
        send_to_char("It is too heavy for you to use.\r\n", ch);
      }
      break;
    }
   /*
    * Check wield to where .. if primary is occupied, wield to
    * secondary, and vice versa.  Four-handed guys aren't quite so simple.
    */
    if( HAS_FOUR_HANDS(ch) )
    {
      if( hands_needed == 1 )
      {
        if( !ch->equipment[PRIMARY_WEAPON] )
        {
          wield_to_where = PRIMARY_WEAPON;
        }
        else if( !ch->equipment[SECONDARY_WEAPON]
          && (!ch->equipment[PRIMARY_WEAPON] || !(IS_SET(ch->equipment[PRIMARY_WEAPON]->extra_flags, ITEM_TWOHANDS)
          || ch->equipment[PRIMARY_WEAPON]->value[0] == WEAPON_2HANDSWORD) || IS_TRUSTED(ch)))
        {
          wield_to_where = SECONDARY_WEAPON;
        }
        else if( !ch->equipment[THIRD_WEAPON] )
        {
          wield_to_where = THIRD_WEAPON;
        }
        else if( !(IS_SET(ch->equipment[THIRD_WEAPON]->extra_flags, ITEM_TWOHANDS)
          || ch->equipment[THIRD_WEAPON]->value[0] == WEAPON_2HANDSWORD) || IS_TRUSTED(ch) )
        {
          wield_to_where = FOURTH_WEAPON;
        }
      }
      // Let's assume everything takes one or two hands, so here we are doing
      // the case where the weapon takes two hands
      else
      {
        if( !ch->equipment[PRIMARY_WEAPON] && !ch->equipment[SECONDARY_WEAPON] )
        {
          wield_to_where = PRIMARY_WEAPON;
        }
        else if( !ch->equipment[THIRD_WEAPON] )
        {
          if( ch->equipment[FOURTH_WEAPON] )
          {
            if( showit )
            {
              send_to_char("You do not have enough hands free to use that.\r\n", ch);
            }
            break;
          }
          wield_to_where = THIRD_WEAPON;
        }
        else
        {
          if( showit )
          {
            send_to_char("You do not have enough hands free to use that.\r\n", ch);
          }
          break;
        }
      }
    }
    else
    {
      if( ch->equipment[PRIMARY_WEAPON] )
      {
        wield_to_where = SECONDARY_WEAPON;
      }
      else
      {
        wield_to_where = PRIMARY_WEAPON;
      }
      if( wield_to_where == SECONDARY_WEAPON )
      {
        if( (IS_PC(ch) && !GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD))
          || (IS_NPC(ch) && (!IS_WARRIOR(ch) || (GET_LEVEL(ch) < 15))
          && (!IS_THIEF(ch) || (GET_LEVEL(ch) < 20))) )
        {
          if( showit )
          {
            send_to_char("You lack the training to use two weapons.\r\n", ch);
          }
          break;
        }
      }

      if( (wield_to_where == SECONDARY_WEAPON) && (IS_REACH_WEAPON(obj_object)
        || (!GET_CLASS(ch, CLASS_RANGER) && (GET_OBJ_WEIGHT(obj_object)
        * ((IS_OGRE(ch) || IS_SNOWOGRE(ch)) ? 2 : 3) > (str_app[STAT_INDEX(GET_C_STR(ch))].wield_w)))) )
      {
        if( showit )
        {
          send_to_char("It is too heavy to wield in your secondary hand.\r\n", ch);
        }
        break;
      }
    }

    // Wear Item
    execute_wear(ch, obj_object, wield_to_where, keyword, showit);
    return TRUE;
    break;

  case 13: /* Hold */
    if( !CAN_WEAR(obj_object, ITEM_HOLD) && (GET_ITEM_TYPE(obj_object) != ITEM_LIGHT) )
    {
      if( showit )
      {
        send_to_char("You can't hold this.\r\n", ch);
      }
      break;
    }
    if( !free_hands )
    {
      if( showit )
      {
        send_to_char("Your hands are full.\r\n", ch);
      }
      break;
    }
    if( (IS_SET(obj_object->extra_flags, ITEM_TWOHANDS) || (obj_object->type == ITEM_WEAPON
      && obj_object->value[0] == WEAPON_2HANDSWORD)) && (free_hands < 2) )
    {
      if( showit )
      {
        send_to_char("You need two free hands to hold that.\r\n", ch);
      }
      break;
    }
    if( showit )
    {
      perform_wear(ch, obj_object, keyword);
    }
    obj_from_char(obj_object);
    if( HAS_FOUR_HANDS(ch) )
    {
      if( !ch->equipment[HOLD] )
      {
        equip_char(ch, obj_object, HOLD, !showit);
      }
      else if( !ch->equipment[WIELD] )
      {
        equip_char(ch, obj_object, WIELD, !showit);
      }
      else if( !ch->equipment[WIELD3] )
      {
        equip_char(ch, obj_object, WIELD3, !showit);
      }
      else
      {
        equip_char(ch, obj_object, WIELD4, !showit);
      }
    }
    else
    {
      if( !ch->equipment[HOLD] || !ch->equipment[WIELD] )
      {
        equip_char(ch, obj_object, ch->equipment[HOLD] ? WIELD : HOLD, !showit);
      }
      else
      {
        obj_to_char(obj_object, ch);
        if( showit )
        {
          send_to_char("Weird bug in wield.\r\n", ch);
        }
        return FALSE;
      }
    }
    return TRUE;
    break;

  case 14: /* Shield */
    if( !CAN_WEAR(obj_object, ITEM_WEAR_SHIELD) )
    {
      if( showit )
      {
        send_to_char("You can't use that as a shield.\r\n", ch);
      }
      break;
    }
    if( !free_hands )
    {
      if( showit )
      {
        send_to_char("Your hands are full.\r\n", ch);
      }
      break;
    }
    // Already Wearing the Item
    if( ch->equipment[WEAR_SHIELD] )
    {
      if( showit )
      {
        act("You are already using $p.", 0, ch, ch->equipment[WEAR_SHIELD], 0, TO_CHAR);
      }
      break;
    }
    // Wear Item
    execute_wear(ch, obj_object, WEAR_SHIELD, keyword, showit);
    return TRUE;
    break;

  case 15: /* Eyes */
    if( CAN_WEAR(obj_object, ITEM_WEAR_EYES) )
    {
      // Already Wearing an ITEM_WHOLE_HEAD
      if( ch->equipment[WEAR_HEAD] && IS_SET(ch->equipment[WEAR_HEAD]->extra_flags, ITEM_WHOLE_HEAD) )
      {
        if( showit )
        {
          act("You can't wear something on your eyes and wear $p on your head.", 0, ch, ch->equipment[WEAR_HEAD], 0, TO_CHAR);
        }
        break;
      }
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_EYES, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that on your eyes.\r\n", ch);
      }
    }
    break;

  case 16: /* Face */
    if( CAN_WEAR(obj_object, ITEM_WEAR_FACE) )
    {
      // Already Wearing an ITEM_WHOLE_HEAD
      if( ch->equipment[WEAR_HEAD] && IS_SET(ch->equipment[WEAR_HEAD]->extra_flags, ITEM_WHOLE_HEAD) )
      {
        if( showit )
        {
          act("You can't wear something on your face and wear $p on your head.", 0, ch, ch->equipment[WEAR_HEAD], 0, TO_CHAR);
        }
        break;
      }
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_FACE, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that on your face.\r\n", ch);
      }
    }
    break;

  case 17: /* Earring */
    if( CAN_WEAR(obj_object, ITEM_WEAR_EARRING) && !IS_THRIKREEN(ch) )
    {
      // Already Wearing Two Earrings
      if( (ch->equipment[WEAR_EARRING_L]) && (ch->equipment[WEAR_EARRING_R]) )
      {
        if( showit )
        {
          send_to_char("You already wear an earring in each ear.\r\n", ch);
        }
      }
      else
      {
        // Wear Item
        execute_wear(ch, obj_object, ((ch->equipment[WEAR_EARRING_L]) ? WEAR_EARRING_R : WEAR_EARRING_L), keyword, showit);
        return TRUE;
      }
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that in your ear.\r\n", ch);
      }
    }
    break;

  case 18: /* Quiver */
    if( CAN_WEAR(obj_object, ITEM_WEAR_QUIVER) )
    {
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_QUIVER, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't use that as your quiver.\r\n", ch);
      }
    }
    break;

  case 19: /* Guild Insignia */
  case 28: // Badge
    if( CAN_WEAR(obj_object, ITEM_GUILD_INSIGNIA) )
    {
      // Replace if Wearing Something or Wear New Item
      // Using hardcoded 19, 'cause 28 is just a duplicate.
      return remove_and_wear(ch, obj_object, GUILD_INSIGNIA, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't use that as an insignia.\r\n", ch);
      }
    }
    break;

  case 20: /* Back */
    if( CAN_WEAR(obj_object, ITEM_WEAR_BACK) )
    {
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_BACK, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that on your back.\r\n", ch);
      }
    }
    break;
  case 21: /* Attach Belt */
    if( CAN_WEAR(obj_object, ITEM_ATTACH_BELT) )
    {
      if( !ch->equipment[WEAR_WAIST] )
      {
        if( showit )
        {
          act("You need a belt to attach $p to it.", 0, ch, obj_object, 0, TO_CHAR);
        }
      }
      else if( (ch->equipment[WEAR_ATTACH_BELT_1]) && (ch->equipment[WEAR_ATTACH_BELT_2])
        && (ch->equipment[WEAR_ATTACH_BELT_3]) )
      {
        if( showit )
        {
          send_to_char("Your belt is full.\r\n", ch);
        }
      }
      else
      {
        if( showit )
        {
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object);
        // Left the following an If-Then-Else instead of a ?: for ease of read.  -Sniktiorg (Nov.12.12)
        if( !ch->equipment[WEAR_ATTACH_BELT_1] )
        {
          equip_char(ch, obj_object, WEAR_ATTACH_BELT_1, !showit);
        }
        else if( !ch->equipment[WEAR_ATTACH_BELT_2] )
        {
          equip_char(ch, obj_object, WEAR_ATTACH_BELT_2, !showit);
        }
        else
        {
          equip_char(ch, obj_object, WEAR_ATTACH_BELT_3, !showit);
        }
        return TRUE;
      }
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't attach that to your belt.\r\n", ch);
      }
    }
    break;

  case 22: /* Horse Body */
    if( CAN_WEAR(obj_object, ITEM_HORSE_BODY) && (IS_CENTAUR(ch)) )
    {
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_HORSE_BODY, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that.\r\n", ch);
      }
    }
    break;

  case 23: /* Tail */
    if( HAS_TAIL(ch) )
    {
      if( CAN_WEAR(obj_object, ITEM_WEAR_TAIL) )
      {
        // Replace if Wearing Something or Wear New Item
        return remove_and_wear(ch, obj_object, WEAR_TAIL, keyword, comnd, showit);
      }
      else
      {
        if( showit )
        {
          send_to_char("You can't wear that on your &+Ltail&n.\r\n", ch);
        }
      }
    }
    else
    {
      send_to_char("You don't have a &+Ltail&n to wear that on.\r\n", ch);
    }
    break;

  case 24: /* Nose */
    if( CAN_WEAR(obj_object, ITEM_WEAR_NOSE) && IS_MINOTAUR(ch) )
    {
      return remove_and_wear(ch, obj_object, WEAR_NOSE, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that on your nose.\r\n", ch);
      }
    }
    break;

  case 25: /* Horns */
    if( CAN_WEAR(obj_object, ITEM_WEAR_HORN)
      && (IS_MINOTAUR(ch) || IS_HARPY(ch) || IS_PSBEAST(ch) || IS_TIEFLING(ch)) )
    {
      return remove_and_wear(ch, obj_object, WEAR_HORN, keyword, comnd, showit);
    }
    else
    {
      if( showit )
      {
        send_to_char("You can't wear that on your &+Lhorns&n.\r\n", ch);
      }
    }
    break;

  case 26: /* Ioun Stone */
    if( CAN_WEAR(obj_object, ITEM_WEAR_IOUN) )
    {
      return remove_and_wear(ch, obj_object, WEAR_IOUN, keyword, comnd, showit);
    }
    break;

  case 27: /* Spider Body */
    if( CAN_WEAR(obj_object, ITEM_SPIDER_BODY) )
    {
      if( !has_innate( ch, INNATE_SPIDER_BODY ) )
      {
        if( showit )
        {
          send_to_char("You can't wear &+Lspider's&n abdomen armor.\r\n", ch);
        }
        break;
      }
      // Replace if Wearing Something or Wear New Item
      return remove_and_wear(ch, obj_object, WEAR_SPIDER_BODY, keyword, comnd, showit);
    }
    else
    {
      if (showit)
      {
        send_to_char("You can't wear that.\r\n", ch);
      }
    }
    break;

  case -1:
    if( showit )
    {
      snprintf(Gbuf3, MAX_STRING_LENGTH, "Wear %s where?\r\n", FirstWord(obj_object->name));
      send_to_char(Gbuf3, ch);
    }
    break;

  case -2:
    if( showit )
    {
      snprintf(Gbuf3, MAX_STRING_LENGTH, "You can't wear the %s.\r\n", FirstWord(obj_object->name));
      send_to_char(Gbuf3, ch);
    }
    break;

  default:
    logit(LOG_OBJ, "Unknown type called in wear (%d).", keyword);
    break;
  }
  return FALSE;
}

/*
 * The order in which they appear below, is the order in which they will attempt to be worn (wear all, especially).
 * [0] wearflag, [1] keyword[] number, [2] eq slot position (Do Not Duplicate Numbers)
 */
int equipment_pos_table[CUR_MAX_WEAR][3] = {
  {ITEM_WEAR_BODY, 3, 5},
  {ITEM_WEAR_LEGS, 5, 7},
  {ITEM_WEAR_ARMS, 8, 10},
  {ITEM_WEAR_WAIST, 10, 13},
  {ITEM_WEAR_HANDS, 7, 9},
  {ITEM_WEAR_FEET, 6, 8},
  {ITEM_WEAR_HEAD, 4, 6},
  {ITEM_WEAR_SHIELD, 14, 11},
  {ITEM_WEAR_FINGER, 1, 2},
  {ITEM_WEAR_FINGER, 1, 1},
  {ITEM_WEAR_EARRING, 17, 22},
  {ITEM_WEAR_EARRING, 17, 21},
  {ITEM_WEAR_WRIST, 11, 15},
  {ITEM_WEAR_WRIST, 11, 14},
  {ITEM_WEAR_NECK, 2, 3},
  {ITEM_WEAR_NECK, 2, 4},
  {ITEM_WEAR_FACE, 16, 20},
  {ITEM_GUILD_INSIGNIA, 19, 24}, // Badge
  {ITEM_WEAR_EYES, 15, 19},
  {ITEM_WEAR_ABOUT, 9, 12},
  {ITEM_WEAR_QUIVER, 18, 23},
  {ITEM_WEAR_IOUN, 26, 41},
  {ITEM_WIELD, 12, 16},         // Primary weapon
  {ITEM_WIELD, 12, 17},         // Secondary weapons, same slot as HOLD
  {ITEM_WIELD, 12, 25},		// Tertiary weapon
  {ITEM_WIELD, 12, 26},         // Quarternary weapon
  {ITEM_WEAR_BACK, 20, 27},    
  {ITEM_ATTACH_BELT, 21, 30},   // Primary spot
  {ITEM_ATTACH_BELT, 21, 29},   // Secondary spot
  {ITEM_ATTACH_BELT, 21, 28},   // Tertiary spot
  {ITEM_WEAR_ARMS, 8, 31},
  {ITEM_WEAR_HANDS, 7, 32},
  {ITEM_WEAR_WRIST, 11, 33},
  {ITEM_WEAR_WRIST, 11, 34},
  {ITEM_HORSE_BODY, 22, 35},
  {ITEM_WEAR_LEGS, 5, 36},
  {ITEM_WEAR_TAIL, 23, 37},
  {ITEM_WEAR_FEET, 6, 38},
  {ITEM_WEAR_NOSE, 24, 39},
  {ITEM_WEAR_HORN, 25, 40},
  {ITEM_SPIDER_BODY, 27, 42},
  {ITEM_HOLD, 13, 18}           // HELD gets checked last of all
};

/*
 * Actual Initiating Function.  This basically figures out what the user
 * is trying to do, then passes the correct information onto the Controller
 * class: Wear().  -Sniktiorg (Nov.15.12)  
 */
void do_wear(P_char ch, char *argument, int cmd)
{
  struct obj_affect *o_af;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];
  P_obj    obj_object, next_obj = NULL;
  int      keyword;
  const char *keywords[] = {
    "finger",
    "neck",
    "body",
    "head",
    "legs",                     /* 4 */
    "feet",
    "hands",
    "arms",
    "about",
    "waist",                    /* 9 */
    "wrist",
    "xxx",
    "xxx",
    "shield",
    "eyes",                     /* 14 */
    "face",
    "earring",
    "quiver",
    "insignia",
    "back",                     /* 19 */
    "attach",
    "horse_body",
    "tail",
    "nose",
    "horns",                    /* 24 */
    "ioun",
    "spider_body",
    "badge",
    "\n"
  };
  int      loop = 0;

  // Letting dragons wear eq
  if( IS_ANIMAL(ch) && IS_NPC(ch) )
  {
    send_to_char("Duh, how do you wear stuff?\r\n", ch);
    return;
  }

  argument_interpreter(argument, Gbuf1, Gbuf2);
  // If there's an argument other than 'all'
  if( *Gbuf1 && str_cmp(Gbuf1, "all") )
  {
    obj_object = get_obj_in_list_vis(ch, Gbuf1, ch->carrying);
    if( obj_object )
    {
      // Wear slot to wear obj_object in.
      if( *Gbuf2 )
      {
        keyword = search_block(Gbuf2, keywords, FALSE); // Partial Match
        if( keyword == -1 )
        {
          snprintf(Gbuf4, MAX_STRING_LENGTH, "%s is an unknown body location.\r\n", Gbuf2);
          send_to_char(Gbuf4, ch);
        }
        else
        {
          wear(ch, obj_object, keyword + 1, 1); // UGH!  Passing through a +1 is nasty. But, aligns array with reality.
        }
      }
      else
      {
        keyword = -2;
	     /*
        * Determine the object's proper keyword position using a Break-Loop in place of
        *   a fifty-line if-then statement. - Sniktiorg (Nov.12.12)
	      */
        for (loop = 0; loop < CUR_MAX_WEAR; loop++)
        {
          if (CAN_WEAR(obj_object, equipment_pos_table[loop][0])) // Checks if Item can be Worn in this Spot
          {
            keyword = equipment_pos_table[loop][1]; // Assigns appropriate Keyword
            break;
          }
        }
      	// Can't Find a Wear Position
        //  keyword not set (default -2 above for loop).
        if( keyword == -2 )
        {
          send_to_char("That doesn't seem to work.\r\n", ch);
          return;
        }
        // Wear the Object
        if( obj_index[obj_object->R_num].virtual_number == 400218 && IS_MULTICLASS_PC(ch) )
        {
          send_to_char("&nThe power of this item is too great for a multiclassed character!&n\r\n", ch);
          return;
        }
        if( IS_OBJ_STAT2(obj_object, ITEM2_SOULBIND) && !isname(GET_NAME(ch), obj_object->name) )
        {
          send_to_char("&+LThis item is bound to someone elses &+Wsoul&+L, you may not wear it!&n\r\n", ch);
          return;
        }
        wear(ch, obj_object, keyword, TRUE);
      }
    }
    else // Object Doesn't Exist
    {
      snprintf(Gbuf3, MAX_STRING_LENGTH, "You do not seem to have the '%s'.\r\n", Gbuf1);
      send_to_char(Gbuf3, ch);
    }
  }
  else if (!Gbuf1 || str_cmp(Gbuf1, "all")) // No Item Designated
  {
    send_to_char("Wear what?\r\n", ch);
  }
  else
  {
   /*
    * WEAR ALL
    * Rearranged things here, should be faster, as it only checks for
    * filling empty equipment slots now, and equip is usually at top
    * of the inventory. - JAB
    */
    // Outer Loop
    for( loop = 0; loop < CUR_MAX_WEAR; loop++ )
    {
      if( !(ch->equipment[equipment_pos_table[loop][2]]) )
      {
        // Inner Loop
        for( obj_object = ch->carrying; obj_object; obj_object = next_obj )
        {
		  next_obj = obj_object->next_content;
		  // can't wear if you can't see, although mobs can wear !show items
		  // unless they are PC pets
		  if (!CAN_SEE_OBJ(ch, obj_object) || (IS_NOSHOW(obj_object) && IS_NPC(ch) && IS_PC_PET(ch)))
		  {
			continue;
		  }
          if (obj_index[obj_object->R_num].virtual_number == 400218 && IS_MULTICLASS_PC(ch))
          {
            send_to_char("&nThe power of this item is too great for a multiclassed character!&n\r\n", ch);
			continue;
		  }
		  if( IS_OBJ_STAT2(obj_object, ITEM2_SOULBIND) && !isname(GET_NAME(ch), obj_object->name) )
          {
            send_to_char("&+LThis item is bound to someone elses &+Wsoul&+L, you may not wear it!&n\r\n", ch);
            continue;
          }
          if (obj_object->type != ITEM_SPELLBOOK)
          {
            if (CAN_WEAR(obj_object, equipment_pos_table[loop][0]))
            {
              wear(ch, obj_object, equipment_pos_table[loop][1], TRUE);
              break;
            }
          }
        } // End Inner Loop
      }
    } // End Outer Loop
    // Give a Message that the ch has Equiped itself Fully.  However, doesn't
    // actually check if something was equiped.  Removed for now.
    /* act("$n fully equips $mself.", TRUE, ch, 0, 0, TO_ROOM);
     act("You fully equip yourself.", FALSE, ch, 0, 0, TO_CHAR);
    */
  }
  /*
   * added by DTS 5/18/95 to solve light bug
   */
  char_light(ch);
  room_light(ch->in_room, REAL);
}

void do_wield(P_char ch, char *argument, int cmd)
{
  P_obj    obj_object;
  int      keyword = 12;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];

  if (IS_ANIMAL(ch))
  {
    send_to_char("DUH!\r\n", ch);
    return;
  }

  argument_interpreter(argument, Gbuf1, Gbuf2);
  if (*Gbuf1)
  {
    obj_object = get_obj_in_list_vis(ch, Gbuf1, ch->carrying);

    if (obj_object)
    {
      if (affected_by_spell(ch, SKILL_DISARM) && 
          !IS_ELITE(ch))
      {
        send_to_char("You cant get control over your weapon!\r\n", ch);
        return;
      }
      wear(ch, obj_object, keyword, 1);
    }
    else
    {
      snprintf(Gbuf3, MAX_STRING_LENGTH, "You do not seem to have the '%s'.\r\n", Gbuf1);
      send_to_char(Gbuf3, ch);
    }
  }
  else
  {
    send_to_char("Wield what?\r\n", ch);
  }
  /*
   * added by DTS 5/18/95 to solve light bug
   */
  char_light(ch);
  room_light(ch->in_room, REAL);
}

void do_grab(P_char ch, char *argument, int cmd)
{
  P_obj    obj_object;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];

  argument_interpreter(argument, Gbuf1, Gbuf2);

  if (*Gbuf1)
  {
    obj_object = get_obj_in_list(Gbuf1, ch->carrying);
    if (obj_object)
    {
     if (obj_index[obj_object->R_num].virtual_number == 400218 && IS_MULTICLASS_PC(ch))
      {
        send_to_char("&nThe power of this item is too great for a multiclassed character!&n\r\n", ch);
        return;
      }
      if(IS_OBJ_STAT2(obj_object, ITEM2_SOULBIND) &&
      !isname(GET_NAME(ch), obj_object->name))
      {
       send_to_char("&+LThis item is bound to someone elses &+Wsoul&+L, you may not wear it!&n\r\n", ch);
       return;
      }
      wear(ch, obj_object, 13, 1);
    }
    else
    {
      snprintf(Gbuf3, MAX_STRING_LENGTH, "You do not seem to have the '%s'.\r\n", Gbuf1);
      send_to_char(Gbuf3, ch);
      return;
    }
  }
  else
  {
    send_to_char("Hold what?\r\n", ch);
  }
  /*
   * added by DTS 5/18/95 to solve light bug
   */
  char_light(ch);
  room_light(ch->in_room, REAL);
}

/* Modified Do_Remove which cuts down on repetition and allows for Do_Wear to 
 * properly replace worn equipment.
 * - Sniktiorg 25.1.13
 */
void do_remove(P_char ch, char *argument, int cmd)
{
  P_obj    obj_object, temp_obj;
  int      j, k, ret_type;
  bool     was_invis, naked;
  char     Gbuf1[MAX_STRING_LENGTH];
  struct affected_type af;

  // Determine Argument
  one_argument(argument, Gbuf1);

  // Determine Current Visibility
  was_invis = IS_SET(ch->specials.affected_by, AFF_INVISIBLE) ||
    IS_SET(ch->specials.affected_by2, AFF2_MINOR_INVIS);

  if (*Gbuf1) // If Argument Exists
  {
    if (!str_cmp(Gbuf1, "all")) // Remove All
    {
      if(IS_PC(ch) && affected_by_spell(ch, TAG_PVPDELAY))
      {
        act("$n frantically attempts to remove all of $s clothes and equipment!", FALSE, ch, 0, 0, TO_ROOM);
        act("&+rYou are too high on &+Radrenaline&+R to perform a remove all.&n", FALSE, ch, 0, 0, TO_CHAR);
        CharWait(ch, PULSE_VIOLENCE * 1);
        return;
      }
      // Remove All Section
      naked = TRUE; // Assume Player is Nude
      for (k = 0; k < MAX_WEAR; k++)
      {
        temp_obj = ch->equipment[k];
	      ret_type = remove_item(ch, ch->equipment[k], k);
	      // Acknowledge Removal
        if( ret_type == REMOVE_SUCCESS || ret_type == REMOVE_BREAK_ENCHANT )
        {
          act("You stop using $p.", FALSE, ch, temp_obj, 0, TO_CHAR);
	        //Drannak - set affect noauction to prevent selling off equip prior to being fragged
          affect_from_char(ch, SPELL_NOAUCTION);
          bzero(&af, sizeof(af));
          af.type = SPELL_NOAUCTION;
          af.duration = 2;
          af.modifier = 4000;
          affect_to_char(ch, &af);

          //Battlemage robe
          if (obj_index[temp_obj->R_num].virtual_number == 400218 && !IS_MULTICLASS_PC(ch))
          {
            affect_from_char(ch, SPELL_BATTLEMAGE);
            send_to_char("&+rAs you remove the &+Ymaje&+rst&+Yic &+Yrobe&+r, you feel your enhanced &+mpower&+r fade.&n\r\n", ch);
          }

	        if (naked == TRUE)
            naked = FALSE;
        }
        // Parse Remaining Messages
        switch(ret_type)
        {
          case REMOVE_CURSED:
            act("$p won't budge!  Perhaps it's cursed?!?", TRUE, ch, ch->equipment[k], 0, TO_CHAR);
            naked = FALSE;
      	    break;
	        case REMOVE_BREAK_ENCHANT:
            act("&+cSome of your &+Cmagic&+c dissipates...&n", FALSE, ch, 0, 0, TO_CHAR);
            break;
          case REMOVE_CANT_CARRY:
            send_to_char("You can't carry that many items.\r\n", ch);
            break;
        } // End Switch

        // Break Out of Loop on Full Inventory
        if( ret_type == REMOVE_CANT_CARRY )
          break;
      } // End Loop

      // Give Appropriate Attire Change Messages
      if( naked == TRUE && ret_type != REMOVE_CANT_CARRY )
      {
        send_to_char("You are quite naked at the moment.\r\n", ch);
      }
      else
      {
        act("$n&n removes all of $s equipment.", TRUE, ch, 0, 0, TO_ROOM);
      }
    } // End Remove All
    else
    {
     // Single Object Remove
      obj_object = get_object_in_equip(ch, Gbuf1, &j);
      ret_type = remove_item(ch, obj_object, j);
      // Acknowledge Removal
      if( ret_type == REMOVE_SUCCESS || ret_type == REMOVE_BREAK_ENCHANT )
      {
        act("You stop using $p.", FALSE, ch, obj_object, 0, TO_CHAR);
        act("$n stops using $p.", TRUE, ch, obj_object, 0, TO_ROOM);

     	  affect_from_char(ch, SPELL_NOAUCTION);
        bzero(&af, sizeof(af));
        af.type = SPELL_NOAUCTION;
        af.duration = 2;
        af.modifier = 4000;
        affect_to_char(ch, &af);

        //Battlemage robe
        if (obj_index[obj_object->R_num].virtual_number == 400218 && !IS_MULTICLASS_PC(ch))
        {
          affect_from_char(ch, SPELL_BATTLEMAGE);
          send_to_char("&+rAs you remove the &+Ymaje&+rst&+Yic &+Yrobe&+r, you feel your enhanced &+mpower&+r fade.&n\r\n", ch);
        }
      }

      // Parse Remaining Messages
      switch( ret_type )
      {
        case REMOVE_CURSED:
          act("$p won't budge!  Perhaps it's cursed?!?", TRUE, ch, obj_object, 0, TO_CHAR);
          break;
        case REMOVE_BREAK_ENCHANT:
          act("&+cAs you remove the item, the &+Cenchantment &+cis broken...&n", FALSE, ch, obj_object, 0, TO_CHAR);
          break;
        case REMOVE_CANT_CARRY:
          send_to_char("You can't carry that many items.\r\n", ch);
          break;
        case REMOVE_NOT_USING:
          send_to_char("You are not using it.\r\n", ch);
          break;
      } // End Switch
    } // End Single Object REmove
  }
  else // No Argument
  {
    send_to_char("Remove what?\r\n", ch);
  }

  // Make Proper Adjustments for Changed Affects
  balance_affects(ch);
  if (was_invis && !IS_SET(ch->specials.affected_by, AFF_INVISIBLE) && !IS_SET(ch->specials.affected_by2, AFF2_MINOR_INVIS))
  {
    act("$n snaps into visibility.", FALSE, ch, 0, 0, TO_ROOM);
    act("You snap into visibility.", FALSE, ch, 0, 0, TO_CHAR);
  }

  // Calibrate Lighting
  char_light(ch);
  room_light(ch->in_room, REAL);
}

/* support for do_search */
bool find_chance(P_char ch)
{
  if (IS_TRUSTED(ch))
    return TRUE;

   if (has_innate(ch, INNATE_DRAGONMIND) && number(0,1))
     return TRUE;
     
  if (((GET_C_INT(ch) + GET_C_WIS(ch) + GET_C_LUK(ch)) / 3) > number(1, 101))
    return TRUE;
  return FALSE;
}

bool is_salvageable(P_obj temp)
{

  if( OBJ_VNUM(temp) > 400237 && OBJ_VNUM(temp) < 400259 )
  {
    return TRUE;
  }

  // Make sure its not food or container
  if( (temp->type == ITEM_CONTAINER
    || temp->type == ITEM_STORAGE) && temp->contains )
  {
    return FALSE;
  }

  if( IS_SET(temp->extra_flags, ITEM_NOSELL) )
  {
    return FALSE;
	}

  if( IS_SET(temp->extra2_flags, ITEM2_SOULBIND) )
  {
    return FALSE;
  }

  if( temp->type == ITEM_WAND )
  {
    return FALSE;
  }

  switch( OBJ_VNUM(temp) )
  {
    case VOBJ_RANDOM_ARMOR:
    case VOBJ_RANDOM_THRUSTED:
    case VOBJ_RANDOM_WEAPON:
    case  98:
    case 352:
    case 366:
      return FALSE;
  }

  if( temp->type == ITEM_FOOD )
  {
    return FALSE;
  }

  if( temp->type == ITEM_TREASURE || temp->type == ITEM_POTION || temp->type == ITEM_MONEY || temp->type == ITEM_KEY )
  {
    return FALSE;
  }
  if( IS_OBJ_STAT2(temp, ITEM2_STOREITEM) )
   {
    return FALSE;
   }
  if( IS_SET(temp->extra_flags, ITEM_ARTIFACT) )
  {
    return FALSE;
  }
  if( (temp->type == ITEM_STAFF) && (temp->value[3] > 0) )
  {
    return FALSE;
  }

  return TRUE;
}


void do_salvage(P_char ch, char *argument, int cmd)
{
  static bool DEBUG = TRUE;
  P_obj item, salvaged, recipe;
  char  first_arg[MAX_INPUT_LENGTH], buf1[MAX_STRING_LENGTH];
  char  debugBuf[MAX_STRING_LENGTH];
  int   itemvnum, itemval, lowest, matvnum;
  int   newcost, reciperoll;
  int   scitools = vnum_in_inv(ch, VOBJ_EPIC_LANTAN_TOOLS);
  int   playerroll;
  float modifier;

  one_argument(argument, first_arg);

  if( IS_TRUSTED(ch) && !strcmp(first_arg, "debug") )
  {
    DEBUG = !DEBUG;
    debug( "do_salvage: DEBUG turned %s.", DEBUG ? "ON" : "OFF" );
    return;
  }

  if(GET_CHAR_SKILL(ch, SKILL_SALVAGE) < 1)
  {
    send_to_char("Only &+ycrafters&n have the necessary &+yskill&n to break down &+Witems&n.\n", ch);
    return;
  }

  if (!(item = get_obj_in_list_vis(ch, first_arg, ch->carrying)))
  {
    act("What would you like to salvage?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if( IS_SET(item->extra_flags, ITEM_NODROP) )
  {
    act("But your $q is so pretty.", FALSE, ch, item, 0, TO_CHAR);
    return;
  }

  itemvnum = OBJ_VNUM(item);

  // Handle salvage materials
  if((itemvnum >= LOWEST_MAT_VNUM) && (itemvnum <= HIGHEST_MAT_VNUM))
  {
    lowest = get_matstart(item);
    if(itemvnum == lowest)
    {
      send_to_char("Not possible! That &+ymaterial&n is already of the &+Llowest&n quality.\r\n", ch);
      return;
    }
    if( lowest <= 0 )
    {
      send_to_char( "Could not figure out what this is made out of !?  Can bug it if you want.\n\r", ch );
      logit( LOG_DEBUG, "Couldn't get start material for object: '%s' %d.", item->short_description, itemvnum );
      return;
    }

    obj_to_char(read_object(--itemvnum, VIRTUAL), ch);
    obj_to_char(read_object(itemvnum, VIRTUAL), ch);
    act("$n breaks down their $p into its &+ylesser&n material...", TRUE, ch, item, 0, TO_ROOM);
    act("You break down your $p into its &+ylesser &+Ymaterial&n...", FALSE, ch, item, 0, TO_CHAR);
    obj_from_char(item);
    extract_obj(item);
    return;
  }

  if( !is_salvageable(item) )
  {
    act("That item cannot be &+ysalvaged&n.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if( GET_CHAR_SKILL(ch, SKILL_SALVAGE) < number(1, 105) && scitools < 1 )
  {
    act("&+LYou attempt to break down your $p&+L, but end up &+Rbreaking &+Lit in the process.", FALSE, ch, item, 0, TO_CHAR);
    act("$n attempts to salvage their $p, but clumsily destroys it.", TRUE, ch, item, 0, TO_ROOM);
    extract_obj(item);
    notch_skill(ch, SKILL_SALVAGE, 10);
    return;
  }

  act("$n begins to tear down their $p into its core components...", TRUE, ch, item, 0, TO_ROOM);
  act("You begin breaking down your $p into its &+yraw &+Ymaterials&n...", FALSE, ch, item, 0, TO_CHAR);

  itemval = itemvalue(item);

  if( (itemval <= 5) && (number(1, 1000) > GET_C_LUK(ch)) )
  {
    send_to_char("The &+ypoor &nquality and &+Lcraftsmanship&n of the item yield to your force, &+Rbreaking&n the item into unusable bits.\r\n", ch);
    extract_obj(item);
    return;
  }

  // Find base material via item's material.
  // Note: these are in order of material type (MAT_NONSUBSTANTIAL = 1, MAT_FLESH = 2, ... )
  //   Also, the matvnums are in order of rarity of material (value from cheapest to most expensive)
  //     ie. MAT_NONSUBSTANTIAL = most expensive @ 400205, MAT_FEATHER is cheapest @ 400000.
  switch( item->material )
  {
    case MAT_NONSUBSTANTIAL:
      matvnum = 400205;
      break;
    case MAT_FLESH:
      matvnum = 400005;
      break;
    case MAT_CLOTH:
      matvnum = 400015;
      break;
    case MAT_BARK:
      matvnum = 400035;
      break;
    case MAT_SOFTWOOD:
      matvnum = 400040;
      break;
    case MAT_HARDWOOD:
      matvnum = 400050;
      break;
    //case MAT_SILICON:
      //matvnum = 67283;
      //break;
    case MAT_CRYSTAL:
      matvnum = 400090;
      break;
    //case MAT_CERAMIC:
      //matvnum = 67283;
      //break;
    case MAT_BONE:
      matvnum = 400065;
      break;
    case MAT_STONE:
      matvnum = 400095;
      break;
    case MAT_HIDE:
      matvnum = 400030;
      break;
    case MAT_LEATHER:
      matvnum = 400045;
      break;
    case MAT_CURED_LEATHER:
      matvnum = 400060;
      break;
    case MAT_IRON:
      matvnum = 400110;
      break;
    case MAT_STEEL:
      matvnum = 400120;
      break;
    case MAT_BRASS:
      matvnum = 400125;
      break;
    case MAT_MITHRIL:
      matvnum = 400185;
      break;
    case MAT_ADAMANTIUM:
      matvnum = 400195;
      break;
    case MAT_BRONZE:
      matvnum = 400130;
      break;
    case MAT_COPPER:
      matvnum = 400135;
      break;
    case MAT_SILVER:
      matvnum = 400140;
      break;
    case MAT_ELECTRUM:
      matvnum = 400145;
      break;
    case MAT_GOLD:
      matvnum = 400150;
      break;
    case MAT_PLATINUM:
      matvnum = 400180;
      break;
    case MAT_GEM:
      matvnum = 400155;
      break;
    case MAT_DIAMOND:
      matvnum = 400190;
      break;
    //case MAT_LEAVES:
      //matvnum = 67283;
      //break;
    case MAT_RUBY:
      matvnum = 400165;
      break;
    case MAT_EMERALD:
      matvnum = 400160;
      break;
    case MAT_SAPPHIRE:
      matvnum = 400170;
      break;
    case MAT_IVORY:
      matvnum = 400070;
      break;
    case MAT_DRAGONSCALE:
      matvnum = 400200;
      break;
    case MAT_OBSIDIAN:
      matvnum = 400175;
      break;
    case MAT_GRANITE:
      matvnum = 400100;
      break;
    case MAT_MARBLE:
      matvnum = 400105;
      break;
    //case MAT_LIMESTONE:
      //matvnum = 67283;
      //break;
    case MAT_BAMBOO:
      matvnum = 400055;
      break;
    case MAT_REEDS:
      matvnum = 400010;
      break;
    case MAT_HEMP:
      matvnum = 400020;
      break;
    case MAT_GLASSTEEL:
      matvnum = 400115;
      break;
    case MAT_CHITINOUS:
      matvnum = 400080;
      break;
    case MAT_REPTILESCALE:
      matvnum = 400085;
      break;
    case MAT_RUBBER:
      matvnum = 400025;
      break;
    case MAT_FEATHER:
      matvnum = 400000;
      break;
    case MAT_PEARL:
      matvnum = 400075;
      break;
    default:
      act("&+wYou cant seem to find anything worth &+ysalvaging&+w on that item.&n", FALSE, ch, 0, 0, TO_CHAR);
      return;
      break;
  }

  // Grant Rewards based on ival of item.
  // Note: This only works because all of the materials of the same type are in sequential order.
  //   ie. Feathers are 400000, 400001, 400002, 400003, 400004 and Hemp is 400020, -021, -022, -023, -024.
  if( itemval <= 5 )
  {
    act("&+wYou were able to salvage a rather &+rpoor&n material from your item...", FALSE, ch, 0, 0, TO_CHAR);
  }
  else if( itemval <= 10 )
  {
    matvnum++;
    act("&+wYour focused efforts allow you to salvage a &+ycommon&n material from your item...", FALSE, ch, 0, 0, TO_CHAR);
  }
  else if( itemval <= 15 )
  {
    matvnum += 2;
    act("&+wYou study your item as you break it down, and come away with a rather &+Yuncommon &nmaterial.", FALSE, ch, 0, 0, TO_CHAR);
  }
  else if( itemval <= 20 )
  {
    matvnum += 3;
    act("&+wYou make quick work of your item, salvaging a precious &+crare &nmaterial from it...", FALSE, ch, 0, 0, TO_CHAR);
  }
  // craftsmanship > 20
  else
  {
    matvnum += 4;
    act("&+LUsing your ma&+wst&+Wer&+wfu&+Ll &+Wskill&+L, you delicately break apart your item, salvaging a quite &+Munique &+Lmaterial from it...", FALSE, ch, 0, 0, TO_CHAR);
  }

  // Moved the creation of essences below the checks for valid material types.
  // Get lucky: get tier 4 - Someone should do the math and reduce this to a single comparison.
  if( number(60, 400) < GET_C_LUK(ch) )
  {
    if( number(70, 400) < GET_C_LUK(ch) )
    {
      if( number(80, 500) < GET_C_LUK(ch) )
      {
        obj_to_char(read_object(MAG_ESSENCE_VNUM, VIRTUAL), ch);
        send_to_char("...as you work, a small &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml&n object gently separates from your item!\r\n", ch);
      }
    }
  }

  // It seems like this list is very slim compared to what we might want.
  if( IS_SET(item->bitvector, AFF_STONE_SKIN)    || IS_SET(item->bitvector, AFF_HIDE)
    || IS_SET(item->bitvector, AFF_SNEAK)        || IS_SET(item->bitvector, AFF_FLY)
    || IS_SET(item->bitvector, AFF4_NOFEAR)      || IS_SET(item->bitvector2, AFF2_AIR_AURA)
    || IS_SET(item->bitvector2, AFF2_EARTH_AURA) || IS_SET(item->bitvector3, AFF3_INERTIAL_BARRIER)
    || IS_SET(item->bitvector3, AFF3_REDUCE)     || IS_SET(item->bitvector2, AFF2_GLOBE)
    || IS_SET(item->bitvector, AFF_HASTE)        || IS_SET(item->bitvector, AFF_DETECT_INVISIBLE)
    || IS_SET(item->bitvector4, AFF4_DETECT_ILLUSION) )
  {
    obj_to_char(read_object(MAG_ESSENCE_VNUM, VIRTUAL), ch);
    send_to_char("...as you work, a small &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml&n object gently separates from your item!\r\n", ch);
  }

  // Dynamic pricing - Drannak 3/21/2013
  // between 1 gold, 9 silver and 2 gold 2 silver starting point
  newcost = number( 190, 220 );
  // Since the vnum's are sequential, the greatest rarity gets a 1.3 modifier, lowest gets 100% of value.
  // To do this, we want 400000 to map to 1, and 400209 to map to 1.3:
//    modifier = ((OBJ_VNUM(salvaged) - LOWEST_MAT_VNUM) * 0.3) / (HIGHEST_MAT_VNUM - LOWEST_MAT_VNUM) + 1;
  // However, 200 * 1.3 = only 2 gold, 6 silver.  We want this to be much more profitable, so, instead of
  //   mapping to 1.3, we want to map to 13 -> 2 plat, 6 gold; we set the multiplier to 13 - 1 = 12.
  modifier = ((matvnum - LOWEST_MAT_VNUM) * 12.0) / (float)(HIGHEST_MAT_VNUM - LOWEST_MAT_VNUM) + 1.0;
  if( DEBUG )
    snprintf(debugBuf, MAX_STRING_LENGTH, "do_salvage: Newcost(initial): %d, Modifier: %.3f", newcost, modifier );
  newcost = (int)((float)newcost * modifier);
  if( DEBUG )
    snprintf(debugBuf + strlen(debugBuf), MAX_STRING_LENGTH - strlen(debugBuf), ", Newcost(mod): %d", newcost );
  newcost = (newcost * GET_LEVEL(ch)) / 56;
  if( DEBUG )
    snprintf(debugBuf + strlen(debugBuf), MAX_STRING_LENGTH - strlen(debugBuf), ", Newcost(lvl): %d", newcost );
  newcost = (newcost * GET_CHAR_SKILL(ch, SKILL_SALVAGE) / 100);
  if( DEBUG )
    snprintf(debugBuf + strlen(debugBuf), MAX_STRING_LENGTH - strlen(debugBuf), ", Newcost(skill): %d", newcost );

  // 67% chance to get 2 salvaged materials.
  if( !number(0, 2) )
  {
    act("&+w...and at least you &+ysalvaged&n a decent amount.", FALSE, ch, 0, 0, TO_CHAR);
    salvaged = read_object(matvnum, VIRTUAL);
    if( number(80, 140) < GET_C_LUK(ch) )
    {
      send_to_char("&+mYou &+Ygently&+m break the first &+Mmaterial &+mfree, preserving its natural form.&n\r\n", ch);
      salvaged->cost = (13 * newcost) / 10;
    }
    else
      salvaged->cost = newcost;

    obj_to_char(salvaged, ch);

    if( DEBUG )
    {
      snprintf(debugBuf + strlen(debugBuf), MAX_STRING_LENGTH - strlen(debugBuf), ", Final cost: %d.", salvaged->cost );
      debug( debugBuf );
    }

    salvaged = read_object(matvnum, VIRTUAL);
    // Don't bother recalculating, just add a bit of randomness (1 copper variance per 1 gold value).
    newcost += number( -newcost / 100, newcost / 100 );

    if( number(80, 140) < GET_C_LUK(ch) )
    {
      send_to_char("&+mYou &+Ygently&+m break the second &+Mmaterial &+mfree, preserving its natural form.&n\r\n", ch);
      salvaged->cost = (13 * newcost) / 10;
    }
    else
      salvaged->cost = newcost;

    obj_to_char(salvaged, ch);
  }
  else
  {
    act("&+w...and you only came up with a single piece of &+ymaterial&n.", FALSE, ch, 0, 0, TO_CHAR);
    salvaged = read_object(matvnum, VIRTUAL);

    if(number(80, 140) < GET_C_LUK(ch))
    {
      send_to_char("&+mYou &+Ygently&+m break the &+Mmaterial &+mfree, preserving its natural form.&n\r\n", ch);
      salvaged->cost = (13 * newcost) / 10;
    }
    else
      salvaged->cost = newcost;

    obj_to_char(salvaged, ch);

    if( DEBUG )
    {
      snprintf(debugBuf + strlen(debugBuf), MAX_STRING_LENGTH - strlen(debugBuf), ", Final cost: %d.", salvaged->cost );
      debug( debugBuf );
    }
  }

  notch_skill(ch, SKILL_SALVAGE, 4);

  reciperoll = number(1, 10000);

  if( itemval <= 5 )
  {
    reciperoll /= 3;
  }
  else if( itemval <= 10 )
  {
    reciperoll /= 2;
  }
  else if( itemval <= 15 )
  {
    reciperoll = (reciperoll * 2) / 3;
  }
  else if( itemval >= 100 )
  {
    // 100k -> autofail (playerroll will be around 500 or so at max).
    reciperoll = 100000;
  }

  playerroll = GET_C_LUK(ch) + GET_LEVEL(ch)*2 + GET_CHAR_SKILL(ch, SKILL_SALVAGE);
  if( scitools > 0 )
  {
    send_to_char("&+yYou make sure to utilize your &+cset of &+rLantan &+CScientific &+LTools &+yas you break apart your item...\r\n", ch);
    // (1-10000)/15 -> 0-667
    reciperoll /= 15;
    // (80-100 + 2-112 + 1-100) * 2 -> (83-312)*2 -> 166-624
    playerroll *= 2;
    vnum_from_inv(ch, VOBJ_EPIC_LANTAN_TOOLS, 1);
  }

  /*** CREATE RECIPE ***/
  if( reciperoll < playerroll )
  {
    if( DEBUG )
      debug("do_salvage: player: '%s' - reciperoll: %d, playerroll: %d, scitools: %d.",
        J_NAME(ch), reciperoll, playerroll, scitools);

    if( itemvnum == VOBJ_RANDOM_ARMOR || itemvnum == VOBJ_RANDOM_THRUSTED || itemvnum == VOBJ_RANDOM_WEAPON )
    {
     debug( "do_salvage: player: '%s' Not creating recipe for random item %d.", J_NAME(ch), itemvnum );
      if( scitools )
      {
        act("With your tools, you discover that $p can not be manufactured.", FALSE, ch, item, 0, TO_CHAR);
      }
    }
    else
    {
      recipe = read_object(SALVAGE_RECIPE_VNUM, VIRTUAL);

      SET_BIT(recipe->value[6], itemvnum);
      snprintf(buf1, MAX_STRING_LENGTH, "%s %s", recipe->short_description, item->short_description);
      recipe->short_description = str_dup(buf1);
      recipe->str_mask |= STRUNG_DESC2;

      obj_to_char(recipe, ch);
      if( DEBUG )
        debug( "do_salvage: %s created '%s'.", J_NAME(ch), recipe->short_description );
      act("As $n breaks down their $p, they are suddenly &+Yenlightened&n!\n"
        "$n quickly grabs a quill and &+yvellum paper&n and starts to write down the &+Cdetailed&n\n"
        "intricacies surrounding $p.\r\n", FALSE, ch, item, 0, TO_ROOM);
      act("As you break down your $p, you are suddenly &+Yenlightened&n!\n"
        "You quickly grab a quill and &+yvellum paper&n and start to write down the &+Cdetailed&n\n"
        "intricacies surrounding $p.\r\n", FALSE, ch, item, 0, TO_CHAR);
      act("$n has created $p!\r\n", FALSE, ch, recipe, 0, TO_ROOM);
      act("You have created $p!\r\n", FALSE, ch, recipe, 0, TO_CHAR);
    }
  }
  /*** END CREATE RECIPE ***/

  if( DEBUG )
    debug( "do_salvage: player: '%s&n' just salvaged '%s&n' (%d) at [%d]!",
      J_NAME(ch), OBJ_SHORT(item), itemvnum, ROOM_VNUM(ch->in_room) );
  extract_obj(item);
  char_light(ch);
  room_light(ch->in_room, REAL);
}


void do_search(P_char ch, char *argument, int cmd)
{
  P_char   dummy;
  P_obj    k;
  bool     found_something = FALSE;
  char     name[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH],
    buf2[MAX_STRING_LENGTH];
  int      door;
  bool     proc_handled = FALSE;

  one_argument(argument, name);

  if (!*name)
  {                             /* No argument: search room */
    k = world[ch->in_room].contents;
    /* resources first, as they aren't true objects in the room */
/*    if (world[ch->in_room].resources && find_chance(ch)) {
      sprintbit(world[ch->in_room].resources, resource_list, buf);
      snprintf(buf2, MAX_STRING_LENGTH, "The area appears to be rich in: %s\r\n", buf);
      send_to_char(buf2, ch);
//      found_something = TRUE;
    }*/
  }
  else
  {
    if( IS_TRUSTED(ch) )
    {
      generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &dummy, &k);
    }
    else
    {
      generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &dummy, &k);
    }

    if (!k)
    {
      send_to_char("You don't find anything you didn't see before.\r\n", ch);
      return;
    }
    else if (k->trap_charge &&
             ((GET_CLASS(ch, CLASS_ROGUE) && find_chance(ch)) ||
              IS_TRUSTED(ch)))
    {
      act("Something about $p makes you leery to continue.", TRUE, ch, k, 0,
          TO_CHAR);
      return;
    }
    else if ((k->type != ITEM_CONTAINER) &&
             (k->type != ITEM_STORAGE) &&
             (k->type != ITEM_CORPSE) && (k->type != ITEM_QUIVER))
    {
      send_to_char("You don't find anything you didn't see before.\r\n", ch);
      return;
    }
    if ((k->type != ITEM_CORPSE) && IS_SET(k->value[1], CONT_CLOSED))
    {
      send_to_char("Opening it would probably improve your chances.\r\n", ch);
      return;
    }
    k = k->contains;
  }

  for (; k && (!found_something || IS_TRUSTED(ch)); k = k->next_content)
  {
    if (k->trap_charge &&
        (IS_TRUSTED(ch) || (GET_CLASS(ch, CLASS_ROGUE) && find_chance(ch))) &&
        k->trap_eff && IS_SET(k->trap_eff, 1))
    {
      send_to_char("A small trip wire along the ground catches your eye.\r\n",
                   ch);
      found_something = TRUE;
    }
    else if (IS_SET(k->extra_flags, ITEM_SECRET) && find_chance(ch))
    {
      REMOVE_BIT(k->extra_flags, ITEM_SECRET);
      if (CAN_SEE_OBJ(ch, k))
      {                         /*
                                 * Was it found?
                                 */
        if (obj_index[k->R_num].func.obj)
        {
          proc_handled =
            (*obj_index[k->R_num].func.obj) (k, ch, CMD_FOUND, NULL);
        }
        if (!proc_handled)
        {
          act("You find $p!", FALSE, ch, k, 0, TO_CHAR);
          act("$n finds $p!", FALSE, ch, k, 0, TO_ROOM);
        }
        found_something = TRUE;
      }
      else                      /*
                                 * Make it secret again
                                 */
        SET_BIT(k->extra_flags, ITEM_SECRET);
    }
  }

  /*
   * support for secret exits -DCL
   * but only when searching room, not containers in room. JAB
   * Same goes for searching containers vs hidden chars.
   */
  if (!*name)
  {
    for( door = 0; (door < NUM_EXITS) && (!found_something || IS_TRUSTED(ch)); door++ )
    {
      if( EXIT(ch, door) && IS_SET(EXIT(ch, door)->exit_info, EX_SECRET)
        && !IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED) )
      {
        if (find_chance(ch))
        {
          act("You find a secret entrance!", FALSE, ch, 0, 0, TO_CHAR);
          act("$n finds a secret entrance!", FALSE, ch, 0, 0, TO_ROOM);
          found_something = TRUE;
          REMOVE_BIT(EXIT(ch, door)->exit_info, EX_SECRET);
        }
      }
    }
    /*
     * new bit, give them chance to find hiding thieves/mobs
     */
    for( dummy = world[ch->in_room].people; dummy && (!found_something || IS_TRUSTED(ch)); dummy = dummy->next_in_room )
    {
      if (IS_AFFECTED(dummy, AFF_HIDE) && find_chance(ch) && !number(0, 3))
      {
        REMOVE_BIT(dummy->specials.affected_by, AFF_HIDE);
        if (CAN_SEE(ch, dummy))
        {
          act("You find $N lurking here!", FALSE, ch, 0, dummy, TO_CHAR);
          act("$n points out $N lurking here!", FALSE, ch, 0, dummy, TO_NOTVICT);
          if (find_chance(dummy) && !number(0, 3))
          {
            act("You think $n has spotted you!", TRUE, ch, 0, dummy, TO_VICT);
          }
          found_something = TRUE;
        }
        else
        {
          SET_BIT(dummy->specials.affected_by, AFF_HIDE);
        }
      }
    }
  }

  if (!found_something)
    send_to_char("You don't find anything you didn't see before.\r\n", ch);

  /*
   * temp, until I add the command timing thing. JAB
   */
  CharWait(ch, PULSE_VIOLENCE);
}

void do_apply_poison(P_char ch, char *argument, int cmd)
{
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  P_obj    weapon, poison;

  if (!GET_CHAR_SKILL(ch, SKILL_APPLY_POISON))
  {
    send_to_char("But you don't know how!\r\n", ch);
    return;
  }
  argument_interpreter(argument, Gbuf1, Gbuf2);

  if (!*Gbuf1 || !*Gbuf2)
  {
    send_to_char("Apply poison from what to what weapon?\r\n", ch);
    return;
  }
  if (!(weapon = get_obj_in_list_vis(ch, Gbuf2, ch->carrying)))
  {
    act("You don't have any such weapon!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!(poison = get_obj_in_list_vis(ch, Gbuf1, ch->carrying)))
  {
    act("You don't have any such container!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (poison->type != ITEM_DRINKCON)
  {
    act("But $p can not contain any liquids!", FALSE, ch, poison, 0, TO_CHAR);
    return;
  }
  if (poison->value[2] != LIQ_POISON || !poison->value[3])
  {
    act("Alas, your $q does not contain any &+gpo&+Gi&+gs&+Gon&n!", FALSE, ch, poison, 0,
        TO_CHAR);
    return;
  }
  if (weapon->type != ITEM_WEAPON && !IS_DART(weapon))
  {
    act("Alas, $p is not a weapon!", FALSE, ch, weapon, 0, TO_CHAR);
    return;
  }
  if (poison->value[1] > 0)
  {
    weight_change_object(poison, -1);
    poison->value[1]--;
  }
  else
  {
    act("Your $q is empty!", FALSE, ch, poison, 0, TO_CHAR);
    return;
  }

  weapon->value[4] = poison->value[3];
  act("Your $q now drips with a nasty &+gpo&+Gi&+gs&+Gon&n!", FALSE, ch, weapon, 0,
      TO_CHAR);
  act("$n applies a vile-looking substance to $s $q!", FALSE, ch, weapon, 0,
      TO_ROOM);

  notch_skill(ch, SKILL_APPLY_POISON, 10);

  if (poison->value[1] < 1)
  {                             /* The last bit */
    poison->value[2] = 0;
    poison->value[3] = 0;
    name_from_drinkcon(poison);
  }
  /* Check to see if object is of type TRANSIENT and empty */

  if (poison->value[1] <= 0 && IS_SET(poison->extra_flags, ITEM_TRANSIENT))
  {
    act("The empty $q vanishes in thin air!", FALSE, ch, poison, 0, TO_CHAR);
    obj_from_char(poison);
    extract_obj(poison, TRUE); // Transient artifact poison?
  }

  return;
}

void list_foods()
{
  for (int i = 0; i < 10000000; i++)
  {
    if (P_obj obj = read_object(i, VIRTUAL))
    {
      if (GET_ITEM_TYPE(obj) == ITEM_FOOD)
      {
        char mark1 = ' ', mark2 = ' ', mark3 = ' ';
        if (obj->value[1] != 0 || obj->value[2] != 0 || 
            obj->value[4] != 0 || obj->value[5] != 0 || 
            obj->value[6] != 0 || obj->value[7] != 0)
        {
          mark1 = '*';
        }
        if (obj->value[3] != 0 || obj->value[1] < 0)
        {
          mark1 = 'X';
        }
        if (obj->value[1] > 3 || obj->value[2] > 3)
        {
          mark2 = '*';
        }

        if (obj->value[1] > 8 || obj->value[2] > 8)
        {
          mark3 = '*';
        }

        logit(LOG_SHIP, "%c%c%c %8d: time=%-3d hit=%-2d mov=%-2d poi=%-2d strcon=%-2d agidex=%-2d intwis=%-2d hitdam=%-2d cost=%-6d  : %s",  
            mark1, mark2, mark3, i, 
            obj->value[0],
            obj->value[1],
            obj->value[2],
            obj->value[3],
            obj->value[4],
            obj->value[5],
            obj->value[6],
            obj->value[7],
            obj->cost,
            strip_ansi(obj->short_description ? obj->short_description : "None").c_str());
      }
    }
  }
}

void do_empty(P_char ch, char *argument, int cmd)
{
  P_char unused_ch;
  P_obj obj1, obj2, content;
  char objname[MAX_STRING_LENGTH];
  int count;

  argument = one_argument( argument, objname);
  if( objname[0] == '\0' || !strcmp(objname, "?") )
  {
    send_to_char( "&+YSyntax: &+wempty <container1> <container2>&n\n", ch );
    send_to_char( "Empties the contents of &+w<container1>&n into &+w<container2>&n.\n", ch );
    return;
  }

  generic_find( objname, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &unused_ch, &obj1 );
  if( !obj1 )
  {
    send_to_char_f( ch, "Could not find container1 '%s'.\n", objname );
    send_to_char( "&+YSyntax: &+wempty <container1> <container2>&n\n", ch );
    return;
  }
  if( obj1->type != ITEM_CONTAINER )
  {
    act( "$p is not a container.", FALSE, ch, obj1, NULL, TO_CHAR );
    send_to_char( "&+YSyntax: &+wempty <container1> <container2>&n\n", ch );
    return;
  }
  if( IS_SET(obj1->value[1], CONT_CLOSED) )
  {
    act( "$p is closed.", FALSE, ch, obj1, NULL, TO_CHAR );
    return;
  }
  if( obj1->contains == NULL )
  {
    act( "$p is already empty.\n", FALSE, ch, obj1, NULL, TO_CHAR );
    return;
  }

  one_argument( argument, objname);
  generic_find( objname, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &unused_ch, &obj2 );
  if( !obj2 )
  {
    send_to_char_f( ch, "Could not find container2 '%s'.\n", objname );
    send_to_char( "&+YSyntax: &+wempty <container1> <container2>&n\n", ch );
    return;
  }
  if( IS_SET(obj2->value[1], CONT_CLOSED) )
  {
    act( "$p is closed.", FALSE, ch, obj2, NULL, TO_CHAR );
    return;
  }

  if( obj1 == obj2 )
  {
    act( "I'll put &+Wyou&n in $p!", FALSE, ch, obj2, NULL, TO_CHAR );
    return;
  }

  count = 0;
  while( (content = obj1->contains) != NULL )
  {

    if( (( GET_OBJ_WEIGHT(obj2) + GET_OBJ_WEIGHT(content) ) > obj1->value[0]) && (obj1->value[0] != -1) )
    {
      act( "$P will not fit in $p.", FALSE, ch, obj2, (void *)content, TO_CHAR );
      send_to_char_f( ch, "You moved %d item%s from %s to %s.\n", count, count == 1 ? "" : "s",
        OBJ_SHORT(obj1), OBJ_SHORT(obj2) );
      return;
    }

#if USE_SPACE
    if( (( GET_OBJ_SPACE(obj2) + GET_OBJ_SPACE(content) ) > obj1->value[3]) && (obj1->value[3] != -1) )
    {
      act( "$P will not fit in $p.", FALSE, ch, obj2, (void *)content, TO_CHAR );
      send_to_char_f( ch, "You moved %d item%s from %s to %s.\n", count, count == 1 ? "" : "s",
        OBJ_SHORT(obj1), OBJ_SHORT(obj2) );
      return;
    }
#endif

    obj_from_obj(content);
    obj_to_obj(content, obj2);
    count++;
  }
  send_to_char_f( ch, "You moved %d item%s from %s to %s.\n", count, count == 1 ? "" : "s",
    OBJ_SHORT(obj1), OBJ_SHORT(obj2) );
}
