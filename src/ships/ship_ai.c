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
#include <stdarg.h> 

void  act_to_all_in_ship(P_ship ship, const char *msg);
int   getmap(P_ship ship);
int   bearing(float x1, float y1, float x2, float y2);
float range(float x1, float y1, float z1, float x2, float y2, float z2);
static const char* get_arc_name(int arc);
int check_ram_arc(int heading, int bearing, int size);
bool is_boardable(P_ship ship);

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
////// NPC AI
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////



bool weapon_ok(P_ship ship, int w_num)
{
    if (SHIPWEAPONDESTROYED(ship, w_num)) 
        return false;
    if (SHIPWEAPONDAMAGED(ship, w_num)) 
        return false;
    if (ship->slot[w_num].val1 == 0) 
        return false;
    return true;
}

bool weapon_ready_to_fire(P_ship ship, int w_num)
{
    if (!weapon_ok(ship, w_num))
        return false;
    if (ship->timer[T_MINDBLAST] > 0) 
        return false;
    if (ship->timer[T_RAM_WEAPONS] > 0) 
        return false;
    if (ship->slot[w_num].timer > 0) 
        return false;
    return true;
}



NPCShipAI::NPCShipAI(P_ship s, P_char ch)
{
    ship = s;
    advanced = false;
    mode = NPC_AI_IDLING;
    t_bearing = 0;
    t_arc = 0;
    s_arc = 0;
    t_range = 0;
    t_x = 0;
    t_y = 0;
    debug_char = ch;
    did_board = false;
    speed_restriction = -1;

    if (ship->m_class == SH_DREADNOUGHT)
    {
        is_heavy_ship = true;
        is_multi_target = true;
    }

    out_of_ammo = false;

    for (int i = 0; i < 4;i++) 
        active_arc[i] = 0;
    too_close = 0;
    too_far = false;

    new_heading = 0;
}





void NPCShipAI::activity()
{
    if(!IS_MAP_ROOM(ship->location) || SHIPSINKING(ship)) 
        return; 

    if (ship->timer[T_MINDBLAST])
        return;

    getmap(ship); // doing it here once
    contacts_count = getcontacts(ship, false); // doing it here once
    speed_restriction = -1;


    switch(mode)
    {
    case NPC_AI_ENGAGING:
    {
        // checking if we have ammo left to fight
        if (!check_ammo())
        {
            mode = NPC_AI_RUNNING;
            break;
        }

        if (!find_current_target())
        {
            mode = NPC_AI_CRUISING;
            break;
        }

        if (SHIPIMMOBILE(ship))
        {
            immobile_maneuver();
            break;
        }

        if (check_dir_for_land_from(ship->x, ship->y, t_bearing, t_range))
        { // we have a land between us and target, forget about combat maneuvering for now, lets find a way to go around it
            b_attack(); // trying to fire over land
            if (!go_around_land())
                run_away(); // TODO: do something better?
            break;
        }


        if (worth_ramming() && check_ram())
        {
            ram_target();
        }

        if (is_boardable(ship->target) && !did_board)
        { // not maneuvering, just charging target
            b_attack(); // well, trying to fire on the way
            if (check_boarding_conditions())
            {
                board_target();
                break;
            }
            charge_target(true);
            break;
        }

        // we have our target in sight, lets try firing something
        if (advanced)
            a_attack();
        else
            b_attack();

        if (advanced)
            advanced_combat_maneuver();
        else
            basic_combat_maneuver();
    }   break;

    case NPC_AI_RUNNING:
    {
        if (find_current_target())
        {
            run_away();
        }
        else
        {
            mode = NPC_AI_CRUISING;
        }
    }   break;

    case NPC_AI_LOOTING:
        break;

    case NPC_AI_CRUISING:
    {
        if (find_new_target())
        {
            mode = NPC_AI_ENGAGING;
        }
        else
        {
            reload_and_repair();
            if (check_dir_for_land_from(ship->x, ship->y, ship->heading, 5)) 
                new_heading += 10; 
            set_new_dir();
        }
    }   break;

    default:
        break;
    };
}

void NPCShipAI::reload_and_repair()
{
    if (ship->mainsail < SHIPMAXSAIL(ship))
    {
        if (number(1, 30) == 1)
            ship->mainsail++;
    }
    for (int i = 0; i < 4; i++)
    {
        if (ship->internal[i] < ship->maxinternal[i])
        {
            if (number(1, 20) == 1)
                ship->internal[i]++;
        }
        else if (ship->armor[i] < ship->maxarmor[i])
        {
            if (number(1, 20) == 1)
                ship->armor[i]++;
        }
    }
    for (int slot = 0; slot < MAXSLOTS; slot++) 
    {
        if (ship->slot[slot].type == SLOT_WEAPON) 
        {
            int w_index = ship->slot[slot].index;
            if(ship->slot[slot].val1 < weapon_data[w_index].ammo)
            { 
                if (number(1, 60) == 1)
                    ship->slot[slot].val1++;
            }
        }
    }
}



/////////////////////////
// GENERAL COMBAT ///////
/////////////////////////

bool NPCShipAI::find_current_target()
{
    for (int i = 0; i < contacts_count; i++) 
    {
        if (SHIPISDOCKED(contacts[i].ship) || SHIPSINKING(contacts[i].ship)) continue;
        if (contacts[i].ship == ship->target)
        {
            update_target(i);
            return true;
        }
    }
    ship->timer[T_MAINTENANCE] = 300;
    send_message_to_debug_char("Could not find the current target\n");
    return false;
}

bool NPCShipAI::find_new_target()
{
    for (int i = 0; i < contacts_count; i++) 
    {
        if (is_valid_target(contacts[i].ship))
        {
            ship->target = contacts[i].ship;
            update_target(i);
            ship->timer[T_MAINTENANCE] = 0;
            send_message_to_debug_char("Found new target: %s\n", contacts[i].ship->id);
            return true;
        }
    }
    /*if (ship->timer[T_MAINTENANCE] == 0)
    {
        ship->timer[T_MAINTENANCE] = 300;
        send_message_to_debug_char("Could not find a new target\n");
    }*/
    return false;
}
void NPCShipAI::update_target(int i) // index in contacts
{
    t_bearing = contacts[i].bearing;
    t_range = contacts[i].range;
    t_x = contacts[i].x;
    t_y = contacts[i].y;
    t_arc = get_arc(ship->heading, t_bearing);
    s_bearing = t_bearing + 180;
    normalize_direction(s_bearing);
    s_arc = get_arc(ship->target->heading, s_bearing);
}


bool NPCShipAI::is_valid_target(P_ship tar)
{
    if (SHIPISDOCKED(tar))
        return false;
    if (SHIPSINKING(tar))
        return false;
    return (tar->race == GOODIESHIP || tar->race == EVILSHIP);
}


bool NPCShipAI::check_ammo()
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

bool NPCShipAI::check_boarding_conditions()
{
    if (!is_boardable(ship->target))
        return false;
    if (ship->speed > BOARDING_SPEED)
        return false;
    if (ship->x != t_x || ship->y != t_y)
        return false;
    return true;
}

void NPCShipAI::board_target()
{
    did_board = true;
    send_message_to_debug_char("=============BOARDING TARGET=============\n");
}

