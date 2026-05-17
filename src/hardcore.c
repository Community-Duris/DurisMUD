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

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include "hardcore.h"
#include "hardcore_config.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "mm.h"
#include "ships.h"
#include "sound.h"
#include "spells.h"
#include "sql.h"
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

int getHardCorePts(P_char ch)
{
	const struct hardcore_config *config = hardcore_config_get();
	int hardcorepts = (GET_LEVEL(ch) * config->score_level_points) +
	                  (ch->points.curr_exp / config->score_experience_divisor) +
	                  (ch->only.pc->frags * config->score_frag_points);

	if (IS_MULTICLASS_PC(ch))
		hardcorepts *= config->score_multiclass_multiplier;

	return hardcorepts;
}

void writeHallOfFame(P_char ch, char thekiller[1024])
{
	FILE *halloffamelist;
	// static cuz these are huge and will blow the stack
	static char highPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH];
	//         lowPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH]; // unused, commenting out to save mem
	bool change = FALSE;
	int  highHardcore[MAX_HALLOFFAME_SIZE],
		//       lowHardcore[MAX_HALLOFFAME_SIZE], // unused
		phalloffames, i;
	static char killerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH];
	char        buffer[1024], *ptr;
	int         actualrecords = 0;

	if (!ch)
		return;

	halloffamelist = fopen(halloffamelist_file, "r");

	if (!halloffamelist)
	{
		logit(LOG_DEBUG, "writeHallOfFame(): Could not open file '%s'.", halloffamelist_file);
		if (ch)
			send_to_char("Couldn't open Hall of Fame! Tell a god.\r\n", ch);
		return;
	}

	if (isname(thekiller, "NotDead"))
		phalloffames = getHardCorePts(ch);
	else
		phalloffames = getHardCorePts(ch) + hardcore_config_get()->score_killer_bonus;
	*thekiller = toupper(*thekiller);

	// Read in the hall of fame list from file.
	while ((fscanf(halloffamelist, "%s %d %s\n", highPlayerName[actualrecords], &highHardcore[actualrecords], killerName[actualrecords]) != EOF) && actualrecords < MAX_HALLOFFAME_SIZE)
		actualrecords++;

	fclose(halloffamelist);

	// Delete their entry if they have one.
	for (i = 0; i < actualrecords; i++)
	{
		// Check for player already on list.
		if (!str_cmp(ch->player.name, highPlayerName[i]))
		{
			deleteHallEntry(highPlayerName, highHardcore, i, killerName);
			actualrecords--;
			break;
		}
	}

	/* see if player has beaten anybody currently on the list */
	for (i = 0; i < actualrecords; i++)
	{
		if (phalloffames > highHardcore[i])
		{
			insertHallEntry(highPlayerName, highHardcore, ch->player.name, phalloffames, i, killerName, thekiller);
			actualrecords++;
			change = TRUE;
			break;
		}
	}

	// If new entry:
	if (!change && actualrecords < MAX_HALLOFFAME_SIZE)
	{
		insertHallEntry(highPlayerName, highHardcore, ch->player.name, phalloffames, actualrecords++, killerName, thekiller);
		change = TRUE;
	}

	if (change)
	{
		halloffamelist = fopen(halloffamelist_file, "w");
		if (!halloffamelist)
		{
			logit(LOG_DEBUG, "writeHallOfFame(): Could not open file '%s' for writing.", halloffamelist_file);
			send_to_char("error: couldn't open halloffamelist for writing.\r\n", ch);
			return;
		}

		for (i = 0; i < actualrecords; i++)
			fprintf(halloffamelist, "%s %d %s\n", highPlayerName[i], highHardcore[i], killerName[i]);
		fclose(halloffamelist);
	}
}

void deleteHallEntry(char names[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH], int halloffames[MAX_HALLOFFAME_SIZE], int pos, char killer[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH])
{
	int i;

	if (pos >= MAX_HALLOFFAME_SIZE)
	{
		logit(LOG_DEBUG, "deleteHallEntry(): pos too big: '%d'.", pos);
		return;
	}

	for (i = pos; i < MAX_HALLOFFAME_SIZE - 1; i++)
	{
		strcpy(names[i], names[i + 1]);
		halloffames[i] = halloffames[i + 1];
		strcpy(killer[i], killer[i + 1]);
	}

	strcpy(names[MAX_HALLOFFAME_SIZE - 1], "Nobody");
	halloffames[MAX_HALLOFFAME_SIZE - 1] = 0;
	strcpy(killer[MAX_HALLOFFAME_SIZE - 1], "NotDead");
}

