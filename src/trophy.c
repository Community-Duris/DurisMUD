/*
 *  trophy.c
 *  Duris
 *
 *  Created by Torgal on 1/6/08.
 *
 */

#include <vector>
#include <cstring>
using namespace std;

#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "prototypes.h"
#include "utils.h"
#include "comm.h"
#include "objmisc.h"
#include "timers.h"
#include "trophy.h"
#include "sql.h"

extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern P_room world;
extern const int new_exp_table[];
extern P_index mob_index;

float modify_exp_by_zone_trophy(P_char ch, int type, float XP)
{
  if( !ch || !IS_PC(ch) )
    return XP;
  
  if( type != EXP_DAMAGE &&
      type != EXP_KILL &&
      type != EXP_QUEST &&
      type != EXP_MELEE &&
      type != EXP_HEALING &&
      type != EXP_TANKING)
    return XP;
  
  if (IS_ILLITHID(ch))
    return XP;
  
  if( GET_LEVEL(ch) < 25 )
    return XP;
  
  int zone_number = zone_table[world[ch->in_room].zone].number;
  
  int zone_exp = get_zone_exp(ch, zone_number);
  
 //  debug("%s exp: %d, zone_exp: %d, notch: %d, threshold: %d", GET_NAME(ch), (int)XP, zone_exp, EXP_NOTCH(ch), (3 * EXP_NOTCH(ch)) );
  
  int reduction_scale = (int) get_property("exp.zoneTrophy.scale.notches", 10);
  // if more than the threshold, adjust by average zone_exp from group
  if( zone_exp >= ( EXP_NOTCH(ch) * reduction_scale / 5))
  {
    int avg_zone_exp = 0, group_size = 0;
    
    if( ch->group )
    {
      for( struct group_list *gl = ch->group; gl; gl = gl->next )
      {
        if( !IS_PC(gl->ch) || 
           GET_LEVEL(gl->ch) < 20 ||
           ch->in_room != gl->ch->in_room )
          continue;
        
        avg_zone_exp += get_zone_exp(gl->ch, zone_number);
        group_size++;
      }
      
      group_size = MAX(group_size, 1); // prevent divide by 0 error
      avg_zone_exp = (int) avg_zone_exp / group_size;
    }
    else
    {
      avg_zone_exp = get_zone_exp(ch, zone_number);
    }
    
    int trophy_notches = (int) ( avg_zone_exp / EXP_NOTCH(ch));
    
//    debug("XP: %d, avg_zone_exp: %d, reduction scale: %d, trophy_notches: %d", (int)XP, avg_zone_exp, reduction_scale, trophy_notches);
    
    if( trophy_notches > 0 )
    {
      float exp_mod = 1.0 - ( (float) trophy_notches / (float) reduction_scale );
      
//      debug("exp_mod: %f", exp_mod);
      
      if( exp_mod > 1.0 )
      {
        exp_mod = 1.0;
      }
      else if( exp_mod < 0.05 )
      {
        exp_mod = 0.05;
      }
      
      if(exp_mod < 1.0 &&
        (type == EXP_KILL ||
         type == EXP_QUEST))
      {
        if(exp_mod >= 0.50 )
        {
          send_to_char("&+gThis area feels rather easy.\n", ch);
        }
        else if( exp_mod >= 0.30 )
        {
          send_to_char("&+yThis area really isn't much of a challenge.\n", ch);
        }
        else if( exp_mod >= 0.15 )
        {
          send_to_char("&+rWhat's the point? Isn't this area getting boring?\n", ch);
        }
        else
        {
          send_to_char("&+YYAWN! You really should find somewhere else to gain experience.\n", ch);
        }              
      }
      
//      debug("XP: %d", (int)XP);
      XP = XP * exp_mod;
//      debug("XP: %d", (int)XP);
    }
    
  }  
  
  update_zone_trophy(ch, zone_number, (int)XP);    
  
  return XP;
}

