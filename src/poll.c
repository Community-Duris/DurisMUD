/*
 * poll.c - poll system for durismud
 *
 * allows immortals (level 57+) to create polls
 * mortals (level 30+) can vote and view
 * votes tracked per account (not per character)
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "interp.h"
#include "utils.h"
#include "poll.h"
#include <ctype.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "account.h"
#include "config.h"
#include "json_utils.h"
#include "sql.h"
#include "websocket.h"

using namespace std;

/* in-memory wizard sessions */
static map<P_char, poll_wizard_data> poll_wizards;

/* external declarations */
#ifndef __NO_MYSQL__
extern MYSQL *DB;
#endif

/* forward declarations */
static void poll_handle_vote(P_char ch, char *argument);
static void poll_wizard_show_summary(P_char ch, poll_wizard_data *wiz);
static void poll_display_bar(char *buf, int count, int max_count, int bar_width);
static void poll_send_wrapped(P_char ch, const char *text, int width, const char *prefix, const char *suffix);

/* send wrapped text */
static void poll_send_wrapped(P_char ch, const char *text, int width, const char *prefix, const char *suffix)
{
	char        line[MAX_STRING_LENGTH];
	char        word[256];
	int         line_len = 0;
	int         word_len = 0;
	const char *p        = text;

	line[0] = '\0';

	while (*p)
	{
		/* skip leading spaces */
		while (*p == ' ')
			p++;
		if (!*p)
			break;

		/* get next word */
		word_len = 0;
		while (*p && *p != ' ' && word_len < 255)
		{
			word[word_len++] = *p++;
		}
		word[word_len] = '\0';

		/* check if word fits on current line */
		if (line_len > 0 && line_len + 1 + word_len > width)
		{
			/* flush current line */
			char buf[MAX_STRING_LENGTH];
			snprintf(buf, MAX_STRING_LENGTH, "%s%-*s%s\r\n", prefix, width, line, suffix);
			send_to_char(buf, ch);
			line[0]  = '\0';
			line_len = 0;
		}

		/* add word to line */
		if (line_len > 0)
		{
			strcat(line, " ");
			line_len++;
		}
		strcat(line, word);
		line_len += word_len;
	}

	/* flush remaining text */
	if (line_len > 0)
	{
		char buf[MAX_STRING_LENGTH];
		snprintf(buf, MAX_STRING_LENGTH, "%s%-*s%s\r\n", prefix, width, line, suffix);
		send_to_char(buf, ch);
	}
}

/* wrap text to lines */
static vector<string> poll_wrap_text(const char *text, int width)
{
	vector<string> lines;
	char           line[MAX_STRING_LENGTH];
	char           word[256];
	int            line_len = 0;
	int            word_len = 0;
	const char    *p        = text;

	line[0] = '\0';

	while (*p)
	{
		while (*p == ' ')
			p++;
		if (!*p)
			break;

		word_len = 0;
		while (*p && *p != ' ' && word_len < 255)
		{
			word[word_len++] = *p++;
		}
		word[word_len] = '\0';

		if (line_len > 0 && line_len + 1 + word_len > width)
		{
			lines.push_back(string(line));
			line[0]  = '\0';
			line_len = 0;
		}

		if (line_len > 0)
		{
			strcat(line, " ");
			line_len++;
		}
		strcat(line, word);
		line_len += word_len;
	}

	if (line_len > 0)
	{
		lines.push_back(string(line));
	}

	return lines;
}

/* display option with bar */
static void poll_send_option(P_char ch, int opt_num, const char *text, const char *bar, int count)
{
	vector<string> lines = poll_wrap_text(text, 30);
	char           buf[MAX_STRING_LENGTH];

	/* first line: number + text + bar + count */
	snprintf(buf, MAX_STRING_LENGTH, "&+c|  &+W%2d) &n%-30s &+c[&+g%s&+c] &+W%3d    &+c|\r\n", opt_num, lines.empty() ? "" : lines[0].c_str(), bar, count);
	send_to_char(buf, ch);

	/* continuation lines: indented text */
	for (size_t i = 1; i < lines.size(); i++)
	{
		snprintf(buf, MAX_STRING_LENGTH, "&+c|      %-51s&+c|\r\n", lines[i].c_str());
		send_to_char(buf, ch);
	}
}