void insertHallEntry(
	char names[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH], int halloffames[MAX_HALLOFFAME_SIZE], char *name, int newHardcore, int pos, char killers[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH], char *killer)
{
	int i;

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
	char       name[MAX_STRING_LENGTH], buf[65536], buf2[2048];
	float      pts = 0;
	MYSQL_RES *res;
	MYSQL_ROW  row;
	const struct hardcore_config *config = hardcore_config_get();

	if (!ch)
		return;

	// score follows getHardCorePts(); corrupted frags are excluded by policy.
	res = db_query("SELECT pd.name, "
	               "  ((pd.level * %d) + (pd.exp / %d) + "
	               "   (CASE WHEN pd.frags < %d THEN pd.frags ELSE 0 END * %d)) * "
	               "  (CASE WHEN pd.secondary_class > 0 AND pd.secondary_class <> 2147483648 THEN %d ELSE 1 END) + "
	               "  (CASE WHEN pd.killed_by IS NOT NULL AND pd.killed_by <> 'Notdead' THEN %d ELSE 0 END) as score, "
	               "  COALESCE(pd.killed_by, 'Notdead') as killed_by "
	               "FROM player_data pd "
	               "WHERE (pd.act2 & 8192) > 0 OR pd.killed_by IS NOT NULL "
	               "ORDER BY score DESC "
	               "LIMIT %d",
	               config->score_level_points,
	               config->score_experience_divisor,
	               config->score_invalid_frag_threshold,
	               config->score_frag_points,
	               config->score_multiclass_multiplier,
	               config->score_killer_bonus,
	               MAX_HALLOFFAME_SIZE);

	if (!res)
	{
		send_to_char("&+RError: Couldn't query hall of fame from database.&n\r\n", ch);
		return;
	}

	strcpy(buf, "\t\r\n&+r-= &+LHall Of&+L Fame&+r =-&n\r\n\r\n");
	snprintf(buf2, 2048, "   &+w%-15s           &+w%s           &+w%-15s\r\n", "Name", "Points", "Deaths/Killed by");
	strcat(buf, buf2);

	while ((row = mysql_fetch_row(res)))
	{
		if (row[0] && row[1])
		{
			strncpy(name, row[0], sizeof(name) - 1);
			name[sizeof(name) - 1] = '\0';
			name[0]                = toupper(name[0]);
			pts                    = atof(row[1]) / (float)config->score_display_divisor;

			snprintf(buf2, 2048, "   &+L%-15s          &+r% 6.2f\t      &+W%-15s\r\n", name, pts, row[2] ? row[2] : "unknown");
			strcat(buf, buf2);
		}
	}
	mysql_free_result(res);

	strcat(buf, "\r\n");

	page_string(ch->desc, buf, 1);
}

void checkHallOfFame(P_char ch, char killer[1024])
{
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];

	if (!ch || !killer)
		return;

	argument_split_2(killer, arg1, arg2);
	writeHallOfFame(ch, arg1);
	return;
}

long getLeaderBoardPts(P_char ch)
{
	if (!IS_PC(ch))
		return 0;

	update_shipfrags();
	int sf = calculate_shipfrags(ch);

	const struct hardcore_config *config = hardcore_config_get();

	// debug("ch: %s, shipfrags: %d, levelpoints: %d, exppoints: %d, fragpoints: %d, deathpoints: %d\r\n", GET_NAME(ch), sf, (GET_LEVEL(ch) * config->score_level_points), (ch->points.curr_exp / config->score_experience_divisor), (ch->only.pc->frags
	// * config->score_frag_points), (ch->only.pc->numb_deaths * config->score_death_penalty_points));

	long leaderpts = (GET_LEVEL(ch) * config->score_level_points) +
	                 (ch->points.curr_exp / config->score_experience_divisor) +
	                 (sf * config->score_frag_points) +
	                 (ch->only.pc->frags * config->score_frag_points) -
	                 (ch->only.pc->numb_deaths * config->score_death_penalty_points);

	return leaderpts;
}

// leaderboard is now computed from database, no file sync needed
bool newLeaderBoard(P_char ch, char *arg, int cmd) { return TRUE; }

