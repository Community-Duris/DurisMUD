      /* Made by Zion and Jexni, for snogres */
#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <list>
using namespace std;

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "justice.h"
#include "assocs.h"
#include "graph.h"
#include "damage.h"
#include "reavers.h"
#include "specs.snogres.h"

extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern P_obj justice_items_list;
extern char *coin_names[];
extern const char *command[];
extern const char *dirs[];
extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int planes_room_num[];
extern int racial_base[];
extern int top_of_world;
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern const char *crime_list[];
extern const char *crime_rep[];
extern const char *specdata[][MAX_SPEC];
extern struct class_names class_names_table[];
int      range_scan_track(P_char ch, int distance, int type_scan);
extern P_obj    object_list;

int snogres_lich_shout(P_char ch, P_char tch, int cmd, char *arg)
{
  int      helpers[] = { 87734, 87743, 0 };
  if (cmd == -10)
    return TRUE;
  if (!tch && !number(0, 4))
    return shout_and_hunt(ch, 100, "&+WCome, my pet! Destroy %s&+L!", NULL, helpers, 0, 0);

  return FALSE;
}

#define FLESH_GOLEM_HELPER_LIMIT    6

int snogres_flesh_golem(P_char ch, P_char pl, int cmd, char *arg)
{
  register P_char i;
  P_char   golem, lich;
  int      count = 0;

  if (cmd == CMD_SET_PERIODIC)
  {
    return TRUE;
  }
  if (!ch)
  {
    return FALSE;
  }
  if (cmd != 0)
  {
    return FALSE;
  }
  if (IS_FIGHTING(ch))
  {
    /*
     * attempt to "summon" a flesh golem...only possible if less than FLESH_GOLEM_HELP_LIMIT
     * * in world
     */
    for (i = character_list; i; i = i->next)
    {
      if (IS_NPC(i))
      {
        if(GET_VNUM(i) == 87743)
        {
          count++;
        }
        else if(GET_VNUM(i) == 87733)
        {
          lich = i;
        }
      }
    }
    if (count < FLESH_GOLEM_HELPER_LIMIT)
    {
      if (number(1, 100) < 50)
      {
        golem = read_mobile(87734, VIRTUAL);
        if (!golem)
        {
          logit(LOG_EXIT, "assert: error in snogres_flesh_golem() proc");
          raise(SIGSEGV);
        }
        act
          ("$n &+rsuddenly &+Rshambles &+rfor a moment, and a piece of living &+rflesh&n drops to the ground.&n\r\n"
           "&+rThe piece of flesh suddenly sprouts four arms and two legs, and begins shambling towards &-L&+RYOU&+R!&n&n\r\n",
           FALSE, ch, 0, golem, TO_ROOM);
        char_to_room(golem, ch->in_room, 0);
	MobStartFight(golem, pl);
	add_event(event_snogres_golem_helper, number(100, 500), golem, NULL, NULL, 0, NULL, 0);
        return TRUE;
      }
    }
  }

  if(ch->in_room != real_room(87799))
  {
    if(!IS_FIGHTING(ch))
    {
      act("$n &+rlooks blankly about the area, and then sulks back to its room.&n", FALSE, ch, 0, 0, TO_ROOM);
      char_from_room(ch);
      char_to_room(ch, real_room(87799), 0);
      act("$n &+rsulks in from the west.&n", FALSE, ch, 0, 0, TO_ROOM);
    }
  }
  
  if(IS_FIGHTING(ch) && (GET_HIT(ch) < 2000))
  {
   act("&+rSensing danger, $n &+rmelts into a puddle and scurries away.&n", FALSE, ch, 0, 0, TO_ROOM);
    extract_char(ch);
    golem = read_mobile(87734, VIRTUAL);
    if (!golem)
    {
       logit(LOG_EXIT, "assert: error in snogres_flesh_golem() proc");
       raise(SIGSEGV);
    }
    char_to_room(golem, 87798, 0);
    add_follower(golem, lich);
    act("$n &+rbubbles and boils as it enters from the west, its wounds healing quickly.&n", FALSE, golem, 0, 0, TO_ROOM);
  }

  return FALSE;

}

#undef FLESH_GOLEM_HELPER_LIMIT


void event_snogres_golem_helper(P_char ch, P_char victim, P_obj obj, void *data)
{
act("$n &+rdisappears as the magic holding it is too weak to support it's existence.&n", TRUE, ch, 0, 0, TO_ROOM);
extract_char(ch);
}

void event_hellfire(P_char ch, P_char victim, P_obj obj, void *data)
{
  int dam;
  int count = *((int*)data);

  dam = number(15, 25);

  if ((GET_HIT(ch) - dam) > 0)
  {
    GET_HIT(ch) -= dam;
    send_to_char("&+mHellish &N&+Mflames &+Lflicker and dance across your &N&+mwounds!&N\n", ch);
    send_to_char("&+mThe anguish is unbearable as &+Rfresh &N&+rblood &+merupts from your body.&n\n", ch);
    act("$n's &+mwounds &+Mburn &+Lwith &+mhe&+Mllfi&N&+mre&+L!&N", TRUE, ch, NULL, NULL, TO_NOTVICT);
    make_bloodstain(ch);
  }

  if (count >= 0)
  {
    count--;
    add_event(event_hellfire, PULSE_VIOLENCE, ch, 0, 0, 0, &count, sizeof(count));
  }
  else
  {
    send_to_char("&+mThe flames vanish from whence they came, and your wounds stop bleeding.&N\n", ch);
  }
}