bool NPCShipAI::charge_target(bool for_boarding)
{
    new_heading = t_bearing;
    if (for_boarding)
    {
        if (t_range < 3.0)
            speed_restriction = 40;
        if (t_range < 1.0)
            speed_restriction = BOARDING_SPEED;
    }
    set_new_dir();
    return true;
}

bool NPCShipAI::chase()
{
    if (SHIPIMMOBILE(ship)) return false;
    new_heading = calc_intercept_heading (t_bearing, ship->target->heading);
    if (check_dir_for_land_from(ship->x, ship->y, new_heading, 5))
        new_heading = t_bearing;
    send_message_to_debug_char("Intercepting: ");
    return true;
}

bool NPCShipAI::go_around_land()
{
    if (SHIPIMMOBILE(ship)) return false;

    vector<int> route;
    if (!dijkstra(ship->location, ship->target->location, valid_ship_edge, route))
    {
        send_message_to_debug_char("Going around land failed!\n");
        return false;
    }
    if (route.size() == 0)
    {
        send_message_to_debug_char("\nempty route found!");
        send_message_to_debug_char("Going around land failed!\n");
        return false;
    }
    switch (route[0])
    {
    case 0: new_heading = 0; break;
    case 1: new_heading = 90; break;
    case 2: new_heading = 180; break;
    case 3: new_heading = 270; break;
    default: 
        {
        send_message_to_debug_char("Going around land failed!\n");
        return false;
        }
    };
    send_message_to_debug_char("Going around land: ");
    set_new_dir();
    return true;
}

void NPCShipAI::run_away()
{
    // TODO: smarter choice?
    new_heading = t_bearing + 180;
    if (check_dir_for_land_from(ship->x, ship->y, new_heading, 10))
    {
        bool found = false;
        for (int i = 0; i < 5; i++)
        {
            if (!check_dir_for_land_from(ship->x, ship->y, new_heading + i * 30, 10))
            {
                new_heading = new_heading + i * 30;
                found = true;
                break;
            }
            else if (!check_dir_for_land_from(ship->x, ship->y, new_heading - i * 30, 10))
            {
                new_heading = new_heading - i * 30;
                found = true;
                break;
            }
        }
        if (!found && !check_dir_for_land_from(ship->x, ship->y, t_bearing, 10))
            new_heading = t_bearing;
    }
    send_message_to_debug_char("Running away: ");
    set_new_dir();
}

int NPCShipAI::calc_intercept_heading(int tb, int th)
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

bool NPCShipAI::worth_ramming()
{
    if ((float)SHIPHULLWEIGHT(ship->target) / (float)SHIPHULLWEIGHT(ship) >= 1.5)
        return false;

    if (ship->armor[FORE] == 0 || 
        ship->armor[STARBOARD] == 0 || 
        ship->armor[PORT] == 0 || 
        ship->maxarmor[FORE] / ship->armor[FORE] >= 2 ||
        ship->maxarmor[STARBOARD] / ship->armor[STARBOARD] >= 2 ||
        ship->maxarmor[PORT] / ship->armor[PORT] >= 2)
    {
        return false;
    }

    return true;
}

bool NPCShipAI::check_ram()
{
    if (ship->timer[T_RAM] != 0) 
        return false;
    if (t_range >= 1.0)
        return false;
    if (ship->speed <= BOARDING_SPEED)
        return false;
    if (!check_ram_arc(ship->heading, t_bearing, 120))
        return false;

    return true;
}

void NPCShipAI::ram_target()
{
    send_message_to_debug_char("\nRamming!\n");
    if (try_ram_ship(ship, ship->target, t_bearing))
    {
        //if (SHIPIMMOBILE(ship->target) && !did_board)
        if (is_boardable(ship->target) && !did_board)
        {
            board_target();
        }
    }
}


/////////////////////////
// BASIC COMBAT /////////
/////////////////////////


void NPCShipAI::basic_combat_maneuver()
{
    new_heading = ship->heading;

    b_check_weapons();

    if (ship->target->armor[s_arc] == 0 && ship->target->internal[s_arc] == 0)
    { // aha, he is immobile and faces us with destroyed side, must find way around him
        b_circle_around_arc(s_arc);
    }
    else
    {
        if (b_turn_active_weapon()) // trying to turn a weapon that is ready/almost ready to fire and within good range already
        {
        }
        else if(too_far && chase()) // if there is weapon ready to fire, but we are not in a good range
        {
        }
        else if(too_close && b_make_distance(too_close)) // if there is weapon ready to fire, but we are too close, try breaking distance
        {
            send_message_to_debug_char("Breaking distance: ");
        }
        else if (b_turn_reloading_weapon()) // trying to turn: a weapon that is closest to ready and within good range already
        {
        }
        else if (chase()) // chasing by default
        {
        }
        else // TODO
            return;
    }
    set_new_dir();
}

void NPCShipAI::b_attack()
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
    if (is_multi_target)
    {
        for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
        {
            if (ship->slot[w_num].type == SLOT_WEAPON) 
            {
                if (!weapon_ready_to_fire(ship, w_num))
                    continue;
                int w_index = ship->slot[w_num].index;
                for (int i = 0; i < contacts_count; i++) 
                {
                    if (contacts[i].ship != ship->target && is_valid_target(contacts[i].ship))
                    {
                        int t_a = get_arc(ship->heading, contacts[i].bearing);
                        if (ship->slot[w_num].position == t_a &&
                            contacts[i].range > (float)weapon_data[w_index].min_range && 
                            contacts[i].range < (float)weapon_data[w_index].max_range &&
                            ship->guncrew.stamina > weapon_data[w_index].reload_stamina)
                        {
                            P_ship cur_t = ship->target;
                            ship->target = contacts[i].ship;
                            fire_weapon(ship, debug_char, w_num);
                            ship->target = cur_t;
                        }
                    }
                }
            }
        }
    }
}

