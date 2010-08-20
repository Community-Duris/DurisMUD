/****************************************************************************
 *
 *  File: randomeq.c                                           Part of Duris
 *  Usage: randomboject.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark                   Date: 2002-04-18
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

#define SMITH_MAX_ITEMS   20

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

//extern const int material_absorbtion[][];
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
extern void material_restrictions(P_obj);
extern int find_map_place();
extern struct forge_item forge_item_list[]; 

void     set_keywords(P_obj t_obj, const char *newKeys);
void     set_short_description(P_obj t_obj, const char *newShort);
void     set_long_description(P_obj t_obj, const char *newDescription);

struct mine_range_data {
  char *name;
  char *abbrev;
  int start;
  int end;
  int reload;
} mine_data[] = {
  //Zone display name, command argument matching, start range, end range, reloading mines?
  {"Surface Map", "map", 500000, 659999, TRUE},
  {"Underdark", "ud", 700000, 859999, TRUE},
//  {"Tharnadia Rift", "tharnrift", 110000, 119999, FALSE},
  {0}
};

struct mines_event_data {
  int map;
};

int forge_prices[] = {2000, 5000, 15000, 30000, 50000};

struct smith_data {
  int vnum;
  short int items[SMITH_MAX_ITEMS];
} smith_array[] = {
  {76010, { 131, 132, 53, 54, 55, 56}},
  {82518, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}},
  {22269, { 92, 93, 94, 95, 96}},
  {76691, { 88, 89, 91, 103}}, 
  {20240, { 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22}},
  {40712, { 41, 42, 43, 44, 45, 46, 47}},
  {99716, { 62, 63, 64, 65, 66, 67, 68, 69, 76, 77, 78, 79, 80}},
  {9412, {60, 61, 49, 50, 70, 71, 72, 73, 73, 75}},   
  {96058, {103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 119}},
  {6002, {6, 7, 8, 9, 10, 23, 24, 25, 26, 30, 37, 38, 39, 40, 113}},
  {66410, {127, 128, 129, 130, 88, 89, 90, 91}},
  {1420, {70, 57, 58, 59, 48, 36, 38, 37, 39, 40}}, // kobolds
  {96916, {57, 58, 59, 27, 29, 97, 101, 102, 107, 109, 112}}, //troll caves
  {75628, {12, 13, 14,  66, 67, 67, 64, 65, 77, 76}}, //citadel
  {76502, {60, 61, 34, 31, 32, 33, 74, 75, 88, 89, 92}},
  {28917, {46, 47, 49, 45, 35, 78, 77, 86, 87, 85}}, //torg
  {37755, {30, 26, 22, 23, 24, 25, 21, 11, 12, 13, 14, 15, 16}}, //nax
  {0}
};

int mines_properties(int map)
{
  switch (map)
  {
  case MINES_MAP_SURFACE:
    return (int)get_property("mines.maxSurfaceMap", 50);
    break;
  case MINES_MAP_UD:
    return (int)get_property("mines.maxUD", 50);
    break;
  case MINES_MAP_THARNRIFT:
    return (int)get_property("mines.maxTharnRift", 50);
    break;
  default:
    logit(LOG_DEBUG, "mines_properties(): passing invalid map to function, using default 50 mines");
    return 50;
    break;
  }
}

P_obj get_hammer(P_char ch)
{
  P_obj    t_obj, next_obj;

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    if (obj_index[t_obj->R_num].virtual_number == HAMMER_VNUM)
    {
      return t_obj;
    }
  }

  return NULL;
}

// #define IS_MINING_PICK(obj) ( GET_OBJ_VNUM(obj) == 253 || \
                              // GET_OBJ_VNUM(obj) == 338 || \
                              // GET_OBJ_VNUM(obj) == 10640 || \
                              // GET_OBJ_VNUM(obj) == 95531 || \
                              // GET_OBJ_VNUM(obj) == 49018 )
                              
#define IS_MINING_PICK(obj) (isname("pick", obj->name) && \
                             obj->type == ITEM_WEAPON)

P_obj get_pick(P_char ch)
{
  if( !ch )
    return NULL;
  
  if( ch->equipment[WIELD] && IS_MINING_PICK( ch->equipment[WIELD] ) )
    return ch->equipment[WIELD];

  if( ch->equipment[WIELD2] && IS_MINING_PICK( ch->equipment[WIELD2] ) )
    return ch->equipment[WIELD2];

  return NULL;
}

int parchment_forge(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char temp_ch;
  if(!ch)
    return FALSE;

  if(cmd != CMD_RECITE)
    return FALSE;

  temp_ch = obj->loc.wearing;

  if(!temp_ch)
    return FALSE;;

  if(ch != temp_ch)
    return FALSE;


  for (int tmp = 1; tmp < MAX_FORGE_ITEMS; tmp++)
  {
    if(ch->only.pc->learned_forged_list[tmp] == obj->value[0])
    {
      send_to_char("You already know how to forge this item.\r\n", ch);
      return TRUE;
    }

    if( forge_item_list[ch->only.pc->learned_forged_list[obj->value[0]]].skill_min >
        GET_CHAR_SKILL(ch, SKILL_FORGE))
    {
      send_to_char("This parchment contains smithing too advanced for you.\r\n", ch);
      return TRUE;

    }
    if(ch->only.pc->learned_forged_list[tmp] == 0)  
    {
      ch->only.pc->learned_forged_list[tmp] = obj->value[0];
      send_to_char("You learn a new item to forge!\r\n", ch);
      extract_obj(obj, TRUE);
      return TRUE;
    } 
  }
}

void create_parchment(P_char ch)
{
  P_obj obj = read_object(PARCHMENT_VNUM, VIRTUAL);
  if(!obj)
    return;

  int howmany = 0;
  for (int tmp = 1; tmp < MAX_FORGE_ITEMS; tmp++)
  {
    if(forge_item_list[tmp].id == 0)
      break;

    howmany++;
  }
  obj->value[0] = number(1,howmany); 

  //If it's not rare dont load it!
  if( forge_item_list[ch->only.pc->learned_forged_list[obj->value[0]]].how_rare < number(1,100))
    return; 

  obj_to_room(obj, ch->in_room);
  act("$n's digging turns up $p&n!", TRUE, ch, obj, 0, TO_ROOM);
  act("Your digging turns up $p&n!", TRUE, ch, obj, 0, TO_CHAR);
}

P_obj forge_create(int choice, P_char ch, int material)
{
  char     dummy[MAX_INPUT_LENGTH];
  char     short_desc[MAX_STRING_LENGTH];
  char     long_desc[MAX_INPUT_LENGTH];
  char     keywords[MAX_INPUT_LENGTH];
  P_obj obj = read_object(1255, VIRTUAL); 

  if(!obj)
  {
    wizlog(56, "unabled to load object 1255");
    return 0;
  } 
  sprintf(keywords, "%s", forge_item_list[choice].keywords);
  sprintf(dummy, "%s", forge_item_list[choice].short_desc);

  sprintf(short_desc, dummy, GET_NAME(ch));

  sprintf(dummy, "%s", forge_item_list[choice].long_desc);
  sprintf(long_desc, dummy, GET_NAME(ch));

  set_short_description(obj, short_desc);
  set_long_description(obj, long_desc);         
  set_keywords(obj, keywords);

  obj->material = material;
  obj->affected[0].location = forge_item_list[choice].loc0;
  obj->affected[0].modifier = number(forge_item_list[choice].min0, forge_item_list[choice].max0);                   
  obj->affected[1].location = forge_item_list[choice].loc1;
  obj->affected[1].modifier = number(forge_item_list[choice].min1, forge_item_list[choice].max1);

  SET_BIT(obj->wear_flags, ITEM_TAKE);
  SET_BIT(obj->wear_flags, forge_item_list[choice].wear_flags);

  SET_BIT(obj->bitvector, forge_item_list[choice].aff1);
  SET_BIT(obj->bitvector2, forge_item_list[choice].aff2);
  SET_BIT(obj->bitvector3, forge_item_list[choice].aff3);
  SET_BIT(obj->bitvector4, forge_item_list[choice].aff4);   

  obj->anti_flags |= forge_item_list[choice].classes;

  if(forge_item_list[choice].allow_anti)
    obj->extra_flags = ITEM_ALLOWED_CLASSES;;
  if (isname("quiver", obj->name)){    
    obj->value[0] = number(20, 80);
    obj->value[1] = number(0, 1);
    obj->value[2] = 1;
    obj->value[3] = 0;
    obj->type = ITEM_QUIVER;
  }
  else
    obj->type == ITEM_ARMOR;
  return obj;
}

void forge_describe(int choice, P_char ch)
{
  char buffer[1024];
  int i;

  sprintf(buffer, "To create %s you need:\n", forge_item_list[choice].short_desc);  
  for (i = 0; i < 5 && forge_item_list[choice].ore_needed[i]; i++)
    sprintf(buffer + strlen(buffer), "%s\n", 
        obj_index[real_object(forge_item_list[choice].ore_needed[i])].desc2 ); 

  send_to_char(buffer, ch);
  sprintf(buffer, "It will cost you %s to forge this.\n", coin_stringv(forge_prices[i-1]));
  send_to_char(buffer, ch);
}

P_obj check_foundry(P_char ch)
{
  P_obj t_obj;

  for (t_obj = world[ch->in_room].contents; t_obj; t_obj = t_obj->next_content)
  {
    if (obj_index[t_obj->R_num].virtual_number == 361)
    {
      return t_obj;
    }
  }

  return NULL;
}

void do_forge(P_char ch, char *argument, int cmd)
{
  char     buf1[MAX_STRING_LENGTH];
  char     first[MAX_INPUT_LENGTH];
  char     second[MAX_INPUT_LENGTH];
  char     rest[MAX_INPUT_LENGTH];
 
  int i = 0;  
  int      choice = 0;  
  P_obj hammer, foundry;

  if (GET_CHAR_SKILL(ch, SKILL_FORGE) == 0){
    send_to_char("&+LYou dont know how to forge.\r\n", ch);
    return;
  }

  foundry = check_foundry(ch);

  if (!foundry) { // No furnace/foundry/forge in room
	act("&+LYou need to be by your foundry to forge...&n", FALSE, ch, 0, 0, TO_CHAR);
	return;
  }

  *buf1 = '\0';
  if (!*argument){
    sprintf(buf1 + strlen(buf1), "Syntax: 'forge info #', 'forge create #'\n"); 
    sprintf(buf1 + strlen(buf1), "You know how to forge the following items:\n");
    if(IS_TRUSTED(ch))
      for (int tmp = 1; tmp < MAX_FORGE_ITEMS; tmp++)
      {
        if(forge_item_list[tmp].short_desc == NULL)
          break;
        sprintf(buf1 + strlen(buf1), "%d -  %s\n", tmp, forge_item_list[tmp].short_desc);
      }
    else

      for (int tmp = 1; tmp < MAX_FORGE_ITEMS; tmp++)
      {
        if(ch->only.pc->learned_forged_list[tmp] != 0)
          sprintf(buf1 + strlen(buf1), "%d -  %s\n", tmp, forge_item_list[ch->only.pc->learned_forged_list[tmp]].short_desc );
      }
    strcat(buf1, "\n");
    page_string(ch->desc, buf1, 1);
    return;
  }

  sprintf(buf1, "");
  half_chop(argument, first, rest);
  half_chop(rest, second, rest);
  choice = (ush_int) atoi(second);

  if (choice < 1 || choice >= MAX_FORGE_ITEMS) {
    send_to_char("This is not a valid number.\n", ch);
    return;
  }

  if (is_abbrev(first, "info"))
  { 

    if(!choice){
      send_to_char("Missing forge list number 'forge info 1'.\r\n", ch);   
      return;      
    }
    if(IS_TRUSTED(ch))
      choice = choice;
    else
      choice  = ch->only.pc->learned_forged_list[choice];

    if(!choice){
      send_to_char("You do not know how to make that. Type 'forge' to get a list of know items..\r\n", ch);
      return;
    }

    forge_describe(choice, ch);
    return;
  }

  if (is_abbrev(first, "create"))
  {
    if(!choice){
      send_to_char("Missing forge list number 'forge create 1'.\r\n", ch);
      return;
    }
    if(IS_TRUSTED(ch))
      choice = choice;
    else

      choice  = ch->only.pc->learned_forged_list[choice];
    if(!choice){
      send_to_char("You do not know how to make that. Type 'forge' to get a list of know items..\r\n", ch);
      return;
    }

    hammer = get_hammer(ch);
    if(!hammer)
    {
      act("You need to have a forge hammer in your inventory.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }

    if(forge_item_list[choice].short_desc == NULL)
    {
      send_to_char("Yes you are a god, but why try to create something that's not even possible?!?!.\r\n", ch);
      return;
    }

    int material = 0;     
    int NEEDED = 0;
    int FOUND = 0;
    P_obj t_obj, next_obj;
    while(forge_item_list[choice].ore_needed[i] != 0)
    {
      NEEDED++;
      for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
      {
        next_obj = t_obj->next_content;
        if(obj_index[t_obj->R_num].virtual_number == forge_item_list[choice].ore_needed[i]){
          if(!material)
            material = t_obj->material;
          sprintf(buf1, "You start to work on %s\n", t_obj->short_description );
          send_to_char(buf1, ch);
          extract_obj(t_obj, TRUE);
          FOUND++;
          if(!number(0,2))
            if(GET_CHAR_SKILL(ch, SKILL_FORGE) < number(0, 99))
            {
              send_to_char("You swing wildly, slip, fall, hurt yourself, and destroy the ore!\r\n", ch);
              return;
            }       
          break;
        }
      }
      i++;
    }
    if(FOUND == NEEDED || IS_TRUSTED(ch))
    {
      if(FOUND == 0)
        send_to_char("You are a god! Material type is created from the ore, since you used none it will be wrong..fyi!\n",ch); 
      P_obj obj = forge_create(choice, ch, material);
      if (!obj)
        return;
      obj_to_char(obj, ch);
      sprintf(buf1, "You finishing forging: %s!\n", obj->short_description); 
      send_to_char(buf1, ch);  
      wizlog(56, "%s forged %s" , GET_NAME(ch), buf1);
      notch_skill(ch, SKILL_FORGE, 1);
      return;
    }
    else
    {
      send_to_char("You do not  have the required items. Type forge info # to find out what you need. You wasted some ore.\r\n", ch);
      return;
    }
    return;
  }
}

bool mine_friendly(int to_room) {
  if (world[to_room].sector_type == SECT_HILLS)
    return true;

  if (world[to_room].dir_option[NORTH] &&
      (world[world[to_room].dir_option[NORTH]->to_room].sector_type == SECT_HILLS ||
       world[world[to_room].dir_option[NORTH]->to_room].sector_type == SECT_MOUNTAIN))
    return true;
  
  if (world[to_room].dir_option[EAST] &&
      (world[world[to_room].dir_option[EAST]->to_room].sector_type == SECT_HILLS ||
       world[world[to_room].dir_option[EAST]->to_room].sector_type == SECT_MOUNTAIN))
    return true;
  
  if (world[to_room].dir_option[SOUTH] &&
      (world[world[to_room].dir_option[SOUTH]->to_room].sector_type == SECT_HILLS ||
       world[world[to_room].dir_option[SOUTH]->to_room].sector_type == SECT_MOUNTAIN))
    return true;

  if (world[to_room].dir_option[WEST] &&
      (world[world[to_room].dir_option[WEST]->to_room].sector_type == SECT_HILLS ||
       world[world[to_room].dir_option[WEST]->to_room].sector_type == SECT_MOUNTAIN))
    return true;

  return false;
}

int get_mine_content(P_obj mine)
{
  return mine->value[0];
}

int remove_mine_content(P_obj mine)
{
  return (mine->value[0]--);
}

int random_ore(int mine_quality)
{
  int x = number(1, 99 + mine_quality * 9);
    
  if(x >= 98 + mine_quality * 8)
    return LARGE_MITHRIL_ORE;
  if(x >= 96 + mine_quality * 7) 
    return MEDIUM_MITHRIL_ORE;	
  if(x >= 94 + mine_quality * 6)
    return SMALL_MITHRIL_ORE;

  if(x >= 91 + mine_quality * 5)
    return LARGE_PLATINUM_ORE; 
  if(x >= 88 + mine_quality * 4)
    return MEDIUM_PLATINUM_ORE; 
  if(x >= 85 + mine_quality * 3)
    return SMALL_PLATINUM_ORE;

  if(x >= 81 + mine_quality * 2)
    return LARGE_GOLD_ORE;
  if(x >= 77 + mine_quality * 1)
    return MEDIUM_GOLD_ORE;
  if(x >= 73 + mine_quality * 0)
    return SMALL_GOLD_ORE;

  if(x >= 66)
    return LARGE_SILVER_ORE;
  if(x >= 59)
    return MEDIUM_SILVER_ORE;
  if(x >= 52)
    return SMALL_SILVER_ORE;

  if(x >= 44)
    return LARGE_COPPER_ORE;
  if(x >= 36)
    return MEDIUM_COPPER_ORE;
  if(x >= 28)
    return SMALL_COPPER_ORE;

  if(x >= 19)
    return LARGE_IRON_ORE;
  if(x >= 10)
    return MEDIUM_IRON_ORE;

  return SMALL_IRON_ORE;
}

P_obj get_ore_from_mine(P_char ch, int mine_quality)
{
  P_obj ore;
  int ore_type = random_ore(mine_quality);
  ore = read_object(ore_type, VIRTUAL);
  if(!ore)
    return NULL;
  ore->value[4] = time(NULL);
  return ore;
}

P_obj get_pole(P_char ch)
{

  P_obj    t_obj, next_obj;

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    if (obj_index[t_obj->R_num].virtual_number == POLE_VNUM ||
        obj_index[t_obj->R_num].virtual_number == 6025  ||
        obj_index[t_obj->R_num].virtual_number == 26200 ||
        obj_index[t_obj->R_num].virtual_number == 43120 ||
        obj_index[t_obj->R_num].virtual_number == 66709 ||
        obj_index[t_obj->R_num].virtual_number == 93913 ||
        obj_index[t_obj->R_num].virtual_number == 29439 ||
        obj_index[t_obj->R_num].virtual_number == 88903
       )
    {
      return t_obj;
    }
  }
  return NULL;
}

void do_fish(P_char ch, char *arg, int cmd)
{
  char     buf[MAX_STRING_LENGTH]; 
  const int fishes[12] = {293 , 294 , 295 , 318 , 319 , 330 , 332 , 333 , 334 , 335, 355, 356};
  int random = number(0,11);


  if (IS_NPC(ch)) {
     return;
  }

  if(GET_CHAR_SKILL(ch, SKILL_FISHING) == 0)
  {
    update_skills(ch);
  }

  P_obj pole = get_pole(ch);
  if(!pole)
  {
    send_to_char("You need a fishing pole to fish!\n", ch);
    return;

  }
  if(!IS_WATER_ROOM(ch->in_room))
  {
    send_to_char("Well you DO have a fishing pole, but where do you plan to fish???\n", ch);
    return;
  } 

  CharWait(ch, (int) (PULSE_VIOLENCE)); 
  if (notch_skill(ch, SKILL_FISHING, get_property("skill.notch.fishing", 70)) ||
      GET_CHAR_SKILL(ch, SKILL_FISHING)/10 < number(1,100) )  
  {
    send_to_char("You didn't catch a thing..\n", ch);
    return;
  }

  P_obj fish = read_object(fishes[random], VIRTUAL);

  if(!fish)
  {
    wizlog(56, "Bug with fishing no fish with vnum %d", fishes[random]);
    return;
  }

  if(!number(0,2))
    DamageOneItem(ch, 9, pole, FALSE); 

  act("You reel in $p on your fishing pole!", FALSE,
      ch, fish, NULL, TO_CHAR);

  act("$n reels in $p on $s fishing pole", FALSE,
      ch, fish, NULL, TO_ROOM);

  fish->timer[0] = time(NULL);
  obj_to_char(fish, ch);
}


int mine(P_obj obj, P_char ch, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC )
    return TRUE;
  
  if( cmd == CMD_PERIODIC )
  {
    if( obj->value[0] <= 0 )
    {
      extract_obj(obj, TRUE);
      return TRUE;
    }
  }
  
  if( cmd == CMD_MINE )
  {
    if( !ch || !IS_PC(ch) )
      return FALSE;
    
    if( GET_CHAR_SKILL(ch, SKILL_MINE) == 0 )
    {
      send_to_char("You don't know how to mine.\n", ch);
      return TRUE;      
    }
    
    if( get_scheduled(ch, event_mine_check) )
    {
      send_to_char("You're already mining!\n", ch);
      return TRUE;
    }
    
    if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
    {
      send_to_char("You're too relaxed to mine.\n", ch);
      return TRUE;
    }
    
    P_obj pick = get_pick(ch);
    if( !pick )
    {
      send_to_char("You need to be wielding a suitable mining pick.\n", ch);
      return TRUE;      
    }
    
    if( get_mine_content(obj) <= 0 )
    {
      send_to_char("This area has been completely deplenished!\n", ch);
      return TRUE;      
    }
    
    // start mining
    send_to_char("You begin to mine...\n", ch);

    struct mining_data data;
    data.room = ch->in_room;
    data.counter = (140 - GET_CHAR_SKILL(ch, SKILL_MINE)) /3;
    data.mine_quality = obj->value[1];

    remove_mine_content(obj);
    
    if( get_mine_content(obj) <= 0 )
    {
      send_to_char("There is very little left, but you keep digging one more time!\n", ch);
      extract_obj(obj, TRUE);
    }
    
    add_event( event_mine_check, PULSE_VIOLENCE, ch, 0, 0, 0, &data, sizeof(struct mining_data));
    return TRUE;
  }
  
  return FALSE;
}

void event_mine_check(P_char ch, P_char victim, P_obj, void *data)
{
  struct mining_data *mdata = (struct mining_data*)data;
  P_obj ore, pick;
  char  buf[MAX_STRING_LENGTH];
  
  pick = get_pick(ch);

  if (!ch->desc ||
      IS_FIGHTING(ch) ||
      (ch->in_room != mdata->room) ||            
      !MIN_POS(ch, POS_STANDING + STAT_NORMAL) ||                    
      IS_SET(ch->specials.affected_by, AFF_HIDE) ||
      IS_IMMOBILE(ch) ||
      !AWAKE(ch) ||
      IS_STUNNED(ch) ||
      IS_CASTING(ch) ||
      IS_AFFECTED2(ch, AFF2_CASTING))
  {
    send_to_char("You stop mining.\n", ch);
    return;  
  }

  if(!pick)
  {
    send_to_char("How are you supposed to mine when you don't have anything ready to mine with?\n", ch);
    return;
  }
  
  if (mdata->counter == 0 )
  {
    ore = get_ore_from_mine(ch, mdata->mine_quality);
    
    if(!ore)
    {
      wizlog(56, "Problem with ore item");
      return;
    }
    
    act("Your mining efforts turn up $p&n!", FALSE, ch, ore, 0, TO_CHAR);
    act("$n finds $p&n!", FALSE, ch, ore, 0, TO_ROOM);

    obj_to_room(ore, ch->in_room);
    return;
  }

  if (GET_VITALITY(ch) < 10)
  {
    send_to_char("You are too exhausted to continue mining.\n", ch);
    return;
  }

  if (!notch_skill(ch, SKILL_MINE, get_property("skill.notch.mining", 30)) &&
      !number(0, (GET_CHAR_SKILL(ch, SKILL_MINE) * 2) ))
  {
    send_to_char("You thought you found something, but it was just worthless rock.\n", ch);  
    return; 
  }

  if(!number(0, 999))
    create_parchment(ch); 

  if(!number(0,4) && DamageOneItem(ch, 1, pick, false))
    return;

  send_to_char("You continue mining...\n", ch);
  GET_VITALITY(ch) -= (number(0,100) > GET_CHAR_SKILL(ch, SKILL_MINE)) ? 3 : 2;

  mdata->counter--;
  add_event(event_mine_check, PULSE_VIOLENCE, ch, 0, 0, 0, mdata, sizeof(struct mining_data));
}

int smith(P_char ch, P_char pl, int cmd, char *arg)
{
  struct smith_data *sdata;
  char buffer[256];
  P_obj tobj, needed_ore[5];
  int i, j, choice;

  if (cmd == 0 && !number(0,6)) {
    act("$n hums a soft melody as $e forges another piece of armor.",
        FALSE, ch, 0, 0, TO_ROOM);
    return FALSE;
  }
  
  if (cmd != CMD_FORGE)
    return FALSE;

  for (i = 0; smith_array[i].vnum; i++)
    if (smith_array[i].vnum == GET_VNUM(ch))
      break;

  if (smith_array[i].vnum == 0)
    return FALSE;
  else
    sdata = smith_array + i;

  if (!arg || !*arg) {
    act("$n tells you, 'I can forge the following items:'", FALSE, ch, 0, pl, TO_VICT);
    for (i = 0; i < SMITH_MAX_ITEMS && sdata->items[i]; i++) {
      sprintf(buffer, "%d) %s\n", i + 1, forge_item_list[sdata->items[i]].short_desc);
      send_to_char(buffer, pl);
    }
    return TRUE;
  }

  choice = atoi(arg);

  if (choice < 1 || choice >= SMITH_MAX_ITEMS || sdata->items[choice - 1] == 0)
    return FALSE;

  choice = sdata->items[choice - 1];

  memset(needed_ore, 0, sizeof(needed_ore));
  for (i = 0; forge_item_list[choice].ore_needed[i]; i++) {
    for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {
      if (forge_item_list[choice].ore_needed[i] == 
          obj_index[tobj->R_num].virtual_number) {
        for (j = 0; needed_ore[j] && needed_ore[j] != tobj; j++)
          ;
        if (needed_ore[j] == 0) {
          needed_ore[j] = tobj;
          break;
        }
      }
    }
    if (!tobj) {
      forge_describe(choice, pl);
      return TRUE;
    }
  }

  if (GET_MONEY(pl) < forge_prices[i-1]) {
    forge_describe(choice, pl);
    return TRUE;
  }

  sprintf(buffer, "You hand $N %s.", coin_stringv(forge_prices[i-1]));
  act(buffer, FALSE, pl, 0, ch, TO_CHAR);
  SUB_MONEY(pl, forge_prices[i-1], 0);

  tobj = forge_create(choice, pl, needed_ore[0]->material);

  for (i = 0; i < 5 && needed_ore[i]; i++)
    extract_obj(needed_ore[i], FALSE);

  act("$n takes the pieces of ore and starts to work on them.\n"
      "With powerful hammer strikes, the material starts to quickly take\n"
      "shape of $p.\n"
      "$n puts it in water which explodes in a cloud of &+Csteam&n hissing loudly.\n"
      "'&+WThere you go!&n', $n gives you $p.", FALSE, ch, tobj, pl, TO_VICT);
  act("$n takes the pieces of ore and starts to work on them.\n"
      "With powerful hammer strikes, the material starts to quickly take\n"
      "shape of $p.\n"
      "$n puts it in water which explodes in a cloud of &+Csteam&n hissing loudly.\n"
      "'&+WThere you go!&n', $n gives $N $p.", FALSE, ch, tobj, pl, TO_NOTVICT);

  if (tobj)
    obj_to_char(tobj, pl);

  return TRUE;
}

void initialize_tradeskills()
{
  int i;

  obj_index[real_object0(MINE_VNUM)].func.obj = mine;
  
  for (i = 0; mine_data[i].name; i++)
  {  
    load_mines(mine_data[i].reload, TRUE, i);
  }
  
  // set procs on smiths
  for (i = 0; smith_array[i].vnum; i++)
    if (!mob_index[real_mobile0(smith_array[i].vnum)].func.mob)
      mob_index[real_mobile0(smith_array[i].vnum)].func.mob = smith;
}

bool invalid_mine_room(int rroom_id)
{
  if( IS_SET(world[rroom_id].room_flags, PRIVATE) || 
      IS_SET(world[rroom_id].room_flags, PRIV_ZONE) || 
      //IS_SET(world[rroom_id].room_flags, NO_TELEPORT) || 
      world[rroom_id].dir_option[DOWN] || 
      IS_WATER_ROOM(rroom_id) || 
      world[rroom_id].sector_type == SECT_MOUNTAIN || 
      world[rroom_id].sector_type == SECT_ROAD || 
      world[rroom_id].sector_type == SECT_UNDRWLD_MOUNTAIN || 
      world[rroom_id].sector_type == SECT_UNDRWLD_NOGROUND ||
      world[rroom_id].sector_type == SECT_UNDRWLD_NOSWIM ||
      world[rroom_id].sector_type == SECT_UNDRWLD_WATER ||
      world[rroom_id].sector_type == SECT_UNDRWLD_INSIDE ||
      world[rroom_id].sector_type == SECT_UNDRWLD_CITY ||
      world[rroom_id].sector_type == SECT_OCEAN ||
      world[rroom_id].sector_type == SECT_INSIDE )
    return TRUE;
  
  for( P_obj tobj = world[rroom_id].contents; tobj; tobj = tobj->next )
  {
    if( GET_OBJ_VNUM(tobj) == MINE_VNUM )
      return TRUE;
  }

  return FALSE;
}

bool load_one_mine(int map)
{
  P_obj mine = read_object(MINE_VNUM, VIRTUAL);
  
  if( !mine )
  {
    wizlog(56, "Error loading mine obj [%d]", MINE_VNUM);
    return FALSE;
  }
  
  int start = real_room(mine_data[map].start);
  int end = real_room(mine_data[map].end);

  int tries = 0;
  int to_room = -1;
  
  do {
    to_room = number(start, end);
    
    /* if it's valid and a mine friendly location, or just lucky, put a mine there */
    if( !invalid_mine_room(to_room) && 
        ( mine_friendly(to_room) || number(0,100) < 15 ) )
      break;
    
    tries++;
  } while ( tries < 10000 );
  
  if( tries >= 10000 || invalid_mine_room(to_room) )
  {
    return FALSE;
  }
  
  int random = number(0,99);
  
  if( random < 3 )
  {
    mine->value[0] = number(20, 30);
    mine->value[1] = 3;
    mine->description = str_dup("&+LThe &+yearth &+Lhere is &+cbr&+Lim&+Cm&+Ling with &+Yore&+L - it's the &+GMother &+LLode!&n");
  }  
  else if( random < 20 )
  {
    mine->value[0] = number(10, 15);
    mine->value[1] = 2;
    mine->description = str_dup("&+LThe &+yearth&+L here is &+cst&+Lrea&+ck&+Led &+Lwith &+core&+L.&n");
  }
  else if( random < 60 )
  {
    mine->value[0] = number( 8, 12);
    mine->value[1] = 1;
    mine->description = str_dup("&+LA few chunks of &+Yore &+Lpoke out of the ground here.&n");
  }
  else
  {
    mine->value[0] = number( 5, 10);
    mine->value[1] = 0;
    mine->description = str_dup("&+LA few glimmers &+Ws&+wpa&+Wrk&+wle&+L in the &+yearth &+Lhere.&n");
  }
  
  obj_to_room(mine, to_room);
  wizlog(56, "Mine (%d) loaded in %s [%d]", mine->value[1], world[to_room].name, ROOM_VNUM(to_room) );
  
  return TRUE;
}

