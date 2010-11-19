// Venthix's procs

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <list>
#include <vector>
#include <math.h>
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
#include "map.h"
#include "damage.h"
#include "specs.venthix.h"

vector<ZombieGame*> zgames;

extern P_room world;
extern P_desc descriptor_list;
extern P_index mob_index;
extern struct zone_data *zone_table;

int roulette_pistol(P_obj obj, P_char ch, int cmd, char *arg)
{
  int position; //The local of the live round in the pistol, 0 if none exist (fired)
  int dieval = 0;
  
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch)
    return FALSE;

  if (arg && ((cmd == CMD_RELOAD) || (cmd == CMD_FIRE)) && isname(arg, obj->name))
  {
    if (ch->equipment[HOLD] != obj)
    {
      act("You must be holding the pistol to use it.", FALSE, ch, obj, 0, TO_CHAR);
      return TRUE;
    }
  } 

  if (arg && (cmd == CMD_RELOAD) )
  {
    if (isname(arg, obj->name))
    {
    
      // Does the pistol still have a live round?
      if (obj->value[0])
      {
        act("&+yA live round is still loaded.  You spin the chamber and lock it.&n", FALSE, ch, obj, 0, TO_CHAR);
        //  Uhh how do I make $e show as uppercase?
        act("&+y$n opens the chamber and notices a live round already chambered.&L$e spins the chamber and locks it back.&n", FALSE, ch, obj, 0, TO_ROOM);
      }

      // If no live round is found in the pistol...
      if (!obj->value[0])
      {
        act("&+y$n quickly reloads $p &+ywith a live round.", FALSE, ch, obj, 0, TO_ROOM);
        act("&+yYou quickly reload $p &+ywith a live round.", FALSE, ch, obj, 0, TO_CHAR);
      }
      
      // Randomize the live round...  
      obj->value[0] = number(1,6);
  
      return TRUE;
    }
  }
  
  if (arg && (cmd == CMD_FIRE) )
  {
    if (isname(arg, obj->name))
    {
      if (!obj->value[0])
      {
        // Gun is empty, no live round.  Reload before playing...
        act("&+WClick!  &+yThere is no live round in the gun, reload you dummy!", FALSE, ch, obj, 0, TO_CHAR);
        act("&+WClick!  &+y$n&+y tries to play roulette with an empty gun.", FALSE, ch, obj, 0, TO_ROOM);
        return TRUE;
      }
      else
      {
        if ((obj->value[0] > 6) || (obj->value[0] < 0))
        {
          //debug issue with code. Value out of acceptable parameters.
          return TRUE;
        }
        if (obj->value[0] != 1)
        {
          // CLICK, nothing happened, wew!
          act("&+WClick! &+yNothing happened... &+Lweeew!&n", FALSE, ch, obj, 0, TO_CHAR);
          act("&+WClick! &+y$n&+y pulled the trigger, but nothing happened.", FALSE, ch, obj, 0, TO_ROOM);
        }
        else
        {
          // BANG, your dead!
          act("&-RBANG!&n &+WYou're dead!&n", FALSE, ch, obj, 0, TO_CHAR);
          act("&-RBANG!&n &+W$n&+W shot himself... What a loser!", FALSE, ch, obj, 0, TO_ROOM);
          if (!IS_TRUSTED(ch))
          {
            
            //obj_from_char(unequip_char(ch, HOLD), TRUE);
            //obj_to_room(obj, ch->in_room);
            //act("$p&+y drops to the floor.", FALSE, ch, obj, 0, TO_ROOM);
            GET_HIT(ch) = (-100);
          }
          else
          {
            act("&+yThe bullet simply bounces off $n&+y's head.", FALSE, ch, obj, 0, TO_ROOM);
            send_to_char("You can't die!\n", ch);
          }
        }
        obj->value[0]--;

        return TRUE;
      }
    }
  }
  return FALSE;
}

