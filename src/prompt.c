/*
 * ***************************************************************************
 *  File: prompt.c                                             Part of Duris *
 *  Usage: prompts, and infobars
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * 
 * *************************************************************************** 
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "olc.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "mm.h"

extern P_desc descriptor_list;

void make_prompt(void)
{
  P_char   t_ch = NULL, t_ch_f = NULL, tank = NULL;
  P_obj    t_obj_f = NULL;
  P_desc   point, next_point;
  char     promptbuf[MAX_INPUT_LENGTH]; /*, pname[512]; */
  int      percent, t_ch_p = 0;
  char     prompt_buf[256];

 for (point = descriptor_list; point; point = next_point)
  {
    next_point = point->next;
    t_ch = point->character;
    if (t_ch)
    {
      t_ch_f = t_ch->specials.fighting;
      t_obj_f = t_ch->specials.destroying_obj;
    }

    if (!point->prompt_mode     /*&& !point->showstr_count && !point->str &&
                                   !point->olc */ )
      continue;

    point->prompt_mode = 0;     /* reset it */

    if (point->showstr_count)
    {
      if (IS_ANSI_TERM(point))
        sprintf(promptbuf,
                "\n\033[32m[Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d)]\033[0m\n",
                point->showstr_page, point->showstr_count);
      else
        sprintf(promptbuf,
                "\n[Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d)]\n",
                point->showstr_page, point->showstr_count);
      if (write_to_descriptor(point->descriptor, promptbuf) < 0)
      {
        logit(LOG_COMM, "Closing socket on write error");
        close_socket(point);
      }
      continue;
    }
    if (point->str)
    {
      if (write_to_descriptor(point->descriptor, "] ") < 0)
      {
        logit(LOG_COMM, "Closing socket on write error");
        close_socket(point);
      }
      continue;
    }
/*    if (point->olc) {
      olc_prompt(point->olc);
      continue;
    }*/
    if (point->connected)
      continue;

    /* dont show a prompt if awaiting a yes/no answer - SAM 7-94 */
    if (point->confirm_state == CONFIRM_AWAIT)
      continue;

    if (t_ch)
      t_ch_p =
        IS_PC(t_ch) ? t_ch->only.pc->prompt : point->original->only.pc->
        prompt;



    if (IS_ANSI_TERM(point) && t_ch && t_ch->desc &&
        t_ch->desc->term_type == TERM_MSP)
      strcpy(promptbuf, "\n<prompt>\n\033[32m<");
    else if (IS_ANSI_TERM(point))
      strcpy(promptbuf, "\033[32m<");
    else
      strcpy(promptbuf, "<");

    /* infobar prompt */
    if (IS_SET(t_ch->specials.act, PLR_SMARTPROMPT))
    {
      UpdateScreen(t_ch, 0);
      sprintf(promptbuf, "&+R>&n");
      send_to_char(promptbuf, t_ch);
      return;
    }
    if ((IS_PC(t_ch) || IS_PC(point->original)) && t_ch_p)
    {
      if (IS_SET(t_ch_p, PROMPT_HIT))
      {
        if (IS_ANSI_TERM(point))
        {
          if (GET_MAX_HIT(t_ch) > 0)
            percent = (100 * GET_HIT(t_ch)) / GET_MAX_HIT(t_ch);
          else
            percent = -1;

          if (percent >= 66)
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[0;32m %dh",
                    t_ch->points.hit);
          }
          else if (percent >= 33)
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[33m %dh",
                    t_ch->points.hit);
          }
          else if (percent >= 15)
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[31m %dh",
                    t_ch->points.hit);
          }
          else
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[31;40;1;5m %dh",
                    t_ch->points.hit);
          }
        }
        else
        {
          sprintf(promptbuf + strlen(promptbuf), " %dh", t_ch->points.hit);
        }
      }
      if (IS_SET(t_ch_p, PROMPT_MAX_HIT))
      {
        if (IS_ANSI_TERM(point))
          sprintf(promptbuf + strlen(promptbuf), "\033[0;32m/%dH",
                  GET_MAX_HIT(t_ch));
        else
          sprintf(promptbuf + strlen(promptbuf), "/%dH", GET_MAX_HIT(t_ch));
      }
      if (IS_SET(t_ch_p, PROMPT_MANA))
      {
        if (IS_ANSI_TERM(point))
        {
          if (GET_MAX_MANA(t_ch) > 0)
          {
            percent = (100 * GET_MANA(t_ch)) / GET_MAX_MANA(t_ch);
          }
          else
          {
            percent = -1;       /* How could MAX_HIT be < 1?? */
          }
          if (percent >= 66)
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[0;32m %dm",
                    GET_MANA(t_ch));
          }
          else if (percent >= 33)
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[0;33m %dm",
                    GET_MANA(t_ch));
          }
          else if (percent >= 0)
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[0;31m %dm",
                    GET_MANA(t_ch));
          }
        }
        else
        {
          sprintf(promptbuf + strlen(promptbuf), " %dm", GET_MANA(t_ch));
        }
      }
      if (IS_SET(t_ch_p, PROMPT_MAX_MANA))
      {
        if (IS_ANSI_TERM(point))
          sprintf(promptbuf + strlen(promptbuf), "\033[0;32m/%dM",
                  GET_MAX_MANA(t_ch));
        else
          sprintf(promptbuf + strlen(promptbuf), "/%dM", GET_MAX_MANA(t_ch));
      }
      if (IS_SET(t_ch_p, PROMPT_MOVE))
      {
        if (IS_ANSI_TERM(point))
        {
          if (GET_MAX_VITALITY(t_ch) > 0)
          {
            percent = (100 * GET_VITALITY(t_ch)) / GET_MAX_VITALITY(t_ch);
          }
          else
          {
            percent = -1;
          }

          if (percent >= 66)
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[0;32m %dv",
                    t_ch->points.vitality);
          }
          else if (percent >= 33)
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[0;33m %dv",
                    t_ch->points.vitality);
          }
          else
          {
            sprintf(promptbuf + strlen(promptbuf), "\033[0;31m %dv",
                    t_ch->points.vitality);
          }
        }
        else
        {
          sprintf(promptbuf + strlen(promptbuf), " %dv",
                  t_ch->points.vitality);
        }
      }
      if (IS_SET(t_ch_p, PROMPT_MAX_MOVE))
      {
        if (IS_ANSI_TERM(point))
          sprintf(promptbuf + strlen(promptbuf), "\033[0;32m/%dV",
                  GET_MAX_VITALITY(t_ch));
        else
          sprintf(promptbuf + strlen(promptbuf), "/%dV",
                  GET_MAX_VITALITY(t_ch));
      }
      if (IS_SET(t_ch_p, PROMPT_STATUS))
      {
        if (IS_ANSI_TERM(point))
          strcat(promptbuf, " \033[0;36mPos:\033[0m");
        else
          strcat(promptbuf, " Pos:");
        if (GET_POS(t_ch) == POS_STANDING)
          if (IS_ANSI_TERM(point))
            strcat(promptbuf, "\033[1;32m standing\033[0m");
          else
            strcat(promptbuf, " standing");
        else if (GET_POS(t_ch) == POS_SITTING)
          if (IS_ANSI_TERM(point))
            strcat(promptbuf, "\033[0;36m sitting\033[0m");
          else
            strcat(promptbuf, " sitting");
        else if (GET_POS(t_ch) == POS_KNEELING)
          if (IS_ANSI_TERM(point))
            strcat(promptbuf, "\033[0;36m kneeling\033[0m");
          else
            strcat(promptbuf, " kneeling");
        else if (GET_POS(t_ch) == POS_PRONE)
          if (IS_ANSI_TERM(point))
            strcat(promptbuf, "\033[0;31m on your ass\033[0m");
          else
            strcat(promptbuf, " on your ass");
      
#ifdef STANCES_ALLOWED
        if (IS_ANSI_TERM(point))
          strcat(promptbuf, " \033[0;36mSta:\033[0m");
        else
          strcat(promptbuf, " Sta:");
        
        if(IS_AFFECTED5(t_ch, AFF5_STANCE_DEFENSIVE)){
          if (IS_ANSI_TERM(point)) 
          strcat(promptbuf, "\033[0;33m Def\033[0m");
          else
          strcat(promptbuf, " Def");
        }
        else if(IS_AFFECTED5(t_ch, AFF5_STANCE_OFFENSIVE)){
          if (IS_ANSI_TERM(point))
            strcat(promptbuf, "\033[0;31m Off\033[0m");
          else
            strcat(promptbuf, " Off"); 
        }
        else
          if (IS_ANSI_TERM(point))
            strcat(promptbuf, "\033[0;32m none\033[0m");
          else
            strcat(promptbuf, " None");
#endif
      }
      
      if (IS_SET(t_ch_p, PROMPT_TWOLINE) &&
          (t_ch_p & (PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MANA |
                     PROMPT_MAX_MANA | PROMPT_MOVE | PROMPT_MAX_MOVE)))
        if (IS_ANSI_TERM(point))
          strcat(promptbuf, "\033[0;32m >\n\033[32m<");
        else
          strcat(promptbuf, " >\n<");

      /* the prompt elements only active while fighting */
      if (t_ch_f && (t_ch->in_room == t_ch_f->in_room))
      {

        /* TANK elements only active if... */
        if ((tank = t_ch_f->specials.fighting) &&
            (t_ch->in_room == tank->in_room))
        {

          if (IS_SET(t_ch_p, PROMPT_TANK_NAME))
          {
            if (IS_ANSI_TERM(point))
              sprintf(promptbuf + strlen(promptbuf), " \033[0;34;1mT: %s",
                      (t_ch != tank &&
                       !CAN_SEE(t_ch, tank)) ? "someone" : (IS_PC(tank)
                                                          ? PERS(tank, t_ch, 0, true)
                                                          : (FirstWord
                                                             ((tank)->player.
                                                              name))));
            else
              sprintf(promptbuf + strlen(promptbuf), " T: %s",
                      (t_ch != tank &&
                       !CAN_SEE(t_ch, tank)) ? "someone" : (IS_PC(tank)
                                                          ? PERS(tank, t_ch, 0, true)
                                                          : (FirstWord
                                                             ((tank)->player.
                                                              name))));
          }
          if (IS_SET(t_ch_p, PROMPT_STATUS) &&
              IS_SET(t_ch_p, PROMPT_TANK_COND))
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, " \033[0;36mTP:\033[0m");
            else
              strcat(promptbuf, " TP:");
            if (GET_POS(tank) == POS_STANDING)
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;32m sta\033[0m");
              else
                strcat(promptbuf, " sta");
            else if (GET_POS(tank) == POS_SITTING)
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;32m sit\033[0m");
              else
                strcat(promptbuf, " sit");
            else if (GET_POS(tank) == POS_KNEELING)
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;32m kne\033[0m");
              else
                strcat(promptbuf, " kne");
            else if (GET_POS(tank) == POS_PRONE)
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;32m ass\033[0m");
              else
                strcat(promptbuf, " ass");
          }
          if (IS_SET(t_ch_p, PROMPT_TANK_COND))
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0m TC:");
            else
              strcat(promptbuf, " TC:");
            if (GET_MAX_HIT(tank) > 0)
            {
              percent = (100 * GET_HIT(tank)) / GET_MAX_HIT(tank);
            }
            else
            {
              percent = -1;
            }
            if (percent >= 100)
            {
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;32m excellent");
              else
                strcat(promptbuf, " excellent");
            }
            else if (percent >= 90)
            {
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;33m few scratches");
              else
                strcat(promptbuf, " few scratches");
            }
            else if (percent >= 75)
            {
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;33;40;1m small wounds");
              else
                strcat(promptbuf, " small wounds");
            }
            else if (percent >= 50)
            {
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;35;40;1m few wounds");
              else
                strcat(promptbuf, " few wounds");
            }
            else if (percent >= 30)
            {
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;35m nasty wounds");
              else
                strcat(promptbuf, " nasty wounds");
            }
            else if (percent >= 15)
            {
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;31;40;1m pretty hurt");
              else
                strcat(promptbuf, " pretty hurt");
            }
            else if (percent >= 0)
            {
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;31m awful");
              else
                strcat(promptbuf, " awful");
            }
            else
            {
              if (IS_ANSI_TERM(point))
                strcat(promptbuf, "\033[0;31m bleeding, close to death");
              else
                strcat(promptbuf, " bleeding, close to death");
            }
          }
        }
        if (IS_SET(t_ch_p, PROMPT_ENEMY))
        {
          if (IS_ANSI_TERM(point))
            sprintf(promptbuf + strlen(promptbuf), " \033[0;31mE: %s",
                    (!CAN_SEE(t_ch, t_ch_f)) ? "someone" : (IS_PC(t_ch_f)
                                                      ? PERS(t_ch_f, t_ch, 0, true)
                                                      : (FirstWord
                                                         ((t_ch_f)->player.
                                                          name))));
          else
            sprintf(promptbuf + strlen(promptbuf), " E: %s",
                    (!CAN_SEE(t_ch, t_ch_f) ) ? "someone" : (IS_PC(t_ch_f)
                                                      ? PERS(t_ch_f, t_ch, 0, true)
                                                      : (FirstWord
                                                         ((t_ch_f)->player.
                                                          name))));
        }
        if (IS_SET(t_ch_p, PROMPT_STATUS) &&
            IS_SET(t_ch_p, PROMPT_ENEMY_COND))
        {
          if (IS_ANSI_TERM(point))
            strcat(promptbuf, " \033[0;36mEP:\033[0m");
          else
            strcat(promptbuf, " EP:");
          if (GET_POS(t_ch_f) == POS_STANDING)
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;32m sta\033[0m");
            else
              strcat(promptbuf, " sta");
          else if (GET_POS(t_ch_f) == POS_SITTING)
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;32m sit\033[0m");
            else
              strcat(promptbuf, " sit");
          else if (GET_POS(t_ch_f) == POS_KNEELING)
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;32m kne\033[0m");
            else
              strcat(promptbuf, " kne");
          else if (GET_POS(t_ch_f) == POS_PRONE)
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;32m ass\033[0m");
            else
              strcat(promptbuf, " ass");
        }
        if (IS_SET(t_ch_p, PROMPT_ENEMY_COND))
        {
          if (GET_MAX_HIT(t_ch_f) > 0)
          {
            percent = (100 * GET_HIT(t_ch_f)) / GET_MAX_HIT(t_ch_f);
          }
          else
          {
            percent = -1;
          }
          if (IS_ANSI_TERM(point))
            strcat(promptbuf, "\033[0;36m EC:");
          else
            strcat(promptbuf, " EC:");
          if (percent >= 100)
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;32m excellent");
            else
              strcat(promptbuf, " excellent");
          }
          else if (percent >= 90)
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;33m few scratches");
            else
              strcat(promptbuf, " few scratches");
          }
          else if (percent >= 75)
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;33;40;1m small wounds");
            else
              strcat(promptbuf, " small wounds");
          }
          else if (percent >= 50)
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;35;40;1m few wounds");
            else
              strcat(promptbuf, " few wounds");
          }
          else if (percent >= 30)
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;35m nasty wounds");
            else
              strcat(promptbuf, " nasty wounds");
          }
          else if (percent >= 15)
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;31;40;1m pretty hurt");
            else
              strcat(promptbuf, " pretty hurt");
          }
          else if (percent >= 0)
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;31m awful");
            else
              strcat(promptbuf, " awful");
          }
          else
          {
            if (IS_ANSI_TERM(point))
              strcat(promptbuf, "\033[0;31m bleeding, close to death");
            else
              strcat(promptbuf, " bleeding, close to death");
          }
        }
      }
      if( t_obj_f )
      {
        if (IS_ANSI_TERM(point))
        {
          strcat(promptbuf, "\033[32m E: ");
          strcat(promptbuf, FirstWord(t_obj_f->name) );
          strcat(promptbuf, " EC: " );
          if(t_obj_f->condition > 90)
            strcat(promptbuf, "\033[1;32m");
          else if(t_obj_f->condition > 70)
            strcat(promptbuf, "\033[32m");
          else if(t_obj_f->condition > 50)
            strcat(promptbuf, "\033[0;33m");
          else if(t_obj_f->condition > 20)
            strcat(promptbuf, "\033[0;31m");
          else
            strcat(promptbuf, "\033[1;31m");

          sprintf(promptbuf + strlen(promptbuf), "%d ",
            t_obj_f->condition );
        }
        else
        {
          strcat(promptbuf, " E: ");
          strcat(promptbuf, FirstWord(t_obj_f->name) );
          sprintf(promptbuf + strlen(promptbuf), " EC: %d",
            t_obj_f->condition );
        }
      }

      if (IS_SET(t_ch_p, PROMPT_VIS) && IS_TRUSTED(t_ch))
      {
        if (IS_ANSI_TERM(point))
          strcat(promptbuf, "\033[0;35m");
        sprintf(promptbuf + strlen(promptbuf), " Vis: %d",
                t_ch->only.pc->wiz_invis);
      }
      if (IS_SET(t_ch->specials.act, PLR_AFK))
      {
        if (IS_ANSI_TERM(point))
          strcat(promptbuf, "\033[0m (\033[31;40;1mAFK\033[0m)");
        else
          strcat(promptbuf, " (AFK)");
      }
    }
    if (IS_ANSI_TERM(point))
      strcat(promptbuf, "\033[0;32m> ");
    else
      strcat(promptbuf, " > ");

    if (t_ch && t_ch->desc && t_ch->desc->term_type == TERM_MSP)
    {
      strcat(promptbuf, "\033[0m\n</prompt>\n");
    }

    if (t_ch_p)
    {
      /* give the log some prompts */
      prompt_buf[0] = '\0';
      append_prompt(point->character, prompt_buf);
      write_to_pc_log(point->character, prompt_buf, LOG_PUBLIC);

      if (write_to_descriptor(point->descriptor, promptbuf) < 0)
      {
        logit(LOG_COMM, "Closing socket on write error");
        close_socket(point);
        continue;
      }
      if (IS_ANSI_TERM(point) && point->character && point->character->desc && !(point->character->desc->term_type == TERM_MSP))
      {
        if (write_to_descriptor(point->descriptor, "\033[0m") < 0)
        {
          logit(LOG_COMM, "Closing socket on write error");
          close_socket(point);
          continue;
        }
      }

    }
  }
}


