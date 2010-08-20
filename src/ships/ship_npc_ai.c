/*****************************************************
* ship_npc_ai.c
*
* NPC Ship AI routines
*****************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h> 

#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include "map.h"
#include "ships.h"
#include "ship_npc.h"
#include "ship_npc_ai.h"


extern char buf[MAX_STRING_LENGTH];

static bool weapon_ok(P_ship ship, int w_num)
{
    if (SHIPWEAPONDESTROYED(ship, w_num)) 
        return false;
    if (SHIPWEAPONDAMAGED(ship, w_num)) 
        return false;
    if (ship->slot[w_num].val1 == 0) 
        return false;
    return true;
}

static bool weapon_ready_to_fire(P_ship ship, int w_num)
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

static float get_arc_central_bearing(int arc)
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

static int get_arc_width(int arc)
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

static bool is_boardable(P_ship ship)
{
    if (ship->speed > BOARDING_SPEED)
        return false;
    if (ISWARSHIP(ship) && ship->speed > 0)
        return false;
    return true;
}


NPCShipAI::NPCShipAI(P_ship s, P_char ch)
{
    ship = s;
    escort = 0;
    advanced = 0;
    permanent = false;
    mode = NPC_AI_IDLING;
    turning = NPC_AI_NOT_TURNING;
    t_contact = 0;
    t_bearing = 0;
    t_arc = 0;
    s_arc = 0;
    t_range = 0;
    t_x = 0;
    t_y = 0;
    debug_char = ch;
    did_board = 0;
    speed_restriction = -1;
    since_last_fired_right = 0;

    if (SHIPHULLWEIGHT(ship) > 200)
    {
        is_heavy_ship = true;
        is_multi_target = true;
    }
    else
    {
        is_heavy_ship = false;
        is_multi_target = false;
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

    if (!check_for_captain_on_bridge())
    {
        send_message_to_debug_char("No captain of the bridge!\r\n");
        return;
    }

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

        if (ship->target == 0 || !find_current_target())
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
            if (!go_around_land(ship->target))
                mode = NPC_AI_RUNNING; // TODO: do something better?
            break;
        }

        if (worth_ramming() && check_ram())
        {
            ram_target();
            if (ship->target == 0)
                break;
        }

        if (did_board != ship->target && is_boardable(ship->target))
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

        if (advanced == 1)
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
        if (type == NPC_AI_PIRATE || type == NPC_AI_HUNTER)
        {
            if (find_new_target())
            {
                mode = NPC_AI_ENGAGING;
                break;
            }
        }
        if (type == NPC_AI_ESCORT)
        {
            if (do_escort())
                break;
        }
    } // no break
    case NPC_AI_LEAVING:
    {
        cruise();
        if (!permanent && try_unload())
            return;
    }   break;

    default:
        break;
    };
}

void NPCShipAI::cruise()
{
    send_message_to_debug_char("Cruising: \r\n");
    reload_and_repair();

    if (calc_land_dist(ship->x, ship->y, ship->heading, 5.0) < 5.0) 
    {
        if (turning == NPC_AI_NOT_TURNING)
            turning = (number(1, 2) == 1) ? NPC_AI_TURNING_LEFT : NPC_AI_TURNING_RIGHT;
        if (turning == NPC_AI_TURNING_LEFT)
            new_heading = ship->heading - 30; 
        else
            new_heading = ship->heading + 30; 
    }
    else
        turning = NPC_AI_NOT_TURNING;

    set_new_dir();
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

void NPCShipAI::attacked_by(P_ship attacker)
{
    if (mode != NPC_AI_RUNNING &&  mode != NPC_AI_ENGAGING)
    {
        ship->target = attacker;
        mode = NPC_AI_ENGAGING;
        if (type == NPC_AI_ESCORT && attacker == escort)
            type = NPC_AI_HUNTER; // don't attack your escort, it will turn on you!
    }
}

void NPCShipAI::escort_attacked_by(P_ship attacker)
{
    if (ship->target == 0)
    {
        ship->target = attacker;
        mode = NPC_AI_ENGAGING;
        send_message_to_debug_char("Escortee attacked by %s, engaging!\r\n", attacker->id);
    }
}


bool NPCShipAI::try_unload()
{
    if (ship->timer[T_MAINTENANCE] == 1)
    {
        for (int i = 0; i < contacts_count; i++)
        {
            if (SHIPISDOCKED(contacts[i].ship) || ISNPCSHIP(contacts[i].ship))
                continue;
            ship->timer[T_MAINTENANCE] += 10;
            return FALSE;
        }
        send_message_to_debug_char("Unloading!\r\n");
        return try_unload_npc_ship(ship);
    }
    return FALSE;
}


bool NPCShipAI::check_for_captain_on_bridge()
{
    P_char ch, ch_next;
    for (ch = world[real_room(ship->room[0].roomnum)].people; ch; ch = ch_next)
    {
        if (ch)
        {
            ch_next = ch->next_in_room;
            if (IS_NPC(ch) && mob_index[ch->only.npc->R_num].func.mob == npc_ship_crew_captain_func)
            {
                return true;
            }
            
            if (IS_PC(ch) && IS_TRUSTED(ch))
            {
                return true;
            }
        }
    }
    return false;
}

bool NPCShipAI::do_escort()
{
    if (SHIPIMMOBILE(ship)) return false;
    int i = 0;
    for (; i < contacts_count; i++) 
    {
        if (contacts[i].ship == escort)
            break;
    }
    if (i == contacts_count)
    {
        if (ship->timer[T_MAINTENANCE] == 0)
        {
            ship->timer[T_MAINTENANCE] = 300;
            send_message_to_debug_char("Lost escortee!\r\n");
        }
        return false;
    }

    // found our escortee
    ship->timer[T_MAINTENANCE] = 0;
    float e_range = contacts[i].range;
    float e_bearing = contacts[i].bearing;
    if (check_dir_for_land_from(ship->x, ship->y, e_bearing, e_range))
    { // we have a land between us and escort, lets find a way to go around it
        if (!go_around_land(escort))
            return false;
        return true;
    }
    if (e_range > 5)
    {
        new_heading = calc_intercept_heading (e_bearing, escort->heading);
        if (check_dir_for_land_from(ship->x, ship->y, new_heading, 5))
            new_heading = e_bearing;
        send_message_to_debug_char("Chasing escortee: ");
    }
    else
    {
        speed_restriction = escort->speed;
        new_heading = escort->heading;
        send_message_to_debug_char("Following escortee: ");
    }
    set_new_dir();
    return true;
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
    send_message_to_debug_char("Could not find the current target\r\n");
    return false;
}

bool NPCShipAI::find_new_target()
{
    for (int i = 0; i < contacts_count; i++) 
    {
        if (ship == npc_dreadnought && ship->timer[T_BSTATION] == 0 && !ISNPCSHIP(contacts[i].ship))
        { // chance to ignore player's if not too close
            if (contacts[i].range > 30)
                continue;
            if (contacts[i].range > 10 
                && (contacts[i].ship->m_class == SH_SLOOP || contacts[i].ship->m_class == SH_YACHT ||
                number(0, (int)contacts[i].range * 4) > 0))
            {
                continue;
            }
        }
        if (is_valid_target(contacts[i].ship))
        {
            ship->target = contacts[i].ship;
            update_target(i);
            ship->timer[T_MAINTENANCE] = 0;
            send_message_to_debug_char("Found new target: %s\r\n", contacts[i].ship->id);
            return true;
        }
    }
    return false;
}
void NPCShipAI::update_target(int i) // index in contacts
{
    t_contact = i;
    t_bearing = contacts[i].bearing;
    t_range = contacts[i].range;
    t_x = contacts[i].x;
    t_y = contacts[i].y;
    t_arc = get_arc(ship->heading, t_bearing);
    advanced |= -isverge(contacts[i].ship);
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
    if (tar == ship->target)
        return true;

    if (ship == npc_dreadnought) // Revenge attacks everything
        return true;
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
    if ((int)ship->x != t_x || (int)ship->y != t_y)
        return false;
    return true;
}

void NPCShipAI::board_target()
{
    send_message_to_debug_char("=============BOARDING TARGET=============\r\n");
    did_board = ship->target;

    if (!crew_data)
        return;

    int grunt_count = 0;
    for (; grunt_count < 5; grunt_count++)
        if (crew_data->outer_grunts[grunt_count] == 0)
            break;
    if (!grunt_count)
        return;

    if (ship == npc_dreadnought)
    {
        act_to_all_in_ship(ship->target, "&+YA group of &+Rr&+ra&+Rv&+ra&+Rg&+ri&+Rn&+rg &+Rd&+re&+Rm&+ro&+Rn&+rs &+Yjust &=LWboarded&N &+Yyour ship!&N\r\n");
    }
    else
    {
        if (type == NPC_AI_PIRATE)
            act_to_all_in_ship(ship->target, "&+YA group of &+Rs&+ra&+Rv&+ra&+Rg&+re &+Rp&+ri&+Rr&+ra&+Rt&+re&+Rs &+Yjust &=LWboarded&N &+Yyour ship in search of valuables!&N\r\n");
        else
            act_to_all_in_ship(ship->target, "&+YA group of &+Rs&+ra&+Rv&+ra&+Rg&+re &+Rp&+ri&+Rr&+ra&+Rt&+re&+Rs &+Yjust &=LWboarded&N &+Yyour ship!&N\r\n");
    }
    
    int j;
    for (j = 0; j < MAX_SHIP_ROOM; j++) 
    {
        bool ok = false;
        for (int k = 0; k < NUM_EXITS; k++) 
        {
            if (SHIPROOMEXIT(ship->target, j, k) != -1)
            {
                ok = true;
                break;
            }
        }
        if (!ok) break;
    } // j is number of used room

    int board_count = j * 0.75;
    if (crew_data->level > 2) board_count--;
    int room_no = 0;
    if (grunt_count)
    {
        while (board_count)
        {
            int grunt = crew_data->outer_grunts[number(0, grunt_count - 1)];
            if (!load_npc_ship_crew_member(ship->target, ship->target->bridge + room_no, grunt)) 
                return;
            board_count--;
            room_no = number(0, j - 1);
        }
    }

    if (type == NPC_AI_PIRATE)
    {
        for (int i = 0; i < MAXSLOTS; i++) // TODO: not all maybe, not instantly etc
        {
            if (ship->target->slot[i].type == SLOT_CARGO || ship->target->slot[i].type == SLOT_CONTRABAND)
                ship->target->slot[i].clear();
        }
        ship->target = 0;
        mode = NPC_AI_LEAVING;
        ship->timer[T_MAINTENANCE] = 300;
    }
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
    send_message_to_debug_char("Charging target: ");
    set_new_dir();
    return true;
}

bool NPCShipAI::chase()
{
    if (SHIPIMMOBILE(ship)) return false;
    new_heading = calc_intercept_heading (t_bearing, ship->target->heading);
    if (check_dir_for_land_from(ship->x, ship->y, new_heading, 5))
        new_heading = t_bearing;
    since_last_fired_right = 0;
    send_message_to_debug_char("Intercepting: ");
    return true;
}

bool NPCShipAI::go_around_land(P_ship dest)
{
    if (SHIPIMMOBILE(ship)) return false;

    vector<int> route;
    if (!dijkstra(ship->location, dest->location, valid_ship_edge, route))
    {
        send_message_to_debug_char("Going around land failed!\r\n");
        return false;
    }
    if (route.size() == 0)
    {
        send_message_to_debug_char("\r\nempty route found!");
        send_message_to_debug_char("Going around land failed!\r\n");
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
        send_message_to_debug_char("Going around land failed!\r\n");
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

float NPCShipAI::calc_intercept_heading(float tb, float th)
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

    float new_h = (tb + th) / 2;
    normalize_direction(new_h);
    return new_h;
}

bool NPCShipAI::worth_ramming()
{
    if ((float)SHIPHULLWEIGHT(ship->target) / (float)SHIPHULLWEIGHT(ship) >= 1.5)
        return false;

    if (ship->armor[SIDE_FORE] == 0 || 
        ship->armor[SIDE_STAR] == 0 || 
        ship->armor[SIDE_PORT] == 0 || 
        ship->maxarmor[SIDE_FORE] / ship->armor[SIDE_FORE] >= 2 ||
        ship->maxarmor[SIDE_STAR] / ship->armor[SIDE_STAR] >= 2 ||
        ship->maxarmor[SIDE_PORT] / ship->armor[SIDE_PORT] >= 2 || advanced < 0)
    {
        return false;
    }

    return true;
}

int check_ram_arc(float heading, float bearing, float size);
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

    if (!advanced && !is_boardable(ship->target) && number(1, 3) > 1)
        return false; // dumb ones ram less in combat

    return true;
}

void NPCShipAI::ram_target()
{
    send_message_to_debug_char("\r\nRamming!\r\n");
    if (try_ram_ship(ship, ship->target, t_bearing))
    {
        //if (SHIPIMMOBILE(ship->target) && !did_board)
        if (ship->target && is_boardable(ship->target) && did_board != ship->target)
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
    if (!ship->target)
        return;

    // we have our target in sight, lets try firing something
    b_attack();

    new_heading = ship->heading;
    b_check_weapons();

    if (ship->target->armor[s_arc] == 0 && ship->target->internal[s_arc] == 0)
    { // aha, he is immobile and faces us with destroyed side, must find way around him
        b_circle_around_arc(s_arc);
    }
    else
    {
        if (b_turn_active_weapon()) // trying to turn a weapon that is ready/almost ready to fire and within good range already
        { }
        //else if(too_far && chase()) // if there is weapon ready to fire, but we are not in a good range
        //{ }
        else if(too_close && b_make_distance(too_close)) // if there is weapon ready to fire, but we are too close, try breaking distance
        { }
        else if (b_turn_reloading_weapon()) // trying to turn: a weapon that is closest to ready and within good range already
        { }
        else
        {
            chase();
        }
        //{
        //   send_message_to_debug_char("Nothing works in basic mode, running!\r\n");
        //    mode = NPC_AI_RUNNING;
        //    return;
        //}
    }
    set_new_dir();
}

void NPCShipAI::b_attack()
{    // if we have a ready gun pointing to target in range, fire it right away!
    for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
    {
        if (ship->slot[w_num].type == SLOT_WEAPON) 
        {
            if (!weapon_ready_to_fire(ship, w_num))
                continue;
            int w_index = ship->slot[w_num].index;
            if (ship->slot[w_num].position == t_arc &&
                t_range > (float)weapon_data[w_index].min_range && 
                t_range < (float)weapon_data[w_index].max_range)
            {
                ship->setheading = ship->heading;
                fire_weapon(ship, w_num, t_contact, debug_char);
            }
        }
    }
    if (is_multi_target)
    {
        for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
        {
            if (ship->slot[w_num].type != SLOT_WEAPON) 
                continue;
            if (!weapon_ready_to_fire(ship, w_num))
                continue;
            int w_index = ship->slot[w_num].index;
            for (int i = 0; i < contacts_count; i++) 
            {
                if (contacts[i].ship == ship->target || !is_valid_target(contacts[i].ship))
                    continue;
                int t_a = get_arc(ship->heading, contacts[i].bearing);
                if (ship->slot[w_num].position == t_a &&
                    contacts[i].range > (float)weapon_data[w_index].min_range && 
                    contacts[i].range < (float)weapon_data[w_index].max_range)
                {
                    ship->setheading = ship->heading;
                    fire_weapon(ship, w_num, i, debug_char);
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

            float good_range = MAX((float)weapon_data[w_index].min_range + 1, (float)weapon_data[w_index].max_range * 0.25);

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
            land_dist[i] = check_dir_for_land_from(t_x, t_y, ship->target->heading + get_arc_central_bearing(i), 7);
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
    float dst_dir = ship->target->heading + get_arc_central_bearing(best_dir);
    normalize_direction(dst_dir);
    int dst_loc = get_room_in_direction_from(t_x, t_y, dst_dir, max_dist);

    send_message_to_debug_char("Circling around (%6.2f, %d, %d): ", dst_dir, max_dist, dst_loc);

    // TODO: check if there is land between you and dst_loc, and try to go straight there

    vector<int> route;
    if (!dijkstra(ship->location, dst_loc, valid_ship_edge, route))
    {
        send_message_to_debug_char("\r\nfailed dijkstra! ");
        return false;
    }
    if (route.size() == 0)
    {
        send_message_to_debug_char("\r\nempty route found!");
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
            if (advanced < 0) arc_priority[i] &= -2;
            float n_h = t_bearing - get_arc_central_bearing(arc_priority[i]);
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
            float n_h = t_bearing - get_arc_central_bearing(i);
            if (is_heavy_ship && i == SLOT_REAR && ((n_h - ship->heading) > 60 || (n_h - ship->heading) < -60))
                continue; // not turning heavy ship's rear (TODO: check if rear is the only remaining side)
            best_time = active_arc[i];
            best_arc = i;
        }
    }
    if (best_arc != -1)
    {
        if (advanced < 0) best_arc &= -2;
        new_heading = t_bearing - get_arc_central_bearing(best_arc);
        send_message_to_debug_char("Turning reloading weapon arc %s: ", get_arc_name(best_arc));
        return true;
    }
    return false;
}


bool NPCShipAI::b_make_distance(float distance)
{
    if (SHIPIMMOBILE(ship)) return false;
    if (t_range > distance) return true;

    send_message_to_debug_char("Breaking distance: ");
    
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

    send_message_to_debug_char(" failed\r\n");
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

    int maxspeed = ship->get_maxspeed();
    ship->setspeed = maxspeed;
    ship->setheading = new_heading;

    if (advanced == 1)
    {
        float delta = ABS(ship->heading - new_heading);
        if (delta > 180) delta = 360 - delta;
        if (delta >= 90)
            ship->setspeed = BOARDING_SPEED + 1;
        else if (delta >= 60)
            ship->setspeed = MAX(BOARDING_SPEED + 1, maxspeed / 2);
    }

    ship->setspeed = MIN(ship->setspeed, maxspeed);
    ship->setspeed = MIN(ship->setspeed, safe_speed);
    if (speed_restriction != -1)
        ship->setspeed = MIN(ship->setspeed, speed_restriction);

    send_message_to_debug_char("heading=%d, speed=%d\r\n", (int)ship->setheading, ship->setspeed);
}




void NPCShipAI::b_set_arc_priority(float current_bearing, int current_arc, int* arc_priority)
{
    switch(current_arc)
    {
    case SIDE_FORE:
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
    case SIDE_STAR:
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
    case SIDE_PORT:
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
    case SIDE_REAR:
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
    b_attack();
    b_check_weapons();

    send_message_to_debug_char("(Immobile) ");
    if (!b_turn_active_weapon()) // trying to turn a weapon that is ready/almost ready to fire and within good range already
    {
        if (!b_turn_reloading_weapon())
        {
            // TODO: anchor maybe?
            send_message_to_debug_char("Nothing to do\n");
            return;
        }
    }
    set_new_dir();
}






/////////////////////////
// ADVANCED COMBAT //////
/////////////////////////

void NPCShipAI::advanced_combat_maneuver()
{
    if (!ship->target)
        return;


    if (t_range > 10) // TODO: more smart decision
    { // no point to maneuver around yet
        b_attack();
        chase();
    }
    else
    {
        a_attack();
        a_update_side_props();
        a_predict_target(3);
        a_update_target_side_props();
        a_choose_target_side();
        send_message_to_debug_char("t_r=(%5.2f-%5.2f),t_ld={%5.2f,%5.2f,%5.2f,%5.2f},c_a={%5.2f,%5.2f,%5.2f,%5.2f},p_a={%5.2f,%5.2f,%5.2f,%5.2f},c_p={%5.2f,%5.2f},p_p={%5.2f,%5.2f},p_r=%5.2f,p_sb=%d,p_tb=%d,hdc=%5.2f\r\n", t_min_range, t_max_range, tside_props[0].land_dist, tside_props[1].land_dist, tside_props[2].land_dist, tside_props[3].land_dist, curr_angle[0], curr_angle[1], curr_angle[2], curr_angle[3], proj_angle[0], proj_angle[1], proj_angle[2], proj_angle[3], curr_x, curr_y, proj_x, proj_y, proj_range, proj_sb, proj_tb, hd_change);

        a_calc_rotations();
        a_choose_rotation();
        send_message_to_debug_char("Circling: t_side=%s(%c), w_side=%s, rot=%s,", get_arc_name(target_side), within_target_side ? 'y' : 'n', get_arc_name(chosen_side), (chosen_rot == 1) ? "direct" : ((chosen_rot == -1) ? "counter" : "unknown") );
        if (!a_immediate_turn())
            a_choose_dest_point();
    }
    set_new_dir();
}


void NPCShipAI::a_attack()
{
    char to_fire[MAXSLOTS], can_fire_but_not_right = 0;
    send_message_to_debug_char("Weapons:");
    for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
    {
        to_fire[w_num] = 0;
        if (ship->slot[w_num].type == SLOT_WEAPON) 
        {
            if (!weapon_ready_to_fire(ship, w_num))
                continue;
            int w_index = ship->slot[w_num].index;  
            if (ship->slot[w_num].position == t_arc &&
                t_range > (float)weapon_data[w_index].min_range && 
                t_range < (float)weapon_data[w_index].max_range)
            {
                can_fire_but_not_right = 1;

                float hit_arc = weapon_data[w_index].hit_arc;
                if (w_index == W_FRAG_CAN)
                    hit_arc = 360; // doesnt matter where to fire from
                float arc_width = get_arc_width(target_side);
                float min_intersect = MIN(MIN(hit_arc, (hit_arc / 2 + 10)), arc_width / 2);

                float intersect = 0;
                {
                    float rbearing = s_bearing - ship->target->heading; // how target sees you, relatively to direction
                    normalize_direction(rbearing);

                    float arc_center = get_arc_central_bearing(target_side);
                    float arc_cw = arc_center + arc_width / 2;
                    normalize_direction(arc_cw);
                    float arc_ccw = arc_center - arc_width / 2;
                    normalize_direction(arc_ccw);

                    if ((arc_cw >= arc_ccw && (rbearing > arc_ccw && rbearing < arc_cw)) || (arc_cw < arc_ccw && (rbearing > arc_ccw || rbearing < arc_cw)))
                    { // center inside arc
                        float ccw_diff = rbearing - arc_ccw;
                        if (ccw_diff < 0) ccw_diff += 360;
                        float cw_diff = arc_cw - rbearing;
                        if (cw_diff < 0) cw_diff += 360;
                        intersect = MIN(ccw_diff, hit_arc / 2) + MIN(cw_diff, hit_arc / 2);
                    }
                    else
                    {
                        float ccw_diff = arc_ccw - rbearing;
                        if (ccw_diff < 0) ccw_diff += 360;
                        float cw_diff = rbearing - arc_cw;
                        if (cw_diff < 0) cw_diff += 360;
                        intersect = MAX(hit_arc / 2 - ccw_diff, 0) + MAX(hit_arc / 2 - cw_diff, 0);
                    }
                    send_message_to_debug_char("  %d:%d", w_num, intersect);
                }

                if (intersect >= min_intersect)
                {
                    to_fire[w_num] = 1;
                    since_last_fired_right = 0;
                    can_fire_but_not_right = 0;
                    send_message_to_debug_char("!");
                }

                if (intersect== 0 && (is_heavy_ship || since_last_fired_right > number(30, 180))) // if intersect not zero, we should try a bit more
                {
                    if ((ship->target->armor[s_arc] + ship->target->internal[s_arc]) > 0 || weapon_data[w_index].hit_arc > 180)
                    {
                        to_fire[w_num] = 1;
                        send_message_to_debug_char("!");
                    }
                }
            }
            else
            {
                send_message_to_debug_char("  %d:X", w_num);
            }
        }
    }
    send_message_to_debug_char("  sfr:%d\r\n", since_last_fired_right);
    for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
    {
        if (to_fire[w_num])
        {
            ship->setheading = ship->heading;
            int hit_chance = weaponsight(ship, w_num, t_contact, debug_char);
            if (hit_chance > 50)
                fire_weapon(ship, w_num, t_contact, hit_chance, debug_char);
        }
    }
    if (can_fire_but_not_right)
    {
        since_last_fired_right++;
    }

    if (is_multi_target) // ok now lets see if we should fire on something else
    {
        for (int w_num = 0; w_num < MAXSLOTS; w_num++) 
        {
            if (ship->slot[w_num].type != SLOT_WEAPON) 
                continue;
            if (!weapon_ready_to_fire(ship, w_num))
                continue;
            int w_index = ship->slot[w_num].index;
            for (int i = 0; i < contacts_count; i++) 
            {
                if (contacts[i].ship == ship->target || !is_valid_target(contacts[i].ship))
                    continue;
                int t_a = get_arc(ship->heading, contacts[i].bearing);
                if (ship->slot[w_num].position != t_a)
                    continue;

                int hit_chance = weaponsight(ship, w_num, i, debug_char);
                if (hit_chance < 50)
                    continue;

                bool fire = false;
                if (t_range > weapon_data[w_index].max_range || t_range < weapon_data[w_index].min_range)
                    fire = true;  // main target is not in range anyways, fire it

                if (t_a == (t_arc + 2) % 4)
                    fire = true;  // main target is at opposite arc, fire away

                if (IS_SET(weapon_data[w_index].flags, MINDBLAST))
                {
                    if (contacts[i].ship->timer[T_MINDBLAST] == 0)
                        fire = true; // use mindblast whenever possible
                    else
                        continue; // no point...
                }

                if (fire)
                {
                    ship->setheading = ship->heading;
                    fire_weapon(ship, w_num, i, hit_chance, debug_char);
                }
            }
        }
    }
}


void NPCShipAI::a_predict_target(int steps)
{
    float hd = ship->target->heading;
    curr_x = (float)t_x + (ship->target->x - 50.0);
    curr_y = (float)t_y + (ship->target->y - 50.0);
    curr_angle[SLOT_FORE] = hd;
    curr_angle[SLOT_STAR] = hd + 90;
    curr_angle[SLOT_PORT] = hd - 90;
    curr_angle[SLOT_REAR] = hd + 180;
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

    proj_angle[SLOT_FORE] = hd;
    proj_angle[SLOT_STAR] = hd + 90;
    proj_angle[SLOT_PORT] = hd - 90;
    proj_angle[SLOT_REAR] = hd + 180;
    for (int i = 0; i < 4; i++) normalize_direction(proj_angle[i]);

    float proj_delta_x = ship->x - proj_x;
    float proj_delta_y = ship->y - proj_y;
    proj_range = sqrt(proj_delta_x * proj_delta_x + proj_delta_y * proj_delta_y);
    proj_sb = acos(proj_delta_y / proj_range) / M_PI * 180.0;
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
                if (side_props[pos].good_range > (float)weapon_data[w_index].max_range * 0.25)
                    side_props[pos].good_range = (float)weapon_data[w_index].max_range * 0.25;
                if (side_props[pos].min_range > weapon_data[w_index].min_range)
                    side_props[pos].min_range = weapon_data[w_index].min_range;
                if (min_range_total > weapon_data[w_index].min_range)
                    min_range_total = weapon_data[w_index].min_range;
            }
            else 
            {
                if(side_props[pos].ready_timer > ship->slot[w_num].timer)
                    side_props[pos].ready_timer = ship->slot[w_num].timer;
                if (side_props[pos].good_range > (float)weapon_data[w_index].max_range * 0.25) // still want to know it to get into right distance for reloading guns if none ready
                    side_props[pos].good_range = (float)weapon_data[w_index].max_range * 0.25;
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

void NPCShipAI::a_choose_target_side() // TODO: choose another one if too close to land?
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
    float cw = proj_angle[target_side] + delta;
    if (cw > 360) cw -= 360;
    float ccw = proj_angle[target_side] - delta;
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
    send_message_to_debug_char("cw_cw=%d, cw_ccw=%d, ccw_cw=%d, ccw_ccw=%d\r\n", cw_cw, cw_ccw, ccw_cw, ccw_ccw);
}

void NPCShipAI::a_choose_rotation()
{
    float proj_tb_rel = proj_tb - ship->heading;
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

    // TODO: take side's weapon power into account
    if (side_ok[SLOT_PORT] && (port_count >  star_count || !side_ok[SLOT_STAR]))
    {
        chosen_side = SLOT_PORT;
        chosen_rot = port_dir;
        send_message_to_debug_char("choice 1\r\n");
        return;
    }
    if (side_ok[SLOT_STAR] && (star_count >= port_count || !side_ok[SLOT_PORT]))
    {
        chosen_side = SLOT_STAR;
        chosen_rot = star_dir;
        send_message_to_debug_char("choice 2\r\n");
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
        send_message_to_debug_char("choice 3\r\n");
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
        send_message_to_debug_char("choice 4\r\n");
        return;
    }

    if (side_props[SLOT_REAR].ready_timer != INT_MAX || side_props[SLOT_FORE].ready_timer != INT_MAX)
    {
        chosen_rot = cnt_dir;
        if (side_props[SLOT_FORE].ready_timer > side_props[SLOT_REAR].ready_timer)
            chosen_side = SLOT_REAR;
        else
            chosen_side = SLOT_FORE;
        send_message_to_debug_char("choice 5\r\n");
        return;
    }

        send_message_to_debug_char("no choice!\r\n");
    // TODO: no weapons?
}

bool NPCShipAI::a_immediate_turn()
{
    int delta = (target_side == SLOT_FORE || target_side == SLOT_REAR) ? 30 : 40;
    float cw = curr_angle[target_side] + delta;
    if (cw > 360) cw -= 360;
    float ccw = curr_angle[target_side] - delta;
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
                send_message_to_debug_char(" immediate turn to fire fore!\r\n");
                return true;
            }
        }
        if (chosen_side == SLOT_REAR && side_props[SLOT_REAR].ready_timer == 0 && side_props[SLOT_REAR].max_range > proj_range)
        {
           new_heading = s_bearing; // turning from target to fire rear
           send_message_to_debug_char(" immediate turn to fire rear!\r\n");
           return true;
        }
        if (chosen_side == SLOT_PORT && side_props[SLOT_PORT].ready_timer == 0 && side_props[SLOT_PORT].max_range > proj_range)
        {
           new_heading = t_bearing + 90;
           send_message_to_debug_char(" immediate turn to fire port!\r\n");
           return true;
        }
        if (chosen_side == SLOT_STAR && side_props[SLOT_STAR].ready_timer == 0 && side_props[SLOT_STAR].max_range > proj_range)
        {
           new_heading = t_bearing - 90;
           send_message_to_debug_char(" immediate turn to fire starboard!\r\n");
           return true;
        }
    }
    return false;
}

void NPCShipAI::a_choose_dest_point()
{
    float dest_angle = proj_sb + ((chosen_rot == 1) ? 30 : -30);
    normalize_direction(dest_angle);
    float rad = dest_angle * M_PI / 180.000;

    // TODO: base it on next arc's properties, not general
    float chosen_range = side_props[chosen_side].good_range;
    if (side_props[chosen_side].min_range > 0)
    {
        if (chosen_side == SIDE_FORE)
            chosen_range = side_props[chosen_side].min_range + 3;
        else if (chosen_side == SIDE_REAR)
            chosen_range = side_props[chosen_side].min_range;
        else
            chosen_range = side_props[chosen_side].min_range + 1;
    }
    else
    {
        if (t_min_range && t_min_range - 1 <= chosen_range) // TODO: priority between catapults and short-ranged somehow
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
        new_heading = atan((dest_x - ship->x) / (dest_y - ship->y)) / M_PI * 180.0;
        if (dest_y < ship->y)
            new_heading += 180;
    }
    normalize_direction(new_heading);
    send_message_to_debug_char(" rng=%5.2f\r\n", chosen_range);
}







//////////////////////////////////
// UTILITIES /////////////////////
//////////////////////////////////



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
    //send_message_to_debug_char("\r\ndsin=%5.2f,dcos=%5.2f", dir_sin, dir_cos);
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
        //send_message_to_debug_char("\r\nnx=%5.2f,ny=%5.2f,", next_x, next_y);
        //send_message_to_debug_char("cx=%d,cy=%d,", x_to_check, y_to_check);

        if (world[tactical_map[x_to_check][100 - y_to_check].rroom].sector_type != SECT_OCEAN && !IS_SET(ship->flags, AIR)) 
        { // next room is a land
            //send_message_to_debug_char(" %5.2f\r\n", range);
            return range;
        }

        float delta_y = next_y - y;
        float delta_x = next_x - x;
        //send_message_to_debug_char("dx=%5.2f,dy=%5.2f,", delta_x, delta_y);

        if (dir_cos == 0)
        {
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
    //send_message_to_debug_char(" none\r\n");
    return max_range;
}

// returns the distance to land or 0 if none
int NPCShipAI::check_dir_for_land_from(float cur_x, float cur_y, float heading, float range)
{ // tactical_map is supposed to be filled already
    float rad = heading * M_PI / 180.000;
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

bool NPCShipAI::inside_map(float x, float y)
{
    if ((int)x < 0 && (int)x > 100) return false;
    if ((int)y < 0 && (int)y > 100) return false;
    return true;
}

int NPCShipAI::get_room_in_direction_from(float x, float y, float dir, float range)
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

bool NPCShipAI::get_coord_in_direction_from(float x, float y, float dir, float range, float& rx, float& ry)
{
    float rad = (float) ((float) (dir) * M_PI / 180.000);
    rx = x + sin(rad) * range;
    ry = y + cos(rad) * range;

    if (!inside_map(rx, ry)) 
        return false;
    
    return true;
}

void NPCShipAI::send_message_to_debug_char(const char *fmt, ... )
{
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);

    send_to_char(buf, debug_char);
}



// TODO:
// Take damage_ready into account
// add target's side-per-time statistic, switch target side if too many
// make sure people dont attacked twice on same cargo run??
// Validate cargo!
// Pirate Crews: lower levels, remove necros, set di!

// endless loop somewhere in advanced ai?
// check for ships name in use already
// reduce chance for bloodstones