
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ships.h"
#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "events.h"
#include "sql.h"
#include "timers.h"
#include <math.h>

float ship_cargo_market_mod[NUM_PORTS][NUM_PORTS];
float ship_cargo_market_mod_delayed[NUM_PORTS][NUM_PORTS];
float ship_contra_market_mod[NUM_PORTS][NUM_PORTS];

// This matrix shows the base cost in platinum for each port's cargo/contraband,
// as well as the minimum number of ship frags required to be able to buy that
// type of contraband
const CargoData cargo_location_data[NUM_PORTS] = {
//  Base cargo cost, Base contra cost, Required frags for contraband
  { 42,    192,    150},
  { 46,    202,    150},
  { 40,    176,    100},
  { 56,    196,    150},
  { 36,    214,    200},
  { 44,    183,    100},
  { 38,    220,    200},
  { 52,    204,    150},
  { 48,    190,    150},
/* Old prices: changed to accomodate gem mines causing inflation. 01/04/2015
  { 21,    142,    150},
  { 23,    152,    150},
  { 20,    126,    100},
  { 28,    146,    150},
  { 18,    164,    200},
  { 22,    133,    100},
  { 19,    170,    200},
  { 26,    154,    150},
  { 24,    140,    150},
*/
};

// This is the matrix that shows each port's preference for the other ports' cargo. Number is percentage.
// This table is based on exact shortest-path distances between ocean_map_squares, weighted to reduce difference
const int cargo_location_mod[NUM_PORTS][NUM_PORTS] = {
//            Flann  Dalvik Menden Myrabo Torrha Sarmiz Storm  Venan' Thur'G      MIN     MAX
/*  Flann */ {    0,   254,   190,   214,   225,   311,   211,   249,   272 }, // Menden  Sarmiz
/* Dalvik */ {  254,     0,   286,   178,   235,   257,   275,   296,   201 }, // Myrabo  Venan
/* Menden */ {  190,   286,     0,   246,   271,   290,   179,   185,   297 }, // Storm   Thur'G
/* Myrabo */ {  214,   178,   246,     0,   273,   287,   290,   247,   239 }, // Dalvik  Storm
/* Torrha */ {  225,   235,   271,   273,     0,   308,   224,   254,   203 }, // Thur'G  Sarmiz
/* Sarmiz */ {  311,   257,   290,   287,   308,     0,   243,   276,   314 }, // Storm   Thur'G
/* Storm  */ {  211,   275,   179,   290,   224,   243,     0,   231,   271 }, // Menden  Myrabo
/* Venan' */ {  249,   296,   185,   247,   254,   276,   231,     0,   252 }, // Menden  Dalvik
/* Thur'G */ {  272,   201,   297,   239,   203,   314,   271,   252,     0 }  // Dalvik  Sarmiz
 // Shortest route: Dalvik <-> Myrabolus (178)      
 // Longest route: Sarmiz'Duul <-> Thur'Gurax (314) 
 // Num routes: 36, Average distance: 250           
};

const char *cargo_name[NUM_PORTS] = {
  "&+LCured &+rMeats&N",
  "&+GExotic &+yFoods&N",
  "&+gPine &+LPitch&N",
  "&+CElven &+RWines&N",
  "&+LBulk &+yLumber&N",
  "&+LBlack&+WSteel &+wIngots&N",
  "&+LBulk Coal&N",
  "&+MSi&+mlk &+BCl&+Co&+Yth&N",
  "&+YCopper &+yIngots&N",
};

const char *contra_name[NUM_PORTS] = {
  "&+gAncient &+LBooks &+gand &+yScrolls&N",
  "&+GExotic &+WHerbs&N",
  "&+GExotic &+COils&N",
  "&+CElvish &+BAntiquities&N",
  "&+GRare &+YMagical &+yComponents&N",
  "&+RRare &+MDyes&N",
  "&+LR&+woug&+Lh &+WD&+wi&+Wa&+wm&+Wo&+wn&+Wd&+ws&N",
  "&+LR&+woug&+Lh &+RR&+ru&+Rb&+ri&+Re&+rs&N",
  "&+mUnderdark &+MMithril&N",
};

