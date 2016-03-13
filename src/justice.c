/******************************************************/
/* Justice.c                                          */
/******************************************************/


#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "justice.h"
#include "events.h"
#include "spells.h"
#include "graph.h"
#include "mm.h"
#include "interp.h"
#include "sound.h"

extern P_room world;
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern int mini_mode;
extern const char *race_types[];
extern P_char character_list;
extern P_event current_event;
extern P_index mob_index;
extern P_event event_type_list[LAST_EVENT];
extern P_desc descriptor_list;
extern const char *town_name_list[];
extern int rev_dir[];

void     check_item(P_char);

/* number of seconds after a person is outcast that they can be
   pardoned for it */

#define PARDON_TIME_LIMIT (60 * 10)

P_obj    justice_items_list = NULL;

/* This list MUST correspond to the CRIME_ macros in justice.h!! */
const char *crime_list[CRIME_NB] = {
  "<NONE>",
  "Attempted Theft",
  "Theft",
  "Attempted Murder",
  "Escape",
  "Murder",
  "Casting wall",
  "Kidnapping",
  "Shape changing",
  "No payment of debt",
  "Crime against town",
  "Corpse looting",
  "Corpse dragging",
  "Disguising",
  "Summoning Creatures"
};

/* This list is use for reporting crimes */
const char *crime_rep[CRIME_NB] = {
  "1xxxxxxxx1",
  "Att_Theft",
  "Theft",
  "Att_Murder",
  "Escape",
  "Murder",
  "Cast_wall",
  "Kidnapping",
  "Shape_change",
  "Not_paid",
  "Crime_against_town",
  "Corpse_loot",
  "Corpse_drag",
  "Disguise",
  "Summoning"
};

const char *justice_status[] = {
  "NONE",
  "Wanted",
  "Debt",
  "Pardon",
  "Crime",
  "**False report",
  "In jail",
  "Jail time",
  "**Arraigned",
  "DELETED"
};

struct hometown_data hometowns[LAST_HOME] = {
  /* flags, report_room, guard_room[5], guard_mob, jail_room, NULL,
     t_crime_punish[CRIME_NB], p_crime_punish[CRIME_NB],
     sentence_min[SENTENCE_NB], sentence_max[SENTENCE_NB]
   */

  /* 1. Tharnadia */
  {JUSTICE_GOODHOME, 132618,
   {133050, 132628, 133057, 133065, 132504}, 132546, 132619, NULL,
   {0, 2, 5, 10, 15, 20, 10, 10, 5, 12, 20, 2, 2, 8, 8},
   {0, 0, 0, 10, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 7, 12, 17, 22, 27},
   {6, 11, 16, 21, 26, 9999}},

  /* 2. iliithid  town */
  {JUSTICE_NEUTRALHOME, 0,
   {96421, 0, 0, 0, 0}, 96421, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

  /* 3. drow town */
  {JUSTICE_EVILHOME, 0,
   {36572, 36563, 36544, 0, 0}, 36436, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

  /* 4. elftown */
  {JUSTICE_GOODHOME, 8096,
   {8096, 8087, 8214, 0, 0}, 8025, 8097, NULL,
   {0, 2, 5, 10, 15, 20, 10, 10, 5, 12, 20, 2, 2, 8, 6},
   {0, 0, 0, 10, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 7, 12, 17, 22, 27},
   {6, 11, 16, 21, 26, 9999}},

  /* 5. dwarf town */
  {JUSTICE_GOODHOME, 95643,
   {95540, 95522, 95650, 0, 0}, 95532, 95644, NULL,
   {0, 2, 5, 10, 15, 20, 10, 10, 5, 12, 20, 2, 2, 8, 10},
   {0, 0, 0, 10, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 7, 12, 17, 22, 27},
   {6, 11, 16, 21, 26, 9999}},

  /* 6. duergar */
  {JUSTICE_EVILHOME, 0,
   {17173, 0, 0, 0, 0}, 17173, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

  /* 7. halfling */
  {JUSTICE_GOODHOME, 16512,
   {16506, 16522, 16526, 16653, 16551}, 16705, 16513, NULL,
   {0, 2, 5, 10, 15, 20, 10, 10, 5, 12, 20, 2, 2, 5, 0},
   {0, 0, 0, 10, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 7, 12, 17, 22, 27},
   {6, 11, 16, 21, 26, 9999}},

  /* 8. gnome town */
  {JUSTICE_GOODHOME, 66069,
   {66091, 0, 0, 0, 0}, 66001, 66074, NULL,
   {0, 2, 5, 10, 15, 20, 10, 10, 5, 12, 20, 2, 2, 8, 6},
   {0, 0, 0, 10, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 7, 12, 17, 22, 27},
   {6, 11, 16, 21, 26, 9999}},

  /* 9. faange (ogres) */
  {JUSTICE_EVILHOME, 0,
   {15231, 0, 0, 0, 0}, 15231, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

  /* 10. ghore (trolls) */
  {JUSTICE_EVILHOME, 0,
   {11503, 0, 0, 0, 0}, 11503, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

  /* 11. "ugta"  (barb) */
  {JUSTICE_GOODHOME, 39310,
   {39100, 39109, 39166, 39310, 0}, 39105, 39341, NULL,
   {0, 2, 5, 10, 15, 20, 10, 10, 5, 12, 20, 2, 2, 8, 2},
   {0, 0, 0, 10, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 7, 12, 17, 22, 27},
   {6, 11, 16, 21, 26, 9999}},

  /* 12. bloodstone... */
  {JUSTICE_LEVEL_CHAOS, 0,
   {0, 0, 0, 0, 0}, 0, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

  /* 13. "shady"... orc town */
  {JUSTICE_EVILHOME, 0,
   {97682, 97569, 97682, 97607, 97612}, 97562, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

  /* 14 "nax" - mino hometown */
  {JUSTICE_LEVEL_CHAOS, 0,
   {37701, 0, 0, 0, 0}, 37701, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

  /* 15 "fort marigot" (centaur) */
  {JUSTICE_GOODHOME, 5307,
   {5319, 5306, 5371, 0, 0}, 5340, 5308, NULL,
   {0, 2, 5, 10, 15, 20, 10, 10, 5, 12, 20, 2, 2, 8, 8},
   {0, 0, 0, 10, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 7, 12, 17, 22, 27},
   {6, 11, 16, 21, 26, 9999}},

  /* 16  new elftown */
  {JUSTICE_GOODHOME, 45006,
   {45189, 45205, 45152, 45153, 45036}, 45030, 45036, NULL,
   {0, 2, 5, 10, 15, 20, 10, 10, 5, 12, 20, 2, 2, 8, 8},
   {0, 0, 0, 10, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 7, 12, 17, 22, 27},
   {6, 11, 16, 21, 26, 9999}},

//  17 "Ancient City Ruins" (good undead)
  {JUSTICE_LEVEL_CHAOS, 0,
   {66201, 0, 0, 0, 0}, 66201, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

//  18 "payang" (evil undead)
  {JUSTICE_LEVEL_CHAOS, 0,
   {90341, 0, 0, 0, 0}, 90313, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

//  19 "Githyanki Fortress",
  {JUSTICE_EVILHOME, 0,
   {19400, 0, 0, 0, 0}, 19400, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

//  20 "Moregeeth",
  {JUSTICE_EVILHOME, 0,
   {70187, 0, 0, 0, 0}, 70044, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},

//  "Harpy",
  {JUSTICE_LEVEL_CHAOS, 0,
   {0, 0, 0, 0, 0}, 0, 0, NULL,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}},
};

struct mm_ds *dead_justice_guard_pool = NULL;
struct mm_ds *dead_witness_pool = NULL;
struct mm_ds *dead_crime_pool = NULL;
struct justice_guard_list *guard_list = NULL;

const char *justice_flags[] = {
  "Evil",                       /* JUSTICE_EVILHOME */
  "Good",                       /* JUSTICE_GOODHOME */
  "Harsh",                      /* JUSTICE_LEVEL_HARSH */
  "Chaotic",                    /* JUSTICE_LEVEL_CHAOS */
  "Undead",
  "Neutral",
  "\n"
};

const char *justice_flag_names[] = {
  "citizen",                    /* JUSTICE_IS_CITIZEN */
  "normal",                     /* JUSTICE_IS_NORMAL */
  "citizen_buy",                /* JUSTICE_IS_CITIZEN_BUY */
  "outcast"                     /* JUSTICE_IS_OUTCAST */
};

const char *justice_param[] = {
  "pardon",
  "reporting",
  "pay",                        /* for paying money for crime you cimmited */
  "list",                       /* list of criminals wanted by town justice */
  "turn_in",
  "\n"
};


/* Users interface to the justice system! */
void do_justice(P_char ch, char *arg, int cmd)
{
  P_char   vict;
  char     arg1[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  char     buf1[MAX_INPUT_LENGTH];
  int      i, crime_ok = FALSE, town, town1, in_jail = FALSE;
  P_obj    o_obj = NULL;
  crm_rec *crec = NULL;

  arg = one_argument(arg, arg1);

  if (*arg1)
  {
    if (!str_cmp(arg1, "item") && IS_TRUSTED(ch))
    {
/*      if (!justice_items_list) {
        send_to_char( "No justice items!\r\n", ch);
      } else {
        for (o_obj = justice_items_list; o_obj; o_obj = o_obj->next_content) {
          sprintf(buf1, "%s belong to %s\r\n", o_obj->short_description, o_obj->justice_name);
          send_to_char(buf1, ch);
        }
      }*/
    }
    else if (!str_cmp(arg1, "info"))
    {
      sprintf(buf1, "Justice info for %s.\r\n", J_NAME(ch));

      for (town = 1; town <= LAST_HOME; town++)
      {
        if (!hometowns[town - 1].crime_list)
          continue;
        sprintf(buf1, "%s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n",
                buf1);
        sprintf(buf1, "%sCrime for %s\r\n", buf1, town_name_list[town]);
        while ((crec =
                crime_find(hometowns[town - 1].crime_list, J_NAME(ch), NULL,
                           0, NOWHERE, J_STATUS_NONE, crec)))
        {
          sprintf(buf1, "%s  %s against %s, status &+c%s&n.\r\n",
                  buf1, crime_list[crec->crime], crec->victim,
                  justice_status[crec->status]);
          if (crec->status == J_STATUS_JAIL_TIME)
          {
            in_jail = TRUE;
            town1 = town;
          }
        }
      }
      sprintf(buf1, "%s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n",
              buf1);
      sprintf(buf1, "%sNumber of time judged : &+R%d&N.\r\n", buf1,
              GET_TIME_JUDGE(ch));
      sprintf(buf1, "%s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n",
              buf1);
      if (in_jail)
      {
        sprintf(buf1, "%s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n",
                buf1);
        crec = NULL;
        crec = crime_find(hometowns[town1 - 1].crime_list, J_NAME(ch), NULL,
                          0, NOWHERE, J_STATUS_JAIL_TIME, NULL);
        if (crec)
        {
          sprintf(buf1, "%sYou are in jail (%d hours left).\r\n",
                  buf1, (int) ((crec->time - time(NULL)) / 75));
        }
        sprintf(buf1, "%s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n",
                buf1);
      }
      send_to_char(buf1, ch);

    }
    else if ((vict = get_char(arg1)) && IS_TRUSTED(ch))
    {
      if (restoreJailItems(vict))
        send_to_char("Ok items restored!\r\n", ch);
      else
        send_to_char("Problem with restoring file!\r\n", ch);
    }
    else if (IS_TRUSTED(ch))
      send_to_char
        ("Justice <name of player>, to restore jail items or Justice item, to list justice items.\r\n",
         ch);
    else
      send_to_char("Justice or Justice info.\r\n", ch);
  }
  else
  {
    sprintf(buf1, "List of crimes:\r\n");

    if (CHAR_IN_JUSTICE_AREA(ch))
    {
      sprintf(buf, "&+R---JUSTICE---&N\r\n");
      for (i = 0; i < CRIME_NB; i++)
      {
        if (GET_CRIME_P(CHAR_IN_JUSTICE_AREA(ch), i))
        {
          sprintf(buf1, "%s %s\r\n", buf1, crime_list[i]);
          crime_ok = TRUE;
        }
      }
      if (crime_ok)
      {
        sprintf(buf, "%sThis area is controlled by justice.\r\n", buf);
        sprintf(buf, "%s%s&+R-------------&N\r\n", buf, buf1);
      }

    }
    else if (CHAR_IN_TOWN(ch))
    {
      sprintf(buf, "&+R---JUSTICE---&N\r\n");
      for (i = 0; i < CRIME_NB; i++)
      {
        if (GET_CRIME_T(CHAR_IN_TOWN(ch), i))
        {
          sprintf(buf1, "%s %s\r\n", buf1, crime_list[i]);
          crime_ok = TRUE;
        }
      }
      if (crime_ok)
      {
        sprintf(buf, "%sThis town is controlled by justice.\r\n", buf);
        sprintf(buf, "%s%s&+R-------------&N\r\n", buf, buf1);
      }

    }
    else
    {
      sprintf(buf, "&+CYou are not in an area controlled by justice.&N\r\n");
    }
    send_to_char(buf, ch);
    return;
  }
}


/* function to send guard after a crime report */

void justice_dispatch_guard(int town, char *attacker, char *victim, int crime)
{
  P_char   tch;
  crm_rec *crec;

  if ((crec = crime_find(hometowns[town - 1].crime_list, attacker, victim,
                         crime, NOWHERE, J_STATUS_CRIME, NULL)))
  {
    if (crec->money == 1)
    {                           /* This if the first time */
      if ((tch = get_char(attacker)))
      {
        if ((town == CHAR_IN_TOWN(tch)) &&
            (tch->in_room != real_room(hometowns[town - 1].jail_room)))
          //justice_send_guards(NOWHERE, tch, MOB_SPEC_ARREST1, 1);
          justice_send_guards(NOWHERE, tch, MOB_SPEC_J_PK, 1);
        else
          crime_add(town, attacker, victim,
                    real_room(hometowns[town - 1].report_room), crime,
                    time(NULL), J_STATUS_WANTED, 200);
      }
      else
      {
        crime_add(town, attacker, victim,
                  real_room(hometowns[town - 1].report_room), crime,
                  time(NULL), J_STATUS_WANTED, 200);
      }
    }
  }
  return;
}


/* this function is only called as a replacement for do_move() in
   NewMobHunt for any HUNT_JUSTICE_SPEC* hunt type.  Unless the guard
   is dragging or something, this need only call do_move(); */

void JusticeGuardMove(P_char ch, char *argument, int cmd)
{
  P_char   vict = NULL;
  int      was_in;

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_ARREST2))
  {
    vict = ch->specials.arrest_by;

    was_in = ch->in_room;
    do_move(ch, NULL, cmd);
    if (ch->in_room == NOWHERE)
      return;
    if (vict)
    {
      if (vict->in_room == was_in)
      {                         /* ok vict did not escape */
        act("$n is dragged out of the room.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You are dragged along.\r\n", vict);
        char_from_room(vict);
        char_to_room(vict, ch->in_room, -1);
        act("$n drags $N behind $m.", TRUE, ch, 0, vict, TO_NOTVICT);
      }
    }

  }
  else
    do_move(ch, NULL, cmd);

  return;
}


/* designed to be called from NewMobAct().. returns TRUE if I acted */

int JusticeGuardAct(P_char ch)
{
  hunt_data data;
  P_char   tch, nextch;

#if 1
return FALSE;
#endif

  /* being all mobs check here anyway, its a good place townies to
     check for invaders */

  if (int ht = CHAR_IN_TOWN(ch))
  {
    for (tch = world[ch->in_room].people; tch; tch = nextch)
    {
      nextch = tch->next_in_room;
      if (tch != ch && IS_TOWN_INVADER(tch, ht) && !IS_AFFECTED(tch, AFF_BOUND))
        justice_action_invader(tch);
    }
  }

  if (IS_PC(ch) || (!IS_SET(ch->only.npc->spec[2], MOB_SPEC_JUSTICE)))
    return FALSE;

  /* small hook here to get more guards, if more guards are needed... */

  for (tch = world[ch->in_room].people; tch; tch = nextch)
  {
    nextch = tch->next_in_room;

    if (IS_INVADER(tch) || IS_OUTCAST(tch))
      justice_action_invader(tch);
  }

  if( IS_FIGHTING(ch) || IS_DESTROYING(ch) )
    return FALSE;

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_J_REMOVE))
    return FALSE;

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_J_OUTCAST))
  {
    if (!ch->specials.arrest_by)
    {                           /* wtf? */
      REMOVE_BIT(ch->only.npc->spec[2], MOB_SPEC_J_OUTCAST);
      /* but don't return!  let if fall though so it starts hunting
         for home! */
    }
    else
    {
      /* okay.. they have a target, and aren't fighting right
         now... make sure they are hunting their damn target!  Problem
         is that the hunter code will kill the hunting even if the mob
         fights... but if the player flees, I still want them to be
         hunted! */
      P_nevent  ev;

      LOOP_EVENTS_CH(ev, ch->nevents)
      {
        if (ev->func == mob_hunt_event) /* ah!  good..  */
          return FALSE;
      }
      /* hmm.. I'm targetting someone, but not hunting or
         fighting... fix it */
      data.hunt_type = HUNT_JUSTICE_INVADER;
      data.targ.victim = ch->specials.arrest_by;
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
      //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, ch, data);
      return TRUE;
    }
  }
  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_J_PK))
    return TRUE;

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_ARREST1))
    return TRUE;

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_ARREST2))
  {
    return TRUE;
  }

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_REPORTING))
  {
    return TRUE;
  }

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_GOING_BACK))
  {
    return TRUE;
  }

  /* PUT OTHER POSSIBLE CONDITIONS FOR spec[2] HERE... ALSO, Make damn
     sure that each of them returns either TRUE or FALSE... if you let
     them drop past this point, they will be sent HOME! */

  /* okay.  At this point, the guard doesn't have any reason for
     existing... so we send them the fuck home! */

  ch->only.npc->spec[2] = MOB_SPEC_J_REMOVE | MOB_SPEC_JUSTICE;

  data.hunt_type = HUNT_JUSTICE_SPECROOM;
  data.targ.room = real_room(GET_BIRTHPLACE(ch));
  add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
  //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, ch, data);
  return TRUE;
}