/* display option for results */
static void poll_send_option_results(P_char ch, int opt_num, const char *text, const char *bar, int count, float pct)
{
	vector<string> lines = poll_wrap_text(text, 23);
	char           buf[MAX_STRING_LENGTH];

	/* first line: number + text + count + pct + bar */
	snprintf(buf, MAX_STRING_LENGTH, "&+c|  &+W%2d) &n%-23s &+W%3d &+c(&n%5.1f%%&+c) [&+g%s&+c]  &+c|\r\n", opt_num, lines.empty() ? "" : lines[0].c_str(), count, pct, bar);
	send_to_char(buf, ch);

	/* continuation lines: indented text */
	for (size_t i = 1; i < lines.size(); i++)
	{
		snprintf(buf, MAX_STRING_LENGTH, "&+c|      %-51s&+c|\r\n", lines[i].c_str());
		send_to_char(buf, ch);
	}
}

/* format time remaining */
static void format_time_remaining(time_t expires_at, char *buf, size_t buflen)
{
	time_t remaining = expires_at - get_time();

	if (remaining <= 0)
	{
		snprintf(buf, buflen, "expired");
	}
	else if (remaining > 86400)
	{
		snprintf(buf, buflen, "%ldd", (long)(remaining / 86400));
	}
	else if (remaining > 3600)
	{
		snprintf(buf, buflen, "%ldh", (long)(remaining / 3600));
	}
	else if (remaining > 60)
	{
		snprintf(buf, buflen, "%ldm", (long)(remaining / 60));
	}
	else
	{
		snprintf(buf, buflen, "soon");
	}
}

/* check if account voted */
bool poll_has_voted(const char *account_name, int poll_id)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!account_name || !*account_name)
		return false;

	string     acct_esc = escape_str(account_name);
	MYSQL_RES *res      = db_query("SELECT id FROM poll_votes WHERE poll_id = %d AND account_name = '%s' LIMIT 1", poll_id, acct_esc.c_str());

	if (!res)
		return false;

	MYSQL_ROW row   = mysql_fetch_row(res);
	bool      voted = (row != NULL);
	mysql_free_result(res);
	return voted;
#endif
}

/* get all polls */
vector<poll_data> poll_get_all(bool active_only)
{
	vector<poll_data> polls;

#ifndef __NO_MYSQL__
	MYSQL_RES *res;

	if (active_only)
	{
		res = db_query("SELECT id, question, created_by, UNIX_TIMESTAMP(created_at), UNIX_TIMESTAMP(expires_at), is_active, multi_select, max_choices "
		               "FROM polls WHERE is_active = 1 AND expires_at > FROM_UNIXTIME(%ld) ORDER BY id DESC",
		               (long)get_time());
	}
	else
	{
		res = db_query("SELECT id, question, created_by, UNIX_TIMESTAMP(created_at), UNIX_TIMESTAMP(expires_at), is_active, multi_select, max_choices "
		               "FROM polls ORDER BY id DESC");
	}

	if (!res)
		return polls;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		poll_data poll;
		poll.id           = atoi(row[0]);
		poll.question     = row[1] ? row[1] : "";
		poll.created_by   = row[2] ? row[2] : "";
		poll.created_at   = atol(row[3]);
		poll.expires_at   = atol(row[4]);
		poll.is_active    = (atoi(row[5]) == 1);
		poll.multi_select = (atoi(row[6]) == 1);
		poll.max_choices  = atoi(row[7]);
		poll.total_votes  = 0;
		polls.push_back(poll);
	}
	mysql_free_result(res);

	/* vote counts */
	for (size_t i = 0; i < polls.size(); i++)
	{
		res = db_query("SELECT COUNT(DISTINCT account_name) FROM poll_votes WHERE poll_id = %d", polls[i].id);
		if (res)
		{
			row = mysql_fetch_row(res);
			if (row && row[0])
			{
				polls[i].total_votes = atoi(row[0]);
			}
			mysql_free_result(res);
		}
	}
#endif

	return polls;
}