void reset_cargo()
{
  for (int i = 0; i < NUM_PORTS; i++) 
  {
    for (int j = 0; j < NUM_PORTS; j++) 
    {
      ship_cargo_market_mod[i][j] = 1.0;
      ship_contra_market_mod[i][j] = 1.0;      
      ship_cargo_market_mod_delayed[i][j] = 1.0;
    }
  }  
}

void initialize_ship_cargo()
{
  reset_cargo();
  
  if(!read_cargo())
  {
    logit(LOG_SHIP, "Error reading market values from database!");
  }
}

int read_cargo()
{
#ifdef __NO_MYSQL__
    return FALSE;
#else

    if(!qry("select type, port_id, cargo_type, modifier from ship_cargo_market_mods"))
    {
        logit(LOG_DEBUG, "read_cargo(): cargo query failed!");
        return FALSE;
    }
  
	MYSQL_RES *res = mysql_store_result(DB);
  
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
    {
        char *type = row[0];
        int port_id = atoi(row[1]);
        int cargo_type = atoi(row[2]);
        float modifier = atof(row[3]);
    
        if( port_id < 0 || port_id >= NUM_PORTS )
        {
            logit(LOG_DEBUG, "read_cargo(): invalid cargo record: (%s, %d, %d, %f)", type, port_id, cargo_type, modifier); 
            continue;
        }
    
        if( !strcmp(type, "CARGO") )
        {
            ship_cargo_market_mod[port_id][cargo_type] = modifier; //BOUNDEDF(get_property("ship.cargo.minPriceMod", 0.0), modifier, get_property("ship.cargo.maxPriceMod", 0.0));
            ship_cargo_market_mod_delayed[port_id][cargo_type] = ship_cargo_market_mod[port_id][cargo_type];
        }
        else if( !strcmp(type, "CONTRABAND") )
        {
            ship_contra_market_mod[port_id][cargo_type] = modifier; //BOUNDEDF(get_property("ship.contraband.minPriceMod", 0.0), modifier, get_property("ship.contraband.maxPriceMod", 0.0));
        }
	}
  
	mysql_free_result(res);
  
    return TRUE;

#endif
}


int write_cargo()
{
#ifdef __NO_MYSQL__
  return FALSE;
#else
  if(!qry("delete from ship_cargo_market_mods"))
  {
    logit(LOG_DEBUG, "write_cargo(): cargo query failed!");
    return FALSE;
  }

  if(!qry("delete from ship_cargo_prices"))
  {
    logit(LOG_DEBUG, "write_cargo(): price query failed!");
    return FALSE;
  }
  
  for( int port = 0; port < NUM_PORTS; port++ )
  {
    for( int type = 0; type < NUM_PORTS; type++ )
    {
      if(port == type)
      {
        // insert into prices table
        if(!qry("insert into ship_cargo_prices (type, port_id, cargo_type, price) values ('%s', %d, %d, %d)", "CARGO", port, type, cargo_sell_price(port)))
        {
          logit(LOG_DEBUG, "write_cargo(): insert query failed!");
          return FALSE;
        }
        if(!qry("insert into ship_cargo_prices (type, port_id, cargo_type, price) values ('%s', %d, %d, %d)", "CONTRABAND", port, type, contra_sell_price(port)))
        {
          logit(LOG_DEBUG, "write_cargo(): insert query failed!");
          return FALSE;
        }              
      }
      else
      {
        // insert into prices table
        if(!qry("insert into ship_cargo_prices (type, port_id, cargo_type, price) values ('%s', %d, %d, %d)", "CARGO", port, type, cargo_buy_price(port, type)))
        {
          logit(LOG_DEBUG, "write_cargo(): insert query failed!");
          return FALSE;
        }
        
        if(!qry("insert into ship_cargo_prices (type, port_id, cargo_type, price) values ('%s', %d, %d, %d)", "CONTRABAND", port, type, contra_buy_price(port, type)))
        {
          logit(LOG_DEBUG, "write_cargo(): insert query failed!");
          return FALSE;
        }        
      }
      
      // insert into mods table
      if(!qry("insert into ship_cargo_market_mods (type, port_id, cargo_type, modifier) values ('%s', %d, %d, %f)", "CARGO", port, type, ship_cargo_market_mod[port][type]))
      {
        logit(LOG_DEBUG, "write_cargo(): insert query failed!");
        return FALSE;
      }
              
      if(!qry("insert into ship_cargo_market_mods (type, port_id, cargo_type, modifier) values ('%s', %d, %d, %f)", "CONTRABAND", port, type, ship_contra_market_mod[port][type]))
      {
        logit(LOG_DEBUG, "write_cargo(): insert query failed!");
        return FALSE;
      }
    }
  }
  return TRUE;
#endif
}

