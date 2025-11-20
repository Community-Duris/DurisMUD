/****************************************************************************
 *
 *  File: world_quest.c                                      Part of Duris
 *  Usage: world_quest.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark 			Date: 2006-04-17
 * ***************************************************************************

todo:


Version 2
- Change so you can ask solo, small.



 */

#include <vector>
using namespace std;

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fnmatch.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "objmisc.h"
#include "interp.h"
#include "utils.h"
#include "utility.h"
#include "justice.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "weather.h"
#include "sql.h"
#include "world_quest.h"
#include "epic.h"
#include "map.h"

/* * external variables */

extern char *target_locs[];
extern char *set_master_text[];
extern int mini_mode;
extern FILE *help_fl;
extern FILE *info_fl;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern struct class_names class_names_table[];
extern const char *color_liquid[];
extern const char *command[];
extern const char *connected_types[];
extern const char *craftsmanship_names[];
extern const char *dirs[];
extern const char *event_names[];
extern const char *fullness[];
extern const char *language_names[];
extern struct material_data materials[];
extern const char *month_name[];
extern const char *player_bits[];
extern const char *player_law_flags[];
extern const char *player_prompt[];
extern flagDef weapon_types[];
extern const char *weekdays[];
extern const int rev_dir[];
extern const long boot_time;
extern const struct stat_data stat_factor[];
extern const char *size_types[];
extern const char *item_size_types[];
extern long new_exp_table[];  // Arih: Fixed type mismatch bug - was int, should be long
extern int avail_descs;
extern int help_array[27][2];
extern int info_array[27][2];
extern int max_descs;
extern int max_users_playing;
extern int number_of_quests;
extern int number_of_shops;
extern int pulse;
extern int str_weightcarried, str_todam, str_tohit, dex_defense;
extern int top_of_helpt;
extern int top_of_infot;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_zone_table;
extern int used_descs;
extern struct agi_app_type agi_app[];
extern struct command_info cmd_info[];
extern struct dex_app_type dex_app[];
extern struct info_index_element *info_index;
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern uint event_counter[];
extern char *specdata[][MAX_SPEC];
extern long sentbytes;
extern long recivedbytes;
extern const struct race_names race_names_table[];
extern Skill skills[];
extern const mcname multiclass_names[];
extern void displayShutdownMsg(P_char);

void resetQuest(P_char ch)
{
  ch->only.pc->quest_active = 0;
  ch->only.pc->quest_mob_vnum = 0;
  ch->only.pc->quest_type = 0;
  ch->only.pc->quest_accomplished = 0;
  ch->only.pc->quest_started = 0; 
  ch->only.pc->quest_zone_number = -1;
  ch->only.pc->quest_giver = 0;
  ch->only.pc->quest_level = 0;
  ch->only.pc->quest_receiver = 0;
  ch->only.pc->quest_shares_left = 0;
  ch->only.pc->quest_kill_how_many = 0;
  ch->only.pc->quest_kill_original = 0;
  ch->only.pc->quest_map_room = 0;
  ch->only.pc->quest_map_bought = 0;
}

void quest_epic_reward(P_char ch, int type)
{
  if(GET_LEVEL(ch) > 45 && ch->only.pc->quest_level > 45)
  {
    //snprintf(Gbuf1, MAX_STRING_LENGTH, "You gain some epic experience.");
    //act(Gbuf1, FALSE, quest_mob, 0, ch, TO_VICT);

    if(ch->only.pc->quest_level == 46)
      gain_epic(ch, EPIC_QUEST, 0, number(1, 3));

    if(ch->only.pc->quest_level == 47)
      gain_epic(ch, EPIC_QUEST, 0, number(2, 4));

    if(ch->only.pc->quest_level == 48)
      gain_epic(ch, EPIC_QUEST, 0, number(3, 5));

    if(ch->only.pc->quest_level == 49)
      gain_epic(ch, EPIC_QUEST, 0, number(4, 6));

    if(ch->only.pc->quest_level == 50)
      gain_epic(ch, EPIC_QUEST, 0, number(5, 7));

    if(ch->only.pc->quest_level == 51)
      gain_epic(ch, EPIC_QUEST, 0, number(6, 8));

    if(ch->only.pc->quest_level == 52)
      gain_epic(ch, EPIC_QUEST, 0, number(7, 9));

    if(ch->only.pc->quest_level == 53)
      gain_epic(ch, EPIC_QUEST, 0, number(8, 10));

    if(ch->only.pc->quest_level == 54)
      gain_epic(ch, EPIC_QUEST, 0, number(9, 10));

    if(ch->only.pc->quest_level == 55)
      gain_epic(ch, EPIC_QUEST, 0, number(10, 11));

    if(ch->only.pc->quest_level > 55)
      gain_epic(ch, EPIC_QUEST, 0, 15);
  }
}

int quest_exp_reward(P_char ch, int type)
{
  int exp_gain = 0;
  if(GET_LEVEL(ch) <= 30)
    exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.30.andUnder", 1.300);
  else if(GET_LEVEL(ch) <= 40)
    exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.40.andUnder", 1.000);
  else if(GET_LEVEL(ch) <= 50) 
    exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.50.andUnder", 0.800);
  else if(GET_LEVEL(ch) <= 55) 
    exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.55.andUnder", 0.500);
  else 
    exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.other", 1.000);
  return exp_gain;
}