/* get poll by id */
poll_data poll_get_by_id(int poll_id)
{
	poll_data poll;
	poll.id = 0;

#ifndef __NO_MYSQL__
	MYSQL_RES *res = db_query("SELECT id, question, created_by, UNIX_TIMESTAMP(created_at), UNIX_TIMESTAMP(expires_at), is_active, multi_select, max_choices "
	                          "FROM polls WHERE id = %d",
	                          poll_id);

	if (!res)
		return poll;

	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		mysql_free_result(res);
		return poll;
	}

	poll.id           = atoi(row[0]);
	poll.question     = row[1] ? row[1] : "";
	poll.created_by   = row[2] ? row[2] : "";
	poll.created_at   = atol(row[3]);
	poll.expires_at   = atol(row[4]);
	poll.is_active    = (atoi(row[5]) == 1);
	poll.multi_select = (atoi(row[6]) == 1);
	poll.max_choices  = atoi(row[7]);
	poll.total_votes  = 0;
	mysql_free_result(res);

	/* options */
	res = db_query("SELECT id, option_num, option_text FROM poll_options WHERE poll_id = %d ORDER BY option_num", poll_id);
	if (res)
	{
		while ((row = mysql_fetch_row(res)))
		{
			poll_option opt;
			opt.id         = atoi(row[0]);
			opt.option_num = atoi(row[1]);
			opt.text       = row[2] ? row[2] : "";
			opt.vote_count = 0;
			poll.options.push_back(opt);
		}
		mysql_free_result(res);
	}

	/* vote counts per option */
	for (size_t i = 0; i < poll.options.size(); i++)
	{
		res = db_query("SELECT COUNT(*) FROM poll_votes WHERE option_id = %d", poll.options[i].id);
		if (res)
		{
			row = mysql_fetch_row(res);
			if (row && row[0])
			{
				poll.options[i].vote_count = atoi(row[0]);
			}
			mysql_free_result(res);
		}
	}

	/* total voters */
	res = db_query("SELECT COUNT(DISTINCT account_name) FROM poll_votes WHERE poll_id = %d", poll_id);
	if (res)
	{
		row = mysql_fetch_row(res);
		if (row && row[0])
		{
			poll.total_votes = atoi(row[0]);
		}
		mysql_free_result(res);
	}
#endif

	return poll;
}

/* create poll */
bool poll_create(poll_data *poll)
{
#ifdef __NO_MYSQL__
	return false;
#else
	string question_esc = escape_str(poll->question.c_str());
	string creator_esc  = escape_str(poll->created_by.c_str());

	if (!qry("INSERT INTO polls (question, created_by, created_at, expires_at, is_active, multi_select, max_choices) "
	         "VALUES ('%s', '%s', FROM_UNIXTIME(%ld), FROM_UNIXTIME(%ld), 1, %d, %d)",
	         question_esc.c_str(),
	         creator_esc.c_str(),
	         (long)poll->created_at,
	         (long)poll->expires_at,
	         poll->multi_select ? 1 : 0,
	         poll->max_choices))
	{
		return false;
	}

	int poll_id = (int)mysql_insert_id(DB);
	poll->id    = poll_id;

	/* options */
	for (size_t i = 0; i < poll->options.size(); i++)
	{
		string opt_esc = escape_str(poll->options[i].text.c_str());
		qry("INSERT INTO poll_options (poll_id, option_num, option_text) VALUES (%d, %d, '%s')", poll_id, poll->options[i].option_num, opt_esc.c_str());
	}

	return true;
#endif
}

/* close poll */
bool poll_close(int poll_id, P_char ch)
{
#ifdef __NO_MYSQL__
	return false;
#else
	poll_data poll = poll_get_by_id(poll_id);
	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return false;
	}

	if (!poll.is_active)
	{
		send_to_char("That poll is already closed.\r\n", ch);
		return false;
	}

	if (!qry("UPDATE polls SET is_active = 0 WHERE id = %d", poll_id))
	{
		send_to_char("Failed to close poll.\r\n", ch);
		return false;
	}

	char buf[MAX_STRING_LENGTH];
	snprintf(buf, MAX_STRING_LENGTH, "&+W[POLL]&n Poll #%d has been closed by %s.\r\n", poll_id, GET_NAME(ch));
	send_to_all(buf);

	poll_broadcast_close(poll_id, poll.question.c_str());

	return true;
#endif
}

/* cast vote */
int poll_cast_vote(P_char ch, int poll_id, vector<int> &choices)
{
#ifdef __NO_MYSQL__
	return 0;
#else
	const char *acct = get_account_name_safe(ch);
	if (!acct || !strcmp(acct, "Unknown"))
	{
		send_to_char("You must be logged in with an account to vote.\r\n", ch);
		return 0;
	}

	poll_data poll = poll_get_by_id(poll_id);
	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return 0;
	}

	int votes_cast = poll_record_votes(acct, GET_NAME(ch), poll_id, poll, choices);

	/* broadcast */
	if (votes_cast > 0)
	{
		poll = poll_get_by_id(poll_id);
		poll_broadcast_vote(poll_id, poll.total_votes);
	}

	return votes_cast;
#endif
}

/* close expired polls */
void poll_check_expirations(void)
{
#ifndef __NO_MYSQL__
	qry("UPDATE polls SET is_active = 0 WHERE is_active = 1 AND expires_at < FROM_UNIXTIME(%ld)", (long)get_time());
#endif
}

