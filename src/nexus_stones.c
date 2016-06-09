/* nexus_stones.c
   - Torgal (torgal@durismud.com)
     Feb 2007

*/

#include <stdlib.h>
#include <cstring>
#include <vector>
using namespace std;

#include "prototypes.h"
#include "defines.h"
#include "utils.h"
#include "utility.h"
#include "structs.h"
#include "nexus_stones.h"
#include "racewar_stat_mods.h"
#include "sql.h"
#include "interp.h"
#include "comm.h"
#include "spells.h"
#include "db.h"
#include "damage.h"
#include "epic.h"
#include "ship_npc.h"
#include "boon.h"

extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const int top_of_world;
extern P_desc descriptor_list;
extern P_char character_list;
extern P_obj object_list;
extern const char *apply_names[];


sh_int *char_stat(P_char ch, int stat);

#ifdef __NO_MYSQL__
int init_nexus_stones()
{
    // load nothing
}

void do_nexus(P_char ch, char *arg, int cmd) 
{
    // do nothing
}
P_obj get_random_enemy_nexus(P_char ch)
{
  return NULL;
}

P_obj get_nexus_stone(int stone_id)
{
  return NULL;
}
int check_nexus_bonus(P_char ch, int amount, int type)
{
  return 0;
}
#else

//#include <mysql.h>


/*
 As Peeblo's hand makes contact with the nexus stone, he seems to cry out in a silent scream.
 His body is incased in blazing eldritch energy, and as it subsides, he seems to have a benevolent smile on his
 face./he grins with wicked delight.
 */


#define _NO_TOUCH 0
#define _CH_INDIFFERENT 1
#define _TOO_EARLY 2
#define _TOUCH_CH 3
#define _TOUCH_ROOM 4
#define _GLOBAL_GOOD_TOUCH 5
#define _GLOBAL_GOOD_TURN 6
#define _GLOBAL_EVIL_TOUCH 7
#define _GLOBAL_EVIL_TURN 8
#define _STONE_HUM 9
#define _GOOD_GUARD_LOAD 10
#define _EVIL_GUARD_LOAD 11
#define _GOOD_GUARD_DIE 12
#define _EVIL_GUARD_DIE 13
#define _TOO_EARLY_TURN 14
#define _CH_LOW_LEVEL 15
#define _STONE_EXPIRED 16
#define _GOOD_SAGE_LOAD 17
#define _EVIL_SAGE_LOAD 18
#define _GOOD_SAGE_DIE 19
#define _EVIL_SAGE_DIE 20
#define _NEUTRAL_GUARD_LOAD 21

char *ns_messages[] = {
  "&+gA strange field of energy prevents you from touching $p&+G.&n",
  "&+WThe stone is indifferent to your touch.",
  "&+WYour touch is repulsed by the stone!",
  "As your hand makes contact with the nexus-stone, you feel the essence \n" \
  "of your very soul being siphoned from your body.",
  "As $n's hand makes contact with the nexus stone, he seems to cry out in a silent scream. \n" \
  "$s body is incased in blazing eldritch energy, and as it subsides, $e crumbles to the floor.",
  "&+WA warm feeling permeates your body as a feeling of peace and warmth flows through you.&n",
  "&+WThe sun seems to shine a bit longer, and a sense of harmony washes through the lands!",
  "&+LThe sun seems to sag in the sky, and the sound of goblin and orcish wardrums fills your ears.&n",
  "&+LYou feel great fear and &+rBloodLust&+L as evil and hate spread through the lands!!",
  "$p starts to &+rhum&n wildly!",
  "The light in the room seems to coallesce into a brilliant mass of energy in the room.",
  "The shadows seem to coallesce into a dark mass of energy in the room.",
  "&+WAs $n&+W dissipates into the surroundings, a gigantic energy wave is released!",
  "&+LAs $n&+L dissipates into the surroundings, a gigantic energy wave is released!",
  "&+WThere is still too much residual energy in the stone! You must wait until it dissipates.",
  "&+CThe stone is indifferent to your touch. Perhaps you do not yet have enough experience?",
  "&+cAs if awakening from a fever, the world suddenly seems clearer and less extreme.",
  "A good wandering sage walks up and begins contemplating the stone.",
  "An evil wandering sage walks up and begins contemplating the stone.",
  "$n&n's earthly incarnation simply ceases to be, sucking energy away from the universe.",
  "$n&n's earthly incarnation simply ceases to be, sucking energy away from the universe.",
  "The light and the darkness in this room seem to combine into some sort of shadow entity."
};

struct NexusBonusData {
  char *name;
  int minval;
} nexus_bonus_data[] = {
  {"\0", 0},      // ID Name
  {"epics", 1},   //  1 Marduk
  {"exp", 1},     //  2 Enki 
  {"cargo", 10},
  {"prestige", 1},
  {"\0", 0}
};

extern MYSQL* DB;

int init_nexus_stones()
{
  fprintf(stderr, "-- Booting nexus stones\r\n");
  
  mob_index[real_mobile(MOB_GOOD_GUARDIAN)].func.mob = nexus_stone_guardian;
  mob_index[real_mobile(MOB_EVIL_GUARDIAN)].func.mob = nexus_stone_guardian;
  mob_index[real_mobile(MOB_GOOD_SAGE)].func.mob = nexus_sage;
  mob_index[real_mobile(MOB_EVIL_SAGE)].func.mob = nexus_sage;  
  obj_index[real_object(OBJ_NEXUS_STONE)].func.obj = nexus_stone;
  obj_index[real_object(OBJ_GUARDIAN_MACE)].func.obj = nexus_guardian_pwn_mace;

  load_nexus_stones();
}

int load_nexus_stones()
{
  // load nexus stones from DB
  if( !qry("SELECT id, name, room_vnum, align FROM nexus_stones") )
    return FALSE;
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return FALSE;
  }
    
  MYSQL_ROW row;
  while( row = mysql_fetch_row(res) )
  {
    int stone_id = atoi(row[0]);
    char* stone_name = row[1];
    int room_vnum = atoi(row[2]);
    int align = atoi(row[3]);
    
    load_nexus_stone(stone_id, stone_name, room_vnum, align);
  }
  
  mysql_free_result(res);

  update_nexus_stat_mods();
  
  return TRUE;
}

bool nexus_stone_info(int stone_id, NexusStoneInfo *info)
{
  if( !info )
    return FALSE;
  
  // load nexus stones from DB
  if( !qry("SELECT name, room_vnum, align, stat_affect, affect_amount, last_touched_at FROM nexus_stones where id = %d", stone_id) )
    return FALSE;
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return FALSE;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);
  
  info->id = stone_id;
  info->name = row[0];
  info->room_vnum = atoi(row[1]);
  info->align = atoi(row[2]);
  info->stat_affect = atoi(row[3]);
  info->last_touched_at = atoi(row[4]);
  
  mysql_free_result(res);
   
  return TRUE;
}

