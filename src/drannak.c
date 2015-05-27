/****************************************************************************
 *
 *  File: drannak.c                                           Part of Duris
 *  Usage: drannak.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Drannak                   Date: 2013-05-29
 * ***************************************************************************
 */

#define TROPHY

#include <stdio.h>
#include <string.h>
#include <math.h>


#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "mm.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "weather.h"
#include "sound.h"
#include "objmisc.h"
#include "tradeskill.h"
#include "map.h"
#include "specs.prototypes.h"
#include "ships.h"
#include "achievements.h"

/*
 * external variables
 */
extern Skill skills[];
extern struct zone_data *zone_table;
extern const char *material_names[];
extern P_char character_list;
extern P_desc descriptor_list;
extern P_event event_type_list[];
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern char debug_mode;
extern const char *race_types[];


extern const struct stat_data stat_factor[];
extern float fake_sqrt_table[];
extern int pulse;
extern int arena_hometown_location[];
extern struct arena_data arena;
extern struct agi_app_type agi_app[];
extern struct dex_app_type dex_app[];
extern struct message_list fight_messages[];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern P_obj quest_item_reward(P_char ch);
extern int find_map_place();
extern int getItemFromZone(int zone);

void     set_short_description(P_obj t_obj, const char *newShort);
void     set_keywords(P_obj t_obj, const char *newKeys);
void     set_long_description(P_obj t_obj, const char *newDescription);

void set_surname(P_char ch, int num)
{
  /* Surname List
     0 - Leaderboard Based (default)
     1 - User Toggled Custom Surname Off
     2 - Lightbringer
     3 - Dragonslayer
     4 - Decepticon
     */

  if( !IS_ALIVE(ch) )
  {
    return;
  }

  if((num == 0 && !IS_SET(ch->specials.act3, PLR3_NOSUR)) || num == 1 )
  {
    int points = getLeaderBoardPts(ch);
    points *= .01;
    //debug("points: %d", points);
    clear_surname(ch);

    if( IS_TRUSTED( ch ) )
    {
      SET_BIT(ch->specials.act3, PLR3_SURKING);
      return;
    }

    if(points < 200)
    {
      SET_BIT(ch->specials.act3, PLR3_SURSERF);
      return;
    }

    if(points < 500)
    {
      SET_BIT(ch->specials.act3, PLR3_SURCOMMONER);
      return;
    }

    if(points < 1500)
    {
      SET_BIT(ch->specials.act3, PLR3_SURKNIGHT);
      return;
    }

    if(points < 2800)
    {
      SET_BIT(ch->specials.act3, PLR3_SURNOBLE);
      return;
    }

    if(points < 4000)
    {
      SET_BIT(ch->specials.act3, PLR3_SURLORD);
      return;
    }

    if(points >= 4000)
    {
      SET_BIT(ch->specials.act3, PLR3_SURKING);
      return;
    }
  }

  if (num == 2)
  {
    if(affected_by_spell(ch, ACH_YOUSTRAHDME))
    {
      clear_surname(ch);
      SET_BIT(ch->specials.act3, PLR3_SURLIGHT);
      SET_BIT(ch->specials.act3, PLR3_NOSUR);
      return;
    }
    send_to_char("You have not obtained that &+Wtitle&n yet.\r\n", ch);
  }

  if (num == 3)
  {
    if(affected_by_spell(ch, ACH_DRAGONSLAYER))
    {
      clear_surname(ch);
      SET_BIT(ch->specials.act3, PLR3_SURDRAGON);
      SET_BIT(ch->specials.act3, PLR3_NOSUR);
      return;
    }
    send_to_char("You have not obtained that &+Wtitle&n yet.\r\n", ch);
  }

  if (num == 4)
  {
    if(affected_by_spell(ch, ACH_MAYIHEALSYOU))
    {
      clear_surname(ch);
      SET_BIT(ch->specials.act3, PLR3_SURHEALS);
      SET_BIT(ch->specials.act3, PLR3_NOSUR);
      return;
    }
    send_to_char("You have not obtained that &+Wtitle&n yet.\r\n", ch);
  }
  if (num == 5)
  {
    if(affected_by_spell(ch, ACH_SERIALKILLER))
    {
      clear_surname(ch);
      SET_BIT(ch->specials.act3, PLR3_SURSERIAL);
      SET_BIT(ch->specials.act3, PLR3_NOSUR);
      return;
    }
    send_to_char("You have not obtained that &+Wtitle&n yet.\r\n", ch);
  }
  if (num == 6)
  {
    if(get_frags(ch) >= 2000)
    {
      clear_surname(ch);
      SET_BIT(ch->specials.act3, PLR3_SURREAPER);
      SET_BIT(ch->specials.act3, PLR3_NOSUR);
      return;
    }
    send_to_char("You have not obtained that &+Wtitle&n yet.\r\n", ch);
  }
  if (num == 7)
  {
    if(affected_by_spell(ch, ACH_DECEPTICON))
    {
      clear_surname(ch);
      SET_BIT(ch->specials.act3, PLR3_SURDECEPTICON);
      SET_BIT(ch->specials.act3, PLR3_NOSUR);
      return;
    }
    send_to_char("You have not obtained that &+Wtitle&n yet.\r\n", ch);
  }
  if (num == 8)
  {
    if(affected_by_spell(ch, ACH_DEATHSDOOR))
    {
      clear_surname(ch);
      SET_BIT(ch->specials.act3, PLR3_SURDEATHSDOOR);
      SET_BIT(ch->specials.act3, PLR3_NOSUR);
      return;
    }
    send_to_char("You have not obtained that &+Wtitle&n yet.\r\n", ch);
  }
}

void display_surnames(P_char ch)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];

  sprintf(buf, "\r\n&+L=-=-=-=-=-=-=-=-=-=--= &+rTitles &+Lfor &+r%s &+L=-=-=-=-=-=-=-=-=-=-=-&n\r\n", GET_NAME(ch));


  sprintf(buf2, "   &+W%-22s\r\n",
      "Syntax: toggle surname <number>");
  strcat(buf, buf2);
  sprintf(buf2, "   &+L%-49s\r\n",
      "&+W1)&+WFeudal Rank &n(default)&n");
  strcat(buf, buf2);

  if(affected_by_spell(ch, ACH_YOUSTRAHDME))
  {
    sprintf(buf2, "   &+L%-49s\r\n",
        "&+W2)&+WLight&+wbri&+Lnger&n");
    strcat(buf, buf2);
  }

  if(affected_by_spell(ch, ACH_DRAGONSLAYER))
  {
    sprintf(buf2, "   &+L%-49s\r\n",
        "&+W3)&+gDr&+Gag&+Lon &+gS&+Glaye&+gr&n");
    strcat(buf, buf2);
  }

  if(affected_by_spell(ch, ACH_MAYIHEALSYOU))
  {
    sprintf(buf2, "   &+L%-49s\r\n",
        "&+W4)&+WD&+Ro&+rct&+Ro&+Wr&n");
    strcat(buf, buf2);
  }

  if(affected_by_spell(ch, ACH_SERIALKILLER))
  {
    sprintf(buf2, "   &+L%-49s\r\n",
        "&+W5)&+LSe&+wr&+Wi&+wa&+Ll &+rKiller&n");
    strcat(buf, buf2);
  }

  if(get_frags(ch) >= 2000)
  {
    sprintf(buf2, "   &+L%-49s\r\n",
        "&+W6)&+LGrim Reaper&n");
    strcat(buf, buf2);
  }

  if(affected_by_spell(ch, ACH_DECEPTICON))
  {
    sprintf(buf2, "   &+L%-49s\r\n",
        "&+W7)&+LDe&+mceptic&+LoN&n");
    strcat(buf, buf2);
  }

  if(affected_by_spell(ch, ACH_DEATHSDOOR))
  {
    sprintf(buf2, "   &+L%-49s\r\n",
        "&+W8)&+MTo&+mug&+Mh G&+muy&n");
    strcat(buf, buf2);
  }

  sprintf(buf2, "\r\n   &+W%-22s\r\n",
      "Note: &nSome &+cachievements&n grant access to additional surnames&n\r\n");
  strcat(buf, buf2);


  page_string(ch->desc, buf, 1);
}

bool quested_spell(P_char ch, int spl)
{

  //debug("spell: %d\r\n");
  if(IS_NPC(ch))
    return FALSE;

  /*if(spl == 56)
    return TRUE;*/

  return FALSE;
}


void clear_surname(P_char ch)
{
  REMOVE_BIT(ch->specials.act3, PLR3_SURLIGHT);
  REMOVE_BIT(ch->specials.act3, PLR3_SURSERF);
  REMOVE_BIT(ch->specials.act3, PLR3_SURCOMMONER);
  REMOVE_BIT(ch->specials.act3, PLR3_SURKNIGHT);
  REMOVE_BIT(ch->specials.act3, PLR3_SURNOBLE);
  REMOVE_BIT(ch->specials.act3, PLR3_SURLORD);
  REMOVE_BIT(ch->specials.act3, PLR3_SURKING);
  REMOVE_BIT(ch->specials.act3, PLR3_SURDRAGON);
  REMOVE_BIT(ch->specials.act3, PLR3_SURHEALS);
  REMOVE_BIT(ch->specials.act3, PLR3_SURSERIAL);
  REMOVE_BIT(ch->specials.act3, PLR3_SURREAPER);
  REMOVE_BIT(ch->specials.act3, PLR3_SURDECEPTICON);
  REMOVE_BIT(ch->specials.act3, PLR3_SURDEATHSDOOR);
  REMOVE_BIT(ch->specials.act3, PLR3_NOSUR);
}

void vnum_from_inv(P_char ch, int item, int count)
{
  int i = count;
  P_obj t_obj, nextobj;

  int checkit = vnum_in_inv(ch, item);
  if (checkit < count)
  {
    send_to_char("You don't have enough of that item in your inventory.\r\n", ch);
    return;
  }

  for (t_obj = ch->carrying; t_obj; t_obj = nextobj)
  {
    nextobj = t_obj->next_content;

    if((GET_OBJ_VNUM(t_obj) == item) && (i > 0) )
    {
      obj_from_char(t_obj, TRUE);
      extract_obj(t_obj, TRUE);
      i--;
    }

  }
}

int vnum_in_inv(P_char ch, int cmd)
{
  P_obj t_obj, nextobj;
  int count = 0;
  for (t_obj = ch->carrying; t_obj; t_obj = nextobj)
  {
    nextobj = t_obj->next_content;

    if(GET_OBJ_VNUM(t_obj) == cmd)
      count++;
  }
  return count;
}

