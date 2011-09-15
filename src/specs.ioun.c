/*
   Procs for the shady grove orc hometown
 */

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"

/*
   external vars
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_event current_event;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern const struct stat_data stat_factor[];
extern int pulse;
extern int top_of_world;
extern int top_of_zone_table;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

int ioun_sustenance(P_obj obj, P_char ch, int cmd, char *argument)
{
  if (!ch || !obj || !IS_PC(ch))
    return FALSE;

  if (!OBJ_WORN(obj))
    return FALSE;

  if (argument && (cmd == CMD_SAY))
  {
    if (isname(argument, "feedme"))
    {
      int      curr_time = time(NULL);

      if (curr_time >= obj->timer[0] + 60)
      {
        act("You say 'feedme'", FALSE, ch, 0, 0, TO_CHAR);
        act("$n says 'feedme'", TRUE, ch, obj, NULL, TO_ROOM);

        if (ch->specials.conditions[FULL] <= 4 ||
            ch->specials.conditions[THIRST] <= 4)
        {
          act("&+W$n's $p &+Wglows with a bright light!&n", FALSE, ch, obj, 0,
              TO_ROOM);
          act("&+WYour $p &+Wglows with a bright light!&n", FALSE, ch, obj, 0,
              TO_CHAR);
          spell_greater_sustenance(50, ch, NULL, SPELL_TYPE_SPELL, NULL,
                                   NULL);

          obj->timer[0] = curr_time;
        }
        return TRUE;
      }
    }


  }

  return FALSE;

}

int ioun_testicle(P_obj obj, P_char ch, int cmd, char *argument)
{
  int      current_time = time(NULL);
  P_char   next, target, vict;
  P_obj    pobj, x;
  int      i = 0;

  return FALSE;

  if (!ch || !obj || !IS_PC(ch))
    return FALSE;

  if (!OBJ_WORN(obj))
    return FALSE;

  if (cmd != CMD_RUB)
    return FALSE;

  act("&+W$n's $p &+Wmoans with pleasure!&n", FALSE, ch, obj, 0, TO_ROOM);
  act("&+WYour $p &+Wmoans with pleasure!&n", FALSE, ch, obj, 0, TO_CHAR);

  if ((cmd == CMD_RUB) && OBJ_WORN(obj) &&
      ((obj->timer[0] + 604800) <= current_time))
  {
    if (GET_LEVEL(ch) == 50)
    {
      act("&+W$n looks strangely satisfied...&n", FALSE, ch, obj, 0, TO_ROOM);
      act("&+WYou feel satisfied...&n", FALSE, ch, obj, 0, TO_CHAR);
      obj->value[0]++;
      advance_level(ch, TRUE);
      obj->timer[0] = current_time;
      if (obj->value[0] > 1)
      {
        act("&+WOh NO!!  Something's wrong!&n", FALSE, ch, obj, 0, TO_ROOM);
        act("&+W$p &+Wbegins to pulsate!&n", FALSE, ch, obj, 0, TO_ROOM);
        act("&+W$p &+Wemits a SCALDING STREAM of acidic liquid!!&n", FALSE,
            ch, obj, 0, TO_ROOM);
        act("&+WYou try to avoid the stream, but fail miserably!&n", FALSE,
            ch, obj, 0, TO_ROOM);

        act("&+WOh NO!!  Something's wrong!&n", FALSE, ch, obj, 0, TO_CHAR);
        act("&+W$p &+Wbegins to pulsate!&n", FALSE, ch, obj, 0, TO_CHAR);
        act("&+W$p &+Wemits a SCALDING STREAM of acidic liquid!!&n", FALSE,
            ch, obj, 0, TO_CHAR);
        act("&+WYou try to avoid the stream, but fail miserably!&n", FALSE,
            ch, obj, 0, TO_CHAR);

        for (vict = world[ch->in_room].people; vict; vict = next)
        {
          next = vict->next_in_room;
          if (IS_TRUSTED(vict) || IS_NPC(vict))
            continue;

          do
          {
            if (vict->equipment[i])
            {
              pobj = vict->equipment[i];
              i++;
              if (IS_ARTIFACT(pobj))
                continue;
              if (IS_IOUN(pobj))
                continue;
              if (OBJ_CARRIED(pobj))
              {                 /* remove the obj */
                obj_from_char(pobj, TRUE);
              }
              else if (OBJ_WORN(pobj))
              {
                unequip_char_dale(pobj);
              }
              else if (OBJ_INSIDE(pobj))
              {
                obj_from_obj(pobj);
                obj_to_room(pobj, ch->in_room);
              }
              if (pobj->contains)
              {
                while (pobj->contains)
                {
                  x = pobj->contains;
                  obj_from_obj(x);
                  obj_to_room(x, ch->in_room);
                }
              }
              if (pobj)
              {
                extract_obj(pobj, TRUE);
                pobj = NULL;
              }
            }
            else
              i++;
          }
          while (i < MAX_WEAR);

          GET_HIT(vict) = -9000;
        }
        extract_obj(obj, TRUE);
      }

      return TRUE;
    }
  }

  return FALSE;

}

