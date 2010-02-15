
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
#include "ship_ai.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include <math.h>

struct CargoStats ship_cargo_market_mod[NUM_PORTS];
struct CargoStats ship_contra_market_mod[NUM_PORTS];

const CargoData cargo_location_data[NUM_PORTS] = {
  //  Base cargo cost, Base contra cost, Required frags for contraband
  { 31,    162,    150},
  { 33,    172,    150},
  { 30,    146,    100},
  { 38,    166,    150},
  { 28,    184,    200},
  { 32,    153,    100},
  { 29,    190,    200},
  { 36,    174,    150},
  { 34,    160,    150},
};

const char *cargo_name[NUM_PORTS] = {
  "&+LCured &+rMeats &+gand &+YCheeses&N",
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

const int ship_cargo_location_mod[NUM_PORTS][NUM_PORTS] = {
// Flann  Dalvik  Menden   Myra  Torrhan  Sarmiz    SP    Venan     TG
  { 100,    100,     70,     90,     85,    105,     90,    100,    105}, /* Flann        */
  { 100,    100,    100,     70,     80,     95,     95,    100,     75}, /* Dalvik       */
  {  70,    100,    100,     90,    100,    100,     70,     70,    105}, /* Menden       */
  {  90,     70,     90,    100,    100,     95,    105,     95,     90}, /* Myra         */
  {  85,     80,    100,    100,    100,    100,     90,     80,     75}, /* Torrhan      */
  { 105,     90,    100,     90,    100,    100,     85,    100,    100}, /* Sarmiz       */
  {  70,     95,     70,    105,     90,     85,    100,     80,     95}, /* SP           */
  { 100,    100,     70,     85,     80,    105,     80,    100,     85}, /* Venan        */
  { 105,     75,    100,     90,     75,    100,     95,     85,    100}, /* TG           */
};

void initialize_ship_cargo()
{
  for (int i = 0; i < NUM_PORTS; i++) 
  {
    for (int j = 0; j < NUM_PORTS; j++) 
    {
      ship_cargo_market_mod[i].buy[j] =    50;
      ship_cargo_market_mod[i].sell[j] =  100;
      ship_contra_market_mod[i].buy[j] =   50;
      ship_contra_market_mod[i].sell[j] = 100;
    }
  }
  if (!read_cargo()) {
    logit(LOG_FILE, "Error reading cargo file!\r\n");
  }
}

int read_cargo()
{
  FILE    *f = NULL;
  int      i, j;

  f = fopen("Ships/CargoData", "r");
  if (!f)
  {
    return FALSE;
  }
  for (i = 0; i < NUM_PORTS; i++)
  {
    for (j = 0; j < NUM_PORTS; j++)
    {
      fscanf(f, "%f %f %f %f\n", &ship_cargo_market_mod[i].buy[j], &ship_cargo_market_mod[i].sell[j], &ship_contra_market_mod[i].buy[j], &ship_contra_market_mod[i].sell[j]);
    }
  }
  fclose(f);
  return TRUE;
}

int write_cargo()
{
  FILE    *f = NULL;
  int      i, j;

  f = fopen("Ships/CargoData", "w");
  if (!f)
  {
    return FALSE;
  }
  for (i = 0; i < NUM_PORTS; i++)
  {
    for (j = 0; j < NUM_PORTS; j++)
    {
      fprintf(f, "%d %d %d %d\n", (int) ship_cargo_market_mod[i].buy[j], (int) ship_cargo_market_mod[i].sell[j], (int) ship_contra_market_mod[i].buy[j], (int) ship_contra_market_mod[i].sell[j]);
    }
  }
  fclose(f);
  return TRUE;
}

void cargo_activity()
{
  int      i, j;

  for (i = 0; i < NUM_PORTS; i++)
  {
    for (j = 0; j < NUM_PORTS; j++)
    {
      if(i==j)
        continue;
      
      if (ship_cargo_market_mod[i].buy[j] > MINBUYMOD)
      {
        ship_cargo_market_mod[i].buy[j] = max((float)MINBUYMOD, ship_cargo_market_mod[i].buy[j] - BUYADJUSTAUTO);
      }
      if (ship_contra_market_mod[i].buy[j] > MINBUYMOD)
      {
        ship_contra_market_mod[i].buy[j] = max((float)MINBUYMOD, ship_contra_market_mod[i].buy[j] - BUYADJUSTAUTO);
      }
      if (ship_cargo_market_mod[i].sell[j] < MAXSELLMODAUTO)
      {
        ship_cargo_market_mod[i].sell[j] = MIN((float)MAXSELLMODAUTO, ship_cargo_market_mod[i].sell[j] + SELLADJUSTAUTOINC);
      }
      if (ship_cargo_market_mod[i].sell[j] > MAXSELLMODAUTO)
      {
        ship_cargo_market_mod[i].sell[j] = MAX((float)MAXSELLMODAUTO, ship_cargo_market_mod[i].sell[j] - SELLADJUSTAUTODEC);
      }
      if (ship_contra_market_mod[i].sell[j] < MAXSELLMODAUTO)
      {
        ship_contra_market_mod[i].sell[j] = MIN((float)MAXSELLMODAUTO, ship_contra_market_mod[i].sell[j] + SELLADJUSTAUTOINC);
      }
      if (ship_contra_market_mod[i].sell[j] > MAXSELLMODAUTO)
      {
        ship_contra_market_mod[i].sell[j] = MAX((float)MAXSELLMODAUTO, ship_contra_market_mod[i].sell[j] - SELLADJUSTAUTODEC);
      }
    }
  }
}

int cargo_sell_price(int location)
{
  return cargo_sell_price(location, location);
}

int cargo_sell_price(int location, int type)
{
  return (int) (cargo_location_data[type].base_cost_cargo * (ship_cargo_location_mod[location][type] / 100.0) * (ship_cargo_market_mod[location].buy[type] / 100.0));
}

int cargo_buy_price(int location, int type)
{
  return (int) (cargo_location_data[type].base_cost_cargo * (ship_cargo_location_mod[location][type] / 100.0) * (ship_cargo_market_mod[location].sell[type] / 100.0));
}

int contra_sell_price(int location)
{
  return contra_sell_price(location, location);
}

int contra_sell_price(int location, int type)
{
  return (int) (cargo_location_data[type].base_cost_contra * (ship_cargo_location_mod[location][type] / 100.0) * (ship_contra_market_mod[location].buy[type] / 100.0));
}

int contra_buy_price(int location, int type)
{
  return (int) (cargo_location_data[type].base_cost_contra * (ship_cargo_location_mod[location][type] / 100.0) * (ship_contra_market_mod[location].sell[type] / 100.0));
}

void adjust_ship_market(int transaction, int location, int type, int volume)
{
  if( transaction == SOLD_CARGO )
  {
    ship_cargo_market_mod[location].sell[type] = MAX(MINSELLMOD, ship_cargo_market_mod[location].sell[type] - (ONSELLADJUST * (float)volume));
    for (int j = 0; j < NUM_PORTS; j++) 
    {
      if (j != location)
      {
        ship_cargo_market_mod[j].sell[type] = MIN(MAXSELLMOD, ship_cargo_market_mod[j].sell[type] + (ONSELLADJUSTCARGO * (float)volume));
      }
      for (int k = 0; k < NUM_PORTS; k++) 
      {
        if (k != type)
        {
          ship_cargo_market_mod[j].sell[k] = MIN(MAXSELLMOD, ship_cargo_market_mod[j].sell[k] + (ONSELLADJUSTALL * (float)volume));
        }
      }
    }
  }
  else if( transaction == BOUGHT_CARGO )
  {
    ship_cargo_market_mod[location].buy[type] += ONBUYADJUST * volume;

    if (ship_cargo_market_mod[location].buy[type] > MAXBUYMOD)
      ship_cargo_market_mod[location].buy[type] = MAXBUYMOD;    
  }
  else if( transaction == SOLD_CONTRA )
  {
    ship_contra_market_mod[location].sell[type] = MAX(MINSELLMOD, ship_contra_market_mod[location].sell[type] - (ONSELLADJUST * (float)volume * 5.0));
    for (int j = 0; j < NUM_PORTS; j++) 
    {
      if (j != location)
      {
        ship_contra_market_mod[j].sell[type] = MIN(MAXSELLMOD, ship_contra_market_mod[j].sell[type] + (ONSELLADJUSTCARGO * (float)volume * 5.0));
      }
      for (int k = 0; k < NUM_PORTS; k++) 
      {
        if (k != type)
        {
          ship_contra_market_mod[j].sell[k] = MIN(MAXSELLMOD, ship_contra_market_mod[j].sell[k] + (ONSELLADJUSTALL * (float)volume * 5.0));
        }
      }
    } 
  }
  else if( transaction == BOUGHT_CONTRA )
  {
    ship_contra_market_mod[location].buy[type] += ONBUYADJUST * volume;
    
    if (ship_contra_market_mod[location].buy[type] > MAXBUYMOD)
      ship_contra_market_mod[location].buy[type] = MAXBUYMOD;
  }
  
  write_cargo(); 
}

int required_ship_frags_for_contraband(int type)
{
  return cargo_location_data[type].required_frags;
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
