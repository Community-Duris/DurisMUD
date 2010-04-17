/*****************************************************
* newshipact.c
*
* Ship AI routines
*****************************************************/

#include <stdio.h>
#include <string.h>

#include "ships.h"
#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "ship_ai.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include "map.h"
#include <math.h>

void  act_to_all_in_ship(P_ship ship, const char *msg);
int   getmap(P_ship ship);
int   bearing(float x1, float y1, float x2, float y2);
float range(float x1, float y1, float z1, float x2, float y2, float z2);

static char buf[MAX_STRING_LENGTH];

//Internal variables
struct shipai_data *shipai;

void initialize_shipai()
{
  shipai = NULL;
}

int xbearing(int bearing, int range)
{
  float    rad;

  rad = (float) ((float) bearing * M_PI / 180);
  return (int) (range * sin(rad));
}

int ybearing(int bearing, int range)
{
  float    rad;

  rad = (float) ((float) bearing * M_PI / 180);
  return (int) (range * cos(rad));
}

int assign_shipai(P_ship ship)
{
  struct shipai_data *ai;
  int      i;

  if (!ship)
  {
    return FALSE;
  }
  if (ship->shipai)
  {
    if (IS_SET(ship->shipai->flags, AIB_ENABLED))
    {
      return FALSE;
    }
    else
    {
      ai = ship->shipai;
    }
  }
  else
  {
    CREATE(ai, shipai_data, 1, MEM_TAG_SHIPAI);

    ship->shipai = ai;
  }

  ai->type = 0;
  ai->target = NULL;
  ai->flags = 0;
  ai->t_room = 0;
  ai->ship = ship;
  ai->group = NULL;
  for (i = 0; i < MAXAITIMER; i++)
  {
    ai->timer[i] = 0;
  }
  SET_BIT(ai->flags, AIB_ENABLED);
  return TRUE;
}

int shipgroupremove(struct shipai_data *shipai)
{
  struct shipgroup_data *tmpgroup;
  struct shipgroup_data *tmpgroup2;
  struct shipai_data *leader = NULL;

  if (!shipai)
  {
    return FALSE;
  }
  if (!shipai->group)
  {
    return FALSE;
  }

  if (shipai->group->leader == shipai)
  {
    if (!shipai->group->next)
    {
      shipai->group->leader = NULL;
      shipai->group->ai = NULL;
      FREE(shipai->group);
      shipai->group = NULL;
      return TRUE;
    }
    tmpgroup2 = shipai->group;
    tmpgroup = shipai->group->next;
    tmpgroup2->leader = NULL;
    tmpgroup2->ai = NULL;
    FREE(tmpgroup2);
    shipai->group = NULL;

    while (tmpgroup)
    {
      if (IS_SET(shipai->flags, AIB_DRONE))
      {
        //Destroy drone code
        tmpgroup2 = tmpgroup;
        tmpgroup = tmpgroup->next;
        tmpgroup2->ai->group = NULL;
        tmpgroup2->leader = NULL;
        tmpgroup2->ai = NULL;
        FREE(tmpgroup2);
        continue;
      }
      if (IS_SET(shipai->flags, AIB_MOB))
      {
        if (!leader)
        {
          tmpgroup->leader = shipai;
          leader = shipai;
        }
        else
        {
          tmpgroup->leader = leader;
        }
        if (IS_SET(shipai->flags, AIB_HUNTER))
        {
          shipai->mode = AIM_SEEK;
        }
        else
        {
          shipai->mode = AIM_WAIT;
        }
      }
      tmpgroup = tmpgroup->next;
    }
    return TRUE;
  }
  tmpgroup = shipai->group->leader->group;
  while (tmpgroup)
  {
    if ((tmpgroup = shipai->group))
    {
      tmpgroup2 = tmpgroup;
      tmpgroup = tmpgroup->next;
      tmpgroup2->leader = NULL;
      tmpgroup2->ai = NULL;
      FREE(tmpgroup2);
      shipai->group = NULL;
      continue;
    }
    tmpgroup = tmpgroup->next;
  }
  return TRUE;
}

int shipgroupadd(struct shipai_data *shipai, struct shipgroup_data *group)
{
  struct shipgroup_data *newgroup;
  struct shipgroup_data *tmpgroup;

  if (!shipai)
  {
    return FALSE;
  }
  if (shipai->group)
  {
    return FALSE;
  }
  if (!group)
  {
    CREATE(group, shipgroup_data, 1, MEM_TAG_SHIPGRP);

    group->leader = shipai;
    group->next = NULL;
    group->ai = shipai;
    shipai->group = group;
    return TRUE;
  }
  else
  {
    CREATE(newgroup, shipgroup_data, 1, MEM_TAG_SHIPGRP);

    newgroup->leader = group->leader;
    newgroup->ai = shipai;
    newgroup->next = NULL;
    tmpgroup = group;
    while (tmpgroup->next)
    {
      tmpgroup = tmpgroup->next;
    }
    tmpgroup->next = newgroup;
    shipai->group = newgroup;
    return TRUE;
  }
}

