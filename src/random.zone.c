/****************************************************************************
 *
 *  File: random.zone.c                                      Part of Duris
 *  Usage: randomboject.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark 			Date: 2002-11-12
 * ***************************************************************************
 */

#define TROPHY

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "random.zone.h"
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
#include "vnum.obj.h"
#include "epic.h"
#include "map.h"
#include "utility.h"

/*
 * external variables
 */
extern Skill skills[];
extern struct zone_data *zone_table;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern char debug_mode;
extern const char *class_names[];
extern const char *race_types[];

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
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern char *random_zone;
extern struct random_spells spells_data[];
extern const int rev_dir[];

int      find_map_place();
void     set_long_description(P_obj t_obj, const char *newDescription);
void     set_keywords(P_obj t_obj, const char *newKeys);
void     set_short_description(P_obj t_obj, const char *newShort);
void     writeCorpseNumbs(int start, int end, int map);
int      find_a_zone();
int      connect_lab(int room, int dir);
int      connect_other(int room);
int      dir_to_num(int dir);

int      loaded_potion = 0;

const int theme_rooms[6][10] = {
  {19500, 19501, 19502, 19503, 19504, 19505, 0, 0, 0, 0},       //grave theme
  {19506, 19507, 19508, 19509, 19510, 19511, 19512, 19513, 19514, 19515},       //glass theme
  {19516, 19517, 19518, 19519, 19520, 19521, 19522, 0, 0, 0},   //glass theme
  {19523, 19524, 0, 0, 0, 0, 0, 0, 0, 0},       //tunnel
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

const int theme_obj[6][6] = {
  {19500, 19501, 19502, 19503, 19504, 19505},   //grave theme
  {19500, 19501, 19502, 19503, 19504, 19505},   //glass theme
  {19500, 19501, 19502, 19503, 19504, 19505},   //glass theme
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};
const int theme_entrance_obj[10] =
  { 19507, 19511, 19515, 0, 0, 0, 0, 0, 0, 0 };

//int      generic_room_start = 100000;
//int      generic_room_stop = 105000;
int      random_room_current = RANDOM_START_ROOM;
//int      MAX_GENERIC_ROOM = 50;
int      LOADED_RANDOM_ZONES = 0;

struct random_zone random_zone_data[MAX_RANDOM_ZONES+1];
struct random_quest random_quest_data[80];

int random_entrance_vnum(int rroom)
{
  if( !IS_RANDOM_ROOM(rroom) )
    return 0;
  
  if( LOADED_RANDOM_ZONES == 0 )
    return 0;
  
  for( int i = 0; i <= LOADED_RANDOM_ZONES; i++ )
  {
    if( world[rroom].number >= random_zone_data[i].first_room &&
        world[rroom].number <= random_zone_data[i].last_room )
      return random_zone_data[i].map_room;
  }
  
  return 0;  
}

void add_quest_data(char *map)
{
  if (LOADED_RANDOM_ZONES == 0)
    memset(random_quest_data, 0, sizeof(random_quest_data));

//display_map() maybe=

  random_quest_data[LOADED_RANDOM_ZONES].z_mapdesc = str_dup(map);
}

void create_zone(int theme, int map_room1, int map_room2, int level_range,
                 int rooms)
{
  int      room_nr = 0, room2_nr = 0;
  int      temp_room = 0;
  int      i = 0;
  int      zz = 0;
  int      map_room = 0;
  int      direction = 0;
  int      valid_to_room = 0;
  int      parent_room = 0;
  int      number_mobs = 0;
  int      chest_mob = 0;
  int      tries = 0;
  P_obj    chest_obj;
  P_obj    key_obj;
  P_char   mob;
  P_obj    tomb;
  P_obj    random_zone_obj;
  P_obj    sign;
  char     buf1[MAX_STRING_LENGTH];
  bool    has_epic = false;



  //Let's try to find a map room
  if (map_room1 == 999)
  {
    if (!(map_room = find_map_place()))
      return;
  }
  else
  {
    //map_room = real_room(map_room1);
    if (!(map_room = find_map_place()))
      return;
    map_room1 = map_room;

    if (!(map_room2 = find_map_place()))
      return;
  }

  //Init the data struct just in case!

  if (random_zone_data[0].map_room == 0)
  {
    memset(random_zone_data, 0, sizeof(random_zone_data));
    //wizlog(56," reset the random_zone_data");
  }

  random_zone_data[LOADED_RANDOM_ZONES].map_room = map_room;
  random_zone_data[LOADED_RANDOM_ZONES].theme = theme;


  //Create max MAX_GENERIC_ROOMS rooms, but somewhat random numbers. 
  if (random_room_current + MAX_RANDOM_ROOMS > RANDOM_END_ROOM)
  {
    wizlog(56, "current_room higher then stop_room, not enough room");
    return;
  }

  if (rooms == 999)
    rooms = number(5, 50);

  while (rooms - i)
  {
    i++;
    // make the numbers of rooms a bit random

    room_nr = real_room(random_room_current);

    //Find next free room

    //Assign temp room to a random room from the theme!
    temp_room = real_room(theme_rooms[theme][number(0, 10)]);
    while (temp_room == 0)
      if (map_room1 == 999)
        temp_room = real_room(theme_rooms[theme][number(0, 10)]);
      else if (i == 1 || i == rooms)
        temp_room = real_room(theme_rooms[theme][0]);
      else
        temp_room = real_room(theme_rooms[theme][1]);

    world[room_nr] = world[temp_room];
    world[room_nr].number = random_room_current;

    if (!random_zone_data[LOADED_RANDOM_ZONES].first_room)
      random_zone_data[LOADED_RANDOM_ZONES].first_room = random_room_current;

    random_zone_data[LOADED_RANDOM_ZONES].last_room = random_room_current;
    room_nr = real_room(random_room_current);

    //Make connect the first room to map!
    if (map_room)
    {
      connect_rooms(world[map_room].number, world[room_nr].number, DOWN);
      random_zone_data[LOADED_RANDOM_ZONES].map_room = world[map_room].number;
      //First room is !mob.

      SET_BIT(world[room_nr].room_flags, NO_MOB);

      if (map_room1 == 999)
      {
        if (world[map_room].dir_option[DOWN])
        {
          world[map_room].dir_option[DOWN]->exit_info |= EX_SECRET;

          SET_BIT(world[map_room].dir_option[DOWN]->exit_info, EX_ISDOOR);
          world[map_room].dir_option[DOWN]->keyword = str_dup("trapdoor");
          SET_BIT(world[map_room].dir_option[DOWN]->exit_info, EX_CLOSED);
          SET_BIT(world[room_nr].dir_option[rev_dir[DOWN]]->exit_info,
                  EX_ISDOOR);
          world[room_nr].dir_option[rev_dir[DOWN]]->keyword =
            str_dup("trapdoor");
          SET_BIT(world[room_nr].dir_option[rev_dir[DOWN]]->exit_info,
                  EX_CLOSED);

        }
        tomb = read_object(real_object(theme_entrance_obj[theme]), REAL);
        if (!tomb)
        {
          logit(LOG_DEBUG, "could not load entrance obj");
          wizlog(56, "could not load entrance obj ");
          return;
        }

        obj_to_room(tomb, map_room);
      }
      else
        world[map_room].sector_type = 34;

      map_room = 0;

    }


    else
    {
      parent_room = real_room(random_room_current - 1);

      //wizlog(56, "parent room:%d, current_room:%d", parent_room, room_nr);
      //This remove the "tunnel" aspect of the zone
      if (i > 3)
      {
        if (!number(0, 5))
          parent_room = parent_room - number(1, i - 2);
      }

      tries = 0;
      while (!valid_to_room && tries++ < 20)
      {
        direction = number(0, 5);
        if (!world[parent_room].dir_option[(int) direction])
          valid_to_room = 1;

        if (map_room2 != 999 && (rooms - i < 2))
          if (direction == UP || direction == DOWN)
            valid_to_room = 0;


      }
      
      if( !valid_to_room )
      {
        continue;
      }
      
      connect_rooms(world[parent_room].number, world[room_nr].number,
                    direction);

      valid_to_room = 0;

      if (!number(0, 9) && i > 1)
      {
        world[parent_room].dir_option[direction]->exit_info |= EX_SECRET;
        SET_BIT(world[parent_room].dir_option[direction]->exit_info,
                EX_ISDOOR);
        world[parent_room].dir_option[direction]->keyword = str_dup("secret");
        SET_BIT(world[parent_room].dir_option[direction]->exit_info,
                EX_CLOSED);

        SET_BIT(world[room_nr].dir_option[rev_dir[direction]]->exit_info,
                EX_ISDOOR);
        world[room_nr].dir_option[rev_dir[direction]]->keyword =
          str_dup("secret");
        SET_BIT(world[room_nr].dir_option[rev_dir[direction]]->exit_info,
                EX_CLOSED);

        world[parent_room].dir_option[direction]->exit_info |= EX_SECRET;

        /*while (zz < 5){
           if (world[room_nr].dir_option[zz])
           world[room_nr].dir_option[zz]->exit_info |= EX_SECRET;
           zz++;
           } */
      }
      else if (!number(0, 9) && i > 1)
      {                         //Adding some doors to zone!
        SET_BIT(world[parent_room].dir_option[direction]->exit_info,
                EX_ISDOOR);
        if (direction == 4 || direction == 5)
          world[parent_room].dir_option[direction]->keyword =
            str_dup("trapdoor");
        else
          world[parent_room].dir_option[direction]->keyword = str_dup("door");

        SET_BIT(world[parent_room].dir_option[direction]->exit_info,
                EX_CLOSED);

        SET_BIT(world[room_nr].dir_option[rev_dir[direction]]->exit_info,
                EX_ISDOOR);

        if (direction == 4 || direction == 5)
          world[room_nr].dir_option[rev_dir[direction]]->keyword =
            str_dup("trapdoor");
        else
          world[room_nr].dir_option[rev_dir[direction]]->keyword =
            str_dup("door");
        SET_BIT(world[room_nr].dir_option[rev_dir[direction]]->exit_info,
                EX_CLOSED);
      }

      zz = 0;
      //and now fill it with mobs!
      if (number(0, 2) && i > 2)
      {
        if (level_range == 999)
        {
          mob =
            (P_char) create_random_mob(theme, (int) (i * 2 + number(1, 7)));
        }
        else
        {
          mob =
            (P_char) create_random_mob(theme,
                                       (int) (level_range + number(1, 2)));
        }
        if (mob)
        {
          char_from_room(mob);
          char_to_room(mob, parent_room, 0);
          GET_HOME(mob) = GET_BIRTHPLACE(mob) = GET_ORIG_BIRTHPLACE(mob) =
            world[parent_room].number;
          number_mobs++;
        }

      }

    }
    random_room_current++;

    // wizlog(56, "creating room a random room %d", random_room_current -1);

  }


//Let's add a chest with stuff at the last room, once in a while!
  if (map_room2 != 999)
  {
    connect_rooms(world[map_room2].number,
                  random_zone_data[LOADED_RANDOM_ZONES].last_room, DOWN);
    world[map_room2].sector_type = 34;
  }

  if (!number(0, 1) && map_room2 == 999)
  {
    chest_obj = read_object(real_object(19508), REAL);
    if (!chest_obj)
    {
      logit(LOG_DEBUG, "chest not load able #19508");
      wizlog(56, "Cant load chest#19508");
    }
    else
    {
      obj_to_room(chest_obj,
                  real_room(random_zone_data[LOADED_RANDOM_ZONES].last_room));
      random_zone_data[LOADED_RANDOM_ZONES].chest = 1;

      key_obj = read_object(real_object(19509), REAL);
      if (!key_obj)
      {
        logit(LOG_DEBUG, "key not load able #19509");
        wizlog(56, "Cant key load #19509");
      }
      else
      {
        obj_to_room(key_obj,
                    real_room(random_zone_data[LOADED_RANDOM_ZONES].
                              last_room - number(0, i - 1)));

        key_obj->value[1] = 99;
        chest_obj->value[2] = obj_index[key_obj->R_num].virtual_number;
        //fill chest with stuff!
        random_zone_obj = read_object(real_object(3), REAL);
        random_zone_obj->value[3] = number(1, 5) * i * 5;
        SET_BIT(chest_obj->extra_flags, ITEM_SECRET);
        SET_BIT(key_obj->extra_flags, ITEM_SECRET);
        obj_to_obj(random_zone_obj, chest_obj);

        while (!number(0, 1))
        {
          random_zone_obj =
            read_object(real_object(theme_obj[theme][number(0, 5)]), REAL);
          SET_BIT(random_zone_obj->extra_flags, ITEM_SECRET);
          obj_to_obj(random_zone_obj, chest_obj);
        }

        /* SPECIAL CODE TO ADD LVL POTIONS - disabled AGAIN */
        if (false && (i > 28) && (loaded_potion < 8))
        {
          wizlog(56, "Loaded a potion in chest");
          switch (number(0, 10))
          {
          case 0:
          case 1:
          case 2:
          case 3:
          case 4:
            random_zone_obj = read_object(51004, VIRTUAL);
            random_zone_data[LOADED_RANDOM_ZONES].lvl_potion = 51;
            break;
          case 5:
          case 6:
            random_zone_obj = read_object(25106, VIRTUAL);
            random_zone_data[LOADED_RANDOM_ZONES].lvl_potion = 52;
            break;
          case 7:
            random_zone_obj = read_object(80831, VIRTUAL);
            random_zone_data[LOADED_RANDOM_ZONES].lvl_potion = 53;
            break;
          case 8:
            random_zone_obj = read_object(26661, VIRTUAL);
            random_zone_data[LOADED_RANDOM_ZONES].lvl_potion = 54;
            break;
          case 9:
            random_zone_obj = read_object(25759, VIRTUAL);
            random_zone_data[LOADED_RANDOM_ZONES].lvl_potion = 55;
            break;
          case 10:
            random_zone_obj = read_object(45534, VIRTUAL);
            random_zone_data[LOADED_RANDOM_ZONES].lvl_potion = 56;
            break;

          }

          SET_BIT(random_zone_obj->extra_flags, ITEM_SECRET);
          obj_to_obj(random_zone_obj, chest_obj);
          loaded_potion++;
        }


        while (chest_mob < 1)
        {
          mob =
            (P_char) create_random_mob(theme, (int) (i * 2 + number(1, 7)));
          if (mob)
          {
            //wizlog(56, "extra mob named: %s", GET_NAME(mob));
            char_from_room(mob);
            char_to_room(mob,
                         real_room(random_zone_data[LOADED_RANDOM_ZONES].
                                   last_room), 0);
            SET_BIT(mob->specials.act, ACT_SENTINEL);
            REMOVE_BIT(mob->specials.act, ACT_HUNTER);

            if (chest_obj)
            {
              obj_from_room(chest_obj);
              obj_to_char(chest_obj, mob);
              chest_obj = 0;
            }

            if( !has_epic && i > 19 )
            {
              // add epic stone to last mob
              P_obj epic_stone_obj = read_object(EPIC_SMALL_STONE, VIRTUAL);
              // should payout at least 1 epic, max of 5
              epic_stone_obj->value[0] = BOUNDED(1, (int) ( (i-25) / 2 ), 3) + number(0,1);
              // intended group size from 3-6 based on size of zone
              epic_stone_obj->value[1] = BOUNDED(3, (int) ( (i-25) / 2 ), 6);
              epic_stone_obj->value[2] = zone_table[world[real_room(random_zone_data[LOADED_RANDOM_ZONES].last_room)].zone].number;
              obj_to_char(epic_stone_obj, mob);
              has_epic = true;
            }

            GET_BIRTHPLACE(mob) = GET_ORIG_BIRTHPLACE(mob) =
              random_zone_data[LOADED_RANDOM_ZONES].last_room;
            number_mobs++;
          }
          if (number(0, 1))
            chest_mob++;
        }
      }
    }
  }

  if (map_room2 == 999)
  {

//Ok let's give the people a clue how hard this will be!
    sign = read_object(real_object(19510), REAL);
    if (!sign)
    {
      logit(LOG_DEBUG, "sign not load able #19510");
      wizlog(56, "Cant sign load #19510");
    }
    else
    {
      if (i > 1 && i < 10)
        sprintf(buf1,
                "&+WA huge sign with the engraving '&+RWarning, &+Lthis zone should be a challenge for lowbie groups&+W'&n ",
                i * 2);
      else if (i > 9 && i < 20)
        sprintf(buf1,
                "&+WA huge sign with the engraving '&+RWarning, &+Lthis zone should be a challenge for mid leveled groups&+W'&n ",
                i * 2);
      else if (i > 19 && i < 30)
        sprintf(buf1,
                "&+WA huge sign with the engraving '&+RWarning, &+Lthis zone should be a challenge for &+rsmall&+L high leveled groups towards the end&+W'&n ",
                i * 2);
      else
        sprintf(buf1,
                "&+WA huge sign with the engraving '&+RWarning, &+Lthis zone should be a challenge for &+RBIG&+L high leveled groups towards the end&+W'&n ",
                i * 2);

      set_long_description(sign, buf1);
      obj_to_room(sign,
                  real_room(random_zone_data[LOADED_RANDOM_ZONES].
                            first_room));

    }
//Yay another zone loaded!

//Quest sigil!
    chest_obj = create_sigil(LOADED_RANDOM_ZONES);

    obj_to_room(chest_obj,
                real_room(random_zone_data[LOADED_RANDOM_ZONES].last_room));
  }

  wizlog(56, "Random zone created at %d with %d rooms and %d mobs",
         random_zone_data[LOADED_RANDOM_ZONES].map_room, i, number_mobs);

  LOADED_RANDOM_ZONES++;
}

//a neat function to find a particular map room!
int find_random_zone_map_room(int room_number)
{
  int      j = 0;
  int      i = 0;

  while (i < LOADED_RANDOM_ZONES)
  {
    j = random_zone_data[i].first_room;
    while (j <= random_zone_data[i].last_room)
    {
      if (room_number == j)
        return random_zone_data[i].map_room;
      j++;
    }
    i++;
  }
  return 0;
}

void display_random_zones(P_char ch)
{

  int      i = 0;
  char     buf[20000];
  char     temp_buf[20000];
  char     color_buf[20];
  int      start_room = 0;
  int      end_room = 0;
  int      j = 0;
  int      mob = 0;
  int      player = 0;
  int      unknown = 0;
  P_char   temp_ch, next;

  sprintf(buf,
          "\t\t\t\t&+L-=&+WInitialized &+Lrandom&+W zones&+L=-&n\r\n\r\n");

  while (i < LOADED_RANDOM_ZONES)
  {
    mob = 0;
    player = 0;
    if (random_zone_data[i].first_room && random_zone_data[i].last_room &&
        random_zone_data[i].map_room)
    {
      j = random_zone_data[i].first_room;
      while (j <= random_zone_data[i].last_room)
      {
        for (temp_ch = world[real_room(j)].people; temp_ch; temp_ch = next)
        {
          if (IS_PC(temp_ch))
            player++;
          else if (IS_NPC(temp_ch))
            mob++;

          //wizlog(56 , "MObile:%s in room %d", GET_NAME(temp_ch), j);
          next = temp_ch->next_in_room;

        }
        j++;
      }


      if (player)
        sprintf(color_buf, "%s", "&+Y");
      else if (mob)
        sprintf(color_buf, "%s", "&+L");
      else if (!mob)
        sprintf(color_buf, "%s", "&+y");


      if (god_check(GET_NAME(ch)))
      {
        sprintf(temp_buf,
                "&+W%2d %sTheme:&+W%2d %sRoom range:(&+W%d%s->&+W%d%s) Total:&+W%2d%s Map room:&+W%7d %sPlayers:%s%2d %sMobs:&+W%2d %sChest:&+W%d %sPotion:&+W%d\r\n",
                i + 1, color_buf, random_zone_data[i].theme, color_buf,
                random_zone_data[i].first_room, color_buf,
                random_zone_data[i].last_room, color_buf,
                random_zone_data[i].last_room -
                random_zone_data[i].first_room, color_buf,
                random_zone_data[i].map_room, color_buf,
                player > 0 ? "&+G" : "&+W", player, color_buf, mob, color_buf,
                random_zone_data[i].chest, color_buf,
                random_zone_data[i].lvl_potion);
      }
      else
      {
        sprintf(temp_buf,
                "&+W%2d %sTheme:&+W%2d %sRoom range:(&+W%d%s->&+W%d%s) Total:&+W%2d%s Map room:&+W%7d %sPlayers:%s%2d %sMobs:&+W%2d %sChest:&+W%d %sPotion:&+W??\r\n",
                i + 1, color_buf, random_zone_data[i].theme, color_buf,
                random_zone_data[i].first_room, color_buf,
                random_zone_data[i].last_room, color_buf,
                random_zone_data[i].last_room -
                random_zone_data[i].first_room, color_buf,
                random_zone_data[i].map_room, color_buf,
                player > 0 ? "&+G" : "&+W", player, color_buf, mob, color_buf,
                random_zone_data[i].chest, color_buf);
      }
      strcat(buf, temp_buf);
    }


    i++;
  }

  send_to_char(buf, ch);
  send_to_char
    ("\r\n   &+yNo mobs left in zone(done)&n, &+LMobs left in zone(not done)&n, &+YCharacter(s) in zone(beeing done)&n\r\n",
     ch);


}

int find_map_place()
{
  int start, end;
  int      to_room;
  int      tries = 0;

  start = real_room(SURFACE_MAP_START);
  end = real_room(SURFACE_MAP_END);
  
  do
  {
    to_room = number(start, end);
  }
  while ((IS_SET(world[to_room].room_flags, PRIVATE) || 
          IS_SET(world[to_room].room_flags, PRIV_ZONE) || 
          IS_SET(world[to_room].room_flags, NO_TELEPORT) || 
          world[to_room].dir_option[DOWN] || IS_WATER_ROOM(to_room) || 
          world[to_room].sector_type == SECT_MOUNTAIN ||
          world[to_room].sector_type == SECT_UNDRWLD_MOUNTAIN ||
          world[to_room].sector_type == SECT_OCEAN) && tries++ < 1000);

  if (tries >= 1000)
  {
    return 0;
  }
  return to_room;
}


struct relic_struct relic_struct_data[3];

int      GOODIE_RELIC_POINTS = 0;
int      EVIL_RELIC_POINTS = 0;
int      UNDEAD_RELIC_POINTS = 0;



int read_relic_highscore()
{

  FILE    *f;

  f = fopen("lib/relic_highscore", "r+");

  if (!f)
    return 0;

  fscanf(f, "%d %d %d", &GOODIE_RELIC_POINTS, &EVIL_RELIC_POINTS,
         &UNDEAD_RELIC_POINTS);
  fclose(f);

  wizlog(56, "%d %d %d", GOODIE_RELIC_POINTS, EVIL_RELIC_POINTS,
         UNDEAD_RELIC_POINTS);
}


int write_relic_highscore()
{

  FILE    *f;

  f = fopen("lib/relic_highscore", "w+");

  if (!f)
    return 0;

  fprintf(f, "%d %d %d", GOODIE_RELIC_POINTS, EVIL_RELIC_POINTS,
          UNDEAD_RELIC_POINTS);
  fclose(f);

  wizlog(56, "%d %d %d", GOODIE_RELIC_POINTS, EVIL_RELIC_POINTS,
         UNDEAD_RELIC_POINTS);
}


int update_relic(P_char ch, P_obj obj)
{
  if (IS_TRUSTED(ch) || IS_NPC(ch))
    return 0;



  if (IS_GOOD(ch))
  {

    if (obj_index[obj->R_num].virtual_number == 59)
    {
      if (relic_struct_data[0].undead_relic == 0)
      {
        relic_struct_data[0].undead_relic = 1;
        GOODIE_RELIC_POINTS++;
      }
    }
    if (obj_index[obj->R_num].virtual_number == 68)
    {
      if (relic_struct_data[0].evil_relic == 0)
      {
        relic_struct_data[0].evil_relic = 1;
        GOODIE_RELIC_POINTS++;
      }
    }


  }

  if (IS_EVIL(ch) && !IS_PUNDEAD(ch))
  {

    if (obj_index[obj->R_num].virtual_number == 58)
    {
      if (relic_struct_data[1].good_relic == 0)
      {
        relic_struct_data[1].good_relic = 1;
        EVIL_RELIC_POINTS++;
      }

    }
    if (obj_index[obj->R_num].virtual_number == 59)
    {
      if (relic_struct_data[1].undead_relic == 0)
      {
        relic_struct_data[1].undead_relic = 1;
        EVIL_RELIC_POINTS++;
      }
    }


  }

  if (IS_PUNDEAD(ch))
  {

    if (obj_index[obj->R_num].virtual_number == 58)
    {
      if (relic_struct_data[2].good_relic == 0)
      {
        relic_struct_data[2].good_relic = 1;
        UNDEAD_RELIC_POINTS++;
      }

    }


    if (obj_index[obj->R_num].virtual_number == 68)
    {
      if (relic_struct_data[2].evil_relic == 0)
      {
        relic_struct_data[2].evil_relic = 1;
        UNDEAD_RELIC_POINTS++;
      }
    }
  }

  write_relic_highscore();

  if (obj_index[obj->R_num].virtual_number == 58)
  {
    reset_lab(0);
    return 0;
  }

  if (obj_index[obj->R_num].virtual_number == 68)
  {
    reset_lab(1);
    return 0;
  }

  if (obj_index[obj->R_num].virtual_number == 59)
  {
    reset_lab(2);
    return 0;
  }


}

int get_relic_num(P_char ch)
{

  if (GOOD_RACE(ch))
    return relic_struct_data[0].evil_relic +
      relic_struct_data[0].undead_relic;

  if (EVIL_RACE(ch) && !IS_PUNDEAD(ch))
    return relic_struct_data[1].good_relic +
      relic_struct_data[1].undead_relic;

  if (IS_PUNDEAD(ch))
    return relic_struct_data[2].evil_relic + relic_struct_data[2].good_relic;

  if (IS_NHARPY(ch))
    return 0;

  /* if none of the above, return _something_ meaningful.. */
  return 0;
}

void displayRelic(P_char ch, char *arg, int cmd)
{
  char     buf[512];


  send_to_char("\t  &+L-= Total &+WRELIC&+L points =-&n\r\n\r\n", ch);

  sprintf(buf, "\t         &+WGoodies:&+W %d&n\r\n", GOODIE_RELIC_POINTS);
  send_to_char(buf, ch);
  sprintf(buf, "\t         &+rEvils:&+W   %d&n\r\n", EVIL_RELIC_POINTS);
  send_to_char(buf, ch);
  sprintf(buf, "\t         &+LUndeads:&+W %d&n\r\n\r\n", UNDEAD_RELIC_POINTS);
  send_to_char(buf, ch);

  send_to_char("\t  &+L-= This boot &+WRELIC&+L points =-&n\r\n\r\n", ch);

  sprintf(buf, "\t         &+WGoodies:&+W %d&n\r\n",
          relic_struct_data[0].evil_relic +
          relic_struct_data[0].undead_relic);
  send_to_char(buf, ch);
  sprintf(buf, "\t         &+rEvils:&+W   %d&n\r\n",
          relic_struct_data[1].good_relic +
          relic_struct_data[1].undead_relic);
  send_to_char(buf, ch);
  sprintf(buf, "\t         &+LUndeads:&+W %d&n\r\n",
          relic_struct_data[2].evil_relic + relic_struct_data[2].good_relic);
  send_to_char(buf, ch);


}

int relic_proc(P_obj obj, P_char ch, int cmd, char *arg)
{

  P_char   kala;
  int      dicea = 0;
  int      diceb = 0;
  int      proc = 0;
  int      somone_fighting = 0;
  int      j = 0;
  char     Gbuf2[256];
  struct spell_target_data target_data;
  P_char   t_char = NULL;
  P_desc   d;
  P_obj    potion;

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( cmd == CMD_MELEE_HIT || cmd == CMD_GOTHIT || cmd == CMD_GOTNUKED || cmd == CMD_PERIODIC )
  {
    ;
  }
  else if( (cmd < 1000) && (cmd > 0) )
  {
    if( !OBJ_CARRIED_BY(obj, ch) || IS_TRUSTED(ch) )
    {
      return FALSE;
    }

    // Put time to 30 hours.
    if( obj->value[6] == 0 && IS_PC(ch) )
    {
      obj->value[6] = 1;
      obj->timer[3] = time(NULL) - (90 * 60 * 60);
      update_relic(ch, obj);

      for( d = descriptor_list; d; d = d->next )
      {
        if( d->connected == CON_PLYNG )
        {
          send_to_char("&+LA scream&+L of &+WFREEDOM&+L echos in your ear!&n\r\n", d->character);
        }
      }
      update_relic(ch, obj);

      if( number(0, 1) )
      {
        wizlog(56, "relic_proc: Loaded a LEVEL potion.");
        switch (number(0, 10))
        {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
          potion = read_object(51004, VIRTUAL);
          break;
        case 5:
        case 6:
          potion = read_object(25106, VIRTUAL);
          break;
        case 7:
          potion = read_object(80831, VIRTUAL);
          break;
        case 8:
          potion = read_object(26661, VIRTUAL);
          break;
        case 9:
          potion = read_object(25759, VIRTUAL);
          break;
        case 10:
          potion = read_object(45534, VIRTUAL);
          break;
        }
        obj_to_char(potion, ch);
        act("Your $q &+Mhums&+W briefly.", FALSE, ch, potion, potion, TO_CHAR);
        act("$n's $q &+Mhums&+W briefly.", TRUE, ch, potion, NULL, TO_ROOM);
      }
    }

    one_argument(arg, Gbuf2);

    if( cmd == CMD_RENT || cmd == CMD_CAMP || cmd == CMD_QUIT )
    {
      act("$p shakes it's head?!?!&N", FALSE, ch, obj, 0, TO_CHAR);
      act("$p shakes it's head?!?!&N", FALSE, ch, obj, 0, TO_ROOM);
      return TRUE;
    }

    if( cmd == CMD_WIELD && *arg )
    {
      if( OBJ_WORN(obj) )
      {
        return FALSE;
      }

      if( isname(Gbuf2, "relic") || isname(Gbuf2, "true") || isname(Gbuf2, "holy")
        || isname(Gbuf2, "divine") || isname(Gbuf2, "undead") || isname(Gbuf2, "kings")
        || isname(Gbuf2, "blood") || isname(Gbuf2, "evil") || isname(Gbuf2, "ancient")
        || isname(Gbuf2, "spider") || isname(Gbuf2, "lloth") )
      {
        act("$p &+rfl&+Rares&+L up...&N", FALSE, ch, obj, 0, TO_CHAR);
        act("$p &+rfl&+Rares&+L up...&N", FALSE, ch, obj, 0, TO_ROOM);

        if( GET_RACE(ch) == RACE_OGRE || GET_RACE(ch) == RACE_MINOTAUR ||
            GET_RACE(ch) == RACE_WIGHT || GET_RACE(ch) == RACE_SGIANT ||
            GET_CLASS(ch, CLASS_PALADIN) || GET_CLASS(ch, CLASS_ANTIPALADIN) )
        {
          SET_BIT(obj->extra_flags, ITEM_TWOHANDS);
          dicea = 10;
          diceb = 3;
        }
        else
        {
          REMOVE_BIT(obj->extra_flags, ITEM_TWOHANDS);
          dicea = 8;
          diceb = 3;
        }

        obj->value[1] = dicea;
        obj->value[2] = diceb;

        if( GET_CLASS(ch, CLASS_THIEF) || GET_CLASS(ch, CLASS_ASSASSIN)
          || GET_CLASS(ch, CLASS_MERCENARY) || GET_CLASS(ch, CLASS_ROGUE) )
        {
          obj->value[0] = WEAPON_DAGGER;
        }
        else
        {
          obj->value[0] = WEAPON_LONGSWORD;
        }
        obj->value[3] = 3;
        obj->type = ITEM_WEAPON;
        obj->affected[1].location = APPLY_DAMROLL;
        obj->affected[1].modifier = 5;
        obj->affected[2].location = APPLY_HITROLL;
        obj->affected[2].modifier = 5;
      }
      return FALSE;
    }

    if (cmd == CMD_WEAR && *Gbuf2)
    {
      if( OBJ_WORN(obj) )
      {
        return FALSE;
      }

      if( isname(Gbuf2, "relic") || isname(Gbuf2, "true") || isname(Gbuf2, "holy")
        || isname(Gbuf2, "divine") || isname(Gbuf2, "undead") || isname(Gbuf2, "kings")
        || isname(Gbuf2, "blood") || isname(Gbuf2, "evil") || isname(Gbuf2, "ancient")
        || isname(Gbuf2, "spider") || isname(Gbuf2, "lloth") )
      {
        act("$p &+rfl&+Rares&+L up...&N", FALSE, ch, obj, 0, TO_CHAR);
        act("$p &+rfl&+Rares&+L up...&N", FALSE, ch, obj, 0, TO_ROOM);

        obj->value[1] = 0;
        obj->value[2] = 0;
        obj->value[0] = 0;
        obj->value[3] = 3;
        obj->type = ITEM_ARMOR;
        obj->affected[1].location = APPLY_WIS_MAX;
        obj->affected[1].modifier = 15;
        obj->affected[2].location = APPLY_INT_MAX;
        obj->affected[2].modifier = 15;
      }
      return FALSE;
    }
    return FALSE;
  }

  if( !OBJ_WORN_BY(obj, ch) || IS_NPC(ch) || GET_LEVEL(ch) < 40 )
  {
    return FALSE;
  }

  // 1/20 chance.
  if( IS_FIGHTING(ch) && number(0, 19) )
  {
    return FALSE;
  }
  kala = ch->specials.fighting;

  // Find an offensive spell.
  while( !(IS_SET(skills[j].targets, TAR_FIGHT_VICT)) || (!IS_AGG_SPELL(j)) )
  {
    j = number(FIRST_SPELL, LAST_SPELL);
  }
  // And hit kala with it.
  if( IS_ALIVE(kala) )
  {
    act("$p&+L carried by $n sends a ray of &+Wlight&+L towards $N!&N", TRUE, ch, obj, kala, TO_NOTVICT);
    act("$p&+L carried by you sends a ray of &+Wlight&+L towards $N!&N", TRUE, ch, obj, kala, TO_CHAR);
    act("$p&+L carried by $n sends a ray of &+Wlight&+L towards you!&N", TRUE, ch, obj, kala, TO_VICT);
    ((*skills[j].spell_pointer) ((int) GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, kala, NULL));
  }
  return FALSE;
}

int random_mob_proc(P_char ch, P_char pl, int cmd, char *arg)
{
  P_char   kala;
  int      proc = 0;
  int      j = 0;
  struct spell_target_data target_data;

  if (cmd == CMD_MELEE_HIT)
  {;
  }
  else if ((cmd < 1000) && (cmd > 0))
  {
    return FALSE;
  }


  if (!IS_SET(ch->specials.act, ACT_SENTINEL))
    return FALSE;

  if (GET_LEVEL(ch) < 40)
    return FALSE;
  if (!IS_FIGHTING(ch) && number(0, 7))
    return FALSE;



  kala = ch->specials.fighting;

  while (!(IS_SET(skills[j].targets, TAR_FIGHT_VICT)) || (!IS_AGG_SPELL(j)))
    j = number(FIRST_SPELL, LAST_SPELL);




  if (kala)
  {
    act("&+LA&n $n &+Lgrowls in fury and unleashes his &+rANGER&+L at&n $N",
        TRUE, ch, 0, kala, TO_NOTVICT);
    act("&+LAll of your &+rRAGE&+L is unleashed at& $N!", TRUE, ch, NULL,
        kala, TO_CHAR);
    act
      ("&+LA&n $n &+Lsuddenly becomes &+rENRAGED&+L and unleashes his fury at YOU!",
       TRUE, ch, 0, kala, TO_VICT);
    ((*skills[j].spell_pointer) ((int) GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL,
                                 kala, NULL));
  }


  return 0;

}

int random_quest_mob_proc(P_char ch, P_char pl, int cmd, char *arg)
{
  P_obj    obj;
  int      x = 0;
  int      y = 0;
  int      valuediff = 0;
  char     Gbuf2[MAX_STRING_LENGTH];
  char     obj_name[MAX_INPUT_LENGTH], *argument;
  int      group_fact = 1, fragdiff = 0;
  int      value_pts = 0;
  char     buffer[1024];
  struct group_list *gl;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;
  if (!ch || !cmd || !arg || !pl)
    return FALSE;

  argument = arg;
  arg = one_argument(arg, Gbuf2);

  x = GET_HOME(ch) % 10;
  y = (GET_HOME(ch) / 10) % 10;
  x = (y * 10) + x;

  if (cmd == CMD_GIVE)
  {
    argument = one_argument(argument, obj_name);

    obj = get_obj_in_list_vis(pl, obj_name, pl->carrying);

    if (!obj)
    {
      send_to_char("You do not seem to have anything like that.\r\n", pl);
      return TRUE;
    }

    if( OBJ_VNUM(obj) == VOBJ_RANDOM_ARMOR && obj->value[5] == x )
    {
      gain_epic(pl, EPIC_RANDOM_ZONE, 0, value_pts / 100 );

      act("$n gives $p to $N.", 1, pl, obj, ch, TO_NOTVICT);
      act("$n gives you $p.", 0, pl, obj, ch, TO_VICT);
      send_to_char("Ok.\r\n", pl);

      obj_from_char(obj);
      extract_obj(obj);

      mobsay(ch, "Yes! Perfect! Right one !");
      do_action(ch, 0, CMD_CACKLE);
      mobsay(ch, "Take this...!");

      //PEriod value award!
      value_pts =
        2500 * (random_zone_data[x].last_room -
                random_zone_data[x].first_room);

      if (pl->group)
      {

        for (gl = pl->group; gl; gl = gl->next)
          if ((pl->in_room == gl->ch->in_room) && (gl->ch != pl))
            group_fact++;

        value_pts = (int) value_pts / group_fact;

        value_pts = BOUNDED(1, value_pts, 10000 + number(100, 1500));

        for (gl = pl->group; gl; gl = gl->next)
          if ((pl->in_room == gl->ch->in_room) && (gl->ch != pl) &&
              IS_PC(gl->ch))
          {
            gain_epic(gl->ch, EPIC_RANDOM_ZONE, 0, value_pts / 100  );
          }
      }

      value_pts = BOUNDED(1, value_pts, 10000 + number(100, 1500));

      gain_epic(pl, EPIC_RANDOM_ZONE, 0, value_pts / 100 );

      obj = create_stones(ch);
      obj_to_char(obj, pl);

      act("$n gives $q to $N!", TRUE, ch, obj, pl, TO_NOTVICT);
      act("$n gives you $q ", TRUE, ch, obj, pl, TO_VICT);

      while (1)
      {

        obj = create_material(ch, ch);
        obj_to_char(obj, pl);
        act("$n gives $q to $N!", TRUE, ch, obj, pl, TO_NOTVICT);
        act("$n gives you $q ", TRUE, ch, obj, pl, TO_VICT);


        obj = create_random_eq_new(ch, ch, -1, -1);
        obj_to_char(obj, pl);
        act("$n gives $q to $N!", TRUE, ch, obj, pl, TO_NOTVICT);
        act("$n gives you $q ", TRUE, ch, obj, pl, TO_VICT);


        if (number(0, 2))
          return 1;
      }


    }

    return FALSE;               // let normal give handle it
  }



  if (arg && cmd == CMD_ASK)
  {

    if (!CAN_SPEAK(ch))
    {
      return 0;
    }

    if (!CAN_SEE(ch, pl))
    {
      mobsay(ch, "How may I be of help if I cannot see you?");
      return TRUE;
    }


    //Aggros? well they just agro!
    if (IS_SET(ch->only.npc->aggro_flags, AGGR_ALL))
    {
      mobsay(ch,
             "I'll help ya with directions to hell, if I could see you!!");
      return 1;
    }


    if (LOADED_RANDOM_ZONES <= x)
    {

      mobsay(ch,
             "I heard a traveling friends of mine had a quest, but I can't help you any futher.&n");
      return 1;
    }

    if (isname(arg, "quest"))
    {
      mobsay(ch,
             "Hmm, you are looking for a quest? My friends recently got killed, perhaps you can help me recover our sigil!?!&n");
      return 1;
    }

    if (isname(arg, "friends"))
    {
      mobsay(ch,
             "Hmm, you are looking for a quest? I have this map, but are you strong enough!?!&n");
      return 1;
    }

    if (isname(arg, "sigil"))
    {
      mobsay(ch,
             "Bring me the sigil carried by my friend, and rewarded you shall be!&n");
      return 1;
    }

    if (!isname(arg, "map"))
      return FALSE;


    mobsay(ch,
           "I was out exploring the wilderness with my fellow adventurers when we came across a pathway leading into the ground.");
    mobsay(ch,
           "We lit our torches and decended into the depths, only to be attacked by a horde of hostile creatures!");
    mobsay(ch,
           "They slaughtered us without mercy and only I managed to escape");
    mobsay(ch, "I beg you, avenge my friends");
    mobsay(ch,
           "I managed to draw a map of this hidden tomb, but alas, I cannot do more than just show it to thee.");

    send_to_room("&+yAn old piece of paper with a detailed map.\r\n",
                 ch->in_room);
    send_to_room(random_quest_data[x].z_mapdesc, ch->in_room);
    mobsay(ch, "Avenge my comrades! &+rKILL THEM ALL!&n.");
    mobsay(ch,
           "Also, if you happen to find a sigil in there please bring it to me.");
    mobsay(ch,
           "It is something I treasure and I will reward you apon its return.");
    return 1;
  }

  return 0;
}


P_obj create_sigil(int zone_number)
{

  P_obj    obj;
  char     buf1[MAX_STRING_LENGTH];
  char     buf2[MAX_STRING_LENGTH];
  char     buf3[MAX_STRING_LENGTH];
  struct zone_data *zone = 0;

  obj = read_object( VOBJ_RANDOM_ARMOR, VIRTUAL );
  zone = &zone_table[find_a_zone()];

  sprintf(buf1, "sigil %s", zone->name);
  sprintf(buf2, "&+ra sigil from %s&n", zone->name);
  sprintf(buf3, "&+ra sigil from %s&n lies here.", zone->name);

  set_keywords(obj, buf1);
  set_short_description(obj, buf2);
  set_long_description(obj, buf3);

  SET_BIT(obj->wear_flags, ITEM_TAKE);
  obj->value[5] = zone_number;
  SET_BIT(obj->extra_flags, ITEM_NORENT);

  return obj;

}


//Code to generate labiryths.


int reset_lab(int type)
{
  int      start_room, entrance_room;
  int      i = 0;
  int      sector_type = 0;
  P_char   vict, next_v;
  P_obj    obj, next_o;


  if (type == 0)
  {
    start_room = 700000;
    entrance_room = 154938;
    sector_type = 3;
  }
  if (type == 1)
  {
    start_room = 900000;
    sector_type = 3;
    entrance_room = 145443;
  }
  if (type == 2)
  {
    start_room = 800000;
    entrance_room = 211827;
    sector_type = 3;

  }


  while (i < 10000)
  {

    world[real_room(start_room + i)].sector_type = sector_type;
    i++;

    for (vict = world[real_room(start_room + i)].people; vict; vict = next_v)
    {
      next_v = vict->next_in_room;

      if (IS_NPC(vict) && !IS_MORPH(vict))
      {
        extract_char(vict);
        vict = NULL;
      }
      if (vict && IS_PC(vict))
      {
        char_from_room(vict);
        char_to_room(vict, real_room(entrance_room), -1);
      }


    }


    for (obj = world[real_room(start_room + i)].contents; obj; obj = next_o)
    {
      next_o = obj->next_content;

      if (obj->R_num == real_object(VOBJ_WALLS)) 
        continue;

      if ((obj->wear_flags & ITEM_TAKE && !IS_ARTIFACT(obj))
        || (obj->type == ITEM_CORPSE && !obj->contains))
      {
        extract_obj(obj);
        obj = NULL;
      }
      if (obj && (obj->type == ITEM_CORPSE || IS_ARTIFACT(obj)))
      {
        obj_from_room(obj);
        obj_to_room(obj, real_room(entrance_room));

      }

    }



  }

//Remove entrance!
  world[real_room(entrance_room)].dir_option[DOWN] = 0;

}
int create_lab(int type)
{
  int      start_room = 0;
  int      current_room = start_room;
  int      parent_room = 0;
  int      direction = 0;
  int      rooms = 800;
  int      parent_dir = -1;
  int      relic_in_game = 0;
  P_char   mob;
  P_obj    obj;
  int      i = 0;
  int      j = 0;
  int      N = -100;
  int      E = 1;
  int      S = 100;
  int      W = -1;
  int      theme_to_use = 0;
  int      RELIC = 0;
  int      ENDMOBS = 5;
  int      map_room = 0;
  int      vnum = 0;

  memset(relic_struct_data, 0, sizeof(relic_struct_data));
  read_relic_highscore();

  wizlog(56, "%d", type);

  if (type == 0)
  {
    start_room = 705050;
    theme_to_use = 4;
    RELIC = 58;
    map_room = 154938;

  }
  if (type == 1)
  {
    start_room = 905050;
    theme_to_use = 5;
    RELIC = 68;
    map_room = 145443;
  }
  if (type == 2)
  {
    start_room = 805050;
    theme_to_use = 6;
    RELIC = 59;
    map_room = 211827;
  }

  obj = read_object(RELIC, VIRTUAL);
  vnum = obj_index[obj->R_num].virtual_number;

  if( get_artifact_data_sql(vnum, NULL) )
  {
    wizlog(56, "Already tracked %d, not creating random map #%d ", vnum, type);
    extract_obj(obj);
    return 0;
  }
  else
  {
    wizlog(56, "Did not track %d, creating random map #%d ", vnum, type);
    extract_obj(obj);
  }

  current_room = start_room;


  //Remove all exit if the room havent been used yet...
  if (world[real_room(start_room)].sector_type != 1)
  {
    world[real_room(start_room)].dir_option[NORTH] = 0;
    world[real_room(start_room)].dir_option[EAST] = 0;
    world[real_room(start_room)].dir_option[SOUTH] = 0;
    world[real_room(start_room)].dir_option[WEST] = 0;
  }


  while (rooms - i > 0)
  {

    if (!number(0, 2) || parent_dir == -1)
    {
      direction = number(0, 3);
      parent_dir = direction;
    }
    else
    {
      direction = parent_dir;
    }


    parent_room = current_room;

    SET_BIT(world[real_room(current_room)].room_flags, NO_TELEPORT);

    current_room = connect_lab(current_room, direction);
    connect_other(current_room);

    //Mob start
    if (!number(0, 4))
    {
      mob =
        (P_char) create_random_mob(theme_to_use, (int) (40 + number(0, 22)));
      if (mob)
      {
        char_from_room(mob);
        char_to_room(mob, real_room(current_room), 0);
        GET_HOME(mob) = GET_BIRTHPLACE(mob) = GET_ORIG_BIRTHPLACE(mob) =
          current_room;
        if (!number(0, 5))
          SET_BIT(mob->specials.act, ACT_SENTINEL);
      }
    }
    //Mob end




    if (rooms - i == 1)
    {
      while (ENDMOBS - j > 0)
      {

        mob = (P_char) create_random_mob(7, (int) (60 + number(0, 2)));
        if (mob)
        {
          char_from_room(mob);
          char_to_room(mob, real_room(current_room), 0);
          GET_HOME(mob) = GET_BIRTHPLACE(mob) = GET_ORIG_BIRTHPLACE(mob) =
            current_room;

          SET_BIT(mob->specials.act, ACT_SENTINEL);
          if (!relic_in_game)
          {
            obj = read_object(RELIC, VIRTUAL);
            vnum = obj_index[obj->R_num].virtual_number;

            if( get_artifact_data_sql(vnum, NULL) )
            {
              wizlog(56, "Already tracked %d", vnum);
              extract_obj(obj);
              relic_in_game = 1;
            }
            else
            {
              obj->timer[3] = time(NULL) - (85 * 60 * 60);
              obj_to_char(obj, mob);
              relic_in_game = 1;
            }
          }


        }

        j++;
      }
    }


    i++;
  }

  connect_rooms(world[real_room(map_room)].number,
                world[real_room(start_room)].number, DOWN);
  world[real_room(map_room)].sector_type = 0;

}



int connect_lab(int room, int dir)
{
  //Remove all exit if the room havent been used yet...
  if (world[real_room(room + dir_to_num(dir))].sector_type != 1)
  {
    world[real_room(room + dir_to_num(dir))].dir_option[NORTH] = 0;
    world[real_room(room + dir_to_num(dir))].dir_option[EAST] = 0;
    world[real_room(room + dir_to_num(dir))].dir_option[SOUTH] = 0;
    world[real_room(room + dir_to_num(dir))].dir_option[WEST] = 0;
  }


  if (real_room(room + dir_to_num(dir)) == NOWHERE)
    dir = rev_dir[dir];

  world[real_room(room + dir_to_num(dir))].sector_type = 1;


  connect_rooms(world[real_room(room)].number,
                world[real_room(room + dir_to_num(dir))].number, dir);
  return room + dir_to_num(dir);
}

int connect_other(int room)
{
  int      dir = 0;
  int      i = 0;


  while (dir < 4)
  {

    if (world[real_room(room + dir_to_num(dir))].sector_type == 1 &&
        !(world[real_room(room + dir_to_num(dir))].number == -1))
    {
      connect_rooms(world[real_room(room)].number,
                    world[real_room(room + dir_to_num(dir))].number, dir);
      //wizlog(56, "Connecting others %d to %d -- dir=%d", room , room + dir_to_num(dir)), dir;
    }

    dir++;
  }


}



int dir_to_num(int dir)
{

  int      N = -100;
  int      E = 1;
  int      S = 100;
  int      W = -1;

  if (dir == NORTH)
    return -100;

  if (dir == EAST)
    return 1;

  if (dir == SOUTH)
    return 100;

  if (dir == WEST)
    return -1;

}
