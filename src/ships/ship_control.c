#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comm.h"
#include "db.h"
#include "interp.h"
#include "structs.h"
#include "prototypes.h"
#include "utils.h"
#include "events.h"
#include "map.h"
#include "limits.h"
#include "ships.h"
#include "ship_auto.h"
#include "ship_npc.h"
#include "ship_npc_ai.h"
#include "ctf.h"

extern char buf[MAX_STRING_LENGTH];
extern char arg1[MAX_STRING_LENGTH];
extern char arg2[MAX_STRING_LENGTH];
extern char arg3[MAX_STRING_LENGTH];
extern char tmp_str[MAX_STRING_LENGTH];



int order_sail(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    if (SHIP_ANCHORED(ship)) 
    {
        send_to_char("You are anchored, de-anchor first.\r\n", ch);
        return TRUE;
    }
    if (!is_valid_sailing_location(ship, ship->location)) 
    {
        send_to_char ("You cannot autopilot until you are out in the open seas!\r\n", ch);
        return TRUE;
    }
    return engage_autopilot(ch, ship, arg1, arg2);
}

int jettison_cargo(P_char ch, P_ship ship, char* arg)
{
    int crates = INT_MAX;
    if (is_number(arg))
        crates = atoi(arg);
    return jettison_cargo(ch, ship, crates);
}

int jettison_contraband(P_char ch, P_ship ship, char* arg)
{
    int crates = INT_MAX;
    if (is_number(arg))
        crates = atoi(arg);
    return jettison_contraband(ch, ship, crates);
}

int salvage_cargo(P_char ch, P_ship ship, char* arg)
{
    if (ship->speed > 0)
    {
        send_to_char("You have to stop your ship to salvage!\r\n", ch);
        return TRUE;
    }

    int crates = INT_MAX;
    if (is_number(arg))
        crates = atoi(arg);

    return salvage_cargo(ch, ship, crates);
}

int order_undock(P_char ch, P_ship ship)
{        
    if (!SHIP_DOCKED(ship) && !SHIP_ANCHORED(ship)) 
    {
        send_to_char("The ship is not docked or anchored!\r\n", ch);
        return TRUE;
    }
    
    if (ship->timer[T_UNDOCK] != 0)
    {
        send_to_char("The crew is already working on it!\r\n", ch);
        return TRUE;
    }

    if (SHIP_DOCKED(ship))
    {
        if (ship->mainsail == 0)
        {
            send_to_char ("You cannot unfurl your sails, they are destroyed.\r\n", ch);
            return TRUE;
        }
        if (SHIP_IMMOBILE(ship))
        {
            send_to_char("&+RYour ship is immobilized. Undocking procedures cancelled.\r\n", ch);
            return TRUE;
        }
        if (!check_undocking_conditions(ship, ship->m_class, ch))
            return TRUE;
    }

    send_to_char("Your crew begins undocking procedures.\r\n", ch);
    if (!IS_NPC_SHIP(ship))
    {
      if( RACE_GOOD(ch) )
        ship->race = GOODIESHIP;
      else if( RACE_EVIL(ch) )
        ship->race = EVILSHIP;
      else if( RACE_PUNDEAD(ch) )
        ship->race = UNDEADSHIP;
      else if( RACE_NEUTRAL(ch) )
        ship->race = SQUIDSHIP;
      else
        ship->race = UNKNOWNSHIP;
    }
    else
      ship->race = NPCSHIP;

    if (IS_TRUSTED(ch))
        ship->timer[T_UNDOCK] = 2;
    else
        ship->timer[T_UNDOCK] = 30;
    ship->time = time(NULL);
    update_ship_status(ship);
    return TRUE;
}

int order_maneuver(P_char ch, P_ship ship, char* arg)
{
    if (SHIP_DOCKED(ship)) 
    {
        send_to_char("Might want to undock first.\r\n", ch);
        return TRUE;
    }
    if (SHIP_FLYING(ship)) 
    {
        send_to_char("Land your ship before maneuvering it.\r\n", ch);
        return TRUE;
    }
    if (SHIP_IMMOBILE(ship)) 
    {
        send_to_char("You're immobile, you can't maneuver!\r\n", ch);
        return TRUE;
    }
    if (ship->speed > 20) 
    {
        send_to_char("You're coming in too fast!!\r\n", ch);
        return TRUE;
    }
    if (ship->timer[T_MANEUVER] > 0) 
    {
        send_to_char("Your ship isn't ready to maneuver again yet.\r\n", ch);
        return TRUE;
    }

    int dir;
    if (isname(arg, "n") || isname(arg, "north"))
        dir = NORTH;
    else if (isname(arg, "e") || isname(arg, "east")) 
        dir = EAST;
    else if (isname(arg, "s") || isname(arg, "south")) 
        dir = SOUTH;
    else if (isname(arg, "w") || isname(arg, "west"))
        dir = WEST;
    else 
    {
        send_to_char_f(ch, "%s is not a valid direction try (north, east, south, west)\r\n", arg);
        return TRUE;
    }
    if (!world[ship->location].dir_option[dir]) 
    {
        send_to_char("Sorry the ship will not go there.\r\n", ch);
        return TRUE;
    }
    int dir_room = world[ship->location].dir_option[dir]->to_room;
    if (dir_room == NOWHERE) 
    {
        send_to_char("Sorry the ship will not go there.\r\n", ch);
        return TRUE;
    }

    if ((IS_SET(world [dir_room].room_flags, DOCKABLE) || world[dir_room].number < 110000) && ship->timer[T_BSTATION] > 0)
    {
        if (ship->target != NULL) 
            send_to_char ("Your crew is on alert status while you have a target locked!\r\n", ch);
        else
            send_to_char_f(ch, "Your crew is still on alert status and will take %d seconds longer to stand down.\r\n", ship->timer[T_BSTATION]);
        return TRUE;
    }

    if (IS_WATER_ROOM(dir_room) || IS_SET(world [dir_room].room_flags, DOCKABLE)) 
    {
        if (ship->autopilot) 
            stop_autopilot(ship);


        if ((world[ship->location].sector_type != SECT_OCEAN) || 
            (world[ship->location].number < 110000) ||
            (world[dir_room].sector_type != SECT_OCEAN) || 
            (world[dir_room].number < 110000) ||
            IS_SET(world[dir_room].room_flags, DOCKABLE)) 
        //if ((world[dir_room].number < 110000) || (world[ship->location].number < 110000) ||
        {
            ship->speed = 0;
            ship->setspeed = 0;
            send_to_room_f(ship->location, "%s &Nmaneuvers to the %s.\r\n", ship->name, dirs[dir]);
            ship->location = world[ship->location].dir_option[dir]->to_room;
            obj_from_room(SHIP_OBJ(ship));
            obj_to_room(SHIP_OBJ(ship), ship->location);
            send_to_room_f(ship->location, "%s &Nmaneuvers in from the %s.\r\n", ship->name, dirs[rev_dir[dir]]);
            act_to_all_in_ship_f(ship, "Your ship manuevers to the %s.", dirs[dir]);
            if (!IS_TRUSTED(ch))
                ship->timer[T_MANEUVER] = 5;
            everyone_look_out_ship(ship);
            if (IS_SET(world[ship->location].room_flags, DOCKABLE))
            {
                dock_ship(ship, ship->location);
                check_contraband(ship, ship->location);
            }
            return TRUE;
        } 
        else 
        {
            send_to_char("You cannot maneuver out in the open seas!\r\n", ch);
            return TRUE;
        }
    } 
    else 
    {
        send_to_char("We'll crash into land if we do that!\r\n", ch);
        return TRUE;
    }
}