void announceheading(P_ship ship, int heading)
{
  char     tbuf[MAX_STRING_LENGTH];

  sprintf(tbuf, "AI:Heading changed to %d", heading);
  act_to_all_in_ship(ship, tbuf);
}

void announcespeed(P_ship ship, int speed)
{
  char     tbuf[MAX_STRING_LENGTH];

  sprintf(tbuf, "AI:Speed changed to %d", speed);
  act_to_all_in_ship(ship, tbuf);
}

void aishipspeedadjust(P_ship ship, int speed)
{
  if (speed > ship->maxspeed)
  {
    speed = ship->maxspeed;
  }
  if (speed != ship->setspeed)
  {
    ship->setspeed = speed;
    announcespeed(ship, ship->setspeed);
  }
}

void shipai_activity(P_ship ship)
{
  struct shipai_data *ai;
  int      i, j, k, b, x, y;
  float    r;

  if (ship->shipai == NULL)
  {
    return;
  }
  if (!IS_SET(ship->shipai->flags, AIB_ENABLED))
  {
    return;
  }
  ai = ship->shipai;
/* Ship AI starts here */

  switch (ai->type)
  {
  case AI_LINE:
    getmap(ship);
    k = 0;
    for (i = 0; i < 101; i++)
    {
      for (j = 0; j < 101; j++)
      {
        if (tactical_map[i][100 - j].rroom == ai->t_room)
        {
          k = 1;
          x = i;
          y = j;
          b = bearing(50, 50, i, j);
          if (ship->setheading != b && ai->t_room != ship->location)
          {
            ship->setheading = b;
            announceheading(ship, b);
          }
        }
      }
    }
    if (k)
    {
      if (ship->setheading != ship->heading)
      {
        aishipspeedadjust(ai->ship, 0);
      }
      else
      {
        if (ship->location == ai->t_room)
        {
          aishipspeedadjust(ai->ship, 0);
          if (IS_SET(ai->flags, AIB_AUTOPILOT))
          {
            REMOVE_BIT(ai->flags, AIB_AUTOPILOT);
            REMOVE_BIT(ai->flags, AIB_ENABLED);
            act_to_all_in_ship(ship, "Destination has been reached.");
            return;
          }
        }
        else
        {
          r = 0;
          r = range(50, 50, 0, x, y, 0);
          if (IS_SET(ai->flags, AIB_BATTLER))
          {
            aishipspeedadjust(ship, ship->maxspeed);
          }
          else if (r < 2.70)
          {
            aishipspeedadjust(ai->ship, 10);
          }
          else if (r < 5.00)
          {
            aishipspeedadjust(ai->ship, 30);
          }
          else if (r < 10.00)
          {
            aishipspeedadjust(ai->ship, 60);
          }
          else
          {
            aishipspeedadjust(ship, ship->maxspeed);
          }
        }
      }
    }
    else
    {
      act_to_all_in_ship(ai->ship, "Target room not found!\r\n");
    }
    break;
  case AI_STOP:
    break;
  case AI_PATH:
    break;
  default:
    break;
  }
}



///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
////// COMBAT AI
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////



bool weapon_ready_to_fire(P_ship ship, int w_num)
{
    if (ship->timer[T_MINDBLAST] > 0) 
        return false;
    if (ship->timer[T_RAM_WEAPONS] > 0) 
        return false;
    if (SHIPWEAPONDESTROYED(ship, w_num)) 
        return false;
    if (SHIPWEAPONDAMAGED(ship, w_num)) 
        return false;
    if (ship->slot[w_num].timer > 0) 
        return false;
    if (ship->slot[w_num].val1 == 0) 
        return false;
    return true;
}



ShipCombatAI::ShipCombatAI(P_ship s, P_char ch)
{
    ship = s;
    t_bearing = 0;
    t_arc = 0;
    your_arc = 0;
    t_range = 0;
    t_x = 0;
    t_y = 0;
    debug_char = ch;

    out_of_ammo = false;

    for (int i = 0; i < 4;i++) 
        active_arc[i] = 0;
    too_close = 0;
    too_far = false;

    new_heading = 0;
    new_safe_speed = 0;
    cur_safe_speed = 0;
}