P_obj quest_item_reward(P_char ch)
{
  P_obj reward = read_object(real_object(getItemFromZone(real_zone(ch->only.pc->quest_zone_number))), REAL);

  if(!reward)
  {
    reward = create_random_eq_new(ch, ch, -1, -1);
  }

  if(reward)
  {
    wizlog(56, "quest_item_reward: %s reward was: %s [%d] ivalue %d.", GET_NAME(ch), reward->short_description, OBJ_VNUM(reward), itemvalue(reward));

    REMOVE_BIT(reward->extra_flags, ITEM_SECRET);
    REMOVE_BIT(reward->extra_flags, ITEM_INVISIBLE);
    SET_BIT(reward->extra_flags, ITEM_NOREPAIR);
    REMOVE_BIT(reward->extra_flags, ITEM_NODROP);
  }
  return reward;
}

void quest_full_reward(P_char ch, P_char quest_mob, int type)
{
  char     Gbuf1[MAX_STRING_LENGTH];

  if( !IS_ALIVE(ch) || IS_NPC(ch) )
  {
    return;
  }

  P_obj reward = quest_item_reward(ch);
  if(reward)
  {
    act("$n gives you $q ", TRUE, quest_mob, reward, ch, TO_VICT);
    act("$n gives $N $q.", FALSE, quest_mob, reward, ch, TO_NOTVICT);
    obj_to_char(reward, ch);
  }

  if( GET_CLASS(ch, CLASS_MERCENARY) )
  {
    if( GET_LEVEL(ch) > 24 )
    {
      int temp = GET_LEVEL(ch) * 1256 + number(1,500);
      mobsay(quest_mob, "I know you mercenaries don't work for free. Take this.");
      snprintf(Gbuf1, MAX_STRING_LENGTH, "You receive %s.\r\n", coin_stringv(temp) );
      send_to_char(Gbuf1, ch);
      ADD_MONEY(ch, temp);
    }
    else
    {
      mobsay(quest_mob, "Sorry, but it's hard to take a little pipsqueek like you seriously as a mercenary. Grow up a bit and you'll start earning your keep.");
    }
  }

  quest_epic_reward(ch, type);

  int exp_gain = quest_exp_reward(ch, type);
  gain_exp(ch, NULL, exp_gain, EXP_WORLD_QUEST);

  snprintf(Gbuf1, MAX_STRING_LENGTH, "&+WYou gain some experience.&n");
  act(Gbuf1, FALSE, quest_mob, 0, ch, TO_VICT);

  sql_world_quest_finished(ch, reward);

  resetQuest(ch);
}

void quest_ask(P_char ch, P_char quest_mob)
{
  if(ch->only.pc->quest_active != 1)
    return;

  if(IS_PC(quest_mob))
    return;

  if(	(GET_VNUM(quest_mob) != ch->only.pc->quest_mob_vnum) )
    return;

  if( ch->only.pc->quest_type != FIND_AND_ASK )
    return;

  wizlog(56, "%s finished quest @%s (ask quest)", GET_NAME(ch), quest_mob->player.short_descr );
  do_action(quest_mob, 0, CMD_NOD);

  mobsay(quest_mob, "Thanks for bringing me the message. Here is a little something for your trouble.");

  quest_full_reward(ch, quest_mob, FIND_AND_ASK);
}

void quest_kill(P_char ch, P_char quest_mob)
{

  if( !IS_ALIVE(ch) || !(quest_mob) )
  {
    return;
  }

  if( ch->only.pc->quest_active != 1 || IS_PC(quest_mob)
    || (GET_VNUM(quest_mob) != ch->only.pc->quest_mob_vnum) )
  {
    return;
  }

  if( ch->only.pc->quest_accomplished == 1 )
  {
    send_to_char("This quest is already finished, go visit your quest master for your reward!\r\n", ch);
    return;
  }

  ch->only.pc->quest_kill_how_many++;

  int exp_gain = quest_exp_reward(ch, FIND_AND_KILL);

  if( ch->only.pc->quest_kill_original == 0 )
  {
    send_to_char("&-RMEMORY ERROR! quest_kill_original is zero, inform a god!&n\r\n", ch);
    wizlog(56, "MEMORY ERROR: quest_kill_original is zero!");
    ch->only.pc->quest_kill_original = ch->only.pc->quest_kill_how_many;
  }

  gain_exp(ch, NULL, exp_gain / ch->only.pc->quest_kill_original, EXP_WORLD_QUEST);
  if( number(1, ch->only.pc->quest_kill_original + 1) <= 2 )
  {
    P_obj reward = quest_item_reward(ch);
    obj_to_char(reward, quest_mob);
  }


  if( ch->only.pc->quest_kill_how_many - ch->only.pc->quest_kill_original == 0 )
  {
    send_to_char("&+WCongratulations&n&n&+W, you finished your quest!&n\r\n", ch);
    wizlog(56, "%s finished quest @%s (kill quest)", GET_NAME(ch), quest_mob->player.short_descr );

    if( GET_CLASS(ch, CLASS_MERCENARY) && GET_LEVEL(ch) > 24)
    {
      int temp = GET_LEVEL(ch) * 1256 + number(1,500);
      act("A sneaky gremlin appears near $n and whispers something.", TRUE, ch, 0, 0, TO_ROOM);
      act("A sneaky gremlin appears beside you.", FALSE, ch, 0, 0, TO_CHAR);
      act("A sneaky gremlin whispers '&+LMy master sent me with your payment&N'.", FALSE, ch, 0, 0, TO_CHAR);
      send_to_char_f(ch, "You receive %s.\r\n", coin_stringv(temp));
      ADD_MONEY(ch, temp);
    }

    quest_epic_reward(ch, FIND_AND_KILL);
    sql_world_quest_finished(ch, 0);
    resetQuest(ch);
  }
  else
  {
    send_to_char("&+YCongratulations&+y, you found the right mob, but you're not done yet.\r\n", ch);
  }
}