void load_mines(bool set_event, bool load_all, int map)
{
  int max_mines, num_mines = 0;
  struct mines_event_data mdata;

  for( P_obj tobj = object_list; tobj; tobj = tobj->next )
  {
    if( (GET_OBJ_VNUM(tobj) == MINE_VNUM) &&
        (world[tobj->loc.room].number >= mine_data[map].start) &&
        (world[tobj->loc.room].number <= mine_data[map].end) )
    {
      num_mines++;
    }
  }

  max_mines = mines_properties(map) + number(-5,5);
  debug("mines currently loaded: %d / %d", num_mines, max_mines );

  if( num_mines < max_mines )
  {
    if( load_all )
    {
      for( int i = 0; (i < ( max_mines - num_mines )); i++ )
      {
        load_one_mine(map);
      }
    }
    else
    {
      load_one_mine(map);
    }
  }
  
  mdata.map = map;

  if( set_event )
    add_event(event_load_mines, ( WAIT_SEC * 60 ) * ((int) get_property("mines.reloadMins", 10)), NULL, NULL, 0, 0, &mdata, sizeof(mdata));
}

void event_load_mines(P_char ch, P_char victim, P_obj, void *data)
{
  struct mines_event_data *mdata = (struct mines_event_data*)data;

  if (!mdata)
  {
    debug("Passed null pointer to event_load_mines()");
    return;
  }

  load_mines(TRUE, FALSE, mdata->map);
}