int order_anchor(P_char ch, P_ship ship)
{
    if (SHIP_SINKING(ship)) 
    {
        send_to_char ("Anchor while sinking?! Your ship IS the anchor now!\r\n", ch);
        return TRUE;
    }

    if (SHIP_FLYING(ship)) 
    {
        send_to_char ("You have to land your ship to anchor!\r\n", ch);
        return TRUE;
    }

    if (SHIP_DOCKED(ship)) 
    {
        send_to_char("You are docked, undock first!\r\n", ch);
        return TRUE;
    }
  /*  if (ship->speed != 0) 
    {
        send_to_char("You need to stop to anchor!\r\n", ch);
        return TRUE;
    }*/

    if (ship->autopilot) 
        stop_autopilot(ship);

    act_to_all_in_ship(ship, "&+yYour ship drops anchor here and your crew begins repairs.&N\r\n");
    SET_BIT(ship->flags, ANCHOR);
    ship->speed = 0;
    ship->setspeed = 0;
    return TRUE;
}

int order_ram(P_char ch, P_ship ship, char* arg)
{
   
    if (SHIP_SINKING(ship)) 
    {
        send_to_char("Ram while sinking, yeah right!\r\n", ch);
        return TRUE;
    }

    if (SHIP_DOCKED(ship)) 
    {
        send_to_char("Ram while docked, yeah right!\r\n", ch);
        return TRUE;
    }
    if (!*arg) 
    {
        if (ship->timer[T_RAM] != 0) 
        {
            send_to_char("&+WYou aren't ready to ram again!\r\n", ch);
            return TRUE;
        }
        if (ship->target == NULL) 
        {
            send_to_char("No target to ram.\r\n", ch);
            return TRUE;
        }

        if (ship->speed < 20)
        {
            send_to_char("&+WYou are too slow to ram!\r\n", ch);
            return TRUE;
        }

        if (!IS_SET(ship->flags, RAMMING))
        {
            SET_BIT(ship->flags, RAMMING);
            act_to_all_in_ship(ship, "&+WYou crew braces for impact!&N");
        } 
        else 
        {
            send_to_char("Ship is already in ramming mode!\r\n", ch);
        }
        return TRUE;
    }

    if (isname(arg, "off")) 
    {
        if (IS_SET(ship->flags, RAMMING)) 
        {
            act_to_all_in_ship(ship, "&+WYour crew has returned to their battle stations!");
            REMOVE_BIT(ship->flags, RAMMING);
            return TRUE;
        } 
        else 
        {
            send_to_char("Ship is currently not ramming anyone.\r\n", ch);
            return TRUE;
        }
    } 
    else 
    {
        send_to_char("Valid syntax: order ram [off]\r\n", ch);
        return TRUE;
    }
}

int order_heading(P_char ch, P_ship ship, char* arg)
{
    int      heading;
    if (!*arg) 
    {
        send_to_char_f(ch, "Current heading: &+W%d&N\r\nSet heading: &+W%d&N\r\n", (int)ship->heading, (int)ship->setheading);
        return TRUE;
    } 
    else 
    {
        if (!is_number(arg)) 
        {
            if (isname(arg, "e") || isname(arg, "east"))
                ship->setheading = 90;
            else if (isname(arg, "w") || isname(arg, "west"))
                ship->setheading = 270;
            else if (isname(arg, "s") || isname(arg, "south"))
                ship->setheading = 180;
            else if (isname(arg, "n") || isname(arg, "north"))
                ship->setheading = 0;
            else if (isname(arg, "ne northeast"))
                ship->setheading = 45;
            else if (isname(arg, "nw northwest"))
                ship->setheading = 315;
            else if (isname(arg, "se southeast"))
                ship->setheading = 135;
            else if (isname(arg, "sw southwest"))
                ship->setheading = 225;
            else if (isname(arg, "h heading"))
                ship->setheading = ship->heading;
            else 
            {
                send_to_char("Please enter a heading from 0-360 or N E S W NW NE SE SW.\r\n", ch);
                return TRUE;
            }
        }
    }
    if (is_number(arg)) 
    {
        heading = atoi(arg);
        if (heading >= 0 && heading <= 360) 
            ship->setheading = heading;
        else 
        {
            send_to_char("Please enter a heading from 0-360 or N E S W NW NE SE SW.\r\n", ch);
            return TRUE;
        }
    }
    act_to_all_in_ship_f(ship, "Heading set to &+W%d&N.", (int)ship->setheading);
    return TRUE;
}

int carrying_flag(P_ship ship)
{
  int i;
  P_char ch;
  P_obj obj;

  for (i = 0; i < ship->room_count; i++)
  {
    for (obj = world[real_room0(ship->room[i].roomnum)].contents; obj; obj = obj->next_content)
    {
      if (OBJ_VNUM(obj) == 790 ||
	  OBJ_VNUM(obj) == 791 ||
	  OBJ_VNUM(obj) == 792)
	return TRUE;
    }
    for (ch = world[real_room0(ship->room[i].roomnum)].people; ch; ch = ch->next_in_room)
    {
      if (ctf_carrying_flag(ch))
	return TRUE;
    }
  }
  return 0;
}

int order_speed(P_char ch, P_ship ship, char* arg)
{
    int      speed;
    int realspeed = ship->get_maxspeed();
    
    if(has_innate(ch, INNATE_SEADOG))
    realspeed += 2;

    if (!is_valid_sailing_location(ship, ship->location)) 
    {
        send_to_char ("You cannot unfurl the sails till you are out on open sea!\r\n", ch);
        return TRUE;
    }
    if (!*arg) 
    {
        send_to_char_f(ch, "Current speed: &+W%d&N\r\nSet speed: &+W%d&N\r\n",  ship->speed, ship->setspeed);
    } 
    else 
    {
        if (is_number(arg)) 
        {
            speed = atoi(arg);
            if (!SHIP_IMMOBILE(ship)) 
            {	     
             // if ((speed <= ship->get_maxspeed()) && speed >=0) 
             if ((speed <= realspeed) && speed >=0)
             {
                ship->setspeed = speed;
                act_to_all_in_ship_f(ship, "Speed set to &+W%d&N.", ship->setspeed);
             } 
            else 
             {
                //send_to_char_f(ch, "This ship can only go from &+W%d&N to &+W%d&N.\r\n", 0, ship->get_maxspeed());
	         send_to_char_f(ch, "This ship can only go from &+W%d&N to &+W%d&N.\r\n", 0, realspeed);
             }
            }
            else 
                send_to_char("&+RThe ship is immobile, it cannot move!\r\n", ch); 
       } 
        else if (isname(arg, "max maximum")) 
        {
            if (!SHIP_IMMOBILE(ship)) 
            {
               // ship->setspeed = ship->get_maxspeed();
		  ship->setspeed = realspeed;
                act_to_all_in_ship_f(ship, "Speed set to &+W%d&N.", ship->setspeed);
            } 
            else 
                send_to_char("&+RThe ship is immobile, it cannot move!\r\n", ch);
        } 
        else if (isname(arg, "med medium")) 
        {
            if (!SHIP_IMMOBILE(ship)) 
            {
              //  ship->setspeed = MAX(1, ship->get_maxspeed() * 2 / 3);
	 	  ship->setspeed = MAX(1, realspeed * 2 / 3);
                act_to_all_in_ship_f(ship, "Speed set to &+W%d&N.", ship->setspeed);
            } 
            else 
                send_to_char("&+RThe ship is immobile, it cannot move!\r\n", ch);
        } 
        else if (isname(arg, "min minimum slow")) 
        {
            if (!SHIP_IMMOBILE(ship)) 
            {
                //ship->setspeed = MAX(1, ship->get_maxspeed() / 3);
		  ship->setspeed = MAX(1, realspeed / 3);
                act_to_all_in_ship_f(ship, "Speed set to &+W%d&N.", ship->setspeed);
            } 
            else
                send_to_char("&+RThe ship is immobile, it cannot move!\r\n", ch);
        } 
        else 
        {
            send_to_char_f(ch, "Please enter a number value between %3d-%-d.\r\n", 0, ship->get_maxspeed());
        }
    }

#if (1)
    if (carrying_flag(ship))
    {
      ship->setspeed = ship->setspeed-(int)pow((double)(ship->setspeed/12), 2);
      act_to_all_in_ship_f(ship, "Speed reduced to &+W%d&N because it's carrying a CTF flag.", ship->setspeed);
    }
#endif

    return TRUE;
}

