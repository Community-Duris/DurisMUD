/****************************************************************************
 *
 *  File: hardcore.c                                           Part of Duris
 *  Usage: Hardcore chars related materia.
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark 			Date: 2002-04-18
 * ***************************************************************************
 */

#define TROPHY

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "mm.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "weather.h"
#include "sound.h"
#include "ships.h"


/*
 * external variables
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_event event_type_list[];
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern char debug_mode;
extern const char *race_types[];
extern const int exp_table[];

//extern const int material_absorbtion[][];
extern const struct stat_data stat_factor[];
extern float fake_sqrt_table[];
extern int pulse;
extern int arena_hometown_location[];
extern struct arena_data arena;
extern struct agi_app_type agi_app[];
extern struct dex_app_type dex_app[];
extern struct message_list fight_messages[];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;

#define MAX_HALLOFFAME_SIZE    20       /* max size of high/low lists */
#define MAX_LEADERBOARD_SIZE    20       /* max size of high/low lists */
int getHardCorePts(P_char ch)
{
  
 int hardcorepts =
       (GET_LEVEL(ch) * 1000) + (ch->points.curr_exp / 10000) +
             (ch->only.pc->frags * 100);

 if(IS_MULTICLASS_NPC(ch))
   hardcorepts = hardcorepts * 2;

 return hardcorepts;         
}
void writeHallOfFame(P_char ch, char thekiller[1024])
{
  FILE    *halloffamelist;
  char     highPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
    lowPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH], change = FALSE;
  int      highHardcore[MAX_HALLOFFAME_SIZE],
    lowHardcore[MAX_HALLOFFAME_SIZE], phalloffames, i;
  char     killerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH];
  char     halloffamelist_file[1024];
  char     buffer[1024], *ptr;

  if (!ch)
    return;

  sprintf(halloffamelist_file, "lib/information/hardcore_hall_of_fame");

  ptr = halloffamelist_file;
  for (ptr = halloffamelist_file; *ptr != '\0'; ptr++)
  {
    *ptr = LOWER(*ptr);
    if (*ptr == ' ')
      *ptr = '_';
  }

  //halloffamelist_file[0] = 'F';

  halloffamelist = fopen(halloffamelist_file, "rt");

  if (!halloffamelist)
  {
    sprintf(buffer, "Couldn't open halloffamelist: %s\r\n",
            halloffamelist_file);
    send_to_char(buffer, ch);
    return;
  }

  if (isname(thekiller, "NotDead"))
    phalloffames = getHardCorePts(ch);
  else
    phalloffames =
      getHardCorePts(ch) + 100;
  *thekiller = toupper(*thekiller);
  /* read ten highest */
 
   for (i = 0; i < MAX_HALLOFFAME_SIZE; i++)
  {
    if (feof(halloffamelist))
    {
     // send_to_char("error: halloffame list terminated prematurely.\r\n", ch);
			fclose(halloffamelist);
      return;
    }

    fscanf(halloffamelist, "%s %d %s\n", highPlayerName[i], &highHardcore[i],
           killerName[i]);
  }

  fclose(halloffamelist);

/* check if player already has an entry and is higher than somebody else
    (including his previous entry) - if so, delete it.  if they end up at
    the end of the list (higher than nobody after deleted), stick em there
    here */

  for (i = 0; i < MAX_HALLOFFAME_SIZE; i++)
  {
    // check for dupe entry - just delete it if it exists.  let's see what
    // happens, shall we?

    if (!str_cmp(ch->player.name, highPlayerName[i]))
    {
      deleteHallEntry(highPlayerName, highHardcore, i, killerName);

      break;
    }
  }

  /* see if player has beaten anybody currently on the list */

  for (i = 0; (i < MAX_HALLOFFAME_SIZE); i++)
  {
    if (phalloffames > highHardcore[i])
    {
      insertHallEntry(highPlayerName, highHardcore, ch->player.name,
                      phalloffames, i, killerName, thekiller);


      change = TRUE;
      break;
    }
  }

  if (change)
  {
    halloffamelist = fopen(halloffamelist_file, "wt");
    if (!halloffamelist)
    {
      send_to_char("error: couldn't open halloffamelist for writing.\r\n",
                   ch);
      return;
    }

    for (i = 0; i < MAX_HALLOFFAME_SIZE; i++)
    {
      fprintf(halloffamelist, "%s %d %s\n", highPlayerName[i],
              highHardcore[i], killerName[i]);
    }

    fclose(halloffamelist);
  }



}