bool ShipCombatAI::weapon_ok(int w_num)
{
    if (SHIPWEAPONDESTROYED(ship, w_num)) 
        return false;
    if (SHIPWEAPONDAMAGED(ship, w_num)) 
        return false;
    if (ship->slot[w_num].val1 == 0) 
        return false;
    return true;
}


bool ShipCombatAI::find_target()
{
    bool found_target = false;
    int k = getcontacts(ship);

    // looking for a target
    // first, trying to find original target within range
    if (ship->target != 0)
    {
        for (int i = 0; i < k; i++) 
        {
            if (SHIPISDOCKED(contacts[i].ship) || SHIPSINKING(contacts[i].ship)) continue;
            if (contacts[i].ship == ship->target)
            {
                found_target = true;
                t_bearing = contacts[i].bearing;
                t_range = contacts[i].range;
                t_x = contacts[i].x;
                t_y = contacts[i].y;
                break;
            }
        }
    }

    // hmm, lost original one, lets see if there is anything else to sink on contacts
    if (!found_target)
    {
        send_message_to_debug_char("Could not find the current target\n");
        for (int i = 0; i < k; i++) 
        {
            if (SHIPISDOCKED(contacts[i].ship) || SHIPSINKING(contacts[i].ship)) continue;
            if (contacts[i].ship->race == GOODIESHIP || contacts[i].ship->race == EVILSHIP)
            {
                found_target = true;
                ship->target = contacts[i].ship;
                t_bearing = contacts[i].bearing;
                t_range = contacts[i].range;
                t_x = contacts[i].x;
                t_y = contacts[i].y;
                send_message_to_debug_char("Found new target: %s\n", contacts[i].ship->id);
                break;
            }
        }
    }
    if (found_target)
    {
        t_arc = getarc(ship->heading, t_bearing);
        your_arc = getarc(ship->target->heading, (t_bearing >= 180) ? (t_bearing - 180) : (t_bearing + 180));
        return true;
    }
    else
    {
        send_message_to_debug_char("Could not find a new target\n");
        return false;
    }
}

void ShipCombatAI::try_attack()
{    // if we have a ready gun pointing to target in range and enough stamina, fire it right away!
    for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
    {
        if (ship->slot[w_num].type == SLOT_WEAPON) 
        {
            if (!weapon_ready_to_fire(ship, w_num))
                continue;
            int w_index = ship->slot[w_num].index;
            if (ship->slot[w_num].position == t_arc &&
                t_range > (float)weapon_data[w_index].min_range && 
                t_range < (float)weapon_data[w_index].max_range &&
                ship->guncrew.stamina > weapon_data[w_index].reload_stamina)
            {
                fire_weapon(ship, debug_char, w_num);
            }
        }
    }
}

bool ShipCombatAI::check_ammo()
{
    out_of_ammo = true;
    for (int slot = 0; slot < MAXSLOTS; slot++) 
    {
        if (ship->slot[slot].type == SLOT_WEAPON) 
        {
            if(ship->slot[slot].val1 != 0)
            { out_of_ammo = false; break; }
        }
    }
    return !out_of_ammo;
}

void ShipCombatAI::run_away()
{
    // TODO: smarter choice?
    new_heading = t_bearing + 180;
    if (check_dir_for_land(new_heading, 10))
    {
        bool found = false;
        for (int i = 0; i < 5; i++)
        {
            if (!check_dir_for_land(new_heading + i * 30, 10))
            {
                new_heading = new_heading + i * 30;
                found = true;
                break;
            }
            else if (!check_dir_for_land(new_heading - i * 30, 10))
            {
                new_heading = new_heading - i * 30;
                found = true;
                break;
            }
        }
        if (!found && !check_dir_for_land(t_bearing, 10))
            new_heading = t_bearing;
    }
    send_message_to_debug_char("Running away: ");
    set_new_dir();
}

void ShipCombatAI::check_weapons()
{
    too_close = 0;
    too_far = false;
    for (int i = 0; i < 4; i++) active_arc[i] = 1000;
    for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
    {
        if (ship->slot[w_num].type == SLOT_WEAPON) 
        {
            int w_index = ship->slot[w_num].index;
            if (!weapon_ok(w_num))
                continue;

            float good_range = (float)weapon_data[w_index].min_range + ((float)weapon_data[w_index].max_range - (float)weapon_data[w_index].min_range) * 0.7;

            if (ship->slot[w_num].timer == 0) // weapon ready to fire
            {
                if (t_range < (float)weapon_data[w_index].min_range && weapon_data[w_index].min_range < 10)
                    too_close = weapon_data[w_index].min_range;
                else if (t_range > good_range)
                    too_far = true;
            }
            
            if(t_range >= (float)weapon_data[w_index].min_range && t_range <= good_range)
            {
                if (active_arc[ship->slot[w_num].position] > ship->slot[w_num].timer)
                    active_arc[ship->slot[w_num].position] = ship->slot[w_num].timer;
            }
        }
    }
}



