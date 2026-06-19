#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "assocs.h"
#include "config.h"
#include "frag_cap_config.h"
#include "redis.h"
#include "ships.h"
#include "spells.h"
#include "sql.h"
#define MAX_FRAG_SIZE 10 /* max size of high/low lists */

extern const struct class_names class_names_table[];
extern const struct race_names  race_names_table[];
extern P_char                   misfire_check(P_char ch, P_char spell_target, int flag);

extern P_room               world;
extern const racewar_struct racewar_color[MAX_RACEWAR + 2];

extern void get_level_cap_info(long *max_frags, int *racewar, int *level, time_t *next_update);
extern void get_level_cap(int *max_level, int *racewar);
extern int  sql_level_cap(int racewar_side);

/*
 * fragWorthy - is ch worthy of gaining a frag and victim worthy of losing
 *              one?
 */

int fragWorthy(P_char ch, P_char victim)
{
	int    racew;
	P_char tch;

	if (IS_NPC(victim))
		return FALSE;

	if (IS_NPC(ch))
		if (ch->following && IS_PC(ch->following))
			ch = ch->following;
		else
			return FALSE;

	if ((GET_LEVEL(ch) > 56) || (GET_LEVEL(victim) > 56))
		return FALSE;

	if (CHAR_IN_ARENA(ch) || CHAR_IN_ARENA(victim))
		return FALSE;

	/* killing people under 20 - no frag.  killing people more than 10
	   levels under you - no frag. */

	/* non-floating point floating point system, dig it?  100 frags = 1.00 */
	/* Commenting this out for the 2017 wipe.
	  if ((ch->only.pc->frags > 2000) && (GET_LEVEL(victim) < 40))
	    return FALSE;
	*/

	if (GET_LEVEL(victim) < 20)
		return FALSE;

	racew = (opposite_racewar(ch, victim) /* || (IS_ILLITHID(ch) && !IS_ILLITHID(victim)) ||
	                                         (IS_DISGUISE(victim) && (EVIL_RACE(victim) != EVIL_RACE(ch))) */
	);

	if (!racew)
		return FALSE;

	/* Kvark adding harder check for frags, connected to missfire. */
	/*
	  misfire_check(ch, victim,
	                  DISALLOW_SELF | DISALLOW_BACKRANK);

	  if(!affected_by_spell(ch, TAG_NOMISFIRE))
	  {
	      send_to_char("&+WThis kind of frag counts as nothing, go prove your self in a fair fight instead.&n\n", ch);
	                return FALSE;

	  }
	*/
	/*
	else
	{
	 for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
	 {
	  if(tch)
	   if (tch != ch)
	    if(opposite_racewar(victim, tch) && !IS_TRUSTED(tch))
	     if(!affected_by_spell(tch, TAG_NOMISFIRE)){
	        send_to_char("This kind of frag counts as nothing, blame your firends.", tch);
	        return FALSE;
	     }
	 }

	}
	*/

	if (victim->only.pc->frags > 500)
		return TRUE;

	if (racew && (victim->only.pc->frags > 200))
		return TRUE;

	if ((GET_LEVEL(victim) + 10) < GET_LEVEL(ch))
		return FALSE;

	return racew;
	/*  if (racew) return TRUE;

	  if (IS_ILLITHID(ch) && !IS_ILLITHID(victim)) return TRUE;

	  return FALSE;*/
}

// helper to build fraglist query with filter
static MYSQL_RES *query_frag_leaders(const char *filter, int ascending, int limit)
{
	char query[2048];

	if (filter && filter[0])
	{
		snprintf(query,
		         sizeof(query),
		         "SELECT char_name, total_frags FROM frag_leaderboard "
		         "WHERE deleted_at IS NULL AND %s "
		         "ORDER BY total_frags %s LIMIT %d",
		         filter,
		         ascending ? "ASC" : "DESC",
		         limit);
	}
	else
	{
		snprintf(query,
		         sizeof(query),
		         "SELECT char_name, total_frags FROM frag_leaderboard "
		         "WHERE deleted_at IS NULL "
		         "ORDER BY total_frags %s LIMIT %d",
		         ascending ? "ASC" : "DESC",
		         limit);
	}

	return db_query(query);
}