void deleteHallEntry(char names[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
                     int halloffames[MAX_HALLOFFAME_SIZE], int pos,
                     char killer[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH])
{
  int      i;

  if (pos >= MAX_HALLOFFAME_SIZE)
    return;

  for (i = pos; i < MAX_HALLOFFAME_SIZE; i++)
  {
    strcpy(names[i], names[i + 1]);
    halloffames[i] = halloffames[i + 1];
    strcpy(killer[i], killer[i + 1]);
  }

  strcpy(names[MAX_HALLOFFAME_SIZE - 1], "Nobody");
  halloffames[MAX_HALLOFFAME_SIZE - 1] = 0;
  strcpy(killer[MAX_HALLOFFAME_SIZE - 1], "NotDead");
}

void insertHallEntry(char names[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
                     int halloffames[MAX_HALLOFFAME_SIZE], char *name,
                     int newHardcore, int pos,
                     char killers[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
                     char *killer)
{
  int      i;


  if (pos >= MAX_HALLOFFAME_SIZE)
    return;

  if (pos == (MAX_HALLOFFAME_SIZE - 1))
  {
    strcpy(names[pos], name);
    halloffames[pos] = newHardcore;
    strcpy(killers[pos], killer);
    return;
  }

  for (i = MAX_HALLOFFAME_SIZE - 2; i >= pos; i--)
  {
    strcpy(names[i + 1], names[i]);
    halloffames[i + 1] = halloffames[i];
    strcpy(killers[i + 1], killers[i]);
  }

  strcpy(names[pos], name);
  halloffames[pos] = newHardcore;
  strcpy(killers[pos], killer);
}

void displayHardCore(P_char ch, char *arg, int cmd)
{

  FILE    *halloffameList;
  char     name[MAX_STRING_LENGTH], buf[65536], buf2[2048], tempbuf[2048];
  char     killer[MAX_STRING_LENGTH];
  int      halloffames;
  char     i;
  float    pts = 0;
  char     filename[1024];

  sprintf(filename, "lib/information/hardcore_hall_of_fame");
  //sprintf(filename, "lib/information/leaderboard");

  if (!(halloffameList = fopen(filename, "rt")))
  {
   /* sprintf(name, "Couldn't open halloffamelist: %s\r\n", filename);
    send_to_char(name, ch); */
    return;
  }

  strcpy(buf, "\t\r\n&+r-= &+LHall Of&+L Fame&+r =-&n\r\n\r\n");
  sprintf(tempbuf, "   &+w%-15s           &+w%s           &+w%-15s\r\n",
          "Name", "Points", "Deaths/Killed by");
  strcat(buf, tempbuf);
  for (i = 0; i < MAX_HALLOFFAME_SIZE; i++)
  {
    fscanf(halloffameList, "%s %d %s\n", name, &halloffames, killer);
    name[0] = toupper(name[0]);
    pts = halloffames;
    pts /= 100.0;

    sprintf(buf2, "   &+L%-15s          &+r% 6.2f\t      &+W%-15s\r\n",
            name, pts, killer);
    strcat(buf, buf2);
  }


  fclose(halloffameList);

  strcat(buf, "\r\n");

  page_string(ch->desc, buf, 1);
}



void checkHallOfFame(P_char ch, char killer[1024])
{
  char     arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];

  if (!ch)
    return;
  if (!killer)
    return;

  argument_split_2(killer, arg1, arg2);
  writeHallOfFame(ch, arg1);
  return;
}


long getLeaderBoardPts(P_char ch)
{
 if(!IS_PC(ch))
 return 0;

 update_shipfrags();
 int sf = calculate_shipfrags(ch); 

 //debug("ch: %s, shipfrags: %d, levelpoints: %d, exppoints: %d, fragpoints: %d, deathpoints: %d\r\n", GET_NAME(ch), sf, (GET_LEVEL(ch) * 1000), (ch->points.curr_exp / 10000), (ch->only.pc->frags * 100), (ch->only.pc->numb_deaths * 25));

 long hardcorepts =
       (GET_LEVEL(ch) * 1000) + (ch->points.curr_exp / 10000) + (sf * 100) +
             (ch->only.pc->frags * 100) - (ch->only.pc->numb_deaths * 25);
  /*
 if(IS_MULTICLASS_NPC(ch))
   hardcorepts = hardcorepts * 2;
  */ 

 return hardcorepts;         
}

void deleteLeaderEntry(char names[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
                     int halloffames[MAX_HALLOFFAME_SIZE], int pos,
                     char killer[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH])
{
  int      i;

  if (pos >= 100)
    return;

  for (i = pos; i < MAX_HALLOFFAME_SIZE; i++)
  {
    strcpy(names[i], names[i + 1]);
    halloffames[i] = halloffames[i + 1];
    strcpy(killer[i], killer[i + 1]);
  }
/*
  strcpy(names[MAX_HALLOFFAME_SIZE - 1], "Nobody");
  halloffames[MAX_HALLOFFAME_SIZE - 1] = 0;
  strcpy(killer[MAX_HALLOFFAME_SIZE - 1], "NotDead");
*/
}

void insertLeaderEntry(char names[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
                     int halloffames[MAX_HALLOFFAME_SIZE], char *name,
                     int newHardcore, int pos,
                     char killers[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
                     char *killer)
{
  int      i;


  if (pos >= MAX_HALLOFFAME_SIZE)
    return;

  if (pos == (MAX_HALLOFFAME_SIZE - 1))
  {
    strcpy(names[pos], name);
    halloffames[pos] = newHardcore;
    strcpy(killers[pos], killer);
    return;
  }

  for (i = MAX_HALLOFFAME_SIZE - 2; i >= pos; i--)
  {
    strcpy(names[i + 1], names[i]);
    halloffames[i + 1] = halloffames[i];
    strcpy(killers[i + 1], killers[i]);
  }

  strcpy(names[pos], name);
  halloffames[pos] = newHardcore;
  strcpy(killers[pos], killer);
}

 void newLeaderBoard(P_char ch, char *arg, int cmd)
{
  FILE    *halloffamelist, *f, *newleaderlist;
  char     highPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
    lowPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH], change = FALSE;
	  char     killer[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];
  int      highHardcore[MAX_HALLOFFAME_SIZE],
    lowHardcore[MAX_HALLOFFAME_SIZE], i;
	  int      halloffames, x;
  long     phalloffames;
  float    pts = 0;
  char     killerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH];
  char     halloffamelist_file[1024], newleader_file[1024], buf2[MAX_STRING_LENGTH];
  char     buffer[1024], *ptr;

 
 sprintf(halloffamelist_file, "lib/information/leaderboard");
 sprintf(newleader_file, "lib/information/leaderboardprod");

  ptr = halloffamelist_file;
  for (ptr = halloffamelist_file; *ptr != '\0'; ptr++)
  {
    *ptr = LOWER(*ptr);
    if (*ptr == ' ')
      *ptr = '_';
  }


    newleaderlist = fopen(newleader_file, "wt");
    if (!newleaderlist)
    {
      send_to_char("error: couldn't open newleaderlist for writing.\r\n",
                   ch);
      return;
    }
	
	halloffamelist = fopen(halloffamelist_file, "rt");
	    if (!halloffamelist)
    {
      send_to_char("error: couldn't open oldhalloffamelist for writing.\r\n",
                   ch);
      return;
    }
  
    while(fscanf(halloffamelist, "%s %d %s\n", name, &halloffames, killer) != EOF)
  {
    pts = halloffames;
  /*  pts /= 100.0;*/
    if(!strcmp(name, "none"))
    break;
    sprintf(buf2, "%s %d\t\r\n",
            name, (int)pts);
        fprintf(newleaderlist, buf2);

  }


  
    fclose(halloffamelist);
	fclose(newleaderlist);
}
  

void displayLeader(P_char ch, char *arg, int cmd)
{

  FILE    *halloffameList, *f;
  char     name[MAX_STRING_LENGTH], buf[65536], buf2[2048], tempbuf[2048], tempbuf2[2048];
  char     killer[MAX_STRING_LENGTH];
  int      halloffames, x;
  char     i;
  float    pts = 0;
  char     filename[1024];
  update_shipfrags();


  
  sprintf(filename, "lib/information/leaderboardprod");

  if (!(halloffameList = fopen(filename, "rt")))
  {
   /*sprintf(name, "Couldn't open leaderboard: %s\r\n", filename);
   send_to_char(name, ch);*/
    return;
  }

  int actualrecords = 0;
 
  while(fscanf(halloffameList, "%s %d\n", name, &halloffames) != EOF)
  {
   actualrecords++;
  }
  fclose(halloffameList);

  halloffameList = fopen(filename, "rt");
  if (!halloffameList)
  {
    return;
  }


  strcpy(buf, "\r\n&+y=-=-=-=-=-=-=-=-=-=--= &+rDuris Mud &+WLeader Board&+y =-=-=-=-=-=-=-=-=-=-=-&n\r\n\r\n");
  sprintf(tempbuf, "   &+W%-15s           &+Y%s\r\n",
          "Name", "Score");
  sprintf(tempbuf2, "   &+L%-15s           &+L%s\r\n",
          "----", "-----");
  strcat(buf, tempbuf);
  strcat(buf, tempbuf2);
  for (i = 0; i < actualrecords; i++)
  {
    fscanf(halloffameList, "%s %d\n", name, &halloffames);
    name[0] = toupper(name[0]);
    pts = halloffames;
    pts /= 100.0;

    sprintf(buf2, "   &+w%-15s          &+Y%6.2f\t\r\n",
            name, pts);
    strcat(buf, buf2);
  }


  fclose(halloffameList);

  strcat(buf, "\r\n");

  page_string(ch->desc, buf, 1);
}



void checkLeaderBoard(P_char ch, char killer[1024])
{
  char     arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];

  if (!ch)
    return;
  if (!killer)
    return;

  argument_split_2(killer, arg1, arg2);
  writeLeaderBoard(ch, arg1);
  return;
}

void writeLeaderBoard(P_char ch, char thekiller[1024])
{
  FILE    *halloffamelist, *f;
  char     highPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
    lowPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH], change = FALSE;
  int      highHardcore[MAX_HALLOFFAME_SIZE],
    lowHardcore[MAX_HALLOFFAME_SIZE], i;
  long    phalloffames;
  char     killerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH];
  char     halloffamelist_file[1024];
  char     buffer[1024], *ptr;

  if (!ch)
    return;

  sprintf(halloffamelist_file, "lib/information/leaderboard");

  ptr = halloffamelist_file;
  for (ptr = halloffamelist_file; *ptr != '\0'; ptr++)
  {
    *ptr = LOWER(*ptr);
    if (*ptr == ' ')
      *ptr = '_';
  }

  //halloffamelist_file[0] = 'F';

  halloffamelist = fopen(halloffamelist_file, "rt");

  if (!halloffamelist)
  {
    return;
  }  

  if (isname(thekiller, "NotDead"))
    phalloffames = getLeaderBoardPts(ch);
  else
    phalloffames =
      getLeaderBoardPts(ch) + 100;
  *thekiller = toupper(*thekiller);
  /* read ten highest */
 
  // for (i = 0; i < MAX_HALLOFFAME_SIZE; i++)
  int highx, actualrecords=0;
  char namex[MAX_STRING_LENGTH], killx[MAX_STRING_LENGTH], debugbuf[MAX_STRING_LENGTH];
  
  while((fscanf(halloffamelist, "%s %d %s\n", namex, &highx, killx)) != EOF && actualrecords < MAX_HALLOFFAME_SIZE)
  {
   actualrecords++;
  }

  fclose(halloffamelist);

  halloffamelist = fopen(halloffamelist_file, "rt");
  if (!halloffamelist)
  {
    return;
  }

  for (i = 0; i < actualrecords; i++)
  {
   /* if (feof(halloffamelist))
    {
     // send_to_char("error: halloffame list terminated prematurely.\r\n", ch);
      return;
    }*/

    fscanf(halloffamelist, "%s %d %s\n", highPlayerName[i], &highHardcore[i],
           killerName[i]);
  }

  fclose(halloffamelist);

/* check if player already has an entry and is higher than somebody else
    (including his previous entry) - if so, delete it.  if they end up at
    the end of the list (higher than nobody after deleted), stick em there
    here */

  for (i = 0; i < actualrecords; i++)
  {
    // check for dupe entry - just delete it if it exists.  let's see what
    // happens, shall we?

    if (!str_cmp(ch->player.name, highPlayerName[i]))
    {
      deleteLeaderEntry(highPlayerName, highHardcore, i, killerName);

      break;
    }
  }

  /* see if player has beaten anybody currently on the list */

  for (i = 0; (i < actualrecords); i++)
  {
        if (phalloffames > highHardcore[i])
        {
          insertLeaderEntry(highPlayerName, highHardcore, ch->player.name,
                          phalloffames, i, killerName, thekiller);


          change = TRUE;
          break;
        }
   }

  if (change)
  {
    halloffamelist = fopen(halloffamelist_file, "wt");
    if (!halloffamelist)
    {
    /*  send_to_char("error: couldn't open halloffamelist for writing.\r\n",
                   ch);*/
      return;
    }
  
    for (i = 0; i < actualrecords; i++)
    {
      fprintf(halloffamelist, "%s %d %s\n", highPlayerName[i],
              highHardcore[i], killerName[i]);
      if(i == (actualrecords - 1))
       {
        if(strcmp(highPlayerName[i], "none"))
        {
      	  fprintf(halloffamelist, "none 0 0\n");
	 }
	}
     
    }

    fclose(halloffamelist);
  }



}