bool ShipCombatAI::try_circle_around(int arc)
{
    if (t_range >= 8)
    {
        new_heading = t_bearing;
        return true;
    }

    int land_dist[4];
    for (int i = 0; i < 4; i++)
    {
        if (i == arc) 
            land_dist[i] = 0;
        else
        {
            land_dist[i] = check_dir_for_land_from_target(ship->target->heading + get_arc_main_bearing(i), 7);
            if (land_dist[i] == 0)
                land_dist[i] = 7;
            else
                land_dist[i]--;
        }
    }
    int max_dist = 0, best_dir = 0;
    for (int i = 0; i < 4; i++)
    {
        if (max_dist < land_dist[i])
        {
            max_dist = land_dist[i];
            best_dir = i;
        }
    }
    int dst_dir = ship->target->heading + get_arc_main_bearing(best_dir);
    normalize_direction(dst_dir);
    int dst_loc = get_room_in_direction_from_target(dst_dir, max_dist);

    send_message_to_debug_char("Circling around (%d, %d, %d): ", dst_dir, max_dist, dst_loc);

    // TODO: check if there is land between you and dst_loc, and try to go straight there

    vector<int> route;
    if (!dijkstra(ship->location, dst_loc, valid_ship_edge, route))
    {
        send_message_to_debug_char("\nfailed dijkstra! ");
        return false;
    }
    if (route.size() == 0)
    {
        send_message_to_debug_char("\nempty route found!");
        return false;
    }
    switch (route[0])
    {
    case 0: new_heading = 0; break;
    case 1: new_heading = 90; break;
    case 2: new_heading = 180; break;
    case 3: new_heading = 270; break;
    default: return false;
    };

    //send_message_to_debug_char("Circling around: ");
    return true;
}


static const char* get_arc_name(int arc)
{
    switch (arc)
    {
    case FORE: return "fore";
    case PORT: return "port";
    case REAR: return "rear";
    case STARBOARD: return "starboard";
    }
    return "";
}

bool ShipCombatAI::try_turn_active_weapon()
{
    if (ship->timer[T_RAM_WEAPONS] > 0) 
        return false;

    int arc_priority[4];
    set_arc_priority(t_bearing, t_arc, arc_priority);
    for (int i = 0; i < 4; i++)
    {
        if (active_arc[arc_priority[i]] < 4) // there is weapon ready or almost ready to fire, turning
        {
            new_heading = t_bearing - get_arc_main_bearing(arc_priority[i]);
            send_message_to_debug_char("Turning active weapon arc %s: ", get_arc_name(arc_priority[i]));
            return true;
        }
    }
    return false;
}

bool ShipCombatAI::try_turn_reloading_weapon()
{
    int best_arc = -1, best_time = 1000;
    for (int i = 0; i < 4; i++)
    {
        if (active_arc[i] < best_time)
        {
            best_time = active_arc[i];
            best_arc = i;
        }
    }
    if (best_arc != -1)
    {
        new_heading = t_bearing - get_arc_main_bearing(best_arc);
        send_message_to_debug_char("Turning reloading weapon arc %s: ", get_arc_name(best_arc));
        return true;
    }
    return false;
}


bool ShipCombatAI::try_make_distance(int distance)
{
    if (SHIPIMMOBILE(ship)) return false;

    
    float rad = (float) ((float) ((t_bearing - 180) - ship->target->heading) * M_PI / 180.000);
    float star_dist = (1.0 - sin(rad)) * (float)distance;
    float port_dist = (1.0 + sin(rad)) * (float)distance;
    float fore_dist = (1.0 - cos(rad)) * (float)distance;
    float rear_dist = (1.0 + cos(rad)) * (float)distance;

    if (star_dist < port_dist)
    {
        if (!check_dir_for_land(ship->target->heading + 90, star_dist))
        {
            new_heading = ship->target->heading + 90;
            return true;
        }
        else if (!check_dir_for_land(ship->target->heading - 90, port_dist))
        {
            new_heading = ship->target->heading - 90;
            return true;
        }
    }
    else
    {
        if (!check_dir_for_land(ship->target->heading - 90, port_dist))
        {
            new_heading = ship->target->heading - 90;
            return true;
        }
        else if (!check_dir_for_land(ship->target->heading + 90, star_dist))
        {
            new_heading = ship->target->heading + 90;
            return true;
        }
    }

    if (fore_dist < rear_dist)
    {
        if (!check_dir_for_land(ship->target->heading, fore_dist))
        {
            new_heading = ship->target->heading;
            return true;
        }
        else if (!check_dir_for_land(ship->target->heading - 180, rear_dist))
        {
            new_heading = ship->target->heading - 180;
            return true;
        }
    }
    else
    {
        if (!check_dir_for_land(ship->target->heading - 180, rear_dist))
        {
            new_heading = ship->target->heading - 180;
            return true;
        }
        else if (!check_dir_for_land(ship->target->heading, fore_dist))
        {
            new_heading = ship->target->heading;
            return true;
        }
    }

    send_message_to_debug_char("Failed breaking distance\n");
    return false;
}