/* record votes to db - shared by command and websocket */
int poll_record_votes(const char *acct_name, const char *char_name, int poll_id, poll_data &poll, vector<int> &choices)
{
#ifdef __NO_MYSQL__
	return 0;
#else
	string acct_esc = escape_str(acct_name);
	string char_esc = escape_str(char_name);

	int votes_cast = 0;
	for (size_t i = 0; i < choices.size(); i++)
	{
		int option_id = 0;
		for (size_t j = 0; j < poll.options.size(); j++)
		{
			if (poll.options[j].option_num == choices[i])
			{
				option_id = poll.options[j].id;
				break;
			}
		}
		if (option_id == 0)
			continue;

		if (qry("INSERT IGNORE INTO poll_votes (poll_id, account_name, option_id, voted_at, char_name) "
		        "VALUES (%d, '%s', %d, FROM_UNIXTIME(%ld), '%s')",
		        poll_id,
		        acct_esc.c_str(),
		        option_id,
		        (long)get_time(),
		        char_esc.c_str()))
		{
			votes_cast++;
		}
	}
	return votes_cast;
#endif
}

/* list polls */
void poll_display_list(P_char ch, bool show_all)
{
	vector<poll_data> polls = poll_get_all(!show_all);

	char buf[MAX_STRING_LENGTH];

	send_to_char("\r\n&+c.---------------------------------------------------------.\r\n", ch);
	if (show_all)
	{
		send_to_char("|  &+WAll Polls&+c                                              |\r\n", ch);
	}
	else
	{
		send_to_char("|  &+WActive Polls&+c                                           |\r\n", ch);
	}
	send_to_char("|---------------------------------------------------------|\r\n", ch);
	send_to_char("| &+YID&+c  |  &+YQuestion&+c                         | &+YExp&+c   | &+YVotes&+c |\r\n", ch);
	send_to_char("|-----|-----------------------------------|-------|-------|\r\n&n", ch);

	if (polls.empty())
	{
		send_to_char("&+c|             &+wNo polls available.&+c                         |\r\n", ch);
	}
	else
	{
		for (size_t i = 0; i < polls.size(); i++)
		{
			string q = polls[i].question;
			if (q.length() > 33)
			{
				q = q.substr(0, 30) + "...";
			}

			char expire_str[16];
			if (!polls[i].is_active)
			{
				snprintf(expire_str, 16, "closed");
			}
			else
			{
				format_time_remaining(polls[i].expires_at, expire_str, 16);
			}

			snprintf(buf, MAX_STRING_LENGTH, "&+c| &+w%3d &+c| &n%-33s &+c| &n%-5s &+c| &+W%5d &+c|\r\n", polls[i].id, q.c_str(), expire_str, polls[i].total_votes);
			send_to_char(buf, ch);
		}
	}

	send_to_char("&+c'---------------------------------------------------------'&n\r\n", ch);
	send_to_char("&+yUse 'poll <id>' to view, 'poll vote <id> <opt>' to vote.&n\r\n", ch);
}

/* show single poll */
void poll_display_single(P_char ch, int poll_id)
{
	poll_data poll = poll_get_by_id(poll_id);

	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return;
	}

	char buf[MAX_STRING_LENGTH];
	char expire_str[16];

	if (!poll.is_active)
	{
		snprintf(expire_str, 16, "closed");
	}
	else
	{
		format_time_remaining(poll.expires_at, expire_str, 16);
	}

	send_to_char("\r\n", ch);
	send_to_char("&+c.---------------------------------------------------------.\r\n", ch);
	snprintf(buf, MAX_STRING_LENGTH, "&+c|  &+WPoll #%-4d                                             &+c|\r\n", poll.id);
	send_to_char(buf, ch);

	/* question */
	poll_send_wrapped(ch, poll.question.c_str(), 55, "&+c|  &+Y", "&+c|");

	send_to_char("&+c|---------------------------------------------------------|\r\n", ch);

	/* metadata */
	snprintf(buf, MAX_STRING_LENGTH, "&+c|  &nBy: &+W%-15s  &nExpires: &+W%-8s  &nVotes: &+W%5d   &+c|\r\n", poll.created_by.c_str(), expire_str, poll.total_votes);
	send_to_char(buf, ch);

	if (poll.multi_select)
	{
		snprintf(buf, MAX_STRING_LENGTH, "&+c|  &nType: &+Wmultiple choice (max %d)                          &+c|\r\n", poll.max_choices);
		send_to_char(buf, ch);
	}

	send_to_char("&+c|---------------------------------------------------------|\r\n", ch);

	/* max votes for bar scaling */
	int max_votes = 1;
	for (size_t i = 0; i < poll.options.size(); i++)
	{
		if (poll.options[i].vote_count > max_votes)
		{
			max_votes = poll.options[i].vote_count;
		}
	}

	/* options */
	for (size_t i = 0; i < poll.options.size(); i++)
	{
		char bar[12];
		poll_display_bar(bar, poll.options[i].vote_count, max_votes, 10);
		poll_send_option(ch, poll.options[i].option_num, poll.options[i].text.c_str(), bar, poll.options[i].vote_count);
	}

	send_to_char("&+c'---------------------------------------------------------'&n\r\n", ch);

	/* already voted? */
	const char *acct = get_account_name_safe(ch);
	if (poll_has_voted(acct, poll_id))
	{
		send_to_char("&+GYou have already voted in this poll.&n\r\n", ch);
	}
	else if (poll.is_active)
	{
		if (poll.multi_select)
		{
			snprintf(buf, MAX_STRING_LENGTH, "&+yVote: poll vote %d <opt1,opt2,...>&n\r\n", poll_id);
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH, "&+yVote: poll vote %d <option>&n\r\n", poll_id);
		}
		send_to_char(buf, ch);
	}
	else
	{
		send_to_char("&+rThis poll is closed.&n\r\n", ch);
	}
}