P_obj get_bandage(P_char ch)
{
  P_obj    t_obj, next_obj;

  for (t_obj = ch->carrying; t_obj; t_obj = next_obj)
  {
    next_obj = t_obj->next_content;
    if (t_obj->type == ITEM_BANDAGE)
    {
      return t_obj;
    }
  }

  return NULL;
}

int poison_common_remove(P_char ch);

struct bandage_data {
  int room;
  int healed;
  int maxheal;
};
	 
void event_bandage_check(P_char ch, P_char victim, P_obj, void *data)
{
  struct bandage_data *mdata = (struct bandage_data*)data;
  P_obj bandage;
  char  buf[MAX_STRING_LENGTH];
  int healed = 0;

  if(!ch || !victim)
    return;

  if (mdata->healed >= mdata->maxheal) {
    sprintf(buf, "You can't &+Wbandage&n any more with this bandage.\n");
    send_to_char(buf, ch);
    return;
  }

  if (!ch->desc ||
      IS_FIGHTING(ch) ||
      (ch->in_room != mdata->room) ||
      (GET_STAT(ch) < STAT_SLEEPING) ||                    
      IS_SET(ch->specials.affected_by, AFF_HIDE) ||
      IS_FIGHTING(victim) ||
      (victim->in_room != mdata->room) ||            
      IS_SET(victim->specials.affected_by, AFF_HIDE) ||
      affected_by_spell(ch, TAG_FIRING) ||
      affected_by_spell(victim, TAG_FIRING) ||
      IS_AFFECTED2(ch, AFF2_MEMORIZING) ||
      IS_AFFECTED2(victim, AFF2_MEMORIZING) ||
      IS_IMMOBILE(ch))
  {
    send_to_char("You are no longer bandaging...\n", ch);
    send_to_char("You are no longer being &+Wbandaged&n.\n", victim);
    return;  
  }

  if (GET_VITALITY(ch) < 10) {
    send_to_char("You are too exhausted to continue.\n", ch);
    send_to_char("You are no longer being &+Wbandaged&n.\n", victim);
    return;
  }

  if (!notch_skill(ch, SKILL_BANDAGE, get_property("skill.notch.bandage", 30)) &&
      !number(number(0,3), (GET_CHAR_SKILL(ch, SKILL_BANDAGE) ) ))
  {
    send_to_char("You are not focused enough, you destroy the &+Wbandage&n.\n", ch);  
    send_to_char("You are no longer being &+Wbandaged&n.\n", victim);
    return; 
  }

  if(ch!= victim)
    act("You continue &+Wbandaging&n $N.", FALSE, ch, 0,victim, TO_CHAR);
  else
    act("You continue &+Wbandaging&n yourself.", FALSE, ch, 0, victim, TO_CHAR);

  act("$n continues to &+Wbandage&n you.", FALSE, ch, 0,
      victim, TO_VICT);
  act("$n continues to &+Wbandage&n $N", FALSE, ch, 0,
      victim, TO_NOTVICT);

  healed = (mdata->maxheal / 20 + GET_CHAR_SKILL(ch, SKILL_BANDAGE) / 7);
  healed = MIN(healed, mdata->maxheal - mdata->healed);
  mdata->healed = mdata->healed + healed;
  healed = vamp(victim, healed, GET_MAX_HIT(victim) * 0.875);
  update_pos(victim);

  if( IS_AFFECTED2(victim, AFF2_POISONED) && 
      !number(0, 4) &&
      ( number(0,100) < GET_CHAR_SKILL(ch, SKILL_BANDAGE)) )
  {
    send_to_char("&+WThe poison in your bloodstream is neutralized!\n", victim);
    poison_common_remove(victim);
  }

  if (healed == 0)
  {
    send_to_char("&+WYour bandage won't do any more good now&n.\n", ch); 
    send_to_char("You are no longer being bandaged.\n", victim);
    return; 
  }

  GET_VITALITY(ch) -= 
    (number(0,100) > GET_CHAR_SKILL(ch, SKILL_BANDAGE)) ? 3 : 2;
  add_event(event_bandage_check, PULSE_VIOLENCE, ch, victim
      , 0, 0, mdata, sizeof(struct bandage_data));
}