int pvp_store(P_char ch, P_char pl, int cmd, char *arg)
{
  char buffer[MAX_STRING_LENGTH];
  char     buf[256], *buff;
  char     Gbuf1[MAX_STRING_LENGTH], *c;

  if(cmd == CMD_LIST)
  {//iflist
    if(!arg || !*arg)
    {//ifnoarg
      sprintf(buffer,
          "&+LThe Harvester&+L fills your mind with words...'\n"
          "&+LThe Harvester&+L &+wsays 'Welcome combatant. I offer exotic items to those who have &+rproven &+Lthemselves in the arts of mortal &+rcombat&+L.&n'\n"
          "&+LThe Harvester&+L &+wsays 'Only those who have collected the necessary amount frags of may purchase these items.&+L.&n'\n"
          "&+LThe Harvester&+L &+wsays 'Additionally, I offer a reward for &+R6 &+we&+Wt&+Lh&+rer&+Le&+Wa&+wl &+Wsoul &+rshards&n from beings who have fallen in battle&n.&+L.&n'\n"
          "&+LThe Harvester&+L &+wsays 'Simply have them in your &+Winventory&n and buy the item from the list below&n.&+L.&n'\n"
          "&+y=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"
          "&+y|		&+cItem Name					           Frags Required       &+y|\n"																
          "&+y|&+W 1) &+Ya &+Mgreater&+Y o&+Mr&+Bb &+Yof &+mM&+Ma&+Wg&+Mi&+mc&n&+C%30d&n		                        &+y|\n"
          "&+y=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"
          "\n", 0);
      send_to_char(buffer, pl);
      return TRUE;
    }//endifnoarg
  }//endiflist
  else if(cmd == CMD_BUY)
  {
    if(!arg || !*arg)
    {//ifnoarg
      // practice called with no arguments
      sprintf(buffer,
          "&+LThe Harvester&+L &+wsays 'What item would you like to buy?'\n");

      send_to_char(buffer, pl);
      return TRUE;
    }//endifnoarg

    else if(strstr(arg, "1"))
    {//buy1
      //check for 50 soul shards
      if (vnum_in_inv(pl, 400230) < 2)
      {
        send_to_char("&+LThe Harvester&+L &+wsays '&nI'm sorry, but you do not seem to have the 6 e&+Wt&+Lh&+rer&+Le&+Wa&+wl &+Wsoul &+rshards&n required to purchase that item.\r\n&n", pl);
        return TRUE;
      }
      //subtract 2 soul shards
      P_obj obj;
      obj = read_object(400231, VIRTUAL);
      vnum_from_inv(pl, 400230, 2);
      send_to_char("&+LThe Harvester&+L &+wsays '&nExcellent, mortal.'\n", pl);
      send_to_char("&+LThe Harvester &ntakes the &+rshards&n from you and tightly grasps them with his hands. After a moment, a large grin appears across it's face.\r\n&n", pl);
      send_to_char("Moments later, &+LThe Harvester &nmakes a strange gesture about your body.\r\n&n", pl);
      act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
      extract_obj(obj, FALSE);
      obj_to_char(read_object(400231, VIRTUAL), pl);
      return TRUE;
    }//endbuy1

  }
  return FALSE;
}

bool lightbringer_weapon_proc(P_char ch, P_char victim)
{
  int num, room = ch->in_room, save, pos;
  P_obj wpn;

  typedef void (*spell_func_type) (int, P_char, char *, int, P_char, P_obj);
  spell_func_type spells[5] = {
    spell_bigbys_crushing_hand,
    spell_bigbys_clenched_fist,
    spell_disintegrate,
    spell_destroy_undead,
    spell_flamestrike
  };
  spell_func_type spell_func;

  if (!IS_FIGHTING(ch) ||
      !(victim = ch->specials.fighting) ||
      !IS_ALIVE(victim) ||
      !(room) ||
      number(0, 15)) // 3%
    return false;

  P_char vict = victim;

  for (wpn = NULL, pos = 0; pos < MAX_WEAR; pos++)
  {
    if((wpn = ch->equipment[pos]) &&
        wpn->type == ITEM_WEAPON &&
        CAN_SEE_OBJ(ch, wpn))
      break;
  }

  if(wpn == NULL)
    return false;



  act("&+LAs you strike your &+rfoe&+L, the power of the &+WLight&+wbri&+Lngers fill you with &+wundead &+Lpurging &+Ymight&+L!&n",
      TRUE, ch, wpn, vict, TO_CHAR);
  act("&+LAs $n strikes their &+rfoe&+L, the power of the &+WLight&+wbri&+Lngers fill them with &+wundead &+Lpurging &+Ymight&+L!&n",
      TRUE, ch, wpn, vict, TO_NOTVICT);


  num = number(0, 4);

  spell_func = spells[num];

  spell_func(number(1, GET_LEVEL(ch)), ch, 0, 0, victim, 0);
  /*
     return !is_char_in_room(ch, room) || !is_char_in_room(victim, room);
     victim->specials.apply_saving_throw[SAVING_SPELL] = save;
     */
}

// The 'merc' is the mercenary being hit on by the 'hitter'.
bool intercept_defensiveproc(P_char merc, P_char hitter)
{
  int num, room, save, pos;

  // If !( both are alive and hitter hitting merc )
  if( !IS_ALIVE(hitter) || !IS_FIGHTING(hitter) || !(merc == hitter->specials.fighting)
    || !IS_ALIVE(merc) || !(room = hitter->in_room) || !has_innate( merc, INNATE_INTERCEPT) )
  {
    return FALSE;
  }

  // If hitter already affected by armlock..
  if(affected_by_spell(hitter, TAG_INTERCEPT))
  {
    return FALSE;
  }

  // If merc not in correct position to defend.
  if(!MIN_POS(merc, POS_STANDING + STAT_NORMAL))
  {
    return FALSE;
  }

  int num1 = number(1, GET_C_LUK(merc));
  int num2 = number(1, 800);

  // Approx 1/8 chance for 100 luck, but really random.
//  debug("intercept_defensiveproc: merc: '%s', num1: %d, hitter: '%s', num2: %d", J_NAME(merc), num1, J_NAME(hitter), num2);

  if( num1 < num2 )
  {
    return FALSE;
  }

  struct affected_type af;


  act("&+LAs $n&+L attempts to attack you, you &+Cintercept&+L the attack with your &+yhands&+L and &+ytwist&n $n's arm!&n",
      TRUE, hitter, 0, merc, TO_VICT);
  act("&+LAs $n&+L attempts to attack $n, $N &+Cintercepts&+L the attack with their &+yhands&+L and &+ytwist&n $n's arm!&n",
      TRUE, hitter, 0, merc, TO_NOTVICT);
  act("&+LAs you attempt to attack $N, they quickly reach out, &+Cintercepting&+L the attack with their &+yhands&+L and quickly &+ytwist&n your arm!&n",
      TRUE, hitter, 0, merc, TO_CHAR);

  memset(&af, 0, sizeof(af));
  af.type = TAG_INTERCEPT;
  af.duration = 100;
  af.flags = AFFTYPE_SHORT;
  affect_to_char(hitter, &af);

  return TRUE;
}

bool minotaur_race_proc(P_char ch, P_char victim)
{
  int num, room = ch->in_room, save, pos, cmd;
  int class_chance = 0;

  switch(ch->player.m_class)
  {
    case CLASS_REAVER:
    case CLASS_RANGER:
      class_chance = 30;
      break;
    case CLASS_MONK:
      class_chance = 20;
      break;
    case CLASS_SORCERER:
    case CLASS_CONJURER:
    case CLASS_SUMMONER:
      class_chance = 6;
      break;
    case CLASS_WARRIOR:
      class_chance = 10;
      break;
    default:
      class_chance = 15;
      break;
  }

  if (!IS_FIGHTING(ch) ||
      !(victim = ch->specials.fighting) ||
      !IS_ALIVE(victim) ||
      !(room) ||
      number(0, class_chance)) // 3%(15)
        return false; 

  struct affected_type af;


  act("&+LAs you strike $N&+L, the power of your &+rance&+Lstor&+rs&+L fill you with &+rR&+RAG&+RE&+L!&n",
      TRUE, ch, 0, victim, TO_CHAR);
  act("&+LAs $n strikes $N&+L, the power of $n's &+rance&+Lstor&+rs&+L fill them with &+rR&+RAG&+RE&+L!&n",
      TRUE, ch, 0, victim, TO_NOTVICT);

  if(affected_by_spell_count(ch, TAG_MINOTAUR_RAGE) < 6)
  {
    memset(&af, 0, sizeof(af));
    af.type = TAG_MINOTAUR_RAGE;
    af.duration = 130;
    af.flags = AFFTYPE_SHORT;
    affect_to_char(ch, &af);
  }

  memset(&af, 0, sizeof(af));
  af.type = TAG_INNATE_TIMER;
  af.location = APPLY_COMBAT_PULSE;
  af.modifier = -1;
  af.duration = 130;
  af.flags = AFFTYPE_SHORT;
  affect_to_char(ch, &af);

  memset(&af, 0, sizeof(af));
  af.type = TAG_INNATE_TIMER;
  af.location = APPLY_SPELL_PULSE;
  af.modifier = -1;
  af.duration = 130;
  af.flags = AFFTYPE_SHORT;
  affect_to_char(ch, &af);


}

static FILE *aliaslist;

char get_alias(P_char ch, char *argument)
{

  char     buf[256], aliasword[MAX_STRING_LENGTH], rbuf[MAX_STRING_LENGTH], *bufx;
  char     gbuf1[MAX_STRING_LENGTH], charalias[MAX_STRING_LENGTH], bufbug[256];
  FILE *aliaslist;

  if(!str_cmp(argument, ""))
  {
    send_to_char("No arg provided\r\n", ch);
    return FALSE;
  }

  strcpy(buf, GET_NAME(ch));
  bufx = buf;
  for (; *bufx; bufx++)
    *bufx = LOWER(*bufx);
  sprintf(gbuf1, "%s/%c/%s.aliases", SAVE_DIR, buf[0], buf);

  aliaslist = fopen(gbuf1, "rt");
  if(!aliaslist)
  {
    create_alias_name(GET_NAME(ch));
    aliaslist = fopen(gbuf1, "rt");
  }
  if(!aliaslist)
  {
    send_to_char("error reading alias file\r\n", ch);
    return FALSE;
  }

  //see if the alias exists.
  while((fscanf(aliaslist, "%s", charalias) != EOF))
  {
    int i = 0;
    sprintf(bufbug, "%s", charalias);
    int times = 0;
    char buffer[MAX_STRING_LENGTH] = "";

    while(times < 1)
    {
      sprintf(rbuf, "%c", bufbug[i]);
      if(!strstr(rbuf, "("))
      {
        strcat(buffer, rbuf);
      }
      else
        times++;
      i++;
    }
    //if valid alias
    if(!str_cmp(buffer, argument))
    {
      char bfbug[256];
      char bfr[MAX_STRING_LENGTH] = "";
      char ruf[MAX_STRING_LENGTH];
      int i = 0;
      int times = 0;

      sprintf(bfbug, "%s", charalias);

      while(times < 2)
      {
        sprintf(ruf, "%c", bfbug[i]);
        if(strstr(ruf, "(") || strstr(ruf, ")"))
          times++;

        if(times > 0)
        {
          if(!strstr(ruf, "(") && !strstr(ruf, ")") )
          {
            strcat(bfr, ruf);
          }

        }
        i++;
      }
      send_to_char("Valid Alias!\r\n", ch);
      send_to_char(bfr, ch);
    }
    // end valid

    //make sure to close aliaslist
  }
  fclose(aliaslist);
  return TRUE;
}

void create_alias_file(const char *dir, char *name)
{
  char     buf[256], *buff;
  char     Gbuf1[MAX_STRING_LENGTH];
  FILE    *f;

  strcpy(buf, name);
  buff = buf;
  for (; *buff; buff++)
    *buff = LOWER(*buff);
  sprintf(Gbuf1, "%s/%c/%s.aliases", dir, buf[0], buf);
  f = fopen(Gbuf1, "w");
  fclose(f);
}

void create_alias_name(char *name)
{
  create_alias_file("Players", name);
}