/* Converts numerical data to a dynamic line for infobar */
char    *make_bar(long val, long max, long len)
{
  static char buf[512];
  int      i, n;
  int      percent;

  strcpy(buf, "&+R<&n");

  percent = (100 * val) / max;
  if (percent >= 100)
    sprintf(buf + strlen(buf), "&+g");
  else if (percent >= 90)
    sprintf(buf + strlen(buf), "&+y");
  else if (percent >= 75)
    sprintf(buf + strlen(buf), "&+Y");
  else if (percent >= 50)
    sprintf(buf + strlen(buf), "&+M");
  else if (percent >= 30)
    sprintf(buf + strlen(buf), "&+m");
  else if (percent >= 15)
    sprintf(buf + strlen(buf), "&+R");
  else
    sprintf(buf + strlen(buf), "&+r");

  if (val > max)
  {
    for (i = 0; i < len; i++)
      sprintf(buf + strlen(buf), "|");
    sprintf(buf + strlen(buf), "&n+");
  }
  else
  {
    for (i = ((val * len) / max), n = 0; n < i; n++)
      sprintf(buf + strlen(buf), "|");
    sprintf(buf + strlen(buf), "&n");
    while ((n++) < len)
      sprintf(buf + strlen(buf), "|");
  }
  sprintf(buf + strlen(buf), "&+R>&n");

  return buf;
}