/* detailed results */
void poll_display_results(P_char ch, int poll_id)
{
	poll_data poll = poll_get_by_id(poll_id);

	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return;
	}

	/* mortals only see closed poll results */
	if (GET_LEVEL(ch) < MINLVLIMMORTAL && poll.is_active)
	{
		send_to_char("You can only view detailed results after a poll closes.\r\n", ch);
		return;
	}

	char buf[MAX_STRING_LENGTH];

	send_to_char("\r\n", ch);
	send_to_char("&+c.---------------------------------------------------------.\r\n", ch);
	snprintf(buf, MAX_STRING_LENGTH, "&+c|  &+WPoll #%-4d Results                                      &+c|\r\n", poll.id);
	send_to_char(buf, ch);

	/* question */
	poll_send_wrapped(ch, poll.question.c_str(), 55, "&+c|  &+Y", "&+c|");

	send_to_char("&+c|---------------------------------------------------------|\r\n", ch);
	snprintf(buf, MAX_STRING_LENGTH, "&+c|  &nStatus: &+W%-8s  &nTotal voters: &+W%-5d                  &+c|\r\n", poll.is_active ? "open" : "closed", poll.total_votes);
	send_to_char(buf, ch);
	send_to_char("&+c|---------------------------------------------------------|\r\n", ch);

	int max_votes = 1;
	for (size_t i = 0; i < poll.options.size(); i++)
	{
		if (poll.options[i].vote_count > max_votes)
		{
			max_votes = poll.options[i].vote_count;
		}
	}

	for (size_t i = 0; i < poll.options.size(); i++)
	{
		float pct = 0.0;
		if (poll.total_votes > 0)
		{
			pct = (float)poll.options[i].vote_count / poll.total_votes * 100.0;
		}

		char bar[12];
		poll_display_bar(bar, poll.options[i].vote_count, max_votes, 10);
		poll_send_option_results(ch, poll.options[i].option_num, poll.options[i].text.c_str(), bar, poll.options[i].vote_count, pct);
	}

	send_to_char("&+c'---------------------------------------------------------'&n\r\n", ch);
}

/* display vote bar */
static void poll_display_bar(char *buf, int count, int max_count, int bar_width)
{
	int filled = 0;
	if (max_count > 0)
	{
		filled = (count * bar_width) / max_count;
	}

	for (int i = 0; i < bar_width; i++)
	{
		buf[i] = (i < filled) ? '#' : ' ';
	}
	buf[bar_width] = '\0';
}

