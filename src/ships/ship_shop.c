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
#include "nexus_stones.h"
#include "epic.h"
#include "ships.h"
#include "epic_bonus.h"
#include "epic_skills.h"
#include "defines.h"
#include "spells.h"
#include "structs.h"
#include "utility.h"
#include "../achievements.h"

extern char buf[MAX_STRING_LENGTH];
extern char arg1[MAX_STRING_LENGTH];
extern char arg2[MAX_STRING_LENGTH];
extern char arg3[MAX_STRING_LENGTH];
extern P_char character_list;
extern const char *rude_ass[];
extern const int davy_jones_locker_rnum;
extern const int ship_transit_rnum;

int list_cargo(P_char ch, P_ship ship, bool owned)
{
  int portnum, cost, profit;
  bool caption;

  if( !owned )
  {
      send_to_char("&+gYou do not own a ship, and we can't strap cargo to your back.&n\n", ch);
      return TRUE;
  }

  portnum = -1;
  while( ++portnum < NUM_PORTS )
  {
    if( ports[portnum].loc_room == world[ch->in_room].number )
      break;
  }
  if( portnum >= NUM_PORTS )
  {
      send_to_char ("&+gThere is no cargo for sale here!&n\n", ch);
      return TRUE;
  }

  send_to_char ("&+y---=== For sale ===---&N\n", ch);

  cost = cargo_sell_price(portnum);
  // hook for epic bonus
  cost -= (int) (cost * get_epic_bonus(ch, EPIC_BONUS_CARGO));

  send_to_char_f(ch, "%s %s per crate.\n", pad_ansi(cargo_type_name(portnum), 30).c_str(), coin_stringv(cost, 3));

  send_to_char("\n&+y---=== We're buying ===---&n\n", ch);
  for( int i = 0; i < NUM_PORTS; i++ )
  {
    cost = cargo_buy_price(portnum, i);

    send_to_char_f(ch, "%s %s per crate.\n", pad_ansi(cargo_type_name(i), 30).c_str(), coin_stringv(cost, 3));
  }

  send_to_char("\n&+gTo buy cargo, type '&+Gbuy cargo <number of crates>&+g'&n\n", ch);
  send_to_char("&+gTo sell cargo, type '&+Gsell cargo&+g'&n\n", ch);

  if( GET_ALIGNMENT(ch) <= MINCONTRAALIGN )
  {
    if( can_buy_contraband(ship, portnum) )
    {
      send_to_char("\n&+L---=== Contraband for sale ===---&N\n", ch);

      cost = contra_sell_price(portnum);
	    // hook for epic bonus
	    cost -= (int) (cost * get_epic_bonus(ch, EPIC_BONUS_CARGO));

      send_to_char_f(ch, "%s %s &+Lper crate.&n\n", pad_ansi(contra_type_name(portnum), 30).c_str(), coin_stringv(cost, 3));
    }

    caption = FALSE;
    for( int i = 0; i < NUM_PORTS; i++ )
    {
      if( can_buy_contraband(ship, i) )
      {
        if( !caption )
        {
          send_to_char("\n&+L---=== We buy contraband for ===---&N\n", ch);
          caption = TRUE;
        }
        cost = contra_buy_price(portnum, i);
        send_to_char_f(ch, "%s %s &+Lper crate.&n\n", pad_ansi(contra_type_name(i), 30).c_str(), coin_stringv(cost));
      }
    }
    if( caption )
    {
      send_to_char("\n&+LTo buy contraband, type '&+Gbuy contraband <number of crates>&+L'&n\n"
        "&+LTo sell contraband, type '&+Gsell contraband&+L'.&n\n", ch);
    }
  }

  send_to_char("\n&+cYour cargo manifest&n\n&+W-------------------&n\n", ch);

  for( int i = 0; i < MAXSLOTS; i++ )
  {
    // Less than 1 crate.
    if( ship->slot[i].val0 < 1 )
      continue;

    // If it's cargo and more than 0 crates.
    if( ship->slot[i].type == SLOT_CARGO )
    {
      /* We used to show manifest by crate.
      if( cargo_type == portnum )
      {
        send_to_char_f(ch, "%s&n, &+W%d&n crates, bought for %s&n.\n",
          cargo_type_name(cargo_type), ship->slot[i].val0, coin_stringv(ship->slot[i].val1));
      }
      else
      {
      */
      // The cost is the price per crate * number of crates.
      cost = cargo_buy_price(portnum, ship->slot[i].index) * ship->slot[i].val0;
      if( ship->slot[i].val1 > 0 )
      {
        profit = (int)(( ((float)cost / (float)ship->slot[i].val1) - 1.00 ) * 100.0);
        send_to_char_f( ch, " %s &+Y%2d&n crates.\n",
          pad_ansi( cargo_type_name(ship->slot[i].index), 30).c_str(), ship->slot[i].val0 );
        send_to_char_f( ch, "   Paid %s.\n", coin_stringv(ship->slot[i].val1, 4) );
        send_to_char_f( ch, "   Sell %s (%d%% profit).\n", coin_stringv(cost, 4), profit );
      }
      else
      {
        send_to_char_f( ch, " %s &+Y%2d&n crates.\n",
          pad_ansi( cargo_type_name(ship->slot[i].index), 30).c_str(), ship->slot[i].val0 );
        send_to_char_f( ch, "   Paid NOTHING.\n" );
        send_to_char_f( ch, "   Sell %s.\n", coin_stringv(cost, 4) );
      }
    }
    else if( ship->slot[i].type == SLOT_CONTRABAND )
    {
      /* We used to show manifest by crate.
      if( ship->slot[i].index == portnum )
      {
        send_to_char_f(ch, "&+L*&n%s&n, &+Y%d&n crates, bought for %s.\n",
          contra_type_name(ship->slot[i].index), ship->slot[i].val0, coin_stringv(ship->slot[i].val1));
      }
      else
      {
      */
      // Price per crate * num crates.
      cost = contra_buy_price(portnum, ship->slot[i].index) * ship->slot[i].val0;

      if( ship->slot[i].val1 > 0 )
      {
        profit = (int)(( ((float)cost / (float)ship->slot[i].val1) - 1.00 ) * 100.0);
        send_to_char_f( ch, " %s &+Y%2d&n crates.\n",
          pad_ansi( contra_type_name(ship->slot[i].index), 30).c_str(), ship->slot[i].val0 );
        send_to_char_f( ch, "   Paid %s.\n", coin_stringv(ship->slot[i].val1, 4) );
        send_to_char_f( ch, "   Sell %s (%d%% profit).\n", coin_stringv(cost, 4), profit );
      }
      else
      {
        send_to_char_f( ch, " %s &+Y%2d&n crates.\n",
          pad_ansi( contra_type_name(ship->slot[i].index), 30).c_str(), ship->slot[i].val0 );
        send_to_char_f( ch, "   Paid NOTHING.\n" );
        send_to_char_f( ch, "   Sell %s.\n", coin_stringv(cost, 4) );
      }
    }
  }

  send_to_char_f(ch, "\n&+gShip Capacity: &+W%d&+g/&+W%d&n\n", SHIP_AVAIL_CARGO_LOAD(ship), SHIP_MAX_CARGO_LOAD(ship));

  return TRUE;
}

int list_weapons(P_char ch, P_ship ship, int owned)
{
    char rng[20], dam[20];
    if( !owned )
    {
        send_to_char("&+gBut you do not own a ship!&n\n", ch);
        return TRUE;
    }

    send_to_char("&+gWeapons\n", ch);
    send_to_char("&+y/============================================================================================\\\n", ch);
    send_to_char("&+y|&+Y    Name             Weight  Range   Damage Ammo Disper Sail Hull Sail Reload  Cost         |\n", ch);
    send_to_char("&+y|&+Y                                                   sion  Hit  Dam  Dam   Time               |\n", ch);
    send_to_char("&+y\\--------------------------------------------------------------------------------------------/\n", ch);
    for( int i = 0; i < MAXWEAPON; i++ )
    {
        sprintf(rng, "%d-%d", weapon_data[i].min_range, weapon_data[i].max_range);
        if (weapon_data[i].fragments > 1)
        {
            if (weapon_data[i].min_damage == weapon_data[i].max_damage)
                sprintf(dam, "%d x %d", weapon_data[i].fragments, weapon_data[i].min_damage);
            else
                sprintf(dam, "%d x %d-%d", weapon_data[i].fragments, weapon_data[i].min_damage, weapon_data[i].max_damage);
        }
        else
        {
            if (weapon_data[i].min_damage == weapon_data[i].max_damage)
                sprintf(dam, "%d", weapon_data[i].min_damage);
            else
                sprintf(dam, "%d-%d", weapon_data[i].min_damage, weapon_data[i].max_damage);
        }

        send_to_char_f(ch,  " %-2d) &+%c%-20s &+Y%2d  %5s  %7s   %2d    %3d %3d%% %3d%% %3d%%    %3d  &n%10s&n\n",
          i + 1,
          ship_allowed_weapons[ship->m_class][i] && (ship->frags >= weapon_data[i].min_frags || ship->crew.guns_skill >= weapon_data[i].min_crewexp) ? 'W' : 'L',
          weapon_data[i].name,
          weapon_data[i].weight,
          rng,
          dam,
          weapon_data[i].ammo,
          weapon_data[i].hit_arc,
          weapon_data[i].sail_hit,
          weapon_data[i].hull_dam,
          weapon_data[i].sail_dam,
          weapon_data[i].reload_time,
          coin_stringv(weapon_data[i].cost, 4));
    }

    send_to_char("\nTo buy a weapon, type '&+gbuy &+Gw&+geapon <number> <&+Gf&+gore|&+Gr&+gear|&+Gp&+gort|&+Gs&+gtarboard>&n'.\n"
      "To sell a weapon, type '&+gsell <slot number>&n'.\n"
      "To reload a weapon, type '&+greload <weapon slot number>&n'.\n\n", ch);

    return TRUE;
}

int list_equipment(P_char ch, P_ship ship, int owned)
{
  int weight, cost;

  if( !owned )
  {
    send_to_char("&+gBut you do not own a ship!&n\n", ch);
    return TRUE;
  }

  send_to_char("&+gEquipment&n\n", ch);
  send_to_char("&+y/=====================================================\\\n", ch);
  send_to_char("&+y|&+Y    Name                       Weight   Cost         &+y|\n", ch);
  send_to_char("&+y\\-----------------------------------------------------/\n", ch);

    for( int i = 0; i < MAXEQUIPMENT; i++ )
    {
      // Rams / Levistones vary with ship size I think.
      if( i == E_RAM )
        weight = eq_ram_weight(ship);
      else if( i == E_LEVISTONE)
        weight = eq_levistone_weight(ship);
      else
        weight = equipment_data[i].weight;

      // Ram cost varies with ship size too maybe?
      if( i == E_RAM )
        cost = eq_ram_cost(ship);
      else
        cost = equipment_data[i].cost;

      send_to_char_f( ch,  " %-2d) %s%s      &+Y%2d&n %s&n\n", i + 1,
        ship_allowed_equipment[ship->m_class][i] && (ship->frags >= equipment_data[i].min_frags) ? "&+W" : "&+L",
        pad_ansi(equipment_data[i].name, 25, TRUE).c_str(),
        weight, (cost == 0) ? "  &+WFREE" : coin_stringv(cost, 6) );
    }

  send_to_char("\nTo buy an equipment, type '&+gbuy &+Ge&+gquipment <number>&n'.\n"
    "To sell an equipment, type '&+gsell <slot number>&n'.\n\n", ch);
  return TRUE;
}

// Returns a  string with the value of the epic cost of the hull type
//   with preceeding spaces to make the length total 6 (excluding the terminating char).
// If epic cost is 0, just returns 6 spaces.
char *epic_cost_string( int hull_type)
{
  // We have length 8 here, just in case somewhere in the future we get in the millions.
  static char buf[8];
  int cost = SHIPTYPE_EPIC_COST(hull_type);

  if( cost == 0 )
  {
    return "      ";
  }

  sprintf( buf, "%6d", SHIPTYPE_EPIC_COST(hull_type) );
  return buf;
}

