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

int list_cargo(P_char ch, P_ship ship, int owned)
{
    if (!owned)
    {
        send_to_char("You do not own a ship!\r\n", ch);
        return TRUE;
    }
    int rroom = 0;
    while (rroom < NUM_PORTS) 
    {
        if (ports[rroom].loc_room == world[ch->in_room].number)
            break;
        rroom++;
    }
    if (rroom >= NUM_PORTS) 
    {
        send_to_char ("There is no cargo for sale here!\r\n", ch);
        return TRUE;
    }
    send_to_char ("&+y---=== For sale ===---&N\r\n", ch);
    
    int cost = cargo_sell_price(rroom);
    
    // hook for epic bonus
    cost -= (int) (cost * get_epic_bonus(ch, EPIC_BONUS_CARGO));

    //if( GET_LEVEL(ch) < 50 )
    //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);
    
    send_to_char_f(ch, "%s:&N %s &Nper crate.\r\n", cargo_type_name(rroom), coin_stringv(cost));

    send_to_char("\r\n&+y---=== We're buying ===---&N\r\n", ch);
    for (int i = 0; i < NUM_PORTS; i++) 
    {
        /*if (i == rroom)
            continue;*/
      
        cost = cargo_buy_price(rroom, i);

        //if( GET_LEVEL(ch) < 50 )
        //  cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);
    
        send_to_char_f(ch, "%s %s &nper crate.\r\n", pad_ansi(cargo_type_name(i), 30).c_str(), coin_stringv(cost));
    }

    send_to_char("\r\nTo buy cargo, type 'buy cargo <number of crates>'\r\n", ch);
    send_to_char("To sell cargo, type 'sell cargo'\r\n", ch);

    if (GET_ALIGNMENT(ch) <= MINCONTRAALIGN) 
    {
        if (can_buy_contraband(ship, rroom))
        {
            send_to_char ("\r\n&+L---=== Contraband for sale ===---&N\r\n", ch);
            cost = contra_sell_price(rroom);
	    
	    // hook for epic bonus
	    cost -= (int) (cost * get_epic_bonus(ch, EPIC_BONUS_CARGO));

            send_to_char_f(ch, "%s&n: %s &+Lper crate.&N\r\n", contra_type_name(rroom), coin_stringv(cost));
        }
        bool caption = false;
        for (int i = 0; i < NUM_PORTS; i++) 
        {
            /*if (i == rroom)
                continue;*/
          
            if (can_buy_contraband(ship, i))
            {
                if (!caption)
                {
                    send_to_char("\r\n&+L---=== We buy contraband for ===---&N\r\n", ch);
                    caption = true;
                }
                cost = contra_buy_price(rroom, i);
                send_to_char_f(ch, "%s %s &nper crate.\r\n", pad_ansi(contra_type_name(i), 30).c_str(), coin_stringv(cost));
            }
        }
        if (caption)
        {
            send_to_char("\r\nTo buy contraband, type 'buy contraband <number of crates>'\r\n", ch);
            send_to_char("To sell contraband, type 'sell contraband'\r\n", ch);
        }
    }

    send_to_char("\r\n&+cYour cargo manifest\r\n", ch);
    send_to_char("&+W----------------------------------\r\n", ch);

    for (int i = 0; i < MAXSLOTS; i++) 
    {
        if (ship->slot[i].type == SLOT_CARGO) 
        {
            int cargo_type = ship->slot[i].index;
            /*if (cargo_type == rroom) 
            {
                send_to_char_f(ch, "%s&n, &+W%d&n crates, bought for %s&n.\r\n", 
                    cargo_type_name(cargo_type), 
                    ship->slot[i].val0,
                    coin_stringv(ship->slot[i].val1));
            }
            else
            {*/
                cost = cargo_buy_price(rroom, cargo_type) * ship->slot[i].val0;

                //if( GET_LEVEL(ch) < 50 )
                //    cost = (int) (cost * (float) ((float) GET_LEVEL(ch) / 50.0));
            
                int profit = 100;
                if (ship->slot[i].val1 != 0)
                    profit = (int)(( ((float)cost / (float)ship->slot[i].val1) - 1.00 ) * 100.0);
            
                sprintf(buf, "%s", ship->slot[i].val1 != 0 ? coin_stringv(ship->slot[i].val1) : "nothing");
                send_to_char_f(ch, "%s&n, &+Y%d&n crates. Bought for %s&n, can sell for %s (%d%% profit)\r\n",
                    cargo_type_name(cargo_type), 
                    ship->slot[i].val0,
                    buf,
                    coin_stringv(cost),
                    profit);
            //}
        }
        if (ship->slot[i].type == SLOT_CONTRABAND) 
        {
            int contra_type = ship->slot[i].index;
            /*if (contra_type == rroom) 
            {
                send_to_char_f(ch, "&+L*&n%s&n, &+Y%d&n crates, bought for %s&n.\r\n", 
                    contra_type_name(contra_type), 
                    ship->slot[i].val0,
                    coin_stringv(ship->slot[i].val1));
            }
            else
            {*/
                cost = contra_buy_price(rroom, contra_type);
                cost *= ship->slot[i].val0;

                //if( GET_LEVEL(ch) < 50 )
                //    cost = (int) (cost * (float) ((float) GET_LEVEL(ch) / 50.0));
            
                int profit = 100;
                if (ship->slot[i].val1 != 0)
                    profit = (int)(( ((float)cost / (float)ship->slot[i].val1) - 1.00 ) * 100.0);

                sprintf(buf, "%s", ship->slot[i].val1 != 0 ? coin_stringv(ship->slot[i].val1) : "nothing");
                send_to_char_f(ch, "&+L*&n%s&n, &+Y%d&n crates. Bought for %s&n, can sell for %s (%d%% profit)\r\n", 
                    contra_type_name(contra_type),
                    ship->slot[i].val0,
                    buf,
                    coin_stringv(cost),
                    profit);
            //}
        }
    }

    send_to_char_f(ch, "\r\nCapacity: &+W%d&n/&+W%d\r\n", SHIP_AVAIL_CARGO_LOAD(ship), SHIP_MAX_CARGO_LOAD(ship));

    return TRUE;
}

int list_weapons(P_char ch, P_ship ship, int owned)
{
    char rng[20], dam[20];
    if (!owned)
    {
        send_to_char("You do not own a ship!\r\n", ch);
        return TRUE;
    }

    send_to_char("&+gWeapons\r\n", ch);
    send_to_char("&+y==================================================================================================\r\n", ch);
    send_to_char("    Name               Weight  Range   Damage  Ammo Disper  Sail  Hull  Sail Reload  Cost            \r\n", ch);
    send_to_char("                                                      sion   Hit   Dam   Dam   Time                  \r\n", ch);
    send_to_char("&+W--------------------------------------------------------------------------------------------------\r\n", ch);
    for (int i = 0; i < MAXWEAPON; i++) 
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

        send_to_char_f(ch,  "%2d) %s%-20s   &+Y%2d  %5s  %7s    %2d    %3d  %3d%%  %3d%%  %3d%%    %3d  &n%10s&n\r\n",
          i + 1, 
          ship_allowed_weapons[ship->m_class][i] && (ship->frags >= weapon_data[i].min_frags || ship->crew.guns_skill >= weapon_data[i].min_crewexp) ? "&+W" : "&+L", 
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
          coin_stringv(weapon_data[i].cost));
    }

    send_to_char("\r\nTo buy a weapon, type 'buy weapon <number> <fore/rear/port/starboard>'\r\n", ch);
    send_to_char("To sell a weapon, type 'sell <slot number>'\r\n", ch);
    send_to_char("To reload a weapon, type 'reload <weapon number>'\r\n", ch);
    send_to_char("\r\n", ch);
    
    return TRUE;
}

int list_equipment(P_char ch, P_ship ship, int owned)
{
    if (!owned)
    {
        send_to_char("You do not own a ship!\r\n", ch);
        return TRUE;
    }

    send_to_char("&+gEqiupment\r\n", ch);
    send_to_char("&+y===============================================\r\n", ch);
    send_to_char("    Name                  Weight  Cost            \r\n", ch);
    send_to_char("                                                  \r\n", ch);
    send_to_char("&+W-----------------------------------------------\r\n", ch);
    for (int i = 0; i < MAXEQUIPMENT; i++) 
    {
        int weight = equipment_data[i].weight;
        if (i == E_RAM) weight = eq_ram_weight(ship);
        if (i == E_LEVISTONE) weight = eq_levistone_weight(ship);

        int cost = equipment_data[i].cost;
        if (i == E_RAM) cost = eq_ram_cost(ship);

        send_to_char_f(ch,  "%2d) %s%-20s      &+Y%2d  &n%10s&n\r\n",
          i + 1, 
          ship_allowed_equipment[ship->m_class][i] && (ship->frags >= equipment_data[i].min_frags) ? "&+W" : "&+L", 
          equipment_data[i].name,
          weight,
          coin_stringv(cost));
    }

    send_to_char("\r\nTo buy an equipment, type 'buy equipment <number>'\r\n", ch);
    send_to_char("To sell an equipment, type 'sell <slot number>'\r\n", ch);
    send_to_char("\r\n", ch);
    
    return TRUE;
}