/*
- ship.cargo.autoSellAdjustMod
- ship.cargo.autoBuyAdjustMod
- ship.cargo.minPriceMod
- ship.cargo.maxPriceMod
- ship.contraband.autoSellAdjustMod
- ship.contraband.autoBuyAdjustMod
- ship.contraband.minPriceMod
- ship.contraband.maxPriceMod

ship.cargo.autoSellAdjustRate=0.05
ship.cargo.autoBuyAdjustRate=0.05
ship.cargo.sellPriceMod=1.0
ship.cargo.buyPriceMod=1.0
ship.contraband.autoSellAdjustRate=0.05
ship.contraband.autoBuyAdjustRate=0.05
ship.contraband.sellPriceMod=1.0
ship.contraband.buyPriceMod=1.0
*/

void update_cargo(bool force)
{
  if(!force && !has_elapsed("update_cargo", get_property("ship.cargo.update.secs", 1800)) )
    return;

  debug("update_cargo()");

  int      i, j;
  
  // all mods auto-balance to 1.0 with time, the farther it from 1.0, the faster it changes
  for (i = 0; i < NUM_PORTS; i++)
  {
    for (j = 0; j < NUM_PORTS; j++)
    {
      if(i==j)
      {
        float cargo_sell_mod = get_property("ship.cargo.sellPriceMod", 1.0);
        if (ship_cargo_market_mod[i][j] < cargo_sell_mod)
        {
          ship_cargo_market_mod[i][j] = MAX(cargo_sell_mod, ship_cargo_market_mod[i][j] + (cargo_sell_mod - ship_cargo_market_mod[i][j] + 0.1) * get_property("ship.cargo.autoSellAdjustRate", 0.05));
        }
        if (ship_cargo_market_mod[i][j] > cargo_sell_mod)
        {
          ship_cargo_market_mod[i][j] = MIN(cargo_sell_mod, ship_cargo_market_mod[i][j] - (ship_cargo_market_mod[i][j] - cargo_sell_mod + 0.1) * get_property("ship.cargo.autoSellAdjustRate", 0.05));
        }

        float contra_sell_mod = get_property("ship.contraband.sellPriceMod", 1.0);
        if (ship_contra_market_mod[i][j] < contra_sell_mod)
        {
          ship_contra_market_mod[i][j] = MAX(contra_sell_mod, ship_contra_market_mod[i][j] + (contra_sell_mod - ship_contra_market_mod[i][j] + 0.1) * get_property("ship.contraband.autoSellAdjustRate", 0.05));
        }
        if (ship_contra_market_mod[i][j] > contra_sell_mod)
        {
          ship_contra_market_mod[i][j] = MIN(contra_sell_mod, ship_contra_market_mod[i][j] - (ship_contra_market_mod[i][j] - contra_sell_mod + 0.1) * get_property("ship.contraband.autoSellAdjustRate", 0.05));
        }
      }
      else
      {
        float cargo_buy_mod = get_property("ship.cargo.buyPriceMod", 1.0);
        if (ship_cargo_market_mod[i][j] < cargo_buy_mod)
        {
          ship_cargo_market_mod[i][j] = MAX(cargo_buy_mod, ship_cargo_market_mod[i][j] + (cargo_buy_mod - ship_cargo_market_mod[i][j] + 0.1) * get_property("ship.cargo.autoBuyAdjustRate", 0.05));
        }
        if (ship_cargo_market_mod[i][j] > cargo_buy_mod)
        {
          ship_cargo_market_mod[i][j] = MIN(cargo_buy_mod, ship_cargo_market_mod[i][j] - (ship_cargo_market_mod[i][j] - cargo_buy_mod + 0.1) * get_property("ship.cargo.autoBuyAdjustRate", 0.05));
        }

        float contra_buy_mod = get_property("ship.contraband.buyPriceMod", 1.0);
        if (ship_contra_market_mod[i][j] < contra_buy_mod)
        {
          ship_contra_market_mod[i][j] = MAX(contra_buy_mod, ship_contra_market_mod[i][j] + (contra_buy_mod - ship_contra_market_mod[i][j] + 0.1) * get_property("ship.contraband.autoBuyAdjustRate", 0.05));
        }
        if (ship_contra_market_mod[i][j] > contra_buy_mod)
        {
          ship_contra_market_mod[i][j] = MIN(contra_buy_mod, ship_contra_market_mod[i][j] - (ship_contra_market_mod[i][j] - contra_buy_mod + 0.1) * get_property("ship.contraband.autoBuyAdjustRate", 0.05));
        }
      }     
    }
  }
  
  if(!write_cargo())
  {
    logit(LOG_SHIP, "Error writing market values!");
  }  
  
  set_timer("update_cargo");
}