/* called by NewMobHunt whenever a HUNT_JUSTICE_SPEC* hunt type
   reaches its target room/victim */

void JusticeGuardHunt(P_char ch)
{
  crm_rec *crec;
  wtns_rec *rec = NULL;
  P_char   vict, tch;
  int      outcast_room, ret_room;
  hunt_data data;
  P_event  ev;
  char     buf[MAX_STRING_LENGTH];

  if (IS_PC(ch) || (!IS_SET(ch->only.npc->spec[2], MOB_SPEC_JUSTICE)))
    return;

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_J_REMOVE))
  {
    justice_delete_guard(ch);
    return;
  }
  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_J_PK))
  {

    crec = hometowns[CHAR_IN_TOWN(ch) - 1].crime_list;
    vict = ch->specials.arrest_by;

    /* eventually, this will bound the player, drag him around some,
       and THEN strip/kill him... but for now... lets just strip and
       kill them on the spot.  After I have the dragging stuff done,
       I'll make it fancier. */
    switch (number(1, 3))
    {
    case 1:
      outcast_room = real_room(175555);
      break;
    case 2:
      outcast_room = real_room(169162);
      break;
    case 3:
    default:
      outcast_room = real_room(148781);
      break;
    }
    if (outcast_room < 1)
      outcast_room = real_room(74444);

    mobsay(ch, "We don't want you stinking murderers here!!");
    act("$n utters the words 'power word outcast'.", TRUE, ch, 0,
        vict, TO_ROOM);
    act("$n disappears in a puff of smoke!", TRUE, vict, 0, 0, TO_ROOM);
    send_to_char("&+LYou suddenly feel very dizzy...\r\n", vict);
    stop_fighting(vict);
    if( IS_DESTROYING(vict) )
      stop_destroying(vict);
    char_from_room(vict);
    char_to_room(vict, outcast_room, -1);

    /* if vict is morphed, make sure the guards 'arrest_by' points
       to the real player.  That way, when justice_hunt_cancel is
       called for this guard, the player gets outcasted  */

    if (IS_MORPH(vict))
      ch->specials.arrest_by = MORPH_ORIG(vict);

    justice_hunt_cancel(ch);

    return;
  }

  /* ok guard is arresting the criminal */

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_ARREST1))
  {

    vict = ch->specials.arrest_by;

    sprintf(buf, "&+RStop!&N  %s, you're under &+RARREST!&N", J_NAME(vict));
    mobsay(ch, buf);
    if (PC_TOWN_JUSTICE_FLAGS(ch, CHAR_IN_TOWN(ch)) == JUSTICE_IS_NORMAL)
      sprintf(buf, "Tourists always cause problems around here.");
    mobsay(ch, buf);
    play_sound(SOUND_LAWPAY, NULL, ch->in_room, TO_ROOM);
    LOOP_THRU_PEOPLE(tch, ch) stop_fighting(tch);

    SET_BIT(vict->specials.affected_by, AFF_BOUND);
    act("$n chains $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    act("$n chains you.", TRUE, ch, 0, vict, TO_VICT);

    REMOVE_BIT(ch->only.npc->spec[2], MOB_SPEC_ARREST1);
    SET_BIT(ch->only.npc->spec[2], MOB_SPEC_ARREST2);

    data.hunt_type = HUNT_JUSTICE_SPECROOM;
    data.targ.room = real_room(hometowns[CHAR_IN_TOWN(ch) - 1].report_room);
    add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
    //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, ch, data);

    return;
  }

  /* guard had brought the criminal to justice, unless the criminal escape on his way here */

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_ARREST2))
  {

    vict = ch->specials.arrest_by;
    if (ch->in_room == vict->in_room)
    {
      char_from_room(vict);
      char_to_room(vict, real_room(hometowns[CHAR_IN_TOWN(ch) - 1].jail_room),
                   -1);
      act("$n throws $N in jail.", FALSE, ch, 0, vict, TO_NOTVICT);
      send_to_char("You've been thrown in jail.\r\n", vict);
      send_to_char("All your equipment has been removed.\r\n", vict);
      REMOVE_BIT(vict->specials.affected_by, AFF_BOUND);
      crime_add(CHAR_IN_TOWN(ch), J_NAME(vict), J_NAME(ch),
                real_room(hometowns[CHAR_IN_TOWN(ch) - 1].jail_room), 0,
                time(NULL), J_STATUS_IN_JAIL, 0);
//     check_item(vict);
      writeJailItems(vict);
      writeCharacter(vict, 7, vict->in_room);

    }
    else
    {
      crime_add(CHAR_IN_TOWN(ch), J_NAME(vict), J_NAME(ch),
                real_room(hometowns[CHAR_IN_TOWN(ch) - 1].report_room),
                CRIME_ESCAPE, time(NULL), J_STATUS_WANTED, 200);

    }

    justice_hunt_cancel(ch);
    return;
  }

  /* witness mob reporting a crime */

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_REPORTING))
  {

    while ((rec = witness_find(ch->specials.witnessed,
                               NULL, NULL, 0, NOWHERE, rec)))
    {
      crime_add(CHAR_IN_TOWN(ch), rec->attacker, rec->victim, rec->room,
                rec->crime, rec->time, J_STATUS_CRIME, 1);
      ret_room = rec->room;
      justice_dispatch_guard(CHAR_IN_TOWN(ch), rec->attacker, rec->victim,
                             rec->crime);
    }

    witness_destroy(ch);

    ch->only.npc->spec[2] = MOB_SPEC_JUSTICE | MOB_SPEC_GOING_BACK;

    /* we send the witness mob to where it was */

    if (GET_BIRTHPLACE(ch) > 0)
    {
      data.hunt_type = HUNT_JUSTICE_SPECROOM;
      data.targ.room = real_room(GET_BIRTHPLACE(ch));
      add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
      //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, ch, data);
    }
    else
    {
      logit(LOG_CRIMES, "%s witness a crime and got no return room",
            J_NAME(ch));
    }
    return;
  }

  /* witness mob is back to where it was before */

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_GOING_BACK))
  {
    ch->only.npc->spec[2] = 0;
    return;
  }

  /* finally, if no murder, and they aren't fighting.... just drop
     through */

  justice_hunt_cancel(ch);      /* for now, just treat it like a
                                   waylaid guard... only used right
                                   now for MOB_SPEC_J_PK, in which
                                   case, the person getting hunted
                                   will get outcast  */
  return;
}