int order_signal(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    if (!*arg1) 
    {
        send_to_char("Syntax: signal <ship id> <message>\r\n", ch);
        return TRUE;
    }

    if (!*arg2) 
    {
        send_to_char("Send what message?\r\n", ch);
        return TRUE;
    }

    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to send signals!\r\n", ch); 
        return TRUE; 
    } 
    int k = getcontacts(ship);
    for (int i = 0; i < k; i++) 
    {
        P_ship target = contacts[i].ship;
        if (isname(arg1, target->id)) 
        {
            if (SHIP_DOCKED(target)) 
            {
                send_to_char ("You cannot send signals to docked ships!\r\n", ch);
                return TRUE;
            }
            if (contacts[i].range > 20.0)
            {
                send_to_char ("This ship is too far to see your signals!\r\n", ch);
                return TRUE;
            }
            if (target->race != ship->race)
            {
                send_to_char ("They wouldn't understand your signals!\r\n", ch);
                return TRUE;
            }

            send_to_char_f(ch, "&+GYou've sent a &+Ys&+Bi&+Wg&+yn&+Ma&+Cl &+Gmessage to &+W[%s]&N:%s&+G.&N\r\n", SHIP_ID(target), SHIP_NAME(target));
            act_to_all_in_ship_f(target, "&+GYour ship has recieved a &+Ys&+Bi&+Wg&+yn&+Ma&+Cl &+Gmessage from &+W[%s]&N:%s &+Gdecoded as \'&+Y%s&+G\'.&n", SHIP_ID(ship), SHIP_NAME(ship), arg2);
            return TRUE;
        }
    }
    send_to_char("This ship not in sight!\r\n", ch);
    return TRUE;
}

int order_fly(P_char ch, P_ship ship)
{
    int levi_slot = eq_levistone_slot(ship);
    if (!IS_SET(ship->flags, AIR) && levi_slot == -1)
    {
        send_to_char("Flying ship... Thats a nice dream, isn't it?\r\n", ch); 
        return TRUE; 
    }
    if(!IS_MAP_ROOM(ship->location)) 
    {
        send_to_char("Your ship must be outside to fly.\r\n", ch); 
        return TRUE; 
    }
    if (SHIP_FLYING(ship))
    {
        send_to_char("Your ship is already floating in air.\r\n", ch); 
        return TRUE; 
    }
    if (!IS_SET(ship->flags, AIR) && ship->slot[levi_slot].timer > 0 && !IS_TRUSTED(ch))
    {
        send_to_char("Your Levistone is still recharging.\r\n", ch); 
        return TRUE; 
    }
    fly_ship(ship);
    return TRUE;
}
int order_land(P_char ch, P_ship ship)
{
    if (!SHIP_FLYING(ship))
    {
        send_to_char("Your ship is not flying or anything.\r\n", ch); 
        return TRUE;
    }
    land_ship(ship);
    return TRUE;
}

int do_scan(P_char ch, P_ship ship, char* arg)
{
    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to scan!\r\n", ch); 
        return TRUE; 
    } 
 
    if (!*arg) 
    {
        if (ship->target != NULL) 
        {
            scan_target(ship, ship->target, ch);
            return TRUE;
        }
        send_to_char("No target locked. Syntax: Scan <target ID> or Scan\r\n", ch);
        return TRUE;
    }
    int k = getcontacts(ship);
    for (int i = 0; i < k; i++) 
    {
        if (isname(arg, contacts[i].ship->id)) 
        {
            scan_target(ship, contacts[i].ship, ch);
            return TRUE;
        }
    }
    send_to_char("Target not in sight!\r\n", ch);
    return TRUE;
}

int do_fire_weapon(P_ship ship, P_char ch, int w_num)
{
    if (w_num < 0 || w_num >= MAXSLOTS)
    {
        if (ch) send_to_char("Invalid weapon!\r\n", ch);
        return TRUE;
    }
    if (ship->slot[w_num].type != SLOT_WEAPON)
    {
        if (ch) send_to_char("Invalid weapon!\r\n", ch);
        return TRUE;
    }

    int j;
    int k = getcontacts(ship);
    for (j = 0; j < k; j++) 
    {
        if (contacts[j].ship == ship->target) 
            break;
    }
    if (j == k)
    {
        if (ch) send_to_char("Target out of sight!\r\n", ch);
        return TRUE;
    }

    int w_index = ship->slot[w_num].index;
    float range = contacts[j].range;
    float bearing = contacts[j].bearing;
    if (range > (float) weapon_data[w_index].max_range)
    {
        if (ch) send_to_char("Out of Range!\r\n", ch);
        return TRUE;
    }
    if (range < (float) weapon_data[w_index].min_range) 
    {
        if (ch) send_to_char("You're too close to use this weapon!\r\n", ch);
        return TRUE;
    }
    if (get_arc(ship->heading, bearing) != ship->slot[w_num].position) 
    {
        if (ch) send_to_char("Target is not in weapon's firing arc!\r\n", ch);
        return TRUE;
    }
    if (SHIP_WEAPON_DESTROYED(ship, w_num)) 
    {
        if (ch) send_to_char("Weapon is destroyed!\r\n", ch);
        return TRUE;
    }
    if (SHIP_WEAPON_DAMAGED(ship, w_num)) 
    {
        if (ch) send_to_char("Weapon is damaged!\r\n", ch);
        return TRUE;
    }
    if (ship->slot[w_num].timer > 0) 
    {
        if (ch) send_to_char("Weapon is still reloading.\r\n", ch);
        return TRUE;
    }
    if (ship->slot[w_num].val1 == 0) 
    {
        if (ch) send_to_char("Out of Ammo!\r\n", ch);
        return TRUE;
    }

    return fire_weapon(ship, w_num, j, ch);
}

int do_fire_arc(P_ship ship, P_char ch, int arc)
{
    int fired = 0;
    for (int i = 0; i < MAXSLOTS; i++)
    {
        if (ship->slot[i].type == SLOT_WEAPON && ship->slot[i].position == arc)
        {
            if (SHIP_ANCHORED(ship)) 
                return TRUE;
            do_fire_weapon(ship, ch, i);
            fired++;
        }
    }
    if (fired == 0)
        send_to_char("No weapons installed on this arc!\r\n", ch);
    return TRUE;
}