// 67282 &+mt&+Mh&+me &+Mo&+mr&+Mb &+mo&+Mf &+md&+Me&+mc&+Me&+mp&+Mt&+mi&+Mo&+mn
int orb_of_deception(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time = time(NULL);

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch)
    return FALSE;

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "mirage"))
    {
      if (((ch->equipment[WEAR_EARRING_L] == obj) ||
           (ch->equipment[WEAR_EARRING_R] == obj)) &&
          ((obj->timer[0] + 180) <= curr_time))
      {
        act("&+L$n&+L's $p &+Lbegins to vibrate.", FALSE, ch, obj, 0, TO_ROOM);
        act("&+LYour $p &+Lbegins to vibrate.", FALSE, ch, obj, 0, TO_CHAR);
        spell_mirage(51, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = time(NULL);
        return TRUE;
      }
    }
  }
  return FALSE;
}

#define CANNON_PLAYER 0
#define CANNON_AMMO   1
#define CANNON_DIR    2
#define CANNON_FIRING 3

#define COORD_X      0
#define COORD_Y      1

#define M_PI 3.14159265358979323846

struct cannon_data {
  int fuze;  // fuze for countdown
  int ammo;  // gunpowder 1-50
  int dir;   // dir to go
  double x;  // slope data x coordinates east and west
  double y;  // slope data y coordinates north and south
  int doxy[2];   // make the move north south east or west
  int moved; // how many rooms has player moved
};

void get_cannon_heading(void *data)
{
  struct cannon_data *cdata = (struct cannon_data*)data;
  
  if (!cdata)
  {
    debug("Error: get_cannon_heading called with null pointer");
    return;
  }
 
  float rad = (float)cdata->dir * M_PI / 180.000;
  cdata->x += (float) sin(rad);
  cdata->y += (float) cos(rad);

  cdata->doxy[COORD_X] = -1;
  cdata->doxy[COORD_Y] = -1;

  if ((cdata->y >= 51.000) || (cdata->x >= 51.000) || (cdata->y < 50.000) || (cdata->x < 50.000))
  {
    if (cdata->x > 50.999)
    {
      cdata->x -= 1.000;
      cdata->doxy[COORD_X] = EAST;
    }
    else if (cdata->x < 50.000)
    {
      cdata->x += 1.000;
      cdata->doxy[COORD_X] = WEST;
    }
    if (cdata->y > 50.999)
    {
      cdata->y -= 1.000;
      cdata->doxy[COORD_Y] = NORTH;
    }
    else if (cdata->y < 50.000)
    {
      cdata->y += 1.000;
      cdata->doxy[COORD_Y] = SOUTH;
    }
  }
  debug("dir: %d, doxy[x]: %d, doxy[y]: %d, x: %f, y: %f", cdata->dir, cdata->doxy[COORD_X], cdata->doxy[COORD_Y], cdata->x, cdata->y);
  return;
}