void justice_set_outcast(P_char ch, int town)
{
  int      r_room, old_birth;
  char     buf[MAX_STRING_LENGTH];
  crm_rec *crec, *t;
  P_char   r_ch;

  if (!town || !ch)
    return;

  if (IS_NPC(ch))
    return;

  if (IS_MORPH(ch))
    ch = MORPH_ORIG(ch);

  /* you CAN'T be set outcast from bloodstone, unless a god manually
     does it! */
  if (town == HOME_BLOODSTONE)
    return;

  /* if 'ch' is morphed, have 'r_ch' point to the mob they are in */
  if (IS_PC(ch) && IS_SET(ch->specials.act, PLR_MORPH) &&
      ch->only.pc->switched)
    r_ch = ch->only.pc->switched;
  else
    r_ch = ch;

  /* find out their original birth room.  Need this so that we can put
     it back if they get outcasted */
  old_birth = GET_BIRTHPLACE(ch);
  /* hmm.. if they are already starting in the outcast room, something
     is up.  Lets check old records, and see if they have a "good"
     old_birth room... */
  if (old_birth == OUTCAST_BIRTH)
  {
    t = NULL;
    while ((t = crime_find(hometowns[town - 1].crime_list, J_NAME(ch), NULL,
                           CRIME_FAKE_OUTCAST, NOWHERE, J_STATUS_NONE, t)))
      if (t->room != OUTCAST_BIRTH)
        old_birth = t->room;

  }

  /* If we are going to outcast them, we can nuke all the witness
     records against them in town.  However, keep around the records
     that got them outcast, and convert to a new type.. */
  crec = hometowns[town - 1].crime_list;
  t = NULL;
  do
  {
    t = crime_find(hometowns[town - 1].crime_list, J_NAME(ch), NULL, 0,
                   NOWHERE, J_STATUS_NONE, t);
    if (t && (t->crime != CRIME_FAKE_OUTCAST))
      if (t->crime <= CRIME_LAST_NON_VIO)
      {
        crime_remove(town, t);
        t = NULL;
      }
      else
      {
        t->time = time(NULL);
        t->crime = CRIME_FAKE_OUTCAST;
        t->room = old_birth;
      }
  }
  while (t);


  /* if they are already outcast, don't do it again */
  if (PC_TOWN_JUSTICE_FLAGS(ch, town) == JUSTICE_IS_OUTCAST)
    return;

  PC_SET_TOWN_JUSTICE_FLAGS(ch, JUSTICE_IS_OUTCAST, town);

  /* adjust home/birth rooms... */

  r_room = real_room(GET_BIRTHPLACE(ch));
  if ((r_room != NOWHERE) &&
      (town == zone_table[world[r_room].zone].hometown))
  {

    if (PC_TOWN_JUSTICE_FLAGS(ch, HOME_BLOODSTONE) != JUSTICE_IS_OUTCAST)
      GET_BIRTHPLACE(ch) = OUTCAST_BIRTH;
    else
      GET_BIRTHPLACE(ch) = EVIL_RACE(ch) ? 4093 : 1757;
  }
  r_room = real_room(GET_HOME(ch));
  if ((r_room != NOWHERE) &&
      (town == zone_table[world[r_room].zone].hometown))
    GET_HOME(ch) = GET_BIRTHPLACE(ch);

  /* brag to the hometown about it */
  sprintf(buf, "&+WSomeone shouts '%s has been exiled from %s!'&n",
          J_NAME(ch), town_name_list[town]);
  justice_hometown_echo(town, buf);

  /* now let them (and the gods) know about it */

  sprintf(buf,
          "&+Y************************************************************&N\r\n"
          "                          &+R&-L WARNING!&N\r\n"
          "\r\n"
          "&+WThe government of %s has declared that you are&n\r\n"
          "&+Wnow an outcast.  This means that you are exiled from there,&n\r\n"
          "&+Wand will be killed should you ever be seen!!&n\r\n"
          "&+Y************************************************************&N\r\n",
          town_name_list[town]);
  if (GET_STAT(r_ch) != STAT_DEAD)
    writeCharacter(r_ch, 1, NOWHERE);
  else
    writeCharacter(r_ch, 4, NOWHERE);

  /* send it to them twice, just so they don't miss it */
  send_to_char(buf, r_ch);

  sprintf(buf, "%s declared OUTCAST from %s by justice code",
          J_NAME(ch), town_name_list[town]);

#if 0
  if ((GET_CLASS(ch) == CLASS_RANGER) || (GET_CLASS(ch) == CLASS_PALADIN))
  {
    GET_CLASS(ch) = CLASS_WARRIOR;
    logit(LOG_PLAYER, "%s fucked up!  Changed to Warrior", J_NAME(ch));
    send_to_char
      ("&+RYou suddenly feel your magic abilities fading from you!&N\r\n",
       ch);
  }
#endif
  wizlog(56, buf);
}


/* fonction to outcast a player after being judge */

void justice_sentence_outcast(P_char ch, int town)
{
  int      r_room;
  int      outcast_room;
  P_char   r_ch;

  if (!town || !ch)
    return;

  if (IS_NPC(ch))
    return;

  if (IS_MORPH(ch))
    ch = MORPH_ORIG(ch);

  /* you CAN'T be set outcast from bloodstone, unless a god manually
     does it! */
  if (town == HOME_BLOODSTONE)
    return;

  /* if 'ch' is morphed, have 'r_ch' point to the mob they are in */
  if (IS_PC(ch) && IS_SET(ch->specials.act, PLR_MORPH) &&
      ch->only.pc->switched)
    r_ch = ch->only.pc->switched;
  else
    r_ch = ch;

  /* if they are already outcast, don't do it again */
  if (PC_TOWN_JUSTICE_FLAGS(ch, town) == JUSTICE_IS_OUTCAST)
    return;

  PC_SET_TOWN_JUSTICE_FLAGS(ch, JUSTICE_IS_OUTCAST, town);

  /* adjust home/birth rooms... */

  r_room = real_room(GET_BIRTHPLACE(ch));
  if ((r_room != NOWHERE) &&
      (town == zone_table[world[r_room].zone].hometown))
  {

    if (PC_TOWN_JUSTICE_FLAGS(ch, HOME_BLOODSTONE) != JUSTICE_IS_OUTCAST)
      GET_BIRTHPLACE(ch) = OUTCAST_BIRTH;
    else
      GET_BIRTHPLACE(ch) = EVIL_RACE(ch) ? 4093 : 1757;
  }
  r_room = real_room(GET_HOME(ch));
  if ((r_room != NOWHERE) &&
      (town == zone_table[world[r_room].zone].hometown))
    GET_HOME(ch) = GET_BIRTHPLACE(ch);

  switch (number(1, 3))
  {
  case 1:
    outcast_room = real_room(175555);
    break;
  case 2:
    outcast_room = real_room(169162);
    break;
  case 3:
  default:
    outcast_room = real_room(148781);
    break;
  }
  if (outcast_room < 1)
    outcast_room = real_room(74444);

  stop_fighting(ch);
  if( IS_DESTROYING(ch) )
    stop_destroying(ch);
  char_from_room(ch);
  char_to_room(ch, outcast_room, -1);

  if (GET_STAT(r_ch) != STAT_DEAD)
    writeCharacter(r_ch, 1, NOWHERE);
  else
    writeCharacter(r_ch, 4, NOWHERE);

}


/* Function used to make sure 'how_many' justice guards are out
   performing the task dictated by 'type' (which should be a
   MOB_SPEC_* flag!!).  'victim' is who the task is performed against
   and 'to_rroom' is the real room number where the task is to be
   performed.  For some types, 'victim' is needed,  and for others,
   'to_rroom' is needed.  If victim is passed, and to_rroom is
   NOWHERE, the room number will be determined by the location of
   victim.

   return TRUE if any guards sent out... */

int justice_send_guards(int to_rroom, P_char victim, int type, int how_many)
{
  struct justice_guard_list *gl;
  int      best_dist = 9999;
  int      best_room = NOWHERE;
  int      i, town;
  int      hunt_type;
  hunt_data data;
  int ht = CHAR_IN_TOWN(victim);

  //WIPE2013 - Drannak
  return FALSE; //disabling

  // Only good hts get guards atm.
  if( ht <= 0 || ht > LAST_HOME
    || !IS_SET( hometowns[ht-1].flags, JUSTICE_GOODHOME ))
  {
    wizlog(56, "Justice: %s not in good ht %d; no guards dispensed.", GET_NAME(victim), ht );
    return FALSE;
  }

  if ((to_rroom == NOWHERE) && !victim)
    return FALSE;

  /* figure out how many are REALLY needed (ie: aren't already doing
     it) */
  for (gl = guard_list; gl; gl = gl->next)
  {

    /* justice guards that have the same victim as we'd set, and the
       same type flag that we'd set are counted as part of how_many */
    if ( /*gl->ch->only.npc && */ (GET_STAT(gl->ch) != STAT_DEAD) &&
        IS_NPC(gl->ch) &&
        IS_SET(gl->ch->only.npc->spec[2], MOB_SPEC_JUSTICE) &&
        (gl->ch->specials.arrest_by == victim) &&
        (gl->ch->only.npc->spec[2] & type))
      how_many--;
  }

  /* if the original how_many request is already being done, then
     don't do any more */
  if (how_many <= 0)
    return FALSE;

  /* figure out what room we are going to, and what town thats in */
  if (victim && (to_rroom == NOWHERE))
  {
    to_rroom = victim->in_room;
    if (to_rroom == NOWHERE)
      return FALSE;
  }
  town = zone_table[world[to_rroom].zone].hometown;

  /* now find the best place to send the guards FROM */
  for (i = 0; i < NUM_EXITS; i++)
  {
    int      rr, dist, dir;

    rr = real_room(hometowns[town - 1].guard_room[i]);
    if (rr == NOWHERE)
      continue;
    dir = find_first_step(rr, to_rroom, BFS_CAN_FLY | BFS_CAN_DISPEL, 0, 0, &dist);
    if (dir == BFS_ALREADY_THERE)
      dist = 0;
    else if (dir < 0)
      continue;
    if (dist < best_dist)
    {
      best_room = rr;
      best_dist = dist;
    }
  }

  if (best_room == NOWHERE)
  {
    wizlog(56,
           "Justice: No guards able to get to room %d. Tracking %s in room %d.",
           world[to_rroom].number, GET_NAME(victim),
           world[victim->in_room].number);
    return FALSE;
  }
  /* This sets up the type of hunter event, based on the 'type' mob
     spec[2] passed. */

  switch (type)
  {
  case MOB_SPEC_J_OUTCAST:
    hunt_type = HUNT_JUSTICE_INVADER;
    break;

  default:
    if (victim)
      hunt_type = HUNT_JUSTICE_SPECVICT;
    else
      hunt_type = HUNT_JUSTICE_SPECROOM;
  }

  // Can comment this out later.
  wizlog(56, "Justice: Dispatching %d guard(s) for %s.", how_many, 
    GET_NAME(victim) );

  /* now send them out! */
  while (how_many)
  {
    P_char   tch;

    tch = justice_make_guard(best_room);
    if (!tch)
    {
      wizlog(56,
             "Justice: Unable to load mob vnum %d. Tracking: %s in room %d.",
             hometowns[town - 1].guard_mob, GET_NAME(victim),
             world[victim->in_room].number);
      return FALSE;
    }
    SET_BIT(tch->only.npc->spec[2], type);

    tch->specials.arrest_by = victim;

    data.hunt_type = hunt_type;
    if (victim)
      data.targ.victim = victim;
    else
      data.targ.room = to_rroom;

    data.huntFlags = BFS_CAN_FLY | BFS_BREAK_WALLS;
    if (npc_has_spell_slot(tch, SPELL_DISPEL_MAGIC))
      data.huntFlags |= BFS_CAN_DISPEL;
    
    add_event(mob_hunt_event, PULSE_MOB_HUNT, tch, NULL, NULL, 0, &data, sizeof(hunt_data));
    //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, tch, data);
    how_many--;

  }                             /* while() */
  return TRUE;
}