int list_hulls( P_char ch, P_ship ship, int owned )
{
    if (owned)
    {
        send_to_char("&+gHull Upgrades\n",ch);
        send_to_char("&+y/=================================================================================\\\n", ch);
        send_to_char("&+y|&+Y                 Load Cargo Passenger   Max Total  Weapons   Epic                &+y|\n", ch);
        send_to_char("&+y|&+Y    Name         Cap.  Cap.      Cap. Speed Armor (F/S/R/P)  Cost   Cost         &+y|\n", ch);
        send_to_char("&+y\\---------------------------------------------------------------------------------/\n", ch);
        for (int i = 0; i < MAXSHIPCLASS; i++)
        {
            if (SHIPTYPE_COST(i) == 0)
            {
              continue;
            }
            send_to_char_f(ch, " %-2d) %-11s  &+Y%4d  &+Y%4d       &+Y%3d  &+Y%4d  &+Y%4d  &+Y%1d/&+Y%1d/&+Y%1d/&+Y%1d &+R%s&n %s\n",
              i + 1, SHIPTYPE_NAME(i), SHIPTYPE_MAX_WEIGHT(i), SHIPTYPE_CARGO(i), SHIPTYPE_PEOPLE(i), SHIPTYPE_SPEED(i),
              ship_arc_properties[i].armor[SIDE_FORE] + ship_arc_properties[i].armor[SIDE_STAR]
              + ship_arc_properties[i].armor[SIDE_REAR] + ship_arc_properties[i].armor[SIDE_PORT],
              ship_arc_properties[i].max_weapon_slots[SIDE_FORE], ship_arc_properties[i].max_weapon_slots[SIDE_STAR],
              ship_arc_properties[i].max_weapon_slots[SIDE_REAR], ship_arc_properties[i].max_weapon_slots[SIDE_PORT],
              epic_cost_string(i), coin_stringv(SHIPTYPE_COST(i), 6));
        }

        send_to_char("\nTo upgrade/downgrade your hull, type '&+gbuy &+Gh&+gull <number>&n'.\n", ch);
        /* Selling ships disabled...
        send_to_char("To sell your ship completely, type 'sell ship'. &+RCAUTION!&n You will loose your frags and crews!\r\n", ch);
        */


        send_to_char("\n&+gRepairs\n",ch);
        send_to_char("&+y===================================================================&N\n", ch);

        if( ship->timer[T_MAINTENANCE] > 0 )
        {
          send_to_char_f(ch, "&+gYour ship is currently being worked on, please wait another &n%.1f&+g hours.&n\n",
            (float) ship->timer[T_MAINTENANCE] / 75.0);
        }
        else
        {
          send_to_char("To repair, type '&+grepair <&+Ga&+grmor|&+Gi&+gnternal|&+Gs&+gail|&+Gw&+geapon>"
            " <&+Gf&+gore|&+Gp&+gort|&+Gs&+gtarboard|&+Gr&+gear>&n'.\n", ch);
        }

        send_to_char("\n&+gRename\r\n",ch);
        send_to_char("&+y===================================================================&N\r\n", ch);
        send_to_char("To rename your ship for a price, type '&+gbuy &+Gr&+gename <new ship name>&n'.\n", ch);
    }
    else
    {
        send_to_char("&+gShips Available&n\n",ch);
        send_to_char("&+y/=================================================================================\\\n", ch);
        send_to_char("&+y|&+Y                 Load Cargo Passenger   Max Total  Weapons   Epic                &+y|\n", ch);
        send_to_char("&+y|&+Y    Name         Cap.  Cap.      Cap. Speed Armor (F/S/R/P)  Cost   Cost         &+y|\n", ch);
        send_to_char("&+y\\---------------------------------------------------------------------------------/\n", ch);
        for( int i = 0; i < MAXSHIPCLASS; i++ )
        {
          if (SHIPTYPE_COST(i) == 0)
          {
            continue;
          }
          send_to_char_f(ch, " %-2d) %-11s  &+Y%4d  &+Y%4d       &+Y%3d  &+Y%4d  &+Y%4d  &+Y%1d/&+Y%1d/&+Y%1d/&+Y%1d &+R%s&n %s\n",
            i + 1, SHIPTYPE_NAME(i), SHIPTYPE_MAX_WEIGHT(i), SHIPTYPE_CARGO(i), SHIPTYPE_PEOPLE(i), SHIPTYPE_SPEED(i),
            ship_arc_properties[i].armor[SIDE_FORE] + ship_arc_properties[i].armor[SIDE_STAR]
            + ship_arc_properties[i].armor[SIDE_REAR] + ship_arc_properties[i].armor[SIDE_PORT],
            ship_arc_properties[i].max_weapon_slots[SIDE_FORE], ship_arc_properties[i].max_weapon_slots[SIDE_STAR],
            ship_arc_properties[i].max_weapon_slots[SIDE_REAR], ship_arc_properties[i].max_weapon_slots[SIDE_PORT],
            epic_cost_string(i), coin_stringv(SHIPTYPE_COST(i), 6));
        }
        if( affected_by_spell( ch, AIP_FREESLOOP ) )
        {
          send_to_char( "\r\n&+yYou qualify for a &+WFree Sloop&+y!&n\n\r", ch );
          send_to_char( "&+yTo claim this reward, use the command '&+Gbuy hull 1 <name>&+y' where <name> is the name of your ship (ansi color allowed).&n\n\r", ch );
        }
        send_to_char("\n&+YRead HELP WARSHIP before buying one!&n\n", ch);
        send_to_char("To buy a ship, type '&+gbuy &+Gh&+gull <number> <name of ship>&n'.\n", ch);
    }
    return TRUE;
}

int summon_ship( P_char ch, P_ship ship, bool time_only )
{
  char buf[32];

  if( ship->location == ch->in_room )
  {
    send_to_char("&+gYour ship is already here!&n\n", ch);
    return TRUE;
  }
  if( IS_SET(ship->flags, SINKING) )
  {
    send_to_char("&+gWe can't summon your ship, mate, as it's sinking.&n\n", ch);
    sprintf(buf, "beer me" );
    command_interpreter(ch, buf );
    return TRUE;
  }
  if( ship->timer[T_BSTATION] > 0 && !IS_TRUSTED(ch) )
  {
    send_to_char ("&+gYour crew is too busy to respond to our summons!&n\n", ch);
    return TRUE;
  }
  if( !is_Raidable(ch, 0, 0) )
  {
    send_to_char("\n&+RGET RAIDABLE!\n", ch);
    return TRUE;
  }
  if( IS_SET(ship->flags, SUMMONED) )
  {
    if( time_only )
    {
      P_nevent ev;
      LOOP_EVENTS_OBJ( ev, ship->shipobj->nevents )
      {
        if( ev->func == summon_ship_event )
        {
          if( ev->obj == ship->shipobj )
          {
            break;
          }
        }
      }
      if( ev && ev->obj == ship->shipobj )
      {
        int time = ne_event_time(ev) / (SECS_PER_MUD_HOUR * WAIT_SEC);
        send_to_char_f(ch, "&+gYour ship should arrive in &n%d &+ghour%s.&n\n", time, (time == 1) ? "" : "s" );
      }
      else
      {
        send_to_char( "&=LrCould not find summon ship event.  You might want to tell an Immortal.\n\r", ch );
      }
    }
    else
    {
      send_to_char( "&+gThere is already an order out on your ship.&n\n", ch);
    }
    return TRUE;
  }

  if( !time_only )
  {
    int summon_cost = SHIPTYPE_HULL_WEIGHT(ship->m_class) * 50;
    if (GET_MONEY(ch) < summon_cost)
    {
      send_to_char_f(ch, "&+gIt will cost &n%s &+gto summon your ship!&n\n", coin_stringv(summon_cost));
      return TRUE;
    }
    if (ship->location == davy_jones_locker_rnum)
    {
      send_to_char("&+gYou start to call your ship back from &+LDavy Jones Locker...&n\n", ch);
    }
    SUB_MONEY(ch, summon_cost, 0);
  }

  int summontime;
  bool pvp;
  pvp = ocean_pvp_state();

  if( IS_TRUSTED(ch) )
      summontime = 1;
  else if (SHIP_CLASS(ship) == SH_SLOOP || SHIP_CLASS(ship) == SH_YACHT)
  {
    // 1/2 to 25 mud hrs.
    summontime = (SECS_PER_MUD_HOUR * WAIT_SEC * 50) / MAX(get_maxspeed_without_cargo(ship), 2);

    // If the ship was sunk, double summon time (1 to 50 mud hrs).
    if( ship->location == davy_jones_locker_rnum )
      summontime *= 2;

    // If ocean in pvp state, double summon time (1 to 100 mud hrs).
    if( pvp )
      summontime *= 2;
  }
  else
  {
    // Base of 70 mud hrs, divided by the speed of the ship (min 2).  So, between 1 and 35 mud hrs to start.
    summontime = (SECS_PER_MUD_HOUR * WAIT_SEC * 70) / MAX((get_maxspeed_without_cargo(ship) - 20), 2);
    // Add 15 mud hrs for pvp situations.
    if( pvp )
      summontime += 15 * SECS_PER_MUD_HOUR * WAIT_SEC;
  }
  // Max at 60 mud hrs.
  summontime = MIN(summontime, SECS_PER_MUD_HOUR * WAIT_SEC * 60);

  if( pvp )
    send_to_char_f(ch, "&+YDue to dangerous conditions, it will take about &n%d &+Yhours for your ship to get here.&n\n",
      summontime / (SECS_PER_MUD_HOUR * WAIT_SEC));
  else if( time_only )
    send_to_char_f(ch, "&+gIt would take %d hours for your ship to get here.\r\n",
      summontime / (SECS_PER_MUD_HOUR * WAIT_SEC));
  else
    send_to_char_f(ch, "&+gThanks for your business, it will take &n%d &+ghours for your ship to get here.&n\n",
      summontime / (SECS_PER_MUD_HOUR * WAIT_SEC));

  if( !time_only )
  {
    if(!IS_TRUSTED(ch))
    {
      clear_cargo(ship);
      write_ship(ship);
    }
    SET_BIT(ship->flags, SUMMONED);
    kick_everyone_off(ship);
    send_to_room_f(ship->location, "&+y%s is called away elsewhere.\n", ship->name);
    obj_from_room(ship->shipobj);
    obj_to_room(ship->shipobj, ship_transit_rnum );

    sprintf(buf, "%s %d", GET_NAME(ch), ch->in_room);
    add_event(summon_ship_event, summontime, NULL, NULL, ship->shipobj, 0, buf, strlen(buf)+1);
  }
  return TRUE;
}

int sell_cargo_slot(P_char ch, P_ship ship, int slot, int rroom)
{
  int type = ship->slot[slot].index;
  int crates = ship->slot[slot].val0;
  int cost = crates * cargo_buy_price(rroom, type);
  int profit = 100;
  struct affected_type *paf;

  if( type >= NUM_PORTS )
  {
    return 0;
  }

 /* Random snippet.. Supposed to stop from selling cargo at the port you buy it in I guess.
  if (type == rroom)
  {
    send_to_char_f(ch, "&+gWe don't buy &n%s &+ghere.&n\n", cargo_type_name(type));
    return 0;
  }
  */

	if( has_eq_diplomat(ship) )
	{
	  cost *= 0.9;
	}
  if( has_innate(ch, INNATE_SEADOG) )
  {
    send_to_char("Your &+csea&+Cfaring&n heritage has earned you a &+Ybonus&n in your sale...&n\r\n", ch);
    cost *= 1.10;
  }
  if( ship->slot[slot].val1 != 0 )
  {
    profit = (int)(( ((float)cost / (float)ship->slot[slot].val1) - 1.00 ) * float(profit));
  }
  ship->slot[slot].clear();

  if( IS_WARSHIP(ship) )
  {
    send_to_char("&+gBecause your cargo is obviosly stolen, local merchants bargain the price down.&n\n", ch);
    cost = cost * 0.6;
  }

  sprintf(buf, "CARGO: %s sold &+W%d&n %s&n at %s&n [%d] for %s&n (%d percent profit)", GET_NAME(ch), crates, cargo_type_name(type), ports[rroom].loc_name, ports[rroom].loc_room, coin_stringv(cost), profit);
  statuslog(56, buf);
  logit(LOG_SHIP, strip_ansi(buf).c_str());

  send_to_char_f(ch, "&+gYou sell &+Y%d&+g crates of &n%s &+gfor &n%s&+g, making a &n%d&+g%% profit.&n\n", crates, cargo_type_name(type), coin_stringv(cost), profit);

 /* Hell yeah - Drannak
  * .. So fucking random. - Lohrr
  if( affected_by_spell(ch, SPELL_STONE_SKIN) )
  {
    send_to_char("You are stoned!\r\n", ch);
  }
  */
  // economy affect
  adjust_ship_market(SOLD_CARGO, rroom, type, crates);

  // Movement towards the Trader boon.
  if( !(paf = get_spell_from_char(ch, AIP_CARGOCOUNT)) )
  {
    paf = apply_achievement(ch, AIP_CARGOCOUNT);
    paf->modifier = 0;
  }
  if( paf->modifier < 10000 && paf->modifier + crates >= 10000 )
  {
    send_to_char( "&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RTrader&+r achievement!&n\n", ch );
  }
  paf->modifier += crates;

  return cost;
}

int sell_cargo(P_char ch, P_ship ship, int slot)
{
  int rroom, i;
  int total_cost = 0;
  bool none_to_sell = TRUE;

  if( ship->location != ch->in_room )
  {
    send_to_char("&+gYou ship is not here to unload cargo!&n\n", ch);
    return TRUE;
  }

  for( rroom = 0; rroom < NUM_PORTS; ++rroom )
  {
    if( ports[rroom].loc_room == world[ch->in_room].number )
    {
      break;
    }
  }
  if( rroom >= NUM_PORTS )
  {
    send_to_char("&+gWe don't buy cargo here!&n\n", ch);
    return 0;
  }

  if( slot == -1 )
  {
    for( i = 0; i < MAXSLOTS; i++ )
    {
      if( ship->slot[i].type == SLOT_CARGO )
      {
        none_to_sell = FALSE;
        break;
      }
    }

    if( none_to_sell )
    {
      send_to_char("&+gYou don't have anything we're interested in.&n\n", ch);
    }
    else
    {
      // i is set to the first cargo slot.
      for( ; i < MAXSLOTS; i++ )
      {
        if( ship->slot[i].type == SLOT_CARGO )
        {
          total_cost += sell_cargo_slot(ch, ship, i, rroom);
        }
      }
    }
  }
  else
  {
    total_cost = sell_cargo_slot(ch, ship, slot, rroom);
  }

    if (total_cost > 0)
    {
        total_cost = check_nexus_bonus(ch, total_cost, NEXUS_BONUS_CARGO);
        // Actual price change handled in sell_cargo_slot().
        if( has_eq_diplomat(ship) )
        {
          send_to_char("&+gYour &+bdip&+Blo&+bmat &+gstatus costs you a &+W10 &+gpercent cut on your profits.&n\n", ch);
        }
        send_to_char_f(ch, "&+gYou receive &n%s&+g.&n\n", coin_stringv(total_cost));
        send_to_char("&+gThanks for your business!&n\n", ch);
        ADD_MONEY(ch, total_cost);

        ship->crew.sail_skill_raise(((float)total_cost / 1000000.0) * 1.5);
        ship->crew.rpar_skill_raise(((float)total_cost / 1000000.0) / 2.0);

        update_crew(ship);
        update_ship_status(ship);
        write_ship(ship);

        write_cargo();
    }
    return TRUE;
}

