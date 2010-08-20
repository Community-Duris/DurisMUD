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
#include "justice.h"
#include "objmisc.h"
#include "necromancy.h"
#include "sql.h"

/*
 * external variables
 */

extern P_desc descriptor_list;
extern P_index obj_index;
extern P_room world;
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
  if (!IS_SET(obj->extra_flags, ITEM_TWOHANDS) ||
      (IS_GIANT(ch) && obj->type == ITEM_WEAPON))
    return 1;
  else
    return 2;
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

  if (!o_obj || 
      !(ch))
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
    GET_PLATINUM(ch) += (got_p = o_obj->value[3]);      //(got_p = MAX(0, MIN(o_obj->value[3], CAN_CARRY_COINS(ch))));
//    notall += (got_p != o_obj->value[3]);
    o_obj->value[3] = 0;        //-= got_p;

    GET_GOLD(ch) += (got_g = o_obj->value[2]);  //(got_g = MAX(0, MIN(o_obj->value[2], CAN_CARRY_COINS(ch))));
//    notall += (got_g != o_obj->value[2]);
    o_obj->value[2] = 0;        //-= got_g;

    GET_SILVER(ch) += (got_s = o_obj->value[1]);        //(got_s = MAX(0, MIN(o_obj->value[1], CAN_CARRY_COINS(ch))));
//    notall += (got_s != o_obj->value[1]);
    o_obj->value[1] = 0;        //-= got_s;

    GET_COPPER(ch) += (got_c = o_obj->value[0]);        //(got_c = MAX(0, MIN(o_obj->value[0], CAN_CARRY_COINS(ch))));
//    notall += (got_c != o_obj->value[0]);
    o_obj->value[0] = 0;        //-= got_c;

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

      sql_log(ch, PLAYERLOG, "Got %s from %s.",
        coin_stringv(total_value),
        OBJ_NOWHERE(o_obj) ? "NOWHERE!!" : OBJ_ROOM(o_obj) ? "room" :
        OBJ_INSIDE(o_obj) ? o_obj->loc.inside->name :
        OBJ_CARRIED(o_obj) ? GET_NAME(o_obj->loc.carrying) :
        GET_NAME(o_obj->loc.wearing));
      
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
      sprintf(Gbuf3, "You got: ");
    else
      sprintf(Gbuf3, "There were: ");
    if (got_p)
      sprintf(Gbuf3 + strlen(Gbuf3), "%d &+Wplatinum&N coin%s, ", got_p,
              ((got_p > 1) ? "s" : ""));
    if (got_g)
      sprintf(Gbuf3 + strlen(Gbuf3), "%d &+Ygold&N coin%s, ", got_g,
              ((got_g > 1) ? "s" : ""));
    if (got_s)
      sprintf(Gbuf3 + strlen(Gbuf3), "%d silver coin%s, ", got_s,
              ((got_s > 1) ? "s" : ""));
    if (got_c)
      sprintf(Gbuf3 + strlen(Gbuf3), "%d &+ycopper&N coin%s, ", got_c,
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
        if (OBJ_CARRIED(s_obj))
        {
          GET_CARRYING_W(ch) -= GET_OBJ_WEIGHT(s_obj);
          obj_from_obj(o_obj);
          GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(s_obj);
        }
        else
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
      extract_obj(o_obj, FALSE);
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
    if (OBJ_CARRIED(s_obj))
    {
      GET_CARRYING_W(ch) -= GET_OBJ_WEIGHT(s_obj);
      obj_from_obj(o_obj);
      GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(s_obj);
    }
    else
      obj_from_obj(o_obj);
    obj_to_char(o_obj, ch);

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
  }
  else
  {
    obj_from_room(o_obj);
    obj_to_char(o_obj, ch);
    act("You get $p.", 0, ch, o_obj, 0, TO_CHAR);
    if (showit && !slip)
      act("$n gets $p.", 1, ch, o_obj, 0, TO_ROOM);
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
  P_char   hood = NULL, owner = NULL;
  P_obj    s_obj = NULL, o_obj = NULL, next_obj;
  bool     found = FALSE, fail = FALSE, corpse_flag = FALSE, alldot = FALSE,
    carried;
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

  if(!(ch) ||
    !IS_ALIVE(ch))
  {
    return;
  }

  if(IS_IMMOBILE(ch))
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

  if(IS_AFFECTED(ch, AFF_WRAITHFORM))
  {
    send_to_char("You are too intangible!\r\n", ch);
    return;
  }

  if(IS_ANIMAL(ch))
  {
    send_to_char("You are a beast.!\r\n", ch);
    return;
  }

  argument_interpreter(argument, arg1, arg2);

  if(IS_NPC(ch) &&
    ch->following &&
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

        if ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch))
        {
          if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(o_obj)) <= CAN_CARRY_W(ch))
          {
            if (CAN_WEAR(o_obj, ITEM_TAKE) || GET_LEVEL(ch) >= 60)
            {
              get(ch, o_obj, 0, TRUE);
              total++;
            }
            else
            {
              strcpy(Gbuf4, o_obj->short_description);
              CAP(Gbuf4);
              sprintf(Gbuf3, "%s isn't takeable.\r\n", Gbuf4);
              send_to_char(Gbuf3, ch);
              fail = TRUE;
            }
          }
          else
          {
            strcpy(Gbuf4, o_obj->short_description);
            CAP(Gbuf4);
            sprintf(Gbuf3, "%s is too heavy to lift.\r\n", Gbuf4);
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
      sprintf(Gbuf3, "You got %d items.\r\n", total);
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

    o_obj = get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents);
    if (o_obj)
    {
      /*
       * was object disarmed?  did PC still manage to get it? if so,
       * get object --TAM
       if (check_get_disarmed_obj(ch, o_obj->last_to_hold, o_obj))
       return;
       */

      if (IS_CARRYING_N(ch) < CAN_CARRY_N(ch))
      {
        if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(o_obj)) <= CAN_CARRY_W(ch))
        {
          if (CAN_WEAR(o_obj, ITEM_TAKE) || (GET_LEVEL(ch) >= 60))
          {
            if ((GET_ITEM_TYPE(o_obj) == ITEM_CORPSE) &&
                IS_SET(o_obj->value[1], PC_CORPSE))
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
                send_to_char
                  ("Looting of player corpses requires consent.\r\n", ch);
                return;
              }
            }
            found = TRUE;
            get(ch, o_obj, s_obj, TRUE);

          }
          else
          {
            strcpy(Gbuf4, o_obj->short_description);
            CAP(Gbuf4);
            sprintf(Gbuf3, "%s isn't takeable.\r\n", Gbuf4);
            send_to_char(Gbuf3, ch);
            fail = TRUE;
          }
        }
        else
        {
          strcpy(Gbuf4, o_obj->short_description);
          CAP(Gbuf4);
          sprintf(Gbuf3, "%s is too heavy.\r\n", Gbuf4);
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
      sprintf(Gbuf3, "You do not see a %s here.\r\n", arg1);
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
        if (IS_FIGHTING(ch))
        {
          send_to_char
            ("You're too busy fighting to be pulling things out of bags!\r\n",
             ch);
          return;
        }
        if ((GET_ITEM_TYPE(s_obj) == ITEM_CORPSE) &&
            IS_SET(s_obj->value[CORPSE_FLAGS], PC_CORPSE))
          corpse_flag = 1;
        else
          corpse_flag = 0;
        if (corpse_flag && fight_in_room(ch) && !on_front_line(ch))
        {
          send_to_char
            ("There's too much blood flying around for you to do that!\r\n",
             ch);
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
          if (CAN_SEE_OBJ(ch, o_obj))
          {
            if (IS_CARRYING_N(ch) < CAN_CARRY_N(ch))
            {
              if (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(o_obj)) <
                   CAN_CARRY_W(ch)) || OBJ_CARRIED(s_obj))
              {
                if (CAN_WEAR(o_obj, ITEM_TAKE) || (GET_LEVEL(ch) >= 60))
                {
                  if ((GET_ITEM_TYPE(o_obj) == ITEM_CORPSE) &&
                      IS_SET(o_obj->value[1], PC_CORPSE))
                  {
                    logit(LOG_CORPSE, "%s%s: corpse of %s from %s",
                          GET_NAME(ch), (hood == ch) ? "" : GET_NAME(hood),
                          o_obj->action_description, s_obj->name);
                  }
                  else if (corpse_flag && o_obj)
                  {
                    if (o_obj->type == ITEM_MONEY)
                    {
                      logit(LOG_CORPSE, "%s%s: %dp, %dg, %ds, %dc from %s",
                        GET_NAME(ch), (hood == ch) ? "" : GET_NAME(hood),
                        o_obj->value[3], o_obj->value[2], o_obj->value[1],
                        o_obj->value[0], s_obj->action_description);
                    }
                    else
                    {
                      if (CAN_WEAR(o_obj, ITEM_WEAR_IOUN) ||
                          IS_ARTIFACT(o_obj))
                      {
                        logit(LOG_CORPSE, "%s%s: %s [%d] (ARTIFACT) from %s",
                              GET_NAME(ch),
                              (hood == ch) ? "" : GET_NAME(hood), o_obj->name,
                              obj_index[o_obj->R_num].virtual_number,
                              s_obj->action_description);
                        
                        act("$n gets $P from $p.", 0, ch, s_obj, o_obj, TO_ROOM);
                          
                        //if((GET_LEVEL(ch) > 44) && (s_obj->value[2] > 44)) {
                        if ((s_obj->value[5] != 0) &&
                            ((RACE_GOOD(ch) && (s_obj->value[5] != 1)) ||
                             (RACE_EVIL(ch) && (s_obj->value[5] != 2)) ||
                             (RACE_PUNDEAD(ch) && (s_obj->value[5] != 3))))
                        {
                          int vnum = GET_OBJ_VNUM(o_obj);
                          int owner_pid = -1;
                          int timer = time(NULL);
                          sql_update_bind_data(vnum, &owner_pid, &timer);
                          feed_artifact(ch, o_obj, (int) get_property("artifact.feeding.initial.secs", ( 3600 * 100 ) ), FALSE );
                        }
                      }
                      else
                      {
                        logit(LOG_CORPSE, "%s%s: %s [%d] from %s",
                              GET_NAME(ch),
                              (hood == ch) ? "" : GET_NAME(hood), o_obj->name,
                              obj_index[o_obj->R_num].virtual_number,
                              s_obj->action_description);

                        act("$n gets $P from $p.", 0, ch, s_obj, o_obj, TO_ROOM);

                      }
                    }
                  }
                  get(ch, o_obj, s_obj, FALSE);
/*
      if (corpse_flag == 1 && s_obj->value[4] == 1) {
        strcpy(Gbuf4, s_obj->action_description);
        *Gbuf4 = tolower(*Gbuf4);
        owner = get_char(Gbuf4);
        if (owner) {
          if (!is_linked_to(ch, owner, LNK_CONSENT) &&
              (owner != ch)) {
            if (CHAR_IN_TOWN(ch)) {
              if (GET_CRIME_T(CHAR_IN_TOWN(ch),CRIME_CORPSE_LOOT)) {
                if (GET_ITEM_TYPE(o_obj) != ITEM_MONEY) {
                  o_obj->justice_status = J_OBJ_CORPSE;
                  o_obj->justice_name = str_dup(s_obj->action_description);
                }
                looting = TRUE;
              }
            }
          }
        } else {
          if (CHAR_IN_TOWN(ch)) {
            if (GET_CRIME_T(CHAR_IN_TOWN(ch),CRIME_CORPSE_LOOT)) {
              if (GET_ITEM_TYPE(o_obj) != ITEM_MONEY) {
                o_obj->justice_status = J_OBJ_CORPSE;
                o_obj->justice_name = str_dup(s_obj->action_description);
              }
              looting = TRUE;
            }
          }
        }
      }*/

                  total++;
                  if (GET_ITEM_TYPE(s_obj) == ITEM_QUIVER)
                    if (s_obj->value[3] > 0)
                      s_obj->value[3]--;
                }
                else
                {
                  sprintf(Gbuf3, "%s isn't takeable.\r\n",
                          o_obj->short_description);
                  send_to_char(Gbuf3, ch);
                  fail = TRUE;
                }
              }
              else
              {
                sprintf(Gbuf3, "%s is too heavy.\r\n",
                        o_obj->short_description);
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
          sprintf(Gbuf3, "%s appears to be empty.\r\n",
                  s_obj->short_description);
          send_to_char(Gbuf3, ch);
          fail = TRUE;
        }
        else if (total == 1)
          act("$n gets something from $p.", TRUE, ch, s_obj, 0, TO_ROOM);
        else if ((total > 1) && (total < 6))
          act("$n gets some stuff from $p.", TRUE, ch, s_obj, 0, TO_ROOM);
        else if (total > 5)
          act("$n gets a bunch of stuff from $p.",
              TRUE, ch, s_obj, 0, TO_ROOM);
      }
      else
      {
        sprintf(Gbuf3, "%s is not a container.\r\n",
                s_obj->short_description);
        send_to_char(Gbuf3, ch);
        fail = TRUE;
      }
    }
    else
    {
      sprintf(Gbuf3, "You do not see or have the %s.\r\n", arg2);
      send_to_char(Gbuf3, ch);
      fail = TRUE;
    }
  }

    /* get ??? all */
  if(type == 5)
  {
    send_to_char("You can't take things from two or more containers.\r\n",
                 ch);
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

        if (IS_FIGHTING(ch))
        {
          send_to_char
            ("You're too busy fighting to be pulling things out of bags!\r\n",
             ch);
          return;
        }

        if ((GET_ITEM_TYPE(s_obj) == ITEM_CORPSE) &&
            IS_SET(s_obj->value[CORPSE_FLAGS], PC_CORPSE))
          corpse_flag = 1;
        else
          corpse_flag = 0;

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
          if (IS_CARRYING_N(ch) < CAN_CARRY_N(ch))
          {
            if (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(o_obj)) <
                 CAN_CARRY_W(ch)) || OBJ_CARRIED(s_obj))
            {
              if (CAN_WEAR(o_obj, ITEM_TAKE) || (GET_LEVEL(ch) == 60 &&
                                                 GET_ITEM_TYPE(o_obj) !=
                                                 ITEM_CORPSE))
              {
                /*
                 * SAM 7-94 log corpse looting of
                 * player
                 */
                if((GET_ITEM_TYPE(o_obj) == ITEM_CORPSE) &&
                  IS_SET(o_obj->value[1], PC_CORPSE))
                {
                  logit(LOG_CORPSE, "%s%s: corpse of %s from %s",
                        GET_NAME(ch), (hood == ch) ? "" : GET_NAME(hood),
                        o_obj->action_description, s_obj->name);
                }
                else if (corpse_flag && o_obj)
                {
                  if (GET_ITEM_TYPE(o_obj) == ITEM_MONEY)
                  {
                    logit(LOG_CORPSE, "%s%s: %dp, %dg, %ds, %dc from %s",
                          GET_NAME(ch), (hood == ch) ? "" : GET_NAME(hood),
                          o_obj->value[3], o_obj->value[2], o_obj->value[1],
                          o_obj->value[0], s_obj->action_description);
                  }
                  else
                  {
                    if (CAN_WEAR(o_obj, ITEM_WEAR_IOUN) ||
                        IS_ARTIFACT(o_obj))
                    {
                      logit(LOG_CORPSE, "%s%s: %s [%d] (ARTIFACT) from %s",
                            GET_NAME(ch),
                            (hood == ch) ? "" : GET_NAME(hood), o_obj->name,
                            obj_index[o_obj->R_num].virtual_number,
                            s_obj->action_description);
                      //if((GET_LEVEL(ch) > 44) && (s_obj->value[2] > 44)) {
                      if ((s_obj->value[5] != 0) &&
                          ((RACE_GOOD(ch) && (s_obj->value[5] != 1)) ||
                           (RACE_EVIL(ch) && (s_obj->value[5] != 2)) ||
                           (RACE_PUNDEAD(ch) && (s_obj->value[5] != 3))))
                      {
                        int vnum = GET_OBJ_VNUM(o_obj);
                        int owner_pid = -1;
                        int timer = time(NULL);
                        sql_update_bind_data(vnum, &owner_pid, &timer);
                        feed_artifact(ch, o_obj, (int) get_property("artifact.feeding.initial.secs", 360000 ), FALSE );
                      }
                    }
                    else
                    { // No
                      logit(LOG_CORPSE, "%s%s: %s [%d] from %s",
                            GET_NAME(ch),
                            (hood == ch) ? "" : GET_NAME(hood), o_obj->name,
                            obj_index[o_obj->R_num].virtual_number,
                            s_obj->action_description);
                    }
                  }
                  if (!IS_TRUSTED(ch))
                  {
                    CharWait(ch, PULSE_VIOLENCE);
                  }
                }
                get(ch, o_obj, s_obj, TRUE);

                /*
                   if (corpse_flag == 1 && s_obj->value[4] == 1) {
                   strcpy(Gbuf4, s_obj->action_description);
                   *Gbuf4 = tolower(*Gbuf4);
                   owner = get_char(Gbuf4);
                   if (owner) {
                   if (!is_linked_to(ch, owner, LNK_CONSENT) &&
                   (owner != ch)) {
                   if (CHAR_IN_TOWN(ch)) {
                   if (GET_CRIME_T(CHAR_IN_TOWN(ch),CRIME_CORPSE_LOOT)) {
                   if (GET_ITEM_TYPE(o_obj) != ITEM_MONEY) {
                   o_obj->justice_status = J_OBJ_CORPSE;
                   o_obj->justice_name = str_dup(s_obj->action_description);
                   }
                   looting = TRUE;
                   }
                   }
                   }
                   } else {
                   if (CHAR_IN_TOWN(ch)) {
                   if (GET_CRIME_T(CHAR_IN_TOWN(ch),CRIME_CORPSE_LOOT)) {
                   if (GET_ITEM_TYPE(o_obj) != ITEM_MONEY) {
                   o_obj->justice_status = J_OBJ_CORPSE;
                   o_obj->justice_name = str_dup(s_obj->action_description);
                   }
                   looting = TRUE;
                   }
                   }
                   }
                   }

                 */

                found = TRUE;
                if (GET_ITEM_TYPE(s_obj) == ITEM_QUIVER)
                  if (s_obj->value[3] > 0)
                    s_obj->value[3]--;
              }
              else
              {
                sprintf(Gbuf3, "%s isn't takable.\r\n",
                        o_obj->short_description);
                send_to_char(Gbuf3, ch);
                fail = TRUE;
              }
            }
            else
            {
              sprintf(Gbuf3, "%s is too heavy.\r\n",
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
          sprintf(Gbuf3, "%s does not contain the %s.\r\n",
                  s_obj->short_description, arg1);
          send_to_char(Gbuf3, ch);
          fail = TRUE;
        }
      }
      else
      {
        sprintf(Gbuf3, "%s isn't a container.\r\n",
                s_obj->short_description);
        send_to_char(Gbuf3, ch);
        fail = TRUE;
      }
    }
    else
    {
      sprintf(Gbuf3, "You do not see or have the %s.\r\n", arg2);
      send_to_char(Gbuf3, ch);
      fail = TRUE;
    }
  }

  writeCharacter(ch, 1, ch->in_room);
  char_light(ch);
  room_light(ch->in_room, REAL);

  if(looting)
  {
    /* thieves may get away with it */
    CharWait(ch, 10);
    if (GET_CLASS(ch, CLASS_ROGUE))
      if (GET_CHAR_SKILL(ch, SKILL_STEAL) > number(1, 101))
        return;
    justice_witness(ch, NULL, CRIME_CORPSE_LOOT);
  }

}