int do_fire (P_char ch, P_ship ship, char* arg)
{
    if (!isname(GET_NAME(ch), SHIP_OWNER(ship)) && !IS_TRUSTED(ch) &&
        (ch->group == NULL ? 1 : get_char2(str_dup(SHIP_OWNER(ship))) ==
         NULL ? 1 : (get_char2(str_dup(SHIP_OWNER(ship)))->group != ch->group))) 
    {
        send_to_char("You are not the captain of this ship, the crew ignores you.\r\n", ch);
        return TRUE;
    }

    if (SHIP_SINKING(ship)) 
    {
        send_to_char ("The ship is sinking! Your crew has already abandoned ship!\r\n", ch);
        return TRUE;
    }
    if (SHIP_DOCKED(ship) || SHIP_ANCHORED(ship)) 
    {
        send_to_char("Your crew isn't ready, undock first.\r\n", ch);
        return TRUE;
    }

    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to fire your weapons!\r\n", ch); 
        return TRUE; 
    } 
    if (!*arg) 
    {
        send_to_char("Valid syntax: 'fire <fore/starboard/port/rear/weapon number>'\r\n", ch);
        return TRUE;
    }
    arg = skip_spaces(arg);

    
    if (IS_TRUSTED(ch))
    { // manual npc loading
        half_chop(arg, arg1, arg2);
        if (isname(arg1, "pirate")) 
        {
            int lvl = 0;
            if (is_number(arg2)) lvl = atoi(arg2);
            if (try_load_npc_ship(ship, NPC_AI_PIRATE, lvl, ch))
                return true;
            else
            {
                send_to_char("Failed to load pirate ship!\r\n", ch);
                return true;
            }
        }
        if (isname(arg1, "hunter")) 
        {
            int lvl = 0;
            if (is_number(arg2)) lvl = atoi(arg2);
            if (try_load_npc_ship(ship, NPC_AI_HUNTER, lvl, ch))
                return true;
            else
            {
                send_to_char("Failed to load hunter ship!\r\n", ch);
                return true;
            }
        }
        if (isname(arg1, "escort")) 
        {
            int lvl = 0;
            if (is_number(arg2)) lvl = atoi(arg2);
            if (try_load_npc_ship(ship, NPC_AI_ESCORT, lvl, ch))
                return true;
            else
            {
                send_to_char("Failed to load escort ship!\r\n", ch);
                return true;
            }
        }
    }
    
    if (ship->target == NULL) 
    {
        send_to_char("No target locked.\r\n", ch);
        return TRUE;
    }
    if (ship->timer[T_MINDBLAST] > 0) 
    {
        send_to_char("&+RYour crew members crawl around and ignore your orders!&N\r\n", ch);
        return TRUE;
    }
    if (ship->timer[T_RAM_WEAPONS] > 0) 
    {
        send_to_char("Your gun crew has not recovered from the ram impact yet!\r\n", ch);
        return TRUE;
    }
    if (isname(arg, "fore")) 
    {
        return do_fire_arc(ship, ch, SIDE_FORE);
    }
    else if (isname(arg, "starboard")) 
    {
        return do_fire_arc(ship, ch, SIDE_STAR);
    }
    else if (isname(arg, "port")) 
    {
        return do_fire_arc(ship, ch, SIDE_PORT);
    }
    else if (isname(arg, "rear")) 
    {
        return do_fire_arc(ship, ch, SIDE_REAR);
    }
    else if (is_number(arg)) 
    {
        return do_fire_weapon(ship, ch, atoi(arg));
    }
    send_to_char("Valid syntax: 'fire <fore/starboard/port/rear/weapon number>'\r\n", ch);
    return TRUE;
}

int do_lock_target(P_char ch, P_ship ship, char* arg)
{
    if (!isname(GET_NAME(ch), SHIP_OWNER(ship)) && !IS_TRUSTED(ch) &&
        (ch->group == NULL ? 1 : get_char2(str_dup(SHIP_OWNER(ship))) ==
         NULL ? 1 : (get_char2(str_dup(SHIP_OWNER(ship)))->group != ch->group))) 
    {
        send_to_char ("You are not the captain of this ship, the crew ignores you.\r\n", ch);
        return TRUE;
    }

    if (!*arg) 
    {
        send_to_char("Syntax: Lock <id>/off\r\n", ch);
        return TRUE;
    }
    if (isname(arg, "off")) 
    {

        if (ship->target != NULL) {
            ship->target = NULL;
            act_to_all_in_ship(ship, "Target Cleared.\r\n");
            return TRUE;
        } else {
            send_to_char("You currently have no target.\r\n", ch);
            return TRUE;
        }
    }
    if (IS_TRUSTED(ch))
    {
        if (isname(arg, "ai_off") && IS_TRUSTED(ch)) 
        {
            if (ship->npc_ai != 0)
            {
                delete ship->npc_ai;
                ship->npc_ai = 0;
            }
        }
        if (isname(arg, "ai_pirate")) 
        {
            if (!ship->npc_ai)
                ship->npc_ai = new NPCShipAI(ship, ch);
            ship->npc_ai->type = NPC_AI_PIRATE;
            ship->npc_ai->mode = NPC_AI_CRUISING;
        }
        if (isname(arg, "ai_hunter")) 
        {
            if (!ship->npc_ai)
                ship->npc_ai = new NPCShipAI(ship, ch);
            ship->npc_ai->type = NPC_AI_HUNTER;
            ship->npc_ai->mode = NPC_AI_CRUISING;
        }
        if (isname(arg, "ai_escort")) 
        {
            if (!ship->npc_ai)
                ship->npc_ai = new NPCShipAI(ship, ch);
            ship->npc_ai->type = NPC_AI_ESCORT;
            ship->npc_ai->mode = NPC_AI_CRUISING;
        }
        if (isname(arg, "ai_advanced")) 
        {
            if (ship->npc_ai)
                ship->npc_ai->advanced = true;
        }
        if (isname(arg, "ai_basic")) 
        {
            if (ship->npc_ai)
                ship->npc_ai->advanced = false;
        }
    }


    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to lock your weapons!\r\n", ch); 
        return TRUE; 
    } 
    int k = getcontacts(ship);
    for (int i = 0; i < k; i++) 
    {
        P_ship temp = contacts[i].ship;
        if (isname(arg, temp->id)) 
        {
            if (SHIP_DOCKED(temp)) 
            {
                send_to_char ("You cannot lock onto docked ships, that's against the sailor's code!\r\n", ch);
                return TRUE;
            }
            if (ship->timer[T_BSTATION] == 0) 
            {
                act_to_all_in_ship(ship, "&+RYour crew scrambles to battle stations!&N\r\n");
                ship->timer[T_BSTATION] = BSTATION;
            }
            ship->target = temp;
            act_to_all_in_ship_f(ship, "Locked onto &+W[%s]:&N %s&N\r\n", temp->id, temp->name);
            return TRUE;
        }
    }
    send_to_char("Target not in sight!\r\n", ch);
    return TRUE;
}

int look_cargo(P_char ch, P_ship ship)
{
    if( ship->money > 0 )
    {
        send_to_char_f(ch, "&+WShip's Coffer: %s\r\n\r\n", coin_stringv(ship->money));
    } 

    send_to_char("&+cCargo Manifest&N\r\n", ch);
    send_to_char("----------------------------------\r\n", ch);

    for (int slot = 0; slot < MAXSLOTS; slot++) 
    {
        if (ship->slot[slot].type == SLOT_CARGO) 
        {
            send_to_char_f(ch, "%s&n, &+Y%d&n crates, bought for %s.\r\n",
              cargo_type_name(ship->slot[slot].index),
              ship->slot[slot].val0,
              ship->slot[slot].val1 != 0 ? coin_stringv(ship->slot[slot].val1) : "nothing");
        }
        else if (ship->slot[slot].type == SLOT_CONTRABAND) 
        {
            send_to_char_f(ch, "&+Y*&n%s&n, &+Y%d&n crates, bought for %s.\r\n",
              contra_type_name(ship->slot[slot].index),
              ship->slot[slot].val0,
              ship->slot[slot].val1 != 0 ? coin_stringv(ship->slot[slot].val1) : "nothing");
        }
    }

    send_to_char_f(ch, "\r\nCargo capacity: &+W%d&n/&+W%d\r\n", SHIP_AVAIL_CARGO_LOAD(ship), SHIP_MAX_CARGO_LOAD(ship));

    return TRUE;
}