void NPCShipAI::b_check_weapons()
{
    too_close = 0;
    too_far = false;
    for (int i = 0; i < 4; i++) active_arc[i] = 1000;
    for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
    {
        if (ship->slot[w_num].type == SLOT_WEAPON) 
        {
            if (!weapon_ok(ship, w_num))
                continue;
            int w_index = ship->slot[w_num].index;

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

bool NPCShipAI::b_circle_around_arc(int arc)
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
            land_dist[i] = check_dir_for_land_from(t_x, t_y, ship->target->heading + get_arc_main_bearing(i), 7);
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
    int dst_loc = get_room_in_direction_from(t_x, t_y, dst_dir, max_dist);

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


bool NPCShipAI::b_turn_active_weapon()
{
    if (ship->timer[T_RAM_WEAPONS] > 0) 
        return false;

    int arc_priority[4];
    b_set_arc_priority(t_bearing, t_arc, arc_priority);
    for (int i = 0; i < 4; i++)
    {
        if (active_arc[arc_priority[i]] < 4) // there is weapon ready or almost ready to fire, turning
        {
            int n_h = t_bearing - get_arc_main_bearing(arc_priority[i]);
            if (is_heavy_ship && arc_priority[i] == SLOT_REAR &&  ((n_h - ship->heading) > 60 || (n_h - ship->heading) < -60))
                continue; // not turning heavy ship's rear (TODO: check if rear is the only remaining side)
            new_heading = n_h;
            send_message_to_debug_char("Turning active weapon arc %s: ", get_arc_name(arc_priority[i]));
            return true;
        }
    }
    return false;
}

bool NPCShipAI::b_turn_reloading_weapon()
{
    int best_arc = -1, best_time = 1000;
    for (int i = 0; i < 4; i++)
    {
        if (active_arc[i] < best_time)
        {
            int n_h = t_bearing - get_arc_main_bearing(i);
            if (is_heavy_ship && i == SLOT_REAR && ((n_h - ship->heading) > 60 || (n_h - ship->heading) < -60))
                continue; // not turning heavy ship's rear (TODO: check if rear is the only remaining side)
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


bool NPCShipAI::b_make_distance(float distance)
{
    if (SHIPIMMOBILE(ship)) return false;
    if (t_range > distance) return true;

    
    float rad = (float) ((float) ((t_bearing - 180) - ship->target->heading) * M_PI / 180.000);

    float side_dist_full = sin(acos(cos(rad) * t_range / distance)) * distance; // always positive
    float side_dist_self = sin(rad);
    float star_dist = side_dist_full - side_dist_self;
    float port_dist = side_dist_full + side_dist_self;

    float dire_dist_full = cos(asin(sin(rad) * t_range / distance)) * distance; // always positive
    float dire_dist_self = cos(rad);
    float fore_dist = dire_dist_full - dire_dist_self;
    float rear_dist = dire_dist_full + dire_dist_self;

    if (star_dist < port_dist)
    {
        if (!check_dir_for_land_from(ship->x, ship->y, ship->target->heading + 90, star_dist + 1))
        {
            new_heading = ship->target->heading + 90;
            return true;
        }
        else if (!check_dir_for_land_from(ship->x, ship->y, ship->target->heading - 90, port_dist + 1))
        {
            new_heading = ship->target->heading - 90;
            return true;
        }
    }
    else
    {
        if (!check_dir_for_land_from(ship->x, ship->y, ship->target->heading - 90, port_dist + 1))
        {
            new_heading = ship->target->heading - 90;
            return true;
        }
        else if (!check_dir_for_land_from(ship->x, ship->y, ship->target->heading + 90, star_dist + 1))
        {
            new_heading = ship->target->heading + 90;
            return true;
        }
    }

    if (fore_dist < rear_dist)
    {
        if (!check_dir_for_land_from(ship->x, ship->y, ship->target->heading, fore_dist + 1))
        {
            new_heading = ship->target->heading;
            return true;
        }
        else if (!check_dir_for_land_from(ship->x, ship->y, ship->target->heading - 180, rear_dist + 1))
        {
            new_heading = ship->target->heading - 180;
            return true;
        }
    }
    else
    {
        if (!check_dir_for_land_from(ship->x, ship->y, ship->target->heading - 180, rear_dist + 1))
        {
            new_heading = ship->target->heading - 180;
            return true;
        }
        else if (!check_dir_for_land_from(ship->x, ship->y, ship->target->heading, fore_dist + 1))
        {
            new_heading = ship->target->heading;
            return true;
        }
    }

    send_message_to_debug_char("Failed breaking distance\n");
    return false;
}

void NPCShipAI::set_new_dir()
{
    normalize_direction(new_heading);

    float cur_land_dist = calc_land_dist(ship->x, ship->y, ship->heading, 2.0);
    float new_land_dist = calc_land_dist(ship->x, ship->y, new_heading, 2.0);
    if (new_land_dist < cur_land_dist)
        cur_land_dist = new_land_dist;
    int safe_speed = ship->get_maxspeed();
    if (cur_land_dist < 0.1)
        safe_speed = 1;
    else if (cur_land_dist < 0.3)
        safe_speed = 5;
    else if (cur_land_dist < 0.5)
        safe_speed = 10;
    else if (cur_land_dist < 1.0)
        safe_speed = 20;
    else if (cur_land_dist < 2.0)
        safe_speed = 40;

    ship->setheading = new_heading;
    int delta = (int)abs(ship->heading - new_heading);
    if (delta > 180) delta = 360 - delta;
    if (delta < 60)
        ship->setspeed = ship->get_maxspeed();
    else if (delta < 90)
        ship->setspeed = ship->get_maxspeed() / 2;
    else
        ship->setspeed = MIN(10, ship->get_maxspeed());


    if (ship->setspeed > safe_speed)
        ship->setspeed = safe_speed;
    if (speed_restriction != -1 && ship->setspeed > speed_restriction)
        ship->setspeed = speed_restriction;

    send_message_to_debug_char("heading=%d, speed=%d\n", ship->setheading, ship->setspeed);
}




void NPCShipAI::b_set_arc_priority(int current_bearing, int current_arc, int* arc_priority)
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

void NPCShipAI::immobile_maneuver()
{
// TODO
}






















/////////////////////////
// ADVANCED COMBAT //////
/////////////////////////

/// TODO: update previous heading!!!


void NPCShipAI::advanced_combat_maneuver()
{
    if (t_range > 10) // TODO: more smart decision
    { // no point to maneuver around yet
        chase();
    }
    else
    {
        a_update_side_props();
        a_predict_target(3);
        a_update_target_side_props();
        a_choose_target_side();
        send_message_to_debug_char("t_range=(%5.2f-%5.2f),t_ldist={%5.2f,%5.2f,%5.2f,%5.2f},curr_angle={%d,%d,%d,%d},proj_angle={%d,%d,%d,%d},curr_pos={%5.2f,%5.2f},proj_pos={%5.2f,%5.2f},proj_r=%5.2f,proj_sb=%d,proj_tb=%d,hd_change=%5.2f\n", t_min_range, t_max_range, tside_props[0].land_dist, tside_props[1].land_dist, tside_props[2].land_dist, tside_props[3].land_dist, curr_angle[0], curr_angle[1], curr_angle[2], curr_angle[3], proj_angle[0], proj_angle[1], proj_angle[2], proj_angle[3], curr_x, curr_y, proj_x, proj_y, proj_range, proj_sb, proj_tb, hd_change);

        a_calc_rotations();
        a_choose_rotation();
        send_message_to_debug_char("Circling: tar_side=%s(%c), wpn_side=%s, rot=%s,", get_arc_name(target_side), within_target_side ? 'y' : 'n', get_arc_name(chosen_side), (chosen_rot == 1) ? "direct" : ((chosen_rot == -1) ? "counter" : "unknown") );
        if (!a_immediate_turn())
            a_choose_dest_point();

    
    }
    set_new_dir();
}


void NPCShipAI::a_attack()
{
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
                int hit_arc = weapon_data[w_index].hit_arc;
                if (w_index == W_MINDBLAST || w_index == W_FRAG_CAN)
                    hit_arc = 360; // doesnt matter where to fire from
                int arc_width = get_arc_width(target_side);
                int min_intersect = MIN(MIN(hit_arc, (hit_arc / 2 + 10)), arc_width / 2);
                int rbearing = s_bearing - ship->target->heading; // how target sees you, relatively to direction
                normalize_direction(rbearing);

                // TODO: do it more correctly, does not support hit arc close to 360 right
                int arc_center = get_arc_main_bearing(target_side);
                if (rbearing > 180 && arc_center == 0) arc_center = 360;
                int arc_cw = arc_center + arc_width / 2;
                int arc_ccw = arc_center - arc_width / 2;
                int hit_cw = rbearing + hit_arc / 2;
                int hit_ccw = rbearing - hit_arc / 2;
                int intersect = arc_width;
                if (hit_ccw > arc_ccw) intersect -= (hit_ccw - arc_ccw);
                if (hit_cw < arc_cw) intersect -= (arc_cw - hit_cw);
                if (intersect < 0) intersect = 0;
                if (intersect >= min_intersect)
                    fire_weapon(ship, debug_char, w_num);
            }
        }
    }
}


void NPCShipAI::a_predict_target(int steps)
{
    float hd = ship->target->heading;
    curr_x = (float)t_x + (ship->target->x - 50.0);
    curr_y = (float)t_y + (ship->target->y - 50.0);
    curr_angle[SLOT_FORE] = (int)hd;
    curr_angle[SLOT_STAR] = (int)hd + 90;
    curr_angle[SLOT_PORT] = (int)hd - 90;
    curr_angle[SLOT_REAR] = (int)hd + 180;
    for (int i = 0; i < 4; i++) normalize_direction(curr_angle[i]);

    hd_change = hd - prev_hd;
    prev_hd = hd;
    if (hd_change < -180.0) hd_change += 360.0;
    if (hd_change > 180.0) hd_change -= 360.0;

    float x = curr_x, y = curr_y;
    for (int step = 0; step < steps; step++)
    {// TODO: acceleration also?
        hd += hd_change;
        float rad = (float) (hd * M_PI / 180.000);
        x += (float) ((float) ship->target->speed * sin(rad)) / 150.000;
        y += (float) ((float) ship->target->speed * cos(rad)) / 150.000;
    }

    proj_x = x;
    proj_y = y;

    proj_angle[SLOT_FORE] = (int)hd;
    proj_angle[SLOT_STAR] = (int)hd + 90;
    proj_angle[SLOT_PORT] = (int)hd - 90;
    proj_angle[SLOT_REAR] = (int)hd + 180;
    for (int i = 0; i < 4; i++) normalize_direction(proj_angle[i]);

    float proj_delta_x = ship->x - proj_x;
    float proj_delta_y = ship->y - proj_y;
    proj_range = sqrt(proj_delta_x * proj_delta_x + proj_delta_y * proj_delta_y);
    proj_sb = (int)(acos(proj_delta_y / proj_range) / M_PI * 180.0);
    if (proj_delta_x < 0) proj_sb = 360 - proj_sb;
    proj_tb = proj_sb - 180;
    if (proj_tb < 0) proj_tb += 360;

}


void NPCShipAI::a_update_side_props()
{
    min_range_total = INT_MAX;
    for (int i = 0; i < 4; i++)
    {
        side_props[i].ready_timer = INT_MAX;
        side_props[i].damage_ready = 0;
        side_props[i].max_range = INT_MAX;
        side_props[i].good_range = INT_MAX;
//        side_props[i].min_range = INT_MAX;
        side_props[i].min_range = INT_MAX;
    }
    for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
    {
        if (ship->slot[w_num].type == SLOT_WEAPON) 
        {
            if (!weapon_ok(ship, w_num))
                continue;

            int w_index = ship->slot[w_num].index;
            int pos = ship->slot[w_num].position;

            if (weapon_ready_to_fire(ship, w_num))
            {
                side_props[pos].ready_timer = 0;
                if (weapon_data[w_index].min_range == 0) // only counting ballista-types
                    side_props[pos].damage_ready += weapon_data[w_index].average_hull_damage();
                if (side_props[pos].max_range > (float)weapon_data[w_index].max_range)
                    side_props[pos].max_range = (float)weapon_data[w_index].max_range;
                if (side_props[pos].good_range > (float)weapon_data[w_index].max_range * 0.3)
                    side_props[pos].good_range = (float)weapon_data[w_index].max_range * 0.3;
                if (side_props[pos].min_range > weapon_data[w_index].min_range)
                    side_props[pos].min_range = weapon_data[w_index].min_range;
                if (min_range_total > weapon_data[w_index].min_range)
                    min_range_total = weapon_data[w_index].min_range;
            }
            else 
            {
                if(side_props[pos].ready_timer > ship->slot[w_num].timer)
                    side_props[pos].ready_timer = ship->slot[w_num].timer;
                if (side_props[pos].good_range > (float)weapon_data[w_index].max_range * 0.3) // still want to know it to get into right distance for reloading guns if none ready
                    side_props[pos].good_range = (float)weapon_data[w_index].max_range * 0.3;
                //if (weapon_data[w_index].min_range == 0 && ship->slot[w_num].timer < 15) // if weapon is more or less close to reload, account for it damage partially
                //    side_props[pos].damage_ready += weapon_data[w_index].average_hull_damage() * (float)ship->slot[w_num].timer / 30.0;
            }
        }
    }
    for (int i = 0; i < 4; i++)
    {
        if (side_props[i].good_range == INT_MAX)
            side_props[i].good_range = 0;
        if (side_props[i].min_range == INT_MAX)
            side_props[i].min_range = 0;
    }
}

void NPCShipAI::a_update_target_side_props()
{

    t_max_range = INT_MAX;
    t_min_range = INT_MAX;
    for (int i = 0; i < 4; i++)
    {
        tside_props[i].ready_timer = INT_MAX;
        tside_props[i].damage_ready = 0;
        tside_props[i].max_range = INT_MAX;
        tside_props[i].min_range = INT_MAX;
        tside_props[i].land_dist = calc_land_dist(proj_x, proj_y, proj_angle[i], 10);
    }

    P_ship target = ship->target;
    if (!target) return;
    
    for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
    {
        if (target->slot[w_num].type == SLOT_WEAPON) 
        {
            if (!weapon_ok(target, w_num))
                continue;

            int w_index = target->slot[w_num].index;
            int pos = target->slot[w_num].position;

            if (weapon_ready_to_fire(target, w_num))
            {
                tside_props[pos].damage_ready += weapon_data[w_index].average_hull_damage();
                if (weapon_data[w_index].min_range > 0)
                {
                    if (tside_props[pos].min_range > weapon_data[w_index].min_range)
                        tside_props[pos].min_range = weapon_data[w_index].min_range;
                    if (t_min_range < weapon_data[w_index].min_range)
                        t_min_range = weapon_data[w_index].min_range;

                }
                if (tside_props[pos].max_range > (float)weapon_data[w_index].max_range)
                    tside_props[pos].max_range = (float)weapon_data[w_index].max_range;
                tside_props[pos].ready_timer = 0;
            }
            else 
            {
                if(tside_props[pos].ready_timer > target->slot[w_num].timer)
                    tside_props[pos].ready_timer = target->slot[w_num].timer;
                if (tside_props[pos].ready_timer < 6)
                    tside_props[pos].ready_timer = 0; // counting weapons that are almost ready as ready
            }
            if (weapon_data[w_index].min_range > 0 && t_min_range > weapon_data[w_index].min_range)
                t_min_range = weapon_data[w_index].min_range;
            if (t_max_range > weapon_data[w_index].max_range)
                t_max_range = weapon_data[w_index].max_range;
        }
    }
    if (t_min_range == INT_MAX)
        t_min_range = 0;
}

void NPCShipAI::a_choose_target_side() // TODO: choose another one if too close to land
{
    P_ship target = ship->target;
    if (!target) return;

    int min_left = INT_MAX;
    for (int i = 0; i < 4; i++)
    {
        if (tside_props[i].land_dist > min_range_total)
        {
            int left = target->armor[i] + target->internal[i];
            if (left > 0 && min_left > left)
            {
                min_left = left;
                target_side = i;
            }
        }
    }
}



void NPCShipAI::a_calc_rotations()
{
    int delta = (target_side == SLOT_FORE || target_side == SLOT_REAR) ? 30 : 40;
    int cw = proj_angle[target_side] + delta;
    if (cw > 360) cw -= 360;
    int ccw = proj_angle[target_side] - delta;
    if (ccw < 0) ccw += 360;

    if ((proj_sb <= cw  && proj_sb >= ccw) || ((cw < ccw) && (proj_sb <= cw || proj_sb >=ccw)))
        within_target_side = true;
    else
        within_target_side = false;

    cw_cw = cw - proj_sb;
    if (cw_cw < 0) cw_cw += 360;
    cw_ccw = ccw - proj_sb;
    if (cw_ccw < 0) cw_ccw += 360;
    ccw_cw = proj_sb - cw;
    if (ccw_cw < 0) ccw_cw += 360;
    ccw_ccw = proj_sb - ccw;
    if (ccw_ccw < 0) ccw_ccw += 360;
    send_message_to_debug_char("cw_cw=%d, cw_ccw=%d, ccw_cw=%d, ccw_ccw=%d\n", cw_cw, cw_ccw, ccw_cw, ccw_ccw);
}

void NPCShipAI::a_choose_rotation()
{
    int proj_tb_rel = proj_tb - ship->heading;
    if (proj_tb_rel < 0) proj_tb_rel += 360;
    int star_count, star_dir;
    int port_count, port_dir;
    int cnt_dir;

    if (proj_tb < 180) // starboard
    {
        if (cw_ccw > ccw_ccw)
        {
            star_count = ccw_ccw; // double turn
            star_dir = -1;
        }
        else
        {
            star_count = cw_ccw;  // no turn
            star_dir = 1;
        }

        if (ccw_ccw > cw_cw)
        {
            port_count = cw_cw; // post-turn
            port_dir = 1;
        }
        else
        {
            port_count = ccw_ccw; // pre-turn
            port_dir = -1;
        }

        if (ccw_ccw > cw_ccw)
            cnt_dir = 1; // no turn
        else
            cnt_dir = -1; // pre-turn
    }
    else // port
    {
        if (ccw_cw > cw_cw)
        {
            port_count = cw_cw;  // double turn
            port_dir = 1;
        }
        else
        {
            port_count = ccw_cw; // no turn
            port_dir = -1;
        }

        if (ccw_ccw > cw_cw)
        {
            star_count = cw_cw; // pre-turn
            star_dir = 1;
        }
        else
        {
            star_count = ccw_ccw; // post-turn
            star_dir = -1;
        }

        if (ccw_cw > cw_cw)
            cnt_dir = 1; // pre-turn
        else
            cnt_dir = -1; // no turn
    }

    bool side_ok[4];
    for (int i = 0; i < 4; i++)
    { // ready to fire and target side is not too close to land for this arc weapons
        side_ok[i] = (side_props[i].ready_timer == 0) && (side_props[i].min_range < tside_props[target_side].land_dist);
    }

    send_message_to_debug_char("pc=%d, pd=%d, sc=%d, sd=%d, cd=%d, sides=%c%c%c%c ", port_count, port_dir, star_count, star_dir, cnt_dir, side_ok[0]?'1':'0', side_ok[1]?'1':'0', side_ok[2]?'1':'0', side_ok[3]?'1':'0');

    // todo: take damage into account
    if (side_ok[SLOT_PORT] && (port_count >  star_count || !side_ok[SLOT_STAR]))
    {
        chosen_side = SLOT_PORT;
        chosen_rot = port_dir;
        send_message_to_debug_char("choice 1\n");
        return;
    }
    if (side_ok[SLOT_STAR] && (star_count >= port_count || !side_ok[SLOT_PORT]))
    {
        chosen_side = SLOT_STAR;
        chosen_rot = star_dir;
        send_message_to_debug_char("choice 2\n");
        return;
    }


    // TODO: if we should stay close at all times, skip this one
    if (side_ok[SLOT_FORE] || side_ok[SLOT_REAR])
    {
        chosen_rot = cnt_dir;
        if (side_ok[SLOT_REAR])
            chosen_side = SLOT_REAR;
        else if (side_ok[SLOT_FORE])
            chosen_side = SLOT_FORE; 
        send_message_to_debug_char("choice 3\n");
        return;
    }

    if (side_props[SLOT_PORT].ready_timer != INT_MAX || side_props[SLOT_STAR].ready_timer != INT_MAX)
    {
        if (side_props[SLOT_PORT].ready_timer > side_props[SLOT_STAR].ready_timer)
        {
            chosen_side = SLOT_STAR;
            chosen_rot = star_dir;
        }
        else
        {
            chosen_side = SLOT_PORT;
            chosen_rot = star_dir;
        }
        send_message_to_debug_char("choice 4\n");
        return;
    }

    if (side_props[SLOT_REAR].ready_timer != INT_MAX || side_props[SLOT_FORE].ready_timer != INT_MAX)
    {
        chosen_rot = cnt_dir;
        if (side_props[SLOT_FORE].ready_timer > side_props[SLOT_REAR].ready_timer)
            chosen_side = SLOT_REAR;
        else
            chosen_side = SLOT_FORE;
        send_message_to_debug_char("choice 5\n");
        return;
    }

        send_message_to_debug_char("no choice!\n");
    // todo: no weapons?
}

bool NPCShipAI::a_immediate_turn()
{
    int delta = (target_side == SLOT_FORE || target_side == SLOT_REAR) ? 30 : 40;
    int cw = curr_angle[target_side] + delta;
    if (cw > 360) cw -= 360;
    int ccw = curr_angle[target_side] - delta;
    if (ccw < 0) ccw += 360;

    bool within_target_side = false;
    if ((s_bearing <= cw  && s_bearing >= ccw) || ((cw < ccw) && (s_bearing <= cw || s_bearing >=ccw)))
        within_target_side = true;

    if (within_target_side)
    {
        if ((chosen_side == SLOT_FORE && side_props[SLOT_FORE].ready_timer == 0))
        {
            if (t_range >= side_props[SLOT_FORE].min_range + 2) // have a little distance reserve to turn
            {
                new_heading = t_bearing; // turning toward target to fire fore
                send_message_to_debug_char(" immediate turn to fire fore!\n");
                return true;
            }
        }
        if ((chosen_side == SLOT_REAR && side_props[SLOT_REAR].ready_timer == 0) && side_props[SLOT_REAR].max_range > proj_range)
        {
           new_heading = s_bearing; // turning from target to fire rear
           send_message_to_debug_char(" immediate turn to fire rear!\n");
           return true;
        }
        if (chosen_side == SLOT_PORT && side_props[SLOT_PORT].ready_timer == 0 && side_props[SLOT_PORT].max_range > proj_range)
        {
           new_heading = t_bearing + 90;
           send_message_to_debug_char(" immediate turn to fire port!\n");
           return true;
        }
        if (chosen_side == SLOT_STAR && side_props[SLOT_STAR].ready_timer == 0 && side_props[SLOT_STAR].max_range > proj_range)
        {
           new_heading = t_bearing - 90;
           send_message_to_debug_char(" immediate turn to fire starboard!\n");
           return true;
        }
    }
    return false;
}

void NPCShipAI::a_choose_dest_point()
{
    int dest_angle = proj_sb + ((chosen_rot == 1) ? 30 : -30);
    normalize_direction(dest_angle);
    float rad = ((float)dest_angle * M_PI / 180.000);

    // todo: base it on next arc's properties, not general
    float chosen_range = side_props[chosen_side].good_range;
    if (side_props[chosen_side].min_range > 0)
    {
        if (chosen_side == FORE)
            chosen_range = side_props[chosen_side].min_range + 3;
        else if (chosen_side == REAR)
            chosen_range = side_props[chosen_side].min_range;
        else
            chosen_range = side_props[chosen_side].min_range + 1;
    }
    else
    {
        if (t_min_range && t_min_range - 1 <= chosen_range) // todo: priority between catapults and short-ranged somehow
        {
            chosen_range = t_min_range - 1;
        }
        if (t_max_range < 6 && t_max_range + 1 <= side_props[chosen_side].max_range) // short-ranged take priority for now (heavies)
        {
            chosen_range = t_max_range + 1;
        }
    }

    float land_dist = calc_land_dist(proj_x, proj_y, dest_angle, chosen_range);

    if (land_dist < 1)
        chosen_range = land_dist / 2;
    else if (land_dist < chosen_range)
        chosen_range = land_dist - 0.5;

    float dest_x = proj_x + sin(rad) * chosen_range;
    float dest_y = proj_y + cos(rad) * chosen_range;

    if (dest_y == ship->y)
    {
        if (dest_x >= ship->x)
            new_heading = 90;
        else
            new_heading = 270;
    }
    else
    {
        new_heading = (int)(atan((dest_x - ship->x) / (dest_y - ship->y)) / M_PI * 180.0);
        if (dest_y < ship->y)
            new_heading += 180;
    }
    normalize_direction(new_heading);
    // todo: check for land to dest point and switch to basic ai
    send_message_to_debug_char(" range=%5.2f\n", chosen_range);
}







//////////////////////////////////
// UTILITIES /////////////////////
//////////////////////////////////


bool NPCShipAI::inside_map(float x, float y)
{
    if ((int)x < 0 && (int)x > 100) return false;
    if ((int)y < 0 && (int)y > 100) return false;
    return true;
}

// returns exact dir or max_range if not found
float NPCShipAI::calc_land_dist(float x, float y, float dir, float max_range)
{
    float loc_range;
    float rad = dir * M_PI / 180.000;
    float dir_cos = cos(rad);
    float dir_sin = sin(rad);
    float range = 0;
    float next_x, next_y;


    //send_message_to_debug_char("Calculating land dist from (%5.2f, %5.2f) toward %4.0f within %5.2f:", x, y, dir, max_range);
    //send_message_to_debug_char("\ndsin=%5.2f,dcos=%5.2f", dir_sin, dir_cos);
    while (range < max_range)
    {
        if (x < 1 || y < 1 || x >= 100 || y >= 100)
            break;

        int y_to_check = (int) y;
        if (dir_cos > 0)
        {
            next_y = ceil(y);
            if (next_y == y) next_y = y + 1;
        }
        else
        {
            next_y = floor(y);
            if (next_y == y) 
            {
                next_y = y - 1; 
                y_to_check--;
            }
        }

        int x_to_check = (int) x;
        if (dir_sin > 0)
        {
            next_x = ceil(x);
            if (next_x == x) next_x = x + 1;
        }
        else
        {
            next_x = floor(x);
            if (next_x == x) 
            {
                next_x = x - 1;
                x_to_check--;;
            }
        }
        //send_message_to_debug_char("\nnx=%5.2f,ny=%5.2f,", next_x, next_y);
        //send_message_to_debug_char("cx=%d,cy=%d,", x_to_check, y_to_check);

        if (world[tactical_map[x_to_check][100 - y_to_check].rroom].sector_type != SECT_OCEAN && !IS_SET(ship->flags, AIR)) 
        { // next room is a land
            //send_message_to_debug_char(" %5.2f\n", range);
            return range;
        }

        float delta_y = next_y - y;
        float delta_x = next_x - x;
        //send_message_to_debug_char("dx=%5.2f,dy=%5.2f,", delta_x, delta_y);

        if (dir_cos == 0)
        {
            //loc_range = abs(delta_x);
            loc_range = delta_x;
            if (loc_range < 0.0) loc_range = loc_range * -1.0;
            x = next_x;
            // y doesnt change
        }
        else
        {
            float r1 = dir_sin / dir_cos;
            if (r1 < 0.0) r1 = r1 * -1.0;
            float r2 = delta_x / delta_y;
            if (r2 < 0.0) r2 = r2 * -1.0;
            //if (abs(dir_sin / dir_cos) >  abs(delta_x / delta_y))  // w/e
            if (r1 >  r2)  // w/e
            {
                loc_range = delta_x / dir_sin;
                x = next_x;
                y = y + loc_range * dir_cos;
            }
            else // n/s
            {
                loc_range = delta_y / dir_cos;
                x = x + loc_range * dir_sin;
                y = next_y;
            }
        }
        range += loc_range;
        //send_message_to_debug_char("x=%5.2f,y=%5.2f, lr=%5.2f, r=%5.2f", x, y, loc_range, range);
    }
    //send_message_to_debug_char(" none\n");
    return max_range;
}

// returns the distance to land or 0 if none
int NPCShipAI::check_dir_for_land_from(float cur_x, float cur_y, int heading, float range)
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


int NPCShipAI::get_room_in_direction_from(float x, float y, int dir, float range)
{
    float rx, ry;
    if (get_coord_in_direction_from(x, y, dir, range, rx, ry))
        return get_room_at(rx, ry);
    return 0;
}

int NPCShipAI::get_room_at(float x, float y)
{
    return tactical_map[(int) x][100 - (int) y].rroom;
}

bool NPCShipAI::get_coord_in_direction_from(float x, float y, int dir, float range, float& rx, float& ry)
{
    float rad = (float) ((float) (dir) * M_PI / 180.000);
    rx = x + sin(rad) * range;
    ry = y + cos(rad) * range;

    if (!inside_map(rx, ry)) 
        return false;
    
    return true;
}

int NPCShipAI::get_arc_main_bearing(int arc)
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

int NPCShipAI::get_arc_width(int arc)
{
    switch (arc)
    {
    case SLOT_FORE: 
    case SLOT_REAR: 
        return 80;
    case SLOT_PORT:
    case SLOT_STAR:
        return 100;
    };
    return 0;
}

void NPCShipAI::normalize_direction(int &dir)
{
    while (dir >= 360) dir = dir - 360;
    while (dir < 0) dir = dir + 360;
}

void NPCShipAI::send_message_to_debug_char(char *fmt, ... )
{
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);

    send_to_char(buf, debug_char);
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

bool is_boardable(P_ship ship)
{
    if (ISWARSHIP(ship) && ship->speed > 0)
        return false;
    if (ship->speed > BOARDING_SPEED)
        return false;
    return true;
}


/////////////////////////
// NPC SETUP ////////////
/////////////////////////






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
    "&+LOl&+we' &+WSk&+wu&+Ll&+Wl N' &+wBo&+Ln&+Wes",
    "&+LThe &+YGo&+yld&+Yen &+LThief",
    "&+cIron &+CCu&+ctla&+Css",
    "&+GJ&+gad&+Ge Dr&+gag&+Gon",
    "&&+WSa&+wlt&+Ly &+yLeu&+Lcro&+ytta",
    "&+LAv&+wer&+Lnu&+Ws &+YR&+ri&+Rs&+rin&+Yg",
};


struct NPCShipSetup
{
    int m_class;
    int level;
    void (*setup)(P_ship);
};

void setup_npc_clipper_01(P_ship ship) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, PORT);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(100, 150);
}
void setup_npc_clipper_02(P_ship ship) // level 0
{
    set_weapon(ship, 0, W_MEDIUM_BAL, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(100, 150);
}
void setup_npc_clipper_03(P_ship ship) // level 0
{
    set_weapon(ship, 0, W_SMALL_BAL, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 3, W_SMALL_BAL, REAR);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(150, 200);
}
void setup_npc_ketch_01(P_ship ship) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(150, 200);
}
void setup_npc_ketch_02(P_ship ship) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(200, 250);
}
void setup_npc_ketch_03(P_ship ship) // level 0
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, REAR);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(200, 250);
}
void setup_npc_caravel_01(P_ship ship) // level 0
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(250, 300);
}
void setup_npc_caravel_02(P_ship ship) // level 0
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 2, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 200000);
    setcrew(ship, gun_crew_list[0], 200000);
    setcrew(ship, repair_crew_list[0], 200000);
    ship->frags = number(250, 300);
}
void setup_npc_caravel_03(P_ship ship) // level 1
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    setcrew(ship, sail_crew_list[0], 600000);
    setcrew(ship, gun_crew_list[0], 600000);
    setcrew(ship, repair_crew_list[0], 600000);
    ship->frags = number(400, 500);
}
void setup_npc_corvette_01(P_ship ship) // level 1
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 2, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 300000);
    setcrew(ship, gun_crew_list[0], 300000);
    setcrew(ship, repair_crew_list[0], 300000);
    ship->frags = number(400, 500);
}
void setup_npc_corvette_02(P_ship ship) // level 1
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 2, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 300000);
    setcrew(ship, gun_crew_list[0], 300000);
    setcrew(ship, repair_crew_list[0], 300000);
    ship->frags = number(400, 500);
}
void setup_npc_corvette_03(P_ship ship) // level 1
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, PORT);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 4, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[0], 300000);
    setcrew(ship, gun_crew_list[0], 300000);
    setcrew(ship, repair_crew_list[0], 300000);
    ship->frags = number(400, 500);
}
void setup_npc_corvette_04(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_SMALL_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, PORT);
    set_weapon(ship, 2, W_SMALL_BAL, PORT);
    set_weapon(ship, 3, W_SMALL_BAL, PORT);
    set_weapon(ship, 4, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 5, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 6, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[1], 800000);
    setcrew(ship, gun_crew_list[2], 800000);
    setcrew(ship, repair_crew_list[1], 800000);
    ship->frags = number(600, 700);
}
void setup_npc_corvette_05(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, PORT);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(600, 700);
}

