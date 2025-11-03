
/*********************************************************************
 * Statistics.c handles all game statistic information logging       *
 * Kvark 2002-11-09						     *	
 *********************************************************************/



#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "sql.h"

extern int abs(int);

int      show_moth_stati(P_char ch, char *stati_date, char *argument);

/*
   external vars 
 */


extern P_char character_list;
extern P_event current_event;
extern P_room world;
extern int top_of_zone_table;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern P_desc descriptor_list;

void event_write_statistic(P_char ch, P_char victim, P_obj obj, void *data)
{
  P_desc   d;
  P_char   t_ch;
  char     fname[256];
  char     mdate[15];
  int      goodies = 0;
  int      evils = 0;
  int      undeads = 0;
  int      illithids = 0;
  int      gods = 0;
  int      inhalls = 0;
  int      goodies_lvl = 0;
  int      evils_lvl = 0;
  int      illithids_lvl = 0;
  int      undeads_lvl = 0;
  int      unique_ips = 0;
  vector<const char *> seen_ips;

  long     ct;
  char    *tmstr;
  struct tm *lt;
  FILE    *f;
  char    *buf = 0;


  ct = time(0);
  lt = localtime(&ct);
//tmstr = asctime(lt);
  strftime(mdate, 10, "%Y%m%d", lt);

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->character)
      t_ch = d->character;
    else
      t_ch = NULL;

    if (d->connected != CON_PLAYING)
      continue;


    if (t_ch && !IS_TRUSTED(t_ch))
    {
      if (world[t_ch->in_room].zone == 50)
        inhalls++;

      if (IS_RACEWAR_GOOD(t_ch))
      {
        goodies++;
        goodies_lvl = goodies_lvl + GET_LEVEL(t_ch);
      }
      else if (IS_RACEWAR_EVIL(t_ch) && !IS_ILLITHID(t_ch))
      {
        evils++;
        evils_lvl = evils_lvl + GET_LEVEL(t_ch);
      }
      else if (IS_RACEWAR_UNDEAD(t_ch))
      {
        undeads++;
        undeads_lvl = undeads_lvl + GET_LEVEL(t_ch);
      }
      else if IS_ILLITHID
        (t_ch)
      {
        illithids++;
        illithids_lvl = illithids_lvl + GET_LEVEL(t_ch);
      }

      vector<const char *>::iterator it;
      for( it = seen_ips.begin(); it != seen_ips.end(); it++ )
      {
        if (!strcmp(d->host, *it))
          break;
      }
      if (it == seen_ips.end())
      {
        seen_ips.push_back(d->host);
        unique_ips++;
      }
    }
    else if (t_ch && IS_TRUSTED(t_ch))
      gods++;
  }
  snprintf(fname, 256, "lib/statistics/statistics_general%s", mdate);
  f = fopen(fname, "a");
  if (!f)
    return;

  strftime(mdate, 10, "%H", lt);

  fprintf(f, "%s %d %d %d %d %d %d %d %d %d %d %d\r\n", mdate, goodies, evils, illithids, undeads, gods, inhalls, goodies_lvl, evils_lvl, undeads_lvl, illithids_lvl, unique_ips);
  fclose(f);

#ifndef __NO_MYSQL__
    qry("INSERT INTO statistics (" \
        "date, goods_count, evils_count, illithids_count, undeads_count, gods_count, in_guildhall_count, sum_goods_levels, sum_evils_levels, sum_undeads_levels, sum_illithids_levels, unique_ips_count) " \
        "VALUES (unix_timestamp(), %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
         goodies, evils, illithids, undeads, gods, inhalls, goodies_lvl, evils_lvl, undeads_lvl, illithids_lvl, unique_ips
    );
#endif

  add_event(event_write_statistic, PULSES_IN_TICK, NULL, NULL, NULL, 0, NULL, 0);
  //AddEvent(EVENT_SPECIAL, 500, TRUE, write_statistic, NULL);
}


struct statistics_day
{

