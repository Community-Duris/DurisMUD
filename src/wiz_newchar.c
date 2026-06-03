// wiz_newchar.c - wizard command to create test characters
#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "utils.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "account.h"
#include "epic.h"
#include "epic_skills.h"
#include "files.h"
#include "mm.h"
#include "spells.h"
#include "sql.h"
#include "sql_player.h"

extern int                class_table[LAST_RACE + 1][CLASS_COUNT + 1];
extern Skill              skills[];
extern epic_reward        epic_rewards[];
extern struct race_names  race_names_table[];
extern struct class_names class_names_table[];
extern struct mm_ds      *dead_mob_pool;
extern struct mm_ds      *dead_pconly_pool;

// syntax: newchar <name> <race_id> <class_id> <level> <true|false>
// see: newchar help race, newchar help class
void do_newchar(P_char ch, char *argument, int cmd)
{
	extern int   writeCharacter(P_char ch, int type, int room);
	extern void  clear_char(P_char ch);
	extern void  init_char(P_char ch);
	extern void  setCharPhysTypeInfo(P_char ch);
	extern char *mysql_str(const char *str, char *buf);

	char   arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char   arg3[MAX_INPUT_LENGTH], arg4[MAX_INPUT_LENGTH], arg5[MAX_INPUT_LENGTH];
	char   buf[MAX_STRING_LENGTH];
	char   name_lower[MAX_NAME_LENGTH + 1];
	int    race_id, class_id, level, class_bitmask, class_index;
	int    i, is_playable;
	bool   max_skills;
	P_char newch;

	if (IS_NPC(ch))
		return;

	if (GET_LEVEL(ch) < OVERLORD)
	{
		send_to_char("you need to be an overlord to use this.\r\n", ch);
		return;
	}

	if (!ch->desc || !ch->desc->account)
	{
		send_to_char("you need a valid account connection.\r\n", ch);
		return;
	}

	half_chop(argument, arg1, buf);
	half_chop(buf, arg2, buf);
	half_chop(buf, arg3, buf);
	half_chop(buf, arg4, buf);
	half_chop(buf, arg5, buf);

	// handle help subcommands
	if (!*arg1 || !str_cmp(arg1, "help"))
	{
		if (!*arg2 || !str_cmp(arg2, "usage"))
		{
			send_to_char("usage: newchar <name> <race_id> <class_id> <level> <true|false>\r\n", ch);
			send_to_char("       newchar help race   - list playable races\r\n", ch);
			send_to_char("       newchar help class  - list classes\r\n", ch);
			send_to_char("\r\nexample: newchar testnecro 3 11 56 true\r\n", ch);
			send_to_char("creates a drow elf necromancer at level 56 with maxed skills.\r\n", ch);
			return;
		}

		if (!str_cmp(arg2, "race"))
		{
			send_to_char("playable races:\r\n", ch);
			send_to_char("id   race              faction\r\n", ch);
			send_to_char("---- ----------------- -------\r\n", ch);
			for (i = 0; playable_races[i].race_id != -1; i++)
			{
				snprintf(buf, MAX_STRING_LENGTH, "%-4d %-17s %s\r\n", playable_races[i].race_id, race_names_table[playable_races[i].race_id].normal, playable_races[i].faction);
				send_to_char(buf, ch);
			}
			return;
		}

		if (!str_cmp(arg2, "class"))
		{
			send_to_char("classes:\r\n", ch);
			send_to_char("id   class\r\n", ch);
			send_to_char("---- --------------\r\n", ch);
			for (i = 1; i <= CLASS_COUNT; i++)
			{
				if (class_names_table[i].normal)
				{
					snprintf(buf, MAX_STRING_LENGTH, "%-4d %s\r\n", i, class_names_table[i].normal);
					send_to_char(buf, ch);
				}
			}
			return;
		}

		send_to_char("unknown help topic. try: newchar help\r\n", ch);
		return;
	}

	// validate we have all required args
	if (!*arg2 || !*arg3 || !*arg4 || !*arg5)
	{
		send_to_char("usage: newchar <name> <race_id> <class_id> <level> <true|false>\r\n", ch);
		return;
	}

	// validate name
	if (_parse_name(arg1, name_lower))
	{
		send_to_char("invalid name.\r\n", ch);
		return;
	}

	// capitalize properly
	name_lower[0] = toupper(name_lower[0]);
	for (i = 1; name_lower[i]; i++)
		name_lower[i] = tolower(name_lower[i]);

	if (sql_player_exists(name_lower))
	{
		send_to_char("that name is already taken.\r\n", ch);
		return;
	}

	// parse race_id
	race_id = atoi(arg2);
	if (race_id < 0 || race_id > LAST_RACE)
	{
		send_to_char("invalid race id. use 'newchar help race' to see valid ids.\r\n", ch);
		return;
	}

	// check if race is playable
	is_playable = 0;
	for (i = 0; playable_races[i].race_id != -1; i++)
	{
		if (playable_races[i].race_id == race_id)
		{
			is_playable = 1;
			break;
		}
	}
	if (!is_playable)
	{
		send_to_char("that race is not playable. use 'newchar help race'.\r\n", ch);
		return;
	}

	// parse class_id
	class_id = atoi(arg3);
	if (class_id < 1 || class_id > CLASS_COUNT)
	{
		send_to_char("invalid class id. use 'newchar help class' to see valid ids.\r\n", ch);
		return;
	}

	// check race/class combo
	if (class_table[race_id][class_id] == 5)
	{
		send_to_char("that class is not available for that race.\r\n", ch);
		return;
	}

	// parse level
	level = atoi(arg4);
	if (level < 1 || level > 56)
	{
		send_to_char("level must be between 1 and 56.\r\n", ch);
		return;
	}

	// parse max_skills boolean
	if (!str_cmp(arg5, "true") || !str_cmp(arg5, "1") || !str_cmp(arg5, "yes"))
		max_skills = TRUE;
	else if (!str_cmp(arg5, "false") || !str_cmp(arg5, "0") || !str_cmp(arg5, "no"))
		max_skills = FALSE;
	else
	{
		send_to_char("last argument must be true or false.\r\n", ch);
		return;
	}

	// check account char limit
	if (ch->desc->account->num_chars >= MAX_CHARS_PER_ACCOUNT)
	{
		send_to_char("your account already has maximum characters.\r\n", ch);
		return;
	}

	// all validation passed - now allocate
	newch = (P_char)mm_get(dead_mob_pool);
	if (!newch)
	{
		send_to_char("memory allocation failed.\r\n", ch);
		return;
	}
	clear_char(newch);

	ensure_pconly_pool();
	newch->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
	if (!newch->only.pc)
	{
		free_char(newch);
		send_to_char("memory allocation failed.\r\n", ch);
		return;
	}
	memset(newch->only.pc, 0, sizeof(struct pc_only_data));
	newch->only.pc->aggressive = -1;

	// set name
	newch->player.name = str_dup(name_lower);

	// set race
	GET_RACE(newch) = race_id;

	// set class bitmask
	class_bitmask         = 1 << (class_id - 1);
	newch->player.m_class = class_bitmask;
	class_index           = flag2idx(class_bitmask) - 1;

	// set sex (default male)
	newch->player.sex = SEX_MALE;

	// set alignment based on race faction
	for (i = 0; playable_races[i].race_id != -1; i++)
	{
		if (playable_races[i].race_id == race_id)
		{
			if (!strcmp(playable_races[i].faction, "good"))
			{
				GET_ALIGNMENT(newch) = 1000;
				GET_RACEWAR(newch)   = RACEWAR_GOOD;
			}
			else if (!strcmp(playable_races[i].faction, "evil"))
			{
				GET_ALIGNMENT(newch) = -1000;
				GET_RACEWAR(newch)   = RACEWAR_EVIL;
			}
			else
			{
				// neutral race - default to evil
				GET_ALIGNMENT(newch) = -1000;
				GET_RACEWAR(newch)   = RACEWAR_EVIL;
			}
			break;
		}
	}

	// set hometown
	GET_HOME(newch)            = find_hometown(race_id, FALSE);
	GET_BIRTHPLACE(newch)      = GET_HOME(newch);
	GET_ORIG_BIRTHPLACE(newch) = GET_HOME(newch);

	// set all stats to 100
	for (int i = 0; i < MAX_ATTRIBUTES; i++)
		newch->base_stats[i] = 100;
	newch->curr_stats                                                     = newch->base_stats;

	// init_char does pid assignment, skill setup, hp/mana/vitality etc
	init_char(newch);

	// override level after init_char (it may reset to 1)
	newch->player.level = level;

	// override pid to 0 to force db insert
	newch->only.pc->pid = 0;

	// set default player flags and prompt (init_char doesn't do this)
	newch->specials.act = (PLR_PETITION | PLR_ECHO | PLR_SNOTIFY | PLR_PAGING_ON | PLR_MAP);
	SET_BIT(newch->specials.act2, PLR2_NCHAT);
	SET_BIT(newch->specials.act2, PLR2_QUICKCHANT);
	SET_BIT(newch->specials.act2, PLR2_SPEC);
	SET_BIT(newch->specials.act2, PLR2_HINT_CHANNEL);
	SET_BIT(newch->specials.act2, PLR2_SHOW_QUEST);
	SET_BIT(newch->specials.act2, PLR2_BOON);
	SET_BIT(newch->specials.act2, PLR2_SHIPMAP);
	if (!GET_CLASS(newch, CLASS_PALADIN))
		SET_BIT(newch->specials.act, PLR_VICIOUS);

	newch->only.pc->wimpy      = 10;
	newch->only.pc->aggressive = -1;
	newch->only.pc->prompt = (PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MOVE | PROMPT_MAX_MOVE | PROMPT_TANK_NAME | PROMPT_TANK_COND | PROMPT_ENEMY | PROMPT_ENEMY_COND | PROMPT_TWOLINE | PROMPT_STATUS);

	// set skills based on max_skills flag
	if (max_skills)
	{
		// regular skills
		for (i = FIRST_SKILL; i <= LAST_SKILL; i++)
		{
			if (class_index >= 0 && class_index < CLASS_COUNT)
			{
				int rlevel   = skills[i].m_class[class_index].rlevel[0];
				int maxlearn = skills[i].m_class[class_index].maxlearn[0];
				if (rlevel > 0 && rlevel <= level)
				{
					newch->only.pc->skills[i].learned = maxlearn;
					newch->only.pc->skills[i].taught  = maxlearn;
				}
			}
		}

		// epic skills
		for (i = 0; epic_rewards[i].type != 0; i++)
		{
			if (epic_rewards[i].type == EPIC_REWARD_SKILL)
			{
				int          skill_num       = epic_rewards[i].value;
				unsigned int allowed_classes = epic_rewards[i].classes;

				// check if class can have this epic skill (0 means all classes)
				if (allowed_classes == 0 || (allowed_classes & class_bitmask))
				{
					int maxlearn                              = epic_rewards[i].points_cost;
					newch->only.pc->skills[skill_num].learned = maxlearn;
					newch->only.pc->skills[skill_num].taught  = maxlearn;
				}
			}
		}
	}

	// set physical type info
	setCharPhysTypeInfo(newch);

	// save character to db - this assigns auto-increment pid
	if (!writeCharacter(newch, RENT_QUIT, NOWHERE))
	{
		send_to_char("failed to save character to database.\r\n", ch);
		free_char(newch);
		return;
	}

	// now we have a valid pid from db auto-increment
	if (GET_PID(newch) <= 0)
	{
		send_to_char("failed to save character to database.\r\n", ch);
		free_char(newch);
		return;
	}

	// link to account via direct sql
	{
		char account_sql[MAX_STRING_LENGTH * 2 + 1];
		char name_sql[MAX_STRING_LENGTH * 2 + 1];

		mysql_str(ch->desc->account->acct_name, account_sql);
		mysql_str(newch->player.name, name_sql);

		if (!db_query("INSERT INTO account_characters "
		         "(account_name, pid, char_name, created_at, deleted_at) "
		         "VALUES('%s', %ld, '%s', NOW(), NULL) "
		         "ON DUPLICATE KEY UPDATE char_name = VALUES(char_name), deleted_at = NULL",
		         account_sql,
		         GET_PID(newch),
		         name_sql))
		{
			send_to_char("failed to link character to account.\r\n", ch);
			logit(LOG_DEBUG, "wiz_newchar: failed to link %s to account %s", newch->player.name, ch->desc->account->acct_name);
			deleteCharacter(newch, FALSE);
			free_char(newch);
			return;
		}
	}

	// update in-memory account state for immediate visibility
	{
		struct acct_chars *c;
		CREATE(c, struct acct_chars, 1, MEM_TAG_OTHER);
		memset(c, 0, sizeof(struct acct_chars));
		c->charname                            = str_dup(newch->player.name);
		c->count                               = 1;
		c->last                                = time(NULL);
		c->racewar                             = (GET_RACEWAR(newch) == RACEWAR_EVIL) ? ACCT_EVIL : ACCT_GOOD;
		c->next                                = ch->desc->account->acct_character_list;
		ch->desc->account->acct_character_list = c;
		ch->desc->account->num_chars++;
	}

	// log it
	logit(LOG_WIZ, "%s created test character %s (race %d, class %d, level %d, maxskills %s)", GET_NAME(ch), newch->player.name, race_id, class_id, level, max_skills ? "true" : "false");
	wizlog(GET_LEVEL(ch), "%s created test character %s", GET_NAME(ch), newch->player.name);

	// notify the wizard
	snprintf(buf, MAX_STRING_LENGTH, "created %s - %s %s level %d (pid %d)\r\n", newch->player.name, race_names_table[race_id].normal, class_names_table[class_id].normal, level, GET_PID(newch));
	send_to_char(buf, ch);

	if (max_skills)
		send_to_char("all skills maxed.\r\n", ch);

	send_to_char("character saved. log out and select from account menu.\r\n", ch);

	// cleanup temp structures
	free_char(newch);
}