/* Keeps the infobar updated. Will look like shit if the screen has not
   been previously initialized! */
void UpdateScreen(P_char ch, int update)
{
  char     buf[255];
  int      size, percent;
  P_char   enemy = NULL, tank = NULL;

  size = ch->only.pc->screen_length + 1;
  if (ch->specials.fighting)
    enemy = ch->specials.fighting;

/* hits */
  sprintf(buf, VT_CURSAVE);
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size - 3, 13);
  send_to_char(buf, ch);
  sprintf(buf, "          ");
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size - 3, 13);
  send_to_char(buf, ch);
  sprintf(buf, "%s", make_bar(GET_HIT(ch), GET_MAX_HIT(ch), 50));
  send_to_char(buf, ch);
  sprintf(buf, VT_CURREST);
  send_to_char(buf, ch);

/* moves */
  sprintf(buf, VT_CURSAVE);
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size - 2, 13);
  send_to_char(buf, ch);
  sprintf(buf, "          ");
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size - 2, 13);
  send_to_char(buf, ch);
  sprintf(buf, "%s", make_bar(GET_VITALITY(ch), GET_MAX_VITALITY(ch), 50));
  send_to_char(buf, ch);
  sprintf(buf, VT_CURREST);
  send_to_char(buf, ch);

/* mana */
  if (IS_SET(ch->only.pc->prompt, PROMPT_MANA))
  {
    sprintf(buf, VT_CURSAVE);
    send_to_char(buf, ch);
    sprintf(buf, VT_CURSPOS, size - 1, 13);
    send_to_char(buf, ch);
    sprintf(buf, "          ");
    send_to_char(buf, ch);
    sprintf(buf, VT_CURSPOS, size - 1, 13);
    send_to_char(buf, ch);
    sprintf(buf, "%s", make_bar(GET_MANA(ch), GET_MAX_MANA(ch), 50));
    send_to_char(buf, ch);
    sprintf(buf, VT_CURREST);
    send_to_char(buf, ch);
  }

