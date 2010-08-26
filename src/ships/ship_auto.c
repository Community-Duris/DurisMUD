/*****************************************************
* ship_auto.c
*
* Ship autopilot
*****************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
//#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include "map.h"
#include "ships.h"
#include "ship_auto.h"

extern char buf[MAX_STRING_LENGTH];

//Internal variables
static shipai_data *autopilot;

void initialize_shipai()
{
  autopilot = NULL;
}

int assign_autopilot(P_ship ship)
{
  struct shipai_data *ai;
  int      i;

  if (!ship)
  {
    return FALSE;
  }
  if (ship->autopilot)
  {
    if (IS_SET(ship->autopilot->flags, AIB_ENABLED))
    {
      return FALSE;
    }
    else
    {
      ai = ship->autopilot;
    }
  }
  else
  {
    CREATE(ai, shipai_data, 1, MEM_TAG_SHIPAI);

    ship->autopilot = ai;
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

void clear_autopilot(P_ship ship)
{
    if (ship->autopilot)
        FREE(ship->autopilot);
    ship->autopilot = 0;
}

void stop_autopilot(P_ship ship)
{
    if (IS_SET(ship->autopilot->flags, AIB_ENABLED)) 
    {
        if (IS_SET(ship->autopilot->flags, AIB_AUTOPILOT)) 
        {
            REMOVE_BIT(ship->autopilot->flags, AIB_AUTOPILOT);
            REMOVE_BIT(ship->autopilot->flags, AIB_ENABLED);
            act_to_all_in_ship(ship, "Autopilot stopped.");
        }
    }
}

int engage_autopilot(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    int dir = 0;
    if (is_number(arg1)) 
    {
        dir = atoi(arg1);
        if (dir < 0 || dir > 359) 
        {
            if (ch) send_to_char("0-359 degrees or N E S W!\r\n", ch);
            return TRUE;
        }
    } 
    else 
    {
        if (isname(arg1, "north n"))
            dir = 0;
        else if (isname(arg1, "east e"))
            dir = 90;
        else if (isname(arg1, "south s"))
            dir = 180;
        else if (isname(arg1, "west w"))
            dir = 270;
        else if (isname(arg1, "off"))
        {
            if (ship->autopilot) 
            {
                if (!IS_SET(ship->autopilot->flags, AIB_ENABLED) && !IS_SET(ship->autopilot->flags, AIB_AUTOPILOT)) 
                {
                    if (ch) send_to_char("There IS no active autopilot atm!\r\n", ch);
                    return TRUE;
                } 
                else 
                {
                    REMOVE_BIT(ship->autopilot->flags, AIB_ENABLED);
                    REMOVE_BIT(ship->autopilot->flags, AIB_AUTOPILOT);
                    act_to_all_in_ship(ship, "Autopilot disengaged.");
                    return TRUE;
                }
            } 
            else 
            {
                if (ch) send_to_char("There IS no active autopilot atm!\r\n", ch);
                return TRUE;
            }
        } 
        else 
        {
            if (ch) send_to_char ("Valid syntax is order sail <N/E/S/W/Heading> <number of rooms>\r\n", ch);
            return TRUE;
        }
    }
    int dist = 0;
    if (is_number(arg2)) 
    {
        dist = atoi(arg2);
        if (dist > 35) 
        {
            if (ch) send_to_char("Maximum number of rooms is 35!\r\n", ch);
            return TRUE;
        }
    } 
    else 
    {
        if (ch) send_to_char ("Valid syntax is order sail <N/E/S/W/Heading> <number of rooms>\r\n", ch);
        return TRUE;
    }
    if (ship->autopilot)
        REMOVE_BIT(ship->autopilot->flags, AIB_ENABLED);

    if (!getmap(ship))
      return TRUE;

    assign_autopilot(ship);

    float rad = (float) ((float) dir * M_PI / 180);
    int xdist = (int) (sin(rad) * (dist + 1));
    int ydist = (int) (cos(rad) * (dist + 1));
    ship->autopilot->t_room = tactical_map [(int) (xdist + ship->x)][100 - (int)(ydist + ship->y)].rroom;

    SET_BIT(ship->autopilot->flags, AIB_AUTOPILOT);
    ship->autopilot->mode = AIM_AUTOPILOT;
    act_to_all_in_ship_f(ship, "Autopilot engaged, heading %d for %d rooms. Target room is %d", dir, dist, world[ship->autopilot->t_room].number);
    return TRUE;
}

int shipgroupremove(struct shipai_data *autopilot)
{
  struct shipgroup_data *tmpgroup;
  struct shipgroup_data *tmpgroup2;
  struct shipai_data *leader = NULL;

  if (!autopilot)
  {
    return FALSE;
  }
  if (!autopilot->group)
  {
    return FALSE;
  }

  if (autopilot->group->leader == autopilot)
  {
    if (!autopilot->group->next)
    {
      autopilot->group->leader = NULL;
      autopilot->group->ai = NULL;
      FREE(autopilot->group);
      autopilot->group = NULL;
      return TRUE;
    }
    tmpgroup2 = autopilot->group;
    tmpgroup = autopilot->group->next;
    tmpgroup2->leader = NULL;
    tmpgroup2->ai = NULL;
    FREE(tmpgroup2);
    autopilot->group = NULL;

    while (tmpgroup)
    {
      if (IS_SET(autopilot->flags, AIB_DRONE))
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
      if (IS_SET(autopilot->flags, AIB_MOB))
      {
        if (!leader)
        {
          tmpgroup->leader = autopilot;
          leader = autopilot;
        }
        else
        {
          tmpgroup->leader = leader;
        }
        if (IS_SET(autopilot->flags, AIB_HUNTER))
        {
          autopilot->mode = AIM_SEEK;
        }
        else
        {
          autopilot->mode = AIM_WAIT;
        }
      }
      tmpgroup = tmpgroup->next;
    }
    return TRUE;
  }
  tmpgroup = autopilot->group->leader->group;
  while (tmpgroup)
  {
    if ((tmpgroup = autopilot->group))
    {
      tmpgroup2 = tmpgroup;
      tmpgroup = tmpgroup->next;
      tmpgroup2->leader = NULL;
      tmpgroup2->ai = NULL;
      FREE(tmpgroup2);
      autopilot->group = NULL;
      continue;
    }
    tmpgroup = tmpgroup->next;
  }
  return TRUE;
}

int shipgroupadd(struct shipai_data *autopilot, struct shipgroup_data *group)
{
  struct shipgroup_data *newgroup;
  struct shipgroup_data *tmpgroup;

  if (!autopilot)
  {
    return FALSE;
  }
  if (autopilot->group)
  {
    return FALSE;
  }
  if (!group)
  {
    CREATE(group, shipgroup_data, 1, MEM_TAG_SHIPGRP);

    group->leader = autopilot;
    group->next = NULL;
    group->ai = autopilot;
    autopilot->group = group;
    return TRUE;
  }
  else
  {
    CREATE(newgroup, shipgroup_data, 1, MEM_TAG_SHIPGRP);

    newgroup->leader = group->leader;
    newgroup->ai = autopilot;
    newgroup->next = NULL;
    tmpgroup = group;
    while (tmpgroup->next)
    {
      tmpgroup = tmpgroup->next;
    }
    tmpgroup->next = newgroup;
    autopilot->group = newgroup;
    return TRUE;
  }
}

void announceheading(P_ship ship, int heading)
{
  act_to_all_in_ship_f(ship, "AI:Heading changed to %d", heading);
}

void announcespeed(P_ship ship, int speed)
{
  act_to_all_in_ship_f(ship, "AI:Speed changed to %d", speed);
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

void autopilot_activity(P_ship ship)
{
  struct shipai_data *ai;
  int      i, j, k, b, x, y;
  float    r;

  if (ship->autopilot == NULL)
  {
    return;
  }
  if (!IS_SET(ship->autopilot->flags, AIB_ENABLED))
  {
    return;
  }
  ai = ship->autopilot;
/* Ship AI starts here */

  switch (ai->type)
  {
  case AI_LINE:
    if (!getmap(ship))
      return;
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
          if ((int)ship->setheading != b && ai->t_room != ship->location)
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