P_char justice_make_guard(int rroom)
{
  /* assumes rroom is a valid real room number */
  int      vnum, town;
  P_char   ch;
  P_obj    obj = NULL;
  struct justice_guard_list *gl;

  vnum = hometowns[zone_table[world[rroom].zone].hometown - 1].guard_mob;

  ch = read_mobile(vnum, VIRTUAL);

  if (!ch)
    return NULL;

  /* make DAMN sure the birthplace is set... I need this to get back
     home when I'm done with my duty */
  GET_BIRTHPLACE(ch) = world[rroom].number;
  char_to_room(ch, rroom, 0);

  // clear money
  CLEAR_MONEY(ch);

  /* standard clause */
  if (GET_HIT(ch) > GET_MAX_HIT(ch))
    GET_HIT(ch) = GET_MAX_HIT(ch);

  /* justice guards that don't have the ability to see in the dark
     need torches... this takes care of that */
  if (!IS_AFFECTED2(ch, AFF2_ULTRAVISION))
  {
    obj = read_object(398, VIRTUAL);
    if (obj)
      equip_char(ch, obj, HOLD, 0);
  }
  /* justice guards should always be aggro to outcasts! */
/*  if (!IS_SET(ch->specials.act, ACT_AGG_OUTCAST))
    SET_BIT(ch->specials.act, ACT_AGG_OUTCAST);*/
  if (!IS_AGGROFLAG(ch, AGGR_OUTCASTS))
    SET_BIT(ch->only.npc->aggro_flags, AGGR_OUTCASTS);

  /* however, don't rely on the "standard" protector code for these
     special guards... if I want them to do something similar, I'll
     code it in JusticeGuardAct() */
  if (IS_SET(ch->specials.act, ACT_PROTECTOR))
    REMOVE_BIT(ch->specials.act, ACT_PROTECTOR);

  /* also, I don't want guards that I control to be hunting on their
     own, so nuke HUNTER flags */
  if (IS_SET(ch->specials.act, ACT_HUNTER))
    REMOVE_BIT(ch->specials.act, ACT_HUNTER);

  // set them to always be able to fly.
  SET_BIT(ch->specials.affected_by, AFF_FLY);

  /* THIS flag designates this mob as exclusive property of justice
     code */
  SET_BIT(ch->only.npc->spec[2], MOB_SPEC_JUSTICE);

  town = CHAR_IN_TOWN(ch);

  /* justice guards in hometowns should be aggro to evil/good based on
     the hometown flags */

  if (IS_SET(hometowns[town - 1].flags, JUSTICE_EVILHOME))
//    SET_BIT(ch->specials.act, ACT_AGG_RACEGOOD);
    SET_BIT(ch->only.npc->aggro_flags, AGGR_GOOD_RACE);

  if (IS_SET(hometowns[town - 1].flags, JUSTICE_GOODHOME))
//    SET_BIT(ch->specials.act, ACT_AGG_RACEEVIL);
    SET_BIT(ch->only.npc->aggro_flags, AGGR_EVIL_RACE);

// 1 in 4 chance that warriors are changed to clerics.  This is used for
// dispelling walls, and just neat that a group of 5 should have a cleric
  if (!npc_has_spell_slot(ch, SPELL_DISPEL_MAGIC) && !number(0,3))
  {
    ch->player.level = 61;
    ch->player.m_class = CLASS_CLERIC;
  } 


  /* give them some basic eq */
  //load_obj_to_newbies(ch);


  /* finally, put the guard in the "master" list of guards that
     justice controls.  This allows me to loop through just those mobs
     faster when I'm looking for a particular justice mob.  It also
     provides a nice monitoring system via the mm_* debugging code. */

  if (!dead_justice_guard_pool)
    dead_justice_guard_pool = mm_create("JUSTICE",
                                        sizeof(struct justice_guard_list),
                                        offsetof(struct justice_guard_list,
                                                 next), 1);
  gl = (struct justice_guard_list *) mm_get(dead_justice_guard_pool);
  gl->ch = ch;
  gl->next = guard_list;
  guard_list = gl;

  return ch;
}

/* this function needed for extract_char() in case a justice mob is
   purged, etc */

void justice_guard_remove(P_char ch)
{
  struct justice_guard_list *gl;

  if (IS_PC(ch) || !ch->only.npc || (!IS_SET(ch->only.npc->spec[2], MOB_SPEC_JUSTICE)))
    return;

  /* if I'm hunting anyone, I should stop now :) */
  justice_hunt_cancel(ch);

  /* pull the guard from the master list of justice controlled guards
   */

  if (!guard_list)
    return;
  if (!ch)
    return;

  if (ch == guard_list->ch)
  {
    gl = guard_list;
    guard_list = gl->next;
    mm_release(dead_justice_guard_pool, gl);
  }
  else
  {
    for (gl = guard_list; gl && gl->next; gl = gl->next)
      if (ch == gl->next->ch)
      {
        struct justice_guard_list *t = gl->next;

        gl->next = t->next;
        mm_release(dead_justice_guard_pool, t);
      }
  }
}

void justice_delete_guard(P_char ch)
{
  int      i;
  P_obj    obj;

  if (IS_PC(ch) || (!IS_SET(ch->only.npc->spec[2], MOB_SPEC_JUSTICE)))
    return;


  /* Justice guards which are removed via this function haven't been
     killed or purged, but are "done" with their mission.  As well,
     anything they are wearing is stuff I put on them when they
     loaded.  Instead of having that stuff just drop to the ground,
     which could result in a mess, just nuke it */

  for (i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
    {
      obj = unequip_char(ch, i);
      extract_obj(obj, TRUE); // If they've managed to pick up an arti.
    }
  /* similar story for stuff they are carrying.  While stuff they are
     carrying was probably given to them, just dropping it makes a
     mess... so... I'll just nuke it :) */

  if (ch->carrying)
  {
    P_obj    next_obj;

    for (obj = ch->carrying; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      extract_obj(obj, TRUE); // If they've managed to pick up an arti.
    }
  }
  /* delete the char.  This will have the side effect of calling
     justice_guard_remove() which will deal with taking the guard off
     the master guard list. */

  extract_char(ch);
}


/*
 * checks to see if ch is a justice mob, if so, and its not fighting its
 * intended victim, clears the proper variables so that mob can be re-used
 * for justice purposes
 */

void justice_hunt_cancel(P_char ch)
{
  P_event  ev;

  if (IS_PC(ch) || !ch->only.npc || (!IS_SET(ch->only.npc->spec[2], MOB_SPEC_JUSTICE)))
    return;

  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_J_OUTCAST))
    if (ch->specials.arrest_by == ch->specials.fighting)
      /* we are doing exactly what we are supposed to! */
      return;


  /* if for some reason a guard going after a PK'er stops hunting,
     then just just outcast the PK'er and let'em suffer */
  if (IS_SET(ch->only.npc->spec[2], MOB_SPEC_J_PK))
  {
    justice_set_outcast(GET_PLYR(ch->specials.arrest_by), CHAR_IN_TOWN(ch));
    /* fall through... */
  }
  /* Insert here other conditions why I might NOT want to cancel the
     justice action!  For example, what happens if a mob is dragging
     someone to jail, and stops to kill something? */



  /* okay.. If I'm this far, I'm SURE I want to cancel the justice
     action (perhaps its finished?... or the victim died?) */

  /* then clear any unwanted spec[2] flags... which are all but the
     justice one */
  ch->only.npc->spec[2] = MOB_SPEC_JUSTICE;

  return;
}

void justice_victim_remove(P_char ch)
{
  struct justice_guard_list *gl;

  /* this is pretty much only called when a char dies.  It will see if
     any of the justice mobs are for any reason after them, and if so,
     will get them to stop it */

  for (gl = guard_list; gl; gl = gl->next)
    if (gl->ch->specials.arrest_by == ch)
      justice_hunt_cancel(gl->ch);
}


/*
 * this one function will handle both players and mobs reporting crimes.
 * First, we check if its a mob. If so, give him something to say. Then
 * for either, we check the validity of the complaint. If true, various
 * justice levels plus bribes will send the guards running.
 */
void do_report_crime(P_char ch, char *arg, int cmd)
{
  return;
}


/*
 * this function sends the mob to report a crime if he's a CITIZEN
 */

void justice_send_witness(P_char ch, P_char attacker, P_char victim, int rroom,
                     int crime)
{
  hunt_data data;

  return;

  witness_add(ch, attacker, victim, rroom, crime);

  ch->only.npc->spec[2] = MOB_SPEC_REPORTING | MOB_SPEC_JUSTICE;

  data.hunt_type = HUNT_JUSTICE_SPECROOM;
  data.targ.room = real_room(hometowns[CHAR_IN_TOWN(ch) - 1].report_room);
  add_event(mob_hunt_event, PULSE_MOB_HUNT, ch, NULL, NULL, 0, &data, sizeof(hunt_data));
  //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, ch, data);

  return;
}


/* This function check for witness in room and in adj. room and set witness record
   Some crime cannot be witness if not in same room, if send_witness is TRUE, send
   the witness to report the crime, we only send 2 witness max. */

void
witness_scan(P_char attacker, P_char victim, int rroom, int crime,
             int send_witness)
{
  P_char   t_ch, nextch, next;
  int      door, i = 0;
  int      nb_witness = 0;
  const int no_crime[] = { CRIME_THEFT, CRIME_ATT_THEFT, -1 };  /* crime we cannot witness from afar */
  return;

  if (!CHAR_IN_TOWN(attacker))
    return;

/*  LOOP_THRU_PEOPLE(t_ch, attacker) {*/
  for (t_ch = world[attacker->in_room].people; t_ch; t_ch = nextch)
  {
    nextch = t_ch->next_in_room;

    if ((send_witness) && (NPC_IS_CITIZEN(t_ch)))
    {
      if (nb_witness < 2)
        justice_send_witness(t_ch, attacker, victim, rroom, crime);
      else
      {
        witness_add(t_ch, attacker, victim, rroom, crime);
        nb_witness++;
      }
    }
    else
      witness_add(t_ch, attacker, victim, rroom, crime);
  }

  while (no_crime[i] != -1)
  {
    if (no_crime[i++] == crime)
      return;
  }

  for (door = 0; door < NUM_EXITS; door++)
  {
    if (CAN_GO(attacker, door))
    {
      for ((t_ch) = world[EXIT(attacker, door)->to_room].people;
           (t_ch) != NULL; (t_ch) = next)
      {
        next = t_ch->next_in_room;

        if ((send_witness) && (NPC_IS_CITIZEN(t_ch)))
        {
          if (nb_witness < 2)
            justice_send_witness(t_ch, attacker, victim, rroom, crime);
          else
            witness_add(t_ch, attacker, victim, rroom, crime);
          nb_witness++;
        }
        else
          witness_add(t_ch, attacker, victim, rroom, crime);
      }
    }
  }

  return;
}


/*
 * this function will handle triggering shouts for help, and recording
 * witness records IF the victim isn't OUTLAW.  (If the victim is a NPC
 * and isn't flagged as a citizen, no witness records are made)
 * SEARCH HOOK */

