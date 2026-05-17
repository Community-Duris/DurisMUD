/****************************************************************************
 *
 *  File: hardcore.c                                           Part of Duris
 *  Usage: PERIODcore chars related materia.
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark 			Date: 2002-04-18
 * ***************************************************************************
 */

#define TROPHY

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "mm.h"
#include "sound.h"
#include "spells.h"
#include "weather.h"

/*
 * external variables
 */

extern P_char      character_list;
extern P_desc      descriptor_list;
extern P_event     event_type_list[];
extern P_index     mob_index;
extern P_index     obj_index;
extern P_obj       object_list;
extern P_room      world;
extern char        debug_mode;
extern const char *race_types[];

// extern const int material_absorbtion[][];
extern const struct stat_data stat_factor[];
extern float                  fake_sqrt_table[];
extern int                    pulse;
extern int                    arena_hometown_location[];
extern struct arena_data      arena;
extern struct agi_app_type    agi_app[];
extern struct dex_app_type    dex_app[];
extern struct message_list    fight_messages[];
extern struct str_app_type    str_app[];
extern struct time_info_data  time_info;
extern struct zone_data      *zone_table;

int gain_pp(P_char ch, int howmuch);

#define MAX_PERIODOFFAME_SIZE 15 /* max size of high/low lists */

int last_update = 0;
int MORTAL_LIST = 0;

void writePeriodOfFame(P_char ch, char thekiller[1024])
{
	FILE *periodoffamelist;
	char  highPlayerName[MAX_PERIODOFFAME_SIZE][MAX_STRING_LENGTH], lowPlayerName[MAX_PERIODOFFAME_SIZE][MAX_STRING_LENGTH], change = FALSE;
	int   highPERIODcore[MAX_PERIODOFFAME_SIZE], lowPERIODcore[MAX_PERIODOFFAME_SIZE], pperiodoffames                               = 0, i;
	char  killerName[MAX_PERIODOFFAME_SIZE][MAX_STRING_LENGTH];
	char  periodoffamelist_file[1024];
	char  buffer[1024], *ptr;

	if (!ch)
		return;

	snprintf(periodoffamelist_file, 1024, "lib/information/period_list");

	ptr = periodoffamelist_file;
	for (ptr = periodoffamelist_file; *ptr != '\0'; ptr++)
	{
		*ptr = LOWER(*ptr);
		if (*ptr == ' ')
			*ptr = '_';
	}

	// periodoffamelist_file[0] = 'F';

	periodoffamelist = fopen(periodoffamelist_file, "rt");

	if (!periodoffamelist)
	{
		snprintf(buffer, 1024, "Couldn't open periodoffamelist: %s\r\n", periodoffamelist_file);
		send_to_char(buffer, ch);
		return;
	}

	if (IS_PC(ch) && isname(thekiller, "NotDead"))
		pperiodoffames = ch->only.pc->epics;
	*thekiller = toupper(*thekiller);
	/* read ten highest */
	for (i = 0; i < MAX_PERIODOFFAME_SIZE; i++)
	{
		if (feof(periodoffamelist))
		{
			send_to_char("error: periodoffame list terminated prematurely.\r\n", ch);
			fclose(periodoffamelist);

			return;
		}

		fscanf(periodoffamelist, "%s %d %s\n", highPlayerName[i], &highPERIODcore[i], killerName[i]);
	}

	fclose(periodoffamelist);

	/* check if player already has an entry and is higher than somebody else
	    (including his previous entry) - if so, delete it.  if they end up at
	    the end of the list (higher than nobody after deleted), stick em there
	    here */

	for (i = 0; i < MAX_PERIODOFFAME_SIZE; i++)
	{
		// check for dupe entry - just delete it if it exists.  let's see what
		// happens, speriod we?

		if (!str_cmp(ch->player.name, highPlayerName[i]))
		{
			deletePeriodEntry(highPlayerName, highPERIODcore, i, killerName);

			break;
		}
	}

	/* see if player has beaten anybody currently on the list */

	for (i = 0; (i < MAX_PERIODOFFAME_SIZE); i++)
	{
		if (pperiodoffames > highPERIODcore[i])
		{
			insertPeriodEntry(highPlayerName, highPERIODcore, ch->player.name, pperiodoffames, i, killerName, thekiller);

			change = TRUE;
			break;
		}
	}

	if (change)
	{
		periodoffamelist = fopen(periodoffamelist_file, "wt");
		if (!periodoffamelist)
		{
			send_to_char("error: couldn't open periodoffamelist for writing.\r\n", ch);
			return;
		}

		for (i = 0; i < MAX_PERIODOFFAME_SIZE; i++)
		{
			fprintf(periodoffamelist, "%s %d %s\n", highPlayerName[i], highPERIODcore[i], killerName[i]);
		}

		fclose(periodoffamelist);
	}
}