bool ShipCombatAI::try_chase()
{
    if (SHIPIMMOBILE(ship)) return false;
    new_heading = calc_intercept_heading (t_bearing, ship->target->heading);
    if (check_dir_for_land(new_heading, 5))
        new_heading = t_bearing;
    return true;
}

bool ShipCombatAI::go_around_land()
{
    if (SHIPIMMOBILE(ship)) return false;

    vector<int> route;
    if (!dijkstra(ship->location, ship->target->location, valid_ship_edge, route))
        return false;
    if (route.size() == 0)
    {
        send_message_to_debug_char("\nempty route found!");
        return false;
    }
    switch (route[0])
    {
    case 0: new_heading = 0; break;
    case 1: new_heading = 90; break;
    case 2: new_heading = 180; break;
    case 3: new_heading = 270; break;
    default: return false;
    };
    return true;
}

void ShipCombatAI::activity()
{
    if(!IS_MAP_ROOM(ship->location) || SHIPSINKING(ship)) 
        return; 

    if (ship->timer[T_MINDBLAST])
        return;

    getmap(ship); // doing it here once

    // checking if we have ammo left to fight
    if (out_of_ammo || !check_ammo())
    {
        run_away();
        return;
    }

    // crap, no target in sight at all, keep moving/reloading for a bit, then disintegrate
    if (!find_target())
    {
        // TODO: set heading to last target bearing, check if its safe, init disappearance timer if not set
        // TODO: reload weapons slowly
        return;
    }


    // okay we have our taget in sight, lets attack
    try_attack();


    // now trying to figure out new heading

    cur_safe_speed = get_safe_speed(ship->heading);
    new_heading = ship->heading;
    new_safe_speed = cur_safe_speed;

    int land_between = check_dir_for_land(t_bearing, t_range);
    if (land_between)
    { // we have a land between us and target, forget about combat maneuvering for now, lets find a way to go around it
        if (!go_around_land())
        {
            send_message_to_debug_char("Going around land failed!\n");
            run_away(); // TODO: do something better?
        }
        send_message_to_debug_char("Going around land: ");
        set_new_dir();
        return;
    }

    check_weapons();

    if (ship->target->armor[your_arc] == 0 && ship->target->internal[your_arc] == 0)
    { // aha, he is immobile and faces us: heading=destroyed side, must find way around him
        try_circle_around(your_arc);
    }
    else
    {
        if (try_turn_active_weapon()) // trying to turn: heading=a weapon that is ready/almost ready to fire and within good range already
        {
        }
        else if(too_far && try_chase()) // if there is weapon ready to fire, but we are not in a good range
        {
            send_message_to_debug_char("Intercepting: ");
        }
        else if(too_close && try_make_distance(too_close)) // if there is weapon ready to fire, but we are too close, try breaking distance
        {
            send_message_to_debug_char("Breaking distance: ");
        }
        else if (try_turn_reloading_weapon()) // trying to turn: heading=a weapon that is closest to ready and within good range already
        {
        }
        else if (try_chase()) // chasing by default
        {
            send_message_to_debug_char("Intercepting: ");
        }
        else // TODO
            return;
    }

    set_new_dir();
}

void ShipCombatAI::set_new_dir()
{
    normalize_direction(new_heading);
    ship->setheading = new_heading;
    if (!SHIPIMMOBILE(ship)) 
        new_safe_speed = get_safe_speed(new_heading);
    ship->setspeed = MIN(cur_safe_speed, new_safe_speed);
    ship->setspeed = MIN(ship->setspeed, ship->get_maxspeed());
    send_message_to_debug_char("heading=%d, speed=%d\n", new_heading, ship->setspeed);
}