void justice_witness(P_char attacker, P_char victim, int crime)
{
  P_char   ch = NULL;
  struct affected_type af;

  return;

  if (!attacker)
    return;

/*  if (mini_mode)
    return;*/

  /* on the mud, suicide is NOT a crime */

  if (victim)
    if (attacker == victim)
      return;

  if (victim)
  {
    if (GET_LEVEL(victim) > 25) // over 25?  You can fight back! */
      return;
  }

  /* make sure if they are invaders, they are dealt with */
  if (IS_INVADER(attacker))
  {
    justice_action_invader(attacker);
    return;
  }

  if (victim)
  {
    if (IS_INVADER(victim))
    {
      justice_action_invader(victim);
      return;
    }
  }

  if (victim && IS_DISGUISE(victim))
  {
    /* Disguise stuff in here ... evils get invaders, goodies get normal. */
  }

  /*
     if (victim && attacker && (IS_DISGUISE(attacker) || IS_DISGUISE(victim)))
     return; /* disguised?  Tough luck, they can do what they want to you */

  if (IS_TRUSTED(attacker))
    return;

  if (victim)
    if (IS_TRUSTED(victim))
      return;

  /* thieves/assassins have repurcussions */
  if (victim)
    if (GET_CLASS(victim, CLASS_ROGUE) && IS_PC(attacker)
        && (crime == CRIME_ATT_MURDER || crime == CRIME_MURDER)
        && justice_is_criminal(victim))
      return;

  if (IS_NPC(attacker))
  {
    if (IS_PC_PET(attacker))
    {
      if (!IS_FIGHTING(attacker) &&
          (crime == CRIME_ATT_MURDER || crime == CRIME_MURDER))
      {
        attacker = GET_MASTER(ch);
      }
      else if (crime != CRIME_ATT_MURDER && crime != CRIME_MURDER)
      {
        attacker = GET_MASTER(ch);
      }
      else
        return;
    }
    else
      return;
  }

  if (attacker && victim)
  {
    if (CHAR_IN_JUSTICE_AREA(attacker) && !CHAR_IN_JUSTICE_AREA(victim))
      return;
    if (racewar(attacker, victim))
      return;
  }
  if (ch)
  {
    if (!CHAR_IN_JUSTICE_AREA(attacker))
      return;
    if (!CHAR_IN_TOWN(attacker))
      return;
  }

  if (victim)
    if (IS_NPC(victim))
      return;
/*
  if (attacker && IS_DISGUISE(attacker) && (!number(0,2)))
    return;  /* in theory should report wrongly, but we just wont' report at all */

/*
  if (victim && IS_DISGUISE(victim) && (!number(0,2)))
    return;  /* in theory should report wrongly, but we just wont' report at all */
  /* if a crime happens OUTSIDE a hometown but in patrol area, log it on the char.
     we only log violent crime outside hometown */

  if (CHAR_IN_JUSTICE_AREA(attacker))
  {

    /* first we check if this crime is a crime in this hometown patrol area */

    if (!GET_CRIME_P(CHAR_IN_JUSTICE_AREA(attacker), crime))
      return;

    if (victim)
    {
      if ((attacker->specials.fighting == victim) ||
          (victim->specials.fighting == attacker))
        return;

      if (crime == CRIME_ATT_MURDER)
        if (witness_find
            (victim->specials.witnessed, J_NAME(victim), J_NAME(attacker),
             CRIME_ATT_MURDER, victim->in_room, NULL))
          return;

    }

    /* add timer to char, and prevent renting */
    if (!IS_AFFECTED4(attacker, AFF4_LOOTER))
    {
      bzero(&af, sizeof(af));
//      af.type = SKILL_CRIME;
      af.duration = 10;
      af.bitvector4 = AFF4_LOOTER;
      affect_to_char(attacker, &af);
    }

    /* ok for murder just make sure we get the real murderer */
    if (victim && crime == CRIME_MURDER)
    {
      if (witness_find
          (attacker->specials.witnessed, J_NAME(attacker), J_NAME(victim),
           CRIME_ATT_MURDER, attacker->in_room, NULL))
      {
        witness_scan(attacker, victim, attacker->in_room, crime, FALSE);
      }
      else
        if (witness_find
            (attacker->specials.witnessed, J_NAME(victim), J_NAME(attacker),
             CRIME_ATT_MURDER, attacker->in_room, NULL))
      {
        witness_scan(victim, attacker, attacker->in_room, crime, FALSE);
      }
      else
      {
        witness_scan(attacker, victim, attacker->in_room, crime, FALSE);
      }
      return;
    }

    if (!victim && crime == CRIME_ATT_MURDER)
      return;

    witness_scan(attacker, victim, attacker->in_room, crime, FALSE);

    return;
  }

  /* CODE FOR DEALING WITH HOMETOWNS */
  if (CHAR_IN_TOWN(attacker))
  {

    /* first we check if this crime is a crime in this hometown */

    if (!GET_CRIME_T(CHAR_IN_TOWN(attacker), crime))
      return;

    /* what do I care if outcasts get nuked?  I'll even send help! */

    if (victim)
    {
      if (IS_OUTCAST(victim))
      {
        justice_action_invader(victim);
        return;
      }
    }

    /* if the attacker is outcast, let invader code deal with it... */
    if (world[attacker->in_room].zone && IS_OUTCAST(attacker))
    {
      justice_action_invader(attacker);
      return;
    }

    /* if they are already fighting each other, and its attempted
       murder, we really can't tell who started it.  Just hope that
       the first blow got logged */
    if (victim)
    {
      if ((attacker->specials.fighting == victim) ||
          (victim->specials.fighting == attacker))
        return;

      if (crime == CRIME_ATT_MURDER)
        if (witness_find
            (victim->specials.witnessed, J_NAME(victim), J_NAME(attacker),
             CRIME_ATT_MURDER, victim->in_room, NULL))
          return;

    }

    /* add timer to char, and prevent renting */
    if (!IS_AFFECTED4(attacker, AFF4_LOOTER))
    {
      bzero(&af, sizeof(af));
//      af.type = SKILL_CRIME;
      af.duration = 10;
      af.bitvector4 = AFF4_LOOTER;
      affect_to_char(attacker, &af);
    }

    /* ok for murder just make sure we get the real murderer */
    if (victim && crime == CRIME_MURDER)
    {
      if (witness_find
          (attacker->specials.witnessed, J_NAME(attacker), J_NAME(victim),
           CRIME_ATT_MURDER, attacker->in_room, NULL))
      {
        witness_scan(attacker, victim, attacker->in_room, crime, TRUE);
      }
      else
        if (witness_find
            (attacker->specials.witnessed, J_NAME(victim), J_NAME(attacker),
             CRIME_ATT_MURDER, attacker->in_room, NULL))
      {
        witness_scan(victim, attacker, attacker->in_room, crime, TRUE);
      }
      else
      {
        witness_scan(attacker, victim, attacker->in_room, crime, TRUE);
      }
      return;
    }

    if (!victim && crime == CRIME_ATT_MURDER)
      return;

    witness_scan(attacker, victim, attacker->in_room, crime, TRUE);

    if (crime == CRIME_MURDER)
      wizlog(56, "JUSTICE: %s murdered %s in COLD BLOOD in a hometown!\r\n",
             J_NAME(attacker), J_NAME(victim));

    return;
  }
}


void justice_action_invader(P_char ch)
{
  struct zone_data *zone_struct;
  int room;

  if( IS_TRUSTED(ch) )
    return;

/*
  if (mini_mode)
   return;
*/

  if( !CHAR_IN_TOWN(ch) )
    return;

  if (!IS_INVADER(ch) && !IS_OUTCAST(ch))
    return;


  /*Original Justice
  if (!justice_send_guards(NOWHERE, ch, MOB_SPEC_J_OUTCAST, (MAX(11, GET_LEVEL(ch)) / 11) + 1))
   {
    return;
   }
  */

  zone_struct = &zone_table[world[ch->in_room].zone];
  room = ch->in_room;

  //  Re-doing justice here.  Major idea is to get rid of the overkill response
  //  which doesn't allow for any hometown raiding, but still not make it far
  //  too easy for a group to sit in a hometown and frag people all day long.
  //  I've removed the constant spamming of the justice_raiding event that was
  //  causing mobs to constantly be spawned to hunt invaders.  Instead, it will
  //  be applied once only and give a 3 minute opportunity before it is refreshed.
  //  When it gets refreshed, I'll add a different event as a placeholder/counter.
  //  If this counter reaches 3x(10 minutes approx), I'll have a reinforcement
  //  squad issued from another hometown that will 'encourage' the invaders to
  //  leave.  Hopefully this works out better than justice ever has...  - Jexni 3/4/11

  if( IS_INVADER(ch) && IS_PC(ch) && get_scheduled(ch, event_justice_raiding) )
  {
    if( !number(0, 1) )
      return;
  }
  else
  {
    zone_struct->status = ZONE_RAID;
    add_event(event_justice_raiding, WAIT_SEC * 200, ch, 0, 0, 0, &room, sizeof(room));
  }

  if( IS_INVADER(ch) )
  {
    if( !number(0, 2) )
    {
      justice_send_guards(NOWHERE, ch, MOB_SPEC_J_OUTCAST, 1);
    }

    if( IS_SET(hometowns[CHAR_IN_TOWN(ch) - 1].flags, JUSTICE_GOODHOME)
      && !(number(0, 15)) && get_property("justice.alarms.good", 1) )
    {
      int rnum = number(1, 4);
      if(rnum == 1)
        justice_hometown_echo(CHAR_IN_TOWN(ch), "&+RAlarm bells sound, &+rsignalling an invasion!&n");
      if(rnum == 2)
        justice_hometown_echo(CHAR_IN_TOWN(ch), "&+LMilitia forces muster to bolster the town's defenses against the &=Lrinvaders!!!&n");
      if(rnum == 3)
        justice_hometown_echo(CHAR_IN_TOWN(ch), "&+LMilitia forces muster to bolster the town's defenses against the &=Lrinvaders!!!&n");
      if(rnum == 4)
        justice_hometown_echo(CHAR_IN_TOWN(ch), "&+RAlarm bells sound, &+rsignalling an invasion!&n");
      return;
    }
    else if( IS_SET(hometowns[CHAR_IN_TOWN(ch) - 1].flags, JUSTICE_EVILHOME)
      && !number(0, 15) && get_property("justice.alarms.evil", 1) )
    {
      justice_hometown_echo(CHAR_IN_TOWN(ch), "&+yHorns begin to &+Ybellow &+yand drums &+cthunder&n &+yto the &+Rcall to arms!&n");
      return;
    }
  }
}

void event_justice_raiding(P_char ch, P_char victim, P_obj obj, void *data) {

  struct zone_data *zone;
  int room = *((int*)data);

  zone = &zone_table[world[room].zone];
  zone->status = ZONE_NORMAL;
  return;
}


void justice_action_wanted(P_char ch)
{
}

void justice_action_arrest(P_char police, P_char criminal)
{
  return;
}

int justice_is_criminal(P_char ch)
{
  int      town;
  crm_rec *crec, *t;

  if (!(town = CHAR_IN_TOWN(ch)))
    return FALSE;

  crec = hometowns[town - 1].crime_list;
  t = NULL;
  do
  {
    t =
      crime_find(crec, J_NAME(GET_PLYR(ch)), NULL, 0, NOWHERE, J_STATUS_CRIME,
                 t);
    if (t && (t->crime > CRIME_LAST_FAKE))
      return TRUE;
  }
  while (t);

  return FALSE;

}



/*
 * setting law flags, due to the complicated (and therefore better) way
 * they are done, its a bit more difficult. :(  First, we have to CLEAR
 * the bits, then we can set them.  As well, a bogus flag parameter to
 * this can cause a MAJOR problem...  therefore, I'm making it a function
 */

void PC_SET_TOWN_JUSTICE_FLAGS(P_char ch, int flag, int town)
{
  ulong    tmp;

  if (IS_NPC(ch))
    return;

  if ((flag > 3) || (flag < 0))
  {
    logit(LOG_EXIT, "make_just_flags - flag out of range!");
    raise(SIGSEGV);
  }
  /*
   * okay... this will clear the flags... yes.. it IS messy.. but it
   * works.  what is does is take 3 (11 in binary), shift it over to the
   * proper town, and then inverses the bits of the whole fucking mess.
   * The end result is that the target town has 00, and the other towns
   * all have 11.  Then we do a bitwise AND... effectively clearing the
   * flags for the target town
   */

  tmp = ch->only.pc->law_flags & (~(3U << ((town - 1) * 2)));

  /* now we can just set the bits of the target town...   */

  ch->only.pc->law_flags = tmp | (flag << ((town - 1) * 2));
}


/* destroy and deallocate witness records for ch */

void witness_destroy(P_char ch)
{
  wtns_rec *rec;

  while (ch->specials.witnessed)
  {
    rec = ch->specials.witnessed->next;
    str_free(ch->specials.witnessed->attacker);
    str_free(ch->specials.witnessed->victim);
    mm_release(dead_witness_pool, ch->specials.witnessed);
    ch->specials.witnessed = rec;
  }
}

/* the witness_* functions must work via NAMES, and not pointers.
   Doing it with pointers is BEGGING for problems if a char is killed,
   or rents, or is purged, or whatever.. as then the pointer is
   pointing into limbo... */

/* add a witness record to ch with the mentioned parameters.  This assumes
   that the crime occured in the same room as ch.  If ch is NULL, then
   the witness record is added to the hometowns's struct that the
   attacker is in. */

void
witness_add(P_char ch, P_char attacker, P_char victim, int rroom, int crime)
{
  /* adding records to the beginning of the list will make things faster */
  wtns_rec *rec;

  return;

#ifdef JUSTICE_DEBUG
  char     debug_buf[MAX_STRING_LENGTH];

  sprintf(debug_buf, "You see %s commit crime %d against %s\r\n",
          attacker, crime, victim);
  if (ch)
    send_to_char(debug_buf, ch);
#endif
  logit(LOG_CRIMES, "%s%s commited %s against %s at [%d]%s",
        ch ? "" : "(TOWN) ",
        J_NAME(attacker),
        crime_list[crime],
        (victim) ? J_NAME(victim) : "None",
        world[rroom].number, world[rroom].name);
/*  if (crime > CRIME_LAST_FAKE)
    statuslog(55, "&+R%sJUSTICE:&N %s commited %s against %s at [%d]%s",
              ch ? "" : "TOWN ",
              J_NAME(attacker),
              crime_list[crime],
              (victim) ? J_NAME(victim) : "None",
              world[rroom].number,
              world[rroom].name);*/

  if (!dead_witness_pool)
    dead_witness_pool = mm_create("WITNESS",
                                  sizeof(wtns_rec),
                                  offsetof(wtns_rec, next), 1);

  rec = (wtns_rec *) mm_get(dead_witness_pool);
  rec->time = time(NULL);
  rec->attacker = str_dup(J_NAME(attacker));
  if (victim)
    rec->victim = str_dup(J_NAME(victim));
  else
    rec->victim = str_dup("None");
  rec->crime = crime;
  rec->room = rroom;
  rec->next = ch->specials.witnessed;
  ch->specials.witnessed = rec;

}


/* fonction to add a crime to the town list (TASFALEN) */