  int      hour;
  float    goodies;
  float    total_goodies;
  float    evils;
  float    total_evils;
  float    illithids;
  float    total_illithids;
  float    undeads;
  float    total_undeads;
  float    inhalls;
  float    total_inhalls;
  float    gods;
  float    total_gods;
  float    total;
  float    total_total;
  float    howmany;
  float    goodies_lvl;
  float    total_goodie_lvl;
  float    evils_lvl;
  float    total_evils_lvl;
  float    undeads_lvl;
  float    total_undeads_lvl;
  float    illithids_lvl;
  float    total_illithids_lvl;
};


void do_statistic(P_char ch, char *argument, int val)
{
  char     Gbuf0[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  char     Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];
  char     Gbuf4[MAX_STRING_LENGTH];
  char     Gbuf5[MAX_STRING_LENGTH];

  char     buf2[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH];
  char     fname[256];
  int      goodies = 0;
  int      evils = 0;
  int      illithids = 0;
  int      undeads = 0;
  int      gods = 0;
  int      inhalls = 0;
  int      goodies_lvl = 0;
  int      evils_lvl = 0;
  int      illithids_lvl = 0;
  int      undeads_lvl = 0;
  int      unique_ips = 0;
  int      i = 0;
  int      j = 0;
  int      hour = 0;
  struct statistics_day day[24];
  int      x, args;
  long     ct;
  char    *tmstr;
  struct tm *lt;
  char     mdate[15];
  FILE    *f;
  int      k = 0;

  ct = time(0);
  lt = localtime(&ct);
//tmstr = asctime(lt);
//create_zone(0);


  strftime(mdate, 10, "%Y%m%d", lt);

  memset(day, 0, sizeof(day));
  argument = one_argument(argument, Gbuf0);
  argument_interpreter(argument, Gbuf1, Gbuf2);
// argument = one_argument(argument, Gbuf2); 
  if (!*Gbuf0 || !*Gbuf1 || !*Gbuf2)
  {
    send_to_char
      ("Syntax is:stati day <date> <all | goodies | evils | undeads | gods | illithids | inhalls | crash | level>.\r\n&+WExample: \"&+Ystati day 20021109 evils&+W\"&n\r\n",
       ch);

    send_to_char
      ("Syntax is:stati month <date> <all | goodies | evils | undeads | gods | illithids | inhalls | crash | level>.\r\n&+WExample: \"&+Ystati month 20021109 evils&+W\"&n\r\n",
       ch);
    send_to_char("&+WTodays date:&+Y ", ch);
    send_to_char(mdate, ch);
    send_to_char("&n\r\n", ch);

    return;
  }



  if (!strcmp(Gbuf0, "month"))
  {
    Gbuf1[strlen(Gbuf1) - 2] = '\0';
    strcat(Gbuf1, "01");
    show_moth_stati(ch, Gbuf1, Gbuf2);
    return;
  }

  snprintf(fname, 256, "lib/statistics/statistics_general%s", Gbuf1);
  f = fopen(fname, "r");
  if (!f)
  {
    send_to_char("No information about that date", ch);
    return;
  }
  while (!(feof(f)))
  {
    if (fgets(buf2, sizeof(buf2) - 1, f))
    {
      i++;
      if (sscanf
          (buf2, "%d %d %d %d %d %d %d %d %d %d %d %d", &hour, &goodies, &evils,
           &illithids, &undeads, &gods, &inhalls, &goodies_lvl, &evils_lvl,
           &undeads_lvl, &illithids_lvl, &unique_ips) == 12)
      {
        day[hour].hour = hour;
        day[hour].goodies = day[hour].goodies + goodies;
        day[hour].evils = day[hour].evils + evils;
        day[hour].illithids = day[hour].illithids + illithids;
        day[hour].undeads = day[hour].undeads + undeads;
        day[hour].gods = day[hour].gods + gods;
        day[hour].inhalls = day[hour].inhalls + inhalls;
        day[hour].total =
          day[hour].total + goodies + evils + undeads + illithids + gods;
        day[hour].howmany = day[hour].howmany + 1;
        day[hour].goodies_lvl = day[hour].goodies_lvl + goodies_lvl;
        day[hour].evils_lvl = day[hour].evils_lvl + evils_lvl;
        day[hour].undeads_lvl = day[hour].undeads_lvl + undeads_lvl;
        day[hour].illithids_lvl = day[hour].illithids_lvl + illithids_lvl;
      }
    }
  }
  fclose(f);

  if (i > 1)
  {
    send_to_char("&+Y\t\t- ", ch);
    send_to_char(Gbuf1, ch);
    send_to_char("\t", ch);
    send_to_char(Gbuf2, ch);
    send_to_char(" -&n\r\n", ch);
    buf[0] = '\0';
    while (j < 24)
    {
      if (day[j].howmany > 1)
      {
        day[j].goodies = (float) (day[j].goodies / day[j].howmany);
        day[j].evils = (float) (day[j].evils / day[j].howmany);
        day[j].illithids = (float) (day[j].illithids / day[j].howmany);
        day[j].undeads = (float) (day[j].undeads / day[j].howmany);
        day[j].gods = (float) (day[j].gods / day[j].howmany);
        day[j].inhalls = (float) (day[j].inhalls / day[j].howmany);
        day[j].total = (float) (day[j].total / day[j].howmany);

        Gbuf5[0] = '\0';

        if (!strcmp(Gbuf2, "goodies") || !strcmp(Gbuf2, "all"))
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
          i = 0;

          while (i < day[j].goodies)
          {
            strcat(Gbuf3, "-");
            i = i + 2;
          }
          snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%d:00\t%s> %.1f Goodies", j, Gbuf3,
                  day[j].goodies);
          //snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%d:00\t%s> %.1f",j, Gbuf3,  day[j].goodies);
          strcat(Gbuf5, buf);
        }
        if (!strcmp(Gbuf2, "evils") || !strcmp(Gbuf2, "all"))
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
          i = 0;

          while (i < day[j].evils)
          {
            strcat(Gbuf3, "-");
            i = i + 2;
          }
          snprintf(buf, MAX_STRING_LENGTH, "\r\n&+r%d:00\t%s> %.1f Evils", j, Gbuf3,
                  day[j].evils);
          //snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%d:00\t%s> %.1f",j, Gbuf3,  day[j].evils);
          strcat(Gbuf5, buf);

        }
        if (!strcmp(Gbuf2, "undeads") || !strcmp(Gbuf2, "all"))
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
          i = 0;

          while (i < day[j].undeads)
          {
            strcat(Gbuf3, "-");
            i = i + 2;
          }
          snprintf(buf, MAX_STRING_LENGTH, "\r\n&+L%d:00\t%s> %.1f Undeads", j, Gbuf3,
                  day[j].undeads);
          //snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%d:00\t%s> %.1f",j, Gbuf3,  day[j].undeads);
          strcat(Gbuf5, buf);

        }
        if (!strcmp(Gbuf2, "illithids") || !strcmp(Gbuf2, "all"))
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
          i = 0;

          while (i < day[j].illithids)
          {
            strcat(Gbuf3, "-");
            i = i + 2;
          }
          snprintf(buf, MAX_STRING_LENGTH, "\r\n&+M%d:00\t%s> %.1f Illithids", j, Gbuf3,
                  day[j].illithids);
          //snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%d:00\t%s> %.1f",j, Gbuf3,  day[j].illithids);
          strcat(Gbuf5, buf);

        }

        if (!strcmp(Gbuf2, "total") || !strcmp(Gbuf2, "all"))
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
          i = 0;

          while (i < day[j].total)
          {
            strcat(Gbuf3, "-");
            i = i + 2;
          }
          snprintf(buf, MAX_STRING_LENGTH, "\r\n&+G%d:00\t%s> %.1f Total", j, Gbuf3,
                  day[j].total);
          //snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%d:00\t%s> %.1f",j, Gbuf3,  day[j].total);
          strcat(Gbuf5, buf);

        }
        if (!strcmp(Gbuf2, "inhalls") || !strcmp(Gbuf2, "all"))
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
          i = 0;

          while (i < day[j].inhalls)
          {
            strcat(Gbuf3, "-");
            i = i + 2;
          }
          snprintf(buf, MAX_STRING_LENGTH, "\r\n&+B%d:00\t%s> %.1f Inhalls", j, Gbuf3,
                  day[j].inhalls);
          //snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%d:00\t%s> %.1f",j, Gbuf3,  day[j].inhalls);
          strcat(Gbuf5, buf);
        }

        if (!strcmp(Gbuf2, "gods") || !strcmp(Gbuf2, "all"))
        {
          snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
          i = 0;

          while (i < day[j].gods)
          {
            strcat(Gbuf3, "-");
            i = i + 2;
          }
          snprintf(buf, MAX_STRING_LENGTH, "\r\n&+Y%d:00\t%s> %.1f Gods", j, Gbuf3, day[j].gods);
          strcat(Gbuf5, buf);
        }


        if (!strcmp(Gbuf2, "level"))
        {

          if (day[j].total)
          {
            if (day[j].goodies_lvl)
              day[j].goodies_lvl =
                (float) (day[j].goodies_lvl / day[j].howmany /
                         day[j].goodies);
            if (day[j].evils_lvl)
              day[j].evils_lvl =
                (float) (day[j].evils_lvl / day[j].howmany / day[j].evils);
            if (day[j].undeads_lvl)
              day[j].undeads_lvl =
                (float) (day[j].undeads_lvl / day[j].howmany /
                         day[j].undeads);
            if (day[j].illithids_lvl)
              day[j].illithids_lvl =
                (float) (day[j].illithids_lvl / day[j].howmany /
                         day[j].illithids);

            Gbuf3[0] = '\0';
            i = 0;
            while (i < day[j].goodies_lvl)
            {
              strcat(Gbuf3, "-");
              i = i + 2;
            }
            snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%d:00\t%s> %.1f Goodies averge lvl", j,
                    Gbuf3, day[j].goodies_lvl);
            strcat(Gbuf5, buf);

            Gbuf3[0] = '\0';
            i = 0;
            while (i < day[j].evils_lvl)
            {
              strcat(Gbuf3, "-");
              i = i + 2;
            }
            snprintf(buf, MAX_STRING_LENGTH, "\r\n&+r%d:00\t%s> %.1f Evil averge lvl", j, Gbuf3,
                    day[j].evils_lvl);
            strcat(Gbuf5, buf);

            Gbuf3[0] = '\0';
            i = 0;
            while (i < day[j].undeads_lvl)
            {
              strcat(Gbuf3, "-");
              i = i + 2;
            }
            snprintf(buf, MAX_STRING_LENGTH, "\r\n&+L%d:00\t%s> %.1f Undeads averge lvl", j,
                    Gbuf3, day[j].undeads_lvl);
            strcat(Gbuf5, buf);

            Gbuf3[0] = '\0';
            i = 0;
            while (i < day[j].illithids_lvl)
            {
              strcat(Gbuf3, "-");
              i = i + 2;
            }
          }
          snprintf(buf, MAX_STRING_LENGTH, "\r\n&+M%d:00\t%s> %.1f Illithids averge lvl", j,
                  Gbuf3, day[j].illithids_lvl);
          strcat(Gbuf5, buf);
        }


        send_to_char(Gbuf5, ch);

      }
      else
      {
        snprintf(Gbuf4, MAX_STRING_LENGTH, "\r\n&+C%d:00\t> 0.0 Mud Down", j);
        send_to_char(Gbuf4, ch);
      }
      j++;
    }
  }
}


