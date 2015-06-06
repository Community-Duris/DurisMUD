#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <list>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "graph.h"
#include "transport.h"
#include "reavers.h"
#include "assocs.h"
#include "ctf.h"

extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const char *dirs[];
extern const char *dirs2[];
extern const int rev_dir[];
extern int top_of_world;
extern int top_of_zone_table;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern P_obj    object_list;


#define TRANSPORT_STATE_WAITING 0
#define TRANSPORT_STATE_MOVING 1
#define TRANSPORT_STATE_RETURNING 2

#define TRANSPORT_ID(x) ( (x)->only.npc->value[0] )
#define TRANSPORT_STATE(x) ( (x)->only.npc->value[1] )
#define TRANSPORT_ORIGIN(x) ( GET_BIRTHPLACE(x) )
#define TRANSPORT_ROUTE(x) ( (x)->only.npc->value[2] )
#define TRANSPORT_STEP(x) ( (x)->only.npc->value[3] )

#define TRANSPORT_SIGN_VNUM 420

struct transport_route {
  int origin_vnum;
  int destination_vnum;
  const char *name;
  int cost_in_plat;
  int min_level;
  int max_level;
  vector<int> path;
} transport_routes[] =
{
  /*
   { start room vnum, destination room vnum, name of route, cost in platinum, min level (0 for none), max level (0 for none)
   }
   */
  
  // snogres dragon
  {619771, 620606, "&+LA Huge &N&+rCavern &+Lon the &N&+rBloodfist &+yMountains", 50, 51, 0 },

  // skeletal dragon
  {576577, 579365, "&+LA Desolate Island", 200, 51, 0 },

  // 
  {606262, 635756, "&+LQuietus Quay", 20, 0, 0 },
  {606262, 588326, "&+WSto&+Lrm &+bPort", 20, 0, 0},

  //
  {635756, 606262, "&+RBloodstone Keep", 20, 0, 0},

  //
  {588326, 606262, "&+RBloodstone Keep", 20, 0, 0},
  
  //
  {543701, 514072, "&+MMyrabolus", 10, 0, 0},
  {543701, 582084, "&+WThe City of &+YTorrhan", 10, 0, 0},

  //
  {582084, 543701, "&+yUgta", 10, 0, 0},

  //
  {514072, 543701, "&+yUgta", 10, 0, 0},

  //
  {580080, 545407, "&+YVenan'Trut", 50, 47, 0},
  
  //
  {545407, 564319, "&+rFort &+RBoyard", 50, 46, 0},
  {545407, 622276, "Sarmiz'Duul", 50, 46, 0},
  {545407, 580080, "&+WThe City of &+YTorrhan", 50, 46, 0},
  
  //
  {622276, 545407, "&+YVenan'Trut", 50, 47, 0},  

  //
  {563522, 545407, "&+YVenan'Trut", 50, 47, 0},  

  // alatorin routes
  {543662, 519043, "&+CFort &+WKhor&N&+calator", 100, 0, 0}, // Kimordril -> Fort Khoralator
  {519043, 543662, "&+YKimordril", 100, 0, 0}, // Fort Khoralator -> Kimordril
  {631413, 522247, "&+RHel&N&+rgor Outpost", 100, 0, 0},   // Clan Shatter Stone -> Helgor Outpost
  {522247, 631413, "&+WClan Shatter Stone", 100, 0, 0},   // Helgor Outpost -> Clan Shatter Stone
  
  {0}  
};

struct transport_data {
  int mob_vnum;
  int origin_vnum;
} transports[] =
{
  // { mob vnum , load room vnum }
  {47027, 619771}, // snogres dragon
  {47015, 576577}, // skeletal dragon
  {47016, 606262},
  {47017, 635756},
  {47018, 588326},
  {47021, 543701},
  {47025, 582084},
  {47022, 514072},
  {47020, 580080},
  {47019, 545407},
  {47023, 622276},
  {47024, 563522},  
  // alatorin routes	
  {47028, 519043}, // Fort Khoralator
  {47030, 543662}, // Kimordril
  {47026, 522247}, // Helgor Outpost
  {47029, 631413}, // Clan Shatter Stone
	
  {0}
};