int ShipCombatAI::calc_intercept_heading(int tb, int th)
{
    if (tb >= th)
    {
        if (tb - th > 180)
            th += 360;
    }
    else
    {
        if (th - tb > 180)
            tb += 360;
    }
    if (tb - th > 90)
        th += ((tb - th) - 90) * 2;
    else if (tb - th < - 90)
        th -= (-(tb - th) - 90) * 2;

    int new_h = (tb + th) / 2;
    normalize_direction(new_h);
    return new_h;
}


int ShipCombatAI::get_safe_speed(int dir)
{
    normalize_direction(dir);
    int land_dist = check_dir_for_land(dir, 4);
    if (land_dist)    
    {        
        if (land_dist == 1)            
            return 0; 
        else if (land_dist == 2)
            return 11;
        else if (land_dist == 3)
            return 30;
        else if (land_dist == 4)
            return 60;
    }    
    return ship->get_maxspeed();
}

// returns the distance to land or 0 if none
int ShipCombatAI::check_dir_for_land(int heading, float range)
{
    return check_dir_for_land_from(heading, range, ship->x, ship->y);
}

int ShipCombatAI::check_dir_for_land_from_target(int heading, float range)
{
    return check_dir_for_land_from(heading, range, (float)t_x, (float)t_y);
}

int ShipCombatAI::check_dir_for_land_from(int heading, float range, float cur_x, float cur_y)
{ // tactical_map is supposed to be filled already
    float rad = (float) ((float) (heading) * M_PI / 180.000);
    float delta_x = sin(rad);
    float delta_y = cos(rad);

    for (int r = 1; r <= (int)range; r++)
    {
        cur_x += delta_x;
        cur_y += delta_y;

        if (!inside_map(cur_x, cur_y)) return 0;
        int location = tactical_map[(int) cur_x][100 - (int) cur_y].rroom;
        if (world[location].sector_type != SECT_OCEAN && !IS_SET(ship->flags, AIR))
            return r;
    }
    return 0;
}


bool ShipCombatAI::inside_map(float x, float y)
{
    if ((int)x < 0 && (int)x > 100) return false;
    if ((int)y < 0 && (int)y > 100) return false;
    return true;
}


int ShipCombatAI::get_room_in_direction_from_target(int dir, float range)
{
    float dummy1, dummy2;
    return get_room_in_direction_from_target(dummy1, dummy2, dir, range);
}
int ShipCombatAI::get_room_in_direction_from_target(float &x, float &y, int dir, float range)
{
    float rad = (float) ((float) (dir) * M_PI / 180.000);
    x = (float)t_x + sin(rad) * range;
    y = (float)t_y + cos(rad) * range;

    //send_message_to_debug_char("\nt_x = %d, t_y = %d, dir = %d", t_x, t_y, dir);
    //send_message_to_debug_char("\nrange = %f, x = %f, y = %f\n", range, x, y);

    if (!inside_map(x, y)) return 0;
    int location = tactical_map[(int) x][100 - (int) y].rroom;
    return location;
}

int ShipCombatAI::get_room_in_direction_from_ship(int dir, float range)
{
    float dummy1, dummy2;
    return get_room_in_direction_from_ship(dummy1, dummy2, dir, range);
}
int ShipCombatAI::get_room_in_direction_from_ship(float &x, float &y, int dir, float range)
{
    float rad = (float) ((float) (dir) * M_PI / 180.000);
    x = ship->x + sin(rad) * range;
    y = ship->y + cos(rad) * range;

    if (!inside_map(x, y)) return 0;

    int location = tactical_map[(int) x][100 - (int) y].rroom;
    return location;
}

int ShipCombatAI::get_arc_main_bearing(int arc)
{
    switch (arc)
    {
    case SLOT_FORE: return 0;
    case SLOT_PORT: return 270;
    case SLOT_REAR: return 180;
    case SLOT_STAR: return 90;
    };
    return 0;
}

void ShipCombatAI::normalize_direction(int &dir)
{
    while (dir >= 360) dir = dir - 360;
    while (dir < 0) dir = dir + 360;
}