#define EVIL_CONT   0
#define GOOD_CONT   1
#define UNDER_DARK  2

int get_map_room(int zone_id)
{
  int i2;
  int i;
  struct zone_data *zone = 0;
  char     buf[MAX_STRING_LENGTH],o_buf[MAX_STRING_LENGTH];

  if( zone_id < 0 )
    return -1;

  zone = &zone_table[zone_id];

  for (i = zone->real_bottom; (i != NOWHERE) && (i <= zone->real_top); i++)
    for (i2 = 0; i2 < NUM_EXITS; i2++)
      if (world[i].dir_option[i2])
      {
        if ((world[i].dir_option[i2]->to_room == NOWHERE) ||
            (world[world[i].dir_option[i2]->to_room].zone !=
             world[i].zone))
        {
          if (world[i].dir_option[i2]->to_room == NOWHERE)
            ;
          else
          {
            if(IS_MAP_ROOM (world[i].dir_option[i2]->to_room) )
            {
                  /*
                 snprintf(o_buf, MAX_STRING_LENGTH,
                 " &+Y[&n%5d&+Y](&n%5d&+Y)&n &+R%-5s&n to &+Y[&+R%3d&n:&+Y%5d&+Y](&n%5d&+Y)&n %s\n",
                 world[i].number, i, dirs[i2],
                 world[world[i].dir_option[i2]->to_room].zone,
                 world[world[i].dir_option[i2]->to_room].number,
                 world[i].dir_option[i2]->to_room,
                 world[world[i].dir_option[i2]->to_room].name);
                 wizlog(56, "%s", o_buf);
               */
              return world[i].dir_option[i2]->to_room;
            }
          }
        }
      }
  return -1;

}