// check if player is at top or bottom of overall fraglist
static void check_frag_position(P_char ch)
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

	if (!ch || IS_NPC(ch))
		return;

	// check if player is #1 overall
	res = db_query("SELECT char_name FROM frag_leaderboard "
	               "WHERE deleted_at IS NULL ORDER BY total_frags DESC LIMIT 1");
	if (res)
	{
		row = mysql_fetch_row(res);
		if (row && row[0] && isname(row[0], GET_NAME(ch)))
		{
			SET_BIT(ch->specials.act3, PLR3_FRAGLEAD);
		}
		else
		{
			REMOVE_BIT(ch->specials.act3, PLR3_FRAGLEAD);
		}
		mysql_free_result(res);
	}

	// check if player is #1 lowest
	res = db_query("SELECT char_name FROM frag_leaderboard "
	               "WHERE deleted_at IS NULL ORDER BY total_frags ASC LIMIT 1");
	if (res)
	{
		row = mysql_fetch_row(res);
		if (row && row[0] && isname(row[0], GET_NAME(ch)))
		{
			SET_BIT(ch->specials.act3, PLR3_FRAGLOW);
		}
		else
		{
			REMOVE_BIT(ch->specials.act3, PLR3_FRAGLOW);
		}
		mysql_free_result(res);
	}
}