int sell_contra_slot(P_char ch, P_ship ship, int slot, int rroom)
{
    int type = ship->slot[slot].index;
    if( type >= NUM_PORTS )
        return 0;

    /*if (type == rroom)
    {
        send_to_char_f(ch, "&+gWe're not interested in your &n%s&+g.&n\n", contra_type_name(type));
        return 0;
    }
    else
    {*/
      int crates = ship->slot[slot].val0;
      int cost = crates * contra_buy_price(rroom, type);
      int profit = 100;
      if (ship->slot[slot].val1 != 0)
          profit = (int)(( ((float)cost / (float)ship->slot[slot].val1) - 1.00 ) * 100.0);
      ship->slot[slot].clear();

      if (IS_WARSHIP(ship))
      {
        send_to_char("&+gBecause your contraband is obviosly stolen, local merchants bargain the price down.&n\n", ch);
        cost = cost * 0.6;
      }

      sprintf(buf, "CONTRABAND: %s sold &+W%d&n %s&n at %s&n [%d] for %s&n (%d percent profit)", GET_NAME(ch), crates, contra_type_name(type), ports[rroom].loc_name, ports[rroom].loc_room, coin_stringv(cost), profit);
      statuslog(56, buf);
      logit(LOG_SHIP, strip_ansi(buf).c_str());

      send_to_char_f(ch, "&+gYou sell &+Y%d &+gcrates of &n%s &+gfor &n%s&+g, making a &n%d&+g%% profit.&n\n",
        crates, contra_type_name(type), coin_stringv(cost), profit);

      // economy affect
      adjust_ship_market(SOLD_CONTRA, rroom, type, crates);

      return cost;
    //}
}

int sell_contra(P_char ch, P_ship ship, int slot)
{
    if (ship->location != ch->in_room) 
    {
        send_to_char( "&+gYou ship is not here to unload contraband!&n\n", ch);
        return TRUE;
    }

    int rroom = 0;
    for (rroom = 0; rroom < NUM_PORTS; ++rroom) 
        if (ports[rroom].loc_room == world[ch->in_room].number)
            break;
    if (rroom >= NUM_PORTS) 
    {
        send_to_char("&+gWe don't buy any contraband here!&n\n", ch);
        return 0;
    }

    int total_cost = 0;
    if (slot == -1)
    {
        int none_to_sell = true;
        for (int i = 0; i < MAXSLOTS; i++)
            if (ship->slot[i].type == SLOT_CONTRABAND)
            { none_to_sell = false; break; }

        if (none_to_sell)
        {
            send_to_char("&+gYou don't have anything we're interested in.&n\n", ch);
        }
        else
        {
            for (int i = 0; i < MAXSLOTS; i++)
            {
                if (ship->slot[i].type == SLOT_CONTRABAND)
                {
                    total_cost += sell_contra_slot(ch, ship, i, rroom);
                }
            }
        }
    }
    else
    {
        total_cost = sell_contra_slot(ch, ship, slot, rroom);
    }

    if (total_cost > 0)
    {
        total_cost = check_nexus_bonus(ch, total_cost, NEXUS_BONUS_CARGO);
        send_to_char_f(ch, "&+gYou receive &n%s&+g.&n\n", coin_stringv(total_cost));
        send_to_char("&+gThanks for your business!&n\n", ch);
        ADD_MONEY(ch, total_cost);

        ship->crew.sail_skill_raise(((float)total_cost / 1000000.0) * 1.0);
        ship->crew.rpar_skill_raise(((float)total_cost / 1000000.0) / 3.0);

        update_crew(ship);
        update_ship_status(ship);
        write_ship(ship);

        write_cargo();
    }
    return TRUE;
}

int sell_slot( P_char ch, P_ship ship, int slot )
{
  int value;

  if( ship->location != ch->in_room )
  {
//    send_to_char("&+gYour ship is not here!\n&+gYou can only sell your entire ship, not specific parts!\r\n", ch);
    send_to_char( "&+gBut your ship is not here!&n", ch );
    return TRUE;
  }
  if( slot < 0 || slot >= MAXSLOTS )
  {
    send_to_char_f( ch, "Invalid slot number (%d is not between 1 and %d).\n"
      "&+YSyntax: '&+gsell <&+Gca&+grgo|&+Gco&+gntraband|slot number>&+Y'.&n\n", slot, MAXSLOTS - 1 );
    return TRUE;
  }

  if( ship->slot[slot].type == SLOT_WEAPON )
  {
    if( SHIP_WEAPON_DAMAGED(ship, slot) )
      value = weapon_data[ship->slot[slot].index].cost / 10;
    else
      value = (weapon_data[ship->slot[slot].index].cost * 9) / 10;

    ADD_MONEY(ch, value);
    send_to_char_f( ch, "&+gHere's &n%s &+gfor that &+W%s&+g.&n\n", coin_stringv(value),
      ship->slot[slot].get_description() );
    ship->slot[slot].clear();
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
  }
  else if( is_diplomat_slot(ship, slot) )
  {
    for( int i = 0; i < MAXSLOTS; i++ )
    {
      // Must actually have some crates of the stuff.
      if( ship->slot[i].val0 < 1 )
        continue;
      if( ship->slot[i].type == SLOT_CARGO )
      {
        send_to_char( "&+gYou cannot sell your diplomat flag with cargo onboard. Sell the cargo first!&n\n", ch);
        return TRUE;
      }
      else if( ship->slot[i].type == SLOT_CONTRABAND )
      {
        send_to_char("&+gYou cannot sell your diplomat flag with contraband onboard. Sell the contraband first!&n\n", ch);
        return TRUE;
      }
    }

    value = (equipment_data[ship->slot[slot].index].cost * 9) / 10;
    send_to_char_f( ch, "&+gYou get &n%s &+gfor the &n%s&+g.&n\n", (value == 0) ? "nothing" : coin_stringv(value),
      ship->slot[slot].get_description() );
    ship->slot[slot].clear();
    ADD_MONEY(ch, value);
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
  }
  else if( ship->slot[slot].type == SLOT_EQUIPMENT )
  {
    if( ship->slot[slot].index == E_RAM )
      value = (eq_ram_cost(ship) * 9) / 10;
    else
      value = (equipment_data[ship->slot[slot].index].cost * 9) / 10;

    send_to_char_f( ch, "&+gYou get &n%s &+gfor the &n%s&+g.&n\n", (value == 0) ? "nothing" : coin_stringv(value),
      ship->slot[slot].get_description() );
    ship->slot[slot].clear();
    ADD_MONEY(ch, value);
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
  }
  else if( ship->slot[slot].type == SLOT_CARGO )
  {
    return sell_cargo(ch, ship, slot);
  }
  else if( ship->slot[slot].type == SLOT_CONTRABAND )
  {
    return sell_contra(ch, ship, slot);
  }

  send_to_char ("&+gArrr.. That slot's empty cap'n!&n\n", ch);
  return TRUE;
}


void check_contraband(P_ship ship, int to_room)
{
    int rroom = 0;    
    for (rroom = 0; rroom < NUM_PORTS; ++rroom) 
        if (ports[rroom].loc_room == world[to_room].number)
            break;
    if (rroom >= NUM_PORTS) 
        return;

    if (SHIP_CONTRA(ship) + SHIP_CARGO(ship) == 0)
        return;

    act_to_all_in_ship(ship, "&+BThe port authorities board the ship in search of &+Lcontraband&+B...&n");

    float total_load = (float)SHIP_CARGO_LOAD(ship) / (float)SHIP_MAX_CARGO_SALVAGE(ship);
    bool did_confiscate = false;
    for (int slot = 0; slot < MAXSLOTS; slot++)
    {
        if (ship->slot[slot].type == SLOT_CONTRABAND)
        {
            int type = ship->slot[slot].index;
            int crates = ship->slot[slot].val0;
            if( type < 0 || type >= NUM_PORTS ) return;
            if (type == rroom) continue; // port does not confiscate its own contraband

            float conf_chance = 100.0;

            /*if (IS_TRUSTED(ch))
            {
                conf_chance = 0;
            }
            else*/
            {
                conf_chance = get_property("ship.contraband.baseConfiscationChance", 0.0) + (float)crates / 2; // the more contraband you have, the bigger confiscation chance
                conf_chance -= sqrt(ship->frags) / 5.0;
                conf_chance += (100.0 - conf_chance) * (1.0 - total_load); // the more total cargo onboard, the less confiscation chance
                if (conf_chance > 100.0) conf_chance = 100.0;
                if (conf_chance < 0) conf_chance = 5.0; // always a small chance of being confiscated
            }
          
            debug("SHIP: (%s) confiscation chance (%f).", SHIP_OWNER(ship), conf_chance);

            int confiscated = 0;
            for (int i = 0; i < crates; i++)
                if (number(0, 99) < (int)conf_chance)
                    confiscated++;
          
            if (confiscated > 0)
            {
                sprintf(buf, "CONTRABAND: %s had &+W%d&n/&+W%d&n %s&n confiscated in %s [%d]", SHIP_OWNER(ship), confiscated, crates, contra_type_name(type), ports[rroom].loc_name, ports[rroom].loc_room);
                statuslog(56, buf);
                logit(LOG_SHIP, strip_ansi(buf).c_str());

                act_to_all_in_ship_f(ship, "&+L...&+Band find &+Y%d &+Bcrates of &n%s&+B, which they confiscate.&n",
                  confiscated, contra_type_name(type));
                ship->slot[slot].val0 -= confiscated;
                if (ship->slot[slot].val0 <= 0)
                    ship->slot[slot].clear();
                did_confiscate = true;
            }
        }
    }
    if (!did_confiscate)
        act_to_all_in_ship(ship, "&+L...&+Bbut don't find anything suspicious.&n");
}

int sell_ship(P_char ch, P_ship ship, const char* arg)
{
    int i = 0, k = 0, j;

    send_to_char ("&+RSelling ships completely is not allowed.&N\r\n", ch);
    return TRUE;

    if( !arg || !(*arg) || !isname(arg, "confirm") )
    {
      send_to_char("&+RIf you are sure you want to sell your ship, type '&+gsell &+Gs&+ghip confirm&+R'.&n\n"
        "&+RYou &=LRWILL&N&+R loose your frags and crews!&n\n", ch);
      return TRUE;
    }


    for (j = 0; j < 4; j++)
    {
        i += (ship->armor[j] + ship->internal[j]);
        k += (ship->maxarmor[j] + ship->maxinternal[j]);
    }
    int cost = (int) ((int) (SHIPTYPE_COST(ship->m_class) * .90) * (float) ((float) i / (float) k));
    for (j = 0; j < MAXSLOTS; j++)
    {
        if (ship->slot[j].type == SLOT_WEAPON)
        {
            if (SHIP_WEAPON_DAMAGED(ship, j))
                cost += (int) (weapon_data[ship->slot[j].index].cost * .1);
            else
                cost += (int) (weapon_data[ship->slot[j].index].cost * .9);
        }
    }
    if (ship->location != ch->in_room) 
    {
        cost /= 2;
        if (cost <= 0)
            cost = 1;
        send_to_char ("&+gSince your ship is not here, you only get half the price, salvage costs and all!&n\n", ch);
    }
    ADD_MONEY(ch, cost);
    send_to_char_f(ch, "&+gHere's &n%s &+gfor your ship: &n%s&+g.&n\n", coin_stringv(cost), ship->name);

    shipObjHash.erase(ship);
    delete_ship(ship);
    return TRUE;
}

int repair_all(P_char ch, P_ship ship)
{
    int cost = 0, buildtime = 0;

    int sail_damage = SHIP_MAX_SAIL(ship) - ship->mainsail;
    if (sail_damage > 0)
    {
        cost += sail_damage * 2000;
        buildtime += (75 + sail_damage);
    }


    for (int j = 0; j < 4; j++) 
    {
        int side_damage = (ship->maxarmor[j] - ship->armor[j]) + (ship->maxinternal[j] - ship->internal[j]);
        if (side_damage > 0)
        {
            cost += side_damage * 1000;
            buildtime += (75 + side_damage);
        }
    }

    for (int slot = 0; slot < MAXSLOTS; slot++) 
    {
        if (ship->slot[slot].type == SLOT_WEAPON && SHIP_WEAPON_DAMAGED(ship, slot))
        {
            if (SHIP_WEAPON_DESTROYED(ship, slot))
            {
                cost += weapon_data[ship->slot[slot].index].cost / 2;
                buildtime += 150;
            }
            else
            {
                cost += ship->slot[slot].val2 * 1000;
                buildtime += 75;
            }
        }
    }

    if (cost == 0)
    {
        send_to_char("&+gYour ship is unscathed!&n\n", ch);
        return TRUE;
    }

    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "&+gIt will cost costs &n%s &+gto repair your ship!&n\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);

    ship->mainsail = SHIP_MAX_SAIL(ship);
    for (int j = 0; j < 4; j++) 
    {
        ship->armor[j] = ship->maxarmor[j];
        ship->internal[j] = ship->maxinternal[j];
    }
    for (int slot = 0; slot < MAXSLOTS; slot++) 
    {
        if (ship->slot[slot].type == SLOT_WEAPON && SHIP_WEAPON_DAMAGED(ship, slot))
            ship->slot[slot].val2 = 0;
    }

    send_to_char_f(ch, "&+gThank you for for your business, it will take &n%d &+ghours to complete this repair.&n\n",
      buildtime / 75);
    if (!IS_TRUSTED(ch) && BUILDTIME)
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}