int check_nexus_bonus(P_char ch, int amount, int type)
{
  char buff[MAX_STRING_LENGTH], buff2[MAX_STRING_LENGTH];

  if ( !qry("SELECT align, bonus FROM nexus_stones WHERE align in ('%d', '%d') AND bonus = '%d'", STONE_ALIGN_GOOD, STONE_ALIGN_EVIL, type) )
    return amount;

  if (!ch)
  {
    debug("check_nexus_bonus(): passed invalid ch pointer to function");
    return amount;
  }
 
  MYSQL_RES *res = mysql_store_result(DB);

  if ( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return amount;
  }

  MYSQL_ROW row = mysql_fetch_row(res);
  
  int align = atoi(row[0]);
  int bonus = atoi(row[1]);
  int racewar, newamnt;

  mysql_free_result(res);

  if (align == STONE_ALIGN_GOOD)
    racewar = RACEWAR_GOOD;
  else if (align == STONE_ALIGN_EVIL)
    racewar = RACEWAR_EVIL;
  else
    racewar = 0;

  sprintf(buff, "nexusStones.bonus.%s", nexus_bonus_data[type].name);

  if (GET_RACEWAR(ch) == racewar)
  {
   newamnt = MAX(nexus_bonus_data[type].minval, (int)((double)amount*(double)get_property(buff, 0.100)));
   
   switch (type)
   {
     case NEXUS_BONUS_EPICS:
       break;
     case NEXUS_BONUS_EXP:
       break;
     case NEXUS_BONUS_CARGO:
       sprintf(buff2, "&+yEnlil&n grants you an additional %s&n.\r\n", coin_stringv(newamnt));
       send_to_char(buff2, ch);
       break;
     default:
       break;
   }

   return (int)(amount + newamnt);
  }
  else
  {
    return amount;
  }
}

void update_nexus_stat_mods()
{
  // Not using stat modifiers right now... -Venthix 3/29/09
  return;

  if( !qry("SELECT align, stat_affect, affect_amount, FROM nexus_stones WHERE align in ('%d', '%d')", STONE_ALIGN_GOOD, STONE_ALIGN_EVIL) )
    return;
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return;
  }
  
  reset_racewar_stat_mods();
  
  MYSQL_ROW row;
  while( row = mysql_fetch_row(res) )
  {
    int align = atoi(row[0]);
    int stat_affect = atoi(row[1]);
    int affect_amount = atoi(row[2]);
    
    if( align == STONE_ALIGN_GOOD )
    {
      set_racewar_stat_mod(RACEWAR_GOOD, stat_affect, affect_amount, 0);
    }
    else if( align == STONE_ALIGN_EVIL )
    {
      set_racewar_stat_mod(RACEWAR_EVIL, stat_affect, affect_amount, 0);      
    }
  }
  
  mysql_free_result(res);
}

int update_nexus_stone_align(int stone_id, int align)
{
  if( !qry("UPDATE nexus_stones SET align = '%d', last_touched_at = unix_timestamp() WHERE id = '%d'", align, stone_id) )
    return FALSE;
  
  return TRUE;
}