/* voting handler */
static void poll_handle_vote(P_char ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (!*arg1 || !is_number(arg1))
	{
		send_to_char("Usage: poll vote <poll_id> <option(s)>\r\n", ch);
		return;
	}

	int       poll_id = atoi(arg1);
	poll_data poll    = poll_get_by_id(poll_id);

	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return;
	}

	if (!poll.is_active)
	{
		send_to_char("That poll is closed.\r\n", ch);
		return;
	}

	const char *acct = get_account_name_safe(ch);
	if (!acct || !strcmp(acct, "Unknown"))
	{
		send_to_char("You must be logged in with an account to vote.\r\n", ch);
		return;
	}

	if (poll_has_voted(acct, poll_id))
	{
		send_to_char("Your account has already voted in this poll.\r\n", ch);
		return;
	}

	if (!*arg2)
	{
		send_to_char("You must specify which option(s) to vote for.\r\n", ch);
		return;
	}

	/* parse options */
	vector<int> choices;
	char       *token = strtok(arg2, ",");
	while (token)
	{
		while (*token == ' ')
			token++;
		if (is_number(token))
		{
			choices.push_back(atoi(token));
		}
		token = strtok(NULL, ",");
	}

	if (choices.empty())
	{
		send_to_char("Invalid option format.\r\n", ch);
		return;
	}

	if (!poll.multi_select && choices.size() > 1)
	{
		send_to_char("This poll only allows one selection.\r\n", ch);
		return;
	}

	if ((int)choices.size() > poll.max_choices)
	{
		char buf[256];
		snprintf(buf, 256, "You can only select up to %d option(s).\r\n", poll.max_choices);
		send_to_char(buf, ch);
		return;
	}

	/* validate choices */
	for (size_t i = 0; i < choices.size(); i++)
	{
		bool found = false;
		for (size_t j = 0; j < poll.options.size(); j++)
		{
			if (poll.options[j].option_num == choices[i])
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			char buf[256];
			snprintf(buf, 256, "Option %d is not valid for this poll.\r\n", choices[i]);
			send_to_char(buf, ch);
			return;
		}
	}

	/* vote */
	int votes_cast = poll_cast_vote(ch, poll_id, choices);

	if (votes_cast > 0)
	{
		send_to_char("&+GYour vote has been recorded. Thank you for participating!&n\r\n", ch);
	}
	else
	{
		send_to_char("Failed to record your vote.\r\n", ch);
	}
}

/* wizard active? */
bool poll_wizard_active(P_char ch) { return poll_wizards.count(ch) > 0; }

/* cancel wizard */
void poll_wizard_cancel(P_char ch) { poll_wizards.erase(ch); }

/* start wizard */
void poll_wizard_start(P_char ch)
{
	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		send_to_char("Only immortals can create polls.\r\n", ch);
		return;
	}

	poll_wizard_data wiz;
	wiz.state             = POLL_WIZ_QUESTION;
	wiz.current_option    = 1;
	wiz.poll.multi_select = false;
	wiz.poll.max_choices  = 1;
	wiz.poll.created_by   = GET_NAME(ch);
	wiz.poll.is_active    = true;
	wiz.poll.total_votes  = 0;

	poll_wizards[ch] = wiz;

	send_to_char("\r\n&+W=== Poll Creation Wizard ===&n\r\n", ch);
	send_to_char("Type 'cancel' at any time to abort.\r\n\r\n", ch);
	send_to_char("&+YEnter the poll question:&n\r\n", ch);
}

/* show summary */
static void poll_wizard_show_summary(P_char ch, poll_wizard_data *wiz)
{
	char buf[MAX_STRING_LENGTH];

	send_to_char("\r\n&+W=== Poll Summary ===&n\r\n", ch);
	snprintf(buf, MAX_STRING_LENGTH, "&+YQuestion:&n %s\r\n", wiz->poll.question.c_str());
	send_to_char(buf, ch);
	snprintf(buf, MAX_STRING_LENGTH, "&+YType:&n %s", wiz->poll.multi_select ? "multiple choice" : "single choice");
	send_to_char(buf, ch);
	if (wiz->poll.multi_select)
	{
		snprintf(buf, MAX_STRING_LENGTH, " (max %d selections)\r\n", wiz->poll.max_choices);
		send_to_char(buf, ch);
	}
	else
	{
		send_to_char("\r\n", ch);
	}

	time_t duration = wiz->poll.expires_at - get_time();
	snprintf(buf, MAX_STRING_LENGTH, "&+YDuration:&n %ld hours\r\n", (long)(duration / 3600));
	send_to_char(buf, ch);

	send_to_char("&+YOptions:&n\r\n", ch);
	for (size_t i = 0; i < wiz->poll.options.size(); i++)
	{
		snprintf(buf, MAX_STRING_LENGTH, "  %d) %s\r\n", wiz->poll.options[i].option_num, wiz->poll.options[i].text.c_str());
		send_to_char(buf, ch);
	}
}