int repair_sail(P_char ch, P_ship ship)
{
    int total_damage = SHIP_MAX_SAIL(ship) - ship->mainsail;
    if (total_damage <= 0) 
    {
        send_to_char("&+gYour &+Wsails &+gare fine; they don't need to be repaired!&n\n", ch);
        return TRUE;
    }

    int cost = total_damage * 2000;

    if (GET_MONEY(ch) < cost)
    {
        send_to_char_f(ch, "&+gThis &+Wsail &+gcosts &n%s &+gto repair!  Come back with enough gravy.&n\n",
          coin_stringv(cost));
        return TRUE;
    }

    if( cost > 0 )
    {
        send_to_char_f(ch, "&+gYou pay &n%s for the repairs&+g.&n\n", coin_stringv(cost));
    }

    SUB_MONEY(ch, cost, 0);

    ship->mainsail = SHIP_MAX_SAIL(ship);

    int buildtime = 75 + total_damage;
    send_to_char_f(ch, "&+gThanks for your business, it will take &n%d &+ghours to complete this repair.&n\n",
      buildtime / 75);
    if (!IS_TRUSTED(ch) && BUILDTIME) 
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}


int repair_armor(P_char ch, P_ship ship, char* arg)
{
    int cost, buildtime, total_damage, j;
    if (!*arg) 
    {
        send_to_char("&+gWhich armor do you want to repair?&n\n", ch);
        return TRUE;
    }
    if (isname(arg, "all")) 
    {
        total_damage = 0;
        for (j = 0; j < 4; j++) 
            total_damage += ship->maxarmor[j] - ship->armor[j];

        if (total_damage == 0)
        {
            send_to_char_f(ch, "&+gYour ship's armor is unscathed!&n\n");
            return TRUE;
        }

        cost = total_damage * 1000;
        if (GET_MONEY(ch) < cost) 
        {
            send_to_char_f(ch, "&+gThis will cost &n%s &+gto repair!&n\n", coin_stringv(cost));
            return TRUE;
        }
        /* OKay, they have the plat, deduct it and do the repairs */
        SUB_MONEY(ch, cost, 0);
        for (j = 0; j < 4; j++)
            ship->armor[j] = ship->maxarmor[j];

        buildtime = 75 + total_damage;
        send_to_char_f(ch, "&+gThanks for your business, it will take &n%d &+ghours to complete this repair.&n\n",
          buildtime / 75);
        if (!IS_TRUSTED(ch) && BUILDTIME)
            ship->timer[T_MAINTENANCE] += buildtime;
        return TRUE;
    }
    if (isname(arg, "fore") || isname(arg, "f")) {
        j = SIDE_FORE;
    } else if (isname(arg, "rear") || isname(arg, "r")) {
        j = SIDE_REAR;
    } else if (isname(arg, "port") || isname(arg, "p")) {
        j = SIDE_PORT;
    } else if (isname(arg, "starboard") || isname(arg, "s")) {
        j = SIDE_STAR;
    } else {
        send_to_char( "Invalid location.\n"
          "&+YSyntax: '&+grepair &+Ga&+grmor <&+Gf&+gore|&+Gr&+gear|&+Gs&+gtarboard|&+Gp&+gort>&+Y'.&n\n", ch );
        return TRUE;
    }
    total_damage = ship->maxarmor[j] - ship->armor[j];
    if (total_damage == 0)
    {
        send_to_char_f(ch, "&+gThis side's armor is unscathed!&n\n");
        return TRUE;
    }
    cost = total_damage * 1000;
    buildtime = 75 + total_damage;
    if (GET_MONEY(ch) < cost)
    {
        send_to_char_f(ch, "&+gThis will cost &n%s &+gto repair!&n\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->armor[j] = ship->maxarmor[j];
    send_to_char_f(ch, "&+gThanks for your business, it will take &n%d &+ghours to complete this repair.&n\n",
      buildtime / 75);
    if (!IS_TRUSTED(ch) && BUILDTIME)
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int repair_internal(P_char ch, P_ship ship, char* arg)
{
    int cost, buildtime, total_damage, j;

    if (!*arg) 
    {
        send_to_char("&+gWhich internal do you want to repair?&n\n", ch);
        return TRUE;
    }
    if (isname(arg, "all")) 
    {
        total_damage = 0;
        for (j = 0; j < 4; j++)
            total_damage += ship->maxinternal[j] - ship->internal[j];

        if (total_damage == 0)
        {
            send_to_char_f(ch, "&+gYour ship's internal structured are fine!&n\n");
            return TRUE;
        }

        cost = total_damage * 1000;
        if (GET_MONEY(ch) < cost) 
        {
            send_to_char_f(ch, "&+gThis will cost &n%s &+gto repair!&n\n", coin_stringv(cost));
            return TRUE;
        }
    /* OKay, they have the plat, deduct it and do the repairs */
        SUB_MONEY(ch, cost, 0);
        for (j = 0; j < 4; j++)
            ship->internal[j] = ship->maxinternal[j];

        buildtime = 75 + total_damage;
        send_to_char_f(ch, "&+gThanks for your business, it will take &n%d &+ghours to complete this repair.&n\n",
          buildtime / 75);
        if (!IS_TRUSTED(ch) && BUILDTIME)
            ship->timer[T_MAINTENANCE] += buildtime;
        update_ship_status(ship);
        write_ship(ship);
        return TRUE;
    }
    if (isname(arg, "fore") || isname(arg, "f")) {
        j = SIDE_FORE;
    } else if (isname(arg, "rear") || isname(arg, "r")) {
        j = SIDE_REAR;
    } else if (isname(arg, "port") || isname(arg, "p")) {
        j = SIDE_PORT;
    } else if (isname(arg, "starboard") || isname(arg, "s")) {
        j = SIDE_STAR;
    } else {
        send_to_char( "Invalid location.\n"
          "&+YSyntax: '&+grepair &+Gi&+gnternal <&+Gf&+gore|&+Gr&+gear|&+Gs&+gtarboard|&+Gp&+gort>&+Y'.&n\n", ch );
        return TRUE;
    }
    total_damage = ship->maxinternal[j] - ship->internal[j];
    if (total_damage == 0)
    {
        send_to_char_f(ch, "&+gThis side's internal structures are fine!&n\n");
        return TRUE;
    }
    cost = total_damage * 1000;
    buildtime = 75 + total_damage;
    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "&+gThis will cost &n%s &+gto repair!&n\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->internal[j] = ship->maxinternal[j];
    send_to_char_f(ch, "&+gThanks for your business, it will take &n%d &+ghours to complete this repair.&n\n",
      buildtime / 75);
    if (!IS_TRUSTED(ch) && BUILDTIME)
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int repair_weapon(P_char ch, P_ship ship, char* arg)
{
    int cost, buildtime;

    if( !is_number(arg) )
    {
        send_to_char( "&+YSyntax: '&+grepair &+Gw&+geapon <weapon slot number>&+Y'.&n\n", ch);
        return TRUE;
    }
    int slot = atoi(arg);
    if( slot > MAXSLOTS || slot < 0 )
    {
        send_to_char ("Invalid weapon slot number.\n"
          "&+YSyntax: '&+grepair &+Gw&+geapon <weapon slot number>&+Y'.&n\n", ch);
        return TRUE;
    }
    if (ship->slot[slot].type != SLOT_WEAPON)
    {
        send_to_char_f( ch, "Slot number %d is not a weapon.\n"
          "&+YSyntax: '&+grepair &+Gw&+geapon <weapon slot number>&+Y'.&n\n", slot);
        return TRUE;
    }
    if( !SHIP_WEAPON_DAMAGED(ship, slot) )
    {
        send_to_char("&+gBut cap'n, this weapon jus' isn't broken!&n\n", ch);
        return TRUE;
    }
    if (SHIP_WEAPON_DESTROYED(ship, slot))
    {
        cost = weapon_data[ship->slot[slot].index].cost / 2;
        buildtime = 150;
    }
    else
    {
        cost = ship->slot[slot].val2 * 1000;
        buildtime = 75;
    }

    if (GET_MONEY(ch) < cost)
    {
        send_to_char_f(ch, "&+gThis will cost &n%s &+gto repair!&n\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->slot[slot].val2 = 0;
    send_to_char_f(ch, "&+gThanks for your business, it will take &n%d &+ghours to complete this repair.&n\n",
      buildtime / 75);
    if (!IS_TRUSTED(ch) && BUILDTIME)
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int reload_ammo(P_char ch, P_ship ship, char* arg)
{
  char weapons_to_reload[MAXSLOTS];
  int cost, buildtime = 0;

  if( is_number(arg) )
  {
    int slot = atoi(arg);

    if( slot >= MAXSLOTS || slot < 0 )
    {
      send_to_char( "Invalid slot number.\n"
        "&+YSyntax: '&+greload <&+Gall&+g|weapon slot>&+Y'.&n\n", ch );
      return TRUE;
    }
    if( ship->slot[slot].type != SLOT_WEAPON )
    {
      send_to_char_f( ch, "Slot %d does not house a weapon.\n"
        "&+YSyntax: '&+greload <&+Gall&+g|weapon slot>&+Y'.&n\n", slot );
      return TRUE;
    }
    if( ship->slot[slot].val1 >= weapon_data[ship->slot[slot].index].ammo )
    {
      send_to_char("&+gThat weapon is already fully loaded!&n\n", ch);
      return TRUE;
    }

    for( int k = 0; k < MAXSLOTS; k++ )
      weapons_to_reload[k] = 0;
    weapons_to_reload[slot] = 1;
    cost = (weapon_data[ship->slot[slot].index].ammo - ship->slot[slot].val1) * 1000;
    buildtime = 75;
  }
  else if( isname(arg, "all") )
  {
    cost = 0;
    for( int slot = 0; slot < MAXSLOTS; slot++ )
    {
      if( (ship->slot[slot].type == SLOT_WEAPON)
        && (ship->slot[slot].val1 < weapon_data[ship->slot[slot].index].ammo) )
      {
        weapons_to_reload[slot] = 1;
        cost += (weapon_data[ship->slot[slot].index].ammo - ship->slot[slot].val1) * 1000;
        buildtime += 75;
      }
      else
      {
        weapons_to_reload[slot] = 0;
      }
    }
  }

  if( cost == 0 || buildtime == 0 )
  {
    send_to_char("&+gNothing to reload here!&n\n", ch);
    return TRUE;
  }

  if( GET_MONEY(ch) < cost )
  {
    send_to_char_f(ch, "&+gYou need a total of &n%s&+g to complete this reload!&n\n", coin_stringv(cost) );
    return TRUE;
  }

  SUB_MONEY(ch, cost, 0);
  for( int slot = 0; slot < MAXSLOTS; slot++ )
  {
    if( weapons_to_reload[slot] == 1 )
      ship->slot[slot].val1 = weapon_data[ship->slot[slot].index].ammo;
  }

  send_to_char_f(ch, "&+gThanks for your business, it will take &n%d&+g hours to complete this reload.&n\n",
    buildtime / 75);
  if( !IS_TRUSTED(ch) && BUILDTIME )
    ship->timer[T_MAINTENANCE] += buildtime;
  update_ship_status(ship);
  write_ship(ship);
  return TRUE;
}

int rename_ship(P_char ch, P_ship ship, char* new_name)
{
  if( !new_name || !*new_name )
  {
    send_to_char( "&+YSyntax: '&+gbuy &+Gr&+gename <new name>&+Y'.&n\n", ch);
    return TRUE;
  }

  if( !check_ship_name(ship, ch, new_name) )
    return TRUE;


  int renamePrice = (int) (SHIPTYPE_COST(ship->m_class) / 10);

  if( GET_MONEY(ch) < renamePrice )
  {
    send_to_char_f(ch, "&+gYou need a total of &n%s&+g to rename your ship.&n\n", coin_stringv(renamePrice));
    return TRUE;
  }

  // If we fail at renaming it for whatever reason.
  if( !rename_ship(ch, GET_NAME(ch), new_name) )
  {
    return TRUE;
  }
  // Otherwise take their money!
  SUB_MONEY(ch, renamePrice, 0);

  send_to_char_f(ch, "&+WCongratulations! From now on your ship will be known as '&n%s&+W'.&n\n", arg2);

  wizlog(AVATAR, "%s has renamed %s ship to '%s'.\n", GET_NAME(ch), HSHR(ch), arg2);
  logit(LOG_PLAYER, "%s has renamed %s ship to '%s'.\n", GET_NAME(ch), HSHR(ch), arg2);

  return TRUE;
}

int buy_cargo(P_char ch, P_ship ship, char* arg)
{
    int rroom = 0, asked_for, slot;

    while( rroom < NUM_PORTS )
    {
        if( ports[rroom].loc_room == world[ch->in_room].number )
        {
          break;
        }
        rroom++;
    }
    if( rroom >= NUM_PORTS )
    {
        send_to_char("&+gWhat cargo? We don't sell any cargo here!&n\n", ch);
        return TRUE;
    }
    if( SHIP_MAX_CARGO(ship) == 0 ) // && !(IS_WARSHIP(ship) && isname( GET_NAME(ch), "Soldon")) )
    {
        send_to_char("&+gYou ship has no space for cargo.&n\n", ch);
        return TRUE;
    }
    if( !is_number(arg) )
    {
      send_to_char( "&+YSyntax: '&+gbuy &+Gca&+grgo <number of crates>&+Y'.&n\n", ch);
      return TRUE;
    }
    if( (asked_for = atoi(arg)) < 1 )
    {
        send_to_char("&+gPlease enter a valid number of crates to buy.&n\n", ch);
        return TRUE;
    }

    if( SHIP_AVAIL_CARGO_LOAD(ship) <= 0 && !(IS_WARSHIP(ship) && isname( GET_NAME(ch), "Soldon")) )
    {
        send_to_char("&+gYou don't have any space left on your ship!&n\n", ch);
        return TRUE;
    }
    if( asked_for > SHIP_AVAIL_CARGO_LOAD(ship) && !(IS_WARSHIP(ship) && isname( GET_NAME(ch), "Soldon")) )
    {
        send_to_char_f(ch, "&+gYou only have space for &+Y%d&+g more crates!&n\n", SHIP_AVAIL_CARGO_LOAD(ship));
        return TRUE;
    }

    /* Soldon's old bonus for winning frag ship competition.
    int slot;
    if( IS_WARSHIP(ship) && isname( GET_NAME(ch), "Soldon") )
    {
      struct ShipSlot *lastSlot = &(ship->slot[MAXSLOTS-1]);
      int max_avail = 10 - ((lastSlot->type == SLOT_EMPTY) ? 0 :
        (lastSlot->type == SLOT_CARGO && lastSlot->index == rroom) ? lastSlot->val0 : 10);
      // Allowing Soldon to have last slot as cargo/contra.
      slot = MAXSLOTS-1;
      if( ship->slot[slot].type == SLOT_EMPTY || max_avail >= asked_for )
      {
        send_to_char( "&+YYou open up your secret cargo hold.&n\n\r", ch );
      }
      else
      {
        // Set slot to MAX_SLOTS and be done with it.
        slot++;
      }
    }
    else
    {
    */
    for( slot = 0; slot < MAXSLOTS; ++slot )
    {
      if (ship->slot[slot].type == SLOT_CARGO && ship->slot[slot].index == rroom)
      {
        break;
      }
    }
    if( slot == MAXSLOTS )
    {
      for (slot = 0 ; slot < MAXSLOTS; ++slot)
      {
        if (ship->slot[slot].type == SLOT_EMPTY)
        {
          break;
        }
      }
    }
    if( slot == MAXSLOTS )
    {
      send_to_char("&+gYou do not have a free slot to fit this cargo into!&n\n", ch);
      return TRUE;
    }

    int unit_cost = cargo_sell_price(rroom);
    int cost = asked_for * unit_cost;

    // hook for epic bonus
    cost -= (int) (cost * get_epic_bonus(ch, EPIC_BONUS_CARGO));

    //if (GET_LEVEL(ch) < 50)
    //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);

    if( GET_MONEY(ch) < cost )
    {
      send_to_char_f(ch, "&+gThis cargo load costs &n%s&+g.  Maybe you should do some babysitting first.&n\n",
        coin_stringv(cost));
      return TRUE;
    }

    int placed = asked_for;
    if( ship->slot[slot].type == SLOT_CARGO )
    {
        ship->slot[slot].val0 += placed;
        ship->slot[slot].val1 += cost;
    }
    else
    {
        ship->slot[slot].type = SLOT_CARGO;
        ship->slot[slot].index = rroom;
        ship->slot[slot].position = SLOT_HOLD;
        ship->slot[slot].val0 = placed;
        ship->slot[slot].val1 = cost;
    }

    sprintf(buf, "CARGO: %s bought &+W%d&n %s&n at %s&n [%d] for %s", GET_NAME(ch), placed, cargo_type_name(rroom), ports[rroom].loc_name, ports[rroom].loc_room, coin_stringv(cost));
    statuslog(56, buf);
    logit(LOG_SHIP, strip_ansi(buf).c_str());

    send_to_char_f(ch, "&+gYou buy &+Y%d &+gcrates of &n%s &+gfor &n%s&+g.&n\n",
      placed, cargo_type_name(rroom), coin_stringv(cost) );

    SUB_MONEY(ch, cost, 0);

    // economy affect
    adjust_ship_market(BOUGHT_CARGO, rroom, rroom, placed);

    send_to_char ("&+gThank you for your purchase, your cargo is loaded and set to go!&n\n", ch);

    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int buy_contra(P_char ch, P_ship ship, char* arg)
{
  int rroom = 0, slot;

  while( rroom < NUM_PORTS )
  {
    if (ports[rroom].loc_room == world[ch->in_room].number)
    {
      break;
    }
    rroom++;
  }
  if( rroom >= NUM_PORTS )
  {
    send_to_char("&+gWhat contraband?  We don't sell any contraband!&n\n", ch);
    return TRUE;
  }
  if( !IS_TRUSTED(ch) && GET_ALIGNMENT(ch) > MINCONTRAALIGN )
  {
    send_to_char ("&+gGoodie goodie two shoes like you shouldn't think of contraband.&n\n", ch);
    return TRUE;
  }
  if( !IS_TRUSTED(ch) && !can_buy_contraband(ship, rroom) )
  {
    send_to_char ("&+gContraband?! &+w*gasp* &+gHow could you even think that I would sell such things!&n\n", ch);
    return TRUE;
  }
  if( SHIP_MAX_CONTRA(ship) == 0 ) //&& !(IS_WARSHIP(ship) && isname( GET_NAME(ch), "Soldon")) )
  {
    send_to_char("&+gYour ship has no places to hide contraband.&n\n", ch);
    return TRUE;
  }
  if( !is_number(arg) )
  {
    send_to_char( "Invalid number of crates!\n"
      "&+YSyntax: '&+gbuy &+Gco&+gntraband <number of crates>&+Y'.&n\n", ch);
    return TRUE;
  }

  int asked_for = atoi(arg);
  if( asked_for < 1 )
  {
    send_to_char("&+gPlease enter a valid number of crates to buy.&n\n", ch);
    return TRUE;
  }
  if( SHIP_AVAIL_CARGO_LOAD(ship) <= 0 && !(IS_WARSHIP(ship) && isname( GET_NAME(ch), "Soldon")) )
  {
    send_to_char("&+gYou don't have any space left on your ship!&n\n", ch);
    return TRUE;
  }
  // max contraband amount reduced proportionally to total available cargo load
  int contra_max = int ((float)SHIP_MAX_CONTRA(ship) * ((float)SHIP_MAX_CARGO_LOAD(ship) / (float)SHIP_MAX_CARGO(ship)));
  int contra_avail = contra_max - SHIP_CONTRA(ship);
  /* Part of Soldon's reward from winning the ship frag contest.
  if( IS_WARSHIP(ship) && isname( GET_NAME(ch), "Soldon") )
  {
    contra_avail = 10;
  }
  */
  if( contra_avail <= 0 )
  {
    send_to_char("&+gYou don't have any space left on your ship to hide contraband!&n\n", ch);
    return TRUE;
  }
  int max_avail = MIN(SHIP_AVAIL_CARGO_LOAD(ship), contra_avail);
  /* Soldon's reward from winning ship pvp contest
  if( IS_WARSHIP(ship) && isname( GET_NAME(ch), "Soldon") )
  {
    struct ShipSlot *lastSlot = &(ship->slot[MAXSLOTS-1]);

    max_avail = 10 - ((lastSlot->type == SLOT_EMPTY) ? 0 :
      (lastSlot->type == SLOT_CONTRABAND && lastSlot->index == rroom) ? lastSlot->val0 : 10);
  }
  */
  if( asked_for > max_avail )
  {
    send_to_char_f(ch, "&+gYou only have space for &+Y%d&+g more crates of contraband to hide!&n\n", max_avail);
    return TRUE;
  }

  /* More of Soldon's reward.
  slot = 0;
  if( IS_WARSHIP(ship) && isname(GET_NAME( ch ), "Soldon") )
  {
    slot = MAXSLOTS-1;
  }
  else
  {
    for (slot = 0 ; slot < MAXSLOTS; ++slot)
    {
      if( ship->slot[slot].type == SLOT_CONTRABAND && ship->slot[slot].index == rroom )
      {
        break;
      }
    }
  }
  */

  for( slot = 0 ; slot < MAXSLOTS; ++slot )
  {
    if( ship->slot[slot].type == SLOT_EMPTY )
    {
      break;
    }
  }
  if( slot == MAXSLOTS )
  {
    send_to_char("&+gYou do not have a free slot to fit this &+Lcontraband &+ginto!&n\n", ch);
    return TRUE;
  }

  int unit_cost = contra_sell_price(rroom);
  int cost = asked_for * unit_cost;

  //if (GET_LEVEL(ch) < 50)
  //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);

  if( GET_MONEY(ch) < cost )
  {
    send_to_char_f(ch, "&+gThis contraband costs &n%s&+g; better go sell your sister.&n\n", coin_stringv(cost));
    return TRUE;
  }

  int placed = asked_for;
  if (ship->slot[slot].type == SLOT_CONTRABAND)
  {
    ship->slot[slot].val0 += placed;
    ship->slot[slot].val1 += cost;
  }
  else
  {
    ship->slot[slot].type = SLOT_CONTRABAND;
    ship->slot[slot].index = rroom;
    ship->slot[slot].position = SLOT_HOLD;
    ship->slot[slot].val0 = placed;
    ship->slot[slot].val1 = cost;
  }

  sprintf(buf, "CONTRABAND: %s bought &+W%d&n %s&n at %s&n [%d] for %s", GET_NAME(ch), placed, contra_type_name(rroom), ports[rroom].loc_name, ports[rroom].loc_room, coin_stringv(cost));
  statuslog(56, buf);
  logit(LOG_SHIP, strip_ansi(buf).c_str());

  send_to_char_f( ch, "&+gYou buy &+Y%d &+gcrates of &n%s &+gfor &n%s&+g.&n\n",
    placed, contra_type_name(rroom), coin_stringv(cost) );

  SUB_MONEY(ch, cost, 0);

  // economy affect
  adjust_ship_market(BOUGHT_CONTRA, rroom, rroom, placed);

  send_to_char ("&+gThank you for your 'purchase' &+w*snicker*&+g, your 'cargo' is loaded and set to go!&n\n", ch);

  update_ship_status(ship);
  write_ship(ship);
  return TRUE;
}

int buy_weapon(P_char ch, P_ship ship, char* arg1, char* arg2)
{
  struct affected_type *paf = get_spell_from_char(ch, AIP_CARGOCOUNT);
  bool quickbuild = (paf && paf->modifier >= 10000) ? TRUE : FALSE;

    if( !is_number(arg1) || !arg2 || !*arg2 )
    {
        send_to_char(
          "&+YSyntax: '&+gbuy &+Gw&+geapon <number> <&+Gf&+gore|&+Gr&+gear|&+Gs&+gtarboard|&+Gp&+gort>&+Y'.&n\n", ch );
        return TRUE;
    }

    int w = atoi(arg1) - 1;
    if ((w < 0) || (w >= MAXWEAPON)) 
    {
        send_to_char( "Invalid weapon number.\n"
          "&+YSyntax: '&+gbuy &+Gw&+geapon <number> <&+Gf&+gore|&+Gr&+gear|&+Gs&+gtarboard|&+Gp&+gort>&+Y'.&n\n", ch );
        return TRUE;
    }

    int arc;
    if( isname(arg2, "fore") || isname(arg2, "f") )
        arc = SIDE_FORE;
    else if( isname(arg2, "rear") || isname(arg2, "r") )
        arc = SIDE_REAR;
    else if( isname(arg2, "port") || isname(arg2, "p") )
        arc = SIDE_PORT;
    else if( isname(arg2, "starboard") || isname(arg2, "s") )
        arc = SIDE_STAR;
    else
    {
        send_to_char( "Invalid weapon placement.\n"
          "&+YSyntax: '&+gbuy &+Gw&+geapon <number> <&+Gf&+gore|&+Gr&+gear|&+Gs&+gtarboard|&+Gp&+gort>&+Y'.&n\n", ch );
        return TRUE;
    }

    if( weapon_data[w].weight > SHIP_AVAIL_WEIGHT(ship) )
    {
        send_to_char_f( ch, "&+gThat weapon weighs &+w%d&+g! Your ship can only support &+w%d&+g more!&n\n",
          weapon_data[w].weight, SHIP_AVAIL_WEIGHT(ship) );
        return TRUE;
    }
    if( !ship_allowed_weapons[ship->m_class][w] )
    {
        send_to_char_f(ch, "&+gSuch a great weapon can not be installed on such a small hull!&n\n");
        return TRUE;
    }

    int slot = 0;
    while (slot < MAXSLOTS) 
    {
        if (ship->slot[slot].type == SLOT_EMPTY)
            break;
        slot++;
    }
    if (slot >= MAXSLOTS) 
    {
        send_to_char("&+gYou do not have a free slot to intall this weapon!&n\n", ch);
        return TRUE;
    }

    int same_arc = 0;
    int same_arc_weight = 0;
    for (int sl = 0; sl < MAXSLOTS; sl++) 
    {
        if (ship->slot[sl].type == SLOT_WEAPON && ship->slot[sl].position == arc)
        {
            same_arc++;
            same_arc_weight += weapon_data[ship->slot[sl].index].weight;
        }
    }
    if (same_arc >= ship_arc_properties[ship->m_class].max_weapon_slots[arc])
    {
        send_to_char_f(ch, "&+gYour can not put more weapons to this side!&n\n");
        return TRUE;
    }
    if (same_arc_weight + weapon_data[w].weight > ship_arc_properties[ship->m_class].max_weapon_weight[arc])
    {
        send_to_char_f(ch, "&+gYour can not put more weight to this side! Maximum allowed: &+w%d&+g.&n\n",
          ship_arc_properties[ship->m_class].max_weapon_weight[arc]);
        return TRUE;
    }

    if (!IS_SET(weapon_data[w].flags, arcbitmap[arc]))
    {
        send_to_char ("&+gThat weapon cannot be placed in that position, try another one.&n\n", ch);
        return TRUE;
    }

/* -Removing frag requirements for weapons - Drannak 6/3/2013
 *  Readding them. - Lohrr 7/23/2014
 *  Also added a min_crewexp value for each weapon and compared it to guns_skill for exp.
*/
    if( ship->frags < weapon_data[w].min_frags
      && ship->crew.guns_skill < weapon_data[w].min_crewexp )
    {
        send_to_char ("&+gI'm sorry, but not just anyone can buy this weapon!  You must earn it!&n\n", ch);
        return TRUE;
    }

    if( IS_SET(weapon_data[w].flags, CAPITAL) )
    {
      if( ship->buy_check_capital(ch) )
      {
        return TRUE;
      }
    }

    int cost = weapon_data[w].cost;
    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "&+gThis weapon costs &n%s&+g, but you can dream, right?&n\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    set_weapon(ship, slot, w, arc);
    int buildtime = weapon_data[w].weight * 75;
    // Achievement - Trader
    if( quickbuild )
    {
      buildtime /= 2;
    }
  //bool pvp = false;
	//pvp = ocean_pvp_state();
  //if (pvp) buildtime *= 4;
    send_to_char_f( ch, "&+gThank you for your purchase, it will take &n%d &+ghours to install the part.&n\n",
      buildtime / 75 );
    if (!IS_TRUSTED(ch) && BUILDTIME)
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int buy_equipment(P_char ch, P_ship ship, char* arg1)
{
  struct affected_type *paf = get_spell_from_char(ch, AIP_CARGOCOUNT);
  bool quickbuild = (paf && paf->modifier >= 10000) ? TRUE : FALSE;
  int equip_num, slot, j, cost, buildtime;
  bool pvp;

  if( !is_number(arg1) )
  {
    send_to_char( "&+YSyntax: '&+gbuy &+Ge&+gquipment <number>&+Y'.&n\n", ch );
    return TRUE;
  }

  equip_num = atoi(arg1) - 1;
  if( (equip_num < 0) || (equip_num >= MAXEQUIPMENT) )
  {
    send_to_char_f( ch, "Invalid equipment number.\n"
      "&+YSyntax: '&+gbuy &+Ge&+gquipment <number 1-%d>&+Y'.&n\n", MAXEQUIPMENT );
    return TRUE;
  }

  if( equipment_data[equip_num].weight > SHIP_AVAIL_WEIGHT(ship) )
  {
    send_to_char_f(ch, "&+gThat equipment weighs &+w%d&+g! Your ship can only support &+w%d&+g more!&n\n",
      equipment_data[equip_num].weight, SHIP_AVAIL_WEIGHT(ship));
    return TRUE;
  }
  if( !ship_allowed_equipment[ship->m_class][equip_num] )
  {
    send_to_char_f(ch, "&+gSuch marvelous equipment can not be installed on such a pitiful hull!&n\n");
    return TRUE;
  }

  slot = 0;
  while( slot < MAXSLOTS )
  {
    if( ship->slot[slot].type == SLOT_EMPTY )
      break;
    slot++;
  }
  if( slot >= MAXSLOTS )
  {
    send_to_char("&+gYou do not have a free slot to install this equipment!&n\n", ch);
    return TRUE;
  }

  if( ship->frags < equipment_data[equip_num].min_frags )
  {
    send_to_char( "&+gI'm sorry, but not just anyone can buy this equipment!  You must earn it!&n\n", ch);
    return TRUE;
  }

  for( j = 0; j < MAXSLOTS; j++ )
  {
    if( (ship->slot[j].type == SLOT_EQUIPMENT) && (ship->slot[j].index == equip_num) )
    {
      send_to_char_f( ch, "&+gYou already have the &n%s&+g! You can only have one such item.&n\n",
        equipment_data[equip_num].name );
      return TRUE;
    }
  }

  if( IS_SET(equipment_data[equip_num].flags, CAPITAL) )
  {
    if( ship->buy_check_capital(ch) )
    {
      return TRUE;
    }
  }

  if( equip_num == E_RAM )
    cost = eq_ram_cost(ship);
  else
    cost = equipment_data[equip_num].cost;

  if( GET_MONEY(ch) < cost )
  {
    send_to_char_f(ch, "&+gThis equipment costs &n%s&+g, and you ain't got that much on ya!&n\n", coin_stringv(cost));
    return TRUE;
  }

  SUB_MONEY(ch, cost, 0);
  set_equipment(ship, slot, equip_num);

/* Unused variable weight. - Lohrr
    int weight = equipment_data[e].weight;
    if (e == E_RAM) weight = eq_ram_weight(ship);
    if (e == E_LEVISTONE) weight = eq_levistone_weight(ship);
*/
  buildtime = equipment_data[equip_num].weight * 75;
  pvp = ocean_pvp_state();
  if( pvp )
    buildtime *= 4;
  // Achievement - Trader
  if( quickbuild )
  {
    buildtime /= 2;
  }
  send_to_char_f(ch, "Thank you for your purchase, it will take %d hours to install the %s.\r\n",
    (int) (buildtime / 75), equipment_data[equip_num].name );
  if( !IS_TRUSTED(ch) && BUILDTIME )
    ship->timer[T_MAINTENANCE] += buildtime;
  update_ship_status(ship);
  write_ship(ship);
  return TRUE;
}


int buy_hull(P_char ch, P_ship ship, int owned, char* arg1, char* arg2)
{
  int cost, buildtime, hull_type, oldhull;
  struct affected_type *paf = get_spell_from_char(ch, AIP_CARGOCOUNT);
  bool quickbuild = (paf && paf->modifier >= 10000) ? TRUE : FALSE;
  char buf[32];

  hull_type = atoi(arg1) - 1;
  // Note: hull_type MAXSHIPCLASS - 1 is a NPC - Only ship class.
  if( (hull_type < 0) || (hull_type >= MAXSHIPCLASS - 1) )
  {
    send_to_char_f( ch, "Invalid hull size.\n"
      "&+YSyntax: '&+gbuy &+Gh&+gull <number 1-%d>&+Y'.&n\n", MAXSHIPCLASS - 2 );
    return TRUE;
  }

  if( !owned && (!arg2 || strip_ansi(arg2).length() < 1) )
  {
    send_to_char_f( ch, "Invalid name: %s"
      "&+YSyntax: '&+gbuy &+Gh&+gull <number> <name>&+Y'.&n\n", (!arg2) ? "NULL" : arg2 );
    return TRUE;
  }

  if( SHIPTYPE_MIN_LEVEL(hull_type) > GET_LEVEL(ch) )
  {
    send_to_char ("&+gYou are too little for such a big ship! Get more experience!&n\n", ch);
    return TRUE;
  }

  if( SHIPTYPE_COST(hull_type) == 0 && !IS_TRUSTED(ch) )
  {
    send_to_char ("&+gThis hull is not for sale.&n\n", ch);
    return TRUE;
  }

  /* Okay, we have a valid selection for ship hull */
  if( owned )
  {
    oldhull = ship->m_class;
    if( hull_type == oldhull )
    {
      send_to_char ("&+gYou own this hull already!&n\n", ch);
      return TRUE;
    }
    if( SHIP_CONTRA(ship) + SHIP_CARGO(ship) > 0 )
    {
      send_to_char_f( ch, "&+gYou can not reconstruct a ship that has %s!&n\n",
        (SHIP_CARGO( ship )) ? "cargo" : "contraband" );
      return TRUE;
    }
    if( !check_undocking_conditions (ship, hull_type, ch) )
    {
      return TRUE;
    }
   /* There are checks for hull change being valid now.
    for( int k = 0; k < MAXSLOTS; k++ )
    {
      if( ship->slot[k].type != SLOT_EMPTY )
      {
        send_to_char("You can not change a hull with non-empty slots!\r\n", ch);
        return TRUE;
      }
    }
    */

    cost = SHIPTYPE_COST(hull_type) - (int) (SHIPTYPE_COST(oldhull) * .90);
    if( cost >= 0 )
    {
      if( GET_MONEY(ch) < cost || GET_EPIC_POINTS(ch) < SHIPTYPE_EPIC_COST(hull_type) )
      {
        if( SHIPTYPE_EPIC_COST(hull_type) > 0 )
        {
          send_to_char_f( ch, "&+gThat upgrade costs &n%s &+gand &+W%d epics&+g!  Come back when you can afford it.&n\n",
            coin_stringv(cost), SHIPTYPE_EPIC_COST(hull_type) );
        }
        else
        {
          send_to_char_f( ch, "&+gThat upgrade costs &n%s&+g!  Come back when you can afford it.&n\n",
            coin_stringv(cost) );
        }
        return TRUE;
      }
      SUB_MONEY(ch, cost, 0);/* OKay, they have the plat, deduct it and build the ship */
      if(SHIPTYPE_EPIC_COST(hull_type) > 0 )
      {
        ch->only.pc->epics -= SHIPTYPE_EPIC_COST(hull_type);
      }
    }
    else
    {
      if( GET_EPIC_POINTS(ch) < SHIPTYPE_EPIC_COST(hull_type) )
      {
        send_to_char_f(ch, "&+gThat upgrade costs &+W%d epics&+g!&n\n", SHIPTYPE_EPIC_COST(1928));
        return TRUE;
      }
      cost *= -1;
      send_to_char_f(ch, "&+gYou receive &n%s&+g for the remaining materials.&n\n", coin_stringv(cost));
      ADD_MONEY(ch, cost);
      if( SHIPTYPE_EPIC_COST(hull_type) > 0 )
      {
        ch->only.pc->epics -= SHIPTYPE_EPIC_COST(hull_type);
      }
    }

    kick_everyone_off(ship);
    ship->m_class = hull_type;
    reset_ship(ship, false);

    if( ship->m_class > oldhull )
    {
      buildtime = 75 * (ship->m_class / 2 - oldhull / 3);
    }
    else
    {
      buildtime = 75 * (oldhull / 2 - ship->m_class / 3);
    }

    if( ocean_pvp_state() )
    {
      buildtime *= 5;
    }

    // Achievement - Trader
    if( quickbuild )
    {
      buildtime /= 2;
    }
    send_to_char_f( ch, "&+gThanks for your business, it will take &n%d&+g hours to complete this upgrade.&n\n",
      buildtime / 75 );
  }
  else
  {
    if( (int)strlen(strip_ansi(arg2).c_str()) <= 0 )
    {
      send_to_char( "No name selected.\n"
        "&+YSyntax: '&+gbuy &+Gh&+gull <number> <name>&+Y'.&n\n", ch );
      return TRUE;
    }
    if( !is_valid_ansi_with_msg(ch, arg2, FALSE) )
    {
      send_to_char ("&+gInvalid ANSI name!&n\n", ch); 
      sprintf( buf, "spank me" );
      command_interpreter( ch, buf );
      return TRUE;
    }
    if( sub_string_set(strip_ansi(arg2).c_str(), rude_ass) )
    {
      send_to_char("&+gName must not contain rude terms.&n\n", ch);
      sprintf( buf, "whap me" );
      command_interpreter( ch, buf );
      return TRUE;
    }
    if( (int) strlen(strip_ansi(arg2).c_str()) > 20 )
    {
      send_to_char("&+gShip names can be at most 20 characters (not including ansi).&n\n", ch);
      return TRUE;
    }
    if( !check_ship_name(0, ch, arg2) )
    {
      return TRUE;
    }
    if( affected_by_spell( ch, AIP_FREESLOOP ) )
    {
      if( (GET_MONEY(ch) >= SHIPTYPE_COST(hull_type) - 100000) && GET_EPIC_POINTS(ch) >= SHIPTYPE_EPIC_COST(hull_type) )
      {
        send_to_char( "You show your &+ysmall &+bS&+Ba&+bi&+Bl&+bo&+Br&+b'&+Bs&n &+yTattoo&n for a discount.&n\n\r", ch );
        ADD_MONEY(ch, 100000);
        // Remove the flag for free sloop.
        affect_from_char( ch, AIP_FREESLOOP );
      }
    }

    if( GET_MONEY(ch) < SHIPTYPE_COST(hull_type) || GET_EPIC_POINTS(ch) < SHIPTYPE_EPIC_COST(hull_type) )
    {
      if( SHIPTYPE_EPIC_COST(hull_type) > 0 )
      {
        send_to_char_f(ch, "&+gThat hull costs &n%s &+gand &+W%d epics&+g!&n\n",
          coin_stringv(SHIPTYPE_COST(hull_type)), SHIPTYPE_EPIC_COST(hull_type));
      }
      else
      {
        send_to_char_f(ch, "&+gThat ship costs &n%s&+g!\r\n", coin_stringv(SHIPTYPE_COST(hull_type)));
      }
      return TRUE;
    }

    // Now, create the ship object
    ship = new_ship(hull_type);
    if( ship == NULL )
    {
      logit(LOG_FILE, "Error in new_ship(): %d\n", shiperror);
      send_to_char_f(ch, "&=LrError creating new ship (%d), please notify a god.&n\n", shiperror);
      return TRUE;
    }

    buildtime = 75 * SHIPTYPE_ID(hull_type) / 4;
    ship->ownername = str_dup(GET_NAME(ch));
    ship->anchor = world[ch->in_room].number;
    name_ship(arg2, ship);
    // Achievement - Trader
    if( quickbuild )
    {
      buildtime /= 2;
    }
    if( !load_ship(ship, ch->in_room) )
    {
      logit(LOG_FILE, "Error in load_ship(): %d\n", shiperror);
      send_to_char_f(ch, "&=LrError loading ship (%d), please notify a god.&n\n", shiperror);
      return TRUE;
    }
    write_ships_index();

    // everything went successfully, substracting the cost
    SUB_MONEY(ch, SHIPTYPE_COST(hull_type), 0);
    if( SHIPTYPE_EPIC_COST(hull_type) > 0 )
    {
      epic_gain_skillpoints(ch, -SHIPTYPE_EPIC_COST(hull_type));
    }

    send_to_char_f( ch, "&+gYour ship, '&n%s&+g', will be &n%s&+g once painted.&n\n", strip_ansi(arg2).c_str(), arg2 );
    send_to_char_f(ch, "&+gThanks for your business, this hull will take &n%d &+ghours to build.\r\n", buildtime/75);
  }

  if( !IS_TRUSTED(ch) && BUILDTIME )
  {
    ship->timer[T_MAINTENANCE] += buildtime;
  }
  update_ship_status(ship);
  write_ship(ship);
  return TRUE;
}

// Swaps the contents of slot 1 with slot 2.
int swap_slots(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    if( !*arg1 || !is_number(arg1) || !*arg2 || !is_number(arg2) )
    {
        send_to_char( "&+YSyntax: '&+gbuy &+Gs&+gwap <slot number 1> <slot number 2>&+Y'.&n\n", ch );
        return TRUE;
    }
    int slot1 = atoi(arg1);
    int slot2 = atoi(arg2);
    if( (slot1 >= MAXSLOTS) || (slot1 < 0) || (slot2 >= MAXSLOTS) || (slot2 < 0) )
    {
        send_to_char_f( ch, "&+gSlot numbers range from 0 to %d!&n\n", MAXSLOTS - 1 );
        return TRUE;
    }
    if( slot1 == slot2 )
    {
        send_to_char("&+gSwitch slot with itself?! What for?&n\n", ch);
        return TRUE;
    }
    ShipSlot temp;
    temp.clone(ship->slot[slot1]);
    ship->slot[slot1].clone(ship->slot[slot2]);
    ship->slot[slot2].clone(temp);
    send_to_char("&+gDone! Thank you for your business!&n\n", ch);
    write_ship(ship);
    return TRUE;
}

int ship_shop_proc(int room, P_char ch, int cmd, char *arg)
{
  P_ship ship;
  ShipVisitor svs;
  bool owned;

  if( !IS_ALIVE(ch) || IS_NPC(ch) )
    return FALSE;

  if( (cmd != CMD_SUMMON) && (cmd != CMD_LIST) && (cmd != CMD_BUY)
    && (cmd != CMD_RELOAD) && (cmd != CMD_REPAIR) && (cmd != CMD_SELL) )
  {
    return FALSE;
  }

  half_chop(arg, arg1, arg2);

  // Hack to allow ferry ticket automats in room with ship store
  if( cmd == CMD_BUY && isname(arg1, "ticket") )
    return FALSE;

  // Look for ch's ship.
  ship = NULL;
  owned = FALSE;
  // If there are any ships in the game..
  if( shipObjHash.get_first(svs) )
  {
    do
    {
      // Check to see if ch is the owner.
      if( isname(svs->ownername, GET_NAME( ch )) )
      {
        ship = svs;
        owned = TRUE;
        break;
      }
    }
    // If ch not the owner, move to next ship.
    while( shipObjHash.get_next(svs) ) ;
  }

  if( owned )
  {
    if( ship->location != ch->in_room )
    {
      if( (cmd != CMD_SUMMON) && (cmd != CMD_LIST) && !(cmd == CMD_SELL && isname( arg1, "ship" )) )
      {
        send_to_char("&+gYour ship needs to be here to receive service.&n\n", ch);
        send_to_char_f( ch, "&+gFor a small fee of &n%s&+g, I can have my men tell your crew to sail here.&n\n",
          coin_stringv(SHIPTYPE_HULL_WEIGHT(ship->m_class) * 100) );
        send_to_char("&+gJust type '&+Gsummon ship&+g' to have your ship sail here.&n\n", ch);
        return TRUE;
      }
    }

    if( !SHIP_DOCKED(ship) || (ship->timer[T_UNDOCK] != 0) )
    {
      if( (cmd != CMD_SUMMON) && (cmd != CMD_LIST) && !(cmd == CMD_SELL && isname( arg1, "ship" )) )
      {
        send_to_char("&+gYou must dock your ship first!&n\n", ch);
        return TRUE;
      }
    }

    if( (cmd == CMD_SELL || cmd == CMD_SUMMON) && ship->timer[T_MAINTENANCE] > 0 )
    {
      send_to_char_f(ch, "&+gYour ship is currently being worked on, please wait for another &n%.1f&+g hours.&n\n",
        (float) ship->timer[T_MAINTENANCE] / 75.0);
      return TRUE;
    }

    // arg1 must be ship or time per check above.
    if( cmd == CMD_SUMMON )
    {
      if( isname(arg1, "ship") )
        return summon_ship(ch, ship, FALSE);
      if( isname(arg1, "time") )
        return summon_ship(ch, ship, TRUE);
      return FALSE;
    }

    if( cmd == CMD_SELL )
    {
      if( ship->timer[T_BSTATION] != 0 )
      {
        send_to_char("&+gYour ship is currently pre-occupied in &+rcombat&+g!&n\n", ch);
        return TRUE;
      }
      if( *arg1 )
      {
        if( isname(arg1, "ship") )
        {
          return sell_ship(ch, ship, arg2);
        }

        // Must type at least 'sell ca' vs 'sell contraband'.
        if( is_abbrev(arg1, "cargo") && arg1[1] != '\0' )
        {
          return sell_cargo(ch, ship, -1);
        }
        // Must type at least 'sell co' vs 'sell cargo'.
        else if( is_abbrev(arg1, "contraband") && arg1[1] != '\0' )
        {
          return sell_contra(ch, ship, -1);
        }
        else if( is_number(arg1) )
        {
          return sell_slot(ch, ship, atoi(arg1));
        }
      }
      send_to_char( "&+YSyntax: '&+gsell <&+Gca&+grgo|&+Gco&+gntraband|slot number>&+Y'.&n\n", ch );
      return TRUE;
    }

    if( cmd == CMD_REPAIR )
    {
      if( *arg1 )
      {
        if( isname(arg1, "all") )
        {
          return repair_all(ch, ship);
        }
        if( isname(arg1, "sail") || isname(arg1, "s") )
        {
          return repair_sail (ch, ship);
        }
        else if( isname(arg1, "armor") || isname(arg1, "a") )
        {
          return repair_armor(ch, ship, arg2);
        }
        else if( isname(arg1, "internal") || isname(arg1, "i") )
        {
          return repair_internal(ch, ship, arg2);
        }
        else if( isname(arg1, "weapon") || isname(arg1, "w") )
        {
          return repair_weapon(ch, ship, arg2);
        }
      }
      send_to_char( "&+YSyntax: '&+grepair <&+Gall&+g|&+Ga&+grmor|&+Gi&+gnternal|&+Gs&+gail|&+Gw&+geapon>&+Y'.&n\n", ch );
      return TRUE;
    }

    if( cmd == CMD_RELOAD )
    {
      if( *arg1 )
      {
        if( is_number(arg1) || isname(arg1, "all") )
        {
          return reload_ammo(ch, ship, arg1);
        }
      }
      send_to_char( "&+YSyntax: '&+greload <&+Gall&+g|slot number>&+Y'.&n\n", ch );
      return TRUE;
    }
  }
  else if( (cmd == CMD_SUMMON) || (cmd == CMD_RELOAD) || (cmd == CMD_REPAIR) || (cmd == CMD_SELL) )
  {
    send_to_char("&+gBut you do not own a ship!?&n\n", ch);
    return TRUE;
  }

  if( cmd == CMD_LIST )
  {
    if( *arg1 )
    {
      if( is_abbrev(arg1, "cargo") )
      {
        return list_cargo(ch, ship, owned);
      }
      else if( is_abbrev(arg1, "equipment") )
      {
        return list_equipment(ch, ship, owned);
      }
      else if( is_abbrev(arg1, "hulls") )
      {
        return list_hulls(ch, ship, owned);
      }
      else if( is_abbrev(arg1, "weapons") )
      {
        return list_weapons(ch, ship, owned);
      }
    }
    send_to_char("Valid syntax is '&+glist <&+Gh&+gull/&+Gw&+geapon/&+Ge&+gquipment/&+Gc&+gargo>'\r\n", ch);
    return TRUE;
  }

  if( cmd == CMD_BUY )
  {
    if( isname(arg1, "hull") || isname(arg1, "h") )
    {
      if( owned && ship->timer[T_MAINTENANCE] > 0 )
      {
        send_to_char_f(ch, "&+gYour ship is currently being worked on, please wait for another &n%.1f&+g hours.&n\n",
          (float) ship->timer[T_MAINTENANCE] / 75.0);
        return TRUE;
      }

      half_chop(arg2, arg1, arg3);
      return buy_hull(ch, ship, owned, arg1, arg3);
    }
    else if( !owned )
    {
      send_to_char( "&+gYou have no ship.. land lubber.&n\n", ch );
      return TRUE;
    }
    else if( isname(arg1, "rename") || isname(arg1, "r") )
    {
      return rename_ship(ch, ship, arg2);
    }
    else if( isname(arg1, "swap") || isname(arg1, "s") )
    {
      half_chop(arg2, arg1, arg3);
      return swap_slots(ch, ship, arg1, arg3);
    }
    else if( isname(arg1, "weapon") || isname(arg1, "w") )
    {
      half_chop(arg2, arg1, arg3);
      return buy_weapon(ch, ship, arg1, arg3);
    }
    else if( isname(arg1, "equipment") || isname(arg1, "e") )
    {
      return buy_equipment(ch, ship, arg2);
    }
    else if( ship->timer[T_MAINTENANCE] > 0 )
    {
      send_to_char_f(ch, "&+gYour ship is currently being worked on, please wait for another &n%.1f&+g hours.&n\n",
        (float) ship->timer[T_MAINTENANCE] / 75.0);
      return TRUE;
    }
    else if( isname(arg1, "cargo") || isname(arg1, "ca") )
    {
      return buy_cargo(ch, ship, arg2);
    }
    else if( isname(arg1, "contraband") || isname(arg1, "co") )
    {
      return buy_contra(ch, ship, arg2);
    }
    else
    {
      send_to_char ("Valid syntax: '&+gbuy &+Gh&+gull|&+Gca&+grgo|&+Gco&+gntraband|&+Gw&+geapon|&+Ge&+gquipment|&+Gr&+gename|&+Gs&+gwap>'.\r\n", ch);
      return TRUE;
    }
  }

  send_to_char ("ship_shop deadend reached, report a bug!\r\n", ch);
  debug( "&+Rship_shop: Error with string '&n%s&n&+R'.&n", arg);
  return FALSE;
};


const int good_crew_shops[] = { 43220, 43222, 133075, 28197 };
const int evil_crew_shops[] = { 43221,  9704,  22481, 22648 };

int look_crew (P_char ch, P_ship ship);
int crew_shop_proc(int room, P_char ch, int cmd, char *arg)
{
  int n;
  char *bonuses;

  if( !IS_ALIVE(ch) )
    return FALSE;

  if( (cmd != CMD_LIST) && (cmd != CMD_HIRE) )
    return FALSE;

  // Skip beginning spaces.
  for( ; isspace(*arg); arg++ )
    ;

  if( IS_RACEWAR_EVIL(ch) && !IS_TRUSTED(ch) )
  {
    for( unsigned i = 0; i < sizeof(good_crew_shops) / sizeof(int); i++ )
    {
      if( good_crew_shops[i] == world[room].number )
      {
        send_to_char("&+gNoone here wants to talk to such &+Rsuspicious &+gcharacter.&n\n", ch);
        return TRUE;
      }
    }
  }
  if( IS_RACEWAR_GOOD(ch) && !IS_TRUSTED(ch) )
  {
    for( unsigned i = 0; i < sizeof(evil_crew_shops) / sizeof(int); i++ )
    {
      if( evil_crew_shops[i] == world[room].number )
      {
        send_to_char("&+gNoone here wants to talk to such &+Ysuspicious &+gcharacter.\r\n", ch);
        return TRUE;
      }
    }
  }

  P_ship ship = get_ship_from_owner( GET_NAME(ch) );

  if( !ship )
  {
    send_to_char("&+gYou own no ship; no one here wants to talk to you.&n\n", ch);
    return TRUE;
  }

  if( cmd == CMD_LIST )
  {
    send_to_char("&+YYou ask around and find these people looking for employment:&N\n\n", ch);

    send_to_char("                                         Base Skills     Base   Skill Modifiers   Frags  Hiring \n", ch);
    send_to_char("    -========== Crews ==========- Lvl Deck/Guns/Repair Stamina Deck/Guns/Repair to hire   Cost  \n", ch);
    for( int i = n = 0; i < MAXCREWS; i++ )
    {
      if( ship_crew_data[i].hire_room(world[room].number) )
      {
        send_to_char_f( ch, "&+Y%2d)&n %s &+C%2d&n &+W%4d&n/&+W%4d&n/&+W%4d&n   &+G%5d&n    &+W%c%-2d&n/ &+W%c%-2d&n/ &+W%c%-2d   &+r%7d &n%s\n",
          ++n, pad_ansi(ship_crew_data[i].name, 30).c_str(), ship_crew_data[i].level + 1,
          ship_crew_data[i].base_sail_skill, ship_crew_data[i].base_guns_skill, ship_crew_data[i].base_rpar_skill,
          ship_crew_data[i].base_stamina, (ship_crew_data[i].sail_mod > 0) ? '+' : ' ', ship_crew_data[i].sail_mod,
          (ship_crew_data[i].guns_mod > 0) ? '+' : ' ', ship_crew_data[i].guns_mod,
          (ship_crew_data[i].rpar_mod > 0) ? '+' : ' ', ship_crew_data[i].rpar_mod, ship_crew_data[i].hire_frags,
          coin_stringv(ship_crew_data[i].hire_cost, 6) );

        bonuses = crew_bonuses(ship_crew_data[i]);
        if( bonuses && (bonuses[0] != '\0') )
        {
          send_to_char_f(ch, "&+M  --- %s&n\n", bonuses );
        }
      }
    }

    send_to_char("\r\n\r\n", ch);
    send_to_char("                                                 Minimum  Skill   Skill     Frags        Hiring\r\n", ch);
    send_to_char("-======= Chief members =======-     Speciality    skill    gain  modifier  to hire        cost \r\n\r\n", ch);

    for( int i = 0; i < MAXCHIEFS; i++ )
    {
      if( ship_chief_data[i].hire_room(world[room].number) )
      {
        send_to_char_f(ch, "&+Y%2d)&N ", ++n);

        char name_format[20];
        sprintf(name_format, "%%-%ds&N", (int)(strlen(ship_chief_data[i].name) + (30 - strlen(strip_ansi(ship_chief_data[i].name).c_str()))));
        send_to_char_f(ch, name_format, ship_chief_data[i].name);

        send_to_char_f(ch, "   %-11s", ship_chief_data[i].get_spec());
        send_to_char_f(ch, "  &+W%5d", ship_chief_data[i].min_skill);

        if( ship_chief_data[i].skill_gain_bonus != 0 )
        {
          char skill_gain[10];
          sprintf(skill_gain, "%s%d%%", (ship_chief_data[i].skill_gain_bonus > 0) ? "+" : "", ship_chief_data[i].skill_gain_bonus);
          send_to_char_f(ch, "  &+W%6s", skill_gain);
        }
        else
          send_to_char_f(ch, "        ");

        if( ship_chief_data[i].skill_mod != 0 )
        {
          char skill_mod[10];
          sprintf(skill_mod, "%s%d", (ship_chief_data[i].skill_mod > 0) ? "+" : "", ship_chief_data[i].skill_mod);
          send_to_char_f(ch, "    &+W%3s", skill_mod);
        }
        else
          send_to_char_f(ch, "       ");

        send_to_char_f(ch, "       &+r%4d    &+W%20s\r\n", ship_chief_data[i].hire_frags, coin_stringv(ship_chief_data[i].hire_cost));
      }
    }
    send_to_char("\r\n\r\n", ch);
    look_crew (ch, ship);
    return TRUE;
  }

  if( cmd == CMD_HIRE )
  {
    if( !is_number(arg) )
    {
      send_to_char("Valid syntax 'hire <number>'\r\n", ch);
      return TRUE;
    }

    int n = atoi(arg);
    int c = 0;
    for( int i = 0; i < MAXCREWS; i++ )
    {
      if( ship_crew_data[i].hire_room(world[room].number) && ++c == n )
      {
        if( i == ship->crew.index )
        {
          send_to_char("&+wYou already have this crew!\r\n", ch);
          return TRUE;
        }
        if( ship->frags < ship_crew_data[i].hire_frags
          && (ship->crew.sail_skill < ship_crew_data[i].base_sail_skill
          || ship->crew.guns_skill < ship_crew_data[i].base_guns_skill
          || ship->crew.rpar_skill < ship_crew_data[i].base_rpar_skill) )
        {
          send_to_char("&+wNever heard of you and y'er crew looks weak, get lost!\r\n", ch);
          return TRUE;
        }
        int cost = ship_crew_data[i].hire_cost;
        if( cost == 0 && !IS_TRUSTED(ch) )
        {
          send_to_char_f(ch, "You can't hire this crew!\r\n");
          return TRUE;
        }
        if( GET_MONEY(ch) < cost )
        {
          send_to_char_f(ch, "It will cost %s to hire this crew!\r\n", coin_stringv(cost));
          return TRUE;
        }

        SUB_MONEY(ch, cost, 0);
        send_to_char ("Aye aye cap'n!  We'll be on yer ship before you board!\r\n", ch);
        change_crew(ship, i, true);
        update_ship_status(ship);
        write_ship(ship);
        return TRUE;
      }
    }

    for( int i = 0; i < MAXCHIEFS; i++ )
    {
      if( ship_chief_data[i].hire_room(world[room].number) && ++c == n )
      {
        if (i == ship->crew.sail_chief || i == ship->crew.guns_chief || i == ship->crew.rpar_chief)
        {
          send_to_char("&+wYou already have this chief!\r\n", ch);
          return TRUE;
        }
        if( ship->frags < ship_chief_data[i].hire_frags
          && (( ship_chief_data[i].type == SAIL_CHIEF && ship->crew.sail_skill < ship_chief_data[i].min_skill )
          || ( ship_chief_data[i].type == GUNS_CHIEF && ship->crew.guns_skill < ship_chief_data[i].min_skill )
          || ( ship_chief_data[i].type == RPAR_CHIEF && ship->crew.rpar_skill < ship_chief_data[i].min_skill )) )
        {
          send_to_char("&+wI won't work with these greenhorns unless their captain is someone famous.\r\n", ch);
          return TRUE;
        }
        int cost = ship_chief_data[i].hire_cost;
        if( cost == 0 && !IS_TRUSTED(ch) )
        {
          send_to_char_f(ch, "You can't hire this guy!\r\n");
          return TRUE;
        }
        if( GET_MONEY(ch) < cost )
        {
          send_to_char_f(ch, "It will cost %s to hire this guy!\r\n", coin_stringv(cost));
          return TRUE;
        }

        SUB_MONEY(ch, cost, 0);
        send_to_char ("Aye aye cap'n!  I'll be on yer ship before you board!\r\n", ch);
        set_chief(ship, i);
        update_ship_status(ship);
        write_ship(ship);
        return TRUE;
      }
    }
    send_to_char("Invalid number.\r\n", ch);
    return TRUE;
  }
  return FALSE;
}


/////////////////////////
/// Automatons quest
/////////////////////////


#define IS_MOONSTONE(obj)          (obj_index[obj->R_num].virtual_number == AUTOMATONS_MOONSTONE)
#define IS_MOONSTONE_FRAGMENT(obj) (obj_index[obj->R_num].virtual_number == AUTOMATONS_MOONSTONE_FRAGMENT)
#define IS_MOONSTONE_CORE(obj)     (obj_index[obj->R_num].virtual_number == AUTOMATONS_MOONSTONE_CORE)
#define IS_MOONSTONE_PART(obj)     (IS_MOONSTONE_FRAGMENT(obj) || IS_MOONSTONE_CORE(obj))

bool moonstone_fragment(P_obj obj, P_char ch, int cmd, char *argument)
{
  if (cmd == CMD_SET_PERIODIC)
    return TRUE;

  if (!obj)
    return FALSE;

  if (cmd == CMD_PERIODIC)
  {
    if (obj->loc_p == LOC_CARRIED && obj->loc.carrying != 0)
    {
      ch = obj->loc.carrying;
      int fragment_count = 0, core_count = 0;
      for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
      {
        if (IS_MOONSTONE_FRAGMENT(obj))
          fragment_count++;
        if (IS_MOONSTONE_CORE(obj))
          core_count++;
      }
      if (core_count > 1)
      {
          // TODO: something nasty
          return TRUE;
      }
      if (fragment_count == 2 && core_count == 1)
      {
        send_to_char("&+WMoonstone &+Rf&+rra&+Rg&+rme&+Rn&+rts &+Gsuddenly glow &+Wbrightly &+Gin your hands and combine into &+Rh&+rea&+Rr&+rt-&+Rs&+rha&+Rp&+red &+Wobject&+G!&N\r\n\r\n", ch);
        P_obj next_obj = 0;
        for (P_obj obj = ch->carrying; obj; obj = next_obj)
        {
          next_obj = obj->next_content;
          if (IS_MOONSTONE_PART(obj))
          {
              obj_from_char(obj);
              extract_obj(obj, TRUE);
          }
        }
        int r_num = real_object(AUTOMATONS_MOONSTONE);
        if(r_num < 0)
          return FALSE;
        P_obj moonstone = read_object(r_num, REAL);
        if (!moonstone)
          return FALSE;
        obj_to_char(moonstone, ch);

      }
    }
    return TRUE;
  }
  return FALSE;
}

P_obj has_moonstone_part(P_char ch)
{
  for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
  {
    if (IS_MOONSTONE_PART(obj))
      return obj;
  }
  return NULL;
}
P_obj has_moonstone_fragment(P_char ch)
{
  for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
  {
    if (IS_MOONSTONE_FRAGMENT(obj))
      return obj;
  }
  return NULL;
}
P_obj has_moonstone(P_char ch)
{
  for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
  {
    if (IS_MOONSTONE(obj))
      return obj;
  }
  return NULL;
}

bool load_moonstone_fragments()
{
  P_char olhydra = 0;
  for (P_char mob = character_list; mob; mob = mob->next)
  {
    if(IS_NPC(mob) && GET_VNUM(mob) == 23240)
    {
        olhydra = mob;
        break;
    }
  }

  int r_num = real_object(AUTOMATONS_MOONSTONE_FRAGMENT);
  if (r_num < 0) return FALSE;
  P_obj fragment = read_object(r_num, REAL);

  if (olhydra && number(0, 1))
  {
    obj_to_char(fragment, olhydra);
  }
  else
  {
    obj_to_room(fragment, real_room(31724)); // to locker
  }

  P_char automaton = 0;
  for (P_char mob = character_list; mob; mob = mob->next)
  {
    if(IS_NPC(mob) && GET_VNUM(mob) == 12027)
    {
        automaton = mob;
        break;
    }
  }
  if (!automaton)
      return FALSE;

  r_num = real_object(AUTOMATONS_MOONSTONE_FRAGMENT);
  if (r_num < 0) return FALSE;
  fragment = read_object(r_num, REAL);

  obj_to_char(fragment, automaton);
  return TRUE;
}

int erzul_proc(P_char ch, P_char pl, int cmd, char *arg)
{
  /*
   check for periodic event calls 
   */
  if (cmd == -10)
    return TRUE;

  if (GET_STAT(ch) <= STAT_SLEEPING)
    return FALSE;

  if (pl == ch)
    return FALSE;

  if (!pl || !ch)
      return FALSE;

  P_ship ship = get_ship_from_owner(GET_NAME(pl));
  int cost = ship_crew_data[AUTOMATON_CREW].hire_cost;

  if (cmd == CMD_ASK)
  {
    half_chop(arg, arg1, arg2);
    if (has_moonstone_part(pl))
    {
       send_to_char ("Erzul says '&+GYou have it! You have the moonstone!\r\n", pl);
       send_to_char ("&+G  What?! It's broken! You broke my masterpiece!!&N'\r\n", pl);
       send_to_char ("Erzul screams '&+RNOOOOOO!!!&N'\r\n", pl);
       attack(ch, pl);
       return TRUE;
    } 
    if (P_obj moonstone = has_moonstone(pl))
    {
      if (*arg2)
      {
        if (isname(arg2, "automaton automatons"))
        {
          if (!ship || (ship->frags < ship_crew_data[AUTOMATON_CREW].hire_frags))
          {
            send_to_char ("Erzul says '&+GI'm sorry, but you are not famous enough to be trusted with this crew!\r\n", pl);
            send_to_char ("&+G  But... if you &+Wask&+G for your &+Wreward&+G you will receive it!&N'\r\n", pl);
            return TRUE;
          }
          else
          {
            send_to_char_f(pl, "Erzul says '&+GOkay, i heard about you. I will trust you with my best creation!\r\n");
            send_to_char_f(pl, "&+G  But i need about &+W%s &+Gto start this work!&N'\r\n", coin_stringv(cost));
            if (GET_MONEY(pl) < cost)
            {
              return TRUE;
            }
            send_to_char_f(pl, "You hand over the &+Wmoonstone&N and the &+Wmoney&N.\r\n");
            SUB_MONEY(pl, cost, 0);
            obj_from_char(moonstone);
            extract_obj(moonstone, TRUE);
            set_crew(ship, AUTOMATON_CREW, false);
            update_ship_status(ship);
            write_ship(ship);
            send_to_char ("Erzul says '&+GThey'll be at your ship by the time you get there!&N'\r\n", pl);
            return TRUE;
          }
        }
        if (isname(arg2, "reward"))
        {
          send_to_char_f(pl, "You hand over the &+Wmoonstone&N.\r\n");
          obj_from_char(moonstone);
          extract_obj(moonstone, TRUE);

          int r_num = real_object(9460);
          if (r_num < 0) return FALSE;
          P_obj ring = read_object(r_num, REAL);
          send_to_char_f(pl, "Erzul rewarded you with %s&N.\r\n", ring->short_description);
          obj_to_char(ring, pl);
          return TRUE;
        }
      }
      send_to_char ("Erzul says '&+GYou have it! You have the moonstone! &+WAsk&+G for your &+Wreward&+G and you will receive it!&N'\r\n", pl);
      if (ship && (ship->frags >= ship_crew_data[AUTOMATON_CREW].hire_frags))
      {
        send_to_char ("Erzul says '&+GI could even trust you with my &+Wautomatons &+Gif you &+Wask &+Gfor them!&N'\r\n", pl);
      }
      return TRUE;
    }
    if (*arg2)
    {
        if (isname(arg2, "hello hi quest help"))
        {
          send_to_char ("Erzul says '&+GThat cursed &+WXexos&N!  &+GNow I can't complete my masterpiece!'\r\n", pl);
          return TRUE;
        }
        if (isname(arg2, "xexos"))
        {
          send_to_char ("Erzul says '&+GYes, Xexos, the bastard who stole the &+Wmoonstone&N &+Gheart i've created!&N\r\n", pl);
          return TRUE;
        }
        if (isname(arg2, "moonstone"))
        {
          send_to_char ("Erzul says '&+GThe moonstone was my life's work.  It's the power source I was\r\n", pl);
          send_to_char ("&+g  going to use to power my &+Wautomatons&N, &+gmy masterpiece!&N'", pl);
          return TRUE;
        }
        if (isname(arg2, "automaton automatons"))
        {
          send_to_char ("Erzul says '&+GYes, automatons, ultimate ship crew made of powerful and skilled golems.\r\n", pl);
          send_to_char ("  &+GIf only someone brought back my moonstone, I could astound the world with my greatest work!\r\n", pl);
          send_to_char ("  &+GThough it would &+Wcost&+G a fortune to make them...&N'\r\n", pl);
          return TRUE;
        }
        if (isname(arg2, "cost money"))
        {
          send_to_char_f (pl, "Erzul says '&+GHow much? I would need about &+W%s &+Gfor that work.\r\n", coin_stringv(cost));
          send_to_char_f (pl, "&+G  Does it even matter now, as i can not even start it!'\r\n");
          return TRUE;
        }
    }
    return FALSE;
  }
  return FALSE;
}


// ring: 9460