void deletePeriodEntry(char names[MAX_PERIODOFFAME_SIZE][MAX_STRING_LENGTH], int periodoffames[MAX_PERIODOFFAME_SIZE], int pos, char killer[MAX_PERIODOFFAME_SIZE][MAX_STRING_LENGTH])
{
	int i;

	if (pos >= MAX_PERIODOFFAME_SIZE)
		return;

	for (i = pos; i < MAX_PERIODOFFAME_SIZE; i++)
	{
		strcpy(names[i], names[i + 1]);
		periodoffames[i] = periodoffames[i + 1];
		strcpy(killer[i], killer[i + 1]);
	}

	strcpy(names[MAX_PERIODOFFAME_SIZE - 1], "Nobody");
	periodoffames[MAX_PERIODOFFAME_SIZE - 1] = 0;
	strcpy(killer[MAX_PERIODOFFAME_SIZE - 1], "NotDead");
}

void insertPeriodEntry(char  names[MAX_PERIODOFFAME_SIZE][MAX_STRING_LENGTH],
                       int   periodoffames[MAX_PERIODOFFAME_SIZE],
                       char *name,
                       int   newPERIODcore,
                       int   pos,
                       char  killers[MAX_PERIODOFFAME_SIZE][MAX_STRING_LENGTH],
                       char *killer)
{
	int i;

	if (pos >= MAX_PERIODOFFAME_SIZE)
		return;

	if (pos == (MAX_PERIODOFFAME_SIZE - 1))
	{
		strcpy(names[pos], name);
		periodoffames[pos] = newPERIODcore;
		strcpy(killers[pos], killer);
		return;
	}

	for (i = MAX_PERIODOFFAME_SIZE - 2; i >= pos; i--)
	{
		strcpy(names[i + 1], names[i]);
		periodoffames[i + 1] = periodoffames[i];
		strcpy(killers[i + 1], killers[i]);
	}

	strcpy(names[pos], name);
	periodoffames[pos] = newPERIODcore;
	strcpy(killers[pos], killer);
}

void displayPERIODCore(P_char ch, char *arg, int cmd)
{

	FILE *periodoffameList;
	char  name[MAX_STRING_LENGTH], buf[65536], buf2[2048], tempbuf[2048];
	char  killer[MAX_STRING_LENGTH];
	int   periodoffames;
	char  i;
	char  filename[1024];

	if (!str_cmp("tome", arg))
	{
		//    display_book(ch);
		return;
	}

	if (MORTAL_LIST && !IS_TRUSTED(ch))
	{
		snprintf(filename, 1024, "lib/information/period_list_mortal");
	}
	else
	{
		snprintf(filename, 1024, "lib/information/period_list");
	}
	if (!(periodoffameList = fopen(filename, "rt")))
	{
		snprintf(name, MAX_STRING_LENGTH, "Couldn't open periodoffamelist: %s\n", filename);
		send_to_char(name, ch);
		return;
	}

	strcpy(buf, " \t\n\t&+Y-= &+y  Epic points&+Y   =-&n\n\n");
	snprintf(tempbuf, 2048, "   &+w%-15s           &+w%s           &+w%-15s\n", "Name", "Points", "");
	strcat(buf, tempbuf);
	for (i = 0; i < MAX_PERIODOFFAME_SIZE; i++)
	{
		fscanf(periodoffameList, "%s %d %s\n", name, &periodoffames, killer);
		name[0] = toupper(name[0]);

		snprintf(buf2, 2048, "   &+y%-15s          &+Y%d&n\n", name, periodoffames);
		strcat(buf, buf2);
	}

	if (!MORTAL_LIST)
	{
		system("cp lib/information/period_list lib/information/period_list_mortal");
		wizlog(56, "Created peroid list for mortals.");
		MORTAL_LIST = 1;
	}
	fclose(periodoffameList);

	strcat(buf, "\n");

	page_string(ch->desc, buf, 1);
}

void checkPeriodOfFame(P_char ch, char killer[1024])
{
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];

	if (!ch)
		return;
	if (!killer)
		return;

	argument_split_2(killer, arg1, arg2);
	writePeriodOfFame(ch, arg1);
	return;
}

#define QUEST_FILE_TROPHY      "areas/tome.trophy"
#define TEMP_QUEST_FILE_TROPHY "areas/temp_tome.trophy"