bool valid_flying_edge(int from_room, int dir) {
	return VALID_FLYING_EDGE(from_room, dir);
}

void initialize_transport()
{
  for( int i = 0; transport_routes[i].origin_vnum; i++ )
  {
    bool found_path = dijkstra(real_room0(transport_routes[i].origin_vnum),
                               real_room0(transport_routes[i].destination_vnum),
                               valid_flying_edge,
                               transport_routes[i].path );
    if( !found_path )
    {
      fprintf(stderr, "  - no route found for [%d] -> [%d] %s\n", transport_routes[i].origin_vnum, transport_routes[i].destination_vnum, strip_ansi(transport_routes[i].name).c_str());
      transport_routes[i].origin_vnum = 0;
    }    
  }
  
  for( int i = 0; transports[i].mob_vnum; i++ )
  {
    P_obj sign = read_object(TRANSPORT_SIGN_VNUM, VIRTUAL);    
    if( !sign ) continue;
    P_char mob = read_mobile(transports[i].mob_vnum, VIRTUAL);
    if( !mob ) continue;
    mob_index[real_mobile0(transports[i].mob_vnum)].func.mob = flying_transport;
    GET_HOME(mob) = GET_BIRTHPLACE(mob) = GET_ORIG_BIRTHPLACE(mob) = transports[i].origin_vnum;    
    SET_BIT(mob->specials.affected_by, AFF_FLY);
    TRANSPORT_ROUTE(mob) = -1;
    TRANSPORT_STATE(mob) = TRANSPORT_STATE_WAITING;
    char_to_room(mob, real_room0(transports[i].origin_vnum), -1);
    obj_to_room(sign, real_room0(transports[i].origin_vnum));
  }
    
}

// Handles list command for flying_transport proc
bool flying_transport_cmd_list(P_char ch, P_char victim, char *arg)
{
  // ch - player listing command
  // victim - mob with the flying_transport proc

  char buff[MAX_STRING_LENGTH];

  if(!ch)
    return FALSE;
  
  send_to_char("&+WFlight Service\n", ch);
  send_to_char("The following routes are available:\n", ch);
  
  bool has_one = false;      
  for( int i = 0, j = 1; transport_routes[i].origin_vnum; i++ )
  {
    if( real_room(transport_routes[i].origin_vnum) != victim->in_room )
      continue;
    
    if( transport_routes[i].path.size() < 1 )
      continue;
    
    if( !IS_TRUSTED(ch) && transport_routes[i].min_level && GET_LEVEL(ch) < transport_routes[i].min_level )
      continue;

    if( !IS_TRUSTED(ch) && transport_routes[i].max_level && GET_LEVEL(ch) > transport_routes[i].max_level )
      continue;

    has_one = true;
    sprintf(buff, "%d) %s &n- %s\n", j++, pad_ansi(transport_routes[i].name, 20).c_str(), coin_stringv(transport_routes[i].cost_in_plat * 1000)); 
    send_to_char(buff, ch);
  }
  
  if( !has_one )
  {
    send_to_char("None!\n", ch);
  }      
  else
  {
    send_to_char("\nType 'buy <id>' to ride to the destination\n", ch);
  }
  
  return TRUE;
}

#define VNUM_OBJ_FLIGHT_PATH_TICKET 47008