/* God visibility */
  if (IS_TRUSTED(ch))
  {
    sprintf(buf, VT_CURSAVE);
    send_to_char(buf, ch);
    sprintf(buf, VT_CURSPOS, size - 1, 70);
    send_to_char(buf, ch);
    sprintf(buf, "     ");
    send_to_char(buf, ch);
    sprintf(buf, VT_CURSPOS, size - 1, 70);
    send_to_char(buf, ch);
    sprintf(buf, "%d", ch->only.pc->wiz_invis);
    send_to_char(buf, ch);
    sprintf(buf, VT_CURREST);
    send_to_char(buf, ch);
  }

/* tank */
  sprintf(buf, VT_CURSAVE);
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size, 7);
  send_to_char(buf, ch);
  sprintf(buf, "          ");
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size, 7);
  send_to_char(buf, ch);
  if (enemy && (enemy->in_room == ch->in_room) &&
      IS_SET(ch->specials.act, PLR_DEBUG))
  {
    if ((tank = enemy->specials.fighting) && (ch->in_room == tank->in_room))
      sprintf(buf, "%s", ch == tank ? "yourself" : !CAN_SEE(ch, tank) ?
              "someone" : J_NAME(tank));
    if (ch != tank)
    {
      if (GET_MAX_HIT(tank) > 0 && GET_HIT(tank) > 0)
        percent = (100 * GET_HIT(tank)) / GET_MAX_HIT(tank);
      else
        percent = -1;
      if (percent >= 100)
        strcat(buf, " excellent");
      else if (percent >= 90)
        strcat(buf, " few scratches");
      else if (percent >= 75)
        strcat(buf, " small wounds");
      else if (percent >= 50)
        strcat(buf, " few wounds");
      else if (percent >= 30)
        strcat(buf, " nasty wounds");
      else if (percent >= 15)
        strcat(buf, " pretty hurt");
      else if (percent >= 0)
        strcat(buf, " awful");
      else
        strcat(buf, " bleeding, close to death");
    }
  }
  else
    sprintf(buf, " N/A");
  send_to_char(buf, ch);
  sprintf(buf, VT_CURREST);
  send_to_char(buf, ch);