void setup_npc_corvette_06(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(600, 700);
}

void setup_npc_corvette_07(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 2, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 3, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1800000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 800);
}

void setup_npc_corvette_08(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, REAR);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 4, W_SMALL_BAL, PORT);
    set_weapon(ship, 5, W_SMALL_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 800);
}

void setup_npc_destroyer_01(P_ship ship) // level 1
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 2, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 4, W_SMALL_BAL, PORT);
    set_weapon(ship, 5, W_SMALL_BAL, PORT);
    set_weapon(ship, 6, W_SMALL_BAL, PORT);
    setcrew(ship, sail_crew_list[0], 400000);
    setcrew(ship, gun_crew_list[0], 400000);
    setcrew(ship, repair_crew_list[0], 400000);
    ship->frags = number(600, 700);
}

void setup_npc_destroyer_02(P_ship ship) // level 1
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 2, W_LARGE_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[1], 500000);
    setcrew(ship, gun_crew_list[0], 500000);
    setcrew(ship, repair_crew_list[0], 500000);
    ship->frags = number(600, 700);
}

void setup_npc_destroyer_03(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, FORE);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 4, W_MEDIUM_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 900);
}
void setup_npc_destroyer_04(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    set_weapon(ship, 3, W_LARGE_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 900);
}
void setup_npc_destroyer_05(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 4, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 5, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 6, W_SMALL_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(800, 1000);
}