// Handles buy command for flying_transport proc
bool flying_transport_cmd_buy(P_char ch, P_char victim, char *arg)
{
  // ch - player listing command
  // victim - mob with the flying_transport proc

  char buff[MAX_STRING_LENGTH];
  P_obj ticket = NULL;
  
  if( IS_RIDING(victim) )
    return FALSE;
    
  one_argument(arg, buff);
  int choice = atoi(buff);
  
  int i, j;
  bool found_it = false;
  for( i = 0, j = 1; transport_routes[i].origin_vnum; i++ )
  {
    if( real_room(transport_routes[i].origin_vnum) != victim->in_room )
      continue;
    
    if( transport_routes[i].path.size() < 1 )
      continue;
    
    if( !IS_TRUSTED(ch) && transport_routes[i].min_level && GET_LEVEL(ch) < transport_routes[i].min_level )
      continue;
    
    if( !IS_TRUSTED(ch) && transport_routes[i].max_level && GET_LEVEL(ch) > transport_routes[i].max_level )
      continue;
    
    if( choice == j )
    {
      found_it = true;
      break;
    }
    
    j++;
  }
  
  if( !found_it || !transport_routes[i].origin_vnum )
  {
    send_to_char("Invalid choice.\n", ch);
    return TRUE;
  }      

  if( GET_MONEY(ch) < ( transport_routes[i].cost_in_plat * 1000 ) )
  {
    send_to_char("You don't have enough money!\n", ch);
    return TRUE;        
  }

  ticket = read_object(VNUM_OBJ_FLIGHT_PATH_TICKET, VIRTUAL);
  
  if(ticket)
  {
    SUB_MONEY(ch, ( transport_routes[i].cost_in_plat * 1000 ), 0);
    send_to_char("Here is your ticket!\n", ch);
    ticket->value[6] = mob_index[GET_RNUM(victim)].virtual_number;
    ticket->value[7] = i;
    obj_to_char(ticket, ch);
  }
  else
  {
    send_to_char("&+RCan't create ticket object, please bug this right now.\n", ch);
  }

  return TRUE;
}

// Handles give command for flying_transport proc
bool flying_transport_cmd_give(P_char ch, P_char victim, char *arg)
{
  // ch - player listing command
  // victim - mob with the flying_transport proc
  char objectarg[MAX_INPUT_LENGTH], targetarg[MAX_INPUT_LENGTH];
  P_obj ticket = NULL;
  P_char target = NULL;

  arg = one_argument(arg, objectarg);
  one_argument(arg, targetarg);

  if(objectarg[0] == '\0' || targetarg[0] == '\0')
    return FALSE;

  ticket = get_obj_in_list_vis(ch, objectarg, ch->carrying);
  if(!ticket ||
     obj_index[ticket->R_num].virtual_number != VNUM_OBJ_FLIGHT_PATH_TICKET || 
     ticket->value[6] != mob_index[GET_RNUM(victim)].virtual_number)
  {
    return FALSE;
  }

  target = get_char_room_vis(ch, targetarg);
  if(!target || target != victim)
    return FALSE;

  if(IS_FIGHTING(ch) || IS_DESTROYING(ch))
  {
    send_to_char("You're a bit busy fighting to be flying!", ch);
    return TRUE;
  }

  if( IS_RIDING(ch) )
  {
    send_to_char("You're already riding something!\n", ch);
    return TRUE;
  }

  // just destroy the ticket
  extract_obj(ticket, TRUE);  

  TRANSPORT_ROUTE(victim) = ticket->value[7];
  TRANSPORT_STEP(victim) = 0;

#if defined(CTF_MUD) && (CTF_MUD == 1)
    if (ctf_carrying_flag(ch) == CTF_PRIMARY)
    {
      send_to_char("You can't carry that with you.\r\n", ch);
      drop_ctf_flag(ch);
    }
#endif

  act("You climb up and ride $n.", FALSE, victim, 0, ch, TO_VICT);
  act("$N climbs up and rides $n.", FALSE, victim, 0, ch, TO_NOTVICT);
  link_char(ch, victim, LNK_RIDING);
  
  ch->specials.z_cord = 10;
  victim->specials.z_cord = 10;
  
  TRANSPORT_STATE(victim) = TRANSPORT_STATE_MOVING;
  
  act("With a mighty leap, $n launches $mself into the air.", FALSE, victim, 0, ch, TO_ROOM);      
  add_event(event_flying_transport_move, PULSE_VIOLENCE, victim, ch, 0, 0, 0, 0);      
  
  return TRUE;
}