void displayLeader(P_char ch, char *arg, int cmd)
{
	char       name[MAX_STRING_LENGTH], buf[65536], buf2[2048];
	float      pts = 0;
	MYSQL_RES *res;
	MYSQL_ROW  row;
	const struct hardcore_config *config = hardcore_config_get();

	if (!ch)
		return;

	// score = (level * points) + (exp / divisor) + (shipfrags * frag points) + (frags * frag points) - (deaths * penalty)
	// filter out corrupted frags (> threshold = overflow junk from migration)
	res = db_query("SELECT pd.name, "
	               "  (pd.level * %d) + (pd.exp / %d) + "
	               "  (COALESCE(s.frags, 0) * %d) + "
	               "  (CASE WHEN pd.frags < %d THEN pd.frags ELSE 0 END * %d) - "
	               "  (pd.numb_deaths * %d) as score "
	               "FROM player_data pd "
	               "LEFT JOIN ships s ON LOWER(pd.name) = LOWER(s.owner_name) "
	               "WHERE pd.frags < %d "
	               "ORDER BY score DESC "
	               "LIMIT %d",
	               config->score_level_points,
	               config->score_experience_divisor,
	               config->score_frag_points,
	               config->score_invalid_frag_threshold,
	               config->score_frag_points,
	               config->score_death_penalty_points,
	               config->score_invalid_frag_threshold,
	               MAX_LEADERBOARD_SIZE);

	if (!res)
	{
		send_to_char("&+RError: Couldn't query leaderboard from database.&n\r\n", ch);
		return;
	}

	strcpy(buf, "\r\n&+y=-=-=-=-=-=-=-=-=-=--= &+rDuris Mud &+WLeader Board&+y =-=-=-=-=-=-=-=-=-=-=-&n\r\n\r\n");
	snprintf(buf2, 2048, "   &+W%-15s           &+Y%s\r\n", "Name", "Score");
	strcat(buf, buf2);
	snprintf(buf2, 2048, "   &+L%-15s           &+L%s\r\n", "----", "-----");
	strcat(buf, buf2);

	while ((row = mysql_fetch_row(res)))
	{
		if (row[0] && row[1])
		{
			strncpy(name, row[0], sizeof(name) - 1);
			name[sizeof(name) - 1] = '\0';
			name[0]                = toupper(name[0]);
			pts                    = atof(row[1]) / (float)config->score_display_divisor;

			snprintf(buf2, 2048, "   &+w%-15s          &+Y%6.2f\t\r\n", name, pts);
			strcat(buf, buf2);
		}
	}
	mysql_free_result(res);

	strcat(buf, "\r\n");
	page_string(ch->desc, buf, 1);
}

// leaderboard is now computed from database on-the-fly, no need to write
void writeLeaderBoard(P_char ch)
{
	// no-op - leaderboard is computed from player_data table
}

void checkLeaderBoard(P_char ch)
{
	// no-op - leaderboard is computed from player_data table
}

// Copies leaderboard file to leaderboardprod.
// Returns TRUE iff leaderboard is copied over.
bool newHardcoreBoard(P_char ch, char *arg, int cmd)
{
	FILE *hardcorelist, *f, *newhardcorelist;
	// these arrays are unused and huge, commenting out to save mem
	// char     highPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH],
	//          lowPlayerName[MAX_HALLOFFAME_SIZE][MAX_STRING_LENGTH];
	char name[MAX_STRING_LENGTH], killedby[MAX_STRING_LENGTH];
	bool change = FALSE;
	// int      highHardcore[MAX_LEADERBOARD_SIZE],
	//          lowHardcore[MAX_LEADERBOARD_SIZE], i;
	int   halloffames, x;
	long  phalloffames;
	float pts = 0;
	char  buf[MAX_STRING_LENGTH], buffer[1024], *ptr;

	newhardcorelist = fopen(mort_halloffame_file, "w");
	if (!newhardcorelist)
	{
		if (ch)
			send_to_char("error: couldn't open newhardcore file for writing.\r\n", ch);
		logit(LOG_DEBUG, "newHardcoreBoard(): Could not open file '%s'.", mort_halloffame_file);
		return FALSE;
	}

	hardcorelist = fopen(halloffamelist_file, "r");
	if (!hardcorelist)
	{
		if (ch)
			send_to_char("error: couldn't open halloffamelist for reading.\r\n", ch);
		logit(LOG_DEBUG, "newHardcoreBoard(): Could not open file '%s'.", halloffamelist_file);
		fclose(newhardcorelist);
		return FALSE;
	}

	while (fscanf(hardcorelist, "%s %d %s\n", name, &halloffames, killedby) != EOF)
	{
		pts = halloffames;
		if (!strcmp(name, "none"))
			break;
		snprintf(buf, MAX_STRING_LENGTH, "%s %d %s\r\n", name, (int)pts, killedby);
		fprintf(newhardcorelist, "%s", buf);
	}

	fclose(hardcorelist);
	fclose(newhardcorelist);

	return TRUE;
}
