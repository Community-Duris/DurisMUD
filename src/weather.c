/*
   ***************************************************************************
   *  File: weather.c                                          Part of Duris *
   *  Usage: Performing the clock and the weather                              *
   *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
   *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
   ***************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "weather.h"
#include "sound.h"
#include "map.h"

extern int abs(int);

/*
   external vars
 */

extern P_char character_list;
extern P_event current_event;
extern P_room world;
extern int top_of_zone_table;
extern int map_g_modifier;
extern int map_e_modifier;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern P_desc descriptor_list;
extern int top_of_world;
extern P_index obj_index;

int real_object( const int virt );
/*
   some magic values
 */

#define MAGIC_PRECIP_START 1060
#define MAGIC_PRECIP_STOP 970
#define STWS(z, t) send_to_weather_sector((z), (t))

void event_another_hour(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_obj flower;
  const int flowerroom = real_room(41059);

  time_info.hour++;

  if( time_info.hour == 23 )
  {
     flower = world[flowerroom].contents;
     while( flower )
     {
        if( obj_index[flower->R_num].virtual_number == 41004 )
        {
           extract_obj( flower, TRUE );
           flower = read_object( real_object( 41005 ), REAL );
           obj_to_room( flower, flowerroom );
           break;
        }
        flower = flower->next_content;
     }
  }
  else if ( time_info.hour == 1 )
  {
     flower = world[flowerroom].contents;
     while( flower )
     {
        if( obj_index[flower->R_num].virtual_number == 41005 )
        {
           extract_obj( flower, TRUE );
           flower = read_object( real_object( 41004 ), REAL );
           obj_to_room( flower, flowerroom );
           break;
        }
        flower = flower->next_content;
     }
  }

  if (time_info.hour > 23)
  {
    time_info.hour -= 24;
    time_info.day++;
    if (time_info.day > 34)
    {
      time_info.day = 0;
      time_info.month++;
//      update_kingdoms();
      if (time_info.month > 16)
      {
        time_info.month = 0;
        time_info.year++;
      }
    }
  }
  add_event(event_another_hour, PULSES_IN_TICK, NULL, NULL, NULL, 0, NULL, 0);
}
const char *astralMsgs[] =
{
  NULL,
  "&+bThe first hint of &+Cdaylight&N &+bcan be seen on the northern horizon.&n\r\n",
  "&+BThe first rays of&N &+Ysunlight&N&+B signal that day is approaching.&n\r\n",
  "&+cThe&N &+Ysun&N&+c rises over the northern horizon.&n\r\n",
  "&+CThe day has begun.&n\r\n",
  "&+MThe&n&+Y sun&N&+M hangs low on the southern sky.&n\r\n",
  "&+mThe &+Ysun&N &+mstarts to set in the south.&n\r\n",
  "&+LShadows stretch across the land as the &+Ysun&+L limps towards the horizon.&n\r\n",
  "&+bThe &N&+Ysun&N &+bvanishes behind the southern horizon.&n\r\n",
  "&+LThe night has begun.&n\r\n",
};

// returns an index into astralMsgs (or -1)
int astral_clock_setMapModifiers(void)
{
  int astralMsgIdx = 0;

  switch (time_info.hour)
  {
  // Dawn
  case 4:
    astralMsgIdx = 1;
    map_g_modifier = 5;
    map_e_modifier = 7;
    break;
  case 5:
    astralMsgIdx = 2;
    map_g_modifier = 6;
    map_e_modifier = 6;
    break;
  case 6:
  case 7:
    map_g_modifier = 7;
    map_e_modifier = 3;
    break;
  case 8:
    astralMsgIdx = 3;
    map_g_modifier = 7;
    map_e_modifier = 3;
    break;
  case 9:
    astralMsgIdx = 4;
    map_g_modifier = 7;
    map_e_modifier = 3;
    break;
  case 10:
  case 11:
  case 12:
    map_g_modifier = 7;
    map_e_modifier = 3;
    break;
  case 13:
    map_g_modifier = 8;
    map_e_modifier = 3;
    break;
  case 14:
    astralMsgIdx = 5;
  case 15:
    map_g_modifier = 7;
    map_e_modifier = 3;
    break;
  case 16:
    astralMsgIdx = 6;
    map_g_modifier = 7;
    map_e_modifier = 3;
    break;
  case 17:
    astralMsgIdx = 7;
    map_g_modifier = 6;
    map_e_modifier = 3;
    break;
  case 18:
    astralMsgIdx = 8;
    map_g_modifier = 6;
    map_e_modifier = 5;
    break;
  case 19:
    astralMsgIdx = 9;
    map_g_modifier = 5;
    map_e_modifier = 7;
    break;
  // Night time
  case 20:
  case 21:
  case 22:
  case 23:
  case 24:
  case 0:
  case 1:
  case 2:
  case 3:
    map_g_modifier = 3;
    map_e_modifier = 8;
    break;
  }
  return astralMsgIdx;
}