void ShipCombatAI::set_arc_priority(int current_bearing, int current_arc, int* arc_priority)
{
    switch(current_arc)
    {
    case FORE:
    {
        arc_priority[0] = SLOT_FORE;
        arc_priority[3] = SLOT_REAR;
        if (current_bearing < 45)
        {
            arc_priority[1] = SLOT_STAR;
            arc_priority[2] = SLOT_PORT;
        }
        else
        {
            arc_priority[1] = SLOT_PORT;
            arc_priority[2] = SLOT_STAR;
        }
    } break;
    case STARBOARD:
    {
        arc_priority[0] = SLOT_STAR;
        arc_priority[3] = SLOT_PORT;
        if (current_bearing < 90)
        {
            arc_priority[1] = SLOT_FORE;
            arc_priority[2] = SLOT_REAR;
        }
        else
        {
            arc_priority[1] = SLOT_REAR;
            arc_priority[2] = SLOT_FORE;
        }
    } break;
    case PORT:
    {
        arc_priority[0] = SLOT_PORT;
        arc_priority[3] = SLOT_STAR;
        if (current_bearing > 270)
        {
            arc_priority[1] = SLOT_FORE;
            arc_priority[2] = SLOT_REAR;
        }
        else
        {
            arc_priority[1] = SLOT_REAR;
            arc_priority[2] = SLOT_FORE;
        }
    } break;
    case REAR:
    {
        arc_priority[0] = SLOT_REAR;
        arc_priority[3] = SLOT_FORE;
        if (current_bearing < 180)
        {
            arc_priority[1] = SLOT_STAR;
            arc_priority[2] = SLOT_PORT;
        }
        else
        {
            arc_priority[1] = SLOT_PORT;
            arc_priority[2] = SLOT_STAR;
        }
    }
    break;
    };
}



void ShipCombatAI::send_message_to_debug_char(const char *msg)
{
    if (!debug_char) return;
    sprintf(buf, msg);
    send_to_char(buf, debug_char);
}

void ShipCombatAI::send_message_to_debug_char(const char *msg, int arg1)
{
    if (!debug_char) return;
    sprintf(buf, msg, arg1);
    send_to_char(buf, debug_char);
}

void ShipCombatAI::send_message_to_debug_char(const char *msg, int arg1, int arg2)
{
    if (!debug_char) return;
    sprintf(buf, msg, arg1, arg2);
    send_to_char(buf, debug_char);
}

void ShipCombatAI::send_message_to_debug_char(const char *msg, int arg1, int arg2, int arg3)
{
    if (!debug_char) return;
    sprintf(buf, msg, arg1, arg2, arg3);
    send_to_char(buf, debug_char);
}

void ShipCombatAI::send_message_to_debug_char(const char *msg, float arg1)
{
    if (!debug_char) return;
    sprintf(buf, msg, arg1);
    send_to_char(buf, debug_char);
}

void ShipCombatAI::send_message_to_debug_char(const char *msg, float arg1, float arg2)
{
    if (!debug_char) return;
    sprintf(buf, msg, arg1, arg2);
    send_to_char(buf, debug_char);
}

void ShipCombatAI::send_message_to_debug_char(const char *msg, float arg1, float arg2, float arg3)
{
    if (!debug_char) return;
    sprintf(buf, msg, arg1, arg2, arg3);
    send_to_char(buf, debug_char);
}

void ShipCombatAI::send_message_to_debug_char(const char *msg, const char* arg1)
{
    if (!debug_char) return;
    sprintf(buf, msg, arg1);
    send_to_char(buf, debug_char);
}

const char* pirateShipNames[] = 
{
    "&+WB&+wu&+Lcc&+wan&+Weer&+L'&+ws H&+Wo&+ww&+Ll",
    "&+RD&+rev&+ri&+Rl&+L'&+Rs &+LD&+wea&+Lt&+wh",
    "&+BO&+bce&+Ba&+bn&+L'&+bs &+CM&+cer&+Bma&+ci&+Cd",
    "&+BT&+bhe &+BC&+bru&+Be&+bl &+ME&+me&+Ml",
    "&+wThe &+RB&+rl&+Roo&+rd&+Ry &+RS&+rku&+Rll &+wof &+BAt&+bl&+Ban&+bti&+Bs",
    "&+wThe &+YS&+yha&+Ym&+ye &+Lof &+wthe &+CSe&+cv&+Cen &+CS&+cea&+Cs",
    "&+wThe &+LH&+wo&+Lrr&+wi&+Wb&+wl&+Le &+wD&+Loo&+wm",
    "&+RT&+rhe &+RV&+ri&+Rle &+rH&+Roa&+rr&+Rd",
    "&+LT&+yhe &+LF&+yo&+Lul &+yC&+Lut&+yl&+La&+yss",
    "&+rT&+Lhe &+rA&+Lwf&+ru&+Ll &+rD&+Le&+rmo&+Ln",
    "&+BN&+be&+Bpt&+bun&+Be&+L'&+bs &+GS&+gerp&+Gen&+gt",
    "&+LT&+whe &+LBla&+wc&+Lk &+WTh&+wun&+Wd&+we&+Lr",
    "&+YC&+yal&+Yy&+yps&+Yo&+L'&+ys &+RAn&+rge&+Rr",
    "&+YT&+yhe &+YC&+Wu&+Yrs&+ye&+Yd &+yG&+Yo&+Wl&+Yd",
    "&+LT&+whe &+LDark &+wD&+Lagg&+we&+Lr",
    "&+WT&+whe &+WWa&+wn&+Wde&+wr&+Win&+wg &+bT&+Br&+Cid&+Be&+bnt",
    "&+WJ&+Ye&+Wwe&+Yl &+wof the &+YE&+yas&+Yt",
    "&+GD&+gra&+Ggo&+gns &+LBla&+wc&+Lk &+yPl&+Yund&+Wer",
    "&+WMa&+Cel&+Ws&+Ctr&+Wom &+wof the &+WC&+Ca&+Wrr&+Cibe&+Wan",
    "&+YB&+yu&+Ycc&+yan&+Yee&+yr&+L'&+ys &+wDeath&+Wwish",
};