int look_crew (P_char ch, P_ship ship)
{
    send_to_char_f(ch, "&+LCurrent crew: %s\r\n", ship_crew_data[ship->crew.index].name);


    if (ship->crew.sail_chief != NO_CHIEF)
        send_to_char_f(ch, "              %s\r\n", ship_chief_data[ship->crew.sail_chief].name);
    if (ship->crew.guns_chief != NO_CHIEF)
        send_to_char_f(ch, "              %s\r\n", ship_chief_data[ship->crew.guns_chief].name);
    if (ship->crew.rpar_chief != NO_CHIEF)
        send_to_char_f(ch, "              %s\r\n", ship_chief_data[ship->crew.rpar_chief].name);

    send_to_char_f(ch, "&+LDeck skill:   &+W%-5d&N", (int)ship->crew.sail_skill);
    if (ship->crew.sail_mod() != 0)
        send_to_char_f(ch, " &+W(%s%d)&N\r\n", (ship->crew.sail_mod() > 0) ? "+" : "", ship->crew.sail_mod());
    else
        send_to_char("\r\n", ch);

    send_to_char_f(ch, "&+LGuns skill:   &+W%-5d&N", (int)ship->crew.guns_skill);
    if (ship->crew.guns_mod() != 0)
        send_to_char_f(ch, " &+W(%s%d)&N\r\n", (ship->crew.guns_mod() > 0) ? "+" : "", ship->crew.guns_mod());
    else
        send_to_char("\r\n", ch);

    send_to_char_f(ch, "&+LRepair skill: &+W%-5d&N", (int)ship->crew.rpar_skill);
    if (ship->crew.rpar_mod() != 0)
        send_to_char_f(ch, " &+W(%s%d)&N\r\n", (ship->crew.rpar_mod() > 0) ? "+" : "", ship->crew.rpar_mod());
    else
        send_to_char("\r\n", ch);
          
    send_to_char_f(ch, "&+LStamina:      %s%d/&+G%d&N\r\n", ship->crew.get_stamina_prefix(), ship->crew.get_display_stamina(), (int)ship->crew.max_stamina);

    return TRUE;
}

int look_weapon (P_char ch, P_ship ship, char* arg)
{
    if (!*arg) 
    {
        send_to_char("Valid syntax: look <sight/weapon> <weapon number>\r\n", ch);
        return TRUE;
    }
    if(SHIP_DOCKED(ship)) 
    { 
        send_to_char("You must be undocked to sight your weapons!\r\n", ch); 
        return TRUE; 
    } 
                     
    if(!IS_MAP_ROOM(ship->location)) 
    { 
        send_to_char("You must be on the open sea to sight your weapons!\r\n", ch); 
        return TRUE; 
    } 
                          
    if (ship->target == NULL)
        send_to_char("No target.\r\n", ch);

    if (!is_number(arg)) 
    {
        send_to_char("Invalid number!\r\n", ch);
        return TRUE;
    }
    int slot = atoi(arg);
    if ((slot >= MAXSLOTS) || (slot < 0)) 
    {
        send_to_char("Invalid Weapon\r\n", ch);
        return TRUE;
    }
    if (ship->slot[slot].type != SLOT_WEAPON) 
    {
        send_to_char("Invalid Weapon!\r\n", ch);
        return TRUE;
    }
    if (SHIP_WEAPON_DESTROYED(ship, slot)) 
    {
        send_to_char("That weapon is destroyed!\r\n", ch);
        return TRUE;
    }
    if (SHIP_WEAPON_DAMAGED(ship, slot)) 
    {
        send_to_char("That weapon is damaged!\r\n", ch);
        return TRUE;
    }

    int k = getcontacts(ship);
    int j;
    for (j = 0; j < k; j++)
    {
        if (ship->target == contacts[j].ship)
            break;
    }
    if (j == k) 
    {
        send_to_char("Target out of range or out of sight!\r\n", ch);
        return TRUE;
    }
    send_to_char_f(ch, "Chance to hit target: &+W%d%%&N\r\n", weaponsight(ship, slot, j, ch));
    return TRUE;
}

int look_tactical_map(P_char ch, P_ship ship, char* arg1, char* arg2)
{
  int      x, y;
  float    shiprange;

  if( SHIP_DOCKED(ship))
  {
      send_to_char("You must be undocked to look tactical.\r\n", ch);
      return TRUE;
  }

  if( !*arg1 )
  {
      x = (int) ship->x;
      y = (int) ship->y;
  }
  else
  {
    if( !*arg2 )
    {
      send_to_char("&+WValid syntax: look <tactical/t> [<x> <y>]&N\r\n", ch);
      return TRUE;
    }
    if( is_number(arg1) && is_number(arg2) )
    {
      shiprange = range(ship->x, ship->y, 0, atoi(arg1), atoi(arg2), 0);
      if ((int) (shiprange + .5) <= 35) 
      {
        x = atoi(arg1);
        y = atoi(arg2);
      }
      else
      {
        send_to_char_f(ch, "This coord is out of range.\r\nMust be within 35 units.\r\nCurrent range: %3.1f\r\n", shiprange);
        return TRUE;
      }
    }
    else
    {
      send_to_char("&+WValid syntax: look <tactical/t> [<x> <y>]&N\r\n", ch);
      return TRUE;
    }
  }

  if( !getmap(ship) )
  {
    send_to_char("You have no maps for this region.\r\n", ch);
    return TRUE;
  }

  send_to_char_f(ch,
    "&+WTactical Map                                                H: %3d S: %3d&n\r\n", (int)ship->heading, ship->speed );

  send_to_char_f(ch,
    "&+W     %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d&N\r\n",
    x - 11, x - 9, x - 7, x - 5, x - 3, x - 1, x + 1, x + 3, x + 5, x + 7, x + 9, x + 11);
  send_to_char_f(ch,
    "     __ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__&N\r\n",
    x - 10, x - 8, x - 6, x - 4, x - 2, x, x + 2, x + 4, x + 6, x + 8, x + 10);

    y = 100 - y;
    for (int i = y - 7; i < y + 8; i++) 
    {
        send_to_char_f(ch,
                "&+W%-3d&N /%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\\r\n",
                100 - i, tactical_map[x - 11][i].map, tactical_map[x - 9][i].map,
                tactical_map[x - 7][i].map, tactical_map[x - 5][i].map,
                tactical_map[x - 3][i].map, tactical_map[x - 1][i].map,
                tactical_map[x + 1][i].map, tactical_map[x + 3][i].map,
                tactical_map[x + 5][i].map, tactical_map[x + 7][i].map,
                tactical_map[x + 9][i].map, tactical_map[x + 11][i].map);
        send_to_char_f(ch,
                "    \\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/\r\n",
                tactical_map[x - 10][i].map, tactical_map[x - 8][i].map,
                tactical_map[x - 6][i].map, tactical_map[x - 4][i].map,
                tactical_map[x - 2][i].map, tactical_map[x][i].map,
                tactical_map[x + 2][i].map, tactical_map[x + 4][i].map,
                tactical_map[x + 6][i].map, tactical_map[x + 8][i].map,
                tactical_map[x + 10][i].map);
    }
    send_to_char("       \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/\r\n", ch);
    return TRUE;
}