void setup_npc_destroyer_06(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_MEDIUM_CAT, FORE);
    set_weapon(ship, 1, W_SMALL_CAT, REAR);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 4, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 5, W_MEDIUM_BAL, STARBOARD);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(800, 1000);
}

void setup_npc_destroyer_07(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_LARGE_CAT, FORE);
    set_weapon(ship, 1, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 2, W_MEDIUM_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, STARBOARD);
    set_weapon(ship, 4, W_MEDIUM_BAL, STARBOARD);
    ship->frags = number(400, 500);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(800, 1000);
}

void setup_npc_destroyer_08(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_LARGE_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, PORT);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    set_weapon(ship, 3, W_MEDIUM_BAL, PORT);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(700, 900);
}

void setup_npc_destroyer_09(P_ship ship) // level 2
{
    set_weapon(ship, 0, W_LARGE_CAT, FORE);
    set_weapon(ship, 1, W_LARGE_BAL, PORT);
    set_weapon(ship, 2, W_LARGE_BAL, PORT);
    set_weapon(ship, 3, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 4, W_SMALL_BAL, STARBOARD);
    set_weapon(ship, 5, W_SMALL_BAL, STARBOARD);
    ship->frags = number(500, 600);
    setcrew(ship, sail_crew_list[2], 1500000);
    setcrew(ship, gun_crew_list[3], 1500000);
    setcrew(ship, repair_crew_list[1], 1500000);
    ship->frags = number(800, 1000);
}

