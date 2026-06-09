/*
   ***************************************************************************
   *  File: Ferrys.c                                         Part of Duris *
   *  Usage: implementation of Ferrys - actions and procs                  *
   *  Copyright  1994, 1995, 2006 - Duris Systems Ltd.                       *
   ***************************************************************************

For the main ferry documentation, see ferry.c

*/

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utility.h"
#include "utils.h"
#include <fstream>
#include <list>
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>
#include "ferry.h"
#include "graph.h"
#include "spells.h"
#include "vnum.mob.h"
#include "vnum.obj.h"
using namespace std;

/* external variables */
extern P_room world;

// all created Ferrys
list<Ferry *> ferry_list;

struct ferry_definition 
{
	char       *name;
	int         id;
	int         obj_vnum;
	int         board_room_vnum;
	int        *other_rooms;
	int         speed;
	int         wait_time;
	int         depart_notice_time;
	int         ticket_price;
	struct stop_info
	{
		int room_vnum;
		char *name;
	} *stops;
};

Ferry *create_ferry(struct ferry_definition *fd)
{
	Ferry *wd = new Ferry();

	wd->name              = fd->name;
	wd->id                = fd->id;
	wd->obj_num           = real_object0(fd->obj_vnum); // the ship object the ferry is bound to
	wd->boarding_room_num = real_room0(fd->board_room_vnum);   // the room num of the room passengers board/disembark from
	wd->ticket_price      = fd->ticket_price;


	// all rooms on ship
	wd->rooms.push_back(real_room0(fd->board_room_vnum));
	int start_room = fd->other_rooms[0];
	int end_room = fd->other_rooms[1];
	for (int room_num = start_room; room_num <= end_room; room_num++)
	{
		wd->rooms.push_back(real_room0(room_num));
	}

	wd->speed              = fd->speed;   // number of seconds to wait between moves. 0 == move every step
	wd->wait_time          = fd->wait_time; // number of seconds to wait at each named destination
	wd->depart_notice_time = fd->depart_notice_time;  // number of seconds before departure to announce

	// stops on the route. stops without names are considered just waypoints and are not stopped at
	struct ferry_definition::stop_info *stops = fd->stops;
	while(stops->room_vnum != 0)
	{
		wd->add_stop(real_room0(stops->room_vnum), stops->name);
		stops++;
	}

	return wd;
}

const struct ferry_definition ferries[] = {
	{ 
		"&+yThe &+WWave&+BDancer&N",      // name
		1,                     // id
		47013,                 // shop object
		47011,                 // boarding room vnum
		(int[]){47003, 47010, 0}, // other rooms
		1,                     // speed
		180,                   // wait time
		60,                    // depart notice time
		10000,                  // ticket price
		(struct ferry_definition::stop_info[]){         // stops
			{
				635261, 
				"&+gKhomani-Khan&N"
			}, 
			{
				76654,
				"&+gThe &+GJade &+gEmpire&N"
			},
			{
				1712,
				"&+mQuietus Quay&N"
			},
			{
				76654,
				"&+gThe &+GJade &+gEmpire&N"
			},
			{
				82686,
				"&+MMyrabolus&N"
			},
			{
				76654,
				"&+gThe &+GJade &+gEmpire&N"
			},
			{ 0 }
		}
	},
	{ 
		"&+yThe &+bSe&+ca&+Wsp&+Cr&+bay&N",      // name
		2,                     // id
		47014,                 // shop object
		47026,                 // boarding room vnum
		(int[]){47025, 47025, 0}, // other rooms
		1,                     // speed
		120,                   // wait time
		60,                    // depart notice time
		10000,                  // ticket price
		(struct ferry_definition::stop_info[]){         // stops
			{
				22444, 
				"&+WSto&+Lrm Port&n"
			}, 
			{
				550724,
				"&+bMenden-on-the-Deep&n"
			},
			{ 0 }
		}
	},
	{ 
		"&+yThe &+CStalval&N",      // name
		3,                     // id
		47016,                 // shop object
		47072,                 // boarding room vnum
		(int[]){47073, 47132, 0}, // other rooms
		2,                     // speed
		180,                   // wait time
		60,                    // depart notice time
		10000,                  // ticket price
		(struct ferry_definition::stop_info[]){         // stops
			{
				83788, 
				"&+WDeramuth Port&N"
			}, 
			{
				564319,
				"&+rFort &+RBoyard&N"
			},
			{ 0 }
		}
	},
	{ 
		"&+rThe &+RC&+rr&+Ri&+rm&+Rs&+ro&+Rn F&+ru&+ylg&+ru&+Rr&N",      // name
		4,                     // id
		47017,                 // shop object
		47146,                 // boarding room vnum
		(int[]){47133, 47195, 0}, // other rooms
		2,                     // speed
		240,                   // wait time
		30,                    // depart notice time
		10000,                  // ticket price
		(struct ferry_definition::stop_info[]){         // stops
			{
				83788, 
				"&+WDeramuth Port&N"
			}, 
			{
				9979,
				"&+ySarmiz'Duul&N"
			},
			{ 0 }
		}
	},
	{ 
		"&+LThe &+cIron&+Lhold &+CG&+crudg&+Ce&n",      // name
		5,                     // id
		47015,                 // shop object
		47029,                 // boarding room vnum
		(int[]){47030, 47071, 0}, // other rooms
		2,                     // speed
		180,                   // wait time
		60,                    // depart notice time
		10000,                  // ticket price
		(struct ferry_definition::stop_info[]){         // stops
			{
				83789, 
				"&+WDeramuth Port&N"
			}, 
			{
				49089,
				"&+YVenan'Trut&N"
			},
			{ 0 }
		}
	},
	{ 
		"&+yOld Rickety Ferry&N",      // name
		6,                     // id
		47004,                 // shop object
		47024,                 // boarding room vnum
		(int[]){47027, 47028, 0}, // other rooms
		2,                     // speed
		120,                   // wait time
		60,                    // depart notice time
		5000,                  // ticket price
		(struct ferry_definition::stop_info[]){         // stops
			{
				133052, 
				"&+WTharnadia&N"
			}, 
			{
				557216,
				"&+CMoonshae &+GIsland&N"
			},
			{
				550723,
				"&+bMenden-of-the-Deep&N"
			},
			{ 0 }
		}
	},
	{
		"The Stromvok",        // name
		7,                     // id
		47018,                 // shop object
		47198,                 // boarding room vnum
		(int[]){47198, 47125, 0}, // other rooms
		2,                     // speed
		120,                   // wait time
		60,                    // depart notice time
		5000,                  // ticket price
		(struct ferry_definition::stop_info[]){         // stops
			{
				22445,
				"&+WSto&+Lrm Port&N"
			},
			{
				66688,
				"&+YTorrhan&N"
			},
			{
				30929,
				"&+WStrathor&N"
			},
			{ 0 }
		}
	},
	{ 0 }
};