void update_cargo()
{
  update_cargo(false);
}

void update_delayed_cargo_prices()
{
  if( !has_elapsed("update_delayed_cargo_prices", get_property("ship.cargo.updateDelayedPrices.secs", 1800)) )
    return;
  
  debug("update_delayed_cargo_prices()");
  
  int      i, j;
  
  for (i = 0; i < NUM_PORTS; i++)
  {
    for (j = 0; j < NUM_PORTS; j++)
    {
      ship_cargo_market_mod_delayed[i][j] = ship_cargo_market_mod[i][j];
    }
  }
  
  set_timer("update_delayed_cargo_prices");
}

// this gets run once every minute
void cargo_activity()
{
  update_cargo();
  update_delayed_cargo_prices();
}


void calculate_port_distances()
{
  logit(LOG_SHIP, "Calculating distances between ports...");
  
  char line[MAX_STRING_LENGTH];
  char buff[MAX_STRING_LENGTH];
  strcat(line, "       ");
  
  for( int i = 0; i < NUM_PORTS; i++ )
  {
    sprintf(buff, "%6s ", string(ports[i].loc_name).substr(0,6).c_str());
    strcat(line, buff);
  }
  
  strcat(line, "MIN    MAX");
  
  logit(LOG_SHIP, line);
  line[0] = '\0';
  
  int global_min_route[3] = {
    10000, 0, 0
  };
  
  int global_max_route[3] = {
    0, 0, 0
  };
  
  float avg_distance = 0;
  int count = 0;
  
  for( int i = 0; i < NUM_PORTS; i++ )
  {
    int min_port = 0;
    int min_dist = 10000;
    int max_port = 0;
    int max_dist = -1;
    
    sprintf(buff, "%6s ", string(ports[i].loc_name).substr(0,6).c_str());
    strcat(line, buff);
    
    for( int j = 0; j < NUM_PORTS; j++ )
    {
      if( i == j )
      {
        strcat(line, "     0,");
        continue;
      }
            
      vector<int> route;
      
      bool found_path = dijkstra(real_room0(ports[i].ocean_map_room), real_room0(ports[j].ocean_map_room), valid_ship_edge, route);
      
      if( found_path )
      {
        int dist = (int) route.size();
        dist = (dist + 250) / 2;
        
        if( dist < min_dist )
        {
          min_dist = dist;
          min_port = j;
        }
        else if( dist > max_dist )
        {
          max_dist = dist;
          max_port = j;
        }
        
        sprintf(buff, "%6d,", dist);
        strcat(line, buff);
        
        avg_distance += (float) dist;
        count++;
      }
      else
      {
        strcat(line, "     ? ");
      }
    }    
    
    sprintf(buff, "%6s %6s", string(ports[min_port].loc_name).substr(0,6).c_str(), string(ports[max_port].loc_name).substr(0,6).c_str());
    strcat(line, buff);
    
    if( min_dist < global_min_route[0] )
    {
      global_min_route[0] = min_dist;
      global_min_route[1] = i;
      global_min_route[2] = min_port;
    }
    else if( max_dist > global_max_route[0] )
    {
      global_max_route[0] = max_dist;
      global_max_route[1] = i;
      global_max_route[2] = max_port;
    }
        
    logit(LOG_SHIP, line);
    line[0] = '\0';
  }
  
  logit(LOG_SHIP, "Shortest route: %s <-> %s (%d)", ports[global_min_route[1]].loc_name, ports[global_min_route[2]].loc_name, global_min_route[0]);
  logit(LOG_SHIP, "Longest route: %s <-> %s (%d)", ports[global_max_route[1]].loc_name, ports[global_max_route[2]].loc_name, global_max_route[0]);  
  logit(LOG_SHIP, "Num routes: %d, Average distance: %d", count/2, (int) avg_distance / count);  
}