int hellfire_axe(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict = NULL;
  struct proc_data *data = NULL;
  struct affected_type af;
  int      dam = cmd / 1000;
  struct damage_messages messages = {
    "&+mYour &+Maxe &N&+mvibrates as it strikes $N&N&+m, causing a &+rsevere &+Rgash &N&+mand great &+Rblood &N&+rloss.&n",
    "&+mA wicked cut from $n&N&+m's &+Maxe &n&+mcauses you to &+Rbleed &N&+rprofusely.&n",
    "&+mA wicked cut from $n&N&+m's &+Maxe &n&+mcauses $N &+mto &+Rbleed &N&+rprofusely.&n",
    "&+mYour axe &+mbecomes lodged in $N&n&+m's chest.\n&+mAs you &+rrip &+myour weapon free, &+Rblood &N&+rgushes freely &+mfrom your vanquished foe.&n",
    "&+m$n&n&+m's axe &+Wshatters bone &N&+mand &+reviscerates organs\n&+mas it impacts with your chest.&n",
    "&+m$n&n&+m's axe &+mbecomes lodged in $N&n&+m's chest.\n&+mAs $e &+rrips &+mthe weapon free, &+Rblood &N&+rgushes freely &+mfrom $s vanquished foe.&n",
      0
  };

  if ( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }
  if ( !obj || !ch )
  {
    return FALSE;
  }

  if( (cmd == CMD_MELEE_HIT) && !number(0, 50))
  {
    vict = (P_char) arg;

    if (obj->loc.wearing->equipment[WIELD] != obj)
      return FALSE;

    if(!dam)
      return FALSE;

    if(!vict || !IS_HUMANOID(vict) || !IS_ALIVE(vict))
      return FALSE;
    
    melee_damage(ch, vict, (GET_LEVEL(ch) * number(3,5)), 0, &messages); //this will nerf it for pleveling - Jexni
    int numb = number(2, 6);
    add_event(event_hellfire, PULSE_VIOLENCE, vict, 0, 0, 0, &numb, sizeof(numb));

    return TRUE;
  }
  return FALSE;
}
    
int berserker_toss(P_char ch, P_char vict, int cmd, char *arg)
{
  P_char   tch, victtwo = NULL;
  P_obj    obj;
  int      numbPCs = 0, luckyPC = 0, currPC = 0, numb;

  /*
   * check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd)
    return FALSE;

  if (!IS_FIGHTING(ch))
    return FALSE;

  if (!ch->specials.fighting)
    return FALSE;

  /* count number of PCs, pick someone */

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (IS_PC(tch) && !IS_TRUSTED(tch))
      numbPCs++;
  }

  if (!numbPCs)
    return FALSE;               /* doh */

  luckyPC = number(0, numbPCs - 1);
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
     if (IS_PC(tch) && !IS_TRUSTED(tch))
     {
       if (currPC == luckyPC)
       {
         vict = tch;
         break;
       }
       else
         currPC++;
     }
  }

  if(vict)
  {
    numbPCs--;
  }
  else
  {
    return FALSE;
  }

  luckyPC = number(0, numbPCs - 1);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
     if (IS_PC(tch) && !IS_TRUSTED(tch) && tch != vict)
     {
       if (currPC == luckyPC)
       {
         victtwo = tch;
         break;
       }
       else
         currPC++;
     }
  }

  if (!vict || !victtwo)          
    return FALSE;  //redundant sanity check

  if(!number(0, 2))
  {
     act("&+rFilled with &+RRAGE&n $n &+rhammers his fist into &+R$N&n&+r, sending $m flying!&n", FALSE, ch, 0, vict, TO_NOTVICT);
     act("&+rFilled with &+RRAGE&n $n &+rhammers his fist into &+RYOU&n&+r, sending you &+RFLYING&N&+r!&n", FALSE, ch, 0, vict, TO_VICT);
     act("&+rFilled with &+RRAGE&n&+r, you hammer your fist into &+R$N&n&+r sending $m flying!&n", FALSE, ch, 0, vict, TO_CHAR);
     damage(ch, vict, (70 + number(0, 150)), TYPE_UNDEFINED);
 
     int i = 0;
     do
     {   //damage an item
       if (vict->equipment[i])
       {
         obj = vict->equipment[i];
         if (!number(0, 3))
         {
           DamageOneItem(vict, SPLDAM_GENERIC, obj, FALSE);
           break;
         }
       }
       i++;
     }
     while (i < MAX_WEAR);
  
  act("&+rHurtling through the air, &+R$n&N &+rslams into &+RYOU&N&+r!&n", FALSE, vict, 0, victtwo, TO_VICT);
  act("&+rYour path through the air sends you &+Rcareening violently &N&+rinto &+R$N&n&+r!&n", FALSE, vict, 0, victtwo, TO_CHAR); 
  act("&+R$n&n&+r's path through the air sends $m &+Rcareening violently &N&+rinto &+R$N&n&+r!&n", FALSE, vict, 0, victtwo, TO_NOTVICTROOM);
  damage(victtwo, vict, (GET_WEIGHT(victtwo) / 2), TYPE_UNDEFINED);
  Stun(vict, 2 * PULSE_VIOLENCE);
  Stun(victtwo, 2 * PULSE_VIOLENCE);
  stop_fighting(victtwo);
  stop_fighting(vict);
  } //  33% chance of proc

  return FALSE;
}