int show_moth_stati(P_char ch, char *stati_date, char *argument)
{
  char     Gbuf0[MAX_STRING_LENGTH];
  char     Gbuf1[MAX_STRING_LENGTH];
  char     Gbuf2[MAX_STRING_LENGTH];
  char     Gbuf3[MAX_STRING_LENGTH];
  char     Gbuf4[MAX_STRING_LENGTH];
  char     Gbuf5[MAX_STRING_LENGTH];

  char     buf2[MAX_STRING_LENGTH];
  char     buf[MAX_STRING_LENGTH * 30];
  char     fname[256];
  int      goodies = 0;
  int      evils = 0;
  int      illithids = 0;
  int      undeads = 0;
  int      gods = 0;
  int      inhalls = 0;
  int      goodies_lvl = 0;
  int      evils_lvl = 0;
  int      illithids_lvl = 0;
  int      undeads_lvl = 0;
  int      i = 0;
  int      j = 1;
  int      g = 0;
  int      hour = 0;
  struct statistics_day day[24];
  struct statistics_day month[31];
  int      x, args;
  long     ct;
  char    *tmstr;
  struct tm *lt;
  char     mdate[15];
  FILE    *f;
  int      k = 0;

  ct = time(0);
  lt = localtime(&ct);
//tmstr = asctime(lt);
//create_zone(0);
  memset(day, 0, sizeof(day));
  memset(month, 0, sizeof(month));
  Gbuf4[0] = '\0';

  send_to_char("\t\t\t&+W", ch);

  send_to_char(stati_date, ch);
  send_to_char(" ", ch);

  send_to_char(argument, ch);
  send_to_char("&n\r\n", ch);


  while (i < 31)
  {
    memset(day, 0, sizeof(day));

    i++;
    stati_date[strlen(stati_date) - 2] = '\0';
    if (i < 10)
      snprintf(Gbuf0, MAX_STRING_LENGTH, "0%d", i);
    else
      snprintf(Gbuf0, MAX_STRING_LENGTH, "%2d", i);
    strcat(stati_date, Gbuf0);
    snprintf(fname, 256, "lib/statistics/statistics_general%s", stati_date);
    f = fopen(fname, "r");
    if (!f)
    {
      //send_to_char("No information about that date", ch);
      continue;
    }
    while (!(feof(f)))
    {
      if (fgets(buf2, sizeof(buf2) - 1, f))
      {
        if (sscanf
            (buf2, "%d %d %d %d %d %d %d %d %d %d %d", &hour, &goodies,
             &evils, &illithids, &undeads, &gods, &inhalls, &goodies_lvl,
             &evils_lvl, &undeads_lvl, &illithids_lvl) == 11)
        {
          day[hour].hour = hour;
          day[hour].goodies = day[hour].goodies + goodies;
          day[hour].evils = day[hour].evils + evils;
          day[hour].illithids = day[hour].illithids + illithids;
          day[hour].undeads = day[hour].undeads + undeads;
          day[hour].gods = day[hour].gods + gods;
          day[hour].inhalls = day[hour].inhalls + inhalls;
          day[hour].total =
            day[hour].total + goodies + evils + undeads + illithids + gods;
          day[hour].howmany = day[hour].howmany + 1;
          day[hour].goodies_lvl = day[hour].goodies_lvl + goodies_lvl;
          day[hour].evils_lvl = day[hour].evils_lvl + evils_lvl;
          day[hour].undeads_lvl = day[hour].undeads_lvl + undeads_lvl;
          day[hour].illithids_lvl = day[hour].illithids_lvl + illithids_lvl;
        }
      }
    }
    fclose(f);

    j = 0;
    k = 0;
    while (j < 24)
    {
      if (day[j].howmany > 1)
      {
        k++;
        month[i].total_goodies =
          month[i].total_goodies + day[j].goodies / day[j].howmany;
        month[i].total_evils =
          month[i].total_evils + day[j].evils / day[j].howmany;
        month[i].total_undeads =
          month[i].total_undeads + day[j].undeads / day[j].howmany;
        month[i].total_illithids =
          month[i].total_illithids + day[j].illithids / day[j].howmany;

        month[i].total_total =
          month[i].total_total + day[j].total / day[j].howmany;
        month[i].total_inhalls =
          month[i].total_inhalls + day[j].inhalls / day[j].howmany;
        month[i].total_gods =
          month[i].total_gods + day[j].gods / day[j].howmany;
      }
      j++;

    }
    if (k > 1)
    {
      //     snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%s> %.1f Goodies", stati_date,  month[i].total_goodies / k);
      //GOOD
      if (!strcmp(argument, "goodies") || !strcmp(argument, "all"))
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
        g = 0;

        while (g < ((int) (month[i].total_goodies / k)))
        {
          strcat(Gbuf3, "-");
          g = g + 2;
        }

        snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%s\t%s> %.1f Goodies&n", stati_date, Gbuf3,
                month[i].total_goodies / k);
        strcat(Gbuf4, buf);
      }

      //EVILS
      if (!strcmp(argument, "evils") || !strcmp(argument, "all"))
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
        g = 0;

        while (g < ((int) (month[i].total_evils / k)))
        {
          strcat(Gbuf3, "-");
          g = g + 2;
        }
        snprintf(buf, MAX_STRING_LENGTH, "\r\n&+r%s\t%s> %.1f Evils&n", stati_date, Gbuf3,
                month[i].total_evils / k);
        strcat(Gbuf4, buf);
      }

      //UNDEADS
      if (!strcmp(argument, "undeads") || !strcmp(argument, "all"))
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
        g = 0;

        while (g < (month[i].total_undeads / k))
        {
          strcat(Gbuf3, "-");
          g = g + 2;
        }
        snprintf(buf, MAX_STRING_LENGTH, "\r\n&+L%s\t%s> %.1f Undeads&n", stati_date, Gbuf3,
                month[i].total_undeads / k);
        strcat(Gbuf4, buf);
      }

      //Illithids
      if (!strcmp(argument, "illithids") || !strcmp(argument, "all"))
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
        g = 0;

        while (g < (month[i].total_illithids / k))
        {
          strcat(Gbuf3, "-");
          g = g + 2;
        }
        snprintf(buf, MAX_STRING_LENGTH, "\r\n&+M%s\t%s> %.1f Illithids&n", stati_date, Gbuf3,
                month[i].total_illithids / k);
        strcat(Gbuf4, buf);
      }

      //Total

      if (!strcmp(argument, "total") || !strcmp(argument, "all"))
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
        g = 0;
        while (g < (month[i].total_total / k))
        {
          strcat(Gbuf3, "-");
          g = g + 2;
        }
        snprintf(buf, MAX_STRING_LENGTH, "\r\n&+G%s\t%s> %.1f Total&n", stati_date, Gbuf3,
                month[i].total_total / k);
        strcat(Gbuf4, buf);
      }
      //Inhalls
      if (!strcmp(argument, "inhalls") || !strcmp(argument, "all"))
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
        g = 0;
        while (g < (month[i].total_inhalls / k))
        {
          strcat(Gbuf3, "-");
          g = g + 2;
        }
        snprintf(buf, MAX_STRING_LENGTH, "\r\n&+B%s\t%s> %.1f Inhalls&n", stati_date, Gbuf3,
                month[i].total_inhalls / k);
        strcat(Gbuf4, buf);
      }
      //Gods
      if (!strcmp(argument, "gods") || !strcmp(argument, "all"))
      {
        snprintf(Gbuf3, MAX_STRING_LENGTH, "-");
        g = 0;
        while (g < (month[i].total_gods / k))
        {
          strcat(Gbuf3, "-");
          g = g + 2;
        }
        snprintf(buf, MAX_STRING_LENGTH, "\r\n&+Y%s\t%s> %.1f Gods&n", stati_date, Gbuf3,
                month[i].total_gods / k);
        strcat(Gbuf4, buf);
      }








    }
    else
    {
      snprintf(buf, MAX_STRING_LENGTH, "\r\n&+W%s>Crash!", stati_date);
    }

  }

  send_to_char(Gbuf4, ch);

  return 0;
}