void setup_npc_dreadnought_01(P_ship ship) // level 4
{
    set_weapon(ship, 0, W_LONGTOM, FORE);
    set_weapon(ship, 1, W_FRAG_CAN, FORE);
    set_weapon(ship, 2, W_LARGE_CAT, FORE);
    set_weapon(ship, 3, W_HEAVY_BAL, STARBOARD);
    set_weapon(ship, 4, W_HEAVY_BAL, STARBOARD);
    set_weapon(ship, 5, W_HEAVY_BAL, STARBOARD);
    set_weapon(ship, 6, W_HEAVY_BAL, STARBOARD);
    set_weapon(ship, 7, W_LARGE_BAL, PORT);
    set_weapon(ship, 8, W_LARGE_BAL, PORT);
    set_weapon(ship, 9, W_LARGE_BAL, PORT);
    set_weapon(ship,10, W_LARGE_BAL, PORT);
    set_weapon(ship,11, W_LARGE_BAL, PORT);
    set_weapon(ship,12, W_SMALL_CAT, REAR);
    set_weapon(ship,13, W_SMALL_CAT, REAR);
    setcrew(ship, sail_crew_list[3], 7500000);
    setcrew(ship, gun_crew_list[4], 7500000);
    setcrew(ship, repair_crew_list[2], 7500000);
    ship->frags = number(3000, 4000);
}