void do_trophy(P_char ch, char *arg, int cmd)
{
  int      real, clear_it = FALSE;
  char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];
  P_char   who, rch;
  struct trophy_data *tr;
  
  rch = GET_PLYR(ch);
  
  if (IS_NPC(rch))
  {
    send_to_char("Mobs don't have trophy, duh.\r\n", ch);
    return;
  }
  
  if (IS_ILLITHID(ch))
  {
    send_to_char("Illithids don't have trophy.\n", ch);
    return; 
  }

  argument_interpreter(arg, Gbuf2, Gbuf3);
  
  if (IS_TRUSTED(rch))
  {
    if (!(who = get_char_vis(rch, Gbuf2)))
    {
      send_to_char("They don't appear to be in the game.\r\n", ch);
      return;
    }
    
    if( *Gbuf3 && isname("frags", Gbuf3))
    {
      show_frag_trophy(ch, who);
      return;
    }
  }
  else
  {
    who = rch;
  }
  
  if( !IS_PC(who) )
  {
    send_to_char("Don't be silly, mobs don't have trophy!\n", ch);
    return;
  }
  
  if( isname("frags", Gbuf2) )
  {
    show_frag_trophy(ch, ch);
  }
  else
  {
    if( !ZONE_TROPHY(who) )
      return;
    
    send_to_char("&+WTrophy zones:&n\r\n", ch);
//    debug("exp notch: %d", EXP_NOTCH(who)); 

    int reduction_scale = (int) get_property("exp.zoneTrophy.scale.notches", 10);
    for( zone_trophy_iterator it = ZONE_TROPHY(who)->begin(); it != ZONE_TROPHY(who)->end(); it++ )
    {
      struct zone_info zone;
      if( !get_zone_info( it->zone_number, &zone) )
        continue;
      
      int notches = (int) ( (float) it->exp / (float) EXP_NOTCH(who) );
            
      if( it->exp <= 0 )
        continue;

      if( !IS_TRUSTED(ch) && notches < reduction_scale / 5 )
        continue;      
      
      if( notches >= reduction_scale / 1.5 )
      {
        strcpy(Gbuf2, "&+Roverdone");
      }
      else if( notches >= reduction_scale / 2.5 )
      {
        strcpy(Gbuf2, "&+yboring");
      }
      else if( notches >= reduction_scale / 5 )
      {
        strcpy(Gbuf2, "&+gexperienced");
      }
      else
      {
        strcpy(Gbuf2, "");
      }
      
      if( IS_TRUSTED(ch) )
      {        
        sprintf(Gbuf1, " %s  &n%s&n(%d:%d)\n", pad_ansi(zone.name.c_str(), 40).c_str(), Gbuf2, it->exp, notches);
      }
      else
      {
        sprintf(Gbuf1, " %s  &n%s\n", pad_ansi(zone.name.c_str(), 40).c_str(), Gbuf2);
      }      
      
      send_to_char(Gbuf1, ch);
    }
    
  }
  
}

void update_zone_trophy(P_char ch, int zone_number, int XP)
{
  // No more trophy!
  return;

  if( !ch || !IS_PC(ch) )
    return;
    
  if( XP <= 0 )
    return;

  struct zone_info zone;
  if( !get_zone_info(zone_number, &zone) || !zone.trophy_zone )
    return;  
  
  if( !ZONE_TROPHY(ch) )
  {
    ZONE_TROPHY(ch) = new vector<struct zone_trophy_data>();    
  }

  bool has_it = false;
  zone_trophy_iterator it;
  for( it = ZONE_TROPHY(ch)->begin(); it != ZONE_TROPHY(ch)->end(); it++ )
  {
    if( it->zone_number == zone_number )
    {
      has_it = true;
      break;
    }
  }
  
  if( has_it )
  {
    it->exp += XP;
    it->exp = MAX(0, it->exp);
  }
  else
  {
    struct zone_trophy_data z;
    z.zone_number = zone_number;
    z.exp = XP;
    z.exp = MAX(0, z.exp);
    ZONE_TROPHY(ch)->push_back(z);    
  }
  
  // lower all other zones a little bit
  for( it = ZONE_TROPHY(ch)->begin(); it != ZONE_TROPHY(ch)->end(); it++ )
  {
    if( it->zone_number != zone_number )
    {
      it->exp -= (int) ( XP * (float) get_property("exp.zoneTrophy.others.reductionPct", 0.01) );
      it->exp = MAX(0, it->exp);
    }
  }
      
}