bool nexus_stone_touch(P_obj stone, P_char ch)
{
  P_char i, next;

  if( !stone || !ch )
    raise(SIGSEGV);

  for (i = world[ch->in_room].people; i; i = next)
  {
    next = i->next_in_room;

    if(IS_PC(i) ||
       IS_PC_PET(i))
    {
      continue;
    }

    if(IS_NPC(i))
    {
       if(GET_VNUM(i) == MOB_NEUTRAL_GUARDIAN ||
          GET_VNUM(i) == MOB_GOOD_GUARDIAN ||
          GET_VNUM(i) == MOB_EVIL_GUARDIAN)
       {
         act(ns_messages[_NO_TOUCH], FALSE, ch, stone, 0, TO_CHAR);
         return false;
       } 
    }
  }  

  P_char guardian = get_nexus_guardian(STONE_ID(stone));
 
  if( guardian )
  {
    // guardian is still alive, prevent touching
    act(ns_messages[_NO_TOUCH], FALSE, ch, stone, 0, TO_CHAR);
    return FALSE;
  }
  
  if( !IS_RACEWAR_GOOD(ch) && !IS_RACEWAR_EVIL(ch) )
  {
    act(ns_messages[_CH_INDIFFERENT], FALSE, ch, stone, 0, TO_CHAR);
    return FALSE;
  }
  
  if( GET_LEVEL(ch) < 51 )
  {
    act(ns_messages[_CH_LOW_LEVEL], FALSE, ch, stone, 0, TO_CHAR); 
    return FALSE;
  }
  
  if( ( IS_RACEWAR_GOOD(ch) && STONE_ALIGN(stone) >= STONE_ALIGN_GOOD ) ||
      ( IS_RACEWAR_EVIL(ch) && STONE_ALIGN(stone) <= STONE_ALIGN_EVIL ) )
  {
    // if the stone is turned, only allow touches from opposite side
    act(ns_messages[_CH_INDIFFERENT], FALSE, ch, stone, 0, TO_CHAR);
    return FALSE;
  }
    
  if( !IS_TRUSTED(ch) && !IS_SET(stone->extra_flags, ITEM_HUM) )
  {
    // not enough time since last touch
    act(ns_messages[_TOO_EARLY], FALSE, ch, stone, 0, TO_CHAR);
    return FALSE;
  }
  
  bool will_turn = FALSE;
  
  if( ( IS_RACEWAR_GOOD(ch) && (STONE_ALIGN(stone) + 1) >= STONE_ALIGN_GOOD ) ||
      ( IS_RACEWAR_EVIL(ch) && (STONE_ALIGN(stone) - 1) <= STONE_ALIGN_EVIL ) )
  {
    will_turn = true;
  }
  
  int time_now = time(NULL);

  if( !IS_TRUSTED(ch) && 
      will_turn &&
      STONE_TURN_TIMER(stone) > ( time_now - get_property("nexusStones.turnTimerSecs", 10) ) )
  {
    // if this touch would turn the stone, but not enough time has passed, then prevent touch
    act(ns_messages[_TOO_EARLY_TURN], FALSE, ch, stone, 0, TO_CHAR);
    return FALSE;
  }

  // after here is a valid touch
  
  STONE_TIMER(stone) = time_now;
    
  statuslog(57, "Nexus stone (%d) touched by %s", STONE_ID(stone), GET_NAME(ch));
  logit(LOG_STATUS, "Nexus stone (%d) touched by %s", STONE_ID(stone), GET_NAME(ch));  
  
  act(ns_messages[_TOUCH_CH], FALSE, ch, stone, 0, TO_CHAR);
  act(ns_messages[_TOUCH_ROOM], FALSE, ch, stone, 0, TO_ROOM);
  
  if( !IS_TRUSTED(ch) )
  {
    SET_POS(ch, POS_PRONE + STAT_INCAP);
    GET_HIT(ch) = -5;
  }
  
  if( !IS_TRUSTED(ch) )
  {
    REMOVE_BIT(stone->extra_flags, ITEM_HUM);
  }
  
  bool turned = false;
  
  if( IS_RACEWAR_GOOD(ch) )
  {
    // a good raced player touches
    world_echo(ns_messages[_GLOBAL_GOOD_TOUCH]);
    
    STONE_ALIGN(stone)++;
    update_nexus_stone_align(STONE_ID(stone), STONE_ALIGN(stone));
    
    if( STONE_ALIGN(stone) >= STONE_ALIGN_GOOD )
    {
      // turned to good
      turned = true;
      world_echo(ns_messages[_GLOBAL_GOOD_TURN]);
      STONE_ALIGN(stone) = STONE_ALIGN_GOOD; // prevent overflow
      
      statuslog(57, "Nexus stone (%d) turned to good by %s", STONE_ID(stone), GET_NAME(ch));
      logit(LOG_STATUS, "Nexus stone (%d) turned to good by %s", STONE_ID(stone), GET_NAME(ch));
      sql_log(ch, WIZLOG, "Nexus stone (%d) turned to good", STONE_ID(stone));

      load_guardian(STONE_ID(stone), ch->in_room, STONE_ALIGN_GOOD);
      remove_nexus_sage(STONE_ID(stone));
      //load_sage(STONE_ID(stone), ch->in_room, STONE_ALIGN_GOOD);
    }
  }
  else if( IS_RACEWAR_EVIL(ch) )
  {
    // an evil raced player touches
    world_echo(ns_messages[_GLOBAL_EVIL_TOUCH]);
    
    STONE_ALIGN(stone)--;
    update_nexus_stone_align(STONE_ID(stone), STONE_ALIGN(stone));
    
    if( STONE_ALIGN(stone) <= STONE_ALIGN_EVIL )
    {
      // turned to evil
      turned = true;
      world_echo(ns_messages[_GLOBAL_EVIL_TURN]);
      STONE_ALIGN(stone) = STONE_ALIGN_EVIL; // prevent overflow

      statuslog(57, "Nexus stone (%d) turned to evil by %s", STONE_ID(stone), GET_NAME(ch));
      logit(LOG_STATUS, "Nexus stone (%d) turned to evil by %s", STONE_ID(stone), GET_NAME(ch));
      sql_log(ch, WIZLOG, "Nexus stone (%d) turned to evil", STONE_ID(stone));

      load_guardian(STONE_ID(stone), ch->in_room, STONE_ALIGN_EVIL);
      remove_nexus_sage(STONE_ID(stone));
      //load_sage(STONE_ID(stone), ch->in_room, STONE_ALIGN_EVIL);
    }
  }
  
  if( turned )
  {
    // prevent the stone from being turned right back
    STONE_TURN_TIMER(stone) = time_now;
    
    nexus_stone_epics(ch, stone);
  
    if (ch->group)
    {
      for (struct group_list *gl = ch->group; gl; gl = gl->next)
      {
	if (gl->ch->in_room == ch->in_room)
	  check_boon_completion(gl->ch, NULL, STONE_ID(stone), BOPT_NEXUS);
      }
    }
    else
      check_boon_completion(ch, NULL, STONE_ID(stone), BOPT_NEXUS);
  }      
  
  update_nexus_stat_mods();
  
  // handle normal touch
  if( !IS_TRUSTED(ch) )
  {
    add_event(event_nexus_stone_hum, (get_property("nexusStones.touchTimerSecs", 10) * WAIT_SEC), 0, 0, stone, 0, 0, 0);
  }
  
  return TRUE;
}

void event_nexus_stone_hum(P_char __ch, P_char __victim, P_obj stone, void *data)
{
  SET_BIT(stone->extra_flags, ITEM_HUM);
  act(ns_messages[_STONE_HUM], FALSE, 0, stone, 0, TO_ROOM);
}

// nexus_stone object spec. only assigned if nexus stones initialized
int nexus_stone(P_obj stone, P_char ch, int cmd, char *arg)
{
  if( !stone )
    raise(SIGSEGV);
    
  if( cmd == CMD_SET_PERIODIC )
    return TRUE;

  if( cmd == CMD_PERIODIC )
  {
    // find nexus sage; if not there, spawn one
    P_char sage = get_nexus_sage(STONE_ID(stone));
    
    int time_now = time(NULL);
    
    if( !sage &&
        STONE_TURNED(stone) &&
        ( STONE_SAGE_TIMER(stone) < ( time_now - (int) get_property("nexusStones.sage.respawnWaitSecs", 3600) ) ) )
    {
      //load_sage( STONE_ID(stone), stone->loc.room, STONE_ALIGN(stone) );  
    }
    
    if( nexus_stone_expired(STONE_ID(stone)) )
    {
      expire_nexus_stone(STONE_ID(stone));
    }
    
    return TRUE;    
  }  
  
  if( !IS_PC(ch) )
    return FALSE;
  
  char buff[MAX_STRING_LENGTH];
  char buff2[MAX_STRING_LENGTH];
  argument_interpreter(arg, buff, buff2);

  if( !isname(buff, stone->name) )
    return FALSE;
    
  if( cmd == CMD_KICK && IS_TRUSTED(ch) )
  {
    send_to_char("Stone timers reset.\n", ch);
    STONE_TIMER(stone) = 0;
    STONE_TURN_TIMER(stone) = 0;
    SET_BIT(stone->extra_flags, ITEM_HUM);
    return TRUE;
  }
  
  if( cmd == CMD_TOUCH )
  {
    nexus_stone_touch(stone, ch);
    return TRUE;
  }
  
  return FALSE;
}

