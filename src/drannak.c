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
extern const int exp_table[];

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
extern int find_map_place();

void set_surname(P_char ch, int num)
{
  /* Surname List
  0 - Leaderboard Based (default)
  1 - User Toggled Custom Surname Off
  2 - Lightbringer
  3 - Dragonslayer
  */

 if((num == 0 && !IS_SET(ch->specials.act3, PLR3_NOSUR)) || num == 1 )
  {
  int points = getLeaderBoardPts(ch);
  points *= .01;
  debug("points: %d", points);
  clear_surname(ch);
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



  sprintf(buf2, "\r\n   &+W%-22s\r\n",
          "Note: &nSome &+cachievements&n grant access to additional surnames&n\r\n");
  strcat(buf, buf2);


 page_string(ch->desc, buf, 1);
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
	       "&+LThe Harvester&+L &+wsays 'Only those who have collected the necessary amount souls of may purchase these items.&+L.&n'\n"
	       "&+LThe Harvester&+L &+wsays 'Please &+Yrefer to my &-L&+ysign&n&-l for an explanation of each of these items and their affects.'\n"
              "&+y=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"
		"&+y|		&+cItem Name					           Frags Required            &+y|\n"																
              "&+y|&+W 1) &+ga M&+Ga&+Wg&+Gi&+gc&+Ga&+Wl &+GGreen &+gMu&+Gshro&+gom from the &+GSylvan &+yWoods&n&+C%30d&n		&+y|\n"
              "&+y|&+W 2) &+ya tightly wrapped vellum scroll named '&+LFix&+y'&n   &+C%30d&n		&+y|\n"
              "&+y|&+W 3) &+Wa &+mm&+My&+Ys&+Bt&+Gc&+Ra&+Gl &+MFaerie &+Wbag of &+Lstolen loot&n           &+C%30d&n               &+y|\n"
              "&+y|&+W 4) &+Ya r&+ro&+Yb&+re &+Yof a &+mN&+We&+Mt&+Wh&+me&+Wr&+Mi&+Wl &+rBa&+Ytt&+rle&+Y M&+rag&+Ye&n              &+C%30d&n               &+y|\n"
              "&+y|&+W 5) &+La &+Gbottle &+Lof &+GT&+go&+GR&+gM&+Ge&+gN&+GT&+ge&+GD &+gS&+Goul&+gs     &n              &+C%30d&n               &+y|\n"
              "&+y=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"
		"\n", 125, 85, 20, 5000, 500);
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
	//check for 200 epics required to reset
	int availepics = pl->only.pc->epics;
	if (availepics < 125)
	{
	  send_to_char("&+LThe Harvester&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
	  return TRUE;
        }
	//subtract 125 epics
       P_obj obj;
	obj = read_object(400213, VIRTUAL);
	pl->only.pc->epics -= 125;
       send_to_char("&+LThe Harvester&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
	send_to_char("&+LThe Harvester &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
       act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
       extract_obj(obj, FALSE);
	obj_to_char(read_object(400213, VIRTUAL), pl);
       return TRUE;
    }//endbuy1

    //14126 - fix scroll
	else if(strstr(arg, "2"))
    {//buy2
	//check for 85 epics required to reset
	int availepics = pl->only.pc->epics;
	if (availepics < 85)
	{
	  send_to_char("&+LThe Harvester&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
	  return TRUE;
        }
	//subtract 85 epics
       P_obj obj;
	obj = read_object(14126, VIRTUAL);
	pl->only.pc->epics -= 85;
       send_to_char("&+LThe Harvester&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
	send_to_char("&+LThe Harvester &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
       act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
       extract_obj(obj, FALSE);
	obj_to_char(read_object(14126, VIRTUAL), pl);
       return TRUE;
    }//endbuy2

    //400217 - faerie bag
	else if(strstr(arg, "3"))
    {//buy3
	//check for 20 epics required to reset
	int availepics = pl->only.pc->epics;
	if (availepics < 20)
	{
	  send_to_char("&+LThe Harvester&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
	  return TRUE;
        }
	//subtract 20 epics
       P_obj obj;
	obj = read_object(400217, VIRTUAL);
	pl->only.pc->epics -= 20;
       send_to_char("&+LThe Harvester&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
	send_to_char("&+LThe Harvester &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
       act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
       extract_obj(obj, FALSE);
	obj_to_char(read_object(400217, VIRTUAL), pl);
       return TRUE;
    }//endbuy3

  //400218 - netheril robe
	else if(strstr(arg, "4"))
    {//buy4
	//check for 5000 epics required to reset
	int availepics = pl->only.pc->epics;
	if (availepics < 5000)
	{
	  send_to_char("&+LThe Harvester&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
	  return TRUE;
        }
	//subtract 5000 epics
       P_obj obj;
	obj = read_object(400218, VIRTUAL);
	pl->only.pc->epics -= 5000;
       send_to_char("&+LThe Harvester&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
	send_to_char("&+LThe Harvester &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
       act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
       extract_obj(obj, FALSE);
	obj_to_char(read_object(400218, VIRTUAL), pl);
       return TRUE;
    }//endbuy4

  //400221 - corpse portal potion
	else if(strstr(arg, "5"))
    {//buy5
	//check for 500 epics required to reset
	int availepics = pl->only.pc->epics;
	if (availepics < 500)
	{
	  send_to_char("&+LThe Harvester&+L &+wsays '&nI'm sorry, but you do not seem to have the &+Wepics&n available for that item.\r\n&n", pl);
	  return TRUE;
        }
	//subtract 500 epics
       P_obj obj;
	obj = read_object(400221, VIRTUAL);
	pl->only.pc->epics -= 500;
       send_to_char("&+LThe Harvester&+L &+wsays '&nAh, good choice! Quite a rare item!'\n", pl);
	send_to_char("&+LThe Harvester &+Lthe &+ctra&+Cvell&+cer &nmakes a strange gesture about your body, and hands you your item.\r\n&n", pl);
       act("You now have $p!\r\n", FALSE, pl, obj, 0, TO_CHAR);
       extract_obj(obj, FALSE);
	obj_to_char(read_object(400221, VIRTUAL), pl);
       return TRUE;
    }//endbuy5


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