int look_contacts(P_char ch, P_ship ship)
{

    if( !IS_MAP_ROOM(ship->location) ||
        SHIP_DOCKED(ship) ) 
    {
        send_to_char("You must be on the open sea to display contacts.\r\n", ch);
        return TRUE;
    }

    int k = getcontacts(ship);
    send_to_char_f (ch, "&+WContact listing                                               H:%-3d S:%-3d&N\r\n", (int)ship->heading, ship->speed);
    send_to_char_f (ch, "=========================================================================|&N\r\n");
    for (int i = 0; i < k; i++) 
    {
        if (SHIP_DOCKED(contacts[i].ship)) 
        {
            if (contacts[i].range > 5)
                continue;
        }
        //dispcontact(i);

        const char* race_indicator = "&n";
        if (contacts[i].range < SCAN_RANGE && !SHIP_DOCKED(contacts[i].ship))
        {
            if (contacts[i].ship->race == GOODIESHIP)
                race_indicator = "&+Y";
            else if (contacts[i].ship->race == EVILSHIP)
                race_indicator = "&+R";
            else if (contacts[i].ship->race == UNDEADSHIP)
                race_indicator = "&+L";
            else if (contacts[i].ship->race == SQUIDSHIP)
                race_indicator = "&+M";
            else if (contacts[i].ship->race == UNKNOWNSHIP)
                race_indicator = "&+C";
        }
        const char* target_indicator1 =  (contacts[i].ship->target == ship) ? "&+W" : "";
        const char* target_indicator2 =  (contacts[i].ship == ship->target) ? "&+G" : "";

        send_to_char_f(ch,
          "%s[&N%s%s&N%s]&N %s%-30s X:%-3d Y:%-3d R:%-5.1f B:%-3d H:%-3d S:%-3d&N|%s%s\r\n",
          race_indicator,
          target_indicator1,
          contacts[i].ship->id, 
          race_indicator,
          target_indicator2,
          strip_ansi(contacts[i].ship->name).c_str(), 
          contacts[i].x, 
          contacts[i].y, 
          contacts[i].range, 
          (int)contacts[i].bearing, 
          (int)contacts[i].ship->heading,
          contacts[i].ship->speed, 
          contacts[i].arc,
          SHIP_FLYING(contacts[i].ship) ? "&+cF&N" :
            SHIP_SINKING(contacts[i].ship) ? "&+RS&N" :
              SHIP_DOCKED(contacts[i].ship) ? "&+yD&N" :
                SHIP_ANCHORED(contacts[i].ship) ? "&+yA&N" : "");
    }
    return TRUE;
}

int look_weaponspec(P_char ch, P_ship ship)
{
    char rng[20], dam[20];
    send_to_char("&+rWeapon Specifications&N\r\n", ch);
    send_to_char("&+r===========================================================&N\r\n", ch);
    send_to_char("Num  Name                     Range  Damage    Ammo   Status\r\n", ch);
    for (int slot = 0; slot < MAXSLOTS; slot++) 
    {
        if (ship->slot[slot].type == SLOT_WEAPON) 
        {
            int w_index = ship->slot[slot].index;
            sprintf(rng, "%d-%d", weapon_data[w_index].min_range, weapon_data[w_index].max_range);
            if (weapon_data[w_index].fragments > 1)
            {
                if (weapon_data[w_index].min_damage == weapon_data[w_index].max_damage)
                    sprintf(dam, "%d x %d", weapon_data[w_index].fragments, weapon_data[w_index].min_damage);
                else
                    sprintf(dam, "%d x %d-%d", weapon_data[w_index].fragments, weapon_data[w_index].min_damage, weapon_data[w_index].max_damage);
            }
            else
            {
                if (weapon_data[w_index].min_damage == weapon_data[w_index].max_damage)
                    sprintf(dam, "%d", weapon_data[w_index].min_damage);
                else
                    sprintf(dam, "%d-%d", weapon_data[w_index].min_damage, weapon_data[w_index].max_damage);
            }
            if (!SHIP_WEAPON_DESTROYED(ship, slot))
            {
                send_to_char_f(ch,  "&+W[%2d]  %-20s    %5s  %7s    %2d    %s&N\r\n",
                  slot, weapon_data[w_index].name, rng, dam, weapon_data[w_index].ammo, ship->slot[slot].get_status_str());
            }
            else
            {
                send_to_char_f(ch,  "&+W[%2d]  %-20s    %5s  %7s    &+L**    %s&N\r\n",
                  slot, weapon_data[w_index].name, rng, dam, ship->slot[slot].get_status_str());
            }
       }
    }
    return TRUE;
}


char slot_desc[100];
char* generate_slot(P_ship ship, int sl)
{
  if (sl >= MAXSLOTS)
  {
    strcpy(slot_desc, "");
    return slot_desc;
  }
    
  if (ship->slot[sl].type == SLOT_CARGO || ship->slot[sl].type == SLOT_CONTRABAND)
  {
      sprintf(slot_desc, "&+W[%2d] &n%s&n (&+Y%d&n %s)", sl, ship->slot[sl].get_description(), ship->slot[sl].val0, (ship->slot[sl].val0 > 1) ? "crates" : "crate");
  }
  else if (ship->slot[sl].type == SLOT_WEAPON)
  {
      if (!SHIP_WEAPON_DESTROYED(ship, sl))
      {
          sprintf(slot_desc, "&+W[%2d] %-20s &+W%-9s   %2d    %s", sl, 
              ship->slot[sl].get_description(), ship->slot[sl].get_position_str(), ship->slot[sl].val1, ship->slot[sl].get_status_str());
      }
      else
      {
          sprintf(slot_desc, "&+W[%2d] %-20s &+W%-9s   &+L**    %s", sl,
              ship->slot[sl].get_description(), ship->slot[sl].get_position_str(), ship->slot[sl].get_status_str());
      }
  }
  else if (ship->slot[sl].type == SLOT_EQUIPMENT)
  {
      sprintf(slot_desc, "&+W[%2d] %-20s                   %s", sl, 
          ship->slot[sl].get_description(), ship->slot[sl].get_status_str());
  }
  else
  {
    strcpy(slot_desc, " ");
  }
  return slot_desc;
}

const char* get_ship_status(P_ship ship)
{
    return
    SHIP_SINKING(ship)  ? "&=LRSINKING&N" :
    SHIP_IMMOBILE(ship) ? "&+RIMMOBILE&N" : 
    SHIP_ANCHORED(ship) ? "&+yANCHORED&N" : 
    SHIP_DOCKED(ship) ? "&+yDOCKED&N" : 
                         "&+yUNDOCKED&N"; 
}