void init_astral_clock(void)
{
  astral_clock_setMapModifiers();
}

void event_astral_clock(P_char ch, P_char victim, P_obj obj, void *data)
{
  const char *s = astralMsgs[astral_clock_setMapModifiers()];

  if (s)
  {
    P_desc   d;

    /*
       echo it to the world... but ONLY people who are not indoors, and who are
       awake
     */
    for (d = descriptor_list; d; d = d->next)
    {
      if ((d->connected == CON_PLYNG) && (d->character)
          && (AWAKE(d->character)))
      {
        int      r = d->character->in_room;

        if (!IS_SET(world[r].room_flags, INDOORS) && NORMAL_PLANE(r) &&
            !IS_UNDERWORLD(r) && d->character->specials.z_cord >= 0)
          send_to_char(s, d->character);
      }
    }
  }
  add_event(event_astral_clock, PULSES_IN_TICK, NULL, NULL, NULL, 0, NULL, 0);
}

void event_weather_change(P_char ch, P_char victim, P_obj obj, void *data)
{
  int      zon, magic, old_wind;
  signed char old_temp;
  unsigned char old_precip, season_num;
  struct climate *clime;
  struct weather_data *cond;

  zon = *((int*)data);
  clime = &sector_table[zon].climate;
  cond = &sector_table[zon].conditions;
  old_temp = cond->temp;
  old_precip = cond->precip_rate;
  old_wind = cond->windspeed;

  /* Which season is it? */
  season_num = get_season(zon);

  /* Clear the control weather bit */
  REMOVE_BIT(cond->flags, WEATHER_CONTROLLED);

  /* Create changes for this hour */
  cond->free_energy =
    BOUNDED(3000, clime->energy_add + cond->free_energy, 50000);
  switch (clime->season_wind[season_num])
  {
  case SEASON_CALM:
    if (cond->windspeed > 25)
      cond->windspeed -= 5;
    else
      cond->windspeed += number(-2, 1);
    break;
  case SEASON_BREEZY:
    if (cond->windspeed > 40)
      cond->windspeed -= 5;
    else
      cond->windspeed += number(-2, 2);
    break;
  case SEASON_UNSETTLED:
    if (cond->windspeed < 5)
      cond->windspeed += 5;
    else if (cond->windspeed > 60)
      cond->windspeed -= 5;
    else
      cond->windspeed += number(-6, 6);
    break;
  case SEASON_WINDY:
    if (cond->windspeed < 15)
      cond->windspeed += 5;
    else if (cond->windspeed > 80)
      cond->windspeed -= 5;
    else
      cond->windspeed += number(-6, 6);
    break;
  case SEASON_CHINOOK:
    if (cond->windspeed < 25)
      cond->windspeed += 5;
    else if (cond->windspeed > 110)
      cond->windspeed -= 5;
    else
      cond->windspeed += number(-15, 15);
    break;
  case SEASON_VIOLENT:
    if (cond->windspeed < 40)
      cond->windspeed += 5;
    else
      cond->windspeed += number(-8, 8);
    break;
  case SEASON_HURRICANE:
    cond->windspeed = 100;
    break;
  default:
    break;
  }
  cond->free_energy += cond->windspeed; /* + or - ? */
  if (cond->free_energy < 0)
    cond->free_energy = 0;
  else if (cond->free_energy > 20000)
    cond->windspeed += number(-10, -1);
  cond->windspeed = MAX(0, cond->windspeed);
  switch (clime->season_wind_variance[season_num])
  {
  case 0:
    cond->wind_dir = clime->season_wind_dir[season_num];
    break;
  case 1:
    if (dice(2, 15) * 1000 < cond->free_energy)
      cond->wind_dir = number(0, 3);
    break;
  }
  switch (clime->season_temp[season_num])
  {
  case SEASON_FROSTBITE:
    if (cond->temp > -20)
      cond->temp -= 4;
    else
      cond->temp += number(-3, 3);
    break;
  case SEASON_NIPPY:
    if (cond->temp < -40)
      cond->temp += 2;
    else if (cond->temp > 5)
      cond->temp -= 3;
    else
      cond->temp += number(-3, 3);
    break;
  case SEASON_FREEZING:
    if (cond->temp < -20)
      cond->temp += 2;
    else if (cond->temp > 0)
      cond->temp -= 2;
    else
      cond->temp += number(-2, 2);
    break;
  case SEASON_COLD:
    if (cond->temp < -10)
      cond->temp += 1;
    else if (cond->temp > 5)
      cond->temp -= 2;
    else
      cond->temp += number(-2, 2);
    break;
  case SEASON_COOL:
    if (cond->temp < -3)
      cond->temp += 2;
    else if (cond->temp > 14)
      cond->temp -= 2;
    else
      cond->temp += number(-3, 3);
    break;
  case SEASON_MILD:
    if (cond->temp < 7)
      cond->temp += 2;
    else if (cond->temp > 26)
      cond->temp -= 2;
    else
      cond->temp += number(-2, 2);
    break;
  case SEASON_WARM:
    if (cond->temp < 19)
      cond->temp += 2;
    else if (cond->temp > 33)
      cond->temp -= 2;
    else
      cond->temp += number(-3, 3);
    break;
  case SEASON_HOT:
    if (cond->temp < 24)
      cond->temp += 3;
    else if (cond->temp > 46)
      cond->temp -= 2;
    else
      cond->temp += number(-3, 3);
    break;
  case SEASON_BLUSTERY:
    if (cond->temp < 34)
      cond->temp += 3;
    else if (cond->temp > 53)
      cond->temp -= 2;
    else
      cond->temp += number(-5, 5);
    break;
  case SEASON_HEATSTROKE:
    if (cond->temp < 44)
      cond->temp += 5;
    else if (cond->temp > 60)
      cond->temp -= 5;
    else
      cond->temp += number(-3, 3);
    break;
  case SEASON_BOILING:
    if (cond->temp < 80)
      cond->temp += 5;
    else if (cond->temp > 120)
      cond->temp -= 5;
    else
      cond->temp += number(-6, 6);
    break;
  default:
    break;
  }
  if (cond->flags & SUN_VISIBLE)
    cond->temp += 2;
  else if (!(clime->flags & NO_SUN_EVER))
    cond->temp -= 2;
  switch (clime->season_precip[season_num])
  {
  case SEASON_NO_PRECIP_EVER:
    if (cond->precip_rate > 0)
      cond->precip_rate /= 2;
    cond->humidity = 0;
    break;
  case SEASON_ARID:
    if (cond->humidity > 30)
      cond->humidity -= 3;
    else
      cond->humidity += number(-3, 2);
    if (old_precip > 20)
      cond->precip_rate -= 8;
    break;
  case SEASON_DRY:
    if (cond->humidity > 50)
      cond->humidity -= 3;
    else
      cond->humidity += number(-4, 3);
    if (old_precip > 35)
      cond->precip_rate -= 6;
    break;
  case SEASON_LOW_PRECIP:
    if (cond->humidity < 13)
      cond->humidity += 3;
    else if (cond->humidity > 91)
      cond->humidity -= 2;
    else
      cond->humidity += number(-5, 4);
    if (old_precip > 45)
      cond->precip_rate -= 10;
    break;
  case SEASON_AVG_PRECIP:
    if (cond->humidity < 30)
      cond->humidity += 3;
    else if (cond->humidity > 80)
      cond->humidity -= 2;
    else
      cond->humidity += number(-9, 9);
    if (old_precip > 55)
      cond->precip_rate -= 5;
    if (old_precip < 15)
      cond->precip_rate += 5;
    break;
  case SEASON_HIGH_PRECIP:
    if (cond->humidity < 40)
      cond->humidity += 3;
    else if (cond->humidity > 90)
      cond->humidity -= 2;
    else
      cond->humidity += number(-8, 8);
    if (old_precip > 65)
      cond->precip_rate -= 10;
    if (old_precip < 20)
      cond->precip_rate += 10;
    break;
  case SEASON_STORMY:
    if (cond->humidity < 50)
      cond->humidity += 4;
    else
      cond->humidity += number(-6, 6);
    if (old_precip > 80)
      cond->precip_rate -= 10;
    if (old_precip < 30)
      cond->precip_rate += 10;
    break;
  case SEASON_TORRENT:
    if (cond->humidity < 60)
      cond->humidity += 4;
    else
      cond->humidity += number(-6, 9);
    if (old_precip > 100)
      cond->precip_rate -= 15;
    if (old_precip < 40)
      cond->precip_rate += 15;
    break;
  case SEASON_CONSTANT_PRECIP:
    cond->humidity = 100;
    if (cond->precip_rate < 10)
      cond->precip_rate += number(5, 12);
    break;
  default:
    break;
  }
  cond->humidity = MIN(100, cond->humidity);
  cond->humidity = MAX(0, cond->humidity);

  cond->pressure_change += number(-3, 3);
  cond->pressure_change = MIN(8, cond->pressure_change);
  cond->pressure_change = MAX(-8, cond->pressure_change);
  cond->pressure += cond->pressure_change;
  cond->pressure = MIN(cond->pressure, 1040);
  cond->pressure = MAX(cond->pressure, 960);

  cond->free_energy += cond->pressure_change;

  /* The numbers that follow are truly magic since  */
  /* they have little bearing on reality and are an */
  /* attempt to create a mix of precipitation which */
  /* will seem reasonable for a specified climate   */
  /* without any complicated formulae that could    */
  /* cause a performance hit. To get more specific  */
  /* or exacting would certainly not be "Diku..."   */

  magic =
    ((1240 - cond->pressure) * cond->humidity >> 4) + cond->temp +
    old_precip * 2 + (cond->free_energy - 10000) / 100;

  if (old_precip == 0)
  {
    if (magic > MAGIC_PRECIP_START)
    {
      cond->precip_rate += 1;
      if (cond->temp > 0)
        STWS(zon, "It begins to rain.\r\n");
      else
        STWS(zon, "It starts to snow.\r\n");
    }
    else if (!old_wind && cond->windspeed)
    {
      STWS(zon, "The wind begins to blow.\r\n");
      //STWS(zon, "!!SOUND(wind* L=-1)");
    }
    else if (cond->windspeed - old_wind > 10)
    {
      STWS(zon, "The wind picks up some.\r\n");
      //STWS(zon, "!!SOUND(wind* L=-1)");
    }
    else if (cond->windspeed - old_wind < -10)
    {
      STWS(zon, "The wind calms down a bit.\r\n");
      //STWS(zon, "!!SOUND(wind* L=1 P=100)");    /* a final play to shut off */
    }
    else if (cond->windspeed > 60)
    {
      if (cond->temp > 50)
        STWS(zon,
             "A violent scorching wind blows hard in the face of any poor travellers in the area.\r\n");
      else if (cond->temp > 21)
        STWS(zon, "A hot wind gusts wildly through the area.\r\n");
      else if (cond->temp > 0)
        STWS(zon, "A fierce wind cuts the air like a razor-sharp knife.\r\n");
      else if (cond->temp > -10)
        STWS(zon, "A freezing gale blasts through the area.\r\n");
      else
        STWS(zon, "An icy wind drains the warmth from all in sight.\r\n");
    }
    else if (cond->windspeed > 25)
    {
      if (cond->temp > 50)
        STWS(zon, "A hot, dry breeze blows languidly around.\r\n");
      else if (cond->temp > 22)
        STWS(zon, "A warm pocket of air is rolling through here.\r\n");
      else if (cond->temp > 10)
        STWS(zon, "It's breezy.\r\n");
      else if (cond->temp > 2)
        STWS(zon, "A cool breeze wafts by.\r\n");
      else if (cond->temp > -5)
        STWS(zon, "A slight wind blows a chill into living tissue.\r\n");
      else if (cond->temp > -15)
        STWS(zon,
             "A freezing wind blows gently, but firmly against all obstacles in the area.\r\n");
      else
        STWS(zon,
             "The wind isn't very strong here, but the cold makes it quite noticeable.\r\n");
    }
    else if (cond->temp > 52)
      STWS(zon, "It's hotter than anyone could imagine.\r\n");
    else if (cond->temp > 37)
      STWS(zon,
           "It's really, really hot here.  A slight breeze would really improve things.\r\n");
    else if (cond->temp > 25)
      STWS(zon, "It's hot out here.\r\n");
    else if (cond->temp > 19)
      STWS(zon, "It's nice and warm out.\r\n");
    else if (cond->temp > 9)
      STWS(zon, "It's mild out today.\r\n");
    else if (cond->temp > 1)
      STWS(zon, "It's cool out here.\r\n");
    else if (cond->temp > -5)
      STWS(zon, "It's a bit nippy here.\r\n");
    else if (cond->temp > -20)
      STWS(zon, "It's cold!\r\n");
    else if (cond->temp > -25)
      STWS(zon, "It's really c-c-c-cold!!\r\n");
    else
      STWS(zon,
           "Better get inside - this is too cold for man or -most- beasts.\r\n");
  }
  else if (magic < MAGIC_PRECIP_STOP)
  {
    cond->precip_rate = 0;
    if (old_temp > 0)
      STWS(zon, "The rain stops.\r\n");
    else
      STWS(zon, "It stops snowing.\r\n");
  }
  else
  {
    /* Still precip'ing, update the rate */
    /* Check rain->snow or snow->rain */
    if (cond->free_energy > 10000)
      cond->precip_change += number(-3, 4);
    else
      cond->precip_change += number(-4, 2);
    cond->precip_change = MAX(-10, cond->precip_change);
    cond->precip_change = MIN(10, cond->precip_change);
    cond->precip_rate += cond->precip_change;
    cond->precip_rate = MAX(1, cond->precip_rate);
    cond->precip_rate = MIN(100, cond->precip_rate);
    cond->precip_depth += cond->precip_rate;
/*    cond->precip_depth -= 5; */
    cond->precip_depth = MAX(1, cond->precip_depth);
    cond->free_energy -= cond->precip_rate * 2 - abs(cond->precip_change);

    if (old_temp > 0 && cond->temp <= 0)
      STWS(zon, "The rain turns to snow.\r\n");
    else if (old_temp <= 0 && cond->temp > 0)
      STWS(zon, "The snow turns to a cold rain.\r\n");
    else if (cond->precip_change > 5)
    {
      if (cond->temp > 0)
        STWS(zon, "It rains a bit harder.\r\n");
      else
        STWS(zon, "The snow is coming down faster now.\r\n");
    }
    else if (cond->precip_change < -5)
    {
      if (cond->temp > 0)
        STWS(zon, "The rain is falling less heavily now.\r\n");
      else
        STWS(zon, "The snow has let up a little.\r\n");
    }
    else if (cond->temp > 0)
    {
      if (cond->precip_rate > 80)
      {
        if (cond->windspeed > 80)
          STWS(zon, "There's a hurricane out here!\r\n");
        else if (cond->windspeed > 40)
          STWS(zon,
               "The wind and the rain are nearly too much to handle.\r\n");
        else
          STWS(zon, "It's raining really hard right now.\r\n");
      }
      else if (cond->precip_rate > 50)
      {
        if (cond->windspeed > 60)
        {
          STWS(zon, "What a rainstorm!\r\n");
        }
        else if (cond->windspeed > 30)
          STWS(zon,
               "The wind is lashing this wild rain seemingly straight into your face.\r\n");
        else
          STWS(zon, "It's raining pretty hard.\r\n");
      }
      else if (cond->precip_rate > 30)
      {
        if (cond->windspeed > 50)
          STWS(zon,
               "A respectable rain is being thrashed about by a vicious wind.\r\n");
        else if (cond->windspeed > 25)
        {
          STWS(zon,
               "It's rainy and windy but altogether not too uncomfortable.\r\n");
        }
        else
          STWS(zon, "Hey, it's raining...\r\n");
      }
      else if (cond->precip_rate > 10)
      {
        if (cond->windspeed > 50)
          STWS(zon,
               "The light rain here is nearly unnoticeable compared to the horrendous wind.\r\n");
        else if (cond->windspeed > 24)
          STWS(zon, "A light rain is being driven fiercely by the wind.\r\n");
        else
          STWS(zon, "It's raining lightly.\r\n");
      }
      else if (cond->windspeed > 55)
        STWS(zon,
             "A few drops of rain are falling admidst a fierce windstorm.\r\n");
      else if (cond->windspeed > 30)
        STWS(zon,
             "The wind and a bit of rain hint at the possibility of a storm.\r\n");
      else
        STWS(zon, "A light drizzle is falling here.\r\n");
    }
    else if (cond->precip_rate > 70)
    {
      if (cond->windspeed > 50)
        STWS(zon, "This must be the worst blizzard ever.\r\n");
      else if (cond->windspeed > 25)
        STWS(zon,
             "There's a blizzard out here, making it quite difficult to see.\r\n");
      else
        STWS(zon, "It's snowing very hard.\r\n");
    }
    else if (cond->precip_rate > 40)
    {
      if (cond->windspeed > 60)
        STWS(zon,
             "The heavily falling snow is being whipped up to a frenzy by a ferocious wind.\r\n");
      else if (cond->windspeed > 35)
        STWS(zon,
             "A heavy snow is being blown randomly about by a brisk wind.\r\n");
      else if (cond->windspeed > 18)
        STWS(zon, "Drifts in the snow are being formed by the wind.\r\n");
      else
        STWS(zon, "The snow's coming down pretty fast now.\r\n");
    }
    else if (cond->precip_rate > 19)
    {
      if (cond->windspeed > 70)
        STWS(zon,
             "The snow wouldn't be too bad, except for the awful wind blowing it in every direction.\r\n");
      else if (cond->windspeed > 45)
        STWS(zon, "There's a minor blizzard here, more wind than snow.\r\n");
      else if (cond->windspeed > 12)
        STWS(zon, "Snow is being blown about by a stiff breeze.\r\n");
      else
        STWS(zon, "It is snowing here.\r\n");
    }
    else if (cond->windspeed > 60)
      STWS(zon, "A light snow is being tossed about by a fierce wind.\r\n");
    else if (cond->windspeed > 42)
      STWS(zon,
           "A lightly falling snow is being driven by a strong wind.\r\n");
    else if (cond->windspeed > 18)
      STWS(zon, "A light snow is falling admidst an unsettled wind.\r\n");
    else
      STWS(zon, "It is lightly snowing.\r\n");

    /* Handle celestial objects */
    if (!(clime->flags & NO_SUN_EVER))
    {
      if ((time_info.hour < 6) || (time_info.hour > 18) ||
          (cond->humidity > 90) || (cond->precip_rate > 80))
        cond->flags &= ~SUN_VISIBLE;
      else
        cond->flags |= SUN_VISIBLE;
    }
    if (!(clime->flags & NO_MOON_EVER))
    {
      if (((time_info.hour > 5) && (time_info.hour < 19)) ||
          (cond->humidity > 80) || (cond->precip_rate > 70) ||
          (time_info.day < 3) || (time_info.day > 31))
        cond->flags &= ~MOON_VISIBLE;
      else if (!(cond->flags & MOON_VISIBLE))
      {
        cond->flags |= MOON_VISIBLE;
        if (time_info.day == 17)
          STWS(zon, "The full moon floods the area with light.\r\n");
        else
          STWS(zon,
               "The moon casts a little bit of light on the ground.\r\n");
      }
    }
  }
  calc_light_zone(zon);

  add_event(event_weather_change, 1500 + number(-90, 90), NULL, NULL, NULL, 0, &zon, sizeof(zon));
  //AddEvent(EVENT_SPECIAL, 1500 + number(-90, 90), TRUE, weather_change, current_event->target.t_arg);
}