void do_quest(P_char ch, char *args, int cmd)
{
  P_char   q_mob;
  P_char   q_giver;
  P_char    victim;

  char    buf[MAX_STRING_LENGTH];
  char    q_name[MAX_STRING_LENGTH];

  char     name[MAX_INPUT_LENGTH], who[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];

  if( IS_NPC(ch) )
  {
    return;
  }

  if( IS_ILLITHID(ch) )
  {
    send_to_char("No quests for you!", ch);
    return;
  }

  half_chop(args, name, who);
  if( *name )
  {
    if (isname(name, "reset") && IS_TRUSTED(ch))
    {
      if(!*who)
      {
        send_to_char("Whos quest do you want to reset?\r\n", ch);
        return;
      }

      victim = ParseTarget(ch, who);
      if(!victim){
        send_to_char("Hmm, who do you mean?\r\n", ch);
        return;
      }

      resetQuest(victim);

      send_to_char("Someone magically cleared your quest.\r\n", victim);
      send_to_char("You've cleared his quest.\r\n", ch);
      return;
    }
    else if( isname(name, "stat") && IS_TRUSTED(ch) )
    {
      pc_only_data *pcdata;

      if( !*who )
      {
        send_to_char("Whos quest do you want to examine?\r\n", ch);
        return;
      }
      if( !(victim = ParseTarget(ch, who)) )
      {
        send_to_char("Hmm, nobody around here by that name.\r\n", ch);
        return;
      }
      if( IS_NPC(victim) )
      {
        send_to_char( "Hmm, mobs don't have quests.  Try again.\n\r", ch );
        return;
      }
      pcdata = victim->only.pc;
      // One big string.. rawr.
      snprintf(buf, MAX_STRING_LENGTH, "Quest for %s (PID %d): \n\rActive: %s, Level: %d, Type: %s, Started: %s, Accomplished: %s\n\r"
        "Bartender Vnum: %d, Shares left: %d, Original Questor PID: %d, Can share: %s\n\r"
        "Target Mob Vnum: %d, Number Killed: %d out of %d needed.\n\r"
        "Zone: %s (%d), Map room: %s (%d), Map purchased: %s.\n\r", J_NAME(victim), GET_PID(victim),
        YESNO(pcdata->quest_active), pcdata->quest_level, (pcdata->quest_type == FIND_AND_ASK) ? "ASK"
        : (pcdata->quest_type == FIND_AND_KILL) ? "KILL" : (pcdata->quest_type == FIND_AND_SOMETHING)
        ? "SOMETHING" : "UNKNOWN", YESNO(pcdata->quest_started), YESNO(pcdata->quest_accomplished),
        pcdata->quest_giver, pcdata->quest_shares_left, pcdata->quest_receiver,
        YESNO(GET_PID(victim) == pcdata->quest_receiver), pcdata->quest_mob_vnum,
        pcdata->quest_kill_how_many, pcdata->quest_kill_original, zone_table[real_zone0(pcdata->quest_zone_number)].name,
        pcdata->quest_zone_number, world[real_room0(pcdata->quest_map_room)].name,
        pcdata->quest_map_room, YESNO(pcdata->quest_map_bought) );
      send_to_char( buf, ch );

      return;
    }
  }

  if(ch->only.pc->quest_active == 0){
    send_to_char("You don't have a quest!\r\n", ch);
    return;
  }


  if(*name)
    if (isname(name, "share"))
    {

      if(ch->only.pc->quest_receiver != GET_PID(ch))
      {
        send_to_char("Only the quest receiver can share the quest.\r\n", ch);
        return;
      }

      /*if(ch->only.pc->quest_kill_how_many != 0)
      {
        send_to_char("You already started the quest alone, so finish it alone!\r\n", ch);
        return;
      }*/
      if(ch->only.pc->quest_shares_left == 0)
      {
        send_to_char("You cant share this quest with more people.\r\n", ch);
        return;
      }

      if(!*who)
      {
        send_to_char("Who do you want to share your quest with?\r\n", ch);
        return;
      }

      victim = ParseTarget(ch, who);
      if(!victim){
        send_to_char("Hmm, who do you mean?\r\n", ch);
        return;
      }

      if(victim->only.pc->quest_active){
        send_to_char("Your fellow adventurer is already on a task.\r\n", ch);
        return;
      }

      if(!is_linked_to(ch, victim, LNK_CONSENT))
      {
        send_to_char("Your fellow adventurer needs to consent you first.\r\n", ch);
        return;
      }

      if(ch->only.pc->quest_accomplished == 1)
      {
        send_to_char("Hehe, sharing it so they get reward without work?!\r\n", ch);
        return;
      }


      if(ch->only.pc->quest_level < GET_LEVEL(victim) - 
         get_property("world.quest.level.high.value", 6))
      {
        send_to_char("They are much too experienced for that quest!\r\n", ch);
        return;
      }
      
      if(ch->only.pc->quest_level > GET_LEVEL(victim) + 
         get_property("world.quest.level.low.value", 6)) 
      { 
        send_to_char("They are too inexperienced for that quest!\r\n", ch); 
        return; 
      }      

      if(sql_world_quest_can_do_another(victim) < 1)
      {

        send_to_char("You are unable to receive a new quest, maybe you have already done a few recently?\r\n", victim);
        send_to_char("They are unable to receive your quest, you need to find another friend to join you.\r\n", ch);
        return;
      }

      if(sql_world_quest_done_already(victim, ch->only.pc->quest_mob_vnum))
      {
        send_to_char("You are unable to receive a new quest, maybe you have already done this quest??\r\n", victim);
        send_to_char("They've already done this quest!\r\n", ch);
        return;

      }
      resetQuest(victim);

      ch->only.pc->quest_shares_left--;

      send_to_char("You receive a quest.\r\n", victim);

      send_to_char("You've shared your quest.\r\n", ch);

      victim->only.pc->quest_active =  ch->only.pc->quest_active;
      victim->only.pc->quest_mob_vnum  = ch->only.pc->quest_mob_vnum;
      victim->only.pc->quest_type = ch->only.pc->quest_type;
      victim->only.pc->quest_accomplished  = ch->only.pc->quest_accomplished;
      victim->only.pc->quest_started = ch->only.pc->quest_started;
      victim->only.pc->quest_zone_number = ch->only.pc->quest_zone_number;
      victim->only.pc->quest_giver = ch->only.pc->quest_giver;
      victim->only.pc->quest_level =  ch->only.pc->quest_level;
      victim->only.pc->quest_receiver = ch->only.pc->quest_receiver;
      victim->only.pc->quest_kill_how_many = ch->only.pc->quest_kill_how_many;
      victim->only.pc->quest_kill_original =  ch->only.pc->quest_kill_original;
      do_quest(victim, "", 0);


      return;
    }



  if ((q_giver = read_mobile(real_mobile(ch->only.pc->quest_giver), REAL)))
  {
    snprintf(q_name, MAX_STRING_LENGTH, "%s", q_giver->player.short_descr);
    snprintf(buf, MAX_STRING_LENGTH,"%s gave you the following quest:\r\n", q_name);
    send_to_char(buf, ch);

    if (q_giver)
    {
      extract_char(q_giver);
      q_giver = NULL;
    }
  }
  else
  {
    wizlog(56, "UNABLE TO LOAD Q MOB:%d for char %s reseting his quest.", ch->only.pc->quest_giver, GET_NAME(ch));
    send_to_char("HMMMMM (bug), Seems like you forgot your quest already, go grab a new one, if this one dont work, try another bartender around the world. Sorry we working on a fix ASAP.!\r\n", ch);
    resetQuest(ch);	
    return;
  }

  //wizlog(56, "questmob num:%d",ch->only.pc->quest_mob_vnum);
  if ((q_mob = read_mobile(real_mobile(ch->only.pc->quest_mob_vnum), REAL)))
  {
    snprintf(q_name, MAX_STRING_LENGTH, "%s", q_mob->player.short_descr);


    if(ch->only.pc->quest_type == FIND_AND_ASK)
    {
      snprintf(buf, MAX_STRING_LENGTH,"Go ask %s in %s about the %s.\r\n", q_name, zone_table[real_zone0(ch->only.pc->quest_zone_number)].name,  month_name[time_info.month]);
      send_to_char(buf, ch);

    }
    else if(ch->only.pc->quest_type == FIND_AND_KILL)
    {
      snprintf(buf, MAX_STRING_LENGTH, "Go kill %d %s (%d left) in %s!\r\n", ch->only.pc->quest_kill_original, q_name,  (ch->only.pc->quest_kill_original - ch->only.pc->quest_kill_how_many),  zone_table[real_zone0(ch->only.pc->quest_zone_number)].name );
      send_to_char(buf, ch);
      wizlog(56, "%s got a quest to go kill %d of mob vnum %d; they have %d left.", 
           GET_NAME(ch), ch->only.pc->quest_kill_original, ch->only.pc->quest_mob_vnum,
           (ch->only.pc->quest_kill_original - ch->only.pc->quest_kill_how_many));
    }

    if (q_mob)
    {
      extract_char(q_mob);
      q_mob = NULL;
    }
  }
  else
  {
    send_to_char("HMMMMMMMMMM, something's broken with your quest, tell a god ):(2)\r\n", ch);
    return;
  }
  //always call this

  if(ch->only.pc->quest_map_bought == 1)
  {
    send_to_char("You also got some additional information from the quest master:\r\n", ch);
    show_map_at(ch, real_room(ch->only.pc->quest_map_room) );
  }

  if( ch->only.pc->quest_accomplished == 1)
  {
    send_to_char("This quest is already finished, go talk to the quest master!\r\n", ch);
  }

  if(ch->only.pc->quest_receiver != GET_PID(ch)){
    send_to_char("&+RThis quest was shared with you&n.\r\n", ch);

  }
  else {
    send_to_char("&+RThis quest was given to you&n.\r\n", ch);
    snprintf(buf, MAX_STRING_LENGTH, "You can share this quest with %d more people.\r\n", ch->only.pc->quest_shares_left);
    send_to_char(buf, ch);
  }

  send_to_char("If you're unable to finish it, go to the quest master and ask him to &+Wabandon&n your quest!\r\n", ch);

}