void nexus_stone_epics(P_char ch, P_obj stone)
{
  struct affected_type *afp;
  int amount_max = get_property("nexusStones.epicsMax", 70);
  int amount_min = get_property("nexusStones.epicsMin", 50);

  if (STONE_ID(stone) == CYRICS_REVENGE_NEXUS_STONE && nexus_to_cyrics_revenge)
  {
      amount_max *= 2;
      amount_min *= 2;
  }

  int amount = amount_max;
  if( afp = get_epic_task(ch) )
  {
    if( afp->modifier - SPILL_BLOOD == STONE_ID(stone) )
    {
      send_to_char("The &+rGods of Duris&n are very pleased with your success.\n", ch);
      send_to_char("You can now progress further in your quest for epic power!\n", ch);
      amount *= 2;
      affect_remove(ch, afp);
    }
  }

  // give out epics for turning the stone
  gain_epic(ch, EPIC_NEXUS_STONE, STONE_ID(stone), 
            number( amount_min, amount )
            );

  if( ch->group )
  {
    for( struct group_list *gl = ch->group; gl; gl = gl->next )
    {
      if( ch == gl->ch ) continue;
      if( !IS_PC(gl->ch) || IS_TRUSTED(gl->ch) ) continue;
      amount = amount_max;
      if( gl->ch->in_room == ch->in_room )
      {
        if( (afp = get_epic_task( gl->ch )) &&
             (( afp->modifier - SPILL_BLOOD ) == STONE_ID( stone )) )
        {
          send_to_char("The &+rGods of Duris&n are very pleased with your success.\n", gl->ch);
          send_to_char("You can now progress further in your quest for epic power!\n", gl->ch);
          amount *= 2;
          affect_remove(gl->ch, afp);
        }
        gain_epic(gl->ch, EPIC_NEXUS_STONE, STONE_ID(stone), 
                  number( amount_min, amount )
                  );
      }
    }
  }
}

void world_echo(char *str)
{
  for (P_desc d = descriptor_list; d; d = d->next)
  {
    if (d->connected == CON_PLYNG)
    {
      send_to_char(str, d->character);
      send_to_char("\n", d->character);
    }
  }
}

// get the object pointer from a stone_id
P_obj get_nexus_stone(int stone_id)
{
  for( P_obj tobj = object_list; tobj; tobj = tobj->next )
  {
    if( IS_NEXUS_STONE(tobj) && STONE_ID(tobj) == stone_id )
    {
      return tobj;
    }
    
  }
  return NULL;
}

// get the guardian pointer from a stone_id
P_char get_nexus_guardian(int stone_id)
{
  for( P_char tch = character_list; tch; tch = tch->next )
  {
    if( IS_NEXUS_GUARDIAN(tch) && guardian_stone_id(tch) == stone_id )
    {
      return tch;
    }
  }
  return NULL;
}

// get the guardian pointer from a stone_id
P_char get_nexus_sage(int stone_id)
{
  for( P_char tch = character_list; tch; tch = tch->next )
  {
    if( IS_NEXUS_SAGE(tch) && guardian_stone_id(tch) == stone_id )
    {
      return tch;
    }
  }
  return NULL;
}

// get the stone_id from a guardian
int guardian_stone_id(P_char ch)
{
  for (struct affected_type *afp = ch->affected; afp; afp = afp->next)
  {
    if (afp->type == TAG_GUARD_NEXUS_STONE)
    {
      return afp->modifier;
    }
  }
  return -1;
}