#undef STWS

void blow_out_torches(void)
{
  P_char   i;
  P_obj    obj;

  for (i = character_list; i; i = i->next)
    if (!IS_SET(world[i->in_room].room_flags, INDOORS) &&
        i->equipment[WEAR_LIGHT])
    {
      obj = i->equipment[WEAR_LIGHT];
      if (obj->value[3])
      {
        if ((obj->value[3] <
             sector_table[in_weather_sector(i->in_room)].conditions.windspeed)
            && number(0, 1))
        {
          act("$p goes out", TRUE, i, obj, NULL, TO_ROOM);
          act("Your $p goes out and you put it away.", TRUE, i, obj, NULL,
              TO_CHAR);
          unequip_char(i, WEAR_LIGHT);
          obj_to_char(obj, i);
        }
      }
    }
}

void calc_light_zone(int zon)
{
  char     light_sum = 0, temp, temp2;
  struct sector_data *t_zone = &sector_table[zon];

  if (!(t_zone->climate.flags & NO_SUN_EVER))
  {
    temp = time_info.hour;
    if (temp > 11)
      temp = 23 - temp;
    temp -= 5;
    if (temp > 0)
    {
      t_zone->conditions.flags |= SUN_VISIBLE;
      light_sum += temp * 10;
    }
    else
      t_zone->conditions.flags &= ~SUN_VISIBLE;
  }
  if (!(t_zone->climate.flags & NO_MOON_EVER))
  {
    temp = abs(time_info.hour - 12);
    temp -= 7;
    if (temp > 0)
    {
      temp2 = 17 - abs(time_info.day - 17);
      temp2 -= 3;
      if (temp2 > 0)
      {
        t_zone->conditions.flags |= MOON_VISIBLE;
        light_sum += temp * temp2 / 2;
      }
      else
        t_zone->conditions.flags &= ~MOON_VISIBLE;
    }
  }
  light_sum -= t_zone->conditions.precip_rate;
  light_sum = BOUNDED(0, light_sum, 100);
  t_zone->conditions.ambient_light = light_sum;
}