// i.e. the price the port charges to sell its cargo
int cargo_sell_price(int location, bool delayed)
{
  // the port sells its own cargo at just base price * market mod
  if( delayed )
  {
    return (int) (1000 * cargo_location_data[location].base_cost_cargo * ship_cargo_market_mod_delayed[location][location]);    
  }
  else
  {
    return (int) (1000 * cargo_location_data[location].base_cost_cargo * ship_cargo_market_mod[location][location]);
  }
}

// i.e. the price the port will pay to buy cargo
int cargo_buy_price(int location, int type, bool delayed)
{
  if (location == type)
    return cargo_sell_price(location, delayed) * 0.5;
  // Adding a 1.5 multiplier for cargo being sold to account for gem mines. 01/04/2015
  if( delayed )
  {
    return (int) (1500 * cargo_location_data[type].base_cost_cargo * (cargo_location_mod[location][type] / 100.0) * ship_cargo_market_mod_delayed[location][type]);
  }
  else
  {
    return (int) (1500 * cargo_location_data[type].base_cost_cargo * (cargo_location_mod[location][type] / 100.0) * ship_cargo_market_mod[location][type]);
  }
}

// i.e. the price the port charges to sell its contraband
int contra_sell_price(int location)
{
  // the port sells its own contraband at just base price * market mod
   return (int) (1000 * cargo_location_data[location].base_cost_contra * ship_contra_market_mod[location][location]);
}

// i.e. the price the port will pay to buy contraband
int contra_buy_price(int location, int type)
{
  if (location == type)
    return contra_sell_price(location) * 0.5;
  else
    // Adding a 1.5 multiplier for cargo being sold to account for gem mines. 01/04/2015
    return (int) (1500 * cargo_location_data[type].base_cost_contra * (cargo_location_mod[location][type] / 100.0) * ship_contra_market_mod[location][type]);
}

void adjust_ship_market(int transaction, int location, int type, int volume)
{
  if( transaction == SOLD_CARGO )
  {
    // player sold cargo, so adjust market price downwards slightly
    ship_cargo_market_mod[location][type] = ship_cargo_market_mod[location][type] * (1.0 - get_property("ship.cargo.sellAdjustMod", 0.005) * volume);
  }
  else if( transaction == BOUGHT_CARGO )
  {
    // player bought cargo, so adjust market price upwards slightly
    ship_cargo_market_mod[location][type] = ship_cargo_market_mod[location][type] * (1.0 + get_property("ship.cargo.buyAdjustMod", 0.003) * volume);
  }
  else if( transaction == SOLD_CONTRA )
  {
    // player sold contraband, so adjust market price downwards slightly
    ship_contra_market_mod[location][type] = ship_contra_market_mod[location][type] * (1.0 - get_property("ship.contraband.sellAdjustMod", 0.025) * volume);
  }
  else if( transaction == BOUGHT_CONTRA )
  {
    // player bought contraband, so adjust market price upwards slightly
    ship_contra_market_mod[location][type] = ship_contra_market_mod[location][type] * (1.0 + get_property("ship.contraband.buyAdjustMod", 0.015) * volume);
  }
  
  if(!write_cargo())
  {
    logit(LOG_SHIP, "Error writing market values!");
  }
}