void nexus_guardian_nuke(P_char ch, P_char victim, int dam)
{
  if( !victim )
    return;
  
  act("$n&+W's surface seems to ripple...\n            &+W...and a &+YMASSIVE &+Wburst of energy is released!&n", FALSE, ch, 0, victim, TO_ROOM);
  
  struct damage_messages messages = {
    "Your beam lances into $N!",
    "&+WThe energy beam slams into you!",
    "&+WThe energy beam slams into $N!",
    "&+WYour beam utterly destroys $N!",
    "&+WThe energy beam annihilates you!"
    "&+WThe energy beam utterly annihilates $N!", 0
  };
  
  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void nexus_guardian_energy_burst(P_char ch, bool show_message = true)
{
  P_char tch, next;
  
  if( show_message )
  {
    act("&+BSuddenly, $n &+Wexplodes into a blazing blue&+B eldritch flame. As he continues\n"\
        "&+Bhis attack, the flame suddenly &+Wflares red, and time&+B itself seems to slow down to a crawl.\n" \
        "&+BSuddenly, a crackling wave of kinetic &+Wenergy&+B lashes out..headed directly at YOU!", FALSE, ch, 0, 0, TO_ROOM);
  }
  
  for (tch = world[ch->in_room].people; tch; tch = next)
  {
    next = tch->next_in_room;
    if (should_area_hit(ch, tch))
    {
      int chance = BOUNDED(25, GET_C_AGI(tch) - 50, 75);
      
      if( number(0,100) < chance )
      {
        act("You deftly slip through the wave of energy.", FALSE, ch, 0, tch, TO_VICT);
        act("$N deftly slips through the wave of energy.", FALSE, ch, 0, tch, TO_NOTVICT);          
        continue;
      }
      
      int door = number(0,9);
      if ((CAN_GO(tch, door)) && (!check_wall(tch->in_room, door)))
      {
        act("The wave slams into you, throwing from the room!", FALSE, ch, 0, tch, TO_VICT);
        act("The wave slams into $N, throwing $M from the room!", FALSE, ch, 0, tch, TO_NOTVICT);
        int target_room = world[tch->in_room].dir_option[door]->to_room;
        char_from_room(tch);
        if (char_to_room(tch, target_room, -1))
        {
          act("$n flies in, crashing on the floor!", TRUE, tch, 0, 0, TO_ROOM);
          SET_POS(tch, POS_PRONE + GET_STAT(tch));
          stop_fighting(tch);
          if( IS_DESTROYING(tch) )
            stop_destroying(tch);
          CharWait(tch, PULSE_VIOLENCE * 3);
        }
        
      }
      else
      {
        act("The wave slams into you, knocking you off your feet!", FALSE, ch, 0, tch, TO_VICT);
        act("The wave slams into $N, knocking $M off $S feet!", FALSE, ch, 0, tch, TO_NOTVICT);
        SET_POS(tch, POS_SITTING + GET_STAT(tch));
        stop_fighting(tch);
        if( IS_DESTROYING(tch) )
          stop_destroying(tch);
        CharWait(tch, PULSE_VIOLENCE * 2);
      }
    
    }
  }

}

int nexus_guardian_pwn_mace(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char victim;
  
  if( !ch )
    return FALSE;
  
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
      
  if( cmd == CMD_GOTHIT && !number(0,5) )
  {
    struct proc_data *data = (struct proc_data *) arg;
    victim = data->victim;
    
    if( ch == victim )
      return FALSE;

    act("&+wAs your mighty blow impacts on $n, time itself &+Wseems to stop.\n" \
        "&+wAs you watch your comrades freeze around you, &+W$n &+Wstares down\n" \
        "&+wtowards you with what appears to be a &+Wcondescending gaze. The aura around your\n" \
        "&+wweapon begins to bend and &+Wtwist, and time seems to resume, as the energy of your\n" \
        "&+wblow is being &+Wsiphoned into $n!", FALSE, ch, 0, victim, TO_VICT);
    
    act("&+wYou feel &+Mstrange&+w for a moment. Suddenly, reality seems to resume,\n"
        "and the &+raura&+w around $N's weapon is absorbed by $n.", FALSE, ch, 0, victim, TO_NOTVICT);
        
    if( counter(ch, TAG_HIT_COUNTER) > number(5,10) )
    {
      remove_counter(ch, TAG_HIT_COUNTER);
      nexus_guardian_energy_burst(ch);
      return TRUE;
    }
    
    add_counter(ch, TAG_HIT_COUNTER);
    return TRUE;
  }
  
  if( cmd == CMD_GOTNUKED && !number(0,5) )
  {
    struct proc_data *data = (struct proc_data *) arg;
    int dam = data->dam;
    victim = data->victim;
    
    if( ch == victim )
      return FALSE;
    
    act("&+LAs you finish your &+Bincantation&+L, nothing seems to happen. Suddenly, a huge burst \n"
        "&+Lof pure &+Geldritch &+Menergy&+L surrounds $n&+L, channeling the very power of the &+Bweave&+L into $s body!",
        FALSE, ch, 0, victim, TO_VICT);

    act("&+LAs $N finishes $S &+Bincantation&+L, nothing seems to happen. Suddenly, a huge burst \n"
        "&+Lof pure &+Geldritch &+Menergy&+L surrounds $n&+L, channeling the very power of the &+Bweave&+L into $s body!",
        FALSE, ch, 0, victim, TO_NOTVICT);
    
    int tdam = counter(ch, TAG_NUKED_COUNTER);
    if( tdam > number(1500,2000) )
    {
      P_char target;
      
      if( ch->specials.fighting )
      {
        target = ch->specials.fighting;
      }
      else
      {
        target = victim;
      }

      remove_counter(ch, TAG_NUKED_COUNTER);
      nexus_guardian_nuke(ch, target, tdam);
      return TRUE;
    }
    
    add_counter(ch, TAG_NUKED_COUNTER, dam, -1);
    return TRUE;
  }
  
  if( cmd == CMD_MELEE_HIT && (P_char) arg && !number(0, 18) )
  {
    spell_prismatic_spray(60, ch, 0, 0, (P_char) arg, 0);    
    return TRUE;
  }
  
  return FALSE;
}

// nexus guardian mob spec. only set if nexus stones initialized
int nexus_stone_guardian(P_char ch, P_char pl, int cmd, char *arg)
{
  if( ch->player.birthplace && ch->in_room != real_room(ch->player.birthplace) )
  {
    act("$n pops out of existence.&n", FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(ch->player.birthplace), -1);
    act("$n pops into existence.&n", FALSE, ch, 0, 0, TO_ROOM);
  }

  int stone_id = guardian_stone_id(ch);
  
  if( cmd == CMD_DEATH )
  {    
    if( stone_id == -1 )
    {
      debug("nexus_stone_guardian(CMD_DEATH): couldn't find stone_id (%d)!", stone_id);
      logit(LOG_DEBUG, "nexus_stone_guardian(CMD_DEATH): couldn't find stone_id (%d)!", stone_id);
    }
    
    P_obj ns = get_nexus_stone(stone_id);
    
    if( !ns )
    {
      debug("nexus_stone_guardian(CMD_DEATH): couldn't find stone (%d)!", stone_id);
      logit(LOG_DEBUG, "nexus_stone_guardian(CMD_DEATH): couldn't find stone (%d)!", stone_id);
    }
        
    if( ch->specials.alignment > 0 )
    {
      act(ns_messages[_GOOD_GUARD_DIE], FALSE, ch, 0, 0, TO_ROOM);      
    } else {
      act(ns_messages[_EVIL_GUARD_DIE], FALSE, ch, 0, 0, TO_ROOM);            
    }
        
    nexus_guardian_energy_burst(ch, false);
    
    return TRUE;
  }
  
  return FALSE;
}

// load guardian mob into a room, set the correct flags
P_char load_guardian(int stone_id, int rroom_id, int align) 
{
  statuslog(57, "Nexus Guardian [%d] loaded in [%d]", stone_id, ROOM_VNUM(rroom_id));
  logit(LOG_STATUS, "Nexus Guardian [%d] loaded in [%d]", stone_id, ROOM_VNUM(rroom_id));

  int mob_id;
  if( align < 0 )
  {
    mob_id = MOB_EVIL_GUARDIAN;
  }
  else if( align > 0 )
  {
    mob_id = MOB_GOOD_GUARDIAN;
  }
  else
  {
    mob_id = MOB_NEUTRAL_GUARDIAN;
  }
  
  P_char mob = read_mobile(real_mobile(mob_id), REAL);
 
  if( !mob )
  {
    logit(LOG_DEBUG, "load_guardian():0 mob %d not loadable.", mob_id);
    debug("load_guardian(): mob %d not loadable.", mob_id);
    return NULL;
  }

  SET_BIT(mob->specials.act, ACT_SPEC);
  SET_BIT(mob->specials.act, ACT_SPEC_DIE);
  
  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = 30000;
  
  GET_PLATINUM(mob) = 0;
  GET_GOLD(mob) = 0;
  GET_SILVER(mob) = 0;
  GET_COPPER(mob) = 0;
      
  GET_HOME(mob) = GET_BIRTHPLACE(mob) = GET_ORIG_BIRTHPLACE(mob) = world[rroom_id].number;
  
  struct affected_type af;
  memset(&af, 0, sizeof(af));
  af.type = TAG_GUARD_NEXUS_STONE;
  af.flags = AFFTYPE_PERM;
  af.modifier = stone_id;
  af.duration = -1;
  affect_to_char(mob, &af);
  
  P_obj mace = read_object(real_object(OBJ_GUARDIAN_MACE), REAL);
  
  if( !mace )
  {
    logit(LOG_DEBUG, "load_guardian(): guardian mace %d not loadable.", OBJ_GUARDIAN_MACE);
    debug("load_guardian(): guardian mace %d not loadable.", OBJ_GUARDIAN_MACE);
  } 
  else
  {
    equip_char(mob, mace, PRIMARY_WEAPON, TRUE);
  }

  char_to_room(mob, rroom_id, 0);

  if( align < 0 )
  {
    act(ns_messages[_EVIL_GUARD_LOAD], FALSE, mob, 0, 0, TO_ROOM);
  }
  else if( align > 0 )
  {
    act(ns_messages[_GOOD_GUARD_LOAD], FALSE, mob, 0, 0, TO_ROOM);
  }
  else
  {
    act(ns_messages[_NEUTRAL_GUARD_LOAD], FALSE, mob, 0, 0, TO_ROOM);
  }

  return mob;
}

int nexus_sage_ask(P_char ch, P_char pl, char *arg)
{
  char buff[MAX_STRING_LENGTH];
  arg = one_argument(arg, buff);

  if( !isname(buff, ch->player.name) )
  {
    return FALSE;
  }

  int stone_id = guardian_stone_id(ch);
  NexusStoneInfo info;

  if( !nexus_stone_info(stone_id, &info) )
  {
    debug("nexus_sage(CMD_ASK): couldn't find stone info (%d)!", stone_id);
    logit(LOG_DEBUG, "nexus_sage(CMD_ASK): couldn't find stone info (%d)!", stone_id);
    return FALSE;
  }

  sh_int current_stat = *char_stat(pl, info.stat_affect);

  if( current_stat >= 100 )
  {
    act("$n says, 'You have already surpassed my ability to train you!'", FALSE, ch, 0, pl, TO_VICT);
  }
  else
  {
    sprintf(buff, "$n says, 'If you have acquired truly epic experiences, type \"train\" to learn from my considerable %s.'", apply_names[info.stat_affect]);
    act(buff, FALSE, ch, 0, pl, TO_VICT);
  }

  return TRUE;
}

int nexus_sage_train(P_char ch, P_char pl, char *arg)
{
  char buff[MAX_STRING_LENGTH];

  int stone_id = guardian_stone_id(ch);
  NexusStoneInfo info;

  if( !nexus_stone_info(stone_id, &info) )
  {
    debug("nexus_sage(CMD_TRAIN): couldn't find stone info (%d)!", stone_id);
    logit(LOG_DEBUG, "nexus_sage(CMD_TRAIN): couldn't find stone info (%d)!", stone_id);
    return FALSE;
  }

  int current_stat = (int) (*char_stat(pl, info.stat_affect));

  if( current_stat >= 100 )
  {
    act("$n says, 'You have already surpassed my ability to teach you!'", FALSE, ch, 0, pl, TO_VICT);
    return TRUE;
  }

  int cost = (int) get_property("nexusStones.sage.statAddCost", 1);
  int stat_add = (int) get_property("nexusStones.sage.statAdd", 3);

  if( GET_EPIC_POINTS(pl) < cost )
  {
    act("$n says, 'You have not learned enough. Come back later when you are ready!'", FALSE, ch, 0, pl, TO_VICT);
    return TRUE;
  }

  ch->only.pc->epics -= cost;
  (*char_stat(pl, info.stat_affect)) = BOUNDED(1, (current_stat + stat_add), 100);

  act("You sit at the feet of $n&n and learn, gaining something of $s ability.", FALSE, ch, 0, pl, TO_VICT);

  sprintf(buff, "&+WYou feel your %s increasing!\r\n", apply_names[info.stat_affect]);
  send_to_char(buff, pl);

  do_save_silent(pl, 1);

  act("\nAfter imparting you with $s knowledge, $n&n utters a word and disappears completely.", FALSE, ch, 0, pl, TO_VICT);  

  extract_char(ch);

  P_obj ns = get_nexus_stone(stone_id);

  if( ns )
    STONE_SAGE_TIMER(ns) = time(NULL);

  return TRUE;
}


// nexus sage mob spec. only set if nexus stones initialized
int nexus_sage(P_char ch, P_char pl, int cmd, char *arg)
{
  if( pl )
  {
    if( (IS_GOOD(ch) && !GOOD_RACE(pl))
      || (IS_EVIL(ch) && !EVIL_RACE(pl)) )
    {
      return FALSE;
    }

    switch( cmd )
    {
      case CMD_ASK:
        return nexus_sage_ask(ch, pl, arg);
        break;
      case CMD_TRAIN:
        return nexus_sage_train(ch, pl, arg);
        break;
    }
  }

  if( cmd == CMD_DEATH )
  {
    int stone_id = guardian_stone_id(ch);

    if( stone_id == -1 )
    {
      debug("nexus_sage(CMD_DEATH): couldn't find stone_id (%d)!", stone_id);
      logit(LOG_DEBUG, "nexus_sage(CMD_DEATH): couldn't find stone_id (%d)!", stone_id);
      return TRUE;
    }

    P_obj ns = get_nexus_stone(stone_id);

    if( !ns )
    {
      debug("nexus_sage(CMD_DEATH): couldn't find stone (%d)!", stone_id);
      logit(LOG_DEBUG, "nexus_sage(CMD_DEATH): couldn't find stone (%d)!", stone_id);
      return TRUE;
    }

    STONE_SAGE_TIMER(ns) = time(NULL);

    if( ch->specials.alignment > 0 )
    {
      act(ns_messages[_GOOD_SAGE_DIE], FALSE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      act(ns_messages[_EVIL_SAGE_DIE], FALSE, ch, 0, 0, TO_ROOM);
    }

    P_char guardian = get_nexus_guardian(stone_id);

    if( guardian && IS_ALIVE(guardian) )
    {
      GET_HIT(guardian) = (int)( GET_HIT(guardian) * get_property("nexusStones.sage.death.guardian.hit.modifier", 0.500));
      act("$n&n's presence trembles slightly, as $s energy is siphoned away.", FALSE, guardian, 0, 0, TO_ROOM);
    }

    return TRUE;
  }

  return FALSE;
}

// load sage mob into a room, set the correct flags
P_char load_sage(int stone_id, int rroom_id, int align) 
{
  statuslog(57, "Nexus Sage [%d] loaded in [%d]", stone_id, ROOM_VNUM(rroom_id));
  logit(LOG_STATUS, "Nexus Sage [%d] loaded in [%d]", stone_id, ROOM_VNUM(rroom_id));
  
  int mob_id;
  if( align < 0 )
  {
    mob_id = MOB_EVIL_SAGE;
  }
  else 
  {
    mob_id = MOB_GOOD_SAGE;
  }
  
  P_char mob = read_mobile(real_mobile(mob_id), REAL);
  
  if( !mob )
  {
    logit(LOG_DEBUG, "load_sage():0 mob %d not loadable.", mob_id);
    debug("load_sage(): mob %d not loadable.", mob_id);
    return NULL;
  }
  
  SET_BIT(mob->specials.act, ACT_SPEC);
  SET_BIT(mob->specials.act, ACT_SPEC_DIE);
  
  GET_MAX_HIT(mob) = GET_HIT(mob) = mob->points.base_hit = (int) get_property("nexusStones.sage.hitPoints", 6000);
  
  GET_PLATINUM(mob) = 0;
  GET_GOLD(mob) = 0;
  GET_SILVER(mob) = 0;
  GET_COPPER(mob) = 0;
  
	GET_HOME(mob) = GET_BIRTHPLACE(mob) = GET_ORIG_BIRTHPLACE(mob) = world[rroom_id].number;
  
  struct affected_type af;
  memset(&af, 0, sizeof(af));
  af.type = TAG_GUARD_NEXUS_STONE;
  af.flags = AFFTYPE_PERM;
  af.modifier = stone_id;
  af.duration = -1;
  affect_to_char(mob, &af);

  NexusStoneInfo info;
  
  if( !nexus_stone_info(stone_id, &info) )
  {
    logit(LOG_DEBUG, "load_sage(): error reading nexus stone info.");
    debug("load_sage(): error reading nexus stone info.");
  }
  else
  {
    char buff[MAX_STRING_LENGTH];
    buff[0] = '\0';
    
    if( align > 0 )
    {
      // good
      strcat(buff, "&+wthe &+WSage&+w of ");
      strcat(buff, info.name.c_str());
      mob->player.short_descr = str_dup(buff);
    }
    else
    {
      // evil
      strcat(buff, "&+wthe &+LSage&+w of ");
      strcat(buff, info.name.c_str());
      mob->player.short_descr = str_dup(buff);
    }
    
  }
  
  char_to_room(mob, rroom_id, 0);

  if( align > 0 )
  {
    act(ns_messages[_GOOD_SAGE_LOAD], FALSE, mob, 0, 0, TO_ROOM);
  }
  else
  {
    act(ns_messages[_EVIL_SAGE_LOAD], FALSE, mob, 0, 0, TO_ROOM);    
  }
  
  
  return mob;
}

int remove_nexus_sage(int stone_id)
{
  for( P_char tch = character_list; tch; tch = tch->next )
  {
    if( IS_NEXUS_SAGE(tch) && guardian_stone_id(tch) == stone_id )
    {
      extract_char(tch);
      return TRUE;
    }
  }
  
  return FALSE;
}

// load nexus stone into room, set the correct flags
bool load_nexus_stone(int stone_id, const char* stone_name, int room_vnum, int align)
{
  if (stone_id == CYRICS_REVENGE_NEXUS_STONE && nexus_to_cyrics_revenge)
  {
    if (cyrics_revenge != 0)
      room_vnum = get_cyrics_revenge_nexus_rvnum(cyrics_revenge);
    else
      return false;
  }
      
  P_obj stone = read_object(real_object(OBJ_NEXUS_STONE), REAL);
  
  if( !stone )
  {
    logit(LOG_DEBUG, "load_nexus_stone(): nexus stone obj %d not loadable.", OBJ_NEXUS_STONE);
    debug("load_nexus_stone(): nexus stone obj %d not loadable.", OBJ_NEXUS_STONE);
    return FALSE;    
  }
  
  STONE_ID(stone) = stone_id;
  STONE_ALIGN(stone) = align;

  SET_BIT(stone->extra_flags, ITEM_HUM);

  STONE_TIMER(stone) = 0;
  STONE_TURN_TIMER(stone) = 0;
  STONE_SAGE_TIMER(stone) = 0;
  
  char namebuff[MAX_STRING_LENGTH];
  sprintf(namebuff, "nexus stone %s", strip_ansi(stone_name).c_str());
  stone->name = str_dup(namebuff);

  sprintf(namebuff, "The nexus stone of %s stands here.", stone_name);
  stone->description = str_dup(namebuff);

  sprintf(namebuff, "the nexus stone of %s", stone_name);
  stone->short_description = str_dup(namebuff);

  obj_to_room(stone, real_room(room_vnum));

  statuslog(57, "Nexus Stone (%d) loaded in [%d]", stone_id, room_vnum);
  logit(LOG_STATUS, "Nexus Stone (%d) loaded in [%d]", stone_id, room_vnum);
  
  if( align <= STONE_ALIGN_EVIL )
  {
    STONE_TURN_TIMER(stone) = time(NULL);
    load_guardian(stone_id, real_room(room_vnum), STONE_ALIGN_EVIL);
    //load_sage(stone_id, real_room(room_vnum), STONE_ALIGN_EVIL);
  }
  else if( align >= STONE_ALIGN_GOOD )
  {
    STONE_TURN_TIMER(stone) = time(NULL);
    load_guardian(stone_id, real_room(room_vnum), STONE_ALIGN_GOOD);
    //load_sage(stone_id, real_room(room_vnum), STONE_ALIGN_GOOD);
  }
  else if( align == 0 )
  {
    STONE_TURN_TIMER(stone) = time(NULL);
    load_guardian(stone_id, real_room(room_vnum), 0);
  }

  return TRUE;
}

void do_nexus(P_char ch, char *arg, int cmd)
{
  char buff2[MAX_STRING_LENGTH];
  char buff3[MAX_STRING_LENGTH];
  
  if( !ch || IS_NPC(ch) )
    return;

  argument_interpreter(arg, buff2, buff3);
  
  if( IS_TRUSTED(ch) && !str_cmp("reset", buff2) )
  {
    reset_nexus_stones(ch);
    return;
  }
  
  if( IS_TRUSTED(ch) && !str_cmp("reload", buff2) )
  {
    if( strlen(buff3) < 1 )
    {
      send_to_char("syntax: nexus reload <stone id>\r\n", ch);
      return;
    }

    reload_nexus_stone(ch, atoi(buff3));
    return;
  }

  if( IS_TRUSTED(ch) )
    nexus_stone_god_list(ch);
  else
    nexus_stone_list(ch);
    
}

void nexus_stone_list(P_char ch)
{
  char buff[MAX_STRING_LENGTH];

  if( !qry("SELECT name, align FROM nexus_stones WHERE align IN ('%d', '%d') ORDER BY id", STONE_ALIGN_GOOD, STONE_ALIGN_EVIL) )
    return;
  
  sprintf(buff, "&+WNexus Stones &+G=================================\n\n");
  send_to_char(buff, ch);
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    send_to_char("  All stones are neutral.\n", ch); 
    
    mysql_free_result(res);
    return;
  }
  
  MYSQL_ROW row;
  while( row = mysql_fetch_row(res) )
  {
    char* stone_name = row[0];
    int align = atoi(row[1]);
    
    if( align == STONE_ALIGN_EVIL )
    {
      sprintf(buff, "  %s &n(&+Levil&n)\n", stone_name);
      send_to_char(buff, ch);
    }
    else if( align == STONE_ALIGN_GOOD )
    {
      sprintf(buff, "  %s &n(&+Wgood&n)\n", stone_name);
      send_to_char(buff, ch);
    }
  }
  
  mysql_free_result(res);
}

void nexus_stone_god_list(P_char ch)
{
  char buff[MAX_STRING_LENGTH];
  
  if( !qry("SELECT id, name, align FROM nexus_stones ORDER BY id", STONE_ALIGN_GOOD, STONE_ALIGN_EVIL) )
    return;
  
  sprintf(buff, "&+WNexus Stones &+G=================================\n\n");
  send_to_char(buff, ch);
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    send_to_char(" (no stones in database)\n", ch); 
    
    mysql_free_result(res);
    return;
  }
  
  MYSQL_ROW row;
  while( row = mysql_fetch_row(res) )
  {
    int stone_id = atoi(row[0]);
    char* stone_name = row[1];
    int align = atoi(row[2]);
    
    if( align == STONE_ALIGN_EVIL )
    {
      sprintf(buff, "  [&+W%d&n] %s &n(&+L%d&n)\n", stone_id, stone_name, align);
      send_to_char(buff, ch);
    }
    else if( align == STONE_ALIGN_GOOD )
    {
      sprintf(buff, "  [&+W%d&n] %s &n(&+W%d&n)\n", stone_id, stone_name, align);
      send_to_char(buff, ch);
    }
    else
    {
      sprintf(buff, "  [&+W%d&n] %s &n(%d)\n", stone_id, stone_name, align);
      send_to_char(buff, ch);
    }
  }
  
  mysql_free_result(res);
}  