void init_ferries()
{
	P_obj  undead_ferry;
	P_char Charon;

#ifdef DISABLE_FERRIES
	fprintf(stderr, "--    Ferries disabled\r\n");
	return;
#endif

	fprintf(stderr, "--    Booting Ferries\r\n");

	struct ferry_definition *it = (struct ferry_definition *)ferries;
	while(it->name != NULL)
	{
		ferry_list.push_back(create_ferry(it));
		it++;
	}

	for (list<Ferry *>::iterator it = ferry_list.begin(); it != ferry_list.end(); it++)
	{
		if (*it)
		{
			(*it)->init();
		}
	}

	undead_ferry = read_object(VOBJ_UNDEAD_FERRY, VIRTUAL);
	if (undead_ferry != NULL)
	{
		obj_to_room(undead_ferry, real_room0(600586));
	}
	Charon = read_mobile(VMOB_CHARON_BOATMAN, VIRTUAL);
	if (Charon)
	{
		char_to_room(Charon, real_room0(600586), -2);
	}
	// fprintf(stderr, "      Ferry loading complete.\r\n");
}

void shutdown_ferries()
{

#ifdef DISABLE_FERRIES
	return;
#endif

	for (list<Ferry *>::iterator it = ferry_list.begin(); it != ferry_list.end(); it++)
	{
		if (*it)
		{
			delete *it;
			*it = NULL;
		}
	}
}

// utility to retrieve the ferry that a room belongs to
Ferry *get_ferry_from_room(int room_num)
{
	for (list<Ferry *>::iterator it = ferry_list.begin(); it != ferry_list.end(); it++)
	{
		if (*it)
		{
			if ((*it)->room_num_on_board(room_num))
				return (*it);
		}
	}
	return (NULL);
}

// utility to retrieve the ferry that an object belongs to
Ferry *get_ferry_from_obj(int obj_num)
{
	for (list<Ferry *>::iterator it = ferry_list.begin(); it != ferry_list.end(); it++)
	{
		if (*it)
		{
			if ((*it)->obj->R_num == obj_num)
				return (*it);
		}
	}
	return (NULL);
}

int ferry_room_proc(int room_num, P_char ch, int cmd, char *arg)
{
	if ((cmd != CMD_LOOK) && (cmd != CMD_DISEMBARK))
		return FALSE;

	Ferry *ferry = get_ferry_from_room(room_num);

	if (!ferry)
		return FALSE;

	if (cmd == CMD_LOOK)
	{
		if (!arg || !(*arg) || str_cmp(arg, " out"))
			return FALSE;

		// i think this is a hack-y way to do this, but following
		// foo's lead from newships. this basically transfers the
		// player temporarily to the outside room which triggers
		// the show room function and then transfers them immediately back
		int old_room_id = ch->in_room;
		char_from_room(ch);
		char_to_room(ch, ferry->obj->loc.room, -1);
		char_from_room(ch);
		ch->specials.z_cord = 0;
		char_to_room(ch, old_room_id, -2);
		return (TRUE);
	}

	if (cmd == CMD_DISEMBARK)
	{
		if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			send_to_char("You're in no position to disembark!&n\r\n", ch);
			return (TRUE);
		}

		if (IS_FIGHTING(ch) || IS_DESTROYING(ch))
		{
			send_to_char("You're too busy fighting for your life to disembark!&n\r\n", ch);
			return (TRUE);
		}

		if (room_num != ferry->boarding_room_num)
		{
			send_to_char("You must disembark from the boarding area.&n\r\n", ch);
			return (TRUE);
		}

		act("You disembark from $p.&n", FALSE, ch, ferry->obj, 0, TO_CHAR);
		act("$n disembarks from $p.&n", TRUE, ch, ferry->obj, 0, TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, ferry->cur_room(), 0);
		act("$n disembarks from $p.&n", TRUE, ch, ferry->obj, 0, TO_ROOM);
		return (TRUE);
	}

	return (FALSE);
}