int flying_transport(P_char ch, P_char victim, int cmd, char *arg)
{
  if( cmd == CMD_SET_PERIODIC )
  {
    return TRUE;
  }

  if( !IS_ALIVE(ch) )
  {
    return FALSE;
  }

  if( TRANSPORT_STATE(ch) == TRANSPORT_STATE_WAITING )
  {
    if(!victim)
    {
      return FALSE;
    }

    switch(cmd)
    {
    case CMD_LIST:
      return flying_transport_cmd_list(victim, ch, arg);
    case CMD_BUY:
      return flying_transport_cmd_buy(victim, ch, arg);
    case CMD_GIVE:
      return flying_transport_cmd_give(victim, ch, arg);
    default:
      break;
    }
  }
  else if( TRANSPORT_STATE(ch) == TRANSPORT_STATE_MOVING )
  {
    if( !victim || victim != get_linking_char(ch, LNK_RIDING) )
    {
      return FALSE;
    }

    if( IS_TRUSTED(victim) )
    {
      return FALSE;
    }

    if( cmd == CMD_LOOK || cmd == CMD_SCORE || cmd == CMD_INVENTORY
      || cmd == CMD_ATTRIBUTES || cmd == CMD_NEWS
      || cmd == CMD_PETITION || cmd == CMD_TOGGLE )
    {
      return FALSE;
    }

    act("You can't do that, you're holding on for dear life!", FALSE, ch, 0, victim, TO_VICT);    
    return TRUE;
  }

  return FALSE;
}

int do_simple_move_skipping_procs(P_char ch, int exitnumb, unsigned int flags);

void event_flying_transport_move(P_char ch, P_char victim, P_obj obj, void *data)
{
  char buff[MAX_STRING_LENGTH];

  if( !ch || !victim )
    return;
  
  if( TRANSPORT_STATE(ch) != TRANSPORT_STATE_MOVING ||
     ch->in_room != victim->in_room )
    return;
  
  if( TRANSPORT_ROUTE(ch) < 0 )
    return;
  
  int dir = BFS_ERROR;
  int dist = 0;
  
  int path_size = transport_routes[TRANSPORT_ROUTE(ch)].path.size();

  if( path_size < 1 )
  {
    dir = BFS_NO_PATH;    
  }
  else if( TRANSPORT_STEP(ch) == path_size )
  {
    dir = BFS_ALREADY_THERE;
  }
  else if( TRANSPORT_STEP(ch) > path_size )
  {
    dir = BFS_ERROR;
  }
  else
  {
    dir = transport_routes[TRANSPORT_ROUTE(ch)].path[TRANSPORT_STEP(ch)++];    
  }
  
//  dir = (int) find_first_step(ch->in_room, real_room0(TRANSPORT_DESTINATION(ch)), BFS_CAN_FLY, 0, WAGON_TYPE_FLYING, &dist);
  
  if (dir == BFS_ERROR || dir == BFS_NO_PATH)
  {
    act("$n looks confused for a moment, lands, and you dismount hastily.", FALSE, ch, 0, victim, TO_VICT);
    act("$n looks confused for a moment, lands, and $N dismounts hastily.", FALSE, ch, 0, victim, TO_NOTVICT);
    
    ch->specials.z_cord = 0;
    victim->specials.z_cord = 0;
    do_dismount(victim, 0, CMD_DISMOUNT);
    
    logit(LOG_DEBUG, "BFS_ERROR or BFS_NO_PATH in event_flying_transport_move() transport.c with %s as rider and %s as transport.", GET_NAME(victim), GET_NAME(ch));
    
    TRANSPORT_STATE(ch) = TRANSPORT_STATE_WAITING;
    return;
  }
  else if (dir == BFS_ALREADY_THERE)
  {
    act("$n lands.", FALSE, ch, 0, victim, TO_ROOM);

    ch->specials.z_cord = 0;
    victim->specials.z_cord = 0;
    do_dismount(victim, 0, CMD_DISMOUNT);
    
    act("$n launches $mself into the air.", FALSE, ch, 0, victim, TO_ROOM);
    
    add_event(event_flying_transport_return, (int) (PULSE_VIOLENCE / 2), ch, victim, 0, 0, 0, 0);
    return;
  }

  int was_in = ch->in_room;
  
  if( !world[was_in].dir_option[dir] )
  {
    act("$n looks confused for a moment, lands, and you dismount hastily.", FALSE, ch, 0, victim, TO_VICT);
    act("$n looks confused for a moment, lands, and $N dismounts hastily.", FALSE, ch, 0, victim, TO_NOTVICT);

    logit(LOG_DEBUG, "dir option error in event_flying_transport_move() transport.c with %s as rider and %s as transport.", GET_NAME(victim), GET_NAME(ch));
    
    ch->specials.z_cord = 0;
    victim->specials.z_cord = 0;
    do_dismount(victim, 0, CMD_DISMOUNT);
    
    TRANSPORT_STATE(ch) = TRANSPORT_STATE_WAITING;
    return;
  }
  
  int dest_room = TOROOM(was_in, dir);
  
  sprintf(buff, "Far above, $n flies %s.", dirs[dir]);
  act(buff, FALSE, ch, 0, victim, TO_NOTVICT | ACT_IGNORE_ZCOORD);

  sprintf(buff, "$n flies %s.", dirs[dir]);
  act(buff, FALSE, ch, 0, victim, TO_VICT);

  char_from_room(ch);
  ch->specials.was_in_room = was_in;    
  char_to_room(ch, dest_room, dir);
  
  sprintf(buff, "Far above, $n flies in from the %s.", dirs2[rev_dir[dir]]);
  act(buff, FALSE, ch, 0, victim, TO_NOTVICT | ACT_IGNORE_ZCOORD);
  
  add_event(event_flying_transport_move, (int) (PULSE_VIOLENCE / 5), ch, victim, 0, 0, 0, 0);
}

