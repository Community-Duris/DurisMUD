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
#include "hardcore.h"

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

int getHardCorePts(P_char ch)
{ 
  int hardcorepts = (GET_LEVEL(ch) * 1000) + (ch->points.curr_exp / 10000) +
                    (ch->only.pc->frags * 100);

  if(IS_MULTICLASS_NPC(ch))
    hardcorepts *= 2;

 return hardcorepts;         
}

void writeHallOfFame(P_char ch, char thekiller[1024])
{
  FILE    *halloffamelist;
  char     highPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
           lowPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH];
  bool     change = FALSE;
  int      highHardcore[MAX_HALLOFFAME_SIZE],
           lowHardcore[MAX_HALLOFFAME_SIZE], phalloffames, i;
  char     killerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH];
  char     buffer[1024], *ptr;
  int      actualrecords = 0;

  if( !ch )
    return;

  halloffamelist = fopen(halloffamelist_file, "r");

  if( !halloffamelist )
  {
    logit(LOG_DEBUG, "writeHallOfFame(): Could not open file '%s'.", halloffamelist_file );
    if( ch )
      send_to_char("Couldn't open Hall of Fame! Tell a god.\r\n", ch);
    return;
  }

  if( isname(thekiller, "NotDead") )
    phalloffames = getHardCorePts(ch);
  else
    phalloffames = getHardCorePts(ch) + 100;
  *thekiller = toupper(*thekiller);

  // Read in the hall of fame list from file.
  while( (fscanf(halloffamelist, "%s %d %s\n", highPlayerName[actualrecords], 
            &highHardcore[actualrecords], killerName[actualrecords]) != EOF)
         && actualrecords < MAX_HALLOFFAME_SIZE )
    actualrecords++;

  fclose(halloffamelist);

  // Delete their entry if they have one.
  for( i = 0; i < actualrecords; i++ )
  {
    // Check for player already on list.
    if( !str_cmp(ch->player.name, highPlayerName[i]) )
    {
      deleteHallEntry(highPlayerName, highHardcore, i, killerName);
      actualrecords--;
      break;
    }
  }

  /* see if player has beaten anybody currently on the list */
  for( i = 0; i < actualrecords; i++ )
  {
    if( phalloffames > highHardcore[i] )
    {
      insertHallEntry(highPlayerName, highHardcore, ch->player.name,
                      phalloffames, i, killerName, thekiller);
      actualrecords++;
      change = TRUE;
      break;
    }
  }

  // If new entry:
  if( !change && actualrecords < MAX_HALLOFFAME_SIZE )
  {
    insertHallEntry(highPlayerName, highHardcore, ch->player.name,
                      phalloffames, actualrecords++, killerName, thekiller);
    change = TRUE;
  }

  if( change )
  {
    halloffamelist = fopen(halloffamelist_file, "w");
    if( !halloffamelist )
    {
      logit(LOG_DEBUG, "writeHallOfFame(): Could not open file '%s' for writing.", halloffamelist_file );
      send_to_char("error: couldn't open halloffamelist for writing.\r\n", ch);
      return;
    }

    for( i = 0; i < actualrecords; i++ )
      fprintf(halloffamelist, "%s %d %s\n", highPlayerName[i],
              highHardcore[i], killerName[i]);
    fclose(halloffamelist);
  }
}