void
crime_add(int town, char *attacker, const char *victim,
          int rroom, int crime, time_t ttime, int status, int money)
{
  /* adding records to the beginning of the list will make things faster */
  crm_rec *crec;

#ifdef JUSTICE_DEBUG
  char     debug_buf[MAX_STRING_LENGTH];

  sprintf(debug_buf, "You report %s commit crime %d against %s\r\n",
          attacker, crime, victim);
  if (ch)
    send_to_char(debug_buf, ch);
#endif
/*  logit(LOG_CRIMES, "report %s commited %s against %s at [%d]%s",
        attacker,
        crime_list[crime],
        victim,
        world[rroom].number,
        world[rroom].name);
  if (crime > CRIME_LAST_FAKE)
    statuslog(55, "&+RTOWN JUSTICE REPORT:&N %s commited %s against %s at [%d]%s",
              attacker,
              crime_list[crime],
              victim,
              world[rroom].number,
              world[rroom].name);*/

  /* ok we dont want to log the same crime over and over but we want to know how
     many time it been log, so we use the var. money to keep count and update
     the record for last crime */

  if ((crec = crime_find(hometowns[town - 1].crime_list,
                         attacker, victim, crime, NOWHERE, status, NULL)))
  {
    crec->money = crec->money + money;
    crec->time = ttime;
    crec->room = rroom;
  }
  else
  {
    if (!dead_crime_pool)
      dead_crime_pool = mm_create("CRIME",
                                  sizeof(crm_rec),
                                  offsetof(crm_rec, next), 1);

    crec = (crm_rec *) mm_get(dead_crime_pool);
    crec->time = ttime;
    crec->attacker = str_dup(attacker);
    crec->victim = str_dup(victim);
    crec->crime = crime;
    crec->room = rroom;
    crec->money = money;
    crec->status = status;
    crec->next = hometowns[town - 1].crime_list;
    hometowns[town - 1].crime_list = crec;
  }
  if (!writeTownJustice(town))
    statuslog(55, "Town Justice %d save problem", town);

}

/*
 * finds and returns a pointer to the first wtns_rec on ch based on the
 * passed search parameters.  Returns NULL if no record was found.
 *
 * For parameters which you don't wish to be included as search
 * parameters, pass a value of 0 or NULL (whichever is appropiate) or
 * NOWHERE (for room)  attacker   - attacker name victim     - victim name
 * crime      - crime type last_found - used for a "find next" type
 * operation.  If this points to a record in ch's records, the search will
 * begin at the record following that one.
 */

wtns_rec *witness_find(wtns_rec * rec, char *attacker, char *victim,
                       int crime, int room, wtns_rec * last_found)
{

  /*
   * if last_found is none-NULL, past that record...
   */

  if (last_found)
  {
    wtns_rec *tmp_rec;

    while (rec)
    {
      tmp_rec = rec;
      rec = rec->next;
      if (tmp_rec == last_found)
        break;
    }
  }
  while (rec)
  {

    if (!attacker || !str_cmp(attacker, rec->attacker))
      if (!victim || !str_cmp(victim, rec->victim))
        if ((room == NOWHERE) || (room == rec->room))
          if ((!crime || (crime == rec->crime)))
            break;
    rec = rec->next;
  }
  return rec;
}

/* fonction to find a specific record in the crime list of town (TASFALEN) */
crm_rec *crime_find(crm_rec * rec, char *attacker, const char *victim,
                    int crime, int room, int status, crm_rec * last_found)
{

  /*
   * if last_found is none-NULL, past that record...
   */

  if (last_found)
  {
    crm_rec *tmp_rec;

    while (rec)
    {
      tmp_rec = rec;
      rec = rec->next;
      if (tmp_rec == last_found)
        break;
    }
  }

  while (rec)
  {
    if (!attacker || !str_cmp(attacker, rec->attacker))
      if (!victim || !str_cmp(victim, rec->victim))
        if ((room == NOWHERE) || (room == rec->room))
          if ((!crime || (crime == rec->crime)))
            if ((!status || (status == rec->status)))
              break;
    rec = rec->next;
  }

  return rec;
}

/* removes the witness record pointed to by 'what' from ch or
   hometown.  Returns TRUE if the record was found and removed.  If 'ch'
   is provided, the witness records on 'ch' are searched.  If hometown
   is provided, the witness records in that hometown are searched.  If
   neither, or both, are provided, the function crashes. */

int witness_remove(P_char ch, wtns_rec * what)
{
  wtns_rec *rec;

  /* sanity checks */

  if (!what)
    return 0;

  rec = ch->specials.witnessed;

  if (!rec)
    return 0;

  if (what == rec)
  {                             /* first record */
    ch->specials.witnessed = rec->next;
    str_free(rec->attacker);
    str_free(rec->victim);
    mm_release(dead_witness_pool, rec);
    return 1;
  }
  while (rec->next)
  {
    if (rec->next == what)
    {
      wtns_rec *tmp_rec;

      tmp_rec = (rec->next)->next;
      str_free(rec->next->attacker);
      str_free(rec->next->victim);
      mm_release(dead_witness_pool, rec->next);
      rec->next = tmp_rec;
      return 1;
    }
    rec = rec->next;
  }
  return 0;
}


int crime_remove(int hometown, crm_rec * what)
{
  crm_rec *crec;

  /* sanity checks */

  if (!what)
    return 0;

  crec = hometowns[hometown - 1].crime_list;

  if (!crec)
    return 0;

  if (what == crec)
  {                             /* first record */
    hometowns[hometown - 1].crime_list = crec->next;
    str_free(crec->attacker);
    str_free(crec->victim);
    mm_release(dead_crime_pool, crec);
    return 1;
  }
  while (crec->next)
  {
    if (crec->next == what)
    {
      crm_rec *tmp_rec;

      tmp_rec = (crec->next)->next;
      str_free(crec->next->attacker);
      str_free(crec->next->victim);
      mm_release(dead_crime_pool, crec->next);
      crec->next = tmp_rec;
      return 1;
    }
    crec = crec->next;
  }
  return 0;
}


/*
 * shout_and_hunt().  This function is the result of some of the justice
 * code which has been kludged to work with other, non-hometown, mobs.
 * Its intended to be called from within the special procedure of a mob.
 * For an example, check 'sister_knight()' in specs.mobile.c.
 *
 * ch - pointer to me.. the mob the proc was called for. max_distance -
 * mobs which are greater then this distance away won't come to help.
 * This MUST be specified as greater then 0. shout_str - this is a string
 * which the mob will shout when its being attacked.  It must include a
 * single %s which will be replaced by the name of the char attacking.
 * locator_proc - This parameter, if specified as non-NULL should be the
 * address of a function.  Only mobs which have this function as a special
 * will run to help. vnums - This is a pointer to a list of vnums.  I
 * think the best way to specify it would be "{num1, num2, num3, 0}".  In
 * this case, only mobs with vnums of num1, num2, and num3 will respond.
 * The last vnum in the list MUST be 0.  If you don't want to limit based
 * on vnum, pass NULL here.  As a special note:  specifying a long list of
 * vnums here will lag the mud.  Don't do it.  This method should ONLY be
 * used in the cases discussed below within the function. act_mask - If
 * non-0, mobs which respond must have these bits set in their ACT flags.
 * For example, passing (ACT_HAS_MU | ACT_HAS_WA) would tell the function
 * to only have mobs which have BOTH of those act flags to come running.
 * no_act_mask - if non-0, mobs which respond must NOT have ANY of these
 * bits set in their ACT flags.  It would be foolish to specify the same
 * bit in both the act_mask, and the no_act_mask.  Therefore, the function
 * will segfault and blame you if you do it.
 */

int shout_and_hunt(P_char ch, int max_distance, const char *shout_str, int (*locator_proc) (P_char, P_char, int, char *), int vnums[], ulong act_mask, ulong no_act_mask)
{
  P_char    target;
  P_nevent  ev;
  char      buffer[MAX_STRING_LENGTH], buffer2[MAX_STRING_LENGTH];
  hunt_data data;
  int       dummy;
  int       i;
  bool      has_help = FALSE;
  bool      found_help = FALSE;

  if( !ch || !max_distance || (act_mask & no_act_mask) )
  {
    logit(LOG_EXIT, "shout_and_hunt proc called with bogus params");
    raise(SIGSEGV);;
  }

  if( IS_PC(ch) )
  {
    return FALSE;
  }

  if( !IS_FIGHTING(ch) )
  {
    return FALSE;
  }

  /*
   * if its quiet in here, can't shout for help
   */
  if( IS_SET(world[ch->in_room].room_flags, ROOM_SILENT) )
  {
    return FALSE;
  }
  /*
   * if paralyzed, casting, or sleeping, can't shout for help
   */
  if( IS_IMMOBILE(ch) || IS_CASTING(ch) || GET_STAT(ch) == STAT_SLEEPING )
  {
    return FALSE;
  }
  /*
   * how do I keep from shouting every round?
   */

  /*
   * I am gonna use the justice scheme here.  This assumes, of course,
   * that this proc will NOT be used on mobs subject to justice. Because
   * of this, I'll core dump if it happens. (Note that it would be
   * better to just remove the proc - however then the "bug" might go on
   * unnoticed forever.  Doing a core dump will get attention ).
   */
  /*
   * However, guards need to be vicious. Let's allow them to have as
   * many abilities, procs, etc as we need to get the job done right.
   */

  if( NPC_IS_CITIZEN(ch) && !IS_GUARD(ch) )
  {
    logit(LOG_EXIT, "shout_and_hunt proc called for a ACT_WITNESS mob");
    raise(SIGSEGV);
  }

  // Attack the master and shout for the master not the pet.
  if( IS_NPC(ch->specials.fighting) && IS_PC_PET( ch->specials.fighting )
    && ch->specials.fighting->in_room == (GET_MASTER(ch->specials.fighting))->in_room )
  {
    ch->specials.fighting = GET_MASTER(ch->specials.fighting);
  }

  /*
   * Okay... find out if we've dealt with this tank.  If not, deal with
   * him
   */

  if( witness_find(ch->specials.witnessed, J_NAME(ch->specials.fighting), NULL, 0, NOWHERE, NULL) )
    return FALSE;
  /*
   * party time
   */
  /*
   * first shout for help.  Note that I'm going to shout even if I'm
   * bashed.  Just because I'm sitting on my butt doesn't mean I can
   * scream for help
   */
  sprintf(buffer, "%s shouts '", ch->player.short_descr);
  sprintf(buffer2, shout_str, CAN_SEE(ch, ch->specials.fighting) ? J_NAME(ch->specials.fighting) : "Someone");
  strcat(buffer, buffer2);
  strcat(buffer, "&n'\n");

/*
 * Replacing this with new generic function, ha.
 * Also, i think it's the first place this new function is going to be used, go me!

  do_sorta_yell(ch, buffer);
*/
  int shout_distance = MIN(RMFR_MAX_RADIUS, max_distance);

  if (!ch->in_room)
  {
    // REMOVE_BIT on a non-existant room?!
//    REMOVE_BIT(world[ch->in_room].room_flags, SINGLE_FILE);
    logit(LOG_DEBUG, "Problem in shout_and_run() for %s - bogus room", GET_NAME(ch));
    wizlog(MINLVLIMMORTAL,"Problem in shout_and_run() for %s - bogus room", GET_NAME(ch));
    return FALSE;
  }
/*
 * Decided to let it be heard only inside the same zone, as it is now.
 */
  act(buffer, TRUE, ch, 0, 0, TO_ROOM);
  radiate_message_from_room(ch->in_room, buffer, shout_distance, (RMFR_FLAGS) (RMFR_RADIATE_ALL_DIRS | RMFR_PASS_WALL), 0);

  /*
   * need to find all the mobs which 'locator proc' as their special
   * procedure.  Unfortunatly, there is only 1 way to do this: going
   * through the entire character_list.  For all mobs which have the
   * proper proc, if they aren't already hunting, have them start :)
   */

  for (target = character_list; target; target = target->next)
  {
    if( IS_NPC(target) && (!locator_proc ||
      (mob_index[GET_RNUM(target)].func.mob == locator_proc)) &&
      !((act_mask & target->specials.act) ^ act_mask) &&
      !(no_act_mask & target->specials.act))
    {

      i = 0;

      /*
       * if a vnum list was provided, check it.  Note that this is
       * the LEAST preferred method for finding a target mob, as if
       * more then 1 vnum is provided, its the slowest.  However, if
       * alot of the mobs involved have their own specialized
       * procedure, or if you wish some mobs to come running who
       * don't shout for help, this is the only way of checking.  I
       * don't like this. While its the best way to do it, its the
       * most likely part of this function to lag the mud while it
       * compares an entire list of vnums to every fucking mob in
       * the game.  If you have 10,000 mobs in the game, and send
       * down a list of only 5 vnums, it results in 50,000
       * comparisms.  In contrast, using only the locator_proc
       * and/or act_mask methods only check each mob ONCE.  Gond
       * requested that this function be able to work with a list of
       * vnums...  blame him!  (neb)
       */

      if (vnums)
      {
        while ((vnums[i] != mob_index[GET_RNUM(target)].virtual_number) &&
               vnums[++i]) ;
        if (!vnums[i])
          continue;
      }
      /*
       * got one.  If they aren't already hunting someone, make
       * things happen
       */

      if (IS_FIGHTING(target))
        continue;

//debug( "shout_and_hunt: NPC vnum %d.", mob_index[GET_RNUM(target)].virtual_number );

      LOOP_EVENTS_CH(ev, target->nevents)
      {
        if (ev->func == mob_hunt_event)
        {
          break;
        }
      }
      if( ev )
        continue;

      has_help = TRUE;

      /*
       * check if its a possible hunt.. and if its within the max
       * distance
       */

//       debug("Okay boss mob is shouting at this point and we calling for find_first_step in justice.c 2421");
      if( find_first_step(target->in_room, ch->in_room, (IS_MAGE(target) ? BFS_CAN_FLY : 0) |
        (npc_has_spell_slot(target, SPELL_DISPEL_MAGIC) ? BFS_CAN_DISPEL : 0) |
        BFS_BREAK_WALLS, 0, 0, &dummy) < 0)
      {
//        debug( "shout_and_hunt: mob (%s) no first step to room (%d).", J_NAME(target), ch->in_room);
        continue;
      }

      if (dummy > max_distance)
      {
//        debug( "shout_and_hunt: mob (%s) too far to hunt to room (%d).", J_NAME(target), ch->in_room);
        continue;
      }

      if (CAN_SEE(ch, ch->specials.fighting))
      {
        data.hunt_type = HUNT_JUSTICE_INVADER;
        data.targ.victim = ch->specials.fighting;
      }
      else
      {
        data.hunt_type = HUNT_ROOM;
        data.targ.room = ch->in_room;
      }
      found_help = TRUE;
//    debug( "shout_and_hunt: adding hunt event mob (%s) to room (%d).", J_NAME(target), ch->in_room);
      add_event(mob_hunt_event, PULSE_MOB_HUNT, target, NULL, NULL, 0, &data, sizeof(hunt_data));
      //AddEvent(EVENT_MOB_HUNT, PULSE_MOB_HUNT, TRUE, target, data);
    }
  }

  // Flee if no help can get to ch.
  if( has_help && !found_help )
    do_flee( ch, "", CMD_FLEE );

  /*
   * okay... last thing to do is add this "tank" to the mobs justice
   * memory so we don't shout about him/her again
   */

  witness_add(ch, ch->specials.fighting, ch, ch->in_room, CRIME_ATT_MURDER);

  return TRUE;
}


