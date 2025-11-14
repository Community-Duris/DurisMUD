/*
   ***************************************************************************
   *  File: ferry.h                                            Part of Duris *
   *  Usage: implementation of ferries                                       *
   *  Copyright  1994, 1995, 2006 - Duris Systems Ltd.                       *
   ***************************************************************************

For the changelog and main ferry documentation, see ferry.c

NOTE: TO DISABLE FERRIES COMPLETELY, UNCOMMENT THE #define DISABLE_FERRIES
*/

#ifndef _FERRY_H_
#define _FERRY_H_

//#define DISABLE_FERRIES

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <list>
#include <string>
using namespace std;

#define FRY_STATE_DISABLED 0
#define FRY_STATE_WAITING 1
#define FRY_STATE_DEPARTED 2
#define FRY_STATE_UNDERWAY 3

// number of seconds to wait after announcing departure
#define DEPARTING_DELAY 60

#define FERRY_AUTOMAT_OBJ 47006
#define FERRY_TICKET_VNUM 47003

int ferry_automat_proc(P_obj board, P_char ch, int cmd, char *arg);

// struct for defining one leg of the ferry's journey
struct RouteLeg {
	RouteLeg(int _dest_room, string _dest_name) : 
		dest_room(_dest_room), 
		stop_here(true), dest_name(_dest_name) {			
	}

	RouteLeg(int _dest_room) : 
		dest_room(_dest_room), 
		stop_here(false), dest_name("waypoint") {			
	}

	const char* name() const {
		return dest_name.c_str();
	}

	int dest_room;
	
	vector<int> path;
	
	bool stop_here;
	string dest_name;
};

class Ferry {
  public:
    Ferry();
	Ferry(int);

	int cur_room() const {
		if( !obj ) return 0;
		return obj->loc.room;
	}

	int cur_dest_room() const {
		if( route.empty() ) return 0;
		return route[(cur_route_leg+1)%route.size()].dest_room;
	}

	const char* cur_dest_name() {
		if( route.empty() ) return "";

		// hack to find the next route leg that is an actual stop
		int i = (cur_route_leg+1)%route.size();
		for( ; !route[i].stop_here && i != cur_route_leg; i = (i+1)%route.size() ) ;

		return route[i].dest_name.c_str();
	}

	// add a stop on the route, with name
	void add_stop(int room_num, const char* stop_name) {
		route.push_back(RouteLeg(room_num, stop_name));
	}

	// add a "waypoint" on the route, without name - the ferry won't stop
	void add_stop(int room_num) {
		route.push_back(RouteLeg(room_num));
	}

	void init();

	// determine whether or not a room is on board the ferry
	bool room_num_on_board(int room_num);

	int num_chars_on_board();

	void act_to_all_on_board(const char* msg);
  void everyone_look_out_ferry();

	bool on_passenger_list(P_char p);
	void add_to_passenger_list(P_char p);

	void activity();
	void move();
	void ticket_control();

	string get_route_list(int route_stop);
	int eta(int route_stop);

	void panic();

	// variables
	string name;
	int id;
	int ticket_price;

	int obj_num;
	P_obj obj;

	int ticket_obj_num;

 	int boarding_room_num;

	int speed; // number of seconds to wait between movement steps: 0 = every second
	int wait_time; // number of seconds to wait at each destination
	int depart_notice_time; // number of seconds before departure to announce departing.

	vector<int> rooms;

	int cur_route_leg;
	int cur_route_leg_step;
	vector<RouteLeg> route;

	int cur_state;
	int state_timer;

	list<int> passenger_list;
};

// utility functions
void init_ferries();
void shutdown_ferries();

int ferry_room_proc(int room_num, P_char ch, int cmd, char *arg);
int ferry_obj_proc(P_obj obj, P_char ch, int cmd, char *arg);

void ferry_activity();
Ferry* get_ferry_from_room(int room_num);
Ferry* get_ferry_from_obj(int obj_num);
Ferry* get_ferry(int ferry_num);
bool has_item(P_char ch, int obj_num);
bool has_valid_ticket(P_char ch, int ferry_num);

#endif
