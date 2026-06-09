/*
 * ***************************************************************************
 *  File: language.c                                         Part of Duris *
 *  Usage: handle 'foreign' languages
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "spells.h"

/*
 * external variables
 */

char *makedrunk(char *, P_char);

/*
 * Muhahaa, main code
 */

int can_understand_language(P_char speaker, P_char victim)
{
	if (GET_RACEWAR(speaker) == GET_RACEWAR(victim))
		return 1;
	if (!GET_RACEWAR(speaker)) // unaligned NPCs
		return 1;
	if (IS_TRUSTED(speaker) || IS_TRUSTED(victim))
		return 1;
	if (affected_by_spell(speaker, SPELL_COMPREHEND_LANGUAGES)
	    || affected_by_spell(victim, SPELL_COMPREHEND_LANGUAGES))
	{
		// Might want to not always assume the speaker wants
		// to be overheard, but since no one uses the spell,
		// no need to bother for now.
		return 1;
	}
	return 0;
}

void do_speak(P_char ch, char *argument, int cmd)
{
	send_to_char("You mean, 'say'?\n", ch);
}

char *language_known(P_char ch, P_char vict)
{
	// "in some strange language"?
	return "";
}

struct translation_table
{
	const char *OrigString;
	const char *NewString;
};

/* fill this table with language transforms. */
struct translation_table language_table[] = {
	{"mine",  "myne"},
    {"Mine",  "Myne"},
    {"that", "thaet"},
    {"That", "Thaet"},
    {"this",  "thys"},
    {"This",  "Thys"},
    { "the",  "thea"},
    { "The",  "Thea"},
    { "you",   "you"},
    { "You",   "You"},
    {  "me",    "me"},
	{  "Me",    "Me"},
    {   "a",     "e"},
    {   "A",     "E"},
    {   "b",     "c"},
    {   "B",     "C"},
    {   "c",     "d"},
    {   "C",     "D"},
    {   "d",     "f"},
    {   "D",     "F"},
    {   "e",     "i"},
    {   "E",     "I"},
	{   "f",     "g"},
    {   "F",     "G"},
    {   "g",     "h"},
    {   "G",     "H"},
    {   "h",     "j"},
    {   "H",     "J"},
    {   "i",     "o"},
    {   "I",     "O"},
    {   "j",     "k"},
    {   "J",     "K"},
    {   "k",     "l"},
	{   "K",     "L"},
    {   "l",     "m"},
    {   "L",     "M"},
    {   "m",     "n"},
    {   "M",     "N"},
    {   "n",     "p"},
    {   "N",     "P"},
    {   "o",     "u"},
    {   "O",     "U"},
    {   "p",     "q"},
    {   "P",     "Q"},
	{   "q",     "r"},
    {   "Q",     "R"},
    {   "r",     "s"},
    {   "R",     "S"},
    {   "s",     "t"},
    {   "S",     "T"},
    {   "t",     "v"},
    {   "T",     "V"},
    {   "u",     "y"},
    {   "U",     "Y"},
    {   "v",     "w"},
	{   "V",     "W"},
    {   "w",     "x"},
    {   "W",     "X"},
    {   "x",     "z"},
    {   "X",     "Z"},
    {   "y",     "a"},
    {   "Y",     "A"},
    {   "z",     "b"},
    {   "Z",     "B"},
    {    "",      ""}  /*table must end with empty
  string */
};

static void trans_char(char *ntstr, const char *str)
{
	for (int i = 0; *str != '\0'; i++)
	{
		struct translation_table *wijzer = language_table;

		while ((wijzer->OrigString)[0] != '\0')
		{
			if (!strncmp(wijzer->OrigString, str, strlen(wijzer->OrigString)))
			{
				strcpy(ntstr, wijzer->NewString);
				ntstr += strlen(wijzer->NewString);
				str += strlen(wijzer->OrigString);
				break;
			}
			wijzer++;
		}

		/* if letter that occurs that isn't in table */
		if ((wijzer->OrigString)[0] == '\0')
		{
			ntstr[0] = str[0];
			ntstr++;
			str++;
		}
	}
	*ntstr = '\0';
}