int ioun_warp(P_obj obj, P_char ch, int cmd, char *argument)
{
  char     Gbuf[MAX_STRING_LENGTH];
  char     Gbuf2[MAX_STRING_LENGTH];
  P_char   next, target, vict;
  int      dam;

  int      curr_time = time(NULL);;

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!IS_PC(ch))
    return FALSE;

  if (!ch || !obj)
    return FALSE;

  if (!OBJ_WORN(obj))
    return FALSE;

  if (obj->timer[0] + 60 > curr_time)
    return FALSE;

  half_chop(argument, Gbuf, Gbuf2);

  if (*Gbuf && (cmd == CMD_SAY))
  {
    if (!strcmp(Gbuf, "join") && *Gbuf2)
    {
      obj->value[5]++;
      if (obj->value[5] > 10)
        obj->value[5] == 10;
      if (obj->value[5] == 5)
      {
        send_to_char("Uhoh, your stone seems unhappy...\r\n", ch);
        return FALSE;
      }

      if (obj->value[5] > 5)
      {
        act("Your $q &+WEXPLODES&n, &+Lengulfing the area in a dark haze!", 0,
            ch, obj, 0, TO_CHAR);
        act("$n's $q &+WEXPLODES&n, &+Lengulfing the area in a dark haze!", 0,
            ch, obj, 0, TO_ROOM);
        for (vict = world[ch->in_room].people; vict; vict = next)
        {
          next = vict->next_in_room;
          if (IS_TRUSTED(vict) || IS_NPC(vict))
            continue;

          dam = 450;
          if ((GET_HIT(vict) - dam) < -10)
          {
            act("&+LYou are engulfed by the haze!&N", FALSE, ch, 0, 0,
                TO_CHAR);
            act("&+L$n&+L is engulfed completely by the haze!&N", TRUE, ch, 0,
                0, TO_ROOM);
            die(vict, ch);
          }
          else
            GET_HIT(vict) -= dam;
        }

        extract_obj(obj, TRUE);
        return FALSE;
      }

      target = get_char_vis(ch, Gbuf2);
      if (target && target != ch && IS_PC(target) && !IS_TRUSTED(target))
      {
        act
          ("&+G$n's $p &+Gbegins swirling in circles very fast, engulfing $s whole body in a blurry haze!&n\r\n"
           "&+wThe haze slowly subsides, and $n is gone!&n", FALSE, ch, obj,
           0, TO_ROOM);
        act("&+GYour $p &+Gbegins whirling around your body very fast!!&n",
            FALSE, ch, obj, 0, TO_CHAR);
        char_from_room(ch);
        char_to_room(ch, target->in_room, -1);
        act
          ("&+wAs the haze slowly subsides, you realize you are somewhere else!&n",
           FALSE, ch, obj, 0, TO_CHAR);
        act
          ("&+wA soft, &+bblurry &+mhaze &+wfloats in from above and begins whirling about a central area...\r\n"
           "&+wAs the haze subides, $n is standing there with a grin on $s face.&n",
           FALSE, ch, obj, 0, TO_ROOM);
        obj->timer[0] = curr_time;
        return TRUE;
      }
    }
  }
  return FALSE;
}