/* wizard input */
void poll_wizard_handle_input(P_char ch, char *input)
{
	if (!poll_wizards.count(ch))
		return;

	poll_wizard_data &wiz = poll_wizards[ch];
	char             *arg = skip_spaces(input);

	if (!str_cmp(arg, "cancel"))
	{
		poll_wizards.erase(ch);
		send_to_char("Poll creation cancelled.\r\n", ch);
		return;
	}

	char buf[MAX_STRING_LENGTH];

	switch (wiz.state)
	{
		case POLL_WIZ_QUESTION:
			if (strlen(arg) < 10)
			{
				send_to_char("Question too short. Please enter a meaningful question:\r\n", ch);
				return;
			}
			if (strlen(arg) > MAX_POLL_QUESTION - 1)
			{
				send_to_char("Question too long. Please keep it under 512 characters:\r\n", ch);
				return;
			}
			wiz.poll.question = arg;
			wiz.state         = POLL_WIZ_MULTI;
			send_to_char("\r\n&+YAllow multiple selections? (yes/no):&n\r\n", ch);
			break;

		case POLL_WIZ_MULTI:
			if (!str_cmp(arg, "yes") || !str_cmp(arg, "y"))
			{
				wiz.poll.multi_select = true;
				wiz.state             = POLL_WIZ_MAX_CHOICE;
				send_to_char("\r\n&+YHow many choices can voters select? (2-10):&n\r\n", ch);
			}
			else if (!str_cmp(arg, "no") || !str_cmp(arg, "n"))
			{
				wiz.poll.multi_select = false;
				wiz.poll.max_choices  = 1;
				wiz.state             = POLL_WIZ_DURATION;
				send_to_char("\r\n&+YPoll duration in hours (1-720):&n\r\n", ch);
			}
			else
			{
				send_to_char("Please answer yes or no:\r\n", ch);
			}
			break;

		case POLL_WIZ_MAX_CHOICE:
			if (!is_number(arg) || atoi(arg) < 2 || atoi(arg) > 10)
			{
				send_to_char("Please enter a number between 2 and 10:\r\n", ch);
				return;
			}
			wiz.poll.max_choices = atoi(arg);
			wiz.state            = POLL_WIZ_DURATION;
			send_to_char("\r\n&+YPoll duration in hours (1-720):&n\r\n", ch);
			break;

		case POLL_WIZ_DURATION:
			if (!is_number(arg) || atoi(arg) < 1 || atoi(arg) > 720)
			{
				send_to_char("Please enter hours between 1 and 720 (30 days max):\r\n", ch);
				return;
			}
			wiz.poll.created_at = get_time();
			wiz.poll.expires_at = get_time() + (atoi(arg) * 3600);
			wiz.state           = POLL_WIZ_OPTIONS;
			wiz.current_option  = 1;
			send_to_char("\r\n&+YEnter option 1 (or 'done' when finished adding options):&n\r\n", ch);
			break;

		case POLL_WIZ_OPTIONS:
			if (!str_cmp(arg, "done"))
			{
				if (wiz.poll.options.size() < 2)
				{
					send_to_char("You need at least 2 options. Enter another option:\r\n", ch);
					return;
				}
				wiz.state = POLL_WIZ_CONFIRM;
				poll_wizard_show_summary(ch, &wiz);
				send_to_char("\r\n&+YCreate this poll? (yes/no):&n\r\n", ch);
			}
			else
			{
				if (strlen(arg) < 1)
				{
					send_to_char("Option cannot be empty. Enter option or 'done':\r\n", ch);
					return;
				}
				if (strlen(arg) > MAX_OPTION_TEXT - 1)
				{
					send_to_char("Option too long. Please keep it under 256 characters:\r\n", ch);
					return;
				}
				poll_option opt;
				opt.option_num = wiz.current_option;
				opt.text       = arg;
				opt.vote_count = 0;
				opt.id         = 0;
				wiz.poll.options.push_back(opt);
				wiz.current_option++;

				if (wiz.current_option > MAX_POLL_OPTIONS)
				{
					wiz.state = POLL_WIZ_CONFIRM;
					send_to_char("Maximum options reached.\r\n", ch);
					poll_wizard_show_summary(ch, &wiz);
					send_to_char("\r\n&+YCreate this poll? (yes/no):&n\r\n", ch);
				}
				else
				{
					snprintf(buf, MAX_STRING_LENGTH, "\r\n&+YEnter option %d (or 'done' when finished):&n\r\n", wiz.current_option);
					send_to_char(buf, ch);
				}
			}
			break;

		case POLL_WIZ_CONFIRM:
			if (!str_cmp(arg, "yes") || !str_cmp(arg, "y"))
			{
				if (poll_create(&wiz.poll))
				{
					send_to_char("&+GPoll created successfully!&n\r\n", ch);

					/* announce */
					snprintf(buf, MAX_STRING_LENGTH, "&+W[POLL]&n %s has created a new poll: %s\r\n", GET_NAME(ch), wiz.poll.question.c_str());
					send_to_all(buf);

					/* websocket */
					poll_broadcast_new(wiz.poll.id, wiz.poll.question.c_str(), wiz.poll.created_by.c_str());
				}
				else
				{
					send_to_char("&+RError creating poll. Check logs.&n\r\n", ch);
				}
				poll_wizards.erase(ch);
			}
			else if (!str_cmp(arg, "no") || !str_cmp(arg, "n"))
			{
				poll_wizards.erase(ch);
				send_to_char("Poll creation cancelled.\r\n", ch);
			}
			else
			{
				send_to_char("Please answer yes or no:\r\n", ch);
			}
			break;
	}
}