/* transformation routine */
/* ch is the one who speaks.. victim the one who reads .. */
char *language_CRYPT(P_char ch, P_char victim, char *message)
{
	char        translation[MAX_INPUT_LENGTH];
	static char string[MAX_INPUT_LENGTH];
	int         len, learned, i, ch_skill, vict_skill;
	ulong       ttl;

	strlcpy(string, message, MAX_INPUT_LENGTH);
	makedrunk(string, ch);

	if (string[0] == 0)
		return string;
	if ((ch == NULL) || (victim == NULL))
	{
		strcpy(string, "");
		return string;
	}

	if (can_understand_language(ch, victim))
		return string; /* no need for translation .. */

	trans_char(translation, string);
	strcpy(string, translation);
	return string;
}

struct drunk_struct
{
	int         min_drunk_level;
	int         number_of_rep;
	const char *replacement[11];
};

char *makedrunk(char *string, P_char ch)
{
	char                buf[MAX_STRING_LENGTH], temp;
	int                 pos     = 0, randomnum;
	struct drunk_struct drunk[] = {
		{3, 10,      {"a", "a", "a", "A", "aa", "ah", "Ah", "ao", "aw", "oa", "ahhhh"}},
		{8,  5,										{"b", "b", "b", "B", "B", "vb"}},
		{3,  5,									  {"c", "c", "C", "cj", "sj", "zj"}},
		{5,  2,														{"d", "d", "D"}},
		{3,  3,												  {"e", "e", "eh", "E"}},
		{4,  5,									{"f", "f", "ff", "fff", "fFf", "F"}},
		{8,  2,														{"g", "g", "G"}},
		{9,  6,							 {"h", "h", "hh", "hhh", "Hhh", "HhH", "H"}},
		{7,  6,							   {"i", "i", "Iii", "ii", "iI", "Ii", "I"}},
		{9,  5,									  {"j", "j", "jj", "Jj", "jJ", "J"}},
		{7,  2,														{"k", "k", "K"}},
		{3,  2,														{"l", "l", "L"}},
		{5,  8,             {"m", "m", "mm", "mmm", "mmmm", "mmmmm", "MmM", "mM", "M"}},
		{6,  6,							  {"n", "n", "nn", "Nn", "nnn", "nNn", "N"}},
		{3,  6,						 {"o", "o", "ooo", "ao", "aOoo", "Ooo", "ooOo"}},
		{3,  2,														{"p", "p", "P"}},
		{5,  5,							  {"q", "q", "Q", "ku", "ququ", "kukeleku"}},
		{4,  2,														{"r", "r", "R"}},
		{2,  5,					 {"s", "ss", "zzZzssZ", "ZSssS", "sSzzsss", "sSss"}},
		{5,  2,														{"t", "t", "T"}},
		{3,  6,					   {"u", "u", "uh", "Uh", "Uhuhhuh", "uhU", "uhhu"}},
		{4,  2,														{"v", "v", "V"}},
		{4,  2,														{"w", "w", "W"}},
		{5,  6,							   {"x", "x", "X", "ks", "iks", "kz", "xz"}},
		{3,  2,														{"y", "y", "Y"}},
		{2,  9, {"z", "z", "ZzzZz", "Zzz", "Zsszzsz", "szz", "sZZz", "ZSz", "zZ", "Z"}}
    };

	if (GET_COND(ch, DRUNK) > 0)
	{
		do
		{
			temp = toupper(*string);
			if ((temp >= 'A') && (temp <= 'Z'))
			{
				if (GET_COND(ch, DRUNK) > drunk[(temp - 'A')].min_drunk_level)
				{
					randomnum = number(0, (drunk[(temp - 'A')].number_of_rep));
					strcpy(&buf[pos], drunk[(temp - 'A')].replacement[randomnum]);
					pos += strlen(drunk[(temp - 'A')].replacement[randomnum]);
				}
				else
					buf[pos++] = *string;
			}
			else
			{
				if ((temp >= '0') && (temp <= '9'))
				{
					temp       = '0' + number(0, 9);
					buf[pos++] = temp;
				}
				else
					buf[pos++] = *string;
			}
		} while (*string++);

		buf[pos] = '\0'; /* Mark end of the string... */
		strcpy(string, buf);
		return (string);
	}
	return (string);
}