void setup_npc_dreadnought_02(P_ship ship) // level 4
{
    set_weapon(ship, 0, W_LONGTOM, FORE);
    set_weapon(ship, 1, W_LARGE_CAT, FORE);
    set_weapon(ship, 2, W_LARGE_CAT, FORE);
    set_weapon(ship, 3, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 4, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 5, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 6, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 7, W_LARGE_BAL, STARBOARD);
    set_weapon(ship, 8, W_LARGE_BAL, PORT);
    set_weapon(ship, 9, W_LARGE_BAL, PORT);
    set_weapon(ship,10, W_LARGE_BAL, PORT);
    set_weapon(ship,11, W_LARGE_BAL, PORT);
    set_weapon(ship,12, W_LARGE_BAL, PORT);
    set_weapon(ship,13, W_MINDBLAST, REAR);
    setcrew(ship, sail_crew_list[3], 7500000);
    setcrew(ship, gun_crew_list[4], 7500000);
    setcrew(ship, repair_crew_list[2], 7500000);
    ship->frags = number(3000, 4000);
}


NPCShipSetup npcShipSetup [] = {
    { SH_CLIPPER,     0, &setup_npc_clipper_01 },
    { SH_CLIPPER,     0, &setup_npc_clipper_02 },
    { SH_CLIPPER,     0, &setup_npc_clipper_03 },
    { SH_KETCH,       0, &setup_npc_ketch_01 },
    { SH_KETCH,       0, &setup_npc_ketch_02 },
    { SH_KETCH,       0, &setup_npc_ketch_03 },
    { SH_CARAVEL,     0, &setup_npc_caravel_01 },
    { SH_CARAVEL,     0, &setup_npc_caravel_02 },
    { SH_CARAVEL,     1, &setup_npc_caravel_03 },
    { SH_CORVETTE,    1, &setup_npc_corvette_01 },
    { SH_CORVETTE,    1, &setup_npc_corvette_02 },
    { SH_CORVETTE,    1, &setup_npc_corvette_03 },
    { SH_CORVETTE,    2, &setup_npc_corvette_04 },
    { SH_CORVETTE,    2, &setup_npc_corvette_05 },
    { SH_CORVETTE,    2, &setup_npc_corvette_06 },
    { SH_CORVETTE,    2, &setup_npc_corvette_07 },
    { SH_CORVETTE,    2, &setup_npc_corvette_08 },
    { SH_DESTROYER,   1, &setup_npc_destroyer_01 },
    { SH_DESTROYER,   1, &setup_npc_destroyer_02 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_03 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_04 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_05 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_06 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_07 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_08 },
    { SH_DESTROYER,   2, &setup_npc_destroyer_09 },
    { SH_DREADNOUGHT, 4, &setup_npc_dreadnought_01 },
    { SH_DREADNOUGHT, 4, &setup_npc_dreadnought_02 },
};