void reset_nexus_stones(P_char ch)
{
  wizlog(57, "%s reset nexus stones", GET_NAME(ch));
  logit(LOG_WIZ, "%s reset nexus stones", GET_NAME(ch));
  
  vector<P_char> delete_chars;
  
  for( P_char tch = character_list; tch; tch = tch->next )
  {
    if( IS_NEXUS_GUARDIAN(tch) )
    {
      delete_chars.push_back(tch);
    }
  }

  while( delete_chars.size() )
  {
    extract_char(delete_chars.back());
    delete_chars.pop_back();
  }
  
  vector<P_obj> delete_objs;
  
  for( P_obj tobj = object_list; tobj; tobj = tobj->next )
  {
   if( IS_NEXUS_STONE(tobj) )
   {
     delete_objs.push_back(tobj);
   }
  }
  
  while( delete_objs.size() )
  {
    extract_obj(delete_objs.back(), TRUE);
    delete_objs.pop_back();
  }
    
  load_nexus_stones();
}

void reload_nexus_stone(P_char ch, int stone_id)
{
  wizlog(57, "%s reloaded nexus stone (%d)", GET_NAME(ch), stone_id);
  logit(LOG_WIZ, "%s reloaded nexus stone (%d)", GET_NAME(ch), stone_id);

  vector<P_char> delete_chars;

  for( P_char tch = character_list; tch; tch = tch->next )
  {
    if( ( IS_NEXUS_GUARDIAN(tch) || IS_NEXUS_SAGE(tch) ) && guardian_stone_id(tch) == stone_id )
    {
      delete_chars.push_back(tch);
    }
  }

  while( delete_chars.size() )
  {
    extract_char(delete_chars.back());
    delete_chars.pop_back();
  }

  for( P_obj tobj = object_list; tobj; tobj = tobj->next )
  {
    if( IS_NEXUS_STONE(tobj) && STONE_ID(tobj) == stone_id )
    {
      extract_obj(tobj);
      break;
    }
  }

  // load nexus stones from DB
  if( !qry("SELECT name, room_vnum, align FROM nexus_stones WHERE id = '%d'", stone_id) )
    return;

  MYSQL_RES *res = mysql_store_result(DB);

  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return;
  }

  MYSQL_ROW row = mysql_fetch_row(res);
  char* stone_name = row[0];
  int room_vnum = atoi(row[1]);
  int align = atoi(row[2]);

  load_nexus_stone(stone_id, stone_name, room_vnum, align);

  mysql_free_result(res);
  
  update_nexus_stat_mods();  
}