int list_hulls (P_char ch, P_ship ship, int owned)
{
    if (owned)
    {
        send_to_char("&+gHull Upgrades\r\n",ch);
        send_to_char("&+y===================================================================================&N\r\n", ch);
        send_to_char("                 Load   Cargo  Passenger   Max   Total  Weapons  Epic     \r\n", ch);
        send_to_char("    Name         Cap.    Cap.     Cap.    Speed  Armor (F/S/R/P) Cost Cost\r\n", ch);
        send_to_char("&+W-----------------------------------------------------------------------------------\r\n", ch);
        for (int i = 0; i < MAXSHIPCLASS; i++)
        {
            if (SHIPTYPE_COST(i) == 0)
            {
              continue;
            }
            send_to_char_f(ch, "%-2d) %-11s   &+Y%-3d    &+Y%-3d       &+Y%-2d      &+Y%-3d    &+Y%-3d   &+Y%1d/&+Y%1d/&+Y%1d/&+Y%1d   &+R%c   &n%-14s\r\n", 
                i + 1,
                SHIPTYPE_NAME(i), 
                SHIPTYPE_MAX_WEIGHT(i),
                SHIPTYPE_CARGO(i),
                SHIPTYPE_PEOPLE(i),
                SHIPTYPE_SPEED(i),
                ship_arc_properties[i].armor[SIDE_FORE] + ship_arc_properties[i].armor[SIDE_STAR] + ship_arc_properties[i].armor[SIDE_REAR] + ship_arc_properties[i].armor[SIDE_PORT],
                ship_arc_properties[i].max_weapon_slots[SIDE_FORE], ship_arc_properties[i].max_weapon_slots[SIDE_STAR], ship_arc_properties[i].max_weapon_slots[SIDE_REAR], ship_arc_properties[i].max_weapon_slots[SIDE_PORT],
                (SHIPTYPE_EPIC_COST(i) > 0) ? ('0' + SHIPTYPE_EPIC_COST(i)) : ' ',
                coin_stringv(SHIPTYPE_COST(i)));
        }
        send_to_char("\r\nTo upgrade/downgrade your hull, type 'buy <number>'\r\n", ch);
        send_to_char("To sell your ship completely, type 'sell ship'. &+RCAUTION!&n You will loose your frags and crews!\r\n", ch);

        send_to_char("\r\n", ch);


        send_to_char("&+gRepairs\r\n",ch);
        send_to_char("&+y===================================================================&N\r\n", ch);

        if (ship->timer[T_MAINTENANCE] > 0) 
        {
            send_to_char_f(ch, "Your ship is currently being worked on, please wait another %.1f hours.\r\n", (float) ship->timer[T_MAINTENANCE] / 75.0);
        }
        else
        {
            send_to_char("To repair, type 'repair <armor/internal/sail/weapon> <fore/port/starboard/rear>'\r\n", ch);
        }

        send_to_char("\r\n", ch);
        send_to_char("&+gRename\r\n",ch);
        send_to_char("&+y===================================================================&N\r\n", ch);
        send_to_char("To rename ship, for a price, type 'buy rename <new ship name>'\r\n", ch);
    }
    else
    {
        send_to_char("&+cShips Available\r\n",ch);
        send_to_char("&+y=======================================================&N\r\n", ch);

        send_to_char("&+y===================================================================================&N\r\n", ch);
        send_to_char("                 Load   Cargo  Passenger   Max   Total  Weapons  Epic     \r\n", ch);
        send_to_char("    Name         Cap.    Cap.     Cap.    Speed  Armor (F/S/R/P) Cost Cost\r\n", ch);
        send_to_char("&+W-----------------------------------------------------------------------------------\r\n", ch);
        for (int i = 0; i < MAXSHIPCLASS; i++)
        {
            if (SHIPTYPE_COST(i) == 0)
            {
              continue;
            }
            send_to_char_f(ch, "%-2d) %-11s   &+Y%-3d    &+Y%-3d       &+Y%-2d      &+Y%-3d    &+Y%-3d   &+Y%1d/&+Y%1d/&+Y%1d/&+Y%1d   &+R%c   &n%-14s\r\n",
                i + 1,
                SHIPTYPE_NAME(i), 
                SHIPTYPE_MAX_WEIGHT(i),
                SHIPTYPE_CARGO(i),
                SHIPTYPE_PEOPLE(i),
                SHIPTYPE_SPEED(i),
                ship_arc_properties[i].armor[SIDE_FORE] + ship_arc_properties[i].armor[SIDE_STAR] + ship_arc_properties[i].armor[SIDE_REAR] + ship_arc_properties[i].armor[SIDE_PORT],
                ship_arc_properties[i].max_weapon_slots[SIDE_FORE], ship_arc_properties[i].max_weapon_slots[SIDE_STAR], ship_arc_properties[i].max_weapon_slots[SIDE_REAR], ship_arc_properties[i].max_weapon_slots[SIDE_PORT],
                (SHIPTYPE_EPIC_COST(i) > 0) ? ('0' + SHIPTYPE_EPIC_COST(i)) : ' ',
                coin_stringv(SHIPTYPE_COST(i)));
        }
        if( affected_by_spell( ch, AIP_FREESLOOP ) )
        {
          send_to_char( "\r\n&+yYou qualify for a &+WFree Sloop&+y!&n\n\r", ch );
          send_to_char( "&+yTo claim this reward, use the command '&+Gbuy hull 1 <name>&+y' where <name> is the name of your ship (ansi color allowed).&n\n\r", ch );
        }
        send_to_char("\r\n", ch);
        send_to_char("&+YRead HELP WARSHIP before buying one!\r\n", ch);
        send_to_char("To buy a ship, type 'buy <number> <name of ship>'\r\n", ch);
    }
    return TRUE;
}

int summon_ship (P_char ch, P_ship ship, bool time_only)
{

    if (ship->location == ch->in_room) {
        send_to_char("Your ship is already docked here!\r\n", ch);
        return TRUE;
    }
    if (IS_SET(ship->flags, SINKING)) {
        send_to_char("We can't summon your ship.  Sorry.\r\n", ch);
        return TRUE;
    }
    if (ship->timer[T_BSTATION] > 0 && !IS_TRUSTED(ch)) 
    {
        send_to_char ("Your crew is not responding to our summons!\r\n", ch);
        return TRUE;
    }
    if (!is_Raidable(ch, 0, 0))
    {
        send_to_char("\r\n&+RGET RAIDABLE!\r\n", ch);
        return TRUE;
    }
    if (IS_SET(ship->flags, SUMMONED)) {

        send_to_char ("There is already an order out on your ship.\r\n", ch);
        return TRUE;
    }

    if (!time_only)
    {
        int summon_cost = SHIPTYPE_HULL_WEIGHT(ship->m_class) * 50;
        if (GET_MONEY(ch) < summon_cost) 
        {
            send_to_char_f(ch, "It will cost %s to summon your ship!\r\n", coin_stringv(summon_cost));
            return TRUE;
        }
    
        if (ship->location == DAVY_JONES_LOCKER) {
            send_to_char("You start to call your ship back from &+LDavy Jones Locker...\r\n", ch);
        }
    
        SUB_MONEY(ch, summon_cost, 0);
    }

    int summontime, pvp = false;
    if( IS_TRUSTED(ch) )
        summontime = 0;
    else if (SHIP_CLASS(ship) == SH_SLOOP || SHIP_CLASS(ship) == SH_YACHT)
    {
        summontime = (280 * 50) / MAX(get_maxspeed_without_cargo(ship), 1);
    }
    else
    {
        summontime = (280 * 70) / MAX((get_maxspeed_without_cargo(ship) - 20), 1);
        pvp = ocean_pvp_state();
        if (pvp) summontime *= 4;
    }
    summontime = MIN(summontime, 200 * 70);

    if (pvp)
        send_to_char_f(ch, "Due to dangerous conditions, it will take about %d hours for your ship to get here.\r\n", summontime / 280);
    else if (time_only)
        send_to_char_f(ch, "It will take %d hours for your ship to get here.\r\n", summontime / 280);
    else
        send_to_char_f(ch, "Thanks for your business, it will take %d hours for your ship to get here.\r\n", summontime / 280);

    if (!time_only)
    {
        if(!IS_TRUSTED(ch))
        {
            clear_cargo(ship);
            write_ship(ship);
        }
        SET_BIT(ship->flags, SUMMONED);
        everyone_get_out_ship(ship);
        send_to_room_f(ship->location, "&+y%s is called away elsewhere.&N\r\n", ship->name);
        obj_from_room(ship->shipobj);

        sprintf(buf, "%s %d", GET_NAME(ch), ch->in_room);
        add_event(summon_ship_event, summontime, NULL, NULL, NULL, 0, buf, strlen(buf)+1);
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
    send_to_char_f(ch, "We don't buy %s&n here.\r\n", cargo_type_name(type));
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
    send_to_char("Because your cargo is obviosly stolen, local merchants bargain the price down.\r\n", ch);
    cost = cost * 0.6;
  }

  sprintf(buf, "CARGO: %s sold &+W%d&n %s&n at %s&n [%d] for %s&n (%d percent profit)", GET_NAME(ch), crates, cargo_type_name(type), ports[rroom].loc_name, ports[rroom].loc_room, coin_stringv(cost), profit);
  statuslog(56, buf);
  logit(LOG_SHIP, strip_ansi(buf).c_str());

  send_to_char_f(ch, "You sell &+W%d&n crates of %s&n for %s&n, for a %d%% profit.\r\n", crates, cargo_type_name(type), coin_stringv(cost), profit);

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
  if( !(paf = get_spell_from_char(ch, ACH_CARGOCOUNT)) )
  {
    paf = apply_achievement(ch, ACH_CARGOCOUNT);
    paf->modifier = 0;
  }
  if( paf->modifier < 10000 && paf->modifier + crates >= 10000 )
  {
    send_to_char( "&+rCon&+Rgra&+Wtula&+Rtio&+rns! You have completed the &+RTrader&+r achievement!&n\r\n", ch );
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
    send_to_char("You ship is not here to unload cargo!\r\n", ch);
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
    send_to_char("We don't buy cargo here!\r\n", ch);
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
      send_to_char("You don't have anything we're interested in.\r\n", ch);
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
        send_to_char_f(ch, "You receive %s&n.\r\n", coin_stringv(total_cost));
	if(has_eq_diplomat(ship))
	 send_to_char("Your &+bdip&+Blo&+bmat &nstatus costs you a &+W10&n percent cut on your profits.\r\n", ch);
        send_to_char("Thanks for your business!\r\n", ch);        
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
        send_to_char_f(ch, "We're not interested in your %s&n.\r\n", contra_type_name(type));
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
        send_to_char("Because your contraband is obviosly stolen, local merchants bargain the price down.\r\n", ch);
        cost = cost * 0.6;
      }

      sprintf(buf, "CONTRABAND: %s sold &+W%d&n %s&n at %s&n [%d] for %s&n (%d percent profit)", GET_NAME(ch), crates, contra_type_name(type), ports[rroom].loc_name, ports[rroom].loc_room, coin_stringv(cost), profit);
      statuslog(56, buf);
      logit(LOG_SHIP, strip_ansi(buf).c_str());

      send_to_char_f(ch, "You sell &+W%d&n crates of %s&n for %s&n, for a %d%% profit.\r\n", crates, contra_type_name(type), coin_stringv(cost), profit);

      // economy affect
      adjust_ship_market(SOLD_CONTRA, rroom, type, crates);

      return cost;      
    //}
}