P_ship load_npc_ship(int level, int speed, int m_class, int room, P_char ch)
{
    int num = number(0, sizeof(npcShipSetup) / sizeof(NPCShipSetup) - 1);

    int i = 0, ii = 0;
    while (true)
    {
        if ((npcShipSetup[i].m_class == m_class || m_class == -1) && (SHIPTYPESPEED(npcShipSetup[i].m_class) >= speed || speed == -1)&& npcShipSetup[i].level == level)
        {
            if (ii == num)
                break;
            ii++;
        }
        i++;
        if (i == sizeof(npcShipSetup) / sizeof(NPCShipSetup))
        {
            if (ii == 0)
                return 0; // there is no such ship setup in list
            i = 0;
        }
    }

    P_ship ship = newship(npcShipSetup[i].m_class, true);
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

    npcShipSetup[i].setup(ship);

    int name_index = number(0, sizeof(pirateShipNames)/sizeof(char*) - 1);
    nameship(pirateShipNames[name_index], ship);
    assignid(ship, NULL, true);
    ship->race = NPCSHIP;
    ship->npc_ai = new NPCShipAI(ship, ch);
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

bool try_load_pirate_ship(P_ship target)
{
    if (target->m_class != SH_SLOOP && target->m_class != SH_YACHT && ISMERCHANT(target))
    {
        int n = number(0, SHIPHULLWEIGHT(target)) + target->frags;
        int level;
        if (n < 200)
            level = 0;
        else if (n < 800)
            level = 1;
        else
            level = 2;
        return try_load_pirate_ship(target, 0, level);
    }
    return false;
}

bool try_load_pirate_ship(P_ship target, P_char ch, int level)
{
    getmap(target);

    int dir = target->heading + number(-45, 45);
    NPCShipAI::normalize_direction(dir);

    float rad = (float) ((float) (dir) * M_PI / 180.000);
    float ship_x = 50 + sin(rad) * 36;
    float ship_y = 50 + cos(rad) * 36;

    int location = tactical_map[(int) ship_x][100 - (int) ship_y].rroom;

    //char ttt[100];
    //sprintf(ttt, "Location: x=%d, y=%d, loc=%d, sect=%d\r\n", (int)ship_x, (int)ship_y, location, world[location].sector_type);
    //if (ch) send_to_char(ttt, ch);
    
    if (world[location].sector_type != SECT_OCEAN)
    {
        if (ch) send_to_char("Wrong location type\r\n", ch);
        return false;
    }

    P_ship ship = load_npc_ship(level, SHIPTYPESPEED(target->m_class), -1, location, ch);
    if (!ship)
        ship = load_npc_ship(level, 0, -1, location, ch);
    if (!ship)
        return false;

    ship->target = target;
    int ship_heading = dir + 180;
    NPCShipAI::normalize_direction(ship_heading);
    ship->setheading = ship_heading;
    ship->heading = ship_heading;
    int ship_speed = ship->get_maxspeed();
    ship->setspeed = ship_speed;
    ship->speed = ship_speed;
    ship->npc_ai->mode = NPC_AI_CRUISING;

    /////////////
    //ship->npc_ai->advanced = true;
    /////////////

    return true;
}

bool try_unload_pirate_ship(P_ship ship)
{
    if (ship->timer[T_MAINTENANCE] == 1)
    {
        everyone_get_out_newship(ship);
        shipObjHash.erase(ship);
        delete_ship(ship, true);
        return TRUE;
    }
    return FALSE;
}




//&+LCyric's &+RRe&+rven&+Rge


//Cyric's Revenge     (Should be rareload Dreadnaught) as per the legend in Thur Gax.

//&+RCy&+rri&+Lc's Rev&+ren&+Rge


// Problems:
// if you cant hit target for too long in advanced mode, switch to basic
// Take damage_ready into account