bool nexus_stone_expired(int stone_id)
{
  int threshold_secs = ( (int) get_property("nexusStones.expireDays", 30) ) * ( 24 * 60 * 60 );
  
  if( !qry("SELECT id FROM nexus_stones WHERE id = '%d' AND last_touched_at > 0 AND last_touched_at < ( unix_timestamp() - %d ) AND align <> 0", stone_id, threshold_secs) )
    return false;
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  bool expired = ( mysql_num_rows(res) > 0 );

  mysql_free_result(res);
  
  return expired;
}

void expire_nexus_stone(int stone_id)
{
  wizlog(57, "nexus stone (%d) expired", stone_id);
  logit(LOG_WIZ, "nexus stone (%d) expired", stone_id);  
   
  
  vector<P_char> delete_chars;
  
  for( P_char tch = character_list; tch; tch = tch->next )
  {
    if( ( IS_NEXUS_GUARDIAN(tch) || IS_NEXUS_SAGE(tch) ) && guardian_stone_id(tch) == stone_id )
    {
      delete_chars.push_back(tch);
    }
  }
  
  while( delete_chars.size() )
  {
    act("$n pops out of existence.&n", FALSE, delete_chars.back(), 0, 0, TO_ROOM);
    extract_char(delete_chars.back());
    delete_chars.pop_back();
  }
    
  for( P_obj tobj = object_list; tobj; tobj = tobj->next )
  {
    if( IS_NEXUS_STONE(tobj) && STONE_ID(tobj) == stone_id )
    {
      STONE_ALIGN(tobj) = 0;
      extract_obj(tobj);
      break;
    }
  }
 
  world_echo(ns_messages[_STONE_EXPIRED]);
  
  if( !qry("UPDATE nexus_stones SET align = 0, last_touched_at = 0 WHERE id = '%d'", stone_id) )
    return;
  
  // load nexus stones from DB
  if( !qry("SELECT name, room_vnum, align FROM nexus_stones WHERE id = '%d'", stone_id) )
    return;
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return;
  }
  
  MYSQL_ROW row = mysql_fetch_row(res);
  char* stone_name = row[0];
  int room_vnum = atoi(row[1]);
  int align = atoi(row[2]);
  
  load_nexus_stone(stone_id, stone_name, room_vnum, align);
  
  mysql_free_result(res);
  
  update_nexus_stat_mods();  
}  

P_obj get_random_enemy_nexus(P_char ch)
{
  int stones[MAX_NEXUS_STONES];
  int align;
  P_obj stone = NULL;

  if ( !qry("SELECT id, align FROM nexus_stones") || !ch)
  {
    debug("failed1");
    return NULL;
  }

  MYSQL_RES *res = mysql_store_result(DB);

  if ( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return NULL;
  }

  int i = 1;
  MYSQL_ROW row;
  while(row = mysql_fetch_row(res))
  {
    if (atoi(row[1]) == STONE_ALIGN_GOOD)
      align = RACEWAR_GOOD;
    else if (atoi(row[1]) == STONE_ALIGN_EVIL)
      align = RACEWAR_EVIL;
    else
      align = 0;

    if (GET_RACEWAR(ch) == align)
      continue;

    stones[i] = atoi(row[0]);
    i++;
  }
 
  int j = number(1, i);

  stone = get_nexus_stone(stones[j]);

  mysql_free_result(res);

  return stone;
}


// endif define __NO_MYSQL__
#endif