/* enemy */
  sprintf(buf, VT_CURSAVE);
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size, 50);
  send_to_char(buf, ch);
  sprintf(buf, "          ");
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size, 50);
  send_to_char(buf, ch);
  if (enemy && IS_SET(ch->specials.act, PLR_DEBUG))
  {
    sprintf(buf, " %s", !CAN_SEE(ch, enemy) ? "someone" : J_NAME(enemy));
    if (GET_MAX_HIT(enemy) > 0 && GET_HIT(enemy) > 0)
      percent = (100 * GET_HIT(enemy)) / GET_MAX_HIT(enemy);
    else
      percent = -1;
    if (percent >= 100)
      strcat(buf, " excellent");
    else if (percent >= 90)
      strcat(buf, " few scratches");
    else if (percent >= 75)
      strcat(buf, " small wounds");
    else if (percent >= 50)
      strcat(buf, " few wounds");
    else if (percent >= 30)
      strcat(buf, " nasty wounds");
    else if (percent >= 15)
      strcat(buf, " pretty hurt");
    else if (percent >= 0)
      strcat(buf, " awful");
    else
      strcat(buf, " bleeding, close to death");
  }
  else
    sprintf(buf, " N/A");
  send_to_char(buf, ch);

  sprintf(buf, VT_CURREST);
  send_to_char(buf, ch);
}