int get_zone_exp(P_char ch, int zone_number)
{
  if( !ch || !IS_PC(ch) || !ZONE_TROPHY(ch) || ZONE_TROPHY(ch)->empty() )
    return 0;
 
  for( zone_trophy_iterator it = ZONE_TROPHY(ch)->begin(); it != ZONE_TROPHY(ch)->end(); it++ )
  {
    if( it->zone_number == zone_number )
      return it->exp;
  }
  
  return 0;
}

#ifdef __NO_MYSQL__
void load_zone_trophy(P_char ch)
{
}

void zone_trophy_update()
{
}

void save_zone_trophy(P_char ch)
{
}
#else
void load_zone_trophy(P_char ch)
{
  // No more trophy!
  return;

  if( !ch || !IS_PC(ch) )
    return;
    
  ZONE_TROPHY(ch) = new vector<struct zone_trophy_data>();
  
  if( !qry("SELECT zone_number, exp FROM zone_trophy, zones WHERE zone_trophy.zone_number = zones.number AND zones.trophy_zone = 1 " \
           "AND pid = %d", GET_PID(ch)) )
    return;
  
  MYSQL_RES *res = mysql_store_result(DB);
  
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return ;
  }
  
  MYSQL_ROW row;
  while( row = mysql_fetch_row(res) )
  {
    struct zone_trophy_data data;
    data.zone_number = atoi(row[0]);
    data.exp = atoi(row[1]);
    ZONE_TROPHY(ch)->push_back(data);
  }
  
  mysql_free_result(res);
}

void zone_trophy_update()
{
  if( !has_elapsed("zone_trophy_reduction", (int) get_property("exp.zoneTrophy.update.secs", 3600)) )
    return;
  
//  debug("zone_trophy_update()");
  
  qry("UPDATE zone_trophy SET exp = exp * %f", (float) get_property("exp.zoneTrophy.update.multiplier", 1.0));
  qry("DELETE FROM zone_trophy WHERE exp <= 0");
  
  set_timer("zone_trophy_reduction");
}

void save_zone_trophy(P_char ch)
{
  if( !ch || !IS_PC(ch) || !ZONE_TROPHY(ch) || ZONE_TROPHY(ch)->empty() )
    return;
  
  for( zone_trophy_iterator it = ZONE_TROPHY(ch)->begin(); it != ZONE_TROPHY(ch)->end(); it++ )
  {
    qry("SELECT exp FROM zone_trophy WHERE pid = %d AND zone_number = %d", GET_PID(ch), it->zone_number);
    
    MYSQL_RES *res = mysql_store_result(DB);
    
    if( mysql_num_rows(res) > 0 )
    {
      MYSQL_ROW row = mysql_fetch_row(res);
      int old_exp = atoi(row[0]);
      
      if( it->exp != old_exp )
      {
        qry("UPDATE zone_trophy SET exp = %d WHERE pid = %d AND zone_number = %d", 
            it->exp, GET_PID(ch), it->zone_number);        
      }
    }
    else
    {
      qry("INSERT INTO zone_trophy (pid, zone_number, exp) VALUES ('%d', '%d', '%d')", 
          GET_PID(ch), it->zone_number, it->exp);      
    }
    
    mysql_free_result(res);    
  }
}
#endif

/* this macro is used in the code to determine the size of a chars 'a'
 trophy.  If can be used to reduce trophy size based on <whatever> */