int remo_burn(P_char vict, P_char ch, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_GOTHIT)
    return FALSE;

  act("&+W$N&+W's tough hide &+rburns &+L$n &+Was $e gets too close!&n", TRUE, ch, 0, vict, TO_NOTVICTROOM | ACT_NOTTERSE );
  act("&+W$N&+W's tough hide &N&+rburns &+Lyou &+Was you get too close!&n", TRUE, ch, 0, vict, TO_CHAR | ACT_NOTTERSE);

  damage(ch, vict, (20 + dice(20, 4)), SPLDAM_NEGATIVE);

  // Let's let the rest of pv_common() continue...
  return FALSE;
}

int illithid_whip(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_char   vict = NULL;
  struct proc_data *data = NULL;
  int      dam = cmd / 1000;
 
  if ( cmd == CMD_SET_PERIODIC )
  {
    return FALSE;
  }
  if ( !obj || !ch )
  {
    return FALSE;
  }

  if( (cmd == CMD_MELEE_HIT) && !number(0, 30))
  {
    vict = (P_char) arg;

    if (obj->loc.wearing->equipment[WIELD] != obj)
      return FALSE;

    if(!dam)
      return FALSE;

    if(!vict || !IS_HUMANOID(vict) || !IS_ALIVE(vict) || GET_POS(vict) < POS_STANDING)
      return FALSE;

      act("&+mYour $p &+mwraps about $N&+m's legs, tripping $M up!\n&N", FALSE, ch, obj, vict, TO_CHAR);
      act("&+m$n&+m's $p &+mwraps about $N&+m's legs, tripping $M up!\n&N", FALSE, ch, obj, vict, TO_ROOM);
      act("&+m$n&+m's $p wraps about your legs, tripping you up!\n&N", FALSE, ch, obj, vict, TO_VICT);
      Stun(vict, PULSE_VIOLENCE * 1);
      CharWait(vict, PULSE_VIOLENCE * 1);
      SET_POS(vict, POS_PRONE + GET_STAT(vict));
  }
  return FALSE;
}

int skull_leggings(P_obj leggings, P_char ch, int cmd, char *arg)
{

   if (cmd == CMD_SET_PERIODIC)
     return TRUE;

   if(cmd == CMD_PERIODIC)
   {
     if (OBJ_CARRIED(leggings))
     {
        ch = leggings->loc.carrying;
     }
     if (OBJ_CARRIED_BY(leggings, ch))
     {
       if (GET_RACE(ch) == RACE_SNOW_OGRE || GET_RACE(ch) == RACE_OGRE  || \
           GET_RACE(ch) == RACE_MINOTAUR || GET_RACE(ch) == RACE_FIRBOLG)
       {
          int slot = WEAR_LEGS;
          if (ch->equipment[slot])
            obj_to_char(unequip_char(ch, slot), ch);
          obj_from_char(leggings, TRUE);
          equip_char(ch, leggings, slot, FALSE);
          act("&+LA longing for &+rblood &+Lbegins to &+Rburn &+Lin your &+rheart&+L, and you feel a strange\n"
              "&+Lforce overcome you, compelling you to don the &+Wskull leggings&+L.  They quickly\n"
              "&+Lform to your shins, almost as if they were made for &+Ryou&+L.&n\n", FALSE, ch, 0, 0, TO_CHAR);
          act("$n &+Lstraps the &+Wskull leggings &+Lto $s legs and &+Rroars &+Lwith renewed &+rvigor&+L.&n\n", FALSE, ch, 0, 0, TO_ROOM);

          return TRUE;
        }
      }
    } 
  return FALSE;
}

int flesh_golem_repop(P_obj obj, P_char ch, int cmd, char *arg)
{
  int mob_vnum = 87734;

  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if( !obj || !OBJ_ROOM(obj) )
    return FALSE;

  if (cmd == CMD_PERIODIC)
  {
    int curr_time = time(NULL);

    if( curr_time < obj->timer[0] + 1 )
      return FALSE;

    int rnum = real_mobile(mob_vnum);
    
    if(mob_index[rnum].number < mob_index[rnum].limit)
    {
      P_char mob = read_mobile(rnum, REAL);
      char_to_room(mob, obj->loc.room, -1);
      obj->timer[0] = curr_time;
    }

    return FALSE;
  }

  return FALSE;
}