/* Sets up the static aspects of the infobar, and displays first update */
void InitScreen(P_char ch)
{
  char     buf[255];
  int      size;

  size = ch->only.pc->screen_length + 1;
  sprintf(buf, VT_HOMECLR);
  send_to_char(buf, ch);
  sprintf(buf, VT_MARGSET, 0, size - 5);
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size - 4, 1);
  send_to_char(buf, ch);
  sprintf(buf,
          "&+r-===================================Duris=====================================-&n");
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size - 3, 1);
  send_to_char(buf, ch);
  sprintf(buf, "Vitality: ");
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size - 2, 1);
  send_to_char(buf, ch);
  sprintf(buf, "Stamina : ");
  send_to_char(buf, ch);
  if (IS_SET(ch->only.pc->prompt, PROMPT_MANA))
  {
    sprintf(buf, VT_CURSPOS, size - 1, 1);
    send_to_char(buf, ch);
    sprintf(buf, "Mental: ");
    send_to_char(buf, ch);
  }

  if (IS_TRUSTED(ch))
  {
    sprintf(buf, VT_CURSPOS, size - 1, 70);
    send_to_char(buf, ch);
    sprintf(buf, "Vis: ");
    send_to_char(buf, ch);
  }
  sprintf(buf, VT_CURSPOS, size, 1);
  send_to_char(buf, ch);
  sprintf(buf, "Tank: ");
  send_to_char(buf, ch);
  sprintf(buf, VT_CURSPOS, size, 40);
  send_to_char(buf, ch);
  sprintf(buf, "Enemy: ");
  send_to_char(buf, ch);

/* first update */
  UpdateScreen(ch, 0);

/* put cursor back to top */
  sprintf(buf, VT_CURSPOS, 0, 0);
  send_to_char(buf, ch);
}