void event_super_cannon_fire(P_char ch, P_char vict, P_obj obj, void *data)
{
  struct cannon_data *cdata = (struct cannon_data*)data;
  int troom;
  int dir;

  if (!cdata)
  {
    debug("passed null pointer to event_super_cannon_fire");
    return;
  }

  if (!ch)
  {
    debug("event_super_cannon_fire called with !ch");
    return;
  }

  if (ch)
  {
    //Lets fly through the air!
    cdata->moved++;
    
    if (cdata->moved <= cdata->ammo/2)
      ch->specials.z_cord = MIN((int)get_property("cannon.super.max.z", 15), 
	  (int)((float)cdata->moved/((float)cdata->ammo/2)*(float)cdata->ammo/5));
    else
      ch->specials.z_cord = MIN((int)get_property("cannon.super.max.z", 15),
	  (int)(((float)cdata->ammo-(float)cdata->moved)/((float)cdata->ammo/2)*(float)cdata->ammo/5));

    // handle movement
    get_cannon_heading(cdata);

    for (int n = COORD_X; n <= COORD_Y; n++)
    {
      dir = -1;
      if (cdata->doxy[n] != -1 &&
	  (cdata->doxy[n] == NORTH ||
	   cdata->doxy[n] == EAST ||
	   cdata->doxy[n] == SOUTH ||
	   cdata->doxy[n] == WEST))
	dir = cdata->doxy[n];
      debug("dir: %d", dir); 
      if (dir < -1 || dir > 3)
      {
	debug("Error in event_super_cannon_fire: invalid dir");
	send_to_char("Ack! Something went wrong!\r\n", ch);
	ch->specials.z_cord = 0;
	return;
      }

      if (dir == -1)
        continue;

      if (!EXIT(ch, dir) ||
	  EXIT(ch, dir)->to_room == NOWHERE ||
	  !IS_SURFACE_MAP(EXIT(ch, dir)->to_room))
      {
        send_to_char("&+WSMACK! &+LYou ran into something, that hurt!\r\n", ch);
        ch->specials.z_cord = 0;
        spell_damage(ch, ch, 20, SPLDAM_GENERIC,
                SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
        SET_POS(ch, GET_STAT(ch) + POS_PRONE);
        return;
      }
      troom = EXIT(ch, dir)->to_room;
      //can we enter next room or did we hit something bad?
      if (IS_FIGHTING(ch))
        stop_fighting(ch);
    
      if (ch->in_room != NOWHERE)
        for (P_char attacker = world[ch->in_room].people; attacker; attacker = attacker->next)
	  if (IS_FIGHTING(attacker) && (attacker->specials.fighting == ch))
	    stop_fighting(attacker);

      if (IS_PC(ch) && IS_RIDING(ch))
        stop_riding(ch);
      if (P_char rider = GET_RIDER(ch))
        stop_riding(rider);
    
      char_from_room(ch);
      char_to_room(ch, troom, -1);
    }
    
    if (cdata->moved >= cdata->ammo)
    {
      //landed!
      ch->specials.z_cord = 0;
      if (world[ch->in_room].sector_type == SECT_WATER_SWIM ||
          world[ch->in_room].sector_type == SECT_WATER_NOSWIM ||
          world[ch->in_room].sector_type == SECT_OCEAN)
      {
        send_to_char("&+LYou &+Bsplash &+Linto the &+Bwater &+Las you land!\r\n", ch);
      }
      else
      {	  
        send_to_char("&+LYou've landed!  You roll as you hit the ground.&n\r\n", ch);
        if (!affected_by_spell(ch, SPELL_FLY) &&
          GET_CHAR_SKILL(ch, SKILL_SAFE_FALL) >= number(0, 100))
        {
          send_to_char("&+WOuch! &+LYou should try a fly spell next time.&n\r\n", ch);
          spell_damage(ch, ch, 20, SPLDAM_GENERIC,
              SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
        }
        SET_POS(ch, GET_STAT(ch) + POS_PRONE);
      }
      return;
    }

    add_event(event_super_cannon_fire, 1, ch, 0, 0, 0, data, sizeof(struct cannon_data));
  }
}

void event_super_cannon(P_char pl, P_char vict, P_obj obj, void *data)
{
  struct cannon_data *cdata = (struct cannon_data*)data;
  P_char ch;
  double slope, x, y;
  int dir, offsetaim;

  if (!cdata)
  {
    debug("Passed null pointer to event_super_cannon");
    obj->value[CANNON_PLAYER] = 0;
    return;
  }

  // If our projectile still in the room?
  if (obj->loc.room > 0)
  {
    for (P_char tch = world[obj->loc.room].people; tch; tch = tch->next_in_room )
    {
      if (IS_PC(tch) && GET_PID(tch) == obj->value[CANNON_PLAYER])
      {
        ch = tch;
        break;
      }
    }
    if (!ch || !IS_ALIVE(ch))
    {
      obj->value[CANNON_PLAYER] = 0;
      return;
    }
  }

  cdata->fuze--;

  if (cdata->fuze == 2)
  {
    //perform lighting of fuze
    act("&+LA &+Rgnomish &+Ct&+Bi&+Mn&+Rk&+Ge&+Yr &+Lruns in from the s&+bh&+La&+bd&+Lo&+bw&+Ls and &+rl&+Ri&+rgh&+Rt&+rs &+Lthe fuze before everyone is ready!", FALSE, ch, obj, 0, TO_CHAR);
    act("&+LA &+Rgnomish &+Ct&+Bi&+Mn&+Rk&+Ge&+Yr &+Lruns in from the s&+bh&+La&+bd&+Lo&+bw&+Ls and &+rl&+Ri&+rgh&+Rt&+rs &+Lthe fuze before everyone is ready!", FALSE, ch, obj, 0, TO_ROOM);
    add_event(event_super_cannon, WAIT_SEC, 0, 0, obj, 0, data, sizeof(struct cannon_data));
    return;
  }
  else if (cdata->fuze == 1)
  {
    //perform fuze burning msg
    act("&+LThe fuze &+Rsi&+rzz&+Rl&+re&+Rs &+Lquickly...&n", FALSE, ch, obj, 0, TO_CHAR);
    act("&+LThe fuze &+Rsi&+rzz&+Rl&+re&+Rs &+Lquickly...&n", FALSE, ch, obj, 0, TO_ROOM);
    add_event(event_super_cannon, WAIT_SEC, 0, 0, obj, 0, data, sizeof(struct cannon_data));
    return;
  }
  else if (cdata->fuze == 0)
  {
    //perform firing event on player
    act("&-RBOOOOOOOOOOOOOOOOM!", FALSE, ch, obj, 0, TO_CHAR);
    act("&-RBOOOOOOOOOOOOOOOOM!", FALSE, ch, obj, 0, TO_ROOM);
    act("$n has a shocked look on his face as he flies out of $p.", FALSE, ch, obj, 0, TO_ROOM);
    obj->value[CANNON_PLAYER] = 0;
    obj->value[CANNON_FIRING] = FALSE; 
    cdata->x = 50.500;
    cdata->y = 50.500;
    cdata->doxy[COORD_X] = -1;
    cdata->doxy[COORD_Y] = -1;
    cdata->moved = 0; 
    offsetaim = cdata->ammo / 50;
    cdata->dir += number(-offsetaim, offsetaim);  // So the aim isn't perfect
    add_event(event_super_cannon_fire, 0, ch, 0, 0, 0, data, sizeof(struct cannon_data));
    return;
  }
}

int super_cannon(P_obj obj, P_char ch, int cmd, char *arg)
{
  char load[MAX_STRING_LENGTH], dirstr[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int i, found = 0;
  
  struct cannon_data cdata;
  cdata.fuze = 3;

  if (cmd == CMD_SET_PERIODIC)
  {
    // Make sure the cannon is reset on object creation
    for (i = 0; i < 6; i++)
    {
      obj->value[i] = 0;
    }
    return FALSE;
  }

  if (cmd == CMD_PERIODIC)
    return FALSE;

  if (!obj || !ch)
    return FALSE;

  if (!IS_PC(ch))
    return FALSE;

  if (arg && (cmd == CMD_ORDER))
  {
    half_chop(arg, arg1, arg2);
    if (!isname(arg1, "cannon"))
      return FALSE;
  
    // If player isn't in room, reset the cannon for another's use.
    for (P_char tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if (IS_PC(tch) && GET_PID(tch) == obj->value[CANNON_PLAYER])
        found = TRUE;
    }
    if (!found)
    {
      obj->value[CANNON_PLAYER] = 0;
      obj->value[CANNON_FIRING] = FALSE;
    }
    if (obj->value[CANNON_PLAYER] != GET_PID(ch) &&
	obj->value[CANNON_PLAYER] != 0)
    {
      act("$p is already in use!", FALSE, ch, obj, 0, TO_CHAR);
      return TRUE;
    }

    argument_interpreter(arg2, load, dirstr);
    
    if (!*load || !*dirstr || !isdigit(*load) || !isdigit(*dirstr))
    {
      act("Please specify the load and direction.  Example: Order Cannon 10 45", FALSE, ch, 0, 0, TO_CHAR);
      return TRUE;
    }
    if (atoi(load) <= 0)
    {
      act("Zero ammo wont get you very far.", FALSE, ch, 0, 0, TO_CHAR);
      return TRUE;
    }
    if (atoi(load) > (int)get_property("cannon.super.range", 500))
    {
      sprintf(buf, "Are you trying to blow up the cannon? That's too much gunpowder! Try %d or less.", (int)get_property("cannon.super.range", 500));
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
      return TRUE;
    }
    
    obj->value[CANNON_AMMO] = atoi(load);
    obj->value[CANNON_DIR] = atoi(dirstr);
    obj->value[CANNON_PLAYER] = GET_PID(ch);
    sprintf(buf, "&+L$n &+Lloads &+L$p &+Lwith %d pounds of gunpowder and tweaks it's aim.  Grinning, $e hops in.", atoi(load));
    act(buf, FALSE, ch, obj, 0, TO_ROOM);
    sprintf(buf, "&+LYou load $p &+Lwith %d pounds of gunpowder and aim it.", atoi(load));
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return TRUE;
  }

  if (arg && (cmd == CMD_SHOUT) &&
      obj->value[CANNON_FIRING] != TRUE)
  {

    if (isname(arg, "fire"))
    {
      if (obj->value[CANNON_PLAYER] == 0)
      {
        act("$p isn't primed for firing yet.", FALSE, ch, obj, 0, TO_CHAR);
        return TRUE;
      }
    
      if (obj->value[CANNON_PLAYER] != GET_PID(ch))
      {
        act("$p is already in use!", FALSE, ch, obj, 0, TO_CHAR);
        return TRUE;
      }

      if (!IS_SURFACE_MAP(ch->in_room))
      {
	act("Sorry, this cannon only works on the surface.", FALSE, ch, 0, 0, TO_CHAR);
	obj->value[CANNON_PLAYER] = 0;
	return TRUE;
      }
      obj->value[CANNON_FIRING] = TRUE;
      cdata.ammo = obj->value[CANNON_AMMO];
      cdata.dir = obj->value[CANNON_DIR];
      add_event(event_super_cannon, WAIT_SEC, 0, 0, obj, 0, &cdata, sizeof(cdata));
      return FALSE;
    }
  }
  return FALSE;
}

void halloween_mine_proc(P_char ch)
{
  char buff[MAX_STRING_LENGTH];
  sprintf(buff, " %s 86", GET_NAME(ch));
  act("Your dig hits a burried &+ypumpkin&n.&LSuddenly it begins to move and digs itself out of the mine!", TRUE, ch, 0, 0, TO_CHAR);
  act("$n dig hits a burried &+ypumpkin&n.&LSuddenly it begins to move and digs itself out of the mine!", TRUE, ch, 0, 0, TO_ROOM);
  do_givepet(ch, buff, CMD_GIVEPET);
}

#define ZOMBIES_STATUS  0 // game 0=off, 1=on, 2=standby
#define ZOMBIES_LEVEL   1 // current game level
#define ZOMBIES_ID      2 // ZombiesGame Class id

// Zombies game: loads zombies per round until all players are dead. :)
// Need to setup a zombies class to load zombie vectors per spawner item.

int zombies_game(P_obj obj, P_char ch, int cmd, char *arg)
{
  P_desc i;
  char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  char arg3[MAX_STRING_LENGTH], arg4[MAX_STRING_LENGTH];
  char buff[MAX_STRING_LENGTH], buff2[MAX_STRING_LENGTH];
  int level = 0;
  int zone = 0;
  int num = 0;
  int palive = 0;
  int max_level = (int)get_property("zombies.game.maxlevel", 99);
  
  if (!obj)
    return FALSE;

  int zombies = zg_count_zombies(obj);
	
  if (cmd == CMD_SET_PERIODIC)
  {
    //load object with game status set to off
    obj->value[ZOMBIES_STATUS] = FALSE;
    //load the zombies game class object with mob vector
    ZombieGame* zgame = new ZombieGame(obj);
    zgames.push_back(zgame);
    obj->value[ZOMBIES_ID] = zgame->id;
    mob_index[real_mobile0(87)].func.mob = zgame_mob_proc;
    return TRUE;
  }

  ZombieGame* zgame = get_zgame_from_obj(obj);
  
  if (OBJ_ROOM(obj))
  {
    zone = world[obj->loc.room].zone;
  }
  else
  {
    debug("Zombies Game generator obj not placed in room (perhaps in someones inventory?");
    return FALSE;
  }

  // how many players left in the zone.
  for (i = descriptor_list; i; i = i->next)
  {
    if (!i->connected && !IS_TRUSTED(i->character) &&
        (GET_ZONE(i->character) == zone))
      palive++;
  }
  
  if (cmd == CMD_PERIODIC)
  {
    //if game is off, don't load zombies
    if (obj->value[ZOMBIES_STATUS] == 0)
      return TRUE;

    //zombie count returns -1, somethings wrong, remove the object
    if (zombies < 0)
    {
      debug("Error with zg_count_zombies(): can't find zombie game data");
      send_to_zone(zone, "Game has ended due to technical difficulties, please notify a god.\r\n");
      extract_obj(obj, TRUE);
      return TRUE;
    }
    
    // If game is active
    if (obj->value[ZOMBIES_STATUS] == 1)
    {
      //zombies are all dead, set to standby, and prepare for next round
      if (zombies == 0 &&
	  zgame->zombies_to_load <= 0)
      {
        if (obj->value[ZOMBIES_LEVEL] >= max_level)
        {
	  //reached end of max level end the game
	  obj->value[ZOMBIES_LEVEL] = 0;
	  obj->value[ZOMBIES_STATUS] = 0;
	  send_to_zone(zone, "You've completed the last level, you're game is complete.  Congrats!\r\n");
	  return TRUE;
        }
        else
	{
	  //set game to standby mode to setup the next round
          obj->value[ZOMBIES_STATUS] = 2;
          obj->timer[0] = time(NULL);
	  sprintf(buff, "You've completed round %d.  Prepare for the next round.\r\n", obj->value[ZOMBIES_LEVEL]);
          send_to_zone(zone, buff);
	  return TRUE;
	}
      }
      else if (!palive)
      {
	sprintf(buff, "All players are dead, the game has ended on level %d.\r\n", obj->value[ZOMBIES_LEVEL]);
        send_to_zone(zone, buff);
	obj->value[ZOMBIES_STATUS] = 0;
	obj->timer[0] = 0;
        zgame_clear_zombies(obj);
	return TRUE;
      }
      else
      {
        //load zombies here
	//need to make this more complex
	//can have different types of mobs load at higher levels eventually
	num = number(0, 2);
	num = BOUNDED(0, num, zgame->zombies_to_load);
	debug("loading %d zombies", num);
	for (int i = 0; i < num; i++)
	{
          if (zgame_load_zombie(obj))
	    zgame->zombies_to_load--;
	}
	return TRUE;
      }
    }
    // if 2 minutes have passed and we are in standby mode begin next round
    if (obj->value[ZOMBIES_STATUS] == 2 &&
	(obj->timer[0] + (60 * 2)) <= time(NULL))
    {
      obj->timer[0] = 0;
      obj->value[ZOMBIES_STATUS] = 1;
      obj->value[ZOMBIES_LEVEL]++;
      // need to scale the amount of mobs per level here
      zgame->zombies_to_load = obj->value[ZOMBIES_LEVEL];
      sprintf(buff, "Round %d is beginning.  Good Luck!\r\n", obj->value[ZOMBIES_LEVEL]);
      send_to_zone(zone, buff);
      return TRUE;
    }

    return TRUE;
  }
  
  if (!ch)
    return FALSE;

  if (!IS_PC(ch))
    return FALSE;

  if (cmd == CMD_HELP && arg && strstr(arg, "zombies") && IS_TRUSTED(ch))
  {
    half_chop(arg, arg1, arg2);
    if (arg2 && isname(arg2, "begin"))
    {
      //set game to standby mode to setup the next round
      obj->value[ZOMBIES_STATUS] = 2;
      obj->timer[0] = time(NULL) - (110); // should be 10 seconds to start
      send_to_zone(zone, "The game is beginning, good luck.\r\n");
      return TRUE;
    }
    else if (arg2 && isname(arg2, "end"))
    {
      obj->value[ZOMBIES_STATUS] = 0;
      obj->timer[0] = 0;
      sprintf(buff, "The game has been stopped by %s.\r\n", GET_NAME(ch));
      send_to_zone(zone, buff);
      zgame_clear_zombies(obj);
      return TRUE;
    }
    else if (arg2 && isname(arg2, "status"))
    {
      sprintf(buff, "&+WZombies Game Status&n\r\n");
      sprintf(buff2, "&+L           Status&+W: &+c%s&n\r\n", (obj->value[ZOMBIES_STATUS] == 1 ? "On" : obj->value[ZOMBIES_STATUS] == 0 ? "Off" : "Standby"));
      strcat(buff, buff2);
      sprintf(buff2, "&+L            Round&+W: &+c%d&n\r\n", obj->value[ZOMBIES_LEVEL]);
      strcat(buff, buff2);
      sprintf(buff2, "&+L    Players Alive&+W: &+c%d&n\r\n", palive);
      strcat(buff, buff2);
      sprintf(buff2, "&+L    Zombies Alive&+W: &+c%d&n\r\n", zombies);
      strcat(buff, buff2);
      sprintf(buff2, "&+LZombies Remaining&+W: &+c%d&n\r\n", zgame->zombies_to_load);
      strcat(buff, buff2);
      send_to_char(buff, ch);
      return TRUE;
    }
    else if (arg2 && strstr(arg2, "level"))
    {
      argument_interpreter(arg2, arg3, arg4);
      if (isdigit(*arg4))
	level = atoi(arg4);
      else
      {
	send_to_char("Please specify a number for the level.\r\n", ch);
	return TRUE;
      }
      if (level < 0 || level > max_level)
      {
	send_to_char("Sorry that level is too high.\r\n", ch);
	return TRUE;
      }
      obj->value[ZOMBIES_LEVEL] = level;
      sprintf(buff, "&+WZombies Game level set to: &+c%d&n.\r\n", level);
      send_to_char(buff, ch);
      return TRUE;
    }
    else
    {
      send_to_char("&+WZombies Game Help&n\r\n", ch);
      send_to_char("&+LSyntax: &+wzombies &+W[&+cargument&+W]&n\r\n", ch);
      send_to_char("&+cArguments:&n\r\n", ch);
      send_to_char("&+W* &+c begin&+W: &nBegin the zombies game.\r\n", ch);
      send_to_char("&+W* &+c   end&+W: &nEnd the game.\r\n", ch);
      send_to_char("&+W* &+c level&+W: &nSet game level.\r\n", ch);
      send_to_char("&+W* &+cstatus&+W: &nView game status.\r\n", ch);
      send_to_char("&+W* &+c  help&+W: &nThis help listing.\r\n", ch);
      send_to_char("&n\r\n", ch);
      return TRUE;
    }
    return TRUE;
  }
  return FALSE;
}

int zgame_load_zombie(P_obj obj)
{
  P_char z;
  P_desc i;

  if (!obj)
    return 0;

  if (!OBJ_ROOM(obj))
    return 0;

  int zone = world[obj->loc.room].zone;

  ZombieGame* zgame = get_zgame_from_obj(obj);

  if (!zgame)
    return 0;

  z = read_mobile(87, VIRTUAL);

  if (!z)
    return 0;

  // Can randomize here where we want the zombie to load within the zone
  GET_BIRTHPLACE(z) = obj->loc.room;

  char_to_room(z, obj->loc.room, 0);
  CLEAR_MONEY(z);

  // Set memory for hunting
  for (i = descriptor_list; i; i = i->next)
  {
    if (!i->connected && !IS_TRUSTED(i->character) &&
        (GET_ZONE(i->character) == zone))
      remember(z, i->character);
  }

  zgame->zombies.push_back(z);
    
  return 1;
}

void zgame_clear_zombies(P_obj obj)
{
  if (!obj)
    return;

  ZombieGame* zgame = get_zgame_from_obj(obj);

  if (!zgame)
    return;

  while (zgame->zombies.size())
  {
    P_char z = zgame->zombies.back();
    zgame->zombies.pop_back();

    if (!z)
      continue;

    extract_char(z);
  }

    return;
}

// finds zombies game class object from obj and counts alive
// zombies in the mob vector.
int zg_count_zombies(P_obj obj)
{
  if (!obj)
    return -1;

  ZombieGame* zgame = get_zgame_from_obj(obj);
  
  if (!zgame)
    return -1;
  
  return (int)zgame->zombies_alive();
}

ZombieGame* get_zgame_from_obj(P_obj obj)
{
  for (int i = 0; i < zgames.size(); i++)
  {
    if (zgames[i] && zgames[i]->id == obj->value[ZOMBIES_ID])
      return zgames[i];
  }
  return NULL;
}

ZombieGame* get_zgame_from_zombie(P_char z)
{
  if (!z)
    return NULL;

  for (int i = 0; i < zgames.size(); i++)
  {
    for (int j = 0;  j < zgames[i]->zombies.size(); j++)
    {
      if (zgames[i]->zombies[j] == z)
	return zgames[i];
    }
  }

  return NULL;
}

int zgame_mob_proc(P_char ch, P_char pl, int cmd, char *arg)
{
  if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC)
    return FALSE;

  SET_BIT(ch->specials.act, ACT_SPEC_DIE);

  if (cmd == CMD_DEATH)
  {
    ZombieGame* zgame = get_zgame_from_zombie(ch);
    for (int i = 0; i < zgame->zombies.size(); i++)
    {
      if (zgame->zombies[i] == ch)
	// This ends up moving all the iterators past this point
	// so if this becomes too inefficient when we have too many
	// mobs (ie high level), we might have to rework how we
	// are keeping track of mobs.
	zgame->zombies.erase(zgame->zombies.begin()+i);
    }
    return FALSE;
  }

  return FALSE;
}

int ZombieGame::next_id = 1;

ZombieGame::ZombieGame() : generator(NULL) {}

ZombieGame::ZombieGame(P_obj _generator) : generator(_generator)
{
  load();
}

ZombieGame::~ZombieGame()
{
  unload();
}

int ZombieGame::load()
{
  id = ZombieGame::next_id++; 
  return TRUE;
}

int ZombieGame::unload()
{
  while (zombies.size())
  {
    P_char zombie = zombies.back();
    zombies.pop_back();

    if (!zombie)
      continue;

    extract_char(zombie);
  }
  return TRUE;
}
