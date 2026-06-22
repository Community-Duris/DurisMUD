/*
   ***************************************************************************
   *  File: ferrys.c                                         Part of Duris *
   *  Usage: implementation of ferrys                                      *
   *  Copyright  1994, 1995, 2006 - Duris Systems Ltd.                       *
   ***************************************************************************

Changelog
-----------
3-20-06 - Initial implementation by Torgal (torgal@durismud.com)



TO DISABLE FERRIES COMPLETELY, UNCOMMENT THE #define DISABLE_FERRIES IN ferry.h

All objects and rooms should be given with their real number.

Ferries were designed to be very self contained; they rely on specific rooms and objects in the mud,
but all of the initialisation code for the room/obj procs and loading the objects are contained in
Ferry::init(), so if ferries are disabled they should not affect the game world at all.

Ferry::activity() for each ferry is called once per second; depending on its internal state, it either counts down on its internal wait timer or move()s.

Every move() step, Ferry::ticket_control() is called. Each player character that has the defined ticket object in inventory is added to the internal passenger list, and those without are kicked off
the ship into the current room. The passenger list is reset after every stop on the route. Players already on the passenger list are not checked again until the next route leg.

Route legs are defined by giving a destination room number; the route legs are followed in order, and the paths between the destinations are generated once at mud bootup. If an explicit name is given
to the route leg, it is considered a stop and the ferry will wait at that stop for the defined waiting period. If no explicit name is given, it is just considered a waypoint and the ferry will not
actually stop there.

Ferry.h/ferry.c contain the Ferry class declaration and definition, and ferryact.c contains the non-class functions that interact with the mud directly, such as bootup/shutdown and pulse activity.

A ferry class should be instantiated, and its requisite information set, and then its init() method should be explicitly called to load the ferry into the game world.

I have tried to check for as many possible error states as possible; in case of an error, the ferry Ferry:panic()s and all passengers are dumped into the first room on the first route, and the ferry
is completely disabled.

Enjoy!

*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <list>
#include <vector>
#include "ferry.h"
#include "graph.h"
#include "spells.h"
using namespace std;

extern P_room      world;
extern P_index     obj_index;
extern const char *dirs[];
extern const int   rev_dir[];
extern const char *dirs2[];
extern const char *short_dirs[];

char buffer[MAX_STRING_LENGTH];

// Ferry Class
Ferry::Ferry()
	: id(0), ticket_price(5000), boarding_room_num(0), cur_state(FRY_STATE_DISABLED), state_timer(0), speed(0), wait_time(0), depart_notice_time(0), obj_num(0), obj(NULL), ticket_obj_num(0),
	  cur_route_leg(0), cur_route_leg_step(0)
{
}

// initialize a particular ferry - that means actually loading the objects, room functs, etc
void Ferry::init()
{
	obj = read_object(obj_num, REAL);
	obj_to_room(obj, route[0].dest_room);
	obj_index[obj_num].func.obj = ferry_obj_proc;
	cur_state                   = FRY_STATE_WAITING;
	state_timer                 = wait_time;
	cur_route_leg               = 0;

	// fprintf(stderr, "        Loading %s...\r\n", name.c_str() );

	for (vector<int>::iterator it = rooms.begin(); it != rooms.end(); it++)
	{
		world[*it].funct = ferry_room_proc;
	}

	// fprintf(stderr, "         Generating route paths...\r\n");
	for (int i = 0; i < route.size(); i++)
	{

		if (route[i].stop_here)
		{
			P_obj automat = read_object(FERRY_AUTOMAT_OBJ, VIRTUAL);

			if (!automat)
			{
				logit(LOG_DEBUG, "Can't find ferry ticket automat object (%d)!", FERRY_AUTOMAT_OBJ);
				break;
			}

			automat->value[0] = id;
			automat->value[1] = ticket_price;
			automat->value[2] = i;
			obj_to_room(automat, route[i].dest_room);
			obj_index[automat->R_num].func.obj = ferry_automat_proc;
		}

		bool found_path = dijkstra(route[i].dest_room, route[(i + 1) % route.size()].dest_room, valid_ship_edge, route[i].path);

		if (!found_path)
		{
			fprintf(stderr, "   %s        %s-> no path found!\r\n", name.c_str(), route[i].name());
		}
	}
}

// do an action to all player characters on board the ferry
void Ferry::act_to_all_on_board(const char *msg)
{
	for (vector<int>::iterator it = rooms.begin(); it != rooms.end(); it++)
	{
		if (*it > 0 && world[*it].people)
		{
			for (P_char ch = world[*it].people; ch; ch = ch->next_in_room)
			{
				act(msg, FALSE, ch, 0, 0, TO_CHAR);
			}
		}
	}
}

// determine whether or not a room is on board this ferry
bool Ferry::room_num_on_board(int room_num)
{
	for (vector<int>::iterator it = rooms.begin(); it != rooms.end(); it++)
	{
		if (*it == room_num)
			return (TRUE);
	}
	return (FALSE);
}

int Ferry::num_chars_on_board()
{
	int char_count = 0;

	for (vector<int>::iterator it = rooms.begin(); it != rooms.end(); it++)
	{
		if (*it && *it != NOWHERE)
		{
			for (P_char p = world[*it].people; p; p = p->next_in_room)
				char_count++;
		}
	}

	return (char_count);
}

void Ferry::everyone_look_out_ferry()
{
	P_char passengers;

	// For each room on the ferry
	for (vector<int>::iterator rroom = rooms.begin(); rroom != rooms.end(); rroom++)
	{
		if ((*rroom > 0) && ((passengers = world[*rroom].people) != NULL))
		{
			// For each person in the room
			while (passengers != NULL)
			{
				if (IS_SET(passengers->specials.act2, PLR2_SHIPMAP))
					new_look(passengers, 0, CMD_LOOKOUT, obj->loc.room);
				passengers = passengers->next_in_room;
			}
		}
	}
}

// this function is called every second; determine which state the ferry is in and react accordingly
void Ferry::activity()
{
	switch (cur_state)
	{
		case FRY_STATE_DISABLED:
			return;
			break;

		case FRY_STATE_WAITING:
			state_timer--;

			if (state_timer == depart_notice_time)
			{
				snprintf(buffer,
				         MAX_STRING_LENGTH,
				         "A bell rings, announcing final boarding call for %s\r\nA strange voice announces, 'All aboard! Now departing for %s.'&n\r\n",
				         name.c_str(),
				         cur_dest_name());
				act_to_all_on_board(buffer);
				send_to_room(buffer, cur_room());
			}
			else if (state_timer <= 0)
			{
				snprintf(buffer, MAX_STRING_LENGTH, "The %s departs for %s&n.\r\n", name.c_str(), cur_dest_name());
				act_to_all_on_board(buffer);
				send_to_room(buffer, cur_room());

				cur_state   = FRY_STATE_UNDERWAY;
				state_timer = speed;
			}
			break;

		case FRY_STATE_UNDERWAY:
			state_timer--;

			ticket_control();

			if (state_timer <= 0)
			{
				move();
				state_timer = speed;
			}

			if (cur_room() == cur_dest_room())
			{
				int next_route_leg = (cur_route_leg + 1) % route.size();

				if (route[next_route_leg].stop_here)
				{
					// we've reached one of our stops
					snprintf(buffer, MAX_STRING_LENGTH, "A shrill horn blows, alerting passengers to disembark.\r\nA strange voice announces, 'Now arrived at %s.'&n\r\n", cur_dest_name());
					act_to_all_on_board(buffer);
					send_to_room(buffer, cur_room());

					cur_state   = FRY_STATE_WAITING;
					state_timer = wait_time;
					passenger_list.clear();
				}

				cur_route_leg      = next_route_leg;
				cur_route_leg_step = 0;
			}
			break;

		default:
			// error, reset to disabled
			debug("%s got into invalid state.", name.c_str());
			cur_state   = FRY_STATE_DISABLED;
			state_timer = 0;
			panic();
	}
}

void Ferry::ticket_control()
{
	for (vector<int>::iterator it = rooms.begin(); it != rooms.end(); it++)
	{
		if (!(*it) || (*it) == NOWHERE)
			continue;

		vector<P_char> stowaways;

		for (P_char p = world[*it].people; p; p = p->next_in_room)
		{
			if (IS_NPC(p) || IS_TRUSTED(p) || on_passenger_list(p))
				continue;

			if (has_valid_ticket(p, id))
			{
				// put on passenger list so we won't keep checking their ticket
				send_to_char("The Automated Ticket System punches your ticket for the journey.&n\r\n", p);
				add_to_passenger_list(p);

				logit(LOG_STATUS, "%s is a passenger on ferry %s", p->player.name, name.c_str());
			}
			else
			{
				stowaways.push_back(p);
			}
		}

		for (vector<P_char>::iterator jt = stowaways.begin(); jt != stowaways.end(); jt++)
		{
			if (!(*jt))
				continue;

			// stowaway!! kick 'em off!
			send_to_char("The Stowaway Detection System ushers you off the boat.&n\r\n", (*jt));
			char_from_room(*jt);
			char_to_room(*jt, cur_room(), 0);

			logit(LOG_STATUS, "%s kicked off ferry %s in %d", (*jt)->player.name, name.c_str(), cur_room());
		}
	}
}

bool Ferry::on_passenger_list(P_char p)
{
	if (!p)
	{
		logit(LOG_DEBUG, "Invalid P_char passed to Ferry::on_passenger_list");
		return false;
	}

	for (list<int>::iterator it = passenger_list.begin(); it != passenger_list.end(); it++)
	{
		if (GET_PID(p) == *it)
			return true;
	}
	return false;
}

void Ferry::add_to_passenger_list(P_char p)
{
	if (!p)
	{
		logit(LOG_DEBUG, "Invalid P_char passed to Ferry::add_to_passenger_list");
		return;
	}

	passenger_list.push_back(GET_PID(p));
}

void Ferry::move()
{
	int dummy;

	if (!obj || !obj->loc.room || !world)
	{
		logit(LOG_DEBUG, "ferry %s wants to move but room is invalid.", name.c_str());
		panic();
		return;
	}

	if (cur_route_leg_step >= route[cur_route_leg].path.size())
	{
		// somehow the path is messed up - panic!
		logit(LOG_DEBUG, "ferry %s tried to move past the path length.", name.c_str());
		panic();
		return;
	}

	int next_step = route[cur_route_leg].path[cur_route_leg_step];
	cur_route_leg_step++;

	if (next_step >= 0 && next_step < NUM_EXITS)
	{
		int to_room = world[obj->loc.room].dir_option[next_step]->to_room;

		if (!to_room)
		{
			logit(LOG_DEBUG, "ferry %s wants to move but to_room doesn't exist!", name.c_str());
			panic();
			return;
		}

		snprintf(buffer, MAX_STRING_LENGTH, "$p sails to %s.", dirs2[next_step]);
		act(buffer, FALSE, 0, obj, 0, TO_ROOM);
		obj_from_room(obj);

		obj_to_room(obj, to_room);
		snprintf(buffer, MAX_STRING_LENGTH, "$p sails in from %s.", dirs2[rev_dir[next_step]]);
		act(buffer, FALSE, 0, obj, 0, TO_ROOM);

		everyone_look_out_ferry();
	}
	else
	{
		// ferry tried to go invalid direction
		logit(LOG_DEBUG, "ferry %s wants to move but exit is invalid.", name.c_str());
		panic();
	}
}

void Ferry::panic()
{
	// if something goes really wrong, transfer all PCs to the first room,
	// switch to DISABLED, and remove the ship object

	int to_room;
	if (!route.size())
	{
		logit(LOG_DEBUG, "something went really wrong with ferry %s - no initial destination room, moved passengers to limbo.", name.c_str());
		to_room = 1;
	}
	else
	{
		to_room = route[0].dest_room;
	}

	logit(LOG_DEBUG, "ferry %s panicked in %d, disabling and dropping passengers in %d.", name.c_str(), cur_room(), to_room);

	cur_state = FRY_STATE_DISABLED;

	for (vector<int>::iterator it = rooms.begin(); it != rooms.end(); it++)
	{
		if ((*it) && (*it) != NOWHERE)
		{
			for (P_char p = world[*it].people; p; p = p->next_in_room)
			{
				if (IS_PC(p))
				{
					send_to_char("&+MThe ship disappears in a blinding flash of magic. As the smoke clears, you find yourself somewhere ... else.\r\n", p);
					char_from_room(p);
					char_to_room(p, to_room, 0);
				}
			}
		}
	}
}

string Ferry::get_route_list(int route_stop)
{
	string str;
	for (int i = (route_stop + 1) % route.size(); i != route_stop; i = (i + 1) % route.size())
	{
		if (!route[i].stop_here)
			continue;
		str = str + route[i].dest_name;
		str = str + "\r\n";
	}

	return str;
}

int Ferry::eta(int route_stop)
{

	if (cur_state == FRY_STATE_DISABLED)
		return -2;

	if (cur_state == FRY_STATE_WAITING && route_stop == cur_route_leg)
		return -1;

	int secs = 0;

	int steptime = speed + 1;
	int leg      = cur_route_leg;

	if (cur_state == FRY_STATE_WAITING)
	{
		secs += state_timer;
		secs += route[leg].path.size() * steptime;
	}
	else
	{
		secs += (route[leg].path.size() - cur_route_leg_step) * steptime;
	}

	leg = (leg + 1) % route.size();

	int failsafe = 0; // just to prevent an infinite loop
	while (leg != route_stop && failsafe < 1000)
	{
		if (route[leg].stop_here)
			secs += wait_time;
		secs += route[leg].path.size() * steptime;
		leg = (leg + 1) % route.size();
		failsafe++;
	}

	return (int)secs / 60;
}

// Utility function
// checks for item in character's inventory
bool has_item(P_char ch, int obj_num)
{
	if (!ch || !ch->carrying)
		return false;

	for (P_obj t_obj = ch->carrying; t_obj; t_obj = t_obj->next_content)
	{
		if (t_obj->R_num == obj_num)
		{
			return true;
		}
	}

	return false;
}

bool has_valid_ticket(P_char ch, int ferry_num)
{
	if (!ch || !ch->carrying)
		return false;

	for (P_obj t_obj = ch->carrying; t_obj; t_obj = t_obj->next_content)
	{
		if (t_obj->R_num == real_object0(FERRY_TICKET_VNUM) && t_obj->value[0] == ferry_num)
		{
			return true;
		}
	}

	return false;
}