/* broadcasts */
void poll_broadcast_new(int poll_id, const char *question, const char *creator)
{
#ifndef __NO_MYSQL__
	extern struct descriptor_data *descriptor_list;
	struct descriptor_data        *d;

	cJSON *root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "poll_new");

	cJSON *data = cJSON_CreateObject();
	cJSON_AddNumberToObject(data, "id", poll_id);
	cJSON_AddStringToObject(data, "question", question ? question : "");
	cJSON_AddStringToObject(data, "creator", creator ? creator : "");
	cJSON_AddItemToObject(root, "data", data);

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->websocket && d->account)
		{
			websocket_send_text(d, json);
		}
	}

	free(json);
#endif
}

void poll_broadcast_vote(int poll_id, int total_votes)
{
#ifndef __NO_MYSQL__
	extern struct descriptor_data *descriptor_list;
	struct descriptor_data        *d;

	cJSON *root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "poll_update");

	cJSON *data = cJSON_CreateObject();
	cJSON_AddNumberToObject(data, "id", poll_id);
	cJSON_AddNumberToObject(data, "total_votes", total_votes);
	cJSON_AddItemToObject(root, "data", data);

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->websocket && d->account)
		{
			websocket_send_text(d, json);
		}
	}

	free(json);
#endif
}

void poll_broadcast_close(int poll_id, const char *question)
{
#ifndef __NO_MYSQL__
	extern struct descriptor_data *descriptor_list;
	struct descriptor_data        *d;

	cJSON *root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "poll_close");

	cJSON *data = cJSON_CreateObject();
	cJSON_AddNumberToObject(data, "id", poll_id);
	cJSON_AddStringToObject(data, "question", question ? question : "");
	cJSON_AddItemToObject(root, "data", data);

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->websocket && d->account)
		{
			websocket_send_text(d, json);
		}
	}

	free(json);
#endif
}

/* main handler */
void do_poll(P_char ch, char *argument, int cmd)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
	{
		send_to_char("Mobs can't participate in polls.\r\n", ch);
		return;
	}

	if (GET_LEVEL(ch) < MINIMUM_POLL_LEVEL)
	{
		send_to_char("You must be at least level 30 to participate in polls.\r\n", ch);
		return;
	}

	/* wizard mode? */
	if (poll_wizard_active(ch))
	{
		poll_wizard_handle_input(ch, argument);
		return;
	}

	argument = one_argument(argument, arg1);

	/* no arg or "list" */
	if (!*arg1 || !str_cmp(arg1, "list"))
	{
		poll_display_list(ch, false);
		return;
	}

	/* numeric = view */
	if (is_number(arg1))
	{
		poll_display_single(ch, atoi(arg1));
		return;
	}

	/* view */
	if (!str_cmp(arg1, "view"))
	{
		argument = one_argument(argument, arg2);
		if (!*arg2 || !is_number(arg2))
		{
			send_to_char("Usage: poll view <poll_id>\r\n", ch);
			return;
		}
		poll_display_single(ch, atoi(arg2));
		return;
	}

	/* vote */
	if (!str_cmp(arg1, "vote"))
	{
		poll_handle_vote(ch, argument);
		return;
	}

	/* results */
	if (!str_cmp(arg1, "results"))
	{
		argument = one_argument(argument, arg2);
		if (!*arg2 || !is_number(arg2))
		{
			send_to_char("Usage: poll results <poll_id>\r\n", ch);
			return;
		}
		poll_display_results(ch, atoi(arg2));
		return;
	}

	/* imm only below */
	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		send_to_char("Unknown poll command. Try: list, view, vote, results\r\n", ch);
		return;
	}

	/* create */
	if (!str_cmp(arg1, "create"))
	{
		poll_wizard_start(ch);
		return;
	}

	/* close */
	if (!str_cmp(arg1, "close"))
	{
		argument = one_argument(argument, arg2);
		if (!*arg2 || !is_number(arg2))
		{
			send_to_char("Usage: poll close <poll_id>\r\n", ch);
			return;
		}
		poll_close(atoi(arg2), ch);
		return;
	}

	/* all (including closed) */
	if (!str_cmp(arg1, "all"))
	{
		poll_display_list(ch, true);
		return;
	}

	/* unknown */
	send_to_char("Poll commands: list, view, vote, results", ch);
	if (GET_LEVEL(ch) >= MINLVLIMMORTAL)
	{
		send_to_char(", create, close, all", ch);
	}
	send_to_char("\r\n", ch);
}