// Attempts to create a quest for ch (a PC), given by giver (a NPC).
// Returns TRUE if successful, and FALSE if failed.
bool createQuest(P_char ch, P_char giver)
{
  int quest_zone = -1;
  int quest_mob  = -1;
  int QUEST_TYPE = -1;

  // Fail if missing an arg, or ch not a PC, or giver not an NPC or God.
  if( !giver || !ch || IS_NPC(ch) || (IS_PC(giver) && !IS_TRUSTED(giver)) )
  {
    return FALSE;
  }

  //No valid zone found return FALSE
  vector<int> valid_zones;
  getQuestZoneList(ch, valid_zones);

  if( valid_zones.empty() )
  {
    wizlog(56, "Unable to find a quest zone for %s", GET_NAME(ch));
    return FALSE;
  }

  for (int z = 0; z < 10; z++)
  {
    quest_zone  = valid_zones[number(0, valid_zones.size() - 1)];

    for (int m = 0; m < 3; m++)
    {
      QUEST_TYPE = number(1,2);
      if(GET_LEVEL(ch) > 49)
      {
        QUEST_TYPE = FIND_AND_KILL;
      }
      //wizlog(56, "suggesting zone:%s for this dude",zone_table[quest_zone].name);
      quest_mob = suggestQuestMob(quest_zone,ch, QUEST_TYPE);
      // If there aren't enough mobs to complete a quest (Need 2 for kill, 1 for ask)..
      if( quest_mob > 0 && ((QUEST_TYPE == FIND_AND_KILL && mob_index[real_mobile(quest_mob)].number <= 1)
        || (QUEST_TYPE == FIND_AND_ASK && mob_index[real_mobile(quest_mob)].number < 1)) )
      {
        quest_mob = -1;
      }
      //wizlog(56, "suggesting quest mob :%d for this dude",quest_mob);
      if( quest_mob == -1 && GET_LEVEL(ch) < 50 )
      {
        // Swap Quest type and try again.
        QUEST_TYPE = (QUEST_TYPE == FIND_AND_KILL) ? QUEST_TYPE = FIND_AND_ASK : QUEST_TYPE = FIND_AND_KILL;

        quest_mob = suggestQuestMob(quest_zone,ch, QUEST_TYPE);
        // If there aren't enough mobs to complete a quest (Need 2 for kill, 1 for ask)..
        if( quest_mob > 0 && ((QUEST_TYPE == FIND_AND_KILL && mob_index[real_mobile(quest_mob)].number <= 1)
          || (QUEST_TYPE == FIND_AND_ASK && mob_index[real_mobile(quest_mob)].number < 1)) )
        {
          quest_mob = -1;
          break;
        }
      }

      if(sql_world_quest_done_already(ch, quest_mob))
      {
        quest_mob = -1;
      }

      if (quest_mob != -1)
        break;
    }
    if (quest_mob != -1)
      break;
  }


  if(quest_mob == -1)
  {
    wizlog(56, "Unable to find a valid quest for %s", GET_NAME(ch));
    return FALSE;
  }

  ch->only.pc->quest_shares_left = 4;
  ch->only.pc->quest_active = 1;
  ch->only.pc->quest_mob_vnum = quest_mob;
  ch->only.pc->quest_type = QUEST_TYPE;
  ch->only.pc->quest_accomplished = 0;
  ch->only.pc->quest_started = time(NULL);
  ch->only.pc->quest_zone_number = zone_table[quest_zone].number;
  ch->only.pc->quest_giver = GET_VNUM(giver);
  ch->only.pc->quest_level = GET_LEVEL(ch);
  ch->only.pc->quest_receiver = GET_PID(ch);

  int rnum = real_mobile(quest_mob);

  ch->only.pc->quest_kill_original =  MIN(number(7,9) , mob_index[rnum].number - 1);
  debug("Quest Kill Original Value: %d, mob_index number: %d, mob_index limit: %d, mob_vnum: %d",
    ch->only.pc->quest_kill_original, mob_index[rnum].number, mob_index[rnum].limit -1, mob_index[rnum].virtual_number);

  return TRUE;
}