void newbie_reincarnate(P_char ch)
{
  // checks to see if the character is much lower than the killer

  P_char   t, t_next;


  wizlog(MINLVLIMMORTAL, "%s returned to the birthplace.", GET_NAME(ch));
  char_from_room(ch);
  char_to_room(ch, real_room(GET_BIRTHPLACE(ch)), -1);


  if (CAN_SPEAK(ch))
  {
    death_cry(ch);
  }

  if (ch->specials.fighting)
    stop_fighting(ch);
  if( IS_DESTROYING(ch) )
    stop_destroying(ch);

  for (t = world[ch->in_room].people; t; t = t_next)
  {
    t_next = t->next_in_room;
    if (t->specials.fighting == ch)
    {
      stop_fighting(t);
      clearMemory(t);
    }
  }

  GET_HIT(ch) = BOUNDED(10, GET_MAX_HIT(ch), 100);
  CharWait(ch, dice(2, 2) * 4);
  update_pos(ch);

  act
    ("&+CAs $n's body succumbs to the overwhelming &+rpain&+C, &+M$n &+Cis quickly &+cwhisked &+Caway by a &+Ldark, sh&+wad&+Wowy &+Lfigure&+C!&N",
     TRUE, ch, 0, 0, TO_ROOM);
  act
    ("&+cAs your &+Cbody&+c succumbs to the overwhelming &+rpain&+c, a &+Ldark, sh&+wad&+Wowy &+Lfigure&+c appears from no where and quickly whisks you from the &+rviolence&+c. After opening your eyes you discover that the worst of your wounds are &+Whealed&+c, and you are once again in &+Cfamiliar &+clands!&N",
     FALSE, ch, 0, 0, TO_CHAR);
  send_to_char("&+cA voice whispers to you: &+L'Beware, this time you have been spared death due to the supreme &+rpowers&+L of your adversary, but the battle lines draw ever closer each day...'&n", ch);

}


int equipped_value(P_char ch)
{
  P_obj    obj_object, temp_obj;
  int      total, k, ret_type;
  bool     was_invis, naked;
  char     Gbuf1[MAX_STRING_LENGTH];
  struct affected_type af;

  total = 0;

  for (k = 0; k < MAX_WEAR; k++)
  {
    temp_obj = ch->equipment[k];
    if(temp_obj)
      total += itemvalue(ch, temp_obj);
  }
  return total;
}

void create_recipe(P_char ch, P_obj temp)
{
  /***RECIPE CREATE***/
  P_obj objrecipe;
  char buffer[256], old_name[256];
  char *c;
  int recipenumber = obj_index[temp->R_num].virtual_number;

  if ((temp->type == ITEM_CONTAINER ||
        temp->type == ITEM_STORAGE) && temp->contains)
  {
    return;
  }

  // No recipes for high end equipment.
  if( IS_SET(temp->bitvector, (AFF_STONE_SKIN | AFF_BIOFEEDBACK | AFF_SNEAK | AFF_HIDE ))
    || IS_SET(temp->bitvector2, ( AFF2_EARTH_AURA | AFF2_WATER_AURA | AFF2_FIRE_AURA | AFF2_AIR_AURA | AFF2_FLURRY ))
    || IS_SET(temp->bitvector3, ( AFF3_ENLARGE | AFF3_REDUCE | AFF3_INERTIAL_BARRIER | AFF3_BLUR ))
    || IS_SET(temp->bitvector4, ( AFF4_VAMPIRE_FORM | AFF4_HOLY_SACRIFICE | AFF4_BATTLE_ECSTASY | AFF4_SANCTUARY | AFF4_HELLFIRE | AFF4_ICE_AURA | AFF4_WILDMAGIC )) )
  {
    return;
  }
  for( int i = 0;i < MAX_OBJ_AFFECT;i++ )
  {
    if( (temp->affected[i].location == APPLY_COMBAT_PULSE)
      || (temp->affected[i].location == APPLY_SPELL_PULSE) )
    {
      return;
    }
  }

  if(!is_salvageable(temp))
    return;

  objrecipe = read_object(400210, VIRTUAL);
  SET_BIT(objrecipe->value[6], recipenumber);
  strcpy(old_name, objrecipe->short_description);
  sprintf(buffer, "%s %s&n", old_name, temp->short_description);

  if( (objrecipe->str_mask & STRUNG_DESC2) && objrecipe->short_description )
    FREE( objrecipe->short_description );

  objrecipe->short_description = str_dup(buffer);

  objrecipe->str_mask |= STRUNG_DESC2;
  debug( "create_recipe: %s reward was: %s ival: %d.", J_NAME(ch), objrecipe->short_description, itemvalue(ch, temp) );
  obj_to_char( objrecipe, ch );
}

void random_recipe(P_char ch, P_char victim)
{
  int chance = 1;

  if(IS_PC(victim))
    return;

  if(IS_PC_PET(victim))
    return;

  chance += GET_LEVEL(victim) * 2;

  if(IS_ELITE(victim))
    chance *= 5;

  chance += GET_C_LUK(ch)/2;

  int result = number(1, 20000);

  if( result < chance )
  {
    P_obj reward = random_zone_item(ch);
    if(obj_index[reward->R_num].virtual_number == 1252 ||
        obj_index[reward->R_num].virtual_number == 1253 ||
        obj_index[reward->R_num].virtual_number == 1254 )
    {
      extract_obj(reward, !IS_TRUSTED(ch));
      return;
    }
    create_recipe(victim, reward);
    extract_obj(reward, TRUE);
  }

  return;

}

