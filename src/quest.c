/* ***************************************************************************
 *  File: quest.c                                            Part of Duris *
 *  Usage: Quest procs for mobs... -DCL                                      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 *************************************************************************** */

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#include "comm.h"
#include "db.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "sql.h"

/* external variables */

extern Skill skills[];
extern P_char character_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const char *item_types[];
extern const struct stat_data stat_factor[];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern int mini_mode;
extern long new_exp_table[];  // Arih: Fixed type mismatch bug - was int, should be long

#define QUEST_FILE "areas/world.qst"
#define MINI_QUEST_FILE "areas/mini.qst"

struct quest_data quest_index[MAX_QUESTS];
int      number_of_quests = 0;

bool     quest_completion(struct quest_complete_data *, P_char, P_char);
void     give_reward(struct quest_complete_data *, P_char, P_char);

bool execute_quest_routine(P_char ch, int cmd)
{
  int arg;
  int qi = find_quester_id(GET_RNUM(ch));
  struct quest_msg_data *qdata = quest_index[qi].quest_message;

  if( !IS_ALIVE(ch) || IS_IMMOBILE(ch) )
    return false;

  for (qdata = quest_index[qi].quest_message; qdata; qdata = qdata->next) {
    if (cmd == CMD_NONE) {
      if (sscanf(qdata->key_words, QC_ACTION " %d", &arg) == 1) {
        if (!number(0, (arg * WAIT_SEC)/PULSE_MOBILE)) {
          act(qdata->message, FALSE, ch, 0, 0, TO_ROOM);
          return true;
        }
      }
//    } else if (cmd == CMD_INCOMING) {
    }
  }

  return false;
}

int binary_search(int num, int min, int max)
{
  int      mid, midn;

  if (min > max)
    return (-1);

  midn = (min + max) >> 1;
  mid = quest_index[midn].quester;

  if (num == mid)
    return (midn);
  else if (num < mid)
    return (binary_search(num, min, midn - 1));
  else
    return (binary_search(num, midn + 1, max));
}

int find_quester_id(int num)
{
  return (binary_search(num, 0, number_of_quests - 1));
}

void questcheck(P_char ch)
{
  int      i;
  char     tmp_buf[MAX_STRING_LENGTH];

  for (i = 1; i < number_of_quests; i++)
  {
    if (quest_index[i].quester <= quest_index[i - 1].quester)
    {
      snprintf(tmp_buf, MAX_STRING_LENGTH,
              "Real: %d Virtual: %d is out of order with Real: %d Virtual: %d\n",
              i, quest_index[i].quester, i - 1, quest_index[i - 1].quester);
      send_to_char(tmp_buf, ch);
    }
  }
}

bool quest_completion(struct quest_complete_data *qcp, P_char mob, P_char pl)
{
  struct goal_data *gp, *gp2;
  P_obj    obj;
  int      num_needed, count;

  for (gp = qcp->give; gp; gp = gp->next)
  {
    for (gp2 = gp->next, num_needed = 1; gp2; gp2 = gp2->next)
    {
      if ((gp2->goal_type == gp->goal_type) && (gp2->number == gp->number))
        num_needed++;
    }
    switch (gp->goal_type)
    {
    case QUEST_GOAL_ITEM:
      for (obj = mob->carrying, count = 0; obj; obj = obj->next_content)
        if (obj_index[obj->R_num].virtual_number == gp->number)
          count++;
      if (count < num_needed)
        return (FALSE);
      break;
    case QUEST_GOAL_ITEM_TYPE:
      for (obj = mob->carrying, count = 0; obj; obj = obj->next_content)
        if (obj->type == gp->number)
          count++;
      if (count < num_needed)
        return (FALSE);
      break;
    case QUEST_GOAL_COINS:
      if (GET_MONEY(mob) < gp->number)
        return (FALSE);
      break;
    }
  }
//  send_to_char(qcp->message, pl);
  act(qcp->message, FALSE, mob, 0, pl, qcp->echoAll ? TO_ROOM : TO_VICT);
  for (gp = qcp->give; gp; gp = gp->next)
  {
    if (gp->goal_type == QUEST_GOAL_ITEM)
    {
      for (obj = mob->carrying; obj; obj = obj->next_content)
        if (obj_index[obj->R_num].virtual_number == gp->number)
          break;
      if (!obj)
        return (FALSE);
      obj_from_char(obj);
      extract_obj(obj, TRUE); // Arti quest item?
      obj = NULL;
    }
    if (gp->goal_type == QUEST_GOAL_COINS)
      SUB_MONEY(mob, gp->number, 0);
  }
  for (gp = qcp->give; gp; gp = gp->next)
  {
    if (gp->goal_type == QUEST_GOAL_ITEM_TYPE)
    {
      for (obj = mob->carrying; obj; obj = obj->next_content)
        if (obj->type == gp->number)
          break;
      if (!obj)
        return (FALSE);
      obj_from_char(obj);
      extract_obj(obj, TRUE); // Arti quest item?
      obj = NULL;
    }
  }
  return (TRUE);
}