/* this function was needed so that (for justice purposes), mobs could
   shout and it would be "heard" even if the mob was indoors. */

void do_sorta_yell(P_char ch, char *str)
{
  P_desc   i;
  char     Gbuf1[MAX_STRING_LENGTH];

  if (IS_PC(ch))
  {
    send_to_char
      ("Sorry, only mobs can sort of yell.  You'll have to shout\r\n", ch);
    return;
  }
  if (!str)
    return;

  for (i = descriptor_list; i; i = i->next)
  {
    if (i->character && (i->character != ch) &&
        !is_silent(i->character, FALSE) &&
        !IS_SET(i->character->specials.act, PLR_NOSHOUT) && !i->connected)
      if (world[i->character->in_room].zone == world[ch->in_room].zone)
      {
        sprintf(Gbuf1, "$n shouts %s'%s'", language_known(ch, i->character),
                language_CRYPT(ch, i->character, str));
        act(Gbuf1, 0, ch, 0, i->character, TO_VICT);
      }
  }

  if (ch->desc)
    ch->desc->last_input[0] = '\0';
}

/* function to echo 'str' to anyone in 'town' hometown (who's awake)  */

void justice_hometown_echo(int town, const char *str)
{
  P_desc   d;

  if (!str || !*str)
    return;

  for (d = descriptor_list; d; d = d->next)
    if ((d->connected == CON_PLYNG) &&
        (CHAR_IN_TOWN(d->character) == town) && (IS_AWAKE(d->character)))
    {
      send_to_char(str, d->character);
      send_to_char("\r\n", d->character);
    }
}


/* fonction to judge a criminal for his/her crimes */