void deleteHallEntry(char names[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
                     int halloffames[MAX_HALLOFFAME_SIZE], int pos,
                     char killer[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH])
{
  int      i;

  if( pos >= MAX_HALLOFFAME_SIZE )
  {
    logit(LOG_DEBUG, "deleteHallEntry(): pos too big: '%d'.", pos );
    return;
  }

  for( i = pos; i < MAX_HALLOFFAME_SIZE-1; i++ )
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

  if( IS_TRUSTED( ch ) )
  {
    if( !(halloffameList = fopen(halloffamelist_file, "r")) )
    {
      logit(LOG_DEBUG, "displayHardCore(): could not open file '%s'.", halloffamelist_file );
      if( ch )
        send_to_char("Couldn't open God's Hall of Fame! Tell a god.\r\n", ch);
      return;
    }
  }
  else
  {
    if( !(halloffameList = fopen(mort_halloffame_file, "r")) )
    {
      logit(LOG_DEBUG, "displayHardCore(): could not open file '%s'.", halloffamelist_file );
      if( ch )
        send_to_char("Couldn't open Hall of Fame! Tell a god.\r\n", ch);
      return;
    }
  }

  strcpy(buf, "\t\r\n&+r-= &+LHall Of&+L Fame&+r =-&n\r\n\r\n");
  sprintf(tempbuf, "   &+w%-15s           &+w%s           &+w%-15s\r\n",
          "Name", "Points", "Deaths/Killed by");
  strcat(buf, tempbuf);
  for (i = 0; i < MAX_HALLOFFAME_SIZE; i++)
  {
    // Less than full hall of fame list?
    if( fscanf(halloffameList, "%s %d %s\n", name, &halloffames, killer) == EOF )
      break;
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
  char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];

  if( !ch || !killer )
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

  long leaderpts =
       (GET_LEVEL(ch) * 1000) + (ch->points.curr_exp / 10000) + (sf * 100) +
             (ch->only.pc->frags * 100) - (ch->only.pc->numb_deaths * 25);

  return leaderpts;
}

void deleteLeaderEntry(char names[MAX_LEADERBOARD_SIZE][MAX_STRING_LENGTH],
                     int halloffames[MAX_LEADERBOARD_SIZE], int pos )
{
  int      i;

  if( pos >= MAX_LEADERBOARD_SIZE )
  {
    logit(LOG_DEBUG, "deleteLeaderEntry(): pos too big: '%d'.", pos );
    return;
  }

  for( i = pos; i < MAX_LEADERBOARD_SIZE-1; i++ )
  {
    strcpy(names[i], names[i + 1]);
    halloffames[i] = halloffames[i + 1];
  }
}

void insertLeaderEntry(char names[MAX_LEADERBOARD_SIZE][MAX_STRING_LENGTH],
                     int halloffames[MAX_LEADERBOARD_SIZE], char *name,
                     int newHardcore, int pos )
{
  int      i;

  if( pos >= MAX_LEADERBOARD_SIZE )
  {
    logit(LOG_DEBUG, "insertLeaderEntry(): pos too big: '%d'.", pos );
    return;
  }

  if( pos == MAX_LEADERBOARD_SIZE - 1 )
  {
    strcpy(names[pos], name);
    halloffames[pos] = newHardcore;
    return;
  }

  for( i = MAX_LEADERBOARD_SIZE - 2; i >= pos; i-- )
  {
    strcpy(names[i + 1], names[i]);
    halloffames[i + 1] = halloffames[i];
  }

  strcpy(names[pos], name);
  halloffames[pos] = newHardcore;
}

// Copies leaderboard file to leaderboardprod.
// Returns TRUE iff leaderboard is copied over.
bool newLeaderBoard(P_char ch, char *arg, int cmd)
{
  FILE    *leaderboardlist, *f, *newleaderlist;
  char     highPlayerName[MAX_LEADERBOARD_SIZE][MAX_STRING_LENGTH],
           lowPlayerName[MAX_LEADERBOARD_SIZE][MAX_STRING_LENGTH];
  char     name[MAX_STRING_LENGTH];
  bool     change = FALSE;
  int      highHardcore[MAX_LEADERBOARD_SIZE],
           lowHardcore[MAX_LEADERBOARD_SIZE], i;
  int      halloffames, x;
  long     phalloffames;
  float    pts = 0;
  char     buf[MAX_STRING_LENGTH], buffer[1024], *ptr;

 
  newleaderlist = fopen(mort_leader_file, "w");
  if( !newleaderlist )
  {
    if( ch )
      send_to_char("error: couldn't open newleaderlist for writing.\r\n", ch);
    logit(LOG_DEBUG, "newLeaderBoard(): Could not open file '%s'.", mort_leader_file );
    return FALSE;
  }
	
  leaderboardlist = fopen(leaderboard_file, "r");
  if( !leaderboardlist )
  {
    if( ch )
      send_to_char("error: couldn't open leaderboard for reading.\r\n", ch);
    logit(LOG_DEBUG, "newLeaderBoard(): Could not open file '%s'.", leaderboard_file );
    fclose(newleaderlist);
    return FALSE;
  }

  while( fscanf(leaderboardlist, "%s %d\n", name, &halloffames) != EOF )
  {
    pts = halloffames;
    if(!strcmp(name, "none"))
      break;
    sprintf(buf, "%s %d\r\n", name, (int)pts);
    fprintf(newleaderlist, "%s", buf);
  }

  fclose(leaderboardlist);
  fclose(newleaderlist);

  return TRUE;
}
  

void displayLeader(P_char ch, char *arg, int cmd)
{

  FILE    *halloffameList, *f;
  char     name[MAX_STRING_LENGTH], buf[65536], buf2[2048], 
           tempbuf[2048], tempbuf2[2048];
  int      halloffames, x;
  char     i;
  float    pts = 0;

  one_argument( arg, name );
  // If !ch then !IS_TRUSTED(ch)
  if( name[0] != '\0' && is_abbrev( name, "reset" ) && IS_TRUSTED( ch ) )
  {
    send_to_char( "Updating leader board for mortals...\n", ch );
    if( newLeaderBoard( ch, arg, cmd ) )
      send_to_char( "Leader board updated!\n", ch );
    else
      send_to_char( "Leader board update failed!\n", ch );
    return;
  }

  update_shipfrags();

  if( IS_TRUSTED( ch ) )
  {
    if (!(halloffameList = fopen(leaderboard_file, "r")))
    {
      if( ch )
        send_to_char("Couldn't open God's leaderboard! Tell a god.\r\n", ch);
      logit(LOG_DEBUG, "displayLeader(): Could not open file '%s'.", leaderboard_file );
      return;
    }
  }
  else
  {
    if (!(halloffameList = fopen(mort_leader_file, "r")))
    {
      if( ch )
        send_to_char("Couldn't open leaderboard! Tell a god.\r\n", ch);
      logit(LOG_DEBUG, "displayLeader(): Could not open file '%s'.", mort_leader_file );
      return;
    }
  }

  int actualrecords = 0;
 
  while(fscanf(halloffameList, "%s %d\n", name, &halloffames) != EOF)
  {
   actualrecords++;
  }
  fclose(halloffameList);

  if( IS_TRUSTED( ch ) )
    halloffameList = fopen(leaderboard_file, "r");
  else
    halloffameList = fopen(mort_leader_file, "r");

  if (!halloffameList)
  {
    if( IS_TRUSTED( ch ) )
      logit(LOG_DEBUG, "displayLeader(): 2nd Could not open file '%s'.", leaderboard_file );
    else
      logit(LOG_DEBUG, "displayLeader(): 2nd Could not open file '%s'.", mort_leader_file );
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

void writeLeaderBoard( P_char ch )
{
  FILE    *halloffamelist;
  char     highPlayerName[MAX_LEADERBOARD_SIZE][MAX_STRING_LENGTH],
           lowPlayerName[MAX_LEADERBOARD_SIZE][MAX_STRING_LENGTH];
  bool     change = FALSE;
  int      highHardcore[MAX_LEADERBOARD_SIZE],
           lowHardcore[MAX_LEADERBOARD_SIZE], i;
  long     phalloffames;
  char     buffer[1024], *ptr;
  int      highx, actualrecords=0;
  char     namex[MAX_STRING_LENGTH];

  if( !ch )
    return;

  halloffamelist = fopen(leaderboard_file, "r");

  if( !halloffamelist )
  {
    logit(LOG_DEBUG, "writeLeaderBoard(): Could not open file '%s'.", leaderboard_file );
    return;
  }  

  phalloffames = getLeaderBoardPts(ch);
 
  // Count the number of records.  
  while((fscanf(halloffamelist, "%s %d\n", namex, &highx)) != EOF && actualrecords < MAX_LEADERBOARD_SIZE)
    actualrecords++;

  fclose(halloffamelist);

  halloffamelist = fopen(leaderboard_file, "r");
  if( !halloffamelist )
  {
    logit(LOG_DEBUG, "writeLeaderBoard(): 2nd Could not open file '%s'.", leaderboard_file );
    return;
  }

  // Load the records.
  for( i = 0; i < actualrecords; i++ )
    fscanf(halloffamelist, "%s %d\n", highPlayerName[i], &highHardcore[i] );

  fclose(halloffamelist);

  /* Check to see if they're already on the leaderboard.
     If so, then remove the entry so the new info can be placed there. */
  for (i = 0; i < actualrecords; i++)
  {
    // Check for player already on list.  See above comment.
    if (!str_cmp(ch->player.name, highPlayerName[i]))
    {
      deleteLeaderEntry(highPlayerName, highHardcore, i);
      actualrecords--;
      break;
    }
  }

  /* see if player has beaten anybody currently on the list */
  for( i = 0; i < actualrecords; i++ )
  {
    if( phalloffames > highHardcore[i] )
    {
      insertLeaderEntry(highPlayerName, highHardcore, ch->player.name,
                        phalloffames, i);
      // If nobody was knocked off, increment the number on the list.
      if( actualrecords < MAX_LEADERBOARD_SIZE )
        actualrecords++;
      change = TRUE;
      break;
    }
  }

  // If new entry:
  if( !change && actualrecords < MAX_LEADERBOARD_SIZE )
  {
    insertLeaderEntry(highPlayerName, highHardcore, ch->player.name,
                      phalloffames, actualrecords++);
    change = TRUE;
  }

  if( change )
  {
    halloffamelist = fopen(leaderboard_file, "w");
    if( !halloffamelist )
    {
      logit(LOG_DEBUG, "writeLeaderBoard(): Could not open file '%s' for writing.", leaderboard_file );
      return;
    }

    for (i = 0; i < actualrecords; i++)
      fprintf(halloffamelist, "%s %d\n", highPlayerName[i], highHardcore[i]);
    fclose(halloffamelist);
  }
}

void checkLeaderBoard( P_char ch )
{

  if( !ch || !IS_ALIVE(ch) )
    return;

  writeLeaderBoard( ch );
}

// Copies leaderboard file to leaderboardprod.
// Returns TRUE iff leaderboard is copied over.
bool newHardcoreBoard(P_char ch, char *arg, int cmd)
{
  FILE    *hardcorelist, *f, *newhardcorelist;
  char     highPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
           lowPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH];
  char     name[MAX_STRING_LENGTH], killedby[MAX_STRING_LENGTH];
  bool     change = FALSE;
  int      highHardcore[MAX_LEADERBOARD_SIZE],
           lowHardcore[MAX_LEADERBOARD_SIZE], i;
  int      halloffames, x;
  long     phalloffames;
  float    pts = 0;
  char     buf[MAX_STRING_LENGTH], buffer[1024], *ptr;

 
  newhardcorelist = fopen(mort_halloffame_file, "w");
  if( !newhardcorelist )
  {
    if( ch )
      send_to_char("error: couldn't open newhardcore file for writing.\r\n", ch);
    logit(LOG_DEBUG, "newHardcoreBoard(): Could not open file '%s'.", mort_halloffame_file );
    return FALSE;
  }
	
  hardcorelist = fopen(halloffamelist_file, "r");
  if( !hardcorelist )
  {
    if( ch )
      send_to_char("error: couldn't open halloffamelist for reading.\r\n", ch);
    logit(LOG_DEBUG, "newHardcoreBoard(): Could not open file '%s'.", halloffamelist_file );
    fclose(newhardcorelist);
    return FALSE;
  }

  while( fscanf(hardcorelist, "%s %d %s\n", name, &halloffames, killedby ) != EOF )
  {
    pts = halloffames;
    if(!strcmp(name, "none"))
      break;
    sprintf(buf, "%s %d %s\r\n", name, (int)pts, killedby );
    fprintf(newhardcorelist, "%s", buf);
  }

  fclose(hardcorelist);
  fclose(newhardcorelist);

  return TRUE;
}