// shows the frag list from database
void do_fraglist(P_char ch, char *arg, int cmd)
{
	char       buf[65536], buf2[2048], name[256];
	int        frags, count;
	float      fragnum     = 0;
	char       filter[256] = "";
	int        cap_level, cap_racewar, cap_others;
	long       cap_frags;
	time_t     cap_timer;
	int        days, hours, mins, secs;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	if (!IS_ALIVE(ch))
		return;

	// for default view (no filter), use cache
	if (!arg || !arg[0])
	{
		char *cached = redis_get_fraglist();
		if (cached)
		{
			page_string(ch->desc, cached, 1);
			free(cached);
			return;
		}
		// cache miss - regenerate and cache
		redis_cache_fraglist();
		cached = redis_get_fraglist();
		if (cached)
		{
			page_string(ch->desc, cached, 1);
			free(cached);
			return;
		}
	}

	if (arg && arg[0])
	{
		// racewar filters
		if (strstr("normal", arg))
		{
			filter[0] = '\0'; // no filter
		}
		else if (strstr("goodie", arg))
		{
			snprintf(filter, sizeof(filter), "racewar = %d", RACEWAR_GOOD);
		}
		else if (strstr("evil", arg))
		{
			snprintf(filter, sizeof(filter), "racewar = %d", RACEWAR_EVIL);
		}
		else if (strstr("undead", arg))
		{
			snprintf(filter, sizeof(filter), "racewar = %d", RACEWAR_UNDEAD);
		}
		else if (strstr("illithid", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'illithid'");
		}
		// race filters
		else if (strstr("lich", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'lich'");
		}
		else if (strstr("vampire", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'vampire'");
		}
		else if (strstr("revenant", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'revenant'");
		}
		else if (strstr("human", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'human'");
		}
		else if (strstr("barbarian", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'barbarian'");
		}
		else if (strstr("drow elf", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'drow_elf'");
		}
		else if (strstr("grey elf", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'grey_elf'");
		}
		else if (strstr("mountain dwarf", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'mountain_dwarf'");
		}
		else if (strstr("duergar dwarf", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'duergar_dwarf'");
		}
		else if (strstr("halfling", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'halfling'");
		}
		else if (strstr("gnome", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'gnome'");
		}
		else if (strstr("storm giant", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'storm_giant'");
		}
		else if (strstr("ogre", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'ogre'");
		}
		else if (strstr("troll", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'troll'");
		}
		else if (strstr("drider", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'drider'");
		}
		else if (strstr("half elf", arg) || strstr("half-elf", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'half-elf'");
		}
		else if (strstr("orc", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'orc'");
		}
		else if (strstr("thrikreen", arg) || strstr("thri-kreen", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'thri-kreen'");
		}
		else if (strstr("centaur", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'centaur'");
		}
		else if (strstr("githyanki", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'githyanki'");
		}
		else if (strstr("minotaur", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'minotaur'");
		}
		else if (strstr("goblin", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'goblin'");
		}
		else if (strstr("orog", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'orog'");
		}
		else if (strstr("githzerai", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'githzerai'");
		}
		else if (strstr("agathinon", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'agathinon'");
		}
		else if (strstr("eladrin", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'eladrin'");
		}
		else if (strstr("pillithid", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'planetbound_illithid'");
		}
		else if (strstr("wood elf", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'wood_elf'");
		}
		else if (strstr("kobold", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'kobold'");
		}
		else if (strstr("kuo toa", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'kuo_toa'");
		}
		else if (strstr("firbolg", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'firbolg'");
		}
		else if (strstr("tiefling", arg))
		{
			snprintf(filter, sizeof(filter), "race = 'tiefling'");
		}
		// class filters
		else if (strstr("warrior", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'warrior'");
		}
		else if (strstr("ranger", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'ranger'");
		}
		else if (strstr("paladin", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'paladin'");
		}
		else if (strstr("psionicist", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'psionicist'");
		}
		else if (strstr("anti-paladin", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'anti-paladin'");
		}
		else if (strstr("cleric", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'cleric'");
		}
		else if (strstr("monk", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'monk'");
		}
		else if (strstr("unholy-piper", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'unholy-piper'");
		}
		else if (strstr("shaman", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'shaman'");
		}
		else if (strstr("sorcerer", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'sorcerer'");
		}
		else if (strstr("necromancer", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'necromancer'");
		}
		else if (strstr("conjurer", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'conjurer'");
		}
		else if (strstr("summoner", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'summoner'");
		}
		else if (strstr("rogue", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'rogue'");
		}
		else if (strstr("assassin", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'assassin'");
		}
		else if (strstr("mercenary", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'mercenary'");
		}
		else if (strstr("bard", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'bard'");
		}
		else if (strstr("thief", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'thief'");
		}
		else if (strstr("druid", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'druid'");
		}
		else if (strstr("blighter", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'blighter'");
		}
		else if (strstr("reaver", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'reaver'");
		}
		else if (strstr("illusionist", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'illusionist'");
		}
		else if (strstr("berserker", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'berserker'");
		}
		else if (strstr("dreadlord", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'dreadlord'");
		}
		else if (strstr("ethermancer", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'ethermancer'");
		}
		else if (strstr("avenger", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'avenger'");
		}
		else if (strstr("theurgist", arg))
		{
			snprintf(filter, sizeof(filter), "class = 'theurgist'");
		}
		else if (strstr("ship", arg))
		{
			update_shipfrags();
			display_shipfrags(ch);
			return;
		}
		else if (strstr("guild", arg))
		{
			show_guild_frags(ch);
			return;
		}
		else
		{
			send_to_char("Valid fraglists exist by race, class, undead/evil/good, and overall (no argument).\r\n", ch);
			return;
		}
	}

	// get level cap info (already uses sql)
	get_level_cap_info(&cap_frags, &cap_racewar, &cap_level, &cap_timer);
	cap_others = sql_level_cap((cap_racewar == RACEWAR_GOOD) ? RACEWAR_EVIL : RACEWAR_GOOD);
	cap_timer -= time(NULL);

	if (cap_timer <= 0)
	{
		secs = mins = hours = days = 0;
	}
	else
	{
		secs = cap_timer % 60;
		cap_timer /= 60;
		mins = cap_timer % 60;
		cap_timer /= 60;
		hours = cap_timer % 24;
		cap_timer /= 24;
		days = cap_timer;
	}

	long frag_totals[MAX_RACEWAR] = {0};
	for (int i = 0; i < MAX_RACEWAR; i++)
	{
		MYSQL_RES *res = db_query("SELECT SUM(total_frags) FROM frag_leaderboard WHERE racewar=%d", i);
		if (res)
		{
			MYSQL_ROW row = mysql_fetch_row(res);
			if(row and row[0])
			{
				frag_totals[i] = atol(row[0]);
			}
			mysql_free_result(res);
		}
	}

	snprintf(buf,
	         MAX_STRING_LENGTH,
	         "&+YFrag Level Cap:&+w %d - All, &+WGoodies Total Frags - &+w%d.%02d, &+REvils Total Frags - &+w%d.%02d\n&+YTimer:&+w %02d:%02d:%02d:%02d &+YFrags needed:&+w %.2f&n\n\n&+WTop Fraggers\n\n",
	         cap_level,
	         (int)(frag_totals[RACEWAR_GOOD] / 100),
	         (int)(frag_totals[RACEWAR_GOOD] % 100),
			 (int)(frag_totals[RACEWAR_EVIL] / 100),
	         (int)(frag_totals[RACEWAR_EVIL] % 100),
	         days,
	         hours,
	         mins,
	         secs,
	         frag_cap_config_frags_for_level(cap_level + 1));

	// query top fraggers
	res = query_frag_leaders(filter, 0, MAX_FRAG_SIZE);
	if (!res)
	{
		send_to_char("&+RError: Couldn't query fraglist from database.&n\n", ch);
		return;
	}

	count = 0;
	while ((row = mysql_fetch_row(res)) && count < MAX_FRAG_SIZE)
	{
		if (row[0] && row[1])
		{
			strncpy(name, row[0], sizeof(name) - 1);
			name[sizeof(name) - 1] = '\0';
			name[0]                = toupper(name[0]);
			frags                  = atoi(row[1]);
			fragnum                = frags / 100.0;
			snprintf(buf2, MAX_STRING_LENGTH, "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
			strcat(buf, buf2);
			count++;
		}
	}
	mysql_free_result(res);

	// pad with "nobody" if less than 10 results
	while (count < MAX_FRAG_SIZE)
	{
		snprintf(buf2, MAX_STRING_LENGTH, "   &+Y%-30s             &+R% 6.2f\r\n", "Nobody", 0.0);
		strcat(buf, buf2);
		count++;
	}

	strcat(buf, "\r\n\r\n&+LLowest Fraggers\r\n\r\n");

	// query lowest fraggers
	res = query_frag_leaders(filter, 1, MAX_FRAG_SIZE);
	if (!res)
	{
		send_to_char("&+RError: Couldn't query fraglist from database.&n\n", ch);
		return;
	}

	count = 0;
	while ((row = mysql_fetch_row(res)) && count < MAX_FRAG_SIZE)
	{
		if (row[0] && row[1])
		{
			strncpy(name, row[0], sizeof(name) - 1);
			name[sizeof(name) - 1] = '\0';
			name[0]                = toupper(name[0]);
			frags                  = atoi(row[1]);
			fragnum                = frags / 100.0;
			snprintf(buf2, MAX_STRING_LENGTH, "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
			strcat(buf, buf2);
			count++;
		}
	}
	mysql_free_result(res);

	// pad with "nobody" if less than 10 results
	while (count < MAX_FRAG_SIZE)
	{
		snprintf(buf2, MAX_STRING_LENGTH, "   &+Y%-30s             &+R% 6.2f\r\n", "Nobody", 0.0);
		strcat(buf, buf2);
		count++;
	}

	strcat(buf, "\r\n");

	page_string(ch->desc, buf, 1);
}

// update frag leaderboard in database and check position flags
void checkFragList(P_char ch)
{
	if (!ch || IS_NPC(ch))
		return;

	// update the database leaderboard
	sql_update_frag_leaderboard(ch);

	// check if player is at top/bottom of list for special flags
	check_frag_position(ch);
}