/*
__attribute__((deprecated)) int GET_TROPHY_SIZE(P_char ch)
{
  
	if IS_NPC(ch)
		return 0;
	
	if (RACE_GOOD(ch)){
    if( (GET_LEVEL(ch)) > MAX_TROPHY_SIZE)
			return MAX_TROPHY_SIZE;
    else
			return GET_LEVEL(ch);
	}
	else
	{
    if( (3 * GET_LEVEL(ch)) > MAX_TROPHY_SIZE)
			return MAX_TROPHY_SIZE;
    else
			return GET_LEVEL(ch) * 3;
	}
  
  
  
}

#define GET_TROPHY_SIZE_OLD(a) (IS_NPC(a) ? 0 :  \
GET_LEVEL(a) < 6 ? 1 : \
GET_LEVEL(a) > MAX_TROPHY_SIZE ? MAX_TROPHY_SIZE : \
GET_LEVEL(a))

__attribute__((deprecated)) int modify_exp_by_trophies(P_char ch, P_char victim, int XP, int type)
{
  struct trophy_data *tr1;
  int      group_fact;
  float    factor;
  
  if (type == EXP_KILL && IS_PC(ch) && IS_PC(victim))
  {
    if (!racewar(ch, victim))   // check for racewar
      XP = 0;
    else if (GET_LEVEL(victim) < 20)    //check if victim is higher then 20
      XP = 0;
    else if ((GET_LEVEL(ch) - GET_LEVEL(victim)) > 15)  //check if diff is bigger then 15
      XP = 0;
    else                        //else give this..
      XP = 60000 * GET_LEVEL(victim);
    return XP;
  }
  if (IS_PC(victim) || (XP < 10) || (GET_LEVEL(ch) < 6))
    return XP;
  
  if ((GET_LEVEL(ch) - 15) > GET_LEVEL(victim))
    return XP;
  
  tr1 = ch->only.pc->trophy;
  
  while ((tr1) && (tr1->vnum != mob_index[GET_RNUM(victim)].virtual_number))
    tr1 = tr1->next;
  
  if (type == EXP_KILL && !tr1)
  {                             
    struct trophy_data *temp1, *temp2;
    int      i, j;
    
    if (!dead_trophy_pool)
      dead_trophy_pool =
      mm_create("TROPHY", sizeof(struct trophy_data),
                offsetof(struct trophy_data, next), 3);
    
    tr1 = (struct trophy_data *) mm_get(dead_trophy_pool);
    
    tr1->next = ch->only.pc->trophy;
    tr1->kills = 0;
    tr1->vnum = mob_index[GET_RNUM(victim)].virtual_number;
    ch->only.pc->trophy = tr1;
    
    i = 0;
    j = BOUNDED(1, GET_TROPHY_SIZE(ch), MAX_TROPHY_SIZE);
    temp1 = ch->only.pc->trophy;
    while (temp1)
    {
      i++;
      if (i >= j)
      {
        while (temp1->next)
        {
          temp2 = temp1->next;  
          temp1->next = temp2->next;   
          mm_release(dead_trophy_pool, temp2);
        }
      }
      temp1 = temp1->next;
    }
  }
  
  if (type == EXP_KILL)
  {
    group_fact = 1;

    tr1->kills += 100 / MIN(group_fact, 100);
    
    
    if (tr1->kills > 10000)
      tr1->kills -= 10000;
    
    if (tr1->kills > 200)
    {
      if (tr1->kills < 1000)
        send_to_char
        ("You're beginning to learn your victim's weak spots.\r\n", ch);
      else if (tr1->kills < 2500)
        send_to_char
        ("Not as thrilling as the first few, but fun nonetheless.\r\n", ch);
      else if (tr1->kills < 5000)
        send_to_char("Ho hum. This is getting boring.\r\n", ch);
      else
        send_to_char
        ("What's the point anymore?  It just doesn't seem worth it!\r\n",
         ch);
    }
  }
  
  if (tr1)
  {
	  if(RACE_GOOD(ch)){
      factor =
			(((float) tr1->kills / 500.0) * ((float) tr1->kills / 500.0));
	  }
	  else {
      factor =
			(((float) tr1->kills / 100.0) * ((float) tr1->kills / 100.0));
      
      
      
	  }
    if (factor > 100.0)
      factor = 100.0;
    
    
  }
  else
    factor = 0;
  
  
  return (XP - (int) ((float) XP * ((float) factor / 100.0)));
}
 */