int sell_contra(P_char ch, P_ship ship, int slot)
{
    if (ship->location != ch->in_room) 
    {
        send_to_char ("You ship is not here to unload contraband!\r\n", ch);
        return TRUE;
    }

    int rroom = 0;    
    for (rroom = 0; rroom < NUM_PORTS; ++rroom) 
        if (ports[rroom].loc_room == world[ch->in_room].number)
            break;
    if (rroom >= NUM_PORTS) 
    {
        send_to_char("We don't buy any contraband here!\r\n", ch);
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
            send_to_char("You don't have anything we're interested in.\r\n", ch);
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
        send_to_char_f(ch, "You receive %s&n.\r\n", coin_stringv(total_cost));
        send_to_char("Thanks for your business!\r\n", ch);        
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

int sell_slot (P_char ch, P_ship ship, int slot)
{
  if (ship->location != ch->in_room) 
  {
    send_to_char("Your ship is not here!\r\nYou can only sell your entire ship, not specific parts!\r\n", ch);
    return TRUE;
  }
  if (slot < 0 || slot >= MAXSLOTS) 
  {
    send_to_char("Invalid slot number.\r\n", ch);
    return TRUE;
  }
  if (ship->slot[slot].type == SLOT_WEAPON) 
  {
    int cost;
    if (SHIP_WEAPON_DAMAGED(ship, slot))
      cost = (int) (weapon_data[ship->slot[slot].index].cost * .1);
    else
      cost = (int) (weapon_data[ship->slot[slot].index].cost * .9);

    ADD_MONEY(ch, cost);
    send_to_char_f(ch, "Here's %s for that %s.\r\n", coin_stringv(cost), ship->slot[slot].get_description());
    ship->slot[slot].clear();
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
  } 
  else if (ship->slot[slot].type == SLOT_EQUIPMENT) 
  {
    for (int i = 0; i < MAXSLOTS; i++)
    {
      if ((ship->slot[i].type == SLOT_CONTRABAND) || (ship->slot[i].type == SLOT_CARGO))
      {
        if(has_eq_diplomat(ship))
        {
          send_to_char("You cannot sell your diplomat flag with cargo onboard. Sell the cargo first!\r\n", ch);
          return TRUE;
        }
      } 
      else
      {
        int cost = equipment_data[ship->slot[slot].index].cost;
        if (ship->slot[slot].index == E_RAM) cost = eq_ram_cost(ship);
        cost = cost * 0.9;

        ADD_MONEY(ch, cost);
        send_to_char_f(ch, "Here's %s for that %s.\r\n", coin_stringv(cost), ship->slot[slot].get_description());
        ship->slot[slot].clear();
        update_ship_status(ship);
        write_ship(ship);
        return TRUE;
      } 
    }
  }
  else if (ship->slot[slot].type == SLOT_CARGO)
  {
    return sell_cargo(ch, ship, slot);
  }
  else if (ship->slot[slot].type == SLOT_CONTRABAND)
  {
    return sell_contra(ch, ship, slot);
  }
  else 
  {
    send_to_char ("That slot does not contain anything to sell!\r\n", ch);
    return TRUE;
  }

  return FALSE;
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

    act_to_all_in_ship(ship, "The port authorities board the ship in search of contraband...");

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
                
                act_to_all_in_ship_f(ship, "and find %d crates of %s&n, which they confiscate.", confiscated, contra_type_name(type));
                ship->slot[slot].val0 -= confiscated;
                if (ship->slot[slot].val0 <= 0)
                    ship->slot[slot].clear();
                did_confiscate = true;
            }
        }
    }
    if (!did_confiscate)
        act_to_all_in_ship(ship, "but don't find anything suspicious.");
}