struct NPCShipSetup
{
    int m_class;
    int level;
    void (*setup)(P_ship);
};

void setup_npc_01(P_ship ship)
{
    set_weapon(ship, 0, W_MEDIUM_CATAPULT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BALLISTA, STARBOARD);
    set_weapon(ship, 2, W_LARGE_BALLISTA, STARBOARD);
    set_weapon(ship, 3, W_SMALL_CATAPULT, REAR);
    setcrew(ship, 1, 3600);
    setcrew(ship, 6, 2500);
    setcrew(ship, 10, 2500);
}


NPCShipSetup npcShipSetup [] = {
    { SH_CORVETTE, 1, &setup_npc_01 },
};


P_ship load_npc_ship(int m_class, int level, int room, P_char ch = 0)
{
    P_ship ship = newship(m_class, true);
    if (!ship)
    {
        if (ch) send_to_char("Couldn't create npc ship!\r\n", ch);
        return false;
    }
    if (!ship->panel) 
    {
        if (ch) send_to_char("No panel!\r\n", ch);
        return false;
    }

    int num = number(0, sizeof(npcShipSetup) / sizeof(NPCShipSetup) - 1);

    int i = 0, ii = 0;
    while (true)
    {
        if (npcShipSetup[i].m_class == m_class && npcShipSetup[i].level == level)
        {
            if (ii == num)
                break;
            ii++;
        }
        i++;
        if (i == sizeof(npcShipSetup) / sizeof(NPCShipSetup))
        {
            if (ii == 0)
                return 0; // there is no such ship setups in list
            i = 0;
        }
    }
    npcShipSetup[i].setup(ship);

    int name_index = number(0, sizeof(pirateShipNames)/sizeof(char*) - 1);
    nameship(pirateShipNames[name_index], ship);
    assignid(ship, NULL, true);
    ship->race = NPCSHIP;
    ship->combat_ai = new ShipCombatAI(ship, ch);
    ship->ownername = 0;

    if (!loadship(ship, room))
    {
        if (ch) send_to_char("Couldnt load npc ship!\r\n", ch);
        return false;
    }
    ship->anchor = 0;
    REMOVE_BIT(ship->flags, DOCKED);
    return ship;
}


bool try_load_pirate_ship(P_ship target, P_char ch)
{
    getmap(target);

    int dir = target->heading + number(-45, 45);
    ShipCombatAI::normalize_direction(dir);

    float rad = (float) ((float) (dir) * M_PI / 180.000);
    float ship_x = 50 + sin(rad) * 34;
    float ship_y = 50 + cos(rad) * 34;

    int location = tactical_map[(int) ship_x][100 - (int) ship_y].rroom;

    char ttt[100];
    sprintf(ttt, "Location: x=%d, y=%d, loc=%d, sect=%d\r\n", (int)ship_x, (int)ship_y, location, world[location].sector_type);
    if (ch) send_to_char(ttt, ch);
    
    if (world[location].sector_type != SECT_OCEAN)
    {
        if (ch) send_to_char("Wrong location\r\n", ch);
        return false;
    }

    P_ship ship = load_npc_ship(SH_CORVETTE, 1, location, ch);

    ship->target = target;
    int ship_heading = dir + 180;
    ShipCombatAI::normalize_direction(ship_heading);
    ship->setheading = ship_heading;
    ship->heading = ship_heading;
    int ship_speed = ship->get_maxspeed();
    ship->setspeed = ship_speed;
    ship->speed = ship_speed;
    return true;
}