void event_flying_transport_return(P_char ch, P_char victim, P_obj obj, void *data)
{
  char buff[MAX_STRING_LENGTH];
  
  if( !ch )
    return;
  
  if( TRANSPORT_STATE(ch) != TRANSPORT_STATE_MOVING )
    return;
  
  if( TRANSPORT_ROUTE(ch) < 0 )
    return;
  
  int dir = BFS_ERROR;
  int dist = 0;
  
  int path_size = transport_routes[TRANSPORT_ROUTE(ch)].path.size();
  
  if( path_size < 1 )
  {
    dir = BFS_NO_PATH;    
  }
  else if( TRANSPORT_STEP(ch) == 0 )
  {
    dir = BFS_ALREADY_THERE;
  }
  else if( TRANSPORT_STEP(ch) < 0 )
  {
    dir = BFS_ERROR;
  }
  else
  {
    dir = rev_dir[transport_routes[TRANSPORT_ROUTE(ch)].path[--TRANSPORT_STEP(ch)]];    
  }
  
//  dir = (int) find_first_step(ch->in_room, real_room0(TRANSPORT_DESTINATION(ch)), BFS_CAN_FLY, 0, WAGON_TYPE_FLYING, &dist);
  
  if (dir == BFS_ERROR || dir == BFS_NO_PATH || dir == BFS_ALREADY_THERE )
  {
    if(dir == BFS_ERROR || dir == BFS_NO_PATH)
    {
    logit(LOG_DEBUG, "BFS_ERROR or BFS_NO_PATH in event_flying_transport_return() transport.c with %s as rider and %s as transport.", GET_NAME(victim), GET_NAME(ch));
    }
    
    ch->specials.z_cord = 0;    
    act("$n lands.", FALSE, ch, 0, victim, TO_ROOM);
    TRANSPORT_STATE(ch) = TRANSPORT_STATE_WAITING;
    return;
  }
  
  int was_in = ch->in_room;
  int dest_room = TOROOM(was_in, dir);
  
  sprintf(buff, "Far above, $n flies %s.", dirs[dir]);
  act(buff, FALSE, ch, 0, 0, TO_ROOM | ACT_IGNORE_ZCOORD);
  
  char_from_room(ch);
  ch->specials.was_in_room = was_in;    
  char_to_room(ch, dest_room, dir);
  
  sprintf(buff, "Far above, $n flies in from the %s.", dirs2[rev_dir[dir]]);
  act(buff, FALSE, ch, 0, 0, TO_ROOM | ACT_IGNORE_ZCOORD);
  
  add_event(event_flying_transport_return, (int) (PULSE_VIOLENCE / 8), ch, victim, 0, 0, 0, 0);
}