int sell_ship(P_char ch, P_ship ship, const char* arg)
{
    int i = 0, k = 0, j;

    send_to_char ("&+RSelling ships completely is not allowed.&N\r\n", ch);
    return TRUE;

    if (!arg || !(*arg) || !isname(arg, "confirm"))
    {
        send_to_char ("&+RIf you are sure you want to sell your ship, type 'sell ship confirm'.\r\n&+RYou WILL loose your frags and crews!&N\r\n", ch);
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
        send_to_char ("Since your ship is not here, you only get half the price, salvage costs and all!\r\n", ch);
    }
    ADD_MONEY(ch, cost);
    send_to_char_f(ch, "Here's %s for your ship: %s.\r\n", coin_stringv(cost), ship->name);

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
        send_to_char("Your ship is unscathed!\r\n", ch);
        return TRUE;
    }

    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "It will cost costs %s to repair your ship!\r\n", coin_stringv(cost));
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

    send_to_char_f(ch, "Thank you for for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
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
        send_to_char("Your sails are fine, they don't need repair!\r\n", ch);
        return TRUE;
    }

    int cost = total_damage * 2000;

    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "This sail costs %s to repair!\r\n", coin_stringv(cost));
        return TRUE;
    }

    if( cost > 0 )
    {
        send_to_char_f(ch, "You pay %s&n.\r\n", coin_stringv(cost));
    }

    SUB_MONEY(ch, cost, 0);

    ship->mainsail = SHIP_MAX_SAIL(ship);

    int buildtime = 75 + total_damage;
    send_to_char_f(ch, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
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
        send_to_char("Which armor do you want to repair?\r\n", ch);
        return TRUE;
    }
    if (isname(arg, "all")) 
    {
        total_damage = 0;
        for (j = 0; j < 4; j++) 
            total_damage += ship->maxarmor[j] - ship->armor[j];

        if (total_damage == 0)
        {
            send_to_char_f(ch, "Your ship's armor is unscathed!\r\n");
            return TRUE;
        }

        cost = total_damage * 1000;
        if (GET_MONEY(ch) < cost) 
        {
            send_to_char_f(ch, "This will cost %s to repair!\r\n", coin_stringv(cost));
            return TRUE;
        }
        /* OKay, they have the plat, deduct it and do the repairs */
        SUB_MONEY(ch, cost, 0);
        for (j = 0; j < 4; j++)
            ship->armor[j] = ship->maxarmor[j];

        buildtime = 75 + total_damage;
        send_to_char_f(ch, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
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
        send_to_char ("Invalid location.\r\nValid syntax is 'repair armor <fore/rear/starboard/port>'\r\n", ch);
        return TRUE;
    }
    total_damage = ship->maxarmor[j] - ship->armor[j];
    if (total_damage == 0)
    {
        send_to_char_f(ch, "This side's armor is unscathed!\r\n");
        return TRUE;
    }
    cost = total_damage * 1000;
    buildtime = 75 + total_damage;
    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "This will cost %s to repair!\r\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->armor[j] = ship->maxarmor[j];
    send_to_char_f(ch, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
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
        send_to_char("Which internal do you want to repair?\r\n", ch);
        return TRUE;
    }
    if (isname(arg, "all")) 
    {
        total_damage = 0;
        for (j = 0; j < 4; j++)
            total_damage += ship->maxinternal[j] - ship->internal[j];

        if (total_damage == 0)
        {
            send_to_char_f(ch, "Your ship's internal structured are fine!\r\n");
            return TRUE;
        }

        cost = total_damage * 1000;
        if (GET_MONEY(ch) < cost) 
        {
            send_to_char_f(ch, "This will cost %s to repair!\r\n", coin_stringv(cost));
            return TRUE;
        }
    /* OKay, they have the plat, deduct it and do the repairs */
        SUB_MONEY(ch, cost, 0);
        for (j = 0; j < 4; j++)
            ship->internal[j] = ship->maxinternal[j];

        buildtime = 75 + total_damage;
        send_to_char_f(ch, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
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
        send_to_char ("Invalid location.\r\nValid syntax is 'repair internal <fore/rear/starboard/port>'\r\n", ch);
        return TRUE;
    }
    total_damage = ship->maxinternal[j] - ship->internal[j];
    if (total_damage == 0)
    {
        send_to_char_f(ch, "This side's internal structures are fine!\r\n");
        return TRUE;
    }
    cost = total_damage * 1000;
    buildtime = 75 + total_damage;
    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "This will cost %s to repair!\r\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->internal[j] = ship->maxinternal[j];
    send_to_char_f(ch, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
    if (!IS_TRUSTED(ch) && BUILDTIME)
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int repair_weapon(P_char ch, P_ship ship, char* arg)
{
    int cost, buildtime;

    if (!is_number(arg)) 
    {
        send_to_char("Valid syntax is 'repair weapon <weapon slot>'\r\n", ch);
        return TRUE;
    }
    int slot = atoi(arg);
    if (slot > MAXSLOTS || slot < 0) 
    {
        send_to_char ("Invalid weapon slot'\r\n", ch);
        return TRUE;
    }
    if (ship->slot[slot].type != SLOT_WEAPON) 
    {
        send_to_char ("Invalid weapon slot'\r\n", ch);
        return TRUE;
    }
    if (!SHIP_WEAPON_DAMAGED(ship, slot)) 
    {
        send_to_char("This weapon isn't broken!\r\n", ch);
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
        send_to_char_f(ch, "This will cost %s to repair!\r\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    ship->slot[slot].val2 = 0;
    send_to_char_f(ch, "Thanks for your business, it will take %d hours to complete this repair.\r\n", buildtime / 75);
    if (!IS_TRUSTED(ch) && BUILDTIME)
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int reload_ammo(P_char ch, P_ship ship, char* arg)
{
    char weapons_to_reload[MAXSLOTS];
    int cost = 0, buildtime = 0;

    if (is_number(arg)) 
    {
        int slot = atoi(arg);

        if (slot >= MAXSLOTS || slot < 0) 
        {
            send_to_char ("Invalid weapon.\r\nValid syntax is 'reload <weapon number>' or 'reload all'\r\n", ch);
            return TRUE;
        }
        if (ship->slot[slot].type != SLOT_WEAPON) 
        {
            send_to_char ("Invalid weapon.\r\nValid syntax is 'reload <weapon number> or 'reload all''\r\n", ch);
            return TRUE;
        }
        if (ship->slot[slot].val1 >= weapon_data[ship->slot[slot].index].ammo) 
        {
            send_to_char("That weapon is already fully loaded!\r\n", ch);
            return TRUE;
        }

        for (int k = 0; k < MAXSLOTS; k++)
            weapons_to_reload[k] = 0;
        weapons_to_reload[slot] = 1;
        cost = (weapon_data[ship->slot[slot].index].ammo - ship->slot[slot].val1) * 1000;
        buildtime = 75;

    } 
    else if (isname(arg, "all"))
    {
        for (int slot = 0; slot < MAXSLOTS; slot++) 
        {
            if ((ship->slot[slot].type == SLOT_WEAPON) && (ship->slot[slot].val1 < weapon_data[ship->slot[slot].index].ammo)) 
            {
                weapons_to_reload[slot] = 1;
                cost += (weapon_data[ship->slot[slot].index].ammo - ship->slot[slot].val1) * 1000;
                buildtime += 75;
            } 
            else
                weapons_to_reload[slot] = 0;
        }
    }

    if (cost == 0 || buildtime == 0) 
    {
        send_to_char("Nothing to reload here!\n\r", ch);
        return TRUE;
    }

    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "This will cost %s to reload!\r\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);

    for (int slot = 0; slot < MAXSLOTS; slot++)
        if (weapons_to_reload[slot] == 1)
            ship->slot[slot].val1 = weapon_data[ship->slot[slot].index].ammo;

    send_to_char_f(ch, "Thanks for your business, it will take %d hours to complete this reload.\r\n", buildtime / 75);
    if (!IS_TRUSTED(ch) && BUILDTIME) 
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int rename_ship(P_char ch, P_ship ship, char* new_name)
{
    if (!new_name || !*new_name)
    {
        send_to_char("Invalid syntax.\r\n", ch);
        return TRUE;
    }
    if (!check_ship_name(ship, ch, new_name))
        return TRUE;


    /* count money */
    int renamePrice = (int) (SHIPTYPE_COST(ship->m_class) / 10);

    if (GET_MONEY(ch) < renamePrice)
    {
        send_to_char_f(ch, "It will cost you %s!\r\n", coin_stringv(renamePrice));
        return TRUE;
    }
    /* count money end */

    if( !rename_ship(ch, GET_NAME(ch), new_name) == TRUE)
    {
        return TRUE;
    }
    SUB_MONEY(ch, renamePrice, 0);

    send_to_char_f(ch, "&+WCongratulations! From now on yours ship will be known as&n %s\r\n", arg2);

    wizlog(AVATAR, "%s renamed %s ship to %s\r\n", GET_NAME(ch), GET_SEX(ch) == SEX_MALE ? "his" : "her", arg2);
    logit(LOG_PLAYER, "%s renamed %s ship to %s\r\n", GET_NAME(ch), GET_SEX(ch) == SEX_MALE ? "his" : "her", arg2);

    return TRUE;
}

int buy_cargo(P_char ch, P_ship ship, char* arg)
{
    int rroom = 0;
        
    while (rroom < NUM_PORTS) 
    {
        if (ports[rroom].loc_room == world[ch->in_room].number)
            break;
        rroom++;
    }
    if (rroom >= NUM_PORTS) 
    {
        send_to_char("What cargo? We don't sell any cargo here!\r\n", ch);
        return TRUE;
    }
    if (SHIP_MAX_CARGO(ship) == 0)
    {
        send_to_char("You ship has no space for cargo.\r\n", ch);
        return TRUE;    
    }
    if (!is_number(arg)) 
    {
        send_to_char("Valid syntax is 'buy cargo <number of crates>'\r\n", ch);
        return TRUE;
    }
    int asked_for = atoi(arg);
    if (asked_for < 1) 
    {
        send_to_char("Please enter a valid number of crates to buy.\r\n", ch);
        return TRUE;
    }

    if (SHIP_AVAIL_CARGO_LOAD(ship) <= 0)
    {
        send_to_char("You don't have any space left on your ship!\r\n", ch);
        return TRUE;
    }
    else if (asked_for > SHIP_AVAIL_CARGO_LOAD(ship))
    {
        send_to_char_f(ch, "You only have space for %d more crates!\r\n", SHIP_AVAIL_CARGO_LOAD(ship));
        return TRUE;
    }

    int slot;
    for (slot = 0 ; slot < MAXSLOTS; ++slot) 
    {
        if (ship->slot[slot].type == SLOT_CARGO && ship->slot[slot].index == rroom)
            break;
    }
    if (slot == MAXSLOTS)
    {
        for (slot = 0 ; slot < MAXSLOTS; ++slot) 
        {
            if (ship->slot[slot].type == SLOT_EMPTY) 
                break;
        }
    }
    if (slot == MAXSLOTS) 
    {
        send_to_char("You do not have a free slot to fit this cargo into!\r\n", ch);
        return TRUE;
    }

    int unit_cost = cargo_sell_price(rroom);
    int cost = asked_for * unit_cost;

    // hook for epic bonus
    cost -= (int) (cost * get_epic_bonus(ch, EPIC_BONUS_CARGO));

    //if (GET_LEVEL(ch) < 50)
    //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);

    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "This cargo load costs %s&n!\r\n", coin_stringv(cost));
        return TRUE;
    }

    int placed = asked_for;
    if (ship->slot[slot].type == SLOT_CARGO)
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
  
    send_to_char_f(ch, "You buy &+W%d&n crates of %s&n for %s.\r\n", placed, cargo_type_name(rroom), coin_stringv(cost) );

    SUB_MONEY(ch, cost, 0);

    // economy affect
    adjust_ship_market(BOUGHT_CARGO, rroom, rroom, placed);

    send_to_char ("Thank you for your purchase, your cargo is loaded and set to go!\r\n", ch);

    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int buy_contra(P_char ch, P_ship ship, char* arg)
{
    int rroom = 0;
    while (rroom < NUM_PORTS) 
    {
        if (ports[rroom].loc_room == world[ch->in_room].number) 
            break;
        rroom++;
    }
    if (rroom >= NUM_PORTS) 
    {
        send_to_char("What contraband?  We don't sell any contraband!\r\n", ch);
        return TRUE;
    }
    if (!IS_TRUSTED(ch) && GET_ALIGNMENT(ch) > MINCONTRAALIGN) 
    {
        send_to_char ("Goodie goodie two shoes like you shouldn't think of contraband.\r\n", ch);
        return TRUE;
    }
    if (!IS_TRUSTED(ch) && !can_buy_contraband(ship, rroom)) 
    {
        send_to_char ("Contraband?! *gasp* how could you even think that I would sell such things!\r\n", ch);
        return TRUE;
    }
    if (SHIP_MAX_CONTRA(ship) == 0)
    {
        send_to_char("You ship has no places to hide contraband.\r\n", ch);
        return TRUE;
    }
    if (!is_number(arg)) 
    {
        send_to_char("Invalid number of crates!  Syntax is 'buy contraband <number of crates>'\r\n", ch);
        return TRUE;
    }

    int asked_for = atoi(arg);
    if (asked_for < 1) 
    {
        send_to_char("Please enter a valid number of crates to buy.\r\n", ch);
        return TRUE;
    }

    if (SHIP_AVAIL_CARGO_LOAD(ship) <= 0)
    {
        send_to_char("You don't have any space left on your ship!\r\n", ch);
        return TRUE;
    }
    // max contraband amount reduced proportionally to total available cargo load
    int contra_max = int ((float)SHIP_MAX_CONTRA(ship) * ((float)SHIP_MAX_CARGO_LOAD(ship) / (float)SHIP_MAX_CARGO(ship)));
    int contra_avail = contra_max - SHIP_CONTRA(ship);
    if (contra_avail <= 0)
    {
        send_to_char("You don't have any space left on your ship to hide contraband!\r\n", ch);
        return TRUE;
    }
    int max_avail = MIN(SHIP_AVAIL_CARGO_LOAD(ship), contra_avail);
    if (asked_for > max_avail)
    {
        send_to_char_f(ch, "You only have space for %d more crates of contraband to hide!\r\n", max_avail);
        return TRUE;
    }
    
    int slot;
    for (slot = 0 ; slot < MAXSLOTS; ++slot) 
    {
        if (ship->slot[slot].type == SLOT_CONTRABAND && ship->slot[slot].index == rroom)
            break;
    }
    if (slot == MAXSLOTS)
    {
        for (slot = 0 ; slot < MAXSLOTS; ++slot) 
        {
            if (ship->slot[slot].type == SLOT_EMPTY) 
                break;
        }
    }
    if (slot == MAXSLOTS) 
    {
        send_to_char("You do not have a free slot to fit this contraband into!\r\n", ch);
        return TRUE;
    }
  
    int unit_cost = contra_sell_price(rroom);

    int cost = asked_for * unit_cost;

    //if (GET_LEVEL(ch) < 50) 
    //    cost = (int) (cost * (float) GET_LEVEL(ch) / 50.0);

    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "This contraband costs %s!\r\n", coin_stringv(cost));
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
  
    send_to_char_f(ch, "You buy &+W%d&n crates of %s&n for %s.\r\n", placed, contra_type_name(rroom), coin_stringv(cost) );

    SUB_MONEY(ch, cost, 0);

    // economy affect
    adjust_ship_market(BOUGHT_CONTRA, rroom, rroom, placed);

    send_to_char ("Thank you for your 'purchase' *snicker*, your 'cargo' is loaded and set to go!\r\n", ch);

    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int buy_weapon(P_char ch, P_ship ship, char* arg1, char* arg2)
{
  struct affected_type *paf = get_spell_from_char(ch, ACH_CARGOCOUNT);
  bool quickbuild = (paf && paf->modifier >= 10000) ? TRUE : FALSE;

    if (!is_number(arg1) || !arg2 || !*arg2) 
    {
        send_to_char ("Valid syntax is 'buy weapon <number> <fore/rear/port/starboard>'\r\n", ch);
        return TRUE;
    }
    
    int w = atoi(arg1) - 1;
    if ((w < 0) || (w >= MAXWEAPON)) 
    {
        send_to_char ("Invalid weapon number.  Valid syntax is 'buy weapon <number> <fore/rear/starboard/port>'\r\n", ch);
        return TRUE;
    }

    int arc;
    if (isname(arg2, "fore") || isname(arg2, "f"))
        arc = SIDE_FORE;
    else if (isname(arg2, "rear") || isname(arg2, "r"))
        arc = SIDE_REAR;
    else if (isname(arg2, "port") || isname(arg2, "p"))
        arc = SIDE_PORT;
    else if (isname(arg2, "starboard") || isname(arg2, "s"))
        arc = SIDE_STAR;
    else
    {
        send_to_char ("Invalid weapon placement. Valid syntax is 'buy weapon <number> <fore/rear/starboard/port>'\r\n", ch);
        return TRUE;
    }

    if (weapon_data[w].weight > SHIP_AVAIL_WEIGHT(ship) ) 
    {
        send_to_char_f(ch, "That weapon weighs %d! Your ship can only support %d!\r\n", weapon_data[w].weight, SHIP_MAX_WEIGHT(ship));
        return TRUE;
    }
    if (!ship_allowed_weapons[ship->m_class][w])
    {
        send_to_char_f(ch, "Such weapon can not be installed on this hull!\r\n");
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
        send_to_char("You do not have a free slot to intall this weapon!\r\n", ch);
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
        send_to_char_f(ch, "Your can not put more weapons to this side!\r\n");
        return TRUE;
    }
    if (same_arc_weight + weapon_data[w].weight > ship_arc_properties[ship->m_class].max_weapon_weight[arc])
    {
        send_to_char_f(ch, "Your can not put more weight to this side! Maximum allowed: %d\r\n", ship_arc_properties[ship->m_class].max_weapon_weight[arc]);
        return TRUE;
    }
    
    if (!IS_SET(weapon_data[w].flags, arcbitmap[arc])) 
    {
        send_to_char ("That weapon cannot be placed in that position, try another one.\r\n", ch);
        return TRUE;
    }

/* -Removing frag requirements for weapons - Drannak 6/3/2013
 *  Readding them. - Lohrr 7/23/2014
 *  Also added a min_crewexp value for each weapon and compared it to guns_skill for exp.
*/
    if( ship->frags < weapon_data[w].min_frags
      && ship->crew.guns_skill < weapon_data[w].min_crewexp )
    {
        send_to_char ("I'm sorry, but not just anyone can buy this weapon!  You must earn it!\r\n", ch);
        return TRUE;
    }

    if (IS_SET(weapon_data[w].flags, CAPITAL)) 
    {
        for (int j = 0; j < MAXSLOTS; j++) 
        {
            if (((ship->slot[j].type == SLOT_WEAPON) && IS_SET(weapon_data[ship->slot[j].index].flags, CAPITAL)) ||
                ((ship->slot[j].type == SLOT_EQUIPMENT) && IS_SET(equipment_data[ship->slot[j].index].flags, CAPITAL))) 
            {
                send_to_char ("You already have a capital equipment! You can only have one.\r\n", ch);
                return TRUE;
            }
        }
    }

    int cost = weapon_data[w].cost;
    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "This weapon costs %s!\r\n", coin_stringv(cost));
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
  //int pvp = false;
	//pvp = ocean_pvp_state();
  //if (pvp) buildtime *= 4;
    send_to_char_f(ch, "Thank you for your purchase, it will take %d hours to install the part.\r\n", (int) (buildtime / 75));
    if (!IS_TRUSTED(ch) && BUILDTIME)
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}

int buy_equipment(P_char ch, P_ship ship, char* arg1)
{
  struct affected_type *paf = get_spell_from_char(ch, ACH_CARGOCOUNT);
  bool quickbuild = (paf && paf->modifier >= 10000) ? TRUE : FALSE;

    if (!is_number(arg1)) 
    {
        send_to_char ("Valid syntax is 'buy equipment <number>'\r\n", ch);
        return TRUE;
    }
    
    int e = atoi(arg1) - 1;
    if ((e < 0) || (e >= MAXEQUIPMENT)) 
    {
        send_to_char ("Invalid equipment number.  Valid syntax is 'buy equipment <number>'\r\n", ch);
        return TRUE;
    }

    if (equipment_data[e].weight > SHIP_AVAIL_WEIGHT(ship) ) 
    {
        send_to_char_f(ch, "That equipment weighs %d! Your ship can only support %d!\r\n", equipment_data[e].weight, SHIP_MAX_WEIGHT(ship));
        return TRUE;
    }
    if (!ship_allowed_equipment[ship->m_class][e])
    {
        send_to_char_f(ch, "Such equipment can not be installed on this hull!\r\n");
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
        send_to_char("You do not have a free slot to intall this equipment!\r\n", ch);
        return TRUE;
    }

    if( ship->frags < equipment_data[e].min_frags )
    {
        send_to_char ("I'm sorry, but not just anyone can buy this equipment!  You must earn it!\r\n", ch);
        return TRUE;
    }

    if (IS_SET(equipment_data[e].flags, CAPITAL)) 
    {
        for (int j = 0; j < MAXSLOTS; j++) 
        {
            if (((ship->slot[j].type == SLOT_WEAPON) && IS_SET(weapon_data[ship->slot[j].index].flags, CAPITAL)) ||
                ((ship->slot[j].type == SLOT_EQUIPMENT) && IS_SET(equipment_data[ship->slot[j].index].flags, CAPITAL))) 
            {
                send_to_char ("You already have a capital equipment! You can only have one.\r\n", ch);
                return TRUE;
            }
        }
    }

    int cost = equipment_data[e].cost;
    if (e == E_RAM) cost = eq_ram_cost(ship);

    if (GET_MONEY(ch) < cost) 
    {
        send_to_char_f(ch, "This equipment costs %s!\r\n", coin_stringv(cost));
        return TRUE;
    }

    SUB_MONEY(ch, cost, 0);
    set_equipment(ship, slot, e);
    
/* Unused variable weight. - Lohrr
    int weight = equipment_data[e].weight;
    if (e == E_RAM) weight = eq_ram_weight(ship);
    if (e == E_LEVISTONE) weight = eq_levistone_weight(ship);
*/
    int buildtime = equipment_data[e].weight * 75;
    int pvp = false;
    pvp = ocean_pvp_state();
        if (pvp) buildtime *= 4;
    // Achievement - Trader
    if( quickbuild )
    {
      buildtime /= 2;
    }
    send_to_char_f(ch, "Thank you for your purchase, it will take %d hours to install the part.\r\n", (int) (buildtime / 75));
    if (!IS_TRUSTED(ch) && BUILDTIME)
        ship->timer[T_MAINTENANCE] += buildtime;
    update_ship_status(ship);
    write_ship(ship);
    return TRUE;
}


int buy_hull(P_char ch, P_ship ship, int owned, char* arg1, char* arg2)
{
  int cost, buildtime;
  struct affected_type *paf = get_spell_from_char(ch, ACH_CARGOCOUNT);
  bool quickbuild = (paf && paf->modifier >= 10000) ? TRUE : FALSE;

  if( owned )
  {
    if( !is_number(arg1) )
    {
      send_to_char("To upgrade or downgrade a hull type the follow: 'buy hull <number>.'\r\n", ch);
      return TRUE;
    }
  }
  else
  {
    if( !is_number(arg1) || !arg2 || !*arg2 )
    {
      send_to_char("To buy a hull type the follow: 'buy hull <number> <name>.'\r\n", ch);
      return TRUE;
    }
  }

  int i = atoi(arg1) - 1;
  if( (i < 0) || (i >= MAXSHIPCLASS) )
  {
    send_to_char ("Not a valid hull selection.\r\n", ch);
    return TRUE;
  }

  if( SHIPTYPE_MIN_LEVEL(i) > GET_LEVEL(ch) )
  {
    send_to_char ("You are too low for such a big ship! Get more experience!\r\n", ch);
    return TRUE;
  }

  if( SHIPTYPE_COST(i) == 0 && !IS_TRUSTED(ch) )
  {
    send_to_char ("You can not buy this hull.\r\n", ch);
    return TRUE;
  }

  /* Okay, we have a valid selection for ship hull */
  if( owned )
  {
    int oldclass = ship->m_class;
    if( i == oldclass )
    {
      send_to_char ("You own this hull already!\r\n", ch);
      return TRUE;
    }
    if( SHIP_CONTRA(ship) + SHIP_CARGO(ship) > 0 )
    {
      send_to_char ("You can not reconstruct ship full of cargo!\r\n", ch);
      return TRUE;
    }
    if( !check_undocking_conditions (ship, i, ch) )
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

    cost = SHIPTYPE_COST(i) - (int) (SHIPTYPE_COST(oldclass) * .90);
    if( cost >= 0 )
    {
      if( GET_MONEY(ch) < cost || GET_EPIC_POINTS(ch) < SHIPTYPE_EPIC_COST(i) )
      {
        if( SHIPTYPE_EPIC_COST(i) > 0 )
        {
          send_to_char_f(ch, "That upgrade costs %s and %d epic points!\r\n", coin_stringv(cost), SHIPTYPE_EPIC_COST(i));
        }
        else
        {
          send_to_char_f(ch, "That upgrade costs %s!\r\n", coin_stringv(cost));
        }
        return TRUE;
      }
      SUB_MONEY(ch, cost, 0);/* OKay, they have the plat, deduct it and build the ship */
      if(SHIPTYPE_EPIC_COST(i) > 0 )
      {
        ch->only.pc->epics -= SHIPTYPE_EPIC_COST(i);
      }
    }
    else
    {
      if( GET_EPIC_POINTS(ch) < SHIPTYPE_EPIC_COST(i) )
      {
        send_to_char_f(ch, "That upgrade costs %d epic points!\r\n", SHIPTYPE_EPIC_COST(i));
        return TRUE;
      }
      cost *= -1;
      send_to_char_f(ch, "You receive %s&n for remaining materials.\r\n", coin_stringv(cost));
      ADD_MONEY(ch, cost);
      if( SHIPTYPE_EPIC_COST(i) > 0 )
      {
        ch->only.pc->epics -= SHIPTYPE_EPIC_COST(i);
      }
    }

    ship->m_class = i;
    reset_ship(ship, false);

    if( ship->m_class > oldclass )
    {
      buildtime = 75 * (ship->m_class / 2 - oldclass / 3);
    }
    else
    {
      buildtime = 75 * (oldclass / 2 - ship->m_class / 3);
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
    send_to_char_f(ch, "Thanks for your business, it will take %d hours to complete this upgrade.\r\n", buildtime / 75);
  }
  else
  {
    if( (int)strlen(strip_ansi(arg2).c_str()) <= 0 )
    {
      send_to_char ("No name selected. Valid syntax is 'buy <number> <name>'\r\n", ch); 
      return TRUE;
    }
    if( !is_valid_ansi_with_msg(ch, arg2, FALSE) )
    {
      send_to_char ("Invalid ANSI name'\r\n", ch); 
      return TRUE;
    }
    if( sub_string_set(strip_ansi(arg2).c_str(), rude_ass) )
    {
      send_to_char("Name must not contain rude terms.\r\n", ch);
      return TRUE;
    }
    if( (int) strlen(strip_ansi(arg2).c_str()) > 20 )
    {
      send_to_char("Name must be less than 20 characters (not including ansi))\r\n", ch);
      return TRUE;
    }
    if( !check_ship_name(0, ch, arg2) )
    {
      return TRUE;
    }
    if( affected_by_spell( ch, AIP_FREESLOOP ) )
    {
      if( (GET_MONEY(ch) >= SHIPTYPE_COST(i) - 100000) && GET_EPIC_POINTS(ch) >= SHIPTYPE_EPIC_COST(i) )
      {
        send_to_char( "You show your &+ysmall &+bS&+Ba&+bi&+Bl&+bo&+Br&+b'&+Bs&n &+yTattoo&n for a discount.&n\n\r", ch );
        ADD_MONEY(ch, 100000);
        // Remove the flag for free sloop.
        affect_from_char( ch, AIP_FREESLOOP );
      }
    }

    if( GET_MONEY(ch) < SHIPTYPE_COST(i) || GET_EPIC_POINTS(ch) < SHIPTYPE_EPIC_COST(i) )
    {
      if( SHIPTYPE_EPIC_COST(i) > 0 )
      {
        send_to_char_f(ch, "That ship costs %s and %d epic points!\r\n", coin_stringv(SHIPTYPE_COST(i)), SHIPTYPE_EPIC_COST(i));
      }
      else
      {
        send_to_char_f(ch, "That ship costs %s!\r\n", coin_stringv(SHIPTYPE_COST(i)));
      }
      return TRUE;
    }

    // Now, create the ship object
    ship = new_ship(i);
    if( ship == NULL )
    {
      logit(LOG_FILE, "error in new_ship(): %d\n", shiperror);
      send_to_char_f(ch, "Error creating new ship (%d), please notify a god.\r\n", shiperror);
      return TRUE;
    }

    buildtime = 75 * SHIPTYPE_ID(i) / 4;
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
      logit(LOG_FILE, "error in load_ship(): %d\n", shiperror);
      send_to_char_f(ch, "Error loading ship (%d), please notify a god.\r\n", shiperror);
      return TRUE;
    }
    write_ships_index();

    // everything went successfully, substracting the cost
    SUB_MONEY(ch, SHIPTYPE_COST(i), 0);
    if (SHIPTYPE_EPIC_COST(i) > 0)
    {
      epic_gain_skillpoints(ch, -SHIPTYPE_EPIC_COST(i));
    }

    send_to_char( "Your ship, '", ch );
    send_to_char( strip_ansi(arg2).c_str(), ch );
    send_to_char( "', will be ", ch );
    send_to_char( arg2, ch );
    send_to_char( " with paint.\n", ch );

    send_to_char_f(ch, "Thanks for your business, this hull will take %d hours to build.\r\n", buildtime/75);
  }

  if( !IS_TRUSTED(ch) && BUILDTIME )
  {
    ship->timer[T_MAINTENANCE] += buildtime;
  }
  update_ship_status(ship);
  write_ship(ship);
  return TRUE;
}

int swap_slots(P_char ch, P_ship ship, char* arg1, char* arg2)
{
    if (!*arg1 || !is_number(arg1) || !*arg2 || !is_number(arg2)) 
    {
        send_to_char("Valid Syntax: buy swap <slot1> <slot2>\r\n", ch);
        return TRUE;
    }
    int slot1 = atoi(arg1);
    int slot2 = atoi(arg2);
    if ((slot1 >= MAXSLOTS) || (slot1 < 0) || (slot2 >= MAXSLOTS) || (slot2 < 0)) 
    {
        send_to_char("Invalid slot number!\r\n", ch);
        return TRUE;
    }
    if (slot1 == slot2)
    {
        send_to_char("Switch slot with itself?! What for?\r\n", ch);
        return TRUE;
    }
    ShipSlot temp;
    temp.clone(ship->slot[slot1]);
    ship->slot[slot1].clone(ship->slot[slot2]);
    ship->slot[slot2].clone(temp);
    send_to_char("Done! Thank you for your business!\r\n", ch);
    write_ship(ship);
    return TRUE;
}

int ship_shop_proc(int room, P_char ch, int cmd, char *arg)
{
    if (!ch)
        return FALSE;

    if ((cmd != CMD_SUMMON) && (cmd != CMD_LIST) && (cmd != CMD_BUY) &&
        (cmd != CMD_RELOAD) && (cmd != CMD_REPAIR) && (cmd != CMD_SELL))
        return FALSE;

    half_chop(arg, arg1, arg2);

    /* hack to allow ferry ticket automats in room with ship store */
    if ( cmd == CMD_BUY && isname(arg1, "ticket") )
        return FALSE;

    if ( cmd == CMD_SUMMON && !isname(arg1, "ship")  && !isname(arg1, "time"))
        return FALSE;

    int  owned = 0;
    P_ship ship = NULL;
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (isname(svs->ownername, GET_NAME(ch))) 
        {
            owned = 1;
            ship = svs;
            break;
        }
    }

    if (owned && (ship->location != ch->in_room)) 
    {
        if ((cmd != CMD_SUMMON) && (cmd != CMD_LIST) && (cmd != CMD_SELL))
        {
            int summon_cost = SHIPTYPE_HULL_WEIGHT(ship->m_class) * 100;
            send_to_char("Your ship needs to be here to receive service.\r\n", ch);
            send_to_char_f(ch, "For a small fee of %s, &nI can have my men tell your crew to sail here.\r\n", coin_stringv(summon_cost));
            send_to_char("Just type 'summon ship' to have your ship sail here.\r\n", ch);
            return TRUE;
        }
    }

    if (owned && (!SHIP_DOCKED(ship) || ship->timer[T_UNDOCK] != 0)) 
    {
        if ((cmd != CMD_SUMMON) && (cmd != CMD_LIST) && (cmd != CMD_SELL))
        {
            send_to_char_f(ch,"Dock your ship to recieve a maintenance!\r\n");
           return TRUE;
        }
    }

    if (owned && (cmd == CMD_SELL || cmd == CMD_SUMMON) && ship->timer[T_MAINTENANCE] > 0)
    {
        send_to_char_f(ch, "Your ship is currently being worked on, please wait for another %.1f hours.\r\n", (float) ship->timer[T_MAINTENANCE] / 75.0);
        return TRUE;
    }

    if (cmd == CMD_LIST) 
    {
        if (*arg1) 
        {
            if (isname(arg1, "cargo")) 
            {
                return list_cargo(ch, ship, owned);
            }
            else if (isname(arg1, "weapon") || isname(arg1, "weapons") || isname(arg1, "w")) 
            {
                return list_weapons(ch, ship, owned);
            }
            else if (isname(arg1, "equipment") || isname(arg1, "e")) 
            {
                return list_equipment(ch, ship, owned);
            }
            else if (isname(arg1, "hull") || isname(arg1, "hulls") || isname(arg1, "h")) 
            {
                return list_hulls(ch, ship, owned);
            }
        }
        send_to_char("Valid syntax is '&+glist <&+Gh&+gull/&+Gw&+geapon/&+Ge&+gquipment/cargo>'\r\n", ch);
        return TRUE;
    }
    if (cmd == CMD_SUMMON) 
    {
        if (!owned) 
        {
            send_to_char("You do not own a ship!\r\n", ch);
            return TRUE;
        }
        if (isname(arg1, "ship"))
            return summon_ship(ch, ship, false);
        if (isname(arg1, "time"))
            return summon_ship(ch, ship, true);
    }

    if (cmd == CMD_SELL) 
    {
        if (!owned) 
        {
            send_to_char("You have no ship to sell something!\r\n", ch);
            return TRUE;
        }
        if (ship->location != ch->in_room && ship->timer[T_BSTATION] != 0) 
        {
            send_to_char("Your ship is currently pre-occupied in combat!\r\n", ch);
            return TRUE;
        }
        if (*arg1) 
        {
            if (isname(arg1, "ship")) 
            {
                return sell_ship(ch, ship, arg2);
            }
            else 
            {
                if (!SHIP_DOCKED(ship) || ship->timer[T_UNDOCK] != 0)
                {
                    send_to_char_f(ch,"Dock your ship first!\r\n");
                    return TRUE;
                }
                if (isname(arg1, "cargo")) 
                {
                    return sell_cargo(ch, ship, -1);
                } 
                else if (isname(arg1, "contraband")) 
                {
                    return sell_contra(ch, ship, -1);
                } 
                else if (is_number(arg1)) 
                {
                    return sell_slot(ch, ship, atoi(arg1));
                }
            }
        }
        send_to_char("Valid syntax is 'sell <cargo/contraband/[slot]>'\r\n", ch);
        return TRUE;
    }

    if (cmd == CMD_REPAIR) 
    {
        if (!owned) 
        {
            send_to_char("You have no ship to repair!\r\n", ch);
            return TRUE;
        }
        if (*arg1) 
        {
            if (isname(arg1, "all")) 
            {
                return repair_all (ch, ship);
            } 
            if (isname(arg1, "sail") || isname(arg1, "s")) 
            {
                return repair_sail (ch, ship);
            } 
            else if (isname(arg1, "armor") || isname(arg1, "a")) 
            {
                return repair_armor(ch, ship, arg2);
            } 
            else if (isname(arg1, "internal") || isname(arg1, "i")) 
            {
                return repair_internal(ch, ship, arg2);
            } 
            else if (isname(arg1, "weapon") || isname(arg1, "w")) 
            {
                return repair_weapon(ch, ship, arg2);
            } 
        }
        send_to_char("Valid syntax is 'repair <armor/internal/sail/weapon/all>'\r\n", ch);
        return TRUE;
    }
    
    if (cmd == CMD_RELOAD) 
    {
        if (!owned) 
        {
            send_to_char("You have no ship to reload!\r\n", ch);
            return TRUE;
        }
        if (*arg1) 
        {
            if (is_number(arg1) || isname(arg1, "all"))
            {
                return reload_ammo(ch, ship, arg1);
            }
        }
        send_to_char ("Valid syntax is 'reload <[slot]/all>'\r\n", ch);
        return TRUE;

    }

    if (cmd == CMD_BUY) 
    {
        if (isname(arg1, "rename"))
        {
            if (!owned) 
            {
                send_to_char("You have no ship to rename!\r\n", ch);
                return TRUE;
            }
            return rename_ship(ch, ship, arg2);
        }
        else if (isname(arg1, "swap"))
        {
            if (!owned) 
            {
                send_to_char("You have no ship to swap slots!\r\n", ch);
                return TRUE;
            }
            half_chop(arg2, arg1, arg3);
            return swap_slots(ch, ship, arg1, arg3);
        }
        else if (isname(arg1, "cargo")) 
        {
            if (!owned) 
            {
                send_to_char("You have no ship to load!\r\n", ch);
                return TRUE;
            }
            if (ship->timer[T_MAINTENANCE] > 0)
            {
                send_to_char_f(ch, "Your ship is currently being worked on, please wait for another %.1f hours.\r\n", (float) ship->timer[T_MAINTENANCE] / 75.0);
                return TRUE;
            }
            return buy_cargo(ch, ship, arg2);
        } 
        else if (isname(arg1, "contraband")) 
        {
            if (!owned) 
            {
                send_to_char("You have no ship to load!\r\n", ch);
                return TRUE;
            }
            if (ship->timer[T_MAINTENANCE] > 0)
            {
                send_to_char_f(ch, "Your ship is currently being worked on, please wait for another %.1f hours.\r\n", (float) ship->timer[T_MAINTENANCE] / 75.0);
                return TRUE;
            }
            return buy_contra(ch, ship, arg2);
        } 
        else if (isname(arg1, "weapon") || isname(arg1, "w")) 
        {
            if (!owned) 
            {
                send_to_char("You have no ship to equip!\r\n", ch);
                return TRUE;
            }
            half_chop(arg2, arg1, arg3);
            return buy_weapon(ch, ship, arg1, arg3);
        }
        else if (isname(arg1, "equipment") || isname(arg1, "e")) 
        {
            if (!owned) 
            {
                send_to_char("You have no ship to equip!\r\n", ch);
                return TRUE;
            }
            return buy_equipment(ch, ship, arg2);
        }
        else if (isname(arg1, "hull") || isname(arg1, "h")) 
        {
            if (owned && ship->timer[T_MAINTENANCE] > 0)
            {
                send_to_char_f(ch, "Your ship is currently being worked on, please wait for another %.1f hours.\r\n", (float) ship->timer[T_MAINTENANCE] / 75.0);
                return TRUE;
            }
            half_chop(arg2, arg1, arg3);
            return buy_hull(ch, ship, owned, arg1, arg3);
        } 
        else
        {
            send_to_char ("Valid syntax: '&+gbuy &+Gh&+gull/cargo/contraband/&+Gw&+geapon/&+Ge&+gquipment/rename/swap>'.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char ("ship_shop deadend reached, report a bug!\r\n", ch);
    return FALSE;
};


const int good_crew_shops[] = { 43220, 43222, 132766, 28197 };
const int evil_crew_shops[] = { 43221,  9704,  22481, 22648 };

int look_crew (P_char ch, P_ship ship);
int crew_shop_proc(int room, P_char ch, int cmd, char *arg)
{
    if (!ch)
        return FALSE;
 
    if ((cmd != CMD_LIST) && (cmd != CMD_HIRE))
        return FALSE;

    for (; isspace(*arg); arg++);

    if (RACE_EVIL(ch) && !IS_TRUSTED(ch))
    {
        for (unsigned i = 0; i < sizeof(good_crew_shops) / sizeof(int); i++)
        {
            if (good_crew_shops[i] == world[room].number)
            {
                send_to_char("Noone here wants to talk to such suspicious character.\r\n", ch);
                return TRUE;
            }
        }
    }
    if (RACE_GOOD(ch) && !IS_TRUSTED(ch))
    {
        for (unsigned i = 0; i < sizeof(evil_crew_shops) / sizeof(int); i++)
        {
            if (evil_crew_shops[i] == world[room].number)
            {
                send_to_char("Noone here wants to talk to such suspicious character.\r\n", ch);
                return TRUE;
            }
        }
    }

    P_ship ship = 0;
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
        if (isname(GET_NAME(ch), svs->ownername))
        {
            ship = svs;
            break;
        }
    }

    if (!ship)
    {
        send_to_char("You own no ship, no one here wants to talk to you.\r\n", ch);
        return TRUE;
    }

    if (cmd == CMD_LIST)
    {
        send_to_char("&+YYou ask around and find these people looking for employer:&N\r\n", ch);
        int n = 0;

        send_to_char("\r\n", ch);
        send_to_char("                                             Base Skills      Base    Skill modifiers    Frags       Hiring                 \r\n", ch);
        send_to_char("-=========== Crews ===========-    Level  Deck/Guns/Repair  stamina  Deck  Guns Repair  to hire       cost          Specials\r\n\r\n", ch);
        for (int i = 0; i < MAXCREWS; i++)
        {
            if (ship_crew_data[i].hire_room(world[room].number))
            {
                n++;
                send_to_char_f(ch, "&+Y%2d)&N ", n);

                char name_format[20];
                sprintf(name_format, "%%-%ds&N", strlen(ship_crew_data[i].name) + (30 - strlen(strip_ansi(ship_crew_data[i].name).c_str())));
                send_to_char_f(ch, name_format, ship_crew_data[i].name);
                send_to_char_f(ch, "   &+C%1d   ", ship_crew_data[i].level + 1);

                send_to_char_f(ch, " &+W%4d&N/&+W%4d&N/&+W%4d&N", ship_crew_data[i].base_sail_skill, ship_crew_data[i].base_guns_skill, ship_crew_data[i].base_rpar_skill);
                send_to_char_f(ch, "      &+G%4d", ship_crew_data[i].base_stamina);

                if (ship_crew_data[i].sail_mod != 0)
                {
                    char sail_mod[10];
                    sprintf(sail_mod, "%s%d", (ship_crew_data[i].sail_mod > 0) ? "+" : "", ship_crew_data[i].sail_mod);
                    send_to_char_f(ch, "   &+W%3s", sail_mod);
                }
                else
                    send_to_char_f(ch, "      ");

                if (ship_crew_data[i].guns_mod != 0)
                {
                    char guns_mod[10];
                    sprintf(guns_mod, "%s%d", (ship_crew_data[i].guns_mod > 0) ? "+" : "", ship_crew_data[i].guns_mod);
                    send_to_char_f(ch, "   &+W%3s", guns_mod);
                }
                else
                    send_to_char_f(ch, "      ");

                if (ship_crew_data[i].rpar_mod != 0)
                {
                    char rpar_mod[10];
                    sprintf(rpar_mod, "%s%d", (ship_crew_data[i].rpar_mod > 0) ? "+" : "", ship_crew_data[i].rpar_mod);
                    send_to_char_f(ch, "   &+W%3s", rpar_mod);
                }
                else
                    send_to_char_f(ch, "      ");

                send_to_char_f(ch, "      &+r%4d", ship_crew_data[i].hire_frags);
                if (ship_crew_data[i].hire_cost)
                    send_to_char_f(ch, "   &+W%20s    ", coin_stringv(ship_crew_data[i].hire_cost));
                else
                    send_to_char_f(ch, "                      ");

                int bonus_no = -1;
                do {
                    const char* bonus_str = ship_crew_data[i].get_next_bonus(bonus_no);
                    if (bonus_str)
                        send_to_char_f(ch, "&+M%s  ", bonus_str);
                }
                while(bonus_no != -1);

                send_to_char("\r\n", ch);
            }
        }

        send_to_char("\r\n\r\n", ch);
        send_to_char("                                                 Minimum  Skill   Skill     Frags        Hiring\r\n", ch);
        send_to_char("-======= Chief members =======-     Speciality    skill    gain  modifier  to hire        cost \r\n\r\n", ch);

        for (int i = 0; i < MAXCHIEFS; i++)
        {
            if (ship_chief_data[i].hire_room(world[room].number))
            {
                n++;
                send_to_char_f(ch, "&+Y%2d)&N ", n);

                char name_format[20];
                sprintf(name_format, "%%-%ds&N", strlen(ship_chief_data[i].name) + (30 - strlen(strip_ansi(ship_chief_data[i].name).c_str())));
                send_to_char_f(ch, name_format, ship_chief_data[i].name);

                send_to_char_f(ch, "   %-11s", ship_chief_data[i].get_spec());
                send_to_char_f(ch, "  &+W%5d", ship_chief_data[i].min_skill);

                if (ship_chief_data[i].skill_gain_bonus != 0)
                {
                    char skill_gain[10];
                    sprintf(skill_gain, "%s%d%%", (ship_chief_data[i].skill_gain_bonus > 0) ? "+" : "", ship_chief_data[i].skill_gain_bonus);
                    send_to_char_f(ch, "  &+W%6s", skill_gain);
                }
                else
                    send_to_char_f(ch, "        ");

                if (ship_chief_data[i].skill_mod != 0)
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

    if (cmd == CMD_HIRE)
    {
        if (!is_number(arg))
        {
            send_to_char("Valid syntax 'hire <number>'\r\n", ch);
            return TRUE;
        }

        int n = atoi(arg);
        int c = 0;
        for (int i = 0; i < MAXCREWS; i++)
        {
            if (ship_crew_data[i].hire_room(world[room].number) && ++c == n) 
            {
                if (i == ship->crew.index)
                {
                    send_to_char("&+wYou already have this crew!\r\n", ch);
                    return TRUE;
                }
                if (ship->frags < ship_crew_data[i].hire_frags && 
                    (ship->crew.sail_skill < ship_crew_data[i].base_sail_skill ||
                     ship->crew.guns_skill < ship_crew_data[i].base_guns_skill ||
                     ship->crew.rpar_skill < ship_crew_data[i].base_rpar_skill ))
                {
                    send_to_char("&+wNever heard of you and y'er crew looks weak, get lost!\r\n", ch);
                    return TRUE;
                }
                int cost = ship_crew_data[i].hire_cost;
                if (cost == 0 && !IS_TRUSTED(ch))
                {
                    send_to_char_f(ch, "You can't hire this crew!\r\n");
                    return TRUE;
                }
                if (GET_MONEY(ch) < cost)
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
        for (int i = 0; i < MAXCHIEFS; i++)
        {
            if (ship_chief_data[i].hire_room(world[room].number) && ++c == n) 
            {
                if (i == ship->crew.sail_chief || i == ship->crew.guns_chief || i == ship->crew.rpar_chief)
                {
                    send_to_char("&+wYou already have this chief!\r\n", ch);
                    return TRUE;
                }
                if (ship->frags < ship_chief_data[i].hire_frags && 
                    ((ship_chief_data[i].type == SAIL_CHIEF && ship->crew.sail_skill < ship_chief_data[i].min_skill) ||
                     (ship_chief_data[i].type == GUNS_CHIEF && ship->crew.guns_skill < ship_chief_data[i].min_skill) ||
                     (ship_chief_data[i].type == RPAR_CHIEF && ship->crew.rpar_skill < ship_chief_data[i].min_skill) ))
                {
                    send_to_char("&+wI won't work with these greenhorns unless their captain is someone famous.\r\n", ch);
                    return TRUE;
                }
                int cost = ship_chief_data[i].hire_cost;
                if (cost == 0 && !IS_TRUSTED(ch))
                {
                    send_to_char_f(ch, "You can't hire this guy!\r\n");
                    return TRUE;
                }
                if (GET_MONEY(ch) < cost)
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
#define IS_MOONSTONE_CORE(obj) (obj_index[obj->R_num].virtual_number == AUTOMATONS_MOONSTONE_CORE)
#define IS_MOONSTONE_PART(obj) (IS_MOONSTONE_FRAGMENT(obj) || IS_MOONSTONE_CORE(obj))

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
              obj_from_char(obj, TRUE);
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
            obj_from_char(moonstone, TRUE);
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
          obj_from_char(moonstone, TRUE);
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