char get_season(int sector)
{
  char     season_num;

  season_num = (time_info.month < 5) ? 0 :
    (time_info.month < 9) ? 1 : (time_info.month < 13) ? 2 : 3;

  return (season_num);
}

  /* accepts real room only, returns -1 to 99 */

int in_weather_sector(int room)
{
  int      temp, mod, mod2, sector, element1, element2, element3;

  if (room > top_of_world || room < 0)
    return -1;

  if (!IS_MAP_ROOM(room))
    room = maproom_of_zone(world[room].zone);
  else
    room = world[room].number;
  if (room == NOWHERE)
    return 0;

  /* weather sectors are 30x30 squares, thus we have 10x10 = 100 of them */
  /* number them same way map rooms are, 0-9, drop a line, 10-19 */

  /* seperate the 6 digits into groups of 2 */
  element1 = (room / 10000);
  element2 = (room - (element1 * 10000)) / 100;
  element3 = (room - (element1 * 10000) - (element2 * 100));

  /* screwy math below, be forewarned */
  switch (element1)
  {
  case 11:
    mod = 0;
    mod2 = 0;
    break;
  case 12:
    mod = 0;
    mod2 = 3;
    break;
  case 13:
    mod = 0;
    mod2 = 6;
    break;
  case 14:
    mod = 3;
    mod2 = 0;
    break;
  case 15:
    mod = 3;
    mod2 = 3;
    break;
  case 16:
    mod = 3;
    mod2 = 6;
    break;
  case 17:
    mod = 6;
    mod2 = 0;
    break;
  case 18:
    mod = 6;
    mod2 = 3;
    break;
  case 19:
    mod = 6;
    mod2 = 6;
    break;
  }

  temp = (int) ((element2 / 30 + mod) * 10);    /* 1st# of sector */
  sector = (int) (element3 / 30 + mod2 + temp); /* 2nd# of sector */

  return BOUNDED(0, sector, 99);
}

void send_to_weather_sector(int z, const char *msg)
{
  P_desc   i;

  if (z < 0)
    return;
  if (z > 99)
    return;
  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && (in_weather_sector(i->character->in_room) == z)
        && NORMAL_PLANE(i->character->in_room) && OUTSIDE(i->character)
        && !IS_SET(world[i->character->in_room].room_flags, NO_PRECIP)
        && !IS_SET(world[i->character->in_room].room_flags, DARK)
        && i->character->specials.z_cord >= 0)
    {
      send_to_char(msg, i->character);
    }
}