void do_mine(P_char ch, char *arg, int cmd)
{

  if( !ch || IS_NPC(ch) )
    return;
 
  // Anyone wana take a crack at this below to make it work correctly?
  // If you don't get the idea, give me a hollar.
  // From hearing Torgal's responses to it as well as knowing nobody ever uses
  // this command, i'm going to go ahead and get the engine in game. -Venthix

 
  int i;
  char buff[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  half_chop(arg, arg1, arg2);
  //one_argument(arg, buff);

  debug("(arg) %s, (arg1) %s, (arg2) %s", arg, arg1, arg2);

  if( !strcmp(arg1, "reset") && IS_TRUSTED(ch) )
  {
    for (i = 0; mine_data[i].start; i++)
    {
      if (isname(arg2, mine_data[i].abbrev))
      {
        sprintf(buf2, "purge %s", mine_data[i].abbrev);
        do_mine(ch, buf2, CMD_MINE);
        wizlog(56, "%s loaded mines in %s", GET_NAME(ch), mine_data[i].name);
        logit(LOG_WIZ, "%s loaded mines in %s", GET_NAME(ch), mine_data[i].name);
        load_mines(FALSE, TRUE, i);
        return;
      }
    }
    sprintf(buf2, "Available options for mine reset: map | ud\n");
    /*
      for (i = 0; mine_data[i].start; i++);
      {
      strcat(buf2, (mine_data[i].abbrev));
      if (mine_data[i+1].abbrev)
        strcat(buf2, " | ");
    }
    strcat(buf2, "\n");
    */
    send_to_char(buf2, ch);
  }
  else if( !strcmp(arg1, "load") && IS_TRUSTED(ch) )
  {
    for (i = 0; mine_data[i].start; i++)
    {
      if (!strcmp(arg, mine_data[i].abbrev))
      {
        wizlog(56, "%s loaded mine in %s", GET_NAME(ch), mine_data[i].name);
        logit(LOG_WIZ, "%s loaded mine in %s", GET_NAME(ch), mine_data[i].name);
        load_one_mine(i);
        return;
      }
    }
    sprintf(buf2, "Available options for mine load: map | ud\n");
    /*for (i = 0; mine_data[i].abbrev; i++);
    {
      debug("%s", mine_data[i].abbrev);
      strcat(buf2,  mine_data[i].abbrev);
      if (mine_data[i+1].abbrev)
        strcat(buf2, " | ");
    }
    strcat(buf2, "\n");
    */
    send_to_char(buf2, ch);
  }  
  else if( !strcmp(buff, "purge") && IS_TRUSTED(ch) )
  {
    P_obj tobj = object_list;
    P_obj next = object_list->next;

    for (i = 0; mine_data[i].start; i++)
    {
      if (!strcmp(arg, mine_data[i].abbrev))
        break;
    }

    for ( ; tobj && next; tobj = next )
    {
      next = tobj->next;

      if( (GET_OBJ_VNUM(tobj) == MINE_VNUM) &&
          (!strcmp(arg, mine_data[i].abbrev)) &&
	  (world[tobj->loc.room].number >= mine_data[i].start) &&
          (world[tobj->loc.room].number <= mine_data[i].end) )
      {
        extract_obj(tobj, TRUE);
      } 
      // The all factor
      else if ( GET_OBJ_VNUM(tobj) == MINE_VNUM &&
          (!strcmp(arg, "all")) )
      {
        extract_obj(tobj, TRUE);
      }
    }
    
    if (!strcmp(arg, "all"))
    {
      wizlog(56, "%s purged all mines.", GET_NAME(ch));
      logit(LOG_WIZ, "%s purged all mines.", GET_NAME(ch));
      return;
    }
    else if (!strcmp(arg, mine_data[i].abbrev))
    {
      wizlog(56, "%s purged %s mines.", GET_NAME(ch), mine_data[i].name);
      logit(LOG_WIZ, "%s purged %s mines.", GET_NAME(ch), mine_data[i].name);
      return;
    }
    else
    {
      sprintf(buf2, "Available options for mine purge: all | map | tharnrift\n");
      /*
      for (i = 0; mine_data[i].start; i++);
      {
        strcat(buf2, mine_data[i].abbrev);
        if (mine_data[i+1].abbrev)
          strcat(buf2, " | ");
      }
      strcat(buf2, " | all\n");
      */
      send_to_char(buf2, ch);
    }
  }
  
  send_to_char("You can't mine here!\n", ch);
}


void do_bandage(P_char ch, char *arg, int cmd)
{
  struct affected_type af;
  char     name[MAX_INPUT_LENGTH];
  struct bandage_data data;
  char  buf1[MAX_STRING_LENGTH];
  P_char   victim = NULL;
  P_obj bandage;

  P_char   t_char = NULL;

  if (!SanityCheck(ch, "do_bandage"))
    return;

  if (IS_NPC(ch))
  {
    send_to_char("You're too NPC-like to try.\r\n", ch);
    return;
  }

  if (get_scheduled(ch, event_bandage_check))
  {
    send_to_char("Yes you are &+Wbandaging&n.\n", ch);
    return;
  }

  victim = ParseTarget(ch, arg);
  if (!victim)
  {
    send_to_char("Who are you trying to &+Wbandage&n?\r\n", ch);
    return;
  }

  if (affected_by_spell(victim, SKILL_BANDAGE) && GET_HIT(victim) >= 0)
  {
    send_to_char("&+WBandaging&n again wont do any good now.\r\n", ch);
    return;
  }

  if (IS_PC(ch) && GET_CHAR_SKILL(ch, SKILL_BANDAGE) == 0)
  {
    send_to_char
      ("Maybe you should leave that to someone skilled in battlefield first aid.\r\n",
       ch);
    return;
  }

  if (IS_FIGHTING(victim) || IS_FIGHTING(victim) )
  {
    send_to_char("The battle in room prevents you from that.\r\n", ch);
    return;
  }

  if (NumAttackers(victim))
  {
    send_to_char
      ("That person is still being attacked, maybe you should wait?\r\n", ch);
    return;
  }

  if (GET_HIT(victim) > GET_MAX_HIT(victim) - (GET_MAX_HIT(victim) / 8))
  {
    if(ch == victim)
      send_to_char("Try again when you are more hurt..\r\n", ch);
    else
      send_to_char("Try again when they more hurt.\r\n", ch);

    return;
  }

  bandage = get_bandage(ch);

  if(!bandage)
  {
    act("You have no &+Wbandage&n.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  data.maxheal = bandage->value[0];
  extract_obj(bandage, TRUE);

  if(ch!= victim)
    act("You attempt to &+Wbandage&n $N.", FALSE, ch, 0,victim, TO_CHAR);
  else
    act("You attempt to &+Wbandage&n yourself.", FALSE, ch, 0, victim, TO_CHAR);

  act("$n attempts to &+Wbandage&n you.", FALSE, ch, 0,
      victim, TO_VICT);
  act("$n attempts to &+Wbandage&n $N", FALSE, ch, 0,
      victim, TO_NOTVICT);

  data.room = ch->in_room;
  data.healed = 0;

  add_event(event_bandage_check, PULSE_VIOLENCE, ch, victim, 0, 0, &data, sizeof(struct bandage_data));

  bzero(&af, sizeof(af));
  af.duration = 3;
  af.type = SKILL_BANDAGE;
  af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
  affect_to_char(victim, &af);

  CharWait(ch, 2 * PULSE_VIOLENCE);
  return;
}
