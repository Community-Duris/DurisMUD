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
extern P_obj quest_item_reward(P_char ch);
extern int find_map_place();
extern int getItemFromZone(int zone);

void set_surname(P_char ch, int num)
{
  /* Surname List
  0 - Leaderboard Based (default)
  1 - User Toggled Custom Surname Off
  2 - Lightbringer
  3 - Dragonslayer
  4 - Decepticon
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
          "&+W6)&+LDe&+mceptic&+LoN&n");
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
  REMOVE_BIT(ch->specials.act3, PLR3_SURDECEPTICON);
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
	       "&+LThe Harvester&+L &+wsays 'Only those who have collected the necessary amount frags of may purchase these items.&+L.&n'\n"
	       "&+LThe Harvester&+L &+wsays 'Additionally, I offer a reward for &+R6 &+we&+Wt&+Lh&+rer&+Le&+Wa&+wl &+Wsoul &+rshards&n from beings who have fallen in battle&n.&+L.&n'\n"
	       "&+LThe Harvester&+L &+wsays 'Simply have them in your &+Winventory&n and buy the item from the list below&n.&+L.&n'\n"
              "&+y=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"
		"&+y|		&+cItem Name					           Frags Required        &+y|\n"																
              "&+y|&+W 1) &+Ya &+Mgreater&+Y o&+Mr&+Bb &+Yof &+YConjuring&n&+C%30d&n		                        &+y|\n"
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
	if (vnum_in_inv(pl, 400230) < 6)
	{
	  send_to_char("&+LThe Harvester&+L &+wsays '&nI'm sorry, but you do not seem to have the 6 e&+Wt&+Lh&+rer&+Le&+Wa&+wl &+Wsoul &+rshards&n required to purchase that item.\r\n&n", pl);
	  return TRUE;
        }
	//subtract 6 soul shards
       P_obj obj;
	obj = read_object(400231, VIRTUAL);
	vnum_from_inv(pl, 400230, 6);
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

bool minotaur_race_proc(P_char ch, P_char victim)
{
  int num, room = ch->in_room, save, pos, cmd;
  int class_chance = 0;

  switch(ch->player.m_class)
  {
    case CLASS_REAVER:
    class_chance = 30;
    case CLASS_RANGER:
    class_chance = 30;
    case CLASS_MONK:
    class_chance = 20;
    case CLASS_SORCERER:
    class_chance = 6;
    case CLASS_CONJURER:
    class_chance = 6;
    case CLASS_WARRIOR:
    class_chance = 10;
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
  sprintf(gbuf1, "Players/%c/%s.aliases", buf[0], buf);
  
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
       sprintf(bufbug, charalias);
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
	   
       sprintf(bfbug, charalias);

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

  if (IS_SET(temp->extra_flags, ITEM_NOSELL))
    {
        return;
	}

  if(GET_OBJ_VNUM(temp) == 366)
   {
    return;
   }

  if (temp->type == ITEM_FOOD)
   {
    return;
   }
  if (temp->type == ITEM_TREASURE || temp->type == ITEM_POTION || temp->type == ITEM_MONEY || temp->type == ITEM_KEY)
   {
    return;
   }
  if (IS_OBJ_STAT2(temp, ITEM2_STOREITEM))
   {
    return;
   }
  if (IS_SET(temp->extra_flags, ITEM_ARTIFACT))
  {
    return;
  }
       

       
	objrecipe = read_object(400210, VIRTUAL);
       SET_BIT(objrecipe->value[6], recipenumber);
       strcpy(old_name, objrecipe->short_description);
       sprintf(buffer, "%s %s&n", old_name, temp->short_description);


        if ((objrecipe->str_mask & STRUNG_DESC2) && objrecipe->short_description)
         FREE(objrecipe->short_description);

       objrecipe->short_description = str_dup(buffer);

       objrecipe->str_mask |= STRUNG_DESC2;
       obj_to_char(objrecipe, ch);
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

 int result = number(1, 20000);

 if(result < chance)
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

P_obj random_zone_item(P_char ch)
{
  P_obj reward = read_object(real_object(getItemFromZone(GET_ZONE(ch))), REAL);
  
  if(!reward)
    reward = create_random_eq_new(ch, ch, -1, -1);
  
  if(reward)
  {
    wizlog(56, "%s reward was: %s", GET_NAME(ch), reward->short_description);
    
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


 if (!GET_SPEC(ch, CLASS_CONJURER, SPEC_WATER) && !GET_SPEC(ch, CLASS_CONJURER, SPEC_AIR) && !GET_SPEC(ch, CLASS_CONJURER, SPEC_EARTH))
  {
    act("&+YConjuring advanced beings &nis a &+Mmagic &nbeyond your abilities&n.",
        FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
 /* if(!IS_TRUSTED(ch))
  {
    send_to_char("Coming soon...\r\n", ch);
	return;
  }*/

  if(GET_LEVEL(ch) < 30)
  {
   send_to_char("You are not high enough level to conjure beings...\r\n", ch);
   return;
  }


/***DISPLAYSPELLBOOK STUFF***/
 
  char     buf[256], *buff, buf2[256], rbuf[MAX_STRING_LENGTH], cinfo[MAX_STRING_LENGTH], cinfo2[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH], selectedrecipe[MAX_STRING_LENGTH];
  char buffer[256];
  FILE    *f;
  FILE    *recipelist;
  int line, recfind; 
  unsigned long	linenum = 0;
  long recnum, choice2;
  long selected = 0;
  P_char tobj;
 
	
  //Create buffers for name
  strcpy(buf, GET_NAME(ch));
  buff = buf;
  for (; *buff; buff++)
  *buff = LOWER(*buff);
  //buf[0] snags first character of name
  sprintf(Gbuf1, "Players/%c/%s.spellbook", buf[0], buf);
  recipelist = fopen(Gbuf1, "r");
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
       half_chop(argument, first, rest);
       half_chop(rest, second, rest);
       choice2 = atoi(second);

 
 if (!*argument)
  {
  send_to_char("&+WThese are the &+mmys&+Mtic&+Wal commands for &+Yconjuring&+W:\n&n", ch);
  send_to_char("&+W(&+wconjure stat <number> &+m- &+mreveal statistical properties about this &+Mminion&n.)\n&n", ch);
  send_to_char("&+W(&+wconjure summon <number> &+m- &+mcall the &+Mminion&+m into existence&n.)\n&n", ch);
  send_to_char("&+MYou have learned the following &+mMobs&+M:\n&n", ch);
  send_to_char("----------------------------------------------------------------------------\n", ch);
  send_to_char("&+M Mob Number		          &+mMob Name		&n\n\r", ch);
      while((fscanf(recipelist, "%i", &recnum)) != EOF )
	{  
       if(recnum == choice2)
       selected = choice2;
      
    tobj = read_mobile(recnum, VIRTUAL);
 	sprintf(rbuf, "%d\n", recnum);
    sprintf(buffer, "   &+W%-22d&n%-41s&n\n", recnum, tobj->player.short_descr);
	//stores the actual vnum written in file into rbuf 
	page_string(ch->desc, buffer, 1);
    send_to_char("----------------------------------------------------------------------------\n", ch);
	  extract_char(tobj);
   	}
     fclose(recipelist);
  /***ENDDISPLAYRECIPES***/

  return;
  }

  while((fscanf(recipelist, "%i", &recnum)) != EOF )
	{  
       if(recnum == choice2)
       selected = choice2;
     
 	sprintf(rbuf, "%d\n", recnum);
	}
  fclose(recipelist);
 

  if (is_abbrev(first, "stat"))
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
    send_to_char("&+rYou open your &+Yconjurers &+Lt&+mo&+Mm&+We &+rwhich &+Rreveals&+r the following information...&n.\n", ch);
    get_class_string(tobj, cinfo2);
    sprintf(cinfo, "You glean they are: \r\n&+YLevel &+W%d \r\n&+YClass:&n %s \r\n&+YBase Hitpoints:&n %d\r\n", GET_LEVEL(tobj), get_class_string(tobj, buf2), GET_MAX_HIT(tobj));
    send_to_char(cinfo, ch);
    extract_char(tobj);
    return;
  }
 
  else if (is_abbrev(first, "summon"))
  {

    if(selected == 0)
     {
      send_to_char("&+mIt appears you have not yet &+Mlearned&+m how to conjure that &+Mminion&+m.&n\n", ch);
      return;
     }



    int duration;
    tobj = read_mobile(selected, VIRTUAL);

    if(selected != 400003)
    {
    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_AIR) && !IS_HUMANOID(tobj))
    {
     send_to_char("You cannot summon a being outside of your area of expertise.\r\n", ch);
    extract_char(tobj);
     return;
    }

    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_WATER) && !IS_ELEMENTAL(tobj))
    {
     send_to_char("You cannot summon a being outside of your area of expertise.\r\n", ch);
    extract_char(tobj);
     return;
    }

    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_EARTH) && !IS_ANIMAL(tobj))
    {
     send_to_char("You cannot summon a being outside of your area of expertise.\r\n", ch);
    extract_char(tobj);
     return;
    }
    }

    if(!new_summon_check(ch, tobj))
    {
     send_to_char("You have too many, or too powerful followers to summon this minion.\r\n", ch);
    extract_char(tobj);
     return;
    }


    if(affected_by_spell(ch, SPELL_CONJURE_ELEMENTAL))
    {
     send_to_char("You must wait a short time before calling another &+Yminion&n into existence.\r\n", ch);
    extract_char(tobj);
     return;
    }

    if((GET_LEVEL(tobj) > 56) && !vnum_in_inv(ch, 400231) && !IS_TRUSTED(ch))
    {
     send_to_char("You must have a &+Ya &+Mgreater&+Y o&+Mr&+Bb &+Yof &+YConjuring&n in your &+Winventory&n in order to &+Ysummon&+ a being of such &+Mgreat&n power.\r\n", ch);
    extract_char(tobj);
     return;
    }

      struct affected_type af;

    #define TARGET_CONJ_MOB = tobj;

    //Set up NPCACT etc.
    if(tobj->points.base_hit > 8000)
    GET_MAX_HIT(tobj) = GET_HIT(tobj) = tobj->points.base_hit = 8000;
	 
     REMOVE_BIT(tobj->specials.affected_by, AFF_SLEEP);
     //REMOVE_BIT(tobj->specials.act, ACT_SENTINEL); Needed for mob to follow.
     REMOVE_BIT(tobj->specials.act, ACT_ELITE);
     REMOVE_BIT(tobj->specials.act, ACT_HUNTER);
     GET_EXP(tobj) = 0;
     apply_achievement(tobj, TAG_CONJURED_PET);
    if(!IS_SET(tobj->specials.affected_by, AFF_INFRAVISION))
    {
      SET_BIT(tobj->specials.affected_by, AFF_INFRAVISION);
    }
   REMOVE_BIT(tobj->specials.act, ACT_SPEC); 
   REMOVE_BIT(tobj->specials.act, ACT_BREAK_CHARM);


   if(GET_LEVEL(tobj) > 56 && !IS_TRUSTED(ch))
   {
   vnum_from_inv(ch, 400231, 1);
   act("$n &+Ltosses their &+Ya &+Mgreater&+Y o&+Mr&+Bb &+Yof &+YConjuring&n &+Linto the &+Cair&+L, which quickly forms an &+Rextra-dimensional &+lpocket&n!", TRUE, ch, 0,
        tobj, TO_ROOM);
    act("You &+Ltoss your &+Ya &+Mgreater&+Y o&+Mr&+Bb &+Yof &+YConjuring&n &+Linto the &+Cair&+L, which quickly forms an &+Rextra-dimensional &+lpocket&n!", TRUE, ch, 0,
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
    af.duration = 100;
    af.flags = AFFTYPE_SHORT;
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
  sprintf(Gbuf1, "Players/%c/%s.spellbook", buf[0], buf);
  f = fopen(Gbuf1, "w");
  fclose(f);
  f = fopen(Gbuf1, "a");
  fprintf(f, "%d\n", defrec);
  fclose(f);
}

bool new_summon_check(P_char ch, P_char selected)
{
  struct follow_type *k;
  P_char   victim;
  int i, j, count = 0, desired = 0;
  
  desired = GET_LEVEL(selected);

  for (k = ch->followers, i = 0, j = 0; k; k = k->next)
  {
    victim = k->follower;

    if(!IS_PC(victim))
    {
    i += GET_LEVEL(victim);
    count++;
    }
  }
  i += desired;

  if(count >= 4)
  return FALSE;


  if(i > 120)
  return FALSE;


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

 if(GET_SPEC(ch, CLASS_CONJURER, SPEC_AIR) && !IS_HUMANOID(victim))
    {
     send_to_char("You cannot learn to summon a being outside of your area of expertise.\r\n", ch);
    extract_char(victim);
     return;
    }

    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_WATER) && !IS_ELEMENTAL(victim))
    {
     send_to_char("You cannot learn to summon a being outside of your area of expertise.\r\n", ch);
    extract_char(victim);
     return;
    }

    if(GET_SPEC(ch, CLASS_CONJURER, SPEC_EARTH) && !IS_ANIMAL(victim))
    {
     send_to_char("You cannot learn to summon a being outside of your area of expertise.\r\n", ch);
    extract_char(victim);
     return;
    }
	
 int recipenumber = GET_VNUM(victim);
 	
 



  //Create buffers for name
  strcpy(buf, GET_NAME(ch));
  buff = buf;
  for (; *buff; buff++)
    *buff = LOWER(*buff);
  //buf[0] snags first character of name
  sprintf(Gbuf1, "Players/%c/%s.spellbook", buf[0], buf);

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
   send_to_char("Just stop singing.\r\n", ch);
   return;
  }
  

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


  if(count == 0)
   {
    send_to_char("You have no followers to dismiss.\r\n", ch);
    return;
    }

}