void do_junk(P_char ch, char *argument, int cmd)
{
  P_obj    tmp_object, next_obj;
  P_char   t_ch;
  bool     test = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];

  argument = one_argument(argument, Gbuf1);

  if(!IS_TRUSTED(ch) ||
    !(ch))
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
      sprintf(Gbuf3,
              "WARNING: junk permanently destroys the specified object(s).\r\n"
              "Please confirm that you wish to junk %s (Yes/No) [No]:\r\n",
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

        if (!IS_SET(tmp_object->extra_flags, ITEM_NODROP) || IS_TRUSTED(ch))
        {
          if (!IS_SET(tmp_object->extra_flags, ITEM_TRANSIENT))
          {
            if (CAN_SEE_OBJ(ch, tmp_object))
            {
              sprintf(Gbuf3, "You junk a %s.\r\n",
                      FirstWord(tmp_object->name));
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
            sprintf(Gbuf3, "The %s dissolves with a blinding light.\r\n",
                    FirstWord(tmp_object->name));
            for (t_ch = world[ch->in_room].people;
                 t_ch; t_ch = t_ch->next_in_room)
            {
              if (CAN_SEE_OBJ(t_ch, tmp_object))
                send_to_char(Gbuf3, t_ch);
            }
            extract_obj(tmp_object, TRUE);
            tmp_object = NULL;
            test = TRUE;
            continue;
          }
          act("$n junks $p.", 1, ch, tmp_object, 0, TO_ROOM);
          obj_from_char(tmp_object, TRUE);
          extract_obj(tmp_object, TRUE);
          tmp_object = NULL;
          GET_SILVER(ch) += 1;
          test = TRUE;
        }
        else
        {
          if (CAN_SEE_OBJ(ch, tmp_object))
          {
            sprintf(Gbuf3, "You can't junk the %s, it must be CURSED!\r\n",
                    FirstWord(tmp_object->name));
            send_to_char(Gbuf3, ch);
            test = TRUE;
          }
        }
      }                         /*
                                 * (!str_cmp(Gbuf1, "all"))
                                 */
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
          if (!IS_SET(tmp_object->extra_flags, ITEM_TRANSIENT))
          {
            sprintf(Gbuf3, "You junk a %s.\r\n", FirstWord(tmp_object->name));
            send_to_char(Gbuf3, ch);
            act("$n junks $p.", 1, ch, tmp_object, 0, TO_ROOM);
            extract_obj(tmp_object, TRUE);
            tmp_object = NULL;
            GET_SILVER(ch) += 1;
          }
          else
          {
            sprintf(Gbuf3, "The %s dissolves with a blinding light.\r\n",
                    FirstWord(tmp_object->name));
            for (t_ch = world[ch->in_room].people;
                 t_ch; t_ch = t_ch->next_in_room)
            {
              if (CAN_SEE_OBJ(t_ch, tmp_object))
                send_to_char(Gbuf3, t_ch);
            }
            extract_obj(tmp_object, TRUE);
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

    sprintf(Gbuf3,
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
        obj_from_char(tmp_object, TRUE);
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

      }
  }

  if (total)
  {
    if (total == 1)
    {
      sprintf(Gbuf1, "You drop one %s.", name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
      sprintf(Gbuf1, "$n drops one %s.", name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
    }
    else if (total < 6)
    {
      sprintf(Gbuf1, "You drop %d %s(s).", total, name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
      sprintf(Gbuf1, "$n drops some %s(s).", name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      sprintf(Gbuf1, "You drop %d %s(s).", total, name);
      act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
      sprintf(Gbuf1, "$n drops a bunch of %s(s).", name);
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

  if (IS_ANIMAL(ch))
    return;

  argument = one_argument(argument, Gbuf1);

  if (is_number(Gbuf1))
  {
    if (strlen(Gbuf1) > 7)
    {
      send_to_char("Number field too big.\r\n", ch);
      return;
    }
    amount = atoi(Gbuf1);
    argument = one_argument(argument, Gbuf1);

    ctype = coin_type(Gbuf1);

    if ((ctype == -1) || (amount <= 0))
    {
      send_to_char
        ("Eh? Bugged call to do_drop(), you will be terminated.\r\n", ch);
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
        send_to_char("You do not have that many silver coins!\r\n", ch);
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
        send_to_char("You do not have that many &+Wplatinum&N coins!\r\n",
                     ch);
        return;
      }
      break;
    }

    if (cmd == 1 && IS_PC(ch))
      send_to_char
        ("Oops, trying to juggle too many loose coins, you drop a few.\r\n",
         ch);
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
      act("$n drops some silver coins.", FALSE, ch, 0, 0, TO_ROOM);
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
            sprintf(Gbuf3, "You drop %s.\r\n", tmp_object->short_description);
            send_to_char(Gbuf3, ch);
          }
          else
          {
            send_to_char("You drop something.\r\n", ch);
          }
          act("$n drops $p.", 1, ch, tmp_object, 0, TO_ROOM);
          obj_from_char(tmp_object, TRUE);
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
          obj_to_room(tmp_object, ch->in_room);

          if (IS_PC(ch))
            writeCharacter(ch, 1, ch->in_room);

          test = TRUE;
        }
        else
        {
          if (CAN_SEE_OBJ(ch, tmp_object))
          {
            sprintf(Gbuf3, "You can't drop %s, it must be CURSED!\r\n",
                    tmp_object->short_description);
            send_to_char(Gbuf3, ch);
            test = TRUE;
          }
        }
        /*
         * update player corpse file  (if needed)
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
          sprintf(Gbuf3, "You drop %s.\r\n", tmp_object->short_description);
          send_to_char(Gbuf3, ch);
          act("$n drops $p.", 0, ch, tmp_object, 0, TO_ROOM);
          obj_from_char(tmp_object, TRUE);
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
  int      bits, amount, ctype, count = 0;
  int      plat = 0, gold = 0, silv = 0, copp = 0;
  int      p, g, s, c, type = 0;
  char     buf[MAX_STRING_LENGTH];
  char     obj_name[MAX_STRING_LENGTH];
  char     cont_name[MAX_STRING_LENGTH];
  bool     attempted = FALSE;

  if (IS_ANIMAL(ch))
    return;

  argument = one_argument(argument, obj_name);

  if (!*obj_name)
  {
    send_to_char("Put what in what?\r\n", ch);
    return;
  }

  if (is_number(obj_name))
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
      sprintf(buf, "You do not have that many %s coins!\r\n",
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
  else if (!str_cmp(obj_name, "all"))
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
  else if (sscanf(obj_name, "all.%s", buf) != 0)
  {
    strcpy(obj_name, buf);
    type = PUT_ALLDOT;
  }
  else
    type = PUT_ITEM;

  argument = one_argument(argument, cont_name);

  if (!*cont_name)
  {
    sprintf(buf, "Put %s in what?\r\n", obj_name);
    send_to_char(buf, ch);
    return;
  }

  bits = generic_find(cont_name, FIND_OBJ_INV | FIND_OBJ_ROOM,
                      ch, &t_ch, &s_obj);
  if (!s_obj)
  {
    send_to_char("Into what?\r\n", ch);
    return;
  }

  if (type == PUT_COINS && (attempted = plat + gold + silv + copp > 0))
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
    if (count = put(ch, o_obj, s_obj, TRUE))
    {
      sprintf(buf,
              "You put %d &+Wplatinum&n, %d &+Ygold&n, %d silver, and %d &+ycopper&n coins into $P.",
              plat, gold, silv, copp);
      act(buf, TRUE, ch, 0, s_obj, TO_CHAR);
      ch->points.cash[3] -= plat;
      ch->points.cash[2] -= gold;
      ch->points.cash[1] -= silv;
      ch->points.cash[0] -= copp;
      if (tmp_obj)
      {
        GET_CARRYING_W(ch) -= GET_OBJ_WEIGHT(s_obj);
        extract_obj(tmp_obj, TRUE);
        GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(s_obj);
      }
    }
    else
    {
      extract_obj(o_obj, TRUE);
    }
  }

  if (type == PUT_ITEM)
  {
    attempted = generic_find(obj_name, FIND_OBJ_INV, ch, &t_ch, &o_obj);
    if (attempted)
      count = put(ch, o_obj, s_obj, TRUE);
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
      attempted = TRUE;
      if (put(ch, o_obj, s_obj, FALSE))
        count++;
    }
  }
  if (!attempted)
  {
    if (type != PUT_ITEM)
    {
      send_to_char("You don't have anything to put in it.\r\n", ch);
    }
    else
    {
      sprintf(buf, "You don't have the %s.\r\n", obj_name);
      send_to_char(buf, ch);
    }
  }
  else if (count)
  {
    if (type == PUT_ALL)
    {
      sprintf(buf, "You put %d items into $p.", count);
      act(buf, FALSE, ch, s_obj, 0, TO_CHAR);
      if (count < 6)
        act("$n puts some stuff into $p.", TRUE, ch, s_obj, 0, TO_ROOM);
      else
        act("$n puts a bunch of stuff into $p.", TRUE, ch, s_obj, 0, TO_ROOM);
    }
    else if (type == PUT_ALLDOT)
    {
      sprintf(buf, "You put %d %s(s) into $p.", count, obj_name);
      act(buf, FALSE, ch, s_obj, 0, TO_CHAR);
      if (count < 6)
        sprintf(buf, "$n puts some %s(s) into $p.", obj_name);
      else
        sprintf(buf, "$n puts a bunch of %s(s) into $p.", obj_name);
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
            obj_from_char(o_obj, TRUE);
            if (OBJ_CARRIED(s_obj))
              GET_CARRYING_W(ch) -= GET_OBJ_WEIGHT(s_obj);
            obj_to_obj(o_obj, s_obj);
            if (OBJ_CARRIED(s_obj))
              GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(s_obj);
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
              obj_from_char(o_obj, TRUE);
              if (OBJ_CARRIED(s_obj))
                GET_CARRYING_W(ch) -= GET_OBJ_WEIGHT(s_obj);
              obj_to_obj(o_obj, s_obj);
              if (OBJ_CARRIED(s_obj))
                GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(s_obj);
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
        sprintf(Gbuf3, "The %s is not a container.\r\n",
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
  P_char   vict;
  P_obj    obj;
  int      quest = 0;

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
      if (amount <= 0)
        wizlog(57, "&-L&+R%s just tried to give %d %s in room %d!",
               GET_NAME(ch), amount,
               (ctype == 3) ? "plat" : (ctype ==
                                        2) ? "gold" : "coper or silver",
               world[ch->in_room].number);
      return;
    }
    if ((ch->points.cash[ctype] < amount) &&
        (IS_NPC(ch) || (GET_LEVEL(ch) < MAXLVL)))
    {
      sprintf(Gbuf1, "You do not have that many %s coins!\r\n",
              coin_names[ctype]);
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

    if (IS_NPC(vict) && mob_index[real_mobile0(GET_RNUM(vict))].qst_func)
      quest = 1;


    if (racewar(ch, vict) && !IS_TRUSTED(ch) && !IS_TRUSTED(vict))
    {
      send_to_char("Hey now, why would you want to do that?\r\n", ch);
      return;
    }

    if ((IS_NPC(vict) && (GET_RNUM(vict) == real_mobile(250))) ||
        IS_AFFECTED(vict, AFF_WRAITHFORM))
    {
      send_to_char("They couldn't carry that if they tried.\r\n", ch);
      return;
    }

    send_to_char("Ok.\r\n", ch);

    if (((CAN_CARRY_COINS(vict) < amount) || (amount > 1000)) &&
        !is_linked_to(ch, vict, LNK_CONSENT) && (!IS_TRUSTED(vict)) &&
        (cmd != -4) && !IS_TRUSTED(ch))
    {
      act("$E must consent to you before you can overload $M.", 0, ch, 0,
          vict, TO_CHAR);
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
    sprintf(Gbuf1, "%s gives you %d %s coins.\r\n",
            PERS(ch, vict, FALSE), amount, coin_names[ctype]);
    send_to_char(Gbuf1, vict);
    sprintf(Gbuf1, "$n gives some %s to $N", coin_names[ctype]);
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
  if (((((GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict)) > CAN_CARRY_W(vict)) ||
        (GET_OBJ_WEIGHT(obj) > 25)) && !is_linked_to(ch, vict, LNK_CONSENT))
      && (cmd != -4) && (!IS_TRUSTED(ch)))
  {
    act("$E must consent to you before you can overload $M.", 0, ch, 0, vict,
        TO_CHAR);
    return;
  }
  if (IS_ARTIFACT(obj) && racewar(ch, vict) && !IS_TRUSTED(ch) && !IS_TRUSTED(vict))
  {
    send_to_char("That would just be unethical now wouldn't it?\r\n", ch);
    return;
  }
  obj_from_char(obj, TRUE);
  obj_to_char(obj, vict);
  act("$n gives $p to $N.", 1, ch, obj, vict, TO_NOTVICT);
  act("$n gives you $p.", 0, ch, obj, vict, TO_VICT);
  send_to_char("Ok.\r\n", ch);
  if (IS_TRUSTED(ch))
  {
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
    obj_from_char(obj, TRUE);
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

  sprintf(new_name, "%s %s", drinks[type], obj->name);

  if ((obj->str_mask & STRUNG_KEYS) && obj->name)
    str_free(obj->name);

  obj->str_mask |= STRUNG_KEYS;
  obj->name = new_name;
}

/*
 ** Improved version of "drink".  Empty transient object should now
 ** vanish.
 ** You can drink from fountain and other such places....
 */

void do_drink(P_char ch, char *argument, int cmd)
{
  P_obj    temp;
  int      amount, healamt;
  char     Gbuf4[MAX_STRING_LENGTH];
  int      own_object;          /*
                                 * Boolean flag used to determine
                                 *
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
      sprintf(Gbuf4, "$n drinks %s from $p.", drinks[temp->value[2]]);
      act(Gbuf4, TRUE, ch, temp, 0, TO_ROOM);
      sprintf(Gbuf4, "You drink the %s from $p.", drinks[temp->value[2]]);
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
        act("You do not feel thirsty.", FALSE, ch, 0, 0, TO_CHAR);

      if (GET_COND(ch, FULL) > 20)
        act("You are full.", FALSE, ch, 0, 0, TO_CHAR);

      if (temp->value[2] == LIQ_HOLYWATER)
      {
        if (IS_GOOD(ch))
        {
          healamt = MIN(GET_MAX_HIT(ch) - GET_HIT(ch), dice(3, 3));

          send_to_char("You feel touched by a higher power!\r\n", ch);
          GET_HIT(ch) += healamt;
          healCondition(ch, healamt);
        }
        else if (IS_EVIL(ch))
        {
          send_to_char("You are blasted by a higher power!\r\n", ch);
          GET_HIT(ch) = MAX(0, GET_HIT(ch) - dice(3, 3));
        }
      }
      else if (temp->value[2] == LIQ_UNHOLYWAT)
      {
        if (IS_EVIL(ch))
        {
          healamt = MIN(GET_MAX_HIT(ch) - GET_HIT(ch), dice(3, 3));

          send_to_char("You feel touched by a higher power!\r\n", ch);
          GET_HIT(ch) += healamt;
          healCondition(ch, healamt);
        }
        else if (IS_GOOD(ch))
        {
          send_to_char("You are blasted by a higher power!\r\n", ch);
          GET_HIT(ch) = MAX(0, GET_HIT(ch) - dice(3, 3));
        }
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

      if (temp->value[1] <= 0 &&
          IS_SET(temp->extra_flags, ITEM_TRANSIENT) && own_object)
      {

        act("The empty $q vanishes into thin air.\r\n", TRUE, ch, temp, 0,
            TO_CHAR);
        obj_from_char(temp, TRUE);
        extract_obj(temp, TRUE);
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
  char     Gbuf4[MAX_STRING_LENGTH];

  one_argument(argument, Gbuf4);

  if (GET_RACE(ch) == RACE_ILLITHID && GET_LEVEL(ch) < AVATAR)
  {
    send_to_char
      ("Ugh. Even if you had the means to eat, the thought revolts you.\r\n",
       ch);
    return;
  }
  if (!(temp = get_obj_in_list_vis(ch, Gbuf4, ch->carrying)))
  {
    act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
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
  if (affected_by_spell(ch, TAG_EATING) && !IS_TRUSTED(ch))
  {
    act("You feel sated already.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if(temp->value[1] < 0 || (temp->timer[0] && (time(NULL) - temp->timer[0] > 1 * 60 * 10)) )
  {
    act("That stinks, find some fresh food instead.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  act("$n eats $p.", TRUE, ch, temp, 0, TO_ROOM);
  act("You eat the $q.", FALSE, ch, temp, 0, TO_CHAR);

  if (temp->type == ITEM_FOOD)
  {
    /* New code to grant reg from food */
    struct affected_type af;
    if (!affected_by_spell(ch, TAG_EATING)) 
    {
      bzero(&af, sizeof(af));
      af.type = TAG_EATING;
      af.flags = AFFTYPE_NOSHOW;
      af.duration = 1 + (1 * temp->value[0]);

      int hit_reg;
      int mov_reg;
      if (temp->value[3] > 0) // TODO: apply poison
      {
          hit_reg = -temp->value[3];
          mov_reg = 0;
      }
      else
      {
          hit_reg = 1;
          if (temp->value[1] != 0)
            hit_reg = temp->value[1];
          mov_reg = hit_reg;
          if (temp->value[2] != 0)
            mov_reg = temp->value[2];
      }

      af.location = APPLY_HIT_REG;
      af.modifier = 15 * hit_reg;
      affect_to_char(ch, &af);
      
      af.location = APPLY_MOVE_REG;
      af.modifier = mov_reg;
      affect_to_char(ch, &af);

      af.location = APPLY_STR;
      af.modifier = temp->value[4];
      affect_to_char(ch, &af);
      af.location = APPLY_CON;
      af.modifier = temp->value[4];
      affect_to_char(ch, &af);

      af.location = APPLY_AGI;
      af.modifier = temp->value[5];
      affect_to_char(ch, &af);
      af.location = APPLY_DEX;
      af.modifier = temp->value[5];
      affect_to_char(ch, &af);

      af.location = APPLY_INT;
      af.modifier = temp->value[6];
      affect_to_char(ch, &af);
      af.location = APPLY_WIS;
      af.modifier = temp->value[6];
      affect_to_char(ch, &af);

      af.location = APPLY_DAMROLL;
      af.modifier = temp->value[7];
      affect_to_char(ch, &af);
      af.location = APPLY_HITROLL;
      af.modifier = temp->value[7];
      affect_to_char(ch, &af);
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
  extract_obj(temp, !IS_TRUSTED(ch));
  /*
   * added by DTS 5/18/95 to solve light bug
   */
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
  else if (to_char = get_char_vis(ch, Gbuf2)) {
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
  sprintf(Gbuf4, "You pour the %s into the %s.",
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
  sprintf(Gbuf4, "You fill the %s with the %s.", Gbuf1,
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
    send_to_char("Mobs don't need to sip liquids!\n", ch);
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
  sprintf(Gbuf4, "It tastes like %s.\r\n", drinks[temp->value[2]]);
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
    send_to_char
      ("Ugh. Even if you had the means to eat, the thought revolts you.\r\n",
       ch);
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
  if (!(temp->type == ITEM_FOOD))
  {
    act("It tastes inedible, aren't you glad it wasn't coated with poison?",
        FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  act("$n tastes the $q.", TRUE, ch, temp, 0, TO_ROOM);
  act("You taste the $q.", FALSE, ch, temp, 0, TO_CHAR);

  if (temp->value[3] > 0)
  {
    act("Oops, it did not taste good at all!", FALSE, ch, 0, 0, TO_CHAR);
    poison_lifeleak(10, ch, 0, 0, ch, 0);
  }
  return;
}

/*
 * functions related to wear
 */

void perform_wear(P_char ch, P_obj obj_object, int keyword)
{
  struct affected_type af;

  switch (keyword)
  {
  case 0:
    act("$n lights $p and holds it.", FALSE, ch, obj_object, 0, TO_ROOM);
    break;
  case 1:
    act("$n slips $s finger into $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 2:
    act("$n places $p around $s neck.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 3:
    act("$n shrugs into $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 4:
    act("$n dons $p on $s head.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 5:
    act("$n slides $s legs into $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 6:
    act("$n places $p on $s feet.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 7:
    act("$n wears $p on $s hands.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 8:
    act("$n covers $s arms with $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 9:
    act("$n wears $p about $s body.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 10:
    act("$n clasps $p about $s waist.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 11:
    act("$n places $p around $s wrist.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 12:
    act("$n wields $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 13:
    if ((GET_ITEM_TYPE(obj_object) == ITEM_LIGHT) && obj_object->value[2])
    {
      act("$n lights $p and holds it.", TRUE, ch, obj_object, 0, TO_ROOM);
      if (obj_object->value[2] > 0)
        CharWait(ch, 2);
    }
    else
      act("$n grabs $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 14:
    act("$n straps $p to $s arm.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 15:
    act("$n wears $p over $s eyes.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 16:
    act("$n covers $s face with $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 17:
    act("$n wears $p in $s ear.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 18:
    act("$n straps $p to $s back.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 19:
    act("$n dons $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 20:
    act("$n straps $p to $s back.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 21:
    act("$n attaches $p to $s belt.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 22:
    act("$n wears $p about $s horse body.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 23:
    act("$n wears $p on $s tail.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 24:
    act("$n wears $p on $s nose.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 25:
    act("$n wears $p on $s horns.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  case 26:
    act("$n throws $p in the air and it begins circling $s head.", TRUE, ch,
        obj_object, 0, TO_ROOM);
    break;
  case 27:
    act("$n shrugs into $p.", TRUE, ch, obj_object, 0, TO_ROOM);
    break;
  }
  if (IS_SET(obj_object->extra_flags, ITEM_LIT))
  {
    act("&+WIt glows brightly.&n", TRUE, ch, 0, 0, TO_ROOM);
    act("&+WIt glows brightly.&n", TRUE, ch, 0, 0, TO_CHAR);
  }

  if (IS_SET(obj_object->bitvector, AFF_INVISIBLE) &&
      !affected_by_spell(ch, SKILL_PERMINVIS))
  {
    bzero(&af, sizeof(af));
    af.type = SKILL_PERMINVIS;
    af.duration = -1;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char(ch, &af);
    if (!IS_AFFECTED(ch, AFF_INVISIBLE) &&
        !IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
    {
      act("$n slowly fades out of existence.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You vanish.\r\n", ch);
    }
  }

/*
  if (IS_SET(obj_object->bitvector, AFF_INVISIBLE) && !IS_AFFECTED(ch, AFF_INVISIBLE)
      && !IS_AFFECTED2(ch, AFF2_MINOR_INVIS))
    if ((keyword != 13) || (obj_object->wear_flags == (ITEM_TAKE + ITEM_HOLD))) {
      act("$n slowly fades out of existence.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You vanish.\r\n", ch);
    }
*/
}

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

int get_numb_free_hands(P_char ch)
{
  int      free_hands = 2;

  if (HAS_FOUR_HANDS(ch))
    free_hands += 2;

  if (ch->equipment[HOLD]) {
    free_hands -= wield_item_size(ch, ch->equipment[HOLD]);
  }

  if (ch->equipment[WEAR_SHIELD]) {
    free_hands--;
  }

  if (ch->equipment[WIELD]) {
    free_hands -= wield_item_size(ch, ch->equipment[WIELD]);
  }

  if (ch->equipment[WIELD2]) {
    free_hands -= wield_item_size(ch, ch->equipment[WIELD2]);
  }

  if (ch->equipment[WIELD3]) {
    free_hands -= wield_item_size(ch, ch->equipment[WIELD3]);
  }

  if (ch->equipment[WIELD4]) {
    free_hands -= wield_item_size(ch, ch->equipment[WIELD4]);
  }

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
 ** Improvements:
 **
 * 1) Let player to wield 2 weapons (dual wield)
 * 2) Returns int now because else it would piss off mobs.
 *    (1=successful, 0=failure)
 */

int wear(P_char ch, P_obj obj_object, int keyword, int showit)
{
  char     Gbuf3[MAX_STRING_LENGTH];
  int      free_hands, wield_to_where, o_size, hands_needed;

  if(!obj_object)
  {
    return FALSE;
  }
  
  if(!ch)
  {
    return false;
  }

  if(obj_object->condition <= 0) // Scrap it. Might cause crash. Dec08 -Lucrot
  {
    wizlog(56, "%s wore %s that's condition 0 or less : attempting to scrap.",
          GET_NAME(ch),
          obj_object->short_description);
    MakeScrap(ch, obj_object);
    return false;
  }

// Quick and dirty periodic check for buggy items. Dec08 -Lucrot
// Can write to player log if these checks are sufficient.

  for (int i = 0; i < 3; i++ )
  {  // Hunting a bad apply ...
    if(obj_object->affected[i].location > APPLY_LAST &&
       obj_object->affected[i].modifier == 2)
    {
      wizlog(56, "%s has buggy item with a bad apply : %s",
            GET_NAME(ch),
            obj_object->short_description);
    }
     // Hunting max race eq ...
    if(obj_object->affected[i].location >= 41 &&
      obj_object->affected[i].location <= 50)
    {
      wizlog(56, "%s has MAX_RACE item : %s",
            GET_NAME(ch),
            obj_object->short_description);
    }
    
    if(obj_object->affected[i].location == APPLY_DAMROLL &&
       obj_object->affected[i].modifier >= 20)
    {
      wizlog(56, "%s has item with >= 20 damroll : %s",
             GET_NAME(ch),
             obj_object->short_description);
    }
  
    if(obj_object->affected[i].location == APPLY_HITROLL &&
       obj_object->affected[i].modifier >= 20)
    {
      wizlog(56, "%s has item with >= 20 hitroll : %s",
             GET_NAME(ch),
             obj_object->short_description);
    }

    if(is_stat_max(obj_object->affected[i].location) && !IS_ARTIFACT(obj_object))
    {
      if(obj_object->affected[i].modifier > 10)
      {
        char buf[128];
        sprinttype(obj_object->affected[i].location, apply_types, buf);
        wizlog(56, "%s has %s with %d %s.",
              GET_NAME(ch),
              obj_object->short_description,
              obj_object->affected[i].modifier,
              buf);
      }
    }
  }

  if (!can_char_use_item(ch, obj_object))
  {
    if (showit)
      act("You can't use $p.", FALSE, ch, obj_object, 0, TO_CHAR);
    return FALSE;
  }
#if 0
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

  free_hands = get_numb_free_hands(ch);

  switch (keyword)
  {
  case 0:
    logit(LOG_OBJ,
          "wear(): object worn in invalid location (%s, %s)",
          GET_NAME(ch), obj_object->short_description);
    break;

  case 1:
    if (CAN_WEAR(obj_object, ITEM_WEAR_FINGER) && !IS_THRIKREEN(ch))
    {
      if ((ch->equipment[WEAR_FINGER_L]) && (ch->equipment[WEAR_FINGER_R]))
      {
        if (showit)
          send_to_char
            ("You are already wearing something on your fingers.\r\n", ch);
      }
      else
      {
        if (showit)
          perform_wear(ch, obj_object, keyword);
        if (ch->equipment[WEAR_FINGER_L])
        {
          if (showit)
            act("You place $p on your right ring finger.", 0, ch, obj_object,
                0, TO_CHAR);
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_FINGER_R, !showit);
          return TRUE;
        }
        else
        {
          if (showit)
            act("You place $p on your left ring finger.", 0, ch, obj_object,
                0, TO_CHAR);
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_FINGER_L, !showit);
          return TRUE;
        }
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your fingers.\r\n", ch);
    }
    break;
  case 2:
    if (CAN_WEAR(obj_object, ITEM_WEAR_NECK))
    {
      if ((ch->equipment[WEAR_NECK_1]) && (ch->equipment[WEAR_NECK_2]))
      {
        if (showit)
          send_to_char("You can't wear any more around your neck.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You duck your head and place $p around your neck.", 0, ch,
              obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        if (ch->equipment[WEAR_NECK_1])
        {
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_NECK_2, !showit);
          return TRUE;
        }
        else
        {
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_NECK_1, !showit);
          return TRUE;
        }
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that around your neck.\r\n", ch);
    }
    break;

  case 3:
    if (CAN_WEAR(obj_object, ITEM_WEAR_BODY) && !IS_THRIKREEN(ch))
    {
      if (IS_SET(obj_object->extra_flags, ITEM_WHOLE_BODY))
      {
        if (IS_CENTAUR(ch) || IS_MINOTAUR(ch) || IS_OGRE(ch) || IS_SGIANT(ch) ||
                   GET_RACE(ch) == RACE_WIGHT || GET_RACE(ch) == RACE_SNOW_OGRE)
        {
          if (showit)
            send_to_char("You can't wear full body armor.\r\n", ch);
          break;
        }
        else if ((ch->equipment[WEAR_ARMS]) && (ch->equipment[WEAR_LEGS]))
        {
          if (showit)
            send_to_char
              ("You can't wear something on your arms and legs and wear that.\r\n",
               ch);
          break;
        }
        else if (ch->equipment[WEAR_ARMS])
        {
          if (showit)
            send_to_char
              ("You can't wear something on your arms and wear that.\r\n",
               ch);
          break;
        }
        else if (ch->equipment[WEAR_LEGS])
        {
          if (showit)
            send_to_char
              ("You can't wear something on your legs and wear that.\r\n",
               ch);
          break;
        }
      }
      if (ch->equipment[WEAR_BODY])
      {
        if (showit)
          send_to_char("You already wear something on your body.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You shrug into $p.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_BODY, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your body.\r\n", ch);
    }
    break;

  case 4:
    if (CAN_WEAR(obj_object, ITEM_WEAR_HEAD)
        && !IS_MINOTAUR(ch) && !IS_ILLITHID(ch) && !IS_PILLITHID(ch))
    {
      if (IS_SET(obj_object->extra_flags, ITEM_WHOLE_HEAD))
      {
        if ((ch->equipment[WEAR_EYES]) && (ch->equipment[WEAR_FACE]))
        {
          if (showit)
            send_to_char
              ("You can't wear something on your eyes and face and wear that.\r\n",
               ch);
          break;
        }
        else if (ch->equipment[WEAR_EYES])
        {
          if (showit)
            send_to_char
              ("You can't wear something on your eyes and wear that.\r\n",
               ch);
          break;
        }
        else if (ch->equipment[WEAR_FACE])
        {
          if (showit)
            send_to_char
              ("You can't wear something on your face and wear that.\r\n",
               ch);
          break;
        }
      }
      if (ch->equipment[WEAR_HEAD])
      {
        if (showit)
          send_to_char("You already wear something on your head.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You don $p on your head.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_HEAD, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
      {
        if (IS_ILLITHID(ch) || IS_PILLITHID(ch))
          send_to_char("Sorry, you can't wear anything on your head.\r\n",
                       ch);
        else
          send_to_char("You can't wear that on your head.\r\n", ch);
      }
    }
    break;

  case 5:
    if (CAN_WEAR(obj_object, ITEM_WEAR_LEGS) && 
       !IS_DRIDER(ch) &&
       !IS_CENTAUR(ch) &&
       !IS_HARPY(ch)/* &&
       !IS_MINOTAUR(ch)*/)
    {
      if (ch->equipment[WEAR_BODY] &&
          IS_SET(ch->equipment[WEAR_BODY]->extra_flags, ITEM_WHOLE_BODY))
      {
        if (showit)
          send_to_char
            ("You can't wear something on your legs and wear that on your body.\r\n",
             ch);
        break;
      }
      if (ch->equipment[WEAR_LEGS])
      {
        if (showit)
          send_to_char("You already wear something on your legs.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You slide your legs into $p.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_LEGS, !showit);
        return TRUE;
      }
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
            send_to_char("You can't wear that on your legs.\r\n", ch);
      }
    }
    break;

  case 6:
    if (CAN_WEAR(obj_object, ITEM_WEAR_FEET) 
	&& !IS_DRIDER(ch) && !IS_THRIKREEN(ch)
	&& !IS_HARPY(ch) && !IS_MINOTAUR(ch)
	&& (!IS_CENTAUR(ch)
        || (IS_CENTAUR(ch) && !strcmp(obj_object->name, "horseshoe")))) 
    {
      if (ch->equipment[WEAR_FEET])
      {
        if (showit)
          send_to_char("You already wear something on your feet.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You place $p on your feet.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_FEET, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your feet.\r\n", ch);
    }
    break;

  case 7:
    if (CAN_WEAR(obj_object, ITEM_WEAR_HANDS))
    {
      if (HAS_FOUR_HANDS(ch))
      {
        if ((ch->equipment[WEAR_HANDS]) && (ch->equipment[WEAR_HANDS_2]))
        {
          if (showit)
            send_to_char("You can't wear any more on your hands.\r\n", ch);
        }
        else
        {
          if (showit)
          {
            act("You wear $p on your hands.", 0, ch, obj_object, 0, TO_CHAR);
            perform_wear(ch, obj_object, keyword);
          }
          if (ch->equipment[WEAR_HANDS])
          {
            obj_from_char(obj_object, TRUE);
            equip_char(ch, obj_object, WEAR_HANDS_2, !showit);
            return TRUE;
          }
          else
          {
            obj_from_char(obj_object, TRUE);
            equip_char(ch, obj_object, WEAR_HANDS, !showit);
            return TRUE;
          }
        }
      }
      else
      {
        if (ch->equipment[WEAR_HANDS])
        {
          if (showit)
            send_to_char("You already wear something on your hands.\r\n", ch);
        }
        else
        {
          if (showit)
          {
            act("You wear $p on your hands.", 0, ch, obj_object, 0, TO_CHAR);
            perform_wear(ch, obj_object, keyword);
          }
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_HANDS, !showit);
          return TRUE;
        }
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your hands.\r\n", ch);
    }
    break;

  case 8:
    if(CAN_WEAR(obj_object, ITEM_WEAR_ARMS)
      // &&
      // !IS_OGRE(ch) &&
      // !IS_SGIANT(ch) &&
      // !(GET_RACE(ch) == RACE_SNOW_OGRE)
      )
    {
      if (HAS_FOUR_HANDS(ch))
      {
        if ((ch->equipment[WEAR_ARMS]) && (ch->equipment[WEAR_ARMS_2]))
        {
          if (showit)
            send_to_char("You can't wear any more on your arms.\r\n", ch);
        }
        else
        {
          if (showit)
          {
            act("You cover your arms with $p.", 0, ch, obj_object, 0,
                TO_CHAR);
            perform_wear(ch, obj_object, keyword);
          }
          if (ch->equipment[WEAR_ARMS])
          {
            obj_from_char(obj_object, TRUE);
            equip_char(ch, obj_object, WEAR_ARMS_2, !showit);
            return TRUE;
          }
          else
          {
            obj_from_char(obj_object, TRUE);

            equip_char(ch, obj_object, WEAR_ARMS, !showit);
            return TRUE;
          }
        }
      }
      else
      {
        if (ch->equipment[WEAR_BODY] &&
            IS_SET(ch->equipment[WEAR_BODY]->extra_flags, ITEM_WHOLE_BODY))
        {
          if (showit)
            send_to_char
              ("You can't wear something on your arms and wear that on your body.\r\n",
               ch);
          break;
        }
        if (ch->equipment[WEAR_ARMS])
        {
          if (showit)
            send_to_char("You already wear something on your arms.\r\n", ch);
        }
        else
        {
          if (showit)
          {
            act("You cover your arms with $p.", 0, ch, obj_object, 0,
                TO_CHAR);
            perform_wear(ch, obj_object, keyword);
          }
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_ARMS, !showit);
          return TRUE;
        }
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your arms.\r\n", ch);
    }
    break;

  case 9:
    if (CAN_WEAR(obj_object, ITEM_WEAR_ABOUT))
    {
      if (ch->equipment[WEAR_ABOUT])
      {
        if (showit)
          send_to_char("You already wear something about your body.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You wear $p about your body.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_ABOUT, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that about your body.\r\n", ch);
    }
    break;

  case 10:
    if ((CAN_WEAR(obj_object, ITEM_WEAR_WAIST)))
    {
      if (ch->equipment[WEAR_WAIST])
      {
        if (showit)
          send_to_char("You already wear something about your waist.\r\n",
                       ch);
      }
      else
      {
        if (showit)
        {
          act("You clasp $p about your waist.", 0, ch, obj_object, 0,
              TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_WAIST, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that about your waist.\r\n", ch);
    }
    break;

  case 11:
    if (CAN_WEAR(obj_object, ITEM_WEAR_WRIST))
    {
      if (((ch->equipment[WEAR_WRIST_L]) && (ch->equipment[WEAR_WRIST_R]) &&
           (!HAS_FOUR_HANDS(ch))) || ((ch->equipment[WEAR_WRIST_L]) &&
                                      (ch->equipment[WEAR_WRIST_R]) &&
                                      (ch->equipment[WEAR_WRIST_LL]) &&
                                      (ch->equipment[WEAR_WRIST_LR]) &&
                                      (HAS_FOUR_HANDS(ch))))
      {
        if (showit)
          send_to_char
            ("You already wear something around all your wrists.\r\n", ch);
      }
      else
      {
        if (showit)
          perform_wear(ch, obj_object, keyword);
        obj_from_char(obj_object, TRUE);
        if (!ch->equipment[WEAR_WRIST_L])
        {
          if (showit)
            act("You place $p around your left wrist.", 0, ch, obj_object, 0,
                TO_CHAR);
          equip_char(ch, obj_object, WEAR_WRIST_L, !showit);
          return TRUE;
        }
        else if (!ch->equipment[WEAR_WRIST_R])
        {
          if (showit)
            act("You place $p around your right wrist.", 0, ch, obj_object, 0,
                TO_CHAR);
          equip_char(ch, obj_object, WEAR_WRIST_R, !showit);
          return TRUE;
        }
        else if (!ch->equipment[WEAR_WRIST_LL])
        {
          if (showit)
            act("You place $p around your left wrist.", 0, ch, obj_object, 0,
                TO_CHAR);
          equip_char(ch, obj_object, WEAR_WRIST_LL, !showit);
          return TRUE;
        }
        else
        {
          if (showit)
            act("You place $p around your right wrist.", 0, ch, obj_object, 0,
                TO_CHAR);
          equip_char(ch, obj_object, WEAR_WRIST_LR, !showit);
          return TRUE;
        }
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that around your wrist.\r\n", ch);
    }
    break;

  case 12:
    if (!CAN_WEAR(obj_object, ITEM_WIELD))
    {
      if (showit)
        send_to_char("You can't wield that.\r\n", ch);
      break;
    }
    if (!free_hands)
    {
      if (showit)
        send_to_char("You need at least one free hand to wield anything.\r\n",
                     ch);
      break;
    }

    hands_needed = IS_SET(obj_object->extra_flags, ITEM_TWOHANDS) ? 2 : 1;

    if (hands_needed == 2 && free_hands < 2)
    {
      if (showit)
        send_to_char("You need two free hands to wield that.\r\n", ch);
      break;
    }

    if (GET_OBJ_WEIGHT(obj_object) >
        (str_app[STAT_INDEX(GET_C_STR(ch))].wield_w))
    {
      if (showit)
        send_to_char("It is too heavy for you to use.\r\n", ch);
      break;
    }
    /*
     * Check wield to where .. if primary is occupied, wield to
     * secondary, and vice versa
     */

    // four-handed guys aren't quite so simple

    if (HAS_FOUR_HANDS(ch))
    {
      if (hands_needed == 1)
      {
        if (!ch->equipment[PRIMARY_WEAPON])
          wield_to_where = PRIMARY_WEAPON;
        else if (!ch->equipment[SECONDARY_WEAPON] &&
                 (!ch->equipment[PRIMARY_WEAPON] ||
                  !IS_SET(ch->equipment[PRIMARY_WEAPON]->extra_flags,
                          ITEM_TWOHANDS) || IS_TRUSTED(ch)))
          wield_to_where = SECONDARY_WEAPON;
        else if (!ch->equipment[THIRD_WEAPON])
          wield_to_where = THIRD_WEAPON;
        else
          if (!IS_SET(ch->equipment[THIRD_WEAPON]->extra_flags, ITEM_TWOHANDS)
              || IS_TRUSTED(ch))
          wield_to_where = FOURTH_WEAPON;
      }

      // let's assume everything takes one or two hands, so here we are doing
      // the case where the weapon takes two hands

      else
      {
        if (!ch->equipment[PRIMARY_WEAPON])
        {
          if (ch->equipment[SECONDARY_WEAPON])
          {
            if (showit)
              send_to_char
                ("You do not have enough hands free to use that.\r\n", ch);
            break;
          }

          wield_to_where = PRIMARY_WEAPON;
        }
        else if (!ch->equipment[THIRD_WEAPON])
        {
          if (ch->equipment[FOURTH_WEAPON])
          {
            if (showit)
              send_to_char
                ("You do not have enough hands free to use that.\r\n", ch);
            break;
          }

          wield_to_where = THIRD_WEAPON;
        }
        else
        {
          if (showit)
            send_to_char("You do not have enough hands free to use that.\r\n",
                         ch);
          break;
        }
      }
    }
    else
    {
      if (ch->equipment[PRIMARY_WEAPON])
        wield_to_where = SECONDARY_WEAPON;
      else
        wield_to_where = PRIMARY_WEAPON;

      if (wield_to_where == SECONDARY_WEAPON)
      {
        if ((IS_PC(ch) && !GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD)) ||
            (IS_NPC(ch) && (!IS_WARRIOR(ch) || (GET_LEVEL(ch) < 15)) &&
             (!IS_THIEF(ch) || (GET_LEVEL(ch) < 20))))
        {
          if (showit)
            send_to_char("You lack the training to use two weapons.\r\n", ch);
          break;
        }
      }

      if ((wield_to_where == SECONDARY_WEAPON) &&
          (IS_REACH_WEAPON(obj_object) ||
           (!GET_CLASS(ch, CLASS_RANGER) &&
            (GET_OBJ_WEIGHT(obj_object) * ((IS_OGRE(ch) || IS_SNOWOGRE(ch)) ? 2 : 3) >
             (str_app[STAT_INDEX(GET_C_STR(ch))].wield_w)))))
      {
        if (showit)
          send_to_char("It is too heavy to wield in your secondary hand.\r\n",
                       ch);
        break;
      }
    }

    /* Removed by Zod */
/*
    if (wield_item_size(ch, obj_object) <= free_hands) { 
*/
    if (showit)
    {
      act("You wield $p.", 0, ch, obj_object, 0, TO_CHAR);
      perform_wear(ch, obj_object, keyword);
    }
    obj_from_char(obj_object, TRUE);
    equip_char(ch, obj_object, wield_to_where, !showit);
    return TRUE;
/*
    } else {
      if (showit)
        send_to_char("Dont seem to be the right size.\r\n", ch);
    }
*/
    break;

  case 13:
    if (!CAN_WEAR(obj_object, ITEM_HOLD) &&
        (GET_ITEM_TYPE(obj_object) != ITEM_LIGHT))
    {
      if (showit)
        send_to_char("You can't hold this.\r\n", ch);
      break;
    }
    if (!free_hands)
    {
      if (showit)
        send_to_char("Your hands are full.\r\n", ch);
      break;
    }
    if (IS_SET(obj_object->extra_flags, ITEM_TWOHANDS) && (free_hands < 2))
    {
      if (showit)
        send_to_char("You need two free hands to hold that.\r\n", ch);
      break;
    }
    if (showit)
    {
      if ((GET_ITEM_TYPE(obj_object) == ITEM_LIGHT) && obj_object->value[2])
        act("You light $p and hold it.", 0, ch, obj_object, 0, TO_CHAR);
      else
        act("You hold $p.", 0, ch, obj_object, 0, TO_CHAR);
    }
    if (showit)
      perform_wear(ch, obj_object, keyword);
    obj_from_char(obj_object, TRUE);
    if (HAS_FOUR_HANDS(ch))
    {
      if (!ch->equipment[HOLD])
        equip_char(ch, obj_object, HOLD, !showit);
      else if (!ch->equipment[WIELD])
        equip_char(ch, obj_object, WIELD, !showit);
      else if (!ch->equipment[WIELD3])
        equip_char(ch, obj_object, WIELD3, !showit);
      else
        equip_char(ch, obj_object, WIELD4, !showit);
    }
    else
    {
      if (!ch->equipment[HOLD] || !ch->equipment[WIELD])
        equip_char(ch, obj_object, ch->equipment[HOLD] ? WIELD : HOLD,
                   !showit);
      else
      {
        obj_to_char(obj_object, ch);
        if (showit)
          send_to_char("weird bug in eq..\r\n", ch);

        return FALSE;
      }
    }
    return TRUE;

    break;

  case 14:
    if (!CAN_WEAR(obj_object, ITEM_WEAR_SHIELD))
    {
      if (showit)
        send_to_char("You can't use that as a shield.\r\n", ch);
      break;
    }
    if (!free_hands)
    {
      if (showit)
        send_to_char("Your hands are full.\r\n", ch);
      break;
    }
    if ((ch->equipment[WEAR_SHIELD]))
    {
      if (showit)
        send_to_char("You are already using a shield.\r\n", ch);
      break;
    }
    if (showit)
    {
      perform_wear(ch, obj_object, keyword);
      act("You strap $p to your arm.", 0, ch, obj_object, 0, TO_CHAR);
    }
    obj_from_char(obj_object, TRUE);
    equip_char(ch, obj_object, WEAR_SHIELD, !showit);
    return TRUE;
    break;

  case 15:
    if (CAN_WEAR(obj_object, ITEM_WEAR_EYES))
    {
      if (ch->equipment[WEAR_HEAD] &&
          IS_SET(ch->equipment[WEAR_HEAD]->extra_flags, ITEM_WHOLE_HEAD))
      {
        if (showit)
          send_to_char
            ("You can't wear something on your eyes and wear that on your head.\r\n",
             ch);
        break;
      }
      if (ch->equipment[WEAR_EYES])
      {
        if (showit)
          send_to_char("You already wear something on your eyes.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You wear $p over your eyes.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_EYES, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your eyes.\r\n", ch);
    }
    break;
  case 16:
    if (CAN_WEAR(obj_object, ITEM_WEAR_FACE))
    {
      if (ch->equipment[WEAR_HEAD] &&
          IS_SET(ch->equipment[WEAR_HEAD]->extra_flags, ITEM_WHOLE_HEAD))
      {
        if (showit)
          send_to_char
            ("You can't wear something on your face and wear that on your head.\r\n",
             ch);
        break;
      }
      if (ch->equipment[WEAR_FACE])
      {
        if (showit)
          send_to_char("You already wear something on your face.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You cover your face with $p.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_FACE, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your face.\r\n", ch);
    }
    break;

  case 17:
    if (CAN_WEAR(obj_object, ITEM_WEAR_EARRING) && !IS_THRIKREEN(ch))
    {
      if ((ch->equipment[WEAR_EARRING_L]) && (ch->equipment[WEAR_EARRING_R]))
      {
        if (showit)
          send_to_char("You already wear an earring in each ear.\r\n", ch);
      }
      else
      {
        if (showit)
          perform_wear(ch, obj_object, keyword);
        obj_from_char(obj_object, TRUE);
        if (ch->equipment[WEAR_EARRING_L])
        {
          if (showit)
            act("You wear $p on your right ear.", 0, ch, obj_object, 0,
                TO_CHAR);
          equip_char(ch, obj_object, WEAR_EARRING_R, !showit);
          return TRUE;
        }
        else
        {
          if (showit)
            act("You wear $p on your left ear.", 0, ch, obj_object, 0,
                TO_CHAR);
          equip_char(ch, obj_object, WEAR_EARRING_L, !showit);
          return TRUE;
        }
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that in your ear.\r\n", ch);
    }
    break;

  case 18:
    if (CAN_WEAR(obj_object, ITEM_WEAR_QUIVER))
    {
      if (ch->equipment[WEAR_QUIVER])
      {
        if (showit)
          send_to_char("You already have a quiver strapped to your back.\r\n",
                       ch);
      }
      else
      {
        if (showit)
        {
          act("You strap $p onto your back.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_QUIVER, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't use that as your quiver.\r\n", ch);
    }
    break;

  case 19:
    if (CAN_WEAR(obj_object, ITEM_GUILD_INSIGNIA))
    {
      if (ch->equipment[GUILD_INSIGNIA])
      {
        if (showit)
          send_to_char("You are already wearing a guild insignia.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You don the guild insignia of $p.", 0, ch, obj_object, 0,
              TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, GUILD_INSIGNIA, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't use that as an insignia.\r\n", ch);
    }
    break;

  case 20:
    if (CAN_WEAR(obj_object, ITEM_WEAR_BACK))
    {
      if (ch->equipment[WEAR_BACK])
      {
        if (showit)
          send_to_char("You already have something on your back.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You strap $p on your back.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_BACK, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your back.\r\n", ch);
    }
    break;

  case 21:
    if (CAN_WEAR(obj_object, ITEM_ATTACH_BELT))
    {
      if (!ch->equipment[WEAR_WAIST])
      {
        if (showit)
          act("You need a belt to attach $p to it.", 0, ch, obj_object, 0,
              TO_CHAR);
      }
      else if ((ch->equipment[WEAR_ATTACH_BELT_1]) &&
               (ch->equipment[WEAR_ATTACH_BELT_2]) &&
               (ch->equipment[WEAR_ATTACH_BELT_3]))
      {
        if (showit)
          send_to_char("Your belt is full.\r\n", ch);
      }
      else
      {
        if (showit)
          perform_wear(ch, obj_object, keyword);
        if (!ch->equipment[WEAR_ATTACH_BELT_1])
        {
          if (showit)
            act("You attach $p to your belt.", 0, ch, obj_object, 0, TO_CHAR);
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_ATTACH_BELT_1, !showit);
          return TRUE;
        }
        else if (!ch->equipment[WEAR_ATTACH_BELT_2])
        {
          if (showit)
            act("You attach $p to your belt.", 0, ch, obj_object, 0, TO_CHAR);
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_ATTACH_BELT_2, !showit);
          return TRUE;
        }
        else
        {
          if (showit)
            act("You attach $p to your belt.", 0, ch, obj_object, 0, TO_CHAR);
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_ATTACH_BELT_3, !showit);
          return TRUE;
        }
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't attach that to your belt.\r\n", ch);
    }
    break;

  case 22:
    if (CAN_WEAR(obj_object, ITEM_HORSE_BODY) && (IS_CENTAUR(ch)))
    {
      if (ch->equipment[WEAR_HORSE_BODY])
      {
        if (showit)
          send_to_char
            ("You're already wearing something on your horse body.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You shrug into $p.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_HORSE_BODY, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that.\r\n", ch);
    }
    break;

  case 23:
    if (IS_CENTAUR(ch) || IS_MINOTAUR(ch) || IS_PSBEAST(ch) || IS_KOBOLD(ch))
    {
      if (CAN_WEAR(obj_object, ITEM_WEAR_TAIL))
      {
        if (ch->equipment[WEAR_TAIL])
        {
          if (showit)
            send_to_char("You're already wearing something on your tail.\r\n", ch);
        }
        else
        {
          if (showit)
          {
            act("You wear $p on your tail.", 0, ch, obj_object, 0, TO_CHAR);
            perform_wear(ch, obj_object, keyword);
          }
          obj_from_char(obj_object, TRUE);
          equip_char(ch, obj_object, WEAR_TAIL, !showit);
          return TRUE;
        }
      }
      else
      {
        if (showit)
          send_to_char("You can't wear that on your tail.\r\n", ch);
      }
    }
    else
    {
      send_to_char("You don't have a tail to wear that on.\r\n", ch);
    }
    break;
  case 24:                     /* nose */
    if (CAN_WEAR(obj_object, ITEM_WEAR_NOSE) && IS_MINOTAUR(ch))
    {
      if (ch->equipment[WEAR_NOSE])
      {
        send_to_char("You're already wearing something on your nose.\r\n",
                     ch);
      }
      else
      {
        if (showit)
        {
          act("You wear $p on your nose.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_NOSE, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your nose.\r\n", ch);
    }
    break;
  case 25:                     /* horns */
    if (CAN_WEAR(obj_object, ITEM_WEAR_HORN) &&
        (IS_MINOTAUR(ch) || IS_HARPY(ch) || IS_PSBEAST(ch)))
    {
      if (ch->equipment[WEAR_HORN])
      {
        send_to_char("You're already wearing something on your horns.\r\n",
                     ch);
      }
      else
      {
        if (showit)
        {
          act("You wear $p on your horns.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_HORN, !showit);
        return TRUE;
      }
    }
    else
    {
      if (showit)
        send_to_char("You can't wear that on your horns.\r\n", ch);
    }
    break;
  case 26:                     /* ioun stone */
    if (CAN_WEAR(obj_object, ITEM_WEAR_IOUN))
    {
      if (ch->equipment[WEAR_IOUN])
      {
        send_to_char
          ("You've already got an ioun stone floating about your head.\r\n",
           ch);
      }
      else
      {
        if (showit)
        {
          act("You throw $p in the air and it begins circling your head.", 0,
              ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_IOUN, !showit);
        return TRUE;
      }
    }
    break;
  case 27:
    if (CAN_WEAR(obj_object, ITEM_SPIDER_BODY))
    {
      if (GET_RACE(ch) != RACE_DRIDER)
      {
        if (showit)
          send_to_char("You can't wear spider body armor.\r\n", ch);
        break;
      }
      if (ch->equipment[WEAR_SPIDER_BODY])
      {
        send_to_char("You're already wearing something on your spider body.\r\n", ch);
      }
      else
      {
        if (showit)
        {
          act("You shrug into $p.", 0, ch, obj_object, 0, TO_CHAR);
          perform_wear(ch, obj_object, keyword);
        }
        obj_from_char(obj_object, TRUE);
        equip_char(ch, obj_object, WEAR_SPIDER_BODY, !showit);
        return TRUE;
      }
    }
    else
      if (showit)
        send_to_char("You can't wear that.\r\n", ch);
    break;

  case -1:
    if (showit)
    {
      sprintf(Gbuf3, "Wear %s where?\r\n", FirstWord(obj_object->name));
      send_to_char(Gbuf3, ch);
    }
    break;

  case -2:
    if (showit)
    {
      sprintf(Gbuf3, "You can't wear the %s.\r\n",
              FirstWord(obj_object->name));
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
 * [0] wearflag, [1] keyword[] number, [2] eq slot position
 */
/*
 * the order in which they appear below, is the order in which they will
 * attempt to be worn (wear all, especially)
 */

int      equipment_pos_table[CUR_MAX_WEAR][3] = {
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
  {ITEM_GUILD_INSIGNIA, 19, 24},
  {ITEM_WEAR_EYES, 15, 19},
  {ITEM_WEAR_ABOUT, 9, 12},
  {ITEM_WEAR_QUIVER, 18, 23},
  {ITEM_WEAR_IOUN, 26, 41},
  {ITEM_WIELD, 12, 16},         /*
                                 * primary weapon
                                 */
  {ITEM_WIELD, 12, 17},         /*
                                 * secondary weapons, same slot as HOLD
                                 */
  {ITEM_WIELD, 12, 25},
  {ITEM_WIELD, 12, 26},
  {ITEM_WEAR_BACK, 20, 27},
  {ITEM_ATTACH_BELT, 21, 30},
  {ITEM_ATTACH_BELT, 21, 29},
  {ITEM_ATTACH_BELT, 21, 28},
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
  {ITEM_HOLD, 13, 18}           /*
                                 * held gets checked last of all
                                 */
};



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
    "\n"
  };
  int      loop = 0;

  if (IS_ANIMAL(ch) || IS_DRAGON(ch))
  {
    send_to_char("DUH!\r\n", ch);
    return;
  }
  argument_interpreter(argument, Gbuf1, Gbuf2);
  if (*Gbuf1 && str_cmp(Gbuf1, "all"))
  {
    obj_object = get_obj_in_list_vis(ch, Gbuf1, ch->carrying);
    if (obj_object)
    {
      if (*Gbuf2)
      {
        keyword = search_block(Gbuf2, keywords, FALSE); /*
                                                         * Partial
                                                         * Match
                                                         */
        if (keyword == -1)
        {
          sprintf(Gbuf4, "%s is an unknown body location.\r\n", Gbuf2);
          send_to_char(Gbuf4, ch);
        }
        else
          wear(ch, obj_object, keyword + 1, 1);
      }
      else
      {
        keyword = -2;
        if (CAN_WEAR(obj_object, ITEM_WEAR_FINGER))
          keyword = 1;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_NECK))
          keyword = 2;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_BODY))
          keyword = 3;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_HEAD))
          keyword = 4;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_LEGS))
          keyword = 5;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_FEET))
          keyword = 6;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_HANDS))
          keyword = 7;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_ARMS))
          keyword = 8;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_BACK))
          keyword = 20;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_ABOUT))
          keyword = 9;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_WAIST))
          keyword = 10;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_WRIST))
          keyword = 11;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_SHIELD))
          keyword = 14;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_EYES))
          keyword = 15;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_FACE))
          keyword = 16;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_EARRING))
          keyword = 17;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_QUIVER))
          keyword = 18;
        else if (CAN_WEAR(obj_object, ITEM_GUILD_INSIGNIA))
          keyword = 19;
        else if (CAN_WEAR(obj_object, ITEM_ATTACH_BELT))
          keyword = 21;
        else if (CAN_WEAR(obj_object, ITEM_HORSE_BODY))
          keyword = 22;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_TAIL))
          keyword = 23;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_NOSE))
          keyword = 24;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_HORN))
          keyword = 25;
        else if (CAN_WEAR(obj_object, ITEM_WEAR_IOUN))
          keyword = 26;
        else if (CAN_WEAR(obj_object, ITEM_SPIDER_BODY))
          keyword = 27;

        if (keyword == -2)
        {
          send_to_char("That doesn't seem to work.\r\n", ch);
          return;
        }

        wear(ch, obj_object, keyword, 1);
      }
    }
    else
    {
      sprintf(Gbuf3, "You do not seem to have the '%s'.\r\n", Gbuf1);
      send_to_char(Gbuf3, ch);
    }
  }
  else if (!Gbuf1 || str_cmp(Gbuf1, "all"))
  {
    send_to_char("Wear what?\r\n", ch);
  }
  else
  {
    /*
     * WEAR ALL
     */
    /*
     * rearranged things here, should be faster, as it only checks for
     * filling empty equipment slots now, and equip is usually at top
     * of inven. JAB
     */
    for (loop = 0; loop < CUR_MAX_WEAR; loop++)
      if (!(ch->equipment[equipment_pos_table[loop][2]]))
        for (obj_object = ch->carrying; obj_object; obj_object = next_obj)
        {
          next_obj = obj_object->next_content;
          if (obj_object->type != ITEM_SPELLBOOK)
          {
            if (CAN_WEAR(obj_object, equipment_pos_table[loop][0]))
            {
              wear(ch, obj_object, equipment_pos_table[loop][1], 1);
/*
              if (!loop) {
                act("$n fully equips $mself.", TRUE, ch, 0, 0, TO_ROOM);
                act("You fully equip yourself.", FALSE, ch, 0, 0, TO_CHAR);
              }
*/
              break;
            }
          }
        }
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

  if (IS_ANIMAL(ch) || IS_DRAGON(ch))
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
      sprintf(Gbuf3, "You do not seem to have the '%s'.\r\n", Gbuf1);
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
      wear(ch, obj_object, 13, 1);
    }
    else
    {
      sprintf(Gbuf3, "You do not seem to have the '%s'.\r\n", Gbuf1);
      send_to_char(Gbuf3, ch);
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

/* returns true if ch is wearing perm invis eq */
int wearing_invis(P_char ch)
{
  int      found = 0, k;

  for (k = 0; k < MAX_WEAR; k++)
    if (ch->equipment[k] &&
        IS_SET(ch->equipment[k]->bitvector, AFF_INVISIBLE))
      found = 1;
  return found;
}

void do_remove(P_char ch, char *argument, int cmd)
{
  P_obj    obj_object, temp_obj;
  struct obj_affect *o_af;
  int      j, k;
  bool     was_invis;
  char     Gbuf1[MAX_STRING_LENGTH];

  one_argument(argument, Gbuf1);

  was_invis = IS_SET(ch->specials.affected_by, AFF_INVISIBLE) ||
    IS_SET(ch->specials.affected_by2, AFF2_MINOR_INVIS);

  if (*Gbuf1)
  {
    if (!str_cmp(Gbuf1, "all"))
    {
      if(IS_PC(ch) &&
        affected_by_spell(ch, TAG_PVPDELAY))
      {
        act("$n frantically attempts to remove all of $s clothes and equipment!", FALSE, ch, 0, 0, TO_ROOM);
        act("&+RWay too much adrenaline to perform a remove all.", FALSE, ch, 0, 0, TO_CHAR);
        CharWait(ch, PULSE_VIOLENCE * 1);
        return;
      } 
      
      for (k = 0; k < MAX_WEAR; k++)
      {
        if (ch->equipment[k])
          if (IS_SET(ch->equipment[k]->extra_flags, ITEM_NODROP) &&
              !IS_TRUSTED(ch))
          {
            act("$p won't budge!  Perhaps it's cursed?!?", TRUE, ch,
                ch->equipment[k], 0, TO_CHAR);
            continue;
          }
          else if (CAN_CARRY_N(ch) > IS_CARRYING_N(ch))
          {
            act("You stop using $p.", FALSE, ch, ch->equipment[k], 0,
                TO_CHAR);
            if (!k)
              act("$n removes all of $s equipment.", TRUE, ch, 0, 0, TO_ROOM);
            temp_obj = ch->equipment[k];
            obj_to_char(unequip_char(ch, k), ch);
            if (IS_SET(temp_obj->bitvector, AFF_INVISIBLE) &&
                affected_by_spell(ch, SKILL_PERMINVIS) && !wearing_invis(ch))
              affect_from_char(ch, SKILL_PERMINVIS);

            if (temp_obj && (o_af = get_obj_affect(temp_obj, SKILL_ENCHANT)))
            {
              affect_from_char(ch, o_af->data);
              act("&+ysome of your magic dissipates..&n", FALSE, ch, temp_obj,
                  0, TO_CHAR);
            }

          }
          else
          {
            send_to_char("You can't carry that many items.\r\n", ch);
            break;
          }
      }
    }
    else
    {
     
      obj_object = get_object_in_equip(ch, Gbuf1, &j);
      
      if (obj_object)
      {
        if (IS_SET(obj_object->extra_flags, ITEM_NODROP) && !IS_TRUSTED(ch))
        {
          act("$p won't budge!  Perhaps it's cursed?!?", TRUE, ch, obj_object,
              0, TO_CHAR);
          return;
        }
        else if (CAN_CARRY_N(ch) > IS_CARRYING_N(ch))
        {
          act("You stop using $p.", FALSE, ch, obj_object, 0, TO_CHAR);
          act("$n stops using $p.", TRUE, ch, obj_object, 0, TO_ROOM);
          if (ch->equipment[WEAR_WAIST] &&
              ch->equipment[WEAR_WAIST] == obj_object)
          {
            if (ch->equipment[WEAR_ATTACH_BELT_1])
              obj_to_char(unequip_char(ch, WEAR_ATTACH_BELT_1), ch);
            if (ch->equipment[WEAR_ATTACH_BELT_2])
              obj_to_char(unequip_char(ch, WEAR_ATTACH_BELT_2), ch);
            if (ch->equipment[WEAR_ATTACH_BELT_3])
              obj_to_char(unequip_char(ch, WEAR_ATTACH_BELT_3), ch);
          }
          obj_to_char(unequip_char(ch, j), ch);
          if (IS_SET(obj_object->bitvector, AFF_INVISIBLE) &&
              affected_by_spell(ch, SKILL_PERMINVIS) && !wearing_invis(ch))
            affect_from_char(ch, SKILL_PERMINVIS);

          if (obj_object &&
              (o_af = get_obj_affect(obj_object, SKILL_ENCHANT)))
          {
            affect_from_char(ch, o_af->data);
            act("&+ysome of your magic dissipates..&n", FALSE, ch, obj_object,
                0, TO_CHAR);
          }



          /* check to see if removing weapon in combat */
          /* No - Zod
             if (IS_FIGHTING(ch)) {
             if (obj_object->type != ITEM_TOTEM) {
             if (number(1,100) > GET_LEVEL(ch)) {
             act("&+ROOPS!&N You lose your grip on $p.", FALSE, ch, obj_object, 0, TO_CHAR);
             act("$n loses $s grip on $p, causing it to fall to the ground.", TRUE, ch, obj_object, 0, TO_ROOM);
             obj_from_char(obj_object, TRUE);
             obj_to_room(obj_object, ch->in_room);
             CharWait(ch, PULSE_VIOLENCE);
             }
             }
             }
           */
        }
        else
        {
          send_to_char("You can't carry that many items.\r\n", ch);
        }
      }
      else
      {
        send_to_char("You are not using it.\r\n", ch);
      }
    }
  }
  else
  {
    send_to_char("Remove what?\r\n", ch);
  }

  balance_affects(ch);
  if (was_invis && !IS_SET(ch->specials.affected_by, AFF_INVISIBLE)
      && !IS_SET(ch->specials.affected_by2, AFF2_MINOR_INVIS))
  {
    act("$n snaps into visibility.", FALSE, ch, 0, 0, TO_ROOM);
    act("You snap into visibility.", FALSE, ch, 0, 0, TO_CHAR);
  }

  /*
   * added by DTS 5/18/95 to solve light bug
   */
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
     
  if (((GET_C_INT(ch) + GET_C_WIS(ch) + GET_C_LUCK(ch)) / 3) > number(1, 101))
    return TRUE;
  return FALSE;
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
      sprintf(buf2, "The area appears to be rich in: %s\r\n", buf);
      send_to_char(buf2, ch);
//      found_something = TRUE;
    }*/
  }
  else
  {
    generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &dummy, &k);
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
   */
  /*
   * but only when searching room, not containers in room. JAB
   */
  if (!*name)
    for (door = 0; (door < NUM_EXITS) &&
         (!found_something || IS_TRUSTED(ch)); door++)
      if (EXIT(ch, door) && IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) &&
          !IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED))
      {
        if (find_chance(ch))
        {
          act("You find a secret entrance!", FALSE, ch, 0, 0, TO_CHAR);
          act("$n finds a secret entrance!", FALSE, ch, 0, 0, TO_ROOM);
          found_something = TRUE;
          REMOVE_BIT(EXIT(ch, door)->exit_info, EX_SECRET);
        }
      }
  /*
   * new bit, give them chance to find hiding thieves/mobs
   */
  for (dummy = world[ch->in_room].people;
       dummy && (!found_something || IS_TRUSTED(ch));
       dummy = dummy->next_in_room)
  {
    if (IS_AFFECTED(dummy, AFF_HIDE) && find_chance(ch) && !number(0, 3))
    {
      REMOVE_BIT(dummy->specials.affected_by, AFF_HIDE);
      if (CAN_SEE(ch, dummy))
      {
        act("You find $N lurking here!", FALSE, ch, 0, dummy, TO_CHAR);
        act("$n points out $N lurking here!", FALSE, ch, 0, dummy,
            TO_NOTVICT);
        if (find_chance(dummy) && !number(0, 3))
          act("You think $n has spotted you!", TRUE, ch, 0, dummy, TO_VICT);
        found_something = TRUE;
      }
      else
        SET_BIT(dummy->specials.affected_by, AFF_HIDE);
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
    act("Alas, your $q does not contain any poison!", FALSE, ch, poison, 0,
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
  act("Your $q now drips with a nasty poison!", FALSE, ch, weapon, 0,
      TO_CHAR);
  act("$n applies a vile-looking substance to $s $q!", FALSE, ch, weapon, 0,
      TO_ROOM);

  notch_skill(ch, SKILL_APPLY_POISON, 20);

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
    obj_from_char(poison, TRUE);
    extract_obj(poison, TRUE);
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