void randomizeitem(P_char ch, P_obj obj)
{
  int   i = 0, good = 0, workingvalue, range, value, limit, luckroll, rchance;
  P_obj t_obj, nextobj;
  bool  modified = FALSE;
  char  tempdesc [MAX_INPUT_LENGTH];
  char  short_desc[MAX_STRING_LENGTH], emsg[MAX_STRING_LENGTH];

  // Changed 2 to 1, don't need uber randomization.
  while( i < 1 )
  {
    //no randomization of items with combat/spell pulse
    if( obj->affected[i].location == APPLY_COMBAT_PULSE || obj->affected[i].location == APPLY_SPELL_PULSE )
    {
      return;
    }

    //Get current a0/1 value
    if(obj->affected[i].location != 0)
    {
      //debug("obj->affected[i].location: %d\r\n", obj->affected[i].location);
      luckroll = (number(1, 110));

      //initialize workingvalue
      workingvalue = obj->affected[i].modifier; //base value
      //send_to_char("Item has been randomized\r\n", ch);

      // This prevents objects from breaking the over 10 difference report-to-Imms.
      if( workingvalue > 10 )
        range = 3;
      else if( workingvalue == 10 )
        range = 0;
      else if( workingvalue == 9 || workingvalue == 8 )
        range = 1;
      else if( workingvalue >= 5 )
        range = 2;
      else if( workingvalue >= 1 )
        range = 1;
      else if( workingvalue < -10 )
        range = -3;
      else if( workingvalue == -10 )
        range = 0;
      else if( workingvalue == -9 || workingvalue == -8 )
        range = -1;
      else if( workingvalue <= -5 )
        range = -2;
      else if( workingvalue <= -1 )
        range = -1;
      // Otherwise, workingvalue is 0.
      else
        range = 1;

      limit = range * -1;
      // Check for negative values
      if( range < 0 )
      {
        // Assuming a negative stat is good here, although not always.
        if( luckroll > 90 && range < -1)
        {
          range--;
          limit--;
        }
        value = (number(range, limit));
        if( value != 0 )
        {
          if( value < 0 )
          {
            good++;
          }
          else
          {
            good--;
          }

          // Something happened, but might zero out.
          modified = TRUE;
          value += workingvalue;
          obj->affected[i].modifier = value;
        }
      }
      else if( range == 0 )
      {
        ;
      }
      else
      {
        if( luckroll > 90 && range > 1 )
        {
          range++;
          limit++;
        }
        value = (number(limit, range));
        if( value != 0 )
        {
          if (value > 0)
          {
            good++;
          }
          else
          {
            good--;
          }

          // Something happened, but might zero out.
          modified = TRUE;
          value += workingvalue;
          obj->affected[i].modifier = value;
        }
      }
    }
    i++;
  }

  emsg[0] = '\0';
  for( t_obj = ch->carrying; t_obj; t_obj = nextobj )
  {
    nextobj = t_obj->next_content;

    if( GET_OBJ_VNUM(t_obj) == 1250 )
    {
      rchance = (number(1, 100));
      if( isname("cross", t_obj->name) )
      {
        if( rchance < 5 )
        {
          SET_BIT(obj->bitvector, AFF_SNEAK);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the Ass&+rass&+Rin&n");
        }
        else if( rchance < 25 )
        {
          SET_BIT(obj->bitvector3, AFF3_COLDSHIELD);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+Bc&+Chi&+Wll&+Cin&+Bg&n");
        }
        else if( rchance < 76 )
        {
          SET_BIT(obj->bitvector, AFF_FARSEE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+Rsight&n");
        }
        else
        {
          SET_BIT(obj->bitvector2, AFF2_PROT_COLD);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+Ccold &+Wprotection&n");
        }
        obj_from_char(t_obj, TRUE);
        extract_obj(t_obj, TRUE);
        break;
      }
      else if( isname("bloodstone", t_obj->name) )
      {
        if( rchance < 5 )
        {
          SET_BIT(obj->bitvector4, AFF4_DAZZLER);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+bd&+Baz&+Wzl&+Bin&+bg&n");
        }
        else if( rchance < 25 )
        {
          SET_BIT(obj->bitvector, AFF_HASTE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+rs&+Rp&+re&+Re&+rd&n");
        }
        else if( rchance < 76 )
        {
          SET_BIT(obj->bitvector, AFF_PROTECT_EVIL);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof evil &+Wprotection&n");
        }
        else
        {
          SET_BIT(obj->bitvector, AFF_PROT_FIRE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+rfire &+Wprotection&n");
        }
        obj_from_char(t_obj, TRUE);
        extract_obj(t_obj, TRUE);
        break;
      }
      else if( isname("black", t_obj->name) )
      {
        if( rchance < 5 )
        {
          SET_BIT(obj->bitvector4, AFF4_NOFEAR);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the Se&+yn&+Yti&+yn&+Lel&n");
        }
        else if( rchance < 25 )
        {
          SET_BIT(obj->bitvector4, AFF4_GLOBE_OF_DARKNESS);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+Wda&+wrk&+Lness&n");
        }
        else if( rchance < 76 )
        {
          SET_BIT(obj->bitvector, AFF_PROTECT_GOOD);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+Ygood &+Wprotection&n");
        }
        else
        {
          SET_BIT(obj->bitvector, AFF_MINOR_GLOBE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+ylesser &+Yde&+yfen&+Lse&n");
        }
        obj_from_char(t_obj, TRUE);
        extract_obj(t_obj, TRUE);
        break;
      }
      else if( isname("pink", t_obj->name) )
      {
        if( rchance < 5 )
        {
          SET_BIT(obj->bitvector4, AFF4_REGENERATION);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the &+gTroll&n");
        }
        else if( rchance < 25 )
        {
          SET_BIT(obj->bitvector, AFF_FLY);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the &+WAn&+Cge&+cls&n");
        }
        else if( rchance < 76 )
        {
          SET_BIT(obj->bitvector2, AFF2_FIRESHIELD);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+rbu&+Rrn&+Ying&n");
        }
        else
        {
          SET_BIT(obj->bitvector, AFF_MINOR_GLOBE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+ylesser &+Yde&+yfen&+Lse&n");
        }
        obj_from_char(t_obj, TRUE);
        extract_obj(t_obj, TRUE);
        break;
      }
      else if( isname("rubin", t_obj->name) )
      {
        if( rchance < 5 )
        {
          SET_BIT(obj->bitvector4, AFF4_DETECT_ILLUSION);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the &+CIl&+clu&+Wsi&+con&+Cist&n");
        }
        else if( rchance < 25 )
        {
          SET_BIT(obj->bitvector, AFF_DETECT_INVISIBLE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+Cvi&+csi&+Won&n");
        }
        else if( rchance < 76 )
        {
          SET_BIT(obj->bitvector, AFF_SENSE_LIFE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+rlife &+Lsensing&n");
        }
        else
        {
          SET_BIT(obj->bitvector, AFF2_DETECT_EVIL);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+revil &+Ldetection&n");
        }
        obj_from_char(t_obj, TRUE);
        extract_obj(t_obj, TRUE);
        break;
      }
      else if( isname("green", t_obj->name) )
      {
        if( rchance < 5 )
        {
          SET_BIT(obj->bitvector3, AFF3_PASS_WITHOUT_TRACE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the &+GRanger&n");
        }
        else if( rchance < 25 )
        {
          SET_BIT(obj->bitvector, AFF_INVISIBLE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof Invi&+Wsibi&+Llity&n");
        }
        else if( rchance < 76 )
        {
          SET_BIT(obj->bitvector2, AFF2_PROT_GAS);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+ggas &+Ldefense&n");
        }
        else
        {
          SET_BIT(obj->bitvector, AFF2_DETECT_GOOD);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+Wgood &+Ldetection&n");
        }
        obj_from_char(t_obj, TRUE);
        extract_obj(t_obj, TRUE);
        break;
      }
      else if( isname("red", t_obj->name) )
      {
        if( rchance < 5 )
        {
          SET_BIT(obj->bitvector3, AFF3_TOWER_IRON_WILL);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the &+MIllithid&n");
        }
        else if( rchance < 25 )
        {
          SET_BIT(obj->bitvector2, AFF2_VAMPIRIC_TOUCH);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the &+rVa&+Rmp&+Lire&n");
        }
        else if( rchance < 76 )
        {
          SET_BIT(obj->bitvector2, AFF2_PROT_ACID);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+Gacid &+Ldefense&n");
        }
        else
        {
          SET_BIT(obj->bitvector2, AFF2_DETECT_MAGIC);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+mmagic &+Ldetection&n");
        }
        obj_from_char(t_obj, TRUE);
        extract_obj(t_obj, TRUE);
        break;
      }
      else if( isname("yellow", t_obj->name) )
      {
        if( rchance < 5 )
        {
          SET_BIT(obj->bitvector3, AFF3_GR_SPIRIT_WARD);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the &+CShaman&n");
        }
        else if( rchance < 25 )
        {
          SET_BIT(obj->bitvector2, AFF2_SOULSHIELD);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the &+Wsoul&n");
        }
        else if( rchance < 76 )
        {
          SET_BIT(obj->bitvector, AFF_UD_VISION);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof the &+munderdark&n");
        }
        else
        {
          SET_BIT(obj->bitvector2, AFF2_DETECT_MAGIC);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+mmagic &+Ldetection&n");
        }
        obj_from_char(t_obj, TRUE);
        extract_obj(t_obj, TRUE);
        break;
      }
      else if( isname("blue", t_obj->name) )
      {
        if( rchance < 5 )
        {
          SET_BIT(obj->bitvector2, AFF2_GLOBE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+mmag&+Mic&+mal pro&+Mtec&+mtion&n");
        }
        else if( rchance < 25 )
        {
          SET_BIT(obj->bitvector, AFF_AWARE);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+WAwa&+wrene&+Lss&n");
        }
        else if( rchance < 76 )
        {
          SET_BIT(obj->bitvector4, AFF4_NEG_SHIELD);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+Lne&+mga&+Lti&+mvi&+Lty&n");
        }
        else
        {
          SET_BIT(obj->bitvector2, AFF2_DETECT_MAGIC);
          send_to_char("&+LYou infuse the &+Mmagical&+L properties of your stone into your creation...\r\n", ch);
          sprintf(emsg, " &+Lof &+mmagic &+Ldetection&n");
        }
        obj_from_char(t_obj, TRUE);
        extract_obj(t_obj, TRUE);
        break;
      }
    }
  }


  if( good > 0 )
  {
    sprintf(tempdesc, "%s", obj->short_description);
    sprintf(short_desc, "%s&n%s &+w[&+Lsu&+wp&+Wer&+wi&+Lor&+w]&n", tempdesc, emsg);
    set_short_description(obj, short_desc);
  }
  else if( good < 0 )
  {
    sprintf(tempdesc, "%s", obj->short_description);
    sprintf(short_desc, "%s&n%s &+w[&+ypoor&+w]&n", tempdesc, emsg);
    set_short_description(obj, short_desc);
  }
  else if( modified )
  {
    sprintf(tempdesc, "%s", obj->short_description);
    sprintf(short_desc, "%s&n%s &+w[&+Gmodified&+w]&n", tempdesc, emsg);
    set_short_description(obj, short_desc);
  }
}

P_obj random_zone_item(P_char ch)
{
  P_obj reward = read_object(real_object(getItemFromZone(GET_ZONE(ch))), REAL);

  if(!reward)
    reward = create_random_eq_new(ch, ch, -1, -1);

  if(reward)
  {
    debug( "random_zone_item: %s reward was: %s (%d)", J_NAME(ch), reward->short_description, GET_OBJ_VNUM(reward) );

    REMOVE_BIT(reward->extra_flags, ITEM_SECRET);
    REMOVE_BIT(reward->extra_flags, ITEM_INVISIBLE);
    SET_BIT(reward->extra_flags, ITEM_NOREPAIR);
    REMOVE_BIT(reward->extra_flags, ITEM_NODROP);
  }
  return reward;
}

void do_conjure(P_char ch, char *argument, int cmd)
{
  char     buf1[MAX_STRING_LENGTH];
  char     first[MAX_INPUT_LENGTH];
  char     second[MAX_INPUT_LENGTH];
  char     rest[MAX_INPUT_LENGTH];
  int i = 0, room = ch->in_room;
  int choice = 0;

  if( !IS_ALIVE(ch) || IS_NPC(ch) )
  {
    return;
  }

  if( GET_LEVEL(ch) < 21 )
  {
    send_to_char("You are not high enough level to conjure beings...\r\n", ch);
    return;
  }
  if( !GET_CLASS(ch, CLASS_SUMMONER) )
  {
    act("&+YConjuring advanced beings &nis a &+Mmagic &nbeyond your abilities&n.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
/* Commenting this out atm; going to allow lvl 21 + to conjure the basic mob prototype thingy.
  // If not spec'd.
  if( !GET_SPEC(ch, CLASS_SUMMONER, SPEC_CONTROLLER) && !GET_SPEC(ch, CLASS_SUMMONER, SPEC_MENTALIST) && !GET_SPEC(ch, CLASS_SUMMONER, SPEC_NATURALIST) )
  {
    act("&+YConjuring advanced beings &nis a &+Mmagic &nbeyond your abilities&n.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
*/

  if(CHAR_IN_SAFE_ZONE(ch))
  {
    send_to_char("A mysterious force blocks your conjuring!\n", ch);
    return;
  }


  /***DISPLAYSPELLBOOK STUFF***/

  char     buf[256], *buff, buf2[256], rbuf[MAX_STRING_LENGTH], cinfo[MAX_STRING_LENGTH], cinfo2[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH], selectedrecipe[MAX_STRING_LENGTH];
  char buffer[256];
  FILE    *f;
  FILE    *recipelist;
  int line, recfind;
  int duration;
  unsigned long	linenum = 0;
  struct affected_type af;
  long recnum, choice2;
  long selected = 0;
  P_char tobj;


  //Create buffers for name
  strcpy(buf, GET_NAME(ch));
  buff = buf;
  for (; *buff; buff++)
  {
    *buff = LOWER(*buff);
  }
  //buf[0] snags first character of name
  sprintf(Gbuf1, "%s/%c/%s.spellbook", SAVE_DIR, buf[0], buf);
  recipelist = fopen(Gbuf1, "r");
  if( !recipelist )
  {
    create_spellbook_file(ch);
    recipelist = fopen(Gbuf1, "r");

    if(!recipelist)
    {
      send_to_char("Fatal error opening spellbook, notify a god.\r\n", ch);
      return;
    }
  }
  half_chop(argument, first, rest);
  half_chop(rest, second, rest);
  choice2 = atoi(second);


  if( !*argument )
  {
    send_to_char("&+WThese are the &+mmys&+Mtic&+Wal commands for &+Yconjuring&+W:\n&n", ch);
    send_to_char("&+W(&+wconjure stat <number> &+m- &+mreveal statistical properties about this &+Mminion&n.)\n&n", ch);
    send_to_char("&+W(&+wconjure summon <number> &+m- &+mcall the &+Mminion&+m into existence&n.)\n&n", ch);
    send_to_char("&+MYou have learned the following &+mMobs&+M:\n&n", ch);
    send_to_char("----------------------------------------------------------------------------\n", ch);
    send_to_char("&+M Mob Number		          &+mMob Name		&n\n\r", ch);

    while( (fscanf(recipelist, "%ld", &recnum)) != EOF )
    {
      if(recnum == choice2)
      {
        selected = choice2;
      }

      tobj = read_mobile(recnum, VIRTUAL);

      if(tobj)
      {
        sprintf(rbuf, "%ld\n", recnum);
        sprintf(buffer, "   &+W%-22ld&n%-41s&n\n", recnum, tobj->player.short_descr);
        //stores the actual vnum written in file into rbuf 
        page_string(ch->desc, buffer, 1);
        send_to_char("----------------------------------------------------------------------------\n", ch);
        extract_char(tobj);
      }
    }
    fclose(recipelist);
    /***ENDDISPLAYRECIPES***/

    return;
  }

  while( (fscanf(recipelist, "%ld", &recnum)) != EOF )
  {
    if( recnum == choice2 )
    {
      selected = choice2;
    }

    sprintf(rbuf, "%ld\n", recnum);
  }
  fclose(recipelist);


  if( is_abbrev(first, "stat") )
  {
    if(choice2 == 0)
    {
      send_to_char("&+mWhich &+Mminion&n would you like &+Wstatistics&+m about?\n", ch);
      return;
    }
    if(selected == 0)
    {
      send_to_char("&+mIt appears you have not yet &+Mlearned&+m how to conjure that &+Mminion&+m.&n\n", ch);
      return;
    }
    tobj = read_mobile(selected, VIRTUAL);
    send_to_char("&+rYou open your &+RSummoners &+Lt&+mo&+Mm&+We &+rwhich &+Rreveals&+r the following information...&n.\n", ch);
    get_class_string(tobj, cinfo2);
    sprintf(cinfo, "You glean they are: \r\n&+YLevel &+W%d \r\n&+YClass:&n %s \r\n&+YBase Hitpoints:&n %d\r\n", GET_LEVEL(tobj), get_class_string(tobj, buf2), GET_MAX_HIT(tobj));
    send_to_char(cinfo, ch);
    extract_char(tobj);
    return;
  }

  else if (is_abbrev(first, "summon"))
  {

    if( selected == 0 )
    {
      send_to_char("&+mIt appears you have not yet &+Mlearned&+m how to conjure that &+Mminion&+m.&n\n", ch);
      return;
    }

    tobj = read_mobile(selected, VIRTUAL);

    if( !valid_conjure(ch, tobj) && !IS_TRUSTED(ch) )
    {
      if( tobj )
      {
        send_to_char("Your character does not have &+Ldominion&n over this race of &+Lmonster&n, either because its level is too high, or it is not a valid race for you to summon.\r\n", ch);
        extract_char(tobj);
      }
      else
      {
        sprintf( Gbuf1, "Failed load on mob %ld.  Sorry, try again or tell a God.\n\r", selected );
        send_to_char( Gbuf1, ch);
      }
      return;
    }

    if( !new_summon_check(ch, tobj) && !IS_TRUSTED(ch) )
    {
      extract_char(tobj);
      return;
    }

    if( affected_by_spell(ch, SPELL_CONJURE_ELEMENTAL) && !IS_TRUSTED(ch) )
    {
      send_to_char("You must wait a short time before calling another &+Yminion&n into existence.\r\n", ch);
      extract_char(tobj);
      return;
    }

    if( GET_C_CHA(ch) < number(1, 130) )
    {
      if(!IS_TRUSTED(ch))
      {
        memset(&af, 0, sizeof(af));
        af.type = SPELL_CONJURE_ELEMENTAL;
        af.duration = 2;
        affect_to_char(ch, &af);
      }
      extract_char(tobj);
      send_to_char("You feel a brief &+mtinge&n of &+Mmagical power&n engulf you as you &+rfail&n to call forth your &+Lminion&n.\r\n", ch);
      return;
    }


    if( (GET_LEVEL(tobj) > 51) && !vnum_in_inv(ch, 400231) && !IS_TRUSTED(ch) )
    {
      send_to_char("You must have a &+Ya &+Mgreater&+Y o&+Mr&+Bb &+Yof &+mM&+Ma&+Wg&+Mi&+mc&n in your &+Winventory&n in order to &+Ysummon&n a being of such &+Mgreat&+M power&n.\r\n", ch);
      extract_char(tobj);
      return;
    }

    struct affected_type af;

#define TARGET_CONJ_MOB = tobj;

    //set up stats
    int chance = number(1, GET_C_CHA(ch));
//    debug("Conjure chance %d", chance);

    if(chance > 70)
    {
      act("$n's &+mcha&+Mris&+Mma&n &+Cradiates&n as they call forth their minion!", TRUE, ch, 0,
          tobj, TO_ROOM);
      act("Your &+mcha&+Mris&+Mma&n &+Cradiates&n as you call forth your minion!", TRUE, ch, 0,
          tobj, TO_CHAR);
      GET_MAX_HIT(tobj) = GET_HIT(tobj) =  tobj->points.base_hit = (tobj->points.base_hit * (1 + (number(1, 4) * .1)));
    }

    if(chance < 30)
    {
      act("An &+Lug&+yli&+Ler &nside of $n seems to eminate as they call forth their minion.", TRUE, ch, 0,
          tobj, TO_ROOM);
      act("Your &+Lug&+yli&+Ler &nside seems to eminate as you call forth your minion.", TRUE, ch, 0,
          tobj, TO_CHAR);
      GET_MAX_HIT(tobj) = GET_HIT(tobj) = tobj->points.base_hit = (tobj->points.base_hit * (number(6, 9) * .1));
    }
    // 20% bonus hps for max skill, 2% for each skill notch.
    if( GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE) )
    {
      act("You channel extra &+Wlifeforce&n as you call forth your minion.", TRUE, ch, 0, tobj, TO_CHAR);
      GET_MAX_HIT(tobj) = GET_HIT(tobj) = tobj->points.base_hit = GET_HIT(tobj) * ((500.0 + GET_CHAR_SKILL(ch, SKILL_INFUSE_LIFE)) / 500.0);
    }

    //Set up NPCACT etc.
    if(tobj->points.base_hit > 8000)
      GET_MAX_HIT(tobj) = GET_HIT(tobj) = tobj->points.base_hit = 8000;

    //REMOVE_BIT(tobj->specials.act, ACT_SENTINEL); Needed for mob to follow.

    REMOVE_BIT(tobj->specials.affected_by, AFF_SLEEP);
    REMOVE_BIT(tobj->specials.act, ACT_ELITE);
    REMOVE_BIT(tobj->specials.act, ACT_HUNTER);
    REMOVE_BIT(tobj->specials.act, ACT_PROTECTOR);
    GET_EXP(tobj) = 0;
    apply_achievement(tobj, TAG_CONJURED_PET);
    SET_BIT(tobj->specials.affected_by, AFF_INFRAVISION);
    REMOVE_BIT(tobj->specials.affected_by4, AFF4_DEFLECT);
    REMOVE_BIT(tobj->specials.act, ACT_SCAVENGER);
    REMOVE_BIT(tobj->specials.act, ACT_PATROL);
    REMOVE_BIT(tobj->specials.act, ACT_SPEC);
    if( number(1,100) > 50 )
    {
      tobj->player.spec = 0;
    }
    REMOVE_BIT(tobj->specials.act, ACT_BREAK_CHARM);
    // Stop mobs from randomly sitting all the time.
    tobj->only.npc->default_pos = POS_STANDING + STAT_NORMAL;

    if(GET_LEVEL(tobj) > 56 && !IS_TRUSTED(ch))
    {
      vnum_from_inv(ch, 400231, 1);
      act("$n &+Ltosses their &+Ya &+Mgreater&+Y o&+Mr&+Bb &+Yof &+mM&+Ma&+Wg&+Mi&+mc&n &+Linto the &+Cair&+L, which quickly forms an &+Rextra-dimensional &+Lpocket&n!", TRUE, ch, 0,
          tobj, TO_ROOM);
      act("You &+Ltoss your &+Ya &+Mgreater&+Y o&+Mr&+Bb &+Yof &+mM&+Ma&+Wg&+Mi&+mc&n &+Linto the &+Cair&+L, which quickly forms an &+Rextra-dimensional &+Lpocket&n!", TRUE, ch, 0,
          tobj, TO_CHAR);
    }

    tobj->only.npc->aggro_flags = 0;
    act("$n utters a quick &+mincantation&n, calling forth $N who softly says 'Your wish is my command, $n!'", TRUE, ch, 0,
        tobj, TO_ROOM);
    act("You utter a quick &+mincantation&n, calling forth $N who softly says 'Your wish is my command, master!'", TRUE, ch, 0,
        tobj, TO_CHAR);

    duration = setup_pet(tobj, ch, 400 / STAT_INDEX(GET_C_INT(tobj)), PET_NOCASH);
    char_to_room(tobj, room, 0);
    SET_POS(tobj, POS_STANDING + STAT_NORMAL);
    add_follower(tobj, ch);
    if(duration >= 0)
    {
      duration += number(1,10);
      add_event(event_pet_death, (duration+1) * 60 * 4, tobj, NULL, NULL, 0, NULL, 0);
    }

    if(!IS_TRUSTED(ch))
    {
      memset(&af, 0, sizeof(af));
      af.type = SPELL_CONJURE_ELEMENTAL;
      af.flags = AFFTYPE_NODISPEL;
      af.duration = 1;
      affect_to_char(ch, &af);
    }
  }
}

void create_spellbook_file(P_char ch)
{
  char buf[256], *buff, Gbuf1[MAX_STRING_LENGTH];
  FILE *f;
  int defrec;
  defrec = 400003;

  strcpy(buf, GET_NAME(ch));
  buff = buf;
  for (; *buff; buff++)
    *buff = LOWER(*buff);
  sprintf(Gbuf1, "%s/%c/%s.spellbook", SAVE_DIR, buf[0], buf);
  f = fopen(Gbuf1, "w");
  fclose(f);
  f = fopen(Gbuf1, "a");
  fprintf(f, "%d\n", defrec);
  fclose(f);
}

int count_classes( P_char mob )
{
  int count = 0;

  // PCs and !multi mobs have 1 class
  if( !IS_NPC(mob) )
  {
    return IS_MULTICLASS_PC(mob) ? 2 : 1;
  }

  if( !IS_MULTICLASS_NPC(mob) )
  {
    return 1;
  }

  for( int i = 0;i < CLASS_COUNT;i++ )
  {
    if( mob->player.m_class & (1 << i) )
    {
      count++;
    }
  }

  return count;
}

bool valid_conjure(P_char ch, P_char victim)
{
  int maxclasses = IS_MULTICLASS_PC(ch) ? 1 : 3;

  if( !victim || !ch )
    return FALSE;

  if( IS_PC(victim) )
    return FALSE;

/* Commented out new changes for now.
  if( GET_LEVEL(victim) > GET_LEVEL(ch) || IS_MULTICLASS_NPC(victim) )
    return FALSE;
*/

  if(GET_VNUM(victim) != 400003)
  {
    if(GET_SPEC(ch, CLASS_SUMMONER, SPEC_CONTROLLER) && !IS_HUMANOID(victim) || IS_UNDEADRACE(victim))
    {
      return FALSE;
    }

    if(GET_SPEC(ch, CLASS_SUMMONER, SPEC_MENTALIST) && !IS_ELEMENTAL(victim))
    {
      return FALSE;
    }

    if(GET_SPEC(ch, CLASS_SUMMONER, SPEC_NATURALIST) && !IS_ANIMAL(victim))
    {
      return FALSE;
    }

    // New change: All conj's pets must be within 5 lvls.
    if( GET_LEVEL(victim) - GET_LEVEL(ch) > 5 )
      return FALSE;

    // New change: Pets can have at most 3 classes.
    if( count_classes(victim) > maxclasses )
    {
      return FALSE;
    }
  }

  return TRUE;

}

bool new_summon_check(P_char ch, P_char selected)
{
  struct follow_type *k;
  P_char   victim;
  int i, j, count = 0, desired = 0, greater = 0;

  desired = GET_LEVEL(selected);

  if( desired - GET_LEVEL(ch) > 5 )
  {
    send_to_char("That monster is too powerful for you to summon yet.\r\n", ch);
    return FALSE;
  }

  for( k = ch->followers, i = 0, j = 0; k; k = k->next )
  {
    victim = k->follower;

    if( !IS_PC(victim) && !isname("image", GET_NAME(victim)) )
    {
      i += GET_LEVEL(victim);
      if(GET_LEVEL(victim) >= 50)
      {
        greater = 1;
      }
      count++;
    }
  }
  i += desired;

  if( count >= 4 )
  {
    send_to_char("You have too many followers already.\r\n", ch);
    return FALSE;
  }

  if( greater == 1 && desired >= 50 )
  {
    send_to_char("You may not summon an additional being of such great power.\r\n", ch);
    return FALSE;
  }

  // 70 at lvl 30 and 122 at lvl 56.
  if( i > GET_LEVEL(ch) * 2 + 10 )
  {
    send_to_char("Your current pets are too powerful to summon that being.\r\n", ch);
    return FALSE;
  }

  return TRUE;
}

void learn_conjure_recipe(P_char ch, P_char victim)
{

  char     buf[256], *buff;
  char     Gbuf1[MAX_STRING_LENGTH], *c;
  FILE    *f;
  FILE    *recipelist;

  int recnum;

  if(!ch)
    return;

  if(!victim)
    return;

  if(IS_PC_PET(victim))
    return;

  if(GET_SPEC(ch, CLASS_SUMMONER, SPEC_CONTROLLER) && !IS_HUMANOID(victim))
  {
    send_to_char("You cannot learn to summon a being outside of your area of expertise.\r\n", ch);
    extract_char(victim);
    return;
  }

  if(GET_SPEC(ch, CLASS_SUMMONER, SPEC_MENTALIST) && !IS_ELEMENTAL(victim))
  {
    send_to_char("You cannot learn to summon a being outside of your area of expertise.\r\n", ch);
    extract_char(victim);
    return;
  }

  if(GET_SPEC(ch, CLASS_SUMMONER, SPEC_NATURALIST) && !IS_ANIMAL(victim))
  {
    send_to_char("You cannot learn to summon a being outside of your area of expertise.\r\n", ch);
    extract_char(victim);
    return;
  }

  if(mob_index[GET_RNUM(victim)].func.mob == shop_keeper || (mob_index[GET_RNUM(victim)].qst_func == shop_keeper))
  {
    act("$n &+Ltries to charm their listeners &+Lbut suddenly &+rDrannak&+L appears from the skies!", TRUE, ch, NULL, 0, TO_ROOM);
    act("You attempt to charm your listeners, but suddenly &+rDrannak&n appears from the skies!", FALSE, ch, NULL, 0, TO_CHAR);
    act("&+rDrannak&n makes a strange gesture, causing a large brick of &+Ycheese&n the size of a whale to suddenly fall on $n, crushing them completely!", TRUE, ch, NULL, 0, TO_ROOM);
    act("&+rDrannak&n makes a strange gesture, causing a large brick of &+Ycheese&n the size of a whale to suddenly fall on you, crushing you completely!", FALSE, ch, NULL, 0, TO_CHAR);
    die(ch, ch);
    return;
  }

  int recipenumber = GET_VNUM(victim);





  //Create buffers for name
  strcpy(buf, GET_NAME(ch));
  buff = buf;
  for (; *buff; buff++)
    *buff = LOWER(*buff);
  //buf[0] snags first character of name
  sprintf(Gbuf1, "%s/%c/%s.spellbook", SAVE_DIR, buf[0], buf);

  /*just a debug test
    send_to_char(Gbuf1, ch);*/

  //check if tradeskill file exists for player
  recipelist = fopen(Gbuf1, "rt");

  if (!recipelist)
  {
    create_spellbook_file(ch);
    recipelist = fopen(Gbuf1, "r");

    if(!recipelist)
    {
      send_to_char("Fatal error opening spellbook, notify a god.\r\n", ch);
      return;
    }

  }
  /* Check to see if recipe exists */
  while((fscanf(recipelist, "%i", &recnum)) != EOF )
  {  
    if(recnum == recipenumber)
    {
      send_to_char("You already know how to summon that creature!&n\r\n", ch);
      fclose(recipelist);
      return;
    }

  }
  fclose(recipelist);
  recipelist = fopen(Gbuf1, "a");
  fprintf(recipelist, "%d\n", recipenumber);
  act("$n &+gsuddenly &+Greaches &+gout and makes a &+Mmagical &+mgesture &+gabout &n$N&+g...\n"
      "&+gsh&+Gar&+Wds&+g of &+mcry&+Mstallized &+Wmagic&+g begin to form a square dome around &n$N&+g.\n"
      "&+gWith a &+Gfinal&+g &+mgesture&+g, &n$n &+gpoints at &n$N &+gwho is &+Gconsumed&+g by the &+mmagical &+Mdome&+g, and disappears from sight.\r\n", FALSE, ch, 0, victim, TO_ROOM);
  act("&+gYou &+gsuddenly &+Greach &+gout and make a &+Mmagical &+mgesture &+gabout &n$N&+g...\n"
      "&+gsh&+Gar&+Wds&+g of &+mcry&+Mstallized &+Wmagic&+g begin to form a square dome around &n$N&+g.\n"
      "&+gWith a &+Gfinal&+g &+mgesture&+g, you &+gpoint at &n$N &+gwho is &+Gconsumed&+g by the &+mmagical &+Mdome&+g, and disappears from sight.\r\n", FALSE, ch, 0, victim, TO_CHAR);   
  fclose(recipelist);
  send_to_char("&+gYou have learned a new minion to &+Gconjure&+g!\r\n", ch);
  return;
}

void do_dismiss(P_char ch, char *argument, int cmd)
{
  struct follow_type *k;
  struct follow_type *x;
  P_char   victim, next_vict;
  int i, j, count = 0, desired = 0;

  if(GET_CLASS(ch, CLASS_BARD))
  {
    for (k = ch->followers; k; k = x)
    {
      x = k->next;

      // Dismiss mirror images
      if(IS_NPC(k->follower) && GET_VNUM(k->follower) == 250 )
      {
        act("$n makes a &+Mmagical &+mgesture&n, sending $N back to the &+Lnether plane&n.", TRUE, ch, 0,
            k->follower, TO_ROOM);
        act("You make a &+Mmagical &+mgesture&n, sending $N back to the &+Lnether plane&n.", TRUE, ch, 0,
            k->follower, TO_CHAR);
        extract_char(k->follower);
        count++;
      }
    }
    // If no images..
    if( !count )
      send_to_char("Just stop singing.\r\n", ch);
    return;
  }

  if(!argument || !*argument)
  {
    for (k = ch->followers; k; k = x)
    {
      x = k->next;

      if(!IS_PC(k->follower))
      {
        act("$n makes a &+Mmagical &+mgesture&n, sending $N back to the &+Lnether plane&n.", TRUE, ch, 0,
            k->follower, TO_ROOM);
        act("You make a &+Mmagical &+mgesture&n, sending $N back to the &+Lnether plane&n.", TRUE, ch, 0,
            k->follower, TO_CHAR);
        extract_char(k->follower);
        count++;
      }
    }
    return;
  }



  victim = parse_victim(ch, argument, 0);
  if(!victim)
  {
    send_to_char("Who?\r\n", ch);
    return;
  }
  if(ch->in_room != victim->in_room ||
      ch->specials.z_cord != victim->specials.z_cord)
  {
    send_to_char("Who?\r\n", ch);
    return;
  }
  if(get_linked_char(victim, LNK_PET) == ch)
  {
    act("$n makes a &+Mmagical &+mgesture&n, sending $N back to the &+Lnether plane&n.", TRUE, ch, 0,
        victim, TO_ROOM);
    act("You make a &+Mmagical &+mgesture&n, sending $N back to the &+Lnether plane&n.", TRUE, ch, 0,
        victim, TO_CHAR);
    extract_char(victim);
  }




  if(count == 0)
    return;


}

int calculate_shipfrags(P_char ch)
{
  for (int i = 0; i < 2000; i++)
  {
    if (shipfrags[i].ship == NULL)
    {
      break;
    }
    int found = 0;
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
      if (svs == shipfrags[i].ship)
      {
        found = 1;
      }
    }
    if (!found)
    {
      break;
    }

    //debug("ownername: %s frags: %d getname: %s\r\n", shipfrags[i].ship->ownername, shipfrags[i].ship->frags, GET_NAME(ch));
    if(!strcmp(shipfrags[i].ship->ownername, GET_NAME(ch)))
    {
      int shipfr = shipfrags[i].ship->frags;
      // debug("shipfr: %d\r\n", shipfr);
      return shipfr;
    }

    if (shipfrags[i].ship->frags == 0)
    {
      break;
    }
    if (i != 0)
    {
      if (shipfrags[i].ship == shipfrags[i - 1].ship)
      {
        break;
      }
    }


    /*send_to_char_f(ch,
      "&+W%d:&N %s\r\n&+LCaptain: &+W%-20s &+LClass: &+y%-15s&+R Tonnage Sunk: &+W%d&N\r\n\r\n",
      i + 1, shipfrags[i].ship->name, shipfrags[i].ship->ownername,
      SHIPTYPE_NAME(SHIP_CLASS(shipfrags[i].ship)),
      shipfrags[i].ship->frags);*/
  }
}

bool calmcheck(P_char ch)
{
  if( CALMCHANCE(ch) > number(1, 100) )
  {
    return TRUE;
  }
  return FALSE;
}

void enhance(P_char ch, P_obj source, P_obj material)
{
  int mod = 0;
  char buf[MAX_STRING_LENGTH];
  P_obj robj;
  long robjint;
  int cost, searchcount, maxsearch, tries, sval;
  bool validobj;
  int val, minval, chluck, wearflags;

  if(!ch || !source || !material)
    return;

  chluck = (GET_C_LUK(ch));
  sval = itemvalue(ch, source);
  val = itemvalue(ch, material);
  minval = itemvalue(ch, source) - 5;
  validobj = FALSE;
  searchcount = 0;
  maxsearch = 20000;
  // Only search matching wear flags unless none matching, then just search source wear flags.
  //  We skip ITEM_TAKE 'cause it's not really a wear flag.  We skip ITEM_HOLD, ITEM_ATTACH_BELT, and
  //  ITEM_WEAR_BACK because these are too common and override what people really want (i.e. a quiver).
  wearflags = ( source->wear_flags & material->wear_flags ) & ~( ITEM_TAKE | ITEM_HOLD | ITEM_ATTACH_BELT | ITEM_WEAR_BACK );
  if( !wearflags )
    wearflags = ( source->wear_flags ) & ~( ITEM_TAKE | ITEM_HOLD | ITEM_ATTACH_BELT | ITEM_WEAR_BACK );

  if( !wearflags )
  {
    send_to_char( "This item can not be enhanced.\n", ch );
    return;
  }

  if( itemvalue( ch, source ) > 100 )
  {
    send_to_char( "This item is too powerful to be enhanced further.\n", ch );
    return;
  }

  if( IS_SET(source->wear_flags, ITEM_GUILD_INSIGNIA) )
    minval +=5;

  if( val < minval )
  {
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    sprintf(buf2, "%s", source->short_description);
    sprintf(buf, "&+REnhancing %s requires an item with at least an &+Witem value of: %d&n\r\n", buf2, minval);
    send_to_char(buf, ch);
    return;
  }

  if( val <= 20 )
  {
    cost = 1000;
  }
  else
  {
    cost = 10000;
  }

  if( GET_MONEY(ch) < cost )
  {
    sprintf(buf, "It will require &+W%d platinum&n to &+Benhance&n this item.\r\n", cost/1000);
    send_to_char(buf, ch);
    return;
  }

  mod = 1;
  if(number(1, 1200) < chluck)
  {
    mod += 3;
    maxsearch *= 4;
    send_to_char("&+YYou feel &+MEXTREMELY Lucky&+Y!\r\n", ch);
  }
  else if(number(1, 800) < chluck)
  {
    mod += 2;
    maxsearch *= 3;
    send_to_char("&+YYou feel &+MVery Lucky&+Y!\r\n", ch);
  }
  else if(number(1, 400) < chluck)
  {
    mod ++;
    maxsearch *= 2;
    send_to_char("&+YYou feel &+MLucky&+Y!\r\n", ch);
  }

  mod += itemvalue(ch, source);
  /*debug sprintf(buf, "validobj value: %d\n\r", validobj);
    send_to_char(buf, ch);*/
  while(validobj == 0)
  {
    robjint = number(1300, 134000);
    robj = read_object(robjint, VIRTUAL);
    validobj = 1;

    if(!robj)
    {
      validobj = 0;
    }
    else if(!IS_SET(robj->wear_flags, ITEM_TAKE) || robj->type == ITEM_KEY || IS_SET(robj->extra_flags, ITEM_ARTIFACT) || IS_SET(robj->extra_flags, ITEM_NOSELL) || IS_SET(robj->extra_flags, ITEM_NORENT) || IS_SET(robj->extra_flags, ITEM_NOSHOW) || IS_SET(robj->extra_flags, ITEM_TRANSIENT))
    {
      validobj = 0;
      extract_obj(robj, FALSE);
    }
    else if( itemvalue(ch, robj) != mod
      || (robj->type == ITEM_STAFF && robj->value[3] > 0)
      || robj->type == ITEM_TREASURE || robj->type == ITEM_POTION
      || robj->type == ITEM_MONEY || robj->type == ITEM_KEY
      || robj->type == ITEM_WAND )
    {
      validobj = 0;
      extract_obj(robj, FALSE);
    }
    else if(GET_OBJ_VNUM(robj) == GET_OBJ_VNUM(source))
    {
      validobj = 0;
      extract_obj(robj, FALSE);
    }
    else if(IS_SET(source->wear_flags, ITEM_WIELD) && !IS_SET(robj->wear_flags, ITEM_WIELD))
    {
      validobj = 0;
      extract_obj(robj, FALSE);
    }
/*  Tidying this up into a short simple statement...
    else if( (IS_SET(source->wear_flags, ITEM_WIELD) && IS_SET(robj->wear_flags, ITEM_WIELD)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_BODY) && IS_SET(robj->wear_flags, ITEM_WEAR_BODY)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_LEGS) && IS_SET(robj->wear_flags, ITEM_WEAR_LEGS)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_ARMS) && IS_SET(robj->wear_flags, ITEM_WEAR_ARMS)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_HEAD) && IS_SET(robj->wear_flags, ITEM_WEAR_HEAD)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_EYES) && IS_SET(robj->wear_flags, ITEM_WEAR_EYES)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_EARRING) && IS_SET(robj->wear_flags, ITEM_WEAR_EARRING)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_NOSE) && IS_SET(robj->wear_flags, ITEM_WEAR_NOSE)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_FACE) && IS_SET(robj->wear_flags, ITEM_WEAR_FACE)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_QUIVER) && IS_SET(robj->wear_flags, ITEM_WEAR_QUIVER)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_FINGER) && IS_SET(robj->wear_flags, ITEM_WEAR_FINGER)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_WRIST) && IS_SET(robj->wear_flags, ITEM_WEAR_WRIST)) ||
        (IS_SET(source->wear_flags, ITEM_GUILD_INSIGNIA) && IS_SET(robj->wear_flags, ITEM_GUILD_INSIGNIA)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_TAIL) && IS_SET(robj->wear_flags, ITEM_WEAR_TAIL)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_NECK) && IS_SET(robj->wear_flags, ITEM_WEAR_NECK)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_WAIST) && IS_SET(robj->wear_flags, ITEM_WEAR_WAIST)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_ABOUT) && IS_SET(robj->wear_flags, ITEM_WEAR_ABOUT)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_FEET) && IS_SET(robj->wear_flags, ITEM_WEAR_FEET)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_SHIELD) && IS_SET(robj->wear_flags, ITEM_WEAR_SHIELD)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_HANDS) && IS_SET(robj->wear_flags, ITEM_WEAR_HANDS)) ||
        (IS_SET(source->wear_flags, ITEM_WEAR_BACK) && IS_SET(robj->wear_flags, ITEM_WEAR_BACK))
        )
*/
    else if( wearflags & robj->wear_flags )
    {
          validobj = 1;
    }
    else
    {
      validobj = 0;
      extract_obj(robj, FALSE);
    }

    searchcount++;
    if( searchcount > maxsearch )
    {
      act("&+GThe &+ritem gods&+G could not find a better type of &+yitem &+Gthan your &n$p&+G this time. &+WTry again&+G. If your item's value is above &+W55&+G you may have the &+Wbest&+G item of that type!\r\n", FALSE, ch, source, 0, TO_CHAR);
      return;
    }

  }
  //Remove Curse, Secret, add Invis
  if(IS_SET(robj->extra_flags, ITEM_SECRET))
  {
    REMOVE_BIT(robj->extra_flags, ITEM_SECRET);
  }
  if(IS_SET(robj->extra_flags, ITEM_NODROP))
  {
    REMOVE_BIT(robj->extra_flags, ITEM_NODROP);
  }

  if(IS_SET(robj->extra_flags, ITEM_INVISIBLE))
  {
    REMOVE_BIT(robj->extra_flags, ITEM_INVISIBLE);
  }
  SET_BIT(robj->extra_flags, ITEM_NOREPAIR);
  SUB_MONEY(ch, cost, 0);
  send_to_char("Your pockets feel &+Wlighter&n.\r\n", ch);

  //randomizeitem(ch, robj);

  act("&+BYour enhancement is a success! You now have &n$p&+B!\r\n", FALSE, ch, robj, 0, TO_CHAR);
  //debug("search count: %d\r\n", searchcount);
  obj_to_char(robj, ch);     
  obj_from_char(source, TRUE);
  extract_obj(source, FALSE);
  obj_from_char(material, TRUE);
  extract_obj(material, FALSE);
  statuslog(ch->player.level,
    "&+BEnhancement&n:&n %s&n just got [%d] '%s&n' ival [%d] at [%d]!",
    GET_NAME(ch), obj_index[robj->R_num].virtual_number, robj->short_description,
      itemvalue(ch, robj), (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);

  return;
}

void do_enhance(P_char ch, char *argument, int cmd)
{
  P_obj source, material;
  char     first[MAX_INPUT_LENGTH];
  char     second[MAX_INPUT_LENGTH];
  char     rest[MAX_INPUT_LENGTH];

  half_chop(argument, first, rest);
  half_chop(rest, second, rest);
  //material = atoi(second);

  if( IS_NPC(ch) )
    return;

  if(!argument || !*argument)
  {
    send_to_char("&+yWhich &+Witem &+ywould you like to &+men&+Mhan&+mce&+y? &n\r\nSyntax: enhance <source item you want to upgrade> <upgrade material item>\r\n", ch);
    return;
  }

  if (!(source = get_obj_in_list_vis(ch, first, ch->carrying)))
  {
    act("&+yWhich &+Witem &+ywould you like to &+men&+Mhan&+mce&+y?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!(material = get_obj_in_list_vis(ch, second, ch->carrying)))
  {
    act("And which object is the enhancement object?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if(!strcmp(first, second))
  {
    send_to_char("&+yYou cannot enhance an item with itself!\r\n", ch);
    return;
  }
  if (!is_salvageable(source))
  {
    // act("&+yThe item which you wish to &+Menhance&+y cannot be altered this way... try something else&n.", FALSE, ch, 0, 0, TO_CHAR);
    act("&+yYour $p&+y cannot be used in this way... try something else&n.", FALSE, ch, source, 0, TO_CHAR);
    return;
  }
  if (!is_salvageable(material))
  {
    //act("&+yYour &+Menhancement&+y item cannot be used in this way... try something else&n.", FALSE, ch, 0, 0, TO_CHAR);
    act("&+yYour $p&+y cannot be used in this way... try something else&n.", FALSE, ch, material, 0, TO_CHAR);
    return;
  }
  else if(GET_OBJ_VNUM(material) == GET_OBJ_VNUM(source))
  {
    send_to_char("&+yYou cannot enhance an item with itself!\r\n", ch);
    return;
  }
  // If source is a weapon, material must be either a weapon or an essence.
  if( IS_SET(source->wear_flags, ITEM_WIELD) && !IS_SET(material->wear_flags, ITEM_WIELD)
    && (GET_OBJ_VNUM(material) < 400238 || GET_OBJ_VNUM(material) > 400258) )
  {
    send_to_char("&+YWeapons&+y can only enhance other &+Yweapons&n!\r\n", ch);
    return;
  }

  /* Removing this.  There's a cap in void enhance(..).
  if(itemvalue(ch, source) > (GET_LEVEL(ch) * 1.5))
  {
    send_to_char("&+YYou must gain a higher level in order to enhance that item!&n\r\n", ch);
    return;
  }
  */

  if(GET_OBJ_VNUM(material) > 400237 && GET_OBJ_VNUM(material) < 400259)
    modenhance(ch, source, material);
  else
    enhance(ch, source, material);
}

void modenhance(P_char ch, P_obj source, P_obj material)
{


  if(!ch || !source || !material)
    return;

  char buf[MAX_STRING_LENGTH], modstring[MAX_STRING_LENGTH];
  P_obj robj;
  long robjint;
  int mod = 0, loctype = 0;
  int validobj, cost, searchcount = 0, tries;
  int sval = itemvalue(ch, source);
  validobj = 0;
  int val = itemvalue(ch, material);
  int minval = itemvalue(ch, source) - 5;


  if(val <= 20)
  {
    cost = 1000;
    if (GET_MONEY(ch) < 1000)
    {
      send_to_char("It will require &+W1 platinum&n to &+Benhance&n this item.\r\n", ch);
      return;
    }
  }
  if(val > 20 && val < 30)
  {
    cost = 10000;
    if (GET_MONEY(ch) < 20000)
    {
      send_to_char("It will require &+W20 platinum&n to &+Benhance&n this item.\r\n", ch);
      return;
    }
  }
  if(val > 30)
  {
    cost = 50000;
    if (GET_MONEY(ch) < 100000)
    {
      send_to_char("It will require &+W100 platinum&n to &+Benhance&n this item.\r\n", ch);
      return;
    }
  }

  int modtype = GET_OBJ_VNUM(material);
  switch (modtype)
  {
    case 400238:
      if (source->affected[2].location == APPLY_INT)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_INT;
      sprintf(modstring, "&+wof &+Mintelligence&n");
      mod = 1;
      break;
    case 400239:
      if (source->affected[2].location == APPLY_INT_MAX)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_INT_MAX;
      sprintf(modstring, "&+wof &+Mgreater intelligence&n");
      mod = 1;
      break;
    case 400240:
      if (source->affected[2].location == APPLY_CON)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_CON;
      sprintf(modstring, "&+wof &+cconstitution&n");
      mod = 1;
      break;
    case 400241:
      if (source->affected[2].location == APPLY_CON_MAX)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_CON_MAX;
      sprintf(modstring, "&+wof &+cgreater constitution&n");
      mod = 1;
      break;
    case 400242:
      if (source->affected[2].location == APPLY_AGI)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_AGI;
      sprintf(modstring, "&+wof &+Bagility&n");
      mod = 1;
      break;
    case 400243:
      if (source->affected[2].location == APPLY_AGI_MAX)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_AGI_MAX;
      sprintf(modstring, "&+wof &+Bgreater agility&n");
      mod = 1;
      break;
    case 400244:
      if (source->affected[2].location == APPLY_DEX)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_DEX;
      sprintf(modstring, "&+wof &+gdexterity&n");
      mod = 1;
      break;
    case 400245:
      if (source->affected[2].location == APPLY_DEX_MAX)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_DEX_MAX;
      sprintf(modstring, "&+wof &+ggreater dexterity&n");
      mod = 1;
      break;
    case 400246:
      if (source->affected[2].location == APPLY_STR)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_STR;
      sprintf(modstring, "&+wof &+rstrength&n");
      mod = 1;
      break;
    case 400247:
      if (source->affected[2].location == APPLY_STR_MAX)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_STR_MAX;
      sprintf(modstring, "&+wof &+rgreater strength&n");
      mod = 1;
      break;
    case 400248:
      if (source->affected[2].location == APPLY_CHA)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_CHA;
      sprintf(modstring, "&+wof &+Ccharisma&n");
      mod = 1;
      break;
    case 400249:
      if (source->affected[2].location == APPLY_CHA_MAX)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_CHA_MAX;
      sprintf(modstring, "&+wof &+Cgreater charisma&n");
      mod = 1;
      break;
    case 400250:
      if (source->affected[2].location == APPLY_WIS)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_WIS;
      sprintf(modstring, "&+wof &+cwisdom&n");
      mod = 1;
      break;
    case 400251:
      if (source->affected[2].location == APPLY_WIS_MAX)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_WIS_MAX;
      sprintf(modstring, "&+wof &+cgreater wisdom&n");
      mod = 1;
      break;
    case 400252:
      if (source->affected[2].location == APPLY_POW)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_POW;
      sprintf(modstring, "&+wof &+Lpower&n");
      mod = 1;
      break;
    case 400253:
      if (source->affected[2].location == APPLY_POW_MAX)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_POW_MAX;
      sprintf(modstring, "&+wof &+Lgreater power&n");
      mod = 1;
      break;
    case 400254:
      if (source->affected[2].location == APPLY_HIT)
        loctype = 1;
      else
        source->affected[2].location = APPLY_HIT;
      sprintf(modstring, "&+wof &+Rhealth&n");
      mod = 3;
      break;
    case 400255:
      if (source->affected[2].location == APPLY_HITROLL)
        loctype = 1;
      else
        source->affected[2].location = APPLY_HITROLL;
      sprintf(modstring, "&+wof &+yprecision&n");
      mod = 1;
      break;
    case 400256:
      if (source->affected[2].location == APPLY_DAMROLL)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_DAMROLL;
      sprintf(modstring, "&+wof &+ydamage&n");
      mod = 1;
      break;
    case 400257:
      if (source->affected[2].location == APPLY_HIT_REG)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_HIT_REG;
      sprintf(modstring, "&+wof &+gregeneration&n");
      mod = 3;
      break;
    case 400258:
      if (source->affected[2].location == APPLY_MOVE_REG)
        loctype = 1; 
      else
        source->affected[2].location = APPLY_MOVE_REG;
      sprintf(modstring, "&+wof &+Gendurance&n");
      mod = 3;
      break;

    default:
      break;
  }

  if (loctype == 1)
  {
    // IF they've been modified less than 3 times.
    if( source->affected[2].modifier/mod < 3 )
      source->affected[2].modifier += mod;
    else
    {
      send_to_char( "Your enhancement was a failure.  Too much magic.\n", ch );
      return;
    }
  }
  else
    source->affected[2].modifier = mod;



  SUB_MONEY(ch, cost, 0);
  send_to_char("Your pockets feel &+Wlighter&n.\r\n", ch);

  act("&+BYour enhancement is a success! Your &n$p&+B now feels slightly more powerful!\r\n", FALSE, ch, source, 0, TO_CHAR);

  obj_from_char(material, TRUE);
  extract_obj(material, FALSE);

  P_obj tempobj = read_object(GET_OBJ_VNUM(source), VIRTUAL);
  char tempdesc[MAX_STRING_LENGTH], short_desc[MAX_STRING_LENGTH], keywords[MAX_STRING_LENGTH];

  sprintf(keywords, "%s enhanced", tempobj->name);

  sprintf(tempdesc, "%s", tempobj->short_description);
  sprintf(short_desc, "%s %s&n", tempdesc, modstring);
  set_keywords(source, keywords);
  set_short_description(source, short_desc);
  extract_obj(tempobj, FALSE);

  return;
}

int get_progress(P_char ch, int ach, uint required)
{
  int prog = 0, percentage = 0;
  struct affected_type *findaf, *next_af;  //initialize affects

  for(findaf = ch->affected; findaf; findaf = next_af)
  {
    next_af = findaf->next;
    if(findaf && findaf->type == ach)
      prog = findaf->modifier;
  }



  if(prog > 0)
  {
    prog *= 100;
    percentage = prog / required;
  }

  if(prog < 0)
    return 0;

  return percentage;
}

void thanksgiving_proc(P_char ch)
{
  P_char mob;
  if(!ch)
    return;
  char buff[MAX_STRING_LENGTH];
  sprintf(buff, " %s 86", GET_NAME(ch));
  act("&+YSuddenly and without warning, a &+rPlump &+yTurkey &+Yappears out of no where, seemly attracted to the freshly spilled &+Rblood&n!", TRUE, ch, 0, 0, TO_CHAR);
  act("&+YSuddenly and without warning, a &+rPlump &+yTurkey &+Yappears out of no where, seemly attracted to the freshly spilled &+Rblood&n!", TRUE, ch, 0, 0, TO_ROOM);
  //do_givepet(ch, buff, CMD_GIVEPET);
  mob = read_mobile(400005, VIRTUAL);
  if(!mob)
    return;
  obj_to_char(read_object(400232, VIRTUAL), mob);
  char_to_room(mob, ch->in_room, 0);
}

// This function assumes ch exists and is a mob (Verified in fight.c before call).
void enhancematload( P_char ch, P_char killer )
{
  int reward;
  int moblvl = GET_LEVEL(ch);
  if( IS_ELITE(ch) )
  {
    moblvl * 10;
  }
  if( number(1, 5000) < moblvl )
  {
    debug( "enhancematload: mob: '%s' (%d) moblvl %d%s", J_NAME(ch), GET_VNUM(ch), moblvl, IS_ELITE(ch) ? " ELITE." : "." );
    if(number(1, 4000) < moblvl)
    {
      reward = number(1, 8);
      switch( reward )
      {
        case 1:
          reward = 400239;
          break;
        case 2:
          reward = 400241;
          break;
        case 3:
          reward = 400243;
          break;
        case 4:
          reward = 400245;
          break;
        case 5:
          reward = 400247;
          break;
        case 6:
          reward = 400249;
          break;
        case 7:
          reward = 400251;
          break;
        case 8:
          reward = 400253;
          break;
      }
    }
    else
    {
      reward = number(1, 13);
      switch( reward )
      {
        case 1:
          reward = 400238;
          break;
        case 2:
          reward = 400240;
          break;
        case 3:
          reward = 400242;
          break;
        case 4:
          reward = 400244;
          break;
        case 5:
          reward = 400246;
          break;
        case 6:
          reward = 400248;
          break;
        case 7:
          reward = 400250;
          break;
        case 8:
          reward = 400252;
          break;
        case 9:
          reward = 400254;
          break;
        case 10:
          reward = 400255;
          break;
        case 11:
          reward = 400256;
          break;
        case 12:
          reward = 400257;
          break;
        case 13:
          reward = 400258;
          break;
      }
    }
    P_obj gift = read_object(reward, VIRTUAL);
    if( gift )
    {
      // Show reward to master if killer is a pet.
      debug( "enhancematload: '%s' (%d) rewarded to %s.", gift->short_description, GET_OBJ_VNUM(gift),
        IS_PC_PET(killer) ? J_NAME(get_linked_char(killer, LNK_PET)) : J_NAME(killer) );
      obj_to_char( gift, ch );
    }
  }
}

void christmas_proc(P_char ch)
{
  P_char mob;
  if(!ch)
    return;
  char buff[MAX_STRING_LENGTH];
  sprintf(buff, " %s 86", GET_NAME(ch));
  act("&+WAs the last bit of &+rblood&+W drips onto the ground, a &+Gf&+We&+Rs&+Gt&+Wi&+Rv&+Ge&+Wl&+Ry &+rdressed &+Gelf &+Wsuddenly appears in a &+Lpuff &+Wof smoke....&n\r\n"
      "&n'Ah ha! Being quite &+RNaughty&n this year aren't we $n? Wait until &+rSanta Claws&n hears about this!' &+Whe cackles as he reaches beneath his &+Gc&+Ro&+Wa&+Gt&+W and pulls out a small parchment and quill.\r\n", TRUE, ch, 0, 0, TO_CHAR);
  act("&+WAs the last bit of &+rblood&+W drips onto the ground, a &+Gf&+We&+Rs&+Gt&+Wi&+Rv&+Ge&+Wl&+Ry &+rdressed &+Gelf &+Wsuddenly appears in a &+Lpuff &+Wof smoke....&n\r\n"
      "&n'Ah ha! Being quite &+RNaughty&n this year aren't we $n? Wait until &+rSanta Claws&n hears about this!' &+Whe cackles as he reaches beneath his &+Gc&+Ro&+Wa&+Gt&+W and pulls out a small parchment and quill.\r\n", TRUE, ch, 0, 0, TO_ROOM);
  //do_givepet(ch, buff, CMD_GIVEPET);
  mob = read_mobile(400006, VIRTUAL);
  if(!mob)
    return;
  obj_to_char(read_object(400217, VIRTUAL), mob);
  char_to_room(mob, ch->in_room, 0);
}

void add_bloodlust(P_char ch, P_char victim)
{
  int dur;

  if( !IS_ALIVE(ch) || !victim )
  {
    return;
  }

  if( IS_PC_PET(victim) || affected_by_spell(victim, TAG_CONJURED_PET) )
  {
    return;
  }

  if( GET_LEVEL(victim) < (GET_LEVEL(ch) - 5) || GET_LEVEL(ch) > 49 )
  {
    return;
  }

  // We're nice to ogres.. dunno why.
  if( GET_RACE(ch) == RACE_OGRE )
  {
    dur = 10;
  }
  else
  {
    dur = 5;
  }

  send_to_char("&+rThe smell of fresh &+Rblood &+renters your body, &+Rinfusing&+r you with &+Rpower&+r!\r\n", ch);
  struct affected_type af;
  if( !affected_by_spell(ch, TAG_BLOODLUST) )
  {
    memset(&af, 0, sizeof(struct affected_type));
    af.type = TAG_BLOODLUST;
    af.modifier = 1;
    af.duration = dur;
    af.location = 0;
    af.flags = AFFTYPE_NODISPEL;
    affect_to_char(ch, &af);
  }
  else
  {
    struct affected_type *findaf, *next_af;  //initialize affects
    int lvl = MIN(50, GET_LEVEL(ch));
    for(findaf = ch->affected; findaf; findaf = next_af)
    {
      next_af = findaf->next;
      // Lvls 1-40 get 100% bloodlust
      if( findaf && findaf->type == TAG_BLOODLUST && findaf->modifier < 10 && lvl <= 40 )
      {
        findaf->modifier += 1;
        findaf->duration = dur;
      }
      // We are guarenteed here that level is between 41 and 49.
      else if(findaf && findaf->type == TAG_BLOODLUST && lvl > 40 )
      {
        // Lose 10% for ever level over 40.
        // 10 == max_modifier @ 40, 18 = max_modifier @ 41, ... 1 = max_modifier @ 49.
        findaf->modifier = (findaf->modifier < 50 - lvl) ? findaf->modifier + 1 : 50 - lvl;
        findaf->duration = dur;
      }
      // We are at maxxed bloodlust, so just reset timer.
      else if( findaf && findaf->type == TAG_BLOODLUST )
      {
        findaf->duration = dur;
      }
    }

  }
}

bool add_epiccount(P_char ch, int gain)
{
  if (!ch)
    return FALSE;

  if (IS_PC_PET(ch))
    return FALSE;

  if (IS_NPC(ch))
    return FALSE;

  struct affected_type af;
  if(!affected_by_spell(ch, TAG_EPICS))
  {
    memset(&af, 0, sizeof(struct affected_type));
    af.type = TAG_EPICS;
    af.modifier = gain;
    af.duration = -1;
    af.location = 0;
    af.flags = AFFTYPE_NODISPEL, AFFTYPE_PERM;
    affect_to_char(ch, &af);
    return FALSE;
  }
  else
  {
    struct affected_type *findaf, *next_af;  //initialize affects
    for(findaf = ch->affected; findaf; findaf = next_af)
    {
      next_af = findaf->next;
      if((findaf && findaf->type == TAG_EPICS) && findaf->modifier < 100)
      {
        findaf->modifier += gain;
        return FALSE;
      }
      else if(findaf && findaf->type == TAG_EPICS)
      {
        //has over 100 - gain a skill point. modifier needs to equal 100 - gain
        findaf->modifier += gain;
        findaf->modifier -= 100;
        //gain epic skill point;
        return TRUE;
      }
    }
  }
}