int look_ship(P_char ch, P_ship ship)
{
    char name_format[20];
    sprintf(name_format, "%%-%ds", strlen(ship->name) + (20 - strlen(strip_ansi(ship->name).c_str())));
    int realspeed = ship->get_maxspeed();
    if(has_innate(ch, INNATE_SEADOG))
    realspeed += 2;

    char target_str[100];
    P_ship target = ship->target;
    if (target != NULL && SHIP_LOADED(target))
        sprintf(target_str, "&+GTarget: &+W[%s]&N: %s", target->id, target->name);
    else
        sprintf(target_str, " ");

    send_to_char_f(ch, "&+GName:&N ");
    send_to_char_f(ch, name_format, ship->name);
    send_to_char_f(ch, "               %s\r\n", target_str);
    send_to_char(      "&+L-========================================================================-&N\r\n", ch);
    send_to_char_f(ch, "&+LCaptain: &+W%-20s &+rFrags: &+W%-5d     &+LStatus: %-13s     &+LID[&+Y%s&+L]&N\r\n",
            SHIP_OWNER(ship), ship->frags, get_ship_status(ship), SHIP_ID(ship));
    send_to_char_f(ch, "\r\n");
    send_to_char_f(ch, "        %s%3d&N/&+G%-3d      &+LSpeed Range: &+W0-%-3d      &+LCrew: &+W%-20s&N\r\n",
            SHIP_ARMOR_COND(SHIP_MAX_FARMOR(ship), SHIP_FARMOR(ship)),
            SHIP_FARMOR(ship), SHIP_MAX_FARMOR(ship),
            //ship->get_maxspeed(), ship_crew_data[ship->crew.index].name);
	     realspeed, ship_crew_data[ship->crew.index].name);
    send_to_char_f(ch, "                          &+LWeight: &+W%3d,000          &+W%-20s&N\r\n",
            SHIP_HULL_WEIGHT(ship), (ship->crew.sail_chief == NO_CHIEF) ? "" : ship_chief_data[ship->crew.sail_chief].name);
    send_to_char_f(ch, "           &+y||&N               &+LLoad: &+W%3d/&+W%3d&N          %-20s&N\r\n",
            SHIP_SLOT_WEIGHT(ship), SHIP_MAX_WEIGHT(ship), (ship->crew.guns_chief == NO_CHIEF) ? "" : ship_chief_data[ship->crew.guns_chief].name);
    send_to_char_f(ch, "          &+y/..\\&N        &+LPassengers: &+W%2d/%2d&N            %-20s&N\r\n", 
            num_people_in_ship(ship), ship->get_capacity(), (ship->crew.rpar_chief == NO_CHIEF) ? "" : ship_chief_data[ship->crew.rpar_chief].name);
    send_to_char_f(ch, "         &+y/.%s%2d&+y.\\        &N\r\n", SHIP_INTERNAL_COND(SHIP_MAX_FINTERNAL(ship), SHIP_FINTERNAL(ship)), SHIP_FINTERNAL(ship));
    send_to_char(      "        &+y/..&N--&+y..\\        &+LNum  Name                 Position   Ammo   Status&N\r\n", ch);
    send_to_char_f(ch, "        &+y|..&+g%2d&+y..|        %s&N\r\n", SHIP_MAX_FINTERNAL(ship), generate_slot(ship, 0));
    send_to_char_f(ch, "        &+y|......| &+g%3d    %s&N\r\n", ship->mainsail, generate_slot(ship, 1));
    send_to_char_f(ch, "        &+y\\__..__/ &N---    %s&N\r\n", generate_slot(ship, 2));
    send_to_char_f(ch, "        &+y|..||..|&+L/&N&+g%3d    %s&N\r\n", SHIP_MAX_SAIL(ship), generate_slot(ship, 3));
    send_to_char_f(ch, "        &+y|......&+L/        %s&N\r\n", generate_slot(ship, 4));
    send_to_char_f(ch, "        &+y|.....&+L/&N&+y|        %s&N\r\n", generate_slot(ship, 5));
    send_to_char_f(ch, "        &+y|....&+L/&N&+y.|        %s&N\r\n", generate_slot(ship, 6));
    send_to_char_f(ch, "    %s%3d &N&+y|%s%2d&+y.&+L/&N%s%2d&+y| %s%3d    %s&N\r\n",
            SHIP_ARMOR_COND(SHIP_MAX_PARMOR(ship), SHIP_PARMOR(ship)), SHIP_PARMOR(ship),
            SHIP_INTERNAL_COND(SHIP_MAX_PINTERNAL(ship), SHIP_PINTERNAL(ship)), SHIP_PINTERNAL(ship),
            SHIP_INTERNAL_COND(SHIP_MAX_SINTERNAL(ship), SHIP_SINTERNAL(ship)), SHIP_SINTERNAL(ship), 
            SHIP_ARMOR_COND(SHIP_MAX_SARMOR(ship), SHIP_SARMOR(ship)), SHIP_SARMOR(ship),  generate_slot(ship, 7));
    send_to_char_f(ch, "    &N--- &+y|&N--&+Y/\\&N--&+y| &N---    %s&N\r\n", generate_slot(ship, 8));
    send_to_char_f(ch, "    &+G%3d &N&+y|&+g%2d&+Y\\/&N&+g%2d&+y| &+G%3d    %s&N\r\n",
            SHIP_MAX_PARMOR(ship), SHIP_MAX_PINTERNAL(ship), SHIP_MAX_SINTERNAL(ship), SHIP_MAX_SARMOR(ship), generate_slot(ship, 9));
    send_to_char_f(ch, "        &+y|......|        %s&N\r\n", generate_slot(ship, 10));
    send_to_char_f(ch, "        &+y|__||__|        %s&N\r\n", generate_slot(ship, 11));
    send_to_char_f(ch, "        &+y/......\\        %s&N\r\n", generate_slot(ship, 12));
    send_to_char_f(ch, "        &+y|......|        %s&N\r\n", generate_slot(ship, 13));
    send_to_char_f(ch, "        &+y|..%s%2d&+y..|        %s&N\r\n", 
            SHIP_INTERNAL_COND(SHIP_MAX_RINTERNAL(ship), SHIP_RINTERNAL(ship)), SHIP_RINTERNAL(ship), generate_slot(ship, 14));
    send_to_char_f(ch, "        &+y|..&N--&+y..|        %s&N\r\n", generate_slot(ship, 15));
    send_to_char_f(ch, "        &+y|..&+g%2d&+y..|\r\n", SHIP_MAX_RINTERNAL(ship));
    send_to_char_f(ch, "        &+y\\______/    &NSet Heading: &+W%-3d  &NSet Speed: &+W%-4d&N  Crew Stamina: %s%d&N\r\n",
            (int)ship->setheading, ship->setspeed, ship->crew.get_stamina_prefix(), ship->crew.get_display_stamina());
    send_to_char_f(ch, "                        &NHeading: &+W%-3d      &NSpeed: &+W%-4d&N  Repair Stock: &+W%d&N\r\n", 
            (int)ship->heading, ship->speed, ship->repair);
    send_to_char_f(ch, "        %s%3d&N/&+G%-3d&N\r\n", SHIP_ARMOR_COND(SHIP_MAX_RARMOR(ship), SHIP_RARMOR(ship)), SHIP_RARMOR(ship), SHIP_MAX_RARMOR(ship));
    return TRUE;
}

int claim_coffer(P_char ch, P_ship ship)
{
    if (!isname(GET_NAME(ch), SHIP_OWNER(ship)))
    {
        send_to_char("But you are not the captain of this ship...\r\n", ch);
        return false;
    }

    if (ship->money == 0) 
    {
        send_to_char("The ship's coffers are empty!\r\n", ch);
        return TRUE;
    }
    send_to_char_f(ch, "You get %s from the ship coffers.\r\n", coin_stringv(ship->money));
    ADD_MONEY(ch, ship->money);
    logit(LOG_SHIP, "%s got %d from the ships coffers.", J_NAME(ch), ship->money);
    ship->money = 0;
    return TRUE;
}