void show_map_at(P_char ch, int room)
{
  if( !IS_ALIVE(ch) || IS_NPC(ch) )
  {
    return;
  }

  if( !(room) )
  {
    send_to_char("&+W* There is a bug with the map for this quest please contact an Admin.&n.\r\n", ch);
    logit(LOG_DEBUG, "World quest: (%s) failed in show_map_at for room (%d).", GET_NAME(ch), room);
    return;
  }

  if(ch->only.pc->quest_map_bought == 1)
  {
    if(room == -1)
    {
      send_to_char("&+W* This zone is not connected to the overland maps of duris&n.\r\n", ch);
      return;
    }

    int old_room = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, room, -2);
    display_map(ch, 20, 999);
    char_from_room(ch);
    char_to_room(ch, old_room, -2);
    return;
  }
}


int quest_buy_map(P_char ch)
{
  if( !IS_ALIVE(ch) || IS_NPC(ch) )
  {
    return 0;
  }

  if(ch->only.pc->quest_map_bought == 1)
  {
    send_to_char("Your memory is bad, you dont have to pay me for this, you can just type 'quest'\r\n.", ch);
    return 0;
  }

  ch->only.pc->quest_map_bought = 1;
  send_to_char("The 'quest' command will now show you additional information about the quest.\r\n", ch);

  int map_room = get_map_room(real_zone(ch->only.pc->quest_zone_number));
  
  //debug("quest_zone_number: %d, real_zone: %d, map_room: %d", ch->only.pc->quest_zone_number, real_zone(ch->only.pc->quest_zone_number), map_room);
  
  if( map_room > 0 )
  {
    ch->only.pc->quest_map_room = world[map_room].number;    
    //debug("quest_map_room: %d", ch->only.pc->quest_map_room);
    show_map_at(ch, map_room);
  }
  
  return 1;
}


// Return random vnum for item from zone.
int getItemFromZone(int zone)
{
  /* For debugging...
  char     buf[MAX_STRING_LENGTH];
  */
  long     ct, diff_time;
  char    *tmstr;
  int      count, i, choice, zone_count, world_index, room_count, length;
  struct zone_data *z_num = &zone_table[zone];
  P_obj    t_obj;
  P_char   t_mob;

  int valid_items[1000];
  int list = 0;

  length = 0;
  for( i = 0; i <= top_of_objt; i++ )
  {
    if ((obj_index[i].virtual_number >= world[z_num->real_bottom].number) &&
        (obj_index[i].virtual_number <= world[z_num->real_top].number))
    {
      // Easier to just load one, and free it, than load only ones that aren't already in game.
      if ((t_obj = read_object(obj_index[i].virtual_number, VIRTUAL)))
      {
        if( IS_SET(t_obj->extra_flags, ITEM_NORENT) || IS_SET(t_obj->str_mask, STRUNG_KEYS)
          || !IS_SET(t_obj->wear_flags, ITEM_TAKE) || IS_SET(t_obj->extra_flags, ITEM_TRANSIENT)
          // These are _allowed_ types
          || (t_obj->type != ITEM_WAND && t_obj->type != ITEM_STAFF
            && t_obj->type != ITEM_ARMOR && t_obj->type != ITEM_WORN
            && t_obj->type != ITEM_BOOK && t_obj->type != ITEM_QUIVER
            && t_obj->type != ITEM_INSTRUMENT && t_obj->type != ITEM_SPELLBOOK
            && t_obj->type != ITEM_TOTEM && t_obj->type != ITEM_SHIELD
            && t_obj->type != ITEM_FIREWEAPON && t_obj->type != ITEM_WEAPON
            && t_obj->type != ITEM_POTION)
          || IS_SET(t_obj->bitvector, AFF_STONE_SKIN) || IS_SET(t_obj->bitvector, AFF_HIDE)
          || IS_SET(t_obj->bitvector, AFF_SNEAK) || IS_SET(t_obj->bitvector, AFF_FLY)
          || IS_SET(t_obj->bitvector, AFF4_NOFEAR) || IS_SET(t_obj->bitvector2, AFF2_AIR_AURA)
          || IS_SET(t_obj->bitvector2, AFF2_EARTH_AURA) || IS_SET(t_obj->bitvector3, AFF3_INERTIAL_BARRIER)
          || IS_SET(t_obj->bitvector3, AFF3_REDUCE) || IS_SET(t_obj->bitvector2, AFF2_GLOBE)
          || IS_SET(t_obj->bitvector, AFF_HASTE) || IS_SET(t_obj->bitvector, AFF_DETECT_INVISIBLE)
          || IS_SET(t_obj->bitvector4, AFF4_DETECT_ILLUSION) || IS_NOSHOW(t_obj) || GET_OBJ_WEIGHT(t_obj) > 99
          || IS_ARTIFACT(t_obj) || isname("_noquest_", t_obj->name) || IS_OBJ_STAT2(t_obj, ITEM2_QUESTITEM) )
        {
          extract_obj(t_obj, FALSE);
          continue;
        }
        /* Debugging code.
        snprintf(buf, MAX_STRING_LENGTH, "%6d  %5d  %-s\n", obj_index[i].virtual_number, obj_index[i].number - 1,
          (t_obj->short_description) ? t_obj->short_description : "None");
        wizlog(56, buf);
        */

        valid_items[list++] = obj_index[i].virtual_number;
        extract_obj(t_obj, FALSE);
        t_obj = NULL;

      }
      else
      {
        logit(LOG_DEBUG, "do_world(): obj %d not loadable", obj_index[i].virtual_number);
      }
    }
  }

  if( list == 0 )
    return -1;

  return valid_items[number(0, list-1)];
}