void justice_judge(P_char ch, int town)
{
  crm_rec *crec = NULL;
  int      crime_index = 0;
  int      temp;
  int      previous_debt = 0;
  char     buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];;


  if (!ch)
    return;

  sprintf(buf, "&+R-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-&N\r\n");
  sprintf(buf, "%s&+RTHE JUSTICE OF %s&N.\r\n", buf, town_name_list[town]);
  sprintf(buf, "%s%s, this is what we have against you.\r\n", buf,
          J_NAME(ch));

  while ((crec = crime_find(hometowns[town - 1].crime_list, J_NAME(ch), NULL,
                            0, NOWHERE, J_STATUS_NONE, crec)))
  {

    switch (crec->status)
    {
    case J_STATUS_CRIME:
      crime_index += GET_CRIME_T(town, crec->crime);
      sprintf(buf, "%s  %s against %s\r\n", buf, crime_list[crec->crime],
              crec->victim);
      break;
    case J_STATUS_DEBT:
      crime_index += (crec->money / 100);
      sprintf(buf, "%s  You still owe money for a previous crime\r\n", buf);
      previous_debt += crec->money;
      break;
    case J_STATUS_PARDON:
      crime_index -= (GET_CRIME_T(town, crec->crime) + 10);
      sprintf(buf, "%s  %s pardoned you for %s\r\n", buf, crec->victim,
              crime_list[crec->crime]);
      break;
    case J_STATUS_WANTED:
      if (crec->crime == CRIME_NOT_PAID)
      {
        crime_index += GET_CRIME_T(town, crec->crime);
        sprintf(buf, "%s  You did not pay your debt in time\r\n", buf);
      }
      else
      {
        crime_index += GET_CRIME_T(town, crec->crime);
        sprintf(buf, "%s  You are a wanted criminal\r\n", buf);
      }
      break;
    }
  }

  if (PC_IS_CITIZEN(ch))
  {
    crime_index -= 2;
  }

  crime_index -= GET_PRESTIGE(ch) / 25;

  /* removing all crime rec for this player */

  crec = NULL;
  while ((crec = crime_find(hometowns[town - 1].crime_list, J_NAME(ch), NULL,
                            0, NOWHERE, J_STATUS_NONE, NULL)))
  {
    crime_remove(town, crec);
  }

  sprintf(buf, "%s\r\nThe Judge says '", buf);

  if ((crime_index >= SENTENCE_MIN(town, SENTENCE_NONE)) &&
      (crime_index <= SENTENCE_MAX(town, SENTENCE_NONE)))
  {
    logit(LOG_CRIMES, "%s sentence : free", J_NAME(ch));
    sprintf(buf,
            "%sWe're gonna let you go this time, provided you obey all laws henceforth.'\r\n",
            buf);
    char_from_room(ch);
    char_to_room(ch, real_room(hometowns[town - 1].report_room), -1);
    restoreJailItems(ch);
    act("$n is brought out of jail.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You are brought out of jail.\r\n", ch);
    send_to_char("You get your equipment back.\r\n", ch);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(buf, ch);
    send_to_char("You get your equipment back.\r\n", ch);
    if (previous_debt > 0)
    {
      crime_add(town, J_NAME(ch), town_name_list[town],
                real_room(hometowns[town - 1].report_room), CRIME_NONE,
                time(NULL), J_STATUS_DEBT, previous_debt);
    }

  }
  else if ((crime_index >= SENTENCE_MIN(town, SENTENCE_DEBT)) &&
           (crime_index <= SENTENCE_MAX(town, SENTENCE_DEBT)))
  {

    temp = (crime_index - SENTENCE_MIN(town, SENTENCE_DEBT) + 1) * 50;

    logit(LOG_CRIMES, "%s sentence : pay %d plat.", J_NAME(ch), temp);
    sprintf(buf, "%sYou are ordered to pay %d platinum to the town.'\r\n",
            buf, temp);
    sprintf(buf, "%s&+RYou have 30 days to pay your debt.&N\r\n", buf);
    char_from_room(ch);
    char_to_room(ch, real_room(hometowns[town - 1].report_room), -1);
    restoreJailItems(ch);
    act("$n is brought out of jail.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You are brought out of jail.\r\n", ch);
    send_to_char("You get your equipment back.\r\n", ch);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(buf, ch);
    crime_add(town, J_NAME(ch), town_name_list[town],
              real_room(hometowns[town - 1].report_room), CRIME_NONE,
              time(NULL), J_STATUS_DEBT, temp + previous_debt);
    GET_PRESTIGE(ch) -= 2;

  }
  else if ((crime_index >= SENTENCE_MIN(town, SENTENCE_JAIL)) &&
           (crime_index <= SENTENCE_MAX(town, SENTENCE_JAIL)))
  {

    temp = (crime_index - SENTENCE_MIN(town, SENTENCE_JAIL) + 1);

    logit(LOG_CRIMES, "%s sentence : %d days of jail", J_NAME(ch), temp);
    sprintf(buf, "%sYou are sentenced to %d days of jail.'\r\n", buf, temp);
    char_from_room(ch);
    char_to_room(ch, real_room(hometowns[town - 1].report_room), -1);
    act("$n is brought out of jail.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You are brought out of jail.\r\n", ch);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(buf, ch);
    act("$n is put back in jail.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(hometowns[town - 1].jail_room), -1);
    send_to_char("You are put back in jail.\r\n", ch);
    act("$n is put back in jail.", TRUE, ch, 0, 0, TO_ROOM);
    crime_add(town, J_NAME(ch), town_name_list[town],
              real_room(hometowns[town - 1].jail_room), CRIME_NONE,
              time(NULL) + (temp * JUSTICE_DAY), J_STATUS_JAIL_TIME, temp);
    GET_PRESTIGE(ch) -= 5;

  }
  else if ((crime_index >= SENTENCE_MIN(town, SENTENCE_OUTCAST)) &&
           (crime_index <= SENTENCE_MAX(town, SENTENCE_OUTCAST)))
  {
    logit(LOG_CRIMES, "%s sentence : outcast", J_NAME(ch));
    sprintf(buf, "%sYou are sentenced to be outcast from the town.'\r\n",
            buf);
    char_from_room(ch);
    char_to_room(ch, real_room(hometowns[town - 1].report_room), -1);
    restoreJailItems(ch);
    act("$n is brought out of jail.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You are brought out of jail.\r\n", ch);
    send_to_char("You get your equipment back.\r\n", ch);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(buf, ch);
    send_to_char("You get your equipment back.\r\n", ch);
    act("The Judge utters the words 'power word outcast'.", TRUE, ch, 0, 0,
        TO_ROOM);
    act("$n disappears in a puff of smoke!", TRUE, ch, 0, 0, TO_NOTVICT);
    send_to_char("&+LYou suddenly feel very dizzy...\r\n", ch);
    justice_sentence_outcast(ch, town);
    GET_PRESTIGE(ch) -= 10;

  }
  else if ((crime_index >= SENTENCE_MIN(town, SENTENCE_OUT_NO_EQ)) &&
           (crime_index <= SENTENCE_MAX(town, SENTENCE_OUT_NO_EQ)))
  {
    logit(LOG_CRIMES, "%s sentence : outcast and lost of eq.", J_NAME(ch));
    sprintf(buf,
            "%sYou are sentenced to be outcast from the town and lose everything you have.'\r\n",
            buf);
    char_from_room(ch);
    char_to_room(ch, real_room(hometowns[town - 1].report_room), -1);
    act("$n is brought out of jail.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You are brought out of jail.\r\n", ch);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(buf, ch);
    act("The Judge utters the words 'power word outcast'.", TRUE, ch, 0, 0,
        TO_ROOM);
    act("$n disappears in a puff of smoke!", TRUE, ch, 0, 0, TO_NOTVICT);
    send_to_char("&+LYou suddenly feel very dizzy...\r\n", ch);
    justice_sentence_outcast(ch, town);
    GET_PRESTIGE(ch) -= 15;

  }
  else if ((crime_index >= SENTENCE_MIN(town, SENTENCE_DEATH)) &&
           (crime_index <= SENTENCE_MAX(town, SENTENCE_DEATH)))
  {
    logit(LOG_CRIMES, "%s sentence : DEATH", J_NAME(ch));
    sprintf(buf, "%sYou are sentenced to &+RDEATH&N!.'\r\n", buf);
    char_from_room(ch);
    char_to_room(ch, real_room(hometowns[town - 1].report_room), -1);
    act("$n is brought out of jail.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You are brought out of jail.\r\n", ch);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(buf, ch);
    act("$n is put back in jail.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(hometowns[town - 1].jail_room), -1);
    send_to_char("You are put back in jail.\r\n", ch);
    act("$n is put back in jail.", TRUE, ch, 0, 0, TO_ROOM);
    crime_add(town, J_NAME(ch), town_name_list[town],
              real_room(hometowns[town - 1].jail_room), CRIME_NONE,
              time(NULL), J_STATUS_SENTENCE_DEATH, temp);

    sprintf(buf1, "&+WSomeone shouts '%s has been sentenced to death!'&n\r\n"
            "&+WSomeone shouts 'The execution will be held in three hours at the justice hall!'&n\r\n",
            J_NAME(ch));

    justice_hometown_echo(town, buf1);
    GET_PRESTIGE(ch) -= 20;

  }

  if (GET_PRESTIGE(ch) < 0)
    GET_PRESTIGE(ch) = 0;

  if (GET_STAT(ch) != STAT_DEAD)
    writeCharacter(ch, 7, ch->in_room);
  else
    writeCharacter(ch, 4, NOWHERE);

  return;
}


/* justice main engine */

void justice_engine(int town)
{
  crm_rec *crec = NULL;
  P_char   ch = NULL;
  P_char   tch = NULL, temp = NULL;
  char     buf1[MAX_STRING_LENGTH];
  int      town_tmp;

  /* now lets check if there is any wanted criminal in any town */

  for (town_tmp = 1; town_tmp <= LAST_HOME; town_tmp++)
  {
    if (!hometowns[town_tmp - 1].crime_list)
      continue;
    while ((crec = crime_find(hometowns[town_tmp - 1].crime_list, NULL, NULL,
                              0, NOWHERE, J_STATUS_WANTED, crec)))
    {
      if ((ch = get_char(crec->attacker)))
      {                         /* now we check if he's on   */
        if ((town_tmp == CHAR_IN_TOWN(ch)) &&
            (ch->in_room != real_room(hometowns[town_tmp - 1].jail_room)) &&
            !IS_AFFECTED(ch, AFF_BOUND))
          //justice_send_guards(NOWHERE, ch, MOB_SPEC_ARREST1, 1);  /* go get him boy */
          justice_send_guards(NOWHERE, ch, MOB_SPEC_J_PK, 1);   /* go get him boy */
      }
    }
  }

  if ((hometowns[town - 1].jail_room != 0) &&
      IS_SET(world[real_room(hometowns[town - 1].jail_room)].room_flags,
             JAIL))
  {
    for (tch = world[real_room(hometowns[town - 1].jail_room)].people; tch;
         tch = temp)
    {
      temp = tch->next_in_room;

      if (!
          (crec =
           crime_find(hometowns[town - 1].crime_list, J_NAME(tch), NULL, 0,
                      NOWHERE, J_STATUS_NONE, NULL)))
      {
        if (real_room(hometowns[town - 1].report_room) < 0)
        {
          send_to_char("Strange bug with justice, let a god know..\r\n", tch);
        }
        else
        {
          char_from_room(tch);
          char_to_room(tch, real_room(hometowns[town - 1].report_room), -1);
          act("$n is brought out of jail.", TRUE, tch, 0, 0, TO_ROOM);
          send_to_char("You are brought of jail.\r\n", tch);
          restoreJailItems(tch);
          send_to_char
            ("Lucky you!  It seems we've misplaced your file, so we are setting you free.  Behave or you will end up here again!\r\n",
             tch);
          send_to_char("You get your equipment back.\r\n", tch);
          writeCharacter(tch, 7, tch->in_room);
        }
      }
    }
  }

  crec = NULL;

  if (hometowns[town - 1].crime_list)
  {

    /* ok first we check for execution */

    while ((crec = crime_find(hometowns[town - 1].crime_list, NULL, NULL,
                              0, NOWHERE, J_STATUS_SENTENCE_DEATH, crec)))
    {
      if ((ch = get_char(crec->attacker)))
      {                         /* now we check if he's on   */
        if (ch->in_room == real_room(hometowns[town - 1].jail_room))
        {                       /* ok is he still in jail? */
          char_from_room(ch);
          char_to_room(ch, real_room(hometowns[town - 1].report_room), -1);
          act("$n is brought out of jail.", TRUE, ch, 0, 0, TO_ROOM);
          send_to_char("You are brought of jail.\r\n", ch);
          act("&+LThe executioner&N enters the room.", TRUE, ch, 0, 0,
              TO_ROOM);
          act
            ("&+LThe executioner&N places a &+Mglowing&N dagger in $n's &+Rheart&N.",
             TRUE, ch, 0, 0, TO_NOTVICT);
          act
            ("&+LThe executioner&N places a &+Mglowing&N dagger in your &+Rheart&N.",
             TRUE, ch, 0, 0, TO_VICT);
          sprintf(buf1, "&+WSomeone shouts '%s has been executed!'&n\r\n",
                  J_NAME(ch));
          justice_hometown_echo(town, buf1);
          while ((crec = crime_find(hometowns[town - 1].crime_list,
                                    J_NAME(ch), NULL, 0, NOWHERE,
                                    J_STATUS_NONE, NULL)))
          {
            crime_remove(town, crec);
          }
	  die(ch, ch);
          crec = NULL;
        }
        else
        {                       /* ok he is out of jail */
          deleteJailItems(ch);
          crec->crime = CRIME_NONE;
          crec->status = J_STATUS_DELETED;
          crime_add(town, J_NAME(ch), town_name_list[town],
                    real_room(hometowns[town - 1].report_room), CRIME_ESCAPE,
                    time(NULL), J_STATUS_WANTED, 500);
        }
      }
    }

    /* ok first we check the player that did there time in jail */

    while ((crec = crime_find(hometowns[town - 1].crime_list, NULL, NULL,
                              0, NOWHERE, J_STATUS_JAIL_TIME, crec)))
    {

      if (time(NULL) >= crec->time)
      {                         /* ok the player is free now */
        if ((ch = get_char(crec->attacker)))
        {                       /* now we check if he's on   */
          if (ch->in_room == real_room(hometowns[town - 1].jail_room))
          {                     /* ok is he still in jail? */
            crec->crime = CRIME_NONE;
            crec->status = J_STATUS_DELETED;
            char_from_room(ch);
            char_to_room(ch, real_room(hometowns[town - 1].report_room), -1);
            act("$n is brought out of jail.", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char("You are brought of jail.\r\n", ch);
            restoreJailItems(ch);
            send_to_char
              ("Your time is over.  Behave or you can expect to be back.\r\n",
               ch);
            send_to_char("You get your equipment back.\r\n", ch);
            writeCharacter(ch, 7, ch->in_room);

          }
          else
          {                     /* seem to have escape */
            deleteJailItems(ch);
            crec->crime = CRIME_NONE;
            crec->status = J_STATUS_DELETED;
            crime_add(town, J_NAME(ch), town_name_list[town],
                      real_room(hometowns[town - 1].report_room),
                      CRIME_ESCAPE, time(NULL), J_STATUS_WANTED, 300);
            crec = NULL;
          }
        }
      }
    }

    crec = NULL;

    /* now we check for debt */

    while ((crec = crime_find(hometowns[town - 1].crime_list, NULL, NULL,
                              0, NOWHERE, J_STATUS_DEBT, crec)))
    {

      if ((ch = get_char(crec->attacker)))
      {                         /* now we check if he's on   */
        if (time(NULL) >= (crec->time + (JUSTICE_DAY * 30)))
        {                       /* ok 30 days is up */

          crec->crime = CRIME_NONE;
          crec->status = J_STATUS_DELETED;
          crime_add(town, J_NAME(ch), town_name_list[town],
                    real_room(hometowns[town - 1].report_room),
                    CRIME_NOT_PAID, time(NULL), J_STATUS_WANTED, 200);
          crec = NULL;
        }
      }
    }

    crec = NULL;

    /* now we check the player in jail in wait to be judge for there crimes */

    while ((crec = crime_find(hometowns[town - 1].crime_list, NULL, NULL,
                              0, NOWHERE, J_STATUS_IN_JAIL, crec)))
    {

      if ((ch = get_char(crec->attacker)))
      {                         /* now we check if he's on   */
        if (ch->in_room == real_room(hometowns[town - 1].jail_room))
        {                       /* ok is he still in jail? */

          justice_judge(ch, town);
          crec = NULL;

        }
        else
        {                       /* seem to have escape */
          deleteJailItems(ch);
          crec->crime = CRIME_NONE;
          crec->status = J_STATUS_DELETED;
          crime_add(town, J_NAME(ch), town_name_list[town],
                    real_room(hometowns[town - 1].report_room), CRIME_ESCAPE,
                    time(NULL), J_STATUS_WANTED, 200);
          crec = NULL;
        }
      }
    }

    crec = NULL;

    /* now we remove unwanted record */

    while ((crec = crime_find(hometowns[town - 1].crime_list, NULL, NULL,
                              0, NOWHERE, J_STATUS_DELETED, NULL)))
    {
      crime_remove(town, crec);
    }

    if (!writeTownJustice(town))
      statuslog(55, "Town Justice %d save problem", town);
  }
  return;
}


/* fonction to load the outside room that are under justice patrol */
void load_justice_area(void)
{
  FILE    *fl;
  int      the_end = TRUE, i;
  int      town_number, room_number, room_number1;
  char     type_rec;

  if (!(fl = fopen(JUSTICE_FILE, "r")))
  {
    perror("fopen");
    logit(LOG_FILE, "justice_area: could not open file.");
    logit(LOG_SYS, "justice_area: could not open file.");
    raise(SIGSEGV);
  }

  do
  {

    fscanf(fl, " %c ", &type_rec);
    if (type_rec == 'H')
      fscanf(fl, " %d\n", &town_number);
    else if (type_rec == 'R')
    {
      fscanf(fl, " %d %d\n", &room_number, &room_number1);
      for (i = room_number; i <= room_number1; i++)
        world[real_room0(i)].justice_area = town_number;
    }
    else if (type_rec == 'U')
    {
      fscanf(fl, " %d\n", &room_number);
      world[real_room0(room_number)].justice_area = town_number;
    }
    else if (type_rec == '$')
      the_end = FALSE;

  }
  while (the_end);
  fclose(fl);

  return;
}


/* set new flag for town justice */
void set_town_flag_justice(P_char ch, int start)
{
  int      town;
  int      r_room;

  if (start == TRUE)
  {
    for (town = 1; town <= LAST_HOME; town++)
      PC_SET_TOWN_JUSTICE_FLAGS(ch, JUSTICE_IS_NORMAL, town);

  }
  else
  {
    for (town = 1; town <= LAST_HOME; town++)
    {
      if (PC_TOWN_JUSTICE_FLAGS(ch, town) == JUSTICE_IS_CITIZEN)
        PC_SET_TOWN_JUSTICE_FLAGS(ch, JUSTICE_IS_NORMAL, town);
    }
  }
  r_room = real_room(GET_BIRTHPLACE(ch));
  if (r_room != NOWHERE)
  {
    town = zone_table[world[r_room].zone].hometown;
    if (town > 0)
    {
      PC_SET_TOWN_JUSTICE_FLAGS(ch, JUSTICE_IS_CITIZEN, town);
    }
  }
}

void clean_town_justice()
{
  int      town;
  crm_rec *crec = NULL;

  for (town = 1; town <= LAST_HOME; town++)
  {
    if (!hometowns[town - 1].crime_list)
      continue;

    while ((crec = crime_find(hometowns[town - 1].crime_list, NULL, NULL,
                              0, NOWHERE, J_STATUS_NONE, crec)))
    {
      if (crec->status != J_STATUS_IN_JAIL &&
          crec->status != J_STATUS_JAIL_TIME)
      {
        if (time(NULL) >= (crec->time + (JUSTICE_DAY * 90)))
        {
          crec->crime = CRIME_NONE;
          crec->status = J_STATUS_DELETED;
        }
      }
    }
    while ((crec = crime_find(hometowns[town - 1].crime_list, NULL, NULL,
                              0, NOWHERE, J_STATUS_DELETED, NULL)))
    {
      crime_remove(town, crec);
    }
  }
}

void event_justice_engine(P_char ch, P_char victim, P_obj obj, void *data)
{
  int town = *((int*)data);
  justice_engine(town);
  //AddEvent(EVENT_SPECIAL, 200, TRUE, justice_engine2, NULL);
  if(++town > 15)
    town = 1;
  add_event(event_justice_engine, 200, NULL, NULL, NULL, 0, &town, sizeof(town));
}