int required_ship_frags_for_contraband(int type)
{
  return cargo_location_data[type].required_frags;
}

bool can_buy_contraband(P_ship ship, int type)
{
    int frags = required_ship_frags_for_contraband(type);
    if (ship->frags >= frags)
        return true;
    if (ship->crew.sail_skill >= frags * 4 &&
        ship->crew.guns_skill >= frags * 1 &&
        ship->crew.rpar_skill >= frags * 2)
    {
        return true;
    }
    return false;
}

const char *cargo_type_name(int type)
{
  if( type < 0 || type >= NUM_PORTS )
    return "";
  
  return cargo_name[type];
}

const char *contra_type_name(int type)
{
  if( type < 0 || type >= NUM_PORTS )
    return "";

  return contra_name[type];
}

void show_cargo_prices(P_char ch)
{
  char line[MAX_STRING_LENGTH];
  char buff[MAX_STRING_LENGTH];
  
  bool delayed = (bool) !IS_TRUSTED(ch);
  
  send_to_char("&+y/------------------------------------------------------------------------------------\\\r\n", ch);
  
  line[0] = '\0';
  strcat(line, "&+y|&n                    ");
  
  // Port names
  for( int i = 0; i < NUM_PORTS; i++ )
  {
    sprintf(buff, "%6s ", string(ports[i].loc_name).substr(0, 6).c_str());
    strcat(line, buff);
  }
  strcat(line, " &+y|\r\n");
  send_to_char(line, ch);
  
  for( int type = 0; type < NUM_PORTS; type++ )
  {
    line[0] = '\0';
    
    sprintf(buff, "&+y| %s ", pad_ansi(cargo_type_name(type), 18).c_str());
    strcat(line, buff);
    
    for( int port = 0; port < NUM_PORTS; port++ )
    {
      if( type == port )
      {
        // show sell price
        sprintf(buff, "&+W%6.2f", (float) cargo_sell_price(port, delayed) / 1000);
      }
      else if( cargo_buy_price(port, type, delayed) > cargo_sell_price(type, delayed) )
      {
        sprintf(buff, "&+g%6.2f", (float) cargo_buy_price(port, type, delayed) / 1000);
      }
      else if( cargo_buy_price(port, type, delayed) < cargo_sell_price(type, delayed) )
      {
        sprintf(buff, "&+r%6.2f", (float) cargo_buy_price(port, type, delayed) / 1000);
      }
      else
      {
        sprintf(buff, "%6.2f", (float) cargo_buy_price(port, type, delayed) / 1000);
      }
      
      strcat(line, pad_ansi(buff, 6).c_str());
      strcat(line, " ");
    }
    
    strcat(line, " &+y|\r\n");
    send_to_char(line, ch);
  }
  
  send_to_char("&+y|                                                                                    &+y|\r\n", ch);
  send_to_char("&+y| &nAll prices in platinum per crate, and are current within the last hour.            &+y|\r\n", ch);
  send_to_char("&+y\\------------------------------------------------------------------------------------/\r\n", ch);
}