int ferry_obj_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	// ignore periodic calls
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	// we're only listening for the "enter" command
	if (cmd != CMD_ENTER)
		return (FALSE);

	char obj_name[MAX_INPUT_LENGTH];
	one_argument(arg, obj_name);

	P_obj obj_target = get_obj_in_list_vis(ch, obj_name, world[ch->in_room].contents);

	// the target was something else
	if (obj_target != obj)
		return (FALSE);

	if (!obj || (obj->type != ITEM_SHIP))
		return FALSE;

	// find which ferry the object belongs to
	Ferry *ferry = get_ferry_from_obj(obj->R_num);

	if (!ferry)
		return FALSE;

	act("$n boards $p.&n", TRUE, ch, ferry->obj, 0, TO_ROOM);
	char_from_room(ch);
	act("You board $p.&n", FALSE, ch, ferry->obj, 0, TO_CHAR);
	char_to_room(ch, ferry->boarding_room_num, 0);
	act("$n comes onto $p.&n", TRUE, ch, ferry->obj, 0, TO_ROOM);

	return TRUE;
}

// called every second
void ferry_activity()
{
	for (list<Ferry *>::iterator it = ferry_list.begin(); it != ferry_list.end(); it++)
	{
		if (*it)
			(*it)->activity();
	}
}

int ferry_automat_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd != CMD_LOOK && cmd != CMD_BUY)
		return FALSE;

	int ferry_id = obj->value[0]; // which ferry

	Ferry *ferry = get_ferry(ferry_id);

	if (!ferry)
		return FALSE;

	int ticket_cost = obj->value[1]; // ticket cost
	int route_stop  = obj->value[2];

	char buff[MAX_STRING_LENGTH];

	if (cmd == CMD_LOOK && isname(arg, "contraption"))
	{
		snprintf(buff, MAX_STRING_LENGTH, "The %s sails from here to the following destinations:\r\n%s\r\n", ferry->name.c_str(), ferry->get_route_list(route_stop).c_str());
		send_to_char(buff, ch);

		int eta = ferry->eta(route_stop);

		if (eta == -2)
		{
			send_to_char("This ferry is &+Rnot in service.\r\n\r\n", ch);
		}
		else if (eta == -1)
		{
			send_to_char("This ferry is &+Gnow boarding&n.\r\n\r\n", ch);
		}
		else if (eta < 1)
		{
			send_to_char("This ferry is due to arrive &+Gshortly&n.\r\n\r\n", ch);
		}
		else
		{
			snprintf(buff, MAX_STRING_LENGTH, "This ferry is due to arrive in approximately &+G%d hours&n.\r\n\r\n", eta);
			send_to_char(buff, ch);
		}

		string price_str(coin_stringv(ferry->ticket_price));
		snprintf(
			buff, MAX_STRING_LENGTH, "Tickets for this ferry cost %s. Type '&+Wbuy ticket&n' to\r\nbuy a ticket, and please keep your ticket for the duration of your journey.\r\n", price_str.c_str());
		send_to_char(buff, ch);

		return TRUE;
	}

	if (cmd == CMD_BUY && isname(arg, "ticket"))
	{

		P_obj ticket = read_object(FERRY_TICKET_VNUM, VIRTUAL);

		if (!ticket)
			return FALSE;

		if (GET_MONEY(ch) < ticket_cost)
		{
			extract_obj(ticket);
			send_to_char("You don't have enough money to buy a ticket!\r\n", ch);
			return TRUE;
		}

		ticket->value[0] = ferry_id;

		char buf[MAX_STRING_LENGTH];
		snprintf(buf, MAX_STRING_LENGTH, "a &+Wferry ticket&n for the %s", ferry->name.c_str());
		ticket->short_description = str_dup(buf);

		send_to_char("You put your money into the machine and receive a ticket.\r\n", ch);
		SUB_MONEY(ch, ticket_cost, 0);
		obj_to_char(ticket, ch);

		return TRUE;
	}

	return FALSE;
}

Ferry *get_ferry(int ferry_id)
{
	for (list<Ferry *>::iterator it = ferry_list.begin(); it != ferry_list.end(); it++)
	{
		if (*it)
		{
			if ((*it)->id == ferry_id)
				return (*it);
		}
	}
	return (NULL);
}