bool isInvalidQuestZone(int zone_number)
{
  struct zone_info zone;
  if( !get_zone_info(zone_number, &zone) )
    return TRUE;
  
  return !zone.quest_zone;  
}


void getQuestZoneList(P_char ch, vector<int> &valid_zones)
{
  //If lvl 15 or lower it's same zone as the dude.
  //if( GET_LEVEL(ch) < 15)
  //	return world[ch->in_room].zone;

  int curZone = world[ch->in_room].zone;
  vector<int> curMapExits;
  bool curUDExit = false;
  bool curUCExit = false;
  for (int i = zone_table[curZone].real_bottom; (i != NOWHERE) && (i <= zone_table[curZone].real_top); i++)
  {
    for (int i2 = 0; i2 < NUM_EXITS; i2++)
    {
      if (!world[i].dir_option[i2])
        continue;
      int to_room = world[i].dir_option[i2]->to_room;
      if (to_room == NOWHERE)
        continue;
      if (world[to_room].zone == curZone)
        continue;
      if (IS_MAP_ROOM(to_room))
      {
          curMapExits.push_back(to_room);
          if (IS_UD_MAP(to_room))
            curUDExit = true;
          if (world[to_room].continent == CONT_UC)
            curUCExit = true;
      }
      else
      { // lets try looking zone ahead
        int nextZone = world[to_room].zone;
        for (int j = zone_table[nextZone].real_bottom; (j != NOWHERE) && (j <= zone_table[nextZone].real_top); j++)
        {
          for (int j2 = 0; j2 < NUM_EXITS; j2++)
          {
            if (!world[j].dir_option[j2])
              continue;
            int to_room1 = world[j].dir_option[j2]->to_room;
            if (to_room1 == NOWHERE)
              continue;
            if (world[to_room1].zone == curZone || world[to_room1].zone == nextZone)
              continue;
            if (!IS_MAP_ROOM(to_room1))
              continue;

            int k = 0;
            for (; k < curMapExits.size(); k++)
              if (curMapExits[k] == to_room1)
                break;
            if (k == curMapExits.size())
            {
              curMapExits.push_back(to_room1);
              if (IS_UD_MAP(to_room1))
                curUDExit = true;
              if (world[to_room1].continent == CONT_UC)
                curUCExit = true;
            }
          }
        }
      }
    }
  }

  //Add zones to list that's -5 - -15 in avg levels
  for (int zone_count = 0; zone_count <= top_of_zone_table; zone_count++)
  {
    if( isInvalidQuestZone(zone_table[zone_count].number) ||
        zone_table[zone_count].hometown != 0 ||
        zone_table[zone_count].avg_mob_level < 0 )
     continue;
    
    if( zone_table[zone_count].avg_mob_level >= (GET_LEVEL(ch) + 5) || 
        zone_table[zone_count].avg_mob_level <= (GET_LEVEL(ch) - 7))
      continue;
 
    if(zone_count == curZone) //do not use same zone as the dude is in..
      continue;

    if (GET_LEVEL(ch) >= 51)
    {
      valid_zones.push_back(zone_count);
      continue;
    }

    int zone_maproom = get_map_room(zone_count);
    if(zone_maproom == -1)
        continue;

    if (curMapExits.size() == 0)
    {  // bartender doesnt have exits to map, gives only to 40+
      if (GET_LEVEL(ch) >= 40)
        valid_zones.push_back(zone_count);
      continue;
    }
 
    for (int i = 0; i < curMapExits.size(); i++)
    {
      int cur_maproom = curMapExits[i];
      if (GET_LEVEL(ch) < 30)
      { 
        if (world[zone_maproom].map_section != world[cur_maproom].map_section)
        { // same continent below 30.
          continue;
        }
        int distance = calculate_map_distance(zone_maproom, cur_maproom);
        //debug("Distance between %d and %d is %d", world[zone_maproom].number, world[cur_maproom].number, distance);
        if(distance > (3000 + GET_LEVEL(ch) * GET_LEVEL(ch) * GET_LEVEL(ch)))
        { // mostly affects 25-
          continue;
        }
      }
      if (IS_UD_MAP(zone_maproom) && !curUDExit && GOOD_RACE(ch) && GET_LEVEL(ch) < 40) 
      { // only bartenders with direct exit to UD give UD quests to goods below 40
        continue;
      }
      if (world[zone_maproom].continent == CONT_UC && !curUCExit && (GET_LEVEL(ch) < 35 || (GOOD_RACE(ch) && GET_LEVEL(ch) < 40))) 
      { // evils dont get UC quests till 35, goods till 40 (unless its UC bartender)
        continue;
      }
      valid_zones.push_back(zone_count);
      break;
    }
  }
}