void give_reward(struct quest_complete_data *qcp, P_char mob, P_char pl)
{
  struct goal_data *gp;
  P_obj    obj;
  int      i;
  char     Gbuf1[MAX_STRING_LENGTH];
  int      value_pts = 0;
  char     buffer[1024];
  struct group_list *gl;
  int      group_fact = 1;
  int temp = 1;
  P_char rider;
  /*
     value_pts = 2500 * (GET_LEVEL(mob) / 2);

     if (pl->group)
     {

     for (gl = pl->group; gl; gl = gl->next)
     if ((pl->in_room == gl->ch->in_room) && (gl->ch != pl))
     group_fact++;

     value_pts = (int) value_pts / group_fact;

     value_pts = BOUNDED(1, value_pts, 10000 + number(100,1500) );

     for (gl = pl->group; gl; gl = gl->next)
     if ((pl->in_room == gl->ch->in_room) && (gl->ch != pl) && IS_PC(gl->ch)) {

     adjust_lvl_from_epics(gl->ch, value_pts);

     snprintf(buffer, 1024, "You gained %d epic points!!\r\n", (value_pts / 100));
     send_to_char(buffer, gl->ch);
     }  
     }

     value_pts = BOUNDED(1, value_pts, 10000 + number(100,1500) );

     snprintf(buffer, 1024, "You gained %d epic points!!\r\n", (value_pts / 100));


     adjust_lvl_from_epics(pl, value_pts);
     send_to_char(buffer, pl);
   */


  wizlog(58, "%s has completed quest from %s [%d].", GET_NAME(pl),
         mob->player.short_descr, GET_VNUM(mob));

  sql_log(pl, QUESTLOG, "Completed quest from %s &n[%d].", mob->player.short_descr, GET_VNUM(mob));

  for (gp = qcp->receive; gp; gp = gp->next)
  {
    switch (gp->goal_type)
    {
    case QUEST_GOAL_ITEM:
      obj = read_object(gp->number, VIRTUAL);

      if (!obj)
      {
        logit(LOG_DEBUG, "give_reward(): obj %d not loadable", gp->number);
        break;
      }

      // No duplicating artifacts.
      if( get_artifact_data_sql(OBJ_VNUM(obj), NULL) )
      {
        // It's imperative that we extract FALSE (no poofing arti data)..
        extract_obj(obj);
        break;
      }

      if ((IS_CARRYING_N(pl) < CAN_CARRY_N(pl)) &&
          ((IS_CARRYING_W(pl, rider) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(pl)))
      {
        obj_to_char(obj, pl);
        act("$n gives you $p.", FALSE, mob, obj, pl, TO_VICT);
        act("$n gives $p to $N.", FALSE, mob, obj, pl, TO_NOTVICT);
      }
      else
      {
        obj_to_room(obj, pl->in_room);
        act("$n places $p on the ground in front of you.", FALSE, mob, obj,
            pl, TO_VICT);
        act("$n places $p on the ground in front of $N.", FALSE, mob, obj, pl,
            TO_NOTVICT);
      }
      statuslog(58, "%s was rewarded %s [%d] by %s", GET_NAME(pl),
                  obj->short_description, obj_index[obj->R_num].virtual_number, mob->player.short_descr);
      sql_log(pl, QUESTLOG, "Rewarded %s [%d] by %s",
                obj->short_description, obj_index[obj->R_num].virtual_number, mob->player.short_descr);
      //sql_quest_finish(pl, mob, 1, obj_index[obj->R_num].virtual_number);
      break;
    case QUEST_GOAL_COINS:

	     /* if( (temp = sql_quest_trophy(mob)) > 1){
           snprintf(Gbuf1, MAX_STRING_LENGTH, "$n says 'This quest is very commonly done, reward is currently very low.'\r\n");
           act(Gbuf1, FALSE, mob, 0, pl, TO_VICT);
        }  else 
				*/
			temp = 1;

      snprintf(Gbuf1, MAX_STRING_LENGTH, "$n gives %s to you.", coin_stringv(gp->number / temp));
      act(Gbuf1, FALSE, mob, 0, pl, TO_VICT);
      act("$n gives some coins to $N.", FALSE, mob, 0, pl, TO_NOTVICT);
      statuslog(58, "%s was rewarded %d coins by %s", GET_NAME(pl), gp->number / temp, mob->player.short_descr);
      sql_log(pl, QUESTLOG, "Rewarded %s &ncoins by %s", coin_stringv(gp->number / temp), mob->player.short_descr);
      ADD_MONEY(pl, gp->number / temp);

      //sql_quest_finish(pl, mob, 2, gp->number / temp);
      break;
    case QUEST_GOAL_SKILL:
      if (IS_NPC(pl))
        break;

      for (i = 0; (i < MAX_SKILLS) && (i != gp->number); i++) ;

      if (i >= MAX_SKILLS)
        break;

      if( SKILL_DATA_ALL(pl, i).rlevel[pl->player.spec] > (int) GET_LEVEL(pl) )
      {
        act("$n says to you, 'Come back when you are more powerful.'",
            FALSE, mob, 0, pl, TO_VICT);
        break;
      }
      if (pl->only.pc->skills[gp->number].learned < 1)
      {
        snprintf(Gbuf1, MAX_STRING_LENGTH, "$n teaches you '%s'.", skills[gp->number].name);
        act(Gbuf1, FALSE, mob, 0, pl, TO_VICT);
        act("$n teaches $N a new skill.", FALSE, mob, 0, pl, TO_NOTVICT);
        pl->only.pc->skills[gp->number].learned = 1;
      }
      break;
    case QUEST_GOAL_EXP:
			/*
	  if( (temp = sql_quest_trophy(mob)) > 1){
	   snprintf(Gbuf1, MAX_STRING_LENGTH, "$n says 'This quest is very commonly done, reward is currently very low.'\r\n");
           act(Gbuf1, FALSE, mob, 0, pl, TO_VICT);
		}  else temp = 1;
			*/

      // Capping exp at 1 notch per quest.
			temp = gp->number;
      if( temp > new_exp_table[GET_LEVEL(pl)+1] / 10 )
        temp = new_exp_table[GET_LEVEL(pl)+1] / 10;
      gain_exp(pl, NULL, temp, EXP_QUEST);
      send_to_char("You gain some experience.\n", pl);
      statuslog(58, "%s was rewarded %d experience by %s", GET_NAME(pl), temp, mob->player.short_descr );
      sql_log(pl, QUESTLOG, "Rewarded %d experience by %s", temp, mob->player.short_descr );

      if( pl->group )
      {
        for( struct group_list *gl = pl->group; gl; gl = gl->next )
        {
          if( gl->ch == pl )
            continue;
          if( !IS_PC(gl->ch) )
            continue;
          if( gl->ch->in_room == pl->in_room)
          {
            temp = gp->number;
            if( temp > new_exp_table[GET_LEVEL(gl->ch)+1] / 10 )
              temp = new_exp_table[GET_LEVEL(gl->ch)+1] / 10;
      			temp = gp->number;
            if( temp > new_exp_table[GET_LEVEL(gl->ch)+1] )
              temp = new_exp_table[GET_LEVEL(gl->ch)+1];
            gain_exp(gl->ch, NULL, temp, EXP_QUEST);
            send_to_char("You gain some experience.\n", gl->ch);
            statuslog(58, "%s was rewarded %d experience by %s", GET_NAME(gl->ch), temp, mob->player.short_descr );
            sql_log(gl->ch, QUESTLOG, "Rewarded %d experience by %s", gp->number/ temp, mob->player.short_descr );
          }          
        }
        
      }
      
      //sql_quest_finish(pl, mob, 3, gp->number / temp);
      break;
    }
  }
}

void tell_quest(int id, P_char pl)
{
  struct quest_msg_data *qmp;
  struct quest_complete_data *qcp;
  struct goal_data *gp;
  char     Gbuf2[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  P_obj    obj;

  send_to_char("My key words are:\n", pl);
  for (qmp = quest_index[id].quest_message; qmp; qmp = qmp->next)
  {
    snprintf(Gbuf2, MAX_STRING_LENGTH, "+ Key: %s\n", qmp->key_words);
    send_to_char(Gbuf2, pl);
  }
  for (qcp = quest_index[id].quest_complete; qcp; qcp = qcp->next)
  {
    send_to_char("I'm looking for:\n", pl);
    for (gp = qcp->give; gp; gp = gp->next)
    {
      switch (gp->goal_type)
      {
      case QUEST_GOAL_ITEM:
        obj = read_object(gp->number, VIRTUAL);
        if (obj && real_object(gp->number) > 0)
        {
          snprintf(Gbuf2, MAX_STRING_LENGTH, "  - item #%d %s\n", gp->number, obj->short_description);
          extract_obj(obj);
        }
        else
        {
          logit(LOG_DEBUG, "tell_quest(): obj %d not loadable", gp->number);
          snprintf(Gbuf2, MAX_STRING_LENGTH, "  - ?????\n");
        }
        send_to_char(Gbuf2, pl);
        break;
      case QUEST_GOAL_ITEM_TYPE:
        sprinttype(gp->number, item_types, buf);
        snprintf(Gbuf2, MAX_STRING_LENGTH, "  - item type %d - %s\n", gp->number, buf);
        send_to_char(Gbuf2, pl);
        break;
      case QUEST_GOAL_COINS:
        snprintf(Gbuf2, MAX_STRING_LENGTH, "  - %s\n", coin_stringv(gp->number));
        send_to_char(Gbuf2, pl);
        break;
      default:
        send_to_char("  - Don't know\n", pl);
      }
    }
    send_to_char("And I will reward:\n", pl);
    for (gp = qcp->receive; gp; gp = gp->next)
    {
      switch (gp->goal_type)
      {
      case QUEST_GOAL_ITEM:
        obj = read_object(gp->number, VIRTUAL);
        if (obj && real_object(gp->number) > 0)
        {
          snprintf(Gbuf2, MAX_STRING_LENGTH, "  - item #%d %s\n", gp->number,
                  obj->short_description);
          extract_obj(obj);
        }
        else
        {
          logit(LOG_DEBUG, "tell_quest(): obj %d not loadable", gp->number);
          snprintf(Gbuf2, MAX_STRING_LENGTH, "  - ?????\n");
        }
        send_to_char(Gbuf2, pl);
        break;
      case QUEST_GOAL_COINS:
        snprintf(Gbuf2, MAX_STRING_LENGTH, "  - %s\n", coin_stringv(gp->number));
        send_to_char(Gbuf2, pl);
        break;
      case QUEST_GOAL_SKILL:
        snprintf(Gbuf2, MAX_STRING_LENGTH, "  - %s\n", skills[gp->number].name);
        send_to_char(Gbuf2, pl);
        break;
      case QUEST_GOAL_EXP:
        if (gp->number > 0)
          snprintf(Gbuf2, MAX_STRING_LENGTH, "  - %d Experience points\n", gp->number);
        else
          snprintf(Gbuf2, MAX_STRING_LENGTH, "  - ?????\n");
        send_to_char(Gbuf2, pl);
        break;
      default:
        send_to_char("  - Don't know\n", pl);
      }
    }
    if (qcp->disappear)
      send_to_char("  I will disappear after this quest is finished.\n", pl);
  }
}

int quester(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   vict;
  char     name[MAX_INPUT_LENGTH];
  int      quester_id;
  struct quest_msg_data *qmp;
  struct quest_complete_data *qcp;
  char     Gbuf1[MAX_STRING_LENGTH], *temparg;

  /* ask/tell "key word" or give "item" */
  if ((cmd != CMD_ASK) && (cmd != CMD_TELL) && (cmd != CMD_GIVE))
    return (FALSE);

  /* if quester can't see player */
  if( ((!CAN_SEE(ch, pl) || !CAN_SEE(pl, ch)) && (GET_LEVEL(pl) < MINLVLIMMORTAL))
    || IS_FIGHTING(ch) || IS_DESTROYING(ch) || (GET_STAT(ch) <= STAT_SLEEPING) )
    return (FALSE);

  if(affected_by_spell(ch, TAG_CONJURED_PET) || affected_by_spell(pl, TAG_CONJURED_PET))
    return (FALSE);

  if (cmd == CMD_ASK || cmd == CMD_TELL)
  {                             /* player asked about quest */
    half_chop(arg, name, Gbuf1);
    if( !*name || !*Gbuf1
      || (!(vict = get_char_room_vis(pl, name)) && (GET_LEVEL(pl) < MINLVLIMMORTAL)) || (vict != ch) )
      return (FALSE);
    if( (quester_id = find_quester_id( GET_RNUM(ch) )) < 0 )
      return (FALSE);

    if ((cmd == CMD_TELL) && IS_TRUSTED(pl) && !strn_cmp(Gbuf1, "quest", 5))
    {
      tell_quest(quester_id, pl);
      return (TRUE);
    }
    for( qmp = quest_index[quester_id].quest_message; qmp; qmp = qmp->next)
    {
      if (qmp->key_words && isname(Gbuf1, qmp->key_words))
      {
/*	send_to_char(qmp->message, pl);*/
        act("$n asks $N a question.", FALSE, pl, 0, ch, TO_NOTVICT);
        act(qmp->message, FALSE, ch, 0, pl, qmp->echoAll ? TO_ROOM : TO_VICT);
        if (qmp->echoAll)
          send_to_room("\n", ch->in_room);
        else
          send_to_char("\n", pl);

        return (TRUE);
      }
    }
    return (FALSE);
  }
  if (cmd == CMD_GIVE)
  {
    /* This next chunk of code is to deal with the case where someone's
     * giving the quest mob money.  Need to check a different argument to
     * get the name.
     * -- DTS 2/28/95
     */
    for (temparg = arg; isspace(*temparg); temparg++)
      ; /* skip whitespaces */
    if (isdigit(*temparg))
      temparg = one_argument(arg, name);

    temparg = one_argument(temparg, name);
    one_argument(temparg, name);
    if (!(vict = get_char_room_vis(pl, name)) || vict != ch)
      return (FALSE);
    do_give(pl, arg, -4);       /* give item to mob */
    if ((quester_id = find_quester_id(GET_RNUM(ch))) < 0)
      return (TRUE);

    for (qcp = quest_index[quester_id].quest_complete; qcp; qcp = qcp->next)
    {
      if (quest_completion(qcp, ch, pl))
      {
        give_reward(qcp, ch, pl);
        if (qcp->disappear)
        {                       /* mob disappear after this quest */
/*** Lets try this: Rather than erase quest mob, move him to void. This
      keeps them in game, so they dont repop every friggin update
 ***/
//        send_to_char(qcp->disappear_message, pl);
          act(qcp->disappear_message, FALSE, ch, 0, pl, qcp->echoAll ? TO_ROOM : TO_VICT);

          P_obj obj;

          for (int l = 0; l < MAX_WEAR; l++)
            if (ch->equipment[l])
            {
              obj = unequip_char(ch, l);
              extract_obj(obj, TRUE); // Quest mob with an arti?
            }

          if (ch->carrying)
          {
            P_obj    next_obj;

            for (obj = ch->carrying; obj != NULL; obj = next_obj)
            {
              next_obj = obj->next_content;
              extract_obj(obj, TRUE); // Quest mob with an arti?
            }
          }
          extract_char(ch);
        }
        return (TRUE);
      }
    }
    return (TRUE);
  }
  return (TRUE);
}

int quest_sort_comp(const void *va, const void *vb)
{
  struct quest_data *a, *b;

  a = (struct quest_data *) va;
  b = (struct quest_data *) vb;

  if (a->quester > b->quester)
    return 1;
  else if (a->quester == b->quester)
    return 0;
  else
    return -1;
}

void quick_sort_quest_index(int min, int max)
{
  int      q;

  qsort(quest_index, number_of_quests, sizeof(struct quest_data),
        quest_sort_comp);
}

void boot_the_quests(void)
{
  int      temp;
  char     tbuf, letterStrn[256], letter;
  FILE    *quest_f;
  struct quest_msg_data *qmp;
  struct quest_complete_data *qcp;
  struct goal_data *gp;
  char filename[MAX_STRING_LENGTH];

  if( mini_mode == 1 ) {
    strcpy(filename, MINI_QUEST_FILE);
  }
  else
  {
    strcpy(filename, QUEST_FILE);
  }

  if (!(quest_f = fopen(filename, "r")))
  {
    perror("Error in boot quest\n");
    raise(SIGSEGV);
  }
  quest_index[number_of_quests].quest_message = 0;
  quest_index[number_of_quests].quest_complete = 0;

  for (number_of_quests = 0; number_of_quests < MAX_QUESTS;
       number_of_quests++)
  {
    tbuf = 0;
    temp = 0;
    fscanf(quest_f, "%c%d \n", &tbuf, &temp);
    if (tbuf == '#')
    {                           /* a new quest */

      quest_index[number_of_quests].quester = real_mobile(temp);
      if (temp <= 0)
      {
        perror("Error in boot quest: mob id must be greater than 0\n");
        raise(SIGSEGV);
      }
      if (quest_index[number_of_quests].quester < 0)
      {
        fprintf(stderr, "Error in boot quest:  real_mobile(%d) = %d\n",
                temp, quest_index[number_of_quests].quester);
        fprintf(stderr, "Continuing anyway, against my better judgement.\n");
        /*
           perror("Error in boot quest: mob does not exist.\n");
           raise(SIGSEGV);
         */
      }
//      letter = 0;
      fscanf(quest_f, " %s \n", letterStrn);
      while (letterStrn[0] != 'S')
      {
        switch (letterStrn[0])
        {
        case 'M':
          CREATE(qmp, quest_msg_data, 1, MEM_TAG_QSTMSG);

          qmp->key_words = fread_string(quest_f);
          qmp->message = fread_string(quest_f);
          if (qmp->message && qmp->message[strlen(qmp->message)-1] == '\n')
            qmp->message[strlen(qmp->message)-1] = '\0';
          qmp->echoAll = (letterStrn[1] == 'A');
          qmp->next = quest_index[number_of_quests].quest_message;
          quest_index[number_of_quests].quest_message = qmp;
          break;
        case 'Q':
          CREATE(qcp, quest_complete_data, 1, MEM_TAG_QSTCOMP);

          qcp->message = fread_string(quest_f);
          qcp->receive = 0;
          qcp->give = 0;
          qcp->echoAll = (letterStrn[1] == 'A');
          qcp->disappear = 0;
          qcp->disappear_message = 0;
          qcp->next = quest_index[number_of_quests].quest_complete;
          quest_index[number_of_quests].quest_complete = qcp;
          break;
        case 'D':
          if (quest_index[number_of_quests].quest_complete)
          {
            quest_index[number_of_quests].quest_complete->disappear = TRUE;
            quest_index[number_of_quests].quest_complete->disappear_message =
              fread_string(quest_f);
          }
          else
          {
            logit(LOG_EXIT, "Error in boot quest: quester %d.\n",
                  mob_index[quest_index[number_of_quests].quester].
                  virtual_number);
            raise(SIGSEGV);
          }
          break;
        case 'G':
          if (quest_index[number_of_quests].quest_complete)
          {
            CREATE(gp, goal_data, 1, MEM_TAG_QSTGOAL);

            fscanf(quest_f, " %c \n", &letter);
            switch (letter)
            {
            case 'I':
              gp->goal_type = QUEST_GOAL_ITEM;
              break;
            case 'T':
              gp->goal_type = QUEST_GOAL_ITEM_TYPE;
              break;
            case 'C':
              gp->goal_type = QUEST_GOAL_COINS;
              break;
            default:
              logit(LOG_EXIT, "Error in boot quest: quester %d.\n",
                    mob_index[quest_index[number_of_quests].quester].
                    virtual_number);
              raise(SIGSEGV);
            }
            gp->number = 0;
            fscanf(quest_f, " %d \n", &(gp->number));
            gp->next = quest_index[number_of_quests].quest_complete->give;
            quest_index[number_of_quests].quest_complete->give = gp;
          }
          else
          {
            logit(LOG_EXIT, "Error in boot quest: quester %d.\n",
                  mob_index[quest_index[number_of_quests].quester].
                  virtual_number);
            raise(SIGSEGV);
          }
          break;
        case 'R':
          if (quest_index[number_of_quests].quest_complete)
          {
            CREATE(gp, goal_data, 1, MEM_TAG_QSTGOAL);

            fscanf(quest_f, " %c \n", &letter);
            switch (letter)
            {
            case 'I':
              gp->goal_type = QUEST_GOAL_ITEM;
              break;
            case 'C':
              gp->goal_type = QUEST_GOAL_COINS;
              break;
            case 'S':
              gp->goal_type = QUEST_GOAL_SKILL;
              break;
            case 'E':
              gp->goal_type = QUEST_GOAL_EXP;
              break;
            default:
              logit(LOG_EXIT, "Error in boot quest: quester %d.\n",
                    mob_index[quest_index[number_of_quests].quester].
                    virtual_number);
              raise(SIGSEGV);
            }
            gp->number = 0;
            fscanf(quest_f, " %d \n", &(gp->number));
            if (gp->goal_type == QUEST_GOAL_SKILL && gp->number >= MAX_SKILLS)
            {
              gp->number = 0;
              gp->goal_type = QUEST_GOAL_UNKNOWN;
            }
            gp->next = quest_index[number_of_quests].quest_complete->receive;
            quest_index[number_of_quests].quest_complete->receive = gp;
          }
          else
          {
            logit(LOG_EXIT, "Error in boot quest: quester %d.\n",
                  mob_index[quest_index[number_of_quests].quester].
                  virtual_number);
            raise(SIGSEGV);
          }
          break;
        default:
          logit(LOG_EXIT, "Error in boot quest: quester %d, letterStrn=%s.\n",
                mob_index[quest_index[number_of_quests].quester].
                virtual_number, letterStrn);
          raise(SIGSEGV);
        }
        if (!fscanf(quest_f, " %s \n", letterStrn))
        {
          logit(LOG_EXIT, "Error in boot quest: quester %d.\n",
                mob_index[quest_index[number_of_quests].quester].
                virtual_number);
          raise(SIGSEGV);
        }
      }
    }
    else if (tbuf == '$')
    {
      break;
    }
    else
    {
      logit(LOG_EXIT, "Error in boot quest: quester #%d.\n",
            number_of_quests);
      raise(SIGSEGV);
    }
  }
  fclose(quest_f);

  if (number_of_quests == MAX_QUESTS)
  {
    logit(LOG_EXIT, "\t\tMaximum of %d quests exceeded\n", MAX_QUESTS);
    raise(SIGSEGV);
  }
  fprintf(stderr, "\t\t%d quests allocated\n", number_of_quests);

  if (number_of_quests > 0)
    quick_sort_quest_index(0, number_of_quests - 1);
}

void assign_the_questers(void)
{
  int      count;

  for (count = 0; count < number_of_quests; count++)
  {
    mob_index[quest_index[count].quester].qst_func = quester;
    /*
       fprintf(stderr, "Assigning: mob# %d\n", quest_index[count].quester);
     */
  }
}

#define QUEST_FILE_TROPHY "areas/quest.trophy"
#define TEMP_QUEST_FILE_TROPHY "areas/temp_quest.trophy"


int addQuestTropy(int questID)
{
  int      found = 0;
  int      t_id = 0;
  int      t_trophy = 0;
  int      i = 0;
  FILE    *f;
  FILE    *temp_f;
  char     sys[200];

  f = fopen(QUEST_FILE_TROPHY, "r");
  temp_f = fopen(TEMP_QUEST_FILE_TROPHY, "w+");

  if (!f || !temp_f)
  {
    wizlog(56, "Cant open file %s", QUEST_FILE_TROPHY);
    return 0;
  }


  while (!(feof(f)))
  {

    fscanf(f, "%d %d\n", &t_id, &t_trophy);

    if (t_id == questID)
    {
      found = 1;
      t_trophy++;
    }
    fprintf(temp_f, "%d %d\n", t_id, t_trophy);
  }

  if (!found)
    fprintf(temp_f, "%d %d\n", questID, 0);


  fclose(f);
  fclose(temp_f);
  snprintf(sys, 200, "cp %s %s", TEMP_QUEST_FILE_TROPHY, QUEST_FILE_TROPHY);
  system(sys);

  return 0;
}

float getQuestTropy(int questID)
{
  int      t_id = 0;
  int      t_trophy = 0;
  int      total_trophy = 0;
  int      trophy = 0;
  FILE    *f;


  f = fopen(QUEST_FILE_TROPHY, "r");

  if (!f)
  {
    wizlog(56, "Cant open file %s", QUEST_FILE_TROPHY);
    return 0;
  }

  while (!(feof(f)))
  {

    fscanf(f, "%d %d\n", &t_id, &t_trophy);

    if (t_id == questID)
      trophy = t_trophy;

    total_trophy = total_trophy + t_trophy;        //total trophy

  }
  fclose(f);

  return (float) ((float) (trophy * 1.00) / (float) (total_trophy * 1.00));
}

bool has_quest(P_char ch)
{
  int qi;

  if( !IS_NPC(ch) )
    return FALSE;

  qi = find_quester_id(GET_RNUM(ch));

  if( qi < 0 )
  {
    return FALSE;
  }

  if( quest_index[qi].quest_complete || quest_index[qi].quest_message )
    return TRUE;

  return FALSE;
}

bool has_quest_ask( int qi )
{
  int arg;
  struct quest_msg_data *qdata;

  for( qdata = quest_index[qi].quest_message; qdata; qdata = qdata->next )
  {
    // Skip the room messages on timers and the unblock messages.
    if( (sscanf( qdata->key_words, QC_ACTION " %d", &arg ) == 1)
    || is_abbrev(QC_UNBLOCK " ", qdata->key_words) )
    {
      continue;
    }
    else
      return TRUE;
  }
  return FALSE;
}

bool has_quest_complete( int qi )
{
  if( quest_index[qi].quest_complete )
    return TRUE;

  return FALSE;
}

#if 0
void delete_quests()
{
  int      count;

  for (count = 0; count < number_of_quests; count++)
  {
    mob_index[quest_index[count].quester].func.mob = 0;
    if (quest_index[count].key_word1)
      L_free_string(quest_index[count].key_word1);
    if (quest_index[count].key_word2)
      L_free_string(quest_index[count].key_word2);
    if (quest_index[count].quest_message1)
      L_free_string(quest_index[count].quest_message1);

    if (quest_index[count].key_word3)
      L_free_string(quest_index[count].key_word3);
    if (quest_index[count].key_word4)
      L_free_string(quest_index[count].key_word4);
    if (quest_index[count].quest_message2)
      L_free_string(quest_index[count].quest_message2);

    if (quest_index[count].complete_message)
      L_free_string(quest_index[count].complete_message[0]);

    if (quest_index[count].complete_message)
      L_free_string(quest_index[count].complete_message[1]);
  }
}

void do_reload_quest(P_char ch, char *arg, int cmd)
{
  P_char   i;

  snprintf(Gbuf2, MAX_STRING_LENGTH, "Reload the quests by %s.", ch->player.name);
  logit(LOG_WIZ, Gbuf2);
  for (i = character_list; i; i = i->next)
    if (IS_PC(i))
      send_to_char("Reloading quests...\n", i);
  delete_quests();
  assign_mobiles();             /* reassign mobiles with prev spec procs */
  send_to_char("Done.\n", ch);
}

#endif