void show_contra_prices(P_char ch)
{
  char line[MAX_STRING_LENGTH];
  char buff[MAX_STRING_LENGTH];
  
  send_to_char("&+yCurrent contraband market:\r\n"
               "&+y-------------------------------------------------------------------------------------------------\r\n", ch);
  
  line[0] = '\0';
  strcat(line, "                          ");
  
  // Port names
  for( int i = 0; i < NUM_PORTS; i++ )
  {
    sprintf(buff, "%7s ", string(ports[i].loc_name).substr(0, 7).c_str());
    strcat(line, buff);
  }
  strcat(line, "\r\n");
  send_to_char(line, ch);
  
  for( int type = 0; type < NUM_PORTS; type++ )
  {
    line[0] = '\0';
    
    sprintf(buff, "%s ", pad_ansi(contra_type_name(type), 25).c_str());
    strcat(line, buff);
    
    for( int port = 0; port < NUM_PORTS; port++ )
    {
      if( type == port )
      {
        // show sell price
        sprintf(buff, "&+W%7.2f", (float) contra_sell_price(port) / 1000);
      }
      else if( contra_buy_price(port, type) > contra_sell_price(type) )
      {
        sprintf(buff, "&+g%7.2f", (float) contra_buy_price(port, type) / 1000);
      }
      else if( contra_buy_price(port, type) < contra_sell_price(type) )
      {
        sprintf(buff, "&+r%7.2f", (float) contra_buy_price(port, type) / 1000);
      }
      else
      {
        sprintf(buff, "%7.2f", (float) contra_buy_price(port, type) / 1000);
      }
      
      strcat(line, pad_ansi(buff, 6).c_str());
      strcat(line, " ");
    }
    
    strcat(line, "\r\n");
    send_to_char(line, ch);
  }
  
  send_to_char("\r\nAll prices in platinum per crate.\r\n", ch);
}

int ship_cargo_info_stick(P_obj obj, P_char ch, int cmd, char *arg)
{
  ShipVisitor svs;

  /*
   check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;
  
  if (!obj || !ch)
    return (FALSE);
  
  if (!OBJ_WORN_POS(obj, HOLD))
    return (FALSE);
 
  if (!(IS_TRUSTED(ch)))
    return (FALSE);
 
  if (arg && (cmd == CMD_LOOK))
  {
    if (isname(arg, "cargo"))
    {
      show_cargo_prices(ch);
      return TRUE;
    }
    if (isname(arg, "ships"))
    {
      act( "&+yListing &+YALL ships &+yin game:&n", FALSE, ch, obj, obj, TO_CHAR );
      // LOOP through all ships
      for( bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs) )
      {
        send_to_char_f(ch, "&+yShip:&+C %s&+y Owner: &+C%s ", SHIP_NAME(svs), SHIP_OWNER(svs));
        send_to_char_f(ch, "&+yRoom: %s ", world[svs->location].name);
        if( SHIP_DOCKED(svs) )
        {
          send_to_char_f(ch, "&+y | &+LDOCKED&+y");
        }
        if( SHIP_ANCHORED(svs) )
        {
          send_to_char_f(ch, "&+y | &+YANCHORED&+y");
        }
        if( SHIP_IMMOBILE(svs) )
        {
          send_to_char_f(ch, "&+y | &+rIMMOBILE&+y");
        }
        if( SHIP_SINKING(svs) )
        {
          send_to_char_f(ch, "&+y | &+RSINKING&+y");
        }
        send_to_char_f(ch, "\n");
      }
      return TRUE;
    }
  }
    
  return FALSE;
}

void do_world_cargo(P_char ch, char *arg)
{
  if(!ch)
    return;
  
  if(!arg || !*arg)
  {
    show_cargo_prices(ch);
    send_to_char("\r\n",ch);
    show_contra_prices(ch);    
  }
  else if( is_abbrev(arg, "reload") )
  {
    send_to_char("Reloading cargo mods from database...\r\n", ch);
    if(!read_cargo())
      send_to_char("FAILED!\r\n", ch);
  }
  else if( is_abbrev(arg, "reset") )
  {
    send_to_char("Resetting cargo mods...\r\n", ch);
    reset_cargo();
  }
  else if( is_abbrev(arg, "save") )
  {
    send_to_char("Writing cargo mods to database...\r\n", ch);
    if(!write_cargo())
      send_to_char("FAILED!\r\n", ch);
  }
  else if( is_abbrev(arg, "update") )
  {
    send_to_char("Updating cargo prices...\r\n", ch);
    update_cargo(true);
  }
  
}