int suggestQuestMob(int zone_num, P_char ch, int QUEST_TYPE)
{
  P_char   t_mob;
  int count, i;
  int valid_mobs[300];
  int list = 0;
  int KIND_OF_QUEST = -1;
  int MAX_LEVEL = GET_LEVEL(ch);


  if(QUEST_TYPE == 0)
    KIND_OF_QUEST = number(1,2);
  else
    KIND_OF_QUEST = QUEST_TYPE;


  if(KIND_OF_QUEST == FIND_AND_KILL) //FIND AND KILL A MOB
  {
    MAX_LEVEL = MAX_LEVEL + 6;
  }
  if(KIND_OF_QUEST == FIND_AND_ASK) //FIND AND TALK TO HIM
  {
    MAX_LEVEL = 62;
  }


  //wizlog(56, "Looking for a vald mob in: %s KIND_OF_QUEST = %d MAX_LEVEL=%d", zone_table[zone_num].name, KIND_OF_QUEST, MAX_LEVEL);
  for (i = 0; i <= top_of_mobt; i++)
  {
    if ((mob_index[i].virtual_number >= world[zone_table[zone_num].real_bottom].number) &&
        (mob_index[i].virtual_number <= world[zone_table[zone_num].real_top].number))
    {

      if ((t_mob = read_mobile(mob_index[i].virtual_number, VIRTUAL)))
      {

        if(IS_SET(t_mob->specials.act, ACT_SPEC))
        {
          REMOVE_BIT(t_mob->specials.act, ACT_SPEC);
        }
        
        char_to_room(t_mob, 1, -2);

        if(GET_LEVEL(t_mob) < MAX_LEVEL  &&
           GET_LEVEL(t_mob) > GET_LEVEL(ch) - 5 &&
           KIND_OF_QUEST == FIND_AND_KILL ||
           GET_LEVEL(t_mob) < MAX_LEVEL &&
           KIND_OF_QUEST == FIND_AND_ASK)
        {
          if(mob_index[i].number == 2 &&
             KIND_OF_QUEST == FIND_AND_ASK ||
             KIND_OF_QUEST == FIND_AND_KILL && mob_index[i].number > 2)
          {

            if(KIND_OF_QUEST == FIND_AND_KILL )
            {
              ch->only.pc->quest_kill_how_many = 0;/*
              ch->only.pc->quest_kill_original =  MIN(number(7,9) , mob_index[i].limit - 1);
              debug("Quest Kill Original Value: %d, mob_index number: %d, mob_index limit: %d, mob_vnum: %d", 
               ch->only.pc->quest_kill_original, mob_index[i].number, mob_index[i].limit -1, mob_index[i].virtual_number);*/
            }


            if(aggressive_to(t_mob, ch) &&
               KIND_OF_QUEST == FIND_AND_ASK || 
               !CAN_SPEAK(t_mob) && KIND_OF_QUEST == FIND_AND_ASK)
            {
              //dont suggest aggresive ask mobs..nor not humanoids
              ;
            }
            else{
              valid_mobs[list] = mob_index[i].virtual_number;
              list++;
            }
          }
        }

        if (t_mob)
        {
          extract_char(t_mob);
          t_mob = NULL;
        }
      }
      else
      {
        logit(LOG_DEBUG, "do_world(): mob %d not loadable",
            mob_index[i].virtual_number);
      }

    }
  }

  if(list == 0)
  {
    return -1;
  }
  else
  {
    return valid_mobs[number(0, list-1)];
  }
}

//Called from comm.c to populate avg_mob_level
int calc_zone_mob_level()
{
  // Use C-style arrays instead of std::vector to avoid destructor issues
  float *avg_mob_level = (float *)calloc(top_of_zone_table+1, sizeof(float));
  float *mob_count = (float *)calloc(top_of_zone_table+1, sizeof(float));

  if (!avg_mob_level || !mob_count) {
    if (avg_mob_level) free(avg_mob_level);
    if (mob_count) free(mob_count);
    return 0;
  }

  for( P_char tch = character_list; tch; tch = tch->next )
  {
    if( !IS_NPC(tch) )
      continue;

    if( GET_ZONE(tch) > top_of_zone_table )
      continue;

    avg_mob_level[GET_ZONE(tch)] =
      ( (avg_mob_level[GET_ZONE(tch)] * mob_count[GET_ZONE(tch)]) + (float) GET_LEVEL(tch) ) / ( mob_count[GET_ZONE(tch)] + 1.0);
    mob_count[GET_ZONE(tch)] += 1.0;
  }

  for( int i = 0; i <= top_of_zone_table; i++ )
  {
    if( mob_count[i] < 1 )
    {
      zone_table[i].avg_mob_level = -1;
    }
    else
    {
      zone_table[i].avg_mob_level = (int) avg_mob_level[i];
    }
  }

  free(avg_mob_level);
  free(mob_count);
  return 0;
}