int do_commands_help(P_ship ship, P_char ch)
{
    send_to_char("Info commands:\r\n", ch);
    send_to_char(" look commands\r\n", ch);
    send_to_char(" look ship/status\r\n", ch);
    send_to_char(" look crew\r\n", ch);
    send_to_char(" look cargo\r\n", ch);
    send_to_char(" look weaponspec\r\n", ch);
    send_to_char(" look contacts\r\n", ch);
    send_to_char(" look tactical [<x> <y>]\r\n", ch);
    send_to_char(" scan [<id>]\r\n\r\n", ch);

    send_to_char("Control commands:\r\n", ch);
    send_to_char(" order heading <N/E/S/W/NW/NE/SW/SE/heading>\r\n", ch);
    send_to_char(" order speed <speed>:\r\n", ch);
    send_to_char(" order sail <N/E/S/W/heading/off> <distance>:\r\n", ch);
    send_to_char(" order jettison cargo/contraband [<ncrates>]\r\n", ch);
    send_to_char(" order salvage [<ncrates>]\r\n", ch);
    send_to_char(" order anchor/undock\r\n", ch);
    send_to_char(" order maneuver <N/E/S/W>\r\n", ch);
    send_to_char(" order fly/land\r\n", ch);
    send_to_char(" order signal <id> <message>\r\n", ch);
    send_to_char(" get coins/money\r\n\r\n", ch);

    send_to_char("Combat commands:\r\n", ch);
    send_to_char(" lock <id>/off\r\n", ch);
    send_to_char(" look sight/weapon <weapon>\r\n", ch);
    send_to_char(" fire <weapon/fore/rear/port/starboard>\r\n", ch);
    send_to_char(" order ram [off]\r\n", ch);
    return TRUE;
}
int ship_panel_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
    if (!ch && !cmd)
        return TRUE;

    if (cmd == -10)
        return TRUE;


    if (!ch)
        return(FALSE);

    if (cmd != CMD_GET && cmd != CMD_ORDER && cmd != CMD_SCAN 
        && cmd != CMD_FIRE && cmd != CMD_LOCK && cmd != CMD_LOOK)
        return FALSE;

    P_ship ship = NULL;

    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (svs->panel == obj)
        {
            ship = svs;
            break;
        }
    }
    
    
    if (ship == NULL) 
    {
        send_to_char("Not valid ship or error in ship code.\r\n", ch);
        return FALSE;
    }
    if (!SHIP_LOADED(ship))
    {
        send_to_char("Error: ship is not loaded!\r\n", ch);
        return FALSE;
    }

    if ((cmd == CMD_GET) && arg)
    {
        if (isname(arg, "coins money"))
        {
            return claim_coffer(ch, ship);
        }
        return FALSE;
    }
    if( (cmd == CMD_ORDER) && arg )
    {
        half_chop(arg, arg1, tmp_str);
        if( *arg1 != '\0' && is_abbrev(arg1, "fire") )
        {
          ship_panel_proc(obj, ch, CMD_FIRE, tmp_str);
          return TRUE;
        }

        half_chop(tmp_str, arg2, arg3);

        if (!(isname(arg1, "sail sa") ||
              isname(arg1, "jettison j") || isname(arg1, "salvage") ||
              isname(arg1, "undock") ||
              isname(arg1, "maneuver") || isname(arg1, "m") ||
              isname(arg1, "anchor") ||
              isname(arg1, "ram") ||
              isname(arg1, "heading") || isname(arg1, "h") ||
              isname(arg1, "speed") || isname(arg1, "s") ||
              isname(arg1, "fly") || isname(arg1, "land") ||
              isname(arg1, "signal")))
        {
            do_order(ch, arg, cmd);
            return TRUE;
        }
      
        if (!isname(str_dup(SHIP_OWNER(ship)), GET_NAME(ch)) && !IS_TRUSTED(ch))
        {
            P_char real_owner = get_char2(str_dup(SHIP_OWNER(ship)));
            if (ch->group == NULL || real_owner == NULL || real_owner->group != ch->group)
            {
                send_to_char ("You are not the captain of this ship, the crew ignores you.\r\n", ch);
                return TRUE;
            }
        }
        if (ship->timer[T_MAINTENANCE] > 0) 
        {
            send_to_char_f(ch, "This ship is being worked on for at least another %.1f hours, it can't move.\r\n", (float) ship->timer[T_MAINTENANCE] / 75.0);
            return TRUE;
        }
        if (ship->get_capacity() < num_people_in_ship(ship)) 
        {
            send_to_char ("Arrgh! There are too many people on this ship to move!\r\n", ch);
            return TRUE;
        }
        if (SHIP_SINKING(ship)) 
        {
            send_to_char("&+RYou cannot control the ship while it's sinking!&N\r\n", ch);
            return TRUE;
        }
        if (ship->timer[T_MINDBLAST] > 0) 
        {
            send_to_char("&+RYour crew members crawl around and ignore your orders!&N\r\n", ch);
            return TRUE;
        }

        if (isname(arg1, "sail sa")) 
        {
            return order_sail(ch, ship, arg2, arg3);
        }
        if (isname(arg1, "salvage"))
        {
            return salvage_cargo(ch, ship, arg2);
        }
        if (isname(arg1, "jettison j"))
        {
            if (isname(arg2, "cargo")) 
            {
                return jettison_cargo(ch, ship, arg3);
            } 
            else if (isname(arg2, "contraband")) 
            {
                return jettison_contraband(ch, ship, arg3);
            }
            send_to_char("Valid syntax: order jettison <cargo/contraband> [<number of crates>]\r\n", ch);
            return TRUE;
        }
        if (isname(arg1, "undock")) 
        {
            return order_undock(ch, ship);
        }
        if (isname(arg1, "maneuver") || isname(arg1, "m")) 
        {
            return order_maneuver(ch, ship, arg2);
        }
        if (isname(arg1, "anchor")) 
        {
            return order_anchor(ch, ship);
        }
        if (isname(arg1, "ram")) 
        {
            return order_ram(ch, ship, arg2);
        }

        if (isname(arg1, "heading") || isname(arg1, "h")) 
        {
            return order_heading(ch, ship, arg2);
        }

        if (isname(arg1, "speed") || isname(arg1, "s")) 
        {
            return order_speed(ch, ship, arg2);
        }

        if (isname(arg1, "signal")) 
        {
            return order_signal(ch, ship, arg2, arg3);
        }

        if (isname(arg1, "fly")) 
        {
            return order_fly(ch, ship);
        }

        if (isname(arg1, "land")) 
        {
            return order_land(ch, ship);
        }

        return FALSE;
    }

    if (cmd == CMD_SCAN) 
    {
      // Use regular scan if ship is docked.
      if( SHIP_DOCKED(ship) )
      {
        return FALSE;
      }
      return do_scan(ch, ship, arg);
    }

    if (cmd == CMD_FIRE) 
    {
        return do_fire(ch, ship, arg);
    }

    if (cmd == CMD_LOCK) 
    {
        return do_lock_target(ch, ship, arg);
    }

    if ((cmd == CMD_LOOK) && arg) 
    {
        half_chop(arg, arg1, tmp_str);
        half_chop(tmp_str, arg2, arg3);

        if (isname(arg1, "commands")) 
        {
            return do_commands_help(ship, ch);
        }
        if (isname(arg1, "cargo")) 
        {
            return look_cargo(ch, ship);
        }
        if (isname(arg1, "crew")) 
        {
            return look_crew(ch, ship);
        }
        if (isname(arg1, "sight weapon")) 
        {
            return look_weapon(ch, ship, arg2);
        }
        if (isname(arg1, "tactical") || isname(arg1, "t")) 
        {
            return look_tactical_map(ch, ship, arg2, arg3);
        }
        if (isname(arg, "contacts") || isname(arg, "c")) 
        {
            return look_contacts(ch, ship);
        }
        if (isname(arg, "weaponspec")) 
        {
            return look_weaponspec(ch, ship);
        }
        if (isname(arg, "show"))
        {
            update_ship_status(ship);
            ship->show(ch);
            return TRUE;
        }
        if (isname(arg, "status") || isname(arg, "ship")) 
        {
            return look_ship(ch, ship);
        }
        return FALSE;
    }
    return FALSE;
}

